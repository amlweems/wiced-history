/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "wwd_bus_protocol.h"
#include "Platform/wwd_sdio_interface.h"
#include "Platform/wwd_bus_interface.h"
#include "stm32f2xx.h"
#include "stm32f2xx_syscfg.h"
#include "misc.h"
#include "wwd_assert.h"
#include "RTOS/wwd_rtos_interface.h"
#include "string.h" /* For memcpy */
#include "gpio_irq.h"
#include "platform_common_config.h"

/* Powersave functionality */
#ifndef WICED_DISABLE_MCU_POWERSAVE
extern void stm32f2xx_clocks_needed( void );
extern void stm32f2xx_clocks_not_needed( void );
extern void wake_up_interrupt_notify( void );

#define MCU_CLOCKS_NEEDED()         stm32f2xx_clocks_needed()
#define MCU_CLOCKS_NOT_NEEDED()     stm32f2xx_clocks_not_needed()
#define MCU_NOTIFY_WAKE_UP()        wake_up_interrupt_notify()
#else
#define MCU_CLOCKS_NEEDED()
#define MCU_CLOCKS_NOT_NEEDED()
#define MCU_NOTIFY_WAKE_UP()
#endif /* ifndef WICED_DISABLE_MCU_POWERSAVE */

/******************************************************
 *             Constants
 ******************************************************/

#define COMMAND_FINISHED_CMD52_TIMEOUT_LOOPS (100000)
#define COMMAND_FINISHED_CMD53_TIMEOUT_LOOPS (100000)
#define SDIO_TX_RX_COMPLETE_TIMEOUT_LOOPS    (100000)
#define SDIO_DMA_TIMEOUT_LOOPS               (1000000)
#define MAX_TIMEOUTS                         (30)

#define SDIO_ERROR_MASK   ( SDIO_STA_CCRCFAIL | SDIO_STA_DCRCFAIL | SDIO_STA_CTIMEOUT | SDIO_STA_DTIMEOUT | SDIO_STA_TXUNDERR | SDIO_STA_RXOVERR | SDIO_STA_STBITERR )

static const uint32_t bus_direction_mapping[] =
{
    [BUS_READ]  = SDIO_TransferDir_ToSDIO,
    [BUS_WRITE] = SDIO_TransferDir_ToCard
};

#define SDIO_IRQ_CHANNEL       ((u8)0x31)
#define DMA2_3_IRQ_CHANNEL     ((u8)DMA2_Stream3_IRQn)

#define WL_GPIO_INTR_PIN_NUM   EXTI_PinSource0
#define WL_GPIO_INTR_PORT_SRC  EXTI_PortSourceGPIOB
#define WL_GPIO_INTR_ILINE     EXTI_Line0
#define WL_GPIO_INTR_CHAN      0x06

#define BUS_LEVEL_MAX_RETRIES   5

#define SDIO_ENUMERATION_TIMEOUT_MS    (500)

/******************************************************
 *             Structures
 ******************************************************/

typedef struct
{
    /*@shared@*/ /*@null@*/ uint8_t* data;
    uint16_t length;
} sdio_dma_segment_t;

/******************************************************
 *             Variables
 ******************************************************/

static uint8_t   temp_dma_buffer[2*1024];
static uint8_t*  user_data;
static uint32_t  user_data_size;
static uint8_t*  dma_data_source;
static uint32_t  dma_transfer_size;

static host_semaphore_type_t sdio_transfer_finished_semaphore;

static wiced_bool_t             sdio_transfer_failed;
static bus_transfer_direction_t current_transfer_direction;
static uint32_t                 current_command;

/******************************************************
 *             Function declarations
 ******************************************************/

static uint32_t          sdio_get_blocksize_dctrl   ( sdio_block_size_t block_size );
static sdio_block_size_t find_optimal_block_size    ( uint32_t data_size );
static void              sdio_prepare_data_transfer ( bus_transfer_direction_t direction, sdio_block_size_t block_size, /*@unique@*/ uint8_t* data, uint16_t data_size ) /*@modifies dma_data_source, user_data, user_data_size, dma_transfer_size@*/;

void dma_irq ( void );

/******************************************************
 *             Function definitions
 ******************************************************/

static void sdio_oob_irq_handler( void* arg )
{
    UNUSED_PARAMETER(arg);
    MCU_NOTIFY_WAKE_UP( );
    wiced_platform_notify_irq( );
}

void sdio_irq( void )
{
    uint32_t intstatus = SDIO->STA;

    if ( ( intstatus & ( SDIO_STA_CCRCFAIL | SDIO_STA_DCRCFAIL | SDIO_STA_TXUNDERR | SDIO_STA_RXOVERR  | SDIO_STA_STBITERR )) != 0 )
    {
        //wiced_assert("sdio error flagged",0);
        sdio_transfer_failed = WICED_TRUE;
        SDIO->ICR = (uint32_t) 0xffffffff;
        host_rtos_set_semaphore( &sdio_transfer_finished_semaphore, WICED_TRUE );
    }
    else
    {
        if ((intstatus & (SDIO_STA_CMDREND | SDIO_STA_CMDSENT)) != 0)
        {
            if ( ( SDIO->RESP1 & 0x800 ) != 0 )
            {
                sdio_transfer_failed = WICED_TRUE;
                host_rtos_set_semaphore( &sdio_transfer_finished_semaphore, WICED_TRUE );
            }
            else if (current_command == SDIO_CMD_53)
            {
                if (current_transfer_direction == BUS_WRITE)
                {
                    DMA2_Stream3->CR = DMA_DIR_MemoryToPeripheral |
                                       DMA_Channel_4 | DMA_PeripheralInc_Disable | DMA_MemoryInc_Enable |
                                       DMA_PeripheralDataSize_Word | DMA_MemoryDataSize_Word |
                                       DMA_Mode_Normal | DMA_Priority_VeryHigh |
                                       DMA_MemoryBurst_INC4 | DMA_PeripheralBurst_INC4 | DMA_SxCR_PFCTRL | DMA_SxCR_EN | DMA_SxCR_TCIE;
                }
                else
                {
                    DMA2_Stream3->CR = DMA_DIR_PeripheralToMemory |
                                       DMA_Channel_4 | DMA_PeripheralInc_Disable | DMA_MemoryInc_Enable |
                                       DMA_PeripheralDataSize_Word | DMA_MemoryDataSize_Word |
                                       DMA_Mode_Normal | DMA_Priority_VeryHigh |
                                       DMA_MemoryBurst_INC4 | DMA_PeripheralBurst_INC4 | DMA_SxCR_PFCTRL | DMA_SxCR_EN | DMA_SxCR_TCIE;
                }
            }

            /* Clear all command/response interrupts */
            SDIO->ICR = (SDIO_STA_CMDREND | SDIO_STA_CMDSENT);
        }

        /* Check whether the external interrupt was triggered */
        if ( ( intstatus & SDIO_STA_SDIOIT ) != 0 )
        {
            /* Clear the interrupt and then inform WICED thread */
            SDIO->ICR = SDIO_ICR_SDIOITC;
            wiced_platform_notify_irq( );
        }
    }
}

/*@-exportheader@*/ /* Function picked up by linker script */
void dma_irq( void )
{
    wiced_result_t result;

    /* Clear interrupt */
    DMA2->LIFCR = (uint32_t) (0x3F << 22);

    result = host_rtos_set_semaphore( &sdio_transfer_finished_semaphore, WICED_TRUE );

    /* check result if in debug mode */
    wiced_assert( "failed to set dma semaphore", result == WICED_SUCCESS );

    /*@-noeffect@*/
    (void) result; /* ignore result if in release mode */
    /*@+noeffect@*/
}
/*@+exportheader@*/

static void sdio_enable_bus_irq( void )
{
    SDIO->MASK = SDIO_MASK_SDIOITIE | SDIO_MASK_CMDRENDIE | SDIO_MASK_CMDSENTIE;
}

static void sdio_disable_bus_irq( void )
{
    SDIO->MASK = 0;
}

wiced_result_t host_enable_oob_interrupt( void )
{
    GPIO_InitTypeDef gpio_init_structure;

    RCC_AHB1PeriphClockCmd( WL_GPIO0_BANK_CLK | WL_GPIO1_BANK_CLK, ENABLE );

    /* Set GPIO_B[1:0] to input. One of them will be re-purposed as OOB interrupt */
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_IN;
    gpio_init_structure.GPIO_OType = GPIO_OType_OD;
    gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    gpio_init_structure.GPIO_Pin   = ( 1 << WL_GPIO0_PIN );
    GPIO_Init( WL_GPIO0_BANK, &gpio_init_structure );
    gpio_init_structure.GPIO_Pin   = ( 1 << WL_GPIO1_PIN );
    GPIO_Init( WL_GPIO1_BANK, &gpio_init_structure );

    gpio_irq_enable( SDIO_OOB_IRQ_BANK, SDIO_OOB_IRQ_PIN, IRQ_TRIGGER_RISING_EDGE, sdio_oob_irq_handler, 0 );
    return WICED_SUCCESS;
}

uint8_t host_platform_get_oob_interrupt_pin( void )
{
    if ( ( WL_GPIO1_BANK == SDIO_OOB_IRQ_BANK ) && ( WL_GPIO1_PIN == SDIO_OOB_IRQ_PIN ) )
    {
        /* WLAN GPIO1 */
        return 1;
    }
    else
    {
        /* WLAN GPIO0 */
        return 0;
    }
}

wiced_result_t host_platform_bus_init( void )
{
    SDIO_InitTypeDef sdio_init_structure;
    NVIC_InitTypeDef nvic_init_structure;
    wiced_result_t result;

    MCU_CLOCKS_NEEDED();

    result = host_rtos_init_semaphore( &sdio_transfer_finished_semaphore );
    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Turn on SDIO IRQ */
    SDIO->ICR = (uint32_t) 0xffffffff;
    nvic_init_structure.NVIC_IRQChannel                   = SDIO_IRQ_CHANNEL;
    /* Must be lower priority than the value of configMAX_SYSCALL_INTERRUPT_PRIORITY */
    /* otherwise FreeRTOS will not be able to mask the interrupt */
    /* keep in mind that ARMCM3 interrupt priority logic is inverted, the highest value */
    /* is the lowest priority */
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0x2;
    nvic_init_structure.NVIC_IRQChannelSubPriority        = 0x0;
    nvic_init_structure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init( &nvic_init_structure );

    nvic_init_structure.NVIC_IRQChannel                   = DMA2_3_IRQ_CHANNEL;
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = (uint8_t) 0x3;
    nvic_init_structure.NVIC_IRQChannelSubPriority        = 0x0;
    nvic_init_structure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init( &nvic_init_structure );

    RCC_AHB1PeriphClockCmd( SDIO_CMD_BANK_CLK | SDIO_CLK_BANK_CLK | SDIO_D0_BANK_CLK | SDIO_D1_BANK_CLK | SDIO_D2_BANK_CLK | SDIO_D3_BANK_CLK | WL_GPIO0_BANK_CLK | WL_GPIO1_BANK_CLK, ENABLE );

    /* Set GPIO_B[1:0] to 00 to put WLAN module into SDIO mode */
    WL_GPIO0_BANK->MODER   |= (GPIO_Mode_OUT << (2*WL_GPIO0_PIN));
    WL_GPIO1_BANK->MODER   |= (GPIO_Mode_OUT << (2*WL_GPIO1_PIN));
    WL_GPIO0_BANK->OSPEEDR |= (GPIO_Speed_50MHz << (2*WL_GPIO0_PIN));
    WL_GPIO1_BANK->OSPEEDR |= (GPIO_Speed_50MHz << (2*WL_GPIO1_PIN));
    WL_GPIO0_BANK->BSRRH    = 1 << WL_GPIO0_PIN;
    WL_GPIO1_BANK->BSRRH    = 1 << WL_GPIO1_PIN;

    /* Setup GPIO pins for SDIO data & clock */
    SDIO_CMD_BANK->MODER |= (GPIO_Mode_AF << (2*SDIO_CMD_PIN));
    SDIO_CLK_BANK->MODER |= (GPIO_Mode_AF << (2*SDIO_CLK_PIN));
    SDIO_D0_BANK->MODER  |= (GPIO_Mode_AF << (2*SDIO_D0_PIN));
    SDIO_D1_BANK->MODER  |= (GPIO_Mode_AF << (2*SDIO_D1_PIN));
    SDIO_D2_BANK->MODER  |= (GPIO_Mode_AF << (2*SDIO_D2_PIN));
    SDIO_D3_BANK->MODER  |= (GPIO_Mode_AF << (2*SDIO_D3_PIN));

    SDIO_CMD_BANK->OSPEEDR |= (GPIO_Speed_50MHz << (2*SDIO_CMD_PIN));
    SDIO_CLK_BANK->OSPEEDR |= (GPIO_Speed_50MHz << (2*SDIO_CLK_PIN));
    SDIO_D0_BANK->OSPEEDR  |= (GPIO_Speed_50MHz << (2*SDIO_D0_PIN));
    SDIO_D1_BANK->OSPEEDR  |= (GPIO_Speed_50MHz << (2*SDIO_D1_PIN));
    SDIO_D2_BANK->OSPEEDR  |= (GPIO_Speed_50MHz << (2*SDIO_D2_PIN));
    SDIO_D3_BANK->OSPEEDR  |= (GPIO_Speed_50MHz << (2*SDIO_D3_PIN));

    SDIO_CMD_BANK->PUPDR |= (GPIO_PuPd_UP << (2*SDIO_CMD_PIN));
    SDIO_CLK_BANK->PUPDR |= (GPIO_PuPd_UP << (2*SDIO_CLK_PIN));
    SDIO_D0_BANK->PUPDR  |= (GPIO_PuPd_UP << (2*SDIO_D0_PIN));
    SDIO_D1_BANK->PUPDR  |= (GPIO_PuPd_UP << (2*SDIO_D1_PIN));
    SDIO_D2_BANK->PUPDR  |= (GPIO_PuPd_UP << (2*SDIO_D2_PIN));
    SDIO_D3_BANK->PUPDR  |= (GPIO_PuPd_UP << (2*SDIO_D3_PIN));

    SDIO_CMD_BANK->AFR[SDIO_CMD_PIN >> 0x03] |= (GPIO_AF_SDIO << (4*(SDIO_CMD_PIN & 0x07)));
    SDIO_CLK_BANK->AFR[SDIO_CLK_PIN >> 0x03] |= (GPIO_AF_SDIO << (4*(SDIO_CLK_PIN & 0x07)));
    SDIO_D0_BANK->AFR[SDIO_D0_PIN >> 0x03]   |= (GPIO_AF_SDIO << (4*(SDIO_D0_PIN & 0x07)));
    SDIO_D1_BANK->AFR[SDIO_D1_PIN >> 0x03]   |= (GPIO_AF_SDIO << (4*(SDIO_D1_PIN & 0x07)));
    SDIO_D2_BANK->AFR[SDIO_D2_PIN >> 0x03]   |= (GPIO_AF_SDIO << (4*(SDIO_D2_PIN & 0x07)));
    SDIO_D3_BANK->AFR[SDIO_D3_PIN >> 0x03]   |= (GPIO_AF_SDIO << (4*(SDIO_D3_PIN & 0x07)));

    /*!< Enable the SDIO AHB Clock and the DMA2 Clock */
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_DMA2, ENABLE );
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_SDIO, ENABLE );

    SDIO_DeInit( );
    sdio_init_structure.SDIO_ClockDiv       = (uint8_t) 120; /* 0x78, clock is taken from the high speed APB bus ; */ /* About 400KHz */
    sdio_init_structure.SDIO_ClockEdge      = SDIO_ClockEdge_Rising;
    sdio_init_structure.SDIO_ClockBypass    = SDIO_ClockBypass_Disable;
    sdio_init_structure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Enable;
    sdio_init_structure.SDIO_BusWide        = SDIO_BusWide_1b;
    sdio_init_structure.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;
    SDIO_Init( &sdio_init_structure );
    SDIO_SetPowerState( SDIO_PowerState_ON );
    SDIO_SetSDIOReadWaitMode( SDIO_ReadWaitMode_CLK );
    SDIO_ClockCmd( ENABLE );

    MCU_CLOCKS_NOT_NEEDED();

    return WICED_SUCCESS;
}

wiced_result_t host_platform_sdio_enumerate( void )
{
    wiced_result_t result;
    uint32_t       loop_count;
    uint32_t       data = 0;

    loop_count = 0;
    do
    {
        /* Send CMD0 to set it to idle state */
        host_platform_sdio_transfer( BUS_WRITE, SDIO_CMD_0, SDIO_BYTE_MODE, SDIO_1B_BLOCK, 0, 0, 0, NO_RESPONSE, NULL );

        /* CMD5. */
        host_platform_sdio_transfer( BUS_READ, SDIO_CMD_5, SDIO_BYTE_MODE, SDIO_1B_BLOCK, 0, 0, 0, NO_RESPONSE, NULL );

        /* Send CMD3 to get RCA. */
        result = host_platform_sdio_transfer( BUS_READ, SDIO_CMD_3, SDIO_BYTE_MODE, SDIO_1B_BLOCK, 0, 0, 0, RESPONSE_NEEDED, &data );
        loop_count++;
        if ( loop_count >= (uint32_t) SDIO_ENUMERATION_TIMEOUT_MS )
        {
            return WICED_TIMEOUT;
        }
    } while ( ( result != WICED_SUCCESS ) && ( host_rtos_delay_milliseconds( (uint32_t) 1 ), ( 1 == 1 ) ) );
    /* If you're stuck here, check the platform matches your hardware */

    /* Send CMD7 with the returned RCA to select the card */
    host_platform_sdio_transfer( BUS_WRITE, SDIO_CMD_7, SDIO_BYTE_MODE, SDIO_1B_BLOCK, data, 0, 0, RESPONSE_NEEDED, NULL );

    return WICED_SUCCESS;
}

wiced_result_t host_platform_bus_deinit( void )
{
    NVIC_InitTypeDef nvic_init_structure;
    wiced_result_t   result;

    result = host_rtos_deinit_semaphore( &sdio_transfer_finished_semaphore );

    MCU_CLOCKS_NEEDED();

    /* Disable SPI and SPI DMA */
    sdio_disable_bus_irq( );
    SDIO_ClockCmd( DISABLE );
    SDIO_SetPowerState( SDIO_PowerState_OFF );
    SDIO_DeInit( );
//    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_DMA2, DISABLE );
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_SDIO, DISABLE );

    /* Clear GPIO pins for SDIO data & clock */
    SDIO_CMD_BANK->MODER  &= ((uint32_t) ~(3 << (2*SDIO_CMD_PIN)));
    SDIO_CLK_BANK->MODER  &= ((uint32_t) ~(3 << (2*SDIO_CLK_PIN)));
    SDIO_D0_BANK->MODER   &= ((uint32_t) ~(3 << (2*SDIO_D0_PIN )));
    SDIO_D1_BANK->MODER   &= ((uint32_t) ~(3 << (2*SDIO_D1_PIN )));
    SDIO_D2_BANK->MODER   &= ((uint32_t) ~(3 << (2*SDIO_D2_PIN )));
    SDIO_D3_BANK->MODER   &= ((uint32_t) ~(3 << (2*SDIO_D3_PIN )));
    SDIO_CMD_BANK->OTYPER &= ((uint32_t) ~(GPIO_OType_OD << SDIO_CMD_PIN));
    SDIO_CLK_BANK->OTYPER &= ((uint32_t) ~(GPIO_OType_OD << SDIO_CLK_PIN));
    SDIO_D0_BANK->OTYPER  &= ((uint32_t) ~(GPIO_OType_OD << SDIO_D0_PIN ));
    SDIO_D1_BANK->OTYPER  &= ((uint32_t) ~(GPIO_OType_OD << SDIO_D1_PIN ));
    SDIO_D2_BANK->OTYPER  &= ((uint32_t) ~(GPIO_OType_OD << SDIO_D2_PIN ));
    SDIO_D3_BANK->OTYPER  &= ((uint32_t) ~(GPIO_OType_OD << SDIO_D3_PIN ));

    /* Clear GPIO_B[1:0] */
    WL_GPIO0_BANK->MODER  &= ((uint32_t) ~(3 << (2*WL_GPIO0_PIN)));
    WL_GPIO1_BANK->MODER  &= ((uint32_t) ~(3 << (2*WL_GPIO1_PIN)));

    /* Turn off SDIO IRQ */
    nvic_init_structure.NVIC_IRQChannel                   = SDIO_IRQ_CHANNEL;
    nvic_init_structure.NVIC_IRQChannelCmd                = DISABLE;
    nvic_init_structure.NVIC_IRQChannelPreemptionPriority = 0x0;
    nvic_init_structure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_Init( &nvic_init_structure );

    MCU_CLOCKS_NOT_NEEDED();

    return result;
}

wiced_result_t host_platform_sdio_transfer( bus_transfer_direction_t direction, sdio_command_t command, sdio_transfer_mode_t mode, sdio_block_size_t block_size, uint32_t argument, /*@null@*/ uint32_t* data, uint16_t data_size, sdio_response_needed_t response_expected, /*@out@*/ /*@null@*/ uint32_t* response )
{
    uint32_t loop_count = 0;
    wiced_result_t result;
    uint16_t attempts = 0;

    wiced_assert("Bad args", !((command == SDIO_CMD_53) && (data == NULL)));

    if ( response != NULL )
    {
        *response = 0;
    }

    MCU_CLOCKS_NEEDED();

    /* Ensure the bus isn't stuck half way through transfer */
    DMA2_Stream3->CR   = 0;

restart:
    SDIO->ICR = (uint32_t) 0xFFFFFFFF;
    sdio_transfer_failed = WICED_FALSE;
    ++attempts;

    /* Check if we've tried too many times */
    if (attempts >= (uint16_t) BUS_LEVEL_MAX_RETRIES)
    {
        result = WICED_ERROR;
        goto exit;
    }

    /* Prepare the data transfer register */
    current_command = command;
    if ( command == SDIO_CMD_53 )
    {
        sdio_enable_bus_irq();

        /* Dodgy STM32 hack to set the CMD53 byte mode size to be the same as the block size */
        if ( mode == SDIO_BYTE_MODE )
        {
            block_size = find_optimal_block_size( data_size );
            if ( block_size < SDIO_512B_BLOCK )
            {
                argument = ( argument & (uint32_t) ( ~0x1FF ) ) | block_size;
            }
            else
            {
                argument = ( argument & (uint32_t) ( ~0x1FF ) );
            }
        }

        /* Prepare the SDIO for a data transfer */
        current_transfer_direction = direction;
        sdio_prepare_data_transfer( direction, block_size, (uint8_t*) data, data_size );

        /* Send the command */
        SDIO->ARG = argument;
        SDIO->CMD = (uint32_t) ( command | SDIO_Response_Short | SDIO_Wait_No | SDIO_CPSM_Enable );

        /* Wait for the whole transfer to complete */
        result = host_rtos_get_semaphore( &sdio_transfer_finished_semaphore, (uint32_t) 50, WICED_TRUE );
        if ( result != WICED_SUCCESS )
        {
            goto exit;
        }

        if ( sdio_transfer_failed == WICED_TRUE )
        {
            goto restart;
        }

        /* Check if there were any SDIO errors */
        if ( ( SDIO->STA & ( SDIO_STA_DTIMEOUT | SDIO_STA_CTIMEOUT ) ) != 0 )
        {
            goto restart;
        }
        else if ( ( ( SDIO->STA & ( SDIO_STA_CCRCFAIL | SDIO_STA_DCRCFAIL | SDIO_STA_TXUNDERR | SDIO_STA_RXOVERR ) ) != 0 ) )
        {
            wiced_assert( "SDIO communication failure", 0 );
            goto restart;
        }

        /* Wait till complete */
        loop_count = (uint32_t) SDIO_TX_RX_COMPLETE_TIMEOUT_LOOPS;
        do
        {
            loop_count--;
            if ( loop_count == 0 || ( ( SDIO->STA & SDIO_ERROR_MASK ) != 0 ) )
            {
                goto restart;
            }
        } while ( ( SDIO->STA & ( SDIO_STA_TXACT | SDIO_STA_RXACT ) ) != 0 );

        if ( direction == BUS_READ )
        {
            memcpy( user_data, dma_data_source, (size_t) user_data_size );
        }
    }
    else
    {
        uint32_t temp_sta;

        /* Send the command */
        SDIO->ARG = argument;
        SDIO->CMD = (uint32_t) ( command | SDIO_Response_Short | SDIO_Wait_No | SDIO_CPSM_Enable );

        loop_count = (uint32_t) COMMAND_FINISHED_CMD52_TIMEOUT_LOOPS;
        do
        {
            temp_sta = SDIO->STA;
            loop_count--;
            if ( loop_count == 0 || ( ( response_expected == RESPONSE_NEEDED ) && ( ( temp_sta & SDIO_ERROR_MASK ) != 0 ) ) )
            {
                goto restart;
            }
        } while ( ( temp_sta & SDIO_FLAG_CMDACT ) != 0 );
    }

    if ( response != NULL )
    {
        *response = SDIO->RESP1;
    }
    result = WICED_SUCCESS;

exit:
    MCU_CLOCKS_NOT_NEEDED();
    SDIO->MASK = SDIO_MASK_SDIOITIE;
    return result;
}


static void sdio_prepare_data_transfer( bus_transfer_direction_t direction, sdio_block_size_t block_size, /*@unique@*/ uint8_t* data, uint16_t data_size ) /*@modifies dma_data_source, user_data, user_data_size, dma_transfer_size@*/
{
    /* Setup a single transfer using the temp buffer */
    user_data         = data;
    user_data_size    = data_size;
    dma_transfer_size = (uint32_t) ( ( ( data_size + (uint16_t) block_size - 1 ) / (uint16_t) block_size ) * (uint16_t) block_size );

    if ( direction == BUS_WRITE )
    {
        dma_data_source = data;
    }
    else
    {
        dma_data_source = temp_dma_buffer;
    }

    SDIO->DTIMER = (uint32_t) 0xFFFFFFFF;
    SDIO->DLEN   = dma_transfer_size;
    SDIO->DCTRL  = (uint32_t)sdio_get_blocksize_dctrl(block_size) | bus_direction_mapping[(int)direction] | SDIO_TransferMode_Block | SDIO_DPSM_Enable | (1 << 3) | (1 << 11);

    /* DMA2 Stream3 */
    DMA2_Stream3->CR   = 0;
    DMA2->LIFCR        = (uint32_t) ( 0x3F << 22 );
    DMA2_Stream3->FCR  = (uint32_t) ( 0x00000021 | DMA_FIFOMode_Enable | DMA_FIFOThreshold_Full );
    DMA2_Stream3->PAR  = (uint32_t) &SDIO->FIFO;
    DMA2_Stream3->M0AR = (uint32_t) dma_data_source;
    DMA2_Stream3->NDTR = dma_transfer_size/4;
}



void host_platform_enable_high_speed_sdio( void )
{
    SDIO_InitTypeDef sdio_init_structure;

    sdio_init_structure.SDIO_ClockDiv       = (uint8_t) 0; /* 0 = 24MHz if SDIO clock = 48MHz */
    sdio_init_structure.SDIO_ClockEdge      = SDIO_ClockEdge_Rising;
    sdio_init_structure.SDIO_ClockBypass    = SDIO_ClockBypass_Disable;
    sdio_init_structure.SDIO_ClockPowerSave = SDIO_ClockPowerSave_Disable;
#ifndef SDIO_1_BIT
    sdio_init_structure.SDIO_BusWide = SDIO_BusWide_4b;
#else
    sdio_init_structure.SDIO_BusWide = SDIO_BusWide_1b;
#endif
    sdio_init_structure.SDIO_HardwareFlowControl = SDIO_HardwareFlowControl_Disable;

    SDIO_DeInit( );
    SDIO_Init( &sdio_init_structure );
    SDIO_SetPowerState( SDIO_PowerState_ON );
    SDIO_ClockCmd( ENABLE );
    sdio_enable_bus_irq( );
}

static sdio_block_size_t find_optimal_block_size( uint32_t data_size )
{
    if ( data_size > (uint32_t) 256 )
        return SDIO_512B_BLOCK;
    if ( data_size > (uint32_t) 128 )
        return SDIO_256B_BLOCK;
    if ( data_size > (uint32_t) 64 )
        return SDIO_128B_BLOCK;
    if ( data_size > (uint32_t) 32 )
        return SDIO_64B_BLOCK;
    if ( data_size > (uint32_t) 16 )
        return SDIO_32B_BLOCK;
    if ( data_size > (uint32_t) 8 )
        return SDIO_16B_BLOCK;
    if ( data_size > (uint32_t) 4 )
        return SDIO_8B_BLOCK;
    if ( data_size > (uint32_t) 2 )
        return SDIO_4B_BLOCK;

    return SDIO_4B_BLOCK;
}

static uint32_t sdio_get_blocksize_dctrl(sdio_block_size_t block_size)
{
    switch (block_size)
    {
        case SDIO_1B_BLOCK:    return SDIO_DataBlockSize_1b;
        case SDIO_2B_BLOCK:    return SDIO_DataBlockSize_2b;
        case SDIO_4B_BLOCK:    return SDIO_DataBlockSize_4b;
        case SDIO_8B_BLOCK:    return SDIO_DataBlockSize_8b;
        case SDIO_16B_BLOCK:   return SDIO_DataBlockSize_16b;
        case SDIO_32B_BLOCK:   return SDIO_DataBlockSize_32b;
        case SDIO_64B_BLOCK:   return SDIO_DataBlockSize_64b;
        case SDIO_128B_BLOCK:  return SDIO_DataBlockSize_128b;
        case SDIO_256B_BLOCK:  return SDIO_DataBlockSize_256b;
        case SDIO_512B_BLOCK:  return SDIO_DataBlockSize_512b;
        case SDIO_1024B_BLOCK: return SDIO_DataBlockSize_1024b;
        case SDIO_2048B_BLOCK: return SDIO_DataBlockSize_2048b;
        default: return 0;
    }
}
