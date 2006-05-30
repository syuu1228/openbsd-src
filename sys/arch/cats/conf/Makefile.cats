#	$OpenBSD: src/sys/arch/cats/conf/Attic/Makefile.cats,v 1.4 2006/05/30 18:25:42 miod Exp $
#	$OpenBSD: src/sys/arch/cats/conf/Attic/Makefile.cats,v 1.4 2006/05/30 18:25:42 miod Exp $
#	$NetBSD: Makefile.i386,v 1.67 1996/05/11 16:12:11 mycroft Exp $

# Makefile for OpenBSD
#
# This makefile is constructed from a machine description:
#	config machineid
# Most changes should be made in the machine description
#	/sys/arch/cats/conf/``machineid''
# after which you should do
#	config machineid
# Machine generic makefile changes should be made in
#	/sys/arch/cats/conf/Makefile.cats
# after which config should be rerun for all machines of that type.
#
# N.B.: NO DEPENDENCIES ON FOLLOWING FLAGS ARE VISIBLE TO MAKEFILE
#	IF YOU CHANGE THE DEFINITION OF ANY OF THESE RECOMPILE EVERYTHING
#
# -DTRACE	compile in kernel tracing hooks
# -DQUOTA	compile in file system quotas

# DEBUG is set to -g if debugging.
# PROF is set to -pg if profiling.

.include <bsd.own.mk>

MKDEP?=	mkdep
SIZE?=	size
STRIP?=	strip

# source tree is located via $S relative to the compilation directory
.ifndef S
S!=	cd ../../../..; pwd
.endif
CATS=	$S/arch/cats
ARM=	$S/arch/arm

INCLUDES=	-nostdinc -I. -I$S/arch -I$S
CPPFLAGS=	${INCLUDES} ${IDENT} -D_KERNEL -D__cats__
CDIAGFLAGS=	-Werror -Wall -Wstrict-prototypes -Wmissing-prototypes \
		-Wno-uninitialized -Wno-format -Wno-main

CMACHFLAGS= -ffreestanding
#CMACHFLAGS=	-march=armv4 -mtune=strongarm -ffreestanding
.if ${IDENT:M-DNO_PROPOLICE}
CMACHFLAGS+=	-fno-stack-protector
.endif
CMACHFLAGS+=	-msoft-float -fno-builtin-printf -fno-builtin-log

COPTS?=		-O2
CFLAGS=		${DEBUG} ${CDIAGFLAGS} ${CMACHFLAGS} ${COPTS} ${PIPE}
AFLAGS=		-x assembler-with-cpp -D_LOCORE ${CMACHFLAGS} 
#LINKFLAGS=	-Ttext 0xF0000020 -e start --warn-common
#LINKFLAGS=	-T ${CATS}/conf/kern.ldscript
LINKFLAGS=	-T ${CATS}/conf/ldscript.elf
LINKFLAGS+=	--warn-common
STRIPFLAGS=	-g -X -x

HOSTCC= ${CC}
HOSTED_CPPFLAGS=${CPPFLAGS:S/^-nostdinc$//}
HOSTED_CFLAGS=	${CFLAGS}

### find out what to use for libkern
.include "$S/lib/libkern/Makefile.inc"
.ifndef PROF
LIBKERN=	${KERNLIB}
.else
LIBKERN=	${KERNLIB_PROF}
.endif

### find out what to use for libcompat
.include "$S/compat/common/Makefile.inc"
.ifndef PROF
LIBCOMPAT=	${COMPATLIB}
.else
LIBCOMPAT=	${COMPATLIB_PROF}
.endif

# compile rules: rules are named ${TYPE}_${SUFFIX}${CONFIG_DEP}
# where TYPE is NORMAL, DRIVER, or PROFILE; SUFFIX is the file suffix,
# capitalized (e.g. C for a .c file), and CONFIG_DEP is _C if the file
# is marked as config-dependent.

NORMAL_C=	${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} -c $<
NORMAL_C_C=	${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} ${PARAM} -c $<

DRIVER_C=	${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} -c $<
DRIVER_C_C=	${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} ${PARAM} -c $<

NORMAL_S=	${CC} ${AFLAGS} ${CPPFLAGS} -c $<
NORMAL_S_C=	${CC} ${AFLAGS} ${CPPFLAGS} ${PARAM} -c $<

HOSTED_C=	${HOSTCC} ${HOSTED_CFLAGS} ${HOSTED_CPPFLAGS} -c $<

%OBJS

%CFILES

%SFILES

# load lines for config "xxx" will be emitted as:
# xxx: ${SYSTEM_DEP} swapxxx.o
#	${SYSTEM_LD_HEAD}
#	${SYSTEM_LD} swapxxx.o
#	${SYSTEM_LD_TAIL}
SYSTEM_OBJ=	locore.o \
		param.o ioconf.o ${OBJS} ${LIBKERN} ${LIBCOMPAT}
SYSTEM_DEP=	Makefile ${SYSTEM_OBJ}
SYSTEM_LD_HEAD=	rm -f $@
SYSTEM_LD=	@echo ${LD} ${LINKFLAGS} -o $@ '$${SYSTEM_OBJ}' vers.o; \
		${LD} ${LINKFLAGS} -o $@ ${SYSTEM_OBJ} vers.o
SYSTEM_LD_TAIL=	@${SIZE} $@; chmod 755 $@

DEBUG?=
.if ${DEBUG} == "-g"
LINKFLAGS+=	-X
SYSTEM_LD_TAIL+=; \
		echo cp $@ $@.gdb; rm -f $@.gdb; cp $@ $@.gdb; \
		echo ${STRIP} ${STRIPFLAGS} $@; ${STRIP} ${STRIPFLAGS} $@
.else
LINKFLAGS+=	-S -x
.endif

%LOAD

assym.h: $S/kern/genassym.sh ${ARM}/arm/genassym.cf Makefile
	cat ${ARM}/arm/genassym.cf | \
	    sh $S/kern/genassym.sh ${CC} ${CFLAGS} \
	    ${CPPFLAGS} ${PARAM} > assym.h.tmp && \
	    mv -f assym.h.tmp assym.h

param.c: $S/conf/param.c
	rm -f param.c
	cp $S/conf/param.c .

param.o: param.c Makefile
	${NORMAL_C_C}

ioconf.o: ioconf.c
	${NORMAL_C}

newvers: ${SYSTEM_DEP} ${SYSTEM_SWAP_DEP}
	sh $S/conf/newvers.sh
	${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} -c vers.c


clean::
	rm -f eddep *bsd bsd.gdb tags *.[io] [a-z]*.s \
	    [Ee]rrs linterrs makelinks assym.h

lint:
	@lint -hbxncez -Dvolatile= ${CPPFLAGS} ${PARAM} -UKGDB \
	    ${CFILES} ioconf.c param.c | \
	    grep -v 'static function .* unused'

tags:
	@echo "see $S/kern/Makefile for tags"

links:
	egrep '#if' ${CFILES} | sed -f $S/conf/defines | \
	  sed -e 's/:.*//' -e 's/\.c/.o/' | sort -u > dontlink
	echo ${CFILES} | tr -s ' ' '\12' | sed 's/\.c/.o/' | \
	  sort -u | comm -23 - dontlink | \
	  sed 's,.*/\(.*.o\),rm -f \1; ln -s ../GENERIC/\1 \1,' > makelinks
	sh makelinks && rm -f dontlink makelinks

SRCS=	${ARM}/arm/locore.S \
	param.c ioconf.c ${CFILES} ${SFILES}
depend:: .depend
.depend: ${SRCS} assym.h param.c ${APMINC}
	${MKDEP} ${AFLAGS} ${CPPFLAGS} ${ARM}/arm/locore.S
	${MKDEP} -a ${CFLAGS} ${CPPFLAGS} param.c ioconf.c ${CFILES}
	${MKDEP} -a ${AFLAGS} ${CPPFLAGS} ${SFILES}
	cat ${ARM}/arm/genassym.cf | \
	    sh $S/kern/genassym.sh ${MKDEP} -f assym.dep ${CFLAGS} \
	    ${CPPFLAGS} 
	@sed -e 's/.*\.o:.* /assym.h: /' < assym.dep >> .depend
	@rm -f assym.dep


# depend on root or device configuration
autoconf.o conf.o: Makefile
 
# depend on network or filesystem configuration 
uipc_domain.o uipc_proto.o vfs_conf.o: Makefile 
if.o if_tun.o if_loop.o if_ethersubr.o: Makefile
if_arp.o if_ether.o: Makefile
ip_input.o ip_output.o in_pcb.o in_proto.o: Makefile
tcp_subr.o tcp_timer.o tcp_output.o: Makefile

# depend on maxusers
machdep.o: Makefile

# depend on CPU configuration 
locore.o machdep.o: Makefile


locore.o: ${ARM}/arm/locore.S assym.h
	${NORMAL_S}

# The install target can be redefined by putting a
# install-kernel-${MACHINE_NAME} target into /etc/mk.conf
MACHINE_NAME!=  uname -n
install: install-kernel-${MACHINE_NAME}
.if !target(install-kernel-${MACHINE_NAME}})
install-kernel-${MACHINE_NAME}:
	rm -f /obsd
	ln /bsd /obsd
	cp bsd /nbsd
	mv /nbsd /bsd
.endif

%RULES
