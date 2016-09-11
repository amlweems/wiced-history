#
# Copyright 2013, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

NAME := ThreadX

VERSION := 5.5

$(NAME)_COMPONENTS := Wiced/RTOS/ThreadX/wwd
ifeq (,$(APP_WWD_ONLY)$(NS_WWD_ONLY)$(RTOS_WWD_ONLY))
$(NAME)_COMPONENTS += Wiced/RTOS/ThreadX/wiced
endif

# Define some macros to allow for some network-specific checks
GLOBAL_DEFINES += RTOS_$(NAME)=1
GLOBAL_DEFINES += $(NAME)_VERSION=$$(SLASH_QUOTE_START)v$(VERSION)$$(SLASH_QUOTE_END)
GLOBAL_INCLUDES := ver$(VERSION)

GLOBAL_DEFINES += TX_INCLUDE_USER_DEFINE_FILE

ifneq ($(filter $(HOST_ARCH), ARM_Cortex_M3 ARM_Cortex_M4),)
THREADX_ARCH:=Cortex_M3_M4
GLOBAL_INCLUDES += ver$(VERSION)/Cortex_M3_M4/GCC
endif

$(NAME)_HEADERS := \
           ver$(VERSION)/tx_api.h \
           ver$(VERSION)/tx_trace.h \
           ver$(VERSION)/tx_user.h

ifdef WICED_ENABLE_TRACEX
$(info TRACEX)
THREADX_LIBRARY_NAME :=ThreadX.TraceX.$(HOST_ARCH).$(BUILD_TYPE).a
else
THREADX_LIBRARY_NAME :=ThreadX.$(HOST_ARCH).$(BUILD_TYPE).a
endif

ifneq ($(wildcard $(CURDIR)$(THREADX_LIBRARY_NAME)),)
$(NAME)_PREBUILT_LIBRARY := $(THREADX_LIBRARY_NAME)
else
$(NAME)_SOURCES := \
           ver$(VERSION)/tx_block_allocate.c \
           ver$(VERSION)/tx_block_pool_cleanup.c \
           ver$(VERSION)/tx_block_pool_create.c \
           ver$(VERSION)/tx_block_pool_delete.c \
           ver$(VERSION)/tx_block_pool_info_get.c \
           ver$(VERSION)/tx_block_pool_initialize.c \
           ver$(VERSION)/tx_block_pool_performance_info_get.c \
           ver$(VERSION)/tx_block_pool_performance_system_info_get.c \
           ver$(VERSION)/tx_block_pool_prioritize.c \
           ver$(VERSION)/tx_block_release.c \
           ver$(VERSION)/tx_byte_allocate.c \
           ver$(VERSION)/tx_byte_pool_cleanup.c \
           ver$(VERSION)/tx_byte_pool_create.c \
           ver$(VERSION)/tx_byte_pool_delete.c \
           ver$(VERSION)/tx_byte_pool_info_get.c \
           ver$(VERSION)/tx_byte_pool_initialize.c \
           ver$(VERSION)/tx_byte_pool_performance_info_get.c \
           ver$(VERSION)/tx_byte_pool_performance_system_info_get.c \
           ver$(VERSION)/tx_byte_pool_prioritize.c \
           ver$(VERSION)/tx_byte_pool_search.c \
           ver$(VERSION)/tx_byte_release.c \
           ver$(VERSION)/tx_event_flags_cleanup.c \
           ver$(VERSION)/tx_event_flags_create.c \
           ver$(VERSION)/tx_event_flags_delete.c \
           ver$(VERSION)/tx_event_flags_get.c \
           ver$(VERSION)/tx_event_flags_info_get.c \
           ver$(VERSION)/tx_event_flags_initialize.c \
           ver$(VERSION)/tx_event_flags_performance_info_get.c \
           ver$(VERSION)/tx_event_flags_performance_system_info_get.c \
           ver$(VERSION)/tx_event_flags_set.c \
           ver$(VERSION)/tx_event_flags_set_notify.c \
           ver$(VERSION)/tx_initialize_high_level.c \
           ver$(VERSION)/tx_initialize_kernel_enter.c \
           ver$(VERSION)/tx_initialize_kernel_setup.c \
           ver$(VERSION)/tx_low_power.c \
           ver$(VERSION)/tx_mutex_cleanup.c \
           ver$(VERSION)/tx_mutex_create.c \
           ver$(VERSION)/tx_mutex_delete.c \
           ver$(VERSION)/tx_mutex_get.c \
           ver$(VERSION)/tx_mutex_info_get.c \
           ver$(VERSION)/tx_mutex_initialize.c \
           ver$(VERSION)/tx_mutex_performance_info_get.c \
           ver$(VERSION)/tx_mutex_performance_system_info_get.c \
           ver$(VERSION)/tx_mutex_prioritize.c \
           ver$(VERSION)/tx_mutex_priority_change.c \
           ver$(VERSION)/tx_mutex_put.c \
           ver$(VERSION)/tx_queue_cleanup.c \
           ver$(VERSION)/tx_queue_create.c \
           ver$(VERSION)/tx_queue_delete.c \
           ver$(VERSION)/tx_queue_flush.c \
           ver$(VERSION)/tx_queue_front_send.c \
           ver$(VERSION)/tx_queue_info_get.c \
           ver$(VERSION)/tx_queue_initialize.c \
           ver$(VERSION)/tx_queue_performance_info_get.c \
           ver$(VERSION)/tx_queue_performance_system_info_get.c \
           ver$(VERSION)/tx_queue_prioritize.c \
           ver$(VERSION)/tx_queue_receive.c \
           ver$(VERSION)/tx_queue_send.c \
           ver$(VERSION)/tx_queue_send_notify.c \
           ver$(VERSION)/tx_semaphore_ceiling_put.c \
           ver$(VERSION)/tx_semaphore_cleanup.c \
           ver$(VERSION)/tx_semaphore_create.c \
           ver$(VERSION)/tx_semaphore_delete.c \
           ver$(VERSION)/tx_semaphore_get.c \
           ver$(VERSION)/tx_semaphore_info_get.c \
           ver$(VERSION)/tx_semaphore_initialize.c \
           ver$(VERSION)/tx_semaphore_performance_info_get.c \
           ver$(VERSION)/tx_semaphore_performance_system_info_get.c \
           ver$(VERSION)/tx_semaphore_prioritize.c \
           ver$(VERSION)/tx_semaphore_put.c \
           ver$(VERSION)/tx_semaphore_put_notify.c \
           ver$(VERSION)/tx_thread_create.c \
           ver$(VERSION)/tx_thread_delete.c \
           ver$(VERSION)/tx_thread_entry_exit_notify.c \
           ver$(VERSION)/tx_thread_identify.c \
           ver$(VERSION)/tx_thread_info_get.c \
           ver$(VERSION)/tx_thread_initialize.c \
           ver$(VERSION)/tx_thread_performance_info_get.c \
           ver$(VERSION)/tx_thread_performance_system_info_get.c \
           ver$(VERSION)/tx_thread_preemption_change.c \
           ver$(VERSION)/tx_thread_priority_change.c \
           ver$(VERSION)/tx_thread_relinquish.c \
           ver$(VERSION)/tx_thread_reset.c \
           ver$(VERSION)/tx_thread_resume.c \
           ver$(VERSION)/tx_thread_shell_entry.c \
           ver$(VERSION)/tx_thread_sleep.c \
           ver$(VERSION)/tx_thread_stack_analyze.c \
           ver$(VERSION)/tx_thread_stack_error_handler.c \
           ver$(VERSION)/tx_thread_stack_error_notify.c \
           ver$(VERSION)/tx_thread_suspend.c \
           ver$(VERSION)/tx_thread_system_preempt_check.c \
           ver$(VERSION)/tx_thread_system_resume.c \
           ver$(VERSION)/tx_thread_system_suspend.c \
           ver$(VERSION)/tx_thread_terminate.c \
           ver$(VERSION)/tx_thread_time_slice.c \
           ver$(VERSION)/tx_thread_time_slice_change.c \
           ver$(VERSION)/tx_thread_timeout.c \
           ver$(VERSION)/tx_thread_wait_abort.c \
           ver$(VERSION)/tx_time_get.c \
           ver$(VERSION)/tx_time_set.c \
           ver$(VERSION)/tx_timer_activate.c \
           ver$(VERSION)/tx_timer_change.c \
           ver$(VERSION)/tx_timer_create.c \
           ver$(VERSION)/tx_timer_deactivate.c \
           ver$(VERSION)/tx_timer_delete.c \
           ver$(VERSION)/tx_timer_expiration_process.c \
           ver$(VERSION)/tx_timer_info_get.c \
           ver$(VERSION)/tx_timer_initialize.c \
           ver$(VERSION)/tx_timer_performance_info_get.c \
           ver$(VERSION)/tx_timer_performance_system_info_get.c \
           ver$(VERSION)/tx_timer_system_activate.c \
           ver$(VERSION)/tx_timer_system_deactivate.c \
           ver$(VERSION)/tx_timer_thread_entry.c \
           ver$(VERSION)/tx_trace_disable.c \
           ver$(VERSION)/tx_trace_enable.c \
           ver$(VERSION)/tx_trace_event_filter.c \
           ver$(VERSION)/tx_trace_event_unfilter.c \
           ver$(VERSION)/tx_trace_initialize.c \
           ver$(VERSION)/tx_trace_interrupt_control.c \
           ver$(VERSION)/tx_trace_isr_enter_insert.c \
           ver$(VERSION)/tx_trace_isr_exit_insert.c \
           ver$(VERSION)/tx_trace_object_register.c \
           ver$(VERSION)/tx_trace_object_unregister.c \
           ver$(VERSION)/tx_trace_user_event_insert.c \
           ver$(VERSION)/txe_block_allocate.c \
           ver$(VERSION)/txe_block_pool_create.c \
           ver$(VERSION)/txe_block_pool_delete.c \
           ver$(VERSION)/txe_block_pool_info_get.c \
           ver$(VERSION)/txe_block_pool_prioritize.c \
           ver$(VERSION)/txe_block_release.c \
           ver$(VERSION)/txe_byte_allocate.c \
           ver$(VERSION)/txe_byte_pool_create.c \
           ver$(VERSION)/txe_byte_pool_delete.c \
           ver$(VERSION)/txe_byte_pool_info_get.c \
           ver$(VERSION)/txe_byte_pool_prioritize.c \
           ver$(VERSION)/txe_byte_release.c \
           ver$(VERSION)/txe_event_flags_create.c \
           ver$(VERSION)/txe_event_flags_delete.c \
           ver$(VERSION)/txe_event_flags_get.c \
           ver$(VERSION)/txe_event_flags_info_get.c \
           ver$(VERSION)/txe_event_flags_set.c \
           ver$(VERSION)/txe_event_flags_set_notify.c \
           ver$(VERSION)/txe_mutex_create.c \
           ver$(VERSION)/txe_mutex_delete.c \
           ver$(VERSION)/txe_mutex_get.c \
           ver$(VERSION)/txe_mutex_info_get.c \
           ver$(VERSION)/txe_mutex_prioritize.c \
           ver$(VERSION)/txe_mutex_put.c \
           ver$(VERSION)/txe_queue_create.c \
           ver$(VERSION)/txe_queue_delete.c \
           ver$(VERSION)/txe_queue_flush.c \
           ver$(VERSION)/txe_queue_front_send.c \
           ver$(VERSION)/txe_queue_info_get.c \
           ver$(VERSION)/txe_queue_prioritize.c \
           ver$(VERSION)/txe_queue_receive.c \
           ver$(VERSION)/txe_queue_send.c \
           ver$(VERSION)/txe_queue_send_notify.c \
           ver$(VERSION)/txe_semaphore_ceiling_put.c \
           ver$(VERSION)/txe_semaphore_create.c \
           ver$(VERSION)/txe_semaphore_delete.c \
           ver$(VERSION)/txe_semaphore_get.c \
           ver$(VERSION)/txe_semaphore_info_get.c \
           ver$(VERSION)/txe_semaphore_prioritize.c \
           ver$(VERSION)/txe_semaphore_put.c \
           ver$(VERSION)/txe_semaphore_put_notify.c \
           ver$(VERSION)/txe_thread_create.c \
           ver$(VERSION)/txe_thread_delete.c \
           ver$(VERSION)/txe_thread_entry_exit_notify.c \
           ver$(VERSION)/txe_thread_info_get.c \
           ver$(VERSION)/txe_thread_preemption_change.c \
           ver$(VERSION)/txe_thread_priority_change.c \
           ver$(VERSION)/txe_thread_relinquish.c \
           ver$(VERSION)/txe_thread_reset.c \
           ver$(VERSION)/txe_thread_resume.c \
           ver$(VERSION)/txe_thread_suspend.c \
           ver$(VERSION)/txe_thread_terminate.c \
           ver$(VERSION)/txe_thread_time_slice_change.c \
           ver$(VERSION)/txe_thread_wait_abort.c \
           ver$(VERSION)/txe_timer_activate.c \
           ver$(VERSION)/txe_timer_change.c \
           ver$(VERSION)/txe_timer_create.c \
           ver$(VERSION)/txe_timer_deactivate.c \
           ver$(VERSION)/txe_timer_delete.c \
           ver$(VERSION)/txe_timer_info_get.c

# Cortex-M3 & Cortex-M4 specific files
ifeq ($(THREADX_ARCH),Cortex_M3_M4)

ifeq ($(TOOLCHAIN_NAME),IAR)
$(NAME)_SOURCES += \
           ver$(VERSION)/Cortex_M3_M4/IAR/tx_thread_stack_build.S \
           ver$(VERSION)/Cortex_M3_M4/IAR/tx_thread_schedule.S \
           ver$(VERSION)/Cortex_M3_M4/IAR/tx_thread_system_return.S \
           ver$(VERSION)/Cortex_M3_M4/IAR/tx_thread_context_save.S \
           ver$(VERSION)/Cortex_M3_M4/IAR/tx_thread_context_restore.S \
           ver$(VERSION)/Cortex_M3_M4/IAR/tx_thread_interrupt_control.S \
           ver$(VERSION)/Cortex_M3_M4/IAR/tx_timer_interrupt.S

$(NAME)_HEADERS := \
           ver$(VERSION)/Cortex_M3_M4/IAR/tx_port.h

else

$(NAME)_SOURCES += \
           ver$(VERSION)/Cortex_M3_M4/GCC/tx_thread_stack_build.S \
           ver$(VERSION)/Cortex_M3_M4/GCC/tx_thread_schedule.S \
           ver$(VERSION)/Cortex_M3_M4/GCC/tx_thread_system_return.S \
           ver$(VERSION)/Cortex_M3_M4/GCC/tx_thread_context_save.S \
           ver$(VERSION)/Cortex_M3_M4/GCC/tx_thread_context_restore.S \
           ver$(VERSION)/Cortex_M3_M4/GCC/tx_thread_interrupt_control.S \
           ver$(VERSION)/Cortex_M3_M4/GCC/tx_timer_interrupt.S

$(NAME)_HEADERS := \
           ver$(VERSION)/Cortex_M3_M4/GCC/tx_port.h


endif

endif # ifeq ($(THREADX_ARCH),Cortex_M3_M4)

endif # ifneq ($(wildcard $(CURDIR)ThreadX.$(HOST_ARCH).$(BUILD_TYPE).a),)
