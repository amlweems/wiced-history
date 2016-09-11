/**************************************************************************/
/*                                                                        */
/*            Copyright (c) 1996-2011 by Express Logic Inc.               */
/*                                                                        */
/*  This software is copyrighted by and is the sole property of Express   */
/*  Logic, Inc.  All rights, title, ownership, or other interests         */
/*  in the software remain the property of Express Logic, Inc. This       */
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
/** NetX SMTP Client Component                                            */
/**                                                                       */
/**   Simple Mail Transfer Protocol (SMTP)                                */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */
/*                                                                        */
/*    nx_smtp_client.h                                    PORTABLE C      */
/*                                                           5.2          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Janet Christiansen, Express Logic, Inc.                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the NetX Simple Mail Transfer Protocol (SMTP)     */
/*    Client component, including all data types and external references. */
/*    It is assumed that tx_api.h, tx_port.h, nx_api.h, and nx_port.h,    */
/*    have already been included.                                         */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-24-2007     Janet Christiansen         Initial version 5.0         */
/*  04-01-2010     Janet Christiansen         Modified comment(s),        */ 
/*                                              resulting in version 5.1  */
/*  07-11-2011     Janet Christiansen         Modified comment(s),        */
/*                                              resulting in version 5.2  */
/*                                                                        */
/**************************************************************************/

#ifndef NX_SMTP_CLIENT_H
#define NX_SMTP_CLIENT_H


/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */ 
extern   "C" {

#endif

#include "nx_smtp.h"


/* NX SMTP Client configurable options.  */

/* Set the event reporting/debug output level for the NetX SMTP Client */

#ifndef NX_SMTP_CLIENT_DEBUG
#define NX_SMTP_CLIENT_DEBUG                         MODERATE
#endif



/* Scheme for filtering messages during program execution. 

   printf() itself may need to be defined for the specific 
   processor that is running the application and communication
   available (e.g. serial port).  */


#define NX_SMTP_CLIENT_EVENT_LOG(debug_level, msg)                         \
{                                                                          \
    UINT level = (UINT)debug_level;                                        \
    if ((level <= ALL) && (NX_SMTP_CLIENT_DEBUG == ALL))                   \
    {                                                                      \
       printf msg;                                                         \
    }                                                                      \
    else if ((level <= MODERATE) && (NX_SMTP_CLIENT_DEBUG == MODERATE))    \
    {                                                                      \
       printf msg;                                                         \
    }                                                                      \
    else if ((level == SEVERE) && (NX_SMTP_CLIENT_DEBUG == SEVERE))        \
    {                                                                      \
       printf msg;                                                         \
    }                                                                      \
}

/* Configure the Client to let the host application handle memory allocation for
   mail, recipient and mail message data items.  */
#define NX_SMTP_CLIENT_NO_MEM_ALLOC

/* Enable print client session mail summary feature.  */
/* #define NX_SMTP_PRINT_CLIENT_MAIL_DATA  */ 

/* Enable print client packet and memory pool reserves feature.  */
/* #define NX_SMTP_PRINT_CLIENT_RESERVES  */


/* Set the number of client SMTP sessions.  */

#ifndef NX_SMTP_CLIENT_SESSION_COUNT            
#define NX_SMTP_CLIENT_SESSION_COUNT                1
#endif


/* Set Net TCP print summary mutex timeout in ticks.  */

#ifndef NX_SMTP_CLIENT_PRINT_TIMEOUT            
#define NX_SMTP_CLIENT_PRINT_TIMEOUT                (1 * NX_SMTP_TICKS_PER_SECOND)  
#endif


/* Set the NetX SMTP client stack size .  */

#ifndef NX_SMTP_CLIENT_STACK_SIZE
#define NX_SMTP_CLIENT_STACK_SIZE                   4096   
#endif


/* Set the client session thread stack size */

#ifndef NX_SMTP_CLIENT_SESSION_STACK_SIZE              
#define NX_SMTP_CLIENT_SESSION_STACK_SIZE           4096
#endif


/* Set the client thread time slice.  */

#ifndef NX_SMTP_CLIENT_THREAD_TIME_SLICE
#define NX_SMTP_CLIENT_THREAD_TIME_SLICE            TX_NO_TIME_SLICE
#endif


/* Set the client session thread time slice.  */

#ifndef NX_SMTP_CLIENT_SESSION_THREAD_TIME_SLICE
#define NX_SMTP_CLIENT_SESSION_THREAD_TIME_SLICE    TX_NO_TIME_SLICE
#endif


#ifndef NX_SMTP_CLIENT_PRIORITY                 
#define NX_SMTP_CLIENT_PRIORITY                     2
#endif


/* Set the client session thread priority.  */

#ifndef NX_SMTP_CLIENT_SESSION_PRIORITY                
#define NX_SMTP_CLIENT_SESSION_PRIORITY             NX_SMTP_CLIENT_PRIORITY
#endif


/* Set NetX SMTP client thread preemption threshold.  */

#ifndef NX_SMTP_CLIENT_PREEMPTION_THRESHOLD     
#define NX_SMTP_CLIENT_PREEMPTION_THRESHOLD         2
#endif


/* Set the client session thread preemption level.  */

#ifndef NX_SMTP_SESSION_PREEMPTION_THRESHOLD    
#define NX_SMTP_SESSION_PREEMPTION_THRESHOLD        NX_SMTP_CLIENT_PREEMPTION_THRESHOLD
#endif

/* Configure client memory resources.  */

/* Set Client byte pool name.  */

#ifndef NX_SMTP_CLIENT_BYTE_POOL_NAME 
#define NX_SMTP_CLIENT_BYTE_POOL_NAME               "SMTP Client bytepool"
#endif

/* Set Client byte pool size.  */

#ifndef NX_SMTP_CLIENT_BYTE_POOL_SIZE           
#define NX_SMTP_CLIENT_BYTE_POOL_SIZE               2048 
#endif

/* Set Client byte pool mutex name.  */

#ifndef NX_SMTP_CLIENT_BYTE_POOL_MUTEX_NAME
#define NX_SMTP_CLIENT_BYTE_POOL_MUTEX_NAME         "SMTP Client bytepool mutex" 
#endif

/* Set timeout to obtain Client byte pool mutex.  */

#ifndef NX_SMTP_CLIENT_BYTE_POOL_MUTEX_WAIT
#define NX_SMTP_CLIENT_BYTE_POOL_MUTEX_WAIT         (5 * NX_SMTP_TICKS_PER_SECOND) 
#endif


/* Set Client block pool name.  */

#ifndef NX_SMTP_CLIENT_BLOCK_POOL_NAME 
#define NX_SMTP_CLIENT_BLOCK_POOL_NAME              "SMTP Client blockpool"
#endif


/* Set Client block pool block size. Best if block size is close to the device MTU.  */

#ifndef NX_SMTP_CLIENT_BLOCK_SIZE               
#define NX_SMTP_CLIENT_BLOCK_SIZE                   NX_SMTP_CLIENT_PACKET_SIZE
#endif


/* Set Client block pool size.  */

#ifndef NX_SMTP_CLIENT_BLOCK_POOL_SIZE          
#define NX_SMTP_CLIENT_BLOCK_POOL_SIZE              (16 * NX_SMTP_CLIENT_BLOCK_SIZE)
#endif


/* Set Client block pool mutex name.  */

#ifndef NX_SMTP_CLIENT_BLOCK_POOL_MUTEX_NAME  
#define NX_SMTP_CLIENT_BLOCK_POOL_MUTEX_NAME        "SMTP Client blockpool mutex" 
#endif


/* Set timeout to obtain Client block pool mutex.  */

#ifndef NX_SMTP_CLIENT_BLOCK_POOL_MUTEX_WAIT
#define NX_SMTP_CLIENT_BLOCK_POOL_MUTEX_WAIT        (5 * NX_SMTP_TICKS_PER_SECOND) 
#endif


/* Configure client IP.  */

/* Set the default NetX SMTP client domain name.  */

#ifndef NX_SMTP_CLIENT_DOMAIN                       
#define NX_SMTP_CLIENT_DOMAIN                       "expresslogic.com"
#endif


/* Set NetX IP helper thread stack size.  */

#ifndef NX_SMTP_CLIENT_IP_STACK_SIZE
#define NX_SMTP_CLIENT_IP_STACK_SIZE                2048 
#endif


/* Set NetX IP helper thread priority.  */

#ifndef NX_SMTP_CLIENT_IP_THREAD_PRIORITY              
#define NX_SMTP_CLIENT_IP_THREAD_PRIORITY           2 
#endif


/* Configure the NetX SMTP client network parameters */

/* Set port for client to connect to SMTP server.  */

#ifndef NX_SMTP_CLIENT_SESSION_PORT                       
#define NX_SMTP_CLIENT_SESSION_PORT                 25     
#endif


/* Set size of NetX SMTP client packet. Best if close to the device MTU.  */

#ifndef NX_SMTP_CLIENT_PACKET_SIZE
#define NX_SMTP_CLIENT_PACKET_SIZE                  1500
#endif


/* Set size of header data from network Frame, IP, TCP and NetX in bytes.  */

#ifndef NX_SMTP_CLIENT_PACKET_HEADER_SIZE       
#define NX_SMTP_CLIENT_PACKET_HEADER_SIZE           60
#endif    


/* Set size of NetX SMTP Client packet pool.  */

#ifndef NX_SMTP_CLIENT_PACKET_POOL_SIZE     
#define NX_SMTP_CLIENT_PACKET_POOL_SIZE             (10 * NX_SMTP_CLIENT_PACKET_SIZE)     
#endif


/* Set Client TCP socket name.  */

#ifndef NX_SMTP_CLIENT_TCP_SOCKET_NAME     
#define NX_SMTP_CLIENT_TCP_SOCKET_NAME              "SMTP Client socket"    
#endif


/* Set ARP cache memory size.  */

#ifndef NX_SMTP_CLIENT_ARP_CACHE_SIZE 
#define NX_SMTP_CLIENT_ARP_CACHE_SIZE               1040 
#endif


/* Set Client TCP window size (maximum number of bytes in socket receive queue). 
   Best if close to the device MTU.*/

#ifndef NX_SMTP_CLIENT_WINDOW_SIZE    
#define NX_SMTP_CLIENT_WINDOW_SIZE                  NX_SMTP_CLIENT_PACKET_SIZE
#endif


/* Set timeout on Client packet allocation in ticks.  */

#ifndef NX_SMTP_PACKET_TIMEOUT
#define NX_SMTP_PACKET_TIMEOUT                      (2 * NX_SMTP_TICKS_PER_SECOND)    
#endif


/* Set timeout for TCP socket send completion.  */

#ifndef NX_SMTP_TCP_SOCKET_SEND_WAIT     
#define NX_SMTP_TCP_SOCKET_SEND_WAIT                (2 * NX_SMTP_TICKS_PER_SECOND)
#endif


/* Set Client TCP connection timeout in ticks.  */

#ifndef NX_SMTP_CLIENT_CONNECTION_TIMEOUT 
#define NX_SMTP_CLIENT_CONNECTION_TIMEOUT           (5 * NX_SMTP_TICKS_PER_SECOND)
#endif


/* Set Client TCP disconnect timeout in ticks.  */

#ifndef NX_SMTP_DISCONNECT_TIMEOUT 
#define NX_SMTP_DISCONNECT_TIMEOUT                  (5 * NX_SMTP_TICKS_PER_SECOND)
#endif


/* Configure SMTP session parameters.  */

/* Set the maximum number of bytes for RFC mandated 'data transparency' processing to add to message data. 
   To disable data transparency processing of message data e.g. if a previous mail user agent/browser
   has already done this, set this to zero.  */

#ifndef NX_SMTP_DATA_TRANSPARENCY_BYTES
#define NX_SMTP_DATA_TRANSPARENCY_BYTES             5
#endif


/* Set maximum number of times a client session can request a reset.  */

#ifndef NX_SMTP_CLIENT_SESSION_RESETS      
#define NX_SMTP_CLIENT_SESSION_RESETS               2      
#endif


/* Set maximum number of times a client session can retry the same command.  */

#ifndef NX_SMTP_CLIENT_SESSION_COMMAND_RETRIES       
#define NX_SMTP_CLIENT_SESSION_COMMAND_RETRIES      2       
#endif


/* Set maximum size of Client unique mail message ID.
   This assumes a format of timestamp @ domain.  */

#ifndef NX_SMTP_CLIENT_MESSAGE_ID_SIZE 
#define NX_SMTP_CLIENT_MESSAGE_ID_SIZE              (NX_SMTP_DATE_AND_TIME_STAMP_SIZE + 1 + NX_SMTP_MAX_USERNAME)
#endif


/* Set the Client list of support authentication types.  */

#ifndef  NX_SMTP_CLIENT_AUTHENTICATION_LIST    
#define  NX_SMTP_CLIENT_AUTHENTICATION_LIST         "LOGIN"     
#endif


/* Set the maximum size for a server reply (not including mail message data).  */

#ifndef NX_SMTP_REPLY_BUFFER_SIZE
#define NX_SMTP_REPLY_BUFFER_SIZE                   512  
#endif


/* Set Client timeout waiting for server reply to client greeting.  */

#ifndef NX_SMTP_GREETING_TIMEOUT_TICKS
#define NX_SMTP_GREETING_TIMEOUT_TICKS              (10 * NX_SMTP_TICKS_PER_SECOND)
#endif


/* Set Client 'envelope' timeout waiting for server reply to client commands.  */

#ifndef NX_SMTP_ENVELOPE_TIMEOUT_TICKS
#define NX_SMTP_ENVELOPE_TIMEOUT_TICKS              (10 * NX_SMTP_TICKS_PER_SECOND)
#endif


/* Set Client timeout waiting to receive server acceptance of client message data.  */

#ifndef NX_SMTP_MESSAGE_TIMEOUT_TICKS
#define NX_SMTP_MESSAGE_TIMEOUT_TICKS               (30 * NX_SMTP_TICKS_PER_SECOND) 
#endif


/* These define the state of the protocol state machine */

#define   NX_SMTP_CLIENT_SESSION_STATE_AWAITING_REPLY       -1     /* Session state depends on outcome of current response handler.  */
#define   NX_SMTP_CLIENT_SESSION_STATE_COMPLETED_NORMALLY   -2     /* No internal errors, session completed normally.  */
#define   NX_SMTP_CLIENT_SESSION_STATE_ERROR                -3     /* Internal errors e.g. TCP send or receive fails; session terminated abnormally.  */


/* Enumerated states of the protocol state machine. These MUST be in the 
   same order as the list of protocol states  in NX_SMTP_CLIENT_SESSION_STATES.  */

    typedef enum NX_SMTP_CLIENT_SESSION_STATE_ENUM
    {
        NX_SMTP_CLIENT_SESSION_STATE_GREETING,      
        NX_SMTP_CLIENT_SESSION_STATE_EHLO,
        NX_SMTP_CLIENT_SESSION_STATE_HELO,
        NX_SMTP_CLIENT_SESSION_STATE_MAIL,
        NX_SMTP_CLIENT_SESSION_STATE_RCPT,
        NX_SMTP_CLIENT_SESSION_STATE_DATA,
        NX_SMTP_CLIENT_SESSION_STATE_MESSAGE,
        NX_SMTP_CLIENT_SESSION_STATE_RSET,
        NX_SMTP_CLIENT_SESSION_STATE_QUIT,
        NX_SMTP_CLIENT_SESSION_STATE_NOOP,
        NX_SMTP_CLIENT_SESSION_STATE_AUTH,
        NX_SMTP_CLIENT_SESSION_STATE_AUTH_CHALLENGE

    } NX_SMTP_CLIENT_SESSION_STATE;


/* Define the NetX SMTP RECIPIENT structure */

    typedef struct NX_SMTP_CLIENT_RECIPIENT_STRUCT
    {
        CHAR                                    *recipient_mailbox;         /* Recipient's mailbox address */
        struct NX_SMTP_CLIENT_RECIPIENT_STRUCT  *next_ptr;                  /* Next recipient in the mail item's list of recipients.  */
        CHAR                                    *recipient_name;            /* Alternate recipient identifier e.g username */
        UINT                                    recipient_type;             /* Type of recipient e.g TO, CC or BCC.  */
        UINT                                    recipient_accepted;         /* Status of server accepting recipient address */

    } NX_SMTP_CLIENT_RECIPIENT;

/* Define the NetX SMTP MAIL structure */

    typedef struct NX_SMTP_CLIENT_MAIL_STRUCT
    {
        CHAR                                    *reverse_path_mailbox_ptr;   /* Sender's mailbox address.  */
        CHAR                                    *message_id;                 /* Globally unique identifier for mail item */
        UINT                                    priority;                    /* Mail item priority level */
        UINT                                    valid_recipients;            /* Number of valid recipients in this mail item.  */
        struct NX_SMTP_CLIENT_SESSION_STRUCT    *session_ptr;                /* Pointer to the session to which mail item belongs.  */
        struct NX_SMTP_CLIENT_MAIL_STRUCT       *next_ptr;                   /* Pointer to the next mail in the client session.  */
        struct NX_SMTP_CLIENT_RECIPIENT_STRUCT  *start_recipient_ptr;        /* Pointer to the first recipient in mail item list.  */
        struct NX_SMTP_CLIENT_RECIPIENT_STRUCT  *current_recipient_ptr;      /* Pointer to the current recipient in a mail item.*/
        struct NX_SMTP_CLIENT_RECIPIENT_STRUCT  *end_recipient_ptr;          /* Pointer to the last recipient in mail item list.  */    


/* Used for SMTP Clients saving mail message text in Client block pools and message structs.  */
        NX_SMTP_MESSAGE_SEGMENT                 *start_message_segment_ptr;  /* Pointer to first segment of mail message.  */
        NX_SMTP_MESSAGE_SEGMENT                 *current_message_segment_ptr;/* Pointer to current segment of mail message.  */
        NX_SMTP_MESSAGE_SEGMENT                 *end_message_segment_ptr;    /* Pointer to last segment of mail message.  */
        ULONG                                   message_length;              /* Total mail message size summed over all message segments.  */
        
/* Used for SMTP Clients NOT saving mail message text in Client block pools and message structs.  */
        CHAR                                    *mail_buffer_ptr;            /* Pointer to text of mail to send.  */
        UINT                                    mail_buffer_length;          /* Size of mail buffer.  */

        UINT                                    mail_sent;                   /* Status of mail acceptance by the server */
    } NX_SMTP_CLIENT_MAIL;

/* Define the NetX SMTP SESSION structure  */

    typedef struct NX_SMTP_CLIENT_SESSION_STRUCT
    {
        ULONG                                   session_id;                   /* Unique session identifier.  */
        UINT                                    server_ip_address;            /* Server IP address.  */
        USHORT                                  server_port;                  /* Server port.  */
        NX_TCP_SOCKET                           tcp_socket;                   /* Client NetX TCP socket.  */
        TX_THREAD                               session_thread;               /* SMTP Client session thread */
        UINT                                    available;                    /* Status on session availability for processing an SMTP transaction.  */
        struct NX_SMTP_CLIENT_STRUCT            *client_ptr;                  /* Pointer to Client this session belongs to.  */    
        struct NX_SMTP_CLIENT_SESSION_STRUCT    *next_ptr;                    /* Pointer to next session in client session list.  */
        struct NX_SMTP_CLIENT_MAIL_STRUCT       *start_mail_ptr;              /* Pointer to the first mail in session mail list.  */
        struct NX_SMTP_CLIENT_MAIL_STRUCT       *end_mail_ptr;                /* Pointer to the last mail in session mail list.  */
        struct NX_SMTP_CLIENT_MAIL_STRUCT       *current_mail_ptr;            /* Pointer to the current mail being processed in the session.  */
        UINT                                    default_reply_handler;        /* Status if using default SMTP Client API handlers for server replies.  */
        INT                                     cmd_state;                    /* Command state of the SMTP session.  */
        INT                                     rsp_state;                    /* Response state of the SMTP session.  */
        UINT                                    reply_code_status;            /* Reply code received from SMTP server.  */
        CHAR                                    reply_buffer[NX_SMTP_REPLY_BUFFER_SIZE];
        /* Text of reply received from SMTP server.  */
        UINT                                    server_authentication_service;/* Status if server has an authentication service */
        UINT                                    client_authentication_index;  /* Index into client list of supported authentication types */
        NX_SMTP_REPLY_TO_AUTH_PROMPT            authentication_reply;         /* Buffer holding server reply text during authentication process */
        NX_SMTP_SESSION_AUTHENTICATION_STATE    authentication_state;         /* State of the authentication process */
        CHAR                                    accepted_authentication[NX_SMTP_SESSION_AUTH_TYPE];
        /* Authentication type for the current session */
        UINT                                    session_command_retries;      /* Number of consecutive times a command may be retried.  */
        UINT                                    session_resets;               /* Number of times a client session may request a reset.  */
        UINT                                    data_transparency_bytes;      /* Extra bytes allowed for data transparency processing to add to message data.  */

    } NX_SMTP_CLIENT_SESSION;



/* Define the SMTP client structure  */

    typedef struct NX_SMTP_CLIENT_STRUCT
    {
        ULONG                           nx_smtp_client_id;                       /* SMTP ID for identify client service.  */
        CHAR                            client_name[NX_SMTP_MAX_USERNAME];       /* Client name (may be used in authentication) */
        CHAR                            client_password[NX_SMTP_MAX_PASSWORD];   /* Client password (used in authentication) */
        CHAR                            client_domain[NX_SMTP_MAX_USERNAME];     /* Client domain of the client (and sender) */
        CHAR                            authentication_list[50];                 /* Client's space-separated list of authentication types */
        NX_SMTP_CLIENT_SESSION                                                   /* SMTP client session array */ 
        nx_smtp_client_session_list[NX_SMTP_CLIENT_SESSION_COUNT]; 
        NX_IP                           *ip_ptr;                                 /* Client IP instance  */
        NX_PACKET_POOL                  *packet_pool_ptr;                        /* Client packet pool for sending data packets to the server */
        TX_BYTE_POOL                    *bytepool_ptr;                           /* Pointer to client byte pool.  */
        TX_MUTEX                        *bytepool_mutex_ptr;                     /* Pointer to client byte pool mutex.  */
        UINT                            bytepool_mutex_timeout;                  /* Timeout value for byte pool mutex.  */
        TX_BLOCK_POOL                   *blockpool_ptr;                          /* Pointer to client block pool.  */
        TX_MUTEX                        *blockpool_mutex_ptr;                    /* Pointer to client block pool mutex.  */
        UINT                            blockpool_mutex_timeout;                 /* Timeout value for block pool mutex.  */
        ULONG                           greeting_timeout;                        /* Timeout in ticks to wait for server reply to greeting */
        ULONG                           envelope_timeout;                        /* Timeout in ticks to wait for server reply to client commands */
        ULONG                           message_timeout;                         /* Timeout in ticks to wait for server reply to receiving message data.*/    
        ULONG                           tcp_window_size;                         /* TCP window size                                            */
        UINT                            (*client_date_stamp)(CHAR *date_stamp);  /* Callback function to generate a time stamp for a mail item being sent.  */
        UINT                            (*client_create_unique_message_id)(NX_SMTP_CLIENT_MAIL *mail_ptr);
        /* Callback function to generate unique id for each mail */
        TX_MUTEX                        print_summary_mutex;                     /* Mutex lock for printing a client session mail transaction summary.  */
        UINT                            print_summary_mutex_ptr_timeout;         /* Timeout value for obtaining the print summary mutex.  */

    } NX_SMTP_CLIENT;


    typedef struct NX_SMTP_CLIENT_SESSION_STATES_STRUCT
    {
        UINT    (*cmd) (NX_SMTP_CLIENT_SESSION *session_ptr);
        UINT    (*rsp) (NX_SMTP_CLIENT_SESSION *session_ptr);

    } NX_SMTP_CLIENT_SESSION_STATES;




#ifndef     NX_SMTP_SOURCE_CODE     

/* Define the system API mappings based on the error checking 
   selected by the user.   */

/* Determine if error checking is desired.  If so, map API functions
   to the appropriate error checking front-ends.  Otherwise, map API
   functions to the core functions that actually perform the work.
   Note: error checking is enabled by default.  */


#ifdef NX_SMTP_DISABLE_ERROR_CHECKING

/* Services without error checking.  */

#define   nx_smtp_client_create                     _nx_smtp_client_create
#define   nx_smtp_client_delete                     _nx_smtp_client_delete
#define   nx_smtp_session_initialize                _nx_smtp_session_initialize
#define   nx_smtp_session_reinitialize              _nx_smtp_session_reinitialize
#define   nx_smtp_session_delete                    _nx_smtp_session_delete
#define   nx_smtp_mail_create                       _nx_smtp_mail_create
#define   nx_smtp_mail_create_memalloc              _nx_smtp_mail_create_memalloc
#define   nx_smtp_mail_delete                       _nx_smtp_mail_delete
#define   nx_smtp_mail_add                          _nx_smtp_mail_add
#define   nx_smtp_mail_message_append               _nx_smtp_mail_message_append
#define   nx_smtp_mail_message_process              _nx_smtp_mail_message_process
#define   nx_smtp_recipient_create                  _nx_smtp_recipient_create
#define   nx_smtp_recipient_create_memalloc         _nx_smtp_recipient_create_memalloc
#define   nx_smtp_recipient_add                     _nx_smtp_recipient_add
#define   nx_smtp_recipient_delete                  _nx_smtp_recipient_delete
#define   nx_smtp_session_run                       _nx_smtp_session_run
#define   nx_smtp_cmd_greeting                      _nx_smtp_cmd_greeting
#define   nx_smtp_rsp_greeting                      _nx_smtp_rsp_greeting
#define   nx_smtp_cmd_ehlo                          _nx_smtp_cmd_ehlo
#define   nx_smtp_rsp_ehlo                          _nx_smtp_rsp_ehlo
#define   nx_smtp_cmd_helo                          _nx_smtp_cmd_helo
#define   nx_smtp_rsp_helo                          _nx_smtp_rsp_helo
#define   nx_smtp_rsp_hello_command                 _nx_smtp_rsp_hello_command  
#define   nx_smtp_cmd_mail                          _nx_smtp_cmd_mail
#define   nx_smtp_rsp_mail                          _nx_smtp_rsp_mail
#define   nx_smtp_cmd_rcpt                          _nx_smtp_cmd_rcpt
#define   nx_smtp_rsp_rcpt                          _nx_smtp_rsp_rcpt
#define   nx_smtp_cmd_data                          _nx_smtp_cmd_data
#define   nx_smtp_rsp_data                          _nx_smtp_rsp_data
#define   nx_smtp_cmd_message                       _nx_smtp_cmd_message
#define   nx_smtp_cmd_message_memalloc              _nx_smtp_cmd_message_memalloc
#define   nx_smtp_rsp_message                       _nx_smtp_rsp_message
#define   nx_smtp_cmd_rset                          _nx_smtp_cmd_rset
#define   nx_smtp_rsp_rset                          _nx_smtp_rsp_rset
#define   nx_smtp_cmd_quit                          _nx_smtp_cmd_quit
#define   nx_smtp_rsp_quit                          _nx_smtp_rsp_quit
#define   nx_smtp_cmd_noop                          _nx_smtp_cmd_noop
#define   nx_smtp_rsp_noop                          _nx_smtp_rsp_noop   
#define   nx_smtp_cmd_auth                          _nx_smtp_cmd_auth   
#define   nx_smtp_rsp_auth                          _nx_smtp_rsp_auth   
#define   nx_smtp_cmd_auth_challenge                _nx_smtp_cmd_auth_challenge
#define   nx_smtp_rsp_auth_challenge                _nx_smtp_rsp_auth_challenge
#define   nx_smtp_utility_read_server_code          _nx_smtp_utility_read_server_code
#define   nx_smtp_utility_send_to_server            _nx_smtp_utility_send_to_server
#define   nx_smtp_utility_data_transparency         _nx_smtp_utility_data_transparency
#define   nx_smtp_utility_parse_server_services     _nx_smtp_utility_parse_server_services
#define   nx_smtp_utility_authentication_challenge  _nx_smtp_utility_authentication_challenge
#define   nx_smtp_utility_print_client_mail_status  _nx_smtp_utility_print_client_mail_status
#define   nx_smtp_utility_print_client_reserves     _nx_smtp_utility_print_client_reserves
#define   nx_smtp_bytepool_memory_get               _nx_smtp_bytepool_memory_get
#define   nx_smtp_bytepool_memory_release           _nx_smtp_bytepool_memory_release 
#define   nx_smtp_blockpool_memory_get              _nx_smtp_blockpool_memory_get
#define   nx_smtp_blockpool_memory_release          _nx_smtp_blockpool_memory_release 

#else

/* Services with error checking.  */

#define nx_smtp_client_create                    _nxe_smtp_client_create
#define nx_smtp_client_delete                    _nxe_smtp_client_delete
#define nx_smtp_session_initialize               _nxe_smtp_session_initialize
#define nx_smtp_session_reinitialize             _nxe_smtp_session_reinitialize
#define nx_smtp_session_delete                   _nxe_smtp_session_delete
#define nx_smtp_mail_create                      _nxe_smtp_mail_create
#define nx_smtp_mail_create_memalloc             _nxe_smtp_mail_create_memalloc
#define nx_smtp_mail_delete                      _nxe_smtp_mail_delete
#define nx_smtp_mail_add                         _nxe_smtp_mail_add
#define nx_smtp_mail_message_append              _nxe_smtp_mail_message_append
#define nx_smtp_mail_message_process             _nxe_smtp_mail_message_process
#define nx_smtp_recipient_create                 _nxe_smtp_recipient_create
#define nx_smtp_recipient_create_memalloc        _nxe_smtp_recipient_create_memalloc
#define nx_smtp_recipient_delete                 _nxe_smtp_recipient_delete
#define nx_smtp_recipient_add                    _nxe_smtp_recipient_add
#define nx_smtp_session_run                      _nxe_smtp_session_run
#define nx_smtp_cmd_greeting                     _nxe_smtp_cmd_greeting
#define nx_smtp_rsp_greeting                     _nxe_smtp_rsp_greeting
#define nx_smtp_cmd_ehlo                         _nxe_smtp_cmd_ehlo
#define nx_smtp_rsp_ehlo                         _nxe_smtp_rsp_ehlo
#define nx_smtp_cmd_helo                         _nxe_smtp_cmd_helo
#define nx_smtp_rsp_helo                         _nxe_smtp_rsp_helo
#define nx_smtp_rsp_hello_command                _nxe_smtp_rsp_hello_command
#define nx_smtp_cmd_mail                         _nxe_smtp_cmd_mail
#define nx_smtp_rsp_mail                         _nxe_smtp_rsp_mail
#define nx_smtp_cmd_rcpt                         _nxe_smtp_cmd_rcpt
#define nx_smtp_rsp_rcpt                         _nxe_smtp_rsp_rcpt
#define nx_smtp_cmd_data                         _nxe_smtp_cmd_data
#define nx_smtp_rsp_data                         _nxe_smtp_rsp_data
#define nx_smtp_cmd_message                      _nxe_smtp_cmd_message
#define nx_smtp_cmd_message_memalloc             _nxe_smtp_cmd_message_memalloc
#define nx_smtp_rsp_message                      _nxe_smtp_rsp_message
#define nx_smtp_cmd_rset                         _nxe_smtp_cmd_rset
#define nx_smtp_rsp_rset                         _nxe_smtp_rsp_rset
#define nx_smtp_cmd_quit                         _nxe_smtp_cmd_quit
#define nx_smtp_rsp_quit                         _nxe_smtp_rsp_quit
#define nx_smtp_cmd_noop                         _nxe_smtp_cmd_noop
#define nx_smtp_rsp_noop                         _nxe_smtp_rsp_noop
#define nx_smtp_cmd_auth                         _nxe_smtp_cmd_auth
#define nx_smtp_rsp_auth                         _nxe_smtp_rsp_auth
#define nx_smtp_cmd_auth_challenge               _nxe_smtp_cmd_auth_challenge
#define nx_smtp_rsp_auth_challenge               _nxe_smtp_rsp_auth_challenge
#define nx_smtp_utility_read_server_code         _nxe_smtp_utility_read_server_code
#define nx_smtp_utility_send_to_server           _nxe_smtp_utility_send_to_server
#define nx_smtp_utility_data_transparency        _nxe_smtp_utility_data_transparency
#define nx_smtp_utility_parse_server_services    _nxe_smtp_utility_parse_server_services
#define nx_smtp_utility_authentication_challenge _nxe_smtp_utility_authentication_challenge
#define nx_smtp_utility_print_client_mail_status _nxe_smtp_utility_print_client_mail_status
#define nx_smtp_utility_print_client_reserves    _nxe_smtp_utility_print_client_reserves
#define nx_smtp_bytepool_memory_get              _nxe_smtp_bytepool_memory_get
#define nx_smtp_bytepool_memory_release          _nxe_smtp_bytepool_memory_release
#define nx_smtp_blockpool_memory_get             _nxe_smtp_blockpool_memory_get
#define nx_smtp_blockpool_memory_release         _nxe_smtp_blockpool_memory_release


#endif /* NX_SMTP_DISABLE_ERROR_CHECKING */


/* Define the prototypes accessible to the application software.  */

    UINT    nx_smtp_client_create(NX_SMTP_CLIENT *client_ptr, CHAR *client_name, CHAR *client_password, CHAR *client_domain, CHAR *authentication_list, 
                                   UINT (*client_date_stamp)(CHAR *date_stamp),UINT (*client_create_unique_message_id)(NX_SMTP_CLIENT_MAIL *mail_ptr),
                                   NX_IP *ip_ptr, NX_PACKET_POOL *packet_pool_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, UINT bytepool_mutex_timeout,
                                   TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *blockpool_mutex_ptr, UINT blockpool_mutex_timeout, ULONG greeting_timeout, ULONG envelope_timeout,
                                   ULONG message_timeout, ULONG window_size);
    UINT    nx_smtp_client_delete (NX_SMTP_CLIENT *client_ptr);
    UINT    nx_smtp_session_initialize(NX_SMTP_CLIENT_SESSION *this_session, ULONG session_id, NX_SMTP_CLIENT *client_ptr, ULONG ip_addr, USHORT port);
    UINT    nx_smtp_session_delete(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_session_reinitialize(NX_SMTP_CLIENT_SESSION *session_ptr, UINT session_availability);
    UINT    nx_smtp_mail_create(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr, CHAR *reverse_path_ptr, UINT priority, CHAR *mail_buffer, UINT buffer_length);
    UINT    nx_smtp_mail_create_memalloc(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL **mail_ptr, CHAR *reverse_path_ptr, UINT priority);
    UINT    nx_smtp_mail_add(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr);
    UINT    nx_smtp_mail_delete(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr);
    UINT    nx_smtp_recipient_create(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr, NX_SMTP_CLIENT_RECIPIENT *recipient_ptr, CHAR *recipient_name, CHAR *recipient_mailbox, UINT type);
    UINT    nx_smtp_recipient_create_memalloc(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr, NX_SMTP_CLIENT_RECIPIENT **recipient_ptr, CHAR *recipient_name, CHAR *recipient_mailbox, UINT type);
    UINT    nx_smtp_recipient_add(NX_SMTP_CLIENT_MAIL *mail_ptr, NX_SMTP_CLIENT_RECIPIENT *recipient_ptr);
    UINT    nx_smtp_recipient_delete(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_RECIPIENT  *recipient_ptr);
    UINT    nx_smtp_mail_message_append(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr, CHAR *message_in_mail, ULONG message_length);
    UINT    nx_smtp_mail_message_process(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr);
    UINT    nx_smtp_session_run (NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_cmd_greeting(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_rsp_greeting(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_cmd_ehlo(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_rsp_ehlo(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_cmd_helo(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_rsp_helo(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_cmd_mail(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_rsp_mail(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_cmd_rcpt(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_rsp_rcpt(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_cmd_data(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_rsp_data(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_cmd_message(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_cmd_message_memalloc(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_rsp_message(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_cmd_rset(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_rsp_rset(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_cmd_quit(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_rsp_quit(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_cmd_noop(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_rsp_noop(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_cmd_auth(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_rsp_auth(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_cmd_auth_challenge(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_rsp_auth_challenge(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_rsp_hello_command(NX_SMTP_CLIENT_SESSION* session_ptr);
    UINT    nx_smtp_utility_read_server_code(NX_SMTP_CLIENT_SESSION *session_ptr, ULONG timeout, UINT  receive_all_lines);
    UINT    nx_smtp_utility_send_to_server(NX_SMTP_CLIENT_SESSION *session_ptr, CHAR *buffer_ptr, UINT buffer_length, ULONG timeout);
    UINT    nx_smtp_utility_data_transparency(CHAR *buffer, UINT *length, UINT additional_buffer);
    UINT    nx_smtp_utility_parse_server_services(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    nx_smtp_utility_authentication_challenge(NX_SMTP_CLIENT_SESSION *session_ptr, CHAR *buffer_ptr, UINT length);
    UINT    nx_smtp_utility_print_client_mail_status(NX_SMTP_CLIENT_MAIL *mail_ptr);
    UINT    nx_smtp_utility_print_client_reserves(NX_SMTP_CLIENT *client_ptr);
    UINT    nx_smtp_bytepool_memory_get(VOID **memory_ptr, ULONG memory_size, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
    UINT    nx_smtp_bytepool_memory_release(VOID *memory_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
    UINT    nx_smtp_blockpool_memory_get(VOID **memory_ptr, TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
    UINT    nx_smtp_blockpool_memory_release(VOID *memory_ptr, TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);

    


#else  /*  NX_SMTP_SOURCE_CODE */


/* SMTP source code is being compiled, do not perform any API mapping.  */

    UINT    _nx_smtp_client_create(NX_SMTP_CLIENT *client_ptr, CHAR *client_name, CHAR *client_password, CHAR *client_domain, CHAR *authentication_list, 
                                   UINT (*client_date_stamp)(CHAR *date_stamp),UINT (*client_create_unique_message_id)(NX_SMTP_CLIENT_MAIL *mail_ptr),
                                   NX_IP *ip_ptr, NX_PACKET_POOL *packet_pool_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, UINT bytepool_mutex_timeout,
                                   TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *blockpool_mutex_ptr, UINT blockpool_mutex_timeout, ULONG greeting_timeout, ULONG envelope_timeout,
                                   ULONG message_timeout, ULONG window_size);
    UINT    _nxe_smtp_client_create(NX_SMTP_CLIENT *client_ptr, CHAR *client_name, CHAR *client_password, CHAR *client_domain, CHAR *authentication_list, 
                                    UINT (*client_date_stamp)(CHAR *date_stamp),UINT (*client_create_unique_message_id)(NX_SMTP_CLIENT_MAIL *mail_ptr),
                                    NX_IP *ip_ptr, NX_PACKET_POOL *packet_pool_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *bytepool_mutex_ptr, UINT bytepool_mutex_timeout,
                                    TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *blockpool_mutex_ptr, UINT blockpool_mutex_timeout, ULONG greeting_timeout, ULONG envelope_timeout,
                                    ULONG message_timeout, ULONG window_size);
    UINT    _nx_smtp_client_delete (NX_SMTP_CLIENT *client_ptr);
    UINT    _nxe_smtp_client_delete (NX_SMTP_CLIENT *client_ptr);
    UINT    _nx_smtp_session_initialize(NX_SMTP_CLIENT_SESSION *this_session, ULONG session_id, NX_SMTP_CLIENT *client_ptr, ULONG ip_addr, USHORT port);
    UINT    _nxe_smtp_session_initialize(NX_SMTP_CLIENT_SESSION *this_session, ULONG session_id, NX_SMTP_CLIENT *client_ptr,  ULONG ip_addr, USHORT port);
    UINT    _nx_smtp_session_delete(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_session_delete(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_session_reinitialize(NX_SMTP_CLIENT_SESSION *session_ptr, UINT session_availability);
    UINT    _nxe_smtp_session_reinitialize(NX_SMTP_CLIENT_SESSION *session_ptr, UINT session_availability);
    UINT    _nx_smtp_mail_create(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr, CHAR *reverse_path_ptr, UINT priority, CHAR *mail_buffer, UINT buffer_length);
    UINT    _nxe_smtp_mail_create(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr, CHAR *reverse_path_ptr, UINT priority, CHAR *mail_buffer, UINT buffer_length);
    UINT    _nx_smtp_mail_create_memalloc(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL **mail_ptr, CHAR *reverse_path_ptr, UINT priority);
    UINT    _nxe_smtp_mail_create_memalloc(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL **mail_ptr, CHAR *reverse_path_ptr, UINT priority);
    UINT    _nx_smtp_mail_add(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr);
    UINT    _nxe_smtp_mail_add(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr);
    UINT    _nx_smtp_mail_delete(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr);
    UINT    _nxe_smtp_mail_delete(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr);
    UINT    _nx_smtp_recipient_create(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr, NX_SMTP_CLIENT_RECIPIENT *recipient_ptr, CHAR *recipient_name, CHAR *recipient_mailbox, UINT type);
    UINT    _nxe_smtp_recipient_create(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr, NX_SMTP_CLIENT_RECIPIENT *recipient_ptr, CHAR *recipient_name, CHAR *recipient_mailbox, UINT type);
    UINT    _nx_smtp_recipient_create_memalloc(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr, NX_SMTP_CLIENT_RECIPIENT **recipient_ptr, CHAR *recipient_name, CHAR *recipient_mailbox, UINT type);
    UINT    _nxe_smtp_recipient_create_memalloc(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr, NX_SMTP_CLIENT_RECIPIENT **recipient_ptr, CHAR *recipient_name, CHAR *recipient_mailbox, UINT type);
    UINT    _nx_smtp_recipient_add(NX_SMTP_CLIENT_MAIL *mail_ptr, NX_SMTP_CLIENT_RECIPIENT *recipient_ptr);
    UINT    _nxe_smtp_recipient_add(NX_SMTP_CLIENT_MAIL *mail_ptr, NX_SMTP_CLIENT_RECIPIENT *recipient_ptr);
    UINT    _nx_smtp_recipient_delete(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_RECIPIENT  *recipient_ptr);
    UINT    _nxe_smtp_recipient_delete(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_RECIPIENT  *recipient_ptr);
    UINT    _nx_smtp_mail_message_append(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr, CHAR *message_in_mail, ULONG message_length);
    UINT    _nxe_smtp_mail_message_append(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr, CHAR *message_in_mail, ULONG message_length);
    UINT    _nx_smtp_mail_message_process(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr);
    UINT    _nxe_smtp_mail_message_process(NX_SMTP_CLIENT_SESSION *session_ptr, NX_SMTP_CLIENT_MAIL *mail_ptr);
    UINT    _nx_smtp_session_run (NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_session_run (NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_cmd_greeting(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_cmd_greeting(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_rsp_greeting(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_rsp_greeting(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_cmd_ehlo(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_cmd_ehlo(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_rsp_ehlo(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_rsp_ehlo(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_cmd_helo(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_cmd_helo(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_rsp_helo(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_rsp_helo(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_cmd_mail(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_cmd_mail(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_rsp_mail(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_rsp_mail(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_cmd_rcpt(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_cmd_rcpt(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_rsp_rcpt(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_rsp_rcpt(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_cmd_data(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_cmd_data(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_rsp_data(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_rsp_data(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_cmd_message(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_cmd_message(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_cmd_message_memalloc(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_cmd_message_memalloc(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_rsp_message(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_rsp_message(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_cmd_rset(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_cmd_rset(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_rsp_rset(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_rsp_rset(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_cmd_quit(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_cmd_quit(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_rsp_quit(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_rsp_quit(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_cmd_noop(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_cmd_noop(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_rsp_noop(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_rsp_noop(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_cmd_auth(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_cmd_auth(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_rsp_auth(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_rsp_auth(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_cmd_auth_challenge(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_cmd_auth_challenge(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_rsp_auth_challenge(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_rsp_auth_challenge(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_rsp_hello_command(NX_SMTP_CLIENT_SESSION* session_ptr);
    UINT    _nxe_smtp_rsp_hello_command(NX_SMTP_CLIENT_SESSION* session_ptr);
    UINT    _nx_smtp_utility_read_server_code(NX_SMTP_CLIENT_SESSION *session_ptr, ULONG timeout, UINT  receive_all_lines);
    UINT    _nxe_smtp_utility_read_server_code(NX_SMTP_CLIENT_SESSION *session_ptr, ULONG timeout, UINT  receive_all_lines);
    UINT    _nxe_smtp_utility_send_to_server(NX_SMTP_CLIENT_SESSION *session_ptr, CHAR *buffer_ptr, UINT buffer_length, ULONG timeout);
    UINT    _nx_smtp_utility_send_to_server(NX_SMTP_CLIENT_SESSION *session_ptr, CHAR *buffer_ptr, UINT buffer_length, ULONG timeout);
    UINT    _nxe_smtp_utility_data_transparency(CHAR *buffer, UINT *length, UINT additional_buffer);
    UINT    _nx_smtp_utility_data_transparency(CHAR *buffer, UINT *length, UINT additional_buffer);
    UINT    _nxe_smtp_utility_parse_server_services(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nx_smtp_utility_parse_server_services(NX_SMTP_CLIENT_SESSION *session_ptr);
    UINT    _nxe_smtp_utility_authentication_challenge(NX_SMTP_CLIENT_SESSION *session_ptr, CHAR *buffer_ptr, UINT length);
    UINT    _nx_smtp_utility_authentication_challenge(NX_SMTP_CLIENT_SESSION *session_ptr, CHAR *buffer_ptr, UINT length);
    UINT    _nxe_smtp_utility_print_client_mail_status(NX_SMTP_CLIENT_MAIL *mail_ptr);
    UINT    _nx_smtp_utility_print_client_mail_status(NX_SMTP_CLIENT_MAIL *mail_ptr);
    UINT    _nx_smtp_utility_print_client_reserves(NX_SMTP_CLIENT *client_ptr);
    UINT    _nxe_smtp_utility_print_client_reserves(NX_SMTP_CLIENT *client_ptr);
    UINT    _nx_smtp_bytepool_memory_get(VOID **memory_ptr, ULONG memory_size, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
    UINT    _nxe_smtp_bytepool_memory_get(VOID **memory_ptr, ULONG memory_size, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
    UINT    _nx_smtp_bytepool_memory_release(VOID *memory_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
    UINT    _nxe_smtp_bytepool_memory_release(VOID *memory_ptr, TX_BYTE_POOL *bytepool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
    UINT    _nx_smtp_blockpool_memory_get(VOID **memory_ptr, TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
    UINT    _nxe_smtp_blockpool_memory_get(VOID **memory_ptr, TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
    UINT    _nx_smtp_blockpool_memory_release(VOID *memory_ptr, TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);
    UINT    _nxe_smtp_blockpool_memory_release(VOID *memory_ptr, TX_BLOCK_POOL *blockpool_ptr, TX_MUTEX *mutex_ptr, UINT mutex_wait_option);



/* Define internal SMTP Client functions.  */

    VOID    _nx_smtp_utility_service_name(CHAR *buffer, UINT command);
    VOID    _nx_smtp_utility_set_next_auth_type(NX_SMTP_CLIENT_SESSION *session_ptr);
    VOID    _nx_smtp_parse_response(CHAR *buffer, UINT arguement_index, UINT buffer_length, CHAR *arguement, UINT arguement_length, UINT convert_to_uppercase, UINT include_crlf);
    VOID    _nx_smtp_find_crlf(CHAR *buffer, UINT length, CHAR **CRLF, UINT reverse);
    VOID    _nx_smtp_parse_mailbox_address(CHAR *start_buffer, CHAR *end_buffer, CHAR **mailbox_address, UINT addr_is_parameter);
    VOID    _nx_smtp_base64_encode(CHAR *name, CHAR *base64name);
    VOID    _nx_smtp_base64_decode(CHAR *base64name, CHAR *name);

#endif   /*  NX_SMTP_SOURCE_CODE */

/* If a C++ compiler is being used....*/
#ifdef   __cplusplus
}
#endif


#endif /* NX_SMTP_CLIENT_H  */
