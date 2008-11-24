/*	$OpenBSD: src/usr.sbin/btd/bt.c,v 1.1 2008/11/24 23:34:41 uwe Exp $ */

/*
 * Copyright (c) 2008 Uwe Stuehler <uwe@openbsd.org>
 * Copyright (c) 2003, 2004 Henning Brauer <henning@openbsd.org>
 * Copyright (c) 2004 Alexander Guy <alexander.guy@andern.org>
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <dev/bluetooth/btdev.h>

#include <bluetooth.h>
#include <errno.h>
#include <event.h>
#include <fcntl.h>
#include <paths.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <usbhid.h>

#include "btd.h"

void bt_sighdlr(int, short, void *);

struct event ev_sigint;
struct event ev_sigterm;
struct event ev_siginfo;

struct bufferevent *ev_priv;
void bt_priv_readcb(struct bufferevent *, void *);
void bt_priv_writecb(struct bufferevent *, void *);
void bt_priv_errorcb(struct bufferevent *, short, void *);

void
bt_sighdlr(int sig, short ev, void *arg)
{
	switch (sig) {
	case SIGINT:
	case SIGTERM:
		(void)event_loopexit(NULL);
		break;

	case SIGINFO:
		/* report status */
		break;
	}
}

pid_t
bt_main(int pipe_prnt[2], struct btd *env, struct passwd *pw)
{
	struct stat stb;
	pid_t pid;

	switch (pid = fork()) {
	case -1:
		fatal("cannot fork");
		break;
	case 0:
		break;
	default:
		return pid;
	}

	setproctitle("bt engine");

	event_init();

	hid_init(NULL);

	db_open(BTD_DB, &env->db);

	if (hci_init(env))
		fatalx("can't set up HCI listeners");

	control_init(env);

	if (stat(pw->pw_dir, &stb) == -1)
		fatal("stat");
	if (stb.st_uid != 0 || (stb.st_mode & (S_IWGRP|S_IWOTH)) != 0)
		fatalx("bad privsep dir permissions");
	if (chroot(pw->pw_dir) == -1)
		fatal("chroot");
	if (chdir("/") == -1)
		fatal("chdir(\"/\")");

	if (setgroups(1, &pw->pw_gid) ||
	    setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) ||
	    setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid))
		fatal("can't drop privileges");

	close(pipe_prnt[0]);

	if ((ev_priv = bufferevent_new(pipe_prnt[1], bt_priv_readcb,
	    bt_priv_writecb, bt_priv_errorcb, env)) == NULL)
		fatalx("bufferevent_new ev_priv");

	bufferevent_enable(ev_priv, EV_READ);

	signal_set(&ev_sigint, SIGINT, bt_sighdlr, NULL);
	signal_set(&ev_sigterm, SIGTERM, bt_sighdlr, NULL);
	signal_set(&ev_siginfo, SIGINFO, bt_sighdlr, NULL);
	signal_add(&ev_sigint, NULL);
	signal_add(&ev_sigterm, NULL);
	signal_add(&ev_siginfo, NULL);

	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGCHLD, SIG_DFL);

	log_info("bt engine ready");

	(void)event_dispatch();

	log_info("bt engine exiting");
	exit(0);
}

void
bt_priv_readcb(struct bufferevent *ev, void *arg)
{
	log_debug(__func__);
}

void
bt_priv_writecb(struct bufferevent *ev, void *arg)
{
	/* nothing to do */
	log_debug(__func__);
}

void
bt_priv_errorcb(struct bufferevent *ev, short what, void *arg)
{
	log_warnx("priv pipe error, what=%#x", what);
	exit(0);
}
