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

/* Pin Assignments
 */

#include "wiced_platform.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define BLUETOOTH_UART                  WICED_UART_2
#define BLUETOOTH_GPIO_RESET_PIN        WICED_GPIO_3
#define BLUETOOTH_GPIO_REG_EN_PIN      WICED_GPIO_12
#define BLUETOOTH_GPIO_RTS_PIN         WICED_GPIO_28
#define BLUETOOTH_GPIO_CTS_PIN         WICED_GPIO_29

#define BLUETOOTH_READ_TIMEOUT                    50
#define BLUETOOTH_POLL_INTERVAL                 5000
#define BLUETOOTH_BAUD_RATE                   115200
#define BLUETOOTH_DATA_WIDTH         DATA_WIDTH_8BIT
#define BLUETOOTH_PARITY_BIT               NO_PARITY
#define BLUETOOTH_FLOW_CONTROL FLOW_CONTROL_DISABLED
#define BLUETOOTH_STOP_BITS              STOP_BITS_1

#define PC_UART                         WICED_UART_1

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
