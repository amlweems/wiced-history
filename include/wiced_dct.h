/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Defines functions that allow access to the Device Configuration Table (DCT)
 *
 */

#pragma once

#include "wwd_constants.h"
#include "bootloader_app.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 * @cond                Macros
 ******************************************************/

#define DCT_OFFSET(type, member)             ((uint32_t)&((type *)0)->member)

#if defined(__IAR_SYSTEMS_ICC__)
#pragma section="initial_dct_section"
#define DEFINE_APP_DCT(type) \
    const type _app_dct @ "initial_dct_section"; \
    const type _app_dct =
#else /* #if defined(__IAR_SYSTEMS_ICC__) */
#define DEFINE_APP_DCT(type) const type _app_dct =
#endif /* #if defined(__IAR_SYSTEMS_ICC__) */

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
 *                 Global Variablesmkdir -p $JENKINSDIR/repo_2.4.x
 ******************************************************/

/******************************************************
 *               Function Declarations
 * @endcond
 ******************************************************/

/*****************************************************************************/
/** @addtogroup dct       DCT
 *
 * Device Configuration Table (Non-volatile flash storage space)
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Retrieves a pointer to the application section of the current DCT
 *
 * @return    The app section pointer
 */
extern void const* wiced_dct_get_app_section( void );


/** Retrieves a pointer to the manufacturing info section of the current DCT
 *
 * @return    The manufacturing section pointer
 */
extern platform_dct_mfg_info_t const* wiced_dct_get_mfg_info_section( void );


/** Retrieves a pointer to the security section of the current DCT
 *
 * @return    The security section pointer
 */
extern platform_dct_security_t const* wiced_dct_get_security_section( void );


/** Retrieves a pointer to the Wi-Fi config info section of the current DCT
 *
 * @return    The Wi-Fi section pointer
 */
extern platform_dct_wifi_config_t const* wiced_dct_get_wifi_config_section( void );

//----------------------------------
/** Reads a volatile copy of the DCT security section from flash into a block of RAM
 *
 * @param[out] security_dct : A pointer to the RAM that will receive a copy of the DCT security section
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_dct_read_security_section( platform_dct_security_t* security_dct );


/** Writes a volatile copy of the DCT security section in RAM to the flash
 *
 * @warning: To avoid flash wear, this function should only be used for settings which are changed rarely
 *
 * @param[in] security_dct : A pointer to the volatile copy of the DCT security section in RAM
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_dct_write_security_section( const platform_dct_security_t* security_dct );
//-------------------------------------

/** Reads a volatile copy of the DCT wifi config section from flash into a block of RAM
 *
 * @param[out] wifi_config_dct : A pointer to the RAM that will receive a copy of the DCT wifi config section
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_dct_read_wifi_config_section( platform_dct_wifi_config_t* wifi_config_dct );


/** Writes a volatile copy of the DCT Wi-Fi config section in RAM to the flash
 *
 * @warning: To avoid flash wear, this function should only be used for settings which are changed rarely
 *
 * @param[in] wifi_config_dct : A pointer to the volatile copy of DCT Wi-Fi config section in RAM
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_dct_write_wifi_config_section( const platform_dct_wifi_config_t* wifi_config_dct );
//-------------------------------------

/** Reads a volatile copy of the DCT app section from flash into a block of RAM
 *
 * @param[out] app_dct : A pointer to the RAM that will receive a copy of the DCT app section
 * @param[in]  size    : The size of the DCT app section
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_dct_read_app_section( void* app_dct, uint32_t size );


/** Writes a volatile copy of the DCT app section in RAM to the flash
 *
 * @note: To avoid wearing out the flash, this function should only be
 *        used for settings which are changed rarely.
 *
 * @param[in] size    : The size of the DCT app section
 * @param[in] app_dct : A pointer to the volatile copy of DCT app section in RAM
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_dct_write_app_section( const void* app_dct, uint32_t size );
//-------------------------------------

/** @} */

#ifdef __cplusplus
} /*extern "C" */
#endif
