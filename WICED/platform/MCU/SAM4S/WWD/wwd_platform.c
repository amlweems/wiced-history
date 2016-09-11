/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * Defines STM32F2xx-specific WWD platform functions
 */
#include <stdint.h>
#include <string.h>
#include "platform_peripheral.h"
#include "platform_config.h"
#include "wwd_constants.h"
#include "wwd_platform_common.h"
#include "platform/wwd_platform_interface.h"
#include "genclk.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

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
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wwd_result_t host_platform_init_wlan_powersave_clock( void )
{
    /* Configure IO port CLKOUT if enabled */
#if defined ( WICED_USE_WIFI_32K_CLOCK_MCO )
    struct genclk_config gcfg;

    /* Since all SCK are currently on Peripheral mode B - hardwire it */
    ioport_set_pin_mode(wifi_control_pins[WWD_PIN_32K_CLK].pin, IOPORT_MODE_MUX_B);
    ioport_disable_pin(wifi_control_pins[WWD_PIN_32K_CLK].pin);
    /* Configure the output clock */
    genclk_config_defaults(&gcfg, MCO_CLK_ID);
    genclk_config_set_source(&gcfg, GENCLK_PCK_SRC_SLCK_XTAL);
    genclk_config_set_divider(&gcfg, GENCLK_PCK_PRES_1);
    genclk_enable(&gcfg, MCO_CLK_ID);
    return WICED_SUCCESS;
#else
    return WICED_UNSUPPORTED;
#endif
}
