/*	$OpenBSD: src/usr.bin/gprof/hppa.c,v 1.1 2001/03/22 05:18:30 mickey Exp $	*/

#ifndef lint
static char rcsid[] = "$OpenBSD: src/usr.bin/gprof/hppa.c,v 1.1 2001/03/22 05:18:30 mickey Exp $";
#endif /* not lint */

#include "gprof.h"

/*
 * gprof -c isn't currently supported...
 */
void
findcall( parentp , p_lowpc , p_highpc )
    nltype		*parentp;
    unsigned long	p_lowpc;
    unsigned long	p_highpc;
{
}
