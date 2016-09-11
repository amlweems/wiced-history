
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
 *  Unit Tester for Wiced layer
 *
 */

#include "gtest/gtest.h"
#include "wiced.h"
#include "mock.h"

#define RETCHK_START( retcheck_var_in )  \
        { \
            int count = 0; \
            int* retcheck_var = &(retcheck_var_in); \
            do \
            { \
                *retcheck_var = count; \
                count++;

#define RETCHK_ACTIVATED()  (*retcheck_var == -1)

#define RETCHK_EXPECT_EQ( expected, val )  if ( RETCHK_ACTIVATED( ) ) { EXPECT_EQ( (expected), (val) ); }
#define RETCHK_EXPECT_NE( expected, val )  if ( RETCHK_ACTIVATED( ) ) { EXPECT_NE( (expected), (val) ); }
#define RETCHK_EXPECT_GT( expected, val )  if ( RETCHK_ACTIVATED( ) ) { EXPECT_GT( (expected), (val) ); }

#define RETCHK_COUNT( ) count

#define RETCHK_END( ) \
            } while ( *retcheck_var == -1 ); \
            *retcheck_var = -1; \
        }


static int bad_malloc_countdown = -1;
static const wiced_ip_setting_t ip_settings  = { .ip_address = { WICED_IPV4, { .v4 = MAKE_IPV4_ADDRESS(192, 168, 1, 105) } },
                                                 .gateway    = { WICED_IPV4, { .v4 = MAKE_IPV4_ADDRESS(192, 168, 1,   1) } },
                                                 .netmask    = { WICED_IPV4, { .v4 = MAKE_IPV4_ADDRESS(192, 168, 1, 255) } },
};

static void* wiced_unit_malloc( size_t size );
static void * wiced_unit_malloc_named( const char* name, size_t size );
extern "C" void * __real_malloc_named( const char* name, size_t size );
extern "C" void * malloc_debug_malloc( size_t size );

class unit_test_wiced_init :  public ::testing::Test
{

    protected:

    virtual void SetUp()
    {
        malloc_leak_set_ignored( LEAK_CHECK_GLOBAL );
    }
    virtual void TearDown()
    {
        malloc_leak_check( NULL, LEAK_CHECK_GLOBAL );
    }
};

/* Initialise WicedFS normally */
TEST_F(unit_test_wiced_init, normal)
{
    EXPECT_EQ( WICED_SUCCESS, wiced_init( ) );
    EXPECT_EQ( WICED_SUCCESS, wiced_deinit( ) );
}

TEST_F(unit_test_wiced_init, double_init)
{
    EXPECT_EQ( WICED_SUCCESS, wiced_init( ) );
    EXPECT_EQ( WICED_SUCCESS, wiced_init( ) );
    EXPECT_EQ( WICED_SUCCESS, wiced_deinit( ) );
}

TEST_F(unit_test_wiced_init, double_deinit)
{
    EXPECT_EQ( WICED_SUCCESS, wiced_init( ) );
    EXPECT_EQ( WICED_SUCCESS, wiced_deinit( ) );
    EXPECT_EQ( WICED_SUCCESS, wiced_deinit( ) );
}


TEST_F(unit_test_wiced_init, bad_mallocs_init)
{
    set_mock_function( malloc, wiced_unit_malloc );
    set_mock_function( malloc_named, wiced_unit_malloc_named );

//    malloc(1);

    RETCHK_START( bad_malloc_countdown )
    {
        RETCHK_EXPECT_NE( WICED_SUCCESS, wiced_init( ) );
        malloc_leak_check( NULL, LEAK_CHECK_GLOBAL );
    }
    RETCHK_END( )

    reset_mock_function( malloc );
    reset_mock_function( malloc_named );
}



TEST_F(unit_test_wiced_init, bad_mallocs_up)
{
    EXPECT_EQ( WICED_SUCCESS, wiced_init( ) );
    malloc_leak_set_ignored( LEAK_CHECK_GLOBAL );

    set_mock_function( malloc, wiced_unit_malloc );
    set_mock_function( malloc_named, wiced_unit_malloc_named );

//    malloc(1);

    RETCHK_START( bad_malloc_countdown )
    {
        RETCHK_EXPECT_NE( WICED_SUCCESS, wiced_network_up(WICED_STA_INTERFACE, WICED_USE_STATIC_IP, &ip_settings) );
        malloc_leak_check( NULL, LEAK_CHECK_GLOBAL );
    }
    RETCHK_END( )

    reset_mock_function( malloc );
    reset_mock_function( malloc_named );

    EXPECT_EQ( WICED_SUCCESS, wiced_deinit( ) );

}



TEST_F(unit_test_wiced_init, net_up_normal)
{
    EXPECT_EQ( WICED_SUCCESS, wiced_init( ) );
    EXPECT_EQ( WICED_SUCCESS, wiced_network_up(WICED_STA_INTERFACE, WICED_USE_STATIC_IP, &ip_settings) );
    EXPECT_EQ( WICED_SUCCESS, wiced_network_down( WICED_STA_INTERFACE ) );
    EXPECT_EQ( WICED_SUCCESS, wiced_deinit( ) );
}

TEST_F(unit_test_wiced_init, net_up_twice)
{
    EXPECT_EQ( WICED_SUCCESS, wiced_init( ) );
    EXPECT_EQ( WICED_SUCCESS, wiced_network_up(WICED_STA_INTERFACE, WICED_USE_STATIC_IP, &ip_settings) );
    EXPECT_EQ( WICED_SUCCESS, wiced_network_up(WICED_STA_INTERFACE, WICED_USE_STATIC_IP, &ip_settings) );
    EXPECT_EQ( WICED_SUCCESS, wiced_network_down( WICED_STA_INTERFACE ) );
    EXPECT_EQ( WICED_SUCCESS, wiced_deinit( ) );
}


TEST_F(unit_test_wiced_init, net_down_twice)
{
    EXPECT_EQ( WICED_SUCCESS, wiced_init( ) );
    EXPECT_EQ( WICED_SUCCESS, wiced_network_up(WICED_STA_INTERFACE, WICED_USE_STATIC_IP, &ip_settings) );
    EXPECT_EQ( WICED_SUCCESS, wiced_network_down( WICED_STA_INTERFACE ) );
    EXPECT_EQ( WICED_SUCCESS, wiced_network_down( WICED_STA_INTERFACE ) );
    EXPECT_EQ( WICED_SUCCESS, wiced_deinit( ) );
}






static void * wiced_unit_malloc_named( const char* name, size_t size )
{
    if ( bad_malloc_countdown == 0 )
    {
        bad_malloc_countdown--;
        return NULL;
    }
    if ( bad_malloc_countdown != -1 )
    {
        bad_malloc_countdown--;
    }

    return __real_malloc_named( name, size );
}


static void* wiced_unit_malloc( size_t size )
{
    if ( bad_malloc_countdown == 0 )
    {
        bad_malloc_countdown--;
        return NULL;
    }
    if ( bad_malloc_countdown != -1 )
    {
        bad_malloc_countdown--;
    }

    return malloc_debug_malloc( size );
}




