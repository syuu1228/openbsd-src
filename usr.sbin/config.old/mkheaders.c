/*	$OpenBSD: src/usr.sbin/config.old/Attic/mkheaders.c,v 1.2 1997/01/12 07:43:34 downsj Exp $	*/
/*	$NetBSD: mkheaders.c,v 1.7 1995/08/17 17:22:15 thorpej Exp $	*/

/*
 * Copyright (c) 1980 Regents of the University of California.
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
/*static char sccsid[] = "from: @(#)mkheaders.c	5.7 (Berkeley) 7/1/91";*/
static char rcsid[] = "$NetBSD: mkheaders.c,v 1.7 1995/08/17 17:22:15 thorpej Exp $";
#endif /* not lint */

/*
 * Make all the .h files for the optional entries
 */

#include <stdio.h>
#include <ctype.h>
#include "config.h"
#include "y.tab.h"
#include "specfile.h"

headers()
{
	register struct file_list *fl;

	for (fl = ftab; fl != 0; fl = fl->f_next)
	    if (fl->f_was_driver)
		    do_identifiers(fl->f_needs);
	    else if (fl->f_needs_count)
		    do_identifiers(fl->f_countname);
}

do_identifiers(expr)
	register struct name_expr *expr;
{

	if (expr->left)
		do_identifiers(expr->left);
	if (expr->type == T_IDENTIFIER)
		do_count(expr->name, expr->name, 1);
	if (expr->right)
		do_identifiers(expr->right);
}

/*
 * count all the devices of a certain type and recurse to count
 * whatever the device is connected to
 */
do_count(dev, hname, search)
	register char *dev, *hname;
	int search;
{
	register struct device *dp, *mp;
	register int count;

	for (count = 0,dp = dtab; dp != 0; dp = dp->d_next)
		if (dp->d_unit != -1 && eq(dp->d_name, dev)) {
			if (dp->d_type == PSEUDO_DEVICE) {
				count =
				    dp->d_slave != UNKNOWN ? dp->d_slave : 1;
				if (dp->d_flags)
				    dev = NULL;
				break;
			}
			count++;
			/*
			 * Allow holes in unit numbering,
			 * assumption is unit numbering starts
			 * at zero.
			 */
			if (dp->d_unit + 1 > count)
				count = dp->d_unit + 1;
			if (search) {
				mp = dp->d_conn;
				if (mp != 0 && mp != TO_NEXUS &&
				    mp->d_conn != 0 && mp->d_conn != TO_NEXUS) {
					do_count(mp->d_name, hname, 0);
					search = 0;
				}
			}
		}
	do_header(dev, hname, count);
}

do_header(dev, hname, count)
	char *dev, *hname;
	int count;
{
	char *file, *name, *inw, *toheader(), *tomacro(), *get_rword();
	struct file_list *fl, *fl_head;
	FILE *inf, *outf;
	int inc = 0, oldcount;

	file = toheader(hname);
	name = tomacro(dev?dev:hname) + (dev == NULL);
	inf = fopen(file, "r");
	oldcount = -1;
	if (inf == 0) {
		outf = fopen(file, "w");
		if (outf == 0) {
			perror(file);
			exit(1);
		}
                if (dev || (!dev && count))
		        fprintf(outf, "#define %s %d\n", name, count);
		(void) fclose(outf);
		return;
	}
	fl_head = 0;
	for (;;) {
		char *cp;
		if ((inw = get_rword(inf)) == 0 || inw == (char *)EOF)
			break;
		if ((inw = get_rword(inf)) == 0 || inw == (char *)EOF)
			break;
		inw = ns(inw);
		cp = get_rword(inf);
		if (cp == 0 || cp == (char *)EOF)
			break;
		inc = atoi(cp);
		if (eq(inw, name)) {
			oldcount = inc;
			inc = count;
		}
		cp = get_rword(inf);
		if (cp == (char *)EOF)
			break;
		fl = (struct file_list *) malloc(sizeof *fl);
		bzero(fl, sizeof(*fl));
		fl->f_fn = inw;
		fl->f_type = inc;
		fl->f_next = fl_head;
		fl_head = fl;
	}
	(void) fclose(inf);
/*DEBUG  printf("%s: dev=%x, inc=%d, oldcount=%d count=%d\n",
		 file, dev, inc, oldcount, count); */
	if (!dev && inc == 0 && oldcount == -1 && count == 0)
		oldcount = 0;
	else if (!dev && count == 0)
		oldcount = -1;
	if (count == oldcount) {
		for (fl = fl_head; fl;) {
			struct file_list *fln = fl->f_next;

			free((char *)fl);
			fl = fln;
		}
		return;
	}
	if (oldcount == -1) {
		fl = (struct file_list *) malloc(sizeof *fl);
		bzero(fl, sizeof(*fl));
		fl->f_fn = name;
		fl->f_type = count;
		fl->f_next = fl_head;
		fl_head = fl;
	}
	outf = fopen(file, "w");
	if (outf == 0) {
		perror(file);
		exit(1);
	}
	for (fl = fl_head; fl;) {
		struct file_list *fln = fl->f_next;

                if (dev || (!dev && count))
                    fprintf(outf, "#define %s %u\n",
                            fl->f_fn, count ? fl->f_type : 0);
		free((char *)fl);
		fl = fln;
	}
	(void) fclose(outf);
}

/*
 * convert a dev name to a .h file name
 */
char *
toheader(dev)
	char *dev;
{
	static char hbuf[80];

	(void) strcpy(hbuf, path(dev));
	(void) strcat(hbuf, ".h");
	return (hbuf);
}

/*
 * convert a dev name to a macro name
 */
char *tomacro(dev)
	register char *dev;
{
	static char mbuf[20];
	register char *cp;

	cp = mbuf;
	*cp++ = 'N';
	while (*dev) {
		if (islower(*dev))
		    *cp++ = toupper(*dev++);
		else
		    *cp++ = *dev++;
	}
	*cp++ = 0;
	return (mbuf);
}
