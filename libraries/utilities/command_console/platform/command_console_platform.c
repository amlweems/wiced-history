/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "wwd_debug.h"
#include "wwd_wifi.h"
#include "wiced_framework.h"
#include "wiced_rtos.h"
#include "wiced_platform.h"
#include "wiced_deep_sleep.h"
#include "wiced_time.h"
#include "wiced_crypto.h"
#include "wiced_management.h"
#include "platform/wwd_bus_interface.h"
#include "command_console.h"

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
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

int reboot_console_command( int argc, char *argv[] )
{
    printf( "Rebooting...\n" );

    wiced_rtos_delay_milliseconds( 1000 );

    wiced_framework_reboot();

    /* Never reached */
    return ERR_CMD_OK;
}

int get_time_console_command( int argc, char *argv[] )
{
    wiced_time_t curr_time;

    wiced_time_get_time( &curr_time );

    printf( "Time: %u\n", (unsigned)curr_time );

    return ERR_CMD_OK;
}

int sleep_console_command( int argc, char *argv[] )
{
    wiced_rtos_delay_milliseconds( atoi( argv[ 1 ] ) );
    return ERR_CMD_OK;
}

int prng_console_command( int argc, char* argv[] )
{
    uint16_t buffer_length = atoi( argv[ 1 ] );
    uint8_t* buffer = malloc( buffer_length );
    wiced_result_t result;

    result = wiced_crypto_get_random( buffer, buffer_length );
    if ( result != WICED_SUCCESS )
    {
        printf( "Failed to get random number: %d\n", (int)result );
    }
    else
    {
        uint16_t i;
        for ( i = 0; i < buffer_length; i++ )
        {
            printf( "0x%02x ", (unsigned)buffer[ i ] );
            if ( ( i % 8 ) == 7 )
            {
                printf( "\n" );
            }
        }
        printf( "\n" );
    }

    free( buffer );

    return ERR_CMD_OK;
}

int mcu_powersave_console_command( int argc, char *argv[] )
{
    int enable = atoi( argv[ 1 ] );

    if ( enable )
    {
        wiced_platform_mcu_enable_powersave();
    }
    else
    {
        wiced_platform_mcu_disable_powersave();
    }

    return ERR_CMD_OK;
}

int wiced_init_console_command( int argc, char *argv[] )
{
    int init = atoi( argv[ 1 ] );

    if ( init )
    {
        wiced_init();
    }
    else
    {
        wiced_deinit();
    }

    return ERR_CMD_OK;
}

#ifdef BCM43909

int mcu_powersave_clock_console_command( int argc, char *argv[] )
{
    int request_par = atoi( argv[ 1 ] );
    int clock_par = atoi( argv[ 2 ] );
    platform_mcu_powersave_clock_t clock = clock_par;

    if ( request_par )
    {
        platform_mcu_powersave_request_clock( clock );
    }
    else
    {
        platform_mcu_powersave_release_clock( clock );
    }

    return ERR_CMD_OK;
}

int mcu_powersave_tick_console_command( int argc, char *argv[] )
{
    int tick_mode_par = atoi( argv[ 1 ] );
    platform_tick_powersave_mode_t tick_mode = tick_mode_par;

    platform_mcu_powersave_set_tick_mode( tick_mode );

    return ERR_CMD_OK;
}

int mcu_powersave_mode_console_command( int argc, char *argv[] )
{
    int mcu_mode_par = atoi( argv[ 1 ] );
    platform_mcu_powersave_mode_t mcu_mode = mcu_mode_par;

    platform_mcu_powersave_set_mode( mcu_mode );

    return ERR_CMD_OK;
}

int mcu_powersave_freq_console_command( int argc, char *argv[] )
{
    int freq_mode_par = atoi( argv[ 1 ] );
    platform_cpu_clock_frequency_t freq_mode = freq_mode_par;
    wiced_bool_t result;

    platform_tick_stop();
    result = platform_cpu_clock_init( freq_mode );
    platform_tick_start();

    if ( result != WICED_TRUE )
    {
        printf( "Failed to set %d frequency mode\n", freq_mode_par );
    }

    return ERR_CMD_OK;
}

void WEAK NEVER_INLINE mcu_powersave_deep_sleep_event_handler( wiced_bool_t before )
{
    static uint32_t WICED_DEEP_SLEEP_SAVED_VAR( rxbeaconmbss_saved );
    static uint32_t WICED_DEEP_SLEEP_SAVED_VAR( rxbeaconobss_saved );

    static wiced_counters_t counters; /* large object, keep it static */

    uint32_t rxbeaconmbss = 0;
    uint32_t rxbeaconobss = 0;

    if ( wwd_wifi_get_counters( WWD_STA_INTERFACE, &counters ) == WWD_SUCCESS )
    {
        rxbeaconmbss = counters.rxbeaconmbss;
        rxbeaconobss = counters.rxbeaconobss;
    }

    if ( before )
    {
        rxbeaconmbss_saved = rxbeaconmbss;
        rxbeaconobss_saved = rxbeaconobss;
    }
    else
    {
        uint32_t time_since_deep_sleep_enter = wiced_deep_sleep_ticks_since_enter(); /* call before any prints to not skew results by delays due to printing */

        printf( "\n%s boot\n", WICED_DEEP_SLEEP_IS_WARMBOOT() ? "WARM" : "COLD" );
        printf( "BEACONS:\n" );
        printf( "    before sleep: from BSS %u from not BSS %u\n", (unsigned)rxbeaconmbss_saved, (unsigned)rxbeaconobss_saved );
        printf( "    after  sleep: from BSS %u from not BSS %u\n", (unsigned)rxbeaconmbss, (unsigned)rxbeaconobss );
        printf( "    during sleep: from BSS %u from not BSS %u\n", (unsigned)(rxbeaconmbss - rxbeaconmbss_saved), (unsigned)(rxbeaconobss - rxbeaconobss_saved) );
        printf( "TIME since deep-sleep enter: %u\n", (unsigned)time_since_deep_sleep_enter );
    }
}

int mcu_powersave_sleep_console_command( int argc, char *argv[] )
{
    int mode = atoi( argv[ 1 ] );
    int sleep_ms = atoi( argv[ 2 ] );

    mcu_powersave_deep_sleep_event_handler( WICED_TRUE );

    wiced_platform_mcu_enable_powersave();

    wwd_thread_notify_irq();

    if ( mode == 0 )
    {
        wiced_rtos_delay_milliseconds( sleep_ms );
    }
    else
    {
        int i;

        i = 0;
        while ( 1  )
        {
            uint32_t flags;

            WICED_SAVE_INTERRUPTS( flags );

            if ( PLATFORM_WLAN_POWERSAVE_IS_RES_UP() )
            {
                WICED_RESTORE_INTERRUPTS( flags );

                if ( ++i > 1000 )
                {
                    printf( "Refuse to sleep due to WLAN resources are still requested\n" );
                    break;
                }

                wiced_rtos_delay_milliseconds( 1 );
            }
            else
            {
                platform_mcu_powersave_sleep( sleep_ms, PLATFORM_TICK_SLEEP_FORCE_INTERRUPTS_WLAN_ON );

                WICED_RESTORE_INTERRUPTS( flags );

                break;
            }
        }
    }

    wiced_platform_mcu_disable_powersave();

    mcu_powersave_deep_sleep_event_handler( WICED_FALSE );

    return ERR_CMD_OK;
}

int mcu_powersave_info_console_command( int argc, char *argv[] )
{
    int i;

    if ( WICED_DEEP_SLEEP_IS_ENABLED() )
    {
        printf( "Deep-sleep enabled. " );

        if ( WICED_DEEP_SLEEP_IS_WARMBOOT() )
        {
            printf( "Ticks since enter deep-sleep: %u.\n", (unsigned)wiced_deep_sleep_ticks_since_enter());
        }
        else
        {
            printf( "This is cold boot.\n" );
        }
    }
    else
    {
        printf( "Deep-sleep disabled or not supported.\n" );
    }

    if ( platform_hibernation_is_returned_from( ) )
    {
        printf( "Returned from hibernation where spent %u ticks\n", (unsigned)platform_hibernation_get_ticks_spent( ) );
    }

    printf( "MCU powersave is %s now.\n",  platform_mcu_powersave_is_permitted() ? "enabled" : "disabled");

    printf( "MCU powersave mode is %d now.\n",  (int)platform_mcu_powersave_get_mode() );

    printf( "MCU powersave tick mode is %d now.\n",  (int)platform_mcu_powersave_get_tick_mode() );

    for ( i = 0; i < PLATFORM_MCU_POWERSAVE_CLOCK_MAX; i++ )
    {
        printf( "MCU clock %d requested %lu times.\n", i, (unsigned long)platform_mcu_powersave_get_clock_request_counter( (platform_mcu_powersave_clock_t)i ) );
    }

    return ERR_CMD_OK;
}

int mcu_powersave_gpio_wakeup_enable_console_command( int argc, char *argv[] )
{
    int                                          gpio_wakeup_config_par  = atoi( argv[ 1 ] );
    int                                          gpio_wakeup_trigger_par = atoi( argv[ 2 ] );
    platform_mcu_powersave_gpio_wakeup_config_t  gpio_wakeup_config      = gpio_wakeup_config_par;
    platform_mcu_powersave_gpio_wakeup_trigger_t gpio_wakeup_trigger     = gpio_wakeup_trigger_par;
    platform_result_t                            result;

    result = platform_mcu_powersave_gpio_wakeup_enable( gpio_wakeup_config, gpio_wakeup_trigger );
    if ( result != PLATFORM_SUCCESS )
    {
        printf( "Enabling failure: %d\n",  result );
    }

    return ERR_CMD_OK;
}

int mcu_powersave_gpio_wakeup_disable_console_command( int argc, char *argv[] )
{
    platform_mcu_powersave_gpio_wakeup_disable();
    return ERR_CMD_OK;
}

int mcu_powersave_gpio_wakeup_ack_console_command( int argc, char *argv[] )
{
    platform_mcu_powersave_gpio_wakeup_ack();
    return ERR_CMD_OK;
}

int mcu_powersave_gci_gpio_wakeup_enable_console_command( int argc, char *argv[] )
{
    int                                           gpio_pin_par           = atoi( argv[ 1 ] );
    int                                           gpio_wakeup_config_par = atoi( argv[ 2 ] );
    int                                           gpio_trigger_par       = atoi( argv[ 3 ] );
    platform_pin_t                                gpio_pin               = gpio_pin_par;
    platform_mcu_powersave_gpio_wakeup_config_t   gpio_wakeup_config     = gpio_wakeup_config_par;
    platform_gci_gpio_irq_trigger_t               gpio_trigger           = gpio_trigger_par;
    platform_result_t                             result;

    result = platform_mcu_powersave_gci_gpio_wakeup_enable( gpio_pin, gpio_wakeup_config, gpio_trigger );
    if ( result != PLATFORM_SUCCESS )
    {
        printf( "Enabling failure: %d\n",  result );
    }

    return ERR_CMD_OK;
}

int mcu_powersave_gci_gpio_wakeup_disable_console_command( int argc, char *argv[] )
{
    int            gpio_pin_par = atoi( argv[ 1 ] );
    platform_pin_t gpio_pin     = gpio_pin_par;

    platform_mcu_powersave_gci_gpio_wakeup_disable( gpio_pin );

    return ERR_CMD_OK;
}

int mcu_powersave_gci_gpio_wakeup_ack_console_command( int argc, char *argv[] )
{
    int            gpio_pin_par = atoi( argv[ 1 ] );
    platform_pin_t gpio_pin     = gpio_pin_par;
    wiced_bool_t   res          = platform_mcu_powersave_gci_gpio_wakeup_ack( gpio_pin );

    printf( "Ack %s\n", res ? "SUCCEEDED" : "FAILED" );

    return ERR_CMD_OK;
}

int hibernation_console_command( int argc, char *argv[] )
{
    uint32_t          freq            = platform_hibernation_get_clock_freq();
    uint32_t          max_ticks       = platform_hibernation_get_max_ticks();
    platform_result_t result          = PLATFORM_BADARG;
    uint32_t          ms_to_wakeup    = atoi( argv[ 1 ] );
    uint32_t          ticks_to_wakeup = freq * ms_to_wakeup / 1000;

    printf( "\n\n*** To make it work please make sure that application is flashed if not done so! ***\n\n" );
    printf( "Frequency %u maximum ticks 0x%x\n", (unsigned)freq, (unsigned)max_ticks );

    if ( ticks_to_wakeup > max_ticks )
    {
        printf( "Scheduled ticks number %u is too big\n", (unsigned)ticks_to_wakeup );
    }
    else
    {
        printf( "After short sleep will hibernate for %u ms (or %u ticks)\n", (unsigned)ms_to_wakeup, (unsigned)ticks_to_wakeup );

        host_rtos_delay_milliseconds( 1000 );

        result = platform_hibernation_start( ticks_to_wakeup );
    }

    printf( "Hibernation failure: %d\n", (int)result );

    return ERR_CMD_OK;
}

int mcu_wlan_powersave_stats_console_command( int argc, char *argv[] )
{
#if PLATFORM_WLAN_POWERSAVE
    uint32_t     call_num     = platform_wlan_powersave_get_stats( PLATFORM_WLAN_POWERSAVE_STATS_CALL_NUM );
    uint32_t     up_time      = platform_wlan_powersave_get_stats( PLATFORM_WLAN_POWERSAVE_STATS_UP_TIME );
    uint32_t     wait_up_time = platform_wlan_powersave_get_stats( PLATFORM_WLAN_POWERSAVE_STATS_WAIT_UP_TIME );
    wiced_bool_t is_res_up    = platform_wlan_powersave_is_res_up();

    printf( "call_num=%lu up_time=%lu wait_up_time=%lu is_res_up=%d\n", (unsigned long)call_num, (unsigned long)up_time, (unsigned long)wait_up_time, is_res_up );
#else
    printf( "WLAN powersave is not compiled-in\n" );
#endif /* PLATFORM_WLAN_POWERSAVE */

    return ERR_CMD_OK;
}

WICED_DEEP_SLEEP_EVENT_HANDLER( deep_sleep_event_handler )
{
    if ( event == WICED_DEEP_SLEEP_EVENT_WLAN_RESUME )
    {
        mcu_powersave_deep_sleep_event_handler( WICED_FALSE );
    }
}

#endif /* BCM43909 */
