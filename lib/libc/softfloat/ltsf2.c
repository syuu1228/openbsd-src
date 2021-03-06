/*	$OpenBSD: src/lib/libc/softfloat/ltsf2.c,v 1.1 2006/11/06 15:11:37 drahn Exp $	*/
/* $NetBSD: ltsf2.c,v 1.1 2000/06/06 08:15:06 bjh21 Exp $ */

/*
 * Written by Ben Harris, 2000.  This file is in the Public Domain.
 */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#include <sys/cdefs.h>

flag __ltsf2(float32, float32);

flag
__ltsf2(float32 a, float32 b)
{

	/* libgcc1.c says -(a < b) */
	return -float32_lt(a, b);
}
