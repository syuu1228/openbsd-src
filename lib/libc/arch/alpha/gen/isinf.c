/*	$OpenBSD: src/lib/libc/arch/alpha/gen/Attic/isinf.c,v 1.3 1996/11/13 21:20:19 niklas Exp $	*/
/*	$NetBSD: isinf.c,v 1.1 1995/02/10 17:50:23 cgd Exp $	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$OpenBSD: src/lib/libc/arch/alpha/gen/Attic/isinf.c,v 1.3 1996/11/13 21:20:19 niklas Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <machine/ieee.h>
#include <math.h>

int
isnan(d)
	double d;
{
	register struct ieee_double *p = (struct ieee_double *)&d;

	return (p->dbl_exp == DBL_EXP_INFNAN &&
	    (p->dbl_frach || p->dbl_fracl));
}

int
isinf(d)
	double d;
{
	register struct ieee_double *p = (struct ieee_double *)&d;

	return (p->dbl_exp == DBL_EXP_INFNAN &&
	    !p->dbl_frach && !p->dbl_fracl);
}
