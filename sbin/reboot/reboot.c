/*	$OpenBSD: src/sbin/reboot/reboot.c,v 1.8 1997/06/22 22:19:11 downsj Exp $	*/
/*	$NetBSD: reboot.c,v 1.8 1995/10/05 05:36:22 mycroft Exp $	*/

/*
 * Copyright (c) 1980, 1986, 1993
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
"@(#) Copyright (c) 1980, 1986, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)reboot.c	8.1 (Berkeley) 6/5/93";
#else
static char rcsid[] = "$OpenBSD: src/sbin/reboot/reboot.c,v 1.8 1997/06/22 22:19:11 downsj Exp $";
#endif
#endif /* not lint */

#include <sys/reboot.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>

void err __P((const char *fmt, ...));
void usage __P((void));

#define _PATH_RCSHUTDOWN	"/etc/rc.shutdown"

int dohalt;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	register int i;
	struct passwd *pw;
	int ch, howto, lflag, nflag, pflag, qflag, sverrno;
	char *p, *user;

	/* Get our name */
	p = strrchr(*argv, '/');
	if(p == NULL) p = *argv;
	else p++;

	/* Nuke login shell */
	if(*p == '-') p++;

	if (!strcmp(p, "halt")) {
		dohalt = 1;
		howto = RB_HALT;
	} else
		howto = 0;
	lflag = nflag = pflag = qflag = 0;
	while ((ch = getopt(argc, argv, "dlnpq")) != -1)
		switch(ch) {
		case 'd':
			howto |= RB_DUMP;
			break;
		case 'l':		/* Undocumented; used by shutdown. */
			lflag = 1;
			break;
		case 'n':
			nflag = 1;
			howto |= RB_NOSYNC;
			break;
		case 'p':
			/* Only works if we're called as halt. */
			if (dohalt) {
				pflag = 1;
				howto |= RB_POWERDOWN;
			}
			break;
		case 'q':
			qflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (geteuid())
		err("%s", strerror(EPERM));

	if (qflag) {
		reboot(howto);
		err("%s", strerror(errno));
	}

	/* Log the reboot. */
	if (!lflag)  {
		if ((user = getlogin()) == NULL)
			user = (pw = getpwuid(getuid())) ?
			    pw->pw_name : "???";
		if (dohalt) {
			openlog("halt", 0, LOG_AUTH | LOG_CONS);
			if (pflag) {
				syslog(LOG_CRIT,
					"halted (with powerdown) by %s", user);
			} else {
				syslog(LOG_CRIT, "halted by %s", user);
			}
		} else {
			openlog("reboot", 0, LOG_AUTH | LOG_CONS);
			syslog(LOG_CRIT, "rebooted by %s", user);
		}
	}
	logwtmp("~", "shutdown", "");

	/*
	 * Do a sync early on, so disks start transfers while we're off
	 * killing processes.  Don't worry about writes done before the
	 * processes die, the reboot system call syncs the disks.
	 */
	if (!nflag)
		sync();

	/* Just stop init -- if we fail, we'll restart it. */
	if (kill(1, SIGTSTP) == -1)
		err("SIGTSTP init: %s", strerror(errno));

	/* Ignore the SIGHUP we get when our parent shell dies. */
	(void)signal(SIGHUP, SIG_IGN);

	if (access(_PATH_RCSHUTDOWN, R_OK) != -1) {
		pid_t pid;

		switch ((pid = fork())) {
		case -1:
			break;
		case 0:
			execl(_PATH_BSHELL, "sh", _PATH_RCSHUTDOWN, NULL);
			exit(1);
		default:
			waitpid(pid, NULL, 0);
		}
	}

	/* Send a SIGTERM first, a chance to save the buffers. */
	if (kill(-1, SIGTERM) == -1) {
		/*
		 * If ESRCH, everything's OK: we're the only non-system
		 * process!  That can happen e.g. via 'exec reboot' in
		 * single-user mode.
		 */
		if (errno != ESRCH) {
			(void)fprintf(stderr, "%s: SIGTERM processes: %s",
			    dohalt ? "halt" : "reboot", strerror(errno));
			goto restart;
		}
	}

	/*
	 * After the processes receive the signal, start the rest of the
	 * buffers on their way.  Wait 5 seconds between the SIGTERM and
	 * the SIGKILL to give everybody a chance.
	 */
	sleep(2);
	if (!nflag)
		sync();
	sleep(3);

	for (i = 1;; ++i) {
		if (kill(-1, SIGKILL) == -1) {
			if (errno == ESRCH)
				break;
			goto restart;
		}
		if (i > 5) {
			(void)fprintf(stderr,
			    "WARNING: some process(es) wouldn't die\n");
			break;
		}
		(void)sleep(2 * i);
	}

	reboot(howto);
	/* FALLTHROUGH */

restart:
	sverrno = errno;
	err("%s%s", kill(1, SIGHUP) == -1 ? "(can't restart init): " : "",
	    strerror(sverrno));
	/* NOTREACHED */
}

void
usage()
{
	(void)fprintf(stderr, "usage: %s [-nqd]\n", dohalt ? "halt" : "reboot");
	exit(1);
}

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

void
#if __STDC__
err(const char *fmt, ...)
#else
err(fmt, va_alist)
	char *fmt;
        va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void)fprintf(stderr, "%s: ", dohalt ? "halt" : "reboot");
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	exit(1);
	/* NOTREACHED */
}
