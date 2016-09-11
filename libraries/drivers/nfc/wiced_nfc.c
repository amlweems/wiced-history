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
#include "wiced_rtos.h"
#include "wiced_nfc_api.h"
#include "wiced_utilities.h"
//#include "wiced_nfc_utils.h"
#include "platform_resource.h"
#include "platform_nfc.h"
#include "resources.h"
#include "nfa_api.h"
#include "gki_common.h"
#include "wwd_assert.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_NFC_SUPPORTED_TECHNOLOGIES  ( 0xFF ) /* all */
#define WICED_NFC_POLL_BIT_RATE           ( 1 )
#define WICED_NFC_NDEP_BIT_RATE           ( 1 )

#define WICED_NFC_NUM_MSG                 ( 1 )

/* Task States: (For OSRdyTbl) */
#define TASK_DEAD       0   /* b0000 */
#define TASK_READY      1   /* b0001 */
#define TASK_WAIT       2   /* b0010 */
#define TASK_DELAY      4   /* b0100 */
#define TASK_SUSPEND    8   /* b1000 */

#define NFC_INITIALIZATION_TIMEOUT     (8000)

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/** WICED NFC queue data structure
 * The structure should be always 4 byte aligned.
 */
typedef struct
{
    uint32_t status;
} wiced_nfc_queue_element_t;

typedef struct
{
    wiced_mutex_t GKI_mutex;
    wiced_thread_t thread_id[GKI_MAX_TASKS];
    wiced_mutex_t thread_evt_mutex[GKI_MAX_TASKS];
    wiced_queue_t thread_evt_queue[GKI_MAX_TASKS];
} tGKI_OS;


/******************************************************
 *               Function Declarations
 ******************************************************/

extern void nfc_fwk_dm_bkgd_conn_event_handler( uint8_t event, tNFA_CONN_EVT_DATA *p_data );
extern void nfc_fwk_dm_event_handler( uint8_t event, tNFA_DM_CBACK_DATA *p_data );
extern int nfc_fwk_boot_entry( void );
extern tHAL_NFC_ENTRY* nfc_fwk_get_hal_functions( void );
extern wiced_result_t nfc_fwk_ndef_build( wiced_nfc_tag_msg_t* p_ndef_msg, uint8_t** pp_ndef_buf, uint32_t* p_ndef_size );
extern void nfc_fwk_mem_co_free( void *p_buf );
extern tNFA_STATUS NFA_RwWriteNDef (UINT8 *p_data, UINT32 len);
extern unsigned char NFA_RegisterNDefTypeHandler( unsigned char handle_whole_message, unsigned char tnf, unsigned char *p_type_name, unsigned char type_name_len, tNFA_NDEF_CBACK *p_ndef_cback );
extern void GKI_shutdown( void );

static void gki_update_timer_cback( );

/******************************************************
 *               Variables Definitions
 ******************************************************/

wiced_nfc_workspace_t* wiced_nfc_workspace_ptr;

tGKI_OS gki_cb_os;
tGKI_COM_CB gki_cb_com;
wiced_timer_t update_tick_timer;

/******************************************************
 *               Function Definitions
 ******************************************************/

/* Enables the NFC protocol layer */
wiced_result_t wiced_nfc_init ( wiced_nfc_workspace_t* workspace )
{
    wiced_nfc_queue_element_t message;

    wiced_assert("wiced_nfc_init - invalid param", workspace != NULL);

    /* Initialize NFC fwk workspace */
    memset( workspace, 0, sizeof( wiced_nfc_workspace_t ) );

    wiced_nfc_workspace_ptr = workspace;
    nfc_fwk_boot_entry( );

    NFA_Init( nfc_fwk_get_hal_functions( ) );

    wiced_rtos_init_queue( &wiced_nfc_workspace_ptr->queue, NULL, sizeof(&wiced_nfc_workspace_ptr->queue), WICED_NFC_NUM_MSG );

    if ( NFA_Enable( nfc_fwk_dm_event_handler, nfc_fwk_dm_bkgd_conn_event_handler ) != NFA_STATUS_OK )
    {
        return WICED_ERROR;
    }

    /* Wait for the first event which should occur once NFC device has been initialized */
    if ( wiced_rtos_pop_from_queue( &wiced_nfc_workspace_ptr->queue, &message, NFC_INITIALIZATION_TIMEOUT ) != WICED_SUCCESS )
    {
        return WICED_ERROR;
    }

    return WICED_SUCCESS;
}

/* Disable NFC protocol layer */
wiced_result_t wiced_nfc_deinit( void )
{
    wiced_assert("wiced_nfc not initialized", wiced_nfc_workspace_ptr != NULL);

    NFA_Disable ( TRUE );
    GKI_shutdown( );

    wiced_rtos_deinit_queue( &wiced_nfc_workspace_ptr->queue );

    return WICED_SUCCESS;
}

/* NFC read tag */
wiced_result_t wiced_nfc_read_tag( uint8_t* read_buffer, uint32_t* length, uint32_t timeout )
{
    wiced_nfc_queue_element_t message;
    wiced_result_t            result = WICED_TIMEOUT;

    wiced_assert("wiced_nfc not initialized", wiced_nfc_workspace_ptr != NULL);
    if ( read_buffer == NULL || length == NULL )
    {
        return WICED_BADARG;
    }

    /* Flush the queue if it has any data due to the timeout and data receive simultaneously for previous read,-Do dummy read*/
    result = wiced_rtos_pop_from_queue( &wiced_nfc_workspace_ptr->queue, &message, WICED_NO_WAIT );

    /* Its cleared back in the NDEF data event */
    wiced_nfc_workspace_ptr->read_buffer_ptr = read_buffer;

    /* Length should be always initialized second because only length check is done in the data callback*/
    wiced_nfc_workspace_ptr->read_length_ptr = length;

    result = wiced_rtos_pop_from_queue( &wiced_nfc_workspace_ptr->queue, &message, timeout );

    if ( result != WICED_TIMEOUT )
    {
        /* If the given buffer length is less than the received data then we return the status of Bad argument here */
        result = message.status;
    }
    else
    {
        wiced_nfc_workspace_ptr->read_length_ptr = NULL;
    }
    return result;
}


/* Write NFC tag */
wiced_result_t wiced_nfc_write_tag( wiced_nfc_tag_msg_t* write_data, uint32_t timeout )
{
    wiced_result_t            result;
    wiced_nfc_queue_element_t message;

    wiced_assert("wiced_nfc not initialized", wiced_nfc_workspace_ptr != NULL);
    if ( write_data == NULL )
    {
        return WICED_BADARG;
    }

    /* Flush the queue if it has any data due to the timeout and data receive simulatiously for previous read,-Do dummy read*/
    result = wiced_rtos_pop_from_queue( &wiced_nfc_workspace_ptr->queue, &message, WICED_NO_WAIT );

    result = nfc_fwk_ndef_build( write_data, &wiced_nfc_workspace_ptr->write_buffer_ptr, &wiced_nfc_workspace_ptr->write_length );

    if ( result == WICED_SUCCESS )
    {
        result = wiced_rtos_pop_from_queue( &wiced_nfc_workspace_ptr->queue, &message, timeout );
    }
    nfc_fwk_mem_co_free( wiced_nfc_workspace_ptr->write_buffer_ptr );
    wiced_nfc_workspace_ptr->write_buffer_ptr = NULL;

    return result;
}

/* NFC NDEF handler */
void nfc_default_ndef_handler( tNFA_NDEF_EVT event, tNFA_NDEF_EVT_DATA* p_data )
{
    wiced_nfc_queue_element_t message;

    switch ( event )
    {
        case NFA_NDEF_REGISTER_EVT:
            /* NDEF record type registered. (In response to NFA_RegisterNDefHandler) */
            break;

        case NFA_NDEF_DATA_EVT:
            /* Received an NDEF record with the registered type. See [tNFA_NDEF_DATA] */
            if ( p_data != NULL )
            {
                /* If read is waiting */
                if ( ( wiced_nfc_workspace_ptr->read_length_ptr != NULL ) && ( p_data->ndef_data.status == NFA_STATUS_OK ) )
                {
                    if ( *wiced_nfc_workspace_ptr->read_length_ptr < p_data->ndef_data.len )
                    {
                        /* Insufficient memory */
                        message.status = WICED_BADVALUE;
                    }
                    else
                    {
                        memcpy( wiced_nfc_workspace_ptr->read_buffer_ptr, p_data->ndef_data.p_data, p_data->ndef_data.len );
                        *wiced_nfc_workspace_ptr->read_length_ptr = p_data->ndef_data.len;
                        message.status = WICED_SUCCESS;
                    }
                    wiced_nfc_workspace_ptr->read_length_ptr = NULL;
                    wiced_rtos_push_to_queue( &wiced_nfc_workspace_ptr->queue, &message, WICED_NO_WAIT );
                }
            }
            break;

        default:
            break;
    }
}

void nfc_fwk_dm_event_handler( uint8_t event, tNFA_DM_CBACK_DATA* p_data )
{
    switch ( event )
    {
        case NFA_DM_ENABLE_EVT:
            if ( p_data->status == NFA_STATUS_OK )
            {
                wiced_nfc_queue_element_t message;
                message.status = WICED_SUCCESS;
                wiced_rtos_push_to_queue( &wiced_nfc_workspace_ptr->queue, &message, WICED_NO_WAIT );

                uint8_t data;
                /* NFA enabled */
                /* Register for default NDEF handler */
                NFA_RegisterNDefTypeHandler( TRUE, NFA_TNF_DEFAULT, NULL, 0, nfc_default_ndef_handler );

                /* Initialize nfapp sub-systems */
                NFA_SetP2pListenTech( WICED_NFC_SUPPORTED_TECHNOLOGIES );

                data = WICED_NFC_POLL_BIT_RATE;
                NFA_SetConfig( NCI_PARAM_ID_PF_BIT_RATE, sizeof( data ), &data );

                data = WICED_NFC_NDEP_BIT_RATE;
                NFA_SetConfig( NCI_PARAM_ID_BITR_NFC_DEP, sizeof( data ), &data );

                NFA_EnablePolling( WICED_NFC_SUPPORTED_TECHNOLOGIES );
                NFA_StartRfDiscovery( );
            }
            break;

        case NFA_DM_DISABLE_EVT:
            break;

        case NFA_DM_GET_CONFIG_EVT:
            break;
        case NFA_DM_SET_CONFIG_EVT:
            break;

        default:
            break;
    }
}

/* nfc connection callback event handler */
void nfc_fwk_dm_bkgd_conn_event_handler( uint8_t event, tNFA_CONN_EVT_DATA* p_data )
{
    wiced_nfc_queue_element_t message;

    switch ( event )
    {
        case NFA_RF_DISCOVERY_STARTED_EVT:
            break;
        case NFA_RF_DISCOVERY_STOPPED_EVT:
            break;
        case NFA_POLL_ENABLED_EVT:
            break;
        case NFA_POLL_DISABLED_EVT:
            break;
        case NFA_NDEF_DETECT_EVT:
            if ( wiced_nfc_workspace_ptr->write_buffer_ptr != NULL )
            {
                NFA_RwWriteNDef( wiced_nfc_workspace_ptr->write_buffer_ptr, wiced_nfc_workspace_ptr->write_length );
            }
            break;
        case NFA_DISC_RESULT_EVT:
            break;
        case NFA_SELECT_RESULT_EVT:
            break;
        case NFA_ACTIVATED_EVT:
            break;
        case NFA_DEACTIVATED_EVT:
            break;
        case NFA_READ_CPLT_EVT:
            break;
        case NFA_WRITE_CPLT_EVT:
            if ( wiced_nfc_workspace_ptr->write_buffer_ptr != NULL )
            {
                message.status = WICED_SUCCESS;
                wiced_rtos_push_to_queue( &wiced_nfc_workspace_ptr->queue, &message, WICED_NO_WAIT );
            }
            break;
        case NFA_FORMAT_CPLT_EVT:
            break;
    }
}


void nfc_host_free_patch_ram_resource( const void* buffer )
{
    resource_free_readonly_buffer ( &resources_nfc_DIR_patch_DIR_43341B0_DIR_patch_ram_ncd_bin, buffer );
}

void nfc_host_free_i2c_pre_patch_resource( const void* buffer )
{
    resource_free_readonly_buffer ( &resources_nfc_DIR_patch_DIR_43341B0_DIR_i2c_pre_patch_ncd_bin, buffer );
}

void nfc_host_get_patch_ram_resource( const UINT8** buffer, uint32_t* size )
{
    resource_get_readonly_buffer( &resources_nfc_DIR_patch_DIR_43341B0_DIR_patch_ram_ncd_bin, 0, 0, size, (const void**)buffer );
}

void nfc_host_get_i2c_pre_patch_resource( const UINT8** buffer, uint32_t* size  )
{
    resource_get_readonly_buffer( &resources_nfc_DIR_patch_DIR_43341B0_DIR_i2c_pre_patch_ncd_bin, 0, 0, size, (const void**)buffer );
}

void nfc_host_wake_pin_low( void )
{
    platform_gpio_output_low( wiced_nfc_control_pins[WICED_NFC_PIN_WAKE] );
}

void nfc_host_wake_pin_high( void )
{
    platform_gpio_output_high( wiced_nfc_control_pins[WICED_NFC_PIN_WAKE] );
}


void GKI_exception( UINT16 code, char *msg )
{
    GKI_TRACE_ERROR_0( "GKI_exception(): Task State Table");

    /*
     for(task_id = 0; task_id < GKI_MAX_TASKS; task_id++)
     {
     GKI_TRACE_ERROR_3( "TASK ID [%d] task name [%s] state [%d]",
     task_id,
     gki_cb_com.OSTName[task_id],
     gki_cb_com.OSRdyTbl[task_id]);
     }
     */

    GKI_TRACE_ERROR_2("GKI_exception %d %s", code, msg);
    GKI_TRACE_ERROR_0( "********************************************************************");
    GKI_TRACE_ERROR_2( "* GKI_exception(): %d %s", code, msg);
    GKI_TRACE_ERROR_0( "********************************************************************");

#if (GKI_DEBUG == TRUE)
    GKI_disable();

    if (gki_cb_com.ExceptionCnt < GKI_MAX_EXCEPTION)
    {
        EXCEPTION_T *pExp;

        pExp = &gki_cb_com.Exception[gki_cb_com.ExceptionCnt++];
        pExp->type = code;
        pExp->taskid = GKI_get_taskid();
        strncpy((char *)pExp->msg, msg, GKI_MAX_EXCEPTION_MSGLEN - 1);
    }

    GKI_enable();
#endif

    GKI_TRACE_ERROR_2("GKI_exception %d %s done", code, msg);

    return;
}

UINT8 GKI_get_taskid( void )
{
    int i;
    wiced_result_t ret;

    for ( i = 0; i < GKI_MAX_TASKS; i++ )
    {
        ret = wiced_rtos_is_current_thread( &gki_cb_os.thread_id[i] );
        if ( ret == WICED_SUCCESS )
        {
            return ( i );
        }
    }
    return ( -1 );
}

void GKI_enable( void )
{
    wiced_rtos_unlock_mutex( &gki_cb_os.GKI_mutex );
    return;
}

void GKI_disable( void )
{
    wiced_rtos_lock_mutex( &gki_cb_os.GKI_mutex );
    return;
}

void GKI_delay( UINT32 timeout )
{
    UINT8 rtask = GKI_get_taskid( );
    wiced_rtos_delay_milliseconds( timeout );

    // Check for a GKI task
    if ( rtask == -1 || rtask >= GKI_MAX_TASKS )
    {
    }
    else if ( rtask && gki_cb_com.OSRdyTbl[rtask] == TASK_DEAD )
    {
        GKI_exit_task( rtask );
    }

    return;
}


void GKI_init( void )
{
    memset( &gki_cb_com, 0, sizeof( gki_cb_com ) );
    memset( &gki_cb_os,  0, sizeof( gki_cb_os ) );

    gki_buffer_init( );
    gki_timers_init( );

    gki_cb_com.OSTicks = 0;
}

UINT8 GKI_create_task( TASKPTR task_entry, UINT8 task_id, INT8 *taskname, UINT16 *stack, UINT16 stacksize, UINT8 priority )
{
    wiced_result_t ret = 0;

    if ( task_id >= GKI_MAX_TASKS )
    {
        return ( GKI_FAILURE );
    }

    gki_cb_com.OSRdyTbl[task_id] = TASK_READY;
    gki_cb_com.OSTName[task_id] = taskname;
    gki_cb_com.OSWaitTmr[task_id] = 0;
    gki_cb_com.OSWaitEvt[task_id] = 0;

    ret = wiced_rtos_init_mutex( &gki_cb_os.thread_evt_mutex[task_id] );
    if ( ret != WICED_SUCCESS )
    {
        return GKI_FAILURE;
    }

    ret = wiced_rtos_init_queue( &gki_cb_os.thread_evt_queue[task_id], NULL, THREAD_EVT_QUEUE_MSG_SIZE, THREAD_EVT_QUEUE_NUM_MSG );
    if ( ret != WICED_SUCCESS )
    {
        return GKI_FAILURE;
    }

    ret = wiced_rtos_create_thread( &gki_cb_os.thread_id[task_id], priority, (const char *) taskname, (void *) task_entry, stacksize, NULL );
    if ( ret != WICED_SUCCESS )
    {
        return GKI_FAILURE;
    }

    return ( GKI_SUCCESS );
}


void GKI_exit_task( UINT8 task_id )
{
    GKI_disable( );
    gki_cb_com.OSRdyTbl[task_id] = TASK_DEAD;

    /* Destroy mutex and condition variable objects */
    wiced_rtos_deinit_mutex( &gki_cb_os.thread_evt_mutex[task_id] );
    wiced_rtos_deinit_queue( &gki_cb_os.thread_evt_queue[task_id] );
    wiced_rtos_delete_thread( &gki_cb_os.thread_id[task_id] );

    GKI_enable( );
    return;
}

UINT8 GKI_send_event( UINT8 task_id, UINT16 event )
{
    wiced_result_t ret;

    /* use efficient coding to avoid pipeline stalls */
    if ( task_id < GKI_MAX_TASKS )
    {
        /* protect OSWaitEvt[task_id] from manipulation in GKI_wait() */
        wiced_rtos_lock_mutex( &gki_cb_os.thread_evt_mutex[task_id] );

        /* Set the event bit */
        gki_cb_com.OSWaitEvt[task_id] |= event;

        ret = wiced_rtos_push_to_queue( &gki_cb_os.thread_evt_queue[task_id], (void*) &event, WICED_NO_WAIT );
        BT_TRACE_3( TRACE_LAYER_HCI, TRACE_TYPE_DEBUG, "GKI_send_event wiced_rtos_push_to_queue task_id=0x%x ret=0x%x queueData=0x%x", task_id, ret, event );
        REFERENCE_DEBUG_ONLY_VARIABLE( ret );

        wiced_rtos_unlock_mutex( &gki_cb_os.thread_evt_mutex[task_id] );

        return ( GKI_SUCCESS );
    }
    return ( GKI_FAILURE );
}


void GKI_run( void *p_task_id )
{
    wiced_rtos_init_timer( &update_tick_timer, 100, gki_update_timer_cback, NULL );
    wiced_rtos_start_timer( &update_tick_timer );
}


void gki_update_timer_cback( )
{
    wiced_rtos_start_timer( &update_tick_timer ); // Start time timer over again
    GKI_timer_update( 100 );
}

UINT16 GKI_wait( UINT16 flag, UINT32 timeout )
{
    UINT8 rtask;
    UINT8 check;
    UINT16 evt;
    UINT32 queueData = 0;
    wiced_result_t ret;

    rtask = GKI_get_taskid( );
    if ( rtask >= GKI_MAX_TASKS )
    {
        return 0;
    }

    gki_cb_com.OSWaitForEvt[rtask] = flag;

    wiced_rtos_lock_mutex( &gki_cb_os.thread_evt_mutex[rtask] );
    check = !( gki_cb_com.OSWaitEvt[rtask] & flag );
    wiced_rtos_unlock_mutex( &gki_cb_os.thread_evt_mutex[rtask] );

    if ( check )
    {
        timeout = ( timeout ? timeout : WICED_WAIT_FOREVER );
        if ( ( ret = wiced_rtos_pop_from_queue( &gki_cb_os.thread_evt_queue[rtask], (void*) &queueData, timeout ) ) != WICED_SUCCESS )
        {
            BT_TRACE_1( TRACE_LAYER_HCI, TRACE_TYPE_DEBUG, "GKI_wait wiced_rtos_pop_from_queue failed ret=0x%x", ret);
        }

        wiced_rtos_lock_mutex( &gki_cb_os.thread_evt_mutex[rtask] );
        if ( gki_cb_com.OSRdyTbl[rtask] == TASK_DEAD )
        {
            gki_cb_com.OSWaitEvt[rtask] = 0;
            BT_TRACE_1( TRACE_LAYER_HCI, TRACE_TYPE_DEBUG, "GKI TASK_DEAD received. exit thread %d...", rtask );
            GKI_exit_task( rtask );
            return ( EVENT_MASK(GKI_SHUTDOWN_EVT) );
        }
    }
    else
    {
        if ( ( ret = wiced_rtos_pop_from_queue( &gki_cb_os.thread_evt_queue[rtask], (void*) &queueData, 0 ) ) != WICED_SUCCESS )
        {
            BT_TRACE_1( TRACE_LAYER_HCI, TRACE_TYPE_DEBUG, "GKI_wait wiced_rtos_pop_from_queue failed ret=0x%x", ret);
        }
    }

    /* Clear the wait for event mask */
    gki_cb_com.OSWaitForEvt[rtask] = 0;

    /* Return only those bits which user wants... */
    evt = gki_cb_com.OSWaitEvt[rtask] & flag;

    /* Clear only those bits which user wants... */
    gki_cb_com.OSWaitEvt[rtask] &= ~flag;

    BT_TRACE_6(
            TRACE_LAYER_HCI,
            TRACE_TYPE_DEBUG,
            "GKI_wait taskid=0x%x check=0x%x OSWaitEvt=0x%x flag=0x%x queueData=0x%x evt=0x%x",
            rtask,
            check,
            gki_cb_com.OSWaitEvt[rtask],
            flag,
            queueData & 0x0000FFFF,
            evt);

    /* unlock thread_evt_mutex as wiced_cond_wait() does auto lock mutex when cond is met */
    wiced_rtos_unlock_mutex( &gki_cb_os.thread_evt_mutex[rtask] );

    return ( evt );
}

void *GKI_os_malloc( UINT32 size )
{
    return ( malloc( size ) );
}
