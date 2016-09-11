/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wiced.h"
#include "command_console.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define MAX_LINE_LENGTH    (128)
#define MAX_HISTORY_LENGTH (20)
#define DELIMIT            ((char*) " ")
#define MAX_PARAMS         (16)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef struct
{

    wiced_bool_t        quit;
    wiced_uart_t        uart;

    const command_t*    command_table;
    uint32_t            console_line_len;
    uint8_t*            console_buffer;
    uint32_t            console_cursor_position;
    uint32_t            console_current_line;
    wiced_bool_t        console_in_esc_seq;
    char                console_esc_seq_last_char;
    uint8_t*            history_buffer;
    uint32_t            history_length;
    uint32_t            history_num_lines;
    uint32_t            history_newest_line;
    wiced_ring_buffer_t console_rx_ring_buffer;
    wiced_thread_t      console_thread;
    wiced_semaphore_t   console_quit_semaphore;
    wiced_bool_t        in_process_char;
    wiced_bool_t        console_in_tab_tab;
} command_console_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
static void         history_load_line         ( uint32_t line, char* buffer );
static void         history_store_line        ( char* buffer );
static uint32_t     history_get_num_lines     ( void );
static wiced_bool_t console_process_esc_seq   ( char c );
static cmd_err_t    console_parse_cmd         ( char* line );
static void         console_load_line         ( uint32_t new_line );
static void         console_insert_char       ( char c );
static void         console_remove_char       ( void );

/* forward declarations */
static void console_do_home      ( void );
static void console_do_end       ( void );
static void console_do_up        ( void );
static void console_do_down      ( void );
static void console_do_left      ( void );
static void console_do_right     ( void );
static void console_do_delete    ( void );
static void console_do_backspace ( void );
static void console_do_tab       ( void );
static cmd_err_t console_do_enter( void );
static cmd_err_t console_do_newline_without_command_repeat( void );

static void send_char( char c );
static void send_str( char* s );
static void send_charstr( char* s );
static cmd_err_t console_process_char(char c);
static int help_command( int argc, char* argv[] );


/******************************************************
 *               Variables Definitions
 ******************************************************/
/* default error strings */
static const char* const console_default_error_strings[] = {
    (char*) "OK",
    (char*) "Unknown Error",
    (char*) "Unknown Command",
    (char*) "Insufficient Arguments",
    (char*) "Too Many Arguments",
    (char*) "Bad Address Value",
    (char*) "No Command Entered"
};

wiced_uart_config_t uart_console_config = {
    .baud_rate    = 115200,
    .data_width   = DATA_WIDTH_8BIT,
    .parity       = NO_PARITY,
    .stop_bits    = STOP_BITS_1,
    .flow_control = FLOW_CONTROL_DISABLED,
};

static const char* const *console_error_strings = console_default_error_strings;
static char* console_bell_string = (char*) "\a";
static char* console_delimit_string = DELIMIT;
static char* console_prompt_string = (char*) "> ";

static void (*tab_handling_func)( void ) = console_do_tab;
static cmd_err_t (*newline_handling_func)( void ) = console_do_newline_without_command_repeat;

uint8_t console_rx_ring_data[64];
extern const platform_uart_config_t stdio_config;
command_console_t cons;


/******************************************************
 *               Function Definitions
 ******************************************************/


/*!
 ******************************************************************************
 * Load a line of history out of the store into the supplied buffer.
 * To change the way the command history is stored, for example to move it to NVM
 * or flash, patch the 3 History functions.
 *
 * @param[in] line    The number of the line to be loaded.
 * @param[in] buffer  Buffer to load the line into.
 *
 * @return  void
 */

static void history_load_line( uint32_t line, char* buffer )
{
    if ( ( line >= cons.history_num_lines ) )
    {
        buffer[0] = 0;
    }
    else
    {
        uint32_t actual_line = ( cons.history_newest_line + cons.history_length - line ) % cons.history_length;
        strncpy( buffer, &cons.history_buffer[actual_line * cons.console_line_len], cons.console_line_len );
    }
}

/*!
 ******************************************************************************
 * Store a command line into the history. To change the way the command history is
 * stored, for example to move it to NVM or flash, patch the 3 History functions.
 *
 * @param[in] buffer  The line to be stored.
 *
 * @return  void
 */

static void history_store_line( char* buffer )
{
    if ( cons.history_length > 0 )
    {
        cons.history_newest_line = ( cons.history_newest_line + 1 ) % cons.history_length;
        strncpy( &cons.history_buffer[cons.history_newest_line * cons.console_line_len], buffer, cons.console_line_len );
        if ( cons.history_num_lines < cons.history_length )
        {
            cons.history_num_lines++;
        }
    }
}

/*!
 ******************************************************************************
 * Ask the history store how many lines of history are currently stored.
 * To change the way the command history is stored, for example to move it to NVM
 * or flash, patch the 3 History functions.
 *
 * @return  The number of currently stored lines.
 */

static uint32_t history_get_num_lines( void )
{
    return cons.history_num_lines;
}

/*!
 ******************************************************************************
 * Print a single character.
 *
 * @param[in] c  The string to be printed.
 *
 * @return  void
 */

static void send_char( char c )
{
    wiced_result_t result;
    result = wiced_uart_transmit_bytes( cons.uart, c, 1 );
    wiced_assert("", result == WICED_SUCCESS);
}

/*!
 ******************************************************************************
 * Print a string.
 *
 * @param[in] s  The string to be printed.
 *
 * @return  void
 */

static void send_str( char* s )
{
    wiced_result_t result;
    result = wiced_uart_transmit_bytes( cons.uart, s, strlen(s) );
    wiced_assert("", result == WICED_SUCCESS);
}

/*!
 ******************************************************************************
 * Print a string character at a time (gets around eCos not printing strings with non printable characters).
 *
 * @param[in] s  The string to be printed.
 *
 * @return  void
 */

static void send_charstr( char* s )
{
    wiced_result_t result;
    while ( *s != 0 )
    {
        result = wiced_uart_transmit_bytes( cons.uart, *s, 1 );
        s++;
    }
}



void console_thread_func( uint32_t arg )
{
    wiced_result_t result;
    uint8_t received_character;
    UNUSED_PARAMETER(result);
    for(;;)
    {
        result = wiced_uart_receive_bytes( cons.uart, &received_character, 1, 1000 );
        if( result == WICED_SUCCESS )
        {
            console_process_char( received_character );
        }
        else
        {
            if( cons.quit == WICED_TRUE )
            {
                wiced_rtos_set_semaphore(&cons.console_quit_semaphore);
                break;
            }
        }
    }
}

wiced_result_t command_console_init( wiced_uart_t uart, const command_t* command_table, uint32_t line_len, char* buffer, uint32_t history_len, char* history_buffer_ptr )
{
    wiced_result_t result;
    UNUSED_PARAMETER(result);

    cons.command_table  = command_table;
    cons.console_line_len   = line_len;
    cons.console_buffer = buffer;

    cons.console_buffer[0] = 0;
    cons.console_cursor_position = 0;
    cons.console_current_line = 0;
    cons.console_in_esc_seq = WICED_FALSE;
    cons.console_esc_seq_last_char = ' ';

    cons.history_buffer = history_buffer_ptr;
    cons.history_length = history_len;
    cons.history_num_lines = 0;
    cons.history_newest_line = 0;
    cons.quit = WICED_FALSE;

    if( uart != STDIO_UART )
    {
        /* Init uart the same as stdio uart configuration */
        ring_buffer_init( (wiced_ring_buffer_t*) &cons.console_rx_ring_buffer, (uint8_t*) console_rx_ring_data, sizeof(console_rx_ring_data) );
        result = wiced_uart_init(uart, &uart_console_config, &cons.console_rx_ring_buffer);
    }

    wiced_rtos_init_semaphore(&cons.console_quit_semaphore);

    WPRINT_APP_INFO(("%s", console_prompt_string));

    /* create a console thread */
    result = wiced_rtos_create_thread( &cons.console_thread, 3, "console", console_thread_func, 4096, NULL);

    return result;

}

wiced_result_t command_console_deinit(void)
{
    wiced_result_t result;
    cons.quit = WICED_TRUE;

    /* Wait on a semaphore till the console thread is ready to quit */
    result = wiced_rtos_get_semaphore(&cons.console_quit_semaphore, 1000);
    if( result != WICED_SUCCESS )
    {
        return result;
    }
    wiced_rtos_thread_join(&cons.console_thread);
    result = wiced_rtos_delete_thread( &cons.console_thread);
    wiced_rtos_deinit_semaphore(&cons.console_quit_semaphore);
    if( cons.uart != STDIO_UART )
    {
        wiced_uart_deinit(cons.uart);
    }
    return WICED_SUCCESS;
}

/*!
 ******************************************************************************
 * Load a line from the history into the current line and update the screen.
 * This calls HistoryLoadLine to interface with the history store.
 *
 * @param[in] new_line The index of the line to load from the history.
 *
 * @return    void
 */

static void console_load_line( uint32_t new_line )
{
    uint32_t i;
    uint32_t old_len;
    uint32_t new_len;

    old_len = strlen( cons.console_buffer );

    if ( new_line > 0 )
    {
        history_load_line( new_line - 1, cons.console_buffer );
    }
    else
    {
        cons.console_buffer[0] = 0;
    }
    new_len = strlen( cons.console_buffer );

    /* seek to the start of the line */
    send_char('\r');

    /* print out the prompt and new line */
    send_str( console_prompt_string );
    send_str( cons.console_buffer );

    /* write spaces over the rest of the old line */
    for ( i = new_len; i < old_len; i++ )
    {
        send_char( ' ' );
    }

    /* then move back the same amount */
    for ( i = new_len; i < old_len; i++ )
    {
        send_char( '\b' );
    }

    cons.console_current_line = new_line;

    /* position the cursor at the end of the line */
    cons.console_cursor_position = new_len;
}

/*!
 ******************************************************************************
 * Insert a character into the current line at the cursor position and update the screen.
 *
 * @param[in] c  The character to be inserted.
 *
 * @return    void
 */

static void console_insert_char( char c )
{
    uint32_t i;
    uint32_t len = strlen( cons.console_buffer );

    /* move the end of the line out to make space */
    for ( i = len + 1; i > cons.console_cursor_position; i-- )
    {
        cons.console_buffer[i] = cons.console_buffer[i - 1];
    }

    /* insert the character */
    len++;
    cons.console_buffer[cons.console_cursor_position] = c;

    /* print out the modified part of the ConsoleBuffer */
    send_str( &cons.console_buffer[cons.console_cursor_position] );

    /* move the cursor back to where it's supposed to be */
    cons.console_cursor_position++;
    for ( i = len; i > cons.console_cursor_position; i-- )
    {
        send_char( '\b' );
    }
}

/*!
 ******************************************************************************
 * Remove a character from the current line at the cursor position and update the screen.
 *
 * @return    void
 */

static void console_remove_char( void )
{
    uint32_t i;
    uint32_t len = strlen( cons.console_buffer );

    /* back the rest of the line up a character */
    for ( i = cons.console_cursor_position; i < len; i++ )
    {
        cons.console_buffer[i] = cons.console_buffer[i + 1];
    }
    len--;

    /* print out the modified part of the ConsoleBuffer */
    send_str( &cons.console_buffer[cons.console_cursor_position] );

    /* overwrite the extra character at the end */
    send_charstr( (char*) " \b" );

    /* move the cursor back to where it's supposed to be */
    for ( i = len; i > cons.console_cursor_position; i-- )
    {
        send_char( '\b' );
    }
}

/*!
 ******************************************************************************
 * Handle a character or escape sequence representing a goto home operation.
 *
 * @return  void
 */

static void console_do_home( void )
{
    cons.console_cursor_position = 0;
    send_char('\r');
    send_str(console_prompt_string);
}

/*!
 ******************************************************************************
 * Handle a character or escape sequence representing an goto end operation.
 *
 * @return  void
 */

static void console_do_end( void )
{
    send_str( &cons.console_buffer[cons.console_cursor_position] );
    cons.console_cursor_position = strlen( cons.console_buffer );
}

/*!
 ******************************************************************************
 * Handle a character or escape sequence representing an up cursor operation.
 *
 * @return  void
 */

static void console_do_up( void )
{
    if ( cons.console_current_line < history_get_num_lines( ) )
    {
        console_load_line( cons.console_current_line + 1 );
    }
    else
    {
        send_charstr( console_bell_string );
    }
}

/*!
 ******************************************************************************
 * Handle a character or escape sequence representing a down cursor operation.
 *
 * @return  void
 */

static void console_do_down( void )
{
    if ( cons.console_current_line > 0 )
    {
        console_load_line( cons.console_current_line - 1 );
    }
    else
    {
        send_charstr( console_bell_string );
    }
}

/*!
 ******************************************************************************
 * Handle a character or escape sequence representing a left cursor operation.
 *
 * @return  void
 */

void console_do_left( void )
{
    if ( cons.console_cursor_position > 0 )
    {
        send_char( '\b' );
        cons.console_cursor_position--;
    }
    else
    {
        send_charstr( console_bell_string );
    }
}

/*!
 ******************************************************************************
 * Handle a character or escape sequence representing a right cursor operation.
 *
 * @return  void
 */

void console_do_right( void )
{
    if ( cons.console_cursor_position < strlen( cons.console_buffer ) )
    {
        send_char( cons.console_buffer[cons.console_cursor_position] );
        cons.console_cursor_position++;
    }
    else
    {
        send_charstr( console_bell_string );
    }
}

/*!
 ******************************************************************************
 * Handle a character or escape sequence representing a delete operation.
 *
 * @return  void
 */

void console_do_delete( void )
{
    if ( cons.console_cursor_position < strlen( cons.console_buffer ) )
    {
        cons.console_current_line = 0;
        console_remove_char( );
    }
    else
    {
        send_charstr( console_bell_string );
    }
}

/*!
 ******************************************************************************
 * Handle a character or escape sequence representing a backspace operation.
 *
 * @return  void
 */

void console_do_backspace( void )
{
    if ( cons.console_cursor_position > 0 )
    {
        cons.console_current_line = 0;
        cons.console_cursor_position--;
        send_char( '\b' );
        console_remove_char( );
    }
    else
    {
        send_charstr( console_bell_string );
    }
}

/*!
 ******************************************************************************
 * Handle a character or escape sequence representing a new line operation.
 *
 * If the current line is not empty then it's contents will be parsed and invoked.
 * If the current line is empty then the last command will be reinvoked.
 *
 * @return    Console error code indicating if the command was parsed and run correctly.
 */

cmd_err_t console_do_enter( void )
{
    cmd_err_t err = ERR_CMD_OK;

    cons.console_current_line = 0;
    send_str( (char*) "\r\n" );

    if ( strlen( cons.console_buffer ) ) /* if theres something in the buffer */
    {
        if ( strlen( cons.console_buffer ) > strspn( cons.console_buffer, console_delimit_string ) ) /* and it's not just delimit characters */
        {
            /* store it and invoke the command parser */
            history_store_line( cons.console_buffer );
            err = console_parse_cmd( cons.console_buffer );
        }
    }
    else
    {
        if ( history_get_num_lines( ) ) /* if theres a previous history line */
        {
            /* load it and invoke the command parser */
            history_load_line( 0, cons.console_buffer );
            err = console_parse_cmd( cons.console_buffer );
        }
    }

    /* prepare for a new line of entry */
    cons.console_buffer[0] = 0;
    cons.console_cursor_position = 0;
    send_str( console_prompt_string );

    return err;
}

/*!
 ******************************************************************************
 * Process escape sequences.
 *
 * This function doesn't process the sequences properly, but does handle all the common
 * ones. The ConsoleEscSeqLastChar variable is used to parse vt320 like sequences
 * without implementing sequence paramater parsing. In general the function doesn't
 * distinguish between the esc-[ and esc-O codes. Despite this the console functions are
 * setup to make sure that any failure doesn't produce non printable characters, so failures
 * should be soft.
 *
 * If you want to handle additional escape sequences then create a wraper patch that has
 * essentially the same code structure as this function and calls this one in its default case(s).
 *
 * @param[in] c  The incomming character.
 *
 * @return    Boolean indicating if more characters are needed to complete the sequence.
 */

wiced_bool_t console_process_esc_seq( char c )
{
    wiced_bool_t still_in_esc_seq = WICED_FALSE;

    switch ( c )
    {
        case ';':
        case '[':
        case 'O':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            still_in_esc_seq = WICED_TRUE;
            break;
        case 'A': /* up arrow */
            console_do_up( );
            break;
        case 'B': /* down arrow */
            console_do_down( );
            break;
        case 'C': /* right arrow */
            console_do_right( );
            break;
        case 'D': /* left arrow */
            console_do_left( );
            break;
        case 'F': /* end */
            console_do_end( );
            break;
        case 'H': /* home */
            console_do_home( );
            break;
        case '~': /* vt320 style control codes */
            switch ( cons.console_esc_seq_last_char )
            {
                case '1': /* home */
                    console_do_home( );
                    break;
                case '3': /* delete */
                    console_do_delete( );
                    break;
                case '4': /* end */
                    console_do_end( );
                    break;
                default:
                    send_charstr( console_bell_string ); /* unknown escape sequence we are probably in trouble, beep and pretend it's going to be ok */
                    break;
            }
            break;
        default:
            send_charstr( console_bell_string ); /* unknown escape sequence we are probably in trouble, beep and pretend it's going to be ok */
            break;
    }

    cons.console_esc_seq_last_char = c;

    return still_in_esc_seq;
}

/*!
 ******************************************************************************
 * Break an input line into command and arguments and invoke it.
 *
 * @param[in] line  The line of input.
 *
 * @return    Console error code indicating if the command was parsed correctly.
 */

cmd_err_t console_parse_cmd( char* line )
{
    const command_t* cmd_ptr = NULL;
    char* params[MAX_PARAMS];
    uint32_t param_cnt = 0;

    cmd_err_t err = ERR_CMD_OK;

    /* First call to strtok. */
    params[param_cnt++] = strtok( line, console_delimit_string );

    if ( params[0] == NULL ) /* no command entered */
    {
        err = ERR_NO_CMD;
    }
    else
    {
        /* find the command */
        for ( cmd_ptr = cons.command_table; cmd_ptr->name != NULL; cmd_ptr++ )
        {
            if ( strcmp( params[0], cmd_ptr->name ) == 0 )
            {
                break;
            }
        }

        if ( cmd_ptr->name == NULL )
        {
            err = ERR_UNKNOWN_CMD;
        }
        else
        {
            /* determine argument delimit string */
            char* delimit;
            if ( cmd_ptr->delimit != NULL )
            {
                delimit = cmd_ptr->delimit;
            }
            else
            {
                delimit = console_delimit_string;
            }

            /* parse arguments */
            while ( ( ( params[param_cnt] = strtok( NULL, delimit ) ) != NULL ) )
            {
                param_cnt++;
                if ( param_cnt > ( MAX_PARAMS - 1 ) )
                {
                    err = ERR_TOO_MANY_ARGS;
                    break;
                }
            }

            /* check arguments */
            if ( ( param_cnt - 1 ) < cmd_ptr->arg_count )
            {
                err = ERR_INSUFFICENT_ARGS;
            }

            /* run command */
            if ( ( err == ERR_CMD_OK ) && ( cmd_ptr->command != NULL ) )
            {
//                platform_enable_mcu_powersave();

                err = (cmd_err_t) cmd_ptr->command( param_cnt, params );

//                platform_disable_mcu_powersave();
            }
        }

        /* process errors */
        if ( err != ERR_CMD_OK )
        {
            if ((err <=0) && (err > ERR_LAST_ERROR))
            {
                printf( "ERROR: %s\r\n", console_error_strings[-err] );
            }
            if ( err != ERR_UNKNOWN_CMD )
            {
                printf("Usage: %s %s\r\n",cmd_ptr->name, cmd_ptr->format);
            }
        }
    }

    return err;
}



/* default help function */
int help_command_default( int argc, char* argv[] )
{
    const command_t* cmd_ptr;
    cmd_err_t err = ERR_CMD_OK;
    uint32_t eg_sel = 0;

    switch ( argc )
    {
        case 0:
        case 1:
            printf( "Console Commands:\r\n" );
            for ( cmd_ptr = cons.command_table; cmd_ptr->name != NULL; cmd_ptr++ )
            {
                if ( ( cmd_ptr->format != NULL ) || ( cmd_ptr->brief != NULL ) )
                {
                    if ( cmd_ptr->format != NULL )
                    {
                        printf( "    %s%c%s\r\n", cmd_ptr->name, console_delimit_string[0], cmd_ptr->format );
                    }
                    else
                    {
                        printf( "    %s\r\n", cmd_ptr->name );
                    }

                    if ( cmd_ptr->brief != NULL )
                    {
                        printf( "        - %s\r\n", cmd_ptr->brief );
                    }
                }
            }
            break;
        default: /* greater than 2 */
            eg_sel = str_to_int( argv[2] );
            /* Disables Eclipse static analysis warning */
            /* Intentional drop through */
            /* no break */
        case 2:
            err = ERR_UNKNOWN_CMD;
            for ( cmd_ptr = cons.command_table; cmd_ptr->name != NULL; cmd_ptr++ )
            {
                if ( strcmp( argv[1], cmd_ptr->name ) == 0 )
                {
                    if ( cmd_ptr->help_example != NULL )
                    {
                        err = cmd_ptr->help_example( argv[1], eg_sel );
                    }
                    else
                    {
                        err = ERR_CMD_OK;
                        printf( "No example available for %s\r\n\r\n", argv[1] );
                    }
                }
            }
            break;
    }

    return err;
}

/*!
 ******************************************************************************
 * Handle a character or escape sequence representing a tab completion operation.
 *
 * @return  void
 */

void console_do_tab( void )
{
    uint32_t buf_len = strlen( cons.console_buffer );
    const command_t* cmd_ptr = NULL;

    console_do_end( );

    if ( cons.console_in_tab_tab == WICED_FALSE )
    {
        char *src = NULL;
        wiced_bool_t single_match = WICED_FALSE;
        uint32_t len = 0;

        /* for each where the buffer matches it's start */
        for ( cmd_ptr = cons.command_table; cmd_ptr->name != NULL; cmd_ptr++ )
        {
            if ( strncmp( cmd_ptr->name, cons.console_buffer, buf_len ) == 0 )
            {
                /* if we already have one or more matches then the completion is the longest common prefix */
                if ( src )
                {
                    single_match = WICED_FALSE;
                    uint32_t i = buf_len;
                    while ( ( i < len ) && ( src[i] == cmd_ptr->name[i] ) )
                    {
                        i++;
                    }
                    len = i;
                }
                /* for the first match the completion is the whole command */
                else
                {
                    single_match = WICED_TRUE;
                    src = cmd_ptr->name;
                    len = strlen( cmd_ptr->name );
                }
            }
        }

        /* if there is a valid completion then add it to the buffer */
        if ( src && ( ( len > strlen( cons.console_buffer ) ) || single_match ) )
        {
            uint32_t i;
            for ( i = buf_len; i < len; i++ )
            {
                console_insert_char( src[i] );
            }
            if ( single_match )
            {
                console_insert_char( ' ' );
            }
        }
        else
        {
            cons.console_in_tab_tab = WICED_TRUE;
        }
    }
    else
    {
        uint32_t cnt = 0;

        for ( cmd_ptr = cons.command_table; cmd_ptr->name != NULL; cmd_ptr++ )
        {
            if ( ( strncmp( cmd_ptr->name, cons.console_buffer, buf_len ) == 0 ) && ( strcmp( cmd_ptr->name, "" ) != 0 ) )
            {
                if ( ( cnt % 4 ) == 0 )
                {
                    send_str( (char*) "\r\n" );
                }
                else
                {
                    send_char( ' ' );
                }
                printf( "%-19.19s", cmd_ptr->name );
                cnt++;
            }
        }
        if ( cnt )
        {
            send_str( (char*) "\r\n" );
            send_str( console_prompt_string );
            send_str( cons.console_buffer );
        }
        cons.console_in_tab_tab = WICED_FALSE;
    }
}

/*!
 ******************************************************************************
 * Version of the console newline handler that does not repeat previous command
 * when newline pressed on empty line.
 */

cmd_err_t console_do_newline_without_command_repeat( void )
{
    cmd_err_t err = ERR_CMD_OK;

    if ( strlen( cons.console_buffer ) ) /* if theres something in the buffer */
    {
        err = console_do_enter( );
    }
    else
    {
        /* prepare for a new line of entry */
        cons.console_buffer[0] = 0;
        cons.console_cursor_position = 0;
        send_str( (char*) "\r\n" );
        send_str( console_prompt_string );
    }
    return err;
}

/*!
 ******************************************************************************
 * Process regular characters.
 *
 * If you want to handle additional characters then create a wraper patch that has essentially
 * the same code structure as this function and calls this one in its default case(s).
 *
 * @param[in] c  The incoming character.
 *
 * @return    Console error code indicating if the character and any resulting command were processed correctly.
 */

cmd_err_t console_process_char( char c )
{
    cmd_err_t err = ERR_CMD_OK;
    cons.in_process_char = WICED_TRUE;

    /* printf("ConsoleProcessChar\r\n"); */
    if ( cons.console_in_esc_seq )
    {
        cons.console_in_esc_seq = console_process_esc_seq( c );
    }
    else
    {
        /* printf("%lu %lu\r\n", (uint32_t)c, (uint32_t)'\b'); */
        switch ( c )
        {
            case 9: /* tab char */
                if ( tab_handling_func )
                {
                    tab_handling_func( );
                }
                break;
            case 10: /* line feed */
                /* ignore it */
                break;
            case '\r': /* newline */
                if ( newline_handling_func )
                {
                    err = newline_handling_func( );
                }
                break;
            case 27: /* escape char */
                cons.console_in_esc_seq = WICED_TRUE;
                break;
            case '\b': /* backspace */
            case '\x7F': /* backspace */
                console_do_backspace( );
                break;
            case 16: /* ctrl-p */
                console_do_up( );
                break;
            case 14: /* ctrl-n */
                console_do_down( );
                break;
            case 2: /* ctrl-b */
                console_do_left( );
                break;
            case 6: /* ctrl-f */
                console_do_right( );
                break;
            case 1: /* ctrl-a */
                console_do_home( );
                break;
            case 5: /* ctrl-e */
                console_do_end( );
                break;
            case 4: /* ctrl-d */
                console_do_delete( );
                break;
            default:
                if ( ( c > 31 ) && ( c < 127 ) )
                { /* limit to printables */
                    if ( strlen( cons.console_buffer ) + 1 < cons.console_line_len )
                    {
                        cons.console_current_line = 0;
                        console_insert_char( c );
                    }
                    else
                    {
                        send_charstr( console_bell_string );
                    }
                }
                else
                {
                    send_charstr( console_bell_string );
                }
                break;
        }
    }

    cons.in_process_char = WICED_FALSE;
    return err;
}

