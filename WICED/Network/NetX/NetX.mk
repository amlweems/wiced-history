#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
NAME := NetX

VERSION := 5.4

$(NAME)_COMPONENTS += Wiced/Network/NetX/wwd
ifeq (,$(APP_WWD_ONLY)$(NS_WWD_ONLY)$(RTOS_WWD_ONLY))
$(NAME)_COMPONENTS += Wiced/Network/NetX/wiced
endif

# Define some macros to allow for some network-specific checks
GLOBAL_DEFINES += NETWORK_$(NAME)=1
GLOBAL_DEFINES += $(NAME)_VERSION=$$(SLASH_QUOTE_START)v$(VERSION)$$(SLASH_QUOTE_END)

GLOBAL_INCLUDES := ver$(VERSION) \
                   wiced

VALID_RTOS_LIST:= ThreadX

GLOBAL_DEFINES  += NX_INCLUDE_USER_DEFINE_FILE
GLOBAL_DEFINES  += __fd_set_defined
GLOBAL_DEFINES  += __suseconds_t_defined

ifdef WICED_ENABLE_TRACEX
$(info TRACEX)
NETX_LIBRARY_NAME :=NetX.TraceX.$(HOST_ARCH).$(BUILD_TYPE).a
else
NETX_LIBRARY_NAME :=NetX.$(HOST_ARCH).$(BUILD_TYPE).a
endif

ifneq ($(wildcard $(CURDIR)$(NETX_LIBRARY_NAME)),)
$(NAME)_PREBUILT_LIBRARY := $(NETX_LIBRARY_NAME)
else

$(NAME)_HEADERS = \
           ver$(VERSION)/nx_api.h \
           ver$(VERSION)/nx_arp.h \
           ver$(VERSION)/nx_icmp.h \
           ver$(VERSION)/nx_igmp.h \
           ver$(VERSION)/nx_ip.h \
           ver$(VERSION)/nx_packet.h \
           ver$(VERSION)/nx_port.h \
           ver$(VERSION)/nx_rarp.h \
           ver$(VERSION)/nx_system.h \
           ver$(VERSION)/nx_tcp.h \
           ver$(VERSION)/nx_udp.h \
           ver$(VERSION)/nx_user.h \
           ver$(VERSION)/netx_bsd_layer/nx_bsd.h \
           ver$(VERSION)/netx_applications/auto_ip/nx_auto_ip.h \
           ver$(VERSION)/netx_applications/dhcp/nx_dhcp.h \
           ver$(VERSION)/netx_applications/dhcp/nx_dhcp_server.h \
           ver$(VERSION)/netx_applications/dns/nx_dns.h \
           ver$(VERSION)/netx_applications/ftp/nx_ftp.h \
           ver$(VERSION)/netx_applications/ftp/filex_stub.h \
           ver$(VERSION)/netx_applications/http/filex_stub.h \
           ver$(VERSION)/netx_applications/http/nx_http.h \
           ver$(VERSION)/netx_applications/http/nx_md5.h \
           ver$(VERSION)/netx_applications/nat/nx_nat.h \
           ver$(VERSION)/netx_applications/pop3/nx_md5.h \
           ver$(VERSION)/netx_applications/pop3/nx_pop3.h \
           ver$(VERSION)/netx_applications/pop3/nx_pop3_client.h \
           ver$(VERSION)/netx_applications/pop3/nx_pop3_server.h \
           ver$(VERSION)/netx_applications/ppp/nx_md5.h \
           ver$(VERSION)/netx_applications/ppp/nx_ppp.h \
           ver$(VERSION)/netx_applications/smtp/nx_smtp.h \
           ver$(VERSION)/netx_applications/smtp/nx_smtp_client.h \
           ver$(VERSION)/netx_applications/smtp/nx_smtp_server.h \
           ver$(VERSION)/netx_applications/snmp/nx_des.h \
           ver$(VERSION)/netx_applications/snmp/nx_snmp.h \
           ver$(VERSION)/netx_applications/snmp/nx_sha1.h \
           ver$(VERSION)/netx_applications/snmp/nx_md5.h \
           ver$(VERSION)/netx_applications/sntp/nx_sntp.h \
           ver$(VERSION)/netx_applications/sntp/nx_sntp_client.h \
           ver$(VERSION)/netx_applications/telnet/nx_telnet.h \
           ver$(VERSION)/netx_applications/tftp/filex_stub.h \
           ver$(VERSION)/netx_applications/tftp/nx_tftp.h

$(NAME)_SOURCES := \
           ver$(VERSION)/nx_igmp_multicast_interface_join.c \
           ver$(VERSION)/nx_ip_interface_address_get.c \
           ver$(VERSION)/nx_ip_interface_address_set.c \
           ver$(VERSION)/nx_ip_interface_attach.c \
           ver$(VERSION)/nx_ip_interface_info_get.c \
           ver$(VERSION)/nx_ip_interface_status_check.c \
           ver$(VERSION)/nx_ip_loopback_send.c \
           ver$(VERSION)/nx_ip_raw_packet_interface_send.c \
           ver$(VERSION)/nx_ip_route_find.c \
           ver$(VERSION)/nx_packet_data_extract_offset.c \
           ver$(VERSION)/nx_tcp_socket_window_update_notify_set.c \
           ver$(VERSION)/nx_udp_packet_info_extract.c \
           ver$(VERSION)/nx_udp_socket_interface_send.c \
           ver$(VERSION)/nxe_igmp_multicast_interface_join.c \
           ver$(VERSION)/nxe_ip_interface_address_get.c \
           ver$(VERSION)/nxe_ip_interface_address_set.c \
           ver$(VERSION)/nxe_ip_interface_attach.c \
           ver$(VERSION)/nxe_ip_interface_info_get.c \
           ver$(VERSION)/nxe_ip_interface_status_check.c \
           ver$(VERSION)/nxe_ip_raw_packet_interface_send.c \
           ver$(VERSION)/nxe_packet_data_extract_offset.c \
           ver$(VERSION)/nxe_tcp_socket_window_update_notify_set.c \
           ver$(VERSION)/nxe_udp_packet_info_extract.c \
           ver$(VERSION)/nxe_udp_socket_interface_send.c \
           ver$(VERSION)/nx_arp_dynamic_entries_invalidate.c \
           ver$(VERSION)/nx_arp_dynamic_entry_set.c \
           ver$(VERSION)/nx_arp_enable.c \
           ver$(VERSION)/nx_arp_entry_allocate.c \
           ver$(VERSION)/nx_arp_gratuitous_send.c \
           ver$(VERSION)/nx_arp_hardware_address_find.c \
           ver$(VERSION)/nx_arp_info_get.c \
           ver$(VERSION)/nx_arp_ip_address_find.c \
           ver$(VERSION)/nx_arp_packet_deferred_receive.c \
           ver$(VERSION)/nx_arp_packet_receive.c \
           ver$(VERSION)/nx_arp_packet_send.c \
           ver$(VERSION)/nx_arp_periodic_update.c \
           ver$(VERSION)/nx_arp_queue_process.c \
           ver$(VERSION)/nx_arp_static_entries_delete.c \
           ver$(VERSION)/nx_arp_static_entry_create.c \
           ver$(VERSION)/nx_arp_static_entry_delete.c \
           ver$(VERSION)/nx_icmp_checksum_compute.c \
           ver$(VERSION)/nx_icmp_cleanup.c \
           ver$(VERSION)/nx_icmp_enable.c \
           ver$(VERSION)/nx_icmp_info_get.c \
           ver$(VERSION)/nx_icmp_packet_process.c \
           ver$(VERSION)/nx_icmp_packet_receive.c \
           ver$(VERSION)/nx_icmp_ping.c \
           ver$(VERSION)/nx_icmp_queue_process.c \
           ver$(VERSION)/nx_igmp_enable.c \
           ver$(VERSION)/nx_igmp_info_get.c \
           ver$(VERSION)/nx_igmp_loopback_disable.c \
           ver$(VERSION)/nx_igmp_loopback_enable.c \
           ver$(VERSION)/nx_igmp_multicast_check.c \
           ver$(VERSION)/nx_igmp_multicast_join.c \
           ver$(VERSION)/nx_igmp_multicast_leave.c \
           ver$(VERSION)/nx_igmp_packet_process.c \
           ver$(VERSION)/nx_igmp_packet_receive.c \
           ver$(VERSION)/nx_igmp_periodic_processing.c \
           ver$(VERSION)/nx_igmp_queue_process.c \
           ver$(VERSION)/nx_ip_address_change_notify.c \
           ver$(VERSION)/nx_ip_address_get.c \
           ver$(VERSION)/nx_ip_address_set.c \
           ver$(VERSION)/nx_ip_create.c \
           ver$(VERSION)/nx_ip_suspend.c \
           ver$(VERSION)/nx_ip_resume.c \
           ver$(VERSION)/nx_ip_delete.c \
           ver$(VERSION)/nx_ip_delete_queue_clear.c \
           ver$(VERSION)/nx_ip_driver_deferred_enable.c \
           ver$(VERSION)/nx_ip_driver_deferred_processing.c \
           ver$(VERSION)/nx_ip_driver_deferred_receive.c \
           ver$(VERSION)/nx_ip_driver_direct_command.c \
           ver$(VERSION)/nx_ip_forwarding_disable.c \
           ver$(VERSION)/nx_ip_forwarding_enable.c \
           ver$(VERSION)/nx_ip_forward_packet_process.c \
           ver$(VERSION)/nx_ip_fragment_assembly.c \
           ver$(VERSION)/nx_ip_fragment_disable.c \
           ver$(VERSION)/nx_ip_fragment_enable.c \
           ver$(VERSION)/nx_ip_fragment_packet.c \
           ver$(VERSION)/nx_ip_fragment_timeout_check.c \
           ver$(VERSION)/nx_ip_gateway_address_set.c \
           ver$(VERSION)/nx_ip_info_get.c \
           ver$(VERSION)/nx_ip_initialize.c \
           ver$(VERSION)/nx_ip_packet_deferred_receive.c \
           ver$(VERSION)/nx_ip_packet_receive.c \
           ver$(VERSION)/nx_ip_packet_send.c \
           ver$(VERSION)/nx_ip_periodic_timer_entry.c \
           ver$(VERSION)/nx_ip_raw_packet_cleanup.c \
           ver$(VERSION)/nx_ip_raw_packet_disable.c \
           ver$(VERSION)/nx_ip_raw_packet_enable.c \
           ver$(VERSION)/nx_ip_raw_packet_processing.c \
           ver$(VERSION)/nx_ip_raw_packet_receive.c \
           ver$(VERSION)/nx_ip_raw_packet_send.c \
           ver$(VERSION)/nx_ip_static_route_add.c \
           ver$(VERSION)/nx_ip_static_route_delete.c \
           ver$(VERSION)/nx_ip_status_check.c \
           ver$(VERSION)/nx_ip_thread_entry.c \
           ver$(VERSION)/nx_packet_allocate.c \
           ver$(VERSION)/nx_packet_copy.c \
           ver$(VERSION)/nx_packet_data_append.c \
           ver$(VERSION)/nx_packet_data_retrieve.c \
           ver$(VERSION)/nx_packet_length_get.c \
           ver$(VERSION)/nx_packet_pool_cleanup.c \
           ver$(VERSION)/nx_packet_pool_create.c \
           ver$(VERSION)/nx_packet_pool_delete.c \
           ver$(VERSION)/nx_packet_pool_info_get.c \
           ver$(VERSION)/nx_packet_pool_initialize.c \
           ver$(VERSION)/nx_packet_release.c \
           ver$(VERSION)/nx_packet_transmit_release.c \
           ver$(VERSION)/nx_rarp_disable.c \
           ver$(VERSION)/nx_rarp_enable.c \
           ver$(VERSION)/nx_rarp_info_get.c \
           ver$(VERSION)/nx_rarp_packet_deferred_receive.c \
           ver$(VERSION)/nx_rarp_packet_receive.c \
           ver$(VERSION)/nx_rarp_packet_send.c \
           ver$(VERSION)/nx_rarp_periodic_update.c \
           ver$(VERSION)/nx_rarp_queue_process.c \
           ver$(VERSION)/nx_system_initialize.c \
           ver$(VERSION)/nx_tcp_checksum.c \
           ver$(VERSION)/nx_tcp_cleanup_deferred.c \
           ver$(VERSION)/nx_tcp_client_bind_cleanup.c \
           ver$(VERSION)/nx_tcp_client_socket_bind.c \
           ver$(VERSION)/nx_tcp_client_socket_connect.c \
           ver$(VERSION)/nx_tcp_client_socket_port_get.c \
           ver$(VERSION)/nx_tcp_client_socket_unbind.c \
           ver$(VERSION)/nx_tcp_connect_cleanup.c \
           ver$(VERSION)/nx_tcp_deferred_cleanup_check.c \
           ver$(VERSION)/nx_tcp_disconnect_cleanup.c \
           ver$(VERSION)/nx_tcp_enable.c \
           ver$(VERSION)/nx_tcp_suspend.c \
           ver$(VERSION)/nx_tcp_resume.c \
           ver$(VERSION)/nx_tcp_fast_periodic_processing.c \
           ver$(VERSION)/nx_tcp_fast_periodic_timer_entry.c \
           ver$(VERSION)/nx_tcp_free_port_find.c \
           ver$(VERSION)/nx_tcp_info_get.c \
           ver$(VERSION)/nx_tcp_initialize.c \
           ver$(VERSION)/nx_tcp_mss_option_get.c \
           ver$(VERSION)/nx_tcp_no_connection_reset.c \
           ver$(VERSION)/nx_tcp_packet_process.c \
           ver$(VERSION)/nx_tcp_packet_receive.c \
           ver$(VERSION)/nx_tcp_packet_send_ack.c \
           ver$(VERSION)/nx_tcp_packet_send_fin.c \
           ver$(VERSION)/nx_tcp_packet_send_rst.c \
           ver$(VERSION)/nx_tcp_packet_send_syn.c \
           ver$(VERSION)/nx_tcp_periodic_processing.c \
           ver$(VERSION)/nx_tcp_queue_process.c \
           ver$(VERSION)/nx_tcp_receive_cleanup.c \
           ver$(VERSION)/nx_tcp_server_socket_accept.c \
           ver$(VERSION)/nx_tcp_server_socket_listen.c \
           ver$(VERSION)/nx_tcp_server_socket_relisten.c \
           ver$(VERSION)/nx_tcp_server_socket_unaccept.c \
           ver$(VERSION)/nx_tcp_server_socket_unlisten.c \
           ver$(VERSION)/nx_tcp_socket_bytes_available.c \
           ver$(VERSION)/nx_tcp_socket_connection_reset.c \
           ver$(VERSION)/nx_tcp_socket_create.c \
           ver$(VERSION)/nx_tcp_socket_delete.c \
           ver$(VERSION)/nx_tcp_socket_disconnect.c \
           ver$(VERSION)/nx_tcp_socket_info_get.c \
           ver$(VERSION)/nx_tcp_socket_mss_get.c \
           ver$(VERSION)/nx_tcp_socket_mss_peer_get.c \
           ver$(VERSION)/nx_tcp_socket_mss_set.c \
           ver$(VERSION)/nx_tcp_socket_packet_process.c \
           ver$(VERSION)/nx_tcp_socket_peer_info_get.c \
           ver$(VERSION)/nx_tcp_socket_receive.c \
           ver$(VERSION)/nx_tcp_socket_receive_notify.c \
           ver$(VERSION)/nx_tcp_socket_receive_queue_flush.c \
           ver$(VERSION)/nx_tcp_socket_send.c \
           ver$(VERSION)/nx_tcp_socket_state_ack_check.c \
           ver$(VERSION)/nx_tcp_socket_state_closing.c \
           ver$(VERSION)/nx_tcp_socket_state_data_check.c \
           ver$(VERSION)/nx_tcp_socket_state_established.c \
           ver$(VERSION)/nx_tcp_socket_state_fin_wait1.c \
           ver$(VERSION)/nx_tcp_socket_state_fin_wait2.c \
           ver$(VERSION)/nx_tcp_socket_state_last_ack.c \
           ver$(VERSION)/nx_tcp_socket_state_syn_received.c \
           ver$(VERSION)/nx_tcp_socket_state_syn_sent.c \
           ver$(VERSION)/nx_tcp_socket_state_transmit_check.c \
           ver$(VERSION)/nx_tcp_socket_state_wait.c \
           ver$(VERSION)/nx_tcp_socket_thread_resume.c \
           ver$(VERSION)/nx_tcp_socket_thread_suspend.c \
           ver$(VERSION)/nx_tcp_socket_transmit_configure.c \
           ver$(VERSION)/nx_tcp_socket_transmit_queue_flush.c \
           ver$(VERSION)/nx_tcp_transmit_cleanup.c \
           ver$(VERSION)/nx_trace_event_insert.c \
           ver$(VERSION)/nx_trace_event_update.c \
           ver$(VERSION)/nx_trace_object_register.c \
           ver$(VERSION)/nx_trace_object_unregister.c \
           ver$(VERSION)/nx_udp_bind_cleanup.c \
           ver$(VERSION)/nx_udp_enable.c \
           ver$(VERSION)/nx_udp_free_port_find.c \
           ver$(VERSION)/nx_udp_info_get.c \
           ver$(VERSION)/nx_udp_packet_receive.c \
           ver$(VERSION)/nx_udp_receive_cleanup.c \
           ver$(VERSION)/nx_udp_socket_bind.c \
           ver$(VERSION)/nx_udp_socket_bytes_available.c \
           ver$(VERSION)/nx_udp_socket_checksum_disable.c \
           ver$(VERSION)/nx_udp_socket_checksum_enable.c \
           ver$(VERSION)/nx_udp_socket_create.c \
           ver$(VERSION)/nx_udp_socket_delete.c \
           ver$(VERSION)/nx_udp_socket_info_get.c \
           ver$(VERSION)/nx_udp_socket_port_get.c \
           ver$(VERSION)/nx_udp_socket_receive.c \
           ver$(VERSION)/nx_udp_socket_receive_notify.c \
           ver$(VERSION)/nx_udp_socket_send.c \
           ver$(VERSION)/nx_udp_socket_unbind.c \
           ver$(VERSION)/nx_udp_source_extract.c \
           ver$(VERSION)/nxe_arp_dynamic_entries_invalidate.c \
           ver$(VERSION)/nxe_arp_dynamic_entry_set.c \
           ver$(VERSION)/nxe_arp_enable.c \
           ver$(VERSION)/nxe_arp_gratuitous_send.c \
           ver$(VERSION)/nxe_arp_hardware_address_find.c \
           ver$(VERSION)/nxe_arp_info_get.c \
           ver$(VERSION)/nxe_arp_ip_address_find.c \
           ver$(VERSION)/nxe_arp_static_entries_delete.c \
           ver$(VERSION)/nxe_arp_static_entry_create.c \
           ver$(VERSION)/nxe_arp_static_entry_delete.c \
           ver$(VERSION)/nxe_icmp_enable.c \
           ver$(VERSION)/nxe_icmp_info_get.c \
           ver$(VERSION)/nxe_icmp_ping.c \
           ver$(VERSION)/nxe_igmp_enable.c \
           ver$(VERSION)/nxe_igmp_info_get.c \
           ver$(VERSION)/nxe_igmp_loopback_disable.c \
           ver$(VERSION)/nxe_igmp_loopback_enable.c \
           ver$(VERSION)/nxe_igmp_multicast_join.c \
           ver$(VERSION)/nxe_igmp_multicast_leave.c \
           ver$(VERSION)/nxe_ip_address_change_notify.c \
           ver$(VERSION)/nxe_ip_address_get.c \
           ver$(VERSION)/nxe_ip_address_set.c \
           ver$(VERSION)/nxe_ip_create.c \
           ver$(VERSION)/nxe_ip_delete.c \
           ver$(VERSION)/nxe_ip_driver_direct_command.c \
           ver$(VERSION)/nxe_ip_forwarding_disable.c \
           ver$(VERSION)/nxe_ip_forwarding_enable.c \
           ver$(VERSION)/nxe_ip_fragment_disable.c \
           ver$(VERSION)/nxe_ip_fragment_enable.c \
           ver$(VERSION)/nxe_ip_gateway_address_set.c \
           ver$(VERSION)/nxe_ip_info_get.c \
           ver$(VERSION)/nxe_ip_raw_packet_disable.c \
           ver$(VERSION)/nxe_ip_raw_packet_enable.c \
           ver$(VERSION)/nxe_ip_raw_packet_receive.c \
           ver$(VERSION)/nxe_ip_raw_packet_send.c \
           ver$(VERSION)/nxe_ip_static_route_add.c \
           ver$(VERSION)/nxe_ip_static_route_delete.c \
           ver$(VERSION)/nxe_ip_status_check.c \
           ver$(VERSION)/nxe_packet_allocate.c \
           ver$(VERSION)/nxe_packet_copy.c \
           ver$(VERSION)/nxe_packet_data_append.c \
           ver$(VERSION)/nxe_packet_data_retrieve.c \
           ver$(VERSION)/nxe_packet_length_get.c \
           ver$(VERSION)/nxe_packet_pool_create.c \
           ver$(VERSION)/nxe_packet_pool_delete.c \
           ver$(VERSION)/nxe_packet_pool_info_get.c \
           ver$(VERSION)/nxe_packet_release.c \
           ver$(VERSION)/nxe_packet_transmit_release.c \
           ver$(VERSION)/nxe_rarp_disable.c \
           ver$(VERSION)/nxe_rarp_enable.c \
           ver$(VERSION)/nxe_rarp_info_get.c \
           ver$(VERSION)/nxe_tcp_client_socket_bind.c \
           ver$(VERSION)/nxe_tcp_client_socket_connect.c \
           ver$(VERSION)/nxe_tcp_client_socket_port_get.c \
           ver$(VERSION)/nxe_tcp_client_socket_unbind.c \
           ver$(VERSION)/nxe_tcp_enable.c \
           ver$(VERSION)/nxe_tcp_free_port_find.c \
           ver$(VERSION)/nxe_tcp_info_get.c \
           ver$(VERSION)/nxe_tcp_server_socket_accept.c \
           ver$(VERSION)/nxe_tcp_server_socket_listen.c \
           ver$(VERSION)/nxe_tcp_server_socket_relisten.c \
           ver$(VERSION)/nxe_tcp_server_socket_unaccept.c \
           ver$(VERSION)/nxe_tcp_server_socket_unlisten.c \
           ver$(VERSION)/nxe_tcp_socket_bytes_available.c \
           ver$(VERSION)/nxe_tcp_socket_create.c \
           ver$(VERSION)/nxe_tcp_socket_delete.c \
           ver$(VERSION)/nxe_tcp_socket_disconnect.c \
           ver$(VERSION)/nxe_tcp_socket_info_get.c \
           ver$(VERSION)/nxe_tcp_socket_mss_get.c \
           ver$(VERSION)/nxe_tcp_socket_mss_peer_get.c \
           ver$(VERSION)/nxe_tcp_socket_mss_set.c \
           ver$(VERSION)/nxe_tcp_socket_peer_info_get.c \
           ver$(VERSION)/nxe_tcp_socket_receive.c \
           ver$(VERSION)/nxe_tcp_socket_receive_notify.c \
           ver$(VERSION)/nxe_tcp_socket_send.c \
           ver$(VERSION)/nxe_tcp_socket_state_wait.c \
           ver$(VERSION)/nxe_tcp_socket_transmit_configure.c \
           ver$(VERSION)/nxe_udp_enable.c \
           ver$(VERSION)/nxe_udp_free_port_find.c \
           ver$(VERSION)/nxe_udp_info_get.c \
           ver$(VERSION)/nxe_udp_socket_bind.c \
           ver$(VERSION)/nxe_udp_socket_bytes_available.c \
           ver$(VERSION)/nxe_udp_socket_checksum_disable.c \
           ver$(VERSION)/nxe_udp_socket_checksum_enable.c \
           ver$(VERSION)/nxe_udp_socket_create.c \
           ver$(VERSION)/nxe_udp_socket_delete.c \
           ver$(VERSION)/nxe_udp_socket_info_get.c \
           ver$(VERSION)/nxe_udp_socket_port_get.c \
           ver$(VERSION)/nxe_udp_socket_receive.c \
           ver$(VERSION)/nxe_udp_socket_receive_notify.c \
           ver$(VERSION)/nxe_udp_socket_send.c \
           ver$(VERSION)/nxe_udp_socket_unbind.c \
           ver$(VERSION)/nxe_udp_source_extract.c \
           ver$(VERSION)/netx_applications/auto_ip/nx_auto_ip.c \
           ver$(VERSION)/netx_applications/dhcp/nx_dhcp_server.c \
           ver$(VERSION)/netx_applications/dhcp/nx_dhcp.c \
           ver$(VERSION)/netx_applications/dns/nx_dns.c \
           ver$(VERSION)/netx_applications/ftp/nx_ftp_client.c \
           ver$(VERSION)/netx_applications/http/nx_http_client.c \
           ver$(VERSION)/netx_applications/http/nx_http_server.c \
           ver$(VERSION)/netx_applications/ppp/nx_md5.c \
           ver$(VERSION)/netx_applications/ppp/nx_ppp.c \
           ver$(VERSION)/netx_applications/snmp/nx_des.c \
           ver$(VERSION)/netx_applications/snmp/nx_md5.c \
           ver$(VERSION)/netx_applications/snmp/nx_sha1.c \
           ver$(VERSION)/netx_applications/snmp/nx_snmp.c \
           ver$(VERSION)/netx_applications/telnet/nx_telnet_server.c \
           ver$(VERSION)/netx_applications/telnet/nx_telnet_client.c \
           ver$(VERSION)/netx_bsd_layer/nx_bsd.c

# FTP server FileX stub is missing prototype for fx_directory_local_path_restore
#           ver$(VERSION)/netx_applications/ftp/nx_ftp_server.c

# These use incorrect printf format specifiers which cause errors with GCC -Wall -Werror options
#           ver$(VERSION)/netx_applications/nat/nx_nat.c \
#           ver$(VERSION)/netx_applications/pop3/nx_md5.c \
#           ver$(VERSION)/netx_applications/pop3/nx_packet_data_extract.c \
#           ver$(VERSION)/netx_applications/pop3/nx_pop3_client.c \
#           ver$(VERSION)/netx_applications/pop3/nx_pop3_server.c \
#           ver$(VERSION)/netx_applications/smtp/nx_smtp_client.c \
#           ver$(VERSION)/netx_applications/smtp/nx_smtp_server.c \
#           ver$(VERSION)/netx_applications/sntp/nx_sntp_client.c \

# This gives the following error: operation on 'buffer_ptr' may be undefined
#           ver$(VERSION)/netx_applications/tftp/nx_tftp_client.c \
#           ver$(VERSION)/netx_applications/tftp/nx_tftp_server.c \


endif #ifneq ($(wildcard $(CURDIR)NetX.$(HOST_ARCH).$(BUILD_TYPE).a),)