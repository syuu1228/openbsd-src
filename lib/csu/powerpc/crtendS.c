/*	$OpenBSD: src/lib/csu/powerpc/Attic/crtendS.c,v 1.1 2000/06/13 04:07:03 rahnds Exp $	*/
/*	$NetBSD: crtend.c,v 1.1 1997/04/16 19:38:24 thorpej Exp $	*/

#include <sys/cdefs.h>

static void (*__CTOR_LIST__[1]) __P((void))
    __attribute__((section(".ctors"))) = { (void *)0 };		/* XXX */
static void (*__DTOR_LIST__[1]) __P((void))
    __attribute__((section(".dtors"))) = { (void *)0 };		/* XXX */
