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
 *  Defines macro for assertions
 *
 */

#ifndef INCLUDED_WWD_ASSERT_H_
#define INCLUDED_WWD_ASSERT_H_

#include "wwd_debug.h"
#include "wiced_defaults.h"

#ifdef DEBUG
#include "platform_assert.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 * @cond       Macros
 ******************************************************/


#ifdef DEBUG
    #ifdef WPRINT_ENABLE_ERROR
        #define WPRINT_ERROR(args)                      { WPRINT_MACRO(args); WICED_ASSERTION_FAIL_ACTION(); }
        #define wiced_assert( error_string, assertion ) { if (!(assertion)) { WICED_ASSERTION_FAIL_ACTION(); } }
    #else
        #define WPRINT_ERROR(args)                      { WICED_ASSERTION_FAIL_ACTION();}
        #define wiced_assert( error_string, assertion ) { if (!(assertion)) { WICED_ASSERTION_FAIL_ACTION();} }
    #endif
#else
    #define wiced_assert( error_string, assertion )
#endif


/** @endcond */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ifndef INCLUDED_WWD_ASSERT_H_ */
