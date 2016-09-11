/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file Maxim17040 Library Functions
 *
 */

#include "max17040.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define RETRIES                 5
#define VOLTS_PER_VCELL_BIT     0.00125
#define VCELL_REGISTER_ADDRESS  0x02
#define SOC_REGISTER_ADDRESS    0x04
#define UPPER_VOLTAGE_LIMIT     4.20
#define LOWER_VOLTAGE_LIMIT     3.40

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

static uint32_t max_runtime = 0;

static wiced_i2c_device_t maxim17040 =
{
    .port          = WICED_I2C_1,
    .address       = 0x36,
    .address_width = I2C_ADDRESS_WIDTH_7BIT,
    .flags         = 0,
    .speed_mode    = I2C_HIGH_SPEED_MODE,
};

/******************************************************
 *               Function Definitions
 ******************************************************/

wiced_result_t max17040_initialize_i2c_device( void )
{
    if ( ( wiced_i2c_init( &maxim17040 ) != WICED_SUCCESS ) ||
         ( wiced_i2c_probe_device(&maxim17040, RETRIES) != WICED_TRUE ) )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

/* Sequence to read data from specified registers on maxim chips.
 *
 * This consists of [Start] [Write (slave address) (register address)] [Repeat-Start] [Read] [Stop].
 */
static wiced_result_t maxim_i2c_reg_read(uint8_t* write_buffer, uint8_t write_buffer_size, uint8_t* read_buffer, uint8_t read_buffer_size )
{
    /* Write slave address of maxim and register address to read from */
    if (wiced_i2c_write(&maxim17040, WICED_I2C_START_FLAG, write_buffer, write_buffer_size) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    /* Read bytes from maxim register address into read_buffer */
    if (wiced_i2c_read(&maxim17040, WICED_I2C_REPEATED_START_FLAG | WICED_I2C_STOP_FLAG, read_buffer, read_buffer_size) != WICED_SUCCESS)
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

/* Maxim17040 reports voltage in two bytes.
 * All 8 bits of the first byte are used, but only the upper 4 bits
 * of the second byte are used for a total of 12 bits.
 */
static float calculate_voltage( uint8_t register_value[2] )
{
    uint32_t voltage = 0;

    voltage |= register_value[0];
    voltage <<= 4;
    voltage |= register_value[1] >> 4;

    return (float) voltage * VOLTS_PER_VCELL_BIT;
}

float max17040_get_vcell_voltage( void )
{
    uint8_t vcell_reg = VCELL_REGISTER_ADDRESS;
    uint8_t bytes_read[2] = {0, 0};

    maxim_i2c_reg_read(&vcell_reg, 1, bytes_read, 2);

    return calculate_voltage(bytes_read);
}

uint8_t max17040_get_soc_percent( void )
{
    uint8_t soc_reg = SOC_REGISTER_ADDRESS;
    uint8_t bytes_read[2] = {0, 0};

    maxim_i2c_reg_read(&soc_reg, 1, bytes_read, 2);

    return bytes_read[0];
}

void max17040_set_max_runtime( uint32_t minutes )
{
    max_runtime = minutes;
}

float max17040_get_time_remaining( void )
{
    return ( max17040_get_vcell_voltage() - LOWER_VOLTAGE_LIMIT) * max_runtime / (UPPER_VOLTAGE_LIMIT - LOWER_VOLTAGE_LIMIT );
}
