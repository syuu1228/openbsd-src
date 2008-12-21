/*	$OpenBSD: src/sys/sys/stdarg.h,v 1.6 2008/12/21 09:59:24 ragge Exp $ */
/*
 * Copyright (c) 2003, 2004  Marc espie <espie@openbsd.org>
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

#ifndef _STDARG_H_
#define _STDARG_H_

#include <sys/cdefs.h>

#if (defined(__GNUC__) && __GNUC__ >= 3) || defined(__PCC__)

/* Define __gnuc_va_list.  */

#ifndef __GNUC_VA_LIST
#define __GNUC_VA_LIST
typedef __builtin_va_list __gnuc_va_list;
#endif

/* Note that the type used in va_arg is supposed to match the
   actual type **after default promotions**.
   Thus, va_arg (..., short) is not valid.  */

#define va_start(ap, last)	__builtin_stdarg_start((ap), last)
#define va_end			__builtin_va_end
#define va_arg			__builtin_va_arg
#define __va_copy(dst, src)	__builtin_va_copy((dst),(src))

typedef __gnuc_va_list va_list;

#else
#include <machine/stdarg.h>
#endif

#if __ISO_C_VISIBLE >= 1999
#define	va_copy(dst, src)	__va_copy((dst), (src))
#endif

#endif /* not _STDARG_H_ */
