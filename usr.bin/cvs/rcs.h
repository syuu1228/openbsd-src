/*	$OpenBSD: src/usr.bin/cvs/rcs.h,v 1.16 2005/03/13 22:07:49 jfb Exp $	*/
/*
 * Copyright (c) 2004 Jean-Francois Brousseau <jfb@openbsd.org>
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

#ifndef RCS_H
#define RCS_H

#include <sys/types.h>
#include <sys/queue.h>

#include <time.h>

#include "buf.h"

#define RCS_DIFF_MAXARG   32
#define RCS_DIFF_DIV \
	"==================================================================="

#define RCSDIR         "RCS"
#define RCS_FILE_EXT   ",v"

#define RCS_HEAD_INIT  "1.1"


/* lock types */
#define RCS_LOCK_LOOSE    0
#define RCS_LOCK_STRICT   1


/* RCS keyword expansion modes (kflags) */
#define RCS_KWEXP_NONE     0x00
#define RCS_KWEXP_NAME     0x01		/* include keyword name */
#define RCS_KWEXP_VAL      0x02		/* include keyword value */
#define RCS_KWEXP_LKR      0x04		/* include name of locker */
#define RCS_KWEXP_OLD      0x08		/* generate old keyword string */
#define RCS_KWEXP_ERR      0x10		/* mode has an error */

#define RCS_KWEXP_DEFAULT  (RCS_KWEXP_NAME | RCS_KWEXP_VAL)
#define RCS_KWEXP_KVL      (RCS_KWEXP_NAME | RCS_KWEXP_VAL | RCS_KWEXP_LKR)

#define RCS_KWEXP_INVAL(k) \
	((k & RCS_KWEXP_ERR) || \
	((k & RCS_KWEXP_OLD) && (RCS_KWEXP_OLD & ~RCS_KWEXP_OLD)))



#define RCSNUM_MAXNUM  USHRT_MAX
#define RCSNUM_MAXLEN  64


/* file flags */
#define RCS_READ    0x01
#define RCS_WRITE   0x02
#define RCS_RDWR    (RCS_READ|RCS_WRITE)
#define RCS_CREATE  0x04			/* create the file */

/* internal flags */
#define RCS_PARSED  0x010000   /* file has been parsed */
#define RCS_SYNCED  0x020000   /* in-memory copy is in sync with disk copy */
#define RCS_SLOCK   0x040000   /* strict lock */

/* delta flags */
#define RCS_RD_DEAD   0x01     /* dead */

/* RCS error codes */
#define RCS_ERR_NOERR    0
#define RCS_ERR_NOENT    1
#define RCS_ERR_DUPENT   2
#define RCS_ERR_BADNUM   3


typedef struct rcs_num {
	u_int      rn_len;
	u_int16_t *rn_id;
} RCSNUM;


struct rcs_access {
	char    *ra_name;
	uid_t    ra_uid;
	TAILQ_ENTRY(rcs_access) ra_list;
};

struct rcs_sym {
	char    *rs_name;
	RCSNUM  *rs_num;
	TAILQ_ENTRY(rcs_sym) rs_list;
};

struct rcs_lock {
	RCSNUM   *rl_num;

	TAILQ_ENTRY(rcs_lock) rl_list;
};


struct rcs_branch {
	RCSNUM  *rb_num;
	TAILQ_ENTRY(rcs_branch) rb_list;
};

struct rcs_dlist {
	struct rcs_delta *tqh_first;
	struct rcs_delta **tqh_last;
};

struct rcs_delta {
	RCSNUM      *rd_num;
	RCSNUM      *rd_next;
	u_int        rd_flags;
	struct tm    rd_date;
	char        *rd_author;
	char        *rd_state;
	char        *rd_log;
	char        *rd_text;

	struct rcs_dlist rd_snodes;

	TAILQ_HEAD(, rcs_branch) rd_branches;
	TAILQ_ENTRY(rcs_delta)  rd_list;
};


typedef struct rcs_file {
	char   *rf_path;
	u_int   rf_ref;
	mode_t  rf_mode;
	u_int   rf_flags;

	RCSNUM *rf_head;
	RCSNUM *rf_branch;
	char   *rf_comment;
	char   *rf_expand;
	char   *rf_desc;

	u_int   rf_ndelta;
	struct rcs_dlist                  rf_delta;
	TAILQ_HEAD(rcs_alist, rcs_access) rf_access;
	TAILQ_HEAD(rcs_slist, rcs_sym)    rf_symbols;
	TAILQ_HEAD(rcs_llist, rcs_lock)   rf_locks;


	void   *rf_pdata;
} RCSFILE;


extern int rcs_errno;


RCSFILE*      rcs_open          (const char *, int, ...);
void          rcs_close         (RCSFILE *);
const RCSNUM* rcs_branch_get    (RCSFILE *);
int           rcs_branch_set    (RCSFILE *, const RCSNUM *);
int           rcs_access_add    (RCSFILE *, const char *);
int           rcs_access_remove (RCSFILE *, const char *);
int           rcs_access_check  (RCSFILE *, const char *);
int           rcs_sym_add       (RCSFILE *, const char *, RCSNUM *);
int           rcs_sym_remove    (RCSFILE *, const char *);
RCSNUM*       rcs_sym_getrev    (RCSFILE *, const char *);
int           rcs_lock_getmode  (RCSFILE *);
int           rcs_lock_setmode  (RCSFILE *, int);
BUF*          rcs_getrev        (RCSFILE *, RCSNUM *);
BUF*          rcs_gethead       (RCSFILE *);
RCSNUM*       rcs_getrevbydate  (RCSFILE *, struct tm *);
const char*   rcs_desc_get      (RCSFILE *);
int           rcs_desc_set      (RCSFILE *, const char *);
const char*   rcs_comment_get   (RCSFILE *);
int           rcs_comment_set   (RCSFILE *, const char *);
int           rcs_kwexp_set     (RCSFILE *, int);
int           rcs_kwexp_get     (RCSFILE *);
const char*   rcs_errstr        (int);

int       rcs_kflag_get    (const char *);
void      rcs_kflag_usage  (void);
int       rcs_kw_expand    (RCSFILE *, u_char *, size_t, size_t *);

BUF*      rcs_patch     (const char *, const char *);
size_t    rcs_stresc    (int, const char *, char *, size_t *);

RCSNUM*   rcsnum_alloc  (void);
RCSNUM*   rcsnum_parse  (const char *);
void      rcsnum_free   (RCSNUM *);
int       rcsnum_aton   (const char *, char **, RCSNUM *);
char*     rcsnum_tostr  (const RCSNUM *, char *, size_t);
int       rcsnum_cpy    (const RCSNUM *, RCSNUM *, u_int);
int       rcsnum_cmp    (const RCSNUM *, const RCSNUM *, u_int);

/* from cache.c */
int       rcs_cache_init    (u_int);
RCSFILE  *rcs_cache_fetch   (const char *path);
int       rcs_cache_store   (RCSFILE *);
void      rcs_cache_destroy (void);

#endif /* RCS_H */
