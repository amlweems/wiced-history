/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>

#include <nuttx/arch.h>
#include <nuttx/spi/spi.h>

#include "bcm4390x_spi.h"

#if !defined(CONFIG_BCM4390X_SPI1) && !defined(CONFIG_BCM4390X_SPI2)
# error CONFIG_BCM4390X_SPI1 or CONFIG_BCM4390X_SPI2 must be defined in defconfig
#endif /* defined(CONFIG_BCM4390X_SPI1) || defined(CONFIG_BCM4390X_SPI2) */

/************************************************************************************
 * Definitions
 ************************************************************************************/

extern const platform_spi_t   platform_spi_peripherals[];
extern const platform_gpio_t  platform_gpio_pins[];

/************************************************************************************
 * Private Types
 ************************************************************************************/

struct bcm4390x_spidev_s
{
  /* spidev MUST be the first field of this structure */
  /* Otherwise spi_select() and other functions will fail to cast pointers properly */

  struct spi_dev_s         spidev;     /* Externally visible part of the SPI interface */
  wiced_spi_device_t       *wiced_spi_device;
  platform_gpio_t          *chip_select;
#ifndef CONFIG_SPI_OWNBUS
  sem_t                    exclsem;    /* Held while chip is selected for mutual exclusion */
  uint32_t                 frequency;  /* Requested clock frequency */
  uint32_t                 actual;     /* Actual clock frequency */
  uint8_t                  nbits;      /* Width of work in bits (8 or 16) */
  uint8_t                  mode;       /* Mode 0,1,2,3 */
#endif
};

/************************************************************************************
 * Private Function Prototypes
 ************************************************************************************/
#ifndef CONFIG_SPI_OWNBUS
static int         spi_lock(struct spi_dev_s *dev, bool lock);
#endif
static void        spi_select(struct spi_dev_s *dev, enum spi_dev_e devid, bool selected);
static uint32_t    spi_setfrequency(struct spi_dev_s *dev, uint32_t frequency);
static void        spi_setmode(struct spi_dev_s *dev, enum spi_mode_e mode);
static void        spi_setbits(struct spi_dev_s *dev, int nbits);
static uint8_t     spi_status(struct spi_dev_s *dev, enum spi_dev_e devid);
static uint16_t    spi_send(struct spi_dev_s *dev, uint16_t word);
static void        spi_exchange(struct spi_dev_s *dev, const void *txbuffer,
                                void *rxbuffer, size_t nwords);
#ifndef CONFIG_SPI_EXCHANGE
static void        spi_sndblock(struct spi_dev_s *dev, const void *txbuffer, size_t nwords);
static void        spi_recvblock(struct spi_dev_s *dev, void *rxbuffer, size_t nwords);
#endif

/************************************************************************************
 * Private Data
 ************************************************************************************/

static const struct spi_ops_s g_spiops =
{
#ifndef CONFIG_SPI_OWNBUS
  .lock              = spi_lock,
#endif
  .select            = spi_select,
  .setfrequency      = spi_setfrequency,
  .setmode           = spi_setmode,
  .setbits           = spi_setbits,
  .status            = spi_status,
  .send              = spi_send,
#ifdef CONFIG_SPI_EXCHANGE
  .exchange          = spi_exchange,
#else
  .sndblock          = spi_sndblock,
  .recvblock         = spi_recvblock,
#endif
  .registercallback  = 0,
};

#ifdef CONFIG_BCM4390X_SPI1
static struct bcm4390x_spidev_s g_spidev0 =
{
  .spidev            =
  {
    .ops             =  &g_spiops,
  },
};
#endif /* CONFIG_BCM4390X_SPI1 */

#ifdef CONFIG_BCM4390X_SPI2
static struct bcm4390x_spidev_s g_spidev1 =
{
  .spidev            =
  {
    .ops             =  &g_spiops,
  },
};
#endif /* CONFIG_BCM4390X_SPI2 */

/************************************************************************************
 * Public Data
 ************************************************************************************/

/************************************************************************************
 * Private Functions
 ************************************************************************************/

/****************************************************************************
 * Name: spi_lock
 *
 * Description:
 *   On SPI busses where there are multiple devices, it will be necessary to
 *   lock SPI to have exclusive access to the busses for a sequence of
 *   transfers.  The bus should be locked before the chip is selected. After
 *   locking the SPI bus, the caller should then also call the setfrequency,
 *   setbits, and setmode methods to make sure that the SPI is properly
 *   configured for the device.  If the SPI buss is being shared, then it
 *   may have been left in an incompatible state.
 *
 * Input Parameters:
 *   dev  - Device-specific state data
 *   lock - true: Lock spi bus, false: unlock SPI bus
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/
#ifndef CONFIG_SPI_OWNBUS
static int spi_lock(struct spi_dev_s *dev, bool lock)
{
  struct bcm4390x_spidev_s *priv = (struct bcm4390x_spidev_s *)dev;
  int result;

  if (lock)
    {
      /* Take the semaphore (perhaps waiting) */

      while (sem_wait(&priv->exclsem) != 0)
        {
          /* The only case that an error should occur here is if the wait was awakened
           * by a signal.
           */

          DEBUGASSERT(errno == EINTR);
        }
    }
  else
    {
      result = sem_post(&priv->exclsem);
      DEBUGASSERT(result == OK);
    }
  return OK;
}
#endif
/****************************************************************************
 * Name: spi_select
 *
 * Description:
 *   Enable/disable the SPI slave select.   The implementation of this method
 *   must include handshaking:  If a device is selected, it must hold off
 *   all other attempts to select the device until the device is deselecte.
 *
 * Input Parameters:
 *   dev -      Device-specific state data
 *   devid -    Identifies the device to select
 *   selected - true: slave selected, false: slave de-selected
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void spi_select(struct spi_dev_s *dev, enum spi_dev_e devid, bool selected)
{
  struct bcm4390x_spidev_s *priv     = (struct bcm4390x_spidev_s *)dev;
  const wiced_spi_device_t *spi      = priv->wiced_spi_device;
  platform_spi_config_t config;

  config.chip_select = ( spi->chip_select != WICED_GPIO_NONE ) ? &platform_gpio_pins[spi->chip_select] : NULL;
  config.speed       = spi->speed;
  config.mode        = spi->mode;
  config.bits        = spi->bits;

  if (selected == TRUE)
  {
    platform_spi_core_clock_toggle(WICED_TRUE);
    platform_spi_init(&platform_spi_peripherals[priv->wiced_spi_device->port], &config);
    platform_spi_chip_select_toggle(&platform_spi_peripherals[priv->wiced_spi_device->port], &config, WICED_TRUE);
  }
  else
  {
    platform_spi_chip_select_toggle(&platform_spi_peripherals[priv->wiced_spi_device->port], &config, WICED_FALSE);
    platform_spi_core_clock_toggle(WICED_FALSE);
  }
}

/************************************************************************************
 * Name: spi_setfrequency
 *
 * Description:
 *   Set the SPI frequency.
 *
 * Input Parameters:
 *   dev -       Device-specific state data
 *   frequency - The SPI frequency requested
 *
 * Returned Value:
 *   Returns the actual frequency selected
 *
 ************************************************************************************/

static uint32_t spi_setfrequency(struct spi_dev_s *dev, uint32_t frequency)
{
  /* Frequency is set by spi_select()->platform_spi_init() */
  struct bcm4390x_spidev_s *priv = (struct bcm4390x_spidev_s *)dev;

  return priv->actual;
}

/************************************************************************************
 * Name: spi_setmode
 *
 * Description:
 *   Set the SPI mode.  see enum spi_mode_e for mode definitions
 *
 * Input Parameters:
 *   dev  - Device-specific state data
 *   mode - The SPI mode requested
 *
 * Returned Value:
 *   Returns the actual frequency selected
 *
 ************************************************************************************/

static void spi_setmode(struct spi_dev_s *dev, enum spi_mode_e mode)
{
  DEBUGASSERT(mode == SPIDEV_MODE0); /* TODO: need to double check */
}

/************************************************************************************
 * Name: spi_setbits
 *
 * Description:
 *   Set the number of bits per word.
 *
 * Input Parameters:
 *   dev   - Device-specific state data
 *   nbits - The number of bits requested
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

static void spi_setbits(struct spi_dev_s *dev, int nbits)
{
  DEBUGASSERT(nbits == 8);
}

/****************************************************************************
 * Name: spi_status
 *
 * Description:
 *   Get SPI/MMC status
 *
 * Input Parameters:
 *   dev -   Device-specific state data
 *   devid - Identifies the device to report status on
 *
 * Returned Value:
 *   Returns a bitset of status values (see SPI_STATUS_* defines
 *
 ****************************************************************************/

static uint8_t spi_status(struct spi_dev_s *dev, enum spi_dev_e devid)
{
    return SPI_STATUS_PRESENT;
}

/************************************************************************************
 * Name: spi_send
 *
 * Description:
 *   Exchange one word on SPI
 *
 * Input Parameters:
 *   dev  - Device-specific state data
 *   word - The word to send.  the size of the data is determined by the
 *          number of bits selected for the SPI interface.
 *
 * Returned Value:
 *   response
 *
 ************************************************************************************/

static uint16_t spi_send(struct spi_dev_s *dev, uint16_t word)
{
  uint8_t txbyte;
  uint8_t rxbyte;

  txbyte = (uint8_t)word;
  rxbyte = (uint8_t)0;
  spi_exchange(dev, &txbyte, &rxbyte, 1);

  return (uint16_t)rxbyte;
}

/*************************************************************************
 * Name: spi_exchange
 *
 * Description:
 *   Exchange a block of data on SPI
 *
 * Input Parameters:
 *   dev      - Device-specific state data
 *   txbuffer - A pointer to the buffer of data to be sent
 *   rxbuffer - A pointer to a buffer in which to receive data
 *   nwords   - the length of data to be exchaned in units of words.
 *              The wordsize is determined by the number of bits-per-word
 *              selected for the SPI interface.  If nbits <= 8, the data is
 *              packed into uint8_t's; if nbits >8, the data is packed into uint16_t's
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

static void spi_exchange(struct spi_dev_s *dev, const void *txbuffer,
                         void *rxbuffer, size_t nwords)
{
  struct bcm4390x_spidev_s *priv = (struct bcm4390x_spidev_s *)dev;
  const wiced_spi_device_t *spi;
  platform_spi_config_t config;

  DEBUGASSERT(priv != NULL);

  spi = priv->wiced_spi_device;

  config.chip_select = ( spi->chip_select != WICED_GPIO_NONE ) ? &platform_gpio_pins[spi->chip_select] : NULL;
  config.speed       = spi->speed;
  config.mode        = spi->mode;
  config.bits        = spi->bits;

  platform_spi_transfer_nosetup(&platform_spi_peripherals[priv->wiced_spi_device->port], &config,
    (const uint8_t*)txbuffer, (uint8_t*)rxbuffer, nwords);
}

/*************************************************************************
 * Name: spi_sndblock
 *
 * Description:
 *   Send a block of data on SPI
 *
 * Input Parameters:
 *   dev      - Device-specific state data
 *   txbuffer - A pointer to the buffer of data to be sent
 *   nwords   - the length of data to send from the buffer in number of words.
 *              The wordsize is determined by the number of bits-per-word
 *              selected for the SPI interface.  If nbits <= 8, the data is
 *              packed into uint8_t's; if nbits >8, the data is packed into uint16_t's
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

#ifndef CONFIG_SPI_EXCHANGE
static void spi_sndblock(struct spi_dev_s *dev, const void *txbuffer, size_t nwords)
{
  return spi_exchange(dev, txbuffer, NULL, nwords);
}
#endif

/************************************************************************************
 * Name: spi_recvblock
 *
 * Description:
 *   Revice a block of data from SPI
 *
 * Input Parameters:
 *   dev      - Device-specific state data
 *   rxbuffer - A pointer to the buffer in which to recieve data
 *   nwords   - the length of data that can be received in the buffer in number
 *              of words.  The wordsize is determined by the number of bits-per-word
 *              selected for the SPI interface.  If nbits <= 8, the data is
 *              packed into uint8_t's; if nbits >8, the data is packed into uint16_t's
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

#ifndef CONFIG_SPI_EXCHANGE
static void spi_recvblock(struct spi_dev_s *dev, void *rxbuffer, size_t nwords)
{
  return spi_exchange(dev, NULL, rxbuffer, nwords);
}
#endif

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: up_spiinitialize
 *
 * Description:
 *   Initialize the selected SPI port
 *
 * Input Parameter:
 *   Port number (for hardware that has mutiple SPI interfaces)
 *
 * Returned Value:
 *   Valid SPI device structure reference on succcess; a NULL on failure
 *
 ************************************************************************************/

struct spi_dev_s* up_spiinitialize(int port)
{
  struct bcm4390x_spidev_s *priv = NULL;

  switch(port)
  {
#ifdef CONFIG_BCM4390X_SPI1
    case 0 :
      priv = &g_spidev0;
      break;
#endif
#ifdef CONFIG_BCM4390X_SPI2
    case 1 :
      priv = &g_spidev1;
      break;
#endif
    default :
      return NULL;
  }

  /* Board Specific Init */
  bcm4390x_spiinitialize(port, &(priv->wiced_spi_device));
  if (priv->wiced_spi_device == NULL)
  {
      DEBUGASSERT(FALSE);
      return NULL;
  }

  priv->actual     = priv->wiced_spi_device->speed;
  priv->frequency  = priv->wiced_spi_device->speed;
  priv->nbits      = priv->wiced_spi_device->bits;
  priv->mode       = priv->wiced_spi_device->mode;

  /* Initialize the SPI semaphore that enforces mutually exclusive access */
#ifndef CONFIG_SPI_OWNBUS
  sem_init(&priv->exclsem, 0, 1);
#endif

  return (struct spi_dev_s *)priv;
}


