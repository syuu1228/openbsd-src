/*	$OpenBSD: src/games/trek/systemname.c,v 1.2 1998/08/19 07:42:10 pjanzen Exp $	*/
/*	$NetBSD: systemname.c,v 1.3 1995/04/22 10:59:32 cgd Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
 */

#ifndef lint
#if 0
static char sccsid[] = "@(#)systemname.c	8.1 (Berkeley) 5/31/93";
#else
static char rcsid[] = "$OpenBSD: src/games/trek/systemname.c,v 1.2 1998/08/19 07:42:10 pjanzen Exp $";
#endif
#endif /* not lint */

#include "trek.h"

/*
**  RETRIEVE THE STARSYSTEM NAME
**
**	Very straightforward, this routine just gets the starsystem
**	name.  It returns zero if none in the specified quadrant
**	(which, by the way, is passed it).
**
**	This routine knows all about such things as distressed
**	starsystems, etc.
*/

char *systemname(q1)
struct quad	*q1;
{
	register struct quad	*q;
	register int		i;

	q = q1;

	i = q->qsystemname;
	if (i & Q_DISTRESSED)
		i = Event[i & Q_SYSTEM].systemname;

	i &= Q_SYSTEM;
	if (i == 0)
		return (0);
	return (Systemname[i]);
}
