/*	$OpenBSD: src/sys/arch/arm/arm/cpufunc.c,v 1.9 2009/05/08 02:57:32 drahn Exp $	*/
/*	$NetBSD: cpufunc.c,v 1.65 2003/11/05 12:53:15 scw Exp $	*/

/*
 * arm7tdmi support code Copyright (c) 2001 John Fremlin
 * arm8 support code Copyright (c) 1997 ARM Limited
 * arm8 support code Copyright (c) 1997 Causality Limited
 * arm9 support code Copyright (C) 2001 ARM Ltd
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Causality Limited.
 * 4. The name of Causality Limited may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUSALITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * cpufuncs.c
 *
 * C functions for supporting CPU / MMU / TLB specific operations.
 *
 * Created      : 30/01/97
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <machine/cpu.h>
#include <machine/bootconfig.h>

#include <uvm/uvm.h>

#include <arm/cpuconf.h>
#include <arm/cpufunc.h>

#ifdef CPU_XSCALE_80200
#include <arm/xscale/i80200reg.h>
#include <arm/xscale/i80200var.h>
#endif

#ifdef CPU_XSCALE_80321
#include <arm/xscale/i80321reg.h>
#include <arm/xscale/i80321var.h>
#endif

#ifdef CPU_XSCALE_IXP425
#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>
#endif

#if defined(CPU_XSCALE_80200) || defined(CPU_XSCALE_80321)
#include <arm/xscale/xscalereg.h>
#endif

#if defined(PERFCTRS)
struct arm_pmc_funcs *arm_pmc;
#endif

/* PRIMARY CACHE VARIABLES */
int	arm_picache_size;
int	arm_picache_line_size;
int	arm_picache_ways;

int	arm_pdcache_size;	/* and unified */
int	arm_pdcache_line_size;
int	arm_pdcache_ways;

int	arm_pcache_type;
int	arm_pcache_unified;

int	arm_dcache_align;
int	arm_dcache_align_mask;

/* 1 == use cpu_sleep(), 0 == don't */
int cpu_do_powersave;

#ifdef CPU_ARM3
struct cpu_functions arm3_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	cpufunc_nullop,			/* cpwait		*/

	/* MMU functions */

	arm3_control,			/* control		*/
	NULL,				/* domain		*/
	NULL,				/* setttb		*/
	NULL,				/* faultstatus		*/
	NULL,				/* faultaddress		*/

	/* TLB functions */

	cpufunc_nullop,			/* tlb_flushID		*/
	(void *)cpufunc_nullop,		/* tlb_flushID_SE	*/
	cpufunc_nullop,			/* tlb_flushI		*/
	(void *)cpufunc_nullop,		/* tlb_flushI_SE	*/
	cpufunc_nullop,			/* tlb_flushD		*/
	(void *)cpufunc_nullop,		/* tlb_flushD_SE	*/

	/* Cache operations */

	cpufunc_nullop,			/* icache_sync_all	*/
	(void *) cpufunc_nullop,	/* icache_sync_range	*/

	arm3_cache_flush,		/* dcache_wbinv_all	*/
	(void *)arm3_cache_flush,	/* dcache_wbinv_range	*/
	(void *)arm3_cache_flush,	/* dcache_inv_range	*/
	(void *)cpufunc_nullop,		/* dcache_wb_range	*/

	arm3_cache_flush,		/* idcache_wbinv_all	*/
	(void *)arm3_cache_flush,	/* idcache_wbinv_range	*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	cpufunc_nullop,			/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	(void *)cpufunc_nullop,		/* sleep		*/

	/* Soft functions */

	early_abort_fixup,		/* dataabt_fixup	*/
	cpufunc_null_fixup,		/* prefetchabt_fixup	*/

	NULL,				/* context_switch	*/

	(void *)cpufunc_nullop		/* cpu setup		*/

};
#endif	/* CPU_ARM3 */

#ifdef CPU_ARM6
struct cpu_functions arm6_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	cpufunc_nullop,			/* cpwait		*/

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* domain		*/
	arm67_setttb,			/* setttb		*/
	cpufunc_faultstatus,		/* faultstatus		*/
	cpufunc_faultaddress,		/* faultaddress		*/

	/* TLB functions */

	arm67_tlb_flush,		/* tlb_flushID		*/
	arm67_tlb_purge,		/* tlb_flushID_SE	*/
	arm67_tlb_flush,		/* tlb_flushI		*/
	arm67_tlb_purge,		/* tlb_flushI_SE	*/
	arm67_tlb_flush,		/* tlb_flushD		*/
	arm67_tlb_purge,		/* tlb_flushD_SE	*/

	/* Cache operations */

	cpufunc_nullop,			/* icache_sync_all	*/
	(void *) cpufunc_nullop,	/* icache_sync_range	*/

	arm67_cache_flush,		/* dcache_wbinv_all	*/
	(void *)arm67_cache_flush,	/* dcache_wbinv_range	*/
	(void *)arm67_cache_flush,	/* dcache_inv_range	*/
	(void *)cpufunc_nullop,		/* dcache_wb_range	*/

	arm67_cache_flush,		/* idcache_wbinv_all	*/
	(void *)arm67_cache_flush,	/* idcache_wbinv_range	*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	cpufunc_nullop,			/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	(void *)cpufunc_nullop,		/* sleep		*/

	/* Soft functions */

#ifdef ARM6_LATE_ABORT
	late_abort_fixup,		/* dataabt_fixup	*/
#else
	early_abort_fixup,		/* dataabt_fixup	*/
#endif
	cpufunc_null_fixup,		/* prefetchabt_fixup	*/

	arm67_context_switch,		/* context_switch	*/

	arm6_setup			/* cpu setup		*/

};
#endif	/* CPU_ARM6 */

#ifdef CPU_ARM7
struct cpu_functions arm7_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	cpufunc_nullop,			/* cpwait		*/

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* domain		*/
	arm67_setttb,			/* setttb		*/
	cpufunc_faultstatus,		/* faultstatus		*/
	cpufunc_faultaddress,		/* faultaddress		*/

	/* TLB functions */

	arm67_tlb_flush,		/* tlb_flushID		*/
	arm67_tlb_purge,		/* tlb_flushID_SE	*/
	arm67_tlb_flush,		/* tlb_flushI		*/
	arm67_tlb_purge,		/* tlb_flushI_SE	*/
	arm67_tlb_flush,		/* tlb_flushD		*/
	arm67_tlb_purge,		/* tlb_flushD_SE	*/

	/* Cache operations */

	cpufunc_nullop,			/* icache_sync_all	*/
	(void *)cpufunc_nullop,		/* icache_sync_range	*/

	arm67_cache_flush,		/* dcache_wbinv_all	*/
	(void *)arm67_cache_flush,	/* dcache_wbinv_range	*/
	(void *)arm67_cache_flush,	/* dcache_inv_range	*/
	(void *)cpufunc_nullop,		/* dcache_wb_range	*/

	arm67_cache_flush,		/* idcache_wbinv_all	*/
	(void *)arm67_cache_flush,	/* idcache_wbinv_range	*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	cpufunc_nullop,			/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	(void *)cpufunc_nullop,		/* sleep		*/

	/* Soft functions */

	late_abort_fixup,		/* dataabt_fixup	*/
	cpufunc_null_fixup,		/* prefetchabt_fixup	*/

	arm67_context_switch,		/* context_switch	*/

	arm7_setup			/* cpu setup		*/

};
#endif	/* CPU_ARM7 */

#ifdef CPU_ARM7TDMI
struct cpu_functions arm7tdmi_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	cpufunc_nullop,			/* cpwait		*/

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* domain		*/
	arm7tdmi_setttb,		/* setttb		*/
	cpufunc_faultstatus,		/* faultstatus		*/
	cpufunc_faultaddress,		/* faultaddress		*/

	/* TLB functions */

	arm7tdmi_tlb_flushID,		/* tlb_flushID		*/
	arm7tdmi_tlb_flushID_SE,	/* tlb_flushID_SE	*/
	arm7tdmi_tlb_flushID,		/* tlb_flushI		*/
	arm7tdmi_tlb_flushID_SE,	/* tlb_flushI_SE	*/
	arm7tdmi_tlb_flushID,		/* tlb_flushD		*/
	arm7tdmi_tlb_flushID_SE,	/* tlb_flushD_SE	*/

	/* Cache operations */

	cpufunc_nullop,			/* icache_sync_all	*/
	(void *)cpufunc_nullop,		/* icache_sync_range	*/

	arm7tdmi_cache_flushID,		/* dcache_wbinv_all	*/
	(void *)arm7tdmi_cache_flushID,	/* dcache_wbinv_range	*/
	(void *)arm7tdmi_cache_flushID,	/* dcache_inv_range	*/
	(void *)cpufunc_nullop,		/* dcache_wb_range	*/

	arm7tdmi_cache_flushID,		/* idcache_wbinv_all	*/
	(void *)arm7tdmi_cache_flushID,	/* idcache_wbinv_range	*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	cpufunc_nullop,			/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	(void *)cpufunc_nullop,		/* sleep		*/

	/* Soft functions */

	late_abort_fixup,		/* dataabt_fixup	*/
	cpufunc_null_fixup,		/* prefetchabt_fixup	*/

	arm7tdmi_context_switch,	/* context_switch	*/

	arm7tdmi_setup			/* cpu setup		*/

};
#endif	/* CPU_ARM7TDMI */

#ifdef CPU_ARM8
struct cpu_functions arm8_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	cpufunc_nullop,			/* cpwait		*/

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* domain		*/
	arm8_setttb,			/* setttb		*/
	cpufunc_faultstatus,		/* faultstatus		*/
	cpufunc_faultaddress,		/* faultaddress		*/

	/* TLB functions */

	arm8_tlb_flushID,		/* tlb_flushID		*/
	arm8_tlb_flushID_SE,		/* tlb_flushID_SE	*/
	arm8_tlb_flushID,		/* tlb_flushI		*/
	arm8_tlb_flushID_SE,		/* tlb_flushI_SE	*/
	arm8_tlb_flushID,		/* tlb_flushD		*/
	arm8_tlb_flushID_SE,		/* tlb_flushD_SE	*/

	/* Cache operations */

	cpufunc_nullop,			/* icache_sync_all	*/
	(void *)cpufunc_nullop,		/* icache_sync_range	*/

	arm8_cache_purgeID,		/* dcache_wbinv_all	*/
	(void *)arm8_cache_purgeID,	/* dcache_wbinv_range	*/
/*XXX*/	(void *)arm8_cache_purgeID,	/* dcache_inv_range	*/
	(void *)arm8_cache_cleanID,	/* dcache_wb_range	*/

	arm8_cache_purgeID,		/* idcache_wbinv_all	*/
	(void *)arm8_cache_purgeID,	/* idcache_wbinv_range	*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	cpufunc_nullop,			/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	(void *)cpufunc_nullop,		/* sleep		*/

	/* Soft functions */

	cpufunc_null_fixup,		/* dataabt_fixup	*/
	cpufunc_null_fixup,		/* prefetchabt_fixup	*/

	arm8_context_switch,		/* context_switch	*/

	arm8_setup			/* cpu setup		*/
};
#endif	/* CPU_ARM8 */

#ifdef CPU_ARM9
struct cpu_functions arm9_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	cpufunc_nullop,			/* cpwait		*/

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* Domain		*/
	arm9_setttb,			/* Setttb		*/
	cpufunc_faultstatus,		/* Faultstatus		*/
	cpufunc_faultaddress,		/* Faultaddress		*/

	/* TLB functions */

	armv4_tlb_flushID,		/* tlb_flushID		*/
	arm9_tlb_flushID_SE,		/* tlb_flushID_SE	*/
	armv4_tlb_flushI,		/* tlb_flushI		*/
	(void *)armv4_tlb_flushI,	/* tlb_flushI_SE	*/
	armv4_tlb_flushD,		/* tlb_flushD		*/
	armv4_tlb_flushD_SE,		/* tlb_flushD_SE	*/

	/* Cache operations */

	arm9_icache_sync_all,		/* icache_sync_all	*/
	arm9_icache_sync_range,		/* icache_sync_range	*/

		/* ...cache in write-though mode... */
	arm9_dcache_wbinv_all,		/* dcache_wbinv_all	*/
	arm9_dcache_wbinv_range,	/* dcache_wbinv_range	*/
	arm9_dcache_wbinv_range,	/* dcache_inv_range	*/
	arm9_dcache_wb_range,		/* dcache_wb_range	*/

	arm9_idcache_wbinv_all,		/* idcache_wbinv_all	*/
	arm9_idcache_wbinv_range,	/* idcache_wbinv_range	*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	armv4_drain_writebuf,		/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	(void *)cpufunc_nullop,		/* sleep		*/

	/* Soft functions */

	cpufunc_null_fixup,		/* dataabt_fixup	*/
	cpufunc_null_fixup,		/* prefetchabt_fixup	*/

	arm9_context_switch,		/* context_switch	*/

	arm9_setup			/* cpu setup		*/

};
#endif /* CPU_ARM9 */

#if defined(CPU_ARM9E) || defined(CPU_ARM10)
struct cpu_functions armv5_ec_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	cpufunc_nullop,			/* cpwait		*/

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* Domain		*/
	armv5_ec_setttb,		/* Setttb		*/
	cpufunc_faultstatus,		/* Faultstatus		*/
	cpufunc_faultaddress,		/* Faultaddress		*/

	/* TLB functions */

	armv4_tlb_flushID,		/* tlb_flushID		*/
	arm10_tlb_flushID_SE,		/* tlb_flushID_SE	*/
	armv4_tlb_flushI,		/* tlb_flushI		*/
	arm10_tlb_flushI_SE,		/* tlb_flushI_SE	*/
	armv4_tlb_flushD,		/* tlb_flushD		*/
	armv4_tlb_flushD_SE,		/* tlb_flushD_SE	*/

	/* Cache operations */

	armv5_ec_icache_sync_all,	/* icache_sync_all	*/
	armv5_ec_icache_sync_range,	/* icache_sync_range	*/

		/* ...cache in write-though mode... */
	armv5_ec_dcache_wbinv_all,	/* dcache_wbinv_all	*/
	armv5_ec_dcache_wbinv_range,	/* dcache_wbinv_range	*/
	armv5_ec_dcache_wbinv_range,	/* dcache_inv_range	*/
	armv5_ec_dcache_wb_range,	/* dcache_wb_range	*/

	armv5_ec_idcache_wbinv_all,	/* idcache_wbinv_all	*/
	armv5_ec_idcache_wbinv_range,	/* idcache_wbinv_range	*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	armv4_drain_writebuf,		/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	(void *)cpufunc_nullop,		/* sleep		*/

	/* Soft functions */

	cpufunc_null_fixup,		/* dataabt_fixup	*/
	cpufunc_null_fixup,		/* prefetchabt_fixup	*/

	arm10_context_switch,		/* context_switch	*/

	arm10_setup			/* cpu setup		*/

};
#endif /* CPU_ARM9E || CPU_ARM10 */


#ifdef CPU_ARM10
struct cpu_functions arm10_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	cpufunc_nullop,			/* cpwait		*/

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* Domain		*/
	armv5_setttb,			/* Setttb		*/
	cpufunc_faultstatus,		/* Faultstatus		*/
	cpufunc_faultaddress,		/* Faultaddress		*/

	/* TLB functions */

	armv4_tlb_flushID,		/* tlb_flushID		*/
	arm10_tlb_flushID_SE,		/* tlb_flushID_SE	*/
	armv4_tlb_flushI,		/* tlb_flushI		*/
	arm10_tlb_flushI_SE,		/* tlb_flushI_SE	*/
	armv4_tlb_flushD,		/* tlb_flushD		*/
	armv4_tlb_flushD_SE,		/* tlb_flushD_SE	*/

	/* Cache operations */

	armv5_icache_sync_all,		/* icache_sync_all	*/
	armv5_icache_sync_range,	/* icache_sync_range	*/

	armv5_dcache_wbinv_all,		/* dcache_wbinv_all	*/
	armv5_dcache_wbinv_range,	/* dcache_wbinv_range	*/
	armv5_dcache_inv_range,		/* dcache_inv_range	*/
	armv5_dcache_wb_range,		/* dcache_wb_range	*/

	armv5_idcache_wbinv_all,	/* idcache_wbinv_all	*/
	armv5_idcache_wbinv_range,	/* idcache_wbinv_range	*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	armv4_drain_writebuf,		/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	(void *)cpufunc_nullop,		/* sleep		*/

	/* Soft functions */

	cpufunc_null_fixup,		/* dataabt_fixup	*/
	cpufunc_null_fixup,		/* prefetchabt_fixup	*/

	arm10_context_switch,		/* context_switch	*/

	arm10_setup			/* cpu setup		*/

};
#endif /* CPU_ARM10 */

#ifdef CPU_ARM11
struct cpu_functions arm11_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id				*/
	cpufunc_nullop,			/* cpwait			*/

	/* MMU functions */

	cpufunc_control,		/* control			*/
	cpufunc_domains,		/* Domain			*/
	arm11_setttb,			/* Setttb			*/
	cpufunc_faultstatus,		/* Faultstatus			*/
	cpufunc_faultaddress,		/* Faultaddress			*/

	/* TLB functions */

	arm11_tlb_flushID,		/* tlb_flushID			*/
	arm11_tlb_flushID_SE,		/* tlb_flushID_SE		*/
	arm11_tlb_flushI,		/* tlb_flushI			*/
	arm11_tlb_flushI_SE,		/* tlb_flushI_SE		*/
	arm11_tlb_flushD,		/* tlb_flushD			*/
	arm11_tlb_flushD_SE,		/* tlb_flushD_SE		*/

	/* Cache operations */

	armv5_icache_sync_all,		/* icache_sync_all	*/
	armv5_icache_sync_range,	/* icache_sync_range	*/

	armv5_dcache_wbinv_all,		/* dcache_wbinv_all	*/
	armv5_dcache_wbinv_range,	/* dcache_wbinv_range	*/
/*XXX*/	armv5_dcache_wbinv_range,	/* dcache_inv_range	*/
	armv5_dcache_wb_range,		/* dcache_wb_range	*/

	armv5_idcache_wbinv_all,	/* idcache_wbinv_all	*/
	armv5_idcache_wbinv_range,	/* idcache_wbinv_range	*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	arm11_drain_writebuf,		/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	arm11_cpu_sleep,		/* sleep (wait for interrupt) */

	/* Soft functions */

	cpufunc_null_fixup,		/* dataabt_fixup	*/
	cpufunc_null_fixup,		/* prefetchabt_fixup	*/

	arm11_context_switch,		/* context_switch	*/

	arm11_setup			/* cpu setup		*/
};
#endif /* CPU_ARM11 */

#ifdef CPU_ARMv7
struct cpu_functions armv7_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	cpufunc_nullop,			/* cpwait		*/

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* Domain		*/
	armv7_setttb,			/* Setttb		*/
	cpufunc_faultstatus,		/* Faultstatus		*/
	cpufunc_faultaddress,		/* Faultaddress		*/

	/* TLB functions */

	armv7_tlb_flushID,		/* tlb_flushID		*/
	armv7_tlb_flushID_SE,		/* tlb_flushID_SE	*/
	armv7_tlb_flushI,		/* tlb_flushI		*/
	armv7_tlb_flushI_SE,		/* tlb_flushI_SE	*/
	armv7_tlb_flushD,		/* tlb_flushD		*/
	armv7_tlb_flushD_SE,		/* tlb_flushD_SE	*/

	/* Cache operations */

	armv7_icache_sync_all,		/* icache_sync_all	*/
	armv7_icache_sync_range,	/* icache_sync_range	*/

	armv7_dcache_wbinv_all,		/* dcache_wbinv_all	*/
	armv7_dcache_wbinv_range,	/* dcache_wbinv_range	*/
/*XXX*/	armv7_dcache_wbinv_range,	/* dcache_inv_range	*/
	armv7_dcache_wb_range,		/* dcache_wb_range	*/

	armv7_idcache_wbinv_all,	/* idcache_wbinv_all	*/
	armv7_idcache_wbinv_range,	/* idcache_wbinv_range	*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	armv7_drain_writebuf,		/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	armv7_cpu_sleep,		/* sleep (wait for interrupt) */

	/* Soft functions */

	cpufunc_null_fixup,		/* dataabt_fixup	*/
	cpufunc_null_fixup,		/* prefetchabt_fixup	*/

	armv7_context_switch,		/* context_switch	*/

	armv7_setup			/* cpu setup		*/
};
#endif /* CPU_ARMv7 */


#ifdef CPU_SA110
struct cpu_functions sa110_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	cpufunc_nullop,			/* cpwait		*/

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* domain		*/
	sa1_setttb,			/* setttb		*/
	cpufunc_faultstatus,		/* faultstatus		*/
	cpufunc_faultaddress,		/* faultaddress		*/

	/* TLB functions */

	armv4_tlb_flushID,		/* tlb_flushID		*/
	sa1_tlb_flushID_SE,		/* tlb_flushID_SE	*/
	armv4_tlb_flushI,		/* tlb_flushI		*/
	(void *)armv4_tlb_flushI,	/* tlb_flushI_SE	*/
	armv4_tlb_flushD,		/* tlb_flushD		*/
	armv4_tlb_flushD_SE,		/* tlb_flushD_SE	*/

	/* Cache operations */

	sa1_cache_syncI,		/* icache_sync_all	*/
	sa1_cache_syncI_rng,		/* icache_sync_range	*/

	sa1_cache_purgeD,		/* dcache_wbinv_all	*/
	sa1_cache_purgeD_rng,		/* dcache_wbinv_range	*/
/*XXX*/	sa1_cache_purgeD_rng,		/* dcache_inv_range	*/
	sa1_cache_cleanD_rng,		/* dcache_wb_range	*/

	sa1_cache_purgeID,		/* idcache_wbinv_all	*/
	sa1_cache_purgeID_rng,		/* idcache_wbinv_range	*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	armv4_drain_writebuf,		/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	(void *)cpufunc_nullop,		/* sleep		*/

	/* Soft functions */

	cpufunc_null_fixup,		/* dataabt_fixup	*/
	cpufunc_null_fixup,		/* prefetchabt_fixup	*/

	sa110_context_switch,		/* context_switch	*/

	sa110_setup			/* cpu setup		*/
};
#endif	/* CPU_SA110 */

#if defined(CPU_SA1100) || defined(CPU_SA1110)
struct cpu_functions sa11x0_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	cpufunc_nullop,			/* cpwait		*/

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* domain		*/
	sa1_setttb,			/* setttb		*/
	cpufunc_faultstatus,		/* faultstatus		*/
	cpufunc_faultaddress,		/* faultaddress		*/

	/* TLB functions */

	armv4_tlb_flushID,		/* tlb_flushID		*/
	sa1_tlb_flushID_SE,		/* tlb_flushID_SE	*/
	armv4_tlb_flushI,		/* tlb_flushI		*/
	(void *)armv4_tlb_flushI,	/* tlb_flushI_SE	*/
	armv4_tlb_flushD,		/* tlb_flushD		*/
	armv4_tlb_flushD_SE,		/* tlb_flushD_SE	*/

	/* Cache operations */

	sa1_cache_syncI,		/* icache_sync_all	*/
	sa1_cache_syncI_rng,		/* icache_sync_range	*/

	sa1_cache_purgeD,		/* dcache_wbinv_all	*/
	sa1_cache_purgeD_rng,		/* dcache_wbinv_range	*/
/*XXX*/	sa1_cache_purgeD_rng,		/* dcache_inv_range	*/
	sa1_cache_cleanD_rng,		/* dcache_wb_range	*/

	sa1_cache_purgeID,		/* idcache_wbinv_all	*/
	sa1_cache_purgeID_rng,		/* idcache_wbinv_range	*/

	/* Other functions */

	sa11x0_drain_readbuf,		/* flush_prefetchbuf	*/
	armv4_drain_writebuf,		/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	sa11x0_cpu_sleep,		/* sleep		*/

	/* Soft functions */

	cpufunc_null_fixup,		/* dataabt_fixup	*/
	cpufunc_null_fixup,		/* prefetchabt_fixup	*/

	sa11x0_context_switch,		/* context_switch	*/

	sa11x0_setup			/* cpu setup		*/
};
#endif	/* CPU_SA1100 || CPU_SA1110 */

#ifdef CPU_IXP12X0
struct cpu_functions ixp12x0_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	cpufunc_nullop,			/* cpwait		*/

	/* MMU functions */

	cpufunc_control,		/* control		*/
	cpufunc_domains,		/* domain		*/
	sa1_setttb,			/* setttb		*/
	cpufunc_faultstatus,		/* faultstatus		*/
	cpufunc_faultaddress,		/* faultaddress		*/

	/* TLB functions */

	armv4_tlb_flushID,		/* tlb_flushID		*/
	sa1_tlb_flushID_SE,		/* tlb_flushID_SE	*/
	armv4_tlb_flushI,		/* tlb_flushI		*/
	(void *)armv4_tlb_flushI,	/* tlb_flushI_SE	*/
	armv4_tlb_flushD,		/* tlb_flushD		*/
	armv4_tlb_flushD_SE,		/* tlb_flushD_SE	*/

	/* Cache operations */

	sa1_cache_syncI,		/* icache_sync_all	*/
	sa1_cache_syncI_rng,		/* icache_sync_range	*/

	sa1_cache_purgeD,		/* dcache_wbinv_all	*/
	sa1_cache_purgeD_rng,		/* dcache_wbinv_range	*/
/*XXX*/	sa1_cache_purgeD_rng,		/* dcache_inv_range	*/
	sa1_cache_cleanD_rng,		/* dcache_wb_range	*/

	sa1_cache_purgeID,		/* idcache_wbinv_all	*/
	sa1_cache_purgeID_rng,		/* idcache_wbinv_range	*/

	/* Other functions */

	ixp12x0_drain_readbuf,			/* flush_prefetchbuf	*/
	armv4_drain_writebuf,		/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	(void *)cpufunc_nullop,		/* sleep		*/

	/* Soft functions */

	cpufunc_null_fixup,		/* dataabt_fixup	*/
	cpufunc_null_fixup,		/* prefetchabt_fixup	*/

	ixp12x0_context_switch,		/* context_switch	*/

	ixp12x0_setup			/* cpu setup		*/
};
#endif	/* CPU_IXP12X0 */

#if defined(CPU_XSCALE_80200) || defined(CPU_XSCALE_80321) || \
    defined(CPU_XSCALE_PXA2X0) || defined(CPU_XSCALE_IXP425)
struct cpu_functions xscale_cpufuncs = {
	/* CPU functions */

	cpufunc_id,			/* id			*/
	xscale_cpwait,			/* cpwait		*/

	/* MMU functions */

	xscale_control,			/* control		*/
	cpufunc_domains,		/* domain		*/
	xscale_setttb,			/* setttb		*/
	cpufunc_faultstatus,		/* faultstatus		*/
	cpufunc_faultaddress,		/* faultaddress		*/

	/* TLB functions */

	armv4_tlb_flushID,		/* tlb_flushID		*/
	xscale_tlb_flushID_SE,		/* tlb_flushID_SE	*/
	armv4_tlb_flushI,		/* tlb_flushI		*/
	(void *)armv4_tlb_flushI,	/* tlb_flushI_SE	*/
	armv4_tlb_flushD,		/* tlb_flushD		*/
	armv4_tlb_flushD_SE,		/* tlb_flushD_SE	*/

	/* Cache operations */

	xscale_cache_syncI,		/* icache_sync_all	*/
	xscale_cache_syncI_rng,		/* icache_sync_range	*/

	xscale_cache_purgeD,		/* dcache_wbinv_all	*/
	xscale_cache_purgeD_rng,	/* dcache_wbinv_range	*/
	xscale_cache_flushD_rng,	/* dcache_inv_range	*/
	xscale_cache_cleanD_rng,	/* dcache_wb_range	*/

	xscale_cache_purgeID,		/* idcache_wbinv_all	*/
	xscale_cache_purgeID_rng,	/* idcache_wbinv_range	*/

	/* Other functions */

	cpufunc_nullop,			/* flush_prefetchbuf	*/
	armv4_drain_writebuf,		/* drain_writebuf	*/
	cpufunc_nullop,			/* flush_brnchtgt_C	*/
	(void *)cpufunc_nullop,		/* flush_brnchtgt_E	*/

	xscale_cpu_sleep,		/* sleep		*/

	/* Soft functions */

	cpufunc_null_fixup,		/* dataabt_fixup	*/
	cpufunc_null_fixup,		/* prefetchabt_fixup	*/

	xscale_context_switch,		/* context_switch	*/

	xscale_setup			/* cpu setup		*/
};
#endif
/* CPU_XSCALE_80200 || CPU_XSCALE_80321 || CPU_XSCALE_PXA2X0 || CPU_XSCALE_IXP425 */

/*
 * Global constants also used by locore.s
 */

struct cpu_functions cpufuncs;
u_int cputype;
u_int cpu_reset_needs_v4_MMU_disable;	/* flag used in locore.s */

#if defined(CPU_ARM7TDMI) || defined(CPU_ARM8) || defined(CPU_ARM9) || \
    defined(CPU_ARM9E) || defined(CPU_ARM10) || defined(CPU_ARM11) || \
    defined(CPU_XSCALE_80200) || defined(CPU_XSCALE_80321) || \
    defined(CPU_XSCALE_PXA2X0) || defined(CPU_XSCALE_IXP425)
static void get_cachetype_cp15 (void);

/* Additional cache information local to this file.  Log2 of some of the
   above numbers.  */
static int	arm_dcache_l2_nsets;
static int	arm_dcache_l2_assoc;
static int	arm_dcache_l2_linesize;

static void
get_cachetype_cp15()
{
	u_int ctype, isize, dsize;
	u_int multiplier;

	__asm __volatile("mrc p15, 0, %0, c0, c0, 1"
		: "=r" (ctype));

	/*
	 * ...and thus spake the ARM ARM:
	 *
	 * If an <opcode2> value corresponding to an unimplemented or
	 * reserved ID register is encountered, the System Control
	 * processor returns the value of the main ID register.
	 */
	if (ctype == cpufunc_id())
		goto out;

	if ((ctype & CPU_CT_S) == 0)
		arm_pcache_unified = 1;

	/*
	 * If you want to know how this code works, go read the ARM ARM.
	 */

	arm_pcache_type = CPU_CT_CTYPE(ctype);

	if (arm_pcache_unified == 0) {
		isize = CPU_CT_ISIZE(ctype);
		multiplier = (isize & CPU_CT_xSIZE_M) ? 3 : 2;
		arm_picache_line_size = 1U << (CPU_CT_xSIZE_LEN(isize) + 3);
		if (CPU_CT_xSIZE_ASSOC(isize) == 0) {
			if (isize & CPU_CT_xSIZE_M)
				arm_picache_line_size = 0; /* not present */
			else
				arm_picache_ways = 1;
		} else {
			arm_picache_ways = multiplier <<
			    (CPU_CT_xSIZE_ASSOC(isize) - 1);
		}
		arm_picache_size = multiplier << (CPU_CT_xSIZE_SIZE(isize) + 8);
	}

	dsize = CPU_CT_DSIZE(ctype);
	multiplier = (dsize & CPU_CT_xSIZE_M) ? 3 : 2;
	arm_pdcache_line_size = 1U << (CPU_CT_xSIZE_LEN(dsize) + 3);
	if (CPU_CT_xSIZE_ASSOC(dsize) == 0) {
		if (dsize & CPU_CT_xSIZE_M)
			arm_pdcache_line_size = 0; /* not present */
		else
			arm_pdcache_ways = 1;
	} else {
		arm_pdcache_ways = multiplier <<
		    (CPU_CT_xSIZE_ASSOC(dsize) - 1);
	}
	arm_pdcache_size = multiplier << (CPU_CT_xSIZE_SIZE(dsize) + 8);

	arm_dcache_align = arm_pdcache_line_size;

	arm_dcache_l2_assoc = CPU_CT_xSIZE_ASSOC(dsize) + multiplier - 2;
	arm_dcache_l2_linesize = CPU_CT_xSIZE_LEN(dsize) + 3;
	arm_dcache_l2_nsets = 6 + CPU_CT_xSIZE_SIZE(dsize) -
	    CPU_CT_xSIZE_ASSOC(dsize) - CPU_CT_xSIZE_LEN(dsize);

 out:
	arm_dcache_align_mask = arm_dcache_align - 1;
}
#endif /* ARM7TDMI || ARM8 || ARM9 || XSCALE */

#if defined(CPU_ARM2) || defined(CPU_ARM250) || defined(CPU_ARM3) || \
    defined(CPU_ARM6) || defined(CPU_ARM7) || defined(CPU_SA110) || \
    defined(CPU_SA1100) || defined(CPU_SA1110) || defined(CPU_IXP12X0)
/* Cache information for CPUs without cache type registers. */
struct cachetab {
	u_int32_t ct_cpuid;
	int	ct_pcache_type;
	int	ct_pcache_unified;
	int	ct_pdcache_size;
	int	ct_pdcache_line_size;
	int	ct_pdcache_ways;
	int	ct_picache_size;
	int	ct_picache_line_size;
	int	ct_picache_ways;
};

struct cachetab cachetab[] = {
    /* cpuid,		cache type,	  u,  dsiz, ls, wy,  isiz, ls, wy */
    { CPU_ID_ARM2,	0,		  1,    0,  0,  0,	0,  0,  0 },
    { CPU_ID_ARM250,    0,		  1,	0,  0,  0,	0,  0,  0 },
    { CPU_ID_ARM3,	CPU_CT_CTYPE_WT,  1,  4096, 16, 64,	0,  0,  0 },
    { CPU_ID_ARM610,	CPU_CT_CTYPE_WT,  1,  4096, 16, 64,	0,  0,  0 },
    { CPU_ID_ARM710,    CPU_CT_CTYPE_WT,  1,  8192, 32,  4,	0,  0,  0 },
    { CPU_ID_ARM7500,   CPU_CT_CTYPE_WT,  1,  4096, 16,  4,	0,  0,  0 },
    { CPU_ID_ARM710A,   CPU_CT_CTYPE_WT,  1,  8192, 16,  4,	0,  0,  0 },
    { CPU_ID_ARM7500FE, CPU_CT_CTYPE_WT,  1,  4096, 16,  4,	0,  0,  0 },
    /* XXX is this type right for SA-1? */
    { CPU_ID_SA110,	CPU_CT_CTYPE_WB1, 0, 16384, 32, 32, 16384, 32, 32 },
    { CPU_ID_SA1100,	CPU_CT_CTYPE_WB1, 0,  8192, 32, 32, 16384, 32, 32 },
    { CPU_ID_SA1110,	CPU_CT_CTYPE_WB1, 0,  8192, 32, 32, 16384, 32, 32 },
    { CPU_ID_IXP1200,	CPU_CT_CTYPE_WB1, 0, 16384, 32, 32, 16384, 32, 32 }, /* XXX */
    { 0, 0, 0, 0, 0, 0, 0, 0}
};

static void get_cachetype_table (void);

static void
get_cachetype_table()
{
	int i;
	u_int32_t cpuid = cpufunc_id();

	for (i = 0; cachetab[i].ct_cpuid != 0; i++) {
		if (cachetab[i].ct_cpuid == (cpuid & CPU_ID_CPU_MASK)) {
			arm_pcache_type = cachetab[i].ct_pcache_type;
			arm_pcache_unified = cachetab[i].ct_pcache_unified;
			arm_pdcache_size = cachetab[i].ct_pdcache_size;
			arm_pdcache_line_size =
			    cachetab[i].ct_pdcache_line_size;
			arm_pdcache_ways = cachetab[i].ct_pdcache_ways;
			arm_picache_size = cachetab[i].ct_picache_size;
			arm_picache_line_size =
			    cachetab[i].ct_picache_line_size;
			arm_picache_ways = cachetab[i].ct_picache_ways;
		}
	}
	arm_dcache_align = arm_pdcache_line_size;

	arm_dcache_align_mask = arm_dcache_align - 1;
}

#endif /* ARM2 || ARM250 || ARM3 || ARM6 || ARM7 || SA110 || SA1100 || SA1111 || IXP12X0 */

#ifdef CPU_ARMv7
void arm_get_cachetype_cp15v7 (void);
int	arm_dcache_l2_nsets;
int	arm_dcache_l2_assoc;
int	arm_dcache_l2_linesize;

static int
log2(int size)
{
	int i = 0;
	while (size != 0)
	for (i = 0; size != 0; i++)
		size >>= 1;
	return i;
}

void
arm_get_cachetype_cp15v7(void)
{
	uint32_t cachereg;
	uint32_t cache_level_id;
	uint32_t line_size, ways, sets, size;
	uint32_t sel;
	__asm __volatile("mrc p15, 1, %0, c0, c0, 1"
		: "=r" (cache_level_id) :);

	/* dcache L1 */
	sel = 0;
	__asm __volatile("mcr p15, 2, %0, c0, c0, 0"
		:: "r" (sel));
	__asm __volatile("mrc p15, 1, %0, c0, c0, 0"
		: "=r" (cachereg) :);
	line_size = 1 << ((cachereg & 7)+4);
	ways = ((0x00000ff8 & cachereg) >> 3) + 1;
	sets = ((0x0ffff000 & cachereg) >> 13) + 1;
	arm_pcache_unified = (cache_level_id & 0x7) == 2;
	arm_pdcache_line_size = line_size;
	arm_pdcache_ways = ways;
	size = line_size * ways * sets;
	arm_pdcache_size = size;

	switch (cachereg & 0xc0000000) {
	case 0x00000000:
		arm_pcache_type = 0;
	case 0x40000000:
		arm_pcache_type = CPU_CT_CTYPE_WT;
	case 0x80000000:
	case 0xc0000000:
		arm_pcache_type = CPU_CT_CTYPE_WB1;
	}

	/* icache L1 */
	sel = 1;
	__asm __volatile("mcr p15, 2, %0, c0, c0, 0"
		:: "r" (sel));
	__asm __volatile("mrc p15, 1, %0, c0, c0, 0"
		: "=r" (cachereg) :);
	line_size = 1 << ((cachereg & 7)+4);
	ways = ((0x00000ff8 & cachereg) >> 3) + 1;
	sets = ((0x0ffff000 & cachereg) >> 13) + 1;
	arm_picache_line_size = line_size;
	size = line_size * ways * sets;
	arm_picache_size = size;
	arm_picache_ways = ways;

	arm_dcache_align = arm_pdcache_line_size;

	arm_dcache_align_mask = arm_dcache_align - 1;

	/* ucache L2 */
	sel = 1;
	__asm __volatile("mcr p15, 2, %0, c0, c0, 0"
		:: "r" (sel));
	__asm __volatile("mrc p15, 1, %0, c0, c0, 0"
		: "=r" (cachereg) :);
	line_size = 1 << ((cachereg & 7)+4);
	ways = ((0x00000ff8 & cachereg) >> 3) + 1;
	sets = ((0x0ffff000 & cachereg) >> 13) + 1;
	arm_dcache_l2_nsets = log2(sets);
	arm_dcache_l2_assoc = log2(ways);
	arm_dcache_l2_linesize = log2(line_size);
}
#endif


/*
 * Cannot panic here as we may not have a console yet ...
 */

int
set_cpufuncs()
{
	cputype = cpufunc_id();
	cputype &= CPU_ID_CPU_MASK;

	/*
	 * NOTE: cpu_do_powersave defaults to off.  If we encounter a
	 * CPU type where we want to use it by default, then we set it.
	 */

#ifdef CPU_ARM3
	if ((cputype & CPU_ID_IMPLEMENTOR_MASK) == CPU_ID_ARM_LTD &&
	    (cputype & 0x00000f00) == 0x00000300) {
		cpufuncs = arm3_cpufuncs;
		cpu_reset_needs_v4_MMU_disable = 0;
		get_cachetype_table();
		return 0;
	}
#endif	/* CPU_ARM3 */
#ifdef CPU_ARM6
	if ((cputype & CPU_ID_IMPLEMENTOR_MASK) == CPU_ID_ARM_LTD &&
	    (cputype & 0x00000f00) == 0x00000600) {
		cpufuncs = arm6_cpufuncs;
		cpu_reset_needs_v4_MMU_disable = 0;
		get_cachetype_table();
		pmap_pte_init_generic();
		return 0;
	}
#endif	/* CPU_ARM6 */
#ifdef CPU_ARM7
	if ((cputype & CPU_ID_IMPLEMENTOR_MASK) == CPU_ID_ARM_LTD &&
	    CPU_ID_IS7(cputype) &&
	    (cputype & CPU_ID_7ARCH_MASK) == CPU_ID_7ARCH_V3) {
		cpufuncs = arm7_cpufuncs;
		cpu_reset_needs_v4_MMU_disable = 0;
		get_cachetype_table();
		pmap_pte_init_generic();
		return 0;
	}
#endif	/* CPU_ARM7 */
#ifdef CPU_ARM7TDMI
	if ((cputype & CPU_ID_IMPLEMENTOR_MASK) == CPU_ID_ARM_LTD &&
	    CPU_ID_IS7(cputype) &&
	    (cputype & CPU_ID_7ARCH_MASK) == CPU_ID_7ARCH_V4T) {
		cpufuncs = arm7tdmi_cpufuncs;
		cpu_reset_needs_v4_MMU_disable = 0;
		get_cachetype_cp15();
		pmap_pte_init_generic();
		return 0;
	}
#endif
#ifdef CPU_ARM8
	if ((cputype & CPU_ID_IMPLEMENTOR_MASK) == CPU_ID_ARM_LTD &&
	    (cputype & 0x0000f000) == 0x00008000) {
		cpufuncs = arm8_cpufuncs;
		cpu_reset_needs_v4_MMU_disable = 0;	/* XXX correct? */
		get_cachetype_cp15();
		pmap_pte_init_arm8();
		return 0;
	}
#endif	/* CPU_ARM8 */
#ifdef CPU_ARM9
	if (((cputype & CPU_ID_IMPLEMENTOR_MASK) == CPU_ID_ARM_LTD ||
	     (cputype & CPU_ID_IMPLEMENTOR_MASK) == CPU_ID_TI) &&
	    (cputype & 0x0000f000) == 0x00009000) {
		cpufuncs = arm9_cpufuncs;
		cpu_reset_needs_v4_MMU_disable = 1;	/* V4 or higher */
		get_cachetype_cp15();
		arm9_dcache_sets_inc = 1U << arm_dcache_l2_linesize;
		arm9_dcache_sets_max =
		    (1U << (arm_dcache_l2_linesize + arm_dcache_l2_nsets)) -
		    arm9_dcache_sets_inc;
		arm9_dcache_index_inc = 1U << (32 - arm_dcache_l2_assoc);
		arm9_dcache_index_max = 0U - arm9_dcache_index_inc;
		pmap_pte_init_arm9();
		return 0;
	}
#endif /* CPU_ARM9 */
#if defined(CPU_ARM9E) || defined(CPU_ARM10)
	if (cputype == CPU_ID_ARM926EJS || cputype == CPU_ID_ARM1026EJS) {
		cpufuncs = armv5_ec_cpufuncs;
		cpu_reset_needs_v4_MMU_disable = 1;	/* V4 or higher */
		get_cachetype_cp15();
		pmap_pte_init_generic();
		return 0;
	}
#endif /* CPU_ARM9E || CPU_ARM10 */
#ifdef CPU_ARM10
	if (/* cputype == CPU_ID_ARM1020T || */
	    cputype == CPU_ID_ARM1020E) {
		/*
		 * Select write-through cacheing (this isn't really an
		 * option on ARM1020T).
		 */
		cpufuncs = arm10_cpufuncs;
		cpu_reset_needs_v4_MMU_disable = 1;	/* V4 or higher */
		get_cachetype_cp15();
		armv5_dcache_sets_inc = 1U << arm_dcache_l2_linesize;
		armv5_dcache_sets_max =
		    (1U << (arm_dcache_l2_linesize + arm_dcache_l2_nsets)) -
		    armv5_dcache_sets_inc;
		armv5_dcache_index_inc = 1U << (32 - arm_dcache_l2_assoc);
		armv5_dcache_index_max = 0U - armv5_dcache_index_inc;
		pmap_pte_init_generic();
		return 0;
	}
#endif /* CPU_ARM10 */
#ifdef CPU_ARM11
	if (cputype == CPU_ID_ARM1136JS ||
	    cputype == CPU_ID_ARM1136JSR1 || 1) {
		cpufuncs = arm11_cpufuncs;
		cpu_reset_needs_v4_MMU_disable = 1;	/* V4 or higher */
		get_cachetype_cp15();
		arm11_dcache_sets_inc = 1U << arm_dcache_l2_linesize;
		arm11_dcache_sets_max =
		    (1U << (arm_dcache_l2_linesize + arm_dcache_l2_nsets)) -
		    arm11_dcache_sets_inc;
		arm11_dcache_index_inc = 1U << (32 - arm_dcache_l2_assoc);
		arm11_dcache_index_max = 0U - arm11_dcache_index_inc;
		pmap_pte_init_arm11();

		/* Use powersave on this CPU. */
		cpu_do_powersave = 1;
		return 0;
	}
#endif /* CPU_ARM11 */
#ifdef CPU_ARMv7
	if (cputype == CPU_ID_OMAP3430 || cputype == CPU_ID_OMAP3530) {
		cpufuncs = armv7_cpufuncs;
		cpu_reset_needs_v4_MMU_disable = 1;	/* V4 or higher */
		arm_get_cachetype_cp15v7();
		armv7_dcache_sets_inc = 1U << arm_dcache_l2_linesize;
		armv7_dcache_sets_max =
		    (1U << (arm_dcache_l2_linesize + arm_dcache_l2_nsets)) -
		    armv7_dcache_sets_inc;
		armv7_dcache_index_inc = 1U << (32 - arm_dcache_l2_assoc);
		armv7_dcache_index_max = 0U - armv7_dcache_index_inc;
		pmap_pte_init_armv7();

		/* Use powersave on this CPU. */
		cpu_do_powersave = 1;
		return 0;
	}
#endif /* CPU_ARMv7 */
#ifdef CPU_SA110
	if (cputype == CPU_ID_SA110) {
		cpufuncs = sa110_cpufuncs;
		cpu_reset_needs_v4_MMU_disable = 1;	/* SA needs it */
		get_cachetype_table();
		pmap_pte_init_sa1();
		return 0;
	}
#endif	/* CPU_SA110 */
#ifdef CPU_SA1100
	if (cputype == CPU_ID_SA1100) {
		cpufuncs = sa11x0_cpufuncs;
		cpu_reset_needs_v4_MMU_disable = 1;	/* SA needs it	*/
		get_cachetype_table();
		pmap_pte_init_sa1();

		/* Use powersave on this CPU. */
		cpu_do_powersave = 1;

		return 0;
	}
#endif	/* CPU_SA1100 */
#ifdef CPU_SA1110
	if (cputype == CPU_ID_SA1110) {
		cpufuncs = sa11x0_cpufuncs;
		cpu_reset_needs_v4_MMU_disable = 1;	/* SA needs it	*/
		get_cachetype_table();
		pmap_pte_init_sa1();

		/* Use powersave on this CPU. */
		cpu_do_powersave = 1;

		return 0;
	}
#endif	/* CPU_SA1110 */
#ifdef CPU_IXP12X0
	if (cputype == CPU_ID_IXP1200) {
		cpufuncs = ixp12x0_cpufuncs;
		cpu_reset_needs_v4_MMU_disable = 1;
		get_cachetype_table();
		pmap_pte_init_sa1();
		return 0;
	}
#endif  /* CPU_IXP12X0 */
#ifdef CPU_XSCALE_80200
	if (cputype == CPU_ID_80200) {
		int rev = cpufunc_id() & CPU_ID_REVISION_MASK;

		i80200_icu_init();

#ifdef PERFCTRS
		/*
		 * Reset the Performance Monitoring Unit to a
		 * pristine state:
		 *	- CCNT, PMN0, PMN1 reset to 0
		 *	- overflow indications cleared
		 *	- all counters disabled
		 */
		__asm __volatile("mcr p14, 0, %0, c0, c0, 0"
			:
			: "r" (PMNC_P|PMNC_C|PMNC_PMN0_IF|PMNC_PMN1_IF|
			       PMNC_CC_IF));
#endif /* PERFCTRS */

#if defined(XSCALE_CCLKCFG)
		/*
		 * Crank CCLKCFG to maximum legal value.
		 */
		__asm __volatile ("mcr p14, 0, %0, c6, c0, 0"
			:
			: "r" (XSCALE_CCLKCFG));
#endif

		/*
		 * XXX Disable ECC in the Bus Controller Unit; we
		 * don't really support it, yet.  Clear any pending
		 * error indications.
		 */
		__asm __volatile("mcr p13, 0, %0, c0, c1, 0"
			:
			: "r" (BCUCTL_E0|BCUCTL_E1|BCUCTL_EV));

		cpufuncs = xscale_cpufuncs;
#if defined(PERFCTRS)
		xscale_pmu_init();
#endif

		/*
		 * i80200 errata: Step-A0 and A1 have a bug where
		 * D$ dirty bits are not cleared on "invalidate by
		 * address".
		 *
		 * Workaround: Clean cache line before invalidating.
		 */
		if (rev == 0 || rev == 1)
			cpufuncs.cf_dcache_inv_range = xscale_cache_purgeD_rng;

		cpu_reset_needs_v4_MMU_disable = 1;	/* XScale needs it */
		get_cachetype_cp15();
		pmap_pte_init_xscale();
		return 0;
	}
#endif /* CPU_XSCALE_80200 */
#ifdef CPU_XSCALE_80321
	if (cputype == CPU_ID_80321_400 || cputype == CPU_ID_80321_600 ||
	    cputype == CPU_ID_80321_400_B0 || cputype == CPU_ID_80321_600_B0 ||
	    cputype == CPU_ID_80219_400 || cputype == CPU_ID_80219_600) {
		i80321intc_init();

#ifdef PERFCTRS
		/*
		 * Reset the Performance Monitoring Unit to a
		 * pristine state:
		 *	- CCNT, PMN0, PMN1 reset to 0
		 *	- overflow indications cleared
		 *	- all counters disabled
		 */
		__asm __volatile("mcr p14, 0, %0, c0, c0, 0"
			:
			: "r" (PMNC_P|PMNC_C|PMNC_PMN0_IF|PMNC_PMN1_IF|
			       PMNC_CC_IF));
#endif /* PERFCTRS */

		cpufuncs = xscale_cpufuncs;
#if defined(PERFCTRS)
		xscale_pmu_init();
#endif

		cpu_reset_needs_v4_MMU_disable = 1;	/* XScale needs it */
		get_cachetype_cp15();
		pmap_pte_init_xscale();
		return 0;
	}
#endif /* CPU_XSCALE_80321 */
#ifdef CPU_XSCALE_PXA2X0
	/* ignore core revision to test PXA2xx CPUs */
	if ((cputype & ~CPU_ID_XSCALE_COREREV_MASK) == CPU_ID_PXA250 ||
	    (cputype & ~CPU_ID_XSCALE_COREREV_MASK) == CPU_ID_PXA27X ||
	    (cputype & ~CPU_ID_XSCALE_COREREV_MASK) == CPU_ID_PXA210) {

		cpufuncs = xscale_cpufuncs;
#if defined(PERFCTRS)
		xscale_pmu_init();
#endif

		cpu_reset_needs_v4_MMU_disable = 1;	/* XScale needs it */
		get_cachetype_cp15();
		pmap_pte_init_xscale();

		/* Use powersave on this CPU. */
		cpu_do_powersave = 1;

		return 0;
	}
#endif /* CPU_XSCALE_PXA2X0 */
#ifdef CPU_XSCALE_IXP425
	if (cputype == CPU_ID_IXP425_533 || cputype == CPU_ID_IXP425_400 ||
	    cputype == CPU_ID_IXP425_266) {
		ixp425_icu_init();

		cpufuncs = xscale_cpufuncs;
#if defined(PERFCTRS)
		xscale_pmu_init();
#endif

		cpu_reset_needs_v4_MMU_disable = 1;	/* XScale needs it */
		get_cachetype_cp15();
		pmap_pte_init_xscale();

		return 0;
	}
#endif /* CPU_XSCALE_IXP425 */
	/*
	 * Bzzzz. And the answer was ...
	 */
	panic("No support for this CPU type (%08x) in kernel", cputype);
	return(ARCHITECTURE_NOT_PRESENT);
}

/*
 * Fixup routines for data and prefetch aborts.
 *
 * Several compile time symbols are used
 *
 * DEBUG_FAULT_CORRECTION - Print debugging information during the
 * correction of registers after a fault.
 * ARM6_LATE_ABORT - ARM6 supports both early and late aborts
 * when defined should use late aborts
 */


/*
 * Null abort fixup routine.
 * For use when no fixup is required.
 */
int
cpufunc_null_fixup(arg)
	void *arg;
{
	return(ABORT_FIXUP_OK);
}


#if defined(CPU_ARM2) || defined(CPU_ARM250) || defined(CPU_ARM3) || \
    defined(CPU_ARM6) || defined(CPU_ARM7) || defined(CPU_ARM7TDMI)

#ifdef DEBUG_FAULT_CORRECTION
#define DFC_PRINTF(x)		printf x
#define DFC_DISASSEMBLE(x)	disassemble(x)
#else
#define DFC_PRINTF(x)		/* nothing */
#define DFC_DISASSEMBLE(x)	/* nothing */
#endif

/*
 * "Early" data abort fixup.
 *
 * For ARM2, ARM2as, ARM3 and ARM6 (in early-abort mode).  Also used
 * indirectly by ARM6 (in late-abort mode) and ARM7[TDMI].
 *
 * In early aborts, we may have to fix up LDM, STM, LDC and STC.
 */
int
early_abort_fixup(arg)
	void *arg;
{
	trapframe_t *frame = arg;
	u_int fault_pc;
	u_int fault_instruction;
	int saved_lr = 0;

	if ((frame->tf_spsr & PSR_MODE) == PSR_SVC32_MODE) {

		/* Ok an abort in SVC mode */

		/*
		 * Copy the SVC r14 into the usr r14 - The usr r14 is garbage
		 * as the fault happened in svc mode but we need it in the
		 * usr slot so we can treat the registers as an array of ints
		 * during fixing.
		 * NOTE: This PC is in the position but writeback is not
		 * allowed on r15.
		 * Doing it like this is more efficient than trapping this
		 * case in all possible locations in the following fixup code.
		 */

		saved_lr = frame->tf_usr_lr;
		frame->tf_usr_lr = frame->tf_svc_lr;

		/*
		 * Note the trapframe does not have the SVC r13 so a fault
		 * from an instruction with writeback to r13 in SVC mode is
		 * not allowed. This should not happen as the kstack is
		 * always valid.
		 */
	}

	/* Get fault address and status from the CPU */

	fault_pc = frame->tf_pc;
	fault_instruction = *((volatile unsigned int *)fault_pc);

	/* Decode the fault instruction and fix the registers as needed */

	if ((fault_instruction & 0x0e000000) == 0x08000000) {
		int base;
		int loop;
		int count;
		int *registers = &frame->tf_r0;

		DFC_PRINTF(("LDM/STM\n"));
		DFC_DISASSEMBLE(fault_pc);
		if (fault_instruction & (1 << 21)) {
			DFC_PRINTF(("This instruction must be corrected\n"));
			base = (fault_instruction >> 16) & 0x0f;
			if (base == 15)
				return ABORT_FIXUP_FAILED;
			/* Count registers transferred */
			count = 0;
			for (loop = 0; loop < 16; ++loop) {
				if (fault_instruction & (1<<loop))
					++count;
			}
			DFC_PRINTF(("%d registers used\n", count));
			DFC_PRINTF(("Corrected r%d by %d bytes ",
			    base, count * 4));
			if (fault_instruction & (1 << 23)) {
				DFC_PRINTF(("down\n"));
				registers[base] -= count * 4;
			} else {
				DFC_PRINTF(("up\n"));
				registers[base] += count * 4;
			}
		}
	} else if ((fault_instruction & 0x0e000000) == 0x0c000000) {
		int base;
		int offset;
		int *registers = &frame->tf_r0;

		/* REGISTER CORRECTION IS REQUIRED FOR THESE INSTRUCTIONS */

		DFC_DISASSEMBLE(fault_pc);

		/* Only need to fix registers if write back is turned on */

		if ((fault_instruction & (1 << 21)) != 0) {
			base = (fault_instruction >> 16) & 0x0f;
			if (base == 13 &&
			    (frame->tf_spsr & PSR_MODE) == PSR_SVC32_MODE)
				return ABORT_FIXUP_FAILED;
			if (base == 15)
				return ABORT_FIXUP_FAILED;

			offset = (fault_instruction & 0xff) << 2;
			DFC_PRINTF(("r%d=%08x\n", base, registers[base]));
			if ((fault_instruction & (1 << 23)) != 0)
				offset = -offset;
			registers[base] += offset;
			DFC_PRINTF(("r%d=%08x\n", base, registers[base]));
		}
	} else if ((fault_instruction & 0x0e000000) == 0x0c000000)
		return ABORT_FIXUP_FAILED;

	if ((frame->tf_spsr & PSR_MODE) == PSR_SVC32_MODE) {

		/* Ok an abort in SVC mode */

		/*
		 * Copy the SVC r14 into the usr r14 - The usr r14 is garbage
		 * as the fault happened in svc mode but we need it in the
		 * usr slot so we can treat the registers as an array of ints
		 * during fixing.
		 * NOTE: This PC is in the position but writeback is not
		 * allowed on r15.
		 * Doing it like this is more efficient than trapping this
		 * case in all possible locations in the prior fixup code.
		 */

		frame->tf_svc_lr = frame->tf_usr_lr;
		frame->tf_usr_lr = saved_lr;

		/*
		 * Note the trapframe does not have the SVC r13 so a fault
		 * from an instruction with writeback to r13 in SVC mode is
		 * not allowed. This should not happen as the kstack is
		 * always valid.
		 */
	}

	return(ABORT_FIXUP_OK);
}
#endif	/* CPU_ARM2/250/3/6/7 */


#if (defined(CPU_ARM6) && defined(ARM6_LATE_ABORT)) || defined(CPU_ARM7) || \
	defined(CPU_ARM7TDMI)
/*
 * "Late" (base updated) data abort fixup
 *
 * For ARM6 (in late-abort mode) and ARM7.
 *
 * In this model, all data-transfer instructions need fixing up.  We defer
 * LDM, STM, LDC and STC fixup to the early-abort handler.
 */
int
late_abort_fixup(arg)
	void *arg;
{
	trapframe_t *frame = arg;
	u_int fault_pc;
	u_int fault_instruction;
	int saved_lr = 0;

	if ((frame->tf_spsr & PSR_MODE) == PSR_SVC32_MODE) {

		/* Ok an abort in SVC mode */

		/*
		 * Copy the SVC r14 into the usr r14 - The usr r14 is garbage
		 * as the fault happened in svc mode but we need it in the
		 * usr slot so we can treat the registers as an array of ints
		 * during fixing.
		 * NOTE: This PC is in the position but writeback is not
		 * allowed on r15.
		 * Doing it like this is more efficient than trapping this
		 * case in all possible locations in the following fixup code.
		 */

		saved_lr = frame->tf_usr_lr;
		frame->tf_usr_lr = frame->tf_svc_lr;

		/*
		 * Note the trapframe does not have the SVC r13 so a fault
		 * from an instruction with writeback to r13 in SVC mode is
		 * not allowed. This should not happen as the kstack is
		 * always valid.
		 */
	}

	/* Get fault address and status from the CPU */

	fault_pc = frame->tf_pc;
	fault_instruction = *((volatile unsigned int *)fault_pc);

	/* Decode the fault instruction and fix the registers as needed */

	/* Was is a swap instruction ? */

	if ((fault_instruction & 0x0fb00ff0) == 0x01000090) {
		DFC_DISASSEMBLE(fault_pc);
	} else if ((fault_instruction & 0x0c000000) == 0x04000000) {

		/* Was is a ldr/str instruction */
		/* This is for late abort only */

		int base;
		int offset;
		int *registers = &frame->tf_r0;

		DFC_DISASSEMBLE(fault_pc);

		/* This is for late abort only */

		if ((fault_instruction & (1 << 24)) == 0
		    || (fault_instruction & (1 << 21)) != 0) {
			/* postindexed ldr/str with no writeback */

			base = (fault_instruction >> 16) & 0x0f;
			if (base == 13 &&
			    (frame->tf_spsr & PSR_MODE) == PSR_SVC32_MODE)
				return ABORT_FIXUP_FAILED;
			if (base == 15)
				return ABORT_FIXUP_FAILED;
			DFC_PRINTF(("late abt fix: r%d=%08x : ",
			    base, registers[base]));
			if ((fault_instruction & (1 << 25)) == 0) {
				/* Immediate offset - easy */

				offset = fault_instruction & 0xfff;
				if ((fault_instruction & (1 << 23)))
					offset = -offset;
				registers[base] += offset;
				DFC_PRINTF(("imm=%08x ", offset));
			} else {
				/* offset is a shifted register */
				int shift;

				offset = fault_instruction & 0x0f;
				if (offset == base)
					return ABORT_FIXUP_FAILED;

				/*
				 * Register offset - hard we have to
				 * cope with shifts !
				 */
				offset = registers[offset];

				if ((fault_instruction & (1 << 4)) == 0)
					/* shift with amount */
					shift = (fault_instruction >> 7) & 0x1f;
				else {
					/* shift with register */
					if ((fault_instruction & (1 << 7)) != 0)
						/* undefined for now so bail out */
						return ABORT_FIXUP_FAILED;
					shift = ((fault_instruction >> 8) & 0xf);
					if (base == shift)
						return ABORT_FIXUP_FAILED;
					DFC_PRINTF(("shift reg=%d ", shift));
					shift = registers[shift];
				}
				DFC_PRINTF(("shift=%08x ", shift));
				switch (((fault_instruction >> 5) & 0x3)) {
				case 0 : /* Logical left */
					offset = (int)(((u_int)offset) << shift);
					break;
				case 1 : /* Logical Right */
					if (shift == 0) shift = 32;
					offset = (int)(((u_int)offset) >> shift);
					break;
				case 2 : /* Arithmetic Right */
					if (shift == 0) shift = 32;
					offset = (int)(((int)offset) >> shift);
					break;
				case 3 : /* Rotate right (rol or rxx) */
					return ABORT_FIXUP_FAILED;
					break;
				}

				DFC_PRINTF(("abt: fixed LDR/STR with "
				    "register offset\n"));
				if ((fault_instruction & (1 << 23)))
					offset = -offset;
				DFC_PRINTF(("offset=%08x ", offset));
				registers[base] += offset;
			}
			DFC_PRINTF(("r%d=%08x\n", base, registers[base]));
		}
	}

	if ((frame->tf_spsr & PSR_MODE) == PSR_SVC32_MODE) {

		/* Ok an abort in SVC mode */

		/*
		 * Copy the SVC r14 into the usr r14 - The usr r14 is garbage
		 * as the fault happened in svc mode but we need it in the
		 * usr slot so we can treat the registers as an array of ints
		 * during fixing.
		 * NOTE: This PC is in the position but writeback is not
		 * allowed on r15.
		 * Doing it like this is more efficient than trapping this
		 * case in all possible locations in the prior fixup code.
		 */

		frame->tf_svc_lr = frame->tf_usr_lr;
		frame->tf_usr_lr = saved_lr;

		/*
		 * Note the trapframe does not have the SVC r13 so a fault
		 * from an instruction with writeback to r13 in SVC mode is
		 * not allowed. This should not happen as the kstack is
		 * always valid.
		 */
	}

	/*
	 * Now let the early-abort fixup routine have a go, in case it
	 * was an LDM, STM, LDC or STC that faulted.
	 */

	return early_abort_fixup(arg);
}
#endif	/* CPU_ARM6(LATE)/7/7TDMI */

/*
 * CPU Setup code
 */

#if defined(CPU_ARM6) || defined(CPU_ARM7) || defined(CPU_ARM7TDMI) || \
	defined(CPU_ARM8) || defined (CPU_ARM9) || defined (CPU_ARM9E) || \
	defined(CPU_SA110) || defined(CPU_SA1100) || defined(CPU_SA1110) || \
	defined(CPU_XSCALE_80200) || defined(CPU_XSCALE_80321) || \
	defined(CPU_XSCALE_PXA2X0) || defined(CPU_XSCALE_IXP425) || \
	defined(CPU_ARM10) | defined(CPU_ARM11) || defined(CPU_ARMv7)

#define IGN	0
#define OR	1
#define BIC	2

struct cpu_option {
	char	*co_name;
	int	co_falseop;
	int	co_trueop;
	int	co_value;
};

static u_int parse_cpu_options (char *, struct cpu_option *, u_int);

static u_int
parse_cpu_options(args, optlist, cpuctrl)
	char *args;
	struct cpu_option *optlist;
	u_int cpuctrl;
{
	int integer;

	if (args == NULL)
		return(cpuctrl);

	while (optlist->co_name) {
		if (get_bootconf_option(args, optlist->co_name,
		    BOOTOPT_TYPE_BOOLEAN, &integer)) {
			if (integer) {
				if (optlist->co_trueop == OR)
					cpuctrl |= optlist->co_value;
				else if (optlist->co_trueop == BIC)
					cpuctrl &= ~optlist->co_value;
			} else {
				if (optlist->co_falseop == OR)
					cpuctrl |= optlist->co_value;
				else if (optlist->co_falseop == BIC)
					cpuctrl &= ~optlist->co_value;
			}
		}
		++optlist;
	}
	return(cpuctrl);
}
#endif /* CPU_ARM6 || CPU_ARM7 || CPU_ARM7TDMI || CPU_ARM8 || CPU_SA110 */

#if defined (CPU_ARM6) || defined(CPU_ARM7) || defined(CPU_ARM7TDMI) \
	|| defined(CPU_ARM8)
struct cpu_option arm678_options[] = {
	{ "cpu.cache",		BIC, OR,  CPU_CONTROL_IDC_ENABLE },
	{ "cpu.nocache",	OR,  BIC, CPU_CONTROL_IDC_ENABLE },
	{ "cpu.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ "cpu.nowritebuf",	OR,  BIC, CPU_CONTROL_WBUF_ENABLE },
	{ NULL,			IGN, IGN, 0 }
};

#endif	/* CPU_ARM6 || CPU_ARM7 || CPU_ARM7TDMI || CPU_ARM8 */

#ifdef CPU_ARM6
struct cpu_option arm6_options[] = {
	{ "arm6.cache",		BIC, OR,  CPU_CONTROL_IDC_ENABLE },
	{ "arm6.nocache",	OR,  BIC, CPU_CONTROL_IDC_ENABLE },
	{ "arm6.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ "arm6.nowritebuf",	OR,  BIC, CPU_CONTROL_WBUF_ENABLE },
	{ NULL,			IGN, IGN, 0 }
};

void
arm6_setup(args)
	char *args;
{
	int cpuctrl, cpuctrlmask;

	/* Set up default control registers bits */
	cpuctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IDC_ENABLE | CPU_CONTROL_WBUF_ENABLE;
	cpuctrlmask = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IDC_ENABLE | CPU_CONTROL_WBUF_ENABLE
		 | CPU_CONTROL_ROM_ENABLE | CPU_CONTROL_BEND_ENABLE
		 | CPU_CONTROL_AFLT_ENABLE;

#ifdef ARM6_LATE_ABORT
	cpuctrl |= CPU_CONTROL_LABT_ENABLE;
#endif	/* ARM6_LATE_ABORT */

#ifndef ARM32_DISABLE_ALIGNMENT_FAULTS
	cpuctrl |= CPU_CONTROL_AFLT_ENABLE;
#endif

	cpuctrl = parse_cpu_options(args, arm678_options, cpuctrl);
	cpuctrl = parse_cpu_options(args, arm6_options, cpuctrl);

#ifdef __ARMEB__
	cpuctrl |= CPU_CONTROL_BEND_ENABLE;
#endif

	/* Clear out the cache */
	cpu_idcache_wbinv_all();

	/* Set the control register */
	curcpu()->ci_ctrl = cpuctrl;
	cpu_control(0xffffffff, cpuctrl);
}
#endif	/* CPU_ARM6 */

#ifdef CPU_ARM7
struct cpu_option arm7_options[] = {
	{ "arm7.cache",		BIC, OR,  CPU_CONTROL_IDC_ENABLE },
	{ "arm7.nocache",	OR,  BIC, CPU_CONTROL_IDC_ENABLE },
	{ "arm7.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ "arm7.nowritebuf",	OR,  BIC, CPU_CONTROL_WBUF_ENABLE },
	{ "arm700.fpaclk",	BIC, OR,  CPU_CONTROL_CPCLK },
	{ NULL,			IGN, IGN, 0 }
};

void
arm7_setup(args)
	char *args;
{
	int cpuctrl, cpuctrlmask;

	cpuctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IDC_ENABLE | CPU_CONTROL_WBUF_ENABLE;
	cpuctrlmask = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IDC_ENABLE | CPU_CONTROL_WBUF_ENABLE
		 | CPU_CONTROL_CPCLK | CPU_CONTROL_LABT_ENABLE
		 | CPU_CONTROL_ROM_ENABLE | CPU_CONTROL_BEND_ENABLE
		 | CPU_CONTROL_AFLT_ENABLE;

#ifndef ARM32_DISABLE_ALIGNMENT_FAULTS
	cpuctrl |= CPU_CONTROL_AFLT_ENABLE;
#endif

	cpuctrl = parse_cpu_options(args, arm678_options, cpuctrl);
	cpuctrl = parse_cpu_options(args, arm7_options, cpuctrl);

#ifdef __ARMEB__
	cpuctrl |= CPU_CONTROL_BEND_ENABLE;
#endif

	/* Clear out the cache */
	cpu_idcache_wbinv_all();

	/* Set the control register */
	curcpu()->ci_ctrl = cpuctrl;
	cpu_control(0xffffffff, cpuctrl);
}
#endif	/* CPU_ARM7 */

#ifdef CPU_ARM7TDMI
struct cpu_option arm7tdmi_options[] = {
	{ "arm7.cache",		BIC, OR,  CPU_CONTROL_IDC_ENABLE },
	{ "arm7.nocache",	OR,  BIC, CPU_CONTROL_IDC_ENABLE },
	{ "arm7.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ "arm7.nowritebuf",	OR,  BIC, CPU_CONTROL_WBUF_ENABLE },
	{ "arm700.fpaclk",	BIC, OR,  CPU_CONTROL_CPCLK },
	{ NULL,			IGN, IGN, 0 }
};

void
arm7tdmi_setup(args)
	char *args;
{
	int cpuctrl;

	cpuctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IDC_ENABLE | CPU_CONTROL_WBUF_ENABLE;

	cpuctrl = parse_cpu_options(args, arm678_options, cpuctrl);
	cpuctrl = parse_cpu_options(args, arm7tdmi_options, cpuctrl);

#ifdef __ARMEB__
	cpuctrl |= CPU_CONTROL_BEND_ENABLE;
#endif

	/* Clear out the cache */
	cpu_idcache_wbinv_all();

	/* Set the control register */
	curcpu()->ci_ctrl = cpuctrl;
	cpu_control(0xffffffff, cpuctrl);
}
#endif	/* CPU_ARM7TDMI */

#ifdef CPU_ARM8
struct cpu_option arm8_options[] = {
	{ "arm8.cache",		BIC, OR,  CPU_CONTROL_IDC_ENABLE },
	{ "arm8.nocache",	OR,  BIC, CPU_CONTROL_IDC_ENABLE },
	{ "arm8.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ "arm8.nowritebuf",	OR,  BIC, CPU_CONTROL_WBUF_ENABLE },
	{ "cpu.branchpredict", 	BIC, OR,  CPU_CONTROL_BPRD_ENABLE },
	{ "arm8.branchpredict",	BIC, OR,  CPU_CONTROL_BPRD_ENABLE },
	{ NULL,			IGN, IGN, 0 }
};

void
arm8_setup(args)
	char *args;
{
	int integer;
	int cpuctrl, cpuctrlmask;
	int clocktest;
	int setclock = 0;

	cpuctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IDC_ENABLE | CPU_CONTROL_WBUF_ENABLE;
	cpuctrlmask = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IDC_ENABLE | CPU_CONTROL_WBUF_ENABLE
		 | CPU_CONTROL_BPRD_ENABLE | CPU_CONTROL_ROM_ENABLE
		 | CPU_CONTROL_BEND_ENABLE | CPU_CONTROL_AFLT_ENABLE;

#ifndef ARM32_DISABLE_ALIGNMENT_FAULTS
	cpuctrl |= CPU_CONTROL_AFLT_ENABLE;
#endif

	cpuctrl = parse_cpu_options(args, arm678_options, cpuctrl);
	cpuctrl = parse_cpu_options(args, arm8_options, cpuctrl);

#ifdef __ARMEB__
	cpuctrl |= CPU_CONTROL_BEND_ENABLE;
#endif

	/* Get clock configuration */
	clocktest = arm8_clock_config(0, 0) & 0x0f;

	/* Special ARM8 clock and test configuration */
	if (get_bootconf_option(args, "arm8.clock.reset", BOOTOPT_TYPE_BOOLEAN, &integer)) {
		clocktest = 0;
		setclock = 1;
	}
	if (get_bootconf_option(args, "arm8.clock.dynamic", BOOTOPT_TYPE_BOOLEAN, &integer)) {
		if (integer)
			clocktest |= 0x01;
		else
			clocktest &= ~(0x01);
		setclock = 1;
	}
	if (get_bootconf_option(args, "arm8.clock.sync", BOOTOPT_TYPE_BOOLEAN, &integer)) {
		if (integer)
			clocktest |= 0x02;
		else
			clocktest &= ~(0x02);
		setclock = 1;
	}
	if (get_bootconf_option(args, "arm8.clock.fast", BOOTOPT_TYPE_BININT, &integer)) {
		clocktest = (clocktest & ~0xc0) | (integer & 3) << 2;
		setclock = 1;
	}
	if (get_bootconf_option(args, "arm8.test", BOOTOPT_TYPE_BININT, &integer)) {
		clocktest |= (integer & 7) << 5;
		setclock = 1;
	}

	/* Clear out the cache */
	cpu_idcache_wbinv_all();

	/* Set the control register */
	curcpu()->ci_ctrl = cpuctrl;
	cpu_control(0xffffffff, cpuctrl);

	/* Set the clock/test register */
	if (setclock)
		arm8_clock_config(0x7f, clocktest);
}
#endif	/* CPU_ARM8 */

#ifdef CPU_ARM9
struct cpu_option arm9_options[] = {
	{ "cpu.cache",		BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "cpu.nocache",	OR,  BIC, (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "arm9.cache",	BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "arm9.icache",	BIC, OR,  CPU_CONTROL_IC_ENABLE },
	{ "arm9.dcache",	BIC, OR,  CPU_CONTROL_DC_ENABLE },
	{ "cpu.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ "cpu.nowritebuf",	OR,  BIC, CPU_CONTROL_WBUF_ENABLE },
	{ "arm9.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ NULL,			IGN, IGN, 0 }
};

void
arm9_setup(args)
	char *args;
{
	int cpuctrl, cpuctrlmask;

	cpuctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
	    | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
	    | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
	    | CPU_CONTROL_WBUF_ENABLE;
	cpuctrlmask = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
		 | CPU_CONTROL_WBUF_ENABLE | CPU_CONTROL_ROM_ENABLE
		 | CPU_CONTROL_BEND_ENABLE | CPU_CONTROL_AFLT_ENABLE
		 | CPU_CONTROL_LABT_ENABLE | CPU_CONTROL_VECRELOC
		 | CPU_CONTROL_ROUNDROBIN;

#ifndef ARM32_DISABLE_ALIGNMENT_FAULTS
	cpuctrl |= CPU_CONTROL_AFLT_ENABLE;
#endif

	cpuctrl = parse_cpu_options(args, arm9_options, cpuctrl);

#ifdef __ARMEB__
	cpuctrl |= CPU_CONTROL_BEND_ENABLE;
#endif

	if (vector_page == ARM_VECTORS_HIGH)
		cpuctrl |= CPU_CONTROL_VECRELOC;

	/* Clear out the cache */
	cpu_idcache_wbinv_all();

	/* Set the control register */
	curcpu()->ci_ctrl = cpuctrl;
	cpu_control(cpuctrlmask, cpuctrl);

}
#endif	/* CPU_ARM9 */

#if defined(CPU_ARM9E) || defined(CPU_ARM10)
struct cpu_option arm10_options[] = {
	{ "cpu.cache",		BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "cpu.nocache",	OR,  BIC, (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "arm10.cache",	BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "arm10.icache",	BIC, OR,  CPU_CONTROL_IC_ENABLE },
	{ "arm10.dcache",	BIC, OR,  CPU_CONTROL_DC_ENABLE },
	{ "cpu.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ "cpu.nowritebuf",	OR,  BIC, CPU_CONTROL_WBUF_ENABLE },
	{ "arm10.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ NULL,			IGN, IGN, 0 }
};

void
arm10_setup(args)
	char *args;
{
	int cpuctrl, cpuctrlmask;

	cpuctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_SYST_ENABLE
	    | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
	    | CPU_CONTROL_WBUF_ENABLE | CPU_CONTROL_BPRD_ENABLE;
	cpuctrlmask = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_SYST_ENABLE
	    | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
	    | CPU_CONTROL_WBUF_ENABLE | CPU_CONTROL_ROM_ENABLE
	    | CPU_CONTROL_BEND_ENABLE | CPU_CONTROL_AFLT_ENABLE
	    | CPU_CONTROL_BPRD_ENABLE
	    | CPU_CONTROL_ROUNDROBIN | CPU_CONTROL_CPCLK;

#ifndef ARM32_DISABLE_ALIGNMENT_FAULTS
	cpuctrl |= CPU_CONTROL_AFLT_ENABLE;
#endif

	cpuctrl = parse_cpu_options(args, arm10_options, cpuctrl);

#ifdef __ARMEB__
	cpuctrl |= CPU_CONTROL_BEND_ENABLE;
#endif

	if (vector_page == ARM_VECTORS_HIGH)
		cpuctrl |= CPU_CONTROL_VECRELOC;

	/* Clear out the cache */
	cpu_idcache_wbinv_all();

	/* Now really make sure they are clean.  */
	__asm __volatile ("mcr\tp15, 0, r0, c7, c7, 0" : : );

	/* Allow detection code to find the VFP if it's fitted.  */
	__asm __volatile ("mcr\tp15, 0, %0, c1, c0, 2" : : "r" (0x0fffffff));

	/* Set the control register */
	curcpu()->ci_ctrl = cpuctrl;
	cpu_control(0xffffffff, cpuctrl);

	/* And again. */
	cpu_idcache_wbinv_all();
}
#endif	/* CPU_ARM9E || CPU_ARM10 */

#ifdef CPU_ARM11
struct cpu_option arm11_options[] = {
	{ "cpu.cache",		BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "cpu.nocache",	OR,  BIC, (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "arm11.cache",	BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "arm11.icache",	BIC, OR,  CPU_CONTROL_IC_ENABLE },
	{ "arm11.dcache",	BIC, OR,  CPU_CONTROL_DC_ENABLE },
	{ NULL,			IGN, IGN, 0 }
};

void
arm11_setup(args)
	char *args;
{
	int cpuctrl, cpuctrlmask;

	cpuctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_SYST_ENABLE
	    | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
	    /* | CPU_CONTROL_BPRD_ENABLE */;
	cpuctrlmask = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_SYST_ENABLE
	    | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
	    | CPU_CONTROL_ROM_ENABLE | CPU_CONTROL_BPRD_ENABLE
	    | CPU_CONTROL_BEND_ENABLE | CPU_CONTROL_AFLT_ENABLE
	    | CPU_CONTROL_ROUNDROBIN | CPU_CONTROL_CPCLK;

#ifndef ARM32_DISABLE_ALIGNMENT_FAULTS
	cpuctrl |= CPU_CONTROL_AFLT_ENABLE;
#endif

	cpuctrl = parse_cpu_options(args, arm11_options, cpuctrl);

#ifdef __ARMEB__
	cpuctrl |= CPU_CONTROL_BEND_ENABLE;
#endif

	/* Clear out the cache */
	cpu_idcache_wbinv_all();

	/* Now really make sure they are clean.  */
	asm volatile ("mcr\tp15, 0, r0, c7, c7, 0" : : );

	/* Set the control register */
	curcpu()->ci_ctrl = cpuctrl;
	cpu_control(0xffffffff, cpuctrl);

	/* And again. */
	cpu_idcache_wbinv_all();
}
#endif	/* CPU_ARM11 */

#ifdef CPU_ARMv7
struct cpu_option armv7_options[] = {
	{ "cpu.cache",		BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "cpu.nocache",	OR,  BIC, (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "armv7.cache",	BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "armv7.icache",	BIC, OR,  CPU_CONTROL_IC_ENABLE },
	{ "armv7.dcache",	BIC, OR,  CPU_CONTROL_DC_ENABLE },
	{ NULL,			IGN, IGN, 0 }
};

void
armv7_setup(args)
	char *args;
{
	int cpuctrl, cpuctrlmask;

	cpuctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_SYST_ENABLE
	    | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
	    /* | CPU_CONTROL_BPRD_ENABLE */;
	cpuctrlmask = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_SYST_ENABLE
	    | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
	    | CPU_CONTROL_ROM_ENABLE | CPU_CONTROL_BPRD_ENABLE
	    | CPU_CONTROL_BEND_ENABLE | CPU_CONTROL_AFLT_ENABLE
	    | CPU_CONTROL_ROUNDROBIN | CPU_CONTROL_CPCLK;

#ifndef ARM32_DISABLE_ALIGNMENT_FAULTS
	cpuctrl |= CPU_CONTROL_AFLT_ENABLE;
#endif

	cpuctrl = parse_cpu_options(args, armv7_options, cpuctrl);

#ifdef __ARMEB__
	cpuctrl |= CPU_CONTROL_BEND_ENABLE;
#endif

	/* Clear out the cache */
	cpu_idcache_wbinv_all();

	/* Now really make sure they are clean.  */
	/* XXX */
	/*
	asm volatile ("mcr\tp15, 0, r0, c7, c7, 0" : : );
	*/

	/* Set the control register */
	curcpu()->ci_ctrl = cpuctrl;
	cpu_control(0xffffffff, cpuctrl);

	/* And again. */
	cpu_idcache_wbinv_all();
}
#endif	/* CPU_ARMv7 */

#ifdef CPU_SA110
struct cpu_option sa110_options[] = {
	{ "cpu.cache",		BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "cpu.nocache",	OR,  BIC, (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "sa110.cache",	BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "sa110.icache",	BIC, OR,  CPU_CONTROL_IC_ENABLE },
	{ "sa110.dcache",	BIC, OR,  CPU_CONTROL_DC_ENABLE },
	{ "cpu.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ "cpu.nowritebuf",	OR,  BIC, CPU_CONTROL_WBUF_ENABLE },
	{ "sa110.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ NULL,			IGN, IGN, 0 }
};

void
sa110_setup(args)
	char *args;
{
	int cpuctrl, cpuctrlmask;

	cpuctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
		 | CPU_CONTROL_WBUF_ENABLE;
	cpuctrlmask = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
		 | CPU_CONTROL_WBUF_ENABLE | CPU_CONTROL_ROM_ENABLE
		 | CPU_CONTROL_BEND_ENABLE | CPU_CONTROL_AFLT_ENABLE
		 | CPU_CONTROL_LABT_ENABLE | CPU_CONTROL_BPRD_ENABLE
		 | CPU_CONTROL_CPCLK;

#ifndef ARM32_DISABLE_ALIGNMENT_FAULTS
	cpuctrl |= CPU_CONTROL_AFLT_ENABLE;
#endif

	cpuctrl = parse_cpu_options(args, sa110_options, cpuctrl);

#ifdef __ARMEB__
	cpuctrl |= CPU_CONTROL_BEND_ENABLE;
#endif

	/* Clear out the cache */
	cpu_idcache_wbinv_all();

	/* Set the control register */
	curcpu()->ci_ctrl = cpuctrl;
/*	cpu_control(cpuctrlmask, cpuctrl);*/
	cpu_control(0xffffffff, cpuctrl);

	/*
	 * enable clockswitching, note that this doesn't read or write to r0,
	 * r0 is just to make it valid asm
	 */
	__asm ("mcr 15, 0, r0, c15, c1, 2");
}
#endif	/* CPU_SA110 */

#if defined(CPU_SA1100) || defined(CPU_SA1110)
struct cpu_option sa11x0_options[] = {
	{ "cpu.cache",		BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "cpu.nocache",	OR,  BIC, (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "sa11x0.cache",	BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "sa11x0.icache",	BIC, OR,  CPU_CONTROL_IC_ENABLE },
	{ "sa11x0.dcache",	BIC, OR,  CPU_CONTROL_DC_ENABLE },
	{ "cpu.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ "cpu.nowritebuf",	OR,  BIC, CPU_CONTROL_WBUF_ENABLE },
	{ "sa11x0.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ NULL,			IGN, IGN, 0 }
};

void
sa11x0_setup(args)
	char *args;
{
	int cpuctrl, cpuctrlmask;

	cpuctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
		 | CPU_CONTROL_WBUF_ENABLE | CPU_CONTROL_LABT_ENABLE;
	cpuctrlmask = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
		 | CPU_CONTROL_WBUF_ENABLE | CPU_CONTROL_ROM_ENABLE
		 | CPU_CONTROL_BEND_ENABLE | CPU_CONTROL_AFLT_ENABLE
		 | CPU_CONTROL_LABT_ENABLE | CPU_CONTROL_BPRD_ENABLE
		 | CPU_CONTROL_CPCLK | CPU_CONTROL_VECRELOC;

#ifndef ARM32_DISABLE_ALIGNMENT_FAULTS
	cpuctrl |= CPU_CONTROL_AFLT_ENABLE;
#endif

	cpuctrl = parse_cpu_options(args, sa11x0_options, cpuctrl);

#ifdef __ARMEB__
	cpuctrl |= CPU_CONTROL_BEND_ENABLE;
#endif

	if (vector_page == ARM_VECTORS_HIGH)
		cpuctrl |= CPU_CONTROL_VECRELOC;

	/* Clear out the cache */
	cpu_idcache_wbinv_all();

	/* Set the control register */
	cpu_control(0xffffffff, cpuctrl);
}
#endif	/* CPU_SA1100 || CPU_SA1110 */

#if defined(CPU_IXP12X0)
struct cpu_option ixp12x0_options[] = {
	{ "cpu.cache",		BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "cpu.nocache",	OR,  BIC, (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "ixp12x0.cache",	BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "ixp12x0.icache",	BIC, OR,  CPU_CONTROL_IC_ENABLE },
	{ "ixp12x0.dcache",	BIC, OR,  CPU_CONTROL_DC_ENABLE },
	{ "cpu.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ "cpu.nowritebuf",	OR,  BIC, CPU_CONTROL_WBUF_ENABLE },
	{ "ixp12x0.writebuf",	BIC, OR,  CPU_CONTROL_WBUF_ENABLE },
	{ NULL,			IGN, IGN, 0 }
};

void
ixp12x0_setup(args)
	char *args;
{
	int cpuctrl, cpuctrlmask;


	cpuctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_DC_ENABLE
		 | CPU_CONTROL_WBUF_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IC_ENABLE;

	cpuctrlmask = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_AFLT_ENABLE
		 | CPU_CONTROL_DC_ENABLE | CPU_CONTROL_WBUF_ENABLE
		 | CPU_CONTROL_BEND_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_ROM_ENABLE | CPU_CONTROL_IC_ENABLE
		 | CPU_CONTROL_VECRELOC;

#ifndef ARM32_DISABLE_ALIGNMENT_FAULTS
	cpuctrl |= CPU_CONTROL_AFLT_ENABLE;
#endif

	cpuctrl = parse_cpu_options(args, ixp12x0_options, cpuctrl);

#ifdef __ARMEB__
	cpuctrl |= CPU_CONTROL_BEND_ENABLE;
#endif

	if (vector_page == ARM_VECTORS_HIGH)
		cpuctrl |= CPU_CONTROL_VECRELOC;

	/* Clear out the cache */
	cpu_idcache_wbinv_all();

	/* Set the control register */
	curcpu()->ci_ctrl = cpuctrl;
	/* cpu_control(0xffffffff, cpuctrl); */
	cpu_control(cpuctrlmask, cpuctrl);
}
#endif /* CPU_IXP12X0 */

#if defined(CPU_XSCALE_80200) || defined(CPU_XSCALE_80321) || \
    defined(CPU_XSCALE_PXA2X0) || defined(CPU_XSCALE_IXP425)
struct cpu_option xscale_options[] = {
	{ "cpu.branchpredict", 	BIC, OR,  CPU_CONTROL_BPRD_ENABLE },
	{ "cpu.cache",		BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "cpu.nocache",	OR,  BIC, (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "xscale.branchpredict", BIC, OR,  CPU_CONTROL_BPRD_ENABLE },
	{ "xscale.cache",	BIC, OR,  (CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE) },
	{ "xscale.icache",	BIC, OR,  CPU_CONTROL_IC_ENABLE },
	{ "xscale.dcache",	BIC, OR,  CPU_CONTROL_DC_ENABLE },
	{ NULL,			IGN, IGN, 0 }
};

void
xscale_setup(args)
	char *args;
{
	uint32_t auxctl;
	int cpuctrl, cpuctrlmask;

	/*
	 * The XScale Write Buffer is always enabled.  Our option
	 * is to enable/disable coalescing.  Note that bits 6:3
	 * must always be enabled.
	 */

	cpuctrl = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
		 | CPU_CONTROL_WBUF_ENABLE | CPU_CONTROL_LABT_ENABLE
		 | CPU_CONTROL_BPRD_ENABLE;
	cpuctrlmask = CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_32BP_ENABLE
		 | CPU_CONTROL_32BD_ENABLE | CPU_CONTROL_SYST_ENABLE
		 | CPU_CONTROL_IC_ENABLE | CPU_CONTROL_DC_ENABLE
		 | CPU_CONTROL_WBUF_ENABLE | CPU_CONTROL_ROM_ENABLE
		 | CPU_CONTROL_BEND_ENABLE | CPU_CONTROL_AFLT_ENABLE
		 | CPU_CONTROL_LABT_ENABLE | CPU_CONTROL_BPRD_ENABLE
		 | CPU_CONTROL_CPCLK | CPU_CONTROL_VECRELOC;

#ifndef ARM32_DISABLE_ALIGNMENT_FAULTS
	cpuctrl |= CPU_CONTROL_AFLT_ENABLE;
#endif

	cpuctrl = parse_cpu_options(args, xscale_options, cpuctrl);

#ifdef __ARMEB__
	cpuctrl |= CPU_CONTROL_BEND_ENABLE;
#endif

	if (vector_page == ARM_VECTORS_HIGH)
		cpuctrl |= CPU_CONTROL_VECRELOC;

	/* Clear out the cache */
	cpu_idcache_wbinv_all();

	/*
	 * Set the control register.  Note that bits 6:3 must always
	 * be set to 1.
	 */
	curcpu()->ci_ctrl = cpuctrl;
/*	cpu_control(cpuctrlmask, cpuctrl);*/
	cpu_control(0xffffffff, cpuctrl);

	/* Make sure write coalescing is turned on */
	__asm __volatile("mrc p15, 0, %0, c1, c0, 1"
		: "=r" (auxctl));
#ifdef XSCALE_NO_COALESCE_WRITES
	auxctl |= XSCALE_AUXCTL_K;
#else
	auxctl &= ~XSCALE_AUXCTL_K;
#endif
	__asm __volatile("mcr p15, 0, %0, c1, c0, 1"
		: : "r" (auxctl));
}
#endif	/* CPU_XSCALE_80200 || CPU_XSCALE_80321 || CPU_XSCALE_PXA2X0 || CPU_XSCALE_IXP425 */
