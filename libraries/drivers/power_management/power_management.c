/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file BCM943907WAE_1 Power Management API
 *
 */

#include "power_management.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define SAFE_LOWER_LIMIT_VOLTAGE 3.10 /* Discharging the battery below this voltage may damage the battery */
#define STATUS_VOLTAGE_IN_ERROR  0x80
#define STATUS_CURRENT_IN_ERROR  0x40
#define STATUS_THERMISTOR_ERROR  0x02
#define DETAILS2_BATTERY_DETAILS 0x30
#define DETAILS2_CHARGER_DETAILS 0x0F

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
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

#ifdef POWER_MANAGEMENT_ON_BCM943907WAE_1
wiced_result_t power_management_init(uint32_t minutes)
{
    if (max17040_initialize_i2c_device() != WICED_SUCCESS || max8971_initialize() != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    max17040_set_max_runtime(minutes);

    return WICED_SUCCESS;
}

wiced_result_t power_management_update(power_management_status_t* return_status, maxim_charge_speed_t speed_setting, uint8_t options)
{
    uint8_t status = 0x00;

    if (return_status == NULL)
    {
        if (options & CHARGER_OPTIONS_ENABLE_PRINT || options & CHARGER_OPTIONS_ENABLE_DEBUG)
        {
            printf("Error - null argument pointer\n");
        }
        return WICED_BADARG;
    }

    return_status->return_flags = 0x00;
    max8971_set_charge_speed(speed_setting);
    max8971_set_topoff_timer();
    status = max8971_get_charger_status();
    return_status->voltage = max17040_get_vcell_voltage();
    return_status->state_of_charge = max17040_get_soc_percent();
    return_status->time_remaining = max17040_get_time_remaining();

    if (options & CHARGER_OPTIONS_ENABLE_PRINT || options & CHARGER_OPTIONS_ENABLE_DEBUG)
    {
        printf("****************************************\n");
        if (options & CHARGER_OPTIONS_ENABLE_DEBUG)
        {
            max8971_print_all_registers_debug();
        }
        if (return_status->voltage <= SAFE_LOWER_LIMIT_VOLTAGE)
        {
            printf("Warning - Connect charger immediately!\n");
        }
        max8971_print_charger_status(status);
        printf("Battery: %.2f volts - %d%c\n", return_status->voltage, return_status->state_of_charge, '%');
        printf("%.0f minutes remaining\n", return_status->time_remaining);
        printf("****************************************\n");
    }

    if (status & STATUS_THERMISTOR_ERROR)
    {
        return_status->return_flags |= CHARGER_RETURN_THERMISTOR_ERROR;
    }
    if (status & STATUS_VOLTAGE_IN_ERROR || status & STATUS_CURRENT_IN_ERROR)
    {
        return_status->return_flags |= CHARGER_RETURN_INPUT_ERROR;
    }

    status = max8971_get_charger_and_battery_details();

    if ((status & DETAILS2_BATTERY_DETAILS) != 0x20)
    {
        return_status->return_flags |= CHARGER_RETURN_BATTERY_ERROR;
    }
    if ((status & DETAILS2_CHARGER_DETAILS) == 0x05)
    {
        return_status->return_flags |= CHARGER_RETURN_IS_DONE;
    }
    else if ((status & DETAILS2_CHARGER_DETAILS) == 0x01 || (status & DETAILS2_CHARGER_DETAILS) == 0x02 || (status & DETAILS2_CHARGER_DETAILS) == 0x03)
    {
        return_status->return_flags |= CHARGER_RETURN_FAST_CHARGING;
    }
    else if ((status & DETAILS2_CHARGER_DETAILS) == 0x04)
    {
        return_status->return_flags |= CHARGER_RETURN_TOPPING_OFF;
    }
    else
    {
        return_status->return_flags |= CHARGER_RETURN_CHARGER_ERROR;
    }

    return WICED_SUCCESS;
}
#else
wiced_result_t power_management_init(uint32_t minutes)
{
    return WICED_UNSUPPORTED;
}

wiced_result_t power_management_update(power_management_status_t* return_status, maxim_charge_speed_t speed_setting, uint8_t options)
{
    if (return_status != NULL)
    {
        return_status->return_flags = CHARGER_RETURN_PLATFORM_UNSUPPORTED;
    }

    if (options & CHARGER_OPTIONS_ENABLE_PRINT || options & CHARGER_OPTIONS_ENABLE_DEBUG)
    {
        printf("Error - Power Management is only supported on BCM943907WAE_1\n");
    }

    return WICED_UNSUPPORTED;
}
#endif