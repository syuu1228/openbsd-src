/*	$OpenBSD: src/sys/arch/mvme88k/include/mvme187.h,v 1.5 2003/09/16 20:52:19 miod Exp $ */
/*
 * Copyright (c) 1996 Nivas Madhur
 * Copyright (c) 1999 Steve Murphree, Jr.
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
 *      This product includes software developed by Nivas Madhur.
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
 *
 */
/*
 * Mach Operating System
 * Copyright (c) 1991 Carnegie Mellon University
 * Copyright (c) 1991 OMRON Corporation
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 */
#ifndef __MACHINE_MVME187_H__
#define __MACHINE_MVME187_H__
/*
 *      MVME187 CPU board constants - derived from Luna88k
 *      This file is included from <machine/board.h>
 */

#define BUG187_START	0xFF800000	/* start of BUG PROM */
#define BUG187_SIZE	0x003FFFFF	/* size of BUG PROM */
#define SRAM_START	0xFFE00000	/* start of sram used by bug */
#define SRAM_SIZE	0x0001FFFF	/* size of sram */

#define SBC_CMMU_I	0xFFF77000	/* Single Board Computer code CMMU */
#define SBC_CMMU_D	0xFFF7F000	/* Single Board Computer data CMMU */

#define M187_ILEVEL	0xFFF4203E	/* interrupt priority level */
#define M187_IMASK	0xFFF4203F	/* interrupt mask level */
#define M187_ISRC	0x00000000	/* interrupt mask src (NULL) */
#define M187_IACK	0xFFFE0000	/* interrupt ACK base */

#endif	/* __MACHINE_MVME187_H__ */
