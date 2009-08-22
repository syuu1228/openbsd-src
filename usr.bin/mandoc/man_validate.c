/*	$Id: man_validate.c,v 1.5 2009/07/08 00:04:10 schwarze Exp $ */
/*
 * Copyright (c) 2008, 2009 Kristaps Dzonsons <kristaps@kth.se>
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
#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>

#include "libman.h"
#include "libmandoc.h"

#define	POSTARGS  struct man *m, const struct man_node *n

typedef	int	(*v_post)(POSTARGS);

struct	man_valid {
	v_post	 *posts;
};

static	int	  check_eq0(POSTARGS);
static	int	  check_eq1(POSTARGS);
static	int	  check_ge1(POSTARGS);
static	int	  check_ge2(POSTARGS);
static	int	  check_le1(POSTARGS);
static	int	  check_le2(POSTARGS);
static	int	  check_le5(POSTARGS);
static	int	  check_root(POSTARGS);
static	int	  check_sp(POSTARGS);
static	int	  check_text(POSTARGS);

static	v_post	  posts_eq0[] = { check_eq0, NULL };
static	v_post	  posts_ge1[] = { check_ge1, NULL };
static	v_post	  posts_ge2_le5[] = { check_ge2, check_le5, NULL };
static	v_post	  posts_le1[] = { check_le1, NULL };
static	v_post	  posts_le2[] = { check_le2, NULL };
static	v_post	  posts_sp[] = { check_sp, NULL };

static	const struct man_valid man_valids[MAN_MAX] = {
	{ posts_eq0 }, /* br */
	{ posts_ge2_le5 }, /* TH */
	{ posts_ge1 }, /* SH */
	{ posts_ge1 }, /* SS */
	{ NULL }, /* TP */
	{ posts_eq0 }, /* LP */
	{ posts_eq0 }, /* PP */
	{ posts_eq0 }, /* P */
	{ posts_le2 }, /* IP */
	{ posts_le1 }, /* HP */
	{ NULL }, /* SM */
	{ NULL }, /* SB */
	{ NULL }, /* BI */
	{ NULL }, /* IB */
	{ NULL }, /* BR */
	{ NULL }, /* RB */
	{ NULL }, /* R */
	{ NULL }, /* B */
	{ NULL }, /* I */
	{ NULL }, /* IR */
	{ NULL }, /* RI */
	{ posts_eq0 }, /* na */
	{ NULL }, /* i */
	{ posts_sp }, /* sp */
};


int
man_valid_post(struct man *m)
{
	v_post		*cp;

	if (MAN_VALID & m->last->flags)
		return(1);
	m->last->flags |= MAN_VALID;

	switch (m->last->type) {
	case (MAN_TEXT): 
		return(check_text(m, m->last));
	case (MAN_ROOT):
		return(check_root(m, m->last));
	default:
		break;
	}

	if (NULL == (cp = man_valids[m->last->tok].posts))
		return(1);
	for ( ; *cp; cp++)
		if ( ! (*cp)(m, m->last))
			return(0);

	return(1);
}


static int
check_root(POSTARGS) 
{
	
	if (NULL == m->first->child)
		return(man_nerr(m, n, WNODATA));
	if (NULL == m->meta.title)
		return(man_nerr(m, n, WNOTITLE));

	return(1);
}


static int
check_text(POSTARGS) 
{
	const char	*p;
	int		 pos, c;

	assert(n->string);

	for (p = n->string, pos = n->pos + 1; *p; p++, pos++) {
		if ('\\' == *p) {
			c = mandoc_special(p);
			if (c) {
				p += c - 1;
				pos += c - 1;
				continue;
			}
			if ( ! (MAN_IGN_ESCAPE & m->pflags))
				return(man_perr(m, n->line, pos, WESCAPE));
			if ( ! man_pwarn(m, n->line, pos, WESCAPE))
				return(0);
			continue;
		}

		if ('\t' == *p || isprint((u_char)*p)) 
			continue;

		if (MAN_IGN_CHARS & m->pflags)
			return(man_pwarn(m, n->line, pos, WNPRINT));
		return(man_perr(m, n->line, pos, WNPRINT));
	}

	return(1);
}


#define	INEQ_DEFINE(x, ineq, name) \
static int \
check_##name(POSTARGS) \
{ \
	if (n->nchild ineq (x)) \
		return(1); \
	return(man_verr(m, n->line, n->pos, \
			"expected line arguments %s %d, have %d", \
			#ineq, (x), n->nchild)); \
}

INEQ_DEFINE(0, ==, eq0)
INEQ_DEFINE(1, ==, eq1)
INEQ_DEFINE(1, >=, ge1)
INEQ_DEFINE(2, >=, ge2)
INEQ_DEFINE(1, <=, le1)
INEQ_DEFINE(2, <=, le2)
INEQ_DEFINE(5, <=, le5)


static int
check_sp(POSTARGS)
{
	long		 lval;
	char		*ep, *buf;

	if (NULL == m->last->child)
		return(1);
	else if ( ! check_eq1(m, n))
		return(0);

	assert(MAN_TEXT == m->last->child->type);
	buf = m->last->child->string;
	assert(buf);
	
	/* From OpenBSD's strtol(3). */
	errno = 0;
	lval = strtol(buf, &ep, 10);
	if (buf[0] == '\0' || *ep != '\0')
		return(man_nerr(m, m->last->child, WNUMFMT));

	if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) ||
			(lval > INT_MAX || lval < 0))
		return(man_nerr(m, m->last->child, WNUMFMT));

	return(1);
}
