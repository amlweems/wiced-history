/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef INCLUDED_GSPI_H
#define INCLUDED_GSPI_H

#include <stdint.h>

/******************************************************
 *             Constants
 ******************************************************/

/******************************************************
 *             Structures
 ******************************************************/

typedef uint32_t gspi_header_t;


#pragma pack(1)

typedef struct
{
    gspi_header_t gspi_header;
} wiced_bus_header_t;

#pragma pack()

#define WICED_BUS_HAS_HEADER                (1)
#define WICED_BUS_HEADER_SIZE               (sizeof(wiced_bus_header_t))

#define WICED_BUS_PACKET_AVAILABLE_TO_READ(intstatus)    ((intstatus) & (F2_PACKET_AVAILABLE))
#define WICED_BUS_USE_STATUS_REPORT_SCHEME               (1)

/******************************************************
 *             Function declarations
 ******************************************************/

/******************************************************
 *             Global variables
 ******************************************************/

#endif /* ifndef _GSPI_H */
