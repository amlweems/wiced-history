/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  OTA Image status check and extraction routines
 *
 *  OTA Image File consists of:
 *
 *  wiced_ota2_image_header_t            - info about the OTA Image
 *  wiced_ota2_image_component_t         - array of individual component info structs
 *  ota_data                            - all the data
 *
 */

#include <stdlib.h>
#include <string.h>

#include "wwd_assert.h"
#include "wiced_result.h"
#include "wiced_utilities.h"
#include "wiced_time.h"
#include "platform_dct.h"
#include "wiced_rtos.h"
#include "wiced_apps_common.h"
#include "wiced_waf_common.h"
#include "wiced_dct_common.h"
#include "waf_platform.h"
#include "platform.h"
#include "platform_peripheral.h"
#include "spi_flash.h"

#include "wiced_ota2_image.h"
#include "mini_printf.h"

/* define to verify writes  */
//#define VERIFY_SFLASH_WRITES    1

// if we are running bootloader, use a different print mechanism
#if defined(BOOTLOADER)
extern int mini_printf( const char *message, ...);
#define OTA2_WPRINT_ERROR(arg)  {mini_printf arg; }
#define OTA2_WPRINT_INFO(arg)   {mini_printf arg; }
#define OTA2_WPRINT_DEBUG(arg)  {mini_printf arg; }
#else
#define OTA2_WPRINT_ERROR(arg)  WPRINT_LIB_ERROR(arg)
#define OTA2_WPRINT_INFO(arg)   WPRINT_LIB_INFO(arg)
#define OTA2_WPRINT_DEBUG(arg)  WPRINT_LIB_DEBUG(arg)
#endif

/* only time how long it takes when in an app - bootloader doesn't have the time stuff */
#if !defined(BOOTLOADER)
#define  DEBUG_GET_DECOMP_TIME      1
#endif

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
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

char *ota2_image_type_name_string[] =
{
    "OTA2_IMAGE_TYPE_NONE",
    "OTA2_IMAGE_TYPE_FACTORY",
    "OTA2_IMAGE_TYPE_CURRENT_APP",
    "OTA2_IMAGE_TYPE_LAST_KNOWN_GOOD",
    "OTA2_IMAGE_TYPE_STAGED"
};

/* used by wiced_ota2_safe_write_data_to_sflash() so we don't have to allocate a buffer every time! */
uint8_t sector_in_ram[SECTOR_SIZE];
uint8_t component_decomp_ram[SECTOR_SIZE];

/******************************************************
 *               Function Declarations
 ******************************************************/
static void wiced_ota2_print_header_info(wiced_ota2_image_header_t* ota2_header, const char* mssg);

uint32_t wiced_ota2_image_get_offset( wiced_ota2_image_type_t ota_type );
/******************************************************
 *               Internal Functions
 ******************************************************/

static void wiced_ota2_print_component_info(wiced_ota2_image_component_t *component)
{
    OTA2_WPRINT_INFO(("          name: %s\r\n", component->name));
    OTA2_WPRINT_INFO(("          type: %d\r\n", component->type));
    OTA2_WPRINT_INFO(("          comp: %d\r\n", component->compression));
    OTA2_WPRINT_INFO(("           crc: 0x%08lx\r\n", component->crc));
    OTA2_WPRINT_INFO(("      src_size: %8ld 0x%lx\r\n", component->source_size, component->source_size));
    OTA2_WPRINT_INFO(("       src_off: %8ld 0x%lx\r\n", component->source_offset, component->source_offset));
    OTA2_WPRINT_INFO(("      dst_size: %8ld 0x%lx\r\n", component->destination_size, component->destination_size));
    OTA2_WPRINT_INFO(("          dest: %8ld 0x%lx\r\n", component->destination, component->destination));
}

static void wiced_ota2_print_header_info(wiced_ota2_image_header_t* ota2_header, const char* mssg)
{
    if( ota2_header == NULL )
    {
        OTA2_WPRINT_INFO(( "wiced_ota2_print_header_info() ota2_header == NULL\r\n" ));
        return;
    }

    OTA2_WPRINT_INFO(("OTA header Information: %s\r\n", mssg));
    OTA2_WPRINT_INFO(("       ota_version: %d\r\n", ota2_header->ota2_version));
    OTA2_WPRINT_INFO(("    software_major: %d\r\n", ota2_header->major_version));
    OTA2_WPRINT_INFO(("    software_minor: %d\r\n", ota2_header->minor_version));
    OTA2_WPRINT_INFO(("          platform: %s\r\n", ota2_header->platform_name));

    switch( (wiced_ota2_image_status_t)ota2_header->download_status )
    {
    default:
    case WICED_OTA2_IMAGE_INVALID:
        OTA2_WPRINT_INFO(("            status: %d (invalid)\r\n", ota2_header->download_status));
        break;
    case WICED_OTA2_IMAGE_VALID:
        OTA2_WPRINT_INFO(("            status: %d (valid)\r\n", ota2_header->download_status));
        break;
    case WICED_OTA2_IMAGE_DOWNLOAD_IN_PROGRESS:
        OTA2_WPRINT_INFO(("            status: %d (download in progress)\r\n", ota2_header->download_status));
        break;
    case WICED_OTA2_IMAGE_DOWNLOAD_FAILED:
        OTA2_WPRINT_INFO(("            status: %d (download FAILED)\r\n", ota2_header->download_status));
        break;
    case WICED_OTA2_IMAGE_DOWNLOAD_COMPLETE:
        OTA2_WPRINT_INFO(("            status: %d (download complete)\r\n", ota2_header->download_status));
        break;
    case WICED_OTA2_IMAGE_EXTRACT_ON_NEXT_BOOT:
        OTA2_WPRINT_INFO(("            status: %d (extract on boot)\r\n", ota2_header->download_status));
        break;
    case WICED_OTA2_IMAGE_DOWNLOAD_EXTRACTED:
        OTA2_WPRINT_INFO(("            status: %d (extracted!)\r\n", ota2_header->download_status));
        break;
    }
    OTA2_WPRINT_INFO(("    bytes_received: %ld\r\n", ota2_header->bytes_received));
    OTA2_WPRINT_INFO(("        header_crc: 0x%lx\r\n", ota2_header->header_crc));
    OTA2_WPRINT_INFO(("      magic_string: %c%c%c%c%c%c%c%c\r\n", ota2_header->magic_string[0], ota2_header->magic_string[1],
                                                 ota2_header->magic_string[2], ota2_header->magic_string[3],
                                                 ota2_header->magic_string[4], ota2_header->magic_string[5],
                                                 ota2_header->magic_string[6], ota2_header->magic_string[7] ));
    OTA2_WPRINT_INFO(("         sign_type: %d\r\n", ota2_header->secure_sign_type));
    OTA2_WPRINT_INFO(("  secure_signature: TODO \r\n")); /* TODO */

    OTA2_WPRINT_INFO(("        image_size: %ld\r\n", ota2_header->image_size));
    OTA2_WPRINT_INFO(("   component_count: %d\r\n", ota2_header->component_count));
    OTA2_WPRINT_INFO((" data start offset: %ld\r\n", ota2_header->data_start));
}

/**
 * read data from FLASH
 *
 * @param[in]  offset   - offset to the data
 * @param[in]  data     - the data area to copy into
 * @param[in]  size     - size of the data
 *
 * @return - WICED_SUCCESS
 *           WICED_BADARG
 */
static wiced_result_t wiced_ota2_read_sflash(uint32_t offset, uint8_t* data, uint32_t size)
{
    wiced_result_t  result = WICED_SUCCESS;

    sflash_handle_t sflash_handle;
    if (init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_NOT_ALLOWED ) != WICED_SUCCESS)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_read_sflash() Failed to init SFLASH for reading.\r\n"));
        return WICED_ERROR;
    }
    if (sflash_read( &sflash_handle, (unsigned long)offset, data, size) != 0 )
    {
        /* read sector fail */
        OTA2_WPRINT_INFO(("wiced_ota2_read_sflash() read error!\r\n"));
        result = WICED_ERROR;
    }

    if (deinit_sflash( &sflash_handle ) != WICED_SUCCESS)   /* TODO: leave sflash in a known state */
    {
        OTA2_WPRINT_INFO(("wiced_ota2_read_sflash() Failed to deinit SFLASH.\r\n"));
        result = WICED_ERROR;
    }

    return result;
}

/**
 * Write any size data to FLASH
 * Handle erasing then writing the data
 * Erasing / writing is done on sector boundaries, work around this limitation
 *
 * @param[in]  offset   - offset to the sector
 * @param[in]  data     - the data
 * @param[in]  size     - size of the data
 *
 * @return - WICED_SUCCESS
 *           WICED_BADARG
 *           WICED_OUT_OF_HEAP_SPACE
 */
static wiced_result_t wiced_ota2_safe_write_data_to_sflash( uint32_t offset, uint8_t *data, uint32_t size )
{
    uint32_t        current_offset, sector_base_offset, in_sector_offset;
    uint32_t        bytes_to_write;
    uint32_t        chunk_size;
    uint8_t*        in_ptr;
    wiced_result_t  result = WICED_SUCCESS;
    sflash_handle_t sflash_handle;


    if (init_sflash( &sflash_handle, PLATFORM_SFLASH_PERIPHERAL_ID, SFLASH_WRITE_ALLOWED )!= 0)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_safe_write_data_to_sflash() Failed to init SFLASH for writing.\r\n"));
        return WICED_ERROR;
    }

    /* read a whole sector into RAM
     * modify the parts of the data in the RAM copy
     * erase the whole sector
     * write the new data to the sector
     */

    current_offset = offset;
    bytes_to_write = size;
    in_ptr = data;
    while ((result == WICED_SUCCESS) && (bytes_to_write > 0))
    {
        /* figure out the base of the sector we want to modify
         * NOTE: This is an offset into the FLASH
         */
        sector_base_offset = current_offset - (current_offset % SECTOR_SIZE);

        /* read the whole sector into our RAM buffer before we modify it */
        if (sflash_read( &sflash_handle, (unsigned long)sector_base_offset, sector_in_ram, SECTOR_SIZE) != 0 )
        {
            /* read sector fail */
            OTA2_WPRINT_INFO(("wiced_ota2_safe_write_data_to_sflash() read sector error!\r\n"));
            result = WICED_ERROR;
            break;
        }

        /* how far into the sector will we start copying ? */
        in_sector_offset = current_offset - sector_base_offset;

        /* How many bytes in this sector after in_sector_offset will we copy?
         * We are going to copy from in_sector_offset up to the end of the sector
         */
        chunk_size = SECTOR_SIZE - in_sector_offset;
        if (chunk_size > bytes_to_write)
        {
            chunk_size = bytes_to_write;
        }

        memcpy(&sector_in_ram[in_sector_offset], in_ptr, chunk_size);

        if (sflash_sector_erase( &sflash_handle, (unsigned long)sector_base_offset) != 0)
        {
            /* erase sector fail */
            OTA2_WPRINT_INFO(("wiced_ota2_safe_write_data_to_sflash() erase sector error!\r\n"));
            result = WICED_ERROR;
            break;
        }

        if ( 0 != sflash_write( &sflash_handle, (unsigned long)sector_base_offset, sector_in_ram, SECTOR_SIZE ) )
        {
            /* Verify Error - Chip not erased properly */
            OTA2_WPRINT_INFO(("wiced_ota2_safe_write_data_to_sflash() WRITE ERROR!\r\n"));
            result = WICED_ERROR;
        }

#ifdef VERIFY_SFLASH_WRITES
        if (sflash_read( &sflash_handle, (unsigned long)sector_base_offset, sector_in_ram, SECTOR_SIZE) != 0 )
        {
            /* read sector fail */
            OTA2_WPRINT_INFO(("wiced_ota2_safe_write_data_to_sflash() read sector error!\r\n"));
            result = WICED_ERROR;
            break;
        }

        if ( (memcmp(&sector_in_ram[in_sector_offset], in_ptr, chunk_size) != 0) )
        {
            OTA2_WPRINT_INFO(("wiced_ota2_safe_write_data_to_sflash() Verify ERROR!\r\n"));
            OTA2_WPRINT_INFO(("     base:%lx offset:%lx to_write:%ld chunk:%lx!\r\n", sector_base_offset, current_offset, bytes_to_write, chunk_size));
            result = WICED_ERROR;
        }
#endif

        bytes_to_write -= chunk_size;
        in_ptr         += chunk_size;
        current_offset += chunk_size;
    }

    if (deinit_sflash( &sflash_handle ) != WICED_SUCCESS)   /* TODO: leave sflash in a known state */
    {
        OTA2_WPRINT_INFO(("wiced_ota2_safe_write_data_to_sflash() Failed to deinit SFLASH.\r\n"));
        result = WICED_ERROR;
    }

    return result;
}

/* *****************************************************
 *
 *read the ota2_header and swap values from network order to host order
 */
static inline wiced_result_t wiced_ota2_read_ota2_header(wiced_ota2_image_type_t ota_type, wiced_ota2_image_header_t* ota2_header)
{
    uint32_t    base_ota2_offset;

    base_ota2_offset = wiced_ota2_image_get_offset(ota_type);
    if ((base_ota2_offset == 0) || (ota2_header == NULL))
    {
        OTA2_WPRINT_INFO(("wiced_ota2_read_ota2_header() Bad args.\r\n"));
        return WICED_BADARG;
    }

    if (wiced_ota2_read_sflash( base_ota2_offset, (uint8_t*)ota2_header, sizeof(wiced_ota2_image_header_t)) != WICED_SUCCESS)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_read_ota2_header(%s) Failed to read SFLASH.\r\n", ota2_image_type_name_string[ota_type]));
        return WICED_ERROR;
    }

    wiced_ota2_image_header_swap_network_order(ota2_header, WICED_OTA2_IMAGE_SWAP_NETWORK_TO_HOST);
    return WICED_SUCCESS;
}

/* *****************************************************
 *
 *  swap the data for network order before saving
 */
static inline wiced_result_t wiced_ota2_write_ota2_header(wiced_ota2_image_type_t ota_type, wiced_ota2_image_header_t* ota2_header)
{
    wiced_ota2_image_header_t   swapped_ota2_header;
    uint32_t                    base_ota_offset;
    wiced_result_t              result = WICED_SUCCESS;

    base_ota_offset = wiced_ota2_image_get_offset(ota_type);
    if ((base_ota_offset == 0) || (ota2_header == NULL))
    {
        OTA2_WPRINT_INFO(("wiced_ota_read_component_header() Bad args!\r\n"));
        return WICED_BADARG;
    }

    /* make our copy of the data so we can swap it without affecting the caller's copy */
    memcpy( &swapped_ota2_header, ota2_header, sizeof(wiced_ota2_image_header_t));
    wiced_ota2_image_header_swap_network_order(&swapped_ota2_header, WICED_OTA2_IMAGE_SWAP_HOST_TO_NETWORK);
    if (wiced_ota2_safe_write_data_to_sflash( base_ota_offset, (uint8_t*)&swapped_ota2_header, sizeof(wiced_ota2_image_header_t)) != WICED_SUCCESS)
    {
        result = WICED_ERROR;
    }

    return result;
}

/* *****************************************************
 *
 */
static inline wiced_result_t wiced_ota_read_component_header( wiced_ota2_image_type_t ota_type, uint16_t component_index, wiced_ota2_image_component_t* component_header)
{
    uint32_t    base_ota_offset;
    uint32_t    component_header_offset;

    base_ota_offset = wiced_ota2_image_get_offset(ota_type);
    if ((base_ota_offset == 0) || (component_header == NULL))
    {
        OTA2_WPRINT_INFO(("wiced_ota_read_component_header() Bad args!\r\n"));
        return WICED_BADARG;
    }

    component_header_offset = base_ota_offset + sizeof(wiced_ota2_image_header_t) + (component_index * sizeof(wiced_ota2_image_component_t));
    if (wiced_ota2_read_sflash( component_header_offset, (uint8_t*)component_header, sizeof(wiced_ota2_image_header_t)) != WICED_SUCCESS)
    {
        /* read header fail */
        OTA2_WPRINT_INFO(("wiced_ota_read_component_header() read Failed!\r\n"));
        return WICED_ERROR;
    }
    wiced_ota2_image_component_header_swap_network_order(component_header, WICED_OTA2_IMAGE_SWAP_NETWORK_TO_HOST);
    return WICED_SUCCESS;
}

static wiced_result_t wiced_ota2_image_copy_uncompressed_component(wiced_ota2_image_type_t ota_type,
                                                                  wiced_ota2_image_header_t* ota2_header,
                                                                  wiced_ota2_image_component_t* component_header)
{
    wiced_result_t  result = WICED_SUCCESS;
    uint32_t    pos, chunk_size, base_ota_offset;
    uint32_t    src_offset;
    uint32_t    src_size;
    uint32_t    dst_offset;
    uint16_t    progress = 0;
    OTA2_CRC_VAR crc_check;

    base_ota_offset = wiced_ota2_image_get_offset( ota_type );
    if (base_ota_offset == 0)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_copy_uncompressed_component(%s) get_address() failed\r\n", ota2_image_type_name_string[ota_type]));
        return WICED_BADARG;
    }

    if ((ota2_header == NULL) || (component_header == NULL))
    {
        OTA2_WPRINT_INFO(("wiced_ota2_copy_uncompressed_component() bad args\r\n"));
        return WICED_BADARG;
    }

    /* point to the component in the OTA image file and the destination */
    src_offset = (base_ota_offset + ota2_header->data_start + component_header->source_offset);
    src_size   = component_header->source_size;
    dst_offset = (OTA2_IMAGE_FLASH_BASE + component_header->destination);

    pos = 0;
    crc_check = OTA2_CRC_INIT_VALUE;
    while ( pos < src_size )
    {
        chunk_size = ((src_size - pos) > SECTOR_SIZE ) ? SECTOR_SIZE : ( src_size - pos );

        /* todo: get first chunk to hit the end of a sector so all other reads/writes go faster? */

        memset(component_decomp_ram, 0x00, chunk_size);

        if (wiced_ota2_read_sflash( (unsigned long)(src_offset + pos), component_decomp_ram, chunk_size) != WICED_SUCCESS)
        {
            OTA2_WPRINT_INFO(("wiced_ota2_copy_uncompressed_component() read component chunk FAILED!\r\n"));
            result = WICED_ERROR;
            goto _no_comp_end;
        }

        crc_check = OTA2_CRC_FUNCTION(component_decomp_ram, chunk_size, crc_check);

        if (wiced_ota2_safe_write_data_to_sflash( (dst_offset + pos), component_decomp_ram, chunk_size) != WICED_SUCCESS)
        {
            result = WICED_ERROR;
            goto _no_comp_end;
        }

        if ((progress++ % 0x03) == 0)
        {
            OTA2_WPRINT_INFO(("."));
        }

        pos += chunk_size;
    } /* while pos < src_size */

_no_comp_end:

    if (component_header->crc != crc_check)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_copy_uncompressed_component() CRC Failed (expected -> 0x%lx != 0x%lx <- actual) size:%ld %ld %s.\r\n",
                         (uint32_t)component_header->crc, (uint32_t)crc_check,
                         component_header->source_size, component_header->destination_size,
                         component_header->name));
        result = WICED_ERROR;
    }

    return result;
}

/******************************************************
 *               External Functions
 ******************************************************/

/**
 * Validate OTA Image
 *
 * @param[in]  ota_type     - OTA Image type
 *
 * @return WICED_SUCCESS
 *         WICED_ERROR      - Bad OTA Image
 *         WICED_BADARG     - NULL pointer passed in or bad size
 */
wiced_result_t wiced_ota2_image_validate ( wiced_ota2_image_type_t ota_type )
{
    wiced_ota2_image_header_t    ota2_header;
    wiced_ota2_image_component_t component_header;
    uint16_t                     index;
    uint32_t                     offset, size;
    OTA2_CRC_VAR                 header_crc;     /* for calculating the CRC */
    OTA2_CRC_VAR                 ota2_header_crc; /* The OTA header CRC before we clear it to re-compute */
    wiced_bool_t                 print_header;

    if (wiced_ota2_read_ota2_header(ota_type, &ota2_header) != WICED_SUCCESS)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_image_validate() wiced_ota2_read_ota2_header() fail.\r\n"));
        return WICED_ERROR;
    }

    /* check magic string */
    if (strncmp((char*)ota2_header.magic_string, WICED_OTA2_IMAGE_MAGIC_STRING, WICED_OTA2_IMAGE_MAGIC_STR_LEN) != 0)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_image_validate() OTA Image Magic String fail.\r\n"));
        return WICED_ERROR;
    }

    /* OTA version */
    if (ota2_header.ota2_version != WICED_OTA2_IMAGE_VERSION)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_image_validate() OTA Image version fail.\r\n"));
        return WICED_ERROR;
    }

    /* check hardware platform */
    if (strcmp((char *)ota2_header.platform_name, PLATFORM) != 0)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_image_validate() OTA Image Platform type fail. %s != %s \r\n", ota2_header.platform_name, PLATFORM));
        return WICED_UNSUPPORTED;
    }

    /* check component count */
    if ((ota2_header.component_count == 0) || (ota2_header.component_count > DCT_MAX_APP_COUNT))
    {
        OTA2_WPRINT_INFO(("wiced_ota2_image_validate() OTA Image component count fail. %d\r\n", ota2_header.component_count));
        return WICED_ERROR;
    }

    /* check downloaded bytes vs. expected image size  */
    if (ota2_header.bytes_received < ota2_header.image_size)
    {
        /* this is normal while we are downloading  */
        if (ota2_header.download_status != WICED_OTA2_IMAGE_DOWNLOAD_IN_PROGRESS)
        {
            OTA2_WPRINT_DEBUG(("wiced_ota2_image_validate() OTA Image size error: expected -> %ld != %ld <- actual\r\n", ota2_header.bytes_received, ota2_header.image_size));
            return WICED_ERROR;
        }
    }

    print_header = WICED_TRUE;

    switch( (wiced_ota2_image_status_t)ota2_header.download_status )
    {
        case WICED_OTA2_IMAGE_DOWNLOAD_IN_PROGRESS:
            if (ota2_header.bytes_received < ota2_header.data_start)
            {
                /* not enough bytes to check the header CRC, assume all is going well */
                return WICED_SUCCESS;
            }
            print_header = WICED_FALSE;
            break;
        default:
        case WICED_OTA2_IMAGE_INVALID:
        case WICED_OTA2_IMAGE_DOWNLOAD_FAILED:
        case WICED_OTA2_IMAGE_DOWNLOAD_UNSUPPORTED:
        case WICED_OTA2_IMAGE_DOWNLOAD_EXTRACTED:
            /* if there is a problem, or we haven't finished loading, don't print the components */
            return WICED_ERROR;
            break;
        case WICED_OTA2_IMAGE_VALID:
        case WICED_OTA2_IMAGE_DOWNLOAD_COMPLETE:
        case WICED_OTA2_IMAGE_EXTRACT_ON_NEXT_BOOT:
            break;
    }

    if (print_header == WICED_TRUE)
    {
        wiced_ota2_print_header_info(&ota2_header, "OTA Image Validate:");
    }

     /* prepare for Header CRC check
     * We do not need to save these values here, as they are local
     */
    /* save this so we can check it ! */
    ota2_header_crc = ota2_header.header_crc;
    ota2_header.header_crc = 0;

    /* only clear this out for non-FACTORY_RESET images */
    if (ota_type != WICED_OTA2_IMAGE_TYPE_FACTORY_RESET_APP)
    {
        ota2_header.download_status = 0;
        ota2_header.bytes_received = 0;
    }

    header_crc = OTA2_CRC_INIT_VALUE;
    header_crc = OTA2_CRC_FUNCTION((uint8_t*)&ota2_header, sizeof(wiced_ota2_image_header_t), header_crc);

    /* check that the component sizes are reasonable (the source size fits in the image) */
    for (index = 0; index < ota2_header.component_count; index++)
    {
        /* read the component header */
        if (wiced_ota_read_component_header( ota_type, index, &component_header) != WICED_SUCCESS)
        {
            OTA2_WPRINT_INFO(("wiced_ota2_image_validate() wiced_ota_read_component_header() failed. \r\n"));
            return WICED_ERROR;
        }

        if (print_header == WICED_TRUE)
        {
            wiced_ota2_print_component_info(&component_header);
        }

        /* if source size is greater than destination, we have a problem! */
        if (component_header.source_size > component_header.destination_size)
        {
            OTA2_WPRINT_INFO(("wiced_ota2_image_validate() OTA Image component %ld source size > dest %ld \r\n",
                             component_header.source_size, component_header.destination_size));
            return WICED_ERROR;
        }

        offset = component_header.source_offset;   /* offset from start of data (after components in OTA Image) */
        size   =  component_header.source_size;    /* size in the in OTA Image */
        /* if component offset + size is greater than the total image size, this is bad */
        if ((ota2_header.data_start + offset + size) > ota2_header.image_size)
        {
            OTA2_WPRINT_INFO(("wiced_ota2_image_validate() OTA Image component source + size > image size %ld > %ld\r\n",
                           (ota2_header.data_start + offset + size), ota2_header.image_size));
            return WICED_ERROR;
        }

        header_crc = OTA2_CRC_FUNCTION((uint8_t*)&component_header, sizeof(wiced_ota2_image_component_t), header_crc);
    }

    /* check crc value */
    if (ota2_header_crc != header_crc)
    {
        OTA2_WPRINT_INFO(("\r\nwiced_ota2_image_validate() computed CRC 0x%lx does not match OTA Header CRC 0x%lx \r\n\r\n",
                        header_crc, ota2_header_crc));
        return WICED_ERROR;
    }

    // TODO: other checks?
//    dst_addr = component_header.destination;
//    dst_size = component_header.destination_size;

    return WICED_SUCCESS;

}

/**
 * Get status of OTA Image at download location
 *
 * @param[in]  ota_type     - OTA Image type
 * @param[out] status       - Receives the OTA Image status.
 *
 * @return WICED_SUCCESS
 *         WICED_ERROR      - Bad OTA Image
 *         WICED_BADARG     - NULL pointer passed in
 */
wiced_result_t wiced_ota2_image_get_status ( wiced_ota2_image_type_t ota_type, wiced_ota2_image_status_t *status )
{
    wiced_ota2_image_header_t    ota2_header;

    if (status == NULL)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_get_status() bad arg.\r\n"));
        return WICED_BADARG;
    }

    if (ota_type == WICED_OTA2_IMAGE_TYPE_CURRENT_APP)  // TODO: test crc from LUT
    {
        OTA2_WPRINT_DEBUG(("wiced_ota2_get_status() Current App - OK for now, TODO: check CRC when added to apps_lut!\r\n"));
        *status = WICED_OTA2_IMAGE_VALID;
        return WICED_SUCCESS;
    }

    if (wiced_ota2_read_ota2_header(ota_type, &ota2_header) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    /* return the status */
    *status = ota2_header.download_status;

    switch (*status)
    {
        case WICED_OTA2_IMAGE_DOWNLOAD_IN_PROGRESS:
            OTA2_WPRINT_INFO(("wiced_ota2_get_status(%d) Download in progress.\r\n", ota_type));
            break;
        case WICED_OTA2_IMAGE_DOWNLOAD_FAILED:
            OTA2_WPRINT_INFO(("wiced_ota2_get_status(%d) Download FAILED!\r\n", ota_type));
            break;
        case WICED_OTA2_IMAGE_DOWNLOAD_COMPLETE:
            OTA2_WPRINT_INFO(("wiced_ota2_get_status(%d) Download complete. Ready for extract.\r\n", ota_type));
            break;
        case WICED_OTA2_IMAGE_VALID:
            OTA2_WPRINT_INFO(("wiced_ota2_get_status(%d) Download valid.\r\n", ota_type));
            break;
        case WICED_OTA2_IMAGE_EXTRACT_ON_NEXT_BOOT:
            OTA2_WPRINT_INFO(("wiced_ota2_get_status(%d) Download complete. Extract on Reboot.\r\n", ota_type));
            break;
        case WICED_OTA2_IMAGE_DOWNLOAD_EXTRACTED:
            OTA2_WPRINT_INFO(("wiced_ota2_get_status(%d) Download complete. Download extracted (Used).\r\n", ota_type));
            break;
        case WICED_OTA2_IMAGE_INVALID:
        default:
            OTA2_WPRINT_INFO(("wiced_ota2_get_status(%d) Download invalid.\r\n", ota_type));
            break;
    }
    return WICED_SUCCESS;
}

/**
 * Extract OTA Image
 * NOTE: All information regarding location of data in the system is part of the OTA Image.
 *
 * @param[in]  ota_type     - OTA Image type
 *
 * @return WICED_SUCCESS
 *         WICED_ERROR      - Bad OTA Image, not fully downloaded
 *         WICED_BADARG     - NULL pointer passed in or bad size
 */
wiced_result_t wiced_ota2_image_extract ( wiced_ota2_image_type_t ota_type )
{
    uint16_t                    index;
    wiced_ota2_image_header_t   ota2_header;
    wiced_ota2_image_status_t   status;
    wiced_result_t              result;

    /* validate the image */
    if (wiced_ota2_image_validate(ota_type) != WICED_SUCCESS)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_extract(%s) validate() failed\r\n", ota2_image_type_name_string[ota_type]));
        return WICED_BADARG;
    }

    /* check download status */
    wiced_ota2_image_get_status ( ota_type, &status );
    if ( (status != WICED_OTA2_IMAGE_DOWNLOAD_COMPLETE) && (status != WICED_OTA2_IMAGE_EXTRACT_ON_NEXT_BOOT) &&
         (status != WICED_OTA2_IMAGE_VALID))
    {
        return WICED_ERROR;
    }

#ifdef CHECK_BATTERY_LEVEL_BEFORE_OTA2_UPGRADE
    /* check for battery level before doing any writing! - todo: make this a platform-specific function*/
     if (platform_check_battery_level(CHECK_BATTERY_LEVEL_OTA2_UPGRADE_MINIMUM) != WICED_SUCCESS)
     {
         OTA2_WPRINT_INFO(("check_battery_level() failed\r\n"));
         return WICED_ERROR;
     }
#endif /* CHECK_BATTERY_LEVEL_BEFORE_OTA2_UPGRADE */

     if (wiced_ota2_read_ota2_header(ota_type, &ota2_header) != WICED_SUCCESS)
     {
         return WICED_ERROR;
     }

     OTA2_WPRINT_INFO(("\r\n"));

    /* Extract the components and check CRCs while doing so - bail out if any one component fails */
     result = WICED_SUCCESS;
    for (index = 0; index < ota2_header.component_count; index++)
    {
#ifdef  DEBUG_GET_DECOMP_TIME
        wiced_time_t    start_time, end_time;
#endif
        wiced_ota2_image_component_t  component_header;

        /* read the component header */
        if (wiced_ota_read_component_header( ota_type, index, &component_header) != WICED_SUCCESS)
        {
            OTA2_WPRINT_INFO(("wiced_ota2_extract() wiced_ota_read_component_header() failed. \r\n"));
            return WICED_ERROR;
        }

        wiced_ota2_print_component_info(&component_header);

        OTA2_WPRINT_DEBUG(("extracting %s:", component_header.name));

#ifdef  DEBUG_GET_DECOMP_TIME
        wiced_time_get_time( &start_time );
#endif
        /* decompress component and write to FLASH */
        /* need to do this in chunks  !!! The file system is > 800k !!! */
        switch (component_header.compression)  // TODO
        {
            case WICED_OTA2_IMAGE_COMPONENT_COMPRESSION_LZW:   // TODO
                OTA2_WPRINT_INFO(("wiced_ota2_extract() LZW COMPRESSION UNSUPPORTED .\r\n"));
                result = WICED_ERROR;
                break;
            case WICED_OTA2_IMAGE_COMPONENT_COMPRESSION_GZIP:  // TODO
                OTA2_WPRINT_INFO(("wiced_ota2_extract() GZIP COMPRESSION UNSUPPORTED .\r\n"));
                result = WICED_ERROR;
                break;
            case WICED_OTA2_IMAGE_COMPONENT_COMPRESSION_BZ2:   // TODO
                OTA2_WPRINT_INFO(("wiced_ota2_extract() BZ2 COMPRESSION UNSUPPORTED .\r\n"));
                result = WICED_ERROR;
                break;
            case WICED_OTA2_IMAGE_COMPONENT_COMPRESSION_NONE:
                result = wiced_ota2_image_copy_uncompressed_component( ota_type, &ota2_header, &component_header);
                break;
            default:
                OTA2_WPRINT_INFO(("wiced_ota2_extract() UNDEFINED COMPRESSION UNSUPPORTED .\r\n"));
                result = WICED_ERROR;
                break;
        } /* switch */

#ifdef  DEBUG_GET_DECOMP_TIME
        wiced_time_get_time( &end_time );
        OTA2_WPRINT_DEBUG(("\r\ntime: %ld\r\n", (end_time - start_time)));
#endif
        OTA2_WPRINT_DEBUG(("\r\n"));

    }   /* for component_count */

    /* we always want to leave the Factory Reset Application alone - no touchy-touchy! */
    if (ota_type != WICED_OTA2_IMAGE_TYPE_FACTORY_RESET_APP)
    {
        wiced_ota2_image_update_staged_status(WICED_OTA2_IMAGE_DOWNLOAD_EXTRACTED);
    }

    return result;
}

/**
 * Write OTA Image to the Staging area
 * NOTE: The total size of the OTA image is included in a valid OTA image header.
 *       This function will update the status as appropriate
 *
 * @param[in]  data      - pointer to part or all of an OTA image to be stored in the staging area
 * @param[in]  offset    - offset from start of OTA Image to store this data
 * @param[in]  size      - size of the data to store
 *
 * @return - WICED_SUCCESS
 *           WICED_ERROR
 *           WICED_BADARG
 */
wiced_result_t  wiced_ota2_image_write_data(uint8_t* data, uint32_t offset, uint32_t size)
{
    wiced_result_t              result;
    wiced_ota2_image_header_t   ota2_header;

    if ((data == NULL) || (size == 0))
    {
        OTA2_WPRINT_INFO(("wiced_ota2_write() Bad Args data:%p size:%ld!\r\n", data, size));
        return WICED_BADARG;
    }

    /* We always write to the staging area, make sure we don't go past the area size */
    if ( (offset + size) > OTA2_IMAGE_STAGING_AREA_SIZE)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_write() offset:0x%lx + size:0x%lx larger than staging area: 0x%x!\r\n", offset, size, OTA2_IMAGE_STAGING_AREA_SIZE));
        return WICED_BADARG;
    }

    if (wiced_ota2_safe_write_data_to_sflash( (wiced_ota2_image_get_offset(WICED_OTA2_IMAGE_TYPE_STAGED) + offset), data, size) != WICED_SUCCESS)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_write() wiced_ota2_safe_write_data_to_sflash() failed!\r\n"));
        return WICED_ERROR;
    }

    if (wiced_ota2_read_ota2_header(WICED_OTA2_IMAGE_TYPE_STAGED, &ota2_header) != WICED_SUCCESS)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_write() wiced_ota2_read_ota2_header() failed!\r\n"));
        return WICED_ERROR;
    }

    /* update the header info */
    result = wiced_ota2_image_update_staged_header(ota2_header.bytes_received + size);
    if (result == WICED_UNSUPPORTED)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_write() wiced_ota2_update_staged_header() OTA image not supported!\r\n"));
        return WICED_UNSUPPORTED;
    }

    if (result != WICED_SUCCESS)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_write() wiced_ota2_update_staged_header() failed!\r\n"));
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

/** Update the OTA image header after writing (parts of) the downloaded OTA image to FLASH
 *
 * @param total_bytes_received - number of bytes written to the image
 *
 * @return  WICED_SUCCESS
 *          WICED_BADARG
 *          WICED_ERROR
 */
wiced_result_t wiced_ota2_image_update_staged_header(uint32_t total_bytes_received)
{
    wiced_result_t              result;
    wiced_ota2_image_header_t   ota2_header;

    if (wiced_ota2_read_ota2_header(WICED_OTA2_IMAGE_TYPE_STAGED, &ota2_header) != WICED_SUCCESS)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_update_staged_header() wiced_ota2_read_ota2_header() failed!\r\n"));
        return WICED_ERROR;
    }

    if (total_bytes_received > ota2_header.image_size)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_update_staged_header() total_bytes_received %ld > %ld ota2_header.image_size!\r\n", total_bytes_received, ota2_header.image_size));
        return WICED_ERROR;
    }

    ota2_header.bytes_received = total_bytes_received;

    /* assume in progress */
    ota2_header.download_status = WICED_OTA2_IMAGE_DOWNLOAD_IN_PROGRESS;
    if (ota2_header.bytes_received == 0)
    {
        ota2_header.download_status = WICED_OTA2_IMAGE_INVALID;
        OTA2_WPRINT_INFO(("wiced_ota2_update_staged_header() ota2_header.bytes_received == 0!\r\n"));
    }
    else if (ota2_header.bytes_received >= ota2_header.image_size)
    {
        ota2_header.download_status = WICED_OTA2_IMAGE_DOWNLOAD_COMPLETE;
    }

    if (wiced_ota2_write_ota2_header(WICED_OTA2_IMAGE_TYPE_STAGED, &ota2_header) != WICED_SUCCESS)
    {
        OTA2_WPRINT_INFO(("wiced_ota2_update_staged_header() wiced_ota2_write_ota2_header() FAILED!\r\n"));
        return WICED_ERROR;
    }

    /* verify that the image is valid, if not, mark as failed */
    result = wiced_ota2_image_validate(WICED_OTA2_IMAGE_TYPE_STAGED);
    if (result == WICED_UNSUPPORTED)    /* result from validate */
    {
        ota2_header.download_status = WICED_OTA2_IMAGE_DOWNLOAD_UNSUPPORTED;
    }
    else if (result != WICED_SUCCESS)
    {
        ota2_header.download_status = WICED_OTA2_IMAGE_DOWNLOAD_FAILED;
    }

    return result;
}

/** Update the OTA image header status
 *
 * @param total_bytes_received - number of bytes written to the image
 *
 * @return  WICED_SUCCESS
 *          WICED_BADARG
 *          WICED_ERROR
 */
wiced_result_t wiced_ota2_image_update_staged_status(wiced_ota2_image_status_t new_status)
{
    wiced_ota2_image_header_t    ota2_header;

    if (wiced_ota2_read_ota2_header(WICED_OTA2_IMAGE_TYPE_STAGED, &ota2_header) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    ota2_header.download_status = new_status;

    if (wiced_ota2_write_ota2_header(WICED_OTA2_IMAGE_TYPE_STAGED, &ota2_header) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    /* If we are complete, verify that the image is valid, if not, mark as failed */
    if (ota2_header.download_status == WICED_OTA2_IMAGE_DOWNLOAD_COMPLETE)
    {
        /* verify that the image is valid, if not, mark as failed */
        if (wiced_ota2_image_validate(WICED_OTA2_IMAGE_TYPE_STAGED) != WICED_SUCCESS)
        {
            ota2_header.download_status = WICED_OTA2_IMAGE_DOWNLOAD_FAILED;
            if (wiced_ota2_write_ota2_header(WICED_OTA2_IMAGE_TYPE_STAGED, &ota2_header) != WICED_SUCCESS)
            {
                return WICED_ERROR;
            }
        }
    }

    /* read and print out the header info */
    if (wiced_ota2_read_ota2_header(WICED_OTA2_IMAGE_TYPE_STAGED, &ota2_header) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    wiced_ota2_print_header_info(&ota2_header, "OTA Image Header after Updating OTAImage header:");
    return WICED_SUCCESS;

}

/**
 * Get the OTA Image offset into the SFLASH
 *
 * @param[in]  ota_type     - OTA Image type
 *
 * @return - offset to the ota image header
 *           0 if bad ota_type
 */
uint32_t wiced_ota2_image_get_offset( wiced_ota2_image_type_t ota_type )
{
    uint32_t   ota_offset = 0;
    switch (ota_type)
    {
        case WICED_OTA2_IMAGE_TYPE_FACTORY_RESET_APP:
            ota_offset = OTA2_IMAGE_FACTORY_RESET_AREA_BASE;
            break;
        case WICED_OTA2_IMAGE_TYPE_CURRENT_APP:
            ota_offset = OTA2_IMAGE_CURRENT_AREA_BASE;
            break;
        case WICED_OTA2_IMAGE_TYPE_STAGED:
            ota_offset = OTA2_IMAGE_STAGING_AREA_BASE;
            break;
        case WICED_OTA2_IMAGE_TYPE_LAST_KNOWN_GOOD:
#if defined(OTA2_IMAGE_LAST_KNOWN_GOOD_AREA_BASE)
            ota_offset= OTA2_IMAGE_LAST_KNOWN_GOOD_AREA_BASE;
            break;
#endif
        case WICED_OTA2_IMAGE_TYPE_NONE:
        default:
            OTA2_WPRINT_INFO(("wiced_ota2_image_get_offset() UNKNOWN OTA_TYPE. \r\n"));
            break;
    }
    return ota_offset;
}

/** Get the last boot type
 * NOTE: Also sets boot type to OTA2_BOOT_NORMAL
 * @param   N/A
 *
 * @return  ota2_boot_type_t
 */
ota2_boot_type_t wiced_ota2_get_boot_type( void )
{
    ota2_boot_type_t boot_type = OTA2_BOOT_NORMAL;

    /* Did User cause a Factory Reset ? */
    platform_dct_ota2_config_t  ota2_header;

    /* is saved DCT valid ? */
    if (wiced_dct_ota2_read_saved_copy( &ota2_header, DCT_OTA2_CONFIG_SECTION, 0, sizeof(platform_dct_ota2_config_t)) == WICED_SUCCESS)
    {
        boot_type = ota2_header.boot_type;
    }
    else
    {
        boot_type = OTA2_BOOT_NEVER_RUN_BEFORE;
    }

    return boot_type;
}

/* DEBUGGING - fake that we downloaded the image
 * download the image manually before calling this...
 */
wiced_result_t wiced_ota2_image_fakery(wiced_ota2_image_status_t new_status)
{
    wiced_ota2_image_header_t    ota2_header;

    if (wiced_ota2_read_ota2_header(WICED_OTA2_IMAGE_TYPE_STAGED, &ota2_header) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    ota2_header.bytes_received = ota2_header.image_size;
    ota2_header.download_status = new_status;

    if (wiced_ota2_write_ota2_header(WICED_OTA2_IMAGE_TYPE_STAGED, &ota2_header) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    wiced_ota2_print_header_info(&ota2_header, "OTA Image Header after FAKING IT OTAImage header:");
    return WICED_SUCCESS;
}

