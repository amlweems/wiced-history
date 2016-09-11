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

#include "tls_types.h"
#include "wiced_result.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

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
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/*****************************************************************************/
/** @addtogroup tls       TLS Security
 *  @ingroup ipcoms
 *
 * Security initialisation functions for TLS enabled connections (Transport Layer Security - successor to SSL Secure Sockets Layer )
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Initialises a simple TLS context handle
 *
 * @param[out] context : A pointer to a wiced_tls_context_t context object that will be initialised
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tls_init_context( wiced_tls_context_t* context, wiced_tls_identity_t* identity, const char* peer_cn );


/** Initialises a TLS identity using a supplied certificate and private key
 *
 * @param[out] identity    : A pointer to a wiced_tls_identity_t object that will be initialised
 * @param[in] private_key : The server private key in binary format
 * @param[in] certificate_data : The server x509 certificate in PEM or DER format
 * @param[in] certificate_length : The length of the certificate
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tls_init_identity( wiced_tls_identity_t* identity, const char* private_key, const uint8_t* certificate_data, uint32_t certificate_length );

/** DeiInitialises a TLS identity
 *
 * @param[in] identity    : A pointer to a wiced_tls_identity_t object that will be de-initialised
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tls_deinit_identity( wiced_tls_identity_t* tls_identity);

/** Initialise the trusted root CA certificates
 *
 *  Initialises the collection of trusted root CA certificates used to verify received certificates
 *
 * @param[in] trusted_ca_certificates : A chain of x509 certificates in base64 encoded string format
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tls_init_root_ca_certificates( const char* trusted_ca_certificates );


/** De-initialise the trusted root CA certificates
 *
 *  De-initialises the collection of trusted root CA certificates used to verify received certificates
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tls_deinit_root_ca_certificates( void );


/** De-initialise a previously inited simple or advanced context
 *
 * @param[in,out] context : a pointer to either a wiced_tls_context_t object
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tls_deinit_context( wiced_tls_context_t* context );


/** Reset a previously inited simple or advanced context
 *
 * @param[in,out] tls_context : a pointer to either a wiced_tls_context_t object
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_tls_reset_context( wiced_tls_context_t* tls_context );

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif
