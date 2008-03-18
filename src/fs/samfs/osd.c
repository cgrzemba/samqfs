/*
 * ----- osd.c - Process the object storage device functions.
 *
 * Processes the OSD functions supported on the SAM File System.
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

#pragma ident "$Revision: 1.6 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

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
#include <sys/ddi.h>
#include <sys/byteorder.h>
#if defined(SAM_OSD_SUPPORT)
#include <sys/osd.h>
#endif


/* ----- SAMFS Includes */

#include <sam/types.h>
#include "macros.h"
#include "inode.h"
#include "mount.h"
#include "quota.h"
#include "ino_ext.h"
#include "rwio.h"
#include "indirect.h"
#include "extern.h"
#include "trace.h"
#include "debug.h"


#if defined(SAM_OSD_SUPPORT)
#include "object.h"

/*
 * The following externals are resolved by the sosd device driver which will
 * be in Solaris 11. These externals only affect developers testing the new
 * object file system (mb).
 * osd_open_by_dev
 * osd_close
 * osd_iotask_alloc
 * osd_iotask_start
 * osd_iotask_free
 */

/*
 * Remove the following after the device driver is in Solaris 11
 */
char _depends_on[]	= "drv/sosd drv/scsi";

static int sam_osd_errno(struct osd_iotask *iotp);
static int sam_osd_bp_from_kva(caddr_t memp, size_t memlen, int rw,
	struct buf **bpp);
static void sam_osd_release_bp(struct buf *bp, caddr_t memp, size_t memlen,
	int rw);
static void sam_format_get_attr_list(sam_out_get_osd_attr_list_t *listp,
	uint32_t attr_page, uint32_t attr_num);
#if SAM_ATTR_LIST
static char *sam_get_attr_addr(osd_attributes_list_t *list,
	uint32_t attr_num);
static uint64_t sam_get_8_byte_attr_val(char *listp, uint32_t attr_num);
#endif /* SAM_ATTR_LIST */
static void sam_object_req_done(struct osd_iotask *iotp);
static void sam_dk_object_done(struct osd_iotask *iotp);
static void sam_pg_object_done(osd_iotask_t *iotp);


/*
 * ----- sam_open_osd_device - Open an osd device. Return object handle.
 */
int
sam_open_osd_device(
	struct samdent *dp,	/* Pointer to device entr */
	int filemode,		/* Filemode for open */
	cred_t *credp)		/* Credentials pointer. */
{
	ldi_ident_t lident;
	osd_handle_t oh;
	vnode_t *svp;
	dev_t dev;
	int error;

	if ((error = lookupname(dp->part.pt_name, UIO_SYSSPACE,
	    FOLLOW, NULL, &svp))) {
		return (ENODEV);
	}
	if (svp->v_type != VCHR) {
		VN_RELE(svp);
		return (EINVAL);	/* Must be type chr */
	}
	dev = svp->v_rdev;
	VN_RELE(svp);
	if ((error = ldi_ident_from_dev(dev, &lident))) {
		return (error);
	}
	if ((error = osd_open_by_dev(&dev, OTYP_CHR, filemode,
	    credp, &oh, lident))) {
		return (error);
	}
	dp->oh = oh;
	dp->dev = dev;
	dp->system = 0;
	dp->dev_bsize = SAM_OSD_BSIZE;	/* Byte alignment */
	dp->opened = 1;
	dp->svp = svp;
	return (0);
}


/*
 * ----- sam_close_osd_device - Close an osd device.
 */
void
sam_close_osd_device(
	void *oh,		/* Object device handle */
	int filemode,		/* Filemode for open */
	cred_t *credp)		/* Credentials pointer. */
{
	ASSERT(oh != NULL);
	(void) osd_close(oh, filemode, credp);
}


/*
 * ----- sam_issue_object_io - Issue object I/O.
 */
int
sam_issue_object_io(
	void *oh,
	uint32_t command,
	uint64_t obj_id,
	enum uio_seg seg,
	char *data,
	offset_t offset,
	offset_t length)
{
	osd_iotask_t	*iotp;
	uint64_t	partid = SAM_OBJ_PAR_ID;
	char *bufp;
	buf_t *bp;
	int error;

	error = osd_iotask_alloc(oh, &iotp,
	    sizeof (sam_osd_iot_priv_t), OSD_SLEEP);
	if (error != OSD_SUCCESS) {
		return (EINVAL);
	}

	if (seg == UIO_USERSPACE) {
		bufp = (char *)kmem_alloc(length, KM_SLEEP);
	} else {
		bufp = data;
	}
	if (error = sam_osd_bp_from_kva((char *)bufp, length,
	    command == OSD_WRITE ? B_WRITE : B_READ, &bp)) {
		goto fini;
	}
	if (command == OSD_WRITE) {
		osd_setup_WRITE(iotp, obj_id, length, offset);
		if (seg == UIO_USERSPACE) {
			if (copyin(data, bufp, length)) {
				error = EFAULT;
				goto fini;
			}
		}
		iotp->ot_out_command_bp = bp;
	} else {
		osd_setup_READ(iotp, obj_id, length, offset);
		iotp->ot_in_command_bp = bp;
	}

	iotp->ot_partition_id = partid;
	iotp->ot_iodone = sam_object_req_done;
	sam_osd_setup_PRIVATE(iotp);

	if ((error = osd_iotask_start(iotp)) == OSD_SUCCESS) {
		sam_osd_io_task_WAIT(iotp);
		if (iotp->ot_error != OSD_SUCCESS) {
			error = sam_osd_errno(iotp);
		} else {
			if (command == OSD_READ) {
				if (seg == UIO_USERSPACE) {
					if (copyout(bufp, data, length)) {
						error = EFAULT;
					}
				}
			}
		}
	} else {
		error = sam_osd_errno(iotp);
	}
	sam_osd_remove_PRIVATE(iotp);
	if (command == OSD_WRITE) {
		sam_osd_release_bp(iotp->ot_out_command_bp, (char *)bufp,
		    length, B_WRITE);
	} else {
		sam_osd_release_bp(iotp->ot_in_command_bp, (char *)bufp,
		    length, B_READ);
	}
fini:
	(void) osd_iotask_free(iotp);
	if (seg == UIO_USERSPACE) {
		kmem_free(bufp, length);
	}
	return (error);
}


/*
 * ----- sam_issue_direct_object_io - Issue object direct async I/O.
 */
int
sam_issue_direct_object_io(
	sam_node_t *ip,
	enum uio_rw rw,
	struct buf *bp,
	offset_t contig,
	offset_t cur_loffset)
{
	osd_iotask_t	*iotp;
	uint64_t	requested_user_object_id;
	uint64_t	partid = SAM_OBJ_PAR_ID;
	sam_di_obj_t	*obj;
	int error;

	error = osd_iotask_alloc(ip->mp->mi.m_fs[ip->di.unit].oh, &iotp,
	    sizeof (sam_osd_iot_priv_t), OSD_SLEEP);
	if (error != OSD_SUCCESS) {
		return (EINVAL);
	}
	obj = (sam_di_obj_t *)(void *)&ip->di.extent[0];
	requested_user_object_id = obj->user_id;
	if (rw == UIO_WRITE) {
		osd_setup_WRITE(iotp, requested_user_object_id, contig,
		    cur_loffset);
		iotp->ot_out_command_bp = bp;
		if ((cur_loffset + contig) > obj->wr_size) {
			obj->wr_size = (cur_loffset + contig);
		}
		ASSERT(contig <= SAM_OSD_MAX_WR_CONTIG);
	} else {
		osd_setup_READ(iotp, requested_user_object_id, contig,
		    cur_loffset);
		iotp->ot_in_command_bp = bp;
		ASSERT(contig <= SAM_OSD_MAX_RD_CONTIG);
	}
	iotp->ot_partition_id = partid;
	iotp->ot_iodone = sam_dk_object_done;
	(iot_priv(iotp))->bp = (struct sam_buf *)bp;

	if ((error = osd_iotask_start(iotp)) != OSD_SUCCESS) {
		iotp->ot_error = error; /* SET BY SOSD???? */
		sam_dk_object_done(iotp);
	}
	return (error);
}


/*
 * ----- sam_dk_object_done - Process object direct async I/O completion.
 *
 * This is the interrupt routine. Called when each async I/O completes.
 */

static void
sam_dk_object_done(
	osd_iotask_t *iotp)	/* The io_task pointer. */
{
	sam_node_t *ip;
	buf_descriptor_t	*bdp;
	sam_buf_t  		*sbp;
	buf_t			*bp;
	offset_t 		count;
	offset_t		offset;
	sam_di_obj_t		*obj;
	boolean_t		async;

	bp = (buf_t *)(iot_priv(iotp))->bp;
	sbp = (sam_buf_t *)bp;
	bdp = (buf_descriptor_t *)(void *)bp->b_vp;
	ip = bdp->ip;
	obj = (sam_di_obj_t *)(void *)&ip->di.extent[0];
	async = (bdp->aiouiop != NULL);
	bp->b_vp = NULL;
	bp->b_iodone = NULL;
	count = bp->b_bcount;
	offset = bp->b_offset;

	if (bp->b_flags & B_REMAPPED) {
		bp_mapout(bp);
	}

	mutex_enter(&bdp->bdp_mutex);
	SAM_BUF_DEQ(&sbp->link);
	if (!async) {
		sema_v(&bdp->io_sema);		/* Increment completed I/Os */
	}
	bdp->io_count--;		/* Decrement number of issued I/O */
	if (bp->b_flags & B_WRITE) {
		TRACE(T_SAM_DIOWROBJ_COMP, SAM_ITOV(ip), (sam_tr_t)iotp,
		    offset, count);
	} else {
		TRACE(T_SAM_DIORDOBJ_COMP, SAM_ITOV(ip), (sam_tr_t)iotp,
		    offset, count);
	}

	/*
	 *  Check for errors
	 */
	if (iotp->ot_error != OSD_SUCCESS) {
		bdp->error = sam_osd_io_errno(iotp, bp);
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: DK er=%d, ip=%p, ino=%d, op=%x, off=%x"
		    " len=%x r=%x sz=%x %x %x",
		    ip->mp->mt.fi_name, iotp->ot_error, (void *)ip,
		    ip->di.id.ino, iotp->ot_service_action, offset,
		    bp->b_bcount, bp->b_resid, (offset+bp->b_bcount),
		    obj->wr_size, ip->size);
	}
	mutex_exit(&bdp->bdp_mutex);

	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS|B_SHADOW);
	sam_free_buf_header((sam_uintptr_t *)bp);
	(void) osd_iotask_free(iotp);

	if (async) {
		sam_dk_aio_direct_done(ip, bdp, count);
	}
}


/*
 * ----- sam_pageio_object - Start object I/O on a page.
 */

buf_t *
sam_pageio_object(
	sam_node_t *ip,		/* Pointer to inode table */
	sam_ioblk_t *iop,	/* Pointer to I/O entry */
	page_t *pp,		/* Pointer to page list */
	offset_t offset,	/* Logical file offset */
	uint_t pg_off,		/* Offset within page */
	uint_t vn_len,		/* Length of transfer */
	int flags)		/* Flags --B_INVAL, B_DIRTY, B_FREE, */
				/*   B_DONTNEED, B_FORCE, B_ASYNC. */
{
	sam_mount_t 	*mp;	/* Pointer to mount table */
	buf_t 		*bp;
	osd_iotask_t	*iotp;
	uint64_t	requested_user_object_id;
	uint64_t	partid = SAM_OBJ_PAR_ID;
	offset_t	high_addr;
	sam_di_obj_t	*obj;

	mp = ip->mp;
	if ((osd_iotask_alloc(mp->mi.m_fs[ip->di.unit].oh, &iotp,
	    sizeof (sam_osd_iot_priv_t), OSD_SLEEP)) != OSD_SUCCESS) {
		return (NULL);
	}
	bp = pageio_setup(pp, vn_len, mp->mi.m_fs[iop->ord].svp, flags);
	bp->b_edev = mp->mi.m_fs[iop->ord].dev;
	bp->b_dev = cmpdev(bp->b_edev);
	bp->b_blkno = 0;
	bp->b_un.b_addr = (char *)pg_off;
	bp->b_file = SAM_ITOP(ip);
	bp->b_offset = offset;
	TRACE((bp->b_flags & B_READ) ? T_SAM_PGRDOBJ_ST :
	    T_SAM_PGWROBJ_ST, SAM_ITOV(ip), (sam_tr_t)iotp, offset, vn_len);
	if (!(bp->b_flags & B_READ)) {	/* If writing */
		if (bp->b_flags & B_ASYNC) {
			bp->b_iodone = (int (*) ())sam_page_wrdone;
			TRACE(T_SAM_ASYNC_WR, SAM_ITOV(ip), ip->di.id.ino,
			    (sam_tr_t)bp->b_pages, (sam_tr_t)bp);
			mutex_enter(&ip->write_mutex);
			ip->cnt_writes += bp->b_bcount;
			mutex_exit(&ip->write_mutex);
		}
	}
	obj = (sam_di_obj_t *)(void *)&ip->di.extent[0];
	requested_user_object_id = obj->user_id;
	high_addr = (offset + vn_len);
	if (bp->b_flags & B_READ) {	/* If reading */
		osd_setup_READ(iotp, requested_user_object_id, vn_len, offset);
		iotp->ot_in_command_bp = bp;
		ASSERT(vn_len <= SAM_OSD_MAX_RD_CONTIG);
		/*
		 * Must wait to issue read until write (end of obj)
		 * has completed. Note, requests can complete out
		 * of order AND issue out of order.
		 */
	} else {
		boolean_t lock_set = B_FALSE;

		osd_setup_WRITE(iotp, requested_user_object_id, vn_len, offset);
		iotp->ot_out_command_bp = bp;
		if (RW_OWNER_OS(&ip->inode_rwl) != curthread) {
			lock_set = B_TRUE;
			mutex_enter(&ip->fl_mutex);
		}
		if (high_addr > obj->wr_size) {
			obj->wr_size = high_addr;
		}
		if (lock_set) {
			mutex_exit(&ip->fl_mutex);
		}
		ASSERT(vn_len <= SAM_OSD_MAX_WR_CONTIG);
	}
	bp->b_private = (void *)iotp;
	iotp->ot_partition_id = partid;
	iotp->ot_iodone = sam_pg_object_done;
	sam_osd_setup_PRIVATE(iotp);
	(iot_priv(iotp))->bp = (struct sam_buf *)bp;
	(iot_priv(iotp))->ip = (struct sam_node *)ip;
	if ((osd_iotask_start(iotp)) != OSD_SUCCESS) {
		iotp->ot_error = OSD_FAILURE;
		sam_pg_object_done(iotp);
	}
	return (bp);
}


/*
 * ----- sam_pg_object_done - Process object page async I/O completion.
 *
 * This is the interrupt routine. Called when each async I/O completes.
 */

static void
sam_pg_object_done(
	osd_iotask_t *iotp)	/* The io_task pointer. */
{
	sam_node_t 		*ip;
	buf_t			*bp;
	offset_t		count;
	offset_t		offset;
	sam_di_obj_t		*obj;

	ip = (struct sam_node *)(iot_priv(iotp))->ip;
	obj = (sam_di_obj_t *)(void *)&ip->di.extent[0];
	bp = (buf_t *)(iot_priv(iotp))->bp;
	offset = bp->b_offset;
	count = bp->b_bcount;
	if (bp->b_flags & B_WRITE) {
		TRACE(T_SAM_PGWROBJ_COMP, SAM_ITOV(ip), (sam_tr_t)iotp,
		    iotp->ot_write.op_starting_byte_address,
		    iotp->ot_write.op_length);
		ASSERT(iotp->ot_write.op_starting_byte_address == bp->b_offset);
	} else {
		TRACE(T_SAM_PGRDOBJ_COMP, SAM_ITOV(ip), (sam_tr_t)iotp,
		    iotp->ot_read.op_starting_byte_address,
		    iotp->ot_read.op_length);
		ASSERT(iotp->ot_read.op_starting_byte_address == bp->b_offset);
	}

	/*
	 *  Check for errors
	 */
	if (iotp->ot_error != OSD_SUCCESS) {
		bp->b_error = sam_osd_io_errno(iotp, bp);
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: PG er=%d, ip=%p ino=%d op=%x off=%x"
		    " len=%x r=%x ha=%x %x %x %x",
		    ip->mp->mt.fi_name, iotp->ot_error, (void *)ip,
		    ip->di.id.ino, iotp->ot_service_action, offset, count,
		    bp->b_resid, (offset+count), obj->wr_size, obj->cm_size,
		    ip->size);
	}
	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS|B_SHADOW);
	if (bp->b_flags & B_WRITE) {
		if ((offset + count) > obj->cm_size) {
			obj->cm_size = (offset + count);
		}
	}
	bp->b_iodone = NULL;
	if (bp->b_flags & B_ASYNC) {
		if (!(bp->b_flags & B_READ)) {	/* If writing */
			sam_page_wrdone(bp);
		} else {
			biodone(bp);
		}
		(void) osd_iotask_free(iotp);
	} else {
		sam_osd_io_task_DONE(iotp);
	}
}


/*
 * ----- sam_pg_object_sync_done - Process object page sync I/O completion.
 */

int
sam_pg_object_sync_done(
	sam_node_t *ip,		/* Pointer to inode table */
	buf_t	*bp,		/* Pointer to the buffer */
	char	*str)		/* "GETPAGE" OR "PUTPAGE" */
{
	struct osd_iotask	*iotp;
	int error = 0;

	iotp = (struct osd_iotask *)bp->b_private;
	sam_osd_io_task_WAIT(iotp);
	if (bp->b_error) {
		dcmn_err((CE_WARN,
		    "SAM-QFS: %s: %s ip=%p, ino=%d, op=%x"
		    " off=%x len=%x err=%d",
		    ip->mp->mt.fi_name, str, (void *)ip, ip->di.id.ino,
		    iotp->ot_service_action, bp->b_offset, bp->b_bcount,
		    bp->b_error));
		if (error == 0) {
			error = bp->b_error;
		}
	}
	pageio_done(bp);
	(void) osd_iotask_free(iotp);
	return (error);
}


/*
 * ----- sam_osd_errno - Set errno given osd ot_errno.
 */
static int
sam_osd_errno(struct osd_iotask *iotp)
{
	int error;

	switch (iotp->ot_error) {
	case OSD_SUCCESS:
		return (0);

	case OSD_RESERVATION_CONFLICT:
		return (EACCES);

	case OSD_BUSY:
		return (EAGAIN);

	case OSD_INVALID:
	case OSD_BADREQUEST:
		return (EINVAL);

	case OSD_TOOBIG:
		return (E2BIG);

	case OSD_FAILURE:
	case OSD_CHECK_CONDITION:
		return (EIO);

	default:
		error = EIO;
		break;

	}
	return (error);
}


/*
 * ----- sam_osd_io_errno - Set errno in buffer given osd ot_error.
 */
int
sam_osd_io_errno(
	struct osd_iotask *iotp,
	struct buf *bp)
{
	int error;

	bp->b_error = 0;
	switch (iotp->ot_error) {
	case OSD_SUCCESS:
		bp->b_resid = 0;
		return (0);

	case OSD_RESIDUAL:
		{
			osd_resid_t *orp;

			orp = iotp->ot_errp;
			bp->b_resid = (bp->b_flags & B_READ) ?
			    orp->ot_in_command_resid :
			    orp->ot_out_command_resid;
			cmn_err(CE_NOTE,
			    "SAM-QFS: OSD RESID resid=%x, off=%llx",
			    bp->b_resid, (long long)bp->b_offset);
			return (0);
		}

	case OSD_RESERVATION_CONFLICT:
		error = EACCES;
		break;

	case OSD_BUSY:
		error = EAGAIN;
		break;

	case OSD_INVALID:
	case OSD_BADREQUEST:
		error = EINVAL;
		break;

	case OSD_TOOBIG:
		error = E2BIG;
		break;

	case OSD_FAILURE:
	case OSD_CHECK_CONDITION:
		error = EIO;
		break;

#if DECODE_SCSI
	{
		int osderr_detail = 0;
		if (iotp->ot_errtype == OSD_ERR_SENSE && iotp->ot_errlen != 0) {
			osderr_detail = osd_decode_sense(iotp->ot_errbufp,
			    iotp->ot_errlen);
		}
	}
#endif /* DECODE_SCSI */

	default:
		error = EIO;
		break;
	}
	bp->b_error = error;
	bp->b_flags |= B_ERROR;
	return (error);
}


/*
 * ----- sam_osd_bp_from_kva - Setup the buf struct for the memobj.
 */
static int
sam_osd_bp_from_kva(
	char *memp,
	size_t memlen,
	int rw,
	struct buf **bpp)
{
	struct buf	*bp;
	page_t		**pplist;
	int		error;

	ASSERT((rw == B_READ) || (rw == B_WRITE));

	bp = getrbuf(KM_SLEEP);
	bioinit(bp);
	bp->b_offset = -1;
	bp->b_dip = NULL;
	bp->b_proc = NULL;
	bp->b_flags = B_BUSY | B_PHYS | rw;

	bp->b_bcount    = memlen;
	bp->b_un.b_addr = memp;

	if ((error = as_pagelock(&kas, &pplist, memp, memlen,
	    rw == B_READ ? S_WRITE : S_READ))) {
		biofini(bp);
		freerbuf(bp);
		return (error);
	}
	bp->b_shadow = pplist;
	if (pplist != NULL) {
		bp->b_flags |= B_SHADOW;
	}
	bp->b_flags |= B_STARTED;
	*bpp = bp;
	return (0);
}


/*
 * ----- sam_osd_release_bp - Teardown the buf struct for the memobj.
 */
static void
sam_osd_release_bp(
	struct buf *bp,
	char *memp,
	size_t memlen,
	int rw)
{
	as_pageunlock(&kas, bp->b_shadow, memp, memlen,
	    rw == B_READ? S_WRITE : S_READ);
	biofini(bp);
	freerbuf(bp);
}


/*
 * ----- sam_object_req_done - Process object osd request completion.
 *
 * This is the interrupt routine. Called when the object request completes.
 */

static void
sam_object_req_done(struct osd_iotask *iotp)	/* The osd_iotask pointer. */
{
	sam_osd_io_task_DONE(iotp);
}


/*
 * ----- sam_create_priv_object_id - Process create of privileged object id.
 */
int
sam_create_priv_object_id(
	void		*oh,
	uint64_t	user_obj_id)
{
	struct osd_iotask	*iotp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	int error;

	if ((error = osd_iotask_alloc(oh, &iotp,
	    sizeof (sam_osd_iot_priv_t), OSD_SLEEP)) != OSD_SUCCESS) {
		return (EINVAL);
	}

	osd_setup_CREATE(iotp, user_obj_id, (uint64_t)1);
	iotp->ot_partition_id = partid;
	iotp->ot_iodone = sam_object_req_done;
	sam_osd_setup_PRIVATE(iotp);

	if ((error = osd_iotask_start(iotp)) == OSD_SUCCESS) {
		sam_osd_io_task_WAIT(iotp);
		if (iotp->ot_error != OSD_SUCCESS) {
			error = sam_osd_errno(iotp);
		}
	} else {
		error = sam_osd_errno(iotp);
	}
	sam_osd_remove_PRIVATE(iotp);
	(void) osd_iotask_free(iotp);
	return (error);
}


/*
 * ----- sam_create_object_id - Process create_object id.
 */
int
sam_create_object_id(
	sam_mount_t *mp,
	struct sam_disk_inode *dp)
{
	struct osd_iotask	*iotp;
	uint64_t		requested_user_object_id;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	int error;

	ASSERT(mp->mi.m_fs[dp->unit].oh != 0);
	dp->version = SAM_INODE_VERSION;
	if ((error = osd_iotask_alloc(mp->mi.m_fs[dp->unit].oh, &iotp,
	    sizeof (sam_osd_iot_priv_t), OSD_SLEEP)) != OSD_SUCCESS) {
		return (EINVAL);
	}
	mutex_enter(&mp->ms.m_waitwr_mutex);
	requested_user_object_id = mp->mi.m_sbp->info.sb.osd_user_id++;
	mutex_exit(&mp->ms.m_waitwr_mutex);

	osd_setup_CREATE(iotp, requested_user_object_id, (uint64_t)1);
	iotp->ot_partition_id = partid;
	iotp->ot_iodone = sam_object_req_done;
	sam_osd_setup_PRIVATE(iotp);

	if ((error = osd_iotask_start(iotp)) == OSD_SUCCESS) {
		sam_osd_io_task_WAIT(iotp);
		if (iotp->ot_error == OSD_SUCCESS) {
			sam_di_obj_t	*obj;
			sam_id_attr_t	x;

			dp->rm.ui.flags |= RM_OBJECT_FILE;
			obj = (sam_di_obj_t *)(void *)&dp->extent[0];
			obj->user_id = requested_user_object_id;
			x.id = dp->id;

			/*
			 * Replace OSD_USER_OBJECT_INFORMATION_USERNAME
			 * with QFS vendor specific attribute for ino/gen.
			 */
			error = sam_set_user_object_attr(mp, dp,
			    OSD_USER_OBJECT_INFORMATION_USERNAME, x.attr);
		} else {
			error = sam_osd_errno(iotp);
		}
	} else {
		error = sam_osd_errno(iotp);
	}
	sam_osd_remove_PRIVATE(iotp);
	(void) osd_iotask_free(iotp);
	return (error);
}


/*
 * ----- sam_remove_object_id - Process remove_object id.
 */
int
sam_remove_object_id(
	sam_mount_t *mp,
	struct sam_disk_inode *dp)
{
	struct osd_iotask	*iotp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	sam_di_obj_t	*obj;
	int error;

	if ((error = osd_iotask_alloc(mp->mi.m_fs[dp->unit].oh, &iotp,
	    sizeof (sam_osd_iot_priv_t), OSD_SLEEP)) != OSD_SUCCESS) {
		return (EINVAL);
	}
	obj = (sam_di_obj_t *)(void *)&dp->extent[0];
	osd_setup_REMOVE(iotp, (uint64_t)obj->user_id);
	iotp->ot_partition_id = partid;
	iotp->ot_iodone = sam_object_req_done;
	sam_osd_setup_PRIVATE(iotp);

	if ((error = osd_iotask_start(iotp)) == OSD_SUCCESS) {
		sam_osd_io_task_WAIT(iotp);
		if (iotp->ot_error != OSD_SUCCESS) {
			error = sam_osd_errno(iotp);
		}
	} else {
		error = sam_osd_errno(iotp);
	}
	sam_osd_remove_PRIVATE(iotp);
	(void) osd_iotask_free(iotp);
	return (error);
}


/*
 * ----- sam_get_user_object_attr - Process get attribute call.
 */
int
sam_get_user_object_attr(
	sam_mount_t *mp,
	struct sam_disk_inode *dp,
	uint32_t	attr_num,	/* Attribute number */
	int64_t		*attrp)		/* Returned attribute */
{
	struct osd_iotask	*iotp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	sam_out_get_osd_attr_list_t out_attr;
	sam_out_get_osd_attr_list_t *out_attrp = &out_attr;
	sam_in_get_osd_attr_list_t	*in_attrp;
	sam_di_obj_t	*obj;
	int 						error;

	if ((error = osd_iotask_alloc(mp->mi.m_fs[dp->unit].oh, &iotp,
	    sizeof (sam_osd_iot_priv_t), OSD_SLEEP)) != OSD_SUCCESS) {
		return (EINVAL);
	}
	out_attrp = (sam_out_get_osd_attr_list_t *)kmem_zalloc
	    (sizeof (sam_out_get_osd_attr_list_t), KM_SLEEP);
	sam_format_get_attr_list(out_attrp,
	    OSD_USER_OBJECT_INFORMATION_PAGE, attr_num);
	in_attrp = (sam_in_get_osd_attr_list_t *)kmem_zalloc
	    (sizeof (sam_in_get_osd_attr_list_t), KM_SLEEP);

	obj = (sam_di_obj_t *)(void *)&dp->extent[0];
	osd_setup_GET_ATTRIBUTES(iotp, (uint64_t)obj->user_id);
	iotp->ot_partition_id = partid;
	iotp->ot_iodone = sam_object_req_done;
	iotp->ot_opts = OSD_O_LISTFMT;

	if (error = sam_osd_bp_from_kva((char *)out_attrp,
	    sizeof (sam_out_get_osd_attr_list_t), B_WRITE,
	    &iotp->ot_out_get_attrs_bp)) {
		goto fini1;
	}
	if (error = sam_osd_bp_from_kva((char *)in_attrp,
	    sizeof (sam_in_get_osd_attr_list_t), B_READ,
	    &iotp->ot_in_ret_attrs_bp)) {
		goto fini;
	}
	sam_osd_setup_PRIVATE(iotp);
	if ((error = osd_iotask_start(iotp)) == OSD_SUCCESS) {
		sam_osd_io_task_WAIT(iotp);
		if (iotp->ot_error == OSD_SUCCESS) {
			/*
			 * length = sam_get_8_byte_attr_val(
			 * (char *)in_attrp,0x82);
			 */
			memcpy((char *)attrp, &in_attrp->value[0], 8);
			NTOHLL(*attrp);
		} else {
			error = sam_osd_errno(iotp);
			dcmn_err((CE_WARN,
			    "SAM-QFS: %s: OSD get attr %x err=%d.%d, ino=%d,"
			    " getattr = 0x%llx", mp->mt.fi_name, attr_num,
			    error, iotp->ot_error, dp->id.ino,
			    (long long)*attrp));
		}
	} else {
		error = sam_osd_errno(iotp);
	}
	sam_osd_remove_PRIVATE(iotp);
	sam_osd_release_bp(iotp->ot_in_ret_attrs_bp,
	    (char *)in_attrp,
	    sizeof (sam_in_get_osd_attr_list_t), B_READ);
fini:
	sam_osd_release_bp(iotp->ot_out_get_attrs_bp,
	    (char *)out_attrp,
	    sizeof (osd_list_type_retrieve_t), B_WRITE);
fini1:
	(void) osd_iotask_free(iotp);
	kmem_free((char *)out_attrp, sizeof (sam_out_get_osd_attr_list_t));
	kmem_free((char *)in_attrp, sizeof (sam_in_get_osd_attr_list_t));
	return (error);
}


/*
 * ----- sam_format_get_attr_list - Param for a single attribute.
 */
static void
sam_format_get_attr_list(
	sam_out_get_osd_attr_list_t *listp,
	uint32_t attr_page,
	uint32_t attr_num)
{
	osd_attributes_list_t		*out_hdr;
	osd_list_type_retrieve_t    *out_listp;

	out_hdr = (osd_attributes_list_t *)&listp->attr_list;
	out_hdr->oal_list_type = OSD_LIST_TYPE_RETRIEVE;
	out_hdr->oal_list_length = 0;

	out_listp = (osd_list_type_retrieve_t *)&listp->entry;
	out_listp->oltr_attributes_page  = attr_page;
	out_listp->oltr_attribute_number = attr_num;
}


#if SAM_ATTR_LIST
/*
 * ----- sam_get_attr_addr - Search an attribute value list for the
 * specified attribute number and return a pointer to the list entry.
 */
static char *
sam_get_attr_addr(
	osd_attributes_list_t *list,
	uint32_t attr_num)
{
	uint32_t n_anum = HTONL(attr_num);
	char *cur	= (char *)list + sizeof (osd_attributes_list_t);
	char *last	= cur + NTOHS(list->oal_list_length);
	osd_list_type_retset_t	temp;

	while (cur < last) {
		temp.entry = *((osd_list_type_retset_t *)cur);
		if (temp.entry.oltrs_attribute_number == n_anum) {
			return (cur);
		}

		/*
		 * Make sure attribute is valid
		 */
		if (temp.entry.oltrs_attribute_length != 0xffff) {
			cur += NTOHS(temp.entry.oltrs_attribute_length);
		}
		cur += sizeof (osd_list_type_retset_t);
	}
	return (NULL);
}


/*
 * ----- sam_get_8_byte_attr_val - Get a single 8-byte length object
 * attribute value from a data-in buffer.
 */
static uint64_t
sam_get_8_byte_attr_val(
	char *listp,
	uint32_t attr_num)
{
	uint64_t	rc = 0;
	char *valp;

	valp = sam_get_attr_addr((osd_attributes_list_t *)listp, attr_num);
	if (valp != NULL) {
		memcpy(&rc, valp, 8);
		NTOHLL(rc);
	}
	return (rc);
}
#endif /* SAM_ATTR_LIST */


/*
 * ----- sam_set_user_object_attr - Process set user object attribute call.
 */
int
sam_set_user_object_attr(
	sam_mount_t *mp,
	struct sam_disk_inode *dp,
	uint32_t	attr_num,	/* Attribute number */
	int64_t		attribute)	/* Attribute */
{
	struct osd_iotask	*iotp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	sam_osd_attr_page_t	attr;
	sam_osd_attr_page_t	*attrp = &attr;
	sam_di_obj_t	*obj;
	int 			error;

	if ((error = osd_iotask_alloc(mp->mi.m_fs[dp->unit].oh, &iotp,
	    sizeof (sam_osd_iot_priv_t), OSD_SLEEP)) != OSD_SUCCESS) {
		return (EINVAL);
	}
	obj = (sam_di_obj_t *)(void *)&dp->extent[0];
	osd_setup_SET_ATTRIBUTES(iotp, (uint64_t)obj->user_id);
	attrp = kmem_zalloc(sizeof (sam_osd_attr_page_t), KM_SLEEP);
	memcpy(&attrp->value, &attribute, 8);
	iotp->ot_set_attributes_page = OSD_USER_OBJECT_INFORMATION_PAGE;
	iotp->ot_set_attribute_number = attr_num;
	iotp->ot_opts = OSD_O_PAGEFMT;

	iotp->ot_partition_id = partid;
	iotp->ot_iodone = sam_object_req_done;

	if (error = sam_osd_bp_from_kva((char *)attrp, sizeof (offset_t),
	    B_WRITE, &iotp->ot_out_set_attrs_bp)) {
		goto fini;
	}
	sam_osd_setup_PRIVATE(iotp);
	if ((error = osd_iotask_start(iotp)) == OSD_SUCCESS) {
		sam_osd_io_task_WAIT(iotp);
		if (iotp->ot_error != OSD_SUCCESS) {
			error = sam_osd_errno(iotp);
			dcmn_err((CE_WARN, "SAM-QFS: %s: OSD set attr %x"
			    " err=%d.%d, ino=%d, setattr = 0x%llx",
			    mp->mt.fi_name, attr_num, error, iotp->ot_error,
			    dp->id.ino, (long long)attribute));
		}
	} else {
		error = sam_osd_errno(iotp);
	}
	sam_osd_remove_PRIVATE(iotp);
	sam_osd_release_bp(iotp->ot_out_set_attrs_bp, (char *)attrp,
	    sizeof (offset_t), B_WRITE);
fini:
	(void) osd_iotask_free(iotp);
	kmem_free(attrp, sizeof (sam_osd_attr_page_t));
	return (error);
}


#else /* SAM_OSD_SUPPORT */

/*
 * ----- sam_open_osd_device - Open an osd device. Return object handle.
 */
/* ARGSUSED */
int
sam_open_osd_device(
	struct samdent *dp,	/* Pointer to device entr */
	int filemode,		/* Filemode for open */
	cred_t *credp)		/* Credentials pointer. */
{
	return (EINVAL);
}


/*
 * ----- sam_close_osd_device - Close an osd device.
 */
/* ARGSUSED */
void
sam_close_osd_device(
	void *oh,			/* Object device handle */
	int filemode,		/* Filemode for open */
	cred_t *credp)		/* Credentials pointer. */
{
}


/*
 * ----- sam_issue_object_io - Issue object I/O.
 */
/* ARGSUSED */
int
sam_issue_object_io(
	void *oh,
	uint32_t command,
	uint64_t user_obj_id,
	enum uio_seg seg,
	char *data,
	offset_t offset,
	offset_t length)
{
	return (EINVAL);
}


/*
 * ----- sam_issue_direct_object_io - Issue object direct async I/O.
 */
/* ARGSUSED */
int
sam_issue_direct_object_io(
	sam_node_t *ip,
	enum uio_rw rw,
	struct buf *bp,
	offset_t contig,
	offset_t cur_loffset)
{
	return (EIO);
}


/*
 * ----- sam_pageio_object - Start object I/O on a page.
 */
/* ARGSUSED */
buf_t *
sam_pageio_object(
	sam_node_t *ip,		/* Pointer to inode table */
	sam_ioblk_t *iop,	/* Pointer to I/O entry */
	page_t *pp,		/* Pointer to page list */
	offset_t offset,	/* Logical file offset */
	uint_t pg_off,		/* Offset within page */
	uint_t vn_len,		/* Length of transfer */
	int flags)		/* Flags --B_INVAL, B_DIRTY, B_FREE, */
				/*   B_DONTNEED, B_FORCE, B_ASYNC. */
{
	return (NULL);
}


/*
 * ----- sam_pg_object_sync_done - Process object page sync I/O completion.
 */
/* ARGSUSED */
int
sam_pg_object_sync_done(
	sam_node_t *ip,		/* Pointer to inode table */
	buf_t	*bp,		/* Pointer to the buffer */
	char	*str)		/* "GETPAGE" OR "PUTPAGE" */
{
	return (EIO);
}


/*
 * ----- sam_create_priv_object_id - Process create of privileged object id.
 */
/* ARGSUSED */
int
sam_create_priv_object_id(
	void		*oh,
	uint64_t	user_obj_id)
{
	return (EINVAL);
}


/*
 * ----- sam_create_object_id - Process create_object id.
 */
/* ARGSUSED */
int
sam_create_object_id(
	struct sam_mount *mp,
	struct sam_disk_inode *dp)
{
	return (EINVAL);
}


/*
 * ----- sam_remove_object_id - Process remove_object id.
 */
/* ARGSUSED */
int
sam_remove_object_id(
	struct sam_mount *mp,
	struct sam_disk_inode *dp)
{
	return (EINVAL);
}


/*
 * ----- sam_get_user_object_attr - Process get attribute call.
 */
/* ARGSUSED */
int
sam_get_user_object_attr(
	sam_mount_t *mp,
	struct sam_disk_inode *dp,
	uint32_t	attr_num,	/* Attribute number */
	int64_t		*attrp)		/* Returned attribute */
{
	return (EINVAL);
}


/*
 * ----- sam_set_user_object_attr - Process set user object attribute call.
 */
/* ARGSUSED */
int
sam_set_user_object_attr(
	sam_mount_t *mp,
	struct sam_disk_inode *dp,
	uint32_t	attr_num,	/* Attribute number */
	int64_t		attribute)	/* Attribute */
{
	return (EINVAL);
}
#endif /* SAM_OSD_SUPPORT */
