/*	$OpenBSD: src/usr.bin/cvs/add.c,v 1.43 2006/05/28 07:56:44 joris Exp $	*/
/*
 * Copyright (c) 2006 Joris Vink <joris@openbsd.org>
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

#include "includes.h"

#include "cvs.h"
#include "diff.h"
#include "log.h"
#include "proto.h"

int	cvs_add(int, char **);
void	cvs_add_local(struct cvs_file *);

char	*logmsg;

struct cvs_cmd cvs_cmd_add = {
	CVS_OP_ADD, CVS_REQ_ADD, "add",
	{ "ad", "new" },
	"Add a new file or directory to the repository",
	"[-m message] ...",
	"m",
	NULL,
	cvs_add
};

int
cvs_add(int argc, char **argv)
{
	int ch;
	int flags;
	char *arg = ".";
	struct cvs_recursion cr;

	flags = CR_RECURSE_DIRS | CR_REPO;

	while ((ch = getopt(argc, argv, cvs_cmd_add.cmd_opts)) != -1) {
		switch (ch) {
		case 'm':
			logmsg = xstrdup(optarg);
			break;
		default:
			fatal("%s", cvs_cmd_add.cmd_synopsis);
		}
	}

	argc -= optind;
	argv += optind;

	cr.enterdir = NULL;
	cr.leavedir = NULL;
	cr.local = cvs_add_local;
	cr.remote = NULL;
	cr.flags = flags;

	if (argc > 0)
		cvs_file_run(argc, argv, &cr);
	else
		cvs_file_run(1, &arg, &cr);

	return (0);
}

void
cvs_add_local(struct cvs_file *cf)
{
	cvs_log(LP_TRACE, "cvs_add_local(%s)", cf->file_path);

	cvs_file_classify(cf, 0);

	switch (cf->file_status) {
	case FILE_ADDED:
		cvs_log(LP_NOTICE, "%s has already been entered",
		    cf->file_path);
		break;
	case FILE_UNKNOWN:
		cvs_log(LP_NOTICE, "%s is unknown to us so far",
		    cf->file_path);
		break;
	default:
		break;
	}
}
