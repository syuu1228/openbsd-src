/*	$OpenBSD: src/sys/miscfs/union/Attic/union_vfsops.c,v 1.12 2001/11/21 22:23:14 csapuntz Exp $	*/
/*	$NetBSD: union_vfsops.c,v 1.10 1995/06/18 14:47:47 cgd Exp $	*/

/*
 * Copyright (c) 1994 The Regents of the University of California.
 * Copyright (c) 1994 Jan-Simon Pendry.
 * All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
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
 *	@(#)union_vfsops.c	8.13 (Berkeley) 12/10/94
 */

/*
 * Union Layer
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/filedesc.h>
#include <sys/queue.h>
#include <sys/stat.h>

#include <miscfs/union/union.h>

int union_mount __P((struct mount *, const char *, void *, struct nameidata *,
		     struct proc *));
int union_start __P((struct mount *, int, struct proc *));
int union_unmount __P((struct mount *, int, struct proc *));
int union_root __P((struct mount *, struct vnode **));
int union_statfs __P((struct mount *, struct statfs *, struct proc *));

/*
 * Mount union filesystem
 */
int
union_mount(mp, path, data, ndp, p)
	struct mount *mp;
	const char *path;
	void *data;
	struct nameidata *ndp;
	struct proc *p;
{
	int error = 0;
	struct union_args args;
	struct vnode *lowerrootvp = NULLVP;
	struct vnode *upperrootvp = NULLVP;
	struct union_mount *um = 0;
	struct ucred *cred = 0;
	char *cp;
	int len;
	size_t size;

#ifdef UNION_DIAGNOSTIC
	printf("union_mount(mp = %p)\n", mp);
#endif

	/*
	 * Update is a no-op
	 */
	if (mp->mnt_flag & MNT_UPDATE) {
		/*
		 * Need to provide.
		 * 1. a way to convert between rdonly and rdwr mounts.
		 * 2. support for nfs exports.
		 */
		error = EOPNOTSUPP;
		goto bad;
	}

	/*
	 * Get argument
	 */
	error = copyin(data, &args, sizeof(struct union_args));
	if (error)
		goto bad;

	lowerrootvp = mp->mnt_vnodecovered;
	VREF(lowerrootvp);

	/*
	 * Find upper node.
	 */
	NDINIT(ndp, LOOKUP, FOLLOW|WANTPARENT,
	       UIO_USERSPACE, args.target, p);

	if ((error = namei(ndp)) != 0)
		goto bad;

	upperrootvp = ndp->ni_vp;
	vrele(ndp->ni_dvp);
	ndp->ni_dvp = NULL;

	if (upperrootvp->v_type != VDIR) {
		error = EINVAL;
		goto bad;
	}
	
	um = (struct union_mount *) malloc(sizeof(struct union_mount),
				M_UFSMNT, M_WAITOK);	/* XXX */

	/*
	 * Keep a held reference to the target vnodes.
	 * They are vrele'd in union_unmount.
	 *
	 * Depending on the _BELOW flag, the filesystems are
	 * viewed in a different order.  In effect, this is the
	 * same as providing a mount under option to the mount syscall.
	 */

	um->um_op = args.mntflags & UNMNT_OPMASK;
	switch (um->um_op) {
	case UNMNT_ABOVE:
		um->um_lowervp = lowerrootvp;
		um->um_uppervp = upperrootvp;
		break;

	case UNMNT_BELOW:
		um->um_lowervp = upperrootvp;
		um->um_uppervp = lowerrootvp;
		break;

	case UNMNT_REPLACE:
		vrele(lowerrootvp);
		lowerrootvp = NULLVP;
		um->um_uppervp = upperrootvp;
		um->um_lowervp = lowerrootvp;
		break;

	default:
		error = EINVAL;
		goto bad;
	}

	/*
	 * Unless the mount is readonly, ensure that the top layer
	 * supports whiteout operations
	 */
	if ((mp->mnt_flag & MNT_RDONLY) == 0) {
		error = VOP_WHITEOUT(um->um_uppervp, (struct componentname *) 0, LOOKUP);
		if (error)
			goto bad;
	}

	um->um_cred = p->p_ucred;
	crhold(um->um_cred);
	um->um_cmode = UN_DIRMODE &~ p->p_fd->fd_cmask;

	/*
	 * Depending on what you think the MNT_LOCAL flag might mean,
	 * you may want the && to be || on the conditional below.
	 * At the moment it has been defined that the filesystem is
	 * only local if it is all local, ie the MNT_LOCAL flag implies
	 * that the entire namespace is local.  If you think the MNT_LOCAL
	 * flag implies that some of the files might be stored locally
	 * then you will want to change the conditional.
	 */
	if (um->um_op == UNMNT_ABOVE) {
		if (((um->um_lowervp == NULLVP) ||
		     (um->um_lowervp->v_mount->mnt_flag & MNT_LOCAL)) &&
		    (um->um_uppervp->v_mount->mnt_flag & MNT_LOCAL))
			mp->mnt_flag |= MNT_LOCAL;
	}

	/*
	 * Copy in the upper layer's RDONLY flag.  This is for the benefit
	 * of lookup() which explicitly checks the flag, rather than asking
	 * the filesystem for it's own opinion.  This means, that an update
	 * mount of the underlying filesystem to go from rdonly to rdwr
	 * will leave the unioned view as read-only.
	 */
	mp->mnt_flag |= (um->um_uppervp->v_mount->mnt_flag & MNT_RDONLY);

	mp->mnt_data = (qaddr_t)um;
	vfs_getnewfsid(mp);

	(void) copyinstr(path, mp->mnt_stat.f_mntonname, MNAMELEN - 1, &size);
	bzero(mp->mnt_stat.f_mntonname + size, MNAMELEN - size);

	switch (um->um_op) {
	case UNMNT_ABOVE:
		cp = "<above>:";
		break;
	case UNMNT_BELOW:
		cp = "<below>:";
		break;
	case UNMNT_REPLACE:
		cp = "";
		break;
	default:
		cp = "<invalid>:";
#ifdef DIAGNOSTIC
		panic("union_mount: bad um_op");
#endif
		break;
	}
	len = strlen(cp);
	bcopy(cp, mp->mnt_stat.f_mntfromname, len);

	cp = mp->mnt_stat.f_mntfromname + len;
	len = MNAMELEN - len;

	(void) copyinstr(args.target, cp, len - 1, &size);
	bzero(cp + size, len - size);

#ifdef UNION_DIAGNOSTIC
	printf("union_mount: from %s, on %s\n",
		mp->mnt_stat.f_mntfromname, mp->mnt_stat.f_mntonname);
#endif
	return (0);

bad:
	if (um)
		free(um, M_UFSMNT);
	if (cred)
		crfree(cred);
	if (upperrootvp)
		vrele(upperrootvp);
	if (lowerrootvp)
		vrele(lowerrootvp);
	return (error);
}

/*
 * VFS start.  Nothing needed here - the start routine
 * on the underlying filesystem(s) will have been called
 * when that filesystem was mounted.
 */
 /*ARGSUSED*/
int
union_start(mp, flags, p)
	struct mount *mp;
	int flags;
	struct proc *p;
{

	return (0);
}


int union_unmount_count_vnode(struct vnode *vp, void *args);

int
union_unmount_count_vnode(struct vnode *vp, void *args) {
	int *n = args;

	*n = *n + 1;
	simple_unlock(&vp->v_interlock);
	return (0);
}

/*
 * Free reference to union layer
 */
int
union_unmount(mp, mntflags, p)
	struct mount *mp;
	int mntflags;
	struct proc *p;
{
	struct union_mount *um = MOUNTTOUNIONMOUNT(mp);
	struct vnode *um_rootvp;
	int error;
	int freeing;
	int flags = 0;

#ifdef UNION_DIAGNOSTIC
	printf("union_unmount(mp = %p)\n", mp);
#endif

	if (mntflags & MNT_FORCE) {
		flags |= FORCECLOSE;
	}

	if ((error = union_root(mp, &um_rootvp)) != 0)
		return (error);

	/*
	 * Keep flushing vnodes from the mount list.
	 * This is needed because of the un_pvp held
	 * reference to the parent vnode.
	 * If more vnodes have been freed on a given pass,
	 * the try again.  The loop will iterate at most
	 * (d) times, where (d) is the maximum tree depth
	 * in the filesystem.
	 */
	for (freeing = 0; vflush(mp, um_rootvp, flags) != 0;) {
		int n;

		/* count #vnodes held on mount list */
		vfs_mount_foreach_vnode(mp, union_unmount_count_vnode, &n);

		/* if this is unchanged then stop */
		if (n == freeing)
			break;

		/* otherwise try once more time */
		freeing = n;
	}

	/* At this point the root vnode should have a single reference */
	if (um_rootvp->v_usecount > 1) {
		vput(um_rootvp);
		return (EBUSY);
	}

#ifdef UNION_DIAGNOSTIC
	vprint("union root", um_rootvp);
#endif	 
	/*
	 * Discard references to upper and lower target vnodes.
	 */
	if (um->um_lowervp)
		vrele(um->um_lowervp);
	vrele(um->um_uppervp);
	crfree(um->um_cred);
	/*
	 * Release reference on underlying root vnode
	 */
	vput(um_rootvp);
	/*
	 * And blow it away for future re-use
	 */
	vgone(um_rootvp);
	/*
	 * Finally, throw away the union_mount structure
	 */
	free(mp->mnt_data, M_UFSMNT);	/* XXX */
	mp->mnt_data = 0;
	return (0);
}

int
union_root(mp, vpp)
	struct mount *mp;
	struct vnode **vpp;
{
	struct proc *p = curproc;
	struct union_mount *um = MOUNTTOUNIONMOUNT(mp);
	int error;
	int loselock;

	/*
	 * Return locked reference to root.
	 */
	VREF(um->um_uppervp);
	if ((um->um_op == UNMNT_BELOW) &&
	     VOP_ISLOCKED(um->um_uppervp)) {
		loselock = 1;
	} else {
		vn_lock(um->um_uppervp, LK_EXCLUSIVE | LK_RETRY, p);
		loselock = 0;
	}
	if (um->um_lowervp)
		VREF(um->um_lowervp);
	error = union_allocvp(vpp, mp,
			      (struct vnode *) 0,
			      (struct vnode *) 0,
			      (struct componentname *) 0,
			      um->um_uppervp,
			      um->um_lowervp,
			      1);

	if (error) {
		if (loselock)
			vrele(um->um_uppervp);
		else
			vput(um->um_uppervp);
		if (um->um_lowervp)
			vrele(um->um_lowervp);
	} else {
		if (loselock)
			VTOUNION(*vpp)->un_flags &= ~UN_ULOCK;
	}

	return (error);
}

int
union_statfs(mp, sbp, p)
	struct mount *mp;
	struct statfs *sbp;
	struct proc *p;
{
	int error;
	struct union_mount *um = MOUNTTOUNIONMOUNT(mp);
	struct statfs mstat;
	int lbsize;

#ifdef UNION_DIAGNOSTIC
	printf("union_statfs(mp = %p, lvp = %p, uvp = %p)\n", mp,
			um->um_lowervp,
	       		um->um_uppervp);
#endif

	bzero(&mstat, sizeof(mstat));

	if (um->um_lowervp) {
		error = VFS_STATFS(um->um_lowervp->v_mount, &mstat, p);
		if (error)
			return (error);
	}

	/* now copy across the "interesting" information and fake the rest */
#if 0
	sbp->f_flags = mstat.f_flags;
	sbp->f_bsize = mstat.f_bsize;
	sbp->f_iosize = mstat.f_iosize;
#endif
	lbsize = mstat.f_bsize;
	sbp->f_blocks = mstat.f_blocks;
	sbp->f_bfree = mstat.f_bfree;
	sbp->f_bavail = mstat.f_bavail;
	sbp->f_files = mstat.f_files;
	sbp->f_ffree = mstat.f_ffree;

	error = VFS_STATFS(um->um_uppervp->v_mount, &mstat, p);
	if (error)
		return (error);

	sbp->f_flags = mstat.f_flags;
	sbp->f_bsize = mstat.f_bsize;
	sbp->f_iosize = mstat.f_iosize;

	/*
	 * if the lower and upper blocksizes differ, then frig the
	 * block counts so that the sizes reported by df make some
	 * kind of sense.  none of this makes sense though.
	 */

	if (mstat.f_bsize != lbsize) {
		sbp->f_blocks = sbp->f_blocks * lbsize / mstat.f_bsize;
		sbp->f_bfree = sbp->f_bfree * lbsize / mstat.f_bsize;
		sbp->f_bavail = sbp->f_bavail * lbsize / mstat.f_bsize;
	}
	sbp->f_blocks += mstat.f_blocks;
	sbp->f_bfree += mstat.f_bfree;
	sbp->f_bavail += mstat.f_bavail;
	sbp->f_files += mstat.f_files;
	sbp->f_ffree += mstat.f_ffree;

	if (sbp != &mp->mnt_stat) {
		bcopy(&mp->mnt_stat.f_fsid, &sbp->f_fsid, sizeof(sbp->f_fsid));
		bcopy(mp->mnt_stat.f_mntonname, sbp->f_mntonname, MNAMELEN);
		bcopy(mp->mnt_stat.f_mntfromname, sbp->f_mntfromname, MNAMELEN);
	}
	strncpy(sbp->f_fstypename, mp->mnt_vfc->vfc_name, MFSNAMELEN);
	return (0);
}

#define union_sync ((int (*) __P((struct mount *, int, struct ucred *, \
				  struct proc *)))nullop)

#define union_fhtovp ((int (*) __P((struct mount *, struct fid *, \
	    struct vnode **)))eopnotsupp)
#define union_quotactl ((int (*) __P((struct mount *, int, uid_t, caddr_t, \
	    struct proc *)))eopnotsupp)
#define union_sysctl ((int (*) __P((int *, u_int, void *, size_t *, void *, \
	    size_t, struct proc *)))eopnotsupp)
#define union_vget ((int (*) __P((struct mount *, ino_t, struct vnode **))) \
	    eopnotsupp)
#define union_vptofh ((int (*) __P((struct vnode *, struct fid *)))eopnotsupp)
#define union_checkexp ((int (*) __P((struct mount *, struct mbuf *,	\
	int *, struct ucred **)))eopnotsupp)

struct vfsops union_vfsops = {
	union_mount,
	union_start,
	union_unmount,
	union_root,
	union_quotactl,
	union_statfs,
	union_sync,
	union_vget,
	union_fhtovp,
	union_vptofh,
	union_init,
	union_sysctl,
	union_checkexp
};
