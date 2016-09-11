/**************************************************************************/
/*                                                                        */
/*            Copyright (c) 1996-2010 by Express Logic Inc.               */
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
/** NetX SNTP Client Component                                            */
/**                                                                       */
/**   Simple Network Time Protocol (SNTP)                                 */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */
/*                                                                        */
/*    nx_sntp_client.h                                    PORTABLE C      */
/*                                                           5.2          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Express Logic, Inc.                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the NetX Simple Network Time Protocol (SNTP)      */
/*    Client component, including all data types and external references. */
/*    It is assumed that tx_api.h, tx_port.h, nx_api.h, and nx_port.h,    */
/*    have already been included.                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-25-2007     William E. Lamie         Initial Version 5.0           */
/*  08-15-2008     William E. Lamie         Modified comment(s), added    */
/*                                             logic for little Endian    */
/*                                             processing, backoff        */
/*                                             algorithm processing of    */
/*                                             polling intervals, zero    */
/*                                             SNTP data checking,        */
/*                                             handling failed sanity     */
/*                                             checks by error code,      */
/*                                             displaying NTP time in     */
/*                                             readable date time format  */
/*                                             resulting in version 5.1   */
/*  04-01-2010     William E. Lamie         Modified comment(s),          */ 
/*                                             resulting in version 5.2   */ 
/*                                                                        */
/**************************************************************************/

#ifndef NX_SNTP_CLIENT_H
#define NX_SNTP_CLIENT_H


/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */ 
extern   "C" {

#endif

#include "nx_sntp.h"

/* To use the utility for displaying NTP time in date time format
   readable by humans, set the current year.  */
#define NX_SNTP_CURRENT_YEAR        0

/* NX SNTP Client configurable options.  */


/* Scheme for filtering messages during program execution. 

   printf() itself may need to be defined for the specific 
   processor that is running the application and communication
   available (e.g. serial port).  */


#define NX_SNTP_CLIENT_EVENT_LOG(debug_level, msg)                  \
{                                                                   \
    UINT level = (UINT)debug_level;                                 \
    if (level <= ALL && NX_SNTP_CLIENT_DEBUG == ALL)                \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
    else if (level <= MODERATE && NX_SNTP_CLIENT_DEBUG == MODERATE) \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
    else if (level == SEVERE && NX_SNTP_CLIENT_DEBUG == SEVERE)     \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
    else if (level == LOG)                                          \
    {                                                               \
       printf msg ;                                                 \
    }                                                               \
}

/* Set the event reporting and debug output level for the NetX SNTP Client */

#ifndef NX_SNTP_CLIENT_DEBUG
#define NX_SNTP_CLIENT_DEBUG                         MODERATE
#endif


/* Set the NetX SNTP client stack size .  */

#ifndef NX_SNTP_CLIENT_STACK_SIZE
#define NX_SNTP_CLIENT_STACK_SIZE                   2048   
#endif


/* Set the client thread time slice.  */

#ifndef NX_SNTP_CLIENT_THREAD_TIME_SLICE
#define NX_SNTP_CLIENT_THREAD_TIME_SLICE            TX_NO_TIME_SLICE
#endif



#ifndef NX_SNTP_CLIENT_THREAD_PRIORITY                 
#define NX_SNTP_CLIENT_THREAD_PRIORITY              2
#endif


/* Set NetX SNTP client thread preemption threshold.  */

#ifndef NX_SNTP_CLIENT_PREEMPTION_THRESHOLD     
#define NX_SNTP_CLIENT_PREEMPTION_THRESHOLD         NX_SNTP_CLIENT_THREAD_PRIORITY
#endif


/* Configure client IP.  */

/* Set the default NetX SNTP client address.  */

#ifndef NX_SNTP_CLIENT_IP_ADDRESS                       
#define NX_SNTP_CLIENT_IP_ADDRESS                   IP_ADDRESS(192,2,2,34)
#endif


/* Set NetX IP helper thread stack size.  */

#ifndef NX_SNTP_CLIENT_IP_STACK_SIZE
#define NX_SNTP_CLIENT_IP_STACK_SIZE                2048 
#endif


/* Set NetX IP helper thread priority.  */

#ifndef NX_SNTP_CLIENT_IP_THREAD_PRIORITY              
#define NX_SNTP_CLIENT_IP_THREAD_PRIORITY           NX_SNTP_CLIENT_THREAD_PRIORITY 
#endif


/* Configure the NetX SNTP client network parameters */

/* Set Client UDP socket name.  */

#ifndef NX_SNTP_CLIENT_UDP_SOCKET_NAME     
#define NX_SNTP_CLIENT_UDP_SOCKET_NAME              "SNTP Client socket"    
#endif


/* Set port for client to connect to SNTP server.  */

#ifndef NX_SNTP_CLIENT_UDP_PORT              
#define NX_SNTP_CLIENT_UDP_PORT                    123     
#endif


/* Set Time to Live (TTL) value for transmitted UDP packets, including manycast and
   multicast transmissions. The default TTL for windows operating system time 
   server is used.  */

#ifndef NX_SNTP_CLIENT_TIME_TO_LIVE    
#define NX_SNTP_CLIENT_TIME_TO_LIVE                  NX_IP_TIME_TO_LIVE
#endif


/* Set maximum queue depth for client socket.*/

#ifndef NX_SNTP_CLIENT_MAX_QUEUE_DEPTH    
#define NX_SNTP_CLIENT_MAX_QUEUE_DEPTH               5
#endif


/* Set size of NetX SNTP client packet in bytes. This includes the packet header for UDP packets.
   Packet size should not exceed the device MTU. 

   The default client packet size includes authentication data (20 bytes per packet).  
   This can be removed in applications with limited memory resources. 

   14 - frame header
   20 - IP header
   8  - UDP header
   __
   42 bytes header data


   42 bytes header data
   48 bytes NTP time protocol (no authentication data)
   32 bytes fields used for internal processing
   __
   122 bytes total as the minimum packet size 
  
  If including authentication data, add more bytes:  
  + 20 bytes authentication data (optional)
   ___
   142 bytes total as the minimum packet size*/

#ifndef NX_SNTP_CLIENT_PACKET_SIZE
#define NX_SNTP_CLIENT_PACKET_SIZE                  122
/* If using authentication data, use this minimum packet size. 
#define NX_SNTP_CLIENT_PACKET_SIZE                  142
*/
#endif

/* Set size of header data from network Frame, IP, UDP and NetX in bytes.  */

#ifndef NX_SNTP_CLIENT_PACKET_HEADER_SIZE       
#define NX_SNTP_CLIENT_PACKET_HEADER_SIZE           42
#endif    


/* Set the size of the Client packet pool.  This default size accommodates
   10 packets, which is more than enough for typical time update operation.  */

#ifndef NX_SNTP_CLIENT_PACKET_POOL_SIZE       
#define NX_SNTP_CLIENT_PACKET_POOL_SIZE             NX_SNTP_CLIENT_PACKET_SIZE * 10
#endif    


/* Set the time out (timer ticks) for packet allocation from the client packet pool.  */

#ifndef NX_SNTP_CLIENT_PACKET_TIMEOUT
#define NX_SNTP_CLIENT_PACKET_TIMEOUT               (1 * NX_SNTP_TICKS_PER_SECOND)    
#endif


/* Set ARP cache memory size.  */

#ifndef NX_SNTP_CLIENT_ARP_CACHE_SIZE 
#define NX_SNTP_CLIENT_ARP_CACHE_SIZE               1040 
#endif

/* Configure Client NTP parameters and mode of operation.  */


/* Set the time out (timer ticks) to obtain the Client's server list exclusive lock.  */

#ifndef NX_SNTP_CLIENT_SERVER_LIST_WAIT
#define NX_SNTP_CLIENT_SERVER_LIST_WAIT             (1 * NX_SNTP_TICKS_PER_SECOND)
#endif


/* Set the maximum number of time servers on the Client server list.  */

#ifndef NX_SNTP_CLIENT_MAX_SERVERS
#define NX_SNTP_CLIENT_MAX_SERVERS                  6
#endif

/* Set the limit on size of optional text to can be added to a time update log entry.  */

#ifndef NX_SNTP_CLIENT_MAX_LOG_ENTRY
#define NX_SNTP_CLIENT_MAX_LOG_ENTRY                50
#endif


/* Disable the Client to run in test mode.  */

#ifndef NX_SNTP_CLIENT_RUN_IN_TEST_MODE             
#define NX_SNTP_CLIENT_RUN_IN_TEST_MODE             NX_FALSE
#endif



/* Enable Client logging to time update events.  */

#ifndef NX_SNTP_CLIENT_LOG_ENABLED
#define NX_SNTP_CLIENT_LOG_ENABLED                  NX_TRUE
#endif


/* Define the client's list of unicast servers, in order of decreasing preference. 
   For automatic/dynamic discovery only, set to the empty string.  */

#ifndef NX_SNTP_UDP_UNICAST_SERVER_ADDRESSES
#define NX_SNTP_UDP_UNICAST_SERVER_ADDRESSES        "192.2.2.35 192.2.2.100 64.125.78.85"
#endif


/* Set the Client manycast IP address to an empty string. This effectively
   disables manycast.  A standard manycast address is 239.1.1.1.   */

#ifndef NX_SNTP_CLIENT_MANYCAST_ADDRESS
#define NX_SNTP_CLIENT_MANYCAST_ADDRESS             ""  
#endif


/*  Set the Client broadcast subnet (domain).  */

#ifndef NX_SNTP_CLIENT_BROADCAST_DOMAIN  
#define NX_SNTP_CLIENT_BROADCAST_DOMAIN              "192.2.2.255"   
#endif


/*    Set the list of broadcast time servers in the Client subnet.  */

#ifndef NX_SNTP_UDP_BROADCAST_SERVER_ADDRESSES
#define NX_SNTP_UDP_BROADCAST_SERVER_ADDRESSES      "192.2.2.100 192.2.2.35"   
#endif


/* Set the Client multicast IP address to an empty string. This effectively
   disables multicast.  The standard multicast address is 224.0.1.1.   */

#ifndef NX_SNTP_CLIENT_MULTICAST_ADDRESS
#define NX_SNTP_CLIENT_MULTICAST_ADDRESS            ""  
#endif


/* Set the NTP/SNTP version of this NTP Client.  */

#ifndef NX_SNTP_CLIENT_NTP_VERSION
#define NX_SNTP_CLIENT_NTP_VERSION                   3
#endif


/* Set minimum NTP/SNTP version the Client will accept from its time server.  */

#ifndef NX_SNTP_CLIENT_MIN_NTP_VERSION
#define NX_SNTP_CLIENT_MIN_NTP_VERSION               3
#endif



/* Define the minimum (numerically highest) stratum the Client will 
   accept for a time server. Valid range is 1 - 15.  */

#ifndef NX_SNTP_CLIENT_MIN_SERVER_STRATUM
#define NX_SNTP_CLIENT_MIN_SERVER_STRATUM           2
#endif


/* Define minimum time difference (usec) between server and client time
   the Client requires to change its local time.  */

#ifndef NX_SNTP_CLIENT_MIN_TIME_ADJUSTMENT
#define NX_SNTP_CLIENT_MIN_TIME_ADJUSTMENT          10
#endif


/* Define maximum time update (msec) the Client will make to its local time
   per update.  */
        
#ifndef NX_SNTP_CLIENT_MAX_TIME_ADJUSTMENT
#define NX_SNTP_CLIENT_MAX_TIME_ADJUSTMENT          180000
#endif

/* Set the Client to ignore the maximum time adjustment on startup.  */

#ifndef NX_SNTP_CLIENT_IGNORE_MAX_ADJUST_STARTUP    
#define NX_SNTP_CLIENT_IGNORE_MAX_ADJUST_STARTUP    NX_TRUE
#endif


/* Set the maximum time lapse (sec) without a time update that can be tolerated by 
   the Client (application). This should be determined by application time sensitivity. 
   Here it is set to allow up to three unicast servers timeout to expire before the max 
   lapse timeout expires.  */

#ifndef NX_SNTP_CLIENT_MAX_TIME_LAPSE
#define NX_SNTP_CLIENT_MAX_TIME_LAPSE               (3 * NX_SNTP_CLIENT_UNICAST_SERVER_TIMEOUT)
#endif


/* Define a time out (sec) for the main update timer. 
   With each timer expiration, the Client nx_sntp_update_time_remaining parameter
   is decremented by this interval.  When nx_sntp_update_time_remaining is zero,
   the main update timer has expired.  */

#ifndef NX_SNTP_UPDATE_TIMEOUT_INTERVAL
#define NX_SNTP_UPDATE_TIMEOUT_INTERVAL             1
#endif


/* Set the unicast poll interval (sec) between Client request's for time updates.  */

#ifndef NX_SNTP_CLIENT_UNICAST_POLL_INTERVAL    
#define NX_SNTP_CLIENT_UNICAST_POLL_INTERVAL        3600
#endif


/* Set the unicast server timeout (sec) without a valid update from the current server.  */

#ifndef NX_SNTP_CLIENT_UNICAST_SERVER_TIMEOUT    
#define NX_SNTP_CLIENT_UNICAST_SERVER_TIMEOUT       (3 * NX_SNTP_CLIENT_UNICAST_POLL_INTERVAL)
#endif


/* Set the Client exponential back off rate for extending Client poll interval.
   To effectively disable, set to 1.  */

#ifndef NX_SNTP_CLIENT_EXP_BACKOFF_RATE             
#define NX_SNTP_CLIENT_EXP_BACKOFF_RATE             2
#endif


/* Set the broadcast server timeout (sec) without a valid update from the current server.  */

#ifndef NX_SNTP_CLIENT_BROADCAST_SERVER_TIMEOUT    
#define NX_SNTP_CLIENT_BROADCAST_SERVER_TIMEOUT     3600
#endif


/* Set the initial unicast request timeout (sec) for the broadcast Client on its first
   update from the broadcast server. This should be kept short because the server either
   will or it won't support unicast, so no need to wait very long.  */

#ifndef NX_SNTP_CLIENT_INITIAL_UNICAST_TIMEOUT    
#define NX_SNTP_CLIENT_INITIAL_UNICAST_TIMEOUT      10
#endif


/* Define the upper limit of server clock dispersion (usec) the Client
   will accept. To disable this check, set this parameter to 0x0.  */

#ifndef NX_SNTP_CLIENT_MAX_ROOT_DISPERSION
#define NX_SNTP_CLIENT_MAX_ROOT_DISPERSION          500
#endif


/* Set the limit on consecutive bad updates from server before Client 
   switches to alternate server.  */

#ifndef NX_SNTP_CLIENT_BAD_UPDATE_LIMIT             
#define NX_SNTP_CLIENT_BAD_UPDATE_LIMIT             3
#endif


/* Define status levels for SNTP update processing */

#define UPDATE_STATUS_CONTINUE                      1
#define UPDATE_STATUS_BREAK                         2
#define UPDATE_STATUS_ERROR                         3
#define UPDATE_STATUS_SUCCESS                       4


#define    NTP_SECONDS_AT_01011999  3124137600

/* Define which epoch time is relative on the host system. */

#define UNIX_EPOCH                 1
#define NTP_EPOCH                  2

    

/* Enumerate months*/

#define JANUARY         1
#define FEBRUARY        2
#define MARCH           3
#define APRIL           4
#define MAY             5
#define JUNE            6
#define JULY            7
#define AUGUST          8
#define SEPTEMBER       9
#define OCTOBER         10
#define NOVEMBER        11
#define DECEMBER        12


/* Compute seconds per month for convenience computating date and time. */

#define SEC_IN_JAN           (31 * SECONDS_PER_DAY)
#define SEC_IN_LEAPFEB       (29 * SECONDS_PER_DAY) 
#define SEC_IN_NONLEAPFEB    (28 * SECONDS_PER_DAY)
#define SEC_IN_MAR           (31 * SECONDS_PER_DAY)
#define SEC_IN_APR           (30 * SECONDS_PER_DAY)
#define SEC_IN_MAY           (31 * SECONDS_PER_DAY)
#define SEC_IN_JUN           (30 * SECONDS_PER_DAY)
#define SEC_IN_JUL           (31 * SECONDS_PER_DAY)
#define SEC_IN_AUG           (31 * SECONDS_PER_DAY)
#define SEC_IN_SEP           (30 * SECONDS_PER_DAY)
#define SEC_IN_OCT           (31 * SECONDS_PER_DAY)
#define SEC_IN_NOV           (30 * SECONDS_PER_DAY)
#define SEC_IN_DEC           (31 * SECONDS_PER_DAY)

/* Compute seconds per year, month,day for convenience computating date and time. */

#define SECONDS_PER_LEAPYEAR        (86400 * 366)
#define SECONDS_PER_NONLEAPYEAR     (86400 * 365)
#define SECONDS_PER_DAY             86400
#define SECONDS_PER_HOUR            3600
#define SECONDS_PER_MINUTE          60

/* Define the Netx Date Time structure.  */

    typedef struct NX_SNTP_DATE_TIME_STRUCT
    {
        UINT     year;
        UINT     month;
        UINT     day;
        UINT     hour;
        UINT     minute;
        UINT     second;
        UINT     millisecond;                                               /* This is the fraction part of the NTP time data. */
        UINT     time_zone;                                                 /* NTP time is represented in Coordinated Universal Time (UTC). */
        UINT     leap_year;                                                 /* Indicates if current time is in a leap year. */

    } NX_SNTP_DATE_TIME;


/* Define the Netx SNTP Time structure.  */

    typedef struct NX_SNTP_TIME_STRUCT
    {
        ULONG    seconds;                                                   /* Seconds, in the 32 bit field of an NTP time data.  */
        ULONG    fraction;                                                  /* Fraction of a second, in the 32 bit fraction field of an NTP time data.  */

    } NX_SNTP_TIME;


/* Define the NetX SNTP Time Message structure.  The Client only uses the flags field and the transmit_time_stamp field
   in time requests it sends to its time server.  */

    typedef struct NX_SNTP_TIME_MESSAGE_STRUCT
    {
        /* These are represented as 8 bit data fields in the time message format.  */
        UINT flags;                                                         /* Flag containing host's NTP version, mode and leap indicator.  */
        UINT peer_clock_stratum;                                            /* Level of precision in the NTP network. Applicable only in server NTP messages.  */
        UINT peer_poll_interval;                                            /* Frequency at which an NTP host polls its NTP peer. Applicable only in server NTP messages.  */
        UINT peer_clock_precision;                                          /* Precision of the NTP server clock. Applicable only in server NTP messages.  */

        /* These are represented as 32 bit data fields in the time message format*/
        ULONG root_delay;                                                   /* Round trip time from NTP Server to primary reference source. Applicable only in server NTP messages.  */
        ULONG clock_dispersion;                                             /* Server reference clock type (but if stratum is zero, indicates server status when not able to send time updates.  */ 
        UCHAR reference_clock_id[4];                                        /* Maximum error in server clock based to the clock frequency tolerance. Applicable only in server NTP messages.  */ 

        /* These are represented as 64 bit data fields in the time message format*/
        UCHAR reference_clock_update_time_stamp[8];                         /* Time at which the server clock was last set or corrected in a server time message.  */
        UCHAR originate_time_stamp[8];                                      /* Time at which the Client update request left the Client in a server time message.  */
        UCHAR receive_time_stamp[8];                                        /* Time at which the server received the Client request in a server time message.  */
        UCHAR transmit_time_stamp[8];                                       /* Time at which the server transmitted its reply to the Client in a server time message (or the time client request was sent in the client request message).  */

        /* Optional authenticator fields.  */
        UCHAR KeyIdentifier[4];                                             /* Key Identifier and Message Digest fields contain the...  */
        UCHAR MessageDigest[16];                                            /* ...message authentication code (MAC) information defined.  */ 

        /* These fields are used internally for 'convert' UCHAR data in NX_SNTP_TIME data e.g. seconds and fractions.  */
        NX_SNTP_TIME reference_clock_update_time;                           /* Time at which the server clock was last set or corrected in a server time message.  */
        NX_SNTP_TIME originate_time;                                        /* Time at which the Client update request left the Client in a server time message.  */
        NX_SNTP_TIME receive_time;                                          /* Time at which the server received the Client request in a server time message.  */
        NX_SNTP_TIME transmit_time;                                         /* Time at which the server transmitted its reply to the Client in a server time message (or the time client request was sent in the client request message).  */

    } NX_SNTP_TIME_MESSAGE;


/* Define the SNTP client structure.  */

    typedef struct NX_SNTP_CLIENT_STRUCT
    {
        ULONG                           nx_sntp_client_id;                       /* SNTP ID for identifying the client service task.  */
        NX_IP                           *ip_ptr;                                 /* Pointer to the Client IP instance.  */
        NX_PACKET_POOL                  *packet_pool_ptr;                        /* Pointer to the Client packet pool.  */
        ULONG                           server_ip_address;                       /* Client's current time server IP address.  */
        USHORT                          server_port;                             /* Port for receiving UDP data (e.g. time updates).  */
        NX_UDP_SOCKET                   udp_socket;                              /* Client UDP socket for sending and receiving time updates.  */
        UINT                            max_queue_depth;                         /* Maximum packet queue stored in the Client UDP socket.  */
        ULONG                           time_to_live;                            /* UDP socket time to live value.  */

        /* Generic operating parameters.  */
        UINT                            protocol_mode;                           /* Type of association as per RFC protocol e.g. Client, Server. Must be set to Client (3) for this API.  */
        UINT                            operating_mode;                          /* Mode of operation, either unicast or broadcast */
        ULONG                           min_time_adjustment;                     /* Minimum time adjustment (msec) for Client to change its local time.  */
        ULONG                           max_time_adjustment;                     /* Maximum time adjustment (msec) Client will make during a single update to its local time.  */
        UINT                            ignore_max_adjustment_on_startup;        /* Enable maximum time adjustment on startup e.g. first update Client receives from server.  */
        ULONG                           max_timelapse_without_update;            /* Maximum time (sec) Client task will run without a valid update from its server.  */
        UINT                            exponential_backoff_rate;                /* Rate (e.g. double, triple, or not at all), to increase the Client unicast poll interval when an update is missed. This is the RFC protocol 'back off' algorithm.  */
        UINT                            invalid_time_update_limit;               /* Limit on consecutive faulty/invalid data received from server.  */
        ULONG                           max_root_dispersion;                     /* Maximum server clock dispersion (usec) Client will accept.  */
        UINT                            client_requires_rtt;                     /* Set the Client to require a round trip time computed for its current server.  */
        UINT                            test_mode;                               /* Enable 'test' mode: time updates are not applied to local time.  */

        /* Broadcast parameters.  */
        UINT                            broadcast_initialized;                   /* Client task is ready to receive broadcast time data.  */
        ULONG                           broadcast_time_servers[NX_SNTP_CLIENT_MAX_SERVERS];   
                                                                                 /* Client list of broadcast time servers.  */
        TX_MUTEX                        broadcast_server_list_mutex;             /* Mutex lock on Client list of broadcast servers.  */ 
        ULONG                           broadcast_server_domain;                 /* Domain address to listen for broadcasts e.g. 192.2.2.255 */
        ULONG                           broadcast_server_timeout;                /* Time out (sec) for receiving next server broadcast.  */
        ULONG                           initial_unicast_timeout;                 /* Time out (sec) for receiving server response to initial unicast request.  */
        UINT                            send_initial_unicast;                    /* Enable option to send an initial unicast request to a broadcast server to compute round trip time.  */
        ULONG                           multicast_server_address;                /* IP address Client should listen on to receive broadcasts from a multicast server.  */
                                                                                 
        /* Unicast parameters.  */
        UINT                            unicast_initialized;                     /* Client task is ready to receive unicast time data.  */
        ULONG                           unicast_time_servers[NX_SNTP_CLIENT_MAX_SERVERS];   /* List of acceptable unicast time servers.  */
        TX_MUTEX                        unicast_server_list_mutex;               /* Mutex for protecting access to list of unicast servers */ 
        ULONG                           unicast_server_timeout;                  /* Time out for a specific unicast server to receive valid update.  */
        ULONG                           unicast_poll_interval;                   /* Unicast interval at which client is polling the time server.  */
        UINT                            randomize_wait_on_startup;               /* Enable varying timeout on startup or reset to avoid overloading LAN time server.  */
        ULONG                           manycast_server_address;                 /* IP address to send to for manycast servers.  */

        /* Internal time parameters and computations */
        TX_TIMER                        nx_sntp_update_timer;                    /* SNTP update timer; expires when no data received for specified time lapse.  */
        ULONG                           nx_sntp_update_timer_timeout;            /* Time (sec) interval when update timer is scheduled to run. It decrements nx_sntp_update_time_remaining till it hits zero.  */
        ULONG                           nx_sntp_update_time_remaining;           /* Time (sec) remaining that the Client task can continue running without receiving a valid update.  */
        UINT                            unicast_server_num;                      /* Number of valid unicast servers in Client list.  */
        UINT                            broadcast_server_num;                    /* Number of valid broadcast servers in Client list.  */
        LONG                            roundtrip_time_msec;                     /* Round trip time (msec) for a packet to travel to server and back to client. Does not include server processing time.  */
        LONG                            system_clock_offset_msec;                /* Offset (msec) between system (server) clock and client clock.  */
         
        /* Time messages sent/received */
        NX_SNTP_TIME_MESSAGE            current_server_time_message;             /* Time update which the Client has just received from its server.  */
        NX_SNTP_TIME_MESSAGE            current_time_message_request;            /* Client request to send to its time server.  */

        NX_SNTP_TIME_MESSAGE            previous_server_time_message;            /* Previous valid time update received from the Client time server.  */
        NX_SNTP_TIME                    local_ntp_time;                          /* Client's notion of local time.  */
        NX_SNTP_TIME                    server_update_time;                      /* Time (based on client local time) when the server update was received in response to the current request.  */

        /* Application defined services.  */
        UINT                            (*get_local_device_time)(NX_SNTP_TIME *time_ptr); 
                                                                                 /* Pointer to callback service for getting local device time.  */
        UINT                            (*set_local_device_time)(NX_SNTP_TIME *time_ptr); 
                                                                                 /* Pointer to callback service for setting local device time.  */
        UINT                            (*adjust_local_device_time)(LONG msec);  /* Pointer to callback service for adjusting local device time.  */
        UINT                            (*apply_custom_sanity_checks)(struct NX_SNTP_CLIENT_STRUCT *client_ptr, NX_SNTP_TIME_MESSAGE *client_time_msg_ptr, NX_SNTP_TIME_MESSAGE *server_time_msg_ptr);
                                                                                 /* Pointer to callback service for  performing additional sanity checks on received time data.  */
        UINT                            (*leap_second_handler)(struct NX_SNTP_CLIENT_STRUCT *client_ptr, UINT indicator);   
                                                                                 /* Pointer to callback service for handling an impending leap second.  */
        UINT                            (*kiss_of_death_handler)(struct NX_SNTP_CLIENT_STRUCT *client_ptr, NX_SNTP_TIME_MESSAGE *server_time_msg);
                                                                                 /* Pointer to callback service for handling kiss of death packets received from server.  */   
        VOID                            (*random_number_generator)(struct NX_SNTP_CLIENT_STRUCT *client_ptr, ULONG *rand);
                                                                                 /* Pointer to callback service for random number generator.  */

    } NX_SNTP_CLIENT;


#ifndef     NX_SNTP_SOURCE_CODE     

/* Define the system API mappings based on the error checking selected by the user.   */


/* Determine if error checking is desired.  If so, map API functions
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work.
   Note: error checking is enabled by default.  */


#ifdef NX_SNTP_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define   nx_sntp_client_add_server_IP_to_list              _nx_sntp_client_add_server_IP_to_list
#define   nx_sntp_client_add_servers_from_input_list        _nx_sntp_client_add_servers_from_input_list
#define   nx_sntp_client_apply_sanity_checks                _nx_sntp_client_apply_sanity_checks
#define   nx_sntp_client_calculate_roundtrip                _nx_sntp_client_calculate_roundtrip
#define   nx_sntp_client_check_server_clock_dispersion      _nx_sntp_client_check_server_clock_dispersion
#define   nx_sntp_client_create                             _nx_sntp_client_create
#define   nx_sntp_client_delete                             _nx_sntp_client_delete
#define   nx_sntp_client_find_server_in_list                _nx_sntp_client_find_server_in_list
#define   nx_sntp_client_get_next_server                    _nx_sntp_client_get_next_server
#define   nx_sntp_client_get_server_roundtrip               _nx_sntp_client_get_server_roundtrip
#define   nx_sntp_client_initialize_broadcast               _nx_sntp_client_initialize_broadcast
#define   nx_sntp_client_initialize_unicast                 _nx_sntp_client_initialize_unicast
#define   nx_sntp_client_process_time_data                  _nx_sntp_client_process_time_data
#define   nx_sntp_client_receive_time_update                _nx_sntp_client_receive_time_update
#define   nx_sntp_client_remove_server_from_list            _nx_sntp_client_remove_server_from_list
#define   nx_sntp_client_reset_broadcast                    _nx_sntp_client_reset_broadcast
#define   nx_sntp_client_reset_unicast                      _nx_sntp_client_reset_unicast
#define   nx_sntp_client_run_broadcast                      _nx_sntp_client_run_broadcast
#define   nx_sntp_client_run_unicast                        _nx_sntp_client_run_unicast
#define   nx_sntp_client_send_unicast_request               _nx_sntp_client_send_unicast_request
#define   nx_sntp_client_utility_add_msecs_to_NTPtime       _nx_sntp_client_utility_add_msecs_to_NTPtime
#define   nx_sntp_client_utility_add_NTPtime                _nx_sntp_client_utility_add_NTPtime
#define   nx_sntp_client_utility_convert_fraction_to_msecs  _nx_sntp_client_utility_convert_fraction_to_msecs
#define   nx_sntp_client_utility_convert_LONG_to_IP         _nx_sntp_client_utility_convert_LONG_to_IP
#define   nx_sntp_client_utility_convert_refID_KOD_code     _nx_sntp_client_utility_convert_refID_KOD_code
#define   nx_sntp_client_utility_convert_seconds_to_date    _nx_sntp_client_utility_convert_seconds_to_date
#define   nx_sntp_client_utility_display_NTP_time           _nx_sntp_client_utility_display_NTP_time
#define   nx_sntp_client_utility_display_date_time          _nx_sntp_client_utility_display_date_time
#define   nx_sntp_client_utility_get_msec_diff              _nx_sntp_client_utility_get_msec_diff


#else

/* Services with error checking.  */

#define   nx_sntp_client_add_server_IP_to_list              _nxe_sntp_client_add_server_IP_to_list
#define   nx_sntp_client_add_servers_from_input_list        _nxe_sntp_client_add_servers_from_input_list
#define   nx_sntp_client_apply_sanity_checks                _nxe_sntp_client_apply_sanity_checks
#define   nx_sntp_client_calculate_roundtrip                _nxe_sntp_client_calculate_roundtrip
#define   nx_sntp_client_check_server_clock_dispersion      _nxe_sntp_client_check_server_clock_dispersion
#define   nx_sntp_client_create                             _nxe_sntp_client_create
#define   nx_sntp_client_delete                             _nxe_sntp_client_delete
#define   nx_sntp_client_find_server_in_list                _nxe_sntp_client_find_server_in_list
#define   nx_sntp_client_get_next_server                    _nxe_sntp_client_get_next_server
#define   nx_sntp_client_get_server_roundtrip               _nxe_sntp_client_get_server_roundtrip
#define   nx_sntp_client_initialize_broadcast               _nxe_sntp_client_initialize_broadcast
#define   nx_sntp_client_initialize_unicast                 _nxe_sntp_client_initialize_unicast
#define   nx_sntp_client_process_time_data                  _nxe_sntp_client_process_time_data
#define   nx_sntp_client_receive_time_update                _nxe_sntp_client_receive_time_update
#define   nx_sntp_client_remove_server_from_list            _nxe_sntp_client_remove_server_from_list
#define   nx_sntp_client_reset_broadcast                    _nxe_sntp_client_reset_broadcast
#define   nx_sntp_client_reset_unicast                      _nxe_sntp_client_reset_unicast
#define   nx_sntp_client_run_broadcast                      _nxe_sntp_client_run_broadcast
#define   nx_sntp_client_run_unicast                        _nxe_sntp_client_run_unicast
#define   nx_sntp_client_send_unicast_request               _nxe_sntp_client_send_unicast_request
#define   nx_sntp_client_utility_add_msecs_to_NTPtime       _nxe_sntp_client_utility_add_msecs_to_NTPtime
#define   nx_sntp_client_utility_add_NTPtime                _nxe_sntp_client_utility_add_NTPtime
#define   nx_sntp_client_utility_convert_fraction_to_msecs  _nxe_sntp_client_utility_convert_fraction_to_msecs
#define   nx_sntp_client_utility_convert_LONG_to_IP         _nxe_sntp_client_utility_convert_LONG_to_IP
#define   nx_sntp_client_utility_convert_refID_KOD_code     _nxe_sntp_client_utility_convert_refID_KOD_code
#define   nx_sntp_client_utility_convert_seconds_to_date    _nxe_sntp_client_utility_convert_seconds_to_date
#define   nx_sntp_client_utility_display_NTP_time           _nxe_sntp_client_utility_display_NTP_time
#define   nx_sntp_client_utility_display_date_time          _nxe_sntp_client_utility_display_date_time
#define   nx_sntp_client_utility_get_msec_diff              _nxe_sntp_client_utility_get_msec_diff

#endif /* NX_SNTP_DISABLE_ERROR_CHECKING */


/* Define the prototypes accessible to the application software.  */

    UINT    nx_sntp_client_add_server_IP_to_list(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, CHAR *server_to_add);
    UINT    nx_sntp_client_add_servers_from_input_list(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, CHAR *static_server_list);
    UINT    nx_sntp_client_apply_sanity_checks(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *client_time_msg_ptr, NX_SNTP_TIME_MESSAGE *server_time_msg_ptr);
    UINT    nx_sntp_client_calculate_roundtrip(NX_SNTP_TIME *time_server_update_received, NX_SNTP_TIME_MESSAGE *server_time_message, INT *roundtrip_time, INT *offset_time);
    UINT    nx_sntp_client_check_server_clock_dispersion(NX_SNTP_CLIENT *client_ptr, ULONG server_clock_dispersion, UINT *dispersion_ok);
    UINT    nx_sntp_client_create(NX_SNTP_CLIENT *client_ptr, NX_IP *ip_ptr, NX_PACKET_POOL *packet_pool_ptr, 
                                    ULONG time_to_live, UINT max_queue_depth, UINT port, UINT operating_mode, ULONG min_time_adjustment, 
                                    ULONG max_time_adjustment, UINT exponential_backoff_rate, ULONG max_timelapse_without_update,  
                                    UINT invalid_time_update_limit, ULONG max_root_dispersion, UINT ignore_max_adjust_on_startup, UINT test_mode,
                                    UINT (*get_local_device_time)(NX_SNTP_TIME *time_ptr),
                                    UINT (*set_local_device_time)(NX_SNTP_TIME *time_ptr),
                                    UINT (*apply_custom_sanity_checks)(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *client_time_msg_ptr, NX_SNTP_TIME_MESSAGE *server_time_msg_ptr),
                                    UINT (*adjust_local_device_time)(LONG msecs), 
                                    UINT (*leap_second_handler)(NX_SNTP_CLIENT *client_ptr, UINT indicator), 
                                    UINT (*kiss_of_death_handler)(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *server_time_msg),
                                    VOID (random_number_generator)(struct NX_SNTP_CLIENT_STRUCT *client_ptr, ULONG *rand));
    UINT    nx_sntp_client_delete (NX_SNTP_CLIENT *client_ptr);
    UINT    nx_sntp_client_find_server_in_list(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, ULONG server_to_find, INT *index);
    UINT    nx_sntp_client_get_next_server(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, UINT *search_index, UINT wrap);
    UINT    nx_sntp_client_get_server_roundtrip(NX_SNTP_CLIENT *client_ptr, ULONG timeout, UINT incoming_address_must_match);
    UINT    nx_sntp_client_initialize_broadcast(NX_SNTP_CLIENT *client_ptr, ULONG broadcast_server_timeout, ULONG initial_unicast_timeout, UINT send_unicast_on_startup, UINT randomize_wait_on_startup, CHAR *broadcast_domain,  CHAR *multicast_server_address,CHAR *broadcast_time_servers);
    UINT    nx_sntp_client_initialize_unicast(NX_SNTP_CLIENT *client_ptr, ULONG unicast_server_timeout, ULONG unicast_poll_interval, UINT randomize_wait_on_startup, UINT client_requires_rtt, CHAR *manycast_server_address, CHAR *unicast_time_servers);
    UINT    nx_sntp_client_process_time_data(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *server_time_message, UINT first);
    UINT    nx_sntp_client_receive_time_update(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *time_message, ULONG timeout, UINT incoming_address_must_match);
    UINT    nx_sntp_client_remove_server_from_list(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, ULONG server_to_remove);
    UINT    nx_sntp_client_reset_broadcast(NX_SNTP_CLIENT *client_ptr);
    UINT    nx_sntp_client_reset_unicast(NX_SNTP_CLIENT *client_ptr);
    UINT    nx_sntp_client_run_broadcast(NX_SNTP_CLIENT *client_ptr);
    UINT    nx_sntp_client_run_unicast(NX_SNTP_CLIENT *client_ptr);
    UINT    nx_sntp_client_send_unicast_request(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *unicast_request);
    UINT    nx_sntp_client_utility_add_msecs_to_NTPtime(NX_SNTP_TIME *timeA_ptr, LONG msecs_to_add);
    UINT    nx_sntp_client_utility_add_NTPtime(NX_SNTP_TIME *timeA_ptr, NX_SNTP_TIME *timeB_ptr, NX_SNTP_TIME *sum_time_ptr);
    UINT    nx_sntp_client_utility_convert_fraction_to_msecs(ULONG *milliseconds, NX_SNTP_TIME *time_ptr);
    UINT    nx_sntp_client_utility_convert_refID_KOD_code(UCHAR *reference_id, UINT *code_id);
    UINT    nx_sntp_client_utility_convert_LONG_to_IP(CHAR *buffer, ULONG IP_UL);
    UINT    nx_sntp_client_utility_convert_seconds_to_date(NX_SNTP_TIME *current_NTP_time_ptr, UINT current_year, NX_SNTP_DATE_TIME *current_date_time_ptr);
    UINT    nx_sntp_client_utility_display_NTP_time(NX_SNTP_TIME *time_ptr, CHAR *title, UINT log_time_event);
    UINT    nx_sntp_client_utility_display_date_time(CHAR *buffer, UINT length, NX_SNTP_DATE_TIME *current_date_time_ptr);
    UINT    nx_sntp_client_utility_get_msecs_from_NTPTime(ULONG *milliseconds, NX_SNTP_TIME *time_ptr, ULONG fraction_only);

#else  /*  NX_SNTP_SOURCE_CODE */


/* SNTP source code is being compiled, do not perform any API mapping.  */

    UINT    _nx_sntp_client_add_server_IP_to_list(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, CHAR *server_to_add);
    UINT    _nxe_sntp_client_add_server_IP_to_list(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, CHAR *server_to_add);
    UINT    _nx_sntp_client_add_servers_from_input_list(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, CHAR *static_server_list);
    UINT    _nxe_sntp_client_add_servers_from_input_list(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, CHAR *static_server_list);
    UINT    _nx_sntp_client_apply_sanity_checks(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *client_time_msg_ptr, NX_SNTP_TIME_MESSAGE *server_time_msg_ptr);
    UINT    _nxe_sntp_client_apply_sanity_checks(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *client_time_msg_ptr, NX_SNTP_TIME_MESSAGE *server_time_msg_ptr);
    UINT    _nx_sntp_client_calculate_roundtrip(NX_SNTP_TIME *time_server_update_received, NX_SNTP_TIME_MESSAGE *server_time_message, LONG *roundtrip_time, LONG *offset_time);
    UINT    _nxe_sntp_client_calculate_roundtrip(NX_SNTP_TIME *time_server_update_received, NX_SNTP_TIME_MESSAGE *server_time_message, LONG *roundtrip_time, LONG *offset_time);
    UINT    _nx_sntp_client_check_server_clock_dispersion(NX_SNTP_CLIENT *client_ptr, ULONG server_clock_dispersion, UINT *dispersion_ok);
    UINT    _nxe_sntp_client_check_server_clock_dispersion(NX_SNTP_CLIENT *client_ptr, ULONG server_clock_dispersion, UINT *dispersion_ok);
    UINT    _nx_sntp_client_create(NX_SNTP_CLIENT *client_ptr, NX_IP *ip_ptr, NX_PACKET_POOL *packet_pool_ptr, 
                                    ULONG time_to_live, UINT max_queue_depth, UINT port, UINT operating_mode, ULONG min_time_adjustment, 
                                    ULONG max_time_adjustment, UINT exponential_backoff_rate, ULONG max_timelapse_without_update,  
                                    UINT invalid_time_update_limit, ULONG max_root_dispersion, UINT ignore_max_adjust_on_startup, UINT test_mode,
                                    UINT (*get_local_device_time)(NX_SNTP_TIME *time_ptr),
                                    UINT (*set_local_device_time)(NX_SNTP_TIME *time_ptr),
                                    UINT (*apply_custom_sanity_checks)(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *client_time_msg_ptr, NX_SNTP_TIME_MESSAGE *server_time_msg_ptr),
                                    UINT (*adjust_local_device_time)(LONG msecs), 
                                    UINT (*leap_second_handler)(NX_SNTP_CLIENT *client_ptr, UINT indicator), 
                                    UINT (*kiss_of_death_handler)(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *server_time_msg),
                                    VOID (random_number_generator)(struct NX_SNTP_CLIENT_STRUCT *client_ptr, ULONG *rand));
    UINT    _nxe_sntp_client_create(NX_SNTP_CLIENT *client_ptr, NX_IP *ip_ptr, NX_PACKET_POOL *packet_pool_ptr, 
                                    ULONG time_to_live, UINT max_queue_depth, UINT port, UINT operating_mode, ULONG min_time_adjustment, 
                                    ULONG max_time_adjustment, UINT exponential_backoff_rate, ULONG max_timelapse_without_update,  
                                    UINT invalid_time_update_limit, ULONG max_root_dispersion, UINT ignore_max_adjust_on_startup, UINT test_mode,
                                    UINT (*get_local_device_time)(NX_SNTP_TIME *time_ptr),
                                    UINT (*set_local_device_time)(NX_SNTP_TIME *time_ptr),
                                    UINT (*apply_custom_sanity_checks)(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *client_time_msg_ptr, NX_SNTP_TIME_MESSAGE *server_time_msg_ptr),
                                    UINT (*adjust_local_device_time)(LONG msecs), 
                                    UINT (*leap_second_handler)(NX_SNTP_CLIENT *client_ptr, UINT indicator), 
                                    UINT (*kiss_of_death_handler)(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *server_time_msg),
                                    VOID (random_number_generator)(struct NX_SNTP_CLIENT_STRUCT *client_ptr, ULONG *rand));
    UINT    _nx_sntp_client_delete (NX_SNTP_CLIENT *client_ptr);
    UINT    _nxe_sntp_client_delete (NX_SNTP_CLIENT *client_ptr);
    UINT    _nx_sntp_client_find_server_in_list(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, ULONG server_to_find, INT *index);
    UINT    _nxe_sntp_client_find_server_in_list(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, ULONG server_to_find, INT *index);
    UINT    _nx_sntp_client_get_next_server(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, UINT *search_index, UINT wrap);
    UINT    _nxe_sntp_client_get_next_server(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, UINT *search_index, UINT wrap);
    UINT    _nx_sntp_client_get_server_roundtrip(NX_SNTP_CLIENT *client_ptr, ULONG timeout, UINT incoming_address_must_match);
    UINT    _nxe_sntp_client_get_server_roundtrip(NX_SNTP_CLIENT *client_ptr, ULONG timeout, UINT incoming_address_must_match);
    UINT    _nx_sntp_client_initialize_broadcast(NX_SNTP_CLIENT *client_ptr, ULONG broadcast_server_timeout, ULONG initial_unicast_timeout, UINT send_unicast_on_startup, UINT randomize_wait_on_startup,CHAR *broadcast_domain,  CHAR * multicast_server_address,CHAR *broadcast_time_servers);
    UINT    _nxe_sntp_client_initialize_broadcast(NX_SNTP_CLIENT *client_ptr, ULONG broadcast_server_timeout, ULONG initial_unicast_timeout, UINT send_unicast_on_startup, UINT randomize_wait_on_startup,CHAR *broadcast_domain,  CHAR *multicast_server_address,CHAR *broadcast_time_servers);
    UINT    _nx_sntp_client_initialize_unicast(NX_SNTP_CLIENT *client_ptr, ULONG unicast_server_timeout, ULONG unicast_poll_interval, UINT randomize_wait_on_startup, UINT client_requires_rtt, CHAR *manycast_server_address, CHAR *unicast_time_servers);
    UINT    _nxe_sntp_client_initialize_unicast(NX_SNTP_CLIENT *client_ptr, ULONG unicast_server_timeout, ULONG unicast_poll_interval, UINT randomize_wait_on_startup, UINT client_requires_rtt, CHAR *manycast_server_address, CHAR *unicast_time_servers);
    UINT    _nx_sntp_client_process_time_data(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *server_time_message, UINT first);
    UINT    _nxe_sntp_client_process_time_data(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *server_time_message, UINT first);
    UINT    _nx_sntp_client_receive_time_update(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *time_message, ULONG timeout, UINT incoming_address_must_match);
    UINT    _nxe_sntp_client_receive_time_update(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *time_message, ULONG timeout, UINT incoming_address_must_match);
    UINT    _nx_sntp_client_remove_server_from_list(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, ULONG server_to_remove);
    UINT    _nxe_sntp_client_remove_server_from_list(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, ULONG server_to_remove);
    UINT    _nx_sntp_client_reset_broadcast(NX_SNTP_CLIENT *client_ptr);
    UINT    _nxe_sntp_client_reset_broadcast(NX_SNTP_CLIENT *client_ptr);
    UINT    _nx_sntp_client_reset_unicast(NX_SNTP_CLIENT *client_ptr);
    UINT    _nxe_sntp_client_reset_unicast(NX_SNTP_CLIENT *client_ptr);
    UINT    _nx_sntp_client_run_broadcast(NX_SNTP_CLIENT *client_ptr);
    UINT    _nxe_sntp_client_run_broadcast(NX_SNTP_CLIENT *client_ptr);
    UINT    _nx_sntp_client_run_unicast(NX_SNTP_CLIENT *client_ptr);
    UINT    _nxe_sntp_client_run_unicast(NX_SNTP_CLIENT *client_ptr);
    UINT    _nx_sntp_client_send_unicast_request(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *unicast_request);
    UINT    _nxe_sntp_client_send_unicast_request(NX_SNTP_CLIENT *client_ptr, NX_SNTP_TIME_MESSAGE *unicast_request);
    UINT    _nx_sntp_client_utility_add_msecs_to_NTPtime(NX_SNTP_TIME *timeA_ptr, LONG msecs_to_add);
    UINT    _nxe_sntp_client_utility_add_msecs_to_NTPtime(NX_SNTP_TIME *timeA_ptr, LONG msecs_to_add);
    UINT    _nx_sntp_client_utility_add_NTPtime(NX_SNTP_TIME *timeA_ptr, NX_SNTP_TIME *timeB_ptr, NX_SNTP_TIME *sum_time);
    UINT    _nxe_sntp_client_utility_add_NTPtime(NX_SNTP_TIME *timeA_ptr, NX_SNTP_TIME *timeB_ptr, NX_SNTP_TIME *sum_time);
    UINT    _nx_sntp_client_utility_convert_fraction_to_msecs(ULONG *milliseconds, NX_SNTP_TIME *time_ptr);
    UINT    _nxe_sntp_client_utility_convert_fraction_to_msecs(ULONG *milliseconds, NX_SNTP_TIME *time_ptr);
    UINT    _nx_sntp_client_utility_convert_LONG_to_IP(CHAR *buffer, ULONG IP_UL);
    UINT    _nxe_sntp_client_utility_convert_LONG_to_IP(CHAR *buffer, ULONG IP_UL);
    UINT    _nx_sntp_client_utility_convert_refID_KOD_code(UCHAR *reference_id, UINT *code_id);
    UINT    _nxe_sntp_client_utility_convert_refID_KOD_code(UCHAR *reference_id, UINT *code_id);
    UINT    _nx_sntp_client_utility_convert_seconds_to_date(NX_SNTP_TIME *current_NTP_time_ptr, UINT current_year, NX_SNTP_DATE_TIME *current_date_time_ptr);
    UINT    _nxe_sntp_client_utility_convert_seconds_to_date(NX_SNTP_TIME *current_NTP_time_ptr, UINT current_year, NX_SNTP_DATE_TIME *current_date_time_ptr);
    UINT    _nx_sntp_client_utility_display_NTP_time(NX_SNTP_TIME *time_ptr, CHAR *title);
    UINT    _nxe_sntp_client_utility_display_NTP_time(NX_SNTP_TIME *time_ptr, CHAR *title);
    UINT    _nx_sntp_client_utility_display_date_time(CHAR *buffer, UINT length, NX_SNTP_DATE_TIME *current_date_time_ptr);
    UINT    _nxe_sntp_client_utility_display_date_time(CHAR *buffer, UINT length, NX_SNTP_DATE_TIME *current_date_time_ptr);
    UINT    _nx_sntp_client_utility_get_msec_diff(NX_SNTP_TIME *timeA_ptr, NX_SNTP_TIME *timeB_ptr, LONG *diff_msec);
    UINT    _nxe_sntp_client_utility_get_msec_diff(NX_SNTP_TIME *timeA_ptr, NX_SNTP_TIME *timeB_ptr, LONG *diff_msec);

/* Define internal SNTP Client functions.  */

    UINT    _nx_sntp_client_add_server_ULONG_to_list(NX_SNTP_CLIENT *client_ptr, UINT operating_mode, ULONG server_to_add);
    UINT    _nx_sntp_client_create_time_request_packet(NX_SNTP_CLIENT *client_ptr, NX_PACKET *packet_ptr, NX_SNTP_TIME_MESSAGE *time_message_ptr);
    UINT    _nx_sntp_client_duplicate_update_check(NX_SNTP_TIME_MESSAGE *timeA_msg_ptr, NX_SNTP_TIME_MESSAGE *timeB_msg_ptr, UINT *is_a_dupe);
    UINT    _nx_sntp_client_extract_time_message_from_packet(NX_PACKET *packet_ptr, NX_SNTP_TIME_MESSAGE *time_message_ptr);
    UINT    _nx_sntp_client_process_update_packet(NX_SNTP_CLIENT *client_ptr, UINT first_update_pending, CHAR *server_ip_address, UINT receive_status, UINT *invalid_time_updates, ULONG *current_poll_interval, UINT *result);
    UINT    _nx_sntp_client_reset_current_time_message(NX_SNTP_CLIENT *client_ptr);
    VOID    _nx_sntp_client_update_timeout_entry(ULONG info);
    VOID    _nx_sntp_client_utility_fraction_to_usecs(ULONG tsf, ULONG *usecs); 
    UINT    _nx_sntp_client_utility_addition_overflow_check(ULONG temp1, ULONG temp2);
    UINT    _nx_sntp_client_utility_convert_time_to_UCHAR(NX_SNTP_TIME *time, NX_SNTP_TIME_MESSAGE *time_message_ptr, UINT which_stamp);
    UINT    _nx_sntp_client_utility_convert_UCHAR_to_time(NX_SNTP_TIME *time, NX_SNTP_TIME_MESSAGE *time_message_ptr, UINT which_stamp);
    UINT    _nx_sntp_client_utility_is_zero_data(UCHAR *data, UINT size);
    VOID    _nx_sntp_client_utility_IP_to_ULONG(CHAR *buffer, UINT length, ULONG *IP_UL);
    UINT    _nx_sntp_client_utility_msecs_to_fraction(ULONG msecs, ULONG *fraction);
    VOID    _nx_sntp_client_utility_parse_response(CHAR *buffer, UINT argument_index, UINT buffer_length, CHAR *argument, UINT argument_length, UINT convert_to_uppercase);
    VOID    _nx_sntp_client_utility_valid_ip_address(CHAR *start_buffer, CHAR *end_buffer, UINT *valid_address);
    VOID    _nx_sntp_client_utility_server_in_domain(ULONG domain_mask, ULONG ip_address, UINT *valid_domain_IP); 
    UINT    _nx_sntp_client_utility_usec_to_fraction(ULONG usecs, ULONG *tsf);


#endif   /*  NX_SNTP_SOURCE_CODE */

/* If a C++ compiler is being used....*/
#ifdef   __cplusplus
}
#endif


#endif /* NX_SNTP_CLIENT_H  */
