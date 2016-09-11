/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "besl_structures.h"
#include "crypto_structures.h"
#include <time.h>
#include "dtls_cipher_suites.h"
#include "wwd_constants.h"
#include "tls_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define DTLS_VERSION 0xfefd /* DTLS v1.2 */

/** Length of DTLS master_secret */
#define DTLS_MASTER_SECRET_LENGTH 48
#define DTLS_RANDOM_LENGTH 32
#define DTLS_PSK_MAX_CLIENT_IDENTITY_LEN   32
#define DTLS_COOKIE_LENGTH_MAX 32

#define  DTLS_WAIT_FOREVER                (0xFFFFFFFF)
#define  DTLS_HANDSHAKE_PACKET_TIMEOUT_MS (40000)

/* TLS_PSK_WITH_AES_128_CCM_8 */
#define DTLS_MAC_KEY_LENGTH    0
#define DTLS_KEY_LENGTH        16 /* AES-128 */
#define DTLS_BLK_LENGTH        16 /* AES-128 */
#define DTLS_MAC_LENGTH        DTLS_HMAC_DIGEST_SIZE
#define DTLS_IV_LENGTH         4  /* length of nonce_explicit */

#define MAX_KEYBLOCK_LENGTH  \
  (2 * DTLS_MAC_KEY_LENGTH + 2 * DTLS_KEY_LENGTH + 2 * DTLS_IV_LENGTH)

/******************************************************
 *                   Enumerations
 ******************************************************/
typedef uint32_t dtls_packet_t;

/** Known compression suites.*/
typedef enum
{
    DTLS_COMPRESSION_NULL = 0x0000
/* NULL compression */
} dtls_compression_t;

typedef enum
{
    DTLS_NO_VERIFICATION       = 0,
    DTLS_VERIFICATION_OPTIONAL = 1,
    DTLS_VERIFICATION_REQUIRED = 2,
} wiced_dtls_certificate_verification_t;

typedef enum
{
   DTLS_NULL_WITH_NULL_NULL = 0x0000
} dtls_cipher_t;

typedef enum
{
    DTLS_RECEIVE_PACKET_IF_NEEDED,
    DTLS_AVOID_NEW_RECORD_PACKET_RECEIVE,
} dtls_packet_receive_option_t;

typedef enum
{
    DTLS_STATE_HELLO_REQUEST                = 0,
    DTLS_STATE_CLIENT_HELLO                 = 1,
    DTLS_STATE_SERVER_HELLOVERIFY           = 2,
    DTLS_STATE_CLIENT_HELLO_COOKIE          = 3,
    DTLS_STATE_SERVER_HELLO                 = 4,
    DTLS_STATE_SERVER_CERTIFICATE           = 5,
    DTLS_STATE_SERVER_KEY_EXCHANGE          = 6,
    DTLS_STATE_SERVER_CERTIFICATE_REQUEST   = 7,
    DTLS_STATE_SERVER_HELLO_DONE            = 8,
    DTLS_STATE_CLIENT_CERTIFICATE           = 9,
    DTLS_STATE_CLIENT_KEY_EXCHANGE          = 10,
    DTLS_CERTIFICATE_VERIFY                 = 11,
    DTLS_CLIENT_CHANGE_CIPHER_SPEC          = 12,
    DTLS_CLIENT_FINISHED                    = 13,
    DTLS_SERVER_CHANGE_CIPHER_SPEC          = 14,
    DTLS_SERVER_FINISHED                    = 15,
    DTLS_FLUSH_BUFFERS                      = 16,
    DTLS_HANDSHAKE_OVER                     = 17
} dtls_states_t;

typedef enum
{
    WICED_DTLS_AS_CLIENT = 0,
    WICED_DTLS_AS_SERVER = 1
} wiced_dtls_endpoint_type_t;

typedef enum
{
    DTLS_CLIENT = 0,
    DTLS_SERVER
} dtls_peer_type;

typedef enum
{
    DTLS_TCP_TRANSPORT = 0,
    DTLS_EAP_TRANSPORT = 1,
    DTLS_UDP_TRANSPORT = 2,
} dtls_transport_protocol_t;

typedef struct
{
        uint32_t ver;
        union
        {
            uint32_t v4;
            uint32_t v6[4];
        } ip;
} wiced_ip;

typedef struct
{
        wiced_ip ip;
        uint16_t port;
} dtls_session_t;

#pragma  pack(1)

/** Generic header structure of the DTLS record layer. */
typedef struct
{
        uint8_t     content_type;      /**< content type of the included message */
        uint16_t    version;           /**< Protocol version */
        uint16_t    epoch;             /**< counter for cipher state changes */
        uint8_t     sequence_number[6];   /**< sequence number */
        uint8_t    length[2];            /**< length of the following fragment */
        /* fragment */
} dtls_record_header_t;

/** Header structure for the DTLS handshake protocol. */
typedef struct
{
        uint8_t     msg_type;               /**< Type of handshake message  (one of DTLS_HT_) */
        uint8_t     length[ 3 ];            /**< length of this message */
        uint16_t    message_seq;            /**< Message sequence number */
        uint8_t     fragment_offset[ 3 ];   /**< Fragment offset. */
        uint8_t     fragment_length[ 3 ];   /**< Fragment length. */
        /* body */
} dtls_handshake_header_t;

/** Structure of the Hello Verify Request. */
typedef struct
{
        uint16_t    version;                /**< Server version */
        uint8_t     cookie_length;          /**< Length of the included cookie */
        uint8_t     cookie[ ];              /**< up to 32 bytes making up the cookie */
} dtls_hello_verify_t;

typedef struct
{
        dtls_handshake_header_t handshake_header;
        uint8_t                 data[ 1 ];
} dtls_handshake_message_t;

/** Header structure for the DTLS handshake protocol. */
typedef struct
{
        dtls_record_header_t record_header;
        uint8_t              data[ 1 ];
} dtls_record_t;

#pragma  pack()

typedef struct
{
        const uint8_t*   certificate_data;
        uint32_t         certificate_data_length;
        wiced_bool_t     certificate_data_malloced;
        wiced_tls_key_t* public_key;
        const char*      common_name;
        x509_cert*       processed_certificate_data;
} wiced_dtls_certificate_t;

typedef struct
{
        union
        {
            wiced_tls_key_t      common;
            wiced_dtls_psk_key_t psk;
        } private_key;
        wiced_dtls_certificate_t certificate;
} wiced_dtls_identity_t;


typedef struct
{
        uint16_t        id_length;
        unsigned char   identity[ DTLS_PSK_MAX_CLIENT_IDENTITY_LEN ];
} dtls_handshake_parameters_psk_t;

typedef struct
{
        dtls_compression_t      compression;   /* compression method */
        const cipher_suite_t*   cipher;        /* cipher suite selected by server. */
        uint16_t                epoch;         /* counter for cipher state changes */
        uint64_t                rseq;          /* sequence number of last record sent */
} dtls_security_parameters_t;

typedef struct
{
        uint16_t     mseq_s;
        uint16_t     mseq_r;
        sha2_context hs_hash;
} dtls_hs_state_t;

typedef struct
{
        union
        {
                struct random_t
                {
                        uint8_t         client[ DTLS_RANDOM_LENGTH ]; /**< client random gmt and bytes */
                        uint8_t         server[ DTLS_RANDOM_LENGTH ]; /**< server random gmt and bytes */
                } random;
                uint8_t                 master_secret[ DTLS_MASTER_SECRET_LENGTH ];
        } tmp;

        dtls_hs_state_t                 hs_state;
        unsigned int                    do_client_auth :1;
        dtls_handshake_parameters_psk_t psk;
} dtls_handshake_parameters_t;

/**
 * Holds security parameters, local state and the transport address
 * for each peer. */
typedef struct dtls_peer_t
{
        unsigned char                cookie_secret[ 32 ];
        uint16_t                     cookie_length;
        dtls_session_t               session;              /**< peer address and local interface */
        dtls_peer_type               role;                 /**< denotes if this host is DTLS_CLIENT or DTLS_SERVER */
        dtls_states_t                state;                /**< DTLS engine state */
        dtls_security_parameters_t  *security_params[ 2 ];
        dtls_handshake_parameters_t *handshake_params;
} dtls_peer_t;

typedef struct context_t
{
        int32_t                   state;                /* current state of Peer. */
        dtls_peer_t               peer;                 /* peer hash map */
        void*                     send_context;
        dtls_packet_t*            outgoing_packet;

        wiced_dtls_identity_t*    identity;
        const cipher_suite_t**    ciphers;              /* allowed ciphersuites for DTLS */
        dtls_transport_protocol_t transport_protocol;

        unsigned char             randbytes[64];        /* random bytes     */
        unsigned char             premaster[256];       /* premaster secret */
        uint32_t                  pmslen;               /* premaster length */

        uint32_t*                 received_packet;
        uint16_t                  received_packet_length;
        uint16_t                  received_packet_bytes_skipped;
        uint16_t                  current_record_original_length;
        dtls_record_t*            current_record;
        uint16_t                  current_record_bytes_processed;
        dtls_handshake_message_t* current_handshake_message;

        uint8_t                   key_block[ MAX_KEYBLOCK_LENGTH ];
        int32_t                   do_crypt;             /* en(de)cryption flag     */
        tls_cipher_context        ctx_enc;              /* encryption context   */

} dtls_context_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct context_t wiced_dtls_workspace_t;

typedef struct wiced_dtls_context_s
{
        wiced_dtls_workspace_t context;
        wiced_dtls_identity_t* identity;
} wiced_dtls_context_t;

typedef enum
{
    DTLS_RESULT_LIST ( DTLS_ )
    /* 5000 - 5999 */
}    dtls_result_t;

    /******************************************************
     *                    Structures
     ******************************************************/

    /******************************************************
     *                 Global Variables
     ******************************************************/

    /******************************************************
     *               Function Declarations
     ******************************************************/

    dtls_result_t dtls_handshake_client_async( dtls_context_t* dtls, dtls_session_t* session );
    dtls_result_t dtls_close_notify( dtls_context_t* context );
    void          dtls_free ( dtls_context_t* context, dtls_session_t* session );



#ifdef __cplusplus
} /*extern "C" */
#endif
