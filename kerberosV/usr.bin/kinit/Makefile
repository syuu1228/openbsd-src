# $OpenBSD: src/kerberosV/usr.bin/kinit/Makefile,v 1.9 2010/10/13 08:14:22 jsg Exp $

KRBDEPTH=../..
.include <${.CURDIR}/../../Makefile.inc>

PROG=	kinit
MLINKS=	kinit.1 kauth.1

SRCS=	kinit.c
SRCS+=	${PRINTVER}

.PATH:	${KRB5SRC}/kuser

DPADD+=	${LIBKRB5} ${LIBCRYPTO}
LDADD+=	-lkrb5 -lcrypto

.include <bsd.prog.mk>
