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
 *  Defines common constants and types for the WICED Bluetooth Framework
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

/** @cond !ADDTHIS*/
#define WICED_BT_ADDRESS_BYTE_SIZE 6
/** @endcond */

/******************************************************
 *                   Enumerations
 ******************************************************/

/**
 * UUID size
 */
typedef enum
{
    UUID_16BIT  = 0x02, /**< 16-bit  */
    UUID_32BIT  = 0x04, /**< 32-bit  */
    UUID_128BIT = 0x10  /**< 128-bit */
} wiced_bt_uuid_size_t;


/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

#if 0
/**
 * Bluetooth Device Address (BD_ADDR)
 * A 48-bit address that uniquely identifies a Bluetooth device
 */
typedef struct
{
    uint8_t address[WICED_BT_ADDRESS_BYTE_SIZE]; /**< Address. 48-bit Bluetooth device address in a little-endian format */
} wiced_bt_device_address_t;


/**
 * Universally Unique Identifier (UUID)
 * A standardised format of string ID that uniquely identifies a Bluetooth service
 */
typedef struct
{
    union
    {
        uint16_t value_16_bit;                  /**< 16-bit UUID value.     */
        uint16_t value_128_bit[UUID_128BIT/2];  /**< 128-bit UUID value.    */
    } value;                                    /**< A union of UUID values */

    wiced_bt_uuid_size_t size;                  /**< UUID size              */

} wiced_bt_uuid_t;
#endif

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif
