/*	$OpenBSD: src/usr.sbin/afs/src/lib/ko/ports.c,v 1.1.1.1 1998/09/14 21:53:00 art Exp $	*/
/*
 * Copyright (c) 1995, 1996, 1997, 1998 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
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
 *      This product includes software developed by the Kungliga Tekniska
 *      H�gskolan and its contributors.
 * 
 * 4. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Take care of the port stuff.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <roken.h>
#include <stdio.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

RCSID("$KTH: ports.c,v 1.5 1998/06/07 05:44:38 map Exp $") ;

typedef struct {
     const char *name;		/* Name of the service */
     const char *proto;		/* Protocol */
     int *port;			/* Variable with port-value */
     int defport;		/* Default port */
} Port;

int afsport, afscallbackport, afsprport, afsvldbport,
   afskaport, afsvolport, afserrorsport, afsbosport,
   afsupdateport, afsrmtsys ;

Port ports[] = {
{"afs3-fileserver",	"udp", &afsport,	7000},
{"afs3-callback",       "udp", &afscallbackport,7001},
{"afs3-prserver",       "udp", &afsprport,      7002},
{"afs3-vlserver",	"udp", &afsvldbport,	7003},
{"afs3-kaserver",       "udp", &afskaport,      7004},
{"afs3-volser",         "udp", &afsvolport,     7005},
{"afs3-errors",         "udp", &afserrorsport,  7006},
{"afs3-bos",            "udp", &afsbosport,     7007},
{"afs3-update",         "udp", &afsupdateport,  7008},
{"afs3-rmtsys",         "udp", &afsrmtsys,      7009}
};

/*
 * Find all ports and set their values.
 */

void
initports (void)
{
     int i;

     for (i = 0; i < sizeof (ports) / sizeof (*ports); ++i) {
	  struct servent *service;

	  service = getservbyname (ports[i].name, ports[i].proto);
	  if (service == NULL) {

	       fprintf (stderr,
			"Unable to find service %s/%s, using port %d\n",
			ports[i].name, ports[i].proto, ports[i].defport);
	       *(ports[i].port) = ports[i].defport;
	  } else
	       *(ports[i].port) = ntohs (service->s_port);
     }
}
