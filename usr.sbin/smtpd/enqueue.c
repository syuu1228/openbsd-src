/*	$OpenBSD: src/usr.sbin/smtpd/enqueue.c,v 1.30 2009/12/13 22:02:55 jacekm Exp $	*/

/*
 * Copyright (c) 2005 Henning Brauer <henning@bulabula.org>
 * Copyright (c) 2009 Jacek Masiulaniec <jacekm@dobremiasto.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/tree.h>
#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <event.h>
#include <fcntl.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "smtpd.h"
#include "client.h"

extern struct imsgbuf	*ibuf;

void	 usage(void);
void	 sighdlr(int);
int	 main(int, char *[]);
void	 build_from(char *, struct passwd *);
int	 parse_message(FILE *, int, int, struct buf *);
void	 parse_addr(char *, size_t, int);
void	 parse_addr_terminal(int);
char	*qualify_addr(char *);
void	 rcpt_add(char *);
int	 open_connection(void);

enum headerfields {
	HDR_NONE,
	HDR_FROM,
	HDR_TO,
	HDR_CC,
	HDR_BCC,
	HDR_SUBJECT,
	HDR_DATE,
	HDR_MSGID
};

struct {
	char			*word;
	enum headerfields	 type;
} keywords[] = {
	{ "From:",		HDR_FROM },
	{ "To:",		HDR_TO },
	{ "Cc:",		HDR_CC },
	{ "Bcc:",		HDR_BCC },
	{ "Subject:",		HDR_SUBJECT },
	{ "Date:",		HDR_DATE },
	{ "Message-Id:",	HDR_MSGID }
};

#define	SMTP_LINELEN		1000
#define	TIMEOUTMSG		"Timeout\n"

#define WSP(c)			(c == ' ' || c == '\t')

int	  verbose = 0;
char	  host[MAXHOSTNAMELEN];
char	 *user = NULL;
time_t	  timestamp;

struct {
	int	  fd;
	char	 *from;
	char	 *fromname;
	char	**rcpts;
	int	  rcpt_cnt;
	char	 *data;
	size_t	  len;
	int	  saw_date;
	int	  saw_msgid;
	int	  saw_from;
} msg;

struct {
	u_int		quote;
	u_int		comment;
	u_int		esc;
	u_int		brackets;
	size_t		wpos;
	char		buf[SMTP_LINELEN];
} pstate;

void
sighdlr(int sig)
{
	if (sig == SIGALRM) {
		write(STDERR_FILENO, TIMEOUTMSG, sizeof(TIMEOUTMSG));
		_exit (2);
	}
}

int
enqueue(int argc, char *argv[])
{
	int			 i, ch, tflag = 0, noheader;
	char			*fake_from = NULL;
	struct passwd		*pw;
	struct smtp_client	*pcb;
	struct buf		*body;

	bzero(&msg, sizeof(msg));
	time(&timestamp);

	while ((ch = getopt(argc, argv,
	    "A:B:b:E::e:F:f:iJ::L:mo:p:qtvx")) != -1) {
		switch (ch) {
		case 'f':
			fake_from = optarg;
			break;
		case 'F':
			msg.fromname = optarg;
			break;
		case 't':
			tflag = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		/* all remaining: ignored, sendmail compat */
		case 'A':
		case 'B':
		case 'b':
		case 'E':
		case 'e':
		case 'i':
		case 'L':
		case 'm':
		case 'o':
		case 'p':
		case 'x':
			break;
		case 'q':
			/* XXX: implement "process all now" */
			return (0);
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (gethostname(host, sizeof(host)) == -1)
		err(1, "gethostname");
	if ((pw = getpwuid(getuid())) == NULL)
		user = "anonymous";
	if (pw != NULL && (user = strdup(pw->pw_name)) == NULL)
		err(1, "strdup");

	build_from(fake_from, pw);

	while(argc > 0) {
		rcpt_add(argv[0]);
		argv++;
		argc--;
	}

	signal(SIGALRM, sighdlr);
	alarm(300);

	if ((msg.fd = open_connection()) == -1)
		errx(1, "server too busy");

	/* init session */
	pcb = client_init(msg.fd, open("/dev/null", O_RDONLY), "localhost",
	    verbose);

	/* parse message */
	if ((body = buf_dynamic(0, SIZE_T_MAX)) < 0)
		err(1, "buf_dynamic failed");
	noheader = parse_message(stdin, fake_from == NULL, tflag, body);

	/* set envelope from */
	client_sender(pcb, "%s", msg.from);

	/* add recipients */
	if (msg.rcpt_cnt == 0)
		errx(1, "no recipients");
	for (i = 0; i < msg.rcpt_cnt; i++)
		client_rcpt(pcb, "%s", msg.rcpts[i]);

	/* add From */
	if (!msg.saw_from)
		client_printf(pcb, "From: %s%s<%s>\n",
		    msg.fromname ? msg.fromname : "",
		    msg.fromname ? " " : "", 
		    msg.from);

	/* add Date */
	if (!msg.saw_date)
		client_printf(pcb, "Date: %s\n", time_to_text(timestamp));

	/* add Message-Id */
	if (!msg.saw_msgid)
		client_printf(pcb, "Message-Id: <%llu.enqueue@%s>\n",
		    generate_uid(), host);

	/* add separating newline */
	if (noheader)
		client_printf(pcb, "\n");

	client_printf(pcb, "%.*s", buf_size(body), body->buf);
	buf_free(body);

	/* run the protocol engine */
	for (;;) {
		alarm(pcb->timeout.tv_sec);

		switch (client_talk(pcb)) {
		case CLIENT_WANT_READ:
		case CLIENT_WANT_WRITE:
			continue;
		case CLIENT_RCPT_FAIL:
			errx(1, "%s", pcb->reply);
		case CLIENT_DONE:
			break;
		default:
			errx(1, "client_talk: unexpected code");
		}

		if (pcb->status[0] != '2')
			errx(1, "%s", pcb->status);
		break;
	}

	client_close(pcb);

	close(msg.fd);
	exit (0);
}

void
build_from(char *fake_from, struct passwd *pw)
{
	char	*p;

	if (fake_from == NULL)
		msg.from = qualify_addr(user);
	else {
		if (fake_from[0] == '<') {
			if (fake_from[strlen(fake_from) - 1] != '>')
				errx(1, "leading < but no trailing >");
			fake_from[strlen(fake_from) - 1] = 0;
			if ((p = malloc(strlen(fake_from))) == NULL)
				err(1, "malloc");
			strlcpy(p, fake_from + 1, strlen(fake_from));

			msg.from = qualify_addr(p);
			free(p);
		} else
			msg.from = qualify_addr(fake_from);
	}

	if (msg.fromname == NULL && fake_from == NULL && pw != NULL) {
		int	 len, apos;

		len = strcspn(pw->pw_gecos, ",");
		if ((p = memchr(pw->pw_gecos, '&', len))) {
			apos = p - pw->pw_gecos;
			if (asprintf(&msg.fromname, "%.*s%s%.*s",
			    apos, pw->pw_gecos,
			    pw->pw_name,
			    len - apos - 1, p + 1) == -1)
				err(1, NULL);
			msg.fromname[apos] = toupper(msg.fromname[apos]);
		} else {
			if (asprintf(&msg.fromname, "%.*s", len,
			    pw->pw_gecos) == -1)
				err(1, NULL);
		}
	}
}

int
parse_message(FILE *fin, int get_from, int tflag, struct buf *body)
{
	char	*buf;
	size_t	 len;
	u_int	 i, cur = HDR_NONE;
	u_int	 header_seen = 0, header_done = 0;

	bzero(&pstate, sizeof(pstate));
	for (;;) {
		buf = fgetln(fin, &len);
		if (buf == NULL && ferror(fin))
			err(1, "fgetln");
		if (buf == NULL && feof(fin))
			break;

		/* account for \r\n linebreaks */
		if (len >= 2 && buf[len - 2] == '\r' && buf[len - 1] == '\n')
			buf[--len - 1] = '\n';

		if (len == 1 && buf[0] == '\n')		/* end of header */
			header_done = 1;

		if (buf == NULL || len < 1)
			err(1, "fgetln weird");

		if (!WSP(buf[0])) {	/* whitespace -> continuation */
			if (cur == HDR_FROM)
				parse_addr_terminal(1);
			if (cur == HDR_TO || cur == HDR_CC || cur == HDR_BCC)
				parse_addr_terminal(0);
			cur = HDR_NONE;
		}

		for (i = 0; !header_done && cur == HDR_NONE &&
		    i < nitems(keywords); i++)
			if (len > strlen(keywords[i].word) &&
			    !strncasecmp(buf, keywords[i].word,
			    strlen(keywords[i].word)))
				cur = keywords[i].type;

		if (cur != HDR_NONE)
			header_seen = 1;

		if (cur != HDR_BCC) {
			if (buf_add(body, buf, len) < 0)
				err(1, "buf_add failed");
			if (buf[len - 1] != '\n' && buf_add(body, "\n", 1) < 0)
				err(1, "buf_add failed");
		}

		/*
		 * using From: as envelope sender is not sendmail compatible,
		 * but I really want it that way - maybe needs a knob
		 */
		if (cur == HDR_FROM) {
			msg.saw_from++;
			if (get_from)
				parse_addr(buf, len, 1);
		}

		if (tflag && (cur == HDR_TO || cur == HDR_CC || cur == HDR_BCC))
			parse_addr(buf, len, 0);

		if (cur == HDR_DATE)
			msg.saw_date++;
		if (cur == HDR_MSGID)
			msg.saw_msgid++;
	}

	return (!header_seen);
}

void
parse_addr(char *s, size_t len, int is_from)
{
	size_t	 pos = 0;
	int	 terminal = 0;

	/* unless this is a continuation... */
	if (!WSP(s[pos]) && s[pos] != ',' && s[pos] != ';') {
		/* ... skip over everything before the ':' */
		for (; pos < len && s[pos] != ':'; pos++)
			;	/* nothing */
		/* ... and check & reset parser state */
		parse_addr_terminal(is_from);
	}

	/* skip over ':' ',' ';' and whitespace */
	for (; pos < len && !pstate.quote && (WSP(s[pos]) || s[pos] == ':' ||
	    s[pos] == ',' || s[pos] == ';'); pos++)
		;	/* nothing */

	for (; pos < len; pos++) {
		if (!pstate.esc && !pstate.quote && s[pos] == '(')
			pstate.comment++;
		if (!pstate.comment && !pstate.esc && s[pos] == '"')
			pstate.quote = !pstate.quote;

		if (!pstate.comment && !pstate.quote && !pstate.esc) {
			if (s[pos] == ':') {	/* group */
				for(pos++; pos < len && WSP(s[pos]); pos++)
					;	/* nothing */
				pstate.wpos = 0;
			}
			if (s[pos] == '\n' || s[pos] == '\r')
				break;
			if (s[pos] == ',' || s[pos] == ';') {
				terminal = 1;
				break;
			}
			if (s[pos] == '<') {
				pstate.brackets = 1;
				pstate.wpos = 0;
			}
			if (pstate.brackets && s[pos] == '>')
				terminal = 1;
		}

		if (!pstate.comment && !terminal && (!(!(pstate.quote ||
		    pstate.esc) && (s[pos] == '<' || WSP(s[pos]))))) {
			if (pstate.wpos >= sizeof(pstate.buf))
				errx(1, "address exceeds buffer size");
			pstate.buf[pstate.wpos++] = s[pos];
		}

		if (!pstate.quote && pstate.comment && s[pos] == ')')
			pstate.comment--;

		if (!pstate.esc && !pstate.comment && !pstate.quote &&
		    s[pos] == '\\')
			pstate.esc = 1;
		else
			pstate.esc = 0;
	}

	if (terminal)
		parse_addr_terminal(is_from);

	for (; pos < len && (s[pos] == '\r' || s[pos] == '\n'); pos++)
		;	/* nothing */

	if (pos < len)
		parse_addr(s + pos, len - pos, is_from);
}

void
parse_addr_terminal(int is_from)
{
	if (pstate.comment || pstate.quote || pstate.esc)
		errx(1, "syntax error in address");
	if (pstate.wpos) {
		if (pstate.wpos >= sizeof(pstate.buf))
			errx(1, "address exceeds buffer size");
		pstate.buf[pstate.wpos] = '\0';
		if (is_from)
			msg.from = qualify_addr(pstate.buf);
		else
			rcpt_add(pstate.buf);
		pstate.wpos = 0;
	}	
}

char *
qualify_addr(char *in)
{
	char	*out;

	if (strlen(in) > 0 && strchr(in, '@') == NULL) {
		if (asprintf(&out, "%s@%s", in, host) == -1)
			err(1, "qualify asprintf");
	} else
		if ((out = strdup(in)) == NULL)
			err(1, "qualify strdup");

	return (out);
}

void
rcpt_add(char *addr)
{
	void	*nrcpts;

	if ((nrcpts = realloc(msg.rcpts,
	    sizeof(char *) * (msg.rcpt_cnt + 1))) == NULL)
		err(1, "rcpt_add realloc");
	msg.rcpts = nrcpts;
	msg.rcpts[msg.rcpt_cnt++] = qualify_addr(addr);
}

int
open_connection(void)
{
	struct imsg	imsg;
	int		fd;
	int		n;

	imsg_compose(ibuf, IMSG_SMTP_ENQUEUE, 0, 0, -1, NULL, 0);

	while (ibuf->w.queued)
		if (msgbuf_write(&ibuf->w) < 0)
			err(1, "write error");

	while (1) {
		if ((n = imsg_read(ibuf)) == -1)
			errx(1, "imsg_read error");
		if (n == 0)
			errx(1, "pipe closed");

		if ((n = imsg_get(ibuf, &imsg)) == -1)
			errx(1, "imsg_get error");
		if (n == 0)
			continue;

		switch (imsg.hdr.type) {
		case IMSG_CTL_OK:
			break;
		case IMSG_CTL_FAIL:
			errx(1, "server disallowed submission request");
		default:
			errx(1, "unexpected imsg reply type");
		}

		fd = imsg.fd;
		imsg_free(&imsg);

		break;
	}

	return fd;
}

int
enqueue_offline(int argc, char *argv[])
{
	char	 path[MAXPATHLEN];
	FILE	*fp;
	int	 i, fd, ch;

	if (! bsnprintf(path, sizeof(path), "%s%s/%d.XXXXXXXXXX", PATH_SPOOL,
		PATH_OFFLINE, time(NULL)))
		err(1, "snprintf");

	if ((fd = mkstemp(path)) == -1 || (fp = fdopen(fd, "w+")) == NULL) {
		warn("cannot create temporary file %s", path);
		if (fd != -1)
			unlink(path);
		exit(1);
	}

	for (i = 1; i < argc; i++) {
		if (strchr(argv[i], '|') != NULL) {
			warnx("%s contains illegal character", argv[i]);
			unlink(path);
			exit(1);
		}
		fprintf(fp, "%s%s", i == 1 ? "" : "|", argv[i]);
	}

	fprintf(fp, "\n");

	while ((ch = fgetc(stdin)) != EOF)
		if (fputc(ch, fp) == EOF) {
			warn("write error");
			unlink(path);
			exit(1);
		}

	if (ferror(stdin)) {
		warn("read error");
		unlink(path);
		exit(1);
	}

	fclose(fp);

	return (0);
}
