/*	$OpenBSD: src/sys/arch/m88k/include/atomic.h,v 1.1 2007/02/06 17:13:33 art Exp $	*/

/* Public Domain */

#ifndef __M88K_ATOMIC_H__
#define __M88K_ATOMIC_H__

static __inline void
atomic_setbits_int(__volatile unsigned int *uip, unsigned int v)
{
	*uip |= v;
}

static __inline void
atomic_clearbits_int(__volatile unsigned int *uip, unsigned int v)
{
	*uip &= ~v;
}

#endif
