/*	$OpenBSD: src/sys/net/if_pfsync.c,v 1.80 2007/06/21 11:55:54 henning Exp $	*/

/*
 * Copyright (c) 2002 Michael Shalayeff
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/timeout.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/bpf.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/tcp.h>
#include <netinet/tcp_seq.h>

#ifdef	INET
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#endif

#ifdef INET6
#include <netinet6/nd6.h>
#endif /* INET6 */

#include "carp.h"
#if NCARP > 0
#include <netinet/ip_carp.h>
#endif

#include <net/pfvar.h>
#include <net/if_pfsync.h>

#include "bpfilter.h"
#include "pfsync.h"

#define PFSYNC_MINMTU	\
    (sizeof(struct pfsync_header) + sizeof(struct pf_state))

#ifdef PFSYNCDEBUG
#define DPRINTF(x)    do { if (pfsyncdebug) printf x ; } while (0)
int pfsyncdebug;
#else
#define DPRINTF(x)
#endif

struct pfsync_softc	*pfsyncif = NULL;
struct pfsyncstats	 pfsyncstats;

void	pfsyncattach(int);
int	pfsync_clone_create(struct if_clone *, int);
int	pfsync_clone_destroy(struct ifnet *);
void	pfsync_setmtu(struct pfsync_softc *, int);
int	pfsync_alloc_scrub_memory(struct pfsync_state_peer *,
	    struct pf_state_peer *);
int	pfsync_insert_net_state(struct pfsync_state *, u_int8_t);
void	pfsync_update_net_tdb(struct pfsync_tdb *);
int	pfsyncoutput(struct ifnet *, struct mbuf *, struct sockaddr *,
	    struct rtentry *);
int	pfsyncioctl(struct ifnet *, u_long, caddr_t);
void	pfsyncstart(struct ifnet *);

struct mbuf *pfsync_get_mbuf(struct pfsync_softc *, u_int8_t, void **);
int	pfsync_request_update(struct pfsync_state_upd *, struct in_addr *);
int	pfsync_sendout(struct pfsync_softc *);
int	pfsync_tdb_sendout(struct pfsync_softc *);
int	pfsync_sendout_mbuf(struct pfsync_softc *, struct mbuf *);
void	pfsync_timeout(void *);
void	pfsync_tdb_timeout(void *);
void	pfsync_send_bus(struct pfsync_softc *, u_int8_t);
void	pfsync_bulk_update(void *);
void	pfsync_bulkfail(void *);

int	pfsync_sync_ok;

struct if_clone	pfsync_cloner =
    IF_CLONE_INITIALIZER("pfsync", pfsync_clone_create, pfsync_clone_destroy);

void
pfsyncattach(int npfsync)
{
	if_clone_attach(&pfsync_cloner);
}
int
pfsync_clone_create(struct if_clone *ifc, int unit)
{
	struct ifnet *ifp;

	if (unit != 0)
		return (EINVAL);

	pfsync_sync_ok = 1;
	if ((pfsyncif = malloc(sizeof(*pfsyncif), M_DEVBUF, M_NOWAIT)) == NULL)
		return (ENOMEM);
	bzero(pfsyncif, sizeof(*pfsyncif));
	pfsyncif->sc_mbuf = NULL;
	pfsyncif->sc_mbuf_net = NULL;
	pfsyncif->sc_mbuf_tdb = NULL;
	pfsyncif->sc_statep.s = NULL;
	pfsyncif->sc_statep_net.s = NULL;
	pfsyncif->sc_statep_tdb.t = NULL;
	pfsyncif->sc_maxupdates = 128;
	pfsyncif->sc_sync_peer.s_addr = INADDR_PFSYNC_GROUP;
	pfsyncif->sc_sendaddr.s_addr = INADDR_PFSYNC_GROUP;
	pfsyncif->sc_ureq_received = 0;
	pfsyncif->sc_ureq_sent = 0;
	pfsyncif->sc_bulk_send_next = NULL;
	pfsyncif->sc_bulk_terminator = NULL;
	ifp = &pfsyncif->sc_if;
	snprintf(ifp->if_xname, sizeof ifp->if_xname, "pfsync%d", unit);
	ifp->if_softc = pfsyncif;
	ifp->if_ioctl = pfsyncioctl;
	ifp->if_output = pfsyncoutput;
	ifp->if_start = pfsyncstart;
	ifp->if_type = IFT_PFSYNC;
	ifp->if_snd.ifq_maxlen = ifqmaxlen;
	ifp->if_hdrlen = PFSYNC_HDRLEN;
	pfsync_setmtu(pfsyncif, ETHERMTU);
	timeout_set(&pfsyncif->sc_tmo, pfsync_timeout, pfsyncif);
	timeout_set(&pfsyncif->sc_tdb_tmo, pfsync_tdb_timeout, pfsyncif);
	timeout_set(&pfsyncif->sc_bulk_tmo, pfsync_bulk_update, pfsyncif);
	timeout_set(&pfsyncif->sc_bulkfail_tmo, pfsync_bulkfail, pfsyncif);
	if_attach(ifp);
	if_alloc_sadl(ifp);

#if NCARP > 0
	if_addgroup(ifp, "carp");
#endif

#if NBPFILTER > 0
	bpfattach(&pfsyncif->sc_if.if_bpf, ifp, DLT_PFSYNC, PFSYNC_HDRLEN);
#endif

	return (0);
}

int
pfsync_clone_destroy(struct ifnet *ifp)
{
#if NBPFILTER > 0
	bpfdetach(ifp);
#endif
	if_detach(ifp);
	free(pfsyncif, M_DEVBUF);
	pfsyncif = NULL;
	return (0);
}

/*
 * Start output on the pfsync interface.
 */
void
pfsyncstart(struct ifnet *ifp)
{
	struct mbuf *m;
	int s;

	for (;;) {
		s = splnet();
		IF_DROP(&ifp->if_snd);
		IF_DEQUEUE(&ifp->if_snd, m);
		splx(s);

		if (m == NULL)
			return;
		else
			m_freem(m);
	}
}

int
pfsync_alloc_scrub_memory(struct pfsync_state_peer *s,
    struct pf_state_peer *d)
{
	if (s->scrub.scrub_flag && d->scrub == NULL) {
		d->scrub = pool_get(&pf_state_scrub_pl, PR_NOWAIT);
		if (d->scrub == NULL)
			return (ENOMEM);
		bzero(d->scrub, sizeof(*d->scrub));
	}

	return (0);
}

int
pfsync_insert_net_state(struct pfsync_state *sp, u_int8_t chksum_flag)
{
	struct pf_state	*st = NULL;
	struct pf_state_key *sk = NULL;
	struct pf_rule *r = NULL;
	struct pfi_kif	*kif;

	if (sp->creatorid == 0 && pf_status.debug >= PF_DEBUG_MISC) {
		printf("pfsync_insert_net_state: invalid creator id:"
		    " %08x\n", ntohl(sp->creatorid));
		return (EINVAL);
	}

	kif = pfi_kif_get(sp->ifname);
	if (kif == NULL) {
		if (pf_status.debug >= PF_DEBUG_MISC)
			printf("pfsync_insert_net_state: "
			    "unknown interface: %s\n", sp->ifname);
		/* skip this state */
		return (0);
	}

	/*
	 * If the ruleset checksums match, it's safe to associate the state
	 * with the rule of that number.
	 */
	if (sp->rule != htonl(-1) && sp->anchor == htonl(-1) && chksum_flag)
		r = pf_main_ruleset.rules[
		    PF_RULESET_FILTER].active.ptr_array[ntohl(sp->rule)];
	else
		r = &pf_default_rule;

	if (!r->max_states || r->states < r->max_states)
		st = pool_get(&pf_state_pl, PR_NOWAIT);
	if (st == NULL) {
		pfi_kif_unref(kif, PFI_KIF_REF_NONE);
		return (ENOMEM);
	}
	bzero(st, sizeof(*st));

	if ((sk = pf_alloc_state_key(st)) == NULL) {
		pool_put(&pf_state_pl, st);
		pfi_kif_unref(kif, PFI_KIF_REF_NONE);
		return (ENOMEM);
	}

	/* allocate memory for scrub info */
	if (pfsync_alloc_scrub_memory(&sp->src, &st->src) ||
	    pfsync_alloc_scrub_memory(&sp->dst, &st->dst)) {
		pfi_kif_unref(kif, PFI_KIF_REF_NONE);
		if (st->src.scrub)
			pool_put(&pf_state_scrub_pl, st->src.scrub);
		pool_put(&pf_state_pl, st);
		pool_put(&pf_state_key_pl, sk);
		return (ENOMEM);
	}

	st->rule.ptr = r;
	/* XXX get pointers to nat_rule and anchor */

	/* XXX when we have nat_rule/anchors, use STATE_INC_COUNTERS */
	r->states++;

	/* fill in the rest of the state entry */
	pf_state_host_ntoh(&sp->lan, &sk->lan);
	pf_state_host_ntoh(&sp->gwy, &sk->gwy);
	pf_state_host_ntoh(&sp->ext, &sk->ext);

	pf_state_peer_ntoh(&sp->src, &st->src);
	pf_state_peer_ntoh(&sp->dst, &st->dst);

	bcopy(&sp->rt_addr, &st->rt_addr, sizeof(st->rt_addr));
	st->creation = time_second - ntohl(sp->creation);
	st->expire = ntohl(sp->expire) + time_second;

	sk->af = sp->af;
	sk->proto = sp->proto;
	sk->direction = sp->direction;
	st->log = sp->log;
	st->timeout = sp->timeout;
	st->allow_opts = sp->allow_opts;

	bcopy(sp->id, &st->id, sizeof(st->id));
	st->creatorid = sp->creatorid;
	st->sync_flags = PFSTATE_FROMSYNC;

	if (pf_insert_state(kif, st)) {
		pfi_kif_unref(kif, PFI_KIF_REF_NONE);
		/* XXX when we have nat_rule/anchors, use STATE_DEC_COUNTERS */
		r->states--;
		if (st->dst.scrub)
			pool_put(&pf_state_scrub_pl, st->dst.scrub);
		if (st->src.scrub)
			pool_put(&pf_state_scrub_pl, st->src.scrub);
		pool_put(&pf_state_pl, st);
		return (EINVAL);
	}

	return (0);
}

void
pfsync_input(struct mbuf *m, ...)
{
	struct ip *ip = mtod(m, struct ip *);
	struct pfsync_header *ph;
	struct pfsync_softc *sc = pfsyncif;
	struct pf_state *st;
	struct pf_state_key *sk;
	struct pf_state_cmp id_key;
	struct pfsync_state *sp;
	struct pfsync_state_upd *up;
	struct pfsync_state_del *dp;
	struct pfsync_state_clr *cp;
	struct pfsync_state_upd_req *rup;
	struct pfsync_state_bus *bus;
#ifdef IPSEC
	struct pfsync_tdb *pt;
#endif
	struct in_addr src;
	struct mbuf *mp;
	int iplen, action, error, i, s, count, offp, sfail, stale = 0;
	u_int8_t chksum_flag = 0;

	pfsyncstats.pfsyncs_ipackets++;

	/* verify that we have a sync interface configured */
	if (!sc || !sc->sc_sync_ifp || !pf_status.running)
		goto done;

	/* verify that the packet came in on the right interface */
	if (sc->sc_sync_ifp != m->m_pkthdr.rcvif) {
		pfsyncstats.pfsyncs_badif++;
		goto done;
	}

	/* verify that the IP TTL is 255.  */
	if (ip->ip_ttl != PFSYNC_DFLTTL) {
		pfsyncstats.pfsyncs_badttl++;
		goto done;
	}

	iplen = ip->ip_hl << 2;

	if (m->m_pkthdr.len < iplen + sizeof(*ph)) {
		pfsyncstats.pfsyncs_hdrops++;
		goto done;
	}

	if (iplen + sizeof(*ph) > m->m_len) {
		if ((m = m_pullup(m, iplen + sizeof(*ph))) == NULL) {
			pfsyncstats.pfsyncs_hdrops++;
			goto done;
		}
		ip = mtod(m, struct ip *);
	}
	ph = (struct pfsync_header *)((char *)ip + iplen);

	/* verify the version */
	if (ph->version != PFSYNC_VERSION) {
		pfsyncstats.pfsyncs_badver++;
		goto done;
	}

	action = ph->action;
	count = ph->count;

	/* make sure it's a valid action code */
	if (action >= PFSYNC_ACT_MAX) {
		pfsyncstats.pfsyncs_badact++;
		goto done;
	}

	/* Cheaper to grab this now than having to mess with mbufs later */
	src = ip->ip_src;

	if (!bcmp(&ph->pf_chksum, &pf_status.pf_chksum, PF_MD5_DIGEST_LENGTH))
		chksum_flag++;

	switch (action) {
	case PFSYNC_ACT_CLR: {
		struct pf_state *nexts;
		struct pf_state_key *nextsk;
		struct pfi_kif *kif;
		u_int32_t creatorid;
		if ((mp = m_pulldown(m, iplen + sizeof(*ph),
		    sizeof(*cp), &offp)) == NULL) {
			pfsyncstats.pfsyncs_badlen++;
			return;
		}
		cp = (struct pfsync_state_clr *)(mp->m_data + offp);
		creatorid = cp->creatorid;

		s = splsoftnet();
		if (cp->ifname[0] == '\0') {
			for (st = RB_MIN(pf_state_tree_id, &tree_id);
			    st; st = nexts) {
				nexts = RB_NEXT(pf_state_tree_id, &tree_id, st);
				if (st->creatorid == creatorid) {
					st->sync_flags |= PFSTATE_FROMSYNC;
					pf_unlink_state(st);
				}
			}
		} else {
			if ((kif = pfi_kif_get(cp->ifname)) == NULL) {
				splx(s);
				return;
			}
			for (sk = RB_MIN(pf_state_tree_lan_ext,
			    &pfi_all->pfik_lan_ext); sk; sk = nextsk) {
				nextsk = RB_NEXT(pf_state_tree_lan_ext,
				    &pfi_all->pfik_lan_ext, sk);
				TAILQ_FOREACH(st, &sk->states, next) {
					if (st->creatorid == creatorid) {
						st->sync_flags |=
						    PFSTATE_FROMSYNC;
						pf_unlink_state(st);
					}
				}
			}
		}
		splx(s);

		break;
	}
	case PFSYNC_ACT_INS:
		if ((mp = m_pulldown(m, iplen + sizeof(*ph),
		    count * sizeof(*sp), &offp)) == NULL) {
			pfsyncstats.pfsyncs_badlen++;
			return;
		}

		s = splsoftnet();
		for (i = 0, sp = (struct pfsync_state *)(mp->m_data + offp);
		    i < count; i++, sp++) {
			/* check for invalid values */
			if (sp->timeout >= PFTM_MAX ||
			    sp->src.state > PF_TCPS_PROXY_DST ||
			    sp->dst.state > PF_TCPS_PROXY_DST ||
			    sp->direction > PF_OUT ||
			    (sp->af != AF_INET && sp->af != AF_INET6)) {
				if (pf_status.debug >= PF_DEBUG_MISC)
					printf("pfsync_insert: PFSYNC_ACT_INS: "
					    "invalid value\n");
				pfsyncstats.pfsyncs_badstate++;
				continue;
			}

			if ((error = pfsync_insert_net_state(sp,
			    chksum_flag))) {
				if (error == ENOMEM) {
					splx(s);
					goto done;
				}
				continue;
			}
		}
		splx(s);
		break;
	case PFSYNC_ACT_UPD:
		if ((mp = m_pulldown(m, iplen + sizeof(*ph),
		    count * sizeof(*sp), &offp)) == NULL) {
			pfsyncstats.pfsyncs_badlen++;
			return;
		}

		s = splsoftnet();
		for (i = 0, sp = (struct pfsync_state *)(mp->m_data + offp);
		    i < count; i++, sp++) {
			int flags = PFSYNC_FLAG_STALE;

			/* check for invalid values */
			if (sp->timeout >= PFTM_MAX ||
			    sp->src.state > PF_TCPS_PROXY_DST ||
			    sp->dst.state > PF_TCPS_PROXY_DST) {
				if (pf_status.debug >= PF_DEBUG_MISC)
					printf("pfsync_insert: PFSYNC_ACT_UPD: "
					    "invalid value\n");
				pfsyncstats.pfsyncs_badstate++;
				continue;
			}

			bcopy(sp->id, &id_key.id, sizeof(id_key.id));
			id_key.creatorid = sp->creatorid;

			st = pf_find_state_byid(&id_key);
			if (st == NULL) {
				/* insert the update */
				if (pfsync_insert_net_state(sp, chksum_flag))
					pfsyncstats.pfsyncs_badstate++;
				continue;
			}
			sk = st->state_key;
			sfail = 0;
			if (sk->proto == IPPROTO_TCP) {
				/*
				 * The state should never go backwards except
				 * for syn-proxy states.  Neither should the
				 * sequence window slide backwards.
				 */
				if (st->src.state > sp->src.state &&
				    (st->src.state < PF_TCPS_PROXY_SRC ||
				    sp->src.state >= PF_TCPS_PROXY_SRC))
					sfail = 1;
				else if (SEQ_GT(st->src.seqlo,
				    ntohl(sp->src.seqlo)))
					sfail = 3;
				else if (st->dst.state > sp->dst.state) {
					/* There might still be useful
					 * information about the src state here,
					 * so import that part of the update,
					 * then "fail" so we send the updated
					 * state back to the peer who is missing
					 * our what we know. */
					pf_state_peer_ntoh(&sp->src, &st->src);
					/* XXX do anything with timeouts? */
					sfail = 7;
					flags = 0;
				} else if (st->dst.state >= TCPS_SYN_SENT &&
				    SEQ_GT(st->dst.seqlo, ntohl(sp->dst.seqlo)))
					sfail = 4;
			} else {
				/*
				 * Non-TCP protocol state machine always go
				 * forwards
				 */
				if (st->src.state > sp->src.state)
					sfail = 5;
				else if (st->dst.state > sp->dst.state)
					sfail = 6;
			}
			if (sfail) {
				if (pf_status.debug >= PF_DEBUG_MISC)
					printf("pfsync: %s stale update "
					    "(%d) id: %016llx "
					    "creatorid: %08x\n",
					    (sfail < 7 ?  "ignoring"
					     : "partial"), sfail,
					    betoh64(st->id),
					    ntohl(st->creatorid));
				pfsyncstats.pfsyncs_badstate++;

				if (!(sp->sync_flags & PFSTATE_STALE)) {
					/* we have a better state, send it */
					if (sc->sc_mbuf != NULL && !stale)
						pfsync_sendout(sc);
					stale++;
					if (!st->sync_flags)
						pfsync_pack_state(
						    PFSYNC_ACT_UPD, st, flags);
				}
				continue;
			}
	    		pfsync_alloc_scrub_memory(&sp->dst, &st->dst);
			pf_state_peer_ntoh(&sp->src, &st->src);
			pf_state_peer_ntoh(&sp->dst, &st->dst);
			st->expire = ntohl(sp->expire) + time_second;
			st->timeout = sp->timeout;
		}
		if (stale && sc->sc_mbuf != NULL)
			pfsync_sendout(sc);
		splx(s);
		break;
	/*
	 * It's not strictly necessary for us to support the "uncompressed"
	 * delete action, but it's relatively simple and maintains consistency.
	 */
	case PFSYNC_ACT_DEL:
		if ((mp = m_pulldown(m, iplen + sizeof(*ph),
		    count * sizeof(*sp), &offp)) == NULL) {
			pfsyncstats.pfsyncs_badlen++;
			return;
		}

		s = splsoftnet();
		for (i = 0, sp = (struct pfsync_state *)(mp->m_data + offp);
		    i < count; i++, sp++) {
			bcopy(sp->id, &id_key.id, sizeof(id_key.id));
			id_key.creatorid = sp->creatorid;

			st = pf_find_state_byid(&id_key);
			if (st == NULL) {
				pfsyncstats.pfsyncs_badstate++;
				continue;
			}
			st->sync_flags |= PFSTATE_FROMSYNC;
			pf_unlink_state(st);
		}
		splx(s);
		break;
	case PFSYNC_ACT_UPD_C: {
		int update_requested = 0;

		if ((mp = m_pulldown(m, iplen + sizeof(*ph),
		    count * sizeof(*up), &offp)) == NULL) {
			pfsyncstats.pfsyncs_badlen++;
			return;
		}

		s = splsoftnet();
		for (i = 0, up = (struct pfsync_state_upd *)(mp->m_data + offp);
		    i < count; i++, up++) {
			/* check for invalid values */
			if (up->timeout >= PFTM_MAX ||
			    up->src.state > PF_TCPS_PROXY_DST ||
			    up->dst.state > PF_TCPS_PROXY_DST) {
				if (pf_status.debug >= PF_DEBUG_MISC)
					printf("pfsync_insert: "
					    "PFSYNC_ACT_UPD_C: "
					    "invalid value\n");
				pfsyncstats.pfsyncs_badstate++;
				continue;
			}

			bcopy(up->id, &id_key.id, sizeof(id_key.id));
			id_key.creatorid = up->creatorid;

			st = pf_find_state_byid(&id_key);
			if (st == NULL) {
				/* We don't have this state. Ask for it. */
				error = pfsync_request_update(up, &src);
				if (error == ENOMEM) {
					splx(s);
					goto done;
				}
				update_requested = 1;
				pfsyncstats.pfsyncs_badstate++;
				continue;
			}
			sk = st->state_key;
			sfail = 0;
			if (sk->proto == IPPROTO_TCP) {
				/*
				 * The state should never go backwards except
				 * for syn-proxy states.  Neither should the
				 * sequence window slide backwards.
				 */
				if (st->src.state > up->src.state &&
				    (st->src.state < PF_TCPS_PROXY_SRC ||
				    up->src.state >= PF_TCPS_PROXY_SRC))
					sfail = 1;
				else if (st->dst.state > up->dst.state)
					sfail = 2;
				else if (SEQ_GT(st->src.seqlo,
				    ntohl(up->src.seqlo)))
					sfail = 3;
				else if (st->dst.state >= TCPS_SYN_SENT &&
				    SEQ_GT(st->dst.seqlo, ntohl(up->dst.seqlo)))
					sfail = 4;
			} else {
				/*
				 * Non-TCP protocol state machine always go
				 * forwards
				 */
				if (st->src.state > up->src.state)
					sfail = 5;
				else if (st->dst.state > up->dst.state)
					sfail = 6;
			}
			if (sfail) {
				if (pf_status.debug >= PF_DEBUG_MISC)
					printf("pfsync: ignoring stale update "
					    "(%d) id: %016llx "
					    "creatorid: %08x\n", sfail,
					    betoh64(st->id),
					    ntohl(st->creatorid));
				pfsyncstats.pfsyncs_badstate++;

				/* we have a better state, send it out */
				if ((!stale || update_requested) &&
				    sc->sc_mbuf != NULL) {
					pfsync_sendout(sc);
					update_requested = 0;
				}
				stale++;
				if (!st->sync_flags)
					pfsync_pack_state(PFSYNC_ACT_UPD, st,
					    PFSYNC_FLAG_STALE);
				continue;
			}
	    		pfsync_alloc_scrub_memory(&up->dst, &st->dst);
			pf_state_peer_ntoh(&up->src, &st->src);
			pf_state_peer_ntoh(&up->dst, &st->dst);
			st->expire = ntohl(up->expire) + time_second;
			st->timeout = up->timeout;
		}
		if ((update_requested || stale) && sc->sc_mbuf)
			pfsync_sendout(sc);
		splx(s);
		break;
	}
	case PFSYNC_ACT_DEL_C:
		if ((mp = m_pulldown(m, iplen + sizeof(*ph),
		    count * sizeof(*dp), &offp)) == NULL) {
			pfsyncstats.pfsyncs_badlen++;
			return;
		}

		s = splsoftnet();
		for (i = 0, dp = (struct pfsync_state_del *)(mp->m_data + offp);
		    i < count; i++, dp++) {
			bcopy(dp->id, &id_key.id, sizeof(id_key.id));
			id_key.creatorid = dp->creatorid;

			st = pf_find_state_byid(&id_key);
			if (st == NULL) {
				pfsyncstats.pfsyncs_badstate++;
				continue;
			}
			st->sync_flags |= PFSTATE_FROMSYNC;
			pf_unlink_state(st);
		}
		splx(s);
		break;
	case PFSYNC_ACT_INS_F:
	case PFSYNC_ACT_DEL_F:
		/* not implemented */
		break;
	case PFSYNC_ACT_UREQ:
		if ((mp = m_pulldown(m, iplen + sizeof(*ph),
		    count * sizeof(*rup), &offp)) == NULL) {
			pfsyncstats.pfsyncs_badlen++;
			return;
		}

		s = splsoftnet();
		if (sc->sc_mbuf != NULL)
			pfsync_sendout(sc);
		for (i = 0,
		    rup = (struct pfsync_state_upd_req *)(mp->m_data + offp);
		    i < count; i++, rup++) {
			bcopy(rup->id, &id_key.id, sizeof(id_key.id));
			id_key.creatorid = rup->creatorid;

			if (id_key.id == 0 && id_key.creatorid == 0) {
				sc->sc_ureq_received = time_uptime;
				if (sc->sc_bulk_send_next == NULL)
					sc->sc_bulk_send_next =
					    TAILQ_FIRST(&state_list);
				sc->sc_bulk_terminator = sc->sc_bulk_send_next;
				if (pf_status.debug >= PF_DEBUG_MISC)
					printf("pfsync: received "
					    "bulk update request\n");
				pfsync_send_bus(sc, PFSYNC_BUS_START);
				timeout_add(&sc->sc_bulk_tmo, 1 * hz);
			} else {
				st = pf_find_state_byid(&id_key);
				if (st == NULL) {
					pfsyncstats.pfsyncs_badstate++;
					continue;
				}
				if (!st->sync_flags)
					pfsync_pack_state(PFSYNC_ACT_UPD,
					    st, 0);
			}
		}
		if (sc->sc_mbuf != NULL)
			pfsync_sendout(sc);
		splx(s);
		break;
	case PFSYNC_ACT_BUS:
		/* If we're not waiting for a bulk update, who cares. */
		if (sc->sc_ureq_sent == 0)
			break;

		if ((mp = m_pulldown(m, iplen + sizeof(*ph),
		    sizeof(*bus), &offp)) == NULL) {
			pfsyncstats.pfsyncs_badlen++;
			return;
		}
		bus = (struct pfsync_state_bus *)(mp->m_data + offp);
		switch (bus->status) {
		case PFSYNC_BUS_START:
			timeout_add(&sc->sc_bulkfail_tmo,
			    pf_pool_limits[PF_LIMIT_STATES].limit /
			    (PFSYNC_BULKPACKETS * sc->sc_maxcount));
			if (pf_status.debug >= PF_DEBUG_MISC)
				printf("pfsync: received bulk "
				    "update start\n");
			break;
		case PFSYNC_BUS_END:
			if (time_uptime - ntohl(bus->endtime) >=
			    sc->sc_ureq_sent) {
				/* that's it, we're happy */
				sc->sc_ureq_sent = 0;
				sc->sc_bulk_tries = 0;
				timeout_del(&sc->sc_bulkfail_tmo);
#if NCARP > 0
				if (!pfsync_sync_ok)
					carp_group_demote_adj(&sc->sc_if, -1);
#endif
				pfsync_sync_ok = 1;
				if (pf_status.debug >= PF_DEBUG_MISC)
					printf("pfsync: received valid "
					    "bulk update end\n");
			} else {
				if (pf_status.debug >= PF_DEBUG_MISC)
					printf("pfsync: received invalid "
					    "bulk update end: bad timestamp\n");
			}
			break;
		}
		break;
#ifdef IPSEC
	case PFSYNC_ACT_TDB_UPD:
		if ((mp = m_pulldown(m, iplen + sizeof(*ph),
		    count * sizeof(*pt), &offp)) == NULL) {
			pfsyncstats.pfsyncs_badlen++;
			return;
		}
		s = splsoftnet();
		for (i = 0, pt = (struct pfsync_tdb *)(mp->m_data + offp);
		    i < count; i++, pt++)
			pfsync_update_net_tdb(pt);
		splx(s);
		break;
#endif
	}

done:
	if (m)
		m_freem(m);
}

int
pfsyncoutput(struct ifnet *ifp, struct mbuf *m, struct sockaddr *dst,
	struct rtentry *rt)
{
	m_freem(m);
	return (0);
}

/* ARGSUSED */
int
pfsyncioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct proc *p = curproc;
	struct pfsync_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ip_moptions *imo = &sc->sc_imo;
	struct pfsyncreq pfsyncr;
	struct ifnet    *sifp;
	int s, error;

	switch (cmd) {
	case SIOCSIFADDR:
	case SIOCAIFADDR:
	case SIOCSIFDSTADDR:
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP)
			ifp->if_flags |= IFF_RUNNING;
		else
			ifp->if_flags &= ~IFF_RUNNING;
		break;
	case SIOCSIFMTU:
		if (ifr->ifr_mtu < PFSYNC_MINMTU)
			return (EINVAL);
		if (ifr->ifr_mtu > MCLBYTES)
			ifr->ifr_mtu = MCLBYTES;
		s = splnet();
		if (ifr->ifr_mtu < ifp->if_mtu)
			pfsync_sendout(sc);
		pfsync_setmtu(sc, ifr->ifr_mtu);
		splx(s);
		break;
	case SIOCGETPFSYNC:
		bzero(&pfsyncr, sizeof(pfsyncr));
		if (sc->sc_sync_ifp)
			strlcpy(pfsyncr.pfsyncr_syncdev,
			    sc->sc_sync_ifp->if_xname, IFNAMSIZ);
		pfsyncr.pfsyncr_syncpeer = sc->sc_sync_peer;
		pfsyncr.pfsyncr_maxupdates = sc->sc_maxupdates;
		if ((error = copyout(&pfsyncr, ifr->ifr_data, sizeof(pfsyncr))))
			return (error);
		break;
	case SIOCSETPFSYNC:
		if ((error = suser(p, p->p_acflag)) != 0)
			return (error);
		if ((error = copyin(ifr->ifr_data, &pfsyncr, sizeof(pfsyncr))))
			return (error);

		if (pfsyncr.pfsyncr_syncpeer.s_addr == 0)
			sc->sc_sync_peer.s_addr = INADDR_PFSYNC_GROUP;
		else
			sc->sc_sync_peer.s_addr =
			    pfsyncr.pfsyncr_syncpeer.s_addr;

		if (pfsyncr.pfsyncr_maxupdates > 255)
			return (EINVAL);
		sc->sc_maxupdates = pfsyncr.pfsyncr_maxupdates;

		if (pfsyncr.pfsyncr_syncdev[0] == 0) {
			sc->sc_sync_ifp = NULL;
			if (sc->sc_mbuf_net != NULL) {
				/* Don't keep stale pfsync packets around. */
				s = splnet();
				m_freem(sc->sc_mbuf_net);
				sc->sc_mbuf_net = NULL;
				sc->sc_statep_net.s = NULL;
				splx(s);
			}
			if (imo->imo_num_memberships > 0) {
				in_delmulti(imo->imo_membership[--imo->imo_num_memberships]);
				imo->imo_multicast_ifp = NULL;
			}
			break;
		}

		if ((sifp = ifunit(pfsyncr.pfsyncr_syncdev)) == NULL)
			return (EINVAL);

		s = splnet();
		if (sifp->if_mtu < sc->sc_if.if_mtu ||
		    (sc->sc_sync_ifp != NULL &&
		    sifp->if_mtu < sc->sc_sync_ifp->if_mtu) ||
		    sifp->if_mtu < MCLBYTES - sizeof(struct ip))
			pfsync_sendout(sc);
		sc->sc_sync_ifp = sifp;

		pfsync_setmtu(sc, sc->sc_if.if_mtu);

		if (imo->imo_num_memberships > 0) {
			in_delmulti(imo->imo_membership[--imo->imo_num_memberships]);
			imo->imo_multicast_ifp = NULL;
		}

		if (sc->sc_sync_ifp &&
		    sc->sc_sync_peer.s_addr == INADDR_PFSYNC_GROUP) {
			struct in_addr addr;

			if (!(sc->sc_sync_ifp->if_flags & IFF_MULTICAST)) {
				sc->sc_sync_ifp = NULL;
				splx(s);
				return (EADDRNOTAVAIL);
			}

			addr.s_addr = INADDR_PFSYNC_GROUP;

			if ((imo->imo_membership[0] =
			    in_addmulti(&addr, sc->sc_sync_ifp)) == NULL) {
				sc->sc_sync_ifp = NULL;
				splx(s);
				return (ENOBUFS);
			}
			imo->imo_num_memberships++;
			imo->imo_multicast_ifp = sc->sc_sync_ifp;
			imo->imo_multicast_ttl = PFSYNC_DFLTTL;
			imo->imo_multicast_loop = 0;
		}

		if (sc->sc_sync_ifp ||
		    sc->sc_sendaddr.s_addr != INADDR_PFSYNC_GROUP) {
			/* Request a full state table update. */
			sc->sc_ureq_sent = time_uptime;
#if NCARP > 0
			if (pfsync_sync_ok)
				carp_group_demote_adj(&sc->sc_if, 1);
#endif
			pfsync_sync_ok = 0;
			if (pf_status.debug >= PF_DEBUG_MISC)
				printf("pfsync: requesting bulk update\n");
			timeout_add(&sc->sc_bulkfail_tmo, 5 * hz);
			error = pfsync_request_update(NULL, NULL);
			if (error == ENOMEM) {
				splx(s);
				return (ENOMEM);
			}
			pfsync_sendout(sc);
		}
		splx(s);

		break;

	default:
		return (ENOTTY);
	}

	return (0);
}

void
pfsync_setmtu(struct pfsync_softc *sc, int mtu_req)
{
	int mtu;

	if (sc->sc_sync_ifp && sc->sc_sync_ifp->if_mtu < mtu_req)
		mtu = sc->sc_sync_ifp->if_mtu;
	else
		mtu = mtu_req;

	sc->sc_maxcount = (mtu - sizeof(struct pfsync_header)) /
	    sizeof(struct pfsync_state);
	if (sc->sc_maxcount > 254)
	    sc->sc_maxcount = 254;
	sc->sc_if.if_mtu = sizeof(struct pfsync_header) +
	    sc->sc_maxcount * sizeof(struct pfsync_state);
}

struct mbuf *
pfsync_get_mbuf(struct pfsync_softc *sc, u_int8_t action, void **sp)
{
	struct pfsync_header *h;
	struct mbuf *m;
	int len;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		sc->sc_if.if_oerrors++;
		return (NULL);
	}

	switch (action) {
	case PFSYNC_ACT_CLR:
		len = sizeof(struct pfsync_header) +
		    sizeof(struct pfsync_state_clr);
		break;
	case PFSYNC_ACT_UPD_C:
		len = (sc->sc_maxcount * sizeof(struct pfsync_state_upd)) +
		    sizeof(struct pfsync_header);
		break;
	case PFSYNC_ACT_DEL_C:
		len = (sc->sc_maxcount * sizeof(struct pfsync_state_del)) +
		    sizeof(struct pfsync_header);
		break;
	case PFSYNC_ACT_UREQ:
		len = (sc->sc_maxcount * sizeof(struct pfsync_state_upd_req)) +
		    sizeof(struct pfsync_header);
		break;
	case PFSYNC_ACT_BUS:
		len = sizeof(struct pfsync_header) +
		    sizeof(struct pfsync_state_bus);
		break;
	case PFSYNC_ACT_TDB_UPD:
		len = (sc->sc_maxcount * sizeof(struct pfsync_tdb)) +
		    sizeof(struct pfsync_header);
		break;
	default:
		len = (sc->sc_maxcount * sizeof(struct pfsync_state)) +
		    sizeof(struct pfsync_header);
		break;
	}

	if (len > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			sc->sc_if.if_oerrors++;
			return (NULL);
		}
		m->m_data += (MCLBYTES - len) &~ (sizeof(long) - 1);
	} else
		MH_ALIGN(m, len);

	m->m_pkthdr.rcvif = NULL;
	m->m_pkthdr.len = m->m_len = sizeof(struct pfsync_header);
	h = mtod(m, struct pfsync_header *);
	h->version = PFSYNC_VERSION;
	h->af = 0;
	h->count = 0;
	h->action = action;
	if (action != PFSYNC_ACT_TDB_UPD)
		bcopy(&pf_status.pf_chksum, &h->pf_chksum,
		    PF_MD5_DIGEST_LENGTH);

	*sp = (void *)((char *)h + PFSYNC_HDRLEN);
	if (action == PFSYNC_ACT_TDB_UPD)
		timeout_add(&sc->sc_tdb_tmo, hz);
	else
		timeout_add(&sc->sc_tmo, hz);
	return (m);
}

int
pfsync_pack_state(u_int8_t action, struct pf_state *st, int flags)
{
	struct ifnet *ifp = NULL;
	struct pfsync_softc *sc = pfsyncif;
	struct pfsync_header *h, *h_net;
	struct pfsync_state *sp = NULL;
	struct pfsync_state_upd *up = NULL;
	struct pfsync_state_del *dp = NULL;
	struct pf_state_key *sk = st->state_key;
	struct pf_rule *r;
	u_long secs;
	int s, ret = 0;
	u_int8_t i = 255, newaction = 0;

	if (sc == NULL)
		return (0);
	ifp = &sc->sc_if;

	/*
	 * If a packet falls in the forest and there's nobody around to
	 * hear, does it make a sound?
	 */
	if (ifp->if_bpf == NULL && sc->sc_sync_ifp == NULL &&
	    sc->sc_sync_peer.s_addr == INADDR_PFSYNC_GROUP) {
		/* Don't leave any stale pfsync packets hanging around. */
		if (sc->sc_mbuf != NULL) {
			m_freem(sc->sc_mbuf);
			sc->sc_mbuf = NULL;
			sc->sc_statep.s = NULL;
		}
		return (0);
	}

	if (action >= PFSYNC_ACT_MAX)
		return (EINVAL);

	s = splnet();
	if (sc->sc_mbuf == NULL) {
		if ((sc->sc_mbuf = pfsync_get_mbuf(sc, action,
		    (void *)&sc->sc_statep.s)) == NULL) {
			splx(s);
			return (ENOMEM);
		}
		h = mtod(sc->sc_mbuf, struct pfsync_header *);
	} else {
		h = mtod(sc->sc_mbuf, struct pfsync_header *);
		if (h->action != action) {
			pfsync_sendout(sc);
			if ((sc->sc_mbuf = pfsync_get_mbuf(sc, action,
			    (void *)&sc->sc_statep.s)) == NULL) {
				splx(s);
				return (ENOMEM);
			}
			h = mtod(sc->sc_mbuf, struct pfsync_header *);
		} else {
			/*
			 * If it's an update, look in the packet to see if
			 * we already have an update for the state.
			 */
			if (action == PFSYNC_ACT_UPD && sc->sc_maxupdates) {
				struct pfsync_state *usp =
				    (void *)((char *)h + PFSYNC_HDRLEN);

				for (i = 0; i < h->count; i++) {
					if (!memcmp(usp->id, &st->id,
					    PFSYNC_ID_LEN) &&
					    usp->creatorid == st->creatorid) {
						sp = usp;
						sp->updates++;
						break;
					}
					usp++;
				}
			}
		}
	}

	secs = time_second;

	st->pfsync_time = time_uptime;

	if (sp == NULL) {
		/* not a "duplicate" update */
		i = 255;
		sp = sc->sc_statep.s++;
		sc->sc_mbuf->m_pkthdr.len =
		    sc->sc_mbuf->m_len += sizeof(struct pfsync_state);
		h->count++;
		bzero(sp, sizeof(*sp));

		bcopy(&st->id, sp->id, sizeof(sp->id));
		sp->creatorid = st->creatorid;

		strlcpy(sp->ifname, st->u.s.kif->pfik_name, sizeof(sp->ifname));
		pf_state_host_hton(&sk->lan, &sp->lan);
		pf_state_host_hton(&sk->gwy, &sp->gwy);
		pf_state_host_hton(&sk->ext, &sp->ext);

		bcopy(&st->rt_addr, &sp->rt_addr, sizeof(sp->rt_addr));

		sp->creation = htonl(secs - st->creation);
		pf_state_counter_hton(st->packets[0], sp->packets[0]);
		pf_state_counter_hton(st->packets[1], sp->packets[1]);
		pf_state_counter_hton(st->bytes[0], sp->bytes[0]);
		pf_state_counter_hton(st->bytes[1], sp->bytes[1]);
		if ((r = st->rule.ptr) == NULL)
			sp->rule = htonl(-1);
		else
			sp->rule = htonl(r->nr);
		if ((r = st->anchor.ptr) == NULL)
			sp->anchor = htonl(-1);
		else
			sp->anchor = htonl(r->nr);
		sp->af = sk->af;
		sp->proto = sk->proto;
		sp->direction = sk->direction;
		sp->log = st->log;
		sp->allow_opts = st->allow_opts;
		sp->timeout = st->timeout;

		if (flags & PFSYNC_FLAG_STALE)
			sp->sync_flags |= PFSTATE_STALE;
	}

	pf_state_peer_hton(&st->src, &sp->src);
	pf_state_peer_hton(&st->dst, &sp->dst);

	if (st->expire <= secs)
		sp->expire = htonl(0);
	else
		sp->expire = htonl(st->expire - secs);

	/* do we need to build "compressed" actions for network transfer? */
	if (sc->sc_sync_ifp && flags & PFSYNC_FLAG_COMPRESS) {
		switch (action) {
		case PFSYNC_ACT_UPD:
			newaction = PFSYNC_ACT_UPD_C;
			break;
		case PFSYNC_ACT_DEL:
			newaction = PFSYNC_ACT_DEL_C;
			break;
		default:
			/* by default we just send the uncompressed states */
			break;
		}
	}

	if (newaction) {
		if (sc->sc_mbuf_net == NULL) {
			if ((sc->sc_mbuf_net = pfsync_get_mbuf(sc, newaction,
			    (void *)&sc->sc_statep_net.s)) == NULL) {
				splx(s);
				return (ENOMEM);
			}
		}
		h_net = mtod(sc->sc_mbuf_net, struct pfsync_header *);

		switch (newaction) {
		case PFSYNC_ACT_UPD_C:
			if (i != 255) {
				up = (void *)((char *)h_net +
				    PFSYNC_HDRLEN + (i * sizeof(*up)));
				up->updates++;
			} else {
				h_net->count++;
				sc->sc_mbuf_net->m_pkthdr.len =
				    sc->sc_mbuf_net->m_len += sizeof(*up);
				up = sc->sc_statep_net.u++;

				bzero(up, sizeof(*up));
				bcopy(&st->id, up->id, sizeof(up->id));
				up->creatorid = st->creatorid;
			}
			up->timeout = st->timeout;
			up->expire = sp->expire;
			up->src = sp->src;
			up->dst = sp->dst;
			break;
		case PFSYNC_ACT_DEL_C:
			sc->sc_mbuf_net->m_pkthdr.len =
			    sc->sc_mbuf_net->m_len += sizeof(*dp);
			dp = sc->sc_statep_net.d++;
			h_net->count++;

			bzero(dp, sizeof(*dp));
			bcopy(&st->id, dp->id, sizeof(dp->id));
			dp->creatorid = st->creatorid;
			break;
		}
	}

	if (h->count == sc->sc_maxcount ||
	    (sc->sc_maxupdates && (sp->updates >= sc->sc_maxupdates)))
		ret = pfsync_sendout(sc);

	splx(s);
	return (ret);
}

/* This must be called in splnet() */
int
pfsync_request_update(struct pfsync_state_upd *up, struct in_addr *src)
{
	struct ifnet *ifp = NULL;
	struct pfsync_header *h;
	struct pfsync_softc *sc = pfsyncif;
	struct pfsync_state_upd_req *rup;
	int ret = 0;

	if (sc == NULL)
		return (0);

	ifp = &sc->sc_if;
	if (sc->sc_mbuf == NULL) {
		if ((sc->sc_mbuf = pfsync_get_mbuf(sc, PFSYNC_ACT_UREQ,
		    (void *)&sc->sc_statep.s)) == NULL)
			return (ENOMEM);
		h = mtod(sc->sc_mbuf, struct pfsync_header *);
	} else {
		h = mtod(sc->sc_mbuf, struct pfsync_header *);
		if (h->action != PFSYNC_ACT_UREQ) {
			pfsync_sendout(sc);
			if ((sc->sc_mbuf = pfsync_get_mbuf(sc, PFSYNC_ACT_UREQ,
			    (void *)&sc->sc_statep.s)) == NULL)
				return (ENOMEM);
			h = mtod(sc->sc_mbuf, struct pfsync_header *);
		}
	}

	if (src != NULL)
		sc->sc_sendaddr = *src;
	sc->sc_mbuf->m_pkthdr.len = sc->sc_mbuf->m_len += sizeof(*rup);
	h->count++;
	rup = sc->sc_statep.r++;
	bzero(rup, sizeof(*rup));
	if (up != NULL) {
		bcopy(up->id, rup->id, sizeof(rup->id));
		rup->creatorid = up->creatorid;
	}

	if (h->count == sc->sc_maxcount)
		ret = pfsync_sendout(sc);

	return (ret);
}

int
pfsync_clear_states(u_int32_t creatorid, char *ifname)
{
	struct ifnet *ifp = NULL;
	struct pfsync_softc *sc = pfsyncif;
	struct pfsync_state_clr *cp;
	int s, ret;

	if (sc == NULL)
		return (0);

	ifp = &sc->sc_if;
	s = splnet();
	if (sc->sc_mbuf != NULL)
		pfsync_sendout(sc);
	if ((sc->sc_mbuf = pfsync_get_mbuf(sc, PFSYNC_ACT_CLR,
	    (void *)&sc->sc_statep.c)) == NULL) {
		splx(s);
		return (ENOMEM);
	}
	sc->sc_mbuf->m_pkthdr.len = sc->sc_mbuf->m_len += sizeof(*cp);
	cp = sc->sc_statep.c;
	cp->creatorid = creatorid;
	if (ifname != NULL)
		strlcpy(cp->ifname, ifname, IFNAMSIZ);

	ret = (pfsync_sendout(sc));
	splx(s);
	return (ret);
}

void
pfsync_timeout(void *v)
{
	struct pfsync_softc *sc = v;
	int s;

	s = splnet();
	pfsync_sendout(sc);
	splx(s);
}

void
pfsync_tdb_timeout(void *v)
{
	struct pfsync_softc *sc = v;
	int s;

	s = splnet();
	pfsync_tdb_sendout(sc);
	splx(s);
}

/* This must be called in splnet() */
void
pfsync_send_bus(struct pfsync_softc *sc, u_int8_t status)
{
	struct pfsync_state_bus *bus;

	if (sc->sc_mbuf != NULL)
		pfsync_sendout(sc);

	if (pfsync_sync_ok &&
	    (sc->sc_mbuf = pfsync_get_mbuf(sc, PFSYNC_ACT_BUS,
	    (void *)&sc->sc_statep.b)) != NULL) {
		sc->sc_mbuf->m_pkthdr.len = sc->sc_mbuf->m_len += sizeof(*bus);
		bus = sc->sc_statep.b;
		bus->creatorid = pf_status.hostid;
		bus->status = status;
		bus->endtime = htonl(time_uptime - sc->sc_ureq_received);
		pfsync_sendout(sc);
	}
}

void
pfsync_bulk_update(void *v)
{
	struct pfsync_softc *sc = v;
	int s, i = 0;
	struct pf_state *state;

	s = splnet();
	if (sc->sc_mbuf != NULL)
		pfsync_sendout(sc);

	/*
	 * Grab at most PFSYNC_BULKPACKETS worth of states which have not
	 * been sent since the latest request was made.
	 */
	state = sc->sc_bulk_send_next;
	if (state)
		do {
			/* send state update if syncable and not already sent */
			if (!state->sync_flags
			    && state->timeout < PFTM_MAX
			    && state->pfsync_time <= sc->sc_ureq_received) {
				pfsync_pack_state(PFSYNC_ACT_UPD, state, 0);
				i++;
			}

			/* figure next state to send */
			state = TAILQ_NEXT(state, u.s.entry_list);

			/* wrap to start of list if we hit the end */
			if (!state)
				state = TAILQ_FIRST(&state_list);
		} while (i < sc->sc_maxcount * PFSYNC_BULKPACKETS &&
		    state != sc->sc_bulk_terminator);

	if (!state || state == sc->sc_bulk_terminator) {
		/* we're done */
		pfsync_send_bus(sc, PFSYNC_BUS_END);
		sc->sc_ureq_received = 0;
		sc->sc_bulk_send_next = NULL;
		sc->sc_bulk_terminator = NULL;
		timeout_del(&sc->sc_bulk_tmo);
		if (pf_status.debug >= PF_DEBUG_MISC)
			printf("pfsync: bulk update complete\n");
	} else {
		/* look again for more in a bit */
		timeout_add(&sc->sc_bulk_tmo, 1);
		sc->sc_bulk_send_next = state;
	}
	if (sc->sc_mbuf != NULL)
		pfsync_sendout(sc);
	splx(s);
}

void
pfsync_bulkfail(void *v)
{
	struct pfsync_softc *sc = v;
	int s, error;

	if (sc->sc_bulk_tries++ < PFSYNC_MAX_BULKTRIES) {
		/* Try again in a bit */
		timeout_add(&sc->sc_bulkfail_tmo, 5 * hz);
		s = splnet();
		error = pfsync_request_update(NULL, NULL);
		if (error == ENOMEM) {
			if (pf_status.debug >= PF_DEBUG_MISC)
				printf("pfsync: cannot allocate mbufs for "
				    "bulk update\n");
		} else
			pfsync_sendout(sc);
		splx(s);
	} else {
		/* Pretend like the transfer was ok */
		sc->sc_ureq_sent = 0;
		sc->sc_bulk_tries = 0;
#if NCARP > 0
		if (!pfsync_sync_ok)
			carp_group_demote_adj(&sc->sc_if, -1);
#endif
		pfsync_sync_ok = 1;
		if (pf_status.debug >= PF_DEBUG_MISC)
			printf("pfsync: failed to receive "
			    "bulk update status\n");
		timeout_del(&sc->sc_bulkfail_tmo);
	}
}

/* This must be called in splnet() */
int
pfsync_sendout(struct pfsync_softc *sc)
{
#if NBPFILTER > 0
	struct ifnet *ifp = &sc->sc_if;
#endif
	struct mbuf *m;

	timeout_del(&sc->sc_tmo);

	if (sc->sc_mbuf == NULL)
		return (0);
	m = sc->sc_mbuf;
	sc->sc_mbuf = NULL;
	sc->sc_statep.s = NULL;

#if NBPFILTER > 0
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_OUT);
#endif

	if (sc->sc_mbuf_net) {
		m_freem(m);
		m = sc->sc_mbuf_net;
		sc->sc_mbuf_net = NULL;
		sc->sc_statep_net.s = NULL;
	}

	return pfsync_sendout_mbuf(sc, m);
}

int
pfsync_tdb_sendout(struct pfsync_softc *sc)
{
#if NBPFILTER > 0
	struct ifnet *ifp = &sc->sc_if;
#endif
	struct mbuf *m;

	timeout_del(&sc->sc_tdb_tmo);

	if (sc->sc_mbuf_tdb == NULL)
		return (0);
	m = sc->sc_mbuf_tdb;
	sc->sc_mbuf_tdb = NULL;
	sc->sc_statep_tdb.t = NULL;

#if NBPFILTER > 0
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_OUT);
#endif

	return pfsync_sendout_mbuf(sc, m);
}

int
pfsync_sendout_mbuf(struct pfsync_softc *sc, struct mbuf *m)
{
	struct sockaddr sa;
	struct ip *ip;

	if (sc->sc_sync_ifp ||
	    sc->sc_sync_peer.s_addr != INADDR_PFSYNC_GROUP) {
		M_PREPEND(m, sizeof(struct ip), M_DONTWAIT);
		if (m == NULL) {
			pfsyncstats.pfsyncs_onomem++;
			return (0);
		}
		ip = mtod(m, struct ip *);
		ip->ip_v = IPVERSION;
		ip->ip_hl = sizeof(*ip) >> 2;
		ip->ip_tos = IPTOS_LOWDELAY;
		ip->ip_len = htons(m->m_pkthdr.len);
		ip->ip_id = htons(ip_randomid());
		ip->ip_off = htons(IP_DF);
		ip->ip_ttl = PFSYNC_DFLTTL;
		ip->ip_p = IPPROTO_PFSYNC;
		ip->ip_sum = 0;

		bzero(&sa, sizeof(sa));
		ip->ip_src.s_addr = INADDR_ANY;

		if (sc->sc_sendaddr.s_addr == INADDR_PFSYNC_GROUP)
			m->m_flags |= M_MCAST;
		ip->ip_dst = sc->sc_sendaddr;
		sc->sc_sendaddr.s_addr = sc->sc_sync_peer.s_addr;

		pfsyncstats.pfsyncs_opackets++;

		if (ip_output(m, NULL, NULL, IP_RAWOUTPUT, &sc->sc_imo, NULL))
			pfsyncstats.pfsyncs_oerrors++;
	} else
		m_freem(m);

	return (0);
}

#ifdef IPSEC
/* Update an in-kernel tdb. Silently fail if no tdb is found. */
void
pfsync_update_net_tdb(struct pfsync_tdb *pt)
{
	struct tdb		*tdb;
	int			 s;

	/* check for invalid values */
	if (ntohl(pt->spi) <= SPI_RESERVED_MAX ||
	    (pt->dst.sa.sa_family != AF_INET &&
	     pt->dst.sa.sa_family != AF_INET6))
		goto bad;

	s = spltdb();
	tdb = gettdb(pt->spi, &pt->dst, pt->sproto);
	if (tdb) {
		pt->rpl = ntohl(pt->rpl);
		pt->cur_bytes = betoh64(pt->cur_bytes);

		/* Neither replay nor byte counter should ever decrease. */
		if (pt->rpl < tdb->tdb_rpl ||
		    pt->cur_bytes < tdb->tdb_cur_bytes) {
			splx(s);
			goto bad;
		}

		tdb->tdb_rpl = pt->rpl;
		tdb->tdb_cur_bytes = pt->cur_bytes;
	}
	splx(s);
	return;

 bad:
	if (pf_status.debug >= PF_DEBUG_MISC)
		printf("pfsync_insert: PFSYNC_ACT_TDB_UPD: "
		    "invalid value\n");
	pfsyncstats.pfsyncs_badstate++;
	return;
}

/* One of our local tdbs have been updated, need to sync rpl with others */
int
pfsync_update_tdb(struct tdb *tdb, int output)
{
	struct ifnet *ifp = NULL;
	struct pfsync_softc *sc = pfsyncif;
	struct pfsync_header *h;
	struct pfsync_tdb *pt = NULL;
	int s, i, ret;

	if (sc == NULL)
		return (0);

	ifp = &sc->sc_if;
	if (ifp->if_bpf == NULL && sc->sc_sync_ifp == NULL &&
	    sc->sc_sync_peer.s_addr == INADDR_PFSYNC_GROUP) {
		/* Don't leave any stale pfsync packets hanging around. */
		if (sc->sc_mbuf_tdb != NULL) {
			m_freem(sc->sc_mbuf_tdb);
			sc->sc_mbuf_tdb = NULL;
			sc->sc_statep_tdb.t = NULL;
		}
		return (0);
	}

	s = splnet();
	if (sc->sc_mbuf_tdb == NULL) {
		if ((sc->sc_mbuf_tdb = pfsync_get_mbuf(sc, PFSYNC_ACT_TDB_UPD,
		    (void *)&sc->sc_statep_tdb.t)) == NULL) {
			splx(s);
			return (ENOMEM);
		}
		h = mtod(sc->sc_mbuf_tdb, struct pfsync_header *);
	} else {
		h = mtod(sc->sc_mbuf_tdb, struct pfsync_header *);
		if (h->action != PFSYNC_ACT_TDB_UPD) {
			/*
			 * XXX will never happen as long as there's
			 * only one "TDB action".
			 */
			pfsync_tdb_sendout(sc);
			sc->sc_mbuf_tdb = pfsync_get_mbuf(sc,
			    PFSYNC_ACT_TDB_UPD, (void *)&sc->sc_statep_tdb.t);
			if (sc->sc_mbuf_tdb == NULL) {
				splx(s);
				return (ENOMEM);
			}
			h = mtod(sc->sc_mbuf_tdb, struct pfsync_header *);
		} else if (sc->sc_maxupdates) {
			/*
			 * If it's an update, look in the packet to see if
			 * we already have an update for the state.
			 */
			struct pfsync_tdb *u =
			    (void *)((char *)h + PFSYNC_HDRLEN);

			for (i = 0; !pt && i < h->count; i++) {
				if (tdb->tdb_spi == u->spi &&
				    tdb->tdb_sproto == u->sproto &&
			            !bcmp(&tdb->tdb_dst, &u->dst,
				    SA_LEN(&u->dst.sa))) {
					pt = u;
					pt->updates++;
				}
				u++;
			}
		}
	}

	if (pt == NULL) {
		/* not a "duplicate" update */
		pt = sc->sc_statep_tdb.t++;
		sc->sc_mbuf_tdb->m_pkthdr.len =
		    sc->sc_mbuf_tdb->m_len += sizeof(struct pfsync_tdb);
		h->count++;
		bzero(pt, sizeof(*pt));

		pt->spi = tdb->tdb_spi;
		memcpy(&pt->dst, &tdb->tdb_dst, sizeof pt->dst);
		pt->sproto = tdb->tdb_sproto;
	}

	/*
	 * When a failover happens, the master's rpl is probably above
	 * what we see here (we may be up to a second late), so
	 * increase it a bit for outbound tdbs to manage most such
	 * situations.
	 *
	 * For now, just add an offset that is likely to be larger
	 * than the number of packets we can see in one second. The RFC
	 * just says the next packet must have a higher seq value.
	 *
	 * XXX What is a good algorithm for this? We could use
	 * a rate-determined increase, but to know it, we would have
	 * to extend struct tdb.
	 * XXX pt->rpl can wrap over MAXINT, but if so the real tdb
	 * will soon be replaced anyway. For now, just don't handle
	 * this edge case.
	 */
#define RPL_INCR 16384
	pt->rpl = htonl(tdb->tdb_rpl + (output ? RPL_INCR : 0));
	pt->cur_bytes = htobe64(tdb->tdb_cur_bytes);

	if (h->count == sc->sc_maxcount ||
	    (sc->sc_maxupdates && (pt->updates >= sc->sc_maxupdates)))
		ret = pfsync_tdb_sendout(sc);

	splx(s);
	return (ret);
}
#endif
