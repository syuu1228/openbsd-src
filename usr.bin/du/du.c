/*	$OpenBSD: src/usr.bin/du/du.c,v 1.19 2006/01/25 06:20:03 tedu Exp $	*/
/*	$NetBSD: du.c,v 1.11 1996/10/18 07:20:35 thorpej Exp $	*/

/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Newcomb.
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

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1989, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)du.c	8.5 (Berkeley) 5/4/95";
#else
static const char rcsid[] = "$OpenBSD: src/usr.bin/du/du.c,v 1.19 2006/01/25 06:20:03 tedu Exp $";
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/tree.h>
#include <unistd.h>
#include <util.h>


int	 linkchk(FTSENT *);
void	 prtout(quad_t, char *, int);
void	 usage(void);

int
main(int argc, char *argv[])
{
	FTS *fts;
	FTSENT *p;
	long blocksize;
	quad_t totalblocks;
	int ftsoptions, listdirs, listfiles;
	int Hflag, Lflag, aflag, cflag, hflag, kflag, sflag;
	int ch, notused, rval;
	char **save;

	save = argv;
	Hflag = Lflag = aflag = cflag = hflag = kflag = sflag = 0;
	totalblocks = 0;
	ftsoptions = FTS_PHYSICAL;
	while ((ch = getopt(argc, argv, "HLPachksxr")) != -1)
		switch (ch) {
		case 'H':
			Hflag = 1;
			Lflag = 0;
			break;
		case 'L':
			Lflag = 1;
			Hflag = 0;
			break;
		case 'P':
			Hflag = Lflag = 0;
			break;
		case 'a':
			aflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'h':
			hflag = 1;
			break;
		case 'k':
			kflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 'r':
			break;
		case 'x':
			ftsoptions |= FTS_XDEV;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/*
	 * XXX
	 * Because of the way that fts(3) works, logical walks will not count
	 * the blocks actually used by symbolic links.  We rationalize this by
	 * noting that users computing logical sizes are likely to do logical
	 * copies, so not counting the links is correct.  The real reason is
	 * that we'd have to re-implement the kernel's symbolic link traversing
	 * algorithm to get this right.  If, for example, you have relative
	 * symbolic links referencing other relative symbolic links, it gets
	 * very nasty, very fast.  The bottom line is that it's documented in
	 * the man page, so it's a feature.
	 */
	if (Hflag)
		ftsoptions |= FTS_COMFOLLOW;
	if (Lflag) {
		ftsoptions &= ~FTS_PHYSICAL;
		ftsoptions |= FTS_LOGICAL;
	}

	if (aflag) {
		if (sflag)
			usage();
		listdirs = listfiles = 1;
	} else if (sflag)
		listdirs = listfiles = 0;
	else {
		listfiles = 0;
		listdirs = 1;
	}

	if (!*argv) {
		argv = save;
		argv[0] = ".";
		argv[1] = NULL;
	}

	if (hflag)
		blocksize = 512;
	else if (kflag)
		blocksize = 1024;
	else
		(void)getbsize(&notused, &blocksize);
	blocksize /= 512;

	if ((fts = fts_open(argv, ftsoptions, NULL)) == NULL)
		err(1, "fts_open");

	for (rval = 0; (p = fts_read(fts)) != NULL;)
		switch (p->fts_info) {
		case FTS_D:			/* Ignore. */
			break;
		case FTS_DP:
			p->fts_parent->fts_number += 
			    p->fts_number += p->fts_statp->st_blocks;
			if (cflag)
				totalblocks += p->fts_statp->st_blocks;
			/*
			 * If listing each directory, or not listing files
			 * or directories and this is post-order of the
			 * root of a traversal, display the total.
			 */
			if (listdirs || (!listfiles && !p->fts_level))
				prtout((quad_t)howmany(p->fts_number, blocksize),
				    p->fts_path, hflag);
			break;
		case FTS_DC:			/* Ignore. */
			break;
		case FTS_DNR:			/* Warn, continue. */
		case FTS_ERR:
		case FTS_NS:
			warnx("%s: %s", p->fts_path, strerror(p->fts_errno));
			rval = 1;
			break;
		default:
			if (p->fts_statp->st_nlink > 1 && linkchk(p))
				break;
			/*
			 * If listing each file, or a non-directory file was
			 * the root of a traversal, display the total.
			 */
			if (listfiles || !p->fts_level)
				prtout(howmany(p->fts_statp->st_blocks, blocksize),
				    p->fts_path, hflag);
			p->fts_parent->fts_number += p->fts_statp->st_blocks;
			if (cflag)
				totalblocks += p->fts_statp->st_blocks;
		}
	if (errno)
		err(1, "fts_read");
	if (cflag) {
		prtout((quad_t)howmany(totalblocks, blocksize), "total", hflag);
	}
	exit(rval);
}


struct links_entry {
	RB_ENTRY(links_entry) entry;
	struct links_entry *fnext;
	int	 links;
	dev_t	 dev;
	ino_t	 ino;
};

int
links_cmp(struct links_entry *e1, struct links_entry *e2)
{
	if (e1->dev == e2->dev) {
		if (e1->ino == e2->ino)
			return (0);
		else
			return (e1->ino < e2->ino ? -1 : 1);
	}
	else
		return (e1->dev < e2->dev ? -1 : 1);
}

RB_HEAD(ltree, links_entry) links = RB_INITIALIZER(&links);

RB_GENERATE(ltree, links_entry, entry, links_cmp);


int
linkchk(FTSENT *p)
{
	static struct links_entry *free_list = NULL;
	static int stop_allocating = 0;
	struct links_entry ltmp, *le;
	struct stat *st;

	st = p->fts_statp;

	ltmp.ino = st->st_ino;
	ltmp.dev = st->st_dev;

	le = RB_FIND(ltree, &links, &ltmp);
	if (le != NULL) {
		/*
		 * Save memory by releasing an entry when we've seen
		 * all of it's links.
		 */
		if (--le->links <= 0) {
			RB_REMOVE(ltree, &links, le);
			/* Recycle this node through the free list */
			if (stop_allocating) {
				free(le);
			} else {
				le->fnext = free_list;
				free_list = le;
			}
		}
		return (1);
	}

	if (stop_allocating)
		return (0);

	/* Add this entry to the links cache. */
	if (free_list != NULL) {
		/* Pull a node from the free list if we can. */
		le = free_list;
		free_list = le->fnext;
	} else
		/* Malloc one if we have to. */
		le = malloc(sizeof(struct links_entry));

	if (le == NULL) {
		stop_allocating = 1;
		warnx("No more memory for tracking hard links");
		return (0);
	}

	le->dev = st->st_dev;
	le->ino = st->st_ino;
	le->links = st->st_nlink - 1;
	le->fnext = NULL;

	RB_INSERT(ltree, &links, le);

	return (0);
}

void
prtout(quad_t size, char *path, int hflag)
{
	if (!hflag)
		(void)printf("%lld\t%s\n", (long long)size, path);
	else {
		char buf[FMT_SCALED_STRSIZE];

		if (fmt_scaled(size * 512, buf) == 0)
			(void)printf("%s\t%s\n", buf, path);
		else
			(void)printf("%lld\t%s\n", (long long)size, path);
	}
}

void
usage(void)
{

	(void)fprintf(stderr,
		"usage: du [-a | -s] [-chkrx] [-H | -L | -P] [file ...]\n");
	exit(1);
}
