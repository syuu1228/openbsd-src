/*	$OpenBSD: src/usr.bin/mg/macro.h,v 1.4 2001/05/24 03:05:24 mickey Exp $	*/

/* definitions for keyboard macros */

#ifndef EXTERN
#define EXTERN extern
#define INIT(i)
#endif

#define MAXMACRO 256		/* maximum functs in a macro */

EXTERN int inmacro INIT(FALSE);
EXTERN int macrodef INIT(FALSE);
EXTERN int macrocount INIT(0);

EXTERN union {
	PF	m_funct;
	int	m_count;	/* for count-prefix	 */
} macro[MAXMACRO];

EXTERN LINE	*maclhead INIT(NULL);
EXTERN LINE	*maclcur;

#undef	EXTERN
#undef	INIT
