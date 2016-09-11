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
 *  A tool to create a Wiced OTA Update Image
 *
 *  The tool takes an argument for a file that contains information to build the Image.
 *  Currently, we are using all the data included in the LUT table.
 *  We don't build the LUT, just use it and add all the components.
 *
 *  NOTES:
 *  - FR_APP and OTA_APP are for different WICED code, not expected to be used for this upgrade
 *  - WiFi Firmware is expected to be part of the FILESYSTEM
 *
 *  Values can be decimal or hex (preceded by "0x").
 *
 *  FACTORY_RESET=
 *  MAJOR_VERSION=
 *  MINOR_VERSION=
 *  APPS_LUT_LOC=
 *  APPS_LUT_FILE=
 *  FR_APP_LOC=
 *  FR_APP_FILE=
 *  DCT_LOC=
 *  DCT_FILE=
 *  OTA_APP_LOC=
 *  OTA_APP_FILE=
 *  FILESYSTEM_LOC=
 *  FILESYSTEM_FILE=
 *  APPLICATION_0_LOC=
 *  APPLICATION_0_FILE=
 *  APPLICATION_1_LOC=
 *  APPLICATION_1_FILE=
 *  APPLICATION_2_LOC=
 *  APPLICATION_2_FILE=
 *
 *  example:
FACTORY_RESET=0x00
MAJOR_VERSION=0x00
MINOR_VERSION=0x00
APPS_LUT_LOC=0x0020c000
APPS_LUT_FILE=build\snip.over_the_air_2_example-BCM943907WAE_1.B0-debug\APPS.bin
FR_APP_LOC=0
FR_APP_FILE=
DCT_LOC=0x0020d000
DCT_FILE=build\snip.over_the_air_2_example-BCM943907WAE_1.B0-debug\DCT.bin
OTA_APP_LOC=0
OTA_APP_FILE=
FILESYSTEM_LOC=0x00216000
FILESYSTEM_FILE=build\snip.over_the_air_2_example-BCM943907WAE_1.B0-debug\filesystem.bin
WIFI_FIRMWARE_LOC=0
WIFI_FIRMWARE_FILE=
APPLICATION_0_LOC=0x00300000
APPLICATION_0_FILE=build\snip.over_the_air_2_example-BCM943907WAE_1.B0-debug\binary\snip.over_the_air_2_example-BCM943907WAE_1.B0-debug.stripped.elf
APPLICATION_1_LOC=0
APPLICATION_1_FILE=
APPLICATION_2_LOC=0
APPLICATION_2_FILE=

 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wiced_ota2_image_create.h"

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
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

int main (int argc, const char * argv[])
{
    int verbose = 0;
    const char *ota2_spec_file;
    const char *ota2_image_file;

    /* simple test, we need 2 args*/
    if ((argc < 3) || (argc > 5))
    {
        printf( "Usage: mk_wiced_ota32 description_file output_file <-v [0|1]>  argc:%d\n\n", argc );
        return -1;
    }

    ota2_spec_file = argv[1];
    ota2_image_file = argv[2];
    if ((argc == 5) && (strncmp(argv[3], "-v", 2) == 0) )
    {
        verbose = atoi(argv[4]);
    }

    return create_wiced_ota2( ota2_spec_file, ota2_image_file, verbose );
}
