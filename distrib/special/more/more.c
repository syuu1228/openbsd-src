/*	$OpenBSD: src/distrib/special/more/more.c,v 1.19 2003/06/04 00:26:12 millert Exp $	*/

/*-
 * Copyright (c) 1980 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
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
static const char copyright[] =
"@(#) Copyright (c) 1980 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static const char sccsid[] = "@(#)more.c	5.28 (Berkeley) 3/1/93";
#else
static const char rcsid[] = "$OpenBSD: src/distrib/special/more/more.c,v 1.19 2003/06/04 00:26:12 millert Exp $";
#endif
#endif /* not lint */

/*
 * more.c - General purpose tty output filter and file perusal program
 *
 *	by Eric Shienbrood, UC Berkeley
 *
 *	modified by Geoff Peck, UCB to add underlining, single spacing
 *	modified by John Foderaro, UCB to add -c and MORE environment variable
 */

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <ctype.h>
#include <curses.h>
#include <errno.h>
#include <locale.h>
#include <regex.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <paths.h>

#define Fopen(s,m)	(Currline = 0, file_pos=0, fopen(s,m))
#define Ftell(f)	(file_pos)
#define Fseek(f,off)	(file_pos=off, fseek(f,off,0))
#define Getc(f)		(++file_pos, getc(f))
#define Ungetc(c,f)	(--file_pos, ungetc(c,f))

#define	cleareol()	(tputs(eraseln, 1, putch))
#define	clreos()	(tputs(EodClr, 1, putch))
#define	home()		(tputs(Home, 1, putch))

#define TBUFSIZ		1024
#define LINSIZ		256
#define ctrl(letter)	(letter & 077)
#define RUBOUT		'\177'
#define ESC		'\033'
#define QUIT		'\034'

#define	DUM_PROMPT	"[Press space to continue, 'q' to quit.]"
#define	DUM_ERROR	"[Press 'h' for instructions.]"
#define	QUIT_IT		"[Use q or Q to quit]"

#include "morehelp.h"

struct termios	otty, ntty;
long		file_pos, file_size;
int		fnum, no_intty, no_tty, slow_tty;
int		dum_opt, dlines;
int		nscroll = 11;	/* Number of lines scrolled by 'd' */
int		fold_opt = 1;	/* Fold long lines */
int		stop_opt = 1;	/* Stop after form feeds */
int		ssp_opt = 0;	/* Suppress white space */
int		ul_opt = 1;	/* Underline as best we can */
int		promptlen;
int		Currline;	/* Line we are currently at */
int		startup = 1;
int		firstf = 1;
int		notell = 1;
int		docrterase = 0;
int		docrtkill = 0;
int		bad_so;	/* True if overwriting does not turn off standout */
int		inwait, Pause, errors;
int		within;		/* true if we are within a file,
				   false if we are between files */
int		hard, dumb, noscroll, hardtabs, clreol, eatnl;
int		catch_susp;	/* We should catch the SIGTSTP signal */
char		**fnames;	/* The list of file names */
int		nfiles;		/* Number of files left to process */
char		*shell;		/* The name of the shell to use */
int		shellp;		/* A previous shell command exists */
char		Line[LINSIZ];	/* Line buffer */
int		Lpp = 24;	/* lines per page */
char		*Clear;		/* clear screen */
char		*eraseln;	/* erase line */
char		*Senter, *Sexit;/* enter and exit standout mode */
char		*ULenter, *ULexit; /* enter and exit underline mode */
char		*chUL;		/* underline character */
char		*chBS;		/* backspace character */
char		*Home;		/* go to home */
char		*cursorm;	/* cursor movement */
char		cursorhome[40];	/* contains cursor movement to home */
char		*EodClr;	/* clear rest of screen */
int		Mcol = 80;	/* number of columns */
int		Wrap = 1;	/* set if automargins */
int		soglitch;	/* terminal has standout mode glitch */
int		ulglitch;	/* terminal has underline mode glitch */
int		pstate = 0;	/* current UL state */
int		altscr = 0;	/* terminal supports an alternate screen */

volatile sig_atomic_t signo;	/* signal received */

struct {
	long chrctr, line;
} context, screen_start;

extern char	PC;		/* pad character (termcap) */
extern char	*__progname;	/* program name (crt0) */

int   colon(char *, int, int);
int   command(char *, FILE *);
int   do_shell(char *);
int   expand(char *, size_t, char *);
int   getline(FILE *, int *);
int   magic(FILE *, char *);
int   number(char *);
int   readch(void);
int   search(char *, FILE *, int);
int   ttyin(char *, int, char);
void  argscan(char *);
void  copy_file(FILE *);
void  doclear(void);
void  end_it(void);
void  erasep(int);
void  error(char *);
void  errwrite(char *);
void  errwrite1(char *);
void  execute(char *filename, char *cmd, char *, char *, char *);
void  initterm(void);
void  kill_line(void);
void  onsignal(int);
void  prbuf(char *, int);
void  putch(int);
void  rdline(FILE *);
void  reset_tty(void);
void  screen(FILE *, int);
void  set_tty(void);
void  show(int);
void  skipf(int);
void  skiplns(int, FILE *);
FILE *checkf(char *, int *);
__dead void usage(void);
struct sigaction sa;

int
main(int argc, char **argv)
{
	FILE * volatile f;
	char		*s;
	volatile int	left;
	volatile int	initline;
	volatile int	prnames = 0;
	volatile int	initopt = 0;
	volatile int	srchopt = 0;
	int		clearit = 0;
	int		ch;
	char		initbuf[80];

	setlocale(LC_ALL, "");

	/* all signals just use a stub handler and interrupt syscalls */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = onsignal;

	nfiles = argc;
	fnames = argv;
	initterm();
	nscroll = Lpp/2 - 1;
	if (nscroll <= 0)
		nscroll = 1;
	if ((s = getenv("MORE")) != NULL && *s != '\0')
		argscan(s);
	while (--nfiles > 0) {
		if ((ch = (*++fnames)[0]) == '-')
			argscan(*fnames + 1);
		else if (ch == '+') {
			s = *fnames;
			if (*++s == '/') {
				srchopt++;
				(void)strlcpy(initbuf, ++s, sizeof(initbuf));
			} else {
				initopt++;
				for (initline = 0; *s != '\0'; s++) {
					if (isdigit(*s))
						initline =
						    initline * 10 + *s - '0';
				}
				--initline;
			}
		} else
			break;
	}
	/*
	 * Allow clreol only if Home and eraseln and EodClr strings are
	 * defined, and in that case, make sure we are in noscroll mode.
	 */
	if (clreol) {
		if (Home == NULL || *Home == '\0' || eraseln == NULL ||
		    *eraseln == '\0' || EodClr == NULL || *EodClr == '\0')
			clreol = 0;
		else
			noscroll = 1;
	}
	if (dlines == 0)
		dlines = Lpp - (noscroll ? 1 : 2);
	left = dlines;
	if (nfiles > 1)
		prnames++;
	if (!no_intty && nfiles == 0)
		usage();
	else
		f = stdin;
	if (!no_tty) {
		struct sigaction osa;

		(void)sigaction(SIGQUIT, &sa, NULL);
		(void)sigaction(SIGINT, &sa, NULL);
		(void)sigaction(SIGWINCH, &sa, NULL);
		if (sigaction(SIGTSTP, &osa, NULL) == 0 &&
		    osa.sa_handler == SIG_DFL) {
			(void)sigaction(SIGTSTP, &sa, NULL);
			(void)sigaction(SIGTTIN, &sa, NULL);
			(void)sigaction(SIGTTOU, &sa, NULL);
			catch_susp++;
		}
		set_tty();
	}
	if (no_intty) {
		if (no_tty)
			copy_file(stdin);
		else {
			if ((ch = Getc(f)) == '\f')
				doclear();
			else {
				Ungetc(ch, f);
				if (noscroll && ch != EOF) {
					if (clreol)
						home();
					else
						doclear();
				}
			}
			if (srchopt) {
				if (search(initbuf, stdin, 1) == 0 && noscroll)
					left--;
			} else if (initopt)
				skiplns(initline, stdin);
			screen(stdin, left);
		}
		no_intty = 0;
		dup2(STDERR_FILENO, STDIN_FILENO);	/* stderr is a tty */
		prnames++;
		firstf = 0;
	}

	while (fnum < nfiles) {
		if ((f = checkf(fnames[fnum], &clearit)) != NULL) {
			context.line = context.chrctr = 0;
			Currline = 0;
		restart:
			if (firstf) {
				firstf = 0;
				if (srchopt) {
					if (search(initbuf, f, 1) < 0)
						goto restart;
					if (noscroll)
						left--;
				} else if (initopt)
					skiplns(initline, f);
			} else if (fnum < nfiles && !no_tty)
				left = command(fnames[fnum], f);
			if (left != 0) {
				if ((noscroll || clearit) &&
				    (file_size != LONG_MAX)) {
					if (clreol)
						home();
					else
						doclear();
				}
				if (prnames) {
					if (bad_so)
						erasep(0);
					if (clreol)
						cleareol();
					fputs("::::::::::::::", stdout);
					if (promptlen > 14)
						erasep(14);
					putchar('\n');
					if (clreol)
						cleareol();
					printf("%s\n", fnames[fnum]);
					if (clreol)
						cleareol();
					fputs("::::::::::::::\n", stdout);
					if (left > Lpp - 4)
						left = Lpp - 4;
				}
				if (no_tty)
					copy_file(f);
				else {
					within++;
					screen(f, left);
					within = 0;
				}
			}
			fflush(stdout);
			fclose(f);
			screen_start.line = screen_start.chrctr = 0L;
			context.line = context.chrctr = 0L;
		}
		fnum++;
		firstf = 0;
	}
	reset_tty();
	exit(0);
}

void
argscan(char *s)
{
	int seen_num = 0;

	while (*s != '\0') {
		switch (*s) {
		case '0': case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7': case '8':
		case '9':
			if (!seen_num) {
				dlines = 0;
				seen_num = 1;
			}
			dlines = (dlines * 10) + (*s - '0');
			break;
		case 'd':
			dum_opt = 1;
			break;
		case 'l':
			stop_opt = 0;
			break;
		case 'f':
			fold_opt = 0;
			break;
		case 'p':
			noscroll++;
			break;
		case 'c':
			clreol++;
			break;
		case 's':
			ssp_opt = 1;
			break;
		case 'u':
			ul_opt = 0;
			break;
		case '-':
		case ' ':
		case '\t':
			break;
		default:
			fprintf(stderr, "%s: unknown option \"-%c\"\n",
			    __progname, *s);
			usage();
		}
		s++;
	}
}

/*
 * Check whether the file named by fs is an ASCII file which the user may
 * access.  If it is, return the opened file. Otherwise return NULL.
 */
FILE *
checkf(char *fs, int *clearfirst)
{
	struct stat stbuf;
	FILE *f;
	int c;

	if (stat(fs, &stbuf) == -1) {
		(void)fflush(stdout);
		if (clreol)
			cleareol();
		perror(fs);
		return (NULL);
	}
	if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
		printf("\n*** %s: directory ***\n\n", fs);
		return (NULL);
	}
	if ((f = Fopen(fs, "r")) == NULL) {
		(void)fflush(stdout);
		perror(fs);
		return (NULL);
	}
	if (magic(f, fs))
		return (NULL);
	c = Getc(f);
	*clearfirst = (c == '\f');
	Ungetc(c, f);
	if ((file_size = stbuf.st_size) == 0)
		file_size = LONG_MAX;
	return (f);
}

/*
 * magic --
 *	Check for file magic numbers.  This code would best be shared with
 *	the file(1) program or, perhaps, more should not try and be so smart?
 */
int
magic(FILE *f, char *fs)
{
	char twobytes[2];

	/* don't try to look ahead if the input is unseekable */
	if (fseek(f, 0L, SEEK_SET))
		return (0);

	if (fread(twobytes, 2, 1, f) == 1) {
		switch(twobytes[0] + (twobytes[1]<<8)) {
		case OMAGIC:
		case NMAGIC:
		case ZMAGIC:
		case 0405:
		case 0411:
		case 0x457f:
		case 0177545:
			printf("\n******** %s: Not a text file ********\n\n",
			    fs);
			(void)fclose(f);
			return (1);
		}
	}
	(void)fseek(f, 0L, L_SET);		/* rewind() not necessary */
	return (0);
}

/*
 * A real function (not a macro), for the tputs() routine in termlib
 */
void
putch(int ch)
{
	putchar(ch);
}

#define	STOP	(-10)

/*
 * Print out the contents of the file f, one screenful at a time.
 */
void
screen(FILE *f, int num_lines)
{
	int c;
	int nchars;
	int length;			/* length of current line */
	static int prev_len = 1;	/* length of previous line */

	for (;;) {
		while (num_lines > 0 && !Pause) {
			if ((nchars = getline(f, &length)) == EOF) {
				if (clreol)
					clreos();
				return;
			}
			if (ssp_opt && length == 0 && prev_len == 0)
				continue;
			prev_len = length;
			if (bad_so ||
			    (Senter && *Senter == ' ' && promptlen > 0))
				erasep(0);
			/*
			 * Must clear before drawing line since tabs on some
			 * terminals do not erase what they tab over.
			 */
			if (clreol)
				cleareol();
			prbuf(Line, length);
			if (nchars < promptlen) {
				/* erasep() sets promptlen to 0 */
				erasep(nchars);
			} else
				promptlen = 0;
#if 0
			/* XXX - is this needed? */
			if (clreol) {
				/* must clear again in case we wrapped * */
				cleareol();
			}
#endif
			if (nchars < Mcol || !fold_opt) {
				/* will turn off UL if necessary */
				prbuf("\n", 1);
			}
			if (nchars == STOP)
				break;
			num_lines--;
		}
		if (pstate) {
			tputs(ULexit, 1, putch);
			pstate = 0;
		}
		fflush(stdout);
		if ((c = Getc(f)) == EOF) {
			if (clreol)
				clreos();
			return;
		}

		if (Pause && clreol)
			clreos();
		Ungetc(c, f);
		Pause = 0;
		startup = 0;
		if ((num_lines = command(NULL, f)) == 0)
			return;
		if (hard && promptlen > 0)
			erasep(0);
		if (noscroll && num_lines >= dlines) {
			if (clreol)
				home();
			else
				doclear();
		}
		screen_start.line = Currline;
		screen_start.chrctr = Ftell(f);
	}
}

/*
 * Clean up terminal state and exit. Also come here if interrupt signal received
 */
void
end_it(void)
{
	reset_tty();
	if (clreol) {
		putchar('\r');
		clreos();
		fflush(stdout);
	} else if (promptlen > 0) {
		kill_line();
		fflush(stdout);
	} else
		write(STDERR_FILENO, "\n", 1);
	_exit(0);
}

void
copy_file(FILE *f)
{
	int c;

	while ((c = getc(f)) != EOF)
		putchar(c);
}

static char bell = ctrl('G');

/*
 * See whether the last component of the path name "path" is equal to the
 * string "string".
 */
int
tailequ(char *path, char *string)
{
	char *tail;

	tail = path + strlen(path);
	while (tail >= path)
		if (*(--tail) == '/')
			break;
	++tail;
	while (*tail++ == *string++)
		if (*tail == '\0')
			return (1);
	return (0);
}

void
prompt(char *filename)
{
	if (clreol)
		cleareol();
	else if (promptlen > 0)
		kill_line();
	if (!hard) {
		promptlen = 8;
		if (Senter && Sexit) {
			tputs(Senter, 1, putch);
			promptlen += (2 * soglitch);
		}
		if (clreol)
			cleareol();
		fputs("--More--", stdout);
		if (filename != NULL)
			promptlen += printf("(Next file: %s)", filename);
		else if (!no_intty)
			promptlen += printf("(%d%%)",
			    (int)((file_pos * 100) / file_size));
		if (dum_opt) {
			fputs(DUM_PROMPT, stdout);
			promptlen += sizeof(DUM_PROMPT) - 1;
		}
		if (Senter && Sexit)
			tputs(Sexit, 1, putch);
		if (clreol)
			clreos();
		fflush(stdout);
	} else
		write(STDERR_FILENO, &bell, 1);
	inwait++;
}

/*
 * Get a logical line.
 */
int
getline(FILE *f, int *length)
{
	int		c;
	char		*p;
	int		column;
	static int	colflg;

	p = Line;
	column = 0;
	c = Getc(f);
	if (colflg && c == '\n') {
		Currline++;
		c = Getc(f);
	}
	while (p < &Line[LINSIZ - 1]) {
		if (c == EOF) {
			if (p > Line) {
				*p = '\0';
				*length = p - Line;
				return (column);
			}
			*length = p - Line;
			return (EOF);
		}
		if (c == '\n') {
			Currline++;
			break;
		}
		*p++ = c;
		if (c == '\t') {
			if (!hardtabs || (column < promptlen && !hard)) {
				if (hardtabs && eraseln && !dumb) {
					column = 1 + (column | 7);
					tputs(eraseln, 1, putch);
					promptlen = 0;
				} else {
					for (--p; p < &Line[LINSIZ - 1];) {
						*p++ = ' ';
						if ((++column & 7) == 0)
							break;
					}
					if (column >= promptlen)
						promptlen = 0;
				}
			} else
				column = 1 + (column | 7);
		} else if (c == '\b' && column > 0)
			column--;
		else if (c == '\r')
			column = 0;
		else if (c == '\f' && stop_opt) {
			p[-1] = '^';
			*p++ = 'L';
			column += 2;
			Pause++;
		} else if (c == EOF) {
			*length = p - Line;
			return (column);
		} else if (c >= ' ' && c != RUBOUT)
			column++;
		if (column >= Mcol && fold_opt)
			break;
		c = Getc(f);
	}
	if (column >= Mcol && Mcol > 0 && !Wrap)
		*p++ = '\n';
	colflg = (column == Mcol && fold_opt);
	if (colflg && eatnl && Wrap)
		*p++ = '\n';	/* simulate normal wrap */
	*length = p - Line;
	*p = 0;
	return (column);
}

/*
 * Erase the rest of the prompt, assuming we are starting at column col.
 */
void
erasep(int col)
{
	if (promptlen == 0)
		return;
	if (hard)
		putchar('\n');
	else {
		if (col == 0)
			putchar('\r');
		if (!dumb && eraseln)
			tputs(eraseln, 1, putch);
		else {
			for (col = promptlen - col; col > 0; col--)
				putchar(' ');
		}
	}
	promptlen = 0;
}

/*
 * Erase the current line entirely
 */
void
kill_line(void)
{
	erasep(0);
	if (!eraseln || dumb)
		putchar('\r');
}

/*
 * Print a buffer of n characters.
 */
void
prbuf(char *s, int n)
{
	char c;			/* next output character */
	int state;		/* next output char's UL state */
#define wouldul(s,n)	((n) >= 2 && (((s)[0] == '_' && (s)[1] == '\b') || ((s)[1] == '\b' && (s)[2] == '_')))

	while (--n >= 0) {
		if (!ul_opt)
			putchar(*s++);
		else {
			if (*s == ' ' && pstate == 0 && ulglitch &&
			    wouldul(s + 1, n - 1)) {
				s++;
				continue;
			}
			if ((state = wouldul(s, n))) {
				c = (*s == '_')? s[2] : *s ;
				n -= 2;
				s += 3;
			} else
				c = *s++;
			if (state != pstate) {
				if (c == ' ' && state == 0 && ulglitch &&
				    wouldul(s, n - 1))
					state = 1;
				else
					tputs(state ? ULenter : ULexit, 1, putch);
			}
			if (c != ' ' || pstate == 0 || state != 0 ||
			    ulglitch == 0)
				putchar(c);
			if (state && *chUL) {
				fputs(chBS, stdout);
				tputs(chUL, 1, putch);
			}
			pstate = state;
		}
	}
}

/*
 * Clear the screen
 */
void
doclear(void)
{
	if (Clear && !hard) {
		tputs(Clear, 1, putch);

		/*
		 * Put out carriage return so that system doesn't
		 * get confused by escape sequences when expanding tabs.
		 */
		putchar('\r');
		promptlen = 0;
	}
}

static int lastcmd, lastarg, lastp;
static int lastcolon;
char shell_line[BUFSIZ];

/*
 * Read a command and do it. A command consists of an optional integer
 * argument followed by the command character.  Return the number of lines
 * to display in the next screenful.  If there is nothing more to display
 * in the current file, zero is returned.
 */
int
command(char *filename, FILE *f)
{
	int nlines;
	int retval;
	char c;
	char colonch;
	int done;
	char comchar, cmdbuf[80], *p;

#define ret(val) retval=val;done++;break

	retval = done = 0;
	if (!errors)
		prompt(filename);
	else
		errors = 0;
	for (;;) {
		nlines = number(&comchar);
		lastp = colonch = 0;
		if (comchar == '.') {	/* Repeat last command */
			lastp++;
			comchar = lastcmd;
			nlines = lastarg;
			if (lastcmd == ':')
				colonch = lastcolon;
		}
		lastcmd = comchar;
		lastarg = nlines;
		if (comchar == otty.c_cc[VERASE]) {
			kill_line();
			prompt(filename);
			continue;
		}
		switch (comchar) {
		case ':':
			retval = colon(filename, colonch, nlines);
			if (retval >= 0)
				done++;
			break;
		case 'b':
		case ctrl('B'):
		    {
			int initline;

			if (no_intty) {
				write(STDERR_FILENO, &bell, 1);
				return (-1);
			}

			if (nlines == 0)
				nlines++;

			putchar('\r');
			erasep(0);
			putchar('\n');
			if (clreol)
				cleareol();
			printf("...back %d page", nlines);
			if (nlines > 1)
				fputs("s\n", stdout);
			else
				putchar('\n');

			if (clreol)
				cleareol();
			putchar('\n');

			initline = Currline - dlines * (nlines + 1);
			if (!noscroll)
				--initline;
			if (initline < 0)
				initline = 0;
			Fseek(f, 0L);
			Currline = 0; /* skiplns() will make Currline correct */
			skiplns(initline, f);
			if (!noscroll) {
				ret(dlines + 1);
			} else {
				ret(dlines);
			}
		    }
		case ' ':
		case 'z':
			if (nlines == 0)
				nlines = dlines;
			else if (comchar == 'z')
				dlines = nlines;
			ret(nlines);
		case 'd':
		case ctrl('D'):
			if (nlines != 0)
				nscroll = nlines;
			ret(nscroll);
		case 'q':
		case 'Q':
			end_it();
		case 's':
		case 'f':
			if (nlines == 0)
				nlines++;
			if (comchar == 'f')
				nlines *= dlines;
			putchar('\r');
			erasep(0);
			putchar('\n');
			if (clreol)
				cleareol();
			printf("...skipping %d line", nlines);
			if (nlines > 1)
				fputs("s\n", stdout);
			else
				putchar('\n');

			if (clreol)
				cleareol();
			putchar('\n');

			while (nlines > 0) {
				while ((c = Getc(f)) != '\n') {
					if (c == EOF) {
						retval = 0;
						done++;
						goto endsw;
					}
				}
				Currline++;
				nlines--;
			}
			ret(dlines);
		case '\n':
			if (nlines != 0)
				dlines = nlines;
			else
				nlines = 1;
			ret(nlines);
		case '\f':
			if (!no_intty) {
				doclear();
				Fseek(f, screen_start.chrctr);
				Currline = screen_start.line;
				ret(dlines);
			} else {
				write(STDERR_FILENO, &bell, 1);
				break;
			}
		case '\'':
			if (!no_intty) {
				kill_line();
				fputs("\n***Back***\n\n", stdout);
				Fseek(f, context.chrctr);
				Currline = context.line;
				ret(dlines);
			} else {
				write(STDERR_FILENO, &bell, 1);
				break;
			}
		case '=':
			kill_line();
			promptlen = printf("%d", Currline);
			fflush(stdout);
			break;
		case 'n':
			lastp++;
		case '/':
			if (nlines == 0)
				nlines++;
			kill_line();
			putchar('/');
			promptlen = 1;
			fflush(stdout);
			if (lastp) {
				/* Use previous r.e. */
				write(STDERR_FILENO, "\r", 1);
				if (search(NULL, f, nlines) < 0)
					break;
			} else {
				if (ttyin(cmdbuf, sizeof(cmdbuf) - 2, '/') < 0) {
					kill_line();
					prompt(filename);
					continue;
				}
				write(STDERR_FILENO, "\r", 1);
				if (search(cmdbuf, f, nlines) < 0)
					break;
			}
			ret(dlines-1);
		case '!':
			if (do_shell(filename) < 0) {
				kill_line();
				prompt(filename);
				continue;
			}
			break;
		case '?':
		case 'h':
			if (noscroll)
				doclear();
			fputs(more_help, stdout);
			prompt(filename);
			break;
		case 'v':	/* This case should go right before default */
			if (!no_intty) {
				char *editor;

				editor = getenv("VISUAL");
				if (editor == NULL || *editor == '\0')
					editor = getenv("EDITOR");
				if (editor == NULL || *editor == '\0')
					editor = _PATH_VI;
				if ((p = strrchr(editor, '/')) != NULL)
					p++;
				else
					p = editor;
				kill_line();
				snprintf(cmdbuf, sizeof(cmdbuf), "+%d",
				    Currline);
				if (!altscr)
					printf("%s %s %s", p, cmdbuf,
					    fnames[fnum]);
				execute(filename, editor, p, cmdbuf,
				    fnames[fnum]);
				break;
			}
		default:
			if (dum_opt) {
				kill_line();
				if (Senter && Sexit) {
					tputs(Senter, 1, putch);
					fputs(DUM_ERROR, stdout);
					promptlen = sizeof(DUM_ERROR) - 1 +
					    (2 * soglitch);
					tputs(Sexit, 1, putch);
				} else {
					fputs(DUM_ERROR, stdout);
					promptlen = sizeof(DUM_ERROR) - 1;
				}
				fflush(stdout);
			} else
				write(STDERR_FILENO, &bell, 1);
			break;
		}
		if (done)
			break;
	}
	putchar('\r');
endsw:
	inwait = 0;
	notell++;
	return (retval);
}

/*
 * Execute a colon-prefixed command.
 * Returns <0 if not a command that should cause
 * more of the file to be printed.
 */
int
colon(char *filename, int cmd, int nlines)
{
	int ch;

	if (cmd == 0)
		ch = readch();
	else
		ch = cmd;
	lastcolon = ch;
	switch (ch) {
	case 'f':
		kill_line();
		if (!no_intty)
			promptlen =
			    printf("\"%s\" line %d", fnames[fnum], Currline);
		else
			promptlen = printf("[Not a file] line %d", Currline);
		fflush(stdout);
		return (-1);
	case 'n':
		if (nlines == 0) {
			if (fnum >= nfiles - 1)
				end_it();
			nlines++;
		}
		putchar('\r');
		erasep(0);
		skipf(nlines);
		return (0);
	case 'p':
		if (no_intty) {
			write(STDERR_FILENO, &bell, 1);
			return (-1);
		}
		putchar('\r');
		erasep(0);
		if (nlines == 0)
			nlines++;
		skipf (-nlines);
		return (0);
	case '!':
		if (do_shell(filename) < 0) {
			kill_line();
			prompt(filename);
		}
		return (-1);
	case 'q':
	case 'Q':
		end_it();
	default:
		write(STDERR_FILENO, &bell, 1);
		return (-1);
	}
}

/*
 * Read a decimal number from the terminal. Set cmd to the non-digit which
 * terminates the number.
 */
int
number(char *cmd)
{
	int ch, i;

	ch = otty.c_cc[VKILL];
	i = 0;
	for (;;) {
		ch = readch();
		if (isdigit(ch))
			i = i*10 + ch - '0';
		else if (ch == otty.c_cc[VKILL])
			i = 0;
		else {
			*cmd = ch;
			break;
		}
	}
	return (i);
}

int
do_shell(char *filename)
{
	char cmdbuf[200];

	kill_line();
	putchar('!');
	fflush(stdout);
	promptlen = 1;
	if (lastp)
		fputs(shell_line, stdout);
	else {
		if (ttyin(cmdbuf, sizeof(cmdbuf) - 2, '!') < 0)
			return (-1);
		if (expand(shell_line, sizeof(shell_line), cmdbuf)) {
			kill_line();
			promptlen = printf("!%s", shell_line);
		}
	}
	fflush(stdout);
	write(STDERR_FILENO, "\n", 1);
	promptlen = 0;
	shellp = 1;
	execute(filename, shell, shell, "-c", shell_line);
}

/*
 * Search for nth ocurrence of regular expression contained in buf in the file
 */
int
search(char *buf, FILE *file, int n)
{
	long startline = Ftell(file);
	long line1 = startline;
	long line2 = startline;
	long line3 = startline;
	int lncount;
	int saveln, rv;
	char ebuf[BUFSIZ];
	static regex_t reg;
	static int initialized;

	context.line = saveln = Currline;
	context.chrctr = startline;
	lncount = 0;
	if (buf != NULL && *buf != '\0') {
		if ((rv = regcomp(&reg, buf, REG_NOSUB)) != 0) {
			initialized = 0;
			regerror(rv, &reg, ebuf, sizeof(ebuf));
			regfree(&reg);
			error(ebuf);
			return (-1);
		}
		initialized = 1;
	} else if (!initialized) {
		error("No previous regular expression");
		return (-1);
	}
	while (!feof(file)) {
		line3 = line2;
		line2 = line1;
		line1 = Ftell(file);
		rdline (file);
		lncount++;
		if ((rv = regexec(&reg, Line, 0, NULL, 0)) == 0) {
			if (--n == 0) {
				if (lncount > 3 || (lncount > 1 && no_intty)) {
					putchar('\n');
					if (clreol)
						cleareol();
					fputs("...skipping\n", stdout);
				}
				if (!no_intty) {
					Currline -= (lncount >= 3 ? 3 : lncount);
					Fseek(file, line3);
					if (noscroll) {
						if (clreol) {
							home();
							cleareol();
						} else
							doclear();
					}
				} else {
					kill_line();
					if (noscroll) {
					    if (clreol) {
						    home();
						    cleareol();
					    } else
						    doclear();
					}
					fputs(Line, stdout);
					putchar('\n');
				}
				break;
			}
		} else if (rv != REG_NOMATCH) {
			regerror(rv, &reg, ebuf, sizeof(ebuf));
			error(ebuf);
			return (-1);
		}
	}
	if (feof(file)) {
		if (!no_intty) {
			Currline = saveln;
			Fseek (file, startline);
		} else {
			fputs("\nPattern not found\n", stdout);
			end_it();
		}
		error("Pattern not found");
		return (-1);
	}
	return (0);
}

void
execute(char *filename, char *cmd, char *av0, char *av1, char *av2)
{
	int id;
	int n;
	char *argp[4];

	argp[0] = av0;
	argp[1] = av1;
	argp[2] = av2;
	argp[3] = NULL;

	fflush(stdout);
	reset_tty();
	for (n = 10; (id = fork()) < 0 && n > 0; n--)
		sleep(5);
	if (id == 0) {
		execvp(cmd, argp);
		write(STDERR_FILENO, "exec failed\n", 12);
		exit(1);
	}
	if (id > 0) {
		sa.sa_flags = SA_RESTART;
		sa.sa_handler = SIG_IGN;
		(void)sigaction(SIGINT, &sa, NULL);
		(void)sigaction(SIGQUIT, &sa, NULL);
		if (catch_susp) {
			sa.sa_handler = SIG_DFL;
			(void)sigaction(SIGTSTP, &sa, NULL);
			(void)sigaction(SIGTTIN, &sa, NULL);
			(void)sigaction(SIGTTOU, &sa, NULL);
		}
		while (wait(NULL) > 0)
			continue;
		sa.sa_flags = 0;
		sa.sa_handler = onsignal;
		(void)sigaction(SIGINT, &sa, NULL);
		(void)sigaction(SIGQUIT, &sa, NULL);
		if (catch_susp) {
			(void)sigaction(SIGTSTP, &sa, NULL);
			(void)sigaction(SIGTTIN, &sa, NULL);
			(void)sigaction(SIGTTOU, &sa, NULL);
		}
	} else
		write(STDERR_FILENO, "can't fork\n", 11);
	set_tty();
	if (!altscr)
		fputs("------------------------\n", stdout);
	prompt(filename);
}

/*
 * Skip n lines in the file f
 */
void
skiplns(int n, FILE *f)
{
	char c;

	while (n > 0) {
		while ((c = Getc(f)) != '\n') {
			if (c == EOF)
				return;
		}
		n--;
		Currline++;
	}
}

/*
 * Skip nskip files in the file list (from the command line).
 * Nskip may be negative.
 */
void
skipf(int nskip)
{
	if (nskip == 0)
		return;
	if (nskip > 0) {
		if (fnum + nskip > nfiles - 1)
			nskip = nfiles - fnum - 1;
	}
	else if (within)
		++fnum;
	fnum += nskip;
	if (fnum < 0)
		fnum = 0;
	fputs("\n...Skipping \n", stdout); /* XXX huh? */
	if (clreol)
		cleareol();
	printf("...Skipping %sto file %s\n", nskip > 0 ? "" : "back ",
	    fnames[fnum]);
	if (clreol)
		cleareol();
	putchar('\n');
	--fnum;
}

/*
 * Terminal I/O
 */
void
initterm(void)
{
	char		buf[TBUFSIZ];
	static char	clearbuf[TBUFSIZ];
	char		*clearptr, *padstr;
	char		*term;
	int		tgrp;
	struct winsize	win;

retry:
	if (!(no_tty = tcgetattr(STDOUT_FILENO, &otty))) {
		docrterase = (otty.c_cc[VERASE] != _POSIX_VDISABLE);
		docrtkill =  (otty.c_cc[VKILL] != _POSIX_VDISABLE);
		/*
		 * Wait until we're in the foreground before we save the
		 * the terminal modes.
		 */
		if ((tgrp = tcgetpgrp(STDOUT_FILENO)) < 0) {
			perror("tcgetpgrp");
			exit(1);
		}
		if (tgrp != getpgrp()) {
			kill(0, SIGTTOU);
			goto retry;
		}
		if ((term = getenv("TERM")) == 0 || tgetent(buf, term) <= 0) {
			dumb++; ul_opt = 0;
		} else {
			if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) < 0) {
				Lpp = tgetnum("li");
				Mcol = tgetnum("co");
			} else {
				if ((Lpp = win.ws_row) == 0)
					Lpp = tgetnum("li");
				if ((Mcol = win.ws_col) == 0)
					Mcol = tgetnum("co");
			}
			if (Lpp <= 0 || tgetflag("hc")) {
				hard++;		/* Hard copy terminal */
				Lpp = 24;
			}
			if (tgetflag("xn")) {
				/* Eat newline at last column + 1 */
				eatnl++;
			}
			if (Mcol <= 0)
				Mcol = 80;

			if (tailequ(fnames[0], "page") ||
			    (!hard && tgetflag("ns")))
				noscroll++;
			Wrap = tgetflag("am");
			bad_so = tgetflag ("xs");
			clearptr = clearbuf;
			eraseln = tgetstr("ce",&clearptr);
			Clear = tgetstr("cl", &clearptr);
			Senter = tgetstr("so", &clearptr);
			Sexit = tgetstr("se", &clearptr);
			if ((soglitch = tgetnum("sg")) < 0)
				soglitch = 0;

			/*
			 * Setup for underlining.  Some terminals don't need it,
			 * others have start/stop sequences, still others have
			 * an underline char sequence which is assumed to move
			 * the cursor forward one character.  If underline seq
			 * isn't available, settle for standout sequence.
			 */
			if (tgetflag("ul") || tgetflag("os"))
				ul_opt = 0;
			if ((chUL = tgetstr("uc", &clearptr)) == NULL )
				chUL = "";
			if (((ULenter = tgetstr("us", &clearptr)) == NULL ||
			    (ULexit = tgetstr("ue", &clearptr)) == NULL) &&
			    !*chUL) {
				if ((ULenter = Senter) == NULL ||
				    (ULexit = Sexit) == NULL) {
					ULenter = "";
					ULexit = "";
				} else
					ulglitch = soglitch;
			} else {
				if ((ulglitch = tgetnum("ug")) < 0)
					ulglitch = 0;
			}

			if ((padstr = tgetstr("pc", &clearptr)))
				PC = *padstr;
			Home = tgetstr("ho", &clearptr);
			if (Home == 0 || *Home == '\0') {
				cursorm = tgetstr("cm", &clearptr);
				if (cursorm != NULL) {
					strlcpy(cursorhome,
					    tgoto(cursorm, 0, 0),
					    sizeof(cursorhome));
					Home = cursorhome;
				}
			}
			EodClr = tgetstr("cd", &clearptr);
			if ((chBS = tgetstr("bc", &clearptr)) == NULL)
				chBS = "\b";
			if (tgetstr("te", &clearptr) != NULL &&
			    tgetstr("ti", &clearptr) != NULL)
				altscr = 1;
		}
		if ((shell = getenv("SHELL")) == NULL)
			shell = _PATH_BSHELL;
	}
	no_intty = !isatty(STDIN_FILENO);
	tcgetattr(STDERR_FILENO, &otty);
	slow_tty = cfgetospeed(&otty) < B1200;
	hardtabs = !(otty.c_oflag & OXTABS);
	ntty = otty;
	if (!no_tty) {
		ntty.c_lflag &= ~(ICANON|ECHO);
		ntty.c_cc[VMIN] = 1;	/* read at least 1 char */
		ntty.c_cc[VTIME] = 0;	/* no timeout */
	}
}

int
handle_signal(int sig)
{
	int ch = -1;

	signo = 0;

	switch (sig) {
	case SIGQUIT:
		if (!inwait) {
			putchar('\n');
			if (startup)
				Pause++;
		} else if (!dum_opt && notell) {
			write(STDERR_FILENO, QUIT_IT,
			    sizeof(QUIT_IT) - 1);
			promptlen += sizeof(QUIT_IT) - 1;
			notell = 0;
		}
		break;
	case SIGTSTP:
	case SIGTTIN:
	case SIGTTOU:
		/* XXX - should use saved values instead of SIG_DFL */
		sa.sa_handler = SIG_DFL;
		sa.sa_flags = SA_RESTART;
		(void)sigaction(SIGTSTP, &sa, NULL);
		(void)sigaction(SIGTTIN, &sa, NULL);
		(void)sigaction(SIGTTOU, &sa, NULL);
		reset_tty();
		kill(getpid(), sig);

		sa.sa_handler = onsignal;
		sa.sa_flags = 0;
		(void)sigaction(SIGTSTP, &sa, NULL);
		(void)sigaction(SIGTTIN, &sa, NULL);
		(void)sigaction(SIGTTOU, &sa, NULL);
		set_tty();
		if (!no_intty)
			ch = '\f';	/* force redraw */
		break;
	case SIGINT:
		end_it();
		break;
	case SIGWINCH: {
		struct winsize win;

		if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) != 0)
			break;
		if (win.ws_row != 0) {
			Lpp = win.ws_row;
			nscroll = Lpp/2 - 1;
			if (nscroll <= 0)
				nscroll = 1;
			dlines = Lpp - (noscroll ? 1 : 2);
		}
		if (win.ws_col != 0)
			Mcol = win.ws_col;
		if (!no_intty)
			ch = '\f';	/* force redraw */
		break;
	} default:
		/* NOTREACHED */
		break;
	}
	return (ch);
}

int
readch(void)
{
	int ch;

	errno = 0;
	/* We know stderr is hooked up to /dev/tty so this is safe. */
again:
	if (read(STDERR_FILENO, &ch, 1) <= 0) {
		if (signo != 0) {
			if ((ch = handle_signal(signo)) == -1)
				goto again;
		} else {
			if (errno != EINTR)
				end_it();
			else
				ch = otty.c_cc[VKILL];
		}
	}
	return (ch);
}

static char BS = '\b';
static char BSB[] = "\b \b";
static char CARAT = '^';
#define	ERASEONECHAR	do {					\
	if (docrterase)						\
		write(STDERR_FILENO, BSB, sizeof(BSB) - 1);	\
	else							\
		write(STDERR_FILENO, &BS, 1);			\
} while (0)

int
ttyin(char *buf, int nmax, char pchar)
{
	char	cbuf, ch, *sptr;
	int	maxlen, slash;

	sptr = buf;
	slash = maxlen = 0;
	while (sptr - buf < nmax) {
		if (promptlen > maxlen)
			maxlen = promptlen;
		ch = readch();
		if (ch == '\\')
			slash++;
		else if ((ch == otty.c_cc[VERASE]) && !slash) {
			if (sptr > buf) {
				--promptlen;
				ERASEONECHAR;
				--sptr;
				if ((*sptr < ' ' && *sptr != '\n') ||
				    *sptr == RUBOUT) {
					--promptlen;
					ERASEONECHAR;
				}
				continue;
			} else {
				if (!eraseln)
					promptlen = maxlen;
				return (-1);
			}
		} else if ((ch == otty.c_cc[VKILL]) && !slash) {
			if (hard) {
				show(ch);
				putchar('\n');
				putchar(pchar);
			} else {
				putchar('\r');
				putchar(pchar);
				if (eraseln)
					erasep(1);
				else if (docrtkill) {
					while (promptlen-- > 1)
						write(STDERR_FILENO, BSB,
						    sizeof(BSB) - 1);
				}
				promptlen = 1;
			}
			sptr = buf;
			fflush(stdout);
			continue;
		}
		if (slash && (ch == otty.c_cc[VKILL] ||
		    ch == otty.c_cc[VERASE])) {
			ERASEONECHAR;
			--sptr;
		}
		if (ch != '\\')
			slash = 0;
		*sptr++ = ch;
		if ((ch < ' ' && ch != '\n' && ch != ESC) || ch == RUBOUT) {
			ch += ch == RUBOUT ? -0100 : 0100;
			write(STDERR_FILENO, &CARAT, 1);
			promptlen++;
		}
		cbuf = ch;
		if (ch != '\n' && ch != ESC) {
			write(STDERR_FILENO, &cbuf, 1);
			promptlen++;
		} else
			break;
	}
	*--sptr = '\0';
	if (!eraseln)
		promptlen = maxlen;
	if (sptr - buf >= nmax - 1)
		error("Line too long");

	return (0);
}

int
expand(char *outbuf, size_t olen, char *inbuf)
{
	size_t len;
	char *instr;
	char *outstr;
	char ch;
	char temp[200];
	int changed = 0;

	instr = inbuf;
	outstr = temp;
	while ((ch = *instr++) != '\0') {
		switch (ch) {
		case '%':
			if (!no_intty) {
				len = strlcpy(outstr, fnames[fnum],
				    temp + sizeof(temp) - outstr);
				if (len >= temp + sizeof(temp) - outstr)
					len = temp + sizeof(temp) - outstr - 1;
				outstr += len;
				changed++;
			} else
				*outstr++ = ch;
			break;
		case '!':
			if (!shellp)
				error("No previous command to substitute for");
			len = strlcpy(outstr, shell_line,
			    temp + sizeof(temp) - outstr);
			if (len >= temp + sizeof(temp) - outstr)
				len = temp + sizeof(temp) - outstr - 1;
			outstr += len;
			changed++;
			break;
		case '\\':
			if (*instr == '%' || *instr == '!') {
				*outstr++ = *instr++;
				break;
			}
		default:
			*outstr++ = ch;
			break;
		}
	}
	*outstr++ = '\0';
	strlcpy(outbuf, temp, olen);
	return (changed);
}

void
show(int ch)
{
	char cbuf;

	if ((ch < ' ' && ch != '\n' && ch != ESC) || ch == RUBOUT) {
		ch += ch == RUBOUT ? -0100 : 0100;
		write(STDERR_FILENO, &CARAT, 1);
		promptlen++;
	}
	cbuf = ch;
	write(STDERR_FILENO, &cbuf, 1);
	promptlen++;
}

void
error(char *mess)
{
	if (clreol)
		cleareol();
	else
		kill_line();
	promptlen += strlen (mess);
	if (Senter && Sexit) {
		tputs(Senter, 1, putch);
		fputs(mess, stdout);
		tputs(Sexit, 1, putch);
	} else
		fputs(mess, stdout);
	fflush(stdout);
	errors++;
}

void
set_tty(void)
{
	tcsetattr(STDERR_FILENO, TCSANOW, &ntty);
}

void
reset_tty(void)
{
	if (no_tty)
		return;
	if (pstate) {
		tputs(ULexit, 1, putch);
		fflush(stdout);
		pstate = 0;
	}
	tcsetattr(STDERR_FILENO, TCSANOW, &otty);
}

void
rdline(FILE *f)
{
	char c;
	char *p;

	p = Line;
	while ((c = Getc(f)) != '\n' && c != EOF && p - Line < LINSIZ - 1)
		*p++ = c;
	if (c == '\n')
		Currline++;
	*p = '\0';
}

/*
 * Come here when we get a signal we can handle.
 */
void
onsignal(int sig)
{
	signo = sig;
}

__dead void
usage(void)
{
	fprintf(stderr,
	    "usage: %s [-dfln] [+linenum | +/pattern] name1 name2 ...\n",
	    __progname);
	exit(1);
}
