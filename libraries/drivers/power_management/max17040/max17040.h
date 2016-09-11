/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file Maxim17040 Library Header
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX17040_H
#include "wiced.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX17040_H

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

/* Initializes and probes the maxim17040 chip.
 *
 * Returns:
 *     WICED_SUCCESS - if successful.
 *     WICED_ERROR   - otherwise.
 */
wiced_result_t max17040_initialize_i2c_device();

/* Returns the current battery voltage in volts. */
float max17040_get_vcell_voltage();

/* Returns the calculated State Of Charge in percent.
 *
 * Note: The SOC calculation becomes more accurate
 *       with prolonged charging and discharging.
 */
uint8_t max17040_get_soc_percent();

/* Sets the runtime parameters for the time remaning calculation.
 *
 * Parameters:
 *     minutes - The number of minutes the specific application
 *               runs from a full charge to SOC = 0%.
 */
void max17040_set_max_runtime(uint32_t minutes);

/* Returns an estimate of remaining runtime duration in minutes.
 *
 * Note: The max runtime for the specific application must be
 *       set for this function to work. This estimate is less
 *       accurate as SOC approaches 0%.
 */
float max17040_get_time_remaining();

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
