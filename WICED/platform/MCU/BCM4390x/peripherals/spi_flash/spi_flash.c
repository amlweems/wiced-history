/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include "spi_flash.h"
#include "spi_flash_internal.h"
#include <string.h> /* for NULL */
#include "platform_appscr4.h"
#include "platform_mcu_peripheral.h"
#include "platform_config.h"
#include "m2m_hnddma.h"
#include "platform_m2m.h"
#include "crypto_api.h"
#include <platform_toolchain.h>

#if (PLATFORM_NO_SFLASH_WRITE == 0)
#include "wiced_osl.h"
#include <hndsoc.h>
#endif

#define SFLASH_CHECK_RESULT( expr, retval )        { if (expr) { return retval; }}

#define SFLASH_POLLING_TIMEOUT       (3000)     /* sflash status wait timeout in milisecond */
#define SFLASH_POLLING_ERASE_TIMEOUT (200000)   /* sflash chip erase timeout in milisecond */

#define SFLASH_DRIVE_STRENGTH_MASK   (0x1C000)

#define SFLASH_CTRL_OPCODE_MASK      (0xff)
#define SFLASH_CTRL_ACTIONCODE_MASK  (0xff)
#define SFLASH_CTRL_NUMBURST_MASK    (0x03)

/* QuadAddrMode(bit 24) of SFlashCtrl register only works after Chipcommon Core Revision 55 */
#define CHIP_SUPPORT_QUAD_ADDR_MODE(ccrev)   ( ccrev >= 55  )

struct sflash_capabilities
{
    unsigned long  size;
    unsigned short max_write_size;
    unsigned int   fast_read_divider:5;
    unsigned int   normal_read_divider:5;
    unsigned int   supports_fast_read:1;
    unsigned int   supports_quad_read:1;
    unsigned int   supports_quad_write:1;
    unsigned int   fast_dummy_cycles:4;
    unsigned int   write_enable_required_before_every_write:1;
};

typedef struct sflash_capabilities sflash_capabilities_t;

#define MAX_DIVIDER  ((1<<5)-1)

#define MEGABYTE  (0x100000)

#define DIRECT_WRITE_BURST_LENGTH    (64)
#define MAX_NUM_BURST                (2)
#define INDIRECT_DATA_4BYTE          (4)
#define INDIRECT_DATA_1BYTE          (1)

typedef struct
{
        uint32_t device_id;
        sflash_capabilities_t capabilities;
} sflash_capabilities_table_element_t;

const sflash_capabilities_table_element_t sflash_capabilities_table[] =
{
#ifdef SFLASH_SUPPORT_MACRONIX_PARTS
#if defined(PLATFORM_4390X_OVERCLOCK)
        { SFLASH_ID_MX25L8006E,  { .size = 1*MEGABYTE,  .max_write_size = 1, .write_enable_required_before_every_write = 1, .normal_read_divider = 12, .fast_read_divider = 4, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
        { SFLASH_ID_MX25L1606E,  { .size = 2*MEGABYTE,  .max_write_size = 1, .write_enable_required_before_every_write = 1, .normal_read_divider = 12, .fast_read_divider = 4, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
        { SFLASH_ID_MX25L6433F,  { .size = 8*MEGABYTE,  .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 12, .fast_read_divider = 4, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
        { SFLASH_ID_MX25L25635F, { .size = 32*MEGABYTE, .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 12, .fast_read_divider = 4, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#else
        { SFLASH_ID_MX25L8006E,  { .size = 1*MEGABYTE,  .max_write_size = 1, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
        { SFLASH_ID_MX25L1606E,  { .size = 2*MEGABYTE,  .max_write_size = 1, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
        { SFLASH_ID_MX25L6433F,  { .size = 8*MEGABYTE,  .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 1, .supports_quad_write = 1, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
        { SFLASH_ID_MX25L25635F, { .size = 32*MEGABYTE, .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 1, .supports_quad_write = 1, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#endif /* PLATFORM_4390X_OVERCLOCK */
#endif /* SFLASH_SUPPORT_MACRONIX_PARTS */
#ifdef SFLASH_SUPPORT_SST_PARTS
#if defined(PLATFORM_4390X_OVERCLOCK)
        { SFLASH_ID_SST25VF080B, { .size = 1*MEGABYTE,   .max_write_size = 1, .write_enable_required_before_every_write = 1, .normal_read_divider = 12, .fast_read_divider = 4, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#else
        { SFLASH_ID_SST25VF080B, { .size = 1*MEGABYTE,   .max_write_size = 1, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 4, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#endif /* PLATFORM_4390X_OVERCLOCK */
#endif /* if SFLASH_SUPPORT_SST_PARTS */
#ifdef SFLASH_SUPPORT_EON_PARTS
#if defined(PLATFORM_4390X_OVERCLOCK)
        { SFLASH_ID_EN25QH16,    { .size = 2*MEGABYTE,  .max_write_size = 1, .write_enable_required_before_every_write = 1, .normal_read_divider = 12, .fast_read_divider = 4, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#else
        { SFLASH_ID_EN25QH16,    { .size = 2*MEGABYTE,  .max_write_size = 1, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 1, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#endif /* PLATFORM_4390X_OVERCLOCK */
#endif /* if SFLASH_SUPPORT_EON_PARTS */
#ifdef SFLASH_SUPPORT_ISSI_PARTS
#if defined(PLATFORM_4390X_OVERCLOCK)
        { SFLASH_ID_ISSI25CQ032, { .size = 4*MEGABYTE,  .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 12, .fast_read_divider = 4, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
        { SFLASH_ID_ISSI25LP064, { .size = 8*MEGABYTE,  .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 12, .fast_read_divider = 4, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#else
        { SFLASH_ID_ISSI25CQ032, { .size = 4*MEGABYTE,  .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 1, .supports_quad_write = 1, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
        { SFLASH_ID_ISSI25LP064, { .size = 8*MEGABYTE,  .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 1, .supports_quad_write = 1, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#endif /* PLATFORM_4390X_OVERCLOCK */
#endif /* if SFLASH_SUPPORT_ISSI_PARTS */
#ifdef SFLASH_SUPPORT_MICRON_PARTS
#if defined(PLATFORM_4390X_OVERCLOCK)
        { SFLASH_ID_N25Q064A, { .size = 8*MEGABYTE,  .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 12, .fast_read_divider = 4, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#else
        { SFLASH_ID_N25Q064A, { .size = 8*MEGABYTE,  .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 1, .supports_quad_write = 1, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#endif /* PLATFORM_4390X_OVERCLOCK */
#endif /* if SFLASH_SUPPORT_MICRON_PARTS */
#ifdef SFLASH_SUPPORT_WINBOND_PARTS
#if defined(PLATFORM_4390X_OVERCLOCK)
        { SFLASH_ID_W25Q64FV, { .size = 8*MEGABYTE,  .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 12, .fast_read_divider = 4, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#else
        { SFLASH_ID_W25Q64FV, { .size = 8*MEGABYTE,  .max_write_size = 256, .write_enable_required_before_every_write = 1, .normal_read_divider = 6, .fast_read_divider = 2, .supports_quad_read = 1, .supports_quad_write = 1, .supports_fast_read = 1, .fast_dummy_cycles = 8 } },
#endif /* PLATFORM_4390X_OVERCLOCK */
#endif /* if SFLASH_SUPPORT_WINBOND_PARTS */
        { SFLASH_ID_DEFAULT,     { .size = 0,           .max_write_size = 1, .write_enable_required_before_every_write = 1, .normal_read_divider = MAX_DIVIDER, .fast_read_divider = MAX_DIVIDER, .supports_quad_read = 0, .supports_quad_write = 0, .supports_fast_read = 0, .fast_dummy_cycles = 0 } }
};

static const uint32_t address_masks[SFLASH_ACTIONCODE_MAX_ENUM] =
{
        [SFLASH_ACTIONCODE_ONLY]           = 0x00,
        [SFLASH_ACTIONCODE_1DATA]          = 0x00,
        [SFLASH_ACTIONCODE_3ADDRESS]       = 0x00ffffff,
        [SFLASH_ACTIONCODE_3ADDRESS_1DATA] = 0x00ffffff,
        [SFLASH_ACTIONCODE_3ADDRESS_4DATA] = 0x00ffffff,
        [SFLASH_ACTIONCODE_2DATA]          = 0x00,
        [SFLASH_ACTIONCODE_4DATA]          = 0x00,
};

static const uint32_t data_masks[SFLASH_ACTIONCODE_MAX_ENUM] =
{
        [SFLASH_ACTIONCODE_ONLY]           = 0x00000000,
        [SFLASH_ACTIONCODE_1DATA]          = 0x000000ff,
        [SFLASH_ACTIONCODE_3ADDRESS]       = 0x00000000,
        [SFLASH_ACTIONCODE_3ADDRESS_1DATA] = 0x000000ff,
        [SFLASH_ACTIONCODE_3ADDRESS_4DATA] = 0xffffffff,
        [SFLASH_ACTIONCODE_2DATA]          = 0x0000ffff,
        [SFLASH_ACTIONCODE_4DATA]          = 0xffffffff,
};

static const uint8_t data_bytes[SFLASH_ACTIONCODE_MAX_ENUM] =
{
        [SFLASH_ACTIONCODE_ONLY]           = 0,
        [SFLASH_ACTIONCODE_1DATA]          = 1,
        [SFLASH_ACTIONCODE_3ADDRESS]       = 0,
        [SFLASH_ACTIONCODE_3ADDRESS_1DATA] = 1,
        [SFLASH_ACTIONCODE_3ADDRESS_4DATA] = 4,
        [SFLASH_ACTIONCODE_2DATA]          = 2,
        [SFLASH_ACTIONCODE_4DATA]          = 4,
};

static uint ccrev;
static securesflash_handle_t securesflash_handle ALIGNED(HWCRYPTO_ALIGNMENT_BYTES);

static void sflash_get_capabilities( sflash_handle_t* handle );

static int generic_sflash_command(                               const sflash_handle_t* const handle,
                                                                 sflash_command_t             cmd,
                                                                 bcm43909_sflash_actioncode_t actioncode,
                                                                 unsigned long                device_address,
                            /*@null@*/ /*@observer@*/            const void* const            data_MOSI,
                            /*@null@*/ /*@out@*/ /*@dependent@*/ void* const                  data_MISO );

/*@access sflash_handle_t@*/ /* Lint: permit access to abstract sflash handle implementation */

static int sflash_read_internal( const sflash_handle_t* const handle, unsigned long device_address,
        /*@out@*/ /*@dependent@*/ void* data_addr, unsigned int size , wiced_bool_t blocking );
static void sflash_read_post_read_operations( const sflash_handle_t* const handle, void* data_addr, unsigned int size );
static int sflash_read_nonblocking( const sflash_handle_t* const handle, unsigned long device_address,
        /*@out@*/ /*@dependent@*/ void* data_addr, unsigned int size );
static void sflash_read_wait_for_completion( void* data_addr, unsigned int size );

static int sflash_verify_data( uint8_t* aes128_key, uint8_t* aes_iv, uint32_t crypt_size, uint32_t auth_size,
        uint8_t* hmac_key, uint32_t hmac_key_len, uint8_t* input_buffer, uint8_t* output_buffer, uint8_t* hmac_output );
static void otp_read_keys( uint8_t* buffer, uint8_t* buffer_r, uint32_t word_num, uint32_t word_num_r, uint32_t size_in_bytes );
static int init_securesflash( /*@out@*/ sflash_handle_t* const handle);

int sflash_read_ID( const sflash_handle_t* handle, /*@out@*/ device_id_t* data_addr )
{
    /* The indirect SFLASH interface does not allow reading of 3 bytes - read 4 bytes instead and copy */
    uint32_t temp_data;
    int retval;

    retval = generic_sflash_command( handle, SFLASH_READ_JEDEC_ID, SFLASH_ACTIONCODE_4DATA, 0, NULL, &temp_data );

    memset( data_addr, 0, 3 );
    memcpy( data_addr, &temp_data, 3 );

    return retval;
}

int sflash_write_enable( const sflash_handle_t* const handle )
{
    if ( handle->write_allowed == SFLASH_WRITE_ALLOWED )
    {
        unsigned char status_register;
        unsigned char masked_status_register;
        int status;

        /* Send write-enable command */
        status = generic_sflash_command( handle, SFLASH_WRITE_ENABLE, SFLASH_ACTIONCODE_ONLY, 0, NULL, NULL );
        SFLASH_CHECK_RESULT( status != 0, status );

        /* Check status register */
        status = sflash_read_status_register( handle, &status_register );
        SFLASH_CHECK_RESULT( status != 0, status );

        /* Check if Block protect bits are set */
        masked_status_register = status_register & ( SFLASH_STATUS_REGISTER_WRITE_ENABLED |
                                                     SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_0 |
                                                     SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_1 |
                                                     SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_2 |
                                                     SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_3 );

        if ( masked_status_register != SFLASH_STATUS_REGISTER_WRITE_ENABLED )
        {
            /* Disable protection for all blocks */
            status_register = status_register & (unsigned char)(~( SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_0 | SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_1 | SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_2 |  SFLASH_STATUS_REGISTER_BLOCK_PROTECTED_3 ));
            status = sflash_write_status_register( handle, status_register );
            SFLASH_CHECK_RESULT( status != 0, status );

            /* Re-Enable writing */
            status = generic_sflash_command( handle, SFLASH_WRITE_ENABLE, SFLASH_ACTIONCODE_ONLY, 0, NULL, NULL );
            SFLASH_CHECK_RESULT( status != 0, status );

            /* Check status register */
            status = sflash_read_status_register( handle, &status_register );
            SFLASH_CHECK_RESULT( status != 0, status );

            status_register++;
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

int sflash_chip_erase( const sflash_handle_t* const handle )
{
    int status = sflash_write_enable( handle );
    SFLASH_CHECK_RESULT( status != 0, status );

    return generic_sflash_command( handle, SFLASH_CHIP_ERASE1, SFLASH_ACTIONCODE_ONLY, 0, NULL, NULL );
}

#include "wwd_assert.h"

int sflash_sector_erase ( const sflash_handle_t* const handle, unsigned long device_address )
{
    int retval;
    int status = sflash_write_enable( handle );
    SFLASH_CHECK_RESULT( status != 0, status );

    retval = generic_sflash_command( handle, SFLASH_SECTOR_ERASE, SFLASH_ACTIONCODE_3ADDRESS, (device_address & 0x00FFFFFF), NULL, NULL );
    wiced_assert("error", retval == 0);
    return retval;
}

int sflash_read_status_register( const sflash_handle_t* const handle, /*@out@*/  /*@dependent@*/ unsigned char* const dest_addr )
{
    return generic_sflash_command( handle, SFLASH_READ_STATUS_REGISTER, SFLASH_ACTIONCODE_1DATA, 0, NULL, dest_addr );
}

int sflash_read_status_register2( const sflash_handle_t* const handle, /*@out@*/  /*@dependent@*/ unsigned char* const dest_addr )
{
    return generic_sflash_command( handle, SFLASH_READ_STATUS_REGISTER2, SFLASH_ACTIONCODE_1DATA, 0, NULL, dest_addr );
}

int sflash_read( const sflash_handle_t* const handle, unsigned long device_address, /*@out@*/ /*@dependent@*/ void* data_addr, unsigned int size )
{
    int result;
    wiced_bool_t blocking = WICED_TRUE;

    result = sflash_read_internal( handle, device_address, data_addr, size, blocking );
    return result;
}

static int sflash_read_nonblocking( const sflash_handle_t* const handle, unsigned long device_address, /*@out@*/ /*@dependent@*/ void* data_addr, unsigned int size )
{
    int result;
    wiced_bool_t blocking = WICED_FALSE;

    result = sflash_read_internal( handle, device_address, data_addr, size, blocking );
    return result;
}

static void sflash_read_wait_for_completion( void* data_addr, unsigned int size )
{
    m2m_switch_off_dma_post_completion( );
    m2m_post_dma_completion_operations( data_addr, size );
}

static void sflash_read_post_read_operations( const sflash_handle_t* const handle, void* data_addr, unsigned int size )
{
    PLATFORM_CHIPCOMMON->clock_control.divider.bits.serial_flash_divider = handle->capabilities->normal_read_divider;

    UNUSED_PARAMETER( data_addr );
    UNUSED_PARAMETER( size );
}

static int sflash_read_internal( const sflash_handle_t* const handle, unsigned long device_address, /*@out@*/ /*@dependent@*/ void* data_addr, unsigned int size, wiced_bool_t blocking )
{
#ifdef SLFASH_43909_INDIRECT

    unsigned int i;
    int retval;
    for ( i = 0; i < size; i++ )
    {
        retval = generic_sflash_command( handle, SFLASH_READ, SFLASH_ACTIONCODE_3ADDRESS_1DATA, ( (device_address+i) & 0x00ffffff ), NULL, &((char*)data_addr)[i] );
        SFLASH_CHECK_RESULT( retval != 0, retval );
    }

#else /* SLFASH_43909_INDIRECT */

    void* direct_address = (void*) ( SI_SFLASH + device_address );

    bcm43909_sflash_ctrl_reg_t ctrl = { .bits = { .opcode                     = SFLASH_READ,
                                                  .action_code                = SFLASH_ACTIONCODE_3ADDRESS_4DATA,
                                                  .use_four_byte_address_mode = 0,
                                                  .use_opcode_reg             = 1,
                                                  .mode_bit_enable            = 0,
                                                  .num_dummy_cycles           = 0,
                                                  .num_burst                  = 3,
                                                  .high_speed_mode            = 0,
                                                  .start_busy                 = 0,
                                                }
                                      };
    if ( ( handle->capabilities->supports_fast_read == 1 ) ||  ( handle->capabilities->supports_quad_read == 1 ) )
    {

        if ( handle->capabilities->supports_quad_read == 1 )
        {
#ifdef CHECK_QUAD_ENABLE_EVERY_READ
            unsigned char status_register;
            int status;
            /* Check status register */
            status = sflash_read_status_register( handle, &status_register );
            SFLASH_CHECK_RESULT( status != 0, status );

            wiced_assert("", (status_register & SFLASH_STATUS_REGISTER_QUAD_ENABLE) != 0 );
            (void) status_register;
#endif /* ifdef CHECK_QUAD_ENABLE_EVERY_READ */

            /* Both address and data in 4-bit mode */
            if ( SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, MACRONIX) )
            {
                ctrl.bits.opcode = SFLASH_X4IO_READ;
                ctrl.bits.num_dummy_cycles = 6;
            }

            if ( SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, MICRON ) )
            {
                ctrl.bits.opcode = SFLASH_X4IO_READ;
                ctrl.bits.num_dummy_cycles = 10;
            }

            if ( SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, WINBOND ) )
            {
                ctrl.bits.opcode = SFLASH_X4IO_READ;
                ctrl.bits.num_dummy_cycles = 4;
                ctrl.bits.mode_bit_enable = 1;
            }

            if ( SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, ISSI_CQ ) )
            {
                ctrl.bits.mode_bit_enable = 1;
                ctrl.bits.num_dummy_cycles = 4;
            }
        }
        else
        {
            ctrl.bits.opcode = SFLASH_FAST_READ;
            ctrl.bits.num_dummy_cycles = handle->capabilities->fast_dummy_cycles;
        }

        /* Set SPI flash clock to higher speed if possible */
        PLATFORM_CHIPCOMMON->clock_control.divider.bits.serial_flash_divider = handle->capabilities->fast_read_divider;

        ctrl.bits.high_speed_mode = ( handle->capabilities->fast_read_divider == 2 ) ? 1 : 0;
    }

    PLATFORM_CHIPCOMMON->sflash.control.raw = ctrl.raw;


    wiced_assert("Check match between highspeed mode and divider",
            ( PLATFORM_CHIPCOMMON->clock_control.divider.bits.serial_flash_divider == 2 ) ==
            ( PLATFORM_CHIPCOMMON->sflash.control.bits.high_speed_mode == 1 ) );

    m2m_unprotected_dma_memcpy( data_addr, direct_address, size , blocking );

    if ( blocking == WICED_TRUE )
    {
        sflash_read_post_read_operations( handle, data_addr, size );
    }


#endif /* ifdef SLFASH_43909_INDIRECT */

    return 0;
}

int sflash_get_size( const sflash_handle_t* const handle, /*@out@*/ unsigned long* const size )
{
    *size = handle->capabilities->size;
    return 0;
}


static void sflash_get_capabilities( sflash_handle_t* handle )
{
    const sflash_capabilities_table_element_t* capabilities_element = sflash_capabilities_table;
    while ( ( capabilities_element->device_id != SFLASH_ID_DEFAULT ) &&
            ( capabilities_element->device_id != handle->device_id ) )
    {
        capabilities_element++;
    }

    handle->capabilities = &capabilities_element->capabilities;
}

static inline int is_quad_write_allowed( uint ccrev_in, const sflash_handle_t* const handle )
{
#if (PLATFORM_NO_SFLASH_WRITE == 0)
    return ( CHIP_SUPPORT_QUAD_ADDR_MODE( ccrev_in ) &&
             ( handle->capabilities->supports_quad_write == 1 ) );
#else
    UNUSED_PARAMETER( ccrev_in );
    UNUSED_PARAMETER( handle );
    return 0;
#endif
}

int sflash_write( const sflash_handle_t* const handle, unsigned long device_address, /*@observer@*/ const void* const data_addr, unsigned int size )
{
    int status;
    unsigned int write_size, num_burst;
    unsigned int max_write_size = handle->capabilities->max_write_size;
    int enable_before_every_write = handle->capabilities->write_enable_required_before_every_write;
    unsigned char* data_addr_ptr = (unsigned char*) data_addr;
    sflash_command_t opcode = SFLASH_WRITE;

    if ( handle->write_allowed != SFLASH_WRITE_ALLOWED )
    {
        return -1;
    }

    /* Some manufacturers support programming an entire page in one command. */
    if ( enable_before_every_write == 0 )
    {
        status = sflash_write_enable( handle );
        SFLASH_CHECK_RESULT( status != 0, status );
    }

    if ( is_quad_write_allowed( ccrev, handle ) )
    {
        if ( SFLASH_MANUFACTURER_SUPPORTED( handle->device_id , MACRONIX) )
        {
            opcode = SFLASH_X4IO_WRITE;
        }
        else if ( SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, ISSI_CQ )  ||
                  SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, MICRON )   ||
                  SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, WINBOND )  ||
                  SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, ISSI_LP ) )
        {
            opcode = SFLASH_QUAD_WRITE;
        }
        else
        {
            wiced_assert( "No opcode applied for quad write", 0 != 1 );
        }
    }

    /* Generic x-bytes-at-a-time write */
    while ( size > 0 )
    {
        write_size = ( size >= max_write_size )? max_write_size : size;
        /* All transmitted data must not go beyond the end of the current page in a write */
        write_size = MIN( max_write_size - (device_address % max_write_size), write_size );
#ifndef SLFASH_43909_INDIRECT
        if ( (write_size >= DIRECT_WRITE_BURST_LENGTH) && ((device_address & 0x03) == 0x00) && (((unsigned long)data_addr_ptr & 0x03) == 0x00) )
        {
            num_burst = MIN( MAX_NUM_BURST, write_size / DIRECT_WRITE_BURST_LENGTH );
            write_size = DIRECT_WRITE_BURST_LENGTH * num_burst;
        }
        else
#endif
        {
            num_burst = 0;
            write_size = ( write_size >= INDIRECT_DATA_4BYTE )? INDIRECT_DATA_4BYTE : INDIRECT_DATA_1BYTE;
        }

        if ( ( enable_before_every_write == 1 ) &&
             ( 0 != ( status = sflash_write_enable( handle ) ) ) )
        {
            return status;
        }

        if ( num_burst > 0 )
        {
            /* Use Direct backplane access */
            uint32_t i, *src_data_ptr, *dst_data_ptr;
            unsigned char status_register;
            uint32_t current_time, elapsed_time;

            bcm43909_sflash_ctrl_reg_t ctrl = { .bits = { .opcode                     = opcode,
                                                          .action_code                = SFLASH_ACTIONCODE_3ADDRESS_4DATA,
                                                          .use_four_byte_address_mode = 0,
                                                          .use_opcode_reg             = 1,
                                                          .mode_bit_enable            = 0,
                                                          .num_dummy_cycles           = 0,
                                                          .num_burst                  = num_burst & SFLASH_CTRL_NUMBURST_MASK,
                                                          .high_speed_mode            = 0,
                                                          .use_quad_address_mode      = ( opcode == SFLASH_X4IO_WRITE ) ? 1 : 0,
                                                          .start_busy                 = 0,
                                                        }
                                              };

            PLATFORM_CHIPCOMMON->clock_control.divider.bits.serial_flash_divider = handle->capabilities->normal_read_divider;
            PLATFORM_CHIPCOMMON->sflash.control.raw = ctrl.raw;

            src_data_ptr = (uint32_t *) data_addr_ptr;
            dst_data_ptr = (uint32_t *) ( SI_SFLASH + device_address );

            for ( i = 0; i < write_size / sizeof(uint32_t); i++ )
            {
                *dst_data_ptr = *src_data_ptr++;
            }

            /* Additional write starts to issue the transaction to the SFLASH */
            *dst_data_ptr = 0xFFFFFFFF;

            /* sflash state machine is still running. Do not change any bits in this register while this bit is high */
            current_time = host_rtos_get_time();
            while ( PLATFORM_CHIPCOMMON->sflash.control.bits.backplane_write_dma_busy == 1 )
            {
                elapsed_time = host_rtos_get_time() - current_time;
                if ( elapsed_time > SFLASH_POLLING_TIMEOUT )
                {
                    /* timeout */
                    return -1;
                }
            }

            /* write commands require waiting until chip is finished writing */
            current_time = host_rtos_get_time();
            do
            {
                status = sflash_read_status_register( handle, &status_register );
                SFLASH_CHECK_RESULT( status != 0, status );

                elapsed_time = host_rtos_get_time() - current_time;
                if ( elapsed_time > SFLASH_POLLING_TIMEOUT )
                {
                    /* timeout */
                    return -1;
                }
            } while( ( status_register & SFLASH_STATUS_REGISTER_BUSY ) != (unsigned char) 0 );

        }
        else
        {
            /* Use indirect sflash access */

            if ( write_size == INDIRECT_DATA_1BYTE )
            {
                status = generic_sflash_command( handle, opcode, SFLASH_ACTIONCODE_3ADDRESS_1DATA, (device_address & 0x00FFFFFF), data_addr_ptr, NULL );
            }
            else if ( write_size == INDIRECT_DATA_4BYTE )
            {
                status = generic_sflash_command( handle, opcode, SFLASH_ACTIONCODE_3ADDRESS_4DATA, (device_address & 0x00FFFFFF), data_addr_ptr, NULL );
            }
        }

        SFLASH_CHECK_RESULT( status != 0, status );

        data_addr_ptr += write_size;
        device_address += write_size;
        size -= write_size;

    }

    return 0;
}

int sflash_write_status_register( const sflash_handle_t* const handle, unsigned char value )
{
    unsigned char status_register_val = value;
    /* SST parts require enabling writing to the status register */
    if ( SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, SST ) )
    {
        int status;
        status = generic_sflash_command( handle, SFLASH_ENABLE_WRITE_STATUS_REGISTER, SFLASH_ACTIONCODE_ONLY, 0, NULL, NULL );
        SFLASH_CHECK_RESULT( status != 0, status );
    }

    /* Macronix and ISSI parts require enabling writing to the status register */
    if ( SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, MACRONIX ) ||
         SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, ISSI_CQ )  ||
         SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, ISSI_LP ) )
    {
        int status;
        /* Send write-enable command */
        status = generic_sflash_command( handle, SFLASH_WRITE_ENABLE, SFLASH_ACTIONCODE_ONLY, 0, NULL, NULL );
        SFLASH_CHECK_RESULT( status != 0, status );
    }

    return generic_sflash_command( handle, SFLASH_WRITE_STATUS_REGISTER, SFLASH_ACTIONCODE_1DATA, 0, &status_register_val, NULL );
}

int init_sflash( /*@out@*/ sflash_handle_t* const handle, /*@shared@*/ void* peripheral_id, sflash_write_allowed_t write_allowed_in )
{
    int status;
    device_id_t tmp_device_id;

    (void) peripheral_id;

    handle->write_allowed = write_allowed_in;
    handle->device_id     = 0;

    /* Sflash only works with divider=2 when sflash drive strength is max */
    platform_gci_chipcontrol( 8, 0, SFLASH_DRIVE_STRENGTH_MASK);

    /* Set to default capabilities */
    handle->capabilities = &sflash_capabilities_table[sizeof(sflash_capabilities_table)/sizeof(sflash_capabilities_table_element_t) - 1].capabilities;

    status = sflash_read_ID( handle, &tmp_device_id );
    SFLASH_CHECK_RESULT( status != 0, status );

    handle->device_id = ( ((uint32_t) tmp_device_id.id[0]) << 16 ) +
                        ( ((uint32_t) tmp_device_id.id[1]) <<  8 ) +
                        ( ((uint32_t) tmp_device_id.id[2]) <<  0 );

    sflash_get_capabilities( handle );

    PLATFORM_CHIPCOMMON->clock_control.divider.bits.serial_flash_divider = handle->capabilities->normal_read_divider;

    if ( write_allowed_in == SFLASH_WRITE_ALLOWED )
    {
#if (PLATFORM_NO_SFLASH_WRITE > 0)
        return -1;
#else
        /* Get chipc core rev to decide whether 4-bit write supported or not */
        ccrev = osl_get_corerev( CC_CORE_ID );

        /* Enable writing */
        status = sflash_write_enable( handle );
        SFLASH_CHECK_RESULT( status != 0, status );
#endif
    }

    if (  handle->capabilities->supports_quad_read == 1 )
    {
        unsigned char status_register;
        unsigned char status_register2;
        unsigned short status_register_new;
        if ( SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, MACRONIX ) )
        {
            /* Check status register */
            status = sflash_read_status_register( handle, &status_register );
            SFLASH_CHECK_RESULT( status != 0, status );

            if ( ( status_register & SFLASH_STATUS_REGISTER_QUAD_ENABLE ) == 0 )
            {
                status_register |= SFLASH_STATUS_REGISTER_QUAD_ENABLE;

                status = sflash_write_status_register( handle, status_register );
                SFLASH_CHECK_RESULT( status != 0, status );
            }
        }

        if ( SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, WINBOND ) )
        {
            status = sflash_read_status_register( handle, &status_register );
            SFLASH_CHECK_RESULT( status != 0, status );

            status = sflash_read_status_register2( handle, &status_register2 );
            SFLASH_CHECK_RESULT( status != 0, status );

            status_register2 |= WINBOND_SFLASH_STATUS_REGISTER2_QUAD_ENABLE; // Enable Quad mode
            status_register_new = (unsigned short)((status_register2 << 8) | status_register);

            /* Send non-volatile status register (register-1 + register-2) for WINBOND*/
            status = generic_sflash_command( handle, SFLASH_WRITE_STATUS_REGISTER, SFLASH_ACTIONCODE_2DATA, 0, &status_register_new, NULL );
            SFLASH_CHECK_RESULT( status != 0, status );
        }

        if ( SFLASH_MANUFACTURER_SUPPORTED( handle->device_id, MICRON ) )
        {
            /* Reading data in Quad mode with Micron sflash, we should disable HOLD# first or it will interfere Quad mode.
             * 1. If sflash in Quade mode, it will use DQ3 as input/output and we should disable function of HOLD#. (P. 11)
             * 2. MICRON_SFLASH_ENH_VOLATILE_STATUS_REGISTER_HOLD is volatile register, it should be configured again after each power-cycle. (P. 24)
             * 3. In any sflash action of PROGRAM & ERASE & WRITE, we should issue SFLASH_WRITE_ENALBE first. (P. 48)*/
            status = sflash_read_status_register( handle, &status_register );
            SFLASH_CHECK_RESULT( status != 0, status );

            if ( !( status_register & SFLASH_STATUS_REGISTER_WRITE_ENABLED ) )
            {
                status = generic_sflash_command( handle, SFLASH_WRITE_ENABLE, SFLASH_ACTIONCODE_ONLY, 0, NULL, NULL );
                SFLASH_CHECK_RESULT( status != 0, status );
                status_register = 0;
            }

            status = generic_sflash_command( handle, SFLASH_READ_ENH_VOLATILE_REGISTER, SFLASH_ACTIONCODE_1DATA, 0, NULL, &status_register );
            SFLASH_CHECK_RESULT( status != 0, status );

            /* Disable function of HOLD# for Quad mode */
            status_register &= (unsigned char)~(MICRON_SFLASH_ENH_VOLATILE_STATUS_REGISTER_HOLD);

            status = generic_sflash_command( handle, SFLASH_WRITE_ENH_VOLATILE_REGISTER, SFLASH_ACTIONCODE_1DATA, 0, &status_register, NULL );
            SFLASH_CHECK_RESULT( status != 0, status );
        }
    }

    init_securesflash( handle );
    return 0;
}

int deinit_sflash( sflash_handle_t* const handle )
{
    UNUSED_PARAMETER( handle );
    return 0;
}

static inline int is_erase_command( sflash_command_t cmd )
{
    return ( ( cmd == SFLASH_CHIP_ERASE1           ) ||
             ( cmd == SFLASH_CHIP_ERASE2           ) ||
             ( cmd == SFLASH_SECTOR_ERASE          ) ||
             ( cmd == SFLASH_BLOCK_ERASE_MID       ) ||
             ( cmd == SFLASH_BLOCK_ERASE_LARGE     ) )? 1 : 0;
}

static inline int is_write_command( sflash_command_t cmd )
{
    return ( ( cmd == SFLASH_X4IO_WRITE            ) ||
             ( cmd == SFLASH_QUAD_WRITE            ) ||
             ( cmd == SFLASH_WRITE                 ) ||
             ( cmd == SFLASH_WRITE_STATUS_REGISTER ) ||
             ( is_erase_command( cmd )             ) )? 1 : 0;
}

static int generic_sflash_command(                               const sflash_handle_t* const handle,
                                                                 sflash_command_t             cmd,
                                                                 bcm43909_sflash_actioncode_t actioncode,
                                                                 unsigned long                device_address,
                            /*@null@*/ /*@observer@*/            const void* const            data_MOSI,
                            /*@null@*/ /*@out@*/ /*@dependent@*/ void* const                  data_MISO )
{

    uint32_t current_time, elapsed_time;
    bcm43909_sflash_ctrl_reg_t ctrl = { .bits = { .opcode                     = cmd,
                                                  .action_code                = actioncode,
                                                  .use_four_byte_address_mode = 0,
                                                  .use_opcode_reg             = 0,
                                                  .mode_bit_enable            = 0,
                                                  .num_dummy_cycles           = 0,
                                                  .num_burst                  = 3,
                                                  .high_speed_mode            = 0,
                                                  .use_quad_address_mode      = 0,
                                                  .start_busy                 = 1,
                                                }
                                       };

    PLATFORM_CHIPCOMMON->clock_control.divider.bits.serial_flash_divider = handle->capabilities->normal_read_divider;

    PLATFORM_CHIPCOMMON->sflash.address = device_address & address_masks[actioncode];
    if ( data_MOSI != NULL )
    {
        uint32_t data = *((uint32_t*)data_MOSI) & data_masks[actioncode];
        if ( actioncode == SFLASH_ACTIONCODE_3ADDRESS_4DATA )
        {
            data = ( ( ( data << 24 ) & ( 0xFF000000 ) ) |
                     ( ( data <<  8 ) & ( 0x00FF0000 ) ) |
                     ( ( data >>  8 ) & ( 0x0000FF00 ) ) |
                     ( ( data >> 24 ) & ( 0x000000FF ) ) );
        }
        PLATFORM_CHIPCOMMON->sflash.data = data ;
    }

    if ( cmd == SFLASH_X4IO_WRITE )
    {
        ctrl.bits.use_quad_address_mode = 1;
    }

    PLATFORM_CHIPCOMMON->sflash.control.raw = ctrl.raw;

    current_time = host_rtos_get_time();
    while ( PLATFORM_CHIPCOMMON->sflash.control.bits.start_busy == 1 )
    {
        elapsed_time = host_rtos_get_time() - current_time;
        if ( elapsed_time > SFLASH_POLLING_TIMEOUT )
        {
            /* timeout */
            return -1;
        }
    }

    if ( data_MISO != NULL )
    {
        uint32_t tmp = PLATFORM_CHIPCOMMON->sflash.data;
        memcpy( data_MISO, &tmp, data_bytes[actioncode] );
    }



    if ( is_write_command( cmd ) == 1 )
    {
        unsigned char status_register;
        uint32_t timeout_thresh;

        timeout_thresh = is_erase_command( cmd ) ? SFLASH_POLLING_ERASE_TIMEOUT : SFLASH_POLLING_TIMEOUT;
        current_time = host_rtos_get_time();

        /* write commands require waiting until chip is finished writing */

        do
        {
            int status = sflash_read_status_register( handle, &status_register );
            SFLASH_CHECK_RESULT( status != 0, status );

            elapsed_time = host_rtos_get_time() - current_time;

            if ( elapsed_time > timeout_thresh )
            {
                /* timeout */
                return -1;
            }
        } while( ( status_register & SFLASH_STATUS_REGISTER_BUSY ) != (unsigned char) 0 );
    }


    /*@-mustdefine@*/ /* Lint: lint does not realise data_MISO was set */
    return 0;
    /*@+mustdefine@*/
}

/*@noaccess sflash_handle_t@*/

/********************************************************************************
 *                   Secure Sflash Functions
 *******************************************************************************/

/*The function init_securesflash should not be ROMmed
 *
 * Initialize Securesflash handle , Which includeds pointers to
 * secure read and write functions and hwcrypto_buffer, used to store
 * data read from Sflash which is passed to HWCrypto engine */
static int init_securesflash( /*@out@*/ sflash_handle_t* const handle)
{

    if ( PLATFORM_SECURESFLASH_ENABLED )
    {
        platform_hwcrypto_init( );

        securesflash_handle.sflash_secure_read_function  = &sflash_read_secure;
        if ( PLATFORM_NO_SFLASH_WRITE )
        {
            securesflash_handle.sflash_secure_write_function = NULL;
        }
        else
        {
            securesflash_handle.sflash_secure_write_function = &sflash_write_secure;
        }

        handle->securesflash_handle = &securesflash_handle;

        otp_read_keys( securesflash_handle.hmac_key, securesflash_handle.hmac_key_redundant,
            OTP_WORD_NUM_SHA_KEY, OTP_WORD_NUM_SHA_KEY_R, SECUREBOOT_SHA_KEY_SIZE );
        otp_read_keys( securesflash_handle.aes128_key, securesflash_handle.aes128_key_redundant,
            OTP_WORD_NUM_AES_KEY, OTP_WORD_NUM_AES_KEY_R, AES128_KEY_LEN );
    }
    else
    {
        handle->securesflash_handle = NULL;
    }

    return 0;
}

/* Read Cryptographic keys stored in OTP
 * buffer_out           : Output buffer, contains the ORred result of Keys read from OTP and redundant OTP bits
 * buff_out_redundant   : Contains the Keys read from redundant OTP bits
 * word_num             : OTP word number for the key
 * word_num_redundant   : OTP word number for the redundant key
 * size_in_bytes        : Size of Key in bytes
 * */
static void otp_read_keys( uint8_t* buffer_out, uint8_t* buffer_out_redundant, uint32_t word_num, uint32_t word_num_redundant, uint32_t size_in_bytes )
{
    uint32_t i;
    uint16_t* otp_ptr = NULL;
    uint16_t* otp_ptr_redundant = NULL;
    uint32_t size_in_word = size_in_bytes/2;

    otp_ptr = (uint16_t *)buffer_out;
    otp_ptr_redundant = ( uint16_t * )buffer_out_redundant;

    for (i = 0; i < ( size_in_word ); i++)
    {
        platform_otp_read_word_unprotected( ( word_num + i ), otp_ptr );
        platform_otp_read_word_unprotected( ( word_num_redundant + i ), otp_ptr_redundant );
        *otp_ptr++ |= *otp_ptr_redundant++;
    }
}

/* Decrypt and Verify the signature of data stored in input_buffer
 * aes128_key   : Key used for AES128-CBC encryption/decryption
 * aes_iv       : iv used for AES128-CBC encryption/decryption
 * crypt_size   : Size of data stored in input_buffer
 * auth_size    : Size of data that needs to be Sign-Verified
 * hmac_key     : Key used for sha256_hmac signature verification
 * hmac_key_len : Length of the hmac_key in bytes
 * input_buffer : Data that needs to be Decrypted/Signature Verified
 * output_buffer: Encryption/Decryption Output from HWCrypto engine
 * hmac_output  : Authentication Output from HWCrypto Engine
 */
static int sflash_verify_data( uint8_t* aes128_key, uint8_t* aes_iv, uint32_t crypt_size, uint32_t auth_size, uint8_t* hmac_key, uint32_t hmac_key_len, uint8_t* input_buffer, uint8_t* output_buffer, uint8_t* hmac_output )
{
    int result = 0;
    uint8_t *expected_digest;

    platform_hwcrypto_aescbc_decrypt_sha256_hmac( aes128_key, aes_iv, crypt_size, auth_size, hmac_key,
            hmac_key_len, input_buffer, output_buffer, hmac_output );

    /* Compare Computed Signature with the Digest */
    /* Digest is stored at SECURE_SECTOR_DATA_SIZE offset from start */
    expected_digest = (uint8_t*) output_buffer + SECURE_SECTOR_DATA_SIZE;
    result = memcmp( expected_digest, hmac_output, HMAC256_OUTPUT_SIZE );
    SFLASH_CHECK_RESULT( result != 0, -1 );

    return result;
}

int sflash_read_secure( const sflash_handle_t* const handle, unsigned long device_address, /*@out@*//*@dependent@*/void* data_addr, unsigned int size )
{
    uint8_t         aes_iv[ AES_IV_LEN ];
    uint8_t*        data_addr_ptr;
    uint8_t*        crypto_buffer = NULL;
    uint8_t*        sflash_buffer = NULL;
    uint32_t        remaining_size;
    uint32_t        read_size;
    uint32_t        prev_sector_read_size = 0;
    uint32_t        prev_sector_read_offset = 0;
    int             result = 0;
    bool            sflash_read_finished = FALSE;
    unsigned long   sector_aligned_sflash_addr;
    unsigned long   read_offset;
    uint8_t         hmac_output[ SHA256_HASH_SIZE ] ALIGNED(HWCRYPTO_ALIGNMENT_BYTES);
    securesflash_handle_t *secure_sflash_handle = handle->securesflash_handle;

    wiced_assert("sflash_read_secure : securesflash handle is null", ( secure_sflash_handle != NULL ));

    data_addr_ptr   = (uint8_t*) data_addr;
    sflash_buffer   = (uint8_t*) secure_sflash_handle->hwcrypto_buffer;
    crypto_buffer   = (uint8_t*) secure_sflash_handle->hwcrypto_buffer + SECURE_SECTOR_SIZE;
    remaining_size  = size;
    memset( aes_iv, 0, sizeof( aes_iv ) );

    /* read in chunks of SECURE_SECTOR_SIZE */
    while ( remaining_size != 0 )
    {
        /* Offset from start of sector*/
        read_offset = device_address % SECURE_SECTOR_SIZE;
        /* How much data can be read from the current sector */
        read_size = MIN( remaining_size, ( SECURE_SECTOR_DATA_SIZE - read_offset ) );

        sector_aligned_sflash_addr = ALIGN_TO_SECTOR_ADDRESS( device_address );

        /* Read FULL Sector, for verification */
        sflash_read_nonblocking( handle, (unsigned long) sector_aligned_sflash_addr, sflash_buffer, SECURE_SECTOR_SIZE );

        if ( sflash_read_finished == TRUE )
        {
            result = sflash_verify_data( secure_sflash_handle->aes128_key, aes_iv, SECURE_SECTOR_SIZE, SECURE_SECTOR_DATA_SIZE,
                        secure_sflash_handle->hmac_key, SECUREBOOT_SHA_KEY_SIZE, crypto_buffer, crypto_buffer, hmac_output );
            SFLASH_CHECK_RESULT( result != 0, result );

            /* Copy back requested data to output buffer */
            memcpy( data_addr_ptr, ( (uint8_t*) crypto_buffer + prev_sector_read_offset ), prev_sector_read_size );
            data_addr_ptr += prev_sector_read_size;
        }

        /* Wait for sflash_read_nonblocking() to complete */
        sflash_read_wait_for_completion( (void*)sflash_buffer, SECURE_SECTOR_SIZE );
        sflash_read_finished = TRUE;
        sflash_read_post_read_operations( handle, (void*)sector_aligned_sflash_addr, SECURE_SECTOR_SIZE );

        /* SWAP crypto_buffer and sflash_buffer */
        SWAP( uint8_t * ,crypto_buffer, sflash_buffer );

        /* Update counters */
        prev_sector_read_size   = read_size;
        prev_sector_read_offset = read_offset;
        device_address          = sector_aligned_sflash_addr + SECURE_SECTOR_SIZE;
        remaining_size          -= read_size;
    }

    /* Last Block */
    result = sflash_verify_data( secure_sflash_handle->aes128_key, aes_iv, SECURE_SECTOR_SIZE, SECURE_SECTOR_DATA_SIZE,
                secure_sflash_handle->hmac_key, SECUREBOOT_SHA_KEY_SIZE, crypto_buffer, crypto_buffer, hmac_output );

    if ( result == 0 )
    {
        memcpy( data_addr_ptr, ( crypto_buffer + prev_sector_read_offset ), prev_sector_read_size );
    }

    return result;
}

int sflash_write_secure( const sflash_handle_t* const handle, unsigned long device_address, /*@observer@*/const void* const data_addr, unsigned int size )
{
    uint8_t         aes_iv[ AES_IV_LEN ];
    uint8_t*        data_ptr;
    uint32_t        write_size;
    uint32_t        write_offset;
    uint32_t        remaining_size;
    int             result = 0;
    unsigned long   sector_aligned_device_address;
    uint8_t*        read_buffer;
    securesflash_handle_t *secure_sflash_handle = handle->securesflash_handle;

    wiced_assert("sflash_write_secure : securesflash handle is null", ( secure_sflash_handle != NULL ));
    remaining_size  = size;
    data_ptr        = (uint8_t*) data_addr;


    read_buffer     = secure_sflash_handle->hwcrypto_buffer;
    while ( remaining_size != 0 )
    {
        /* aligned to SECURE_SECTOR_SIZE */
        sector_aligned_device_address = ALIGN_TO_SECTOR_ADDRESS( device_address );
        write_offset = device_address % SECURE_SECTOR_SIZE;
        write_size = MIN( remaining_size, ( SECURE_SECTOR_DATA_SIZE - write_offset ) );

        /* Read the Virtual sector */
        sflash_read( handle, sector_aligned_device_address, read_buffer, SECURE_SECTOR_SIZE );
        memset( aes_iv, 0, AES_BLOCK_SZ );
        platform_hwcrypto_aes128cbc_decrypt( secure_sflash_handle->aes128_key, 16, aes_iv, SECURE_SECTOR_SIZE, read_buffer, read_buffer );
        memcpy( (uint8_t*) read_buffer + write_offset, data_ptr, write_size );

        /* Compute Digest */
        platform_hwcrypto_sha256_hmac( secure_sflash_handle->hmac_key, 32, read_buffer, SECURE_SECTOR_DATA_SIZE, read_buffer, NULL );

        /* Encrypt */
        memset( aes_iv, 0, sizeof( aes_iv ) );
        platform_hwcrypto_aes128cbc_encrypt( secure_sflash_handle->aes128_key, AES128_KEY_LEN, aes_iv, SECURE_SECTOR_SIZE, read_buffer, read_buffer );

        /* Write the Modified sector back using sflash_write */
        sflash_sector_erase( handle, sector_aligned_device_address );
        result = sflash_write( handle, sector_aligned_device_address, read_buffer, SECURE_SECTOR_SIZE );
        SFLASH_CHECK_RESULT( result != 0, -1 );

        data_ptr        += write_size;
        remaining_size  = remaining_size - write_size;
        device_address  = sector_aligned_device_address + SECURE_SECTOR_SIZE;
    }

    return result;
}
