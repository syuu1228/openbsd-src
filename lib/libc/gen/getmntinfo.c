/*
 * Copyright (c) 1989, 1993
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
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char rcsid[] = "$OpenBSD: src/lib/libc/gen/getmntinfo.c,v 1.5 2003/06/02 20:18:34 millert Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <stdlib.h>

/*
 * Return information about mounted filesystems.
 */
int
getmntinfo(mntbufp, flags)
	struct statfs **mntbufp;
	int flags;
{
	static struct statfs *mntbuf;
	static int mntsize;
	static size_t bufsize;

	if (mntsize <= 0 && (mntsize = getfsstat(0, 0, MNT_NOWAIT)) < 0)
		return (0);
	if (bufsize > 0 && (mntsize = getfsstat(mntbuf, bufsize, flags)) < 0)
		return (0);
	while (bufsize <= mntsize * sizeof(struct statfs)) {
		if (mntbuf)
			free(mntbuf);
		bufsize = (mntsize + 1) * sizeof(struct statfs);
		if ((mntbuf = (struct statfs *)malloc(bufsize)) == 0) {
			bufsize = 0;
			return (0);
		}
		if ((mntsize = getfsstat(mntbuf, bufsize, flags)) < 0)
			return (0);
	}
	*mntbufp = mntbuf;
	return (mntsize);
}
