/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Provides Wiced with function prototypes for IOCTL commands,
 *  and for communicating with the SDPCM module
 *
 */

#ifndef INCLUDED_SDPCM_H
#define INCLUDED_SDPCM_H

#include "wwd_buffer.h"
#include "wwd_constants.h"
#include "wwd_bus_protocol.h"
#include "chip_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *             Constants
 ******************************************************/

/* CDC flag definition taken from bcmcdc.h */
#ifndef CDCF_IOC_SET
#define CDCF_IOC_SET                (0x02)      /** 0=get, 1=set cmd */
#endif /* ifndef CDCF_IOC_SET */

typedef enum sdpcm_command_type_enum
{
    SDPCM_GET = 0x00,
    SDPCM_SET = CDCF_IOC_SET
} sdpcm_command_type_t;

typedef enum
{
    SDPCM_STA_INTERFACE = CHIP_STA_INTERFACE,
    SDPCM_AP_INTERFACE  = CHIP_AP_INTERFACE
} sdpcm_interface_t;

/* IOCTL swapping mode for Big Endian host with Little Endian wlan.  Default to off */
#ifdef IL_BIGENDIAN
wiced_bool_t swap = WICED_FALSE;
#define htod32(i) (swap?bcmswap32(i):i)
#define htod16(i) (swap?bcmswap16(i):i)
#define dtoh32(i) (swap?bcmswap32(i):i)
#define dtoh16(i) (swap?bcmswap16(i):i)
#else /* IL_BIGENDIAN */
#define htod32(i) ((uint32_t)(i))
#define htod16(i) ((uint16_t)(i))
#define dtoh32(i) ((uint32_t)(i))
#define dtoh16(i) ((uint16_t)(i))
#endif /* IL_BIGENDIAN */

/******************************************************
 *             Structures
 ******************************************************/

#define IOCTL_OFFSET ( sizeof(wiced_buffer_header_t) + 12 + 16 )

/******************************************************
 *             Function declarations
 ******************************************************/

extern /*@exposed@*/ /*@null@*/ void* wiced_get_iovar_buffer( /*@returned@*/ /*@out@*/ wiced_buffer_t* buffer, uint16_t data_length, const char* name )  /*@allocates buffer@*/ /*@defines buffer@*/ ;
extern /*@null@*/ void* wiced_get_ioctl_buffer(  /*@returned@*/ /*@out@*/ wiced_buffer_t* buffer, uint16_t data_length )  /*@allocates buffer@*/ ;
extern wiced_result_t wiced_send_ioctl( sdpcm_command_type_t type, uint32_t command, wiced_buffer_t send_buffer_hnd, /*@null@*/ /*@out@*/ wiced_buffer_t* response_buffer_hnd, sdpcm_interface_t interface ) /*@releases send_buffer_hnd@*/ ;
extern wiced_result_t wiced_send_iovar( sdpcm_command_type_t type, wiced_buffer_t send_buffer_hnd, /*@out@*/ /*@null@*/ wiced_buffer_t* response_buffer_hnd, sdpcm_interface_t interface ) /*@releases send_buffer_hnd@*/ ;
extern void wiced_process_sdpcm( /*@only@*/ wiced_buffer_t buffer );
extern wiced_result_t wiced_init_sdpcm( void );
extern void wiced_quit_sdpcm( void );

extern void wiced_process_bus_credit_update(uint8_t* data);
extern uint8_t wiced_get_available_bus_credits( void );

/******************************************************
 *             Global variables
 ******************************************************/

#ifdef __cplusplus
}
#endif 

#endif /* ifndef INCLUDED_SDPCM_H */
