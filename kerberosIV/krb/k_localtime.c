/*
 * $Source: /usr/src/kerberosIV/lib/krb/RCS/k_localtime.c,v $
 *
 * $Locker:  $
 */

/*
 * Copyright 1987, 1988 by the Student Information Processing Board
 *	of the Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is
 * hereby granted, provided that the above copyright notice
 * appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation,
 * and that the names of M.I.T. and the M.I.T. S.I.P.B. not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 * M.I.T. and the M.I.T. S.I.P.B. make no representations about
 * the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 */

#include <kerberosIV/krb.h>

#include <time.h>

struct tm *
k_localtime(tp)
	u_int32_t *tp;
{
  time_t t;
  t = *tp;
  return localtime(&t);
}
