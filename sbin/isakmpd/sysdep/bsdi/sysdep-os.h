/*	$OpenBSD: src/sbin/isakmpd/sysdep/bsdi/Attic/sysdep-os.h,v 1.5 2003/08/06 11:20:00 markus Exp $	*/

/*
 * Copyright (c) 1998, 1999 Niklas Hallqvist.  All rights reserved.
 * Copyright (c) 2000 H\xe5kan Olsson.  All rights reserved.
 * Copyright (c) 2001 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYSDEP_OS_H_

#define _SYSDEP_OS_H_

#define KAME

#include <netinet6/ipsec.h>

/* in_port_t */
#include <netinet/in.h>

#define	timersub(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
		if ((vvp)->tv_usec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_usec += 1000000;			\
		}							\
	} while (0)

#ifndef CPI_RESERVED_MIN
/* Reserved CPI numbers */
#define CPI_RESERVED_MIN        1
#define CPI_RESERVED_MAX        255
#define CPI_PRIVATE_MIN         61440
#define CPI_PRIVATE_MAX         65535
#endif

#if !defined(SADB_X_EALG_CAST) && defined(SADB_X_EALG_CAST128CBC)
#define SADB_X_EALG_CAST SADB_X_EALG_CAST128CBC
#endif

#if !defined(SADB_X_EALG_BLF) && defined(SADB_X_EALG_BLOWFISHCBC)
#define SADB_X_EALG_BLF SADB_X_EALG_BLOWFISHCBC
#endif

#endif /* _SYSDEP_OS_H_ */
