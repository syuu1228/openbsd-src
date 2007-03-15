/*	$OpenBSD: src/sys/sys/resourcevar.h,v 1.11 2007/03/15 10:22:30 art Exp $	*/
/*	$NetBSD: resourcevar.h,v 1.12 1995/11/22 23:01:53 cgd Exp $	*/

/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)resourcevar.h	8.3 (Berkeley) 2/22/94
 */

#ifndef	_SYS_RESOURCEVAR_H_
#define	_SYS_RESOURCEVAR_H_

#include <sys/timeout.h>

/*
 * Kernel per-process accounting / statistics
 * (not necessarily resident except when running).
 */
struct pstats {
#define	pstat_startzero	p_ru
	struct	rusage p_ru;		/* stats for this proc */
	struct	rusage p_cru;		/* sum of stats for reaped children */
	struct	itimerval p_timer[3];	/* virtual-time timers */
#define	pstat_endzero	pstat_startcopy

#define	pstat_startcopy	p_prof
	struct uprof {			/* profile arguments */
		caddr_t	pr_base;	/* buffer base */
		size_t  pr_size;	/* buffer size */
		u_long	pr_off;		/* pc offset */
		u_int   pr_scale;	/* pc scaling */
		u_long	pr_addr;	/* temp storage for addr until AST */
		u_long	pr_ticks;	/* temp storage for ticks until AST */
	} p_prof;
#define	pstat_endcopy	p_start
	struct	timeval p_start;	/* starting time */
	struct	timeout p_virt_to;	/* virtual itimer trampoline. */
	struct	timeout p_prof_to;	/* prof itimer trampoline. */
};

/*
 * Kernel shareable process resource limits.  Because this structure
 * is moderately large but changes infrequently, it is normally
 * shared copy-on-write after forks.  If a group of processes
 * ("threads") share modifications, the PL_SHAREMOD flag is set,
 * and a copy must be made for the child of a new fork that isn't
 * sharing modifications to the limits.
 */
struct plimit {
	struct	rlimit pl_rlimit[RLIM_NLIMITS];
#define	PL_SHAREMOD	0x01		/* modifications are shared */
	int	p_lflags;
	int	p_refcnt;		/* number of references */
};

/* add user profiling from AST */
#define	ADDUPROF(p)							\
do {									\
	atomic_clearbits_int(&(p)->p_flag, P_OWEUPC);			\
	addupc_task((p), (p)->p_stats->p_prof.pr_addr,			\
	    (p)->p_stats->p_prof.pr_ticks);				\
	(p)->p_stats->p_prof.pr_ticks = 0;				\
} while (0)

#ifdef _KERNEL
void	 addupc_intr(struct proc *p, u_long pc);
void	 addupc_task(struct proc *p, u_long pc, u_int ticks);
void	 calcru(struct proc *p, struct timeval *up, struct timeval *sp,
	    struct timeval *ip);
struct plimit *limcopy(struct plimit *lim);
void	limfree(struct plimit *);

void	 ruadd(struct rusage *ru, struct rusage *ru2);

void	virttimer_trampoline(void *);
void	proftimer_trampoline(void *);
#endif
#endif	/* !_SYS_RESOURCEVAR_H_ */
