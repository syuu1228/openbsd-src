/*	$OpenBSD: src/usr.bin/aucat/opt.c,v 1.14 2012/06/27 06:53:13 ratchov Exp $	*/
/*
 * Copyright (c) 2008 Alexandre Ratchov <alex@caoua.org>
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
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dev.h"
#include "conf.h"
#include "opt.h"
#ifdef DEBUG
#include "dbg.h"
#endif

struct opt *opt_list = NULL;

struct opt *
opt_new(char *name, struct dev *dev,
    struct aparams *wpar, struct aparams *rpar,
    int maxweight, int mmc, int join, unsigned int mode)
{
	struct opt *o, **po;
	unsigned int len;
	char c;

	for (len = 0; name[len] != '\0'; len++) {
		if (len == OPT_NAMEMAX)
		    errx(1, "%s: name too long", name);
		c = name[len];
		if ((c < 'a' || c > 'z') &&
		    (c < 'A' || c > 'Z'))
			errx(1, "%s: '%c' not allowed", name, c);
	}
	o = malloc(sizeof(struct opt));
	if (o == NULL)
		err(1, "opt_new: malloc");
	if (mode & MODE_RECMASK)
		o->wpar = (mode & MODE_MON) ? *rpar : *wpar;
	if (mode & MODE_PLAY)
		o->rpar = *rpar;
	o->maxweight = maxweight;
	o->mmc = mmc;
	o->join = join;
	o->mode = mode;
	o->dev = dev;
	memcpy(o->name, name, len + 1);
	for (po = &opt_list; *po != NULL; po = &(*po)->next) {
		if (o->dev->num == (*po)->dev->num &&
		    strcmp(o->name, (*po)->name) == 0)
			errx(1, "%s: already defined", o->name);
	}
	o->next = NULL;
	*po = o;
#ifdef DEBUG
	if (debug_level >= 2) {
		dev_dbg(o->dev);
		dbg_puts(".");
		dbg_puts(o->name);
		dbg_puts(":");
		if (o->mode & MODE_REC) {
			dbg_puts(" rec=");
			dbg_putu(o->wpar.cmin);
			dbg_puts(":");
			dbg_putu(o->wpar.cmax);
		}
		if (o->mode & MODE_PLAY) {
			dbg_puts(" play=");
			dbg_putu(o->rpar.cmin);
			dbg_puts(":");
			dbg_putu(o->rpar.cmax);
			dbg_puts(" vol=");
			dbg_putu(o->maxweight);
		}
		if (o->mode & MODE_MON) {
			dbg_puts(" mon=");
			dbg_putu(o->wpar.cmin);
			dbg_puts(":");
			dbg_putu(o->wpar.cmax);
		}
		if (o->mode & (MODE_RECMASK | MODE_PLAY)) {
			if (o->mmc)
				dbg_puts(" mmc");
			if (o->join)
				dbg_puts(" join");
		}
		if (o->mode & MODE_MIDIIN)
			dbg_puts(" midi/in");
		if (o->mode & MODE_MIDIOUT)
			dbg_puts(" midi/out");
		dbg_puts("\n");
	}
#endif
	return o;
}

struct opt *
opt_byname(char *name, unsigned int num)
{
	struct opt *o;

	for (o = opt_list; o != NULL; o = o->next) {
		if (o->dev->num != num)
			continue;
		if (strcmp(name, o->name) == 0)
			return o;
	}
	return NULL;
}

