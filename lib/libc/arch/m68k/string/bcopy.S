/*	$OpenBSD: src/lib/libc/arch/m68k/string/bcopy.S,v 1.5 2005/08/07 16:40:14 espie Exp $ */
/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "DEFS.h"

/*
 * This is probably not the best we can do, but it is still 2-10 times
 * faster than the C version in the portable gen directory.
 *
 * Things that might help:
 *	- unroll the longword copy loop (might not be good for a 68020)
 *	- longword align when possible (only on the 68020)
 *	- use nested DBcc instructions or use one and limit size to 64K
 */
#ifdef MEMCOPY
ENTRY(memcpy)
#else
#ifdef MEMMOVE
ENTRY(memmove)
#else
ENTRY(bcopy)
#endif
#endif
	movl	sp@(12),d1		| check count
	beq	bcdone			| == 0, do not do anything
#if defined(MEMCOPY) || defined(MEMMOVE)
	movl	sp@(4),a1		| dest address
	movl	sp@(8),a0		| src address
#else
	movl	sp@(4),a0		| src address
	movl	sp@(8),a1		| dest address
#endif
	cmpl	a1,a0			| src after dest?
	blt	bcback			| yes, must copy backwards
	movl	a0,d0
	btst	#0,d0			| src address odd?
	beq	bcfeven			| no, skip alignment
	movb	a0@+,a1@+		| yes, copy a byte
	subql	#1,d1			| adjust count
	beq	bcdone			| count 0, all done 
bcfeven:
	movl	a1,d0
	btst	#0,d0			| dest address odd?
	bne	bcfbloop		| yes, no hope for alignment, copy bytes
	movl	d1,d0			| no, both even
	lsrl	#2,d0			| convert count to longword count
	beq	bcfbloop		| count 0, skip longword loop
bcflloop:
	movl	a0@+,a1@+		| copy a longword
	subql	#1,d0			| adjust count
	bne	bcflloop		| still more, keep copying
	andl	#3,d1			| what remains
	beq	bcdone			| nothing, all done
bcfbloop:
	movb	a0@+,a1@+		| copy a byte
	subql	#1,d1			| adjust count
	bne	bcfbloop		| still more, keep going
bcdone:
#if defined(MEMCOPY) || defined(MEMMOVE)
	movl	sp@(4),d0		| dest address
#endif
	rts
bcback:
	addl	d1,a0			| src pointer to end
	addl	d1,a1			| dest pointer to end
	movl	a0,d0
	btst	#0,d0			| src address odd?
	beq	bcbeven			| no, skip alignment
	movb	a0@-,a1@-		| yes, copy a byte
	subql	#1,d1			| adjust count
	beq	bcdone			| count 0, all done 
bcbeven:
	movl	a1,d0
	btst	#0,d0			| dest address odd?
	bne	bcbbloop		| yes, no hope for alignment, copy bytes
	movl	d1,d0			| no, both even
	lsrl	#2,d0			| convert count to longword count
	beq	bcbbloop		| count 0, skip longword loop
bcblloop:
	movl	a0@-,a1@-		| copy a longword
	subql	#1,d0			| adjust count
	bne	bcblloop		| still more, keep copying
	andl	#3,d1			| what remains
	beq	bcdone			| nothing, all done
bcbbloop:
	movb	a0@-,a1@-		| copy a byte
	subql	#1,d1			| adjust count
	bne	bcbbloop		| still more, keep going
#if defined(MEMCOPY) || defined(MEMMOVE)
	movl	sp@(4),d0		| dest address
#endif
	rts
