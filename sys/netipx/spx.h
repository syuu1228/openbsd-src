/*	$OpenBSD: src/sys/netipx/Attic/spx.h,v 1.1 1996/08/16 09:16:03 mickey Exp $	*/
/*	$NOWHERE: spx.h,v 1.2 1996/05/07 09:49:52 mickey Exp $	*/

/*-
 *
 * Copyright (c) 1996 Michael Shalayeff
 * Copyright (c) 1995, Mike Mitchell
 * Copyright (c) 1984, 1985, 1986, 1987, 1993
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
 *
 *	@(#)spx.h
 *
 * from FreeBSD Id: spx.h,v 1.7 1996/01/30 22:58:51 mpp Exp
 */

#ifndef _NETIPX_SPX_H_
#define	_NETIPX_SPX_H_

#include <sys/queue.h>

#define	XXX	__attribute__((packed));

/*
 * Definitions for IPX style Sequenced Packet Protocol
 */

struct spxhdr {
	u_int8_t	spx_cc	XXX;	/* connection control */
	u_int8_t	spx_dt	XXX;	/* datastream type */
#define	SPX_SP		0x80		/* system packet */
#define	SPX_SA		0x40		/* send acknowledgement */
#define	SPX_OB		0x20		/* attention (out of band data) */
#define	SPX_EM		0x10		/* end of message */
	u_int16_t	spx_sid	XXX;	/* source connection identifier */
	u_int16_t	spx_did	XXX;	/* destination connection identifier */
	u_int16_t	spx_seq	XXX;	/* sequence number */
	u_int16_t	spx_ack	XXX;	/* acknowledge number */
	u_int16_t	spx_alo	XXX;	/* allocation number */
};

/*
 * Definitions for NS(tm) Internet Datagram Protocol
 * containing a Sequenced Packet Protocol packet.
 */
struct spx {
	struct ipx	si_i XXX;
	struct spxhdr 	si_s XXX;
};

#define SI(x)	((struct spx *)x)
#define si_sum	si_i.ipx_sum
#define si_len	si_i.ipx_len
#define si_tc	si_i.ipx_tc
#define si_pt	si_i.ipx_pt
#define si_dna	si_i.ipx_dna
#define si_sna	si_i.ipx_sna
#define si_sport	si_i.ipx_sna.ipx_port
#define si_cc	si_s.spx_cc
#define si_dt	si_s.spx_dt
#define si_sid	si_s.spx_sid
#define si_did	si_s.spx_did
#define si_seq	si_s.spx_seq
#define si_ack	si_s.spx_ack
#define si_alo	si_s.spx_alo

struct spx_q {
	TAILQ_ENTRY(spx_q)	list;
	caddr_t	data;
};

/*
 * SPX control block, one per connection
 */
struct spxpcb {
	TAILQ_HEAD(, spx_q)	spxp_queue;
	struct	ipxpcb	*s_ipxpcb;	/* backpointer to ipx pcb */
	u_char	s_state;
	u_char	s_flags;
#define	SF_ACKNOW	0x01		/* Ack peer immediately */
#define	SF_DELACK	0x02		/* Ack, but try to delay it */
#define	SF_HI	0x04			/* Show headers on input */
#define	SF_HO	0x08			/* Show headers on output */
#define	SF_PI	0x10			/* Packet (datagram) interface */
#define SF_WIN	0x20			/* Window info changed */
#define SF_RXT	0x40			/* Rxt info changed */
#define SF_RVD	0x80			/* Calling from read usrreq routine */
	u_short s_mtu;			/* Max packet size for this stream */
/* use sequence fields in headers to store sequence numbers for this
   connection */
	struct	ipx	*s_ipx;
	struct	spxhdr	s_shdr;		/* prototype header to transmit */
#define s_cc s_shdr.spx_cc		/* connection control (for EM bit) */
#define s_dt s_shdr.spx_dt		/* datastream type */
#define s_sid s_shdr.spx_sid		/* source connection identifier */
#define s_did s_shdr.spx_did		/* destination connection identifier */
#define s_seq s_shdr.spx_seq		/* sequence number */
#define s_ack s_shdr.spx_ack		/* acknowledge number */
#define s_alo s_shdr.spx_alo		/* allocation number */
#define s_dport s_ipx->ipx_dna.ipx_port	/* where we are sending */
	struct spxhdr s_rhdr;		/* last received header (in effect!)*/
	u_short s_rack;			/* their acknowledge number */
	u_short s_ralo;			/* their allocation number */
	u_short s_smax;			/* highest packet # we have sent */
	u_short	s_snxt;			/* which packet to send next */

/* congestion control */
#define	CUNIT	1024			/* scaling for ... */
	int	s_cwnd;			/* Congestion-controlled window */
					/* in packets * CUNIT */
	short	s_swnd;			/* == tcp snd_wnd, in packets */
	short	s_smxw;			/* == tcp max_sndwnd */
					/* difference of two spx_seq's can be
					   no bigger than a short */
	u_short	s_swl1;			/* == tcp snd_wl1 */
	u_short	s_swl2;			/* == tcp snd_wl2 */
	int	s_cwmx;			/* max allowable cwnd */
	int	s_ssthresh;		/* s_cwnd size threshold for
					 * slow start exponential-to-
					 * linear switch */
/* transmit timing stuff
 * srtt and rttvar are stored as fixed point, for convenience in smoothing.
 * srtt has 3 bits to the right of the binary point, rttvar has 2.
 */
	short	s_idle;			/* time idle */
#define	SPXT_NTIMERS	4
	short	s_timer[SPXT_NTIMERS];	/* timers */
	short	s_rxtshift;		/* log(2) of rexmt exp. backoff */
	short	s_rxtcur;		/* current retransmit value */
	u_short	s_rtseq;		/* packet being timed */
	short	s_rtt;			/* timer for round trips */
	short	s_srtt;			/* averaged timer */
	short	s_rttvar;		/* variance in round trip time */
	char	s_force;		/* which timer expired */
	char	s_dupacks;		/* counter to intuit xmt loss */

/* out of band data */
	char	s_oobflags;
#define SF_SOOB	0x08			/* sending out of band data */
#define SF_IOOB 0x10			/* receiving out of band data */
	char	s_iobc;			/* input characters */
/* debug stuff */
	u_short	s_want;			/* Last candidate for sending */
	char	s_outx;			/* exit taken from spx_output */
	char	s_inx;			/* exit taken from spx_input */
	u_short	s_flags2;		/* more flags for testing */
#define SF_NEWCALL	0x100		/* for new_recvmsg */
#define SO_NEWCALL	10		/* for new_recvmsg */
};

#define	ipxtospxpcb(np)	((struct spxpcb *)(np)->ipxp_ppcb)
#define	sotospxpcb(so)	(ipxtospxpcb(sotoipxpcb(so)))

#ifdef _KERNEL

void	spx_abort __P((struct ipxpcb *ipxp));
struct spxpcb *spx_close __P((struct spxpcb *cb));
void	*spx_ctlinput __P((int cmd, struct sockaddr *arg_as_sa, void *dummy));
int	spx_ctloutput __P((int req, struct socket *so, int level, int name,
			   struct mbuf **value));
struct spxpcb *spx_disconnect __P((struct spxpcb *cb));
struct spxpcb *spx_drop __P((struct spxpcb *cb, int errno));
void	spx_fasttimo __P((void));
void	spx_init __P((void));
void	spx_input __P((struct mbuf *m, ...));
int	spx_output __P((struct spxpcb *cb, struct mbuf *m0));
void	spx_quench __P((struct ipxpcb *ipxp));
int	spx_reass __P((struct spxpcb *cb, struct spx *si));
void	spx_setpersist __P((struct spxpcb *cb));
void	spx_slowtimo __P((void));
void	spx_template __P((struct spxpcb *cb));
struct spxpcb *spx_timers __P((struct spxpcb *cb, int timer));
struct spxpcb *spx_usrclosed __P((struct spxpcb *cb));
int	spx_usrreq __P((struct socket *so, int req, struct mbuf *m,
			struct mbuf *nam, struct mbuf *controlp));
int	spx_usrreq_sp __P((struct socket *so, int req, struct mbuf *m,
			   struct mbuf *nam, struct mbuf *controlp));

#endif /* _KERNEL */

#endif /* !_NETIPX_SPX_H_ */
