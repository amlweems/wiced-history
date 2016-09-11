/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * AK4961 I2C bus implementation.
 */

/** @file
 *
 */

#include "wiced_platform.h"
#include "wwd_assert.h"
#include "ak4961.h"
#include <string.h>

/******************************************************
 *                      Macros
 ******************************************************/

#define AK4961_VERIFY(x)                               {wiced_result_t res = (x); if (res != WICED_SUCCESS){wiced_assert(#x, 0==1); return res;}}

/******************************************************
 *                    Constants
 ******************************************************/

#define AK4961_COMMAND_CODE_CTREG_READ      (0x01)
#define AK4961_COMMAND_CODE_CTREG_WRITE     (0x81)

#define I2C_XFER_RETRY_COUNT                (3)
#define I2C_DISABLE_DMA                     WICED_TRUE


/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct i2c_ak4961_payload           i2c_ak4961_payload_t;


/******************************************************
 *                    Structures
 ******************************************************/

struct i2c_ak4961_payload
{
    uint8_t cmd;
    uint8_t addr_hi;
    uint8_t addr_lo;
    uint8_t data;
};


/******************************************************
 *               Variables Definitions
 ******************************************************/

static uint8_t ak4961_reg_state[0x0109 + 1];


/******************************************************
 *              Function Declarations
 ******************************************************/

extern wiced_result_t ak4961_chip_reset(ak4961_device_cmn_data_t *ak4961);


/******************************************************
 *               Function Definitions
 ******************************************************/

/* Defaults from AK4961 datasheet. */
static void ak4961_reset_reg_state( void )
{
    memset( ak4961_reg_state, 0x00, sizeof ak4961_reg_state );

    /* Non-zero defaults. */
    ak4961_reg_state[0x0034] = 0x10;
    ak4961_reg_state[0x0035] = 0x02;
    ak4961_reg_state[0x0036] = 0x19;
    ak4961_reg_state[0x0037] = 0x19;
    ak4961_reg_state[0x0038] = 0x19;
    ak4961_reg_state[0x0039] = 0x19;
    ak4961_reg_state[0x003A] = 0x75;
    ak4961_reg_state[0x003B] = 0x05;
    ak4961_reg_state[0x003C] = 0x55;
    ak4961_reg_state[0x0054] = 0x20;
    ak4961_reg_state[0x005A] = 0x20;
    ak4961_reg_state[0x0067] = 0x03;
    ak4961_reg_state[0x006E] = 0x10;
    ak4961_reg_state[0x0094] = 0x29;
    ak4961_reg_state[0x0096] = 0x0A;
    ak4961_reg_state[0x00D5] = 0x61;
    ak4961_reg_state[0x00DA] = 0xFC;
    ak4961_reg_state[0x00DF] = 0x08;
    ak4961_reg_state[0x00E3] = 0x08;
    ak4961_reg_state[0x00E7] = 0x08;
    ak4961_reg_state[0x00EB] = 0x08;
}

static wiced_result_t ak4961_i2c_reg_write(wiced_i2c_device_t *device, uint16_t reg, uint8_t value)
{
    wiced_i2c_message_t     msg[1];
    i2c_ak4961_payload_t    payload;

    payload.cmd     = AK4961_COMMAND_CODE_CTREG_WRITE;
    payload.addr_hi = (uint8_t)(reg >> 8);
    payload.addr_lo = (uint8_t)(reg);
    payload.data    = value;

    //WPRINT_LIB_INFO(("i2c write: cmd:0x%02x addr_hi:0x%02x addr_lo:0x%02x data:0x%02x (bytes %d)\n", payload.cmd, payload.addr_hi, payload.addr_lo, payload.data, sizeof payload));
    AK4961_VERIFY( wiced_i2c_init_tx_message( msg, &payload, 4, I2C_XFER_RETRY_COUNT, I2C_DISABLE_DMA ) );

    return wiced_i2c_transfer( device, msg, 1 );
}

wiced_result_t ak4961_reg_init( ak4961_device_cmn_data_t *ak4961 )
{
    AK4961_VERIFY( wiced_i2c_init( ak4961->i2c_data ) );

    return WICED_SUCCESS;
}

wiced_result_t ak4961_reg_reset( ak4961_device_cmn_data_t *ak4961 )
{
    /* Chip reset. */
    AK4961_VERIFY( ak4961_chip_reset( ak4961 ) );

    /* Sync register cache. */
    ak4961_reset_reg_state();

    return WICED_SUCCESS;
}

/* I2C combined messages is unsupported in BCM4390x.GSIO! */
wiced_result_t ak4961_reg_read( ak4961_device_cmn_data_t *ak4961, uint16_t register_address, uint8_t *reg_data )
{
    UNUSED_PARAMETER( ak4961 );

    if ( register_address > sizeof( ak4961_reg_state ) )
    {
        return WICED_BADVALUE;
    }

    *reg_data = ak4961_reg_state[register_address];

    return WICED_SUCCESS;
}

wiced_result_t ak4961_reg_write( ak4961_device_cmn_data_t *ak4961, uint16_t register_address, uint8_t reg_data )
{
    AK4961_VERIFY( ak4961_i2c_reg_write( ak4961->i2c_data, register_address, reg_data ) );
    ak4961_reg_state[register_address] = reg_data;
    return WICED_SUCCESS;
}
