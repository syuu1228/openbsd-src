/*	$OpenBSD: src/sys/msdosfs/denode.h,v 1.3 1996/02/29 10:46:45 niklas Exp $	*/
/*	$NetBSD: denode.h,v 1.20 1996/02/09 19:13:39 christos Exp $	*/

/*-
 * Copyright (C) 1994, 1995 Wolfgang Solfrank.
 * Copyright (C) 1994, 1995 TooLs GmbH.
 * All rights reserved.
 * Original code by Paul Popelka (paulp@uts.amdahl.com) (see below).
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 * 
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 * 
 * This software is provided "as is".
 * 
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 * 
 * October 1992
 */

/*
 * This is the pc filesystem specific portion of the vnode structure.
 *
 * To describe a file uniquely the de_dirclust, de_diroffset, and
 * de_StartCluster fields are used.
 *
 * de_dirclust contains the cluster number of the directory cluster
 *	containing the entry for a file or directory.
 * de_diroffset is the index into the cluster for the entry describing
 *	a file or directory.
 * de_StartCluster is the number of the first cluster of the file or directory.
 *
 * Now to describe the quirks of the pc filesystem.
 * - Clusters 0 and 1 are reserved.
 * - The first allocatable cluster is 2.
 * - The root directory is of fixed size and all blocks that make it up
 *   are contiguous.
 * - Cluster 0 refers to the root directory when it is found in the
 *   startcluster field of a directory entry that points to another directory.
 * - Cluster 0 implies a 0 length file when found in the start cluster field
 *   of a directory entry that points to a file.
 * - You can't use the cluster number 0 to derive the address of the root
 *   directory.
 * - Multiple directory entries can point to a directory. The entry in the
 *   parent directory points to a child directory.  Any directories in the
 *   child directory contain a ".." entry that points back to the parent.
 *   The child directory itself contains a "." entry that points to itself.
 * - The root directory does not contain a "." or ".." entry.
 * - Directory entries for directories are never changed once they are created
 *   (except when removed).  The size stays 0, and the last modification time
 *   is never changed.  This is because so many directory entries can point to
 *   the physical clusters that make up a directory.  It would lead to an
 *   update nightmare.
 * - The length field in a directory entry pointing to a directory contains 0
 *   (always).  The only way to find the end of a directory is to follow the
 *   cluster chain until the "last cluster" marker is found.
 *
 * My extensions to make this house of cards work.  These apply only to the in
 * memory copy of the directory entry.
 * - A reference count for each denode will be kept since dos doesn't keep such
 *   things.
 */

/*
 * Internal pseudo-offset for (nonexistent) directory entry for the root
 * dir in the root dir
 */
#define	MSDOSFSROOT_OFS	0x1fffffff

/*
 * The fat cache structure. fc_fsrcn is the filesystem relative cluster
 * number that corresponds to the file relative cluster number in this
 * structure (fc_frcn).
 */
struct fatcache {
	u_short fc_frcn;	/* file relative cluster number */
	u_short fc_fsrcn;	/* filesystem relative cluster number */
};

/*
 * The fat entry cache as it stands helps make extending files a "quick"
 * operation by avoiding having to scan the fat to discover the last
 * cluster of the file. The cache also helps sequential reads by
 * remembering the last cluster read from the file.  This also prevents us
 * from having to rescan the fat to find the next cluster to read.  This
 * cache is probably pretty worthless if a file is opened by multiple
 * processes.
 */
#define	FC_SIZE		2	/* number of entries in the cache */
#define	FC_LASTMAP	0	/* entry the last call to pcbmap() resolved
				 * to */
#define	FC_LASTFC	1	/* entry for the last cluster in the file */

#define	FCE_EMPTY	0xffff	/* doesn't represent an actual cluster # */

/*
 * Set a slot in the fat cache.
 */
#define	fc_setcache(dep, slot, frcn, fsrcn) \
	(dep)->de_fc[slot].fc_frcn = frcn; \
	(dep)->de_fc[slot].fc_fsrcn = fsrcn;

/*
 * This is the in memory variant of a dos directory entry.  It is usually
 * contained within a vnode.
 */
struct denode {
	struct denode *de_next;	/* Hash chain forward */
	struct denode **de_prev; /* Hash chain back */
	struct vnode *de_vnode;	/* addr of vnode we are part of */
	struct vnode *de_devvp;	/* vnode of blk dev we live on */
	u_long de_flag;		/* flag bits */
	dev_t de_dev;		/* device where direntry lives */
	u_long de_dirclust;	/* cluster of the directory file containing this entry */
	u_long de_diroffset;	/* offset of this entry in the directory cluster */
	u_long de_fndoffset;	/* offset of found dir entry */
	int de_fndcnt;		/* number of slots before de_fndoffset */
	long de_refcnt;		/* reference count */
	struct msdosfsmount *de_pmp;	/* addr of our mount struct */
	struct lockf *de_lockf;	/* byte level lock list */
	pid_t de_lockholder;	/* current lock holder */
	pid_t de_lockwaiter;	/* lock wanter */
	u_char de_Name[12];	/* name, from DOS directory entry */
	u_char de_Attributes;	/* attributes, from directory entry */
	u_short de_CTime;	/* creation time */
	u_short de_CDate;	/* creation date */
	u_short de_ADate;	/* access date */
	u_short de_ATime;	/* access time */
	u_short de_MTime;	/* modification time */
	u_short de_MDate;	/* modification date */
	u_short de_StartCluster; /* starting cluster of file */
	u_long de_FileSize;	/* size of file in bytes */
	struct fatcache de_fc[FC_SIZE];	/* fat cache */
};

/*
 * Values for the de_flag field of the denode.
 */
#define	DE_LOCKED	0x0001	/* Denode lock. */
#define	DE_WANTED	0x0002	/* Denode is wanted by a process. */
#define	DE_UPDATE	0x0004	/* Modification time update request. */
#define	DE_CREATE	0x0008	/* Creation time update */
#define	DE_ACCESS	0x0010	/* Access time update */
#define	DE_MODIFIED	0x0020	/* Denode has been modified. */
#define	DE_RENAME	0x0040	/* Denode is in the process of being renamed */

/*
 * Maximum filename length in Win95
 * Note: Must be < sizeof(dirent.d_name)
 */
#define	WIN_MAXLEN	255

/*
 * Transfer directory entries between internal and external form.
 * dep is a struct denode * (internal form),
 * dp is a struct direntry * (external form).
 */
#define DE_INTERNALIZE(dep, dp)			\
	(bcopy((dp)->deName, (dep)->de_Name, 11),	\
	 (dep)->de_Attributes = (dp)->deAttributes,	\
	 (dep)->de_CTime = getushort((dp)->deCTime),	\
	 (dep)->de_CDate = getushort((dp)->deCDate),	\
	 (dep)->de_ATime = getushort((dp)->deATime),	\
	 (dep)->de_ADate = getushort((dp)->deADate),	\
	 (dep)->de_MTime = getushort((dp)->deMTime),	\
	 (dep)->de_MDate = getushort((dp)->deMDate),	\
	 (dep)->de_StartCluster = getushort((dp)->deStartCluster), \
	 (dep)->de_FileSize = getulong((dp)->deFileSize))

#define DE_EXTERNALIZE(dp, dep)				\
	(bcopy((dep)->de_Name, (dp)->deName, 11),	\
	 (dp)->deAttributes = (dep)->de_Attributes,	\
	 putushort((dp)->deCTime, (dep)->de_CTime),	\
	 putushort((dp)->deCDate, (dep)->de_CDate),	\
	 putushort((dp)->deATime, (dep)->de_ATime),	\
	 putushort((dp)->deADate, (dep)->de_ADate),	\
	 putushort((dp)->deMTime, (dep)->de_MTime),	\
	 putushort((dp)->deMDate, (dep)->de_MDate),	\
	 putushort((dp)->deStartCluster, (dep)->de_StartCluster), \
	 putulong((dp)->deFileSize, \
	     ((dep)->de_Attributes & ATTR_DIRECTORY) ? 0 : (dep)->de_FileSize))

#define	de_forw		de_chain[0]
#define	de_back		de_chain[1]

#ifdef _KERNEL

#define	VTODE(vp)	((struct denode *)(vp)->v_data)
#define	DETOV(de)	((de)->de_vnode)

#define	DE_TIMES(dep) \
	if ((dep)->de_flag & (DE_UPDATE | DE_CREATE | DE_ACCESS)) { \
		if (((dep)->de_Attributes & ATTR_DIRECTORY) == 0) { \
			if ((dep)->de_pmp->pm_flags & MSDOSFSMNT_NOWIN95 \
			    || (dep)->de_flag & DE_UPDATE) { \
				unix2dostime(NULL, &(dep)->de_MDate, &(dep)->de_MTime); \
				(dep)->de_Attributes |= ATTR_ARCHIVE; \
			} \
			if (!((dep)->de_pmp->pm_flags & MSDOSFSMNT_NOWIN95)) { \
				if ((dep)->de_flag & DE_ACCESS) \
					unix2dostime(NULL, &(dep)->de_ADate, &(dep)->de_ATime); \
				if ((dep)->de_flag & DE_CREATE) \
					unix2dostime(NULL, &(dep)->de_CDate, &(dep)->de_CTime); \
			} \
			(dep)->de_flag |= DE_MODIFIED; \
		} \
		(dep)->de_flag &= ~(DE_UPDATE | DE_CREATE | DE_ACCESS); \
	}

/*
 * This overlays the fid structure (see mount.h)
 */
struct defid {
	u_short defid_len;	/* length of structure */
	u_short defid_pad;	/* force long alignment */

	u_long defid_dirclust;	/* cluster this dir entry came from */
	u_long defid_dirofs;	/* offset of entry within the cluster */
#if 0
	u_long	defid_gen;	/* generation number */
#endif
};

/*
 * Prototypes for MSDOSFS vnode operations
 */
int	msdosfs_lookup		__P((void *));
int	msdosfs_create		__P((void *));
int	msdosfs_mknod		__P((void *));
int	msdosfs_open		__P((void *));
int	msdosfs_close		__P((void *));
int	msdosfs_access		__P((void *));
int	msdosfs_getattr		__P((void *));
int	msdosfs_setattr		__P((void *));
int	msdosfs_read		__P((void *));
int	msdosfs_write		__P((void *));
#ifdef NFSSERVER
int	lease_check		__P((void *));
#define	msdosfs_lease_check lease_check
#else
#define	msdosfs_lease_check nullop
#endif
int	msdosfs_ioctl		__P((void *));
int	msdosfs_select		__P((void *));
int	msdosfs_mmap		__P((void *));
int	msdosfs_fsync		__P((void *));
int	msdosfs_seek		__P((void *));
int	msdosfs_remove		__P((void *));
int	msdosfs_link		__P((void *));
int	msdosfs_rename		__P((void *));
int	msdosfs_mkdir		__P((void *));
int	msdosfs_rmdir		__P((void *));
int	msdosfs_symlink		__P((void *));
int	msdosfs_readdir		__P((void *));
int	msdosfs_readlink	__P((void *));
int	msdosfs_abortop		__P((void *));
int	msdosfs_inactive	__P((void *));
int	msdosfs_reclaim		__P((void *));
int	msdosfs_lock		__P((void *));
int	msdosfs_unlock		__P((void *));
int	msdosfs_bmap		__P((void *));
int	msdosfs_strategy	__P((void *));
int	msdosfs_print		__P((void *));
int	msdosfs_islocked	__P((void *));
int	msdosfs_advlock		__P((void *));
int	msdosfs_reallocblks	__P((void *));
int	msdosfs_pathconf	__P((void *));

/*
 * Internal service routine prototypes.
 */
int createde __P((struct denode *, struct denode *, struct denode **, struct componentname *));
int deextend __P((struct denode *, u_long, struct ucred *));
int deget __P((struct msdosfsmount *, u_long, u_long, struct denode **));
int detrunc __P((struct denode *, u_long, int, struct ucred *, struct proc *));
int deupdat __P((struct denode *, int));
int doscheckpath __P((struct denode *, struct denode *));
int dosdirempty __P((struct denode *));
int readde __P((struct denode *, struct buf **, struct direntry **));
int readep __P((struct msdosfsmount *, u_long, u_long, struct buf **, struct direntry **));
void reinsert __P((struct denode *));
int removede __P((struct denode *, struct denode *));
int uniqdosname __P((struct denode *, struct componentname *, u_char *));
int findwin95 __P((struct denode *));
#endif	/* _KERNEL */
