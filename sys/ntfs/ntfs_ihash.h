/*	$OpenBSD: src/sys/ntfs/ntfs_ihash.h,v 1.2 2003/05/20 03:23:11 mickey Exp $	*/
/*	$NetBSD: ntfs_ihash.h,v 1.1 2002/12/23 17:38:32 jdolecek Exp $	*/

/*-
 * Copyright (c) 1998, 1999 Semen Ustimenko
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	Id: ntfs_ihash.h,v 1.3 1999/05/12 09:42:59 semenu Exp
 */

extern struct lock ntfs_hashlock;
void ntfs_nthashinit __P((void));
void ntfs_nthashreinit __P((void));
void ntfs_nthashdone __P((void));
struct ntnode   *ntfs_nthashlookup __P((dev_t, ino_t));
struct ntnode   *ntfs_nthashget __P((dev_t, ino_t));
void ntfs_nthashins __P((struct ntnode *));
void ntfs_nthashrem __P((struct ntnode *));
