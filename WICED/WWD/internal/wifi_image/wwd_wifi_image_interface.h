/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef INCLUDED_WWD_WIFI_IMAGE_INTERFACE_H_
#define INCLUDED_WWD_WIFI_IMAGE_INTERFACE_H_

#include "wwd_constants.h"
#include "wwd_buffer.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *             Function declarations
 ******************************************************/

extern wiced_result_t wiced_write_wifi_firmware_image( void );
extern wiced_result_t wiced_write_wifi_nvram_image( void );

extern /*@observer@*/ const char* const    dlimagename;
extern /*@observer@*/ const char* const    dlimagever;
extern /*@observer@*/ const char* const    dlimagedate;
extern                const unsigned long  wifi_firmware_image_size;
extern                const unsigned char  wifi_firmware_image[];


extern uint32_t host_platform_read_memory_wifi_firmware( uint32_t offset, /*@out@*/wiced_buffer_t* buffer );
extern uint32_t host_platform_memory_wifi_nvram_size( void );
extern uint32_t host_platform_read_memory_wifi_nvram( uint32_t offset, /*@out@*/wiced_buffer_t* buffer );

#ifdef __cplusplus
}
#endif

#endif /* ifndef INCLUDED_WWD_WIFI_IMAGE_INTERFACE_H_ */
