/*	$OpenBSD: src/sys/netinet/Attic/if_arc.h,v 1.7 2003/06/02 23:28:13 millert Exp $	*/
/*	$NetBSD: if_arc.h,v 1.5 1995/06/07 00:14:04 cgd Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1993
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
 *
 * from: NetBSD: if_ether.h,v 1.10 1994/06/29 06:37:55 cgd Exp
 *       @(#)if_ether.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _NETINET_IF_ARC_H_
#define _NETINET_IF_ARC_H_

/*
 * Arcnet address - 1 octets
 * don't know who uses this.
 */
struct arc_addr {
	u_int8_t  arc_addr_octet[1];
};

/*
 * Structure of a 2.5MB/s Arcnet header.
 * as given to interface code.
 */
struct	arc_header {
	u_int8_t  arc_shost;
	u_int8_t  arc_dhost;
	u_int8_t  arc_type;
	/*
	 * only present for newstyle encoding with LL fragmentation.
	 * Don't use sizeof(anything), use ARC_HDR{,NEW}LEN instead.
	 */
	u_int8_t  arc_flag;
	u_int16_t arc_seqid;

	/*
	 * only present in exception packets (arc_flag == 0xff)
	 */
	u_int8_t  arc_type2;	/* same as arc_type */
	u_int8_t  arc_flag2;	/* real flag value */
	u_int16_t arc_seqid2;	/* real seqid value */
};

#define	ARC_ADDR_LEN		1

#define	ARC_HDRLEN		3
#define	ARC_HDRNEWLEN		6

/* these lengths are data link layer length - 2*ARC_ADDR_LEN */
#define	ARC_MIN_LEN		1
#define	ARC_MIN_FORBID_LEN	254
#define	ARC_MAX_FORBID_LEN	256
#define	ARC_MAX_LEN		508


/* RFC 1051 */
#define	ARCTYPE_IP_OLD		240	/* IP protocol */
#define	ARCTYPE_ARP_OLD		241	/* address resolution protocol */

/* RFC 1201 */
#define	ARCTYPE_IP		212	/* IP protocol */
#define	ARCTYPE_ARP		213	/* address resolution protocol */
#define	ARCTYPE_REVARP		214	/* reverse addr resolution protocol */

#define	ARCTYPE_ATALK		221	/* Appletalk */
#define	ARCTYPE_BANIAN		247	/* Banyan Vines */
#define	ARCTYPE_IPX		250	/* Novell IPX */

#define	ARCTYPE_INET6		0xc4	/* IPng */

#define	ARCMTU			507
#define	ARCMIN			0

struct	arccom {
	struct 	  ifnet ac_if;		/* network-visible interface */
	u_int8_t  ac_anaddr;		/* arcnet hardware address */
	struct	  in_addr ac_ipaddr;	/* copy of ip address- XXX */

	u_int16_t ac_seqid;		/* seq. id used by PHDS encap. */

	struct ac_frag {
		u_int8_t  af_maxflag;	/* from first packet */
		u_int8_t  af_lastseen;	/* last split flag seen */
		u_int16_t af_seqid;
		struct mbuf *af_packet;
	} ac_fragtab[256];		/* indexed by sender ll address */

};

#ifdef _KERNEL
u_int8_t arcbroadcastaddr;

void	arc_ifattach(struct ifnet *);
char	*arc_sprintf(u_int8_t *);
void	arc_input(struct ifnet *, struct mbuf *);
int	arc_output(struct ifnet *, struct mbuf *, struct sockaddr *,
	    struct rtentry *);
int	arc_isphds(int);
#endif /* _KERNEL */
#endif /* _NETINET_IF_ARC_H_ */
