/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/* For IAR, the <malloc.h> is implemented with advanced
 * heap handling which is defined in <iar_dlmalloc.h>.
 * The corresponding function name has a __iar_dlxxx prefix
 */
#if defined( __GNUC__ )
#include <malloc.h>
#define MALLINFO mallinfo
#elif defined ( __IAR_SYSTEMS_ICC__ )
#include <iar_dlmalloc.h>
#define MALLINFO __iar_dlmallinfo
#endif

#include <stdio.h>
#include "command_console.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

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
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/*!
 ******************************************************************************
 * Print memory allocation information.
 *
 * @param[in] argc  Unused.
 * @param[in] argv  Unused.
 *
 * @return    Console error code indicating if the command ran correctly.
 */

int malloc_info_command( int argc, char *argv[] )
{
    volatile struct mallinfo mi = MALLINFO( );

    printf( "malloc_info {\r\n"
            "\tarena:   \t%5d;\t/* total space allocated from system */\r\n", mi.arena );
    printf( "\tordblks: \t%5d;\t/* number of non-inuse chunks */\r\n", mi.ordblks );
    printf( "\tsmblks:  \t%5d;\t/* unused -- always zero */\r\n", mi.smblks );
    printf( "\thblks:   \t%5d;\t/* number of mmapped regions */\r\n", mi.hblks );
    printf( "\thblkhd:  \t%5d;\t/* total space in mmapped regions */\r\n", mi.hblkhd );
    printf( "\tusmblks: \t%5d;\t/* unused -- always zero */\r\n", mi.usmblks );
    printf( "\tfsmblks: \t%5d;\t/* unused -- always zero */\r\n", mi.fsmblks );
    printf( "\tuordblks:\t%5d;\t/* total allocated space */\r\n", mi.uordblks );
    printf( "\tfordblks:\t%5d;\t/* total non-inuse space */\r\n", mi.fordblks );
    printf( "\tkeepcost:\t%5d;\t/* top-most, releasable (via malloc_trim) space */\r\n"
            "};\r\n", mi.keepcost );

    return ERR_CMD_OK;
} /* malloc_info_command */
