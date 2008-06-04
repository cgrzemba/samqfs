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

#pragma ident "$Revision: 1.12 $"

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

#define	SAM_OSD_STRIPE_DEPTH	128	/* Stripe depth default in kilobytes */
#define	SAM_OSD_STRIPE_SHIFT	17	/* Stripe shift for stripe depth */

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
static int sam_get_user_object_attr(struct sam_mount *mp,
	struct sam_disk_inode *dp, int n, uint32_t attr_num, int64_t *attrp);
static int sam_set_user_object_attr(struct sam_mount *mp,
	struct sam_disk_inode *dp, int n, uint32_t attr_num, int64_t attribute);

static void sam_object_req_done(osd_req_t *reqp, void *ct_priv,
	osd_result_t *resp);
static void sam_dk_object_done(osd_req_t *reqp, void *ct_priv,
	osd_result_t *resp);
static void sam_pg_object_done(osd_req_t *reqp, void *ct_priv,
	osd_result_t *resp);
static int sam_osd_errno(int rc, sam_osd_req_priv_t *iorp);
static uint64_t sam_decode_scsi_buf(char *scsi_buf, int scsi_len);


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
 * ----- sam_get_osd_fs_attr - Process get file system attribute call.
 * Set capacity and space in superblock equipment entry.
 */
int
sam_get_osd_fs_attr(
	sam_mount_t *mp,
	sam_osd_handle_t oh,
	struct sam_sbord *sop)
{
	sam_osd_req_priv_t	ior_priv;
	sam_fsinfo_page_t	fs_attr;
	sam_osd_req_priv_t	*iorp = &ior_priv;
	osd_req_t		*reqp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	int			rc;
	int			error = 0;


	reqp = osd_setup_get_attr(oh, partid, SAM_OBJ_SBLK_ID);
	if (reqp == NULL) {
		return (EINVAL);
	}
	error = osd_add_get_page_attr_to_req(reqp,
	    OSD_SAMQFS_VENDOR_FS_ATTR_PAGE_NO,
	    sizeof (sam_fsinfo_page_t), (void *) &fs_attr);
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
			    "OSD GET FS err=%d, errno=%d",
			    mp->mt.fi_name, iorp->result.err_code, error));
		} else {
			sam_fsinfo_page_t *fap = &fs_attr;

			sop->capacity = SFP_CAPACITY(fap);
			sop->space = SFP_SPACE(fap);
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
		osd_resid_t *rp = (osd_resid_t *)resp->resid_data;

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
	sam_ioblk_t *iop,
	offset_t contig)
{
	int			obji = iop->obji;
	offset_t		offset = iop->blk_off + iop->pboff;
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
	if (!iorp) {
		return (ENOMEM);
	}

	obj = (sam_di_obj_t *)(void *)&ip->di.extent[2];
	user_object_id = obj->ol[obji].obj_id;
	oh = ip->mp->mi.m_fs[obj->ol[obji].ord].oh;
	if (rw == UIO_WRITE) {
		reqp = osd_setup_write_bp(oh, partid, user_object_id,
		    contig, offset, bp);
	} else {
		reqp = osd_setup_read_bp(oh, partid, user_object_id,
		    contig, offset, bp);
	}
	if (reqp == NULL) {
		kmem_free(iorp, sizeof (sam_osd_req_priv_t));
		return (EINVAL);
	}
	iorp->obji = obji;	/* Object layout index */
	iorp->offset = offset;	/* Logical offset in I/O request */
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
	int			obji;


	bp = (buf_t *)iorp->bp;
	sbp = (sam_buf_t *)bp;
	bdp = (buf_descriptor_t *)(void *)bp->b_vp;
	ip = bdp->ip;
	obj = (sam_di_obj_t *)(void *)&ip->di.extent[2];
	obji = iorp->obji;
	async = (bdp->aiouiop != NULL);
	bp->b_vp = NULL;
	bp->b_iodone = NULL;
	count = bp->b_bcount;
	offset = iorp->offset;
	if (bp->b_flags & B_REMAPPED) {
		bp_mapout(bp);
	}

	mutex_enter(&bdp->bdp_mutex);
	SAM_BUF_DEQ(&sbp->link);
	if (!async) {
		sema_v(&bdp->io_sema);	/* Increment completed I/Os */
	}
	bdp->io_count--;		/* Decrement number of issued I/O */
	mutex_exit(&bdp->bdp_mutex);

	/*
	 * If no error, update end of object (eoo). Otherwise, set error.
	 */
	iorp->result = *resp;
	if (bp->b_flags & B_READ) {	/* If reading */
		TRACE(T_SAM_DIORDOBJ_COMP, SAM_ITOV(ip), (sam_tr_t)iorp,
		    offset, (sam_tr_t)(((offset_t)obji << 32)|count));
	} else {			/* If writing */
		TRACE(T_SAM_DIOWROBJ_COMP, SAM_ITOV(ip), (sam_tr_t)iorp,
		    offset, (sam_tr_t)(((offset_t)obji << 32)|count));
		if (resp->err_code == OSD_SUCCESS) {
			mutex_enter(&ip->write_mutex);
			if ((offset + count) > obj->ol[obji].eoo) {
				obj->ol[obji].eoo = (offset + count);
			}
			mutex_exit(&ip->write_mutex);
		}
	}
	if (resp->err_code != 0) {
		bdp->error = sam_osd_errno(resp->err_code, iorp);
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: DK er=%d, ip=%p, ino=%d, sa=%x, off=%llx"
		    " len=%x r=%x sz=%llx %llx %llx",
		    ip->mp->mt.fi_name, resp->err_code, (void *)ip,
		    ip->di.id.ino, resp->service_action, offset,
		    bp->b_bcount, bp->b_resid, (offset + bp->b_bcount),
		    obj->ol[obji].eoo, ip->size);
	}

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
	offset_t		count;
	uint64_t		user_object_id;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	sam_di_obj_t		*obj;
	int			rc;

	iorp = (sam_osd_req_priv_t *)kmem_zalloc(sizeof (sam_osd_req_priv_t),
	    KM_SLEEP);
	if (!iorp) {
		return (NULL);
	}

	mp = ip->mp;
	count = vn_len;
	obj = (sam_di_obj_t *)(void *)&ip->di.extent[2];
	oh = mp->mi.m_fs[obj->ol[iop->obji].ord].oh;
	user_object_id = obj->ol[iop->obji].obj_id;

	bp = pageio_setup(pp, vn_len, mp->mi.m_fs[iop->ord].svp, flags);
	bp->b_edev = mp->mi.m_fs[iop->ord].dev;
	bp->b_dev = cmpdev(bp->b_edev);
	bp->b_blkno = 0;
	bp->b_un.b_addr = (char *)pg_off;
	bp->b_file = SAM_ITOP(ip);
	bp->b_offset = offset;
	bp->b_private = (void *)iorp;

	sam_osd_setup_private(iorp);
	iorp->obji = iop->obji;
	iorp->offset = iop->blk_off;
	iorp->ip = ip;
	iorp->bp = bp;

	if (bp->b_flags & B_READ) {	/* If reading */
		mutex_enter(&ip->write_mutex);
		if ((iop->blk_off + count) > obj->ol[iop->obji].eoo) {
			count = obj->ol[iop->obji].eoo - iop->blk_off;
			mutex_exit(&ip->write_mutex);
			bp->b_bcount = (size_t)count;
			pagezero(pp, pg_off, PAGESIZE);
			if ((count < 0) || (pg_off > PAGESIZE)) {
				dcmn_err((CE_PANIC,
				    "SAM-QFS: %s: PAGE COUNT off=%x, cnt=%llx, "
				    "eoo=%llx pg_off=%x",
				    mp->mt.fi_name, iop->blk_off, count,
				    obj->ol[iop->obji].eoo, pg_off));
				kmem_free(iorp, sizeof (sam_osd_req_priv_t));
				bp->b_error = EINVAL;
				return (bp);
			}
		} else {
			mutex_exit(&ip->write_mutex);
		}
		TRACE(T_SAM_PGRDOBJ_ST, SAM_ITOV(ip), (sam_tr_t)iorp,
		    offset, count);
		reqp = osd_setup_read_bp(oh, partid, user_object_id,
		    count, iop->blk_off, bp);
	} else {
		TRACE(T_SAM_PGWROBJ_ST, SAM_ITOV(ip), (sam_tr_t)iorp,
		    offset, count);
		reqp = osd_setup_write_bp(oh, partid, user_object_id,
		    count, iop->blk_off, bp);
	}
	if (reqp == NULL) {
		kmem_free(iorp, sizeof (sam_osd_req_priv_t));
		bp->b_error = EINVAL;
		return (bp);
	}
	iorp->reqp = reqp;
	if (!(bp->b_flags & B_READ)) {	/* If writing */
		if (bp->b_flags & B_ASYNC) {
			bp->b_iodone = (int (*) ())sam_page_wrdone;
			TRACE(T_SAM_ASYNC_WR, SAM_ITOV(ip), ip->di.id.ino,
			    (sam_tr_t)bp->b_pages, (sam_tr_t)bp);
			mutex_enter(&ip->write_mutex);
			ip->cnt_writes += count;
			mutex_exit(&ip->write_mutex);
		}
	}

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
	int			obji;


	ip = iorp->ip;
	obj = (sam_di_obj_t *)(void *)&ip->di.extent[2];
	bp = iorp->bp;
	obji = iorp->obji;
	offset = iorp->offset;
	count = bp->b_bcount;
	iorp->result = *resp;

	/*
	 * If no error, update end of object (eoo). Otherwise, set error.
	 */
	if (bp->b_flags & B_READ) {	/* If reading */
		TRACE(T_SAM_PGRDOBJ_COMP, SAM_ITOV(ip), (sam_tr_t)iorp,
		    offset, count);
	} else {			/* If writing */
		TRACE(T_SAM_PGWROBJ_COMP, SAM_ITOV(ip), (sam_tr_t)iorp,
		    offset, count);
		if (resp->err_code == OSD_SUCCESS) {
			mutex_enter(&ip->write_mutex);
			if ((offset + count) > obj->ol[obji].eoo) {
				obj->ol[obji].eoo = (offset + count);
			}
			mutex_exit(&ip->write_mutex);
		}
	}
	if (resp->err_code != OSD_SUCCESS) {
		bp->b_error = sam_osd_errno(resp->err_code, iorp);
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: PG er=%d %d, ip=%p ino=%d sa=%x off=%x"
		    " len=%x r=%x ha=%llx wsz=%llx sz=%llx",
		    ip->mp->mt.fi_name, resp->err_code, bp->b_error, ip,
		    ip->di.id.ino, resp->service_action, offset, count,
		    bp->b_resid, (offset+count), obj->ol[0].eoo, ip->size);
	}
	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS|B_SHADOW);
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
		osd_resid_t *residp = (osd_resid_t *)iorp->result.resid_data;

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

	case OSD_CHECK_CONDITION: {
		char		*scsi_buf;
		uint32_t	scsi_len;
		uint64_t	xfer;

		error = EIO;
		if (iorp->result.err_type == OSD_ERRTYPE_SENSE) {
			scsi_buf = (char *)iorp->result.sense_data;
			if (scsi_buf == NULL) {
				cmn_err(CE_WARN,
				    "SAM-QFS: OSD_ERRTYPE_SENSE err_type=%d "
				    "w/no scsi_buf\n",
				    iorp->result.err_type);
				break;
			}
		} else {
			cmn_err(CE_WARN,
			    "SAM-QFS: OSD_CHECK_CONDITION err_type=%d "
			    "w/no scsi_buf\n",
			    iorp->result.err_type);
			break;
		}
		scsi_len = iorp->result.sense_data_len;
		xfer = sam_decode_scsi_buf(scsi_buf, scsi_len);
		if (xfer > 0) {
			bp->b_resid = bp->b_bcount - xfer;
			error = 0;
			cmn_err(CE_NOTE,
			    "SAM-QFS: OSD SCSI resid=%x, off=%llx, cnt=%llx",
			    bp->b_resid, (long long)bp->b_offset, bp->b_bcount);
		} else {
			int64_t *sb = (int64_t *)(void *)scsi_buf;
			int64_t *sb1 = sb + 1;
			int64_t *sb2 = sb + 2;

			cmn_err(CE_WARN,
			    "SAM-QFS: OSD SCSI BUF = %16.16llx "
			    "%16.16.llx %16.16.llx\n",
			    *sb, *sb1, *sb2);
		}
		}
		break;

	case OSD_FAILURE:
		error = EIO;
		break;

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
 * ----- sam_decode_scsi_buf - Return errno given OSD error.
 */
/* ARGSUSED */
static uint64_t
sam_decode_scsi_buf(
	char *scsi_buf,
	int scsi_len)
{
	uint64_t len;

	if (scsi_buf[0] != 0x72) {
		return (-1);
	}
	if (scsi_buf[1] == 1) {		/* Recovered error */
		/*
		 * Check for reading past end of object
		 */
		if ((scsi_buf[2] == 0x3b) && (scsi_buf[3] == 0x17)) {
			if ((scsi_buf[8] == 1) && (scsi_buf[9] == 0xa)) {
				/*
				 * Bytes 12-19 is no. of bytes transferred
				 */
				len = ((uint64_t)(scsi_buf[12]) << 56) |
				    ((uint64_t)(scsi_buf[13]) << 48) |
				    ((uint64_t)(scsi_buf[14]) << 40) |
				    ((uint64_t)(scsi_buf[15]) << 32) |
				    ((uint64_t)(scsi_buf[16]) << 24) |
				    ((uint64_t)(scsi_buf[17]) << 16) |
				    ((uint64_t)(scsi_buf[18]) << 8) |
				    (scsi_buf[19]);
				return (len);
			}
		}
	}
	return (-1);
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
	uint64_t		partid = SAM_OBJ_PAR_ID;
	uint64_t		numobj;
	uint64_t		pglistsize;
	uint64_t		*objlist;
	uint64_t		*objlist_save;
	osd_req_t		*reqp;
	sam_di_obj_t		*obj;
	int			ord;
	int			n;
	int			num_group;
	uint32_t	obj_pt_type;
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
	obj = (sam_di_obj_t *)(void *)&dp->extent[2];
	ord = dp->unit;
	obj_pt_type = mp->mi.m_fs[ord].part.pt_type;

	/*
	 * Set horizontal stripe width to setfa -h. 0 defaults to all.
	 */
	num_group = (dp->status.b.stripe_width) ? dp->stripe : 1;
	num_group = (num_group == 0) ? mp->mi.m_fs[ord].num_group : num_group;
	n = 0;
	while (n < num_group) {
		reqp = osd_setup_create_object(mp->mi.m_fs[ord].oh, partid,
		    0, numobj);
		if (reqp == NULL) {
			error = EINVAL;
			goto fini;
		}

		objlist = objlist_save;
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
				sam_id_attr_t	x;

				dp->rm.ui.flags |= RM_OBJECT_FILE;

				/*
				 * Skip sam_objlist_page header.
				 */
				objlist++;
				obj->ol[n].obj_id = *objlist;
				obj->ol[n].eoo = 0;
				obj->ol[n].ord = (ushort_t)ord;

				/*
				 * Replace OSD_USER_OBJECT_INFORMATION_USERNAME
				 * w/ QFS vendor specific attribute for ino/gen.
				 */
				x.id.ino = BE_32(dp->id.ino);
				x.id.gen = BE_32(dp->id.gen);
				error = sam_set_user_object_attr(mp, dp, n,
				    OSD_USER_OBJECT_INFORMATION_USERNAME,
				    x.attr);
				if (error) {
					break;
				}
			} else {
				error = sam_osd_errno(iorp->result.err_code,
				    iorp);
				ASSERT(error);
				break;
			}
		} else {
			error = sam_osd_errno(rc, iorp);
			ASSERT(error);
			break;
		}
		sam_osd_remove_private(iorp);
		dp->rm.info.obj.num_group = ++n;
		if (n >= num_group) {
			break;
		}
		if (++ord >= mp->mt.fs_count) {
			ord = 0;
		}
		while (obj_pt_type != mp->mi.m_fs[ord].part.pt_type ||
		    mp->mi.m_fs[ord].part.pt_state != DEV_ON) {
			ord++;
			if (ord == dp->unit) {
				break;
			}
		}
		if (ord == dp->unit) {
			break;
		}
	}
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
static int
sam_get_user_object_attr(
	sam_mount_t *mp,
	struct sam_disk_inode *dp,
	int			n,	/* Target array ordinal in array */
	uint32_t	attr_num,	/* Attribute number */
	int64_t		*attrp)		/* Returned attribute */
{
	sam_osd_req_priv_t	ior_priv;
	sam_osd_req_priv_t	*iorp = &ior_priv;
	osd_req_t		*reqp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	sam_di_obj_t		*obj;
	int			rc;
	int			error = 0;


	obj = (sam_di_obj_t *)(void *)&dp->extent[2];
	reqp = osd_setup_get_attr(mp->mi.m_fs[dp->unit].oh, partid,
	    obj->ol[n].obj_id);
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
static int
sam_set_user_object_attr(
	sam_mount_t *mp,
	struct sam_disk_inode	*dp,	/* Pointer to disk inode */
	int			n,	/* Target array ordinal in array */
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
	reqp = osd_setup_set_attr(mp->mi.m_fs[obj->ol[n].ord].oh, partid,
	    obj->ol[n].obj_id);
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
	offset_t size,		/* Current size of the file */
	offset_t length,	/* New file size */
	cred_t	*credp)		/* Pointer to credentials */
{
	sam_di_obj_t	*obj  = (sam_di_obj_t *)(void *)&ip->di.extent[2];
	int		 offline = ip->di.status.b.offline;
	uchar_t		unit = ip->di.unit;
	sam_di_obj_t	oino;
	int		i;
	int		num_group;
	sam_ioblk_t	ioblk;
	int		error;

	ASSERT(obj->ol[0].obj_id != 0);
	bcopy((char *)obj, (char *)&oino, sizeof (sam_di_obj_t));
	num_group = ip->di.rm.info.obj.num_group;

	/*
	 * Sync the inode to make sure the size is updated on disk.
	 * If error syncing inode, restore previous state.
	 */
	ASSERT(tflag != SAM_REDUCE);
	if ((error = sam_sync_inode(ip, length, tflag))) {
		if (tflag == SAM_PURGE) {
			return (EIO);
		}

		bcopy((char *)&oino, (char *)obj, sizeof (sam_di_obj_t));
		ip->di.rm.info.obj.num_group = num_group;
		ip->di.status.b.offline = offline;
		ip->di.rm.size = size;
		ip->di.unit = unit;
		return (EIO);
	}

	/*
	 * For PURGE (remove), delete all objects on the targets.
	 * Inode was cleared in sam_sync_inode.
	 */
	if ((length == 0) && (tflag == SAM_PURGE)) {
		for (i = 0; i < num_group; i++) {
			if (oino.ol[i].obj_id == 0) {
				continue;
			}
			(void) sam_remove_object_id(ip->mp,
			    oino.ol[i].obj_id, oino.ol[i].ord);
		}
		return (0);
	}

	/*
	 * Truncate up/down, reset end of object for all appropriate stripes.
	 */
	if ((error = sam_map_block(ip, length, 0, SAM_WRITE, &ioblk, credp))) {
		return (error);
	}
	for (i = 0; i < num_group; i++) {
		int			err;
		offset_t	eoo;

		if (i < ioblk.obji) {
			eoo = ioblk.blk_off + ioblk.bsize;
		} else if (i == ioblk.obji) {
			eoo = ioblk.blk_off + ioblk.pboff;
		} else {
			eoo = ioblk.blk_off;
		}
		if (oino.ol[i].eoo != eoo) {
			err = sam_set_user_object_attr(ip->mp, &ip->di, i,
			    OSD_USER_OBJECT_INFORMATION_LOGICAL_LENGTH,
			    BE_64((int64_t)eoo));
			if (err == 0) {
				obj->ol[i].eoo = eoo;
			} else {
				error = err;
			}
		}
	}
	return (error);
}


/*
 * ----- sam_map_osd - Map request to osds.
 *
 *  For the given inode and logical byte address: if iop is set, return
 *  the I/O descriptor. If iop is NULL, just return;
 *
 */

/* ARGSUSED */
int					/* ERRNO, 0 if successful. */
sam_map_osd(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t offset,	/* Logical byte offset in file (longlong_t). */
	offset_t count,		/* Requested byte count. */
	sam_map_t flag,		/* SAM_READ if reading. */
				/* SAM_READ_PUT if read with rounded up */
				/*   filesize, return contiguous blocks. */
				/* SAM_RD_DIRECT_IO if read directio */
				/* SAM_WRITE if writing. */
				/* SAM_WR_DIRECT_IO if write directio */
				/*   (READERS lock held ) */
				/* SAM_FORCEFAULT if write and fault for page */
				/* SAM_WRITE_MMAP if memory mapped write. */
				/* SAM_WRITE_BLOCK if get block */
				/* SAM_WRITE_SPARSE if sparse block */
				/* SAM_ALLOC_BLOCK alloc & don't update size */
				/* SAM_ALLOC_BITMAP alloc & bld bit map(srvr) */
				/* SAM_ALLOC_ZERO allocate and zero. */
	sam_ioblk_t *iop)	/* Ioblk array. */
{
	sam_di_obj_t	*obj  = (sam_di_obj_t *)(void *)&ip->di.extent[2];
	offset_t	n;


	TRACE(T_SAM_OKMAP, SAM_ITOP(ip), (sam_tr_t)offset,
	    (sam_tr_t)count, (sam_tr_t)flag);
	if (iop) {
		bzero((char *)iop, sizeof (sam_ioblk_t));
		iop->imap.flags = (M_OBJECT | M_VALID);
		iop->contig = SAM_OSD_MAX_WR_CONTIG;
		if (flag <= SAM_RD_DIRECT_IO) {		/* If reading */
			offset_t size;

			size = (ip->size + PAGESIZE - 1) & PAGEMASK;
			iop->contig = size - offset;
			iop->contig = size - offset;
			if (iop->contig > SAM_OSD_MAX_RD_CONTIG) {
				iop->contig = SAM_OSD_MAX_RD_CONTIG;
			}
		}
		iop->imap.ord0 = ip->di.unit;
		if (ip->di.rm.info.obj.num_group > 1) {
			int stripe_shift;

			iop->num_group = ip->di.rm.info.obj.num_group;
			stripe_shift = ip->di.rm.info.obj.stripe_shift;
			if (stripe_shift == 0) {
				stripe_shift = SAM_OSD_STRIPE_SHIFT;
			}
			iop->bsize = (1 << stripe_shift);
			iop->blk_off = (offset /
			    (iop->num_group << stripe_shift)) << stripe_shift;
			iop->pboff = offset & (iop->bsize - 1);
			iop->contig += iop->pboff;
			iop->imap.blk_off0 = iop->blk_off;

			n = (offset >> stripe_shift);
			if (n >= iop->num_group) {
				n = n % iop->num_group;
			}
			iop->obji = (uchar_t)n;
			iop->ord = obj->ol[n].ord;
		} else {
			iop->blk_off = offset;
			iop->ord = ip->di.unit;
			iop->obji = 0;
			iop->num_group = 1;
			iop->bsize = iop->contig;
			iop->pboff = 0;
		}
		iop->count = count;
		iop->blkno = 0x7fffffff;
		iop->dev_bsize = ip->mp->mi.m_fs[iop->ord].dev_bsize;
	}
	if (flag <= SAM_RD_DIRECT_IO) {		/* Check for reading past eoo */
		if (iop->blk_off >= obj->ol[iop->obji].eoo) {
			iop->imap.flags |= M_OBJ_EOF;
		}
		return (0);
	}

	/*
	 * If writing, increment filesize if no error.
	 */
	ASSERT(RW_WRITE_HELD(&ip->inode_rwl) || (flag == SAM_WR_DIRECT_IO));
	if (((offset + count) < ip->size) || (flag >= SAM_WRITE_SPARSE)) {
		return (0);
	}
	if (flag == SAM_WR_DIRECT_IO) {
		mutex_enter(&ip->fl_mutex);
	}
	ip->size = offset + count;
	if (flag != SAM_WRITE && flag != SAM_WR_DIRECT_IO) {
		if (!ip->flags.b.staging && !S_ISSEGI(&ip->di)) {
			if (ip->size > ip->di.rm.size) {
				ip->di.rm.size = ip->size;
			}
		}
	}
	if (flag == SAM_WR_DIRECT_IO) {
		mutex_exit(&ip->fl_mutex);
	}
	return (0);
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
	sam_ioblk_t *iop,
	offset_t contig)
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
 * ----- sam_truncate_object_file - Truncate an osd file.
 */
/* ARGSUSED */
int
sam_truncate_object_file(
	sam_node_t *ip,
	sam_truncate_t tflag,	/* Truncate file or Release file */
	offset_t size,		/* Current size of the file */
	offset_t length,	/* New file size */
	cred_t	*credp)		/* Pointer to credentials */
{
	return (EINVAL);
}


/* ARGSUSED */
int					/* ERRNO, 0 if successful. */
sam_map_osd(
	sam_node_t *ip,		/* Pointer to the inode. */
	offset_t offset,	/* Logical byte offset in file (longlong_t). */
	offset_t count,		/* Requested byte count. */
	sam_map_t flag,
	sam_ioblk_t *iop)	/* Ioblk array. */
{
	return (EINVAL);
}

#endif /* SAM_OSD_SUPPORT */
