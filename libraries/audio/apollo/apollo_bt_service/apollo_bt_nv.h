/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef BT_NV_H
#define BT_NV_H

#include "wiced_bt_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
 ******************************************************/
#define APOLLO_BT_NV_MAX_LAST_PAIRED_DEVICES ( 8 )


/******************************************************
*                     Typedefs
******************************************************/
typedef    uint8_t  apollo_bt_nv_hash_table_t[APOLLO_BT_NV_MAX_LAST_PAIRED_DEVICES];

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *            Structures
 ******************************************************/
typedef struct
{
    uint32_t                       services_mask;      /**< Supported services*/
    wiced_bt_device_link_keys_t    device_link;            /**< BD address of the peer device. */
} apollo_bt_paired_device_info_t;


typedef struct
{
    apollo_bt_nv_hash_table_t          bt_hash_table;
    apollo_bt_paired_device_info_t     bt_paired_device_info[APOLLO_BT_NV_MAX_LAST_PAIRED_DEVICES];
    wiced_bt_local_identity_keys_t     bt_local_id_keys;
} apollo_bt_dct_t;

/******************************************************
 *            Function Declarations
 ******************************************************/

wiced_result_t apollo_bt_nv_init( uint32_t dct_offset_for_bt );
void apollo_bt_nv_deinit( void );
wiced_result_t apollo_bt_nv_update_device_link_key( wiced_bt_device_link_keys_t *in_device );
wiced_result_t apollo_bt_nv_update_last_connected_device( wiced_bt_device_address_t address );
wiced_result_t apollo_bt_nv_get_device_info_by_addr( wiced_bt_device_address_t *address, apollo_bt_paired_device_info_t *out_device );
wiced_result_t apollo_bt_nv_get_device_info_by_index( uint8_t index, apollo_bt_paired_device_info_t *out_device );
wiced_result_t apollo_bt_nv_delete_device_info( wiced_bt_device_address_t address );
wiced_result_t apollo_bt_nv_delete_device_info_list( void );
wiced_result_t apollo_bt_nv_update_local_id_keys( wiced_bt_local_identity_keys_t *keys);
wiced_result_t apollo_bt_nv_get_local_id_keys( wiced_bt_local_identity_keys_t *keys);

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif //BT_NV_H
