/* ==== posix.h ============================================================
 * Copyright (c) 1995 by Chris Provenzano, proven@athena.mit.edu	
 *
 * Description : Convert an IRIX-5.2 system to a more or less POSIX system.
 *
 * $Id: posix-irix-5.2.h,v 1.1 1995/08/30 22:19:20 proven Exp $
 *
 *  1.00 95/06/01 proven
 *      -Started coding this file.
 */

#ifndef _PTHREAD_POSIX_H_
#define _PTHREAD_POSIX_H_

#include <sys/cdefs.h>

/* More stuff for compiling */
#if defined(__GNUC__)
#define __INLINE                extern inline
#else
#define __INLINE                static
#endif

/* Make sure we have size_t defined */
#include <pthread/types.h>

#ifndef __WAIT_STATUS
#define __WAIT_STATUS	int *
#endif

#endif
