/*  DO NOT EDIT THIS FILE.

    It has been auto-edited by fixincludes from:

	"fixinc/tests/inc/strings.h"

    This had to be done to correct non-standard usages in the
    original, manufacturer supplied header file.  */

#ifndef FIXINC_WRAP_STRINGS_H_ULTRIX_STRINGS
#define FIXINC_WRAP_STRINGS_H_ULTRIX_STRINGS 1



#if defined( SUNOS_STRLEN_CHECK )
 __SIZE_TYPE__ strlen(); /* string length */
#endif  /* SUNOS_STRLEN_CHECK */


#if defined( ULTRIX_STRINGS_CHECK )
@(#)strings.h   6.1     (ULTRIX)

#endif  /* ULTRIX_STRINGS_CHECK */


#if defined( ULTRIX_STRINGS2_CHECK )
@(#)strings.h      6.1     (ULTRIX)
	strncmp( const char *__s1, const char *__s2, size_t __n );

extern int
	strcasecmp( const char *__s1, const char *__s2),
	strncasecmp( const char *__s1, const char *__s2, size_t __n );
	strncmp();
extern int
	strcasecmp(),
	strncasecmp();

#endif  /* ULTRIX_STRINGS2_CHECK */

#endif  /* FIXINC_WRAP_STRINGS_H_ULTRIX_STRINGS */
