#	$OpenBSD: src/share/mk/bsd.sys.mk,v 1.5 1996/03/05 11:13:04 mickey Exp $
#	$NetBSD: bsd.sys.mk,v 1.2 1995/12/13 01:25:07 cgd Exp $
# Overrides used for NetBSD source tree builds.

#CFLAGS+= -Werror

.if defined(PARALLEL)
# Lex
.l:
	${LEX.l} -o${.TARGET:R}.yy.c ${.IMPSRC}
	${LINK.c} -o ${.TARGET} ${.TARGET:R}.yy.c ${LDLIBS} -ll
	rm -f ${.TARGET:R}.yy.c
.l.c:
	${LEX.l} -o${.TARGET} ${.IMPSRC}
.l.o:
	${LEX.l} -o${.TARGET:R}.yy.c ${.IMPSRC}
	${COMPILE.c} -o ${.TARGET} ${.TARGET:R}.yy.c 
	rm -f ${.TARGET:R}.yy.c

# Yacc
.y:
	${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
	${LINK.c} -o ${.TARGET} ${.TARGET:R}.tab.c ${LDLIBS}
	rm -f ${.TARGET:R}.tab.c
.y.c:
	${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
	mv ${.TARGET:R}.tab.c ${.TARGET}
.y.o:
	${YACC.y} -b ${.TARGET:R} ${.IMPSRC}
	${COMPILE.c} -o ${.TARGET} ${.TARGET:R}.tab.c
	rm -f ${.TARGET:R}.tab.c
.endif
