/*	$OpenBSD: src/lib/libc/arch/hppa64/gen/nan.c,v 1.1 2008/07/24 09:31:06 martynas Exp $	*/

/* Written by Martynas Venckus.  Public Domain. */

#include <math.h>

/* bytes for qNaN on a hppa64 (IEEE single format) */
char __nan[] __attribute__((__aligned__(sizeof(float)))) =
					{ 0x7f, 0xa0, 0, 0 };
