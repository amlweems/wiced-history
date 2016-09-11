/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include <nuttx/config.h>
#include <nuttx/arch.h>
#include <nuttx/init.h>
#include <nuttx/watchdog.h>
#include <nuttx/syslog/ramlog.h>

#include "up_internal.h"

#include "platform_appscr4.h"

#include "wiced_management.h"

void _start_low(void);
void _start_platform_init(void);

void _start(void)
{
  _start_low();

  os_start();
}

void up_initialize(void)
{
  _start_platform_init();

  platform_tick_start();

#ifdef CONFIG_RAMLOG_SYSLOG
  ramlog_sysloginit();
#endif

#ifdef CONFIG_DEV_NULL
  devnull_register();
#endif

  up_serialinit();

  up_wdginitialize();

  irqenable();
}

#ifdef CONFIG_BOARD_INITIALIZE
#ifndef CONFIG_BOARD_INITTHREAD
#  error Network initialization required to be in separate thread after the whole NuttX initialization done
#endif
void board_initialize(void)
{
  wiced_core_init();
  up_netinitialize();
}
#endif
