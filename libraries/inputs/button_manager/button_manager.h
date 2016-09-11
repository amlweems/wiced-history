/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "wiced_rtos.h"
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    BUTTON_CLICK_EVENT                 = (1 << 0), //!> A click is a combination of press and release button events. Typically ~ < 200 ms.
    BUTTON_SHORT_DURATION_EVENT        = (1 << 1),
    BUTTON_MEDIUM_DURATION_EVENT       = (1 << 2),
    BUTTON_LONG_DURATION_EVENT         = (1 << 3),
    BUTTON_VERY_LONG_DURATION_EVENT    = (1 << 4),
    BUTTON_DOUBLE_CLICK_EVENT          = (1 << 5), //!> A double click is a combination of two single clicks with some delay between them
} button_manager_event_t;

typedef enum
{
    BUTTON_STATE_HELD      = (0),
    BUTTON_STATE_RELEASED  = (1),
} button_manager_button_state_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    platform_button_t button;
    uint16_t          button_event_mask;
    uint32_t          application_event;
} wiced_button_configuration_t;

typedef struct
{
    const wiced_button_configuration_t*  configuration;
    button_manager_button_state_t        current_state;
    button_manager_event_t               last_sent_event;
    uint32_t                             pressed_timestamp;
    uint32_t                             released_timestamp;
    uint32_t                             last_released_timestamp;
    wiced_bool_t                         check_for_double_click;
} button_manager_button_t;

typedef void ( *wiced_button_event_handler_t )( const button_manager_button_t* button, button_manager_event_t event, button_manager_button_state_t state );

typedef struct
{
    uint16_t                      short_hold_duration;
    uint16_t                      medium_hold_duration;
    uint16_t                      long_hold_duration;
    uint16_t                      very_long_hold_duration;
    uint16_t                      debounce_duration;
    uint16_t                      double_click_interval; //!> Time interval between two RELEASE events
    wiced_button_event_handler_t  event_handler;
} wiced_button_manager_configuration_t;

typedef struct
{
    const wiced_button_manager_configuration_t* configuration;
    button_manager_button_t*                    buttons;
    uint32_t                                    number_of_buttons;
    wiced_button_event_handler_t                button_callback;
    wiced_worker_thread_t*                      worker_thread;
    wiced_timer_t                               timer;
    wiced_time_t                                timer_timestamp;
} button_manager_t;

/**
 * Application's Button event handler
 *
 * @param button    Which button in the list has been pressed/released/held
 * @param event     What exact state the button is in.
 *
 * @return void     Library should not care whether app handled the button-events correctly or not.
 */


typedef void ( *button_state_change_callback_t )( platform_button_t id, wiced_bool_t new_state );

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t button_manager_init( button_manager_t* manager, const wiced_button_manager_configuration_t* configuration, wiced_worker_thread_t* thread, button_manager_button_t* buttons, uint32_t number_of_buttons );
wiced_result_t button_manager_deinit( button_manager_t* manager );

#ifdef __cplusplus
} /* extern "C" */
#endif
