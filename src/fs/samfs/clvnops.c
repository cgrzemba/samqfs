/*
 * ----- clvnops.c - Process the client vnode functions.
 *
 * Processes the vnode functions supported on the SAM Shared
 * Client File System.
 *
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision$"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <unistd.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/dirent.h>
#include <sys/fcntl.h>
#include <sys/sysmacros.h>
#include <sys/vnode.h>
#include <sys/fs_subr.h>
#include <sys/flock.h>
#include <sys/systm.h>
#include <sys/ioccom.h>
#include <sys/vmsystm.h>
#include <sys/file.h>
#include <sys/rwlock.h>
#include <sys/dnlc.h>
#include <sys/share.h>
#include <sys/policy.h>

#if defined(SOL_511_ABOVE)
#include <sys/vfs_opreg.h>
#endif

#include <nfs/nfs.h>
#include <nfs/lm.h>

#include <vm/as.h>
#include <vm/seg_vn.h>
#include <vm/seg_map.h>
#include <vm/pvn.h>

/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"
#include "sam/samaio.h"

#include "samfs.h"
#include "ino.h"
#include "macros.h"
#include "inode.h"
#include "mount.h"
#include "rwio.h"
#include "ioblk.h"
#include "extern.h"
#include "debug.h"
#include "trace.h"
#include "ino_ext.h"
#include "listio.h"
#include "kstats.h"

/*
 * delmap callback identity and status.
 */
typedef struct sam_delmapcall {
	kthread_t	*call_id;
	int		error; /* error from delmap */
	list_node_t	call_node;
} sam_delmapcall_t;

/*
 * delmap address space callback args
 */
typedef struct sam_delmap_args {
	vnode_t			*vp;
	offset_t		off;
	sam_size_t		len;
	uint_t			prot;
	uint_t			flags;
	sam_delmapcall_t	*caller; /* to retrieve errors from the cb */
} sam_delmap_args_t;


#if defined(SOL_511_ABOVE)
/*
 * ----- Solaris 11 SAM-QFS shared client vnode function prototypes.
 */
static int sam_client_open_vn(vnode_t **vpp, int, cred_t *credp,
	caller_context_t *ct);
static int sam_client_close_vn(vnode_t *vp, int filemode, int count,
	offset_t offset, cred_t *credp, caller_context_t *ct);
static int sam_client_read_vn(vnode_t *vp, struct uio *uiop, int ioflag,
	cred_t *credp, caller_context_t *ct);
static int sam_client_write_vn(vnode_t *vp, struct uio *uiop, int ioflag,
	cred_t *credp, caller_context_t *ct);
static int sam_client_getattr_vn(vnode_t *vp, struct vattr *vap, int flag,
	cred_t *credp, caller_context_t *ct);
static int sam_client_setattr_vn(vnode_t *vp, struct vattr *vap, int flag,
	cred_t *credp, caller_context_t *ct);
static int sam_client_lookup_vn(vnode_t *vp, char *cp, vnode_t **vpp,
	struct pathname *pnp, int flag, vnode_t *rdir, cred_t *credp,
	caller_context_t *ct, int *defp, struct pathname *rpnp);
static int sam_client_create_vn(vnode_t *vp, char *cp, struct vattr *vap,
	vcexcl_t ex, int mode, vnode_t **vpp, cred_t *credp, int filemode,
	caller_context_t *ct, vsecattr_t *vsap);
static int sam_client_remove_vn(vnode_t *pvp, char *cp, cred_t *credp,
	caller_context_t *ct, int flag);
static int sam_client_link_vn(vnode_t *pvp, vnode_t *vp, char *cp,
	cred_t *credp, caller_context_t *ct, int flag);
static int sam_client_rename_vn(vnode_t *opvp, char *omn, vnode_t *npvp,
	char *nnm, cred_t *credp, caller_context_t *ct, int flag);
static int sam_client_mkdir_vn(vnode_t *pvp, char *cp, struct vattr *vap,
	vnode_t **vpp, cred_t *credp, caller_context_t *ct, int flag,
	vsecattr_t *vsap);
static int sam_client_rmdir_vn(vnode_t *pvp, char *cp, vnode_t *cdir,
	cred_t *credp, caller_context_t *ct, int flag);
static int sam_client_readdir_vn(vnode_t *vp, struct uio *uiop, cred_t *credp,
	int *eofp, caller_context_t *ct, int flag);
static int sam_client_symlink_vn(vnode_t *pvp, char *cp, struct vattr *vap,
	char *tnm, cred_t *credp, caller_context_t *ct, int flag);
static int sam_client_readlink_vn(vnode_t *vp, struct uio *uiop, cred_t *credp,
	caller_context_t *ct);
static int sam_client_fsync_vn(vnode_t *vp, int filemode, cred_t *credp,
	caller_context_t *ct);
static int sam_client_frlock_vn(vnode_t *vp, int cmd, sam_flock_t *flp,
	int flag, offset_t offset, struct flk_callback *fcb, cred_t *credp,
	caller_context_t *ct);
static int sam_client_space_vn(vnode_t *vp, int cmd, sam_flock_t *flp,
	int flag, offset_t offset, cred_t *credp, caller_context_t *ct);
static int sam_client_getpage_vn(vnode_t *vp, offset_t offset,
	sam_size_t length, uint_t *protp, struct page **pglist,
	sam_size_t plsize, struct seg *segp, caddr_t addr, enum seg_rw rw,
	cred_t *credp, caller_context_t *ct);
static int sam_client_map_vn(vnode_t *vp, offset_t offset, struct as *asp,
	caddr_t *addrpp, sam_size_t length, uchar_t prot, uchar_t maxprot,
	uint_t flags, cred_t *credp, caller_context_t *ct);
static int sam_client_addmap_vn(vnode_t *vp, offset_t offset, struct as *asp,
	caddr_t a, sam_size_t length, uchar_t prot, uchar_t maxprot,
	uint_t flags, cred_t *credp, caller_context_t *ct);
static int sam_client_delmap_vn(vnode_t *vp, offset_t offset, struct as *asp,
	caddr_t a, sam_size_t length, uint_t prot, uint_t maxprot,
	uint_t flags, cred_t *credp, caller_context_t *ct);
static int sam_client_getsecattr_vn(vnode_t *vp, vsecattr_t *vsap, int flag,
	cred_t *credp, caller_context_t *ct);
static int sam_client_setsecattr_vn(vnode_t *vp, vsecattr_t *vsap, int flag,
	cred_t *credp, caller_context_t *ct);

#else
/*
 * ----- Solaris 10 SAM-QFS shared client vnode function prototypes.
 */
static int sam_client_open_vn(vnode_t **vpp, int, cred_t *credp);
static int sam_client_close_vn(vnode_t *vp, int filemode, int count,
	offset_t offset, cred_t *credp);
static int sam_client_read_vn(vnode_t *vp, struct uio *uiop, int ioflag,
	cred_t *credp, caller_context_t *ct);
static int sam_client_write_vn(vnode_t *vp, struct uio *uiop, int ioflag,
	cred_t *credp, caller_context_t *ct);
static int sam_client_getattr_vn(vnode_t *vp, struct vattr *vap, int flag,
	cred_t *credp);
static int sam_client_setattr_vn(vnode_t *vp, struct vattr *vap, int flag,
	cred_t *credp, caller_context_t *ct);
static int sam_client_lookup_vn(vnode_t *vp, char *cp, vnode_t **vpp,
	struct pathname *pnp, int flag, vnode_t *rdir, cred_t *credp);
static int sam_client_create_vn(vnode_t *vp, char *cp, struct vattr *vap,
	vcexcl_t ex, int mode, vnode_t **vpp, cred_t *credp, int filemode);
static int sam_client_remove_vn(vnode_t *pvp, char *cp, cred_t *credp);
static int sam_client_link_vn(vnode_t *pvp, vnode_t *vp, char *cp,
	cred_t *credp);
static int sam_client_rename_vn(vnode_t *opvp, char *omn, vnode_t *npvp,
	char *nnm, cred_t *credp);
static int sam_client_mkdir_vn(vnode_t *pvp, char *cp, struct vattr *vap,
	vnode_t **vpp, cred_t *credp);
static int sam_client_rmdir_vn(vnode_t *pvp, char *cp, vnode_t *cdir,
	cred_t *credp);
static int sam_client_readdir_vn(vnode_t *vp, struct uio *uiop, cred_t *credp,
	int *eofp);
static int sam_client_symlink_vn(vnode_t *pvp, char *cp, struct vattr *vap,
	char *tnm, cred_t *credp);
static int sam_client_readlink_vn(vnode_t *vp, struct uio *uiop, cred_t *credp);
static int sam_client_fsync_vn(vnode_t *vp, int filemode, cred_t *credp);
static int sam_client_frlock_vn(vnode_t *vp, int cmd, sam_flock_t *flp,
	int flag, offset_t offset, struct flk_callback *fcb, cred_t *credp);
static int sam_client_space_vn(vnode_t *vp, int cmd, sam_flock_t *flp,
	int flag, offset_t offset, cred_t *credp, caller_context_t *ct);
static int sam_client_getpage_vn(vnode_t *vp, offset_t offset,
	sam_size_t length, uint_t *protp, struct page **pglist,
	sam_size_t plsize, struct seg *segp, caddr_t addr, enum seg_rw rw,
	cred_t *credp);
static int sam_client_map_vn(vnode_t *vp, offset_t offset, struct as *asp,
	caddr_t *addrpp, sam_size_t length, uchar_t prot, uchar_t maxprot,
	uint_t flags, cred_t *credp);
static int sam_client_addmap_vn(vnode_t *vp, offset_t offset, struct as *asp,
	caddr_t a, sam_size_t length, uchar_t prot, uchar_t maxprot,
	uint_t flags, cred_t *credp);
static int sam_client_delmap_vn(vnode_t *vp, offset_t offset, struct as *asp,
	caddr_t a, sam_size_t length, uint_t prot, uint_t maxprot,
	uint_t flags, cred_t *credp);
static int sam_client_getsecattr_vn(vnode_t *vp, vsecattr_t *vsap, int flag,
	cred_t *credp);
static int sam_client_setsecattr_vn(vnode_t *vp, vsecattr_t *vsap, int flag,
	cred_t *credp);

#endif	/* SAM-QFS shared client vnode function prototypes */

static void sam_mmap_rmlease(vnode_t *vp, boolean_t is_write, int pages);
static int sam_find_and_delete_delmapcall(sam_node_t *ip, int *errp);
static sam_delmapcall_t *sam_init_delmapcall(void);
static void sam_delmap_callback(struct as *as, void *arg, uint_t event);

/*
 * ----- Vnode operations supported on the SAM-QFS shared file system.
 */
struct vnodeops *samfs_client_vnodeopsp;

const fs_operation_def_t samfs_client_vnodeops_template[] = {
	VOPNAME_OPEN, sam_client_open_vn, /* will not be blocked by lockfs */
	VOPNAME_CLOSE, sam_client_close_vn, /* will not be blocked by lockfs */
	VOPNAME_READ, sam_client_read_vn,
	VOPNAME_WRITE, sam_client_write_vn,
	VOPNAME_IOCTL, sam_ioctl_vn,
#if (0)
	VOPNAME_SETFL, ufs_setfl_vn,
#endif
	VOPNAME_GETATTR, sam_client_getattr_vn,
	VOPNAME_SETATTR, sam_client_setattr_vn,
	VOPNAME_ACCESS, sam_access_vn,
	VOPNAME_LOOKUP, sam_client_lookup_vn,
	VOPNAME_CREATE, sam_client_create_vn,
	VOPNAME_REMOVE, sam_client_remove_vn,
	VOPNAME_LINK, sam_client_link_vn,
	VOPNAME_RENAME, sam_client_rename_vn,
	VOPNAME_MKDIR, sam_client_mkdir_vn,
	VOPNAME_RMDIR, sam_client_rmdir_vn,
	VOPNAME_READDIR, sam_client_readdir_vn,
	VOPNAME_SYMLINK, sam_client_symlink_vn,
	VOPNAME_READLINK, sam_client_readlink_vn,
	VOPNAME_FSYNC, sam_client_fsync_vn,
	VOPNAME_INACTIVE, (fs_generic_func_p) sam_inactive_vn, /* not blocked */
	VOPNAME_FID, sam_fid_vn,
	VOPNAME_RWLOCK, (fs_generic_func_p) sam_rwlock_vn, /* not blocked */
	VOPNAME_RWUNLOCK, (fs_generic_func_p) sam_rwunlock_vn, /* not blocked */
	VOPNAME_SEEK, sam_seek_vn,
#if (0)
	VOPNAME_CMP, ufs_cmp_vn,
#endif
	VOPNAME_FRLOCK, sam_client_frlock_vn,
	VOPNAME_SPACE, sam_client_space_vn,
#if (0)
	VOPNAME_REALVP, ufs_realvp_vn,
#endif
	VOPNAME_GETPAGE, sam_client_getpage_vn,
	VOPNAME_PUTPAGE, sam_putpage_vn,
	VOPNAME_MAP, (fs_generic_func_p) sam_client_map_vn,
	VOPNAME_ADDMAP,
		(fs_generic_func_p) sam_client_addmap_vn, /* not blocked */
	VOPNAME_DELMAP,
		sam_client_delmap_vn, /* will not be blocked by lockfs */
	VOPNAME_POLL, (fs_generic_func_p) fs_poll, /* not blocked */
#if (0)
	VOPNAME_DUMP, ufs_dump_vn,
#endif
	VOPNAME_PATHCONF, sam_pathconf_vn,
#if (0)
	VOPNAME_PAGEIO, ufs_pageio_vn,
	VOPNAME_DUMPCTL, ufs_dumpctl_vn,
	VOPNAME_DISPOSE, ufs_dispose_vn,
#endif
	VOPNAME_GETSECATTR, sam_client_getsecattr_vn,
	VOPNAME_SETSECATTR, sam_client_setsecattr_vn,
#if (0)
	VOPNAME_SHRLOCK, ufs_shrlock_vn,
#endif
	VOPNAME_VNEVENT, fs_vnevent_support,
	NULL, NULL
};

struct vnodeops *samfs_client_vnode_staleopsp;

const fs_operation_def_t samfs_client_vnode_staleops_template[] = {
	VOPNAME_OPEN, sam_EIO,
	VOPNAME_CLOSE, sam_close_stale_vn,
	VOPNAME_READ, sam_EIO,
	VOPNAME_WRITE, sam_EIO,
	VOPNAME_IOCTL, sam_EIO,
#if (0)
	VOPNAME_SETFL, ufs_setfl_vn,
#endif
	VOPNAME_GETATTR, sam_EIO,
	VOPNAME_SETATTR, sam_EIO,
	VOPNAME_ACCESS, sam_EIO,
	VOPNAME_LOOKUP, sam_EIO,
	VOPNAME_CREATE, sam_EIO,
	VOPNAME_REMOVE, sam_EIO,
	VOPNAME_LINK, sam_EIO,
	VOPNAME_RENAME, sam_EIO,
	VOPNAME_MKDIR, sam_EIO,
	VOPNAME_RMDIR, sam_EIO,
	VOPNAME_READDIR, sam_EIO,
	VOPNAME_SYMLINK, sam_EIO,
	VOPNAME_READLINK, sam_EIO,
	VOPNAME_FSYNC, sam_EIO,
	VOPNAME_INACTIVE, (fs_generic_func_p) sam_inactive_stale_vn,
	VOPNAME_FID, sam_EIO,
	VOPNAME_RWLOCK, (fs_generic_func_p) sam_rwlock_vn,
	VOPNAME_RWUNLOCK, (fs_generic_func_p) sam_rwunlock_vn,
	VOPNAME_SEEK, sam_EIO,
#if (0)
	VOPNAME_CMP, ufs_cmp_vn,
#endif
	VOPNAME_FRLOCK, sam_EIO,
	VOPNAME_SPACE, sam_EIO,
#if (0)
	VOPNAME_REALVP, ufs_realvp_vn,
#endif
	VOPNAME_GETPAGE, sam_EIO,
	VOPNAME_PUTPAGE, sam_putpage_vn,
	VOPNAME_MAP, (fs_generic_func_p) sam_EIO,
	VOPNAME_ADDMAP, (fs_generic_func_p) sam_EIO,
	VOPNAME_DELMAP, sam_delmap_stale_vn,
	VOPNAME_POLL, (fs_generic_func_p) fs_poll,
#if (0)
	VOPNAME_DUMP, ufs_dump_vn,
#endif
	VOPNAME_PATHCONF, sam_EIO,
#if (0)
	VOPNAME_PAGEIO, ufs_pageio_vn,
	VOPNAME_DUMPCTL, ufs_dumpctl_vn,
	VOPNAME_DISPOSE, ufs_dispose_vn,
#endif
	VOPNAME_GETSECATTR, sam_EIO,
	VOPNAME_SETSECATTR, sam_EIO,
#if (0)
	VOPNAME_SHRLOCK, ufs_shrlock_vn,
	VOPNAME_VNEVENT, fs_vnevent_support,
#endif
	NULL, NULL
};


#ifdef METADATA_SERVER
extern struct vnodeops *samfs_vnodeopsp;
#endif


/*
 * ----- sam_client_open_vn - Open a SAM-QFS shared file.
 */

/* ARGSUSED3 */
static int		/* ERRNO if error, 0 if successful. */
sam_client_open_vn(
#if defined(SOL_511_ABOVE)
	vnode_t **vpp,		/* pointer pointer to vnode */
	int filemode,		/* open file mode */
	cred_t *credp,		/* credentials pointer */
	caller_context_t *ct	/* caller context pointer */
#else
	vnode_t **vpp,		/* pointer pointer to vnode */
	int filemode,		/* open file mode */
	cred_t *credp		/* credentials pointer */
#endif
)
{
	sam_node_t *ip;
	vnode_t *vp = *vpp;
	int error = 0;
	int dec_no_opens = 0;

	ip = SAM_VTOI(vp);
	TRACE(T_SAM_CL_OPEN, vp, ip->di.id.ino, filemode, ip->di.status.bits);

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

	RW_LOCK_OS(&ip->inode_rwl, RW_READER);

	if (ip->di.id.ino == SAM_INO_INO) {
		error = EPERM;
	} else if (ip->di.status.b.damaged) {
		error = ENODATA;		/* best fit errno */
	} else if (S_ISREQ(ip->di.mode)) {	/* If removable media file */
#ifdef METADATA_SERVER
		if (SAM_IS_SHARED_SERVER(ip->mp)) {
			if (!rw_tryupgrade(&ip->inode_rwl)) {
				RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			}
			error = sam_open_rm(ip, filemode, credp);
			rw_downgrade(&ip->inode_rwl);
		} else {
			error = ENOTSUP;
		}
#else
		error = ENOTSUP;
#endif
	}
	if (error) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		goto out;
	}

	/*
	 * The default mode disallows archiving while the file is opened for
	 * write. If the attribute, concurrent_archive (archive -c) is set,
	 * the file is archived while opened for write. Used by databases.
	 */
	if ((ip->di.status.b.concurrent_archive == 0) &&
	    (filemode & (FCREAT | FTRUNC | FWRITE))) {
		mutex_enter(&ip->fl_mutex);
		ip->flags.b.write_mode = 1; /* File opened for write */
		mutex_exit(&ip->fl_mutex);
	}

	/*
	 * If the file is not currently opened (by any process on this host),
	 * request an open lease.  This is used by the server to track which
	 * files are in use by clients.
	 *
	 * If the file mode indicates that the file is being opened for read,
	 * the server will issue a read lease in addition to the open lease,
	 * under the assumption that the file will soon be read.
	 */
	if (S_ISREG(ip->di.mode)) {
		proc_t *p = ttoproc(curthread);

		if (MANDLOCK(vp, ip->di.mode)) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			error = EACCES;
			goto out;
		}
		/*
		 * If exec'ed program, don't increment count because there is
		 * no close. Note that this means we may request multiple open
		 * leases, which may never be released.
		 *
		 * XXX - We could release them if we had some way of detecting
		 *		when the inode was inactive except for holds
		 *		due to leases.
		 */
		mutex_enter(&ip->fl_mutex);
		if (!(p->p_proc_flag & P_PR_EXEC)) {
			ip->no_opens++;
			dec_no_opens = 1;
		}

		if ((ip->no_opens == 0) ||
		    ((ip->no_opens == 1) && dec_no_opens)) {
			sam_lease_data_t data;

			mutex_exit(&ip->fl_mutex);
			bzero(&data, sizeof (data));

			data.ltype = LTYPE_open;
			data.alloc_unit = ip->cl_alloc_unit;
			data.filemode = filemode;
			data.shflags.b.directio = ip->flags.b.directio;
			data.shflags.b.abr = ip->flags.b.abr;
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			error = sam_proc_get_lease(ip, &data,
			    NULL, NULL, SHARE_wait, credp);
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		} else {
			mutex_exit(&ip->fl_mutex);
		}
	}

	if (error != 0) {
		mutex_enter(&ip->fl_mutex);
		if (dec_no_opens) {
			ip->no_opens--;
			if (ip->no_opens < 0) {
				ip->no_opens = 0;
			}
		}
		if ((ip->no_opens == 0) && (ip->pending_mmappers == 0) &&
		    (ip->last_unmap == 0) && (ip->mm_pages == 0) &&
		    S_ISREG(ip->di.mode)) {
			/*
			 * If error on open and no mmap pages, update
			 * inode on the server and cancel lease.
			 * LEASE_remove waits
			 * for the response. This allows the allocated pages
			 * to be released.
			 * Set last_unmap to prevent lease race with new
			 * mmappers.
			 */
			ip->last_unmap = 1;
			mutex_exit(&ip->fl_mutex);

			error = sam_proc_rm_lease(ip, CL_CLOSE, RW_READER);

			mutex_enter(&ip->fl_mutex);
			ASSERT(ip->last_unmap == 1);
			ASSERT(ip->pending_mmappers == 0);
			ASSERT(ip->mm_pages == 0);
			ip->last_unmap = 0;
		}
		mutex_exit(&ip->fl_mutex);
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);

out:
	SAM_CLOSE_OPERATION(ip->mp, error);
	TRACE(T_SAM_CL_OPEN_RET,
	    vp, (sam_tr_t)ip->di.rm.size, ip->no_opens, error);
	return (error);
}


/*
 * ----- sam_close_stale_vn
 *
 * Destroy an inode/vnode that is being closed.
 * Get here on close when forced unmount was done or for
 * some sam_close_vn/sam_client_close_vn error handling.
 */

/* ARGSUSED5 */
int				/* ERRNO if error, 0 if successful. */
sam_close_stale_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	int filemode,		/* open file mode. */
	int count,		/* reference count for the closing file */
	offset_t offset,	/* offset pointer. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	int filemode,		/* open file mode. */
	int count,		/* reference count for the closing file */
	offset_t offset,	/* offset pointer. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	sam_node_t *ip;
	sam_mount_t *mp;
	boolean_t last_close;

	ip = SAM_VTOI(vp);
	mp = ip->mp;

	/*
	 * Prevent the freeing of the mount structures while closing.
	 */
	SAM_VFS_HOLD(mp);

	/*
	 * Wait for any ongoing unmount to finish
	 */
	sam_await_umount_complete(ip);

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	last_close = FALSE;
	if (count <= 1) {
		mutex_enter(&ip->fl_mutex);
		if (--ip->no_opens <= 0) {
			ip->no_opens = 0;
			last_close = 1;
		}
		mutex_exit(&ip->fl_mutex);
	}
	TRACE(T_SAM_KSTALE_VNO, vp, vp->v_count, last_close, ip->no_opens);

	if (ip->cl_locks) {
		if (SAM_IS_SHARED_SERVER(mp) || SAM_IS_STANDALONE(mp)) {
			int error;
			sam_flock_t flock;

			bzero(&flock, sizeof (flock));
			flock.l_type = F_UNLCK;
			TRACE(T_SAM_FRLOCK, vp, 0, filemode, offset);
			error = FS_FRLOCK_OS(vp, 0, &flock, filemode, offset,
			    NULL, credp, NULL);
			TRACE(T_SAM_FRLOCK_RET, vp, 0, filemode, error);
		}
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

	/*
	 * Avoid race conditions herein by serializing closes after
	 * forced unmount.  Any mutex is suitable, provided it doesn't
	 * conflict with the inode/vnode locks, or have precedence
	 * issues w/ them.  We choose shared_lock because it shouldn't
	 * be active after sam-sharefsd has been told to exit.
	 */
	mutex_enter(&mp->ms.m_shared_lock);

	if (last_close) {
		ip->cl_closing = 0;
	}
	if (ip->listiop) {
		(void) sam_remove_listio(ip);
	}

	if (vn_has_cached_data(vp)) {
		/*
		 * If in forced unmount, sam_flush_pages()
		 * will silently throw pages away.
		 */
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		sam_flush_pages(ip, B_INVAL);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (last_close && vn_has_cached_data(vp)) {
			/*
			 * Should not happen for forced umount.  I/O errors
			 * shouldn't be possible in sam_flush_pages() because
			 * no I/O should be done.
			 */
			TRACE(T_SAM_KSTALE_VNOERR, vp, vp->v_count, 1,
			    ip->no_opens);
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: inode %d.%d has unflushable pages\n",
			    ip->mp->mt.fi_name, ip->di.id.ino, ip->di.id.gen);
			goto out;	/* Do not free vnode */
		}
	}

	/*
	 * Remove all leases.  Nuke 'em -- we're not connected to the server.
	 */
	sam_client_remove_all_leases(ip);		/* can call VN_RELE() */

	if (S_ISSEGS(&ip->di) && ip->seg_held) {
		sam_rele_index(ip);
	}

out:
	mutex_exit(&mp->ms.m_shared_lock);
	SAM_VFS_RELE(mp);

	return (EIO);
}


/*
 * ----- sam_client_close_vn - Close a SAM-QFS shared file.
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_close_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	int filemode,		/* open file mode. */
	int count,		/* reference count for the closing file */
	offset_t offset,	/* offset pointer. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	int filemode,		/* open file mode. */
	int count,		/* reference count for the closing file */
	offset_t offset,	/* offset pointer. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	sam_node_t *ip;
	int error;
	boolean_t write_lock = FALSE;
	boolean_t last_close = FALSE;
	boolean_t last_unmap = FALSE;
	krw_t rw_type = RW_READER;

	ip = SAM_VTOI(vp);
	TRACE(T_SAM_CL_CLOSE, vp, ip->di.id.ino, ip->cl_leases, ip->flags.bits);

	/*
	 * Two possible situations here.  If we're attempting to unmount the
	 * FS, then we should let this through (we need to let the archiver
	 * close .inodes, if this is a SAM-QFS FS).  If we're failing over,
	 * we hold things up until the failover completes.
	 */
	if ((ip->mp->mt.fi_status & FS_UMOUNT_IN_PROGRESS) ||
	    !(ip->mp->mt.fi_status & FS_MOUNTED)) {
		sam_open_operation_nb(ip->mp);		/* non-blocking */
	} else {
		if ((error = sam_open_operation(ip)) != 0) {
			SAM_NFS_JUKEBOX_ERROR(error);
#if defined(SOL_511_ABOVE)
			return (sam_close_stale_vn(vp, filemode, count,
			    offset, credp, ct));
#else
			return (sam_close_stale_vn(vp, filemode, count,
			    offset, credp));
#endif
		}
	}

	/*
	 * If advisory locks set (frlock call issued), ask
	 * server to clean locks.
	 *
	 * XXX - If this fails, we still need to execute the close.
	 *	 Should we retry the clean-lock operation?  We'll do
	 *	 it once for now....
	 */
	RW_LOCK_OS(&ip->inode_rwl, rw_type);
	if (ip->cl_locks) {
		sam_flock_t flock;

		if (!rw_tryupgrade(&ip->inode_rwl)) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		rw_type = RW_WRITER;
		bzero(&flock, sizeof (flock));
		flock.l_type = F_UNLCK;
		error = sam_client_frlock_ino(ip, 0, &flock, filemode, offset,
		    NULL, credp);
		if (error != 0) {
			error = sam_client_frlock_ino(ip, 0, &flock, filemode,
			    offset, NULL, credp);
		}
	}
	RW_UNLOCK_OS(&ip->inode_rwl, rw_type);

#if defined(SOL_511_ABOVE)
	error = sam_close_vn(vp, filemode, count, offset, credp, ct);
#else
	error = sam_close_vn(vp, filemode, count, offset, credp);
#endif

	/*
	 * On last reference close for a regular file, sync pages on the client.
	 * There is a possible deadlock which a thread taking the inode
	 * rw lock as a writer here can contribute to.  The condition occurs
	 * when multiple threads want the inode rw lock as a reader and would
	 * block here if we try to take the lock as a writer.  Take the lock
	 * as a writer if there are no readers present.  If there are readers,
	 * drop the lock to clear the exclusive access wanted bit and allow
	 * readers to continue.  Retry the lock after allowing other threads
	 * to run.  The comments in sam_getpages will shed more light on this
	 * problem.
	 */
	last_close = FALSE;
	RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	/*
	 * Need the mutex so that only one
	 * last_close is done.
	 */
	mutex_enter(&ip->fl_mutex);
	if ((count <= 1) && (ip->no_opens == 0) && (ip->cl_closing == 0) &&
	    (ip->pending_mmappers == 0) && (ip->mm_pages == 0)) {
		last_close = TRUE;
		ip->cl_closing = 1;
	}
	mutex_exit(&ip->fl_mutex);

	if (last_close) {
retry:
		if (!RW_TRYUPGRADE_OS(&ip->inode_rwl)) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			delay(1);
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			goto retry;
		} else {
			write_lock = TRUE;
		}

		if (S_ISREG(ip->di.mode) && vn_has_cached_data(vp)) {
			/*
			 * Will drop and reacquire the
			 * inode_rwl, RW_WRITER lock.
			 */
			sam_flush_pages(ip, 0);
		}

		/*
		 * Now that we hold inode_rwl,RW_WRITER we
		 * need to check for a new open.
		 */
		if ((ip->no_opens > 0) || (ip->pending_mmappers > 0) ||
		    (ip->mm_pages > 0)) {
			/*
			 * A new open has jumped in
			 * so skip the last_close.
			 */
			ip->cl_closing = 0;
			last_close = FALSE;
			goto done;
		}

		/*
		 * Prevent a lease race with any new mmappers.
		 */
		if ((ip->pending_mmappers == 0) && (ip->last_unmap == 0) &&
		    (ip->mm_pages == 0)) {
			ip->last_unmap = 1;
			last_unmap = TRUE;
		}
	}

	/*
	 * On last close and no mmapped pages, update inode on
	 * the server and cancel leases. LEASE_remove waits for the response.
	 */
	if (last_close && last_unmap && S_ISREG(ip->di.mode)) {
		error = sam_proc_rm_lease(ip, CL_CLOSE, RW_WRITER);
	}

	if (last_close) {
		ip->cl_closing = 0;
	}
	if (last_unmap) {
		ASSERT(ip->last_unmap == 1);
		ASSERT(ip->pending_mmappers == 0);
		ASSERT(ip->mm_pages == 0);
		ip->last_unmap = 0;
	}

done:
	if (write_lock) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	} else {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}

	SAM_CLOSE_OPERATION(ip->mp, error);
	TRACE(T_SAM_CL_CLOSE_RET, vp, ip->cl_leases, ip->di.rm.size, error);
	return (error);
}


/*
 * ----- sam_client_read_vn - Read a SAM-QFS file.
 *	Parameters are in user I/O vector array.
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_read_vn(
	vnode_t *vp,		/* pointer to vnode. */
	uio_t *uiop,		/* user I/O vector array. */
	int ioflag,		/* file I/O flags (/usr/include/sys/file.h). */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct)	/* caller context pointer. */
{
	sam_node_t *ip;
	int error = 0;
	boolean_t using_lease = FALSE;

	ip = SAM_VTOI(vp);
	TRACE(T_SAM_CL_READ, vp, ip->di.id.ino, (sam_tr_t)uiop->uio_loffset,
	    uiop->uio_resid);

	if (MANDLOCK(vp, ip->di.mode)) {
		return (EACCES);
	}

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

	/*
	 * For a regular file, get a read lease.
	 * The lease will be cancelled at close or it will expire automatically.
	 */
	if (S_ISREG(ip->di.mode)) {
		sam_lease_data_t data;

		bzero(&data, sizeof (data));

		mutex_enter(&ip->ilease_mutex);
		ip->cl_leaseused[LTYPE_read]++;
		mutex_exit(&ip->ilease_mutex);
		using_lease = TRUE;

		data.ltype = LTYPE_read;
		data.lflag = 0;
		data.sparse = SPARSE_none;
		data.offset = uiop->uio_loffset;
		data.resid = uiop->uio_resid;
		data.alloc_unit = ip->cl_alloc_unit;
		data.filemode = ioflag;
		data.cmd = 0;
		data.shflags.b.directio = ip->flags.b.directio;
		data.shflags.b.abr = ip->flags.b.abr;
		error = sam_proc_get_lease(ip, &data, NULL, NULL, SHARE_wait,
		    credp);
	}
	if (error == 0) {
		error = sam_read_vn(vp, uiop, ioflag, credp, ct);
	}
	if (using_lease) {
		mutex_enter(&ip->ilease_mutex);
		ip->cl_leaseused[LTYPE_read]--;
		if ((ip->cl_leaseused[LTYPE_read] == 0) &&
		    (ip->cl_leasetime[LTYPE_read] <= lbolt)) {
			sam_sched_expire_client_leases(ip->mp, 0, FALSE);
		}
		mutex_exit(&ip->ilease_mutex);
	}
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


/*
 * ----- sam_client_write_vn - Write a SAM-QFS file.
 *	Parameters are in user I/O vector array.
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_write_vn(
	vnode_t *vp,		/* pointer to vnode. */
	uio_t *uiop,		/* user I/O vector array. */
	int ioflag,		/* file I/O flags (/usr/include/sys/file.h). */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct)	/* caller context pointer. */
{
	sam_node_t *ip;
	int error = 0;
	boolean_t using_lease = FALSE;
	boolean_t appending = FALSE;

	ip = SAM_VTOI(vp);
	TRACE(T_SAM_CL_WRITE, vp, ip->di.id.ino, (sam_tr_t)uiop->uio_loffset,
	    (uint_t)uiop->uio_resid);

	if (SAM_PRIVILEGE_INO(ip->di.version, ip->di.id.ino)) {
		return (EPERM);
	}
	if (uiop->uio_resid == 0) {
		return (0);
	}

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

	/*
	 * For a regular file, get a write or write append lease.
	 * The lease will be cancelled at close or it will expire automatically.
	 */
	if (S_ISREG(ip->di.mode)) {
		sam_lease_data_t data;

		bzero(&data, sizeof (data));

		data.lflag = 0;
		data.sparse = SPARSE_none;
		if (ioflag & FAPPEND) {
			data.offset = ip->di.rm.size;
		} else {
			data.offset = uiop->uio_loffset;
		}
		data.resid = uiop->uio_resid;
		data.alloc_unit = ip->cl_alloc_unit;
		data.filemode = ioflag;
		data.cmd = 0;
		data.shflags.b.directio = ip->flags.b.directio;
		if (data.shflags.b.directio &&
		    (ip->mp->mt.fi_config & MT_ZERO_DIO_SPARSE)) {
			/*
			 * Tell the server not to allocate space for this lease
			 * request. We need to know when writing to a sparse
			 * region so any needed zeroing can be done. A later
			 * call to sam_map_block will return ELNRNG if we are
			 * writing to a sparse region.
			 */
			data.sparse = SPARSE_noalloc;
		}
		data.shflags.b.abr = ip->flags.b.abr;
		appending = FALSE;
		if ((ioflag & FAPPEND) ||
		    ((uiop->uio_loffset + uiop->uio_resid) > ip->di.rm.size)) {
			appending = TRUE;
		} else if (ioflag == FASYNC &&
		    ((struct samaio_uio *)uiop)->type ==
		    SAMAIO_LIO_PAGED) {
			struct samaio_uio *aiop = (struct samaio_uio *)uiop;
			struct sam_listio_call *callp =
			    (struct sam_listio_call *)aiop->bp;
			int i;

			/*
			 * Note uio_loffset = file_off[0] and uio_resid =
			 * file_len[0].  These were checked in the original
			 * "if", so start this "for" loop with file_off[1] and
			 * file_len[1].
			 */
			for (i = 1; i < callp->file_cnt; i++) {
				if (callp->file_off[i] + callp->file_len[i] >
				    ip->di.rm.size) {
					appending = TRUE;
					break;
				}
			}
		}
		data.ltype = appending ? LTYPE_append : LTYPE_write;

		/*
		 * Prevent existing or new lease from being relinquished
		 * until this write is complete. Note: We'll hold onto
		 * the lease even if we enter sam_wait_space. In this case
		 * the server may forcibly expire it.  That's OK, we
		 * will reacquire the lease when space is available.
		 */
		mutex_enter(&ip->ilease_mutex);
		ip->cl_leaseused[LTYPE_write]++;
		if (appending) {
			ip->cl_leaseused[LTYPE_append]++;
		}
		mutex_exit(&ip->ilease_mutex);
		using_lease = TRUE;

		error = sam_proc_get_lease(ip, &data, NULL, NULL, SHARE_wait,
		    credp);
		/* XXX - Do we need to check again for writing past EOF? */
	}
	if (error == 0) {
		error = sam_write_vn(vp, uiop, ioflag, credp, ct);
	}
	if (using_lease) {
		mutex_enter(&ip->ilease_mutex);
		ip->cl_leaseused[LTYPE_write]--;
		if (appending) {
			ip->cl_leaseused[LTYPE_append]--;
		}
		if (((ip->cl_leaseused[LTYPE_write] == 0) &&
		    (ip->cl_leasetime[LTYPE_write] <= lbolt)) ||
		    (appending &&
		    (ip->cl_leaseused[LTYPE_append] == 0) &&
		    (ip->cl_leasetime[LTYPE_append] <= lbolt))) {
			sam_sched_expire_client_leases(ip->mp, 0, FALSE);
		}
		mutex_exit(&ip->ilease_mutex);
	}
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


/*
 * ----- sam_client_getattr_vn - Get attributes call.
 *	Get attributes in server host for this SAM-QFS client file.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_client_getattr_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	vattr_t *vap,		/* vattr pointer. */
	int flags,		/* flags. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	vattr_t *vap,		/* vattr pointer. */
	int flags,		/* flags. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	sam_node_t *ip;
	int error;

	ip = SAM_VTOI(vp);
	/*
	 * Avoid over-the-wire traffic and also the potential freeze when
	 * flags is ATTR_HINT. Note, when VM function rm_assize() issues a
	 * VOP_GETATTR(), the address space is locked and we don't want an
	 * OTW holding this lock.
	 */
	if ((flags & ATTR_HINT) && (vap->va_mask ==
	    (vap->va_mask & (AT_SIZE | AT_FSID | AT_RDEV))) &&
	    (ip->di.id.ino != SAM_INO_INO)) {
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		if (vap->va_mask & AT_SIZE) {
			if (S_ISREQ(ip->di.mode) &&
			    ip->di.rm.size == MAXOFFSET_T) {
				vap->va_size = 0;
			} else if (S_ISLNK(ip->di.mode) &&
			    (ip->di.ext_attrs & ext_sln)) {
				vap->va_size = ip->di.psize.symlink;
			} else {
				vap->va_size = ip->di.rm.size;
			}
		}
		if (vap->va_mask & AT_FSID) {
			vap->va_fsid = ip->mp->mi.m_vfsp->vfs_dev;
		}
		if (vap->va_mask & AT_RDEV) {
			if ((vp->v_type == VCHR) || (vp->v_type == VBLK)) {
				vap->va_rdev = vp->v_rdev;
			} else {
				/* Device the file represents */
				vap->va_rdev = ip->rdev;
			}
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		return (0);
	}
	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

	if (ip->di.id.ino != SAM_INO_INO) {
		if (!SAM_IS_SHARED_SERVER(ip->mp)) {
			int meta_timeo;

			if ((ip->cl_flags & (SAM_UPDATED | SAM_CHANGED)) &&
			    !(ip->cl_leases & SAM_DATA_MODIFYING_LEASES)) {
				/*
				 * Force an inode update if updated or changed
				 * and not holding any
				 * SAM_DATA_MODIFYING_LEASES.
				 */
				meta_timeo = 0;
			} else {
				/*
				 * Update inode on configured timeout interval.
				 */
				meta_timeo = ip->mp->mt.fi_meta_timeo;
			}

			error = sam_refresh_client_ino(ip, meta_timeo, credp);
		} else {
			if (ip->mp->mt.fi_config & MT_CONSISTENT_ATTR) {
				if ((ip->updtime + ip->mp->mt.fi_meta_timeo) <=
				    SAM_SECOND()) {
					error = sam_client_getino(ip,
					    ip->mp->ms.m_client_ord);
				}
			}
		}
	}
	if (error == 0) {
#if defined(SOL_511_ABOVE)
		error = sam_getattr_vn(vp, vap, flags, credp, ct);
#else
		error = sam_getattr_vn(vp, vap, flags, credp);
#endif
	}
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


/*
 * ----- sam_client_setattr_vn - Set attributes call.
 *	Set attributes in server host for this SAM-QFS client file.
 */

/* ARGSUSED4 */
int				/* ERRNO if error, 0 if successful. */
sam_client_setattr_vn(
	vnode_t *vp,		/* pointer to vnode. */
	vattr_t *vap,		/* vattr pointer. */
	int flags,		/* flags. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct)	/* caller context pointer. */
{
	sam_node_t *ip;
	uint_t mask;
	sam_inode_setattr_t setattr;
	int error;

	mask = vap->va_mask;
	TRACE(T_SAM_CL_SETATTR, vp, (sam_tr_t)vap, flags, mask);
	ip = SAM_VTOI(vp);
	if (mask & AT_NOSET) {
		return (EINVAL);
	}

	setattr.flags = flags;
	if ((error = sam_v_to_v32(vap, &setattr.vattr)) != 0) {
		return (error);
	}

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

	/*
	 * The truncate must be issued from the client in order
	 * for the initiating client to get the truncate lease.
	 * Note, a truncate lease will be granted for a client
	 * who owns other leases.
	 */
	if (mask & AT_SIZE) {				/* -----Change size */
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (vp->v_type == VDIR) {
			error = EISDIR;	/* Cannot change size of a directory */
		} else if ((error = sam_access_ino(ip, S_IWRITE, TRUE, credp))
		    == 0) {
			if (S_ISREQ(ip->di.mode)) {
				/*
				 * Can only truncate a regular file
				 */
				error = EINVAL;
			} else if (SAM_PRIVILEGE_INO(ip->di.version,
			    ip->di.id.ino)) {
				/*
				 * Can't truncate privileged inodes
				 */
				error = EPERM;
			} else if (S_ISSEGI(&ip->di)) {
				/*
				 * Segment not supported on shared
				 */
				error = ENOTSUP;
			} else {
				error = sam_clear_ino(ip,
				    (offset_t)vap->va_size,
				    STALE_ARCHIVE, credp);
			}
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	} else {
		error = sam_proc_inode(ip, INODE_setattr, &setattr, credp);
	}

	SAM_CLOSE_OPERATION(ip->mp, error);
	TRACE(T_SAM_CL_SETATT_RET, vp, ip->di.rm.size, ip->di.mode, error);
	return (error);
}


/*
 * ----- sam_client_lookup_vn - Look up a SAM-QFS file.
 *
 * Given a parent directory and pathname component, look up
 * and return the vnode found. If found, vnode is "held".
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_lookup_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *pvp,		/* Pointer to parent directory vnode */
	char *cp,		/* Pointer to the component name. */
	vnode_t **vpp,		/* Pointer pointer to the vnode (returned). */
	pathname_t *pnp,	/* Pointer to the pathname. */
	int flags,		/* Flags. */
	vnode_t *rdir,		/* Pointer to root directory vnode. */
	cred_t *credp,		/* Credentials pointer. */
	caller_context_t *ct,	/* Caller context pointer. */
	int *defp,		/* Returned per-dirent flags. */
	struct pathname *rpnp	/* Returned case-preserved name in directory. */
#else
	vnode_t *pvp,		/* Pointer to parent directory vnode */
	char *cp,		/* Pointer to the component name. */
	vnode_t **vpp,		/* Pointer pointer to the vnode (returned). */
	pathname_t *pnp,	/* Pointer to the pathname. */
	int flags,		/* Flags. */
	vnode_t *rdir,		/* Pointer to root directory vnode. */
	cred_t *credp		/* Credentials pointer. */
#endif
)
{
	int error;
	sam_node_t *pip;
	sam_node_t *ip;

	TRACES(T_SAM_CL_LOOKUP, pvp, cp);
	pip = SAM_VTOI(pvp);

	/*
	 * Return parent if lookup for ".".
	 */
	if ((*(cp+1) == '\0' && *cp == '.')) {
		VN_HOLD(pvp);
		*vpp = pvp;
		return (0);
	}

	if ((error = sam_open_operation(pip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

	/*
	 * If server, do lookup conventionally.
	 */
#ifdef METADATA_SERVER
	if (SAM_IS_SHARED_SERVER(pip->mp)) {
#if defined(SOL_511_ABOVE)
		error = sam_lookup_vn(pvp, cp, vpp, pnp, flags, rdir, credp,
		    ct, defp, rpnp);
#else
		error = sam_lookup_vn(pvp, cp, vpp, pnp, flags, rdir, credp);
#endif
		goto out;
	}
#endif

	if ((error = sam_client_lookup_name(pip, cp,
	    pip->mp->mt.fi_meta_timeo, flags, &ip, credp)) == 0) {
		*vpp = SAM_ITOV(ip);
		if (((ip->di.rm.ui.flags & RM_CHAR_DEV_FILE) &&
		    S_ISREG(ip->di.mode)) || SAM_ISVDEV((*vpp)->v_type)) {
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			/*
			 * Repeat the test under lock.
			 * Don't need the overhead of always grabbing
			 * inode_rwl, RW_WRITER for a rare condition.
			 *
			 */
			if (((ip->di.rm.ui.flags & RM_CHAR_DEV_FILE) &&
			    S_ISREG(ip->di.mode)) ||
			    SAM_ISVDEV((*vpp)->v_type)) {
				vnode_t *vp;

				error = sam_get_special_file(ip, &vp, credp);

				if (error == 0) {
					*vpp = vp;
				} else {
					*vpp = NULL;
					TRACE(T_SAM_CL_LOOKUP_ERR,
					    pvp, pip->di.id.ino, error, 0);
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
					goto out;
				}
			}
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		if (flags & LOOKUP_XATTR) {
			(*vpp)->v_flag |= V_XATTRDIR;
		}
		TRACE(T_SAM_CL_LOOKUP_RET, pvp, (sam_tr_t)*vpp, ip->di.id.ino,
		    ip->di.nlink);
	} else {
		*vpp = NULL;
		TRACE(T_SAM_CL_LOOKUP_ERR, pvp, pip->di.id.ino, error, 0);
	}
out:
	SAM_CLOSE_OPERATION(pip->mp, error);
	return (error);
}


/*
 * ----- sam_client_create_vn - Create a SAM-QFS file.
 *
 *	Check permissions and if parent directory is valid,
 *	then create entry in the directory and set appropriate modes.
 *	If created, the vnode is "held". The parent is "unheld".
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_create_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* pointer to the component name to create. */
	vattr_t *vap,		/* vattr for type & mode information. */
	vcexcl_t ex,		/* exclusive create flag. */
	int mode,		/* file mode information. */
	vnode_t **vpp,		/* pointer pointer to returned vnode. */
	cred_t *credp,		/* credentials pointer. */
	int flag,		/* large file flag. */
	caller_context_t *ct,	/* caller context pointer. */
	vsecattr_t *vsap	/* security attributes pointer. */
#else
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* pointer to the component name to create. */
	vattr_t *vap,		/* vattr for type & mode information. */
	vcexcl_t ex,		/* exclusive create flag. */
	int mode,		/* file mode information. */
	vnode_t **vpp,		/* pointer pointer to returned vnode. */
	cred_t *credp,		/* credentials pointer. */
	int flag		/* large file flag. */
#endif
)
{
	sam_node_t *pip;
	sam_name_create_t create;
	sam_ino_record_t *nrec;
	sam_ino_t inumber = 0;
	int error = 0;

	TRACES(T_SAM_CL_CREATE, pvp, cp);
	pip = SAM_VTOI(pvp);

	/*
	 * Shared file system does not support pipes and
	 * block/char. special files. Note, the major/minor
	 * number may not be the same on all hosts.
	 */
	if ((vap->va_type == VCHR &&
	    !(pip->di.rm.ui.flags & RM_CHAR_DEV_FILE)) ||
	    (vap->va_type == VBLK) || (vap->va_type == VFIFO)) {
		return (ENOTSUP);
	}

	if ((error = sam_open_operation(pip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

#ifdef METADATA_SERVER
	if (SAM_IS_SHARED_SERVER(pip->mp)) {
#if defined(SOL_511_ABOVE)
		error = sam_create_vn(pvp, cp, vap, ex, mode, vpp, credp, flag,
		    ct, vsap);
#else
		error = sam_create_vn(pvp, cp, vap, ex, mode, vpp, credp, flag);
#endif
		goto out;
	}
#endif

	/*
	 * Cannot set sticky bit unless superuser.
	 */
	if ((vap->va_mode & VSVTX) && secpolicy_vnode_stky_modify(credp)) {
		vap->va_mode &= ~VSVTX;
	}

	/*
	 * Empty pathname refers to this directory.
	 */
	if (*cp == '\0') {
		if (ex == EXCL) {
			error = EEXIST;
		} else if (mode & S_IWRITE) {
			error = EISDIR;
		} else if (SAM_PRIVILEGE_INO(pip->di.version,
		    pip->di.id.ino)) {
			error = EPERM;
		} else {
			error = sam_access_ino(pip, mode, FALSE, credp);
			if (error == 0) {
				VN_HOLD(pvp);
				*vpp = pvp;
			}
		}
		goto out;
	}

	/*
	 * Ask the server if file exists. If not, create it on the server.
	 * If we need to truncate the file, do so after creating it.
	 * This avoids some complex ordering issues associated with trying
	 * to do the truncate on the server (possibly while the file is
	 * in use through another file descriptor on this client).
	 */

	*vpp = NULL;
	create.ex = (int32_t)ex;
	create.mode = mode;
	create.flag = flag;
	if ((error = sam_v_to_v32(vap, &create.vattr)) != 0) {
		goto out;
	}
	create.vattr.va_mask &= ~AT_SIZE;
	nrec = kmem_alloc(sizeof (*nrec), KM_SLEEP);

	error = sam_proc_name(pip, NAME_create, &create,
	    sizeof (create), cp, NULL, credp, nrec);
	if (error == 0) {
		sam_node_t *ip;

		if ((error = sam_get_client_ino(nrec, pip,
		    cp, &ip, credp)) == 0) {
			vnode_t *vp;

			vp = SAM_ITOV(ip);

			/*
			 * Truncate the inode, if requested; otherwise,
			 * stale its extents and pages.  Note, if file
			 * is already empty, no need to truncate it.
			 *
			 * XXX - I don't think we should have to stale
			 *	 anything here. Shouldn't that happen somewhere
			 *	 else?  We do call sam_directed_actions within
			 *	 sam_get_client_ino....
			 */

			sam_rwdlock_ino(ip, RW_WRITER, 0);
			sam_open_mutex_operation(ip, &ip->cl_apmutex);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

			if (S_ISREG(ip->di.mode) && (vap->va_mask & AT_SIZE) &&
			    (vap->va_size == 0) && (ip->di.rm.size != 0)) {
				sam_lease_data_t data;

				bzero(&data, sizeof (data));

				if (vn_has_cached_data(vp)) {
					sam_flush_pages(ip, B_INVAL);
				}
				data.ltype = LTYPE_truncate;
				data.lflag = SAM_TRUNCATE;
				data.sparse = SPARSE_none;
				data.offset = 0;
				data.resid = 0;
				data.alloc_unit = ip->cl_alloc_unit;
				data.filemode = flag;
				data.cmd = 0;
				data.shflags.b.directio = ip->flags.b.directio;
				data.shflags.b.abr = ip->flags.b.abr;
				error = sam_truncate_shared_ino(ip,
				    &data, credp);
			} else {
				sam_clear_map_cache(ip);
				(void) sam_stale_indirect_blocks(ip, 0);
				(void) VOP_PUTPAGE_OS(vp, 0, 0, B_INVAL, credp,
				    NULL);
			}
			if (error == 0) {
				VNEVENT_CREATE_OS(SAM_ITOV(ip), NULL);
			}

			mutex_exit(&ip->cl_apmutex);
			RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);

			*vpp = vp;
			inumber = ip->di.id.ino;
			if (((ip->di.rm.ui.flags & RM_CHAR_DEV_FILE) &&
			    S_ISREG(ip->di.mode)) ||
			    SAM_ISVDEV((*vpp)->v_type)) {

				vnode_t *vp;

				error = sam_get_special_file(ip, &vp, credp);
				if (error == 0) {
					*vpp = vp;
				} else {
					*vpp = NULL;
				}
			}
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
	}
	kmem_free(nrec, sizeof (*nrec));

out:
	SAM_CLOSE_OPERATION(pip->mp, error);
	TRACE(T_SAM_CL_CREATE_RET, pvp, (sam_tr_t)*vpp, inumber, error);
	return (error);
}


/*
 * ----- sam_client_remove_vn - Remove a SAM-QFS file.
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_remove_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *pvp,		/* Pointer to parent directory vnode. */
	char *cp,		/* Pointer to the component name to remove. */
	cred_t *credp,		/* Credentials pointer. */
	caller_context_t *ct,	/* Caller context pointer. */
	int flag		/* File flags. */
#else
	vnode_t *pvp,		/* Pointer to parent directory vnode. */
	char *cp,		/* Pointer to the component name to remove. */
	cred_t *credp		/* Credentials pointer. */
#endif
)
{
	sam_node_t *pip;
	sam_node_t *ip;
	sam_name_remove_t remove;
	int error;

	TRACES(T_SAM_CL_REMOVE, pvp, cp);
	pip = SAM_VTOI(pvp);
	if ((error = sam_open_operation(pip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

#ifdef METADATA_SERVER
	if (SAM_IS_SHARED_SERVER(pip->mp)) {
#if defined(SOL_511_ABOVE)
		error = sam_remove_vn(pvp, cp, credp, ct, flag);
#else
		error = sam_remove_vn(pvp, cp, credp);
#endif
		goto out;
	}
#endif

	remove.id.ino = 0;
	if ((error = sam_client_lookup_name(pip, cp, 0, 0, &ip, credp)) == 0) {
		sam_ino_record_t *nrec;
		vnode_t *rmvp;

		rmvp = SAM_ITOV(ip);
		if ((rmvp->v_type == VDIR) &&
		    secpolicy_fs_linkdir(credp, rmvp->v_vfsp)) {
			VN_RELE(rmvp);
			error = EPERM;
			goto out;
		}
		if (rmvp->v_count > 1) {
			dnlc_remove(pvp, cp);
			if (SAM_THREAD_IS_NFS()) {
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				if (ip->cl_leases &&
				    !(ip->cl_leases & CL_OPEN)) {
					ushort_t leasemask = 0;

					/*
					 * Leases that are not in use, whether
					 * we hold it or not.
					 */
					leasemask |=
					    ip->cl_leaseused[LTYPE_read]
					    ? 0 : CL_READ;
					leasemask |=
					    ip->cl_leaseused[LTYPE_write]
					    ? 0 : CL_WRITE;
					leasemask |=
					    ip->cl_leaseused[LTYPE_append]
					    ? 0 : CL_APPEND;

					/*
					 * Expirable leases that we hold.
					 */
					leasemask &= ip->cl_leases;

					if (leasemask) {
						/*
						 * Remove all expirable leases
						 * that are not in use.
						 */
						error = sam_proc_rm_lease(ip,
						    leasemask, RW_WRITER);
					}
				}
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			}
		}

		/*
		 * File exists, ask server to remove it.
		 * Lift lock and resolve conflicts on the server.
		 * Remove this reference from the name cache.
		 */
		remove.id = ip->di.id;
		nrec = kmem_alloc(sizeof (*nrec), KM_SLEEP);
		if ((error = sam_proc_name(pip, NAME_remove,
		    &remove, sizeof (remove), cp,
		    NULL, credp, nrec)) == 0) {

			VNEVENT_REMOVE_OS(rmvp, pvp, cp, NULL);

			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			error = sam_reset_client_ino(ip, 0, nrec);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		kmem_free(nrec, sizeof (*nrec));
		VN_RELE(SAM_ITOV(ip));  /* Decrement vnode count */
	}

out:
	SAM_CLOSE_OPERATION(pip->mp, error);
	TRACE(T_SAM_CL_REMOVE_RET, pvp, pip->di.id.ino, remove.id.ino, error);
	return (error);
}


/*
 * ----- sam_client_link_vn - Hard link a SAM-QFS file.
 *	Add a hard link to an existing shared file.
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_link_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *pvp,		/* parent directory where link is targeted. */
	vnode_t *vp,		/* existing vnode. */
	char *cp,		/* component name for the link. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct,	/* caller context pointer. */
	int flag		/* file flags. */
#else
	vnode_t *pvp,		/* parent directory where link is targeted. */
	vnode_t *vp,		/* existing vnode. */
	char *cp,		/* component name for the link. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	sam_node_t *pip;
	vnode_t *realvp;
	sam_name_link_t link;
	sam_node_t *ip;
	sam_ino_record_t *nrec;
	sam_ino_t inumber = 0;
	int error = 0;

	if (VOP_REALVP_OS(vp, &realvp, NULL) == 0) {
		vp = realvp;
	}
	pip = SAM_VTOI(pvp);
	if ((vp->v_type == VDIR) &&
	    secpolicy_fs_linkdir(credp, vp->v_vfsp)) {
		return (EPERM);
	}

	if ((error = sam_open_operation(pip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

#ifdef METADATA_SERVER
	if (SAM_IS_SHARED_SERVER(pip->mp)) {
#if defined(SOL_511_ABOVE)
		error = sam_link_vn(pvp, vp, cp, credp, ct, flag);
#else
		error = sam_link_vn(pvp, vp, cp, credp);
#endif
		goto out;
	}
#endif

	TRACES(T_SAM_CL_LINK, pvp, cp);
	/*
	 * Remove this reference from the name cache.
	 * Lift lock and resolve conflicts on the server.
	 */
	dnlc_remove(pvp, cp);
	ip = SAM_VTOI(vp);
	link.id = ip->di.id;
	inumber = 0;
	nrec = kmem_alloc(sizeof (*nrec), KM_SLEEP);
	if ((error = sam_proc_name(pip, NAME_link, &link, sizeof (link), cp,
	    NULL, credp, nrec)) == 0) {

		error = sam_get_client_ino(nrec, pip, cp, &ip, credp);
		if (error == 0) {
			inumber = ip->di.id.ino;
			VNEVENT_LINK_OS(vp, NULL);
			VN_RELE(SAM_ITOV(ip));
		}
	}
	kmem_free(nrec, sizeof (*nrec));
out:

	SAM_CLOSE_OPERATION(pip->mp, error);
	TRACE(T_SAM_CL_LINK_RET, pvp, (sam_tr_t)vp, inumber, error);
	return (error);
}


/*
 * ----- sam_client_rename_vn - Rename a SAM-QFS shared file.
 *
 *	Rename a SAM-QFS shared file: Verify the old file exists.  Remove
 *	the new file if it exists; hard link the new file to the old file;
 *	remove the old file.
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_rename_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *opvp,		/* Pointer to the old file parent vnode. */
	char *onm,		/* Pointer to old file name. */
	vnode_t *npvp,		/* Pointer to the new file parent vnode. */
	char *nnm,		/* Pointer to new file name. */
	cred_t *credp,		/* Credentials pointer. */
	caller_context_t *ct,	/* Caller context pointer. */
	int flag		/* File flags. */
#else
	vnode_t *opvp,		/* Pointer to the old file parent vnode. */
	char *onm,		/* Pointer to old file name. */
	vnode_t *npvp,		/* Pointer to the new file parent vnode. */
	char *nnm,		/* Pointer to new file name. */
	cred_t *credp		/* Credentials pointer. */
#endif
)
{
	vnode_t *realvp;
	sam_name_rename_t rename;
	sam_node_t *opip, *npip;
	sam_node_t *oip, *nip;
	sam_ino_record_t *nrec;
	sam_ino_t inumber = 0;
	int error;

	if (VOP_REALVP_OS(npvp, &realvp, NULL) == 0) {
		npvp = realvp;
	}
	opip = SAM_VTOI(opvp);
	npip = SAM_VTOI(npvp);
	/*
	 * Not allowed to rename either ".", or "..",
	 * nor rename to either ".", or "..".
	 */
	if (IS_DOT_OR_DOTDOT(onm)) {
		return (EINVAL);
	}
	if (IS_DOT_OR_DOTDOT(nnm)) {
		return (EINVAL);
	}
	TRACES(T_SAM_CL_RENAME1, opvp, onm);
	TRACES(T_SAM_CL_RENAME2, npvp, nnm);

	if ((error = sam_open_operation(opip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

#ifdef METADATA_SERVER
	if (SAM_IS_SHARED_SERVER(opip->mp)) {
#if defined(SOL_511_ABOVE)
		error = sam_rename_vn(opvp, onm, npvp, nnm, credp, ct, flag);
#else
		error = sam_rename_vn(opvp, onm, npvp, nnm, credp);
#endif
		goto out;
	}
#endif

	/*
	 * Old file must exist.
	 */
	if ((error = sam_client_lookup_name(opip, onm, 0, 0, &oip, credp))) {
		goto out;
	}
	rename.oid = oip->di.id;

	/*
	 * New file may exist.
	 */
	error = sam_client_lookup_name(npip, nnm, 0, 0, &nip, credp);
	if (error == 0) {
		rename.nid = nip->di.id;
	} else {
		nip = NULL;
		rename.nid.ino = 0;
	}

	rename.osize = strlen(onm);
	rename.nsize = strlen(nnm);
	rename.new_parent_id = npip->di.id;

	inumber = 0;
	nrec = kmem_alloc(sizeof (*nrec), KM_SLEEP);
	if ((error = sam_proc_name(opip, NAME_rename, &rename, sizeof (rename),
	    onm, nnm, credp, nrec)) == 0) {
		sam_node_t *ip;

		/*
		 * Remove DNLC entries for renamed file.  If a directory,
		 * remove DNLC entry for parent as well.
		 */

		dnlc_remove(opvp, onm);
		dnlc_remove(npvp, nnm);
		if (S_ISDIR(oip->di.mode)) {
			dnlc_remove(SAM_ITOV(oip), "..");
		}

		error = sam_get_client_ino(nrec, npip, nnm, &ip, credp);

		VNEVENT_RENAME_SRC_OS(SAM_ITOV(oip), opvp, onm, NULL);

		VN_RELE(SAM_ITOV(oip));  /* Decrement vnode count */

		if (opip->di.id.ino != npip->di.id.ino) {
			(void) sam_refresh_client_ino(npip, -1, credp);
		}

		if (nip) {
			VNEVENT_RENAME_DEST_OS(SAM_ITOV(nip), npvp, nnm, NULL);

			VN_RELE(SAM_ITOV(nip));	/* Decrement vnode count */
		}

		/*
		 * Notify the target directory of the rename event
		 * if source and target directories are not same.
		 */
		if (opvp != npvp) {
			VNEVENT_RENAME_DEST_DIR_OS(npvp, NULL);
		}

		if (error == 0) {
			inumber = ip->di.id.ino;
			VN_RELE(SAM_ITOV(ip)); /* Dec count on renamed file */
		}
	} else {
		VN_RELE(SAM_ITOV(oip));		/* Dec old vnode count */
		if (nip) {
			VN_RELE(SAM_ITOV(nip));	/* Dec new vnode count */
		}
	}
	kmem_free(nrec, sizeof (*nrec));

out:
	SAM_CLOSE_OPERATION(opip->mp, error);
	TRACE(T_SAM_CL_RENAME_RET,
	    SAM_ITOV(npip), (sam_tr_t)oip, inumber, error);
	return (error);
}


/*
 * ----- sam_client_mkdir_vn - Make a SAM-QFS shared directory.
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_mkdir_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* component directory name to create. */
	vattr_t *vap,		/* vnode attributes pointer. */
	vnode_t **vpp,		/* pointer pointer to returned vnode. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct,	/* caller context pointer. */
	int flag,		/* file flags. */
	vsecattr_t *vsap	/* security attributes pointer. */
#else
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* component directory name to create. */
	vattr_t *vap,		/* vnode attributes pointer. */
	vnode_t **vpp,		/* pointer pointer to returned vnode. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	sam_node_t *pip;
	sam_node_t *ip;
	sam_name_mkdir_t mkdir;
	sam_ino_record_t *nrec;
	int error;

	TRACES(T_SAM_CL_MKDIR, pvp, cp);
	pip = SAM_VTOI(pvp);
	if ((error = sam_open_operation(pip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

#ifdef METADATA_SERVER
	if (SAM_IS_SHARED_SERVER(pip->mp)) {
#if defined(SOL_511_ABOVE)
		error = sam_mkdir_vn(pvp, cp, vap, vpp, credp, ct, flag, vsap);
#else
		error = sam_mkdir_vn(pvp, cp, vap, vpp, credp);
#endif
		goto out;
	}
#endif

	/* Only superuser can set sticky bit */
	if ((vap->va_mode & VSVTX) && secpolicy_vnode_stky_modify(credp)) {
		vap->va_mode &= ~VSVTX;
	}

	/*
	 * Ask the server if directory exists. If not, create it on the server.
	 */

	if ((error = sam_v_to_v32(vap, &mkdir.vattr)) != 0) {
		goto out;
	}
	nrec = kmem_alloc(sizeof (*nrec), KM_SLEEP);
	if ((error = sam_proc_name(pip, NAME_mkdir, &mkdir,
	    sizeof (mkdir), cp, NULL, credp, nrec)) == 0) {

		if ((error = sam_get_client_ino(nrec, pip,
		    cp, &ip, credp)) == 0) {

			*vpp = SAM_ITOV(ip);
			TRACE(T_SAM_CL_MKDIR_RET, pvp,
			    (sam_tr_t)* vpp, ip->di.id.ino, ip->di.rm.size);
		}
	}
	kmem_free(nrec, sizeof (*nrec));
	if (error) {
		*vpp = NULL;
		TRACE(T_SAM_CL_MKDIR_ERR, pvp, (sam_tr_t)* vpp, 0, error);
	}

out:
	SAM_CLOSE_OPERATION(pip->mp, error);
	return (error);
}


/*
 * ----- sam_client_rmdir_vn - Remove a SAM-QFS shared directory.
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_rmdir_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* pointer to the component name to remove. */
	vnode_t *cdir,		/* pointer to vnode. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct,	/* Caller context pointer. */
	int flag		/* File flags. */
#else
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* pointer to the component name to remove. */
	vnode_t *cdir,		/* pointer to vnode. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	int error;
	sam_node_t *pip;
	sam_node_t *ip;
	sam_name_rmdir_t rmdir;

	TRACES(T_SAM_CL_RMDIR, pvp, cp);
	pip = SAM_VTOI(pvp);
	if ((error = sam_open_operation(pip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

#ifdef METADATA_SERVER
	if (SAM_IS_SHARED_SERVER(pip->mp)) {
#if defined(SOL_511_ABOVE)
		error = sam_rmdir_vn(pvp, cp, cdir, credp, ct, flag);
#else
		error = sam_rmdir_vn(pvp, cp, cdir, credp);
#endif
		goto out;
	}
#endif

	rmdir.id.ino = 0;
	if ((error = sam_client_lookup_name(pip, cp, 0, 0, &ip, credp)) == 0) {
		sam_ino_record_t *nrec;
		vnode_t *rmvp;

		/* check for current directory */
		rmvp = SAM_ITOV(ip);
		if (rmvp == PTOU(curproc)->u_cdir) {
			VN_RELE(rmvp);			/* release hold */
			error = EINVAL;
			goto out;
		}
		/*
		 * Directory exists, ask server to remove it.
		 * Lift lock and resolve conflicts on the server.
		 * Remove this reference from the name cache.
		 */
		rmdir.id = ip->di.id;
		nrec = kmem_alloc(sizeof (*nrec), KM_SLEEP);
		if ((error = sam_proc_name(pip, NAME_rmdir,
		    &rmdir, sizeof (rmdir), cp,
		    NULL, credp, nrec)) == 0) {

			VNEVENT_RMDIR_OS(rmvp, pvp, cp, NULL);

			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			error = sam_reset_client_ino(ip, 0, nrec);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			if (rmvp->v_count > 1) {
				dnlc_purge_vp(rmvp);
			}
		}
		VN_RELE(SAM_ITOV(ip));	/* Decrement vnode count */
		kmem_free(nrec, sizeof (*nrec));
	}

out:
	SAM_CLOSE_OPERATION(pip->mp, error);
	TRACE(T_SAM_CL_RMDIR_RET, pvp, pip->di.id.ino, rmdir.id.ino, error);
	return (error);
}


/*
 * ----- sam_client_readdir_vn - Return a SAM-QFS directory.
 *
 *	Refresh the directory if the last refresh was older than fi_meta_timeo.
 *	Read the directory and return the entries in the
 *	file system independent format. NOTE, inode data RWLOCK held.
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_readdir_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* directory vnode to be returned. */
	uio_t *uiop,		/* user I/O vector array. */
	cred_t *credp,		/* credentials pointer */
	int *eofp,		/* If not NULL, return eof flag . */
	caller_context_t *ct,	/* Caller context pointer. */
	int flag		/* File flags. */
#else
	vnode_t *vp,		/* directory vnode to be returned. */
	uio_t *uiop,		/* user I/O vector array. */
	cred_t *credp,		/* credentials pointer */
	int *eofp		/* If not NULL, return eof flag . */
#endif
)
{
	sam_node_t *ip;
	int error;

	TRACE(T_SAM_CL_READDIR, vp, (sam_tr_t)uiop->uio_offset,
	    (uint_t)uiop->uio_iov->iov_len, (uint_t)credp);
	ip = SAM_VTOI(vp);
	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

	if (!SAM_IS_SHARED_SERVER(ip->mp)) {
		error = sam_refresh_client_ino(ip,
		    ip->mp->mt.fi_meta_timeo, credp);
		if (error) {
			goto out;
		}
	}
#if defined(SOL_511_ABOVE)
	if ((error = sam_readdir_vn(vp, uiop, credp, eofp, ct, flag)) == 0) {
#else
	if ((error = sam_readdir_vn(vp, uiop, credp, eofp)) == 0) {
#endif
		ip->nfs_ios++;		/* Defer inactivate */
	} else if (error == EDOM) {
		sam_directed_actions(ip,
		    (SR_INVAL_PAGES | SR_STALE_INDIRECT), 0, credp);
#if defined(SOL_511_ABOVE)
		if ((error = sam_readdir_vn(vp, uiop, credp, eofp, ct, flag))
		    == 0) {
#else
		if ((error = sam_readdir_vn(vp, uiop, credp, eofp)) == 0) {
#endif
			ip->nfs_ios++;		/* Defer inactivate */
		}
	}

out:
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


/*
 * ----- sam_client_symlink_vn - Make a SAM-QFS symbolic link.
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_symlink_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* pointer to the name of the symbolic link. */
	vattr_t *vap,		/* vattr pointer for the symbolic link. */
	char *tnm,		/* pointer to the symbolic link pathname. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct,	/* caller context pointer. */
	int flag		/* file flags. */
#else
	vnode_t *pvp,		/* pointer to parent directory vnode. */
	char *cp,		/* pointer to the name of the symbolic link. */
	vattr_t *vap,		/* vattr pointer for the symbolic link. */
	char *tnm,		/* pointer to the symbolic link pathname. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	int error;
	sam_node_t *pip;
	sam_node_t *ip;
	ino_t ino = 0;

	TRACES(T_SAM_CL_SYMLINK, pvp, cp);
	pip = SAM_VTOI(pvp);
	if ((error = sam_open_operation(pip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

#ifdef METADATA_SERVER
	if (SAM_IS_SHARED_SERVER(pip->mp)) {
#if defined(SOL_511_ABOVE)
		error = sam_symlink_vn(pvp, cp, vap, tnm, credp, ct, flag);
#else
		error = sam_symlink_vn(pvp, cp, vap, tnm, credp);
#endif
		goto out;
	}
#endif

	if ((error = sam_client_lookup_name(pip, cp,
	    0, 0, &ip, credp)) == ENOENT) {

		sam_name_symlink_t symlink;
		sam_ino_record_t *nrec;

		if ((error = sam_v_to_v32(vap, &symlink.vattr)) != 0) {
			goto out;
		}
		/*
		 * File does not exist, create it on the server.
		 */
		symlink.comp_size = strlen(cp);
		symlink.path_size = strlen(tnm);
		nrec = kmem_alloc(sizeof (*nrec), KM_SLEEP);
		sam_rwdlock_ino(pip, RW_WRITER, 0);
		if ((error = sam_proc_name(pip, NAME_symlink,
		    &symlink, sizeof (symlink), cp, tnm,
		    credp, nrec)) == 0) {

			RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
			if ((error = sam_get_client_ino(nrec, pip,
			    cp, &ip, credp)) == 0) {
				SAM_ITOV(ip)->v_type = VLNK; /* set as a link */
				VN_RELE(SAM_ITOV(ip));
			}
		} else {
			RW_UNLOCK_OS(&pip->data_rwl, RW_WRITER);
		}
		kmem_free(nrec, sizeof (*nrec));
	} else if (error == 0) {	/* If entry already exists. */
		error = EEXIST;
		VN_RELE(SAM_ITOV(ip)); /* Decrement v_count if already exists */
	}

out:
	SAM_CLOSE_OPERATION(pip->mp, error);
	TRACE(T_SAM_CL_SYMLN_RET, pvp, ino, error, 0);
	return (error);
}


/*
 * ----- sam_client_readlink_vn - Read a SAM-QFS symbolic link.
 *	Return the shared symbolic link contents.
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_readlink_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	struct uio *uiop,	/* User I/O vector array. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	struct uio *uiop,	/* User I/O vector array. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	sam_node_t *ip;
	int error;

	ip = SAM_VTOI(vp);
	TRACE(T_SAM_CL_READLINK, vp, ip->di.id.ino,
	    (sam_tr_t)uiop->uio_offset, (sam_tr_t)uiop->uio_resid);

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

	if (!SAM_IS_SHARED_SERVER(ip->mp)) {
		error = sam_refresh_client_ino(ip,
		    ip->mp->mt.fi_meta_timeo, credp);
		if (error) {
			goto out;
		}
	}
#if defined(SOL_511_ABOVE)
	error = sam_readlink_vn(vp, uiop, credp, ct);
#else
	error = sam_readlink_vn(vp, uiop, credp);
#endif

out:
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


/*
 * ----- sam_client_fsync_vn - Flush this vn to disk.
 * Synchronize the vnode's in-memory state on disk.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_client_fsync_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	int filemode,		/* open filemode. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	int filemode,		/* open filemode. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	sam_node_t *ip;
	int error = 0;

	ip = SAM_VTOI(vp);
	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}
#if defined(SOL_511_ABOVE)
	error = sam_fsync_vn(vp, filemode, credp, ct);
#else
	error = sam_fsync_vn(vp, filemode, credp);
#endif
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


/*
 * ----- sam_client_frlock_vn - File and record locking.
 *	Ask the server host to lock the file or record.
 */

/* ARGSUSED7 */
int				/* ERRNO if error occurred, 0 if successful. */
sam_client_frlock_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,			/* Pointer to vnode. */
	int cmd,			/* Command. */
	sam_flock_t *flp,		/* Pointer to flock */
	int filemode,			/* filemode flags, see file.h */
	offset_t offset,		/* offset. */
	struct flk_callback *fcb,	/* extra callback param */
	cred_t *credp,			/* credentials pointer. */
	caller_context_t *ct		/* caller context pointer. */
#else
	vnode_t *vp,			/* Pointer to vnode. */
	int cmd,			/* Command. */
	sam_flock_t *flp,		/* Pointer to flock */
	int filemode,			/* filemode flags, see file.h */
	offset_t offset,		/* offset. */
	struct flk_callback *fcb,	/* extra callback param */
	cred_t *credp			/* credentials pointer. */
#endif
)
{
	int error;
	u_offset_t start, end;
	sam_node_t *ip;
	void *lmfcb = NULL;

	TRACE(T_SAM_CL_FRLOCK, vp, cmd, flp->l_type, offset);
	ip = SAM_VTOI(vp);

	if (MANDLOCK(vp, ip->di.mode)) {
		return (EACCES);
	}

	/*
	 * Check for supported parameters & verify flock l_type.
	 */
	if (!(cmd == F_GETLK || cmd == F_SETLK || cmd == F_SETLKW ||
	    cmd == F_HASREMOTELOCKS)) {
		return (EINVAL);
	}

	if (cmd != F_HASREMOTELOCKS) {
		switch (flp->l_type) {

		case F_RDLCK:
			if (cmd != F_GETLK && !(filemode & FREAD)) {
				return (EBADF);
			}
			break;

		case F_WRLCK:
			if (cmd != F_GETLK && !(filemode & FWRITE)) {
				return (EBADF);
			}
			break;

		case F_UNLCK:
			break;

		default:
			return (EINVAL);
		}
	}

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

	/*
	 * Check validity of the lock range.
	 * If SEEK_END and append lease, set start without VOP_GETATTR().
	 */

	if (cmd != F_HASREMOTELOCKS) {
		if ((flp->l_whence == SEEK_END) &&
		    (ip->cl_leases & CL_APPEND)) {

			start = (flp->l_start + ip->di.rm.size);
		} else {
			if (error = flk_convert_lock_data(vp, flp,
			    &start, &end, offset)) {
				goto out;
			}
		}
		if (error = flk_check_lock_data(start, end, MAXEND)) {
			goto out;
		}

		flp->l_whence = 0; /* Start set based on input l_whence */
		flp->l_start = start;
	}

	lmfcb = (void *)fcb;

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	error = sam_client_frlock_ino(ip, cmd, flp,
	    filemode, offset, lmfcb, credp);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

out:
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


/*
 * ----- sam_client_space_vn - Truncate/expand storage space.
 *
 *	For cmd F_FREESP, truncate/expand space.
 *  l_whence indicates where the relative offset is measured:
 *    0 - the start of the file.
 *    1 - the current position.
 *    2 - the end of the file.
 *  l_start is the offset from the position specified in l_whence.
 *  l_len is the size to be freed. l_len = 0 frees up to the end of the file.
 */

/* ARGSUSED6 */
int				/* ERRNO if error occurred, 0 if successful. */
sam_client_space_vn(
	vnode_t *vp,		/* vnode entry */
	int cmd,		/* command */
	sam_flock_t *flp,	/* flock, used if mandatory lock set */
	int filemode,		/* filemode flags, see file.h */
	offset_t offset,	/* offset */
	cred_t *credp,		/* credentials */
	caller_context_t *ct)	/* caller context pointer. */
{
	sam_node_t *ip;
	int error;

	TRACE(T_SAM_CL_SPACE, vp, 0, flp->l_start, flp->l_len);
	ip = SAM_VTOI(vp);
	if (S_ISREQ(ip->di.mode)) {
		return (EINVAL); /* Cannot truncate a removable media file */
	}
	if (SAM_PRIVILEGE_INO(ip->di.version, ip->di.id.ino)) {
		return (EPERM);		/* Cannot truncate privileged inodes */
	}

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

	if (cmd == F_FREESP) {
		if ((error = convoff(vp, flp, 0, (sam_offset_t)offset)) == 0) {
			if (flp->l_len != 0) {
				error = EINVAL;
			} else {
				sam_lease_data_t data;

				/*
				 * Truncate is done on the server.
				 */
				bzero(&data, sizeof (data));

				sam_rwdlock_ino(ip, RW_WRITER, 0);
				sam_open_mutex_operation(ip, &ip->cl_apmutex);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				if (vn_has_cached_data(vp)) {
					sam_flush_pages(ip, 0);
				}
				data.ltype = LTYPE_truncate;
				data.lflag = SAM_TRUNCATE;
				data.sparse = SPARSE_none;
				data.offset = 0;
				data.resid = flp->l_start;
				data.alloc_unit = ip->cl_alloc_unit;
				data.filemode = filemode;
				data.cmd = 0;
				data.shflags.b.directio = ip->flags.b.directio;
				data.shflags.b.abr = ip->flags.b.abr;
				error = sam_truncate_shared_ino(ip,
				    &data, credp);

				if (ip->size < ip->zero_end) {
					ip->zero_end = 0;
				}
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				mutex_exit(&ip->cl_apmutex);
				RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
			}
		}
	} else {
		error = EINVAL;
	}
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


/*
 * ----- sam_client_getpage_vn - Get pages for a SAM-QFS file.
 *
 * Get pages for a file for use in a memory mapped function.
 * Return the page list with all the pages for the file from
 * offset to (offset + length).
 */

static int			/* ERRNO if error, 0 if successful. */
sam_client_getpage_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* vnode entry */
	offset_t offset,	/* file offset */
	sam_size_t length,	/* file length provided to mmap */
	uint_t *protp,		/* requested protection <sys/mman.h> */
	page_t **pglist,	/* page list array (returned) */
	sam_size_t plsize,	/* max byte size in page list array */
	struct seg *segp,	/* page segment */
	caddr_t addr,		/* page address */
	enum seg_rw rw,		/* access type for the fault operation */
	cred_t *credp,		/* credentials */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* vnode entry */
	offset_t offset,	/* file offset */
	sam_size_t length,	/* file length provided to mmap */
	uint_t *protp,		/* requested protection <sys/mman.h> */
	page_t **pglist,	/* page list array (returned) */
	sam_size_t plsize,	/* max byte size in page list array */
	struct seg *segp,	/* page segment */
	caddr_t addr,		/* page address */
	enum seg_rw rw,		/* access type for the fault operation */
	cred_t *credp		/* credentials */
#endif
)
{
	sam_node_t *ip, *l_ip;
	boolean_t lock_set = B_FALSE;
	sam_block_fgetbuf_t fgetbuf;
	page_t **pppl;
	page_t *pp;
	offset_t off;
	sam_size_t len = length;
	sam_size_t plsz = plsize;
	int error = 0;
	uint32_t ltype;
	boolean_t using_lease = FALSE;
	int unset_nb = 1;

	TRACE(T_SAM_CL_GETPAGE, vp, (sam_tr_t)offset, length, rw);
	if (vp->v_flag & VNOMAP) {
		return (ENOSYS);
	}
	ip = SAM_VTOI(vp);
	if (rw == S_WRITE) {
		ltype = LTYPE_write;
	} else {
		ltype = LTYPE_read;
	}

	/*
	 * In the mmap path, we may have lost the read-lease or
	 * write-lease so check that we have it (under ilease_mutex
	 * protection) and if needed, re-acquire it.
	 *
	 * Code paths not needing leases need to call SAM_SET_LEASEFLG
	 * to avoid acquiring it here.
	 */
	if (vp->v_type == VDIR) {
		goto start;
	}

	/*
	 * If this thread is just now entering the filesystem (an mmap
	 * page fault), then we need to mark it non-blocking.  If the
	 * filesystem is idling (maybe for failover), then this will
	 * return an error.
	 */
	if (sam_open_operation_nb_unless_idle(ip)) {
		return (EDEADLK);
	}

	/*
	 * Threads which came here via other parts
	 * of the filesystem cannot be nonblocking.
	 */
	if (SAM_GET_LEASEFLG(ip)) {
		unset_nb = 0;
		sam_unset_operation_nb(ip->mp);
	}

	using_lease = TRUE;
	mutex_enter(&ip->ilease_mutex);
	ip->cl_leaseused[ltype]++;
	l_ip = SAM_GET_LEASEFLG(ip);

	/*
	 * If l_ip is null then we're doing an mmap page fault
	 * and this host may no longer have a lease for this file.
	 *
	 * If l_ip is non-null but doesn't match ip, then we've come
	 * through VOP_WRITE and we're trying to read from ip because
	 * the buffer pointed to by the uio struct is backed by this file.
	 *
	 * Either way, we've come through a code path that is not
	 * necessarily actively holding a lease for this particular file.
	 */
#ifdef DEBUG
	if (l_ip == NULL) {
		ASSERT(ip->mm_pages > 0);
		ASSERT(ip->last_unmap == 0);
		ASSERT(ip->cl_leases & CL_MMAP);
	}
	if (l_ip && l_ip != ip) {
		ASSERT(rw == S_READ);
		ASSERT(ip->mm_pages > 0);
		ASSERT(ip->last_unmap == 0);
		ASSERT(ip->cl_leases & CL_MMAP);
	}
#endif

	if ((l_ip != ip) &&
	    (((rw == S_WRITE) && !(ip->cl_leases & CL_WRITE)) ||
	    ((rw == S_READ) && !(ip->cl_leases & CL_READ)))) {
		sam_lease_data_t data;

		mutex_exit(&ip->ilease_mutex);
		bzero(&data, sizeof (data));
		data.ltype = ltype;
		data.lflag = 0;
		data.sparse = SPARSE_none;
		data.offset = offset;
		data.resid = length;
		data.alloc_unit = ip->cl_alloc_unit;
		data.cmd = 0;
		data.filemode = (ltype == LTYPE_read ? FREAD : FWRITE);
		data.shflags.b.directio = 0;
		data.shflags.b.abr = ip->flags.b.abr;

		/*
		 * If it takes more than SAM_QUICK_DELAY seconds to
		 * complete we fail with EDEADLK back to the VM
		 * layer.  VM will re-try the getpage after a small
		 * delay.  This is so we don't block for a long time
		 * while holding the AS_LOCK that was acquired in
		 * as_fault().
		 */
		if (error = sam_proc_get_lease(ip, &data, NULL, NULL,
		    SHARE_quickwait, credp)) {
			SAM_DECREMENT_LEASEUSED(ip, ltype);
			if (error == ETIME || error == EAGAIN) {
				if (sam_check_sig()) {
					error = EINTR;
				} else {
					SAM_COUNT64(shared_client,
					    page_lease_retry);
					error = EDEADLK;
				}
			}
			if (unset_nb) {
				sam_unset_operation_nb(ip->mp);
			}
			SAM_CLOSE_OPERATION(ip->mp, error);
			return (error);
		}
	} else {
		mutex_exit(&ip->ilease_mutex);
	}

start:
	if (SAM_IS_SHARED_SERVER(ip->mp) || !S_ISDIR(ip->di.mode)) {
#if defined(SOL_511_ABOVE)
		error = sam_getpage_vn(vp, offset, length,
		    protp, pglist, plsize, segp, addr, rw, credp, ct);
#else
		error = sam_getpage_vn(vp, offset, length,
		    protp, pglist, plsize, segp, addr, rw, credp);
#endif
		if (error == ENOTCONN || error == EPIPE) {
			if ((error = sam_stop_inode(ip)) == 0) {
				goto start;
			}
		}
		if (using_lease) {
			if (unset_nb) {
				sam_unset_operation_nb(ip->mp);
			}
			SAM_DECREMENT_LEASEUSED(ip, ltype);
			SAM_CLOSE_OPERATION(ip->mp, error);
		}
		return (error);
	}

	ASSERT(S_ISDIR(ip->di.mode));
	ASSERT(using_lease == FALSE);

	if (pglist == NULL) {
		return (0);
	}
	if (protp != NULL) {
		*protp = PROT_ALL;
	}

	/*
	 * Acquire the inode READER lock if WRITER lock not already held.
	 */
	if (RW_OWNER_OS(&ip->inode_rwl) != curthread) {
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		lock_set = B_TRUE;
	}

	pppl = pglist;
	off = offset & PAGEMASK;

again:
	while (len > 0) {
		buf_t *bp;

		/*
		 * Create the page. pageio_setup returns a buf pointer. bp_mapin
		 * converts the bp to the kernel virtual addressable location.
		 */
		if ((pp = page_lookup(vp, off, SE_SHARED)) == NULL) {
			u_offset_t io_off;
			size_t io_len;

			if ((pp = pvn_read_kluster(vp, off, segp, addr, &io_off,
			    &io_len, off, PAGESIZE, 0)) == NULL) {
				goto again;
			}
			bp = pageio_setup(pp, PAGESIZE,
			    ip->mp->mi.m_fs[0].svp, B_READ);
			bp_mapin(bp);
			fgetbuf.offset = off;
			fgetbuf.len = PAGESIZE;
			fgetbuf.bp = (int64_t)bp;
			fgetbuf.ino = ip->di.id.ino;
			error = sam_proc_block(ip->mp, ip,
			    BLOCK_fgetbuf, SHARE_wait, &fgetbuf);
			bp_mapout(bp);
			pageio_done(bp);
			if (SAM_IS_SHARED_SERVER(ip->mp)) {
				error = ENOTCONN;
			}
			if (error) {
				if (off > offset) {
					while (pppl > pglist) {
						page_unlock(*--pppl);
						*pppl = NULL;
					}
				}
				pvn_read_done(pp, B_ERROR);
				break;
			}
			pvn_plist_init(pp, pppl, plsz, off, PAGESIZE, rw);
		}
		*pppl = pp;
		off += PAGESIZE;
		len -= MIN(len, PAGESIZE);
		addr += PAGESIZE;
		plsz -= PAGESIZE;
		pppl++;
		*pppl = NULL;
	}
	if (lock_set) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}
	if ((error == ENOTCONN) || (error == EPIPE)) {
		if ((error = sam_stop_inode(ip)) == 0) {
			if (SAM_IS_SHARED_SERVER(ip->mp)) {
#if defined(SOL_511_ABOVE)
				error = sam_getpage_vn(vp, offset,
				    length, protp, pglist, plsize,
				    segp, addr, rw, credp, ct);
#else
				error = sam_getpage_vn(vp, offset,
				    length, protp, pglist, plsize,
				    segp, addr, rw, credp);
#endif
				return (error);
			}
			if (lock_set) {
				RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			}
			goto again;
		}
	}
	TRACE(T_SAM_CL_GETPG_RET, vp, error, (sam_tr_t)pp, pp->p_offset);
	return (error);
}


/*
 * ----- sam_client_map_vn - Set up a vnode so its pages can be mmapped.
 *
 * Set up a vnode so its pages can be memory mapped.
 */

int				/* ERRNO if error, 0 if successful. */
sam_client_map_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	offset_t offset,	/* file offset. */
	struct as *asp,		/* pointer to address segment. */
	char **addrpp,		/* pointer pointer to address. */
	sam_size_t length,	/* file length provided to mmap. */
	uchar_t prot,		/* requested protection from <sys/mman.h> */
	uchar_t maxprot,	/* max requested protection from <sys/mman.h> */
	uint_t flags,		/* flags. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	offset_t offset,	/* file offset. */
	struct as *asp,		/* pointer to address segment. */
	char **addrpp,		/* pointer pointer to address. */
	sam_size_t length,	/* file length provided to mmap. */
	uchar_t prot,		/* requested protection from <sys/mman.h> */
	uchar_t maxprot,	/* max requested protection from <sys/mman.h> */
	uint_t flags,		/* flags. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	struct segvn_crargs vn_seg;
	int error;
	sam_node_t *ip;
	sam_u_offset_t off = offset;
	boolean_t is_write;
	sam_lease_data_t data;
	offset_t orig_resid;
	offset_t prev_size;
	uint32_t ltype;
	boolean_t clear_pending_mmapper = FALSE;

	TRACE(T_SAM_MAP, vp, (sam_tr_t)offset, length, prot);
	ip = SAM_VTOI(vp);
	if (ip->di.id.ino == SAM_INO_INO) {	/* If .inodes file */
		return (EPERM);
	}
	if (vp->v_flag & VNOMAP) {	/* File cannot be memory mapped */
		return (ENOSYS);
	}
	if ((offset < 0) || ((offset + length) > SAM_MAXOFFSET_T) ||
	    ((offset + length) < 0)) {
		return (EINVAL);
	}
	/*
	 * If file is not regular, file is a segment, or locked, cannot mmap
	 * file.
	 */
	if (vp->v_type != VREG || ip->di.status.b.segment) {
		return (ENODEV);
	}
	if (vn_has_flocks(vp) && MANDLOCK(vp, ip->di.mode)) {
		return (EAGAIN);
	}

	/*
	 * Mmap not supported for removable media files or stage_n
	 * (direct access).
	 */
	if (S_ISREQ(ip->di.mode)) {
		return (EINVAL);
	}
	if (ip->di.status.b.direct) {
		return (EINVAL);
	}

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

	/*
	 * If mmap write and the file is offline, stage in the entire file.
	 * (Only for shared mappings; treat private mappings as read-only.)
	 */
	is_write = (prot & PROT_WRITE) && ((flags & MAP_TYPE) == MAP_SHARED);

reenter:
	if (is_write) {
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (ip->di.status.b.offline || ip->flags.b.stage_pages) {
			error = sam_clear_ino(ip, ip->di.rm.size,
			    MAKE_ONLINE, credp);
			if (error) {
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				goto out;
			}
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}

	RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	mutex_enter(&ip->fl_mutex);
	if (ip->last_unmap) {
		ASSERT(ip->pending_mmappers == 0);
		ASSERT(ip->mm_pages == 0);
		mutex_exit(&ip->fl_mutex);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		delay(hz);
		goto reenter;
	}

	/*
	 * Serialize mmappers.
	 */
	if (ip->pending_mmappers || ip->cl_closing) {
		mutex_exit(&ip->fl_mutex);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		delay(hz);
		goto reenter;
	}

	ip->pending_mmappers = (uint64_t)curthread;
	mutex_exit(&ip->fl_mutex);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);

	/*
	 * We set pending_mmappers as a bridge between the point where
	 * we get the CL_MMAP lease and where we set mm_pages.  This stops
	 * anyone from releasing our lease via CL_CLOSE before
	 * we can set mm_pages.
	 */
	clear_pending_mmapper = TRUE;
	as_rangelock(asp);				/* Lock address space */

	if ((flags & MAP_FIXED) == 0) {
		map_addr(addrpp, length, off, 1, flags);
		if (*addrpp == NULL) {
			as_rangeunlock(asp);
			error = ENOMEM;
			goto out_asunlock;
		}
	} else {
		/* Unmap any previous mappings */
		as_unmap(asp, *addrpp, length);
	}

	/*
	 * Get memory-mapping lease based on requested page permissions
	 * and sharing type.  (Private mappings are always read-only
	 * from the file system's perspective; pages which are written
	 * become anonymous.)
	 */
	prev_size = ip->size;
	bzero(&data, sizeof (data));
	data.offset = off & PAGEMASK;
	data.resid = ((off + length + PAGESIZE - 1) & PAGEMASK) - data.offset;
	orig_resid = length;
	data.alloc_unit = ip->cl_alloc_unit;
	data.shflags.b.directio = ip->flags.b.directio;
	data.shflags.b.abr = ip->flags.b.abr;
	data.ltype = LTYPE_mmap;
	if (is_write) {
		ltype = LTYPE_write;
		/*
		 * Force blocks to be allocated and zeroed
		 */
		data.sparse = SPARSE_zeroall;
		data.filemode = FWRITE;
	} else {
		ltype = LTYPE_read;
		data.sparse = SPARSE_none;
		data.filemode = FREAD;
	}
	mutex_enter(&ip->ilease_mutex);
	ip->cl_leaseused[ltype]++;
	mutex_exit(&ip->ilease_mutex);

	for (;;) {
		error = sam_get_zerodaus(ip, &data, credp);
		if (error == 0) {
			if (is_write) {
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				if (SAM_IS_SHARED_CLIENT(ip->mp)) {
					error = sam_process_zerodaus(ip,
					    &data, credp);
				}
				if ((error == 0) &&
				    SAM_IS_SHARED_CLIENT(ip->mp)) {
					orig_resid -= data.resid;
					if (orig_resid <= 0) {
						if (offset > prev_size) {
							/*
							 * Zero the DAUs between
							 * size and offset.
							 */
							sam_zero_sparse_blocks(
							    ip, prev_size,
							    offset);
						}
						RW_UNLOCK_OS(&ip->inode_rwl,
						    RW_WRITER);
						break;
					}
					data.offset += data.resid;
					data.resid = orig_resid;
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
					continue;
				}
				if (error) {
					if (error == ELNRNG) {
						cmn_err(CE_WARN,
			"SAM-QFS: %s: mmap ELNRNG error: ino %d.%d, "
			"off %llx len %llx",
						    ip->mp->mt.fi_name,
						    ip->di.id.ino,
						    ip->di.id.gen, offset,
						    (long long)length);
						SAMFS_PANIC_INO(
						    ip->mp->mt.fi_name,
						    "SAM-QFS: mmap ELNRNG",
						    ip->di.id.ino);
					}
				}
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			}
		} else {
			SAM_DECREMENT_LEASEUSED(ip, ltype);
			goto out_asunlock;
		}
		break;
	}
	SAM_DECREMENT_LEASEUSED(ip, ltype);

	/* Create vnode mapped segment. */
	bzero((char *)&vn_seg, sizeof (vn_seg));
	vn_seg.vp = vp;
	vn_seg.cred = credp;
	vn_seg.offset = off;
	vn_seg.type = flags & MAP_TYPE;		/* Type of sharing done */
	vn_seg.prot = prot;
	vn_seg.maxprot = maxprot;
	vn_seg.flags = flags & ~MAP_TYPE;
	vn_seg.amp = NULL;

	if (error = as_map(asp, *addrpp, length, segvn_create, (caddr_t)
	    & vn_seg)) {

		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		mutex_enter(&ip->fl_mutex);
		ASSERT(ip->last_unmap == 0);
		ASSERT(ip->pending_mmappers > 0);
		ip->pending_mmappers = 0;
		mutex_exit(&ip->fl_mutex);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);

		clear_pending_mmapper = FALSE;

		sam_mmap_rmlease(vp, is_write, 0);
	}

out_asunlock:
	as_rangeunlock(asp);			/* Unlock address space */

	if (clear_pending_mmapper) {
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		mutex_enter(&ip->fl_mutex);
		ASSERT(ip->last_unmap == 0);
		ASSERT(ip->pending_mmappers > 0);
		ip->pending_mmappers = 0;
		mutex_exit(&ip->fl_mutex);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}

out:
	SAM_CLOSE_OPERATION(ip->mp, error);
	TRACE(T_SAM_MAP_RET, vp, error, 0, 0);
	return (error);
}


/*
 * ----- sam_client_addmap_vn - Increment count of memory mapped pages.
 */

/* ARGSUSED */
static int			/* ERRNO if error, 0 if successful. */
sam_client_addmap_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* pointer to vnode. */
	offset_t offset,	/* file offset. */
	struct as *asp,		/* pointer to address segment. */
	caddr_t a,		/* pointer offset. */
	sam_size_t length,	/* file length provided to mmap. */
	uchar_t prot,		/* requested protection from <sys/mman.h>. */
	uchar_t maxprot,	/* max requested prot from <sys/mman.h>. */
	uint_t flags,		/* flags. */
	cred_t *credp,		/* credentials pointer. */
	caller_context_t *ct	/* caller context pointer. */
#else
	vnode_t *vp,		/* pointer to vnode. */
	offset_t offset,	/* file offset. */
	struct as *asp,		/* pointer to address segment. */
	caddr_t a,		/* pointer offset. */
	sam_size_t length,	/* file length provided to mmap. */
	uchar_t prot,		/* requested protection from <sys/mman.h>. */
	uchar_t maxprot,	/* max requested prot from <sys/mman.h>. */
	uint_t flags,		/* flags. */
	cred_t *credp		/* credentials pointer. */
#endif
)
{
	int error = 0;
	int pages;
	sam_node_t *ip;

	TRACE(T_SAM_CL_ADDMAP, vp, (sam_tr_t)offset, length, prot);
	ip = SAM_VTOI(vp);

	pages = btopr(length);

	if (vp->v_flag & VNOMAP) {	/* If file cannot be memory mapped */
		error = ENOSYS;

	} else {
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		mutex_enter(&ip->fl_mutex);
		ip->mm_pages += pages;
		if ((prot & PROT_WRITE) && ((flags & MAP_TYPE) == MAP_SHARED)) {
			ip->wmm_pages += pages;
		}
		mutex_exit(&ip->fl_mutex);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}
	TRACE(T_SAM_CL_ADDMAP_RET, vp, pages, ip->mm_pages, error);
	return (error);
}


/*
 * ----- sam_client_delmap_vn - Decrement count of memory mapped pages.
 *
 * Setup and add an address space callback to do the work of the delmap call.
 * The callback will (and must be) deleted in the actual callback function.
 *
 * This is done in order to take care of the problem that we have with holding
 * the address space's a_lock for a long period of time (e.g. if failover is
 * happening).  Callbacks will be executed in the address space code while the
 * a_lock is not held.  Holding the address space's a_lock causes things such
 * as ps and fork to hang because they are trying to acquire this lock as well.
 */

/* ARGSUSED */
static int			/* ERRNO if error, 0 if successful. */
sam_client_delmap_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* Pointer to vnode. */
	offset_t offset,	/* File offset. */
	struct as *asp,		/* Pointer to address segment. */
	caddr_t a,		/* Pointer offset. */
	sam_size_t length,	/* File length provided to mmap. */
	uint_t prot,		/* Requested access from <sys/mman.h>. */
	uint_t maxprot,		/* Max requested access from <sys/mman.h> */
	uint_t flags,		/* Flags. */
	cred_t *credp,		/* Credentials pointer. */
	caller_context_t *ct	/* Caller context pointer. */
#else
	vnode_t *vp,		/* Pointer to vnode. */
	offset_t offset,	/* File offset. */
	struct as *asp,		/* Pointer to address segment. */
	caddr_t a,		/* Pointer offset. */
	sam_size_t length,	/* File length provided to mmap. */
	uint_t prot,		/* Requested access from <sys/mman.h>. */
	uint_t maxprot,		/* Max requested access from <sys/mman.h> */
	uint_t flags,		/* Flags. */
	cred_t *credp		/* Credentials pointer. */
#endif
)
{
	int error;
	int pages;
	sam_node_t *ip;
	int caller_found;
	sam_delmap_args_t *dmapp;
	sam_delmapcall_t *delmap_call;

	TRACE(T_SAM_CL_DELMAP, vp, (sam_tr_t)offset, length, prot);
	ip = SAM_VTOI(vp);

	pages = btopr(length);

	if (vp->v_flag & VNOMAP) {
		error = ENOSYS;
		goto out;
	}

	/*
	 * The way that the address space of this process deletes its mapping
	 * of this file is via the following call chains:
	 * - as_free()->SEGOP_UNMAP()/segvn_unmap()->VOP_DELMAP()/...
	 * - as_unmap()->SEGOP_UNMAP()/segvn_unmap()->VOP_DELMAP()/...
	 *
	 * With the use of address space callbacks we are allowed to drop the
	 * address space lock, a_lock, while executing fs operations that
	 * need to go over the wire.  Returning EAGAIN to the caller of this
	 * function is what drives the execution of the callback that we add
	 * below.  The callback will be executed by the address space code
	 * after dropping the a_lock.  When the callback is finished, since
	 * we dropped the a_lock, it must be re-acquired and segvn_unmap()
	 * is called again on the same segment to finish the rest of the work
	 * that needs to happen during unmapping.
	 *
	 * This action of calling back into the segment driver causes
	 * this delmap to get called again, but since the callback was
	 * already executed at this point, it already did the work and there
	 * is nothing left for us to do.
	 *
	 * To Summarize:
	 * - The first time this delmap is called by the current thread is when
	 * we add the caller associated with this delmap to the delmap caller
	 * list, add the callback, and return EAGAIN.
	 * - The second time in this call chain when this delmap is called we
	 * will find this caller in the delmap caller list and realize there
	 * is no more work to do thus removing this caller from the list and
	 * returning the error that was set in the callback execution.
	 */
	caller_found = sam_find_and_delete_delmapcall(ip, &error);
	if (caller_found) {
		/*
		 * 'error' is from the actual delmap operations.  To avoid
		 * hangs, we need to handle the return of EAGAIN differently
		 * since this is what drives the callback execution.
		 * In this case, we don't want to return EAGAIN and do the
		 * callback execution because there are none to execute.
		 */
		if (error == EAGAIN) {
			error = 0;
		}
		return (error);
	}

	/* current caller was not in the list */
	delmap_call = sam_init_delmapcall();

	mutex_enter(&ip->i_indelmap_mutex);
	list_insert_tail(&ip->i_indelmap, delmap_call);
	mutex_exit(&ip->i_indelmap_mutex);

	dmapp = kmem_alloc(sizeof (sam_delmap_args_t), KM_SLEEP);

	dmapp->vp = vp;
	dmapp->off = offset;
	dmapp->len = length;
	dmapp->prot = prot;
	dmapp->flags = flags;
	dmapp->caller = delmap_call;

	error = as_add_callback(asp, sam_delmap_callback, dmapp,
	    AS_UNMAP_EVENT, a, length, KM_SLEEP);
	if (error == 0) {
		error = EAGAIN;
	}
out:
	TRACE(T_SAM_CL_DELMAP_RET, vp, pages, ip->mm_pages, error);
	return (error);
}


/*
 * ----- sam_delmap_callback -
 *
 * This callback is executed by the segmap code after sam_client_delmap_vn
 * returns EAGAIN to segmap.  The callback is executed after the address
 * space lock is released.
 */

static void
sam_delmap_callback(struct as *as, void *arg, uint_t event)
{
	sam_delmap_args_t *dmapp = (sam_delmap_args_t *)arg;
	sam_size_t length;
	offset_t offset;
	uint_t prot;
	int pages;
	int error;
	sam_node_t *ip;
	boolean_t is_write;
	vnode_t *vp;
	uint_t flags;

	vp = dmapp->vp;
	length = dmapp->len;
	prot = dmapp->prot;
	offset = dmapp->off;
	flags = dmapp->flags;

	ip = SAM_VTOI(vp);
	pages = btopr(length);

	TRACE(T_SAM_CL_DELMAPCB, vp, (sam_tr_t)offset, length, prot);

	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		goto out;
	}

	is_write = (prot & PROT_WRITE) &&
	    ((flags & MAP_TYPE) == MAP_SHARED);
	sam_mmap_rmlease(vp, is_write, pages);

	SAM_CLOSE_OPERATION(ip->mp, error);

out:
	dmapp->caller->error = error;
	(void) as_delete_callback(as, arg);
	kmem_free(dmapp, sizeof (sam_delmap_args_t));
	TRACE(T_SAM_CL_DELMAPCB_RET, vp, pages, ip->mm_pages, error);
}


/*
 * ----- sam_init_delmapcall -
 *
 * Initialize a sam_delmapcall_t to hold the identity and status for
 * a callback.
 */
static sam_delmapcall_t *
sam_init_delmapcall()
{
	sam_delmapcall_t	*delmap_call;

	delmap_call = kmem_alloc(sizeof (sam_delmapcall_t), KM_SLEEP);
	delmap_call->call_id = curthread;
	delmap_call->error = 0;

	return (delmap_call);
}


/*
 * ----- sam_free_delmapcall -
 *
 * Destroy a sam_delmapcall_t.
 */
static void
sam_free_delmapcall(sam_delmapcall_t *delmap_call)
{
	kmem_free(delmap_call, sizeof (sam_delmapcall_t));
}


/*
 * Searches for the current delmap caller (based on curthread) in the list of
 * callers.  If it is found, we remove it and free the delmap caller.
 * Returns:
 *	0 if the caller wasn't found
 *	1 if the caller was found, removed and freed.  *errp is set to what
 * 	the result of the delmap was.
 */
static int
sam_find_and_delete_delmapcall(sam_node_t *ip, int *errp)
{
	sam_delmapcall_t	*delmap_call;

	/*
	 * If the list doesn't exist yet, we create it and return
	 * that the caller wasn't found.  No list = no callers.
	 */
	mutex_enter(&ip->i_indelmap_mutex);
	if (!(ip->flags.bits & SAM_DELMAPLIST)) {
		/* The list does not exist */
		list_create(&ip->i_indelmap, sizeof (sam_delmapcall_t),
		    offsetof(sam_delmapcall_t, call_node));
		ip->flags.bits |= SAM_DELMAPLIST;
		mutex_exit(&ip->i_indelmap_mutex);
		return (0);
	} else {
		/* The list exists so search it */
		for (delmap_call = list_head(&ip->i_indelmap);
		    delmap_call != NULL;
		    delmap_call = list_next(&ip->i_indelmap, delmap_call)) {
			if (delmap_call->call_id == curthread) {
				/* current caller is in the list */
				*errp = delmap_call->error;
				list_remove(&ip->i_indelmap, delmap_call);
				mutex_exit(&ip->i_indelmap_mutex);
				sam_free_delmapcall(delmap_call);
				return (1);
			}
		}
	}
	mutex_exit(&ip->i_indelmap_mutex);
	return (0);
}


/*
 * ----- sam_mmap_rmlease - Remove mmap lease.
 */

static void
sam_mmap_rmlease(
	vnode_t *vp,		/* Pointer to vnode. */
	boolean_t is_write,	/* Flag. */
	int pages)		/* Page count. */
{
	krw_t rw_type = RW_READER;
	sam_node_t *ip = SAM_VTOI(vp);
	unsigned long last_unmapper = 0;

reenter:
	RW_LOCK_OS(&ip->inode_rwl, rw_type);
	mutex_enter(&ip->fl_mutex);
	if ((ip->pending_mmappers) &&
	    (ip->pending_mmappers != (uint64_t)curthread)) {
		mutex_exit(&ip->fl_mutex);
		RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
		delay(hz);
		goto reenter;
	}

	if (pages) {
		ip->mm_pages -= pages;
		if (is_write) {
			ip->wmm_pages -= pages;
		}
	}
	if ((ip->mm_pages == 0) && (ip->pending_mmappers == 0) &&
	    (ip->last_unmap == 0)) {
		ip->last_unmap = 1;
		last_unmapper = 1;
	}
	mutex_exit(&ip->fl_mutex);

	if (last_unmapper) {
		/*
		 * On last delete map, sync pages on the client.
		 */
		if (vn_has_cached_data(vp)) {
			if (!rw_tryupgrade(&ip->inode_rwl)) {
				RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			}
			rw_type = RW_WRITER;
			if (vn_has_cached_data(vp)) {
				sam_flush_pages(ip, 0);
			}
		}

		/*
		 * If the file is not open, we were the last
		 * reference, so close the inode.  Otherwise,
		 * just relinquish the lease.
		 */
		if (ip->no_opens == 0) {
			if (sam_proc_rm_lease(ip, CL_CLOSE, rw_type) == 0) {
				/*
				 * Clean up staging flag so inode inactive
				 * happens cleanly.
				 */
				ip->flags.b.staging = 0;
			}
			if (rw_type == RW_WRITER) {
				ASSERT(ip->mm_pages == 0);
				ASSERT(ip->pending_mmappers == 0);
				ASSERT(ip->last_unmap == 1);
				ip->last_unmap = 0;
			}
			RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
		} else {
			RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
			sam_client_remove_leases(ip, CL_MMAP, 0);
			(void) sam_proc_relinquish_lease(ip,
			    CL_MMAP, FALSE, ip->cl_leasegen);
		}
		if (ip->last_unmap) {
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			mutex_enter(&ip->fl_mutex);
			ASSERT(ip->mm_pages == 0);
			ASSERT(ip->pending_mmappers == 0);
			ASSERT(ip->last_unmap == 1);
			ip->last_unmap = 0;
			mutex_exit(&ip->fl_mutex);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		}
	} else {
		RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
	}
}


/*
 * ----- sam_client_getsecattr_vn  - get security attributes for vnode.
 *
 * Retrieve security attributes from access control list.
 */

/* ARGSUSED3 */
int
sam_client_getsecattr_vn(
#if defined(SOL_511_ABOVE)
	vnode_t *vp,		/* Pointer to vnode. */
	vsecattr_t *vsap,	/* Security attributes pointer. */
	int flag,		/* Flags. */
	cred_t *credp,		/* Credentials pointer. */
	caller_context_t *ct	/* Caller context pointer. */
#else
	vnode_t *vp,		/* Pointer to vnode. */
	vsecattr_t *vsap,	/* Security attributes pointer. */
	int flag,		/* Flags. */
	cred_t *credp		/* Credentials pointer. */
#endif
)
{
	sam_node_t *ip;
	int error = 0;

	vsap->vsa_mask &= (VSA_ACL | VSA_ACLCNT | VSA_DFACL | VSA_DFACLCNT);
	if (vsap->vsa_mask == 0) {
		return (0);
	}

	ip = SAM_VTOI(vp);
	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

	RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	if (ip->di.status.b.acl && !ip->aclp) {
		error = sam_get_acl(ip, &ip->aclp);
	}
	if (!error) {
		error = sam_acl_get_vsecattr(ip, vsap);
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);

	SAM_CLOSE_OPERATION(ip->mp, error);
	TRACE(T_SAM_GETSECATTR, vp, (sam_tr_t)vsap, flag, error);
	return (error);
}


/*
 * ----- sam_client_setsecattr_vn  - set security attributes for vnode.
 *
 *	Set security attributes in inode extensions(s).
 */

int
sam_client_setsecattr_vn(
#if defined(SOL_511_ABOVE)
	struct vnode *vp,	/* Pointer to vnode. */
	vsecattr_t *vsap,	/* Security attributes pointer. */
	int flag,		/* Flags. */
	struct cred *credp,	/* Credentials pointer. */
	caller_context_t *ct	/* Caller context pointer. */
#else
	struct vnode *vp,	/* Pointer to vnode. */
	vsecattr_t *vsap,	/* Security attributes pointer. */
	int flag,		/* Flags. */
	struct cred *credp	/* Credentials pointer. */
#endif
)
{
	sam_name_acl_t acl;
	int error = 0;
	sam_node_t *ip;

	ip = SAM_VTOI(vp);
	if ((error = sam_open_operation(ip))) {
		SAM_NFS_JUKEBOX_ERROR(error);
		return (error);
	}

#ifdef METADATA_SERVER
	if (SAM_IS_SHARED_SERVER(ip->mp)) {
#if defined(SOL_511_ABOVE)
		if ((error = sam_setsecattr_vn(vp, vsap, flag, credp, ct))
		    == 0) {
#else
		if ((error = sam_setsecattr_vn(vp, vsap, flag, credp)) == 0) {
#endif
			sam_callout_acl(ip, ip->mp->ms.m_client_ord);
		}
		goto out;
	}
#endif

	/*
	 * Only super-user or file owner can set access control list.
	 */
	if (secpolicy_vnode_setdac(credp, ip->di.uid)) {
		error = EPERM;
		goto out;
	}
	vsap->vsa_mask &= (VSA_ACL | VSA_DFACL);
	if (vsap->vsa_mask == 0 ||
	    (vsap->vsa_aclentp == NULL && vsap->vsa_dfaclentp == NULL)) {
		error = EINVAL;
		goto out;
	}

	/*
	 * Make sure default access control list only on directories.
	 */
	if (vsap->vsa_dfaclcnt > 0 && !S_ISDIR(ip->di.mode)) {
		error = EINVAL;
		goto out;
	}

	bzero(&acl, sizeof (acl));
	acl.set = 1;
	acl.mask = vsap->vsa_mask;
	acl.aclcnt = vsap->vsa_aclcnt;
	acl.dfaclcnt = vsap->vsa_dfaclcnt;
	error = sam_proc_name(ip, NAME_acl, &acl, sizeof (acl),
	    vsap->vsa_aclentp, vsap->vsa_dfaclentp, credp, NULL);

out:
	SAM_CLOSE_OPERATION(ip->mp, error);
	TRACE(T_SAM_CL_SETATT_RET, vp, vsap->vsa_mask, flag, error);
	return (error);
}
