/******************************************************************************
*
* Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
*
*****************************************************************************/

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
 *               Function Declarations
 ******************************************************/
void wiced_bt_protocol_print_service(unsigned int hdr, char* p_buf, unsigned short len);

/******************************************************
 *               Variables Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

void wiced_bt_protocol_print(unsigned int hdr, char* p_buf, unsigned short len)
{
#if defined(ENABLE_BT_PROTOCOL_TRACES)
    wiced_bt_protocol_print_service(hdr, p_buf, len);
#endif
}

