/*	$OpenBSD: src/usr.bin/compile_et/Attic/compiler.h,v 1.1 1996/11/11 05:06:34 downsj Exp $	*/

/*
 * definitions common to the source files of the error table compiler
 */

#ifndef __STDC__
/* loser */
#undef const
#define const
#endif

enum lang {
    lang_C,			/* ANSI C (default) */
    lang_KRC,			/* C: ANSI + K&R */
    lang_CPP			/* C++ */
};

int debug;			/* dump debugging info? */
char *filename;			/* error table source */
enum lang language;
const char *whoami;
