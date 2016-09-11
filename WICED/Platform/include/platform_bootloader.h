/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include <stdint.h>
#include "platform_dct.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    APP_INVALID = 0,
    APP_VALID   = 1
} app_valid_t;

typedef enum boot_status_enum
{
    BOOTLOADER_BOOT_STATUS_START                   = 0x00,
    BOOTLOADER_BOOT_STATUS_APP_OK                  = 0x01,
    BOOTLOADER_BOOT_STATUS_WLAN_BOOTED_OK          = 0x02,
    BOOTLOADER_BOOT_STATUS_WICED_STARTED_OK        = 0x03,
    BOOTLOADER_BOOT_STATUS_DATA_CONNECTIVITY_OK    = 0x04
} boot_status_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct bootloader_api_struct
{
    /* General bootloader functions */
    int                         (*set_boot_status)             ( boot_status_t status );
    void                        (*perform_factory_reset)       ( void );    /* Void return type since function never returns */
    void                        (*start_ota_upgrade)           ( void );    /* Void return type since function never returns */
    int                         (*init_dct_mfg_info)           ( platform_dct_mfg_info_t* data_in );
    int                         (*write_app_config_dct)        ( unsigned long data_start_offset, void* data_in_addr,  unsigned long num_bytes_to_write );
    int                         (*write_wifi_config_dct)       ( unsigned long data_start_offset, void* data_in_addr,  unsigned long num_bytes_to_write );
    int                         (*write_certificate_dct)       ( unsigned long data_start_offset, void* data_in_addr,  unsigned long num_bytes_to_write );
    void*                       (*get_app_config_dct)          ( void );
    platform_dct_wifi_config_t* (*get_wifi_config_dct)         ( void );
    platform_dct_mfg_info_t*    (*get_mfg_info_dct)            ( void );
    platform_dct_security_t*    (*get_security_credentials_dct)( void );
    wiced_config_ap_entry_t*    (*get_ap_list_dct)             ( uint8_t index );
    int                         (*write_ap_list_dct)           ( uint8_t index, wiced_config_ap_entry_t* ap_details );
    wiced_config_soft_ap_t*     (*get_soft_ap_dct)             ( void );
    wiced_config_soft_ap_t*     (*get_config_ap_dct)           ( void );

    /* Platform specific bootloader functions */
    int                         (*platform_kick_watchdog)      ( void );
    void                        (*platform_reboot)             ( void );    /* Void return type since function never returns */
    int                         (*platform_erase_app)          ( void );
    int                         (*platform_write_app_chunk)    ( uint32_t offset, const uint8_t* data, uint32_t size );
    int                         (*platform_set_app_valid_bit)  ( app_valid_t val );
    int                         (*platform_read_wifi_firmware) ( uint32_t address, void* buffer, uint32_t requested_size, uint32_t* read_size );
    void                        (*platform_start_app)          ( uint32_t vector_table_address );
} bootloader_api_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

void          platform_reboot( void );
void          platform_start_app( uint32_t vector_table_address );
int           platform_erase_app( void );
extern int    platform_set_app_valid_bit( app_valid_t val );
int           platform_kick_watchdog( void );
extern int    platform_read_wifi_firmware ( uint32_t address, void* buffer, uint32_t requested_size, uint32_t* read_size );

void          platform_set_load_stack( void );
void          platform_set_bootloader_led( unsigned char on );
unsigned char platform_get_bootloader_button( void );
extern void   platform_restore_factory_app( void );
extern void   platform_load_ota_app       ( void );
extern void   platform_erase_dct( void );

#ifdef __cplusplus
} /* extern "C" */
#endif
