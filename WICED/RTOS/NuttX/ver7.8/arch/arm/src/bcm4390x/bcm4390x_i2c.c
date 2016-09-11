/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*******************************************************************************
 * Included Files
 *******************************************************************************/
#include <nuttx/config.h>
#include <sys/types.h>
#include <stdint.h>

#include "bcm4390x_i2c.h"

#include <debug.h>

/*******************************************************************************
 * Definitions
 *******************************************************************************/
#define I2C_STANDARD_SPEED_FREQUENCY    10000
#define I2C_LOW_SPEED_FREQUENCY         100000
#define I2C_HIGH_SPEED_FREQUENCY        400000

extern platform_i2c_t   platform_i2c_peripherals[];

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
struct bcm4390x_i2cdev_s
{
  /* i2c_dev_S MUST be the first field of this structure */
  /* Otherwise i2c_read/i2c_write and other functions will fail to cast pointers properly */

    struct i2c_dev_s    dev;           /* Generic I2C device */
    wiced_i2c_device_t *wiced_i2c_dev; /* Wiced I2C device */
    bool                supports_repeated_start; /* In BCM4390X, The I2C peripheral pins need to be GPIO muxable */
                                                 /* to support repeated start */
    sem_t               mutex;         /* Only one thread can access at a time */
    struct i2c_msg_s   *msgs;          /* I2C Message(s) */
    unsigned int        nmsg;          /* number of transfer remaining */
    unsigned int        frequency;     /* I2C frequency */
    bool                initialized;   /* initialization flag */

};

static struct bcm4390x_i2cdev_s i2cdevices[WICED_I2C_MAX];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * I2C device operations
 ****************************************************************************/

static uint32_t i2c_setfrequency(struct i2c_dev_s *dev, uint32_t frequency);
static int      i2c_setaddress(struct i2c_dev_s *dev, int addr, int nbits);
static int      i2c_write(struct i2c_dev_s *dev, const uint8_t *buffer, int buflen);
static int      i2c_read(struct i2c_dev_s *dev, uint8_t *buffer, int buflen);
static int      i2c_transfer(struct i2c_dev_s *dev, struct i2c_msg_s *msgs, int count);

struct i2c_ops_s bcm4390x_i2c_ops =
{
  .setfrequency = i2c_setfrequency,
  .setaddress   = i2c_setaddress,
  .write        = i2c_write,
  .read         = i2c_read,
#ifdef CONFIG_I2C_TRANSFER
  .transfer     = i2c_transfer
#endif
};

/*******************************************************************************
 * Name: up_i2cinitialize
 *
 * Description:
 *   Initialize an I2C device
 *
 *******************************************************************************/

struct i2c_dev_s *up_i2cinitialize(int port)
{
  struct bcm4390x_i2cdev_s *priv = &i2cdevices[port];
  platform_i2c_t *i2c;

  if (priv->initialized)
  {
      return &priv->dev;
  }

  sem_init(&priv->mutex, 0, 1);

  priv->dev.ops = &bcm4390x_i2c_ops;

  /* Board Specific Init */
  bcm4390x_i2cinitialize(port, &(priv->wiced_i2c_dev));
  if (priv->wiced_i2c_dev == NULL)
  {
      return NULL;
  }

  i2c = &platform_i2c_peripherals[priv->wiced_i2c_dev->port];
  priv->supports_repeated_start = ( i2c->pin_scl == NULL || i2c->pin_sda == NULL ) ? FALSE : TRUE;

  if (priv->wiced_i2c_dev->speed_mode == I2C_STANDARD_SPEED_MODE)
  {
    priv->frequency = I2C_STANDARD_SPEED_FREQUENCY;
  }
  else if (priv->wiced_i2c_dev->speed_mode == I2C_LOW_SPEED_MODE)
  {
    priv->frequency = I2C_LOW_SPEED_FREQUENCY;
  }
  else if (priv->wiced_i2c_dev->speed_mode == I2C_HIGH_SPEED_MODE)
  {
    priv->frequency = I2C_HIGH_SPEED_FREQUENCY;
  }

  if (WICED_SUCCESS != wiced_i2c_init(priv->wiced_i2c_dev))
  {
    return NULL;
  }

  priv->initialized = TRUE;
  return &priv->dev;
}

/*******************************************************************************
 * Name: up_i2cuninitalize
 *
 * Description:
 *   Uninitialise an I2C device
 *
 *******************************************************************************/

int up_i2cuninitialize(struct i2c_dev_s *dev)
{
  struct bcm4390x_i2cdev_s *priv = (struct bcm4390x_i2cdev_s *) dev;

  sem_destroy(&priv->mutex);
  wiced_i2c_deinit(priv->wiced_i2c_dev);
  priv->initialized = FALSE;

  return OK;
}

/*******************************************************************************
 * Name: bcm4390x_i2c_setfrequency
 *
 * Description:
 *   Set the frequence for the next transfer
 *
 *******************************************************************************/

static uint32_t i2c_setfrequency(struct i2c_dev_s *dev, uint32_t frequency)
{
  struct bcm4390x_i2cdev_s *priv = (struct bcm4390x_i2cdev_s *) dev;
  uint32_t device_frequency;

  DEBUGASSERT(dev != NULL);
  device_frequency = priv->frequency;

  if (((frequency == I2C_STANDARD_SPEED_FREQUENCY) && (I2C_STANDARD_SPEED_MODE != priv->wiced_i2c_dev->speed_mode)) ||
      ((frequency == I2C_LOW_SPEED_FREQUENCY) && (I2C_LOW_SPEED_MODE != priv->wiced_i2c_dev->speed_mode)) ||
      ((frequency == I2C_HIGH_SPEED_FREQUENCY) && (I2C_HIGH_SPEED_MODE != priv->wiced_i2c_dev->speed_mode)) ||
      ((frequency != I2C_STANDARD_SPEED_FREQUENCY) && (frequency != I2C_LOW_SPEED_FREQUENCY) &&
        (frequency != I2C_HIGH_SPEED_FREQUENCY)))
  {
      dbg("failed to set i2c frequency: freq:%d, mode=%d", frequency, priv->wiced_i2c_dev->speed_mode);
      device_frequency = 0;
  }

  return device_frequency;
}

/*******************************************************************************
 * Name: bcm4390x_i2c_setaddress
 *
 * Description:
 *   Set the I2C slave address for a subsequent read/write
 *
 *******************************************************************************/

static int i2c_setaddress(struct i2c_dev_s *dev, int addr, int nbits)
{
  struct bcm4390x_i2cdev_s *priv = (struct bcm4390x_i2cdev_s *) dev;
  int ret = OK;

  DEBUGASSERT(dev != NULL);

  if (((nbits == 7) && (I2C_ADDRESS_WIDTH_7BIT != priv->wiced_i2c_dev->address_width)) ||
      ((nbits == 10) && (I2C_ADDRESS_WIDTH_10BIT != priv->wiced_i2c_dev->address_width)) ||
      ((nbits != 7) && (nbits != 10)))
  {
    dbg("failed to set i2c address: nbits=%d, address_width:%d", nbits, priv->wiced_i2c_dev->address_width);
    ret = ERROR;
  }

  return ret;
}

/*******************************************************************************
 * Name: bcm4390x_i2c_write
 *
 * Description:
 *   Send a block of data on I2C using the previously selected I2C
 *   frequency and slave address.
 *
 *******************************************************************************/

static int i2c_write(struct i2c_dev_s *dev, const uint8_t *buffer, int buflen)
{
  struct bcm4390x_i2cdev_s *priv = (struct bcm4390x_i2cdev_s *) dev;
  struct i2c_msg_s msg = {0};
  int ret = OK;

  DEBUGASSERT(dev != NULL);

  msg.flags &= ~I2C_M_READ;
  msg.flags |= I2C_M_NORESTART;
  msg.buffer = (uint8_t*)buffer;
  msg.length = buflen;

#ifdef CONFIG_I2C_TRANSFER
  UNUSED_VARIABLE(priv);
  ret = i2c_transfer(dev, &msg, 1);
#else
  if (WICED_SUCCESS != wiced_i2c_write(priv->wiced_i2c_dev, (WICED_I2C_START_FLAG | WICED_I2C_STOP_FLAG),
                 msg.buffer, msg.length))
  {
    ret = -EIO;
  }
#endif

  return ret;
}

/*******************************************************************************
 * Name: bcm4390x_i2c_read
 *
 * Description:
 *   Receive a block of data on I2C using the previously selected I2C
 *   frequency and slave address.
 *
 *******************************************************************************/

static int i2c_read(struct i2c_dev_s *dev, uint8_t *buffer, int buflen)
{
  struct bcm4390x_i2cdev_s *priv = (struct bcm4390x_i2cdev_s *) dev;
  int ret = OK;
  struct i2c_msg_s msg = {0};

  DEBUGASSERT(dev != NULL);

  msg.flags |= (I2C_M_READ | I2C_M_NORESTART);
  msg.buffer = buffer;
  msg.length = buflen;

#ifdef CONFIG_I2C_TRANSFER
  UNUSED_VARIABLE(priv);
  ret = i2c_transfer(dev, &msg, 1);
#else
  if (WICED_SUCCESS != wiced_i2c_read(priv->wiced_i2c_dev, (WICED_I2C_START_FLAG | WICED_I2C_STOP_FLAG),
                                     msg.buffer, msg.length))
  {
      ret = -EIO;
  }
#endif

  return ret;
}

/*******************************************************************************
 * Name: i2c_transfer
 *
 * Description:
 *   Perform a sequence of I2C transfers
 *
 *******************************************************************************/
#ifdef CONFIG_I2C_TRANSFER
static int i2c_transfer(struct i2c_dev_s *dev, struct i2c_msg_s *msgs, int count)
{
  struct bcm4390x_i2cdev_s *priv = (struct bcm4390x_i2cdev_s *) dev;
  int ret = OK;
  struct i2c_msg_s *msg;
  struct i2c_msg_s *nextmsg;
  uint32_t wiced_flags = 0;
  wiced_result_t result;

  sem_wait(&priv->mutex);

  priv->msgs  = msgs;
  priv->nmsg  = count;

  msg = priv->msgs;

  while (priv->nmsg > 0)
  {
    wiced_flags = 0;
    nextmsg = (priv->nmsg == 1) ? NULL : (msg + 1);

    /* If the I2C Port does not support repeated start *
     * OR
     * If last message in sequence
     */
    if ((priv->supports_repeated_start == FALSE) || (nextmsg == NULL))
    {
      wiced_flags |= WICED_I2C_STOP_FLAG;
    }
    /* Next message does not have a repeated-start */
    else if ((nextmsg->flags & I2C_M_NORESTART) != 0)
    {
      wiced_flags |= WICED_I2C_STOP_FLAG;
    }
    /* If Next message has different address than previous , No Repeated start*/
    else if (nextmsg->addr != msg->addr)
    {
      wiced_flags |= WICED_I2C_STOP_FLAG;
    }

    /* If the I2C Port does not support repeated start
     * OR
     * If I2C_M_NORESTART is set
     * OR
     * First message in the sequence
     */
    if ((priv->supports_repeated_start == FALSE) || (msg->flags & I2C_M_NORESTART) || (priv->nmsg == count))
    {
      wiced_flags |= WICED_I2C_START_FLAG;
    }
    else
    {
      wiced_flags |= WICED_I2C_REPEATED_START_FLAG;
    }

    if (msg->flags & I2C_M_READ)
    {
      result = wiced_i2c_read(priv->wiced_i2c_dev, wiced_flags, msg->buffer, msg->length);
    }
    else
    {
      result = wiced_i2c_write(priv->wiced_i2c_dev, wiced_flags, msg->buffer, msg->length);
    }

    if (result != WICED_SUCCESS)
    {
      result = -EIO;
      break;
    }

    msg = nextmsg;
    priv->nmsg--;
  }

  sem_post(&priv->mutex);
  return ret;
}
#endif /* CONFIG_I2C_TRANSFER */
