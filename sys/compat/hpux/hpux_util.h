/*	$OpenBSD: src/sys/compat/hpux/Attic/hpux_util.h,v 1.3 1996/08/02 20:35:00 niklas Exp $	 */
/*	$NetBSD: hpux_util.h,v 1.3 1995/12/08 07:45:34 thorpej Exp $	 */

/*
 * Copyright (c) 1995 Christos Zoulas
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef	_HPUX_UTIL_H_
#define	_HPUX_UTIL_H_

#include <compat/common/compat_util.h>

extern const char hpux_emul_path[];

#define	HPUX_CHECK_ALT_EXIST(p, sgp, path)	\
	CHECK_ALT_EXIST(p, sgp, hpux_emul_path, path)

#define	HPUX_CHECK_ALT_CREAT(p, sgp, path)	\
	CHECK_ALT_CREAT(p, sgp, hpux_emul_path, path)

#ifdef DEBUG_HPUX
#define DPRINTF(a)	printf a;
#else
#define DPRINTF(a)
#endif

#endif /* !_HPUX_UTIL_H_ */
