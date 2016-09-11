/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef WICED_BT_COMMON_H
#define WICED_BT_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/**  \file    wiced_bt_common.h
        \brief  Header file for common definitions used in WICED BT library.
*/

/** \addtogroup btdm
*
* @{
*/

#include "wiced_result.h"

/* This represents: sizeof(bt_paired_device_info_t) * maximum_number_of_bluetooth connections */
#define WICED_BT_AUDIO_DCT_PAIRED_DEVICES_TABLE_SIZE       480 //( 60 * 8 )
/* Used for indexing in device table */
#define WICED_BT_AUDIO_DCT_PAIRED_DEVICES_INDEX_SIZE       8   //( 1 * 8 )
#define WICED_BT_AUDIO_DCT_MAX_SIZE                ( WICED_BT_AUDIO_DCT_PAIRED_DEVICES_TABLE_SIZE + \
                                                     WICED_BT_AUDIO_DCT_PAIRED_DEVICES_INDEX_SIZE )
/**
*       \brief Bluetooth Device Address
**/
typedef struct
{
   unsigned char address[6];  //!<bt address of the device
} __attribute__((packed)) wiced_bt_bdaddr_t;

/**
*       \brief Bluetooth Device Name
**/
typedef struct
{
    unsigned char name[249];//!<name of the bt device
} __attribute__((packed)) wiced_bt_bdname_t;

/** @} */ // end of btcommon

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif //#ifndef WICED_BT_COMMON_H
