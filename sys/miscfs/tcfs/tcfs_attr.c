#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <miscfs/tcfs/tcfs.h>
#include "tcfs_rw.h"

int
tcfs_getattr(v)
        void *v;
{
        struct vop_getattr_args *ap = v;
        int error;
	tcfs_fileinfo i;
        if ((error = tcfs_bypass(ap)) != 0)
                return (error);
        /* Requires that arguments be restored. */

        ap->a_vap->va_fsid = ap->a_vp->v_mount->mnt_stat.f_fsid.val[0];
	i.flag=ap->a_vap->va_flags; 
	ap->a_vap->va_size-=FI_SPURE(&i);

        return (0);
}


int           
tcfs_setattr(v)
void *v;
{
	struct vop_setattr_args *a = v;
	struct vattr *ap;
	u_quad_t size=0;
	tcfs_fileinfo i,n;
	int error,sp=0;
	
			
	i=tcfs_xgetflags(a->a_vp,a->a_p,a->a_cred);
	ap=a->a_vap;

        if(FI_GSHAR(&i))
                {
                 if(!tcfs_getgkey(a->a_cred,a->a_p,a->a_vp))
			{
                        return EACCES;
			}
                }
        else
                {
                 if (!tcfs_getpkey(a->a_cred,a->a_p,a->a_vp))
                 	if (!tcfs_getukey(a->a_cred,a->a_p,a->a_vp))
			{
                         	return EACCES;
			}
                }

	if ((ap->va_flags)!=VNOVAL)
	{


		n.flag=ap->va_flags;
		n.end_of_file=i.end_of_file;

		if((FI_CFLAG(&n)&&FI_GSHAR(&i))||(FI_GSHAR(&n)&&FI_CFLAG(&i)))
			{ 
			return EACCES;
			}

		if(FI_SPURE(&n)!=FI_SPURE(&i))
			{ 
			/* le spure no (le settano solo write e trunc) */
			return EACCES;
			}
	
		if(FI_CFLAG(&n)&&(!FI_CFLAG(&i)))
		{
			sp=tcfs_ed(a->a_vp, a->a_p, a->a_cred, &n);
			FI_SET_SP(&n,sp);
		}

		if((!FI_CFLAG(&n))&&FI_CFLAG(&i))
		{
			sp=tcfs_ed(a->a_vp, a->a_p, a->a_cred, &n);
			FI_SET_SP(&n,0);
		}

		if(FI_GSHAR(&n)&&(!FI_GSHAR(&i)))
		{
			sp=tcfs_ed(a->a_vp, a->a_p, a->a_cred, &n);
			FI_SET_SP(&n,sp);
		}

		if((!FI_GSHAR(&n))&&FI_GSHAR(&i))
		{
			sp=tcfs_ed(a->a_vp, a->a_p, a->a_cred, &n);
			FI_SET_SP(&n,0);
		}


		ap->va_flags=i.flag=n.flag;
		if(a->a_vp->v_type==VREG)
		{
			ap->va_size=FI_ENDOF(&i)+sp;
			error=tcfs_xsetflags(a->a_vp, a->a_p,a->a_cred,&i);
		}
		return tcfs_bypass((void*)v);
	}
	if ((ap->va_size)!=VNOVAL)
	{

		if(ap->va_size == 0)
			size=0;
		else
			size=(u_quad_t)(D_PFOFF(ap->va_size)+1);

		FI_SET_SP(&i,(size-ap->va_size));
		ap->va_size=size;
		error=tcfs_xsetflags(a->a_vp, a->a_p,a->a_cred,&i);
	}

	return tcfs_bypass((void*)v);
}

