/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef INCLUDED_BOOTLOADER_APP_H_
#define INCLUDED_BOOTLOADER_APP_H_

#include "platform_bootloader.h"

extern void* bootloader_api_addr_loc; /* from link script */

#define bootloader_api   ((bootloader_api_t*)(&bootloader_api_addr_loc))

#endif /* ifndef INCLUDED_BOOTLOADER_APP_H_ */
