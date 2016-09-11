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

#include "wiced_utilities.h"
#include "wiced_bt_smartbridge_constants.h"
#include "wiced_bt_smart_attribute.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/* GATT Feature Sub-Procedures (SPEC v4.0 Part G Section 4) */
typedef enum
{
    GATT_SUBPROCEDURE_NONE,                         /* Default Value                                       */
    GATT_EXCHANGE_MTU,                              /* GATT Feature: Server Configuration                  */
    GATT_DISCOVER_ALL_PRIMARY_SERVICES,             /* GATT Feature: Primary Service Discovery             */
    GATT_DISCOVER_PRIMARY_SERVICE_BY_SERVICE_UUID,  /* GATT Feature: Primary Service Discovery             */
    GATT_FIND_INCLUDED_SERVICES,                    /* GATT Feature: Relationship Discovery                */
    GATT_DISCOVER_ALL_CHARACTERISTICS_OF_A_SERVICE, /* GATT Feature: Characteristic Discovery              */
    GATT_DISCOVER_CHARACTERISTIC_BY_UUID,           /* GATT Feature: Characteristic Discovery              */
    GATT_DISCOVER_ALL_CHARACTERISTICS_DESCRIPTORS,  /* GATT Feature: Characteristic Descriptor Discovery   */
    GATT_READ_CHARACTERISTIC_VALUE,                 /* GATT Feature: Characteristic Value Read             */
    GATT_READ_USING_CHARACTERISTIC_UUID,            /* GATT Feature: Characteristic Value Read             */
    GATT_READ_LONG_CHARACTERISTIC_VALUES,           /* GATT Feature: Characteristic Value Read             */
    GATT_READ_MULTIPLE_CHARACTERISTIC_VALUES,       /* GATT Feature: Characteristic Value Read             */
    GATT_WRITE_WITHOUT_RESPONSE,                    /* GATT Feature: Characteristic Value Write            */
    GATT_SIGNED_WRITE_WITHOUT_RESPONSE,             /* GATT Feature: Characteristic Value Write            */
    GATT_WRITE_CHARACTERISTIC_VALUE,                /* GATT Feature: Characteristic Value Write            */
    GATT_WRITE_LONG_CHARACTERISTIC_VALUE,           /* GATT Feature: Characteristic Value Write            */
    GATT_CHARACTERISTIC_VALUE_RELIABLE_WRITES,      /* GATT Feature: Characteristic Value Write            */
    GATT_NOTIFICATIONS,                             /* GATT Feature: Characteristic Value Notification     */
    GATT_INDICATIONS,                               /* GATT Feature: Characteristic Value Indication       */
    GATT_READ_CHARACTERISTIC_DESCRIPTORS,           /* GATT Feature: Characteristic Descriptor Value Read  */
    GATT_READ_LONG_CHARACTERISTIC_DESCRIPTORS,      /* GATT Feature: Characteristic Descriptor Value Read  */
    GATT_WRITE_CHARACTERISTIC_DESCRIPTORS,          /* GATT Feature: Characteristic Descriptor Value Write */
    GATT_WRITE_LONG_CHARACTERISTIC_DESCRIPTORS,     /* GATT Feature: Characteristic Descriptor Value Write */
} bt_smart_gatt_subprocedure_t;

typedef struct
{
    bt_smart_gatt_subprocedure_t subprocedure;
    wiced_mutex_t                mutex;
    wiced_semaphore_t            done_semaphore;
    wiced_result_t               result;
    wiced_bt_uuid_t              uuid;
    wiced_bt_smart_attribute_t*  attr_head;
    wiced_bt_smart_attribute_t*  attr_tail;
    uint32_t                     attr_count;
    uint16_t                     server_mtu;
    uint16_t                     start_handle;
    uint16_t                     end_handle;
    uint16_t                     length;
    uint16_t                     offset;
    //bt_smart_att_pdu_t*          pdu;
    uint16_t                     connection_handle;
} gatt_subprocedure_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef wiced_result_t (*bt_smart_gatt_notification_indication_handler_t )( uint16_t connection_handle, uint16_t attribute_handle, uint8_t* data, uint16_t length );

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* Initalisation / Deinitalisation Functions
 */
wiced_result_t bt_smart_gatt_init( void );

wiced_result_t bt_smart_gatt_deinit( void );

wiced_result_t bt_smart_gatt_set_timeout( uint32_t timeout_ms );

/* Callback Registration Function
 */
wiced_result_t bt_smart_gatt_register_notification_indication_handler( bt_smart_gatt_notification_indication_handler_t handler );


/* Server Configuration Function
 */
wiced_result_t bt_smart_gatt_exchange_mtu( uint16_t connection_handle, uint16_t client_mtu, uint16_t* server_mtu );


/* Primary Service Discovery Functions
 */
wiced_result_t bt_smart_gatt_discover_all_primary_services( uint16_t connection_handle, wiced_bt_smart_attribute_list_t* service_list );

wiced_result_t bt_smart_gatt_discover_primary_services_by_uuid( uint16_t connection_handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_list_t* service_list );


/* Relationship Discovery Functions
 */
wiced_result_t bt_smart_gatt_find_included_services( uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* include_list );


/* Characteristic Discovery Functions
 */
wiced_result_t bt_smart_gatt_discover_all_characteristics_in_a_service( uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* characteristic_list );

wiced_result_t bt_smart_gatt_discover_characteristics_by_uuid( uint16_t connection_handle, const wiced_bt_uuid_t* uuid, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* characteristic_list );


/* Characteristic Descriptor Discovery Functions
 */
wiced_result_t bt_smart_gatt_discover_all_characteristic_descriptors( uint16_t connection_handle, uint16_t start_handle, uint16_t end_handle, wiced_bt_smart_attribute_list_t* no_value_descriptor_list );


/* Characteristic Descriptor Read Functions
 */
wiced_result_t bt_smart_gatt_read_characteristic_descriptor( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_t** descriptor );

wiced_result_t bt_smart_gatt_read_long_characteristic_descriptor( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_t** descriptor );


/* Characteristic Descriptor Write Functions
 */
wiced_result_t bt_smart_gatt_write_characteristic_descriptor( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute );

wiced_result_t bt_smart_gatt_write_long_characteristic_descriptor( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute );


/* Characteristic Value Read Functions
 */
wiced_result_t bt_smart_gatt_read_characteristic_value( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* type, wiced_bt_smart_attribute_t** characteristic_value );

wiced_result_t bt_smart_gatt_read_characteristic_values_using_uuid( uint16_t connection_handle,  const wiced_bt_uuid_t* uuid, wiced_bt_smart_attribute_list_t* characteristic_value_list );

wiced_result_t bt_smart_gatt_read_long_characteristic_value( uint16_t connection_handle, uint16_t handle, const wiced_bt_uuid_t* type, wiced_bt_smart_attribute_t** characteristic_value );


/* Characteristic Value Write Functions
 */
//wiced_result_t bt_smart_gatt_write_characteristic_value_without_response( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute );

//wiced_result_t bt_smart_gatt_signed_write_characteristic_value_without_response( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute );

wiced_result_t bt_smart_gatt_write_characteristic_value( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute );

wiced_result_t bt_smart_gatt_write_long_characteristic_value( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute );

//wiced_result_t bt_smart_gatt_reliable_write_characteristic_value( uint16_t connection_handle, wiced_bt_smart_attribute_t* attribute );

wiced_result_t subprocedure_notify_complete               ( void );
wiced_result_t subprocedure_unlock                        ( void );
wiced_result_t subprocedure_lock                          ( void );
wiced_result_t subprocedure_reset                         ( void );
wiced_result_t subprocedure_wait_for_completion           ( void );
wiced_result_t subprocedure_wait_clear_semaphore          ( void );
#ifdef __cplusplus
} /* extern "C" */
#endif
