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
 *
 */

#include "x509.h"
#include "tls_host_api.h"
#include "base64.h"
#include <wiced.h>

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define LINE_BUFFER_SZ      (64)
#define CERT_PEM_HEADER     "-----BEGIN CERTIFICATE-----\n"
#define CERT_PEM_FOOTER     "-----END CERTIFICATE-----\n"

/******************************************************
 *                   Enumerations
 ******************************************************/

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

/** Create x.509 ASN.1 DER tag
 *
 * @param tag : tag id
 * @param len : length tag value, @param val
 * @param val : tag value
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_tag( uint8_t tag, size_t len, void * val, size_t * total_size )
{
    unsigned char header[4];
    size_t len_len = 1;
    unsigned char * buffer = NULL;

    header[ 0 ] = ( unsigned char ) tag;
    header[ 1 ] = ( unsigned char ) ( 0xff & len );

    if ( len > 0x7f )
    {
        len_len = 2;
        header[ 1 ] = 0x81;
        header[ 2 ] = ( unsigned char ) ( 0xff & len );
    }
    if ( len > 0xff )
    {
        len_len = 3;
        header[ 1 ] = 0x82;
        header[ 2 ] = ( unsigned char ) (( 0xff00 & len ) >> 8 );
        header[ 3 ] = ( unsigned char ) ( 0xff & len );
    }

    buffer = ( unsigned char * ) tls_host_malloc( "x509enc", 1 + len_len + len );

    wiced_assert( "x509enc: tls_host_malloc NULL error in x509enc_set_tag", buffer != NULL );

    if ( buffer == NULL )
    {
        return NULL;
    }

    if ( total_size )
    {
        * total_size = 1 + len_len + len;
    }

    memcpy( buffer, header, 1 + len_len );
    memcpy( buffer + 1 + len_len, val, len );

    return ( buffer );
}

/** Create x.509 ASN.1 DER version tag
 *
 * @param ver : '2' for version 3
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated version tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */

/*
    version         [0]  EXPLICIT Version DEFAULT v1

    Version  ::=  INTEGER  {  v1(0), v2(1), v3(2)  }
*/
unsigned char * x509enc_set_version( int8_t ver, size_t * total_size )
{
    unsigned char * p0, * p1;
    size_t size;

    ver -= 1;

    p0 = x509enc_set_tag( ASN1_INTEGER, 1, ( unsigned char * ) &ver, &size );

    if ( p0 == NULL )
    {
        return NULL;
    }

    p1 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_CONTEXT_SPECIFIC | 0, size, p0, total_size );

    tls_host_free( ( void * ) p0 );
    p0 = NULL;

    if ( p1 == NULL )
    {
        return ( NULL );
    }

    return ( p1 );
}

/** Create x.509 ASN.1 DER serial number tag
 *
 * @param serial : serial number in big endian big integer number
 * @param len : size of serial number
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */

/*
    CertificateSerialNumber  ::=  INTEGER
*/
unsigned char * x509enc_set_serial_number( void * serial, size_t serial_size, size_t * total_size )
{
    unsigned char * p = NULL;

    p = x509enc_set_tag( ASN1_INTEGER, serial_size, (unsigned char *) serial, total_size );

    return ( p );
}

/** Create x.509 ASN.1 DER set alorithm tag
 *
 * @param algo : algorithm object id
 * @param len : length of algorithm object id
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */

/*
    signature            AlgorithmIdentifier

    AlgorithmIdentifier  ::=  SEQUENCE  {
        algorithm               OBJECT IDENTIFIER,
        parameters              ANY DEFINED BY algorithm OPTIONAL  }
*/

unsigned char * x509enc_set_algorithm_id( void * algo, size_t algo_size, size_t * total_size )
{
    unsigned char * p0, * p1, * p2, * p3;
    size_t s0, s1, s2;

    p0 = p1 = p2 = p3 = NULL;


    p0 = x509enc_set_tag( ASN1_OID, algo_size, ( unsigned char * ) algo, &s0 );

    if ( p0 == NULL )
    {
        goto final;
    }

    p1 = x509enc_set_tag( ASN1_NULL, 0, NULL, &s1 );

    if ( p1 == NULL )
    {
        goto final;
    }

    s2 = s0 + s1;

    p2 = tls_host_malloc( "x509enc", s2 );

    if ( p2 == NULL )
    {
        goto final;
    }

    memcpy( p2, p0, s0 );
    memcpy( p2 + s0, p1, s1 );

    p3 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SEQUENCE, s2, p2, total_size );

final:
    if ( p0 )
    {
        tls_host_free( ( void * ) p0 );
        p0 = NULL;
    }
    if ( p1 )
    {
        tls_host_free( ( void * ) p1 );
        p1 = NULL;
    }
    if ( p2 )
    {
        tls_host_free( ( void * ) p2 );
        p2 = NULL;
    }

    return ( p3 );
}

/** Create x.509 ASN.1 DER name object tag and value
 *
 * @param object_id : name object id
 * @param object_sz : size of name objec id
 * @param value_tag : value type tag
 * @param value : value data
 * @param value_sz : size of value data
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */

/*
    RelativeDistinguishedName ::=
        SET SIZE (1..MAX) OF AttributeTypeAndValue

    AttributeTypeAndValue ::= SEQUENCE {
        type     AttributeType,
        value    AttributeValue }
*/
unsigned char * x509enc_set_name_type_value( void * object_id, size_t object_sz,
                                             uint8_t value_tag, void * value, size_t value_sz,
                                             size_t * total_size )
{
    unsigned char * p0, * p1, * p2, * p3, * p4;
    size_t s0, s1, s2, s3;

    p0 = p1 = p2 = p3 = p4 = NULL;


    p0 = x509enc_set_tag( ASN1_OID, object_sz, ( unsigned char * ) object_id, &s0 );

    if ( p0 == NULL )
    {
        goto final;
    }

    p1 = x509enc_set_tag( value_tag, value_sz, ( unsigned char * ) value, &s1 );

    if ( p1 == NULL )
    {
        goto final;
    }

    s2 = s0 + s1;

    p2 = tls_host_malloc( "x509enc", s2 );

    if ( p2 == NULL )
    {
        goto final;
    }

    memcpy( p2, p0, s0 );
    memcpy( p2 + s0, p1, s1 );

    p3 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SEQUENCE, s2, p2, &s3 );

    p4 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SET, s3, p3, total_size );

final:
    if ( p0 )
    {
        tls_host_free( ( void * ) p0 );
        p0 = NULL;
    }
    if ( p1 )
    {
        tls_host_free( ( void * ) p1 );
        p1 = NULL;
    }
    if ( p2 )
    {
        tls_host_free( ( void * ) p2 );
        p2 = NULL;
    }
    if ( p3 )
    {
        tls_host_free( ( void * ) p3 );
        p3 = NULL;
    }

    return ( p4 );
}

/** Create x.509 ASN.1 DER name tag
 *
 * @param C : country value
 * @param cz_C : size of country value
 * @param ST : state value
 * @param cz_ST : size of state value
 * @param L : locality value
 * @param cz_L : size of locality value
 * @param O : organization value
 * @param cz_O : size of organization value
 * @param OU : organization unit value
 * @param cz_OU : size of organization unit value
 * @param CN : common name value
 * @param cz_CN : size of common name value
 * @param E : email value
 * @param cz_E : size of email value
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */

/*
    Name ::= CHOICE { -- only one possibility for now --
        rdnSequence  RDNSequence }

    RDNSequence ::= SEQUENCE OF RelativeDistinguishedName
*/
unsigned char * x509enc_set_name( void * C, size_t sz_C,
                                  void * ST, size_t sz_ST,
                                  void * L, size_t sz_L,
                                  void * O, size_t sz_O,
                                  void * OU, size_t sz_OU,
                                  void * CN, size_t sz_CN,
                                  void * E, size_t sz_E,
                                  size_t * total_size )
{
    unsigned char * pC, * pST, * pL, * pO, * pOU, * pCN, * pE, * pNAME, * p;
    size_t sC, sST, sL, sO, sOU, sCN, sE, sNAME;

    pC = pST = pL = pO = pOU = pCN = pE = pNAME = p = NULL;
    sC = sST = sL = sO = sOU = sCN = sE = sNAME = 0;

    if ( C )
    {
        pC = x509enc_set_name_type_value( OID_COUNTRY, strlen( OID_COUNTRY ), ASN1_PRINTABLE_STRING, C, sz_C, &sC );
        if ( pC == NULL )
        {
            goto final;
        }
    }
    if ( ST )
    {
        pST = x509enc_set_name_type_value( OID_STATE, strlen( OID_STATE ), ASN1_UTF8_STRING, ST, sz_ST, &sST );
        if ( pST == NULL )
        {
            goto final;
        }
    }
    if ( L )
    {
        pL = x509enc_set_name_type_value( OID_LOCALITY, strlen( OID_LOCALITY ), ASN1_UTF8_STRING, L, sz_L, &sL );
        if ( pL == NULL )
        {
            goto final;
        }
    }
    if ( O )
    {
        pO = x509enc_set_name_type_value( OID_ORGANIZATION, strlen( OID_ORGANIZATION ), ASN1_UTF8_STRING, O, sz_O, &sO );
        if ( pO == NULL )
        {
            goto final;
        }
    }
    if ( OU )
    {
        pOU = x509enc_set_name_type_value( OID_ORG_UNIT, strlen( OID_ORG_UNIT ), ASN1_UTF8_STRING, OU, sz_OU, &sOU );
        if ( pOU == NULL )
        {
            goto final;
        }
    }
    if ( CN )
    {
        pCN = x509enc_set_name_type_value( OID_CN, strlen( OID_CN ), ASN1_UTF8_STRING, CN, sz_CN, &sCN );
        if ( pCN == NULL )
        {
            goto final;
        }
    }
    if ( E )
    {
        pE = x509enc_set_name_type_value( OID_PKCS9_EMAIL, strlen( OID_PKCS9_EMAIL ), ASN1_IA5_STRING, E, sz_E, &sE );
        if ( pC == NULL )
        {
            goto final;
        }
    }

    sNAME = sC + sST + sL + sO + sOU + sCN + sE;

    pNAME = tls_host_malloc( "x509enc", sNAME );

    if ( pNAME == NULL )
    {
        goto final;
    }

    p = pNAME;

    if ( sC )
    {
        memcpy( p, pC, sC );
        p += sC;
    }
    if ( sST )
    {
        memcpy( p, pST, sST );
        p += sST;
    }
    if ( sL )
    {
        memcpy( p, pL, sL );
        p += sL;
    }
    if ( sO )
    {
        memcpy( p, pO, sO );
        p += sO;
    }
    if ( sOU )
    {
        memcpy( p, pOU, sOU );
        p += sOU;
    }
    if ( sCN )
    {
        memcpy( p, pCN, sCN );
        p += sCN;
    }
    if ( sE )
    {
        memcpy( p, pE, sE );
    }

    p = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SEQUENCE, sNAME, pNAME, total_size );

final:
    if ( pC )
    {
        tls_host_free( ( void * ) pC );
        pC = NULL;
    }
    if ( pST )
    {
        tls_host_free( ( void * ) pST );
        pST = NULL;
    }
    if ( pL )
    {
        tls_host_free( ( void * ) pL );
        pL = NULL;
    }
    if ( pO )
    {
        tls_host_free( ( void * ) pO );
        pO = NULL;
    }
    if ( pOU )
    {
        tls_host_free( ( void * ) pOU );
        pOU = NULL;
    }
    if ( pCN )
    {
        tls_host_free( ( void * ) pCN );
        pCN = NULL;
    }
    if ( pE )
    {
        tls_host_free( ( void * ) pE );
        pE = NULL;
    }
    if ( pNAME )
    {
        tls_host_free( ( void * ) pNAME );
        pE = NULL;
    }

    return ( p );
}

/** Create x.509 ASN.1 DER date and time tag
 *
 * @param y : year
 * @param m : month
 * @param d : day
 * @param h : hour
 * @param min : minute
 * @param sec : second
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */

/*
    Time ::= CHOICE {
        utcTime        UTCTime,
        generalTime    GeneralizedTime }
*/

unsigned char * x509enc_set_time( uint32_t y, uint8_t m, uint8_t d, uint8_t h, uint8_t min, uint8_t sec, size_t * total_size )
{
    char buffer[30];
    uint8_t tag = ASN1_GENERALISED_TIME;

    if ( m < 1 || m > 12 )
    {
        return ( NULL );
    }
    if ( d < 1 || d > 31 )
    {
        return ( NULL );
    }
    if ( h > 24 )
    {
        return ( NULL );
    }
    if ( min > 60 )
    {
        return ( NULL );
    }
    if ( sec > 60 )
    {
        return ( NULL );
    }

    if ( ( y >= 1950 ) && ( y < 2050 ) )
    {
        y -= 1900;
        if ( y >= 100 )
        {
            y -= 100;
        }
        tag = ASN1_UTC_TIME;
        sprintf( buffer, "%02u%02u%02u%02u%02u%02uZ", (unsigned int) y, m, d, h, min, sec );
    }
    else
    {
        sprintf( buffer, "%04u%02u%02u%02u%02u%02uZ", (unsigned int) y, m, d, h, min, sec );
    }

    printf("\nYEAR: %u\n",(unsigned int) y);

    return ( x509enc_set_tag( tag, strlen( buffer ), buffer, total_size ) );
}

/** Create x.509 ASN.1 DER validity tag
 *
 * @param s_y : not before year
 * @param s_m : not before month
 * @param s_d : not before day
 * @param s_h : not before hour
 * @param s_min : not before minute
 * @param s_sec : not before second
 * @param e_y : not after year
 * @param e_m : not after month
 * @param e_d : not after day
 * @param e_h : not after hour
 * @param e_min : not after minute
 * @param e_sec : not after second
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */

/*
    Validity ::= SEQUENCE {
        notBefore      Time,
        notAfter       Time }
*/
unsigned char * x509enc_set_validity( uint32_t s_y, uint8_t s_m, uint8_t s_d, uint8_t s_h, uint8_t s_min, uint8_t s_sec,
                                      uint32_t e_y, uint8_t e_m, uint8_t e_d, uint8_t e_h, uint8_t e_min, uint8_t e_sec,
                                      size_t * total_size )
{
    unsigned char * p_s, * p_e, * p0, * p1;
    size_t sz_s, sz_e, sz;

    p_s = p_e = p0 = p1 = NULL;
    sz_s = sz_e = sz = 0;

    p_s = x509enc_set_time( s_y, s_m, s_d, s_h, s_min, s_sec, &sz_s );

    if ( p_s == NULL )
    {
        goto final;
    }

    p_e = x509enc_set_time( e_y, e_m, e_d, e_h, e_min, e_sec, &sz_e );

    if ( p_e == NULL )
    {
        goto final;
    }

    sz = sz_s + sz_e;

    p0 = tls_host_malloc( "x509enc", sz );

    if ( p0 == NULL )
    {
        goto final;
    }

    memcpy( p0, p_s, sz_s );
    memcpy( p0 + sz_s, p_e, sz_e );

    p1 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SEQUENCE, sz, p0, total_size );

final:
    if ( p_s )
    {
        tls_host_free( ( void * ) p_s );
        p_s = NULL;
    }
    if ( p_e )
    {
        tls_host_free( ( void * ) p_e );
        p_e = NULL;
    }
    if ( p0 )
    {
        tls_host_free( ( void * ) p0 );
        p0 = NULL;
    }

    return ( p1 );
}

/** Create x.509 ASN.1 DER public key tag
 *
 * @param pubkey : public key modulus
 * @param pubkey_size : size of public key modulus
 * @param pubkey : public key exponent
 * @param pubkey_size : size of public key exponent
 * @param sha1hash : calulated DER format public key's sha1 hash
 *                   which is used for key identifier
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_set_pubkey( void * pubkey, size_t pubkey_size,
                                    void * pubexp, size_t pubexp_size,
                                    unsigned char sha1hash[20], size_t * total_size )
{
    unsigned char * p0, * p1, * p2, * p3;
    size_t s0, s1, s2;

    p0 = p1 = p2 = p3 = NULL;
    s0 = s1 = s2 = 0;

    p0 = x509enc_set_tag( ASN1_INTEGER, pubkey_size, pubkey, &s0 );
    if ( p0 == NULL )
    {
        goto final;
    }
    p1 = x509enc_set_tag( ASN1_INTEGER, pubexp_size, pubexp, &s1 );
    if ( p1 == NULL )
    {
        goto final;
    }

    s2 = s0 + s1;

    p2 = tls_host_malloc( "x509enc", s2 );
    if ( p2 == NULL )
    {
        goto final;
    }

    memcpy( p2, p0, s0 );
    memcpy( p2 + s0, p1, s1 );

    p3 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SEQUENCE, s2, p2, total_size );

    if ( p3 && sha1hash )
    {
        sha1( ( const unsigned char * ) p3, *total_size, sha1hash );
    }

final:
    if ( p0 )
    {
        tls_host_free( ( void * ) p0 );
        p0 = NULL;
    }
    if ( p1 )
    {
        tls_host_free( ( void * ) p1 );
        p1 = NULL;
    }
    if ( p2 )
    {
        tls_host_free( ( void * ) p2 );
        p2 = NULL;
    }

    return ( p3 );
}

/** Create x.509 ASN.1 DER public key information tag
 *
 * @param algo : algorithm id of public key
 * @param len : length of algorithm object id
 * @param pubkey : public key modulus
 * @param pubkey_size : size of public key modulus
 * @param pubkey : public key exponent
 * @param pubkey_size : size of public key exponent
 * @param sha1hash : calulated DER format public key's sha1 hash
 *                   which is used for key identifier
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */

/*
    SubjectPublicKeyInfo  ::=  SEQUENCE  {
        algorithm            AlgorithmIdentifier,
        subjectPublicKey     BIT STRING  }
*/
unsigned char * x509enc_set_subject_pubkey_info( void * algo, size_t algo_size,
                                                 void * pubkey, size_t pubkey_size,
                                                 void * pubexp, size_t pubexp_size,
                                                 unsigned char sha1hash[20], size_t * total_size )
{
    unsigned char * p0, * p1, * p2, * p3, * p4, * p5;
    size_t s0, s1, s2, s3, s4;

    p0 = p1 = p2 = p3 = p4 = p5 = NULL;
    s0 = s1 = s2 = s3 = s4 = 0;

    p0 = x509enc_set_algorithm_id( algo, algo_size, &s0 );
    if ( p0 == NULL )
    {
        goto final;
    }

    p1 = x509enc_set_pubkey( pubkey, pubkey_size, pubexp, pubexp_size, sha1hash, &s1 );
    if ( p1 == NULL )
    {
        goto final;
    }

    s2 = s1 + 1;

    p2 = tls_host_malloc( "x509enc", s2 );
    if ( p2 == NULL )
    {
        goto final;
    }
    p2[0] = 0;
    memcpy( p2 + 1, p1, s1 );

    p3 = x509enc_set_tag( ASN1_BIT_STRING, s2, p2, &s3 );
    if ( p2 == NULL )
    {
        goto final;
    }

    s4 = s0 + s3;

    p4 = tls_host_malloc( "x509enc", s4 );
    if ( p4 == NULL )
    {
        goto final;
    }

    memcpy( p4, p0, s0 );
    memcpy( p4 + s0, p3, s3 );

    p5 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SEQUENCE, s4, p4, total_size );

final:
    if ( p0 )
    {
        tls_host_free( ( void * ) p0 );
        p0 = NULL;
    }
    if ( p1 )
    {
        tls_host_free( ( void * ) p1 );
        p1 = NULL;
    }
    if ( p2 )
    {
        tls_host_free( ( void * ) p2 );
        p2 = NULL;
    }
    if ( p3 )
    {
        tls_host_free( ( void * ) p3 );
        p3 = NULL;
    }
    if ( p4 )
    {
        tls_host_free( ( void * ) p4 );
        p4 = NULL;
    }

    return ( p5 );
}

/** Create x.509 ASN.1 DER subject key id tag for x.509 extension v3
 *
 * @param key_sha1hash : sha1hash of subject key
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */

/*
    NOTE: based on reverse engineering of OpenSSL generated extensions
*/
unsigned char * x509enc_set_subject_key_id( unsigned char key_sha1hash[20], size_t * total_size )
{
    unsigned char * p0, * p1, * p2, * p3, * p4;
    size_t s0, s1, s2, s3;

    p0 = p1 = p2 = p3 = p4 = NULL;
    s0 = s1 = s2 = s3 = 0;

    p0 = x509enc_set_tag( ASN1_OID, strlen( OID_SUB_KEY_ID ), OID_SUB_KEY_ID, &s0 );
    if ( p0 == NULL )
    {
        goto final;
    }

    p1 = x509enc_set_tag( ASN1_OCTET_STRING, 20, key_sha1hash, &s1 );
    if ( p1 == NULL )
    {
        goto final;
    }

    p2 = x509enc_set_tag( ASN1_OCTET_STRING, s1, p1, &s2 );
    if ( p2 == NULL )
    {
        goto final;
    }

    s3 = s0 + s2;

    p3 = tls_host_malloc( "x509enc", s3 );
    if ( p3 == NULL )
    {
        goto final;
    }

    memcpy( p3, p0, s0 );
    memcpy( p3 + s0, p2, s2 );
    p4 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SEQUENCE, s3, p3, total_size );

final:
    if ( p0 )
    {
        tls_host_free( ( void * ) p0 );
        p0 = NULL;
    }
    if ( p1 )
    {
        tls_host_free( ( void * ) p1 );
        p1 = NULL;
    }
    if ( p2 )
    {
        tls_host_free( ( void * ) p2 );
        p2 = NULL;
    }
    if ( p3 )
    {
        tls_host_free( ( void * ) p3 );
        p3 = NULL;
    }

    return ( p4 );
}

/** Create x.509 ASN.1 DER authority key id tag for x.509 extension v3
 *
 * @param key_sha1hash : sha1hash of authority key
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */

/*
    NOTE: based on reverse engineering of OpenSSL generated extensions
*/
unsigned char * x509enc_set_authority_key_id( unsigned char key_sha1hash[20], size_t * total_size )
{
    unsigned char * p0, * p1, * p2, * p3, * p4, * p5;
    size_t s0, s1, s2, s3, s4;

    p0 = p1 = p2 = p3 = p4 = p5 = NULL;
    s0 = s1 = s2 = s3 = s4 = 0;

    p0 = x509enc_set_tag( ASN1_OID, strlen( OID_AUTH_KEY_ID ), OID_AUTH_KEY_ID, &s0 );
    if ( p0 == NULL )
    {
        goto final;
    }

    p1 = x509enc_set_tag( ASN1_CONTEXT_SPECIFIC, 20, key_sha1hash, &s1 );
    if ( p1 == NULL )
    {
        goto final;
    }

    p2 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SEQUENCE, s1, p1, &s2 );
    if ( p2 == NULL )
    {
        goto final;
    }

    p3 = x509enc_set_tag( ASN1_OCTET_STRING, s2, p2, &s3 );
    if ( p2 == NULL )
    {
        goto final;
    }

    s4 = s0 + s3;

    p4 = tls_host_malloc( "x509enc", s4 );
    if ( p3 == NULL )
    {
        goto final;
    }

    memcpy( p4, p0, s0 );
    memcpy( p4 + s0, p3, s3 );

    p5 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SEQUENCE, s4, p4, total_size );

final:
    if ( p0 )
    {
        tls_host_free( ( void * ) p0 );
        p0 = NULL;
    }
    if ( p1 )
    {
        tls_host_free( ( void * ) p1 );
        p1 = NULL;
    }
    if ( p2 )
    {
        tls_host_free( ( void * ) p2 );
        p2 = NULL;
    }
    if ( p3 )
    {
        tls_host_free( ( void * ) p3 );
        p3 = NULL;
    }
    if ( p4 )
    {
        tls_host_free( ( void * ) p4 );
        p4 = NULL;
    }

    return ( p5 );
}

/** Create x.509 ASN.1 DER CA value in basic constraints for x.509 extension v3
 *
 * @param ca : ca value. 0 for FALSE or TRUE
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */

/*
    based on reverse engineering of OpenSSL generated extensions
*/

unsigned char * x509enc_set_basic_constraints( uint8_t ca, size_t * total_size )
{
    unsigned char * p0, * p1, * p2, * p3, * p4, * p5;
    size_t s0, s1, s2, s3, s4;

    p0 = p1 = p2 = p3 = p4 = p5 = NULL;
    s0 = s1 = s2 = s3 = s4 = 0;

    if ( ca )
    {
        ca = 0xff;
    }

    p0 = x509enc_set_tag( ASN1_OID, strlen( OID_BASIC_CONSTRAINTS ), OID_BASIC_CONSTRAINTS, &s0 );
    if ( p0 == NULL )
    {
        goto final;
    }

    p1 = x509enc_set_tag( ASN1_BOOLEAN, 1, &ca, &s1 );
    if ( p1 == NULL )
    {
        goto final;
    }

    p2 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SEQUENCE, s1, p1, &s2 );
    if ( p2 == NULL )
    {
        goto final;
    }

    p3 = x509enc_set_tag( ASN1_OCTET_STRING, s2, p2, &s3 );
    if ( p2 == NULL )
    {
        goto final;
    }

    s4 = s0 + s3;

    p4 = tls_host_malloc( "x509enc", s4 );
    if ( p3 == NULL )
    {
        goto final;
    }

    memcpy( p4, p0, s0 );
    memcpy( p4 + s0, p3, s3 );

    p5 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SEQUENCE, s4, p4, total_size );

final:
    if ( p0 )
    {
        tls_host_free( ( void * ) p0 );
        p0 = NULL;
    }
    if ( p1 )
    {
        tls_host_free( ( void * ) p1 );
        p1 = NULL;
    }
    if ( p2 )
    {
        tls_host_free( ( void * ) p2 );
        p2 = NULL;
    }
    if ( p3 )
    {
        tls_host_free( ( void * ) p3 );
        p3 = NULL;
    }
    if ( p4 )
    {
        tls_host_free( ( void * ) p4 );
        p4 = NULL;
    }

    return ( p5 );
}

/** Create x.509 ASN.1 DER x.509 extension v3 tag
 *
 * @param subkey_sha1hash : sha1hash of subject key
 * @param authkey_sha1hash : sha1hash of authority key
 * @param ca : ca value. 0 for FALSE or TRUE
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */

/*
    NOTE: based on reverse engineering of OpenSSL generated extensions
*/
unsigned char * x509enc_set_extensions( unsigned char subkey_sha1hash[20], unsigned char authkey_sha1hash[20],
                                        uint8_t ca, size_t * total_size )
{
    unsigned char * p0, * p1, * p2, * p3, * p4, * p5;
    size_t s0, s1, s2, s3, s4;

    p0 = p1 = p2 = p3 = p4 = p5 = NULL;
    s0 = s1 = s2 = s3 = s4 = 0;

    p0 = x509enc_set_subject_key_id( subkey_sha1hash, &s0 );
    if ( p0 == NULL )
    {
        goto final;
    }

    p1 = x509enc_set_authority_key_id( authkey_sha1hash, &s1 );
    if ( p1 == NULL )
    {
        goto final;
    }

    p2 = x509enc_set_basic_constraints( ca, &s2 );
    if ( p2 == NULL )
    {
        goto final;
    }

    s3 = s0 + s1 + s2;

    p3 = tls_host_malloc( "x509enc", s3 );
    if ( p3 == NULL )
    {
        goto final;
    }

    memcpy( p3, p0, s0 );
    memcpy( p3 + s0, p1, s1 );
    memcpy( p3 + s0 + s1, p2, s2 );

    p4 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SEQUENCE, s3, p3, &s4 );

    p5 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_CONTEXT_SPECIFIC | 3, s4, p4, total_size );

final:
    if ( p0 )
    {
        tls_host_free( ( void * ) p0 );
        p0 = NULL;
    }
    if ( p1 )
    {
        tls_host_free( ( void * ) p1 );
        p1 = NULL;
    }
    if ( p2 )
    {
        tls_host_free( ( void * ) p2 );
        p2 = NULL;
    }
    if ( p3 )
    {
        tls_host_free( ( void * ) p3 );
        p3 = NULL;
    }
    if ( p4 )
    {
        tls_host_free( ( void * ) p4 );
        p4 = NULL;
    }

    return ( p5 );
}

/** Create x.509 ASN.1 DER tag
 *
 * @param tag : tag id
 * @param len : length tag value, @param val
 * @param val : tag value
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */

unsigned char * x509enc_set_signature( unsigned char * signature, size_t signature_size, size_t * total_size )
{
    unsigned char * p0, * p1;
    size_t s0 = signature_size + 1;

    p0 = p1 = NULL;

    p0 = tls_host_malloc( "x509enc", s0 );
    if ( p0 == NULL )
    {
        goto final;
    }

    *p0 = 0;
    memcpy( p0 + 1, signature, signature_size );
    p1 = x509enc_set_tag( ASN1_BIT_STRING, s0, p0, total_size );

final:
    if ( p0 )
    {
        tls_host_free( ( void * ) p0 );
        p0 = NULL;
    }

    return ( p1 );
}

/** Create x.509 ASN.1 DER tbs certificate tag
 *
 * @param ver : version in DER format
 * @param sz_ver : size of ver
 * @param sn : serial number in DER format
 * @param sz_sn : size of sn
 * @param issuer : issuer name in DER format
 * @param sz_issuer : size of issuer
 * @param validity : validity in DER format
 * @param sz_validity : size of validity
 * @param subject : subject name in DER format
 * @param sz_subject : size of subject
 * @param pubkey_info : subject public key info in DER format
 * @param sz_sn : size of pubkey_info
 * @param ext : x.509 ext v3 in DER format
 * @param sz_sn : size of ext
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_gen_tbs_cert( void * ver, size_t sz_ver,
                                      void * sn, size_t sz_sn,
                                      void * algo, size_t sz_algo,
                                      void * issuer, size_t sz_issuer,
                                      void * validity, size_t sz_validity,
                                      void * subject, size_t sz_subject,
                                      void * pubkey_info, size_t sz_pubkey_info,
                                      void * ext, size_t sz_ext,
                                      size_t * total_size )
{
    unsigned char * p0, * p1, * p;
    size_t s = sz_ver + sz_sn + sz_algo + sz_issuer + sz_validity + sz_subject + sz_pubkey_info + sz_ext;

    p0 = tls_host_malloc ( "x509enc", s );
    if ( p0 == NULL )
    {
        goto final;
    }

    p = p0;
    memcpy( p, ver, sz_ver );
    p += sz_ver;
    memcpy( p, sn, sz_sn );
    p += sz_sn;
    memcpy( p, algo, sz_algo );
    p += sz_algo;
    memcpy( p, issuer, sz_issuer );
    p += sz_issuer;
    memcpy( p, validity, sz_validity );
    p += sz_validity;
    memcpy( p, subject, sz_subject );
    p += sz_subject;
    memcpy( p, pubkey_info, sz_pubkey_info );
    p += sz_pubkey_info;
    memcpy( p, ext, sz_ext );

    p1 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SEQUENCE, s, p0, total_size );

final:
    if ( p0 )
    {
        tls_host_free( ( void * ) p0 );
        p0 = NULL;
    }

    return ( p1 );
}

/** Create x.509 ASN.1 DER certificate tag
 *
 * @param tbs_cert : tbs certificate in DER format
 * @param sz_tbs_cert : size of tbs_cert
 * @param algo : algoritm
 * @param sz_tbs_cert : size of tbs_cert
 * @param algo : algorithm id of signature
 * @param len : length of algorithm object id
 * @param signature : signature value
 * @param len : size of signature
 * @param total_size : total size of return pointer data
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_gen_certificate( void * tbs_cert, size_t sz_tbs_cert,
                                         void * algo, size_t sz_algo,
                                         void * signature, size_t sz_signature,
                                         size_t * total_size )
{
    unsigned char * p0, * p1, * p;
    size_t s = sz_tbs_cert + sz_algo + sz_signature;

    p0 = tls_host_malloc ( "x509enc", s );
    if ( p0 == NULL )
    {
        goto final;
    }

    p = p0;
    memcpy( p, tbs_cert, sz_tbs_cert );
    p += sz_tbs_cert;
    memcpy( p, algo, sz_algo );
    p += sz_algo;
    memcpy( p, signature, sz_signature );

    p1 = x509enc_set_tag( ASN1_CONSTRUCTED | ASN1_SEQUENCE, s, p0, total_size );

final:
    if ( p0 )
    {
        tls_host_free( ( void * ) p0 );
        p0 = NULL;
    }

    return ( p1 );
}

/** convert x.509 cert DER to PEM
 *
 * @param der : DER format certificate
 * @param sz_der : size of der
 * @param conv_size : result of converted size of PEM format
 *
 * @return : whole generated tag and value data.
 *           if error, NULL returns
 *           This must be tls_host_free by CALLER
 */
unsigned char * x509enc_der_to_pem( void *der, size_t sz_der, size_t * conv_size )
{
    unsigned char * p0, * p1;
    char * p;
    size_t s0, s1, sz_conv, lines;
    int i;
    unsigned char line_buf[ LINE_BUFFER_SZ + 1 ];

    p0 = p1 = 0;
    p = 0;

    s0 = sz_der * 1.4;
    p0 = tls_host_malloc( "x509enc", s0 );
    if ( p0 == NULL )
    {
        goto final;
    }

    sz_conv = base64_encode( der, sz_der, p0, s0, BASE64_STANDARD );

    lines = ( sz_conv / LINE_BUFFER_SZ ) + 1;

    s1 = strlen( CERT_PEM_HEADER ) + sz_conv + lines + strlen( CERT_PEM_FOOTER ) + 1;

    p1 = tls_host_malloc( "x509enc", s1 );
    if ( p1 == NULL )
    {
        goto final;
    }

    memset ( p1, 0, s1 );

    p = ( char * ) p1;
    sprintf( p, "%s", CERT_PEM_HEADER );
    p += strlen( CERT_PEM_HEADER );

    for ( i = 0; i < sz_conv; i += LINE_BUFFER_SZ )
    {
        size_t to_copy_sz = LINE_BUFFER_SZ;
        if ( ( sz_conv - i ) < LINE_BUFFER_SZ )
        {
            to_copy_sz = sz_conv - i;
        }
        memset( line_buf, 0, LINE_BUFFER_SZ + 1 );
        memcpy( line_buf, &p0[i], to_copy_sz );
        sprintf( p, "%s\n", line_buf );
        p += strlen( p );
    }
    sprintf( p, "%s", CERT_PEM_FOOTER );
    p += strlen( CERT_PEM_FOOTER );

    *conv_size = strlen( ( char * ) p1 );

final:
    if ( p0 )
    {
        tls_host_free( ( void * ) p0 );
        p0 = NULL;
    }

    return ( p1 );
}
