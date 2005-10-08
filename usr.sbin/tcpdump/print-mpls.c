/*	$OpenBSD: src/usr.sbin/tcpdump/print-mpls.c,v 1.1 2005/10/08 19:45:15 canacar Exp $	*/

/*
 * Copyright (c) 2005 Jason L. Wright (jason@thought.net)
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>

#include "interface.h"
#include "extract.h"		    /* must come after interface.h */

void
mpls_print(const u_char *bp, u_int len)
{
	u_int32_t tag, label, exp, bottom, ttl;

 again:
	if (bp + sizeof(tag) > snapend)
		goto trunc;

	tag = EXTRACT_32BITS(bp);
	bp += sizeof(tag);
	len -= sizeof(tag);

	label = (tag >> 12) & 0xfffff;
	exp = (tag >> 9) & 0x7;
	bottom = (tag >> 8) & 0x1;
	ttl = (tag >> 0) & 0xff;

	printf("MPLS(label 0x%x, exp %u, ttl %u) ", label, exp, ttl);

	/* XXX decode "Router Alert Label" */

	if (!bottom)
		goto again;

	/*
	 * guessing the underlying protocol is about all we can do if
	 * it's not explicitly defined.
	 */

	switch (label) {
	case 0x00000:			/* IPv4 Explicit NULL */
		ip_print(bp, len);
		break;
	case 0x00001:			/* Router Alert */
		/* shouldn't happen at stack bottom */
		printf("Route-Alert");
		break;
	case 0x00002:			/* IPv6 Explicit NULL */
		ip6_print(bp, len);
		break;
	case 0x00003:			/* Implicit NULL */
		/* shouldn't happen in the tag stack */
		printf("Implicit-NULL");
		break;

	case 0x00004:			/* reserved labels */
	case 0x00005:
	case 0x00006:
	case 0x00007:
	case 0x00008:
	case 0x00009:
	case 0x0000a:
	case 0x0000b:
	case 0x0000c:
	case 0x0000d:
	case 0x0000e:
	case 0x0000f:
		break;

	default:			/* dunno, guess? */
		if (len == 0)
			break;
		if (bp >= snapend)
			goto trunc;

		switch (bp[0] & 0xf0) {
		case 0x40:
			ip_print(bp, len);
			break;
		case 0x60:
			ip6_print(bp, len);
			break;
		}
	}

	return;
trunc:
	printf("[|mpls]");
}
