/*	$OpenBSD: src/usr.bin/cvs/status.c,v 1.41 2005/07/19 15:48:52 xsa Exp $	*/
/*
 * Copyright (c) 2004 Jean-Francois Brousseau <jfb@openbsd.org>
 * Copyright (c) 2005 Xavier Santolaria <xsa@openbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cvs.h"
#include "log.h"
#include "proto.h"


#define CVS_STATUS_SEP \
 "==================================================================="

/* Keep this sorted as it is now. See file.h for status values. */
const char *cvs_statstr[] = {
	"Unknown",
	"Up-to-date",
	"Locally Modified",
	"Locally Added",
	"Locally Removed",
	"Unresolved Conflict",
	"Patched",
	"Needs Checkout",
};


static int cvs_status_init     (struct cvs_cmd *, int, char **, int *);
static int cvs_status_remote   (CVSFILE *, void *);
static int cvs_status_local    (CVSFILE *, void *);
static int cvs_status_pre_exec (struct cvsroot *);

struct cvs_cmd cvs_cmd_status = {
	CVS_OP_STATUS, CVS_REQ_STATUS, "status",
	{ "st", "stat" },
	"Display status information on checked out files",
	"[-lRv]",
	"lRv",
	NULL,
	CF_SORT | CF_IGNORE | CF_RECURSE,
	cvs_status_init,
	cvs_status_pre_exec,
	cvs_status_remote,
	cvs_status_local,
	NULL,
	NULL,
	CVS_CMD_ALLOWSPEC | CVS_CMD_SENDDIR | CVS_CMD_SENDARGS2
};

static int verbose = 0;

static int
cvs_status_init(struct cvs_cmd *cmd, int argc, char **argv, int *arg)
{
	int ch;

	while ((ch = getopt(argc, argv, cmd->cmd_opts)) != -1) {
		switch (ch) {
		case 'l':
			cmd->file_flags &= ~CF_RECURSE;
			break;
		case 'R':
			cmd->file_flags |= CF_RECURSE;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			return (CVS_EX_USAGE);
		}
	}

	*arg = optind;
	return (0);
}

static int
cvs_status_pre_exec(struct cvsroot *root)
{
	if (root->cr_method != CVS_METHOD_LOCAL) {
		if (verbose && (cvs_sendarg(root, "-v", 0) < 0))
			return (CVS_EX_PROTO);
	}

	return (0);
}

/*
 * cvs_status_remote()
 *
 * Get the status of a single file.
 */
static int
cvs_status_remote(CVSFILE *cfp, void *arg)
{
	int ret;
	char *repo, fpath[MAXPATHLEN];
	RCSFILE *rf;
	struct cvsroot *root;

	ret = 0;
	rf = NULL;
	root = CVS_DIR_ROOT(cfp);
	repo = CVS_DIR_REPO(cfp);

	if (cfp->cf_type == DT_DIR) {
		if (cfp->cf_cvstat == CVS_FST_UNKNOWN)
			ret = cvs_sendreq(root, CVS_REQ_QUESTIONABLE,
			    CVS_FILE_NAME(cfp));
		else
			ret = cvs_senddir(root, cfp);

		if (ret == -1)
			ret = CVS_EX_PROTO;

		return (ret);
	}

	cvs_file_getpath(cfp, fpath, sizeof(fpath));

	if (cvs_sendentry(root, cfp) < 0)
		return (CVS_EX_PROTO);

	switch (cfp->cf_cvstat) {
	case CVS_FST_UNKNOWN:
		ret = cvs_sendreq(root, CVS_REQ_QUESTIONABLE, cfp->cf_name);
		break;
	case CVS_FST_UPTODATE:
		ret = cvs_sendreq(root, CVS_REQ_UNCHANGED, cfp->cf_name);
		break;
	case CVS_FST_ADDED:
	case CVS_FST_MODIFIED:
		ret = cvs_sendreq(root, CVS_REQ_MODIFIED, cfp->cf_name);
		if (ret == 0)
			ret = cvs_sendfile(root, fpath);
	default:
		break;
	}

	if (ret == -1)
		ret = CVS_EX_PROTO;

	return (ret);
}

static int
cvs_status_local(CVSFILE *cf, void *arg)
{
	int len;
	size_t n;
	char buf[MAXNAMLEN], fpath[MAXPATHLEN], rcspath[MAXPATHLEN];
	char numbuf[64], timebuf[32];
	RCSFILE *rf;
	struct rcs_sym *sym;

	if (cf->cf_type == DT_DIR) {
		if (verbosity > 1)
			cvs_log(LP_INFO, "Examining %s", cf->cf_name);
		return (0);
	}

	cvs_file_getpath(cf, fpath, sizeof(fpath));

	if (cvs_rcs_getpath(cf, rcspath, sizeof(rcspath)) == NULL)
		return (CVS_EX_DATA);

	if (cf->cf_cvstat != CVS_FST_UNKNOWN &&
	    cf->cf_cvstat != CVS_FST_ADDED) {
		rf = rcs_open(rcspath, RCS_READ);
		if (rf == NULL)
			return (CVS_EX_DATA);
	}

	buf[0] = '\0';
	if (cf->cf_cvstat == CVS_FST_LOST || cf->cf_cvstat == CVS_FST_UNKNOWN)
		strlcpy(buf, "no file ", sizeof(buf));
	strlcat(buf, cf->cf_name, sizeof(buf));

	cvs_printf(CVS_STATUS_SEP "\nFile: %-17s\tStatus: %s\n\n",
	    buf, cvs_statstr[cf->cf_cvstat]);

	if (cf->cf_cvstat == CVS_FST_UNKNOWN) {
		len = snprintf(buf, sizeof(buf), "No entry for %s",
		    cf->cf_name);
	} else if (cf->cf_cvstat == CVS_FST_ADDED) {
		len = snprintf(buf, sizeof(buf), "New file!");
	} else {
		rcsnum_tostr(cf->cf_lrev, numbuf, sizeof(numbuf));
		len = snprintf(buf, sizeof(buf), "%s", numbuf);

		/* Display etime in local mode only. */
		if (cvs_cmdop != CVS_OP_SERVER) {
			strlcat(buf, "\t", sizeof(buf));

			ctime_r(&(cf->cf_etime), timebuf);
			n = strlen(timebuf);
			if ((n > 0) && (timebuf[n - 1] == '\n'))
				timebuf[--n] = '\0';

			strlcat(buf, timebuf, sizeof(buf));
		}
	}

	if (len == -1 || len >= (int)sizeof(buf)) {
		if (rf != NULL)
			rcs_close(rf);
		return (CVS_EX_DATA);
	}

	cvs_printf("   Working revision:\t%s\n", buf);

	if (cf->cf_cvstat == CVS_FST_UNKNOWN ||
	    cf->cf_cvstat == CVS_FST_ADDED) {
		len = snprintf(buf, sizeof(buf), "No revision control file");
	} else {
		len = snprintf(buf, sizeof(buf), "%s\t%s",
		    rcsnum_tostr(rf->rf_head, numbuf, sizeof(numbuf)),
		    rcspath);
	}

	if (len == -1 || len >= (int)sizeof(buf)) {
		if (rf != NULL)
			rcs_close(rf);
		return (CVS_EX_DATA);
	}

	cvs_printf("   Repository revision:\t%s\n", buf);

	/* If the file is unknown, no other output is needed after this. */
	if (cf->cf_cvstat == CVS_FST_UNKNOWN) {
		cvs_printf("\n");
		return (0);
	}

	if (cf->cf_tag != NULL)
		cvs_printf("   Sticky Tag:\t\t%s\n", cf->cf_tag);
	else if (verbosity > 0)
		cvs_printf("   Sticky Tag:\t\t(none)\n");

	/* XXX */
	if (verbosity > 0)
		cvs_printf("   Sticky Date:\t\t%s\n", "(none)");

	if (cf->cf_opts != NULL)
		cvs_printf("   Sticky Options:\t%s\n", cf->cf_opts);
	else if (verbosity > 0)
		cvs_printf("   Sticky Options:\t(none)\n");

	if (verbose) {
		cvs_printf("\n");
		cvs_printf("   Existing Tags:\n");

		if (!TAILQ_EMPTY(&(rf->rf_symbols))) {
			TAILQ_FOREACH(sym, &(rf->rf_symbols), rs_list) {
				rcsnum_tostr(sym->rs_num, numbuf,
				    sizeof(numbuf));

				cvs_printf("\t%-25s\t(%s: %s)\n",
				    sym->rs_name,  
				    RCSNUM_ISBRANCH(sym->rs_num) ? "branch" :
				    "revision", numbuf);
			}
		} else {
			cvs_printf("\tNo Tags Exist\n");
		}
	}

	cvs_printf("\n");
	rcs_close(rf);

	return (0);
}
