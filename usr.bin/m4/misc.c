/*	$OpenBSD: src/usr.bin/m4/misc.c,v 1.20 2000/07/27 17:44:33 espie Exp $	*/
/*	$NetBSD: misc.c,v 1.6 1995/09/28 05:37:41 tls Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ozan Yigit at York University.
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
static char sccsid[] = "@(#)misc.c	8.1 (Berkeley) 6/6/93";
#else
static char rcsid[] = "$OpenBSD: src/usr.bin/m4/misc.c,v 1.20 2000/07/27 17:44:33 espie Exp $";
#endif
#endif /* not lint */

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <err.h>
#include "mdef.h"
#include "stdd.h"
#include "extern.h"
#include "pathnames.h"


char *ep;		/* first free char in strspace */
static char *strspace;	/* string space for evaluation */
static char *endest;	/* end of string space	       */
static size_t strsize = STRSPMAX;
static size_t bufsize = BUFSIZE;

char *buf;			/* push-back buffer	       */
char *bufbase;			/* the base for current ilevel */
char *bbase[MAXINP];		/* the base for each ilevel    */
char *bp; 			/* first available character   */
static char *endpbb;			/* end of push-back buffer     */


static void enlarge_bufspace __P((void));
static void enlarge_strspace __P((void));
/*
 * find the index of second str in the first str.
 */
ptrdiff_t
indx(s1, s2)
	const char *s1;
	const char *s2;
{
	char *t;

	t = strstr(s1, s2);
	if (t == NULL)
		return (-1);
	else
		return (t - s1);
}
/*
 *  putback - push character back onto input
 */
void
putback(c)
	int c;
{
	if (c == EOF)
		return;
	if (bp >= endpbb)
		enlarge_bufspace();
	*bp++ = c;
}

/*
 *  pbstr - push string back onto input
 *          putback is replicated to improve
 *          performance.
 */
void
pbstr(s)
	const char *s;
{
	size_t n;

	n = strlen(s);
	while (endpbb - bp <= n)
		enlarge_bufspace();
	while (n > 0)
		*bp++ = s[--n];
}

/*
 *  pbnum - convert number to string, push back on input.
 */
void
pbnum(n)
	int n;
{
	int num;

	num = (n < 0) ? -n : n;
	do {
		putback(num % 10 + '0');
	}
	while ((num /= 10) > 0);

	if (n < 0)
		putback('-');
}

/*
 *  pbunsigned - convert unsigned long to string, push back on input.
 */
void
pbunsigned(n)
	unsigned long n;
{
	do {
		putback(n % 10 + '0');
	}
	while ((n /= 10) > 0);
}

void 
initspaces()
{
	int i;

	strspace = xalloc(strsize+1);
	ep = strspace;
	endest = strspace+strsize;
	buf = (char *)xalloc(bufsize);
	bufbase = buf;
	bp = buf;
	endpbb = buf + bufsize;
	for (i = 0; i < MAXINP; i++)
		bbase[i] = buf;
}

static void 
enlarge_strspace()
{
	char *newstrspace;
	int i;

	strsize *= 2;
	newstrspace = malloc(strsize + 1);
	if (!newstrspace)
		errx(1, "string space overflow");
	memcpy(newstrspace, strspace, strsize/2);
	for (i = 0; i <= sp; i++) 
		if (sstack[i])
			mstack[i].sstr = (mstack[i].sstr - strspace) 
			    + newstrspace;
	ep = (ep-strspace) + newstrspace;
	free(strspace);
	strspace = newstrspace;
	endest = strspace + strsize;
}

static void
enlarge_bufspace()
{
	char *newbuf;
	int i;

	bufsize *= 2;
	newbuf = realloc(buf, bufsize);
	if (!newbuf)
		errx(1, "too many characters pushed back");
	for (i = 0; i < MAXINP; i++)
		bbase[i] = (bbase[i]-buf)+newbuf;
	bp = (bp-buf)+newbuf;
	bufbase = (bufbase-buf)+newbuf;
	buf = newbuf;
	endpbb = buf+bufsize;
}

/*
 *  chrsave - put single char on string space
 */
void
chrsave(c)
	int c;
{
	if (ep >= endest) 
		enlarge_strspace();
	*ep++ = c;
}

/*
 * read in a diversion file, and dispose it.
 */
void
getdiv(n)
	int n;
{
	int c;

	if (active == outfile[n])
		errx(1, "undivert: diversion still active");
	rewind(outfile[n]);
	while ((c = getc(outfile[n])) != EOF)
		putc(c, active);
	(void) fclose(outfile[n]);
	outfile[n] = NULL;
}

void
onintr(signo)
	int signo;
{
	errx(1, "interrupted.");
}

/*
 * killdiv - get rid of the diversion files
 */
void
killdiv()
{
	int n;

	for (n = 0; n < maxout; n++)
		if (outfile[n] != NULL) {
			(void) fclose(outfile[n]);
		}
}

/*
 * resizedivs: allocate more diversion files */
void
resizedivs(n)
	int n;
{
	int i;

	outfile = (FILE **)realloc(outfile, sizeof(FILE *) * n);
	if (outfile == NULL)
		    errx(1, "too many diverts %d", n);
	for (i = maxout; i < n; i++)
		outfile[i] = NULL;
	maxout = n;
}

void *
xalloc(n)
	size_t n;
{
	char *p = malloc(n);

	if (p == NULL)
		err(1, "malloc");
	return p;
}

char *
xstrdup(s)
	const char *s;
{
	char *p = strdup(s);
	if (p == NULL)
		err(1, "strdup");
	return p;
}

void
usage()
{
	fprintf(stderr, "usage: m4 [-Dname[=val]] [-Uname] [-I dirname...]\n");
	exit(1);
}

int 
obtain_char(f)
	struct input_file *f;
{
	if (f->c == EOF)
		return EOF;
	else if (f->c == '\n')
		f->lineno++;

	f->c = fgetc(f->file);
	return f->c;
}

void 
set_input(f, real, name)
	struct input_file *f;
	FILE *real;
	const char *name;
{
	f->file = real;
	f->lineno = 1;
	f->c = 0;
	f->name = xstrdup(name);
}

void 
release_input(f)
	struct input_file *f;
{
	if (f->file != stdin)
	    fclose(f->file);
	f->c = EOF;
	/*
	 * XXX can't free filename, as there might still be 
	 * error information pointing to it.
	 */
}

void
doprintlineno(f)
	struct input_file *f;
{
	pbunsigned(f->lineno);
}

void
doprintfilename(f)
	struct input_file *f;
{
	pbstr(f->name);
}
