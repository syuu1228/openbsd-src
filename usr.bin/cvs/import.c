/*	$OpenBSD: src/usr.bin/cvs/import.c,v 1.18 2005/05/25 21:47:19 jfb Exp $	*/
/*
 * Copyright (c) 2004 Joris Vink <joris@openbsd.org>
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "log.h"
#include "file.h"
#include "cvs.h"
#include "proto.h"


#define CVS_IMPORT_DEFBRANCH    "1.1.1"

static int cvs_import_init     (struct cvs_cmd *, int, char **, int *);
static int cvs_import_pre_exec (struct cvsroot *);
static int cvs_import_remote   (CVSFILE *, void *);
static int cvs_import_local    (CVSFILE *, void *);

static int dflag = 0;
static RCSNUM *bnum;
static char *branch, *module, *vendor, *release;

struct cvs_cmd cvs_cmd_import = {
	CVS_OP_IMPORT, CVS_REQ_IMPORT, "import",
	{ "im", "imp" },
	"Import sources into CVS, using vendor branches",
	"[-d] [-b branch] [-I ign] [-k subst] [-m msg] repository "
	"vendor-tag release-tags ...",
	"b:dI:k:m:",
	NULL,
	CF_RECURSE | CF_IGNORE | CF_NOSYMS,
	cvs_import_init,
	cvs_import_pre_exec,
	cvs_import_remote,
	cvs_import_local,
	NULL,
	NULL,
	CVS_CMD_SENDDIR
};

static int
cvs_import_init(struct cvs_cmd *cmd, int argc, char **argv, int *arg)
{
	int ch;

	branch = CVS_IMPORT_DEFBRANCH;

	while ((ch = getopt(argc, argv, cmd->cmd_opts)) != -1) {
		switch (ch) {
		case 'b':
			branch = optarg;
			if ((bnum = rcsnum_parse(branch)) == NULL) {
				cvs_log(LP_ERR, "%s is not a numeric branch",
				    branch);
				return (CVS_EX_USAGE);
			}
			rcsnum_free(bnum);
			break;
		case 'd':
			dflag = 1;
			break;
		case 'I':
			if (cvs_file_ignore(optarg) < 0) {
				cvs_log(LP_ERR, "failed to add `%s' to list "
				    "of ignore patterns", optarg);
				return (CVS_EX_USAGE);
			}
			break;
		case 'k':
			break;
		case 'm':
			cvs_msg = strdup(optarg);
			if (cvs_msg == NULL) {
				cvs_log(LP_ERRNO, "failed to copy message");
				return (CVS_EX_DATA);
			}
			break;
		default:
			return (CVS_EX_USAGE);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 3)
		return (CVS_EX_USAGE);

	module = argv[0];
	vendor = argv[1];
	release = argv[2];

	*arg = optind + 3;

	if ((cvs_msg == NULL) &&
	    (cvs_msg = cvs_logmsg_get(NULL, NULL, NULL, NULL)) == NULL)
		return (CVS_EX_DATA);

	return (0);
}

static int
cvs_import_pre_exec(struct cvsroot *root)
{
	char repodir[MAXPATHLEN];

	if (root->cr_method == CVS_METHOD_LOCAL) {
		snprintf(repodir, sizeof(repodir), "%s/%s", root->cr_dir,
		    module);
		if (mkdir(repodir, 0700) == -1) {
			cvs_log(LP_ERRNO, "failed to create %s", repodir);
			return (CVS_EX_DATA);
		}
	} else {
		if ((cvs_sendarg(root, "-b", 0) < 0) ||
		    (cvs_sendarg(root, branch, 0) < 0) ||
		    (cvs_logmsg_send(root, cvs_msg) < 0) ||
		    (cvs_sendarg(root, module, 0) < 0) ||
		    (cvs_sendarg(root, vendor, 0) < 0) ||
		    (cvs_sendarg(root, release, 0) < 0))
			return (CVS_EX_PROTO);
	}

	return (0);
}

/*
 * cvs_import_remote()
 *
 * Perform the import of a single file or directory.
 */
static int
cvs_import_remote(CVSFILE *cf, void *arg)
{
	int len;
	struct cvsroot *root;
	char fpath[MAXPATHLEN], repodir[MAXPATHLEN];
	char repo[MAXPATHLEN];

	root = CVS_DIR_ROOT(cf);
	len = snprintf(repo, sizeof(repo), "%s/%s", root->cr_dir, module);
	if (len == -1 || len >= (int)sizeof(repo)) {
		errno = ENAMETOOLONG;
		cvs_log(LP_ERRNO, "%s", repo);
		return (CVS_EX_DATA);
	}

	cvs_file_getpath(cf, fpath, sizeof(fpath));

	if (cf->cf_type == DT_DIR) {
		if (!strcmp(cf->cf_name, "."))
			strlcpy(repodir, repo, sizeof(repodir));
		else {
			len = snprintf(repodir, sizeof(repodir), "%s/%s",
			    repo, fpath);
			if (len == -1 || len >= (int)sizeof(repodir)) {
				errno = ENAMETOOLONG;
				cvs_log(LP_ERRNO, "%s", repodir);
				return (CVS_EX_DATA);
			}
		}

		if (cvs_sendreq(root, CVS_REQ_DIRECTORY, fpath) < 0)
			return (CVS_EX_PROTO);
		if (cvs_sendln(root, repodir) < 0)
			return (CVS_EX_PROTO);
		return (0);
	}

	if (cvs_sendreq(root, CVS_REQ_MODIFIED, cf->cf_name) < 0)
		return (CVS_EX_PROTO);
	if (cvs_sendfile(root, fpath) < 0)
		return (CVS_EX_PROTO);

	return (0);
}

static int
cvs_import_local(CVSFILE *cf, void *arg)
{
	int len;
	time_t stamp;
	char fpath[MAXPATHLEN], rpath[MAXPATHLEN], repo[MAXPATHLEN];
	const char *comment;
	struct stat fst;
	struct timeval ts[2];
	struct cvsroot *root;
	RCSFILE *rf;
	RCSNUM *rev;

	root = CVS_DIR_ROOT(cf);
	len = snprintf(repo, sizeof(repo), "%s/%s", root->cr_dir, module);
	if (len == -1 || len >= (int)sizeof(repo)) {
		errno = ENAMETOOLONG;
		cvs_log(LP_ERRNO, "%s", repo);
		return (CVS_EX_DATA);
	}

	cvs_file_getpath(cf, fpath, sizeof(fpath));

	if (cf->cf_type == DT_DIR) {
		if (!strcmp(cf->cf_name, "."))
			strlcpy(rpath, repo, sizeof(rpath));
		else {
			len = snprintf(rpath, sizeof(rpath), "%s/%s",
			    repo, fpath);
			if (len == -1 || len >= (int)sizeof(rpath)) {
				errno = ENAMETOOLONG;
				cvs_log(LP_ERRNO, "%s", rpath);
				return (CVS_EX_DATA);
			}

			cvs_printf("Importing %s\n", rpath);
			if (mkdir(rpath, 0700) == -1) {
				cvs_log(LP_ERRNO, "failed to create %s",
				    rpath);
			}
		}

		return (0);
	}

	/*
	 * If -d was given, use the file's last modification time as the
	 * timestamps for the initial revisions.
	 */
	if (dflag) {
		if (stat(fpath, &fst) == -1) {
			cvs_log(LP_ERRNO, "failed to stat %s", fpath);
			return (CVS_EX_DATA);
		}
		stamp = (time_t)fst.st_mtime;

		ts[0].tv_sec = stamp;
		ts[0].tv_usec = 0;
		ts[1].tv_sec = stamp;
		ts[1].tv_usec = 0;
	} else
		stamp = -1;

	snprintf(rpath, sizeof(rpath), "%s/%s%s",
	    repo, fpath, RCS_FILE_EXT);

	cvs_printf("N %s\n", fpath);

	rf = rcs_open(rpath, RCS_RDWR|RCS_CREATE);
	if (rf == NULL) {
		cvs_log(LP_ERR, "failed to create RCS file: %s",
		    strerror(rcs_errno));
		return (CVS_EX_DATA);
	}

	comment = rcs_comment_lookup(cf->cf_name);
	if ((comment != NULL) && (rcs_comment_set(rf, comment) < 0)) {
		cvs_log(LP_WARN, "failed to set RCS comment leader: %s",
		    rcs_errstr(rcs_errno));
		/* don't error out, no big deal */
	}

	/* first add the magic 1.1.1.1 revision */
	rev = rcsnum_parse("1.1.1.1");
	if (rcs_rev_add(rf, rev, cvs_msg, stamp) < 0) {
		cvs_log(LP_ERR, "failed to add revision: %s",
		    rcs_errstr(rcs_errno));
		rcs_close(rf);
		(void)unlink(rpath);
		return (CVS_EX_DATA);
	}

	if (rcs_sym_add(rf, release, rev) < 0) {
		cvs_log(LP_ERR, "failed to set RCS symbol: %s",
		    strerror(rcs_errno));
		rcs_close(rf);
		(void)unlink(rpath);
		return (CVS_EX_DATA);
	}

	rcsnum_free(rev);

	rev = rcsnum_parse(RCS_HEAD_INIT);
	if (rcs_rev_add(rf, rev, cvs_msg, stamp) < 0) {
		cvs_log(LP_ERR, "failed to add revision: %s",
		    rcs_errstr(rcs_errno));
		rcs_close(rf);
		(void)unlink(rpath);
		return (CVS_EX_DATA);
	}

	if (rcs_head_set(rf, rev) < 0) {
		cvs_log(LP_ERR, "failed to set RCS head: %s",
		    rcs_errstr(rcs_errno));
		rcs_close(rf);
		(void)unlink(rpath);
		return (CVS_EX_DATA);
	}

#if 0
	if (rcs_branch_set(rf, rev) < 0) {
		cvs_log(LP_ERR, "failed to set RCS default branch: %s",
		    strerror(rcs_errno));
		return (CVS_EX_DATA);
	}
#endif

	/* add the vendor tag and release tag as symbols */
	rcs_close(rf);

	if (dflag && (utimes(rpath, ts) == -1))
		cvs_log(LP_ERRNO, "failed to timestamp RCS file");

	return (CVS_EX_OK);
}
