/*
 * Copyright (c) 1995 Jochen Pohl
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Jochen Pohl for
 *     The NetBSD project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 *     $OpenBSD: src/lib/libc/arch/m68k/Attic/protolib.h,v 1.1 1996/03/25 23:31:24 tholo Exp $
 */

#include <float.h>
#include <ieeefp.h>
#include <math.h>
#include <setjmp.h>
#include <stdlib.h>

/* LINTLIBRARY */
/* PROTOLIB1 */

void	*alloca(size_t);
double	fabs(double);
int	__flt_rounds(void);
fp_except fpgetmask(void);
fp_rnd	fpgetround(void);
fp_except fpgetsticky(void);
fp_except fpsetmask(fp_except);
fp_rnd	fpsetround(fp_rnd);
void	_longjmp(jmp_buf, int);
void	longjmp(jmp_buf, int);
double	modf(double, double *);
int	_setjmp(jmp_buf);
int	setjmp(jmp_buf);
void	siglongjmp(sigjmp_buf, int);
int	sigsetjmp(sigjmp_buf, int);
