/*	$OpenBSD: src/sys/arch/hp300/stand/include/rominfo.h,v 1.2 2003/06/02 23:27:46 millert Exp $	*/
/*	$NetBSD: rominfo.h,v 1.5 1994/10/26 07:27:53 cgd Exp $	*/

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
 * from: Utah $Hdr: rominfo.h 1.2 88/05/24$
 *
 *	@(#)rominfo.h	8.1 (Berkeley) 6/10/93
 */

#define ROMADDR	0xFFFFF000

struct jmpvec {
	short op;	/* jmp instruction */
	long  addr;	/* address */
};

struct rominfo {
	char p1[0xDC0];
	short boottype;		/* ??				(FFFFFDC0) */
	char  name[10];		/* HP system name, e.g. SYSHPUX (FFFFFDC2) */
	short p2;		/* ??				(FFFFFDCC) */
	long  lowram;		/* lowest useable RAM location	(FFFFFDCE) */
	char  p3[0x100];	/* ??				(FFFFFDD2) */
	char  sysflag;		/* HP system flags		(FFFFFED2) */
	char  p4;		/* ??				(FFFFFED3) */
	long  rambase;		/* physaddr of lowest RAM	(FFFFFED4) */
	char  ndrives;		/* number of drives		(FFFFFED8) */
	char  p5;		/* ??				(FFFFFED9) */
	char  sysflag2;		/* more system flags		(FFFFFEDA) */
	char  p6;		/* ??				(FFFFFEDB) */
	long  msus;		/* ??				(FFFFFEDC) */
	struct jmpvec jvec[48];	/* jump vectors			(FFFFFEE0) */
};
