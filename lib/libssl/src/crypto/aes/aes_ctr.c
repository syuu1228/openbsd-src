/* crypto/aes/aes_ctr.c -*- mode:C; c-file-style: "eay" -*- */
/* ====================================================================
 * Copyright (c) 1998-2002 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 */

#include <assert.h>
#include <openssl/aes.h>
#include "aes_locl.h"

/* NOTE: CTR mode is big-endian.  The rest of the AES code
 * is endian-neutral. */

/* increment counter (128-bit int) by 2^64 */
static void AES_ctr128_inc(unsigned char *counter) {
	unsigned long c;

	/* Grab 3rd dword of counter and increment */
#ifdef L_ENDIAN
	c = GETU32(counter + 8);
	c++;
	PUTU32(counter + 8, c);
#else
	c = GETU32(counter + 4);
	c++;
	PUTU32(counter + 4, c);
#endif

	/* if no overflow, we're done */
	if (c)
		return;

	/* Grab top dword of counter and increment */
#ifdef L_ENDIAN
	c = GETU32(counter + 12);
	c++;
	PUTU32(counter + 12, c);
#else
	c = GETU32(counter +  0);
	c++;
	PUTU32(counter +  0, c);
#endif

}

/* The input encrypted as though 128bit counter mode is being
 * used.  The extra state information to record how much of the
 * 128bit block we have used is contained in *num;
 */
void AES_ctr128_encrypt(const unsigned char *in, unsigned char *out,
	const unsigned long length, const AES_KEY *key,
	unsigned char *counter, unsigned int *num) {

	unsigned int n;
	unsigned long l=length;
	unsigned char tmp[AES_BLOCK_SIZE];

	assert(in && out && key && counter && num);

	n = *num;

	while (l--) {
		if (n == 0) {
			AES_encrypt(counter, tmp, key);
			AES_ctr128_inc(counter);
		}
		*(out++) = *(in++) ^ tmp[n];
		n = (n+1) % AES_BLOCK_SIZE;
	}

	*num=n;
}
