/*	$OpenBSD: src/sys/altq/altq_cbq.c,v 1.12 2003/02/05 17:43:12 henning Exp $	*/
/*	$KAME: altq_cbq.c,v 1.9 2000/12/14 08:12:45 thorpej Exp $	*/

/*
 * Copyright (c) Sun Microsystems, Inc. 1993-1998 All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the SMCC Technology
 *      Development Group at Sun Microsystems, Inc.
 *
 * 4. The name of the Sun Microsystems, Inc nor may not be used to endorse or
 *      promote products derived from this software without specific prior
 *      written permission.
 *
 * SUN MICROSYSTEMS DOES NOT CLAIM MERCHANTABILITY OF THIS SOFTWARE OR THE
 * SUITABILITY OF THIS SOFTWARE FOR ANY PARTICULAR PURPOSE.  The software is
 * provided "as is" without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this software.
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/time.h>

#include <net/if.h>
#include <netinet/in.h>

#include <net/pfvar.h>
#include <altq/altq.h>
#include <altq/altq_cbq.h>

/*
 * Forward Declarations.
 */
static int		 cbq_class_destroy(cbq_state_t *, struct rm_class *);
static struct rm_class  *clh_to_clp(cbq_state_t *, u_int32_t);
static int		 cbq_clear_interface(cbq_state_t *);
static int		 cbq_request(struct ifaltq *, int, void *);
static int		 cbq_enqueue(struct ifaltq *, struct mbuf *,
			     struct altq_pktattr *);
static struct mbuf	*cbq_dequeue(struct ifaltq *, int);
static void		 cbqrestart(struct ifaltq *);
static void		 get_class_stats(class_stats_t *, struct rm_class *);
static void		 cbq_purge(cbq_state_t *);

/*
 * int
 * cbq_class_destroy(cbq_mod_state_t *, struct rm_class *) - This
 *	function destroys a given traffic class.  Before destroying
 *	the class, all traffic for that class is released.
 */
static int
cbq_class_destroy(cbq_state_t *cbqp, struct rm_class *cl)
{
	u_int32_t	chandle;

	chandle = cl->stats_.handle;

	/* delete the class */
	rmc_delete_class(&cbqp->ifnp, cl);

	/*
	 * free the class handle
	 */
	switch (chandle) {
	case ROOT_CLASS_HANDLE:
		cbqp->ifnp.root_ = NULL;
		break;
	case DEFAULT_CLASS_HANDLE:
		cbqp->ifnp.default_ = NULL;
		break;
	case CTL_CLASS_HANDLE:
		cbqp->ifnp.ctl_ = NULL;
		break;
	case NULL_CLASS_HANDLE:
		break;
	default:
		if (chandle >= CBQ_MAX_CLASSES)
			break;
		cbqp->cbq_class_tbl[chandle] = NULL;
	}

	return (0);
}

/* convert class handle to class pointer */
static struct rm_class *
clh_to_clp(cbq_state_t *cbqp, u_int32_t chandle)
{
	switch (chandle) {
	case NULL_CLASS_HANDLE:
		return (NULL);
	case ROOT_CLASS_HANDLE:
		return (cbqp->ifnp.root_);
	case DEFAULT_CLASS_HANDLE:
		return (cbqp->ifnp.default_);
	case CTL_CLASS_HANDLE:
		return (cbqp->ifnp.ctl_);
	}

	if (chandle >= CBQ_MAX_CLASSES)
		return (NULL);

	return (cbqp->cbq_class_tbl[chandle]);
}

static int
cbq_clear_interface(cbq_state_t *cbqp)
{
	int		 again, i;
	struct rm_class	*cl;

	/* clear out the classes now */
	do {
		again = 0;
		for (i = 0; i < CBQ_MAX_CLASSES; i++) {
			if ((cl = cbqp->cbq_class_tbl[i]) != NULL) {
				if (is_a_parent_class(cl))
					again++;
				else {
					cbq_class_destroy(cbqp, cl);
					cbqp->cbq_class_tbl[i] = NULL;
				}
			}
		}
		if (cbqp->ifnp.ctl_ != NULL &&
		    !is_a_parent_class(cbqp->ifnp.ctl_)) {
			cbq_class_destroy(cbqp, cbqp->ifnp.ctl_);
			cbqp->ifnp.ctl_ = NULL;
		}
		if (cbqp->ifnp.default_ != NULL &&
		    !is_a_parent_class(cbqp->ifnp.default_)) {
			cbq_class_destroy(cbqp, cbqp->ifnp.default_);
			cbqp->ifnp.default_ = NULL;
		}
		if (cbqp->ifnp.root_ != NULL &&
		    !is_a_parent_class(cbqp->ifnp.root_)) {
			cbq_class_destroy(cbqp, cbqp->ifnp.root_);
			cbqp->ifnp.root_ = NULL;
		}
	} while (again);

	return (0);
}

static int
cbq_request(struct ifaltq *ifq, int req, void *arg)
{
	cbq_state_t	*cbqp = (cbq_state_t *)ifq->altq_disc;

	switch (req) {
	case ALTRQ_PURGE:
		cbq_purge(cbqp);
		break;
	}
	return (0);
}

/* copy the stats info in rm_class to class_states_t */
static void
get_class_stats(class_stats_t *statsp, struct rm_class *cl)
{
	statsp->xmit_cnt	= cl->stats_.xmit_cnt;
	statsp->drop_cnt	= cl->stats_.drop_cnt;
	statsp->over		= cl->stats_.over;
	statsp->borrows		= cl->stats_.borrows;
	statsp->overactions	= cl->stats_.overactions;
	statsp->delays		= cl->stats_.delays;

	statsp->depth		= cl->depth_;
	statsp->priority	= cl->pri_;
	statsp->maxidle		= cl->maxidle_;
	statsp->minidle		= cl->minidle_;
	statsp->offtime		= cl->offtime_;
	statsp->qmax		= qlimit(cl->q_);
	statsp->ns_per_byte	= cl->ns_per_byte_;
	statsp->wrr_allot	= cl->w_allotment_;
	statsp->qcnt		= qlen(cl->q_);
	statsp->avgidle		= cl->avgidle_;

	statsp->qtype		= qtype(cl->q_);
#ifdef ALTQ_RED
	if (q_is_red(cl->q_))
		red_getstats(cl->red_, &statsp->red[0]);
#endif
#ifdef ALTQ_RIO
	if (q_is_rio(cl->q_))
		rio_getstats((rio_t *)cl->red_, &statsp->red[0]);
#endif
}

int
cbq_pfattach(struct pf_altq *a)
{
	struct ifnet	*ifp;
	int		 s, error;

	if ((ifp = ifunit(a->ifname)) == NULL || a->altq_disc == NULL)
		return (EINVAL);
	s = splimp();
	error = altq_attach(&ifp->if_snd, ALTQT_CBQ, a->altq_disc,
	    cbq_enqueue, cbq_dequeue, cbq_request, NULL, NULL);
	splx(s);
	return (error);
}

int
cbq_add_altq(struct pf_altq *a)
{
	cbq_state_t	*cbqp;
	struct ifnet	*ifp;

	if ((ifp = ifunit(a->ifname)) == NULL)
		return (EINVAL);
	if (!ALTQ_IS_READY(&ifp->if_snd))
		return (ENODEV);

	/* allocate and initialize cbq_state_t */
	MALLOC(cbqp, cbq_state_t *, sizeof(cbq_state_t), M_DEVBUF, M_WAITOK);
	if (cbqp == NULL)
		return (ENOMEM);
	bzero(cbqp, sizeof(cbq_state_t));
	CALLOUT_INIT(&cbqp->cbq_callout);
	cbqp->cbq_qlen = 0;
	cbqp->ifnp.ifq_ = &ifp->if_snd;	    /* keep the ifq */

	/* keep the state in pf_altq */
	a->altq_disc = cbqp;

	return (0);
}

int
cbq_remove_altq(struct pf_altq *a)
{
	cbq_state_t	*cbqp;

	if ((cbqp = a->altq_disc) == NULL)
		return (EINVAL);
	a->altq_disc = NULL;

	cbq_clear_interface(cbqp);

	if (cbqp->ifnp.ctl_)
		cbq_class_destroy(cbqp, cbqp->ifnp.ctl_);
	if (cbqp->ifnp.default_)
		cbq_class_destroy(cbqp, cbqp->ifnp.default_);
	if (cbqp->ifnp.root_)
		cbq_class_destroy(cbqp, cbqp->ifnp.root_);

	/* deallocate cbq_state_t */
	FREE(cbqp, M_DEVBUF);

	return (0);
}

int
cbq_add_queue(struct pf_altq *a)
{
	struct rm_class	*borrow, *parent;
	cbq_state_t	*cbqp;
	struct rm_class	*cl;
	struct cbq_opts	*opts;
	u_int32_t	 chandle;
	int		 i;

	if ((cbqp = a->altq_disc) == NULL)
		return (EINVAL);

	opts = &a->pq_u.cbq_opts;
	/* check parameters */
	if (a->priority >= RM_MAXPRIO)
		return (EINVAL);

	/* Get pointers to parent and borrow classes.  */
	parent = clh_to_clp(cbqp, a->parent_qid);
	if (opts->flags & CBQCLF_BORROW)
		borrow = parent;
	else
		borrow = NULL;

	/*
	 * A class must borrow from it's parent or it can not
	 * borrow at all.  Hence, borrow can be null.
	 */
	if (parent == NULL && (opts->flags & CBQCLF_ROOTCLASS) == 0) {
		printf("cbq_add_queue: no parent class!\n");
		return (EINVAL);
	}

	if ((borrow != parent)  && (borrow != NULL)) {
		printf("cbq_add_class: borrow class != parent\n");
		return (EINVAL);
	}

	/*
	 * allocate class handle
	 */
	switch (opts->flags & CBQCLF_CLASSMASK) {
	case CBQCLF_ROOTCLASS:
		if (parent != NULL)
			return (EINVAL);
		if (cbqp->ifnp.root_)
			return (EINVAL);
		chandle = ROOT_CLASS_HANDLE;
		break;
	case CBQCLF_DEFCLASS:
		if (cbqp->ifnp.default_)
			return (EINVAL);
		chandle = DEFAULT_CLASS_HANDLE;
		break;
	case CBQCLF_CTLCLASS:
		if (cbqp->ifnp.ctl_)
			return (EINVAL);
		chandle = CTL_CLASS_HANDLE;
		break;
	case 0:
		/* find a free class slot. for now, reserve qid 0 */
		for (i = 1; i < CBQ_MAX_CLASSES; i++)
			if (cbqp->cbq_class_tbl[i] == NULL)
				break;
		if (i == CBQ_MAX_CLASSES)
			return (ENOSPC);
		chandle = (u_int32_t)i;
		break;
	default:
		/* more than two flags bits set */
		return (EINVAL);
	}

	/*
	 * create a class.  if this is a root class, initialize the
	 * interface.
	 */
	if (chandle == ROOT_CLASS_HANDLE) {
		rmc_init(cbqp->ifnp.ifq_, &cbqp->ifnp, opts->ns_per_byte,
		    cbqrestart, a->qlimit, RM_MAXQUEUED,
		    opts->maxidle, opts->minidle, opts->offtime,
		    opts->flags);
		cl = cbqp->ifnp.root_;
	} else {
		cl = rmc_newclass(a->priority,
				  &cbqp->ifnp, opts->ns_per_byte,
				  rmc_delay_action, a->qlimit, parent, borrow,
				  opts->maxidle, opts->minidle, opts->offtime,
				  opts->pktsize, opts->flags);
	}
	if (cl == NULL)
		return (ENOMEM);

	/* return handle to user space. */
	a->qid = chandle;

	cl->stats_.handle = chandle;
	cl->stats_.depth = cl->depth_;

	/* save the allocated class */
	switch (chandle) {
	case NULL_CLASS_HANDLE:
	case ROOT_CLASS_HANDLE:
		break;
	case DEFAULT_CLASS_HANDLE:
		cbqp->ifnp.default_ = cl;
		break;
	case CTL_CLASS_HANDLE:
		cbqp->ifnp.ctl_ = cl;
		break;
	default:
		cbqp->cbq_class_tbl[chandle] = cl;
		break;
	}
	return (0);
}

int
cbq_remove_queue(struct pf_altq *a)
{
	struct rm_class	*cl;
	cbq_state_t	*cbqp;

	if ((cbqp = a->altq_disc) == NULL)
		return (EINVAL);

	if ((cl = clh_to_clp(cbqp, a->qid)) == NULL)
		return (EINVAL);

	/* if we are a parent class, then return an error. */
	if (is_a_parent_class(cl))
		return (EINVAL);

	/* delete the class */
	rmc_delete_class(&cbqp->ifnp, cl);

	/*
	 * free the class handle
	 */
	switch (a->qid) {
	case ROOT_CLASS_HANDLE:
		cbqp->ifnp.root_ = NULL;
		break;
	case DEFAULT_CLASS_HANDLE:
		cbqp->ifnp.default_ = NULL;
		break;
	case CTL_CLASS_HANDLE:
		cbqp->ifnp.ctl_ = NULL;
		break;
	case NULL_CLASS_HANDLE:
		break;
	default:
		if (a->qid >= CBQ_MAX_CLASSES)
			break;
		cbqp->cbq_class_tbl[a->qid] = NULL;
	}

	return (0);
}

int
cbq_getqstats(struct pf_altq *a, void *ubuf, int *nbytes)
{
	cbq_state_t	*cbqp;
	struct rm_class	*cl;
	class_stats_t	 stats;
	int		 error = 0;

	if ((cbqp = altq_lookup(a->ifname, ALTQT_CBQ)) == NULL)
		return (EBADF);

	if ((cl = clh_to_clp(cbqp, a->qid)) == NULL)
		return (EINVAL);

	if (*nbytes < sizeof(stats))
		return (EINVAL);

	get_class_stats(&stats, cl);
	stats.handle = a->qid;

	if ((error = copyout((caddr_t)&stats, ubuf, sizeof(stats))) != 0)
		return (error);
	*nbytes = sizeof(stats);
	return (0);
}

/*
 * int
 * cbq_enqueue(struct ifaltq *ifq, struct mbuf *m, struct altq_pktattr *pattr)
 *		- Queue data packets.
 *
 *	cbq_enqueue is set to ifp->if_altqenqueue and called by an upper
 *	layer (e.g. ether_output).  cbq_enqueue queues the given packet
 *	to the cbq, then invokes the driver's start routine.
 *
 *	Assumptions:	called in splimp
 *	Returns:	0 if the queueing is successful.
 *			ENOBUFS if a packet dropping occurred as a result of
 *			the queueing.
 */

static int
cbq_enqueue(struct ifaltq *ifq, struct mbuf *m, struct altq_pktattr *pktattr)
{
	cbq_state_t	*cbqp = (cbq_state_t *)ifq->altq_disc;
	struct rm_class	*cl;
	struct m_tag	*t;
	int		 len;

	/* grab class set by classifier */
	t = m_tag_find(m, PACKET_TAG_PF_QID, NULL);
	if (t == NULL ||
	    (cl = clh_to_clp(cbqp, ((struct altq_tag *)(t+1))->qid)) == NULL) {
		cl = cbqp->ifnp.default_;
		if (cl == NULL) {
			m_freem(m);
			return (ENOBUFS);
		}
		cl->pktattr_ = NULL;
	}

	len = m_pktlen(m);
	if (rmc_queue_packet(cl, m) != 0) {
		/* drop occurred.  some mbuf was freed in rmc_queue_packet. */
		PKTCNTR_ADD(&cl->stats_.drop_cnt, len);
		return (ENOBUFS);
	}

	/* successfully queued. */
	++cbqp->cbq_qlen;
	IFQ_INC_LEN(ifq);
	return (0);
}

static struct mbuf *
cbq_dequeue(struct ifaltq *ifq, int op)
{
	cbq_state_t	*cbqp = (cbq_state_t *)ifq->altq_disc;
	struct mbuf	*m;

	m = rmc_dequeue_next(&cbqp->ifnp, op);

	if (m && op == ALTDQ_REMOVE) {
		--cbqp->cbq_qlen;  /* decrement # of packets in cbq */
		IFQ_DEC_LEN(ifq);

		/* Update the class. */
		rmc_update_class_util(&cbqp->ifnp);
	}
	return (m);
}

/*
 * void
 * cbqrestart(queue_t *) - Restart sending of data.
 * called from rmc_restart in splimp via timeout after waking up
 * a suspended class.
 *	Returns:	NONE
 */

static void
cbqrestart(struct ifaltq *ifq)
{
	cbq_state_t	*cbqp;
	struct ifnet	*ifp;

	if (!ALTQ_IS_ENABLED(ifq))
		/* cbq must have been detached */
		return;

	if ((cbqp = (cbq_state_t *)ifq->altq_disc) == NULL)
		/* should not happen */
		return;

	ifp = ifq->altq_ifp;
	if (ifp->if_start &&
	    cbqp->cbq_qlen > 0 && (ifp->if_flags & IFF_OACTIVE) == 0)
		(*ifp->if_start)(ifp);
}

static void cbq_purge(cbq_state_t *cbqp)
{
	struct rm_class	*cl;
	int		 i;

	for (i = 0; i < CBQ_MAX_CLASSES; i++)
		if ((cl = cbqp->cbq_class_tbl[i]) != NULL)
			rmc_dropall(cl);
	if (ALTQ_IS_ENABLED(cbqp->ifnp.ifq_))
		cbqp->ifnp.ifq_->ifq_len = 0;
}
