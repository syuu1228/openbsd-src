/*	$OpenBSD: src/usr.sbin/httpd/src/support/rotatelogs.c,v 1.10 2008/10/06 20:50:18 mbalmer Exp $ */

/*
 * Simple program to rotate Apache logs without having to kill the server.
 *
 * Contributed by Ben Laurie <ben@algroup.co.uk>
 *
 * 12 Mar 1996
 */

#include <time.h>
#include <errno.h>
#include <fcntl.h>

#include "ap_config.h"

#define BUFSIZE        65536
#define ERRMSGSZ       82
#ifndef MAX_PATH
#define MAX_PATH       1024
#endif

int
main(int argc, char *argv[])
{
	char buf[BUFSIZE], buf2[MAX_PATH], errbuf[ERRMSGSZ];
	time_t tLogEnd = 0, tRotation;
	int nLogFD = -1, nLogFDprev = -1, nMessCount = 0, nRead, nWrite;
	int utc_offset = 0;
	int use_strftime = 0;
	time_t now;
	char *szLogRoot;

	if (argc < 3) {
		fprintf(stderr, "usage: %s logfile rotationtime [offset]\n\n",
		    argv[0]);
		fprintf(stderr, "Add this:\n\nTransferLog \"|%s /some/where "
		    "86400\"\n\n", argv[0]);
		fprintf(stderr,
		    "to httpd.conf. The generated name will be /some/where.nnnn"
		    " where nnnn is the\nsystem time at which the log nominally"
		    " starts (N.B. this time will always be a\nmultiple of the "
		    "rotation time, so you can synchronize cron scripts with "
		    "it).\nAt the end of each rotation time a new log is "
		    "started.\n");
		exit(1);
	}

	szLogRoot = argv[1];
	if (argc >= 4)
		utc_offset = atoi(argv[3]) * 60;

	tRotation = atoi(argv[2]);
	if (tRotation <= 0) {
		fprintf(stderr, "Rotation time must be > 0\n");
		exit(6);
	}

	use_strftime = (strstr(szLogRoot, "%") != NULL);
	for (;;) {
		nRead = read(0, buf, sizeof buf);
		now = time(NULL) + utc_offset;
		if (nRead == 0)
			exit(3);
		if (nRead < 0)
			if (errno != EINTR)
				exit(4);
		if (nLogFD >= 0 && (now >= tLogEnd || nRead < 0)) {
			nLogFDprev = nLogFD;
			nLogFD = -1;
		}
		if (nLogFD < 0) {
			time_t tLogStart = (now / tRotation) * tRotation;
			if (use_strftime) {
				struct tm *tm_now;
				tm_now = gmtime(&tLogStart);
				strftime(buf2, sizeof(buf2), szLogRoot, tm_now);
			} else
				snprintf(buf2, sizeof(buf2), "%s.%010d",
				    szLogRoot, (int)tLogStart);

			tLogEnd = tLogStart + tRotation;
			do {
				nLogFD = open(buf2, O_WRONLY | O_CREAT |
				    O_APPEND, 0666);
				if (nLogFD < 0 && nLogFDprev == -1) {
					fprintf(stderr, "rotatelogs: can't "
					    "open %s for writing: %s\n", buf2,
					    strerror(errno));
					sleep(2);
				}
			} while (nLogFD < 0 && nLogFDprev == -1);
			if (nLogFD < 0) {
				/*
				 * Uh-oh. Failed to open the new log file. Try
				 * to clear the previous log file, note the
				 * lost log entries, and keep on truckin'.
				 */
				nLogFD = nLogFDprev;
				snprintf(errbuf, sizeof(errbuf),
				    "Resetting log file due to error opening "
				    "new log file. %10d messages lost.\n",
				    nMessCount); 
				nWrite = strlen(errbuf);
				ftruncate(nLogFD, 0);
				write(nLogFD, errbuf, nWrite);
			} else
				close(nLogFDprev);
			nMessCount = 0;
		}
		do {
			nWrite = write(nLogFD, buf, nRead);
		} while (nWrite < 0 && errno == EINTR);
		if (nWrite != nRead) {
			nMessCount++;
			snprintf(errbuf, sizeof(errbuf),
			    "Error writing to log file. "
			    "%10d messages lost.\n", nMessCount);
			nWrite = strlen(errbuf);
			ftruncate(nLogFD, 0);
			write (nLogFD, errbuf, nWrite);
		} else
			nMessCount++; 
	}

	/* We never get here, but suppress the compile warning */
	return 0;
}
