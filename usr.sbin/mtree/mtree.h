/*	$OpenBSD: src/usr.sbin/mtree/mtree.h,v 1.2 1996/12/08 01:13:42 niklas Exp $	*/
/*	$NetBSD: mtree.h,v 1.7 1995/03/07 21:26:27 cgd Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)mtree.h	8.1 (Berkeley) 6/6/93
 */

#include <string.h>
#include <stdlib.h>

#define	KEYDEFAULT \
	(F_GID | F_MODE | F_NLINK | F_SIZE | F_SLINK | F_TIME | F_UID)

#define	MISMATCHEXIT	2

typedef struct _node {
	struct _node	*parent, *child;	/* up, down */
	struct _node	*prev, *next;		/* left, right */
	off_t	st_size;			/* size */
	struct timespec	st_mtimespec;		/* last modification time */
	u_int32_t cksum;				/* check sum */
	char	*slink;				/* symbolic link reference */
	uid_t	st_uid;				/* uid */
	gid_t	st_gid;				/* gid */
#define	MBITS	(S_ISUID|S_ISGID|S_ISTXT|S_IRWXU|S_IRWXG|S_IRWXO)
	mode_t	st_mode;			/* mode */
	nlink_t	st_nlink;			/* link count */

#define	F_CKSUM	0x0001				/* check sum */
#define	F_DONE	0x0002				/* directory done */
#define	F_GID	0x0004				/* gid */
#define	F_GNAME	0x0008				/* group name */
#define	F_IGN	0x0010				/* ignore */
#define	F_MAGIC	0x0020				/* name has magic chars */
#define	F_MODE	0x0040				/* mode */
#define	F_NLINK	0x0080				/* number of links */
#define	F_OPT	0x0100				/* existence optional */
#define	F_SIZE	0x0200				/* size */
#define	F_SLINK	0x0400				/* link count */
#define	F_TIME	0x0800				/* modification time */
#define	F_TYPE	0x1000				/* file type */
#define	F_UID	0x2000				/* uid */
#define	F_UNAME	0x4000				/* user name */
#define	F_VISIT	0x8000				/* file visited */
	u_short	flags;				/* items set */

#define	F_BLOCK	0x001				/* block special */
#define	F_CHAR	0x002				/* char special */
#define	F_DIR	0x004				/* directory */
#define	F_FIFO	0x008				/* fifo */
#define	F_FILE	0x010				/* regular file */
#define	F_LINK	0x020				/* symbolic link */
#define	F_SOCK	0x040				/* socket */
	u_char	type;				/* file type */

	char	name[1];			/* file name (must be last) */
} NODE;

#define	RP(p)	\
	((p)->fts_path[0] == '.' && (p)->fts_path[1] == '/' ? \
	    (p)->fts_path + 2 : (p)->fts_path)
