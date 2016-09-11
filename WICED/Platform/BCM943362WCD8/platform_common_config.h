/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#pragma once
#include "ioport.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* WLAN Regulator On pin */
#define WL_REG_ON_PIN           IOPORT_CREATE_PIN( PIOB, 13 )

/* WLAN reset pin */
#define WL_RESET_PIN            IOPORT_CREATE_PIN( PIOB,  3 )

/* WLAN 32K sleep clock pin */
#define WL_32K_CLK_PIN          IOPORT_CREATE_PIN( PIOB,  0 )

/* WLAN GPIO0 pin */
#define WL_GPIO0_PIN            IOPORT_CREATE_PIN( PIOB,  1 )

/* WLAN GPIO1 pin */
#define WL_GPIO1_PIN            IOPORT_CREATE_PIN( PIOB,  2 )

/* SDIO clock pin */
#define SDIO_CLK_PIN            IOPORT_CREATE_PIN( PIOA, 29 )

/* SDIO clock pin configuration */
#define SDIO_CLK_CONFIG         ( IOPORT_MODE_MUX_C | IOPORT_MODE_PULLUP )

/* SDIO command pin */
#define SDIO_CMD_PIN            IOPORT_CREATE_PIN( PIOA, 28 )

/* SDIO command pin configuration */
#define SDIO_CMD_CONFIG         ( IOPORT_MODE_MUX_C | IOPORT_MODE_PULLUP )

/* SDIO DAT0 pin */
#define SDIO_D0_PIN             IOPORT_CREATE_PIN( PIOA, 30 )

/* SDIO DAT0 pin configuration */
#define SDIO_D0_CONFIG          ( IOPORT_MODE_MUX_C | IOPORT_MODE_PULLUP )

/* SDIO DAT1 pin */
#define SDIO_D1_PIN             IOPORT_CREATE_PIN( PIOA, 31 )

/* SDIO DAT1 pin configuration */
#define SDIO_D1_CONFIG          ( IOPORT_MODE_MUX_C | IOPORT_MODE_PULLUP )

/* SDIO DAT2 pin */
#define SDIO_D2_PIN             IOPORT_CREATE_PIN( PIOA, 26 )

/* SDIO DAT2 pin configuration */
#define SDIO_D2_CONFIG          ( IOPORT_MODE_MUX_C | IOPORT_MODE_PULLUP )

/* SDIO DAT3 pin */
#define SDIO_D3_PIN             IOPORT_CREATE_PIN( PIOA, 27 )

/* SDIO DAT0 pin configuration */
#define SDIO_D3_CONFIG          ( IOPORT_MODE_MUX_C | IOPORT_MODE_PULLUP )

/* SDIO out-of-band (OOB) interrupt pin */
#define SDIO_OOB_IRQ_PIN        IOPORT_CREATE_PIN( PIOB,  2 )

/* SDIO out-of-band (OOB) interrupt pin configuration */
#define SDIO_OOB_IRQ_CONFIG     ( 0 )

/* SDIO OOB interrupt pin WAKEUP ID: WKUP12 */
#define SDIO_OOB_IRQ_WAKEUP_ID  ( 12 )

/* MCU Wakeup pin */
#define MCU_WAKEUP_PIN          IOPORT_CREATE_PIN( PIOA, 20 )

/* MCU Wakeup pin WAKEUP ID: WKUP12 */
#define MCU_WAKEUP_ID           ( 10 )

/* Standard I/O UART */
#define STDIO_UART              ( WICED_UART_1 )

/* SPI flash */
#define SPI_FLASH               ( WICED_SPI_1 )

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
