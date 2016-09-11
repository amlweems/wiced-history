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
 * 802.1AS clock functionality for the BCM43909.
 */

#include "wiced_result.h"
#include "platform_peripheral.h"
#include "platform_ascu.h"
#include "platform_cache.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define NUM_NSECONDS_IN_SECOND      ((uint64_t)1000000000)
#define AVB_ROLLOVER_SECS   ((double)26.844)

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    ASCU_AUDIO_TIMER_TYPE_NONE      = 0,
    ASCU_AUDIO_TIMER_TYPE_FW        = (1 << 0),
    ASCU_AUDIO_TIMER_TYPE_TALKER_FW = (1 << 1)
} ascu_audio_timer_type_t;

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

/******************************************************
 *               Function Definitions
 ******************************************************/

/**
 * Enable the 802.1AS time functionality.
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t platform_time_enable_8021as(void)
{
    platform_ascu_enable_interrupts( ASCU_TX_START_AVB_INT_MASK | ASCU_RX_START_AVB_INT_MASK );

    /*
     * Enable the ascu_avb_clk.
     */

    platform_pmu_chipcontrol(PMU_CHIPCONTROL_PWM_CLK_ASCU_REG, 0, PMU_CHIPCONTROL_PWM_CLK_ASCU_MASK);

    return WICED_SUCCESS;
}


/**
 * Disable the 802.1AS time functionality.
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
wiced_result_t platform_time_disable_8021as(void)
{
    platform_ascu_disable_interrupts( ASCU_TX_START_AVB_INT_MASK | ASCU_RX_START_AVB_INT_MASK );

    return WICED_SUCCESS;
}


/**
 * Read the 802.1AS time.
 *
 * Retrieve the origin timestamp in the last sync message, correct for the
 * intervening interval and return the corrected time in seconds + nanoseconds.
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
static inline wiced_result_t platform_time_read_8021as_extended(ascu_audio_timer_type_t type,
                                                                uint32_t *master_secs, uint32_t *master_nanosecs,
                                                                uint32_t *local_secs, uint32_t *local_nanosecs,
                                                                uint32_t *audio_time_hi, uint32_t *audio_time_lo)
{
    uint32_t correction_secs;
    uint32_t correction_nanosecs;
    uint32_t avb_secs     = 0;
    uint32_t avb_nanosecs = 0;
    uint32_t fw_ntimer_hi = 0;
    uint32_t fw_ntimer_lo = 0;
    uint32_t net_timer_rxhi = 0;
    uint32_t net_timer_rxlo = 0;
    uint64_t net_time = 0;
    uint64_t fw_time;
    uint64_t correction;
    unsigned int lock;
    volatile wlc_avb_timestamp_t *p_avb;


    if (master_secs == NULL || master_nanosecs == NULL || local_secs == NULL || local_nanosecs == NULL)
    {
        return WICED_ERROR;
    }

    p_avb = platform_ascu_get_avb_ts();
    if (p_avb == NULL)
    {
        *master_secs     = 0;
        *master_nanosecs = 0;
        *local_secs      = 0;
        *local_nanosecs  = 0;

        return WICED_ERROR;
    }

    WICED_DISABLE_INTERRUPTS();

    /*
     * Seqlock implemented in WLAN driver per references in
     * https://en.wikipedia.org/wiki/Seqlock.
     * Check lock before and after reading AVB time values from the
     * structure.  If the lock is odd at either point, the WLAN CPU
     * may have been updating them while this code was reading. If not,
     * the WLAN  could have started and completed the write between
     * the two checks of the lock, in which case the values of the
     * lock will not be equal.
     */
    do
    {
        /* invalidate the timestamp for reading & read the pertinent values */
        //platform_dcache_inv_range( (volatile void *)p_avb, sizeof(*p_avb));

        lock = p_avb->lock;
        if (!(lock & 1))
        {
            avb_secs        = p_avb->as_seconds;
            avb_nanosecs    = p_avb->as_nanosecs;
            net_timer_rxhi  = p_avb->net_timer_rxhi;
            net_timer_rxlo  = p_avb->net_timer_rxlo;

            /* Get the current time for the delta since receipt of Sync */
            (void)platform_ascu_read_raw_fw_ntimer(&fw_ntimer_hi, &fw_ntimer_lo);

            /* invalidate the timestamp struct for checking the lock flag again. */
            //platform_dcache_inv_range( (volatile void *)p_avb, sizeof(*p_avb));
        }
    }while((lock & 1) || (p_avb->lock & 1) || (lock != p_avb->lock));


    switch (type)
    {
        case ASCU_AUDIO_TIMER_TYPE_FW:
            (void)platform_ascu_read_fw_audio_timer(audio_time_hi, audio_time_lo);
            break;

        case ASCU_AUDIO_TIMER_TYPE_TALKER_FW:
            (void)platform_ascu_read_fw_audio_talker_timer(audio_time_hi, audio_time_lo);
            break;

        default:
            break;
    }

    WICED_ENABLE_INTERRUPTS();

    /* Compute the local time */
    (void)platform_ascu_convert_ntimer(fw_ntimer_hi, fw_ntimer_lo, local_secs, local_nanosecs);

    /* Perform the correction calculation in 64-bit math */
    net_time   = ((uint64_t)net_timer_rxhi << 32) | net_timer_rxlo;
    fw_time    = ((uint64_t)fw_ntimer_hi << 32) | fw_ntimer_lo;
    correction = fw_time - net_time;

    (void)platform_ascu_convert_ntimer((uint32_t)(correction >> 32), (uint32_t) (correction & 0xFFFFFFFF), &correction_secs, &correction_nanosecs);

    *master_nanosecs = avb_nanosecs + correction_nanosecs;
    if (*master_nanosecs > ONE_BILLION)
    {
        correction_secs  += 1;
        *master_nanosecs -= ONE_BILLION;
    }
    *master_secs     = avb_secs + correction_secs;

    return WICED_SUCCESS;
}


wiced_result_t platform_time_read_8021as(uint32_t *master_secs, uint32_t *master_nanosecs,
                                         uint32_t *local_secs, uint32_t *local_nanosecs)
{
    return platform_time_read_8021as_extended(ASCU_AUDIO_TIMER_TYPE_NONE, master_secs, master_nanosecs, local_secs, local_nanosecs, NULL, NULL);
}


wiced_result_t platform_time_read_8021as_with_audio(uint32_t *master_secs, uint32_t *master_nanosecs,
                                                    uint32_t *local_secs, uint32_t *local_nanosecs,
                                                    uint32_t *audio_time_hi, uint32_t *audio_time_lo)
{
    return platform_time_read_8021as_extended(ASCU_AUDIO_TIMER_TYPE_FW, master_secs, master_nanosecs, local_secs, local_nanosecs, audio_time_hi, audio_time_lo);
}
