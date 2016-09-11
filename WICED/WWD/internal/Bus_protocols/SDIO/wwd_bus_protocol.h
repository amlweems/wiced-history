/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef INCLUDED_WICED_BUS_PROTOCOL_H
#define INCLUDED_WICED_BUS_PROTOCOL_H

#include "Platform/wwd_sdio_interface.h"
#include "wwd_buffer.h"

/******************************************************
 *             Constants
 ******************************************************/

/******************************************************
 *             Structures
 ******************************************************/

#undef WICED_BUS_HAS_HEADER

#define WICED_BUS_HEADER_SIZE (0)

#define WICED_BUS_PACKET_AVAILABLE_TO_READ(intstatus)     ((intstatus) & (FRAME_AVAILABLE_MASK))
#define WICED_BUS_USE_STATUS_REPORT_SCHEME                (0)

/******************************************************
 *             Function declarations
 ******************************************************/


/******************************************************
 *             Global variables
 ******************************************************/

#endif /* ifndef INCLUDED_WICED_BUS_PROTOCOL_H */
