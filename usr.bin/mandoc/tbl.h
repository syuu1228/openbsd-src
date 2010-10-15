/*	$Id: tbl.h,v 1.2 2010/10/15 21:33:47 schwarze Exp $ */
/*
 * Copyright (c) 2009 Kristaps Dzonsons <kristaps@kth.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef TBL_H
#define TBL_H

__BEGIN_DECLS

struct tbl;

struct tbl	*tbl_alloc(void);
void		 tbl_free(struct tbl *);
void		 tbl_reset(struct tbl *);

int	 	 tbl_read(struct tbl *, const char *, int, const char *, int);
int		 tbl_close(struct termp *, struct tbl *, const char *, int);
int		 tbl_write(struct termp *, const struct tbl *);

__END_DECLS

#endif /*TBL_H*/
