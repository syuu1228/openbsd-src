/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char rcsid[] = "$OpenBSD: src/lib/libc/arch/sparc/gen/fpgetround.c,v 1.2 1996/08/19 08:17:30 tholo Exp $";
#endif /* LIBC_SCCS and not lint */

#include <ieeefp.h>

fp_rnd
fpgetround()
{
	int x;

	__asm__("st %%fsr,%0" : "=m" (*&x));
	return (x >> 30) & 0x03;
}
