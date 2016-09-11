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

#include "wiced_management.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define APOLLO_MULTICAST_IPV4_ADDRESS_DEFAULT MAKE_IPV4_ADDRESS(224, 0, 0, 55)

#define APOLLO_MAX_SPEAKERS         (24)
#define APOLLO_SPEAKER_NAME_LENGTH  (36)
#define APOLLO_PSP_RETRY_LIMIT      (7)
#define APOLLO_PSP_THRESHOLD        (3)
#define APOLLO_SWDIV_TIMEOUT        (50)

#define NANOSECONDS_PER_SECOND      (1000000000)
#define NANOSECONDS_PER_MILLISECOND (1000000)
#define MICROSECONDS_PER_SECOND     (1000000)
#define MILLISECONDS_PER_SECOND     (1000)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    CHANNEL_MAP_NONE  = 0,          /* None or undefined     */
    CHANNEL_MAP_FL    = (1 << 0),   /* Front Left            */
    CHANNEL_MAP_FR    = (1 << 1),   /* Front Right           */
    CHANNEL_MAP_FC    = (1 << 2),   /* Front Center          */
    CHANNEL_MAP_LFE1  = (1 << 3),   /* LFE-1                 */
    CHANNEL_MAP_BL    = (1 << 4),   /* Back Left             */
    CHANNEL_MAP_BR    = (1 << 5),   /* Back Right            */
    CHANNEL_MAP_FLC   = (1 << 6),   /* Front Left Center     */
    CHANNEL_MAP_FRC   = (1 << 7),   /* Front Right Center    */
    CHANNEL_MAP_BC    = (1 << 8),   /* Back Center           */
    CHANNEL_MAP_LFE2  = (1 << 9),   /* LFE-2                 */
    CHANNEL_MAP_SIL   = (1 << 10),  /* Side Left             */
    CHANNEL_MAP_SIR   = (1 << 11),  /* Side Right            */
    CHANNEL_MAP_TPFL  = (1 << 12),  /* Top Front Left        */
    CHANNEL_MAP_TPFR  = (1 << 13),  /* Top Front Right       */
    CHANNEL_MAP_TPFC  = (1 << 14),  /* Top Front Center      */
    CHANNEL_MAP_TPC   = (1 << 15),  /* Top Center            */
    CHANNEL_MAP_TPBL  = (1 << 16),  /* Top Back Left         */
    CHANNEL_MAP_TPBR  = (1 << 17),  /* Top Back Right        */
    CHANNEL_MAP_TPSIL = (1 << 18),  /* Top Side Left         */
    CHANNEL_MAP_TPSIR = (1 << 19),  /* Top Side Right        */
    CHANNEL_MAP_TPBC  = (1 << 20),  /* Top Back Center       */
    CHANNEL_MAP_BTFC  = (1 << 21),  /* Bottom Front Center   */
    CHANNEL_MAP_BTFL  = (1 << 22),  /* Bottom Front Left     */
    CHANNEL_MAP_BTFR  = (1 << 23),  /* Bottom Front Right    */
    CHANNEL_MAP_TPLS  = (1 << 24),  /* Top Left Surround     */
    CHANNEL_MAP_TPRS  = (1 << 25),  /* Top Right Surround    */
    CHANNEL_MAP_LS    = (1 << 26),  /* Middle Left Surround  */
    CHANNEL_MAP_RS    = (1 << 27),  /* Middle Right Surround */
    CHANNEL_MAP_BLC   = (1 << 28),  /* Back Left Center      */
    CHANNEL_MAP_BRC   = (1 << 29),  /* Back Right Center     */
    CHANNEL_ZERO_030  = (1 << 30),  /* reserved: zero bit    */
    CHANNEL_VIRTUAL   = (1 << 31)   /* Virtual channel FLAG  */
} APOLLO_CHANNEL_MAP_T;

typedef enum
{
    APOLLO_ROLE_SOURCE,
    APOLLO_ROLE_SINK
} apollo_role_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/** Initialize the Apollo audio network.
 *
 * This routine should be used in place of wiced_network_init
 * for Apollo audio applications.
 *
 * @param interface          : used if functions winds up calling wiced_network_up_default(); will be set to WICED_STA_INTERFACE otherwise
 * @param config             : network IP configuration
 * @param static_ip_settings : used by the sender in the RMC group
 * @param role               : role of apollo device
 *
 * @return @ref wiced_result_t Status of the operation.
 */
wiced_result_t apollo_network_up_default( wiced_interface_t* interface, wiced_network_config_t config,
                                          const wiced_ip_setting_t* static_ip_settings, const apollo_role_t role );


/** Overwrite MAC address in Wi-Fi NVRAM data
 *
 * @return @ref wiced_result_t Status of the operation.
 */
wiced_result_t apollo_set_nvram_mac( void );

#ifdef __cplusplus
} /* extern "C" */
#endif
