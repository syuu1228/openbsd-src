/*	$OpenBSD: src/games/hack/def.wseg.h,v 1.2 2001/01/28 23:41:43 niklas Exp $*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 *
 *	$NetBSD: def.wseg.h,v 1.3 1995/03/23 08:29:42 cgd Exp $
 */

#ifndef NOWORM
/* worm structure */
struct wseg {
	struct wseg *nseg;
	xchar wx,wy;
	unsigned wdispl:1;
};

#define newseg()	(struct wseg *) alloc(sizeof(struct wseg))
#endif NOWORM
