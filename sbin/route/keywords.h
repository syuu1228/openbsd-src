/* $OpenBSD: src/sbin/route/keywords.h,v 1.14 2005/03/30 06:02:52 henning Exp $ */

/* WARNING!  This file was generated by keywords.sh  */

struct keytab {
        char    *kt_cp;
        int     kt_i;
};

#define	K_ADD	1
#define	K_BLACKHOLE	2
#define	K_CHANGE	3
#define	K_CLONING	4
#define	K_DELETE	5
#define	K_DST	6
#define	K_EXPIRE	7
#define	K_FLUSH	8
#define	K_GATEWAY	9
#define	K_GENMASK	10
#define	K_GET	11
#define	K_HOST	12
#define	K_HOPCOUNT	13
#define	K_IFACE	14
#define	K_INTERFACE	15
#define	K_IFA	16
#define	K_IFP	17
#define	K_INET	18
#define	K_INET6	19
#define	K_IPX	20
#define	K_LABEL	21
#define	K_LINK	22
#define	K_LLINFO	23
#define	K_LOCK	24
#define	K_LOCKREST	25
#define	K_MONITOR	26
#define	K_MPATH	27
#define	K_MTU	28
#define	K_NET	29
#define	K_NETMASK	30
#define	K_NOSTATIC	31
#define	K_PREFIXLEN	32
#define	K_PROTO1	33
#define	K_PROTO2	34
#define	K_RECVPIPE	35
#define	K_REJECT	36
#define	K_RTT	37
#define	K_RTTVAR	38
#define	K_SA	39
#define	K_SENDPIPE	40
#define	K_SHOW	41
#define	K_SSTHRESH	42
#define	K_STATIC	43
#define	K_XRESOLVE	44

struct keytab keywords[] = {
	{ "add", K_ADD },
	{ "blackhole", K_BLACKHOLE },
	{ "change", K_CHANGE },
	{ "cloning", K_CLONING },
	{ "delete", K_DELETE },
	{ "dst", K_DST },
	{ "expire", K_EXPIRE },
	{ "flush", K_FLUSH },
	{ "gateway", K_GATEWAY },
	{ "genmask", K_GENMASK },
	{ "get", K_GET },
	{ "host", K_HOST },
	{ "hopcount", K_HOPCOUNT },
	{ "iface", K_IFACE },
	{ "interface", K_INTERFACE },
	{ "ifa", K_IFA },
	{ "ifp", K_IFP },
	{ "inet", K_INET },
	{ "inet6", K_INET6 },
	{ "ipx", K_IPX },
	{ "label", K_LABEL },
	{ "link", K_LINK },
	{ "llinfo", K_LLINFO },
	{ "lock", K_LOCK },
	{ "lockrest", K_LOCKREST },
	{ "monitor", K_MONITOR },
	{ "mpath", K_MPATH },
	{ "mtu", K_MTU },
	{ "net", K_NET },
	{ "netmask", K_NETMASK },
	{ "nostatic", K_NOSTATIC },
	{ "prefixlen", K_PREFIXLEN },
	{ "proto1", K_PROTO1 },
	{ "proto2", K_PROTO2 },
	{ "recvpipe", K_RECVPIPE },
	{ "reject", K_REJECT },
	{ "rtt", K_RTT },
	{ "rttvar", K_RTTVAR },
	{ "sa", K_SA },
	{ "sendpipe", K_SENDPIPE },
	{ "show", K_SHOW },
	{ "ssthresh", K_SSTHRESH },
	{ "static", K_STATIC },
	{ "xresolve", K_XRESOLVE },
	{ 0, 0 }
};

