/*
 * Copyright 2013, Broadcom Corporation
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

#include "sam4s.h"
#include "stdio.h"
#include "string.h"
#include "core_cm4.h"
#include "efc.h"
#include "platform.h"
#include "platform_dct.h"
#include "Platform/wwd_platform_interface.h"
#include "wwd_assert.h"
#include "watchdog.h"
#include "wiced_platform.h"
#ifndef WICED_DISABLE_BOOTLOADER
#include "bootloader_app.h"
#endif

/******************************************************
 *                    Constants
 ******************************************************/

#define SAM4S_FLASH_START                    ( IFLASH0_ADDR )
#define SAM4S_LOCK_REGION_SIZE               ( IFLASH0_LOCK_REGION_SIZE )
#define SAM4S_PAGE_SIZE                      ( IFLASH0_PAGE_SIZE )
#define SAM4S_PAGES_PER_LOCK_REGION          ( SAM4S_LOCK_REGION_SIZE / SAM4S_PAGE_SIZE )

/* These come from the linker script */
extern void* dct1_start_addr_loc;
extern void* dct1_size_loc;
extern void* dct2_start_addr_loc;
extern void* dct2_size_loc;
extern void* app_hdr_start_addr_loc;
extern void* sram_start_addr_loc;
extern void* sram_size_loc;

#define APP_HDR_START_ADDR                   ((uint32_t)&app_hdr_start_addr_loc)
#define DCT1_START_ADDR                      ((uint32_t)&dct1_start_addr_loc)
#define DCT1_SIZE                            ((uint32_t)&dct1_size_loc)
#define DCT2_START_ADDR                      ((uint32_t)&dct2_start_addr_loc)
#define DCT2_SIZE                            ((uint32_t)&dct2_size_loc)
#define SRAM_START_ADDR                      ((uint32_t)&sram_start_addr_loc)
#define SRAM_SIZE                            ((uint32_t)&sram_size_loc)

/*
 * Memory Layout
 * +------------------------------+
 * |                              | Lock Region 0 (8KB)
 * +--------- Bootloader ---------+
 * |                              | Lock Region 1 (8KB)
 * +------------------------------+
 * |                              | Lock Region 2 (8KB)
 * +------------ DCT1 ------------+
 * |                              | Lock Region 3 (8KB)
 * +------------------------------+
 * |                              | Lock Region 4 (8KB)
 * +------------ DCT2 ------------+
 * |                              | Lock Region 5 (8KB)
 * +------------------------------+
 *
 * Section Definition
 * ==================
 * A sector in here refers to SAM4S LOCK REGION (8KB). This design decision is
 * made to align sector size with DCT block size and bootloader size.
 *
 * Page Definition
 * ===============
 * A page in here refers to SAM4S page (512B). SAM4S only supports page write.
 */

#define PLATFORM_DCT_COPY1_START_SECTOR      ( 2 )                           /* Lock Region 2 */
#define PLATFORM_DCT_COPY1_START_ADDRESS     ( DCT1_START_ADDR )             /*               */
#define PLATFORM_DCT_COPY1_END_SECTOR        ( 3 )                           /* Lock Region 3 */
#define PLATFORM_DCT_COPY1_END_ADDRESS       ( DCT1_START_ADDR + DCT1_SIZE ) /*               */
#define PLATFORM_DCT_COPY2_START_SECTOR      ( 4  )                          /* Lock Region 4 */
#define PLATFORM_DCT_COPY2_START_ADDRESS     ( DCT2_START_ADDR )             /*               */
#define PLATFORM_DCT_COPY2_END_SECTOR        ( 5 )                           /* Lock Region 5 */
#define PLATFORM_DCT_COPY2_END_ADDRESS       ( DCT1_START_ADDR + DCT1_SIZE ) /*               */

#define ERASE_DCT_1()                         platform_erase_flash(PLATFORM_DCT_COPY1_START_SECTOR, PLATFORM_DCT_COPY1_END_SECTOR)
#define ERASE_DCT_2()                         platform_erase_flash(PLATFORM_DCT_COPY2_START_SECTOR, PLATFORM_DCT_COPY2_END_SECTOR)

/******************************************************
 *                      Macros
 ******************************************************/

#define SAM4S_GET_LOCK_REGION_ADDR( region ) ( (region) * SAM4S_LOCK_REGION_SIZE + SAM4S_FLASH_START )
#define SAM4S_GET_PAGE_ADDR( page )          ( (page) * SAM4S_PAGE_SIZE + SAM4S_FLASH_START )
#define SAM4S_GET_PAGE_FROM_ADDR( addr )     ( (uint32_t)( ( ( addr ) - SAM4S_FLASH_START ) / SAM4S_PAGE_SIZE ) )

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    SAM4S_FLASH_ERASE_4_PAGES  = 0x04,
    SAM4S_FLASH_ERASE_8_PAGES  = 0x08,
    SAM4S_FLASH_ERASE_16_PAGES = 0x10,
    SAM4S_FLASH_ERASE_32_PAGES = 0x20,
} sam4s_flash_erase_page_amount_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifndef WICED_DISABLE_BOOTLOADER
static wiced_result_t platform_erase_flash_pages ( uint32_t start_page, sam4s_flash_erase_page_amount_t total_pages );
static wiced_result_t platform_lock_flash        ( uint32_t start_address, uint32_t end_address );
static wiced_result_t platform_unlock_flash      ( uint32_t start_address, uint32_t end_address );
static wiced_result_t platform_write_flash       ( uint32_t start_address, const uint8_t* data, uint32_t data_size );
#endif

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

#ifndef WICED_DISABLE_BOOTLOADER
void platform_erase_dct( void )
{
    ERASE_DCT_1();
    ERASE_DCT_2();
}

platform_dct_data_t* platform_get_dct( void )
{
    platform_dct_header_t hdr =
    {
        .write_incomplete    = 0,
        .is_current_dct      = 1,
        .app_valid           = 1,
        .mfg_info_programmed = 0,
        .magic_number        = BOOTLOADER_MAGIC_NUMBER,
        .load_app_func       = NULL
    };

    platform_dct_header_t* dct1 = ((platform_dct_header_t*) PLATFORM_DCT_COPY1_START_ADDRESS);
    platform_dct_header_t* dct2 = ((platform_dct_header_t*) PLATFORM_DCT_COPY2_START_ADDRESS);

    if ( ( dct1->is_current_dct == 1 ) &&
         ( dct1->write_incomplete == 0 ) &&
         ( dct1->magic_number == BOOTLOADER_MAGIC_NUMBER ) )
    {
        return (platform_dct_data_t*)dct1;
    }

    if ( ( dct2->is_current_dct == 1 ) &&
         ( dct2->write_incomplete == 0 ) &&
         ( dct2->magic_number == BOOTLOADER_MAGIC_NUMBER ) )
    {
        return (platform_dct_data_t*)dct2;
    }

    /* No valid DCT! */
    /* Erase the first DCT and init it. */
    ERASE_DCT_1();
//    platform_bootloader_erase_dct( 1 );

    platform_write_flash_chunk( PLATFORM_DCT_COPY1_START_ADDRESS, (uint8_t*) &hdr, sizeof(hdr) );

    return (platform_dct_data_t*)dct1;
}

/* TODO: Disable interrupts during function */
/* Note Function allocates a chunk of memory for the bootloader data on the stack - ensure the stack is big enough */
int platform_write_dct( uint16_t data_start_offset, const void* data, uint16_t data_length, int8_t app_valid, void (*func)(void) )
{
    platform_dct_header_t  updated_header;
    platform_dct_header_t* curr_dct_header;
    uint8_t*               curr_dct_start;
    uint8_t*               curr_dct_ptr;
    uint8_t*               curr_dct_data_start;
    uint8_t*               curr_dct_data_end;
    uint8_t*               curr_dct_header_start;
    uint8_t*               curr_dct_header_end;
    uint32_t*              new_dct_ptr;
    uint8_t*               new_dct_start;
    uint8_t*               updated_header_ptr;
    uint8_t*               data_ptr;

    /* Check if the data is too big to write */
    if ( data_length + data_start_offset > ( PLATFORM_DCT_COPY1_END_ADDRESS - PLATFORM_DCT_COPY1_START_ADDRESS ) )
    {
        return -1;
    }

    /* Get the start of current DCT */
    curr_dct_start  = (uint8_t*) &platform_get_dct( )->dct_header;
    curr_dct_header = (platform_dct_header_t*)curr_dct_start;

    /* Erase the non-current DCT */
    if ( curr_dct_start == ((uint8_t*)PLATFORM_DCT_COPY1_START_ADDRESS) )
    {
        new_dct_start  = (uint8_t*)PLATFORM_DCT_COPY2_START_ADDRESS;
        ERASE_DCT_2();
    }
    else
    {
        new_dct_start  = (uint8_t*)PLATFORM_DCT_COPY1_START_ADDRESS;
        ERASE_DCT_1();
    }

    /* Mark the beginning and end of the new data section and header section */
    curr_dct_header_start = curr_dct_start;
    curr_dct_header_end   = curr_dct_start + sizeof(platform_dct_header_t);
    curr_dct_data_start   = curr_dct_header_end + data_start_offset;
    curr_dct_data_end     = curr_dct_data_start + data_length;

    /* Updated header for the new DCT */
    updated_header.full_size           = curr_dct_header->full_size;
    updated_header.used_size           = curr_dct_header->used_size;
    updated_header.app_valid           = (char) ( ( app_valid == -1 ) ? curr_dct_header->app_valid : app_valid );
    updated_header.mfg_info_programmed = curr_dct_header->mfg_info_programmed;
    updated_header.write_incomplete    = 0;
    updated_header.is_current_dct      = 1;
    updated_header.magic_number        = BOOTLOADER_MAGIC_NUMBER;
    updated_header.load_app_func       = func;

    /* Unlock new DCT */
    platform_unlock_flash( (uint32_t) new_dct_start, (uint32_t) new_dct_start + DCT1_SIZE );

    curr_dct_ptr = curr_dct_start + SAM4S_PAGE_SIZE;
    new_dct_ptr  = (uint32_t*) ( new_dct_start + SAM4S_PAGE_SIZE );
    data_ptr     = ( curr_dct_data_start < curr_dct_ptr ) ? (uint8_t*) data + SAM4S_PAGE_SIZE - sizeof(platform_dct_header_t) - data_start_offset : (uint8_t*) data;

    /* Copy data from second page onwards. Leave the first page which contains DCT header to the end */
    while ( (uint32_t) new_dct_ptr < (uint32_t) ( new_dct_start + DCT1_SIZE ) )
    {
        uint8_t   temp_bytes[4]  = { 0 };
        uint32_t* temp_bytes_ptr = (uint32_t*)temp_bytes;
        uint32_t  byte;

        for ( byte = 0; byte < 4; byte++ )
        {
            if ( ( curr_dct_ptr >= curr_dct_data_start ) && ( curr_dct_ptr < curr_dct_data_end ) )
            {
                /* Copy new data to temp buffer */
                temp_bytes[byte] = *data_ptr++;
            }
            else
            {
                /* Copy data from current/old DCT to temp buffer */
                temp_bytes[byte] = *curr_dct_ptr;
            }

            curr_dct_ptr++;
        }

        /* 32-bit aligned write to the flash internal latch buffer */
        *new_dct_ptr++ = *temp_bytes_ptr;

        if ( ( (uint32_t)new_dct_ptr % SAM4S_PAGE_SIZE ) == 0 )
        {
            /* Send write page command */
            if ( efc_perform_command( EFC0, EFC_FCMD_WP, SAM4S_GET_PAGE_FROM_ADDR( (uint32_t)new_dct_ptr ) - 1 ) != 0 )
            {
                return -2;
            }
        }
    }

    /* Updated reference variables */
    curr_dct_ptr       = curr_dct_start;
    new_dct_ptr        = (uint32_t*)new_dct_start;
    data_ptr           = (uint8_t*)data;
    updated_header_ptr = (uint8_t*)&updated_header;

    /* Copy first page */
    while ( (uint32_t) new_dct_ptr < (uint32_t) ( new_dct_start + SAM4S_PAGE_SIZE ) )
    {
        uint8_t   temp_bytes[4]  = { 0 };
        uint32_t* temp_bytes_ptr = (uint32_t*)temp_bytes;
        uint32_t  byte;

        for ( byte = 0; byte < 4; byte++ )
        {
            if ( ( curr_dct_ptr >= curr_dct_header_start ) && ( curr_dct_ptr < curr_dct_header_end ) )
            {
                /* Copy updated header to temp buffer */
                temp_bytes[byte] = *updated_header_ptr++;
            }
            else if ( ( curr_dct_ptr >= curr_dct_data_start ) && ( curr_dct_ptr < curr_dct_data_end ) )
            {
                /* Copy new data to temp buffer */
                temp_bytes[byte] = *data_ptr++;
            }
            else
            {
                /* Copy data from current/old DCT to temp buffer */
                temp_bytes[byte] = *curr_dct_ptr;
            }

            curr_dct_ptr++;
        }

        /* 32-bit aligned write to the flash internal latch buffer */
        *new_dct_ptr++ = *temp_bytes_ptr;
    }

    /* Write first page to new DCT */
    if ( efc_perform_command( EFC0, EFC_FCMD_WP, SAM4S_GET_PAGE_FROM_ADDR( (uint32_t)new_dct_start ) ) != 0 )
    {
        return -3;
    }

    /* Lock the flash back */
    platform_lock_flash( (uint32_t) new_dct_start, (uint32_t) new_dct_start + DCT1_SIZE );

    /* Erase the non-current DCT */
    if ( curr_dct_start == ((uint8_t*)PLATFORM_DCT_COPY1_START_ADDRESS) )
    {
        ERASE_DCT_1();
    }
    else
    {
        ERASE_DCT_2();
    }

    platform_unlock_flash( (uint32_t) curr_dct_start, (uint32_t) curr_dct_start + DCT1_SIZE );

    updated_header.is_current_dct = 0;

    /* Updated reference variables */
    curr_dct_ptr       = curr_dct_start;
    new_dct_ptr        = (uint32_t*)curr_dct_start;
    updated_header_ptr = (uint8_t*)&updated_header;

    /* Copy first page */
    while ( (uint32_t) new_dct_ptr < (uint32_t) ( curr_dct_start + sizeof(platform_dct_header_t) ) )
    {
        uint8_t   temp_bytes[4]  = { 0 };
        uint32_t* temp_bytes_ptr = (uint32_t*)temp_bytes;
        uint32_t  byte;

        for ( byte = 0; byte < 4; byte++ )
        {
            if ( ( curr_dct_ptr >= curr_dct_header_start ) && ( curr_dct_ptr < curr_dct_header_end ) )
            {
                /* Copy updated header to temp buffer */
                temp_bytes[byte] = *updated_header_ptr++;
            }

            curr_dct_ptr++;
        }

        /* 32-bit aligned write to the flash internal latch buffer */
        *new_dct_ptr++ = *temp_bytes_ptr;
    }

    if ( efc_perform_command( EFC0, EFC_FCMD_WP, SAM4S_GET_PAGE_FROM_ADDR( (uint32_t)curr_dct_start ) ) != 0 )
    {
        return -4;
    }

    platform_lock_flash( (uint32_t) curr_dct_start, (uint32_t) curr_dct_start + DCT1_SIZE );

    return 0;
}

int platform_write_app_chunk( uint32_t offset, const uint8_t* data, uint32_t size )
{
    return platform_write_flash_chunk( APP_HDR_START_ADDR + offset, data, size );
}

int platform_write_flash_chunk( uint32_t address, const uint8_t* data, uint32_t size )
{
    wiced_result_t write_result = WICED_SUCCESS;

    if ( platform_unlock_flash( address, address + size ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    write_result = platform_write_flash(address , data, size );

    if ( platform_lock_flash( address, address + size ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }


    /* Successful */
    return ( write_result == WICED_SUCCESS ) ? 0 : -1;
}

int platform_erase_flash( uint16_t start_sector, uint16_t end_sector )
{
    uint32_t start_address = SAM4S_GET_PAGE_ADDR( start_sector * SAM4S_PAGES_PER_LOCK_REGION );
    uint32_t end_address   = SAM4S_GET_PAGE_ADDR( end_sector * SAM4S_PAGES_PER_LOCK_REGION );
    uint32_t i;

    if ( platform_unlock_flash( start_address, end_address ) != WICED_SUCCESS )
    {
        return -1;
    }

    for ( i = start_sector; i <= end_sector; i++ )
    {
        if ( platform_erase_flash_pages( i * SAM4S_PAGES_PER_LOCK_REGION, SAM4S_FLASH_ERASE_16_PAGES ) != WICED_SUCCESS )
        {
            return -1;
        }
    }

    if ( platform_lock_flash( start_address, end_address ) != WICED_SUCCESS )
    {
        return -1;
    }

    return 0;
}

static wiced_result_t platform_erase_flash_pages( uint32_t start_page, sam4s_flash_erase_page_amount_t total_pages )
{
    uint32_t erase_result = 0;
    uint32_t argument     = 0;

    watchdog_kick( );

    if ( total_pages == 32 )
    {
        start_page &= ~( 32u - 1u );
        argument = ( start_page ) | 3; /* 32 pages */
    }
    else if ( total_pages == 16 )
    {
        start_page &= ~( 16u - 1u );
        argument = ( start_page ) | 2; /* 16 pages */
    }
    else if ( total_pages == 8 )
    {
        start_page &= ~( 8u - 1u );
        argument = ( start_page ) | 1; /* 8 pages */
    }
    else
    {
        start_page &= ~( 4u - 1u );
        argument = ( start_page ) | 0; /* 4 pages */
    }

    erase_result = efc_perform_command( EFC0, EFC_FCMD_EPA, argument ) ;

    watchdog_kick( );

    return ( erase_result == 0 ) ? WICED_SUCCESS : WICED_ERROR;
}

static wiced_result_t platform_lock_flash( uint32_t start_address, uint32_t end_address )
{
    uint32_t start_page = SAM4S_GET_PAGE_FROM_ADDR( start_address );
    uint32_t end_page   = SAM4S_GET_PAGE_FROM_ADDR( end_address );

    start_page -= ( start_page % SAM4S_PAGES_PER_LOCK_REGION );

    while ( start_page <= end_page )
    {
        if ( efc_perform_command( EFC0, EFC_FCMD_SLB, start_page ) != 0 )
        {
            return WICED_ERROR;
        }

        start_page += SAM4S_PAGES_PER_LOCK_REGION;
    }

    return WICED_SUCCESS;
}

static wiced_result_t platform_unlock_flash( uint32_t start_address, uint32_t end_address )
{
    uint32_t start_page = SAM4S_GET_PAGE_FROM_ADDR( start_address );
    uint32_t end_page   = SAM4S_GET_PAGE_FROM_ADDR( end_address );

    start_page -= ( start_page % SAM4S_PAGES_PER_LOCK_REGION );

    while ( start_page <= end_page )
    {
        if ( efc_perform_command( EFC0, EFC_FCMD_CLB, start_page ) != 0 )
        {
            return WICED_ERROR;
        }

        start_page += SAM4S_PAGES_PER_LOCK_REGION;
    }

    return WICED_SUCCESS;
}

static wiced_result_t platform_write_flash( uint32_t start_address, const uint8_t* data, uint32_t data_size )
{
    uint32_t  start_page = SAM4S_GET_PAGE_FROM_ADDR( start_address );
    uint32_t  end_page   = SAM4S_GET_PAGE_FROM_ADDR( ( start_address + data_size ) );
    uint32_t* dst_ptr    = (uint32_t*)SAM4S_GET_PAGE_ADDR( start_page );
    uint8_t*  src_ptr    = (uint8_t*)dst_ptr;
    uint8_t*  data_ptr   = (uint8_t*)data;
    uint8_t*  data_start = (uint8_t*)start_address;
    uint8_t*  data_end   = data_start + data_size;
    uint32_t  page;
    uint32_t  word;
    uint32_t  byte;

    for ( page = start_page; page <= end_page; page++ )
    {
        for ( word = 0; word < 128; word++ )
        {
            uint8_t   word_to_write[4];
            uint32_t* word_to_write_ptr = (uint32_t*)word_to_write;

            UNUSED_PARAMETER( word_to_write );

            for ( byte = 0; byte < 4; byte++ )
            {
                if ( (src_ptr >= data_start) && (src_ptr < data_end) )
                {
                    word_to_write[byte] = *data_ptr++;
                }
                else
                {
                    word_to_write[byte] = *src_ptr;
                }

                src_ptr++;
            }

            /* 32-bit aligned write to the flash */
            *dst_ptr++ = *word_to_write_ptr;
        }

        /* Send write page command */

        if ( efc_perform_command( EFC0, EFC_FCMD_WP, page) != 0 )
        {
            return WICED_ERROR;
        }

    }

    return WICED_SUCCESS;
}
#endif /* WICED_DISABLE_BOOTLOADER */
