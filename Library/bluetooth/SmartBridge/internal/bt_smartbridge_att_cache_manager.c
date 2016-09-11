/*
 * Copyright 2013, Broadcom Corporation
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

#include "bt_smartbridge_att_cache_manager.h"
#include "bt_linked_list.h"
#include "bt_smart_gatt.h"
#include "wiced.h"
#include "wiced_bt_smartbridge.h"
#include "wiced_bt_smart_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/

#ifdef DEBUG
#define CHECK_FOR_ERROR( condition, error_code ) \
do \
{ \
    if ( ( condition ) ) \
    { \
        WPRINT_LIB_INFO( ( "[SmartBridge Att Cache] Error: %d, function: %s, line: %lu\r\n", (int)error_code, __func__, (uint32_t)__LINE__ ) ); \
        error_code_var = (error_code); \
        goto error; \
    } \
} \
while (0)
#else
#define CHECK_FOR_ERROR( condition, error_code ) \
do \
{ \
    if ( ( condition ) ) \
    { \
        error_code_var = (error_code); \
        goto error; \
    } \
} \
while (0)
#endif

#define CALCULATE_ATT_CACHE_MANAGER_SIZE( cache_count ) \
sizeof( bt_smartbridge_att_cache_manager_t ) + \
sizeof( bt_smartbridge_att_cache_t ) * ( cache_count - 1 )

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

#pragma pack(1)
typedef struct bt_smartbridge_att_cache
{
    bt_list_node_t                  node;
    wiced_bool_t                    is_active;
    wiced_bool_t                    is_discovering;
    wiced_bt_smart_device_t         remote_device;
    uint16_t                        connection_handle;
    wiced_bt_smart_attribute_list_t attribute_list;
    wiced_mutex_t                   mutex;
} bt_smartbridge_att_cache_t;

typedef struct
{
    uint32_t                    count;
    bt_linked_list_t            free_list;
    bt_linked_list_t            used_list;
    wiced_mutex_t               mutex;
    bt_smartbridge_att_cache_t  pool[1];
} bt_smartbridge_att_cache_manager_t;
#pragma pack()

/******************************************************
 *               Function Declarations
 ******************************************************/

static wiced_result_t smartbridge_att_cache_get_free_cache          ( bt_smartbridge_att_cache_t** free_cache );
static wiced_result_t smartbridge_att_cache_insert_to_used_list     ( bt_smartbridge_att_cache_t* instance );
static wiced_result_t smartbridge_att_cache_return_to_free_list     ( bt_smartbridge_att_cache_t* instance );
static wiced_result_t smartbridge_att_cache_discover_all            ( bt_smartbridge_att_cache_t* cache, uint16_t connection_handle );
static wiced_bool_t   smartbridge_att_cache_find_by_device_callback ( bt_list_node_t* node_to_compare, void* user_data );
static wiced_bool_t   smartbridge_att_cache_get_free_callback       ( bt_list_node_t* node_to_compare, void* user_data );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/* Attribute Caching Management Globals */

static bt_smartbridge_att_cache_manager_t* att_cache_manager = NULL;

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t bt_smartbridge_att_cache_enable( uint32_t cache_count )
{
    if ( att_cache_manager == NULL )
    {
        uint32_t a;
        bt_smartbridge_att_cache_manager_t* manager = (bt_smartbridge_att_cache_manager_t*)malloc_named( "att_cache", CALCULATE_ATT_CACHE_MANAGER_SIZE( cache_count ) );

        if ( manager == NULL )
        {
            return WICED_NOMEM;
        }

        memset( manager, 0, CALCULATE_ATT_CACHE_MANAGER_SIZE( cache_count ) );

        manager->count = cache_count;

        if ( bt_linked_list_init( &manager->free_list ) != WICED_SUCCESS )
        {
            WPRINT_LIB_INFO( ( "Error creating linked list\r\n" ) );
            return WICED_ERROR;
        }

        if ( bt_linked_list_init( &manager->used_list ) != WICED_SUCCESS )
        {
            WPRINT_LIB_INFO( ( "Error creating linked list\r\n" ) );
            return WICED_ERROR;
        }

        if ( wiced_rtos_init_mutex( &manager->mutex ) != WICED_SUCCESS )
        {
            WPRINT_LIB_INFO( ( "Error creating mutex\r\n" ) );
            return WICED_ERROR;
        }

        /* Initialise mutexes for protecting access to cached attributes */
        for ( a = 0; a < manager->count; a++ )
        {
            if ( wiced_rtos_init_mutex( &manager->pool[a].mutex ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }

            /* Point node data to cached attribute instance */
            manager->pool[a].node.data = (void*)&manager->pool[a];

            /* Insert cached attribute instance into free list */
            if ( bt_linked_list_insert_at_rear( &manager->free_list, &manager->pool[a].node ) != WICED_SUCCESS )
            {
                return WICED_ERROR;
            }
        }

        /* Set status at the end to prevent cached attributes being used when not initialised properly */
        att_cache_manager = manager;
    }

    return WICED_SUCCESS;
}

wiced_result_t bt_smartbridge_att_cache_disable( void )
{
    if ( att_cache_manager != NULL )
    {
        uint32_t a;
        bt_list_node_t* node = NULL;
        bt_smartbridge_att_cache_manager_t* manager = att_cache_manager;

        /* Set status at the beginning to prevent cached attributes being used even when deinitialisation failed */
        att_cache_manager = NULL;

        while ( bt_linked_list_remove_from_front( &manager->free_list, &node ) == WICED_SUCCESS )
        {
            bt_smartbridge_att_cache_t* cache = (bt_smartbridge_att_cache_t*)node->data;

            wiced_bt_smart_attribute_delete_list( &cache->attribute_list );
        }

        if ( bt_linked_list_deinit( &manager->free_list ) != WICED_SUCCESS )
        {
            WPRINT_LIB_INFO( ( "Error deleting linked list\r\n" ) );
            return WICED_ERROR;
        }

        while ( bt_linked_list_remove_from_front( &manager->used_list, &node ) == WICED_SUCCESS )
        {
            bt_smartbridge_att_cache_t* cache = (bt_smartbridge_att_cache_t*)node->data;

            wiced_bt_smart_attribute_delete_list( &cache->attribute_list );
        }

        if ( bt_linked_list_deinit( &manager->used_list ) != WICED_SUCCESS )
        {
            WPRINT_LIB_INFO( ( "Error deleting linked list\r\n" ) );
            return WICED_ERROR;
        }

        if ( wiced_rtos_deinit_mutex( &manager->mutex ) != WICED_SUCCESS )
        {
            WPRINT_LIB_INFO( ( "Error deleting mutex\r\n" ) );
            return WICED_ERROR;
        }

        /* Deinitialise mutexes for protecting access to cached attributes */
        for ( a = 0; a < manager->count; a++ )
        {
            if ( wiced_rtos_deinit_mutex( &manager->pool[a].mutex ) != WICED_SUCCESS )
            {
                WPRINT_LIB_INFO( ( "Error deleting mutex\r\n" ) );
                return WICED_ERROR;
            }
        }

        memset( manager, 0, CALCULATE_ATT_CACHE_MANAGER_SIZE( manager->count ) );

        free( manager );
    }

    return WICED_SUCCESS;
}

wiced_result_t bt_smartbridge_att_cache_get_list( bt_smartbridge_att_cache_t* cache, wiced_bt_smart_attribute_list_t** list )
{
    if ( cache == NULL || list == NULL )
    {
        return WICED_BADARG;
    }

    if ( att_cache_manager == NULL )
    {
        return WICED_ERROR;
    }

    *list = &cache->attribute_list;
    return WICED_SUCCESS;
}

wiced_bool_t   bt_smartbridge_att_cache_is_enabled( void )
{
    return ( att_cache_manager == NULL ) ? WICED_FALSE : WICED_TRUE;
}

wiced_bool_t   bt_smartbridge_att_cache_is_discovering( const bt_smartbridge_att_cache_t* cache )
{
    if ( cache == NULL || att_cache_manager == NULL )
    {
        return WICED_FALSE;
    }

    return cache->is_discovering;
}

wiced_bool_t   bt_smartbridge_att_cache_get_active_state( const bt_smartbridge_att_cache_t* cache )
{
    if ( cache == NULL || att_cache_manager == NULL )
    {
        return WICED_FALSE;
    }

    return cache->is_active;
}

wiced_result_t bt_smartbridge_att_cache_set_active_state( bt_smartbridge_att_cache_t* cache, wiced_bool_t is_active )
{
    if ( cache == NULL )
    {
        return WICED_BADARG;
    }

    if ( att_cache_manager == NULL )
    {
        return WICED_ERROR;
    }

    cache->is_active = is_active;
    return WICED_SUCCESS;
}

wiced_result_t bt_smartbridge_att_cache_lock( bt_smartbridge_att_cache_t* cache )
{
    if ( cache == NULL )
    {
        return WICED_BADARG;
    }

    if ( att_cache_manager == NULL )
    {
        return WICED_ERROR;
    }

    return wiced_rtos_lock_mutex( &cache->mutex );
}

wiced_result_t bt_smartbridge_att_cache_unlock( bt_smartbridge_att_cache_t* cache )
{
    if ( cache == NULL )
    {
        return WICED_BADARG;
    }

    if ( att_cache_manager == NULL )
    {
        return WICED_ERROR;
    }

    return wiced_rtos_unlock_mutex( &cache->mutex );
}

wiced_result_t bt_smartbridge_att_cache_find( const wiced_bt_smart_device_t* remote_device, bt_smartbridge_att_cache_t** cache )
{
    wiced_result_t  result;
    bt_list_node_t* node_found;

    if ( remote_device == NULL || cache == NULL )
    {
        return WICED_BADARG;
    }

    if ( att_cache_manager == NULL )
    {
        return WICED_ERROR;
    }

    /* Lock protection */
    if ( wiced_rtos_lock_mutex( &att_cache_manager->mutex ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    result = bt_linked_list_find( &att_cache_manager->used_list, smartbridge_att_cache_find_by_device_callback, (void*)remote_device, &node_found );

    if ( result == WICED_SUCCESS )
    {
        *cache = (bt_smartbridge_att_cache_t*)node_found->data;
    }

    /* Unlock protection */
    if ( wiced_rtos_unlock_mutex( &att_cache_manager->mutex ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    return result;
}

wiced_result_t bt_smartbridge_att_cache_generate( const wiced_bt_smart_device_t* remote_device, uint16_t connection_handle, bt_smartbridge_att_cache_t** cache )
{
    bt_smartbridge_att_cache_t* new_cache = NULL;
    wiced_result_t              result;

    if ( cache == NULL )
    {
        return WICED_BADARG;
    }

    if ( att_cache_manager == NULL )
    {
        return WICED_ERROR;
    }

    /* Cached attributes not found. Get a free instance and discover services */
    result = smartbridge_att_cache_get_free_cache( &new_cache );

    if ( result != WICED_SUCCESS )
    {
        return result;
    }

    /* Copy remote device to cache */
    wiced_rtos_lock_mutex( &new_cache->mutex );
    memcpy( &new_cache->remote_device, remote_device, sizeof( new_cache->remote_device ) );
    new_cache->connection_handle = connection_handle;
    new_cache->is_discovering = WICED_TRUE;
    wiced_rtos_unlock_mutex( &new_cache->mutex );

    /* Rediscover services */
    result = smartbridge_att_cache_discover_all( new_cache, new_cache->connection_handle );

    wiced_rtos_lock_mutex( &new_cache->mutex );
    new_cache->is_discovering = WICED_FALSE;
    wiced_rtos_unlock_mutex( &new_cache->mutex );

    if ( result == WICED_SUCCESS )
    {
        result = smartbridge_att_cache_insert_to_used_list( new_cache );

        if ( result == WICED_SUCCESS )
        {
            *cache = new_cache;
        }
    }
    else
    {
        smartbridge_att_cache_return_to_free_list( new_cache );
    }

    return result;
}

static wiced_result_t smartbridge_att_cache_get_free_cache( bt_smartbridge_att_cache_t** free_cache )
{
    wiced_result_t  result;
    bt_list_node_t* node;

    if ( att_cache_manager == NULL )
    {
        return WICED_ERROR;
    }

    /* Lock protection */
    if ( wiced_rtos_lock_mutex( &att_cache_manager->mutex ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    /* Remove from front of free list */
    result = bt_linked_list_remove_from_front( &att_cache_manager->free_list, &node );

    /* Free list is empty. Remove the oldest one from used list */
    if ( result != WICED_SUCCESS )
    {
        result = bt_linked_list_find( &att_cache_manager->used_list, smartbridge_att_cache_get_free_callback, NULL, &node );

        if ( result == WICED_SUCCESS )
        {
            result = bt_linked_list_remove( &att_cache_manager->used_list, node );

            if ( result == WICED_SUCCESS )
            {
                wiced_bt_smart_attribute_list_t* list = (wiced_bt_smart_attribute_list_t*)node->data;

                /* Delete list and set data to NULL */
                wiced_bt_smart_attribute_delete_list( list );
            }
        }
    }

    if ( result == WICED_SUCCESS )
    {
        *free_cache = (bt_smartbridge_att_cache_t*)node->data;
    }

    /* Unlock protection */
    if ( wiced_rtos_unlock_mutex( &att_cache_manager->mutex ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    return result;
}

static wiced_result_t smartbridge_att_cache_insert_to_used_list( bt_smartbridge_att_cache_t* cache )
{
    wiced_result_t  result;

    if ( att_cache_manager == NULL )
    {
        return WICED_ERROR;
    }

    /* Lock protection */
    if ( wiced_rtos_lock_mutex( &att_cache_manager->mutex ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    result = bt_linked_list_insert_at_rear( &att_cache_manager->used_list, &cache->node );

    /* Unlock protection */
    if ( wiced_rtos_unlock_mutex( &att_cache_manager->mutex ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    return result;
}

static wiced_result_t smartbridge_att_cache_return_to_free_list( bt_smartbridge_att_cache_t* cache )
{
    wiced_result_t  result;

    if ( att_cache_manager == NULL )
    {
        return WICED_ERROR;
    }

    /* Lock protection */
    if ( wiced_rtos_lock_mutex( &att_cache_manager->mutex ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    result = bt_linked_list_insert_at_rear( &att_cache_manager->free_list, &cache->node );

    /* Unlock protection */
    if ( wiced_rtos_unlock_mutex( &att_cache_manager->mutex ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    return result;
}

static wiced_result_t smartbridge_att_cache_discover_all( bt_smartbridge_att_cache_t* cache, uint16_t connection_handle )
{
    /* This function performs the following:
     * 1. Primary Services discovery
     * 2. Relationship (Included Services) Discovery for every Primary Service
     * 3. Characteristic Discovery for every Primary Service
     * 4. Characteristic Value Read for every Charactertistic
     * 5. Characteristic Descriptor Discovery for every Characteristic
     */

    wiced_bt_smart_attribute_t**     primary_service_array    = NULL;
    wiced_bt_smart_attribute_t**     characteristic_array     = NULL;
    wiced_bt_smart_attribute_t*      characteristic_value     = NULL;
    wiced_bt_smart_attribute_t*      descriptor_with_no_value = NULL;
    wiced_bt_smart_attribute_t*      descriptor_with_value    = NULL;
    wiced_bt_smart_attribute_t*      iterator                 = NULL;
    wiced_result_t                   result                   = WICED_SUCCESS;
    wiced_result_t                   error_code_var           = WICED_ERROR;
    uint32_t                         i                        = 0;
    uint32_t                         j                        = 0;
    uint32_t                         primary_service_count    = 0;
    uint32_t                         characteristic_count     = 0;
    wiced_bt_smart_attribute_list_t  primary_service_list;
    wiced_bt_smart_attribute_list_t  included_service_list;
    wiced_bt_smart_attribute_list_t  characteristic_list;
    wiced_bt_smart_attribute_list_t  descriptor_list;

    if ( att_cache_manager == NULL )
    {
        return WICED_ERROR;
    }

    /* Initialise local variables */
    memset( &primary_service_list,  0, sizeof( primary_service_list  ) );
    memset( &included_service_list, 0, sizeof( included_service_list ) );
    memset( &characteristic_list,   0, sizeof( characteristic_list   ) );
    memset( &descriptor_list,       0, sizeof( descriptor_list       ) );

    wiced_rtos_lock_mutex( &cache->mutex );

    result = wiced_bt_smart_attribute_create_list( &primary_service_list );

    wiced_rtos_unlock_mutex( &cache->mutex );

    CHECK_FOR_ERROR( result != WICED_SUCCESS, result );

    /**************************************************************************
     * Primary Services Discovery
     **************************************************************************/
    result = bt_smart_gatt_discover_all_primary_services( connection_handle, &primary_service_list );

    CHECK_FOR_ERROR( result != WICED_SUCCESS, result );

    wiced_bt_smart_attribute_get_list_count( &primary_service_list, &primary_service_count );

    primary_service_array = (wiced_bt_smart_attribute_t**)malloc_named( "svc_array", primary_service_count * sizeof(wiced_bt_smart_attribute_t));

    CHECK_FOR_ERROR( primary_service_array == NULL, WICED_NOMEM );

    /* Keep the original pointers to the primary service list before the list gets merged */
    wiced_bt_smart_attribute_get_list_head( &primary_service_list, &iterator );

    for ( i = 0; i < primary_service_count; i++ )
    {
        primary_service_array[i] = iterator;
        iterator                 = iterator->next;
    }

    /* Check if characteristic is readable. If not readable, create a control-point attribute */
    for ( i = 0; i < primary_service_count; i++ )
    {
        /* Initialise variable for this iteration */
        memset( &characteristic_list,   0, sizeof( characteristic_list   ) );
        memset( &included_service_list, 0, sizeof( included_service_list ) );
        characteristic_array = NULL;

        /**********************************************************************
         * Relationship Discovery
         **********************************************************************/
        result = bt_smart_gatt_find_included_services( connection_handle, primary_service_array[i]->value.service.start_handle, primary_service_array[i]->value.service.start_handle, &included_service_list );

        CHECK_FOR_ERROR( result == WICED_TIMEOUT, result );

        wiced_rtos_lock_mutex( &cache->mutex );

        result = wiced_bt_smart_attribute_merge_lists( &primary_service_list, &included_service_list );

        wiced_rtos_unlock_mutex( &cache->mutex );

        CHECK_FOR_ERROR( result != WICED_SUCCESS, result );

        /**********************************************************************
         * Characteristic Discovery
         **********************************************************************/
        result = bt_smart_gatt_discover_all_characteristics_in_a_service( connection_handle, primary_service_array[i]->value.service.start_handle, primary_service_array[i]->value.service.end_handle, &characteristic_list );

        CHECK_FOR_ERROR( result == WICED_TIMEOUT, result );

        wiced_bt_smart_attribute_get_list_count( &characteristic_list, &characteristic_count );

        characteristic_array = (wiced_bt_smart_attribute_t**)malloc_named( "char_array", characteristic_count * sizeof(characteristic_list));

        CHECK_FOR_ERROR( characteristic_array == NULL, WICED_NOMEM );

        /* Keep the original pointers to the characteristic list before the list gets merged. */
        wiced_bt_smart_attribute_get_list_head( &characteristic_list, &iterator );

        for ( j = 0; j < characteristic_count; j++ )
        {
            characteristic_array[j] = iterator;
            iterator                = iterator->next;
        }

        /* Traverse through all characteristics to perform Characteristic Value Read and Descriptors Discovery */
        for ( j = 0; j < characteristic_count; j++ )
        {
            /* Initialise local variables for this iteration */
            memset( &descriptor_list, 0, sizeof( descriptor_list ) );
            characteristic_value = NULL;

            /******************************************************************
             * Characteristic Value Read
             ******************************************************************/
            if ( ( characteristic_array[j]->value.characteristic.properties & 0x02 ) != 0 )
            {
                /* If characteristic is readable. If not readable, create a control-point attribute */
                result = bt_smart_gatt_read_characteristic_value( connection_handle, characteristic_array[j]->value.characteristic.value_handle, &characteristic_array[j]->value.characteristic.uuid, &characteristic_value );

                CHECK_FOR_ERROR( result == WICED_TIMEOUT, result );
            }
            else
            {
                /* Failed to read. Let's enter a control-point attribute (dummy) here so when notification come, UUID is known. */
                result = wiced_bt_smart_attribute_create( &characteristic_value, WICED_ATTRIBUTE_TYPE_NO_VALUE, 0 );

                if ( result == WICED_SUCCESS )
                {
                    characteristic_value->handle = characteristic_array[j]->value.characteristic.value_handle;
                    characteristic_value->type   = characteristic_array[j]->value.characteristic.uuid;
                }

                CHECK_FOR_ERROR( result != WICED_SUCCESS, result );
            }

            if ( characteristic_value != NULL )
            {
                /* Add Characteristic Value to main list */
                wiced_rtos_lock_mutex( &cache->mutex );

                result = wiced_bt_smart_attribute_add_to_list( &primary_service_list, characteristic_value );

                wiced_rtos_unlock_mutex( &cache->mutex );

                CHECK_FOR_ERROR( result != WICED_SUCCESS, result );
            }

            /******************************************************************
             * Characteristic Descriptor Discovery
             ******************************************************************/
            if ( characteristic_array[j]->value.characteristic.descriptor_start_handle <= characteristic_array[j]->value.characteristic.descriptor_end_handle )
            {
                result = bt_smart_gatt_discover_all_characteristic_descriptors( connection_handle, characteristic_array[j]->value.characteristic.descriptor_start_handle, characteristic_array[j]->value.characteristic.descriptor_end_handle, &descriptor_list );

                CHECK_FOR_ERROR( result == WICED_TIMEOUT, result );

                wiced_bt_smart_attribute_get_list_head( &descriptor_list, &descriptor_with_no_value );

                /* Traverse through all descriptors */
                while ( descriptor_with_no_value != NULL )
                {
                    /* Initialise variable for this iteration */
                    descriptor_with_value = NULL;

                    result = bt_smart_gatt_read_characteristic_descriptor( connection_handle, descriptor_with_no_value->handle, &descriptor_with_no_value->type, &descriptor_with_value );

                    CHECK_FOR_ERROR( result == WICED_TIMEOUT, result );

                    /* Add Descriptor with Value to main list */
                    result = wiced_bt_smart_attribute_add_to_list( &primary_service_list, descriptor_with_value );

                    CHECK_FOR_ERROR( result != WICED_SUCCESS, result );

                    descriptor_with_no_value = descriptor_with_no_value->next;
                }

                /* Delete the empty descriptor list */
                result = wiced_bt_smart_attribute_delete_list( &descriptor_list );

                CHECK_FOR_ERROR( result != WICED_SUCCESS, result );
            }
        }

        /* Merge Characteristics to main list */
        wiced_rtos_lock_mutex( &cache->mutex );

        result = wiced_bt_smart_attribute_merge_lists( &primary_service_list, &characteristic_list );

        wiced_rtos_unlock_mutex( &cache->mutex );

        CHECK_FOR_ERROR( result != WICED_SUCCESS, result );

        /* Free primary service array */
        free( characteristic_array );
    }

    /* Free primary service array */
    free( primary_service_array );

    /* Successful. Now copy the primary service list to the cached attributes list */
    memcpy( &cache->attribute_list, &primary_service_list, sizeof( cache->attribute_list ) );

    return WICED_SUCCESS;

    error:


    /* Delete all local attributes */
    if ( iterator != NULL )
    {
        wiced_bt_smart_attribute_delete( iterator );
    }

    if ( descriptor_with_no_value != NULL )
    {
        wiced_bt_smart_attribute_delete( descriptor_with_no_value );
    }

    if ( descriptor_with_value != NULL )
    {
        wiced_bt_smart_attribute_delete( descriptor_with_value );
    }

    if ( characteristic_value != NULL )
    {
        wiced_bt_smart_attribute_delete( characteristic_value );
    }

    if ( characteristic_array != NULL )
    {
        free( characteristic_array );
    }

    if ( primary_service_array != NULL )
    {
        free( primary_service_array );
    }

    wiced_bt_smart_attribute_delete_list( &descriptor_list );
    wiced_bt_smart_attribute_delete_list( &characteristic_list );
    wiced_bt_smart_attribute_delete_list( &included_service_list );
    wiced_bt_smart_attribute_delete_list( &primary_service_list );

    /* Return the error code to the caller */
    return error_code_var;
}

static wiced_bool_t   smartbridge_att_cache_find_by_device_callback( bt_list_node_t* node_to_compare, void* user_data )
{
    bt_smartbridge_att_cache_t* cached_attributes = (bt_smartbridge_att_cache_t*)node_to_compare->data;
    wiced_bt_smart_device_t*    remote_device     = (wiced_bt_smart_device_t*)user_data;

    return ( ( memcmp( cached_attributes->remote_device.address.address, remote_device->address.address, sizeof( remote_device->address.address ) ) == 0 ) && ( cached_attributes->remote_device.address_type == remote_device->address_type) ) ? WICED_TRUE : WICED_FALSE;
}

static wiced_bool_t   smartbridge_att_cache_get_free_callback( bt_list_node_t* node_to_compare, void* user_data )
{
    bt_smartbridge_att_cache_t* cache = (bt_smartbridge_att_cache_t*)node_to_compare->data;

    return ( cache->is_active == WICED_FALSE ) ? WICED_TRUE : WICED_FALSE;
}
