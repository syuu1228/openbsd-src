#	$OpenBSD: src/sys/arch/amd64/stand/biosboot/Makefile,v 1.3 2004/10/09 21:15:21 mickey Exp $

MAN=	biosboot.8

.if ${MACHINE} == "amd64"
PROG=	biosboot
SRCS=	biosboot.S
LD=ld
LDFLAGS=-melf_i386 -nostdlib -Ttext 0 -N -x -Bstatic
LDFLAGS+=-L/usr/libdata 
INSTALL_STRIP=
SADIR=	${.CURDIR}/..

${PROG}: $(OBJS) $(DPADD)
	@rm -f $(PROG)
	$(LD) $(LDFLAGS) -o $(PROG) $(OBJS) $(LDADD)
	@size $(PROG)

CPPFLAGS+=-DLOADADDR=$(LOADADDR) -DLINKADDR=$(LINKADDR) -DBOOTMAGIC=$(BOOTMAGIC)
CPPFLAGS+=${DEBUGFLAGS}
AFLAGS+=-m32 # -Wa,-a
.else
NOPROG=
.endif

.include <bsd.prog.mk>

