/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef INCLUDED_WWD_LOGGING_H_
#define INCLUDED_WWD_LOGGING_H_

/*
#define WICED_LOGGING_UART_ENABLE
*/
/*
#define WICED_LOGGING_BUFFER_ENABLE
*/

#if defined( WICED_LOGGING_UART_ENABLE )

#include <stdio.h>

#define WICED_LOG( x ) {printf x; }

#elif defined( WICED_LOGGING_BUFFER_ENABLE )


extern int wiced_logging_printf(const char *format, ...);

#define WICED_LOG( x ) {wiced_logging_printf x; }


#else /* if defined( WICED_LOGGING_BUFFER_ENABLE ) */

#define WICED_LOG(x)

#endif /* if defined( WICED_LOGGING_BUFFER_ENABLE ) */



#endif /* ifndef INCLUDED_WWD_LOGGING_H_ */
