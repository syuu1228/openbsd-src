/*	$OpenBSD: src/usr.sbin/tcpdump/print-mobile.c,v 1.1 2000/01/16 11:20:14 jakob Exp $ */
/*	$NetBSD: print-mobile.c,v 1.3 1999/07/26 06:11:57 itojun Exp $ */

/*
 * (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Heiko W.Rupp <hwr@pilhuhn.de>
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if 0
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: print-mobile.c,v 1.3 1999/07/26 06:11:57 itojun Exp $");
#endif
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <netdb.h>
#include <stdio.h>

#include "interface.h"
#include "addrtoname.h"
#include "extract.h"		/* must come after interface.h */

#define MOBILE_SIZE (8)

struct mobile_ip {
	u_int16_t proto;
	u_int16_t hcheck;
	u_int32_t odst;
	u_int32_t osrc;
};

#define OSRC_PRES	0x0080	/* old source is present */

static u_int16_t mob_in_cksum(u_short *p, int len);

/*
 * Deencapsulate and print a mobile-tunneled IP datagram
 */
void
mobile_print(const u_char *bp, u_int length)
{
	const u_char *cp = bp +8 ;
	const struct mobile_ip *mob;
	u_short proto,crc;
	u_char osp =0;			/* old source address present */

	mob = (const struct mobile_ip *)bp;

	if (length < MOBILE_SIZE) {
		fputs("[|mobile]", stdout);
		return;
	}

	proto = EXTRACT_16BITS(&mob->proto);
	crc =  EXTRACT_16BITS(&mob->hcheck);
	if (proto & OSRC_PRES) {
		osp=1;
		cp +=4 ;
	}
	
	if (osp)  {
		fputs("[S] ",stdout);
		if (vflag)
			(void)printf("%s ",ipaddr_string(&mob->osrc));
	} else {
		fputs("[] ",stdout);
	}
	if (vflag) {
		(void)printf("> %s ",ipaddr_string(&mob->odst));
		(void)printf("(oproto=%d)",proto>>8);
	}
	if (mob_in_cksum((u_short *)mob, osp ? 12 : 8)!=0) {
		(void)printf(" (bad checksum %d)",crc);
	}

	return;
}

static u_int16_t mob_in_cksum(u_short *p, int len)
{
	u_int32_t sum = 0; 
	int nwords = len >> 1;
  
	while (nwords-- != 0)
		sum += *p++;
  
	if (len & 1) {
		union {
			u_int16_t w;
			u_int32_t c[2]; 
		} u;
		u.c[0] = *(u_char *)p;
		u.c[1] = 0;
		sum += u.w;
	} 

	/* end-around-carry */
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return (~sum);
}

