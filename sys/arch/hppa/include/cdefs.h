/*	$OpenBSD: src/sys/arch/hppa/include/cdefs.h,v 1.6 2002/08/28 22:30:42 mickey Exp $	*/
/*	$NetBSD: cdefs.h,v 1.5 1996/10/12 18:08:12 cgd Exp $	*/

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
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

#ifndef _MACHINE_CDEFS_H_
#define	_MACHINE_CDEFS_H_

#define	__weak_alias(alias,sym)						\
    __asm__(".export " __STRING(alias) ", entry ! .weak " __STRING(alias) " ! " __STRING(alias) " = " __STRING(sym))
#define	__warn_references(sym,msg)					\
    __asm__(".section .gnu.warning." __STRING(sym) " ! .ascii \"" msg "\" ; .text")

#endif /* !_MACHINE_CDEFS_H_ */
