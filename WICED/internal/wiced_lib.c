/*
 * Copyright 2015, Broadcom Corporation
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

#include "wiced_utilities.h"
#include <string.h>
#include "wiced_resource.h"
#include "wwd_debug.h"
#include "platform_resource.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define IS_DIGIT(c) ((c >= '0') && (c <= '9'))

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

static uint8_t string_to_generic( const char* string, uint8_t str_length,  uint32_t* value_out, uint8_t is_unsigned, uint8_t is_hex )
{
    uint8_t nibble;
    uint8_t characters_processed = 0;

    if ( string == NULL )
    {
        return 0;
    }

    *value_out = 0;

    while ( ( characters_processed != str_length ) &&
            ( 0 == hexchar_to_nibble( *string, &nibble ) ) &&
            ( ( is_hex != 0 ) || ( nibble < 10 ) )
          )
    {
        if ( is_hex != 0 )
        {
            if ( ( ( *value_out > ( 0x7fffffff >> 4 ) ) && ( is_unsigned == 0 ) ) ||
                 ( *value_out > ( 0xffffffff >> 4 ) )
               )
            {
                break;
            }
            *value_out = ( *value_out << 4 ) + nibble;
        }
        else
        {
            if ( ( ( *value_out > ( 0x7fffffff / 10 ) ) && ( is_unsigned == 0 ) ) ||
                 ( *value_out > ( 0xffffffff / 10 ) )
               )
            {
                break;
            }
            *value_out = ( *value_out * 10 ) + nibble;
        }
        string++;
        characters_processed++;
    }

    return characters_processed;
}

/*!
 ******************************************************************************
 * Convert a decimal or hexidecimal string to an integer.
 *
 * @param[in] str  The string containing the value.
 *
 * @return    The value represented by the string.
 */
uint32_t generic_string_to_unsigned( const char* str )
{
    uint32_t val = 0;
    uint8_t is_hex = 0;

    if ( strncmp( str, "0x", 2 ) == 0 )
    {
        is_hex = 1;
        str += 2;
    }

    string_to_unsigned( str, (uint8_t)strlen(str), &val, is_hex );

    return val;
}

/**
 * Converts a decimal/hexidecimal string (with optional sign) to a signed long int
 * Better than strtol or atol or atoi because the return value indicates if an error occurred
 *
 * @param string[in]     : The string buffer to be converted
 * @param str_length[in] : The maximum number of characters to process in the string buffer
 * @param value_out[out] : The unsigned in that will receive value of the the decimal string
 * @param is_hex[in]     : 0 = Decimal string, 1 = Hexidecimal string
 *
 * @return the number of characters successfully converted (including sign).  i.e. 0 = error
 *
 */
uint8_t string_to_signed( const char* string, uint8_t str_length, int32_t* value_out, uint8_t is_hex )
{
    uint8_t characters_processed = 0;
    uint8_t retval;
    char    first_char;

    if ( string == NULL )
    {
        return 0;
    }

    first_char = *string;

    if ( ( first_char == '-' ) || ( *string == '+' ) )
    {
        characters_processed++;
        string++;
        str_length--;
    }

    retval = string_to_generic( string, str_length, (uint32_t*)value_out, 0, is_hex );
    if ( retval == 0 )
    {
        return 0;
    }

    if ( first_char == '-' )
    {
        *value_out = -(*value_out);
    }
    return (uint8_t) ( characters_processed + retval );
}

/**
 * Converts a decimal/hexidecimal string to an unsigned long int
 * Better than strtol or atol or atoi because the return value indicates if an error occurred
 *
 * @param string[in]     : The string buffer to be converted
 * @param str_length[in] : The maximum number of characters to process in the string buffer
 * @param value_out[out] : The unsigned in that will receive value of the the decimal string
 * @param is_hex[in]     : 0 = Decimal string, 1 = Hexidecimal string
 *
 * @return the number of characters successfully converted.  i.e. 0 = error
 *
 */
uint8_t string_to_unsigned( const char* string, uint8_t str_length, uint32_t* value_out, uint8_t is_hex )
{
    return string_to_generic( string, str_length,  value_out, 1, is_hex );
}

/**
 * Converts a unsigned long int to a decimal string
 *
 * @param value[in]      : The unsigned long to be converted
 * @param output[out]    : The buffer which will receive the decimal string
 * @param min_length[in] : the minimum number of characters to output (zero padding will apply if required).
 * @param max_length[in] : the maximum number of characters to output (up to 10 ). There must be space for terminating NULL.
 *
 * @note: A terminating NULL is added. Wnsure that there is space in the buffer for this.
 *
 * @return the number of characters returned (excluding terminating null)
 *
 */
uint8_t unsigned_to_decimal_string( uint32_t value, char* output, uint8_t min_length, uint8_t max_length )
{
    uint8_t digits_left;
    char buffer[ 10 ] = "0000000000";
    max_length = (uint8_t) MIN( max_length, sizeof( buffer ) );
    digits_left = max_length;
    while ( ( value != 0 ) && ( digits_left != 0 ) )
    {
        --digits_left;
        buffer[ digits_left ] = (char) (( value % 10 ) + '0');
        value = value / 10;
    }

    digits_left = (uint8_t) MIN( ( max_length - min_length ), digits_left );
    memcpy( output, &buffer[ digits_left ], (size_t)( max_length - digits_left ) );

    /* Add terminating null */
    output[( max_length - digits_left )] = '\x00';

    return (uint8_t) ( max_length - digits_left );
}

/**
 * Converts a signed long int to a decimal string
 *
 * @param value[in]      : The signed long to be converted
 * @param output[out]    : The buffer which will receive the decimal string
 * @param min_length[in] : the minimum number of characters to output (zero padding will apply if required)
 * @param max_length[in] : the maximum number of characters to output (up to 10 ). There must be space for terminating NULL.
 *
 * @note: A terminating NULL is added. Wnsure that there is space in the buffer for this.
 *
 * @return the number of characters returned.
 *
 */
uint8_t signed_to_decimal_string( int32_t value, char* output, uint8_t min_length, uint8_t max_length )
{
    uint8_t retval = 0;
    if ( ( value < 0 ) && ( max_length > 0 ) )
    {
        *output = '-';
        output++;
        max_length--;
        value = -value;
        retval++;
    }
    retval = (uint8_t) ( retval + unsigned_to_decimal_string( (uint32_t)value, output, min_length, max_length ) );
    return retval;
}

/**
 * Converts a unsigned long int to a hexidecimal string
 *
 * @param value[in]      : The unsigned long to be converted
 * @param output[out]    : The buffer which will receive the hexidecimal string
 * @param min_length[in] : the minimum number of characters to output (zero padding will apply if required)
 * @param max_length[in] : the maximum number of characters to output (up to 8 ) There must be space for terminating NULL.
 *
 * @note: A terminating NULL is added. Wnsure that there is space in the buffer for this.
 * @note: No leading '0x' is added.
 *
 * @return the number of characters returned.
 *
 */
uint8_t unsigned_to_hex_string( uint32_t value, char* output, uint8_t min_length, uint8_t max_length )
{
    uint8_t digits_left;
    char buffer[ 8 ] = "00000000";
    max_length = (uint8_t) MIN( max_length, sizeof( buffer ) );
    digits_left = max_length;
    while ( ( value != 0 ) && ( digits_left != 0 ) )
    {
        --digits_left;
        buffer[ digits_left ] = nibble_to_hexchar( value & 0x0000000F );
        value = value >> 4;
    }

    digits_left = (uint8_t) MIN( ( max_length - min_length ), digits_left );
    memcpy( output, &buffer[ digits_left ], (size_t)( max_length - digits_left ) );

    /* Add terminating null */
    output[( max_length - digits_left )] = '\x00';

    return (uint8_t) ( max_length - digits_left );
}

int is_digit_str( const char* str )
{
    int res = 0;
    int i = 0;

    if ( str != NULL )
    {
        i = (int)strlen(str);
        res = 1;
        while ( i > 0 )
        {
            if ( !IS_DIGIT(*str) )
            {
                res = 0;
                break;
            }
            str++;
            i--;
        }
    }

    return res;
}

uint8_t match_string_with_wildcard_pattern( const char* string, uint32_t length, const char* pattern )
{
    uint32_t current_string_length = length;
    uint32_t temp_string_length    = 0;
    char*    current_string        = (char*)string;
    char*    current_pattern       = (char*)pattern;
    char*    temp_string           = NULL;
    char*    temp_pattern          = NULL;

    /* Iterate through string and pattern until '*' is found */
    while ( ( current_string_length != 0 ) && ( *current_pattern != '*' ) )
    {
        /* Current pattern is not equal current string and current pattern isn't a wildcard character */
        if ( ( *current_pattern != *current_string ) && ( *current_pattern != '?' ) )
        {
            return 0;
        }
        current_pattern++;
        current_string++;
        current_string_length--;
    }

    /* '*' is detected in pattern. Consume string until matching pattern is found */
    while ( current_string_length != 0 )
    {
        switch ( *current_pattern )
        {
            case '*':
                if ( *(++current_pattern) == '\0' )
                {
                    /* Last character in the pattern is '*'. Return successful */
                    return 1;
                }

                /* Store temp variables for starting another matching iteration when non-matching character is found. */
                temp_pattern       = current_pattern;
                temp_string_length = current_string_length - 1;
                temp_string        = current_string + 1;
                break;

            case '?':
                current_pattern++;
                current_string++;
                current_string_length--;
                break;

            default:
                if ( *current_pattern == *current_string )
                {
                    current_pattern++;
                    current_string++;
                    current_string_length--;
                }
                else
                {
                    current_pattern       = temp_pattern;
                    current_string        = temp_string++;
                    current_string_length = temp_string_length--;
                }
                break;
        }
    }

    while ( *current_pattern == '*' )
    {
        current_pattern++;
    }

    return ( *current_pattern == '\0' );
}

/*
 ******************************************************************************
 * Length limited version of strstr. Ported from bcmutils.c
 *
 * @param     arg  The string to be searched.
 * @param     arg  The length of the string to be searched.
 * @param     arg  The string to be found.
 * @param     arg  The length of the string to be found.
 *
* @return    pointer to the found string if search successful, otherwise NULL
 */
char* strnstr(const char *s, uint16_t s_len, const char *substr, uint16_t substr_len)
{
    for (; s_len >= substr_len; s++, s_len--)
    {
        if (strncmp(s, substr, substr_len) == 0)
        {
            return (char*)s;
        }
    }

    return NULL;
}
