/*	$OpenBSD: src/sys/uvm/uvm_anon.h,v 1.9 2001/11/28 19:28:14 art Exp $	*/
/*	$NetBSD: uvm_anon.h,v 1.15 2001/05/26 16:32:46 chs Exp $	*/

/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _UVM_UVM_ANON_H_
#define _UVM_UVM_ANON_H_

/*
 * uvm_anon.h
 */

/*
 * anonymous memory management
 *
 * anonymous virtual memory is short term virtual memory that goes away
 * when the processes referencing it go away.    an anonymous page of
 * virtual memory is described by the following data structure:
 */

struct vm_anon {
	int an_ref;			/* reference count [an_lock] */
	struct simplelock an_lock;	/* lock for an_ref */
	union {
		struct vm_anon *an_nxt;	/* if on free list [afreelock] */
		struct vm_page *an_page;/* if in RAM [an_lock] */
	} u;
	int an_swslot;		/* drum swap slot # (if != 0)
				   [an_lock.  also, it is ok to read
				   an_swslot if we hold an_page PG_BUSY] */
};

/*
 * a pool of vm_anon data structures is allocated and put on a global
 * free list at boot time.  vm_anon's on the free list use "an_nxt" as
 * a pointer to the next item on the free list.  for active vm_anon's
 * the data can be in one of the following state: [1] in a vm_page
 * with no backing store allocated yet, [2] in a vm_page with backing
 * store allocated, or [3] paged out to backing store (no vm_page).
 *
 * for pageout in case [2]: if the page has been modified then we must
 * flush it out to backing store, otherwise we can just dump the
 * vm_page.
 */

/*
 * anons are grouped together in anonymous memory maps, or amaps.
 * amaps are defined in uvm_amap.h.
 */

/*
 * processes reference anonymous virtual memory maps with an anonymous
 * reference structure:
 */

struct vm_aref {
	int ar_pageoff;			/* page offset into amap we start */
	struct vm_amap *ar_amap;	/* pointer to amap */
};

/*
 * the offset field indicates which part of the amap we are referencing.
 * locked by vm_map lock.
 */

#ifdef _KERNEL

/*
 * prototypes
 */

struct vm_anon *uvm_analloc __P((void));
void uvm_anfree __P((struct vm_anon *));
void uvm_anon_init __P((void));
int uvm_anon_add __P((int));
void uvm_anon_remove __P((int));
struct vm_page *uvm_anon_lockloanpg __P((struct vm_anon *));
void uvm_anon_dropswap __P((struct vm_anon *));
boolean_t anon_swap_off __P((int, int));
#endif /* _KERNEL */

#endif /* _UVM_UVM_ANON_H_ */
