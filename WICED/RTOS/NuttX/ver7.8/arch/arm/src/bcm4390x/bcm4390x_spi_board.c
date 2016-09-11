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

#include <stdbool.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/spi/spi.h>

#include "wiced_platform.h"

#include "bcm4390x_spi.h"

/* This file has the board-specific initialization for SPI
 * BCM943909WCD1_3 and similar boards of the BCM4390X family.
 * Applied to boards which have a SPI flash on one of SPI ports,
 * and where port described in wiced_spi_flash variable.
 * Other boards with different SPI slave devices
 * should have their own file.
 */

#if defined(CONFIG_BCM4390X_SPI1) || defined(CONFIG_BCM4390X_SPI2)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: bcm4390x_spiinitialize
 *
 * Description:
 *   Map SPI port numbers to SPI Slave devices
 *
 ****************************************************************************/

void weak_function bcm4390x_spiinitialize(int port, wiced_spi_device_t **wiced_spi_device)
{
  *wiced_spi_device = NULL;

#ifdef WICED_PLATFORM_INCLUDES_SPI_FLASH
  if (port == wiced_spi_flash.port)
  {
    *wiced_spi_device = (wiced_spi_device_t *)&wiced_spi_flash;
  }
#endif
}

#endif /* (CONFIG_BCM4390X_SPI1) || defined(CONFIG_BCM4390X_SPI2) */
