dnl $KTH: check-getpwnam_r-posix.m4,v 1.5 2005/05/19 11:24:48 lha Exp $
dnl
dnl check for getpwnam_r, and if it's posix or not

AC_DEFUN([AC_CHECK_GETPWNAM_R_POSIX],[
AC_FIND_FUNC_NO_LIBS(getpwnam_r,c_r)
if test "$ac_cv_func_getpwnam_r" = yes; then
	AC_CACHE_CHECK(if getpwnam_r is posix,ac_cv_func_getpwnam_r_posix,
	ac_libs="$LIBS"
	LIBS="$LIBS $LIB_getpwnam_r"
	AC_RUN_IFELSE([AC_LANG_SOURCE([[
#define _POSIX_PTHREAD_SEMANTICS
#include <pwd.h>
int main()
{
	struct passwd pw, *pwd;
	return getpwnam_r("", &pw, NULL, 0, &pwd) < 0;
}
]])],[ac_cv_func_getpwnam_r_posix=yes],[ac_cv_func_getpwnam_r_posix=no],[:])
LIBS="$ac_libs")
if test "$ac_cv_func_getpwnam_r_posix" = yes; then
	AC_DEFINE(POSIX_GETPWNAM_R, 1, [Define if getpwnam_r has POSIX flavour.])
fi
fi
])
