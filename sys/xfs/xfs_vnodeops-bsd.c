/*
 * Copyright (c) 1995 - 2001 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * XFS operations.
 */

#ifdef __APPLE__
#define MACH_KERNEL 1
#endif

#include <xfs/xfs_locl.h>
#include <xfs/xfs_message.h>
#include <xfs/xfs_common.h>
#include <xfs/xfs_fs.h>
#include <xfs/xfs_dev.h>
#include <xfs/xfs_deb.h>
#include <xfs/xfs_syscalls.h>
#include <xfs/xfs_vnodeops.h>
#ifdef HAVE_VM_VNODE_PAGER_H
#include <vm/vnode_pager.h>
#endif

RCSID("$Id: xfs_vnodeops-bsd.c,v 1.1.1.1 2002/06/05 17:24:11 hin Exp $");

/*
 * vnode functions
 */

#ifdef HAVE_VOP_OPEN
static int
xfs_open(struct vop_open_args * ap)
     /*
  struct vop_open {
          struct vnode *vp;
          int mode;
          struct ucred *cred;
          struct proc *p;
  }; */
{
    return xfs_open_common (ap->a_vp, ap->a_mode, ap->a_cred, ap->a_p);
}
#endif /* HAVE_VOP_OPEN */

#ifdef HAVE_VOP_FSYNC
static int
xfs_fsync(struct vop_fsync_args * ap)
     /*
  vop_fsync {
	struct vnode *vp;
	struct ucred *cred;
	int waitfor;
	struct proc *p;
};  */
{
#ifdef HAVE_STRUCT_VOP_FSYNC_ARGS_A_FLAGS
    return xfs_fsync_common(ap->a_vp, ap->a_cred, ap->a_flags, ap->a_p);
#else
    return xfs_fsync_common(ap->a_vp, ap->a_cred, ap->a_waitfor, ap->a_p);
#endif
}
#endif /* HAVE_VOP_FSYNC */

#ifdef HAVE_VOP_CLOSE 
static int
xfs_close(struct vop_close_args * ap)
     /* vop_close {
	IN struct vnode *vp;
	IN int fflag;
	IN struct ucred *cred;
	IN struct proc *p;
  }; */
{
    return xfs_close_common(ap->a_vp, ap->a_fflag, ap->a_p, ap->a_cred);
}
#endif /* HAVE_VOP_CLOSE */

#ifdef HAVE_VOP_READ
static int
xfs_read(struct vop_read_args * ap)
     /* vop_read {
	IN struct vnode *vp;
	INOUT struct uio *uio;
	IN int ioflag;
	IN struct ucred *cred;
   }; */
{
    return xfs_read_common(ap->a_vp, ap->a_uio, ap->a_ioflag, ap->a_cred);
}
#endif /* HAVE_VOP_READ */

#ifdef HAVE_VOP_WRITE
static int
xfs_write(struct vop_write_args * ap)
     /* vop_write {
	IN struct vnode *vp;
	INOUT struct uio *uio;
	IN int ioflag;
	IN struct ucred *cred;
   }; */
{
    return xfs_write_common(ap->a_vp, ap->a_uio, ap->a_ioflag, ap->a_cred);
}
#endif /* HAVE_VOP_WRITE */

#ifdef HAVE_VOP_IOCTL
static int
xfs_ioctl(struct vop_ioctl_args * ap)
     /* struct vnode *vp,
	  int com,
	  caddr_t data,
	  int flag,
	  struct ucred *cred) */
{
    XFSDEB(XDEBVNOPS, ("xfs_ioctl\n"));

    return EOPNOTSUPP;
}
#endif /* HAVE_VOP_IOCTL */

#ifdef HAVE_VOP_SELECT
static int
xfs_select(struct vop_select_args * ap)
     /* struct vnode *vp,
	   int which,
	   struct ucred *cred ) */
{
    XFSDEB(XDEBVNOPS, ("xfs_select\n"));

    return EOPNOTSUPP;
}
#endif /* HAVE_VOP_SELECT */

#ifdef HAVE_VOP_SEEK
static int
xfs_seek(struct vop_seek_args * ap)
     /*
struct vop_seek_args {
        struct vnodeop_desc *a_desc;
        struct vnode *a_vp;
        off_t a_oldoff;
        off_t a_newoff;
        struct ucred *a_cred;
};
*/
{
    XFSDEB(XDEBVNOPS, ("xfs_seek\n"));
    return 0;
}
#endif /* HAVE_VOP_SEEK */

#ifdef HAVE_VOP_POLL
static int
xfs_poll(struct vop_poll_args * ap)
     /* vop_poll {
	IN struct vnode *vp;
	IN int events;
	IN struct proc *p;
   }; */
{
    XFSDEB(XDEBVNOPS, ("xfs_poll\n"));
    return EOPNOTSUPP;
}
#endif /* HAVE_VOP_POLL */

#ifdef HAVE_VOP_GETATTR
static int
xfs_getattr(struct vop_getattr_args * ap)
     /* struct vnode *vp,
	    struct vattr *vap,
	    struct ucred *cred,
	    struct proc *p) */
{
    return xfs_getattr_common(ap->a_vp, ap->a_vap, ap->a_cred, ap->a_p);
}
#endif /* HAVE_VOP_GETATTR */

#ifdef HAVE_VOP_SETATTR
static int
xfs_setattr(struct vop_setattr_args * ap)
     /* struct vnode *vp,
	    struct vattr *vap,
	    struct ucred *cred,
	    struct proc *p)
	    */
{
    return xfs_setattr_common(ap->a_vp, ap->a_vap, ap->a_cred, ap->a_p);
}
#endif /* HAVE_VOP_SETATTR */

#ifdef HAVE_VOP_ACCESS
static int
xfs_access(struct vop_access_args * ap)
     /*
struct vnode *vp,
	   int mode,
	   struct ucred *cred,
	   struct proc *p)
	   */
{
    return xfs_access_common(ap->a_vp, ap->a_mode, ap->a_cred, ap->a_p);
}
#endif /* HAVE_VOP_ACCESS */

#ifdef HAVE_VOP_LOOKUP
static int
xfs_lookup(struct vop_lookup_args * ap)
     /* struct vop_lookup_args {
	struct vnodeop_desc *a_desc;
	struct vnode *a_dvp;
	struct vnode **a_vpp;
	struct componentname *a_cnp;
}; */
{
    struct componentname *cnp = ap->a_cnp;
    int error;
    int lockparent = (cnp->cn_flags & (LOCKPARENT | ISLASTCN))
	== (LOCKPARENT | ISLASTCN);

    XFSDEB(XDEBVNOPS, ("xfs_lookup: (%s, %ld), nameiop = %lu, flags = %lu\n",
		       cnp->cn_nameptr,
		       cnp->cn_namelen,
		       cnp->cn_nameiop,
		       cnp->cn_flags));

#ifdef PDIRUNLOCK
    cnp->cn_flags &= ~PDIRUNLOCK;
#endif

    error = xfs_lookup_common(ap->a_dvp, cnp, ap->a_vpp);

    if (error == ENOENT
	&& (cnp->cn_nameiop == CREATE || cnp->cn_nameiop == RENAME)
	&& (cnp->cn_flags & ISLASTCN)) {
	error = EJUSTRETURN;
    }

    if (cnp->cn_nameiop != LOOKUP && cnp->cn_flags & ISLASTCN)
	cnp->cn_flags |= SAVENAME;

    if (error == 0 || error == EJUSTRETURN) {
	if (ap->a_dvp == *(ap->a_vpp)) {
	    /* if we looked up ourself, do nothing */
	} else if (!(cnp->cn_flags & ISLASTCN) || !lockparent) {
	    /* if we isn't last component and is isn't requested,
	     * return parent unlocked */
	    xfs_vfs_unlock (ap->a_dvp, xfs_cnp_to_proc(cnp));
#ifdef PDIRUNLOCK
	    cnp->cn_flags |= PDIRUNLOCK;
#endif
	}
    } else {
	/* in case of a error do nothing  */
    } 
    
    XFSDEB(XDEBVNOPS, ("xfs_lookup: error = %d\n", error));

    return error;
}
#endif /* HAVE_VOP_LOOKUP */

#ifdef HAVE_VOP_CACHEDLOOKUP
static int
xfs_cachedlookup(struct vop_cachedlookup_args * ap)
     /* struct vop_cachedlookup_args {
	struct vnodeop_desc *a_desc;
	struct vnode *a_dvp;
	struct vnode **a_vpp;
	struct componentname *a_cnp;
}; */
{
    return xfs_lookup((struct vop_lookup_args *)ap);
}
#endif /* HAVE_VOP_CACHEDLOOKUP */

/*
 * whatever clean-ups are needed for a componentname.
 */

static void
cleanup_cnp (struct componentname *cnp, int error)
{
    if (error != 0 || (cnp->cn_flags & SAVESTART) == 0) {
#ifdef HAVE_KERNEL_ZFREEI
	zfreei(namei_zone, cnp->cn_pnbuf);
	cnp->cn_flags &= ~HASBUF;
#elif defined(FREE_ZONE)
	FREE_ZONE(cnp->cn_pnbuf, cnp->cn_pnlen, M_NAMEI);
#elif defined(PNBUF_PUT)
	PNBUF_PUT(cnp->cn_pnbuf);
#else
	FREE (cnp->cn_pnbuf, M_NAMEI);
#endif
    }
}

#ifdef HAVE_VOP_CREATE
static int
xfs_create(struct vop_create_args *ap)
{
    struct vnode *dvp  = ap->a_dvp;
    struct componentname *cnp = ap->a_cnp;
    const char *name   = cnp->cn_nameptr;
    struct ucred *cred = cnp->cn_cred;
    struct proc *p     = xfs_cnp_to_proc(cnp);
    int error;

    error = xfs_create_common(dvp, name, ap->a_vap, cred, p);

    if (error == 0) {
	error = xfs_lookup_common(dvp, cnp, ap->a_vpp);
    }

    cleanup_cnp (cnp, error);

#if defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
    vput (dvp);
#endif

    XFSDEB(XDEBVNOPS, ("xfs_create: error = %d\n", error));
    
    return error;
}
#endif /* HAVE_VOP_CREATE */

#ifdef HAVE_VOP_REMOVE
static int
xfs_remove(struct vop_remove_args * ap)
     /* struct vnode *dvp,
   struct vnode *vp,
   struct componentname *cnp */
{
    struct componentname *cnp = ap->a_cnp;
    struct vnode *dvp = ap->a_dvp;
    struct vnode *vp  = ap->a_vp;

    int error = xfs_remove_common(dvp, vp, cnp->cn_nameptr, 
				  cnp->cn_cred, xfs_cnp_to_proc(cnp));

    cleanup_cnp (cnp, error);

#if !defined(__FreeBSD__) || __FreeBSD_version < 300000
    if (dvp == vp)
	vrele(vp);
    else
	vput(vp);
    vput(dvp);
#endif
    
#ifdef __APPLE__
    if (error == 0) {
	if (UBCINFOEXISTS(vp)) {
	    ubc_setsize(vp, 0);
	    ubc_release(vp);
	    ubc_uncache(vp);
	}
    }
#endif

    return error;
}
#endif /* HAVE_VOP_REMOVE */

#ifdef HAVE_VOP_RENAME
static int
xfs_rename(struct vop_rename_args * ap)
     /* vop_rename {
	IN WILLRELE struct vnode *fdvp;
	IN WILLRELE struct vnode *fvp;
	IN struct componentname *fcnp;
	IN WILLRELE struct vnode *tdvp;
	IN WILLRELE struct vnode *tvp;
	IN struct componentname *tcnp;
  }; */
{
    struct vnode *tdvp = ap->a_tdvp;
    struct vnode *tvp  = ap->a_tvp;
    struct vnode *fdvp = ap->a_fdvp;
    struct vnode *fvp  = ap->a_fvp;

    int error = xfs_rename_common(fdvp,
				  fvp,
				  ap->a_fcnp->cn_nameptr,
				  tdvp,
				  tvp,
				  ap->a_tcnp->cn_nameptr,
				  ap->a_tcnp->cn_cred,
				  xfs_cnp_to_proc (ap->a_fcnp));
    if(tdvp == tvp)
	vrele(tdvp);
    else
	vput(tdvp);
    if(tvp)
	vput(tvp);
    vrele(fdvp);
    vrele(fvp);
    return error;
}
#endif /* HAVE_VOP_RENAME */

#ifdef HAVE_VOP_MKDIR
static int
xfs_mkdir(struct vop_mkdir_args * ap)
     /* struct vnode *dvp,
	  char *nm,
	  struct vattr *va,
	  struct vnode **vpp,
	  struct ucred *cred)      */
{
    struct vnode *dvp  = ap->a_dvp;
    struct componentname *cnp = ap->a_cnp;
    const char *name   = cnp->cn_nameptr;
    struct ucred *cred = cnp->cn_cred;
    struct proc *p     = xfs_cnp_to_proc(cnp);
    int error;

    error = xfs_mkdir_common(dvp, name, ap->a_vap, cred, p);

    if (error == 0)
	error = xfs_lookup_common(dvp, cnp, ap->a_vpp);

    cleanup_cnp (cnp, error);

#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
    vput(dvp);
#endif

    XFSDEB(XDEBVNOPS, ("xfs_mkdir: error = %d\n", error));

    return error;
}
#endif /* HAVE_VOP_MKDIR */

#ifdef HAVE_VOP_RMDIR
static int
xfs_rmdir(struct vop_rmdir_args * ap)
     /* struct vnode *dvp,
   struct vnode *vp,
   struct componentname *cnp */
{
    struct componentname *cnp = ap->a_cnp;
    struct vnode *dvp = ap->a_dvp;
    struct vnode *vp  = ap->a_vp;
    int error = xfs_rmdir_common(ap->a_dvp, ap->a_vp, 
				 cnp->cn_nameptr,
				 cnp->cn_cred,
				 xfs_cnp_to_proc(cnp));

    cleanup_cnp (cnp, error);
#if !defined(__FreeBSD__) || __FreeBSD_version < 300000
    if (dvp == vp)
	vrele(vp);
    else
	vput(vp);
    vput(dvp);
#endif

    return error;
}
#endif /* HAVE_VOP_RMDIR */

#ifdef HAVE_VOP_READDIR

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
typedef u_long xfs_cookie_t;
#elif defined(__NetBSD__)
typedef off_t xfs_cookie_t;
#else
#error dunno want kind of cookies you have
#endif

static int
xfs_readdir(struct vop_readdir_args * ap)
     /* struct vnode *vp,
	    struct uio *uiop,
	    struct ucred *cred) */
{
    int error;
    off_t off;

    off = ap->a_uio->uio_offset;

    error = xfs_readdir_common(ap->a_vp, ap->a_uio, ap->a_cred,
			       xfs_uio_to_proc (ap->a_uio),
			       ap->a_eofflag);

    if (!error && ap->a_ncookies != NULL) {
	struct uio *uio = ap->a_uio;
	const struct dirent *dp, *dp_start, *dp_end;
	int ncookies;
	xfs_cookie_t *cookies, *cookiep;

	if (uio->uio_segflg != UIO_SYSSPACE || uio->uio_iovcnt != 1)
	    panic("xfs_readdir: mail arla-drinkers and tell them to bake burned cookies");
	dp = (const struct dirent *)
	    ((const char *)uio->uio_iov->iov_base - (uio->uio_offset - off));

	dp_end = (const struct dirent *) uio->uio_iov->iov_base;
	for (dp_start = dp, ncookies = 0;
	     dp < dp_end;
	     dp = (const struct dirent *)((const char *) dp + dp->d_reclen)) {
	    if (dp->d_reclen <= 0)
		break;
	    ncookies++;
	}

	MALLOC(cookies, xfs_cookie_t *, ncookies * sizeof(xfs_cookie_t),
	       M_TEMP, M_WAITOK);
	for (dp = dp_start, cookiep = cookies;
	     dp < dp_end;
	     dp = (const struct dirent *)((const char *) dp + dp->d_reclen)) {
	    if (dp->d_reclen <= 0)
		break;
	    off += dp->d_reclen;
	    *cookiep++ = off;
	}
	*ap->a_cookies = cookies;
	*ap->a_ncookies = ncookies;
    }
    return error;
}
#endif /* HAVE_VOP_READDIR */

#ifdef HAVE_VOP_LINK
static int
xfs_link(struct vop_link_args * ap)
     /*
	WILLRELE struct vnode *tdvp;
	struct vnode *vp;
	struct componentname *cnp;
	*/
{
    struct componentname *cnp = ap->a_cnp;
    struct vnode *vp = ap->a_vp;
    struct vnode *dvp;
    struct proc *p = cnp->cn_proc;
    int error;

#if defined (__OpenBSD__) || defined(__NetBSD__)
    dvp = ap->a_dvp;
#elif defined(__FreeBSD__) || defined(__APPLE__)
    dvp = ap->a_tdvp;
#else
#error what kind of BSD is this?
#endif

    if (vp->v_type == VDIR) {
#ifdef HAVE_VOP_ABORTOP
	    VOP_ABORTOP(dvp, cnp);
#endif
	    error = EPERM;
	    goto out;
    }
    if (dvp->v_mount != vp->v_mount) {
#ifdef HAVE_VOP_ABORTOP
	    VOP_ABORTOP(dvp, cnp);
#endif
	    error = EXDEV;
	    goto out;
    }
    if (dvp != vp && (error = xfs_vfs_writelock(vp, p))) {
#ifdef HAVE_VOP_ABORTOP
	    VOP_ABORTOP(dvp, cnp);
#endif
	    goto out;
    }

    error = xfs_link_common(
			   dvp,
			   vp,
			   cnp->cn_nameptr,
			   cnp->cn_cred,
			   xfs_cnp_to_proc (cnp));

    cleanup_cnp (cnp, error);

    if (dvp != vp)
	xfs_vfs_unlock(vp, p);

out:
#if defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
    vput(dvp);
#endif

    return error;
}
#endif /* HAVE_VOP_LINK */

#ifdef HAVE_VOP_SYMLINK
static int
xfs_symlink(struct vop_symlink_args * ap)
     /*
  IN WILLRELE struct vnode *dvp;
  OUT WILLRELE struct vnode **vpp;
  IN struct componentname *cnp;
  IN struct vattr *vap;
  IN char *target;
  */
{
    struct componentname *cnp = ap->a_cnp;
    struct vnode *dvp  = ap->a_dvp;
    struct vnode **vpp = ap->a_vpp;
    
    int error = xfs_symlink_common(dvp,
				   vpp,
				   cnp,
				   ap->a_vap,
				   ap->a_target);

    if (error == 0) {
	error = xfs_lookup_common(dvp, cnp, vpp);
#if (!defined(__FreeBSD__) || __FreeBSD_version < 400012) && (!defined(__NetBSD__) || __NetBSD_Version__ < 105240000) && (!defined(__OpenBSD__) || OpenBSD < 200107)
	if (error == 0)
	    vput (*vpp);
#endif
    }
    cleanup_cnp (cnp, error);
#if !defined(__FreeBSD__)
    vput(dvp);
#endif
    return error;
}
#endif /* HAVE_VOP_SYMLINK */


#ifdef HAVE_VOP_READLINK
static int
xfs_readlink(struct vop_readlink_args * ap)
     /* struct vnode *vp,
	     struct uio *uiop,
	     struct ucred *cred) */
{
    return xfs_readlink_common(ap->a_vp, ap->a_uio, ap->a_cred);
}
#endif /* HAVE_VOP_READLINK */

#ifdef HAVE_VOP_INACTIVE
static int
xfs_inactive(struct vop_inactive_args * ap)
     /*struct vnode *vp,
	     struct ucred *cred)*/
{
    return xfs_inactive_common(ap->a_vp, xfs_curproc());
}
#endif /* HAVE_VOP_INACTICE */

#ifdef HAVE_VOP_RECLAIM
static int
xfs_reclaim(struct vop_reclaim_args * ap)
     /*struct vop_reclaim_args {
	struct vnodeop_desc *a_desc;
	struct vnode *a_vp;
};*/
{
    struct vnode *vp = ap->a_vp;
    int ret;

    ret = xfs_reclaim_common(vp);
    vp->v_data = NULL;
    return ret;
}
#endif /* HAVE_VOP_RECLAIM */

/*
 * Do lock, unlock, and islocked with lockmgr if we have it.
 */

#if defined(HAVE_KERNEL_LOCKMGR) || defined(HAVE_KERNEL_DEBUGLOCKMGR)

#ifdef HAVE_VOP_LOCK
static int
xfs_lock(struct vop_lock_args * ap)
{               
    struct vnode *vp    = ap->a_vp;
    struct xfs_node *xn = VNODE_TO_XNODE(vp);
    xfs_vnode_lock *l   = &xn->lock;
    int flags           = ap->a_flags;
    int ret;

    XFSDEB(XDEBVNOPS, ("xfs_lock: %lx, flags 0x%x\n",
		       (unsigned long)vp, flags));

    if (l == NULL)
      panic("xfs_lock: lock NULL");

    XFSDEB(XDEBVNOPS, ("xfs_lock before: lk flags: %d share: %d "
		       "wait: %d excl: %d holder: %d\n",
		       l->lk_flags, l->lk_sharecount, l->lk_waitcount,
		       l->lk_exclusivecount, l->lk_lockholder));

#ifndef	DEBUG_LOCKS
#ifdef HAVE_FOUR_ARGUMENT_LOCKMGR
    ret = lockmgr(l, flags, &vp->v_interlock, ap->a_p);
#else
    ret = lockmgr(l, flags, &vp->v_interlock);
#endif
#else
    ret = debuglockmgr(l, flags, &vp->v_interlock, ap->a_p,
			"xfs_lock", ap->a_vp->filename, ap->a_vp->line);
#endif
    XFSDEB(XDEBVNOPS, ("xfs_lock: lk flags: %d share: %d "
		       "wait: %d excl: %d holder: %d\n",
		       l->lk_flags, l->lk_sharecount, l->lk_waitcount,
		       l->lk_exclusivecount, l->lk_lockholder));
    return ret;
}
#endif /* HAVE_VOP_LOCK */

#ifdef HAVE_VOP_UNLOCK
static int
xfs_unlock(struct vop_unlock_args * ap)
{
    struct vnode *vp    = ap->a_vp;
    struct xfs_node *xn = VNODE_TO_XNODE(vp);
    xfs_vnode_lock *l   = &xn->lock;
    int flags           = ap->a_flags;
    int ret;

    if (l == NULL)
      panic("xfs_unlock: lock NULL");

    XFSDEB(XDEBVNOPS,
	   ("xfs_unlock: %lx, flags 0x%x, l %lx, ap %lx\n",
	    (unsigned long)vp, flags,
	    (unsigned long)l,
	    (unsigned long)ap));

    XFSDEB(XDEBVNOPS, ("xfs_unlock: lk flags: %d share: %d "
		       "wait: %d excl: %d holder: %d\n",
		       l->lk_flags, l->lk_sharecount, l->lk_waitcount,
		       l->lk_exclusivecount, l->lk_lockholder));
#ifndef	DEBUG_LOCKS
#ifdef HAVE_FOUR_ARGUMENT_LOCKMGR
    ret = lockmgr (l, flags | LK_RELEASE, &vp->v_interlock, ap->a_p);
#else
    ret = lockmgr (l, flags | LK_RELEASE, &vp->v_interlock);
#endif
#else
    ret = debuglockmgr (l, flags | LK_RELEASE, &vp->v_interlock, ap->a_p,
			"xfs_lock", ap->a_vp->filename, ap->a_vp->line);
#endif
    XFSDEB(XDEBVNOPS, ("xfs_unlock: return %d\n", ret));
    return ret;
}
#endif /* HAVE_VOP_UNLOCK */

#ifdef HAVE_VOP_ISLOCKED
static int
xfs_islocked (struct vop_islocked_args *ap)
{
    struct vnode *vp    = ap->a_vp;
    struct xfs_node *xn = VNODE_TO_XNODE(vp);
    xfs_vnode_lock *l   = &xn->lock;

    XFSDEB(XDEBVNOPS, ("xfs_islocked: %lx\n",
		       (unsigned long)vp));

#if defined(HAVE_TWO_ARGUMENT_LOCKSTATUS)
    return lockstatus (l, ap->a_p);
#elif defined(HAVE_ONE_ARGUMENT_LOCKSTATUS)
    return lockstatus (l);
#else
#error what lockstatus?
#endif
}
#endif /* HAVE_VOP_ISLOCKED */

#else /* !HAVE_KERNEL_LOCKMGR && !HAVE_KERNEL_DEBUGLOCKMGR */

#ifdef HAVE_VOP_LOCK
static int
xfs_lock(struct vop_lock_args * ap)
{
    struct vnode *vp    = ap->a_vp;
    struct xfs_node *xn = VNODE_TO_XNODE(vp);

    XFSDEB(XDEBVNOPS, ("xfs_lock: %lx, %d\n",
		       (unsigned long)vp, xn->vnlocks));

    while (vp->v_flag & VXLOCK) {
	vp->v_flag |= VXWANT;
	(void) tsleep((caddr_t)vp, PINOD, "xfs_vnlock", 0);
    }
    if (vp->v_tag == VT_NON)
	return (ENOENT);
    ++xn->vnlocks;
    return 0;
}
#endif /* HAVE_VOP_LOCK */

#ifdef HAVE_VOP_UNLOCK
static int
xfs_unlock(struct vop_unlock_args * ap)
{
    struct vnode *vp    = ap->a_vp;
    struct xfs_node *xn = VNODE_TO_XNODE(vp);
    XFSDEB(XDEBVNOPS, ("xfs_unlock: %lx, %d\n",
		       (unsigned long)vp, xn->vnlocks));

    --xn->vnlocks;
    if (xn->vnlocks < 0) {
	printf ("PANIC: xfs_unlock: unlocking unlocked\n");
	xn->vnlocks = 0;
    }
    XFSDEB(XDEBVNOPS, ("xfs_unlock: return\n"));

    return 0;
}
#endif /* HAVE_VOP_UNLOCK */

#ifdef HAVE_VOP_ISLOCKED
static int
xfs_islocked (struct vop_islocked_args *ap)
{
    struct vnode *vp    = ap->a_vp;
    struct xfs_node *xn = VNODE_TO_XNODE(vp);

    XFSDEB(XDEBVNOPS, ("xfs_islocked: %lx, %d\n",
		       (unsigned long)vp, xn->vnlocks));

    return xn->vnlocks;
}
#endif /* HAVE_VOP_ISLOCKED */
#endif /* !HAVE_KERNEL_LOCKMGR */

#ifdef HAVE_VOP_ABORTOP
static int
xfs_abortop (struct vop_abortop_args *ap)
     /* struct vnode *dvp;
   struct componentname *cnp; */
{
    struct componentname *cnp = ap->a_cnp;

    if ((cnp->cn_flags & (HASBUF | SAVESTART)) == HASBUF)
#ifdef HAVE_KERNEL_ZFREEI
	zfreei(namei_zone, cnp->cn_pnbuf);
	ap->a_cnp->cn_flags &= ~HASBUF;
#elif defined(FREE_ZONE)
	FREE_ZONE(cnp->cn_pnbuf, cnp->cn_pnlen, M_NAMEI);
#elif defined(PNBUF_PUT)
	PNBUF_PUT(cnp->cn_pnbuf);
#else
	FREE(cnp->cn_pnbuf, M_NAMEI);
#endif
    return 0;
}
#endif /* HAVE_VOP_ABORTOP */

#ifdef HAVE_VOP_MMAP
static int
xfs_mmap(struct vop_mmap_args *ap)
     /*
	IN struct vnode *vp;
	IN int fflags;
	IN struct ucred *cred;
	IN struct proc *p;
	*/
{
    XFSDEB(XDEBVNOPS, ("xfs_mmap\n"));
    return EOPNOTSUPP;
}
#endif /* HAVE_VOP_MMAP */

#ifdef HAVE_VOP_BMAP
static int
xfs_bmap(struct vop_bmap_args *ap)
     /*	IN struct vnode *vp;
	IN daddr_t bn;
	OUT struct vnode **vpp;
	IN daddr_t *bnp;
	OUT int *runp;
	OUT int *runb;
	*/
{
    XFSDEB(XDEBVNOPS, ("xfs_bmap\n"));
    return EOPNOTSUPP;
}
#endif /* HAVE_VOP_BMAP */

#ifdef HAVE_VOP_GETPAGES
static int
xfs_getpages (struct vop_getpages_args *ap)
     /* Old BSD
	IN struct vnode *vp;
	IN vm_page_t *m;
	IN int count;
	IN int reqpage;
	IN vm_ooffset_t offset;
     */
    /* NetBSD UBC
	IN struct vnode *vp;
	IN voff_t offset;
	IN vm_page_t *m;
	IN int *count;
	IN int centeridx;
	IN vm_prot_t access_type;
	IN int advice;
	IN int flags;
    */
{
    int error;

    XFSDEB(XDEBVNOPS, ("xfs_getpages\n"));

#if HAVE_KERNEL_VNODE_PAGER_GENERIC_GETPAGES
    error = vnode_pager_generic_getpages (ap->a_vp, ap->a_m, 
					  ap->a_count, ap->a_reqpage);
#else
    error = xfs_data_valid (ap->a_vp, VNODE_TO_XNODE(ap->a_vp)->cred,
			    xfs_curproc(), XFS_DATA_R);
    if (error == 0)
	error = VOP_GETPAGES(DATA_FROM_VNODE(ap->a_vp), 
			     ap->a_offset, ap->a_m,
			     ap->a_count, ap->a_centeridx, ap->a_access_type,
			     ap->a_advice, ap->a_flags);
#endif
    XFSDEB(XDEBVNOPS, ("xfs_getpages = %d\n", error));
    return error;
}
#endif /* HAVE_VOP_GETPAGES */

#ifdef HAVE_VOP_PUTPAGES
static int
xfs_putpages (struct vop_putpages_args *ap)
     /* Old BSD
        IN struct vnode *vp;
        IN vm_page_t *m;
        IN int count;
        IN int sync;
        IN int *rtvals;
        IN vm_ooffset_t offset;
     */
    /* NetBSD UBC (>= 1.5Y)
	IN struct vnode *vp;
	IN voff_t offlo;
	IN voff_t offhi;
	IN int flags;
    */
{
    struct vnode *vp    = ap->a_vp;
    struct xfs_node *xn = VNODE_TO_XNODE(vp);
    struct vnode *t     = DATA_FROM_XNODE(xn);
    int error;

    XFSDEB(XDEBVNOPS, ("xfs_putpages\n"));

    xn->flags |= XFS_DATA_DIRTY;

#ifdef HAVE_STRUCT_VOP_PUTPAGES_ARGS_A_SYNC /* FreeBSD-style */
    return VOP_PUTPAGES(t, ap->a_m, ap->a_count, ap->a_sync, ap->a_rtvals,
			ap->a_offset);
#else /* NetBSD-style */
#if defined(__NetBSD__) && __NetBSD_Version__  >= 105250000 
    return VOP_PUTPAGES(t, ap->a_offlo, ap->a_offhi, ap->a_flags);
#else
    return VOP_PUTPAGES(t, ap->a_m, ap->a_count, ap->a_flags, ap->a_rtvals);
#endif
#endif /* HAVE_STRUCT_VOP_PUTPAGES_ARGS_A_SYNC */
}
#endif /* HAVE_VOP_PUTPAGES */

#ifdef HAVE_VOP_CMP
static int
xfs_cmp(struct vnode * vp1, struct vnode * vp2)
{
    XFSDEB(XDEBVNOPS, ("xfs_cmp\n"));
    return EOPNOTSUPP;
}
#endif /* HAVE_VOP_CMP */

#ifdef HAVE_VOP_REALVP
static int
xfs_realvp(struct vnode * vp,
	   struct vnode ** vpp)
{
    XFSDEB(XDEBVNOPS, ("xfs_realvp\n"));
    return EOPNOTSUPP;
}
#endif /* HAVE_VOP_REALVP */

#ifdef HAVE_VOP_CNTL
static int
xfs_cntl(struct vnode * vp,
	 int cmd,
	 caddr_t idata,
	 caddr_t odata,
	 int iflag,
	 int oflag)
{
    XFSDEB(XDEBVNOPS, ("xfs_cntl\n"));
    return EOPNOTSUPP;
}
#endif /* HAVE_VOP_CNTL */

#ifdef HAVE_VOP_PRINT
static int
xfs_print (struct vnode *vp)
{
    xfs_printnode_common (vp);
    return 0;
}
#endif

#if 0
#ifdef HAVE_VOP_ADVLOCK
static int
xfs_advlock(void *v)
{
    struct vop_advlock_args /* {
	struct vnode *a_vp;
	caddr_t  a_id;
	int  a_op;
	struct flock *a_fl;
	int  a_flags;
    } */ *ap = v;

    struct xfs_node *xn = VNODE_TO_XNODE(ap->a_vp);
    int ret;
    xfs_locktype_t locktype;

/*     if (XFS_TOKEN_GOT(xn,  */

#if 0
    if (ap->a_fl.l_start != 0 ||
	ap->a_fl.l_end != 0)
	printf ("WARN: someone is trying byte-range locking\n");
    
    switch (ap->a_op) {
    case F_SETLCK:
	locktype = XFS_READLOCK;
	break;

    ret = xfs_advlock_common (xn, );

    return ret;
#else
    return 0;
#endif 			      
}
#endif /* HAVE_VOP_ADVOCK */
#endif

#ifdef HAVE_VOP_REVOKE
static int
xfs_revoke(void *v)
{
#if defined(HAVE_KERNEL_GENFS_REVOKE)
    return genfs_revoke (v);
#elif defined(HAVE_KERNEL_VOP_REVOKE)
    return vop_revoke (v);
#else
    return EOPNOTSUPP;
#endif
}
#endif /* HAVE_VOP_REVOKE */

#ifdef HAVE_VOP_PAGEIN
static int
xfs_pagein(struct vop_pagein_args *ap)
{
#ifdef __APPLE__
    struct uio uio;
    struct iovec iov;
    int ret;

    kernel_upl_map(kernel_map, ap->a_pl, &iov.iov_base);
    iov.iov_base+=ap->a_pl_offset;
    iov.iov_len=ap->a_size;

    uio.uio_iov=&iov;
    uio.uio_iovcnt=1;
    uio.uio_offset=ap->a_f_offset;
    uio.uio_resid=ap->a_size;
    uio.uio_segflg=UIO_SYSSPACE; /* XXX what is it? */
    uio.uio_rw=UIO_READ;
    uio.uio_procp=xfs_curproc();

    ret = VOP_READ(ap->a_vp, &uio, 0, ap->a_cred);

    kernel_upl_unmap(kernel_map, ap->a_pl);

    if (ret) {
	kernel_upl_abort_range(ap->a_pl, ap->a_pl_offset, ap->a_size,
			       UPL_ABORT_ERROR | UPL_ABORT_FREE_ON_EMPTY);
    } else {
	kernel_upl_commit_range(ap->a_pl, ap->a_pl_offset, ap->a_size,
				UPL_COMMIT_CLEAR_DIRTY | UPL_COMMIT_FREE_ON_EMPTY,
				UPL_GET_INTERNAL_PAGE_LIST(ap->a_pl));
    }

    return ret;
#else
    return (VOP_READ(ap->a_vp, ap->a_uio, ap->a_ioflag, ap->a_cred));
#endif
}
  
#endif

#ifdef HAVE_VOP_PAGEOUT
static int
xfs_pageout(struct vop_pageout_args *ap)
{
#ifdef __APPLE__
    struct uio uio;
    struct iovec iov;
    int ret;

    kernel_upl_map(kernel_map, ap->a_pl, &iov.iov_base);
    iov.iov_base+=ap->a_pl_offset;
    iov.iov_len=ap->a_size;

    uio.uio_iov=&iov;
    uio.uio_iovcnt=1;
    uio.uio_offset=ap->a_f_offset;
    uio.uio_resid=ap->a_size;
    uio.uio_segflg=UIO_SYSSPACE; /* XXX what is it? */
    uio.uio_rw=UIO_WRITE;
    uio.uio_procp=xfs_curproc();

    ret = VOP_WRITE(ap->a_vp, &uio, 0, ap->a_cred);

    kernel_upl_unmap(kernel_map, ap->a_pl);

    if (ret) {
	kernel_upl_abort_range(ap->a_pl, ap->a_pl_offset, ap->a_size,
			       UPL_ABORT_FREE_ON_EMPTY);
    } else {
	kernel_upl_commit_range(ap->a_pl, ap->a_pl_offset, ap->a_size,
				UPL_COMMIT_CLEAR_DIRTY | UPL_COMMIT_FREE_ON_EMPTY,
				UPL_GET_INTERNAL_PAGE_LIST(ap->a_pl));
    }

    return ret;
#else
    return (VOP_WRITE(ap->a_vp, ap->a_uio, ap->a_ioflag, ap->a_cred));
#endif
}
#endif

#ifdef HAVE_VOP_CREATEVOBJECT
static int
xfs_createvobject(struct vop_createvobject_args *ap)
/*
struct vop_createvobject_args {
	struct vnode *vp;
	struct ucred *cred;
	struct proc *p;
};
 */
{
    XFSDEB(XDEBVNOPS, ("xfs_createvobject\n"));

    return vop_stdcreatevobject (ap);
}
#endif /* HAVE_VOP_CREATEVOBJECT */

#ifdef HAVE_VOP_DESTROYVOBJECT
static int
xfs_destroyvobject(struct vop_destroyvobject_args *ap)
/*
struct vop_destroyvobject_args {
	struct vnode *vp;
};
 */
{
    XFSDEB(XDEBVNOPS, ("xfs_destroyvobject\n"));

    return vop_stddestroyvobject (ap);
}
#endif /* HAVE_VOP_DESTROYVOBJECT */

#ifdef HAVE_VOP_GETVOBJECT
static int
xfs_getvobject(struct vop_getvobject_args *ap)
/*
struct vop_getvobject_args {
	struct vnode *vp;
	struct vm_object **objpp;
};
 */
{
    XFSDEB(XDEBVNOPS, ("xfs_getvobject\n"));

    return vop_stdgetvobject (ap);
}
#endif /* HAVE_VOP_GETVOBJECT */

vop_t **xfs_vnodeop_p;

int
xfs_eopnotsupp (struct vop_generic_args *ap)
{
    XFSDEB(XDEBVNOPS, ("xfs_eopnotsupp %s\n", ap->a_desc->vdesc_name));
    return EOPNOTSUPP;
}

int
xfs_returnzero (struct vop_generic_args *ap)
{
    XFSDEB(XDEBVNOPS, ("xfs_returnzero %s\n", ap->a_desc->vdesc_name));
    return 0;
}

static struct vnodeopv_entry_desc xfs_vnodeop_entries[] = {
    {&vop_default_desc, (vop_t *) xfs_eopnotsupp},
#ifdef HAVE_VOP_LOOKUP
#ifdef HAVE_KERNEL_VFS_CACHE_LOOKUP
    {&vop_lookup_desc, (vop_t *) vfs_cache_lookup },
#else
    {&vop_lookup_desc, (vop_t *) xfs_lookup },
#endif
#endif
#ifdef HAVE_VOP_CACHEDLOOKUP
    {&vop_cachedlookup_desc, (vop_t *) xfs_cachedlookup },
#endif
#ifdef HAVE_VOP_OPEN
    {&vop_open_desc, (vop_t *) xfs_open },
#endif
#ifdef HAVE_VOP_FSYNC
    {&vop_fsync_desc, (vop_t *) xfs_fsync },
#endif
#ifdef HAVE_VOP_CLOSE
    {&vop_close_desc, (vop_t *) xfs_close },
#endif
#ifdef HAVE_VOP_READ
    {&vop_read_desc, (vop_t *) xfs_read },
#endif
#ifdef HAVE_VOP_WRITE
    {&vop_write_desc, (vop_t *) xfs_write },
#endif
#ifdef HAVE_VOP_MMAP
    {&vop_mmap_desc, (vop_t *) xfs_mmap },
#endif
#ifdef HAVE_VOP_BMAP
    {&vop_bmap_desc, (vop_t *) xfs_bmap },
#endif
#ifdef HAVE_VOP_IOCTL
    {&vop_ioctl_desc, (vop_t *) xfs_ioctl },
#endif
#ifdef HAVE_VOP_SELECT
    {&vop_select_desc, (vop_t *) xfs_select },
#endif
#ifdef HAVE_VOP_SEEK
    {&vop_seek_desc, (vop_t *) xfs_seek },
#endif
#ifdef HAVE_VOP_POLL
    {&vop_poll_desc, (vop_t *) xfs_poll },
#endif
#ifdef HAVE_VOP_GETATTR
    {&vop_getattr_desc, (vop_t *) xfs_getattr },
#endif
#ifdef HAVE_VOP_SETATTR
    {&vop_setattr_desc, (vop_t *) xfs_setattr },
#endif
#ifdef HAVE_VOP_ACCESS
    {&vop_access_desc, (vop_t *) xfs_access },
#endif
#ifdef HAVE_VOP_CREATE
    {&vop_create_desc, (vop_t *) xfs_create },
#endif
#ifdef HAVE_VOP_REMOVE
    {&vop_remove_desc, (vop_t *) xfs_remove },
#endif
#ifdef HAVE_VOP_LINK
    {&vop_link_desc, (vop_t *) xfs_link },
#endif
#ifdef HAVE_VOP_RENAME
    {&vop_rename_desc, (vop_t *) xfs_rename },
#endif
#ifdef HAVE_VOP_MKDIR
    {&vop_mkdir_desc, (vop_t *) xfs_mkdir },
#endif
#ifdef HAVE_VOP_RMDIR
    {&vop_rmdir_desc, (vop_t *) xfs_rmdir },
#endif
#ifdef HAVE_VOP_READDIR
    {&vop_readdir_desc, (vop_t *) xfs_readdir },
#endif
#ifdef HAVE_VOP_SYMLINK
    {&vop_symlink_desc, (vop_t *) xfs_symlink },
#endif
#ifdef HAVE_VOP_READLINK
    {&vop_readlink_desc, (vop_t *) xfs_readlink },
#endif
#ifdef HAVE_VOP_INACTIVE
    {&vop_inactive_desc, (vop_t *) xfs_inactive },
#endif
#ifdef HAVE_VOP_RECLAIM
    {&vop_reclaim_desc, (vop_t *) xfs_reclaim },
#endif
#ifdef HAVE_VOP_LOCK
    {&vop_lock_desc, (vop_t *) xfs_lock },
#endif
#ifdef HAVE_VOP_UNLOCK
    {&vop_unlock_desc, (vop_t *) xfs_unlock },
#endif
#ifdef HAVE_VOP_ISLOCKED
    {&vop_islocked_desc, (vop_t *) xfs_islocked },
#endif
#ifdef HAVE_VOP_ABORTOP
    {&vop_abortop_desc, (vop_t *) xfs_abortop },
#endif
#ifdef HAVE_VOP_GETPAGES
    {&vop_getpages_desc, (vop_t *) xfs_getpages },
#endif
#ifdef HAVE_VOP_PUTPAGES
    {&vop_putpages_desc, (vop_t *) xfs_putpages },
#endif
#ifdef HAVE_VOP_REVOKE
    {&vop_revoke_desc, (vop_t *) xfs_revoke },
#endif
#ifdef HAVE_VOP_PRINT
    {&vop_print_desc, (vop_t *) xfs_print}, 
#endif
#if 0
#ifdef HAVE_VOP_ADVLOCK
    {&vop_advlock_desc, (vop_t *) xfs_advlock },
#endif
#endif
#ifdef HAVE_VOP_PAGEIN
    {&vop_pagein_desc, (vop_t *) xfs_pagein },
#endif
#ifdef HAVE_VOP_PAGEOUT
    {&vop_pageout_desc, (vop_t *) xfs_pageout },
#endif
#ifdef HAVE_VOP_CREATEVOBJECT
    {&vop_createvobject_desc, (vop_t *) xfs_createvobject },
#endif
#ifdef HAVE_VOP_DESTROYVOBJECT
    {&vop_destroyvobject_desc, (vop_t *) xfs_destroyvobject },
#endif
#ifdef HAVE_VOP_GETVOBJECT
    {&vop_getvobject_desc, (vop_t *) xfs_getvobject },
#endif
    {(struct vnodeop_desc *) NULL, (int (*) (void *)) NULL}
};

struct vnodeopv_desc xfs_vnodeop_opv_desc =
{&xfs_vnodeop_p, xfs_vnodeop_entries};

#ifdef VNODEOP_SET
VNODEOP_SET(xfs_vnodeop_opv_desc);
#endif
