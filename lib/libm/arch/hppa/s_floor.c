/*
 * Written by Michael Shalayeff. Public Domain
 */

#if defined(LIBM_SCCS) && !defined(lint)
static char rcsid[] = "$OpenBSD: src/lib/libm/arch/hppa/s_floor.c,v 1.5 2009/06/30 20:20:20 kettenis Exp $";
#endif

#include <sys/types.h>
#include <machine/ieeefp.h>
#include "math.h"

double
floor(double x)
{
	u_int64_t ofpsr, fpsr;

	__asm__ __volatile__("fstds %%fr0,0(%1)" : "=m" (ofpsr) : "r" (&ofpsr));
	fpsr = ofpsr | ((u_int64_t)FP_RM << (9 + 32));
	__asm__ __volatile__("fldds 0(%0), %%fr0" :: "r" (&fpsr));

	__asm__ __volatile__("frnd,dbl %0,%0" : "+f" (x));

	__asm__ __volatile__("fldds 0(%0), %%fr0" :: "r" (&ofpsr));
	return (x);
}
