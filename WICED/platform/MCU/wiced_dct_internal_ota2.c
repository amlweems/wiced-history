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
 * OTA2 specific DCT functions for internal flash
 */
#include "string.h"
#include "wwd_assert.h"
#include "wiced_result.h"
#include "spi_flash.h"
#include "platform_dct.h"
#include "waf_platform.h"
#include "wiced_framework.h"
#include "wiced_dct_common.h"
#include "wiced_apps_common.h"
#include "platform_mcu_peripheral.h"
#include "elf.h"
#include "../../../libraries/utilities/crc/crc.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#ifndef PLATFORM_DCT_COPY1_SIZE
#define PLATFORM_DCT_COPY1_SIZE (PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS)
#endif

#ifndef SECTOR_SIZE
#define SECTOR_SIZE ((uint32_t)(2048))
#endif

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
static const uint32_t DCT_section_offsets[ ] =
{
    [DCT_APP_SECTION]           = sizeof(platform_dct_data_t),
    [DCT_SECURITY_SECTION]      = OFFSETOF( platform_dct_data_t, security_credentials ),
    [DCT_MFG_INFO_SECTION]      = OFFSETOF( platform_dct_data_t, mfg_info ),
    [DCT_WIFI_CONFIG_SECTION]   = OFFSETOF( platform_dct_data_t, wifi_config ),
    [DCT_ETHERNET_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, ethernet_config ),
    [DCT_NETWORK_CONFIG_SECTION]  = OFFSETOF( platform_dct_data_t, network_config ),
#ifdef WICED_DCT_INCLUDE_BT_CONFIG
    [DCT_BT_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, bt_config ),
#endif
#ifdef WICED_DCT_INCLUDE_P2P_CONFIG
    [DCT_P2P_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, p2p_config ),
#endif
#ifdef OTA2_SUPPORT
    [DCT_OTA2_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, ota2_config ),
#endif
    [DCT_INTERNAL_SECTION]      = 0,
};

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_dct_ota2_save_copy( uint8_t type )
{

    /* copy current DCT to OTA2_IMAGE_APP_DCT_SAVE_AREA_BASE
     * Change the values for factory_reset and update_count
     * to reflect the update type
     */

    platform_dct_ota2_config_t  ota2_dct_header;

    /* App */
    if (wiced_dct_read_lock( (void**)&ota2_dct_header, WICED_TRUE, DCT_OTA2_CONFIG_SECTION, 0, sizeof(platform_dct_ota2_config_t) ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    /* what type of update are we about to do ? */
    ota2_dct_header.boot_type  = type;
    ota2_dct_header.update_count++;

    if (wiced_dct_write( (void**)&ota2_dct_header, DCT_OTA2_CONFIG_SECTION, 0, sizeof(platform_dct_ota2_config_t) ) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    /* erase DCT copy area and copy current DCT */
    if (wiced_dct_ota2_erase_save_area_and_copy_dct( OTA2_IMAGE_APP_DCT_SAVE_AREA_BASE) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

#if 0   // debugging
    wiced_dct_read_lock( (void**)&ota2_dct_header, WICED_TRUE, DCT_OTA2_CONFIG_SECTION, 0, sizeof(platform_dct_ota2_config_t) );
    mini_printf("Saved DCT to backup 0x%x\r\n", OTA2_IMAGE_APP_DCT_SAVE_AREA_BASE);
    mini_printf("    ota2_dct_header.boot_type %d\r\n", ota2_dct_header.boot_type);
#endif

    return WICED_SUCCESS;

}

wiced_result_t wiced_dct_ota2_read_saved_copy( void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size )
{
    platform_dct_header_t   dct_header;
    uint32_t                curr_dct, curr_dct_end;

    /* do standard checks to see if the DCT copy before an update is valid */
    curr_dct = (uint32_t) OTA2_IMAGE_APP_DCT_SAVE_AREA_BASE;
    curr_dct_end = curr_dct + (uint32_t)PLATFORM_DCT_COPY1_SIZE;
    memcpy(&dct_header, (char *)OTA2_IMAGE_APP_DCT_SAVE_AREA_BASE, sizeof(platform_dct_header_t));

    if ( ( dct_header.write_incomplete == 0 ) &&
         ( dct_header.magic_number == BOOTLOADER_MAGIC_NUMBER ) &&
         ( ((dct_header.initial_write == 1) && (dct_header.crc32 == 0x00)) ||
           (wiced_dct_ota2_check_crc_valid(curr_dct, curr_dct_end) == WICED_TRUE)) )
    {
        curr_dct = (uint32_t) OTA2_IMAGE_APP_DCT_SAVE_AREA_BASE + DCT_section_offsets[ section ];
        memcpy(info_ptr, (char *)(curr_dct + offset), size);
        return WICED_SUCCESS;
    }

    return WICED_ERROR;
}
