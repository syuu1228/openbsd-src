#	$OpenBSD: src/sbin/wsconsctl/Makefile,v 1.35 2012/06/20 18:45:44 matthew Exp $

.if ${MACHINE} == "alpha" || ${MACHINE} == "amd64" || \
    ${MACHINE} == "armish" || ${MACHINE} == "hp300" || \
    ${MACHINE} == "hppa" || ${MACHINE} == "hppa64" || \
    ${MACHINE} == "i386" || ${MACHINE} == "landisk" || \
    ${MACHINE} == "loongson" || ${MACHINE} == "luna88k" || \
    ${MACHINE} == "macppc" || \
    ${MACHINE} == "palm" || ${MACHINE} == "sgi" || \
    ${MACHINE} == "socppc" || ${MACHINE} == "sparc" || \
    ${MACHINE} == "sparc64" || ${MACHINE} == "vax" || \
    ${MACHINE} == "zaurus"

PROG=	wsconsctl
SRCS=	display.c keyboard.c keysym.c map_parse.y map_scan.l \
	mouse.c util.c wsconsctl.c

CPPFLAGS+=	-I${.CURDIR} -I.
CLEANFILES+=	keysym.h y.tab.h

beforedepend: keysym.h

keysym.h: mkkeysym.sh ${DESTDIR}/usr/include/dev/wscons/wsksymdef.h
	sh ${.CURDIR}/mkkeysym.sh \
	  ${DESTDIR}/usr/include/dev/wscons/wsksymdef.h > keysym.h
.else

NOPROG=yes

.endif

MAN=	wsconsctl.8

.include <bsd.prog.mk>
