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
/**   Domain Name System (DNS)                                            */ 
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */ 
/*                                                                        */ 
/*    nxd_dns.h                                           PORTABLE C      */ 
/*                                                           5.2          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    Janet Christiansen, Express Logic, Inc.                             */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX Domain Name System Protocol (DNS)        */ 
/*    component, including all data types and external references.        */ 
/*    It is assumed that nx_api.h and nx_port.h have already been         */ 
/*    included.                                                           */
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  06-01-2010     Janet Christiansen       Initial Version 5.0           */ 
/*  10-31-2011     Janet Christiansen       Modified comment(s),          */ 
/*                                            resulting in version 5.1    */ 
/*  01-27-2012     Janet Christiansen       Modified comment(s),          */ 
/*                                            added support for the       */
/*                                            NX_DNS_CLIENT_USER_CREATE_  */
/*                                            PACKET_POOL option,         */
/*                                            resulting in version 5.2    */ 
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NXD_DNS_H
#define NXD_DNS_H

/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */
extern   "C" {

#endif

#include "nx_udp.h"
#include "nx_ipv4.h"
#include "nx_ipv6.h"

/* Define the DNS ID.  */

#define NX_DNS_ID                       0x444e5320UL

/* Define message and name limits.  */

#define NX_DNS_LABEL_MAX                63          /* Maximum Label (between to dots) size                 */
#define NX_DNS_NAME_MAX                 255         /* Maximum Name size                                    */
#define NX_DNS_IP_LOOKUP_SIZE           75          /* IPv6 needs 66 characters for the address plus 8 for the ip6.arpa plus null           */
                                                    /* IPv4 needs 12 characters max for the address plus 12 for the IN-ADDR.ARPA plus null  */


/* Define offsets into the DNS message buffer.  */

#define NX_DNS_ID_OFFSET                0           /* Offset to ID code in DNS buffer                      */
#define NX_DNS_FLAGS_OFFSET             2           /* Offset to flags in DNS buffer                        */
#define NX_DNS_QDCOUNT_OFFSET           4           /* Offset to Question Count in DNS buffer               */
#define NX_DNS_ANCOUNT_OFFSET           6           /* Offset to Answer Count in DNS buffer                 */
#define NX_DNS_NSCOUNT_OFFSET           8           /* Offset to Authority Count in DNS buffer              */
#define NX_DNS_ARCOUNT_OFFSET           10          /* Offset to Additional Info. Count in DNS buffer       */
#define NX_DNS_QDSECT_OFFSET            12          /* Offset to Question Section in DNS buffer             */


/* Define return code constants.  */

#define NX_DNS_ERROR                    0xA0        /* DNS internal error                                   */ 
#define NX_DNS_NO_SERVER                0xA1        /* No DNS server was specified                          */ 
#define NX_DNS_TIMEOUT                  0xA2        /* DNS timeout occurred                                 */ 
#define NX_DNS_QUERY_FAILED             0xA3        /* DNS query failed; no DNS server sent an 'answer'     */ 
#define NX_DNS_BAD_ADDRESS_ERROR        0xA4        /* Improperly formatted IPv4 or IPv6 address            */ 
#define NX_DNS_SIZE_ERROR               0xA5        /* DNS destination size is too small                    */ 
#define NX_DNS_MALFORMED_PACKET         0xA6        /* Improperly formed or corrupted DNS packet received   */ 
#define NX_DNS_BAD_ID_ERROR             0xA7        /* DNS packet from server does not match query ID       */ 
#define NX_DNS_PARAM_ERROR              0xA8        /* Invalid non pointer input to API                     */
#define NX_DNS_SERVER_NOT_FOUND         0xA9        /* Server not found in Client list of DNS servers       */
#define NX_DNS_PACKET_CREATE_ERROR      0xAA        /* Error creating DNS packet                            */
#define NX_DNS_EMPTY_DNS_SERVER_LIST    0xAB        /* DNS Client's list of DNS servers is empty            */
#define NX_DNS_SERVER_AUTH_ERROR        0xAC        /* Server not able to authenticate answer/authority data*/
#define NX_DNS_ZERO_GATEWAY_IP_ADDRESS  0xAD        /* DNS Client IP instance has a zero gateway IP address */
#define NX_DNS_MISMATCHED_RESPONSE      0xA5        /* Server response type does not match the query request*/
#define NX_DNS_DUPLICATE_ENTRY          0xA6        /* Duplicate entry exists in DNS server table           */
#define NX_DNS_RETRY_A_QUERY            0xA7        /* SOA status returned; web site only exists as IPv4    */
#define NX_DNS_IPV6_DISABLED_ERROR      0xA8        /* Cannot process AAAA or PTR record with IPv6 disabled */

/* Define constants for the flags word.  */

#define NX_DNS_QUERY_MASK               0x8000
#define NX_DNS_RESPONSE_FLAG            0x8000
#define NX_DNS_ERROR_MASK               0x8002      /* Server response indicates an error or data not authenticated by server */


#define NX_DNS_OPCODE_QUERY             (0 << 12)   /* Shifted right 12 is still 0                          */
#define NX_DNS_OPCODE_IQUERY            (1 << 12)   /* 1 shifted right by 12                                */
#define NX_DNS_OPCODE_STATUS            (2 << 12)   /* 2 shifted right by 12                                */

#define NX_DNS_AA_FLAG                  0x0400      /* Authoritative Answer                                 */
#define NX_DNS_TC_FLAG                  0x0200      /* Truncated                                            */
#define NX_DNS_RD_FLAG                  0x0100      /* Recursive Query                                      */
#define NX_DNS_RA_FLAG                  0x0080      /* Recursion Available                                  */
#define NX_DNS_FA_FLAG                  0x0010      /* Force Authentication                                 */

#define NX_DNS_RCODE_MASK               0x000f      /* Isolate the Result Code                              */
#define NX_DNS_RCODE_SUCCESS            0
#define NX_DNS_RCODE_FORMAT_ERR         1
#define NX_DNS_RCODE_SERVER_ERR         2
#define NX_DNS_RCODE_NAME_ERR           3
#define NX_DNS_RCODE_NOT_IMPL           4
#define NX_DNS_RCODE_REFUSED            5

#define NX_DNS_QUERY_FLAGS  (NX_DNS_OPCODE_QUERY | NX_DNS_RD_FLAG)  /* | NX_DNS_FA_FLAG */

/* Define name compression masks.  */

#define NX_DNS_COMPRESS_MASK            0xc0
#define NX_DNS_COMPRESS_VALUE           0xc0
#define NX_DNS_POINTER_MASK             0xc000

/* Define resource record types.  */

#define NX_DNS_RR_TYPE_A                1           /* Host address                                         */
#define NX_DNS_RR_TYPE_NS               2           /* Authoritative name server                            */
#define NX_DNS_RR_TYPE_MD               3           /* Mail destination (Obsolete - use MX)                 */
#define NX_DNS_RR_TYPE_MF               4           /* Mail forwarder (Obsolete - use MX)                   */
#define NX_DNS_RR_TYPE_CNAME            5           /* Canonical name for an alias                          */
#define NX_DNS_RR_TYPE_SOA              6           /* Marks the start of a zone of authority               */
#define NX_DNS_RR_TYPE_MB               7           /* Mailbox domain name (EXPERIMENTAL)                   */
#define NX_DNS_RR_TYPE_MG               8           /* Mail group member (EXPERIMENTAL)                     */
#define NX_DNS_RR_TYPE_MR               9           /* Mail rename domain name (EXPERIMENTAL)               */
#define NX_DNS_RR_TYPE_NULL             10          /* Null RR (EXPERIMENTAL)                               */
#define NX_DNS_RR_TYPE_WKS              11          /* Well known service description                       */
#define NX_DNS_RR_TYPE_PTR              12          /* Domain name pointer                                  */
#define NX_DNS_RR_TYPE_HINFO            13          /* Host information                                     */
#define NX_DNS_RR_TYPE_MINFO            14          /* Mailbox or mail list information                     */
#define NX_DNS_RR_TYPE_MX               15          /* Mail exchange                                        */
#define NX_DNS_RR_TYPE_TXT              16          /* Text strings                                         */
#define NX_DNS_RR_TYPE_AAAA             28          /* IPv6 Host address                                    */


/* Define constants for Qtypes (queries).  */

#define NX_DNS_RR_TYPE_AXFR             252         /* Request for a transfer of an entire zone             */
#define NX_DNS_RR_TYPE_MAILB            253         /* Request for mailbox-related records (MB, MG or MR)   */
#define NX_DNS_RR_TYPE_MAILA            254         /* Request for mail agent RRs (Obsolete - see MX)       */
#define NX_DNS_RR_TYPE_ALL              255         /* Request for all records                              */

/* Define resource record classes.  */

#define NX_DNS_RR_CLASS_IN              1           /* Internet                                             */
#define NX_DNS_RR_CLASS_CS              2           /* CSNET class (Obsolete)                               */
#define NX_DNS_RR_CLASS_CH              3           /* CHAOS class                                          */
#define NX_DNS_RR_CLASS_HS              4           /* Hesiod [Dyer 87]                                     */


/* Define constant valid for Qtypes (queries).  */

#define NX_DNS_RR_CLASS_ALL                     255         /* Any class   */

/* Define the TCP and UDP port number */

#define NX_DNS_PORT                             53          /* Port for TX,RX and TCP/UDP */


/* Start of configurable options. */


/* Set the DNS client IP (IPv4) instance gateway as primary DNS server. 
   Note that NX_DNS_CLIENT_IP_GATEWAY_ADDRESS must also be a non zero
   IP Address to use this option.  

   This must be an IPv4 address. */

/* Define the Client Gateway IP address. */
#ifndef NX_DNS_CLIENT_IP_GATEWAY_ADDRESS    
#define NX_DNS_CLIENT_IP_GATEWAY_ADDRESS        IP_ADDRESS(192,2,2,1)
#endif

/* Define the gate way server to also be the DNS server. 
#define NX_DNS_IP_GATEWAY_AND_DNS_SERVER 
*/

/* Define UDP socket create options.  */

#ifndef NX_DNS_TYPE_OF_SERVICE
#define NX_DNS_TYPE_OF_SERVICE          NX_IP_NORMAL
#endif

#ifndef NX_DNS_FRAGMENT_OPTION
#define NX_DNS_FRAGMENT_OPTION          NX_DONT_FRAGMENT
#endif  

#ifndef NX_DNS_TIME_TO_LIVE
#define NX_DNS_TIME_TO_LIVE                     0x80
#endif

/* Define the queue depth of the DNS Client socket. */

#ifndef NX_DNS_QUEUE_DEPTH
#define NX_DNS_QUEUE_DEPTH                      5
#endif

/* Define maximum size of DNS message.  This is the maximum size for IPv4 DNS messages;
   There is no such restraint on DNS for IPv6 implementations. */

#ifndef NX_DNS_MESSAGE_MAX              
#define NX_DNS_MESSAGE_MAX                      512         /* Maximum DNS/UDP Message size */
#endif

/* Determine if the Client will create its own packet pool
   or let the host application create one. See nx_dns_packet_pool_set
   for how to set the DNS packet pool from the host application. */

#define NX_DNS_CLIENT_USER_CREATE_PACKET_POOL   // jlc comment out


/* Define a payload to include the maximum size DNS message plus the Ethernet, IP and UDP overhead. */

#ifndef NX_DNS_PACKET_PAYLOAD_UNALIGNED
#ifdef FEATURE_NX_IPV6
#define NX_DNS_PACKET_PAYLOAD_UNALIGNED         (16 + sizeof(NX_IPV6_HEADER) + sizeof(NX_UDP_HEADER) + NX_DNS_MESSAGE_MAX)
#else
#define NX_DNS_PACKET_PAYLOAD_UNALIGNED         (16 + sizeof(NX_IPV4_HEADER) + sizeof(NX_UDP_HEADER) + NX_DNS_MESSAGE_MAX)
#endif
#endif

/* Round up to a 4 byte aligned packet payload. */

#define NX_DNS_PACKET_PAYLOAD                   (((NX_DNS_PACKET_PAYLOAD_UNALIGNED + sizeof(ULONG) - 1)/sizeof(ULONG)) * sizeof(ULONG))

/* Define the size (e.g. number of packets) of the DNS Client
   packet pool for sending out DNS messages. */

#ifndef NX_DNS_PACKET_POOL_SIZE
#define NX_DNS_PACKET_POOL_SIZE                 (16 * (NX_DNS_PACKET_PAYLOAD + sizeof(NX_PACKET)))
#endif


/* Define the maximum number of retries to a DNS server. */

#ifndef NX_DNS_MAX_RETRIES              
#define NX_DNS_MAX_RETRIES                      3
#endif

/* Define size of the DNS server list. Remember to allow for one 'null' terminating
   e.g. zero IP address entry for this list. */

#ifndef NX_DNS_MAX_SERVERS
#define NX_DNS_MAX_SERVERS                      5
#endif


/* Define the timeout option in timer ticks for allocating a packet
   from the DNS Client packet pool. */

#ifndef NX_DNS_PACKET_ALLOCATE_TIMEOUT
#define NX_DNS_PACKET_ALLOCATE_TIMEOUT          200
#endif


/* Define the basic DNS data structure.  */

typedef struct NX_IP_DNS_STRUCT 
{
    ULONG           nx_dns_id;                                      /* DNS ID                                                   */
    UCHAR           *nx_dns_domain;                                 /* Pointer to domain name                                   */ 
    NX_IP           *nx_dns_ip_ptr;                                 /* Pointer to associated IP structure                       */ 
    NXD_ADDRESS     nxd_dns_server_ip_array[NX_DNS_MAX_SERVERS+1];  /* List of DNS server NetX Duo IP addresses                 */ 
    ULONG           nx_dns_retries;                                 /* DNS query retries                                        */ 
#ifndef NX_DNS_CLIENT_USER_CREATE_PACKET_POOL
    NX_PACKET_POOL  nx_dns_pool;                                    /* The pool of UDP data packets for DNS messages            */
    UCHAR           nx_dns_pool_area[NX_DNS_PACKET_POOL_SIZE];
#endif
    NX_PACKET_POOL *nx_dns_packet_pool_ptr;                         /* Pointer to DNS Client packet pool                        */
    NX_UDP_SOCKET   nx_dns_socket;                                  /* DNS Socket                                               */
    TX_MUTEX        nx_dns_mutex;                                   /* DNS Mutex to protect DNS instance                        */
} NX_DNS;


#ifndef NX_DNS_SOURCE_CODE

/* Application caller is present, perform API mapping.  */

/* Determine if error checking is desired.  If so, map API functions 
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work. 
   Note: error checking is enabled by default.  */

#ifdef NX_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define nx_dns_create                   _nx_dns_create
#define nx_dns_delete                   _nx_dns_delete
#define nx_dns_packet_pool_set          _nx_dns_packet_pool_set
#define nx_dns_host_by_address_get      _nx_dns_host_by_address_get
#define nx_dns_host_by_name_get         _nx_dns_host_by_name_get
#define nx_dns_server_add               _nx_dns_server_add
#define nx_dns_server_remove            _nx_dns_server_remove
#define nxd_dns_host_by_address_get     _nxd_dns_host_by_address_get
#define nxd_dns_host_by_name_get        _nxd_dns_host_by_name_get
#define nxd_dns_server_add              _nxd_dns_server_add
#define nxd_dns_server_remove           _nxd_dns_server_remove

#else

/* Services with error checking.  */

#define nx_dns_create                   _nxe_dns_create
#define nx_dns_delete                   _nxe_dns_delete
#define nx_dns_packet_pool_set          _nxe_dns_packet_pool_set
#define nx_dns_host_by_address_get      _nxe_dns_host_by_address_get
#define nx_dns_host_by_name_get         _nxe_dns_host_by_name_get
#define nx_dns_server_add               _nxe_dns_server_add
#define nx_dns_server_remove            _nxe_dns_server_remove
#define nxd_dns_host_by_address_get     _nxde_dns_host_by_address_get
#define nxd_dns_host_by_name_get        _nxde_dns_host_by_name_get
#define nxd_dns_server_add              _nxde_dns_server_add
#define nxd_dns_server_remove           _nxde_dns_server_remove

#endif

/* Define the prototypes accessible to the application software.  */

UINT        nx_dns_create(NX_DNS *dns_ptr, NX_IP *ip_ptr, UCHAR *domain_name);
UINT        nx_dns_delete(NX_DNS *dns_ptr);
UINT        nx_dns_packet_pool_set(NX_DNS *dns_ptr, NX_PACKET_POOL *packet_pool_ptr);
UINT        nx_dns_server_add(NX_DNS *dns_ptr, ULONG server_address);
UINT        nx_dns_server_remove(NX_DNS *dns_ptr, ULONG server_address);
UINT        nx_dns_host_by_name_get(NX_DNS *dns_ptr, UCHAR *host_name, ULONG *host_address_ptr, ULONG wait_option);
UINT        nx_dns_host_by_address_get(NX_DNS *dns_ptr, ULONG ip_address, UCHAR *host_name_ptr, UINT host_name_buffer_size, ULONG wait_option);
UINT        nxd_dns_host_by_address_get(NX_DNS *dns_ptr, NXD_ADDRESS *ip_address, UCHAR *host_name_ptr, UINT host_name_buffer_size, ULONG wait_option);
UINT        nxd_dns_host_by_name_get(NX_DNS *dns_ptr, UCHAR *host_name, NXD_ADDRESS *host_address_ptr, ULONG wait_option, UINT lookup_type);
UINT        nxd_dns_server_add(NX_DNS *dns_ptr, NXD_ADDRESS *dns_server_address);
UINT        nxd_dns_server_remove(NX_DNS *dns_ptr, NXD_ADDRESS *server_address);

#else

/* DNS source code is being compiled, do not perform any API mapping.  */

UINT        _nxe_dns_create(NX_DNS *dns_ptr, NX_IP *ip_ptr, UCHAR *domain_name);
UINT        _nx_dns_create(NX_DNS *dns_ptr, NX_IP *ip_ptr, UCHAR *domain_name);
UINT        _nxe_dns_delete(NX_DNS *dns_ptr);
UINT        _nx_dns_delete(NX_DNS *dns_ptr);
UINT        _nx_dns_packet_pool_set(NX_DNS *dns_ptr, NX_PACKET_POOL *packet_pool_ptr);
UINT        _nxe_dns_packet_pool_set(NX_DNS *dns_ptr, NX_PACKET_POOL *packet_pool_ptr);
UINT        _nxe_dns_host_by_address_get(NX_DNS *dns_ptr, ULONG ip_address, UCHAR *host_name_ptr, UINT host_name_buffer_size, ULONG wait_option);
UINT        _nx_dns_host_by_address_get(NX_DNS *dns_ptr, ULONG ip_address, UCHAR *host_name_ptr, UINT host_name_buffer_size, ULONG wait_option);
UINT        _nxe_dns_host_by_name_get(NX_DNS *dns_ptr, UCHAR *host_name, ULONG *host_address_ptr, ULONG wait_option);
UINT        _nx_dns_host_by_name_get(NX_DNS *dns_ptr, UCHAR *host_name, ULONG *host_address_ptr, ULONG wait_option);
UINT        _nxe_dns_server_add(NX_DNS *dns_ptr, ULONG server_address);
UINT        _nx_dns_server_add(NX_DNS *dns_ptr, ULONG server_address);
UINT        _nxe_dns_server_remove(NX_DNS *dns_ptr, ULONG server_address);
UINT        _nx_dns_server_remove(NX_DNS *dns_ptr, ULONG server_address);
UINT        _nxde_dns_host_by_address_get(NX_DNS *dns_ptr, NXD_ADDRESS *host_address_ptr, UCHAR *host_name_ptr, UINT host_name_buffer_size, ULONG wait_option);
UINT        _nxd_dns_host_by_address_get(NX_DNS *dns_ptr, NXD_ADDRESS *host_address_ptr, UCHAR *host_name_ptr, UINT host_name_buffer_size, ULONG wait_option);
UINT        _nxde_dns_host_by_name_get(NX_DNS *dns_ptr, UCHAR *host_name, NXD_ADDRESS *host_address_ptr, ULONG wait_option, UINT lookup_type);
UINT        _nxd_dns_host_by_name_get(NX_DNS *dns_ptr, UCHAR *host_name, NXD_ADDRESS *host_address_ptr, ULONG wait_option, UINT lookup_type);
UINT        _nxde_dns_server_add(NX_DNS *dns_ptr, NXD_ADDRESS *server_address);
UINT        _nxd_dns_server_add(NX_DNS *dns_ptr, NXD_ADDRESS *server_address);
UINT        _nxde_dns_server_remove(NX_DNS *dns_ptr, NXD_ADDRESS *server_address);
UINT        _nxd_dns_server_remove(NX_DNS *dns_ptr, NXD_ADDRESS *server_address);

#endif

/* Internal DNS functions. */

VOID        _nxd_dns_build_an_ipv6_question_string(NXD_ADDRESS *ip_address, UCHAR *buffer, UINT len);
UINT        _nx_dns_header_create(UCHAR *buffer_ptr, USHORT id, USHORT flags);
UINT        _nx_dns_new_packet_create(NX_DNS *dns_ptr, NX_PACKET *packet_ptr, USHORT id, UCHAR *name, USHORT type);
UINT        _nx_dns_name_size_calculate(UCHAR *name);
UINT        _nx_dns_name_string_encode(UCHAR *ptr, UCHAR *name);
UINT        _nx_dns_name_string_unencode(UCHAR *data, UINT start, UCHAR *buffer, UINT size);
VOID        _nx_dns_long_to_network_convert(UCHAR *ptr, ULONG value);
USHORT      _nx_dns_network_to_short_convert(UCHAR *ptr);
ULONG       _nx_dns_network_to_long_convert(UCHAR *ptr);
UINT        _nx_dns_question_add(NX_PACKET *packet_ptr, UCHAR *name, USHORT type);
UCHAR *     _nx_dns_resource_data_address_get(UCHAR *resource);
UINT        _nx_dns_resource_data_length_get(UCHAR *resource);
UINT        _nx_dns_resource_class_get(UCHAR *resource);
UINT        _nx_dns_resource_name_get(UCHAR *buffer, UINT start, UCHAR *destination, UINT size);
UINT        _nx_dns_resource_type_get(UCHAR *resource);
ULONG       _nx_dns_resource_time_to_live_get(UCHAR *resource);
UINT        _nx_dns_resource_size_get(UCHAR *resource);
VOID        _nx_dns_short_to_network_convert(UCHAR *ptr, USHORT value);
UINT        _nx_dns_number_to_ascii_convert(UINT number, CHAR *buffstring);
VOID        _nxd_dns_network_to_long_convert(UCHAR *ptr, NXD_ADDRESS *nxd_address);
UINT        _nxd_dns_send_query_by_address(NX_DNS *dns_ptr, NXD_ADDRESS *dns_server, UCHAR *ip_question, UCHAR *host_name_ptr, UINT host_name_buffer_size, ULONG wait_option);
UINT        _nxd_dns_send_query_by_name(NX_DNS *dns_ptr, NXD_ADDRESS *server_address, UCHAR *host_name, NXD_ADDRESS *host_address_ptr, UINT dns_record_type, ULONG wait_option);



/* Determine if a C++ compiler is being used.  If so, complete the standard
   C conditional started above.  */
#ifdef   __cplusplus
        }
#endif

#endif  

