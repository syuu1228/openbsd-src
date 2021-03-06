/* $OpenBSD: src/lib/librthread/arch/hppa64/_atomic_lock.c,v 1.1 2012/04/13 14:38:22 jsing Exp $ */
/*
 * Copyright (c) 2005 Marco Peereboom <marco@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <spinlock.h>
#ifdef DIAGNOSTIC
#include <stdio.h>
#include <stdlib.h>
#endif

int
_atomic_lock(volatile _spinlock_lock_t *lock)
{
	volatile _spinlock_lock_t old;

#ifdef DIAGNOSTIC
	if ((unsigned long)lock & 0xf) {
		printf("lock not 16 byte aligned\n");
		abort();
	}
#endif

	asm volatile ("ldcw 0(%2), %0"
		     : "=&r" (old), "+m" (lock)
		     : "r" (lock));

	return (old == _SPINLOCK_LOCKED);
}
