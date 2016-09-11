/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include "Network/wwd_buffer_interface.h"
#include "lwip/netbuf.h"
#include "FreeRTOS.h"
#include "lwip/memp.h"
#include "task.h"
#include <string.h> /* for NULL */
#include "wwd_assert.h"

wiced_result_t host_buffer_init( /*@unused@*/ void * native_arg )
{
    /*@-noeffect@*/
    UNUSED_PARAMETER( native_arg );
    /*@+noeffect@*/
    return WICED_SUCCESS;
}


wiced_result_t host_buffer_check_leaked( void )
{
    wiced_assert( "pbuf TX pool Buffer leakage", memp_in_use( MEMP_PBUF_POOL_TX ) == 0 );
    wiced_assert( "pbuf RX pool Buffer leakage", memp_in_use( MEMP_PBUF_POOL_RX ) == 0 );
    wiced_assert( "pbuf ref/rom Buffer leakage", memp_in_use( MEMP_PBUF ) == 0 );
    return WICED_SUCCESS;
}


wiced_result_t host_buffer_get(  /*@out@*/ wiced_buffer_t * buffer, /*@unused@*/ wiced_buffer_dir_t direction, unsigned short size, wiced_bool_t wait )
{
    /*@-noeffect@*/
    UNUSED_PARAMETER( direction );
    /*@+noeffect@*/
    wiced_assert("Error: Invalid buffer size\r\n", size != 0);

    *buffer = NULL;

    if ( size > (unsigned short) WICED_LINK_MTU )
    {
        WPRINT_NETWORK_DEBUG(("Attempt to allocate a buffer larger than the MTU of the link\r\n"));
        return WICED_BUFFER_UNAVAILABLE_PERMANENT;
    }

    do
    {
        *buffer = pbuf_alloc( PBUF_RAW, size, ( direction == WICED_NETWORK_RX ) ? PBUF_POOL_RX : PBUF_POOL_TX );
    } while ( ( *buffer == NULL ) &&
              ( wait == WICED_TRUE ) &&
              ( vTaskDelay( (portTickType) 1 ), 1 == 1 ) );
    if ( *buffer == NULL )
    {
#if 0
        WPRINT_NETWORK_DEBUG(("Failed to allocate packet buffer\r\n"));
#endif /* if 0 */
        return WICED_BUFFER_UNAVAILABLE_TEMPORARY;
    }

    return WICED_SUCCESS;
}

void host_buffer_release( /*@only@*/ wiced_buffer_t buffer, wiced_buffer_dir_t direction )
{
    /*@-noeffect@*/
    UNUSED_PARAMETER( direction );
    /*@+noeffect@*/
    wiced_assert("Error: Invalid buffer\r\n", buffer != NULL);
    (void) pbuf_free( buffer ); /* Ignore returned number of freed segments since TCP packets will still be referenced by LWIP after relase by WICED */
}

/*@exposed@*/ uint8_t* host_buffer_get_current_piece_data_pointer( wiced_buffer_t buffer )
{
    wiced_assert("Error: Invalid buffer\r\n", buffer != NULL);
    return (uint8_t*) buffer->payload;
}

uint16_t host_buffer_get_current_piece_size( wiced_buffer_t buffer )
{
    wiced_assert("Error: Invalid buffer\r\n", buffer != NULL);
    return (uint16_t) buffer->len;
}

/*@exposed@*/ /*@null@*/ wiced_buffer_t host_buffer_get_next_piece( wiced_buffer_t buffer )
{
    wiced_assert("Error: Invalid buffer\r\n", buffer != NULL);
    return buffer->next;
}

wiced_result_t host_buffer_add_remove_at_front( wiced_buffer_t * buffer, int32_t add_remove_amount )
{
    wiced_assert("Error: Invalid buffer\r\n", buffer != NULL);
    if ( (u8_t) 0 != pbuf_header( *buffer, (s16_t) ( -add_remove_amount ) ) )
    {
        WPRINT_NETWORK_DEBUG(("Failed to move pointer - usually because not enough space at front of buffer\r\n"));
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

wiced_result_t host_buffer_set_data_end( wiced_buffer_t buffer, uint8_t* end_of_data )
{
    buffer->len = (uint16_t) ( end_of_data - ( (uint8_t*) buffer->payload ) );
    buffer->tot_len = buffer->len;
    return WICED_SUCCESS;
}
