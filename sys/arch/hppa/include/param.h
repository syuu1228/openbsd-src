/*	$OpenBSD: src/sys/arch/hppa/include/param.h,v 1.2 1998/10/30 19:28:13 mickey Exp $	*/

/* 
 * Copyright (c) 1988-1994, The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: param.h 1.18 94/12/16$
 */

#include <machine/cpu.h>
#include <machine/intr.h>

/*
 * Machine dependent constants for PA-RISC.
 */

#define	_MACHINE	hp700
#define MACHINE		"hp700"
#define	_MACHINE_ARCH	hppa
#define	MACHINE_ARCH	"hppa"
#define	MID_MACHINE	MID_HPUX800

/*
 * Round p (pointer or byte index) up to a correctly-aligned value for all
 * data types (int, long, ...).   The result is u_int and must be cast to
 * any desired pointer type.
 */
#define	ALIGNBYTES	3
#define	ALIGN(p)	(((u_long)(p) + ALIGNBYTES) &~ ALIGNBYTES)

#define	NBPG		4096		/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */
#define	PGSHIFT		12		/* LOG2(NBPG) */

#define	SEGSHIFT	(PGSHIFT + (PGSHIFT-PTESHIFT))	/* LOG2(NBSEG) */
#define NBSEG		(1 << SEGSHIFT)	/* bytes/segment (quadrant) */
#define	SEGOFSET	(NBSEG-1)	/* byte offset into segment */

#define	KERNBASE	0x00000000	/* start of kernel virtual */
#define	BTOPKERNBASE	((u_long)KERNBASE >> PGSHIFT)

#define	DEV_BSIZE	512
#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define BLKDEV_IOSIZE	2048
#define	MAXPHYS		(64 * 1024)	/* max raw I/O transfer size */

#define	STACK_GROWS_UP	1		/* stack grows to higher addresses */

#define	CLSIZE		1
#define	CLSIZELOG2	0

/* NOTE: SSIZE, SINCR and UPAGES must be multiples of CLSIZE */
#define	SSIZE		(1)		/* initial stack size/NBPG */
#define	SINCR		(1)		/* increment of stack/NBPG */

#define	UPAGES		(2)		/* pages of u-area */
#define	USPACE		(UPAGES * NBPG)	/* pages for user struct */

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than CLBYTES (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#define	MSIZE		128		/* size of an mbuf */
#define	MCLSHIFT	11
#define	MCLBYTES	(1 << MCLSHIFT)	/* large enough for ether MTU */
#define	MCLOFSET	(MCLBYTES - 1)
#ifndef NMBCLUSTERS
#ifdef GATEWAY
#define	NMBCLUSTERS	(1024)		/* cl map size: 1MB */
#else
#define	NMBCLUSTERS	(512)		/* cl map size: 0.5MB */
#endif
#endif

/*
 * Size of kernel malloc arena in CLBYTES-sized logical pages
 */
#ifndef NKMEMCLUSTERS
#define	NKMEMCLUSTERS	(8192*1024/CLBYTES)
#endif

/* pages ("clicks") (4096 bytes) to disk blocks */
#define	ctod(x)	((x)<<(PGSHIFT-DEV_BSHIFT))
#define	dtoc(x)	((x)>>(PGSHIFT-DEV_BSHIFT))

/* pages to bytes */
#define	ctob(x)	((x)<<PGSHIFT)
#define	btoc(x)	(((unsigned)(x)+(NBPG-1))>>PGSHIFT)

#define	btodb(bytes)	((unsigned)(bytes) >> DEV_BSHIFT)
#define	dbtob(db)	((unsigned)(db) << DEV_BSHIFT)

/*
 * Map a ``block device block'' to a file system block.
 * This should be device dependent, and should use the bsize
 * field from the disk label.
 * For now though just use DEV_BSIZE.
 */
#define	bdbtofsb(bn)	((bn) / (BLKDEV_IOSIZE/DEV_BSIZE))

/*
 * Mach derived conversion macros
 */
#define hppa_round_page(x)	((((unsigned long)(x)) + NBPG - 1) & ~(NBPG-1))
#define hppa_trunc_page(x)	((unsigned long)(x) & ~(NBPG-1))

#define btop(x)		((unsigned long)(x) >> PGSHIFT)
#define ptob(x)		((unsigned long)(x) << PGSHIFT)

#ifdef _KERNEL
#ifdef COMPAT_HPUX
/*
 * Constants/macros for HPUX multiple mapping of user address space.
 * Pages in the first 256Mb are mapped in at every 256Mb segment.
 */
#define HPMMMASK	0xF0000000
#define ISHPMMADDR(v)	0		/* XXX ...jef */
#define HPMMBASEADDR(v)	((unsigned)(v) & ~HPMMMASK)
#endif

#endif
