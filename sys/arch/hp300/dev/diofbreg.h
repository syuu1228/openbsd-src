/*	$OpenBSD: src/sys/arch/hp300/dev/diofbreg.h,v 1.4 2011/08/18 20:02:57 miod Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *
 * from: Utah $Hdr: grfreg.h 1.6 92/01/31$
 *
 *	@(#)grfreg.h	8.1 (Berkeley) 6/10/93
 */

/* 300 bitmapped display hardware primary id */
#define GRFHWID		0x39

/* 300 internal bitmapped display address */
#define GRFIADDR	0x560000

/* 300 hardware secondary ids - XXX duplicates some diodevs.h values */
#define	GID_GATORBOX	0x01
#define	GID_TOPCAT	0x02
#define	GID_RENAISSANCE	0x04
#define	GID_LRCATSEYE	0x05
#define	GID_HRCCATSEYE	0x06
#define	GID_HRMCATSEYE	0x07
#define	GID_DAVINCI	0x08
#define	GID_XXXCATSEYE	0x09
#define	GID_XGENESIS	0x0b
#define	GID_TIGER	0x0c
#define	GID_YGENESIS	0x0d
#define	GID_HYPERION	0x0e
#define	GID_FB3X2_A	0x10
#define	GID_FB3X2_B	0x11

#ifndef	_LOCORE
struct	diofbreg {
	u_int8_t	:8;
	u_int8_t	id;		/* id and reset register	0x01 */
	u_int8_t 	sec_interrupt;	/* secondary interrupt register	0x02 */
	u_int8_t	interrupt;	/* interrupt register		0x03 */
	u_int8_t	:8;
	u_int8_t	fbwmsb;		/* frame buffer width MSB	0x05 */
	u_int8_t	:8;
	u_int8_t	fbwlsb;		/* frame buffer height LSB	0x07 */
	u_int8_t	:8;
	u_int8_t	fbhmsb;		/* frame buffer height MSB	0x09 */
	u_int8_t	:8;
	u_int8_t	fbhlsb;		/* frame buffer height LSB	0x0b */
	u_int8_t	:8;
	u_int8_t	dwmsb;		/* display width MSB		0x0d */
	u_int8_t	:8;
	u_int8_t	dwlsb;		/* display width LSB		0x0f */
	u_int8_t	:8;
	u_int8_t	dhmsb;		/* display height MSB		0x11 */
	u_int8_t	:8;
	u_int8_t	dhlsb;		/* display height LSB		0x13 */
	u_int8_t	:8;
	u_int8_t	fbid;		/* frame buffer id		0x15 */
	u_int8_t	pad2[0x45];
	u_int8_t	num_planes;	/* number of color planes	0x5b */
	u_int8_t	:8;
	u_int8_t	fbomsb;		/* frame buffer offset MSB	0x5d */
	u_int8_t	:8;
	u_int8_t	fbolsb;		/* frame buffer offset LSB	0x5f */
};
#endif
