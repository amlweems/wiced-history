/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */


#include <fx_api.h>

TX_THREAD  tx_thread_current;

TX_THREAD      *_tx_thread_current_ptr = &tx_thread_current;
TX_THREAD       _tx_timer_thread;
volatile ULONG  _tx_thread_system_state = 0;
