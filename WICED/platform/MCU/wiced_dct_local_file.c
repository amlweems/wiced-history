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
 * Manages DCT writing to external flash
 */
#include "string.h"
#include "wwd_assert.h"
#include "wiced_result.h"
#include "platform_dct.h"
#include "waf_platform.h"
#include "wiced_framework.h"
#include "wiced_dct_common.h"
#include "wiced_apps_common.h"
#include "elf.h"

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
 *               Function Declarations
 ******************************************************/

static wiced_result_t dct_init( void );
static int platform_write_dct( uint16_t data_start_offset, const void* data, uint16_t data_length, int8_t app_valid, void (*func)(void) );
static platform_dct_data_t* platform_get_dct( void );

/******************************************************
 *               Variables Definitions
 ******************************************************/
static const uint32_t DCT_section_offsets[ ] =
{
    [DCT_APP_SECTION]           = sizeof(platform_dct_data_t),
    [DCT_SECURITY_SECTION]      = OFFSETOF( platform_dct_data_t, security_credentials ),
    [DCT_MFG_INFO_SECTION]      = OFFSETOF( platform_dct_data_t, mfg_info ),
    [DCT_WIFI_CONFIG_SECTION]   = OFFSETOF( platform_dct_data_t, wifi_config ),
    [DCT_ETHERNET_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, ethernet_config ),
    [DCT_NETWORK_CONFIG_SECTION]  = OFFSETOF( platform_dct_data_t, network_config ),
#ifdef WICED_DCT_INCLUDE_BT_CONFIG
    [DCT_BT_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, bt_config ),
#endif
#ifdef WICED_DCT_INCLUDE_P2P_CONFIG
    [DCT_P2P_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, p2p_config ),
#endif
#ifdef OTA2_SUPPORT
    [DCT_OTA2_CONFIG_SECTION] = OFFSETOF( platform_dct_data_t, ota2_config ),
#endif
    [DCT_INTERNAL_SECTION]      = 0,
};

static int dct_inited = 0;

char *dct_memory = NULL;
long dct_size = 0;

#define DCT_FILENAME "DCT.bin"
#define MAX_DCT_LENGTH (64*1024)
#define MIN_DCT_LENGTH (sizeof(platform_dct_data_t))



/******************************************************
 *               Function Definitions
 ******************************************************/




/* TODO: Disable interrupts during function */
/* Note Function allocates a chunk of memory for the bootloader data on the stack - ensure the stack is big enough */
wiced_result_t wiced_dct_write( const void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size )
{
    int retval;

    if ( dct_inited == 0 )
    {
        dct_init( );
    }

    retval = platform_write_dct( DCT_section_offsets[section] + offset, info_ptr, size, 1, NULL );
    (void) info_ptr;
    (void) section;
    (void) offset;
    (void) size;
    return (retval == 0)? WICED_SUCCESS : WICED_ERROR;
}


wiced_result_t wiced_dct_read_with_copy( void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size )
{
    char* curr_dct  = (char*) wiced_dct_get_current_address( section );

    memcpy( info_ptr, &curr_dct[ offset ], size );

    return WICED_SUCCESS;
}

void* wiced_dct_get_current_address( dct_section_t section )
{
    return (void*)( (char*)platform_get_dct( )  + DCT_section_offsets[section] );
}




wiced_result_t wiced_dct_get_app_header_location( uint8_t app_id, image_location_t* app_header_location )
{
    if ( app_id >= DCT_MAX_APP_COUNT )
    {
        return WICED_ERROR;
    }
    return wiced_dct_read_with_copy( app_header_location, DCT_INTERNAL_SECTION, DCT_APP_LOCATION_OF(app_id), sizeof(image_location_t) );
}

wiced_result_t wiced_dct_set_app_header_location( uint8_t app_id, image_location_t* app_header_location )
{
    if ( app_id >= DCT_MAX_APP_COUNT )
    {
        return WICED_ERROR;
    }
    return wiced_dct_write( app_header_location, DCT_INTERNAL_SECTION, DCT_APP_LOCATION_OF(app_id), sizeof(image_location_t) );
}



static int expand_dct( FILE *dct_file, long newsize )
{
    int seek_result;
    /* file not long enough - expand it */
    seek_result = fseek( dct_file, SEEK_END, 0 );
    if ( seek_result != 0 )
    {
        printf("Couldn't seek to end of DCT file\n");
        return -2;
    }
    long file_pos = ftell( dct_file );
    char *zeros = (char*)calloc( 1, newsize - file_pos );
    if ( zeros == NULL )
    {
        printf( "Couldn't malloc space to expand DCT\n" );
        return -3;
    }

    size_t write_result = fwrite( zeros, 1, newsize - file_pos, dct_file );
    if ( write_result != newsize - file_pos )
    {
        printf( "Error expanding DCT\n" );
        free( zeros );
        return -4;
    }
    free( zeros );

    void* new_dct_memory = realloc( dct_memory, newsize );

    if ( new_dct_memory == NULL )
    {
        printf( "Error expanding DCT memory\n" );
        free( dct_memory );
        return -5;
    }

    dct_size = newsize;

    return 0;
}



static wiced_result_t dct_init( void )
{
    /* Open DCT file */
    FILE *dct_file = fopen( DCT_FILENAME, "r+b" );
    if ( dct_file == NULL )
    {
        printf("Creating DCT file \"%s\"\n",DCT_FILENAME);

        dct_file = fopen( DCT_FILENAME, "w+b" );
        if ( dct_file == NULL )
        {
            printf("Error opening DCT file \"%s\"\n",DCT_FILENAME);
            return WICED_ERROR;
        }
    }

    int seek_result;
    seek_result = fseek( dct_file, 0, SEEK_END );
    if ( seek_result != 0 )
    {
        printf("Couldn't seek to end of DCT file\n");
        fclose( dct_file );
        return -2;
    }
    dct_size = ftell( dct_file );

    if ( dct_size > MAX_DCT_LENGTH )
    {
        printf( "Error DCT file is too big\n" );
        fclose( dct_file );
        return WICED_ERROR;
    }

    if (dct_size < MIN_DCT_LENGTH )
    {
        int expand_result = expand_dct( dct_file, MIN_DCT_LENGTH );
        if ( expand_result != 0 )
        {
            fclose( dct_file );
            return expand_result;
        }
    }

    dct_memory = calloc_named_hideleak( "dct memory image", 1, dct_size );
    if ( dct_memory == NULL )
    {
        printf( "Could not malloc space for DCT memory image\n" );
        fclose( dct_file );
        return WICED_ERROR;
    }

    seek_result = fseek( dct_file, SEEK_SET, 0 );
    if ( seek_result != 0 )
    {
        printf("Couldn't seek to start of DCT file\n");
        fclose( dct_file );
        return -10;
    }
    size_t num_read = fread( dct_memory, 1, dct_size, dct_file );
    if ( dct_size != num_read )
    {
        printf( "Error - could not read all of DCT\n" );
        fclose( dct_file );
        return WICED_ERROR;
    }

    fclose( dct_file );

    dct_inited = 1;

    platform_dct_header_t *dct_hdr = (platform_dct_header_t*) dct_memory;

    if ( ( dct_hdr->write_incomplete != 0 ) ||
         ( dct_hdr->magic_number != BOOTLOADER_MAGIC_NUMBER ) )
    {
        /* No valid DCT! */
        /* Erase DCT and init it. */
        memset( dct_memory, 0x00, dct_size );

        platform_dct_header_t hdr =
        {
            .write_incomplete = 0,
            .app_valid = 1,
            .mfg_info_programmed = 0,
            .magic_number = BOOTLOADER_MAGIC_NUMBER,
            .load_app_func = NULL
        };

        int write_result;
        write_result = platform_write_dct( 0, &hdr, sizeof(hdr), 1, NULL );
        if ( write_result != 0 )
        {
            printf( "Error initialising blank DCT" );
            return WICED_ERROR;
        }
    }


    return WICED_SUCCESS;
}


static platform_dct_data_t* platform_get_dct( void )
{
    if ( dct_inited == 0 )
    {
        dct_init( );
    }

    return (platform_dct_data_t*) dct_memory;
}



/* TODO: Disable interrupts during function */
/* Note Function allocates a chunk of memory for the bootloader data on the stack - ensure the stack is big enough */
static int platform_write_dct( uint16_t data_start_offset, const void* data, uint16_t data_length, int8_t app_valid, void (*func)(void) )
{
    int seek_result;
    size_t write_result;

    if ( dct_inited == 0 )
    {
        dct_init( );
    }

    if ( data_length + data_start_offset > MAX_DCT_LENGTH )
    {
        printf( "Attempt to write beyond the maximum DCT length\n" );
        return -1;
    }

    /* Open DCT file */
    FILE *dct_file = fopen( DCT_FILENAME, "r+b" );
    if ( dct_file == NULL )
    {
        printf("Error opening DCT file \"%s\"\n",DCT_FILENAME);
        return -1;
    }

    if ( data_start_offset + data_length > dct_size )
    {
        int expand_result = expand_dct( dct_file, data_start_offset + data_length );
        if ( expand_result != 0 )
        {
            fclose( dct_file );
            return expand_result;
        }
    }

    seek_result = fseek( dct_file, SEEK_SET, data_start_offset );
    if ( seek_result != 0 )
    {
        printf( "Error seeking to start of DCT write" );
        fclose( dct_file );
        return -2;
    }

    write_result = fwrite( data, 1, data_length, dct_file );
    if ( write_result != data_length )
    {
        printf( "Error writing DCT\n" );
        fclose( dct_file );
        return -5;
    }

    seek_result = fseek( dct_file, SEEK_SET, 0 );
    if ( seek_result != 0 )
    {
        printf( "Error seeking to start of DCT" );
        fclose( dct_file );
        return -6;
    }

    size_t num_read = fread( dct_memory, 1, dct_size, dct_file );
    if ( num_read != dct_size )
    {
        printf( "Error re-reading DCT after write\n" );
        return -7;
    }

    fclose( dct_file );
    return 0;
}
