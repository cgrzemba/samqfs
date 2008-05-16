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

#pragma ident "$Revision: 1.10 $"

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
#include <sys/scsi/scsi_osd.h>
#define	L8R
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
 *
 * osd_handle_by_name
 * osd_close
 * osd_setup_write
 * osd_setup_read
 * osd_setup_write_bp
 * osd_setup_read_bp
 * osd_setup_get_page_attr
 * osd_setup_set_page_attr
 * osd_submit_req
 * osd_free_req
 */

/*
 * Remove the following after the device driver is in Solaris 11
 */
char _depends_on[] = "drv/sosd drv/scsi misc/scsi_osd";

static int sam_osd_bp_from_kva(caddr_t memp, size_t memlen, int rw,
	struct buf **bpp);
static void sam_osd_release_bp(struct buf *bp, caddr_t memp, size_t memlen,
	int rw);
#if SAM_ATTR_LIST
static void sam_format_get_attr_list(sam_out_get_osd_attr_list_t *listp,
	uint32_t attr_page, uint32_t attr_num);
static char *sam_get_attr_addr(osd_attributes_list_t *list,
	uint32_t attr_num);
static uint64_t sam_get_8_byte_attr_val(char *listp, uint32_t attr_num);
#endif /* SAM_ATTR_LIST */

static void sam_object_req_done(osd_req_t *reqp, void *ct_priv,
	osd_result_t *resp);
static void sam_dk_object_done(osd_req_t *reqp, void *ct_priv,
	osd_result_t *resp);
static void sam_pg_object_done(osd_req_t *reqp, void *ct_priv,
	osd_result_t *resp);
static int sam_osd_errno(int rc, sam_osd_req_priv_t *iorp);


/*
 * ----- sam_open_osd_device - Open an osd device. Return object handle.
 */
int
sam_open_osd_device(
	struct samdent *dp,	/* Pointer to device entr */
	int filemode,		/* Filemode for open */
	cred_t *credp)		/* Credentials pointer. */
{
	sam_osd_handle_t oh;
	vnode_t *svp;
	dev_t dev;
	int rc, error;

	error = lookupname(dp->part.pt_name, UIO_SYSSPACE, FOLLOW, NULL, &svp);
	if (error) {
		return (ENODEV);
	}
	if (svp->v_type != VCHR) {
		VN_RELE(svp);
		return (EINVAL);	/* Must be type chr */
	}
	dev = svp->v_rdev;
	VN_RELE(svp);
	rc = osd_handle_by_name(dp->part.pt_name, filemode, credp, &oh);
	if (rc != OSD_SUCCESS) {
		error = sam_osd_errno(rc, NULL);
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
	sam_osd_handle_t oh,	/* Object device handle */
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
	sam_osd_handle_t oh,
	uint32_t command,
	uint64_t obj_id,
	enum uio_seg seg,
	char *data,
	offset_t offset,
	offset_t length)
{
	sam_osd_req_priv_t	ior_priv;
	sam_osd_req_priv_t	*iorp = &ior_priv;
	osd_req_t		*reqp;
	iovec_t			iov;
	char			*bufp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	int			rc;
	int			error = 0;

	if (seg == UIO_USERSPACE) {
		bufp = kmem_alloc(length, KM_SLEEP);
		if (!bufp) {
			return (ENOMEM);
		}
	} else {
		bufp = data;
	}
	iov.iov_base = (void *)bufp;
	iov.iov_len = (long)length;
	if (command == FWRITE) {
		reqp = osd_setup_write(oh, partid, obj_id, length, offset, 1,
		    (caddr_t)&iov);
		if (reqp == NULL) {
			error = EINVAL;
			goto fini;
		}
		if (seg == UIO_USERSPACE) {
			if (copyin(data, bufp, length)) {
				error = EFAULT;
				goto fini;
			}
		}
	} else {
		reqp = osd_setup_read(oh, partid, obj_id, length, offset, 1,
		    (caddr_t)&iov);
		if (reqp == NULL) {
			error = EINVAL;
			goto fini;
		}
	}
	sam_osd_setup_private(iorp);
	rc = osd_submit_req(reqp, 0, sam_object_req_done, iorp);
	if (rc == OSD_SUCCESS) {
		sam_osd_obj_req_wait(iorp);	/* Wait for completion */
		rc = iorp->result.err_code;
		if (rc == OSD_SUCCESS) {
			if (command == FREAD && seg == UIO_USERSPACE) {
				if (copyout(bufp, data, length)) {
					error = EFAULT;
				}
			}
		} else {
			error = sam_osd_errno(rc, iorp);
		}
	} else {
		error = sam_osd_errno(rc, iorp);
	}
	sam_osd_remove_private(iorp);
fini:
	if (seg == UIO_USERSPACE) {
		kmem_free(bufp, length);
	}
	if (reqp) {
		(void) osd_free_req(reqp);
	}
	return (error);
}


/*
 * ----- sam_object_req_done - Process object osd request completion.
 *
 * This is the interrupt routine. Called when the object request completes.
 */

/* ARGSUSED0 */
static void
sam_object_req_done(
	osd_req_t *reqp,
	void *ct_priv,
	osd_result_t *resp)
{
	sam_osd_req_priv_t *iorp = (sam_osd_req_priv_t *)ct_priv;

	iorp->result = *resp;
	if (resp->err_type == OSD_ERRTYPE_RESID) {
		osd_resid_t *rp = (osd_resid_t *)resp->errp;

		iorp->resid = *rp;
	}
	sam_osd_obj_req_done(iorp);	/* Wakeup caller */
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
	sam_osd_handle_t	oh;
	sam_osd_req_priv_t	*iorp;
	osd_req_t		*reqp;
	uint64_t		user_object_id;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	sam_di_obj_t		*obj;
	int			rc;
	int			error = 0;


	iorp = (sam_osd_req_priv_t *)kmem_zalloc(sizeof (sam_osd_req_priv_t),
	    KM_SLEEP);
	obj = (sam_di_obj_t *)(void *)&ip->di.extent[2];
	user_object_id = obj->user_id;
	oh = ip->mp->mi.m_fs[ip->di.unit].oh;
	if (rw == UIO_WRITE) {
		reqp = osd_setup_write_bp(oh, partid, user_object_id,
		    contig, cur_loffset, bp);
		if (reqp == NULL) {
			kmem_free(iorp, sizeof (sam_osd_req_priv_t));
			return (EINVAL);
		}
		if ((cur_loffset + contig) > obj->wr_size) {
			obj->wr_size = (cur_loffset + contig);
		}
		ASSERT(contig <= SAM_OSD_MAX_WR_CONTIG);
	} else {
		reqp = osd_setup_read_bp(oh, partid, user_object_id,
		    contig, cur_loffset, bp);
		if (reqp == NULL) {
			kmem_free(iorp, sizeof (sam_osd_req_priv_t));
			return (EINVAL);
		}
		ASSERT(contig <= SAM_OSD_MAX_RD_CONTIG);
	}
	iorp->ip = ip;
	iorp->bp = bp;
	rc = osd_submit_req(reqp, 0, sam_dk_object_done, iorp);
	if (rc != OSD_SUCCESS) {
		osd_result_t result;

		result.err_code = (uint8_t)rc;
		result.err_type = OSD_ERRTYPE_NONE;
		sam_dk_object_done(reqp, iorp, &result);
		error = sam_osd_errno(result.err_code, NULL);
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
	osd_req_t *reqp,
	void *ct_priv,
	osd_result_t *resp)
{
	sam_osd_req_priv_t	*iorp = (sam_osd_req_priv_t *)ct_priv;
	sam_node_t		*ip;
	buf_descriptor_t	*bdp;
	sam_buf_t  		*sbp;
	buf_t			*bp;
	offset_t 		count;
	offset_t		offset;
	sam_di_obj_t		*obj;
	boolean_t		async;


	bp = (buf_t *)iorp->bp;
	sbp = (sam_buf_t *)bp;
	bdp = (buf_descriptor_t *)(void *)bp->b_vp;
	ip = bdp->ip;
	obj = (sam_di_obj_t *)(void *)&ip->di.extent[2];
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
		sema_v(&bdp->io_sema);	/* Increment completed I/Os */
	}
	bdp->io_count--;		/* Decrement number of issued I/O */
	TRACE((bp->b_flags & B_READ ? T_SAM_DIORDOBJ_COMP :
	    T_SAM_DIOWROBJ_COMP), SAM_ITOV(ip), (sam_tr_t)iorp,
	    offset, count);

	/*
	 * Check for errors.
	 */
	iorp->result = *resp;
	if (resp->err_code != 0) {
		bdp->error = sam_osd_errno(resp->err_code, iorp);
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: DK er=%d, ip=%p, ino=%d, sa=%x, off=%x"
		    " len=%x r=%x sz=%x %x %x",
		    ip->mp->mt.fi_name, resp->err_code, (void *)ip,
		    ip->di.id.ino, resp->service_action, offset,
		    bp->b_bcount, bp->b_resid, (offset+bp->b_bcount),
		    obj->wr_size, ip->size);
	}
	mutex_exit(&bdp->bdp_mutex);

	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS|B_SHADOW);
	sam_free_buf_header((sam_uintptr_t *)bp);
	(void) osd_free_req(reqp);
	kmem_free(iorp, sizeof (sam_osd_req_priv_t));

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
	sam_osd_handle_t	oh;
	sam_osd_req_priv_t	*iorp;
	osd_req_t		*reqp;
	sam_mount_t		*mp;	/* Pointer to mount table */
	buf_t 			*bp;
	uint64_t		user_object_id;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	offset_t		high_addr;
	sam_di_obj_t		*obj;
	int			rc;


	mp = ip->mp;
	oh = ip->mp->mi.m_fs[ip->di.unit].oh;
	iorp = (sam_osd_req_priv_t *)kmem_zalloc(sizeof (sam_osd_req_priv_t),
	    KM_SLEEP);

	bp = pageio_setup(pp, vn_len, mp->mi.m_fs[iop->ord].svp, flags);
	bp->b_edev = mp->mi.m_fs[iop->ord].dev;
	bp->b_dev = cmpdev(bp->b_edev);
	bp->b_blkno = 0;
	bp->b_un.b_addr = (char *)pg_off;
	bp->b_file = SAM_ITOP(ip);
	bp->b_offset = offset;
	TRACE((bp->b_flags & B_READ) ? T_SAM_PGRDOBJ_ST :
	    T_SAM_PGWROBJ_ST, SAM_ITOV(ip), (sam_tr_t)iorp, offset, vn_len);
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
	obj = (sam_di_obj_t *)(void *)&ip->di.extent[2];
	user_object_id = obj->user_id;
	high_addr = (offset + vn_len);

	if (bp->b_flags & B_READ) {	/* If reading */
		reqp = osd_setup_read_bp(oh, partid, user_object_id,
		    vn_len, offset, bp);
		ASSERT(vn_len <= SAM_OSD_MAX_RD_CONTIG);
		/*
		 * Must wait to issue read until write (end of obj)
		 * has completed. Note, requests can complete out
		 * of order AND issue out of order.
		 */
	} else {
		boolean_t lock_set = B_FALSE;

		reqp = osd_setup_write_bp(oh, partid, user_object_id,
		    vn_len, offset, bp);
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
	bp->b_private = (void *)iorp;
	sam_osd_setup_private(iorp);
	iorp->bp = bp;
	iorp->ip = ip;
	iorp->reqp = reqp;
	rc = osd_submit_req(reqp, 0, sam_pg_object_done, iorp);
	if (rc != OSD_SUCCESS) {
		osd_result_t result;

		result.err_code = (uint8_t)rc;
		result.err_type = OSD_ERRTYPE_NONE;
		sam_pg_object_done(reqp, iorp, &result);
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
	osd_req_t *reqp,
	void *ct_priv,
	osd_result_t *resp)
{
	sam_osd_req_priv_t	*iorp = (sam_osd_req_priv_t *)ct_priv;
	sam_node_t 		*ip;
	buf_t			*bp;
	offset_t		count;
	offset_t		offset;
	sam_di_obj_t		*obj;


	ip = iorp->ip;
	obj = (sam_di_obj_t *)(void *)&ip->di.extent[2];
	bp = iorp->bp;
	offset = bp->b_offset;
	count = bp->b_bcount;
	if (bp->b_flags & B_READ) {
		TRACE(T_SAM_PGRDOBJ_COMP, SAM_ITOV(ip), (sam_tr_t)iorp,
		    bp->b_offset, bp->b_bcount);
	} else {
		TRACE(T_SAM_PGWROBJ_COMP, SAM_ITOV(ip), (sam_tr_t)iorp,
		    bp->b_offset, bp->b_bcount);
	}

	/*
	 * Check for errors.
	 */
	iorp->result = *resp;
	if (resp->err_code != OSD_SUCCESS) {
		bp->b_error = sam_osd_errno(resp->err_code, iorp);
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: PG er=%d %d, ip=%p ino=%d sa=%x off=%x"
		    " len=%x r=%x ha=%x %x %x %x",
		    ip->mp->mt.fi_name, resp->err_code, bp->b_error, ip,
		    ip->di.id.ino, resp->service_action, offset, count,
		    bp->b_resid, (offset+count), obj->wr_size, obj->cm_size,
		    ip->size);
	}
	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS|B_SHADOW);
	if (!(bp->b_flags & B_READ)) {	/* If writing */
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
		(void) osd_free_req(reqp);
		sam_osd_remove_private(iorp);
		kmem_free(iorp, sizeof (sam_osd_req_priv_t));
	} else {
		sam_osd_obj_req_done(iorp);	/* Wakeup caller */
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
	sam_osd_req_priv_t	*iorp;
	int			 error = 0;

	iorp = (sam_osd_req_priv_t *)bp->b_private;
	sam_osd_obj_req_wait(iorp);	/* Wait for completion */
	if (bp->b_error) {
		dcmn_err((CE_WARN,
		    "SAM-QFS: %s: %s ip=%p, ino=%d.%d"
		    " off=%x len=%x err=%d, iorp=%p", ip->mp->mt.fi_name,
		    str, (void *)ip, ip->di.id.ino, ip->di.id.gen,
		    bp->b_offset, bp->b_bcount, bp->b_error, iorp));
		if (error == 0) {
			error = bp->b_error;
		}
	}
	pageio_done(bp);
	(void) osd_free_req(iorp->reqp);
	sam_osd_remove_private(iorp);
	kmem_free(iorp, sizeof (sam_osd_req_priv_t));
	return (error);
}


/*
 * ----- sam_osd_errno - Return errno given OSD error.
 */
int
sam_osd_errno(
	int rc,
	sam_osd_req_priv_t *iorp)
{
	buf_t *bp = NULL;
	int error = 0;

	if (iorp) {
		bp = iorp->bp;
		if (bp) {
			bp->b_error = 0;
		}
	}

	switch (rc) {
	case OSD_SUCCESS:
		if (bp) {
			bp->b_resid = 0;
		}
		return (0);

	case OSD_RESIDUAL: {
		osd_resid_t *residp = (osd_resid_t *)iorp->result.errp;

		ASSERT(residp != NULL);
		if (residp == NULL) {
			return (EINVAL);
		}
		bp->b_resid = (bp->b_flags & B_READ) ?
		    residp->ot_in_command_resid :
		    residp->ot_out_command_resid;
		cmn_err(CE_NOTE,
		    "SAM-QFS: OSD RESID resid=%x, off=%llx, cnt=%llx",
		    bp->b_resid, (long long)bp->b_offset, bp->b_bcount);
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
	if (bp) {
		bp->b_error = error;	/* Return error in buffer */
		bp->b_flags |= B_ERROR;
	}
	return (error);
}


/*
 * ----- sam_osd_bp_from_kva - Setup the buf struct for the request.
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
 * ----- sam_create_priv_object_id - Process create of privileged object id.
 */
int
sam_create_priv_object_id(
	sam_osd_handle_t	oh,
	uint64_t		user_obj_id)
{
	sam_osd_req_priv_t	ior_priv;
	sam_osd_req_priv_t	*iorp = &ior_priv;
	osd_req_t 		*reqp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	int			rc;
	int			error = 0;

	reqp = osd_setup_create_object(oh, partid, user_obj_id, 1);
	if (reqp == NULL) {
		return (EINVAL);
	}
	sam_osd_setup_private(iorp);

	rc = osd_submit_req(reqp, 0, sam_object_req_done, iorp);
	if (rc == OSD_SUCCESS) {
		sam_osd_obj_req_wait(iorp);
		if (iorp->result.err_code != OSD_SUCCESS) {
			error = sam_osd_errno(iorp->result.err_code, iorp);
		}
	} else {
		error = sam_osd_errno(rc, iorp);
		ASSERT(error);
	}
	(void) osd_free_req(reqp);
	sam_osd_remove_private(iorp);
	return (error);
}


/*
 * ----- sam_create_object_id - Process create of user object id.
 */
int
sam_create_object_id(
	sam_mount_t *mp,
	struct sam_disk_inode *dp)
{
	sam_osd_req_priv_t	ior_priv;
	sam_osd_req_priv_t	*iorp = &ior_priv;
	uint64_t		user_object_id;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	uint64_t		numobj;
	uint64_t		pglistsize;
	uint64_t		*objlist;
	uint64_t		*objlist_save;
	osd_req_t		*reqp;
	int			rc;
	int			error = 0;

	ASSERT(mp->mi.m_fs[dp->unit].oh != 0);
	dp->version = SAM_INODE_VERSION;

	/*
	 * Set up the create request and add in the Get Page Attributes
	 * to get the list of object ids created.
	 */
	numobj = 1;
	pglistsize = sizeof (struct sam_objlist_page) +
	    ((sizeof (uint64_t) * numobj));
	objlist = (uint64_t *)kmem_alloc(pglistsize, KM_SLEEP);
	if (!objlist) {
		return (ENOMEM);
	}
	objlist_save = objlist;
	reqp = osd_setup_create_object(mp->mi.m_fs[dp->unit].oh, partid,
	    0, (uint64_t)numobj);
	if (reqp == NULL) {
		error = EINVAL;
		goto fini;
	}

	error = osd_add_get_page_attr_to_req(reqp,
	    OSD_SAMQFS_VENDOR_CREATED_OBJECTS_LIST_PAGE,
	    pglistsize, (void *) objlist);
	if (error) {
		error = EINVAL;
		goto fini;
	}

	sam_osd_setup_private(iorp);
	rc = osd_submit_req(reqp, 0, sam_object_req_done, iorp);
	if (rc == OSD_SUCCESS) {
		sam_osd_obj_req_wait(iorp);
		if (iorp->result.err_code == OSD_SUCCESS) {
			sam_di_obj_t	*obj;
			sam_id_attr_t	x;

			dp->rm.ui.flags |= RM_OBJECT_FILE;
			obj = (sam_di_obj_t *)(void *)&dp->extent[2];
			/*
			 * Skip sam_objlist_page header.
			 */
			objlist++;
			obj->user_id = *objlist;
			/*
			 * Replace OSD_USER_OBJECT_INFORMATION_USERNAME
			 * with QFS vendor specific attribute for ino/gen.
			 */
			x.id.ino = BE_32(dp->id.ino);
			x.id.gen = BE_32(dp->id.gen);
			error = sam_set_user_object_attr(mp, dp,
			    OSD_USER_OBJECT_INFORMATION_USERNAME, x.attr);
		} else {
			error = sam_osd_errno(iorp->result.err_code, iorp);
			ASSERT(error);
		}
	} else {
		error = sam_osd_errno(rc, iorp);
		ASSERT(error);
	}
	sam_osd_remove_private(iorp);
fini:
	kmem_free(objlist_save, pglistsize);
	if (reqp) {
		(void) osd_free_req(reqp);
	}
	return (error);
}


/*
 * ----- sam_remove_object_id - Process remove_object id.
 */
int
sam_remove_object_id(
	sam_mount_t *mp,
	uint64_t user_obj_id,
	uchar_t unit)
{
	sam_osd_req_priv_t	ior_priv;
	sam_osd_req_priv_t	*iorp = &ior_priv;
	osd_req_t		*reqp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	int			rc;
	int			error = 0;

	reqp = osd_setup_remove_object(mp->mi.m_fs[unit].oh, partid,
	    user_obj_id);
	if (reqp == NULL) {
		return (EINVAL);
	}
	sam_osd_setup_private(iorp);

	rc = osd_submit_req(reqp, 0, sam_object_req_done, iorp);
	if (rc == OSD_SUCCESS) {
		sam_osd_obj_req_wait(iorp);
		if (iorp->result.err_code != OSD_SUCCESS) {
			error = sam_osd_errno(iorp->result.err_code, iorp);
		}
	} else {
		error = sam_osd_errno(rc, iorp);
		ASSERT(error);
	}
	(void) osd_free_req(reqp);
	sam_osd_remove_private(iorp);
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
	sam_osd_req_priv_t	ior_priv;
	sam_osd_req_priv_t	*iorp = &ior_priv;
	osd_req_t		*reqp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	sam_objinfo_page_t	*sopp;
	sam_di_obj_t		*obj;
	int			rc;
	int			error = 0;

	obj = (sam_di_obj_t *)(void *)&dp->extent[2];

	reqp = osd_setup_get_attr(mp->mi.m_fs[dp->unit].oh, partid,
	    (uint64_t)obj->user_id);
	if (reqp == NULL) {
		return (EINVAL);
	}
	error = osd_add_get_page_attr_to_req(reqp,
	    OSD_SAMQFS_VENDOR_USERINFO_ATTR_PAGE_NO,
	    sizeof (sam_objinfo_page_t), (void *) attrp);
	if (error) {
		error = EINVAL;
		goto fini;
	}
	sam_osd_setup_private(iorp);
	rc = osd_submit_req(reqp, 0, sam_object_req_done, iorp);
	if (rc == OSD_SUCCESS) {
		sam_osd_obj_req_wait(iorp);
		if (iorp->result.err_code != OSD_SUCCESS) {
			error = sam_osd_errno(iorp->result.err_code, iorp);
			dcmn_err((CE_WARN, "SAM-QFS: %s: "
			    "OSD GET attr %x"
			    " err=%d, errno=%d, ino=%d, setattr = 0x%llx",
			    mp->mt.fi_name, attr_num, iorp->result.err_code,
			    error, dp->id.ino, *attrp));
		}
	} else {
		error = sam_osd_errno(rc, iorp);
	}
	sam_osd_remove_private(iorp);

fini:
	if (reqp) {
		(void) osd_free_req(reqp);
	}
	return (error);
}

#if SAM_ATTR_LIST

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
	char *cur = (char *)list + sizeof (osd_attributes_list_t);
	char *last = cur + NTOHS(list->oal_list_length);
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
	sam_osd_req_priv_t	ior_priv;
	sam_osd_req_priv_t	*iorp = &ior_priv;
	osd_req_t		*reqp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	sam_di_obj_t		*obj;
	int			rc;
	int			error = 0;

	obj = (sam_di_obj_t *)(void *)&dp->extent[2];
	reqp = osd_setup_set_attr(mp->mi.m_fs[dp->unit].oh, partid,
	    (uint64_t)obj->user_id);
	if (reqp == NULL) {
		return (EINVAL);
	}
	error = osd_add_set_page_attr_cdb(reqp,
	    OSD_USER_OBJECT_INFORMATION_PAGE, attr_num, sizeof (attribute),
	    (char *)&attribute);
	if (error) {
		error = EINVAL;
		goto fini;
	}
	sam_osd_setup_private(iorp);
	rc = osd_submit_req(reqp, 0, sam_object_req_done, iorp);
	if (rc == OSD_SUCCESS) {
		sam_osd_obj_req_wait(iorp);
		if (iorp->result.err_code != OSD_SUCCESS) {
			error = sam_osd_errno(iorp->result.err_code, iorp);
			dcmn_err((CE_WARN, "SAM-QFS: %s: OSD SET attr %x"
			    " err=%d.%d, ino=%d, setattr = 0x%llx",
			    mp->mt.fi_name, attr_num, iorp->result.err_code, rc,
			    dp->id.ino, (long long)attribute));
		}
	} else {
		error = sam_osd_errno(rc, iorp);
	}
	sam_osd_remove_private(iorp);
fini:
	if (reqp) {
		(void) osd_free_req(reqp);
	}

	return (error);
}


/*
 * ----- sam_truncate_object_file - Truncate an osd file.
 */
int
sam_truncate_object_file(
	sam_node_t *ip,
	sam_truncate_t tflag,	/* Truncate file or Release file */
	offset_t length)	/* New file size */
{
	sam_di_obj_t *obj  = (sam_di_obj_t *)(void *)&ip->di.extent[2];
	int offline = ip->di.status.b.offline;
	offset_t size = ip->di.rm.size;
	uchar_t	unit = ip->di.unit;
	struct sam_objinfo_page sop;
	uint64_t user_obj_id;
	int error;

	ASSERT(obj->user_id != 0);
	user_obj_id = obj->user_id;

	/*
	 * Sync the inode to make sure the size is updated on disk.
	 * If error syncing inode, restore previous state.
	 */
	ASSERT(tflag != SAM_REDUCE);
	if ((error = sam_sync_inode(ip, length, tflag))) {
		if (tflag == SAM_PURGE) {
			return (EIO);
		}

		obj->user_id = user_obj_id;
		ip->di.status.b.offline = offline;
		ip->di.rm.size = size;
		ip->di.unit = unit;
		return (EIO);
	}

	/*
	 * obj->user_id and ip->di.unit get zeroed in sam_sync_inode().
	 */
	if (length == 0) {
		if (tflag == SAM_PURGE) {
			(void) sam_remove_object_id(ip->mp, user_obj_id, unit);
			return (0);
		}
	}

	(void) sam_get_user_object_attr(ip->mp, &ip->di, 0, (void *)&sop);
	error = sam_set_user_object_attr(ip->mp, &ip->di,
	    OSD_USER_OBJECT_INFORMATION_LOGICAL_LENGTH,
	    BE_64((int64_t)length));

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
	sam_osd_handle_t oh,	/* Object device handle */
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
	sam_osd_handle_t oh,
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
	sam_osd_handle_t	oh,
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
	uint64_t user_obj_id,
	uchar_t unit)
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


/*
 * ----- sam_truncate_object_file - Truncate an osd file.
 */
/* ARGSUSED */
int
sam_truncate_object_file(
	sam_node_t *ip,
	sam_truncate_t tflag,	/* Truncate file or Release file */
	offset_t length)
{
	return (EINVAL);
}

#endif /* SAM_OSD_SUPPORT */
