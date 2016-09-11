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
#include <nuttx/watchdog.h>

#include "platform_peripheral.h"

#ifndef CONFIG_WATCHDOG_DEVPATH
#define CONFIG_WATCHDOG_DEVPATH  "/dev/watchdog0"
#endif

#ifdef CONFIG_WATCHDOG

struct bcm4390x_watchdog_s
{
  struct watchdog_lowerhalf_s lower; /* must be first */
  uint32_t                    timeout_ms;
};

static inline struct bcm4390x_watchdog_s* bcm4390x_watchdog_lowerhalf_to_priv(struct watchdog_lowerhalf_s *lower)
{
  return (struct bcm4390x_watchdog_s*)lower;
}

static int bcm4390x_watchdog_start(struct watchdog_lowerhalf_s *lower)
{
  int result = OK;

  if (platform_watchdog_init() != PLATFORM_SUCCESS)
  {
    result = -EIO;
  }

  return result;
}

static int bcm4390x_watchdog_stop(struct watchdog_lowerhalf_s *lower)
{
  int result = OK;

  if (platform_watchdog_deinit() != PLATFORM_SUCCESS)
  {
    result = -EIO;
  }

  return result;
}

static int bcm4390x_watchdog_keepalive(struct watchdog_lowerhalf_s *lower)
{
  struct bcm4390x_watchdog_s *priv = bcm4390x_watchdog_lowerhalf_to_priv(lower);
  platform_result_t platform_result;

  if (priv->timeout_ms != 0)
  {
    platform_result = platform_watchdog_kick_milliseconds(priv->timeout_ms);
  }
  else
  {
    platform_result = platform_watchdog_kick();
  }

  return (platform_result == PLATFORM_SUCCESS) ? OK : -EIO;
}

static int bcm4390x_watchdog_settimeout(struct watchdog_lowerhalf_s *lower, uint32_t timeout_ms)
{
  struct bcm4390x_watchdog_s *priv = bcm4390x_watchdog_lowerhalf_to_priv(lower);

  priv->timeout_ms = timeout_ms;

  return lower->ops->keepalive(lower);
}

struct watchdog_ops_s bcm4390x_watchdog_ops =
{
  .start      = bcm4390x_watchdog_start,
  .stop       = bcm4390x_watchdog_stop,
  .keepalive  = bcm4390x_watchdog_keepalive,
  .settimeout = bcm4390x_watchdog_settimeout
};

static struct bcm4390x_watchdog_s bcm4390x_watchdog =
{
  .lower      = { .ops = &bcm4390x_watchdog_ops },
  .timeout_ms = 0
};

#endif /* CONFIG_WATCHDOG */

int up_wdginitialize(void)
{
  /*
   * Watchdog is started during early platform initialization.
   * Idea is watchdog would reset board if initalization hang.
   * Even if watchdog support is compiled-in here, let's deinit watchdog
   * and start it again. This is because of watchdog driver initial state
   * is stopped.
   */
  platform_watchdog_deinit();

#ifdef CONFIG_WATCHDOG

  if (watchdog_register(CONFIG_WATCHDOG_DEVPATH, &bcm4390x_watchdog.lower) == NULL)
  {
    DEBUGASSERT(0);
    return -ENODEV;
  }

#endif /* CONFIG_WATCHDOG */

  return OK;
}
