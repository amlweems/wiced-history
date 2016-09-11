/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file BCM943907WAE_1 Power Management API Header
 *
 *  This API utilizes the Maxim17040 and Maxim8971
 *  chips present on the BCM943907WAE_1 board. These
 *  functions should build on other platforms and
 *  just provide "platform unsupported" messages.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef POWER_MANAGEMENT_H
#include "max8971.h"
#include "max17040.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define POWER_MANAGEMENT_H

/* Management function options flags */
#define CHARGER_OPTIONS_ENABLE_PRINT        0x01
#define CHARGER_OPTIONS_ENABLE_DEBUG        0x02

/* Management function return flags */
#define CHARGER_RETURN_IS_DONE              0x01
#define CHARGER_RETURN_FAST_CHARGING        0x02
#define CHARGER_RETURN_TOPPING_OFF          0x04
#define CHARGER_RETURN_CHARGER_ERROR        0x08
#define CHARGER_RETURN_BATTERY_ERROR        0x10
#define CHARGER_RETURN_THERMISTOR_ERROR     0x20
#define CHARGER_RETURN_INPUT_ERROR          0x40
#define CHARGER_RETURN_PLATFORM_UNSUPPORTED 0x80

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct
{
    float voltage;
    uint8_t state_of_charge;
    float time_remaining;
    uint8_t return_flags;
} power_management_status_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* Initializes and probes maxim8971 and maxim17040 chips.
 *
 * Parameters:
 *     minutes - The number of minutes the specific application takes to
 *               run from a full charge to state of charge = 0%. If runtime
 *               is unknown, this value can be set to 0.
 *
 * Returns:
 *     WICED_SUCCESS     - if successful
 *     WICED_UNSUPPORTED - on any platform except BCM943907WAE_1
 *     WICED_ERROR       - otherwise
 */
wiced_result_t power_management_init(uint32_t minutes);

/* Updates the parameters of the maxim chips and returns battery and charger status.
 *
 * Parameters:
 *     return_status - A pointer to a power management struct. Relevant status and details including
 *                     voltage, SOC, time remaining, and charger status is returned here.
 *     speed_setting - The max possible charging speed (SLOW, MED, FAST). If the input power is
 *                     insufficient for this setting, the battery will charge as fast as the input allows.
 *     options_flag  - Enables printing of status and debug messages.
 *
 * Returns:
 *     WICED_SUCCESS     - if successful
 *     WICED_BADARG      - if return_status parameter is a null pointer
 *     WICED_UNSUPPORTED - on any platform except BCM943907WAE_1
 */
wiced_result_t power_management_update(power_management_status_t* return_status, maxim_charge_speed_t speed_setting, uint8_t options_flag);

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
