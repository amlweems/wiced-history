/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include <string.h>
#include <stdlib.h>
#include "spi_flash.h"
#include "platform_config.h"
#include "platform_peripheral.h"
#include "wwd_assert.h"
#include "wiced_framework.h"

/******************************************************
 *                    Constants
 ******************************************************/

#define SAM4S_FLASH_START                    ( IFLASH0_ADDR )
#define SAM4S_LOCK_REGION_SIZE               ( IFLASH0_LOCK_REGION_SIZE )
#define SAM4S_PAGE_SIZE                      ( IFLASH0_PAGE_SIZE )
#define SAM4S_PAGES_PER_LOCK_REGION          ( SAM4S_LOCK_REGION_SIZE / SAM4S_PAGE_SIZE )

#define APP_HDR_START_ADDR                   ((uint32_t)&app_hdr_start_addr_loc)
#define APP_CODE_START_ADDR                  ((uint32_t)&app_code_start_addr_loc)
#define SRAM_START_ADDR                      ((uint32_t)&sram_start_addr_loc)
#define SRAM_SIZE                            ((uint32_t)&sram_size_loc)
#define SAM4S_GET_LOCK_REGION_ADDR( region ) ( (region) * SAM4S_LOCK_REGION_SIZE + SAM4S_FLASH_START )
#define SAM4S_GET_PAGE_ADDR( page )          ( (page) * SAM4S_PAGE_SIZE + SAM4S_FLASH_START )
#define SAM4S_GET_PAGE_FROM_ADDR( addr )     ( (uint32_t)( ( ( addr ) - SAM4S_FLASH_START ) / SAM4S_PAGE_SIZE ) )

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    SAM4S_FLASH_ERASE_4_PAGES = 0x04,
    SAM4S_FLASH_ERASE_8_PAGES = 0x08,
    SAM4S_FLASH_ERASE_16_PAGES = 0x10,
    SAM4S_FLASH_ERASE_32_PAGES = 0x20,
} sam4s_flash_erase_page_amount_t;

/******************************************************
 *               Static Function Declarations
 ******************************************************/
static wiced_result_t platform_write_flash( uint32_t start_address, const uint8_t* data, uint32_t data_size );
static wiced_result_t platform_unlock_flash( uint32_t start_address, uint32_t end_address );
static wiced_result_t platform_lock_flash( uint32_t start_address, uint32_t end_address );

/******************************************************
 *               Variable Definitions
 ******************************************************/
/* These come from linker */
extern void* app_hdr_start_addr_loc;
extern void* app_code_start_addr_loc;
extern void* sram_start_addr_loc;
extern void* sram_size_loc;

/******************************************************
 *               Function Definitions
 ******************************************************/

#if defined ( __ICCARM__ )

static inline void __jump_to( uint32_t addr )
{
    __asm( "MOV R1, #0x00000001" );
    __asm( "ORR R0, R1, #0" ); /* Last bit of jump address indicates whether destination is Thumb or ARM code */
    __asm( "BX R0" );
}

#elif defined ( __GNUC__ )

__attribute__( ( always_inline ) ) static __INLINE void __jump_to( uint32_t addr )
{
    addr |= 0x00000001; /* Last bit of jump address indicates whether destination is Thumb or ARM code */
    __ASM volatile ("BX %0" : : "r" (addr) );
}

#endif

void platform_start_app( uint32_t entry_point )
{

    /* Simulate a reset for the app: */
    /*   Switch to Thread Mode, and the Main Stack Pointer */
    /*   Change the vector table offset address to point to the app vector table */
    /*   Set other registers to reset values (esp LR) */
    /*   Jump to the reset vector */

    if ( entry_point == 0 )
    {
        uint32_t* vector_table = (uint32_t*) APP_CODE_START_ADDR;
        entry_point = vector_table[ 1 ];
    }

    __asm( "MOV LR,        #0xFFFFFFFF" );
    __asm( "MOV R1,        #0x01000000" );
    __asm( "MSR APSR_nzcvq,     R1" );
    __asm( "MOV R1,        #0x00000000" );
    __asm( "MSR PRIMASK,   R1" );
    __asm( "MSR FAULTMASK, R1" );
    __asm( "MSR BASEPRI,   R1" );
    __asm( "MSR CONTROL,   R1" );

    /*  Now rely on the app crt0 to load VTOR / Stack pointer

     SCB->VTOR = vector_table_address; - Change the vector table to point to app vector table
     __set_MSP( *stack_ptr ); */

    __jump_to( entry_point );

}

static wiced_result_t platform_erase_flash_pages( uint32_t start_page, sam4s_flash_erase_page_amount_t total_pages )
{
    uint32_t erase_result = 0;
    uint32_t argument = 0;

    platform_watchdog_kick( );

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

    erase_result = efc_perform_command( EFC0, EFC_FCMD_EPA, argument );

    platform_watchdog_kick( );

    return ( erase_result == 0 ) ? WICED_SUCCESS : WICED_ERROR;
}

int platform_erase_flash( uint16_t start_sector, uint16_t end_sector )
{
    uint32_t start_address = SAM4S_GET_PAGE_ADDR( start_sector * SAM4S_PAGES_PER_LOCK_REGION );
    uint32_t end_address = SAM4S_GET_PAGE_ADDR( end_sector * SAM4S_PAGES_PER_LOCK_REGION );
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

int platform_write_flash_chunk( uint32_t address, const void* data, uint32_t size )
{
    wiced_result_t write_result = WICED_SUCCESS;

    if ( platform_unlock_flash( address, address + size ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    write_result = platform_write_flash( address, data, size );

    if ( platform_lock_flash( address, address + size ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    /* Successful */
    return ( write_result == WICED_SUCCESS ) ? 0 : -1;
}

static wiced_result_t platform_lock_flash( uint32_t start_address, uint32_t end_address )
{
    uint32_t start_page = SAM4S_GET_PAGE_FROM_ADDR( start_address );
    uint32_t end_page = SAM4S_GET_PAGE_FROM_ADDR( end_address );

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
    uint32_t end_page = SAM4S_GET_PAGE_FROM_ADDR( end_address );

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
    uint32_t start_page = SAM4S_GET_PAGE_FROM_ADDR( start_address );
    uint32_t end_page = SAM4S_GET_PAGE_FROM_ADDR( ( start_address + data_size ) );
    uint32_t* dst_ptr = (uint32_t*) SAM4S_GET_PAGE_ADDR( start_page );
    uint8_t* src_ptr = (uint8_t*) dst_ptr;
    uint8_t* data_ptr = (uint8_t*) data;
    uint8_t* data_start = (uint8_t*) start_address;
    uint8_t* data_end = data_start + data_size;
    uint32_t page;
    uint32_t word;
    uint32_t byte;

    for ( page = start_page; page <= end_page; page++ )
    {
        for ( word = 0; word < 128; word++ )
        {
            uint8_t word_to_write[ 4 ];
            uint32_t* word_to_write_ptr = (uint32_t*) word_to_write;

            UNUSED_PARAMETER( word_to_write );

            for ( byte = 0; byte < 4; byte++ )
            {
                if ( ( src_ptr >= data_start ) && ( src_ptr < data_end ) )
                {
                    word_to_write[ byte ] = *data_ptr++ ;
                }
                else
                {
                    word_to_write[ byte ] = *src_ptr;
                }

                src_ptr++ ;
            }

            /* 32-bit aligned write to the flash */
            *dst_ptr++ = *word_to_write_ptr;
        }

        /* Send write page command */

        if ( efc_perform_command( EFC0, EFC_FCMD_WP, page ) != 0 )
        {
            return WICED_ERROR;
        }

    }

    return WICED_SUCCESS;
}

void platform_load_app_chunk( const image_location_t* app_header_location, uint32_t offset, void* physical_address, uint32_t size )
{
    (void) app_header_location;
    (void) offset;
    (void) physical_address;
    (void) size;
}
