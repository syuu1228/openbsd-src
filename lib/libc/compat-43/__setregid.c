/*
 * Copyright (c) 1982, 1986, 1989, 1990, 1991 Regents of the University
 * of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)kern_prot.c	7.21 (Berkeley) 5/3/91
 *	$Id: __setregid.c,v 1.1 1994/04/10 06:32:37 cgd Exp $
 */

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

int
__setregid(rgid, egid)
	gid_t rgid, egid;
{
	static gid_t svgid = -1;

	if (svgid == -1)
		svgid = getegid();
	/*
	 * we assume that the intent of setting rgid is to be able to get
	 * back rgid priviledge. So we make sure that we will be able to
	 * do so, but do not actually set the rgid.
	 */
	if (rgid != -1 && rgid != getgid() && rgid != svgid) {
		errno = EPERM;
		return (-1);
	}
	if (egid != -1 && setegid(egid) < 0)
		return (-1);
	return (0);
}
