/*
 * WICED EAP host implementation
 *
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 */

#include <string.h>
#include "wiced_result.h"
#include "rtos.h"
#include "wwd_rtos_interface.h"
#include "besl_host_interface.h"
#include "besl_host_rtos_structures.h"
#include "network/wwd_buffer_interface.h"
#include "supplicant_structures.h"
#include "wiced_supplicant.h"

#include "wiced_tcpip_tls_api.h"
#include "wiced_utilities.h"
#include "wwd_eapol.h"
#include "wwd_events.h"
#include "wwd_wifi.h"
#include "wiced_time.h"
#include "wiced_tcpip_tls_api.h"

/******************************************************
 *            Includes
 ******************************************************/

/******************************************************
 *             Constants
 ******************************************************/

#define SUPPLICANT_THREAD_STACK_SIZE         ( 4*1024 )
#define TLS_AGENT_THREAD_STACK_SIZE          ( 4*1024 )
#define SUPPLICANT_BUFFER_SIZE               ( 3500 )
#define SUPPLICANT_WORKSPACE_ARRAY_SIZE      ( 1 )
#define WLC_EVENT_MSG_LINK                   ( 0x01 )
#define EAPOL_PACKET_TIMEOUT                 ( 5000 )  /* Milliseconds */
#define SUPPLICANT_HANDSHAKE_ATTEMPT_TIMEOUT ( 30000 ) /* Milliseconds */

/******************************************************
 *             Macros
 ******************************************************/

#define IF_TO_WORKSPACE( interface )   ( active_supplicant_workspaces[ WWD_STA_INTERFACE ] )     /* STA is the only currently supported interface. */

/******************************************************
 *             Local Structures
 ******************************************************/

typedef struct
{
    besl_host_workspace_t host_workspace;
    host_queue_type_t     outgoing_packet_queue;
    besl_event_message_t  outgoing_packet_buffer[10];
} supplicant_host_workspace_t;

struct supplicant_peap_workspace_s
{
    supplicant_peap_state_t state;
    host_thread_type_t      thread;
    void*                   thread_stack;
};
typedef struct supplicant_peap_workspace_s supplicant_peap_workspace_t;

/******************************************************
 *             Static Variables
 ******************************************************/

static const wwd_event_num_t         supplicant_events[] = { WLC_E_LINK, WLC_E_DEAUTH_IND, WLC_E_DISASSOC_IND, WLC_E_NONE };
static       supplicant_workspace_t* active_supplicant_workspaces[SUPPLICANT_WORKSPACE_ARRAY_SIZE] = { 0 };

/******************************************************
 *             Static Function Prototypes
 ******************************************************/

static void          supplicant_tls_agent_thread      ( wwd_thread_arg_t arg );
static void          wiced_supplicant_thread          ( wwd_thread_arg_t arg );
static void          supplicant_eapol_packet_handler  ( wiced_buffer_t buffer, wwd_interface_t interface );
static void*         supplicant_external_event_handler( const wwd_event_header_t* event_header, const uint8_t* event_data, /*@returned@*/ void* handler_user_data );
static void          wiced_supplicant_thread_main     ( wwd_thread_arg_t arg );
static besl_result_t supplicant_peap_init             ( supplicant_workspace_t* workspace, eap_type_t type );
static besl_result_t supplicant_peap_deinit           ( supplicant_workspace_t* workspace );
static void          supplicant_peap_thread           ( wwd_thread_arg_t arg );

/******************************************************
 *             Function definitions
 ******************************************************/

besl_result_t supplicant_tls_agent_start( supplicant_workspace_t* workspace )
{
    besl_host_workspace_t* host_workspace = (besl_host_workspace_t*) workspace->tls_agent.tls_agent_host_workspace;

    workspace->have_packet = 0;
    if ( host_rtos_create_thread_with_arg( &host_workspace->thread, supplicant_tls_agent_thread, "tls_agent", host_workspace->thread_stack, TLS_AGENT_THREAD_STACK_SIZE, RTOS_HIGHER_PRIORTIY_THAN(RTOS_DEFAULT_THREAD_PRIORITY), (wwd_thread_arg_t) workspace ) == WWD_SUCCESS )
    {
        return SUPPLICANT_SUCCESS;
    }
    return SUPPLICANT_NOT_STARTED;
}

static void supplicant_tls_agent_thread( wwd_thread_arg_t arg )
{
    supplicant_workspace_t* workspace = (supplicant_workspace_t*)arg;
    besl_host_workspace_t*  host      = (besl_host_workspace_t*)workspace->tls_agent.tls_agent_host_workspace;
    besl_event_message_t    message;

    if ( wiced_supplicant_start_tls( workspace, WICED_TLS_AS_CLIENT, TLS_VERIFICATION_REQUIRED ) != WICED_SUCCESS )
    {
        SUPPLICANT_DEBUG( ( "TLS handshake failed\n" ) );
    }
    else
    {
        supplicant_tls_agent_finish_connect( workspace );
    }

    /* Clean up left over messages in the event queue */
    while ( host_rtos_pop_from_queue( &host->event_queue, &message, 0 ) == WWD_SUCCESS )
    {
        if (message.event_type == BESL_EVENT_EAPOL_PACKET_RECEIVED || message.event_type == SUPPLICANT_EVENT_PACKET_TO_SEND )
        {
            besl_host_free_packet(message.data.packet);
        }
    }

    WICED_END_OF_CURRENT_THREAD( );
}

besl_result_t supplicant_tls_calculate_overhead( supplicant_workspace_t* workspace, uint16_t available_space, uint16_t* header, uint16_t* footer )
{
    return (besl_result_t)wiced_tls_calculate_overhead( &workspace->tls_context->context, available_space, header, footer );
}

besl_result_t supplicant_send_eap_tls_packet( supplicant_workspace_t* workspace, tls_agent_event_message_t* tls_agent_message, uint32_t timeout )
{
    besl_host_workspace_t* tls_agent_host_workspace = (besl_host_workspace_t*) workspace->tls_agent.tls_agent_host_workspace;
    wwd_result_t result = host_rtos_push_to_queue(  &tls_agent_host_workspace->event_queue, tls_agent_message, timeout);
    if ( result != WWD_SUCCESS )
    {
        SUPPLICANT_DEBUG( ( "Supplicant unable to push packet to tls agent queue\n" ) );
        besl_host_free_packet( tls_agent_message->data.packet );
    }
    return (besl_result_t) result;
}


besl_result_t supplicant_receive_eap_tls_packet( void* workspace_in, tls_packet_t** packet, uint32_t timeout )
{
    supplicant_workspace_t*     workspace = (supplicant_workspace_t*) workspace_in;
    tls_agent_event_message_t   message;
    besl_host_workspace_t* tls_agent_host_workspace = (besl_host_workspace_t*)workspace->tls_agent.tls_agent_host_workspace;
    wwd_result_t result = host_rtos_pop_from_queue(&tls_agent_host_workspace->event_queue, &message, timeout );
    if ( result != WWD_SUCCESS )
    {
        return (besl_result_t) result;
    }
    if ( message.event_type == TLS_AGENT_EVENT_EAPOL_PACKET )
    {
        *packet = message.data.packet;
    }
    else
    {
        host_rtos_delay_milliseconds( 10 );
    }
    return BESL_SUCCESS;
}


besl_result_t supplicant_tls_agent_init( tls_agent_workspace_t* workspace )
{
    besl_host_workspace_t* host_workspace;

    host_workspace = besl_host_malloc("tls_agent", sizeof(besl_host_workspace_t));
    if (host_workspace == NULL)
    {
        return SUPPLICANT_OUT_OF_HEAP_SPACE;
    }
    memset(host_workspace, 0, sizeof(besl_host_workspace_t));
    workspace->tls_agent_host_workspace = host_workspace;

#ifdef RTOS_USE_STATIC_THREAD_STACK
    host_workspace->thread_stack = besl_host_malloc("tls agent stack", TLS_AGENT_THREAD_STACK_SIZE);
    if (host_workspace->thread_stack == NULL)
    {
        besl_host_free(host_workspace);
        host_workspace = NULL;
        return SUPPLICANT_OUT_OF_HEAP_SPACE;
    }
    memset( host_workspace->thread_stack, 0, TLS_AGENT_THREAD_STACK_SIZE );
#else
    host_workspace->thread_stack = NULL;
#endif

    host_rtos_init_queue( &host_workspace->event_queue, host_workspace->event_buffer, sizeof( host_workspace->event_buffer ), sizeof(tls_agent_event_message_t) );

    return SUPPLICANT_SUCCESS;
}


besl_result_t supplicant_tls_agent_deinit( tls_agent_workspace_t* workspace )
{
    besl_host_workspace_t* host_workspace = workspace->tls_agent_host_workspace;

    if ( host_workspace != NULL )
    {
        host_rtos_join_thread(&host_workspace->thread);

        /* Delete the tls agent thread */
        host_rtos_delete_terminated_thread( &host_workspace->thread );

        if ( host_workspace->thread_stack != NULL )
        {
            besl_host_free( host_workspace->thread_stack );
            host_workspace->thread_stack = NULL;
        }

        host_rtos_deinit_queue( &host_workspace->event_queue );
        besl_host_free( host_workspace );
        workspace->tls_agent_host_workspace = NULL;
    }
    return BESL_SUCCESS;
}


besl_result_t besl_supplicant_start( supplicant_workspace_t* workspace )
{
    besl_result_t result = SUPPLICANT_ERROR_AT_THREAD_START;
    besl_host_workspace_t* host_workspace = &((supplicant_host_workspace_t*) workspace->supplicant_host_workspace )->host_workspace;

    result = (besl_result_t) host_rtos_create_thread_with_arg( &host_workspace->thread, wiced_supplicant_thread, "supplicant", host_workspace->thread_stack, SUPPLICANT_THREAD_STACK_SIZE, RTOS_HIGHER_PRIORTIY_THAN(RTOS_DEFAULT_THREAD_PRIORITY), (wwd_thread_arg_t) workspace );

    return result;
}


static void wiced_supplicant_thread( wwd_thread_arg_t arg )
{
    wiced_supplicant_thread_main( arg );

    WICED_END_OF_CURRENT_THREAD( );
}


besl_result_t besl_supplicant_init(supplicant_workspace_t* workspace, eap_type_t eap_type, wwd_interface_t interface )
{
    supplicant_host_workspace_t* supplicant_host_workspace = NULL;
    besl_result_t result;

    memset(workspace, 0, sizeof(*workspace));

    /* Register the EAPOL packet receive handler */
    result = (besl_result_t) wwd_eapol_register_receive_handler( supplicant_eapol_packet_handler );
    if ( result != BESL_SUCCESS )
    {
        return result;
    }

    /* Allocate memory for the supplicant host workspace */
    supplicant_host_workspace = besl_host_calloc("supplicant host", 1, sizeof(supplicant_host_workspace_t));
    if (supplicant_host_workspace == NULL)
    {
        result = SUPPLICANT_OUT_OF_HEAP_SPACE;
        goto exit;
    }
    workspace->supplicant_host_workspace = supplicant_host_workspace;

    /* Allocate memory for the EAP defragmentation buffer */
    workspace->buffer = besl_host_calloc("supplicant buffer", 1, SUPPLICANT_BUFFER_SIZE);
    if (workspace->buffer == NULL)
    {
        result = SUPPLICANT_OUT_OF_HEAP_SPACE;
        goto exit;
    }
    workspace->buffer_size = SUPPLICANT_BUFFER_SIZE;

    /* Allocate memory for the supplicant host thread stack */
#ifdef RTOS_USE_STATIC_THREAD_STACK
    supplicant_host_workspace->host_workspace.thread_stack = besl_host_calloc("supplicant stack", 1, SUPPLICANT_THREAD_STACK_SIZE);
    if (supplicant_host_workspace->host_workspace.thread_stack == NULL)
    {
        result = SUPPLICANT_ERROR_STACK_MALLOC_FAIL;
        goto exit;
    }
#else
    supplicant_host_workspace->host_workspace.thread_stack = NULL;
#endif

    if ( eap_type == EAP_TYPE_PEAP )
    {
        /* Initialize PEAP with MSCHAPV2 as default inner type */
        result = supplicant_peap_init( workspace, EAP_TYPE_MSCHAPV2 );
        if ( result != BESL_SUCCESS )
        {
            goto exit;
        }
    }

    supplicant_host_workspace->host_workspace.interface = interface;
    workspace->interface                                = interface;
    wwd_wifi_get_mac_address( ( wiced_mac_t* ) &workspace->supplicant_mac_address, ( wwd_interface_t ) workspace->interface );
    workspace->eap_type           = eap_type;
    workspace->supplicant_result  = SUPPLICANT_NOT_STARTED;
    workspace->current_main_stage = SUPPLICANT_INITIALISING;
    workspace->current_sub_stage  = SUPPLICANT_EAP_START;

    host_rtos_init_queue( &supplicant_host_workspace->host_workspace.event_queue, supplicant_host_workspace->host_workspace.event_buffer, sizeof( supplicant_host_workspace->host_workspace.event_buffer ), sizeof(besl_event_message_t) );
    host_rtos_init_queue( &supplicant_host_workspace->outgoing_packet_queue, supplicant_host_workspace->outgoing_packet_buffer, sizeof( supplicant_host_workspace->outgoing_packet_buffer ), sizeof(besl_event_message_t) );

    return BESL_SUCCESS;

exit:
    wwd_eapol_unregister_receive_handler( );
    if ( workspace->buffer != NULL )
    {
        besl_host_free(workspace->buffer);
        workspace->buffer = NULL;
    }
    if ( workspace->supplicant_host_workspace != NULL ) /* This points at supplicant_host_workspace */
    {
        if ( supplicant_host_workspace->host_workspace.thread_stack != NULL )
        {
            besl_host_free( supplicant_host_workspace->host_workspace.thread_stack );
            supplicant_host_workspace->host_workspace.thread_stack = NULL;
        }
        besl_host_free(workspace->supplicant_host_workspace);
        workspace->supplicant_host_workspace = NULL;
    }

    return result;
}


besl_result_t besl_supplicant_deinit( supplicant_workspace_t* workspace )
{
    supplicant_host_workspace_t* supplicant_host_workspace = (supplicant_host_workspace_t*)workspace->supplicant_host_workspace;

    besl_supplicant_wait_till_complete( workspace );

    if ( workspace->peap != NULL )
    {
        supplicant_peap_deinit( workspace );
    }

    if ( supplicant_host_workspace != NULL )
    {
        IF_TO_WORKSPACE( supplicant_host_workspace->host_workspace.interface ) = NULL;

        /* Delete the supplicant thread */
        if ( workspace->supplicant_result != SUPPLICANT_NOT_STARTED )
        {
            host_rtos_delete_terminated_thread( &supplicant_host_workspace->host_workspace.thread );
        }

        if ( supplicant_host_workspace->host_workspace.thread_stack != NULL )
        {
            besl_host_free( supplicant_host_workspace->host_workspace.thread_stack );
            supplicant_host_workspace->host_workspace.thread_stack = NULL;
        }

        host_rtos_deinit_queue( &supplicant_host_workspace->host_workspace.event_queue );
        host_rtos_deinit_queue( &supplicant_host_workspace->outgoing_packet_queue );
        besl_host_free( supplicant_host_workspace );
        workspace->supplicant_host_workspace = NULL;
    }

    if ( workspace->buffer != NULL )
    {
        besl_host_free( workspace->buffer );
        workspace->buffer = NULL;
    }

    return BESL_SUCCESS;
}


/* This function is called by the WWD thread so do not print from here unless the WWD thread stack size has been increased by 4K to allow for printing. */
static void supplicant_eapol_packet_handler( wiced_buffer_t buffer, wwd_interface_t interface )
{
    supplicant_workspace_t* workspace;

    if ( interface == WWD_STA_INTERFACE )
    {
        workspace = IF_TO_WORKSPACE( interface );
        if ( workspace == NULL )
        {
            host_buffer_release( buffer, WWD_NETWORK_RX );
        }
        else
        {
            besl_queue_message_packet( workspace, BESL_EVENT_EAPOL_PACKET_RECEIVED, buffer );
        }
    }
    else
    {
        SUPPLICANT_DEBUG( ( "EAPOL packet arriving on incorrect interface %u\n", (unsigned int) interface ) );
    }
}


besl_result_t besl_supplicant_wait_till_complete( supplicant_workspace_t* workspace )
{
    if ( workspace->supplicant_result != SUPPLICANT_NOT_STARTED )
    {
        besl_host_workspace_t* host_workspace = &((supplicant_host_workspace_t*) workspace->supplicant_host_workspace)->host_workspace;
        host_rtos_join_thread( &host_workspace->thread );
    }
    return BESL_SUCCESS;
}


besl_result_t besl_supplicant_management_set_event_handler( supplicant_workspace_t* workspace, wiced_bool_t enable )
{
    wwd_result_t        result;

    if ( enable == WICED_TRUE )
    {
        result = wwd_management_set_event_handler( supplicant_events, supplicant_external_event_handler, workspace, ( wwd_interface_t ) workspace->interface );
    }
    else
    {
        result = wwd_management_set_event_handler( supplicant_events, NULL, workspace, ( wwd_interface_t ) workspace->interface );
    }

    if ( result != WWD_SUCCESS )
    {
        WPRINT_APP_INFO(("Error setting supplicant event handler %u\n", (unsigned int)result));
    }
    return (besl_result_t) result;
}


/* This function is called by the WWD thread so do not print from here unless the WWD thread stack size has been increased by 4K to allow for printing. */
static void* supplicant_external_event_handler( const wwd_event_header_t* event_header, const uint8_t* event_data, /*@returned@*/ void* handler_user_data )
{
    supplicant_workspace_t* workspace = (supplicant_workspace_t*) handler_user_data;

    switch ( event_header->event_type )
    {
        case WLC_E_DEAUTH_IND:
        case WLC_E_DISASSOC_IND:
        {
            // TBD
            break;
        }

        case WLC_E_LINK:
            SUPPLICANT_DEBUG( ("Supplicant received link event\n" ) );
            if ( ( event_header->flags & WLC_EVENT_MSG_LINK ) != 0 )
            {
                if ( workspace->current_sub_stage == SUPPLICANT_EAP_START )
                {
                    besl_host_start_timer( workspace->supplicant_host_workspace, EAPOL_PACKET_TIMEOUT ); /* Start a timer to wait for EAP ID Request */
                }

            }
            /* TBD handle link down and link up events */
            break;

        default:
            break;
    }

    return handler_user_data;
}


static void wiced_supplicant_thread_main( wwd_thread_arg_t arg )
{
    wiced_time_t                 current_time;
    besl_result_t                result;
    besl_event_message_t         message;
    supplicant_workspace_t*      workspace = (supplicant_workspace_t*)arg;
    besl_host_workspace_t*       host      = &((supplicant_host_workspace_t*)workspace->supplicant_host_workspace)->host_workspace;

    workspace->supplicant_result = SUPPLICANT_IN_PROGRESS;

    /* Now that our queue is initialized we can flag the workspace as active */
    IF_TO_WORKSPACE( workspace->interface ) = workspace;

    workspace->start_time = host_rtos_get_time( );
    if ( besl_supplicant_management_set_event_handler( workspace, WICED_TRUE ) != SUPPLICANT_SUCCESS )
    {
        SUPPLICANT_DEBUG( ( "Supplicant unable to set management event handler.\n" ) );
    }

    while ( workspace->supplicant_result == SUPPLICANT_IN_PROGRESS )
    {
        uint32_t     time_to_wait;

        current_time = host_rtos_get_time( );

        if (( current_time - workspace->start_time ) >= SUPPLICANT_HANDSHAKE_ATTEMPT_TIMEOUT )
        {
            workspace->supplicant_result = SUPPLICANT_TIMEOUT;
            continue;
        }

        time_to_wait = ( SUPPLICANT_HANDSHAKE_ATTEMPT_TIMEOUT ) - ( current_time - workspace->start_time );
        if ( host->timer_timeout != 0 )
        {
            time_to_wait = MIN( time_to_wait, host->timer_timeout - (current_time - host->timer_reference));
        }

        if ( host_rtos_pop_from_queue( &host->event_queue, &message, time_to_wait ) != WWD_SUCCESS )
        {
            /* Create a timeout message */
            message.event_type = BESL_EVENT_TIMER_TIMEOUT;
            message.data.value = 0;
        }

        /* Process the message */
        result = supplicant_process_event( workspace, &message );
        if ( result != SUPPLICANT_SUCCESS )
        {
            SUPPLICANT_DEBUG( ( "Supplicant error %u\n", (unsigned int)result ) );
        }
    }

    wwd_eapol_unregister_receive_handler( );

    if ( workspace->peap != NULL )
    {
        supplicant_peap_deinit( workspace );
    }

    supplicant_tls_agent_deinit( &workspace->tls_agent );

    /* Print result (if enabled) */
    if ( workspace->supplicant_result == SUPPLICANT_COMPLETE )
    {
        SUPPLICANT_INFO(( "Supplicant completed successfully\n" ));
        workspace->supplicant_result = SUPPLICANT_SUCCESS;
    }
    else if ( workspace->supplicant_result == SUPPLICANT_ABORTED )
    {
        SUPPLICANT_INFO(( "Supplicant aborted\r\n" ));
        besl_host_leave( ( wwd_interface_t ) workspace->interface );
    }
    else
    {
        SUPPLICANT_INFO(( "Supplicant error %u\n", (unsigned int)workspace->supplicant_result ));
        besl_host_leave( ( wwd_interface_t ) workspace->interface );
    }

    if ( besl_supplicant_management_set_event_handler( workspace, WICED_FALSE ) != SUPPLICANT_SUCCESS )
    {
        SUPPLICANT_DEBUG( ( "Supplicant unable to remove management event handler.\n" ) );
    }

    /* Clean up left over messages in the event and outgoing packet queues */
    while ( host_rtos_pop_from_queue( &host->event_queue, &message, 0 ) == WWD_SUCCESS )
    {
        if (message.event_type == BESL_EVENT_EAPOL_PACKET_RECEIVED || message.event_type == SUPPLICANT_EVENT_PACKET_TO_SEND )
        {
            besl_host_free_packet(message.data.packet);
        }
    }
    while ( host_rtos_pop_from_queue( &((supplicant_host_workspace_t*)workspace->supplicant_host_workspace)->outgoing_packet_queue, &message, 0 ) == WWD_SUCCESS )
    {
        if (message.event_type == SUPPLICANT_EVENT_PACKET_TO_SEND)
        {
            besl_host_free_packet(message.data.packet);
        }
    }
}


besl_result_t supplicant_outgoing_pop( void* workspace, besl_event_message_t* message )
{
    supplicant_host_workspace_t* supplicant_host = (supplicant_host_workspace_t*)workspace;

    return (besl_result_t) host_rtos_pop_from_queue( &supplicant_host->outgoing_packet_queue, message, WICED_NEVER_TIMEOUT );
}


besl_result_t supplicant_outgoing_push( void* workspace, besl_event_message_t* message )
{
    supplicant_host_workspace_t* supplicant_host = (supplicant_host_workspace_t*)workspace;

    return (besl_result_t) host_rtos_push_to_queue( &supplicant_host->outgoing_packet_queue, message, WICED_NEVER_TIMEOUT );
}

/* Create a PEAP workspace to run EAP in it. */
static besl_result_t supplicant_peap_init( supplicant_workspace_t* workspace, eap_type_t type )
{
    supplicant_peap_workspace_t* peap_workspace;

    peap_workspace = besl_host_malloc( "supplicant host", sizeof(supplicant_peap_workspace_t) );
    if ( peap_workspace == NULL )
    {
        return SUPPLICANT_OUT_OF_HEAP_SPACE;
    }
    memset( peap_workspace, 0, sizeof(supplicant_peap_workspace_t) );
    workspace->peap = peap_workspace;

#ifdef RTOS_USE_STATIC_THREAD_STACK
    peap_workspace->thread_stack = besl_host_malloc( "supplicant stack", SUPPLICANT_THREAD_STACK_SIZE );
    if ( peap_workspace->thread_stack == NULL )
    {
        besl_host_free( workspace->peap );
        workspace->peap = NULL;
        return SUPPLICANT_ERROR_STACK_MALLOC_FAIL;
    }
    memset( peap_workspace->thread_stack, 0, SUPPLICANT_THREAD_STACK_SIZE );
#else
    peap_workspace->thread_stack = NULL;
#endif

    peap_workspace->state.eap_type   = type;
    peap_workspace->state.result     = SUPPLICANT_NOT_STARTED;
    peap_workspace->state.main_stage = SUPPLICANT_INITIALISING;
    peap_workspace->state.sub_stage  = SUPPLICANT_EAP_START;

    return BESL_SUCCESS;
}

/* Free a previously create PEAP work space. */
static besl_result_t supplicant_peap_deinit( supplicant_workspace_t* workspace )
{
    supplicant_peap_workspace_t* peap_workspace = workspace->peap;

    if ( peap_workspace->state.result != SUPPLICANT_NOT_STARTED )
    {
        workspace->peap->state.result = SUPPLICANT_ABORTED;
        host_rtos_join_thread( &peap_workspace->thread );
        host_rtos_delete_terminated_thread( &peap_workspace->thread );
    }

    if ( peap_workspace->thread_stack != NULL )
    {
        besl_host_free( peap_workspace->thread_stack );
        peap_workspace->thread_stack = NULL;
    }
    besl_host_free( workspace->peap );
    workspace->peap = NULL;

    return BESL_SUCCESS;
}

besl_result_t supplicant_peap_start( supplicant_workspace_t* workspace )
{
    supplicant_peap_workspace_t* peap_workspace = workspace->peap;
    peap_workspace->state.result = SUPPLICANT_IN_PROGRESS;
    return (besl_result_t) host_rtos_create_thread_with_arg( &peap_workspace->thread, supplicant_peap_thread, "peap", peap_workspace->thread_stack, SUPPLICANT_THREAD_STACK_SIZE, RTOS_HIGHER_PRIORTIY_THAN(RTOS_DEFAULT_THREAD_PRIORITY), (wwd_thread_arg_t) workspace );
}

besl_result_t supplicant_peap_packet_set_data( besl_packet_t* packet, int32_t size )
{
    return (besl_result_t) host_buffer_add_remove_at_front( (wiced_buffer_t *) packet, size );
}

static void supplicant_peap_thread( wwd_thread_arg_t arg )
{
    supplicant_workspace_t*      workspace      = (supplicant_workspace_t*) arg;
    supplicant_peap_workspace_t* peap_workspace = workspace->peap;

    /* Wait until TLS is done before starting to receive packets */
    while ( ( peap_workspace->state.result == SUPPLICANT_IN_PROGRESS ) &&( workspace->tls_context->context.state != SSL_HANDSHAKE_OVER ) )
    {
        host_rtos_delay_milliseconds( 100 );
    }

    while ( peap_workspace->state.result == SUPPLICANT_IN_PROGRESS )
    {
        besl_packet_t packet;
        if ( wiced_tls_receive_eap_packet( workspace, &packet, EAPOL_PACKET_TIMEOUT ) == WICED_SUCCESS )
        {
            supplicant_process_peap_event( workspace, packet );
            besl_host_free_packet( packet );
        }
    }

    WICED_END_OF_CURRENT_THREAD( );
}

