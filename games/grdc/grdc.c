/*	$OpenBSD: src/games/grdc/grdc.c,v 1.8 2000/01/24 07:53:57 pjanzen Exp $	*/
/*
 * Grand digital clock for curses compatible terminals
 * Usage: grdc [-s] [n]   -- run for n seconds (default infinity)
 * Flags: -s: scroll
 *
 * modified 10-18-89 for curses (jrl)
 * 10-18-89 added signal handling
 */

#include <sys/types.h>
#include <curses.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define YBASE	10
#define XBASE	10
#define XLENGTH 58
#define YDEPTH  7

struct timespec now;
struct tm *tm;

short disp[11] = {
	075557, 011111, 071747, 071717, 055711,
	074717, 074757, 071111, 075757, 075717, 002020
};
long old[6], next[6], new[6], mask;
char scrol;

int sigtermed=0;

int hascolor = 0;

void set __P((int, int));
void standt __P((int));
void movto __P((int, int));
void usage __P((void));

void
sighndl(signo)
	int signo;
{
	sigtermed=signo;
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	long t, a;
	int i, j, s, k;
	int n = 0;
	struct timeval nowtv;
	struct timespec delay;
	time_t then;
	char *ep;

	/* revoke privs */
	setegid(getgid());
	setgid(getgid());

	scrol = 0;
	while ((i = getopt(argc, argv, "sh")) != -1)
		switch (i) {
		case 's':
			scrol = 1;
			break;
		case 'h':
		case '?':
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

	if (argc > 1)
		usage();
	if (argc == 1) {
		t = strtol(*argv, &ep, 10);
		if ((*argv)[0] == '\0' || *ep != '\0')
			usage();
		if (t < 1 || t >= INT_MAX) {
			fprintf(stderr, "number of seconds is out of range");
			usage();
		}
		n = t;
	}

	initscr();

	signal(SIGINT,sighndl);
	signal(SIGTERM,sighndl);
	signal(SIGHUP,sighndl);

	cbreak();
	noecho();

	hascolor = has_colors();

	if(hascolor) {
		start_color();
		init_pair(1, COLOR_BLACK, COLOR_RED);
		init_pair(2, COLOR_RED, COLOR_BLACK);
		init_pair(3, COLOR_WHITE, COLOR_BLACK);
		attrset(COLOR_PAIR(2));
	}

	curs_set(0);
	clear();
	refresh();
	if(hascolor) {
		attrset(COLOR_PAIR(3));

		mvaddch(YBASE - 2,  XBASE - 3, ACS_ULCORNER);
		hline(ACS_HLINE, XLENGTH);
		mvaddch(YBASE - 2,  XBASE - 2 + XLENGTH, ACS_URCORNER);

		mvaddch(YBASE + YDEPTH - 1,  XBASE - 3, ACS_LLCORNER);
		hline(ACS_HLINE, XLENGTH);
		mvaddch(YBASE + YDEPTH - 1,  XBASE - 2 + XLENGTH, ACS_LRCORNER);

		move(YBASE - 1,  XBASE - 3);
		vline(ACS_VLINE, YDEPTH);

		move(YBASE - 1,  XBASE - 2 + XLENGTH);
		vline(ACS_VLINE, YDEPTH);

		attrset(COLOR_PAIR(2));
	}
	gettimeofday(&nowtv, NULL);
	TIMEVAL_TO_TIMESPEC(&nowtv, &now);
	then = now.tv_sec - 1;
	do {
		mask = 0;
		tm = localtime(&now.tv_sec);
		set(tm->tm_sec%10, 0);
		set(tm->tm_sec/10, 4);
		set(tm->tm_min%10, 10);
		set(tm->tm_min/10, 14);
		set(tm->tm_hour%10, 20);
		set(tm->tm_hour/10, 24);
		set(10, 7);
		set(10, 17);
		for(k=0; k<6; k++) {
			if(scrol) {
				for(i=0; i<5; i++)
					new[i] = (new[i]&~mask) | (new[i+1]&mask);
				new[5] = (new[5]&~mask) | (next[k]&mask);
			} else
				new[k] = (new[k]&~mask) | (next[k]&mask);
			next[k] = 0;
			for(s=1; s>=0; s--) {
				standt(s);
				for(i=0; i<6; i++) {
					if((a = (new[i]^old[i])&(s ? new : old)[i]) != 0) {
						for(j=0,t=1<<26; t; t>>=1,j++) {
							if(a&t) {
								if(!(a&(t<<1))) {
									movto(YBASE + i, XBASE + 2*j);
								}
								addstr("  ");
							}
						}
					}
					if(!s) {
						old[i] = new[i];
					}
				}
				if(!s) {
					refresh();
				}
			}
		}
		movto(6, 0);
		refresh();
		gettimeofday(&nowtv, NULL);
		TIMEVAL_TO_TIMESPEC(&nowtv, &now);
		/* On some systems nanosleep() can return early, before a full
		 * second has passed according to gettimeofday().
		 */
		if (now.tv_sec == then) {
			delay.tv_sec = 1;
			now.tv_sec++;
		} else
			delay.tv_sec = 0;
		delay.tv_nsec = (1000000000 - now.tv_nsec);
		nanosleep(&delay, NULL);
		then = now.tv_sec;
		now.tv_sec++;

		if (sigtermed) {
			standend();
			clear();
			refresh();
			endwin();
			fprintf(stderr, "grdc terminated by signal %d\n", sigtermed);
			exit(1);
		}
	} while(--n);
	standend();
	clear();
	refresh();
	endwin();
	return(0);
}

void
set(int t, int n)
{
	int i, m;

	m = 7<<n;
	for(i=0; i<5; i++) {
		next[i] |= ((disp[t]>>(4-i)*3)&07)<<n;
		mask |= (next[i]^old[i])&m;
	}
	if(mask&m)
		mask |= m;
}

void
standt(int on)
{
	if (on) {
		if(hascolor) {
			attron(COLOR_PAIR(1));
		} else {
			attron(A_STANDOUT);
		}
	} else {
		if(hascolor) {
			attron(COLOR_PAIR(2));
		} else {
			attroff(A_STANDOUT);
		}
	}
}

void
movto(int line, int col)
{
	move(line, col);
}

void
usage()
{
	(void)fprintf(stderr, "usage: grdc [-s] [number_of_seconds]\n");
	exit(1);
}
