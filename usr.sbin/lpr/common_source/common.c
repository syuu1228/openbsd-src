/*	$OpenBSD: src/usr.sbin/lpr/common_source/common.c,v 1.15 2002/02/16 21:28:03 millert Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
static const char sccsid[] = "@(#)common.c	8.5 (Berkeley) 4/28/95";
#else
static const char rcsid[] = "$OpenBSD: src/usr.sbin/lpr/common_source/common.c,v 1.15 2002/02/16 21:28:03 millert Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lp.h"
#include "pathnames.h"

/*
 * Routines and data common to all the line printer functions.
 */

char	*AF;		/* accounting file */
long	 BR;		/* baud rate if lp is a tty */
char	*CF;		/* name of cifplot filter (per job) */
char	*DF;		/* name of tex filter (per job) */
long	 DU;		/* daeomon user-id */
long	 FC;		/* flags to clear if lp is a tty */
char	*FF;		/* form feed string */
long	 FS;		/* flags to set if lp is a tty */
char	*GF;		/* name of graph(1G) filter (per job) */
long	 HL;		/* print header last */
char	*IF;		/* name of input filter (created per job) */
char	*LF;		/* log file for error messages */
char	*LO;		/* lock file name */
char	*LP;		/* line printer device name */
long	 MC;		/* maximum number of copies allowed */
char	*MS;		/* stty flags to set if lp is a tty */
long	 MX;		/* maximum number of blocks to copy */
char	*NF;		/* name of ditroff filter (per job) */
char	*OF;		/* name of output filter (created once) */
char	*PF;		/* name of vrast filter (per job) */
long	 PL;		/* page length */
long	 PW;		/* page width */
long	 PX;		/* page width in pixels */
long	 PY;		/* page length in pixels */
char	*RF;		/* name of fortran text filter (per job) */
char    *RG;		/* resricted group */
char	*RM;		/* remote machine name */
char	*RP;		/* remote printer name */
long	 RS;		/* restricted to those with local accounts */
long	 RW;		/* open LP for reading and writing */
long	 SB;		/* short banner instead of normal header */
long	 SC;		/* suppress multiple copies */
char	*SD;		/* spool directory */
long	 SF;		/* suppress FF on each print job */
long	 SH;		/* suppress header page */
char	*ST;		/* status file name */
char	*TF;		/* name of troff filter (per job) */
char	*TR;		/* trailer string to be output when Q empties */
char	*VF;		/* name of vplot filter (per job) */
long	 XC;		/* flags to clear for local mode */
long	 XS;		/* flags to set for local mode */

char	line[BUFSIZ];
char	*bp;		/* pointer into printcap buffer. */
char	*printer;	/* printer name */
			/* host machine name */
char	host[MAXHOSTNAMELEN];
char	*from = host;	/* client's machine name */
int	remote;		/* true if sending files to a remote host */
char	*printcapdb[2] = { _PATH_PRINTCAP, 0 };

extern uid_t	uid, euid;

static int compar(const void *, const void *);

/*
 * Create a TCP connection to host "rhost" at port "rport".
 * If rport == 0, then use the printer service port.
 * Most of this code comes from rcmd.c.
 */
int
getport(rhost, rport)
	char *rhost;
	int rport;
{
	struct hostent *hp;
	struct servent *sp;
	struct sockaddr_in sin;
	int s, timo = 1, on = 1, lport = IPPORT_RESERVED - 1;
	int err;

	/*
	 * Get the host address and port number to connect to.
	 */
	if (rhost == NULL)
		fatal("no remote host to connect to");
	bzero((char *)&sin, sizeof(sin));
	if (inet_aton(rhost, &sin.sin_addr) == 1)
		sin.sin_family = AF_INET;
	else {
		siginterrupt(SIGINT, 1);
		hp = gethostbyname(rhost);
		if (hp == NULL) {
			if (errno == EINTR && gotintr) {
				siginterrupt(SIGINT, 0);
				return (-1);
			}
			siginterrupt(SIGINT, 0);
			fatal("unknown host %s", rhost);
		}
		siginterrupt(SIGINT, 0);
		bcopy(hp->h_addr, (caddr_t)&sin.sin_addr, hp->h_length);
		sin.sin_family = hp->h_addrtype;
	}
	if (rport == 0) {
		sp = getservbyname("printer", "tcp");
		if (sp == NULL)
			fatal("printer/tcp: unknown service");
		sin.sin_port = sp->s_port;
	} else
		sin.sin_port = htons(rport);

	/*
	 * Try connecting to the server.
	 */
retry:
	seteuid(euid);
	siginterrupt(SIGINT, 1);
	s = rresvport(&lport);
	siginterrupt(SIGINT, 0);
	seteuid(uid);
	if (s < 0)
		return(-1);
	siginterrupt(SIGINT, 1);
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		err = errno;
		(void) close(s);
		siginterrupt(SIGINT, 0);
		errno = err;
		if (errno == EINTR && gotintr) {
			close(s);
			return (-1);
		}
		if (errno == EADDRINUSE) {
			lport--;
			goto retry;
		}
		if (errno == ECONNREFUSED && timo <= 16) {
			sleep(timo);
			timo *= 2;
			goto retry;
		}
		return(-1);
	}
		siginterrupt(SIGINT, 0);
	
	/* Don't bother if we get an error here.  */
	setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on);
	return(s);
}

/*
 * Getline reads a line from the control file cfp, removes tabs, converts
 *  new-line to null and leaves it in line.
 * Returns 0 at EOF or the number of characters read.
 */
int
getline(cfp)
	FILE *cfp;
{
	int linel = 0;
	char *lp = line;
	int c;

	while ((c = getc(cfp)) != '\n' && linel+1<sizeof(line)) {
		if (c == EOF)
			return(0);
		if (c == '\t') {
			do {
				*lp++ = ' ';
				linel++;
			} while ((linel & 07) != 0 && linel+1<sizeof(line));
			continue;
		}
		*lp++ = c;
		linel++;
	}
	*lp++ = '\0';
	return(linel);
}

/*
 * Scan the current directory and make a list of daemon files sorted by
 * creation time.
 * Return the number of entries and a pointer to the list.
 */
int
getq(namelist)
	struct queue *(*namelist[]);
{
	struct dirent *d;
	struct queue *q, **queue;
	int nitems;
	struct stat stbuf;
	DIR *dirp;
	int arraysz;
	

	seteuid(euid);
	dirp = opendir(SD);
	seteuid(uid);
	if (dirp== NULL)
		return(-1);
	if (fstat(dirp->dd_fd, &stbuf) < 0)
		goto errdone;

	/*
	 * Estimate the array size by taking the size of the directory file
	 * and dividing it by a multiple of the minimum size entry. 
	 */
	arraysz = (stbuf.st_size / 24);
	queue = (struct queue **)malloc(arraysz * sizeof(struct queue *));
	if (queue == NULL)
		goto errdone;

	nitems = 0;
	while ((d = readdir(dirp)) != NULL) {
		if (d->d_name[0] != 'c' || d->d_name[1] != 'f')
			continue;	/* daemon control files only */
		seteuid(euid);
		if (stat(d->d_name, &stbuf) < 0) {
			seteuid(uid);
			continue;	/* Doesn't exist */
		}
		seteuid(uid);
		q = (struct queue *)malloc(sizeof(time_t)+strlen(d->d_name)+1);
		if (q == NULL)
			goto errdone;
		q->q_time = stbuf.st_mtime;
		strcpy(q->q_name, d->d_name);
		/*
		 * Check to make sure the array has space left and
		 * realloc the maximum size.
		 */
		if (++nitems > arraysz) {
			arraysz *= 2;
			queue = (struct queue **)realloc((char *)queue,
				arraysz * sizeof(struct queue *));
			if (queue == NULL)
				goto errdone;
		}
		queue[nitems-1] = q;
	}
	closedir(dirp);
	if (nitems)
		qsort(queue, nitems, sizeof(struct queue *), compar);
	*namelist = queue;
	return(nitems);

errdone:
	closedir(dirp);
	return(-1);
}

/*
 * Compare modification times.
 */
static int
compar(p1, p2)
	const void *p1, *p2;
{
	if ((*(struct queue **)p1)->q_time < (*(struct queue **)p2)->q_time)
		return(-1);
	if ((*(struct queue **)p1)->q_time > (*(struct queue **)p2)->q_time)
		return(1);
	return(0);
}

/*
 * Figure out whether the local machine is the same
 * as the remote machine (RM) entry (if it exists).
 */
char *
checkremote()
{
	char name[MAXHOSTNAMELEN];
	struct hostent *hp;
	static char errbuf[128];
	char *rp, *rp_b;

	remote = 0;	/* assume printer is local */
	if (RM != NULL) {
		/* get the official name of the local host */
		gethostname(name, sizeof(name));
		name[sizeof(name)-1] = '\0';
		siginterrupt(SIGINT, 1);
		hp = gethostbyname(name);
		if (hp == (struct hostent *) NULL) {
			if (errno == EINTR && gotintr) {
				siginterrupt(SIGINT, 0);
				return NULL;
			}
			siginterrupt(SIGINT, 0);
			(void) snprintf(errbuf, sizeof(errbuf),
			    "unable to get official name for local machine %s",
			    name);
			return errbuf;
		} else
			strlcpy(name, hp->h_name, sizeof name);
		siginterrupt(SIGINT, 0);

		/* get the official name of RM */
		hp = gethostbyname(RM);
		if (hp == (struct hostent *) NULL) {
		    (void) snprintf(errbuf, sizeof(errbuf),
			"unable to get official name for remote machine %s",
			RM);
		    return errbuf;
		}

		/*
		 * if the two hosts are not the same,
		 * then the printer must be remote.
		 */
		if (strcasecmp(name, hp->h_name) != 0)
			remote = 1;
		else if (cgetstr(bp, "rp", &rp) > 0) {
			if (cgetent(&rp_b, printcapdb, rp) == 0) {
				if (cgetmatch(rp_b, printer) != 0)
					remote = 1;
				free(rp_b);
			} else {
				(void) snprintf(errbuf, sizeof(errbuf),
					"can't find (local) remote printer %s",
					rp);
					free(rp);
					return errbuf;
			}
			free(rp);
		}
	}
	return NULL;
}

/* sleep n milliseconds */
void
delay(n)
{
	struct timeval tdelay;

	if (n <= 0 || n > 10000)
		fatal("unreasonable delay period (%d)", n);
	tdelay.tv_sec = n / 1000;
	tdelay.tv_usec = n * 1000 % 1000000;
	(void) select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &tdelay);
}

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

void
#ifdef __STDC__
fatal(const char *msg, ...)
#else
fatal(msg, va_alist)
	char *msg;
        va_dcl
#endif
{
	extern char *__progname;
	va_list ap;
#ifdef __STDC__
	va_start(ap, msg);
#else
	va_start(ap);
#endif
	if (from != host)
		(void)printf("%s: ", host);
	(void)printf("%s: ", __progname);
	if (printer)
		(void)printf("%s: ", printer);
	(void)vprintf(msg, ap);
	va_end(ap);
	(void)putchar('\n');
	exit(1);
}
