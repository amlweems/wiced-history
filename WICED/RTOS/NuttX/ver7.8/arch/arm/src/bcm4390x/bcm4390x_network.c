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

#include <assert.h>

#include "up_internal.h"

#include "bcm4390x_wwd.h"

void up_netinitialize(void)
{
#ifdef CONFIG_BCM4390X_WWD
  int err = bcm4390x_wwd_init();
  DEBUGASSERT(err == OK);
#endif /* CONFIG_BCM4390X_WWD */
}
