/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include "command_console_wifi.h"

#include "wwd_wlioctl.h"
#include "wwd_wifi.h"
#include "string.h"
#include "wwd_debug.h"
#include "command_console.h"
#include "wwd_assert.h"
#include "network/wwd_network_interface.h"
#include "stdlib.h"
#include "wwd_management.h"
#include "internal/wwd_sdpcm.h"
#include "internal/wwd_internal.h"
#include "network/wwd_buffer_interface.h"
#include "wiced_management.h"
#include "wiced_crypto.h"
#include "wiced.h"
#include "wiced_security.h"
#include "internal/wiced_internal_api.h"
#include "besl_host.h"
#include "besl_host_interface.h"
#include "certificate.h"
#include "wiced_tls.h"
#include "wiced_utilities.h"
#include "wiced_supplicant.h"
#include "wiced_tls.h"

#ifdef COMMAND_CONSOLE_WPS_ENABLED
#include "command_console_wps.h"
#include "wiced_wps.h"
#include "wwd_events.h"
#include "wps_common.h"
#endif

#ifdef COMMAND_CONSOLE_P2P_ENABLED
#include "p2p_structures.h"
#include "wiced_p2p.h"
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define CHECK_IOCTL_BUFFER( buff )  if ( buff == NULL ) {  wiced_assert("Allocation failed\n", 0 == 1); return ERR_UNKNOWN; }

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX_PASSPHRASE_LEN   ( 64 )
#define MIN_PASSPHRASE_LEN   (  8 )
#define A_SHA_DIGEST_LEN     ( 20 )
#define DOT11_PMK_LEN        ( 32 )

#define DUMP_TX     1
#define DUMP_RX     2
#define DUMP_RATE   4
#define DUMP_AMPDU  8

/******************************************************
 *               Static Function Declarations
 ******************************************************/

static wiced_result_t scan_result_handler( wiced_scan_handler_result_t* malloced_scan_result );
static int            wifi_join_adhoc(char* ssid, uint8_t ssid_length, wiced_security_t auth_type, uint8_t* security_key, uint16_t key_length, char* channel, char* ip, char* netmask, char* gateway);
static int            wifi_join_specific(char* ssid, uint8_t ssid_length, wiced_security_t auth_type, uint8_t* security_key, uint16_t key_length, char* bssid, char* channel, char* ip, char* netmask, char* gateway);
static void           ac_params_print( const wiced_edcf_ac_param_t *acp, const int *priority );
static eap_type_t     str_to_enterprise_security_type ( char* arg );
static void           analyse_failed_join_result( wiced_result_t join_result );
static int            parse_country_spec( char *spec, int8_t *ccode, int32_t *regrev );
static wiced_result_t get_bss_info(wl_bss_info_t *bi);

/******************************************************
 *               Variable Definitions
 ******************************************************/

static char last_joined_ssid[SSID_NAME_SIZE+1] = ""; /* 32 characters + terminating null */
char last_started_ssid[SSID_NAME_SIZE+1] = "";       /* 32 characters + terminating null */
static char last_soft_ap_passphrase[MAX_PASSPHRASE_LEN+1] = "";
static int record_count;
static wiced_semaphore_t scan_semaphore;
static const wiced_ip_setting_t ap_ip_settings =
{
    INITIALISER_IPV4_ADDRESS( .ip_address, MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
    INITIALISER_IPV4_ADDRESS( .netmask,    MAKE_IPV4_ADDRESS( 255,255,255,  0 ) ),
    INITIALISER_IPV4_ADDRESS( .gateway,    MAKE_IPV4_ADDRESS( 192,168,  0,  1 ) ),
};

#ifdef COMMAND_CONSOLE_P2P_ENABLED
extern p2p_workspace_t p2p_workspace;
#endif

static wiced_tls_session_t tls_session = { 0 };

/* RM Enable Capabilities */
static radio_resource_management_capability_debug_msg_t rrm_msg[] =
{
    { DOT11_RRM_CAP_LINK,    "Link_Measurement" },                /* bit0 */
    { DOT11_RRM_CAP_NEIGHBOR_REPORT, "Neighbor_Report" },         /* bit1 */
    { DOT11_RRM_CAP_PARALLEL,    "Parallel_Measurement" },        /* bit2 */
    { DOT11_RRM_CAP_REPEATED,    "Repeated_Measurement" },        /* bit3 */
    { DOT11_RRM_CAP_BCN_PASSIVE, "Beacon_Passive" },          /* bit4 */
    { DOT11_RRM_CAP_BCN_ACTIVE,  "Beacon_Active" },           /* bit5 */
    { DOT11_RRM_CAP_BCN_TABLE,   "Beacon_Table" },            /* bit6 */
    { DOT11_RRM_CAP_BCN_REP_COND,    "Beacon_measurement_Reporting_Condition" }, /* bit7 */
    { DOT11_RRM_CAP_FM,  "Frame_Measurement" },               /* bit8 */
    { DOT11_RRM_CAP_CLM, "Channel_load_Measurement" },            /* bit9 */
    { DOT11_RRM_CAP_NHM, "Noise_Histogram_measurement" },         /* bit10 */
    { DOT11_RRM_CAP_SM,  "Statistics_Measurement" },          /* bit11 */
    { DOT11_RRM_CAP_LCIM,    "LCI_Measurement" },             /* bit12 */
    { DOT11_RRM_CAP_LCIA,    "LCI_Azimuth" },                 /* bit13 */
    { DOT11_RRM_CAP_TSCM,    "Tx_Stream_Category_Measurement" },      /* bit14 */
    { DOT11_RRM_CAP_TTSCM,   "Triggered_Tx_stream_Category_Measurement" },    /* bit15 */
    { DOT11_RRM_CAP_AP_CHANREP,  "AP_Channel_Report" },           /* bit16 */
    { DOT11_RRM_CAP_RMMIB,   "RM_MIB" },                  /* bit17 */
    { 18, "unused"},
    { 19, "unused"},
    { 20, "unused"},
    { 21, "unused"},
    { 22, "unused"},
    { 23, "unused"},
    { 24, "unused"},
    { 25, "unused"},
    { 26, "unused"},
    /* bit 18-26, unused */
    { DOT11_RRM_CAP_MPTI,    "Measurement_Pilot_Transmission_Information" },  /* bit27 */
    { DOT11_RRM_CAP_NBRTSFO, "Neighbor_Report_TSF_Offset" },          /* bit28 */
    { DOT11_RRM_CAP_RCPI,    "RCPI_Measurement" },                /* bit29 */
    { DOT11_RRM_CAP_RSNI,    "RSNI_Measurement" },                /* bit30 */
    { DOT11_RRM_CAP_BSSAAD,  "BSS_Average_Access_Delay" },            /* bit31 */
    { DOT11_RRM_CAP_BSSAAC,  "BSS_Available_Admission_Capacity" },        /* bit32 */
    { DOT11_RRM_CAP_AI,  "Antenna_Information" },             /* bit33 */

};

static unsigned int n_phyrates_kb[] = {7200, 14400, 21700, 28900, 43300, 57800, 65000, 72200};

/******************************************************
 *               Function Definitions
 ******************************************************/

/*!
 ******************************************************************************
 * Processes a pair of join auth_type strings.
 *
 * @return  0 for success, otherwise error
 */
int process_join_security( int argc, char* ssid, wiced_security_t auth_type, char* key_string, uint8_t *key_length, uint8_t *wep_key_buffer)
{

    if ( auth_type == WICED_SECURITY_UNKNOWN )
    {
        WPRINT_APP_INFO(( "Error: Invalid security type\n" ));
        return ERR_UNKNOWN;
    }

#ifdef ENABLE_WEP
    if ( auth_type == WICED_SECURITY_WEP_PSK )
    {
        uint8_t temp_key_length = *key_length;
        int a;
        wiced_wep_key_t* temp_wep_key = (wiced_wep_key_t*)wep_key_buffer;
        char temp_string[3];
        temp_string[2] = 0;
        temp_key_length = (uint8_t) strlen(key_string)/2;

        /* Setup WEP key 0 */
        temp_wep_key[0].index = 0;
        temp_wep_key[0].length = temp_key_length;
        for (a = 0; a < temp_wep_key[0].length; ++a)
        {
            uint32_t tmp_val;
            memcpy(temp_string, &key_string[a*2], 2);
            string_to_unsigned( temp_string, 2, &tmp_val, 1 );
            temp_wep_key[0].data[a] = (uint8_t) tmp_val;
        }

        /* Setup WEP keys 1 to 3 */
        memcpy(wep_key_buffer + 1*(2 + temp_key_length), temp_wep_key, (2 + temp_key_length));
        memcpy(wep_key_buffer + 2*(2 + temp_key_length), temp_wep_key, (2 + temp_key_length));
        memcpy(wep_key_buffer + 3*(2 + temp_key_length), temp_wep_key, (2 + temp_key_length));
        wep_key_buffer[1*(2 + temp_key_length)] = 1;
        wep_key_buffer[2*(2 + temp_key_length)] = 2;
        wep_key_buffer[3*(2 + temp_key_length)] = 3;

        temp_key_length = 4*(2 + temp_key_length);
        *key_length = temp_key_length;
    }
    else if ( ( auth_type != WICED_SECURITY_OPEN ) && ( argc < 4 ) )
#else
    if ( ( auth_type != WICED_SECURITY_OPEN ) && ( argc < 4 ) )
#endif /* ENABLE_WEP */
    {
        *key_length = 0;
        WPRINT_APP_INFO(("Error: Missing security key\n" ));
        return ERR_UNKNOWN;
    }
    else
    {
        *key_length = strlen(key_string);
    }

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Joins an access point specified by the provided arguments
 *
 * @return  0 for success, otherwise error
 */

int join( int argc, char* argv[] )
{
    int              result;
    char*            ssid = argv[1];
    wiced_security_t auth_type = str_to_authtype(argv[2]);
    uint8_t*         security_key;
    uint8_t          key_length;
    uint8_t          wep_key_buffer[64] = { 0 };

    if (argc > 7)
    {
        return ERR_TOO_MANY_ARGS;
    }

    if (argc > 4 && argc != 7)
    {
        return ERR_INSUFFICENT_ARGS;
    }
    security_key = (uint8_t*)argv[3];
    result = process_join_security(argc, ssid, auth_type, argv[3], &key_length, wep_key_buffer);
    if (result == ERR_CMD_OK)
    {
        if ( argc == 7 )
        {
            return wifi_join( ssid, strlen(ssid), auth_type, security_key, key_length, argv[4], argv[5], argv[6]);
        }
        else
        {
            return wifi_join( ssid, strlen(ssid), auth_type, security_key, key_length, NULL, NULL, NULL );
        }
    }
    return result;
}

/*!
 ******************************************************************************
 * Joins an access point using enterprise security
 *
 * @return  0 for success, otherwise error
 */

int join_ent( int argc, char* argv[] )
{
    char* ssid = argv[1];
    supplicant_workspace_t supplicant_workspace;
    wiced_security_t auth_type;
    eap_type_t       eap_type;
    /* When using PEAP note:
     * There are two identities that are being passed, outer identity (un encrypted) and inner identity (encrypted).
     * Different servers have different requirements for the outer identity.
     * For windows IAS its something like "wiced@wiced.local: ( where wiced.local in your server's domain )
     * FreeRadius doesn't care much about outer name
     * Cisco must have the outer identity in the user list e.g. "wiced" ( i.e. outer and inner identities must match ).
     *
     * When using EAP-TLS note:
     * If the RADIUS server is configured to check the user name against the certificate common name then eap_identity needs to be the same as the certificate common name.
     * For example:
     * char eap_identity[] = "wifi-user@wifilabs.local";
     */
    char eap_identity[] = "wiced";
    wiced_tls_context_t context;
    wiced_tls_identity_t identity;

    if ( argc < 4 )
    {
        return ERR_INSUFFICENT_ARGS;
    }

    eap_type = str_to_enterprise_security_type(argv[2]);
    if ( eap_type == EAP_TYPE_NONE )
    {
        WPRINT_APP_INFO(("Unknown security type\n" ));
        return ERR_CMD_OK;
    }

    auth_type = str_to_enterprise_authtype(argv[argc-1]);
    if ( auth_type == WICED_SECURITY_UNKNOWN )
    {
        WPRINT_APP_INFO(("Unknown security type\n" ));
        return ERR_CMD_OK;
    }
    if ( ( eap_type == EAP_TYPE_PEAP ) && ( argc < 6 ) )
    {
        return ERR_INSUFFICENT_ARGS;
    }

    if ( eap_type == EAP_TYPE_PEAP )
    {
        wiced_tls_init_identity( &identity, NULL, 0, NULL, 0 );
    }
    else
    {
        wiced_tls_init_identity( &identity, WIFI_USER_PRIVATE_KEY_STRING, (uint32_t)strlen( (char*)WIFI_USER_PRIVATE_KEY_STRING ) , WIFI_USER_CERTIFICATE_STRING, (uint32_t)strlen( (char*)WIFI_USER_CERTIFICATE_STRING ) );
    }

    wiced_tls_init_context( &context, &identity, NULL );

    if ( tls_session.length > 0 )
    {
        memcpy( &context.session, &tls_session, sizeof(wiced_tls_session_t) );
    }
    else
    {
        memset( &context.session, 0, sizeof(wiced_tls_session_t) );
    }

    wiced_tls_init_root_ca_certificates( WIFI_ROOT_CERTIFICATE_STRING, (uint32_t)strlen( (char*)WIFI_ROOT_CERTIFICATE_STRING ) );

    if ( besl_supplicant_init( &supplicant_workspace, eap_type, WWD_STA_INTERFACE ) == BESL_SUCCESS )
    {
        wiced_supplicant_enable_tls( &supplicant_workspace, &context );
        besl_supplicant_set_identity( &supplicant_workspace, eap_identity, strlen( eap_identity ) );
        if ( eap_type == EAP_TYPE_PEAP )
        {
            /* Default for now is MSCHAPV2 */
            supplicant_mschapv2_identity_t mschap_identity;
            char mschap_password[32];

            /* Convert ASCII to UTF16 */
            int i;
            uint8_t*  password = (uint8_t*)argv[4];
            uint8_t*  unicode  = (uint8_t*)mschap_password;

            for ( i = 0; i <= strlen(argv[4]); i++ )
            {
                *unicode++ = *password++;
                *unicode++ = '\0';
            }

            mschap_identity.identity = (uint8_t*)argv[3];
            mschap_identity.identity_length = strlen(argv[3]);

            mschap_identity.password = (uint8_t*)mschap_password;
            mschap_identity.password_length = 2*(i-1);

            besl_supplicant_set_inner_identity( &supplicant_workspace, eap_type, &mschap_identity );

        }
        if ( besl_supplicant_start( &supplicant_workspace ) == BESL_SUCCESS )
        {
            if ( wifi_join( ssid, strlen(ssid), auth_type, NULL, 0, NULL, NULL, NULL ) == ERR_CMD_OK )
            {
                memcpy( &tls_session, &context.session, sizeof(wiced_tls_session_t) );
            }
        }
    }
    else
    {
        WPRINT_APP_INFO( ("Unable to initialize supplicant\n" ) );
    }

    wiced_tls_deinit_context( &context );
    wiced_tls_deinit_root_ca_certificates();
    wiced_tls_deinit_identity( &identity );

    besl_supplicant_deinit( &supplicant_workspace );

    return ERR_CMD_OK;
}

int wifi_join(char* ssid, uint8_t ssid_length, wiced_security_t auth_type, uint8_t* key, uint16_t key_length, char* ip, char* netmask, char* gateway)
{
    wiced_network_config_t      network_config;
    wiced_ip_setting_t*         ip_settings = NULL;
    wiced_ip_setting_t          static_ip_settings;
    platform_dct_wifi_config_t* dct_wifi_config;
    wiced_result_t              result;

    if (ssid_length > SSID_NAME_SIZE)
    {
        return ERR_TOO_LARGE_ARG;
    }

    if (wwd_wifi_is_ready_to_transceive(WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }

    /* Read config */
    wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );

    /* Modify config */
    dct_wifi_config->stored_ap_list[0].details.SSID.length = ssid_length;
    memset( dct_wifi_config->stored_ap_list[0].details.SSID.value, 0, sizeof(dct_wifi_config->stored_ap_list[0].details.SSID.value) );
    memcpy( (char*)dct_wifi_config->stored_ap_list[0].details.SSID.value, ssid, ssid_length );
    dct_wifi_config->stored_ap_list[0].details.security = auth_type;
    if ( ( auth_type & ENTERPRISE_ENABLED ) == 0 )
    {
        /* Save credentials for non-enterprise AP */
        memcpy((char*)dct_wifi_config->stored_ap_list[0].security_key, (char*)key, MAX_PASSPHRASE_LEN);
        dct_wifi_config->stored_ap_list[0].security_key_length = key_length;
    }

    /* Write config */
    wiced_dct_write( (const void*) dct_wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );

    /* Tell the network stack to setup it's interface */
    if (ip == NULL )
    {
        network_config = WICED_USE_EXTERNAL_DHCP_SERVER;
    }
    else
    {
        network_config = WICED_USE_STATIC_IP;
        str_to_ip( ip,      &static_ip_settings.ip_address );
        str_to_ip( netmask, &static_ip_settings.netmask );
        str_to_ip( gateway, &static_ip_settings.gateway );
        ip_settings = &static_ip_settings;
    }

    if ( ( result = wiced_network_up( WICED_STA_INTERFACE, network_config, ip_settings ) ) != WICED_SUCCESS )
    {
        if ( auth_type == WICED_SECURITY_WEP_PSK ) /* Now try shared instead of open authentication */
        {
            dct_wifi_config->stored_ap_list[0].details.security = WICED_SECURITY_WEP_SHARED;
            wiced_dct_write( (const void*) dct_wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
            WPRINT_APP_INFO(("WEP with open authentication failed, trying WEP with shared authentication...\n"));

            if ( wiced_network_up( WICED_STA_INTERFACE, network_config, ip_settings ) != WICED_SUCCESS ) /* Restore old value */
            {
                WPRINT_APP_INFO(("Trying shared wep\n"));
                dct_wifi_config->stored_ap_list[0].details.security = WICED_SECURITY_WEP_PSK;
                wiced_dct_write( (const void*) dct_wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
            }
            else
            {
                wiced_dct_read_unlock( (void*) dct_wifi_config, WICED_TRUE );
                return ERR_CMD_OK;
            }
        }

        analyse_failed_join_result( result );
        wiced_dct_read_unlock( (void*) dct_wifi_config, WICED_TRUE );

        return ERR_UNKNOWN;
    }

    strlcpy( last_joined_ssid, ssid, MIN( sizeof(last_joined_ssid), ssid_length+1 ) );

    wiced_dct_read_unlock( (void*) dct_wifi_config, WICED_TRUE );

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Joins an adhoc network using the provided arguments
 *
 * @return  0 for success, otherwise error
 */

int join_adhoc( int argc, char* argv[] )
{
    int              result;
    char*            ssid = argv[1];
    wiced_security_t auth_type = str_to_authtype(argv[2]);
    uint8_t          key_length = strlen(argv[3]);
    uint8_t          wep_key_buffer[64];
    uint8_t*         security_key = (uint8_t*)argv[3];

    if (argc < 8)
    {
        return ERR_INSUFFICENT_ARGS;
    }

    if (argc > 8)
    {
        return ERR_TOO_MANY_ARGS;
    }

    result = process_join_security(argc, ssid, auth_type, argv[3], &key_length, wep_key_buffer);
    if (result == ERR_CMD_OK)
        return wifi_join_adhoc( ssid, strlen(ssid), auth_type, security_key,
                key_length, argv[4], argv[5], argv[6], argv[7]);
    else
        return result;
}

/*!
 ******************************************************************************
 * Joins a specific access point using the provided arguments
 *
 * @return  0 for success, otherwise error
 */

int join_specific( int argc, char* argv[] )
{
    int              result;
    char*            ssid = argv[1];
    wiced_security_t auth_type = str_to_authtype(argv[4]);
    uint8_t          key_length;
    uint8_t          wep_key_buffer[64];
    uint8_t*         security_key;

    if (argc > 9)
    {
        return ERR_TOO_MANY_ARGS;
    }

    if (argc > 6 && argc != 9)
    {
        return ERR_INSUFFICENT_ARGS;
    }

    if ( auth_type == WICED_SECURITY_UNKNOWN )
    {
        WPRINT_APP_INFO(( "Error: Invalid security type\n" ));
        return ERR_UNKNOWN;
    }

    if ( argc == 9 )
    {
        security_key = (uint8_t*)argv[5];
        result = process_join_security(argc, ssid, auth_type, argv[5], &key_length, wep_key_buffer);
        if (result == ERR_CMD_OK)
            return wifi_join_specific( ssid, strlen(ssid), auth_type, security_key, key_length, argv[2], argv[3], argv[6], argv[7], argv[8]);
    }
    else
    {
        security_key = (uint8_t*)argv[3];
        result = process_join_security(argc, ssid, auth_type, argv[3], &key_length, wep_key_buffer);
        if (result == ERR_CMD_OK)
            return wifi_join_specific( ssid, strlen(ssid), auth_type, security_key, key_length, argv[2], argv[3], NULL, NULL, NULL );
    }
    return result;
}

static int wifi_join_adhoc(char* ssid, uint8_t ssid_length, wiced_security_t auth_type, uint8_t* security_key, uint16_t key_length, char* channel, char* ip, char* netmask, char* gateway)
{
    int chan;
    wiced_network_config_t network_config;
    wiced_ip_setting_t     static_ip_settings;
    wiced_scan_result_t    ap;
    wiced_result_t         result;

    if (wwd_wifi_is_ready_to_transceive(WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }

    chan = atoi(channel);

    memset( &ap, 0, sizeof( ap ) );
    ap.SSID.length = ssid_length;
    memcpy( ap.SSID.value, ssid, ap.SSID.length );
    if (chan > 0)
       ap.channel = chan;
    else
       ap.channel = 1;
    ap.bss_type = WICED_BSS_TYPE_ADHOC;
    ap.security = auth_type;
    ap.band = WICED_WIFI_CH_TO_BAND( ap.channel );

    result = (wiced_result_t)wwd_wifi_join_specific( &ap, security_key, key_length, NULL, WWD_STA_INTERFACE );
    if ( result == WICED_SUCCESS )
    {
        network_config = WICED_USE_STATIC_IP;
        str_to_ip( ip,      &static_ip_settings.ip_address );
        str_to_ip( netmask, &static_ip_settings.netmask );
        str_to_ip( gateway, &static_ip_settings.gateway );

        if ( ( result = wiced_ip_up( WICED_STA_INTERFACE, network_config, &static_ip_settings ) ) == WICED_SUCCESS )
        {
            strlcpy( last_joined_ssid, ssid, MIN(sizeof(last_joined_ssid), ssid_length+1) );
            return ERR_CMD_OK;
        }
    }
    else
    {
        analyse_failed_join_result( result );
    }

    return ERR_UNKNOWN;
}

static int wifi_join_specific(char* ssid, uint8_t ssid_length, wiced_security_t auth_type, uint8_t* security_key, uint16_t key_length, char* bssid, char* channel, char* ip, char* netmask, char* gateway)
{
    wiced_network_config_t network_config;
    wiced_ip_setting_t     static_ip_settings;
    wiced_scan_result_t    ap;
    wiced_result_t         result;

    if (wwd_wifi_is_ready_to_transceive(WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }

    memset( &ap, 0, sizeof( ap ) );
    ap.SSID.length = ssid_length;
    memcpy( ap.SSID.value, ssid, ap.SSID.length );
    str_to_mac( bssid, &ap.BSSID );
    ap.channel = atoi( channel );
    ap.security = auth_type;
    ap.band = WICED_WIFI_CH_TO_BAND( ap.channel );
    ap.bss_type = WICED_BSS_TYPE_INFRASTRUCTURE;

    if ( !( NULL_MAC(ap.BSSID.octet) ) )
    {
        result = (wiced_result_t)wwd_wifi_join_specific( &ap, security_key, key_length, NULL, WWD_STA_INTERFACE );
        if ( result == WICED_SUCCESS )
        {
            /* Tell the network stack to setup it's interface */
            if (ip == NULL )
            {
                network_config = WICED_USE_EXTERNAL_DHCP_SERVER;
            }
            else
            {
                network_config = WICED_USE_STATIC_IP;
                str_to_ip( ip,      &static_ip_settings.ip_address );
                str_to_ip( netmask, &static_ip_settings.netmask );
                str_to_ip( gateway, &static_ip_settings.gateway );
            }

            if ( ( result = wiced_ip_up( WICED_STA_INTERFACE, network_config, &static_ip_settings ) ) == WICED_SUCCESS )
            {
                strlcpy( last_joined_ssid, ssid, MIN(sizeof(last_joined_ssid), ssid_length+1) );
                return ERR_CMD_OK;
            }
        }
        else
        {
            analyse_failed_join_result( result );
        }
    }

    return ERR_UNKNOWN;
}

/**
 *  Scan result callback
 *  Called whenever a scan result is available
 *
 *  @param result_ptr : pointer to pointer for location where result is stored. The inner pointer
 *                      can be updated to cause the next result to be put in a new location.
 *  @param user_data : unused
 */
static wiced_result_t scan_result_handler( wiced_scan_handler_result_t* malloced_scan_result )
{
    if ( malloced_scan_result != NULL )
    {
        malloc_transfer_to_curr_thread( malloced_scan_result );

        if ( malloced_scan_result->status == WICED_SCAN_INCOMPLETE )
        {
            wiced_scan_result_t* record = &malloced_scan_result->ap_details;

            wiced_assert( "error", ( record->bss_type == WICED_BSS_TYPE_INFRASTRUCTURE ) || ( record->bss_type == WICED_BSS_TYPE_ADHOC ) );

            WPRINT_APP_INFO( ( "%3d ", record_count ) );
            print_scan_result( record );

            ++record_count;
        }
        else
        {
            wiced_rtos_set_semaphore(&scan_semaphore);
        }

        free( malloced_scan_result );
        malloced_scan_result = NULL;
    }

    return WICED_SUCCESS;
}

static int
parse_country_spec( char *spec, int8_t *ccode, int32_t *regrev )
{
    char *revstr;
    char *endptr = NULL;
    int ccode_len;
    int rev = -1;

    revstr = strchr( spec, '/' );
    if ( revstr )
    {
        rev = strtol( revstr + 1, &endptr, 10 );
        if ( *endptr != '\0' )
        {
            return ERR_UNKNOWN_CMD;
        }
    }
    if ( revstr )
    {
        ccode_len = (int)(strlen( spec ) - strlen( revstr ));
    }
    else
    {
        ccode_len = (int)strlen( spec );
    }
    if ( ccode_len > 3 )
    {
        return ERR_UNKNOWN_CMD;
    }
    memcpy( ccode, spec, ccode_len );
    ccode[ccode_len] = '\0';
    *regrev = rev;

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Scans for access points and prints out results
 *
 * @return  0 for success, otherwise error
 */

int scan( int argc, char* argv[] )
{
    record_count = 0;
    WPRINT_APP_INFO( ( "Waiting for scan results...\n" ) );

    WPRINT_APP_INFO( ("  # Type  BSSID              RSSI Rate Chan  Security               SSID\n" ) );
    WPRINT_APP_INFO( ("----------------------------------------------------------------------------------------------\n" ) );

    /* Initialise the semaphore that will tell us when the scan is complete */
    wiced_rtos_init_semaphore(&scan_semaphore);

    wiced_wifi_scan_networks(scan_result_handler, NULL );

    /* Wait until scan is complete */
    wiced_rtos_get_semaphore(&scan_semaphore, WICED_WAIT_FOREVER);

    wiced_rtos_deinit_semaphore(&scan_semaphore);

    /* Done! */
    WPRINT_APP_INFO( ( "\nEnd of scan results\n" ) );

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Starts a soft AP as specified by the provided arguments
 *
 * @return  0 for success, otherwise error
 */

int start_ap( int argc, char* argv[] )
{
    char* ssid = argv[1];
    wiced_security_t auth_type = str_to_authtype(argv[2]);
    char* security_key = argv[3];
    uint8_t channel = atoi(argv[4]);
    wiced_result_t result;
    uint8_t pmk[DOT11_PMK_LEN + 8]; /* PMK storage must be 40 octets in length for use in various functions */
    platform_dct_wifi_config_t* dct_wifi_config;
    uint8_t  key_length = 0;

    if ( wwd_wifi_is_ready_to_transceive( WWD_AP_INTERFACE ) == WWD_SUCCESS )
    {
        WPRINT_APP_INFO(( "Error: AP already started\n" ));
        return ERR_UNKNOWN;
    }

    if ( ( auth_type != WICED_SECURITY_WPA2_AES_PSK ) &&
         ( auth_type != WICED_SECURITY_OPEN ) &&
         ( auth_type != WICED_SECURITY_WPA2_MIXED_PSK ) &&
         ( auth_type != WICED_SECURITY_WEP_PSK ) &&
         ( auth_type != WICED_SECURITY_WEP_SHARED ) )
    {
        WPRINT_APP_INFO(( "Error: Invalid security type\n" ));
        return ERR_UNKNOWN;
    }

    if ( auth_type == WICED_SECURITY_OPEN )
    {
        char c = 0;

        WPRINT_APP_INFO(( "Open without any encryption [y or n]?\n" ));
        while (1)
        {
            c = getchar();
            if ( c == 'y' )
            {
                break;
            }
            if ( c == 'n' )
            {
                return ERR_CMD_OK;
            }
            WPRINT_APP_INFO(( "y or n\n" ));
        }
    }

    if ( argc == 6 )
    {
        if ( memcmp( argv[5], "wps", sizeof("wps") ) != 0 )
        {
            return ERR_UNKNOWN;
        }
    }

    key_length = strlen(security_key);

    if ( ( auth_type & WPA2_SECURITY ) && ( key_length < MIN_PASSPHRASE_LEN ) )
    {
        WPRINT_APP_INFO(("Error: WPA key too short\n" ));
        return ERR_UNKNOWN;
    }

    /* Read config */
    wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );

    strlcpy( last_soft_ap_passphrase, security_key, sizeof( last_soft_ap_passphrase ) );

    /* Modify config */
    if ( key_length < MAX_PASSPHRASE_LEN)
    {
        char  temp_security_key[64];
        memset( temp_security_key, 0, MAX_PASSPHRASE_LEN );

        if ( auth_type == WICED_SECURITY_WEP_PSK || auth_type == WICED_SECURITY_WEP_SHARED )
        {
#ifdef WICED_WIFI_SOFT_AP_WEP_SUPPORT_ENABLED
            /* Format WEP security key */
            format_wep_keys( temp_security_key, security_key, &key_length, WEP_KEY_TYPE );
#else
            WPRINT_APP_INFO(( "Error: WEP is disabled\n" ));
            return ERR_UNKNOWN;
#endif
        }
        else
        {
            memset(pmk, 0, sizeof(pmk));
            if ( besl_802_11_generate_pmk( security_key, (unsigned char *) ssid, strlen( ssid ), (unsigned char*) pmk ) != BESL_SUCCESS )
            {
                WPRINT_APP_INFO(( "Error: Failed to generate pmk\n" ));
                return ERR_UNKNOWN;
            }

            key_length = MAX_PASSPHRASE_LEN;
            besl_host_hex_bytes_to_chars( temp_security_key, pmk, DOT11_PMK_LEN );
        }
        dct_wifi_config->soft_ap_settings.security_key_length = key_length;
        memcpy( dct_wifi_config->soft_ap_settings.security_key, temp_security_key, MAX_PASSPHRASE_LEN );
    }
    else
    {
        dct_wifi_config->soft_ap_settings.security_key_length = MAX_PASSPHRASE_LEN;
        /* strlcpy( ) guarantee to NUL-terminate the result
         *  - strlcpy( ) src ( parameter 2 ) must be NUL-terminated
         */
        strlcpy( dct_wifi_config->soft_ap_settings.security_key, security_key, sizeof( dct_wifi_config->soft_ap_settings.security_key ) );
    }

    if (strlen(ssid) > SSID_NAME_SIZE)
    {
        WPRINT_APP_INFO(( "Error: SSID longer than 32 characters\n" ));
        return ERR_UNKNOWN;
    }

    memcpy( (char*)dct_wifi_config->soft_ap_settings.SSID.value, ssid, strlen( ssid ) );
    dct_wifi_config->soft_ap_settings.security = auth_type;
    dct_wifi_config->soft_ap_settings.channel = channel;
    dct_wifi_config->soft_ap_settings.SSID.length = strlen( ssid );

    /* Write config */
    wiced_dct_write( (const void*)dct_wifi_config, DCT_WIFI_CONFIG_SECTION, 0, sizeof( platform_dct_wifi_config_t));
    wiced_dct_read_unlock( (void*) dct_wifi_config, WICED_TRUE );

    if ( ( result = wiced_network_up( WICED_AP_INTERFACE, WICED_USE_INTERNAL_DHCP_SERVER, &ap_ip_settings ) ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO(("Error starting AP %u\n", (unsigned int)result));
        return result;
    }
#ifdef COMMAND_CONSOLE_WPS_ENABLED
    if ( ( argc == 6 ) && ( memcmp( argv[5], "wps", sizeof("wps") ) == 0 ) )
    {
        result = enable_ap_registrar_events();
        if ( result != WICED_SUCCESS )
        {
            return result;
        }
    }
#endif
    strlcpy( last_started_ssid, ssid, sizeof ( last_started_ssid ) );
    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Stops a running soft AP
 *
 * @return  0 for success, otherwise error
 */

int stop_ap( int argc, char* argv[] )
{
    if (wwd_wifi_is_ready_to_transceive( WWD_AP_INTERFACE ) != WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }

#ifdef COMMAND_CONSOLE_WPS_ENABLED
    disable_ap_registrar_events();
#endif

    wwd_wifi_deauth_all_associated_client_stas( WWD_DOT11_RC_UNSPECIFIED, WWD_AP_INTERFACE );

    return wiced_network_down( WICED_AP_INTERFACE );
}

int get_associated_sta_list( int argc, char* argv[] )
{
    uint8_t* buffer = NULL;
    wiced_maclist_t * clients = NULL;
    const wiced_mac_t * current;
    wl_bss_info_t ap_info;
    wiced_security_t sec;
    uint32_t max_associations = 0;
    size_t size = 0;
    int32_t rssi = 0;

    if ( wwd_wifi_get_max_associations( &max_associations ) != WWD_SUCCESS )
    {
        WPRINT_APP_INFO(("Failed to get max number of associations\n"));
        max_associations = 5;
    }
    else
    {
        WPRINT_APP_INFO(("Max number of associations: %u\n", (unsigned int)max_associations));
    }

    size = (sizeof(uint32_t) + (max_associations * sizeof(wiced_mac_t)));
    buffer = calloc(1, size);

    if (buffer == NULL)
    {
        WPRINT_APP_INFO(("Unable to allocate memory for associations list\n"));
        return WICED_ERROR;
    }
    clients = (wiced_maclist_t*)buffer;
    wwd_wifi_get_associated_client_list(clients, size);

    memset(&ap_info, 0, sizeof(wl_bss_info_t));
    if (wwd_wifi_is_ready_to_transceive( WWD_STA_INTERFACE ) == WWD_SUCCESS)
    {
        wwd_wifi_get_ap_info( &ap_info, &sec );
        if (clients->count == 0 )
        {
            clients->count = 1;
            memcpy(&clients->mac_list[0], &ap_info.BSSID, sizeof(wl_ether_addr_t));
        }
    }

    WPRINT_APP_INFO(("Current number of associated STAs: %u\n", (unsigned int)clients->count));
    current = &clients->mac_list[0];
    WPRINT_APP_INFO(("\n"));
    while ((clients->count > 0) && (!(NULL_MAC(current->octet))))
    {
        WPRINT_APP_INFO(("%02x:%02x:%02x:%02x:%02x:%02x ",
                            current->octet[0],
                            current->octet[1],
                            current->octet[2],
                            current->octet[3],
                            current->octet[4],
                            current->octet[5]));
        if (memcmp(current->octet, &(ap_info.BSSID), sizeof(wiced_mac_t)) != 0)
        {
            wwd_wifi_get_ap_client_rssi(&rssi, (wiced_mac_t*)&current->octet[0]);
            WPRINT_APP_INFO(("%3lddBm  Client\n", (long int)rssi));
        }
        else
        {
            wwd_wifi_get_rssi( &rssi );
            WPRINT_APP_INFO(("%3lddBm  AP\n", (long int)rssi));
        }
        --clients->count;
        ++current;
    }
    WPRINT_APP_INFO(("\n"));
    besl_host_free( buffer );
    return ERR_CMD_OK;
}

int test_ap( int argc, char* argv[] )
{
    int i;
    int iterations;

    if (  argc < 6 )
    {
        return ERR_UNKNOWN;
    }
    iterations = atoi(argv[argc - 1]);
    WPRINT_APP_INFO(("Iterations: %d\n", iterations));
    for (i = 0; i < iterations; i++ )
    {
        WPRINT_APP_INFO(( "Iteration %d\n", i));
        start_ap( argc-1, argv );
        stop_ap( 0, NULL );
    }
    wiced_mac_t mac;
    if ( wwd_wifi_get_mac_address( &mac, WWD_STA_INTERFACE ) == WWD_SUCCESS )
    {
        WPRINT_APP_INFO(("Test Pass (MAC address is: %02X:%02X:%02X:%02X:%02X:%02X)\n", mac.octet[0], mac.octet[1], mac.octet[2], mac.octet[3], mac.octet[4], mac.octet[5]));
    }
    else
    {
        WPRINT_APP_INFO(("Test Fail\n"));
    }
    return ERR_CMD_OK;
}

int test_join( int argc, char* argv[] )
{
    int i;
    int iterations;
    uint32_t join_fails = 0, leave_fails = 0, join_successes = 0;
    wiced_time_t t1, t2, average_time = 0;

    if (  argc < 5 )
    {
        return ERR_UNKNOWN;
    }
    iterations = atoi(argv[argc - 1]);

    for (i = 0; i < iterations; i++ )
    {
        WPRINT_APP_INFO(( "%d ", i));
        wiced_time_get_time( &t1 );
        if ( join( argc-1, argv ) != ERR_CMD_OK)
        {
            ++join_fails;
        }
        else
        {
            wiced_time_get_time( &t2 );
            t2 = t2 - t1;
            WPRINT_APP_INFO( ( "Time for successful join = %u ms\n", (unsigned int)t2 ) );
            average_time += t2;
            join_successes++;
        }
        if ( leave( 0, NULL ) != ERR_CMD_OK )
        {
            ++leave_fails;
        }
    }

    WPRINT_APP_INFO(("Join failures:     %u\n", (unsigned int)join_fails));
    WPRINT_APP_INFO(("Leave failures:    %u\n", (unsigned int)leave_fails));
    WPRINT_APP_INFO(("Average join time: %u ms\n", (unsigned int)(average_time/join_successes)));

    return ERR_CMD_OK;
}

int test_join_specific( int argc, char* argv[] )
{
    int i;
    int iterations;
    uint32_t join_fails = 0, leave_fails = 0;

    if (  argc < 5 )
    {
        return ERR_UNKNOWN;
    }
    iterations = atoi(argv[argc - 1]);

    for (i = 0; i < iterations; i++ )
    {
        WPRINT_APP_INFO(( "%d ", i));
        if ( join_specific( argc-1, argv ) != ERR_CMD_OK)
        {
            ++join_fails;
        }
        if ( leave( 0, NULL ) != ERR_CMD_OK )
        {
            ++leave_fails;
        }
    }

    WPRINT_APP_INFO(("Join specific failures: %u\n", (unsigned int)join_fails));
    WPRINT_APP_INFO(("Leave failures: %u\n", (unsigned int)leave_fails));

    return ERR_CMD_OK;
}

int test_credentials( int argc, char* argv[] )
{
    wwd_result_t result;
    wiced_scan_result_t ap;

    memset(&ap, 0, sizeof(ap));

    ap.SSID.length = strlen(argv[1]);
    memcpy(ap.SSID.value, argv[1], ap.SSID.length);
    str_to_mac(argv[2], &ap.BSSID);
    ap.channel = atoi(argv[3]);
    ap.security = str_to_authtype(argv[4]);
    result = wwd_wifi_test_credentials(&ap, (uint8_t*)argv[5], strlen(argv[5]));

    if ( result == WWD_SUCCESS )
    {
        WPRINT_APP_INFO(("Credentials are good\n"));
    }
    else
    {
        WPRINT_APP_INFO(("Credentials are bad\n"));
    }

    return ERR_CMD_OK;
}

int get_soft_ap_credentials( int argc, char* argv[] )
{
    wiced_security_t sec;
    platform_dct_wifi_config_t* dct_wifi_config;

    if ( wwd_wifi_is_ready_to_transceive( WWD_AP_INTERFACE ) != WWD_SUCCESS )
    {
        WPRINT_APP_INFO(("Use start_ap command to bring up AP interface first\n"));
        return ERR_CMD_OK;
    }

    /* Read config to get internal AP settings */
    wiced_dct_read_lock( (void**) &dct_wifi_config, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );
    WPRINT_APP_INFO(("SSID : %s\n", (char*)dct_wifi_config->soft_ap_settings.SSID.value));
    sec = dct_wifi_config->soft_ap_settings.security;
    WPRINT_APP_INFO( ( "Security : %s\n", ( sec == WICED_SECURITY_OPEN )           ? "Open" :
                                            ( sec == WICED_SECURITY_WEP_PSK )        ? "WEP" :
                                            ( sec == WICED_SECURITY_WPA_TKIP_PSK )   ? "WPA TKIP" :
                                            ( sec == WICED_SECURITY_WPA_AES_PSK )    ? "WPA AES" :
                                            ( sec == WICED_SECURITY_WPA2_AES_PSK )   ? "WPA2 AES" :
                                            ( sec == WICED_SECURITY_WPA2_TKIP_PSK )  ? "WPA2 TKIP" :
                                            ( sec == WICED_SECURITY_WPA2_MIXED_PSK ) ? "WPA2 Mixed" :
                                            "Unknown" ) );
    WPRINT_APP_INFO(("Passphrase : %s\n", last_soft_ap_passphrase));

    wiced_dct_read_unlock( (void*) dct_wifi_config, WICED_FALSE );

    return ERR_CMD_OK;
}

int get_pmk( int argc, char* argv[] )
{
    char pmk[64];

    if ( wwd_wifi_get_pmk( argv[1], strlen( argv[1] ), pmk ) == WWD_SUCCESS )
    {
        WPRINT_APP_INFO( ("%s\n", pmk) );
        return ERR_CMD_OK;
    }
    else
    {
        return ERR_UNKNOWN;
    }
}

#define CNT_DIFF(field) results->field = (second->field - first->field)/diff_secs

/* results = second - first */
static int
counter_diff(int diff_secs,
    wiced_counters_t* first,
    wiced_counters_t* second,
    wiced_counters_t* results)
{

    if (diff_secs <= 0)
        return ERR_UNKNOWN;
    if (!first || !second || !results)
        return ERR_UNKNOWN;
    if ((first->version != second->version )|| (first->length != second->length))
        return ERR_UNKNOWN;

    /* transmit stat counters */
    CNT_DIFF(txframe);
    CNT_DIFF(txbyte);
    CNT_DIFF(txretrans);
    CNT_DIFF(txerror );
    CNT_DIFF(txctl);
    CNT_DIFF(txprshort);
    CNT_DIFF(txserr);
    CNT_DIFF(txnobuf);
    CNT_DIFF(txnoassoc);
    CNT_DIFF(txrunt);
    CNT_DIFF(txchit);
    CNT_DIFF(txcmiss);

    /* transmit chip error counters */
    CNT_DIFF(txuflo);
    CNT_DIFF(txphyerr);
    CNT_DIFF(txphycrs);

    /* receive stat counters */
    CNT_DIFF(rxframe);
    CNT_DIFF(rxbyte);
    CNT_DIFF(rxerror);
    CNT_DIFF(rxctl);
    CNT_DIFF(rxnobuf);
    CNT_DIFF(rxnondata);
    CNT_DIFF(rxbadds);
    CNT_DIFF(rxbadcm);
    CNT_DIFF(rxfragerr);
    CNT_DIFF(rxrunt);
    CNT_DIFF(rxgiant);
    CNT_DIFF(rxnoscb);
    CNT_DIFF(rxbadproto);
    CNT_DIFF(rxbadsrcmac);
    CNT_DIFF(rxbadda);
    CNT_DIFF(rxfilter);

    /* receive chip error counters */
    CNT_DIFF(rxoflo               );
    //rxuflo[NFIFO]

    CNT_DIFF(d11cnt_txrts_off);
    CNT_DIFF(d11cnt_rxcrc_off);
    CNT_DIFF(d11cnt_txnocts_off);

    /* misc counters */
    CNT_DIFF(dmade);
    CNT_DIFF(dmada);
    CNT_DIFF(dmape);
    CNT_DIFF(reset);
    CNT_DIFF(tbtt);
    CNT_DIFF(txdmawar);
    CNT_DIFF(pkt_callback_reg_fail);

    /* MAC counters: 32-bit version of d11.h's macstat_t */
    CNT_DIFF(txallfrm);
    CNT_DIFF(txrtsfrm);
    CNT_DIFF(txctsfrm);
    CNT_DIFF(txackfrm);
    CNT_DIFF(txdnlfrm);
    CNT_DIFF(txbcnfrm);
    //txfunfl[6]
    CNT_DIFF(txtplunfl);
    CNT_DIFF(txphyerror);
    CNT_DIFF(rxfrmtoolong);
    CNT_DIFF(rxfrmtooshrt);
    CNT_DIFF(rxinvmachdr);
    CNT_DIFF(rxbadfcs);
    CNT_DIFF(rxbadplcp);
    CNT_DIFF(rxcrsglitch);
    CNT_DIFF(rxstrt);
    CNT_DIFF(rxdfrmucastmbss);
    CNT_DIFF(rxmfrmucastmbss);
    CNT_DIFF(rxcfrmucast);
    CNT_DIFF(rxrtsucast);
    CNT_DIFF(rxctsucast);
    CNT_DIFF(rxackucast);
    CNT_DIFF(rxdfrmocast);
    CNT_DIFF(rxmfrmocast);
    CNT_DIFF(rxcfrmocast);
    CNT_DIFF(rxrtsocast);
    CNT_DIFF(rxctsocast);
    CNT_DIFF(rxdfrmmcast);
    CNT_DIFF(rxmfrmmcast);
    CNT_DIFF(rxcfrmmcast);
    CNT_DIFF(rxbeaconmbss);
    CNT_DIFF(rxdfrmucastobss);
    CNT_DIFF(rxbeaconobss);
    CNT_DIFF(rxrsptmout);
    CNT_DIFF(bcntxcancl);
    CNT_DIFF(rxf0ovfl);
    CNT_DIFF(rxf1ovfl);
    CNT_DIFF(rxf2ovfl);
    CNT_DIFF(txsfovfl);
    CNT_DIFF(pmqovfl);
    CNT_DIFF(rxcgprqfrm);
    CNT_DIFF(rxcgprsqovfl);
    CNT_DIFF(txcgprsfail);
    CNT_DIFF(txcgprssuc);
    CNT_DIFF(prs_timeout);
    CNT_DIFF(rxnack);
    CNT_DIFF(frmscons);
    CNT_DIFF(txnack);

    /* 802.11 MIB counters, pp. 614 of 802.11 reaff doc. */
    CNT_DIFF(txfrag);
    CNT_DIFF(txmulti);
    CNT_DIFF(txfail);
    CNT_DIFF(txretry);
    CNT_DIFF(txretrie);
    CNT_DIFF(rxdup);
    CNT_DIFF(txrts);
    CNT_DIFF(txnocts);
    CNT_DIFF(txnoack);
    CNT_DIFF(rxfrag);
    CNT_DIFF(rxmulti);
    CNT_DIFF(rxcrc);
    CNT_DIFF(txfrmsnt);
    CNT_DIFF(rxundec);

    /* WPA2 counters (see rxundec for DecryptFailureCount) */
    CNT_DIFF(tkipmicfaill);
    CNT_DIFF(tkipcntrmsr);
    CNT_DIFF(tkipreplay);
    CNT_DIFF(ccmpfmterr);
    CNT_DIFF(ccmpreplay);
    CNT_DIFF(ccmpundec);
    CNT_DIFF(fourwayfail);
    CNT_DIFF(wepundec);
    CNT_DIFF(wepicverr);
    CNT_DIFF(decsuccess);
    CNT_DIFF(tkipicverr);
    CNT_DIFF(wepexcluded);

    CNT_DIFF(txchanrej);
    CNT_DIFF(psmwds);
    CNT_DIFF(phywatchdog);

    /* MBSS counters, AP only */
    CNT_DIFF(prq_entries_handled);
    CNT_DIFF(prq_undirected_entries);
    CNT_DIFF(prq_bad_entries);
    CNT_DIFF(atim_suppress_count);
    CNT_DIFF(bcn_template_not_ready);
    CNT_DIFF(bcn_template_not_ready_done);
    CNT_DIFF(late_tbtt_dpc);

    /* per-rate receive stat counters */
    CNT_DIFF(rx1mbps);
    CNT_DIFF(rx2mbps);
    CNT_DIFF(rx5mbps5);
    CNT_DIFF(rx6mbps);
    CNT_DIFF(rx9mbps);
    CNT_DIFF(rx11mbps);
    CNT_DIFF(rx12mbps);
    CNT_DIFF(rx18mbps);
    CNT_DIFF(rx24mbps);
    CNT_DIFF(rx36mbps);
    CNT_DIFF(rx48mbps);
    CNT_DIFF(rx54mbps);
    CNT_DIFF(rx108mbps);
    CNT_DIFF(rx162mbps);
    CNT_DIFF(rx216mbps);
    CNT_DIFF(rx270mbps);
    CNT_DIFF(rx324mbps);
    CNT_DIFF(rx378mbps);
    CNT_DIFF(rx432mbps);
    CNT_DIFF(rx486mbps);
    CNT_DIFF(rx540mbps);

    /* pkteng rx frame stats */
    CNT_DIFF(pktengrxducast);
    CNT_DIFF(pktengrxdmcast);

    CNT_DIFF(rfdisable);
    CNT_DIFF(bphy_rxcrsglitch);

    CNT_DIFF(txexptime);

    CNT_DIFF(txmpdu_sgi);
    CNT_DIFF(rxmpdu_sgi);
    CNT_DIFF(txmpdu_stbc);
    CNT_DIFF(rxmpdu_stbc);

    CNT_DIFF(rxundec_mcst);

    /* WPA2 counters (see rxundec for DecryptFailureCount) */
    CNT_DIFF(tkipmicfaill_mcst);
    CNT_DIFF(tkipcntrmsr_mcst);
    CNT_DIFF(tkipreplay_mcst);
    CNT_DIFF(ccmpfmterr_mcst);
    CNT_DIFF(ccmpreplay_mcst);
    CNT_DIFF(ccmpundec_mcst);
    CNT_DIFF(fourwayfail_mcst);
    CNT_DIFF(wepundec_mcst);
    CNT_DIFF(wepicverr_mcst);
    CNT_DIFF(decsuccess_mcst);
    CNT_DIFF(tkipicverr_mcst);
    CNT_DIFF(wepexcluded_mcst);

    return ERR_CMD_OK;
}

#define CNT_LOG(field)  if (results->field) WPRINT_APP_INFO(("%-16s%4ld%s  ", #field, results->field, normal?"/s":""))
static int
print_counters_rate(wiced_counters_t* results, int normal)
{
    CNT_LOG(rx1mbps);
    CNT_LOG(rx2mbps);
    CNT_LOG(rx5mbps5);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rx6mbps);
    CNT_LOG(rx9mbps);
    CNT_LOG(rx11mbps);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rx12mbps);
    CNT_LOG(rx18mbps);
    CNT_LOG(rx24mbps);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rx36mbps);
    CNT_LOG(rx48mbps);
    CNT_LOG(rx54mbps);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rx108mbps);
    CNT_LOG(rx162mbps);
    CNT_LOG(rx216mbps);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rx270mbps);
    CNT_LOG(rx324mbps);
    CNT_LOG(rx378mbps);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rx432mbps);
    CNT_LOG(rx486mbps);
    CNT_LOG(rx540mbps);
WPRINT_APP_INFO(("\n"));

    return ERR_CMD_OK;
};

static int
print_counters_tx(wiced_counters_t* results, int normal)
{
    CNT_LOG(txframe);
    CNT_LOG(txbyte);
    CNT_LOG(txretrans);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(txerror );
    CNT_LOG(txctl);
    CNT_LOG(txallfrm);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(txrtsfrm);
    CNT_LOG(txctsfrm);
    CNT_LOG(txackfrm);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(txdnlfrm);
    CNT_LOG(txbcnfrm);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(txtplunfl);
    CNT_LOG(txphyerror);
    CNT_LOG(bcntxcancl);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(txsfovfl);
    CNT_LOG(txcgprsfail);
    CNT_LOG(txcgprssuc);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(txnack);
    CNT_LOG(txfrag);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(txmulti);
    CNT_LOG(txfail);
    CNT_LOG(txretry);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(txretrie);
    CNT_LOG(txrts);
    CNT_LOG(txnocts);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(txnoack);
    CNT_LOG(txfrmsnt);
    CNT_LOG(txchanrej);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(txexptime);
    CNT_LOG(txmpdu_sgi);
    CNT_LOG(txmpdu_stbc);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(txprshort);
    CNT_LOG(txserr);
    CNT_LOG(txnobuf);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(txnoassoc);
    CNT_LOG(txrunt);
    CNT_LOG(txchit);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(txcmiss);
    CNT_LOG(txuflo);
    CNT_LOG(txphyerr);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(txphycrs);
    CNT_LOG(d11cnt_txrts_off);
    CNT_LOG(d11cnt_txnocts_off);
    CNT_LOG(txdmawar);
WPRINT_APP_INFO(("\n"));
    return ERR_CMD_OK;
}

static int
print_counters_rx(wiced_counters_t* results, int normal)
{
    CNT_LOG(rxframe);
    CNT_LOG(rxbyte);
    CNT_LOG(rxerror);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxctl);
    CNT_LOG(rxnobuf);
    CNT_LOG(rxnondata);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxbadds);
    CNT_LOG(rxbadcm);
    CNT_LOG(rxfragerr);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxrunt);
    CNT_LOG(rxgiant);
    CNT_LOG(rxnoscb);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxbadproto);
    CNT_LOG(rxbadsrcmac);
    CNT_LOG(rxbadda);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxfilter);
    CNT_LOG(rxoflo);
    CNT_LOG(d11cnt_rxcrc_off);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxfrmtoolong);
    CNT_LOG(rxfrmtooshrt);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxinvmachdr);
    CNT_LOG(rxbadfcs);
    CNT_LOG(rxbadplcp);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxcrsglitch);
    CNT_LOG(rxstrt);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxcfrmucast);
    CNT_LOG(rxrtsucast);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxctsucast);
    CNT_LOG(rxackucast);
    CNT_LOG(rxf0ovfl);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxbeaconobss);
    CNT_LOG(rxdfrmucastobss);
    CNT_LOG(rxrsptmout);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxf1ovfl);
    CNT_LOG(rxf2ovfl);
    CNT_LOG(rxcgprqfrm);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxcgprsqovfl);
    CNT_LOG(rxnack);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxdup);
    CNT_LOG(rxfrag);
    CNT_LOG(rxmulti);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxcrc);
    CNT_LOG(rxundec);
    CNT_LOG(pktengrxducast);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(pktengrxdmcast);
    CNT_LOG(bphy_rxcrsglitch);
    CNT_LOG(rxmpdu_sgi);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxmpdu_stbc);
    CNT_LOG(rxundec_mcst);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxbeaconmbss);
    CNT_LOG(rxdfrmucastmbss);
    CNT_LOG(rxmfrmucastmbss);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxdfrmmcast);
    CNT_LOG(rxmfrmmcast);
    CNT_LOG(rxcfrmmcast);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxdfrmocast);
    CNT_LOG(rxmfrmocast);
    CNT_LOG(rxcfrmocast);
WPRINT_APP_INFO(("\n"));
    CNT_LOG(rxrtsocast);
    CNT_LOG(rxctsocast);
WPRINT_APP_INFO(("\n"));
    return ERR_CMD_OK;
}

/* Example: get_counters: -t 3 -n rx rxrate
 *  -t seconds: collect 'seconds' worth of data
 *  -n          normalize into per/seconds.
 *  tx          tx counters
 *  rx          rx counters
 *  rate        rx rate histogram
 */
int get_counters( int argc, char* argv[] )
{
    UNUSED_PARAMETER( argc );
    UNUSED_PARAMETER( argv );
    wiced_counters_t* data, *data2 = NULL;
    int seconds = 0;    /* Num seconds to measure */
    int normal = 0;     /* Normalize to per second? */
    int flags = 0;

    while (*++argv) {
        if (!strcmp(*argv, "-t")){
            if (*(argv+1)) {
                seconds = atoi(*(argv+1));
                if (seconds < 0)
                {
                    WPRINT_APP_INFO(("Seconds (%d) must be > 0.\n", seconds));
                    return ERR_UNKNOWN;
                }
                argv++;
            } else {
                WPRINT_APP_INFO(("Usage: get_counters [-t seconds [-n]] [tx] [rx] [rate]\n"));
            }
        } else
            if (!strcmp(*argv, "-n")) {
                normal++;
            } else
                if (!strcmp(*argv, "tx")) {
                    flags |= DUMP_TX;
                } else
                    if (!strcmp(*argv, "rx")) {
                        flags |= DUMP_RX;
                    } else
                        if (!strcmp(*argv, "rate")) {
                            flags |= DUMP_RATE;
                        } else {
                            WPRINT_APP_INFO(("%s: Unknown parameter: %s\n", __FUNCTION__, *argv));
                            return ERR_UNKNOWN;
                        }
    }

    if (seconds == 0)   /* Can't normalize 0 seconds */
        normal = 0;
    if (flags == 0)     /* Default to all counters */
        flags = DUMP_TX | DUMP_RX | DUMP_RATE | DUMP_AMPDU;

    WPRINT_APP_DEBUG(("Flags 0x%x, Normalize %d, Seconds %d\n", flags, normal, seconds));

    data = malloc_named( "stats_counters_buffer", sizeof(wiced_counters_t));
    if (!data) {
         WPRINT_APP_ERROR(("Unable to allocate data buffer!\n"));
         return ERR_UNKNOWN;
    }
    if (seconds) {
        data2 = malloc_named( "stats_counters_buffer2", sizeof(wiced_counters_t));
        if (!data2) {
            free(data);
            WPRINT_APP_ERROR(("Unable to allocate second data buffer!\n"));
            return ERR_UNKNOWN;
        }
    }

    if (wwd_get_counters(data) != WWD_SUCCESS) {
        free(data);
        if (seconds)
            free(data2);
        return ERR_UNKNOWN;
    }

    if (seconds) {
        wiced_rtos_delay_milliseconds(seconds * 1000);
        if (wwd_get_counters(data2) == WWD_SUCCESS) {
            if (!normal)
                seconds = 1; /* Dividing by 1 effectively does nothing */

            counter_diff(seconds, data, data2, data);
        }
    }

    if (flags & DUMP_RATE) {
        WPRINT_APP_INFO(("Rate counters:\n"));
        print_counters_rate(data, normal);
    }
    if (flags & DUMP_TX) {
        WPRINT_APP_INFO(("TX counters:\n"));
        print_counters_tx(data, normal);
    }
    if (flags & DUMP_RX) {
        WPRINT_APP_INFO(("RX counters:\n"));
        print_counters_rx(data, normal);
    }

    free(data);
    if (seconds)
        free(data2);

    return ERR_CMD_OK;
}

int reset_statistics_counters( int argc, char* argv[] )
{
    UNUSED_PARAMETER( argc );
    UNUSED_PARAMETER( argv );

    if (wwd_reset_statistics_counters() == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }
    else
    {
        WPRINT_APP_INFO(("Counter reset failed.\n"));
        return ERR_UNKNOWN;
    }
}

int start_tx_phyrate_log( int argc, char* argv[] )
{
    UNUSED_PARAMETER( argc );
    UNUSED_PARAMETER( argv );

    if (wwd_phyrate_log(WICED_WIFI_PHYRATE_LOG_TX) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }
    else
    {
        return ERR_UNKNOWN;
    }
}

int start_rx_phyrate_log( int argc, char* argv[] )
{
    UNUSED_PARAMETER( argc );
    UNUSED_PARAMETER( argv );

    if (wwd_phyrate_log(WICED_WIFI_PHYRATE_LOG_RX) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }
    else
    {
        return ERR_UNKNOWN;
    }
}

int stop_phyrate_log( int argc, char* argv[] )
{
    UNUSED_PARAMETER( argc );
    UNUSED_PARAMETER( argv );

    if (wwd_phyrate_log(WICED_WIFI_PHYRATE_LOG_OFF) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }
    else
    {
        return ERR_UNKNOWN;
    }
}

int get_phyrate_cnts( int argc, char* argv[] )
{
    UNUSED_PARAMETER( argc );
    UNUSED_PARAMETER( argv );
    wiced_phyrate_counters_t phyrate_counters;

    if (wwd_get_phyrate_statistics_counters(&phyrate_counters, sizeof(phyrate_counters)) == WWD_SUCCESS)
    {

        WPRINT_APP_INFO(("Received frames at MCS rates 0-3 = %u, %u, %u, %u.\n",(unsigned int)phyrate_counters.rx1mbps,(unsigned int)phyrate_counters.rx2mbps,
                                                                                (unsigned int)phyrate_counters.rx5mbps5,(unsigned int)phyrate_counters.rx6mbps));
        WPRINT_APP_INFO(("Received frames at MCS rates 4-7 = %u, %u, %u, %u.\n",(unsigned int)phyrate_counters.rx9mbps,(unsigned int)phyrate_counters.rx11mbps,
                                                                                (unsigned int)phyrate_counters.rx12mbps,(unsigned int)phyrate_counters.rx18mbps));
        WPRINT_APP_INFO(("Received frames at MCS rates 8-11 = %u, %u, %u, %u.\n",(unsigned int)phyrate_counters.rx24mbps,(unsigned int)phyrate_counters.rx36mbps,
                                                                                 (unsigned int)phyrate_counters.rx48mbps,(unsigned int)phyrate_counters.rx54mbps));
        WPRINT_APP_INFO(("Received frames at MCS rates 12-15 = %u, %u, %u, %u.\n",(unsigned int)phyrate_counters.rx108mbps,(unsigned int)phyrate_counters.rx162mbps,
                                                                                  (unsigned int)phyrate_counters.rx216mbps,(unsigned int)phyrate_counters.rx270mbps));
    }
    else
    {
        return ERR_UNKNOWN;
    }
    return ERR_CMD_OK;
}



/* Get the phyrate log from a numbered start index to a stop index inclusive. */
int get_phyrate_log( int argc, char* argv[] )
{
    wiced_phyrate_log_t *data = NULL;
    float *binned_data = NULL;
    unsigned int start;
    unsigned int stop = 0;
    unsigned int log_size;
    unsigned int bin_size = 0;
    unsigned int i;
    unsigned int ibin = 0;
    float bin_total = 0.0;
    wwd_result_t result;

    if (argc < 1)
    {
        WPRINT_APP_INFO(("Usage: phyrate_dump <bin_size> .lt. %u>\n", WICED_WIFI_PHYRATE_LOG_SIZE));
        return ERR_INSUFFICENT_ARGS;
    }

    bin_size = atoi(argv[1]);

    start = 0;
    result = wwd_get_phyrate_log_size(&log_size);
    stop = log_size - 1;

    if ((start >= stop) || (stop >= WICED_WIFI_PHYRATE_LOG_SIZE))
    {
        WPRINT_APP_INFO(("phyrate_dump: log limits error:: start %u, stop %u.\n", start, stop));
        return ERR_UNKNOWN;
    }


    data = malloc_named( "phyrate_buffer", sizeof(wiced_phyrate_log_t));
    if (!data)
    {
             WPRINT_APP_INFO(("Unable to allocate data buffer!\n"));
             return ERR_UNKNOWN;
    }

    if ((bin_size == 0) || (bin_size > log_size))
    {
        WPRINT_APP_INFO(("Usage: phyrate_dump <bin_size> .lt. %u.\n", WICED_WIFI_PHYRATE_LOG_SIZE));
        WPRINT_APP_INFO(("     : bin_size = %u, bin averaging off.\n", bin_size));
        bin_size = 0;
    }
    else
    {
        WPRINT_APP_INFO(("phyrate_dump: start = %u, stop = %u, bin size = %u.\n", start, stop, bin_size));

        binned_data = malloc_named( "avg_rate_buffer", WICED_WIFI_PHYRATE_LOG_SIZE/bin_size * sizeof(float));
        if (!binned_data)
        {
             WPRINT_APP_INFO(("Unable to allocate binned data buffer!\n"));
             free(data);
             return ERR_UNKNOWN;
        }
    }

    result = wwd_get_phyrate_log(data);

    memset(binned_data, 0, sizeof(*binned_data));

    if (result == WWD_SUCCESS)
    {
        if(bin_size > 0)
        {
            for(i = start;  i <= stop; i++)
            {
                bin_total += data->log[i];
                if ((i % bin_size) == (bin_size - 1))
                {
                    binned_data[ibin++] = (float)bin_total/(float)bin_size;
                    bin_total = 0;
                }
            }
        }
        WPRINT_APP_INFO(("\nIndx MCS Phyrate_kb "));
        if (ibin > 0)
            WPRINT_APP_INFO(( "BIN_AVG Avg Phyrate_Mbps"));
        WPRINT_APP_INFO(("\n"));
        for(i = start;  i <= stop; i++)
        {
           unsigned int whole_avg = binned_data[i];
           unsigned int avg_phyrate_kb;
           if (i < ibin)
           {
               if (binned_data[i] == whole_avg)
                   avg_phyrate_kb = n_phyrates_kb[whole_avg];
               else
               {
                   float fract_avg = binned_data[i] - (float)whole_avg;
                   unsigned int rate_difference = n_phyrates_kb[whole_avg +1] - n_phyrates_kb[whole_avg];
                   unsigned int fract_increase = (unsigned int)(fract_avg * (float)rate_difference);
                   avg_phyrate_kb = n_phyrates_kb[whole_avg] + fract_increase;
               }
           }

           WPRINT_APP_INFO(("%04d  %02u      %5.5u", i, data->log[i], n_phyrates_kb[data->log[i]]));

           if (i < ibin)
                WPRINT_APP_INFO(("     %2.1f         %3.1f", binned_data[i], (float)avg_phyrate_kb/1000));
            WPRINT_APP_INFO(("\n"));
        }
        WPRINT_APP_INFO(("\n"));
    }

    free(data);
    free(binned_data);
    return result;
}

int get_ap_info( int argc, char* argv[] )
{
    wl_bss_info_t ap_info;
    wiced_security_t sec;

    if ( wwd_wifi_get_ap_info( &ap_info, &sec ) == WWD_SUCCESS )
    {
        WPRINT_APP_INFO( ("SSID  : %s\n", (char*)ap_info.SSID ) );
        WPRINT_APP_INFO( ("BSSID : %02X:%02X:%02X:%02X:%02X:%02X\n", ap_info.BSSID.octet[0], ap_info.BSSID.octet[1], ap_info.BSSID.octet[2], ap_info.BSSID.octet[3], ap_info.BSSID.octet[4], ap_info.BSSID.octet[5]) );
        WPRINT_APP_INFO( ("RSSI  : %d\n", ap_info.RSSI) );
        WPRINT_APP_INFO( ("SNR   : %d\n", ap_info.SNR) );
        WPRINT_APP_INFO( ("Noise : %d\n", ap_info.phy_noise) );
        WPRINT_APP_INFO( ("Beacon period : %u\n", ap_info.beacon_period) );
        WPRINT_APP_INFO( ( "Security : %s\n", ( sec == WICED_SECURITY_OPEN )           ? "Open" :
                                                ( sec == WICED_SECURITY_WEP_PSK )        ? "WEP" :
                                                ( sec == WICED_SECURITY_WPA_TKIP_PSK )   ? "WPA TKIP" :
                                                ( sec == WICED_SECURITY_WPA_AES_PSK )    ? "WPA AES" :
                                                ( sec == WICED_SECURITY_WPA2_AES_PSK )   ? "WPA2 AES" :
                                                ( sec == WICED_SECURITY_WPA2_TKIP_PSK )  ? "WPA2 TKIP" :
                                                ( sec == WICED_SECURITY_WPA2_MIXED_PSK ) ? "WPA2 Mixed" :
                                                "Unknown" ) );
    }
    else
    {
        return ERR_UNKNOWN;
    }
    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Leaves an associated access point
 *
 * @return  0 for success, otherwise error
 */

int leave( int argc, char* argv[] )
{
    return wiced_network_down( WICED_STA_INTERFACE );
}

/*!
 ******************************************************************************
 * Prints the device MAC address
 *
 * @return  0 for success, otherwise error
 */

int get_mac_addr( int argc, char* argv[] )
{
    wiced_buffer_t buffer;
    wiced_buffer_t response;
    wiced_mac_t mac;
    wwd_interface_t interface = WWD_STA_INTERFACE;

    memset(&mac, 0, sizeof( wiced_mac_t));

    CHECK_IOCTL_BUFFER( wwd_sdpcm_get_iovar_buffer( &buffer, sizeof(wiced_mac_t), IOVAR_STR_CUR_ETHERADDR ) );

    if (argc == 2 && argv[1][0] == '1')
    {
        interface = WWD_AP_INTERFACE;
    }

    if ( wwd_sdpcm_send_iovar( SDPCM_GET, buffer, &response, interface ) == WWD_SUCCESS )
    {
        memcpy( mac.octet, host_buffer_get_current_piece_data_pointer( response ), sizeof(wiced_mac_t) );
        host_buffer_release( response, WWD_NETWORK_RX );
    }
    WPRINT_APP_INFO(("MAC address is: %02X:%02X:%02X:%02X:%02X:%02X\n", mac.octet[0], mac.octet[1], mac.octet[2], mac.octet[3], mac.octet[4], mac.octet[5]));
    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Enables or disables power save mode as specified by the arguments
 *
 * @return  0 for success, otherwise error
 */

int wifi_powersave( int argc, char* argv[] )
{
    int a = atoi( argv[1] );

    switch( a )
    {
        case 0:
        {
            if ( wwd_wifi_disable_powersave( ) != WWD_SUCCESS )
            {
                WPRINT_APP_INFO( ("Failed to disable Wi-Fi powersave\n") );
            }
            break;
        }

        case 1:
        {
            if ( wwd_wifi_enable_powersave( ) != WWD_SUCCESS )
            {
                WPRINT_APP_INFO( ("Failed to enable Wi-Fi powersave\n") );
            }
            break;
        }

        case 2:
        {
            uint8_t return_to_sleep_delay_ms = (uint8_t) atoi( argv[ 2 ] );

            if ( wwd_wifi_enable_powersave_with_throughput( return_to_sleep_delay_ms ) != WWD_SUCCESS )
            {
                WPRINT_APP_INFO( ("Failed to enable Wi-Fi powersave with throughput\n") );
            }
            break;
        }

        default:
            return ERR_UNKNOWN_CMD;

    }

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Resumes networking after deep-sleep
 *
 * @return  0 for success, otherwise error
 */

int wifi_resume( int argc, char* argv[] )
{
    wiced_network_config_t network_config = WICED_USE_EXTERNAL_DHCP_SERVER;
    wiced_ip_setting_t* ip_settings = NULL;
    wiced_ip_setting_t static_ip_settings;

    if ( argc == 4 )
    {
        network_config = WICED_USE_STATIC_IP;
        str_to_ip( argv[1], &static_ip_settings.ip_address );
        str_to_ip( argv[2], &static_ip_settings.netmask );
        str_to_ip( argv[3], &static_ip_settings.gateway );
        ip_settings = &static_ip_settings;
    }

    if ( wiced_network_resume_after_deep_sleep( WICED_STA_INTERFACE, network_config, ip_settings ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("Failed to resume networking\n") );
    }

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Sets the STA listen interval as specified by the arguments
 *
 * @return  0 for success, otherwise error
 */

int set_listen_interval( int argc, char* argv[] )
{
    int listen_interval;
    int time_unit;

    if ( argc < 3 )
    {
        return ERR_UNKNOWN_CMD;
    }

    /* No bounds checking as console app user cannot know beacon interval or DTIM interval without a sniffer */
    listen_interval = atoi( argv[1] );

    time_unit = atoi( argv[2] );
    if ( ( time_unit != WICED_LISTEN_INTERVAL_TIME_UNIT_BEACON ) && ( time_unit != WICED_LISTEN_INTERVAL_TIME_UNIT_DTIM ) )
    {
        WPRINT_APP_INFO( ("0 for units in Beacon Intervals, 1 for units in DTIM intervals\n") );
        return ERR_UNKNOWN_CMD;
    }

    if ( wiced_wifi_set_listen_interval( (uint8_t)listen_interval, (wiced_listen_interval_time_unit_t)time_unit ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ("Failed to set listen interval\n") );
    }

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Sets the transmit power as specified in arguments in dBm
 *
 * @return  0 for success, otherwise error
 */

int set_tx_power( int argc, char* argv[] )
{
    int dbm = atoi( argv[1] );
    return (wwd_wifi_set_tx_power(dbm) != WWD_SUCCESS );
}

/*!
 ******************************************************************************
 * Gets the current transmit power in dBm
 *
 * @return  0 for success, otherwise error
 */

int get_tx_power( int argc, char* argv[] )
{
    wwd_result_t result;
    uint8_t dbm;

    if ( WWD_SUCCESS != ( result = wwd_wifi_get_tx_power( &dbm ) ) )
    {
        return result;
    }

    WPRINT_APP_INFO(("Transmit Power : %ddBm\n", dbm ));

    return result;
}

/*!
 ******************************************************************************
 * Prints the latest RSSI value
 *
 * @return  0 for success, otherwise error
 */

int get_rssi( int argc, char* argv[] )
{
    int32_t rssi;
    wwd_wifi_get_rssi( &rssi );
    WPRINT_APP_INFO(("RSSI is %d\n", (int)rssi));
    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Prints the latest PHY Noise value
 *
 * @return  0 for success, otherwise error
 */

int get_noise( int argc, char* argv[] )
{
    int32_t noise;
    wwd_wifi_get_noise( &noise );
    WPRINT_APP_INFO(("NOISE is %d\r\n", (int)noise));
    return ERR_CMD_OK;
}

static wiced_result_t
get_bss_info(wl_bss_info_t *bi)
{
    wiced_buffer_t buffer, response;
    wiced_result_t result;

    if (wwd_sdpcm_get_ioctl_buffer( &buffer, WLC_IOCTL_SMLEN) == NULL) {
        WPRINT_APP_INFO(("%s: Unable to malloc WLC_GET_BSS_INFO buffer\n", __FUNCTION__));
        return -1;
    }
    result = wwd_sdpcm_send_ioctl( SDPCM_GET, WLC_GET_BSS_INFO, buffer, &response, WWD_STA_INTERFACE );
    if ( result != WICED_SUCCESS ) {
        WPRINT_APP_INFO(("%s: WLC_GET_BSS_INFO Failed\n", __FUNCTION__));
        return result;
    }
    memcpy(bi, host_buffer_get_current_piece_data_pointer( response )  + 4, sizeof(wl_bss_info_t));

    host_buffer_release( response, WWD_NETWORK_RX );
    return result;
}
/*!
 ******************************************************************************
 * Returns the status of the Wi-Fi interface
 *
 * @return  0 for success, otherwise error
 */

int status( int argc, char* argv[] )
{
    wiced_mac_t       mac;
    wiced_interface_t interface;
    int i;
    char ver_buf[128];
    char *ver;

    wiced_wifi_get_mac_address( &mac );
    wwd_wifi_get_wifi_version(ver_buf, sizeof(ver_buf));
    ver = strstr(ver_buf, "version");
    if (ver)
        ver += strlen("version ");

    WPRINT_APP_INFO(("WICED Version  : " WICED_VERSION "\n"));
    WPRINT_APP_INFO(("Platform       : " PLATFORM "\n"));
    WPRINT_APP_INFO(("Driver & FW    : %s\n", ver));
    WPRINT_APP_INFO(("MAC Address    : %02X:%02X:%02X:%02X:%02X:%02X\n\n", mac.octet[0],mac.octet[1],mac.octet[2],mac.octet[3],mac.octet[4],mac.octet[5]));

    for ( i = 0; i < WICED_INTERFACE_MAX; i++ )
    {
        interface = (wiced_interface_t)i;
        if ( wiced_network_is_ip_up( interface ) )
        {
            switch ( interface )
            {
                case WICED_STA_INTERFACE:
                {
                    wl_bss_info_t bss;

                    wwd_wifi_get_mac_address( &mac, WWD_STA_INTERFACE );
                    WPRINT_APP_INFO( ( "STA Interface\n"));
                    WPRINT_APP_INFO( ( "   MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\n",
                        mac.octet[0],mac.octet[1],mac.octet[2],
                        mac.octet[3],mac.octet[4],mac.octet[5]));

                    if (get_bss_info(&bss) == WICED_SUCCESS ) {
                        bss.SSID[bss.SSID_len] = 0;
                        WPRINT_APP_INFO( ( "   SSID        : %s\n", bss.SSID ) );
                        WPRINT_APP_INFO( ( "   Channel     : %d\n", bss.chanspec & 0xff ) );
                        WPRINT_APP_INFO( ( "   BSSID       : %02X:%02X:%02X:%02X:%02X:%02X\n",
                            bss.BSSID.octet[0],bss.BSSID.octet[1],bss.BSSID.octet[2],
                            bss.BSSID.octet[3],bss.BSSID.octet[4],bss.BSSID.octet[5]));
                    }
                    break;
                }

                case WICED_AP_INTERFACE:
                {
                    wwd_wifi_get_mac_address( &mac, WWD_AP_INTERFACE );
                    WPRINT_APP_INFO( ( "AP Interface\n"));
                    WPRINT_APP_INFO( ( "   MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\n", mac.octet[0],mac.octet[1],mac.octet[2],mac.octet[3],mac.octet[4],mac.octet[5]));
                    WPRINT_APP_INFO( ( "   SSID        : %s\n", last_started_ssid ) );
                    break;
                }

#ifdef COMMAND_CONSOLE_P2P_ENABLED
                case WICED_P2P_INTERFACE:
                {
                    char group_owner[]  = "Group Owner";
                    char group_client[] = "Group Client";
                    char* role          = group_owner;

                    wwd_wifi_get_mac_address( &mac, WWD_P2P_INTERFACE );
                    if ( besl_p2p_group_owner_is_up( ) == WICED_FALSE )
                    {
                        role = group_client;
                    }
                    WPRINT_APP_INFO( ( "P2P Interface\n"));
                    WPRINT_APP_INFO( ( "   Role        : %s\n", role ) );
                    WPRINT_APP_INFO( ( "   MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\n", mac.octet[0],mac.octet[1],mac.octet[2],mac.octet[3],mac.octet[4],mac.octet[5]));
                    WPRINT_APP_INFO( ( "   SSID        : %s\n", p2p_workspace.group_candidate.ssid ) );
                    break;
                }
#endif

#ifdef WICED_USE_ETHERNET_INTERFACE
                case WICED_ETHERNET_INTERFACE:
                {
                    WPRINT_APP_INFO( ( "Ethernet Interface\r\n"));
                    /* XXX need to read the MAC address and print it out */
                    break;
                }
#endif
                /* coverity[Dead default in switch]
                    INTENTIONAL:
                    Coverity does signal a valid bug of Dead 'default' switch case as the execution cannot reach it because
                    the value of variable 'interface' ranges between 0 up to 3 in either true branch or the false branch case.
                    The values 0 up to 3 falls in to one of the valid values of the enum 'wiced_interface_t' leaving default case being never
                    called at all. On other hand wiced_interface_t being an enum there could be additional interface elements be added tomorrow in which case
                    having a default switch case would cater to such a situation and having no default case would cause GNU C compiler to warn us during compilation.
                    Hence marking this bug as intentional and keeping the default switch case as it is. */
                default:
                    break;
            }
            network_print_status( interface );
        }
        else
        {
            switch ( interface )
            {
                case WICED_STA_INTERFACE:
                    WPRINT_APP_INFO( ( "STA Interface  : Down\n") );
                    break;

                case WICED_AP_INTERFACE:
                    WPRINT_APP_INFO( ( "AP Interface   : Down\n") );
                    break;

#ifdef COMMAND_CONSOLE_P2P_ENABLED
                case WICED_P2P_INTERFACE:
                    WPRINT_APP_INFO( ( "P2P Interface  : Down\n") );
                    break;
#endif

#ifdef WICED_USE_ETHERNET_INTERFACE
                case WICED_ETHERNET_INTERFACE:
                    WPRINT_APP_INFO( ( "Ethernet Interface  : Down\n") );
                    break;
#endif

                /* coverity[Dead default in switch]
                    INTENTIONAL:
                    Coverity does signal a valid bug of Dead 'default' switch case as the execution cannot reach it because
                    the value of variable 'interface' ranges between 0 up to 3 in either true branch or the false branch case.
                    The values 0 up to 3 falls in to one of the valid values of the enum 'wiced_interface_t' leaving default case being never
                    called at all. On other hand wiced_interface_t being an enum there could be additional interface elements be added tomorrow in which case
                    having a default switch case would cater to such a situation and having no default case would cause GNU C compiler to warn us during compilation.
                    Hence marking this bug as intentional and keeping the default switch case as it is. */
                default:
                    break;
            }
        }
    }

    return ERR_CMD_OK;
}


int antenna( int argc, char* argv[] )
{
    uint32_t value;
    string_to_unsigned( argv[1], strlen(argv[1]), &value, 0);
    if ( ( value == WICED_ANTENNA_1 ) || ( value == WICED_ANTENNA_2 ) || ( value == WICED_ANTENNA_AUTO ) )
    {
        if ( wwd_wifi_select_antenna( (wiced_antenna_t) value ) == WWD_SUCCESS )
        {
            return ERR_CMD_OK;
        }
    }
    return ERR_UNKNOWN;
}

int ant_sel( int argc, char* argv[] )
{
    wiced_buffer_t buffer;
    uint32_t value;
    string_to_unsigned( argv[1], strlen(argv[1]), &value, 0);
    wlc_antselcfg_t* sel = (wlc_antselcfg_t*)wwd_sdpcm_get_iovar_buffer(&buffer, sizeof(wlc_antselcfg_t), "nphy_antsel");
    CHECK_IOCTL_BUFFER( sel );
    sel->ant_config[0] = value;
    sel->ant_config[1] = value;
    sel->ant_config[2] = value;
    sel->ant_config[3] = value;
    sel->num_antcfg = 0;
    if (wwd_sdpcm_send_iovar(SDPCM_SET, buffer, NULL, WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }
    else
    {
        return ERR_UNKNOWN;
    }
}

int antdiv( int argc, char* argv[] )
{
    wiced_buffer_t buffer;
    uint32_t* data = wwd_sdpcm_get_ioctl_buffer(&buffer, sizeof(uint32_t));
    CHECK_IOCTL_BUFFER( data );
    uint32_t tmp_val;
    string_to_unsigned( argv[1], strlen(argv[1]), &tmp_val, 0 );
    *data = tmp_val;
    if (wwd_sdpcm_send_ioctl(SDPCM_SET, WLC_SET_ANTDIV, buffer, NULL, WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }
    else
    {
        return ERR_UNKNOWN;
    }
}

int txant( int argc, char* argv[] )
{
    wiced_buffer_t buffer;
    uint32_t* data = wwd_sdpcm_get_ioctl_buffer(&buffer, sizeof(uint32_t));
    CHECK_IOCTL_BUFFER( data );
    uint32_t tmp_val;
    string_to_unsigned( argv[1], strlen(argv[1]), &tmp_val, 0 );
    *data = tmp_val;
    if (wwd_sdpcm_send_ioctl(SDPCM_SET, WLC_SET_TXANT, buffer, NULL, WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }
    else
    {
        return ERR_UNKNOWN;
    }
}

int ucantdiv( int argc, char* argv[] )
{
    wiced_buffer_t buffer;
    uint32_t* data = wwd_sdpcm_get_ioctl_buffer(&buffer, sizeof(uint32_t));
    CHECK_IOCTL_BUFFER( data );
    uint32_t tmp_val;
    string_to_unsigned( argv[1], strlen(argv[1]), &tmp_val, 0 );
    *data = tmp_val;
    if (wwd_sdpcm_send_ioctl(SDPCM_SET, WLC_SET_UCANTDIV, buffer, NULL, WWD_STA_INTERFACE) == WWD_SUCCESS)
    {
        return ERR_CMD_OK;
    }
    else
    {
        return ERR_UNKNOWN;
    }
}

int get_country( int argc, char* argv[] )
{
    /* Get country information and print the abbreviation */
    wl_country_t cspec;
    wiced_buffer_t buffer;
    wiced_buffer_t response;

    wl_country_t* temp = (wl_country_t*)wwd_sdpcm_get_iovar_buffer( &buffer, sizeof( wl_country_t ), "country" );
    CHECK_IOCTL_BUFFER( temp );
    memset( temp, 0, sizeof(wl_country_t) );
    wwd_result_t result = wwd_sdpcm_send_iovar( SDPCM_GET, buffer, &response, WWD_STA_INTERFACE );

    if (result == WWD_SUCCESS)
    {
        memcpy( (char *)&cspec, (char *)host_buffer_get_current_piece_data_pointer( response ), sizeof(wl_country_t) );
        host_buffer_release(response, WWD_NETWORK_RX);
        WPRINT_APP_INFO(( "Country is %s (%s/%ld)\n", cspec.country_abbrev , cspec.ccode, cspec.rev ));
    }
    else
    {
        WPRINT_APP_INFO(("country iovar not supported, trying ioctl\n"));
        temp = (wl_country_t*) wwd_sdpcm_get_ioctl_buffer( &response, sizeof(wl_country_t) );
        CHECK_IOCTL_BUFFER( temp );
        memset( temp, 0, sizeof( wl_country_t ) );
        result = wwd_sdpcm_send_ioctl( SDPCM_GET, WLC_GET_COUNTRY, buffer, &response, WWD_STA_INTERFACE );
        if ( result == WWD_SUCCESS )
        {
            memcpy( (char *)&cspec, (char *)host_buffer_get_current_piece_data_pointer( response ), sizeof(wl_country_t) );
            host_buffer_release(response, WWD_NETWORK_RX);
        }
    }

    return result;
}

int set_country( int argc, char* argv[] )
{
   wl_country_t*  country_struct;
   wl_country_t cspec = {{0}, 0, {0}};
   wiced_buffer_t buffer;
   int retval;
   /* Check argument list count */
   if ( argc > 2 )
   {
       return ERR_TOO_MANY_ARGS;
   }
   country_struct = (wl_country_t*) wwd_sdpcm_get_iovar_buffer( &buffer,
           (uint16_t) sizeof(wl_country_t), IOVAR_STR_COUNTRY );
   if ( country_struct == NULL )
   {
       wiced_assert( "Could not get buffer for IOCTL", 0 != 0 );
       return WWD_BUFFER_ALLOC_FAIL;
   }
   memset( country_struct, 0, sizeof(wl_country_t) );
   /* Parse a country specification, e.g. "US/1", or a country code.
    * cspec.rev will be -1 if not specified.
    */
   retval = parse_country_spec( argv[1], cspec.country_abbrev, &cspec.rev );
   if ( retval == ERR_CMD_OK )
   {
       memcpy( country_struct->country_abbrev, cspec.country_abbrev, WLC_CNTRY_BUF_SZ );
       memcpy( country_struct->ccode, cspec.country_abbrev, WLC_CNTRY_BUF_SZ );
       country_struct->rev = cspec.rev;
       retval = wwd_sdpcm_send_iovar( SDPCM_SET, buffer, 0, WWD_STA_INTERFACE );
   }
   else
   {
       WPRINT_APP_INFO(( "%s Country is not supported by firmware\n", argv[1] ));
   }

   return retval;
}

int get_rate (int argc, char* argv[])
{
    uint32_t rate;
    wwd_wifi_get_rate(WWD_STA_INTERFACE, &rate);

    if (rate == 0)
        WPRINT_APP_INFO(("auto"));
    else if (rate > 1000) /* this indicates that units are kbps */
        WPRINT_APP_INFO(("%u Kbps", (unsigned int)rate));
    else
        WPRINT_APP_INFO(("%u%s Mbps", ((unsigned int)rate / 2), ((unsigned int)rate & 1) ? ".5" : ""));

    WPRINT_APP_INFO(("\n"));

    return ERR_CMD_OK;
}

int set_legacy_rate (int argc, char* argv[])
{
    uint32_t rate;

    rate = (uint32_t)(2 * atof( argv[1] ));

    if (WWD_SUCCESS != wwd_wifi_set_legacy_rate(WWD_STA_INTERFACE, rate))
        WPRINT_APP_INFO(("Invalid legacy rate rate specification\n"));

    return ERR_CMD_OK;
}

int disable_11n (int argc, char* argv[])
{
    if (WWD_SUCCESS != wwd_wifi_set_11n_support(WWD_STA_INTERFACE, WICED_11N_SUPPORT_DISABLED))
        WPRINT_APP_INFO(("Cannot disable 11n mode\n"));

    return ERR_CMD_OK;
}

int enable_11n (int argc, char* argv[])
{
    if (WWD_SUCCESS != wwd_wifi_set_11n_support(WWD_STA_INTERFACE, WICED_11N_SUPPORT_ENABLED))
        WPRINT_APP_INFO(("Cannot enable 11n mode\n"));

    return ERR_CMD_OK;
}

int set_mcs_rate (int argc, char* argv[])
{
    int32_t mcs;
    wiced_bool_t mcsonly = WICED_FALSE;

    mcs = (int32_t)(atoi( argv[1] ));
    if (argc == 3)
        mcsonly = (wiced_bool_t)(atoi( argv[2]));

    if (WWD_SUCCESS != wwd_wifi_set_mcs_rate(WWD_STA_INTERFACE, mcs, mcsonly))
        WPRINT_APP_INFO(("Invalid MCS rate specification\n"));

    return ERR_CMD_OK;
}

int set_data_rate( int argc, char* argv[] )
{
    wiced_buffer_t buffer;
    uint32_t*      data;
    wwd_result_t   result;
    uint32_t       rate;

    data = wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, "bg_rate" );
    CHECK_IOCTL_BUFFER( data );

    /* Set data to 2 * <rate> as legacy rate unit is in 0.5Mbps */
    rate  = (uint32_t)(2 * atof( argv[1] ));
    *data = rate;

    if ( ( result = wwd_sdpcm_send_iovar( SDPCM_SET, buffer, 0, WWD_STA_INTERFACE ) ) != WWD_SUCCESS )
    {
        if ( result == WWD_WLAN_UNSUPPORTED )
        {
            data = wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, "2g_rate" );
            CHECK_IOCTL_BUFFER( data );
            *data = rate;
            result = wwd_sdpcm_send_iovar( SDPCM_SET, buffer, 0, WWD_STA_INTERFACE );
        }
    }

    if ( result != WWD_SUCCESS )
    {
        WPRINT_APP_INFO(("Unable to set rate %u\n", (unsigned int)result));
        return ERR_UNKNOWN;
    }

    return ERR_CMD_OK;
}

int get_data_rate( int argc, char* argv[] )
{
    wiced_buffer_t buffer;
    wiced_buffer_t response;
    uint32_t*      data;
    wwd_result_t   result;


    data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, "bg_rate" );
    CHECK_IOCTL_BUFFER( data );

    if ( ( result = wwd_sdpcm_send_iovar( SDPCM_GET, buffer, &response, WWD_STA_INTERFACE ) ) != WWD_SUCCESS )
    {
        if ( result == WWD_WLAN_UNSUPPORTED )
        {
            data = (uint32_t*) wwd_sdpcm_get_iovar_buffer( &buffer, (uint16_t) 4, "2g_rate" );
            CHECK_IOCTL_BUFFER( data );
            result = wwd_sdpcm_send_iovar( SDPCM_GET, buffer, &response, WWD_STA_INTERFACE );
        }
    }

    if ( result != WWD_SUCCESS )
    {
        WPRINT_APP_INFO(("Unable to get data rate %u\n", (unsigned int)result));
        return ERR_UNKNOWN;
    }

    data = (uint32_t*) host_buffer_get_current_piece_data_pointer( response );

    *data &= 0x000000FF;

    /* 5.5 Mbps */
    if ( *data == 11 )
    {
        WPRINT_APP_INFO(( "data rate: 5.5 Mbps\n\r" ));
    }
    else
    {
        WPRINT_APP_INFO(( "data rate: %d Mbps\n\r", (int)(*data / 2) ));

    }

    host_buffer_release( response, WWD_NETWORK_RX );
    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Interface to the wiced_crypto_get_random() function. Prints result
 *
 * @return  0 for success, otherwise error
 */
int get_random( int argc, char* argv[] )
{
    uint8_t random[64];
    if ( wiced_crypto_get_random( random, 64 ) == WICED_SUCCESS )
    {
        int a;
        WPRINT_APP_INFO(("Random data is 0x"));
        for (a=0; a<64; ++a)
        {
            WPRINT_APP_INFO(("%.2x", random[a]));
        }
        WPRINT_APP_INFO(("\n"));
        return ERR_CMD_OK;
    }

    return ERR_UNKNOWN;
}

/*!
 ******************************************************************************
 * Get the access categories being used in STA mode
 *
 * @return  0 for success
 */
int get_access_category_parameters_sta( int argc, char* argv[] )
{
    edcf_acparam_t ac_params[AC_COUNT];
    int ac_priority[AC_COUNT];

    if ( wwd_wifi_get_acparams_sta( ac_params ) != WWD_SUCCESS )
    {
        WPRINT_APP_INFO(( "Error reading EDCF AC Parameters\n"));
    }

    wwd_wifi_prioritize_acparams( ac_params, ac_priority ); // Re-prioritize access categories to match AP configuration
    ac_params_print( ac_params, ac_priority );

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Read WLAN chip console buffer and output to host console
 *
 * @return  0 for success, otherwise error
 */
int read_wlan_chip_console_log( int argc, char* argv[] )
{
    const unsigned buffer_size = 200;
    int result = ERR_UNKNOWN;

    char* buffer = malloc_named( "console", buffer_size );
    if ( buffer == NULL )
    {
        return result;
    }

    if ( wwd_wifi_read_wlan_log( buffer, buffer_size ) == WWD_SUCCESS )
    {
        result = ERR_CMD_OK;
    }

    free( buffer );

    return result;
}

int peek(int argc, char* argv[])
{
    unsigned int *addr;
    unsigned int value;

    addr = (unsigned int *)((strtol( argv[1], (char **)NULL, 16 )) & 0xFFFFFFFC);

    if (addr != NULL)
    {
        wwd_bus_set_backplane_window( (uint32_t) addr );
        memcpy ((void *)&value, (void *)addr, sizeof(value));
        WPRINT_APP_INFO(("addr 0x%x = 0x%x.\n", (unsigned int)addr, value));
    }
    else
    {
        WPRINT_APP_INFO(("Usage: peek <hex address>\n"));
    }

    return ERR_CMD_OK;
}

int poke(int argc, char* argv[])
{
    volatile unsigned int *addr;
    unsigned int value = 0xDEADF00D;

    addr  = (unsigned int *)((strtol( argv[1], (char **)NULL, 16 )) & 0xFFFFFFFC);
    value = (unsigned int)   (strtol( argv[2], (char **)NULL, 16 ));

    if (addr != NULL)
    {
        if (value != 0xDEADF00D)
            *addr = value;
        WPRINT_APP_INFO(("addr 0x%x = 0x%x (wrote 0x%x).\n", (unsigned int)addr, *addr, value));
    }
    else
    {
        WPRINT_APP_INFO(("Usage: peek <hex address>\n"));
    }

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Sets the preferred band for association. This is only useful for devices that support more than one band.
 *
 * @return  0 for success, otherwise error
 */

int set_preferred_association_band( int argc, char* argv[] )
{
    int32_t band = 0;

    band = atoi( argv[1] );

    if ( ( argc == 2 ) && ( band ) >= 0 && ( band <= WLC_BAND_2G ) )
    {
        if ( wwd_wifi_set_preferred_association_band( band ) != WWD_SUCCESS )
        {
            WPRINT_APP_INFO( ("Failed to set preferred band for association.\n") );
        }
    }
    else
    {
        return ERR_UNKNOWN;
    }

    return ERR_CMD_OK;
}

/*!
 ******************************************************************************
 * Gets the preferred band for association.
 *
 * @return  0 for success, otherwise error
 */

int get_preferred_association_band( int argc, char* argv[] )
{
    int32_t                     band = 0;

    if ( wwd_wifi_get_preferred_association_band( &band ) == WWD_SUCCESS )
    {
        switch ( band )
        {
            case WLC_BAND_AUTO:
                WPRINT_APP_INFO( ("Preferred band for association is set to auto\n") );
                break;

            case WLC_BAND_5G:
                WPRINT_APP_INFO( ("Preferred band for association is 5GHz\n") );
                break;

            case WLC_BAND_2G:
                WPRINT_APP_INFO( ("Preferred band for association is 2.4GHz\n") );
                break;

            default:
                WPRINT_APP_INFO( ("Preferred band for association is unknown\n") );
        }
    }
    else
    {
        WPRINT_APP_INFO( ("Unable to read preferred band for association. It is not set by default so try setting it before reading it back.\n") );
    }

    return ERR_CMD_OK;
}

void dump_bytes(const uint8_t* bptr, uint32_t len)
{
    int i = 0;

    for (i = 0; i < len; )
    {
        if ((i & 0x0f) == 0)
        {
            WPRINT_APP_INFO( ( "\n" ) );
        }
        else if ((i & 0x07) == 0)
        {
            WPRINT_APP_INFO( (" ") );
        }
        WPRINT_APP_INFO( ( "%02x ", bptr[i++] ) );
    }
    WPRINT_APP_INFO( ( "\n" ) );
}

void ac_params_print( const wiced_edcf_ac_param_t *acp, const int *priority )
{
    int aci;
    int acm, aifsn, ecwmin, ecwmax, txop;
    static const char ac_names[AC_COUNT][6] = {"AC_BE", "AC_BK", "AC_VI", "AC_VO"};

    if ( acp != NULL )
    {
        for (aci = 0; aci < AC_COUNT; aci++, acp++)
        {
            if (((acp->ACI & EDCF_ACI_MASK) >> EDCF_ACI_SHIFT) != aci)
            {
                WPRINT_APP_INFO(("Warning: AC params out of order\n"));
            }
            acm = (acp->ACI & EDCF_ACM_MASK) ? 1 : 0;
            aifsn = acp->ACI & EDCF_AIFSN_MASK;
            ecwmin = acp->ECW & EDCF_ECWMIN_MASK;
            ecwmax = (acp->ECW & EDCF_ECWMAX_MASK) >> EDCF_ECWMAX_SHIFT;
            txop = (uint16_t)acp->TXOP;
            WPRINT_APP_INFO(("%s: raw: ACI 0x%x ECW 0x%x TXOP 0x%x\n", ac_names[aci], acp->ACI, acp->ECW, txop));
            WPRINT_APP_INFO(("       dec: aci %d acm %d aifsn %d " "ecwmin %d ecwmax %d txop 0x%x\n", aci, acm, aifsn, ecwmin, ecwmax, txop) );
                /* CWmin = 2^(ECWmin) - 1 */
                /* CWmax = 2^(ECWmax) - 1 */
                /* TXOP = number of 32 us units */
            WPRINT_APP_INFO(("       eff: CWmin %d CWmax %d TXop %dusec\n", EDCF_ECW2CW(ecwmin), EDCF_ECW2CW(ecwmax), EDCF_TXOP2USEC(txop)));
        }
    }

    if ( priority != NULL )
    {
        for (aci = 0; aci < AC_COUNT; aci++, priority++)
        {
            WPRINT_APP_INFO(("%s: ACI %d Priority %d\n", ac_names[aci], aci, *priority));
        }
    }
}


/*!
 ******************************************************************************
 * Convert a security authentication type string to a wiced_security_t.
 *
 * @param[in] arg  The string containing the value.
 *
 * @return    The value represented by the string.
 */
wiced_security_t str_to_authtype( char* arg )
{
    if ( strcmp( arg, "open" ) == 0 )
    {
        return WICED_SECURITY_OPEN;
    }
    else if ( strcmp( arg, "wep" ) == 0 )
    {
        return WICED_SECURITY_WEP_PSK;
    }
    else if ( strcmp( arg, "wep_shared" ) == 0 )
    {
        return WICED_SECURITY_WEP_SHARED;
    }
    else if ( strcmp( arg, "wpa2_tkip" ) == 0 )
    {
        return WICED_SECURITY_WPA2_TKIP_PSK;
    }
    else if ( strcmp( arg, "wpa2_aes" ) == 0 )
    {
        return WICED_SECURITY_WPA2_AES_PSK;
    }
    else if ( strcmp( arg, "wpa2" ) == 0 )
    {
        return WICED_SECURITY_WPA2_MIXED_PSK;
    }
    else if ( strcmp( arg, "wpa_aes" ) == 0 )
    {
        return WICED_SECURITY_WPA_AES_PSK;
    }
    else if ( strcmp( arg, "wpa_tkip" ) == 0 )
    {
        return WICED_SECURITY_WPA_TKIP_PSK;
    }
    else
    {
        WPRINT_APP_INFO( ("Bad auth type: '%s'\r\n", arg) );
        return WICED_SECURITY_UNKNOWN;
    }
}


/*!
 ******************************************************************************
 * Convert a security authentication type string to an enterprise wiced_security_t.
 *
 * @param[in] arg  The string containing the value.
 *
 * @return    The value represented by the string.
 */
wiced_security_t str_to_enterprise_authtype( char* arg )
{
    if ( strcmp( arg, "wpa2_tkip" ) == 0 )
    {
        return WICED_SECURITY_WPA2_TKIP_ENT;
    }
    else if ( strcmp( arg, "wpa2" ) == 0 )
    {
        return WICED_SECURITY_WPA2_MIXED_ENT;
    }
    else if ( strcmp( arg, "wpa" ) == 0 )
    {
        return WICED_SECURITY_WPA_MIXED_ENT;
    }
    else if ( strcmp( arg, "wpa_tkip" ) == 0 )
    {
        return WICED_SECURITY_WPA_TKIP_ENT;
    }
    else
    {
        WPRINT_APP_INFO( ("Bad auth type: '%s'\r\n", arg) );
        return WICED_SECURITY_UNKNOWN;
    }
}

/*!
 ******************************************************************************
 * Convert a security enterprise type string to an eap_type_t type.
 *
 * @param[in] arg  The string containing the value.
 *
 * @return    The value represented by the string.
 */
static eap_type_t str_to_enterprise_security_type( char* arg )
{
    if ( strcmp( arg, "eap_tls" ) == 0 )
    {
        return EAP_TYPE_TLS;
    }
    else if ( strcmp( arg, "peap" ) == 0 )
    {
        return EAP_TYPE_PEAP;
    }
    else
    {
        WPRINT_APP_INFO( ("Bad EAP type: '%s'\r\n", arg) );
        return EAP_TYPE_NONE;
    }
}


/*!
 ******************************************************************************
 * Convert a MAC string (xx:xx:xx:xx:xx) to a wiced_mac_t.
 *
 * @param[in] arg  The string containing the value.
 *
 * @return    The value represented by the string.
 */
void str_to_mac( char* arg, wiced_mac_t* mac )
{
    char* start = arg;
    char* end;
    int a = 0;
    do
    {
        uint32_t tmp_val;
        end = strchr( start, ':' );
        if ( end != NULL )
        {
            *end = '\0';
        }
        string_to_unsigned( start, 2, &tmp_val, 1);
        mac->octet[a] = (uint8_t) tmp_val;
        if ( end != NULL )
        {
            *end = ':';
            start = end + 1;
        }
        ++a;
    } while ( a < 6 && end != NULL );
}


/*!
 ******************************************************************************
 * Analyse failed join result
 *
 * @param[in] join_result  Result of join attempts.
 *
 * @return
 */
void analyse_failed_join_result( wiced_result_t join_result )
{
    /* Note that DHCP timeouts and EAPOL key timeouts may happen at the edge of the cell. If possible move closer to the AP. */
    /* Also note that the default number of join attempts is three and the join result is returned for the last attempt. */
    WPRINT_APP_INFO( ("Join result %u: ", (unsigned int)join_result) );
    switch( join_result )
    {
        case WICED_ERROR:
            WPRINT_APP_INFO( ("General error\n") ); /* Getting a DHCP address may fail if at the edge of a cell and the join may timeout before DHCP has completed. */
            break;

        case WWD_NETWORK_NOT_FOUND:
            WPRINT_APP_INFO( ("Failed to find network\n") ); /* Check that he SSID is correct and that the AP is up */
            break;

        case WWD_NOT_AUTHENTICATED:
            WPRINT_APP_INFO( ("Failed to authenticate\n") ); /* May happen at the edge of the cell. Try moving closer to the AP. */
            break;

        case WWD_EAPOL_KEY_PACKET_M1_TIMEOUT:
            WPRINT_APP_INFO( ("Timeout waiting for first EAPOL key frame from AP\n") );
            break;

        case WWD_EAPOL_KEY_PACKET_M3_TIMEOUT:
            WPRINT_APP_INFO( ("Check the passphrase and try again\n") ); /* The M3 timeout will occur if the passphrase is incorrect */
            break;

        case WWD_EAPOL_KEY_PACKET_G1_TIMEOUT:
            WPRINT_APP_INFO( ("Timeout waiting for group key from AP\n") );
            break;

        case WWD_INVALID_JOIN_STATUS:
            WPRINT_APP_INFO( ("Some part of the join process did not complete\n") ); /* May happen at the edge of the cell. Try moving closer to the AP. */
            break;

        default:
            WPRINT_APP_INFO( ("\n") );
            break;
    }
}


void network_print_status( wiced_interface_t interface )
{
    if ( wiced_network_is_ip_up( interface ) )
    {
        wiced_ip_address_t netmask;
        wiced_ip_address_t gateway;
        wiced_ip_address_t ipv4_address;
        wiced_ip_get_netmask( interface, &netmask );
        wiced_ip_get_gateway_address( interface, &gateway );
        wiced_ip_get_ipv4_address( interface, &ipv4_address );
        WPRINT_APP_INFO( ( "   IP Addr     : %u.%u.%u.%u\n", (uint8_t)(GET_IPV4_ADDRESS(ipv4_address) >> 24),
                                                             (uint8_t)(GET_IPV4_ADDRESS(ipv4_address) >> 16),
                                                             (uint8_t)(GET_IPV4_ADDRESS(ipv4_address) >> 8),
                                                             (uint8_t)(GET_IPV4_ADDRESS(ipv4_address) >> 0) ) );

        WPRINT_APP_INFO( ( "   Gateway     : %u.%u.%u.%u\n", (uint8_t)(GET_IPV4_ADDRESS(gateway) >> 24),
                                                             (uint8_t)(GET_IPV4_ADDRESS(gateway) >> 16),
                                                             (uint8_t)(GET_IPV4_ADDRESS(gateway) >> 8),
                                                             (uint8_t)(GET_IPV4_ADDRESS(gateway) >> 0) ) );

        WPRINT_APP_INFO( ( "   Netmask     : %u.%u.%u.%u\n", (uint8_t)(GET_IPV4_ADDRESS(netmask) >> 24),
                                                             (uint8_t)(GET_IPV4_ADDRESS(netmask) >> 16),
                                                             (uint8_t)(GET_IPV4_ADDRESS(netmask) >> 8),
                                                             (uint8_t)(GET_IPV4_ADDRESS(netmask) >> 0) ) );
    }
}

/*!
 ******************************************************************************
 * Find AP
 *
 * @return  0 for success, otherwise error
 */

int find_ap( int argc, char* argv[] )
{
    wiced_scan_result_t ap_info;
    if ( wiced_wifi_find_ap( argv[ 1 ], &ap_info, NULL ) == WICED_SUCCESS )
    {
        print_scan_result( &ap_info );
    }
    else
    {
        WPRINT_APP_INFO( ("\"%s\" not found\n", argv[1] ) );
    }

    return ERR_CMD_OK;
}

/* Set and Get RRM Capabilities */
int set_get_rrm(int argc, char* argv[])
{
   radio_resource_management_capability_ie_t rrm_cap;
   char *endptr = NULL;
   char *s;
   /* Input number with prefix 0x */
   char c[32] = "0x";
   int ret = ERR_CMD_OK;
   uint32_t hval = 0, val = 0, len;
   uint32_t high = 0, low = 0, bit = 0, hbit = 0;
   uint32_t init_hval = 0, init_low = 0;
   char str[32];
   wiced_bool_t found = WICED_FALSE;
   uint32_t rmcap_add = 0, rmcap2_add = 0;
   uint32_t rmcap_del = 0, rmcap2_del = 0;
   radio_resource_management_capability_debug_msg_t *dbg_msg = rrm_msg;
   int i;

   memset(&rrm_cap, 0, sizeof(radio_resource_management_capability_ie_t));
   argv++;

   if( *argv == NULL )
   {
       WPRINT_APP_INFO( (" Bad Usage\n" ) );
       return ERR_UNKNOWN;
   }
   if ( strcmp( *argv, "set" ) == 0 )
   {
       wwd_wifi_get_radio_resource_management_capabilities(WWD_STA_INTERFACE, &rrm_cap);
       init_hval =  rrm_cap.radio_resource_management[4];
       init_low = (rrm_cap.radio_resource_management[3] << 24) | (rrm_cap.radio_resource_management[2] << 16) \
                       | (rrm_cap.radio_resource_management[1] << 8) | rrm_cap.radio_resource_management[0];
       argv++;
       s = (char *)*argv;

       while (*argv)
       {
           found = WICED_FALSE;
           val = strtoul(s, &endptr, 0);
           if ( *s == '+' || *s == '-' )
           {
                   s++;
           }
           else
           {
               /* used for clearing previous value */
               rmcap_del = 0;
               rmcap2_del = 0;
           }

           val = strtoul(s, &endptr, 0);
           /* Input is decimal number or hex with prefix 0x and > 32 bits */
           if ( val == 0xFFFFFFFF )
           {
               if ( !(*s == '0' && *(s+1) == 'x') )
               {
                      WPRINT_APP_INFO(("bits > 32 take only numerical input in hex\n"));
                      val = 0;
                      return ERR_UNKNOWN;
               }
               else
               {

                    len = strlen(s);
                    hval = strtoul(strncpy(str, s, len-8), &endptr, 0);
                    *endptr = 0;
                     s = s + strlen(str);
                     s = strcat(c, s);
                     val = strtoul(s, &endptr, 0);
                     /* Input number > 64bit */
                     if ( hval == 0xFFFFFFFF )
                     {
                           WPRINT_APP_INFO(( "Invalid entry for Radio Resource Management Capabilities\n" ));
                           return ERR_UNKNOWN;
                     }
               }
         }
         if ( *endptr != '\0' )
         {

                  for ( i = 0; ((bit = dbg_msg[i].value) <= DOT11_RRM_CAP_BSSAAD ); i++)
                  {
                         if ( strcasecmp(dbg_msg[i].string, s) == 0 )
                         {
                              found = WICED_TRUE;
                              break;
                         }
                  }
                  if ( !found )
                  {
                          for ( ; (hbit = dbg_msg[i].value) <= DOT11_RRM_CAP_AI; i++ ) {
                              if ( strcasecmp(dbg_msg[i].string, s ) == 0) {
                                  break;
                              }
                          }
                          if ( hbit )
                              hval = 1 << (hbit - DOT11_RRM_CAP_BSSAAC);
                          else
                              hval = 0;
                   }
                   else
                   {
                           val = 1 << bit;
                   }
                  if ( !val && !hval )
                       return ERR_UNKNOWN;

         }

          if ( **argv == '-' )  {
                    rmcap_del |= val;
                    if ( !found )
                         rmcap2_del |= hval;
          }
          else
          {
                   rmcap_add |= val;
                   if (!found)
                         rmcap2_add |= hval;
         }
          argv++;
       }
       /* if rrm set 0x0 then clear everything */
       if ( ( rmcap_add == 0 ) && ( rmcap2_del == 0 ) && ( rmcap2_add == 0 ) && ( rmcap2_del == 0 ) ) {
           memset ( &rrm_cap, 0, sizeof(radio_resource_management_capability_ie_t ) );
       }
       else
       {
           low = init_low & ~rmcap_del;
           high = init_hval & ~rmcap2_del;
           low =  low | rmcap_add;
           high = high | rmcap2_add;

           rrm_cap.radio_resource_management[4] = high;
           rrm_cap.radio_resource_management[3] = (low & 0xff000000) >> 24;
           rrm_cap.radio_resource_management[2] = (low & 0x00ff0000) >> 16;
           rrm_cap.radio_resource_management[1] = (low & 0x0000ff00) >> 8;
           rrm_cap.radio_resource_management[0] = low & 0x000000ff;
       }

       wwd_wifi_set_radio_resource_management_capabilities( WWD_STA_INTERFACE, &rrm_cap );
       WPRINT_APP_INFO(( "RRM CAPABILITIES: +enabled -disabled\n" ) );
       print_rrm_caps( &rrm_cap );
   }
   else if (strcmp(*argv, "get") == 0 )
   {
       WPRINT_APP_INFO(( "RRM CAPABILITIES: +enabled -disabled\n" ) );
       wwd_wifi_get_radio_resource_management_capabilities ( WWD_STA_INTERFACE, &rrm_cap );
       print_rrm_caps ( &rrm_cap );
       dump_bytes ( (uint8_t *)&rrm_cap, sizeof(radio_resource_management_capability_ie_t) );
   }
   else
   {
        ret = ERR_UNKNOWN;
   }
   return ret;
}

/* Print RRM Capabilities */
int print_rrm_caps(radio_resource_management_capability_ie_t *rrm_cap)
{
    int i, j;
    radio_resource_management_capability_debug_msg_t *dbg_msg = rrm_msg;
    if ( rrm_cap != NULL )
    {
        for ( i = 0 ; i < RRM_CAPABILITIES_LEN; i++ )
        {
            for ( j = 0; j < 8; j++ )
            {
                if ( (i * 7) + j <=  DOT11_RRM_CAP_AI )
                {
                    const char *str = dbg_msg[(i * 7) + j].string;
                    if ( strcmp(str, "unused") != 0 )
                    {
                        if ( rrm_cap->radio_resource_management[i] & (1 << j) )
                        {
                            WPRINT_APP_INFO ( ( "bit %d +%s\n", ((i * 7) + j), str) );
                        }
                        else
                        {
                            WPRINT_APP_INFO ( ("bit %d -%s\n", ((i * 7) + j), str) );
                        }
                    }
                }
            }
        }
    }

    return ERR_CMD_OK;
}

/* Params: [bcn mode] [da] [duration] [random int] [channel] [ssid] [repetitions] */
int rrm_bcn_req(int argc, char* argv[])
{

   radio_resource_management_beacon_req_t rrm_bcn_req;
   uint32_t len;

   memset( &rrm_bcn_req, 0, sizeof(radio_resource_management_beacon_req_t) );

   if ( argv[1] )
   {
       /* beacon mode: ACTIVE/PASSIVE/SCAN_CACHE */
       rrm_bcn_req.bcn_mode = htod32( strtoul(argv[1], NULL, 0) );
       if ( rrm_bcn_req.bcn_mode > 2 )
       {
           WPRINT_APP_INFO(("wl_rrm_bcn_req parsing bcn mode failed\n"));
               return ERR_UNKNOWN;
       }
   }
   /* destination MAC address */
   if ( argv[2] )
   {
       str_to_mac((char *)argv[2], &rrm_bcn_req.da);
   }

   /* duration */
   if( argv[3] )
   {
      rrm_bcn_req.duration =  htod32(strtoul(argv[3], NULL, 0));
   }

   /* random interval */
   if ( argv[4] )
   {
      rrm_bcn_req.random_int =  htod32(strtoul(argv[4], NULL, 0));
   }

   /* channel */
   if ( argv[5] )
   {
     rrm_bcn_req.channel = htod32(strtoul(argv[5], NULL, 0));
   }

   /* SSID */
   if ( argv[6] )
   {
      len = strlen(argv[6]);
      if (len > SSID_NAME_SIZE)
      {
          WPRINT_APP_INFO ( ("SSID too long\n") );
          return ERR_UNKNOWN;
      }
      memcpy ( rrm_bcn_req.ssid.SSID, argv[6], len );
      rrm_bcn_req.ssid.SSID_len = len;
   }

    /* repetitions */
    if ( argv[7] )
    {
       rrm_bcn_req.repetitions = htod32(strtoul(argv[7], NULL, 0));
    }

    dump_bytes( (uint8_t *)&rrm_bcn_req, sizeof(radio_resource_management_beacon_req_t) );
    if( wwd_wifi_radio_resource_management_beacon_req( WWD_STA_INTERFACE, &rrm_bcn_req) != WWD_SUCCESS ) {
            WPRINT_APP_INFO((" wwd_wifi_rrm_bcn_req.. FAILED\n"));
    }

    return ERR_CMD_OK;
}

/* Params : [regulatory] [da] [duration] [random int] [channel] [repetitions] */
/* rrm_chload_req and rrm_noise_req parameters are same so consolidate to one rrm_req */
int rrm_req(int argc, char* argv[])
{
    radio_resource_management_req_t rrm_req;

    memset( &rrm_req, 0, sizeof(radio_resource_management_req_t) );

    if ( argv[1] )
    {
        /* Regulatory class */
        rrm_req.regulatory = htod32( strtoul(argv[1], NULL, 0) );
    }

    /* destination MAC Address */
    if ( argv[2] )
    {
        str_to_mac( (char *)argv[2], &rrm_req.da );
    }

    /* duration */
    if ( argv[3] )
    {
        rrm_req.duration = htod32( strtoul(argv[3], NULL, 0) );
    }

    /* random interval */
    if ( argv[4] )
    {
        rrm_req.random_int = htod32( strtoul(argv[4], NULL, 0) );
    }

    /* channel */
    if (argv[5])
    {
        rrm_req.channel = htod32( strtoul(argv[5], NULL, 0) );
    }

    /* repetitions */
    if (argv[6])
    {
        rrm_req.repetitions = htod32(strtoul(argv[6], NULL, 0));
    }

    dump_bytes( (uint8_t *)&rrm_req, sizeof(radio_resource_management_req_t) );
    if( strcmp(argv[0], IOVAR_STR_RRM_CHLOAD_REQ) == 0)
    {
        if( wwd_wifi_radio_resource_management_channel_load_req( WWD_STA_INTERFACE, &rrm_req ) != WWD_SUCCESS ) {
            WPRINT_APP_INFO( (" wwd_wifi_rrm_chload_req.. FAILED\n") );
        }
    }
    else if ( strcmp(argv[0], IOVAR_STR_RRM_NOISE_REQ) == 0 )
    {
        if ( wwd_wifi_radio_resource_management_noise_req( WWD_STA_INTERFACE, &rrm_req ) != WWD_SUCCESS )
        {
                WPRINT_APP_INFO ((" wwd_wifi_rrm_noise_req.. FAILED\n"));
        }
    }
    else
    {
        WPRINT_APP_INFO((" unknown command.. FAILED\n"));
        return ERR_UNKNOWN_CMD;
    }

    return ERR_CMD_OK;
}

/* Params: [regulatory] [da] [duration] [random int] [channel] [ta] [repetitions] */
int rrm_frame_req(int argc, char* argv[])
{
    radio_resource_management_framereq_t rrm_framereq;
    memset( &rrm_framereq, 0, sizeof(radio_resource_management_framereq_t) );

    if (argv[1])
    {
        /* Regulatory class */
        rrm_framereq.regulatory = htod32( strtoul(argv[1], NULL, 0) );
    }

    /* destination address */
    if (argv[2])
    {
        str_to_mac( (char *)argv[2], &rrm_framereq.da );
    }

    /* duration */
    if (argv[3])
    {
        rrm_framereq.duration = htod32( strtoul(argv[3], NULL, 0) );
    }

    /* random interval */
    if (argv[4])
    {
        rrm_framereq.random_int = htod32( strtoul(argv[4], NULL, 0) );
    }

    /* channel */
    if (argv[5])
    {
        rrm_framereq.channel = htod32( strtoul(argv[5], NULL, 0) );
    }

    /* transmit address */
    if (argv[6])
    {
        str_to_mac( argv[6], &rrm_framereq.ta );
    }

    /* repetitions */
    if (argv[7])
    {
        rrm_framereq.repetitions = htod32( strtoul(argv[7], NULL, 0) );
    }

    dump_bytes( (uint8_t *)&rrm_framereq, sizeof(radio_resource_management_framereq_t) );

    if( wwd_wifi_radio_resource_management_frame_req( WWD_STA_INTERFACE, &rrm_framereq ) != WWD_SUCCESS )
    {
        WPRINT_APP_INFO((" rrm_frame_req.. FAILED\n"));
    }
    return ERR_CMD_OK;
}

/* Params: [da] [random int] [duration] [peer] [group id] [repetitions] */
int rrm_stat_req(int argc, char* argv[])
{

    radio_resource_management_statreq_t rrm_statreq;

    memset( &rrm_statreq, 0, sizeof(radio_resource_management_statreq_t) );

    if ( argv[1] )
    {
        /* destination MAC Address */
        str_to_mac( argv[1], &rrm_statreq.da );
    }

    /* random interval */
    if ( argv[2] )
    {
        rrm_statreq.random_int = htod32( strtoul(argv[2], NULL, 0) );
    }

    /* duration */
    if ( argv[3] )
    {
        rrm_statreq.duration = htod32( strtoul(argv[3], NULL, 0) );
    }

    /* peer address */
    if ( argv[4] )
    {
        str_to_mac( argv[4], &rrm_statreq.peer );
    }

    /* group id */
    if (argv[5])
    {
        rrm_statreq.group_id = htod32( strtoul(argv[5], NULL, 0) );
    }

    /* repetitions */
    if ( argv[6] )
    {
        rrm_statreq.repetitions = htod32( strtoul(argv[6], NULL, 0) );
    }

    dump_bytes((uint8_t *)&rrm_statreq, sizeof(radio_resource_management_statreq_t));

    if( wwd_wifi_radio_resource_management_stat_req( WWD_STA_INTERFACE, &rrm_statreq ) != WWD_SUCCESS )
    {
        WPRINT_APP_INFO((" wwd_wifi_rrm_stat_req.. FAILED\n"));
    }

    return ERR_CMD_OK;
}

/* get 11k neighbor report list  supported only in AP mode */
int rrm_nbr_list(int argc, char* argv[])
{
    uint8_t buffer[WLC_IOCTL_SMLEN];
    uint16_t buflen = 4;
    uint8_t *ptr;
    int i;
    radio_resource_management_nbr_element_t *nbr_elt;

    memset( buffer, 0, WLC_IOCTL_SMLEN );

    if( wwd_wifi_radio_resource_management_neighbor_list( WWD_AP_INTERFACE, buffer, buflen) != WWD_SUCCESS )
    {
        WPRINT_APP_INFO((" wwd_wifi_rrm_nbr_list.. FAILED\n"));
    }
    buflen = *(uint16_t *)( buffer + strlen(IOVAR_STR_RRM_NBR_LIST) + 1 );
    if ( buflen == 0 )
    {
        WPRINT_APP_INFO(("RRM Neighbor Report List: Buffer EMPTY\n"));
        return ERR_UNKNOWN;
    }

    WPRINT_APP_INFO(("RRM Neighbor Report List: buflen:%d data:%02x %02x %02x %02x\n",
            buflen, buffer[0], buffer[1], buffer[2], buffer[3]));

    if( wwd_wifi_radio_resource_management_neighbor_list(WWD_AP_INTERFACE, buffer, buflen) != WWD_SUCCESS )
    {
        WPRINT_APP_INFO((" wwd_wifi_rrm_nbr_list list_cnt.. FAILED\n"));
        return ERR_UNKNOWN;
    }

    ptr = buffer;

    for (i = 0; i < buflen; i++)
    {
        nbr_elt = (radio_resource_management_nbr_element_t *)ptr;
        WPRINT_APP_INFO(("AP %2d: ", i + 1));
        WPRINT_APP_INFO(("bssid %02x:%02x:%02x:%02x:%02x:%02x ", nbr_elt->bssid.octet[0],
               nbr_elt->bssid.octet[1], nbr_elt->bssid.octet[2], nbr_elt->bssid.octet[3],
               nbr_elt->bssid.octet[4], nbr_elt->bssid.octet[5]));

        WPRINT_APP_INFO(("bssid_info %08x ",(uint) &nbr_elt->bssid_info));
        WPRINT_APP_INFO(("reg %2d channel %3d phytype %d\n", nbr_elt->regulatory,
               nbr_elt->channel, nbr_elt->phytype));

        ptr += TLV_HDR_LEN + DOT11_NEIGHBOR_REP_IE_FIXED_LEN;
    }

    return ERR_CMD_OK;
}


/* delete node from 11k neighbor report list supported only in AP mode */
/* Params: [bssid] */
int rrm_nbr_del_nbr(int argc, char* argv[])
{
    wiced_mac_t ea;

    if ( *++argv == NULL )
    {
         WPRINT_APP_INFO(("no BSSID specified\n"));
         return ERR_UNKNOWN;
    }

    /* bssid */
    if ( argv[1] )
    {
        str_to_mac( argv[1], &ea );
    }

    if ( wwd_wifi_radio_resource_management_neighbor_del_neighbor(WWD_AP_INTERFACE, &ea) != WWD_SUCCESS )
    {
        dump_bytes((uint8_t *)&ea, sizeof(wiced_mac_t));
        WPRINT_APP_INFO((" wwd_wifi_rrm_nbr_del_nbr.. FAILED\n"));
    }

    return ERR_CMD_OK;
}

/* add node to 11k neighbor report list  supported only in AP mode */
/* Params: [bssid] [bssid info] [regulatory] [channel] [phytype] */
int rrm_nbr_add_nbr(int argc, char* argv[])
{
    uint16_t buflen = TLV_HDR_LEN + DOT11_NEIGHBOR_REP_IE_FIXED_LEN;
    radio_resource_management_nbr_element_t nbr_elt;

    memset( &nbr_elt, 0, sizeof(radio_resource_management_nbr_element_t) );

    for (argc = 0; argv[argc]; argc++);

    if ( argc != 6 )
    {
        WPRINT_APP_INFO((" Too many arguments.. FAILED\n"));
        return ERR_UNKNOWN;
    }

    /* BSSID */
    if( argv[1] )
    {
         str_to_mac(argv[1], &nbr_elt.bssid);
    }

    /* BSSID info */
    nbr_elt.bssid_info = htod32( strtoul(argv[2], NULL, 0) );

    /* Regulatory class */
    nbr_elt.regulatory = htod32( strtoul(argv[3], NULL, 0) );

    /* channel */
    nbr_elt.channel = htod32( strtoul(argv[4], NULL, 0) );

    /* PHY Type */
    nbr_elt.phytype = htod32( strtoul(argv[5], NULL, 0) );

    nbr_elt.id = DOT11_MNG_NEIGHBOR_REP_ID;
    nbr_elt.length = DOT11_NEIGHBOR_REP_IE_FIXED_LEN;

    if( wwd_wifi_radio_resource_management_neighbor_add_neighbor( WWD_AP_INTERFACE, &nbr_elt, buflen ) != WWD_SUCCESS )
    {
        WPRINT_APP_INFO((" wwd_wifi_rrm_nbr_add_nbr.. FAILED\n"));
        return ERR_UNKNOWN;
    }
    return ERR_CMD_OK;
}

/* Send Link Management Request */
int rrm_lm_req(int argc, char* argv[])
{
   wiced_mac_t da;

   memset( &da, 0, sizeof(da) );
   /* BSSID of destination */
   str_to_mac((char *)argv[1], &da );

   if( wwd_wifi_radio_resource_management_link_management_req(WWD_STA_INTERFACE, &da) != WWD_SUCCESS )
   {
       WPRINT_APP_INFO((" wwd_wifi_rrm_lm_req.. FAILED\n"));
   }
   dump_bytes( (uint8_t *)&da, sizeof(wiced_mac_t) );

   return ERR_CMD_OK;
}
/* Send RRM Neighbor Request */
int rrm_nbr_req(int argc, char* argv[])
{
   wiced_ssid_t ssid;
   memset( &ssid, 0, sizeof(wiced_ssid_t) );
   ssid.length = (uint8_t) (strlen((const char *)&argv[2]) + 1 );

   memcpy( ssid.value, &argv[2], ssid.length );

   if ( wwd_wifi_radio_resource_management_neighbor_req(WWD_STA_INTERFACE, &ssid) != WWD_SUCCESS )
   {
       WPRINT_APP_INFO((" wwd_wifi_rrm_nbr_req.. FAILED\n"));
       return ERR_UNKNOWN;
   }
    return ERR_CMD_OK;
}

/* get fast bss transition over distribution system
 * 1 : FBT(Fast BSS Transition) Over-the-DS(Distribution System) is allowed
 * 0 : FBT (Fast BSS Transition) Over-the-DS not allowed
 */
int fbt_over_ds(int argc, char* argv[])
{
    int value = 0;
    wiced_bool_t set = WICED_TRUE;

    if( argv[1] )
    {
        value = htod32(strtoul( argv[1], NULL, 0) );
    }
    else
    {
        set = WICED_FALSE;
    }

    if( wwd_wifi_fast_bss_transition_over_distribution_system(WWD_STA_INTERFACE, set, &value) == WWD_SUCCESS ) {
               if ( set )
               {
                   WPRINT_APP_INFO((" set/reset of Fast BSS Transition over DS success \n"));
               }
               else
               {
                   WPRINT_APP_INFO((" get Fast BSS Transition over DS success WLFTBT:%d \n", value));
               }
    }
    else
    {
               WPRINT_APP_INFO((" wwd_wifi_fbt_over_ds.. FAILED\n"));
    }
    return ERR_CMD_OK;
}

/* get Fast BSS Transition capabilities */
int fbt_caps(int argc, char* argv[])
{
    wiced_bool_t value = WICED_FALSE;

    if( wwd_wifi_fast_bss_transition_capabilities(WWD_STA_INTERFACE, &value) == WWD_SUCCESS )
    {
         WPRINT_APP_INFO((" WLFBT Capabilities:%d\n", value));
    }
    else
    {
         WPRINT_APP_INFO((" wwd_wifi_get_fbt_enable.. Failed:\n"));
    }

    return ERR_CMD_OK;
}

/* Get/Set Protected Management Capability */
int mfp_capabilities (int argc, char* argv[] )
{
    uint32_t value = 0;

    if(argv[1] != NULL )
    {
        if( strcasecmp (argv[1], "set" ) == 0 )
        {
           value = htod32(strtoul( argv[2], NULL, 0) );

           if ( wwd_wifi_set_iovar_value ( IOVAR_STR_MFP, value, WWD_STA_INTERFACE ) != WWD_SUCCESS )
           {
               WPRINT_APP_INFO((" Set Management Frame Protection..Failed:\n") );
           }
        }
        else if( strcasecmp ( argv[1], "get" ) == 0 )
        {
            if ( wwd_wifi_get_iovar_value ( IOVAR_STR_MFP, &value, WWD_STA_INTERFACE ) != WWD_SUCCESS )
            {
               WPRINT_APP_INFO((" Get Management Frame Protection..Failed:\n") );
            }
            else
            {
                  switch(value)
                  {
                      case MFP_NONE:
                            WPRINT_APP_INFO((" Management Frame Protection 0- MFP NONE \n"));
                            break;

                      case MFP_CAPABLE:
                            WPRINT_APP_INFO((" Management Frame Protection 1- MFP CAPABLE \n"));
                            break;

                      case MFP_REQUIRED:
                            WPRINT_APP_INFO((" Management Frame Protection 2- MFP REQUIRED \n"));
                            break;
                      default:
                            WPRINT_APP_INFO((" Management Frame Protection UNKNOWN!!\n"));
                  }
             }
        }
        else
        {
            return ERR_UNKNOWN;
        }
    }

    return ERR_CMD_OK;
}


