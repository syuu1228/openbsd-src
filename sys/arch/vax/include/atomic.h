/*	$OpenBSD: src/sys/arch/vax/include/atomic.h,v 1.2 2007/02/19 17:18:43 deraadt Exp $	*/

/* Public Domain */

#ifndef __VAX_ATOMIC_H__
#define __VAX_ATOMIC_H__

#if defined(_KERNEL)

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

#endif /* defined(_KERNEL) */
#endif /* __VAX_ATOMIC_H__ */
