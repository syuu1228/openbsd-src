/*
 * Copyright (c) 1983, 1993, 2001
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "hertz.h"


int
hertz ()
{
#ifdef HERTZ
  return HERTZ;
#else /* ! defined (HERTZ) */
#ifdef HAVE_SETITIMER
  struct itimerval tim;

  tim.it_interval.tv_sec = 0;
  tim.it_interval.tv_usec = 1;
  tim.it_value.tv_sec = 0;
  tim.it_value.tv_usec = 0;
  setitimer (ITIMER_REAL, &tim, 0);
  setitimer (ITIMER_REAL, 0, &tim);
  if (tim.it_interval.tv_usec >= 2)
    {
      return 1000000 / tim.it_interval.tv_usec;
    }
#endif /* ! defined (HAVE_SETITIMER) */
#if defined (HAVE_SYSCONF) && defined (_SC_CLK_TCK)
  return sysconf (_SC_CLK_TCK);
#else /* ! defined (HAVE_SYSCONF) || ! defined (_SC_CLK_TCK) */
#ifdef __MSDOS__
  return 18;
#else  /* ! defined (__MSDOS__) */
  return HZ_WRONG;
#endif /* ! defined (__MSDOS__) */
#endif /* ! defined (HAVE_SYSCONF) || ! defined (_SC_CLK_TCK) */
#endif /* ! defined (HERTZ) */
}
