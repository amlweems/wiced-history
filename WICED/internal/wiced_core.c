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
#include "wiced_result.h"
#include "wwd_constants.h"
//#include "wiced_internal_api.h"
//#include "wiced_management.h"

extern wiced_result_t wiced_core_init( void );
extern wiced_result_t wiced_core_deinit( void );
extern wiced_result_t wiced_platform_init( void );
extern wiced_result_t wiced_rtos_init  ( void );
wiced_result_t wiced_rtos_deinit( void );

static wiced_bool_t wiced_core_initialised = WICED_FALSE;

wiced_result_t wiced_core_init( void )
{
    if ( wiced_core_initialised == WICED_TRUE )
    {
        return WICED_SUCCESS;
    }

    wiced_platform_init( );

    wiced_rtos_init( );

    wiced_core_initialised = WICED_TRUE;

    return WICED_SUCCESS;
}

wiced_result_t wiced_core_deinit( void )
{
    if ( wiced_core_initialised != WICED_TRUE )
    {
        return WICED_SUCCESS;
    }

    wiced_rtos_deinit();

    wiced_core_initialised = WICED_FALSE;

    return WICED_SUCCESS;
}
