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
    uint32_t        core_addr;
    uint32_t        wrapper_addr;
    wiced_bool_t    is_enabled;
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
platform_cores_powersave_shutoff_usb20d( void )
{
    /*
     * Shutoff USB PHY using USB20D core.
     * Need to do it once only during cold boot.
     */

    if ( WICED_DEEP_SLEEP_IS_WARMBOOT( ) || !PLATFORM_CAPABILITY_ENAB( PLATFORM_CAPS_USB  ) )
    {
        return;
    }

    platform_gci_chipcontrol( GCI_CHIPCONTROL_USBPHY_MODE_OVR_VAL_REG, GCI_CHIPCONTROL_USBPHY_MODE_OVR_VAL_MASK, GCI_CHIPCONTROL_USBPHY_MODE_OVR_VAL_USB );

    platform_gci_chipcontrol( GCI_CHIPCONTROL_USBPHY_MODE_OVR_EN_REG, GCI_CHIPCONTROL_USBPHY_MODE_OVR_EN_MASK, GCI_CHIPCONTROL_USBPHY_MODE_OVR_EN_SET );

    osl_wrapper_enable( (void*)PLATFORM_USB20D_MASTER_WRAPPER_REGBASE(0x0) );

    platform_common_chipcontrol( (void*)PLATFORM_USB20D_PHY_UTMI_CTL1_REG, PLATFORM_USB20D_PHY_UTMI1_CTL_PHY_SHUTOFF_MASK, PLATFORM_USB20D_PHY_UTMI1_CTL_PHY_SHUTOFF_DISABLE );
    OSL_DELAY(50);
    platform_common_chipcontrol( (void*)PLATFORM_USB20D_PHY_UTMI_CTL1_REG, PLATFORM_USB20D_PHY_UTMI1_CTL_PHY_SHUTOFF_MASK, PLATFORM_USB20D_PHY_UTMI1_CTL_PHY_SHUTOFF_ENABLE );

    osl_wrapper_disable( (void*)PLATFORM_USB20D_MASTER_WRAPPER_REGBASE(0x0) );

    platform_gci_chipcontrol( GCI_CHIPCONTROL_USBPHY_MODE_OVR_EN_REG, GCI_CHIPCONTROL_USBPHY_MODE_OVR_EN_MASK, 0x0 );
}

static void
platform_cores_powersave_enable_memory_clock_gating( void )
{
    PLATFORM_SOCSRAM_POWERCONTROL_REG->bits.enable_mem_clk_gate = 1;
}

static void
platform_cores_powersave_init_apps_domain( void )
{
    platform_cores_powersave_clock_gate_core_t cores[] =
    {
        { .core_addr = PLATFORM_GMAC_REGBASE(0x0), .wrapper_addr = PLATFORM_GMAC_MASTER_WRAPPER_REGBASE(0x0), PLATFORM_CAPABILITY_ENAB(PLATFORM_CAPS_GMAC) },
        { .core_addr = PLATFORM_I2S0_REGBASE(0x0), .wrapper_addr = PLATFORM_I2S0_MASTER_WRAPPER_REGBASE(0x0), PLATFORM_CAPABILITY_ENAB(PLATFORM_CAPS_I2S) },
        { .core_addr = PLATFORM_I2S1_REGBASE(0x0), .wrapper_addr = PLATFORM_I2S1_MASTER_WRAPPER_REGBASE(0x0), PLATFORM_CAPABILITY_ENAB(PLATFORM_CAPS_I2S) }
    };
    unsigned i;

    for ( i = 0; i < ARRAYSIZE( cores ); ++i )
    {
        if ( cores[i].is_enabled == WICED_TRUE )
        {
            platform_cores_powersave_clock_gate_core( &cores[i] );
        }
    }

    platform_cores_powersave_shutoff_usb20d( );

    platform_cores_powersave_enable_memory_clock_gating( );
}

static void
platform_cores_powersave_disable_wl_reg_on_pulldown( wiced_bool_t disable )
{
    platform_pmu_regulatorcontrol( PMU_REGULATOR_WL_REG_ON_PULLDOWN_REG, PMU_REGULATOR_WL_REG_ON_PULLDOWN_MASK,
                                   disable ? PMU_REGULATOR_WL_REG_ON_PULLDOWN_DIS : PMU_REGULATOR_WL_REG_ON_PULLDOWN_EN );
}

void
platform_cores_powersave_init( void )
{
    platform_cores_powersave_disable_wl_reg_on_pulldown( WICED_TRUE );
    platform_cores_powersave_init_apps_domain( );
    platform_cores_powersave_init_wlan_domain( );
}

WICED_DEEP_SLEEP_EVENT_HANDLER( deep_sleep_cores_powersave_event_handler )
{
    if ( event == WICED_DEEP_SLEEP_EVENT_ENTER )
    {
        platform_cores_powersave_disable_wl_reg_on_pulldown( WICED_FALSE );
    }
    else if ( event == WICED_DEEP_SLEEP_EVENT_CANCEL )
    {
        platform_cores_powersave_disable_wl_reg_on_pulldown( WICED_TRUE );
    }
}

#else

void
platform_cores_powersave_init( void )
{
}

#endif /* PLATFORM_CORES_POWERSAVE */
