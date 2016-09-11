/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include "spi_flash.h"
#include "spi_flash_internal.h"
#include "spi_flash_platform_interface.h"
#include <string.h> /* for NULL */

int sflash_read_ID( const sflash_handle_t* const handle, void* const data_addr )
{
    return generic_sflash_command( handle, SFLASH_READ_JEDEC_ID, 0, NULL, 3, NULL, data_addr );
}

int sflash_write_enable( const sflash_handle_t* const handle )
{
    if ( handle->write_allowed == SFLASH_WRITE_ALLOWED )
    {
        /* Send write-enable command */
        int status = generic_sflash_command( handle, SFLASH_WRITE_ENABLE, 0, NULL, 0, NULL, NULL );
        if ( status != 0 )
        {
            return status;
        }

        /* Check status register */
        unsigned char status_register;
        if ( 0 != ( status = sflash_read_status_register( handle, &status_register ) ) )
        {
            return status;
        }

        /* Check if Block protect bits are set */
        if ( status_register != SFLASH_STATUS_REGISTER_WRITE_ENABLED )
        {
            /* Disable protection for all blocks */
            if (0 != ( status = sflash_write_status_register( handle, 0 ) ) )
            {
                return status;
            }

            /* Re-Enable writing */
            if (0 != ( status = generic_sflash_command( handle, SFLASH_WRITE_ENABLE, 0, NULL, 0, NULL, NULL ) ) )
            {
                return status;
            }
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

int sflash_chip_erase( const sflash_handle_t* const handle )
{
    int status = sflash_write_enable( handle );
    if ( status != 0 )
    {
        return status;
    }
    return generic_sflash_command( handle, SFLASH_CHIP_ERASE1, 0, NULL, 0, NULL, NULL );
}

#include "wwd_assert.h"

int sflash_sector_erase ( const sflash_handle_t* const handle, unsigned long device_address )
{

    char device_address_array[3] =  { ( ( device_address & 0x00FF0000 ) >> 16 ),
                                      ( ( device_address & 0x0000FF00 ) >>  8 ),
                                      ( ( device_address & 0x000000FF ) >>  0 ) };

    int retval;
    int status = sflash_write_enable( handle );
    if ( status != 0 )
    {
        return status;
    }
    retval = generic_sflash_command( handle, SFLASH_SECTOR_ERASE, 3, device_address_array, 0, NULL, NULL );
    wiced_assert("error", retval == 0);
    return retval;
}

int sflash_read_status_register( const sflash_handle_t* const handle, void* const dest_addr )
{
    return generic_sflash_command( handle, SFLASH_READ_STATUS_REGISTER, 0, NULL, 1, NULL, dest_addr );
}



int sflash_read( const sflash_handle_t* const handle, unsigned long device_address, void* const data_addr, unsigned int size )
{
    char device_address_array[3] =  { ( ( device_address & 0x00FF0000 ) >> 16 ),
                                      ( ( device_address & 0x0000FF00 ) >>  8 ),
                                      ( ( device_address & 0x000000FF ) >>  0 ) };

    return generic_sflash_command( handle, SFLASH_READ, 3, device_address_array, size, NULL, data_addr );
}


int sflash_get_size( const sflash_handle_t* const handle, unsigned long* const size )
{
    *size = 0; /* Unknown size to start with */

#ifdef SFLASH_SUPPORT_MACRONIX_PARTS
    if ( handle->device_id == SFLASH_ID_MX25L8006E )
    {
        *size = 0x100000; /* 1MByte */
    }
#endif /* ifdef SFLASH_SUPPORT_MACRONIX_PARTS */
#ifdef SFLASH_SUPPORT_SST_PARTS
    if ( handle->device_id == SFLASH_ID_SST25VF080B )
    {
        *size = 0x100000; /* 1MByte */
    }
#endif /* ifdef SFLASH_SUPPORT_SST_PARTS */
    return 0;
}

int sflash_write( const sflash_handle_t* const handle, unsigned long device_address, const void* const data_addr, int size )
{
    int status;
    int write_size;
    int max_write_size = 1;
    unsigned char enable_before_every_write = 1;
    unsigned char* data_addr_ptr = (unsigned char*) data_addr;
    unsigned char curr_device_address[3];

    if ( handle->write_allowed == SFLASH_WRITE_ALLOWED )
    {
    }
    else
    {
        return -1;
    }

    /* Some manufacturers support programming an entire page in one command. */

#ifdef SFLASH_SUPPORT_MACRONIX_PARTS
    if ( SFLASH_MANUFACTURER( handle->device_id ) == SFLASH_MANUFACTURER_MACRONIX )
    {
        max_write_size = 1;  /* TODO: this should be 256, but that causes write errors */
        enable_before_every_write = 1;
    }
#endif /* ifdef SFLASH_SUPPORT_MACRONIX_PARTS */
#ifdef SFLASH_SUPPORT_SST_PARTS
    if ( SFLASH_MANUFACTURER( handle->device_id ) == SFLASH_MANUFACTURER_SST )
    {
        max_write_size = 1;
        enable_before_every_write = 1;
    }
#endif /* ifdef SFLASH_SUPPORT_SST_PARTS */


    if ( ( enable_before_every_write == 0 ) &&
         ( 0 != ( status = sflash_write_enable( handle ) ) ) )
    {
        return status;
    }

    /* Generic x-bytes-at-a-time write */

    while ( size > 0 )
    {
        write_size = ( size > max_write_size )? max_write_size : size;
        curr_device_address[0] = ( ( device_address & 0x00FF0000 ) >> 16 );
        curr_device_address[1] = ( ( device_address & 0x0000FF00 ) >>  8 );
        curr_device_address[2] = ( ( device_address & 0x000000FF ) >>  0 );

        if ( ( enable_before_every_write == 1 ) &&
             ( 0 != ( status = sflash_write_enable( handle ) ) ) )
        {
            return status;
        }

        if ( 0 != ( status = generic_sflash_command( handle, SFLASH_WRITE, 3, curr_device_address, write_size, data_addr_ptr, NULL ) ) )
        {
            return status;
        }

        data_addr_ptr += write_size;
        device_address += write_size;
        size -= write_size;

    }

    return 0;
}

int sflash_write_status_register( const sflash_handle_t* const handle, char value )
{
    char status_register_val = value;
#ifdef SFLASH_SUPPORT_SST_PARTS
    /* SST parts require enabling writing to the status register */
    if ( SFLASH_MANUFACTURER( handle->device_id ) == SFLASH_MANUFACTURER_SST )
    {
        int status;
        if ( 0 != ( status = generic_sflash_command( handle, SFLASH_ENABLE_WRITE_STATUS_REGISTER, 0, NULL, 0, NULL, NULL ) ) )
        {
            return status;
        }
    }
#endif /* ifdef SFLASH_SUPPORT_SST_PARTS */

    return generic_sflash_command( handle, SFLASH_WRITE_STATUS_REGISTER, 0, NULL, 1, &status_register_val, NULL );
}


int init_sflash( sflash_handle_t* const handle, int peripheral_id, sflash_write_allowed_t write_allowed_in )
{
    int status;
    unsigned char tmp_device_id[3];

    if ( 0 != ( status = sflash_platform_init( peripheral_id, &handle->platform_peripheral ) ) )
    {
        return status;
    }

    if (0 != ( status = sflash_read_ID( handle, tmp_device_id ) ) )
    {
        return status;
    }


    handle->device_id = ( ((uint32_t) tmp_device_id[0]) << 16 ) +
                        ( ((uint32_t) tmp_device_id[1]) <<  8 ) +
                        ( ((uint32_t) tmp_device_id[2]) <<  0 );

    handle->write_allowed = write_allowed_in;

    if ( write_allowed_in == SFLASH_WRITE_ALLOWED )
    {
        /* Enable writing */
        if (0 != ( status = sflash_write_enable( handle ) ) )
        {
            return status;
        }
    }

    return 0;
}




int generic_sflash_command( const sflash_handle_t* const handle, sflash_command_t cmd, unsigned int num_initial_parameter_bytes, const void* const parameter_bytes, int num_data_bytes, const void* const data_MOSI, void* const data_MISO )
{
    int status;
    unsigned char* data_MISO_ptr = (unsigned char*) data_MISO;
    unsigned char* data_MOSI_ptr = (unsigned char*) data_MOSI;
    unsigned char* parameter_bytes_ptr = (unsigned char*) parameter_bytes;
    char is_write_command = ( ( cmd == SFLASH_WRITE ) ||
            ( cmd == SFLASH_CHIP_ERASE1 ) ||
            ( cmd == SFLASH_CHIP_ERASE2 ) ||
            ( cmd == SFLASH_SECTOR_ERASE ) ||
            ( cmd == SFLASH_BLOCK_ERASE_MID ) ||
            ( cmd == SFLASH_BLOCK_ERASE_LARGE ) );

    sflash_platform_chip_select( handle->platform_peripheral );

    if ( 0 != ( status = sflash_platform_send_recv_byte( handle->platform_peripheral, cmd, NULL ) ) )
    {
        return status;
    }

    while ( ( parameter_bytes != NULL ) &&
            ( num_initial_parameter_bytes > 0 ) )
    {
        if ( 0 != ( status = sflash_platform_send_recv_byte( handle->platform_peripheral, *parameter_bytes_ptr, NULL ) ) )
        {
            return status;
        }
        parameter_bytes_ptr++;
        num_initial_parameter_bytes--;
    }


    while ( num_data_bytes > 0 )
    {
        unsigned char dummy_dest;
        if ( 0 != ( status = sflash_platform_send_recv_byte( handle->platform_peripheral, ( data_MOSI == NULL )? SFLASH_DUMMY_BYTE : *data_MOSI_ptr, ( data_MISO == NULL )? &dummy_dest : data_MISO_ptr ) ) )
        {
            return status;
        }
        if ( data_MOSI != NULL )
        {
            data_MOSI_ptr++;
        }
        if ( data_MISO != NULL )
        {
            data_MISO_ptr++;
        }
        num_data_bytes--;
    }


    sflash_platform_chip_deselect( handle->platform_peripheral );

    if ( is_write_command )
    {
        unsigned char status_register;
        /* write commands require waiting until chip is finished writing */

        do
        {
            if ( 0 != ( status = sflash_read_status_register( handle, &status_register ) ) )
            {
                return status;
            }
        } while( ( status_register & SFLASH_STATUS_REGISTER_BUSY ) != 0 );

    }

    return 0;
}

