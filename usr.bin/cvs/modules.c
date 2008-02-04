/*	$OpenBSD: src/usr.bin/cvs/modules.c,v 1.7 2008/02/04 19:08:32 joris Exp $	*/
/*
 * Copyright (c) 2008 Joris Vink <joris@openbsd.org>
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

#include <sys/param.h>
#include <sys/dirent.h>
#include <sys/resource.h>

#include <stdlib.h>
#include <string.h>

#include "cvs.h"
#include "config.h"

TAILQ_HEAD(, module_info)	modules;

struct module_checkout *current_module = NULL;
char *module_repo_root = NULL;

void
cvs_parse_modules(void)
{
	cvs_log(LP_TRACE, "cvs_parse_modules()");

	TAILQ_INIT(&modules);
	cvs_read_config(CVS_PATH_MODULES, modules_parse_line);
}

void
modules_parse_line(char *line, int lineno)
{
	int flags;
	struct module_info *mi;
	char *val, *p, *module, *sp, *dp;
	char *dirname, fpath[MAXPATHLEN], *prog;

	flags = 0;
	p = val = line;
	while (*p != ' ' && *p != '\t' && *p != '\0')
		p++;

	if (*p == '\0')
		goto bad;

	*(p++) = '\0';
	module = val;

	while ((*p == ' ' || *p == '\t') && *p != '\0')
		p++;

	if (*p == '\0')
		goto bad;

	val = p;
	while (*p != ' ' && *p != '\t')
		p++;

	prog = NULL;
	while (val[0] == '-') {
		p = val;
		while (*p != ' ' && *p != '\t' && *p != '\0')
			p++;

		if (*p == '\0')
			goto bad;

		*(p++) = '\0';

		switch (val[1]) {
		case 'a':
			if (flags & MODULE_TARGETDIR) {
				cvs_log(LP_NOTICE, "cannot use -a with -d");
				return;
			}
			flags |= MODULE_ALIAS;
			break;
		case 'd':
			if (flags & MODULE_ALIAS) {
				cvs_log(LP_NOTICE, "cannot use -d with -a");
				return;
			}
			flags |= MODULE_TARGETDIR;
			break;
		case 'l':
			flags |= MODULE_NORECURSE;
			break;
		case 'i':
			if (flags != 0 || prog != NULL) {
				cvs_log(LP_NOTICE,
				    "-i cannot be used with other flags");
				return;
			}

			if ((val = strchr(p, ' ' )) == NULL)
				goto bad;

			*(val++) = '\0';
			prog = xstrdup(p);
			p = val;
			flags |= MODULE_RUN_ON_COMMIT;
			break;
		}

		val = p;
	}

	if (*val == '\0')
		goto bad;

	mi = xmalloc(sizeof(*mi));
	mi->mi_name = xstrdup(module);
	mi->mi_flags = flags;
	mi->mi_prog = prog;

	dirname = NULL;
	TAILQ_INIT(&(mi->mi_modules));
	TAILQ_INIT(&(mi->mi_ignores));
	for (sp = val; sp != NULL; sp = dp) {
		dp = strchr(sp, ' ');
		if (dp != NULL)
			*(dp++) = '\0';

		if (mi->mi_flags & MODULE_ALIAS) {
			if (sp[0] == '!') {
				if (strlen(sp) < 2)
					fatal("invalid ! pattern");
				cvs_file_get((sp + 1), 0, &(mi->mi_ignores));
			} else {
				cvs_file_get(sp, 0, &(mi->mi_modules));
			}
		} else if (sp == val) {
			dirname = sp;
		} else {
			if (sp[0] == '!') {
				if (strlen(sp) < 2)
					fatal("invalid ! pattern");

				sp++;
				(void)xsnprintf(fpath, sizeof(fpath), "%s/%s",
				    dirname, sp);
				cvs_file_get(fpath, 0, &(mi->mi_ignores));
			} else {
				(void)xsnprintf(fpath, sizeof(fpath), "%s/%s",
				    dirname, sp);
				cvs_file_get(fpath, 0, &(mi->mi_modules));
			}
		}
	}

	if (!(mi->mi_flags & MODULE_ALIAS) && TAILQ_EMPTY(&(mi->mi_modules)))
		cvs_file_get(dirname, 0, &(mi->mi_modules));

	TAILQ_INSERT_TAIL(&modules, mi, m_list);
	return;

bad:
	cvs_log(LP_NOTICE, "malformed line in CVSROOT/modules: %d", lineno);
}

struct module_checkout *
cvs_module_lookup(char *name)
{
	struct module_checkout *mc;
	struct module_info *mi;

	mc = xmalloc(sizeof(*mc));

	TAILQ_FOREACH(mi, &modules, m_list) {
		if (!strcmp(name, mi->mi_name)) {
			mc = xmalloc(sizeof(*mc));
			mc->mc_modules = mi->mi_modules;
			mc->mc_ignores = mi->mi_ignores;
			mc->mc_canfree = 0;
			mc->mc_name = mi->mi_name;
			mc->mc_flags = mi->mi_flags;
			mc->mc_prog = mi->mi_prog;
			return (mc);
		}
	}

	TAILQ_INIT(&(mc->mc_modules));
	TAILQ_INIT(&(mc->mc_ignores));
	cvs_file_get(name, 0, &(mc->mc_modules));
	mc->mc_canfree = 1;
	mc->mc_name = name;
	mc->mc_flags |= MODULE_ALIAS;
	mc->mc_prog = NULL;

	return (mc);
}
