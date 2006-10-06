/*	$OpenBSD: src/sys/arch/sh/sh/pmap.c,v 1.2 2006/10/06 21:16:57 mickey Exp $	*/
/*	$NetBSD: pmap.c,v 1.55 2006/08/07 23:19:36 tsutsui Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/msgbuf.h>

#include <uvm/uvm.h>

#include <sh/mmu.h>
#include <sh/cache.h>

#ifdef DEBUG
#define	STATIC
#else
#define	STATIC	static
#endif

#define	__PMAP_PTP_SHIFT	22
#define	__PMAP_PTP_TRUNC(va)						\
	(((va) + (1 << __PMAP_PTP_SHIFT) - 1) & ~((1 << __PMAP_PTP_SHIFT) - 1))
#define	__PMAP_PTP_PG_N		(PAGE_SIZE / sizeof(pt_entry_t))
#define	__PMAP_PTP_INDEX(va)	(((va) >> __PMAP_PTP_SHIFT) & (__PMAP_PTP_N - 1))
#define	__PMAP_PTP_OFSET(va)	((va >> PGSHIFT) & (__PMAP_PTP_PG_N - 1))

struct pmap __pmap_kernel;
STATIC vaddr_t __pmap_kve;	/* VA of last kernel virtual */
paddr_t avail_start;		/* PA of first available physical page */
paddr_t avail_end;		/* PA of last available physical page */

/* pmap pool */
STATIC struct pool __pmap_pmap_pool;

/* pv_entry ops. */
struct pv_entry {
	struct pmap *pv_pmap;
	vaddr_t pv_va;
	SLIST_ENTRY(pv_entry) pv_link;
};
#define	__pmap_pv_alloc()	pool_get(&__pmap_pv_pool, PR_NOWAIT)
#define	__pmap_pv_free(pv)	pool_put(&__pmap_pv_pool, (pv))
STATIC void __pmap_pv_enter(pmap_t, struct vm_page *, vaddr_t);
STATIC void __pmap_pv_remove(pmap_t, struct vm_page *, vaddr_t);
STATIC void *__pmap_pv_page_alloc(struct pool *, int);
STATIC void __pmap_pv_page_free(struct pool *, void *);
STATIC struct pool __pmap_pv_pool;
STATIC struct pool_allocator pmap_pv_page_allocator = {
	__pmap_pv_page_alloc, __pmap_pv_page_free, 0,
};

/* ASID ops. */
STATIC int __pmap_asid_alloc(void);
STATIC void __pmap_asid_free(int);
STATIC struct {
	uint32_t map[8];
	int hint;	/* hint for next allocation */
} __pmap_asid;

/* page table entry ops. */
STATIC pt_entry_t *__pmap_pte_alloc(pmap_t, vaddr_t);

/* pmap_enter util */
STATIC boolean_t __pmap_map_change(pmap_t, vaddr_t, paddr_t, vm_prot_t,
    pt_entry_t);

void
pmap_bootstrap()
{
	/* Steal msgbuf area */
	initmsgbuf((caddr_t)uvm_pageboot_alloc(MSGBUFSIZE), MSGBUFSIZE);

	avail_start = ptoa(vm_physmem[0].start);
	avail_end = ptoa(vm_physmem[vm_nphysseg - 1].end);
	__pmap_kve = VM_MIN_KERNEL_ADDRESS;

	pmap_kernel()->pm_refcnt = 1;
	pmap_kernel()->pm_ptp = (pt_entry_t **)uvm_pageboot_alloc(PAGE_SIZE);
	memset(pmap_kernel()->pm_ptp, 0, PAGE_SIZE);

	/* Enable MMU */
	sh_mmu_start();
	/* Mask all interrupt */
	_cpu_intr_suspend();
	/* Enable exception for P3 access */
	_cpu_exception_resume(0);
}

vaddr_t
pmap_steal_memory(vsize_t size, vaddr_t *vstart, vaddr_t *vend)
{
	struct vm_physseg *bank;
	int i, j, npage;
	paddr_t pa;
	vaddr_t va;

	KDASSERT(!uvm.page_init_done);

	size = round_page(size);
	npage = atop(size);

	for (i = 0, bank = &vm_physmem[i]; i < vm_nphysseg; i++, bank++)
		if (npage <= bank->avail_end - bank->avail_start)
			break;
	KDASSERT(i != vm_nphysseg);

	/* Steal pages */
	pa = ptoa(bank->avail_start);
	bank->avail_start += npage;
	bank->start += npage;

	/* GC memory bank */
	if (bank->avail_start == bank->end) {
		/* Remove this segment from the list. */
		vm_nphysseg--;
		KDASSERT(vm_nphysseg > 0);
		for (j = i; i < vm_nphysseg; j++)
			vm_physmem[j] = vm_physmem[j + 1];
	}

	va = SH3_PHYS_TO_P1SEG(pa);
	memset((caddr_t)va, 0, size);

	if (vstart)
		*vstart = VM_MIN_KERNEL_ADDRESS;
	if (vend)
		*vend = VM_MAX_KERNEL_ADDRESS;

	return (va);
}

vaddr_t
pmap_growkernel(vaddr_t maxkvaddr)
{
	int i, n;

	if (maxkvaddr <= __pmap_kve)
		return (__pmap_kve);

	i = __PMAP_PTP_INDEX(__pmap_kve - VM_MIN_KERNEL_ADDRESS);
	__pmap_kve = __PMAP_PTP_TRUNC(maxkvaddr);
	n = __PMAP_PTP_INDEX(__pmap_kve - VM_MIN_KERNEL_ADDRESS);

	/* Allocate page table pages */
	for (;i < n; i++) {
		if (__pmap_kernel.pm_ptp[i] != NULL)
			continue;

		if (uvm.page_init_done) {
			struct vm_page *pg = uvm_pagealloc(NULL, 0, NULL,
			    UVM_PGA_USERESERVE | UVM_PGA_ZERO);
			if (pg == NULL)
				goto error;
			__pmap_kernel.pm_ptp[i] = (pt_entry_t *)
			    SH3_PHYS_TO_P1SEG(VM_PAGE_TO_PHYS(pg));
		} else {
			pt_entry_t *ptp = (pt_entry_t *)
			    uvm_pageboot_alloc(PAGE_SIZE);
			if (ptp == NULL)
				goto error;
			__pmap_kernel.pm_ptp[i] = ptp;
			memset(ptp, 0, PAGE_SIZE);
		}
	}

	return (__pmap_kve);
 error:
	panic("pmap_growkernel: out of memory.");
	/* NOTREACHED */
}

void
pmap_init()
{
	/* Initialize pmap module */
	pool_init(&__pmap_pmap_pool, sizeof(struct pmap), 0, 0, 0, "pmappl",
	    &pool_allocator_nointr);
	pool_init(&__pmap_pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pvpl",
	    &pmap_pv_page_allocator);
	pool_setlowat(&__pmap_pv_pool, 16);
}

pmap_t
pmap_create()
{
	pmap_t pmap;

	pmap = pool_get(&__pmap_pmap_pool, PR_WAITOK);
	memset(pmap, 0, sizeof(struct pmap));
	pmap->pm_asid = -1;
	pmap->pm_refcnt = 1;
	/* Allocate page table page holder (512 slot) */
	pmap->pm_ptp = (pt_entry_t **)
	    SH3_PHYS_TO_P1SEG(VM_PAGE_TO_PHYS(
		    uvm_pagealloc(NULL, 0, NULL,
			UVM_PGA_USERESERVE | UVM_PGA_ZERO)));

	return (pmap);
}

void
pmap_destroy(pmap_t pmap)
{
	int i;

	if (--pmap->pm_refcnt > 0)
		return;

	/* Deallocate all page table page */
	for (i = 0; i < __PMAP_PTP_N; i++) {
		vaddr_t va = (vaddr_t)pmap->pm_ptp[i];
		if (va == 0)
			continue;
#ifdef DEBUG	/* Check no mapping exists. */
		{
			int j;
			pt_entry_t *pte = (pt_entry_t *)va;
			for (j = 0; j < __PMAP_PTP_PG_N; j++, pte++)
				KDASSERT(*pte == 0);
		}
#endif /* DEBUG */
		/* Purge cache entry for next use of this page. */
		if (SH_HAS_VIRTUAL_ALIAS)
			sh_dcache_inv_range(va, PAGE_SIZE);
		/* Free page table */
		uvm_pagefree(PHYS_TO_VM_PAGE(SH3_P1SEG_TO_PHYS(va)));
	}
	/* Deallocate page table page holder */
	if (SH_HAS_VIRTUAL_ALIAS)
		sh_dcache_inv_range((vaddr_t)pmap->pm_ptp, PAGE_SIZE);
	uvm_pagefree(PHYS_TO_VM_PAGE(SH3_P1SEG_TO_PHYS((vaddr_t)pmap->pm_ptp)));

	/* Free ASID */
	__pmap_asid_free(pmap->pm_asid);

	pool_put(&__pmap_pmap_pool, pmap);
}

void
pmap_reference(pmap_t pmap)
{
	pmap->pm_refcnt++;
}

void
pmap_activate(struct proc *p)
{
	pmap_t pmap = p->p_vmspace->vm_map.pmap;

	if (pmap->pm_asid == -1)
		pmap->pm_asid = __pmap_asid_alloc();

	KDASSERT(pmap->pm_asid >=0 && pmap->pm_asid < 256);
	sh_tlb_set_asid(pmap->pm_asid);
}

int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, int flags)
{
	struct vm_page *pg;
	struct vm_page_md *pvh;
	pt_entry_t entry, *pte;
	boolean_t kva = (pmap == pmap_kernel());

	/* "flags" never exceed "prot" */
	KDASSERT(prot != 0 && ((flags & VM_PROT_ALL) & ~prot) == 0);

	pg = PHYS_TO_VM_PAGE(pa);
	entry = (pa & PG_PPN) | PG_4K;
	if (flags & PMAP_WIRED)
		entry |= _PG_WIRED;

	if (pg != NULL) {	/* memory-space */
		pvh = &pg->mdpage;
		entry |= PG_C;	/* always cached */

		/* Modified/reference tracking */
		if (flags & VM_PROT_WRITE) {
			entry |= PG_V | PG_D;
			pvh->pvh_flags |= PVH_MODIFIED | PVH_REFERENCED;
		} else if (flags & VM_PROT_ALL) {
			entry |= PG_V;
			pvh->pvh_flags |= PVH_REFERENCED;
		}

		/* Protection */
		if ((prot & VM_PROT_WRITE) && (pvh->pvh_flags & PVH_MODIFIED)) {
			if (kva)
				entry |= PG_PR_KRW | PG_SH;
			else
				entry |= PG_PR_URW;
		} else {
			/* RO, COW page */
			if (kva)
				entry |= PG_PR_KRO | PG_SH;
			else
				entry |= PG_PR_URO;
		}

		/* Check for existing mapping */
		if (__pmap_map_change(pmap, va, pa, prot, entry))
			return (0);

		/* Add to physical-virtual map list of this page */
		__pmap_pv_enter(pmap, pg, va);

	} else {	/* bus-space (always uncached map) */
		if (kva) {
			entry |= PG_V | PG_SH |
			    ((prot & VM_PROT_WRITE) ?
			    (PG_PR_KRW | PG_D) : PG_PR_KRO);
		} else {
			entry |= PG_V |
			    ((prot & VM_PROT_WRITE) ?
			    (PG_PR_URW | PG_D) : PG_PR_URO);
		}
	}

	/* Register to page table */
	if (kva)
		pte = __pmap_kpte_lookup(va);
	else {
		pte = __pmap_pte_alloc(pmap, va);
		if (pte == NULL) {
			if (flags & PMAP_CANFAIL)
				return ENOMEM;
			panic("pmap_enter: cannot allocate pte");
		}
	}

	*pte = entry;

	if (pmap->pm_asid != -1)
		sh_tlb_update(pmap->pm_asid, va, entry);

	if (!SH_HAS_UNIFIED_CACHE &&
	    (prot == (VM_PROT_READ | VM_PROT_EXECUTE)))
		sh_icache_sync_range_index(va, PAGE_SIZE);

	if (entry & _PG_WIRED)
		pmap->pm_stats.wired_count++;
	pmap->pm_stats.resident_count++;

	return (0);
}

/*
 * boolean_t __pmap_map_change(pmap_t pmap, vaddr_t va, paddr_t pa,
 *     vm_prot_t prot, pt_entry_t entry):
 *	Handle the situation that pmap_enter() is called to enter a
 *	mapping at a virtual address for which a mapping already
 *	exists.
 */
boolean_t
__pmap_map_change(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot,
    pt_entry_t entry)
{
	pt_entry_t *pte, oentry;
	vaddr_t eva = va + PAGE_SIZE;

	if ((pte = __pmap_pte_lookup(pmap, va)) == NULL ||
	    ((oentry = *pte) == 0))
		return (FALSE);		/* no mapping exists. */

	if (pa != (oentry & PG_PPN)) {
		/* Enter a mapping at a mapping to another physical page. */
		pmap_remove(pmap, va, eva);
		return (FALSE);
	}

	/* Pre-existing mapping */

	/* Protection change. */
	if ((oentry & PG_PR_MASK) != (entry & PG_PR_MASK))
		pmap_protect(pmap, va, eva, prot);

	/* Wired change */
	if (oentry & _PG_WIRED) {
		if (!(entry & _PG_WIRED)) {
			/* wired -> unwired */
			*pte = entry;
			/* "wired" is software bits. no need to update TLB */
			pmap->pm_stats.wired_count--;
		}
	} else if (entry & _PG_WIRED) {
		/* unwired -> wired. make sure to reflect "flags" */
		pmap_remove(pmap, va, eva);
		return (FALSE);
	}

	return (TRUE);	/* mapping was changed. */
}

/*
 * void __pmap_pv_enter(pmap_t pmap, struct vm_page *pg, vaddr_t vaddr):
 *	Insert physical-virtual map to vm_page.
 *	Assume pre-existed mapping is already removed.
 */
void
__pmap_pv_enter(pmap_t pmap, struct vm_page *pg, vaddr_t va)
{
	struct vm_page_md *pvh;
	struct pv_entry *pv;
	int s;

	s = splvm();
	if (SH_HAS_VIRTUAL_ALIAS) {
		/* Remove all other mapping on this physical page */
		pvh = &pg->mdpage;
		while ((pv = SLIST_FIRST(&pvh->pvh_head)) != NULL) {
			pmap_remove(pv->pv_pmap, pv->pv_va,
			    pv->pv_va + PAGE_SIZE);
		}
	}

	/* Register pv map */
	pvh = &pg->mdpage;
	pv = __pmap_pv_alloc();
	pv->pv_pmap = pmap;
	pv->pv_va = va;

	SLIST_INSERT_HEAD(&pvh->pvh_head, pv, pv_link);
	splx(s);
}

void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	struct vm_page *pg;
	pt_entry_t *pte, entry;
	vaddr_t va;

	KDASSERT((sva & PGOFSET) == 0);

	for (va = sva; va < eva; va += PAGE_SIZE) {
		if ((pte = __pmap_pte_lookup(pmap, va)) == NULL ||
		    (entry = *pte) == 0)
			continue;

		if ((pg = PHYS_TO_VM_PAGE(entry & PG_PPN)) != NULL)
			__pmap_pv_remove(pmap, pg, va);

		if (entry & _PG_WIRED)
			pmap->pm_stats.wired_count--;
		pmap->pm_stats.resident_count--;
		*pte = 0;

		/*
		 * When pmap->pm_asid == -1 (invalid ASID), old entry attribute
		 * to this pmap is already removed by pmap_activate().
		 */
		if (pmap->pm_asid != -1)
			sh_tlb_invalidate_addr(pmap->pm_asid, va);
	}
}

/*
 * void __pmap_pv_remove(pmap_t pmap, struct vm_page *pg, vaddr_t vaddr):
 *	Remove physical-virtual map from vm_page.
 */
void
__pmap_pv_remove(pmap_t pmap, struct vm_page *pg, vaddr_t vaddr)
{
	struct vm_page_md *pvh;
	struct pv_entry *pv;
	int s;

	s = splvm();
	pvh = &pg->mdpage;
	SLIST_FOREACH(pv, &pvh->pvh_head, pv_link) {
		if (pv->pv_pmap == pmap && pv->pv_va == vaddr) {
			if (SH_HAS_VIRTUAL_ALIAS ||
			    (SH_HAS_WRITEBACK_CACHE &&
				(pg->mdpage.pvh_flags & PVH_MODIFIED))) {
				/*
				 * Always use index ops. since I don't want to
				 * worry about address space.
				 */
				sh_dcache_wbinv_range_index
				    (pv->pv_va, PAGE_SIZE);
			}

			SLIST_REMOVE(&pvh->pvh_head, pv, pv_entry, pv_link);
			__pmap_pv_free(pv);
			break;
		}
	}
#ifdef DEBUG
	/* Check duplicated map. */
	SLIST_FOREACH(pv, &pvh->pvh_head, pv_link)
	    KDASSERT(!(pv->pv_pmap == pmap && pv->pv_va == vaddr));
#endif
	splx(s);
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	pt_entry_t *pte, entry;

	KDASSERT((va & PGOFSET) == 0);
	KDASSERT(va >= VM_MIN_KERNEL_ADDRESS && va < VM_MAX_KERNEL_ADDRESS);

	entry = (pa & PG_PPN) | PG_V | PG_SH | PG_4K;
	if (prot & VM_PROT_WRITE)
		entry |= (PG_PR_KRW | PG_D);
	else
		entry |= PG_PR_KRO;

	if (PHYS_TO_VM_PAGE(pa))
		entry |= PG_C;

	pte = __pmap_kpte_lookup(va);

	KDASSERT(*pte == 0);
	*pte = entry;

	sh_tlb_update(0, va, entry);
}

void
pmap_kremove(vaddr_t va, vsize_t len)
{
	pt_entry_t *pte;
	vaddr_t eva = va + len;

	KDASSERT((va & PGOFSET) == 0);
	KDASSERT((len & PGOFSET) == 0);
	KDASSERT(va >= VM_MIN_KERNEL_ADDRESS && eva <= VM_MAX_KERNEL_ADDRESS);

	for (; va < eva; va += PAGE_SIZE) {
		pte = __pmap_kpte_lookup(va);
		KDASSERT(pte != NULL);
		if (*pte == 0)
			continue;

		if (SH_HAS_VIRTUAL_ALIAS && PHYS_TO_VM_PAGE(*pte & PG_PPN))
			sh_dcache_wbinv_range(va, PAGE_SIZE);
		*pte = 0;

		sh_tlb_invalidate_addr(0, va);
	}
}

boolean_t
pmap_extract(pmap_t pmap, vaddr_t va, paddr_t *pap)
{
	pt_entry_t *pte;

	/* handle P1 and P2 specially: va == pa */
	if (pmap == pmap_kernel() && (va >> 30) == 2) {
		if (pap != NULL)
			*pap = va & SH3_PHYS_MASK;
		return (TRUE);
	}

	pte = __pmap_pte_lookup(pmap, va);
	if (pte == NULL || *pte == 0)
		return (FALSE);

	if (pap != NULL)
		*pap = (*pte & PG_PPN) | (va & PGOFSET);

	return (TRUE);
}

void
pmap_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	boolean_t kernel = pmap == pmap_kernel();
	pt_entry_t *pte, entry;
	vaddr_t va;

	sva = trunc_page(sva);

	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}

	for (va = sva; va < eva; va += PAGE_SIZE) {

		if (((pte = __pmap_pte_lookup(pmap, va)) == NULL) ||
		    (entry = *pte) == 0)
			continue;

		if (SH_HAS_VIRTUAL_ALIAS && (entry & PG_D)) {
			if (!SH_HAS_UNIFIED_CACHE && (prot & VM_PROT_EXECUTE))
				sh_icache_sync_range_index(va, PAGE_SIZE);
			else
				sh_dcache_wbinv_range_index(va, PAGE_SIZE);
		}

		entry &= ~PG_PR_MASK;
		switch (prot) {
		default:
			panic("pmap_protect: invalid protection mode %x", prot);
			/* NOTREACHED */
		case VM_PROT_READ:
			/* FALLTHROUGH */
		case VM_PROT_READ | VM_PROT_EXECUTE:
			entry |= kernel ? PG_PR_KRO : PG_PR_URO;
			break;
		case VM_PROT_READ | VM_PROT_WRITE:
			/* FALLTHROUGH */
		case VM_PROT_ALL:
			entry |= kernel ? PG_PR_KRW : PG_PR_URW;
			break;
		}
		*pte = entry;

		if (pmap->pm_asid != -1)
			sh_tlb_update(pmap->pm_asid, va, entry);
	}
}

void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	struct vm_page_md *pvh = &pg->mdpage;
	struct pv_entry *pv;
	struct pmap *pmap;
	vaddr_t va;
	int s;

	switch (prot) {
	case VM_PROT_READ | VM_PROT_WRITE:
		/* FALLTHROUGH */
	case VM_PROT_ALL:
		break;

	case VM_PROT_READ:
		/* FALLTHROUGH */
	case VM_PROT_READ | VM_PROT_EXECUTE:
		s = splvm();
		SLIST_FOREACH(pv, &pvh->pvh_head, pv_link) {
			pmap = pv->pv_pmap;
			va = pv->pv_va;

			KDASSERT(pmap);
			pmap_protect(pmap, va, va + PAGE_SIZE, prot);
		}
		splx(s);
		break;

	default:
		/* Remove all */
		s = splvm();
		while ((pv = SLIST_FIRST(&pvh->pvh_head)) != NULL) {
			va = pv->pv_va;
			pmap_remove(pv->pv_pmap, va, va + PAGE_SIZE);
		}
		splx(s);
	}
}

void
pmap_unwire(pmap_t pmap, vaddr_t va)
{
	pt_entry_t *pte, entry;

	if ((pte = __pmap_pte_lookup(pmap, va)) == NULL ||
	    (entry = *pte) == 0 ||
	    (entry & _PG_WIRED) == 0)
		return;

	*pte = entry & ~_PG_WIRED;
	pmap->pm_stats.wired_count--;
}

void
pmap_proc_iflush(struct proc *p, vaddr_t va, size_t len)
{
	if (!SH_HAS_UNIFIED_CACHE)
		sh_icache_sync_range_index(va, len);
}

void
pmap_zero_page(vm_page_t pg)
{
	paddr_t phys = VM_PAGE_TO_PHYS(pg);

	if (SH_HAS_VIRTUAL_ALIAS) {	/* don't pollute cache */
		/* sync cache since we access via P2. */
		sh_dcache_wbinv_all();
		memset((void *)SH3_PHYS_TO_P2SEG(phys), 0, PAGE_SIZE);
	} else {
		memset((void *)SH3_PHYS_TO_P1SEG(phys), 0, PAGE_SIZE);
	}
}

void
pmap_copy_page(vm_page_t srcpg, vm_page_t dstpg)
{
	paddr_t src,dst;
       
	src = VM_PAGE_TO_PHYS(srcpg);
	dst = VM_PAGE_TO_PHYS(dstpg);

	if (SH_HAS_VIRTUAL_ALIAS) {	/* don't pollute cache */
		/* sync cache since we access via P2. */
		sh_dcache_wbinv_all();
		memcpy((void *)SH3_PHYS_TO_P2SEG(dst),
		    (void *)SH3_PHYS_TO_P2SEG(src), PAGE_SIZE);
	} else {
		memcpy((void *)SH3_PHYS_TO_P1SEG(dst),
		    (void *)SH3_PHYS_TO_P1SEG(src), PAGE_SIZE);
	}
}

boolean_t
pmap_is_referenced(struct vm_page *pg)
{
	return ((pg->mdpage.pvh_flags & PVH_REFERENCED) ? TRUE : FALSE);
}

boolean_t
pmap_clear_reference(struct vm_page *pg)
{
	struct vm_page_md *pvh = &pg->mdpage;
	struct pv_entry *pv;
	pt_entry_t *pte;
	pmap_t pmap;
	vaddr_t va;
	int s;

	if ((pg->mdpage.pvh_flags & PVH_REFERENCED) == 0)
		return (FALSE);

	pg->mdpage.pvh_flags &= ~PVH_REFERENCED;

	s = splvm();
	/* Restart reference bit emulation */
	SLIST_FOREACH(pv, &pvh->pvh_head, pv_link) {
		pmap = pv->pv_pmap;
		va = pv->pv_va;

		if ((pte = __pmap_pte_lookup(pmap, va)) == NULL)
			continue;
		if ((*pte & PG_V) == 0)
			continue;
		*pte &= ~PG_V;

		if (pmap->pm_asid != -1)
			sh_tlb_invalidate_addr(pmap->pm_asid, va);
	}
	splx(s);

	return (TRUE);
}

boolean_t
pmap_is_modified(struct vm_page *pg)
{
	return ((pg->mdpage.pvh_flags & PVH_MODIFIED) ? TRUE : FALSE);
}

boolean_t
pmap_clear_modify(struct vm_page *pg)
{
	struct vm_page_md *pvh = &pg->mdpage;
	struct pv_entry *pv;
	struct pmap *pmap;
	pt_entry_t *pte, entry;
	boolean_t modified;
	vaddr_t va;
	int s;

	modified = pvh->pvh_flags & PVH_MODIFIED;
	if (!modified)
		return (FALSE);

	pvh->pvh_flags &= ~PVH_MODIFIED;

	s = splvm();
	if (SLIST_EMPTY(&pvh->pvh_head)) {/* no map on this page */
		splx(s);
		return (TRUE);
	}

	/* Write-back and invalidate TLB entry */
	if (!SH_HAS_VIRTUAL_ALIAS && SH_HAS_WRITEBACK_CACHE)
		sh_dcache_wbinv_all();

	SLIST_FOREACH(pv, &pvh->pvh_head, pv_link) {
		pmap = pv->pv_pmap;
		va = pv->pv_va;
		if ((pte = __pmap_pte_lookup(pmap, va)) == NULL)
			continue;
		entry = *pte;
		if ((entry & PG_D) == 0)
			continue;

		if (SH_HAS_VIRTUAL_ALIAS)
			sh_dcache_wbinv_range_index(va, PAGE_SIZE);

		*pte = entry & ~PG_D;
		if (pmap->pm_asid != -1)
			sh_tlb_invalidate_addr(pmap->pm_asid, va);
	}
	splx(s);

	return (TRUE);
}

/*
 * pv_entry pool allocator:
 *	void *__pmap_pv_page_alloc(struct pool *pool, int flags):
 *	void __pmap_pv_page_free(struct pool *pool, void *v):
 */
void *
__pmap_pv_page_alloc(struct pool *pool, int flags)
{
	struct vm_page *pg;

	pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE);
	if (pg == NULL)
		return (NULL);

	return ((void *)SH3_PHYS_TO_P1SEG(VM_PAGE_TO_PHYS(pg)));
}

void
__pmap_pv_page_free(struct pool *pool, void *v)
{
	vaddr_t va = (vaddr_t)v;

	/* Invalidate cache for next use of this page */
	if (SH_HAS_VIRTUAL_ALIAS)
		sh_icache_sync_range_index(va, PAGE_SIZE);
	uvm_pagefree(PHYS_TO_VM_PAGE(SH3_P1SEG_TO_PHYS(va)));
}

/*
 * pt_entry_t __pmap_pte_alloc(pmap_t pmap, vaddr_t va):
 *	lookup page table entry. if found returns it, else allocate it.
 *	page table is accessed via P1.
 */
pt_entry_t *
__pmap_pte_alloc(pmap_t pmap, vaddr_t va)
{
	struct vm_page *pg;
	pt_entry_t *ptp, *pte;

	if ((pte = __pmap_pte_lookup(pmap, va)) != NULL)
		return (pte);

	/* Allocate page table (not managed page) */
	pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE | UVM_PGA_ZERO);
	if (pg == NULL)
		return NULL;

	ptp = (pt_entry_t *)SH3_PHYS_TO_P1SEG(VM_PAGE_TO_PHYS(pg));
	pmap->pm_ptp[__PMAP_PTP_INDEX(va)] = ptp;

	return (ptp + __PMAP_PTP_OFSET(va));
}

/*
 * pt_entry_t *__pmap_pte_lookup(pmap_t pmap, vaddr_t va):
 *	lookup page table entry, if not allocated, returns NULL.
 */
pt_entry_t *
__pmap_pte_lookup(pmap_t pmap, vaddr_t va)
{
	pt_entry_t *ptp;

	if (pmap == pmap_kernel())
		return (__pmap_kpte_lookup(va));

	/* Lookup page table page */
	ptp = pmap->pm_ptp[__PMAP_PTP_INDEX(va)];
	if (ptp == NULL)
		return (NULL);

	return (ptp + __PMAP_PTP_OFSET(va));
}

/*
 * pt_entry_t *__pmap_kpte_lookup(vaddr_t va):
 *	kernel virtual only version of __pmap_pte_lookup().
 */
pt_entry_t *
__pmap_kpte_lookup(vaddr_t va)
{
	return (__pmap_kernel.pm_ptp
	    [__PMAP_PTP_INDEX(va - VM_MIN_KERNEL_ADDRESS)] +
	    __PMAP_PTP_OFSET(va));
}

/*
 * boolean_t __pmap_pte_load(pmap_t pmap, vaddr_t va, int flags):
 *	lookup page table entry, if found it, load to TLB.
 *	flags specify do emulate reference and/or modified bit or not.
 */
boolean_t
__pmap_pte_load(pmap_t pmap, vaddr_t va, int flags)
{
	struct vm_page *pg;
	pt_entry_t *pte;
	pt_entry_t entry;

	KDASSERT((((int)va < 0) && (pmap == pmap_kernel())) ||
	    (((int)va >= 0) && (pmap != pmap_kernel())));

	/* Lookup page table entry */
	if (((pte = __pmap_pte_lookup(pmap, va)) == NULL) ||
	    ((entry = *pte) == 0))
		return (FALSE);

	KDASSERT(va != 0);

	/* Emulate reference/modified tracking for managed page. */
	if (flags != 0 && (pg = PHYS_TO_VM_PAGE(entry & PG_PPN)) != NULL) {
		if (flags & PVH_REFERENCED) {
			pg->mdpage.pvh_flags |= PVH_REFERENCED;
			entry |= PG_V;
		}
		if (flags & PVH_MODIFIED) {
			pg->mdpage.pvh_flags |= PVH_MODIFIED;
			entry |= PG_D;
		}
		*pte = entry;
	}

	/* When pmap has valid ASID, register to TLB */
	if (pmap->pm_asid != -1)
		sh_tlb_update(pmap->pm_asid, va, entry);

	return (TRUE);
}

/*
 * int __pmap_asid_alloc(void):
 *	Allocate new ASID. if all ASID is used, steal from other process.
 */
int
__pmap_asid_alloc()
{
	struct proc *p;
	int i, j, k, n, map, asid;

	/* Search free ASID */
	i = __pmap_asid.hint >> 5;
	n = i + 8;
	for (; i < n; i++) {
		k = i & 0x7;
		map = __pmap_asid.map[k];
		for (j = 0; j < 32; j++) {
			if ((map & (1 << j)) == 0 && (k + j) != 0) {
				__pmap_asid.map[k] |= (1 << j);
				__pmap_asid.hint = (k << 5) + j;
				return (__pmap_asid.hint);
			}
		}
	}

	/* Steal ASID */
	LIST_FOREACH(p, &allproc, p_list) {
		if ((asid = p->p_vmspace->vm_map.pmap->pm_asid) > 0) {
			pmap_t pmap = p->p_vmspace->vm_map.pmap;
			pmap->pm_asid = -1;
			__pmap_asid.hint = asid;
			/* Invalidate all old ASID entry */
			sh_tlb_invalidate_asid(pmap->pm_asid);

			return (__pmap_asid.hint);
		}
	}

	panic("No ASID allocated.");
	/* NOTREACHED */
}

/*
 * void __pmap_asid_free(int asid):
 *	Return unused ASID to pool. and remove all TLB entry of ASID.
 */
void
__pmap_asid_free(int asid)
{
	int i;

	if (asid < 1)	/* Don't invalidate kernel ASID 0 */
		return;

	sh_tlb_invalidate_asid(asid);

	i = asid >> 5;
	__pmap_asid.map[i] &= ~(1 << (asid - (i << 5)));
}
