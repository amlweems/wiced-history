/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * WICED Over The Air 2 Background Service interface (OTA2)
 *
 *        ***  PRELIMINARY - SUBJECT TO CHANGE  ***
 *
 * NOTE: Network must be up and connected to an AP before starting the service
 * NOTE: The platfrom must have an RTC for the interval update checking
 *
 * Before calling this API
 * - Network must be up and connected to an AP with
 *      access to the update server
 *
 * The OTA2 Service will periodically check and perform OTA updates
 *
 *  if no callback is registered
 *      OTA2 Service will perform default actions:
 *          - check for first update at initial_check_interval
 *          - check for updates at check_interval
 *          - download updates when available
 *          - extract & perform update on next power cycle
 *  else
 *      Inform the application via callback
 *          If Application returns WICED_SUCCESS
 *              OTA Service will perform default action
 *          If application returns WICED_ERROR
 *              Application will perform action
 */

#include <ctype.h>
#include "wiced.h"
#include "wiced_time.h"
#include "wiced_tcpip.h"
#include "wiced_rtos.h"
#include "dns.h"
#include "http_stream.h"
#include "wiced_ota2_image.h"
#include "wiced_ota2_network.h"
#include "wiced_ota2_service.h"
#include "../../utilities/mini_printf/mini_printf.h"

/* define to show a bar graph on the console to show download progress */
#define SHOW_BAR_GRAPH
#define BAR_GRAPH_LENGTH 50

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_OTA2_SERVICE_TAG_VALID        0x61233216
#define WICED_OTA2_SERVICE_TAG_INVALID      0x61DEAD16

#define WICED_OTA2_WORKER_THREAD_PRIORITY           (WICED_DEFAULT_WORKER_PRIORITY)

#define WICED_OTA2_WORKER_STACK_SIZE               (11 * 1024)

#define WICED_OTA2_THREAD_SHUTDOWN_WAIT            (100)   /* wait for a shutdown flag */
#define WICED_OTA2_WORKER_FLAGS_TIMEOUT            ( -1)    /* wait forever             */

#define WICED_OTA2_ANY_PORT                        (  0)

#define WICED_OTA2_HEADER_BUFFER_SIZE              (1024)  /* large enough for the ota2 header + all component headers */

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                  Enumerations
 ******************************************************/
typedef enum
{
    OTA2_EVENT_WORKER_THREAD_SHUTDOWN_NOW   = (1 <<  0),

    OTA2_EVENT_WORKER_START_INITIAL_TIMER   = (1 <<  1),
    OTA2_EVENT_WORKER_START_NEXT_TIMER      = (1 <<  2),
    OTA2_EVENT_WORKER_START_RETRY_TIMER     = (1 <<  3),

    OTA2_EVENT_WORKER_CHECK_FOR_UPDATES     = (1 <<  4),
    OTA2_EVENT_WORKER_START_DOWNLOAD        = (1 <<  5),

    OTA2_EVENT_WORKER_PAUSE                 = (1 <<  12),
    OTA2_EVENT_WORKER_UNPAUSE               = (1 <<  13),

} OTA2_EVENTS_T;

#define OTA2_EVENT_WORKER_THREAD_EVENTS  ( OTA2_EVENT_WORKER_THREAD_SHUTDOWN_NOW |                                                                            \
                                           OTA2_EVENT_WORKER_START_INITIAL_TIMER | OTA2_EVENT_WORKER_START_NEXT_TIMER | OTA2_EVENT_WORKER_START_RETRY_TIMER | \
                                           OTA2_EVENT_WORKER_CHECK_FOR_UPDATES   | OTA2_EVENT_WORKER_START_DOWNLOAD   |                                       \
                                           OTA2_EVENT_WORKER_PAUSE | OTA2_EVENT_WORKER_UNPAUSE )

typedef enum
{
    OTA2_SERVICE_STATE_NOT_INITIALIZED  = 0,
    OTA2_SERVICE_STATE_INITIALIZED,                 /* session has been initialized                                */

    OTA2_SERVICE_STATE_NONE,                        /* background service has been started                         */

    OTA2_SERVICE_STATE_PAUSED,                      /* disconnected, no timers set,
                                                     *  background service has been paused                          */

    OTA2_SERVICE_STATE_WAITING_FOR_TIMER,           /* disconnected, timers are set,
                                                     *  we are waiting for a timed action         */

    OTA2_SERVICE_STATE_TIME_TO_CHECK_FOR_UPDATES,   /* check to see if app wants to check for updates               */

    OTA2_SERVICE_STATE_STARTING_DOWNLOAD,           /* starting the download                                        */

    OTA2_SERVICE_STATE_DOWNLOAD_DONE,               /* TCP connected, done receiving DATA                          */

    OTA2_SERVICE_STATE_MAX                          /* Must be last */

} ota2_state_t;

/******************************************************
 *                    Structures
 ******************************************************/

/* session structure */
typedef struct wiced_ota2_service_session_s
{
        uint32_t                                tag;

        wiced_utc_time_ms_t                     session_start_time; /* date, time session was initialized - used to set timer   */
        wiced_utc_time_ms_t                     session_paused_time; /* if !=0, this is when we paused the update timer         */
        wiced_utc_time_ms_t                     session_next_time;  /* date, time next timer                                    */
        wiced_utc_time_ms_t                     session_last_time;  /* date, time we last updated                               */

        volatile ota2_state_t                   ota2_state;
        wiced_ota2_service_status_t             last_error_status;  /* last error status                                        */

        wiced_time_t                            last_timer_start;   /* time when last timer was started  (0 = no timer active)  */
        wiced_time_t                            last_timer_value;   /* timer value                                              */

        wiced_time_t                            retry_timer_start;   /* time when last retry timer was started  (0 = no timer active)  */
        wiced_time_t                            retry_timer_value;  /* timer value                                              */

        wiced_mutex_t                           ota2_mutex;         /* for updating internal state */

        ota2_service_callback                   cb_function;        /*  Application callback function and data                  */
        void*                                   cb_opaque;

        wiced_thread_t                          ota2_worker_thread;
        volatile wiced_thread_t*                ota2_worker_thread_ptr;

        wiced_event_flags_t                     ota2_worker_flags;  /* to signal worker thread */

        /* connect & download info */
        wiced_config_ap_entry_t*    ota2_ap_info;    /* Alternate AP to use to connect to the OTA2 update server
                                                      * - This is optional. If the default AP has access to the
                                                      *   OTA2 update server, this will be NULL                 */
        wiced_config_ap_entry_t*    default_ap_info; /* Default AP to connect to after the OTA2 update is complete
                                                      * This is optional. If the default AP has access to the OTA2
                                                      * update server, this will be NULL
                                                      * - If the application needs a special access point
                                                      *   connection, This will be NULL.                        */
        wiced_config_ap_entry_t*    curr_network;   /* ptr equals current network info if connected to an AP.
                                                     * if NULL, we are not connected to a network               */


        char                            http_query[WICED_OTA2_HTTP_QUERY_SIZE];       /* for building the http query */
        char                            host_name[WICED_OTA2_HOST_NAME];
        char                            file_path[WICED_OTA2_FILE_PATH];
        wiced_tcp_socket_t              tcp_socket;
        wiced_bool_t                    tcp_socket_created;
        wiced_bool_t                    tcp_socket_bound;

        char                            last_connected_host_name[MAX_HTTP_HOST_NAME_SIZE + 1];

        uint32_t                        last_check_time;        /* last check time (system milliseconds/1000)                 */
        uint32_t                        initial_check_interval; /* seconds before 1st update check                            */
        uint32_t                        check_interval;         /* seconds between start of one check and start of next check */
        uint32_t                        retry_check_interval;   /* seconds between re-try if initial contact to
                                                                 * server for update info fails
                                                                 * 0 = wait until next check_interval                         */


        uint8_t                         auto_update;            /* Callback return value over-rides this parameter
                                                                 * Auto-update behavior if no callback registered.
                                                                 *   1 = Service will inform Bootloader to extract
                                                                 *       and update on next power cycle after download
                                                                 *   0 = Service will inform Bootloader that download
                                                                 *       is complete - Bootloader will NOT extract/update
                                                                 *       until user / application requests                  */

        /* timer */
        wiced_timer_t                   check_update_timer;
        wiced_timer_t                   retry_timer;            /* for starting a re-try, keep seperate from the main timer */

        uint8_t                        header_buffer[WICED_OTA2_HEADER_BUFFER_SIZE];          /* for checking if the update is valid                      */

        /* debugging */
        int                             log_level;              /* debug logging level                                      */
        int                             attempted_updates;
        int                             successful_updates;
        int                             failed_updates;
        int                             download_failures;
        int                             tcp_failures;
        int                             tcp_timeouts;
        int                             ota2_ap_failures;
        int                             default_ap_failures;

} wiced_ota2_service_session_t;

static wiced_ota2_service_session_t *g_only_one_session_at_a_time = NULL;

/******************************************************
 *               Variables Definitions
 ******************************************************/
/* template for HTTP GET */
static char ota2_get_request_template[] =
{
    "GET %s HTTP/1.1\r\n"
    "Host: %s%s \r\n"
    "\r\n"
};

/****************************************************************
 *  Internal functions
 ****************************************************************/
static void wiced_ota2_check_update_timer_handler( void* arg )
{
    wiced_ota2_service_session_t*      session;

    wiced_assert("wiced_ota2_check_update_timer_handler() ARG == NULL!", (arg != 0));
    session = (wiced_ota2_service_session_t*)arg;

    /* signal the worker thread to re-start this timer */
    wiced_rtos_set_event_flags(&session->ota2_worker_flags, OTA2_EVENT_WORKER_START_NEXT_TIMER);
    /* signal the worker thread to check if there is an update available */
    wiced_rtos_set_event_flags(&session->ota2_worker_flags, OTA2_EVENT_WORKER_CHECK_FOR_UPDATES);
}

static void wiced_ota2_retry_timer_handler( void* arg )
{
    wiced_ota2_service_session_t*      session;

    wiced_assert("wiced_ota2_retry_timer_handler() ARG == NULL!", (arg != 0));
    session = (wiced_ota2_service_session_t*)arg;

    /* do not trigger another timer start */
    /* signal the worker thread to check if there is an update available */
    wiced_rtos_set_event_flags(&session->ota2_worker_flags, OTA2_EVENT_WORKER_CHECK_FOR_UPDATES);
}


static wiced_result_t ota2_service_start_timer(wiced_ota2_service_session_t* session, wiced_time_t time)
{
    wiced_time_t    curr_time;

    if (session == NULL )
    {
        return WICED_ERROR;
    }

    if (wiced_rtos_is_current_thread( &session->ota2_worker_thread ) != WICED_SUCCESS)
    {
        mini_printf("START timer wrong thread\r\n");
    }

    wiced_time_get_time( &curr_time );
    session->last_timer_start = curr_time;
    session->last_timer_value = time;

    wiced_rtos_init_timer(&session->check_update_timer,  time, wiced_ota2_check_update_timer_handler, session);
    wiced_rtos_start_timer(&session->check_update_timer );

    return WICED_SUCCESS;
}

static void ota2_service_stop_timer(wiced_ota2_service_session_t* session)
{
    if (session == NULL )
    {
        return;
    }

    if (wiced_rtos_is_current_thread( &session->ota2_worker_thread ) != WICED_SUCCESS)
    {
        mini_printf("STOP timer wrong thread\r\n");
    }

    if (wiced_rtos_is_timer_running(&session->check_update_timer) == WICED_SUCCESS )
    {
        wiced_rtos_stop_timer(&session->check_update_timer);
        wiced_rtos_deinit_timer(&session->check_update_timer);
    }
}

static wiced_result_t ota2_service_start_retry_timer(wiced_ota2_service_session_t* session, wiced_time_t time)
{
    wiced_time_t    curr_time;

    if (session == NULL )
    {
        return WICED_ERROR;
    }

    if (wiced_rtos_is_current_thread( &session->ota2_worker_thread ) != WICED_SUCCESS)
    {
        mini_printf("START timer wrong thread\r\n");
    }

    wiced_time_get_time( &curr_time );
    session->retry_timer_start = curr_time;
    session->retry_timer_value = time;

    wiced_rtos_init_timer(&session->retry_timer,  time, wiced_ota2_retry_timer_handler, session);
    wiced_rtos_start_timer(&session->retry_timer );

    return WICED_SUCCESS;
}

static void ota2_service_stop_retry_timer(wiced_ota2_service_session_t* session)
{
    if (session == NULL )
    {
        return;
    }

    if (wiced_rtos_is_current_thread( &session->ota2_worker_thread ) != WICED_SUCCESS)
    {
        mini_printf("STOP retry timer wrong thread\r\n");
    }

    if (wiced_rtos_is_timer_running(&session->retry_timer) == WICED_SUCCESS )
    {
        wiced_rtos_stop_timer(&session->retry_timer);
        wiced_rtos_deinit_timer(&session->retry_timer);
    }
}

static wiced_result_t  wiced_ota2_service_make_callback(wiced_ota2_service_session_t* session, wiced_ota2_service_status_t status, uint32_t value )
{
    wiced_result_t  cb_result = WICED_SUCCESS;

    if ((session != NULL) && (session->cb_function != NULL) )
    {
        cb_result = (session->cb_function)(session, status, value, session->cb_opaque);
    }

    return cb_result;
}

static wiced_result_t wiced_ota2_service_make_error_callback(wiced_ota2_service_session_t* session, wiced_ota2_service_status_t error_status)
{
    wiced_result_t              cb_result = WICED_SUCCESS;

    if ( session == NULL)
    {
        return WICED_BADARG;
    }
    session->last_error_status = error_status;

    cb_result = wiced_ota2_service_make_callback(session, error_status, 0 );

    return cb_result;
}


static ota2_state_t ota2_service_get_state(wiced_ota2_service_session_t* session)
{
    if (session == NULL)
    {
        return OTA2_SERVICE_STATE_NOT_INITIALIZED;
    }

    return session->ota2_state;
}

static wiced_result_t ota2_service_set_state(wiced_ota2_service_session_t* session, ota2_state_t new_state)
{
    if (wiced_rtos_lock_mutex( &session->ota2_mutex ) != WICED_SUCCESS)
    {
         OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("ota2_service_set_state() fail!\r\n"));
        return WICED_ERROR;
    }

    session->ota2_state = new_state;

    wiced_rtos_unlock_mutex( &session->ota2_mutex );

    return WICED_SUCCESS;
}

static wiced_result_t wiced_ota2_service_pause(wiced_ota2_service_session_t* session)
{
    volatile ota2_state_t state_check;
    if (session == NULL)
    {
        return WICED_BADARG;
    }

    if (session->ota2_state > OTA2_SERVICE_STATE_WAITING_FOR_TIMER)
    {
        return WICED_ERROR;
    }

    /* put the worker thread into pause */
    wiced_rtos_set_event_flags(&session->ota2_worker_flags, OTA2_EVENT_WORKER_PAUSE);

    /* check state - wait until we go into pause */
    state_check = OTA2_SERVICE_STATE_NOT_INITIALIZED;
    while ( state_check != OTA2_SERVICE_STATE_PAUSED)
    {
        wiced_rtos_delay_milliseconds(100);
        state_check = session->ota2_state;
    }

    return WICED_SUCCESS;
}
static wiced_result_t wiced_ota2_service_unpause(wiced_ota2_service_session_t* session)
{
    volatile ota2_state_t state_check;
    if (session == NULL)
    {
        return WICED_BADARG;
    }

    wiced_rtos_set_event_flags(&session->ota2_worker_flags, OTA2_EVENT_WORKER_UNPAUSE);

    /* check state - wait until we come out of pause */
    state_check = OTA2_SERVICE_STATE_NOT_INITIALIZED;
    while (state_check == OTA2_SERVICE_STATE_PAUSED)
    {
        wiced_rtos_delay_milliseconds(100);
        state_check = session->ota2_state;
    }

    return WICED_SUCCESS;
}

/****************************************************************
 *  AP Network Up / Down
 ****************************************************************/

static wiced_result_t wiced_ota2_service_network_switch_to_ota2_ap(wiced_ota2_service_session_t* session )
{
    wiced_result_t result;
    char            ssid_name[ SSID_NAME_SIZE + 1 ] = { 0 };

    if (session->ota2_ap_info == NULL )
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("wiced_ota2_service_network_switch_to_ota2_ap() network to switch to.\r\n"));
        return WICED_SUCCESS;
    }

    /* get a print-safe version of the name */
    memcpy( ssid_name, session->ota2_ap_info ->details.SSID.value, session->ota2_ap_info ->details.SSID.length);

    if (session->curr_network == session->ota2_ap_info )
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("wiced_ota2_service_network_switchto_ota2_ap() already connected to: %s\r\n", ssid_name));
        return WICED_SUCCESS;
    }

    if (session->curr_network != NULL)
    {
        /* bring down current AP, we want to connect to a different one */
        OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("wiced_ota2_service_network_switch_to_ota2_ap() disconnecting\r\n"));
        result = wiced_ota2_network_down() ;
        if (result != WICED_SUCCESS)
        {
            OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("wiced_ota2_service_network_switch_to_ota2_ap() wiced_ota2_network_down() %d\r\n", result));
        }
        session->curr_network = NULL;
    }

    result = wiced_ota2_network_up(session->ota2_ap_info ) ;
    if (result == WICED_SUCCESS)
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("Connected to AP: %s\r\n", ssid_name));
        session->curr_network = session->ota2_ap_info ;
    }
    else
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("Connection to :%s FAILED !!!!!\r\n", ssid_name));
    }

    return result;
}

static wiced_result_t wiced_ota2_service_network_switch_to_default_ap(wiced_ota2_service_session_t* session )
{
    wiced_result_t result;
    char            ssid_name[ SSID_NAME_SIZE + 1 ] = { 0 };

    if (session->default_ap_info == NULL )
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("wiced_ota2_service_network_switch_to_default_ap() network to switch to.\r\n"));
        return WICED_SUCCESS;
    }

    /* get a print-safe version of the name */
    memcpy( ssid_name, session->default_ap_info ->details.SSID.value, session->default_ap_info ->details.SSID.length);

    if (session->curr_network == session->default_ap_info )
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("wiced_ota2_service_network_switchto_default_ap() already connected to: %s\r\n", ssid_name));
        return WICED_SUCCESS;
    }

    if (session->curr_network != NULL)
    {
        /* bring down current AP, we want to connect to a different one */
        OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("wiced_ota2_service_network_switch_to_default_ap() disconnecting\r\n"));
        result = wiced_ota2_network_down() ;
        if (result != WICED_SUCCESS)
        {
            OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("wiced_ota2_service_network_switch_to_ota2_ap() wiced_ota2_network_down() %d\r\n", result));
        }
        session->curr_network = NULL;
    }


    result = wiced_ota2_network_up(session->default_ap_info ) ;
    if (result == WICED_SUCCESS)
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("Connected to :%s\r\n", ssid_name));
        session->curr_network = session->default_ap_info ;
    }
    else
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("Connection to :%s FAILED !!!!!\r\n", ssid_name));
    }

    return result;
}

/****************************************************************
 *  HTTP URI connect / disconnect Function Declarations
 ****************************************************************/

static wiced_result_t  ota2_service_check_socket_created(wiced_ota2_service_session_t* session)
{
    wiced_result_t result;

    if (session->tcp_socket_created == WICED_FALSE)
    {
        result = wiced_tcp_create_socket( &session->tcp_socket, WICED_STA_INTERFACE );
        if ( result != WICED_SUCCESS )
        {
            OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("wiced_tcp_create_socket() failed! %d\r\n", result));
            return result;
        }
        session->tcp_socket_created = WICED_TRUE;
    }

    /* bind */
    if (session->tcp_socket_bound == WICED_FALSE)
    {
        result = wiced_tcp_bind( &session->tcp_socket, WICED_OTA2_ANY_PORT);
        if ( result != WICED_SUCCESS )
        {
            OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("wiced_tcp_bind() failed! %d\r\n", result));
            return result;
        }
        session->tcp_socket_bound = WICED_TRUE;
    }

    return result;
}

static wiced_result_t  ota2_service_disconnect(wiced_ota2_service_session_t* session)
{
    wiced_result_t result = WICED_SUCCESS;

    if (session->tcp_socket_bound == WICED_TRUE)
    {
        result = wiced_tcp_disconnect( &session->tcp_socket);
        if ( result != WICED_SUCCESS )
        {
            OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("wiced_tcp_disconnect() failed! %d\r\n", result));
        }
    }
    session->tcp_socket_bound = WICED_FALSE;


    if (session->tcp_socket_created == WICED_TRUE)
    {
        result = wiced_tcp_delete_socket( &session->tcp_socket );
        if ( result != WICED_SUCCESS )
        {
            OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("wiced_tcp_delete_socket() failed!\r\n"));
        }
    }
    session->tcp_socket_created = WICED_FALSE;

    return result;
}

static wiced_result_t ota2_service_connect(wiced_ota2_service_session_t* session)
{
    wiced_result_t      result = WICED_SUCCESS;
    wiced_ip_address_t  ip_address;
    uint16_t            connect_tries;

    char                host_name[WICED_OTA2_HOST_NAME];
    char                file_path[WICED_OTA2_FILE_PATH];

    if ( ota2_service_check_socket_created(session) != WICED_SUCCESS)
    {
        /* informational */
        wiced_ota2_service_make_error_callback(session, OTA2_SERVICE_SERVER_CONNECT_ERROR);
        return WICED_ERROR;
    }

    strlcpy(host_name, session->host_name, sizeof(host_name));
    strlcpy(file_path, session->file_path, sizeof(file_path));

    OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("Connect to: host:%s path:%s port:%d\r\n", host_name, file_path, WICED_OTA2_HTTP_PORT) );

    /* see if the host is an IP address in order to skip the DNS lookup */
    if (str_to_ip( host_name, &ip_address ) != 0)
    {
        result =  dns_client_hostname_lookup( host_name, &ip_address, 10000 );
        if (result!= WICED_SUCCESS)
        {
            OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("ota2_connect() dns_client_hostname_lookup(%s) failed!\r\n", host_name));
            wiced_ota2_service_make_error_callback(session, OTA2_SERVICE_SERVER_CONNECT_ERROR);
            goto _connect_fail;
        }
    }

    /* try a few times to actually connect */
    connect_tries = 0;
    result = WICED_ERROR;
    while ((connect_tries < 3) && (result != WICED_SUCCESS) )
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("Try %d Connecting to %s:%d  (%ld.%ld.%ld.%ld) !\r\n",
                 connect_tries, host_name, WICED_OTA2_HTTP_PORT,
                ((ip_address.ip.v4 >> 24) & 0xff), ((ip_address.ip.v4 >> 16) & 0x0ff),
                ((ip_address.ip.v4 >>  8) & 0xff), ((ip_address.ip.v4 >>  0) & 0x0ff)) );

        result = wiced_tcp_connect( &session->tcp_socket, &ip_address, WICED_OTA2_HTTP_PORT, 5000 );
        connect_tries++;
    }
    if ( result != WICED_SUCCESS )
    {
        /* informational */
        wiced_ota2_service_make_error_callback(session, OTA2_SERVICE_SERVER_CONNECT_ERROR);
        goto _connect_fail;
    }
    OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("Connected to %ld.%ld.%ld.%ld : %d !\r\n",
            ((ip_address.ip.v4 >> 24) & 0xff), ((ip_address.ip.v4 >> 16) & 0x0ff),
            ((ip_address.ip.v4 >>  8) & 0xff), ((ip_address.ip.v4 >>  0) & 0x0ff), WICED_OTA2_HTTP_PORT) );

    strcpy(session->last_connected_host_name, host_name);

    wiced_ota2_service_make_callback(session, OTA2_SERVICE_SERVER_CONNECTED, 0);
    return result;

_connect_fail:
    OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("ota2_service_connect() wiced_tcp_connect() failed! %d\r\n", result));
    ota2_service_disconnect(session);
    return WICED_ERROR;
}

/* get the OTA Image file */
/* pass req_length = 4096 (one sector) if you want only the OTA2 header & component headers
 *                          and store in provided buffer (not saved to Staging area)
 * req_length = -1 means get the whole file.
 */

static wiced_result_t wiced_ota2_service_wget_update(wiced_ota2_service_session_t* session, uint8_t *req_buffer, uint32_t req_length)
{
    wiced_result_t      result;
    char                port_name[16] = "\0";
    wiced_packet_t*     reply_packet;
    uint32_t            offset;
    wiced_bool_t        done;
    uint32_t            content_length;
    uint32_t            wait_loop_count;
    http_header_t       length_header, range_header;
    uint32_t            range_start, range_end;
    const char*         hostname = session->host_name;
    const char*         filename = session->file_path;
    wiced_tcp_socket_t* tcp_socket = &session->tcp_socket;

    length_header.name = "Content-Length";
    length_header.value = NULL;

    range_header.name = "bytes";
    range_header.value = NULL;
    range_start = 0;
    range_end = 0;

    sprintf(port_name, ":%d", WICED_OTA2_HTTP_PORT);

    sprintf(session->http_query, ota2_get_request_template, filename, hostname, port_name);

    OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("Send query for OTA file: [%s]\r\n", session->http_query));

    result = wiced_tcp_send_buffer( tcp_socket, session->http_query, (uint16_t)strlen(session->http_query) );
    if ( result != WICED_SUCCESS )
    {
        if (result == WICED_TCPIP_SOCKET_CLOSED)
        {
            OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("wiced_tcp_send_buffer() wiced_tcp_receive() %d socket_closed!\r\n", result));
        }
        else
        {
            OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("ota2_get_OTA_file() wiced_tcp_send_buffer() failed %d [%s]!\r\n", result, session->http_query));
        }

        /* informational */
        wiced_ota2_service_make_error_callback(session, OTA2_SERVICE_SERVER_CONNECT_ERROR);
        return WICED_ERROR;
    }

    reply_packet = NULL;
    content_length = 0;
    result = WICED_SUCCESS;
    done = WICED_FALSE;
    offset = 0;
    wait_loop_count = 0;
    while ((result == WICED_SUCCESS) && (done == WICED_FALSE))
    {
        if (reply_packet != NULL)
        {
            /* free the packet */
            wiced_packet_delete( reply_packet );
        }
        reply_packet = NULL;

        result = wiced_tcp_receive( tcp_socket, &reply_packet, 1000 ); /* short timeout */
        if (result == WICED_TCPIP_TIMEOUT)
        {
            OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("ota2_get_OTA_file() wiced_tcp_receive() %d timeout!\r\n", result));

            if ( wait_loop_count++ < 15)
            {
                result = WICED_SUCCESS; /* so we stay in our loop */
            }
            else
            {
                OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("ota2_get_OTA() wiced_ota2_write_data() Timed out received:%ld of %ld!\r\n",
                                                          offset, content_length));
                session->tcp_timeouts++;
                goto _wget_fail;
            }
        }
        else if (result == WICED_TCPIP_SOCKET_CLOSED)
        {
            OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("ota2_get_OTA_file() wiced_tcp_receive() %d socket_closed!\r\n", result));
            goto _wget_fail;
        }
        else if (reply_packet != NULL)
        {
            /* for this packet */
            uint8_t*            body;
            uint32_t            body_length;
            http_status_code_t  response_code;

            OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("ota2_get_OTA_file() wiced_tcp_receive() result:%d reply_packet:%p\r\n", result, reply_packet));

            /* we got a packet, clear this timeout counter */
            wait_loop_count = 0;

            if (result != WICED_SUCCESS)
            {
                result = WICED_SUCCESS; /* stay in the loop and try again */
                continue;
            }
            body = NULL;
            body_length = 0;

            response_code = 0;
            result = http_process_response( reply_packet, &response_code );
            OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("http_process_response() result:%d response:%d -- continue\r\n ", result, response_code));
            if (result != WICED_SUCCESS )
            {
                OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("HTTP response result:%d code: %d, continue to process (could be no header)\r\n ", result, response_code));
                result = WICED_SUCCESS; /* so we try again */
            }
            else
            {

                if (response_code < 100)
                {
                    /* do nothing here */
                }
                else if (response_code < 200 )
                {
                    /* 1xx (Informational): The request was received, continuing process */
                    continue;
                }
                else if (response_code < 300 )
                {
                    /* 2xx (Successful): The request was successfully received, understood, and accepted */
                }
                else if (response_code < 400 )
                {
                    /* 3xx (Redirection): Further action needs to be taken in order to complete the request */
                    OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("HTTP response code: %d, redirection - code needed to handle this!\r\n ", response_code));
                    result = WICED_ERROR;
                    goto _wget_fail;
                }
                else
                {
                    /* 4xx (Client Error): The request contains bad syntax or cannot be fulfilled */
                    OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("HTTP response code: %d, ERROR!\r\n ", response_code));
                    result = WICED_ERROR;
                    goto _wget_fail;
                }
            }

            if (content_length == 0)
            {
                /* we don't know the size of the transfer yet, try to find it */
                if (http_extract_headers( reply_packet, &length_header, 1) == WICED_SUCCESS)
                {
                    if (length_header.value != NULL)
                    {
                        content_length = atol(length_header.value);
                    }
                }
            }
            if (range_start == 0)
            {
                range_end = 0;
                /* we don't know the size of the transfer yet, try to find it */
                if (http_extract_headers( reply_packet, &range_header, 1) == WICED_SUCCESS)
                {
                    if (range_header.value != NULL)
                    {
                        char *minus;
                        range_start = atol(range_header.value);
                        minus = strchr(range_header.value, '-');
                        if (minus != NULL)
                        {
                            /* skip the minus */
                            minus++;
                            range_end = atol(minus);
                        }
                        OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("HTTP DATA RANGE: %ld - %ld \r\n ", range_start, range_end));
                    }
                }
            }

            result = http_get_body( reply_packet, &body, &body_length );
            if ((result != WICED_SUCCESS) || (body == NULL))
            {
                /* get_body can fail if there is no header, try just getting the packet data */
                uint8_t* packet_data;
                uint16_t packet_data_length;
                uint16_t available_data_length;

                wiced_packet_get_data( reply_packet, 0, &packet_data, &packet_data_length, &available_data_length );

                OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("ota2_get_OTA() http_get_body() failed, just use the data: packet_data:%p packet_data_length:%d available_data_length:%d\r\n", packet_data, packet_data_length, available_data_length));

                if ((packet_data != NULL) && (available_data_length != 0) && (available_data_length <= packet_data_length))
                {
                    body = packet_data;
                    body_length = available_data_length;
                    result = WICED_SUCCESS;
                }
            }

            /* if we got data, save it */
            if ((body != NULL) && (body_length > 0))
            {
                wiced_result_t  cb_result;
                uint32_t        percent_done = 0;

                if ((offset > 0) && (content_length != 0))
                {
                    /* offset -2 so we never hit 100% while in this loop - it is communicated after we are finished */
                    percent_done = (((offset - 2)* 100) / content_length);
                }

#ifdef SHOW_BAR_GRAPH
                /* only print it if we are writing to FLASH. writing to a requested buffer is quick */
                if (req_buffer == NULL)
                {
                    char            bar_graph[BAR_GRAPH_LENGTH] = {0};
                    uint32_t        vert_bar;

                    vert_bar = 0;
                    if (content_length != 0)
                    {
                        vert_bar = (offset * (BAR_GRAPH_LENGTH - 2)) / content_length;
                    }

                    bar_graph[0] = '|';
                    bar_graph[BAR_GRAPH_LENGTH - 2] = '|';
                    memset( &bar_graph[1], '-', (BAR_GRAPH_LENGTH - 3));
                    bar_graph[vert_bar] = '|';
                    bar_graph[BAR_GRAPH_LENGTH - 1] = '\0';

                   OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("%s (%ld%%)\r", bar_graph, percent_done));
                }
#endif

                OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("Got data! offset:%ld body:%p length:%ld\r\n", offset, body, body_length));

                if (req_buffer != NULL)
                {
                    uint32_t chunk_size = body_length;
                    /* only want header read - don't save to staging area */
                    if ((offset + chunk_size) > req_length )
                    {
                        chunk_size = req_length - offset;
                    }

                    OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("ota2_get_update_header() header_buffer:%p size:0x%x (%d)\r\n", session->header_buffer, sizeof(session->header_buffer), sizeof(session->header_buffer)));
                    OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("                                  dest:%p offset:%ld chunk:%ld size:%ld end:%p! \r\n", req_buffer, offset, chunk_size, req_length, &req_buffer[offset + chunk_size]));

                    memcpy(&req_buffer[offset], body, chunk_size);
                    offset += chunk_size;
                    if (offset >= req_length)
                    {
                        /* normal finish to this request */
                        result = WICED_SUCCESS;
                        goto _wget_fail;
                    }

                    if ((offset > 0) && (content_length != 0))
                    {
                        /* offset -2 so we never hit 100% while in this loop - it is communicated after we are finished */
                        percent_done = (((offset - 2) * 100) / content_length);
                    }

                    cb_result = wiced_ota2_service_make_callback(session, OTA2_SERVICE_DOWNLOAD_STATUS, percent_done );
                    if (cb_result != WICED_SUCCESS)
                    {
                        /* application said to stop the download */
                        /* drop out of loop */
                        done = WICED_TRUE;
                    }
                }
                else
                {
                    OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("writing! offset:%ld body:%p length:%ld\r\n", offset, body, body_length));

                    result = wiced_ota2_image_write_data(body, offset, body_length);
                    if (result == WICED_SUCCESS)
                    {
                        wiced_result_t  cb_result;
                        offset += body_length;

                        if ((offset > 0) && (content_length != 0))
                        {
                            /* offset -2 so we never hit 100% while in this loop - it is communicated after we are finished */
                            percent_done = (((offset - 2) * 100) / content_length);
                        }
                        cb_result = wiced_ota2_service_make_callback(session, OTA2_SERVICE_DOWNLOAD_STATUS, percent_done );
                        if (cb_result != WICED_SUCCESS)
                        {
                            /* application said to stop the download */
                            /* drop out of loop */
                            done = WICED_TRUE;
                        }
                    }
                }
            }
        } /* reply packet != NULL */

        if ((content_length != 0) && (offset >= content_length))
        {
            wiced_ota2_image_update_staged_status(WICED_OTA2_IMAGE_DOWNLOAD_COMPLETE);
            OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("ota2_get_OTA() Finished %ld >= %ld !\r\n", offset, content_length));
            done = WICED_TRUE;
        }

    } /* while result == success && done == WICED_FALSE */

    if (reply_packet != NULL)
    {
        /* free the packet */
        wiced_packet_delete( reply_packet );
    }
    reply_packet = NULL;

    if ((content_length != 0) && (offset < content_length))
    {
        wiced_ota2_image_update_staged_status(WICED_OTA2_IMAGE_INVALID);
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("ota2_get_OTA() FAILED %ld < %ld!\r\n", offset, content_length ));
        result = WICED_ERROR;
    }

_wget_fail:
    if (reply_packet != NULL)
    {
        /* free the packet */
        wiced_packet_delete( reply_packet );
    }
    reply_packet = NULL;

    if ((result == WICED_SUCCESS) && (content_length != 0) && (offset >= content_length))
    {
        wiced_ota2_service_make_callback(session, OTA2_SERVICE_DOWNLOAD_STATUS, 100);
    }
    OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("\r\n ota2_get_OTA_file() Exiting %d\r\n", result));

    return result;

}

wiced_result_t wiced_ota2_service_get_the_update(wiced_ota2_service_session_t*session)
{
    wiced_time_t    curr_time;
    wiced_time_t    done_time;
    wiced_result_t  update_result = WICED_ERROR;

    /* this service will perform the download of the update */
    wiced_time_get_time( &curr_time );
    OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("wiced_ota2_service_get_the_update() start %ld!\r\n", curr_time));

    session->attempted_updates++;

    /* connect to alternate AP, if provided */
    if (wiced_ota2_service_network_switch_to_ota2_ap( session ) != WICED_SUCCESS)
    {
        wiced_ota2_service_make_error_callback(session, OTA2_SERVICE_AP_CONNECT_ERROR);
        session->ota2_ap_failures++;
        goto _check_update_fail;
    }

    /* informational */
    wiced_ota2_service_make_callback(session, OTA2_SERVICE_AP_CONNECTED, 0);

    /* connect to the server */
    if ( ota2_service_connect(session) != WICED_SUCCESS)
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("wiced_ota2_service_get_the_update() ota2_service_connect() failed!\r\n"));
        wiced_ota2_service_make_error_callback(session, OTA2_SERVICE_SERVER_CONNECT_ERROR );
        session->tcp_failures++;
        goto _check_update_connected_fail;
    }

    /* get the header */
    if (wiced_ota2_service_wget_update(session, session->header_buffer, sizeof(session->header_buffer)) != WICED_SUCCESS)
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("wiced_ota2_service_get_the_update() wiced_ota2_service_wget_update(header) failed!\r\n"));
        wiced_ota2_service_make_error_callback(session, OTA2_SERVICE_UPDATE_ERROR );
        session->download_failures++;
        goto _check_update_connected_fail;
    }

    /* ask the application if it wants to do the update */
    if (wiced_ota2_service_make_callback(session, OTA2_SERVICE_UPDATE_AVAILABLE, (uint32_t)session->header_buffer) != WICED_SUCCESS)
    {
        /* application says do not download, this is not a failure */
        update_result = WICED_SUCCESS;
        goto _check_update_connected_fail;
    }

    /* disconnect and re-connect to reset the file pointer */
    ota2_service_disconnect(session);

    if ( ota2_service_connect(session) != WICED_SUCCESS)
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("wiced_ota2_service_get_the_update() part2: ota2_service_connect() failed!\r\n"));
        wiced_ota2_service_make_error_callback(session, OTA2_SERVICE_SERVER_CONNECT_ERROR );
        session->tcp_failures++;
        goto _check_update_connected_fail;
    }

    /* get the OTA2 file */
    if (wiced_ota2_service_wget_update(session, NULL, 0) != WICED_SUCCESS)
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("wiced_ota2_service_get_the_update() wiced_ota2_service_wget_update(file) failed!\r\n"));
        wiced_ota2_service_make_error_callback(session, OTA2_SERVICE_UPDATE_ERROR );
        session->download_failures++;
        goto _check_update_connected_fail;
    }

    /* we done good ! */
    update_result = WICED_SUCCESS;

    /* ask the application if it wants to do the update automatically on next boot */
    if ( ((session->cb_function == NULL) && (session->auto_update != 0)) ||
         (wiced_ota2_service_make_callback(session, OTA2_SERVICE_PERFORM_UPDATE, 0) == WICED_SUCCESS) )
    {
        /* automatically update on next boot */
        OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("Set  WICED_OTA2_IMAGE_EXTRACT_ON_NEXT_BOOT!\r\n"));
        update_result = wiced_ota2_image_update_staged_status(WICED_OTA2_IMAGE_EXTRACT_ON_NEXT_BOOT);
    }
    else
    {
        /* mark staging area download as complete, but not extract on next reboot */
        OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("set WICED_OTA2_IMAGE_DOWNLOAD_COMPLETE (do not automatically update on reboot)!\r\n"));
        update_result = wiced_ota2_image_update_staged_status(WICED_OTA2_IMAGE_DOWNLOAD_COMPLETE);
    }

    wiced_time_get_time( &done_time );
    OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("wiced_ota2_service_get_the_update() end %ld - start %ld = %ld ms!\r\n", done_time, curr_time, done_time - curr_time));

_check_update_connected_fail:
    ota2_service_disconnect(session);

_check_update_fail:

    if (wiced_ota2_service_network_switch_to_default_ap( session ) != WICED_SUCCESS)
    {
        session->default_ap_failures++;
        wiced_ota2_service_make_error_callback(session, OTA2_SERVICE_AP_CONNECT_ERROR);
    }

    if (update_result == WICED_SUCCESS)
    {
        session->successful_updates++;
    }
    else
    {
        session->failed_updates++;
        wiced_rtos_set_event_flags(&session->ota2_worker_flags, OTA2_EVENT_WORKER_START_RETRY_TIMER);
    }

    /* this update attempt is done */
    wiced_ota2_service_make_callback(session, OTA2_SERVICE_UPDATE_ENDED, 0 );

    return update_result;
}
/******************************************************
 *               Internal Function Definitions
 ******************************************************/

void wiced_ota2_worker_thread(uint32_t arg)
{
    wiced_result_t  result;
    wiced_time_t   curr_time;
    wiced_ota2_service_session_t*   session;

    wiced_assert("wiced_OTA2_worker_thread() ARG == NULL!", (arg != 0));
    session = (wiced_ota2_service_session_t*)arg;

    /* init our mutex */
    result = wiced_rtos_init_mutex( &session->ota2_mutex );
    wiced_assert("wiced_rtos_init_mutex() failed!", (result == WICED_SUCCESS));

    /* init our signal flags */
    result = wiced_rtos_init_event_flags(&session->ota2_worker_flags);
    wiced_assert("wiced_rtos_init_event_flags(&session->ota2_worker_flags) failed!", (result == WICED_SUCCESS));

    while(1)
    {

        wiced_result_t                  result = WICED_SUCCESS;
        wiced_result_t                  cb_result = WICED_SUCCESS;
        uint32_t                        events;

        events = 0;
        result = wiced_rtos_wait_for_event_flags(&session->ota2_worker_flags, OTA2_EVENT_WORKER_THREAD_EVENTS, &events,
                                                 WICED_TRUE, WAIT_FOR_ANY_EVENT, WICED_OTA2_WORKER_FLAGS_TIMEOUT);

        if (result != WICED_SUCCESS)
        {
            continue;
        }

        if (events & OTA2_EVENT_WORKER_THREAD_SHUTDOWN_NOW)
        {
            ota2_service_stop_timer(session);
            ota2_service_stop_retry_timer(session);
            break;
        }

        wiced_time_get_time( &curr_time );

        if (events & OTA2_EVENT_WORKER_PAUSE)
        {
            ota2_service_stop_retry_timer(session);

            wiced_time_get_utc_time_ms( &session->session_paused_time );
            ota2_service_set_state(session, OTA2_SERVICE_STATE_PAUSED);

            OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("OTA2: PAUSED \r\n"));
        }


        if (events & OTA2_EVENT_WORKER_UNPAUSE)
        {
            /* kick off an event timer now, if needed */
            events |= OTA2_EVENT_WORKER_START_NEXT_TIMER;
            OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("OTA2: UNPAUSED \r\n"));
            ota2_service_set_state(session, OTA2_SERVICE_STATE_NONE);
            session->session_paused_time = 0;
        }

        /* if we are paused, don't do anything! */
        if (ota2_service_get_state(session) == OTA2_SERVICE_STATE_PAUSED)
        {
            ota2_service_stop_timer(session);
            ota2_service_stop_retry_timer(session);
            continue;
        }

        if (events & OTA2_EVENT_WORKER_START_INITIAL_TIMER)
        {
            ota2_service_stop_timer(session);

            if (session->initial_check_interval > 0)
            {
                /* initial time is always off of current time */
                OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("OTA2: START_INITIAL_TIMER time: %ld initial interval: %ld sec!\r\n", curr_time, session->initial_check_interval));
                ota2_service_start_timer(session, (session->initial_check_interval * MILLISECONDS_PER_SECOND));
                ota2_service_set_state(session, OTA2_SERVICE_STATE_WAITING_FOR_TIMER);

                wiced_time_get_utc_time_ms( &session->session_next_time );
                session->session_next_time += (session->retry_check_interval * MILLISECONDS_PER_SECOND);    /* for info */
            }
            else
            {
                OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("OTA2: DO NOT START_INITIAL_TIMER initial interval: %ld sec!\r\n", session->initial_check_interval));
                session->session_next_time = 0;                                     /* for info */
                ota2_service_set_state(session, OTA2_SERVICE_STATE_NONE);
            }

            session->last_check_time = (curr_time / MILLISECONDS_PER_SECOND);       /* for info */
        }

        if (events & OTA2_EVENT_WORKER_START_NEXT_TIMER)
        {
            uint32_t    curr_seconds = (curr_time / MILLISECONDS_PER_SECOND);
            uint32_t    timer_value = session->check_interval;

            /* should already be stopped, do it for sanity */
            ota2_service_stop_timer(session);

            if (timer_value > 0)
            {
                /* determine next check time based on when last check was made */
                if (curr_seconds > session->last_check_time)
                {
                    if (timer_value > (curr_seconds - session->last_check_time))
                    {
                        timer_value -= (curr_seconds - session->last_check_time);
                    }
                    else
                    {
                        /* we've already passed the check time - set to 1 sec */
                        timer_value = 1;
                    }
                }

                /* sanity check */
                if (timer_value > WICED_OTA2_MAX_INTERVAL_TIME)
                {
                    timer_value = session->check_interval;
                }

                OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("OTA2: START_NEXT_TIMER time: %ld  interval:%ld sec!\r\n", curr_time, timer_value));
                ota2_service_start_timer(session, (timer_value * MILLISECONDS_PER_SECOND));
                ota2_service_set_state(session, OTA2_SERVICE_STATE_WAITING_FOR_TIMER);

                wiced_time_get_utc_time_ms( &session->session_next_time );
                session->session_next_time += (timer_value * MILLISECONDS_PER_SECOND);  /* for info */
            }
            else
            {
                OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("OTA2: DO NOT START_NEXT_TIMER interval:%ld sec!\r\n", session->check_interval));
                session->session_next_time = 0;                                         /* for info */
                ota2_service_set_state(session, OTA2_SERVICE_STATE_NONE);
            }

            session->last_check_time = curr_seconds;           /* for info */
        }

        if (events & OTA2_EVENT_WORKER_START_RETRY_TIMER)
        {
            ota2_service_stop_retry_timer(session);

            if (session->retry_check_interval == 0)
            {
                OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("OTA2: START_RETRY_TIMER retry is 0, don't retry!\r\n"));
                continue;
            }

            OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("OTA2: START_RETRY_TIMER time: %ld retry:%ld sec!\r\n", curr_time, session->retry_check_interval));
            ota2_service_start_retry_timer(session, (session->retry_check_interval * MILLISECONDS_PER_SECOND));
            ota2_service_set_state(session, OTA2_SERVICE_STATE_WAITING_FOR_TIMER);
        }

        if (events & OTA2_EVENT_WORKER_CHECK_FOR_UPDATES)
        {
            /* It is time to check for an update - ask the application, and continue around the loop.
             * Do not try to get the actual update if the Application says to - just set the OTA2_EVENT_WORKER_START_DOWNLOAD flag.
             * This is important because we also want to re-start the check timer so we can stay
             * as accurate as possible.
             */

            OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("OTA2: OTA2_EVENT_WORKER_CHECK_FOR_UPDATES time: %ld!\r\n", curr_time));
            /* ota2_service_stop_timer(session); Do not stop the timer - it will already have the next interval time set */
            ota2_service_stop_retry_timer(session);

            /* save the time we started the last check */
            wiced_time_get_utc_time_ms( &session->session_last_time );

            ota2_service_set_state(session, OTA2_SERVICE_STATE_TIME_TO_CHECK_FOR_UPDATES);

            /* make callback to see what application wants to do
             * if application replied with success, this service will try to get the update
             */
            cb_result = wiced_ota2_service_make_callback(session, OTA2_SERVICE_CHECK_FOR_UPDATE, 0);
            if (cb_result == WICED_SUCCESS)
            {
                wiced_time_get_time( &curr_time );
                OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("OTA2: OTA2_EVENT_WORKER_CHECK_FOR_UPDATES kick off OTA2_EVENT_WORKER_START_DOWNLOAD %ld!\r\n", curr_time));
                /* This Service will do the check for update */
                wiced_rtos_set_event_flags(&session->ota2_worker_flags, OTA2_EVENT_WORKER_START_DOWNLOAD);
            }
            else
            {
                /* if we have a check timer, set the state appropriately */
                if (session->check_interval != 0)
                {
                    ota2_service_set_state(session, OTA2_SERVICE_STATE_WAITING_FOR_TIMER);
                }
                else
                {
                    ota2_service_set_state(session, OTA2_SERVICE_STATE_NONE);
                }

                /* Let Application know this update attempt is over */
                wiced_ota2_service_make_callback(session, OTA2_SERVICE_UPDATE_ENDED, 0 );
            }
        } /* CHECK_FOR_UPDATES */

        if (events & OTA2_EVENT_WORKER_START_DOWNLOAD)
        {
            OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("OTA2: OTA2_EVENT_WORKER_START_DOWNLOAD time: %ld!\r\n", curr_time));
            /* ota2_service_stop_timer(session); Do not stop the timer - it will already have the next interval time set */

            ota2_service_stop_retry_timer(session);

            ota2_service_set_state(session, OTA2_SERVICE_STATE_STARTING_DOWNLOAD);

            if (wiced_ota2_service_get_the_update(session) == WICED_SUCCESS)
            {
                OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("OTA2: wiced_ota2_service_get_the_update() SUCCEEDED!\r\n"));
                ota2_service_set_state(session, OTA2_SERVICE_STATE_DOWNLOAD_DONE);
                if (session->check_interval != 0)
                {
                    ota2_service_set_state(session, OTA2_SERVICE_STATE_WAITING_FOR_TIMER);
                }
                else
                {
                    ota2_service_set_state(session, OTA2_SERVICE_STATE_NONE);
                }
            }
            else
            {
                OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("OTA2: wiced_ota2_service_get_the_update() FAILED!\r\n"));
                ota2_service_set_state(session, OTA2_SERVICE_STATE_DOWNLOAD_DONE);
                if (session->retry_check_interval != 0)
                {
                    ota2_service_set_state(session, OTA2_SERVICE_STATE_WAITING_FOR_TIMER);
                }
                else
                {
                    ota2_service_set_state(session, OTA2_SERVICE_STATE_NONE);
                }
            }
        } /* start download */
    } /* while */

    OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("OTA2: Worker thread shutting down!\r\n"));

    /* let the app know the service has stopped */
    wiced_ota2_service_make_callback(session, OTA2_SERVICE_STOPPED, 0);

    /* don't need a timer anymore */
    ota2_service_stop_timer(session);

    /* destroy signal flags */
    wiced_rtos_deinit_event_flags(&session->ota2_worker_flags);
    wiced_rtos_deinit_mutex( &session->ota2_mutex );

    WICED_END_OF_CURRENT_THREAD();
}



/******************************************************
 *               External Function Definitions
 ******************************************************/

/**
 * Initialize a timed backgound service to check for updates
 *
 * @param[in]  params - initialization parameter struct pointer
 * @param[in]  opaque - pointer returned to application in callback
 *
 * @return - session pointer
 *           NULL indicates error
 */
void*  wiced_ota2_service_init(wiced_ota2_backround_service_params_t *params, void* opaque)
{
    wiced_ota2_service_session_t*    new_session;

    if (params == NULL)
    {
        WPRINT_LIB_ERROR(("OTA2: wiced_ota2_service_init() No params!\r\n"));
        return NULL;
    }

    /* if there is already a session created, don't allow a second one */
    if (g_only_one_session_at_a_time != NULL)
    {
        WPRINT_LIB_ERROR(("OTA2: wiced_ota2_service_init() Session already initialized !\r\n"));
        return NULL;
    }

    /* validate settings */
    /* If there are no time values, let's bail - nothing for us to do ! */
    if (params->initial_check_interval == 0)
    {
        WPRINT_LIB_ERROR(("OTA2: initial_check_interval == 0 !\r\n"));
        return NULL;
    }
    if (params->check_interval == 0)
    {
        WPRINT_LIB_INFO(("OTA2: check_interval == 0 !\r\n"));
    }
    if (params->retry_check_interval == 0)
    {
        WPRINT_LIB_INFO(("OTA2: retry_check_interval == 0 !\r\n"));
    }

    if ((params->host_name == NULL) || (strlen(params->host_name) < 2))
    {
        /* TODO: should we check this at init? */
        WPRINT_LIB_ERROR(("OTA2: BAD host server: %p %s !\r\n", params->host_name, ((params->host_name == NULL) ? "<null>" : params->host_name)));
        return NULL;
    }

    if ((params->file_path == NULL) || (strlen(params->file_path) < 2))
    {
        /* TODO: should we check this at init? */
        WPRINT_LIB_ERROR(("OTA2: BAD file name: %p %s !\r\n", params->file_path, ((params->file_path == NULL) ? "<null>" : params->file_path) ));
        return NULL;
    }

    if ((params->initial_check_interval > WICED_OTA2_MAX_INTERVAL_TIME) ||
        (params->check_interval > WICED_OTA2_MAX_INTERVAL_TIME)         ||
        (params->retry_check_interval > WICED_OTA2_MAX_INTERVAL_TIME) )
    {
        WPRINT_LIB_ERROR(("One of the Check interval values is > %ld\r\n", WICED_OTA2_MAX_INTERVAL_TIME ));
        return NULL;
    }

    new_session = (wiced_ota2_service_session_t*)calloc(1, sizeof(wiced_ota2_service_session_t));
    if( new_session == NULL)
    {
        WPRINT_LIB_ERROR(("OTA2: wiced_ota2_service_init() failed to calloc main service structure !\r\n"));
        return NULL;
    }

    new_session->cb_opaque = opaque;

    new_session->initial_check_interval = params->initial_check_interval;
    new_session->check_interval         = params->check_interval;
    new_session->retry_check_interval   = params->retry_check_interval;
    new_session->auto_update            = params->auto_update;
    new_session->ota2_ap_info           = params->ota2_ap_info;
    new_session->default_ap_info        = params->default_ap_info;
    strlcpy(new_session->host_name, params->host_name, sizeof(new_session->host_name) - 1);
    strlcpy(new_session->file_path, params->file_path, sizeof(new_session->file_path) - 1);

    /* new session struct initialized */
    new_session->tag       = WICED_OTA2_SERVICE_TAG_VALID;

    new_session->log_level = OTA2_LOG_NOTIFY;

    wiced_time_get_utc_time_ms( &new_session->session_start_time );

    /* save the session pointer */
    g_only_one_session_at_a_time = new_session;

    /* mark as inited */
    new_session->ota2_state = OTA2_SERVICE_STATE_INITIALIZED;

    OTA2_LIB_PRINT(new_session, OTA2_LOG_DEBUG, ("OTA2: wiced_ota2_service_init() DONE %p state:%d!\r\n", new_session, new_session->ota2_state));

    return new_session;
}


/**
 * De-initialize the service
 *
 * @param[in]  session - value returned from wiced_ota2_init()
 *
 * @return - WICED_SUCCESS
 *           WICED_ERROR
 *           WICED_BADARG
 */
wiced_result_t  wiced_ota2_service_deinit(void* session_id)
{
    wiced_ota2_service_session_t*   session ;

    session = (wiced_ota2_service_session_t*)session_id;
    if (session == NULL)
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("OTA2: wiced_ota2_service_deinit() Bad arg!\r\n"));
        return WICED_BADARG;
    }
    if (session != g_only_one_session_at_a_time )
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("OTA2: wiced_ota2_service_deinit() Bad arg (session != global)!\r\n"));
        return WICED_BADARG;
    }

    if (session->tag != WICED_OTA2_SERVICE_TAG_VALID)
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("OTA2: wiced_ota2_service_deinit() Already de-inited!\r\n"));
        return WICED_SUCCESS;
    }

    OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("OTA2: wiced_ota2_service_deinit() calling stop!\r\n"));
    wiced_ota2_service_stop(session_id);

    session->tag = WICED_OTA2_SERVICE_TAG_INVALID;

    free(session);
    g_only_one_session_at_a_time = NULL;

    OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("OTA2: wiced_ota2_service_deinit() Done!\r\n"));
    return WICED_SUCCESS;
}

/**
 * Start the service
 *
 * @param[in]  session - value returned from wiced_ota2_init()
 *
 * @return - WICED_SUCCESS
 *           WICED_ERROR
 *           WICED_BADARG
 */
wiced_result_t  wiced_ota2_service_start(void* session_id)
{
    wiced_result_t                  result;
    wiced_ota2_service_session_t*   session ;

    session = (wiced_ota2_service_session_t*)session_id;
    if ((session == NULL) || (session->tag != WICED_OTA2_SERVICE_TAG_VALID) || (session != g_only_one_session_at_a_time ))
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("OTA2: wiced_ota2_service_start() Bad arg (session %p != global %p)!\r\n", session, g_only_one_session_at_a_time));
        return WICED_BADARG;
    }

    OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("OTA2: wiced_ota2_service_start() session:%p session->worker_thread:%p (must be 0x00)\r\n", session, session->ota2_worker_thread_ptr));

    session->ota2_worker_thread_ptr =  NULL;

    /* Start worker thread */
    result = wiced_rtos_create_thread( &session->ota2_worker_thread, WICED_OTA2_WORKER_THREAD_PRIORITY, "OTA2 worker",
                                       wiced_ota2_worker_thread, WICED_OTA2_WORKER_STACK_SIZE, session);
    if (result != WICED_SUCCESS)
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("OTA2: wiced_rtos_create_thread(worker) failed:%d\r\n", result));
        goto _start_error_exit;
    }
    else
    {
        session->ota2_worker_thread_ptr = &session->ota2_worker_thread;
    }

    /* wait a bit */
    wiced_rtos_delay_milliseconds(100);

    /* let the app know we started the service - informational */
    wiced_ota2_service_make_callback(session, OTA2_SERVICE_STARTED, 0);

    /* send a command to start the initial timer */
    OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("wiced_ota2_service_start() signal initial timer\r\n"));
    wiced_rtos_set_event_flags(&session->ota2_worker_flags, OTA2_EVENT_WORKER_START_INITIAL_TIMER);

    OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("OTA2: wiced_ota2_service_start() Done!\r\n"));
    return WICED_SUCCESS;


_start_error_exit:
    OTA2_LIB_PRINT(session, OTA2_LOG_WARNING, ("OTA2: wiced_ota2_service_start() FAILED!\r\n"));
    wiced_ota2_service_stop(session);
    return WICED_ERROR;
}

/**
 * Stop the service
 *
 * @param[in]  session_id - value returned from wiced_ota2_init()
 *
 * @return - WICED_SUCCESS
 *           WICED_ERROR
 *           WICED_BADARG

 */
wiced_result_t wiced_ota2_service_stop(void* session_id)
{
    wiced_ota2_service_session_t* session = (wiced_ota2_service_session_t*)session_id;

    if ((session == NULL) || (session->tag != WICED_OTA2_SERVICE_TAG_VALID) || (session != g_only_one_session_at_a_time ))
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("OTA2: wiced_ota2_service_stop() Bad arg (session %p != global %p)!\r\n", session, g_only_one_session_at_a_time));
        return WICED_BADARG;
    }

    if (  session->ota2_worker_thread_ptr != NULL)
    {
        /* stop the service thread */
        OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("wiced_ota2_service_stop() stopping worker\r\n"));
        wiced_rtos_set_event_flags(&session->ota2_worker_flags, OTA2_EVENT_WORKER_THREAD_SHUTDOWN_NOW);

        wiced_rtos_thread_force_awake( &session->ota2_worker_thread );
        wiced_rtos_thread_join( &session->ota2_worker_thread);
        wiced_rtos_delete_thread( &session->ota2_worker_thread);
        OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("wiced_ota2_service_stop() worker deleted :) \r\n"));
    }
    session->ota2_worker_thread_ptr = NULL;

    OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("wiced_ota2_service_stop() DONE\r\n"));

    return WICED_SUCCESS;
}

/**
 * Register or Un-register a callback function to handle the actual update check
 *
 * @param[in]  session_id  - value returned from wiced_ota2_init()
 * @param[in]  callback - callback function pointer (NULL to disable)
 *
 * @return - WICED_SUCCESS
 *           WICED_ERROR
 *           WICED_BADARG

 */
wiced_result_t  wiced_ota2_service_register_callback(void* session_id, ota2_service_callback update_callback)
{
    wiced_ota2_service_session_t* session = (wiced_ota2_service_session_t*)session_id;
    if ((session == NULL) || (session->tag != WICED_OTA2_SERVICE_TAG_VALID) || (session != g_only_one_session_at_a_time ))
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("OTA2: wiced_ota2_service_register_callback() Bad arg (session %p != global %p)!\r\n", session, g_only_one_session_at_a_time));
        return WICED_BADARG;
    }

    session->cb_function = update_callback;
    return WICED_SUCCESS;
}

/**
 * Force an update check now
 * NOTE: Does not affect the timed checks - this is separate
 * NOTE: Asynchronous, non-blocking
 *
 * @param[in]  session_id - value returned from wiced_ota2_init()
 *
 * @return - WICED_SUCCESS - There was no current update in progress
 *                           Update check started
 *           WICED_ERROR   - No session_id or bad session_id or update in progress
 *           WICED_BADARG
 */
wiced_result_t  wiced_ota2_service_check_for_updates(void* session_id)
{
    wiced_result_t  result;

    wiced_ota2_service_session_t* session = (wiced_ota2_service_session_t*)session_id;

    OTA2_LIB_PRINT(session, OTA2_LOG_DEBUG, ("wiced_ota2_service_check_for_updates()\r\n"));

    if ((session == NULL) || (session->tag != WICED_OTA2_SERVICE_TAG_VALID) || (session != g_only_one_session_at_a_time ))
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("OTA2: wiced_ota2_service_check_for_updates() Bad arg (session %p != global %p)!\r\n", session, g_only_one_session_at_a_time));
        return WICED_ERROR;
    }

    /* Is an update in progress ? */
    if ( session->ota2_worker_thread_ptr != NULL)
    {
        /* try to set a pause flag for current update */
        OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("OTA2: wiced_ota2_service_check_for_updates() pausing: state:%d!\r\n", session->ota2_state));
        if (wiced_ota2_service_pause(session) != WICED_SUCCESS)
        {
            OTA2_LIB_PRINT(session, OTA2_LOG_INFO, ("OTA2: wiced_ota2_service_check_for_updates() update in progress state:%d!\r\n", session->ota2_state));
            return WICED_ERROR;
        }
    }

    /* just do it! */
    result = wiced_ota2_service_get_the_update(session);

    /* unpause the thread */
    if ( session->ota2_worker_thread_ptr != NULL)
    {
        wiced_ota2_service_unpause(session);
    }

    return result;
}

/**
 * Split a URI into host and file_path parts
 *
 * @param[in]  uri           - the URI of the file desired
 * @param[in]  host_buff     - pointer to where the host part of the URI will be stored
 * @param[in]  host_buff_len - length of host_buff
 * @param[in]  path_buff     - pointer to where the path part of the URI will be stored
 * @param[in]  path_buff_len - length of path_buff
 * @param[in]  port          - pointer to store the port number
 *
 * @return - WICED_SUCCESS
 *           WICED_ERROR
 *           WICED_BADARG
*/
wiced_result_t wiced_ota2_service_uri_split(const char* uri, char* host_buff, uint16_t host_buff_len, char* path_buff, uint16_t path_buff_len, uint16_t* port)
{
   const char *uri_start, *host_start, *host_end;
   const char *path_start;
   uint16_t host_len, path_len;

  if ((uri == NULL) || (host_buff == NULL) || (path_buff == NULL) || (port == NULL))
  {
      return WICED_ERROR;
  }

  *port = 0;

  /* drop http:// or htts://"  */
  uri_start = strstr(uri, "http");
  if (uri_start == NULL)
  {
      uri_start = uri;
  }
  if (strncasecmp(uri_start, "http://", 7) == 0)
  {
      uri_start += 7;
  }
  else if (strncasecmp(uri_start, "https://", 8) == 0)
  {
      uri_start += 8;
  }

  memset(host_buff, 0, host_buff_len);
  host_start = uri_start;
  host_len = strlen(host_start);
  host_end = strchr(host_start, ':');
  if (host_end != NULL)
  {
      *port = atoi(host_end + 1);
  }
  else
  {
      host_end = strchr(host_start, '/');
  }

  if (host_end != NULL)
  {
      host_len = host_end - host_start;
  }
  if( host_len > (host_buff_len - 1))
  {
      host_len = host_buff_len - 1;
  }
  memcpy(host_buff, host_start, host_len);

  memset(path_buff, 0, path_buff_len);
  path_start = strchr(host_start, '/');
  if( path_start != NULL)
  {
      path_len = strlen(path_start);
      if( path_len > (path_buff_len - 1))
      {
          path_len = path_buff_len - 1;
      }
      memcpy(path_buff, path_start, path_len);
  }

  return WICED_SUCCESS;
}


wiced_result_t wiced_ota2_service_set_debug_log_level(void* session_id, wiced_ota2_log_level_t new_log_level)
{
    wiced_ota2_service_session_t* session = (wiced_ota2_service_session_t*)session_id;
    if ((session == NULL) || (session->tag != WICED_OTA2_SERVICE_TAG_VALID) || (session != g_only_one_session_at_a_time ))
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("OTA2: wiced_ota2_service_set_debug_log_level() Bad arg (session %p != global %p)!\r\n", session, g_only_one_session_at_a_time));
        return WICED_BADARG;
    }

    session->log_level = new_log_level;

    return WICED_SUCCESS;
}

wiced_result_t wiced_ota2_service_status(void* session_id)
{
    wiced_utc_time_ms_t             utc_time_ms;
    wiced_iso8601_time_t            iso8601_time;
    wiced_ota2_service_session_t*   session;
    char                            time_string[ sizeof(wiced_iso8601_time_t) + 1];
    session = (wiced_ota2_service_session_t*)session_id;
    if ((session == NULL) || (session->tag != WICED_OTA2_SERVICE_TAG_VALID) || (session != g_only_one_session_at_a_time ))
    {
        OTA2_LIB_PRINT(session, OTA2_LOG_ERROR, ("OTA2: wiced_ota2_service_status() Bad arg (session %p != global %p)!\r\n", session, g_only_one_session_at_a_time));
        return WICED_BADARG;
    }

    if (session->ota2_worker_thread_ptr == NULL)
    {
        return WICED_ERROR;
    }

    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("OTA2 Service Info:\r\n"));

    wiced_time_convert_utc_ms_to_iso8601( session->session_start_time, &iso8601_time );
    iso8601_time.T = ' ';
    iso8601_time.decimal = 0;
    memcpy(time_string, &iso8601_time, sizeof(wiced_iso8601_time_t));
    time_string[sizeof(time_string) - 1] = 0;;

    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("    session started time: %s\r\n", time_string));

    wiced_time_convert_utc_ms_to_iso8601( session->session_last_time, &iso8601_time );
    iso8601_time.T = ' ';
    iso8601_time.decimal = 0;
    memcpy(time_string, &iso8601_time, sizeof(wiced_iso8601_time_t));
    time_string[sizeof(time_string) - 1] = 0;;
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("  last update check time: %s\r\n", time_string));

    wiced_time_get_utc_time_ms( &utc_time_ms );
    wiced_time_convert_utc_ms_to_iso8601( utc_time_ms, &iso8601_time );
    iso8601_time.T = ' ';
    iso8601_time.decimal = 0;
    memcpy(time_string, &iso8601_time, sizeof(wiced_iso8601_time_t));
    time_string[sizeof(time_string) - 1] = 0;;
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("            current time: %s\r\n", time_string));

    wiced_time_convert_utc_ms_to_iso8601( session->session_paused_time, &iso8601_time );
    iso8601_time.T = ' ';
    iso8601_time.decimal = 0;
    memcpy(time_string, &iso8601_time, sizeof(wiced_iso8601_time_t));
    time_string[sizeof(time_string) - 1] = 0;;
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("      paused check time: %s\r\n", (session->session_paused_time != 0) ? time_string : " "));

    wiced_time_get_utc_time_ms( &utc_time_ms );
    wiced_time_convert_utc_ms_to_iso8601( session->session_next_time, &iso8601_time );
    iso8601_time.T = ' ';
    iso8601_time.decimal = 0;
    memcpy(time_string, &iso8601_time, sizeof(wiced_iso8601_time_t));
    time_string[sizeof(time_string) - 1] = 0;;
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("  next update check time: %s\r\n", time_string));

    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("                   state: %d\r\n", session->ota2_state));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("            ota2_ap_info: %p\r\n", session->ota2_ap_info));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("         default_ap_info: %p\r\n", session->default_ap_info));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("                     url: %s\r\n", session->host_name));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("               file_name: %s\r\n", session->file_path));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("         last_check_time: %ld sec\r\n", session->last_check_time));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("  initial_check_interval: %ld sec\r\n", session->initial_check_interval));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("          check_interval: %ld sec\r\n", session->check_interval));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("    retry_check_interval: %ld sec\r\n", session->retry_check_interval));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("             auto_update: %s\r\n", (session->auto_update != 0) ? "true" : "false"));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("         last ota2 error: %d\r\n", session->last_error_status));

    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("\r\n"));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("       attempted updates: %d\r\n", session->attempted_updates));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("      successful updates: %d\r\n", session->successful_updates));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("          failed updates: %d\r\n", session->failed_updates));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("            ota2 AP failures: %d\r\n", session->ota2_ap_failures));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("                TCP failures: %d\r\n", session->tcp_failures));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("                    TCP timeouts: %d\r\n", session->tcp_timeouts));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("           download failures: %d\r\n", session->download_failures));
    OTA2_LIB_PRINT(session, OTA2_LOG_NOTIFY, ("     default AP failures: %d\r\n", session->default_ap_failures));

    return WICED_SUCCESS;
}



