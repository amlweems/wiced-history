/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include <stdlib.h>
//#include "core_cm4.h"
#include "sam4s_platform.h"
#include "watchdog.h"
#include "spi_flash.h"
#include "platform.h"
#include "platform_dct.h"
#include "platform_bootloader.h"
#include "platform_common_config.h"
#include "platform_sflash_dct.h"
#include "bootloader.h"
#include "wiced_platform.h"

#define PLATFORM_APP_START_SECTOR      ( 6 )
#define PLATFORM_APP_END_SECTOR        ( 127 )

#define PLATFORM_LOAD_STACK_LOCATION    ( SRAM_START_ADDR + SRAM_SIZE - 0x2000 )

#define PLATFORM_SFLASH_PERIPHERAL_ID  ( 0 )
#define APP_IMAGE_LOCATION_IN_SFLASH   ( 0 )



#define MIN(x,y)    ((x) < (y) ? (x) : (y))

/* These come from the linker script */
extern void* dct1_start_addr_loc;
extern void* dct1_size_loc;
extern void* dct2_start_addr_loc;
extern void* dct2_size_loc;
extern void* app_hdr_start_addr_loc;
extern void* sram_start_addr_loc;
extern void* sram_size_loc;
#define APP_HDR_START_ADDR   ((uint32_t)&app_hdr_start_addr_loc)
#define DCT1_START_ADDR  ((uint32_t)&dct1_start_addr_loc)
#define DCT1_SIZE        ((uint32_t)&dct1_size_loc)
#define DCT2_START_ADDR  ((uint32_t)&dct2_start_addr_loc)
#define DCT2_SIZE        ((uint32_t)&dct2_size_loc)
#define SRAM_START_ADDR  ((uint32_t)&sram_start_addr_loc)
#define SRAM_SIZE        ((uint32_t)&sram_size_loc)

wiced_result_t platform_get_sflash_dct_loc( sflash_handle_t* sflash_handle, uint32_t* loc )
{
    bootloader_app_header_t image_header;
    int retval;
    retval = sflash_read( sflash_handle, APP_IMAGE_LOCATION_IN_SFLASH, &image_header, sizeof( image_header ) );
    if ( retval != 0 )
    {
        return WICED_ERROR;
    }
    *loc = image_header.size_of_app;

    return WICED_SUCCESS;
}

void platform_restore_factory_app( void )
{
    sflash_handle_t sflash_handle;
    bootloader_app_header_t image_header;
    static uint8_t  rx_buffer[4096]; /* API Function will not return, so it is safe to declare static big buffer */
    uint32_t write_address = APP_HDR_START_ADDR;
    platform_dct_header_t dct_header;
    uint32_t progress;

    platform_set_bootloader_led( 1 );

    /* Mark the App image as invalid to guard against power failure during writing */
    platform_set_app_valid_bit( APP_INVALID );


    watchdog_kick( );

    /* Erase the internal flash memory */
    platform_erase_app( );


    watchdog_kick( );


    /* Init the external SPI flash */
    init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_NOT_ALLOWED );

    watchdog_kick( );

    /* Read the image header */
    sflash_read( &sflash_handle, APP_IMAGE_LOCATION_IN_SFLASH, &image_header, sizeof( image_header ) );

    watchdog_kick( );

    /* Quick image validation */

    /* Flash the factory app */
    progress = 0;
    while ( progress < image_header.size_of_app )
    {
        /* Read a chunk of image data */
        uint32_t read_size = MIN(image_header.size_of_app - progress, sizeof(rx_buffer));
        sflash_read( &sflash_handle, APP_IMAGE_LOCATION_IN_SFLASH + progress, &rx_buffer, read_size );

        /* Write it to the flash */
        platform_write_flash_chunk( write_address, rx_buffer, read_size );

        write_address += read_size;
        progress      += read_size;
        watchdog_kick( );
    }

    /* Read the DCT header (starts immediately after app */
    sflash_read( &sflash_handle, image_header.size_of_app, &dct_header, sizeof( platform_dct_header_t ) );

    /* Erase the DCT */
    platform_erase_dct( );

    /* Flash the factory DCT */
    write_address = DCT1_START_ADDR;
    progress = 0;
    while ( progress < dct_header.used_size )
    {
        /* Read a chunk of dct image data */
        uint32_t read_size = MIN(dct_header.used_size - progress, sizeof(rx_buffer));
        sflash_read( &sflash_handle, image_header.size_of_app + progress, &rx_buffer, read_size );

        /* Write it to the flash */
        platform_write_flash_chunk( write_address, rx_buffer, read_size );

        write_address += read_size;
        progress      += read_size;
        watchdog_kick( );
    }

    platform_set_app_valid_bit( APP_VALID );
    platform_start_app( 0 );
}
uint8_t temp_tx[100];
uint8_t temp_rx[100];

void platform_load_ota_app( void )
{
    sflash_handle_t sflash_handle;
    bootloader_app_header_t image_header;
    uint32_t start_of_ota_image;
    uint32_t start_of_ota_app;
    platform_dct_header_t dct_header;

    /* Move the stack so that it does not get overwritten when the OTA upgrade app is copied into memory */
    platform_set_load_stack( );

    /* Init the external SPI flash */
    init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_NOT_ALLOWED );

    watchdog_kick( );

    /* Read the factory app image header */
    sflash_read( &sflash_handle, APP_IMAGE_LOCATION_IN_SFLASH, &image_header, sizeof( image_header ) );

    watchdog_kick( );

    /* Read the DCT header (starts immediately after app */
    sflash_read( &sflash_handle, image_header.size_of_app, &dct_header, sizeof( platform_dct_header_t ) );

    watchdog_kick( );

    /* Read the image header of the OTA application which starts at the end of the dct image */
    start_of_ota_image = image_header.size_of_app + dct_header.full_size;
    sflash_read( &sflash_handle, APP_IMAGE_LOCATION_IN_SFLASH + start_of_ota_image, &image_header, sizeof( image_header ) );
    start_of_ota_app = start_of_ota_image + image_header.offset_to_vector_table;

    watchdog_kick( );

    /* Write the OTA app */
    sflash_read( &sflash_handle, start_of_ota_app, (void*)SRAM_START_ADDR, image_header.size_of_app );

    watchdog_kick( );

    platform_start_app( SRAM_START_ADDR );
}

int platform_erase_app( void )
{
   return platform_erase_flash( PLATFORM_APP_START_SECTOR, PLATFORM_APP_END_SECTOR );
}


#if defined ( __ICCARM__ )

static inline void __jump_to( uint32_t addr )
{

    __asm( "ORR R0, R0, #1" );  /* Last bit of jump address indicates whether destination is Thumb or ARM code */
    __asm( "BX R0" );
}

#elif defined ( __GNUC__ )

__attribute__( ( always_inline ) ) static __INLINE void __jump_to( uint32_t addr )
{
    addr |= 0x00000001;  /* Last bit of jump address indicates whether destination is Thumb or ARM code */
  __ASM volatile ("BX %0" : : "r" (addr) );
}

#endif


void platform_start_app( uint32_t vector_table_address )
{
    uint32_t* stack_ptr;
    uint32_t* start_ptr;

    /* Simulate a reset for the app: */
    /*   Load the first value of the app vector table into the stack pointer */
    /*   Switch to Thread Mode, and the Main Stack Pointer */
    /*   Change the vector table offset address to point to the app vector table */
    /*   Set other registers to reset values (esp LR) */
    /*   Jump to the reset vector */


    if ( vector_table_address == 0 )
    {
        const bootloader_app_header_t* bootloader_app_header = (bootloader_app_header_t*) APP_HDR_START_ADDR;

        vector_table_address = bootloader_app_header->offset_to_vector_table + APP_HDR_START_ADDR;
    }

    stack_ptr = (uint32_t*) vector_table_address;  /* Initial stack pointer is first 4 bytes of vector table */
    start_ptr = ( stack_ptr + 1 );  /* Reset vector is second 4 bytes of vector table */

    __asm( "MOV LR,        #0xFFFFFFFF" );
    __asm( "MOV R1,        #0x01000000" );
    __asm( "MSR APSR_nzcvq,     R1" );
    __asm( "MOV R1,        #0x00000000" );
    __asm( "MSR PRIMASK,   R1" );
    __asm( "MSR FAULTMASK, R1" );
    __asm( "MSR BASEPRI,   R1" );
    __asm( "MSR CONTROL,   R1" );

    SCB->VTOR = vector_table_address; /* Change the vector table to point to app vector table */
    __set_MSP( *stack_ptr );
    __jump_to( *start_ptr );

}

void platform_set_load_stack( void )
{
    __set_MSP( PLATFORM_LOAD_STACK_LOCATION );
}

void platform_reboot( void )
{
    /* Reset request */
    NVIC_SystemReset( );
}


unsigned char platform_get_bootloader_button( void )
{
    /* Always returns 0 to force bootloader to always continue and start running app */
    return 0;
}

void platform_set_bootloader_led( unsigned char on )
{
    UNUSED_PARAMETER( on );
    /* Empty for now. No Button nor LED */
}


int platform_read_wifi_firmware( uint32_t address, void* buffer, uint32_t requested_size, uint32_t* read_size )
{
    int status;
    sflash_handle_t sflash_handle;
    bootloader_app_header_t image_header;

    *read_size = 0;

    /* Initialise the serial flash */
    if ( 0 != ( status = init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_NOT_ALLOWED ) ) )
    {
        return status;
    }

    /* Read the image size from the serial flash */
    if ( 0 != ( status = sflash_read( &sflash_handle, APP_IMAGE_LOCATION_IN_SFLASH, &image_header, sizeof( image_header ) ) ) )
    {
        return status;
    }

    if ( address > image_header.size_of_wlan_firmware )
    {
        return -1;
    }

    if ( address + requested_size > image_header.size_of_wlan_firmware )
    {
        requested_size = image_header.size_of_wlan_firmware - address;
    }

    address += image_header.address_of_wlan_firmware - APP_HDR_START_ADDR;

    if ( 0 != ( status = sflash_read( &sflash_handle, address, buffer, requested_size ) ) )
    {
        return status;
    }

    *read_size = requested_size;

    return WICED_SUCCESS;
}

int platform_set_app_valid_bit( app_valid_t val )
{
    return platform_write_dct( 0, NULL, 0, (char) val, NULL );
}

int platform_kick_watchdog( void )
{
    /* TODO: Implement */
    return 0;
}

