/*	$OpenBSD: src/games/snake/snake.c,v 1.2 1999/04/20 23:01:12 pjanzen Exp $	*/
/*	$NetBSD: snake.c,v 1.8 1995/04/29 00:06:41 mycroft Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
"@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)snake.c	8.2 (Berkeley) 1/7/94";
#else
static char rcsid[] = "$OpenBSD: src/games/snake/snake.c,v 1.2 1999/04/20 23:01:12 pjanzen Exp $";
#endif
#endif /* not lint */

/*
 * snake - crt hack game.
 *
 * You move around the screen with arrow keys trying to pick up money
 * without getting eaten by the snake.  hjkl work as in vi in place of
 * arrow keys.  You can leave at the exit any time.
 *
 * compile as follows:
 *	cc -O snake.c move.c -o snake -lm -ltermlib
 */

#include <sys/param.h>

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>

#include "snake.h"
#include "pathnames.h"

struct point you;
struct point money;
struct point finish;
struct point snake[6];

int	loot, penalty;
int	long tl, tm=0L;
int	moves;
char	stri[BSIZE];
char	*p;
char	ch, savec;
char	*kl, *kr, *ku, *kd;
int	fast=1;
int	repeat=1;
time_t	tv;
char	*tn;

int rawscores;
#ifdef LOGGING
FILE	*logfile;
#endif

int
main(argc, argv)
	int argc;
	char **argv;
{
	int	ch, i;
	char	*p, **av;

	/* don't create the score file if it doesn't exist. */
	rawscores = open(_PATH_RAWSCORES, O_RDWR, 0664);
#ifdef LOGGING
	logfile = fopen(_PATH_LOGFILE, "a");
#endif

	/* revoke privs */
	setegid(getgid());
	setgid(getgid());

	(void)time(&tv);
	srandom((int)tv);

	/* check to see if we were called as snscore */
	av = argv;
	p = strrchr(*av, '/');
	if (p++ == NULL)
		p = *av;
	if (strcmp(p,"snscore") == 0) {
		snscore(rawscores, 0);
		exit(0);
	}

	while ((ch = getopt(argc, argv, "hl:sw:")) != -1)
		switch ((char)ch) {
#if 0
		case 'd':
			tv = atol(optarg);  /* set seed */
			break;
#endif
		case 'w':	/* width */
			ccnt = atoi(optarg);
			break;
		case 'l':	/* length */
			lcnt = atoi(optarg);
			break;
		case 's': /* score */
			snscore(rawscores, 0);
			exit(0);
			break;
		case '?':
		case 'h':
		default:
			fputs("usage: snake [-w width] [-l length] [-s]\n",
			    stderr);
			exit(1);
		}

	penalty = loot = 0;
	getcap();

	i = MIN(lcnt, ccnt);
	if (i < 4) {
		cook();
		errx(1, "screen too small for a fair game.");
	}
	/*
	 * chunk is the amount of money the user gets for each $.
	 * The formula below tries to be fair for various screen sizes.
	 * We only pay attention to the smaller of the 2 edges, since
	 * that seems to be the bottleneck.
	 * This formula is a hyperbola which includes the following points:
	 *	(24, $25)	(original scoring algorithm)
	 *	(12, $40)	(experimentally derived by the "feel")
	 *	(48, $15)	(a guess)
	 * This will give a 4x4 screen $99/shot.  We don't allow anything
	 * smaller than 4x4 because there is a 3x3 game where you can win
	 * an infinite amount of money.
	 */
	if (i < 12)
		i = 12;	/* otherwise it isn't fair */
	/*
	 * Compensate for border.  This really changes the game since
	 * the screen is two squares smaller but we want the default
	 * to be $25, and the high scores on small screens were a bit
	 * much anyway.
	 */
	i += 2;
	chunk = (675.0 / (i + 6)) + 2.5;	/* min screen edge */

	signal(SIGINT, stop);
	putpad(TI);	/*	String to begin programs that use cm */
	putpad(KS);	/*	Put terminal in keypad transmit mode */

	snrand(&finish);
	snrand(&you);
	snrand(&money);
	snrand(&snake[0]);

	if (ospeed < 9600 || ((!CM) && (!TA)))
		fast = 0;
	for (i = 1; i < 6; i++)
		chase(&snake[i], &snake[i - 1]);
	setup();
	mainloop();
	/* NOT REACHED */
	return(0);
}

/* Main command loop */
void
mainloop()
{
	int	j, k;
	int	c, match, lastc = 0;
	struct point tmp;

	for (;;) {
		tmp.col = you.col + 1;
		tmp.line = you.line + 1; /* Highlight you, not left & above */
		move(&tmp);
		fflush(stdout);
		if (((c = getchar() & 0177) <= '9') && (c >= '0')) {
			ungetc(c, stdin);
			j = scanf("%d", &repeat);
			c = getchar() & 0177;
		} else {
			if (c != '.')
				repeat = 1;
		}
		if (c == '.')
			c = lastc;
		if ((Klength > 0) &&
		    (c == *KL || c == *KR || c == *KU || c == *KD)) {
			savec = c;
			match = 0;
			kl = KL;
			kr = KR;
			ku = KU;
			kd = KD;
			for (j = Klength; j > 0; j--) {
				if (match != 1) {
					match = 0;
					if (*kl++ == c) {
						ch = 'h';
						match++;
					}
					if (*kr++ == c) {
						ch = 'l';
						match++;
					}
					if (*ku++ == c) {
						ch = 'k';
						match++;
					}
					if (*kd++ == c) {
						ch = 'j';
						match++;
					}
					if (match == 0) {
						ungetc(c, stdin);
						ch = savec;
		/* Oops! 
		 * This works if we figure it out on second character.
		 */
						break;
					}
				}
				savec = c;
				if (j != 1)
					c = getchar() & 0177;
			}
			c = ch;
		}
		if (!fast)
			flushi();
		lastc = c;
		switch (c) {
		case CTRL('z'):
			suspend();
			continue;
		case EOT:
		case 'x':
		case 0177:	/* del or end of file */
			ll();
			cook();
			length(moves);
#ifdef LOGGING
			logit("quit");
#endif
			exit(0);
		case CTRL('l'):
			setup();
			winnings(cashvalue);
			continue;
		case 'p':
		case 'd':
			snap();
			continue;
		case 'w':
			spacewarp(0);
			continue;
		case 'A':
			repeat = you.col;
			c = 'h';
			break;
		case 'H':
		case 'S':
			repeat = you.col - money.col;
			c = 'h';
			break;
		case 'T':
			repeat = you.line;
			c = 'k';
			break;
		case 'K':
		case 'E':
			repeat = you.line - money.line;
			c = 'k';
			break;
		case 'P':
			repeat = ccnt - 1 - you.col;
			c = 'l';
			break;
		case 'L':
		case 'F':
			repeat = money.col - you.col;
			c = 'l';
			break;
		case 'B':
			repeat = lcnt - 1 - you.line;
			c = 'j';
			break;
		case 'J':
		case 'C':
			repeat = money.line - you.line;
			c = 'j';
			break;
		}
		for(k = 1; k <= repeat; k++) {
			moves++;
			switch (c) {
			case 's':
			case 'h':
			case '\b':
				if (you.col >0) {
					if ((fast) || (k == 1))
						pchar(&you,' ');
					you.col--;
					if ((fast) || (k == repeat) || (you.col == 0))
						pchar(&you,ME);
				}
				break;
			case 'f':
			case 'l':
			case ' ':
				if (you.col < ccnt-1) {
					if ((fast) || (k == 1))
						pchar(&you, ' ');
					you.col++;
					if ((fast) || (k == repeat) || (you.col == ccnt-1))
						pchar(&you, ME);
				}
				break;
			case CTRL('p'):
			case 'e':
			case 'k':
			case 'i':
				if (you.line > 0) {
					if ((fast) || (k == 1))
						pchar(&you,' ');
					you.line--;
					if ((fast) || (k == repeat) || (you.line == 0))
						pchar(&you,ME);
				}
				break;
			case CTRL('n'):
			case 'c':
			case 'j':
			case LF:
			case 'm':
				if (you.line+1 < lcnt) {
					if ((fast) || (k == 1))
						pchar(&you,' ');
					you.line++;
					if ((fast) || (k == repeat) || (you.line == lcnt-1))
						pchar(&you,ME);
				}
				break;
			}

			if (same(&you, &money)) {
				loot += 25;
				if (k < repeat)
					pchar(&you, ' ');
				do {
					snrand(&money);
				} while ((money.col == finish.col && money.line == finish.line) ||
					 (money.col < 5 && money.line == 0) ||
					 (money.col == you.col && money.line == you.line));
				pchar(&money, TREASURE);
				winnings(cashvalue);
/*				continue;		 Previously, snake missed a turn! */
			}
			if (same(&you,&finish)) {
				win(&finish);
				ll();
				cook();
				printf("You have won with $%d.\n", cashvalue);
#ifdef LOGGING
				logit("won");
#endif
				length(moves);
				post(cashvalue, 1);
				close(rawscores);
				exit(0);
			}
			if (pushsnake())
				break;
		}
		fflush(stdout);
	}
}

/* set up the board */
void
setup()
{
	int	i;

	clear();
	pchar(&you, ME);
	pchar(&finish, GOAL);
	pchar(&money, TREASURE);
	for (i = 1; i < 6; i++) {
		pchar(&snake[i], SNAKETAIL);
	}
	pchar(&snake[0], SNAKEHEAD);
	drawbox();
	fflush(stdout);
}

void
drawbox()
{
	int i;
	struct point p;

	p.line = -1;
	for (i = 0; i < ccnt; i++) {
		p.col = i;
		pchar(&p, '-');
	}
	p.col = ccnt;
	for (i = -1; i <= lcnt; i++) {
		p.line = i;
		pchar(&p, '|');
	}
	p.col = -1;
	for (i = -1; i <= lcnt; i++) {
		p.line = i;
		pchar(&p, '|');
	}
	p.line = lcnt;
	for (i = 0; i < ccnt; i++) {
		p.col = i;
		pchar(&p, '-');
	}
}

void
snrand(sp)
	struct point *sp;
{
	struct point p;
	int i;

	for (;;) {
		p.col = random() % ccnt;
		p.line = random() % lcnt;

		/* make sure it's not on top of something else */
		if (p.line == 0 && p.col < 5)
			continue;
		if (same(&p, &you))
			continue;
		if (same(&p, &money))
			continue;
		if (same(&p, &finish))
			continue;
		for (i = 0; i < 6; i++)
			if (same(&p, &snake[i]))
				break;
		if (i < 6)
			continue;
		break;
	}
	*sp = p;
}

int
post(iscore, flag)
	int	iscore, flag;
{
	short	score = iscore;
	short	oldbest=0;
	uid_t	uid;

	/* I want to printf() the scores for terms that clear on cook(),
	 * but this routine also gets called with flag == 0 to see if
	 * the snake should wink.  If (flag) then we're at game end and
	 * can printf.
	 */
	/*
	 * Neg uid cannot have scores recorded.
	 */
	if ((uid = getuid()) < 0) {
		if (flag)
			printf("\nNo saved scores for uid %d.\n", uid);
		return(1);
	}
	if (rawscores == -1) {
		if (flag)
			printf("Can't open score file %s: %s.\n",
			    _PATH_RAWSCORES, strerror(errno));
		return(1);
	}
	/* Figure out what happened in the past */
	lseek(rawscores, uid * sizeof(short), SEEK_SET);
	read(rawscores, &oldbest, sizeof(short));
	if (!flag)
		return (score > oldbest ? 1 : 0);

	/* Update this jokers best */
	if (score > oldbest) {
		lseek(rawscores, uid * sizeof(short), SEEK_SET);
		write(rawscores, &score, sizeof(short));
		printf("\nYou bettered your previous best of $%d\n", oldbest);
	} else
		printf("\nYour best to date is $%d\n", oldbest);

	fsync(rawscores);
	/* See if we have a new champ */
	snscore(rawscores, TOPN);
	return(1);
}

/*
 * Flush typeahead to keep from buffering a bunch of chars and then
 * overshooting.  This loses horribly at 9600 baud, but works nicely
 * if the terminal gets behind.
 */
void
flushi()
{
	tcflush(0, TCIFLUSH);
}

int	mx [8] = { 0, 1, 1, 1, 0,-1,-1,-1};
int	my [8] = {-1,-1, 0, 1, 1, 1, 0,-1};
float	absv[8] = {1, 1.4, 1, 1.4, 1, 1.4, 1, 1.4};
int	oldw = 0;

void
chase (np, sp)
	struct point *sp, *np;
{
	/* this algorithm has bugs; otherwise the snake would get too good */
	struct point d;
	int	w, i, wt[8];
	double	v1, v2, vp, max;

	point(&d, you.col-sp->col, you.line-sp->line);
	v1 = sqrt((double)(d.col * d.col + d.line * d.line) );
	w  = 0; 
	max = 0;
	for(i = 0; i < 8; i++)
	{
		vp = d.col * mx[i] + d.line * my[i];
		v2 = absv[i];
		if (v1 > 0)
			vp = ((double)vp) / (v1 * v2);
		else
			vp = 1.0;
		if (vp > max) {
			max = vp;
			w = i;
		}
	}
	for (i = 0; i < 8; i++) {
		point(&d, sp->col + mx[i], sp->line + my[i]);
		wt[i] = 0;
		if (d.col < 0 || d.col >= ccnt || d.line < 0 || d.line >= lcnt)
			continue;
		/*
		 * Change to allow snake to eat you if you're on the money;
		 * otherwise, you can just crouch there until the snake goes
		 * away.  Not positive it's right.
		 *
		 * if (d.line == 0 && d.col < 5) continue;
		 */
		if (same(&d, &money) || same(&d,&finish))
			continue;
		wt[i] = (i == w ? loot/10 : 1);
		if (i == oldw) 
			wt[i] += loot/20;
	}
	for (w = i = 0; i < 8; i++)
		w += wt[i];
	vp = ((rand() >> 6) & 01777) % w;
	for (i = 0; i < 8; i++)
		if (vp < wt[i])
			break;
		else
			vp -= wt[i];
	if (i == 8) {
		pr("failure\n"); 
		i = 0;
		while (wt[i] == 0)
			i++;
	}
	oldw = w = i;
	point(np, sp->col + mx[w], sp->line + my[w]);
}

void
spacewarp(w)
	int w;
{
	struct point p;
	int	j;
	char	*str;

	snrand(&you);
	point(&p, COLUMNS / 2 - 8, LINES / 2 - 1);
	if (p.col < 0)
		p.col = 0;
	if (p.line < 0)
		p.line = 0;
	if (w) {
		str = "BONUS!!!";
		loot = loot - penalty;
		penalty = 0;
	} else {
		str = "SPACE WARP!!!";
		penalty += loot / PENALTY;
	}
	for (j = 0; j < 3; j++) {
		clear();
		fflush(stdout);
		delay(5);
		apr(&p, str);
		fflush(stdout);
		delay(10);
	}
	setup();
	winnings(cashvalue);
}

void
snap()
{
	struct point p;

	/* I don't see the graphical purpose of the next block of code.
	 * It just makes no sense.
	 *
	 * if (you.line < 3)
	 *	pchar(point(&p, you.col, 0), '-');
	 * if (you.line > lcnt - 4)
	 *	pchar(point(&p, you.col, lcnt - 1), '_');
	 * if(you.col < 10)
	 *	pchar(point(&p, 0, you.line), '(');
	 * if(you.col > ccnt-10)
	 *	pchar(point(&p, ccnt-1, you.line), ')');
	 */
	if (!stretch(&money))
		if (!stretch(&finish)) {
			pchar(point(&p, you.col, you.line), '?');
			fflush(stdout);
			delay(10);
			pchar(point(&p, you.col, you.line), ME);
		}
	/* Again, I don't see the point of the following either.
	 *
	 * if (you.line < 3) {
	 * 	point(&p, you.col, 0);
	 * 	chk(&p);
	 * }
	 * if (you.line > lcnt - 4) {
	 * 	point(&p, you.col, lcnt - 1);
	 * 	chk(&p);
	 * }
	 * if (you.col < 10) {
	 * 	point(&p, 0, you.line);
	 * 	chk(&p);
	 * }
	 * if (you.col > ccnt-10) {
	 * 	point(&p, ccnt - 1, you.line);
	 * 	chk(&p);
	 * }
	 */
	fflush(stdout);
}

int
stretch(ps)
	struct point *ps;
{
	struct point p;

	point(&p, you.col, you.line);
	if ((abs(ps->col - you.col) < (ccnt / 12)) && (you.line != ps->line)) {
		if (you.line < ps->line) {
			for (p.line = you.line + 1; p.line <= ps->line; p.line++)
				pchar(&p, 'v');
			fflush(stdout);
			delay(10);
			for (; p.line > you.line; p.line--)
				chk(&p);
		} else {
			for (p.line = you.line - 1; p.line >= ps->line; p.line--)
				pchar(&p, '^');
			fflush(stdout);
			delay(10);
			for (; p.line < you.line; p.line++)
				chk(&p);
		}
		return(1);
	} else if ((abs(ps->line - you.line) < (lcnt / 7)) && (you.col != ps->col)) {
		p.line = you.line;
		if (you.col < ps->col) {
			for (p.col = you.col + 1; p.col <= ps->col; p.col++)
				pchar(&p, '>');
			fflush(stdout);
			delay(10);
			for (; p.col > you.col; p.col--)
				chk(&p);
		} else {
			for (p.col = you.col - 1; p.col >= ps->col; p.col--)
				pchar(&p, '<');
			fflush(stdout);
			delay(10);
			for (; p.col < you.col; p.col++)
				chk(&p);
		}
		return(1);
	}
	return(0);
}

void
surround(ps)
	struct point *ps;
{
	struct point x;
	int	j;

	if (ps->col == 0)
		ps->col++;
	if (ps->line == 0)
		ps->line++;
	if (ps->line == LINES -1)
		ps->line--;
	if (ps->col == COLUMNS -1)
		ps->col--;
	apr(point(&x, ps->col-1, ps->line-1), "/*\\\r* *\r\\*/");
	for (j = 0; j < 20; j++) {
		pchar(ps, '@');
		fflush(stdout);
		delay(1);
		pchar(ps,' ');
		fflush(stdout);
		delay(1);
	}
	if (post(cashvalue, 0)) {
		apr(point(&x, ps->col - 1, ps->line - 1), "   \ro.o\r\\_/");
		fflush(stdout);
		delay(6);
		apr(point(&x,ps->col - 1, ps->line - 1), "   \ro.-\r\\_/");
		fflush(stdout);
		delay(6);
	}
	apr(point(&x, ps->col - 1, ps->line - 1), "   \ro.o\r\\_/");
}

void
win(ps)
	struct point *ps;
{
	struct point x;
	int	j,k;
	int	boxsize;	/* actually diameter of box, not radius */

	boxsize = (fast ? 10 : 4);
	point(&x, ps->col, ps->line);
	for (j = 1; j < boxsize; j++) {
		for (k = 0; k < j; k++) {
			pchar(&x, '#');
			x.line--;
		}
		for (k = 0; k < j; k++) {
			pchar(&x, '#');
			x.col++;
		}
		j++;
		for (k = 0; k < j; k++) {
			pchar(&x, '#');
			x.line++;
		}
		for (k = 0; k < j; k++) {
			pchar(&x, '#');
			x.col--;
		}
		fflush(stdout);
		delay(1);
	}
}

int
pushsnake()
{
	int	i, bonus;
	int	issame = 0;
	struct point tmp;

	/*
	 * My manual says times doesn't return a value.  Furthermore, the
	 * snake should get his turn every time no matter if the user is
	 * on a fast terminal with typematic keys or not.
	 * So I have taken the call to times out.
	 */
	for (i = 4; i >=0; i--)
		if (same(&snake[i], &snake[5]))
			issame++;
	if (!issame)
		pchar(&snake[5], ' ');
	/* Need the following to catch you if you step on the snake's tail */
	tmp.col = snake[5].col; tmp.line = snake[5].line;
	for (i = 4; i >= 0; i--)
		snake[i + 1] = snake[i];
	chase(&snake[0], &snake[1]);
	pchar(&snake[1], SNAKETAIL);
	pchar(&snake[0], SNAKEHEAD);
	for (i = 0; i < 6; i++) {
		if ((same(&snake[i], &you)) || (same(&tmp, &you))) {
			surround(&you);
			i = (cashvalue) % 10;
			bonus = ((random() >> 8) & 0377) % 10;
			ll();
			pr("%d\n", bonus);
			fflush(stdout);
			delay(30);
			if (bonus == i) {
				spacewarp(1);
#ifdef LOGGING
				logit("bonus");
#endif
				flushi();
				return(1);
			}
			cook();
			if ( loot >= penalty ) {
				printf("\nYou and your $%d have been eaten\n", cashvalue);
			} else {
				printf("\nThe snake ate you.  You owe $%d.\n", -cashvalue);
			}
#ifdef LOGGING
			logit("eaten");
#endif
			length(moves);
			snscore(rawscores, TOPN);
			close(rawscores);
			exit(0);
		}
	}
	return(0);
}
	
int
chk(sp)
struct point *sp;
{
	int	j;

	if (same(sp, &money)) {
		pchar(sp, TREASURE);
		return(2);
	}
	if (same(sp, &finish)) {
		pchar(sp, GOAL);
		return(3);
	}
	if (same(sp, &snake[0])) {
		pchar(sp, SNAKEHEAD);
		return(4);
	}
	for (j = 1; j < 6; j++) {
		if (same(sp, &snake[j])) {
			pchar(sp, SNAKETAIL);
			return(4);
		}
	}
	if ((sp->col < 4) && (sp->line == 0)) {
		winnings(cashvalue);
		if ((you.line == 0) && (you.col < 4))
			pchar(&you,ME);
		return(5);
	}
	if (same(sp, &you)) {
		pchar(sp, ME);
		return(1);
	}
	pchar(sp, ' ');
	return(0);
}

void
winnings(won)
	int won;
{
	struct point p;

	p.line = p.col = 1;
	if (won > 0) {
		move(&p);
		pr("$%d  ", won);
	}
}

void
stop(dummy)
	int	dummy;
{
	signal(SIGINT, SIG_IGN);
	ll();
	cook();
	length(moves);
	exit(0);
}

void
suspend()
{
	ll();
	cook();
	kill(getpid(), SIGTSTP);
	raw();
	setup();
	winnings(cashvalue);
}

void
length(num)
	int num;
{
	printf("You made %d moves.\n",num);
}

#ifdef LOGGING
void
logit(msg)
	char *msg;
{
	time_t t;

	if (logfile != NULL) {
		time(&t);
		fprintf(logfile, "%s $%d %dx%d %s %s",
		    getlogin(), cashvalue, lcnt, ccnt, msg, ctime(&t));
		fclose(logfile);
	}
}
#endif
