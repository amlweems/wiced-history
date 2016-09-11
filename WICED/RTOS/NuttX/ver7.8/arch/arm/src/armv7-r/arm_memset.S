/*
 * $ Copyright Broadcom Corporation $
 */
/*	$NetBSD: memset.S,v 1.8 2015/03/26 13:34:51 justin Exp $	*/
/*
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1995 Mark Brinicombe.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/************************************************************************************
 * Public Symbols
 ************************************************************************************/

#ifdef CONFIG_ARCH_FPU
	.cpu	cortex-r4f
#else
	.cpu	cortex-r4
#endif
	.syntax	unified
	.thumb
	.file	"arm_memset.S"

/************************************************************************************
 * .text
 ************************************************************************************/

	.text

/************************************************************************************
 * Private Constant Data
 ************************************************************************************/

/************************************************************************************
 * Public Functions
 ************************************************************************************/
/************************************************************************************
 * Name: memset
 *
 * Description:
 *   Optimised "general" set routine
 *
 * Input Parameters:
 *   r0 = destination, r1 = char, r2 = length
 *
 * Returned Value:
 *   r0 = destination r1-r3 burned
 *
 ************************************************************************************/

	.thumb_func
	.globl	memset
	.type	memset, function
	.section	.text.memset
	.align	2 /* power of */
memset:
	/* memset returns destination pointer, so save it and restore */
	push	{r0, lr}
	bl	_do_memset
	pop	{r0, pc}

_do_memset:
	ands	r3, r0, #3
	/* deal unaligned bytes */
	bne	6f

	/* aligned to a word boundary. */
1:	orr	r1, r1, r1, lsl #8
	orr	r1, r1, r1, lsl #16
	mov	r3, r1
	cmp	r2, #16
	blt	4f

	/* use r8 and LR for the loop */
	stmfd	sp!, {r8, lr}
	mov	r8, r1
	mov	lr, r1

	/* repeat 64 bytes at a time */
2:	subs	r2, r2, #64
	itttt 	ge
	stmge	r0!, {r1, r3, r8, lr}
	stmge	r0!, {r1, r3, r8, lr}
	stmge	r0!, {r1, r3, r8, lr}
	stmge	r0!, {r1, r3, r8, lr}
	bgt	2b
	/* exit if zero */
	it	eq
	ldmfdeq	sp!, {r8, pc}

	/* 32 bytes or more */
	tst	r2, #32
	itt	ne
	stmne	r0!, {r1, r3, r8, lr}
	stmne	r0!, {r1, r3, r8, lr}
	/* 16 bytes or more */
	tst	r2, #16
	it	ne
	stmne	r0!, {r1, r3, r8, lr}
	ldmfd	sp!, {r8, lr}

	/* 8 bytes or more */
4:	tst	r2, #8
	it	ne
	stmne	r0!, {r1, r3}
	/* 4 bytes or more */
	tst	r2, #4
	it	ne
	strne	r1, [r0], #4
	/* 2 bytes or more */
5:	tst	r2, #2
	itt	ne
	strbne	r1, [r0], #1
	strbne	r1, [r0], #1
	/* 1 byte */
	tst	r2, #1
	it	ne
	strbne	r1, [r0], #1
	/* return */
	bx	lr

6:	subs	r2, r2, #4
	/* <4, do byte set */
	blt	5b
	cmp	r3, #2
	/* r3 < 2 - 0b01*/
	it	lt
	strblt	r1, [r0], #1
	/* r3 <= 2 - 0b01 or 0b10 */
	it	le
	strble	r1, [r0], #1
	/* all - 0b01, 0b10 or 0b11 */
	strb	r1, [r0], #1
	/* update the size r2 = r2 - (4 - r3) */
	add	r2, r2, r3
	b	1b

	.size	memset, .-memset
	.end
