/*	$OpenBSD: src/usr.sbin/relayd/ssl.c,v 1.3 2007/02/06 10:06:55 reyk Exp $	*/

/*
 * Copyright (c) 2006 Pierre-Yves Ritschard <pyr@spootnik.org>
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
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <net/if.h>
#include <limits.h>
#include <event.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "hoststated.h"

void	ssl_read(int, short, void *);
void	ssl_write(int, short, void *);
void	ssl_connect(int, short, void *);
void	ssl_cleanup(struct ctl_tcp_event *);

void
ssl_read(int s, short event, void *arg)
{
	struct ctl_tcp_event	*cte = arg;
	int			 ret;
	int			 ssl_err;
	int			 retry_flag;
	char			 rbuf[SMALL_READ_BUF_SIZE];

	if (event == EV_TIMEOUT) {
		cte->host->up = HOST_DOWN;
		ssl_cleanup(cte);
		hce_notify_done(cte->host, "ssl_read: timeout");
		return;
	}
	log_debug("ssl_read: event occurred");

	bzero(rbuf, sizeof(rbuf));
	ssl_err = 0;
	retry_flag = EV_READ;

	ret = SSL_read(cte->ssl, rbuf, sizeof(rbuf));

	if (ret <= 0) {
		ssl_err = SSL_get_error(cte->ssl, ret);
		switch (ssl_err) {
		case SSL_ERROR_WANT_READ:
			log_debug("ssl_read: want read");
			retry_flag = EV_READ;
			goto retry;
		case SSL_ERROR_WANT_WRITE:
			log_debug("ssl_read: want read");
			retry_flag = EV_WRITE;
			goto retry;
		case SSL_ERROR_ZERO_RETURN: /* FALLTHROUGH */
		case SSL_ERROR_SYSCALL:
			if (ret == 0) {
				cte->host->up = HOST_DOWN;
				(void)cte->validate_close(cte);
				ssl_cleanup(cte);
				if (cte->host->up == HOST_UP)
					hce_notify_done(cte->host,
					    "ssl_read: check succeeded");
				else
					hce_notify_done(cte->host,
					    "ssl_read: check failed");
				return;
			}
			/* FALLTHROUGH */
		default:
			cte->host->up = HOST_DOWN;
			ssl_error(cte->host->name, "cannot read");
			ssl_cleanup(cte);
			hce_notify_done(cte->host, "ssl_read: SSL error");
			break;
		}
		return;
	} 
	buf_add(cte->buf, rbuf, ret);

	if (cte->validate_read != NULL) {
		if (cte->validate_read(cte) != 0)
			goto retry;

		ssl_cleanup(cte);
		if (cte->host->up == HOST_UP)
			hce_notify_done(cte->host, "ssl_read: check succeeded");
		else
			hce_notify_done(cte->host, "ssl_read: check failed");
		return;
	}

retry:
	log_debug("ssl_read: scheduling ssl_read on %s",
	    (retry_flag == EV_READ) ? "EV_READ" : "EV_WRITE");
	event_again(&cte->ev, s, EV_TIMEOUT|retry_flag, ssl_read,
	    &cte->tv_start, &cte->table->timeout, cte);
	return;
}

void
ssl_write(int s, short event, void *arg)
{
	struct ctl_tcp_event	*cte = arg;
	int			 len;
	int		 	 ret;
	int			 ssl_err;
	int			 retry_flag;

	if (event == EV_TIMEOUT) {
		cte->host->up = HOST_DOWN;
		ssl_cleanup(cte);
		hce_notify_done(cte->host, "ssl_write: timeout");
		return;
	}

	log_debug("ssl_write: event occurred");
	len = strlen(cte->table->sendbuf);
	retry_flag = EV_WRITE;

	ret = SSL_write(cte->ssl, cte->table->sendbuf, len); 

	if (ret <= 0) {
		ssl_err = SSL_get_error(cte->ssl, ret);
		switch (ssl_err) {
		case SSL_ERROR_WANT_READ:
			log_debug("ssl_write: want read");
			retry_flag = EV_READ;
			goto retry;
		case SSL_ERROR_WANT_WRITE:
			log_debug("ssl_write: want write");
			retry_flag = EV_WRITE;
			goto retry;
		default:
			cte->host->up = HOST_DOWN;
			ssl_error(cte->host->name, "cannot write");
			ssl_cleanup(cte);
			hce_notify_done(cte->host, "ssl_write: SSL error");
			return;
		}
	}
	if ((cte->buf = buf_dynamic(SMALL_READ_BUF_SIZE, UINT_MAX)) == NULL)
		fatalx("ssl_write: cannot create dynamic buffer");

	log_debug("ssl_write: scheduling ssl_read on EV_READ");
	event_again(&cte->ev, s, EV_TIMEOUT|EV_READ, ssl_read,
	    &cte->tv_start, &cte->table->timeout, cte);
	return;
retry:
	log_debug("ssl_write: scheduling ssl_write on %s",
	    (retry_flag == EV_READ) ? "EV_READ" : "EV_WRITE");
	event_again(&cte->ev, s, EV_TIMEOUT|retry_flag, ssl_write,
	    &cte->tv_start, &cte->table->timeout, cte);
}

void
ssl_connect(int s, short event, void *arg)
{
	struct ctl_tcp_event	*cte = arg;
	int		 	 ret;
	int			 ssl_err;
	int			 retry_flag;

	if (event == EV_TIMEOUT) {
		cte->host->up = HOST_DOWN;
		hce_notify_done(cte->host, "ssl_connect: timeout");
		return;
	}

	retry_flag = ssl_err = 0;

	ret = SSL_connect(cte->ssl); 

	if (ret <= 0) {
		ssl_err = SSL_get_error(cte->ssl, ret);
		switch (ssl_err) {
		case SSL_ERROR_WANT_READ:
			log_debug("ssl_connect: want read");
			retry_flag = EV_READ;
			goto retry;
		case SSL_ERROR_WANT_WRITE:
			log_debug("ssl_connect: want write");
			retry_flag = EV_WRITE;
			goto retry;
		default:
			cte->host->up = HOST_DOWN;
			ssl_error(cte->host->name, "cannot connect");
			hce_notify_done(cte->host, "ssl_connect: SSL error");
			ssl_cleanup(cte);
			return;
		}
	}

	if (cte->table->check == CHECK_TCP) {
		cte->host->up = HOST_UP;
		hce_notify_done(cte->host, "ssl_connect: connect successful");
		ssl_cleanup(cte);
		return;
	}
	log_debug("ssl_connect: connect succeeded");
	if (cte->table->sendbuf != NULL) {
		log_debug("ssl_connect: scheduling ssl_write on EV_WRITE");
		event_again(&cte->ev, cte->s, EV_TIMEOUT|EV_WRITE, ssl_write,
		    &cte->tv_start, &cte->table->timeout, cte);
		return;
	}

	if ((cte->buf = buf_dynamic(SMALL_READ_BUF_SIZE, UINT_MAX)) == NULL)
		fatalx("ssl_connect: cannot create dynamic buffer");
	log_debug("ssl_connect: scheduling ssl_read on EV_READ");
	event_again(&cte->ev, cte->s, EV_TIMEOUT|EV_READ, ssl_read,
	    &cte->tv_start, &cte->table->timeout, cte);
	return;

retry:
	log_debug("ssl_write: scheduling ssl_write on %s",
	    (retry_flag == EV_READ) ? "EV_READ" : "EV_WRITE");
	event_again(&cte->ev, s, EV_TIMEOUT|retry_flag, ssl_connect,
	    &cte->tv_start, &cte->table->timeout, cte);
}

void
ssl_cleanup(struct ctl_tcp_event *cte)
{
	log_debug("ssl_cleanup: cleaning for %s", cte->host->name);
	close(cte->s);
	if (cte->ssl != NULL)
		SSL_free(cte->ssl);
	if (cte->buf != NULL)
		buf_free(cte->buf);
}

void
ssl_error(const char *where, const char *what)
{
	unsigned long	 code;
	char		 errbuf[128];
	
	for (; (code = ERR_get_error()) != 0 ;) {
		ERR_error_string_n(code, errbuf, sizeof(errbuf));
		log_debug("SSL library error: %s: %s: %s", where, what, errbuf);
	}
}

void
ssl_init(struct hoststated *env)
{
	SSL_library_init();
	SSL_load_error_strings();
}

void
ssl_transaction(struct ctl_tcp_event *cte)
{
	cte->ssl = SSL_new(cte->table->ssl_ctx);
	if (cte->ssl == NULL) {
		ssl_error(cte->host->name, "cannot create object");
		fatal("cannot create SSL object");
	}

	if (SSL_set_fd(cte->ssl, cte->s) == 0) {
		cte->host->up = HOST_UNKNOWN;
		ssl_error(cte->host->name, "cannot set fd"); 
		ssl_cleanup(cte);
		hce_notify_done(cte->host, "cannot set SSL fd");
		return;
	}
	SSL_set_connect_state(cte->ssl);

	event_again(&cte->ev, cte->s, EV_TIMEOUT|EV_WRITE, ssl_connect,
	    &cte->tv_start, &cte->table->timeout, cte);
}

SSL_CTX *
ssl_ctx_create(struct hoststated *env)
{
	SSL_CTX	*ctx;

	ctx = SSL_CTX_new(SSLv23_client_method());
	if (ctx == NULL) {
		ssl_error("ssl_ctx_create", "cannot create context");
		fatal("could not create SSL context");
	}
	return (ctx);
}
