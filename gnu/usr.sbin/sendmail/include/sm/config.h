/*
 * Copyright (c) 2000-2001 Sendmail, Inc. and its suppliers.
 *	All rights reserved.
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the sendmail distribution.
 *
 *	$Sendmail: config.h,v 1.42 2001/06/17 21:31:11 ca Exp $
 */

/*
**  libsm configuration macros.
**  The values of these macros are platform dependent.
**  The default values are given here.
**  If the default is incorrect, then the correct value can be specified
**  in the m4 configuration file in devtools/OS.
*/

#ifndef SM_CONFIG_H
# define SM_CONFIG_H

#  include "sm_os.h"

/*
**  SM_CONF_STDBOOL_H is 1 if <stdbool.h> exists
*/

# ifndef SM_CONF_STDBOOL_H
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#   define SM_CONF_STDBOOL_H		1
#  else /* defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L */
#   define SM_CONF_STDBOOL_H		0
#  endif /* defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L */
# endif /* ! SM_CONF_STDBOOL_H */

/*
**  Configuration macros that specify how __P is defined.
*/

# ifndef SM_CONF_SYS_CDEFS_H
#  define SM_CONF_SYS_CDEFS_H		0
# endif /* ! SM_CONF_SYS_CDEFS_H */

/*
**  SM_CONF_STDDEF_H is 1 if <stddef.h> exists
*/

# ifndef SM_CONF_STDDEF_H
#  define SM_CONF_STDDEF_H		1
# endif /* ! SM_CONF_STDDEF_H */

/*
**  Configuration macro that specifies whether strlcpy/strlcat are available.
**  Note: this is the default so that the libsm version (optimized) will
**  be used by default (sm_strlcpy/sm_strlcat).
*/

# ifndef SM_CONF_STRL
#  define SM_CONF_STRL			0
# endif /* ! SM_CONF_STRL */

/*
**  Configuration macro indicating that setitimer is available
*/

# ifndef SM_CONF_SETITIMER
#  define SM_CONF_SETITIMER		1
# endif /* ! SM_CONF_SETITIMER */

/*
**  Does <sys/types.h> define uid_t and gid_t?
*/

# ifndef SM_CONF_UID_GID
#  define SM_CONF_UID_GID		1
# endif /* ! SM_CONF_UID_GID */

/*
**  Does <sys/types.h> define ssize_t?
*/
# ifndef SM_CONF_SSIZE_T
#  define SM_CONF_SSIZE_T		1
# endif /* ! SM_CONF_SSIZE_T */

/*
**  Does the C compiler support long long?
*/

# ifndef SM_CONF_LONGLONG
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#   define SM_CONF_LONGLONG		1
#  else /* defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L */
#   if defined(__GNUC__)
#    define SM_CONF_LONGLONG		1
#   else /* defined(__GNUC__) */
#    define SM_CONF_LONGLONG		0
#   endif /* defined(__GNUC__) */
#  endif /* defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L */
# endif /* ! SM_CONF_LONGLONG */

/*
**  Does <sys/types.h> define quad_t and u_quad_t?
**  We only care if long long is not available.
*/

# ifndef SM_CONF_QUAD_T
#  define SM_CONF_QUAD_T		0
# endif /* ! SM_CONF_QUAD_T */

/*
**  Configuration macro indicating that shared memory is available
*/

# ifndef SM_CONF_SHM
#  define SM_CONF_SHM		0
# endif /* ! SM_CONF_SHM */

/*
**  Does <setjmp.h> define sigsetjmp?
*/

# ifndef SM_CONF_SIGSETJMP
#  define SM_CONF_SIGSETJMP	1
# endif /* ! SM_CONF_SIGSETJMP */

/*
**  Does <sysexits.h> exist, and define the EX_* macros with values
**  that differ from the default BSD values in <sm/sysexits.h>?
*/

# ifndef SM_CONF_SYSEXITS_H
#  define SM_CONF_SYSEXITS_H	0
# endif /* ! SM_CONF_SYSEXITS_H */

/* has memchr() prototype? (if not: needs memory.h) */
# ifndef SM_CONF_MEMCHR
#  define SM_CONF_MEMCHR	1
# endif /* ! SM_CONF_MEMCHR */

/* try LLONG tests in libsm/t-types.c? */
# ifndef SM_CONF_TEST_LLONG
#  define SM_CONF_TEST_LLONG	1
# endif /* !SM_CONF_TEST_LLONG */

#endif /* ! SM_CONFIG_H */
