/*	$OpenBSD: src/games/hack/def.func_tab.h,v 1.2 2001/01/28 23:41:42 niklas Exp $*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 *
 *	$NetBSD: def.func_tab.h,v 1.3 1995/03/23 08:29:23 cgd Exp $
 */

struct func_tab {
	char f_char;
	int (*f_funct)();
};

extern struct func_tab cmdlist[];

struct ext_func_tab {
	char *ef_txt;
	int (*ef_funct)();
};

extern struct ext_func_tab extcmdlist[];
