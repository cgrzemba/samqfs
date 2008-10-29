/*
 * ----- syscall.c - Process the sam system calls
 *
 *	Processes the system calls supported on the SAM File System.
 *	Called by the samioc system call module.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#ifdef sun
#pragma ident "$Revision: 1.163 $"
#endif

#include "sam/osversion.h"

/*
 * ----- UNIX Includes
 */

#ifdef sun
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/copyops.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/mode.h>
#include <sys/uio.h>
#include <sys/dirent.h>
#include <sys/proc.h>
#include <sys/disp.h>
#include <sys/mount.h>
#include <sys/policy.h>
#endif /* sun */

#ifdef linux
#include <linux/init.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>

#include <linux/fs.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#include <linux/namei.h>
#endif
#endif /* linux */

/*
 * ----- SAMFS Includes
 */

#ifdef linux
#include "sam/linux_types.h"
#endif /* linux */

/* The following lines need to stay in this order. */
#include "pub/stat.h"
#ifdef sun
/* These are duplicated in sys/stat.h */
#undef S_ISBLK
#undef S_ISCHR
#undef S_ISDIR
#undef S_ISFIFO
#undef S_ISGID
#undef S_ISREG
#undef S_ISUID
#undef S_ISLNK
#undef S_ISSOCK
#include <sys/stat.h>
#endif /* sun */
#ifdef linux
#include <linux/stat.h>
#endif /* linux */
#undef st_atime
#undef st_mtime
#undef st_ctime
#include "pub/rminfo.h"

#include "inode.h"
#include "mount.h"
#include "sam/param.h"
#include "sam/types.h"
#include "sam/checksum.h"
#define	DEC_INIT
#include "sam/devnm.h"
#undef DEC_INIT
#include "sam/resource.h"
#include "pub/sam_errno.h"
#include "sam/mount.h"
#include "sam/syscall.h"

#include "global.h"
#include "inode.h"
#include "mount.h"
#include "ino_ext.h"
#include "segment.h"
#ifdef sun
#include "extern.h"
#endif /* sun */
#ifdef linux
#include "clextern.h"
#endif /* linux */
#include "trace.h"
#include "debug.h"
#ifdef sun
#include "qfs_log.h"
#endif

#ifdef sun
#ifdef METADATA_SERVER
extern struct vnodeops samfs_vnodeops;
#endif

extern struct vnodeops samfs_client_vnodeops;

int sam_priv_syscall(int cmd, void *arg, int size, int *rvp);
int sam_priv_sam_syscall(int cmd, void *arg, int size, int *rvp);
#endif /* sun */

#ifdef linux
extern struct super_operations samqfs_super_ops;

int sam_priv_syscall(int cmd, void *arg, int size, int *rvp);
extern int sam_read_disk_ino(sam_mount_t *mp, sam_id_t *fp, char *buf);
int sam_trace_global(trace_global_tbl_t *);
#endif /* linux */

int sam_await_trace_tbl_data(sam_trace_tbl_t *);

/*
 * Private functions.
 */

static int sam_stat_file(int cmd, void *arg);
#ifdef sun
static int sam_stat_segment_file(int cmd, void *arg);
static int sam_proc_foreign_stat(vnode_t *vp, int cmd,
					void *args, cred_t *credp);
static int sam_file_operations(int cmd, void *arg);
static int sam_archive_copy(void *arg);
static int sam_vsn_stat_segment(void *arg);
static int sam_read_rminfo(void *arg);
static int sam_read_rm(sam_node_t *ip, char *buf, int bufsize);
static int sam_set_csum(void *arg);
static int sam_set_projid(void *arg, int size);
#endif /* sn */
static int sam_vsn_stat_file(void *arg);


/*
 * ----- sam_syscall - Process the sam system calls.
 */

static int			/* ERRNO if error, 0 if successful. */
__sam_syscall(
	int cmd,		/* Command number. */
	void *arg,		/* Pointer to arguments. */
	int size,
	int *rvp)
{
	int error = 0;

	if (cmd > SC_USER_MAX) {
		if ((cmd >= SC_FS_MIN) && (cmd <= SC_FS_MAX)) {
			/*
			 * Privileged system calls
			 */
			return (sam_priv_syscall(cmd, arg, size, rvp));

		} else if ((cmd >= SC_SAM_MIN) && (cmd <= SC_SAM_MAX)) {
			/*
			 * Privileged SAM system calls
			 */
			return (sam_priv_sam_syscall(cmd, arg, size, rvp));
		}
	}

	/*
	 * User system calls
	 */
	TRACE(T_SAM_SYSCALL, NULL, cmd, 0, 0);
	switch (cmd) {
		/*
		 *	Stat operations.
		 */
		case SC_stat:
		case SC_lstat:
			error = sam_stat_file(cmd, arg);
			break;

		/*
		 *	VSN file stat operation.
		 */
		case SC_vsn_stat:	/* vsn stat non-seg or index seg */
			error = sam_vsn_stat_file(arg);
			break;

#ifdef sun
		/*
		 *	File operations.
		 */
		case SC_archive:
		case SC_cancelstage:
		case SC_release:
		case SC_stage:
		case SC_ssum:
		case SC_setfa:
		case SC_segment:
			error = sam_file_operations(cmd, arg);
			break;

		case SC_projid:
			error = sam_set_projid(arg, size);
			break;

		case SC_setcsum:
			error = sam_set_csum(arg);
			break;

		/*
		 *	Unarchive, damage, or undamage an archive copy.
		 */
		case SC_archive_copy:
			error = sam_archive_copy(arg);
			break;


		/*
		 *	Read removable media information.
		 */
		case SC_readrminfo:
			error = sam_read_rminfo(arg);
			break;


		/*
		 *	Request removable media.
		 */
		case SC_request:
			error = sam_request_file(arg);
			break;


		/*
		 *	Stat segment operations.
		 */
		case SC_segment_stat:
		case SC_segment_lstat:
			error = sam_stat_segment_file(cmd, arg);
			break;


		/*
		 *	VSN segment stat operation.
		 */
		case SC_segment_vsn_stat:	/* vsn stat data segment */
			error = sam_vsn_stat_segment(arg);
			break;


		/*
		 * Quota operations.
		 */
		case SC_quota:
			error = sam_quota_stat((struct sam_quota_arg *)arg);
			break;
#endif /* sun */

#ifdef SAM_TRACE
		/*
		 * Trace operations
		 */
#ifdef linux
		case SC_trace_info:
			/*
			 * returns number of cpus and per cpu memory requirement
			 */
			error = sam_trace_info((sam_trace_info_t *)arg);
			break;
		case SC_trace_addr_data:
			/* returns samfs_trace */
			error = sam_trace_addr_data((sam_trace_tbl_t **)arg);
			break;
		case SC_trace_tbl_data:
			/*
			 * Returns data for given cpu.  Fill in tbl->t_cpu field
			 * before calling and make sure you've allocated the per
			 * cpu memory for it.
			 */
			error = sam_trace_tbl_data((sam_trace_tbl_t *)arg);
			break;
		case SC_trace_global:
			/*
			 * Returns information from samgt global table.
			 * We don't return samgt directly because it contains
			 * kmutex_t and kcondvar_t variables that are not
			 * consistent in size between linux kernel configs
			 * so the information is transfered to a variable
			 * of type trace_global_tbl_t first.
			 */
			error = sam_trace_global((trace_global_tbl_t *)arg);
			break;
#endif	/* linux */
		case SC_trace_tbl_wait:
			/*
			 * Returns trace data from a single CPU buffer.
			 * Returns data when the buffer is half full, or a
			 * specified interval has passed.  Returns EOVERFLOW
			 * if the buffer has wrapped since the last call (and
			 * returns the remaining data anyway).
			 */
			error = sam_await_trace_tbl_data(
			    (sam_trace_tbl_t *)arg);
			break;
#endif	/* SAM_TRACE */

		default:
			error = ENOTTY;
			break;

	}	/* end switch */

	SYSCALL_TRACE(T_SAM_SYSCALL_RET, 0, cmd, error, *rvp);
	return (error);
}

#ifdef sun
int
sam_syscall(int cmd, void *arg, int size, rval_t *rvp)
{
	int ret;

	ret = __sam_syscall(cmd, arg, size, &rvp->r_val1);
	return (ret);
}
#endif


#ifdef linux
int
sam_syscall(int cmd, void *arg, int size, int *rvp)
{
	int ret;

	ret = __sam_syscall(cmd, arg, size, rvp);
	return (ret);
}
#endif


/*
 * ----- sam_stat_file - Process the sam stat and lstat system calls.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_stat_file(int cmd, void *arg)
{
	struct sam_stat_arg args;
#ifdef sun
	vnode_t *vp, *rvp;
#endif /* sun */
#ifdef linux
	struct nameidata nd;
	struct inode *li;
#endif /* linux */
	sam_node_t *ip;
	int error;
	void *path;

	if (copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

#ifdef sun
	path = (void *)args.path.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		path = (void *)args.path.p64;
	}
	if ((error = lookupname((char *)path, UIO_USERSPACE,
	    cmd == SC_lstat ? NO_FOLLOW : FOLLOW, NULLVPP, &vp))) {
		return (error);
	}

	if (sam_set_realvp(vp, &rvp)) {		/* true if realvp not SAM-QFS */
		error = sam_proc_foreign_stat(vp, cmd, &args, CRED());
	} else {
		ip = SAM_VTOI(rvp);
		RW_LOCK_OS(&ip->inode_rwl, RW_READER);
		error = sam_proc_stat(rvp, cmd, &args, CRED());
		RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	}
	VN_RELE(vp);
#endif /* sun */

#ifdef linux
#ifdef _LP64
	path = (void *)args.path.p64;
#else
	path = (void *)args.path.p32;
#endif /* _LP64 */
	error = (cmd == SC_lstat ? user_path_walk_link(path, &nd) :
	    user_path_walk(path, &nd));
	error = -error;
	if (!error) {
		li = nd.dentry->d_inode;
		if (li->i_sb->s_op == &samqfs_super_ops) {
			ip = SAM_LITOSI(li);
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			error = sam_proc_stat(li, cmd, &args, CRED());
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		} else {
			error = ENOSYS;		/* Not a QFS i-node */
		}
		path_release(&nd);
	}
#endif /* linux */

	return (error);
}


#ifdef sun
/*
 * ----- sam_stat_segment_file - Process the sam stat and lstat system calls
 * for each of the segments.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_stat_segment_file(int cmd, void *arg)
{
	struct sam_stat_arg args;
	vnode_t *vp, *rvp;
	sam_node_t *ip;
	int error;
	void *path;

	if (copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}
	path = (void *)args.path.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		path = (void *)args.path.p64;
	}
	if ((error = lookupname((char *)path, UIO_USERSPACE,
	    cmd == SC_segment_lstat ? NO_FOLLOW : FOLLOW, NULLVPP, &vp))) {
		return (error);
	}

	if (sam_set_realvp(vp, &rvp)) {		/* true if realvp not SAM-QFS */
		error = EINVAL;
	} else {
		ip = SAM_VTOI(rvp);
		if (S_ISSEGI(&ip->di)) {
			sam_callback_segment_t callback;

			callback.p.syscall.func = sam_proc_stat;
			callback.p.syscall.cmd = cmd;
			callback.p.syscall.args = &args;
			callback.p.syscall.credp = CRED();
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			error = sam_callback_segment(ip, CALLBACK_syscall,
			    &callback,
			    FALSE);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
		}
	}
	VN_RELE(vp);
	return (error);
}


/*
 * ----- sam_proc_foreign_stat - Return status information to buffer.
 * Process status for non samfs file.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_proc_foreign_stat(vnode_t *vp, int cmd, void *args, cred_t *credp)
{
	struct sam_stat_arg *ap = (struct sam_stat_arg *)args;
	struct sam_stat sb;
	struct vattr vattr;
	int flag;
	int size;
	int error;
	void *buf;

	/*
	 * Return only UNIX stat() info.
	 */
	bzero((caddr_t)&sb, sizeof (sb));
	vattr.va_mask = AT_STAT | AT_NBLOCKS | AT_BLKSIZE | AT_SIZE;
	flag = (cmd == SC_stat) ? ATTR_REAL : 0;
	if ((error = VOP_GETATTR_OS(vp, &vattr, flag, credp, NULL))) {
		return (error);
	}
	sb.st_mode = VTTOIF(vattr.va_type) | vattr.va_mode;
	sb.st_ino = (ino_t)vattr.va_nodeid;
	sb.st_dev = vattr.va_fsid;
	sb.st_nlink = vattr.va_nlink;
	sb.st_uid = vattr.va_uid;
	sb.st_gid = vattr.va_gid;
	sb.st_size = vattr.va_size;
	sb.st_atime = vattr.va_atime.tv_sec;
	sb.st_mtime = vattr.va_mtime.tv_sec;
	sb.st_ctime = vattr.va_ctime.tv_sec;
	sb.rdev = vattr.va_rdev;
	size = ap->bufsize > sizeof (sb) ? sizeof (sb) : ap->bufsize;
	buf = (void *)ap->buf.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		buf = (void *)ap->buf.p64;
	}
	if (copyout((caddr_t)&sb, (caddr_t)buf, size)) {
		error = EFAULT;
	}
	return (error);
}
#endif /* sun */


/*
 * ----- sam_proc_stat - Return status information to buffer.
 * Process status for samfs file.
 */

/* ARGSUSED */
int		/* ERRNO if error, 0 if successful. */
sam_proc_stat(
#ifdef sun
	vnode_t *vp,
#endif /* sun */
#ifdef linux
	struct inode *li,
#endif /* linux */
	int cmd,
	void *args,
	cred_t *credp)
{
	struct sam_stat_arg *ap = (struct sam_stat_arg *)args;
	struct sam_stat sb;
	struct sam_perm_inode *permip;
	sam_node_t *ip;
	sam_node_t *bip;
	uint_t *arp;
#ifdef sun
	buf_t *bp;
	enum vtype type;			/* vnode type */
#endif /* sun */
#ifdef linux
	char *bp;
#endif /* linux */
	int size;
	int error;
	void *buf;

	bzero((caddr_t)&sb, sizeof (sb));

#ifdef sun
	ip = SAM_VTOI(vp);
#endif /* sun */
#ifdef linux
	ip = SAM_LITOSI(li);
#endif /* linux */

	if ((error = sam_open_operation(ip))) {
		return (error);
	}
	arp = (uint_t *)(void *)&ip->di.ar_flags[0];

	/*
	 * Fill in UNIX stat() info.
	 */
	sb.st_ino = ip->di.id.ino;			/* Inode number */

	if (S_ISSEGS(&ip->di)) {
		bip = ip->segment_ip;
	} else {
		bip = ip;
	}

#ifdef sun
	type = vp->v_type;
	if ((ip->di.rm.ui.flags & RM_CHAR_DEV_FILE) && (type == VCHR)) {
		type = VREG;
	}
	sb.st_mode  = VTTOIF(type) | bip->di.mode;
#endif /* sun */
#ifdef linux
	sb.st_mode  = bip->di.mode;
#endif /* linux */

	sb.st_nlink = bip->di.nlink;		/* Count of references */
	sb.st_uid   = bip->di.uid;			/* Owner user id */
	sb.st_gid   = bip->di.gid;			/* Owner group id */

	if (S_ISLNK(ip->di.mode) && (ip->di.ext_attrs & ext_sln)) {
		sb.st_size = ip->di.psize.symlink; /* Chars in symlink name */
	} else {
		sb.st_size = ip->di.rm.size;		/* File size lower */
	}
	sb.st_atime = ip->di.access_time.tv_sec; /* Time of last access */
	sb.st_mtime = ip->di.modify_time.tv_sec; /* Time of last modification */
	sb.st_ctime = ip->di.change_time.tv_sec; /* Time inode changed */
#ifdef sun
	if (curproc->p_model != DATAMODEL_ILP32) {
		sb.st_dev = ip->dev;		/* ID of device */
		sb.rdev = ip->rdev;
	} else {
		if (((cmpldev((dev32_t *)&sb.st_dev, ip->dev)) == 0) ||
		    ((cmpldev((dev32_t *)&sb.rdev, ip->rdev)) == 0)) {
			error = EOVERFLOW;
			SAM_CLOSE_OPERATION(ip->mp, error);
			return (error);
		}
	}
#endif /* sun */
#ifdef linux
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	if (li->i_bdev != NULL) {
		sb.st_dev = li->i_bdev->bd_dev;
	} /* else what? */
#else
	sb.st_dev = li->i_dev;
#endif
	sb.rdev	= li->i_rdev;
#endif /* linux */

	/*
	 * Add SAMFS info.
	 */
	sb.attr = (ip->di.status.bits & SAM_ATTR_MASK) | SS_SAMFS;
	if (ip->di.arch_status != 0) {
		sb.attr |= SS_ARCHIVED;
	}
	if (!ip->di.status.b.archdone && (*arp & 0x04040404)) {
		sb.attr |= SS_ARCHIVE_A;
	}
	if (*arp & 0x02020202) {
		sb.attr |= SS_ARCHIVE_R;
	}
	if (S_ISREQ(ip->di.mode)) {
		sb.attr |= SS_REMEDIA;
	}
	if (ip->di.rm.ui.flags & RM_CHAR_DEV_FILE) {
		sb.attr |= SS_AIO;
	}
	if (ip->di.rm.ui.flags & RM_DATA_VERIFY) {
		sb.attr |= SS_DATA_V;
	}
	if (SAM_IS_OBJECT_FS(ip->mp)) {
		sb.attr |= SS_OBJECT_FS;
		sb.stripe_width = ip->di.status.b.stripe_width ?
		    ip->di.rm.info.obj.stripe_width : ip->mp->mt.fi_stripe[DD];
		if (ip->di.rm.info.obj.stripe_shift) {	/* Units of kilobytes */
			sb.obj_depth =
			    (1 << (ip->di.rm.info.obj.stripe_shift - 10));
		}
	} else {
		sb.allocahead = ip->di.rm.info.dk.allocahead;
		sb.stripe_width = ip->di.status.b.stripe_width ?
		    ip->di.stripe : ip->mp->mt.fi_stripe[DD];
	}
	sb.old_attr = (uint_t)sb.attr;
	sb.stripe_group = ip->di.stripe_group;
	sb.attribute_time = ip->di.attribute_time;
	sb.creation_time = ip->di.creation_time;
	sb.residence_time = ip->di.residence_time;
	sb.cs_algo = ip->di.cs_algo;

	if (ip->flags.b.staging) sb.flags |= SS_STAGING;
	sb.flags |= ip->di.status.b.stage_failed ? SS_STAGEFAIL : 0;

	sb.gen = ip->di.id.gen;
	if (!S_ISREQ(ip->di.mode)) sb.partial_size = ip->di.psize.partial;
	if (ip->di.status.b.segment) {
		sb.stage_ahead = ip->di.stage_ahead;
		sb.segment_size = ip->di.rm.info.dk.seg_size;
		if (S_ISSEGS(&ip->di)) {
			sb.segment_number = ip->di.rm.info.dk.seg.ord + 1;
		}
	}
	sb.st_blocks  =
	    (u_longlong_t)ip->di.blocks * (u_longlong_t)(SAM_BLK/DEV_BSIZE);
	sb.admin_id   = bip->di.admin_id;

	/*
	 * Read permanent inode.  Enter archive copy info.
	 * Skip .inodes because readers' lock is re-acquired in sam_read_ino.
	 * If there is a thread waiting for the writer's lock, this is a
	 * deadlock condition. Note, .inodes has no archive info.
	 */
#ifdef linux
	if ((bp = kmem_alloc(sizeof (struct sam_perm_inode), KM_SLEEP)) ==
	    NULL) {
		error = ENOMEM;
		SAM_CLOSE_OPERATION(ip->mp, error);
		return (error);
	}
#endif /* linux */
	error = 0;
	if ((ip->di.id.ino != SAM_INO_INO) &&
#ifdef sun
	    ((error = sam_read_ino(ip->mp, ip->di.id.ino, &bp,
	    &permip)) == 0)) {
#endif /* sun */
#ifdef linux
		((error = sam_read_disk_ino(ip->mp, &(ip->di.id), bp)) == 0)) {
#endif /* linux */
		int copy, mask;
#ifdef linux
		permip = (struct sam_perm_inode *)bp;
#endif /* linux */
		sb.cs_val[0] = ((u_longlong_t)permip->csum.csum_val[0] << 32) |
		    (u_longlong_t)permip->csum.csum_val[1];
		sb.cs_val[1] = ((u_longlong_t)permip->csum.csum_val[2] << 32) |
		    (u_longlong_t)permip->csum.csum_val[3];
		for (copy = 0, mask = 1; copy < MAX_ARCHIVE;
		    copy++, mask += mask) {
			char *p;
			int media, n;

			sb.copy[copy].flags = ip->di.ar_flags[copy] &
			    ~AR_required;
			if (((ip->di.arch_status & mask) == 0) &&
			    (permip->ar.image[copy].vsn[0] == 0)) {
				continue;
			}
			/* Return active or stale copy. */
			sb.copy[copy].flags = ip->di.ar_flags[copy] &
			    CF_AR_FLAGS_MASK;
			if (ip->di.arch_status & mask) {
				sb.copy[copy].flags |= CF_ARCHIVED;
			}
			if (permip->ar.image[copy].arch_flags & SAR_pax_hdr) {
				sb.copy[copy].flags |= CF_PAX_ARCH_FMT;
			}
			sb.copy[copy].n_vsns = permip->ar.image[copy].n_vsns;
			sb.copy[copy].creation_time =
			    permip->ar.image[copy].creation_time;

			/*
			 * Set media mnemonic.
			 */
			media = ip->di.media[copy];
			n = media & DT_MEDIA_MASK;
			if (is_optical(media)) {
				p = dev_nmod[(n < OD_CNT ? n : 0)];
			} else if (is_tape(media)) {
				p = dev_nmtp[(n < MT_CNT ? n : 0)];
			} else if (is_disk(media)) {
				p = "dk";
				if (is_stk5800(media)) {
					p = "cb";
				}
			} else if (is_osd_group(media)) {
				if (n < dev_nmog_size) {
					p = dev_nmog[n];
				} else p = "??";
			} else if (is_stripe_group(media)) {
				if (n < dev_nmsg_size) {
					p = dev_nmsg[n];
				} else p = "??";
			} else if (is_third_party(media)) {
				if (n >= '0' && n <= '9') {
					p = dev_nmtr[n - '0'];
				} else if (n >= 'a' && n <= 'z') {
					p = dev_nmtr[(n - 'a') + 10];
				} else p = "??";
			} else p = "??";
			sb.copy[copy].media[0] = *p++;
			sb.copy[copy].media[1] = *p;

			strncpy(sb.copy[copy].vsn, permip->ar.image[copy].vsn,
			    sizeof (sb.copy[copy].vsn));
			sb.copy[copy].position =
			    (uint32_t)permip->ar.image[copy].position;
			sb.copy[copy].offset =
			    permip->ar.image[copy].file_offset;
			if ((permip->ar.image[copy].arch_flags &
			    SAR_size_block) == 0) {
				sb.copy[copy].offset >>= 9;
			}
		}
		if (ip->di.version == SAM_INODE_VERS_2) {
			sb.rperiod_start_time = ip->di2.rperiod_start_time;
			sb.rperiod_duration = ip->di2.rperiod_duration;
			if (ip->di2.p2flags & P2FLAGS_PROJID_VALID) {
				sb.projid = ip->di2.projid;
			} else {
				sb.projid = SAM_NOPROJECT;
			}
		}
#ifdef sun
		brelse(bp);
#endif /* sun */
	}
#ifdef linux
	kfree(bp);
#endif /* linux */

	size = ap->bufsize > sizeof (sb) ? sizeof (sb) : ap->bufsize;
#ifdef sun
	buf = (void *)ap->buf.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		buf = (void *)ap->buf.p64;
	}
#endif /* sun */
#ifdef linux
#ifdef _LP64
	buf = (void *)ap->buf.p64;
#else
	buf = (void *)ap->buf.p32;
#endif /* _LP64 */
#endif /* linux */
	if (copyout((caddr_t)&sb, (caddr_t)buf, size)) {
		error = EFAULT;
	}
	buf = (void *)((long)buf + sizeof (struct sam_stat));
#ifdef sun
	ap->buf.p32 = (int32_t)buf;
	if (curproc->p_model != DATAMODEL_ILP32) {
		ap->buf.p64 = (int64_t)buf;
	}
#endif /* sun */
#ifdef linux
#ifdef _LP64
	ap->buf.p64 = (int64_t)buf;
#else
	ap->buf.p32 = (int32_t)buf;
#endif /* _LP64 */
#endif /* linux */
	ap->bufsize -= size;
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


/*
 * ----- sam_vsn_stat_file - Process the sam vsn stat file system call.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_vsn_stat_file(void *arg)
{
	int						buf_size;
	int						copy;
	int						error;
	sam_node_t				*ip = NULL;
	sam_mount_t				*mp;
	struct sam_vsn_stat_arg	args;
	void					*buf;
	void					*path;
#ifdef sun
	vnode_t					*rvp;
	vnode_t					*vp;

	/* Get arguments. */

	if (copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	path = (void *) args.path.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		path = (void *) args.path.p64;
	}

	copy	 = args.copy;
	buf_size = args.bufsize;
	buf		 = (void *) args.buf.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		buf = (void *) args.buf.p64;
	}

	if ((error = lookupname((char *)path, UIO_USERSPACE, FOLLOW, NULLVPP,
	    &vp))) {
		return (error);
	}

	if (sam_set_realvp(vp, &rvp)) {		/* true if realvp not SAM-QFS */
		VN_RELE(vp);
		return (EINVAL);
	}

	ip = SAM_VTOI(rvp);

	if ((error = sam_open_operation(ip))) {
		VN_RELE(vp);
		return (error);
	}

	RW_LOCK_OS(&ip->inode_rwl, RW_READER);

	error = sam_vsn_stat_inode_operation(ip, copy, buf_size, buf);

	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	mp = ip->mp;
	VN_RELE(vp);
#endif /* sun */
#ifdef linux
	struct nameidata		nd;
	struct inode			*li;

	/* Get arguments. */

	if (copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

#ifdef _LP64
	path = (void *) args.path.p64;
#else
	path = (void *) args.path.p32;
#endif /* _LP64 */

	copy	 = args.copy;
	buf_size = args.bufsize;
#ifdef _LP64
	buf = (void *) args.buf.p64;
#else
	buf = (void *) args.buf.p32;
#endif /* _LP64 */

	if ((error = user_path_walk(path, &nd)) != 0) {
		return (error);
	}
	li = nd.dentry->d_inode;
	if (li->i_sb->s_op == &samqfs_super_ops) {
		ip = SAM_LITOSI(li);
		error = sam_open_operation(ip);
	} else {
		error = ENOSYS;
	}
	if (error) {
		path_release(&nd);
		return (error);
	}

	RW_LOCK_OS(&ip->inode_rwl, RW_READER);

	error = sam_vsn_stat_inode_operation(ip, copy, buf_size, buf);

	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	mp = ip->mp;
#endif /* linux */

	SAM_CLOSE_OPERATION(mp, error); /* can't use ip->mp since vp released */

#ifdef linux
	path_release(&nd);
#endif /* linux */

	return (error);
}

#ifdef sun
/*
 * ----- sam_vsn_stat_segment - Process the sam vsn stat segment system call.
 *
 * Note that this segment system call performs the vsn stat operation for only
 * one data segment.
 *
 * The operation is processed for the data segment indicated by segment_ord.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_vsn_stat_segment(void *arg)
{
	int			buf_size;
	int			copy;
	int			error;
	int			segment_ord;
	sam_node_t		*ip;
	struct sam_segment_vsn_stat_arg	args;
	vnode_t			*rvp;
	vnode_t			*vp;
	void			*buf;
	void			*path;
	sam_callback_segment_t		callback;
	sam_callback_sam_vsn_stat_t	*vsn_stat_callback;
	sam_mount_t		*mp;

	/* Get arguments. */

	if (copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	path = (void *) args.path.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		path = (void *) args.path.p64;
	}

	copy	 = args.copy;
	buf_size = args.bufsize;
	buf		 = (void *) args.buf.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		buf = (void *) args.buf.p64;
	}

	segment_ord = args.segment;

	if (segment_ord < 0) {
		return (EINVAL);
	}

	if ((error = lookupname((char *)path, UIO_USERSPACE, FOLLOW, NULLVPP,
	    &vp))) {
		return (error);
	}

	if (sam_set_realvp(vp, &rvp)) {		/* true if realvp not SAM-QFS */
		VN_RELE(vp);
		return (EINVAL);
	}

	ip = SAM_VTOI(rvp);

	if ((error = sam_open_operation(ip))) {
		VN_RELE(vp);
		return (error);
	}

	RW_LOCK_OS(&ip->inode_rwl, RW_READER);

	if (!S_ISSEGI(&ip->di)) {
		/*
		 * Request is for data segment operation, but
		 * file is not segmented:  return EINVAL.
		 */
		error = EINVAL;
	} else {
		vsn_stat_callback		= &callback.p.sam_vsn_stat;

		vsn_stat_callback->copy		= copy;
		vsn_stat_callback->bufsize	= buf_size;
		vsn_stat_callback->buf		= buf;

		error = sam_callback_one_segment(ip, segment_ord,
		    CALLBACK_stat_vsn,
		    &callback, FALSE);
	}

	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	mp = ip->mp;
	VN_RELE(vp);
	SAM_CLOSE_OPERATION(mp, error); /* can't use ip->mp since vp released */

	return (error);
}
#endif /* sun */


/*
 * ----- sam_vsn_stat_inode_operation - Perform the sam vsn stat operation
 * given the inode of the file or segment to vsn stat.
 */

int					/* ERRNO if error, 0 if successful. */
sam_vsn_stat_inode_operation(
	sam_node_t *ip,			/* file or segment to vsn stat */
	int copy,			/* 0 - 3, copy index of archive */
					/*   copy to vsn stat */
	int buf_size,			/* size of output buffer */
	void *buf)			/* pointer to output buffer */
{
#ifdef sun
	buf_t					*bp;
#endif /* sun */
#ifdef linux
	char					*bp;
#endif /* linux */
	boolean_t				buf_oflow;
	int						error;
	struct sam_perm_inode   *permip;

	buf_oflow = B_FALSE;
	error = EINVAL;		/* returned if trying to access .inodes */

	if (ip->di.id.ino == SAM_INO_INO) {
		return (error);
	}

	/*
	 * Read permanent inode.  Enter multi-volume archive copy info.
	 * Skip .inodes because readers' lock is re-acquired in sam_read_ino.
	 * If there is a thread waiting for the writer's lock, this is a
	 * deadlock condition. Note, .inodes has no archive info.
	 */
#ifdef sun
	error = sam_read_ino(ip->mp, ip->di.id.ino, &bp, &permip);
#endif /* sun */
#ifdef linux
	if ((bp = kmem_alloc(sizeof (struct sam_perm_inode),
	    KM_SLEEP)) == NULL) {
		return (ENOMEM);
	}
	error = sam_read_disk_ino(ip->mp, &(ip->di.id), bp);
	if (error) {
		kfree(bp);
		return (error);
	}
	permip = (struct sam_perm_inode *)bp;
#endif /* linux */
	if (error == 0) {
#ifdef sun
		sam_id_t eid;
#endif
		int t_vsns;

		/*
		 * need_size is the required minimum buffer size necessary to
		 * store the vsn stat file operation result:
		 */

		int need_size;
		struct sam_vsn_section *vsnp;

#ifdef linux
		ASSERT(ip->di.version >= SAM_INODE_VERS_2);
#endif
#ifdef sun
		if (ip->di.version == SAM_INODE_VERS_1) { /* Previous version */
			sam_perm_inode_v1_t *permip_v1 =
			    (sam_perm_inode_v1_t *)permip;

			eid = permip_v1->aid[copy];
		}
#endif

		t_vsns = permip->ar.image[copy].n_vsns;
		need_size = t_vsns * sizeof (struct sam_section);

#ifdef sun
		brelse(bp);
#endif /* sun */

		if (buf_size < need_size) {
			buf_oflow = B_TRUE;
		}

		vsnp = (struct sam_vsn_section *)buf;

		if (vsnp == NULL) {
			error = EINVAL;
		} else {
			if (ip->di.version >= SAM_INODE_VERS_2) {
				if (((ip->di.ext_attrs & ext_mva) == 0) ||
				    (t_vsns <= 0)) {
					error = EINVAL;
				} else {
					error = sam_get_multivolume(ip,
					    &vsnp, copy, &buf_size);

					if (error == EOVERFLOW) {
						buf_oflow = B_TRUE;
					}
				}
#ifdef sun
			} else if (ip->di.version == SAM_INODE_VERS_1) {
				/* Prev version */
				struct sam_inode_ext *eip;
				int ino_vsns;
				int copy_out_num_vsns = 0;

				if (t_vsns <= 0) {
					error = EINVAL;
				} else if (!buf_oflow) {
					copy_out_num_vsns = t_vsns;
				} else {
					copy_out_num_vsns = buf_size /
					    sizeof (*vsnp);
				}

				while ((copy_out_num_vsns > 0) && eid.ino &&
				    (error == 0)) {
					error = sam_read_ino(ip->mp, eid.ino,
					    &bp,
					    (struct sam_perm_inode **)&eip);
					if (error) {
						break;
					}
					if (!(SAM_CHECK_INODE_VERSION(
					    eip->hdr.version)) ||
					    (eip->hdr.version !=
					    ip->di.version) ||
					    (eip->hdr.id.ino != eid.ino) ||
							/*
							 * Backward
							 * compatibility code to
							 * allow 350/33x
							 * generated extension
							 * inodes to pass the
							 * header test.  350/33x
							 * set the "gen" portion
							 * of the "next_id"
							 * field to 0.
							 */
					    ((eip->hdr.id.gen != eid.gen) &&
					    (eid.gen != 0)) ||
					    (eip->hdr.file_id.ino !=
					    ip->di.id.ino) ||
					    (eip->hdr.file_id.gen !=
					    ip->di.id.gen) ||
					    ((eip->hdr.mode & S_IFEXT) !=
					    S_IFMVA)) {
						sam_req_ifsck(ip->mp, -1,
						    "sam_vsn_stat_file: "
						    "ino/gen", &ip->di.id);
						SAMFS_PANIC_INO(
						    ip->mp->mt.fi_name,
						    "Invalid mva ext 1",
						    ip->di.id.ino);
						error = ENOCSI;
					} else {
						ino_vsns = MIN(
						    copy_out_num_vsns,
						    MAX_VSNS_IN_INO);


	/* N.B. Bad indentation here to meet cstyle requirements */
	if (copyout(
	    (caddr_t)&eip->ext.mv1.vsns,
	    (caddr_t)vsnp,
	    ino_vsns *
	    sizeof (struct sam_section))) {
		error = EFAULT;
	}


						vsnp += ino_vsns;
						copy_out_num_vsns -= ino_vsns;
					}
					eid = eip->hdr.next_id;
					brelse(bp);
				}
#endif /* sun */
			}
		}
	}
#ifdef linux
	kfree(bp);
#endif /* linux */
	if (buf_oflow && error == 0) {
		error = EOVERFLOW;
	}

	return (error);
}


#ifdef sun
/*
 * ----- sam_file_operations - archive, stage, release, csum,
 * setfa syscall calls.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_file_operations(int cmd, void *arg)
{
	struct sam_fileops_arg args;
	vnode_t *vp, *rvp;
	sam_node_t *ip;
	cred_t *credp;
	int error;
	void *path;

	if (copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}
	path = (void *)args.path.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		path = (void *)args.path.p64;
	}
	if ((error = lookupname(path, UIO_USERSPACE, NO_FOLLOW,
	    NULLVPP, &vp))) {
		return (error);
	}

	if (sam_set_realvp(vp, &rvp)) {		/* true if realvp not SAM-QFS */
		VN_RELE(vp);
		return (ENOTTY);
	}
	ip = SAM_VTOI(rvp);
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

	/*
	 * Only archive and stage operations are allowed on other than regular
	 * files, symbolic links and directories.
	 */
#if !defined(DIRLINK)
	if (!S_ISREG(ip->di.mode) && !S_ISDIR(ip->di.mode) &&
	    !S_ISLNK(ip->di.mode) &&
	    cmd != SC_archive && cmd != SC_stage && cmd != SC_cancelstage) {
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		VN_RELE(vp);
		return (EINVAL);
	}
#endif

	/*
	 * Only the owner or superuser may perform file operations.
	 * Except - owner cannot do "archive -n" or "release -n"
	 * non-owner with "r" or "x" permission can stage.
	 */
	credp = CRED();
	if (cmd != SC_stage && secpolicy_vnode_owner(credp, ip->di.uid)) {
		error = EPERM;
	} else if (cmd != SC_stage && ip->mp->mt.fi_mflag & MS_RDONLY) {
		/* If read-only filesystem */
		error = EROFS;
	} else {
do_stage:
		if ((cmd == SC_stage) && SAM_IS_SHARED_SERVER(ip->mp)) {
			sam_open_operation_nb(ip->mp);
			error = 0;
		} else {
			error = sam_open_operation(ip);
		}
		if (!error) {
			if (S_ISSEGI(&ip->di)) {
				sam_callback_segment_t callback;

				callback.p.syscall.func =
				    sam_proc_file_operations;
				callback.p.syscall.cmd = cmd;
				callback.p.syscall.args = &args;
				callback.p.syscall.credp = credp;
				error = sam_callback_segment(ip,
				    CALLBACK_syscall, &callback,
				    TRUE);
			}
			if (error == 0) {
				error = sam_proc_file_operations(rvp, cmd,
				    &args, credp);
				if ((cmd == SC_stage) &&
				    SAM_IS_SHARED_SERVER(ip->mp) &&
				    (error == EREMCHG)) {
					SAM_CLOSE_OPERATION(ip->mp, error);
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
					if ((error =
					    sam_open_operation(ip)) == 0) {
						RW_LOCK_OS(&ip->inode_rwl,
						    RW_WRITER);
						SAM_CLOSE_OPERATION(ip->mp,
						    error);
						goto do_stage;
					}
					RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				} else {
					SAM_CLOSE_OPERATION(ip->mp, error);
				}
			} else {
				SAM_CLOSE_OPERATION(ip->mp, error);
			}
		}
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	VN_RELE(vp);
	return (error);
}


/*
 * ----- Process the archive, stage, release, csum, setfa calls.
 */

int		/* ERRNO if error, 0 if successful. */
sam_proc_file_operations(vnode_t *vp, int cmd, void *args, cred_t *credp)
{
	sam_node_t *ip;
	struct sam_fileops_arg *ap = (struct sam_fileops_arg *)args;
	char ops[SAM_MAX_OPS_LEN];
	size_t opslen;
	int error;
	void *ptr;

	ip = SAM_VTOI(vp);
	ptr = (void *)ap->ops.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		ptr = (void *)ap->ops.p64;
	}
	if (copyinstr((caddr_t)ptr, ops, sizeof (ops), &opslen)) {
		return (EFAULT);
	}
	if (opslen >= sizeof (ops)) {
		return (E2BIG);
	}
	ops[sizeof (ops)-1] = '\0';

process_stage:
	if (SAM_IS_SHARED_CLIENT(ip->mp)) {
		sam_inode_samattr_t samattr;

		samattr.cmd = cmd;
		strncpy(samattr.ops, ops, sizeof (samattr.ops));
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		error = sam_proc_inode(ip, INODE_samattr, &samattr, credp);
		if ((cmd == SC_stage) && (ip->stage_err == EREMCHG)) {
			error = sam_idle_operation(ip);
			if (error == 0) {
				delay(hz);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				goto process_stage;
			}
		}
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	} else {
		error = sam_set_file_operations(ip, cmd, ops, credp);
	}
	return (error);
}


/*
 * Unarchive, exarchive, damage, undamage, rearch, or unrearch
 * an archive copy syscall.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_archive_copy(void *arg)
{
	struct sam_archive_copy_arg args;
	vnode_t *vp, *rvp;
	sam_node_t *ip;
	cred_t *credp;
	int error;
	void *path;

	if (copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}
	path = (void *)args.path.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		path = (void *)args.path.p64;
	}
	if ((error = lookupname((char *)path, UIO_USERSPACE, NO_FOLLOW,
	    NULLVPP, &vp))) {
		return (error);
	}

	if (sam_set_realvp(vp, &rvp)) {		/* true if realvp not SAM-QFS */
		VN_RELE(vp);
		return (ENOTTY);
	}
	ip = SAM_VTOI(rvp);
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

	/*
	 * Only the owner or superuser may unarchive a file.
	 */
	error = 0;
	credp = CRED();
	if (secpolicy_vnode_owner(credp, ip->di.uid)) {
		error = EPERM;
	} else if (ip->mp->mt.fi_mflag & MS_RDONLY) {
		/* If read-only filesystem */
		error = EROFS;
	} else {
		if (S_ISSEGI(&ip->di) && !(args.flags & SU_meta)) {
			sam_callback_segment_t callback;

			callback.p.syscall.func = sam_proc_archive_copy;
			callback.p.syscall.cmd = SC_archive_copy;
			callback.p.syscall.args = &args;
			callback.p.syscall.credp = credp;
			error = sam_callback_segment(ip, CALLBACK_syscall,
			    &callback, TRUE);
		} else {
			if (
			    (!(args.flags & SU_meta) &&
			    (S_ISREG(ip->di.mode) || S_ISLNK(ip->di.mode))) ||
			    ((args.flags & SU_meta) &&
			    (!(S_ISREG(ip->di.mode) || S_ISLNK(ip->di.mode)) ||
			    S_ISSEGI(&ip->di)))) {
				if ((error = sam_open_operation(ip)) == 0) {
					if (SAM_IS_SHARED_CLIENT(ip->mp)) {
						struct sam_archive_copy_arg *ap;
						sam_inode_samarch_t samarch;

				ap = (struct sam_archive_copy_arg *)&args;
						samarch.operation =
						    ap->operation;
						samarch.flags = ap->flags;
						samarch.media = ap->media;
						samarch.copies = ap->copies;
						samarch.ncopies = ap->ncopies;
						samarch.dcopy = ap->dcopy;
						strncpy(samarch.vsn, ap->vsn,
						    sizeof (vsn_t));
						RW_UNLOCK_OS(&ip->inode_rwl,
						    RW_WRITER);
						error = sam_proc_inode(ip,
						    INODE_samarch, &samarch,
						    credp);
						RW_LOCK_OS(&ip->inode_rwl,
						    RW_WRITER);
					} else {
						error = sam_proc_archive_copy(
						    rvp, SC_archive_copy,
						    &args, credp);
					}
					SAM_CLOSE_OPERATION(ip->mp, error);
				}
			}
		}
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	VN_RELE(vp);
	return (error);
}


/*
 * ----- sam_read_rminfo - Process the sam read removable media system call.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_read_rminfo(void *arg)
{
	struct sam_readrminfo_arg args;
	vnode_t *vp, *rvp;
	sam_node_t *ip;
	int error;
	void *path;
	void *buf;

	if (copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}
	path = (void *)args.path.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		path = (void *)args.path.p64;
	}
	if ((error = lookupname((char *)path, UIO_USERSPACE, NO_FOLLOW,
	    NULLVPP, &vp))) {
		return (error);
	}

	if (sam_set_realvp(vp, &rvp)) {		/* true if realvp not SAM-QFS */
		VN_RELE(vp);
		return (ENOTTY);
	}
	buf = (void *)args.buf.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		buf = (void *)args.buf.p64;
	}
	ip = SAM_VTOI(rvp);
	RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	error = sam_readrm(ip, (char *)buf, args.bufsize);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	VN_RELE(vp);
	return (error);
}


/*
 * ----- sam_readrm - Return removable media information.
 */

int				/* ERRNO if error, 0 if successful. */
sam_readrm(
	sam_node_t *ip,		/* Pointer to inode. */
	char *buf,		/* Pointer to rminfo buffer */
	int bufsize)		/* Size of rminfo */
{
	int error;

	if (!S_ISREQ(ip->di.mode)) {
		return (ENOENT);
	}
	if (bufsize < sizeof (struct sam_rminfo)) {
		return (EINVAL);
	}

	if ((error = sam_open_operation(ip))) {
		return (error);
	}
	error = sam_read_rm(ip, buf, bufsize);
	SAM_CLOSE_OPERATION(ip->mp, error);
	return (error);
}


/*
 * ----- sam_read_rm - Get removable media information.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_read_rm(
	sam_node_t *ip,	/* Pointer to inode. */
	char *buf,	/* Pointer to rminfo buffer */
	int bufsize)	/* Size of rminfo */
{
	int error = 0;
	buf_t *bp;
	sam_resource_file_t *rfp;
	sam_id_t eid;
	sam_id_t next_id;
	struct sam_inode_ext *eip;
	struct sam_rminfo rb;
	char *p;
	int media, n;

	if (ip->di.version >= SAM_INODE_VERS_2) {
		ASSERT(ip->di.ext_attrs & ext_rfa);

		/* Removable media info stored as inode extension(s) */
		if (error = sam_access_ino(ip, S_IREAD, TRUE, CRED())) {
			return (error);
		}

		if (ip->di.ext_id.ino == 0) {	/* No ino exts is bad */
			sam_req_ifsck(ip->mp, -1, "sam_read_rm: no ext",
			    &ip->di.id);
			return (ENOCSI);
		}

		next_id.ino = 0;
		eid = ip->di.ext_id;
		while (eid.ino) {
			if (error = sam_read_ino(ip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip)) {
				return (error);
			}
			if (EXT_HDR_ERR(eip, eid, ip)) {
				sam_req_ifsck(ip->mp, -1,
				    "sam_read_rm: EXT_HDR", &ip->di.id);
				brelse(bp);
				return (ENOCSI);
			}
			if (S_ISRFA(eip->hdr.mode)) {
				/* save for vsns copy below */
				next_id = eip->hdr.next_id;
				rfp = &eip->ext.rfa.info;
				break;
			}
			eid = eip->hdr.next_id;
			brelse(bp);
		}
		if (eid.ino == 0) {
			sam_req_ifsck(ip->mp, -1, "sam_read_rm: eid.ino = 0",
			    &ip->di.id);
			return (ENOCSI);
		}
	} else {
		if ((error = sam_read_old_rm(ip, &bp))) {
			return (error);
		}
		rfp = (sam_resource_file_t *)(void *)bp->b_un.b_addr;
	}

	bzero((caddr_t)&rb, sizeof (rb));
	rb.position = ip->di.rm.info.rm.position;
	rb.creation_time = rfp->resource.archive.creation_time;
	rb.flags = (ip->di.rm.ui.flags & RM_BUFFERED_IO) ?
	    RI_bufio : RI_blockio;
	rb.block_size = rfp->resource.archive.rm_info.mau;

	/*
	 * Set media mnemonic.
	 */
	media = rfp->resource.archive.rm_info.media;
	n = media & DT_MEDIA_MASK;
	if ((media & DT_CLASS_MASK) == DT_OPTICAL) {
		strncpy(rb.file_id, rfp->resource.archive.mc.od.file_id,
		    sizeof (rb.file_id)-1);
		rb.version = rfp->resource.archive.mc.od.version;
		strncpy(rb.owner_id, rfp->resource.mc.od.owner_id,
		    sizeof (rb.owner_id)-1);
		strncpy(rb.group_id, rfp->resource.mc.od.group_id,
		    sizeof (rb.group_id)-1);
		bcopy(rfp->resource.mc.od.info, rb.info, sizeof (rb.info));
		p = (char *)dev_nmod[n];
	} else if ((media & DT_CLASS_MASK) == DT_TAPE) {
		p = (char *)dev_nmtp[n];
	} else  p = "??";
	rb.media[0] = *p++;
	rb.media[1] = *p;

	/* Copy removable media info including vsn section(s) to user */
	rb.n_vsns = rfp->n_vsns;
	rb.c_vsn = rfp->cur_ord;
	bcopy((caddr_t)&rfp->section[0], (caddr_t)&rb.section[0],
	    sizeof (struct sam_section));
	if (copyout((caddr_t)&rb, (caddr_t)buf, sizeof (struct sam_rminfo))) {
		brelse(bp);
		return (EFAULT);
	}
	if (rb.n_vsns == 1) {
		brelse(bp);
	} else {
		struct sam_rminfo *bufp = (struct sam_rminfo *)(void *)buf;

		bufsize -= sizeof (struct sam_rminfo);
		if (bufsize > (sizeof (struct sam_section) * (rb.n_vsns - 1))) {
			bufsize = (sizeof (struct sam_section) *
			    (rb.n_vsns - 1));
		} else if (bufsize < (sizeof (struct sam_section) *
		    (rb.n_vsns - 1))) {
			brelse(bp);
			return (EOVERFLOW);
		}

		if (ip->di.ext_attrs & ext_rfa) {
			int vsns, ino_vsns, size, ino_size;
			struct sam_rfv_inode *rfv;

			brelse(bp);		/* done with 1st extension */

			/* Removable media info stored as inode extension(s) */
			vsns = 1;
			size = 0;

			eid = next_id;
			while (eid.ino && (size < bufsize)) {
				if (error = sam_read_ino(ip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip)) {
					break;
				}
				if (EXT_HDR_ERR(eip, eid, ip) ||
				    EXT_MODE_ERR(eip, S_IFRFA)) {
					sam_req_ifsck(ip->mp, -1,
					    "sam_read_rm: EXT_HDR/EXT_MODE",
					    &ip->di.id);
					error = ENOCSI;
				} else {
					rfv = &eip->ext.rfv;
					if ((rfv->ord != vsns) ||
					    (rfv->n_vsns == 0) ||
					    (rfv->n_vsns > MAX_VSNS_IN_INO)) {
						error = EDOM;
					} else {



	/* N.B. Bad indentation here to meet cstyle requirements */
	ino_vsns = rfv->n_vsns;
	ino_size = sizeof (struct sam_section)	* ino_vsns;
	if ((size + ino_size) >
	    bufsize) {
		ino_size = bufsize -
		    size;
		error = EOVERFLOW;
	}
	if (ino_size > 0) {
		if (copyout(
		    (caddr_t)&rfv->vsns.section[0],
		    (caddr_t)&bufp->section[vsns],
		    ino_size)) {
			error = EFAULT;
		}
	}



					}
				}
				vsns += ino_vsns;
				size += ino_size;

				eid = eip->hdr.next_id;
				brelse(bp);
				if (error)  break;
			}

		} else {

			/* Removable media info stored as meta data block */
			if (copyout((caddr_t)&rfp->section[1],
			    (caddr_t)&bufp->section[1], bufsize)) {
				error = EFAULT;
			}
			brelse(bp);
		}
	}
	return (error);
}


/*
 * ----- sam_set_csum - Process the SAM-QFS ssum system call.
 */

static int
sam_set_csum(void *arg)
{
	struct sam_set_csum_arg args;
	vnode_t *vp, *rvp;
	sam_node_t *ip;
	sam_mount_t *mp;
	buf_t *bp;
	struct sam_perm_inode *permip;
	cred_t *credp;
	int error;
	void *path;

	if (copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}
	path = (void *)args.path.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		path = (void *)args.path.p64;
	}
	if ((error = lookupname((char *)path, UIO_USERSPACE, NO_FOLLOW,
	    NULLVPP, &vp))) {
		return (error);
	}

	if (sam_set_realvp(vp, &rvp)) {		/* true if realvp not SAM-QFS */
		VN_RELE(vp);
		return (ENOTTY);
	}
	ip = SAM_VTOI(rvp);
	if ((error = sam_open_operation(ip))) {
		return (error);
	}

	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	credp = CRED();
	if (secpolicy_vnode_owner(credp, ip->di.uid)) {
		error = EPERM;
		goto out;
	}
	if (ip->mp->mt.fi_mflag & MS_RDONLY) {	/* If read-only filesystem */
		error = EROFS;
		goto out;
	}
	if (error = sam_read_ino(ip->mp, ip->di.id.ino, &bp, &permip)) {
		goto out;
	}
	/*
	 * Setting the checksum value implies setting csum_gen.
	 */
	ip->di.status.b.cs_gen = 1;
	ip->di.status.b.cs_val = 1;
	ip->di.cs_algo = CS_SIMPLE;
	permip->csum = args.csum;
	if (TRANS_ISTRANS(ip->mp)) {
		TRANS_WRITE_DISK_INODE(ip->mp, bp, permip, ip->di.id);
	} else {
		bdwrite(bp);
	}
	TRANS_INODE(ip->mp, ip);
	ip->flags.b.changed = 1;
	error = 0;

out:
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	mp = ip->mp;
	VN_RELE(vp);
	SAM_CLOSE_OPERATION(mp, error); /* can't use ip->mp since vp released */
	return (error);
}


/*
 * ----- sam_set_realvp - Get rvp if VN_REALVP indicates.
 *
 *	sam_set_realvp performs the needed conversions, if the vnode returned
 *	via lookupname is a vnode such as lofs. The vnode returned via
 *	lookupname has the vnode hold against it. The returned vnode (*rvp)
 *	contains the SAM-QFS vnode, if and only if the return value is
 *	zero.
 *
 *	Return state of SAM_VNODE_IS_NOTSAMFS macro on resultant vnode.
 */

int
sam_set_realvp(vnode_t *vp, vnode_t **rvp)
{
	vnode_t	*realvp;

	if (VOP_REALVP_OS(vp, &realvp, NULL) == 0) {
		*rvp = realvp;
	} else {
		*rvp = vp;
	}
	return (SAM_VNODE_IS_NOTSAMFS(*rvp));
}


/*
 * ----- sam_stage_file - Stage in file.
 *	Note: inode_rwl WRITER lock set on entry and exit.
 */

int				/* Error number */
sam_stage_file(
	sam_node_t *ip,		/* Pointer to inode */
	int wait,		/* Wait for completion */
	int copy,		/* Archive copy requested */
	cred_t *credp)		/* Credentials pointer */
{
	int error;
	offset_t length;

	/* must be owner or superuser, or other with r or x permission */
	if (S_ISSEGS(&ip->di)) {
		sam_node_t *bip = ip->segment_ip;

		if (sam_access_ino(bip, S_IREAD, FALSE, credp) &&
		    sam_access_ino(bip, S_IEXEC, FALSE, credp)) {
			return (EACCES);
		}
	} else {
		if (sam_access_ino(ip, S_IREAD, TRUE, credp) &&
		    sam_access_ino(ip, S_IEXEC, TRUE, credp)) {
			return (EACCES);
		}
	}

	if (!ip->di.status.b.offline || (ip->flags.b.staging && !wait)) {
		return (0);
	}

	/*
	 * Select copy to stage from.
	 */
	if (copy != 0) {
		copy--;
		if ((ip->di.arch_status & (1 << copy)) == 0) {
			/* No archive copy */
			return (ENXIO);
		}
		ip->copy = (char)copy;
	} else {
		if (ip->di.arch_status == 0) {	/* If no archive copy */
			return (ENXIO);
		}
		ip->copy = 0;
	}

	if (wait) {
		if (ip->flags.b.stage_p) {
			length = sam_partial_size(ip);
		} else {
			length = ip->di.rm.size;
		}
	} else {
		length = 0;
	}

	/*
	 * Override direct access.
	 */
	ip->flags.b.stage_n = 0;
	error = sam_proc_offline(ip, (offset_t)0, (offset_t)length, NULL, credp,
	    NULL);
	if (!ip->flags.b.staging) {
		ip->flags.b.stage_n = ip->di.status.b.direct;
	}
	return (error);
}
#endif /* sun */


/*
 * ----- sam_get_multivolume - get the multivolume archive info.
 *
 * Return the vsn list from the inode extension(s) for requested copy.
 *
 * Caller must have the inode read lock held.  This lock is not released.
 */

int					/* ERRNO, else 0 if successful. */
sam_get_multivolume(
	sam_node_t *bip,		/* Pointer to base inode. */
	struct sam_vsn_section **vsnpp,	/* vsn section array */
	int copy,			/* Copy number, 0..(MAX_ARCHIVE-1) */
	int *size)			/* size of buffer (updated) */
{
	boolean_t				buf_oflow;
#ifdef sun
	buf_t					*bp;
#endif /* sun */
#ifdef linux
	char					*bp;
#endif /* linux */
	int						error;
	int						ino_size;
	int						ino_vsns;
	int						vsns;
	sam_id_t				eid;
	struct sam_inode_ext *eip;

	error	  = 0;
	buf_oflow = B_FALSE;
	vsns	  = 0;

#ifdef linux
	if ((bp = kmem_alloc(sizeof (struct sam_perm_inode),
	    KM_SLEEP)) == NULL) {
		return (ENOMEM);
	}
#endif /* linux */

	/* Retrieve vsns for requested copy. */
	eid = bip->di.ext_id;
	while (eid.ino && (*size > 0)) {
#ifdef sun
		error = sam_read_ino(bip->mp, eid.ino, &bp,
		    (struct sam_perm_inode **)&eip);
#endif /* sun */
#ifdef linux
		error = sam_read_disk_ino(bip->mp, &eid, bp);
		eip = (struct sam_inode_ext *)bp;
		ASSERT(bip->di.version >= SAM_INODE_VERS_2);
#endif /* linux */
		if (error) {
			break;
		}

		if (EXT_HDR_ERR(eip, eid, bip)) {
#ifdef sun
			sam_req_ifsck(bip->mp, -1,
			    "sam_get_multivolume: EXT_HDR", &bip->di.id);
			brelse(bp);
#endif /* sun */
			error = ENOCSI;
			break;
		}

		if (S_ISMVA(eip->hdr.mode)) {
			if (bip->di.version >= SAM_INODE_VERS_2) {
				if (copy == eip->ext.mva.copy) {
					ino_vsns = eip->ext.mva.n_vsns;
					if ((ino_vsns == 0) ||
					    (ino_vsns > MAX_VSNS_IN_INO)) {
						error = ENOCSI;
#ifdef sun
						brelse(bp);
#endif /* sun */
						break;
					}
					if (ino_vsns == 1) {
						/*
						 * In 4.0, a logic error as
						 * reported, some
						 * overflow inodes would only
						 * have one vsn (the original).
						 * Suppress reporting of these
						 * single vsn extensions.
						 */
						error = 0;
#ifdef sun
						brelse(bp);
						/*
						 * Attempt to correct the "bad"
						 * extension.  If we can't lock
						 * it, wait until next time.  *
						 */
						/*
						 * Note: Linux does not
						 * have an upgrade for locks.
						 */
						if (RW_TRYUPGRADE_OS(
						    &bip->inode_rwl)) {
							/*
							 * eid receives the id
							 * of the next
							 * extension.  The
							 * continue allows that
							 * extension to be
							 * processed.
							 */
							sam_fix_mva_inode_ext(
							    bip, copy, &eid);
							RW_DOWNGRADE_OS(
							    &bip->inode_rwl);
							continue;
						}
#endif /* sun */
					}

					ino_size = ino_vsns *
					    sizeof (struct sam_vsn_section);

	/* N.B. Bad indentation here to meet cstyle requirements */
	if (ino_size > *size) {
		buf_oflow = B_TRUE;
		ino_vsns  = *size /
		    sizeof (struct sam_vsn_section);
		ino_size  = ino_vsns *
		    sizeof (struct sam_vsn_section);
	}

	if (ino_size > 0) {
		if (copyout(
		    (caddr_t)&eip->ext.mva.vsns.section[0],
		    (caddr_t)*vsnpp,
		    ino_size)) {
			error = EFAULT;
#ifdef sun
			brelse(bp);
#endif /* sun */
			break;
		}

		*vsnpp += ino_vsns;
		*size  -= ino_size;
		vsns   += ino_vsns;
	}

					if (buf_oflow) {
#ifdef sun
						brelse(bp);
#endif /* sun */
						break;
					}
				}
#ifdef sun
			} else if (bip->di.version == SAM_INODE_VERS_1) {
				/* Prev vers */
				if (copy == eip->ext.mv1.copy) {
					ino_vsns = eip->ext.mv1.n_vsns;

					if ((ino_vsns == 0) ||
					    (ino_vsns > MAX_VSNS_IN_INO)) {
						sam_req_ifsck(bip->mp, -1,
						    "sam_get_multivolume: vsns",
						    &bip->di.id);
						brelse(bp);
						error = ENOCSI;
						break;
					}

					ino_size = ino_vsns *
					    sizeof (struct sam_vsn_section);

	/* N.B. Bad indentation here to meet cstyle requirements */
	if (ino_size > *size) {
		buf_oflow = B_TRUE;
		ino_vsns = *size /
		    sizeof (struct sam_vsn_section);
		ino_size = ino_vsns *
		    sizeof (struct sam_vsn_section);
	}

	if (ino_size > 0) {
		if ((copyout(
		    (caddr_t)&eip->ext.mv1.vsns.section[0],
		    (caddr_t)*vsnpp,
		    sizeof (struct sam_vsn_section) * ino_vsns))) {
			error = EFAULT;
			brelse(bp);
			break;
		}

		*vsnpp += ino_vsns;
		*size  -= ino_size;
		vsns   += ino_vsns;
	}

					if (buf_oflow) {
						brelse(bp);
						break;
					}
				}
#endif /* sun */
			}
		}
		eid = eip->hdr.next_id;
#ifdef sun
		brelse(bp);
#endif /* sun */
	}
#ifdef linux
	kfree(bp);
#endif /* linux */

	if (buf_oflow && error == 0) {
		error = EOVERFLOW;
	}

#ifdef sun
	TRACE(T_SAM_GET_MVA_EXT, SAM_ITOV(bip), copy, vsns, error);
#endif /* sun */
#ifdef linux
	TRACE(T_SAM_GET_MVA_EXT, bip, copy, vsns, error);
#endif /* linux */

	return (error);
}


/*
 * ----- sam_set_projid - set the project ID of a file
 *
 */
#ifdef sun
int
sam_set_projid(void *arg, int size)
{
	int follow, error = 0;
	struct sam_projid_arg args;
	char *path;
	vnode_t *vp, *rvp;
	sam_node_t *ip;
	sam_mount_t *mp;
	int issync;
	int trans_size;
	int terr = 0;
	sam_inode_samaid_t sa;
	cred_t *credp = CRED();

	/*
	 * Validate and copy in the arguments.
	 */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	path = (void *)args.path.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		path = (void *)args.path.p64;
	}

	follow = args.follow ? FOLLOW : NO_FOLLOW;
	if ((error = lookupname((char *)path, UIO_USERSPACE, follow,
	    NULLVPP, &vp))) {
		return (error);
	}

	if (sam_set_realvp(vp, &rvp)) {
		error = ENOTTY;
		goto out;
	}

	ip = SAM_VTOI(rvp);
	mp = ip->mp;

	if (ip->di2.projid == args.projid) {		/* no-op */
		goto out;
	}

	/*
	 * Solaris user credentials do not include a user's set of authorized
	 * project IDs.  That information is contained only within the file
	 * /etc/project.
	 *
	 * Rather than make QFS cache that project information for every user,
	 * to support policy enforcement here, it would be nice if Solaris some
	 * day  did that in a more generic way for use by every file system
	 * type.
	 *
	 * Until then, we enforce at user-level via a set-user-ID-root
	 * application (/opt/SUNWsamfs/bin/chproj) the requirement that the
	 * user must belong to the Solaris project that is being set on
	 * the file/directory.  This interface is privileged-only to prevent
	 * an unprivileged application from bypassing that policy.
	 *
	 * We also enforce the policy at user-level that the user must own
	 * the file/directory whose project ID is being set, or must be
	 * the (real) root user.  While that policy could instead be enforced
	 * here, it is enforced at user-level to maintain the consistency
	 * of the QFS secpolicy() implementation.
	 *
	 * If/when Solaris credentials carry the necessary project ID
	 * information, this interface can become available to privilege
	 * unaware processes, with all policy enforcement performed here,
	 * and the user-level application can be installed as a
	 * privilege-unaware application.
	 */
	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EPERM;
		goto out;
	}

	if ((error = sam_open_operation(ip)) != 0) {
		goto out;
	}

	if (SAM_IS_SHARED_CLIENT(mp)) {

		bzero((char *)&sa, sizeof (sa));
		sa.operation = SAM_INODE_PROJID;
		sa.aid = args.projid;

		error = sam_proc_inode(ip, INODE_samaid, &sa, credp);

	} else {

		trans_size = (int)TOP_SETATTR_SIZE(ip);
		TRANS_BEGIN_CSYNC(ip->mp, issync, TOP_SETATTR, trans_size);

		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

		ip->di2.projid = args.projid;

		TRANS_INODE(ip->mp, ip);
		sam_mark_ino(ip, SAM_CHANGED);

		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

		TRANS_END_CSYNC(ip->mp, terr, issync,
		    TOP_SETATTR, trans_size);

		if (error == 0) {
			error = terr;
		}
	}
	SAM_CLOSE_OPERATION(mp, error);

out:
	VN_RELE(vp);
	return (error);
}
#endif
