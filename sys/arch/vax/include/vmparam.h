/*	$OpenBSD: src/sys/arch/vax/include/vmparam.h,v 1.16 2001/11/30 17:37:43 art Exp $	*/
/*	$NetBSD: vmparam.h,v 1.32 2000/03/07 00:05:59 matt Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Slightly modified for the VAX port /IC
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)vmparam.h	5.9 (Berkeley) 5/12/91
 */
#ifndef _VMPARAM_H_
#define _VMPARAM_H_

/*
 * Machine dependent constants for VAX.
 */

/*
 * USRTEXT is the start of the user text/data space, while USRSTACK
 * is the top (end) of the user stack. Immediately above the user stack
 * resides kernel.
 *
 */

#define USRTEXT		NBPG
#define USRSTACK	KERNBASE

/*
 * Virtual memory related constants, all in bytes
 */

#ifndef MAXTSIZ
#define MAXTSIZ		(8*1024*1024)		/* max text size */
#endif
#ifndef MAXDSIZ
#define MAXDSIZ		(24*1024*1024)		/* max data size */
#endif
#ifndef MAXSSIZ
#define MAXSSIZ		(8*1024*1024)		/* max stack size */
#endif
#ifndef DFLDSIZ
#define DFLDSIZ		(16*1024*1024)		/* initial data size limit */
#endif
#ifndef DFLSSIZ
#define DFLSSIZ		(512*1024)		/* initial stack size limit */
#endif

/*
 * All mmap()'ed data will be mapped above MAXDSIZ. This means that
 * pte space must be allocated for (possible) mmap()'ed data.
 * Note: This is just a hint, if we mmap() more than this the page
 * table will be expanded. (at the cost of speed).
 */
#define MMAPSPACE	(8*1024*1024)

/* 
 * Size of shared memory map
 */

#ifndef SHMMAXPGS
#define SHMMAXPGS	64		/* XXXX should be 1024 */
#endif

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.	 You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a ``long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */

#define MAXSLP		20

#define VM_PHYSSEG_MAX		1
#define VM_PHYSSEG_NOADD
#define VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH /* XXX */

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

#define __HAVE_PMAP_PHYSSEG
struct pmap_physseg {
	int	dummy;
};

/* MD round macros */
#define	vax_round_page(x) (((vaddr_t)(x) + VAX_PGOFSET) & ~VAX_PGOFSET)
#define	vax_trunc_page(x) ((vaddr_t)(x) & ~VAX_PGOFSET)

/* user/kernel map constants */
#define VM_MIN_ADDRESS		((vm_offset_t)0)
#define VM_MAXUSER_ADDRESS	((vm_offset_t)KERNBASE)
#define VM_MAX_ADDRESS		((vm_offset_t)KERNBASE)
#define VM_MIN_KERNEL_ADDRESS	((vm_offset_t)KERNBASE)
#define VM_MAX_KERNEL_ADDRESS	((vm_offset_t)(0xC0000000))

#define	USRIOSIZE		(8 * VAX_NPTEPG)	/* 512MB */
#define	VM_PHYS_SIZE		(USRIOSIZE*VAX_NBPG)

/* virtual sizes (bytes) for various kernel submaps */
#define VM_KMEM_SIZE		(NKMEMCLUSTERS*PAGE_SIZE)

#define VM_MBUF_SIZE          (NMBCLUSTERS*MCLBYTES)

#endif
