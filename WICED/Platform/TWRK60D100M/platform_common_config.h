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

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define WL_GPIO0_BANK           K60_GPIO_D
#define WL_GPIO0_PIN            10
#define WL_GPIO1_BANK           K60_GPIO_B
#define WL_GPIO1_PIN            8

#define WL_SPI                  (SPI2_BASE_PTR)
#define SPI_BUS_CLOCK_BANK      K60_GPIO_D
#define SPI_BUS_MISO_BANK       K60_GPIO_D
#define SPI_BUS_MOSI_BANK       K60_GPIO_D
#define SPI_BUS_CS_BANK         K60_GPIO_D
#define SPI_IRQ_BANK            K60_GPIO_E
#define SPI_BUS_CLOCK_PIN       12
#define SPI_BUS_MISO_PIN        14
#define SPI_BUS_MOSI_PIN        13
#define SPI_BUS_CS_PIN          15
#define SPI_IRQ_PIN             26

#define WL_REG_ON_BANK          K60_GPIO_B
#define WL_REG_ON_PIN           9
#define WL_RESET_BANK           K60_GPIO_A
#define WL_RESET_PIN            19

#define STDIO_UART_PERIPHERAL   UART5_BASE_PTR
#define STDIO_TX_PIN            8
#define STDIO_RX_PIN            9
#define STDIO_TX_BANK           K60_GPIO_E
#define STDIO_RX_BANK           K60_GPIO_E
#define STDIO_IRQ_CHANNEL       55

#define STDIO_UART              WICED_UART_1

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
