/*	$OpenBSD: src/usr.sbin/altq/altqstat/Attic/altqstat.c,v 1.1.1.1 2001/06/27 18:23:19 kjc Exp $	*/
/*	$KAME: altqstat.c,v 1.4 2000/12/03 05:44:19 kawa Exp $	*/
/*
 * Copyright (C) 1999-2000
 *	Sony Computer Science Laboratories, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY SONY CSL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL SONY CSL OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <err.h>
#ifndef NO_CURSES
#include <curses.h>
#endif

#include "quip_client.h"
#include "altqstat.h"

#define DEV_PATH	"/dev/altq"

int qdiscfd = -1;
int show_config = 0;
int interval = 5;
int no_server = 0;
char *interface = NULL;
char *qdisc_name = NULL;

stat_loop_t *stat_loop;

static void sig_handler(int sig);
static void usage(void);

static void
sig_handler(int sig)
{
	fprintf(stderr, "Exiting on signal %d\n", sig);

	close(qdiscfd);  /* close altq device */
	quip_closeserver();  /* clocse socket to altqd */
#ifndef NO_CURSES
	if (qdisc_name != NULL && strcmp(qdisc_name, "wfq") == 0)
		endwin();	/* wfqstat uses curses */
#endif
	exit(0);
}

static void 
usage(void)
{
	fprintf(stderr, "usage: altqstat [-enrs] [-c count] [-w wait] [-i interface|-I input_interface]\n");
	exit(1);
}

int
main (int argc, char **argv)
{
	int ch, raw_mode = 0;
	int qtype;
	int count = 0;
	char device[64], qname[64], input[32];

	while ((ch = getopt(argc, argv, "I:c:ei:nrsw:")) != -1) {
		switch (ch) {
		case 'I':
			sprintf(input, "_%s", optarg);
			interface = input;
			break;
		case 'c':
			count = atoi(optarg);
			break;
		case 'e':
			quip_echo = 1;
			break;
		case 'i':
			interface = optarg;
			break;
		case 'n':
			no_server = 1;
			break;
		case 'r':
			raw_mode = 1;
			quip_echo = 1;
			break;
		case 's':
			show_config = 1;
			break;
		case 'w':
			interval = atoi(optarg);
			break;
		default:
			usage();
			break;
		}
	}

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGPIPE, sig_handler);

	if (no_server == 0) {
		if (quip_openserver() < 0 && interface == NULL)
			errx(1, "you have to specify interface!");
	}

	if (raw_mode == 1) {
		quip_rawmode();
		quip_closeserver();
		exit(0);
	}

	if (show_config) {
		if (no_server)
			errx(1, "no server (-n) can't be set for show config (-s)!");
		quip_printconfig();
		quip_closeserver();
		exit(0);
	}

	interface = quip_selectinterface(interface);
	if (interface == NULL)
		errx(1, "no interface found!");

	qtype = ifname2qdisc(interface, qname);
	if (qtype == 0)
		errx(1, "altq is not attached on %s!", interface);

	qdisc_name = qname;

	stat_loop = qdisc2stat_loop(qdisc_name);
	if (stat_loop == NULL)
		errx(1, "qdisc %s is not supported!", qdisc_name);

	printf("%s: %s on interface %s\n",
	       argv[0], qdisc_name, interface);

	sprintf(device, "%s/%s", DEV_PATH, qdisc_name);
	if ((qdiscfd = open(device, O_RDONLY)) < 0)
		err(1, "can't open %s", device);

	(*stat_loop)(qdiscfd, interface, count, interval);
	/* never returns */

	exit(0);
}

/* calculate interval in sec */
double
calc_interval(struct timeval *cur_time, struct timeval *last_time)
{
	double sec;

	sec = (double)(cur_time->tv_sec - last_time->tv_sec) +
	    (double)(cur_time->tv_usec - last_time->tv_usec) / 1000000;
	return (sec);
}


/* calculate rate in bps */
double
calc_rate(u_int64_t new_bytes, u_int64_t last_bytes, double interval)
{
	double rate;

	rate = (double)(new_bytes - last_bytes) * 8 / interval;
	return (rate);
}

/* calculate packets in second */
double
calc_pps(u_int64_t new_pkts, u_int64_t last_pkts, double interval)
{
	double pps;

	pps = (double)(new_pkts - last_pkts) / interval;
	return (pps);
}

#define	R2S_BUFS	8

char *
rate2str(double rate)
{
	char *buf;
	static char r2sbuf[R2S_BUFS][16];  /* ring bufer for up to R2S_BUFS */
	static int idx = 0;

	buf = r2sbuf[idx++];
	if (idx == R2S_BUFS)
		idx = 0;

	if (rate == 0.0)
		sprintf(buf, "0");
	else if (rate >= 1000000.0)
		sprintf(buf, "%.2fM", rate / 1000000.0);
	else
		sprintf(buf, "%.2fK", rate / 1000.0);
	return (buf);
}
