/*	$OpenBSD: src/sys/sys/sched.h,v 1.11 2005/05/25 23:17:47 niklas Exp $	*/
/* $NetBSD: sched.h,v 1.2 1999/02/28 18:14:58 ross Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ross Harvey.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)kern_clock.c	8.5 (Berkeley) 1/21/94
 */

#ifndef	_SYS_SCHED_H_
#define	_SYS_SCHED_H_

/*
 * Posix defines a <sched.h> which may want to include <sys/sched.h>
 */

/*
 * CPU states.
 * XXX Not really scheduler state, but no other good place to put
 * it right now, and it really is per-CPU.
 */
#define CP_USER		0
#define CP_NICE		1
#define CP_SYS		2
#define CP_INTR		3
#define CP_IDLE		4
#define CPUSTATES	5

/*
 * Per-CPU scheduler state.
 * XXX - expose to userland for now.
 */
struct schedstate_percpu {
	struct timeval spc_runtime;	/* time curproc started running */
	__volatile int spc_schedflags;	/* flags; see below */
	u_int spc_schedticks;		/* ticks for schedclock() */
	u_int64_t spc_cp_time[CPUSTATES]; /* CPU state statistics */
	u_char spc_curpriority;		/* usrpri of curproc */
	int spc_rrticks;		/* ticks until roundrobin() */
	int spc_pscnt;			/* prof/stat counter */
	int spc_psdiv;			/* prof/stat divisor */	
};

#ifdef	_KERNEL

/* spc_flags */
#define SPCF_SEENRR             0x0001  /* process has seen roundrobin() */
#define SPCF_SHOULDYIELD        0x0002  /* process should yield the CPU */
#define SPCF_SWITCHCLEAR        (SPCF_SEENRR|SPCF_SHOULDYIELD)

#define	PPQ	(128 / NQS)		/* priorities per queue */
#define NICE_WEIGHT 2			/* priorities per nice level */
#define	ESTCPULIM(e) min((e), NICE_WEIGHT * PRIO_MAX - PPQ)

extern int schedhz;			/* ideally: 16 */
extern int rrticks_init;		/* ticks per roundrobin() */

struct proc;
void schedclock(struct proc *);
#ifdef __HAVE_CPUINFO
struct cpu_info;
void roundrobin(struct cpu_info *);
#endif

/* Inherit the parent's scheduler history */
#define scheduler_fork_hook(parent, child) do {				\
	(child)->p_estcpu = (parent)->p_estcpu;				\
} while (0)

/* Chargeback parents for the sins of their children.  */
#define scheduler_wait_hook(parent, child) do {				\
	(parent)->p_estcpu = ESTCPULIM((parent)->p_estcpu + (child)->p_estcpu);\
} while (0)

#if !defined(__HAVE_CPUINFO) && !defined(splsched)
#define splsched() splhigh()
#endif
#ifndef IPL_SCHED
#define IPL_SCHED IPL_HIGH
#endif

#if defined(MULTIPROCESSOR)
#include <sys/mutex.h>

extern struct mutex sched_mutex;

#define	SCHED_ASSERT_LOCKED()	MUTEX_ASSERT_LOCKED(&sched_mutex)
#define	SCHED_ASSERT_UNLOCKED()	MUTEX_ASSERT_UNLOCKED(&sched_mutex)

/*
 * We need this MUTEX_OLDIPL hack to be able to properly restore the old
 * ipl after all the ipl juggling in cpu_switch.
 */
#define SCHED_LOCK(s) do {						\
	mtx_enter(&sched_mutex);					\
	s = MUTEX_OLDIPL(&sched_mutex);					\
} while (0)

#define SCHED_UNLOCK(s) do {						\
	mtx_leave(&sched_mutex);					\
} while (0)

void	sched_lock_idle(void);
void	sched_unlock_idle(void);

#else /* ! MULTIPROCESSOR */

#define	SCHED_ASSERT_LOCKED()		splassert(IPL_SCHED);
#define	SCHED_ASSERT_UNLOCKED()		/* nothing */

#define	SCHED_LOCK(s)			s = splsched()
#define	SCHED_UNLOCK(s)			splx(s)

#endif /* MULTIPROCESSOR */

#endif	/* _KERNEL */
#endif	/* _SYS_SCHED_H_ */
