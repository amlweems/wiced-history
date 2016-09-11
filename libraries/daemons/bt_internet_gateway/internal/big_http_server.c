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
#include "http_server.h"
#include "big_http_server.h"

#ifdef BIG_INCLUDES_RESTFUL_SMART_SERVER
#include "restful_smart_uri.h"
#endif

#ifdef BIG_INCLUDES_BLE_MESH
#include "blemesh_uri.h"
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define BIG_HTTP_SERVER_STACK_SIZE  ( 5000 )

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

static wiced_http_server_t http_server;

/* The order of the paths matters because of the use of wildcard character '*' */
static START_OF_HTTP_PAGE_DATABASE( web_pages )
#ifdef BIG_INCLUDES_RESTFUL_SMART_SERVER
    RESTFUL_SMART_URIS
#endif
#ifdef BIG_INCLUDES_BLE_MESH
    BLE_MESH_URIS
#endif
END_OF_HTTP_PAGE_DATABASE();

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t big_init_http_server( void )
{
    /* Start HTTP server */
    return wiced_http_server_start( &http_server, 80, 5, web_pages, WICED_STA_INTERFACE, BIG_HTTP_SERVER_STACK_SIZE );

}

wiced_result_t big_deinit_http_server( void )
{
    return wiced_http_server_stop( &http_server );
}
