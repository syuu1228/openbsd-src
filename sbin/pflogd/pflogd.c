/*	$OpenBSD: src/sbin/pflogd/pflogd.c,v 1.23 2003/10/22 18:51:55 canacar Exp $	*/

/*
 * Copyright (c) 2001 Theo de Raadt
 * Copyright (c) 2001 Can Erkin Acar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pcap-int.h>
#include <pcap.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <util.h>
#include "pflogd.h"

pcap_t *hpcap;
pcap_dumper_t *dpcap;

int Debug = 0;
int snaplen = DEF_SNAPLEN;

volatile sig_atomic_t gotsig_close, gotsig_alrm, gotsig_hup;

char *filename = PFLOGD_LOG_FILE;
char *interface = PFLOGD_DEFAULT_IF;
char *filter = NULL;

char errbuf[PCAP_ERRBUF_SIZE];

int log_debug = 0;
unsigned int delay = FLUSH_DELAY;

char *copy_argv(char * const *argv);
int   init_pcap(void);
void  logmsg(int priority, const char *message, ...);
int   reset_dump(void);
void  sig_alrm(int);
void  sig_close(int);
void  sig_hup(int);
void  usage(void);


char *
copy_argv(char * const *argv)
{
	size_t len = 0, n;
	char *buf;

	if (argv == NULL)
		return (NULL);

	for (n = 0; argv[n]; n++)
		len += strlen(argv[n])+1;
	if (len == 0)
		return (NULL);

	buf = malloc(len);
	if (buf == NULL)
		return (NULL);

	strlcpy(buf, argv[0], len);
	for (n = 1; argv[n]; n++) {
		strlcat(buf, " ", len);
		strlcat(buf, argv[n], len);
	}
	return (buf);
}

void
logmsg(int pri, const char *message, ...)
{
	va_list ap;
	va_start(ap, message);

	if (log_debug) {
		vfprintf(stderr, message, ap);
		fprintf(stderr, "\n");
	} else
		vsyslog(pri, message, ap);
	va_end(ap);
}

__dead void
usage(void)
{
	fprintf(stderr, "usage: pflogd [-D] [-d delay] [-f filename] ");
	fprintf(stderr, "[-s snaplen] [expression]\n");
	exit(1);
}

void
sig_close(int sig)
{
	gotsig_close = 1;
}

void
sig_hup(int sig)
{
	gotsig_hup = 1;
}

void
sig_alrm(int sig)
{
	gotsig_alrm = 1;
}

void
set_pcap_filter(void)
{
	struct bpf_program bprog;

	if (pcap_compile(hpcap, &bprog, filter, PCAP_OPT_FIL, 0) < 0)
		logmsg(LOG_WARNING, "%s", pcap_geterr(hpcap));
	else {
		if (pcap_setfilter(hpcap, &bprog) < 0)
			logmsg(LOG_WARNING, "%s", pcap_geterr(hpcap));
		pcap_freecode(&bprog);
	}
}

int
init_pcap(void)
{
	hpcap = pcap_open_live(interface, snaplen, 1, PCAP_TO_MS, errbuf);
	if (hpcap == NULL) {
		logmsg(LOG_ERR, "Failed to initialize: %s", errbuf);
		hpcap = NULL;
		return (-1);
	}

	if (pcap_datalink(hpcap) != DLT_PFLOG) {
		logmsg(LOG_ERR, "Invalid datalink type");
		pcap_close(hpcap);
		hpcap = NULL;
		return (-1);
	}

	set_pcap_filter();

	snaplen = pcap_snapshot(hpcap);

	/* lock */
	if (ioctl(pcap_fileno(hpcap), BIOCLOCK) < 0) {
		logmsg(LOG_ERR, "BIOCLOCK: %s", strerror(errno));
		return (-1);
	}

	return (0);
}

int
reset_dump(void)
{
	struct pcap_file_header hdr;
	struct stat st;
	int fd;
	FILE *fp;

	if (hpcap == NULL)
		return (1);
	if (dpcap) {
		pcap_dump_close(dpcap);
		dpcap = 0;
	}

	/*
	 * Basically reimplement pcap_dump_open() because it truncates
	 * files and duplicates headers and such.
	 */
	fd = priv_open_log();
	if (fd < 0)
		return (1);

	fp = fdopen(fd, "a+");

	if (fp == NULL) {
		logmsg(LOG_ERR, "Error: %s: %s", filename, strerror(errno));
		return (1);
	}
	if (fstat(fileno(fp), &st) == -1) {
		logmsg(LOG_ERR, "Error: %s: %s", filename, strerror(errno));
		return (1);
	}

	dpcap = (pcap_dumper_t *)fp;

#define TCPDUMP_MAGIC 0xa1b2c3d4

	if (st.st_size == 0) {
		if (snaplen != pcap_snapshot(hpcap)) {
			logmsg(LOG_NOTICE, "Using snaplen %d", snaplen);
			if (priv_set_snaplen(snaplen)) {
				logmsg(LOG_WARNING,
				    "Failed, using old settings");
			}
		}
		hdr.magic = TCPDUMP_MAGIC;
		hdr.version_major = PCAP_VERSION_MAJOR;
		hdr.version_minor = PCAP_VERSION_MINOR;
		hdr.thiszone = hpcap->tzoff;
		hdr.snaplen = hpcap->snapshot;
		hdr.sigfigs = 0;
		hdr.linktype = hpcap->linktype;

		if (fwrite((char *)&hdr, sizeof(hdr), 1, fp) != 1) {
			dpcap = NULL;
			fclose(fp);
			return (-1);
		}
		return (0);
	}

	/*
	 * XXX Must read the file, compare the header against our new
	 * options (in particular, snaplen) and adjust our options so
	 * that we generate a correct file.
	 */
	(void) fseek(fp, 0L, SEEK_SET);
	if (fread((char *)&hdr, sizeof(hdr), 1, fp) == 1) {
		if (hdr.magic != TCPDUMP_MAGIC ||
		    hdr.version_major != PCAP_VERSION_MAJOR ||
		    hdr.version_minor != PCAP_VERSION_MINOR ||
		    hdr.linktype != hpcap->linktype) {
			logmsg(LOG_ERR,
			    "Invalid/incompatible log file, move it away");
			fclose(fp);
			return (1);
		}
		if (hdr.snaplen != snaplen) {
			logmsg(LOG_WARNING,
			    "Existing file has different snaplen %u, using it",
			    hdr.snaplen);
			if (priv_set_snaplen(hdr.snaplen)) {
				logmsg(LOG_WARNING,
				    "Failed, using old settings, offset %llu",
				    (unsigned long long) st.st_size);
			}
		}
	}

	(void) fseek(fp, 0L, SEEK_END);
	return (0);
}

int
main(int argc, char **argv)
{
	struct pcap_stat pstat;
	int ch, np;

	while ((ch = getopt(argc, argv, "Dd:s:f:")) != -1) {
		switch (ch) {
		case 'D':
			Debug = 1;
			break;
		case 'd':
			delay = atoi(optarg);
			if (delay < 5 || delay > 60*60)
				usage();
			break;
		case 'f':
			filename = optarg;
			break;
		case 's':
			snaplen = atoi(optarg);
			if (snaplen <= 0)
				snaplen = DEF_SNAPLEN;
			break;
		default:
			usage();
		}

	}

	log_debug = Debug;
	argc -= optind;
	argv += optind;

	if (!Debug) {
		openlog("pflogd", LOG_PID | LOG_CONS, LOG_DAEMON);
		if (daemon(0, 0)) {
			logmsg(LOG_WARNING, "Failed to become daemon: %s",
			    strerror(errno));
		}
		pidfile(NULL);
	}

	(void)umask(S_IRWXG | S_IRWXO);

	/* filter will be used by the privileged process */
	if (argc) {
		filter = copy_argv(argv);
		if (filter == NULL)
			logmsg(LOG_NOTICE, "Failed to form filter expression");
	}

	/* initialize pcap before dropping privileges */
	if (init_pcap()) {
		logmsg(LOG_ERR, "Exiting, init failure");
		exit(1);
	}

	/* Privilege separation begins here */	
	if (priv_init()) {
		logmsg(LOG_ERR, "unable to privsep");
		exit(1);
	}

	/* Process is now unprivileged and inside a chroot */
	signal(SIGTERM, sig_close);
	signal(SIGINT, sig_close);
	signal(SIGQUIT, sig_close);
	signal(SIGALRM, sig_alrm);
	signal(SIGHUP, sig_hup);
	alarm(delay);

	if (reset_dump()) {
		logmsg(LOG_ERR, "Failed to open log file %s", filename);
		pcap_close(hpcap);
		exit(1);
	}

	while (1) {
		np = pcap_dispatch(hpcap, PCAP_NUM_PKTS, pcap_dump, (u_char *)dpcap);
		if (np < 0)
			logmsg(LOG_NOTICE, "%s", pcap_geterr(hpcap));

		if (gotsig_close)
			break;
		if (gotsig_hup) {
			if (reset_dump()) {
				logmsg(LOG_ERR, "Failed to open log file!");
				break;
			}
			gotsig_hup = 0;
		}

		if (gotsig_alrm) {
			/* XXX pcap_dumper is an incomplete type which libpcap
			 * casts to a FILE* currently.  For now it is safe to
			 * make the same assumption, however this may change
			 * in the future.
			 */
			if (dpcap) {
				if (fflush((FILE *)dpcap) == EOF) {
					break;
				}
			}
			gotsig_alrm = 0;
			alarm(delay);
		}
	}

	logmsg(LOG_NOTICE, "Exiting due to signal");
	if (dpcap)
		pcap_dump_close(dpcap);

	if (pcap_stats(hpcap, &pstat) < 0)
		logmsg(LOG_WARNING, "Reading stats: %s", pcap_geterr(hpcap));
	else
		logmsg(LOG_NOTICE, "%u packets received, %u dropped",
		    pstat.ps_recv, pstat.ps_drop);

	pcap_close(hpcap);
	if (!Debug)
		closelog();
	return (0);
}
