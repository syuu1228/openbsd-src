/* crypto/des/qud_cksm.c */
/* Copyright (C) 1995-1997 Eric Young (eay@mincom.oz.au)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@mincom.oz.au).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@mincom.oz.au).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@mincom.oz.au)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@mincom.oz.au)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

/* From "Message Authentication"  R.R. Jueneman, S.M. Matyas, C.H. Meyer
 * IEEE Communications Magazine Sept 1985 Vol. 23 No. 9 p 29-40
 * This module in only based on the code in this paper and is
 * almost definitely not the same as the MIT implementation.
 */
#include "des_locl.h"

/* bug fix for dos - 7/6/91 - Larry hughes@logos.ucs.indiana.edu */
#define Q_B0(a)	(((DES_LONG)(a)))
#define Q_B1(a)	(((DES_LONG)(a))<<8)
#define Q_B2(a)	(((DES_LONG)(a))<<16)
#define Q_B3(a)	(((DES_LONG)(a))<<24)

/* used to scramble things a bit */
/* Got the value MIT uses via brute force :-) 2/10/90 eay */
#define NOISE	((DES_LONG)83653421L)

DES_LONG des_quad_cksum(input, output, length, out_count, seed)
des_cblock (*input);
des_cblock (*output);
long length;
int out_count;
des_cblock (*seed);
	{
	DES_LONG z0,z1,t0,t1;
	int i;
	long l;
	unsigned char *cp;
	unsigned char *lp;

	if (out_count < 1) out_count=1;
	lp=(unsigned char *)output;

	z0=Q_B0((*seed)[0])|Q_B1((*seed)[1])|Q_B2((*seed)[2])|Q_B3((*seed)[3]);
	z1=Q_B0((*seed)[4])|Q_B1((*seed)[5])|Q_B2((*seed)[6])|Q_B3((*seed)[7]);

	for (i=0; ((i<4)&&(i<out_count)); i++)
		{
		cp=(unsigned char *)input;
		l=length;
		while (l > 0)
			{
			if (l > 1)
				{
				t0= (DES_LONG)(*(cp++));
				t0|=(DES_LONG)Q_B1(*(cp++));
				l--;
				}
			else
				t0= (DES_LONG)(*(cp++));
			l--;
			/* add */
			t0+=z0;
			t0&=0xffffffffL;
			t1=z1;
			/* square, well sort of square */
			z0=((((t0*t0)&0xffffffffL)+((t1*t1)&0xffffffffL))
				&0xffffffffL)%0x7fffffffL; 
			z1=((t0*((t1+NOISE)&0xffffffffL))&0xffffffffL)%0x7fffffffL;
			}
		if (lp != NULL)
			{
			/* I believe I finally have things worked out.
			 * The MIT library assumes that the checksum
			 * is one huge number and it is returned in a
			 * host dependant byte order.
			 */
			static DES_LONG ltmp=1;
			static unsigned char *c=(unsigned char *)&ltmp;

			if (c[0])
				{
				l2c(z0,lp);
				l2c(z1,lp);
				}
			else
				{
				lp=output[out_count-i-1];
				l2n(z1,lp);
				l2n(z0,lp);
				}
			}
		}
	return(z0);
	}

