/*	$OpenBSD: src/usr.sbin/bgpd/mrt.c,v 1.15 2004/01/05 22:57:58 claudio Exp $ */

/*
 * Copyright (c) 2003 Claudio Jeker <claudio@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/queue.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bgpd.h"
#include "rde.h"
#include "session.h"	/* needed for MSGSIZE_HEADER et al. */

#include "mrt.h"

/*
 * XXX These functions break the imsg encapsulation.
 * XXX The imsg API is way to basic, we need something like
 * XXX imsg_create(), imsg_add(), imsg_close() ...
 */

static int	mrt_dump_entry(struct mrt_config *, struct prefix *, u_int16_t,
		    struct peer_config *);
static int	mrt_dump_header(struct buf *, u_int16_t, u_int16_t, u_int32_t);
static int	mrt_open(struct mrt *);

#define DUMP_BYTE(x, b)							\
	do {								\
		u_char		t = (b);				\
		if (buf_add((x), &t, sizeof(t)) == -1) {		\
			logit(LOG_ERR, "mrt_dump1: buf_add error");	\
			return (-1);					\
		}							\
	} while (0)

#define DUMP_SHORT(x, s)						\
	do {								\
		u_int16_t	t;					\
		t = htons((s));						\
		if (buf_add((x), &t, sizeof(t)) == -1) {		\
			logit(LOG_ERR, "mrt_dump2: buf_add error");	\
			return (-1);					\
		}							\
	} while (0)

#define DUMP_LONG(x, l)							\
	do {								\
		u_int32_t	t;					\
		t = htonl((l));						\
		if (buf_add((x), &t, sizeof(t)) == -1) {		\
			logit(LOG_ERR, "mrt_dump3: buf_add error");	\
			return (-1);					\
		}							\
	} while (0)

#define DUMP_NLONG(x, l)						\
	do {								\
		u_int32_t	t = (l);				\
		if (buf_add((x), &t, sizeof(t)) == -1) {		\
			logit(LOG_ERR, "mrt_dump4: buf_add error");	\
			return (-1);					\
		}							\
	} while (0)

int
mrt_dump_bgp_msg(struct mrt_config *mrt, void *pkg, u_int16_t pkglen, int type,
    struct peer_config *peer, struct bgpd_config *bgp)
{
	struct buf	*buf;
	struct imsg_hdr	 hdr;
	int		 i, n;
	u_int16_t	 len;

	len = pkglen + MRT_BGP4MP_HEADER_SIZE + (type > 0 ? MSGSIZE_HEADER : 0);

	hdr.len = len + IMSG_HEADER_SIZE + MRT_HEADER_SIZE;
	hdr.type = IMSG_MRT_MSG;
	hdr.peerid = mrt->id;
	buf = buf_open(hdr.len);
	if (buf == NULL) {
		logit(LOG_ERR, "mrt_dump_bgp_msg: buf_open error");
		return (-1);
	}
	if (buf_add(buf, &hdr, sizeof(hdr)) == -1) {
		logit(LOG_ERR, "mrt_dump_bgp_msg: buf_add error");
		return (-1);
	}

	if (mrt_dump_header(buf, MSG_PROTOCOL_BGP4MP, BGP4MP_MESSAGE, len) ==
	    -1) {
		logit(LOG_ERR, "mrt_dump_bgp_msg: buf_add error");
		return (-1);
	}

	DUMP_SHORT(buf, bgp->as);
	DUMP_SHORT(buf, peer->remote_as);
	DUMP_SHORT(buf, /* ifindex */ 0);
	DUMP_SHORT(buf, 4);
	DUMP_NLONG(buf, peer->local_addr.sin_addr.s_addr);
	DUMP_NLONG(buf, peer->remote_addr.sin_addr.s_addr);

	/* bgp header was chopped off so glue a new one together. */
	if (type > 0) {
		for (i = 0; i < MSGSIZE_HEADER_MARKER; i++)
			DUMP_BYTE(buf, 0xff);
		DUMP_SHORT(buf, pkglen + MSGSIZE_HEADER);
		DUMP_BYTE(buf, type);
	}

	if (buf_add(buf, pkg, pkglen) == -1) {
		logit(LOG_ERR, "mrt_dump_bgp_msg: buf_add error");
		return (-1);
	}

	if ((n = buf_close(mrt->msgbuf, buf)) < 0) {
		logit(LOG_ERR, "mrt_dump_bgp_msg: buf_close error");
		return (-1);
	}

	return (n);
}

int
mrt_dump_state(struct mrt_config *mrt, u_int16_t old_state, u_int16_t new_state,
    struct peer_config *peer, struct bgpd_config *bgp)
{
	struct buf	*buf;
	struct imsg_hdr	 hdr;
	int		 n;
	u_int16_t	 len;

	len = 4 + MRT_BGP4MP_HEADER_SIZE;
	hdr.len = len + IMSG_HEADER_SIZE + MRT_HEADER_SIZE;
	hdr.type = IMSG_MRT_MSG;
	hdr.peerid = mrt->id;
	buf = buf_open(hdr.len);
	if (buf == NULL) {
		logit(LOG_ERR, "mrt_dump_bgp_msg: buf_open error");
		return (-1);
	}
	if (buf_add(buf, &hdr, sizeof(hdr)) == -1) {
		logit(LOG_ERR, "mrt_dump_bgp_msg: buf_add error");
		return (-1);
	}

	if (mrt_dump_header(buf, MSG_PROTOCOL_BGP4MP, BGP4MP_STATE_CHANGE,
	    len) == -1) {
		logit(LOG_ERR, "mrt_dump_bgp_msg: buf_add error");
		return (-1);
	}

	DUMP_SHORT(buf, bgp->as);
	DUMP_SHORT(buf, peer->remote_as);
	DUMP_SHORT(buf, /* ifindex */ 0);
	DUMP_SHORT(buf, 4);
	DUMP_NLONG(buf, peer->local_addr.sin_addr.s_addr);
	DUMP_NLONG(buf, peer->remote_addr.sin_addr.s_addr);

	DUMP_SHORT(buf, old_state);
	DUMP_SHORT(buf, new_state);

	if ((n = buf_close(mrt->msgbuf, buf)) < 0) {
		logit(LOG_ERR, "mrt_dump_bgp_msg: buf_close error");
		return (-1);
	}

	return (n);

}

static int
mrt_dump_entry(struct mrt_config *mrt, struct prefix *p, u_int16_t snum,
    struct peer_config *peer)
{
	struct buf	*buf;
	void		*bptr;
	struct imsg_hdr	 hdr;
	u_int16_t	 len, attr_len;
	int		 n;

	attr_len = attr_length(&p->aspath->flags);
	len = MRT_DUMP_HEADER_SIZE + attr_len;

	hdr.len = len + IMSG_HEADER_SIZE + MRT_HEADER_SIZE;
	hdr.type = IMSG_MRT_MSG;
	hdr.peerid = mrt->id;
	buf = buf_open(hdr.len);
	if (buf == NULL) {
		logit(LOG_ERR, "mrt_dump_entry: buf_open error");
		return (-1);
	}
	if (buf_add(buf, &hdr, sizeof(hdr)) == -1) {
		logit(LOG_ERR, "mrt_dump_entry: buf_add error");
		return (-1);
	}

	if (mrt_dump_header(buf, MSG_TABLE_DUMP, AFI_IPv4, len) == -1) {
		logit(LOG_ERR, "mrt_dump_bgp_msg: buf_add error");
		return (-1);
	}

	DUMP_SHORT(buf, 0);
	DUMP_SHORT(buf, snum);
	DUMP_NLONG(buf, p->prefix->prefix.s_addr);
	DUMP_BYTE(buf, p->prefix->prefixlen);
	DUMP_BYTE(buf, 1);		/* state */
	DUMP_LONG(buf, p->lastchange);	/* originated */
	DUMP_NLONG(buf, peer->remote_addr.sin_addr.s_addr);
	DUMP_SHORT(buf, peer->remote_as);
	DUMP_SHORT(buf, attr_len);

	if ((bptr = buf_reserve(buf, attr_len)) == NULL) {
		logit(LOG_ERR, "mrt_dump_entry: buf_reserve error");
		return (-1);
	}

	if (attr_dump(bptr, attr_len, &p->aspath->flags) == -1) {
		logit(LOG_ERR, "mrt_dump_entry: attr_dump error");
		return (-1);
	}

	if ((n = buf_close(mrt->msgbuf, buf)) < 0) {
		logit(LOG_ERR, "mrt_dump_entry: buf_close error");
		return (-1);
	}

	return (n);
}

static u_int16_t sequencenum = 0;

void
mrt_clear_seq(void)
{
	sequencenum = 0;
}

void
mrt_dump_upcall(struct pt_entry *pt, void *ptr)
{
	struct mrt_config	*mrtbuf = ptr;
	struct prefix		*p;

	/*
	 * dump all prefixes even the inactive ones. That is the way zebra
	 * dumps the table so we do the same. If only the active route should
	 * be dumped p should be set to p = pt->active.
	 */
	LIST_FOREACH(p, &pt->prefix_h, prefix_l)
		mrt_dump_entry(mrtbuf, p, sequencenum++, &p->peer->conf);
}

static int
mrt_dump_header(struct buf *buf, u_int16_t type, u_int16_t subtype,
    u_int32_t len)
{
	time_t			now;

	now = time(NULL);

	DUMP_LONG(buf, now);
	DUMP_SHORT(buf, type);
	DUMP_SHORT(buf, subtype);
	DUMP_LONG(buf, len);

	return (0);
}

static struct imsgbuf	*mrt_imsgbuf[2];

void
mrt_init(struct imsgbuf *rde, struct imsgbuf *se)
{
	mrt_imsgbuf[0] = rde;
	mrt_imsgbuf[1] = se;
}

static int
mrt_open(struct mrt *mrt)
{
	time_t	now;

	now = time(NULL);
	if (strftime(mrt->file, sizeof(mrt->file), mrt->name,
		    localtime(&now)) == 0) {
		logit(LOG_ERR, "mrt_open: strftime conversion failed");
		mrt->msgbuf.sock = -1;
		return (0);
	}

	mrt->msgbuf.sock = open(mrt->file,
	    O_WRONLY|O_NONBLOCK|O_CREAT|O_TRUNC, 0644);
	if (mrt->msgbuf.sock == -1) {
		logit(LOG_ERR, "mrt_open %s: %s",
		    mrt->file, strerror(errno));
		return (0);
	}
	return (1);
}

static int
mrt_close(struct mrt *mrt)
{
	/*
	 * close the mrt filedescriptor but first ensure that the last
	 * mrt message was written correctly. If not mrt_write needs to do
	 * that the next time called.
	 * To ensure this we need to fiddle around with internal msgbuf stuff.
	 */
	if (msgbuf_unbounded(&mrt->msgbuf))
		return (0);

	if (mrt->msgbuf.sock == -1) {
		close(mrt->msgbuf.sock);
		mrt->msgbuf.sock = -1;
	}

	return (1);
}

void
mrt_abort(struct mrt *mrt)
{
	/*
	 * something failed horribly. Stop all dumping and go back to start
	 * position. Retry after MRT_MIN_RETRY or ReopenTimerInterval. Which-
	 * ever is bigger.
	 */
	msgbuf_clear(&mrt->msgbuf);
	mrt_close(mrt);
	mrt->state = MRT_STATE_STOPPED;

	if (MRT_MIN_RETRY > mrt->ReopenTimerInterval)
		mrt->ReopenTimer = MRT_MIN_RETRY + time(NULL);
	else
		mrt->ReopenTimer = mrt->ReopenTimerInterval + time(NULL);
}

int
mrt_queue(struct mrt_head *mrtc, struct imsg *imsg)
{
	struct buf	*wbuf;
	struct mrt	*m;
	ssize_t		 len;
	int		 n;

	if (imsg->hdr.type != IMSG_MRT_MSG && imsg->hdr.type != IMSG_MRT_END)
		return (-1);

	LIST_FOREACH(m, mrtc, list) {
		if (m->conf.id != imsg->hdr.peerid)
			continue;
		if (m->state != MRT_STATE_RUNNING &&
		    m->state != MRT_STATE_REOPEN)
			return (0);

		if (imsg->hdr.type == IMSG_MRT_END) {
			if (mrt_close(m) == 0) {
				m->state = MRT_STATE_CLOSE;
			} else {
				msgbuf_clear(&m->msgbuf);
				m->state = MRT_STATE_STOPPED;
			}
			return (0);
		}

		len = imsg->hdr.len - IMSG_HEADER_SIZE;
		wbuf = buf_open(len);
		if (wbuf == NULL)
			return (-1);
		if (buf_add(wbuf, imsg->data, len) == -1) {
			buf_free(wbuf);
			return (-1);
		}
		if ((n = buf_close(&m->msgbuf, wbuf)) < 0) {
			buf_free(wbuf);
			return (-1);
		}
		return (n);
	}
	return (0);
}

int
mrt_write(struct mrt *mrt)
{
	int	r;

	if (mrt->state == MRT_STATE_REOPEN ||
	    mrt->state == MRT_STATE_CLOSE ||
	    mrt->state == MRT_STATE_REMOVE)
		r = msgbuf_writebound(&mrt->msgbuf);
	else
		r = msgbuf_write(&mrt->msgbuf);

	switch (r) {
	case 1:
		/* only msgbuf_writebound returns 1 */
		break;
	case 0:
		return (0);
	case -1:
		logit(LOG_ERR, "mrt_write: msgbuf_write: %s",
		    strerror(errno));
		mrt_abort(mrt);
		return (0);
	case -2:
		logit(LOG_ERR, "mrt_write: msgbuf_write: %s",
		    "connection closed");
		mrt_abort(mrt);
		return (0);
	default:
		fatalx("mrt_write: unexpected retval from msgbuf_write");
	}

	if (mrt_close(mrt) != 1) {
		logit(LOG_ERR, "mrt_write: mrt_close failed");
		return (0);
	}

	switch (mrt->state) {
	case MRT_STATE_REMOVE:
		/*
		 * Remove request: free all left buffers and
		 * remove the descriptor.
		 */
		msgbuf_clear(&mrt->msgbuf);
		LIST_REMOVE(mrt, list);
		free(mrt);
		return (0);
	case MRT_STATE_CLOSE:
		/* Close request: free all left buffers */
		msgbuf_clear(&mrt->msgbuf);
		mrt->state = MRT_STATE_STOPPED;
		return (0);
	case MRT_STATE_REOPEN:
		if (mrt_open(mrt) == 0) {
			mrt_abort(mrt);
			return (0);
		} else {
			if (mrt->ReopenTimerInterval != 0)
				mrt->ReopenTimer = time(NULL) +
				    mrt->ReopenTimerInterval;
			mrt->state = MRT_STATE_RUNNING;
		}
		break;
	default:
		break;
	}
	return (1);
}

int
mrt_select(struct mrt_head *mc, struct pollfd *pfd, struct mrt **mrt,
    int start, int size, int *timeout)
{
	struct mrt	*m, *xm;
	time_t		 now;
	int		 t;

	now = time(NULL);
	for (m = LIST_FIRST(mc); m != LIST_END(mc); m = xm) {
		xm = LIST_NEXT(m, list);
		if (m->state == MRT_STATE_TOREMOVE) {
			imsg_compose(m->ibuf, IMSG_MRT_END, 0,
					&m->conf, sizeof(m->conf));
			if (mrt_close(m) == 0) {
				m->state = MRT_STATE_REMOVE;
				m->ReopenTimer = 0;
			} else {
				msgbuf_clear(&m->msgbuf);
				LIST_REMOVE(m, list);
				free(m);
				continue;
			}
		}
		if (m->state == MRT_STATE_OPEN) {
			switch (m->conf.type) {
			case MRT_TABLE_DUMP:
			case MRT_FILTERED_IN:
				m->ibuf = mrt_imsgbuf[0];
				break;
			case MRT_ALL_IN:
				m->ibuf = mrt_imsgbuf[1];
				break;
			default:
				continue;
			}
			if (mrt_open(m) == 0) {
				mrt_abort(m);
				t = m->ReopenTimer - now;
				if (*timeout > t)
					*timeout = t;
			}
			m->state = MRT_STATE_RUNNING;
			imsg_compose(m->ibuf, IMSG_MRT_REQ, 0,
					&m->conf, sizeof(m->conf));
		}
		if (m->ReopenTimer != 0) {
			t = m->ReopenTimer - now;
			if (t <= 0 && (m->state == MRT_STATE_RUNNING ||
			    m->state == MRT_STATE_STOPPED)) {
				if (m->state == MRT_STATE_RUNNING) {
					/* reopen file */
					if (mrt_close(m) == 0) {
						m->state = MRT_STATE_REOPEN;
						continue;
					}
				}
				if (mrt_open(m) == 0) {
					mrt_abort(m);
					t = m->ReopenTimer - now;
					if (*timeout > t)
						*timeout = t;
				}
				m->state = MRT_STATE_RUNNING;
				if (m->ReopenTimerInterval != 0) {
					m->ReopenTimer = now +
					    m->ReopenTimerInterval;
					if (*timeout > m->ReopenTimerInterval)
						*timeout = t;
				}
			}
		}
		if (m->msgbuf.queued > 0) {
			if (m->msgbuf.sock == -1 ||
			    m->state == MRT_STATE_STOPPED) {
				logit(LOG_ERR, "mrt_select: orphaned buffer");
				mrt_abort(m);
				continue;
			}
			if (start > size) {
				pfd[start].fd = m->msgbuf.sock;
				pfd[start].events |= POLLOUT;
				mrt[start++] = m;
			}
		}
	}
	return (start);
}

int
mrt_handler(struct mrt_head *mrt)
{
	struct mrt	*m;
	time_t		 now;

	now = time(NULL);
	LIST_FOREACH(m, mrt, list) {
		if (m->conf.type == MRT_TABLE_DUMP) {
			if (m->state == MRT_STATE_STOPPED) {
				if (mrt_open(m) == 0) {
					mrt_abort(m);
					break;
				}
				imsg_compose(mrt_imsgbuf[0], IMSG_MRT_REQ, 0,
						&m->conf, sizeof(m->conf));
				m->state = MRT_STATE_RUNNING;
			}
		}
		if (m->state == MRT_STATE_RUNNING)
			m->state = MRT_STATE_REOPEN;
		if (m->ReopenTimerInterval != 0)
			m->ReopenTimer = now + m->ReopenTimerInterval;
	}
	return (0);
}

static u_int32_t	 max_id = 1;

static struct mrt *
getconf(struct mrt_head *c, struct mrt *m)
{
	struct mrt	*t;

	LIST_FOREACH(t, c, list) {
		if (t->conf.type != m->conf.type)
			continue;
		if (t->conf.type == MRT_TABLE_DUMP ||
		    t->conf.type == MRT_ALL_IN ||
		    t->conf.type == MRT_FILTERED_IN)
			return t;
		if (t->conf.peer_id == m->conf.peer_id)
			return t;
	}
	return (NULL);
}

int
mrt_mergeconfig(struct mrt_head *xconf, struct mrt_head *nconf)
{
	struct mrt	*m, *xm;

	LIST_FOREACH(m, nconf, list)
		if ((xm = getconf(xconf, m)) == NULL) {
			/* NEW */
			if ((xm = calloc(1, sizeof(struct mrt))) ==
			    NULL)
				fatal("mrt_mergeconfig");
			memcpy(xm, m, sizeof(struct mrt));
			msgbuf_init(&xm->msgbuf);
			xm->conf.id = max_id++;
			xm->state = MRT_STATE_OPEN;
			LIST_INSERT_HEAD(xconf, xm, list);
		} else {
			/* MERGE */
			if (strlcpy(xm->name, m->name, sizeof(xm->name)) >
			    sizeof(xm->name))
				fatalx("mrt_mergeconfig: strlcpy");
			xm->ReopenTimerInterval = m->ReopenTimerInterval;
			xm->state = MRT_STATE_REOPEN;
		}

	LIST_FOREACH(xm, xconf, list)
		if (getconf(nconf, xm) == NULL)
			/* REMOVE */
			xm->state = MRT_STATE_TOREMOVE;

	/* free config */
	for (m = LIST_FIRST(nconf); m != LIST_END(nconf); m = xm) {
		xm = LIST_NEXT(m, list);
		free(m);
	}

	return (0);
}

