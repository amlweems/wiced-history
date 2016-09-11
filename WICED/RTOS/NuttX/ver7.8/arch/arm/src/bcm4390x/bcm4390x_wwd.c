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
#include <nuttx/wqueue.h>
#include <nuttx/irq.h>
#include <nuttx/net/net.h>
#include <nuttx/net/netdev.h>
#include <nuttx/net/arp.h>
#include <nuttx/net/mii.h>
#ifdef CONFIG_NET_PKT
#  include <nuttx/net/pkt.h>
#endif

#include <arch/chip/wifi.h>

#include <assert.h>
#include <debug.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sched.h>

#include "bcm4390x_wwd.h"

#ifndef CONFIG_NET_NOINTS
#  error WWD driver require NET_NOINTS
#endif

#ifndef CONFIG_NET_MULTIBUFFER
#  error WWD driver require NET_MULTIBUFFER
#endif

#ifndef CONFIG_NET_TCP_WRITE_BUFFERS
#  error WWD driver require NET_TCP_WRITE_BUFFERS
#endif

#ifndef CONFIG_BCM4390X_WWD_POLLHSEC
#  define CONFIG_BCM4390X_WWD_POLLHSEC (1) /* 0.5sec */
#endif

#ifndef CONFIG_BCM4390X_WWD_WDDELAY
#  define CONFIG_BCM4390X_WWD_WDDELAY (CONFIG_BCM4390X_WWD_POLLHSEC * CLK_TCK / 2)
#endif

#ifndef CONFIG_BCM4390X_WWD_TX_WORKQUEUE
#  ifndef CONFIG_SCHED_HPWORK
#    error High priority work queue support is required
#  else
#    define CONFIG_BCM4390X_WWD_TX_WORKQUEUE HPWORK
#  endif
#endif

#ifndef CONFIG_BCM4390X_WWD_RX_WORKQUEUE
#  ifndef CONFIG_SCHED_HPWORK
#    error High priority work queue support is required
#  else
#    define CONFIG_BCM4390X_WWD_RX_WORKQUEUE HPWORK
#  endif
#endif

#ifdef CONFIG_NETDEV_PHY_IOCTL
#ifndef CONFIG_BCM4390X_WWD_LINK_EVENT_SUBSCRIBERS_NUM
#  define CONFIG_BCM4390X_WWD_LINK_EVENT_SUBSCRIBERS_NUM 2
#endif
#if !CONFIG_BCM4390X_WWD_LINK_EVENT_SUBSCRIBERS_NUM
#  error Must be at least 1
#endif
#endif

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define FOREACH_BCM4390X_WWD_DRIVER(i) for ((i) = 0; (i) < ARRAY_SIZE(bcm4390x_wwd_drivers); (i)++)

#define FOREACH_BCM4390X_WWD_DRIVER_LINK_EVENT_SUBSCRIBER(priv, i)  for ((i) = 0; (i) < ARRAY_SIZE((priv)->link_event_subscribers); (i)++)

#define BUF ((struct eth_hdr_s *)priv->dev.d_buf)

struct bcm4390x_wwd_link_event_subscriber_s
{
  bool  assigned;
  pid_t pid;
  int   signo;
  void* arg;
};

struct bcm4390x_wwd_driver_s
{
  struct net_driver_s dev;
  wiced_interface_t   interface;
  const char*         name;
  bool                tx_enable;
  bool                link_up;
  bool                if_register;
  bool                if_up;
  wiced_buffer_t      curr_tx_buffer;
  wiced_buffer_fifo_t rx_fifo;
  struct work_s       tx_work;
  struct work_s       rx_work;
  struct work_s       timer_work;
#ifdef CONFIG_NETDEV_PHY_IOCTL
  struct bcm4390x_wwd_link_event_subscriber_s link_event_subscribers[CONFIG_BCM4390X_WWD_LINK_EVENT_SUBSCRIBERS_NUM];
#endif /* CONFIG_NETDEV_PHY_IOCTL */
};

static uint32_t bcm4390x_ww_tx_buffer_freeing_counter = 0;

static sem_t bcm4390x_wwd_event_sem = SEM_INITIALIZER(1);

static void bcm4390x_wwd_timer_work(void *arg);

static void bcm4390x_wwd_get_event_sem(void)
{
  while (sem_wait(&bcm4390x_wwd_event_sem) != OK)
  {
    DEBUGASSERT(errno == EINTR);
  }
}

static void bcm4390x_wwd_set_event_sem(void)
{
  if (sem_post(&bcm4390x_wwd_event_sem) != OK)
  {
    DEBUGASSERT(0);
  }
}

static void bcm4390x_wwd_atomic_inc(uint32_t *var)
{
  irqstate_t flags = irqsave();

  *var = *var + 1;

  irqrestore(flags);
}

static bool bcm4390x_wwd_atomic_dec(uint32_t *var, uint32_t min_value)
{
  bool ret = false;
  irqstate_t flags = irqsave();

  if (*var > min_value)
  {
    *var = *var - 1;
    ret = true;
  }

  irqrestore(flags);

  return ret;
}

static void bcm4390x_wwd_dump_dev(struct net_driver_s *dev, const char *prefix)
{
#ifdef CONFIG_NET_IPv4
  ndbg("%s: %d.%d.%d.%d\n",
       prefix,
       dev->d_ipaddr & 0xff, (dev->d_ipaddr >> 8) & 0xff,
       (dev->d_ipaddr >> 16) & 0xff, dev->d_ipaddr >> 24);
#endif /* CONFIG_NET_IPv4 */
#ifdef CONFIG_NET_IPv6
  ndbg("%s: %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",
       prefix,
       dev->d_ipv6addr[0], dev->d_ipv6addr[1], dev->d_ipv6addr[2],
       dev->d_ipv6addr[3], dev->d_ipv6addr[4], dev->d_ipv6addr[5],
       dev->d_ipv6addr[6], dev->d_ipv6addr[7]);
#endif /* CONFIG_NET_IPv6 */
}

static void bcm4390x_wwd_free_curr_tx_buffer(struct bcm4390x_wwd_driver_s *priv)
{
  bcm4390x_wwd_atomic_inc(&bcm4390x_ww_tx_buffer_freeing_counter);

  bcm4390x_wwd_free_tx_buffer(priv->curr_tx_buffer);
  priv->curr_tx_buffer = NULL;
}

#if defined(CONFIG_NET_IGMP) || defined(CONFIG_NET_ICMPv6)
static int bcm4390x_wwd_addmac(struct net_driver_s *dev, const uint8_t *mac)
{
  struct bcm4390x_wwd_driver_s *priv = (struct bcm4390x_wwd_driver_s *)dev->d_private;
  return bcm4390x_wwd_register_multicast_address(priv->interface, mac);
}

static int bcm4390x_wwd_rmmac(struct net_driver_s *dev, const uint8_t *mac)
{
  struct bcm4390x_wwd_driver_s *priv = (struct bcm4390x_wwd_driver_s *)dev->d_private;
  return bcm4390x_wwd_unregister_multicast_address(priv->interface, mac);
}
#endif /* CONFIG_NET_IGMP || CONFIG_NET_ICMPv6 */

static int bcm4390x_wwd_ipv6multicast(struct bcm4390x_wwd_driver_s *priv, bool add)
{
#ifdef CONFIG_NET_ICMPv6
  struct net_driver_s *dev = &priv->dev;
  uint16_t tmp16;
  uint8_t mac[6];
  int result;
  unsigned i;

  /* Check whether IPv6 address is already assigned or not. */
  for (i = 0; i < ARRAY_SIZE(dev->d_ipv6addr); i++)
  {
    if (dev->d_ipv6addr[i] != 0)
    {
      break;
    }
  }
  if (i == ARRAY_SIZE(dev->d_ipv6addr))
  {
    /* Address not assigned yet, skip multicast adding */
    return OK;
  }

  /*
   * For ICMPv6, we need to add the IPv6 multicast address.
   *
   * For IPv6 multicast addresses, the Ethernet MAC is derived by
   * the four low-order octets OR'ed with the MAC 33:33:00:00:00:00,
   * so for example the IPv6 address FF02:DEAD:BEEF::1:3 would map
   * to the Ethernet MAC address 33:33:00:01:00:03.
   *
   * NOTES:  This appears correct for the ICMPv6 Router Solicitation
   * Message, but the ICMPv6 Neighbor Solicitation message seems to
   * use 33:33:ff:01:00:03.
   */

  mac[0] = 0x33;
  mac[1] = 0x33;

  tmp16  = dev->d_ipv6addr[6];
  mac[2] = 0xff;
  mac[3] = tmp16 >> 8;

  tmp16  = dev->d_ipv6addr[7];
  mac[4] = tmp16 & 0xff;
  mac[5] = tmp16 >> 8;

  nvdbg("IPv6 Multicast: %02x:%02x:%02x:%02x:%02x:%02x\n",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  result = add ? bcm4390x_wwd_addmac(dev, mac) : bcm4390x_wwd_rmmac(dev, mac);
  if (result != OK)
  {
    return result;
  }

#ifdef CONFIG_NET_ICMPv6_AUTOCONF
  /*
   * Add the IPv6 all link-local nodes Ethernet address.  This is the
   * address that we expect to receive ICMPv6 Router Advertisement
   * packets.
   */
  result = add ? bcm4390x_wwd_addmac(dev, g_ipv6_ethallnodes.ether_addr_octet) : bcm4390x_wwd_rmmac(dev, g_ipv6_ethallnodes.ether_addr_octet);
  if (result != OK)
  {
    return result;
  }
#endif /* CONFIG_NET_ICMPv6_AUTOCONF */

#ifdef CONFIG_NET_ICMPv6_ROUTER
  /*
   * Add the IPv6 all link-local routers Ethernet address.  This is the
   * address that we expect to receive ICMPv6 Router Solicitation
   * packets.
   */
  result = add ? bcm4390x_wwd_addmac(dev, g_ipv6_ethallrouters.ether_addr_octet) : bcm4390x_wwd_rmmac(dev, g_ipv6_ethallrouters.ether_addr_octet);
  if (result != OK)
  {
    return result;
  }
#endif /* CONFIG_NET_ICMPv6_ROUTER */
#else
  UNUSED_PARAMETER(priv);
  UNUSED_PARAMETER(add);
#endif /* CONFIG_NET_ICMPv6 */

  return OK;
}

static void bcm4390x_wwd_prepare_ethernet_data(struct net_driver_s *dev)
{
  /*
   * This function should be called before sending out an IP packet.
   * It checks destination IP address and create ethernet header using ARP cache.
   * If no ARP cache entry is found for the destination IP address, the
   * packet in the d_buf[] is replaced by an ARP request packet for the
   * IP address. The IP packet is dropped and it is assumed that the
   * higher level protocols (e.g., TCP) eventually will retransmit the
   * dropped packet.
   */
#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
  if (IFF_IS_IPv4(dev->d_flags))
#endif /* CONFIG_NET_IPv6 */
  {
    arp_out(dev);
  }
#endif /* CONFIG_NET_IPv4 */
#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
  else
#endif /* CONFIG_NET_IPv4 */
  {
    neighbor_out(dev);
  }
#endif /* CONFIG_NET_IPv6 */
}

static int bcm4390x_wwd_txpoll(struct net_driver_s *dev)
{
  struct bcm4390x_wwd_driver_s *priv = (struct bcm4390x_wwd_driver_s *)dev->d_private;
  int result = 0;

  DEBUGASSERT(priv->dev.d_buf != NULL);

  /*
   * If the polling resulted in data that should be sent out on the network,
   * the field d_len is set to a value > 0.
   */
  if (priv->dev.d_len > 0)
  {
    /* Prepare and send ethernet packet */
    bcm4390x_wwd_prepare_ethernet_data(dev);
    bcm4390x_wwd_send_ethernet_data(priv->interface, priv->curr_tx_buffer, dev->d_len, priv->tx_enable);

    /* Allocate a new buffer for the pool */
    priv->curr_tx_buffer = NULL;
    dev->d_len = 0;
    dev->d_buf = bcm4390x_wwd_alloc_tx_buffer(&priv->curr_tx_buffer);
    if (!dev->d_buf)
    {
      result = -ENOMEM;
    }
  }

  return result;
}

static int bcm4390x_wwd_schedule_work(struct bcm4390x_wwd_driver_s *priv, int qid, struct work_s *work, worker_t worker, uint32_t delay)
{
  int result = OK;

  sched_lock();

  do
  {
    int op_result;

    op_result = work_cancel(qid, work);
    if ((op_result != OK) && (op_result != -ENOENT))
    {
      result = op_result;
      break;
    }

    result = work_queue(qid, work, worker, priv, delay);
    if (result != OK)
    {
      break;
    }
  }
  while (0);

  sched_unlock();

  return result;
}

static void bcm4390x_wwd_devif_txpoll(struct bcm4390x_wwd_driver_s *priv, int hsec)
{
  struct net_driver_s *dev = &priv->dev;
  net_lock_t state;

  state = net_lock();

  if (priv->if_up)
  {
    dev->d_buf = bcm4390x_wwd_alloc_tx_buffer(&priv->curr_tx_buffer);
    if (dev->d_buf)
    {
      if (hsec >= 0)
      {
        (void)devif_timer(dev, bcm4390x_wwd_txpoll, hsec);
      }
      else
      {
        (void)devif_poll(dev, bcm4390x_wwd_txpoll);
      }

      if (priv->curr_tx_buffer)
      {
        DEBUGASSERT(dev->d_len == 0);
        bcm4390x_wwd_free_curr_tx_buffer(priv);
        dev->d_buf = NULL;
      }
    }

    if (hsec >= 0)
    {
      int result = bcm4390x_wwd_schedule_work(priv, CONFIG_BCM4390X_WWD_TX_WORKQUEUE, &priv->timer_work, bcm4390x_wwd_timer_work, CONFIG_BCM4390X_WWD_WDDELAY);
      DEBUGASSERT(result == OK);
    }
  }

  net_unlock(state);
}

static void bcm4390x_wwd_txavail_work(void *arg)
{
  bcm4390x_wwd_devif_txpoll(arg, -1);
}

static void bcm4390x_wwd_timer_work(void *arg)
{
  bcm4390x_wwd_devif_txpoll(arg, CONFIG_BCM4390X_WWD_POLLHSEC);
}

static int bcm4390x_wwd_txavail(struct net_driver_s *dev)
{
  struct bcm4390x_wwd_driver_s *priv = (struct bcm4390x_wwd_driver_s *)dev->d_private;
  return bcm4390x_wwd_schedule_work(priv, CONFIG_BCM4390X_WWD_TX_WORKQUEUE, &priv->tx_work, bcm4390x_wwd_txavail_work, 0);
}

static int bcm4390x_wwd_ifup(struct net_driver_s *dev)
{
  struct bcm4390x_wwd_driver_s *priv = (struct bcm4390x_wwd_driver_s *)dev->d_private;
  net_lock_t state;
  int result;

  DEBUGASSERT(priv->if_register);

  state = net_lock();

  bcm4390x_wwd_dump_dev(dev, "Bringing up");

  do
  {
    /* Set up IPv6 multicast address filtering */
    result = bcm4390x_wwd_ipv6multicast(priv, true);
    if (result != OK)
    {
      break;
    }

    /* Start periodic timer */
    result = bcm4390x_wwd_schedule_work(priv, CONFIG_BCM4390X_WWD_TX_WORKQUEUE, &priv->timer_work, bcm4390x_wwd_timer_work, CONFIG_BCM4390X_WWD_WDDELAY);
    if (result != OK)
    {
      int op_result = bcm4390x_wwd_ipv6multicast(priv, false);
      DEBUGASSERT(op_result == OK);
      break;
    }

    priv->if_up = true;
  }
  while (0);

  net_unlock(state);

  return result;
}

static int bcm4390x_wwd_ifdown(struct net_driver_s *dev)
{
  struct bcm4390x_wwd_driver_s *priv = (struct bcm4390x_wwd_driver_s *)dev->d_private;
  net_lock_t state;

  DEBUGASSERT(priv->if_register);

  state = net_lock();

  priv->if_up = false;

  net_unlock(state);

  return OK;
}

static void bcm4390x_wwd_receive(struct bcm4390x_wwd_driver_s *priv, wiced_buffer_t buffer)
{
  struct net_driver_s *dev = &priv->dev;
  bool consumed = false;
  bool transmit = false;
  bool create_eth = true;
  uint16_t len = 0;
  uint8_t *data = bcm4390x_wwd_get_buffer_data(buffer, &len);

  /* Assign packet to internal structure to be used by network stack */
  dev->d_len = len;
  dev->d_buf = data;

#ifdef CONFIG_NET_PKT
  /* When packet sockets are enabled, feed the frame into the packet tap */
  pkt_input(&priv->dev);
#endif /* CONFIG_NET_PKT */

  if (dev->d_len < sizeof(struct eth_hdr_s))
  {
    nlldbg("DROPPED: Too small: %d\n", dev->d_len);
  }
  else if (dev->d_len > CONFIG_NET_ETH_MTU)
  {
    nlldbg("DROPPED: Too big: %d\n", dev->d_len);
  }
  else
#ifdef CONFIG_NET_IPv4
  if (BUF->type == HTONS(ETHTYPE_IP))
  {
    nllvdbg("IPv4 frame\n");

    /* Handle ARP on input then give the IPv4 packet to the network layer */
    arp_ipin(dev);
    ipv4_input(dev);

    /* Schedule ethernet packet creating and than sending */
    transmit = true;
  }
  else
#endif /* CONFIG_NET_IPv4 */
#ifdef CONFIG_NET_IPv6
  if (BUF->type == HTONS(ETHTYPE_IP6))
  {
    nllvdbg("Iv6 frame\n");

    /* Give the IPv6 packet to the network layer */
    ipv6_input(dev);

    /* Schedule ethernet packet creating and than sending */
    transmit = true;
  }
  else
#endif /* CONFIG_NET_IPv6 */
#ifdef CONFIG_NET_ARP
  if (BUF->type == htons(ETHTYPE_ARP))
  {
    nllvdbg("ARP frame\n");

    /* Handle ARP packet */
    arp_arpin(dev);

    /* Schedule packet sending (ethernet packet is already formed) */
    transmit = true;
    create_eth = false;
  }
  else
#endif /* CONFIG_NET_ARP */
  {
    nlldbg("DROPPED: Unknown type: %04x\n", BUF->type);
  }

  /*
   * Transmit may be expected due to network stack function call.
   * If call resulted in data that should be sent out on the network,
   * the field d_len will set to a value > 0.
   */
  if ((dev->d_len > 0) && transmit)
  {
    if (create_eth)
    {
      bcm4390x_wwd_prepare_ethernet_data(dev);
    }

    bcm4390x_wwd_send_ethernet_data(priv->interface, buffer, dev->d_len, priv->tx_enable);
    consumed = true;
  }

  /* Free the receive packet buffer */
  dev->d_buf = NULL;
  dev->d_len = 0;
  if (!consumed)
  {
    bcm4390x_wwd_free_rx_buffer(buffer);
  }
}

static void bcm4390x_wwd_receive_work(void *arg)
{
  struct bcm4390x_wwd_driver_s *priv = arg;
  struct net_driver_s *dev = &priv->dev;
  net_lock_t state;

  state = net_lock();

  while (true)
  {
    wiced_buffer_t buffer = bcm4390x_wwd_buffer_pop_from_fifo(&priv->rx_fifo);

    if (!buffer)
    {
      break;
    }

    if (priv->if_up)
    {
      bcm4390x_wwd_receive(priv, buffer);
    }
    else
    {
      nlldbg("DROPPED: not up\n");
      bcm4390x_wwd_free_rx_buffer(buffer);
    }
  }

  net_unlock(state);

  dev->d_txavail(dev);
}

#ifdef CONFIG_NETDEV_PHY_IOCTL
static struct bcm4390x_wwd_link_event_subscriber_s* bcm4390x_wwd_find_assigned_link_event_subscriber(struct bcm4390x_wwd_driver_s *priv, pid_t pid)
{
  unsigned subscriber_i;

  FOREACH_BCM4390X_WWD_DRIVER_LINK_EVENT_SUBSCRIBER(priv, subscriber_i)
  {
    struct bcm4390x_wwd_link_event_subscriber_s *subscriber = &priv->link_event_subscribers[subscriber_i];

    if (subscriber->assigned && (subscriber->pid == pid))
    {
      return subscriber;
    }
  }

  return NULL;
}

static struct bcm4390x_wwd_link_event_subscriber_s* bcm4390x_wwd_find_unassigned_link_event_subscriber(struct bcm4390x_wwd_driver_s *priv)
{
  unsigned subscriber_i;

  FOREACH_BCM4390X_WWD_DRIVER_LINK_EVENT_SUBSCRIBER(priv, subscriber_i)
  {
    struct bcm4390x_wwd_link_event_subscriber_s *subscriber = &priv->link_event_subscribers[subscriber_i];

    if (!subscriber->assigned)
    {
      return subscriber;
    }
  }

  return NULL;
}

static void bcm4390x_wwd_signal_link_event_subscribers(struct bcm4390x_wwd_driver_s *priv)
{
  unsigned subscriber_i;

  FOREACH_BCM4390X_WWD_DRIVER_LINK_EVENT_SUBSCRIBER(priv, subscriber_i)
  {
    struct bcm4390x_wwd_link_event_subscriber_s *subscriber = &priv->link_event_subscribers[subscriber_i];

    if (subscriber->assigned)
    {
      int ret;

#ifdef CONFIG_CAN_PASS_STRUCTS
      union sigval value;
      value.sival_ptr = subscriber->arg;
      ret = sigqueue(subscriber->pid, subscriber->signo, value);
#else
      ret = sigqueue(subscriber->pid, subscriber->signo, subscriber->arg);
#endif /* CONFIG_CAN_PASS_STRUCTS */

      DEBUGASSERT(ret == OK);
    }
  }
}

static int bcm4390x_wwd_ioctl(struct net_driver_s *dev, int cmd, long arg)
{
  struct bcm4390x_wwd_driver_s *priv = (struct bcm4390x_wwd_driver_s *)dev->d_private;
  int ret = -EINVAL;

  bcm4390x_wwd_get_event_sem();

  if (!priv->if_register)
  {
    bcm4390x_wwd_set_event_sem();
    DEBUGASSERT(0);
    return -EPERM;
  }

  switch (cmd)
  {
    case SIOCMIINOTIFY: /* Set up for PHY event notifications */
    {
      /*
       * NuttX has typo in mii_iotcl_notify_s name, need to check whether this is fixed in newer release.
       * Here we subscribe to event. There is no unsubscribe support in NuttX. Let's use -1 signal number to unsubscribe.
       */

      struct mii_iotcl_notify_s *req = (struct mii_iotcl_notify_s *)((uintptr_t)arg);
      pid_t  pid = (req->pid == 0) ? getpid() : req->pid;

      if (req->signo == -1)
      {
        /* Unsubscribe */
        struct bcm4390x_wwd_link_event_subscriber_s *subscriber = bcm4390x_wwd_find_assigned_link_event_subscriber(priv, pid);
        if (!subscriber)
        {
          break;
        }
        memset(subscriber, 0, sizeof(*subscriber));
        ret = OK;
      }
      else
      {
        /* Subscribe */
        struct bcm4390x_wwd_link_event_subscriber_s *subscriber = bcm4390x_wwd_find_assigned_link_event_subscriber(priv, pid);
        if (!subscriber)
        {
          subscriber = bcm4390x_wwd_find_unassigned_link_event_subscriber(priv);
        }
        if (!subscriber)
        {
          break;
        }
        subscriber->assigned = true;
        subscriber->pid = pid;
        subscriber->signo = req->signo;
        subscriber->arg = req->arg;
        ret = OK;
      }
    }
    break;

    case SIOCGMIIPHY: /* Get MII PHY address */
    {
      struct mii_ioctl_data_s *req = (struct mii_ioctl_data_s *)((uintptr_t)arg);
      req->phy_id = priv->interface;
      ret = OK;
    }
    break;

    case SIOCGMIIREG: /* Get register from MII PHY */
    {
      struct mii_ioctl_data_s *req = (struct mii_ioctl_data_s *)((uintptr_t)arg);
      if ((req->phy_id == priv->interface) && (req->reg_num == MII_MSR))
      {
        req->val_out = 0;
        if (priv->link_up)
        {
          req->val_out |= MII_MSR_LINKSTATUS;
        }
        ret = OK;
      }
    }
    break;

  case SIOCSMIIREG: /* Set register in MII PHY */
    ret = -EINVAL;
    break;

  default:
    ret = -ENOTTY;
    break;
  }

  bcm4390x_wwd_set_event_sem();

  return ret;
}
#endif /* CONFIG_NETDEV_PHY_IOCTL */

static struct bcm4390x_wwd_driver_s bcm4390x_wwd_sta_driver =
{
  .dev =
  {
    .d_ifup    = bcm4390x_wwd_ifup,
    .d_ifdown  = bcm4390x_wwd_ifdown,
    .d_txavail = bcm4390x_wwd_txavail,
#ifdef CONFIG_NET_IGMP
    .d_addmac  = bcm4390x_wwd_addmac,
    .d_rmmac   = bcm4390x_wwd_rmmac,
#endif /* CONFIG_NET_IGMP */
#ifdef CONFIG_NETDEV_PHY_IOCTL
    .d_ioctl   = bcm4390x_wwd_ioctl,
#endif /* CONFIG_NETDEV_PHY_IOCTL */
    .d_private = &bcm4390x_wwd_sta_driver,
  },
  .interface   = WICED_STA_INTERFACE,
  .name        = "sta",
  .tx_enable   = true,
  .link_up     = true
};

static struct bcm4390x_wwd_driver_s bcm4390x_wwd_ap_driver =
{
  .dev =
  {
    .d_ifup    = bcm4390x_wwd_ifup,
    .d_ifdown  = bcm4390x_wwd_ifdown,
    .d_txavail = bcm4390x_wwd_txavail,
#ifdef CONFIG_NET_IGMP
    .d_addmac  = bcm4390x_wwd_addmac,
    .d_rmmac   = bcm4390x_wwd_rmmac,
#endif /* CONFIG_NET_IGMP */
    .d_private = &bcm4390x_wwd_ap_driver,
  },
  .interface   = WICED_AP_INTERFACE,
  .name        = "ap",
  .tx_enable   = true,
  .link_up     = true
};

static struct bcm4390x_wwd_driver_s* bcm4390x_wwd_drivers[] =
{
  &bcm4390x_wwd_sta_driver,
  &bcm4390x_wwd_ap_driver,
};

void host_platform_bus_buffer_freed_nuttx(int tx_pool)
{
  if (!tx_pool)
  {
    return;
  }

  /*
   * Freeing can come from WICED WWD thread or from this driver.
   * TCP stack need to be kicked if freeing comes from WICED WWD thread and must not when from this driver
   * (otherwise it will kick again and again endlessly and consume all CPU time).
   * Atomically manipulating bcm4390x_ww_tx_buffer_freeing_counter ensure this.
   */

  if (!bcm4390x_wwd_atomic_dec(&bcm4390x_ww_tx_buffer_freeing_counter, 0))
  {
    unsigned dev_i;

    FOREACH_BCM4390X_WWD_DRIVER(dev_i)
    {
      struct net_driver_s *dev = &bcm4390x_wwd_drivers[dev_i]->dev;
      dev->d_txavail(dev);
    }
  }
}

static struct bcm4390x_wwd_driver_s* bcm4390x_wwd_find_driver_by_name(const char *driver_name)
{
  unsigned dev_i;

  FOREACH_BCM4390X_WWD_DRIVER(dev_i)
  {
    struct bcm4390x_wwd_driver_s *priv = bcm4390x_wwd_drivers[dev_i];

    if (!strcmp(priv->name, driver_name))
    {
      return priv;
    }
  }

  return NULL;
}

static struct bcm4390x_wwd_driver_s* bcm4390x_wwd_find_driver_by_interface(wiced_interface_t interface)
{
  unsigned dev_i;

  FOREACH_BCM4390X_WWD_DRIVER(dev_i)
  {
    struct bcm4390x_wwd_driver_s *priv = bcm4390x_wwd_drivers[dev_i];

    if (priv->interface == interface)
    {
      return priv;
    }
  }

  return NULL;
}

void bcm4390x_wwd_link_event_handler(wiced_interface_t interface, bcm4390x_wwd_link_event_t event)
{
  /*
   * Current link event handler called from WICED thread.
   * Events are ignored if network device driver is not registered yet.
   * It is up to WICED to make sure events are passed after the network device registered.
   * Unfortunately this is not the case, and other network stacks has same flaw.
   */

  struct bcm4390x_wwd_driver_s *priv = bcm4390x_wwd_find_driver_by_interface(interface);

  if (!priv)
  {
    DEBUGASSERT(0);
    return;
  }

  ndbg("Link notification %d for interface %d\n", event, interface);

  bcm4390x_wwd_get_event_sem();

  if (priv->if_register)
  {
    bool notify = false;

    switch (event)
    {
      case BCM4390X_WWD_LINK_EVENT_UP:
        priv->tx_enable = true;
        break;

      case BCM4390X_WWD_LINK_EVENT_DOWN:
        priv->tx_enable = false;
        break;

      case BCM4390X_WWD_LINK_EVENT_WIRELESS_UP:
        priv->link_up = true;
        notify = true;
        break;

      case BCM4390X_WWD_LINK_EVENT_WIRELESS_DOWN:
        priv->link_up = false;
        notify = true;
        break;

      case BCM4390X_WWD_LINK_EVENT_WIRELESS_RENEW:
        notify = true;
        break;

      default:
        DEBUGASSERT(0);
        break;
    }

    if (notify)
    {
#ifdef CONFIG_NETDEV_PHY_IOCTL
      bcm4390x_wwd_signal_link_event_subscribers(priv);
#endif /* CONFIG_NETDEV_PHY_IOCTL */
      ndbg("PHY state changed, subscribers notified\n");
    }
  }

  bcm4390x_wwd_set_event_sem();
}

void bcm4390x_wwd_rxavail(wiced_interface_t interface, wiced_buffer_t buffer)
{
  struct bcm4390x_wwd_driver_s *priv = bcm4390x_wwd_find_driver_by_interface(interface);

  if (!priv)
  {
    DEBUGASSERT(0);
    nlldbg("DROPPED: cannot push buffer for further processing\n");
    bcm4390x_wwd_free_rx_buffer(buffer);
  }
  else
  {
    int result;

    bcm4390x_wwd_buffer_push_to_fifo(&priv->rx_fifo, buffer);

    result = bcm4390x_wwd_schedule_work(priv, CONFIG_BCM4390X_WWD_RX_WORKQUEUE, &priv->rx_work, bcm4390x_wwd_receive_work, 0);
    DEBUGASSERT(result == OK);
  }
}

static int bcm4390x_wwd_netdev_register(struct bcm4390x_wwd_driver_s *priv)
{
  struct net_driver_s *dev = &priv->dev;
  int result = -EPERM;

  DEBUGASSERT(!priv->if_up);

  bcm4390x_wwd_get_event_sem();

  do
  {
    if (priv->if_register)
    {
      break;
    }

    result = bcm4390x_wwd_get_interface_mac_address(priv->interface, &dev->d_mac.ether_addr_octet[0]);
    if (result != OK)
    {
      break;
    }

    result = netdev_register(dev, NET_LL_ETHERNET);
    if (result != OK)
    {
      break;
    }

#ifdef CONFIG_NETDEV_PHY_IOCTL
    memset(priv->link_event_subscribers, 0, sizeof(priv->link_event_subscribers));
#endif /* CONFIG_NETDEV_PHY_IOCTL */
    priv->tx_enable = true;
    priv->link_up = true;
    priv->if_register = true;
  }
  while (0);

  if (result != OK)
  {
    memset(&dev->d_mac.ether_addr_octet[0], 0, sizeof(dev->d_mac.ether_addr_octet));
  }

  bcm4390x_wwd_set_event_sem();

  return result;
}

static int bcm4390x_wwd_netdev_unregister(struct bcm4390x_wwd_driver_s *priv)
{
  struct net_driver_s *dev = &priv->dev;
  int result = -EPERM;

  DEBUGASSERT(!priv->if_up);

  bcm4390x_wwd_get_event_sem();

  do
  {
    if (!priv->if_register)
    {
      break;
    }

    result = netdev_unregister(dev);
    if (result != OK)
    {
      break;
    }

    memset(&dev->d_mac.ether_addr_octet[0], 0, sizeof(dev->d_mac.ether_addr_octet));

    priv->if_register = false;

    result = OK;
  }
  while (0);

  bcm4390x_wwd_set_event_sem();

  return result;
}

static void bcm4390x_wwd_wlan_prepare_interfaces(void)
{
  static bool prepared = false;
  unsigned dev_i;

  if (prepared)
  {
    return;
  }

  FOREACH_BCM4390X_WWD_DRIVER(dev_i)
  {
    struct bcm4390x_wwd_driver_s *priv = bcm4390x_wwd_drivers[dev_i];
    bcm4390x_wwd_buffer_init_fifo(&priv->rx_fifo);
  }

  prepared = true;
}

static void bcm4390x_wwd_wlan_down_all(void)
{
  unsigned dev_i;

  FOREACH_BCM4390X_WWD_DRIVER(dev_i)
  {
    struct bcm4390x_wwd_driver_s *priv = bcm4390x_wwd_drivers[dev_i];
    int result = bcm4390x_wwd_wlan_down(priv->interface);
    DEBUGASSERT(result == OK);
  }
}

int bcm4390x_wwd_init(void)
{
  int result;

  bcm4390x_wwd_wlan_prepare_interfaces();

  result = bcm4390x_wwd_wlan_init();
  if (result != OK)
  {
    return result;
  }

  return OK;
}

void bcm4390x_wwd_deinit(void)
{
  int result;

  bcm4390x_wwd_wlan_down_all();

  result = bcm4390x_wwd_wlan_deinit();
  DEBUGASSERT(result == OK);
}

int wifi_driver_up_by_name(const char *driver_name)
{
  struct bcm4390x_wwd_driver_s *priv = bcm4390x_wwd_find_driver_by_name(driver_name);

  if (!priv)
  {
    return -EINVAL;
  }

  return bcm4390x_wwd_wlan_up(priv->interface);
}

int wifi_driver_down_by_name(const char *driver_name)
{
  struct bcm4390x_wwd_driver_s *priv = bcm4390x_wwd_find_driver_by_name(driver_name);

  if (!priv)
  {
    return -EINVAL;
  }

  return bcm4390x_wwd_wlan_down(priv->interface);
}

int wifi_driver_up_by_interface(wiced_interface_t interface)
{
  struct bcm4390x_wwd_driver_s *priv = bcm4390x_wwd_find_driver_by_interface(interface);

  if (!priv)
  {
    return -EINVAL;
  }

  return bcm4390x_wwd_wlan_up(priv->interface);
}

int wifi_driver_down_by_interface(wiced_interface_t interface)
{
  struct bcm4390x_wwd_driver_s *priv = bcm4390x_wwd_find_driver_by_interface(interface);

  if (!priv)
  {
    return -EINVAL;
  }

  return bcm4390x_wwd_wlan_down(priv->interface);
}

int wifi_driver_ip_up(wiced_interface_t interface)
{
  struct bcm4390x_wwd_driver_s *priv = bcm4390x_wwd_find_driver_by_interface(interface);

  if (!priv)
  {
    return -EINVAL;
  }

  return bcm4390x_wwd_netdev_register(priv);
}

int wifi_driver_ip_down(wiced_interface_t interface)
{
  struct bcm4390x_wwd_driver_s *priv = bcm4390x_wwd_find_driver_by_interface(interface);

  if (!priv)
  {
    return -EINVAL;
  }

  return bcm4390x_wwd_netdev_unregister(priv);
}

bool wifi_driver_is_valid(wiced_interface_t interface)
{
  return bcm4390x_wwd_find_driver_by_interface(interface) ? true : false;
}
