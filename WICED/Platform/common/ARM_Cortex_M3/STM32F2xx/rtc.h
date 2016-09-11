/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#define WICED_VERIFY_TIME(time, valid) \
    if( (time->sec > 60) || ( time->min > 60 ) || (time->hr > 24) || ( time->date > 31 ) || ( time->month > 12 )) \
    { \
        valid= WICED_FALSE; \
    } \
    else \
    { \
        valid= WICED_TRUE; \
    }



#ifndef WICED_DISABLE_MCU_POWERSAVE
wiced_result_t rtc_sleep_entry( void );
wiced_result_t rtc_sleep_abort( void );
wiced_result_t rtc_sleep_exit( unsigned long requested_sleep_time, unsigned long *cpu_sleep_time );
#endif /* #ifndef WICED_DISABLE_MCU_POWERSAVE */

wiced_result_t platform_set_rtc_time(wiced_rtc_time_t* time);
