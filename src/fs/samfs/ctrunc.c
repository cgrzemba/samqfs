/*
 * ----- sam/ctrunc.c - Process the sam common truncate functions.
 *
 * Processes the common truncate functions supported on the SAM File System.
 *
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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
#pragma ident "$Revision: 1.29 $"
#endif

#include "sam/osversion.h"

/*
 * ----- UNIX Includes
 */

#ifdef sun
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/cmn_err.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/flock.h>
#include <sys/fs_subr.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <vm/pvn.h>
#endif /* sun */

#ifdef linux
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/dirent.h>
#include <linux/proc_fs.h>
#include <linux/limits.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/skbuff.h>
#include <inode.h>
#include <clextern.h>

#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#endif /* linux */

/*
 * ----- SAMFS Includes
 */

#include <sam/types.h>

#include "macros.h"
#include "inode.h"
#include "mount.h"
#include "ino_ext.h"
#include "segment.h"
#include "rwio.h"
#include "indirect.h"
#ifdef sun
#include "extern.h"
#endif /* sun */
#ifdef linux
#include "clextern.h"
#endif /* linux */
#include "trace.h"
#include "debug.h"
#include "qfs_log.h"

/*
 * ----- sam_clear_file - clear the file.
 * length = 0, if staging, cancel stage and wait until stage terminates.
 * length > 0, if offline, stage file and wait until file is online.
 * Flag = PURGE_FILE	- truncate to 0 and remove inode.
 * Flag = STALE_ARCHIVE - truncate to length and stale archive copies.
 * Flag = MAKE_ONLINE   - do not truncate.
 * Note: inode_rwl WRITER lock set on entry and exit.
 */

int					/* ERRNO if error, 0 if successful. */
sam_clear_file(
	sam_node_t *ip,			/* pointer to inode. */
	offset_t length,		/* Truncate length */
	enum sam_clear_ino_type flag,	/* archive/truncate flag */
	cred_t *credp)			/* credentials pointer. */
{
	int error = 0;

#ifdef sun
	TRANS_INODE(ip->mp, ip);

	/*
	 * We can't just use S_ISSEGI() here because we could be doing a
	 * truncate extend on a file which hasn't been segmented yet.
	 */
	if (!S_ISSEGI(&ip->di) && !S_ISSEGS(&ip->di) &&
	    ip->di.status.b.segment &&
	    (length > SAM_SEGSIZE(ip->di.rm.info.dk.seg_size))) {
		sam_node_t *sip;
		/*
		 * File not yet segmented.  Create the first data segment.
		 */
		if ((error = sam_get_segment_ino(ip, 0, &sip))) {
			return (error);
		}
		ip->segment_ip = sip;
		ip->segment_ord = 0;
	}
	if (S_ISSEGI(&ip->di)) {
		sam_callback_segment_t callback;

		/*
		 * Free cached segment inode.
		 * If truncating segment (STALE_ARCHIVE), purge it.
		 */
		if (ip->segment_ip) {
			SAM_SEGRELE(SAM_ITOV(ip->segment_ip));
			ip->segment_ip = NULL;
		}
		callback.p.clear.length = length;
		callback.p.clear.cflag = flag;
		callback.p.clear.credp = credp;
		error = sam_callback_segment(ip, CALLBACK_clear,
		    &callback, TRUE);
		if (error == 0) {
			if (flag == PURGE_FILE) {
				error = sam_clear_ino(ip, length, flag, credp);
			} else if (flag == STALE_ARCHIVE) {
				ip->di.rm.size = length;
			}
		}
	} else {
		error = sam_clear_ino(ip, length, flag, credp);
	}
#endif /* sun */
#ifdef linux
	error = sam_clear_ino(ip, length, flag, credp);
#endif /* linux */
	return (error);
}


/*
 * ----- sam_clear_ino - clear the inode.
 * length = 0, if staging, cancel stage and wait until stage terminates.
 * length > 0, if offline, stage file and wait until file is online.
 * Flag = PURGE_FILE	- truncate to 0 and remove inode.
 * Flag = STALE_ARCHIVE	- truncate to length and stale archive copies.
 * Flag = MAKE_ONLINE	- do not truncate.
 * Note: inode_rwl WRITER lock set on entry and exit.
 */

int					/* ERRNO if error, 0 if successful. */
sam_clear_ino(
	sam_node_t *ip,			/* pointer to inode. */
	offset_t length,		/* Truncate length */
	enum sam_clear_ino_type flag,	/* archive/truncate flag */
	cred_t *credp)			/* credentials pointer. */
{
	int error = 0;

	for (;;) {
		if ((length == 0) ||
		    (ip->di.status.b.pextents &&
		    (length <= sam_partial_size(ip)))) {
			/*
			 * Don't stage when truncating to 0 or truncating to
			 * partial and the partial extents are online.  If
			 * currently staging, cancel the stage.
			 */
			if (ip->flags.b.staging) {
				(void) sam_cancel_stage(ip, ECANCELED, credp);
			}
		} else {
			/*
			 * If length > 0 or length > partial and file is
			 * offline, make it online.
			 */
			if (ip->di.status.b.offline) {
				/*
				 * Wait for any direct access staging to
				 * complete, then remove direct access and stage
				 * the entire file.
				 */
				if (ip->di.status.b.direct) {
					while (ip->flags.b.staging) {
						if ((error = sam_wait_rm(ip,
						    SAM_STAGING))) {
							break;
						}
					}
					if ((error == 0) &&
					    ip->di.status.b.offline) {
						ip->size = 0;
					}
				}
				/*
				 * If offline, stage the entire file.  Wait
				 * until stage completes and file is on-line.
				 * If nfs, just return the error and nfs will
				 * resend the cmd.
				 */
				if (error == 0) {
					ip->flags.b.stage_n = 0;
					ip->flags.b.stage_all = 0;
					while (ip->di.status.b.offline) {
						error = sam_proc_offline(ip,
						    (offset_t)0,
						    ip->di.rm.size, NULL,
						    credp, NULL);
						if (error &&
						    IS_SAM_ENOSPC(error)) {
							RW_UNLOCK_OS(
							    &ip->inode_rwl,
							    RW_WRITER);
							error = sam_wait_space(
							    ip, error);
							RW_LOCK_OS(
							    &ip->inode_rwl,
							    RW_WRITER);
						}
						if (error) {
							break;
						}
					}
					if (!ip->flags.b.staging) {
						ip->flags.b.stage_n =
						    ip->di.status.b.direct;
						ip->flags.b.stage_all =
						    ip->di.status.b.stage_all;
					}
					if (error) {
						break;
					}
				}
				sam_flush_pages(ip, 0);
			}
		}
		/*
		 * Synchronously flush dirty stage pages. These dirty pages do
		 * not cause the file to be marked modified while stage_pages is
		 * set.
		 */
		if (ip->flags.b.stage_pages) {
			sam_flush_pages(ip, 0);
		}
		if ((error == 0) && (flag != MAKE_ONLINE)) {
			sam_truncate_t tflag;

			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
			mutex_enter(&ip->cl_apmutex);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
#ifdef sun
			/*
			 * Linux does not use this code path to change
			 * file size so this flush is not needed.
			 */
			if (vn_has_cached_data(SAM_ITOV(ip)) &&
			    !ip->stage_seg) {
				sam_flush_pages(ip, 0);
			}
#endif
			/*
			 * If file staged or released during raised lock,
			 * go back and repeat.
			 */
			if ((length == 0 && ip->flags.b.staging) ||
			    (length != 0 && ip->di.status.b.offline)) {
				mutex_exit(&ip->cl_apmutex);
				RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
				continue;
			}
			if (flag == PURGE_FILE) {
				tflag = SAM_PURGE;
			} else {
				tflag = SAM_TRUNCATE;
			}
			if (!(error = sam_truncate_ino(ip, length, tflag,
			    credp))) {
				if (tflag != SAM_PURGE) {
					ip->di.rm.size = ip->size;
					if (ip->di.rm.size == 0) {
						ip->di.status.b.offline = 0;
						ip->di.status.b.stage_failed =
						    0;
					}
					TRANS_INODE(ip->mp, ip);
					sam_mark_ino(ip,
					    (SAM_UPDATED | SAM_CHANGED));
				}
			}
			mutex_exit(&ip->cl_apmutex);
			RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
		}
		break;
	}
	return (error);
}


/*
 * ----- sam_truncate_ino - truncate the file.
 * If length <= filesize, truncate inode by deleting the blocks in the file.
 * If length > filesize, expand the file by allocating blocks.
 * NOTE. inode_rwl lock set on entry and exit.
 * The inode data lock is set on entry.
 */

int					/* ERRNO if error, 0 if successful. */
sam_truncate_ino(
	sam_node_t *ip,			/* pointer to inode. */
	offset_t length,		/* new length for file. */
	sam_truncate_t tflag,		/* Truncate file or Release file */
	cred_t *credp)			/* credentials pointer. */
{
	int error = 0;

	/*
	 * Shared files must be truncated on the server.  However, if
	 * we're doing truncation as part of destroying a newly unlinked
	 * inode on the server, we don't want to acquire a lease, and it
	 * is safe to perform the truncation directly.
	 */
	if (SAM_IS_SHARED_FS((sam_mount_t *)(ip->mp)) && (tflag != SAM_PURGE)) {
		sam_lease_data_t data;

		bzero(&data, sizeof (data));
		data.ltype = LTYPE_truncate;
		data.lflag = (ushort_t)tflag;
		data.resid = length;
		data.alloc_unit = ip->cl_alloc_unit;
		data.shflags.b.directio = ip->flags.b.directio;
		data.shflags.b.abr = ip->flags.b.abr;
		error = sam_truncate_shared_ino(ip, &data, credp);
#ifdef METADATA_SERVER
	} else {
		ASSERT((!SAM_IS_SHARED_FS(ip->mp)) ||
		    (SAM_IS_SHARED_SERVER(ip->mp)));
		error = sam_proc_truncate(ip, length, tflag, credp);
#endif /* METADATA SERVER */
	}
	return (error);
}


/*
 * ----- sam_partial_size - return partial size
 */

offset_t			/* ERRNO if error, 0 if successful. */
sam_partial_size(sam_node_t *ip)
{
	offset_t length;
	int dt;

	dt = ip->di.status.b.meta;
	if (ip->di.status.b.on_large ||
	    ((ip->di.psize.partial * SAM_DEV_BSIZE) > SM_OFF(ip->mp, dt))) {
		length = howmany(ip->di.psize.partial,
		    LG_DEV_BLOCK(ip->mp, dt)) *
		    LG_BLK(ip->mp, dt);
	} else {
		length = howmany(ip->di.psize.partial,
		    SM_DEV_BLOCK(ip->mp, dt)) *
		    SM_BLK(ip->mp, dt);
	}
	return (MIN(length, ip->di.rm.size));
}
