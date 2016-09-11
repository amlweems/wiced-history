/**************************************************************************/ 
/*                                                                        */ 
/*            Copyright (c) 1996-2012 by Express Logic Inc.               */ 
/*                                                                        */ 
/*  This software is copyrighted by and is the sole property of Express   */ 
/*  Logic, Inc.  All rights, title, ownership, or other interests         */ 
/*  in the software remain the property of Express Logic, Inc.  This      */ 
/*  software may only be used in accordance with the corresponding        */ 
/*  license agreement.  Any unauthorized use, duplication, transmission,  */ 
/*  distribution, or disclosure of this software is expressly forbidden.  */ 
/*                                                                        */
/*  This Copyright notice may not be removed or modified without prior    */ 
/*  written consent of Express Logic, Inc.                                */ 
/*                                                                        */ 
/*  Express Logic, Inc. reserves the right to modify this software        */ 
/*  without notice.                                                       */ 
/*                                                                        */ 
/*  Express Logic, Inc.                     info@expresslogic.com         */
/*  11423 West Bernardo Court               http://www.expresslogic.com   */
/*  San Diego, CA  92127                                                  */
/*                                                                        */
/**************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */ 
/** NetX Component                                                        */
/**                                                                       */
/**   Dynamic Host Configuration Protocol over IPv6 (DHCPv6)              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */ 
/*                                                                        */ 
/*    nx_dhcpv6.h                                         PORTABLE C      */ 
/*                                                           5.3          */  
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX Dynamic Host Configuration Protocol over */ 
/*    IPv6 (DHCPv6) component, including all data types and external      */ 
/*    references. It is assumed that nx_api.h and nx_port.h have already  */ 
/*    been included.                                                      */
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  06-01-2010     William E. Lamie         Initial Version 5.0           */ 
/*  03-01-2011     Janet Christiansen       Modified comment(s),          */
/*                                            added multihome support,    */
/*                                            resulting in version 5.1    */
/*  10-10-2011     Janet Christiansen       Modified comment(s),          */
/*                                            added multihome support,    */
/*                                            resulting in version 5.2    */
/*  01-12-2012     Janet Christiansen       Modified comment(s),          */
/*                                            corrected default value of  */
/*                                            NX_DHCPV6_TIME_INTERVAL,    */
/*                                            added a BOUND state to the  */
/*                                            DHCPv6 Client set of states,*/
/*                                            added an address index to   */
/*                                            the DHCPv6 Client struct,   */
/*                                            added new services,         */
/*                                            and stack size argument to  */
/*                                            nx_dhcpv6_client_create,    */
/*                                            removed address_status in   */
/*                                            DHCPv6 Client 'get'         */
/*                                            services, removed debug     */
/*                                            output macros,              */
/*                                            resulting in version 5.3    */
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_DHCPV6_CLIENT_H
#define NX_DHCPV6_CLIENT_H

/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */
extern   "C" {

#endif

  /* Verify NetX Duo version.  */
#if (((__NETXDUO_MAJOR_VERSION__  >= 5) && (__NETXDUO_MINOR_VERSION__  >= 6)))
#define  NETXDUO_MULTIHOME_SUPPORT
#endif  /* NETXDUO VERSION check */

/* If not NetX Duo 5.6 or higher, no multihome devices supported (well, only the primary interface) */
#ifndef NETXDUO_MULTIHOME_SUPPORT
#undef MULTI_HOMED_DEVICE
#endif

/* Define the DHCPv6 ID "DHC6" that is used to mark the DHCPv6 structure as created.  */

#define NX_DHCPV6_ID                      0x44484336UL


#ifndef NETXDUO_MULTIHOME_SUPPORT

/* Set up address index defines for NetX Duo. */
#define     LINK_LOCAL_INTERFACE    0
#define     GLOBAL_IP_INTERFACE     1 

#endif


/* Set the Client lease time. An infinate lease time is not recommended by the RFC 
   unless the Client requires a permanent IP address.  Most servers will likely not
   grant an infinite IP address lease. */

#define NX_DHCPV6_INFINITE_LEASE                0xffffffffUL
#define NX_DHCPV6_MULTICAST_MASK                0xff000000UL

/* Define internal DHCPv6 option flags. */

#define NX_DHCPV6_DNS_SERVER_OPTION             0x00000010UL    /* Option code for requesting DNS server IP address  */
#define NX_DHCPV6_TIME_SERVER_OPTION            0x00000008UL    /* Option code for requesting time server IP address. */
#define NX_DHCPV6_TIME_ZONE_OPTION              0x00000004UL    /* Option code for requesting Time zone. */
#define NX_DHCPV6_DOMAIN_NAME_OPTION            0x00000002UL    /* Option code for requesting domain name. */

/* Define RFC mandated (draft only actually) option codes. */
#define NX_DHCPV6_RFC_DNS_SERVER_OPTION         0x00000017UL    /* RFC Option code for requesting DNS server IP address  */
#define NX_DHCPV6_RFC_TIME_SERVER_OPTION        0x0000001FUL    /* RFC Option code for requesting tme server IP address. */
#define NX_DHCPV6_RFC_TIME_ZONE_OPTION          0x00000029UL    /* RFC Option code for requesting Time zone. */
#define NX_DHCPV6_RFC_DOMAIN_NAME               0x00000018UL    /* RFC Option code for requesting domain name. */

typedef enum 
{
    NX_DHCPV6_DUID_TYPE_LINK_TIME =                     1,
    NX_DHCPV6_DUID_TYPE_VENDOR_ASSIGNED, 
    NX_DHCPV6_DUID_TYPE_LINK_ONLY

} NX_DHCPV6_DUID_TYPE;


/* Define hardware types -  RFC 2464 */

#define NX_DHCPV6_HW_TYPE_IEEE_802                      0x06


/* Define approximate time since Jan 1, 2000 for computing DUID time. This will form the 
   basis for the DUID time ID field.  */

#define SECONDS_SINCE_JAN_1_2000_MOD_32                 2563729999u


/* Define the DHCPv6 Message Types.  */

#define NX_DHCPV6_MESSAGE_TYPE_SOLICIT                  1
#define NX_DHCPV6_MESSAGE_TYPE_ADVERTISE                2
#define NX_DHCPV6_MESSAGE_TYPE_REQUEST                  3
#define NX_DHCPV6_MESSAGE_TYPE_CONFIRM                  4
#define NX_DHCPV6_MESSAGE_TYPE_RENEW                    5
#define NX_DHCPV6_MESSAGE_TYPE_REBIND                   6
#define NX_DHCPV6_MESSAGE_TYPE_REPLY                    7
#define NX_DHCPV6_MESSAGE_TYPE_RELEASE                  8
#define NX_DHCPV6_MESSAGE_TYPE_DECLINE                  9
#define NX_DHCPV6_MESSAGE_TYPE_RECONFIGURE              10
#define NX_DHCPV6_MESSAGE_TYPE_INFORM_REQUEST           11


/* Define the DHCPv6 Options.  */

#define NX_DHCPV6_OP_DUID_CLIENT                        1         /* Client DUID (DHCP unique identifier) */
#define NX_DHCPV6_OP_DUID_SERVER                        2         /* Server DUID (DHCP unique identifier) */
#define NX_DHCPV6_OP_IA_NA                              3         /* Identity association for non temporary addresses */
#define NX_DHCPV6_OP_IA_TA                              4         /* Identity association for temporary addresses */  
#define NX_DHCPV6_OP_IA_ADDRESS                         5         /* Address associated with IA_NA or IA_TA */
#define NX_DHCPV6_OP_OPTION_REQUEST                     6         /* Identifies a list of options */
#define NX_DHCPV6_OP_PREFERENCE                         7         /* Server's means of affecting Client choice of servers. */
#define NX_DHCPV6_OP_ELAPSED_TIME                       8         /* Duration of Client exchange with DHCPv6 server  */
#define NX_DHCPV6_OP_RELAY_MESSAGE                      9         /* Not in use in NetX DHCPV6 */
#define NX_DHCPV6_OP_AUTHENTICATION                     11        /* Not in use in NetX DHCPV6 */
#define NX_DHCPV6_OP_SERVER_UNICAST                     12        /* Server ok's allowing the client to address it in Unicast */
#define NX_DHCPV6_OP_OPTION_STATUS                      13        /* Option request status.  */



/* RFC defined DHCPv6 server status codes */

#define NX_DHCPV6_SUCCESS                               0           /* Server indicates Client DHCPv6 request is granted. */
#define NX_DHCPV6_UNSPECIFIED_FAILURE                   1           /* Unspecified reason e.g. not found in RFC 3315 */
#define NX_DHCPV6_NO_ADDRESS_AVAILABLE                  2           /* Server unable to assign IP address because none are available. */
#define NX_DHCPV6_NO_BINDING                            3           /* Client record (binding) unavailable */
#define NX_DHCPV6_NOT_ON_LINK                           4           /* Client's IPv6 address is not on the Server link */
#define NX_DHCPV6_USE_MULTICAST                         5           /* Server indicating Client must use multicast ALL_SERVERS address to get IP address */

/* Internal DHCPv6 Client status codes */

#define NX_DHCPV6_STATE_INIT                            1           /* Client state with no bound IP address */
#define NX_DHCPV6_STATE_SENDING_SOLICIT                 2           /* Client sends Sollicit to identify a DHCP server */
#define NX_DHCPV6_STATE_SENDING_REQUEST                 3           /* Address requested, Client initiating a request after receiving server advertisement */
#define NX_DHCPV6_STATE_SENDING_RENEWAL                 4           /* Address established, Client is initiating a renewal request */
#define NX_DHCPV6_STATE_SENDING_REBIND                  5           /* Address established, Client is initiating a rebind request */
#define NX_DHCPV6_STATE_SENDING_DECLINE                 6           /* Address was established but Client can't use it e.g. duplicate address check failed. */
#define NX_DHCPV6_STATE_SENDING_CONFIRM                 7           /* Client IP Address is established but Client requires confirmation its still ok */
#define NX_DHCPV6_STATE_SENDING_INFORM_REQUEST          8           /* Client IP Address is established but Client requests information other than IP address */
#define NX_DHCPV6_STATE_SENDING_RELEASE                 9           /* Requesting an IP address release of a recently assigned IP address. */
#define NX_DHCPV6_STATE_BOUND_TO_ADDRESS                15          /* Client is bound to an assigned address; DHCP Client task is basically idle. */

/* Internal DHCPv6 Client address status codes. */

#define NX_DHCPV6_CLIENT_ADDRESS_VALID                  1           /* Client global IP address is valid and registered with DHCPv6 server. */
#define NX_DHCPV6_CLIENT_ADDRESS_INVALID                2           /* Client does not have a valid address status with the DHCPv6 server. */


/* Internal DHCPv6 event flags.  These events are processed by the Client DHCPv6 thread. */

#define NX_DHCPV6_ALL_EVENTS                            0xFFFFFFFFUL    /* All Client DHCPv6 event flags */
#define NX_DHCPV6_IP_PERIODIC_EVENT                     0x00000008UL    /* Client DHCPv6 IP lease time keeper timeout has expired. */
#define NX_DHCPV6_SESSION_PERIODIC_EVENT                0x00000004UL    /* Client DHCPv6 IP session time keeper timeout has expired. */


/* RFC mandated DHCPv6 client and server ports.  */

#define NX_DHCPV6_SERVER_UDP_PORT                       547
#define NX_DHCPV6_CLIENT_UDP_PORT                       546


/* Internal error codes for DHCPv6 Client services.  */

#define NX_DHCPV6_TASK_SUSPENDED                        0xE90    /* DHCPv6 task suspended by host application. */
#define NX_DHCPV6_ALREADY_STARTED                       0xE91    /* DHCPv6 already started when API called to start it. */
#define NX_DHCPV6_NOT_STARTED                           0xE92    /* DHCPv6 was not started when API was called  */ 
#define NX_DHCPV6_PARAM_ERROR                           0xE93    /* Invalid non pointer input to API */

#define NX_DHCPV6_INVALID_CLIENT_DUID                   0xE95    /* Client DUID received from Server with invalid data or mismatches Client server DUID on record. */
#define NX_DHCPV6_INVALID_SERVER_DUID                   0xE96    /* Server DUID received by Client has bad syntax or missing data*/
#define NX_DHCPV6_MESSAGE_MISSING_DUID                  0xE97    /* Client receives a message type missing server or client DUID. */
#define NX_DHCPV6_UNSUPPORTED_DUID_TYPE                 0xE98    /* Client configuration involves a DUID type not supported by this API. */
#define NX_DHCPV6_UNSUPPORTED_DUID_HW_TYPE              0xE99    /* Client configuration involves a network hardware type not supported by this API. */

#define NX_DHCPV6_INVALID_IANA_TIME                     0xEA0    /* Server IA-NA option T1 vs T2 address lease time is invalid. */
#define NX_DHCPV6_MISSING_IANA_OPTION                   0xEA1    /* Client received IA address option not belonging to an IA block */
#define NX_DHCPV6_BAD_IANA_ID                           0xEA2    /* Server IA-NA option does not contain the Client's original IA-NA ID. */
#define NX_DHCPV6_INVALID_IANA_DATA                     0xEA3    /* Server IA-NA option block has bad syntax or missing data */
#define NX_DHCPV6_INVALID_IA_ADDRESS                    0xEA4    /* Client inquiring about an unknown IA address */
#define NX_DHCPV6_INVALID_IA_DATA                       0xEA5    /* Server IA address option block has bad syntax or missing data */
#define NX_DHCPV6_INVALID_IA_TIME                       0xEA6    /* Server IA option preferred vs valid lease time is invalid. */
#define NX_DHCPV6_INVALID_PREF_DATA                     0xEA7    /* Client received Preference block with missing data or bad syntax */
#define NX_DHCPV6_INCOMPLETE_OPTION_BLOCK               0xEA8    /* Empty option block data; either zero length or zero option parsed. */    
#define NX_DHCPV6_MISSING_REQUIRED_OPTIONS              0xEA9    /* Cannot start the DHCPv6 Client because required options are missing e.g. IANA, DUID etc */
#define NX_DHCPV6_INVALID_OPTION_DATA                   0xEAA    /* Client received option data with missing data or bad syntax */
#define NX_DHCPV6_UNKNOWN_OPTION                        0xEAB    /* Client received an unknown or unsupported option from server */
#define NX_DHCPV6_INVALID_SERVER_PACKET                 0xEAC    /* Server reply invalid e.g. bad port, invalid DHCP header  */
#define NX_DHCPV6_IA_ADDRESS_NOT_VALID                  0xEA4    /* Client not assigned an IP address from the DHCPv6 client */

#define NX_DHCPV6_UNKNOWN_PROCESS_STATE                 0xEB0    /* Internal DHCPv6 state machine in an unknown state */
#define NX_DHCPV6_ILLEGAL_MESSAGE_TYPE                  0xEB1    /* Client receives a message type intended for a DHCPv6 server e.g. REQUEST or CONFIRM */
#define NX_DHCPV6_UNKNOWN_MSG_TYPE                      0xEB2    /* NetX DHCPv6 receives an unknown message type  */
#define NX_DHCPV6_BAD_TRANSACTION_ID                    0xEB3    /* Client received message with bad transaction ID */
#define NX_DHCPV6_BAD_IPADDRESS_ERROR                   0xEB4    /* Unable to parse a valid IPv6 address from specified data buffer  */
#define NX_DHCPV6_PROCESSING_ERROR                      0xEB5    /* Server packet size received out of synch with NetX packet length - no assignment of blame */
#define NX_DHCPV6_INSUFFICIENT_PACKET_PAYLOAD           0xEB6    /* Client DHCPv6 message will not fit in Client packet pool packet buffer. */
#define NX_DHCPV6_INVALID_DATA_SIZE                     0xEB7    /* Attempting to parse too large a data object from DHCPv6 request. */

#define NX_DHCPV6_REACHED_MAX_RETRANSMISSION_COUNT      0xEC0    /* No response from server after maximum number of retries. */
#define NX_DHCPV6_REACHED_MAX_RETRANSMISSION_TIMEOUT    0xEC1    /* No response from server after maximum retry timeout. */

/* Define DHCPv6 timeout for checking DHCPv6 flag status. */
#define NX_DHCPV6_TIME_INTERVAL           (1 * _nx_system_ticks_per_second)   

/* Define the DHCP stack size.  */

#ifndef NX_DHCPV6_THREAD_STACK_SIZE
#define NX_DHCPV6_THREAD_STACK_SIZE             2048
#endif

/* Define the DHCP stack priority.  */

#ifndef NX_DHCPV6_THREAD_PRIORITY
#define NX_DHCPV6_THREAD_PRIORITY               2
#endif

/* Define the time out option to obtain a DHCPv6 Client mutex lock. If the 
   the Client appears to be locking up, this can be set to a finite value
   for debugging as well as restore responsiveness to the Client */

#ifndef NX_DHCPV6_MUTEX_WAIT
#define NX_DHCPV6_MUTEX_WAIT                    TX_WAIT_FOREVER 
#endif

/* Define the conversion between timer ticks and seconds (processor dependent). */

#ifndef NX_DHCPV6_TICKS_PER_SECOND
#define NX_DHCPV6_TICKS_PER_SECOND                      100
#endif


/* Define DHCPv6 Client record parameters */

/* Define the timer interval for the IP lifetime timer in seconds.  */

#ifndef NX_DHCPV6_IP_LIFETIME_TIMER_INTERVAL
#define NX_DHCPV6_IP_LIFETIME_TIMER_INTERVAL     60  
#endif


/* Define the timer interval in timer ticks for the session duration timer.  */

#ifndef NX_DHCPV6_SESSION_TIMER_INTERVAL
#define NX_DHCPV6_SESSION_TIMER_INTERVAL         10
#endif


/* Define the address renewal ("T1") request lifetime. */

#ifndef NX_DHCPV6_RENEW_TIME
#define NX_DHCPV6_RENEW_TIME                    NX_DHCPV6_INFINITE_LEASE          
#endif


/* Define the address rebind ("T2")request lifetime. */

#ifndef NX_DHCPV6_REBIND_TIME
#define NX_DHCPV6_REBIND_TIME                   NX_DHCPV6_INFINITE_LEASE          
#endif


/* Define the number of DNS name servers the Client will store. */

#ifndef NX_DHCPV6_NUM_DNS_SERVERS
#define NX_DHCPV6_NUM_DNS_SERVERS               2
#endif


/* Define the number of time servers the Client will store. */

#ifndef NX_DHCPV6_NUM_TIME_SERVERS
#define NX_DHCPV6_NUM_TIME_SERVERS              1
#endif


/* Define the buffer size for storing the DHCPv6 Client domain name. */

#ifndef NX_DHCPV6_DOMAIN_NAME_BUFFER_SIZE
#define NX_DHCPV6_DOMAIN_NAME_BUFFER_SIZE       30
#endif


/* Define the buffer size for storing the DHCPv6 Client time zone e.g. CET, PST etc. */

#ifndef NX_DHCPV6_TIME_ZONE_BUFFER_SIZE
#define NX_DHCPV6_TIME_ZONE_BUFFER_SIZE         10
#endif

/* Define the amount of packet payload to store DHCPv6 server messages. */

#ifndef NX_DHCPV6_MAX_MESSAGE_SIZE      
#define NX_DHCPV6_MAX_MESSAGE_SIZE                      100
#endif  


/* DHCPv6 Client Network Configuration */

/* Define the generic time out option for NetX operations (packet allocate, packet send.  */

#ifndef NX_DHCPV6_PACKET_TIME_OUT
#define NX_DHCPV6_PACKET_TIME_OUT               (3 * NX_DHCPV6_TICKS_PER_SECOND)
#endif


/* Define the DHCPv6 packet size. Should be large enough to hold IP and UDP headers,
   plus DHCPv6 data runs about 200 - 300 bytes for a typical exchange. */

#ifndef NX_DHCPV6_PACKET_SIZE 
#define NX_DHCPV6_PACKET_SIZE                    500  
#endif


/* Define the DHCPv6 packet memory area. Should be large enough for 1 packet per host
   assuming each response requires only one packet. */

#ifndef NX_DHCPV6_PACKET_POOL_SIZE 
#define NX_DHCPV6_PACKET_POOL_SIZE                  (10 * NX_DHCPV6_PACKET_SIZE)
#endif


/* Define UDP socket type of service.  */

#ifndef NX_DHCPV6_TYPE_OF_SERVICE
#define NX_DHCPV6_TYPE_OF_SERVICE                NX_IP_NORMAL
#endif


/* Define the UDP socket fragment option. */

#ifndef NX_DHCPV6_FRAGMENT_OPTION
#define NX_DHCPV6_FRAGMENT_OPTION                NX_DONT_FRAGMENT
#endif  

/* Define the number of routers a UDP packet passes before it is discarded. */

#ifndef NX_DHCPV6_TIME_TO_LIVE
#define NX_DHCPV6_TIME_TO_LIVE                   0x80
#endif

/* Define the stored packets in the UDP socket queue. */

#ifndef NX_DHCPV6_QUEUE_DEPTH
#define NX_DHCPV6_QUEUE_DEPTH                    5
#endif


/* Define the initial retransmission timeout in timer ticks for DHCPv6 messages. 
   For no limit on the retransmission timeout set to 0, for no limit
   on the retries, set to 0. 

   Note that regardless of length of timeout or number of retries, when the IP address 
   valid lifetime expires, the Client can no longer use its global IP address 
   assigned by the DHCPv6 Server. */

#define NX_DHCPV6_INIT_SOL_TRANSMISSION_TIMEOUT         (1 * NX_DHCPV6_TICKS_PER_SECOND) 
#define NX_DHCPV6_MAX_SOL_RETRANSMISSION_TIMEOUT        (120 * NX_DHCPV6_TICKS_PER_SECOND) 
#define NX_DHCPV6_MAX_SOL_RETRANSMISSION_COUNT          0

#define NX_DHCPV6_INIT_REQ_TRANSMISSION_TIMEOUT         (1 * NX_DHCPV6_TICKS_PER_SECOND) 
#define NX_DHCPV6_MAX_REQ_RETRANSMISSION_TIMEOUT        (30 * NX_DHCPV6_TICKS_PER_SECOND) 
#define NX_DHCPV6_MAX_REQ_RETRANSMISSION_COUNT           0

#define NX_DHCPV6_INIT_RENEW_TRANSMISSION_TIMEOUT       (1 * NX_DHCPV6_TICKS_PER_SECOND)     
#define NX_DHCPV6_MAX_RENEW_RETRANSMISSION_TIMEOUT      (600 * NX_DHCPV6_TICKS_PER_SECOND)  
#define NX_DHCPV6_MAX_RENEW_RETRANSMISSION_COUNT         0

#define NX_DHCPV6_INIT_REBIND_TRANSMISSION_TIMEOUT      (1 * NX_DHCPV6_TICKS_PER_SECOND)     
#define NX_DHCPV6_MAX_REBIND_RETRANSMISSION_TIMEOUT     (600 * NX_DHCPV6_TICKS_PER_SECOND)  
#define NX_DHCPV6_MAX_REBIND_RETRANSMISSION_COUNT         0  

#define NX_DHCPV6_INIT_RELEASE_TRANSMISSION_TIMEOUT     (1 * NX_DHCPV6_TICKS_PER_SECOND)
#define NX_DHCPV6_MAX_RELEASE_RETRANSMISSION_TIMEOUT     0 
#define NX_DHCPV6_MAX_RELEASE_RETRANSMISSION_COUNT       5  

#define NX_DHCPV6_INIT_DECLINE_TRANSMISSION_TIMEOUT     (1 * NX_DHCPV6_TICKS_PER_SECOND)
#define NX_DHCPV6_MAX_DECLINE_RETRANSMISSION_TIMEOUT     0
#define NX_DHCPV6_MAX_DECLINE_RETRANSMISSION_COUNT       5  

#define NX_DHCPV6_INIT_CONFIRM_TRANSMISSION_TIMEOUT     (1 * NX_DHCPV6_TICKS_PER_SECOND)
#define NX_DHCPV6_MAX_CONFIRM_RETRANSMISSION_TIMEOUT    (4 * NX_DHCPV6_TICKS_PER_SECOND)
#define NX_DHCPV6_MAX_CONFIRM_RETRANSMISSION_COUNT       0  

#define NX_DHCPV6_INIT_INFORM_TRANSMISSION_TIMEOUT      (1 * NX_DHCPV6_TICKS_PER_SECOND)
#define NX_DHCPV6_MAX_INFORM_RETRANSMISSION_TIMEOUT      120
#define NX_DHCPV6_MAX_INFORM_RETRANSMISSION_COUNT        0  



/* Define the Identity Association Internet Address option structure  */
typedef struct NX_DHCPV6_IA_ADDRESS_STRUCT
{

    USHORT          nx_op_code;                    /* IA internet address option code is 5 */
    USHORT          nx_option_length;              /* Length of the IA address option data = 24 not including length and op code field*/
    NXD_ADDRESS     nx_global_address;             /* Assigned Host IPv6 address */
    ULONG           nx_preferred_lifetime;         /* Server's preference for IPv6 address T1 life time for itself */
    ULONG           nx_valid_lifetime;             /* Server's assigned valid time for T2 for any server  */
    UINT            nx_address_status;             /* Indicates if the global address is registered and validated. */

} NX_DHCPV6_IA_ADDRESS;

/* Define the Option status structure  */
typedef struct NX_DHCPV6_OP_STATUS_STRUCT
{

    USHORT          nx_op_code;                    /* IA address option code is 5 */
    USHORT          nx_option_length;              /* Length of the IA address option data = 24 not including length and op code field*/
} NX_DHCPV6_OP_STATUS;


/* Define the Preference Option structure  */
typedef struct NX_DHCPV6_PREFERENCE_STRUCT
{

    USHORT          nx_op_code;                    /* IA address option code is 5 */
    USHORT          nx_option_length;              /* Length of the IA address option data = 24 not including length and op code field*/
    USHORT          nx_pref_value;                  /* Assigned Host IPv6 address */

} NX_DHCPV6_PREFERENCE;


/* Define the Identity Association for Permanent ("Non Temporary" in RFC) address */

typedef struct NX_DHCPV6_IA_NA_STRUCT
{

    USHORT              nx_op_code;             /* IA NA address option code is 3 */
    USHORT              nx_option_length;       /* 12 + length of variable length fields in IA_NA option . */
    ULONG               nx_IA_NA_id;            /* IANA identifier; must be unique among all client IANA's. Must be the same on restart per IANA */
    ULONG               nx_T1;                  /* Time client can extend time before address lifetime expires from the server it got it from; applies to all addresses in IA_NA. */
    ULONG               nx_T2;                  /* Same as T1 except this is when the client will request REBIND from another server. */

} NX_DHCPV6_IA_NA;


/* Define DHCPv6 Unique Identifier (DUID); both Client and Server must send messages with their own DUID. */

typedef struct NX_DHCPV6_DUID_STRUCT
{

    USHORT            nx_op_code;                 /* Client DUID option code is 1; Server DUID code is 2  */
    USHORT            nx_option_length;           /* Option length = 14 not including length and op code field; */
    USHORT            nx_duid_type;               /* 3 main types: hw; hw + time; vendor assigned ID (not supported here); requires DUID be stored in non volatile storage */
    USHORT            nx_hardware_type;           /* Only if LL/LLT type. Hardware type specified by IANA/RFC 826 e.g. IEEE 802; network byte order */
    ULONG             nx_duid_time;               /* Only if LLT type. Time based on when DUID generated; network byte order. */
    USHORT            nx_link_layer_address_msw;  /* Only if LL/LLT type. Pointer to Unique link layer address - most significant word (2 bytes)*/
    ULONG             nx_link_layer_address_lsw;  /* Only if LL/LLT type. Pointer to Unique link layer address - least significant word (4 bytes) */

} NX_DHCPV6_DUID;


/* Define the elapsed time option structure.  This contains the length of the Client Server session. */

typedef struct NX_DHCPV6_ELAPSED_TIME_STRUCT
{

    USHORT            nx_op_code;                /* Elapsed time option code = 8 not including length and op code field. */
    USHORT            nx_op_length;              /* Length of time data = 2. */
    USHORT            nx_session_time;           /* Time of DHCP session e.g. first msg elapsed time is zero. */

} NX_DHCPV6_ELAPSED_TIME;


/* Define the Message Option structure. Each message from the Client must have a unique message ID. */

typedef struct NX_DHCPV6_MESSAGE_HDR_STRUCT 
{

    USHORT             nx_message_type;           /* Message type (1 byte) */
    ULONG              nx_message_xid;            /* Message transaction ID (3 bytes)*/
} NX_DHCPV6_MESSAGE_HDR;

/* Define the option request structure. This is how the Client requests information other than global IP address.  
   It can ask for domain name, DNS server, time zone, time server and other options. */

typedef struct NX_DHCPV6_OPTIONREQUEST_STRUCT
{
    USHORT             nx_op_code;                /* Option Request code  = 6*/
    USHORT             nx_option_length;          /* Length in bytes of option data = 2 * number of requests */
    USHORT             nx_op_request;             /* e.g. DNS server = 23, ... */

} NX_DHCPV6_OPTIONREQUEST;

/* Define the Client DHCPv6 structure containind the DHCPv6 Client record (DHCPv6 status, server DUID etc).  */

typedef struct NX_DHCPV6_STRUCT 
{
    ULONG                   nx_dhcpv6_id;                               /* DHCPv6 Structure ID  */
    CHAR                    *nx_dhcpv6_name;                            /* DHCPv6 name supplied at create */ 
    UINT                    nx_dhcpv6_client_address_index;             /* Index in IP address table where the Client assigned address is located. */
#ifdef NETXDUO_MULTIHOME_SUPPORT
    UINT                    nx_dhcpv6_client_interface_index;           /* DHCPv6 outgoing network interface index */
#endif
    TX_THREAD               nx_dhcpv6_thread;                           /* Client processing thread */
    TX_EVENT_FLAGS_GROUP    nx_dhcpv6_timer_events;                     /* Message queue for IP lease and session timer events. */
    TX_MUTEX                nx_dhcpv6_client_mutex;                     /* Mutex for exclusive access to the DHCP Client instance */ 
    TX_TIMER                nx_dhcpv6_IP_lifetime_timer;                /* Client IP lifetime timeout timer. */ 
    TX_TIMER                nx_dhcpv6_session_timer;                    /* Client session duration timer. */ 
    NX_IP                   *nx_dhcpv6_ip_ptr;                          /* The associated IP pointer for this DHCPV6 instance */ 
    NX_PACKET_POOL          *nx_dhcpv6_pool_ptr;                        /* Pointer to packet pool for sending DHCPV6 messages */
    NX_UDP_SOCKET           nx_dhcpv6_socket;                           /* UDP socket for communicating with DHCPv6 server */
    UCHAR                   nx_dhcpv6_started;                          /* DHCPv6 client task has been started */ 
    UCHAR                   nx_dhcpv6_state;                            /* The current state of the DHCPv6 Client */
    UINT                    nx_dhcpv6_sleep_flag;                       /* If true, the DHCPv6 client is in a position where it can be stopped */ 
    NX_DHCPV6_MESSAGE_HDR   nx_dhcpv6_message_hdr;                      /* Message Header for all client messages to DHCPv6 Servers */
    NX_DHCPV6_DUID          nx_dhcpv6_client_duid;                      /* Client DUID; ID by which Client and Server identify each other's DUID */
    NX_DHCPV6_DUID          nx_dhcpv6_server_duid;                      /* Server DUID; ID by which Client and Server identify each other's DUID */
    NX_DHCPV6_ELAPSED_TIME  nx_dhcpv6_elapsed_time;                     /* Time duration of the current DHCP msg exchange between Client and Server. */
    NX_DHCPV6_IA_NA         nx_dhcpv6_iana;                             /* Identity Association for non temp address - must be stored in non volatile memory */
    NX_DHCPV6_IA_ADDRESS    nx_dhcpv6_ia;                               /* Client internet address option */
    NX_DHCPV6_PREFERENCE    nx_dhcpv6_preference;                       /* Server's preference affecting the Client's DHCPv6 server selection. */
    NX_DHCPV6_OPTIONREQUEST nx_dhcpv6_option_request;                   /* Set of request options in Solicit, Renew, Confirm or Rebind message types. */
    USHORT                  nx_status_code;                             /* Status of current option received by Client */
    ULONG                   nx_dhcpv6_IP_lifetime_time_accrued;         /* Time since Client set received or renewed its IP address with the DHCPv6 server. */
    CHAR                    nx_status_message[NX_DHCPV6_MAX_MESSAGE_SIZE];                  /* Server's message in its Option status to client.  */                       
    NXD_ADDRESS             nx_dhcpv6_DNS_name_server_address[NX_DHCPV6_NUM_DNS_SERVERS];   /* DNS name server IP address */
    NXD_ADDRESS             nx_dhcpv6_time_server_address[NX_DHCPV6_NUM_TIME_SERVERS];      /* time server IP address */
    CHAR                    nx_dhcpv6_domain_name[NX_DHCPV6_DOMAIN_NAME_BUFFER_SIZE];       /* Buffer for holding domain name. */
    CHAR                    nx_dhcpv6_time_zone[NX_DHCPV6_TIME_ZONE_BUFFER_SIZE];           /* Buffer for holding time zone. */
    ULONG                   nx_dhcpv6_solicitations_sent;               /* The number of Solicit messages sent */ 
    ULONG                   nx_dhcpv6_solicitation_responses;           /* The number of solicitations server responded to */ 
    ULONG                   nx_dhcpv6_requests_sent;                    /* The number of Request messages sent */ 
    ULONG                   nx_dhcpv6_request_responses;                /* The number of requests server responded to */ 
    ULONG                   nx_dhcpv6_renews_sent;                      /* The number of renewal messages sent */ 
    ULONG                   nx_dhcpv6_renew_responses;                  /* The number of renewals server responded to */ 
    ULONG                   nx_dhcpv6_rebinds_sent;                     /* The number of Rebind messages sent */ 
    ULONG                   nx_dhcpv6_rebind_responses;                 /* The number of Rebind requests Server responded to */ 
    ULONG                   nx_dhcpv6_releases_sent;                    /* The number of Release messages sent */ 
    ULONG                   nx_dhcpv6_release_responses;                /* The number of Releases server responded to  */ 
    ULONG                   nx_dhcpv6_confirms_sent;                    /* The number of confirmations sent */ 
    ULONG                   nx_dhcpv6_confirm_responses;                /* The number of confirmations server responded to */ 
    ULONG                   nx_dhcpv6_declines_sent;                    /* The number of declines sent */ 
    ULONG                   nx_dhcpv6_decline_responses;                /* The number of declines server responded to */ 
    ULONG                   nx_dhcpv6_inform_req_sent;                  /* The number of Inform (option requests) sent */ 
    ULONG                   nx_dhcpv6_inform_req_responses;             /* The number of Inform server responsed to */ 
    ULONG                   nx_dhcpv6_transmission_timeout;             /* Timeout on Client messages before resending a request to the server. */
    ULONG                   nx_dhcpv6_transmission_count;               /* The number of request retransmissions to the server. */

    /* Define the callback function for DHCP state change notification. If specified
       by the application, this function is called whenever a state change occurs for
       the DHCP associated with this IP instance.  */
    VOID (*nx_dhcpv6_state_change_callback)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr, UINT old_state, UINT new_state);

    /* Define the callback function for receiving a non successful status from the Server.  The
       context of the status/error is defined by the message type is was received in and what
       option the status is referring to e.g. IA Address.  */
    VOID (*nx_dhcpv6_server_error_handler)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr, UINT op_code, UINT status_code, UINT message_type);

    /* Define a handler for a deprecated IP address. */
    VOID (*nx_dhcpv6_deprecated_IP_address_handler)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr);

    /* Define a handler for an expired IP address. */
    VOID (*nx_dhcpv6_expired_IP_address_handler)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr);

} NX_DHCPV6;


#ifndef NX_DHCPV6_SOURCE_CODE

/* Application caller is present, perform API mapping.  */

/* Determine if error checking is desired.  If so, map DHCP API functions 
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work. 
   Note: error checking is enabled by default.  */


#ifdef NX_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define nx_dhcpv6_client_create                 _nx_dhcpv6_client_create
#define nx_dhcpv6_create_client_duid            _nx_dhcpv6_create_client_duid
#define nx_dhcpv6_create_client_ia              _nx_dhcpv6_create_client_ia
#define nx_dhcpv6_create_client_iana            _nx_dhcpv6_create_client_iana
#define nx_dhcpv6_client_delete                 _nx_dhcpv6_client_delete
#define nx_dhcpv6_force_renew                   _nx_dhcpv6_force_renew
#define nx_dhcpv6_get_client_duid_time_id       _nx_dhcpv6_get_client_duid_time_id
#define nx_dhcpv6_get_IP_address                _nx_dhcpv6_get_IP_address
#define nx_dhcpv6_get_lease_time_data           _nx_dhcpv6_get_lease_time_data
#define nx_dhcpv6_get_other_option_data         _nx_dhcpv6_get_other_option_data
#define nx_dhcpv6_get_DNS_server_address        _nx_dhcpv6_get_DNS_server_address
#define nx_dhcpv6_get_time_accrued              _nx_dhcpv6_get_time_accrued
#define nx_dhcpv6_register_IP_address           _nx_dhcpv6_register_IP_address
#define nx_dhcpv6_reinitialize                  _nx_dhcpv6_reinitialize
#define nx_dhcpv6_request_confirm               _nx_dhcpv6_request_confirm
#define nx_dhcpv6_request_decline               _nx_dhcpv6_request_decline
#define nx_dhcpv6_request_rebind                _nx_dhcpv6_request_rebind
#define nx_dhcpv6_request_release               _nx_dhcpv6_request_release
#define nx_dhcpv6_request_renew                 _nx_dhcpv6_request_renew
#define nx_dhcpv6_request_inform_request        _nx_dhcpv6_request__request
#define nx_dhcpv6_request_option_DNS_server     _nx_dhcpv6_request_option_DNS_server
#define nx_dhcpv6_request_option_domain_name    _nx_dhcpv6_request_option_domain_name
#define nx_dhcpv6_request_option_time_server    _nx_dhcpv6_request_option_time_server
#define nx_dhcpv6_request_option_timezone       _nx_dhcpv6_request_option_timezone
#define nx_dhcpv6_request_solicit               _nx_dhcpv6_request_solicit
#define nx_dhcpv6_resume                        _nx_dhcpv6_resume
#define nx_dhcpv6_set_time_accrued              _nx_dhcpv6_set_time_accrued
#define nx_dhcpv6_client_set_interface          _nx_dhcpv6_client_set_interface
#define nx_dhcpv6_start                         _nx_dhcpv6_start
#define nx_dhcpv6_stop                          _nx_dhcpv6_stop
#define nx_dhcpv6_suspend                       _nx_dhcpv6_suspend

#else

/* Services with error checking.  */

#define nx_dhcpv6_client_create                 _nxe_dhcpv6_client_create
#define nx_dhcpv6_create_client_duid            _nxe_dhcpv6_create_client_duid
#define nx_dhcpv6_create_client_ia              _nxe_dhcpv6_create_client_ia
#define nx_dhcpv6_create_client_iana            _nxe_dhcpv6_create_client_iana
#define nx_dhcpv6_client_delete                 _nxe_dhcpv6_client_delete
#define nx_dhcpv6_get_client_duid_time_id       _nxe_dhcpv6_get_client_duid_time_id
#define nx_dhcpv6_get_IP_address                _nxe_dhcpv6_get_IP_address
#define nx_dhcpv6_get_lease_time_data           _nxe_dhcpv6_get_lease_time_data
#define nx_dhcpv6_get_other_option_data         _nxe_dhcpv6_get_other_option_data
#define nx_dhcpv6_get_DNS_server_address        _nxe_dhcpv6_get_DNS_server_address
#define nx_dhcpv6_get_time_accrued              _nxe_dhcpv6_get_time_accrued
#define nx_dhcpv6_register_IP_address           _nxe_dhcpv6_register_IP_address
#define nx_dhcpv6_reinitialize                  _nxe_dhcpv6_reinitialize
#define nx_dhcpv6_request_confirm               _nxe_dhcpv6_request_confirm
#define nx_dhcpv6_request_decline               _nxe_dhcpv6_request_decline
#define nx_dhcpv6_request_rebind                _nxe_dhcpv6_request_rebind
#define nx_dhcpv6_request_release               _nxe_dhcpv6_request_release
#define nx_dhcpv6_request_renew                 _nxe_dhcpv6_request_renew
#define nx_dhcpv6_request_inform_request        _nxe_dhcpv6_request_inform_request
#define nx_dhcpv6_request_option_DNS_server     _nxe_dhcpv6_request_option_DNS_server
#define nx_dhcpv6_request_option_domain_name    _nxe_dhcpv6_request_option_domain_name
#define nx_dhcpv6_request_option_time_server    _nxe_dhcpv6_request_option_time_server
#define nx_dhcpv6_request_option_timezone       _nxe_dhcpv6_request_option_timezone
#define nx_dhcpv6_request_solicit               _nxe_dhcpv6_request_solicit
#define nx_dhcpv6_resume                        _nxe_dhcpv6_resume
#define nx_dhcpv6_set_time_accrued              _nxe_dhcpv6_set_time_accrued
#ifdef NETXDUO_MULTIHOME_SUPPORT
#define nx_dhcpv6_client_set_interface          _nxe_dhcpv6_client_set_interface
#endif
#define nx_dhcpv6_start                         _nxe_dhcpv6_start
#define nx_dhcpv6_stop                          _nxe_dhcpv6_stop
#define nx_dhcpv6_suspend                       _nxe_dhcpv6_suspend

#endif

/* Define the prototypes accessible to the application software.  */
UINT        nx_dhcpv6_client_create(NX_DHCPV6 *dhcpv6_ptr, NX_IP *ip_ptr, CHAR *name_ptr, NX_PACKET_POOL *packet_pool_ptr, VOID *stack_ptr, ULONG stack_size,
                             VOID (*nx_dhcpv6_state_change_callback)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr, UINT old_state, UINT new_state),                             
                             VOID (*dhcpv6_server_error_handler)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr, UINT op_code, UINT status_code, UINT message_type),
                             VOID (*dhcpv6_deprecated_IP_address_handler)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr),
                             VOID (*dhcpv6_expired_IP_address_handler)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr));
UINT        nx_dhcpv6_create_client_duid(NX_DHCPV6 *dhcpv6_ptr, UINT duid_type, UINT hardware_type, ULONG time);
UINT        nx_dhcpv6_create_client_iana(NX_DHCPV6 *dhcpv6_ptr, UINT IA_ident, ULONG T1, ULONG T2);
UINT        nx_dhcpv6_create_client_ia(NX_DHCPV6 *dhcpv6_ptr, NXD_ADDRESS *ipv6_address, ULONG preferred_lifetime, ULONG valid_lifetime);                             
UINT        nx_dhcpv6_client_delete(NX_DHCPV6 *dhcpv6_ptr);
UINT        nx_dhcpv6_get_client_duid_time_id(NX_DHCPV6 *dhcpv6_ptr, ULONG *time_id);
UINT        nx_dhcpv6_get_IP_address(NX_DHCPV6 *dhcpv6_ptr, NXD_ADDRESS *ip_address);
UINT        nx_dhcpv6_get_lease_time_data(NX_DHCPV6 *dhcpv6_ptr, ULONG *T1, ULONG *T2, ULONG *preferred_lifetime, ULONG *valid_lifetime);
UINT        nx_dhcpv6_get_other_option_data(NX_DHCPV6 *dhcpv6_ptr, UINT option_code, UCHAR *buffer);
UINT        nx_dhcpv6_get_DNS_server_address(NX_DHCPV6 *dhcpv6_ptr, UINT index, NXD_ADDRESS *server_address);
UINT        nx_dhcpv6_get_time_accrued(NX_DHCPV6 *dhcpv6_ptr, ULONG *time_accrued);
UINT        nx_dhcpv6_register_IP_address(NX_DHCPV6 *dhcpv6_ptr, NXD_ADDRESS *ip_address, ULONG prefix_length);
UINT        nx_dhcpv6_reinitialize(NX_DHCPV6 *dhcpv6_ptr);
UINT        nx_dhcpv6_request_confirm(NX_DHCPV6 *dhcpv6_ptr);
UINT        nx_dhcpv6_request_decline(NX_DHCPV6 *dhcpv6_ptr);
UINT        nx_dhcpv6_request_inform_request(NX_DHCPV6 *dhcpv6_ptr);
UINT        nx_dhcpv6_request_option_DNS_server(NX_DHCPV6 *dhcpv6_ptr, UINT enable);
UINT        nx_dhcpv6_request_option_domain_name(NX_DHCPV6 *dhcpv6_ptr, UINT enable);
UINT        nx_dhcpv6_request_option_time_server(NX_DHCPV6 *dhcpv6_ptr, UINT enable);
UINT        nx_dhcpv6_request_option_timezone(NX_DHCPV6 *dhcpv6_ptr, UINT enable);
UINT        nx_dhcpv6_request_rebind(NX_DHCPV6 *dhcpv6_ptr);
UINT        nx_dhcpv6_request_release(NX_DHCPV6 *dhcpv6_ptr);
UINT        nx_dhcpv6_request_renew(NX_DHCPV6 *dhcpv6_ptr);
UINT        nx_dhcpv6_request_solicit(NX_DHCPV6 *dhcpv6_ptr);
UINT        nx_dhcpv6_resume(NX_DHCPV6 *dhcpv6_ptr);
UINT        nx_dhcpv6_set_time_accrued(NX_DHCPV6 *dhcpv6_ptr, ULONG time_accrued);
UINT        nx_dhcpv6_client_set_interface(NX_DHCPV6 *dhcpv6_ptr, UINT interface_index);
UINT        nx_dhcpv6_start(NX_DHCPV6 *dhcpv6_ptr, ULONG nv_time);
UINT        nx_dhcpv6_stop(NX_DHCPV6 *dhcpv6_ptr);
UINT        nx_dhcpv6_suspend(NX_DHCPV6 *dhcpv6_ptr);

#else

/* DHCP source code is being compiled, do not perform any API mapping.  */

UINT        _nxe_dhcpv6_client_create(NX_DHCPV6 *dhcpv6_ptr, NX_IP *ip_ptr, CHAR *name_ptr, NX_PACKET_POOL *packet_pool_ptr, VOID *stack_ptr, ULONG stack_size,
                               VOID (*nx_dhcpv6_state_change_callback)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr, UINT old_state, UINT new_state),
                               VOID (*dhcpv6_server_error_handler)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr, UINT op_code, UINT status_code, UINT message_type),
                               VOID (*dhcpv6_deprecated_IP_address_handler)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr),
                               VOID (*dhcpv6_expired_IP_address_handler)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr));
UINT        _nx_dhcpv6_client_create(NX_DHCPV6 *dhcpv6_ptr, NX_IP *ip_ptr, CHAR *name_ptr, NX_PACKET_POOL *packet_pool_ptr, VOID *stack_ptr, ULONG stack_size,
                              VOID (*nx_dhcpv6_state_change_callback)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr, UINT old_state, UINT new_state),
                              VOID (*dhcpv6_server_error_handler)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr, UINT op_code, UINT status_code, UINT message_type),
                              VOID (*dhcpv6_deprecated_IP_address_handler)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr),
                              VOID (*dhcpv6_expired_IP_address_handler)(struct NX_DHCPV6_STRUCT *dhcpv6_ptr));
UINT        _nxe_dhcpv6_client_delete(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nx_dhcpv6_client_delete(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nxe_dhcpv6_create_client_duid(NX_DHCPV6 *dhcpv6_ptr, UINT duid_type, UINT hardware_type, ULONG time);
UINT        _nx_dhcpv6_create_client_duid(NX_DHCPV6 *dhcpv6_ptr, UINT duid_type, UINT hardware_type, ULONG time);
UINT        _nxe_dhcpv6_create_client_ia(NX_DHCPV6 *dhcpv6_ptr, NXD_ADDRESS *ipv6_address, ULONG preferred_lifetime, ULONG valid_lifetime);
UINT        _nx_dhcpv6_create_client_ia(NX_DHCPV6 *dhcpv6_ptr, NXD_ADDRESS *ipv6_address, ULONG preferred_lifetime, ULONG valid_lifetime);
UINT        _nxe_dhcpv6_create_client_iana(NX_DHCPV6 *dhcpv6_ptr, UINT IA_ident, ULONG T1, ULONG T2);
UINT        _nx_dhcpv6_create_client_iana(NX_DHCPV6 *dhcpv6_ptr, UINT IA_ident, ULONG T1, ULONG T2);
UINT        _nxe_dhcpv6_get_client_duid_time_id(NX_DHCPV6 *dhcpv6_ptr, ULONG *time_id);
UINT        _nx_dhcpv6_get_client_duid_time_id(NX_DHCPV6 *dhcpv6_ptr, ULONG *time_id);
UINT        _nxe_dhcpv6_get_IP_address(NX_DHCPV6 *dhcpv6_ptr,  NXD_ADDRESS *ip_address);
UINT        _nx_dhcpv6_get_IP_address(NX_DHCPV6 *dhcpv6_ptr, NXD_ADDRESS *ip_address);
UINT        _nxe_dhcpv6_get_other_option_data(NX_DHCPV6 *dhcpv6_ptr, UINT option_code, UCHAR *buffer);
UINT        _nx_dhcpv6_get_other_option_data(NX_DHCPV6 *dhcpv6_ptr, UINT option_code, UCHAR *buffer);
UINT        _nxe_dhcpv6_get_lease_time_data(NX_DHCPV6 *dhcpv6_ptr, ULONG *T1, ULONG *T2, ULONG *preferred_lifetime, ULONG *valid_lifetime);
UINT        _nx_dhcpv6_get_lease_time_data(NX_DHCPV6 *dhcpv6_ptr, ULONG *T1, ULONG *T2, ULONG *preferred_lifetime, ULONG *valid_lifetime);
UINT        _nxe_dhcpv6_get_DNS_server_address(NX_DHCPV6 *dhcpv6_ptr, UINT index, NXD_ADDRESS *server_address);
UINT        _nx_dhcpv6_get_DNS_server_address(NX_DHCPV6 *dhcpv6_ptr, UINT index, NXD_ADDRESS *server_address);
UINT        _nxe_dhcpv6_get_time_accrued(NX_DHCPV6 *dhcpv6_ptr, ULONG *time_accrueds);
UINT        _nx_dhcpv6_get_time_accrued(NX_DHCPV6 *dhcpv6_ptr, ULONG *time_accrued);
UINT        _nxe_dhcpv6_register_IP_address(NX_DHCPV6 *dhcpv6_ptr, NXD_ADDRESS *ip_address, ULONG prefix_length);
UINT        _nx_dhcpv6_register_IP_address(NX_DHCPV6 *dhcpv6_ptr, NXD_ADDRESS *ip_address, ULONG prefix_length);
UINT        _nxe_dhcpv6_reinitialize(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nx_dhcpv6_reinitialize(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nxe_dhcpv6_request_confirm(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nx_dhcpv6_request_confirm(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nxe_dhcpv6_request_decline(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nx_dhcpv6_request_decline(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nxe_dhcpv6_request_inform_request(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nx_dhcpv6_request_inform_request(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nxe_dhcpv6_request_option_DNS_server(NX_DHCPV6 *dhcpv6_ptr, UINT enable);
UINT        _nx_dhcpv6_request_option_DNS_server(NX_DHCPV6 *dhcpv6_ptr, UINT enable);
UINT        _nxe_dhcpv6_request_option_time_server(NX_DHCPV6 *dhcpv6_ptr, UINT enable);
UINT        _nx_dhcpv6_request_option_time_server(NX_DHCPV6 *dhcpv6_ptr, UINT enable);
UINT        _nxe_dhcpv6_request_option_timezone(NX_DHCPV6 *dhcpv6_ptr, UINT enable);
UINT        _nx_dhcpv6_request_option_timezone(NX_DHCPV6 *dhcpv6_ptr, UINT enable);
UINT        _nxe_dhcpv6_request_option_domain_name(NX_DHCPV6 *dhcpv6_ptr, UINT enable);
UINT        _nx_dhcpv6_request_option_domain_name(NX_DHCPV6 *dhcpv6_ptr, UINT enable);
UINT        _nxe_dhcpv6_request_rebind(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nx_dhcpv6_request_rebind(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nxe_dhcpv6_request_release(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nx_dhcpv6_request_release(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nxe_dhcpv6_request_renew(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nx_dhcpv6_request_renew(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nxe_dhcpv6_request_solicit(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nx_dhcpv6_request_solicit(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nxe_dhcpv6_resume(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nx_dhcpv6_resume(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nxe_dhcpv6_set_time_accrued(NX_DHCPV6 *dhcpv6_ptr, ULONG time_accrued);
UINT        _nx_dhcpv6_set_time_accrued(NX_DHCPV6 *dhcpv6_ptr, ULONG time_accrued);
UINT        _nxe_dhcpv6_client_set_interface(NX_DHCPV6 *dhcpv6_ptr, UINT interface_index);
UINT        _nx_dhcpv6_client_set_interface(NX_DHCPV6 *dhcpv6_ptr, UINT interface_index);
UINT        _nxe_dhcpv6_start(NX_DHCPV6 *dhcpv6_ptr, ULONG nv_time);
UINT        _nx_dhcpv6_start(NX_DHCPV6 *dhcpv6_ptr, ULONG nv_time);
UINT        _nxe_dhcpv6_stop(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nx_dhcpv6_stop(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nxe_dhcpv6_suspend(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nx_dhcpv6_suspend(NX_DHCPV6 *dhcpv6_ptr);

#endif


/* Define DHCPv6 Client internal functions. */

UINT        _nx_dhcpv6_add_client_duid(NX_DHCPV6 *dhcpv6_ptr, UCHAR *buffer_ptr, UINT *index);
UINT        _nx_dhcpv6_add_elapsed_time(NX_DHCPV6 *dhcpv6_ptr, UCHAR *buffer_ptr, UINT *index); 
UINT        _nx_dhcpv6_add_option_request(NX_DHCPV6 *dhcpv6_ptr, UCHAR *buffer_ptr, UINT *index); 
UINT        _nx_dhcpv6_add_ia_address(NX_DHCPV6 *dhcpv6_ptr, UCHAR *buffer_ptr, UINT *index); 
UINT        _nx_dhcpv6_add_iana(NX_DHCPV6 *dhcpv6_ptr, UCHAR *buffer_ptr, UINT *index, UINT add_ia_address); 
UINT        _nx_dhcpv6_add_server_duid(NX_DHCPV6 *dhcpv6_ptr, UCHAR *buffer_ptr, UINT *index); 
UINT        _nx_dhcpv6_extract_packet_information(NX_DHCPV6 *dhcpv6_ptr, NX_PACKET *packet_ptr);
VOID        _nx_dhcpv6_IP_lifetime_timeout_entry(NX_DHCPV6 *dhcpv6_ptr);
VOID        _nx_dhcpv6_process(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nx_dhcpv6_process_client_duid(NX_DHCPV6 *dhcpv6_ptr, UCHAR *received_buffer, UINT length);
UINT        _nx_dhcpv6_process_DNS_server(NX_DHCPV6 *dhcpv6_ptr, UCHAR *received_buffer, UINT length);
UINT        _nx_dhcpv6_process_domain_name(NX_DHCPV6 *dhcpv6_ptr, UCHAR *received_buffer, UINT length);
UINT        _nx_dhcpv6_process_ia(NX_DHCPV6 *dhcpv6_ptr, UCHAR *received_buffer, UINT length);
UINT        _nx_dhcpv6_process_iana(NX_DHCPV6 *dhcpv6_ptr, UCHAR *received_buffer, UINT length);
UINT        _nx_dhcpv6_process_preference(NX_DHCPV6 *dhcpv6_ptr, UCHAR *received_buffer, UINT length);
UINT        _nx_dhcpv6_process_server_duid(NX_DHCPV6 *dhcpv6_ptr, UCHAR *received_buffer, UINT length);
UINT        _nx_dhcpv6_process_status(NX_DHCPV6 *dhcpv6_ptr, UCHAR *received_buffer, UINT length);
UINT        _nx_dhcpv6_process_time_zone(NX_DHCPV6 *dhcpv6_ptr, UCHAR *received_buffer, UINT length);
UINT        _nx_dhcpv6_process_time_server(NX_DHCPV6 *dhcpv6_ptr, UCHAR *received_buffer, UINT length);
UINT        _nx_dhcpv6_remove_assigned_address(NX_DHCPV6 *dhcpv6_ptr);
UINT        _nx_dhcpv6_request(NX_DHCPV6 *dhcpv6_ptr, UINT dhcpv6_state);
UINT        _nx_dhcpv6_send_request(NX_DHCPV6 *dhcpv6_ptr); 
VOID        _nx_dhcpv6_session_timeout_entry(NX_DHCPV6 *dhcpv6_ptr);
VOID        _nx_dhcpv6_thread_entry(ULONG ip_instance);
UINT        _nx_dhcpv6_utility_get_block_option_length(UCHAR *buffer_ptr, ULONG *option, ULONG *length);
UINT        _nx_dhcpv6_utility_get_data(UCHAR *data, UINT size, ULONG *value);
INT         _nx_dhcpv6_utility_time_randomize(void);
UINT        _nx_dhcpv6_waiting_on_reply(NX_DHCPV6 *dhcpv6_ptr);


/* Determine if a C++ compiler is being used.  If so, complete the standard
   C conditional started above.  */
#ifdef   __cplusplus
        }
#endif

#endif 


