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
#include <sys/types.h>
#include <stdbool.h>
#include <nuttx/serial/serial.h>
#include "platform_peripheral.h"
#include "platform.h"
#include "wiced_platform.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct up_dev_s
{
  platform_uart_port_t  port;
  wiced_uart_t          wiced_uart_num;
  wiced_uart_config_t   wiced_uart_config;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int    up_setup(struct uart_dev_s *dev);
static void   up_shutdown(struct uart_dev_s *dev);
static int    up_attach(struct uart_dev_s *dev);
static void   up_detach(struct uart_dev_s *dev);
static int    up_ioctl(struct file *filep, int cmd, unsigned long arg);
static int    up_receive(struct uart_dev_s *dev, uint32_t *status);
static void   up_rxint(struct uart_dev_s *dev, bool enable);
static bool   up_rxavailable(struct uart_dev_s *dev);
static void   up_send(struct uart_dev_s *dev, int ch);
static void   up_txint(struct uart_dev_s *dev, bool enable);
static bool   up_txready(struct uart_dev_s *dev);
static bool   up_txempty(struct uart_dev_s *dev);

/****************************************************************************
 * Private Variables
 ****************************************************************************/

static const struct uart_ops_s g_uart_ops =
{
  .setup          = up_setup,
  .shutdown       = up_shutdown,
  .attach         = up_attach,
  .detach         = up_detach,
  .ioctl          = up_ioctl,
  .receive        = up_receive,
  .rxint          = up_rxint,
  .rxavailable    = up_rxavailable,
#ifdef CONFIG_SERIAL_IFLOWCONTROL
  .rxflowcontrol  = NULL,
#endif
  .send           = up_send,
  .txint          = up_txint,
  .txready        = up_txready,
  .txempty        = up_txempty,
};

/* I/O buffers */
#ifdef CONFIG_BCM4390X_UART0
static char g_uart0rxbuffer[CONFIG_UART0_TXBUFSIZE];
static char g_uart0txbuffer[CONFIG_UART0_RXBUFSIZE];

/* This describes the state of the BCM4390x uart0 port. */
static struct up_dev_s g_uart0priv =
{
  .port              = UART_SLOW,
  .wiced_uart_num    = WICED_UART_1,
  .wiced_uart_config =
  {
    .baud_rate       = CONFIG_UART0_BAUD,
    .data_width      = (CONFIG_UART0_BITS == 8) ? DATA_WIDTH_8BIT : DATA_WIDTH_7BIT,
    .parity          = CONFIG_UART0_PARITY,
    .stop_bits       = CONFIG_UART0_2STOP,
#if defined(CONFIG_UART0_IFLOWCONTROL) && defined(CONFIG_UART0_OFLOWCONTROL)
    .flow_control    = FLOW_CONTROL_CTS_RTS,
#elif defined(CONFIG_UART0_IFLOW_CONTROL)
    .flow_control    = FLOW_CONTROL_RTS,
#elif defined(CONFIG_UART0_OFLOW_CONTROL)
    .flow_control    = FLOW_CONTROL_CTS,
#else
    .flow_control    = FLOW_CONTROL_DISABLED,
#endif //defined(CONFIG_UART0_IFLOWCONTROL) && defined(CONFIG_UART0_OFLOWCONTROL)
  }
};

static uart_dev_t g_uart0port =
{
  .recv     =
  {
    .size   = CONFIG_UART0_RXBUFSIZE,
    .buffer = g_uart0rxbuffer,
  },
  .xmit     =
  {
    .size   = CONFIG_UART0_TXBUFSIZE,
    .buffer = g_uart0txbuffer,
   },
  .ops      = &g_uart_ops,
  .priv     = &g_uart0priv,
};
#endif //CONFIG_BCM4390x_UART0

#ifdef CONFIG_BCM4390X_UART1
static char g_uart1rxbuffer[CONFIG_UART1_TXBUFSIZE];
static char g_uart1txbuffer[CONFIG_UART1_RXBUFSIZE];

/* This describes the state of the BCM4390x uart1 port. */
static struct up_dev_s g_uart1priv =
{
  .port              = UART_FAST,
  .wiced_uart_num    = WICED_UART_2,
  .wiced_uart_config =
  {
    .baud_rate       = CONFIG_UART1_BAUD,
    .data_width      = (CONFIG_UART0_BITS == 8) ? DATA_WIDTH_8BIT : DATA_WIDTH_7BIT,
    .parity          = CONFIG_UART1_PARITY,
    .stop_bits       = CONFIG_UART1_2STOP,
#if defined(CONFIG_UART1_IFLOWCONTROL) && defined(CONFIG_UART1_OFLOWCONTROL)
    .flow_control    = FLOW_CONTROL_CTS_RTS,
#elif defined(CONFIG_UART1_IFLOW_CONTROL)
    .flow_control    = FLOW_CONTROL_RTS,
#elif defined(CONFIG_UART1_OFLOW_CONTROL)
    .flow_control    = FLOW_CONTROL_CTS,
#else
    .flow_control    = FLOW_CONTROL_DISABLED,
#endif //defined(CONFIG_UART1_IFLOWCONTROL) && defined(CONFIG_UART1_OFLOWCONTROL)
  }
};

static uart_dev_t g_uart1port =
{
  .recv     =
  {
    .size   = CONFIG_UART1_RXBUFSIZE,
    .buffer = g_uart1rxbuffer,
  },
  .xmit     =
  {
    .size   = CONFIG_UART1_TXBUFSIZE,
    .buffer = g_uart1txbuffer,
   },
  .ops      = &g_uart_ops,
  .priv     = &g_uart1priv,
};
#endif // CONFIG_BCM4390X_UART1

#if defined(CONFIG_UART0_SERIAL_CONSOLE) && defined(CONFIG_BCM4390X_UART0)
# define CONSOLE_DEV    g_uart0port
# undef UART1_SERIAL_CONSOLE
# define TTYS0_DEV      g_uart0port
# if defined(CONFIG_BCM4390X_UART1)
#    define TTYS1_DEV   g_uart1port
# else
#    undef TTYS1_DEV
# endif //if defined(CONFIG_BCM4390X_UART1)
#elif defined(CONFIG_UART1_SERIAL_CONSOLE) && defined (CONFIG_BCM4390X_UART1)
# define CONSOLE_DEV    g_uart1port
# undef UART0_SERIAL_CONSOLE
#define TTYS0_DEV       g_uart1port
# if defined (CONFIG_BCM4390X_UART0)
#    define TTYS1_DEV   g_uart0port
# else
#    undef TTYS1_DEV
# endif // defined(CONFIG_UART1_SERIAL_CONSOLE) && defined (CONFIG_BCM4390X_UART1)
#endif //defined(CONFIG_UART0_SERIAL_CONSOLE) && defined(CONFIG_BCM4390X_UART0)

/****************************************************************************
 * Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_setup
 *
 * Description:
 *   Configure the UART baud, bits, parity, fifos, etc. This
 *   method is called the first time that the serial port is
 *   opened.
 *
 ****************************************************************************/

static int up_setup(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;

  wiced_uart_init(priv->wiced_uart_num, &(priv->wiced_uart_config), NULL);
  return OK;
}

/****************************************************************************
 * Name: up_shutdown
 *
 * Description:
 *   Disable the UART.  This method is called when the serial
 *   port is closed
 *
 ****************************************************************************/

static void up_shutdown(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  platform_uart_toggle_txrx_interrupt (priv->port, FALSE, FALSE);
  platform_uart_toggle_txrx_interrupt (priv->port, TRUE, FALSE);
}

/****************************************************************************
 * Name: up_attach
 *
 * Description:
 *   Configure the UART to operation in interrupt driven mode.  This method is
 *   called when the serial port is opened.  Normally, this is just after the
 *   the setup() method is called, however, the serial console may operate in
 *   a non-interrupt driven mode during the boot phase.
 *
 *   RX and TX interrupts are not enabled when by the attach method (unless the
 *   hardware supports multiple levels of interrupt enabling).  The RX and TX
 *   interrupts are not enabled until the txint() and rxint() methods are called.
 *
 ****************************************************************************/

static int up_attach(struct uart_dev_s *dev)
{
  return OK;
}

/****************************************************************************
 * Name: up_detach
 *
 * Description:
 *   Detach UART interrupts.  This method is called when the serial port is
 *   closed normally just before the shutdown method is called.  The exception is
 *   the serial console which is never shutdown.
 *
 ****************************************************************************/

static void up_detach(struct uart_dev_s *dev)
{
}

/****************************************************************************
 * Name: up_serialinit
 *
 * Description:
 *   Performs the low level UART initialization early in
 *   debug so that the serial console will be available
 *   during bootup.  This must be called before up_serialinit.
 *
 ****************************************************************************/

void up_earlyserialinit(void)
{
  CONSOLE_DEV.isconsole = true;
  up_setup(&CONSOLE_DEV);
}

void up_serialinit(void)
{
#ifdef CONSOLE_DEV
  CONSOLE_DEV.isconsole = true;
  (void)uart_register("/dev/console", &CONSOLE_DEV);
#endif //CONSOLE_DEV
#ifdef TTYS0_DEV
  (void)uart_register("/dev/ttyS0", &TTYS0_DEV);
# ifdef TTYS1_DEV
  (void)uart_register("/dev/ttyS1", &TTYS1_DEV);
# endif //TTYS1_DEV
#endif //TTYS0_DEV
}

/****************************************************************************
 * Name: up_send
 *
 * Description:
 *   This method will send one byte on the UART
 *
 ****************************************************************************/

static void up_send(struct uart_dev_s *dev, int ch)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  wiced_uart_transmit_bytes(priv->wiced_uart_num, &ch, 1);
}


void platform_uart_irq(platform_uart_driver_t* driver)
{
  struct uart_dev_s *dev = NULL;
  struct up_dev_s *priv;
  uint32_t irq_status;

  if (driver->interface->port == UART_SLOW)
    {
      dev = &g_uart0port;
    }
  else if (driver->interface->port == UART_FAST)
    {
      dev = &g_uart1port;
    }

  priv = (struct up_dev_s*)dev->priv;

  irq_status = platform_uart_irq_txrx_ready(priv->port);

  if (irq_status & UART_RX_READY)
    {
      uart_recvchars(dev);
    }
  if (irq_status & UART_TX_READY)
    {
      uart_xmitchars(dev);
    }
}

static int up_ioctl (struct file *filep, int cmd, unsigned long arg)
{
  int ret = OK;

  return ret;
}

/****************************************************************************
 * Name: up_receive
 *
 * Description:
 *   Called (usually) from the interrupt level to receive one
 *   character from the UART.  Error bits associated with the
 *   receipt are provided in the return 'status'.
 *
 ****************************************************************************/

static int up_receive(struct uart_dev_s *dev, uint32_t *status)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;
  uint8_t ch;

  *status = 0;
  platform_uart_receive_byte(priv->port, &ch);
  return ch;
}

/****************************************************************************
 * Name: up_rxint
 *
 * Description:
 *   Call to enable or disable RX interrupts
 *
 ****************************************************************************/

static void up_rxint(struct uart_dev_s *dev, bool enable)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;

  platform_uart_toggle_txrx_interrupt (priv->port, FALSE, enable);
}

/****************************************************************************
 * Name: up_rxavailable
 *
 * Description:
 *   Return true if the receive fifo is not empty
 *
 ****************************************************************************/

static bool up_rxavailable(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;

  return platform_uart_txrx_ready (priv->port, FALSE);
}

/****************************************************************************
 * Name: up_txint
 *
 * Description:
 *   Call to enable or disable TX interrupts
 *
 ****************************************************************************/

static void up_txint(struct uart_dev_s *dev, bool enable)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;

  platform_uart_toggle_txrx_interrupt (priv->port, TRUE, enable);
}

/****************************************************************************
 * Name: up_txready
 *
 * Description:
 *   Return true if the tranmsit fifo is not full
 *
 ****************************************************************************/

static bool up_txready(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;

  return platform_uart_txrx_ready(priv->port, TRUE);
}

/****************************************************************************
 * Name: up_txempty
 *
 * Description:
 *   Return true if the transmit fifo is empty
 *
 ****************************************************************************/

static bool up_txempty(struct uart_dev_s *dev)
{
  struct up_dev_s *priv = (struct up_dev_s*)dev->priv;

  return platform_uart_transmit_fifo_empty (priv->port);
}

/****************************************************************************
 * Name: up_putc
 *
 * Description:
 *   Provide priority, low-level access to support OS debug  writes
 *
 ****************************************************************************/

int up_putc(int ch)
{
  up_send( &CONSOLE_DEV, ch);

  /* Check for LF */
  if (ch == '\n')
  {
    const int cr = '\r';
    /* Add CR */
    up_send( &CONSOLE_DEV, cr);
  }

  return ch;
}
