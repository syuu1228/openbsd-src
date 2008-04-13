/*	$OpenBSD: src/sys/arch/i386/i386/ipifuncs.c,v 1.11 2008/04/13 16:11:28 kettenis Exp $	*/
/* $NetBSD: ipifuncs.c,v 1.1.2.3 2000/06/26 02:04:06 sommerfeld Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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


#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

/*
 * Interprocessor interrupt handlers.
 */

#include "npx.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpufunc.h>
#include <machine/cpuvar.h>
#include <machine/intr.h>
#include <machine/atomic.h>
#include <machine/i82093var.h>
#include <machine/db_machdep.h>

#include <uvm/uvm_extern.h>

void i386_ipi_halt(struct cpu_info *);

#if NNPX > 0
void i386_ipi_synch_fpu(struct cpu_info *);
void i386_ipi_flush_fpu(struct cpu_info *);
#else
#define i386_ipi_synch_fpu 0
#define i386_ipi_flush_fpu 0
#endif

void (*ipifunc[I386_NIPI])(struct cpu_info *) =
{
	i386_ipi_halt,
	i386_ipi_flush_fpu,
	i386_ipi_synch_fpu,
#if 0
	i386_reload_mtrr,
	gdt_reload_cpu,
#else
	0,
	0,
#endif
#ifdef DDB
	i386_ipi_db,
#else
	0,
#endif
	i386_setperf_ipi,
};

void
i386_ipi_halt(struct cpu_info *ci)
{
	disable_intr();
	ci->ci_flags &= ~CPUF_RUNNING;

	for(;;) {
		asm volatile("hlt");
	}
}

#if NNPX > 0
void
i386_ipi_flush_fpu(struct cpu_info *ci)
{
	npxsave_cpu(ci, 0);
}

void
i386_ipi_synch_fpu(struct cpu_info *ci)
{
	npxsave_cpu(ci, 1);
}
#endif

void
i386_spurious(void)
{
	printf("spurious intr\n");
}

int
i386_send_ipi(struct cpu_info *ci, int ipimask)
{
	int ret;

	i386_atomic_setbits_l(&ci->ci_ipis, ipimask);

	/* Don't send IPI to cpu which isn't (yet) running. */
	if (!(ci->ci_flags & CPUF_RUNNING))
		return ENOENT;

	ret = i386_ipi(LAPIC_IPI_VECTOR, ci->ci_cpuid, LAPIC_DLMODE_FIXED);
	if (ret != 0) {
		printf("ipi of %x from %s to %s failed\n",
		    ipimask, curcpu()->ci_dev.dv_xname, ci->ci_dev.dv_xname);
	}

	return ret;
}

int
i386_fast_ipi(struct cpu_info *ci, int ipi)
{
	if (!(ci->ci_flags & CPUF_RUNNING))
		return (ENOENT);

	return (i386_ipi(ipi, ci->ci_cpuid, LAPIC_DLMODE_FIXED));
}

void
i386_self_ipi(int vector)
{
	i82489_writereg(LAPIC_ICRLO,
	    vector | LAPIC_DLMODE_FIXED | LAPIC_LVL_ASSERT | LAPIC_DEST_SELF);
}


void
i386_broadcast_ipi(int ipimask)
{
	struct cpu_info *ci, *self = curcpu();
	CPU_INFO_ITERATOR cii;
	int count = 0;

	CPU_INFO_FOREACH(cii, ci) {
		if (ci == self)
			continue;
		if ((ci->ci_flags & CPUF_RUNNING) == 0)
			continue;
		i386_atomic_setbits_l(&ci->ci_ipis, ipimask);
		count++;
	}
	if (!count)
		return;

	i386_ipi(LAPIC_IPI_VECTOR, LAPIC_DEST_ALLEXCL, LAPIC_DLMODE_FIXED); 
}

void
i386_ipi_handler(void)
{
	extern struct evcount ipi_count;
	struct cpu_info *ci = curcpu();
	u_int32_t pending;
	int bit;

	pending = i386_atomic_testset_ul(&ci->ci_ipis, 0);

	for (bit = 0; bit < I386_NIPI && pending; bit++) {
		if (pending & (1<<bit)) {
			pending &= ~(1<<bit);
			(*ipifunc[bit])(ci);
			ipi_count.ec_count++;
		}
	}
}
