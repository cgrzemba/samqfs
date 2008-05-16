/*
 * ----- rmedia.c - Process the sam removable media functions.
 *
 *	Processes the removable media functions supported on the SAM File
 *	System.
 *	1. Open removable media/direct access file -- Mount VSN.
 *	2. Close removable media/direct access file -- Unload VSN.
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

#pragma ident "$Revision: 1.64 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/t_lock.h>
#include <sys/cred.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/ddi.h>
#include <sys/disp.h>
#include <sys/param.h>

/* ----- SAMFS Includes */

#include "pub/rminfo.h"
#include "sam/types.h"
#include "sam/resource.h"

#include "inode.h"
#include "mount.h"
#include "ino_ext.h"
#include "extern.h"
#include "rwio.h"
#include "debug.h"
#include "trace.h"
#include "qfs_log.h"


static int sam_issue_rm(sam_node_t *ip, sam_fs_fifo_ctl_t *fifo_ctl,
	sam_req_t req_type, cred_t *credp);
static int sam_build_amld_cmd(sam_node_t *ip, sam_fs_fifo_t *cp,
	sam_req_t req_type, cred_t *credp);
static int sam_load_rm(sam_node_t *ip, int filemode, cred_t *credp);
static int sam_switch_volume(sam_node_t *ip, offset_t offset, int filemode,
	cred_t *credp);
static int sam_load_next_rm(sam_node_t *ip, int filemode, int ord,
	sam_resource_file_t *rfp, cred_t *credp);
static void sam_set_stage_offset(sam_node_t *ip, u_longlong_t position);
static int sam_get_rm_info(sam_node_t *bip, sam_resource_file_t *rfp,
	cred_t *credp);
static int sam_set_rm_info(sam_node_t *bip, sam_resource_file_t *rfp,
	cred_t *credp);
static int sam_rm_buffered_io(sam_node_t *ip, uio_t *uiop, cred_t *credp);
static int sam_buffer_rmio(sam_node_t *ip, offset_t offset,
	int *bytes_read, cred_t *credp);

/*
 * ----- sam_open_rm - open removable media file.
 *
 *	Open a removable media file.
 *  NOTE. inode_rwl WRITER lock set on entry and exit.
 */

int				/* ERRNO if error, 0 if successful. */
sam_open_rm(
	sam_node_t *ip,		/* Pointer to vnode. */
	int filemode,		/* open file mode. */
	cred_t *credp)		/* Credentials pointer */
{
	int error = 0;

	if (ip->flags.b.rm_opened) {
		/* Treat removable media like an exclusive open */
		error = EBUSY;
	} else {
		ip->flags.b.rm_opened = 1;
		ip->rm_pid = curproc->p_pid;	/* Owner of the mounted media */
		if ((error = sam_load_rm(ip, filemode, credp))) {
			ip->flags.b.rm_opened = 0;
			ip->rm_pid = 0;
			ip->rm_err = 0;
		}
	}
	return (error);
}


/*
 * ----- sam_load_rm - load removable media file.
 *
 * Called when a removable media file is opened.
 * NOTE. inode_rwl WRITER lock set on entry and exit.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_load_rm(
	sam_node_t *ip,		/* pointer to vnode. */
	int filemode,		/* open file mode */
	cred_t *credp)		/* credentials pointer */
{
	int error = 0;
	int rf_size;
	sam_resource_file_t *rfp;
	int n_vsns;
	int ord;

	/*
	 * Cannot append to a removable media file.
	 */
	if (filemode & FAPPEND) {
		return (EINVAL);
	}

	/* Read removable media info */
	rf_size = ip->di.psize.rmfile;
	if (rf_size == 0) {
		return (ENOCSI);
	}
	rfp = (sam_resource_file_t *)kmem_zalloc(rf_size, KM_SLEEP);

	if ((error = sam_get_rm_info(ip, rfp, credp)) == 0) {
		n_vsns = rfp->n_vsns;
		if ((n_vsns == 0) || (n_vsns > MAX_VOLUMES)) {
			error = EDOM;
		} else {
			for (ord = 0; ord < n_vsns; ord++) {
				error = sam_load_next_rm(ip, filemode, ord,
				    rfp, credp);
				if ((error != ENOSPC) ||
				    !(ip->di.rm.ui.flags & RM_VOLUMES)) {
					break;
				}
			}
		}
	}

	kmem_free((void *)rfp, rf_size);
	return (error);
}


/*
 * ----- sam_switch_volume - Volume overflow.
 *
 * Called when a removable media file reaches the end of a volume.
 * Unload the current volume and mount the next volume.
 * NOTE. inode_rwl WRITER lock set on entry and exit.
 */

/* ARGSUSED1 */
static int			/* ERRNO if error, 0 if successful. */
sam_switch_volume(
	sam_node_t *ip,		/* pointer to vnode. */
	offset_t offset,	/* Current offset */
	int filemode,		/* open file mode */
	cred_t *credp)		/* credentials pointer */
{
	int error = 0;
	int rf_size;
	sam_resource_file_t *rfp;
	int n_vsns;
	int ord;
	int eox;

	/* Read removable media info. */
	rf_size = ip->di.psize.rmfile;
	if (rf_size == 0) {
		return (ENOCSI);
	}
	rfp = (sam_resource_file_t *)kmem_zalloc(rf_size, KM_SLEEP);

	/* Get current ord and n_vsns. */
	if (error = sam_get_rm_info(ip, rfp, credp))  goto out;

	n_vsns = rfp->n_vsns;
	if ((n_vsns == 0) || (n_vsns > MAX_VOLUMES)) {
		error = EDOM;
		goto out;
	}
	ord = rfp->cur_ord + 1;
	if (ord < n_vsns) {
		strncpy(rfp->resource.next_vsn, rfp->section[ord].vsn,
		    sizeof (vsn_t)-1);
		strcpy(rfp->resource.prev_vsn, " ");

		/* Update removable media info. */
		if (error = sam_set_rm_info(ip, rfp, credp)) goto out;

		TRACES(T_SAM_SWITCH_VOL, SAM_ITOV(ip),
		    (char *)rfp->resource.next_vsn);

		eox = 1;
	} else {
		eox = 0;
	}
	if (error = sam_unload_rm(ip, filemode, eox, 0, credp)) goto out;
	if (ord >= n_vsns) {
		error = ENOSPC;		/* Return No Space if no more volumes */
		goto out;
	}
	while (ord < n_vsns) {
		error = sam_load_next_rm(ip, filemode, ord, rfp, credp);
		if ((error != ENOSPC) || !(ip->di.rm.ui.flags & RM_VOLUMES)) {
			goto out;
		}
		ord++;
	}

out:
	kmem_free((void *)rfp, rf_size);
	return (error);
}


/*
 * ----- sam_load_next_rm - load next removable media file.
 *
 * Serialize the load requests for this file.
 * Removable media info has been read by the calling routine.
 * Issue load request to assign the next media.
 * Wait until loaded into the drive.
 * NOTE. inode_rwl WRITER lock set on entry and exit.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_load_next_rm(
	sam_node_t *ip,			/* Pointer to inode. */
	int filemode,			/* Open file mode */
	int	ord,			/* Current volume ordinal */
	sam_resource_file_t *rfp,	/* Resource file info from caller */
	cred_t *credp)			/* Credentials pointer */
{
	int error = 0;
	sam_resource_t *brrp;
	sam_resource_t *frrp;
	sam_fs_fifo_ctl_t fifo_ctl;	/* Load media daemon command */
	sam_fs_fifo_t *fifo;

	/* Refresh removable media info. */
	if (error = sam_get_rm_info(ip, rfp, credp))  goto out;

	brrp = &rfp->resource;
	if (filemode & (FCREAT | FTRUNC | FWRITE)) {
		brrp->access = FWRITE;	/* Writing to file */
		if (brrp->archive.rm_info.stranger) {
			error = EINVAL;
			goto out;
		}
		if (ord >= rfp->n_vsns) {
			error = ENOSPC;
			goto out;
		}
		brrp->archive.rm_info.valid = 0;
		brrp->archive.rm_info.size = 0;
		if (ord == 0) {	/* If write for first section, clear fields */
			int i;

			ip->di.rm.size = 0;
			for (i = 0; i < rfp->n_vsns; i++) {
				rfp->section[i].length = 0;
				rfp->section[i].position = 0;
				rfp->section[i].offset = 0;
			}
		}
	} else {
		brrp->access = FREAD;	/* Default is read access */
		if (brrp->archive.rm_info.valid) {

			/*
			 * Skip empty sections.  The VSNs may not have been
			 * written because they may have been full.
			 */
			while (ord < rfp->n_vsns) {
				if (rfp->section[ord].length != 0)  break;
				ord++;
			}
			if (ord >= rfp->n_vsns) {
				error = EIO;
				goto out;
			}
			if (rfp->n_vsns > 1) {
				/* Set size from section array if multivsns */
				brrp->archive.rm_info.size =
				    rfp->section[ord].length;
				brrp->archive.rm_info.position =
				    rfp->section[ord].position;
				brrp->archive.rm_info.file_offset =
				    rfp->section[ord].offset / 512;
			} else {
				brrp->archive.rm_info.size = ip->di.rm.size;
			}
		}
	}
	rfp->cur_ord = ord;		/* Update current vsn ordinal */
	brrp->archive.rm_info.bof_written = 0;
	if (ip->di.rm.ui.flags & RM_VOLUMES) {
		if (ord == 0) { /* If first volume, no prev_vsn */
			strcpy(brrp->prev_vsn, " ");
		} else {		/* If volume switch, set prev_vsn */
			strncpy(brrp->prev_vsn, brrp->archive.vsn,
			    sizeof (vsn_t)-1);
		}
		strncpy(brrp->archive.vsn, rfp->section[ord].vsn,
		    sizeof (vsn_t)-1);
		strcpy(brrp->next_vsn, " ");
	}

	brrp->archive.rm_info.block_io = (ip->di.rm.ui.flags & RM_BLOCK_IO) ?
	    1 : 0;
	brrp->archive.rm_info.buffered_io =
	    (ip->di.rm.ui.flags & RM_BUFFERED_IO) ? 1 : 0;

	/* Move removable media record into fifo */
	fifo = &fifo_ctl.fifo;
	bzero((caddr_t)&fifo_ctl, sizeof (fifo_ctl));
	fifo->cmd = FS_FIFO_LOAD;
	frrp = &fifo->param.fs_load.resource;
	*frrp = *brrp;

	/* Update removable media file */
	if (error = sam_set_rm_info(ip, rfp, credp))  goto out;

	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

	TRACES(T_SAM_FIFO_MT, SAM_ITOV(ip), (char *)frrp->archive.vsn);

	/* Load removable media volume */
	error = sam_issue_rm(ip, &fifo_ctl, SAM_DELAY, credp);
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	if (error)  goto out;
	if ((ip->di.rm.media & DT_CLASS_MASK) == DT_OPTICAL) {
		if (ip->di.rm.info.rm.mau == 0) {
			ASSERT(ip->di.rm.info.rm.mau);
			ip->di.rm.info.rm.mau = 1024;
		}

		/* Because of compiler problems.... */
		sam_set_stage_offset(ip, frrp->archive.rm_info.position);
	}
	if ((ip->di.rm.ui.flags & RM_BOF_WRITTEN) ||
	    !frrp->archive.rm_info.valid ||
	    ((!(ip->di.rm.ui.flags & RM_BOF_WRITTEN)) &&
	    frrp->archive.rm_info.mau == 0)) {
		/*
		 * Update removable media file.
		 */
		brrp->archive = frrp->archive;
		brrp->archive.rm_info.valid = 1;
		/*
		 *  If reading and rm-info's mau value is not set.  Update
		 *  removable media file information with valid mau.
		 */
		if (!(ip->di.rm.ui.flags & RM_BOF_WRITTEN) &&
		    frrp->archive.rm_info.mau == 0) {
			brrp->archive.rm_info.mau = ip->di.rm.info.rm.mau;
		} else {
			brrp->archive.creation_time = SAM_SECOND();
			rfp->section[ord].length = brrp->archive.rm_info.size;
			rfp->section[ord].position =
			    brrp->archive.rm_info.position;
			brrp->archive.rm_info.size = ip->di.rm.size;
		}

		/* Update removable media file with mount info returned. */
		error = sam_set_rm_info(ip, rfp, credp);
	}
out:
	return (error);
}


/*
 * ----- sam_close_rm - close removable media file.
 *
 *	Close a removable media file.
 *  rdev == 0 if removable media file already unloaded by user or daemon
 *  NOTE. inode_rwl WRITER lock set on entry and exit.
 */

int				/* ERRNO if error, 0 if successful. */
sam_close_rm(
	sam_node_t *ip,		/* Pointer to vnode. */
	int filemode,		/* open file mode. */
	cred_t *credp)		/* Credentials pointer */
{
	int error = 0;

	if (ip->rmio_buf)  {
		kmem_free(ip->rmio_buf, ip->di.rm.info.rm.mau);
		ip->rmio_buf = NULL;
	}

	error = ip->rm_err;
	if (ip->rdev) {
		error = sam_unload_rm(ip, filemode, 0, 0, credp);
	} else  {
		while (ip->flags.b.unloading) {
			if ((error = sam_wait_rm(ip, SAM_RM_OPENED))) {
				break;
			}
		}
		if (error == 0) {
			error = ip->rm_err;
		}
	}
	ip->flags.b.rm_opened = 0;
	ip->rm_pid = 0;
	ip->rm_err = 0;
	return (error);
}


/*
 * ----- sam_unload_rm - unload removable media file.
 *
 *	Unload a removable media file.
 *  NOTE. inode_rwl WRITER lock set on entry and exit.
 */

int				/* ERRNO if error, 0 if successful. */
sam_unload_rm(
	sam_node_t *ip,		/* Pointer to vnode. */
	int filemode,		/* open file mode. */
	int eox,		/* 1 if end of section, 0 if end of file */
	int wtm,		/* 1 if just write a tape mark, 0 otherwise */
	cred_t *credp)		/* Credentials pointer */
{
	int error = 0;
	int rf_size;
	sam_fs_fifo_ctl_t fifo_ctl;	/* Unload daemon command */
	sam_req_t req_type;	/* delay or nodelay for daemon response */

	bzero((caddr_t)& fifo_ctl, sizeof (fifo_ctl));
	TRACE(T_SAM_FIFO_UL, SAM_ITOV(ip), ip->rdev, filemode, 0);

	if (ip->rdev != 0) {
		sam_resource_file_t *rfp;
		sam_resource_t *rrp;

		if ((cmpldev((dev32_t *)&fifo_ctl.fifo.param.fs_unload.rdev,
		    ip->rdev)) == 0) {
			return (EOVERFLOW);
		}
	if (filemode & FWRITE) {
			/*
			 * Synchronously write pages out and invalidate the
			 * pages.
			 */
			(void) VOP_PUTPAGE_OS(SAM_ITOV(ip), (offset_t)0, 0,
			    B_INVAL, credp, NULL);
		}

		/* Read removable media info. */
		rf_size = ip->di.psize.rmfile;
		if (rf_size == 0) {
			return (ENOCSI);
		}
		rfp = (sam_resource_file_t *)kmem_zalloc(rf_size, KM_SLEEP);

		if (error = sam_get_rm_info(ip, rfp, credp)) {
			kmem_free((void *)rfp, rf_size);
			return (error);
		}
		rrp = &rfp->resource;
		rrp->archive.rm_info.process_eox = eox ? 1 : 0;
		rrp->archive.rm_info.process_wtm = wtm ? 1 : 0;
		fifo_ctl.fifo.param.fs_unload.resource = *rrp;
		/* If writing, update section info */
		if (ip->di.rm.ui.flags & RM_BOF_WRITTEN) {
			offset_t length = 0;
			int i;

			for (i = 0; i < rfp->cur_ord; i++) {
				length += rfp->section[i].length;
			}
			rfp->section[rfp->cur_ord].length =
			    ip->di.rm.size - length;
		}
		fifo_ctl.fifo.param.fs_unload.resource.archive.rm_info.size =
		    rfp->section[rfp->cur_ord].length;
		fifo_ctl.fifo.param.fs_unload.mt_handle.ptr = ip->mt_handle;
		fifo_ctl.fifo.param.fs_unload.io_count = ip->io_count;

		/* Update removable media info */
		if (error = sam_set_rm_info(ip, rfp, credp)) {
			kmem_free((void *)rfp, rf_size);
			return (error);
		}

		TRACES(T_SAM_FIFO_ULN, SAM_ITOV(ip), (char *)rrp->archive.vsn);

		fifo_ctl.fifo.cmd = FS_FIFO_UNLOAD;
		req_type = SAM_NODELAY;

		/* If BOF written, wait for response -- until EOF written. */
		if (ip->di.rm.ui.flags & RM_BOF_WRITTEN) {
			req_type = SAM_DELAY_DAEMON_ACTIVE;
		}

		ip->flags.b.unloading = 1;	/* Unload pending */
		if (!wtm) {
			ip->rdev = 0;		/* Mark unload done */
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

		error = sam_issue_rm(ip, &fifo_ctl, req_type, credp);

		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (ip->di.rm.ui.flags & RM_BOF_WRITTEN) {
			int error2;

			/*
			 * If BOF written, update section information. The
			 * position is not known until this time for SAM-DC.
			 */
			if (error == 0) {
				rfp->section[rfp->cur_ord].position =
				    ip->di.rm.info.rm.position;
				rfp->resource.archive.rm_info.position =
				    ip->di.rm.info.rm.position;
			} else {
				rfp->section[rfp->cur_ord].position = 0;
				rfp->resource.archive.rm_info.valid = 0;
				rfp->resource.archive.rm_info.size = 0;
			}

			/* Update removable media info */
			if ((error2 = sam_set_rm_info(ip, rfp, credp)) != 0) {
				if (error == 0) {
					error = error2;
				}
			}
		}
		kmem_free((void *)rfp, rf_size);
	}
	ip->flags.b.unloading = 0;	/* Unload finished */
	ip->io_count = 0;
	mutex_enter(&ip->rm_mutex);
	if (ip->rm_wait) {
		cv_broadcast(&ip->rm_cv);
	}
	mutex_exit(&ip->rm_mutex);
	return (error);
}


/*
 * ----- sam_issue_rm - issue removable media command.
 *
 *	Issue the command & optionally wait for daemon reply.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_issue_rm(
	sam_node_t *ip,			/* pointer to vnode. */
	sam_fs_fifo_ctl_t *fifo_ctl,	/* pointer to the command */
	sam_req_t req_type,		/* wait for daemon reply, or not */
	cred_t *credp)			/* credentials pointer */
{
	int error = 0;
	sam_fs_fifo_t *fifo;

	fifo = &fifo_ctl->fifo;
	if (req_type != SAM_NODELAY &&
	    req_type != SAM_NODELAY_DAEMON_ACTIVE) {	/* If wait for daemon */
		mutex_enter(&ip->daemon_mutex);
		ip->daemon_busy = 1;
		mutex_exit(&ip->daemon_mutex);
	}
	if ((error = sam_build_amld_cmd(ip, fifo, req_type, credp))) {
		if (req_type != SAM_NODELAY &&
		    req_type != SAM_NODELAY_DAEMON_ACTIVE) {
			/* If wait for daemon */
			mutex_enter(&ip->daemon_mutex);
			ip->daemon_busy = 0;
			mutex_exit(&ip->daemon_mutex);
		}
		return (error);
	}
	if (req_type == SAM_NODELAY ||
	    req_type == SAM_NODELAY_DAEMON_ACTIVE) {
		return (0);
	}

	/* Wait for ioctl response from daemon. */

	mutex_enter(&ip->daemon_mutex);
	while (ip->daemon_busy) {
		if ((error = cv_wait_sig(&ip->daemon_cv,
		    &ip->daemon_mutex)) == 0) {
			error = EINTR;
			ip->daemon_busy = 0;
			break;
		}
	}
	mutex_exit(&ip->daemon_mutex);

	/*
	 * Command has completed or else interrupt was received.
	 * If interrupt from user, cancel the outstanding command.
	 * If signal killed thread or stopping removable media, cancel command.
	 */
	if (error == EINTR || ip->rm_err == EINTR) {
		TRACE(T_SAM_FIFO_CANRM, SAM_ITOV(ip), ip->di.id.ino,
		    fifo->cmd, error);
		fifo->param.fs_cancel.cmd = fifo->cmd;
		fifo->cmd = FS_FIFO_CANCEL;
		(void) sam_send_amld_cmd(fifo, SAM_NOBLOCK);
	} else {
		error = fifo_ctl->ret_err;	/* ERRNO from daemon. */
	}
	return (error);
}


/*
 * ----- sam_build_amld_cmd - issue removable media command to daemon.
 *
 *	Issue fs command to the daemon. Daemon will do the command and
 *	respond back to the file system with a syscall.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_build_amld_cmd(
	sam_node_t *ip,		/* pointer to inode. */
	sam_fs_fifo_t *cp,	/* pointer to command */
	sam_req_t req_type,	/* type of request, delay or nodelay */
	cred_t *credp)		/* credentials pointer */
{
	sam_mount_t *mp;
	int error;
	enum sam_block_flag flg;

	mp = ip->mp;
	cp->magic = FS_FIFO_MAGIC;

	/* Build file handle. Returned in daemon response. */

	cp->handle.id = ip->di.id;
	/*
	 * Set the owner of the mounted tape into the handle.
	 * Note, the fscancel calling process is not the owner.
	 */
	if (cp->cmd == FS_FIFO_UNLOAD) {
		cp->handle.pid = ip->rm_pid;
	} else {
		cp->handle.pid = curproc->p_pid;
	}
	cp->handle.uid = crgetruid(credp);
	cp->handle.gid = crgetrgid(credp);
	cp->handle.fseq = mp->mt.fi_eq;
	cp->handle.flags.bits = 0;
	cp->handle.flags.b.cs_use = ip->di.status.b.cs_use;
	cp->handle.flags.b.cs_val = ip->di.status.b.cs_val;
	cp->handle.cs_algo = ip->di.cs_algo;
	mutex_enter(&ip->mp->ms.m_seqno_mutex);
	cp->handle.seqno = ip->seqno = ip->mp->mi.m_rmseqno++;
	mutex_exit(&ip->mp->ms.m_seqno_mutex);
	cp->handle.stage_off = ip->stage_off;
	cp->handle.stage_len = ip->stage_len;
	cp->handle.fifo_cmd.ptr = cp;

	switch (req_type) {
	case SAM_NODELAY:
		cp->handle.fifo_cmd.ptr = NULL;
		flg = SAM_NOBLOCK;
		break;
	case SAM_DELAY:
		flg = SAM_BLOCK;
		break;
	case SAM_DELAY_DAEMON_ACTIVE:
		flg = SAM_BLOCK_DAEMON_ACTIVE;
		break;
	case SAM_NODELAY_DAEMON_ACTIVE:
		cp->handle.fifo_cmd.ptr = NULL;
		flg = SAM_BLOCK_DAEMON_ACTIVE;
		break;
	}
	error = sam_send_amld_cmd(cp, flg);
	return (error);
}


/*
 * Set stage offset.
 * This is a function because of compiler problems.
 */
static void
sam_set_stage_offset(sam_node_t *ip, u_longlong_t position)
{
	ip->stage_off = position * ip->di.rm.info.rm.mau;
}


/*
 * ----- sam_get_rm_info - get the removable media info for rmedia inode.
 *
 * Read inode extensions or meta data block for removable media info and
 * copy to caller.
 */

static int				/* ERRNO, else 0 if successful. */
sam_get_rm_info(
	sam_node_t *bip,		/* Pointer to base inode. */
	sam_resource_file_t *rfp,	/* removable media structure */
	cred_t *credp)			/* Credentials pointer */
{
	int error = 0;
	int n_vsns;
	int vsns;
	int size;
	sam_id_t eid;
	buf_t *bp;
	struct sam_inode_ext *eip;
	int ino_size;
	int ino_vsns;

	/* Read removable media info from inode extension(s). */
	if (bip->di.ext_attrs & ext_rfa) {
		n_vsns = vsns = size = 0;

		eid = bip->di.ext_id;
		while (eid.ino && (size < bip->di.psize.rmfile)) {
			if (error = sam_read_ino(bip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip)) {
				goto out;
			}
			if (EXT_HDR_ERR(eip, eid, bip)) {
				sam_req_ifsck(bip->mp, -1,
				    "sam_get_rm_info: EXT_HDR", &bip->di.id);
				brelse(bp);
				error = ENOCSI;
				goto out;
			}
			if (S_ISRFA(eip->hdr.mode)) {
				if (EXT_1ST_ORD(eip)) {
					struct sam_rfa_inode *rfa;

					/*
					 * Resource info and vsn[0] in first
					 * inode ext
					 */
					rfa = &eip->ext.rfa;
					n_vsns = rfa->info.n_vsns;
					if ((n_vsns == 0) ||
					    (n_vsns > MAX_VOLUMES)) {
						brelse(bp);
						error = EDOM;
						goto out;
					}
					ino_size = sizeof (sam_resource_file_t);
					if (ino_size > bip->di.psize.rmfile) {
						brelse(bp);
						error = EOVERFLOW;
						goto out;
					}
					bcopy((caddr_t)&rfa->info,
					    (caddr_t)rfp, ino_size);
					vsns = 1;
				} else {
					struct sam_rfv_inode *rfv;

					/*
					 * Rest of vsn list is in following
					 * inode exts
					 */
					rfv = &eip->ext.rfv;
					ino_vsns = rfv->n_vsns;
					if ((rfv->ord != vsns) ||
					    (ino_vsns == 0) ||
					    (ino_vsns > MAX_VSNS_IN_INO)) {
						brelse(bp);
						error = EDOM;
						goto out;
					}
					ino_size =
					    sizeof (struct sam_vsn_section) *
					    ino_vsns;
					if ((size + ino_size) >
					    bip->di.psize.rmfile) {
						brelse(bp);
						error = EOVERFLOW;
						goto out;
					}
					if (ino_size > 0) {
						bcopy((caddr_t)
						    &rfv->vsns.section[0],
						    (caddr_t)rfp,
						    ino_size);
					}
					vsns += ino_vsns;
				}
				rfp = (sam_resource_file_t *)
				    (void *)((caddr_t)rfp + ino_size);
				size += ino_size;
			}
			eid = eip->hdr.next_id;
			brelse(bp);
		}
		if ((size != bip->di.psize.rmfile) || (vsns != n_vsns)) {
			sam_req_ifsck(bip->mp, -1,
			    "sam_get_rm_info: n_vsns", &bip->di.id);
			error = ENOCSI;
		}

	} else {

		/* Read removable media info from meta data block of file. */
		if (bip->di.status.b.offline) {
			if ((error = sam_proc_offline(bip, 0,
			    (offset_t)bip->di.psize.rmfile, NULL,
			    credp, NULL))) {
				goto out;
			}
		}
		if ((error = SAM_BREAD(bip->mp,
		    bip->mp->mi.m_fs[bip->di.extent_ord[0]].dev,
		    bip->di.extent[0],
		    SAM_RM_SIZE(bip->di.psize.rmfile), &bp)) == 0) {
			bcopy((caddr_t)bp->b_un.b_addr, (caddr_t)rfp,
			    bip->di.psize.rmfile);
			brelse(bp);

			size = bip->di.psize.rmfile;
			vsns = rfp->n_vsns;
		}
	}

out:
	TRACE(T_SAM_GET_RFA_EXT, SAM_ITOV(bip), size, vsns, error);
	return (error);
}


/*
 * ----- sam_set_rm_info - update the removable media info for rmedia inode.
 *
 * Update the removable media info in existing inode extension(s) or meta
 * data block.
 */

static int				/* ERRNO, else 0 if successful. */
sam_set_rm_info(
	sam_node_t *bip,		/* Pointer to base inode. */
	sam_resource_file_t *rfp,	/* removable media structure */
	cred_t *credp)			/* Credentials pointer */
{
	int error = 0;
	int n_vsns;
	int vsns;
	int size;
	sam_id_t eid;
	buf_t *bp;
	struct sam_inode_ext *eip;
	int ino_size;
	int ino_vsns;

	/* Update removable media info in inode extension(s). */
	if (bip->di.ext_attrs & ext_rfa) {
		n_vsns = rfp->n_vsns;
		vsns = size = 0;

		eid = bip->di.ext_id;
		while (eid.ino && (size < bip->di.psize.rmfile)) {
			if (error = sam_read_ino(bip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip)) {
				goto out;
			}
			if (EXT_HDR_ERR(eip, eid, bip)) {
				sam_req_ifsck(bip->mp, -1,
				    "sam_set_rm_info: EXT_HDR", &bip->di.id);
				brelse(bp);
				error = ENOCSI;
				goto out;
			}
			if (S_ISRFA(eip->hdr.mode)) {
				if (EXT_1ST_ORD(eip)) {
					struct sam_rfa_inode *rfa;

					/*
					 * Resource info and vsn[0] go in first
					 * inode ext
					 */
					rfa = &eip->ext.rfa;
					ino_size = sizeof (sam_resource_file_t);
					bcopy((caddr_t)rfp,
					    (caddr_t)&rfa->info, ino_size);
					vsns = 1;
				} else {
					struct sam_rfv_inode *rfv;

					/*
					 * Rest of vsn list goes in following
					 * inode exts
					 */
					rfv = &eip->ext.rfv;
					ino_vsns = MIN((n_vsns - vsns),
					    MAX_VSNS_IN_INO);
					/* ord of 1st vsn in ext */
					rfv->ord = vsns;
					rfv->n_vsns = ino_vsns;
					ino_size =
					    sizeof (struct sam_vsn_section) *
					    ino_vsns;
					if (ino_size > 0) {
						bcopy((caddr_t)rfp,
						    (caddr_t)
						    &rfv->vsns.section[0],
						    ino_size);
					}
					vsns += ino_vsns;
				}
				rfp = (sam_resource_file_t *)
				    (void *)((caddr_t)rfp + ino_size);
				size += ino_size;

				eid = eip->hdr.next_id;
				bdwrite(bp);
			} else {
				eid = eip->hdr.next_id;
				brelse(bp);
			}
		}
		if ((size != bip->di.psize.rmfile) || (vsns != n_vsns)) {
			sam_req_ifsck(bip->mp, -1,
			    "sam_set_rm_info: n_vsns", &bip->di.id);
			error = ENOCSI;
		}

	} else {

		/* Update removable media info in meta data block of file. */
		if (bip->di.status.b.offline) {
			if ((error = sam_proc_offline(bip, 0,
			    (offset_t)bip->di.psize.rmfile,
			    NULL, credp, NULL))) {
				goto out;
			}
		}
		if ((error = SAM_BREAD(bip->mp,
		    bip->mp->mi.m_fs[bip->di.extent_ord[0]].dev,
		    bip->di.extent[0],
		    SAM_RM_SIZE(bip->di.psize.rmfile), &bp)) == 0) {
			bcopy((caddr_t)rfp, (caddr_t)bp->b_un.b_addr,
			    bip->di.psize.rmfile);
			bdwrite(bp);

			size = bip->di.psize.rmfile;
			vsns = rfp->n_vsns;
		}
	}

out:
	TRACE(T_SAM_SET_RFA_EXT, SAM_ITOV(bip), size, vsns, error);
	return (error);
}


/*
 * ----- sam_position_rm - position removable media file.
 *
 *	Position a removable media file.
 *  NOTE. inode_rwl READER lock set on entry and exit.
 */

int				/* ERRNO if error, 0 if successful. */
sam_position_rm(
	sam_node_t *ip,		/* Pointer to vnode. */
	offset_t pos,		/* Requested position. */
	cred_t *credp)		/* Credentials. */
{
	int rm_size;
	int error = 0;
	sam_fs_fifo_ctl_t fifo_ctl;	/* Position daemon command */
	sam_req_t req_type;	/* delay or nodelay for daemon response */

	if ((ip->di.rm.media & DT_CLASS_MASK) == DT_OPTICAL) {
		ip->stage_off = pos * ip->di.rm.info.rm.mau;

	} else {
		/*
		 *	For tape devices send positioning request to daemon.
		 */
		bzero((caddr_t)& fifo_ctl, sizeof (fifo_ctl));
		if (ip->rdev != 0) {
			sam_resource_file_t *rp;
			sam_resource_t *rrp;

			if ((cmpldev((dev32_t *)
			    &fifo_ctl.fifo.param.fs_position.rdev,
			    ip->rdev)) == 0) {
				return (EOVERFLOW);
			}
			fifo_ctl.fifo.param.fs_position.setpos = pos;

			/* Retrieve resource record. */
			rm_size = ip->di.psize.rmfile;
			if (rm_size == 0) {
				return (ENOCSI);
			}
			rp = (sam_resource_file_t *)kmem_zalloc(rm_size,
			    KM_SLEEP);

			if ((error = sam_get_rm_info(ip, rp, credp)) == 0) {
				rrp = (sam_resource_t *)rp;
				fifo_ctl.fifo.param.fs_position.resource = *rrp;
			}
			kmem_free((void *)rp, rm_size);
			if (error) {
				return (error);
			}

			fifo_ctl.fifo.cmd = FS_FIFO_POSITION;
			req_type = SAM_DELAY_DAEMON_ACTIVE;

			mutex_enter(&ip->fl_mutex);
			ip->flags.b.positioning = 1;	/* Position pending */
			mutex_exit(&ip->fl_mutex);

			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			error = sam_issue_rm(ip, &fifo_ctl, req_type, credp);
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);

		}
		mutex_enter(&ip->fl_mutex);
		ip->flags.b.positioning = 0;	/* Position finished */
		mutex_exit(&ip->fl_mutex);
	}

	/*
	 * Update resource record.
	 */
	mutex_enter(&ip->rm_mutex);
	if (ip->rm_wait) {
		cv_broadcast(&ip->rm_cv);
	}
	mutex_exit(&ip->rm_mutex);

	return (error);
}


/*
 * ----- sam_rm_direct_io - Read/Write a SAM-QFS file using raw I/O.
 *
 * WRITE: Move the data from the caller's buffer to the device using
 * the device driver cdev_write call.
 * READ: Move the data from the device to the caller's buffer using
 * the device driver cdev_read call.
 */

int				/* ERRNO if error, 0 if successful. */
sam_rm_direct_io(
	sam_node_t *ip,		/* Pointer to vnode */
	enum uio_rw rw,
	uio_t *uiop,		/* Pointer to user I/O vector array. */
	cred_t *credp)		/* credentials pointer. */
{
	int error = 0;
	int length;
	offset_t offset;

	switch (rw) {

		case UIO_WRITE:
wr_reissue:
		if ((error = ip->rm_err) != 0) {
			return (error);
		}
		if (ip->rdev == 0) {
			return (ENODEV);
		}
		if (ip->di.rm.ui.flags & RM_BUFFERED_IO) {
			if (ip->di.rm.info.rm.mau == 0) {
				ip->di.rm.info.rm.mau = uiop->uio_resid;
			}
		}
		ip->io_count++;
		length = uiop->uio_resid;
		offset = uiop->uio_loffset;
		if ((ip->di.rm.media & DT_CLASS_MASK) == DT_OPTICAL) {
			if (ip->di.rm.ui.flags & RM_PARTIAL_PDU) {
				return (EINVAL);
			}
			uiop->uio_loffset = ip->stage_off;
			if (length > ip->space) {
				ip->io_count--;
				uiop->uio_loffset = offset;
				if (ip->di.rm.ui.flags & RM_VOLUMES) {
					if ((error = sam_switch_volume(ip,
					    offset, FWRITE,
					    credp)) == 0) {
						goto wr_reissue;
					}
				}
				return (ENOSPC);
			}
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		error = cdev_write(ip->rdev, uiop, credp);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);

		/*
		 * If partial block written, throw it away and return EIO error.
		 * If full block not written, encountered EOT and return ENOSPC
		 * unless volume overflow.
		 */
		if (uiop->uio_resid) {
			if (uiop->uio_resid != length) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: partial write:"
				    " ino=%d.%d resid=%lld len=%d "
				    "loff=%lld, off=%lld\n",
				    ip->mp->mt.fi_name, ip->di.id.ino,
				    ip->di.id.gen,
				    (long long)uiop->uio_resid, length,
				    uiop->uio_loffset,
				    offset);
				uiop->uio_loffset = offset;
				uiop->uio_resid = length;
				error = EIO;
			} else {
				error = ENOSPC;		/* EOT encountered */
			}
			length -= uiop->uio_resid;
		}

		if ((ip->di.rm.media & DT_CLASS_MASK) == DT_OPTICAL) {
			uiop->uio_loffset = offset + length;
			ip->stage_off += length;
			if (length & (ip->di.rm.info.rm.mau - 1)) {
				/* partial sector written */
				ip->di.rm.ui.flags |= RM_PARTIAL_PDU;
			}
			ip->space -= length;
		} else if (uiop->uio_resid &&
		    (ip->di.rm.ui.flags & RM_VOLUMES)) {
			if ((error = sam_switch_volume(ip,
			    uiop->uio_loffset, FWRITE,
			    credp)) == 0) {
				goto wr_reissue;
			}
		}
		if (error == 0) {
			ip->size += length;
			ip->di.rm.size = ip->size;
		}
		break;

	case UIO_READ: {
		offset_t rem;
		offset_t resid = 0;
		offset_t lresid;
		dev_t rdev;

rd_reissue:
		if ((error = ip->rm_err) != 0) {
			return (error);
		}
		if (ip->rdev == 0) {
			return (ENODEV);
		}
		ip->io_count++;
		rem = ip->size - uiop->uio_loffset;
		if (rem <= 0) {
			break;
		}

		rdev = ip->rdev;
		length = uiop->uio_resid;
		offset = uiop->uio_loffset;
		if (uiop->uio_resid > rem) {
			resid = uiop->uio_resid - rem;
			uiop->uio_resid = (ssize_t)rem;
		}
		lresid = uiop->uio_resid;
		if (!(ip->di.rm.ui.flags & RM_BUFFERED_IO)) {
			/*
			 * directio mode, read requested amount from the device.
			 */
			if ((ip->di.rm.media & DT_CLASS_MASK) == DT_OPTICAL) {
				uiop->uio_loffset = ip->stage_off;
				if (ip->space <= 0) {
					ip->io_count--;
					uiop->uio_loffset = offset;
					if (ip->di.rm.ui.flags & RM_VOLUMES) {
						if ((error =
						    sam_switch_volume(ip,
						    offset, FREAD,
						    credp)) == 0) {
							goto rd_reissue;
						}
					}
					return (0);
				}
			}
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			error = cdev_read(rdev, uiop, credp);
			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			length -= uiop->uio_resid;
			if ((ip->di.rm.media & DT_CLASS_MASK) == DT_OPTICAL) {
				uiop->uio_loffset = offset + length;
				ip->stage_off += length;
				ip->space -= length;
			}
		} else if ((error = sam_rm_buffered_io(ip, uiop, credp)) != 0) {
			return (error);
		}

		uiop->uio_resid += resid;
		if (error)
			break;

		if (ip->di.rm.ui.flags & RM_STRANGER) {
			/*
			 * Stranger tapes are terminated by two back to back
			 * filemarks.
			 */
			if (uiop->uio_resid == lresid) {
				/* No data read == FM */
				if (ip->di.rm.ui.flags & RM_FILEMARK) {
					/* If 2 FMs */
					if (ip->di.rm.ui.flags & RM_VOLUMES) {
						if ((error =
						    sam_switch_volume(ip,
						    uiop->uio_loffset,
						    FREAD, credp)) == 0)  {
							goto rd_reissue;
						}
						if (error == ENOSPC) {
							error = 0;
						}
					}
				} else {
					ip->di.rm.ui.flags |= RM_FILEMARK;
				}
			} else {
				ip->di.rm.ui.flags &= ~RM_FILEMARK;
			}
		} else {
			if (uiop->uio_resid && (uiop->uio_resid >= resid) &&
			    ip->di.rm.ui.flags & RM_VOLUMES) {
				if ((error = sam_switch_volume(ip,
				    uiop->uio_loffset, FREAD,
				    credp)) == 0) {
					goto rd_reissue;
				}
			}
		}
		}
		break;
	}				/* End of switch */
	return (error);
}


/*
 * ----- sam_rm_buffered_io - Read a SAM-QFS file using raw buffered I/O.
 *
 * Move the data from the device or rmio buffer to the caller's buffer
 * using the device driver cdev_read call.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_rm_buffered_io(
	sam_node_t *ip,		/* Pointer to vnode */
	uio_t *uiop,		/* Pointer to user I/O vector array. */
	cred_t *credp)		/* credentials pointer. */
{
	int		 error = 0;
	int		 resid = 0;
	int		 length = uiop->uio_resid;
	offset_t offset = uiop->uio_loffset;
	offset_t mau_offset = ip->di.rm.info.rm.mau - 1;
	offset_t mau_mask = ~mau_offset;

	while (uiop->uio_resid) {

		offset_t file_offset = ip->di.rm.info.rm.file_offset;
		offset_t media_offset;	 /* offset of media block */
		uint_t block_offset;   /* offset in media block */

		/* check if first read is on the block border */
		if (ip->io_count == 1 && !ip->rmio_buf) {
			media_offset = ((file_offset << 9) & mau_mask) >> 9;
			block_offset = (file_offset - media_offset) << 9;

			if ((ip->di.rm.media & DT_CLASS_MASK) == DT_OPTICAL) {
				ip->stage_off -= block_offset;
			}
		} else block_offset = 0;

		/*
		 * Check if requested data was already read.
		 * ip->stage_n_off is used as the beginning of block
		 * stored in rmio buffer and ip->stage_len is the last byte
		 * of valid data in the buffer.
		 */
		if (ip->rmio_buf &&
		    uiop->uio_loffset > ip->stage_n_off &&
		    uiop->uio_loffset < ip->stage_len) {
			/* already have block read */
			off_t boff = uiop->uio_loffset - ip->stage_n_off;
			int bvalid = ip->stage_len - ip->stage_n_off;
			int blength = MIN(uiop->uio_resid, bvalid - boff);

			error = uiomove(ip->lbase + boff,
			    blength, UIO_READ, uiop);
			if ((ip->di.rm.media & DT_CLASS_MASK) == DT_OPTICAL) {
				uiop->uio_loffset = offset + blength;
						ip->stage_off += blength;
			}
			/*
			 * Check if previous device read was not of block size.
			 * If yes, return to user and let him try again at
			 * the beginning of new block, so 0
			 * could be returned as an indication of EOF.
			 */
			if (error || uiop->uio_resid == 0 ||
			    bvalid < ip->di.rm.info.rm.mau) {
				uiop->uio_resid += resid;
				ip->rm_err = error;
				return (error);
			}

			length -= blength;
		}
		/*
		 * If on the block border and at least one block to read,
		 * read it into the user buffer directly
		 */
		if ((block_offset == 0) && ((length & mau_mask) > 0)) {
			int nbytes;

			iovec_t		*uio_iov;
			int			uio_iovcnt;

			offset = uiop->uio_loffset;
			if ((ip->di.rm.media & DT_CLASS_MASK) == DT_OPTICAL) {
				uiop->uio_loffset = ip->stage_off;
			}

			/* Read blocks directly from the device */
			uiop->uio_iov->iov_len = uiop->uio_resid =
			    length & mau_mask;
			nbytes = uiop->uio_resid;
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);

			/*
			 * Device driver sometimes is too smart and
			 * modifies uiop->uio_iov & uiop->uio_iovcnt
			 * by decrementing uiop->uio_iovcnt even if there
			 * is more space in uiop->uio_iov.
			 * Save these values and restore it after call
			 * to driver.
			 */
			uio_iov = uiop->uio_iov;
			uio_iovcnt = uiop->uio_iovcnt;
			error = cdev_read(ip->rdev, uiop, credp);
			uiop->uio_iovcnt = uio_iovcnt;
			uiop->uio_iov = uio_iov;

			RW_LOCK_OS(&ip->inode_rwl, RW_READER);
			nbytes -= uiop->uio_resid;
			if ((ip->di.rm.media & DT_CLASS_MASK) == DT_OPTICAL) {
				uiop->uio_loffset = offset + nbytes;
				ip->stage_off += nbytes;
			}
			length -= nbytes;
			if (error || uiop->uio_resid) {
				/* not complete read */
				uiop->uio_resid = length + resid;
				ip->rm_err = error;
				return (error);
			}
			uiop->uio_resid = length;
			uiop->uio_iov->iov_len = uiop->uio_resid + resid;
		}

		/* Partial block to read, use buffer */
		if (length) {
			int rmio_buf_len;

			offset = uiop->uio_loffset;

			/* read the next media block in the rmio buffer */
			error = sam_buffer_rmio(ip, offset,
			    &rmio_buf_len, credp);

			/* Break if error or nothing was read */
			if (error || rmio_buf_len == 0) {
				uiop->uio_resid += resid;
				ip->rm_err = error;
				return (error);
			}

			if (block_offset) {
				/*
				 * Adjust the beginning of the data in rmio_buf
				 */
				ip->lbase = ip->rmio_buf + block_offset;
				rmio_buf_len -= block_offset;
				ip->stage_len = ip->stage_n_off + rmio_buf_len;
			} else {
				ip->lbase = ip->rmio_buf;
			}

			/* transfer data to the user buffer */
			length = MIN(length, rmio_buf_len);
			error = uiomove(ip->lbase, length, UIO_READ, uiop);
			if (error) {
				uiop->uio_resid += resid;
				ip->rm_err = error;
				return (error);
			}
			if ((ip->di.rm.media & DT_CLASS_MASK) == DT_OPTICAL) {
				ip->stage_off += length + block_offset;
			}
		}
	}
	return (0);
}


/*
 * ----- sam_buffer_rmio - read block from the device into rmio buffer.
 *
 * Return error code from device read call.
 */

static int
sam_buffer_rmio(
	sam_node_t *ip,		/* Pointer to inode */
	offset_t offset,	/* Offset to read from */
	int *bytes_read,	/* Return number bytes read */
	cred_t *credp)		/* Credentials pointer. */
{
	int error;
	iovec_t ipiovec;
	uio_t ipuio;

	if (ip->rmio_buf == NULL) {
		ip->rmio_buf = kmem_zalloc(ip->di.rm.info.rm.mau, KM_SLEEP);
	}
	bzero((caddr_t)& ipuio, sizeof (ipuio));
	ipiovec.iov_base = ip->rmio_buf;
	ipiovec.iov_len = ip->di.rm.info.rm.mau;
	ipuio.uio_iov = &ipiovec;
	ipuio.uio_iovcnt = 1;
	if ((ip->di.rm.media & DT_CLASS_MASK) == DT_OPTICAL) {
		ipuio.uio_loffset = ip->stage_off;
	} else {
		ipuio.uio_loffset = offset;
	}
	ipuio.uio_segflg = UIO_SYSSPACE;
	ipuio.uio_resid = ip->di.rm.info.rm.mau;

	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	error = cdev_read(ip->rdev, &ipuio, credp);
	RW_LOCK_OS(&ip->inode_rwl, RW_READER);

	*bytes_read = ip->di.rm.info.rm.mau - ipuio.uio_resid;
	if (!error && *bytes_read != 0) {
		ip->stage_n_off = offset;
		if ((ip->di.rm.media & DT_CLASS_MASK) == DT_OPTICAL) {
			ipuio.uio_loffset = offset + *bytes_read;
		}
		ip->stage_len = ipuio.uio_loffset;
	}
	return (error);
}
