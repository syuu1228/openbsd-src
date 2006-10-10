/*	$OpenBSD: src/lib/libc/arch/sh/gen/fpgetmask.c,v 1.1.1.1 2006/10/10 22:07:10 miod Exp $	*/
/*
 * Copyright (c) 2006 Miodrag Vallat.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice, this permission notice, and the disclaimer below
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <ieeefp.h>

fp_except
fpgetmask()
{
	register_t fpscr;

	__asm__ __volatile__ ("sts fpscr, %0" : "=r" (fpscr));
	return ((fpscr >> 7) & 0x1f);
}
