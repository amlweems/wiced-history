/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file Apollo Display Library Header
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef AUDIO_DISPLAY_H

#include "u8g_arm.h"
#include "power_management.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define AUDIO_DISPLAY_H

/* Header options - Battery, Bluetooth, RSSI */
#define BATTERY_ICON_IS_VISIBLE        0x01
#define BATTERY_ICON_SHOW_PERCENT      0x02
#define INVERT_BATTERY_CHARGING_COLORS 0x04
#define BATTERY_ICON_IS_CHARGING       0x08 /* The update function automatically sets/clears this flag for internal use */
#define BLUETOOTH_IS_CONNECTED         0x10
#define INVERT_BLUETOOTH_ICON_COLORS   0x20
#define SIGNAL_STRENGTH_IS_VISIBLE     0x40
#define HEADER_BOTTOM_BAR_IS_VISIBLE   0x80

/* Footer options - Song information appearance */
#define FOOTER_IS_VISIBLE              0x01
#define FOOTER_CENTER_ALIGN            0x02
#define FOOTER_TOP_BAR_IS_VISIBLE      0x04
#define FOOTER_HIDE_SONG_DURATION      0x08
#define FOOTER_OPTION_APOLLO_TX        0x10
#define FOOTER_OPTION_APOLLO_RX        0x20


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

/* Creates a thread to handle the drawing and state management of the display.
 *
 * The thread keeps the battery updated, but the battery options can still be changed by the user.
 * This thread also updates signal strength while allowing the user to change the options.
 * Battery is updated twice per second; signal strength is updated once every three seconds.
 *
 * All other icons must be updated by the user using the audio display API update functions.
 *
 * Note: Updating options overwrites previously set options.
 */
wiced_result_t audio_display_create_management_thread(wiced_thread_t* thread);

/* Initializes the display settings and icons */
wiced_result_t audio_display_init(u8g_t* u8g_copy);

/* Header update functions */
void audio_display_header_update_battery(power_management_status_t* battery_status, uint8_t options);
void audio_display_header_update_bluetooth(uint8_t options);
void audio_display_header_update_signal_strength(int32_t rssi, uint8_t options);
void audio_display_header_update_options(uint8_t options);
/* Header draw functions */
void audio_display_header_draw_battery();
void audio_display_header_draw_bluetooth();
void audio_display_header_draw_signal_strength();
/* Complete Header functions */
void audio_display_update_header(power_management_status_t* battery_status, int32_t rssi, uint8_t options);
void audio_display_draw_header();

/* Footer update functions */
void audio_display_footer_update_song_info(char* title, char* artist);
void audio_display_footer_update_time_info(uint32_t current_time, uint32_t duraiton);
void audio_display_footer_update_options(uint8_t options);
uint8_t audio_display_get_footer_options(void);
/* Complete Footer functions */
void audio_display_update_footer(char* title, char* artist, uint32_t current_time, uint32_t duration, uint8_t options);
void audio_display_draw_footer();

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
