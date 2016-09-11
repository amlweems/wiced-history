/*
 * Copyright 2014, Broadcom Corporation
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

#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>
#include "stdio_newlib.h"

#undef errno
extern int errno;

#include "wiced_defaults.h"
#include "platform_common_config.h"
#include "wiced_platform.h"

#ifndef EBADF
#include <errno.h>
#endif

int _close( int file )
{
    (void) file; /* unused parameter */
    return -1;
}


/* fstat
 * Status of an open file. For consistency with other minimal implementations in these examples,
 * all files are regarded as character special devices.
 * The `sys/stat.h' header file required is distributed in the `include' subdirectory for this C library.
 */

int _fstat( int file, struct stat *st )
{
    (void) file; /* unused parameter */

    st->st_mode = S_IFCHR;
    return 0;
}




/* isatty
 * Query whether output stream is a terminal. For consistency with the other minimal implementations,
 */

int _isatty( int file )
{
    switch ( file )
    {
        case STDOUT_FILENO:
        case STDERR_FILENO:
        case STDIN_FILENO:
            return 1;
        default:
            /* errno = ENOTTY; */
            errno = EBADF;
            return 0;
    }
}



/* lseek - Set position in a file. Minimal implementation: */

int _lseek( int file, int ptr, int dir )
{
    (void) file; /* unused parameter */
    (void) ptr;  /* unused parameter */
    (void) dir;  /* unused parameter */
    return 0;
}





/* read
 * Read a character to a file. `libc' subroutines will use this system routine for input from all files, including stdin
 * Returns -1 on error or blocks until the number of characters have been read.
 */

int _read( int file, char *ptr, int len )
{
    switch ( file )
    {
        case STDIN_FILENO:
            platform_stdio_read( ptr, len );
            break;
        default:
            errno = EBADF;
            return -1;
    }
    return len;
}

/* write
 * Write a character to a file. `libc' subroutines will use this system routine for output to all files, including stdout
 * Returns -1 on error or number of bytes sent
 */

int _write( int file, char *ptr, int len )
{
    char channel;
    switch ( file )
    {
        case STDOUT_FILENO: /*stdout*/
            channel = 0;
            break;
        case STDERR_FILENO: /* stderr */
            channel = 1;
            break;
        default:
            errno = EBADF;
            return -1;
    }

    UNUSED_PARAMETER( channel );
    platform_stdio_write( ptr, len );
    return len;
}





#if 0
/* environ
 * A pointer to a list of environment variables and their values.
 * For a minimal environment, this empty list is adequate:
 */

char *__env[1] =
{   0};
char **environ = __env;

/* getpid - Process-ID; this is sometimes used to generate strings unlikely to conflict with other processes. Minimal implementation, for a system without processes: */

int _getpid()
{
    return 1;
}

/* kill - Send a signal. Minimal implementation: */

int _kill(int pid, int sig)
{
    errno = EINVAL;
    return (-1);
}

/* link - Establish a new name for an existing file. Minimal implementation: */

int _link(char *old, char *new)
{
    errno = EMLINK;
    return -1;
}

/* stat
 * Status of a file (by name). Minimal implementation:
 * int    _EXFUN(stat,( const char *__path, struct stat *__sbuf ));
 */

int _stat(const char *filepath, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}

times - Timing information for current process. Minimal implementation:

clock_t _times(struct tms *buf)
{
    return -1;
}

/* unlink - Remove a file's directory entry. Minimal implementation: */

int _unlink(char *name)
{
    errno = ENOENT;
    return -1;
}

/* wait - Wait for a child process. Minimal implementation: */

int _wait(int *status)
{
    errno = ECHILD;
    return -1;
}

void _exit(int status)
{
    _write(1, "exit", 4);
    while (1)
    {
        ;
    }
}

/* execve - Transfer control to a new process. Minimal implementation (for a system without processes): */

int _execve(char *name, char **argv, char **env)
{
    errno = ENOMEM;
    return -1;
}

/* fork - Create a new process. Minimal implementation (for a system without processes): */

int _fork()
{
    errno = EAGAIN;
    return -1;
}
#endif /* if 0 */

