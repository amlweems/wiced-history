/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "platform.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define WLAN_POWERSAVE_CLOCK_FREQUENCY 32768 /* 32768Hz        */
#define WLAN_POWERSAVE_CLOCK_DUTY_CYCLE   50 /* 50% duty-cycle */

/******************************************************
 *                   Enumerations
 ******************************************************/

/* These are internal module connections only */
typedef enum
{
    WICED_GPIO_SFLASH_CS = WICED_GPIO_MAX,
    WICED_GPIO_SFLASH_CLK,
    WICED_GPIO_SFLASH_MISO,
    WICED_GPIO_SFLASH_MOSI,
    WICED_GPIO_WLAN_POWERSAVE_CLOCK,
} wiced_extended_gpio_t;

typedef enum
{
    WICED_PWM_WLAN_POWERSAVE_CLOCK = WICED_PWM_MAX
} wiced_extended_pwm_t;

typedef enum
{
	WICED_SPI_SFLASH = WICED_SPI_MAX,
} wiced_extended_spi_t;

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

