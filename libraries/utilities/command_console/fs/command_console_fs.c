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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "typedefs.h"
#include "command_console.h"
#include "command_console_fs.h"
#include "wiced_filesystem.h"
#include "wiced_time.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define LOCAL_BUFFER_SIZE   (64*1024)

#define MAX_PATH_LEN   (1024)

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
static void memdump(uint8_t* bptr, uint32_t len);

/******************************************************
 *               Variable Definitions
 ******************************************************/

static wiced_bool_t       current_filesystem_is_mounted = WICED_FALSE;
static wiced_filesystem_t current_filesystem_handle;
static char               current_working_directory[MAX_PATH_LEN] = "/";

/******************************************************
 *               Function Definitions
 ******************************************************/

int mount (int argc, char* argv[])
{

    const filesystem_list_t* curr_item = all_filesystem_devices;

    if ( argc == 2 )
    {
        if ( current_filesystem_is_mounted == WICED_TRUE )
        {
            /* Unmount existing filesystem */
            wiced_filesystem_unmount( &current_filesystem_handle );
            current_filesystem_is_mounted = WICED_FALSE;
        }

        while ( curr_item->device != NULL )
        {
            if ( strcmp ( argv[1], curr_item->name ) == 0 )
            {
                wiced_result_t result;

                /* Found requested device */
                result = wiced_filesystem_mount( curr_item->device, curr_item->type, &current_filesystem_handle, curr_item->name );
                if ( result != WICED_SUCCESS )
                {
                    printf( "Error mounting filesystem" );
                    return ERR_UNKNOWN;
                }
                current_filesystem_is_mounted = WICED_TRUE;
                return ERR_CMD_OK;
            }
            curr_item++;
        }

        /* Not found - print options */
        printf( "Filesystem %s not found\n", argv[1] );
    }
    printf( "mount <filesystem name>\n" );
    printf( "Valid filesystem names:\n" );
    curr_item = all_filesystem_devices;
    while ( curr_item->device != NULL )
    {
        printf( "%s\n", curr_item->name );
        curr_item++;
    }

    return ERR_CMD_OK; /* Return ok since a custom message has been printed already */
}

int unmount (int argc, char* argv[])
{
    if ( current_filesystem_is_mounted != WICED_TRUE )
    {
        printf( "Not currently mounted\n" );
        return ERR_UNKNOWN;
    }

    /* Unmount existing filesystem */
    wiced_filesystem_unmount( &current_filesystem_handle );
    current_filesystem_is_mounted = WICED_FALSE;

    return ERR_CMD_OK;
}

int getcwd (int argc, char* argv[])
{
    printf("%s\n", current_working_directory);

    return ERR_CMD_OK;
}

int mk_dir (int argc, char* argv[])
{
    wiced_result_t result;
    char* directory_name = argv[1];

    if ( current_filesystem_is_mounted != WICED_TRUE )
    {
        printf( "Filesystem not currently mounted\n" );
        return ERR_UNKNOWN;
    }

    result = wiced_filesystem_dir_create( &current_filesystem_handle, directory_name );

    if ( result != WICED_SUCCESS )
    {
       printf( "Unable to create %s.\n", directory_name );
       return ERR_UNKNOWN;
    }

    printf( "%s successfully created.\n", directory_name );

    return ERR_CMD_OK;
}

int change_dir (int argc, char* argv[])
{
    wiced_result_t result;
    wiced_dir_t    dir_handle;
    char*          new_path = argv[1];

    if ( current_filesystem_is_mounted != WICED_TRUE )
    {
        printf( "Filesystem not currently mounted\n" );
        return ERR_UNKNOWN;
    }

    /* Check path exists */
    result = wiced_filesystem_dir_open( &current_filesystem_handle, &dir_handle, new_path );
    if ( result != WICED_SUCCESS )
    {
        printf( "Cannot change to %s \n", new_path );
        return ERR_UNKNOWN;
    }

    (void) wiced_filesystem_dir_close( &dir_handle );

    if ( strlen(new_path) >= sizeof(current_working_directory) )
    {
        printf( "Path too long" );
        return ERR_UNKNOWN;
    }

    strlcpy( current_working_directory, new_path, sizeof(current_working_directory) );

    return ERR_CMD_OK;
}

int cat_file (int argc, char* argv[])
{
    wiced_result_t   result;
    uint64_t         data_length_read;
    wiced_file_t     file_handle;
    char*            file_name = argv[1];
    char*            local_buffer;

    if ( current_filesystem_is_mounted != WICED_TRUE )
    {
        printf( "Filesystem not currently mounted\n" );
        return ERR_UNKNOWN;
    }

    /* Open the source file.  */
    result = wiced_filesystem_file_open( &current_filesystem_handle, &file_handle, file_name, WICED_FILESYSTEM_OPEN_FOR_READ );
    if ( result != WICED_SUCCESS )
    {
        printf( "Failed to open file %s\n", file_name );
        return ERR_UNKNOWN;
    }

    local_buffer = (char*) malloc( LOCAL_BUFFER_SIZE );
    do
    {
        /* Read the file in blocks.  */
        result = wiced_filesystem_file_read( &file_handle, local_buffer, LOCAL_BUFFER_SIZE, &data_length_read );

        /* Dump content of the source file */
        memdump((uint8_t *) local_buffer, data_length_read);
    } while ( ( result == WICED_SUCCESS ) && ( data_length_read == LOCAL_BUFFER_SIZE ) ); /* Check if end of file.  */

    free( local_buffer );

    wiced_filesystem_file_close( &file_handle );

    return ERR_CMD_OK;
}

int ls_dir (int argc, char* argv[])
{
    char                      current_name[MAX_PATH_LEN];
    wiced_dir_t               dir_handle;
    wiced_dir_entry_details_t entry_info;
    wiced_dir_entry_type_t    entry_type;
    wiced_result_t            result;
    uint64_t                  total_size = 0;
    uint32_t                  dir_count = 0;
    uint32_t                  file_count = 0;

    if ( current_filesystem_is_mounted != WICED_TRUE )
    {
        printf( "Filesystem not currently mounted\n" );
        return ERR_UNKNOWN;
    }

    result = wiced_filesystem_dir_open( &current_filesystem_handle, &dir_handle, current_working_directory );
    if ( result != WICED_SUCCESS )
    {
        printf( "Unable to open directory %s\n", current_working_directory );
        return ERR_UNKNOWN;
    }

    while ( result == WICED_SUCCESS )
    {
        result = wiced_filesystem_dir_read( &dir_handle, current_name, MAX_PATH_LEN, &entry_type, &entry_info );

        if ( result == WICED_FILESYSTEM_FILENAME_BUFFER_TOO_SMALL )
        {
            /* Truncation is ok */
            result = WICED_SUCCESS;
        }

        if ( result == WICED_SUCCESS )
        {
            total_size += entry_info.size;
            if ( entry_type == WICED_FILESYSTEM_DIR )
            {
                dir_count++;
            }
            else if ( entry_type == WICED_FILESYSTEM_FILE )
            {
                file_count++;
            }

            if ( entry_info.attributes_available == WICED_TRUE )
            {
                printf("%c%c%c%c%c%c\t",
                       (entry_info.attributes & WICED_FILESYSTEM_ATTRIBUTE_ARCHIVE)   ? 'A' : '-',
                       (entry_info.attributes & WICED_FILESYSTEM_ATTRIBUTE_DIRECTORY) ? 'D' : '-',
                       (entry_info.attributes & WICED_FILESYSTEM_ATTRIBUTE_VOLUME)    ? 'V' : '-',
                       (entry_info.attributes & WICED_FILESYSTEM_ATTRIBUTE_SYSTEM)    ? 'S' : '-',
                       (entry_info.attributes & WICED_FILESYSTEM_ATTRIBUTE_HIDDEN)    ? 'H' : '-',
                       (entry_info.attributes & WICED_FILESYSTEM_ATTRIBUTE_READ_ONLY) ? 'R' : '-');
            }

            printf("%10llu\t", entry_info.size);

            if ( entry_info.date_time_available == WICED_TRUE )
            {
                wiced_iso8601_time_t iso8601_time;
                wiced_time_convert_utc_ms_to_iso8601( entry_info.date_time * 1000, &iso8601_time );
                printf("%27s\t", (char*)&iso8601_time );
            }
            printf("%s\n", current_name);
        }
    }

    result = wiced_filesystem_dir_close( &dir_handle );

    printf( "\n%ld Dir(s), %ld File(s)\n", dir_count, file_count );
    printf( "\n" );

    return ERR_CMD_OK;
}

static void memdump(uint8_t* bptr, uint32_t len)
{
    uint32_t i, j;

    for ( i = 0 ; i < len; i+=16 )
    {
        uint8_t* bptr_char = bptr;

        /* Print first byte address in line */
        printf( "0x%08X: ", (unsigned int)bptr );

        /* Print line data in hex */
        for ( j = 0; j < 16; j++ )
        {
            printf( "%02X ", *bptr++ );
        }

        /* print line data as chars */
        for ( j = 0; j < 16; j++ )
        {
            char chChar = *bptr_char++;

            /* Print only text characters */
            if ( (chChar < ' ') || (chChar > 'z') )
            {
                chChar = '.';
            }

            printf( "%c", chChar );
        }

        /* Next line data */
        printf( "\n" );
    }
}
