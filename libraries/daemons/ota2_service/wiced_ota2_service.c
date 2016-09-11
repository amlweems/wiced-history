/*
 * Copyright 2015, Broadcom Corporation
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
#include "dns.h"
#include "http_stream.h"
#include "wiced_ota2_image.h"
#include "wiced_ota2_service.h"

#define OTA2_LIB_PRINTF(arg)    printf arg

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_OTA2_SERVICE_TAG_VALID        0x61233216
#define WICED_OTA2_SERVICE_TAG_INVALID      0x61DEAD16

#define WICED_OTA2_APP_URI_MAX              1024
#define WICED_OTA2_HTTP_QUERY_SIZE          1024
#define WICED_OTA2_HTTP_FILENAME_SIZE       1024
#define MAX_HTTP_OBJECT_PATH                128

#define HTTP_PORT                           80

#define WICED_OTA2_WORKER_THREAD_PRIORITY           (WICED_DEFAULT_WORKER_PRIORITY)

#define WICED_OTA2_WORKER_STACK_SIZE                (10 * 1024)

#define OTA2_THREAD_SHUTDOWN_WAIT                   (100)   /* wait for a shutdown flag */
#define OTA2_FLAGS_TIMEOUT                          (-1)    /* wait forever             */

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *            to        Enumerations
 ******************************************************/
typedef enum
{
    OTA2_EVENT_WORKER_THREAD_SHUTDOWN       = (1 << 0),

    OTA2_EVENT_WORKER_START_TIMER           = (1 << 1),
    OTA2_EVENT_WORKER_START_RETRY_TIMER     = (1 << 2),

    OTA2_EVENT_WORKER_CHECK_FOR_UPDATES     = (1 << 4),
    OTA2_EVENT_WORKER_CHECK_UPDATE_VALID    = (1 << 5),

    OTA2_EVENT_WORKER_START_DOWNLOAD        = (1 << 8),
    OTA2_EVENT_WORKER_CONTINUE_DOWNLOAD     = (1 << 9),

    OTA2_EVENT_WORKER_THREAD_DONE           = (1 << 15),

} OTA2_EVENTS_T;

#define OTA2_EVENT_WORKER_THREAD_EVENTS  (OTA2_EVENT_WORKER_THREAD_SHUTDOWN | OTA2_EVENT_WORKER_START_TIMER | OTA2_EVENT_WORKER_START_RETRY_TIMER | \
                                          OTA2_EVENT_WORKER_CHECK_FOR_UPDATES | OTA2_EVENT_WORKER_CHECK_UPDATE_VALID |                              \
                                          OTA2_EVENT_WORKER_START_DOWNLOAD | OTA2_EVENT_WORKER_CONTINUE_DOWNLOAD | OTA2_EVENT_WORKER_THREAD_DONE)

/******************************************************
 *                    Structures
 ******************************************************/

/* session structure */
typedef struct wiced_ota2_service_session_s
{
        uint32_t                                tag;

        ota2_service_callback                   cb_function;
        void*                                   cb_opaque;

        wiced_thread_t                          ota2_worker_thread;
        wiced_thread_t*                         ota2_worker_thread_ptr;
        wiced_event_flags_t                     ota2_flags;

        /* connect & download info */
        char                            uri_to_stream[WICED_OTA2_APP_URI_MAX];

        char                            http_query[WICED_OTA2_HTTP_QUERY_SIZE];       /* for building the http query */
        char                            filename[WICED_OTA2_HTTP_FILENAME_SIZE];
        wiced_bool_t                    connect_state;      /* WICED_TRUE when connected to a server */
        wiced_tcp_socket_t              tcp_socket;
        wiced_bool_t                    tcp_socket_created;
        wiced_bool_t                    tcp_packet_pool_created;

        char                            last_connected_host_name[MAX_HTTP_HOST_NAME_SIZE + 1];
        uint16_t                        last_connected_port;

        uint32_t                        check_interval;         /* seconds between update checks                            */
        uint32_t                        retry_check_interval;   /* seconds between re-try if initial contact to
                                                                 * server for update info fails
                                                                 * 0 = wait until next check_interval                       */

        uint8_t                         auto_update;            /* Callback return value over-rides this parameter
                                                                 * Auto-update behavior if no callback registered.
                                                                 *   1 = Service will inform Bootloader to extract
                                                                 *       and update on next power cycle after download
                                                                 *   0 = Service will inform Bootloader that download
                                                                 *       is complete - Bootloader will NOT extract/update
                                                                 *       until user / application requests                  */

        /* timer */
        wiced_bool_t                    retry_in_progress;
        wiced_timer_t                   check_update_timer;
        wiced_utc_time_t                utc_time_when_started_check_update_timer;

} wiced_ota2_service_session_t;

/******************************************************
 *               Variables Definitions
 ******************************************************/
/* template for HTTP GET */
static char ota2_get_request_template[] =
{
    "GET %s/%s HTTP/1.1\r\n"
    "Host: %s \r\n"
    "\r\n"
};

static char g_http_query[WICED_OTA2_HTTP_QUERY_SIZE];

/****************************************************************
 *  HTTP URI connect / disconnect Function Declarations
 ****************************************************************/

static wiced_result_t  ota2_service_check_socket_created(wiced_ota2_service_session_t* session)
{
    wiced_result_t result;
    if (session->tcp_socket_created == WICED_TRUE)
    {
        return WICED_SUCCESS;
    }

    result = wiced_tcp_create_socket( &session->tcp_socket, WICED_STA_INTERFACE );
    if ( result != WICED_SUCCESS )
    {
        OTA2_LIB_PRINTF(("wiced_tcp_create_socket() failed!\r\n"));
        return result;
    }

    /* TODO: bind? */

    session->tcp_socket_created = WICED_TRUE;
    return result;
}

static wiced_result_t  ota2_service_socket_destroy(wiced_ota2_service_session_t* session)
{
    wiced_result_t result;
    if (session->tcp_socket_created == WICED_FALSE)
    {
        return WICED_SUCCESS;
    }

    result = wiced_tcp_delete_socket( &session->tcp_socket );
    if ( result != WICED_SUCCESS )
    {
        OTA2_LIB_PRINTF(("wiced_tcp_delete_socket() failed!\r\n"));
        return result;
    }

    session->tcp_socket_created = WICED_FALSE;
    return result;
}

wiced_result_t ota2_service_tcp_connect_callback(wiced_tcp_socket_t* socket, void* arg )
{
    wiced_ota2_service_session_t*   session;
    session = (wiced_ota2_service_session_t*)arg;
    if ((session == NULL) || (session->tag != WICED_OTA2_SERVICE_TAG_VALID))
    {
        return WICED_ERROR;
    }

    OTA2_LIB_PRINTF(("ota2_service_tcp_connect_callback(%p, %p)!\r\n", socket, arg));

    UNUSED_PARAMETER(session);
    UNUSED_PARAMETER(socket);
    return WICED_SUCCESS;
}

wiced_result_t ota2_service_tcp_receive_callback(wiced_tcp_socket_t* socket, void* arg )
{
    wiced_ota2_service_session_t*   session;
    session = (wiced_ota2_service_session_t*)arg;
    if ((session == NULL) || (session->tag != WICED_OTA2_SERVICE_TAG_VALID))
    {
        return WICED_ERROR;
    }

    OTA2_LIB_PRINTF(("ota2_service_tcp_receive_callback(%p, %p)!\r\n", socket, arg));

    UNUSED_PARAMETER(session);
    UNUSED_PARAMETER(socket);
    return WICED_SUCCESS;
}

wiced_result_t ota2_service_tcp_disconnect_callback(wiced_tcp_socket_t* socket, void* arg )
{
    wiced_ota2_service_session_t*   session;
    session = (wiced_ota2_service_session_t*)arg;
    if ((session == NULL) || (session->tag != WICED_OTA2_SERVICE_TAG_VALID))
    {
        return WICED_ERROR;
    }

    OTA2_LIB_PRINTF(("ota2_service_tcp_disconnect_callback(%p, %p)!\r\n", socket, arg));

    UNUSED_PARAMETER(session);
    UNUSED_PARAMETER(socket);
    return WICED_SUCCESS;
}

void wiced_ota2_check_update_timer_handler( void* arg )
{
    wiced_ota2_service_session_t*      session;

    wiced_assert("wiced_ota2_check_update_timer_handler() ARG == NULL!", (arg != 0));
    session = (wiced_ota2_service_session_t*)arg;

    /* signal the worker thread to check if there is an update available */
    wiced_rtos_set_event_flags(&session->ota2_flags, OTA2_EVENT_WORKER_CHECK_FOR_UPDATES);
}

static wiced_result_t ota2_service_connect(wiced_ota2_service_session_t* session)
{
    wiced_result_t      result = WICED_SUCCESS;
    wiced_ip_address_t  ip_address;
    uint16_t            port = 0;
    uint16_t            connect_tries;

    char                host_name[MAX_HTTP_HOST_NAME_SIZE];
    char                object_path[MAX_HTTP_OBJECT_PATH];

    if (session->connect_state == WICED_TRUE)
    {
        OTA2_LIB_PRINTF(("ota2_service_connect() already connected!\r\n"));
        return WICED_SUCCESS;
    }

    if ( ota2_service_check_socket_created(session) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

//    if (wiced_tcp_register_callbacks( &session->tcp_socket, ota2_service_tcp_connect_callback, ota2_service_tcp_receive_callback, ota2_service_tcp_disconnect_callback, session ) != WICED_SUCCESS)
//    {
//        /* error registering callbacks for the TCP socket */
//        OTA2_LIB_PRINTF(("ota2_service_connect() wiced_tcp_register_callbacks(%s) failed!\r\n", host_name));
//        return WICED_ERROR;
//    }

    strlcpy(host_name, session->uri_to_stream, sizeof(host_name));
    strlcpy(object_path, session->filename, sizeof(object_path));

    port = HTTP_PORT;

    OTA2_LIB_PRINTF(("Connect to: host:%s path:%s port:%d\r\n", host_name, object_path, port) );


    if (isdigit((unsigned char)host_name[0]) && isdigit((unsigned char)host_name[1]) && isdigit((unsigned char)host_name[2])
            && host_name[3] == '.')
    {
        int         ip[4];
        char*       numeral;

        ip_address.version = WICED_IPV4;
        numeral = host_name;
        ip[0] = atoi(numeral);
        numeral = strchr(numeral, '.');
        if( numeral == NULL )
        {
            OTA2_LIB_PRINTF(("ota2_service_connect() parsing URL numerically failed 1!\r\n"));
            return result;

        }
        numeral++;
        ip[1] = atoi(numeral);
        numeral = strchr(numeral, '.');
        if( numeral == NULL )
        {
            OTA2_LIB_PRINTF(("ota2_service_connect() parsing URL numerically failed 2!\r\n"));
            return result;

        }
        numeral++;
        ip[2] = atoi(numeral);
        numeral = strchr(numeral, '.');
        if( numeral == NULL )
        {
            OTA2_LIB_PRINTF(("ota2_service_connect() parsing URL numerically failed 3!\r\n"));
            return result;

        }
        numeral++;
        ip[3] = atoi(numeral);
        numeral = strchr(numeral, '.');

        SET_IPV4_ADDRESS( ip_address, MAKE_IPV4_ADDRESS(ip[0], ip[1], ip[2], ip[3]));

        OTA2_LIB_PRINTF(("Using (%ld.%ld.%ld.%ld)\r\n",
                ((ip_address.ip.v4 >> 24) & 0xff), ((ip_address.ip.v4 >> 16) & 0x0ff),
                ((ip_address.ip.v4 >>  8) & 0xff), ((ip_address.ip.v4 >>  0) & 0x0ff)) );

    }
    else
    {
        OTA2_LIB_PRINTF(("ota2_service_connect() dns_client_hostname_lookup(%s)!\r\n", host_name));
        result =  dns_client_hostname_lookup( host_name, &ip_address, 10000 );
        if (result!= WICED_SUCCESS)
        {
            OTA2_LIB_PRINTF(("ota2_service_connect() dns_client_hostname_lookup(%s) failed!\r\n", host_name));
            return result;
        }
    }


//    OTA2_LIB_PRINTF(("session->tcp_socket.socket.nx_tcp_socket_ip_ptr == NULL!\r\n"));
//    OTA2_LIB_PRINTF(("session->tcp_socket.socket.nx_tcp_socket_ip_ptr->nx_ip_driver_link_up %d\r\n",
//            ((session->tcp_socket.socket.nx_tcp_socket_ip_ptr == NULL) ? 0xFFFF :
//    session->tcp_socket.socket.nx_tcp_socket_ip_ptr->nx_ip_driver_link_up) ));

    connect_tries = 0;
    result = WICED_ERROR;
    while ((connect_tries < 3) && (result != WICED_SUCCESS) )
    {
        OTA2_LIB_PRINTF(("Try %d Connecting to %s:%d  (%ld.%ld.%ld.%ld) !\r\n",
                 connect_tries, host_name, port,
                ((ip_address.ip.v4 >> 24) & 0xff), ((ip_address.ip.v4 >> 16) & 0x0ff),
                ((ip_address.ip.v4 >>  8) & 0xff), ((ip_address.ip.v4 >>  0) & 0x0ff)) );

        result = wiced_tcp_connect( &session->tcp_socket, &ip_address, port, 2000 );
        connect_tries++;;
    }
    if ( result != WICED_SUCCESS )
    {
        OTA2_LIB_PRINTF(("ota2_service_connect() wiced_tcp_connect() failed! %d\r\n", result));
        return result;
    }
    OTA2_LIB_PRINTF(("Connected to %ld.%ld.%ld.%ld : %d !\r\n",
            ((ip_address.ip.v4 >> 24) & 0xff), ((ip_address.ip.v4 >> 16) & 0x0ff),
            ((ip_address.ip.v4 >>  8) & 0xff), ((ip_address.ip.v4 >>  0) & 0x0ff), port) );


    strcpy(session->last_connected_host_name, host_name);
    session->last_connected_port = port;
    session->connect_state = WICED_TRUE;

    //ota2_print_connect_status(session);

    return result;
}

wiced_result_t ot2_service_disconnect(wiced_ota2_service_session_t* session)
{
    wiced_result_t result = WICED_SUCCESS;
    if (wiced_tcp_unregister_callbacks( &session->tcp_socket) != WICED_SUCCESS)
    {
        /* error unregistering callbacks for the TCP socket */
    }

    session->connect_state = WICED_FALSE;

    if (session->tcp_socket_created == WICED_TRUE)
    {
        result = ota2_service_socket_destroy(session);
        OTA2_LIB_PRINTF(("ota2_service_disconnect() ota2_socket_destroy() - %d\r\n", result));
        session->last_connected_port = 0;
    }

    //ota2_print_connect_status(session);
    return result;

}

/* pass req_length = 4096 (one sector) if you want only the OTA2 header & component headers
 *                          and store in provided buffer (not saved to Staging area)
 * req_length = -1 means get the whole file.
 */
wiced_result_t ota2_get_update(const char* hostname, const char* filename, uint16_t port, wiced_tcp_socket_t* tcp_socket,
                               uint8_t *req_buffer, uint32_t req_length)
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

    length_header.name = "Content-Length";
    length_header.value = NULL;

    range_header.name = "bytes";
    range_header.value = NULL;
    range_start = 0;
    range_end = 0;

    sprintf(port_name, ":%d", port);

    OTA2_LIB_PRINTF(("ota2_get_update() hostname:%s\r\n", hostname));
    OTA2_LIB_PRINTF(("                     filename:%s\r\n", filename));
    OTA2_LIB_PRINTF(("                     portname:%s\r\n", port_name));

    sprintf(g_http_query, ota2_get_request_template, hostname, filename, port_name);

    OTA2_LIB_PRINTF(("calling to Send query for OTA file: [%s]\r\n", g_http_query));

    result = wiced_tcp_send_buffer( tcp_socket, g_http_query, (uint16_t)strlen(g_http_query) );
    OTA2_LIB_PRINTF(("ota2_get_update() wiced_tcp_send_buffer() result %d [%s]!\r\n", result, g_http_query));
    if ( result != WICED_SUCCESS )
    {
        if (result == WICED_TCPIP_SOCKET_CLOSED)
        {
            OTA2_LIB_PRINTF(("wiced_tcp_send_buffer() wiced_tcp_receive() %d socket_closed!\r\n", result));
        }
        OTA2_LIB_PRINTF(("ota2_get_update() wiced_tcp_send_buffer() failed %d [%s]!\r\n", result, g_http_query));
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
            OTA2_LIB_PRINTF(("ota2_get_update() wiced_tcp_receive() %d timeout!\r\n", result));
            result = WICED_SUCCESS; /* so we stay in our loop */

            if ( wait_loop_count++ > 100)
            {
                OTA2_LIB_PRINTF(("ota2_get_update() wiced_ota2_write_data() Timed out received:%ld of %ld!\r\n",
                                                          offset, content_length));
                goto _closed_socket;
            }
        }
        else if (result == WICED_TCPIP_SOCKET_CLOSED)
        {
            OTA2_LIB_PRINTF(("ota2_get_update() wiced_tcp_receive() %d socket_closed!\r\n", result));
            goto _closed_socket;
        }
        else if (reply_packet != NULL)
        {
            OTA2_LIB_PRINTF(("ota2_get_update() wiced_tcp_receive() result:%d reply_packet:%p\r\n", result, reply_packet));

            /* for this packet */
            uint8_t*            body;
            uint32_t            body_length;
            http_status_code_t  response_code;

            if (result != WICED_SUCCESS)
            {
                OTA2_LIB_PRINTF(("-------------------- wiced_tcp_receive() result:%d reply_packet:%p\r\n", result, reply_packet));
                result = WICED_SUCCESS; /* stay in the loop and try again */
                continue;
            }
            body = NULL;
            body_length = 0;

            response_code = 0;
            result = http_process_response( reply_packet, &response_code );
            OTA2_LIB_PRINTF(("http_process_response() result:%d response:%d -- continue\r\n ", result, response_code));
            if (result != WICED_SUCCESS )
            {
                OTA2_LIB_PRINTF(("HTTP response result:%d code: %d, continue to process (could be no header)\r\n ", result, response_code));
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

                    OTA2_LIB_PRINTF(("HTTP response code: %d, redirection - code needed to handle this!\r\n ", response_code));
                    return WICED_ERROR;
                }
                else
                {
                    /* 4xx (Client Error): The request contains bad syntax or cannot be fulfilled */
                    OTA2_LIB_PRINTF(("HTTP response code: %d, ERROR!\r\n ", response_code));
                    return WICED_ERROR;
                }
            }

            if (content_length == 0)
            {
                http_extract_headers( reply_packet, &length_header, sizeof(http_header_t) );
                if (length_header.value != NULL)
                {
                    content_length = atol(length_header.value);
                }
            }
            if (range_start == 0)
            {
                range_end = 0;
                http_extract_headers( reply_packet, &range_header, sizeof(http_header_t) );
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
                    OTA2_LIB_PRINTF(("RANGE: %ld - %ld \r\n ", range_start, range_end));
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

                OTA2_LIB_PRINTF(("ota2_get_update() http_get_body() failed, just use the data: packet_data:%p packet_data_length:%d available_data_length:%d\r\n", packet_data, packet_data_length, available_data_length));

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
                OTA2_LIB_PRINTF(("writing! offset:%ld body:%p length:%ld\r\n", offset, body, body_length));

                if (req_buffer != NULL)
                {
                    uint32_t chunk_size = body_length;
                    /* only want header read - don't save to staging area */
                    if ((offset + chunk_size) > req_length )
                    {
                        chunk_size = req_length;
                    }

                    OTA2_LIB_PRINTF(("ota2_get_update() copy just header info offset:%ld size:%ld! \r\n", offset, chunk_size));

                    memcpy(&req_buffer[offset], body, chunk_size);
                    offset += chunk_size;
                    if (offset >= req_length)
                    {
                        goto _closed_socket;
                    }
                }
                else
                {
                    result = wiced_ota2_image_write_data(body, offset, body_length);
                    if (result != WICED_SUCCESS)
                    {
                        OTA2_LIB_PRINTF(("---------------------- ota2_get_update() wiced_ota2_write_data() FAILED!\r\n"));
                    }
                    else
                    {
                        offset += body_length;
                    }
                }
            }
        } /* reply packet != NULL */

        if ((content_length != 0) && (offset >= content_length))
        {
            wiced_ota2_image_update_staged_status(WICED_OTA2_IMAGE_EXTRACT_ON_NEXT_BOOT);
            OTA2_LIB_PRINTF(("ota2_get_update() Finished %ld >= %ld !\r\n", offset, content_length));
            done = WICED_TRUE;
        }

    } /* while result == success && done == WICED_FALSE */

    if ((content_length != 0) && (offset < content_length))
    {
        wiced_ota2_image_update_staged_status(WICED_OTA2_IMAGE_INVALID);
        OTA2_LIB_PRINTF(("ota2_get_update() FAILED %ld < %ld!\r\n", offset, content_length ));
        return WICED_ERROR;
    }

_closed_socket:
    if (reply_packet != NULL)
    {
        /* free the packet */
        wiced_packet_delete( reply_packet );
    }
    reply_packet = NULL;

    OTA2_LIB_PRINTF(("\r\n ota2_get_update() Exiting %d\r\n", result));

    return result;
}


static wiced_result_t ota2_service_validate_update_on_server(wiced_ota2_service_session_t* session)
{
    wiced_result_t  result = WICED_ERROR;
    uint8_t*        header_buffer;

    if (session == NULL)
    {
        return WICED_BADARG;
    }
    header_buffer = malloc(SECTOR_SIZE);
    if (header_buffer == NULL)
    {
        return WICED_OUT_OF_HEAP_SPACE;
    }
    memset(header_buffer, 0x00, SECTOR_SIZE);

        /* try to connect */
    if (ota2_service_connect(session) != WICED_SUCCESS)
    {
        goto _free_header_buffer;
    }

    /* get the header */
    result = ota2_get_update(session->uri_to_stream, session->filename, session->last_connected_port, &session->tcp_socket, header_buffer, SECTOR_SIZE);

_free_header_buffer:
    if (header_buffer != NULL)
    {
        free(header_buffer);
    }

    return result;

}
/* get the OTA Image file */
static wiced_result_t ota2_wget_OTA_file(const char* hostname, const char* filename, uint16_t port, wiced_tcp_socket_t* tcp_socket)
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

    length_header.name = "Content-Length";
    length_header.value = NULL;

    range_header.name = "bytes";
    range_header.value = NULL;
    range_start = 0;
    range_end = 0;

    sprintf(port_name, ":%d", port);

    OTA2_LIB_PRINTF(("ota2_wget_OTA_file() hostname:%s\r\n", hostname));
    OTA2_LIB_PRINTF(("                     filename:%s\r\n", filename));
    OTA2_LIB_PRINTF(("                     portname:%s\r\n", port_name));

    sprintf(g_http_query, ota2_get_request_template, hostname, filename, port_name);

    OTA2_LIB_PRINTF(("calling to Send query for OTA file: [%s]\r\n", g_http_query));

    result = wiced_tcp_send_buffer( tcp_socket, g_http_query, (uint16_t)strlen(g_http_query) );
    OTA2_LIB_PRINTF(("ota2_wget_OTA_file() wiced_tcp_send_buffer() result %d [%s]!\r\n", result, g_http_query));
    if ( result != WICED_SUCCESS )
    {
        if (result == WICED_TCPIP_SOCKET_CLOSED)
        {
            OTA2_LIB_PRINTF(("wiced_tcp_send_buffer() wiced_tcp_receive() %d socket_closed!\r\n", result));
        }
        OTA2_LIB_PRINTF(("ota2_wget_OTA_file() wiced_tcp_send_buffer() failed %d [%s]!\r\n", result, g_http_query));
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
            OTA2_LIB_PRINTF(("ota2_wget_OTA_file() wiced_tcp_receive() %d timeout!\r\n", result));
            result = WICED_SUCCESS; /* so we stay in our loop */

            if ( wait_loop_count++ > 100)
            {
                OTA2_LIB_PRINTF(("ota2_get_OTA() wiced_ota2_write_data() Timed out received:%ld of %ld!\r\n",
                                                          offset, content_length));
                goto _closed_socket;
            }
        }
        else if (result == WICED_TCPIP_SOCKET_CLOSED)
        {
            OTA2_LIB_PRINTF(("ota2_wget_OTA_file() wiced_tcp_receive() %d socket_closed!\r\n", result));
            goto _closed_socket;
        }
        else if (reply_packet != NULL)
        {
            OTA2_LIB_PRINTF(("ota2_wget_OTA_file() wiced_tcp_receive() result:%d reply_packet:%p\r\n", result, reply_packet));

            /* for this packet */
            uint8_t*            body;
            uint32_t            body_length;
            http_status_code_t  response_code;

            if (result != WICED_SUCCESS)
            {
                OTA2_LIB_PRINTF(("-------------------- wiced_tcp_receive() result:%d reply_packet:%p\r\n", result, reply_packet));
                result = WICED_SUCCESS; /* stay in the loop and try again */
                continue;
            }
            body = NULL;
            body_length = 0;

            response_code = 0;
            result = http_process_response( reply_packet, &response_code );
            OTA2_LIB_PRINTF(("http_process_response() result:%d response:%d -- continue\r\n ", result, response_code));
            if (result != WICED_SUCCESS )
            {
                OTA2_LIB_PRINTF(("HTTP response result:%d code: %d, continue to process (could be no header)\r\n ", result, response_code));
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

                    OTA2_LIB_PRINTF(("HTTP response code: %d, redirection - code needed to handle this!\r\n ", response_code));
                    return WICED_ERROR;
                }
                else
                {
                    /* 4xx (Client Error): The request contains bad syntax or cannot be fulfilled */
                    OTA2_LIB_PRINTF(("HTTP response code: %d, ERROR!\r\n ", response_code));
                    return WICED_ERROR;
                }
            }

            if (content_length == 0)
            {
                http_extract_headers( reply_packet, &length_header, sizeof(http_header_t) );
                if (length_header.value != NULL)
                {
                    content_length = atol(length_header.value);
                }
            }
            if (range_start == 0)
            {
                range_end = 0;
                http_extract_headers( reply_packet, &range_header, sizeof(http_header_t) );
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
                    OTA2_LIB_PRINTF(("RANGE: %ld - %ld \r\n ", range_start, range_end));
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

                OTA2_LIB_PRINTF(("ota2_get_OTA() http_get_body() failed, just use the data: packet_data:%p packet_data_length:%d available_data_length:%d\r\n", packet_data, packet_data_length, available_data_length));

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
#define BAR_GRAPH_LENGTH 48
                char            bar_graph[BAR_GRAPH_LENGTH] = {0};
                uint64_t        vert_bar;

                vert_bar = 0;
                if (content_length != 0)
                {
                    vert_bar = (offset * (BAR_GRAPH_LENGTH -2)) / content_length;
                }

                bar_graph[0] = '|';
                bar_graph[BAR_GRAPH_LENGTH - 2] = '|';
                memset( &bar_graph[1], '-', (BAR_GRAPH_LENGTH - 3));
                bar_graph[vert_bar] = '|';
                bar_graph[BAR_GRAPH_LENGTH - 1] = '\0';

                OTA2_LIB_PRINTF(("%s\r", bar_graph));
                OTA2_LIB_PRINTF(("writing! offset:%ld body:%p length:%ld\r\n", offset, body, body_length));

                result = wiced_ota2_image_write_data(body, offset, body_length);
                if (result != WICED_SUCCESS)
                {
                    OTA2_LIB_PRINTF(("---------------------- ota2_get_OTA() wiced_ota2_write_data() FAILED!\r\n"));
                }
                else
                {
                    offset += body_length;
                }
            }
        } /* reply packet != NULL */

        if ((content_length != 0) && (offset >= content_length))
        {
            wiced_ota2_image_update_staged_status(WICED_OTA2_IMAGE_EXTRACT_ON_NEXT_BOOT);
            OTA2_LIB_PRINTF(("ota2_get_OTA() Finished %ld >= %ld !\r\n", offset, content_length));
            done = WICED_TRUE;
        }

    } /* while result == success && done == WICED_FALSE */

    if ((content_length != 0) && (offset < content_length))
    {
        wiced_ota2_image_update_staged_status(WICED_OTA2_IMAGE_INVALID);
        OTA2_LIB_PRINTF(("ota2_get_OTA() FAILED %ld < %ld!\r\n", offset, content_length ));
        return WICED_ERROR;
    }

_closed_socket:
    if (reply_packet != NULL)
    {
        /* free the packet */
        wiced_packet_delete( reply_packet );
    }
    reply_packet = NULL;

    OTA2_LIB_PRINTF(("\r\n ota2_wget_OTA_file() Exiting %d\r\n", result));

    return result;
}

/******************************************************
 *               Internal Function Definitions
 ******************************************************/

void wiced_ota2_worker_thread(uint32_t arg)
{
    wiced_ota2_service_session_t*   session;

    wiced_assert("wiced_OTA2_worker_thread() ARG == NULL!", (arg != 0));
    session = (wiced_ota2_service_session_t*)arg;

    ota2_service_check_socket_created(session);

    OTA2_LIB_PRINTF(("OTA2: Worker thread Started!\n"));

    while(1)
    {

        wiced_result_t                  result = WICED_SUCCESS;
        wiced_result_t                  cb_result = WICED_SUCCESS;
        uint32_t                        events;

        events = 0;
        result = wiced_rtos_wait_for_event_flags(&session->ota2_flags, OTA2_EVENT_WORKER_THREAD_EVENTS, &events,
                                                 WICED_TRUE, WAIT_FOR_ANY_EVENT, OTA2_FLAGS_TIMEOUT);

        if (result != WICED_SUCCESS)
        {
            continue;
        }

        if (events & OTA2_EVENT_WORKER_THREAD_SHUTDOWN)
        {
            break;
        }

        if (events & OTA2_EVENT_WORKER_START_TIMER)
        {
            OTA2_LIB_PRINTF(("OTA2: OTA2_EVENT_WORKER_START_TIMER!\n"));
            if (wiced_rtos_is_timer_running(&session->check_update_timer) == WICED_SUCCESS )
            {
                wiced_rtos_stop_timer(&session->check_update_timer);
                wiced_rtos_deinit_timer(&session->check_update_timer);
            }

            session->retry_in_progress = WICED_FALSE;

            wiced_rtos_init_timer(&session->check_update_timer,  session->check_interval, wiced_ota2_check_update_timer_handler, session);
            wiced_rtos_start_timer(&session->check_update_timer );
        }

        if (events & OTA2_EVENT_WORKER_START_RETRY_TIMER)
        {
            OTA2_LIB_PRINTF(("OTA2: OTA2_EVENT_WORKER_START_RETRY_TIMER!\n"));
            if (wiced_rtos_is_timer_running(&session->check_update_timer) == WICED_SUCCESS )
            {
                wiced_rtos_stop_timer(&session->check_update_timer);
                wiced_rtos_deinit_timer(&session->check_update_timer);
            }

            session->retry_in_progress = WICED_TRUE;
            wiced_rtos_init_timer(&session->check_update_timer,  session->retry_check_interval, wiced_ota2_check_update_timer_handler, session);
            wiced_rtos_start_timer(&session->check_update_timer );
        }

        if (events & OTA2_EVENT_WORKER_CHECK_FOR_UPDATES)
        {
            OTA2_LIB_PRINTF(("OTA2: OTA2_EVENT_WORKER_CHECK_FOR_UPDATES!\n"));
            /* todo: timer went off or user requested check now - connect and get header of OTA2 image */

            if (wiced_rtos_is_timer_running(&session->check_update_timer) == WICED_SUCCESS )
            {
                wiced_rtos_stop_timer(&session->check_update_timer);
                wiced_rtos_deinit_timer(&session->check_update_timer);
            }

            /* make callback to see what application wants to do */
            cb_result = WICED_SUCCESS;
            if (session->cb_function != NULL)
            {
                cb_result = (session->cb_function)(session, OTA2_SERVICE_CHECK_FOR_UPDATE, 0, session->cb_opaque);
            }

            if (cb_result == WICED_SUCCESS)
            {
                /* This Service will to do the check for update */
                wiced_rtos_set_event_flags(&session->ota2_flags, OTA2_EVENT_WORKER_CHECK_UPDATE_VALID);
            }
        }

        if (events & OTA2_EVENT_WORKER_CHECK_UPDATE_VALID)
        {

            OTA2_LIB_PRINTF(("OTA2: OTA2_EVENT_WORKER_CHECK_UPDATE_VALID!\n"));
            /* connect and get the OTA2 header for an update (if it is there) */
            result = ota2_service_validate_update_on_server(session);
            if (result == WICED_SUCCESS)
            {
                /* ask the application if it wants to do the update */
                cb_result = WICED_SUCCESS;
                if (session->cb_function != NULL)
                {
                    cb_result = (session->cb_function)(session, OTA2_SERVICE_UPDATE_AVAILABLE, 0, session->cb_opaque);
                }

                if (cb_result == WICED_SUCCESS)
                {
                    /* this service will perform the download of the update */
                    wiced_rtos_set_event_flags(&session->ota2_flags, OTA2_EVENT_WORKER_START_DOWNLOAD);
                }
            }
            else
            {
                /* failure to get update - start retry timer ? */
                if (session->retry_in_progress != WICED_TRUE)
                {
                    session->retry_in_progress = WICED_TRUE;
                    wiced_rtos_set_event_flags(&session->ota2_flags, OTA2_EVENT_WORKER_START_RETRY_TIMER);
                }
                else
                {
                    /* we did a retry - fail here */
                    cb_result = WICED_SUCCESS;
                    if (session->cb_function != NULL)
                    {
                        cb_result = (session->cb_function)(session, OTA2_SERVICE_UPDATE_ERROR, 0, session->cb_opaque);
                    }

                    if (cb_result == WICED_SUCCESS)
                    {
                        /* this service will retry immediately */
                        wiced_rtos_set_event_flags(&session->ota2_flags, OTA2_EVENT_WORKER_CHECK_UPDATE_VALID);
                    }
                    else
                    {
                        /* service will try at next interval */
                        session->retry_in_progress = WICED_FALSE;
                        wiced_rtos_set_event_flags(&session->ota2_flags, OTA2_EVENT_WORKER_START_TIMER);
                    }
                }
            }
        }

        if (events & OTA2_EVENT_WORKER_START_DOWNLOAD)
        {
            OTA2_LIB_PRINTF(("OTA2: OTA2_EVENT_WORKER_START_DOWNLOAD!\n"));
            /* TODO: start download of OTA2 image */
        }

        if (events & OTA2_EVENT_WORKER_CONTINUE_DOWNLOAD)
        {
            OTA2_LIB_PRINTF(("OTA2: OTA2_EVENT_WORKER_CONTINUE_DOWNLOAD!\n"));
            /* TODO: continue download of OTA2 image */
        }
    } /* while */

    OTA2_LIB_PRINTF(("OTA2: Worker thread shutting down!\n"));

    ota2_service_socket_destroy(session);

    /* Signal worker thread we are done */
    session->ota2_worker_thread_ptr = NULL;
    wiced_rtos_set_event_flags(&session->ota2_flags, OTA2_EVENT_WORKER_THREAD_DONE);

    OTA2_LIB_PRINTF(("OTA2: Worker thread DONE\n"));
}



/******************************************************
 *               External Function Definitions
 ******************************************************/

/**
 * Initialize a timed backgound service to check for updates
 *
 * @param[in]  session - value returned from wiced_ota2_init()
 *
 * @return - session pointer
 *           NULL indicates error
 */
void*  wiced_ota2_service_init(wiced_ota2_backround_service_params_t *params, void* opaque)
{
    wiced_ota2_service_session_t*    new_session;

    if (params == NULL)
    {
        OTA2_LIB_PRINTF(("OTA2: wiced_ota2_service_init() No params!\n"));
        return NULL;
    }

    new_session = (wiced_ota2_service_session_t*)malloc(sizeof(wiced_ota2_service_session_t));
    if( new_session == NULL)
    {
        OTA2_LIB_PRINTF(("OTA2: wiced_ota2_service_init() failed to malloc structure !\n"));
        return NULL;
    }

    memset(new_session, 0x00, sizeof(wiced_ota2_backround_service_params_t));

    new_session->cb_opaque = opaque;
    new_session->tag = WICED_OTA2_SERVICE_TAG_VALID;

    new_session->check_interval       = params->check_interval;
    new_session->retry_check_interval = params->retry_check_interval;
    new_session->auto_update          = params->auto_update;
    strlcpy(new_session->uri_to_stream, params->url, sizeof(new_session->uri_to_stream));
    strlcpy(new_session->filename, params->file_name, sizeof(new_session->filename));

    OTA2_LIB_PRINTF(("OTA2: wiced_ota2_service_init() DONE %p!\n", new_session));
    OTA2_LIB_PRINTF(("                   url = %s\r\n", new_session->uri_to_stream));
    OTA2_LIB_PRINTF(("             file_name = %s\r\n", new_session->filename));
    OTA2_LIB_PRINTF(("        check_interval = %ld\r\n", new_session->check_interval));
    OTA2_LIB_PRINTF(("  retry_check_interval = %ld\r\n", new_session->retry_check_interval));
    OTA2_LIB_PRINTF(("           auto_update = %d\r\n", new_session->auto_update));

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
        OTA2_LIB_PRINTF(("OTA2: wiced_ota2_service_deinit() Bad arg!\n"));
        return WICED_BADARG;
    }

    if (session->tag != WICED_OTA2_SERVICE_TAG_VALID)
    {
        OTA2_LIB_PRINTF(("OTA2: wiced_ota2_service_deinit() Already de-inited!\n"));
        return WICED_SUCCESS;
    }

    wiced_ota2_service_stop(session_id);

    session->tag = WICED_OTA2_SERVICE_TAG_INVALID;
    free(session);

    OTA2_LIB_PRINTF(("OTA2: wiced_ota2_service_deinit() Done!\n"));
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
    if ((session == NULL) || (session->tag != WICED_OTA2_SERVICE_TAG_VALID))
    {
        OTA2_LIB_PRINTF(("OTA2: wiced_ota2_service_start() Bad arg! %p\n", session));
        return WICED_BADARG;
    }

    /* init our signal flags */
    result = wiced_rtos_init_event_flags(&session->ota2_flags);
    if (result != WICED_SUCCESS)
    {
        OTA2_LIB_PRINTF(("OTA2: init_event_flags() failed:%d\r\n", result));
        goto _start_error_exit;
    }

    /* If there are no time values, let's bail - nothing for us to do ! */
    if (session->check_interval == 0)
    {
        /* TODO: should we check this at init? */
        OTA2_LIB_PRINTF(("OTA2: check_interval == 0 !\r\n"));
        goto _start_error_exit;
    }

    if ((session->uri_to_stream == NULL) || (strlen(session->uri_to_stream) < 4))
    {
        /* TODO: should we check this at init? */
        OTA2_LIB_PRINTF(("OTA2: url %p %s !\r\n", session->uri_to_stream, ((session->uri_to_stream == NULL) ? "<null>" : session->uri_to_stream)));
        goto _start_error_exit;
    }

    if ((session->filename == NULL) || (strlen(session->filename) < 2))
    {
        /* TODO: should we check this at init? */
        OTA2_LIB_PRINTF(("OTA2: file_name %p %s !\r\n", session->filename, ((session->filename == NULL) ? "<null>" : session->filename) ));        goto _start_error_exit;
    }

    /* Start worker thread */
    result = wiced_rtos_create_thread( &session->ota2_worker_thread, WICED_OTA2_WORKER_THREAD_PRIORITY, "OTA2 worker",
                                       wiced_ota2_worker_thread, WICED_OTA2_WORKER_STACK_SIZE, session);
    if (result != WICED_SUCCESS)
    {
        OTA2_LIB_PRINTF(("OTA2: wiced_rtos_create_thread(worker) failed:%d\r\n", result));
        goto _start_error_exit;
    }
    else
    {
        session->ota2_worker_thread_ptr = &session->ota2_worker_thread;
    }

    /* send a command to start the timer */
    OTA2_LIB_PRINTF(("wiced_ota2_service_start() signal to start timer\n"));
    wiced_rtos_set_event_flags(&session->ota2_flags, OTA2_EVENT_WORKER_START_TIMER);


    OTA2_LIB_PRINTF(("OTA2: wiced_ota2_service_start() Done!\n"));
    return WICED_SUCCESS;

_start_error_exit:
    wiced_ota2_service_stop(session);
    return WICED_ERROR; /* error until we write the code! */
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
wiced_result_t  wiced_ota2_service_stop(void* session_id)
{
    uint32_t                      events;
    wiced_ota2_service_session_t* session = (wiced_ota2_service_session_t*)session_id;

    if ((session == NULL) || (session->tag != WICED_OTA2_SERVICE_TAG_VALID))
    {
        OTA2_LIB_PRINTF(("OTA2: wiced_ota2_service_start() Bad arg!\n"));
        return WICED_BADARG;
    }

    /* stop the service thread */
    OTA2_LIB_PRINTF(("wiced_ota2_service_stop() stopping worker\n"));
    wiced_rtos_set_event_flags(&session->ota2_flags, OTA2_EVENT_WORKER_THREAD_SHUTDOWN);

    /* Wait until worker thread shut down */
    events = 0;
    while (session->ota2_worker_thread_ptr != NULL)
    {
        wiced_result_t result;

        result = wiced_rtos_wait_for_event_flags(&session->ota2_flags, OTA2_EVENT_WORKER_THREAD_DONE, &events,
                                                 WICED_TRUE, WAIT_FOR_ANY_EVENT, OTA2_THREAD_SHUTDOWN_WAIT);

        if (result != WICED_SUCCESS)
        {
            continue;
        }

        if (events & OTA2_EVENT_WORKER_THREAD_DONE)
        {
            break;
        }
    }
    OTA2_LIB_PRINTF(("wiced_ota2_service_stop() worker stopped\n"));
    session->ota2_worker_thread_ptr = NULL;
    wiced_rtos_delete_thread(&session->ota2_worker_thread);


    /* destroy signal flags */
    wiced_rtos_deinit_event_flags(&session->ota2_flags);

    return WICED_ERROR;
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
    if ((session == NULL) || (session->tag != WICED_OTA2_SERVICE_TAG_VALID))
    {
        return WICED_BADARG;
    }

    session->cb_function = update_callback;
    return WICED_SUCCESS;
}

/**
 * Force an update check now
 * NOTE: does not affect the timed checks - this is separate
 *
 * @param[in]  session_id - value returned from wiced_ota2_init()
 *
 * @return - WICED_SUCCESS
 *           WICED_ERROR
 *           WICED_BADARG
 */
wiced_result_t  wiced_ota2_service_check_for_updates(void* session_id)
{
    wiced_result_t  result;

    wiced_ota2_service_session_t* session = (wiced_ota2_service_session_t*)session_id;

    result = ota2_service_validate_update_on_server(session);
    if (result == WICED_SUCCESS)
    {
        result = ota2_wget_OTA_file(session->uri_to_stream, session->filename, session->last_connected_port, &session->tcp_socket);
    }
    return result;
}



