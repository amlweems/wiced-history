/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Provides an implementation of the Broadcom SDPCM protocol.
 *  The Broadcom SDPCM protocol provides multiplexing of Wireless Data frames,
 *  I/O Control functions (IOCTL), and Asynchronous Event signalling.
 *  It is required when communicating with Broadcom 802.11 devices.
 *
 */

#include "internal/SDPCM.h"
#include <stdint.h>
#include "wwd_rtos.h"
#include "wwd_buffer.h"
#include "wwd_constants.h"
#include "wwd_wifi.h"
#include "wwd_assert.h"
#include "internal/wwd_ap.h"
#include "internal/wwd_thread.h"
#include "RTOS/wwd_rtos_interface.h"
#include "Network/wwd_buffer_interface.h"
#include "Network/wwd_network_interface.h"
#include "internal/bcmendian.h"
#include "wwd_logging.h"
#include "internal/Bus_protocols/wwd_bus_protocol_interface.h"
#include <string.h> /* For strlen, memcpy */
#include "Network/wwd_network_constants.h"
#include "wwd_events.h"
#include "bcmendian.h"
#include "internal/wwd_internal.h"
#include "wwd_management.h"

/******************************************************
 * @cond       Constants
 ******************************************************/

#ifndef WICED_EVENT_HANDLER_LIST_SIZE
#define WICED_EVENT_HANDLER_LIST_SIZE   (5)      /** Maximum number of simultaneously registered event handlers */
#endif

#define WICED_IOCTL_TIMEOUT_MS         (400)


#define BDC_PROTO_VER                  (2)      /** Version number of BDC header */
#define BDC_FLAG_VER_SHIFT             (4)      /** Number of bits to shift BDC version number in the flags field */
#define ETHER_TYPE_BRCM           (0x886C)      /** Broadcom Ethertype for identifying event packets - Copied from DHD include/proto/ethernet.h */
#define BRCM_OUI            "\x00\x10\x18"      /** Broadcom OUI (Organizationally Unique Identifier): Used in the proprietary(221) IE (Information Element) in all Broadcom devices */
#define DOT11_OUI_LEN                  (3)      /** Length in bytes of 802.11 OUI*/
#define BCM_MSG_IFNAME_MAX            (16)      /** Maximum length of an interface name in a wl_event_msg_t structure*/

/* QoS related definitions (type of service) */
#define IPV4_TOS_OFFSET               (15)      /** Offset for finding the TOS field in an IPv4 header */
#define TOS_MASK                    (0x07)      /** Mask for extracting priority the bits (IPv4 or IPv6) */

/* CDC flag definitions taken from bcmcdc.h */
#define CDCF_IOC_ERROR              (0x01)      /** 0=success, 1=ioctl cmd failed */
#define CDCF_IOC_IF_MASK          (0xF000)      /** I/F index */
#define CDCF_IOC_IF_SHIFT             (12)      /** # of bits of shift for I/F Mask */
#define CDCF_IOC_ID_MASK      (0xFFFF0000)      /** used to uniquely id an ioctl req/resp pairing */
#define CDCF_IOC_ID_SHIFT             (16)      /** # of bits of shift for ID Mask */

#define BDC_FLAG2_IF_MASK           (0x0f)

#define SDPCM_HEADER_LEN              (12)
#define BDC_HEADER_LEN                 (4)

/* Event flags */
#define WLC_EVENT_MSG_LINK      (0x01)    /** link is up */
#define WLC_EVENT_MSG_FLUSHTXQ  (0x02)    /** flush tx queue on MIC error */
#define WLC_EVENT_MSG_GROUP     (0x04)    /** group MIC error */
#define WLC_EVENT_MSG_UNKBSS    (0x08)    /** unknown source bsscfg */
#define WLC_EVENT_MSG_UNKIF     (0x10)    /** unknown source OS i/f */


/******************************************************
 *             Macros
 ******************************************************/

#define MIN(x,y) ((x) < (y) ? (x) : (y))

/* bit map related macros */
#ifndef setbit
#ifndef NBBY      /* the BSD family defines NBBY */
#define  NBBY  8  /* 8 bits per byte */
#endif /* #ifndef NBBY */
#define  setbit(a, i)   (((uint8_t*)       a)[(int)(i)/(int)(NBBY)] |= (uint8_t)(1<<((i)%NBBY)))
#define  clrbit(a, i)   (((uint8_t*)       a)[(int)(i)/(int)(NBBY)] &= (uint8_t)~(1<<((i)%NBBY)))
#define  isset(a, i)    (((const uint8_t*) a)[(int)(i)/(int)(NBBY)] & (1<<((i)%NBBY)))
#define  isclr(a, i)    ((((const uint8_t*)a)[(int)(i)/(int)(NBBY)] & (1<<((i)%NBBY))) == 0)
#endif /* setbit */

#ifdef __GNUC__
#define WEAK __attribute__ ((weak))
#elif defined ( __IAR_SYSTEMS_ICC__ )
#define WEAK
#endif

/******************************************************
 *             Local Structures
 ******************************************************/

typedef enum
{
    DATA_HEADER       = 2,
    ASYNCEVENT_HEADER = 1,
    CONTROL_HEADER    = 0
} sdpcm_header_type_t;

#pragma pack(1)

/*TODO: Keep this typedef? (in preference to the defines above */
#if 0
typedef struct
{
    uint16_t control_id;
    uint8_t  interface_index :4;
    uint16_t reserved        :10;
    uint8_t  set             :1;
    uint8_t  error           :1;
}sdpcm_cdc_flags_t;
#endif /* if 0 */
typedef struct
{
    uint8_t sequence;
    uint8_t channel_and_flags;
    uint8_t next_length;
    uint8_t header_length;
    uint8_t wireless_flow_control;
    uint8_t bus_data_credit;
    uint8_t _reserved[2];
} sdpcm_sw_header_t;

typedef struct
{
    uint32_t cmd;    /* ioctl command value */
    uint32_t len;    /* lower 16: output buflen; upper 16: input buflen (excludes header) */
    uint32_t flags;  /* flag defns given in bcmcdc.h */
    uint32_t status; /* status code returned from the device */
} sdpcm_cdc_header_t;

typedef struct
{
    uint8_t flags;      /* Flags */
    uint8_t priority;   /* 802.1d Priority (low 3 bits) */
    uint8_t flags2;
    uint8_t data_offset; /* Offset from end of BDC header to packet data, in 4-uint8_t words.  Leaves room for optional headers.*/
} sdpcm_bdc_header_t;

typedef struct
{
    uint8_t octet[6];
} sdpcm_mac_t;

typedef struct
{
    sdpcm_mac_t destination_address;
    sdpcm_mac_t source_address;
	uint16_t    ethertype;
} sdpcm_ethernet_header_t;


/*
 * SDPCM header definitions
 */
typedef struct
{
    uint16_t           frametag[2];
    sdpcm_sw_header_t  sw_header;
} sdpcm_header_t;

typedef struct
{
    wiced_buffer_header_t  buffer_header;
    uint16_t            frametag[2];
    sdpcm_sw_header_t   sw_header;
} sdpcm_packet_header_t;

/*
 * SDPCM Packet structure definitions
 */
typedef struct
{
    sdpcm_packet_header_t  sdpcm_header;
    uint8_t                data[1];
} sdpcm_common_packet_t;

typedef struct
{
    sdpcm_packet_header_t  common;
    sdpcm_cdc_header_t     cdc_header;
    uint8_t data[1];
} sdpcm_control_packet_t;

typedef struct
{
    sdpcm_packet_header_t  common;
    uint8_t                _padding[2];
    sdpcm_bdc_header_t     bdc_header;
    uint8_t data[1];
} sdpcm_data_packet_t;

typedef struct bcmeth_hdr
{
    uint16_t subtype;      /** Vendor specific..32769 */
    uint16_t length;
    uint8_t  version;      /** Version is 0 */
    uint8_t  oui[3];       /** Broadcom OUI */
    uint16_t usr_subtype;  /** user specific Data */
} bcmeth_hdr_t;

/* these fields are stored in network order */
typedef struct
{
    uint16_t     version;                     /** Version 1 has fields up to ifname.  Version 2 has all fields including ifidx and bss_cfg_idx */
    uint16_t     flags;                       /** see flags */
    uint32_t     event_type;                  /** Message */
    uint32_t     status;                      /** Status code */
    uint32_t     reason;                      /** Reason code (if applicable) */
    uint32_t     auth_type;                   /** WLC_E_AUTH */
    uint32_t     datalen;                     /** data buf */
    wiced_mac_t  addr;                        /** Station address (if applicable) */
    char         ifname[BCM_MSG_IFNAME_MAX];  /** name of the packet incoming interface */
    uint8_t      ifidx;                       /** destination OS i/f index */
    uint8_t      bss_cfg_idx;                 /** source bsscfg index */
} wl_event_msg_t;

/* used by driver msgs */
typedef struct bcm_event
{
    bcmeth_hdr_t    bcm_hdr;
    wl_event_msg_t  event;
    uint8_t         data[1];
} bcm_event_t;

#pragma pack()

/** Event list element structure
 *
 * events : A pointer to a wiced_event_num_t array that is terminated with a WLC_E_NONE event
 * handler: A pointer to the wiced_event_handler_t function that will receive the event
 * handler_user_data : User provided data that will be passed to the handler when a matching event occurs
 */
typedef struct
{
    const /*@null@*/ wiced_event_num_t* events;
    /*@null@*/ wiced_event_handler_t    handler;
    /*@null@*/ void*                    handler_user_data;
} event_list_elem_t;

/** @endcond */

/******************************************************
 *             Static Variables
 ******************************************************/

/* Event list variables */
static event_list_elem_t      event_list[WICED_EVENT_HANDLER_LIST_SIZE];

/* IOCTL variables*/
static uint16_t                  requested_ioctl_id;
static host_semaphore_type_t     wiced_sdpcm_ioctl_mutex;
static /*@only@*/ wiced_buffer_t wiced_ioctl_response;
static host_semaphore_type_t     wiced_ioctl_sleep;
static uint32_t                  ioctl_abort_count = 0;

/* Bus data credit variables */
static uint8_t        sdpcm_packet_transmit_sequence_number;
static uint8_t        sdpcm_highest_rx_tos = 0;
static uint8_t        last_bus_data_credit = (uint8_t) 0;
static uint8_t credit_diff          = 0;
static uint8_t largest_credit_diff = 0;

/* Packet send queue variables */
static host_semaphore_type_t                        wiced_sdpcm_send_queue_mutex;
static wiced_buffer_t /*@owned@*/ /*@null@*/ wiced_sdpcm_send_queue_head = (wiced_buffer_t) NULL;
static wiced_buffer_t /*@owned@*/ /*@null@*/ wiced_sdpcm_send_queue_tail = (wiced_buffer_t) NULL;

extern wiced_bool_t monitor_mode_enabled;

/******************************************************
 *             SDPCM Logging
 *
 * Enable this section to allow logging of SDPCM packets
 * into a buffer for later perusal
 *
 * See sdpcm_log  and  next_sdpcm_log_pos
 *
 ******************************************************/
/** @cond */

#if 0

#define SDPCM_LOG_SIZE 30
#define SDPCM_LOG_HEADER_SIZE (0x60)

typedef enum { UNUSED, LOG_TX, LOG_RX } sdpcm_log_direction_t;
typedef enum { IOCTL, DATA, EVENT } sdpcm_log_type_t;

typedef struct SDPCM_log_entry_struct
{
    sdpcm_log_direction_t direction;
    sdpcm_log_type_t      type;
    unsigned long         time;
    unsigned long         length;
    unsigned char         header[SDPCM_LOG_HEADER_SIZE];
}sdpcm_log_entry_t;

static int next_sdpcm_log_pos = 0;
static sdpcm_log_entry_t sdpcm_log[SDPCM_LOG_SIZE];

static void add_sdpcm_log_entry( sdpcm_log_direction_t dir, sdpcm_log_type_t type, unsigned long length, char* eth_data )
{

    sdpcm_log[next_sdpcm_log_pos].direction = dir;
    sdpcm_log[next_sdpcm_log_pos].type = type;
    sdpcm_log[next_sdpcm_log_pos].time = host_rtos_get_time();
    sdpcm_log[next_sdpcm_log_pos].length = length;
    memcpy( sdpcm_log[next_sdpcm_log_pos].header, eth_data, SDPCM_LOG_HEADER_SIZE );
    next_sdpcm_log_pos++;
    if (next_sdpcm_log_pos >= SDPCM_LOG_SIZE)
    {
        next_sdpcm_log_pos = 0;
    }
}
#else
#define add_sdpcm_log_entry( dir, type, length, eth_data )
#endif

/** @endcond */

/******************************************************
 *             Static Function Prototypes
 ******************************************************/

static wiced_buffer_t wiced_get_next_buffer_in_queue( wiced_buffer_t buffer );
static void wiced_set_next_buffer_in_queue( wiced_buffer_t buffer, wiced_buffer_t prev_buffer );
static void wiced_send_sdpcm_common( /*@only@*/ wiced_buffer_t buffer, sdpcm_header_type_t header_type );

extern void host_network_process_raw_packet( wiced_buffer_t buffer, wiced_interface_t interface );

/******************************************************
 *             Function definitions
 ******************************************************/

/** Initialises the SDPCM protocol handler
 *
 *  Initialises mutex and semaphore flags needed by the SDPCM handler.
 *  Also initialises the list of event handlers. This function is called
 *  from the @ref wiced_thread_init function.
 *
 * @return    WICED_SUCCESS or WICED_ERROR
 */

wiced_result_t wiced_init_sdpcm( void )
{
    uint16_t i;
    ioctl_abort_count = 0;

    /* Create the mutex protecting the packet send queue */
    if ( host_rtos_init_semaphore( &wiced_sdpcm_ioctl_mutex ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }
    host_rtos_set_semaphore( &wiced_sdpcm_ioctl_mutex, WICED_FALSE );

    /* Create the event flag which signals the Wiced thread needs to wake up */
    if ( host_rtos_init_semaphore( &wiced_ioctl_sleep ) != WICED_SUCCESS )
    {
        host_rtos_deinit_semaphore( &wiced_sdpcm_ioctl_mutex );
        return WICED_ERROR;
    }

    /* Create the sdpcm packet queue semaphore */
    if ( host_rtos_init_semaphore( &wiced_sdpcm_send_queue_mutex ) != WICED_SUCCESS )
    {
        host_rtos_deinit_semaphore( &wiced_ioctl_sleep );
        host_rtos_deinit_semaphore( &wiced_sdpcm_ioctl_mutex );
        return WICED_ERROR;
    }
    host_rtos_set_semaphore( &wiced_sdpcm_send_queue_mutex, WICED_FALSE );

    wiced_sdpcm_send_queue_head = NULL;
    wiced_sdpcm_send_queue_tail = NULL;

    /* Initialise the list of event handler functions */
    for ( i = 0; i < (uint16_t) WICED_EVENT_HANDLER_LIST_SIZE; i++ )
    {
        event_list[i].events = NULL;
        event_list[i].handler = NULL;
        event_list[i].handler_user_data = NULL;
    }

    sdpcm_highest_rx_tos = 0;
    sdpcm_packet_transmit_sequence_number = 0;
    last_bus_data_credit = (uint8_t) 1;

    return WICED_SUCCESS;
}


/** Initialises the SDPCM protocol handler
 *
 *  De-initialises mutex and semaphore flags needed by the SDPCM handler.
 *  This function is called from the @ref wiced_thread_func function when it is exiting.
 */

void wiced_quit_sdpcm( void )
{
    /* Delete the sleep mutex */
    host_rtos_deinit_semaphore( &wiced_ioctl_sleep );

    /* Delete the queue mutex.  */
    host_rtos_deinit_semaphore( &wiced_sdpcm_ioctl_mutex );

    /* Delete the SDPCM queue mutex */
    host_rtos_deinit_semaphore( &wiced_sdpcm_send_queue_mutex );

    /* Free any left over packets in the queue */
    while (wiced_sdpcm_send_queue_head != NULL)
    {
        wiced_buffer_t buf = wiced_get_next_buffer_in_queue( wiced_sdpcm_send_queue_head );
        host_buffer_release(wiced_sdpcm_send_queue_head, WICED_NETWORK_TX);
        wiced_sdpcm_send_queue_head = buf;
    }
}


/** Sets/Gets an I/O Variable (IOVar)
 *
 *  This function either sets or retrieves the value of an I/O variable from the Broadcom 802.11 device.
 *  The data which is set or retrieved must be in a format structure which is appropriate for the particular
 *  I/O variable being accessed. These structures can only be found in the DHD source code such as wl/exe/wlu.c.
 *
 *  @Note: The function blocks until the I/O variable read/write has completed
 *
 * @param type       : SDPCM_SET or SDPCM_GET - indicating whether to set or get the I/O variable value
 * @param send_buffer_hnd : A handle for a packet buffer containing the data value to be sent.
 * @param response_buffer_hnd : A pointer which will receive the handle for the packet buffer containing the response data value received..
 * @param interface : Which interface to send the iovar to (SDPCM_STA_INTERFACE or SDPCM_AP_INTERFACE)
 *
 * @return    WICED_SUCCESS or WICED_ERROR
 */
wiced_result_t wiced_send_iovar( sdpcm_command_type_t type, wiced_buffer_t send_buffer_hnd, /*@out@*/ /*@null@*/ wiced_buffer_t* response_buffer_hnd, sdpcm_interface_t interface ) /*@releases send_buffer_hnd@*/
{
    if ( type == SDPCM_SET )
    {
        return wiced_send_ioctl( SDPCM_SET, (uint32_t) WLC_SET_VAR, send_buffer_hnd, response_buffer_hnd, interface );
    }
    else
    {
        return wiced_send_ioctl( SDPCM_GET, (uint32_t) WLC_GET_VAR, send_buffer_hnd, response_buffer_hnd, interface );
    }
}

/** Sends a data packet.
 *
 *  This function should be called by the bottom of the network stack in order for it
 *  to send an ethernet frame.
 *  The function prepends a BDC header, before sending to @ref wiced_send_sdpcm_common where
 *  the SDPCM header will be added
 *
 * @param buffer  : The ethernet packet buffer to be sent
 * @param interface : the interface over which to send the packet (AP or STA)
 *
 */
void wiced_network_send_ethernet_data( /*@only@*/ wiced_buffer_t buffer, wiced_interface_t interface ) /* Returns immediately - Wiced_buffer_tx_completed will be called once the transmission has finished */
{
    sdpcm_data_packet_t* packet;
    wiced_result_t result;
    uint8_t* tos = (uint8_t*)host_buffer_get_current_piece_data_pointer( buffer ) + IPV4_TOS_OFFSET;

    add_sdpcm_log_entry( LOG_TX, DATA, host_buffer_get_current_piece_size( buffer ), (char*) host_buffer_get_current_piece_data_pointer( buffer ) );

    WICED_LOG( ( "Wcd:> DATA pkt 0x%08X len %d\r\n", (unsigned int)buffer, (int)host_buffer_get_current_piece_size( buffer ) ) );

    /* Add link space at front of packet */
    result = host_buffer_add_remove_at_front( &buffer, (int32_t) -WICED_LINK_OVERHEAD_BELOW_ETHERNET_FRAME );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_WWD_DEBUG(("Unable to adjust header space\r\n"));
        host_buffer_release( buffer, WICED_NETWORK_TX );
        return;
    }


    packet = (sdpcm_data_packet_t*) host_buffer_get_current_piece_data_pointer( buffer );

    if ( ( interface != WICED_STA_INTERFACE ) &&
         ( interface != WICED_AP_INTERFACE  ) &&
         ( interface != WICED_CONFIG_INTERFACE  ) )
    {
        WPRINT_WWD_DEBUG(("No interface for packet send\r\n"));
        host_buffer_release( buffer, WICED_NETWORK_TX );
        return;
    }

    /* Prepare the BDC header */
    packet->bdc_header.flags = 0;
    packet->bdc_header.flags = (uint8_t) ( BDC_PROTO_VER << BDC_FLAG_VER_SHIFT );
    packet->bdc_header.priority = *tos & TOS_MASK;
    packet->bdc_header.flags2 = (uint8_t) ( (interface == WICED_STA_INTERFACE)?SDPCM_STA_INTERFACE:SDPCM_AP_INTERFACE );
    packet->bdc_header.data_offset = 0;

    /* Add the length of the BDC header and pass "down" */
    wiced_send_sdpcm_common( buffer, DATA_HEADER );
}

void wiced_process_bus_credit_update(uint8_t* data)
{
    sdpcm_sw_header_t* header = (sdpcm_sw_header_t*)(data + 4);

    if (( header->channel_and_flags & 0x0f ) < 3)
    {
        credit_diff = (uint8_t)(header->bus_data_credit - last_bus_data_credit);
        if ( credit_diff <= CHIP_MAX_BUS_DATA_CREDIT_DIFF )
        {
            last_bus_data_credit = header->bus_data_credit;
        }
        else
        {
            if (credit_diff > largest_credit_diff)
            {
                largest_credit_diff = credit_diff;
            }
        }
    }

    wiced_bus_set_flow_control(header->wireless_flow_control);
}

/** Processes and directs incoming SDPCM packets
 *
 *  This function receives SDPCM packets from the Broadcom 802.11 device and decodes the SDPCM header
 *  to determine where the packet should be directed.
 *
 *  - Control packets (IOCTL/IOVAR) cause the IOCTL flag to be set to allow the resumption of the thread
 *    which sent the IOCTL
 *  - Data Packets are sent to the bottom layer of the network stack via the @ref host_network_process_ethernet_data function
 *  - Event Packets are decoded to determine which event occurred, and the event handler list is consulted
 *    and the appropriate event handler is called.
 *
 * @param buffer  : The SDPCM packet buffer received from the Broadcom 802.11 device
 *
 */
void wiced_process_sdpcm( /*@only@*/ wiced_buffer_t buffer )
{
    sdpcm_common_packet_t* packet;
    uint16_t i;
    uint16_t j;
    uint16_t size;
    uint16_t size_inv;

    packet = (sdpcm_common_packet_t*) host_buffer_get_current_piece_data_pointer( buffer );

    /* Extract the total SDPCM packet size from the first two frametag bytes */
    size = packet->sdpcm_header.frametag[0];

    /* Check that the second two frametag bytes are the binary inverse of the size */
    size_inv = (uint16_t) ~size;  /* Separate variable due to GCC Bug 38341 */
    if ( packet->sdpcm_header.frametag[1] != size_inv )
    {
        WPRINT_WWD_DEBUG(("Received a packet with a frametag which is wrong\r\n"));
        host_buffer_release( buffer, WICED_NETWORK_RX );
        return;
    }

    /* Check whether the size is big enough to contain the SDPCM header */
    if ( size < (uint16_t) 12 )
    {
        WPRINT_WWD_DEBUG(("Received a packet that is too small to contain anything useful\r\n"));
        host_buffer_release( buffer, WICED_NETWORK_RX );
        return;
    }

    wiced_process_bus_credit_update((uint8_t*)&packet->sdpcm_header.frametag);

    if ( size == (uint16_t) 12 )
    {
        /* This is a flow control update packet with no data - release it. */
        host_buffer_release( buffer, WICED_NETWORK_RX );
        return;
    }

    /* Check the SDPCM channel to decide what to do with packet. */
    switch ( packet->sdpcm_header.sw_header.channel_and_flags & 0x0f )
    {
        case CONTROL_HEADER:  /* IOCTL/IOVAR reply packet */
            add_sdpcm_log_entry( LOG_RX, IOCTL, host_buffer_get_current_piece_size( buffer ), (char*) host_buffer_get_current_piece_data_pointer( buffer ) );

            /* Check that packet size is big enough to contain the CDC header as well as the SDPCM header */
            if ( packet->sdpcm_header.frametag[0] < ( sizeof( packet->sdpcm_header.frametag ) + sizeof(sdpcm_sw_header_t) + sizeof(sdpcm_cdc_header_t) ) )
            {
                /* Received a too-short SDPCM packet! */
                WPRINT_WWD_DEBUG(("Received a too-short SDPCM packet!\r\n"));
                host_buffer_release( buffer, WICED_NETWORK_RX );
                break;
            }

            /* Ignore a reply for each aborted ioctl attempt to ensure wiced_ioctl_sleep semaphore does not get out of sync */
            if (ioctl_abort_count != 0)
            {
                --ioctl_abort_count;
                host_buffer_release( buffer, WICED_NETWORK_RX );
                break;
            }

            /* Save the response packet in a global variable */
            wiced_ioctl_response = buffer;

            WICED_LOG( ( "Wcd:< Procd pkt 0x%08X: IOCTL Response (%d bytes)\r\n", (unsigned int)buffer, size ) );

            /* Wake the thread which sent the IOCTL/IOVAR so that it will resume */
            host_rtos_set_semaphore( &wiced_ioctl_sleep, WICED_FALSE );
            break;

        case DATA_HEADER:
            {
                sdpcm_bdc_header_t* bdc_header;
                int32_t headers_len_below_payload;

                /* Check that the packet is big enough to contain SDPCM & BDC headers */
                if ( packet->sdpcm_header.frametag[0] <= (uint16_t)( SDPCM_HEADER_LEN + BDC_HEADER_LEN ) )
                {
                    WPRINT_WWD_DEBUG(("Packet too small to contain SDPCM + BDC headers\r\n"));
                    host_buffer_release( buffer, WICED_NETWORK_RX );
                    break;
                }

                /* Calculate where the BDC header is - this is dependent on the data offset field of the SDPCM SW header */
                bdc_header = (sdpcm_bdc_header_t*) &packet->data[ packet->sdpcm_header.sw_header.header_length - SDPCM_HEADER_LEN ];

                /* Calculate where the payload is - this is dependent on the data offset fields of the SDPCM SW header and the BDC header */
                headers_len_below_payload = (int32_t) ( (int32_t)sizeof(wiced_buffer_header_t) + (int32_t) packet->sdpcm_header.sw_header.header_length + (int32_t)BDC_HEADER_LEN + (int32_t)(bdc_header->data_offset<<2) );

                /* Move buffer pointer past gSPI, SDPCM, BCD headers and padding, so that the network stack or 802.11 monitor sees only the payload */
                if ( WICED_SUCCESS != host_buffer_add_remove_at_front( &buffer, headers_len_below_payload ) )
                {
                    WPRINT_WWD_DEBUG(("No space for headers without chaining. this should never happen\r\n"));
                    host_buffer_release( buffer, WICED_NETWORK_RX );
                    break;
                }

                add_sdpcm_log_entry( LOG_RX, DATA, host_buffer_get_current_piece_size( buffer ), (char*) host_buffer_get_current_piece_data_pointer( buffer ) );
                WICED_LOG( ( "Wcd:< Procd pkt 0x%08X: Data (%d bytes)\r\n", (unsigned int)buffer, size ) );


                /* Check if we are in monitor mode */
                if ( monitor_mode_enabled == WICED_TRUE )
                {
                    host_network_process_raw_packet( buffer, WICED_STA_INTERFACE );
                }
                else
                {
                    /* Otherwise an Ethernet data frame received */

					if ( bdc_header->priority > sdpcm_highest_rx_tos )
					{
						sdpcm_highest_rx_tos = bdc_header->priority;
					}

					/* Send packet to bottom of network stack */
					host_network_process_ethernet_data( buffer, ( wiced_wifi_is_packet_from_ap( bdc_header->flags2 ) == WICED_TRUE ) ? WICED_AP_INTERFACE : WICED_STA_INTERFACE );
					}
            }
            break;

        case ASYNCEVENT_HEADER:
            {
                sdpcm_bdc_header_t* bdc_header;
                uint16_t ether_type;
                bcm_event_t* event;
                wiced_event_header_t* wiced_event;
                sdpcm_ethernet_header_t* ethernet_header;

                bdc_header = (sdpcm_bdc_header_t*) &packet->data[ packet->sdpcm_header.sw_header.header_length - SDPCM_HEADER_LEN ];

                ethernet_header = (sdpcm_ethernet_header_t*) &((uint8_t*)bdc_header)[ 4 + (bdc_header->data_offset<<2) ];

                ether_type = NTOH16( ethernet_header->ethertype );

                /* If frame is truly an event, it should have EtherType equal to the Broadcom type. */
                if ( ether_type != (uint16_t)ETHER_TYPE_BRCM )
                {
                    WPRINT_WWD_DEBUG(("Error - received a channel 1 packet which was not BRCM ethertype\r\n"));
                    host_buffer_release( buffer, WICED_NETWORK_RX );
                    break;
                }

                /* If ethertype is correct, the contents of the ethernet packet
                 * are a structure of type bcm_event_t
                 */
                event = (bcm_event_t*) &ethernet_header[1];

                /* Check that the OUI matches the Broadcom OUI */
                if ( 0 != memcmp( BRCM_OUI, &event->bcm_hdr.oui[0], (size_t)DOT11_OUI_LEN ) )
                {
                    WPRINT_WWD_DEBUG(("Event OUI mismatch\r\n"));
                    host_buffer_release( buffer, WICED_NETWORK_RX );
                    break;
                }

                wiced_event = (wiced_event_header_t*) &event->event;

                /* Search for the event type in the list of event handler functions
                 * event data is stored in network endianness
                 */
                wiced_event->flags      =                        NTOH16( wiced_event->flags      );
                wiced_event->event_type = (wiced_event_num_t)    NTOH32( wiced_event->event_type );
                wiced_event->status     = (wiced_event_status_t) NTOH32( wiced_event->status     );
                wiced_event->reason     = (wiced_event_reason_t) NTOH32( wiced_event->reason     );
                wiced_event->auth_type  =                        NTOH32( wiced_event->auth_type  );
                wiced_event->datalen    =                        NTOH32( wiced_event->datalen    );

                /* This is necessary because people who defined event statuses and reasons overlapped values. */
                if ( wiced_event->event_type == WLC_E_PSK_SUP )
                {
                    wiced_event->status = (wiced_event_status_t) ( (int)wiced_event->status + WLC_SUP_STATUS_OFFSET   );
                    wiced_event->reason = (wiced_event_reason_t) ( (int)wiced_event->reason + WLC_E_SUP_REASON_OFFSET );
                }
                else if ( wiced_event->event_type == WLC_E_PRUNE )
                {
                    wiced_event->reason = (wiced_event_reason_t) ( (int)wiced_event->reason + WLC_E_PRUNE_REASON_OFFSET );
                }
                else if ( ( wiced_event->event_type == WLC_E_DISASSOC ) || ( wiced_event->event_type == WLC_E_DEAUTH ) )
                {
                    wiced_event->status = (wiced_event_status_t) ( (int)wiced_event->status + WLC_DOT11_SC_STATUS_OFFSET   );
                    wiced_event->reason = (wiced_event_reason_t) ( (int)wiced_event->reason + WLC_E_DOT11_RC_REASON_OFFSET );
                }


                wiced_event->interface = event->event.ifidx;

                for ( i = 0; i < (uint16_t) WICED_EVENT_HANDLER_LIST_SIZE; i++ )
                {
                    if ( event_list[i].events != NULL )
                    {
                        for ( j = 0; event_list[i].events[j] != WLC_E_NONE; ++j )
                        {
                            if ( (uint32_t) event_list[i].events[j] == event->event.event_type )
                            {
                                /* Correct event type has been found - call the handler function and exit loop */
                                event_list[i].handler_user_data = event_list[i].handler( wiced_event, (uint8_t*)event->data, event_list[i].handler_user_data );
                                /*@innerbreak@*/
                                break;
                            }
                        }
                    }
                }

                add_sdpcm_log_entry( LOG_RX, EVENT, host_buffer_get_current_piece_size( buffer ), (char*) host_buffer_get_current_piece_data_pointer( buffer ) ); WICED_LOG( ( "Wcd:< Procd pkt 0x%08X: Evnt %d (%d bytes)\r\n", (unsigned int)buffer, (int)event->event.event_type, size ) );

                /* Release the event packet buffer */
                host_buffer_release( buffer, WICED_NETWORK_RX );
            }
            break;

        default:
            WPRINT_WWD_DEBUG(("SDPCM packet of unknown channel received - dropping packet\r\n"));
            host_buffer_release( buffer, WICED_NETWORK_RX );
            break;
    }
}


/** Sends an IOCTL command
 *
 *  Sends a I/O Control command to the Broadcom 802.11 device.
 *  The data which is set or retrieved must be in a format structure which is appropriate for the particular
 *  I/O control being sent. These structures can only be found in the DHD source code such as wl/exe/wlu.c.
 *  The I/O control will always respond with a packet buffer which may contain data in a format specific to
 *  the I/O control being used.
 *
 *  @Note: The caller is responsible for releasing the response buffer.
 *  @Note: The function blocks until the IOCTL has completed
 *  @Note: Only one IOCTL may happen simultaneously.
 *
 *  @param type       : SDPCM_SET or SDPCM_GET - indicating whether to set or get the I/O control
 *  @param send_buffer_hnd : A handle for a packet buffer containing the data value to be sent.
 *  @param response_buffer_hnd : A pointer which will receive the handle for the packet buffer containing the response data value received..
 *  @param interface : Which interface to send the iovar to (SDPCM_STA_INTERFACE or SDPCM_AP_INTERFACE)
 *
 *  @return    WICED_SUCCESS or WICED_ERROR
 */

wiced_result_t wiced_send_ioctl( sdpcm_command_type_t type, uint32_t command, wiced_buffer_t send_buffer_hnd, /*@null@*/ /*@out@*/ wiced_buffer_t* response_buffer_hnd, sdpcm_interface_t interface ) /*@releases send_buffer_hnd@*/
{
    uint32_t data_length;
    uint32_t flags;
    uint16_t id;
    wiced_result_t retval;
    sdpcm_control_packet_t* send_packet;
    sdpcm_common_packet_t*  recv_packet;
    sdpcm_cdc_header_t* cdc_header;

    /* Acquire mutex which prevents multiple simultaneous IOCTLs */
    retval = host_rtos_get_semaphore( &wiced_sdpcm_ioctl_mutex, NEVER_TIMEOUT, WICED_FALSE );
    if ( retval != WICED_SUCCESS )
    {
        host_buffer_release( send_buffer_hnd, WICED_NETWORK_TX );
        return retval;
    }

    /* Get the data length and cast packet to a CDC SDPCM header */
    data_length = host_buffer_get_current_piece_size( send_buffer_hnd ) - sizeof(sdpcm_packet_header_t) - sizeof(sdpcm_cdc_header_t);
    send_packet      = (sdpcm_control_packet_t*) host_buffer_get_current_piece_data_pointer( send_buffer_hnd );

    /* Check if IOCTL is actually IOVAR */
    if ( command == WLC_SET_VAR || command == WLC_GET_VAR )
    {
        uint8_t a = 0;

        /* Calculate the offset added to compensate for IOVAR string creating unaligned data section */
        while ( send_packet->data[a] == 0 )
        {
            ++a;
        }
        if (a != 0)
        {
            data_length -= a;
            memmove(send_packet->data, &send_packet->data[a], data_length);
            host_buffer_set_data_end(send_buffer_hnd, send_packet->data + data_length);
        }
    }

    /* Prepare the CDC header */
    send_packet->cdc_header.cmd    = command;
    send_packet->cdc_header.len    = data_length;
    send_packet->cdc_header.flags  = ( ( (uint32_t) ++requested_ioctl_id << CDCF_IOC_ID_SHIFT ) & CDCF_IOC_ID_MASK ) | type | (uint32_t) interface << 12;
    send_packet->cdc_header.status = 0;

    WICED_LOG( ( "Wcd:> IOCTL pkt 0x%08X: cmd %d, len %d\r\n", (unsigned int)send_buffer_hnd, (int)command, data_length ) );

    /* Store the length of the data and the IO control header and pass "down" */
    wiced_send_sdpcm_common( send_buffer_hnd, CONTROL_HEADER );

    do
    {
        /* Wait till response has been received  */
        retval = host_rtos_get_semaphore( &wiced_ioctl_sleep, (uint32_t) WICED_IOCTL_TIMEOUT_MS, WICED_FALSE );
        if ( retval != WICED_SUCCESS )
        {
            ++ioctl_abort_count;
            /* Release the mutex since Wiced_IOCTL_Response will no longer be referenced. */
            host_rtos_set_semaphore( &wiced_sdpcm_ioctl_mutex, WICED_FALSE );
            return retval;
        }

        /* Cast the response to a CDC + SDPCM header */
        recv_packet = (sdpcm_common_packet_t*) host_buffer_get_current_piece_data_pointer( wiced_ioctl_response );
        cdc_header = (sdpcm_cdc_header_t*) &recv_packet->data[ recv_packet->sdpcm_header.sw_header.header_length - SDPCM_HEADER_LEN ];
        flags = ltoh32( cdc_header->flags );
        id = (uint16_t) ( ( flags & CDCF_IOC_ID_MASK ) >> CDCF_IOC_ID_SHIFT );

        /* Check that the IOCTL identifier matches the identifier that was sent */
        if ( id != requested_ioctl_id )
        {
            host_buffer_release( wiced_ioctl_response, WICED_NETWORK_RX );
            WPRINT_WWD_DEBUG(("Received a response for a different IOCTL - retry\r\n"));
        }
    } while ( id != requested_ioctl_id );


    retval = (wiced_result_t) ltoh32( cdc_header->status );

    /* Check if the caller wants the response */
    if ( response_buffer_hnd != NULL )
    {
        *response_buffer_hnd = wiced_ioctl_response;
        host_buffer_add_remove_at_front( response_buffer_hnd, (int32_t) IOCTL_OFFSET );
    }
    else
    {
        host_buffer_release( wiced_ioctl_response, WICED_NETWORK_RX );
        recv_packet = 0; /* Note: packet will no longer be valid after freeing buffer */
    }

    wiced_ioctl_response = NULL;

    /* Release the mutex since Wiced_IOCTL_Response will no longer be referenced. */
    host_rtos_set_semaphore( &wiced_sdpcm_ioctl_mutex, WICED_FALSE );


    /* Check whether the IOCTL response indicates it failed. */
    if ( ( flags & CDCF_IOC_ERROR ) != 0)
    {
        if ( response_buffer_hnd != NULL )
        {
            host_buffer_release( *response_buffer_hnd, WICED_NETWORK_RX );
            *response_buffer_hnd = NULL;
        }
        return retval;
    }

    return WICED_SUCCESS;
}

/**
 * Registers a handler to receive event callbacks.
 *
 * This function registers a callback handler to be notified when
 * a particular event is received.
 *
 * Alternately the function clears callbacks for given event type.
 *
 * @note : Currently each event may only be registered to one handler
 *         and there is a limit to the number of simultaneously registered
 *         events
 *
 * @param  event_nums     An array of event types that is to trigger the handler. The array must be terminated with a WLC_E_NONE event
 *                        See @ref wiced_event_num_enum for available events
 * @param handler_func   A function pointer to the new handler callback,
 *                        or NULL if callbacks are to be disabled for the given event type
 * @param handler_user_data  A pointer value which will be passed to the event handler function
 *                            at the time an event is triggered (NULL is allowed)
 *
 * @return WICED_SUCCESS or WICED_ERROR
 */
wiced_result_t wiced_management_set_event_handler( const wiced_event_num_t* event_nums, /*@null@*/ wiced_event_handler_t handler_func, /*@null@*/ void* handler_user_data )
{
    return wwd_management_set_event_handler(event_nums, handler_func, handler_user_data, WICED_STA_INTERFACE);
}

wiced_result_t wwd_management_set_event_handler( const wiced_event_num_t* event_nums, /*@null@*/ wiced_event_handler_t handler_func, /*@null@*/ void* handler_user_data, wiced_interface_t interface )
{
    wiced_buffer_t buffer;
    uint8_t* event_mask;
    uint16_t entry = (uint16_t) 0xFF;
    uint16_t i;
    uint16_t j;
    wiced_result_t res;
    uint32_t* data;

    /* Find an existing matching entry OR the next empty entry */
    for ( i = 0; i < (uint16_t) WICED_EVENT_HANDLER_LIST_SIZE; i++ )
    {
        /* Find a matching event list OR the first empty event entry */
        if ( event_list[i].events == event_nums )
        {
            /* Delete the entry */
            event_list[i].events = NULL;
            event_list[i].handler = NULL;
            event_list[i].handler_user_data = NULL;

            entry = i;
            break;
        }
        else if ( ( entry == (uint16_t) 0xFF ) && ( event_list[i].events == NULL ) )
        {
            entry = i;
        }
    }

    /* Check if handler function was provided */
    if ( handler_func != NULL )
    {
        /* Check if an empty entry was not found */
        if ( entry == (uint16_t) 0xFF )
        {
            WPRINT_WWD_DEBUG(("Out of space in event handlers table - try increasing WICED_EVENT_HANDLER_LIST_SIZE\r\n"));
            return WICED_ERROR;
        }

        /* Add the new handler in at the free space */
        event_list[entry].handler           = handler_func;
        event_list[entry].handler_user_data = handler_user_data;
        event_list[entry].events            = event_nums;
    }

    /* Send the new event mask value to the wifi chip */
    data = (uint32_t*) wiced_get_iovar_buffer( &buffer, (uint16_t) 16 + 4, "bsscfg:" IOVAR_STR_EVENT_MSGS );
    data[0] = interface;
    event_mask = (uint8_t*)&data[1];
    if ( event_mask == NULL )
    {
        return WICED_BUFFER_UNAVAILABLE_PERMANENT;
    }

    /* Keep the wlan awake while we set the event_msgs */
    ++wiced_wlan_status.keep_wlan_awake;

    /* Set the event bits for each event from every handler */
    memset( event_mask, 0, (size_t) 16 );
    for ( i = 0; i < (uint16_t) WICED_EVENT_HANDLER_LIST_SIZE; i++ )
    {
        if ( event_list[i].events != NULL )
        {
            for ( j = 0; event_list[i].events[j] != WLC_E_NONE; j++ )
            {
                setbit(event_mask, event_list[i].events[j]);
            }
        }
    }
    res = wiced_send_iovar( SDPCM_SET, buffer, 0, SDPCM_STA_INTERFACE );

    /* The wlan chip can sleep from now on */
    --wiced_wlan_status.keep_wlan_awake;

    return res;
}

/** A helper function to easily acquire and initialise a buffer destined for use as an iovar
 *
 * @param  buffer      : A pointer to a wiced_buffer_t object where the created buffer will be stored
 * @param  data_length : The length of space reserved for user data
 * @param  name        : The name of the iovar
 *
 * @return A pointer to the start of user data with data_length space available
 */
/*@exposed@*/ /*@null@*/ void* wiced_get_iovar_buffer( /*@returned@*/ /*@out@*/ wiced_buffer_t* buffer, uint16_t data_length, const char* name )  /*@allocates buffer@*/ /*@defines buffer@*/
{
    uint32_t name_length = strlen( name ) + 1; /* + 1 for terminating null */
    uint32_t name_length_alignment_offset = (64 - name_length) % sizeof(uint32_t);
    if ( host_buffer_get( buffer, WICED_NETWORK_TX, (unsigned short) ( IOCTL_OFFSET + data_length + name_length + name_length_alignment_offset ), WICED_TRUE ) == WICED_SUCCESS )
    {
        uint8_t* data = ( host_buffer_get_current_piece_data_pointer( *buffer ) + IOCTL_OFFSET );
        memset( data, 0, name_length_alignment_offset );
        memcpy( data + name_length_alignment_offset, name, name_length );
        return ( data + name_length + name_length_alignment_offset );
    }
    else
    {
        WPRINT_WWD_DEBUG(("Error - failed to allocate a packet buffer for IOVAR\r\n"));
        return NULL;
    }
}

/** A helper function to easily acquire and initialise a buffer destined for use as an ioctl
 *
 * @param  buffer      : A pointer to a wiced_buffer_t object where the created buffer will be stored
 * @param  data_length : The length of space reserved for user data
 *
 * @return A pointer to the start of user data with data_length space available
 */
/*@null@*/ void* wiced_get_ioctl_buffer( /*@returned@*/ /*@out@*/ wiced_buffer_t* buffer, uint16_t data_length )  /*@allocates buffer@*/
{
    if ( host_buffer_get( buffer, WICED_NETWORK_TX, (unsigned short) ( IOCTL_OFFSET + data_length ), WICED_TRUE ) == WICED_SUCCESS )
    {
        return ( host_buffer_get_current_piece_data_pointer( *buffer ) + IOCTL_OFFSET );
    }
    else
    {
        WPRINT_WWD_DEBUG(("Error - failed to allocate a packet buffer for IOCTL\r\n"));
        return NULL;
    }
}


wiced_result_t wiced_get_packet_to_send(wiced_buffer_t* buffer)
{
    sdpcm_common_packet_t* packet;
    if ( wiced_sdpcm_send_queue_head != NULL )
    {
        /* Check if we're being flow controlled */
        if ( wiced_bus_is_flow_controlled() == WICED_TRUE )
        {
            return WICED_ERROR;
        }

        /* Check if we have enough bus data credits spare */
        if ( sdpcm_packet_transmit_sequence_number == last_bus_data_credit )
        {
            return WICED_ERROR;
        }

        /* There is a packet waiting to be sent - send it then fix up queue and release packet */
        if ( host_rtos_get_semaphore( &wiced_sdpcm_send_queue_mutex, NEVER_TIMEOUT, WICED_FALSE ) != WICED_SUCCESS )
        {
            /* Could not obtain mutex, push back the flow control semaphore */
            return WICED_ERROR;
        }

        /* Pop the head off and set the new send_queue head */
        *buffer = wiced_sdpcm_send_queue_head;
        wiced_sdpcm_send_queue_head = wiced_get_next_buffer_in_queue( *buffer );
        if ( wiced_sdpcm_send_queue_head == NULL )
        {
            wiced_sdpcm_send_queue_tail = NULL;
        }
        host_rtos_set_semaphore( &wiced_sdpcm_send_queue_mutex, WICED_FALSE );

        /* Set the sequence number */
        packet = (sdpcm_common_packet_t*) host_buffer_get_current_piece_data_pointer( *buffer );
        packet->sdpcm_header.sw_header.sequence = sdpcm_packet_transmit_sequence_number;
        sdpcm_packet_transmit_sequence_number++;

        return WICED_SUCCESS;
    }
    else
    {
        return WICED_ERROR;
    }
}


/** Returns the number of bus credits available
 *
 * @return The number of bus credits available
 */
uint8_t wiced_get_available_bus_credits( void )
{
    return (uint8_t)( last_bus_data_credit - sdpcm_packet_transmit_sequence_number );
}


/******************************************************
 *             Static Functions
 ******************************************************/

/** Writes SDPCM headers and sends packet to Wiced Thread
 *
 *  Prepends the given packet with a new SDPCM header,
 *  then passes the packet to the Wiced thread via @ref wiced_thread_send_data
 *
 *  This function is called by @ref wiced_network_send_ethernet_data and @ref wiced_send_ioctl
 *
 *  @param buffer     : The handle of the packet buffer to send
 *  @param header_type  : DATA_HEADER, ASYNCEVENT_HEADER or CONTROL_HEADER - indicating what type of SDPCM packet this is.
 */

static void wiced_send_sdpcm_common( /*@only@*/ wiced_buffer_t buffer, sdpcm_header_type_t header_type )
{
    uint16_t size;
    sdpcm_common_packet_t* packet = (sdpcm_common_packet_t *) host_buffer_get_current_piece_data_pointer( buffer );

    size = host_buffer_get_current_piece_size( buffer );

#ifdef SUPPORT_BUFFER_CHAINING
    wiced_buffer_t tmp_buff;
    while ( NULL != (tmp_buff = host_buffer_get_next_piece( tmp_buff )
    {
        size += host_buffer_get_current_piece_size( tmp_buff )
    }
#endif /* ifdef SUPPORT_BUFFER_CHAINING */

    size = (uint16_t) ( size - (uint16_t) sizeof(wiced_buffer_header_t) );

    /* Prepare the SDPCM header */
    memset( (uint8_t*) &packet->sdpcm_header, 0, sizeof(sdpcm_packet_header_t) );
    packet->sdpcm_header.sw_header.channel_and_flags = (uint8_t) header_type;
    packet->sdpcm_header.sw_header.header_length = ( header_type == DATA_HEADER ) ? sizeof(sdpcm_header_t) + 2 : sizeof(sdpcm_header_t);
    packet->sdpcm_header.sw_header.sequence = 0; /* Note: The real sequence will be written later */
    packet->sdpcm_header.frametag[0] = size;
    packet->sdpcm_header.frametag[1] = (uint16_t) ~size;

    add_sdpcm_log_entry( LOG_TX, ( header_type == DATA_HEADER )? DATA : ( header_type == CONTROL_HEADER)? IOCTL : EVENT, host_buffer_get_current_piece_size( buffer ), (char*) host_buffer_get_current_piece_data_pointer( buffer ) );

//    /* Remember the last packet sequence we used and increment it
//     */
//    sdpcm_packet_transmit_sequence_number++;

    /* Add the length of the SDPCM header and pass "down" */
    if ( host_rtos_get_semaphore( &wiced_sdpcm_send_queue_mutex, NEVER_TIMEOUT, WICED_FALSE ) != WICED_SUCCESS )
    {
        /* Could not obtain mutex */
        /* Fatal error */
        host_buffer_release(buffer, WICED_NETWORK_TX);
        return;
    }

    wiced_set_next_buffer_in_queue( NULL, buffer );
    if ( wiced_sdpcm_send_queue_tail != NULL )
    {
        wiced_set_next_buffer_in_queue( buffer, wiced_sdpcm_send_queue_tail );
    }
    wiced_sdpcm_send_queue_tail = buffer;
    if ( wiced_sdpcm_send_queue_head == NULL )
    {
        wiced_sdpcm_send_queue_head = buffer;
    }
    host_rtos_set_semaphore( &wiced_sdpcm_send_queue_mutex, WICED_FALSE );

    wiced_thread_notify();
}


static wiced_buffer_t wiced_get_next_buffer_in_queue( wiced_buffer_t buffer )
{
    wiced_buffer_header_t* packet = (wiced_buffer_header_t*) host_buffer_get_current_piece_data_pointer( buffer );
    return packet->queue_next;
}


/** Sets the next buffer in the send queue
 *
 *  The send queue is a linked list of packet buffers where the 'next' pointer
 *  is stored in the first 4 bytes of the buffer content.
 *  This function sets that pointer.
 *
 * @param buffer       : handle of packet in the send queue
 *        prev_buffer  : handle of new packet whose 'next' pointer will point to 'buffer'
 */
static void wiced_set_next_buffer_in_queue( wiced_buffer_t buffer, wiced_buffer_t prev_buffer )
{
    wiced_buffer_header_t* packet = (wiced_buffer_header_t*) host_buffer_get_current_piece_data_pointer( prev_buffer );
    packet->queue_next = buffer;
}

WEAK void host_network_process_raw_packet( wiced_buffer_t buffer, wiced_interface_t interface )
{
    UNUSED_PARAMETER( interface );

    host_buffer_release( buffer, WICED_NETWORK_RX );
}
