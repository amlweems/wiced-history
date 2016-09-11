/*
 * $ Copyright Broadcom Corporation $
 */
/************************************************************************************
 * arch/arm/src/arm/arm_vectorirq.S
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ************************************************************************************/

/************************************************************************************
 * Included Files
 ************************************************************************************/

#ifndef __ASSEMBLY__
#define __ASSEMBLY__
#endif

#include <nuttx/config.h>
#include <nuttx/irq.h>

#include "arm.h"

/************************************************************************************
 * Definitions
 ************************************************************************************/

/************************************************************************************
 * Global Symbols
 ************************************************************************************/

	.syntax	unified
	.thumb
	.file	"arm_vectorirq.S"

/************************************************************************************
 * Global Data
 ************************************************************************************/

/************************************************************************************
 * Assembly Macros
 ************************************************************************************/

/************************************************************************************
 * Private Functions
 ************************************************************************************/

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: up_vectorirq
 *
 * Description:
 *   Interrupt exception. Entered in IRQ mode with spsr = SVC CPSR, lr = SVC PC
 *
 ************************************************************************************/

	.thumb_func
	.globl	up_vectorirq
	.type	up_vectorirq, %function
	.section	.text.up_vectorirq
	.align	2 /* power of */
up_vectorirq:
	/* On entry, we are in IRQ mode.  We are free to use
	 * the IRQ mode r13 and r14.
	 */

	/* Context structure is pre-allocated on stack during IRQ mode SP stack initialization */

	/* Store r0-r12 into the structure */

	stmia	sp, {r0-r12}

	/* Store SPSR and update LR address based on its T flag */

	mrs	r0, spsr
	add	r1, sp, #(4*REG_CPSR)
	str	r0, [r1]

	and	r0, r0, #PSR_T_BIT
	lsr	r0, r0, #PSR_T_SHIFT
	orr	lr, lr, r0

	/* Store current lr into the structure as pc */

	add	r0, sp, #(4*REG_PC)
	sub	lr, lr, #4 /* for IRQ this is real return address */
	str	lr, [r0]

	/* Retrieve banked SP and LR from SVC mode, and store them into structure */

	mrs	r0, cpsr
	mov	r1, r0
	bic	r1, r1, #PSR_MODE_MASK
	orr	r1, r1, #PSR_MODE_SVC
	msr	cpsr_c, r1
	mov	r2, sp
	mov	r3, lr
	msr	cpsr_c, r0

	add	r0, sp, #(4*REG_SP)
	str	r2, [r0]

	add	r0, sp, #(4*REG_LR)
	str	r3, [r0]

	/* Remember pointer to regs  */

	ldr	r0, =current_regs
	str	sp, [r0]

	/* Call interrupt demuxer function */

	blx	nuttx_platform_irq_demuxer

	/* Clear pointer to regs  */

	ldr	r0, =current_regs
	mov	r1, #0
	str	r1, [r0]

	/* Read and restore banked SP and LR of SVC mode */

	add	r0, sp, #(4*REG_SP)
	ldr	r2, [r0]

	add	r0, sp, #(4*REG_LR)
	ldr	r3, [r0]

	mrs	r0, cpsr
	mov	r1, r0
	bic	r1, r1, #PSR_MODE_MASK
	orr	r1, r1, #PSR_MODE_SVC
	msr	cpsr_c, r1
	mov	sp, r2
	mov	lr, r3
	msr	cpsr_c, r0

	/* Restore current lr from the structure */

	add	r0, sp, #(4*REG_PC)
	ldr	lr, [r0]

	/* Restore SPSR */

	add	r0, sp, #(4*REG_CPSR)
	ldr	r1, [r0]
	msr	spsr_cxsf, r1

	/* Restore r0-r12 from the structure */

	ldmia	sp, {r0-r12}

	/* Return from exception */

	movs	pc, lr

	/* -- FUNCTION END -- */

	.thumb_func
	.globl	irq_vector_external_interrupt
	.type	irq_vector_external_interrupt, %function
	.section	.text.irq_vector_external_interrupt
irq_vector_external_interrupt = up_vectorirq

	.globl	current_regs
	.section	.data.current_regs
	.align	2 /* power of */
current_regs:
	.word	0

	.globl	up_vectorirq_stack_initial_frame
	.section	.isr_stack_initial_frame
	.align	2 /* power of */
up_vectorirq_stack_initial_frame:
	.skip	XCPTCONTEXT_SIZE

	.end
