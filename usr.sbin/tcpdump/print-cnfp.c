/*	$OpenBSD: src/usr.sbin/tcpdump/print-cnfp.c,v 1.1 1998/06/25 19:42:46 mickey Exp $	*/

/*
 * Copyright (c) 1998 Michael Shalayeff
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Cisco NetFlow protocol */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <stdio.h>
#include <string.h>

#include "interface.h"

struct nfhdr {
	u_int32_t	ver_cnt;	/* version [15], and # of records */
	u_int32_t	msys_uptime;
	u_int32_t	utc_sec;
	u_int32_t	utc_nsec;
	u_int32_t	sequence;	/* v5 flow sequence number */
	u_int32_t	reserved;	/* v5 only */
};

struct nfrec {
	in_addr_t	src_ina;
	in_addr_t	dst_ina;
	in_addr_t	nhop_ina;
	u_int32_t	ifaces;		/* src,dst ifaces */
	u_int32_t	packets;
	u_int32_t	octets;
	u_int32_t	start_time;	/* sys_uptime value */
	u_int32_t	last_time;	/* sys_uptime value */
	u_int32_t	ports;		/* src,dst ports */
	u_int32_t	proto_tos;	/* proto, tos, pad, flags(v5) */
	u_int32_t	asses;		/* v1: flags; v5: src,dst AS */
	u_int32_t	masks;		/* src,dst addr prefix */

};

void
cnfp_print(register const u_char *cp, u_int len, register const u_char *bp)
{
	register const struct nfhdr *nh;
	register const struct nfrec *nr;
	register const struct ip *ip;
	struct protoent *pent;
	int nrecs, ver;
	time_t t;
	char *p;

	ip = (struct ip *)bp;
	nh = (struct nfhdr *)cp;

	nrecs = ntohl(nh->ver_cnt) & 0xffff;
	ver = (ntohl(nh->ver_cnt) & 0xffff0000) >> 16;
	t = ntohl(nh->utc_sec);
/*	(p = ctime(&t))[24] = '\0'; */

	printf("NetFlow v%x, %u.%03u uptime, %u.%09u, ", ver,
	       ntohl(nh->msys_uptime)/1000, ntohl(nh->msys_uptime)%1000,
	       ntohl(nh->utc_sec), ntohl(nh->utc_nsec));

	if (ver == 5) {
		printf("#%u, ", htonl(nh->sequence));
		nr = (struct nfrec *)&nh[1];
	} else
		nr = (struct nfrec *)&nh->sequence;

	printf("%2u recs:", nrecs);
	for (; nrecs--; nr++) {
		char buf[5];
		char asbuf[7];

		printf("\n  started %u.%03u, last %u.%03u",
			ntohl(nr->start_time)/1000, ntohl(nr->start_time)%1000,
			ntohl(nr->last_time)/1000, ntohl(nr->last_time)%1000);

		asbuf[0] = buf[0] = '\0';
		if (ver == 5) {
			sprintf(buf, "/%d", (ntohl(nr->masks) >> 24) & 0xff);
			sprintf(asbuf, "%d:", (ntohl(nr->asses) >> 16) & 0xffff);
		}
		printf("\n    %s%s%s:%u ", inet_ntoa(nr->src_ina), buf, asbuf,
			ntohl(nr->ports) >> 16);

		if (ver == 5) {
			sprintf(buf, "/%d", (ntohl(nr->masks) >> 16) & 0xff);
			sprintf(asbuf, "%d:", ntohl(nr->asses) & 0xffff);
		}
		printf("> %s%s%s:%u ", inet_ntoa(nr->dst_ina), buf, asbuf,
			ntohl(nr->ports) & 0xffff);

		printf(">> %s\n    ", inet_ntoa(nr->nhop_ina));

		pent = getprotobynumber((ntohl(nr->proto_tos) >> 8) & 0xff);
		if (!pent || nflag)
			printf("%u ", (ntohl(nr->proto_tos) >> 8) & 0xff);
		else
			printf("%s ", pent->p_name);

		/* tcp flags for tcp only */
		if (pent && pent->p_proto == IPPROTO_TCP) {
			int flags;
			if (ver == 1)
				flags = (ntohl(nr->asses) >> 24) & 0xff;
			else
				flags = (ntohl(nr->proto_tos) >> 16) & 0xff;
			if (flags & TH_FIN)	putchar('F');
			if (flags & TH_SYN)	putchar('S');
			if (flags & TH_RST)	putchar('R');
			if (flags & TH_PUSH)	putchar('P');
			if (flags & TH_ACK)	putchar('A');
			if (flags & TH_URG)	putchar('U');
			if (flags)
				putchar(' ');
		}
		printf("tos %u, %u (%u octets)", ntohl(nr->proto_tos) & 0xff,
			ntohl(nr->packets), ntohl(nr->octets));


	}

}

