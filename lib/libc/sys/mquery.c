/*	$OpenBSD: src/lib/libc/sys/mquery.c,v 1.2 2003/04/25 18:30:18 drahn Exp $	*/
/*
 *	Written by Artur Grabowski <art@openbsd.org> Public Domain
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/syscall.h>

/*
 * This function provides 64-bit offset padding.
 */
void *
mquery(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
	return((void *)(long)__syscall((quad_t)SYS_mquery, addr, len, prot,
	    flags, fd, 0, offset));
}
