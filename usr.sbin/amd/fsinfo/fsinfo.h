/*
 * Copyright (c) 1989 Jan-Simon Pendry
 * Copyright (c) 1989 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry at Imperial College, London.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)fsinfo.h	8.1 (Berkeley) 6/6/93
 *	$Id: fsinfo.h,v 1.5 2002/08/05 01:29:37 pvalchev Exp $
 */

#ifdef __GNUC__
#define INLINE /* __inline */
#else
#define	INLINE
#endif /* __GNUC__ */

/*
 * Pick up target dependent definitions
 */
#include "os-defaults.h"
#include OS_HDR

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>

/*
 * Bogosity to deal with ether { ... }
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netinet/if_ether.h>

#include <string.h>
#include <stdlib.h>
#include <err.h>

#include "fsi_data.h"

extern void fatal();
extern void warning();
extern void error();
extern void analyze_automounts(qelem *);
extern void analyze_hosts(qelem *);
extern void compute_automount_point(char *, size_t, host *, char *);
extern automount *new_automount(char *);
extern auto_tree *new_auto_tree(char *, qelem *);
extern host *new_host(void);
extern disk_fs *new_disk_fs(void);
extern void set_disk_fs(disk_fs *, int, char *);
extern ether_if *new_ether_if(void);
extern mount *new_mount(void);
extern void set_mount(mount *, int, char *);
extern fsmount *new_fsmount(void);
extern void set_fsmount(fsmount *, int, char *);
extern qelem *new_que(void);
extern void init_que(qelem *);
extern void ins_que(qelem *, qelem *);
extern void rem_que(qelem *);
extern dict *new_dict(void);
extern dict_ent *dict_locate(dict *, char *);
extern void dict_add(dict *, char *, char *);
extern int dict_iter(dict *, int (*)());
extern void info_hdr();
extern void gen_hdr();
extern FILE *pref_open();
extern int pref_close();
extern ioloc *current_location();

extern char *disk_fs_strings[];
extern char *mount_strings[];
extern char *fsmount_strings[];
extern char *host_strings[];
extern char *ether_if_strings[];
extern char *autodir;
extern char hostname[];
extern char *username;
extern char **g_argv;
extern char *fstab_pref;
extern char *exportfs_pref;
extern char *mount_pref;
extern char *dumpset_pref;
extern char *bootparams_pref;
extern char idvbuf[];

extern int file_io_errors;
extern int parse_errors;
extern int errors;
extern int verbose;

extern dict *dict_of_hosts;
extern dict *dict_of_volnames;
extern char __progname;

extern char *xcalloc();
extern char *xmalloc();
#define	ALLOC(x)	((struct x *) xcalloc(1, sizeof(struct x)))
#define	STREQ(s,t)	(*(s) == *(t) && strcmp((s)+1,(t)+1) == 0)
#define	ISSET(m,b)	((m) & (1<<(b)))
#define	BITSET(m,b)	((m) |= (1<<(b)))

#define	FIRST(ty, q)	((ty *) ((q)->q_forw))
#define	LAST(ty, q)	((ty *) ((q)->q_back))
#define	NEXT(ty, q)	((ty *) (((qelem *) q)->q_forw))
#define	HEAD(ty, q)	((ty *) q)
#define	ITER(v, ty, q) \
	for ((v) = FIRST(ty,(q)); (v) != HEAD(ty,(q)); (v) = NEXT(ty,(v)))
