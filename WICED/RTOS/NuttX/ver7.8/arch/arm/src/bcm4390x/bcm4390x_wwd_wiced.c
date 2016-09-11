/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*
 * This is facade to WICED code.
 * Use to separate compilation of WICED and NuttX code and to avoid header files conflicts.
 */

#include <nuttx/config.h>

#include "bcm4390x_wwd.h"

#include "wwd_wifi.h"
#include "network/wwd_network_interface.h"
#include "network/wwd_buffer_interface.h"

#include "wiced_management.h"
#include "wiced_deep_sleep.h"
#include "internal/wiced_internal_api.h"

#ifndef WWD_TO_WICED_INTERFACE
#define WWD_TO_WICED_INTERFACE(interface) (interface)
#endif

static void bcm4390x_wwd_initialize_wiced_mac(const uint8_t *mac, wiced_mac_t *wiced_mac)
{
  wiced_mac->octet[0] = mac[0];
  wiced_mac->octet[1] = mac[1];
  wiced_mac->octet[2] = mac[2];
  wiced_mac->octet[3] = mac[3];
  wiced_mac->octet[4] = mac[4];
  wiced_mac->octet[5] = mac[5];
}

static void bcm4390x_wwd_initialize_nuttx_mac(const wiced_mac_t *wiced_mac, uint8_t *mac)
{
  mac[0] = wiced_mac->octet[0];
  mac[1] = wiced_mac->octet[1];
  mac[2] = wiced_mac->octet[2];
  mac[3] = wiced_mac->octet[3];
  mac[4] = wiced_mac->octet[4];
  mac[5] = wiced_mac->octet[5];
}

int bcm4390x_wwd_register_multicast_address(wiced_interface_t interface, const uint8_t *mac)
{
  wiced_mac_t wiced_mac;

  bcm4390x_wwd_initialize_wiced_mac(mac, &wiced_mac);

  return (wwd_wifi_register_multicast_address_for_interface(&wiced_mac, WICED_TO_WWD_INTERFACE(interface)) == WWD_SUCCESS) ? OK : ERROR;
}

int bcm4390x_wwd_unregister_multicast_address(wiced_interface_t interface, const uint8_t *mac)
{
  wiced_mac_t wiced_mac;

  bcm4390x_wwd_initialize_wiced_mac(mac, &wiced_mac);

  return (wwd_wifi_unregister_multicast_address_for_interface(&wiced_mac, WICED_TO_WWD_INTERFACE(interface)) == WWD_SUCCESS) ? OK : ERROR;
}

int bcm4390x_wwd_get_interface_mac_address(wiced_interface_t interface, uint8_t *mac)
{
  wiced_mac_t wiced_mac;

  if (wwd_wifi_get_mac_address(&wiced_mac, WICED_TO_WWD_INTERFACE(interface)) != WWD_SUCCESS)
  {
    return ERROR;
  }

  bcm4390x_wwd_initialize_nuttx_mac(&wiced_mac, mac);

  return OK;
}

int bcm4390x_wwd_wlan_init(void)
{
  wiced_result_t result;

  if ( WICED_DEEP_SLEEP_IS_WARMBOOT_HANDLE( ) )
  {
    result = wiced_resume_after_deep_sleep();
  }
  else
  {
    result = wiced_wlan_connectivity_init();
  }

  return (result == WICED_SUCCESS) ? OK : ERROR;
}

int bcm4390x_wwd_wlan_deinit(void)
{
  return (wiced_wlan_connectivity_deinit() == WICED_SUCCESS) ? OK : ERROR;
}

int bcm4390x_wwd_wlan_up(wiced_interface_t interface)
{
  return (wiced_network_up(interface, -1, NULL) == WICED_SUCCESS) ? OK : ERROR;
}

int bcm4390x_wwd_wlan_down(wiced_interface_t interface)
{
  return (wiced_network_down(interface) == WICED_SUCCESS) ? OK : ERROR;
}

void host_network_process_ethernet_data(wiced_buffer_t buffer, wwd_interface_t interface)
{
  bcm4390x_wwd_rxavail(WWD_TO_WICED_INTERFACE(interface), buffer);
}

uint8_t* bcm4390x_wwd_get_buffer_data(wiced_buffer_t buffer, uint16_t *len)
{
  *len = host_buffer_get_current_piece_size(buffer);
  return host_buffer_get_current_piece_data_pointer(buffer);
}

void bcm4390x_wwd_send_ethernet_data(wiced_interface_t interface, wiced_buffer_t buffer, uint16_t len, bool tx_enable)
{
  wwd_result_t wwd_result;

  if (!tx_enable)
  {
    nlldbg("DROPPED: due to transmission disabled\n");
    bcm4390x_wwd_free_tx_buffer(buffer);
    return;
  }

  wwd_result = host_buffer_set_size(buffer, len);
  DEBUGASSERT(wwd_result == WWD_SUCCESS);

  wwd_network_send_ethernet_data(buffer, WICED_TO_WWD_INTERFACE(interface));
}

uint8_t* bcm4390x_wwd_alloc_tx_buffer(wiced_buffer_t *buffer)
{
  wwd_result_t wwd_result;

  if (host_buffer_get(buffer, WWD_NETWORK_TX, WICED_LINK_MTU, WICED_FALSE) != WWD_SUCCESS)
  {
    return NULL;
  }

  wwd_result = host_buffer_add_remove_at_front(buffer, WICED_LINK_OVERHEAD_BELOW_ETHERNET_FRAME_MAX);
  DEBUGASSERT(wwd_result == WWD_SUCCESS);

  return host_buffer_get_current_piece_data_pointer(*buffer);
}

void bcm4390x_wwd_free_tx_buffer(wiced_buffer_t buffer)
{
  host_buffer_release(buffer, WWD_NETWORK_TX);
}

void bcm4390x_wwd_free_rx_buffer(wiced_buffer_t buffer)
{
  host_buffer_release(buffer, WWD_NETWORK_RX);
}

void bcm4390x_wwd_buffer_init_fifo(wiced_buffer_fifo_t *fifo)
{
  host_buffer_init_fifo(fifo);
}

void bcm4390x_wwd_buffer_push_to_fifo(wiced_buffer_fifo_t *fifo, wiced_buffer_t buffer)
{
  host_buffer_push_to_fifo(fifo, buffer, WWD_STA_INTERFACE /* does not matter, not used */);
}

wiced_buffer_t bcm4390x_wwd_buffer_pop_from_fifo(wiced_buffer_fifo_t *fifo)
{
  return host_buffer_pop_from_fifo(fifo, NULL);
}

void wiced_network_notify_link_up(wiced_interface_t interface)
{
  UNUSED_PARAMETER(interface);

  bcm4390x_wwd_link_event_handler(interface, BCM4390X_WWD_LINK_EVENT_UP);
}

void wiced_network_notify_link_down(wiced_interface_t interface)
{
  UNUSED_PARAMETER(interface);

  bcm4390x_wwd_link_event_handler(interface, BCM4390X_WWD_LINK_EVENT_DOWN);
}

wiced_result_t wiced_wireless_link_renew_handler(void *arg)
{
  UNUSED_PARAMETER(arg);

  bcm4390x_wwd_link_event_handler(WICED_STA_INTERFACE, BCM4390X_WWD_LINK_EVENT_WIRELESS_RENEW);

  return WICED_SUCCESS;
}

wiced_result_t wiced_wireless_link_up_handler(void *arg)
{
  UNUSED_PARAMETER(arg);

  bcm4390x_wwd_link_event_handler(WICED_STA_INTERFACE, BCM4390X_WWD_LINK_EVENT_WIRELESS_UP);

  return WICED_SUCCESS;
}

wiced_result_t wiced_wireless_link_down_handler(void *arg)
{
  UNUSED_PARAMETER(arg);

  bcm4390x_wwd_link_event_handler(WICED_STA_INTERFACE, BCM4390X_WWD_LINK_EVENT_WIRELESS_DOWN);

  return WICED_SUCCESS;
}
