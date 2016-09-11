/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Provides prototypes for initialization and other management functions for Wiced system
 *
 */

#ifndef INCLUDED_WWD_MANAGEMENT_H
#define INCLUDED_WWD_MANAGEMENT_H

#include "wwd_constants.h"  /* for wwd_result_t and country codes */
#ifdef __cplusplus
extern "C"
{
#endif

/** @addtogroup mgmt WICED Management
 *  User functions for initialization and other management functions for the WICED system
 *  @{
 */

/******************************************************
 *             Function declarations
 ******************************************************/
/*@-exportlocal@*/

/**
 * Turn on the Wi-Fi device
 *
 * - Initialises the required parts of the hardware platform
 *   i.e. pins for SDIO/SPI, interrupt, reset, power etc.
 *
 * - Bring the Wireless interface "Up"
 * - Initialises the Wiced thread which arbitrates access
 *   to the SDIO/SPI bus
 *
 * @return WWD_SUCCESS if initialization is successful, Error code otherwise
 */
wwd_result_t wwd_management_wifi_on( wiced_country_code_t country );

/**
 * Turn off the Wi-Fi device
 *
 * - De-Initialises the required parts of the hardware platform
 *   i.e. pins for SDIO/SPI, interrupt, reset, power etc.
 *
 * - De-Initialises the Wiced thread which arbitrates access
 *   to the SDIO/SPI bus
 *
 * @return WWD_SUCCESS if deinitialization is successful, Error code otherwise
 */
wwd_result_t wwd_management_wifi_off( void );

/*@+exportlocal@*/

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ifndef INCLUDED_WWD_MANAGEMENT_H */
