/*	$OpenBSD: src/usr.sbin/bgpd/name2id.c,v 1.4 2005/07/01 09:19:24 claudio Exp $ */

/*
 * Copyright (c) 2004, 2005 Henning Brauer <henning@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/socket.h>

#include <net/route.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "bgpd.h"

#define	IDVAL_MAX	50000

struct n2id_label {
	TAILQ_ENTRY(n2id_label)	 entry;
	char			*name;
	u_int16_t		 id;
	int			 ref;
};

TAILQ_HEAD(n2id_labels, n2id_label);

u_int16_t	 _name2id(struct n2id_labels *, const char *);
const char	*_id2name(struct n2id_labels *, u_int16_t);
void		 _unref(struct n2id_labels *, u_int16_t);
void		 _ref(struct n2id_labels *, u_int16_t);
int		 _exists(struct n2id_labels *, const char *);

struct n2id_labels	rt_labels = TAILQ_HEAD_INITIALIZER(rt_labels);
struct n2id_labels	pftable_labels = TAILQ_HEAD_INITIALIZER(pftable_labels);

u_int16_t
rtlabel_name2id(char *name)
{
	return (_name2id(&rt_labels, name));
}

const char *
rtlabel_id2name(u_int16_t id)
{
	return (_id2name(&rt_labels, id));
}

void
rtlabel_unref(u_int16_t id)
{
	_unref(&rt_labels, id);
}

void
rtlabel_ref(u_int16_t id)
{
	_ref(&rt_labels, id);
}

u_int16_t
pftable_name2id(char *name)
{
	return (_name2id(&rt_labels, name));
}

const char *
pftable_id2name(u_int16_t id)
{
	return (_id2name(&rt_labels, id));
}

void
pftable_unref(u_int16_t id)
{
	_unref(&rt_labels, id);
}

void
pftable_ref(u_int16_t id)
{
	_ref(&rt_labels, id);
}

u_int16_t
_name2id(struct n2id_labels *head, const char *name)
{
	struct n2id_label	*label, *p = NULL;
	u_int16_t		 new_id = 1;

	if (!name[0]) {
		errno = EINVAL;
		return (0);
	}

	TAILQ_FOREACH(label, head, entry)
		if (strcmp(name, label->name) == 0) {
			label->ref++;
			return (label->id);
		}

	/*
	 * to avoid fragmentation, we do a linear search from the beginning
	 * and take the first free slot we find. if there is none or the list
	 * is empty, append a new entry at the end.
	 */

	if (!TAILQ_EMPTY(head))
		for (p = TAILQ_FIRST(head); p != NULL &&
		    p->id == new_id; p = TAILQ_NEXT(p, entry))
			new_id = p->id + 1;

	if (new_id > IDVAL_MAX) {
		errno = ERANGE;
		return (0);
	}

	if ((label = calloc(1, sizeof(struct n2id_label))) == NULL)
		return (0);
	if ((label->name = strdup(name)) == NULL) {
		free(label);
		return (0);
	}
	label->id = new_id;
	label->ref++;

	if (p != NULL)	/* insert new entry before p */
		TAILQ_INSERT_BEFORE(p, label, entry);
	else		/* either list empty or no free slot in between */
		TAILQ_INSERT_TAIL(head, label, entry);

	return (label->id);
}

const char *
_id2name(struct n2id_labels *head, u_int16_t id)
{
	struct n2id_label	*label;

	if (id == 0)
		return ("");

	TAILQ_FOREACH(label, head, entry)
		if (label->id == id)
			return (label->name);

	return ("");
}

void
_unref(struct n2id_labels *head, u_int16_t id)
{
	struct n2id_label	*p, *next;

	if (id == 0)
		return;

	for (p = TAILQ_FIRST(head); p != NULL; p = next) {
		next = TAILQ_NEXT(p, entry);
		if (id == p->id) {
			if (--p->ref == 0) {
				TAILQ_REMOVE(head, p, entry);
				free(p->name);
				free(p);
			}
			break;
		}
	}
}

void
_ref(struct n2id_labels *head, u_int16_t id)
{
	struct n2id_label	*label;

	if (id == 0)
		return;

	TAILQ_FOREACH(label, head, entry)
		if (label->id == id) {
			++label->ref;
			break;
		}
}

int
_exists(struct n2id_labels *head, const char *name)
{
	struct n2id_label	*label;

	if (!name[0])
		return (0);

	TAILQ_FOREACH(label, head, entry)
		if (strcmp(name, label->name) == 0)
			return (label->id);

	return (0);
}
