/*	$OpenBSD: src/sys/arch/hp300/hp300/Attic/dkbad.c,v 1.6 2003/06/02 23:27:45 millert Exp $	*/
/*	$NetBSD: dkbad.c,v 1.5 1997/04/01 03:12:15 scottr Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1990, 1993
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
 *	@(#)dkbad.c	8.2 (Berkeley) 1/12/94
 */

#ifndef NOBADSECT
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/dkbad.h>

int	isbad(struct dkbad *, int, int, int);

/*
 * Search the bad sector table looking for
 * the specified sector.  Return index if found.
 * Return -1 if not found.
 */

int
isbad(bt, cyl, trk, sec)
	struct dkbad *bt;
	int cyl, trk, sec;
{
	int i;
	long blk, bblk;

	blk = ((long)cyl << 16) + (trk << 8) + sec;
	for (i = 0; i < NBT_BAD; i++) {
		bblk = ((long)bt->bt_bad[i].bt_cyl << 16) + bt->bt_bad[i].bt_trksec;
		if (blk == bblk)
			return (i);
		if (blk < bblk || bblk < 0)
			break;
	}
	return (-1);
}
#endif
