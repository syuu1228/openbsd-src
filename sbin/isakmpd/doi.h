/*	$OpenBSD: src/sbin/isakmpd/doi.h,v 1.3 1998/11/17 11:10:10 niklas Exp $	*/
/*	$EOM: doi.h,v 1.22 1998/10/11 12:01:03 niklas Exp $	*/

/*
 * Copyright (c) 1998 Niklas Hallqvist.  All rights reserved.
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
 *	This product includes software developed by Ericsson Radio Systems.
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
 * This code was written under funding by Ericsson Radio Systems.
 */

#ifndef _DOI_H_
#define _DOI_H_

#include <sys/types.h>
#include <sys/queue.h>

struct exchange;
struct keystate;
struct message;
struct proto;
struct sa;

/* XXX This structure needs per-field commenting.  */
struct doi {
  LIST_ENTRY (doi) link;
  u_int8_t id;

  /* Size of DOI-specific exchange data.  */
  size_t exchange_size;

  /* Size of DOI-specific security association data.  */
  size_t sa_size;

  /* Size of DOI-specific protocol data.  */
  size_t proto_size;

  int (*debug_attribute) (u_int16_t, u_int8_t *, u_int16_t, void *);
  void (*delete_spi) (struct sa *, struct proto *, int);
  u_int16_t *(*exchange_script) (u_int8_t);
  void (*finalize_exchange) (struct message *);
  void (*free_exchange_data) (void *);
  void (*free_proto_data) (void *);
  void (*free_sa_data) (void *);
  struct keystate *(*get_keystate) (struct message *);
  u_int8_t *(*get_spi) (size_t *, u_int8_t, struct message *);
  int (*is_attribute_incompatible) (u_int16_t, u_int8_t *, u_int16_t, void *);
  void (*setup_situation) (u_int8_t *);
  size_t (*situation_size) (void);
  u_int8_t (*spi_size) (u_int8_t);
  int (*validate_attribute) (u_int16_t, u_int8_t *, u_int16_t, void *);
  int (*validate_exchange) (u_int8_t);
  int (*validate_id_information) (u_int8_t, u_int8_t *, u_int8_t *, size_t,
				  struct exchange *);
  int (*validate_key_information) (u_int8_t *, size_t);
  int (*validate_notification) (u_int16_t);
  int (*validate_proto) (u_int8_t);
  int (*validate_situation) (u_int8_t *, size_t *);
  int (*validate_transform_id) (u_int8_t, u_int8_t);
  int (*initiator) (struct message *msg);
  int (*responder) (struct message *msg);
};

extern void doi_init (void);
extern struct doi *doi_lookup (u_int8_t);
extern void doi_register (struct doi *);

#endif /* _DOI_H_ */
