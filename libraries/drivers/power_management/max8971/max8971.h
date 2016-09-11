/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file Maxim8971 Library Header
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX8971_H
#include "wiced.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX8971_H

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/* Input current must be greater than
 * or equal to charge speed setting */
typedef enum
{
    MAXIM_CHARGE_SPEED_SLOW,   /* 0.5 Amp */
    MAXIM_CHARGE_SPEED_MEDIUM, /* 1.0 Amp */
    MAXIM_CHARGE_SPEED_FAST    /* 1.5 Amp */
} maxim_charge_speed_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* Initializes and probes maxim8971 chip.
 *
 * Returns:
 *     WICED_SUCCESS - if successful.
 *     WICED_ERROR   - otherwise.
 */
wiced_result_t max8971_initialize();

/* Individual functions for setting specific charger parameters. */
void max8971_set_charge_speed(maxim_charge_speed_t speed);
void max8971_set_topoff_timer();

/* Individual functions for retrieving specific charger information.
 * Pass return byte to print functions for visible output.
 *
 * Returns:
 *     Single byte from relevant register containing settings flags.
 *
 * Note: Interrupt requests are cleared upon reading.
 */
uint8_t max8971_get_charger_status();
uint8_t max8971_get_thermistor_details();
uint8_t max8971_get_charger_and_battery_details();
uint8_t max8971_get_interrupt_requests();

/* Individual functions for printing specific charger information.
 *
 * Parameters:
 *     Single byte from related maxim register (use maxim8971_get_*() functions).
 */
void max8971_print_charger_status(uint8_t status);
void max8971_print_thermistor_details(uint8_t details);
void max8971_print_charger_and_battery_details(uint8_t details);

/* Prints all maxim8971 registers.
 *
 * Values in register 0x0F (Interrupt requests register) are cleared on read.
 */
void max8971_print_all_registers_debug();

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
