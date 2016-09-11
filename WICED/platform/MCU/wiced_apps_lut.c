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
 *
 */
#include "platform_dct.h"
#include "wiced_result.h"
#include "wiced_apps_common.h"

/******************************************************
 *                      Macros
 ******************************************************/

#if defined ( __IAR_SYSTEMS_ICC__ )
/* Define the application look-up table as the
 * entry point variable to force the symbol into
 * the resulting image.  Simply forcing the symbol
 * into the image won't work because this file is
 * compiled and linked alone to produce a binary.
 * IAR requires an entry point to link.  The
 * look-up table is never referenced outside of this
 * file.
 */
#define WICED_APPS_LUT              _start
#define PAD_INITIALIZER             { 0, }
#else
#define WICED_APPS_LUT              wiced_apps_lut
#define PAD_INITIALIZER             { [0 ... WICED_APPS_LUT_PAD_SIZE - 1] = 0xFF }
#endif /* if defined ( __IAR_SYSTEMS_ICC__ ) */

/******************************************************
 *                    Constants
 ******************************************************/

#define SFLASH_SECTOR_SIZE          (4096)

#define WICED_APPS_LUT_PAD_SIZE \
    ( SFLASH_SECTOR_SIZE - sizeof( app_headers_array_t ) )

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct wiced_apps_lut wiced_apps_lut_t;
typedef app_header_t app_headers_array_t[8];

/******************************************************
 *                    Structures
 ******************************************************/

struct wiced_apps_lut
{
    app_headers_array_t data;
    /* Pad up to sector size. */
    uint8_t _pad[WICED_APPS_LUT_PAD_SIZE];
};

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Declarations
 ******************************************************/

#if defined ( __IAR_SYSTEMS_ICC__ )
#pragma section="wiced_apps_lut_section"
const wiced_apps_lut_t WICED_APPS_LUT @ "wiced_apps_lut_section";
#endif /* if defined ( __IAR_SYSTEMS_ICC__ ) */

/******************************************************
 *               Variables Definitions
 ******************************************************/

const wiced_apps_lut_t WICED_APPS_LUT =
{
    .data =
    {
        [DCT_FR_APP_INDEX] =
            {
                .count   = FR_APP_ENTRY_COUNT,
                .secure  = FR_APP_IS_SECURE,
                .sectors = { {FR_APP_SECTOR_START,FR_APP_SECTOR_COUNT} }
            },
        [DCT_DCT_IMAGE_INDEX] =
            {
                .count   = DCT_IMAGE_ENTRY_COUNT,
                .secure  = DCT_IMAGE_IS_SECURE,
                .sectors = { {DCT_IMAGE_SECTOR_START,DCT_IMAGE_SECTOR_COUNT} }
            },
        [DCT_OTA_APP_INDEX] =
            {
                .count   = OTA_APP_ENTRY_COUNT,
                .secure  = OTA_APP_IS_SECURE,
                .sectors = { {OTA_APP_SECTOR_START,OTA_APP_SECTOR_COUNT} }
            },
        [DCT_FILESYSTEM_IMAGE_INDEX] =
            {
                .count   = FILESYSTEM_IMAGE_ENTRY_COUNT,
                .secure  = FILESYSTEM_IMAGE_IS_SECURE,
                .sectors = { {FILESYSTEM_IMAGE_SECTOR_START,FILESYSTEM_IMAGE_SECTOR_COUNT} }
            },
        [DCT_WIFI_FIRMWARE_INDEX] =
            {
                .count   = WIFI_FIRMWARE_ENTRY_COUNT,
                .secure  = WIFI_FIRMWARE_IS_SECURE,
                .sectors = { {WIFI_FIRMWARE_SECTOR_START,WIFI_FIRMWARE_SECTOR_COUNT} }
            },
        [DCT_APP0_INDEX] =
            {
                .count   = APP0_ENTRY_COUNT,
                .secure  = APP0_IS_SECURE,
                .sectors = { {APP0_SECTOR_START,APP0_SECTOR_COUNT} }
            },
        [DCT_APP1_INDEX] =
            {
                .count   = APP1_ENTRY_COUNT,
                .secure  = APP1_IS_SECURE,
                .sectors = { {APP1_SECTOR_START,APP1_SECTOR_COUNT} }
            },
        [DCT_APP2_INDEX] =
            {
                .count   = APP2_ENTRY_COUNT,
                .secure  = APP2_IS_SECURE,
                .sectors = { {APP2_SECTOR_START,APP2_SECTOR_COUNT} }
            },
    },

    ._pad = PAD_INITIALIZER,
};


/******************************************************
 *               Function Definitions
 ******************************************************/
