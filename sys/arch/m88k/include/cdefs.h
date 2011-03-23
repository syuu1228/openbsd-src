/*	$OpenBSD: src/sys/arch/m88k/include/cdefs.h,v 1.4 2011/03/23 16:54:35 pirofti Exp $ */

/*
 * Written by J.T. Conklin <jtc@wimsey.com> 01/17/95.
 * Public domain.
 */

#ifndef	_M88K_CDEFS_H_
#define	_M88K_CDEFS_H_

#if defined(lint)
#define __indr_reference(sym,alias)	__lint_equal__(sym,alias)
#define __warn_references(sym,msg)
#define __weak_alias(alias,sym)		__lint_equal__(sym,alias)
#elif defined(__GNUC__) && defined(__STDC__)
#define __indr_reference(sym,alias)			\
	__asm__(".stabs \"_" #alias "\",11,0,0,0");	\
	__asm__(".stabs \"_" #sym "\",1,0,0,0")
#define __warn_references(sym,msg)			\
	__asm__(".stabs \"" msg "\",30,0,0,0");		\
	__asm__(".stabs \"_" #sym "\",1,0,0,0")
#define __weak_alias(alias,sym)				\
	__asm__(".weak _" #alias "; _" #alias "= _" __STRING(sym))
#endif

#endif /* _M88K_CDEFS_H_ */
