/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
//#include "platform_cmsis.h"
#include <stdint.h>
#include <stdlib.h>
#include <yfuns.h>
#include <wwd_assert.h>

int errno;
extern void platform_stdio_write( const char* str, uint32_t len );
extern void platform_stdio_read( char* str, uint32_t len );

size_t __write( int handle, const unsigned char * buffer, size_t size )
{

    if ( buffer == 0 )
    {
        return 0;
    }

    platform_stdio_write( (const char*)buffer, size );

    return size;
}

size_t __read( int handle, unsigned char * buffer, size_t size )
{

    if ( buffer == 0 )
    {
        return 0;
    }

    platform_stdio_read( (char*)buffer, size );

    return size;
}

/* Stubbed for now. */
long __lseek(int handle, long offset, int whence)
{
  wiced_assert( "unimplemented", 0 != 0 );
  return -1;
}

/* Stubbed for now. */
int __close(int handle)
{
  wiced_assert( "unimplemented", 0 != 0 );
  return 0;
}

/* Stubbed for now. */
int remove(const char * filename)
{
  wiced_assert( "unimplemented", 0 != 0 );
  return 0;
}
