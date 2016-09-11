/*
 * $ Copyright Broadcom Corporation $
 */
/************************************************************************************
 * arch/arm/src/armv7-r/arm_fullcontextrestore.S
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

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/

/************************************************************************************
 * Global Symbols
 ************************************************************************************/

	.syntax	unified
	.file	"arm_fullcontextrestore.S"

/************************************************************************************
 * Macros
 ************************************************************************************/

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: up_fullcontextrestore
 *
 * Description:
 *   Restore the specified specified context.
 *   Full prototype is:
 *
 *   void up_fullcontextrestore(uint32_t *restoreregs);
 *
 * Return:
 *   None
 *
 ************************************************************************************/

	.thumb_func
	.globl	up_fullcontextrestore
	.type	up_fullcontextrestore, function
	.section	.text.up_fullcontextrestore
	.align	2 /* power of */
up_fullcontextrestore:

	.thumb

	/* Restore context (R0=restoreregs) */

	/* Restore stack pointer first */

	add	r1, r0, #(4*REG_SP)
	ldr	sp, [r1]

	/* Load and push to stack registers which will be restored before leaving function */

	add	r1, r0, #(4*REG_R0)
	ldr	r2, [r1]

	add	r1, r0, #(4*REG_R1)
	ldr	r3, [r1]

	add	r1, r0, #(4*REG_PC)
	ldr	r4, [r1]

	push	{r2, r3, r4 }

	/* Restore some registers */

	add	r1, r0, #(4*REG_R2)
	ldmia	r1, {r2-r12}

	add	r1, r0, #(4*REG_LR)
	ldr	lr, [r1]

	/*
	 * Restore SPSR.
	 * As last step SPSR will be copied to CPSR.
	 */

	add	r1, r0, #(4*REG_CPSR)
	ldr	r1, [r1]
	msr	spsr_fsxc, r1

	/*
	 * Switch to ARM mode.
	 * This is needed to execute ldm instruction with ^ suffix.
	 * PC in Thumb mode has no least significant bit set and points 4 bytes ahead.
	 * BX and NOP instructions are each 2 bytes long, so having BX and NOP let us jump
	 * just after them.
	 */

	.align	2 /* power of */

	bx.n	pc
	nop.n

	.arm

	/*
	 * Restore remaining registers, CPSR and jump to saved program counter.
	 * We wait until we are completely finished with the context data to do this.
	 * Restore the CPSR may re-enable interrupts and we could be in execution context
	 * where context data structure is only protected by interrupts being
	 * disabled.
	 */

	ldm	sp!, {r0, r1, pc}^

	.end
