/*	$OpenBSD: src/sys/arch/mvmeppc/include/pmap.h,v 1.4 2001/09/10 16:44:52 mickey Exp $	*/

#include <powerpc/pmap.h>

paddr_t vtophys __P((vaddr_t));
