/*
 * $ Copyright Broadcom Corporation $
 */
/************************************************************************************
 * arch/arm/src/armv7-r/arm_switchcontext.S
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
 * Pre-processor Definitions
 ************************************************************************************/

/************************************************************************************
 * Global Symbols
 ************************************************************************************/

	.syntax	unified
	.thumb
	.file	"arm_switchcontext.S"

/************************************************************************************
 * Macros
 ************************************************************************************/

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: up_switchcontext
 *
 * Description:
 *   Save the current thread context and restore the specified context.
 *   Full prototype is:
 *
 *   void up_switchcontext(uint32_t *saveregs, uint32_t *restoreregs);
 *
 * Return:
 *   None
 *
 ************************************************************************************/

	.thumb_func
	.globl	up_switchcontext
	.type	up_switchcontext, function
	.section	.text.up_switchcontext
	.align	2 /* power of */
up_switchcontext:

	/* Switch context (R0=saveregs, R1=restoreregs) */

	/* Save partial context as some registers caller not expect to be preserved */

	/* Save callee-save registers */

	add	r2, r0, #(4*REG_R4)
	stmia	r2, {r4-r11}

	add	r2, r0, #(4*REG_SP)
	str	sp, [r2]

	add	r2, r0, #(4*REG_PC) /* Save LR to PC to return from function during context restoring */
	str	lr, [r2]

	/* Save the current cpsr. Update its T bit if function called from Thumb */

	mrs	r3, cpsr

	and	r2, lr, #0x1
	lsl	r2, r2, #PSR_T_SHIFT
	orr	r3, r3, r2

	add	r2, r0, #(4*REG_CPSR)
	str	r3, [r2]

	/* Restore full context as it may be created by interrupt */

	mov	r0, r1
	b	up_fullcontextrestore

	.end
