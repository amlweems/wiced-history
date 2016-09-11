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
/*  Express Logic, Inc.                                                   */
/*  11423 West Bernardo Court               info@expresslogic.com         */
/*  San Diego, CA 92127                     http://www.expresslogic.com   */
/*                                                                        */
/**************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** NetX SNTP Component                                                   */
/**                                                                       */
/**   Simple Network Time Protocol (SNTP)                                 */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  APPLICATION INTERFACE DEFINITION                       RELEASE        */
/*                                                                        */
/*    nx_sntp.h                                           PORTABLE C      */
/*                                                           5.2          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Express Logic, Inc.                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file contains the NetX Simple Network Time Protocol (SNTP)     */
/*    components, including data types and external references, common    */
/*    to both SNTP Server and SNTP Client API.                            */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-25-2007     William E. Lamie         Initial Version 5.0           */
/*  08-15-2008     William E. Lamie         Modified comment(s), added    */
/*                                            logic for little Endian     */
/*                                            processing, backoff         */
/*                                            algorithm processing of     */
/*                                            polling intervals, zero     */
/*                                            SNTP data checking,         */
/*                                            handling failed sanity      */
/*                                            checks by error code,       */
/*                                            displaying NTP time in      */
/*                                            readable date time format   */
/*                                            resulting in version 5.1    */
/*  04-01-2010     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 5.2    */ 
/*                                                                        */
/**************************************************************************/

#ifndef NX_SNTP_H
#define NX_SNTP_H


#include "tx_api.h"
#include "nx_api.h"
#include "nx_ip.h"


/* Determine if a C++ compiler is being used.  If so, ensure that standard
   C is used to process the API information.  */

#ifdef   __cplusplus

/* Yes, C++ compiler is present.  Use standard C.  */ 
extern   "C" {

#endif



/* ID for identifying as an SNTP client */

#define NX_SNTP_ID                              0x534E5450UL


/* Conversion between seconds and timer ticks. See tx_initialize_low_level.<asm> 
   for timer tick resolution before altering! */ 

#define NX_SNTP_MILLISECONDS_PER_TICK           10
#define NX_SNTP_TICKS_PER_SECOND                100


    /*  SNTP Debug levels in decreased filtering order:  

    NONE:       No events reported;
    LOG:        Event reported if the SNTP Client is enabled for logging 
    SEVERE:     Report only events requiring the Client to stop time update processing.
    MODERATE:   Report events possibly preventing successful time update
    ALL:        All events reported  */

typedef enum NX_SNTP_DEBUG_LEVEL_ENUM
{
    NONE,
    LOG,
    SEVERE, 
    MODERATE, 
    ALL
} NX_SNTP_DEBUG_LEVEL;



/* Define the minimum size of the packet NTP/SNTP time message 
   (e.g. without authentication data).  */

#define NX_SNTP_TIME_MESSAGE_MIN_SIZE           48


/* Define the maximum size of the packet NTP/SNTP time message (includes 20 bytes for
   optional authentication data).  */

#define NX_SNTP_TIME_MESSAGE_MAX_SIZE           68


/* Define the largest IP4 size for ip address e.g. xxx.xxx.xxx.xxx */

#define NX_SNTP_CLIENT_MAX_IP_ADDRESS_SIZE      15


/* Internal SNTP error processing codes.  */

#define NX_SNTP_ERROR_CONSTANT                  0xD00


/* Client side errors.  */
#define NX_SNTP_PARAM_ERROR                 (NX_SNTP_ERROR_CONSTANT | 0x00)       /* Invalid non pointer parameter.  */ 
#define NX_SNTP_CLIENT_NOT_INITIALIZED      (NX_SNTP_ERROR_CONSTANT | 0x01)       /* Client not properly initialized to receive time data.  */
#define NX_SNTP_OVER_LIMIT_ON_SERVERS       (NX_SNTP_ERROR_CONSTANT | 0x02)       /* Cannot accept server because client list has reached max # servers.  */      
#define NX_SNTP_INVALID_DOMAIN              (NX_SNTP_ERROR_CONSTANT | 0x03)       /* Invalid domain such as bad IP format or empty string. Applicable to broadcast mode.  */      
#define NX_SNTP_NO_AVAILABLE_SERVERS        (NX_SNTP_ERROR_CONSTANT | 0x04)       /* Client has no available time servers to contact.  */
#define NX_SNTP_INVALID_LOCAL_TIME          (NX_SNTP_ERROR_CONSTANT | 0x05)       /* Client local time has not been set or is invalid.  */
#define NX_SNTP_OUT_OF_DOMAIN_SERVER        (NX_SNTP_ERROR_CONSTANT | 0x06)       /* Broadcast server does not belong to client broadcast domain.  */
#define NX_SNTP_INVALID_DATETIME_BUFFER     (NX_SNTP_ERROR_CONSTANT | 0x07)       /* Insufficient or invalid buffer for writing human readable time date string.  */
#define NX_SNTP_ERROR_CONVERTING_DATETIME   (NX_SNTP_ERROR_CONSTANT | 0x08)       /* An internal error has occurred converting NTP time to mm-dd-yy time format.  */

/* Server side errors */
#define NX_SNTP_SERVER_NOT_AVAILABLE        (NX_SNTP_ERROR_CONSTANT | 0x08)       /* Client will not get any time update service from current server.  */
#define NX_SNTP_NO_UNICAST_FROM_SERVER      (NX_SNTP_ERROR_CONSTANT | 0x09)       /* Client did not receive a valid unicast response from server.  */
#define NX_SNTP_SERVER_CLOCK_NOT_SYNC       (NX_SNTP_ERROR_CONSTANT | 0x0A)       /* Server clock not synchronized.  */      
#define NX_SNTP_KOD_SERVER_NOT_AVAILABLE    (NX_SNTP_ERROR_CONSTANT | 0x0B)       /* Server sent a KOD packet indicating service temporarily not available.  */      
#define NX_SNTP_KOD_REMOVE_SERVER           (NX_SNTP_ERROR_CONSTANT | 0x0C)       /* Server sent a KOD packet indicating service is not available to client (ever).  */      
#define NX_SNTP_SERVER_AUTH_FAIL            (NX_SNTP_ERROR_CONSTANT | 0x0D)       /* Server rejects Client packet on basis of missing or invalid authorization data.  */      

/* Bad packet and time update errors */
#define NX_SNTP_INVALID_TIME_PACKET         (NX_SNTP_ERROR_CONSTANT | 0x10)       /* Invalid packet (length or data incorrect).   */
#define NX_SNTP_INVALID_NTP_VERSION         (NX_SNTP_ERROR_CONSTANT | 0x11)       /* Server NTP/SNTP version not incompatible with client.  */      
#define NX_SNTP_INVALID_SERVER_MODE         (NX_SNTP_ERROR_CONSTANT | 0x12)       /* Server association invalid (out of protocol with client).  */
#define NX_SNTP_INVALID_SERVER_PORT         (NX_SNTP_ERROR_CONSTANT | 0x13)       /* Server port does not match what the client expects.  */
#define NX_SNTP_INVALID_IP_ADDRESS          (NX_SNTP_ERROR_CONSTANT | 0x14)       /* Server IP address does not match what the client expects.  */
#define NX_SNTP_INVALID_SERVER_STRATUM      (NX_SNTP_ERROR_CONSTANT | 0x15)       /* Server stratum is invalid or below client stratum.  */
#define NX_SNTP_BAD_SERVER_ROOT_DISPERSION  (NX_SNTP_ERROR_CONSTANT | 0x16)       /* Server root dispersion (clock precision) is too high or invalid value (0) reported.  */
#define NX_SNTP_OVER_BAD_UPDATE_LIMIT       (NX_SNTP_ERROR_CONSTANT | 0x17)       /* Client over the limit on consecutive server updates with bad data received.  */
#define NX_SNTP_DUPE_SERVER_PACKET          (NX_SNTP_ERROR_CONSTANT | 0x18)       /* Client has received duplicate packets from server.  */
#define NX_SNTP_INVALID_TIMESTAMP           (NX_SNTP_ERROR_CONSTANT | 0x19)       /* Server time packet has one or more invalid time stamps in update message.  */

/* Arithmetic errors or invalid results */
#define NX_SNTP_INVALID_TIME                (NX_SNTP_ERROR_CONSTANT | 0x20)       /* Invalid time resulting from arithmetic operation.  */
#define NX_SNTP_INVALID_RTT_TIME            (NX_SNTP_ERROR_CONSTANT | 0x21)       /* Round trip time correction to server time yields invalid time (e.g. <0).  */
#define NX_SNTP_OVERFLOW_ERROR              (NX_SNTP_ERROR_CONSTANT | 0x22)       /* Overflow error resulting from multiplying/adding two 32 bit (timestamp) numbers.  */      
#define NX_SNTP_SIGN_ERROR                  (NX_SNTP_ERROR_CONSTANT | 0x23)       /* Loss of sign error resulting from multiplying/adding two 32 bit (timestamp) numbers.  */      

/* Time out errors */
#define NX_SNTP_TIMED_OUT_ON_SERVER         (NX_SNTP_ERROR_CONSTANT | 0x24)       /* Client did not receive update from the current server within specified timeout.  */
#define NX_SNTP_MAX_TIME_LAPSE_EXCEEDED     (NX_SNTP_ERROR_CONSTANT | 0x25)       /* Client has not received update from any server within the max allowed time lapse.  */


/* Define fields in the NTP message format.  */

#define REFERENCE_TIME      0
#define ORIGINATE_TIME      1
#define RECEIVE_TIME        2
#define TRANSMIT_TIME       3

/* Kiss of death strings (see Codes below). Applicable when stratum = 0    

                            Code    Meaning
      --------------------------------------------------------------  */


#define       ANYCAST       "ACST"    /* The association belongs to an anycast server.  */
#define       AUTH_FAIL     "AUTH"    /* Server authentication failed.  */
#define       AUTOKEY_FAIL  "AUTO"    /* Autokey sequence failed.  */
#define       BROADCAST     "BCST"    /* The association belongs to a broadcast server.  */
#define       CRYP_FAIL     "CRYP"    /* Cryptographic authentication or identification failed.  */
#define       DENY          "DENY"    /* Access denied by remote server.  */
#define       DROP          "DROP"    /* Lost peer in symmetric mode.  */
#define       DENY_POLICY   "RSTR"    /* Access denied due to local policy.  */
#define       NOT_INIT      "INIT"    /* The association has not yet synchronized for the first time.  */
#define       MANYCAST      "MCST"    /* The association belongs to a manycast server.  */
#define       NO_KEY        "NKEY"    /* No key found.  Either the key was never installed or is not trusted.  */
#define       RATE          "RATE"    /* Rate exceeded.  The server temporarily denied access; client exceeded rate threshold.  */
#define       RMOT          "RMOT"    /* Somebody is tinkering with association from remote host running ntpdc.  OK unless they've stolen your keys.  */
#define       STEP          "STEP"    /* A step change in system time has occurred, but association has not yet resynchronized.  */

/* Define Kiss of Death error codes.  Note: there should be a 1 : 1 correspondence of 
   KOD strings to KOD error codes! */

#define       NX_SNTP_KISS_OF_DEATH_PACKET                  0xF00

#define       NX_SNTP_KOD_ANYCAST               (NX_SNTP_KISS_OF_DEATH_PACKET | 0x01)
#define       NX_SNTP_KOD_AUTH_FAIL             (NX_SNTP_KISS_OF_DEATH_PACKET | 0x02)
#define       NX_SNTP_KOD_AUTOKEY_FAIL          (NX_SNTP_KISS_OF_DEATH_PACKET | 0x03)  
#define       NX_SNTP_KOD_BROADCAST             (NX_SNTP_KISS_OF_DEATH_PACKET | 0x04)     
#define       NX_SNTP_KOD_CRYP_FAIL             (NX_SNTP_KISS_OF_DEATH_PACKET | 0x05)     
#define       NX_SNTP_KOD_DENY                  (NX_SNTP_KISS_OF_DEATH_PACKET | 0x06)          
#define       NX_SNTP_KOD_DROP                  (NX_SNTP_KISS_OF_DEATH_PACKET | 0x07)          
#define       NX_SNTP_KOD_DENY_POLICY           (NX_SNTP_KISS_OF_DEATH_PACKET | 0x08)   
#define       NX_SNTP_KOD_NOT_INIT              (NX_SNTP_KISS_OF_DEATH_PACKET | 0x09)      
#define       NX_SNTP_KOD_MANYCAST              (NX_SNTP_KISS_OF_DEATH_PACKET | 0x0A)      
#define       NX_SNTP_KOD_NO_KEY                (NX_SNTP_KISS_OF_DEATH_PACKET | 0x0B)        
#define       NX_SNTP_KOD_RATE                  (NX_SNTP_KISS_OF_DEATH_PACKET | 0x0C)          
#define       NX_SNTP_KOD_RMOT                  (NX_SNTP_KISS_OF_DEATH_PACKET | 0x0D)          
#define       NX_SNTP_KOD_STEP                  (NX_SNTP_KISS_OF_DEATH_PACKET | 0x0E)          


/* Define SNTP protocol modes. SNTP is limited to primarily Client, server and broadcast modes.  */

#define     PROTOCOL_MODE_RESERVED          0
#define     PROTOCOL_MODE_SYMM_ACTIVE       1
#define     PROTOCOL_MODE_SYMM_PASSIVE      2
#define     PROTOCOL_MODE_CLIENT            3
#define     PROTOCOL_MODE_SERVER_UNICAST    4
#define     PROTOCOL_MODE_SERVER_BROADCAST  5
#define     PROTOCOL_MODE_NTP_CNTL_MSG      6
#define     PROTOCOL_MODE_PRIVATE           7


/* Define Client request types. Note that this does not limit the 
   NetX SNTP Client from using MANYCAST or MULTICAST discovery options.  */

#define     BROADCAST_MODE      1
#define     UNICAST_MODE        2


/* Define masks for stratum levels.  */

#define STRATUM_KISS_OF_DEATH   0x00
#define STRATUM_PRIMARY         0x01
#define STRATUM_SECONDARY       0x0E  /* 2 - 15 */
#define STRATUM_RESERVED        0xF0  /* 16 - 255*/


/* Set minimum and maximum Client unicast poll period (sec) for requesting  
   time data.  RFC 4330 polling range is from 16 - 131072 seconds.  
   Note that the RFC 4330 strongly recommends polling intervals of at least 
   one minute to unnecessary reduce demand on public time servers.  */

#define NX_SNTP_CLIENT_MIN_UNICAST_POLL_INTERVAL    64
#define NX_SNTP_CLIENT_MAX_UNICAST_POLL_INTERVAL    131072



/* If a C++ compiler is being used */
#ifdef   __cplusplus
        }
#endif


#endif /* NX_SNTP_H */
