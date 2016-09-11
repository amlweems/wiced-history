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
#include "wwd_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#ifndef PRIVATE_KEY_SIZE
#define PRIVATE_KEY_SIZE  (2*1024)
#endif

#ifndef CERTIFICATE_SIZE
#define CERTIFICATE_SIZE  (4*1024)
#endif

#ifndef CONFIG_AP_LIST_SIZE
#define CONFIG_AP_LIST_SIZE   (5)
#endif

#define CONFIG_VALIDITY_VALUE        0xCA1BDF58

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/
#pragma pack(1)

typedef void (*dct_load_app_func_t)(void);

typedef struct
{
    unsigned long full_size;
    unsigned long used_size;
    char write_incomplete;
    char is_current_dct;
    char app_valid;
    char mfg_info_programmed;
    unsigned long magic_number;
    dct_load_app_func_t load_app_func;
} platform_dct_header_t;

typedef struct
{
    char manufacturer[32];
    char product_name[32];
    char BOM_name[24];
    char BOM_rev[8];
    char serial_number[20];
    char manufacture_date_time[20];
    char manufacture_location[12];
    char bootloader_version[8];
} platform_dct_mfg_info_t;

typedef struct
{
    wiced_ap_info_t details;
    uint8_t         security_key_length;
    char            security_key[64];
} wiced_config_ap_entry_t;

typedef struct
{
    wiced_ssid_t     SSID;
    wiced_security_t security;
    uint8_t          channel;
    uint8_t          security_key_length;
    char             security_key[64];
    uint32_t         details_valid;
} wiced_config_soft_ap_t;

typedef struct
{
    wiced_bool_t              device_configured;
    wiced_config_ap_entry_t   stored_ap_list[CONFIG_AP_LIST_SIZE];
    wiced_config_soft_ap_t    soft_ap_settings;
    wiced_config_soft_ap_t    config_ap_settings;
    wiced_country_code_t      country_code;
    wiced_mac_t               mac_address;
} platform_dct_wifi_config_t;

typedef struct
{
    char    private_key[PRIVATE_KEY_SIZE];
    char    certificate[CERTIFICATE_SIZE];
    uint8_t cooee_key[16];
} platform_dct_security_t;

typedef struct
{
    platform_dct_header_t        dct_header;
    platform_dct_mfg_info_t      mfg_info;
    platform_dct_security_t      security_credentials;
    platform_dct_wifi_config_t   wifi_config;
} platform_dct_data_t;

#pragma pack()

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

extern void  platform_read_dct ( uint16_t offset, void* buffer, uint16_t buffer_length );
extern int   platform_write_dct( uint16_t data_start_offset, const void* data, uint16_t data_length, int8_t app_valid, void (*func)(void) );
extern platform_dct_data_t* platform_get_dct  ( void );

extern int platform_erase_flash( uint16_t start_sector, uint16_t end_sector );
extern int platform_write_flash_chunk( uint32_t address, const uint8_t* data, uint32_t size );
extern int platform_write_app_chunk( uint32_t offset, const uint8_t* data, uint32_t size );

#ifdef __cplusplus
} /* extern "C" */
#endif
