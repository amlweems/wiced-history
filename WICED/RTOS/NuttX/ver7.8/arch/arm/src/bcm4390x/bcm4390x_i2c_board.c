/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "wiced_platform.h"

#include "bcm4390x_i2c.h"


/* This file has the board-specific initialization for I2C
 * BCM943909WCD1_3 and similar boards of the BCM4390X family.
 */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

extern const wiced_i2c_device_t auth_chip_i2c_device;

/****************************************************************************
 * Name: bcm4390x_i2cinitialize
 *
 * Description:
 *   Map I2C port numbers to I2C Slave devices
 *
 ****************************************************************************/

void weak_function bcm4390x_i2cinitialize(int port, wiced_i2c_device_t **wiced_i2c_device)
{
  *wiced_i2c_device = NULL;

#ifdef AUTH_IC_I2C_PORT
  if (port == auth_chip_i2c_device.port)
  {
    *wiced_i2c_device = (wiced_i2c_device_t *)&auth_chip_i2c_device;
  }
#endif
}
