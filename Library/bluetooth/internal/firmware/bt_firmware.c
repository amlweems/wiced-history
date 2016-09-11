/**
 * Copyright 2014, Broadcom Corporation
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
#include "wiced.h"
#include "wiced_rtos.h"
#include "wiced_utilities.h"
#include "bt_bus.h"
#include "bt_hci_interface.h"
#include "bt_firmware_image.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define DEFAULT_READ_TIMEOUT 100

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

/******************************************************
 *               Variables Definitions
 ******************************************************/

static const hci_command_header_t const hci_commands[] =
{
    [HCI_CMD_RESET]               = { .packet_type = 0x1, .op_code = HCI_CMD_OPCODE_RESET,               .content_length = 0x0 },
    [HCI_CMD_DOWNLOAD_MINIDRIVER] = { .packet_type = 0x1, .op_code = HCI_CMD_OPCODE_DOWNLOAD_MINIDRIVER, .content_length = 0x0 },
    [HCI_CMD_WRITE_RAM]           = { .packet_type = 0x1, .op_code = HCI_CMD_OPCODE_WRITE_RAM,           .content_length = 0x0 },
    [HCI_CMD_LAUNCH_RAM]          = { .packet_type = 0x1, .op_code = HCI_CMD_OPCODE_LAUNCH_RAM,          .content_length = 0x0 },
};

static const hci_event_extended_header_t const expected_hci_events[] =
{
    [HCI_CMD_RESET]               = { .header = {.packet_type = 0x4, .event_code = 0xE, .content_length = 0x4 }, .total_packets = 0x1, .op_code = HCI_CMD_OPCODE_RESET,               .status = 0x0 },
    [HCI_CMD_DOWNLOAD_MINIDRIVER] = { .header = {.packet_type = 0x4, .event_code = 0xE, .content_length = 0x4 }, .total_packets = 0x1, .op_code = HCI_CMD_OPCODE_DOWNLOAD_MINIDRIVER, .status = 0x0 },
    [HCI_CMD_WRITE_RAM]           = { .header = {.packet_type = 0x4, .event_code = 0xE, .content_length = 0x4 }, .total_packets = 0x1, .op_code = HCI_CMD_OPCODE_WRITE_RAM,           .status = 0x0 },
    [HCI_CMD_LAUNCH_RAM]          = { .header = {.packet_type = 0x4, .event_code = 0xE, .content_length = 0x4 }, .total_packets = 0x1, .op_code = HCI_CMD_OPCODE_LAUNCH_RAM,          .status = 0x0 },
};

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

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t bt_firmware_download( const uint8_t* firmware_image, uint32_t size, const char* version )
{
    uint8_t*       data             = (uint8_t*)firmware_image;
    uint32_t       remaining_length = size;
    wiced_result_t retval           = WICED_SUCCESS;
    hci_event_extended_header_t hci_event;

    if ( bt_bus_is_ready() == WICED_FALSE )
    {
        WPRINT_LIB_DEBUG( ( "Bluetooth bus is NOT ready!\n\r" ) );
        retval = WICED_ERROR;
        goto exit;
    }

    WPRINT_LIB_DEBUG( ( "Sending hci_reset ... \n\r" ) );

    if ( bt_bus_transmit( (const uint8_t*) &hci_commands[HCI_CMD_RESET], sizeof(hci_command_header_t) ) != WICED_SUCCESS )
    {
        WPRINT_LIB_DEBUG( ( "failed!\n\r" ) );
        retval = WICED_ERROR;
        goto exit;
    }

    /* First reset command requires extra delay between write and read */
    if ( bt_bus_receive( (uint8_t*) &hci_event, sizeof( hci_event ), 1000 ) != WICED_SUCCESS )
    {
        WPRINT_LIB_DEBUG( ( "no response!\n\r" ) );
        retval = WICED_ERROR;
        goto exit;
    }

    if ( memcmp( &hci_event, &expected_hci_events[HCI_CMD_RESET], sizeof( hci_event ) ) != 0 )
    {
        WPRINT_LIB_DEBUG( ( "unexpected response!\n\r" ) );
        retval = WICED_ERROR;
        goto exit;
    }

    WPRINT_LIB_DEBUG( ( "done!\n\r" ) );

    WPRINT_LIB_DEBUG( ( "Sending hci_download_minidriver ... \n\r" ) );

    /* Send hci_download_minidriver command */
    if ( bt_bus_transmit( (const uint8_t*) &hci_commands[HCI_CMD_DOWNLOAD_MINIDRIVER], sizeof(hci_command_header_t) ) != WICED_SUCCESS )
    {
        WPRINT_LIB_DEBUG( ( "failed!\n\r" ) );
        retval = WICED_ERROR;
        goto exit;
    }

    if ( bt_bus_receive( (uint8_t*) &hci_event, sizeof( hci_event ), DEFAULT_READ_TIMEOUT ) != WICED_SUCCESS )
    {
        WPRINT_LIB_DEBUG( ( "no response!\n\r" ) );
        retval = WICED_ERROR;
        goto exit;
    }

    /* If event is as expected, download firmware */
    if ( memcmp( &hci_event, &expected_hci_events[HCI_CMD_DOWNLOAD_MINIDRIVER], sizeof( hci_event ) ) != 0 )
    {
        WPRINT_LIB_DEBUG( ( "unexpected response!\n\r" ) );
        retval = WICED_ERROR;
        goto exit;
    }

    WPRINT_LIB_DEBUG( ( "done!\n\r" ) );

    WPRINT_LIB_DEBUG( ( "Uploading %s ... \n\r", version ) );

    /* The firmware image (.hcd format) contains a collection of hci_write_ram command + a block of the image,
     * followed by a hci_write_ram image at the end. Parse and send each individual command and wait for the response.
     * This is to ensure the integrity of the firmware image sent to the bluetooth chip.
     */
    while ( remaining_length )
    {
        uint32_t data_length   = data[2] + 3;
        uint8_t  residual_data = 0;
        hci_command_opcode_t command_opcode = *(hci_command_opcode_t*) data;

        /* Send hci_write_ram command. The length of the data immediately follows the command opcode */
        if ( bt_bus_transmit( (const uint8_t*) data, data_length ) != WICED_SUCCESS )
        {
            WPRINT_LIB_DEBUG( ("failed!\n\r") );
            retval = WICED_ERROR;
            goto exit;
        }

        /* Data sent successfully. Wait for response */
        memset( &hci_event, 0, sizeof( hci_event ) );
        if ( bt_bus_receive( (uint8_t*) &hci_event, sizeof(hci_event), DEFAULT_READ_TIMEOUT ) != WICED_SUCCESS )
        {
            WPRINT_LIB_DEBUG( ("no response! remaining length: %d\n\r", (unsigned int)remaining_length) );
            retval = WICED_ERROR;
            goto exit;
        }

        switch ( command_opcode )
        {
            case HCI_CMD_OPCODE_WRITE_RAM:
                if ( memcmp( &hci_event, &expected_hci_events[HCI_CMD_WRITE_RAM], sizeof( hci_event ) ) != 0 )
                {
                    WPRINT_LIB_DEBUG( ("unexpected hci_write_ram response!\n\r") );
                    retval = WICED_ERROR;
                    goto exit;
                }

                /* Update remaining length and data pointer */
                data += data_length;
                remaining_length -= data_length;
                break;

            case HCI_CMD_OPCODE_LAUNCH_RAM:
                if ( memcmp( &hci_event, &expected_hci_events[HCI_CMD_LAUNCH_RAM], sizeof( hci_event ) ) != 0 )
                {
                    WPRINT_LIB_DEBUG( ("unexpected hci_launch_ram response!\n\r") );
                    retval = WICED_ERROR;
                    goto exit;
                }

                /* All responses have been read. Now let's flush residual data if any and reset remaining length */
                while ( bt_bus_receive( &residual_data, sizeof( residual_data ), DEFAULT_READ_TIMEOUT ) == WICED_SUCCESS )
                {

                }

                remaining_length = 0;
                break;

            default:
                retval = WICED_ERROR;
                goto exit;
        }
    }

    WPRINT_LIB_DEBUG( ( "done!\n\r" ) );

    /* Wait for bluetooth chip to pull its RTS (host's CTS) low. From observation using CRO, it takes the bluetooth chip > 170ms to pull its RTS low after CTS low */
    while ( bt_bus_is_ready( ) == WICED_FALSE )
    {
        wiced_rtos_delay_milliseconds( 10 );
    }

    exit:
    return retval;
}
