/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "stm32f2xx.h"
#include "wwd_bus_protocol.h"
#include "Platform/wwd_platform_interface.h"
#include "Platform/wwd_bus_interface.h"
#include "Platform/wwd_spi_interface.h"
#include "internal/wifi_image/wwd_wifi_image_interface.h"
#include "wifi_nvram_image.h"
#include "Network/wwd_buffer_interface.h"
#include "string.h" /* for memcpy */
#include "wwd_assert.h"
#include "gpio_irq.h"
#include "platform_common_config.h"

/******************************************************
 *             Constants
 ******************************************************/

#define DMA_TIMEOUT_LOOPS      (10000000)
#define EXTI9_5_IRQChannel     ((u8)0x17)  /* External Line[9:5] Interrupts */

#define DMA1_3_IRQ_CHANNEL     ((u8)DMA1_Stream3_IRQn)

/******************************************************
 *             Structures
 ******************************************************/

/******************************************************
 *             Variables
 ******************************************************/

static host_semaphore_type_t spi_transfer_finished_semaphore;

/******************************************************
 *             Function declarations
 ******************************************************/

#ifndef WICED_DISABLE_MCU_POWERSAVE
extern void stm32f2xx_clocks_needed( void );
extern void stm32f2xx_clocks_not_needed( void );
extern void wake_up_interrupt_notify( void );
#define MCU_CLOCKS_NEEDED()      stm32f2xx_clocks_needed()
#define MCU_CLOCKS_NOT_NEEDED()  stm32f2xx_clocks_not_needed()
#else
#define MCU_CLOCKS_NEEDED()
#define MCU_CLOCKS_NOT_NEEDED()
#endif /* ifndef WICED_DISABLE_MCU_POWERSAVE */


void dma_irq( void );

/******************************************************
 *             Function definitions
 ******************************************************/

static void spi_irq_handler( void* arg )
{
    UNUSED_PARAMETER(arg);

#ifndef WICED_DISABLE_MCU_POWERSAVE
    wake_up_interrupt_notify( );
#endif /* ifndef WICED_DISABLE_MCU_POWERSAVE */

    wiced_platform_notify_irq( );
}

void dma_irq( void )
{
    /* Clear interrupt */
    DMA1->LIFCR = (uint32_t) (0x3F << 22);
    host_rtos_set_semaphore( &spi_transfer_finished_semaphore, WICED_TRUE );
}

wiced_result_t host_platform_bus_init( void )
{
    SPI_InitTypeDef  spi_init;
    DMA_InitTypeDef  dma_init_structure;
    GPIO_InitTypeDef gpio_init_structure;
    NVIC_InitTypeDef nvic_init_structure;

    MCU_CLOCKS_NEEDED();

    host_rtos_init_semaphore(&spi_transfer_finished_semaphore);

    RCC_PCLK1Config( RCC_HCLK_Div2 ); /* Set clock to 18MHz (assuming 72MHz STM32 system clock) */

    /* Enable SPI_SLAVE DMA clock */
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_DMA1, ENABLE );

    /* Enable SPI_SLAVE Periph clock */
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, ENABLE );

    /* Enable GPIO Bank B & C */
    RCC_AHB1PeriphClockCmd( SPI_BUS_CLOCK_BANK_CLK | SPI_BUS_MISO_BANK_CLK | SPI_BUS_MOSI_BANK_CLK | SPI_BUS_CS_BANK_CLK | SPI_IRQ_CLK, ENABLE );

    /* Enable SYSCFG. Needed for selecting EXTI interrupt line */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_SYSCFG, ENABLE );

    /* Setup the interrupt input for WLAN_IRQ */
    gpio_init_structure.GPIO_Mode = GPIO_Mode_IN;
    gpio_init_structure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_100MHz;
    gpio_init_structure.GPIO_Pin = ( 1 << SPI_IRQ_PIN );
    GPIO_Init( SPI_IRQ_BANK, &gpio_init_structure );

    gpio_irq_enable(SPI_IRQ_BANK, SPI_IRQ_PIN, IRQ_TRIGGER_RISING_EDGE, spi_irq_handler, 0);

    /* Setup the SPI lines */
    /* Setup SPI slave select GPIOs */
    gpio_init_structure.GPIO_Mode = GPIO_Mode_AF;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_100MHz;
    gpio_init_structure.GPIO_Pin = ( 1 << SPI_BUS_CLOCK_PIN ) | ( 1 << SPI_BUS_MISO_PIN ) | ( 1 << SPI_BUS_MOSI_PIN );
    GPIO_Init( SPI_BUS_CLOCK_BANK, &gpio_init_structure );
    GPIO_PinAFConfig( SPI_BUS_CLOCK_BANK, SPI_BUS_CLOCK_PIN, GPIO_AF_SPI2 );
    GPIO_PinAFConfig( SPI_BUS_MISO_BANK, SPI_BUS_MISO_PIN, GPIO_AF_SPI2 );
    GPIO_PinAFConfig( SPI_BUS_MOSI_BANK, SPI_BUS_MOSI_PIN, GPIO_AF_SPI2 );

    /* Setup SPI slave select GPIOs */
    gpio_init_structure.GPIO_Mode = GPIO_Mode_OUT;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_100MHz;
    gpio_init_structure.GPIO_Pin = ( 1 << SPI_BUS_CS_PIN );
    GPIO_Init( SPI_BUS_CS_BANK, &gpio_init_structure );
    GPIO_SetBits( SPI_BUS_CS_BANK, ( 1 << SPI_BUS_CS_PIN ) ); /* Set CS high (disabled) */

    /* Set GPIO_B[1:0] to 01 to put WLAN module into gSPI mode */
    gpio_init_structure.GPIO_Mode = GPIO_Mode_OUT;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_100MHz;
    gpio_init_structure.GPIO_Pin = ( 1 << WL_GPIO0_PIN );
    GPIO_Init( WL_GPIO0_BANK, &gpio_init_structure );
    gpio_init_structure.GPIO_Pin = ( 1 << WL_GPIO1_PIN );
    GPIO_Init( WL_GPIO1_BANK, &gpio_init_structure );
    GPIO_SetBits( WL_GPIO0_BANK, ( 1 << WL_GPIO0_PIN ) );
    GPIO_ResetBits( WL_GPIO1_BANK, ( 1 << WL_GPIO1_PIN ) );

    /* Setup DMA for SPI2 RX */
    DMA_DeInit( DMA1_Stream3 );
    dma_init_structure.DMA_Channel = DMA_Channel_0;
    dma_init_structure.DMA_PeripheralBaseAddr = (uint32_t) &SPI2->DR;
    dma_init_structure.DMA_Memory0BaseAddr = 0;
    dma_init_structure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    dma_init_structure.DMA_BufferSize = 0;
    dma_init_structure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma_init_structure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma_init_structure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_init_structure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma_init_structure.DMA_Mode = DMA_Mode_Normal;
    dma_init_structure.DMA_Priority = DMA_Priority_VeryHigh;
    dma_init_structure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    dma_init_structure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    dma_init_structure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    dma_init_structure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init( DMA1_Stream3, &dma_init_structure );

    /* Setup DMA for SPI2 TX */
    DMA_DeInit( DMA1_Stream4 );
    dma_init_structure.DMA_Channel = DMA_Channel_0;
    dma_init_structure.DMA_PeripheralBaseAddr = (uint32_t) &SPI2->DR;
    dma_init_structure.DMA_Memory0BaseAddr = 0;
    dma_init_structure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
    dma_init_structure.DMA_BufferSize = 0;
    dma_init_structure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma_init_structure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma_init_structure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma_init_structure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma_init_structure.DMA_Mode = DMA_Mode_Normal;
    dma_init_structure.DMA_Priority = DMA_Priority_VeryHigh;
    dma_init_structure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    dma_init_structure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    dma_init_structure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    dma_init_structure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init( DMA1_Stream4, &dma_init_structure );

    /* Must be lower priority than the value of configMAX_SYSCALL_INTERRUPT_PRIORITY */
    /* otherwise FreeRTOS will not be able to mask the interrupt */
    /* keep in mind that ARMCM3 interrupt priority logic is inverted, the highest value */
    /* is the lowest priority */
    nvic_init_structure.NVIC_IRQChannel                   = DMA1_3_IRQ_CHANNEL;
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0x3;
    nvic_init_structure.NVIC_IRQChannelSubPriority        = 0x0;
    nvic_init_structure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init( &nvic_init_structure );

    /* Enable DMA for TX */
    SPI_I2S_DMACmd( SPI2, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, ENABLE );

    /* Setup SPI */
    spi_init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi_init.SPI_Mode = SPI_Mode_Master;
    spi_init.SPI_DataSize = SPI_DataSize_8b;
    spi_init.SPI_CPOL = SPI_CPOL_High;
    spi_init.SPI_CPHA = SPI_CPHA_2Edge;
    spi_init.SPI_NSS = SPI_NSS_Soft;
    spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
    spi_init.SPI_CRCPolynomial = (uint16_t) 7;

    /* Init SPI and enable it */
    SPI_Init( SPI2, &spi_init );
    SPI_Cmd( SPI2, ENABLE );

    MCU_CLOCKS_NOT_NEEDED();

    return WICED_SUCCESS;
}

wiced_result_t host_platform_bus_deinit( void )
{
    GPIO_InitTypeDef gpio_init_structure;

    MCU_CLOCKS_NEEDED();

    /* Disable SPI and SPI DMA */
    SPI_Cmd( SPI2, DISABLE );
    SPI_I2S_DeInit( SPI2 );
    SPI_I2S_DMACmd( SPI2, SPI_I2S_DMAReq_Tx | SPI_I2S_DMAReq_Rx, DISABLE );
    DMA_DeInit( DMA1_Stream4 );
    DMA_DeInit( DMA1_Stream3 );

    /* Clear GPIO_B[1:0] */
    gpio_init_structure.GPIO_Mode = GPIO_Mode_AIN;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_100MHz;
    gpio_init_structure.GPIO_Pin = ( 1 << WL_GPIO0_PIN );
    GPIO_Init( WL_GPIO0_BANK, &gpio_init_structure );
    gpio_init_structure.GPIO_Pin = ( 1 << WL_GPIO1_PIN );
    GPIO_Init( WL_GPIO1_BANK, &gpio_init_structure );

    /* Clear SPI slave select GPIOs */
    gpio_init_structure.GPIO_Mode = GPIO_Mode_AIN;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_100MHz;
    gpio_init_structure.GPIO_Pin = ( 1 << SPI_BUS_CS_PIN );
    GPIO_Init( SPI_BUS_CS_BANK, &gpio_init_structure );

    /* Clear the SPI lines */
    gpio_init_structure.GPIO_Mode = GPIO_Mode_AIN;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_100MHz;
    gpio_init_structure.GPIO_Pin = ( 1 << SPI_BUS_CLOCK_PIN ) | ( 1 << SPI_BUS_MISO_PIN ) | ( 1 << SPI_BUS_MOSI_PIN );
    GPIO_Init( SPI_BUS_CLOCK_BANK, &gpio_init_structure );

    gpio_irq_disable( SPI_IRQ_BANK, SPI_IRQ_PIN );

    /* Disable SPI_SLAVE Periph clock and DMA1 clock */
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_SPI2, DISABLE );
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_DMA1, DISABLE );
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_SYSCFG, DISABLE );

    MCU_CLOCKS_NOT_NEEDED();

    return WICED_SUCCESS;
}

wiced_result_t host_platform_spi_transfer( bus_transfer_direction_t dir, uint8_t* buffer, uint16_t buffer_length )
{
    wiced_result_t result;
    uint32_t junk;

    MCU_CLOCKS_NEEDED();

    DMA1_Stream4->NDTR = buffer_length;
    DMA1_Stream4->M0AR = (uint32_t) buffer;
    if ( dir == BUS_READ )
    {
        DMA1_Stream3->NDTR = buffer_length;
        DMA1_Stream3->M0AR = (uint32_t) buffer;
        DMA1_Stream3->CR |= DMA_MemoryInc_Enable  | ( 1 << 4);
    }
    else
    {
        DMA1_Stream3->NDTR = buffer_length;
        DMA1_Stream3->M0AR = (uint32_t) &junk;
        DMA1_Stream3->CR &= ( ~DMA_MemoryInc_Enable ) | ( 1 << 4);
    }

    GPIO_ResetBits( SPI_BUS_CS_BANK, ( 1 << SPI_BUS_CS_PIN ) ); /* CS low (to select) */
    DMA_Cmd( DMA1_Stream3, ENABLE );
    DMA_Cmd( DMA1_Stream4, ENABLE );

    /* Wait for DMA TX to complete */
    result = host_rtos_get_semaphore( &spi_transfer_finished_semaphore, 100, WICED_TRUE );
//    loop_count = 0;
//    while ( ( DMA_GetFlagStatus( DMA1_Stream3, DMA_FLAG_TCIF3 ) == RESET ) && ( loop_count < (uint32_t) DMA_TIMEOUT_LOOPS ) )
//    {
//        loop_count++;
//    }

    DMA_Cmd( DMA1_Stream3, DISABLE );
    DMA_Cmd( DMA1_Stream4, DISABLE );

    /* Clear the CS pin and the DMA status flag */
    GPIO_SetBits( SPI_BUS_CS_BANK, ( 1 << SPI_BUS_CS_PIN ) ); /* CS high (to deselect) */
    DMA_ClearFlag( DMA1_Stream3, DMA_FLAG_TCIF4 );
    DMA_ClearFlag( DMA1_Stream4, DMA_FLAG_TCIF3 );

    MCU_CLOCKS_NOT_NEEDED();

    return result;
}
