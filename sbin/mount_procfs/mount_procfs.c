/*	$OpenBSD: src/sbin/mount_procfs/mount_procfs.c,v 1.6 1997/08/24 08:07:17 downsj Exp $	*/
/*	$NetBSD: mount_procfs.c,v 1.7 1996/04/13 01:31:59 jtc Exp $	*/

/*
 * Copyright (c) 1990, 1992, 1993 Jan-Simon Pendry
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
char copyright[] =
"@(#) Copyright (c) 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount_procfs.c	8.3 (Berkeley) 3/27/94";
#else
static char rcsid[] = "$OpenBSD: src/sbin/mount_procfs/mount_procfs.c,v 1.6 1997/08/24 08:07:17 downsj Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>

#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mntopts.h"

const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	{ NULL }
};

void	usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, mntflags;

	mntflags = 0;
	while ((ch = getopt(argc, argv, "o:")) != -1)
		switch (ch) {
		case 'o':
			getmntopts(optarg, mopts, &mntflags);
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	if (mount(MOUNT_PROCFS, argv[1], mntflags, NULL)) {
		if (errno == EOPNOTSUPP)
			errx(1, "%s: Filesystem not supported by kernel",
			    argv[1]);
		else
			err(1, "%s", argv[1]);
	}
	exit(0);
}

void
usage()
{
	(void)fprintf(stderr,
		"usage: mount_procfs [-o options] /proc mount_point\n");
	exit(1);
}
