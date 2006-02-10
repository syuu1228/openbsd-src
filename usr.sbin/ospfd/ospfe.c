/*	$OpenBSD: src/usr.sbin/ospfd/ospfe.c,v 1.38 2006/02/10 18:30:47 claudio Exp $ */

/*
 * Copyright (c) 2005 Claudio Jeker <claudio@openbsd.org>
 * Copyright (c) 2004 Esben Norby <norby@openbsd.org>
 * Copyright (c) 2003, 2004 Henning Brauer <henning@openbsd.org>
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
#include <sys/socket.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_types.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#include <event.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "ospf.h"
#include "ospfd.h"
#include "ospfe.h"
#include "rde.h"
#include "control.h"
#include "log.h"

void		 ospfe_sig_handler(int, short, void *);
void		 ospfe_shutdown(void);
void		 orig_rtr_lsa_all(struct area *);
struct iface	*find_vlink(struct abr_rtr *);

volatile sig_atomic_t	 ospfe_quit = 0;
struct ospfd_conf	*oeconf = NULL;
struct imsgbuf		*ibuf_main;
struct imsgbuf		*ibuf_rde;
struct ctl_conn		*ctl_conn;

void
ospfe_sig_handler(int sig, short event, void *bula)
{
	switch (sig) {
	case SIGINT:
	case SIGTERM:
		ospfe_shutdown();
		/* NOTREACHED */
	default:
		fatalx("unexpected signal");
		/* NOTREACHED */
	}
}

/* ospf engine */
pid_t
ospfe(struct ospfd_conf *xconf, int pipe_parent2ospfe[2], int pipe_ospfe2rde[2],
    int pipe_parent2rde[2])
{
	struct area	*area = NULL;
	struct iface	*iface = NULL;
	struct passwd	*pw;
	struct event	 ev_sigint, ev_sigterm;
	pid_t		 pid;

	switch (pid = fork()) {
	case -1:
		fatal("cannot fork");
	case 0:
		break;
	default:
		return (pid);
	}

	/* create ospfd control socket outside chroot */
	if (control_init() == -1)
		fatalx("control socket setup failed");

	/* create the raw ip socket */
	if ((xconf->ospf_socket = socket(AF_INET, SOCK_RAW,
	    IPPROTO_OSPF)) == -1)
		fatal("error creating raw socket");

	/* set some defaults */
	if (if_set_mcast_ttl(xconf->ospf_socket,
	    IP_DEFAULT_MULTICAST_TTL) == -1)
		fatal("if_set_mcast_ttl");

	if (if_set_mcast_loop(xconf->ospf_socket) == -1)
		fatal("if_set_mcast_loop");

	if (if_set_tos(xconf->ospf_socket, IPTOS_PREC_INTERNETCONTROL) == -1)
		fatal("if_set_tos");

	if_set_recvbuf(xconf->ospf_socket);

	oeconf = xconf;

	if ((pw = getpwnam(OSPFD_USER)) == NULL)
		fatal("getpwnam");

	if (chroot(pw->pw_dir) == -1)
		fatal("chroot");
	if (chdir("/") == -1)
		fatal("chdir(\"/\")");

	setproctitle("ospf engine");
	ospfd_process = PROC_OSPF_ENGINE;

	if (setgroups(1, &pw->pw_gid) ||
	    setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) ||
	    setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid))
		fatal("can't drop privileges");

	event_init();
	nbr_init(NBR_HASHSIZE);
	lsa_cache_init(LSA_HASHSIZE);

	/* setup signal handler */
	signal_set(&ev_sigint, SIGINT, ospfe_sig_handler, NULL);
	signal_set(&ev_sigterm, SIGTERM, ospfe_sig_handler, NULL);
	signal_add(&ev_sigint, NULL);
	signal_add(&ev_sigterm, NULL);
	signal(SIGPIPE, SIG_IGN);

	/* setup pipes */
	close(pipe_parent2ospfe[0]);
	close(pipe_ospfe2rde[1]);
	close(pipe_parent2rde[0]);
	close(pipe_parent2rde[1]);

	if ((ibuf_rde = malloc(sizeof(struct imsgbuf))) == NULL ||
	    (ibuf_main = malloc(sizeof(struct imsgbuf))) == NULL)
		fatal(NULL);
	imsg_init(ibuf_rde, pipe_ospfe2rde[0], ospfe_dispatch_rde);
	imsg_init(ibuf_main, pipe_parent2ospfe[1], ospfe_dispatch_main);

	/* setup event handler */
	ibuf_rde->events = EV_READ;
	event_set(&ibuf_rde->ev, ibuf_rde->fd, ibuf_rde->events,
	    ibuf_rde->handler, ibuf_rde);
	event_add(&ibuf_rde->ev, NULL);

	ibuf_main->events = EV_READ;
	event_set(&ibuf_main->ev, ibuf_main->fd, ibuf_main->events,
	    ibuf_main->handler, ibuf_main);
	event_add(&ibuf_main->ev, NULL);

	event_set(&oeconf->ev, oeconf->ospf_socket, EV_READ|EV_PERSIST,
	    recv_packet, oeconf);
	event_add(&oeconf->ev, NULL);

	/* listen on ospfd control socket */
	TAILQ_INIT(&ctl_conns);
	control_listen();

	/* start interfaces */
	LIST_FOREACH(area, &oeconf->area_list, entry) {
		LIST_FOREACH(iface, &area->iface_list, entry) {
			if_init(xconf, iface);
			if (if_fsm(iface, IF_EVT_UP)) {
				log_debug("error starting interface %s",
				    iface->name);
			}
		}
	}

	event_dispatch();

	ospfe_shutdown();
	/* NOTREACHED */
	return (0);
}

void
ospfe_shutdown(void)
{
	struct area	*area;
	struct iface	*iface;

	/* stop all interfaces and remove all areas */
	while ((area = LIST_FIRST(&oeconf->area_list)) != NULL) {
		LIST_FOREACH(iface, &area->iface_list, entry) {
			if (if_fsm(iface, IF_EVT_DOWN)) {
				log_debug("error stopping interface %s",
				    iface->name);
			}
		}
		LIST_REMOVE(area, entry);
		area_del(area);
	}

	close(oeconf->ospf_socket);

	/* clean up */
	msgbuf_write(&ibuf_rde->w);
	msgbuf_clear(&ibuf_rde->w);
	free(ibuf_rde);
	msgbuf_write(&ibuf_main->w);
	msgbuf_clear(&ibuf_main->w);
	free(ibuf_main);
	free(oeconf);

	log_info("ospf engine exiting");
	_exit(0);
}

/* imesg */
int
ospfe_imsg_compose_parent(int type, pid_t pid, void *data, u_int16_t datalen)
{
	return (imsg_compose(ibuf_main, type, 0, pid, data, datalen));
}

int
ospfe_imsg_compose_rde(int type, u_int32_t peerid, pid_t pid,
    void *data, u_int16_t datalen)
{
	return (imsg_compose(ibuf_rde, type, peerid, pid, data, datalen));
}

void
ospfe_dispatch_main(int fd, short event, void *bula)
{
	struct imsg	 imsg;
	struct imsgbuf  *ibuf = bula;
	struct area	*area = NULL;
	struct iface	*iface = NULL;
	struct kif	*kif;
	int		 n, link_ok;

	switch (event) {
	case EV_READ:
		if ((n = imsg_read(ibuf)) == -1)
			fatal("imsg_read error");
		if (n == 0)	/* connection closed */
			fatalx("pipe closed");
		break;
	case EV_WRITE:
		if (msgbuf_write(&ibuf->w) == -1)
			fatal("msgbuf_write");
		imsg_event_add(ibuf);
		return;
	default:
		fatalx("unknown event");
	}

	for (;;) {
		if ((n = imsg_get(ibuf, &imsg)) == -1)
			fatal("ospfe_dispatch_main: imsg_read error");
		if (n == 0)
			break;

		switch (imsg.hdr.type) {
		case IMSG_IFINFO:
			if (imsg.hdr.len != IMSG_HEADER_SIZE +
			    sizeof(struct kif))
				fatalx("IFINFO imsg with wrong len");
			kif = imsg.data;
			link_ok = (kif->flags & IFF_UP) &&
			    (kif->link_state == LINK_STATE_UP ||
			    (kif->link_state == LINK_STATE_UNKNOWN &&
			    kif->media_type != IFT_CARP));


			LIST_FOREACH(area, &oeconf->area_list, entry) {
				LIST_FOREACH(iface, &area->iface_list, entry) {
					if (kif->ifindex == iface->ifindex &&
					    iface->type != IF_TYPE_VIRTUALLINK) {
						iface->flags = kif->flags;
						iface->linkstate =
						    kif->link_state;

						if (link_ok) {
							if_fsm(iface,
							    IF_EVT_UP);
						} else {
							if_fsm(iface,
							    IF_EVT_DOWN);
						}
					}
				}
			}
			break;
		case IMSG_CTL_KROUTE:
		case IMSG_CTL_KROUTE_ADDR:
		case IMSG_CTL_IFINFO:
		case IMSG_CTL_END:
			control_imsg_relay(&imsg);
			break;
		default:
			log_debug("ospfe_dispatch_main: error handling imsg %d",
			    imsg.hdr.type);
			break;
		}
		imsg_free(&imsg);
	}
	imsg_event_add(ibuf);
}

void
ospfe_dispatch_rde(int fd, short event, void *bula)
{
	struct lsa_hdr		 lsa_hdr;
	struct imsgbuf		*ibuf = bula;
	struct nbr		*nbr;
	struct lsa_hdr		*lhp;
	struct lsa_ref		*ref;
	struct area		*area;
	struct iface		*iface;
	struct lsa_entry	*le;
	struct imsg		 imsg;
	struct abr_rtr		 ar;
	int			 n, noack = 0;
	u_int16_t		 l, age;

	switch (event) {
	case EV_READ:
		if ((n = imsg_read(ibuf)) == -1)
			fatal("imsg_read error");
		if (n == 0)	/* connection closed */
			fatalx("pipe closed");
		break;
	case EV_WRITE:
		if (msgbuf_write(&ibuf->w) == -1)
			fatal("msgbuf_write");
		imsg_event_add(ibuf);
		return;
	default:
		fatalx("unknown event");
	}

	for (;;) {
		if ((n = imsg_get(ibuf, &imsg)) == -1)
			fatal("ospfe_dispatch_rde: imsg_read error");
		if (n == 0)
			break;

		switch (imsg.hdr.type) {
		case IMSG_DD:
			nbr = nbr_find_peerid(imsg.hdr.peerid);
			if (nbr == NULL)
				fatalx("ospfe_dispatch_rde: "
				    "neighbor not found");

			/* put these on my ls_req_list for retrieval */
			lhp = lsa_hdr_new();
			memcpy(lhp, imsg.data, sizeof(*lhp));
			ls_req_list_add(nbr, lhp);
			break;
		case IMSG_DD_END:
			nbr = nbr_find_peerid(imsg.hdr.peerid);
			if (nbr == NULL)
				fatalx("ospfe_dispatch_rde: "
				    "neighbor not found");

			nbr->dd_pending--;
			if (nbr->dd_pending == 0 && nbr->state & NBR_STA_LOAD) {
				if (ls_req_list_empty(nbr))
					nbr_fsm(nbr, NBR_EVT_LOAD_DONE);
				else
					start_ls_req_tx_timer(nbr);
			}
			break;
		case IMSG_DB_SNAPSHOT:
			nbr = nbr_find_peerid(imsg.hdr.peerid);
			if (nbr == NULL)
				fatalx("ospfe_dispatch_rde: "
				    "neighbor not found");

			/* add LSA header to the neighbor db_sum_list */
			lhp = lsa_hdr_new();
			memcpy(lhp, imsg.data, sizeof(*lhp));
			db_sum_list_add(nbr, lhp);
			break;
		case IMSG_DB_END:
			nbr = nbr_find_peerid(imsg.hdr.peerid);
			if (nbr == NULL)
				fatalx("ospfe_dispatch_rde: "
				    "neighbor not found");

			/* snapshot done, start tx of dd packets */
			nbr_fsm(nbr, NBR_EVT_SNAP_DONE);
			break;
		case IMSG_LS_FLOOD:
			nbr = nbr_find_peerid(imsg.hdr.peerid);
			if (nbr == NULL)
				fatalx("ospfe_dispatch_rde: "
				    "neighbor not found");

			l = imsg.hdr.len - IMSG_HEADER_SIZE;
			if (l < sizeof(lsa_hdr))
				fatalx("ospfe_dispatch_rde: "
				    "bad imsg size");
			memcpy(&lsa_hdr, imsg.data, sizeof(lsa_hdr));

			ref = lsa_cache_add(imsg.data, l);

			if (lsa_hdr.type == LSA_TYPE_EXTERNAL) {
				/*
				 * flood on all areas but stub areas and
				 * virtual links
				 */
				LIST_FOREACH(area, &oeconf->area_list, entry) {
				    if (area->stub)
					    continue;
				    LIST_FOREACH(iface, &area->iface_list,
					entry) {
					    noack += lsa_flood(iface, nbr,
						&lsa_hdr, imsg.data, l);
				    }
				}
			} else {
				/*
				 * flood on all area interfaces on
				 * area 0.0.0.0 include also virtual links.
				 */
				area = nbr->iface->area;
				LIST_FOREACH(iface, &area->iface_list, entry) {
					noack += lsa_flood(iface, nbr,
					    &lsa_hdr, imsg.data, l);
				}
				/* XXX virtual links */
			}

			/* remove from ls_req_list */
			le = ls_req_list_get(nbr, &lsa_hdr);
			if (!(nbr->state & NBR_STA_FULL) && le != NULL) {
				ls_req_list_free(nbr, le);
				/*
				 * XXX no need to ack requested lsa
				 * the problem is that the RFC is very
				 * unclear about this.
				 */
				noack = 1;
			}

			if (!noack && nbr->iface != NULL &&
			    nbr->iface->self != nbr) {
				if (!(nbr->iface->state & IF_STA_BACKUP) ||
				    nbr->iface->dr == nbr) {
					/* delayed ack */
					lhp = lsa_hdr_new();
					memcpy(lhp, &lsa_hdr, sizeof(*lhp));
					ls_ack_list_add(nbr->iface, lhp);
				}
			}

			lsa_cache_put(ref, nbr);
			break;
		case IMSG_LS_UPD:
			/*
			 * IMSG_LS_UPD is used in three cases:
			 * 1. as response to ls requests
			 * 2. as response to ls updates where the DB
			 *    is newer then the sent LSA
			 * 3. in EXSTART when the LSA has age MaxAge
			 */
			l = imsg.hdr.len - IMSG_HEADER_SIZE;
			if (l < sizeof(lsa_hdr))
				fatalx("ospfe_dispatch_rde: "
				    "bad imsg size");

			nbr = nbr_find_peerid(imsg.hdr.peerid);
			if (nbr == NULL)
				fatalx("ospfe_dispatch_rde: "
				    "neighbor not found");

			if (nbr->iface->self == nbr)
				break;

			memcpy(&age, imsg.data, sizeof(age));
			ref = lsa_cache_add(imsg.data, l);
			if (ntohs(age) >= MAX_AGE)
				/* add to retransmit list */
				ls_retrans_list_add(nbr, imsg.data, 0, 0);
			else
				ls_retrans_list_add(nbr, imsg.data, 0, 1);

			lsa_cache_put(ref, nbr);
			break;
		case IMSG_LS_ACK:
			/*
			 * IMSG_LS_ACK is used in two cases:
			 * 1. LSA was a duplicate
			 * 2. LS age is MaxAge and there is no current
			 *    instance in the DB plus no neighbor is state
			 *    Exchange or Loading
			 */
			nbr = nbr_find_peerid(imsg.hdr.peerid);
			if (nbr == NULL)
				fatalx("ospfe_dispatch_rde: "
				    "neighbor not found");

			if (nbr->iface->self == nbr)
				break;

			/* TODO for case one check for implied acks */

			/* send a direct acknowledgement */
			send_ls_ack(nbr->iface, nbr->addr, imsg.data,
			    imsg.hdr.len - IMSG_HEADER_SIZE);

			break;
		case IMSG_LS_BADREQ:
			nbr = nbr_find_peerid(imsg.hdr.peerid);
			if (nbr == NULL)
				fatalx("ospfe_dispatch_rde: "
				    "neighbor not found");

			if (nbr->iface->self == nbr)
				fatalx("ospfe_dispatch_rde: "
				    "dummy neighbor got BADREQ");

			nbr_fsm(nbr, NBR_EVT_BAD_LS_REQ);
			break;
		case IMSG_ABR_UP:
			memcpy(&ar, imsg.data, sizeof(ar));

			if ((iface = find_vlink(&ar)) != NULL &&
			    iface->state == IF_STA_DOWN)
				if (if_fsm(iface, IF_EVT_UP)) {
					log_debug("error starting interface %s",
					    iface->name);
				}
			break;
		case IMSG_ABR_DOWN:
			memcpy(&ar, imsg.data, sizeof(ar));

			if ((iface = find_vlink(&ar)) != NULL &&
			    iface->state == IF_STA_POINTTOPOINT)
				if (if_fsm(iface, IF_EVT_DOWN)) {
					log_debug("error stopping interface %s",
					    iface->name);
				}
			break;
		case IMSG_CTL_AREA:
		case IMSG_CTL_END:
		case IMSG_CTL_SHOW_DATABASE:
		case IMSG_CTL_SHOW_DB_EXT:
		case IMSG_CTL_SHOW_DB_NET:
		case IMSG_CTL_SHOW_DB_RTR:
		case IMSG_CTL_SHOW_DB_SELF:
		case IMSG_CTL_SHOW_DB_SUM:
		case IMSG_CTL_SHOW_DB_ASBR:
		case IMSG_CTL_SHOW_RIB:
		case IMSG_CTL_SHOW_SUM:
		case IMSG_CTL_SHOW_SUM_AREA:
			control_imsg_relay(&imsg);
			break;
		default:
			log_debug("ospfe_dispatch_rde: error handling imsg %d",
			    imsg.hdr.type);
			break;
		}
		imsg_free(&imsg);
	}
	imsg_event_add(ibuf);
}

struct iface *
find_vlink(struct abr_rtr *ar)
{
	struct area	*area;
	struct iface	*iface = NULL;

	LIST_FOREACH(area, &oeconf->area_list, entry)
		LIST_FOREACH(iface, &area->iface_list, entry)
			if (iface->abr_id.s_addr == ar->abr_id.s_addr &&
			    iface->type == IF_TYPE_VIRTUALLINK &&
			    iface->area->id.s_addr == ar->area.s_addr) {
				iface->dst.s_addr = ar->dst_ip.s_addr;
				iface->addr.s_addr = ar->addr.s_addr;
				iface->metric = ar->metric;

				return (iface);
			}

	return (iface);
}

void
orig_rtr_lsa_all(struct area *area)
{
	struct area	*a;

	/*
	 * update all router LSA in all areas except area itself,
	 * as this update is already running.
	 */
	LIST_FOREACH(a, &oeconf->area_list, entry)
		if (a != area)
			orig_rtr_lsa(a);
}

void
orig_rtr_lsa(struct area *area)
{
	struct lsa_hdr		 lsa_hdr;
	struct lsa_rtr		 lsa_rtr;
	struct lsa_rtr_link	 rtr_link;
	struct iface		*iface;
	struct buf		*buf;
	struct nbr		*nbr, *self = NULL;
	u_int16_t		 num_links = 0;
	u_int16_t		 chksum;
	u_int8_t		 border, virtual = 0;

	log_debug("orig_rtr_lsa: area %s", inet_ntoa(area->id));

	/* XXX READ_BUF_SIZE */
	if ((buf = buf_dynamic(sizeof(lsa_hdr), READ_BUF_SIZE)) == NULL)
		fatal("orig_rtr_lsa");

	/* reserve space for LSA header and LSA Router header */
	if (buf_reserve(buf, sizeof(lsa_hdr)) == NULL)
		fatal("orig_rtr_lsa: buf_reserve failed");

	if (buf_reserve(buf, sizeof(lsa_rtr)) == NULL)
		fatal("orig_rtr_lsa: buf_reserve failed");

	/* links */
	LIST_FOREACH(iface, &area->iface_list, entry) {
		if (self == NULL && iface->self != NULL)
			self = iface->self;

		bzero(&rtr_link, sizeof(rtr_link));

		if (iface->state & IF_STA_LOOPBACK) {
			rtr_link.id = iface->addr.s_addr;
			rtr_link.data = 0xffffffff;
			rtr_link.type = LINK_TYPE_STUB_NET;
			num_links++;
			if (buf_add(buf, &rtr_link, sizeof(rtr_link)))
				fatalx("orig_rtr_lsa: buf_add failed");
			continue;
		}

		switch (iface->type) {
		case IF_TYPE_POINTOPOINT:
			LIST_FOREACH(nbr, &iface->nbr_list, entry)
				if (nbr != iface->self &&
				    nbr->state & NBR_STA_FULL)
					break;
			if (nbr) {
				log_debug("orig_rtr_lsa: point-to-point, "
				    "interface %s", iface->name);
				rtr_link.id = nbr->id.s_addr;
				rtr_link.data = iface->addr.s_addr;
				rtr_link.type = LINK_TYPE_POINTTOPOINT;
				rtr_link.metric = htons(iface->metric);
				num_links++;
				if (buf_add(buf, &rtr_link, sizeof(rtr_link)))
					fatalx("orig_rtr_lsa: buf_add failed");
			}
			if (iface->state & IF_STA_POINTTOPOINT) {
				log_debug("orig_rtr_lsa: stub net, "
				    "interface %s", iface->name);
				bzero(&rtr_link, sizeof(rtr_link));
				if (nbr) {
					rtr_link.id = nbr->addr.s_addr;
					rtr_link.data = 0xffffffff;
				} else {
					rtr_link.id = iface->addr.s_addr;
					rtr_link.data = iface->mask.s_addr;
				}
				rtr_link.metric = htons(iface->metric);
				rtr_link.type = LINK_TYPE_STUB_NET;
				num_links++;
				if (buf_add(buf, &rtr_link, sizeof(rtr_link)))
					fatalx("orig_rtr_lsa: buf_add failed");
			}
			continue;
		case IF_TYPE_BROADCAST:
		case IF_TYPE_NBMA:
			if ((iface->state & IF_STA_MULTI)) {
				if (iface->dr == iface->self) {
					LIST_FOREACH(nbr, &iface->nbr_list,
					    entry)
						if (nbr != iface->self &&
						    nbr->state & NBR_STA_FULL)
							break;
				} else
					nbr = iface->dr;

				if (nbr && nbr->state & NBR_STA_FULL) {
					log_debug("orig_rtr_lsa: transit net, "
					    "interface %s", iface->name);

					rtr_link.id = iface->dr->addr.s_addr;
					rtr_link.data = iface->addr.s_addr;
					rtr_link.type = LINK_TYPE_TRANSIT_NET;
					break;
				}
			}

			if ((iface->flags & IFF_UP) == 0 ||
			    iface->linkstate == LINK_STATE_DOWN ||
			    (iface->linkstate != LINK_STATE_UP &&
			    iface->media_type == IFT_CARP))
				continue;
			
			log_debug("orig_rtr_lsa: stub net, "
			    "interface %s", iface->name);

			rtr_link.id =
			    iface->addr.s_addr & iface->mask.s_addr;
			rtr_link.data = iface->mask.s_addr;
			rtr_link.type = LINK_TYPE_STUB_NET;
			break;
		case IF_TYPE_VIRTUALLINK:
			LIST_FOREACH(nbr, &iface->nbr_list, entry) {
				if (nbr != iface->self &&
				    nbr->state & NBR_STA_FULL)
					break;
			}
			if (nbr) {
				rtr_link.id = nbr->id.s_addr;
				rtr_link.data = iface->addr.s_addr;
				rtr_link.type = LINK_TYPE_VIRTUAL;
				rtr_link.metric = htons(iface->metric);
				num_links++;
				virtual = 1;
				if (buf_add(buf, &rtr_link, sizeof(rtr_link)))
					fatalx("orig_rtr_lsa: buf_add failed");

				log_debug("orig_rtr_lsa: virtual link, "
				    "interface %s", iface->name);
			}
			continue;
		case IF_TYPE_POINTOMULTIPOINT:
			log_debug("orig_rtr_lsa: stub net, "
			    "interface %s", iface->name);
			rtr_link.id = iface->addr.s_addr;
			rtr_link.data = 0xffffffff;
			rtr_link.type = LINK_TYPE_STUB_NET;
			num_links++;
			if (buf_add(buf, &rtr_link, sizeof(rtr_link)))
				fatalx("orig_rtr_lsa: buf_add failed");

			LIST_FOREACH(nbr, &iface->nbr_list, entry) {
				if (nbr != iface->self &&
				    nbr->state & NBR_STA_FULL) {
					bzero(&rtr_link, sizeof(rtr_link));
					log_debug("orig_rtr_lsa: "
					    "point-to-point, interface %s",
					    iface->name);
					rtr_link.id = nbr->addr.s_addr;
					rtr_link.data = iface->addr.s_addr;
					rtr_link.type = LINK_TYPE_POINTTOPOINT;
					rtr_link.metric = htons(iface->metric);
					num_links++;
					if (buf_add(buf, &rtr_link,
					    sizeof(rtr_link)))
						fatalx("orig_rtr_lsa: "
						    "buf_add failed");
				}
			}
			continue;
		default:
			fatalx("orig_rtr_lsa: unknown interface type");
		}

		rtr_link.num_tos = 0;
		rtr_link.metric = htons(iface->metric);
		num_links++;
		if (buf_add(buf, &rtr_link, sizeof(rtr_link)))
			fatalx("orig_rtr_lsa: buf_add failed");
	}

	/* LSA router header */
	lsa_rtr.flags = 0;
	/*
	 * Set the E bit as soon as an as-ext lsa may be redistributed, only
	 * setting it in case we redistribute something is not worth the fuss.
	 */
	if (oeconf->redistribute_flags && (oeconf->options & OSPF_OPTION_E))
		lsa_rtr.flags |= OSPF_RTR_E;

	border = area_border_router(oeconf);

	if (border != oeconf->border) {
		oeconf->border = border;
		orig_rtr_lsa_all(area);
	}

	if (oeconf->border)
		lsa_rtr.flags |= OSPF_RTR_B;
	if (virtual)
		lsa_rtr.flags |= OSPF_RTR_V;

	/* TODO set V flag if a active virtual link ends here and the
	 * area is the tranist area for this link. */
	lsa_rtr.dummy = 0;
	lsa_rtr.nlinks = htons(num_links);
	memcpy(buf_seek(buf, sizeof(lsa_hdr), sizeof(lsa_rtr)),
	    &lsa_rtr, sizeof(lsa_rtr));

	/* LSA header */
	lsa_hdr.age = htons(DEFAULT_AGE);
	lsa_hdr.opts = oeconf->options;		/* XXX */
	lsa_hdr.type = LSA_TYPE_ROUTER;
	lsa_hdr.ls_id = oeconf->rtr_id.s_addr;
	lsa_hdr.adv_rtr = oeconf->rtr_id.s_addr;
	lsa_hdr.seq_num = htonl(INIT_SEQ_NUM);
	lsa_hdr.len = htons(buf->wpos);
	lsa_hdr.ls_chksum = 0;		/* updated later */
	memcpy(buf_seek(buf, 0, sizeof(lsa_hdr)), &lsa_hdr, sizeof(lsa_hdr));

	chksum = htons(iso_cksum(buf->buf, buf->wpos, LS_CKSUM_OFFSET));
	memcpy(buf_seek(buf, LS_CKSUM_OFFSET, sizeof(chksum)),
	    &chksum, sizeof(chksum));

	if (self)
		imsg_compose(ibuf_rde, IMSG_LS_UPD, self->peerid, 0,
		    buf->buf, buf->wpos);
	else
		log_warnx("orig_rtr_lsa: empty area %s",
		    inet_ntoa(area->id));

	buf_free(buf);
}

void
orig_net_lsa(struct iface *iface)
{
	struct lsa_hdr		 lsa_hdr;
	struct nbr		*nbr;
	struct buf		*buf;
	int			 num_rtr = 0;
	u_int16_t		 chksum;

	/* XXX READ_BUF_SIZE */
	if ((buf = buf_dynamic(sizeof(lsa_hdr), READ_BUF_SIZE)) == NULL)
		fatal("orig_net_lsa");

	/* reserve space for LSA header and LSA Router header */
	if (buf_reserve(buf, sizeof(lsa_hdr)) == NULL)
		fatal("orig_net_lsa: buf_reserve failed");

	/* LSA net mask and then all fully adjacent routers */
	if (buf_add(buf, &iface->mask, sizeof(iface->mask)))
		fatal("orig_net_lsa: buf_add failed");

	/* fully adjacent neighbors + self */
	LIST_FOREACH(nbr, &iface->nbr_list, entry)
		if (nbr->state & NBR_STA_FULL) {
			if (buf_add(buf, &nbr->id, sizeof(nbr->id)))
				fatal("orig_net_lsa: buf_add failed");
			num_rtr++;
		}

	if (num_rtr == 1) {
		/* non transit net therefor no need to generate a net lsa */
		buf_free(buf);
		return;
	}

	/* LSA header */
	if (iface->state & IF_STA_DR)
		lsa_hdr.age = htons(DEFAULT_AGE);
	else
		lsa_hdr.age = htons(MAX_AGE);

	lsa_hdr.opts = oeconf->options;		/* XXX */
	lsa_hdr.type = LSA_TYPE_NETWORK;
	lsa_hdr.ls_id = iface->addr.s_addr;
	lsa_hdr.adv_rtr = oeconf->rtr_id.s_addr;
	lsa_hdr.seq_num = htonl(INIT_SEQ_NUM);
	lsa_hdr.len = htons(buf->wpos);
	lsa_hdr.ls_chksum = 0;		/* updated later */
	memcpy(buf_seek(buf, 0, sizeof(lsa_hdr)), &lsa_hdr, sizeof(lsa_hdr));

	chksum = htons(iso_cksum(buf->buf, buf->wpos, LS_CKSUM_OFFSET));
	memcpy(buf_seek(buf, LS_CKSUM_OFFSET, sizeof(chksum)),
	    &chksum, sizeof(chksum));

	imsg_compose(ibuf_rde, IMSG_LS_UPD, iface->self->peerid, 0,
	    buf->buf, buf->wpos);

	buf_free(buf);
}

u_int32_t
ospfe_router_id(void)
{
	return (oeconf->rtr_id.s_addr);
}

void
ospfe_iface_ctl(struct ctl_conn *c, unsigned int idx)
{
	struct area		*area;
	struct iface		*iface;
	struct ctl_iface	*ictl;

	LIST_FOREACH(area, &oeconf->area_list, entry)
		LIST_FOREACH(iface, &area->iface_list, entry)
			if (idx == 0 || idx == iface->ifindex) {
				ictl = if_to_ctl(iface);
				imsg_compose(&c->ibuf, IMSG_CTL_SHOW_INTERFACE,
				    0, 0, ictl, sizeof(struct ctl_iface));
			}
}

void
ospfe_nbr_ctl(struct ctl_conn *c)
{
	struct area	*area;
	struct iface	*iface;
	struct nbr	*nbr;
	struct ctl_nbr	*nctl;

	LIST_FOREACH(area, &oeconf->area_list, entry)
		LIST_FOREACH(iface, &area->iface_list, entry)
			LIST_FOREACH(nbr, &iface->nbr_list, entry) {
				if (iface->self != nbr) {
					nctl = nbr_to_ctl(nbr);
					imsg_compose(&c->ibuf,
					    IMSG_CTL_SHOW_NBR, 0, 0, nctl,
					    sizeof(struct ctl_nbr));
				}
			}

	imsg_compose(&c->ibuf, IMSG_CTL_END, 0, 0, NULL, 0);
}
