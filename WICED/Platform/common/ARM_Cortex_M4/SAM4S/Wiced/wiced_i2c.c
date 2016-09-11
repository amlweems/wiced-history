/*
 * Copyright 2014, Broadcom Corporation
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
#include "sam4s_platform.h"
#include "wiced_platform.h"
#include "wiced_utilities.h"

/******************************************************
 *                      Macros
 ******************************************************/

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

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t wiced_i2c_init( wiced_i2c_device_t* device )
{
    UNUSED_PARAMETER( device );
    return WICED_SUCCESS;
}

wiced_bool_t wiced_i2c_probe_device( wiced_i2c_device_t* device, int retries )
{
    UNUSED_PARAMETER( device );
    UNUSED_PARAMETER( retries );
    return WICED_SUCCESS;
}

wiced_result_t wiced_i2c_transfer( wiced_i2c_device_t* device, wiced_i2c_message_t* message, uint16_t number_of_messages )
{
    UNUSED_PARAMETER( device );
    UNUSED_PARAMETER( message );
    UNUSED_PARAMETER( number_of_messages );
    return WICED_SUCCESS;
}

wiced_result_t wiced_i2c_deinit( wiced_i2c_device_t* device )
{
    UNUSED_PARAMETER( device );
    return WICED_SUCCESS;
}
