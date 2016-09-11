/*
 * Copyright 2013, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <wiced_utilities.h>
#include <wwd_rtos.h>
#include <wwd_assert.h>

#ifdef MALLOC_DEBUG

//#define MALLOC_FREE_STRICT_ORDER

#define MALLOC_PADDING_SIZE (8)
#define MALLOC_PADDING_VALUE (0xA5)
#define NUM_BACKTRACE_ITEMS (4)
#define MALLOC_MAGIC_NUM  (0xD94BC579)

typedef struct malloc_elem_struct
{
    uint32_t magic_num;
    uint32_t size;
    const char* name;
    struct malloc_elem_struct* next;
    struct malloc_elem_struct* prev;
    void* caller;
    malloc_thread_handle thread;
    uint32_t no_leak_flag;
} malloc_elem_t;

extern void *      __real_malloc  ( size_t size );
extern void *      __wrap_malloc  ( size_t size );
extern void *      __wrap_realloc ( void *ptr, size_t size );
extern void *      __real_realloc ( void *ptr, size_t size );
extern void *      __wrap_calloc  ( size_t nelem, size_t elsize );
extern void *      __real_calloc  ( size_t nelem, size_t elsize );
extern void        __real_free    ( void *m );
extern void        __wrap_free    ( void* m );
extern void *      __builtin_return_address ( unsigned int level );
static void *      malloc_generic( const char* name, size_t size, void* caller );

static void check_mallocs( void );
static void malloc_error( const char* error_message, malloc_elem_t* malloc_block_details );


static const char* curr_malloc_name = NULL;
static uint32_t max_allocated = 0;
static uint32_t curr_allocated = 0;
static malloc_elem_t* malloc_block_list = NULL;

void * __wrap_malloc( size_t size )
{
    return malloc_generic( curr_malloc_name, size, __builtin_return_address( 0 ) );
}


void * malloc_named( const char* name, size_t size )
{
    return malloc_generic( name, size, __builtin_return_address( 0 ) );
}

void *__wrap_calloc(size_t nelem, size_t elsize)
{
    void* ret = malloc_generic( curr_malloc_name, elsize*nelem, __builtin_return_address( 0 ) );
    memset(ret, 0, elsize*nelem);
    return ret;
}

void *calloc_named(const char* name, size_t nelem, size_t elsize)
{
    void* ret = malloc_generic( name, elsize*nelem, __builtin_return_address( 0 ) );
    memset(ret, 0, elsize*nelem);
    return ret;
}

void *__wrap_realloc(void *ptr, size_t size)
{
    malloc_elem_t* elem;
    malloc_elem_t* tmp;

    if ( size == 0 )
    {
        __wrap_free( ptr );
        return NULL;
    }

    check_mallocs( );

    elem = (malloc_elem_t*)  ((char*)ptr-MALLOC_PADDING_SIZE-sizeof(malloc_elem_t));

    tmp = malloc_block_list;
    while ( ( tmp != NULL ) && ( tmp != elem ) )
    {
        tmp = tmp->next;
    }
    if ( tmp == NULL )
    {
        malloc_error( "realloc arg not in malloc list", elem );
    }

    malloc_elem_t* ret = (malloc_elem_t*) __real_realloc( elem, size + 2*MALLOC_PADDING_SIZE + sizeof( malloc_elem_t ) );

    if ( ret == NULL )
    {
        return NULL;
    }

    if ( ret != elem )
    {
        if ( malloc_block_list == elem )
        {
            malloc_block_list = ret;
        }

        if ( ret->next != NULL )
        {
            ret->next->prev = ret;
        }
        if ( ret->prev != NULL )
        {
            ret->prev->next = ret;
        }
    }
    ret->size = size;

    return ((char*)ret) + MALLOC_PADDING_SIZE + sizeof( malloc_elem_t );
}

static void * malloc_generic( const char* name, size_t size, void* caller )
{
    malloc_elem_t* item_ptr;

    check_mallocs( );

    item_ptr = __real_malloc ( size + 2*MALLOC_PADDING_SIZE + sizeof( malloc_elem_t ) );
    if ( item_ptr == NULL )
    {
        return NULL;
    }
    item_ptr->magic_num = MALLOC_MAGIC_NUM;
    item_ptr->name = name;
    item_ptr->size = size;
    item_ptr->caller = caller;


    if ( malloc_block_list == NULL )
    {
        malloc_block_list = item_ptr;
        item_ptr->next = NULL;
        item_ptr->prev = NULL;
    }
    else
    {
        malloc_block_list->prev = item_ptr;
        item_ptr->next = malloc_block_list;
        item_ptr->prev = NULL;
        malloc_block_list = item_ptr;
    }

    char* padding_ptr = (char*)item_ptr + sizeof( malloc_elem_t );
    memset( padding_ptr, MALLOC_PADDING_VALUE, 2*MALLOC_PADDING_SIZE + size  );

    curr_allocated += size;
    if ( curr_allocated > max_allocated )
    {
        max_allocated = curr_allocated;
    }

    item_ptr->no_leak_flag = 0;
    item_ptr->thread = malloc_get_current_thread( );

    return padding_ptr + MALLOC_PADDING_SIZE;
}

void malloc_set_name( const char* name )
{
    curr_malloc_name = name;
}

const char* malloc_get_name( void )
{
    return curr_malloc_name;
}

void __wrap_free (void* m)
{
    malloc_elem_t* elem;

    check_mallocs( );

    elem = (malloc_elem_t*)  ((char*)m-MALLOC_PADDING_SIZE-sizeof(malloc_elem_t));

#ifdef MALLOC_FREE_STRICT_ORDER
    if ( elem != malloc_block_list )
    {
        malloc_error( "Free not in reverse order of malloc", elem );
    }
#else
    {
        malloc_elem_t* tmp = malloc_block_list;
        while ( ( tmp != NULL ) && ( tmp != elem ) )
        {
            tmp = tmp->next;
        }
        if ( tmp == NULL )
        {
            malloc_error( "Free arg not in malloc list", elem );
        }
    }
#endif


    if ( elem->no_leak_flag != 0 )
    {
        malloc_error( "Freeing block marked as base", elem );
    }

    if ( elem->thread != malloc_get_current_thread( ) )
    {
        malloc_error( "Freeing block from wrong thread", elem );
    }

    if ( elem->next != NULL )
    {
        elem->next->prev = elem->prev;
    }
    if ( elem->prev != NULL )
    {
        elem->prev->next = elem->next;
    }
    if ( elem == malloc_block_list )
    {
        malloc_block_list = elem->next;
    }

    curr_allocated -= elem->size;

    __real_free (elem);
}

void malloc_error( const char* error_message, malloc_elem_t* malloc_block_details )
{
    /* Dynamic memory error
     * Please see variables below for details
     */
    (void) error_message;
    (void) malloc_block_details;
    (void) malloc_block_list;
    wiced_assert("Dynamic memory error", 0 != 0 );
}

void check_mallocs( void )
{
    malloc_elem_t* tmp = malloc_block_list;
    uint32_t alloc_count = 0;
    while ( tmp != NULL )
    {
        unsigned char* padding1 = (unsigned char*)tmp + sizeof(malloc_elem_t);
        unsigned char* padding2 = padding1 + MALLOC_PADDING_SIZE + tmp->size;
        int i = 0;
        while ( ( i < MALLOC_PADDING_SIZE ) &&
                ( padding1[i] == MALLOC_PADDING_VALUE ) &&
                ( padding2[i] == MALLOC_PADDING_VALUE ) )
        {
            i++;
        }
        if ( i != MALLOC_PADDING_SIZE )
        {
            malloc_error( "Padding bytes overwritten", tmp );
        }
        if ( tmp->magic_num != MALLOC_MAGIC_NUM )
        {
            malloc_error( "Magic Number corrupted", tmp );
        }

        alloc_count += tmp->size;
        tmp = tmp->next;
    }

    if ( alloc_count != curr_allocated )
    {
        malloc_error( "Allocation count missmatch", NULL );
    }

}


void  malloc_thread_leak_set_base( void )
{
    malloc_elem_t* tmp;

    check_mallocs( );

    tmp = malloc_block_list;
    while ( tmp != NULL )
    {
        if ( tmp->thread == malloc_get_current_thread( ) )
        {
            tmp->no_leak_flag = 1;
        }
        tmp = tmp->next;
    }
}
void malloc_thread_leak_check( void* thread )
{
    malloc_elem_t* tmp;

    check_mallocs( );

    tmp = malloc_block_list;
    while ( tmp != NULL )
    {
        if ( ( tmp->thread == thread ) && ( tmp->no_leak_flag != 1 ) )
        {
            malloc_error( "Memory leak", tmp );
        }
        tmp = tmp->next;
    }
}

void malloc_transfer_to_curr_thread( void* block )
{
    malloc_elem_t* elem;
    malloc_elem_t* tmp;

    check_mallocs( );

    elem = (malloc_elem_t*)  ((char*)block-MALLOC_PADDING_SIZE-sizeof(malloc_elem_t));

    tmp = malloc_block_list;
    while ( ( tmp != NULL ) && ( tmp != elem ) )
    {
        tmp = tmp->next;
    }
    if ( tmp == NULL )
    {
        malloc_error( "Transfer arg not in malloc list", elem );
    }

    elem->thread = malloc_get_current_thread( );
}


#endif /* ifdef MALLOC_DEBUG */
