/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "wiced_result.h"
#include "data_types.h"

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

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef UINT8 tUSERIAL_PORT;
typedef UINT8 tUSERIAL_EVT;

/******************************************************
 *                    Structures
 ******************************************************/

typedef union
{
    UINT8 sigs;
    UINT8 error;
} tUSERIAL_EVT_DATA;

typedef void (tUSERIAL_CBACK)(tUSERIAL_PORT, tUSERIAL_EVT, tUSERIAL_EVT_DATA *);

typedef struct
{
    void    *p_first;
    void    *p_last;
    UINT16   count;
} BUFFER_Q;

typedef struct
{
    UINT16          event;
    UINT16          len;
    UINT16          offset;
    UINT16          layer_specific;
} BT_HDR;

typedef struct
{
    UINT16 fmt;          /* Data format                       */
    UINT8  baud;         /* Baud rate                         */
    UINT8  fc;           /* Flow control                      */
    UINT8  buf;          /* Data buffering mechanism          */
    UINT8  pool;         /* GKI buffer pool for received data */
    UINT16 size;         /* Size of GKI buffer pool           */
    UINT16 offset;       /* Offset in GKI buffer pool         */
} tUSERIAL_OPEN_CFG;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *          NFC to Host Function Declarations
 ******************************************************/

extern wiced_result_t nfc_bus_init( void );
extern wiced_result_t nfc_bus_deinit( void );
extern wiced_result_t nfc_bus_transmit( const uint8_t* data_out, uint32_t size );
extern wiced_result_t nfc_bus_receive( uint8_t* data_in, uint32_t size, uint32_t timeout_ms );

void nfc_host_free_patch_ram_resource( const UINT8* buffer );
void nfc_host_free_i2c_pre_patch_resource( const UINT8* buffer );
void nfc_host_get_patch_ram_resource( const UINT8** buffer, uint32_t* size );
void nfc_host_get_i2c_pre_patch_resource( const UINT8** buffer, uint32_t* size  );

/******************************************************
 *           Host to NFC Function Declarations
 ******************************************************/



#ifdef __cplusplus
} /* extern "C" */
#endif
