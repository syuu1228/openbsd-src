/*	$NetBSD: chpass.c,v 1.7 1995/07/28 07:01:32 phil Exp $	*/

/*-
 * Copyright (c) 1988, 1993, 1994
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
static char copyright[] =
"@(#) Copyright (c) 1988, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)chpass.c	8.4 (Berkeley) 4/2/94";
#else 
static char rcsid[] = "$NetBSD: chpass.c,v 1.7 1995/07/28 07:01:32 phil Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pw_scan.h>
#include <pw_util.h>
#include "pw_copy.h"

#include "chpass.h"
#include "pathnames.h"

char *progname = "chpass";
char *tempname;
uid_t uid;

#ifdef	YP
int use_yp;
int force_yp = 0;
extern struct passwd *ypgetpwnam(), *ypgetpwuid();
#endif

void	baduser __P((void));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char **argv;
{
	enum { NEWSH, LOADENTRY, EDITENTRY } op;
	struct passwd *pw, lpw;
	int ch, pfd, tfd;
	char *arg;

#ifdef	YP
	use_yp = _yp_check(NULL);
#endif

	op = EDITENTRY;
	while ((ch = getopt(argc, argv, "a:s:ly")) != EOF)
		switch(ch) {
		case 'a':
			op = LOADENTRY;
			arg = optarg;
			break;
		case 's':
			op = NEWSH;
			arg = optarg;
			break;
#ifdef	YP
		case 'l':
			use_yp = 0;
			break;
		case 'y':
			if (!use_yp) {
				warnx("YP not in use.");
				usage();
			}
			force_yp = 1;
			break;
#endif
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

#ifdef	YP
	if (op == LOADENTRY && use_yp)
		errx(1, "cannot load entry using NIS.\n\tUse the -l flag to load local.");
#endif
	uid = getuid();

	if (op == EDITENTRY || op == NEWSH)
		switch(argc) {
		case 0:
			pw = getpwuid(uid);
#ifdef	YP
			if (pw && !force_yp)
				use_yp = 0;
			else if (use_yp)
				pw = ypgetpwuid(uid);
#endif	/* YP */
			if (!pw)
				errx(1, "unknown user: uid %u\n", uid);
			break;
		case 1:
			pw = getpwnam(*argv);
#ifdef	YP
			if (pw && !force_yp)
				use_yp = 0;
			else if (use_yp)
				pw = ypgetpwnam(*argv);
#endif	/* YP */
			if (!pw)
				errx(1, "unknown user: %s", *argv);
			if (uid && uid != pw->pw_uid)
				baduser();
			break;
		default:
			usage();
		}

	if (op == NEWSH) {
		/* protect p_shell -- it thinks NULL is /bin/sh */
		if (!arg[0])
			usage();
		if (p_shell(arg, pw, (ENTRY *)NULL))
			pw_error((char *)NULL, 0, 1);
	}

	if (op == LOADENTRY) {
		if (uid)
			baduser();
		pw = &lpw;
		if (!pw_scan(arg, pw, (int *)NULL))
			exit(1);
	}

	/*
	 * The temporary file/file descriptor usage is a little tricky here.
	 * 1:	We start off with two fd's, one for the master password
	 *	file (used to lock everything), and one for a temporary file.
	 * 2:	Display() gets an fp for the temporary file, and copies the
	 *	user's information into it.  It then gives the temporary file
	 *	to the user and closes the fp, closing the underlying fd.
	 * 3:	The user edits the temporary file some number of times.
	 * 4:	Verify() gets an fp for the temporary file, and verifies the
	 *	contents.  It can't use an fp derived from the step #2 fd,
	 *	because the user's editor may have created a new instance of
	 *	the file.  Once the file is verified, its contents are stored
	 *	in a password structure.  The verify routine closes the fp,
	 *	closing the underlying fd.
	 * 5:	Delete the temporary file.
	 * 6:	Get a new temporary file/fd.  Pw_copy() gets an fp for it
	 *	file and copies the master password file into it, replacing
	 *	the user record with a new one.  We can't use the first
	 *	temporary file for this because it was owned by the user.
	 *	Pw_copy() closes its fp, flushing the data and closing the
	 *	underlying file descriptor.  We can't close the master
	 *	password fp, or we'd lose the lock.
	 * 7:	Call pw_mkdb() (which renames the temporary file) and exit.
	 *	The exit closes the master passwd fp/fd.
	 */
	pw_init();
	pfd = pw_lock();
	tfd = pw_tmp();

	if (op == EDITENTRY) {
		display(tfd, pw);
		edit(pw);
		(void)unlink(tempname);
		tfd = pw_tmp();
	}
		
#ifdef	YP
	if (use_yp) {
		(void)unlink(tempname);
		if (pw_yp(pw, uid))
			pw_error((char *)NULL, 0, 1);
		else
			exit(0);
	}
	else
#endif	/* YP */
	pw_copy(pfd, tfd, pw);

	if (!pw_mkdb())
		pw_error((char *)NULL, 0, 1);

	exit(0);
}

void
baduser()
{

	errx(1, "%s", strerror(EACCES));
}

void
usage()
{

#ifdef	YP
	(void)fprintf(stderr, "usage: chpass [-a list] [-s shell] [-l]%s [user]\n", use_yp?" [-y]":"");
#else
	(void)fprintf(stderr, "usage: chpass [-a list] [-s shell] [user]\n");
#endif
	exit(1);
}
