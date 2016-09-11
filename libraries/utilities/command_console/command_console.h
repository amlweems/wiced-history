/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#pragma once


#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/
#define CMD_TABLE_END      { NULL, NULL, 0, NULL, NULL, NULL, NULL }
#define CMD_TABLE_DIV(str) { (char*) "",   NULL, 0, NULL, NULL, str,  NULL }

/******************************************************
 *                    Constants
 ******************************************************/
#define DEFAULT_0_ARGUMENT_COMMAND_ENTRY(nm, func) \
        { \
          .name         = #nm,     \
          .command      = func, \
          .arg_count    = 0,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = NULL,     \
          .brief        = NULL      \
        },

#define COMMAND_TABLE_ENDING() \
        { \
          .name         = NULL,     \
          .command      = NULL,     \
          .arg_count    = 0,        \
          .delimit      = NULL,     \
          .help_example = NULL,     \
          .format       = NULL,     \
          .brief        = NULL      \
        }

/******************************************************
 *                   Enumerations
 ******************************************************/
typedef enum
{
    ERR_CMD_OK           =  0,
    ERR_UNKNOWN          = -1,
    ERR_UNKNOWN_CMD      = -2,
    ERR_INSUFFICENT_ARGS = -3,
    ERR_TOO_MANY_ARGS    = -4,
    ERR_ADDRESS          = -5,
    ERR_NO_CMD           = -6,
    ERR_LAST_ERROR       = -7
} cmd_err_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef int       (*command_function_t)     ( int argc, char *argv[] );
typedef cmd_err_t (*help_example_function_t)( char* command_name, uint32_t eg_select );


typedef struct
{
    char* name;                             /* The command name matched at the command line. */
    command_function_t command;             /* Function that runs the command. */
    int arg_count;                          /* Minimum number of arguments. */
    char* delimit;                          /* String of characters that may delimit the arguments. */

    /*
     * These three elements are only used by the help, not the console dispatching code.
     * The default help function will not produce a help entry if both format and brief elements
     * are set to NULL (good for adding synonym or short form commands).
     */
    help_example_function_t help_example;   /* Command specific help function. Generally set to NULL. */
    char *format;                           /* String describing argument format used by the generic help generator function. */
    char *brief;                            /* Brief description of the command used by the generic help generator function. */
} command_t;


/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* Default help command which can be used as a help command handler */
int help_command_default( int argc, char* argv[] );


wiced_result_t command_console_init   ( wiced_uart_t uart, const command_t* command_table, uint32_t line_len, char* buffer, uint32_t history_len, char* history_buffer_ptr );
wiced_result_t command_console_deinit ( void );

#ifdef __cplusplus
}
#endif
