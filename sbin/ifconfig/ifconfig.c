/*	$OpenBSD: src/sbin/ifconfig/ifconfig.c,v 1.70 2002/11/23 15:08:50 henning Exp $	*/
/*	$NetBSD: ifconfig.c,v 1.40 1997/10/01 02:19:43 enami Exp $	*/

/*
 * Copyright (c) 1983, 1993
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
 */

/*-
 * Copyright (c) 1997, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static const char sccsid[] = "@(#)ifconfig.c	8.2 (Berkeley) 2/16/94";
#else
static const char rcsid[] = "$OpenBSD: src/sbin/ifconfig/ifconfig.c,v 1.70 2002/11/23 15:08:50 henning Exp $";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet6/nd6.h>
#include <arpa/inet.h>
#include <netinet/ip_ipsp.h>
#include <netinet/if_ether.h>
#include <net/if_enc.h>
#include <net/if_ieee80211.h>

#include <netatalk/at.h>

#define	NSIP
#include <netns/ns.h>
#include <netns/ns_if.h>

#define	IPXIP
#include <netipx/ipx.h>
#include <netipx/ipx_if.h>

#include <netdb.h>

#define EON
#include <netiso/iso.h>
#include <netiso/iso_var.h>
#include <sys/protosw.h>

#ifndef INET_ONLY
#include <net/if_vlan_var.h>
#endif

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

struct	ifreq		ifr, ridreq;
struct	ifaliasreq	addreq;
struct	in_aliasreq	in_addreq;
#ifdef INET6
struct	in6_ifreq	ifr6;
struct	in6_ifreq	in6_ridreq;
struct	in6_aliasreq	in6_addreq;
#endif
struct	iso_ifreq	iso_ridreq;
struct	iso_aliasreq	iso_addreq;
struct	sockaddr_in	netmask;
struct  netrange	at_nr;		/* AppleTalk net range */

int	ipx_type = ETHERTYPE_II;
char	name[IFNAMSIZ];
int	flags, setaddr, setipdst, doalias;
u_long	metric, mtu;
int	clearaddr, s;
int	newaddr = 0;
int	nsellength = 1;
int	af = AF_INET;
int	dflag, mflag, lflag, uflag;
int	reset_if_flags;
int	explicit_prefix = 0;
#ifdef INET6
int	Lflag = 1;
#endif

void	notealias(char *, int);
void	notrailers(char *, int);
void	setifaddr(char *, int);
void	setifdstaddr(char *, int);
void	setifflags(char *, int);
void	setifbroadaddr(char *);
void	setifipdst(char *);
void	setifmetric(char *);
void	setifmtu(char *, int);
void	setifnwid(const char *, int);
void	setifnwkey(const char *, int);
void	setifpowersave(const char *, int);
void	setifpowersavesleep(const char *, int);
void	setifnetmask(char *);
void	setifprefixlen(char *, int);
void	setnsellength(char *);
void	setsnpaoffset(char *);
void	setipxframetype(char *, int);
void    setatrange(char *, int);
void    setatphase(char *, int);
void    settunnel(char *, char *);
void    deletetunnel(void);
#ifdef INET6
void	setia6flags(char *, int);
void	setia6pltime(char *, int);
void	setia6vltime(char *, int);
void	setia6lifetime(char *, char *);
void	setia6eui64(const char *, int);
#endif
void    checkatrange(struct sockaddr_at *);
void	setmedia(char *, int);
void	setmediaopt(char *, int);
void	unsetmediaopt(char *, int);
void	setmediainst(char *, int);
void	setvlantag(char *, int);
void	setvlandev(char *, int);
void	unsetvlandev(char *, int);
void	vlan_status ();
void	fixnsel(struct sockaddr_iso *);
int	main(int, char *[]);
int	prefix(void *val, int);

/*
 * Media stuff.  Whenever a media command is first performed, the
 * currently select media is grabbed for this interface.  If `media'
 * is given, the current media word is modifed.  `mediaopt' commands
 * only modify the set and clear words.  They then operate on the
 * current media word later.
 */
int	media_current;
int	mediaopt_set;
int	mediaopt_clear;

int	actions;			/* Actions performed */

#define	A_MEDIA		0x0001		/* media command */
#define	A_MEDIAOPTSET	0x0002		/* mediaopt command */
#define	A_MEDIAOPTCLR	0x0004		/* -mediaopt command */
#define	A_MEDIAOPT	(A_MEDIAOPTSET|A_MEDIAOPTCLR)
#define	A_MEDIAINST	0x0008		/* instance or inst command */

#define	NEXTARG		0xffffff
#define NEXTARG2	0xfffffe

const struct	cmd {
	char	*c_name;
	int	c_parameter;		/* NEXTARG means next argv */
	int	c_action;		/* defered action */
	void	(*c_func)();
} cmds[] = {
	{ "up",		IFF_UP,		0,		setifflags } ,
	{ "down",	-IFF_UP,	0,		setifflags },
	{ "trailers",	-1,		0,		notrailers },
	{ "-trailers",	1,		0,		notrailers },
	{ "arp",	-IFF_NOARP,	0,		setifflags },
	{ "-arp",	IFF_NOARP,	0,		setifflags },
	{ "debug",	IFF_DEBUG,	0,		setifflags },
	{ "-debug",	-IFF_DEBUG,	0,		setifflags },
	{ "alias",	IFF_UP,		0,		notealias },
	{ "-alias",	-IFF_UP,	0,		notealias },
	{ "delete",	-IFF_UP,	0,		notealias },
#ifdef notdef
#define	EN_SWABIPS	0x1000
	{ "swabips",	EN_SWABIPS,	0,		setifflags },
	{ "-swabips",	-EN_SWABIPS,	0,		setifflags },
#endif
	{ "netmask",	NEXTARG,	0,		setifnetmask },
	{ "metric",	NEXTARG,	0,		setifmetric },
	{ "mtu",	NEXTARG,	0,		setifmtu },
	{ "nwid",	NEXTARG,	0,		setifnwid },
	{ "nwkey",	NEXTARG,	0,		setifnwkey },
	{ "-nwkey",	-1,		0,		setifnwkey },
	{ "powersave",	1,		0,		setifpowersave },
	{ "-powersave",	0,		0,		setifpowersave },
	{ "powersavesleep", NEXTARG,	0,		setifpowersavesleep },
	{ "broadcast",	NEXTARG,	0,		setifbroadaddr },
	{ "ipdst",	NEXTARG,	0,		setifipdst },
	{ "prefixlen",  NEXTARG,	0,		setifprefixlen},
#ifdef INET6
	{ "anycast",	IN6_IFF_ANYCAST,	0,	setia6flags },
	{ "-anycast",	-IN6_IFF_ANYCAST,	0,	setia6flags },
	{ "tentative",	IN6_IFF_TENTATIVE,	0,	setia6flags },
	{ "-tentative",	-IN6_IFF_TENTATIVE,	0,	setia6flags },
	{ "pltime",	NEXTARG,	0,		setia6pltime },
	{ "vltime",	NEXTARG,	0,		setia6vltime },
	{ "eui64",	0,		0,		setia6eui64 },
#endif /*INET6*/
#ifndef INET_ONLY
	{ "range",	NEXTARG,	0,		setatrange },
	{ "phase",	NEXTARG,	0,		setatphase },
	{ "snpaoffset",	NEXTARG,	0,		setsnpaoffset },
	{ "nsellength",	NEXTARG,	0,		setnsellength },
	{ "802.2",	ETHERTYPE_8022,	0,		setipxframetype },
	{ "802.2tr",	ETHERTYPE_8022TR, 0,		setipxframetype },
	{ "802.3",	ETHERTYPE_8023,	0,		setipxframetype },
	{ "snap",	ETHERTYPE_SNAP,	0,		setipxframetype },
	{ "EtherII",	ETHERTYPE_II,	0,		setipxframetype },
	{ "vlan",	NEXTARG,	0,		setvlantag },
	{ "vlandev",	NEXTARG,	0,		setvlandev },
	{ "-vlandev",	1,		0,		unsetvlandev },
#endif	/* INET_ONLY */
	/* giftunnel is for backward compat */
	{ "giftunnel",  NEXTARG2,	0,		settunnel } ,
	{ "tunnel",	NEXTARG2,	0,		settunnel } ,
	{ "deletetunnel",  0,   	0,		deletetunnel } ,
	{ "link0",	IFF_LINK0,	0,		setifflags } ,
	{ "-link0",	-IFF_LINK0,	0,		setifflags } ,
	{ "link1",	IFF_LINK1,	0,		setifflags } ,
	{ "-link1",	-IFF_LINK1,	0,		setifflags } ,
	{ "link2",	IFF_LINK2,	0,		setifflags } ,
	{ "-link2",	-IFF_LINK2,	0,		setifflags } ,
	{ "media",	NEXTARG,	A_MEDIA,	setmedia },
	{ "mediaopt",	NEXTARG,	A_MEDIAOPTSET,	setmediaopt },
	{ "-mediaopt",	NEXTARG,	A_MEDIAOPTCLR,	unsetmediaopt },
	{ "instance",	NEXTARG,	A_MEDIAINST,	setmediainst },
	{ "inst",	NEXTARG,	A_MEDIAINST,	setmediainst },
	{ NULL, /*src*/	0,		0,		setifaddr },
	{ NULL, /*dst*/	0,		0,		setifdstaddr },
	{ NULL, /*illegal*/0,		0,		NULL },
};

void	adjust_nsellength();
int	getinfo(struct ifreq *);
void	getsock(int);
void	printif(struct ifreq *, int);
void	printb(char *, unsigned short, char *);
void	status(int, struct sockaddr_dl *);
void	usage();
const char *get_string(const char *, const char *, u_int8_t *, int *);
void	print_string(const u_int8_t *, int);
char	*sec2str(time_t);

const char *get_media_type_string(int);
const char *get_media_subtype_string(int);
int	get_media_subtype(int, const char *);
int	get_media_options(int, const char *);
int	lookup_media_word(const struct ifmedia_description *, int,
	    const char *);
void	print_media_word(int, int, int);
void	process_media_commands(void);
void	init_current_media(void);

/*
 * XNS support liberally adapted from code written at the University of
 * Maryland principally by James O'Toole and Chris Torek.
 */
void	in_status(int);
void	in_getaddr(char *, int);
void	in_getprefix(char *, int);
#ifdef INET6
void	in6_fillscopeid(struct sockaddr_in6 *sin6);
void	in6_alias(struct in6_ifreq *);
void	in6_status(int);
void	in6_getaddr(char *, int);
void	in6_getprefix(char *, int);
#endif
void    at_status(int);
void    at_getaddr(char *, int);
void	xns_status(int);
void	xns_getaddr(char *, int);
void	ipx_status(int);
void	ipx_getaddr(char *, int);
void	iso_status(int);
void	iso_getaddr(char *, int);
void	ieee80211_status(void);

/* Known address families */
const struct afswtch {
	char *af_name;
	short af_af;
	void (*af_status)();
	void (*af_getaddr)();
	void (*af_getprefix)();
	u_long af_difaddr;
	u_long af_aifaddr;
	caddr_t af_ridreq;
	caddr_t af_addreq;
} afs[] = {
#define C(x) ((caddr_t) &x)
	{ "inet", AF_INET, in_status, in_getaddr, in_getprefix,
	     SIOCDIFADDR, SIOCAIFADDR, C(ridreq), C(in_addreq) },
#ifdef INET6
	{ "inet6", AF_INET6, in6_status, in6_getaddr, in6_getprefix,
	     SIOCDIFADDR_IN6, SIOCAIFADDR_IN6, C(in6_ridreq), C(in6_addreq) },
#endif
#ifndef INET_ONLY	/* small version, for boot media */
	{ "atalk", AF_APPLETALK, at_status, at_getaddr, NULL,
	    SIOCDIFADDR, SIOCAIFADDR, C(addreq), C(addreq) },
	{ "ns", AF_NS, xns_status, xns_getaddr, NULL,
	     SIOCDIFADDR, SIOCAIFADDR, C(ridreq), C(addreq) },
	{ "ipx", AF_IPX, ipx_status, ipx_getaddr, NULL,
	     SIOCDIFADDR, SIOCAIFADDR, C(ridreq), C(addreq) },
	{ "iso", AF_ISO, iso_status, iso_getaddr, NULL,
	     SIOCDIFADDR_ISO, SIOCAIFADDR_ISO, C(iso_ridreq), C(iso_addreq) },
#endif	/* INET_ONLY */
	{ 0,	0,	    0,		0 }
};

const struct afswtch *afp;	/*the address family being set or asked about*/

int
main(argc, argv)
	int argc;
	char *argv[];
{
	const struct afswtch *rafp = NULL;
	int aflag = 0;
	int ifaliases = 0;
	int i;

	if (argc < 2)
		usage();
	argc--, argv++;
	if (!strcmp(*argv, "-a"))
		aflag = 1;
	else if (!strcmp(*argv, "-A")) {
		aflag = 1;
		ifaliases = 1;
	}
	else if (!strcmp(*argv, "-ma") || !strcmp(*argv, "-am")) {
		aflag = 1;
		mflag = 1;
	}
	else if (!strcmp(*argv, "-mA") || !strcmp(*argv, "-Am")) {
		aflag = 1;
		ifaliases = 1;
		mflag = 1;
	}
	else if (!strcmp(*argv, "-m")) {
		mflag = 1;
		argc--, argv++;
		if (argc < 1)
			usage();
		(void) strlcpy(name, *argv, sizeof(name));
	} else
		(void) strlcpy(name, *argv, sizeof(name));
	argc--, argv++;
	if (argc > 0) {
		for (afp = rafp = afs; rafp->af_name; rafp++)
			if (strcmp(rafp->af_name, *argv) == 0) {
				afp = rafp; argc--; argv++;
				break;
			}
		rafp = afp;
		af = ifr.ifr_addr.sa_family = rafp->af_af;
	}
	if (aflag) {
		if (argc > 0)
			usage();
		printif(NULL, ifaliases);
		exit(0);
	}
	(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (argc == 0) {
		printif(&ifr, 1);
		exit(0);
	}

#ifdef INET6
	/* initialization */
	in6_addreq.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;
	in6_addreq.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
#endif

	if (getinfo(&ifr) < 0)
		exit(1);
	while (argc > 0) {
		const struct cmd *p;

		for (p = cmds; p->c_name; p++)
			if (strcmp(*argv, p->c_name) == 0)
				break;
		if (p->c_name == 0 && setaddr)
			for (i = setaddr; i > 0; i--) {
				p++;
				if (p->c_func == NULL)
					errx(1, "extra address not accepted");
			}
		if (p->c_func) {
			if (p->c_parameter == NEXTARG) {
				if (argv[1] == NULL)
					errx(1, "'%s' requires argument",
					    p->c_name);
				(*p->c_func)(argv[1]);
				argc--, argv++;
			} else if (p->c_parameter == NEXTARG2) {
				if ((argv[1] == NULL) ||
				    (argv[2] == NULL))
					errx(1, "'%s' requires 2 arguments",
					    p->c_name);
				(*p->c_func)(argv[1], argv[2]);
				argc -= 2;
				argv += 2;
			} else
				(*p->c_func)(*argv, p->c_parameter);
			actions |= p->c_action;
		}
		argc--, argv++;
	}

	/* Process any media commands that may have been issued. */
	process_media_commands();

	if (af == AF_INET6 && explicit_prefix == 0) {
		/*
		 * Aggregatable address architecture defines all prefixes
		 * are 64. So, it is convenient to set prefixlen to 64 if
		 * it is not specified.
		 */
		setifprefixlen("64", 0);
		/* in6_getprefix("64", MASK) if MASK is available here... */
	}

#ifndef INET_ONLY

	switch (af) {
	case AF_ISO:
		adjust_nsellength();
		break;
	case AF_NS:
		if (setipdst) {
			struct nsip_req rq;
			int size = sizeof(rq);

			rq.rq_ns = addreq.ifra_addr;
			rq.rq_ip = addreq.ifra_dstaddr;

			if (setsockopt(s, 0, SO_NSIP_ROUTE, &rq, size) < 0)
				warn("encapsulation routing");
		}
		break;
	case AF_IPX:
		if (setipdst) {
			struct ipxip_req rq;
			int size = sizeof(rq);

			rq.rq_ipx = addreq.ifra_addr;
			rq.rq_ip = addreq.ifra_dstaddr;

			if (setsockopt(s, 0, SO_IPXIP_ROUTE, &rq, size) < 0)
				warn("encapsulation routing");
		}
		break;
	case AF_APPLETALK:
		checkatrange((struct sockaddr_at *) &addreq.ifra_addr);
		break;
	}
#endif	/* INET_ONLY */

	if (clearaddr) {
		int ret;
		(void) strlcpy(rafp->af_ridreq, name, sizeof(ifr.ifr_name));
		if ((ret = ioctl(s, rafp->af_difaddr, rafp->af_ridreq)) < 0) {
			if (errno == EADDRNOTAVAIL && (doalias >= 0)) {
				/* means no previous address for interface */
			} else
				warn("SIOCDIFADDR");
		}
	}
	if (newaddr) {
		(void) strlcpy(rafp->af_addreq, name, sizeof(ifr.ifr_name));
		if (ioctl(s, rafp->af_aifaddr, rafp->af_addreq) < 0)
			warn("SIOCAIFADDR");
	}
	exit(0);
}

void
getsock(naf)
	int naf;
{
	static int oaf = -1;

	if (oaf == naf)
		return;
	if (oaf != -1)
		close(s);
	s = socket(naf, SOCK_DGRAM, 0);
	if (s < 0)
		oaf = -1;
	else
		oaf = naf;
}

int
getinfo(ifr)
	struct ifreq *ifr;
{

	getsock(af);
	if (s < 0)
		err(1, "socket");
	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)ifr) < 0) {
		warn("SIOCGIFFLAGS");
		return (-1);
	}
	flags = ifr->ifr_flags;
	if (ioctl(s, SIOCGIFMETRIC, (caddr_t)ifr) < 0) {
		warn("SIOCGIFMETRIC");
		metric = 0;
	} else
		metric = ifr->ifr_metric;
	if (ioctl(s, SIOCGIFMTU, (caddr_t)ifr) < 0)
		mtu = 0;
	else
		mtu = ifr->ifr_mtu;
	return (0);
}

void
printif(ifrm, ifaliases)
	struct ifreq *ifrm;
{
#ifdef HAVE_IFADDRS_H
	struct ifaddrs *ifap, *ifa;
	const char *namep;
	struct ifreq *ifrp;
	int count = 0, noinet = 1;

	if (getifaddrs(&ifap) != 0)
		err(1, "getifaddrs");

	namep = NULL;
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifrm && strncmp(ifrm->ifr_name, ifa->ifa_name,
		    sizeof(ifrm->ifr_name)))
			continue;
		(void) strlcpy(name, ifa->ifa_name, sizeof(name));
		name[sizeof(name) - 1] = '\0';

#ifdef INET6
		/* quickhack: sizeof(ifr) < sizeof(ifr6) */
		if (ifa->ifa_addr->sa_family == AF_INET6) {
			ifrp = (struct ifreq *)&ifr6;
			memset(&ifr6, 0, sizeof(ifr6));
		} else {
			ifrp = &ifr;
			memset(&ifr, 0, sizeof(ifr));
		}
#else
		ifrp = &ifr;
		memset(&ifr, 0, sizeof(ifr));
#endif

		strlcpy(ifrp->ifr_name, ifa->ifa_name, sizeof(ifrp->ifr_name));
		/* XXX boundary check? */
		memcpy(&ifrp->ifr_addr, ifa->ifa_addr, ifa->ifa_addr->sa_len);

		if (ifa->ifa_addr->sa_family == AF_LINK) {
			namep = ifa->ifa_name;
			if (getinfo(ifrp) < 0)
				continue;
			status(1, (struct sockaddr_dl *)ifa->ifa_addr);
			count++;
			noinet = 1;
			continue;
		}

		if (!namep || !strcmp(namep, ifa->ifa_name)) {
			const struct afswtch *p;

			if (ifa->ifa_addr->sa_family == AF_INET &&
			    ifaliases == 0 && noinet == 0)
				continue;
			if ((p = afp) != NULL) {
				if (ifa->ifa_addr->sa_family == p->af_af)
					(*p->af_status)(1);
			} else {
				for (p = afs; p->af_name; p++) {
					if (ifa->ifa_addr->sa_family == p->af_af)
						(*p->af_status)(0);
				}
			}
			count++;
			if (ifa->ifa_addr->sa_family == AF_INET)
				noinet = 0;
			continue;
		}
	}
	freeifaddrs(ifap);
	if (count == 0) {
		fprintf(stderr, "%s: no such interface\n", name);
		exit(1);
	}
#else
	char *inbuf = NULL;
	struct ifconf ifc;
	struct ifreq ifreq, *ifrp;
	int i, siz, len = 8192;
	int count = 0, noinet = 1;
	char ifrbuf[8192];

	getsock(af);
	if (s < 0)
		err(1, "socket");
	while (1) {
		ifc.ifc_len = len;
		ifc.ifc_buf = inbuf = realloc(inbuf, len);
		if (inbuf == NULL)
			err(1, "malloc");
		if (ioctl(s, SIOCGIFCONF, &ifc) < 0)
			err(1, "SIOCGIFCONF");
		if (ifc.ifc_len + sizeof(ifreq) < len)
			break;
		len *= 2;
	}
	ifrp = ifc.ifc_req;
	ifreq.ifr_name[0] = '\0';
	for (i = 0; i < ifc.ifc_len; ) {
		ifrp = (struct ifreq *)((caddr_t)ifc.ifc_req + i);
		memcpy(ifrbuf, ifrp, sizeof(*ifrp));
		siz = ((struct ifreq *)ifrbuf)->ifr_addr.sa_len;
		if (siz < sizeof(ifrp->ifr_addr))
			siz = sizeof(ifrp->ifr_addr);
		siz += sizeof(ifrp->ifr_name);
		i += siz;
		/* avoid alignment issue */
		if (sizeof(ifrbuf) < siz)
			errx(1, "ifr too big");
		memcpy(ifrbuf, ifrp, siz);
		ifrp = (struct ifreq *)ifrbuf;

		if (ifrm && strncmp(ifrm->ifr_name, ifrp->ifr_name,
		    sizeof(ifrp->ifr_name)))
			continue;
		(void) strlcpy(name, ifrp->ifr_name, sizeof(ifrp->ifr_name));
		if (ifrp->ifr_addr.sa_family == AF_LINK) {
			ifreq = ifr = *ifrp;
			if (getinfo(&ifreq) < 0)
				continue;
			status(1, NULL);
			count++;
			noinet = 1;
			continue;
		}
		if (!strncmp(ifreq.ifr_name, ifrp->ifr_name,
		    sizeof(ifrp->ifr_name))) {
			const struct afswtch *p;

			if (ifrp->ifr_addr.sa_family == AF_INET &&
			    ifaliases == 0 && noinet == 0)
				continue;
			ifr = *ifrp;
#ifdef INET6
			/* quickhack: sizeof(ifr) < sizeof(ifr6) */
			if (ifrp->ifr_addr.sa_family == AF_INET6)
				bcopy(ifrp, &ifr6, sizeof(ifr6));
#endif
			if ((p = afp) != NULL) {
				if (ifr.ifr_addr.sa_family == p->af_af)
					(*p->af_status)(1);
			} else {
				for (p = afs; p->af_name; p++) {
					if (ifr.ifr_addr.sa_family == p->af_af)
						(*p->af_status)(0);
				}
			}
			count++;
			if (ifrp->ifr_addr.sa_family == AF_INET)
				noinet = 0;
			continue;
		}
	}
	free(inbuf);
	if (count == 0) {
		fprintf(stderr, "%s: no such interface\n", name);
		exit(1);
	}
#endif
}

#define RIDADDR 0
#define ADDR	1
#define MASK	2
#define DSTADDR	3

/*ARGSUSED*/
void
setifaddr(addr, param)
	char *addr;
	int param;
{
	/*
	 * Delay the ioctl to set the interface addr until flags are all set.
	 * The address interpretation may depend on the flags,
	 * and the flags may change when the address is set.
	 */
	setaddr++;
	newaddr = 1;
	if (doalias == 0)
		clearaddr = 1;
	(*afp->af_getaddr)(addr, (doalias >= 0 ? ADDR : RIDADDR));
}

void
settunnel(src, dst)
	char *src;
	char *dst;
{
	struct addrinfo hints, *srcres, *dstres;
	int ecode;
	struct if_laddrreq req;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = afp->af_af;
	hints.ai_socktype = SOCK_DGRAM;	/*dummy*/

	if ((ecode = getaddrinfo(src, NULL, &hints, &srcres)) != 0)
		errx(1, "error in parsing address string: %s",
		    gai_strerror(ecode));

	if ((ecode = getaddrinfo(dst, NULL, &hints, &dstres)) != 0)
		errx(1, "error in parsing address string: %s",
		    gai_strerror(ecode));

	if (srcres->ai_addr->sa_family != dstres->ai_addr->sa_family)
		errx(1,
		    "source and destination address families do not match");

	if (srcres->ai_addrlen > sizeof(req.addr) ||
	    dstres->ai_addrlen > sizeof(req.dstaddr))
		errx(1, "invalid sockaddr");

	memset(&req, 0, sizeof(req));
	(void) strlcpy(req.iflr_name, name, sizeof(req.iflr_name));
	memcpy(&req.addr, srcres->ai_addr, srcres->ai_addrlen);
	memcpy(&req.dstaddr, dstres->ai_addr, dstres->ai_addrlen);
	if (ioctl(s, SIOCSLIFPHYADDR, &req) < 0)
		warn("SIOCSLIFPHYADDR");

	freeaddrinfo(srcres);
	freeaddrinfo(dstres);
}

void
deletetunnel()
{
	if (ioctl(s, SIOCDIFPHYADDR, &ifr) < 0)
		warn("SIOCDIFPHYADDR");
}


void
setifnetmask(addr)
	char *addr;
{
	(*afp->af_getaddr)(addr, MASK);
}

void
setifbroadaddr(addr)
	char *addr;
{
	(*afp->af_getaddr)(addr, DSTADDR);
}

void
setifipdst(addr)
	char *addr;
{
	in_getaddr(addr, DSTADDR);
	setipdst++;
	clearaddr = 0;
	newaddr = 0;
}

#define rqtosa(x) (&(((struct ifreq *)(afp->x))->ifr_addr))
/*ARGSUSED*/
void
notealias(addr, param)
	char *addr;
	int param;
{
	if (setaddr && doalias == 0 && param < 0)
		memcpy(rqtosa(af_ridreq), rqtosa(af_addreq),
		    rqtosa(af_addreq)->sa_len);
	doalias = param;
	if (param < 0) {
		clearaddr = 1;
		newaddr = 0;
	} else
		clearaddr = 0;
}

/*ARGSUSED*/
void
notrailers(vname, value)
	char *vname;
	int value;
{
	printf("Note: trailers are no longer sent, but always received\n");
}

/*ARGSUSED*/
void
setifdstaddr(addr, param)
	char *addr;
	int param;
{
	setaddr++;
	(*afp->af_getaddr)(addr, DSTADDR);
}

/*
 * Note: doing an SIOCIGIFFLAGS scribbles on the union portion
 * of the ifreq structure, which may confuse other parts of ifconfig.
 * Make a private copy so we can avoid that.
 */
void
setifflags(vname, value)
	char *vname;
	int value;
{
	struct ifreq my_ifr;

	bcopy((char *)&ifr, (char *)&my_ifr, sizeof(struct ifreq));

	if (ioctl(s, SIOCGIFFLAGS, (caddr_t)&my_ifr) < 0)
		err(1, "SIOCGIFFLAGS");
	(void) strlcpy(my_ifr.ifr_name, name, sizeof(my_ifr.ifr_name));
	flags = my_ifr.ifr_flags;

	if (value < 0) {
		value = -value;
		flags &= ~value;
	} else
		flags |= value;
	my_ifr.ifr_flags = flags;
	if (ioctl(s, SIOCSIFFLAGS, (caddr_t)&my_ifr) < 0)
		err(1, "SIOCSIFFLAGS");
}

#ifdef INET6
void
setia6flags(vname, value)
	char *vname;
	int value;
{

	if (value < 0) {
		value = -value;
		in6_addreq.ifra_flags &= ~value;
	} else
		in6_addreq.ifra_flags |= value;
}

void
setia6pltime(val, d)
	char *val;
	int d;
{

	setia6lifetime("pltime", val);
}

void
setia6vltime(val, d)
	char *val;
	int d;
{

	setia6lifetime("vltime", val);
}

void
setia6lifetime(cmd, val)
	char *cmd;
	char *val;
{
	time_t newval, t;
	char *ep;

	t = time(NULL);
	newval = (time_t)strtoul(val, &ep, 0);
	if (val == ep)
		errx(1, "invalid %s", cmd);
	if (afp->af_af != AF_INET6)
		errx(1, "%s not allowed for the AF", cmd);
	if (strcmp(cmd, "vltime") == 0) {
		in6_addreq.ifra_lifetime.ia6t_expire = t + newval;
		in6_addreq.ifra_lifetime.ia6t_vltime = newval;
	} else if (strcmp(cmd, "pltime") == 0) {
		in6_addreq.ifra_lifetime.ia6t_preferred = t + newval;
		in6_addreq.ifra_lifetime.ia6t_pltime = newval;
	}
}

void
setia6eui64(cmd, val)
	const char *cmd;
	int val;
{
	struct ifaddrs *ifap, *ifa;
	const struct sockaddr_in6 *sin6 = NULL;
	const struct in6_addr *lladdr = NULL;
	struct in6_addr *in6;

	if (afp->af_af != AF_INET6)
		errx(1, "%s not allowed for the AF", cmd);
 	in6 = (struct in6_addr *)&in6_addreq.ifra_addr.sin6_addr;
	if (memcmp(&in6addr_any.s6_addr[8], &in6->s6_addr[8], 8) != 0)
		errx(1, "interface index is already filled");
	if (getifaddrs(&ifap) != 0)
		err(1, "getifaddrs");
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET6 &&
		    strcmp(ifa->ifa_name, name) == 0) {
			sin6 = (const struct sockaddr_in6 *)ifa->ifa_addr;
			if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
				lladdr = &sin6->sin6_addr;
				break;
			}
		}
	}
	if (!lladdr)
		errx(1, "could not determine link local address"); 

 	memcpy(&in6->s6_addr[8], &lladdr->s6_addr[8], 8);

	freeifaddrs(ifap);
}
#endif

void
setifmetric(val)
	char *val;
{
	char *ep = NULL;

	(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_metric = strtoul(val, &ep, 10);
	if (!ep || *ep)
		errx(1, "%s: invalid metric", val);
	if (ioctl(s, SIOCSIFMETRIC, (caddr_t)&ifr) < 0)
		warn("SIOCSIFMETRIC");
}

void
setifmtu(val, d)
	char *val;
	int d;
{
	char *ep = NULL;

	(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_mtu = strtoul(val, &ep, 10);
	if (!ep || *ep)
		errx(1, "%s: invalid mtu", val);
	if (ioctl(s, SIOCSIFMTU, (caddr_t)&ifr) < 0)
		warn("SIOCSIFMTU");
}

const char *
get_string(val, sep, buf, lenp)
	const char *val, *sep;
	u_int8_t *buf;
	int *lenp;
{
	int len;
	int hexstr;
	u_int8_t *p;

	len = *lenp;
	p = buf;
	hexstr = (val[0] == '0' && tolower((u_char)val[1]) == 'x');
	if (hexstr)
		val += 2;
	for (;;) {
		if (*val == '\0')
			break;
		if (sep != NULL && strchr(sep, *val) != NULL) {
			val++;
			break;
		}
		if (hexstr) {
			if (!isxdigit((u_char)val[0]) ||
			    !isxdigit((u_char)val[1])) {
				warnx("bad hexadecimal digits");
				return NULL;
			}
		}
		if (p > buf + len) {
			if (hexstr)
				warnx("hexadecimal digits too long");
			else
				warnx("strings too long");
			return NULL;
		}
		if (hexstr) {
#define	tohex(x)	(isdigit(x) ? (x) - '0' : tolower(x) - 'a' + 10)
			*p++ = (tohex((u_char)val[0]) << 4) |
			    tohex((u_char)val[1]);
#undef tohex
			val += 2;
		} else {
			if (*val == '\\' &&
			    sep != NULL && strchr(sep, *(val + 1)) != NULL)
				val++;
			*p++ = *val++;
		}
	}
	len = p - buf;
	if (len < *lenp)
		memset(p, 0, *lenp - len);
	*lenp = len;
	return val;
}

void
print_string(buf, len)
	const u_int8_t *buf;
	int len;
{
	int i;
	int hasspc;

	i = 0;
	hasspc = 0;
	if (len < 2 || buf[0] != '0' || tolower(buf[1]) != 'x') {
		for (; i < len; i++) {
			/* Only print 7-bit ASCII keys */
			if (buf[i] & 0x80 || !isprint(buf[i]))
				break;
			if (isspace(buf[i]))
				hasspc++;
		}
	}
	if (i == len) {
		if (hasspc || len == 0)
			printf("\"%.*s\"", len, buf);
		else
			printf("%.*s", len, buf);
	} else {
		printf("0x");
		for (i = 0; i < len; i++)
			printf("%02x", buf[i]);
	}
}

void
setifnwid(val, d)
	const char *val;
	int d;
{
	struct ieee80211_nwid nwid;
	int len;

	len = sizeof(nwid.i_nwid);
	if (get_string(val, NULL, nwid.i_nwid, &len) == NULL)
		return;
	nwid.i_len = len;
	(void)strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_data = (caddr_t)&nwid;
	if (ioctl(s, SIOCS80211NWID, (caddr_t)&ifr) < 0)
		warn("SIOCS80211NWID");
}

void
setifnwkey(val, d)
	const char *val;
	int d;
{
	int i, len;
	char *cp = NULL;
	struct ieee80211_nwkey nwkey;
	u_int8_t keybuf[IEEE80211_WEP_NKID][16];

	nwkey.i_wepon = IEEE80211_NWKEY_WEP;
	nwkey.i_defkid = 1;
	if (d == -1) {
		/* disable WEP encryption */
		nwkey.i_wepon = 0;
		i = 0;
	} else if (strcasecmp("persist", val) == 0) {
		/* use all values from persistent memory */
		nwkey.i_wepon |= IEEE80211_NWKEY_PERSIST;
		nwkey.i_defkid = 0;
		for (i = 0; i < IEEE80211_WEP_NKID; i++)
			nwkey.i_key[i].i_keylen = -1;
	} else if (strncasecmp("persist:", val, 8) == 0) {
		val += 8;
		/* program keys in persistent memory */
		nwkey.i_wepon |= IEEE80211_NWKEY_PERSIST;
		goto set_nwkey;
	} else {
  set_nwkey:
		if (isdigit(val[0]) && val[1] == ':') {
			/* specifying a full set of four keys */
			nwkey.i_defkid = val[0] - '0';
			val += 2;
			for (i = 0; i < IEEE80211_WEP_NKID; i++) {
				len = sizeof(keybuf[i]);
				val = get_string(val, ",", keybuf[i], &len);
				if (val == NULL)
					return;
				nwkey.i_key[i].i_keylen = len;
				nwkey.i_key[i].i_keydat = keybuf[i];
			}
			if (cp != NULL) {
				warnx("SIOCS80211NWKEY: too many keys.");
				return;
			}
		} else {
			len = sizeof(keybuf[i]);
			val = get_string(val, NULL, keybuf[0], &len);
			if (val == NULL)
				return;
			nwkey.i_key[0].i_keylen = len;
			nwkey.i_key[0].i_keydat = keybuf[0];
			i = 1;
		}
	}
	/* zero out any unset keys */
	for (; i < IEEE80211_WEP_NKID; i++) {
		nwkey.i_key[i].i_keylen = 0;
		nwkey.i_key[i].i_keydat = NULL;
	}
	(void)strlcpy(nwkey.i_name, name, sizeof(nwkey.i_name));
	if (ioctl(s, SIOCS80211NWKEY, (caddr_t)&nwkey) == -1)
		warn("SIOCS80211NWKEY");
}

void
setifpowersave(val, d)
	const char *val;
	int d;
{
	struct ieee80211_power power;

	(void)strlcpy(power.i_name, name, sizeof(power.i_name));
	if (ioctl(s, SIOCG80211POWER, (caddr_t)&power) == -1) {
		warn("SIOCG80211POWER");
		return;
	}

	power.i_enabled = d;
	if (ioctl(s, SIOCS80211POWER, (caddr_t)&power) == -1)
		warn("SIOCS80211POWER");
}

void
setifpowersavesleep(val, d)
	const char *val;
	int d;
{
	struct ieee80211_power power;

	(void)strlcpy(power.i_name, name, sizeof(power.i_name));
	if (ioctl(s, SIOCG80211POWER, (caddr_t)&power) == -1) {
		warn("SIOCG80211POWER");
		return;
	}

	power.i_maxsleep = atoi(val);
	if (ioctl(s, SIOCS80211POWER, (caddr_t)&power) == -1)
		warn("SIOCS80211POWER");
}

void
ieee80211_status()
{
	int len, i, nwkey_verbose;
	struct ieee80211_nwid nwid;
	struct ieee80211_nwkey nwkey;
	struct ieee80211_power power;
	u_int8_t keybuf[IEEE80211_WEP_NKID][16];

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_data = (caddr_t)&nwid;
	(void)strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCG80211NWID, (caddr_t)&ifr) == 0) {
		/* nwid.i_nwid is not NUL terminated. */
		len = nwid.i_len;
		if (len > IEEE80211_NWID_LEN)
			len = IEEE80211_NWID_LEN;
		fputs("\tnwid ", stdout);
		print_string(nwid.i_nwid, nwid.i_len);
		putchar('\n');
	}

	memset(&nwkey, 0, sizeof(nwkey));
	(void)strlcpy(nwkey.i_name, name, sizeof(nwkey.i_name));
	if (ioctl(s, SIOCG80211NWKEY, (caddr_t)&nwkey) == 0 &&
	    nwkey.i_wepon > 0) {
		fputs("\tnwkey ", stdout);
		/* try to retrieve WEP keys */
		for (i = 0; i < IEEE80211_WEP_NKID; i++) {
			nwkey.i_key[i].i_keydat = keybuf[i];
			nwkey.i_key[i].i_keylen = sizeof(keybuf[i]);
		}
		if (ioctl(s, SIOCG80211NWKEY, (caddr_t)&nwkey) == -1) {
			puts("<not displayed>");
		} else {
			nwkey_verbose = 0;
			/* check to see non default key or multiple keys defined */
			if (nwkey.i_defkid != 1) {
				nwkey_verbose = 1;
			} else {
				for (i = 1; i < IEEE80211_WEP_NKID; i++) {
					if (nwkey.i_key[i].i_keylen != 0) {
						nwkey_verbose = 1;
						break;
					}
				}
			}
			/* check extra ambiguity with keywords */
			if (!nwkey_verbose) {
				if (nwkey.i_key[0].i_keylen >= 2 &&
				    isdigit(nwkey.i_key[0].i_keydat[0]) &&
				    nwkey.i_key[0].i_keydat[1] == ':')
					nwkey_verbose = 1;
				else if (nwkey.i_key[0].i_keylen >= 7 &&
				    strncasecmp("persist",
				    nwkey.i_key[0].i_keydat, 7) == 0)
					nwkey_verbose = 1;
			}
			if (nwkey_verbose)
				printf("%d:", nwkey.i_defkid);
			for (i = 0; i < IEEE80211_WEP_NKID; i++) {
				if (i > 0)
					putchar(',');
				if (nwkey.i_key[i].i_keylen < 0) {
					fputs("persist", stdout);
				} else {
					/* XXX - sanity check nwkey.i_key[i].i_keylen */
					print_string(nwkey.i_key[i].i_keydat,
					    nwkey.i_key[i].i_keylen);
				}
				if (!nwkey_verbose)
					break;
			}
			putchar('\n');
		}
	}

	memset(&power, 0, sizeof(power));
	(void)strlcpy(power.i_name, name, sizeof(power.i_name));
	if (ioctl(s, SIOCG80211POWER, &power) == 0) {
		fputs("\tpowersave ", stdout);
		if (power.i_enabled)
			printf("on (%dms sleep)\n", power.i_maxsleep);
		else
			puts("off");
	}
}

void
init_current_media()
{
	struct ifmediareq ifmr;

	/*
	 * If we have not yet done so, grab the currently-selected
	 * media.
	 */
	if ((actions & (A_MEDIA|A_MEDIAOPT)) == 0) {
		(void) memset(&ifmr, 0, sizeof(ifmr));
		(void) strlcpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));

		if (ioctl(s, SIOCGIFMEDIA, (caddr_t)&ifmr) < 0) {
			/*
			 * If we get E2BIG, the kernel is telling us
			 * that there are more, so we can ignore it.
			 */
			if (errno != E2BIG)
				err(1, "SGIOCGIFMEDIA");
		}

		media_current = ifmr.ifm_current;
	}

	/* Sanity. */
	if (IFM_TYPE(media_current) == 0)
		errx(1, "%s: no link type?", name);
}

void
process_media_commands()
{

	if ((actions & (A_MEDIA|A_MEDIAOPT)) == 0) {
		/* Nothing to do. */
		return;
	}

	/*
	 * Media already set up, and commands sanity-checked.  Set/clear
	 * any options, and we're ready to go.
	 */
	media_current |= mediaopt_set;
	media_current &= ~mediaopt_clear;

	(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_media = media_current;

	if (ioctl(s, SIOCSIFMEDIA, (caddr_t)&ifr) < 0)
		err(1, "SIOCSIFMEDIA");
}

void
setmedia(val, d)
	char *val;
	int d;
{
	int type, subtype, inst;

	init_current_media();

	/* Only one media command may be given. */
	if (actions & A_MEDIA)
		errx(1, "only one `media' command may be issued");

	/* Must not come after mediaopt commands */
	if (actions & A_MEDIAOPT)
		errx(1, "may not issue `media' after `mediaopt' commands");

	/*
	 * No need to check if `instance' has been issued; setmediainst()
	 * craps out if `media' has not been specified.
	 */

	type = IFM_TYPE(media_current);
	inst = IFM_INST(media_current);

	/* Look up the subtype. */
	subtype = get_media_subtype(type, val);

	/* Build the new current media word. */
	media_current = IFM_MAKEWORD(type, subtype, 0, inst);

	/* Media will be set after other processing is complete. */
}

void
setmediaopt(val, d)
	char *val;
	int d;
{

	init_current_media();

	/* Can only issue `mediaopt' once. */
	if (actions & A_MEDIAOPTSET)
		errx(1, "only one `mediaopt' command may be issued");

	/* Can't issue `mediaopt' if `instance' has already been issued. */
	if (actions & A_MEDIAINST)
		errx(1, "may not issue `mediaopt' after `instance'");

	mediaopt_set = get_media_options(IFM_TYPE(media_current), val);

	/* Media will be set after other processing is complete. */
}

void
unsetmediaopt(val, d)
	char *val;
	int d;
{

	init_current_media();

	/* Can only issue `-mediaopt' once. */
	if (actions & A_MEDIAOPTCLR)
		errx(1, "only one `-mediaopt' command may be issued");

	/* May not issue `media' and `-mediaopt'. */
	if (actions & A_MEDIA)
		errx(1, "may not issue both `media' and `-mediaopt'");

	/*
	 * No need to check for A_MEDIAINST, since the test for A_MEDIA
	 * implicitly checks for A_MEDIAINST.
	 */

	mediaopt_clear = get_media_options(IFM_TYPE(media_current), val);

	/* Media will be set after other processing is complete. */
}

void
setmediainst(val, d)
	char *val;
	int d;
{
	int type, subtype, options, inst;

	init_current_media();

	/* Can only issue `instance' once. */
	if (actions & A_MEDIAINST)
		errx(1, "only one `instance' command may be issued");

	/* Must have already specified `media' */
	if ((actions & A_MEDIA) == 0)
		errx(1, "must specify `media' before `instance'");

	type = IFM_TYPE(media_current);
	subtype = IFM_SUBTYPE(media_current);
	options = IFM_OPTIONS(media_current);

	inst = atoi(val);
	if (inst < 0 || inst > IFM_INST_MAX)
		errx(1, "invalid media instance: %s", val);

	media_current = IFM_MAKEWORD(type, subtype, options, inst);

	/* Media will be set after other processing is complete. */
}

const struct ifmedia_description ifm_type_descriptions[] =
    IFM_TYPE_DESCRIPTIONS;

const struct ifmedia_description ifm_subtype_descriptions[] =
    IFM_SUBTYPE_DESCRIPTIONS;

const struct ifmedia_description ifm_option_descriptions[] =
    IFM_OPTION_DESCRIPTIONS;

const char *
get_media_type_string(mword)
	int mword;
{
	const struct ifmedia_description *desc;

	for (desc = ifm_type_descriptions; desc->ifmt_string != NULL;
	     desc++) {
		if (IFM_TYPE(mword) == desc->ifmt_word)
			return (desc->ifmt_string);
	}
	return ("<unknown type>");
}

const char *
get_media_subtype_string(mword)
	int mword;
{
	const struct ifmedia_description *desc;

	for (desc = ifm_subtype_descriptions; desc->ifmt_string != NULL;
	     desc++) {
		if (IFM_TYPE_MATCH(desc->ifmt_word, mword) &&
		    IFM_SUBTYPE(desc->ifmt_word) == IFM_SUBTYPE(mword))
			return (desc->ifmt_string);
	}
	return ("<unknown subtype>");
}

int
get_media_subtype(type, val)
	int type;
	const char *val;
{
	int rval;

	rval = lookup_media_word(ifm_subtype_descriptions, type, val);
	if (rval == -1)
		errx(1, "unknown %s media subtype: %s",
		    get_media_type_string(type), val);

	return (rval);
}

int
get_media_options(type, val)
	int type;
	const char *val;
{
	char *optlist, *str;
	int option, rval = 0;

	/* We muck with the string, so copy it. */
	optlist = strdup(val);
	if (optlist == NULL)
		err(1, "strdup");
	str = optlist;

	/*
	 * Look up the options in the user-provided comma-separated list.
	 */
	for (; (str = strtok(str, ",")) != NULL; str = NULL) {
		option = lookup_media_word(ifm_option_descriptions, type, str);
		if (option == -1)
			errx(1, "unknown %s media option: %s",
			    get_media_type_string(type), str);
		rval |= IFM_OPTIONS(option);
	}

	free(optlist);
	return (rval);
}

int
lookup_media_word(desc, type, val)
	const struct ifmedia_description *desc;
	int type;
	const char *val;
{

	for (; desc->ifmt_string != NULL; desc++) {
		if (IFM_TYPE_MATCH(desc->ifmt_word, type) &&
		    strcasecmp(desc->ifmt_string, val) == 0)
			return (desc->ifmt_word);
	}
	return (-1);
}

void
print_media_word(ifmw, print_type, as_syntax)
	int ifmw, print_type, as_syntax;
{
	const struct ifmedia_description *desc;
	int seen_option = 0;

	if (print_type)
		printf("%s ", get_media_type_string(ifmw));
	printf("%s%s", as_syntax ? "media " : "",
	    get_media_subtype_string(ifmw));

	/* Find options. */
	for (desc = ifm_option_descriptions; desc->ifmt_string != NULL;
	     desc++) {
		if (IFM_TYPE_MATCH(desc->ifmt_word, ifmw) &&
		    (IFM_OPTIONS(ifmw) & IFM_OPTIONS(desc->ifmt_word)) != 0 &&
		    (seen_option & IFM_OPTIONS(desc->ifmt_word)) == 0) {
			if (seen_option == 0)
				printf(" %s", as_syntax ? "mediaopt " : "");
			printf("%s%s", seen_option ? "," : "",
			    desc->ifmt_string);
			seen_option |= IFM_OPTIONS(desc->ifmt_word);
		}
	}
	if (IFM_INST(ifmw) != 0)
		printf(" instance %d", IFM_INST(ifmw));
}

#define	IFFBITS \
"\020\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5POINTOPOINT\6NOTRAILERS\7RUNNING\10NOARP\
\11PROMISC\12ALLMULTI\13OACTIVE\14SIMPLEX\15LINK0\16LINK1\17LINK2\20MULTICAST"

static void
phys_status(force)
	int force;
{
	char psrcaddr[NI_MAXHOST];
	char pdstaddr[NI_MAXHOST];
	const char *ver = "";
#ifdef NI_WITHSCOPEID
	const int niflag = NI_NUMERICHOST | NI_WITHSCOPEID;
#else
	const int niflag = NI_NUMERICHOST;
#endif
	struct if_laddrreq req;

	psrcaddr[0] = pdstaddr[0] = '\0';

	memset(&req, 0, sizeof(req));
	(void) strlcpy(req.iflr_name, name, sizeof(req.iflr_name));
	if (ioctl(s, SIOCGLIFPHYADDR, (caddr_t)&req) < 0)
		return;
#ifdef INET6
	if (req.addr.ss_family == AF_INET6)
		in6_fillscopeid((struct sockaddr_in6 *)&req.addr);
#endif
	getnameinfo((struct sockaddr *)&req.addr, req.addr.ss_len,
	    psrcaddr, sizeof(psrcaddr), 0, 0, niflag);
#ifdef INET6
	if (req.addr.ss_family == AF_INET6)
		ver = "6";
#endif

#ifdef INET6
	if (req.dstaddr.ss_family == AF_INET6)
		in6_fillscopeid((struct sockaddr_in6 *)&req.dstaddr);
#endif
	getnameinfo((struct sockaddr *)&req.dstaddr, req.dstaddr.ss_len,
	    pdstaddr, sizeof(pdstaddr), 0, 0, niflag);

	printf("\tphysical address inet%s %s --> %s\n", ver,
	    psrcaddr, pdstaddr);
}

const int ifm_status_valid_list[] = IFM_STATUS_VALID_LIST;

const struct ifmedia_status_description ifm_status_descriptions[] =
	IFM_STATUS_DESCRIPTIONS;

/*
 * Print the status of the interface.  If an address family was
 * specified, show it and it only; otherwise, show them all.
 */
void
status(link, sdl)
	int link;
	struct sockaddr_dl *sdl;
{
	const struct afswtch *p = afp;
	struct ifmediareq ifmr;
	int *media_list, i;

	printf("%s: ", name);
	printb("flags", flags, IFFBITS);
	if (metric)
		printf(" metric %lu", metric);
	if (mtu)
		printf(" mtu %lu", mtu);
	putchar('\n');
	if (sdl != NULL && sdl->sdl_type == IFT_ETHER && sdl->sdl_alen)
		(void)printf("\taddress: %s\n", ether_ntoa(
		    (struct ether_addr *)LLADDR(sdl)));

#ifndef	INET_ONLY
	vlan_status();
#endif
	ieee80211_status();

	(void) memset(&ifmr, 0, sizeof(ifmr));
	(void) strlcpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));

	if (ioctl(s, SIOCGIFMEDIA, (caddr_t)&ifmr) < 0) {
		/*
		 * Interface doesn't support SIOC{G,S}IFMEDIA.
		 */
		goto proto_status;
	}

	if (ifmr.ifm_count == 0) {
		warnx("%s: no media types?", name);
		goto proto_status;
	}

	media_list = (int *)malloc(ifmr.ifm_count * sizeof(int));
	if (media_list == NULL)
		err(1, "malloc");
	ifmr.ifm_ulist = media_list;

	if (ioctl(s, SIOCGIFMEDIA, (caddr_t)&ifmr) < 0)
		err(1, "SIOCGIFMEDIA");

	printf("\tmedia: ");
	print_media_word(ifmr.ifm_current, 1, 0);
	if (ifmr.ifm_active != ifmr.ifm_current) {
		putchar(' ');
		putchar('(');
		print_media_word(ifmr.ifm_active, 0, 0);
		putchar(')');
	}
	putchar('\n');

	if (ifmr.ifm_status & IFM_AVALID) {
		const struct ifmedia_status_description *ifms;
		int bitno, found = 0;

		printf("\tstatus: ");
		for (bitno = 0; ifm_status_valid_list[bitno] != 0; bitno++) {
			for (ifms = ifm_status_descriptions;
			     ifms->ifms_valid != 0; ifms++) {
				if (ifms->ifms_type !=
				      IFM_TYPE(ifmr.ifm_current) ||
				    ifms->ifms_valid !=
				      ifm_status_valid_list[bitno])
					continue;
				printf("%s%s", found ? ", " : "",
				    IFM_STATUS_DESC(ifms, ifmr.ifm_status));
				found = 1;

				/*
				 * For each valid indicator bit, there's
				 * only one entry for each media type, so
				 * terminate the inner loop now.
				 */
				break;
			}
		}

		if (found == 0)
			printf("unknown");
		putchar('\n');
	}

	if (mflag) {
		int type, printed_type = 0;

		for (type = IFM_NMIN; type <= IFM_NMAX; type += IFM_NMIN) {
			for (i = 0, printed_type = 0; i < ifmr.ifm_count; i++) {
				if (IFM_TYPE(media_list[i]) == type) {
					if (printed_type == 0) {
					    printf("\tsupported media:\n");
					    printed_type = 1;
					}
					printf("\t\t");
					print_media_word(media_list[i], 0, 1);
					printf("\n");
				}
			}
		}
	}

	free(media_list);

 proto_status:
	if (link == 0) {
		if ((p = afp) != NULL) {
			(*p->af_status)(1);
		} else for (p = afs; p->af_name; p++) {
			ifr.ifr_addr.sa_family = p->af_af;
			(*p->af_status)(0);
		}
	}

	phys_status(0);
}


void
in_status(force)
	int force;
{
	struct sockaddr_in *sin, sin2;
	char *inet_ntoa();

	getsock(AF_INET);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(1, "socket");
	}
	(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	sin = (struct sockaddr_in *)&ifr.ifr_addr;

	/*
	 * We keep the interface address and reset it before each
	 * ioctl() so we can get ifaliases information (as opposed
	 * to the primary interface netmask/dstaddr/broadaddr, if
	 * the ifr_addr field is zero).
	 */
	memcpy(&sin2, &ifr.ifr_addr, sizeof(sin2));

	printf("\tinet %s ", inet_ntoa(sin->sin_addr));
	(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFNETMASK, (caddr_t)&ifr) < 0) {
		if (errno != EADDRNOTAVAIL)
			warn("SIOCGIFNETMASK");
		memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
	} else
		netmask.sin_addr =
		    ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
	if (flags & IFF_POINTOPOINT) {
		memcpy(&ifr.ifr_addr, &sin2, sizeof(sin2));
		if (ioctl(s, SIOCGIFDSTADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
			else
			    warn("SIOCGIFDSTADDR");
		}
		(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
		sin = (struct sockaddr_in *)&ifr.ifr_dstaddr;
		printf("--> %s ", inet_ntoa(sin->sin_addr));
	}
	printf("netmask 0x%x ", ntohl(netmask.sin_addr.s_addr));
	if (flags & IFF_BROADCAST) {
		memcpy(&ifr.ifr_addr, &sin2, sizeof(sin2));
		if (ioctl(s, SIOCGIFBRDADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
			else
			    warn("SIOCGIFBRDADDR");
		}
		(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
		sin = (struct sockaddr_in *)&ifr.ifr_addr;
		if (sin->sin_addr.s_addr != 0)
			printf("broadcast %s", inet_ntoa(sin->sin_addr));
	}
	putchar('\n');
}

void
setifprefixlen(addr, d)
	char *addr;
	int d;
{
	if (*afp->af_getprefix)
		(*afp->af_getprefix)(addr, MASK);
	explicit_prefix = 1;
}

#ifdef INET6
void
in6_fillscopeid(sin6)
	struct sockaddr_in6 *sin6;
{
#if defined(__KAME__) && defined(KAME_SCOPEID)
	if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
		sin6->sin6_scope_id =
			ntohs(*(u_int16_t *)&sin6->sin6_addr.s6_addr[2]);
		sin6->sin6_addr.s6_addr[2] = sin6->sin6_addr.s6_addr[3] = 0;
	}
#endif
}

/* XXX not really an alias */
void
in6_alias(creq)
	struct in6_ifreq *creq;
{
	struct sockaddr_in6 *sin6;
	struct	in6_ifreq ifr6;		/* shadows file static variable */
	u_int32_t scopeid;
	char hbuf[NI_MAXHOST];
#ifdef NI_WITHSCOPEID
	const int niflag = NI_NUMERICHOST | NI_WITHSCOPEID;
#else
	const int niflag = NI_NUMERICHOST;
#endif

	/* Get the non-alias address for this interface. */
	getsock(AF_INET6);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(1, "socket");
	}

	sin6 = (struct sockaddr_in6 *)&creq->ifr_addr;

	in6_fillscopeid(sin6);
	scopeid = sin6->sin6_scope_id;
	if (getnameinfo((struct sockaddr *)sin6, sin6->sin6_len,
	    hbuf, sizeof(hbuf), NULL, 0, niflag) != 0)
		strlcpy(hbuf, "", sizeof hbuf);
	printf("\tinet6 %s", hbuf);

	if (flags & IFF_POINTOPOINT) {
		(void) memset(&ifr6, 0, sizeof(ifr6));
		(void) strlcpy(ifr6.ifr_name, name, sizeof(ifr6.ifr_name));
		ifr6.ifr_addr = creq->ifr_addr;
		if (ioctl(s, SIOCGIFDSTADDR_IN6, (caddr_t)&ifr6) < 0) {
			if (errno != EADDRNOTAVAIL)
				warn("SIOCGIFDSTADDR_IN6");
			(void) memset(&ifr6.ifr_addr, 0, sizeof(ifr6.ifr_addr));
			ifr6.ifr_addr.sin6_family = AF_INET6;
			ifr6.ifr_addr.sin6_len = sizeof(struct sockaddr_in6);
		}
		sin6 = (struct sockaddr_in6 *)&ifr6.ifr_addr;
		in6_fillscopeid(sin6);
		if (getnameinfo((struct sockaddr *)sin6, sin6->sin6_len,
		    hbuf, sizeof(hbuf), NULL, 0, niflag) != 0)
			strlcpy(hbuf, "", sizeof hbuf);
		printf(" -> %s", hbuf);
	}

	(void) memset(&ifr6, 0, sizeof(ifr6));
	(void) strlcpy(ifr6.ifr_name, name, sizeof(ifr6.ifr_name));
	ifr6.ifr_addr = creq->ifr_addr;
	if (ioctl(s, SIOCGIFNETMASK_IN6, (caddr_t)&ifr6) < 0) {
		if (errno != EADDRNOTAVAIL)
			warn("SIOCGIFNETMASK_IN6");
	} else {
		sin6 = (struct sockaddr_in6 *)&ifr6.ifr_addr;
		printf(" prefixlen %d", prefix(&sin6->sin6_addr,
		    sizeof(struct in6_addr)));
	}

	(void) memset(&ifr6, 0, sizeof(ifr6));
	(void) strlcpy(ifr6.ifr_name, name, sizeof(ifr6.ifr_name));
	ifr6.ifr_addr = creq->ifr_addr;
	if (ioctl(s, SIOCGIFAFLAG_IN6, (caddr_t)&ifr6) < 0) {
		if (errno != EADDRNOTAVAIL)
			warn("SIOCGIFAFLAG_IN6");
	} else {
		if (ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_ANYCAST)
			printf(" anycast");
		if (ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_TENTATIVE)
			printf(" tentative");
		if (ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_DUPLICATED)
			printf(" duplicated");
		if (ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_DETACHED)
			printf(" detached");
		if (ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_DEPRECATED)
			printf(" deprecated");
	}

	if (scopeid)
		printf(" scopeid 0x%x", scopeid);

	if (Lflag) {
		struct in6_addrlifetime *lifetime;
		(void) memset(&ifr6, 0, sizeof(ifr6));
		(void) strlcpy(ifr6.ifr_name, name, sizeof(ifr6.ifr_name));
		ifr6.ifr_addr = creq->ifr_addr;
		lifetime = &ifr6.ifr_ifru.ifru_lifetime;
		if (ioctl(s, SIOCGIFALIFETIME_IN6, (caddr_t)&ifr6) < 0) {
			if (errno != EADDRNOTAVAIL)
				warn("SIOCGIFALIFETIME_IN6");
		} else if (lifetime->ia6t_preferred || lifetime->ia6t_expire) {
			time_t t = time(NULL);
			printf(" pltime ");
			if (lifetime->ia6t_preferred) {
				printf("%s", lifetime->ia6t_preferred < t
					? "0"
					: sec2str(lifetime->ia6t_preferred - t));
			} else
				printf("infty");

			printf(" vltime ");
			if (lifetime->ia6t_expire) {
				printf("%s", lifetime->ia6t_expire < t
					? "0"
					: sec2str(lifetime->ia6t_expire - t));
			} else
				printf("infty");
		}
	}

	printf("\n");
}

void
in6_status(force)
	int force;
{
	in6_alias((struct in6_ifreq *)&ifr6);
}
#endif /*INET6*/

#ifndef INET_ONLY

void
at_status(force)
	int force;
{
	struct sockaddr_at *sat, null_sat;
	struct netrange *nr;

	getsock(AF_APPLETALK);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(1, "socket");
	}
	(void) memset(&ifr, 0, sizeof(ifr));
	(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) < 0) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			if (!force)
				return;
			(void) memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
		} else
			warn("SIOCGIFADDR");
	}
	(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	sat = (struct sockaddr_at *)&ifr.ifr_addr;

	(void) memset(&null_sat, 0, sizeof(null_sat));

	nr = (struct netrange *) &sat->sat_zero;
	printf("\tAppleTalk %d.%d range %d-%d phase %d",
	    ntohs(sat->sat_addr.s_net), sat->sat_addr.s_node,
	    ntohs(nr->nr_firstnet), ntohs(nr->nr_lastnet), nr->nr_phase);
	if (flags & IFF_POINTOPOINT) {
		if (ioctl(s, SIOCGIFDSTADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    (void) memset(&ifr.ifr_addr, 0,
				sizeof(ifr.ifr_addr));
			else
			    warn("SIOCGIFDSTADDR");
		}
		(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
		sat = (struct sockaddr_at *)&ifr.ifr_dstaddr;
		if (!sat)
			sat = &null_sat;
		printf("--> %d.%d",
		    ntohs(sat->sat_addr.s_net), sat->sat_addr.s_node);
	}
	if (flags & IFF_BROADCAST) {
		/* note RTAX_BRD overlap with IFF_POINTOPOINT */
		sat = (struct sockaddr_at *)&ifr.ifr_broadaddr;
		if (sat)
			printf(" broadcast %d.%d", ntohs(sat->sat_addr.s_net),
			    sat->sat_addr.s_node);
	}
	putchar('\n');
}

void
xns_status(force)
	int force;
{
	struct sockaddr_ns *sns;

	getsock(AF_NS);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(1, "socket");
	}
	memset(&ifr, 0, sizeof(ifr));
	(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) < 0) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			if (!force)
				return;
			memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
		} else
			warn("SIOCGIFADDR");
	}
	(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	sns = (struct sockaddr_ns *)&ifr.ifr_addr;
	printf("\tns %s ", ns_ntoa(sns->sns_addr));
	if (flags & IFF_POINTOPOINT) { /* by W. Nesheim@Cornell */
		if (ioctl(s, SIOCGIFDSTADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
			else
			    warn("SIOCGIFDSTADDR");
		}
		(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
		sns = (struct sockaddr_ns *)&ifr.ifr_dstaddr;
		printf("--> %s ", ns_ntoa(sns->sns_addr));
	}
	putchar('\n');
}

void
setipxframetype(vname, type)
	char	*vname;
	int	type;
{
	struct  sockaddr_ipx	*sipx;

	ipx_type = type;
	getsock(AF_IPX);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(1, "socket");
	}
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	sipx = (struct sockaddr_ipx *)&addreq.ifra_addr;
	sipx->sipx_type = ipx_type;
}

void
ipx_status(force)
	int force;
{
	struct sockaddr_ipx *sipx;

	getsock(AF_IPX);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(1, "socket");
	}
	memset(&ifr, 0, sizeof(ifr));
	(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) < 0) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			if (!force)
				return;
			memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
		} else
			warn("SIOCGIFADDR");
	}
	(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	sipx = (struct sockaddr_ipx *)&ifr.ifr_addr;
	printf("\tipx %s ", ipx_ntoa(sipx->sipx_addr));
	if (flags & IFF_POINTOPOINT) { /* by W. Nesheim@Cornell */
		if (ioctl(s, SIOCGIFDSTADDR, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
			else
			    warn("SIOCGIFDSTADDR");
		}
		(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
		sipx = (struct sockaddr_ipx *)&ifr.ifr_dstaddr;
		printf("--> %s ", ipx_ntoa(sipx->sipx_addr));
	}
	{
		struct frame_types {
			int	type;
			char	*name;
		} *p, frames[] = {
			{ ETHERTYPE_8022, "802.2" },
			{ ETHERTYPE_8022TR, "802.2tr" },
			{ ETHERTYPE_8023, "802.3" },
			{ ETHERTYPE_SNAP, "SNAP" },
			{ ETHERTYPE_II,  "EtherII" },
			{ 0, NULL }
		};
		for (p = frames; p->name && p->type != sipx->sipx_type; p++);
		if (p->name != NULL)
			printf("frame %s ", p->name);
	}
	putchar('\n');
}

void
iso_status(force)
	int force;
{
	struct sockaddr_iso *siso;
	struct iso_ifreq ifr;

	getsock(AF_ISO);
	if (s < 0) {
		if (errno == EPROTONOSUPPORT)
			return;
		err(1, "socket");
	}
	memset(&ifr, 0, sizeof(ifr));
	(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCGIFADDR_ISO, (caddr_t)&ifr) < 0) {
		if (errno == EADDRNOTAVAIL || errno == EAFNOSUPPORT) {
			if (!force)
				return;
			memset(&ifr.ifr_Addr, 0, sizeof(ifr.ifr_Addr));
		} else
			warn("SIOCGIFADDR_ISO");
	}
	(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	siso = &ifr.ifr_Addr;
	printf("\tiso %s ", iso_ntoa(&siso->siso_addr));
	if (ioctl(s, SIOCGIFNETMASK_ISO, (caddr_t)&ifr) < 0) {
		if (errno == EADDRNOTAVAIL)
			memset(&ifr.ifr_Addr, 0, sizeof(ifr.ifr_Addr));
		else
			warn("SIOCGIFNETMASK_ISO");
	} else {
		printf(" netmask %s ", iso_ntoa(&siso->siso_addr));
	}
	if (flags & IFF_POINTOPOINT) {
		if (ioctl(s, SIOCGIFDSTADDR_ISO, (caddr_t)&ifr) < 0) {
			if (errno == EADDRNOTAVAIL)
			    memset(&ifr.ifr_Addr, 0, sizeof(ifr.ifr_Addr));
			else
			    warn("SIOCGIFDSTADDR_ISO");
		}
		(void) strlcpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
		siso = &ifr.ifr_Addr;
		printf("--> %s ", iso_ntoa(&siso->siso_addr));
	}
	putchar('\n');
}

#endif	/* INET_ONLY */

struct	in_addr inet_makeaddr();

#define SIN(x) ((struct sockaddr_in *) &(x))
struct sockaddr_in *sintab[] = {
SIN(ridreq.ifr_addr), SIN(in_addreq.ifra_addr),
SIN(in_addreq.ifra_mask), SIN(in_addreq.ifra_broadaddr)};

void
in_getaddr(s, which)
	char *s;
	int which;
{
	struct sockaddr_in *sin = sintab[which];
	struct hostent *hp;
	struct netent *np;

	sin->sin_len = sizeof(*sin);
	if (which != MASK)
		sin->sin_family = AF_INET;

	if (inet_aton(s, &sin->sin_addr) == 0) {
		if ((hp = gethostbyname(s)))
			memcpy(&sin->sin_addr, hp->h_addr, hp->h_length);
		else if ((np = getnetbyname(s)))
			sin->sin_addr = inet_makeaddr(np->n_net, INADDR_ANY);
		else
			errx(1, "%s: bad value", s);
	}
}

void
in_getprefix(plen, which)
	char *plen;
	int which;
{
	struct sockaddr_in *sin = sintab[which];
	u_char *cp;
	int len = strtol(plen, (char **)NULL, 10);

	if ((len < 0) || (len > 32))
		errx(1, "%s: bad value", plen);
	sin->sin_len = sizeof(*sin);
	if (which != MASK)
		sin->sin_family = AF_INET;
	if ((len == 0) || (len == 32)) {
		memset(&sin->sin_addr, 0xff, sizeof(struct in_addr));
		return;
	}
	memset((void *)&sin->sin_addr, 0x00, sizeof(sin->sin_addr));
	for (cp = (u_char *)&sin->sin_addr; len > 7; len -= 8)
		*cp++ = 0xff;
	if (len)
		*cp = 0xff << (8 - len);
}

/*
 * Print a value a la the %b format of the kernel's printf
 */
void
printb(s, v, bits)
	char *s;
	char *bits;
	unsigned short v;
{
	int i, any = 0;
	char c;

	if (bits && *bits == 8)
		printf("%s=%o", s, v);
	else
		printf("%s=%x", s, v);
	bits++;
	if (bits) {
		putchar('<');
		while ((i = *bits++)) {
			if (v & (1 << (i-1))) {
				if (any)
					putchar(',');
				any = 1;
				for (; (c = *bits) > 32; bits++)
					putchar(c);
			} else
				for (; *bits > 32; bits++)
					;
		}
		putchar('>');
	}
}

#ifdef INET6
#define SIN6(x) ((struct sockaddr_in6 *) &(x))
struct sockaddr_in6 *sin6tab[] = {
SIN6(in6_ridreq.ifr_addr), SIN6(in6_addreq.ifra_addr),
SIN6(in6_addreq.ifra_prefixmask), SIN6(in6_addreq.ifra_dstaddr)};

void
in6_getaddr(s, which)
	char *s;
	int which;
{
#ifndef KAME_SCOPEID
	struct sockaddr_in6 *sin6 = sin6tab[which];

	sin6->sin6_len = sizeof(*sin6);
	if (which != MASK)
		sin6->sin6_family = AF_INET6;

	if (inet_pton(AF_INET6, s, &sin6->sin6_addr) != 1)
		errx(1, "%s: bad value", s);
#else
	struct sockaddr_in6 *sin6 = sin6tab[which];
	struct addrinfo hints, *res;
	int error;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;	/*dummy*/
	error = getaddrinfo(s, "0", &hints, &res);
	if (error)
		errx(1, "%s: %s", s, gai_strerror(error));
	if (res->ai_addrlen != sizeof(struct sockaddr_in6))
		errx(1, "%s: bad value", s);
	memcpy(sin6, res->ai_addr, res->ai_addrlen);
#ifdef __KAME__
	if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr) &&
	    *(u_int16_t *)&sin6->sin6_addr.s6_addr[2] == 0 &&
	    sin6->sin6_scope_id) {
		*(u_int16_t *)&sin6->sin6_addr.s6_addr[2] =
		    htons(sin6->sin6_scope_id & 0xffff);
		sin6->sin6_scope_id = 0;
	}
#endif
	freeaddrinfo(res);
#endif
}

void
in6_getprefix(plen, which)
	char *plen;
	int which;
{
	struct sockaddr_in6 *sin = sin6tab[which];
	u_char *cp;
	int len = strtol(plen, (char **)NULL, 10);

	if ((len < 0) || (len > 128))
		errx(1, "%s: bad value", plen);
	sin->sin6_len = sizeof(*sin);
	if (which != MASK)
		sin->sin6_family = AF_INET6;
	if ((len == 0) || (len == 128)) {
		memset(&sin->sin6_addr, 0xff, sizeof(struct in6_addr));
		return;
	}
	memset((void *)&sin->sin6_addr, 0x00, sizeof(sin->sin6_addr));
	for (cp = (u_char *)&sin->sin6_addr; len > 7; len -= 8)
		*cp++ = 0xff;
	if (len)
		*cp = 0xff << (8 - len);
}

int
prefix(val, size)
	void *val;
	int size;
{
	u_char *name = (u_char *)val;
	int byte, bit, plen = 0;

	for (byte = 0; byte < size; byte++, plen += 8)
		if (name[byte] != 0xff)
			break;
	if (byte == size)
		return (plen);
	for (bit = 7; bit != 0; bit--, plen++)
		if (!(name[byte] & (1 << bit)))
			break;
	for (; bit != 0; bit--)
		if (name[byte] & (1 << bit))
			return(0);
	byte++;
	for (; byte < size; byte++)
		if (name[byte])
			return(0);
	return (plen);
}
#endif /*INET6*/

#ifndef INET_ONLY
void
at_getaddr(addr, which)
	char *addr;
	int which;
{
	struct sockaddr_at *sat = (struct sockaddr_at *) &addreq.ifra_addr;
	u_int net, node;

	sat->sat_family = AF_APPLETALK;
	sat->sat_len = sizeof(*sat);
	if (which == MASK)
		errx(1, "AppleTalk does not use netmasks");
	if (sscanf(addr, "%u.%u", &net, &node) != 2 ||
	    net == 0 || net > 0xffff || node == 0 || node > 0xfe)
		errx(1, "%s: illegal address", addr);
	sat->sat_addr.s_net = htons(net);
	sat->sat_addr.s_node = node;
}

void
setatrange(range, d)
	char *range;
	int d;
{
	u_short first = 123, last = 123;

	if (sscanf(range, "%hu-%hu", &first, &last) != 2 ||
	    first == 0 || first > 0xffff ||
	    last == 0 || last > 0xffff || first > last)
		errx(1, "%s: illegal net range: %u-%u", range, first, last);
	at_nr.nr_firstnet = htons(first);
	at_nr.nr_lastnet = htons(last);
}

void
setatphase(phase, d)
	char *phase;
	int d;
{
	if (!strcmp(phase, "1"))
		at_nr.nr_phase = 1;
	else if (!strcmp(phase, "2"))
		at_nr.nr_phase = 2;
	else
		errx(1, "%s: illegal phase", phase);
}

void
checkatrange(sat)
	struct sockaddr_at *sat;
{
	if (at_nr.nr_phase == 0)
		at_nr.nr_phase = 2;	/* Default phase 2 */
	if (at_nr.nr_firstnet == 0)	/* Default range of one */
		at_nr.nr_firstnet = at_nr.nr_lastnet = sat->sat_addr.s_net;
	printf("\tatalk %d.%d range %d-%d phase %d\n",
	ntohs(sat->sat_addr.s_net), sat->sat_addr.s_node,
	ntohs(at_nr.nr_firstnet), ntohs(at_nr.nr_lastnet), at_nr.nr_phase);
	if ((u_short) ntohs(at_nr.nr_firstnet) >
	    (u_short) ntohs(sat->sat_addr.s_net) ||
	    (u_short) ntohs(at_nr.nr_lastnet) <
	    (u_short) ntohs(sat->sat_addr.s_net))
		errx(1, "AppleTalk address is not in range");
	*((struct netrange *) &sat->sat_zero) = at_nr;
}

#define SNS(x) ((struct sockaddr_ns *) &(x))
struct sockaddr_ns *snstab[] = {
SNS(ridreq.ifr_addr), SNS(addreq.ifra_addr),
SNS(addreq.ifra_mask), SNS(addreq.ifra_broadaddr)};

void
xns_getaddr(addr, which)
	char *addr;
	int which;
{
	struct sockaddr_ns *sns = snstab[which];
	struct ns_addr ns_addr();

	sns->sns_family = AF_NS;
	sns->sns_len = sizeof(*sns);
	sns->sns_addr = ns_addr(addr);
	if (which == MASK)
		printf("Attempt to set XNS netmask will be ineffectual\n");
}

#define SIPX(x) ((struct sockaddr_ipx *) &(x))
struct sockaddr_ipx *sipxtab[] = {
SIPX(ridreq.ifr_addr), SIPX(addreq.ifra_addr),
SIPX(addreq.ifra_mask), SIPX(addreq.ifra_broadaddr)};

void
ipx_getaddr(addr, which)
	char *addr;
	int which;
{
	struct sockaddr_ipx *sipx = sipxtab[which];
	struct ipx_addr ipx_addr();

	sipx->sipx_family = AF_IPX;
	sipx->sipx_len  = sizeof(*sipx);
	sipx->sipx_addr = ipx_addr(addr);
	sipx->sipx_type = ipx_type;
	if (which == MASK)
		printf("Attempt to set IPX netmask will be ineffectual\n");
}

#define SISO(x) ((struct sockaddr_iso *) &(x))
struct sockaddr_iso *sisotab[] = {
SISO(iso_ridreq.ifr_Addr), SISO(iso_addreq.ifra_addr),
SISO(iso_addreq.ifra_mask), SISO(iso_addreq.ifra_dstaddr)};

void
iso_getaddr(addr, which)
	char *addr;
	int which;
{
	struct sockaddr_iso *siso = sisotab[which];
	struct iso_addr *iso_addr();
	siso->siso_addr = *iso_addr(addr);

	if (which == MASK) {
		siso->siso_len = TSEL(siso) - (caddr_t)(siso);
		siso->siso_nlen = 0;
	} else {
		siso->siso_len = sizeof(*siso);
		siso->siso_family = AF_ISO;
	}
}

void
setsnpaoffset(val)
	char *val;
{
	iso_addreq.ifra_snpaoffset = atoi(val);
}

void
setnsellength(val)
	char *val;
{
	nsellength = atoi(val);
	if (nsellength < 0)
		errx(1, "negative NSEL length is absurd");
	if (afp == 0 || afp->af_af != AF_ISO)
		errx(1, "setting NSEL length valid only for iso");
}

void
fixnsel(s)
	struct sockaddr_iso *s;
{
	if (s->siso_family == 0)
		return;
	s->siso_tlen = nsellength;
}

void
adjust_nsellength()
{
	fixnsel(sisotab[RIDADDR]);
	fixnsel(sisotab[ADDR]);
	fixnsel(sisotab[DSTADDR]);
}

#endif	/* INET_ONLY */

void
usage()
{
	fprintf(stderr, "usage: ifconfig [ -m ] [ -a ] [ -A ] [ interface ]\n"
		"\t[ [af] [ address [ dest_addr ] ] [ up ] [ down ] "
		"[ netmask mask ] ]\n"
		"\t[ media media_type ] [ mediaopt media_option ]\n"
		"\t[ metric n ]\n"
		"\t[ mtu n ]\n"
		"\t[ nwid network_id ] [ nwkey network_key | -nwkey ]\n"
		"\t[ powersave | -powersave ] [ powersavesleep duration ]\n"
		"\t[ [af] tunnel srcaddress dstaddress ] [ deletetunnel ]\n"
		"\t[ vlan n vlandev interface ]\n"
		"\t[ arp | -arp ]\n"
		"\t[ anycast | -anycast ] [ deprecated | -deprecated ]\n"
		"\t[ tentative | -tentative ] [ pltime n ] [ vltime n ] [ eui64 ]\n"
		"\t[ -802.2 | -802.3 | -802.2tr | -snap | -EtherII ]\n"
		"\t[ link0 | -link0 ] [ link1 | -link1 ] [ link2 | -link2 ]\n"
		"       ifconfig [-a | -A | -am | -Am] [ af ]\n"
		"       ifconfig -m interface [af]\n");
	exit(1);
}

#ifndef INET_ONLY

static int __tag = 0;
static int __have_tag = 0;

void vlan_status()
{
	struct vlanreq vreq;

	bzero((char *)&vreq, sizeof(struct vlanreq));
	ifr.ifr_data = (caddr_t)&vreq;

	if (ioctl(s, SIOCGETVLAN, (caddr_t)&ifr) == -1)
		return;

	if (vreq.vlr_tag || (vreq.vlr_parent[0] != '\0'))
		printf("\tvlan: %d parent interface: %s\n",
		    vreq.vlr_tag, vreq.vlr_parent[0] == '\0' ?
		    "<none>" : vreq.vlr_parent);

	return;
}

void setvlantag(val, d)
	char *val;
	int d;
{
	u_int16_t tag;
	struct vlanreq vreq;

	__tag = tag = atoi(val);
	__have_tag = 1;

	bzero((char *)&vreq, sizeof(struct vlanreq));
	ifr.ifr_data = (caddr_t)&vreq;

	if (ioctl(s, SIOCGETVLAN, (caddr_t)&ifr) == -1)
		err(1, "SIOCGETVLAN");

	vreq.vlr_tag = tag;

	if (ioctl(s, SIOCSETVLAN, (caddr_t)&ifr) == -1)
		err(1, "SIOCSETVLAN");

	return;
}

void setvlandev(val, d)
	char *val;
	int d;
{
	struct vlanreq vreq;

	if (!__have_tag)
		errx(1, "must specify both vlan tag and device");

	bzero((char *)&vreq, sizeof(struct vlanreq));
	ifr.ifr_data = (caddr_t)&vreq;

	if (ioctl(s, SIOCGETVLAN, (caddr_t)&ifr) == -1)
		err(1, "SIOCGETVLAN");

	(void) strlcpy(vreq.vlr_parent, val, sizeof(vreq.vlr_parent));
	vreq.vlr_tag = __tag;

	if (ioctl(s, SIOCSETVLAN, (caddr_t)&ifr) == -1)
		err(1, "SIOCSETVLAN");

	return;
}

void unsetvlandev(val, d)
	char *val;
	int d;
{
	struct vlanreq vreq;

	bzero((char *)&vreq, sizeof(struct vlanreq));
	ifr.ifr_data = (caddr_t)&vreq;

	if (ioctl(s, SIOCGETVLAN, (caddr_t)&ifr) == -1)
		err(1, "SIOCGETVLAN");

	bzero((char *)&vreq.vlr_parent, sizeof(vreq.vlr_parent));
	vreq.vlr_tag = 0;

	if (ioctl(s, SIOCSETVLAN, (caddr_t)&ifr) == -1)
		err(1, "SIOCSETVLAN");

	return;
}

#endif /* INET_ONLY */

#ifdef INET6
char *
sec2str(total)
	time_t total;
{
	static char result[256];
	int days, hours, mins, secs;
	int first = 1;
	char *p = result;
	char *end = &result[sizeof(result)];
	int n;

	if (0) {	/*XXX*/
		days = total / 3600 / 24;
		hours = (total / 3600) % 24;
		mins = (total / 60) % 60;
		secs = total % 60;

		if (days) {
			first = 0;
			n = snprintf(p, end - p, "%dd", days);
			if (n < 0 || n >= end - p)
				return(result);
			p += n;
		}
		if (!first || hours) {
			first = 0;
			n = snprintf(p, end - p, "%dh", hours);
			if (n < 0 || n >= end - p)
				return(result);
			p += n;
		}
		if (!first || mins) {
			first = 0;
			n = snprintf(p, end - p, "%dm", mins);
			if (n < 0 || n >= end - p)
				return(result);
			p += n;
		}
		snprintf(p, end - p, "%ds", secs);
	} else
		snprintf(p, end - p, "%lu", (u_long)total);

	return(result);
}
#endif
