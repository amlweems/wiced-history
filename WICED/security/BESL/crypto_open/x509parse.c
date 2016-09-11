/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/* Originally taken from TropicSSL
 * https://gitorious.org/tropicssl/
 * commit: 92bb3462dfbdb4568c92be19e8904129a17b1eed
 * Whitespace converted (Tab to 4 spaces, LF to CRLF)
 * int arguments/returns changed to int32_t
 * change malloc/free to tls_host_malloc/tls_host_free
 * remove sscanf usage
 * add option for generalized time
 * add const to arguments where appropriate
 * fix base64_decode usage
 * add better hash_algorithm parsing
 * add typecasting where required
 * remove x509parse_crtfile, x509parse_crtfile and x509parse_keyfile
 * segregate functions which use sprintf
 * add support for RSA_SHA256, RSA_SHA384, RSA_SHA512 hashes
 * replace time() with tls_host_get_time_ms()
 * fix certificate chain verification in x509parse_verify()
 * add function x509parse_pubkey() to parse a public rsa key
 * add support for generalized time
 */

/*
 *    X.509 certificate and private key decoding
 *
 *    Based on XySSL: Copyright (C) 2006-2008     Christophe Devine
 *
 *    Copyright (C) 2009    Paul Bakker <polarssl_maintainer at polarssl dot org>
 *
 *    All rights reserved.
 *
 *    Redistribution and use in source and binary forms, with or without
 *    modification, are permitted provided that the following conditions
 *    are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *      * Neither the names of PolarSSL or XySSL nor the names of its contributors
 *        may be used to endorse or promote products derived from this software
 *        without specific prior written permission.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *    TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *    The ITU-T X.509 standard defines a certificat format for PKI.
 *
 *    http://www.ietf.org/rfc/rfc2459.txt
 *    http://www.ietf.org/rfc/rfc3279.txt
 *
 *    ftp://ftp.rsasecurity.com/pub/pkcs/ascii/pkcs-1v2.asc
 *
 *    http://www.itu.int/ITU-T/studygroups/com17/languages/X.680-0207.pdf
 *    http://www.itu.int/ITU-T/studygroups/com17/languages/X.690-0207.pdf
 */

#include "tls_host_api.h"
#include "tls_cipher_suites.h"
#include "x509.h"
#include "base64.h"
#include "des.h"
#if defined(TROPICSSL_MD2_C)
#include "md2.h"
#endif /* if defined(TROPICSSL_MD2_C) */
#include "md5.h"
#include "sha1.h"
#include "sha2.h"
#include "sha4.h"
#include "bignum.h"
#include "uECC.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

wiced_tls_key_type_t type;

static int32_t x509_get_rsa_public_key( const unsigned char **p, const unsigned char *end, mpi * N, mpi * E );

uint32_t x509_read_cert_length( const uint8_t* der_certificate_data )
{
    uint32_t length;
    uint8_t bytes_used;

    /* Use ASN.1 read length, but we need to skip the first "type" byte */
    if ( asn1_read_length( der_certificate_data+1, &length, &bytes_used ) != 0 )
    {
        return 0;
    }
    return length + bytes_used + 1;
}

/*
 * ASN.1 DER decoding routines
 */
int32_t asn1_read_length( const uint8_t* p, uint32_t* length, uint8_t* bytes_used )
{
    if ( ( *p & 0x80 ) == 0 )
    {
        *length = *p;
        *bytes_used = 1;
    }
    else
    {
        switch ( *p & 0x7F )
        {
            case 1:
                *length = p[ 1 ];
                *bytes_used = 2;
                break;

            case 2:
                *length = ( p[ 1 ] << 8 ) | p[ 2 ];
                *bytes_used = 3;
                break;

            default:
                return ( TROPICSSL_ERR_ASN1_INVALID_LENGTH );
                break;
        }
    }
    return 0;
}

static int32_t asn1_get_len(const unsigned char **p, const unsigned char *end, uint32_t *len)
{
    uint8_t bytes_used;
    int32_t result;

    if ( ( end - *p ) < 1 )
    {
        return ( TROPICSSL_ERR_ASN1_OUT_OF_DATA );
    }

    result = asn1_read_length( *p, len, &bytes_used );
    if ( result != 0 )
    {
        return result;
    }

    if ( bytes_used > ( end - *p ) )
    {
        return ( TROPICSSL_ERR_ASN1_OUT_OF_DATA );
    }
    *p += bytes_used;

    if ( *len > (int) ( end - *p ) )
    {
        return ( TROPICSSL_ERR_ASN1_OUT_OF_DATA );
    }

    return (0);
}

static int32_t asn1_get_tag(const unsigned char **p,
            const unsigned char *end, uint32_t *len, int32_t tag)
{
    if ((end - *p) < 1)
        return (TROPICSSL_ERR_ASN1_OUT_OF_DATA);

    if (**p != tag)
        return (TROPICSSL_ERR_ASN1_UNEXPECTED_TAG);

    (*p)++;

    return (asn1_get_len(p, end, len));
}

static int32_t asn1_get_bool(const unsigned char **p, const unsigned char *end, int32_t *val)
{
    int32_t ret;
    uint32_t len;

    if ((ret = asn1_get_tag(p, end, &len, ASN1_BOOLEAN)) != 0)
        return (ret);

    if (len != 1)
        return (TROPICSSL_ERR_ASN1_INVALID_LENGTH);

    *val = (**p != 0) ? 1 : 0;
    (*p)++;

    return (0);
}

static int32_t asn1_get_int(const unsigned char **p, const unsigned char *end, int32_t *val)
{
    int32_t ret;
    uint32_t len;

    if ((ret = asn1_get_tag(p, end, &len, ASN1_INTEGER)) != 0)
        return (ret);

    if (len > (int)sizeof(int32_t) || (**p & 0x80) != 0)
        return (TROPICSSL_ERR_ASN1_INVALID_LENGTH);

    *val = 0;

    while (len-- > 0) {
        *val = (*val << 8) | **p;
        (*p)++;
    }

    return (0);
}

static int32_t asn1_get_mpi(const unsigned char **p, const unsigned char *end, mpi *X)
{
    int32_t ret;
    uint32_t len;

    if ((ret = asn1_get_tag(p, end, &len, ASN1_INTEGER)) != 0)
        return (ret);

    ret = mpi_read_binary(X, *p, len);

    *p += len;

    return (ret);
}

/*
 *    Version     ::=  INTEGER  {  v1(0), v2(1), v3(2)  }
 */
static int32_t x509_get_version(const unsigned char **p, const unsigned char *end, int32_t* ver)
{
    int32_t ret;
    uint32_t len;

    if ((ret = asn1_get_tag(p, end, &len,
                ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 0))
        != 0) {
        if (ret == TROPICSSL_ERR_ASN1_UNEXPECTED_TAG)
            return (*ver = 0);

        return (ret);
    }

    end = *p + len;

    if ((ret = asn1_get_int(p, end, ver)) != 0)
        return (TROPICSSL_ERR_X509_CERT_INVALID_VERSION | ret);

    if (*p != end)
        return (TROPICSSL_ERR_X509_CERT_INVALID_VERSION |
            TROPICSSL_ERR_ASN1_LENGTH_MISMATCH);

    return (0);
}

/*
 *    CertificateSerialNumber     ::=  INTEGER
 */
static int32_t x509_get_serial(const unsigned char **p,
               const unsigned char *end, x509_buf * serial)
{
    int32_t ret;

    if ((end - *p) < 1)
        return (TROPICSSL_ERR_X509_CERT_INVALID_SERIAL |
            TROPICSSL_ERR_ASN1_OUT_OF_DATA);

    if (**p != (ASN1_CONTEXT_SPECIFIC | ASN1_PRIMITIVE | 2) &&
        **p != ASN1_INTEGER)
        return (TROPICSSL_ERR_X509_CERT_INVALID_SERIAL |
            TROPICSSL_ERR_ASN1_UNEXPECTED_TAG);

    serial->tag = *(*p)++;

    if ((ret = asn1_get_len(p, end, &serial->len)) != 0)
        return (TROPICSSL_ERR_X509_CERT_INVALID_SERIAL | ret);

    serial->p = *p;
    *p += serial->len;

    return (0);
}

/*
 *    AlgorithmIdentifier     ::=  SEQUENCE    {
 *         algorithm                 OBJECT IDENTIFIER,
 *         parameters                 ANY DEFINED BY algorithm OPTIONAL    }
 */
static int32_t x509_get_alg(const unsigned char **p, const unsigned char *end, x509_buf *alg)
{
    int32_t ret;
    uint32_t len;

    if ((ret = asn1_get_tag(p, end, &len,
                ASN1_CONSTRUCTED | ASN1_SEQUENCE)) != 0)
        return (TROPICSSL_ERR_X509_CERT_INVALID_ALG | ret);

    end = *p + len;
    alg->tag = **p;

    if ((ret = asn1_get_tag(p, end, &alg->len, ASN1_OID)) != 0)
        return (TROPICSSL_ERR_X509_CERT_INVALID_ALG | ret);

    alg->p = *p;
    *p += alg->len;

    /* We won't deal with additional parameters here but we should verify the tags are valid */
    while ( *p < end )
    {
        /* Skip the type */
        *p += 1;
        ret = asn1_get_len(p, end, &len);
        if ( ret != 0 )
            return ( TROPICSSL_ERR_X509_CERT_INVALID_ALG | ret );

        *p += len;
    }

    if ( *p != end )
        return ( TROPICSSL_ERR_X509_CERT_INVALID_ALG | TROPICSSL_ERR_ASN1_LENGTH_MISMATCH );

    return (0);
}

/*
 *    RelativeDistinguishedName ::=
 *      SET OF AttributeTypeAndValue
 *
 *    AttributeTypeAndValue ::= SEQUENCE {
 *      type       AttributeType,
 *      value       AttributeValue }
 *
 *    AttributeType ::= OBJECT IDENTIFIER
 *
 *    AttributeValue ::= ANY DEFINED BY AttributeType
 */
static int32_t x509_get_name(const unsigned char **p, const unsigned char *end, x509_name *cur)
{
    int32_t ret;
    uint32_t len;
    const unsigned char *end2;
    x509_buf *oid;
    x509_buf *val;

    if ((ret = asn1_get_tag(p, end, &len,
                ASN1_CONSTRUCTED | ASN1_SET)) != 0)
        return (TROPICSSL_ERR_X509_CERT_INVALID_NAME | ret);

    end2 = end;
    end = *p + len;

    if ((ret = asn1_get_tag(p, end, &len,
                ASN1_CONSTRUCTED | ASN1_SEQUENCE)) != 0)
        return (TROPICSSL_ERR_X509_CERT_INVALID_NAME | ret);

    if (*p + len != end)
        return (TROPICSSL_ERR_X509_CERT_INVALID_NAME |
            TROPICSSL_ERR_ASN1_LENGTH_MISMATCH);

    oid = &cur->oid;
    oid->tag = **p;

    if ((ret = asn1_get_tag(p, end, &oid->len, ASN1_OID)) != 0)
        return (TROPICSSL_ERR_X509_CERT_INVALID_NAME | ret);

    oid->p = *p;
    *p += oid->len;

    if ((end - *p) < 1)
        return (TROPICSSL_ERR_X509_CERT_INVALID_NAME |
            TROPICSSL_ERR_ASN1_OUT_OF_DATA);

    if (**p != ASN1_BMP_STRING && **p != ASN1_UTF8_STRING &&
        **p != ASN1_T61_STRING && **p != ASN1_PRINTABLE_STRING &&
        **p != ASN1_IA5_STRING && **p != ASN1_UNIVERSAL_STRING &&
        **p != ASN1_BIT_STRING )
        return (TROPICSSL_ERR_X509_CERT_INVALID_NAME |
            TROPICSSL_ERR_ASN1_UNEXPECTED_TAG);

    val = &cur->val;
    val->tag = *(*p)++;

    if ((ret = asn1_get_len(p, end, &val->len)) != 0)
        return (TROPICSSL_ERR_X509_CERT_INVALID_NAME | ret);

    val->p = *p;
    *p += val->len;

    cur->next = NULL;

    if (*p != end)
        return (TROPICSSL_ERR_X509_CERT_INVALID_NAME |
            TROPICSSL_ERR_ASN1_LENGTH_MISMATCH);

    /*
     * recurse until end of SEQUENCE is reached
     */
    if (*p == end2)
        return (0);

    cur->next = (x509_name *) tls_host_malloc ("x509", sizeof( x509_name ) );

    if (cur->next == NULL)
        return (1);

    return (x509_get_name(p, end2, cur->next));
}

static inline char* parse_two_digit_decimal_string_uint16( char* string, uint16_t *output )
{
    *output = (string[0] -'0')*10 + (string[1] -'0');
    return string + 2;
}

static inline char* parse_two_digit_decimal_string_uint8( char* string, uint8_t *output )
{
    *output = (string[0] -'0')*10 + (string[1] -'0');
    return string + 2;
}

static int32_t parse_x509_DateTime( char* string, x509_time *datetime, uint8_t short_year )
{
    char* curr = string;

    curr = parse_two_digit_decimal_string_uint16( curr, &datetime->year );

    if ( short_year == 0 )
    {
        /* Long year representation */
        uint16_t tmp_year = datetime->year;
        curr = parse_two_digit_decimal_string_uint16( curr, &datetime->year );
        datetime->year += tmp_year * 100;
    }
    curr = parse_two_digit_decimal_string_uint8(  curr, &datetime->mon );
    curr = parse_two_digit_decimal_string_uint8(  curr, &datetime->day );
    curr = parse_two_digit_decimal_string_uint8(  curr, &datetime->hour );
    curr = parse_two_digit_decimal_string_uint8(  curr, &datetime->min );
           parse_two_digit_decimal_string_uint8(  curr, &datetime->sec );

    if ( ( ( datetime->year >= 100 ) && ( short_year != 0 ) ) ||
         ( datetime->mon > 12 ) ||
         ( datetime->mon == 0 ) ||
         ( datetime->day > 31 ) ||
         ( datetime->day == 0 ) ||
         ( datetime->hour > 24 ) ||
         ( datetime->min  > 60 ) ||
         ( datetime->sec  > 60 ) )
    {
        memset( datetime,  0, sizeof( *datetime ) );
        return ( TROPICSSL_ERR_X509_CERT_INVALID_DATE );
    }

    if ( short_year != 0 )
    {
        /* Adjust year to be between 1990 and 2089 */
        datetime->year +=  100 * ( datetime->year < 50 );
        datetime->year += 1900;
    }

    return ( 0 );
}


/*
 *    Validity ::= SEQUENCE {
 *         notBefore        Time,
 *         notAfter        Time }
 *
 *    Time ::= CHOICE {
 *         utcTime        UTCTime,
 *         generalTime    GeneralizedTime }
 */
static int32_t x509_get_dates(const unsigned char **p,
              const unsigned char *end, x509_time * from, x509_time * to)
{
    int32_t ret;
    uint32_t len;
    char date[64];
    uint8_t short_year = 0;

    if ((ret = asn1_get_tag(p, end, &len,
                ASN1_CONSTRUCTED | ASN1_SEQUENCE)) != 0)
        return (TROPICSSL_ERR_X509_CERT_INVALID_DATE | ret);

    end = *p + len;

    /*
     * TODO: also handle GeneralizedTime
     */
    if( ( ( ret = asn1_get_tag( p, end, &len, ASN1_UTC_TIME ) ) == 0 ) && ( len == 13 ) )
    {
        short_year = 1;
    }
    else if( ( ( ret = asn1_get_tag( p, end, &len, ASN1_GENERALISED_TIME ) ) != 0 ) || ( len != 15 ) )
    {
        return (TROPICSSL_ERR_X509_CERT_INVALID_DATE | ret);
    }

    memset(date, 0, sizeof(date));
    memcpy(date, *p, (len < (int)sizeof(date) - 1) ?
           len : (int)sizeof(date) - 1);

    ret = parse_x509_DateTime( date, from, short_year );
    if ( ret != 0 )
    {
        return ret;
    }

    *p += len;

    short_year = 0;
    if( ( ( ret = asn1_get_tag( p, end, &len, ASN1_UTC_TIME ) ) == 0 ) && ( len == 13 ) )
    {
        short_year = 1;
    }
    else if( ( ( ret = asn1_get_tag( p, end, &len, ASN1_GENERALISED_TIME ) ) != 0 ) || ( len != 15 ) )
    {
        return (TROPICSSL_ERR_X509_CERT_INVALID_DATE | ret);
    }

    memset(date, 0, sizeof(date));
    memcpy(date, *p, (len < (int)sizeof(date) - 1) ?
           len : (int)sizeof(date) - 1);

    ret = parse_x509_DateTime( date, to, short_year );
    if ( ret != 0 )
    {
        return ret;
    }

    *p += len;

    if (*p != end)
        return (TROPICSSL_ERR_X509_CERT_INVALID_DATE |
            TROPICSSL_ERR_ASN1_LENGTH_MISMATCH);

    return (0);
}

/*
 *    SubjectPublicKeyInfo  ::=  SEQUENCE     {
 *         algorithm              AlgorithmIdentifier,
 *         subjectPublicKey      BIT STRING }
 */
static int32_t x509_get_public_key( x509_buf* pk_oid, const unsigned char **p, const unsigned char *end, wiced_tls_key_t** public_key_out )
{
    int32_t ret;
    uint32_t len;
    const unsigned char *end2;

    ret = x509_get_alg( p, end, pk_oid );
    if ( ret != 0 )
    {
        return ( ret );
    }

    ret = asn1_get_tag( p, end, &len, ASN1_BIT_STRING );
    if ( ret != 0 )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_PUBKEY | ret );
    }

    if ( ( end - *p ) < 1 )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_PUBKEY | TROPICSSL_ERR_ASN1_OUT_OF_DATA );
    }

    end2 = *p + len;

    if ( *( *p )++ != 0 )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_PUBKEY );
    }

    /* Check for PKCS1 public keys */
    if ( pk_oid->len == 9 && memcmp( pk_oid->p, OID_PKCS1, 8 ) == 0 )
    {
        rsa_context* rsa_key = (rsa_context*) *public_key_out;
        /* coverity[Resource leaks]
           FALSE-POSITIVE:
           Only when public_key_out is not allocated it allocates heap for rsa_key.
           So caller must manage this very carefully.
           However, generally public_key_out is scanning memory pointer on the x509 DER format
           from loaded file (or data packet) to parse,
           we can assume already loaded on memory. */
        if ( rsa_key == NULL )
        {
            rsa_key = tls_host_malloc( "pubkey", sizeof(rsa_context) );
            if ( rsa_key == NULL )
            {
                return TLS_ERROR_OUT_OF_MEMORY;
            }
            *public_key_out = (wiced_tls_key_t*) rsa_key;
        }

        memset( rsa_key, 0, sizeof(rsa_context) );
        rsa_key->type = TLS_RSA_KEY;
        type = TLS_RSA_KEY;
        ret = x509_get_rsa_public_key( p, end2, &rsa_key->N, &rsa_key->E );
        if ( ret != 0 )
        {
            return ( ret );
        }

        ret = rsa_check_pubkey( rsa_key );
        if ( ret != 0 )
        {
            return ( ret );
        }

        rsa_key->length = mpi_size( &rsa_key->N );
    }
    /* Check for X9.62 public keys */
    else if ( pk_oid->len == 7 && memcmp( pk_oid->p, OID_x962_KEY_TYPES, 6 ) == 0 )
    {
        int32_t l;
        uint8_t compression_type = *(*p)++;
        if ( compression_type != 0x04 && compression_type != 0x02 )
        {
            return TROPICSSL_ERR_X509_CERT_INVALID_PUBKEY;
        }

        wiced_tls_ecc_key_t* ecc_key = (wiced_tls_ecc_key_t*) *public_key_out;
        if ( ecc_key == NULL )
        {
            ecc_key = tls_host_malloc("x509", sizeof(wiced_tls_ecc_key_t));
            if ( ecc_key == NULL )
            {
                return TLS_ERROR_OUT_OF_MEMORY;
            }
            *public_key_out = (wiced_tls_key_t*) ecc_key;
        }
        memset( ecc_key, 0xFF, sizeof( *ecc_key ) );
        ecc_key->type   = TLS_ECC_KEY;
        type = TLS_ECC_KEY;
        l = len - 2;
        /* ecc_key->length have to be bigger than 0 and smaller than ECC_KEY_MAX */
        if ( ( l <= 0 ) || ( l > ECC_KEY_MAX ) )
        {
            return TROPICSSL_ERR_X509_CERT_INVALID_PUBKEY;
        }
        ecc_key->length = l;
        memcpy( ecc_key->key, *p, ecc_key->length );
        *p += ecc_key->length;
    }

    if ( *p != end )
    {
        return TROPICSSL_ERR_X509_CERT_INVALID_PUBKEY;
    }

    return 0;
}

static int32_t x509_get_rsa_public_key( const unsigned char **p, const unsigned char *end, mpi * N, mpi * E )
{
    int32_t ret;
    uint32_t len;

    /*
     *  RSAPublicKey ::= SEQUENCE {
     *      modulus           INTEGER,  -- n
     *      publicExponent    INTEGER   -- e
     *  }
     */
    ret = asn1_get_tag( p, end, &len, ASN1_CONSTRUCTED | ASN1_SEQUENCE );
    if ( ret != 0 )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_PUBKEY | ret );
    }

    if ( *p + len != end )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_PUBKEY | TROPICSSL_ERR_ASN1_LENGTH_MISMATCH );
    }

    ret = asn1_get_mpi( p, end, N );
    if ( ret != 0 )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_PUBKEY | ret );
    }
    ret = asn1_get_mpi( p, end, E );
    if ( ret != 0 )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_PUBKEY | ret );
    }

    if ( *p != end )
        return ( TROPICSSL_ERR_X509_CERT_INVALID_PUBKEY | TROPICSSL_ERR_ASN1_LENGTH_MISMATCH );

    return ( 0 );
}

static int32_t x509_get_sig(const unsigned char **p, const unsigned char *end, x509_buf *sig)
{
    int32_t ret;
    uint32_t len;

    sig->tag = **p;

    if ((ret = asn1_get_tag(p, end, &len, ASN1_BIT_STRING)) != 0)
        return (TROPICSSL_ERR_X509_CERT_INVALID_SIGNATURE | ret);

    if (--len < 1 || *(*p)++ != 0)
        return (TROPICSSL_ERR_X509_CERT_INVALID_SIGNATURE);

    sig->len = len;
    sig->p = *p;

    *p += len;

    return (0);
}

/*
 * X.509 v2/v3 unique identifier (not parsed)
 */
static int32_t x509_get_uid(const unsigned char **p,
            const unsigned char *end, x509_buf *uid, int32_t n)
{
    int32_t ret;

    if (*p == end)
        return (0);

    uid->tag = **p;

    if ((ret = asn1_get_tag(p, end, &uid->len,
                ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | n))
        != 0) {
        if (ret == TROPICSSL_ERR_ASN1_UNEXPECTED_TAG)
            return (0);

        return (ret);
    }

    uid->p = *p;
    *p += uid->len;

    return (0);
}

/*
 * X.509 v3 extensions (only BasicConstraints are parsed)
 */
static int32_t x509_get_ext(const unsigned char **p,
            const unsigned char *end,
            x509_buf * ext, int32_t *ca_istrue, int32_t *max_pathlen)
{
    int32_t ret;
    uint32_t len;
    int32_t is_critical = 1;
    int32_t is_cacert   = 0;
    const unsigned char *end2;

    if (*p == end)
        return (0);

    ext->tag = **p;

    if ((ret = asn1_get_tag(p, end, &ext->len,
                ASN1_CONTEXT_SPECIFIC | ASN1_CONSTRUCTED | 3))
        != 0) {
        if (ret == TROPICSSL_ERR_ASN1_UNEXPECTED_TAG)
            return (0);

        return (ret);
    }

    ext->p = *p;
    end = *p + ext->len;

    /*
     * Extensions  ::=      SEQUENCE SIZE (1..MAX) OF Extension
     *
     * Extension  ::=  SEQUENCE      {
     *              extnID          OBJECT IDENTIFIER,
     *              critical        BOOLEAN DEFAULT FALSE,
     *              extnValue       OCTET STRING  }
     */
    if ((ret = asn1_get_tag(p, end, &len,
                ASN1_CONSTRUCTED | ASN1_SEQUENCE)) != 0)
        return (TROPICSSL_ERR_X509_CERT_INVALID_EXTENSIONS | ret);

    if (end != *p + len)
        return (TROPICSSL_ERR_X509_CERT_INVALID_EXTENSIONS |
            TROPICSSL_ERR_ASN1_LENGTH_MISMATCH);

    while (*p < end) {
        if ((ret = asn1_get_tag(p, end, &len,
                    ASN1_CONSTRUCTED | ASN1_SEQUENCE)) != 0)
            return (TROPICSSL_ERR_X509_CERT_INVALID_EXTENSIONS |
                ret);

        if (memcmp(*p, "\x06\x03\x55\x1D\x13", 5) != 0) {
            *p += len;
            continue;
        }

        *p += 5;

        if ((ret = asn1_get_bool(p, end, &is_critical)) != 0 &&
            (ret != TROPICSSL_ERR_ASN1_UNEXPECTED_TAG))
            return (TROPICSSL_ERR_X509_CERT_INVALID_EXTENSIONS |
                ret);

        if ((ret = asn1_get_tag(p, end, &len, ASN1_OCTET_STRING)) != 0)
            return (TROPICSSL_ERR_X509_CERT_INVALID_EXTENSIONS |
                ret);

        /*
         * BasicConstraints ::= SEQUENCE {
         *              cA                                              BOOLEAN DEFAULT FALSE,
         *              pathLenConstraint               INTEGER (0..MAX) OPTIONAL }
         */
        end2 = *p + len;

        if ((ret = asn1_get_tag(p, end2, &len,
                    ASN1_CONSTRUCTED | ASN1_SEQUENCE)) != 0)
            return (TROPICSSL_ERR_X509_CERT_INVALID_EXTENSIONS |
                ret);

        if (*p == end2)
            continue;

        if ((ret = asn1_get_bool(p, end2, &is_cacert)) != 0) {
            if (ret == TROPICSSL_ERR_ASN1_UNEXPECTED_TAG)
                ret = asn1_get_int(p, end2, &is_cacert);

            if (ret != 0)
                return
                    (TROPICSSL_ERR_X509_CERT_INVALID_EXTENSIONS
                     | ret);

            if (is_cacert != 0)
                is_cacert = 1;
        }

        if (*p == end2)
            continue;

        if ((ret = asn1_get_int(p, end2, max_pathlen)) != 0)
            return (TROPICSSL_ERR_X509_CERT_INVALID_EXTENSIONS |
                ret);

        if (*p != end2)
            return (TROPICSSL_ERR_X509_CERT_INVALID_EXTENSIONS |
                TROPICSSL_ERR_ASN1_LENGTH_MISMATCH);
    }

    if (*p != end)
        return (TROPICSSL_ERR_X509_CERT_INVALID_EXTENSIONS |
            TROPICSSL_ERR_ASN1_LENGTH_MISMATCH);

    *ca_istrue = is_critical & is_cacert;

    return (0);
}

int32_t x509_convert_pem_to_der( const unsigned char* pem_certificate, uint32_t pem_certificate_length, const uint8_t** der_certificate, uint32_t* total_der_bytes )
{
    int32_t length;
    int i;
    const unsigned char *s1;
    const unsigned char *s2;
    uint32_t cert_count = 0;
    uint32_t total_pem_bytes = 0;
    const unsigned char* p = pem_certificate;

    struct
    {
        const unsigned char* pem_cert_start;
        uint32_t             pem_cert_length;
    } chained_cert_details[MAX_CERT_CHAINS];

    do
    {
        /* Verify the certificate is encoded in base64 */
        s1 = (unsigned char *) strstr( (char *) p, "-----BEGIN CERTIFICATE-----" );
        if ( s1 == NULL )
        {
            return TROPICSSL_ERR_X509_CERT_INVALID_PEM;
        }

        s2 = (unsigned char *) strstr( (char *) p, "-----END CERTIFICATE-----" );
        if ( s2 == NULL || s2 <= s1 )
        {
            return ( TROPICSSL_ERR_X509_CERT_INVALID_PEM );
        }

        s1 += 27;
        if ( *s1 == '\r' )
            s1++;
        if ( *s1 == '\n' )
            s1++;
        else
            return ( TROPICSSL_ERR_X509_CERT_INVALID_PEM );

        /* Still Coverity complains about the boundary issue,
           movied boundary checker here */
        /* generally cert chain is 1, no more than 3 in practice */
        if ( cert_count >= MAX_CERT_CHAINS )
        {
            return ( TROPICSSL_ERR_X509_CERT_INVALID_PEM );
        }

        /* Get the PEM data length */
        chained_cert_details[cert_count].pem_cert_length = s2 - s1;
        chained_cert_details[cert_count].pem_cert_start  = s1;

        total_pem_bytes += chained_cert_details[cert_count].pem_cert_length;

        /* Update the buffer size and offset */
        s2 += 25;

        /* Check if it is the end of chain of certificates */
        if ( ( s2 - pem_certificate ) >= pem_certificate_length )
        {
            ++cert_count;
            break;
        }

        if ( *s2 == '\r' )
        {
            s2++;
        }
        if ( *s2 == '\n' )
        {
            s2++;
        }
        else
        {
            return ( TROPICSSL_ERR_X509_CERT_INVALID_PEM );
        }

        ++cert_count;

        /* Move start of PEM certificate to the end of the current one */
        p = s2;
    } while ( ( s2 - pem_certificate ) < pem_certificate_length );

    /* Malloc space to store all the PEM certificates in DER format */
    *total_der_bytes = 3*total_pem_bytes/4;
    unsigned char* temp = (unsigned char *) tls_host_malloc( "x509", 3*total_pem_bytes/4 );
    if ( temp == NULL )
    {
        return ( 1 );
    }

    *der_certificate = temp;

    for ( i = 0; i < cert_count; ++i )
    {
        length = base64_decode( chained_cert_details[ i ].pem_cert_start, chained_cert_details[ i ].pem_cert_length, temp, *total_der_bytes - (temp - *der_certificate), BASE64_STANDARD );
        if ( length < 0 )
        {
            tls_host_free( (void*)*der_certificate );
            *der_certificate = NULL;
            *total_der_bytes = 0;
            return ( TROPICSSL_ERR_X509_CERT_INVALID_PEM );
        }
        temp += length;
    }

    if ( ( temp - *der_certificate ) > *total_der_bytes )
    {
        /* Fatal error. We've overrun our DER buffer */
    }

    /* Update the total DER bytes with the actual after base64 processing */
    *total_der_bytes =  temp - *der_certificate;

    return 0;
}

/*
 * Data MUST be in DER format
 */
int32_t x509_parse_certificate_data( x509_cert* crt, const unsigned char* p, uint32_t len )
{
    int32_t ret;
    const unsigned char *end;

    crt->raw.p = p;
    crt->raw.len = len;
    end = p + len;

    /*
     * Certificate  ::=      SEQUENCE  {
     *              tbsCertificate           TBSCertificate,
     *              signatureAlgorithm       AlgorithmIdentifier,
     *              signatureValue           BIT STRING      }
     */
    ret = asn1_get_tag( (const unsigned char**) &p, end, (uint32_t*) &len, ASN1_CONSTRUCTED | ASN1_SEQUENCE );
    if ( ret != 0 )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_FORMAT );
    }

    if ( len != (int) ( end - p ) )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_FORMAT | TROPICSSL_ERR_ASN1_LENGTH_MISMATCH );
    }

    /*
     * TBSCertificate  ::=  SEQUENCE  {
     */
    crt->tbs.p = p;

    ret = asn1_get_tag( (const unsigned char**) &p, end, (uint32_t*) &len, ASN1_CONSTRUCTED | ASN1_SEQUENCE );
    if ( ret != 0 )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_FORMAT | ret );
    }

    end = p + len;
    crt->tbs.len = end - crt->tbs.p;

    /*
     * Version      ::=      INTEGER  {      v1(0), v2(1), v3(2)  }
     *
     * CertificateSerialNumber      ::=      INTEGER
     *
     * signature                    AlgorithmIdentifier
     */
    ret = x509_get_version( (const unsigned char**) &p, end, &crt->version );
    if ( ret != 0 )
    {
        crt->version = 1;
    }
    ret = x509_get_serial( (const unsigned char**) &p, end, &crt->serial );
    if ( ret != 0 )
    {
        return ret;
    }
    ret = x509_get_alg( (const unsigned char**) &p, end, &crt->sig_oid1 );
    if ( ret != 0 )
    {
        return ( ret );
    }

    crt->version++;

    if ( crt->version > 3 )
    {
        return ( TROPICSSL_ERR_X509_CERT_UNKNOWN_VERSION );
    }

    unsigned char hash_algorithm;
    /* Check for PKCS1 signatures*/
    if ( memcmp( crt->sig_oid1.p, OID_PKCS1, 8 ) == 0 )
    {
        if ( crt->sig_oid1.len != 9 )
        {
            return ( TROPICSSL_ERR_X509_CERT_UNKNOWN_SIG_ALG );
        }
        hash_algorithm = crt->sig_oid1.p[8];
        /**
         * Hash Algorithms for PKCS1 RSA Certificates
         * 1 = No Hash
         * 2 = MD2
         * 3 = MD4
         * 4 = MD5
         * 5 = SHA-1
         * 6 = rsaOAEPEncryptionSET
         * 7 = id-RSAES-OAEP
         * 10 = RSASSA-PSS
         * 11 = SHA-256
         * 12 = SHA-384
         * 13 = SHA-512
         */
        if ( ( hash_algorithm < 2 ) || ( ( hash_algorithm > 5 ) && ( hash_algorithm < 11 ) ) || ( hash_algorithm > 13 ) )
        {
            return ( TROPICSSL_ERR_X509_CERT_UNKNOWN_SIG_ALG );
        }
    }
    /* Check for X9.62 signatures */
    else if ( memcmp( crt->sig_oid1.p, OID_x962_SIGNATURES, 6 ) == 0 )
    {
        if (crt->sig_oid1.len == 7)
        {
            hash_algorithm = crt->sig_oid1.p[6];
            /* Value 1 = ecdsa-with-SHA1         See http://oid-info.com/get/1.2.840.10045.4.1
             * Value 2 = ecdsa-with-Recommended  See http://oid-info.com/get/1.2.840.10045.4.2
             */
            if ( ( hash_algorithm != 1 ) && ( hash_algorithm != 2 ) )
            {
                return ( TROPICSSL_ERR_X509_CERT_UNKNOWN_SIG_ALG );
            }
        }
        else if ( crt->sig_oid1.len == 8 )
        {
            hash_algorithm = crt->sig_oid1.p[7];
            /* Value 3.1 = ecdsa-with-SHA224  See http://oid-info.com/get/1.2.840.10045.4.3.1
             * Value 3.2 = ecdsa-with-SHA256  See http://oid-info.com/get/1.2.840.10045.4.3.2
             * Value 3.3 = ecdsa-with-SHA384  See http://oid-info.com/get/1.2.840.10045.4.3.3
             * Value 3.4 = ecdsa-with-SHA512  See http://oid-info.com/get/1.2.840.10045.4.3.4
             */
            if ( ( crt->sig_oid1.p[ 6 ] != 3 ) || ( ( hash_algorithm > 4 ) || ( hash_algorithm == 0 ) ) )
            {
                return ( TROPICSSL_ERR_X509_CERT_UNKNOWN_SIG_ALG );
            }
        }
    }


    /*
     * issuer   Name
     */
    crt->issuer_raw.p = p;

    ret = asn1_get_tag( (const unsigned char**) &p, end, (uint32_t*) &len, ASN1_CONSTRUCTED | ASN1_SEQUENCE );
    if ( ret != 0 )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_FORMAT | ret );
    }

    ret = x509_get_name( (const unsigned char**) &p, p + len, &crt->issuer );
    if ( ret != 0 )
    {
        return ( ret );
    }

    crt->issuer_raw.len = p - crt->issuer_raw.p;

    /*
     * Validity ::= SEQUENCE {
     *              notBefore          Time,
     *              notAfter           Time }
     *
     */
    ret = x509_get_dates( (const unsigned char**) &p, end, &crt->valid_from, &crt->valid_to );
    if ( ret != 0 )
    {
        return ( ret );
    }

    /*
     * subject  Name
     */
    crt->subject_raw.p = p;

    ret = asn1_get_tag( (const unsigned char**) &p, end, (uint32_t*) &len, ASN1_CONSTRUCTED | ASN1_SEQUENCE );
    if ( ret != 0 )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_FORMAT | ret );
    }

    ret = x509_get_name( (const unsigned char**) &p, p + len, &crt->subject );
    if ( ret != 0 )
    {
        return ( ret );
    }

    crt->subject_raw.len = p - crt->subject_raw.p;

    /*
     * SubjectPublicKeyInfo  ::=  SEQUENCE
     *              algorithm                AlgorithmIdentifier,
     *              subjectPublicKey         BIT STRING      }
     */
    ret = asn1_get_tag( (const unsigned char**) &p, end, (uint32_t*) &len, ASN1_CONSTRUCTED | ASN1_SEQUENCE );
    if ( ret != 0 )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_FORMAT | ret );
    }

    crt->public_key = NULL;
    ret = x509_get_public_key( &crt->pk_oid, &p, p + len, &crt->public_key );
    if ( ret != 0 )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_FORMAT | ret );
    }

    /*
     *      issuerUniqueID  [1]      IMPLICIT UniqueIdentifier OPTIONAL,
     *                                               -- If present, version shall be v2 or v3
     *      subjectUniqueID [2]      IMPLICIT UniqueIdentifier OPTIONAL,
     *                                               -- If present, version shall be v2 or v3
     *      extensions              [3]      EXPLICIT Extensions OPTIONAL
     *                                               -- If present, version shall be v3
     */
    if (crt->version == 2 || crt->version == 3) {
        ret = x509_get_uid((const unsigned char**) &p, end, &crt->issuer_id, 1);
        if (ret != 0) {
            return (ret);
        }
    }

    if (crt->version == 2 || crt->version == 3) {
        ret = x509_get_uid((const unsigned char**) &p, end, &crt->subject_id, 2);
        if (ret != 0) {
            return (ret);
        }
    }

    if (crt->version == 3) {
        ret = x509_get_ext((const unsigned char**) &p, end, &crt->v3_ext,
                   &crt->ca_istrue, &crt->max_pathlen);
        if (ret != 0) {
            return (ret);
        }
    }

    if (p != end) {
        return (TROPICSSL_ERR_X509_CERT_INVALID_FORMAT |
            TROPICSSL_ERR_ASN1_LENGTH_MISMATCH);
    }

    end = crt->raw.p + crt->raw.len;

    /*
     *      signatureAlgorithm       AlgorithmIdentifier,
     *      signatureValue           BIT STRING
     */
    ret = x509_get_alg( (const unsigned char**) &p, end, &crt->sig_oid2 ) ;
    if ( ret != 0 )
    {
        return ( ret );
    }

    if ( memcmp( crt->sig_oid1.p, crt->sig_oid2.p, crt->sig_oid1.len ) != 0 )
    {
        return ( TROPICSSL_ERR_X509_CERT_SIG_MISMATCH );
    }

    ret = x509_get_sig( (const unsigned char**) &p, end, &crt->sig );
    if ( ret != 0 )
    {
        return ( ret );
    }

    if ( p != end )
    {
        return ( TROPICSSL_ERR_X509_CERT_INVALID_FORMAT |
        TROPICSSL_ERR_ASN1_LENGTH_MISMATCH );
    }

    return (0);
}

/*
 * Parse one or more certificates and add them to the chained list
 */
int32_t x509_parse_certificate( x509_cert* chain, const uint8_t* buf, uint32_t buflen )
{
    int            ret;
    const uint8_t* der_certificate;
    const uint8_t* current_cert_pointer;
    uint32_t       der_certificate_length;
    uint32_t       total_der_bytes = 0;
    char           malloced_der_certificate = 1;

    /* Check if certificate is PEM
     * Note: This function will malloc space for DER cert */
    ret = x509_convert_pem_to_der( buf, buflen, &der_certificate, &total_der_bytes );
    if ( ret != 0 )
    {
        der_certificate = buf;
        total_der_bytes = buflen;
        malloced_der_certificate = 0;
    }

    current_cert_pointer = der_certificate;
    do
    {
        der_certificate_length = x509_read_cert_length( current_cert_pointer );

        ret = x509_parse_certificate_data( chain, current_cert_pointer, der_certificate_length );
        if ( ret != 0 )
        {
            x509_free( chain );
            if ( malloced_der_certificate == 1)
            {
                tls_host_free( (void*) der_certificate );
            }
            return ret;
        }

        total_der_bytes -= der_certificate_length;
        current_cert_pointer += der_certificate_length;

        if ( total_der_bytes > 0 )
        {
            chain->next = (x509_cert *) tls_host_malloc( "x509", sizeof(x509_cert) );

            if ( chain->next == NULL )
            {
                x509_free( chain );
                if ( malloced_der_certificate == 1)
                {
                    tls_host_free( (void*) der_certificate );
                }
                return ( 1 );
            }

            chain = chain->next;
            memset( chain, 0, sizeof(x509_cert) );
        }
    } while ( total_der_bytes > 0 );

    chain->der_certificate_malloced = malloced_der_certificate;
    chain->der_certificate_data = (uint8_t*)der_certificate;
    chain->der_certificate_length = der_certificate_length;

    return ret;
}

#if defined(TROPICSSL_DES_C)
/*
 * Read a 16-byte hex string and convert it to binary
 */
static int32_t x509_get_iv(const unsigned char *s, unsigned char iv[8])
{
    int32_t i, j, k;

    memset(iv, 0, 8);

    for (i = 0; i < 16; i++, s++) {
        if (*s >= '0' && *s <= '9')
            j = *s - '0';
        else if (*s >= 'A' && *s <= 'F')
            j = *s - '7';
        else if (*s >= 'a' && *s <= 'f')
            j = *s - 'W';
        else
            return (TROPICSSL_ERR_X509_KEY_INVALID_ENC_IV);

        k = ((i & 1) != 0) ? j : j << 4;

        iv[i >> 1] = (unsigned char)(iv[i >> 1] | k);
    }

    return (0);
}

/*
 * Decrypt with 3DES-CBC, using PBKDF1 for key derivation
 * Works on buf data in-place
 */
static void x509_des3_decrypt(unsigned char des3_iv[8],
                  unsigned char *buf, int32_t buflen,
                  const unsigned char *pwd, int32_t pwdlen)
{
    md5_context md5_ctx;
    des3_context des3_ctx;
    unsigned char md5sum[16];
    unsigned char des3_key[24];

    /*
     * 3DES key[ 0..15] = MD5(pwd || IV)
     *              key[16..23] = MD5(pwd || IV || 3DES key[ 0..15])
     */
    md5_starts(&md5_ctx);
    md5_update(&md5_ctx, pwd, pwdlen);
    md5_update(&md5_ctx, des3_iv, 8);
    md5_finish(&md5_ctx, md5sum);
    memcpy(des3_key, md5sum, 16);

    md5_starts(&md5_ctx);
    md5_update(&md5_ctx, md5sum, 16);
    md5_update(&md5_ctx, pwd, pwdlen);
    md5_update(&md5_ctx, des3_iv, 8);
    md5_finish(&md5_ctx, md5sum);
    memcpy(des3_key + 16, md5sum, 8);

    des3_set3key_dec(&des3_ctx, des3_key);
    des3_crypt_cbc(&des3_ctx, DES_DECRYPT, buflen, des3_iv, buf, buf);

    memset(&md5_ctx, 0, sizeof(md5_ctx));
    memset(&des3_ctx, 0, sizeof(des3_ctx));
    memset(md5sum, 0, 16);
    memset(des3_key, 0, 24);
}
#endif

/*
 * Parse a PUBLIC RSA key
 */
int32_t x509parse_pubkey( rsa_context *rsa, unsigned char *buf, int32_t buflen, unsigned char *pwd, int32_t pwdlen )
{
    int32_t ret, enc;
    uint32_t len;
    int32_t  newlen;
    const unsigned char *s1, *s2;
    const unsigned char *p = buf, *end;
    x509_buf pk_oid;
    unsigned char *decode_buf = NULL;
    unsigned char des3_iv[8];
    s1 = (unsigned char *)strstr((char *)buf,
                     "-----BEGIN PUBLIC KEY-----" );

    if (s1 != NULL) {
        s2 = (unsigned char *)strstr((char *)buf,
                         "-----END PUBLIC KEY-----");

        if (s2 == NULL || s2 <= s1)
            return (TROPICSSL_ERR_X509_KEY_INVALID_PEM);

        s1 += 26;
        if (*s1 == '\r')
            s1++;
        if (*s1 == '\n')
            s1++;
        else
            return (TROPICSSL_ERR_X509_KEY_INVALID_PEM);

        enc = 0;

        if( memcmp( s1, "Proc-Type: 4,ENCRYPTED", 22 ) == 0 ) {
#if defined(TROPICSSL_DES_C)
            enc++;

            s1 += 22;
            if (*s1 == '\r')
                s1++;
            if (*s1 == '\n')
                s1++;
            else
                return (TROPICSSL_ERR_X509_KEY_INVALID_PEM);

            if (memcmp(s1, "DEK-Info: DES-EDE3-CBC,", 23) != 0)
                return (TROPICSSL_ERR_X509_KEY_UNKNOWN_ENC_ALG);

            s1 += 23;
            if (x509_get_iv(s1, des3_iv) != 0)
                return (TROPICSSL_ERR_X509_KEY_INVALID_ENC_IV);

            s1 += 16;
            if (*s1 == '\r')
                s1++;
            if (*s1 == '\n')
                s1++;
            else
                return (TROPICSSL_ERR_X509_KEY_INVALID_PEM);
#else
            return (TROPICSSL_ERR_X509_FEATURE_UNAVAILABLE);
#endif
        }

        len = 3 * ( (((uint32_t) ( s2 - s1 )) + 3 ) / 4);

        decode_buf = (unsigned char *) tls_host_malloc( "x509",  len );
        if( decode_buf == NULL )
        {
            return( 1 );
        }

        newlen = base64_decode( s1, s2 - s1, decode_buf, len, BASE64_STANDARD );
        if( newlen < 0 )
        {
            tls_host_free ( decode_buf );
            return( TROPICSSL_ERR_X509_KEY_INVALID_PEM );
        }
        len = newlen;
        buflen = len;
        p = decode_buf;

        if (enc != 0) {
#if defined(TROPICSSL_DES_C)
            if (pwd == NULL) {
                tls_host_free ( decode_buf );
                return
                    (TROPICSSL_ERR_X509_KEY_PASSWORD_REQUIRED);
            }

            x509_des3_decrypt(des3_iv, decode_buf, buflen, pwd, pwdlen);

            if (decode_buf[0] != 0x30 || decode_buf[1] != 0x82 ||
                decode_buf[4] != 0x02 || decode_buf[5] != 0x01) {
                tls_host_free ( decode_buf );
                return
                    (TROPICSSL_ERR_X509_KEY_PASSWORD_MISMATCH);
            }
#else
            return (TROPICSSL_ERR_X509_FEATURE_UNAVAILABLE);
#endif
        }
    }

    memset(rsa, 0, sizeof(rsa_context));

    end = p + buflen;

    /*
     *  RSAPublicKey ::= SEQUENCE {
     *      modulus           INTEGER,  -- n
     *      publicExponent    INTEGER,  -- e
     *  }
     */
    if ((ret = asn1_get_tag(&p, end, &len,
                ASN1_CONSTRUCTED | ASN1_SEQUENCE)) != 0) {
        if (s1 != NULL)
            tls_host_free ( decode_buf );

        rsa_free(rsa);
        return (TROPICSSL_ERR_X509_KEY_INVALID_FORMAT | ret);
    }

    end = p + len;

    /* coverity[Resource leaks]
       FALSE-POSITIVE:
       rsa_context *rsa should be allocated statically (or dynamically) by caller.
       generally this is the x509 DER format from loaded file (or data packet) to parse,
       we can assume already loaded on memory.
       Refer to the another comment on the function x509_get_public_key as well */
    if ((ret = x509_get_public_key( &pk_oid, &p, p + len, (wiced_tls_key_t**)&rsa)) != 0) {
        if (s1 != NULL)
            tls_host_free ( decode_buf );

        rsa_free(rsa);
        return( ret );
    }

    rsa->length = mpi_size( &rsa->N );

    if (p != end) {
        if (s1 != NULL)
            tls_host_free ( decode_buf );

        rsa_free(rsa);
        return (TROPICSSL_ERR_X509_KEY_INVALID_FORMAT |
            TROPICSSL_ERR_ASN1_LENGTH_MISMATCH);
    }

    if( ( ret = rsa_check_pubkey( rsa ) ) != 0 ){
        if (s1 != NULL)
            tls_host_free ( decode_buf );

        rsa_free(rsa);
        return (ret);
    }

    if (s1 != NULL)
        tls_host_free ( decode_buf );

    return (0);
}

/*
 * Parse a private RSA key
 */
static int32_t _x509parse_key_1(rsa_context * rsa, const unsigned char *buf, uint32_t buflen,
          const unsigned char *pwd, uint32_t pwdlen)
{
    int32_t ret, enc;
    uint32_t len;
    int32_t  newlen;
    unsigned char *decode_buf = NULL;
    const unsigned char *s1, *s2;
    const unsigned char *p = buf, *end;
    unsigned char des3_iv[8];

    s1 = (unsigned char *)strstr((char *)buf,
                     "-----BEGIN RSA PRIVATE KEY-----");

    if (s1 != NULL) {
        s2 = (unsigned char *)strstr((char *)buf,
                         "-----END RSA PRIVATE KEY-----");

        if (s2 == NULL || s2 <= s1)
            return (TROPICSSL_ERR_X509_KEY_INVALID_PEM);

        s1 += 31;
        if (*s1 == '\r')
            s1++;
        if (*s1 == '\n')
            s1++;
        else
            return (TROPICSSL_ERR_X509_KEY_INVALID_PEM);

        enc = 0;

        if (memcmp(s1, "Proc-Type: 4,ENCRYPTED", 22) == 0) {
#if defined(TROPICSSL_DES_C)
            enc++;

            s1 += 22;
            if (*s1 == '\r')
                s1++;
            if (*s1 == '\n')
                s1++;
            else
                return (TROPICSSL_ERR_X509_KEY_INVALID_PEM);

            if (memcmp(s1, "DEK-Info: DES-EDE3-CBC,", 23) != 0)
                return (TROPICSSL_ERR_X509_KEY_UNKNOWN_ENC_ALG);

            s1 += 23;
            if (x509_get_iv(s1, des3_iv) != 0)
                return (TROPICSSL_ERR_X509_KEY_INVALID_ENC_IV);

            s1 += 16;
            if (*s1 == '\r')
                s1++;
            if (*s1 == '\n')
                s1++;
            else
                return (TROPICSSL_ERR_X509_KEY_INVALID_PEM);
#else
            return (TROPICSSL_ERR_X509_FEATURE_UNAVAILABLE);
#endif
        }

        len = 3 * ( (((uint32_t) ( s2 - s1 )) + 3 ) / 4);


        decode_buf = (unsigned char *) tls_host_malloc( "x509",  len );
        if( decode_buf == NULL )
        {
            return (1);
        }

        newlen = base64_decode( s1, s2 - s1, decode_buf, len, BASE64_STANDARD );
        if ( newlen < 0 )
        {
            tls_host_free ( decode_buf );
            return (TROPICSSL_ERR_X509_KEY_INVALID_PEM);
        }

        len = newlen;
        buflen = len;
        p = decode_buf;

        if (enc != 0) {
#if defined(TROPICSSL_DES_C)
            if (pwd == NULL) {
                tls_host_free ( decode_buf );
                return
                    (TROPICSSL_ERR_X509_KEY_PASSWORD_REQUIRED);
            }

            x509_des3_decrypt(des3_iv, decode_buf, buflen, pwd, pwdlen);

            if (decode_buf[0] != 0x30 || decode_buf[1] != 0x82 ||
                decode_buf[4] != 0x02 || decode_buf[5] != 0x01) {
                tls_host_free ( decode_buf );
                return
                    (TROPICSSL_ERR_X509_KEY_PASSWORD_MISMATCH);
            }
#else
            return( TROPICSSL_ERR_X509_FEATURE_UNAVAILABLE );
#endif
        }
    }

    memset( rsa, 0, sizeof( rsa_context ) );

    end = p + buflen;

    /*
     *      RSAPrivateKey ::= SEQUENCE {
     *              version                   Version,
     *              modulus                   INTEGER,      -- n
     *              publicExponent    INTEGER,      -- e
     *              privateExponent   INTEGER,      -- d
     *              prime1                    INTEGER,      -- p
     *              prime2                    INTEGER,      -- q
     *              exponent1                 INTEGER,      -- d mod (p-1)
     *              exponent2                 INTEGER,      -- d mod (q-1)
     *              coefficient               INTEGER,      -- (inverse of q) mod p
     *              otherPrimeInfos   OtherPrimeInfos OPTIONAL
     *      }
     */
    if ((ret = asn1_get_tag(&p, end, &len,
                ASN1_CONSTRUCTED | ASN1_SEQUENCE)) != 0) {
        if (s1 != NULL)
            tls_host_free ( decode_buf );

        rsa_free( rsa );
        return( TROPICSSL_ERR_X509_KEY_INVALID_FORMAT | ret );
    }

    end = p + len;

    if ((ret = asn1_get_int(&p, end, (int32_t*)&rsa->version)) != 0) {
        if (s1 != NULL)
            tls_host_free ( decode_buf );

        rsa_free(rsa);
        return (TROPICSSL_ERR_X509_KEY_INVALID_FORMAT | ret);
    }

    if (rsa->version != 0) {
        if (s1 != NULL)
            tls_host_free ( decode_buf );

        rsa_free( rsa );
        return( ret | TROPICSSL_ERR_X509_KEY_INVALID_VERSION );
    }

    if ((ret = asn1_get_mpi((const unsigned char**) &p, end, &rsa->N  ) ) != 0 ||
        (ret = asn1_get_mpi((const unsigned char**) &p, end, &rsa->E  ) ) != 0 ||
        (ret = asn1_get_mpi((const unsigned char**) &p, end, &rsa->D  ) ) != 0 ||
        (ret = asn1_get_mpi((const unsigned char**) &p, end, &rsa->P  ) ) != 0 ||
        (ret = asn1_get_mpi((const unsigned char**) &p, end, &rsa->Q  ) ) != 0 ||
        (ret = asn1_get_mpi((const unsigned char**) &p, end, &rsa->DP ) ) != 0 ||
        (ret = asn1_get_mpi((const unsigned char**) &p, end, &rsa->DQ ) ) != 0 ||
        (ret = asn1_get_mpi((const unsigned char**) &p, end, &rsa->QP ) ) != 0) {
        if (s1 != NULL)
            tls_host_free (decode_buf);

        rsa_free(rsa);
        return (ret | TROPICSSL_ERR_X509_KEY_INVALID_FORMAT);
    }

    rsa->length = mpi_size(&rsa->N);

    if (p != end) {
        if (s1 != NULL)
            tls_host_free ( decode_buf );

        rsa_free(rsa);
        return (TROPICSSL_ERR_X509_KEY_INVALID_FORMAT |
            TROPICSSL_ERR_ASN1_LENGTH_MISMATCH);
    }

    if ((ret = rsa_check_privkey(rsa)) != 0) {
        if (s1 != NULL)
            tls_host_free ( decode_buf );

        rsa_free(rsa);
        return (ret);
    }

    if (s1 != NULL)
        tls_host_free ( decode_buf );

    return( 0 );
}

/*
 * Parse a private ECC key
 */
static int32_t _x509parse_key_2(wiced_tls_ecc_key_t * ecc, const unsigned char *buf, uint32_t buflen,
          const unsigned char *pwd, uint32_t pwdlen)
{
    int32_t ret, enc;
    uint32_t len;
    int32_t newlen;
    unsigned char *decode_buf = NULL;
    const unsigned char *s1, *s2;
    const unsigned char *p = buf, *end;

    s1 = (unsigned char *) strstr( (char *) buf, "-----BEGIN EC PRIVATE KEY-----" );

    if ( s1 != NULL )
    {
        s2 = (unsigned char *) strstr( (char *) buf, "-----END EC PRIVATE KEY-----" );

        if ( s2 == NULL || s2 <= s1 )
            return ( TROPICSSL_ERR_X509_KEY_INVALID_PEM );

        s1 += 30;

        if ( *s1 == '\r' )
            s1++;
        if ( *s1 == '\n' )
            s1++;
        else
            return ( TROPICSSL_ERR_X509_KEY_INVALID_PEM );

        enc = 0;

        len = 3 * ( ( ( (uint32_t) ( s2 - s1 ) ) + 3 ) / 4 );

        decode_buf = (unsigned char *) tls_host_malloc( "x509", len );
        if ( decode_buf == NULL )
        {
            return ( 1 );
        }

        newlen = base64_decode( s1, s2 - s1, decode_buf, len, BASE64_STANDARD );
        if ( newlen < 0 )
        {
            tls_host_free( decode_buf );
            return ( TROPICSSL_ERR_X509_KEY_INVALID_PEM );
        }

        len = newlen;
        buflen = len;
        p = decode_buf;

        (void) end;
        (void) enc;
        (void) ret;
    }

    memset( ecc, 0, sizeof(wiced_tls_ecc_key_t) );

    ecc->length = uECC_BYTES;
    memcpy( ecc->key, ( p + 7 ), ecc->length );
    ecc->type = TLS_ECC_KEY;
    ecc->version = 0;
    end = p + buflen;

/*
 *   ECPrivateKey ::= SEQUENCE {
 *   version        INTEGER { ecPrivkeyVer1(1) } (ecPrivkeyVer1),
 *   privateKey     OCTET STRING,
 *   parameters [0] ECParameters {{ NamedCurve }} OPTIONAL,
 *   publicKey  [1] BIT STRING OPTIONAL
 *   }
 */

    tls_host_free( decode_buf );
    return ( 0 );
}

/*
 * Parse a private RSA key
 */
int32_t x509parse_key(rsa_context * rsa,
          const unsigned char *key, uint32_t keylen,
          const unsigned char *pwd, uint32_t pwdlen)
{
    int ret;
    unsigned char *buf1;
    buf1 = malloc(keylen);
    if (buf1 == NULL) {
        return (1);
    }
    memcpy(buf1, key, keylen);
    ret = _x509parse_key_1(rsa, buf1, keylen, pwd, pwdlen);
    free(buf1);
    return ret;
}

/*
 * Parse a private ECC key
 */
int32_t x509parse_key_ecc(wiced_tls_ecc_key_t * ecc,
          const unsigned char *key, uint32_t keylen,
          const unsigned char *pwd, uint32_t pwdlen)
{
    int ret;
    unsigned char *buf1;
    buf1 = malloc(keylen);
    if (buf1 == NULL) {
        return (1);
    }
    memcpy(buf1, key, keylen);
    ret = _x509parse_key_2(ecc, buf1, keylen, pwd, pwdlen);
    free(buf1);
    return ret;
}

#ifdef ENABLE_X509_FUNCTIONS_WHICH_USE_SPRINTF


#if defined _MSC_VER && !defined snprintf
#define snprintf _snprintf
#endif

#include <stdarg.h>
static int snprintf2( char * s, size_t n, char **buf, const char * format, ... )
{
    int ret;
    va_list args;

    va_start(args, format);
    ret = vsnprintf( s, n, format, args );
    va_end(args);

    if ( ret >= 0 )
    {
        *buf += ret;
    }

    printf( "%s\n", s );
    printf( "ret = %d\n", ret );

    return ret;
}

/*
 * Store the name in printable form into buf; no more
 * than (end - buf) characters will be written
 */
int32_t x509parse_dn_gets(char *buf, const char *end, const x509_name *dn)
{
    int32_t i, ret;
    unsigned char c;
    const x509_name *name;
    char s[128], *p;

    memset(s, 0, sizeof(s));

    name = dn;
    p = buf;

    while (name != NULL) {
        if (name != dn)
            if ( ( ret = snprintf2(p, end - p, &p, ", ") ) < 0 )
                return ret;

        if (memcmp(name->oid.p, OID_X520, 2) == 0) {
            switch (name->oid.p[2]) {
            case X520_COMMON_NAME:
                if ( ( ret = snprintf2( p, end - p, &p, "CN=" ) ) < 0 )
                    return ret;
                break;

            case X520_COUNTRY:
                if ( ( ret = snprintf2( p, end - p, &p, "C=" ) ) < 0 )
                    return ret;
                break;

            case X520_LOCALITY:
                if ( ( ret = snprintf2( p, end - p, &p, "L=" ) ) < 0 )
                    return ret;
                break;

            case X520_STATE:
                if ( ( ret = snprintf2( p, end - p, &p, "ST=" ) ) < 0 )
                    return ret;
                break;

            case X520_ORGANIZATION:
                if ( ( ret = snprintf2( p, end - p, &p, "O=" ) ) < 0 )
                    return ret;
                break;

            case X520_ORG_UNIT:
                if ( ( ret = snprintf2( p, end - p, &p, "OU=" ) ) < 0 )
                    return ret;
                break;

            default:
                if ( ( ret = snprintf2( p, end - p, &p, "0x%02X=", name->oid.p[2] ) ) < 0 )
                    return ret;
                break;
            }
        } else if (memcmp(name->oid.p, OID_PKCS9, 8) == 0) {
            switch (name->oid.p[8]) {
            case PKCS9_EMAIL:
                if ( ( ret = snprintf2( p, end - p, &p, "emailAddress=" ) ) < 0 )
                    return ret;
                break;

            default:
                if ( ( ret = snprintf2( p, end - p, &p, "0x%02X=", name->oid.p[8] ) ) < 0 )
                    return ret;
                break;
            }
        } else
            if ( ( ret = snprintf2( p, end - p, &p, "\?\?=" ) ) < 0 )
                    return ret;

        for (i = 0; i < name->val.len; i++) {
            if (i >= (int)sizeof(s) - 1)
                break;

            c = name->val.p[i];
            if (c < 32 || c == 127 || (c > 128 && c < 160))
                s[i] = '?';
            else
                s[i] = c;
        }
        s[i] = '\0';
        if ( ( ret = snprintf2( p, end - p, &p, "%s", s ) ) < 0 )
                    return ret;
        name = name->next;
    }

    return (p - buf);
}

/*
 * Return an informational string about the
 * certificate, or NULL if memory allocation failed
 */
char *x509parse_cert_info(char *buf, size_t buf_size,
              const char *prefix, const x509_cert * crt)
{
    int32_t i, n, ret;
    char *p;
    const char *end;

    p = buf;
    end = buf + buf_size - 1;

    if ( snprintf2(p, end - p, &p, "%scert. version : %ld\n", prefix, (long)crt->version) < 0 )
        return NULL;
    if ( snprintf2(p, end - p, &p, "%sserial number : ", prefix) < 0 )
        return NULL;

    n = (crt->serial.len <= 32)
        ? crt->serial.len : 32;

    for (i = 0; i < n; i++)
        if ( snprintf2(p, end - p, &p, "%02X%s", crt->serial.p[i], (i < n - 1) ? ":" : "") < 0 )
            return NULL;

    if ( snprintf2(p, end - p, &p, "\n%sissuer    name  : ", prefix) < 0 )
        return NULL;
    if ( ( ret = x509parse_dn_gets(p, end, &crt->issuer ) ) < 0 )
        return NULL;
    p += ret;

    if ( snprintf2(p, end - p, &p, "\n%ssubject name  : ", prefix) < 0 )
        return NULL;
    if ( ( ret = x509parse_dn_gets(p, end, &crt->subject ) ) < 0 )
        return NULL;
    p += ret;

    if ( snprintf2(p, end - p, &p, "\n%sissued    on      : "
              "%04ld-%02ld-%02ld %02ld:%02ld:%02ld", prefix,
              (long)crt->valid_from.year, (long)crt->valid_from.mon,
              (long)crt->valid_from.day, (long)crt->valid_from.hour,
              (long)crt->valid_from.min, (long)crt->valid_from.sec) < 0 )
        return NULL;

    if ( snprintf2(p, end - p, &p, "\n%sexpires on      : "
              "%04ld-%02ld-%02ld %02ld:%02ld:%02ld", prefix,
              (long)crt->valid_to.year, (long)crt->valid_to.mon,
              (long)crt->valid_to.day, (long)crt->valid_to.hour,
              (long)crt->valid_to.min, (long)crt->valid_to.sec) < 0 )
        return NULL;

    if ( snprintf2(p, end - p, &p, "\n%ssigned using  : RSA+", prefix) < 0 )
        return NULL;

    switch (crt->sig_oid1.p[8]) {
    case RSA_MD2:
        if ( snprintf2(p, end - p, &p, "MD2") < 0 )
            return NULL;
        break;
    case RSA_MD4:
        if ( snprintf2(p, end - p, &p, "MD4") < 0 )
            return NULL;
        break;
    case RSA_MD5:
        if ( snprintf2(p, end - p, &p, "MD5") < 0 )
            return NULL;
        break;
    case RSA_SHA1:
        if ( snprintf2(p, end - p, &p, "SHA1") < 0 )
            return NULL;
        break;
    case RSA_SHA256:
        if ( snprintf2( p, end - p, &p, "SHA256" ) < 0 )
            return NULL;
        break;
    case RSA_SHA384:
        if ( snprintf2( p, end - p, &p, "SHA384" ) < 0 )
            return NULL;
        break;
    case RSA_SHA512:
        if ( snprintf2( p, end - p, &p, "SHA512" ) < 0 )
            return NULL;
        break;
    default:
        if ( snprintf2(p, end - p, &p, "???") < 0 )
            return NULL;
        break;
    }

    if ( crt->public_key->type == TLS_RSA_KEY )
    {
        rsa_context* rsa = (rsa_context*) crt->public_key;
        if ( snprintf(p, end - p, "\n%sRSA key size  : %ld bits\n", prefix,
                  (long)(rsa->N.n * (int) sizeof(uint32_t) * 8)) < 0 )
            return NULL;
    }
    else if ( crt->public_key->type == TLS_ECC_KEY )
    {
        wiced_tls_ecc_key_t* ecc = (wiced_tls_ecc_key_t*) crt->public_key;
        /* for brainpool curve key size in bit is length * 4 */
        if ( snprintf(p, end - p, "\n%sECC key size  : %ld bits\n", prefix,
                  (long)(ecc->length * 4)) < 0 )
            return NULL;
    }

    printf( "%s\n", buf );

    return (buf);
}

#endif /* ifdef ENABLE_X509_FUNCTIONS_WHICH_USE_SPRINTF */

/*
 * Return 0 if the certificate is still valid, or BADCERT_EXPIRED
 */
int32_t x509parse_expired(const x509_cert *crt)
{
    struct tm *lt;
    time_t tt;

    tt = (time_t)( tls_host_get_time_ms( ) / 1000 );
    lt = localtime(&tt);

    if (lt->tm_year > crt->valid_to.year - 1900)
        return (BADCERT_EXPIRED);

    if (lt->tm_year == crt->valid_to.year - 1900 &&
        lt->tm_mon > crt->valid_to.mon - 1)
        return (BADCERT_EXPIRED);

    if (lt->tm_year == crt->valid_to.year - 1900 &&
        lt->tm_mon == crt->valid_to.mon - 1 &&
        lt->tm_mday > crt->valid_to.day)
        return (BADCERT_EXPIRED);

    return (0);
}

static void x509_hash(const unsigned char *in, int32_t len, int32_t alg, unsigned char out[64])
{
    switch (alg) {
#if defined(TROPICSSL_MD2_C)
    case RSA_MD2:
        md2(in, len, out);
        break;
#endif

#if defined(TROPICSSL_MD4_C)
    case RSA_MD4:
        md4(in, len, out);
        break;
#endif

#if defined( X509_SUPPORT_MD5 )
    case RSA_MD5:
        md5(in, len, out);
        break;
#endif

#if defined( X509_SUPPORT_SHA1 )
    case RSA_SHA1:
        sha1(in, len, out);
        break;
#endif

#if defined( X509_SUPPORT_SHA256 )
    case RSA_SHA256:
        sha2( in, len, out, 0 );
        break;
#endif

#if defined( X509_SUPPORT_SHA384 )
    case RSA_SHA384:
        sha4( in, len, out, 1 );
        break;
#endif

#if defined( X509_SUPPORT_SHA512 )
    case RSA_SHA512:
        sha4( in, len, out, 0 );
        break;
#endif

    default:
        memset( out, '\xFF', 64 );
        break;
    }
}

/*
 * Verify the certificate validity
 */
int32_t x509parse_verify(const x509_cert *crt,
             const x509_cert *trust_ca, const char *cn, int32_t *flags)
{
    int32_t cn_len;
    int32_t hash_id;
    const x509_cert *cur;
    const x509_cert *trusted_ca_iter;
    const x509_name *name;
    unsigned char hash[64];

    *flags = x509parse_expired(crt);

    if (cn != NULL) {
        name = &crt->subject;
        cn_len = strlen(cn);

        while (name != NULL) {
            if (memcmp(name->oid.p, OID_CN, 3) == 0 &&
                memcmp(name->val.p, cn, cn_len) == 0 &&
                name->val.len == cn_len)
                break;

            name = name->next;
        }

        if (name == NULL)
            *flags |= BADCERT_CN_MISMATCH;
    }

    *flags |= BADCERT_NOT_TRUSTED;

    /* Traverse through cert chain attempting to find a matching trusted cert */
    for ( cur = crt; cur != NULL && ( *flags & BADCERT_NOT_TRUSTED ) != 0 && cur->version != 0; cur = cur->next )
    {
        /* Verify current certificate is correctly signed by next */
        if ( cur->next != NULL && cur->next->version != 0 )
        {
            hash_id = cur->sig_oid1.p[ 8 ];

            x509_hash( cur->tbs.p, cur->tbs.len, hash_id, hash );

            if ( rsa_pkcs1_verify( (rsa_context*)cur->next->public_key, RSA_PUBLIC, ( rsa_hash_id_t ) hash_id, 0, hash, cur->sig.p ) != 0 )
            {
                return ( TROPICSSL_ERR_X509_CERT_VERIFY_FAILED );
            }
        }

        /* Check if current cert has been issued by trusted root cert */
        for ( trusted_ca_iter = trust_ca; trusted_ca_iter != NULL && trusted_ca_iter->version != 0; trusted_ca_iter = trusted_ca_iter->next )
        {
            if ( cur->issuer_raw.len == trusted_ca_iter->subject_raw.len &&
                 memcmp( cur->issuer_raw.p, trusted_ca_iter->subject_raw.p, cur->issuer_raw.len ) == 0 )
            {
                hash_id = cur->sig_oid1.p[ 8 ];

                x509_hash( cur->tbs.p, cur->tbs.len, hash_id, hash );

                if ( trusted_ca_iter->public_key->type == TLS_RSA_KEY )
                {
                    if ( rsa_pkcs1_verify( (rsa_context*) trusted_ca_iter->public_key, RSA_PUBLIC, ( rsa_hash_id_t ) hash_id, 0, hash, cur->sig.p ) != 0 )
                    {
                        return ( TROPICSSL_ERR_X509_CERT_VERIFY_FAILED );
                    }
                }
                else if ( trusted_ca_iter->public_key->type == TLS_ECC_KEY )
                {
                    if ( uECC_verify( trusted_ca_iter->public_key->data, hash, cur->sig.p ) != 0 )
                    {
                        return ( TROPICSSL_ERR_X509_CERT_VERIFY_FAILED );
                    }
                }

                *flags &= ~BADCERT_NOT_TRUSTED;
                break;
            }
        }
    }

    if (*flags != 0)
        return (TROPICSSL_ERR_X509_CERT_VERIFY_FAILED);

    return (0);
}

/*
 * Unallocate all certificate data
 */
void x509_free(x509_cert * crt)
{
    x509_cert *cert_cur = crt;
    x509_cert *cert_prv;
    x509_name *name_cur;
    x509_name *name_prv;

    if (crt == NULL)
        return;

    do {
        if ( cert_cur->public_key != NULL )
        {
            if ( cert_cur->public_key->type == TLS_RSA_KEY )
            {
                rsa_free( (rsa_context*) cert_cur->public_key );
                tls_host_free( cert_cur->public_key );
            }
            else
            {
                tls_host_free( cert_cur->public_key );
            }
            cert_cur->public_key = NULL;
        }

        name_cur = cert_cur->issuer.next;
        while (name_cur != NULL) {
            name_prv = name_cur;
            name_cur = name_cur->next;
            memset(name_prv, 0, sizeof(x509_name));
            tls_host_free ( name_prv );
        }

        name_cur = cert_cur->subject.next;
        while (name_cur != NULL) {
            name_prv = name_cur;
            name_cur = name_cur->next;
            memset(name_prv, 0, sizeof(x509_name));
            tls_host_free ( name_prv );
        }

        if ( cert_cur->der_certificate_malloced == 1)
        {
            memset( cert_cur->der_certificate_data, 0, cert_cur->der_certificate_length );
            tls_host_free( (void*) cert_cur->der_certificate_data );
        }

        cert_cur = cert_cur->next;
    } while (cert_cur != NULL);

    cert_cur = crt;
    do {
        cert_prv = cert_cur;
        cert_cur = cert_cur->next;

        memset(cert_prv, 0, sizeof(x509_cert));
        if (cert_prv != crt)
            tls_host_free ( cert_prv );
    } while (cert_cur != NULL);
}

#if defined(TROPICSSL_SELF_TEST)

#include <certs.h>

/*
 * Checkup routine
 */
int32_t x509_self_test(int32_t verbose)
{
    int32_t ret, i, j;
    x509_cert cacert;
    x509_cert clicert;
    rsa_context rsa;

    if (verbose != 0)
        printf("  X.509 certificate load: ");

    memset(&clicert, 0, sizeof(x509_cert));

    ret = x509_parse_certificate(&clicert, (const unsigned char *)test_cli_crt,
                strlen(test_cli_crt));
    if (ret != 0) {
        if (verbose != 0)
            printf("failed\n");

        return (ret);
    }

    memset(&cacert, 0, sizeof(x509_cert));

    ret = x509_parse_certificate(&cacert, (const unsigned char *)test_ca_crt,
                strlen(test_ca_crt));
    if (ret != 0) {
        if (verbose != 0)
            printf("failed\n");

        x509_free(&clicert);
        return (ret);
    }

    if (verbose != 0)
        printf("passed\n  X.509 private key load: ");

    i = strlen(test_ca_key);
    j = strlen(test_ca_pwd);

    if ((ret = x509parse_key(&rsa,
                 (const unsigned char *)test_ca_key, i,
                 (const unsigned char *)test_ca_pwd, j)) != 0) {
        if (verbose != 0)
            printf("failed\n");

        x509_free(&cacert);
        x509_free(&clicert);
        return (ret);
    }

    if (verbose != 0)
        printf("passed\n  X.509 signature verify: ");

    ret = x509parse_verify(&clicert, &cacert, "Joe User", &i);
    if (ret != 0) {
        if (verbose != 0)
            printf("failed\n");

        x509_free(&cacert);
        x509_free(&clicert);
        rsa_free(&rsa);
        return (ret);
    }

    if (verbose != 0)
        printf("passed\n\n");

    x509_free(&cacert);
    x509_free(&clicert);
    rsa_free(&rsa);

    return (0);
}

#endif
