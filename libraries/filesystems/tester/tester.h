/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#pragma once

#include <stdio.h>

#include "wiced_filesystem.h"
#include "wiced_block_device.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    char* filename;
    FILE* image_file_handle;
    wiced_block_device_write_mode_t write_mode;
} tester_block_device_specific_data_t;

extern const wiced_block_device_driver_t tester_block_device_driver;

wiced_result_t create_wiced_filesystem( wiced_block_device_t* device, wiced_filesystem_handle_type_t fs_type, const char* dir_name );

#ifdef __cplusplus
} /* extern "C" */
#endif
