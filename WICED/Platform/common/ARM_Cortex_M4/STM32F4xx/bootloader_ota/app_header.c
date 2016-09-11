/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include "bootloader.h"
#include "internal/wifi_image/wwd_wifi_image_interface.h"

#ifndef APP_VERSION
#define APP_VERSION ""
#endif /*ifndef APP_VERSION */


extern const void* vectors_offset;
extern const void* total_app_image_size;
extern const void* wifi_firmware_image_size_from_link;

const bootloader_app_header_t bootloader_app_header =
{
    (uint32_t)&vectors_offset,
    (uint32_t)&total_app_image_size,
    (uint32_t)wifi_firmware_image,
    (uint32_t)&wifi_firmware_image_size_from_link,
    APP_NAME,
    APP_VERSION,
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};
