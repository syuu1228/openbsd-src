/*	$OpenBSD: src/games/hack/def.gold.h,v 1.2 2001/01/28 23:41:42 niklas Exp $*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 *
 *	$NetBSD: def.gold.h,v 1.3 1995/03/23 08:29:27 cgd Exp $
 */

struct gold {
	struct gold *ngold;
	xchar gx,gy;
	long amount;
};

extern struct gold *fgold;
struct gold *g_at();
#define newgold()	(struct gold *) alloc(sizeof(struct gold))
