#	$OpenBSD: src/usr.sbin/npppd/npppd/Makefile,v 1.10 2012/05/08 13:18:37 yasuoka Exp $

NPPPD_COMMON_DIR=	${.CURDIR}/../common

PROG=		npppd
NOMAN=		noman
#MAN=		npppd.8 npppd.conf.5 npppd-users.csv.5
CPPFLAGS+=	-I${NPPPD_COMMON_DIR} -I${.CURDIR}
SRCS=		ccp.c chap.c chap_ms.c fsm.c ipcp.c lcp.c
SRCS+=		mppe.c pap.c ppp.c
SRCS+=		npppd.c npppd_config.c npppd_subr.c npppd_auth.c npppd_iface.c
SRCS+=		config_helper.c slist.c hash.c properties.c bytebuf.c
SRCS+=		debugutil.c csvreader.c net_utils.c radish.c time_utils.c
SRCS+=		npppd_pool.c addr_range.c
SRCS+=		radius+.c radius_req.c npppd_radius.c
SRCS+=		recvfromto.c
SRCS+=		privsep.c
#SRCS+=		ipsec_util.c
SRCS+=		npppd_ctl.c

CPPFLAGS+=	-DUSE_NPPPD_PPTP -I${.CURDIR}/../pptp
SRCS+=		pptp_call.c pptp_ctrl.c pptp_subr.c pptpd.c
.PATH:	${.CURDIR}/../pptp

CPPFLAGS+=	-DUSE_NPPPD_L2TP -I${.CURDIR}/../l2tp
SRCS+=		l2tp_call.c l2tp_ctrl.c l2tp_subr.c l2tpd.c
.PATH:	${.CURDIR}/../l2tp

CPPFLAGS+=	-DUSE_NPPPD_PPPOE -I${.CURDIR}/../pppoe
SRCS+=		pppoe_session.c pppoed.c
.PATH:	${.CURDIR}/../pppoe

CPPFLAGS+=	-D__COPYRIGHT\(x\)= -D__RCSID\(x\)=
CPPFLAGS+=	-DUSE_NPPPD_MPPE
CPPFLAGS+=	-DUSE_NPPPD_PIPEX
CPPFLAGS+=	-DUSE_NPPPD_RADIUS

CPPFLAGS+=	-DGENERIC_USE -DRADISH

LDADD+=		-levent -lcrypto
DPADD+=		${LIBEVENT} ${LIBCRYPTO}

.ifdef	DEBUG
CPPFLAGS+=	-DDEBUG=1
CPPFLAGS+=	-DNPPPD_DEBUG=1 -DNPPPD_TUN_DEBUG=1 -DNPPPD_CONFIG_DEBUG=1
CPPFLAGS+=	-DRADIUS_REQ_DEBUG=1 -DPPP_DEBUG=1
CPPFLAGS+=	-DLCP_DEBUG=1 -DFSM_DEBUG=1 -DMPPE_DEBUG=1 -DTUNDEV_DEBUG=1
CPPFLAGS+=	-DIPCP_DEBUG=1 -DNPPPD_INTERFACE_DEBUG=1 -DNPPPD_POOL_DEBUG=1
CPPFLAGS+=	-DNPPPD_AUTH_DEBUG=1
CPPFLAGS+=	-DPAP_DEBUG=1 -DCHAP_DEBUG=1
CPPFLAGS+=	-DNPPPD_IFACE_DEBUG
.endif

.include <bsd.prog.mk>
.PATH: ${.CURDIR}/../common
