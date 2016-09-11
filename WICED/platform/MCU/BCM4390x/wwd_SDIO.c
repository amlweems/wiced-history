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
 * Defines WWD SDIO functions for BCM4390x SDIOH
 */
#include <typedefs.h>
#include <osl.h>

#include "platform/wwd_platform_interface.h"
#include "platform/wwd_bus_interface.h"
#include "platform_sdio.h"

/******************************************************
 *             Constants
 ******************************************************/
#ifndef WICED_DISABLE_MCU_POWERSAVE
#error "Not support WICED_DISABLE_MCU_POWERSAVE"
#endif /* ifndef  WICED_DISABLE_MCU_POWERSAVE */

/******************************************************
 *             Structures
 ******************************************************/

/******************************************************
 *             Variables
 ******************************************************/
extern sdioh_info_t *glob_sd;

/******************************************************
 *             Function declarations
 ******************************************************/
void sdio_client_check_isr( void* );

/******************************************************
 *             Function definitions
 ******************************************************/
void host_platform_power_wifi( wiced_bool_t power_enabled )
{
    UNUSED_PARAMETER( power_enabled );
}

wwd_result_t host_platform_bus_init( void )
{
    if ( platform_sdio_host_init( sdio_client_check_isr ) != WICED_SUCCESS )
    {
        WPRINT_WWD_ERROR(("SDIO Host init FAIL\n"));
        return WWD_SDIO_BUS_UP_FAIL;
    }
    else
    {
        return WWD_SUCCESS;
    }
}

wwd_result_t host_platform_sdio_enumerate( void )
{
    /* Select card already done in sdioh_attach */
    return WWD_SUCCESS;
}

wwd_result_t host_platform_bus_deinit( void )
{
    return WWD_SUCCESS;
}

wwd_result_t host_platform_sdio_transfer( wwd_bus_transfer_direction_t direction, sdio_command_t command, sdio_transfer_mode_t mode, sdio_block_size_t block_size, uint32_t argument, /*@null@*/ uint32_t* data, uint16_t data_size, sdio_response_needed_t response_expected, /*@out@*/ /*@null@*/ uint32_t* response )
{
    wiced_result_t wiced_result;

    wiced_result = platform_sdio_host_transfer( direction == BUS_READ ? SDIO_READ : SDIO_WRITE,
                                                command, mode, block_size, argument, data, data_size, response_expected, response );

    return (wiced_result == WICED_SUCCESS ? WWD_SUCCESS : WWD_WLAN_SDIO_ERROR);
}

void host_platform_enable_high_speed_sdio( void )
{
    platform_sdio_host_enable_high_speed();
}

wwd_result_t host_platform_bus_enable_interrupt( void )
{
    platform_sdio_host_enable_interrupt();
    return  WWD_SUCCESS;
}

wwd_result_t host_platform_bus_disable_interrupt( void )
{
    platform_sdio_host_disable_interrupt();
    return  WWD_SUCCESS;
}

#ifdef WICED_PLATFORM_MASKS_BUS_IRQ
wwd_result_t host_platform_unmask_sdio_interrupt( void )
{
    return host_platform_bus_enable_interrupt();
}
#endif

void host_platform_bus_buffer_freed( wwd_buffer_dir_t direction )
{
    UNUSED_PARAMETER( direction );
}

/* Client Interrupt handler */
void sdio_client_check_isr( void* user_data )
{
    UNUSED_PARAMETER( user_data );
    host_platform_bus_disable_interrupt();
    wwd_thread_notify_irq();
}

