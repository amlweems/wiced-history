/**************************************************************************/ 
/*                                                                        */ 
/*            Copyright (c) 1996-2006 by Express Logic Inc.               */ 
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
/**   Point-to-Point Protocol (PPP)                                       */ 
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */ 
/*                                                                        */ 
/*    nx_ppp.h                                            PORTABLE C      */ 
/*                                                           5.0          */ 
/*  AUTHOR                                                                */ 
/*                                                                        */ 
/*    William E. Lamie, Express Logic, Inc.                               */ 
/*                                                                        */ 
/*  DESCRIPTION                                                           */ 
/*                                                                        */ 
/*    This file defines the NetX Point-to-Point Protocol (PPP)            */ 
/*    component, including all data types and external references.        */ 
/*    It is assumed that nx_api.h and nx_port.h have already been         */ 
/*    included.                                                           */
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  03-01-2006     William E. Lamie         Initial Version 5.0           */ 
/*                                                                        */ 
/**************************************************************************/ 

#ifndef NX_PPP_H
#define NX_PPP_H

/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */
extern   "C" {

#endif

/* Define the PPP ID.  */

#define NX_PPP_ID                       0x50505020UL


#ifndef NX_PPP_THREAD_TIME_SLICE
#define NX_PPP_THREAD_TIME_SLICE        TX_NO_TIME_SLICE
#endif

#ifndef NX_PPP_MRU
#define NX_PPP_MRU                      1500        /* Minimum value!  */
#endif

#ifndef NX_PPP_MINIMUM_MRU
#define NX_PPP_MINIMUM_MRU              1500
#endif

#ifndef NX_PPP_SERIAL_BUFFER_SIZE
#define NX_PPP_SERIAL_BUFFER_SIZE       NX_PPP_MRU*2
#endif

#ifndef NX_PPP_NAME_SIZE
#define NX_PPP_NAME_SIZE                32
#endif

#ifndef NX_PPP_PASSWORD_SIZE
#define NX_PPP_PASSWORD_SIZE            32
#endif

#ifndef NX_PPP_VALUE_SIZE
#define NX_PPP_VALUE_SIZE               32
#endif

#ifndef NX_PPP_HASHED_VALUE_SIZE
#define NX_PPP_HASHED_VALUE_SIZE        16
#endif

#ifndef NX_PPP_RECEIVE_TIMEOUT
#define NX_PPP_RECEIVE_TIMEOUT          125
#endif

#ifndef NX_PPP_TIMEOUT
#define NX_PPP_TIMEOUT                  NX_IP_PERIODIC_RATE*5       /* 5 seconds.  */
#endif

#ifndef NX_PPP_TIMEOUT_RETRIES
#define NX_PPP_TIMEOUT_RETRIES          4
#endif

#ifndef NX_PPP_DEBUG_LOG_SIZE
#define NX_PPP_DEBUG_LOG_SIZE           50
#endif

#ifndef NX_PPP_DEBUG_FRAME_SIZE         
#define NX_PPP_DEBUG_FRAME_SIZE         50
#endif

#define NX_PPP_PACKET_ALERT_THRESHOLD   NX_PPP_SERIAL_BUFFER_SIZE/4 
#define NX_PPP_OPTION_MESSAGE_LENGTH    50              
#define NX_PPP_RESTART_COUNTER          100
#define NX_PPP_LINK_CHECK_COUNTER       2

/* Define PPP protocol types.  */

#define NX_PPP_LCP_PROTOCOL             0xc021
#define NX_PPP_IPCP_PROTOCOL            0x8021
#define NX_PPP_PAP_PROTOCOL             0xC023
#define NX_PPP_CHAP_PROTOCOL            0xc223
#define NX_PPP_DATA                     0x0021
            

/* Define API return codes.  */

#define NX_PPP_FAILURE                  0xb0
#define NX_PPP_BUFFER_FULL              0xb1
#define NX_PPP_PACKET_END               0xb2
#define NX_PPP_BAD_PACKET               0xb3


/* Define basic PPP events.  */

#define NX_PPP_NOE                      0
#define NX_PPP_ALL                      0xffffffff
#define NX_PPP_LCP_EVENT                (1 << 0)
#define NX_PPP_PAP_EVENT                (1 << 1)
#define NX_PPP_CHAP_EVENT               (1 << 2)
#define NX_PPP_IPCP_EVENT               (1 << 3)



/* Define PPP LCP State Machine events.  */

#define NX_PPP_TX_EVENT                 (1 << 0)    
#define NX_PPP_RX_EVENT                 (1 << 1)    
#define NX_PPP_UP_EVENT                 (1 << 2)
#define NX_PPP_DOWN_EVENT               (1 << 3)
#define NX_PPP_OPEN_EVENT               (1 << 4)
#define NX_PPP_CLOSE_EVENT              (1 << 5) 
#define NX_PPP_TOP_EVENT                (1 << 6)
#define NX_PPP_TOM_EVENT                (1 << 7)
#define NX_PPP_RCR_EVENT                (1 << 8)
#define NX_PPP_RCA_EVENT                (1 << 10)
#define NX_PPP_RCN_EVENT                (1 << 11)
#define NX_PPP_RTR_EVENT                (1 << 12)
#define NX_PPP_RTA_EVENT                (1 << 13)
#define NX_PPP_RUC_EVENT                (1 << 14)
#define NX_PPP_RXJM_EVENT               (1 << 15)
#define NX_PPP_FINISHED_EVENT           (1 << 31) 


/* Define PPP LCP State Machine states.  */

#define NX_PPP_INITIAL                  0
#define NX_PPP_STARTING                 1
#define NX_PPP_CLOSED                   2
#define NX_PPP_STOPPED                  3
#define NX_PPP_CLOSING                  4
#define NX_PPP_STOPPING                 5
#define NX_PPP_RQ_SENT                  6
#define NX_PPP_ACK_RCVD                 7
#define NX_PPP_ACK_SENT                 8
#define NX_PPP_OPENED                   9 


/* Define PPP IPCP State Machine events.  */

#define NX_PPP_IPCP_OPEN_EVENT          (1 << 0) 
#define NX_PPP_IPCP_RCR_EVENT           (1 << 1)
#define NX_PPP_IPCP_RCA_EVENT           (1 << 2)
#define NX_PPP_IPCP_RCN_EVENT           (1 << 3)
#define NX_PPP_IPCP_RTA_EVENT           (1 << 4)
#define NX_PPP_IPCP_TOP_EVENT           (1 << 5)


/* Define PPP IPCP State Machine states.  */

#define NX_PPP_IPCP_INITIAL             0
#define NX_PPP_IPCP_CLOSED              2
#define NX_PPP_IPCP_STOPPED             3
#define NX_PPP_IPCP_RQ_SENT             6
#define NX_PPP_IPCP_ACK_RCVD            7
#define NX_PPP_IPCP_ACK_SENT            8   
#define NX_PPP_IPCP_OPENED              9


/* Define PPP PAP State Machine events.  */

#define NX_PPP_PAP_RQ_EVENT             (1 << 0)
#define NX_PPP_PAP_ACK_EVENT            (1 << 1)
#define NX_PPP_PAP_NAK_EVENT            (1 << 2)
#define NX_PPP_PAP_OPEN_EVENT           (1 << 3)


/* Define PPP PAP State Machine states.  */

#define NX_PPP_PAP_CLOSED               1
#define NX_PPP_PAP_RQ_SENT              2
#define NX_PPP_PAP_RQR_OPENED           3
#define NX_PPP_PAP_OPENED               4


/* Define PPP CHAP State Machine events.  */

#define NX_PPP_CHAP_CHAL_EVENT          (1 << 0)
#define NX_PPP_CHAP_RESP_EVENT          (1 << 1)
#define NX_PPP_CHAP_ACK_EVENT           (1 << 2)
#define NX_PPP_CHAP_NAK_EVENT           (1 << 3)
#define NX_PPP_CHAP_OPEN_EVENT          (1 << 4)


/* Define PPP CHAP State Machine states.  */

#define NX_PPP_CHAP_CLOSED              1
#define NX_PPP_CHAP_CHALLENGE_SENT      2
#define NX_PPP_CHAP_RESPONSE_SENT       3
#define NX_PPP_CHAP_RESPONSE_OPENED     4
#define NX_PPP_CHAP_OPENED              5



#define NX_PPP_IP_LCP_OPENED            (1 << 0)
#define NX_PPP_IP_AP_OPENED             (1 << 1)
#define NX_PPP_IP_AP_AUTHENTICATED      (1 << 2)
#define NX_PPP_IP_IPCP_OPENED           (1 << 3)
#define NX_PPP_IP_LCP_RESTART           (1 << 4)
#define NX_PPP_IP_LCP_CLOSED            (1 << 5)


/* Define optional debug log.  This is used to capture the PPP traffic for debug
   purposes.  */

typedef struct NX_PPP_DEBUG_ENTRY_STRUCT
{

    ULONG           nx_ppp_debug_entry_time_stamp;
    UCHAR           nx_ppp_debug_present_state;
    UCHAR           nx_ppp_debug_authenticated;
    UCHAR           nx_ppp_debug_frame_type;
    ULONG           nx_ppp_debug_packet_length;
    UCHAR           nx_ppp_debug_frame[NX_PPP_DEBUG_FRAME_SIZE];
} NX_PPP_DEBUG_ENTRY;


/* Define the PPP Password Authentication Protocol (PAP) data structure.  This is the most 
   typical authentication protocol for PPP.  */

typedef struct NX_PPP_PAP_STRUCT
{

    USHORT          nx_ppp_pap_protocol_id;
    USHORT          nx_ppp_pap_enabled;
    USHORT          nx_ppp_pap_state; 
    UINT            (*nx_ppp_pap_verify_login)(CHAR *name, CHAR *password);
    UINT            (*nx_ppp_pap_generate_login)(CHAR *name, CHAR *password);
} NX_PPP_PAP;


/* Define the PPP Challenge-Handshake Authentication Protocol (CHAP) data structure.  */

typedef struct NX_PPP_CHAP_STRUCT
{

    USHORT          nx_ppp_chap_protocol_id;
    USHORT          nx_ppp_chap_enabled;
    USHORT          nx_ppp_chap_state; 
    CHAR            nx_ppp_chap_challenger_name[NX_PPP_NAME_SIZE];
    CHAR            nx_ppp_chap_random_value[NX_PPP_VALUE_SIZE];
    UINT            (*nx_ppp_chap_get_challenge_values)(CHAR *rand_value, CHAR *id, CHAR *name);
    UINT            (*nx_ppp_chap_get_responder_values)(CHAR *sys, CHAR *name, CHAR *secret);
    UINT            (*nx_ppp_chap_get_verification_values)(CHAR *sys, CHAR *name, CHAR *secret);
} NX_PPP_CHAP;


/* Define the PPP Internet Protocol Control Protocol (IPCP) data structure.  */

typedef struct NX_PPP_IPCP_STRUCT 
{

    USHORT          nx_ppp_ipcp_protocol_id;
    USHORT          nx_ppp_ipcp_state;
    UCHAR           nx_ppp_ipcp_local_ip[4];
    UCHAR           nx_ppp_ipcp_peer_ip[4];
} NX_PPP_IPCP;


/* Define the main PPP data structure.  */

typedef struct NX_PPP_STRUCT 
{

    ULONG           nx_ppp_id;
    CHAR            *nx_ppp_name;
    NX_IP           *nx_ppp_ip_ptr;
    NX_PACKET_POOL  *nx_ppp_packet_pool_ptr;
    UINT            nx_ppp_present_state;
    UINT            nx_ppp_authenticated;    
    UINT            nx_ppp_generate_authentication_protocol; 
    UINT            nx_ppp_verify_authentication_protocol;
#ifndef NX_PPP_DISABLE_INFO
    ULONG           nx_ppp_frame_timeouts;
    ULONG           nx_ppp_internal_errors;
    ULONG           nx_ppp_frame_crc_errors;
    ULONG           nx_ppp_packet_overflow;
    ULONG           nx_ppp_bytes_received;
    ULONG           nx_ppp_bytes_sent;
    ULONG           nx_ppp_bytes_dropped;
    ULONG           nx_ppp_bytes_invalid;
    ULONG           nx_ppp_lcp_frames_received;
    ULONG           nx_ppp_lcp_frames_sent;
    ULONG           nx_ppp_pap_frames_received;
    ULONG           nx_ppp_pap_frames_sent;
    ULONG           nx_ppp_chap_frames_received;
    ULONG           nx_ppp_chap_frames_sent;
    ULONG           nx_ppp_ipcp_frames_received;
    ULONG           nx_ppp_ipcp_frames_sent;
    ULONG           nx_ppp_ip_frames_received;
    ULONG           nx_ppp_ip_frames_sent;
    ULONG           nx_ppp_frames_dropped;
    ULONG           nx_ppp_invalid_frame_id;
#endif
    UCHAR           nx_ppp_transmit_id;
    UCHAR           nx_ppp_receive_id;
    USHORT          nx_ppp_mru;
    USHORT          nx_ppp_check_time_out;
    void            (*nx_ppp_byte_send)(UCHAR byte);
    void            (*nx_ppp_non_ppp_packet_handler)(NX_PACKET *packet_ptr);
    void            (*nx_ppp_nak_authentication_notify)(void);

    TX_EVENT_FLAGS_GROUP  
                    nx_ppp_ip_event;
    TX_MUTEX        nx_ppp_protection;
    NX_PPP_PAP      nx_ppp_pap;
    NX_PPP_CHAP     nx_ppp_chap;
    NX_PPP_IPCP     nx_ppp_ipcp;

    USHORT          nx_ppp_serial_buffer_alert_threshold;
    TX_SEMAPHORE    nx_ppp_serial_buffer_semaphore;
    UCHAR           nx_ppp_serial_buffer[NX_PPP_SERIAL_BUFFER_SIZE];
    USHORT          nx_ppp_serial_buffer_write_index;
    USHORT          nx_ppp_serial_buffer_read_index;
    USHORT          nx_ppp_serial_buffer_byte_count;

    UINT            nx_ppp_restart_counter;
    TX_THREAD       nx_ppp_receive_thread;
    struct NX_PPP_STRUCT 
                    *nx_ppp_created_next,
                    *nx_ppp_created_previous;

#ifdef NX_PPP_DEBUG_LOG_ENABLE
    NX_PPP_DEBUG_ENTRY
                    nx_ppp_debug_log[NX_PPP_DEBUG_LOG_SIZE];
    UINT            nx_ppp_debug_log_oldest_index;
#endif

} NX_PPP;


#ifndef NX_PPP_SOURCE_CODE

/* Application caller is present, perform API mapping.  */

/* Determine if error checking is desired.  If so, map API functions 
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work. 
   Note: error checking is enabled by default.  */

#ifdef NX_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define nx_ppp_byte_receive                 _nx_ppp_byte_receive
#define nx_ppp_chap_challenge               _nx_ppp_chap_challenge
#define nx_ppp_chap_enable                  _nx_ppp_chap_enable
#define nx_ppp_create                       _nx_ppp_create
#define nx_ppp_delete                       _nx_ppp_delete
#define nx_ppp_driver                       _nx_ppp_driver
#define nx_ppp_ip_address_assign            _nx_ppp_ip_address_assign
#define nx_ppp_nak_authentication_notify    _nx_ppp_nak_authentication_notify
#define nx_ppp_pap_enable                   _nx_ppp_pap_enable
#define nx_ppp_raw_string_send              _nx_ppp_raw_string_send

#else

/* Services with error checking.  */

#define nx_ppp_byte_receive                 _nxe_ppp_byte_receive
#define nx_ppp_chap_challenge               _nxe_ppp_chap_challenge
#define nx_ppp_chap_enable                  _nxe_ppp_chap_enable
#define nx_ppp_create                       _nxe_ppp_create
#define nx_ppp_delete                       _nxe_ppp_delete
#define nx_ppp_driver                       _nx_ppp_driver
#define nx_ppp_ip_address_assign            _nxe_ppp_ip_address_assign
#define nx_ppp_nak_authentication_notify    _nx_ppp_nak_authentication_notify
#define nx_ppp_pap_enable                   _nxe_ppp_pap_enable
#define nx_ppp_raw_string_send              _nxe_ppp_raw_string_send

#endif

/* Define the prototypes accessible to the application software.  */

UINT    nx_ppp_byte_receive(NX_PPP *ppp_ptr, UCHAR byte);
UINT    nx_ppp_chap_challenge(NX_PPP *ppp_ptr);
UINT    nx_ppp_chap_enable(NX_PPP *ppp_ptr, 
            UINT (*get_challenge_values)(CHAR *rand_value, CHAR *id, CHAR *name),
            UINT (*get_responder_values)(CHAR *system, CHAR *name, CHAR *secret),
            UINT (*get_verification_values)(CHAR *system, CHAR *name, CHAR *secret));
UINT    nx_ppp_create(NX_PPP *ppp_ptr, CHAR *name, NX_IP *ip_ptr, 
               VOID *stack_memory_ptr, ULONG stack_size, UINT thread_priority, 
               NX_PACKET_POOL *pool_ptr,
               void (*ppp_non_ppp_packet_handler)(NX_PACKET *packet_ptr),
               void (*ppp_byte_send)(UCHAR byte));
UINT    nx_ppp_delete(NX_PPP *ppp_ptr);
UINT    nx_ppp_ip_address_assign(NX_PPP *ppp_ptr, ULONG local_ip_address, ULONG peer_ip_address);
UINT    nx_ppp_nak_authentication_notify(NX_PPP *ppp_ptr, void (*nak_authentication_notify)(void));
UINT    nx_ppp_pap_enable(NX_PPP *ppp_ptr, UINT (*generate_login)(CHAR *name, CHAR *password),
                        UINT (*verify_login)(CHAR *name, CHAR *password));
void    nx_ppp_driver(NX_IP_DRIVER *driver_req_ptr);
UINT    nx_ppp_raw_string_send(NX_PPP *ppp_ptr, CHAR *string_ptr);

#else

UINT    _nx_ppp_byte_receive(NX_PPP *ppp_ptr, UCHAR byte);
UINT    _nxe_ppp_byte_receive(NX_PPP *ppp_ptr, UCHAR byte);
UINT    _nx_ppp_chap_challenge(NX_PPP *ppp_ptr);
UINT    _nxe_ppp_chap_challenge(NX_PPP *ppp_ptr);
UINT    _nx_ppp_chap_enable(NX_PPP *ppp_ptr, 
            UINT (*get_challenge_values)(CHAR *rand_value, CHAR *id, CHAR *name),
            UINT (*get_responder_values)(CHAR *system, CHAR *name, CHAR *secret),
            UINT (*get_verification_values)(CHAR *system, CHAR *name, CHAR *secret));
UINT    _nxe_ppp_chap_enable(NX_PPP *ppp_ptr, 
            UINT (*get_challenge_values)(CHAR *rand_value, CHAR *id, CHAR *name),
            UINT (*get_responder_values)(CHAR *system, CHAR *name, CHAR *secret),
            UINT (*get_verification_values)(CHAR *system, CHAR *name, CHAR *secret));
UINT    _nx_ppp_create(NX_PPP *ppp_ptr, CHAR *name, NX_IP *ip_ptr, 
               VOID *stack_memory_ptr, ULONG stack_size, UINT thread_priority, 
               NX_PACKET_POOL *pool_ptr,
               void (*ppp_non_ppp_packet_handler)(NX_PACKET *packet_ptr),
               void (*ppp_byte_send)(UCHAR byte));
UINT    _nxe_ppp_create(NX_PPP *ppp_ptr, CHAR *name, NX_IP *ip_ptr, 
               VOID *stack_memory_ptr, ULONG stack_size, UINT thread_priority, 
               NX_PACKET_POOL *pool_ptr,
               void (*ppp_non_ppp_packet_handler)(NX_PACKET *packet_ptr),
               void (*ppp_byte_send)(UCHAR byte));
UINT    _nx_ppp_delete(NX_PPP *ppp_ptr);
UINT    _nxe_ppp_delete(NX_PPP *ppp_ptr);
UINT    _nx_ppp_ip_address_assign(NX_PPP *ppp_ptr, ULONG local_ip_address, ULONG peer_ip_address);
UINT    _nxe_ppp_ip_address_assign(NX_PPP *ppp_ptr, ULONG local_ip_address, ULONG peer_ip_address);
UINT    _nx_ppp_nak_authentication_notify(NX_PPP *ppp_ptr, void (*nak_authentication_notify)(void));
UINT    _nx_ppp_pap_enable(NX_PPP *ppp_ptr, UINT (*generate_login)(CHAR *name, CHAR *password),
                        UINT (*verify_login)(CHAR *name, CHAR *password));
UINT    _nxe_ppp_pap_enable(NX_PPP *ppp_ptr, UINT (*generate_login)(CHAR *name, CHAR *password),
                        UINT (*verify_login)(CHAR *name, CHAR *password));
void    _nx_ppp_driver(NX_IP_DRIVER *driver_req_ptr);
UINT    _nx_ppp_raw_string_send(NX_PPP *ppp_ptr, CHAR *string_ptr);
UINT    _nxe_ppp_raw_string_send(NX_PPP *ppp_ptr, CHAR *string_ptr);

void    _nx_ppp_receive_thread_entry(ULONG ppp_addr);
void    _nx_ppp_receive_packet_get(NX_PPP *ppp_ptr, NX_PACKET **return_packet_ptr);
void    _nx_ppp_receive_packet_process(NX_PPP *ppp_ptr, NX_PACKET *packet_ptr);
void    _nx_ppp_lcp_state_machine_update(NX_PPP *ppp_ptr, NX_PACKET *packet_ptr, ULONG received_event);
void    _nx_ppp_lcp_code_reject(NX_PPP *ppp_ptr, UCHAR *lcp_ptr);
void    _nx_ppp_lcp_configure_reply_send(NX_PPP *ppp_ptr, UINT status, UCHAR *lcp_ptr, UCHAR *naked_list, UCHAR *rejected_list);
void    _nx_ppp_lcp_configure_request_send(NX_PPP *ppp_ptr, UCHAR *negotiate_list);
UINT    _nx_ppp_lcp_configuration_retrieve(NX_PPP *ppp_ptr, NX_PACKET *packet_ptr, UCHAR *naked_list, UCHAR *rejected_list);
void    _nx_ppp_lcp_nak_configure_list(NX_PPP *ppp_ptr, UCHAR *naked_list);
void    _nx_ppp_lcp_terminate_ack_send(NX_PPP *ppp_ptr);
void    _nx_ppp_lcp_terminate_request_send(NX_PPP *ppp_ptr);
void    _nx_ppp_pap_state_machine_update(NX_PPP *ppp_ptr, NX_PACKET *packet_ptr, ULONG received_event);
void    _nx_ppp_chap_state_machine_update(NX_PPP *ppp_ptr, NX_PACKET *packet_ptr, ULONG received_event);
void    _nx_ppp_ipcp_state_machine_update(NX_PPP *ppp_ptr, NX_PACKET *packet_ptr, ULONG received_event);
UINT    _nx_ppp_ipcp_configure_check(NX_PPP *ppp_ptr, NX_PACKET *packet_ptr, UCHAR *naked_list, UCHAR *rejected_list, UCHAR *good_data);
void    _nx_ppp_ipcp_configure_request_send(NX_PPP *ppp_ptr);
void    _nx_ppp_ipcp_response_extract(NX_PPP *ppp_ptr, NX_PACKET *packet_ptr);
void    _nx_ppp_ipcp_response_send(NX_PPP *ppp_ptr, UCHAR type, UCHAR *data, UCHAR length, NX_PACKET *packet_ptr);
void    _nx_ppp_ipcp_terminate_ack_send(NX_PPP *ppp_ptr);
void    _nx_ppp_netx_packet_transfer(NX_PPP *ppp_ptr, NX_PACKET *packet_ptr);
void    _nx_ppp_packet_transmit(NX_PPP *ppp_ptr, NX_PACKET *packet_ptr);
void    _nx_ppp_netx_packet_send(NX_PPP *ppp_ptr, NX_PACKET *packet_ptr);
void    _nx_ppp_time_out(NX_PPP *ppp_ptr);
UINT    _nx_ppp_check_crc(NX_PACKET *packet_ptr);
UINT    _nx_ppp_crc_append(NX_PACKET *packet_ptr, UCHAR crc[2]);
void    _nx_ppp_debug_log_capture(NX_PPP *ppp_ptr, UCHAR packet_type, NX_PACKET *packet_ptr);
void    _nx_ppp_hash_generator(unsigned char *hvalue,  unsigned char id, 
                            unsigned char *secret,  unsigned char *rand_value);

#endif

/* Determine if a C++ compiler is being used.  If so, complete the standard
   C conditional started above.  */
#ifdef   __cplusplus
        }
#endif

#endif  
