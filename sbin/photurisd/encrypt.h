/* $OpenBSD: src/sbin/photurisd/Attic/encrypt.h,v 1.3 2001/01/28 22:45:07 niklas Exp $ */
/*
 * Copyright 1997-2000 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
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
 *      This product includes software developed by Niels Provos.
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
 */
/*
 * encrypt.h:
 * prototypes for photuris_packet_encrypt.c
 */

#ifndef _ENCRYPT_H_ 
#define _ENCRYPT_H_ 
 
#include "state.h" 
 
#undef EXTERN 
  
#ifdef _ENCRYPT_C_ 
#define EXTERN 
#else 
#define EXTERN extern 
#endif

EXTERN int packet_create_padding(struct stateob *st, u_int16_t size, 
				 u_int8_t *padd,  u_int16_t *rsize);
EXTERN int packet_encrypt(struct stateob *st, 
			  u_int8_t *payload, u_int16_t payloadlen);
EXTERN int packet_decrypt(struct stateob *st, 
			  u_int8_t *payload, u_int16_t *payloadlen);

#endif /* _ENCRYPT_H_ */
