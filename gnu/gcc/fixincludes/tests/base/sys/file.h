/*  DO NOT EDIT THIS FILE.

    It has been auto-edited by fixincludes from:

	"fixinc/tests/inc/sys/file.h"

    This had to be done to correct non-standard usages in the
    original, manufacturer supplied header file.  */



#if defined( HP_SYSFILE_CHECK )
extern void foo(struct file *, ...); /* HPUX_SOURCE - bad varargs */
#endif  /* HP_SYSFILE_CHECK */


#if defined( ULTRIX_IFDEF_CHECK )
#if defined(KERNEL) && defined( mumbojumbo )
int oops;
#endif
#endif  /* ULTRIX_IFDEF_CHECK */
