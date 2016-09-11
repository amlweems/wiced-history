/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include "spi_flash_platform_interface.h"
#include "stm32f2xx.h"

#define SFLASH_SPI                           SPI1
#define SFLASH_SPI_CLK                       RCC_APB2Periph_SPI1
#define SFLASH_SPI_CLK_INIT                  RCC_APB2PeriphClockCmd

#define SFLASH_SPI_SCK_PIN                   GPIO_Pin_5
#define SFLASH_SPI_SCK_GPIO_PORT             GPIOA
#define SFLASH_SPI_SCK_GPIO_CLK              RCC_AHB1Periph_GPIOA
#define SFLASH_SPI_SCK_SOURCE                GPIO_PinSource5
#define SFLASH_SPI_SCK_AF                    GPIO_AF_SPI1

#define SFLASH_SPI_MISO_PIN                  GPIO_Pin_6
#define SFLASH_SPI_MISO_GPIO_PORT            GPIOA
#define SFLASH_SPI_MISO_GPIO_CLK             RCC_AHB1Periph_GPIOA
#define SFLASH_SPI_MISO_SOURCE               GPIO_PinSource6
#define SFLASH_SPI_MISO_AF                   GPIO_AF_SPI1

#define SFLASH_SPI_MOSI_PIN                  GPIO_Pin_7
#define SFLASH_SPI_MOSI_GPIO_PORT            GPIOA
#define SFLASH_SPI_MOSI_GPIO_CLK             RCC_AHB1Periph_GPIOA
#define SFLASH_SPI_MOSI_SOURCE               GPIO_PinSource7
#define SFLASH_SPI_MOSI_AF                   GPIO_AF_SPI1

#define SFLASH_CS_PIN                        GPIO_Pin_4
#define SFLASH_CS_PORT                       GPIOA
#define SFLASH_CS_CLK                        RCC_AHB1Periph_GPIOA


int sflash_platform_init( int peripheral_id, void** platform_peripheral_out )
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    (void) peripheral_id; /* Unused due to single SPI Flash */

    /* Enable clocks */
    SFLASH_SPI_CLK_INIT( SFLASH_SPI_CLK, ENABLE );

    RCC_AHB1PeriphClockCmd( SFLASH_SPI_SCK_GPIO_CLK  | SFLASH_SPI_MISO_GPIO_CLK |
                            SFLASH_SPI_MOSI_GPIO_CLK | SFLASH_CS_CLK, ENABLE      );


    /* Use Alternate Functions for SPI pins */
    GPIO_PinAFConfig( SFLASH_SPI_SCK_GPIO_PORT,  SFLASH_SPI_SCK_SOURCE,  SFLASH_SPI_SCK_AF  );
    GPIO_PinAFConfig( SFLASH_SPI_MISO_GPIO_PORT, SFLASH_SPI_MISO_SOURCE, SFLASH_SPI_MISO_AF );
    GPIO_PinAFConfig( SFLASH_SPI_MOSI_GPIO_PORT, SFLASH_SPI_MOSI_SOURCE, SFLASH_SPI_MOSI_AF );

    /* Setup pin types */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;

    GPIO_InitStructure.GPIO_Pin   = SFLASH_SPI_SCK_PIN;
    GPIO_Init( SFLASH_SPI_SCK_GPIO_PORT, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin   =  SFLASH_SPI_MOSI_PIN;
    GPIO_Init( SFLASH_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure );

    GPIO_InitStructure.GPIO_Pin   =  SFLASH_SPI_MISO_PIN;
    GPIO_Init( SFLASH_SPI_MISO_GPIO_PORT, &GPIO_InitStructure );

    /* Chip select is used as a GPIO */
    GPIO_InitStructure.GPIO_Pin   = SFLASH_CS_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init( SFLASH_CS_PORT, &GPIO_InitStructure );

    /* Deselect flash initially */
    GPIO_SetBits( SFLASH_CS_PORT, SFLASH_CS_PIN );

    /*!< SPI configuration */
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;

    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SFLASH_SPI, &SPI_InitStructure);

    /* Enable the SPI peripheral */
    SPI_Cmd(SFLASH_SPI, ENABLE);

    *platform_peripheral_out = (void*)SFLASH_SPI;

    return 0;
}

int sflash_platform_send_recv_byte( void* platform_peripheral, unsigned char MOSI_val, void* MISO_addr )
{
    /* Wait until the SPI Data Register is empty */
    while ( SPI_I2S_GetFlagStatus( SFLASH_SPI, SPI_I2S_FLAG_TXE ) == RESET )
    {
        /* wait */
    }


    SPI_I2S_SendData( SFLASH_SPI, MOSI_val );

    /* Wait until the SPI peripheral indicates the received data is ready */
    while ( SPI_I2S_GetFlagStatus( SFLASH_SPI, SPI_I2S_FLAG_RXNE ) == RESET )
    {
        /* wait */
    }

    /* read the received data */
    char x  = SPI_I2S_ReceiveData( SFLASH_SPI );

    *( (char*) MISO_addr ) = x;
    return 0;
}

int sflash_platform_chip_select( void* platform_peripheral )
{
    GPIO_ResetBits( SFLASH_CS_PORT, SFLASH_CS_PIN );
    return 0;
}

int sflash_platform_chip_deselect( void* platform_peripheral )
{
    GPIO_SetBits( SFLASH_CS_PORT, SFLASH_CS_PIN );
    return 0;
}

