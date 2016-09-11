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
#include <nuttx/pwm.h>

#include <stdio.h>

#include "wiced_platform.h"

#include "platform_config.h"

#ifndef CONFIG_PWM_NAME
#define CONFIG_PWM_NAME  "/dev/pwm%d"
#endif

#if PLATFORM_NO_PWM

int pwm_devinit(void)
{
  return -ENXIO;
}

#else

struct bcm4390x_pwm_s
{
  struct pwm_lowerhalf_s lower; /* must be first */
  wiced_pwm_t            pwm;
};

static inline struct bcm4390x_pwm_s* bcm4390x_pwm_lowerhalf_to_priv(struct pwm_lowerhalf_s *lower)
{
  return (struct bcm4390x_pwm_s*)lower;
}

static inline float bcm4390x_pwm_nuttx_to_wiced_duty(const struct pwm_info_s *info)
{
  /*
   * WICED duty cycle defined as floating-point percentage (0.0 to 100.0).
   * NuttX duty cycle defined as 16-bit: maximum 65535/65536 (0x0000ffff), minimum 1/65536 (0x00000001).
   * Convert NuttX representation to WICED.
   */
  return ((float)info->duty * (100.0f / 65536.0f));
}

static int bcm4390x_pwm_setup(struct pwm_lowerhalf_s *lower)
{
  return OK;
}

static int bcm4390x_pwm_shutdown(struct pwm_lowerhalf_s *lower)
{
  return lower->ops->stop(lower);
}

static int bcm4390x_pwm_start(struct pwm_lowerhalf_s *lower, const struct pwm_info_s *info)
{
  struct bcm4390x_pwm_s *priv  = bcm4390x_pwm_lowerhalf_to_priv(lower);
  int                   result = OK;

  if (wiced_pwm_init(priv->pwm, info->frequency, bcm4390x_pwm_nuttx_to_wiced_duty(info)) != WICED_SUCCESS)
  {
    result = -EIO;
  }
  else if (wiced_pwm_start(priv->pwm) != WICED_SUCCESS)
  {
    result = -EIO;
  }

  return result;
}

static int bcm4390x_pwm_stop(struct pwm_lowerhalf_s *lower)
{
  struct bcm4390x_pwm_s *priv  = bcm4390x_pwm_lowerhalf_to_priv(lower);
  int                   result = OK;

  if (wiced_pwm_stop(priv->pwm) != WICED_SUCCESS)
  {
    result = -EIO;
  }

  return result;
}

static int bcm4390x_pwm_ioctl(struct pwm_lowerhalf_s *lower, int cmd, unsigned long arg)
{
  return -ENOSYS;
}

struct pwm_ops_s bcm4390x_pwm_ops =
{
  .setup    = bcm4390x_pwm_setup,
  .shutdown = bcm4390x_pwm_shutdown,
  .start    = bcm4390x_pwm_start,
  .stop     = bcm4390x_pwm_stop,
  .ioctl    = bcm4390x_pwm_ioctl
};

static struct bcm4390x_pwm_s bcm4390x_pwm[WICED_PWM_MAX];

static struct pwm_lowerhalf_s* bcm4390x_pwm_init_lowerhalf(wiced_pwm_t pwm)
{
  struct bcm4390x_pwm_s *priv = &bcm4390x_pwm[pwm];

  DEBUGASSERT(pwm < WICED_PWM_MAX);

  priv->lower.ops = &bcm4390x_pwm_ops;
  priv->pwm       = pwm;

  return &priv->lower;
}

int pwm_devinit(void)
{
  static bool initialized = false;

  int         result = OK;
  wiced_pwm_t pwm;

  if (initialized)
  {
    return OK;
  }

  for (pwm = WICED_PWM_1; pwm < WICED_PWM_MAX; pwm++)
  {
    char path[sizeof(CONFIG_PWM_NAME) + 2];

    if (snprintf(path, sizeof(path), CONFIG_PWM_NAME, pwm) >= sizeof(path))
    {
      result = -EIO;
      DEBUGASSERT(0);
    }
    else if (pwm_register(path, bcm4390x_pwm_init_lowerhalf(pwm)) != 0)
    {
      result = -EIO;
      DEBUGASSERT(0);
    }
  }

  initialized = true;

  return result;
}

#endif /* PLATFORM_NO_PWM */
