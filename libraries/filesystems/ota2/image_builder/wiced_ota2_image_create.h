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
 *  Exported API for Wiced OTA Image file creation code
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
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

/**
 * Create a Wiced OTA2 Image file suitable for OTA updating
 *
 * Reads a specification file for creating the OTA Image
 * creates an image file containing a Wiced OTA Image
 *
 * @param[in] spec_file : Pathname ot the OTA2 Image specification file
 * @param[in] out_file  : Filename of the Wiced OTA2 Image to be created
 * @param[in] verbose   : Verbose debug output
 *
 * @return 0 = success
 */
int create_wiced_ota2( const char* ota_spec_file, const char* ota2_image_file, int verbose );

#ifdef __cplusplus
} /*extern "C" */
#endif

