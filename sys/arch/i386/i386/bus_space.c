/*	$OpenBSD: src/sys/arch/i386/i386/bus_space.c,v 1.4 2009/07/16 21:07:41 mk Exp $ */
/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1996 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1996 Jason R. Thorpe.  All rights reserved.
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *	This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <machine/bus.h>

u_int8_t
bus_space_read_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o)
{
	return ((t) == I386_BUS_SPACE_IO ? (inb((h) + (o))) :
	    (*(volatile u_int8_t *)((h) + (o))));
}

u_int16_t
bus_space_read_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o)
{
	return ((t) == I386_BUS_SPACE_IO ? (inw((h) + (o))) :
	    (*(volatile u_int16_t *)((h) + (o))));
}

u_int32_t
bus_space_read_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o)
{
	return ((t) == I386_BUS_SPACE_IO ? (inl((h) + (o))) :
	    (*(volatile u_int32_t *)((h) + (o))));
}


void
bus_space_read_multi_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    u_int8_t *a, bus_size_t cnt)
{
	if (t == I386_BUS_SPACE_IO) {
		insb(h + o, a, cnt);
	} else {
		void *_addr=a;
		int _cnt=cnt;
		__asm __volatile("cld				;"
		"1:	movb (%2),%%al				;"
		"	stosb					;"
		"	loop 1b"				:
		    "+D" (_addr), "+c" (_cnt) : "r" (h + o)	:
		    "%eax", "memory", "cc");
	}
}

void
bus_space_read_multi_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    u_int16_t *a, bus_size_t cnt)
{
	if (t == I386_BUS_SPACE_IO) {
		insw(h + o, a, cnt);
	} else {
		void *_addr=a;
		int _cnt=cnt;
		__asm __volatile("cld				;"
		"1:	movw (%2),%%ax				;"
		"	stosw					;"
		"	loop 1b"				:
		    "+D" (_addr), "+c" (_cnt) : "r" ((h) + (o))	:
		    "%eax", "memory", "cc");
	}
}
void
bus_space_read_multi_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    u_int32_t *a, bus_size_t cnt)
{
	if (t == I386_BUS_SPACE_IO) {
		insl(h + o, a, cnt);
	} else {
		void *_addr=a;
		int _cnt=cnt;
		__asm __volatile("cld				;"
		"1:	movl (%2),%%eax				;"
		"	stosl					;"
		"	loop 1b"				:
		    "+D" (_addr), "+c" (_cnt) : "r" (h + o)	:
		    "%eax", "memory", "cc");
	}
}

void
bus_space_read_region_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int8_t *a, bus_size_t cnt)
{
	int _cnt = cnt;
	void *_addr = a;
	int _port = h + o;

	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile("cld				;"
		"1:	inb %w2,%%al				;"
		"	stosb					;"
		"	incl %2					;"
		"	loop 1b"				:
		    "+D" (_addr), "+c" (_cnt), "+d" (_port)	::
		    "%eax", "memory", "cc");
	} else
		i386_space_copy(_port, _addr, 1, _cnt);
}

void
bus_space_read_region_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int16_t *a, bus_size_t cnt)
{
	int _cnt = cnt;
	void *_addr = a;
	int _port = h + o;

	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile("cld				;"
		"1:	inw %w2,%%ax				;"
		"	stosw					;"
		"	addl $2,%2				;"
		"	loop 1b"				:
		    "+D" (_addr), "+c" (_cnt), "+d" (_port)	::
		    "%eax", "memory", "cc");
	} else
		i386_space_copy(_port, _addr, 2, _cnt);
}

void
bus_space_read_region_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int32_t *a, bus_size_t cnt)
{
	int _cnt = cnt;
	void *_addr = a;
	int _port = h + o;

	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile("cld				;"
		"1:	inl %w2,%%eax				;"
		"	stosl					;"
		"	addl $4,%2				;"
		"	loop 1b"				:
		    "+D" (_addr), "+c" (_cnt), "+d" (_port)	::
		    "%eax", "memory", "cc");
	} else
		i386_space_copy(_port, _addr, 4, _cnt);
}

void
bus_space_write_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int8_t v)
{
	if (t == I386_BUS_SPACE_IO)
		outb(h + o, v);
	else
		((void)(*(volatile u_int8_t *)(h + o) = v));
}

void
bus_space_write_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int16_t v)
{
	if ((t) == I386_BUS_SPACE_IO)
		outw(h + o, v);
	else
		((void)(*(volatile u_int16_t *)(h + o) = v));
}

void
bus_space_write_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int32_t v)
{
	if ((t) == I386_BUS_SPACE_IO)
		outl(h + o, v);
	else
		((void)(*(volatile u_int32_t *)(h + o) = v));
}

void
bus_space_write_multi_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const u_int8_t *a, bus_size_t cnt)
{
	if (t == I386_BUS_SPACE_IO) {
		outsb(h + o, a, cnt);
	} else {
		const void *_addr=a;
		int _cnt=cnt;

		__asm __volatile("cld				;"
		"1:	lodsb					;"
		"	movb %%al,(%2)				;"
		"	loop 1b"				:
		    "+S" (_addr), "+c" (_cnt) : "r" (h + o)	:
		    "%eax", "memory", "cc");
	}
}

void
bus_space_write_multi_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const u_int16_t *a, bus_size_t cnt)
{
	if (t == I386_BUS_SPACE_IO) {
		outsw(h + o, a, cnt);
	} else {
		const void *_addr = a;
		int _cnt = cnt;

		__asm __volatile("cld				;"
		"1:	lodsw					;"
		"	movw %%ax,(%2)				;"
		"	loop 1b"				:
		    "+S" (_addr), "+c" (_cnt) : "r" (h + o)	:
		    "%eax", "memory", "cc");
	}
}
void
bus_space_write_multi_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const u_int32_t *a, bus_size_t cnt)
{
	if (t == I386_BUS_SPACE_IO) {
		outsl(h + o, a, cnt);
	} else {
		const void *_addr=a;
		int _cnt=cnt;

		__asm __volatile("cld				;"
		"1:	lodsl					;"
		"	movl %%eax,(%2)				;"
		"	loop 1b"				:
		    "+S" (_addr), "+c" (_cnt) : "r" (h + o)	:
		    "%eax", "memory", "cc");
	}
}

void
bus_space_write_region_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const u_int8_t *a, bus_size_t cnt)
{
	int _port = h + o;
	const void *_addr = a;
	int _cnt = cnt;

	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile("cld				;"
		"1:	lodsb					;"
		"	outb %%al,%w0				;"
		"	incl %0					;"
		"	loop 1b"				:
		    "+d" (_port), "+S" (_addr), "+c" (_cnt)	::
		    "%eax", "memory", "cc");
	} else
		i386_space_copy(_addr, _port, 1, _cnt);
}

void
bus_space_write_region_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const u_int16_t *a, bus_size_t cnt)
{
	int _port = h + o;
	const void *_addr = a;
	int _cnt = cnt;

	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile("cld				;"
		"1:	lodsw					;"
		"	outw %%ax,%w0				;"
		"	addl $2,%0				;"
		"	loop 1b"				:
		    "+d" (_port), "+S" (_addr), "+c" (_cnt)	::
		    "%eax", "memory", "cc");
	} else
		i386_space_copy(_addr, _port, 2, _cnt);
}

void
bus_space_write_region_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const u_int32_t *a, bus_size_t cnt)
{
	int _port = h + o;
	const void *_addr = a;
	int _cnt = cnt;

	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile("cld				;"
		"1:	lodsl					;"
		"	outl %%eax,%w0				;"
		"	addl $4,%0				;"
		"	loop 1b"				:
		    "+d" (_port), "+S" (_addr), "+c" (_cnt)	::
		    "%eax", "memory", "cc");
	} else
		i386_space_copy(_addr, _port, 4, _cnt);
}

void
bus_space_set_multi_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int8_t v, size_t cnt)
{
	int _cnt = cnt;

	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile("cld				;"
		"1:	outb %b2, %w1				;"
		"	loop 1b"				:
		    "+c" (_cnt) : "d" (h + o), "a" (v)		:
		    "cc");
	} else {
		__asm __volatile("cld				;"
		"1:	movb %b2, (%1)				;"
		"	loop 1b"				:
		    "+c" (_cnt) : "D" (h + o), "a" (v)		:
		    "cc", "memory");
	}
}

void
bus_space_set_multi_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int16_t v, size_t cnt)
{
	int _cnt = cnt;

	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile("cld				;"
		"1:	outw %w2, %w1				;"
		"	loop 1b"				:
		    "+c" (_cnt) : "d" (h + o), "a" (v)	:
		    "cc");
	} else {
		__asm __volatile("cld				;"
		"1:	movw %w2, (%1)				;"
		"	loop 1b"				:
		    "+c" (_cnt) : "D" (h + o), "a" (v)		:
		    "cc", "memory");
	}
}

void
bus_space_set_multi_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int32_t v, size_t cnt)
{
	int _cnt = cnt;

	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile("cld				;"
		"1:	outl %2,%w1				;"
		"	loop 1b"				:
		    "+c" (_cnt) : "d" (h + o), "a" (v)	:
		    "cc");
	} else {
		__asm __volatile("cld				;"
		"1:	movl %2,(%1)				;"
		"	loop 1b"				:
		    "+c" (_cnt) : "D" (h + o), "a" (v)	:
		    "cc", "memory");
	}
}

void
bus_space_set_region_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int8_t v, size_t cnt)
{
	int _port = h + o;
	int _cnt = cnt;

	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile(
		"1:	outb %%al,%w0				;"
		"	incl %0					;"
		"	loop 1b"				:
		    "+d" (_port), "+c" (_cnt) : "a" (v)	:
		    "cc");
	} else {
		__asm __volatile("cld				;"
		"	repne					;"
		"	stosb"					:
		    "+D" (_port), "+c" (_cnt) : "a" (v)	:
		    "memory", "cc");
	}
}

void
bus_space_set_region_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int16_t v, size_t cnt)
{
	int _port = h + o;
	int _cnt = cnt;

	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile(
		"1:	outw %%ax,%w0				;"
		"	addl $2, %0				;"
		"	loop 1b"				:
		    "+d" (_port), "+c" (_cnt) : "a" (v)		:
		    "cc");
	} else {
		__asm __volatile("cld				;"
		"	repne					;"
		"	stosw"					:
		    "+D" (_port), "+c" (_cnt) : "a" (v)	:
		    "memory", "cc");
	}
}

void
bus_space_set_region_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int32_t v, size_t cnt)
{
	int _port = h + o;
	int _cnt = cnt;

	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile(
		"1:	outl %%eax,%w0				;"
		"	addl $4, %0				;"
		"	loop 1b"				:
		    "+d" (_port), "+c" (_cnt) : "a" (v)		:
		    "cc");
	} else {
		__asm __volatile("cld				;"
		"	repne					;"
		"	stosl"					:
		    "+D" (_port), "+c" (_cnt) : "a" (v)	:
		    "memory", "cc");
	}
}

void
bus_space_copy_1(bus_space_tag_t t, bus_space_handle_t h1, bus_size_t o1,
     bus_space_handle_t h2, bus_size_t o2, bus_size_t cnt)
{
	int _port1 = h1 + o1;
	int _port2 = h2 + o2;
	int _cnt = cnt;

	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile(
		"1:	movl %k1,%%edx				;"
		"	inb  %%dx,%%al				;"
		"	movl %k0,%%edx				;"
		"	outb %%al,%%dx				;"
		"	incl %0					;"
		"	incl %1					;"
		"	loop 1b"				:
		    "+D" (_port2), "+S" (_port1), "+c" (_cnt)	::
		    "%edx", "%eax", "cc");
	} else
		i386_space_copy(_port1, _port2, 1, _cnt);
}

void
bus_space_copy_2(bus_space_tag_t t, bus_space_handle_t h1, bus_size_t o1,
     bus_space_handle_t h2, bus_size_t o2, bus_size_t cnt)
{
	int _port1 = h1 + o1;
	int _port2 = h2 + o2;
	int _cnt=cnt;
	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile(
		"1:	movl %k1,%%edx				;"
		"	inw  %%dx,%%ax				;"
		"	movl %k0,%%edx				;"
		"	outw %%ax,%%dx				;"
		"	addl $2, %0				;"
		"	addl $2, %1				;"
		"	loop 1b"				:
		    "+D" (_port2), "+S" (_port1), "+c" (_cnt)	::
		    "%edx", "%eax", "cc");
	} else
		i386_space_copy(_port1, _port2, 2, _cnt);
}

void
bus_space_copy_4(bus_space_tag_t t, bus_space_handle_t h1, bus_size_t o1,
     bus_space_handle_t h2, bus_size_t o2, bus_size_t cnt)
{
	int _port1 = h1 + o1;
	int _port2 = h2 + o2;
	int _cnt = cnt;

	if (t == I386_BUS_SPACE_IO) {
		__asm __volatile(
		"1:	movl %k1,%%edx				;"
		"	inl  %%dx,%%eax				;"
		"	movl %k0,%%edx				;"
		"	outl %%eax,%%dx				;"
		"	addl $4, %0				;"
		"	addl $4, %1				;"
		"	loop 1b"				:
		    "+D" (_port2), "+S" (_port1), "+c" (_cnt)	::
		    "%edx", "%eax", "cc");
	} else
		i386_space_copy(_port1, _port2, 4, _cnt);
}
