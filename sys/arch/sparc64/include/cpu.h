/*	$OpenBSD: src/sys/arch/sparc64/include/cpu.h,v 1.28 2004/06/21 23:12:14 art Exp $	*/
/*	$NetBSD: cpu.h,v 1.28 2001/06/14 22:56:58 thorpej Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)cpu.h	8.4 (Berkeley) 1/5/94
 */

#ifndef _CPU_H_
#define _CPU_H_

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_BOOTED_KERNEL	1	/* string: booted kernel name */
#define	CPU_LED_BLINK		2	/* int: blink leds? */
#define	CPU_ALLOWAPERTURE	3	/* allow xf86 operations */
#define	CPU_CPUTYPE		4	/* cpu type */
#define	CPU_CECCERRORS		5	/* Correctable ECC errors */
#define	CPU_CECCLAST		6	/* Correctable ECC last fault addr */
#define	CPU_MAXID		7	/* number of valid machdep ids */

#define	CTL_MACHDEP_NAMES {			\
	{ 0, 0 },				\
	{ "booted_kernel", CTLTYPE_STRING },	\
	{ "led_blink", CTLTYPE_INT },		\
	{ "allowaperture", CTLTYPE_INT },	\
	{ "cputype", CTLTYPE_INT },		\
	{ "ceccerrs", CTLTYPE_INT },		\
	{ "cecclast", CTLTYPE_QUAD },		\
}

#ifdef _KERNEL
/*
 * Exported definitions unique to SPARC cpu support.
 */

#include <machine/ctlreg.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/intr.h>

#include <sys/sched.h>

/*
 * The cpu_info structure is part of a 64KB structure mapped both the kernel
 * pmap and a single locked TTE a CPUINFO_VA for that particular processor.
 * Each processor's cpu_info is accessible at CPUINFO_VA only for that
 * processor.  Other processors can access that through an additional mapping
 * in the kernel pmap.
 *
 * The 64KB page contains:
 *
 * cpu_info
 * interrupt stack (all remaining space)
 * idle PCB
 * idle stack (STACKSPACE - sizeof(PCB))
 * 32KB TSB
 */

struct cpu_info {
	/* Most important fields first */
	struct proc		*ci_curproc;
	struct pcb		*ci_cpcb;	/* also initial stack */
	struct cpu_info		*ci_next;

	struct proc		*ci_fpproc;
	int			ci_number;
	int			ci_upaid;
	struct schedstate_percpu ci_schedstate; /* scheduler state */

	/* DEBUG/DIAGNOSTIC stuff */
	u_long			ci_spin_locks;	/* # of spin locks held */
	u_long			ci_simple_locks;/* # of simple locks held */

	/* Spinning up the CPU */
	void			(*ci_spinup)(void); /* spinup routine */
	void			*ci_initstack;
	paddr_t			ci_paddr;	/* Phys addr of this structure. */
};

extern struct cpu_info *cpus;
extern struct cpu_info cpu_info_store;

#if 1
#define	curcpu()	(&cpu_info_store)
#else
#define	curcpu()	((struct cpu_info *)CPUINFO_VA)
#endif

/*
 * definitions of cpu-dependent requirements
 * referenced in generic code
 */
#define	cpu_swapin(p)	/* nothing */
#define	cpu_swapout(p)	/* nothing */
#define	cpu_wait(p)	/* nothing */
#if 1
#define cpu_number()	0
#else
#define	cpu_number()	(curcpu()->ci_number)
#endif

/*
 * Arguments to hardclock, softclock and gatherstats encapsulate the
 * previous machine state in an opaque clockframe.  The ipl is here
 * as well for strayintr (see locore.s:interrupt and intr.c:strayintr).
 * Note that CLKF_INTR is valid only if CLKF_USERMODE is false.
 */
extern int intstack[];
extern int eintstack[];
struct clockframe {
	struct trapframe64 t;
};

#define	CLKF_USERMODE(framep)	(((framep)->t.tf_tstate & TSTATE_PRIV) == 0)
#define	CLKF_PC(framep)		((framep)->t.tf_pc)
#define	CLKF_INTR(framep)	((!CLKF_USERMODE(framep))&&\
				(((framep)->t.tf_kstack < (vaddr_t)EINTSTACK)&&\
				((framep)->t.tf_kstack > (vaddr_t)INTSTACK)))

/*
 * Software interrupt request `register'.
 */
#ifdef DEPRECATED
union sir {
	int	sir_any;
	char	sir_which[4];
} sir;

#define SIR_NET		0
#define SIR_CLOCK	1
#endif

void setsoftint(void);
void setsoftnet(void);

extern	int want_ast;

/*
 * Preempt the current process if in interrupt from user mode,
 * or after the current trap/syscall if in system mode.
 */
extern	int want_resched;	/* resched() was called */
#define	need_resched(ci)	(want_resched = 1, want_ast = 1)

/*
 * Give a profiling tick to the current process when the user profiling
 * buffer pages are invalid.  On the sparc, request an ast to send us
 * through trap(), marking the proc as needing a profiling tick.
 */
#define	need_proftick(p)	((p)->p_flag |= P_OWEUPC, want_ast = 1)

/*
 * Notify the current process (p) that it has a signal pending,
 * process as soon as possible.
 */
#define	signotify(p)		(want_ast = 1)

/*
 * Only one process may own the FPU state.
 *
 * XXX this must be per-cpu (eventually)
 */
extern	struct proc *fpproc;	/* FPU owner */
extern	int foundfpu;		/* true => we have an FPU */

/* disksubr.c */
struct dkbad;
int isbad(struct dkbad *bt, int, int, int);
/* machdep.c */
int	ldcontrolb(caddr_t);
void	dumpconf(void);
caddr_t	reserve_dumppages(caddr_t);
/* clock.c */
struct timeval;
int	tickintr(void *); /* level 10 (tick) interrupt code */
int	clockintr(void *);/* level 10 (clock) interrupt code */
int	statintr(void *);	/* level 14 (statclock) interrupt code */
/* locore.s */
struct fpstate64;
void	savefpstate(struct fpstate64 *);
void	loadfpstate(struct fpstate64 *);
u_int64_t	probeget(paddr_t, int, int);
int	probeset(paddr_t, int, int, u_int64_t);
#define	 write_all_windows() __asm __volatile("flushw" : : )
#define	 write_user_windows() __asm __volatile("flushw" : : )
void 	proc_trampoline(void);
struct pcb;
void	snapshot(struct pcb *);
struct frame *getfp(void);
int	xldcontrolb(caddr_t, struct pcb *);
void	copywords(const void *, void *, size_t);
void	qcopy(const void *, void *, size_t);
void	qzero(void *, size_t);
void	switchtoctx(int);
/* locore2.c */
void	remrq(struct proc *);
/* trap.c */
void	kill_user_windows(struct proc *);
int	rwindow_save(struct proc *);
/* amd7930intr.s */
void	amd7930_trap(void);
/* cons.c */
int	cnrom(void);
/* zs.c */
void zsconsole(struct tty *, int, int, void (**)(struct tty *, int));
#ifdef KGDB
void zs_kgdb_init(void);
#endif
/* fb.c */
void	fb_unblank(void);
/* kgdb_stub.c */
#ifdef KGDB
void kgdb_attach(int (*)(void *), void (*)(void *, int), void *);
void kgdb_connect(int);
void kgdb_panic(void);
#endif
/* emul.c */
int	fixalign(struct proc *, struct trapframe64 *);
int	emulinstr(vaddr_t, struct trapframe64 *);
int	emul_qf(int32_t, struct proc *, union sigval, struct trapframe64 *);
int	emul_popc(int32_t, struct proc *, union sigval, struct trapframe64 *);

/*
 *
 * The SPARC has a Trap Base Register (TBR) which holds the upper 20 bits
 * of the trap vector table.  The next eight bits are supplied by the
 * hardware when the trap occurs, and the bottom four bits are always
 * zero (so that we can shove up to 16 bytes of executable code---exactly
 * four instructions---into each trap vector).
 *
 * The hardware allocates half the trap vectors to hardware and half to
 * software.
 *
 * Traps have priorities assigned (lower number => higher priority).
 */

struct trapvec {
	int	tv_instr[8];		/* the eight instructions */
};
extern struct trapvec *trapbase;	/* the 256 vectors */

extern void wzero(void *, u_int);
extern void wcopy(const void *, void *, u_int);

#endif /* _KERNEL */
#endif /* _CPU_H_ */
