/*	$OpenBSD: src/sys/arch/vax/uba/ubareg.h,v 1.8 2000/04/27 03:14:51 bjc Exp $ */
/*	$NetBSD: ubareg.h,v 1.10 1998/10/18 18:51:30 ragge Exp $ */

/*-
 * Copyright (c) 1982, 1986 The Regents of the University of California.
 * All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)ubareg.h	7.8 (Berkeley) 5/9/91
 */

/*
 * VAX UNIBUS adapter registers
 */

/*
 * "UNIBUS" adaptor types.
 * This code is used for both UNIBUSes and Q-buses
 * with different types of adaptors.
 * Definition of a type includes support code for that type.
 */

#if VAX780 || VAX8600
#define DW780	1		/* has adaptor regs, sr: 780/785/8600 */
#else
#undef DW780
#endif


#if VAX750
#define DW750	2		/* has adaptor regs, no sr: 750, 730 */
#endif

#if VAX730
#define DW730	3		/* has adaptor regs, no sr: 750, 730 */
#endif

#if VAX630 || VAX650
#define QBA	4		/* 22-bit Q-bus, no adaptor regs: uVAX II */
#endif

/*
 * Size of unibus memory address space in pages
 * (also number of map registers).
 * QBAPAGES should be 8192, but we don't need nearly that much
 * address space, and the return from the allocation routine
 * can accommodate at most 2047 (ubavar.h: UBA_MAXMR);
 * QBAPAGES must be at least UBAPAGES.	Choose pragmatically.
 * 
 * Is there ever any need to have QBAPAGES != UBAPAGES???
 * Wont work now anyway, QBAPAGES _must_ be .eq. UBAPAGES.
 */
#define UBAPAGES	496
#define NUBMREG		496
#define	QBAPAGES	1024
#define UBAIOADDR	0760000		/* start of I/O page */
#define UBAIOPAGES	16

#ifndef _LOCORE
/*
 * DW780/DW750 hardware registers
 */
struct uba_regs {
	int	uba_cnfgr;		/* configuration register */
	int	uba_cr;			/* control register */
	int	uba_sr;			/* status register */
	int	uba_dcr;		/* diagnostic control register */
	int	uba_fmer;		/* failed map entry register */
	int	uba_fubar;		/* failed UNIBUS address register */
	int	pad1[2];
	int	uba_brsvr[4];
	int	uba_brrvr[4];		/* receive vector registers */
	int	uba_dpr[16];		/* buffered data path register */
	int	pad2[480];
	struct pte uba_map[UBAPAGES];	/* unibus map register */
	int	pad3[UBAIOPAGES];	/* no maps for device address space */
};
#endif

#ifdef DW780
/* uba_cnfgr */
#define UBACNFGR_UBINIT 0x00040000	/* unibus init asserted */
#define UBACNFGR_UBPDN	0x00020000	/* unibus power down */
#define UBACNFGR_UBIC	0x00010000	/* unibus init complete */

#define UBACNFGR_BITS \
"\40\40PARFLT\37WSQFLT\36URDFLT\35ISQFLT\34MXTFLT\33XMTFLT\30ADPDN\27ADPUP\23UBINIT\22UBPDN\21UBIC"

/* uba_cr */
#define UBACR_MRD16	0x40000000	/* map reg disable bit 4 */
#define UBACR_MRD8	0x20000000	/* map reg disable bit 3 */
#define UBACR_MRD4	0x10000000	/* map reg disable bit 2 */
#define UBACR_MRD2	0x08000000	/* map reg disable bit 1 */
#define UBACR_MRD1	0x04000000	/* map reg disable bit 0 */
#define UBACR_IFS	0x00000040	/* interrupt field switch */
#define UBACR_BRIE	0x00000020	/* BR interrupt enable */
#define UBACR_USEFIE	0x00000010	/* UNIBUS to SBI error field IE */
#define UBACR_SUEFIE	0x00000008	/* SBI to UNIBUS error field IE */
#define UBACR_CNFIE	0x00000004	/* configuration IE */
#define UBACR_UPF	0x00000002	/* UNIBUS power fail */
#define UBACR_ADINIT	0x00000001	/* adapter init */

/* uba_sr */
#define UBASR_BR7FULL	0x08000000	/* BR7 receive vector reg full */
#define UBASR_BR6FULL	0x04000000	/* BR6 receive vector reg full */
#define UBASR_BR5FULL	0x02000000	/* BR5 receive vector reg full */
#define UBASR_BR4FULL	0x01000000	/* BR4 receive vector reg full */
#define UBASR_RDTO	0x00000400	/* UNIBUS to SBI read data timeout */
#define UBASR_RDS	0x00000200	/* read data substitute */
#define UBASR_CRD	0x00000100	/* corrected read data */
#define UBASR_CXTER	0x00000080	/* command transmit error */
#define UBASR_CXTMO	0x00000040	/* command transmit timeout */
#define UBASR_DPPE	0x00000020	/* data path parity error */
#define UBASR_IVMR	0x00000010	/* invalid map register */
#define UBASR_MRPF	0x00000008	/* map register parity failure */
#define UBASR_LEB	0x00000004	/* lost error */
#define UBASR_UBSTO	0x00000002	/* UNIBUS select timeout */
#define UBASR_UBSSYNTO	0x00000001	/* UNIBUS slave sync timeout */

#define UBASR_BITS \
"\20\13RDTO\12RDS\11CRD\10CXTER\7CXTMO\6DPPE\5IVMR\4MRPF\3LEB\2UBSTO\1UBSSYNTO"

/* uba_brrvr[] */
#define UBABRRVR_AIRI	0x80000000	/* adapter interrupt request */
#define UBABRRVR_DIV	0x0000ffff	/* device interrupt vector field */
#endif
 
/* uba_dpr */
#ifdef DW780
#define UBADPR_BNE	0x80000000	/* buffer not empty - purge */
#define UBADPR_BTE	0x40000000	/* buffer transfer error */
#define UBADPR_DPF	0x20000000	/* DP function (RO) */
#define UBADPR_BS	0x007f0000	/* buffer state field */
#define UBADPR_BUBA	0x0000ffff	/* buffered UNIBUS address */
#endif
#ifdef DW750
#define UBADPR_ERROR	0x80000000	/* error occurred */
#define UBADPR_NXM	0x40000000	/* nxm from memory */
#define UBADPR_UCE	0x20000000	/* uncorrectable error */
#define UBADPR_PURGE	0x00000001	/* purge bdp */
#endif

/* uba_mr[] */
#define UBAMR_MRV	0x80000000	/* map register valid */
#define UBAMR_BO	0x02000000	/* byte offset bit */
#define UBAMR_DPDB	0x01e00000	/* data path designator field */
#define UBAMR_SBIPFN	0x001fffff	/* SBI page address field */

#define UBAMR_DPSHIFT	21		/* shift to data path designator */

/*
 * Number of unibus buffered data paths and possible uba's per cpu type.
 */
#define NBDP8600	15
#define NBDP780		15
#define NBDPBUA		5
#define NBDP750		3
#define NBDP730		0
#define MAXNBDP		15

/*
 * Symbolic BUS addresses for UBAs.
 */

#if VAX630 || VAX650
#define QBAMAP	0x20088000
#define QMEM	0x30000000
#define QIOPAGE	0x20000000
/*
 * Q-bus control registers
 */
#define QIPCR		0x1f40		/* from start of iopage */
/* bits in QIPCR */
#define Q_DBIRQ		0x0001		/* doorbell interrupt request */
#define Q_LMEAE		0x0020		/* local mem external access enable */
#define Q_DBIIE		0x0040		/* doorbell interrupt enable */
#define Q_AUXHLT	0x0100		/* auxiliary processor halt */
#define Q_DMAQPE	0x8000		/* Q22 bus address space parity error */
#endif

#if VAX730
#define UMEM730		0xfc0000
#endif

#if VAX750
#define UMEM750(i)	(0xfc0000-(i)*0x40000)
#endif

#if VAX780
#define UMEM780(i)	(0x20100000+(i)*0x40000)
#endif

#if VAX8200		/* BEWARE, argument is node, not ubanum */
#define UMEM8200(i)	(0x20400000+(i)*0x40000)
#endif

#if VAX8600 || VAX780
#define UMEMA8600(i)	(0x20100000+(i)*0x40000)
#define UMEMB8600(i)	(0x22100000+(i)*0x40000)
#endif

/*
 * Macro to offset a UNIBUS device address, often expressed as
 * something like 0172520, by forcing it into the last 8K
 * of UNIBUS memory space.
 */
#define ubdevreg(addr)	((addr) & 017777)
