/*	$OpenBSD: src/sys/arch/amiga/amiga/Attic/machdep.c,v 1.15 1996/08/19 00:04:15 niklas Exp $	*/
/*	$NetBSD: machdep.c,v 1.72.4.1 1996/05/26 16:23:23 is Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
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
 * from: Utah $Hdr: machdep.c 1.63 91/04/24$
 *
 *	@(#)machdep.c	7.16 (Berkeley) 6/3/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/user.h>
#include <sys/exec.h>            /* for PS_STRINGS */
#include <sys/vnode.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/core.h>
#include <sys/kcore.h>
#ifdef SYSVSHM
#include <sys/shm.h>
#endif
#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#include <net/netisr.h>
#define	MAXMEM	64*1024*CLSIZE	/* XXX - from cmap.h */
#include <vm/vm_param.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/kcore.h>
#include <dev/cons.h>
#include <amiga/amiga/isr.h>
#include <amiga/amiga/custom.h>
#ifdef DRACO
#include <amiga/amiga/drcustom.h>
#endif
#include <amiga/amiga/cia.h>
#include <amiga/amiga/cc.h>
#include <amiga/amiga/memlist.h>

#include "fd.h"
#include "ser.h"
#include "ether.h"
#include "ppp.h"

#include <net/netisr.h>
#include <net/if.h>

#ifdef INET
#include <netinet/in.h>
#ifdef NETHER
#include <netinet/if_ether.h>
#endif
#include <netinet/ip_var.h>
#endif 
#ifdef NS
#include <netns/ns_var.h>
#endif
#ifdef ISO
#include <netiso/iso.h>
#include <netiso/clnp.h>
#endif
#if NPPP > 0
#include <net/ppp_defs.h>
#include <net/if_ppp.h>
#endif


/* vm_map_t buffer_map; */
extern vm_offset_t avail_end;
extern vm_offset_t avail_start;

/* prototypes */
void identifycpu __P((void));
vm_offset_t reserve_dumppages __P((vm_offset_t));
void dumpsys __P((void));
void initcpu __P((void));
void straytrap __P((int, u_short));
static void netintr __P((void));
static void call_sicallbacks __P((void));
void intrhand __P((int));
static void dumpmem __P((int *, int, int));
static char *hexstr __P((int, int));
#if NSER > 0
void ser_outintr __P((void));
#endif
#if NFD > 0
void fdintr __P((int));
#endif

/*
 * Declare these as initialized data so we can patch them.
 */
int	nswbuf = 0;
#ifdef	NBUF
int	nbuf = NBUF;
#else
int	nbuf = 0;
#endif
#ifdef	BUFPAGES
int	bufpages = BUFPAGES;
#else
int	bufpages = 0;
#endif
int	msgbufmapped;		/* set when safe to use msgbuf */
int	maxmem;			/* max memory per process */
int	physmem = MAXMEM;	/* max supported memory, changes to actual */
/*
 * extender "register" for software interrupts. Moved here 
 * from locore.s, since softints are no longer dealt with
 * in locore.s.
 */
unsigned char ssir;
/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;
extern  int   freebufspace;
extern	u_int lowram;

#ifdef COMPAT_SUNOS
extern struct emul emul_sunos;
#endif

/* used in init_main.c */
char *cpu_type = "m68k";
/* the following is used externally (sysctl_hw) */
char machine[] = "amiga";
 
struct isr *isr_ports;
#ifdef DRACO
struct isr *isr_slot3;
#endif
#if defined(IPL_REMAP_1) || defined(IPL_REMAP_2)
struct isr *isr_exter[7];
#else
struct isr *isr_exter;
#endif

#ifdef IPL_REMAP_1
int isr_exter_lowipl = 7;
int isr_exter_highipl = 0;
int isr_exter_ipl = 0;

/*
 * Poll all registered ISRs starting at IPL start_ipl going down to
 * isr_exter_lowipl.  If we reach the IPL of ending_psw along the way
 * just return stating in isr_exter_ipl that we need to run the remaining
 * layers later when the IPL gets lowered (i.e. a spl? call).  If some
 * ISR handles the interrupt, or all layers have been processed, enable
 * EXTER interrupts again and return.
 */
void
walk_ipls (start_ipl, ending_psw)
	int start_ipl;
	int ending_psw;
{
	int i;

	for (i = start_ipl; i >= isr_exter_lowipl; i--) {
		register int psw = i << 8 | PSL_S;
		struct isr *isr;
		      
		if (psw <= ending_psw) {
			isr_exter_ipl = i;
			return;
		}
		__asm __volatile("movew %0,sr" : : "d" (psw) : "cc");
		for (isr = isr_exter[i]; isr; isr = isr->isr_forw)
			(*isr->isr_intr)(isr->isr_arg);
	}
	isr_exter_ipl = 0;
	__asm __volatile("movew %0,sr" : : "di" (PSL_S|PSL_IPL6) : "cc");
	custom.intreq = INTF_EXTER;
	custom.intena = INTF_SETCLR | INTF_EXTER;
}
#endif

#ifdef IPL_REMAP_2
/*
 * Service all waiting ISRs starting from current IPL to npsl.
 */
void
walk_ipls (npsl)
	int npsl;
{
        struct isr *isr_head;
	register struct isr *isr;
	int opsl;
	register int psl;

	psl = opsl = spl7();
	isr_head = &isr_exter[(psl & PSL_IPL) >> 8];
redo_ipl:
	while (psl > to_psl) {
		for (isr = isr_head; isr; isr = isr->isr_forw) {
			if (isr->isr_status == ISR_WAIT) {
				splx(psl);
				isr->isr_status = ISR_BUSY;
				(*isr->isr_intr)(isr->isr_arg);
				isr->isr_status = ISR_IDLE;
				spl7();
				goto redo_ipl;
			}
		}
		psl -= PSL_IPL1;
		isr_head--;
	}
	return opsl;
}
#endif

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit()
{
	/* initialize custom chip interface */
#ifdef DRACO
	if (is_draco()) {
		/* XXX to be done */
	} else
#endif
		custom_chips_init();
	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

#if defined (DDB)
        ddb_init();
        if (boothowto & RB_KDB)
                Debugger();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup()
{
	register unsigned i;
	register caddr_t v, firstaddr;
	int base, residual;
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;
#endif
	vm_offset_t minaddr, maxaddr;
	vm_size_t size = 0;
#if defined(MACHINE_NONCONTIG) && defined(DEBUG)
	extern struct {
		vm_offset_t start;
		vm_offset_t end;
		int first_page;
	} phys_segs[16];
#endif

	/*
	 * Initialize error message buffer (at end of core).
	 */
#ifdef DEBUG
	pmapdebug = 0;
#endif
	/* avail_end was pre-decremented in pmap_bootstrap to compensate */
	for (i = 0; i < btoc(sizeof (struct msgbuf)); i++)
		pmap_enter(pmap_kernel(), (vm_offset_t)msgbufp, 
		    avail_end + i * NBPG, VM_PROT_ALL, TRUE);
	msgbufmapped = 1;

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	identifycpu();
	printf("real  mem = %d (%d pages)\n", ctob(physmem), ctob(physmem)/NBPG);

	/*
	 * Allocate space for system data structures.
	 * The first available real memory address is in "firstaddr".
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 * As pages of memory are allocated and cleared,
	 * "firstaddr" is incremented.
	 * An index into the kernel page table corresponding to the
	 * virtual memory address maintained in "v" is kept in "mapaddr".
	 */
	/*
	 * Make two passes.  The first pass calculates how much memory is
	 * needed and allocates it.  The second pass assigns virtual
	 * addresses to the various data structures.
	 */
	firstaddr = 0;
again:
	v = (caddr_t)firstaddr;

#define	valloc(name, type, num) \
	    (name) = (type *)v; v = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)v; v = (caddr_t)((lim) = ((name)+(num)))
/*	valloc(cfree, struct cblock, nclist); */
	valloc(callout, struct callout, ncallout);
	valloc(swapmap, struct map, nswapmap = maxproc * 2);
#ifdef SYSVSHM
	valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif
#ifdef SYSVSEM
	valloc(sema, struct semid_ds, seminfo.semmni);
	valloc(sem, struct sem, seminfo.semmns);
	/* This is pretty disgusting! */
	valloc(semu, int, (seminfo.semmnu * seminfo.semusz) / sizeof(int));
#endif
#ifdef SYSVMSG
	valloc(msgpool, char, msginfo.msgmax);
	valloc(msgmaps, struct msgmap, msginfo.msgseg);
	valloc(msghdrs, struct msg, msginfo.msgtql);
	valloc(msqids, struct msqid_ds, msginfo.msgmni);
#endif
	/*
	 * Determine how many buffers to allocate. We allocate
	 * the BSD standard of use 10% of memory for the first 2 Meg,
	 * 5% of remaining. Insure a minimum of 16 buffers.
	 * We allocate 3/4 as many swap buffer headers as file i/o buffers.
	 */
  	if (bufpages == 0)
		if (physmem < btoc(2 * 1024 * 1024))
			bufpages = physmem / (10 * CLSIZE);
		else
			bufpages = (btoc(2 * 1024 * 1024) + physmem) /
			    (20 * CLSIZE);

	if (nbuf == 0) {
		nbuf = bufpages;
		if (nbuf < 16)
			nbuf = 16;
	}

	if (nswbuf == 0) {
		nswbuf = (nbuf * 3 / 4) &~ 1;	/* force even */
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
	}
	valloc(swbuf, struct buf, nswbuf);
	valloc(buf, struct buf, nbuf);
	/*
	 * End of first pass, size has been calculated so allocate memory
	 */
	if (firstaddr == 0) {
		size = (vm_size_t)(v - firstaddr);
		firstaddr = (caddr_t) kmem_alloc(kernel_map, round_page(size));
		if (firstaddr == 0)
			panic("startup: no room for tables");
		goto again;
	}
	/*
	 * End of second pass, addresses have been assigned
	 */
	if ((vm_size_t)(v - firstaddr) != size)
		panic("startup: table size inconsistency");

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	buffer_map = kmem_suballoc(kernel_map, (vm_offset_t *)&buffers,
				   &maxaddr, size, TRUE);
	minaddr = (vm_offset_t)buffers;
	if (vm_map_find(buffer_map, vm_object_allocate(size), (vm_offset_t)0,
			&minaddr, size, FALSE) != KERN_SUCCESS)
		panic("startup: cannot allocate buffers");
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* don't want to alloc more physical mem than needed */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vm_size_t curbufsize;
		vm_offset_t curbuf;

		/*
		 * First <residual> buffers get (base+1) physical pages
		 * allocated for them.  The rest get (base) physical pages.
		 *
		 * The rest of each buffer occupies virtual space,
		 * but has no physical memory allocated for it.
		 */
		curbuf = (vm_offset_t)buffers + i * MAXBSIZE;
		curbufsize = CLBYTES * (i < residual ? base+1 : base);
		vm_map_pageable(buffer_map, curbuf, curbuf+curbufsize, FALSE);
		vm_map_simplify(buffer_map, curbuf);
	}

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, TRUE);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, TRUE);

	/*
	 * Finally, allocate mbuf pool.  Since mclrefcnt is an off-size
	 * we use the more space efficient malloc in place of kmem_alloc.
	 */
	mclrefcnt = (char *)malloc(NMBCLUSTERS+CLBYTES/MCLBYTES,
				   M_MBUF, M_NOWAIT);
	bzero(mclrefcnt, NMBCLUSTERS+CLBYTES/MCLBYTES);
	mb_map = kmem_suballoc(kernel_map, (vm_offset_t *)&mbutl, &maxaddr,
			       VM_MBUF_SIZE, FALSE);

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	printf("avail mem = %ld (%ld pages)\n", ptoa(cnt.v_free_count),
	    ptoa(cnt.v_free_count)/NBPG);
	printf("using %d buffers containing %d bytes of memory\n",
		nbuf, bufpages * CLBYTES);
	
	/*
	 * display memory configuration passed from loadbsd
	 */
	if (memlist->m_nseg > 0 && memlist->m_nseg < 16)
		for (i = 0; i < memlist->m_nseg; i++)
			printf("memory segment %d at %x size %x\n", i,
			    memlist->m_seg[i].ms_start, 
			    memlist->m_seg[i].ms_size);
#if defined(MACHINE_NONCONTIG) && defined(DEBUG)
	printf ("Physical memory segments:\n");
	for (i = 0; i < memlist->m_nseg && phys_segs[i].start; ++i)
		printf ("Physical segment %d at %08lx size %ld offset %d\n", i,
		    phys_segs[i].start,
		    (phys_segs[i].end - phys_segs[i].start) / NBPG,
		    phys_segs[i].first_page);
#endif

#ifdef DEBUG_KERNEL_START
	printf("calling initcpu...\n");
#endif
	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

#ifdef DEBUG_KERNEL_START
	printf("survived initcpu...\n");
#endif
	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

#ifdef DEBUG_KERNEL_START
	printf("survived bufinit...\n");
#endif
	/*
	 * Configure the system.
	 */
	if (boothowto & RB_CONFIG) {
#ifdef BOOT_CONFIG
		user_config();
#else
		printf("kernel does not support -c; continuing..\n");
#endif
	}
	configure();
#ifdef DEBUG_KERNEL_START
	printf("survived configure...\n");
#endif
}

/*
 * Set registers on exec.
 * XXX Should clear registers except sp, pc,
 * but would break init; should be fixed soon.
 */
void
setregs(p, pack, stack, retval)
	register struct proc *p;
	struct exec_package *pack;
	u_long stack;
	register_t *retval;
{
	struct frame *frame = (struct frame *)p->p_md.md_regs;
	
	frame->f_pc = pack->ep_entry & ~1;
	frame->f_regs[SP] = stack;
	frame->f_regs[A2] = (int)PS_STRINGS;

#ifdef FPCOPROC
	/* restore a null state frame */
	p->p_addr->u_pcb.pcb_fpregs.fpf_null = 0;
	m68881_restore(&p->p_addr->u_pcb.pcb_fpregs);
#endif
#ifdef COMPAT_SUNOS
	/*
	 * SunOS' ld.so does self-modifying code without knowing
	 * about the 040's cache purging needs.  So we need to uncache
	 * writeable executable pages.
	 */
	if (p->p_emul == &emul_sunos)
		p->p_md.md_flags |= MDP_UNCACHE_WX;
	else
		p->p_md.md_flags &= ~MDP_UNCACHE_WX;
#endif
}

/*
 * Info for CTL_HW
 */
char cpu_model[120];
extern char version[];
 
void
identifycpu()
{
        /* there's alot of XXX in here... */
	char *mach, *mmu, *fpu;

#ifdef M68060
	char cpubuf[16];
	u_int32_t pcr;
#endif

#ifdef DRACO
	char machbuf[16];

	if (is_draco()) {
		sprintf(machbuf, "DraCo rev.%d", is_draco());
		mach = machbuf;
	} else 
#endif
	if (is_a4000())
		mach = "Amiga 4000";
	else if (is_a3000())
		mach = "Amiga 3000";
	else if (is_a1200())
		mach = "Amiga 1200";
	else
		mach = "Amiga 500/2000";

	fpu = NULL;
#ifdef M68060
	if (machineid & AMIGA_68060) {
		asm(".word 0x4e7a,0x0808; movl d0,%0" : "=d"(pcr) : : "d0");
		sprintf(cpubuf, "68%s060 rev.%d",
		    pcr & 0x10000 ? "LC/EC" : "", (pcr>>8)&0xff);
		cpu_type = cpubuf;
		mmu = "/MMU";
		fpu = "/FPU";
		fputype = FPU_68040; /* XXX */
	} else 
#endif
	if (machineid & AMIGA_68040) {
		cpu_type = "m68040";
		mmu = "/MMU";
		fpu = "/FPU";
		fputype = FPU_68040;
	} else if (machineid & AMIGA_68030) {
		cpu_type = "m68030";	/* XXX */
		mmu = "/MMU";
	} else {
		cpu_type = "m68020";
		mmu = " m68851 MMU";
	}
	if (fpu == NULL) {
		if (machineid & AMIGA_68882) {
			fpu = " m68882 FPU";
			fputype = FPU_68882;
		} else if (machineid & AMIGA_68881) {
			fpu = " m68881 FPU";
			fputype = FPU_68881;
		} else {
			fpu = " no FPU";
			fputype = FPU_NONE;
		}
	}
	sprintf(cpu_model, "%s (%s CPU%s%s)", mach, cpu_type, mmu, fpu);
	printf("%s\n", cpu_model);
}

/*
 * machine dependent system variables.
 */
int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	dev_t consdev;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return(ENOTDIR);               /* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return(sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
		    sizeof(consdev)));
	default:
		return(EOPNOTSUPP);
	}
	/* NOTREACHED */
}

#define SS_RTEFRAME	1
#define SS_FPSTATE	2
#define SS_USERREGS	4

struct sigstate {
	int	ss_flags;		/* which of the following are valid */
	struct	frame ss_frame;		/* original exception frame */
	struct	fpframe ss_fpstate;	/* 68881/68882 state info */
};

/*
 * WARNING: code in locore.s assumes the layout shown for sf_signum
 * thru sf_handler so... don't screw with them!
 */
struct sigframe {
	int	sf_signum;		/* signo for handler */
	int	sf_code;		/* additional info for handler */
	struct	sigcontext *sf_scp;	/* context ptr for handler */
	sig_t	sf_handler;		/* handler addr for u_sigc */
	struct	sigstate sf_state;	/* state of the hardware */
	struct	sigcontext sf_sc;	/* actual context */
};

#ifdef DEBUG
int sigdebug = 0x0;
int sigpid = 0;
#define SDB_FOLLOW	0x01
#define SDB_KSTACK	0x02
#define SDB_FPSTATE	0x04
#endif

/*
 * Send an interrupt to process.
 */
void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig, mask;
	u_long code;
{
	register struct proc *p = curproc;
	register struct sigframe *fp, *kfp;
	register struct frame *frame;
	register struct sigacts *psp = p->p_sigacts;
	register short ft;
	int oonstack;
	extern short exframesize[];
	extern char sigcode[], esigcode[];


/*printf("sendsig %d %d %x %x %x\n", p->p_pid, sig, mask, code, catcher);*/

	frame = (struct frame *)p->p_md.md_regs;
	ft = frame->f_format;
	oonstack = psp->ps_sigstk.ss_flags & SS_ONSTACK;

	/*
	 * Allocate and validate space for the signal handler
	 * context. Note that if the stack is in P0 space, the
	 * call to grow() is a nop, and the useracc() check
	 * will fail if the process has not already allocated
	 * the space with a `brk'.
	 */
	if ((psp->ps_flags & SAS_ALTSTACK) && oonstack == 0 &&
	    (psp->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)(psp->ps_sigstk.ss_sp +
		    psp->ps_sigstk.ss_size - sizeof(struct sigframe));
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;
	} else
		fp = (struct sigframe *)frame->f_regs[SP] - 1;
	if ((unsigned)fp <= USRSTACK - ctob(p->p_vmspace->vm_ssize)) 
		(void)grow(p, (unsigned)fp);
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig(%d): sig %d ssp %p usp %p scp %p ft %d\n",
		       p->p_pid, sig, &oonstack, fp, &fp->sf_sc, ft);
#endif
	if (useracc((caddr_t)fp, sizeof(struct sigframe), B_WRITE) == 0) {
#ifdef DEBUG
		if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
			printf("sendsig(%d): useracc failed on sig %d\n",
			       p->p_pid, sig);
#endif
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		SIGACTION(p, SIGILL) = SIG_DFL;
		sig = sigmask(SIGILL);
		p->p_sigignore &= ~sig;
		p->p_sigcatch &= ~sig;
		p->p_sigmask &= ~sig;
		psignal(p, SIGILL);
		return;
	}
	kfp = malloc(sizeof(struct sigframe), M_TEMP, M_WAITOK);
	/* 
	 * Build the argument list for the signal handler.
	 */
	kfp->sf_signum = sig;
	kfp->sf_code = code;
	kfp->sf_scp = &fp->sf_sc;
	kfp->sf_handler = catcher;
	/*
	 * Save necessary hardware state.  Currently this includes:
	 *	- general registers
	 *	- original exception frame (if not a "normal" frame)
	 *	- FP coprocessor state
	 */
	kfp->sf_state.ss_flags = SS_USERREGS;
	bcopy((caddr_t)frame->f_regs,
	      (caddr_t)kfp->sf_state.ss_frame.f_regs, sizeof frame->f_regs);
	if (ft >= FMT9) {
#ifdef DEBUG
		if (ft != FMT9 && ft != FMTA && ft != FMTB)
			panic("sendsig: bogus frame type");
#endif
		kfp->sf_state.ss_flags |= SS_RTEFRAME;
		kfp->sf_state.ss_frame.f_format = frame->f_format;
		kfp->sf_state.ss_frame.f_vector = frame->f_vector;
		bcopy((caddr_t)&frame->F_u,
		      (caddr_t)&kfp->sf_state.ss_frame.F_u, exframesize[ft]);
		/*
		 * Leave an indicator that we need to clean up the kernel
		 * stack.  We do this by setting the "pad word" above the
		 * hardware stack frame to the amount the stack must be
		 * adjusted by.
		 *
		 * N.B. we increment rather than just set f_stackadj in
		 * case we are called from syscall when processing a
		 * sigreturn.  In that case, f_stackadj may be non-zero.
		 */
		frame->f_stackadj += exframesize[ft];
		frame->f_format = frame->f_vector = 0;
#ifdef DEBUG
		if (sigdebug & SDB_FOLLOW)
			printf("sendsig(%d): copy out %d of frame %d\n",
			       p->p_pid, exframesize[ft], ft);
#endif
	}
#ifdef FPCOPROC
	kfp->sf_state.ss_flags |= SS_FPSTATE;
	m68881_save(&kfp->sf_state.ss_fpstate);
#ifdef DEBUG
	if ((sigdebug & SDB_FPSTATE) && *(char *)&kfp->sf_state.ss_fpstate)
		printf("sendsig(%d): copy out FP state (%x) to %p\n",
		       p->p_pid, *(u_int *)&kfp->sf_state.ss_fpstate,
		       &kfp->sf_state.ss_fpstate);
#endif
#endif
	/*
	 * Build the signal context to be used by sigreturn.
	 */
	kfp->sf_sc.sc_onstack = oonstack;
	kfp->sf_sc.sc_mask = mask;
	kfp->sf_sc.sc_sp = frame->f_regs[SP];
	kfp->sf_sc.sc_fp = frame->f_regs[A6];
	kfp->sf_sc.sc_ap = (int)&fp->sf_state;
	kfp->sf_sc.sc_pc = frame->f_pc;
	kfp->sf_sc.sc_ps = frame->f_sr;
	(void) copyout((caddr_t)kfp, (caddr_t)fp, sizeof(struct sigframe));
	frame->f_regs[SP] = (int)fp;
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sendsig(%d): sig %d scp %p fp %p sc_sp %x sc_ap %x\n",
		       p->p_pid, sig, kfp->sf_scp, fp,
		       kfp->sf_sc.sc_sp, kfp->sf_sc.sc_ap);
#endif
	/*
	 * Signal trampoline code is at base of user stack.
	 */
	frame->f_pc = (int)(((char *)PS_STRINGS) - (esigcode - sigcode));
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sendsig(%d): sig %d returns\n",
		       p->p_pid, sig);
#endif
	free((caddr_t)kfp, M_TEMP);
}


/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper priviledges or to cause
 * a machine fault.
 */
/* ARGSUSED */
int
sys_sigreturn(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_sigreturn_args /* {
		syscallarg(struct sigcontext *) sigcntxp;
	} */ *uap = v;
	struct sigcontext *scp, context;
	struct frame *frame;
	int rf, flags;
	struct sigstate tstate;
	extern short exframesize[];

	scp = SCARG(uap, sigcntxp);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn: pid %d, scp %p\n", p->p_pid, scp);
#endif
	if ((int)scp & 1)
		return(EINVAL);
	/*
	 * Test and fetch the context structure.
	 * We grab it all at once for speed.
	 */
	if (useracc((caddr_t)scp, sizeof(*scp), B_WRITE) == 0 ||
	    copyin(scp, &context, sizeof(context)))
		return(EINVAL);
	scp = &context;
	if ((scp->sc_ps & (PSL_MBZ|PSL_IPL|PSL_S)) != 0)
		return(EINVAL);
	/*
	 * Restore the user supplied information
	 */
	if (scp->sc_onstack & 1)
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else 
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;
	p->p_sigmask = scp->sc_mask &~ sigcantmask;
	frame = (struct frame *) p->p_md.md_regs;
	frame->f_regs[SP] = scp->sc_sp;
	frame->f_regs[A6] = scp->sc_fp;
	frame->f_pc = scp->sc_pc;
	frame->f_sr = scp->sc_ps;
	/*
	 * Grab pointer to hardware state information.
	 * If zero, the user is probably doing a longjmp.
	 */
	if ((rf = scp->sc_ap) == 0)
		return (EJUSTRETURN);
	/*
	 * See if there is anything to do before we go to the
	 * expense of copying in close to 1/2K of data
	 */
	flags = fuword((caddr_t)rf);
#ifdef DEBUG
	if (sigdebug & SDB_FOLLOW)
		printf("sigreturn(%d): sc_ap %x flags %x\n",
		       p->p_pid, rf, flags);
#endif
	/*
	 * fuword failed (bogus sc_ap value).
	 */
	if (flags == -1)
		return (EINVAL);
	if (flags == 0 || copyin((caddr_t)rf, (caddr_t)&tstate, sizeof tstate))
		return (EJUSTRETURN);
#ifdef DEBUG
	if ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid)
		printf("sigreturn(%d): ssp %p usp %x scp %p ft %d\n",
		       p->p_pid, &flags, scp->sc_sp, SCARG(uap, sigcntxp),
		       (flags&SS_RTEFRAME) ? tstate.ss_frame.f_format : -1);
#endif
	/*
	 * Restore most of the users registers except for A6 and SP
	 * which were handled above.
	 */
	if (flags & SS_USERREGS)
		bcopy((caddr_t)tstate.ss_frame.f_regs,
		      (caddr_t)frame->f_regs, sizeof(frame->f_regs)-2*NBPW);
	/*
	 * Restore long stack frames.  Note that we do not copy
	 * back the saved SR or PC, they were picked up above from
	 * the sigcontext structure.
	 */
	if (flags & SS_RTEFRAME) {
		register int sz;
		
		/* grab frame type and validate */
		sz = tstate.ss_frame.f_format;
		if (sz > 15 || (sz = exframesize[sz]) < 0)
			return (EINVAL);
		frame->f_stackadj -= sz;
		frame->f_format = tstate.ss_frame.f_format;
		frame->f_vector = tstate.ss_frame.f_vector;
		bcopy((caddr_t)&tstate.ss_frame.F_u, (caddr_t)&frame->F_u, sz);
#ifdef DEBUG
		if (sigdebug & SDB_FOLLOW)
			printf("sigreturn(%d): copy in %d of frame type %d\n",
			       p->p_pid, sz, tstate.ss_frame.f_format);
#endif
	}
#ifdef FPCOPROC
	/*
	 * Finally we restore the original FP context
	 */
	if (flags & SS_FPSTATE)
		m68881_restore(&tstate.ss_fpstate);
#ifdef DEBUG
	if ((sigdebug & SDB_FPSTATE) && *(char *)&tstate.ss_fpstate)
		printf("sigreturn(%d): copied in FP state (%x) at %p\n",
		       p->p_pid, *(u_int *)&tstate.ss_fpstate,
		       &tstate.ss_fpstate);
#endif
#endif
#ifdef DEBUG
	if ((sigdebug & SDB_FOLLOW) ||
	    ((sigdebug & SDB_KSTACK) && p->p_pid == sigpid))
		printf("sigreturn(%d): returns\n", p->p_pid);
#endif
	return (EJUSTRETURN);
}

static int waittime = -1;

void
bootsync(void)
{
	if (waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}
}

void
boot(howto)
	register int howto;
{
	/* take a snap shot before clobbering any registers */
	if (curproc)
		savectx(&curproc->p_addr->u_pcb);

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0)
		bootsync();

	/* Disable interrupts. */
	spl7();

	/* If rebooting and a dump is requested do it. */
	if (howto & RB_DUMP)
		dumpsys();

	if (howto & RB_HALT) {
		printf("System halted.\n\n");
		asm("	stop	#0x2700");
		/*NOTREACHED*/
	}

	doboot();
	/*NOTREACHED*/
}


unsigned	dumpmag = 0x8fca0101;	/* magic number for savecore */
int	dumpsize = 0;		/* also for savecore */
long	dumplo = 0;
cpu_kcore_hdr_t cpu_kcore_hdr;

void
dumpconf()
{
	int nblks;
	int i;
	extern u_int Sysseg_pa;

	/* XXX new corefile format, single segment + chipmem */
	dumpsize = physmem;
	cpu_kcore_hdr.ram_segs[0].start = lowram;
	cpu_kcore_hdr.ram_segs[0].size = ctob(physmem);
	for (i = 0; i < memlist->m_nseg; i++) {
		if ((memlist->m_seg[i].ms_attrib & MEMF_CHIP) == 0)
			continue;
		dumpsize += btoc(memlist->m_seg[i].ms_size);
		cpu_kcore_hdr.ram_segs[1].start = 0;
		cpu_kcore_hdr.ram_segs[1].size = memlist->m_seg[i].ms_size;
		break;
	}
	cpu_kcore_hdr.mmutype = mmutype;
	cpu_kcore_hdr.kernel_pa = lowram;
	cpu_kcore_hdr.sysseg_pa = (st_entry_t *)Sysseg_pa;
	if (dumpdev != NODEV && bdevsw[major(dumpdev)].d_psize) {
		nblks = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
		if (dumpsize > btoc(dbtob(nblks - dumplo)))
			dumpsize = btoc(dbtob(nblks - dumplo));
		else if (dumplo == 0)
			dumplo = nblks - btodb(ctob(dumpsize));
	}
	--dumplo;	/* XXX assume header fits in one block */
	/*
	 * Don't dump on the first CLBYTES (why CLBYTES?)
	 * in case the dump device includes a disk label.
	 */
	if (dumplo < btodb(CLBYTES))
		dumplo = btodb(CLBYTES);
}

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
#define BYTES_PER_DUMP MAXPHYS	/* Must be a multiple of pagesize XXX small */
static vm_offset_t dumpspace;

vm_offset_t
reserve_dumppages(p)
	vm_offset_t p;
{
	dumpspace = p;
	return (p + BYTES_PER_DUMP);
}

void
dumpsys()
{
	unsigned bytes, i, n, seg;
	int     maddr, psize;
	daddr_t blkno;
	int     (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	int     error = 0;
	kcore_seg_t *kseg_p;
	cpu_kcore_hdr_t *chdr_p;
	char	dump_hdr[dbtob(1)];	/* XXX assume hdr fits in 1 block */

	msgbufmapped = 0;
	if (dumpdev == NODEV)
		return;
	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0)
		dumpconf();
	if (dumplo < 0)
		return;
	printf("\ndumping to dev %x, offset %ld\n", dumpdev, dumplo);

	psize = (*bdevsw[major(dumpdev)].d_psize) (dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable.\n");
		return;
	}
	kseg_p = (kcore_seg_t *)dump_hdr;
	chdr_p = (cpu_kcore_hdr_t *)&dump_hdr[ALIGN(sizeof(*kseg_p))];
	bzero(dump_hdr, sizeof(dump_hdr));

	/*
	 * Generate a segment header
	 */
	CORE_SETMAGIC(*kseg_p, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg_p->c_size = dbtob(1) - ALIGN(sizeof(*kseg_p));

	/*
	 * Add the md header
	 */

	*chdr_p = cpu_kcore_hdr;

	bytes = ctob(dumpsize);
	maddr = cpu_kcore_hdr.ram_segs[0].start;
	seg = 0;
	blkno = dumplo;
	dump = bdevsw[major(dumpdev)].d_dump;
	error = (*dump) (dumpdev, blkno++, (caddr_t)dump_hdr, dbtob(1));
	for (i = 0; i < bytes && error == 0; i += n) {
		/* Print out how many MBs we have to go. */
		n = bytes - i;
		if (n && (n % (1024 * 1024)) == 0)
			printf("%d ", n / (1024 * 1024));

		/* Limit size for next transfer. */
		if (n > BYTES_PER_DUMP)
			n = BYTES_PER_DUMP;

		if (maddr == 0) {	/* XXX kvtop chokes on this */
			maddr += NBPG;
			n -= NBPG;
			i += NBPG;
			++blkno;	/* XXX skip physical page 0 */
		}
		(void) pmap_map(dumpspace, maddr, maddr + n, VM_PROT_READ);
		error = (*dump) (dumpdev, blkno, (caddr_t) dumpspace, n);
		if (error)
			break;
		maddr += n;
		blkno += btodb(n);	/* XXX? */
		if (maddr >= (cpu_kcore_hdr.ram_segs[seg].start +
		    cpu_kcore_hdr.ram_segs[seg].size)) {
			++seg;
			maddr = cpu_kcore_hdr.ram_segs[seg].start;
			if (cpu_kcore_hdr.ram_segs[seg].size == 0)
				break;
		}
	}

	switch (error) {

	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	default:
		printf("succeeded\n");
		break;
	}
	printf("\n\n");
	delay(5000000);		/* 5 seconds */
}

/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  We do this by returning the current time
 * plus the amount of time since the last clock interrupt (clock.c:clkread).
 *
 * Check that this time is no less than any previously-reported time,
 * which could happen around the time of a clock adjustment.  Just for fun,
 * we guarantee that the time will be greater than the value obtained by a
 * previous call.
 */
void
microtime(tvp)
	register struct timeval *tvp;
{
	int s = spl7();
	static struct timeval lasttime;

	*tvp = time;
	tvp->tv_usec += clkread();
	while (tvp->tv_usec > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}

#if defined(M68060)
int m68060_pcr_init = 0x21;	/* make this patchable */
#endif

void
initcpu()
{
	/* XXX should init '40 vecs here, too */
#if defined(M68060) || defined(DRACO)
	extern caddr_t vectab[256];
#endif

#ifdef M68060
#if defined(M060SP)
	extern u_int8_t I_CALL_TOP[];
	extern u_int8_t FP_CALL_TOP[];
#else
	extern u_int8_t illinst;
#endif
	extern u_int8_t fpfault;
#endif

#ifdef DRACO
	extern u_int8_t DraCoIntr, DraCoLev2intr;
#endif

#ifdef M68060
	if (machineid & AMIGA_68060) {
		asm volatile ("movl %0,d0; .word 0x4e7b,0x0808" : : 
			"d"(m68060_pcr_init):"d0" );
#if defined(M060SP)

		/* integer support */
		vectab[61] = &I_CALL_TOP[128 + 0x00];

		/* floating point support */
		vectab[11] = &FP_CALL_TOP[128 + 0x30];
		vectab[55] = &FP_CALL_TOP[128 + 0x38];
		vectab[60] = &FP_CALL_TOP[128 + 0x40];

		vectab[54] = &FP_CALL_TOP[128 + 0x00];
		vectab[52] = &FP_CALL_TOP[128 + 0x08];
		vectab[53] = &FP_CALL_TOP[128 + 0x10];
		vectab[51] = &FP_CALL_TOP[128 + 0x18];
		vectab[50] = &FP_CALL_TOP[128 + 0x20];
		vectab[49] = &FP_CALL_TOP[128 + 0x28];

#else
		vectab[61] = &illinst;
#endif
		vectab[48] = &fpfault;
	}
#endif

#ifdef DRACO
	if (is_draco()) {
		vectab[24+1] = &DraCoIntr;
		vectab[24+2] = &DraCoLev2intr;
		vectab[24+3] = &DraCoIntr;
		vectab[24+4] = &DraCoIntr;
		vectab[24+5] = &DraCoIntr;
		vectab[24+6] = &DraCoIntr;
	}
#endif
	DCIS();
}

void
straytrap(pc, evec)
	int pc;
	u_short evec;
{
	printf("unexpected trap format %x (vector offset %x) from %x\n",
	       evec>>12, evec & 0xFFF, pc);
/*XXX*/	panic("straytrap");
}

int	*nofault;

int
badaddr(addr)
	register caddr_t addr;
{
	register int i;
	label_t	faultbuf;

#ifdef lint
	i = *addr; if (i) return(0);
#endif
	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}
	i = *(volatile short *)addr;
	nofault = (int *) 0;
	return(0);
}

int
badbaddr(addr)
	register caddr_t addr;
{
	register int i;
	label_t	faultbuf;

#ifdef lint
	i = *addr; if (i) return(0);
#endif
	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}
	i = *(volatile char *)addr;
	nofault = (int *) 0;
	return(0);
}

static void
netintr()
{
#ifdef INET
#if NETHER > 0
	if (netisr & (1 << NETISR_ARP)) {
		netisr &= ~(1 << NETISR_ARP);
		arpintr();
	}
#endif
	if (netisr & (1 << NETISR_IP)) {
		netisr &= ~(1 << NETISR_IP);
		ipintr();
	}
#endif
#ifdef NS
	if (netisr & (1 << NETISR_NS)) {
		netisr &= ~(1 << NETISR_NS);
		nsintr();
	}
#endif
#ifdef ISO
	if (netisr & (1 << NETISR_ISO)) {
		netisr &= ~(1 << NETISR_ISO);
		clnlintr();
	}
#endif
#if NPPP > 0
	if (netisr & (1 << NETISR_PPP)) {
		netisr &= ~(1 << NETISR_PPP);
		pppintr();
	}
#endif
}


/*
 * this is a handy package to have asynchronously executed
 * function calls executed at very low interrupt priority.
 * Example for use is keyboard repeat, where the repeat 
 * handler running at splclock() triggers such a (hardware
 * aided) software interrupt.  These functions are called in
 * a FIFO manner as expected.
 */

struct si_callback {
	struct si_callback *next;
	void (*function) __P((void *rock1, void *rock2));
	void *rock1, *rock2;
};
static struct si_callback *si_callbacks;
static struct si_callback *si_callbacks_end;
static struct si_callback *si_free;
#ifdef DIAGNOSTIC
static int ncb;		/* number of callback blocks allocated */
static int ncbd;	/* number of callback blocks dynamically allocated */
#endif

void
alloc_sicallback()
{
	struct si_callback *si;
	int s;

	si = (struct si_callback *)malloc(sizeof(*si), M_TEMP, M_NOWAIT);
	if (si == NULL)
		return;
	s = splhigh();
	si->next = si_free;
	si_free = si;
	splx(s);
#ifdef DIAGNOSTIC
	++ncb;
#endif
}

void
add_sicallback (function, rock1, rock2)
	void (*function) __P((void *rock1, void *rock2));
	void *rock1, *rock2;
{
	struct si_callback *si;
	int s;

	/*
	 * this function may be called from high-priority interrupt handlers.
	 * We may NOT block for  memory-allocation in here!.
	 */
	s = splhigh();
	si = si_free;
	if (si != NULL)
		si_free = si->next;
	splx(s);

	if (si == NULL) {
		si = (struct si_callback *)malloc(sizeof(*si), M_TEMP,
		    M_NOWAIT);
#ifdef DIAGNOSTIC
		if (si)
			++ncbd;		/* count # dynamically allocated */
#endif

		if (!si)
			return;
	}

	si->function = function;
	si->rock1 = rock1;
	si->rock2 = rock2;

	s = splhigh();
	si->next = NULL;
	if (si_callbacks)
		si_callbacks_end->next = si;
	else
		si_callbacks = si;
	si_callbacks_end = si;
	splx(s);

	/*
	 * Cause a software interrupt (spl1). This interrupt might
	 * happen immediately, or after returning to a safe enough level.
	 */
	setsoftcback();
}


void
rem_sicallback(function)
	void (*function) __P((void *rock1, void *rock2));
{
	struct si_callback *si, *psi, *nsi;
	int s;

	s = splhigh();
	for (psi = 0, si = si_callbacks; si; ) {
		nsi = si->next;

		if (si->function != function)
			psi = si;
		else {
			si->next = si_free;
			si_free = si;
			if (psi)
				psi->next = nsi;
			else
				si_callbacks = nsi;
			if (si == si_callbacks_end)
				si_callbacks_end = psi;
		}
		si = nsi;
	}
	splx(s);
}

/* purge the list */
static void
call_sicallbacks()
{
	struct si_callback *si;
	int s;
	void *rock1, *rock2;
	void (*function) __P((void *, void *));

	do {
		s = splhigh ();
		if ((si = si_callbacks) != 0)
			si_callbacks = si->next;
		splx(s);

		if (si) {
			function = si->function;
			rock1 = si->rock1;
			rock2 = si->rock2;
			s = splhigh ();
			si->next = si_free;
			si_free = si;
			splx(s);
			function (rock1, rock2);
		}
	} while (si);
#ifdef DIAGNOSTIC
	if (ncbd) {
		ncb += ncbd;
		printf ("call_sicallback: "
		    "%d more dynamic structures %d total\n", ncbd, ncb);
		ncbd = 0;
	}
#endif
}

void
add_isr(isr)
	struct isr *isr;
{
	struct isr **p, *q;

	switch (isr->isr_ipl) {
	case 2:
		p = &isr_ports;
		break;
#ifdef DRACO
	case 3:
		panic("DraCo IPL3 not yet supported here");
		p = &isr_slot3;
		break;
#endif
	case 6:
#if defined(IPL_REMAP_1) || defined(IPL_REMAP_2)
		p = &isr_exter[isr->isr_mapped_ipl];
		if (isr->isr_mapped_ipl > isr_exter_highipl)
			isr_exter_highipl = isr->isr_mapped_ipl;
		if (isr->isr_mapped_ipl < isr_exter_lowipl)
			isr_exter_lowipl = isr->isr_mapped_ipl;
#else
		p = &isr_exter;
#endif
		break;
	default:
		panic("add_isr: bad isr_ipl (%d)", isr->isr_ipl);
	}
	while ((q = *p) != NULL)
		p = &q->isr_forw;
	isr->isr_forw = NULL;
	*p = isr;
	/* enable interrupt */
#ifdef DRACO
	if (is_draco())
		*draco_intena |= isr->isr_ipl == 6 ?
			DRIRQ_INT6 : DRIRQ_INT2;
	else 
#endif
		custom.intena = INTF_SETCLR |
		    (isr->isr_ipl == 2 ? INTF_PORTS : INTF_EXTER);
}

void
remove_isr(isr)
	struct isr *isr;
{
	struct isr **p, *q, **chain;

	switch (isr->isr_ipl) {
	case 2:
		chain = &isr_ports;
		break;
#ifdef DRACO
	case 3:
		panic("DraCo IPL3 not yet supported here");
		chain = &isr_slot3;
		break;
#endif
	case 6:
#if defined(IPL_REMAP_1) || defined(IPL_REMAP_2)
		chain = &isr_exter[isr->isr_mapped_ipl];
#else
		chain = &isr_exter;
#endif
		break;
	default:
		panic("remove_isr: bad isr_ipl (%d)", isr->isr_ipl);
	}
	for (p = chain; (q = *p) != NULL && q != isr; p = &q->isr_forw)
		;
	if (q)
		*p = q->isr_forw;
	else
		panic("remove_isr: handler not registered");
	if (*chain == NULL)
		switch (isr->isr_ipl) {
		case 2:
#ifdef DRACO
			if (is_draco())
				*draco_intena &= ~DRIRQ_INT2;
			else
#endif
				custom.intena = INTF_PORTS;
			break;
#ifdef DRACO
		case 3:
			panic("DraCo IPL3 not yet supported here");
			break;
#endif
		case 6:
#if defined(IPL_REMAP_1) || defined(IPL_REMAP_2)
			if (isr->isr_mapped_ipl == isr_exter_lowipl) {
				while (isr_exter_lowipl++ < 6 &&
				    isr_exter[isr_exter_lowipl] == NULL)
					;
				if (isr_exter_lowipl == 7)
#ifdef DRACO
					if (is_draco())
						*draco_intena &= ~DRIRQ_INT6;
					else
#endif
						custom.intena = INTF_EXTER;
			}
			if (isr->isr_mapped_ipl == isr_exter_highipl)
				while (isr_exter_highipl-- > 0 &&
				    isr_exter[isr_exter_highipl] == NULL)
					;
#else
#ifdef DRACO
			if (is_draco())
				*draco_intena &= ~DRIRQ_INT6;
			else
#endif
				custom.intena = INTF_EXTER;
#endif
			break;
		}
}

void
intrhand(sr)
	int sr;
{
	register unsigned int ipl;
	register unsigned short ireq;
	register struct isr **p, *q;

	ipl = (sr >> 8) & 7;
#ifdef REALLYDEBUG
	printf("intrhand: got int. %d\n", ipl);
#endif
#ifdef DRACO
	if (is_draco())
		ireq = ((ipl == 1)  && (*draco_intfrc & DRIRQ_SOFT) ?
		    INTF_SOFTINT : 0);
	else
#endif
		ireq = custom.intreqr;

	switch (ipl) {
	case 1:
#ifdef DRACO
		if (is_draco() && (draco_ioct->io_status & DRSTAT_KBDRECV))
			drkbdintr();
#endif
		if (ireq & INTF_TBE) {
#if NSER > 0
			ser_outintr();
#else
			custom.intreq = INTF_TBE;
#endif
		}

		if (ireq & INTF_DSKBLK) {
#if NFD > 0
			fdintr(0);
#endif
			custom.intreq = INTF_DSKBLK;
		}
		if (ireq & INTF_SOFTINT) {
			unsigned char ssir_active;
			int s;

			/*
			 * first clear the softint-bit
			 * then process all classes of softints.
			 * this order is dictated by the nature of 
			 * software interrupts.  The other order
			 * allows software interrupts to be missed.
			 * Also copy and clear ssir to prevent
			 * interrupt loss.
			 */
			clrsoftint();
			s = splhigh();
			ssir_active = ssir;
			siroff(SIR_NET | SIR_CLOCK | SIR_CBACK);
			splx(s);
			if (ssir_active & SIR_NET) {
#ifdef REALLYDEBUG
				printf("calling netintr\n");
#endif
				cnt.v_soft++;
				netintr();
			}
			if (ssir_active & SIR_CLOCK) {
#ifdef REALLYDEBUG
				printf("calling softclock\n");
#endif
				cnt.v_soft++;
				/* XXXX softclock(&frame.f_stackadj); */
				softclock();
			}
			if (ssir_active & SIR_CBACK) {
#ifdef REALLYDEBUG
				printf("calling softcallbacks\n");
#endif
				cnt.v_soft++;
				call_sicallbacks();
			}
		}
		break;

	case 2:
		p = &isr_ports;
		while ((q = *p) != NULL) {
			if ((q->isr_intr)(q->isr_arg))
				break;
			p = &q->isr_forw;
		}
		if (q == NULL)
			ciaa_intr ();
#ifdef DRACO
		if (is_draco())
			*draco_intpen &= ~DRIRQ_INT2;
		else
#endif
			custom.intreq = INTF_PORTS;
		break;
    case 3: 
      /* VBL */
		if (ireq & INTF_BLIT)
			blitter_handler();
		if (ireq & INTF_COPER)
			copper_handler();
		if (ireq & INTF_VERTB)
			vbl_handler();
		break;
#if 0
/* now dealt with in locore.s for speed reasons */
	case 5:
		/* check RS232 RBF */
		serintr (0);

		custom.intreq = INTF_DSKSYNC;
		break;
#endif

	case 4:
#ifdef DRACO
#include "drsc.h"
		if (is_draco())
#if NDRSC > 0
			drsc_handler();
#else
			*draco_intpen &= ~DRIRQ_SCSI;
#endif
		else
#endif
		audio_handler();
		break;
	default:
		printf("intrhand: unexpected sr 0x%x, intreq = 0x%x\n",
		    sr, ireq);
		break;
	}
#ifdef REALLYDEBUG
	printf("intrhand: leaving.\n");
#endif
}

#if defined(DEBUG) && !defined(PANICBUTTON)
#define PANICBUTTON
#endif

#ifdef PANICBUTTON
int panicbutton = 1;	/* non-zero if panic buttons are enabled */
int crashandburn = 0;
int candbdelay = 50;	/* give em half a second */
void candbtimer __P((void));

void
candbtimer()
{
	crashandburn = 0;
}
#endif

#if 0
/*
 * Level 7 interrupts can be caused by the keyboard or parity errors.
 */
nmihand(frame)
	struct frame frame;
{
	if (kbdnmi()) {
#ifdef PANICBUTTON
		static int innmihand = 0;

		/*
		 * Attempt to reduce the window of vulnerability for recursive
		 * NMIs (e.g. someone holding down the keyboard reset button).
		 */
		if (innmihand == 0) {
			innmihand = 1;
			printf("Got a keyboard NMI\n");
			innmihand = 0;
		}
		if (panicbutton) {
			if (crashandburn) {
				crashandburn = 0;
				panic(panicstr ?
				      "forced crash, nosync" : "forced crash");
			}
			crashandburn++;
			timeout(candbtimer, (caddr_t)0, candbdelay);
		}
#endif
		return;
	}
	if (parityerror(&frame))
		return;
	/* panic?? */
	printf("unexpected level 7 interrupt ignored\n");
}
#endif

void
regdump(fp, sbytes)
	struct frame *fp; /* must not be register */
	int sbytes;
{
	static int doingdump = 0;
	register int i;
	int s;

	if (doingdump)
		return;
	s = spl7();
	doingdump = 1;
	printf("pid = %d, pc = %s, ", curproc ? curproc->p_pid : 0,
	    hexstr(fp->f_pc, 8));
	printf("ps = %s, ", hexstr(fp->f_sr, 4));
	printf("sfc = %s, ", hexstr(getsfc(), 4));
	printf("dfc = %s\n", hexstr(getdfc(), 4));
	printf("Registers:\n     ");
	for (i = 0; i < 8; i++)
		printf("        %d", i);
	printf("\ndreg:");
	for (i = 0; i < 8; i++)
		printf(" %s", hexstr(fp->f_regs[i], 8));
	printf("\nareg:");
	for (i = 0; i < 8; i++)
		printf(" %s", hexstr(fp->f_regs[i+8], 8));
	if (sbytes > 0) {
		if (fp->f_sr & PSL_S) {
			printf("\n\nKernel stack (%s):",
			       hexstr((int)(((int *)&fp)-1), 8));
			dumpmem(((int *)&fp)-1, sbytes, 0);
		} else {
			printf("\n\nUser stack (%s):", hexstr(fp->f_regs[SP], 8));
			dumpmem((int *)fp->f_regs[SP], sbytes, 1);
		}
	}
	doingdump = 0;
	splx(s);
}

extern u_int proc0paddr;
#define KSADDR	((int *)((curproc ? (u_int)curproc->p_addr : proc0paddr) + USPACE - NBPG))

static void
dumpmem(ptr, sz, ustack)
	register int *ptr;
	int sz, ustack;
{
	register int i, val;

	for (i = 0; i < sz; i++) {
		if ((i & 7) == 0)
			printf("\n%s: ", hexstr((int)ptr, 6));
		else
			printf(" ");
		if (ustack == 1) {
			if ((val = fuword(ptr++)) == -1)
				break;
		} else {
			if (ustack == 0 &&
			    (ptr < KSADDR || ptr > KSADDR+(NBPG/4-1)))
				break;
			val = *ptr++;
		}
		printf("%s", hexstr(val, 8));
	}
	printf("\n");
}

static char *
hexstr(val, len)
	register int val;
	int len;
{
	static char nbuf[9];
	register int x, i;

	if (len > 8)
		return("");
	nbuf[len] = '\0';
	for (i = len-1; i >= 0; --i) {
		x = val & 0xF;
		if (x > 9)
			nbuf[i] = x - 10 + 'A';
		else
			nbuf[i] = x + '0';
		val >>= 4;
	}
	return(nbuf);
}

/*
 * should only get here, if no standard executable. This can currently
 * only mean, we're reading an old ZMAGIC file without MID, but since Amiga
 * ZMAGIC always worked the `right' way (;-)) just ignore the missing
 * MID and proceed to new zmagic code ;-)
 */
int
cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	int error = ENOEXEC;
#ifdef COMPAT_NOMID
	struct exec *execp = epp->ep_hdr;
#endif

#ifdef COMPAT_NOMID
	if (!((execp->a_midmag >> 16) & 0x0fff)
	    && execp->a_midmag == ZMAGIC)
		return(exec_aout_prep_zmagic(p, epp));
#endif
#ifdef COMPAT_SUNOS
	{
		extern sunos_exec_aout_makecmds
		    __P((struct proc *, struct exec_package *));
		if ((error = sunos_exec_aout_makecmds(p, epp)) == 0)
			return(0);
	}
#endif
	return(error);
}
