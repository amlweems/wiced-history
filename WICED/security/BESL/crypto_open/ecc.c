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
 *
 */

#include "ecc.h"
#include "sha2.h"
#include "uECC.h"
#include <string.h>

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    ECC_EXPLICIT_PRIME = 1,
    ECC_EXPLICIT_CHAR  = 2,
    ECC_NAMED_CURVE    = 3
} ecc_curve_type_t;

typedef enum
{
       ECC_CURVE_sect163k1 = 1,
       ECC_CURVE_sect163r1 = 2,
       ECC_CURVE_sect163r2 = 3,
       ECC_CURVE_sect193r1 = 4,
       ECC_CURVE_sect193r2 = 5,
       ECC_CURVE_sect233k1 = 6,
       ECC_CURVE_sect233r1 = 7,
       ECC_CURVE_sect239k1 = 8,
       ECC_CURVE_sect283k1 = 9,
       ECC_CURVE_sect283r1 = 10,
       ECC_CURVE_sect409k1 = 11,
       ECC_CURVE_sect409r1 = 12,
       ECC_CURVE_sect571k1 = 13,
       ECC_CURVE_sect571r1 = 14,
       ECC_CURVE_secp160k1 = 15,
       ECC_CURVE_secp160r1 = 16,
       ECC_CURVE_secp160r2 = 17,
       ECC_CURVE_secp192k1 = 18,
       ECC_CURVE_secp192r1 = 19,
       ECC_CURVE_secp224k1 = 20,
       ECC_CURVE_secp224r1 = 21,
       ECC_CURVE_secp256k1 = 22,
       ECC_CURVE_secp256r1 = 23,
       ECC_CURVE_secp384r1 = 24,
       ECC_CURVE_secp521r1 = 25,
       ECC_CURVE_arbitrary_explicit_prime_curves = 0xFF01,
       ECC_CURVE_arbitrary_explicit_char2_curves = 0xFF02,
} ecc_named_curve_type_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/*
 *  struct {
            ECParameters    curve_params;
            ECPoint         public;
        } ServerECDHParams;
 */
/*
 *
        struct {
            ECCurveType    curve_type;
            select (curve_type) {
                case explicit_prime:
                    opaque      prime_p <1..2^8-1>;
                    ECCurve     curve;
                    ECPoint     base;
                    opaque      order <1..2^8-1>;
                    opaque      cofactor <1..2^8-1>;
                case explicit_char2:
                    uint16      m;
                    ECBasisType basis;
                    select (basis) {
                        case ec_trinomial:
                            opaque  k <1..2^8-1>;
                        case ec_pentanomial:
                            opaque  k1 <1..2^8-1>;
                            opaque  k2 <1..2^8-1>;
                            opaque  k3 <1..2^8-1>;
                    };
                    ECCurve     curve;
                    ECPoint     base;
                    opaque      order <1..2^8-1>;
                    opaque      cofactor <1..2^8-1>;

                case named_curve:
                    NamedCurve namedcurve;
            };
        } ECParameters;
 */
int ecc_parse_server_key_exchange( ssl_context *ssl, const uint8_t* data, uint32_t data_length, tls_digitally_signed_signature_algorithm_t input_signature_algorithm )
{
    const unsigned char *p, *end;
    unsigned char hash[ 64 ];
    uint8_t curve_type;
    uint16_t named_curve;
    uint8_t public_key[64];
    uint8_t signature_hash;
    uint8_t signature_algorithm;
    uint16_t temp;
    const uint8_t* end_of_server_ecdh_params;

    p = data;
    end = data + data_length;

    curve_type = *p++;
    switch ( curve_type )
    {
        case ECC_NAMED_CURVE:
            named_curve = (( *p++ ) << 8);
            named_curve += *p++;
            break;

        /* Not supported */
        default:
            return TLS_ERROR_BAD_HS_CLIENT_KEY_EXCHANGE;
            break;
    }

    temp = *p++;
    if ( temp > 65 || (*p++) != 0x04 )
    {
        return TLS_ERROR_BAD_HS_CLIENT_KEY_EXCHANGE;
    }
    temp -= 1;

    memcpy( public_key, p, temp );
    p += temp;

    end_of_server_ecdh_params = p;

    signature_hash      = *p++;
    signature_algorithm = *p++;

    temp = ( *p++ ) << 8;
    temp += *p++;

    /*
     *  ServerKeyExchange.signed_params.sha_hash
     *       SHA(ClientHello.random + ServerHello.random + ServerKeyExchange.params);
     */
    switch ( signature_hash )
    {
        case 2: /* SHA1 */
        {
            sha1_context sha1_ctx;
            sha1_starts( &sha1_ctx );
            sha1_update( &sha1_ctx, ssl->randbytes, 64 );
            sha1_update( &sha1_ctx, data, end_of_server_ecdh_params - data );
            sha1_finish( &sha1_ctx, hash );
            break;
        }

        case 4: /* SHA256 */
        {
            sha2_context sha2_ctx;
            sha2_starts( &sha2_ctx, 0 );
            sha2_update( &sha2_ctx, ssl->randbytes, 64 );
            sha2_update( &sha2_ctx, data, end_of_server_ecdh_params - data );
            sha2_finish( &sha2_ctx, hash );
            break;
        }
    }

    switch ( signature_algorithm )
    {
        case TLS_DIGITALLY_SIGNED_SIGNATURE_ALGORITHM_ECDSA:
            if ( uECC_verify( ssl->peer_public_key->data, hash, p ) != 0 )
            {
                return TLS_ERROR_BAD_HS_CLIENT_KEY_EXCHANGE;
            }
    }

    p += temp;
    if ( p != end )
    {
        return TLS_ERROR_BAD_HS_CLIENT_KEY_EXCHANGE;
    }

    /* Replace the peer public key taken from the server certificate with the ephemeral one
     * Note: The certificate is only required to verify the ephemeral key
     */
    memcpy( ssl->peer_public_key->data, public_key, sizeof( public_key ) );

    return TLS_SUCCESS;
}

int ecc_tls_create_premaster_secret( void*      key_context,
                                     uint8_t    is_ssl_3,
                                     uint16_t   max_version,
                                     uint8_t*   premaster_secret_out,
                                     uint32_t*  pms_length_out,
                                     int32_t    (*f_rng)(void *),
                                     void*      p_rng,
                                     uint8_t*   encrypted_output,
                                     uint32_t*  encrypted_length_out )
{
    uint8_t private_key[32];
    wiced_tls_ecc_key_t* peer_public_key = (wiced_tls_ecc_key_t*)key_context;

    *encrypted_output++ = 65;
    *encrypted_output++ = 04;
    uECC_make_key( encrypted_output, private_key );
    *encrypted_length_out = 66;

    /* Generate the premaster secret */
    uECC_shared_secret( peer_public_key->key, private_key, premaster_secret_out );
    *pms_length_out = 32;

    /* Erase the private key */
    memset( private_key, 0, sizeof( private_key ) );

    return 0;
}
