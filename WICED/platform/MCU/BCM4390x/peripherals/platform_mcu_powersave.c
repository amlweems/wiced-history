/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  MCU powersave implementation
 */

#include "platform_peripheral.h"
#include "platform_appscr4.h"
#include "platform_config.h"
#include "wwd_assert.h"
#include "wwd_rtos.h"
#include "cr4.h"

#include "typedefs.h"
#include "wiced_osl.h"
#include "sbchipc.h"

#include "wiced_deep_sleep.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define POWERSAVE_MOVING_AVERAGE_PARAM 128
#if PLATFORM_TICK_STATS
#define POWERSAVE_MOVING_AVERAGE_CALC( var, add ) \
    do \
    { \
        (var) *= POWERSAVE_MOVING_AVERAGE_PARAM - 1; \
        (var) += (add); \
        (var) /= POWERSAVE_MOVING_AVERAGE_PARAM; \
    } \
    while ( 0 )
#else
#define POWERSAVE_MOVING_AVERAGE_CALC( var, add )
#endif /* PLATFORM_TICK_STATS */

#define PMU_RES_WLAN_UP_EVENT_MASK            PMU_RES_MASK( PMU_RES_WLAN_UP_EVENT )

/* Values are based on chip RTL reading */
#define PMU_RES_INTSTATUS_WRITE_LATENCY_TICKS 4
#define PMU_RES_TIMER_WRITE_LATENCY_TICKS     4

#ifdef DEBUG
#define POWERSAVE_WLAN_EVENT_WAIT_MS          1000
#define POWERSAVE_WLAN_EVENT_ASSERT_SEC       1
#define POWERSAVE_WLAN_TIMER_ASSERT_SEC       1
#else
#define POWERSAVE_WLAN_EVENT_WAIT_MS          NEVER_TIMEOUT
#endif

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

static wiced_bool_t          powersave_is_warmboot              = WICED_FALSE;

#if PLATFORM_TICK_POWERSAVE

static int                   powersave_disable_counter          = PLATFORM_TICK_POWERSAVE_DISABLE ? 1 : 0;

#if PLATFORM_TICK_STATS
/* Statistic variables */
float                        powersave_stat_avg_sleep_ticks     = 0;
float                        powersave_stat_avg_run_ticks       = 0;
float                        powersave_stat_avg_load            = 0;
float                        powersave_stat_avg_ticks_req       = 0;
float                        powersave_stat_avg_ticks_adj       = 0;
uint32_t                     powersave_stat_call_number         = 0;
#endif /* PLATFORM_TICK_STATS */

#endif /* PLATFORM_TICK_POWERSAVE */

#if PLATFORM_WLAN_POWERSAVE
static int                   powersave_wlan_res_ref_counter     = 0;
static host_semaphore_type_t powersave_wlan_res_event;
static uint32_t              powersave_wlan_res_event_ack_stamp = 0;
static host_semaphore_type_t powersave_wlan_res_lock;
#if PLATFORM_WLAN_POWERSAVE_STATS
static uint32_t              powersave_wlan_res_call_counter    = 0;
static uint32_t              powersave_wlan_res_up_begin_stamp  = 0;
static uint32_t              powersave_wlan_res_up_time         = 0;
static uint32_t              powersave_wlan_res_wait_up_time    = 0;
#endif /* PLATFORM_WLAN_POWERSAVE_STATS */
#endif /* PLATFORM_WLAN_POWERSAVE */

/******************************************************
 *               Function Definitions
 ******************************************************/

inline static wiced_bool_t
platform_mcu_powersave_permission( void )
{
#if PLATFORM_TICK_POWERSAVE
    return ( powersave_disable_counter == 0 ) ? WICED_TRUE : WICED_FALSE;
#else
    return WICED_FALSE;
#endif
}

static void
platform_mcu_powersave_fire_event( void )
{
    const wiced_bool_t            powersave = platform_mcu_powersave_permission();
    const platform_tick_command_t command   = powersave ? PLATFORM_TICK_COMMAND_POWERSAVE_BEGIN : PLATFORM_TICK_COMMAND_POWERSAVE_END;

    platform_tick_execute_command( command );
}

static platform_result_t
platform_mcu_powersave_add_disable_counter( int add )
{
#if PLATFORM_TICK_POWERSAVE
    uint32_t flags;

    WICED_SAVE_INTERRUPTS( flags );

    powersave_disable_counter += add;
    wiced_assert("unbalanced powersave calls", powersave_disable_counter >= 0);

    platform_mcu_powersave_fire_event();

    WICED_RESTORE_INTERRUPTS( flags );

    return PLATFORM_SUCCESS;
#else
    return PLATFORM_FEATURE_DISABLED;
#endif /* PLATFORM_TICK_POWERSAVE */
}

platform_result_t
platform_mcu_powersave_init( void )
{
    platform_result_t res = PLATFORM_FEATURE_DISABLED;

#if PLATFORM_APPS_POWERSAVE
    /* Define resource mask used to wake up application domain. */
    PLATFORM_PMU->res_req_mask1 = PMU_RES_APPS_UP_MASK;

    /* Clear status fields which may affect warm-boot and which are cleared by power-on-reset only. */
    PLATFORM_APPSCR4->core_status.raw = PLATFORM_APPSCR4->core_status.raw;

    if ( !powersave_is_warmboot )
    {
        /* For deep sleep current measuring. */
        platform_pmu_regulatorcontrol( PMU_REGULATOR_LPLDO1_REG,
                                       PMU_REGULATOR_LPLDO1_MASK,
                                       PMU_REGULATOR_LPLDO1_1_0_V );

        /*
         * Spread over the time power-switch up-delays.
         * This should reduce inrush current which may break digital logic functionality.
         */
        platform_pmu_chipcontrol( PMU_CHIPCONTROL_APP_POWER_UP_DELAY_REG,
                                  PMU_CHIPCONTROL_APP_POWER_UP_DELAY_DIGITAL_MASK |
                                  PMU_CHIPCONTROL_APP_POWER_UP_DELAY_SOCSRAM_MASK |
                                  PMU_CHIPCONTROL_APP_POWER_UP_DELAY_VDDM_MASK,
                                  PMU_CHIPCONTROL_APP_POWER_UP_DELAY_DIGITAL_VAL(0xF) |
                                  PMU_CHIPCONTROL_APP_POWER_UP_DELAY_SOCSRAM_VAL(0x1) |
                                  PMU_CHIPCONTROL_APP_POWER_UP_DELAY_VDDM_VAL(0x1) );

        /* Increase time-up to accomodate above change which spreads power-switch up-delays. */
        platform_pmu_res_updown_time( PMU_RES_APP_DIGITAL_PWRSW,
                                      PMU_RES_UPDOWN_TIME_UP_MASK,
                                      PMU_RES_UPDOWN_TIME_UP_VAL(38) );

        /* Increase VDDM pwrsw up time and make digital pwrsw to depend on VDDM pwrsw to reduce inrush current */
        platform_pmu_res_updown_time( PMU_RES_APP_VDDM_PWRSW,
                                      PMU_RES_UPDOWN_TIME_UP_MASK,
                                      PMU_RES_UPDOWN_TIME_UP_VAL(12) );
        platform_pmu_res_dep_mask( PMU_RES_APP_DIGITAL_PWRSW,
                                   PMU_RES_MASK(PMU_RES_APP_VDDM_PWRSW),
                                   PMU_RES_MASK(PMU_RES_APP_VDDM_PWRSW) );

        /* Force app always-on memory on. */
        platform_pmu_chipcontrol( PMU_CHIPCONTROL_APP_VDDM_POWER_FORCE_REG,
                                  PMU_CHIPCONTROL_APP_VDDM_POWER_FORCE_MASK,
                                  PMU_CHIPCONTROL_APP_VDDM_POWER_FORCE_EN );

        /*
         * Set deep-sleep flag.
         * It is reserved for software.
         * Does not trigger any hardware reaction.
         * Used by software during warm boot to know whether it should go normal boot path or warm boot.
         */
        platform_gci_chipcontrol( GCI_CHIPCONTROL_SW_DEEP_SLEEP_FLAG_REG,
                                  GCI_CHIPCONTROL_SW_DEEP_SLEEP_FLAG_MASK,
                                  GCI_CHIPCONTROL_SW_DEEP_SLEEP_FLAG_SET );
    }

    res = PLATFORM_SUCCESS;
#endif /* PLATFORM_APPS_POWERSAVE */

#if PLATFORM_WLAN_POWERSAVE
    host_rtos_init_semaphore( &powersave_wlan_res_lock );
    host_rtos_set_semaphore ( &powersave_wlan_res_lock, WICED_FALSE );

    host_rtos_init_semaphore( &powersave_wlan_res_event );

    if ( !powersave_is_warmboot )
    {
        /* When wait WLAN up this is resources we are waiting for and for which need interrupt to be generated. */
        PLATFORM_PMU->res_event1 = PMU_RES_WLAN_UP_EVENT_MASK;

        /*
         * Resource requesting via writing to res_req_timer1 has latency.
         * WLAN due to this latency has few ILP clocks to go down after the APPS write to res_req_timer1 and check that PMU_RES_WL_CORE_READY_BUF is up.
         * PMU_RES_WL_CORE_READY_BUF can go down, this is fine, but PMU_RES_WL_CORE_READY must not.
         * To avoid this let's make sure PMU_RES_WL_CORE_READY_BUF down time is large enough to compensate latency.
         */
        platform_pmu_res_updown_time( PMU_RES_WL_CORE_READY_BUF,
                                      PMU_RES_UPDOWN_TIME_DOWN_MASK,
                                      MAX( platform_pmu_res_updown_time( PMU_RES_WL_CORE_READY_BUF, 0, 0) & PMU_RES_UPDOWN_TIME_DOWN_MASK,
                                           PMU_RES_UPDOWN_TIME_DOWN_VAL( PMU_RES_TIMER_WRITE_LATENCY_TICKS ) ) );
    }
#endif /* PLATFORM_WLAN_POWERSAVE */

    platform_mcu_powersave_fire_event();

    return res;
}

void
platform_mcu_powersave_set_mode( platform_mcu_powersave_mode_t mode )
{
#if PLATFORM_APPS_POWERSAVE
    uint32_t mask = PLATFORM_PMU->max_res_mask;

    switch ( mode )
    {
        case MCU_POWERSAVE_DEEP_SLEEP:
            mask = PMU_RES_DEEP_SLEEP_MASK;
            break;

        case MCU_POWERSAVE_SLEEP:
            mask = PMU_RES_SLEEP_MASK;
            break;

        default:
            wiced_assert( "Bad mode", 0 );
            break;
    }

    PLATFORM_PMU->min_res_mask = mask;
#else
    UNUSED_PARAMETER( mode );
#endif /* PLATFORM_APPS_POWERSAVE */
}

void
platform_mcu_powersave_warmboot_init( void )
{
    appscr4_core_status_reg_t status = { .raw =  PLATFORM_APPSCR4->core_status.raw };

    powersave_is_warmboot = WICED_FALSE;

    if ( status.bits.s_error_log || status.bits.s_bp_reset_log )
    {
        /* Board was resetted. */
    }
    else if ( ( platform_gci_chipcontrol( GCI_CHIPCONTROL_SW_DEEP_SLEEP_FLAG_REG,
                                          0x0,
                                          0x0) & GCI_CHIPCONTROL_SW_DEEP_SLEEP_FLAG_MASK ) != GCI_CHIPCONTROL_SW_DEEP_SLEEP_FLAG_SET )
    {
        /* Previously booted software does not want us to run warm reboot sequence. */
    }
    else
    {
        powersave_is_warmboot = WICED_TRUE;
    }
}

wiced_bool_t
platform_mcu_powersave_is_warmboot( void )
{
    return powersave_is_warmboot;
}

platform_result_t
platform_mcu_powersave_disable( void )
{
    return platform_mcu_powersave_add_disable_counter( 1 );
}

platform_result_t
platform_mcu_powersave_enable( void )
{
    return platform_mcu_powersave_add_disable_counter( -1 );
}

/******************************************************
 *               RTOS Powersave Hooks
 ******************************************************/

#if PLATFORM_TICK_POWERSAVE

inline static uint32_t
platform_mcu_appscr4_stamp( void )
{
    return platform_tick_get_time( PLATFORM_TICK_GET_FAST_TIME_STAMP );
}

inline static uint32_t
platform_mcu_pmu_stamp( void )
{
    uint32_t time = platform_tick_get_time( PLATFORM_TICK_GET_SLOW_TIME_STAMP );
    return (uint64_t)CPU_CLOCK_HZ * time / platform_reference_clock_get_freq( PLATFORM_REFERENCE_CLOCK_ILP );
}

static void
platform_mcu_suspend( void )
{
#if PLATFORM_APPS_POWERSAVE
    wiced_bool_t permission = platform_mcu_powersave_permission();
#endif

    WICED_DEEP_SLEEP_CALL_EVENT_HANDLERS( permission, WICED_DEEP_SLEEP_EVENT_ENTER );

#if PLATFORM_TICK_STATS

    static uint32_t begin_stamp_appscr4, end_stamp_appscr4;
#if PLATFORM_TICK_PMU
    uint32_t begin_stamp_pmu = platform_mcu_pmu_stamp();
#endif
    uint32_t sleep_ticks;
    float total_ticks;

    begin_stamp_appscr4 = platform_mcu_appscr4_stamp();

    if ( end_stamp_appscr4 != 0 )
    {
        POWERSAVE_MOVING_AVERAGE_CALC( powersave_stat_avg_run_ticks, begin_stamp_appscr4 - end_stamp_appscr4 );
    }

    cpu_wait_for_interrupt();

    end_stamp_appscr4 = platform_mcu_appscr4_stamp();

#if PLATFORM_TICK_PMU
    sleep_ticks = platform_mcu_pmu_stamp() - begin_stamp_pmu;
#else
    sleep_ticks = end_stamp_appscr4 - begin_stamp_appscr4;
#endif
    POWERSAVE_MOVING_AVERAGE_CALC( powersave_stat_avg_sleep_ticks, sleep_ticks );

    total_ticks             = powersave_stat_avg_run_ticks + powersave_stat_avg_sleep_ticks;
    powersave_stat_avg_load = ( total_ticks > 0.0 ) ? ( powersave_stat_avg_run_ticks / total_ticks ) : 0.0;

    powersave_stat_call_number++;

#else

    cpu_wait_for_interrupt();

#endif /* PLATFORM_TICK_STATS */

    WICED_DEEP_SLEEP_CALL_EVENT_HANDLERS( permission, WICED_DEEP_SLEEP_EVENT_CANCEL );
}

#endif /* PLATFORM_TICK_POWERSAVE */

/* Expect to be called with interrupts disabled */
void platform_idle_hook( void )
{
#if PLATFORM_TICK_POWERSAVE
    (void)platform_tick_sleep( platform_mcu_suspend, 0, WICED_TRUE );
#endif
}

/* Expect to be called with interrupts disabled */
uint32_t platform_power_down_hook( uint32_t ticks )
{
#if PLATFORM_TICK_POWERSAVE
    uint32_t ret = 0;

    if ( platform_mcu_powersave_permission() )
    {
        ret = platform_tick_sleep( platform_mcu_suspend, ticks, WICED_TRUE );

        POWERSAVE_MOVING_AVERAGE_CALC( powersave_stat_avg_ticks_req, ticks );
        POWERSAVE_MOVING_AVERAGE_CALC( powersave_stat_avg_ticks_adj, ret );
    }

    return ret;
#else
    return 0;
#endif /* PLATFORM_TICK_POWERSAVE */
}

/* Expect to be called with interrupts disabled */
int platform_power_down_permission( void )
{
#if PLATFORM_TICK_POWERSAVE
    wiced_bool_t permission = platform_mcu_powersave_permission();

    if ( !permission )
    {
        (void)platform_tick_sleep( platform_mcu_suspend, 0, WICED_FALSE );
    }

    return permission;
#else
    return 0;
#endif /* PLATFORM_TICK_POWERSAVE */
}

#if PLATFORM_WLAN_POWERSAVE

static wiced_bool_t
platform_wlan_powersave_pmu_timer_slow_write_pending( void )
{
    return ( PLATFORM_PMU->pmustatus & PST_SLOW_WR_PENDING ) ? WICED_TRUE : WICED_FALSE;
}

static void platform_wlan_powersave_pmu_timer_slow_write( uint32_t val )
{
    PLATFORM_TIMEOUT_BEGIN( start_stamp );

    while ( platform_wlan_powersave_pmu_timer_slow_write_pending() )
    {
        PLATFORM_TIMEOUT_SEC_ASSERT( "wait status before write for too long", start_stamp,
            !platform_wlan_powersave_pmu_timer_slow_write_pending(), POWERSAVE_WLAN_TIMER_ASSERT_SEC );
    }

    PLATFORM_PMU->res_req_timer1.raw = val;

    while ( platform_wlan_powersave_pmu_timer_slow_write_pending() )
    {
        PLATFORM_TIMEOUT_SEC_ASSERT( "wait status after write for too long", start_stamp,
            !platform_wlan_powersave_pmu_timer_slow_write_pending(), POWERSAVE_WLAN_TIMER_ASSERT_SEC );
    }
}

static void platform_wlan_powersave_res_ack_event( void )
{
    pmu_intstatus_t status;

    status.raw                  = 0;
    status.bits.rsrc_event_int1 = 1;

    PLATFORM_PMU->pmuintstatus.raw = status.raw;

    powersave_wlan_res_event_ack_stamp = platform_tick_get_time( PLATFORM_TICK_GET_SLOW_TIME_STAMP );
}

static wiced_bool_t platform_wlan_powersave_res_is_event_unmask_permitted( void )
{
    /*
     * ISR acks interrupt, but it take few ILP cycles to do it.
     * As result if resource mask is changing quickly we may ack and lost some
     * of the subsequent interrupts and hang by waiting on semaphore.
     * To avoid this let's do not try to enable interrupts till ack is settled.
     */
    uint32_t now = platform_tick_get_time( PLATFORM_TICK_GET_SLOW_TIME_STAMP );
    if ( powersave_wlan_res_event_ack_stamp == 0 )
    {
        powersave_wlan_res_event_ack_stamp = now;
        return WICED_FALSE;
    }
    else if ( now - powersave_wlan_res_event_ack_stamp < (uint32_t)PMU_RES_INTSTATUS_WRITE_LATENCY_TICKS )
    {
        return WICED_FALSE;
    }

    return WICED_TRUE;
}

static wiced_bool_t platform_wlan_powersave_res_mask_event( wiced_bool_t enable )
{
    pmu_intstatus_t mask;

    if ( enable && !platform_wlan_powersave_res_is_event_unmask_permitted() )
    {
        return WICED_FALSE;
    }

    mask.raw                  = 0;
    mask.bits.rsrc_event_int1 = 1;

    if ( enable )
    {
        platform_common_chipcontrol( &PLATFORM_PMU->pmuintmask1.raw, 0x0, mask.raw );
    }
    else
    {
        platform_common_chipcontrol( &PLATFORM_PMU->pmuintmask1.raw, mask.raw, 0x0 );
    }

    return WICED_TRUE;
}

/*
 * Function return synchronized state of 2 registers.
 * As resources can go down let's remove pending one from current resource mask.
 */
static uint32_t platform_wlan_powersave_res_get_current_resources( uint32 mask )
{
    while ( WICED_TRUE )
    {
        uint32_t res_state   = PLATFORM_PMU->res_state & mask;
        uint32_t res_pending = PLATFORM_PMU->res_pending & mask;

        if ( res_state != ( PLATFORM_PMU->res_state & mask ) )
        {
            continue;
        }

        if ( res_pending != ( PLATFORM_PMU->res_pending & mask ) )
        {
            continue;
        }

        return ( res_state & ~res_pending );
    }
}

static void platform_wlan_powersave_res_wait_event( void )
{
    PLATFORM_TIMEOUT_BEGIN( start_stamp );

    while ( WICED_TRUE )
    {
        uint32_t mask = platform_wlan_powersave_res_get_current_resources( PMU_RES_WLAN_UP_MASK );

        if ( mask == PMU_RES_WLAN_UP_MASK )
        {
            /* Done */
            break;
        }

        PLATFORM_TIMEOUT_SEC_ASSERT( "wait res for too long", start_stamp,
            platform_wlan_powersave_res_get_current_resources( PMU_RES_WLAN_UP_MASK ) == PMU_RES_WLAN_UP_MASK, POWERSAVE_WLAN_EVENT_ASSERT_SEC );

        if ( ( mask | PMU_RES_WLAN_UP_EVENT_MASK ) == PMU_RES_WLAN_UP_MASK )
        {
            /* Nearly here, switch to polling */
            continue;
        }

        if ( platform_wlan_powersave_res_mask_event( WICED_TRUE ) )
        {
            if ( host_rtos_get_semaphore( &powersave_wlan_res_event, POWERSAVE_WLAN_EVENT_WAIT_MS, WICED_TRUE ) != WWD_SUCCESS )
            {
                wiced_assert( "powersave event timed out", 0 );
            }
        }
    }
}

void platform_wlan_powersave_res_event( void )
{
    platform_wlan_powersave_res_ack_event();

    platform_wlan_powersave_res_mask_event( WICED_FALSE );

    host_rtos_set_semaphore( &powersave_wlan_res_event, WICED_TRUE );
}

wiced_bool_t platform_wlan_powersave_res_up( void )
{
    wiced_bool_t res_up = WICED_FALSE;

    host_rtos_get_semaphore( &powersave_wlan_res_lock, NEVER_TIMEOUT, WICED_FALSE );

    powersave_wlan_res_ref_counter++;

    wiced_assert( "wrapped around zero", powersave_wlan_res_ref_counter > 0 );

    if ( powersave_wlan_res_ref_counter == 1 )
    {
        res_up = WICED_TRUE;
    }

    if ( res_up )
    {
        pmu_res_req_timer_t timer;

        timer.raw                   = 0;
        timer.bits.req_active       = 1;
        timer.bits.force_ht_request = 1;
        timer.bits.clkreq_group_sel = pmu_res_clkreq_apps_group;

#if PLATFORM_WLAN_POWERSAVE_STATS
        uint32_t wait_up_begin_stamp      = platform_tick_get_time( PLATFORM_TICK_GET_SLOW_TIME_STAMP );
        powersave_wlan_res_up_begin_stamp = wait_up_begin_stamp;
        powersave_wlan_res_call_counter++;
#endif /* PLATFORM_WLAN_POWERSAVE_STATS */

        platform_tick_execute_command( PLATFORM_TICK_COMMAND_RELEASE_PMU_TIMER_BEGIN );

        PLATFORM_PMU->res_req_mask1 = PMU_RES_WLAN_UP_MASK;

        platform_wlan_powersave_pmu_timer_slow_write( timer.raw );

        platform_wlan_powersave_res_wait_event();

#if PLATFORM_WLAN_POWERSAVE_STATS
        powersave_wlan_res_wait_up_time += platform_tick_get_time( PLATFORM_TICK_GET_SLOW_TIME_STAMP ) - wait_up_begin_stamp;
#endif
    }

    host_rtos_set_semaphore( &powersave_wlan_res_lock, WICED_FALSE );

    return res_up;
}

wiced_bool_t platform_wlan_powersave_res_down( wiced_bool_t(*check_ready)(void), wiced_bool_t force )
{
    wiced_bool_t res_down = WICED_FALSE;

    host_rtos_get_semaphore( &powersave_wlan_res_lock, NEVER_TIMEOUT, WICED_FALSE );

    if ( !force )
    {
        wiced_assert( "unbalanced call", powersave_wlan_res_ref_counter > 0 );
        powersave_wlan_res_ref_counter--;
        if ( powersave_wlan_res_ref_counter == 0 )
        {
            res_down = WICED_TRUE;
        }
    }
    else if ( powersave_wlan_res_ref_counter != 0 )
    {
        powersave_wlan_res_ref_counter = 0;
        res_down                       = WICED_TRUE;
    }

    if ( res_down )
    {
        if ( check_ready )
        {
            while ( !check_ready() );
        }

        platform_wlan_powersave_pmu_timer_slow_write( 0x0 );

        PLATFORM_PMU->res_req_mask1 = PMU_RES_APPS_UP_MASK;

        platform_tick_execute_command( PLATFORM_TICK_COMMAND_RELEASE_PMU_TIMER_END );

#if PLATFORM_WLAN_POWERSAVE_STATS
        powersave_wlan_res_up_time += platform_tick_get_time( PLATFORM_TICK_GET_SLOW_TIME_STAMP ) - powersave_wlan_res_up_begin_stamp;
#endif
    }

    host_rtos_set_semaphore( &powersave_wlan_res_lock, WICED_FALSE );

    return res_down;
}

uint32_t platform_wlan_powersave_get_stats( platform_wlan_powersave_stats_t which_counter )
{
#if PLATFORM_WLAN_POWERSAVE_STATS
    switch ( which_counter )
    {
       case PLATFORM_WLAN_POWERSAVE_STATS_CALL_NUM:
           return powersave_wlan_res_call_counter;

       case PLATFORM_WLAN_POWERSAVE_STATS_UP_TIME:
           return powersave_wlan_res_up_time;

       case PLATFORM_WLAN_POWERSAVE_STATS_WAIT_UP_TIME:
           return powersave_wlan_res_wait_up_time;

       default:
           wiced_assert( "unhandled case", 0 );
           return 0;
    }
#else
    UNUSED_PARAMETER( which_counter );
    return 0;
#endif /* PLATFORM_WLAN_POWERSAVE_STATS */
}

#endif /* PLATFORM_WLAN_POWERSAVE */
