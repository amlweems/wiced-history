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
 */
//#include "bt_target.h"
#include "gki.h"
#include "upio.h"
#include "wiced_nfc_utils.h"
#include "userial.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* NFC_TASK declarations */
#define NFC_TASK_STR            ((INT8 *) "NFC_TASK")
#define NFC_TASK_STACK_SIZE     0x800
#define NFC_TASK_THREAD_PRI     6

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

BT_API extern UINT32 nfc_task (UINT32 param);

/******************************************************
 *               Variables Definitions
 ******************************************************/

static uint32_t nfc_task_stack[(NFC_TASK_STACK_SIZE+3)/4];

static tHAL_NFC_ENTRY hal_nfc_entry_tbl;

/******************************************************
 *               Function Definitions
 ******************************************************/

/* Init hardware used by NFC */
void wiced_nfc_InitHW(void)
{
    /* Initialize GPIOs */
    UPIO_Init( NULL );
    GKI_delay( 200 );

    USERIAL_Init( NULL );
}

/* Create NFC task */
void wiced_nfc_CreateTasks (void)
{
    GKI_create_task( (TASKPTR) nfc_task, NFC_TASK, NFC_TASK_STR, (UINT16 *) ( (UINT8 *) nfc_task_stack + NFC_TASK_STACK_SIZE ), sizeof( nfc_task_stack ), NFC_TASK_THREAD_PRI );
}

/* NFC  stack booting */
int nfc_fwk_boot_entry( void )
{
    /* Initialize hardware */
    wiced_nfc_InitHW( );

    /* Initialize OS */
    GKI_init( );

    /* Enable interrupts */
    GKI_enable( );

    /* Create tasks */
    wiced_nfc_CreateTasks( );

    HAL_NfcInitialize( );

    /* Start tasks */
    GKI_run( 0 );

    return 0;
}

/* HAL functions */
tHAL_NFC_ENTRY* nfc_fwk_get_hal_functions( void )
{
     /* Lookup table of HAL entry points (populated using GetProcAddress or platform equivalent) */
    hal_nfc_entry_tbl.initialize       = HAL_NfcInitialize;

    hal_nfc_entry_tbl.terminate        = HAL_NfcTerminate;
    hal_nfc_entry_tbl.open             = HAL_NfcOpen;
    hal_nfc_entry_tbl.close            = HAL_NfcClose;
    hal_nfc_entry_tbl.core_initialized = HAL_NfcCoreInitialized;
    hal_nfc_entry_tbl.write            = HAL_NfcWrite;
    hal_nfc_entry_tbl.prediscover      = HAL_NfcPreDiscover;
    hal_nfc_entry_tbl.control_granted  = HAL_NfcControlGranted;
    hal_nfc_entry_tbl.power_cycle      = HAL_NfcPowerCycle;
    hal_nfc_entry_tbl.get_max_ee       = HAL_NfcGetMaxNfcee;

//#if (NFC_BRCM_NOT_OPEN_INCLUDED == TRUE)
    hal_nfc_entry_tbl.write_buf               = HAL_NfcWriteBuf;
    hal_nfc_entry_tbl.print_buffer_usage      = HAL_NfcPrintBufferUsage;
    hal_nfc_entry_tbl.enter_ce_low_power_mode = HAL_NfcEnterCELowPowerMode;
    hal_nfc_entry_tbl.enter_full_power_mode   = HAL_NfcEnterFullPowerMode;
    hal_nfc_entry_tbl.update_snooze_mode      = HAL_NfcUpdateSnoozeMode;
    hal_nfc_entry_tbl.update_baud_rate        = HAL_NfcUpdateUartBaudRate;
//#endif

    return &hal_nfc_entry_tbl;
}

