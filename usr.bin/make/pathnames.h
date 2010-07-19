/*	$OpenBSD: src/usr.bin/make/pathnames.h,v 1.12 2010/07/19 19:46:44 espie Exp $	*/
/*	$NetBSD: pathnames.h,v 1.6 1996/11/06 17:59:21 christos Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	from: @(#)pathnames.h	5.2 (Berkeley) 6/1/90
 */

#ifdef HAS_PATH_H
# include <paths.h>
#endif
#ifndef _PATH_BSHELL
# define _PATH_BSHELL	"/bin/sh"
#endif
#ifndef _PATH_OBJDIR
#define _PATH_OBJDIR		"obj"
#endif /* !_PATH_OBJDIR */
#ifndef _PATH_OBJDIRPREFIX
#define _PATH_OBJDIRPREFIX	"/usr/obj"
#endif /* !_PATH_OBJDIRPREFIX */
#ifndef _PATH_DEFSHELLDIR
#define _PATH_DEFSHELLDIR	"/bin"
#endif /* !_PATH_DEFSHELLDIR */
#ifndef _PATH_DEFSYSMK
#define _PATH_DEFSYSMK		"sys.mk"
#endif /* ! _PATH_DEFSYSMK */
#ifndef _PATH_DEFSYSPATH
#define _PATH_DEFSYSPATH	"/usr/share/mk"
#endif /* ! _PATH_DEFSYSPATH */
