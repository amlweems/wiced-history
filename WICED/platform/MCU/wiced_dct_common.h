/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#ifndef OFFSETOF
#define OFFSETOF( type, member )  ( (uintptr_t)&((type *)0)->member )
#endif /* OFFSETOF */

#define GET_CURRENT_ADDRESS_FAILED  ((void*)0xffffffff)

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/* DCT section */
typedef enum
{
    DCT_APP_SECTION,
    DCT_SECURITY_SECTION,
    DCT_MFG_INFO_SECTION,
    DCT_WIFI_CONFIG_SECTION,
    DCT_INTERNAL_SECTION, /* Do not use in apps */
    DCT_ETHERNET_CONFIG_SECTION,
    DCT_NETWORK_CONFIG_SECTION,
#ifdef WICED_DCT_INCLUDE_BT_CONFIG
    DCT_BT_CONFIG_SECTION,
#endif
#ifdef WICED_DCT_INCLUDE_P2P_CONFIG
    DCT_P2P_CONFIG_SECTION,
#endif
#ifdef OTA2_SUPPORT
    DCT_OTA2_CONFIG_SECTION,
#endif
} dct_section_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t wiced_dct_read_with_copy         ( void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size );
wiced_result_t wiced_dct_write                  ( const void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size );
wiced_result_t wiced_dct_get_app_header_location( uint8_t app_id, image_location_t* app_header_location );
wiced_result_t wiced_dct_set_app_header_location( uint8_t app_id, image_location_t* app_header_location );
wiced_result_t wiced_dct_restore_factory_reset  ( void );
void*          wiced_dct_get_current_address    ( dct_section_t section );

#if defined(OTA2_SUPPORT)
/* used by OTA2, lives in wiced_dct_external_ota2.c  and  wiced_dct_internal_ota2.c */
wiced_result_t wiced_dct_ota2_save_copy         ( uint8_t boot_type );
wiced_result_t wiced_dct_ota2_read_saved_copy   ( void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size );

/* used by OTA2, lives in wiced_dct_external_common.c  and  wiced_dct_internal_common.c */
wiced_result_t wiced_dct_ota2_erase_save_area_and_copy_dct( uint32_t det_loc );
wiced_bool_t wiced_dct_ota2_check_crc_valid(uint32_t dct_start_addr, uint32_t dct_end_addr);
#endif

#ifdef __cplusplus
} /*extern "C" */
#endif
