/*	$OpenBSD: src/sys/arch/hppa/include/intr.h,v 1.29 2010/04/19 16:32:53 jsing Exp $	*/

/*
 * Copyright (c) 2002-2004 Michael Shalayeff
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MACHINE_INTR_H_
#define _MACHINE_INTR_H_

#include <machine/psl.h>

#define	CPU_NINTS	32
#define	NIPL		16

#define	IPL_NONE	0
#define	IPL_SOFTCLOCK	1
#define	IPL_SOFTNET	2
#define	IPL_BIO		3
#define	IPL_NET		4
#define	IPL_SOFTTTY	5
#define	IPL_TTY		6
#define	IPL_VM		7
#define	IPL_AUDIO	8
#define	IPL_CLOCK	9
#define	IPL_STATCLOCK	10
#define	IPL_HIGH	10
#define	IPL_NESTED	11	/* pseudo-level for sub-tables */

#define	IST_NONE	0
#define	IST_PULSE	1
#define	IST_EDGE	2
#define	IST_LEVEL	3

#if !defined(_LOCORE) && defined(_KERNEL)

#include <machine/atomic.h>

extern volatile u_long imask[NIPL];

#ifdef DIAGNOSTIC
void splassert_fail(int, int, const char *);
extern int splassert_ctl;
void splassert_check(int, const char *);
#define splassert(__wantipl) do {			\
	if (splassert_ctl > 0) {			\
		splassert_check(__wantipl, __func__);	\
	}						\
} while (0)
#define splsoftassert(__wantipl) splassert(__wantipl)
#else
#define	splassert(__wantipl)		do { /* nada */ } while (0)
#define	splsoftassert(__wantipl)	do { /* nada */ } while (0)
#endif /* DIAGNOSTIC */

void	cpu_intr_init(void);
void	cpu_intr(void *);

static __inline int
spllower(int ncpl)
{
	register int ocpl asm("r28") = ncpl;
	__asm __volatile("copy  %0, %%arg0\n\tbreak %1, %2"
	    : "+r" (ocpl) : "i" (HPPA_BREAK_KERNEL), "i" (HPPA_BREAK_SPLLOWER)
	    : "r26", "memory");
	return (ocpl);
}

static __inline int
splraise(int ncpl)
{
	struct cpu_info *ci = curcpu();
	int ocpl = ci->ci_cpl;

	if (ocpl < ncpl)
		ci->ci_cpl = ncpl;
	__asm __volatile ("sync" : "+r" (ci->ci_cpl));

	return (ocpl);
}

static __inline void
splx(int ncpl)
{
	(void)spllower(ncpl);
}

static __inline register_t
hppa_intr_disable(void)
{
	register_t eiem;

	__asm __volatile("mfctl %%cr15, %0": "=r" (eiem));
	__asm __volatile("mtctl %r0, %cr15");

	return eiem;
}

static __inline void
hppa_intr_enable(register_t eiem)
{
	__asm __volatile("mtctl %0, %%cr15":: "r" (eiem));
}

#define	splsoftclock()	splraise(IPL_SOFTCLOCK)
#define	splsoftnet()	splraise(IPL_SOFTNET)
#define	splbio()	splraise(IPL_BIO)
#define	splnet()	splraise(IPL_NET)
#define	splsofttty()	splraise(IPL_SOFTTTY)
#define	spltty()	splraise(IPL_TTY)
#define	splvm()		splraise(IPL_VM)
#define	splaudio()	splraise(IPL_AUDIO)
#define	splclock()	splraise(IPL_CLOCK)
#define	splsched()	splraise(IPL_SCHED)
#define	splstatclock()	splraise(IPL_STATCLOCK)
#define	splhigh()	splraise(IPL_HIGH)
#define	spl0()		spllower(IPL_NONE)

#define	softintr(mask)	atomic_setbits_long(&curcpu()->ci_ipending, mask)

#define	SOFTINT_MASK ((1 << (IPL_SOFTCLOCK - 1)) | \
    (1 << (IPL_SOFTNET - 1)) | (1 << (IPL_SOFTTTY - 1)))

#define	setsoftast(p)	(p->p_md.md_astpending = 1)
#define	setsoftnet()	softintr(1 << (IPL_SOFTNET - 1))

void	*softintr_establish(int, void (*)(void *), void *);
void	 softintr_disestablish(void *);
void	 softintr_schedule(void *);

#endif /* !_LOCORE && _KERNEL */
#endif /* _MACHINE_INTR_H_ */
