/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*
 * @file
 * Interface functions for Newlib libC implementation
 */


#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>
#include <malloc.h>
#include <wwd_assert.h>
//#include "platform.h"

extern char read_usart_blocking( void );
extern void write_usart_blocking( char channel, char val );

#undef errno
extern int errno;


/* sbrk
 * Increase program data space.
 * Malloc and related functions depend on this
 */
extern unsigned char _heap[];
extern unsigned char _eheap[];
static unsigned char *sbrk_heap_top = _heap;
caddr_t _sbrk( int incr )
{
    unsigned char *prev_heap;

    if ( sbrk_heap_top + incr > _eheap )
    {
        /* Out of dynamic memory heap space */

        volatile struct mallinfo mi = mallinfo();

        // See variable mi for malloc information:
        // Total allocated :  mi.uordblks
        // Total free :       mi.fordblks

        wiced_assert("Out of dynamic memory heap space", 0 != 0 );

        (void)mi;

        errno = ENOMEM;
        return (caddr_t) -1;
    }
    prev_heap = sbrk_heap_top;

    sbrk_heap_top += incr;

    return (caddr_t) prev_heap;
}

/* Override the default Newlib assert, since it tries to do printf stuff */

void __assert_func( const char * file, int line, const char * func, const char * failedexpr )
{
    /* Assertion failed!
     *
     * To find out where this assert was triggered, either look up the call stack,
     * or inspect the file, line and function parameters
     */
    wiced_assert("newlib assert", 0 != 0 );

    /* unused parameters */
    (void)file;
    (void)line;
    (void)func;
    (void)failedexpr;
}

/*
 * These are needed for C++ programs. They shouldn't really be here, so let's just
 * hit a breakpoint when these functions are called.
 */

#if 1

int _kill( int pid, int sig )
{
    wiced_assert("", 0 != 0 );
    return 0;
}

int _getpid( )
{
    wiced_assert("", 0 != 0 );
    return 0;
}

#endif
