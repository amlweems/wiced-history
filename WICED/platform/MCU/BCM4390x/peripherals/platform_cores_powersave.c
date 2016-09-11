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
 *  Platform cores power related code
 */
#include "platform_mcu_peripheral.h"
#include "platform_appscr4.h"
#include "platform_map.h"

#include "typedefs.h"
#include "sbchipc.h"
#include "aidmp.h"
#include "hndsoc.h"

#include "wiced_osl.h"
#include "wiced_deep_sleep.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define PLATFORM_CORES_POWERSAVE_WLAN_RESOURCES          \
    ( PMU_RES_MASK( PMU_RES_SR_CLK_START )             | \
      PMU_RES_MASK( PMU_RES_WL_CORE_READY )            | \
      PMU_RES_MASK( PMU_RES_WL_CORE_READY_BUF )        | \
      PMU_RES_MASK( PMU_RES_MINI_PMU )                 | \
      PMU_RES_MASK( PMU_RES_RADIO_PU )                 | \
      PMU_RES_MASK( PMU_RES_SR_CLK_STABLE )            | \
      PMU_RES_MASK( PMU_RES_SR_SAVE_RESTORE )          | \
      PMU_RES_MASK( PMU_RES_SR_VDDM_PWRSW )            | \
      PMU_RES_MASK( PMU_RES_SR_SUBCORE_AND_PHY_PWRSW ) | \
      PMU_RES_MASK( PMU_RES_SR_SLEEP )                 | \
      PMU_RES_MASK( PMU_RES_MAC_PHY_CLK_AVAIL ) )


/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct
{
    uint32_t    core_addr;
    uint32_t    wrapper_addr;
} platform_cores_powersave_clock_gate_core_t;

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

#if PLATFORM_CORES_POWERSAVE

static void
platform_cores_powersave_clock_gate_core( platform_cores_powersave_clock_gate_core_t* core )
{
    /*
     * The clock gating only works when the core is out of reset.
     * But when the core is out of reset it always requests HT.
     * Let's bring core out of reset (so that clock gating works and the
     * clock tree within the core is not running) but also write to ClockControlStatus
     * to force off the HT request.
     * This reduce power consumption as long as core is not used.
     * When driver is going to use core it resets core and cancel changes done here.
     */

    volatile aidmp_t* dmp                        = (aidmp_t*)core->wrapper_addr;
    volatile clock_control_status_t* core_status = PLATFORM_CLOCKSTATUS_REG( core->core_addr );
    uint32_t dummy;

    UNUSED_VARIABLE( dummy );

    dmp->ioctrl    = SICF_FGC | SICF_CLOCK_EN; /* turn on clocks */
    dmp->resetctrl = AIRC_RESET;               /* turn on reset */
    dummy = dmp->resetctrl;                    /* read back to ensure write propagated */
    OSL_DELAY( 1 );

    dmp->resetctrl = 0;                        /* turn off reset */
    dummy = dmp->resetctrl;                    /* read back to ensure write propagated */
    OSL_DELAY( 1 );

    core_status->bits.force_hw_clock_req_off = 1; /* assert ForceHWClockReqOff */
    dummy = core_status->raw;                     /* read back to ensure write propagated */
    OSL_DELAY( 1 );

    dmp->ioctrl = SICF_CLOCK_EN; /* turn off force clock */
    dmp->ioctrl = 0;             /* turn off clock */
}

#ifdef WICED_NO_WIFI

static void
platform_cores_powersave_down_core( uint32_t wrapper_addr )
{
    volatile aidmp_t* dmp = (aidmp_t*)wrapper_addr;
    uint32_t dummy;

    UNUSED_VARIABLE( dummy );

    dmp->resetctrl = AIRC_RESET; /* turn on reset */
    dmp->ioctrl    = 0;          /* turn off clocks */

    dummy = dmp->resetctrl;      /* read back to ensure write propagated */
    OSL_DELAY( 1 );
}

static void
platform_cores_powersave_down_all_wlan_cores( void )
{
    uint32_t wlan_domain_wrappers[] =
    {
        PLATFORM_WLANCR4_MASTER_WRAPPER_REGBASE(0x0),
        PLATFORM_DOT11MAC_MASTER_WRAPPER_REGBASE(0x0)
    };
    unsigned i;

    for ( i = 0; i < ARRAYSIZE( wlan_domain_wrappers ); ++i )
    {
        platform_cores_powersave_down_core( wlan_domain_wrappers[i] );
    }
}

static void
platform_cores_powersave_init_wlan_domain( void )
{
    /* No WiFi needed. Let's down whole domain. */

    if ( !WICED_DEEP_SLEEP_IS_WARMBOOT( ) )
    {
        platform_cores_powersave_down_all_wlan_cores( );
    }

    PLATFORM_PMU->min_res_mask &= ~PLATFORM_CORES_POWERSAVE_WLAN_RESOURCES;
    PLATFORM_PMU->max_res_mask &= ~PLATFORM_CORES_POWERSAVE_WLAN_RESOURCES;
}

#else

static void
platform_cores_powersave_init_wlan_domain( void )
{
}

#endif /* WICED_NO_WIFI */

static void
platform_cores_powersave_init_apps_domain( void )
{
    platform_cores_powersave_clock_gate_core_t cores[] =
    {
        { .core_addr = PLATFORM_GMAC_REGBASE(0x0), .wrapper_addr = PLATFORM_GMAC_MASTER_WRAPPER_REGBASE(0x0) },
        { .core_addr = PLATFORM_I2S0_REGBASE(0x0), .wrapper_addr = PLATFORM_I2S0_MASTER_WRAPPER_REGBASE(0x0) },
        { .core_addr = PLATFORM_I2S1_REGBASE(0x0), .wrapper_addr = PLATFORM_I2S1_MASTER_WRAPPER_REGBASE(0x0) }
    };
    unsigned i;

    for ( i = 0; i < ARRAYSIZE( cores ); ++i )
    {
        platform_cores_powersave_clock_gate_core( &cores[i] );
    }
}

void platform_cores_powersave_init( void )
{
    platform_cores_powersave_init_apps_domain( );
    platform_cores_powersave_init_wlan_domain( );
}

#else

void platform_cores_powersave_init( void )
{
}

#endif /* PLATFORM_CORES_POWERSAVE */
