/*	$OpenBSD: src/lib/libc/arch/arm/softfloat/nedf2.c,v 1.2 2004/02/01 05:40:52 drahn Exp $	*/
/* $NetBSD: nedf2.c,v 1.1 2000/06/06 08:15:07 bjh21 Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>

flag __nedf2(float64, float64);

flag
__nedf2(float64 a, float64 b)
{

	/* libgcc1.c says a != b */
	return !float64_eq(a, b);
}
