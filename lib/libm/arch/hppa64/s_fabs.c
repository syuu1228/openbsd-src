/*	$OpenBSD: src/lib/libm/arch/hppa64/s_fabs.c,v 1.1 2011/07/08 19:21:42 martynas Exp $	*/

/*
 * Written by Miodrag Vallat.  Public domain
 */

double
fabs(double val)
{

	__asm__ __volatile__("fabs,dbl %0,%0" : "+f" (val));
	return (val);
}
