/*
 * ----- vnsubs.c - Process the client externals that are not supported.
 *
 * Process the SAM-QFS client externals that are not supported.
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

#pragma ident "$Revision: 1.20 $"

#include "sam/osversion.h"


/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/t_lock.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/stat.h>
#include <sys/cred.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/fbuf.h>
#include <sys/thread.h>
#include <sys/proc.h>


/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"
#include "sam/syscall.h"

#include "macros.h"
#include "sblk.h"
#include "inode.h"
#include "mount.h"
#include "debug.h"
#include "validation.h"
#include "rwio.h"
#include "extern.h"
#include "trace.h"


/*
 * -----	sam_ioctl_util_cmd - ioctl archiver utility command.
 *	Called when the archiver/csd issues an ioctl utility command "u" for the
 *	file "/mountpoint/.ioctl, /mountpoint/.inodes, etc. file".
 */

/* ARGSUSED */
int				/* ERRNO, else 0 if successful. */
sam_ioctl_util_cmd(
	sam_node_t *ioctl_ip,	/* Pointer to "/mountpoint/file" inode. */
	int cmd,		/* Command number. */
	int *arg,		/* Pointer to arguments. */
	int *rvp,		/* Returned value pointer */
	cred_t *credp)		/* Credentials pointer. */
{
	return (ENOTSUP);
}


/*
 * -----	sam_ioctl_oper_cmd - ioctl operator utility commands.
 *	Called when the operator issues an ioctl utility command "U" for the
 *	file "/mountpoint/.ioctl, /mountpoint/.inodes, etc. file".
 */

/* ARGSUSED */
int				/* ERRNO, else 0 if successful. */
sam_ioctl_oper_cmd(
	sam_node_t *ioctl_ip,	/* Pointer to "/mountpoint/file" inode. */
	int cmd,		/* Command number. */
	int *arg,		/* Pointer to arguments. */
	int *rvp,		/* Returned value pointer */
	cred_t *credp)		/* Credentials pointer. */
{
	return (ENOTSUP);
}


/*
 * -----	sam_ioctl_sam_cmd - ioctl file command.
 *	Called when the user issues an file "f" ioctl command for
 *	an opened file on the SAM-FS file system.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_ioctl_sam_cmd(
	sam_node_t *ip,		/* Pointer to inode. */
	int	cmd,		/* Command number. */
	int	*arg,		/* Pointer to arguments. */
	int flag,		/* Datamodel */
	int	*rvp,		/* Returned value pointer */
	cred_t *credp)		/* Credentials pointer.	*/
{
	return (ENOTSUP);
}


/*
 * ----- sam_priv_sam_syscall - Process the sam privileged system calls.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_priv_sam_syscall(
	int cmd,		/* Command number. */
	void *arg,		/* Pointer to arguments. */
	int size,
	int *rvp)		/* to return a value */
{
	return (ENOTSUP);
}


/*
 * ----- Process the archive, stage, release, csum, setfa calls.
 */

/* ARGSUSED */
int		/* ERRNO if error, 0 if successful. */
sam_set_file_operations(sam_node_t *ip, int cmd, char *ops, cred_t *credp)
{
	return (ENOTSUP);
}



/*
 * Process unarchive, exarchive, damage, undamage, rearch, or unrearch
 * an archive copy.
 */

/* ARGSUSED */
int		/* ERRNO if error, 0 if successful. */
sam_proc_archive_copy(vnode_t *vp, int cmd, void *args, cred_t *credp)
{
	return (ENOTSUP);
}


/*
 * ----- sam_setup_segment - setup the segment access file for I/O.
 *  Given the index parent inode, stage it if offline.
 *  Called at the beginning of r/w I/O.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_setup_segment(
	sam_node_t *bip,	/* Index segment inode pointer */
	uio_t *uiop,		/* user I/O vector array. */
	enum uio_rw	rw,
	segment_descriptor_t *seg,
	cred_t *credp)		/* credentials pointer */
{
	return (ENOTSUP);
}


/*
 * ----- sam_get_segment - get the segment access file
 *  Given the index parent inode, get the segment access inode.
 *  Called at the beginning of r/w I/O.
 *  NOTE. Segment index inode_rwl lock set on entry. Cleared on exit.
 *  Reaquired in sam_release_segment.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_get_segment(
	sam_node_t *bip,	/* Index segment inode pointer */
	uio_t *uiop,		/* user I/O vector array. */
	enum uio_rw	rw,
	segment_descriptor_t *seg,
	sam_node_t **ipp,
	cred_t *credp)
{
	return (ENOTSUP);
}


/*
 * ----- sam_release_segment - release the segment access file
 *  Given the index parent inode, release the segment access inode.
 *  Update segment index file size;
 *  Called at the end of r/w I/O.
 */

/* ARGSUSED */
void
sam_release_segment(
	sam_node_t *bip,	/* Index segment inode pointer */
	uio_t *uiop,		/* user I/O vector array. */
	enum uio_rw	rw,
	segment_descriptor_t *seg,
	sam_node_t *ip)
{
}


/*
 * ----- sam_call_segment - issue function.
 *  Given the index parent inode, call the func for all segment access inode(s).
 */

/* ARGSUSED */
int					/* ERRNO if error, 0 if successful. */
sam_callback_segment(
	sam_node_t *bip,		/* Index segment inode pointer */
	enum CALLBACK_type type,	/* CALLBACK type */
	sam_callback_segment_t *callback,
	boolean_t write_lock)		/* TRUE if WRITERS lock */
{
	return (ENOTSUP);
}


/*
 * ----- sam_get_segment_ino - get segment data inode.
 *  Given the index parent inode, get a segment access inode.
 *  Allocate an inode if not present.
 *  Returns with a inode with activity count incremented.
 *  Note: index inode_rwl WRITER lock set on entry and exit.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_get_segment_ino(
	sam_node_t *bip,	/* Index segment inode pointer */
	int segment_ord,
	sam_node_t **ipp)
{
	return (ENOTSUP);
}


/*
 * ----- sam_rele_index -  Inactivate the segment file index.
 * If segment is inactive and index held, release index.
 */

/* ARGSUSED */
void
sam_rele_index(sam_node_t *ip)
{
}


/*
 * ----- sam_flush_segment_index - Update segment index on disk.
 *  Given the index parent inode, sync. write index to disk.
 */

/* ARGSUSED */
int					/* ERRNO if error, 0 if successful. */
sam_flush_segment_index(sam_node_t *bip)
{
	return (0);
}


/*
 * ----- sam_request_file - Process the sam request system call.
 *	The removable media information is copied from the user to the removable
 *	media file information inode extension and the size of the file is set
 *	to zero.  File must NOT be opened, or else EBUSY is returned.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_request_file(void *arg)
{
	return (ENOTSUP);
}



/*
 * ----- sam_old_request_file - Process the sam request system call for
 * file systems 3.5.0 and older.
 *
 * The removable media information is copied from the user to the
 * removable media file information extent and the size of the file
 * is set to zero.
 * NOTE: the buffer cache is used to bypass file paging.
 * NOTE: File must NOT be opened, or else EBUSY is returned.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_old_request_file(sam_node_t *ip, vnode_t *vp,
    struct sam_readrminfo_arg args)
{
	return (ENOTSUP);
}


/*
 * ----- sam_read_old_rm - Get removable media information.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_read_old_rm(sam_node_t *ip, buf_t **rbp)
{
	return (ENOTSUP);
}


/*
 * ----- sam_alloc_block - allocate block.
 * Get block from small, large, or meta buffer pool (circular buffer).
 * NOTE: Block returned in extent form, that is, not corrected for
 *	ext_bshift
 * If empty signal thread to fill it.
 */

/* ARGSUSED */
int				/* ERRNO if error occured, 0 if successful. */
sam_alloc_block(
	sam_node_t *ip,		/* Pointer to the inode. */
	int bt,			/* The block type: small (SM) or large (LG). */
	sam_bn_t *bn,		/* Pointer to disk block number (returned). */
	int *ord)		/* Pointer to disk ordinal (returned). */
{
	return (ENOSPC);
}


/*
 * -----	sam_trunc_excess_blocks - truncate excess blocks.
 * Called when the file is closed or inactivated.
 * Do not truncate when the file was preallocated.
 */

/* ARGSUSED */
int
sam_trunc_excess_blocks(sam_node_t *ip)
{
	return (0);
}


/*
 * ----- sam_rmmap_block - Map logical block to rm physical block.
 *  For the given inode and logical byte address: if iop is set, return
 *  the I/O descriptor.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_rmmap_block(
	sam_node_t *ip,	/* Pointer to the inode. */
	offset_t offset,	/* Logical byte offset in file (longlong_t). */
	offset_t count,		/* Requested byte count. */
	sam_map_t flag,		/* SAM_WRITE if writing, */
				/* SAM_READ if reading. */
	sam_ioblk_t *iop)	/* Ioblk array. */
{
	return (EINVAL);
}


/*
 * ----- sam_rm_direct_io - Read/Write a SAM-FS file using raw I/O.
 * WRITE: Move the data from the caller's buffer to the device using
 * the device driver cdev_write call.
 * READ: Move the data from the device to the caller's buffer using
 * the device driver cdev_read call.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_rm_direct_io(
	sam_node_t *ip,		/* Pointer to vnode */
	enum uio_rw rw,
	uio_t *uiop,		/* Pointer to user I/O vector array. */
	cred_t *credp)		/* credentials pointer. */
{
	return (ENOTSUP);
}


/*
 * ----- sam_request_preallocated - request and wait for preallocation.
 */

/* ARGSUSED */
int				/* ERRNO if error occured, 0 if successful. */
sam_prealloc_blocks(sam_mount_t *mp, sam_prealloc_t *pap)
{
	return (0);
}


/*
 * ----- sam_release_seg - Unlock a SAM-FS staging page.
 * A page may be left locked due to staging, unlock it. Release
 * the staging segment.
 */

/* ARGSUSED */
void
sam_release_seg(sam_node_t *ip)
{
}


/*
 * NEEDS WORK
 * 1. sam_set_acl - update acl OTW.
 */

/*
 * ----- sam_set_acl - Save access control list.
 * Save incore access control list structure as inode extension(s).
 *
 * Caller must have the inode write lock held.  Lock is not released.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful */
sam_set_acl(
	sam_node_t *bip,	/* Pointer to base inode */
	sam_ic_acl_t **aclpp)	/* out: incore ACL structure (updated). */
{
	return (0);
}


/*
 * Request that this filesystem be noted for an fs check.
 * Set status bits in the root slice's superblock, and
 * push them out to disk.
 *
 * Requests may specify that a particular slice be checked,
 * or just that the filesystem be checked (slice == -1).
 * Each slice contains a pair of status bits indicating
 * that either a non-specific or specific request was made
 * to fsck this slice.
 *
 * These bits are cleared by fsck.
 */

/* ARGSUSED */
void
sam_req_fsck(sam_mount_t *mp, int slice, char *msg)
{
}
