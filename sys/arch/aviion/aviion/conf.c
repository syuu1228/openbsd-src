/*	$OpenBSD: src/sys/arch/aviion/aviion/conf.c,v 1.15 2011/10/06 20:49:27 deraadt Exp $	*/

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 *      @(#)conf.c	7.9 (Berkeley) 5/28/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/vnode.h>

#include <machine/conf.h>

#include "bpfilter.h"
#include "cd.h"
#include "ch.h"
#include "dart.h"
#include "ksyms.h"
#include "nvram.h"
#include "pf.h"
#include "bio.h"
#include "pty.h"
#include "rd.h"
#include "sd.h"
#include "st.h"
#include "systrace.h"
#include "tun.h"
#include "uk.h"
#include "vme.h"
#include "vnd.h"
#include "vscsi.h"
#include "pppx.h"
#ifdef NNPFS
#include <nnpfs/nnnpfs.h>
cdev_decl(nnpfs_dev);
#endif

struct bdevsw	bdevsw[] =
{
	bdev_notdef(),			/* 0 */
	bdev_notdef(),			/* 1 */
	bdev_notdef(),			/* 2 */
	bdev_swap_init(1,sw),		/* 3: swap pseudo-device */
	bdev_disk_init(NSD,sd),		/* 4: SCSI disk */
	bdev_tape_init(NST,st),		/* 5: SCSI tape */
	bdev_disk_init(NCD,cd),		/* 6: SCSI CD-ROM */
	bdev_disk_init(NRD,rd),		/* 7: ramdisk */
	bdev_disk_init(NVND,vnd),	/* 8: vnode disk driver */
	bdev_notdef(),			/* 9: was: concatenated disk driver */
	bdev_notdef(),			/* 10 */
	bdev_notdef(),			/* 11 */
	bdev_notdef(),			/* 12 */
	bdev_lkm_dummy(),		/* 13 */
	bdev_lkm_dummy(),		/* 14 */
	bdev_lkm_dummy(),		/* 15 */
	bdev_lkm_dummy(),		/* 16 */
	bdev_lkm_dummy(),		/* 17 */
	bdev_lkm_dummy(),		/* 18 */
};
int	nblkdev = nitems(bdevsw);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_ctty_init(1,ctty),		/* 1: controlling terminal */
	cdev_mm_init(1,mm),		/* 2: /dev/{null,mem,kmem,...} */
	cdev_notdef(),			/* 3 was /dev/drum */
	cdev_tty_init(NPTY,pts),	/* 4: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 5: pseudo-tty master */
	cdev_log_init(1,log),		/* 6: /dev/klog */
	cdev_notdef(),			/* 7 */
	cdev_disk_init(NSD,sd),		/* 8: SCSI disk */
	cdev_disk_init(NCD,cd),		/* 9: SCSI CD-ROM */
	cdev_mm_init(NNVRAM,nvram),	/* 10: /dev/nvramX */
	cdev_notdef(),			/* 11: */
	cdev_tty_init(NDART,dart),	/* 12: on-board serial (tty[ab]) */
	cdev_notdef(),			/* 13 */
	cdev_notdef(),			/* 14 */
	cdev_notdef(),			/* 15 */
	cdev_notdef(),			/* 16 */
	cdev_notdef(),			/* 17: was: concatenated disk */
	cdev_disk_init(NRD,rd),		/* 18: ramdisk disk */
	cdev_disk_init(NVND,vnd),	/* 19: vnode disk */
	cdev_tape_init(NST,st),		/* 20: SCSI tape */
	cdev_fd_init(1,filedesc),	/* 21: file descriptor pseudo-dev */
	cdev_bpf_init(NBPFILTER,bpf),	/* 22: berkeley packet filter */
	cdev_tun_init(NTUN,tun),	/* 23: network tunnel */
	cdev_lkm_init(NLKM,lkm),	/* 24: loadable module driver */
	cdev_notdef(),			/* 25 */
	cdev_notdef(),			/* 26: XD disk */
	cdev_notdef(),			/* 27 */
	cdev_notdef(),			/* 28: lp */
	cdev_notdef(),			/* 29: lptwo */
	cdev_mm_init(NVME,vme),		/* 30: /dev/vme* */
	cdev_notdef(),			/* 31: old /dev/vmelX */
	cdev_notdef(),			/* 32: old /dev/vmesX */
	cdev_lkm_dummy(),		/* 33 */
	cdev_lkm_dummy(),		/* 34 */
	cdev_lkm_dummy(),		/* 35 */
	cdev_lkm_dummy(),		/* 36 */
	cdev_lkm_dummy(),		/* 37 */
	cdev_lkm_dummy(),		/* 38 */
	cdev_pf_init(NPF,pf),		/* 39: packet filter */
	cdev_random_init(1,random),	/* 40: random data source */
	cdev_uk_init(NUK,uk),		/* 41 */
	cdev_notdef(),			/* 42 */
	cdev_ksyms_init(NKSYMS,ksyms),	/* 43: Kernel symbols device */
	cdev_ch_init(NCH,ch),		/* 44: SCSI autochanger */
	cdev_notdef(),			/* 45 */
	cdev_notdef(),			/* 46 */
	cdev_notdef(),			/* 47 */
	cdev_notdef(),			/* 48 */
	cdev_bio_init(NBIO,bio),	/* 49: ioctl tunnel */
	cdev_systrace_init(NSYSTRACE,systrace),	/* 50 system call tracing */
#ifdef NNPFS
	cdev_nnpfs_init(NNNPFS,nnpfs_dev),	/* 51: nnpfs communication device */
#else
	cdev_notdef(),			/* 51 */
#endif
	cdev_ptm_init(NPTY,ptm),	/* 52: pseudo-tty ptm device */
	cdev_vscsi_init(NVSCSI,vscsi),	/* 53: vscsi */
	cdev_disk_init(1,diskmap),	/* 54: disk mapper */
	cdev_pppx_init(NPPPX,pppx),	/* 55: pppx */
};
int	nchrdev = nitems(cdevsw);

int	mem_no = 2;	/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(3, 0);

/*
 * Returns true if dev is /dev/mem or /dev/kmem.
 */
int
iskmemdev(dev)
	dev_t dev;
{

	return (major(dev) == mem_no && minor(dev) < 2);
}

/*
 * Returns true if dev is /dev/zero.
 */
int
iszerodev(dev)
	dev_t dev;
{

	return (major(dev) == mem_no && minor(dev) == 12);
}

dev_t
getnulldev()
{
	return makedev(mem_no, 2);
}

int chrtoblktbl[] = {
	/* XXXX This needs to be dynamic for LKMs. */
	/*VCHR*/	/*VBLK*/
	/*  0 */	NODEV,
	/*  1 */	NODEV,
	/*  2 */	NODEV,
	/*  3 */	NODEV,
	/*  4 */	NODEV,
	/*  5 */	NODEV,
	/*  6 */	NODEV,
	/*  7 */	NODEV,
	/*  8 */	4,	/* sd */
	/*  9 */	6,	/* cd */
	/* 10 */	NODEV,
	/* 11 */	NODEV,
	/* 12 */	NODEV,
	/* 13 */	NODEV,
	/* 14 */	NODEV,
	/* 15 */	NODEV,
	/* 16 */	NODEV,
	/* 17 */	NODEV,
	/* 18 */	7,	/* rd */
	/* 19 */	8,	/* vnd */
	/* 20 */	5,	/* st */
};
int nchrtoblktbl = nitems(chrtoblktbl);

#include <dev/cons.h>

#define dartcnpollc	nullcnpollc
cons_decl(dart);

struct	consdev constab[] = {
#if NDART > 0
	cons_init(dart),
#endif
	{ 0 },
};
