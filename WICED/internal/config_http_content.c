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
 *
 */

#include <stdlib.h>
#include <string.h>
#include "wiced.h"
#include "http_server.h"
#include "bootloader_app.h"
#include "wwd_constants.h"
#include <wiced_utilities.h>
#include <resources.h>

/******************************************************
 *                      Macros
 ******************************************************/

#define CMP_MAC( a, b )  (((a[0])==(b[0]))&& \
                          ((a[1])==(b[1]))&& \
                          ((a[2])==(b[2]))&& \
                          ((a[3])==(b[3]))&& \
                          ((a[4])==(b[4]))&& \
                          ((a[5])==(b[5])))

#define NULL_MAC( a )  (((a[0])==0)&& \
                        ((a[1])==0)&& \
                        ((a[2])==0)&& \
                        ((a[3])==0)&& \
                        ((a[4])==0)&& \
                        ((a[5])==0))

/******************************************************
 *                    Constants
 ******************************************************/

#define SSID_FIELD_NAME            "ssid"
#define SECURITY_FIELD_NAME        "at0"
#define CHANNEL_FIELD_NAME         "chan"
#define BSSID_FIELD_NAME           "bssid"
#define PASSPHRASE_FIELD_NAME      "ap0"
#define PIN_FIELD_NAME             "pin"

#define APP_SCRIPT_PT1     "var elem_num = "
#define APP_SCRIPT_PT2     ";\n var labelname = \""
#define APP_SCRIPT_PT3     "\";\n var fieldname  = \"v"
#define APP_SCRIPT_PT4     "\";\n var fieldvalue = \""
#define APP_SCRIPT_PT5     "\";\n"


#define SCAN_SCRIPT_PT1    "var elem_num = "
#define SCAN_SCRIPT_PT2    ";\n var SSID = \""
#define SCAN_SCRIPT_PT3    "\";\n var RSSIstr  = \""
#define SCAN_SCRIPT_PT4    "\";\n var SEC = "
#define SCAN_SCRIPT_PT5    ";\n var CH  = "
#define SCAN_SCRIPT_PT6    ";\n var BSSID  = \""
#define SCAN_SCRIPT_PT7    "\";\n"

/* Signal strength defines (in dBm) */
#define RSSI_VERY_POOR             -85
#define RSSI_POOR                  -70
#define RSSI_GOOD                  -55
#define RSSI_VERY_GOOD             -40
#define RSSI_EXCELLENT             -25
#define RSSI_VERY_POOR_STR         "Very Poor"
#define RSSI_POOR_STR              "Poor"
#define RSSI_GOOD_STR              "Good"
#define RSSI_VERY_GOOD_STR         "Very good"
#define RSSI_EXCELLENT_STR         "Excellent"

#define CAPTIVE_PORTAL_REDIRECT_PAGE \
    "<html><head>" \
    "<meta http-equiv=\"refresh\" content=\"0; url=/config/device_settings.html\">" \
    "</head></html>"

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef struct
{
    wiced_tcp_stream_t* stream;
    wiced_semaphore_t semaphore;
    int result_count;
} process_scan_data_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

static int process_app_settings_page ( const char* url, wiced_tcp_stream_t* socket, void* arg );
static int process_wps_go            ( const char* url, wiced_tcp_stream_t* socket, void* arg );
static int process_scan              ( const char* url, wiced_tcp_stream_t* socket, void* arg );
static int process_connect           ( const char* url, wiced_tcp_stream_t* socket, void* arg );
static int process_config_save       ( const char* url, wiced_tcp_stream_t* stream, void* arg );

/******************************************************
 *               Variables Definitions
 ******************************************************/

/**
 * URL Handler List
 */
START_OF_HTTP_PAGE_DATABASE(config_http_page_database)
    ROOT_HTTP_PAGE_REDIRECT("/config/device_settings.html"),
    { "/config/device_settings.html",    "text/html",                         WICED_DYNAMIC_URL_CONTENT,    .url_content.dynamic_data = {process_app_settings_page,     0 } },
    { "/config/join.html",               "text/html",                         WICED_STATIC_URL_CONTENT,     .url_content.static_data  = {resource_config_DIR_join_html,               sizeof(resource_config_DIR_join_html)-1               } },
    { "/config/scan_page_outer.html",    "text/html",                         WICED_STATIC_URL_CONTENT,     .url_content.static_data  = {resource_config_DIR_scan_page_outer_html,    sizeof(resource_config_DIR_scan_page_outer_html)-1    } },
    { "/config/wps_pbc.html",            "text/html",                         WICED_STATIC_URL_CONTENT,     .url_content.static_data  = {resource_config_DIR_wps_pbc_html,            sizeof(resource_config_DIR_wps_pbc_html)-1            } },
    { "/config/wps_pin.html",            "text/html",                         WICED_STATIC_URL_CONTENT,     .url_content.static_data  = {resource_config_DIR_wps_pin_html,            sizeof(resource_config_DIR_wps_pin_html)-1            } },
    { "/scripts/general_ajax_script.js", "application/javascript",            WICED_STATIC_URL_CONTENT,     .url_content.static_data  = {resource_scripts_DIR_general_ajax_script_js, sizeof(resource_scripts_DIR_general_ajax_script_js)-1 } },
    { "/scripts/wpad.dat",               "application/x-ns-proxy-autoconfig", WICED_STATIC_URL_CONTENT,     .url_content.static_data  = {resource_scripts_DIR_wpad_dat,               sizeof(resource_scripts_DIR_wpad_dat)-1               } },
    { "/images/wps_icon.png",            "image/png",                         WICED_STATIC_URL_CONTENT,     .url_content.static_data  = {resource_images_DIR_wps_icon_png,            sizeof(resource_images_DIR_wps_icon_png)              } },
    { "/images/scan_icon.png",           "image/png",                         WICED_STATIC_URL_CONTENT,     .url_content.static_data  = {resource_images_DIR_scan_icon_png,           sizeof(resource_images_DIR_scan_icon_png)             } },
    { "/images/favicon.ico",             "image/vnd.microsoft.icon",          WICED_STATIC_URL_CONTENT,     .url_content.static_data  = {resource_images_DIR_favicon_ico,             sizeof(resource_images_DIR_favicon_ico)               } },
    { "/images/brcmlogo.png",            "image/png",                         WICED_STATIC_URL_CONTENT,     .url_content.static_data  = {resource_images_DIR_brcmlogo_png,            sizeof(resource_images_DIR_brcmlogo_png)              } },
    { "/images/brcmlogo_line.png",       "image/png",                         WICED_STATIC_URL_CONTENT,     .url_content.static_data  = {resource_images_DIR_brcmlogo_line_png,       sizeof(resource_images_DIR_brcmlogo_line_png)         } },
    { "/styles/buttons.css",             "text/css",                          WICED_STATIC_URL_CONTENT,     .url_content.static_data  = {resource_styles_DIR_buttons_css,             sizeof(resource_styles_DIR_buttons_css)               } },
    { "/connect",                        "text/html",                         WICED_DYNAMIC_URL_CONTENT,    .url_content.dynamic_data = {process_connect,               0 } },
    { "/wps_go",                         "text/html",                         WICED_DYNAMIC_URL_CONTENT,    .url_content.dynamic_data = {process_wps_go,                0 } },
    { "/config_save",                    "text/html",                         WICED_DYNAMIC_URL_CONTENT,    .url_content.dynamic_data = {process_config_save,           0 } },
    { "/scan_results.html",              "text/html",                         WICED_DYNAMIC_URL_CONTENT,    .url_content.dynamic_data = {process_scan,                  0 } },
    { IOS_CAPTIVE_PORTAL_ADDRESS,        "text/html",                         WICED_STATIC_URL_CONTENT,     .url_content.static_data  = {CAPTIVE_PORTAL_REDIRECT_PAGE, sizeof(CAPTIVE_PORTAL_REDIRECT_PAGE) } },
END_OF_HTTP_PAGE_DATABASE();

extern const configuration_entry_t* app_configuration;
extern wiced_http_server_t*         http_server;
extern wiced_bool_t                 config_use_wps;
extern char                         config_wps_pin[9];

/******************************************************
 *               Function Definitions
 ******************************************************/

int process_app_settings_page( const char* url, wiced_tcp_stream_t* stream, void* arg )
{
    wiced_tcp_stream_write(stream, resource_config_DIR_device_settings_html, sizeof(resource_config_DIR_device_settings_html)-1);
    const configuration_entry_t* config_entry;
    char temp_buf[10];
    const char* end_str_ptr;
    int end_str_size;
    uint32_t utoa_size;
    uint8_t* app_dct = (uint8_t*)bootloader_api->get_app_config_dct();

    /* Write the app configuration table */
    char config_count[2] = {'0','0'};
    if( app_configuration != NULL )
    {
        for (config_entry = app_configuration; config_entry->name != NULL; ++config_entry)
        {

            /* Write the table entry start html direct from resource file */
            switch (config_entry->data_type)
            {
                case CONFIG_STRING_DATA:
                    wiced_tcp_stream_write(stream, resource_config_DIR_device_settings_html_dev_settings_str, sizeof(resource_config_DIR_device_settings_html_dev_settings_str)-1);
                    break;
                case CONFIG_UINT8_DATA:
                case CONFIG_UINT16_DATA:
                case CONFIG_UINT32_DATA:
                    wiced_tcp_stream_write(stream, resource_config_DIR_device_settings_html_dev_settings_int, sizeof(resource_config_DIR_device_settings_html_dev_settings_int)-1);
                    break;
                default:
                    wiced_tcp_stream_write(stream, "error", 5);
                    break;
            }

            /* Output javascript to fill the table entry */

            wiced_tcp_stream_write( stream, APP_SCRIPT_PT1, sizeof(APP_SCRIPT_PT1)-1 );
            wiced_tcp_stream_write( stream, config_count, 2 );
            wiced_tcp_stream_write( stream, APP_SCRIPT_PT2, sizeof(APP_SCRIPT_PT2)-1 );
            wiced_tcp_stream_write( stream, config_entry->name, strlen(config_entry->name) );
            wiced_tcp_stream_write( stream, APP_SCRIPT_PT3, sizeof(APP_SCRIPT_PT3)-1 );
            wiced_tcp_stream_write( stream, config_count, 2 );
            wiced_tcp_stream_write( stream, APP_SCRIPT_PT4, sizeof(APP_SCRIPT_PT4)-1 );

            /* Fill in current value */
            switch (config_entry->data_type)
            {
                case CONFIG_STRING_DATA:
                    wiced_tcp_stream_write(stream, app_dct + config_entry->dct_offset, strlen((char*)(app_dct + config_entry->dct_offset)));
                    end_str_ptr = resource_config_DIR_device_settings_html_dev_settings_str_end;
                    end_str_size = sizeof(resource_config_DIR_device_settings_html_dev_settings_str_end)-1;
                    break;
                case CONFIG_UINT8_DATA:
                    memset(temp_buf, ' ', 3);
                    utoa_size = utoa(*(uint8_t*)(app_dct + config_entry->dct_offset), (char*)temp_buf, 0, 3);
                    wiced_tcp_stream_write(stream, temp_buf, utoa_size);
                    end_str_ptr = resource_config_DIR_device_settings_html_dev_settings_int_end;
                    end_str_size = sizeof(resource_config_DIR_device_settings_html_dev_settings_int_end)-1;
                    break;
                case CONFIG_UINT16_DATA:
                    memset(temp_buf, ' ', 5);
                    utoa_size = utoa(*(uint16_t*)(app_dct + config_entry->dct_offset), (char*)temp_buf, 0, 5);
                    wiced_tcp_stream_write(stream, temp_buf, utoa_size);
                    end_str_ptr = resource_config_DIR_device_settings_html_dev_settings_int_end;
                    end_str_size = sizeof(resource_config_DIR_device_settings_html_dev_settings_int_end)-1;
                    break;
                case CONFIG_UINT32_DATA:
                    memset(temp_buf, ' ', 10);
                    utoa_size = utoa(*(uint32_t*)(app_dct + config_entry->dct_offset), (char*)temp_buf, 0, 10);
                    wiced_tcp_stream_write(stream, temp_buf, utoa_size);
                    end_str_ptr = resource_config_DIR_device_settings_html_dev_settings_int_end;
                    end_str_size = sizeof(resource_config_DIR_device_settings_html_dev_settings_int_end)-1;
                    break;
                default:
                    wiced_tcp_stream_write(stream, "error", 5);
                    end_str_ptr = NULL;
                    end_str_size = 0;
                    break;
            }

            wiced_tcp_stream_write(stream, APP_SCRIPT_PT5, sizeof(APP_SCRIPT_PT5)-1);
            wiced_tcp_stream_write(stream, end_str_ptr, end_str_size);


            if (config_count[1] == '9')
            {
                ++config_count[0];
                config_count[1] = '0';
            }
            else
            {
                ++config_count[1];
            }
        }
    }
    wiced_tcp_stream_write(stream, resource_config_DIR_device_settings_html_dev_settings_bottom, sizeof(resource_config_DIR_device_settings_html_dev_settings_bottom)-1);

    return 0;
}


wiced_result_t scan_handler( wiced_scan_handler_result_t* malloced_scan_result )
{
    process_scan_data_t* scan_data = (process_scan_data_t*)malloced_scan_result->user_data;

    /* Check if scan is finished (Invalid scan result) */
    if (malloced_scan_result->scan_complete != WICED_TRUE)
    {
        char temp_buffer[40];
        int temp_length;

        malloc_transfer_to_curr_thread( malloced_scan_result );

        wiced_tcp_stream_t* stream = scan_data->stream;

        /* Write out the start HTML for the row */
        wiced_tcp_stream_write(stream, resource_config_DIR_scan_results_html_row, sizeof(resource_config_DIR_scan_results_html_row)-1);

        wiced_tcp_stream_write(stream, SCAN_SCRIPT_PT1, sizeof(SCAN_SCRIPT_PT1)-1);
        temp_length = sprintf( temp_buffer, "%d", scan_data->result_count );
        scan_data->result_count++;
        wiced_tcp_stream_write(stream, temp_buffer, temp_length);

        wiced_tcp_stream_write(stream, SCAN_SCRIPT_PT2, sizeof(SCAN_SCRIPT_PT2)-1);
        /* SSID */
        wiced_tcp_stream_write(stream, malloced_scan_result->ap_details.SSID.val, malloced_scan_result->ap_details.SSID.len);
        wiced_tcp_stream_write(stream, SCAN_SCRIPT_PT3, sizeof(SCAN_SCRIPT_PT3)-1);
        /* RSSI */
        if ( malloced_scan_result->ap_details.signal_strength <= RSSI_VERY_POOR )
        {
            wiced_tcp_stream_write( stream, RSSI_VERY_POOR_STR, sizeof( RSSI_VERY_POOR_STR ) - 1 );
        }
        else if ( malloced_scan_result->ap_details.signal_strength <= RSSI_POOR )
        {
            wiced_tcp_stream_write( stream, RSSI_POOR_STR, sizeof( RSSI_POOR_STR ) - 1 );
        }
        else if ( malloced_scan_result->ap_details.signal_strength <= RSSI_GOOD )
        {
            wiced_tcp_stream_write( stream, RSSI_POOR_STR, sizeof( RSSI_POOR_STR ) - 1 );
        }
        else if ( malloced_scan_result->ap_details.signal_strength <= RSSI_VERY_GOOD )
        {
            wiced_tcp_stream_write( stream, RSSI_GOOD_STR, sizeof( RSSI_GOOD_STR ) - 1 );
        }
        else
        {
            wiced_tcp_stream_write( stream, RSSI_EXCELLENT_STR, sizeof( RSSI_EXCELLENT_STR ) - 1 );
        }
        wiced_tcp_stream_write(stream, SCAN_SCRIPT_PT4, sizeof(SCAN_SCRIPT_PT4)-1);

        /* Security */
        temp_length = sprintf( temp_buffer, "%d", malloced_scan_result->ap_details.security );
        wiced_tcp_stream_write(stream, temp_buffer, temp_length);
        wiced_tcp_stream_write(stream, SCAN_SCRIPT_PT5, sizeof(SCAN_SCRIPT_PT5)-1);

        /* Channel */
        temp_length = sprintf( temp_buffer, "%d", malloced_scan_result->ap_details.channel );
        wiced_tcp_stream_write(stream, temp_buffer, temp_length);
        wiced_tcp_stream_write(stream, SCAN_SCRIPT_PT6, sizeof(SCAN_SCRIPT_PT6)-1);

        /* BSSID */
        temp_length = sprintf( temp_buffer, "%02X%02X%02X%02X%02X%02X", malloced_scan_result->ap_details.BSSID.octet[0], malloced_scan_result->ap_details.BSSID.octet[1], malloced_scan_result->ap_details.BSSID.octet[2], malloced_scan_result->ap_details.BSSID.octet[3], malloced_scan_result->ap_details.BSSID.octet[4], malloced_scan_result->ap_details.BSSID.octet[5] );
        wiced_tcp_stream_write(stream, temp_buffer, temp_length);
        wiced_tcp_stream_write(stream, SCAN_SCRIPT_PT7, sizeof(SCAN_SCRIPT_PT7)-1);

        /* Write out the end HTML for the row */
        wiced_tcp_stream_write(stream, resource_config_DIR_scan_results_html_row_end, sizeof(resource_config_DIR_scan_results_html_row_end)-1);

    }
    else
    {
        wiced_rtos_set_semaphore( &scan_data->semaphore );
    }

    free(malloced_scan_result);

    return WICED_SUCCESS;
}


static int process_scan( const char* url, wiced_tcp_stream_t* stream, void* arg )
{
    process_scan_data_t scan_data;

    scan_data.stream = stream;
    scan_data.result_count = 0;

    /* Initialise the semaphore that will tell us when the scan is complete */
    wiced_rtos_init_semaphore(&scan_data.semaphore);

    /* Send the scan results header */
    wiced_tcp_stream_write(stream, resource_config_DIR_scan_results_html, sizeof(resource_config_DIR_scan_results_html)-1);

    /* Start the scan */
    wiced_wifi_scan_networks( scan_handler, &scan_data );

    /* Wait until scan is complete */
    wiced_rtos_get_semaphore(&scan_data.semaphore, WICED_WAIT_FOREVER);

    /* Send the static footer HTML data */
    wiced_tcp_stream_write(stream, resource_config_DIR_scan_results_html_bottom, sizeof( resource_config_DIR_scan_results_html_bottom ) - 1);

    /* Clean up */
    wiced_rtos_deinit_semaphore(&scan_data.semaphore);

    return 0;
}


static int process_wps_go( const char* url, wiced_tcp_stream_t* stream, void* arg )
{
    int params_len = strlen(url);

    /* client has signalled to start client mode via WPS. */
    config_use_wps = WICED_TRUE;

    /* Check if config method is PIN */
    if ( ( strlen( PIN_FIELD_NAME ) + 1 < params_len ) &&
         ( 0 == strncmp( url, PIN_FIELD_NAME "=", strlen( PIN_FIELD_NAME ) + 1 ) ) )
    {
        url += strlen( PIN_FIELD_NAME ) + 1;
        int pinlen = 0;

        /* Find length of pin */
        while ( ( url[pinlen] != '&'    ) &&
                ( url[pinlen] != '\n'   ) &&
                ( url[pinlen] != '\x00' ) &&
                ( params_len > 0 ) )
        {
            pinlen++;
            params_len--;
        }
        memcpy( config_wps_pin, url, pinlen );
        config_wps_pin[pinlen] = '\x00';
    }
    else
    {
        config_wps_pin[0] = '\x00';
    }

    /* Config has been set. Turn off HTTP server */
    wiced_http_server_stop(http_server);
    return 1;
}


/**
 * URL handler for signaling web server shutdown
 *
 * The reception of this web server request indicates that the client wants to
 * start the appliance, after shutting down the access point, DHCP server and web server
 * Decodes the URL parameters into the connection configuration buffer, then signals
 * for the web server to shut down
 *
 * @param  socket  : a handle for the TCP socket over which the data will be sent
 * @param  params     : a byte array containing any parameters included in the URL
 * @param  params_len : size of the params byte array in bytes
 */
static int process_connect( const char* params, wiced_tcp_stream_t* stream, void* arg )
{
    struct
    {
        wiced_bool_t             device_configured;
        wiced_config_ap_entry_t  ap_entry;
    } temp_config;

    memset(&temp_config, 0, sizeof(temp_config));

    /* First, parse AP details */
    while (params[0] == 'a' && params[3] == '=')
    {
        /* Extract the AP index and check validity */
        uint8_t ap_index = params[2]-'0';
        if (ap_index >= CONFIG_AP_LIST_SIZE)
        {
            return -1;
        }

        /* Find the end of the value */
        const char* end_of_value = &params[4];
        while( (*end_of_value != '&') && (*end_of_value != '\x00') && (*end_of_value != '\n') )
        {
            ++end_of_value;
        }

        /* Parse either the SSID or PSK*/
        if (params[1] == 's')
        {
            memcpy( temp_config.ap_entry.details.SSID.val, &params[4], end_of_value - &params[4]);
            temp_config.ap_entry.details.SSID.len = end_of_value - &params[4];
            temp_config.ap_entry.details.SSID.val[temp_config.ap_entry.details.SSID.len] = 0;
        }
        else if (params[1] == 'p')
        {
            temp_config.ap_entry.security_key_length = end_of_value - &params[4];
            memcpy( temp_config.ap_entry.security_key, &params[4], temp_config.ap_entry.security_key_length);
            temp_config.ap_entry.security_key[temp_config.ap_entry.security_key_length] = 0;
        }
        else if (params[1] == 't')
        {
            temp_config.ap_entry.details.security = (wiced_security_t) atoi( &params[4] );
        }
        else
        {
            return -1;
        }
        params = end_of_value + 1;
    }

    /* Save updated config details */
    temp_config.device_configured = WICED_TRUE;
    bootloader_api->write_wifi_config_dct(0, &temp_config, sizeof(temp_config));

    /* Config has been set. Turn off HTTP server */
    wiced_http_server_stop(http_server);
    return 0;
}

static int process_config_save( const char* params, wiced_tcp_stream_t* stream, void* arg )
{
    if ( app_configuration != NULL )
    {
        uint32_t earliest_offset = 0xFFFFFFFF;
        uint32_t end_of_last_offset = 0x0;
        const configuration_entry_t* config_entry;

        /* Calculate how big the app config details are */
        for ( config_entry = app_configuration; config_entry->name != NULL; ++config_entry )
        {
            if ( config_entry->dct_offset < earliest_offset )
            {
                earliest_offset = config_entry->dct_offset;
            }
            if ( config_entry->dct_offset + config_entry->data_size > end_of_last_offset )
            {
                end_of_last_offset = config_entry->dct_offset + config_entry->data_size;
            }
        }

        uint8_t* app_dct = malloc( end_of_last_offset - earliest_offset );
        memcpy( app_dct, (uint8_t*) bootloader_api->get_app_config_dct( ) + earliest_offset, ( end_of_last_offset - earliest_offset ) );
        if ( app_dct != NULL )
        {
            while ( params[0] == 'v' && params[3] == '=' )
            {
                /* Extract the variable index and check validity */
                uint16_t variable_index = ( ( params[1] - '0' ) << 8 ) | ( params[2] - '0' );

                /* Find the end of the value */
                const char* end_of_value = &params[4];
                while ( ( *end_of_value != '&' ) && ( *end_of_value != '\n' ) )
                {
                    ++end_of_value;
                }

                /* Parse param */
                config_entry = &app_configuration[variable_index];
                switch ( config_entry->data_type )
                {
                    case CONFIG_STRING_DATA:
                        memcpy( (uint8_t*) ( app_dct + config_entry->dct_offset ), &params[4], end_of_value - &params[4]);
                        ( (uint8_t*) ( app_dct + config_entry->dct_offset ) )[end_of_value - &params[4]] = 0;
                        break;
                    case CONFIG_UINT8_DATA:
                        *(uint8_t*) ( app_dct + config_entry->dct_offset - earliest_offset ) = (uint8_t) atoi( &params[4] );
                        break;
                    case CONFIG_UINT16_DATA:
                        *(uint16_t*) ( app_dct + config_entry->dct_offset - earliest_offset ) = (uint16_t) atoi( &params[4] );
                        break;
                    case CONFIG_UINT32_DATA:
                        *(uint32_t*) ( app_dct + config_entry->dct_offset - earliest_offset ) = (uint32_t) atoi( &params[4] );
                        break;
                    default:
                        break;
                }

                params = end_of_value + 1;
            }

            /* Write the app DCT */
            bootloader_api->write_app_config_dct( earliest_offset, app_dct, end_of_last_offset - earliest_offset );

            /* Free the temporary buffer */
            free( app_dct );
        }
    }

    #define CONFIG_SAVE_SUCCESS  "Config saved"
    wiced_tcp_stream_write(stream, CONFIG_SAVE_SUCCESS, sizeof(CONFIG_SAVE_SUCCESS)-1);

    return 0;
}

