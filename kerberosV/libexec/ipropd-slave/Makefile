# $OpenBSD: src/kerberosV/libexec/ipropd-slave/Makefile,v 1.10 2010/10/13 08:14:22 jsg Exp $

KRBDEPTH=../..
.include <${.CURDIR}/../../Makefile.inc>

PROG=	ipropd-slave
NOMAN=

SRCS=	ipropd_slave.c
SRCS+=	${PRINTVER}

.PATH:	${KRB5SRC}/lib/kadm5

CFLAGS+=-I${KRB5SRC}/lib/krb5 -I${KRB5SRC}/lib/kadm5

DPADD+= ${LIBKADM5SRV} ${LIBKRB5} ${LIBCRYPTO} ${LIBUTIL}
LDADD+=	-lkadm5srv -lkrb5 -lcrypto -lutil

.include <bsd.prog.mk>
