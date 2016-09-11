/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

/**************************************************************************************************************
 * INCLUDES
 **************************************************************************************************************/

#include "wiced_tcpip.h"
#include "wiced_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************************
 * CONSTANTS
 **************************************************************************************************************/

/**************************************************************************************************************
 * STRUCTURES
 **************************************************************************************************************/

typedef struct
{
    wiced_ip_address_t ip;
    uint16_t           port;
    const char*        service_name;  /* This variable is used internally */
    char*              instance_name;
    char*              hostname;
    wiced_semaphore_t* semaphore;     /* This variable is used internally */
    volatile wiced_bool_t       instance_resolved;
    volatile wiced_bool_t       hostname_resolved;
    volatile wiced_bool_t       service_resolved;
    volatile wiced_bool_t       is_resolved;
} gedday_service_t;

typedef struct {
    char*         buffer;
    uint16_t      buffer_length;
    unsigned int  current_size;
    wiced_mutex_t mutex;
} gedday_text_record_t;
/**************************************************************************************************************
 * VARIABLES
 **************************************************************************************************************/

/**************************************************************************************************************
 * FUNCTION DECLARATIONS
 **************************************************************************************************************/

extern void           gedday_init            ( wiced_interface_t interface, const char* desired_name );
extern wiced_result_t gedday_discover_service( const char* service_query, gedday_service_t* service_result );
extern wiced_result_t gedday_update_service                ( const char* instance_name, const char* service_name );
extern wiced_result_t gedday_add_service     ( const char* instance_name, const char* service_name, uint16_t port, uint32_t ttl, const char* txt );
extern wiced_result_t gedday_add_dynamic_text_record       ( const char* instance_name, const char* service_name, gedday_text_record_t* text_record );
extern wiced_result_t gedday_remove_service                ( const char* instance_name, const char* service_name );
extern const char*    gedday_get_hostname    ( void );
extern wiced_result_t gedday_update_ip       ( void );
extern void           gedday_deinit          ( void );
wiced_result_t        gedday_text_record_create            ( gedday_text_record_t* text_record_ptr, uint16_t buffer_length, void *buffer );
wiced_result_t        gedday_text_record_delete            ( gedday_text_record_t* text_record_ptr );
wiced_result_t        gedday_text_record_set_key_value_pair( gedday_text_record_t* text_record_ptr, char* key, char* value );
char*                 gedday_text_record_get_string        ( gedday_text_record_t* text_record_ptr );

#ifdef __cplusplus
} /* extern "C" */
#endif
