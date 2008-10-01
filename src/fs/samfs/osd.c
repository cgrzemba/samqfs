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

#pragma ident "$Revision: 1.38 $"

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
#include <sys/modctl.h>
#include "scsi_osd.h"
#include "osd.h"


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
#include "object.h"


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
static int sam_get_user_object_attr(sam_mount_t *mp, sam_osd_handle_t oh,
	uint64_t object_id, int64_t *attrp);
static int sam_set_user_object_attr(struct sam_mount *mp,
	struct sam_disk_inode *dp, int ord, uint64_t object_id,
	uint32_t attr_num, int64_t attribute);
static void sam_object_req_done(osd_req_t *reqp, void *ct_priv,
	osd_result_t *resp);
static void sam_dk_object_done(osd_req_t *reqp, void *ct_priv,
	osd_result_t *resp);
static void sam_pg_object_done(osd_req_t *reqp, void *ct_priv,
	osd_result_t *resp);
static int sam_osd_map_errno(int sosd_errcodes);
static int sam_osd_errno(osd_result_t *resp, sam_osd_req_priv_t *iorp);
static uint64_t sam_decode_scsi_buf(char *scsi_buf, int scsi_len);
static int sam_get_object_id_ext(sam_mount_t *mp, struct sam_disk_inode	*dp,
	int n, sam_id_t id, buf_t **bpp, struct sam_inode_ext **eipp);
static int sam_osd_inode_extent(sam_mount_t *mp, struct sam_disk_inode *dp,
	int *num_groupp);

static const int sosd_errcodes2errno[] = {
	0,		/* OSD_SUCCESS */
	EIO,		/* OSD_FAILURE */
	0,		/* OSD_RECOVERED_ERROR */
	EIO,		/* OSD_CHECK_CONDITION */
	EINVAL,		/* OSD_INVALID */
	E2BIG,		/* OSD_TOOBIG */
	EINVAL,		/* OSD_BADREQUEST */
	EIO,		/* OSD_LUN_ERROR */
	EIO,		/* OSD_LUN_FAILURE */
	EBUSY,		/* OSD_BUSY */
	EACCES,		/* OSD_RESERVATION_CONFLICT */
	EIO,		/* OSD_RESIDUAL */
	EAGAIN		/* OSD_NORESOURCES */
};


struct sam_sosd_vec sam_sosd_vec;

/*
 * sam_sosd_bind - If the sosd module is loaded, retrieve the sosd external
 * functions.
 */
void
sam_sosd_bind(void)
{

	uint32_t api_version;
	int err = 0;

	bzero((char *)&sam_sosd_vec, sizeof (struct sam_sosd_vec));

	/*
	 * Verify if the scsi sosd module is loaded.
	 */
	sam_sosd_vec.scsi_osd_hdl =
	    ddi_modopen(SCSI_SOSD_MOD_NAME, KRTLD_MODE_FIRST, &err);
	if (sam_sosd_vec.scsi_osd_hdl == NULL) {
		goto failed;
	}

	/*
	 * Get the api version and make sure that we match.
	 */
	sam_sosd_vec.get_api_version =
	    (uint32_t (*)())ddi_modsym(sam_sosd_vec.scsi_osd_hdl,
	    OSD_GET_API_VERSION, &err);
	if (!sam_sosd_vec.get_api_version) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_GET_API_VERSION, err);
		goto failed;
	}
	api_version = (sam_sosd_vec.get_api_version)();
	if (OSD_API_REV != api_version) {
		cmn_err(CE_WARN, "SAM-QFS: Expected API REV %d but found %d\n",
		    OSD_API_REV, api_version);
		goto failed;
	}

	/*
	 * The sosd module is available.  Get the vectors of interest.
	 */
	sam_sosd_vec.setup_create_object =
	    (osd_req_t *(*)(osd_dev_t, uint64_t, uint64_t, uint16_t))
	    ddi_modsym(sam_sosd_vec.scsi_osd_hdl, OSD_SETUP_CREATE_OBJECT,
	    &err);
	if (!sam_sosd_vec.setup_create_object) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_SETUP_CREATE_OBJECT, err);
		goto failed;
	}

	sam_sosd_vec.setup_remove_object = (osd_req_t *(*)
	    (osd_dev_t, uint64_t, uint64_t))
	    ddi_modsym(sam_sosd_vec.scsi_osd_hdl, OSD_SETUP_REMOVE_OBJECT,
	    &err);
	if (!sam_sosd_vec.setup_remove_object) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_SETUP_REMOVE_OBJECT, err);
		goto failed;
	}

	sam_sosd_vec.setup_read = (osd_req_t *(*)(osd_dev_t, uint64_t,
	    uint64_t, uint64_t, uint64_t, uint8_t, iovec_t *))
	    ddi_modsym(sam_sosd_vec.scsi_osd_hdl, OSD_SETUP_READ, &err);
	if (!sam_sosd_vec.setup_read) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_SETUP_READ, err);
		goto failed;
	}

	sam_sosd_vec.setup_write = (osd_req_t *(*)(osd_dev_t, uint64_t,
	    uint64_t, uint64_t, uint64_t, uint8_t, iovec_t *))
	    ddi_modsym(sam_sosd_vec.scsi_osd_hdl, OSD_SETUP_WRITE, &err);
	if (!sam_sosd_vec.setup_write) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_SETUP_WRITE, err);
		goto failed;
	}

	sam_sosd_vec.setup_write_bp = (osd_req_t *(*)(osd_dev_t, uint64_t,
	    uint64_t, uint64_t, uint64_t, struct buf *))
	    ddi_modsym(sam_sosd_vec.scsi_osd_hdl, OSD_SETUP_WRITE_BP, &err);
	if (!sam_sosd_vec.setup_write_bp) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_SETUP_WRITE_BP, err);
		goto failed;
	}

	sam_sosd_vec.setup_read_bp = (osd_req_t *(*)(osd_dev_t,
	    uint64_t, uint64_t, uint64_t, uint64_t, struct buf *))
	    ddi_modsym(sam_sosd_vec.scsi_osd_hdl, OSD_SETUP_READ_BP, &err);
	if (!sam_sosd_vec.setup_read_bp) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_SETUP_READ_BP, err);
		goto failed;
	}

	sam_sosd_vec.submit_req = (int (*)(osd_req_t *, void (* done)
	    (osd_req_t  *, void *, osd_result_t *), void *ct_priv))
	    ddi_modsym(sam_sosd_vec.scsi_osd_hdl, OSD_SUBMIT_REQ, &err);
	if (!sam_sosd_vec.submit_req) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_SUBMIT_REQ, err);
		goto failed;
	}

	sam_sosd_vec.free_req = (void (*)(osd_req_t *))
	    ddi_modsym(sam_sosd_vec.scsi_osd_hdl, OSD_FREE_REQ, &err);
	if (!sam_sosd_vec.free_req) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_FREE_REQ, err);
		goto failed;
	}

	sam_sosd_vec.setup_set_attr = (osd_req_t *(*)
	    (osd_dev_t, uint64_t, uint64_t))
	    ddi_modsym(sam_sosd_vec.scsi_osd_hdl, OSD_SETUP_SET_ATTR, &err);
	if (!sam_sosd_vec.setup_set_attr) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_SETUP_SET_ATTR, err);
		goto failed;
	}

	sam_sosd_vec.setup_get_attr = (osd_req_t *(*)
	    (osd_dev_t, uint64_t, uint64_t))
	    ddi_modsym(sam_sosd_vec.scsi_osd_hdl, OSD_SETUP_GET_ATTR, &err);
	if (!sam_sosd_vec.setup_get_attr) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_SETUP_GET_ATTR, err);
		goto failed;
	}

	sam_sosd_vec.add_set_page_1attr_cdb = (void (*)
	    (osd_req_t *, uint32_t, uint32_t, uint16_t, char *))
	    ddi_modsym(sam_sosd_vec.scsi_osd_hdl, OSD_ADD_SET_PAGE_1ATTR_CDB,
	    &err);
	if (!sam_sosd_vec.add_set_page_1attr_cdb) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_ADD_SET_PAGE_1ATTR_CDB, err);
		goto failed;
	}

	sam_sosd_vec.add_set_page_1attr_to_req = (void (*)(osd_req_t *,
	    uint32_t, uint32_t, uint32_t, void *))
	    ddi_modsym(sam_sosd_vec.scsi_osd_hdl,
	    OSD_ADD_SET_PAGE_1ATTR_TO_REQ, &err);
	if (!sam_sosd_vec.add_set_page_1attr_to_req) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_ADD_SET_PAGE_1ATTR_TO_REQ, err);
		goto failed;
	}

	sam_sosd_vec.add_get_page_attr_to_req = (void (*)(osd_req_t *,
	    uint32_t, uint32_t, void *))
	    ddi_modsym(sam_sosd_vec.scsi_osd_hdl, OSD_ADD_SET_PAGE_ATTR_TO_REQ,
	    &err);
	if (!sam_sosd_vec.add_get_page_attr_to_req) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_ADD_SET_PAGE_ATTR_TO_REQ, err);
		goto failed;
	}

	sam_sosd_vec.add_flags_to_req = (void (*)(osd_req_t *, uint32_t))
	    ddi_modsym(sam_sosd_vec.scsi_osd_hdl, OSD_ADD_FLAGS_TO_REQ, &err);
	if (!sam_sosd_vec.add_flags_to_req) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_ADD_FLAGS_TO_REQ, err);
		goto failed;
	}

	/*
	 * Verify if the sosd module is loaded.
	 */
	sam_sosd_vec.sosd_hdl =
	    ddi_modopen(SOSD_MOD_NAME, KRTLD_MODE_FIRST, &err);
	if (sam_sosd_vec.sosd_hdl == NULL) {
		cmn_err(CE_WARN, "SAM-QFS: OSD Target Driver not avaliable\n");
		return;
	}

	sam_sosd_vec.open_by_name = (int (*)(char *, int, cred_t *,
	    osd_dev_t *))
	    ddi_modsym(sam_sosd_vec.sosd_hdl, OSD_OPEN_BY_NAME, &err);
	if (!sam_sosd_vec.open_by_name) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_OPEN_BY_NAME, err);
		goto failed;
	}

	sam_sosd_vec.close = (int (*)(osd_dev_t, int, cred_t *))
	    ddi_modsym(sam_sosd_vec.sosd_hdl, OSD_CLOSE, &err);
	if (!sam_sosd_vec.close) {
		cmn_err(CE_WARN, "SAM-QFS: Unable to get %s %d\n",
		    OSD_CLOSE, err);
		goto failed;
	}

	return;

failed:
	if (sam_sosd_vec.scsi_osd_hdl) {
		ddi_modclose(sam_sosd_vec.scsi_osd_hdl);
	}

	if (sam_sosd_vec.sosd_hdl) {
		ddi_modclose(sam_sosd_vec.sosd_hdl);
	}

	bzero((char *)&sam_sosd_vec, sizeof (struct sam_sosd_vec));

}

/*
 * sam_sosd_unbind - Release the hold on the sosd module.
 */
void
sam_sosd_unbind(void)
{
	if (sam_sosd_vec.scsi_osd_hdl) {
		ddi_modclose(sam_sosd_vec.scsi_osd_hdl);
		bzero((char *)&sam_sosd_vec, sizeof (struct sam_sosd_vec));
	}

}

/*
 * ----- sam_open_osd_device - Open an osd device. Return object handle.
 */
int
sam_open_osd_device(
	struct samdent *dp,	/* Pointer to device entry */
	int filemode,		/* Filemode for open */
	cred_t *credp)		/* Credentials pointer. */
{
	sam_osd_handle_t oh;
	vnode_t *svp;
	dev_t dev;
	int rc, error;

	/*
	 * Make sure that the sosd module has been initialized with SAMQFS.
	 */
	if (!sam_sosd_vec.sosd_hdl) {
		return (EUNATCH);
	}

	error = lookupname(dp->part.pt_name, UIO_SYSSPACE, FOLLOW, NULL, &svp);
	if (error) {
		return (ENODEV);
	}
	if (svp->v_type != VCHR) {
		VN_RELE(svp);
		return (EINVAL);	/* Must be type character */
	}
	dev = svp->v_rdev;
	VN_RELE(svp);
	rc = (sam_sosd_vec.open_by_name)(dp->part.pt_name, filemode, credp,
	    (void *)&oh);
	if (rc != OSD_SUCCESS) {
		error = sam_osd_map_errno(rc);
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
	(void) (sam_sosd_vec.close)((void *)oh, filemode, credp);
}


/*
 * ----- sam_issue_object_io - Issue object I/O.
 */
int
sam_issue_object_io(
	sam_mount_t *mp,	/* Pointer to mount table */
	sam_osd_handle_t oh,	/* Object device handle */
	uint32_t command,	/* Object command: FWRITE, FREAD */
	uint64_t object_id,	/* 64-bit user object identifier */
	enum uio_seg seg,	/* User or system space */
	char *data,		/* Pointer to the buffer */
	offset_t offset,	/* Logical offset in object */
	offset_t length)	/* Byte length of request */
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
	} else {
		bufp = data;
	}
	iov.iov_base = (void *)bufp;
	iov.iov_len = (long)length;
	if (command == FWRITE) {
		reqp = (sam_sosd_vec.setup_write)((void *)oh, partid, object_id,
		    length, offset, 1, &iov);
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
		reqp = (sam_sosd_vec.setup_read)((void *)oh, partid, object_id,
		    length, offset, 1, &iov);
		if (reqp == NULL) {
			error = EINVAL;
			goto fini;
		}
	}
	sam_osd_setup_private(iorp, mp);
	rc = (sam_sosd_vec.submit_req)(reqp, sam_object_req_done, iorp);
	if (rc == OSD_SUCCESS) {
		sam_osd_obj_req_wait(iorp);	/* Wait for completion */
		if ((error = iorp->error) == 0) {
			if (command == FREAD && seg == UIO_USERSPACE) {
				if (copyout(bufp, data, length)) {
					error = EFAULT;
				}
			}
		}
	} else {
		error = sam_osd_map_errno(rc);
	}
	sam_osd_remove_private(iorp);
fini:
	if (seg == UIO_USERSPACE) {
		kmem_free(bufp, length);
	}
	if (reqp) {
		(sam_sosd_vec.free_req)(reqp);
	}
	return (error);
}


/*
 * ----- sam_get_osd_fs_attr - Process get file system attribute call.
 * Set capacity and space in superblock equipment entry.
 */
int
sam_get_osd_fs_attr(
	sam_mount_t *mp,		/* Pointer to the mount table */
	sam_osd_handle_t oh,		/* Object device handle */
	struct sam_fs_part *fsp)	/* Pointer to device partition table */
{
	sam_osd_req_priv_t	ior_priv;
	sam_osd_req_priv_t	*iorp = &ior_priv;
	sam_fsinfo_page_t	fs_attr;
	osd_req_t		*reqp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	int			rc;
	int			error = 0;

	reqp = (sam_sosd_vec.setup_get_attr)((void *)oh, partid,
	    SAM_OBJ_SBLK_ID);
	if (reqp == NULL) {
		return (EINVAL);
	}
	(sam_sosd_vec.add_get_page_attr_to_req)(reqp,
	    OSD_SAMQFS_VENDOR_FS_ATTR_PAGE_NO,
	    sizeof (sam_fsinfo_page_t), (void *) &fs_attr);
	sam_osd_setup_private(iorp, mp);
	rc = (sam_sosd_vec.submit_req)(reqp, sam_object_req_done, iorp);
	if (rc == OSD_SUCCESS) {
		sam_osd_obj_req_wait(iorp);
		if ((error = iorp->error) == 0) {
			sam_fsinfo_page_t *fap = &fs_attr;

			fsp->pt_capacity = SFP_CAPACITY(fap);
			fsp->pt_space = SFP_SPACE(fap);
			fsp->pt_size = fsp->pt_capacity << SAM2SUN_BSHIFT;
		} else {
			dcmn_err((CE_WARN, "SAM-QFS: %s: OSD GET fs attr"
			    " rc=%d, errno=%d", mp->mt.fi_name, iorp->err_code,
			    error));
		}
	} else {
		error = sam_osd_map_errno(rc);
	}
	sam_osd_remove_private(iorp);

fini:
	if (reqp) {
		(sam_sosd_vec.free_req)(reqp);
	}
	return (error);
}


/*
 * ----- sam_object_req_done - Process object osd request completion.
 *
 * This is the interrupt routine. Called when the object request completes.
 * Note, all object requests are async.
 */
/* ARGSUSED0 */
static void
sam_object_req_done(
	osd_req_t *reqp,
	void *ct_priv,
	osd_result_t *resp)
{
	sam_osd_req_priv_t *iorp = (sam_osd_req_priv_t *)ct_priv;

	ASSERT(iorp->name == SAM_OBJ_PRIVATE_NAME);
	if (resp->err_code) {
		iorp->error = sam_osd_errno(resp, iorp);
	}
	sam_osd_obj_req_done(iorp);	/* Wakeup caller */
}


/*
 * ----- sam_issue_direct_object_io - Issue object direct async I/O.
 */
int
sam_issue_direct_object_io(
	sam_node_t *ip,		/* Pointer to inode entry */
	enum uio_rw rw,		/* UIO_WRITE or UIO_READ */
	struct buf *bp,		/* Pointer to buffer */
	sam_ioblk_t *iop,	/* Pointer to ioblk */
	offset_t contig)	/* Length of transfer */
{
	int			obji = iop->obji;
	offset_t		offset = iop->blk_off + iop->pboff;
	sam_osd_handle_t	oh;
	sam_osd_req_priv_t	*iorp;
	osd_req_t		*reqp;
	uint64_t		object_id;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	sam_obj_layout_t	*olp;
	int			rc;
	int			error = 0;


	iorp = kmem_cache_alloc(samgt.object_cache, KM_SLEEP);
	olp = ip->olp;
	ASSERT(olp);
	oh = ip->mp->mi.m_fs[olp->ol[obji].ord].oh;
	object_id = olp->ol[obji].obj_id;
	if (rw == UIO_WRITE) {
		reqp = (sam_sosd_vec.setup_write_bp)((void *)oh, partid,
		    object_id, contig, offset, bp);
	} else {
		reqp = (sam_sosd_vec.setup_read_bp)((void *)oh, partid,
		    object_id, contig, offset, bp);
	}
	if (reqp == NULL) {
		kmem_cache_free(samgt.object_cache, iorp);
		return (EINVAL);
	}
	if (ip->mp->mt.fi_obj_sync_data) {
		(sam_sosd_vec.add_flags_to_req)(reqp, OSD_O_DPO);
	}
	iorp->name = SAM_OBJ_PRIVATE_NAME;
	iorp->mp = ip->mp;
	iorp->obji = obji;	/* Object layout index */
	iorp->offset = offset;	/* Logical offset in I/O request */
	iorp->ip = ip;
	iorp->bp = bp;
	rc = (sam_sosd_vec.submit_req)(reqp, sam_dk_object_done, iorp);
	if (rc != OSD_SUCCESS) {
		osd_result_t result;

		bzero(&result, sizeof (result));
		result.err_code = (uint8_t)rc;
		dcmn_err((CE_WARN,
		    "SAM-QFS: %s: DK SUBMIT ERROR rc=%d, ip=%p, ino=%d.%d, "
		    "off=%llx len=%lx obji=%d eoo=%llx sz=%llx",
		    ip->mp->mt.fi_name, rc, (void *)ip,
		    ip->di.id.ino, ip->di.id.gen, offset,
		    bp->b_bcount, obji, olp->ol[obji].eoo, ip->size));
		sam_dk_object_done(reqp, iorp, &result);
		error = sam_osd_map_errno(rc);
	}
	return (error);
}


/*
 * ----- sam_dk_object_done - Process object direct async I/O completion.
 * This is the interrupt routine. Called when each direct async I/O completes.
 */
static void
sam_dk_object_done(
	osd_req_t *reqp,	/* Pointer to request struct */
	void *ct_priv,		/* Pointer to private area */
	osd_result_t *resp)	/* Pointer to result struct */
{
	sam_osd_req_priv_t	*iorp = (sam_osd_req_priv_t *)ct_priv;
	sam_node_t		*ip;
	buf_descriptor_t	*bdp;
	sam_buf_t  		*sbp;
	buf_t			*bp;
	offset_t 		count;
	offset_t		offset;
	sam_obj_layout_t	*olp;
	boolean_t		async;
	int			obji;


	ASSERT(iorp->name == SAM_OBJ_PRIVATE_NAME);
	bp = (buf_t *)iorp->bp;
	sbp = (sam_buf_t *)bp;
	bdp = (buf_descriptor_t *)(void *)bp->b_vp;
	ip = bdp->ip;
	olp = ip->olp;
	ASSERT(olp);
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
	if (bp->b_flags & B_READ) {	/* If reading */
		TRACE(T_SAM_DIORDOBJ_COMP, SAM_ITOV(ip), (sam_tr_t)iorp,
		    offset, (((offset_t)obji << 32)|count));
	} else {			/* If writing */
		TRACE(T_SAM_DIOWROBJ_COMP, SAM_ITOV(ip), (sam_tr_t)iorp,
		    offset, (((offset_t)obji << 32)|count));
		if (resp->err_code == OSD_SUCCESS) {
			mutex_enter(&ip->write_mutex);
			if ((offset + count) > olp->ol[obji].eoo) {
				olp->ol[obji].eoo = (offset + count);
			}
			mutex_exit(&ip->write_mutex);
		}
	}
	if (resp->err_code != OSD_SUCCESS) {
		bdp->error = sam_osd_errno(resp, iorp);
		dcmn_err((CE_WARN,
		    "SAM-QFS: %s: DK er=%d, ip=%p, ino=%d.%d, SA=%x, off=%llx"
		    " len=%lx r=%lx ha=%llx obji=%d eoo=%llx sz=%llx",
		    ip->mp->mt.fi_name, resp->err_code, (void *)ip,
		    ip->di.id.ino, ip->di.id.gen, resp->service_action, offset,
		    bp->b_bcount, bp->b_resid, (offset + bp->b_bcount), obji,
		    olp->ol[obji].eoo, ip->size));
	}

	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS|B_SHADOW);
	sam_free_buf_header((sam_uintptr_t *)bp);
	(sam_sosd_vec.free_req)(reqp);
	kmem_cache_free(samgt.object_cache, iorp);
	if (async) {
		sam_dk_aio_direct_done(ip, bdp, count);
	}
}


/*
 * ----- sam_pageio_object - Start object I/O on a page.
 */
buf_t *
sam_pageio_object(
	sam_node_t *ip,		/* Pointer to inode entry */
	sam_ioblk_t *iop,	/* Pointer to I/O entry */
	page_t *pp,		/* Pointer to page list */
	offset_t off,		/* Logical file offset */
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
	offset_t		offset;
	uint64_t		object_id;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	sam_obj_layout_t	*olp;
	int			rc;

	iorp = kmem_cache_alloc(samgt.object_cache, KM_SLEEP);
	mp = ip->mp;
	count = vn_len;
	olp = ip->olp;
	ASSERT(olp);
	oh = mp->mi.m_fs[olp->ol[iop->obji].ord].oh;
	object_id = olp->ol[iop->obji].obj_id;

	sam_osd_setup_private(iorp, mp);
	iorp->obji = iop->obji;
	offset = iop->blk_off + iop->pboff;
	iorp->offset = offset;
	iorp->ip = ip;

	if (flags & B_READ) {	/* If reading */
		mutex_enter(&ip->write_mutex);
		if (offset >= olp->ol[iop->obji].eoo) {
			mutex_exit(&ip->write_mutex);
			pagezero(pp, pg_off, PAGESIZE);
			iop->imap.flags |= M_OBJ_EOF;
			TRACE(T_SAM_OBJ_PAGEZERO, SAM_ITOP(ip), off, offset,
			    olp->ol[iop->obji].eoo);
			sam_osd_remove_private(iorp);
			kmem_cache_free(samgt.object_cache, iorp);
			return (NULL);
		} else {
			if ((offset + count) >= olp->ol[iop->obji].eoo) {
				count = olp->ol[iop->obji].eoo - offset;
				iop->imap.flags |= M_OBJ_EOF;
			}
			mutex_exit(&ip->write_mutex);
		}
	}

	bp = pageio_setup(pp, vn_len, mp->mi.m_fs[iop->ord].svp, flags);
	bp->b_edev = mp->mi.m_fs[iop->ord].dev;
	bp->b_dev = cmpdev(bp->b_edev);
	bp->b_blkno = 0;
	bp->b_un.b_addr = (char *)pg_off;
	bp->b_file = SAM_ITOP(ip);
	bp->b_offset = off;
	bp->b_bcount = (size_t)count;
	bp->b_private = (void *)iorp;

	if (flags & B_READ) {
		TRACE(T_SAM_PGRDOBJ_ST, SAM_ITOV(ip), off, offset,
			(((offset_t)iop->obji << 32)|count));
		reqp = (sam_sosd_vec.setup_read_bp)((void *)oh, partid,
		    object_id, count, offset, bp);
	} else {
		TRACE(T_SAM_PGWROBJ_ST, SAM_ITOV(ip), off, offset,
		    (((offset_t)iop->obji << 32)|count));
		reqp = (sam_sosd_vec.setup_write_bp)((void *)oh, partid,
		    object_id, count, offset, bp);
	}
	ASSERT(count > 0);
	if (reqp == NULL) {
		pageio_done(bp);
		sam_osd_remove_private(iorp);
		kmem_cache_free(samgt.object_cache, iorp);
		return (NULL);
	}
	iorp->reqp = reqp;
	iorp->bp = bp;
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

	rc = (sam_sosd_vec.submit_req)(reqp, sam_pg_object_done, iorp);
	if (rc != OSD_SUCCESS) {
		osd_result_t result;

		dcmn_err((CE_WARN,
		    "SAM-QFS: %s: PG SUBMIT ERROR rc=%d, ip=%p, ino=%d.%d, "
		    "off=%llx len=%lx obji=%d eoo=%llx sz=%llx",
		    ip->mp->mt.fi_name, rc, (void *)ip,
		    ip->di.id.ino, ip->di.id.gen, offset,
		    bp->b_bcount, iop->obji, olp->ol[iop->obji].eoo, ip->size));
		bzero(&result, sizeof (result));
		result.err_code = (uint8_t)rc;
		sam_pg_object_done(reqp, iorp, &result);
	}
	return (bp);
}


/*
 * ----- sam_pg_object_done - Process object page async I/O completion.
 * This is the interrupt routine. Called when each paged async I/O completes.
 */
static void
sam_pg_object_done(
	osd_req_t *reqp,	/* Pointer to request struct */
	void *ct_priv,		/* Pointer to private area */
	osd_result_t *resp)	/* Pointer to result struct */
{
	sam_osd_req_priv_t	*iorp = (sam_osd_req_priv_t *)ct_priv;
	sam_node_t 		*ip;
	buf_t			*bp;
	offset_t		count;
	offset_t		offset;
	sam_obj_layout_t	*olp;
	int			obji;


	ASSERT(iorp->name == SAM_OBJ_PRIVATE_NAME);
	ip = iorp->ip;
	olp = ip->olp;
	ASSERT(olp);
	bp = iorp->bp;
	obji = iorp->obji;
	offset = iorp->offset;
	count = bp->b_bcount;

	/*
	 * If no error, update end of object for write. Otherwise, set error.
	 */
	if (bp->b_flags & B_READ) {	/* If reading */
		TRACE(T_SAM_PGRDOBJ_COMP, SAM_ITOV(ip), (sam_tr_t)iorp,
		    offset, (sam_tr_t)(((offset_t)obji << 32)|count));
	} else {			/* If writing */
		TRACE(T_SAM_PGWROBJ_COMP, SAM_ITOV(ip), (sam_tr_t)iorp,
		    offset, (sam_tr_t)(((offset_t)obji << 32)|count));
		if (resp->err_code == OSD_SUCCESS) {
			mutex_enter(&ip->write_mutex);
			if ((offset + count) > olp->ol[obji].eoo) {
				olp->ol[obji].eoo = (offset + count);
			}
			mutex_exit(&ip->write_mutex);
		}
	}
	if (resp->err_code != OSD_SUCCESS) {
		bp->b_error = sam_osd_errno(resp, iorp);
		dcmn_err((CE_WARN,
		    "SAM-QFS: %s: PG1 er=%d %d, ip=%p ino=%d.%d SA=%x off=%llx"
		    " len=%llx r=%lx ha=%llx obji=%d eoo=%llx sz=%llx",
		    ip->mp->mt.fi_name, resp->err_code, bp->b_error, (void *)ip,
		    ip->di.id.ino, ip->di.id.gen, resp->service_action,
		    offset, count, bp->b_resid, (offset+count), obji,
		    olp->ol[obji].eoo, ip->size));
	}
	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS|B_SHADOW);
	bp->b_iodone = NULL;
	if (bp->b_flags & B_ASYNC) {
		if (!(bp->b_flags & B_READ)) {	/* If writing */
			sam_page_wrdone(bp);
		} else {
			biodone(bp);
		}
		(sam_sosd_vec.free_req)(reqp);
		sam_osd_remove_private(iorp);
		kmem_cache_free(samgt.object_cache, iorp);
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
	int			obji;
	int			 error = 0;

	iorp = (sam_osd_req_priv_t *)bp->b_private;
	ASSERT(iorp->name == SAM_OBJ_PRIVATE_NAME);
	obji = iorp->obji;
	sam_osd_obj_req_wait(iorp);	/* Wait for completion */
	if (bp->b_error) {
		error = bp->b_error;
		dcmn_err((CE_WARN,
		    "SAM-QFS: %s: PG2 %s er=%d, ip=%p ino=%d.%d off=%llx"
		    " cnt=%lx r=%lx obji=%d eoo=%llx sz=%llx",
		    ip->mp->mt.fi_name, str, error, (void *)ip,
		    ip->di.id.ino, ip->di.id.gen, iorp->offset, bp->b_bcount,
		    bp->b_resid, obji, ip->olp->ol[obji].eoo, ip->size));
	}
	pageio_done(bp);
	(sam_sosd_vec.free_req)(iorp->reqp);
	sam_osd_remove_private(iorp);
	kmem_cache_free(samgt.object_cache, iorp);
	return (error);
}


/*
 * ----- sam_osd_map_errno - Map sosd device driver error codes to generic
 * errno in sys/errno.h
 * This routine should only be called to map sychronous error codes(rc)
 * returned when calling sosd routines e.g.
 * rc = (sam_sosd_vec.submit_req)(reqp, sam_object_req_done, iorp);
 */
static int
sam_osd_map_errno(int sosd_errcode)
{
	if ((sosd_errcode >= OSD_SUCCESS) &&
	    (sosd_errcode <= OSD_NORESOURCES)) {
		return (sosd_errcodes2errno[sosd_errcode]);
	} else {
		dcmn_err((CE_WARN, "SAM-QFS: Unknown SOSD ERROR sosd_errcode "
		    "=%d", sosd_errcode));
		return (EIO);
	}
}


/*
 * ----- sam_osd_errno - Return errno given OSD error.
 */
int
sam_osd_errno(
	osd_result_t *resp,		/* Pointer to result struct */
	sam_osd_req_priv_t *iorp)	/* Pointer to private struct */
{
	sam_mount_t *mp = iorp->mp;
	buf_t *bp;
	int rc;
	int error;

	rc = resp->err_code;
	error = sam_osd_map_errno(rc);
	iorp->err_code = (uint16_t)rc;
	iorp->service_action = resp->service_action;
	bp = iorp->bp;

	switch (rc) {
	case OSD_RESIDUAL: {
		osd_resid_t *residp = (osd_resid_t *)&resp->resid_data;

		ASSERT(residp != NULL);
		if (residp == NULL) {
			error = EINVAL;
		} else if (bp) {
			bp->b_resid = (bp->b_flags & B_READ) ?
			    residp->ot_in_command_resid :
			    residp->ot_out_command_resid;
			dcmn_err((CE_WARN, "SAM-QFS: %s: OSD RESID SA=%x "
			    "rc=%d, errno=%d, resid=%lx, off=%llx, cnt=%llx",
			    mp ? mp->mt.fi_name : " ", resp->service_action,
			    resp->err_code, error, bp->b_resid,
			    (long long)bp->b_offset, (long long)bp->b_bcount));
		}
		}
		break;

	case OSD_CHECK_CONDITION: {
		char		*scsi_buf;
		uint32_t	scsi_len;
		uint64_t	xfer;

		error = EIO;
		scsi_buf = (char *)resp->sense_data;
		if (scsi_buf == NULL) {
			cmn_err(CE_WARN,
			    "SAM-QFS: OSD_CHECK_CONDITION "
			    "with no scsi_buf\n");
			break;
		}
		scsi_len = resp->sense_data_len;
		xfer = sam_decode_scsi_buf(scsi_buf, scsi_len);
		if (xfer > 0) {
			error = 0;
			if (bp) {
				bp->b_resid = bp->b_bcount - xfer;
				cmn_err(CE_NOTE,
				    "SAM-QFS: OSD SCSI resid=%lx, off=%llx, "
				    "cnt=%lx", bp->b_resid,
				    (long long)bp->b_offset, bp->b_bcount);
			}
		} else {
			int64_t *sb = (int64_t *)(void *)scsi_buf;
			int64_t *sb1 = sb + 1;
			int64_t *sb2 = sb + 2;

			cmn_err(CE_WARN,
			    "SAM-QFS: OSD SCSI BUF = %16.16llx "
			    "%16.16llx %16.16llx\n",
			    *sb, *sb1, *sb2);
		}
		}
		break;
	default:
		break;
	}

	if (error) {
		if (iorp->ip != NULL) {
			dcmn_err((CE_WARN, "SAM-QFS: %s: ino %d.%d OSD "
			    "error %d SA=%x rc=%d", mp ? mp->mt.fi_name : " ",
			    iorp->ip->di.id.ino, iorp->ip->di.id.gen,
			    error, resp->service_action, resp->err_code));
		} else {
			dcmn_err((CE_WARN, "SAM-QFS: %s: OSD error %d SA=%x "
			    "rc=%d", mp ? mp->mt.fi_name : " ",
			    error, resp->service_action, resp->err_code));
		}
		if (bp) {
			bp->b_error = error;	/* Return error in buffer */
			bp->b_flags |= B_ERROR;
		}
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
		return (0);
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
	return (0);
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
	sam_mount_t		*mp,
	sam_osd_handle_t	oh,
	uint64_t		object_id)
{
	sam_osd_req_priv_t	ior_priv;
	sam_osd_req_priv_t	*iorp = &ior_priv;
	osd_req_t 		*reqp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	int			rc;
	int			error = 0;

	reqp = (sam_sosd_vec.setup_create_object)((void *)oh, partid,
	    object_id, 1);
	if (reqp == NULL) {
		return (EINVAL);
	}
	sam_osd_setup_private(iorp, mp);

	rc = (sam_sosd_vec.submit_req)(reqp, sam_object_req_done, iorp);
	if (rc == OSD_SUCCESS) {
		sam_osd_obj_req_wait(iorp);
		error = iorp->error;
	} else {
		error = sam_osd_map_errno(rc);
	}
	(sam_sosd_vec.free_req)(reqp);
	sam_osd_remove_private(iorp);
	return (error);
}


/*
 * ----- sam_create_object_id - Process create of user object id.
 */
int
sam_create_object_id(
	sam_mount_t		*mp,	/* Pointer to the mount table */
	struct sam_disk_inode	*dp)	/* Pointer to disk inode */
{
	sam_osd_req_priv_t	ior_priv;
	sam_osd_req_priv_t	*iorp = &ior_priv;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	sam_objlist_page_t	*objlist;
	osd_req_t		*reqp = NULL;
	sam_di_osd_t		*oip;
	struct sam_inode_ext	*eip;
	sam_id_t		ext_id;
	int			ord;
	int			n, i;
	int			num_group;
	uint32_t		obj_pt_type;
	struct buf		*bp = NULL;
	int			rc;
	int			error = 0;

	ASSERT(mp->mi.m_fs[dp->unit].oh != 0);
	dp->version = SAM_INODE_VERSION;
	dp->rm.ui.flags |= RM_OBJECT_FILE;

	/*
	 * Set up the create request and add in the Get Page Attributes
	 * to create an object ID.
	 */
	objlist = (sam_objlist_page_t *)kmem_alloc(
	    sizeof (sam_objlist_page_t), KM_SLEEP);
	oip = (sam_di_osd_t *)(void *)&dp->extent[0];
	ord = dp->unit;
	obj_pt_type = mp->mi.m_fs[ord].part.pt_type;
	sam_osd_setup_private(iorp, mp);

	/*
	 * Set the stripe width in num_group and get optional inode extent(s).
	 */
	if (error = sam_osd_inode_extent(mp, dp, &num_group)) {
		goto fini;
	}

	/*
	 * Vertical object stripe depth set with setfa -v.
	 * Defaults to mount parameter obj_depth;.
	 */
	if (dp->rm.info.obj.stripe_shift == 0) {
		dp->rm.info.obj.stripe_shift = mp->mt.fi_obj_depth_shift;
	}

	/*
	 * Get object IDs for each member of the stripe.
	 */
	n = 0;
	i = 0;
	ext_id = oip->ext_id;
	while (n < num_group) {
		sam_id_attr_t x;
		uint64_t object_id;

		reqp = (sam_sosd_vec.setup_create_object)(
		    (void *)mp->mi.m_fs[ord].oh, partid, 0, 1);
		if (reqp == NULL) {
			error = EINVAL;
			goto fini;
		}
		(sam_sosd_vec.add_get_page_attr_to_req)(reqp,
		    OSD_SAMQFS_VENDOR_CREATED_OBJECTS_LIST_PAGE,
		    sizeof (sam_objlist_page_t), (void *) objlist);
		sam_osd_setup_private(iorp, mp);
		rc = (sam_sosd_vec.submit_req)(reqp, sam_object_req_done, iorp);
		if (rc != OSD_SUCCESS) {
			error = sam_osd_map_errno(rc);
			break;
		}
		sam_osd_obj_req_wait(iorp);
		if ((error = iorp->error) != 0) {
			break;
		}
		object_id = BE_64(objlist->solp_object_id[0]);
		if (n < SAM_OSD_DIRECT) {
			oip->obj_id[n] = object_id;
			oip->ord[n] = (ushort_t)ord;
		} else {
			if ((error = sam_get_object_id_ext(mp, dp, n, ext_id,
			    &bp, &eip))) {
				break;
			}
			eip->ext.obj.obj_id[i] = object_id;
			eip->ext.obj.ord[i] = (ushort_t)ord;
			i++;
			if (i >= SAM_OSD_EXTENT) {
				ext_id = eip->hdr.next_id;
				bdwrite(bp);
				bp = NULL;
				i = 0;
			}
		}
		dp->rm.info.obj.num_group = ++n;

		/*
		 * Replace OSD_USER_OBJECT_INFORMATION_USERNAME
		 * w/ QFS vendor specific attribute for ino/gen.
		 */
		x.id.ino = BE_32(dp->id.ino);
		x.id.gen = BE_32(dp->id.gen);
		error = sam_set_user_object_attr(mp, dp, ord,
		    object_id, OSD_USER_OBJECT_INFO_USERNAME, x.attr);
		if (error) {
			break;
		}
		if (n >= num_group) {
			break;
		}
		if (++ord >= mp->mt.fs_count) {
			ord = 0;
		}
		while (obj_pt_type != mp->mi.m_fs[ord].part.pt_type ||
		    mp->mi.m_fs[ord].part.pt_state != DEV_ON) {
			if (++ord >= mp->mt.fs_count) {
				ord = 0;
			}
			if (ord == dp->unit) {
				break;
			}
		}
		if (ord == dp->unit) {		/* If wrapped */
			break;
		}
	}
fini:
	sam_osd_remove_private(iorp);
	kmem_free(objlist, sizeof (sam_objlist_page_t));
	if (bp) {
		bdwrite(bp);
	}
	if (reqp) {
		(sam_sosd_vec.free_req)(reqp);
	}
	return (error);
}


/*
 * ----- sam_get_object_id_ext - Get object inode extension.
 */
static int
sam_get_object_id_ext(
	sam_mount_t		*mp,	/* Pointer to the mount table */
	struct sam_disk_inode	*dp,	/* Pointer to disk inode */
	int			n,	/* Current index */
	sam_id_t		eid,	/* Ext inode */
	buf_t			**bpp,	/* Ptr ptr to buffer */
	struct sam_inode_ext	**eipp)	/* Ptr ptr to inode ext */
{
	int		n_ext;
	buf_t		*bp;
	struct sam_inode_ext *eip;
	int		error;

	bp = *bpp;
	eip = *eipp;
	n_ext = (n - SAM_OSD_DIRECT + SAM_OSD_EXTENT) / SAM_OSD_EXTENT;
	if (bp != NULL) {
		if (n_ext == (eip->hdr.mode & S_IFORD)) {
			return (0);
		}
		dcmn_err((CE_WARN, "SAM-QFS: %s: 1. Get Ext"
		    " ino=%d.%d, n_ext=%d, cnt=%x, %d.%d\n",
		    mp->mt.fi_name, dp->id.ino, dp->id.gen,
		    n_ext, eip->hdr.mode & S_IFORD, eip->hdr.id.ino,
		    eip->hdr.id.gen));
		error = EINVAL;
		return (error);
	}

	ASSERT(eid.ino);
	error = sam_read_ino(mp, eid.ino, &bp, (struct sam_perm_inode **)&eip);
	if (error) {
		return (error);
	}
	*bpp = bp;
	*eipp = eip;
	if (EXT_HDR_ERR_DP(eip, eid, dp) || !S_ISOBJ(eip->hdr.mode)) {
		sam_req_ifsck(mp, -1, "sam_set_object_id: EXT_HDR", &dp->id);
		return (ENOCSI);
	}
	if (n_ext != (eip->hdr.mode & S_IFORD)) {
		dcmn_err((CE_WARN, "SAM-QFS: %s: 2. Get Ext"
		    " ino=%d.%d, n_ext=%d, cnt=%x, %d.%d\n",
		    mp->mt.fi_name, dp->id.ino, dp->id.gen, n_ext,
		    eip->hdr.mode & S_IFORD, eip->hdr.id.ino, eip->hdr.id.gen));
	}
	return (0);
}


/*
 * ----- sam_osd_inode_extent - Get extention inodes for striping.
 */
static int
sam_osd_inode_extent(
	sam_mount_t		*mp,		/* Pointer to the mount table */
	struct sam_disk_inode	*dp,		/* Pointer to disk inode */
	int			*num_groupp)	/* Returned stripe width */
{
	sam_di_osd_t *oip;
	int num_group;
	int unit;
	int error = 0;

	oip = (sam_di_osd_t *)(void *)&dp->extent[0];
	unit = dp->unit;

	/*
	 * Horizontal object stripe width set with setfa -h.
	 * Defaults to mount parameter obj_width;.
	 */
	num_group = (dp->status.b.stripe_width) ?
	    dp->rm.info.obj.stripe_width : mp->mt.fi_obj_width;
	num_group = (num_group == 0) ? mp->mi.m_fs[unit].num_group : num_group;
	if (num_group > SAM_OSD_DIRECT) {
		int n_exts, ng;

		ng = num_group - SAM_OSD_DIRECT;
		n_exts = howmany(ng, SAM_OSD_EXTENT);
		error = sam_alloc_inode_ext_dp(mp, dp, S_IFOBJ, n_exts,
		    &oip->ext_id);
	}
	*num_groupp = num_group;
	return (error);
}


/*
 * ----- sam_remove_object_id - Process remove_object id.
 */
int
sam_remove_object_id(
	sam_mount_t	*mp,		/* Pointer to the mount table */
	uint64_t	object_id,	/* 64-bit user object identifier */
	uchar_t		unit)		/* Ordinal */
{
	sam_osd_req_priv_t	ior_priv;
	sam_osd_req_priv_t	*iorp = &ior_priv;
	osd_req_t		*reqp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	int			rc;
	int			error = 0;

	reqp = (sam_sosd_vec.setup_remove_object)((void *)mp->mi.m_fs[unit].oh,
	    partid, object_id);
	if (reqp == NULL) {
		return (EINVAL);
	}
	sam_osd_setup_private(iorp, mp);

	rc = (sam_sosd_vec.submit_req)(reqp, sam_object_req_done, iorp);
	if (rc == OSD_SUCCESS) {
		sam_osd_obj_req_wait(iorp);
		error = iorp->error;
	} else {
		error = sam_osd_map_errno(rc);
	}
	(sam_sosd_vec.free_req)(reqp);
	sam_osd_remove_private(iorp);
	return (error);
}


/*
 * ----- sam_get_user_object_attr - Process get user attribute call.
 */
static int
sam_get_user_object_attr(
	sam_mount_t		*mp,		/* Pointer to the mount table */
	sam_osd_handle_t	oh,		/* Object device handle */
	uint64_t		object_id,	/* OSD group object id */
	int64_t			*attrp)		/* Returned attribute */
{
	sam_osd_req_priv_t	ior_priv;
	sam_osd_req_priv_t	*iorp = &ior_priv;
	sam_objinfo_page_t	user_attr;
	osd_req_t		*reqp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	int			rc;
	int			error = 0;

	reqp = (sam_sosd_vec.setup_get_attr)((void *)oh, partid, object_id);
	if (reqp == NULL) {
		return (EINVAL);
	}
	(sam_sosd_vec.add_get_page_attr_to_req)(reqp,
	    OSD_SAMQFS_VENDOR_USERINFO_ATTR_PAGE_NO,
	    sizeof (sam_objinfo_page_t), (void *) &user_attr);
	sam_osd_setup_private(iorp, mp);
	rc = (sam_sosd_vec.submit_req)(reqp, sam_object_req_done, iorp);
	if (rc == OSD_SUCCESS) {
		sam_osd_obj_req_wait(iorp);
		if ((error = iorp->error) == 0) {
			sam_objinfo_page_t *uap = &user_attr;

			/* Return blocks allocated (4K units). */
			*attrp = SOP_BYTES_ALLOC(uap) >> SAM_SHIFT;
		} else {
			dcmn_err((CE_WARN, "SAM-QFS: %s: OSD GET user attr"
			    " rc=%d, errno=%d", mp->mt.fi_name, iorp->err_code,
			    error));
		}
	} else {
		error = sam_osd_map_errno(rc);
	}
	sam_osd_remove_private(iorp);

fini:
	if (reqp) {
		(sam_sosd_vec.free_req)(reqp);
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
	osd_list_type_retrieve_t	*out_listp;

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
	sam_mount_t		*mp,		/* Pointer to the mount table */
	struct sam_disk_inode	*dp,		/* Pointer to disk inode */
	int			ord,		/* OSD group ordinal */
	uint64_t		object_id,	/* OSD group object id */
	uint32_t		attr_num,	/* Attribute number */
	int64_t			attribute)	/* Attribute */
{
	sam_osd_req_priv_t	ior_priv;
	sam_osd_req_priv_t	*iorp = &ior_priv;
	osd_req_t		*reqp;
	uint64_t		partid = SAM_OBJ_PAR_ID;
	int			rc;
	int			error = 0;

	reqp = (sam_sosd_vec.setup_set_attr)((void *)mp->mi.m_fs[ord].oh,
	    partid, object_id);
	if (reqp == NULL) {
		return (EINVAL);
	}
	(sam_sosd_vec.add_set_page_1attr_cdb)(reqp,
	    OSD_USER_OBJECT_INFORMATION_PAGE, attr_num,
	    sizeof (attribute), (char *)&attribute);
	sam_osd_setup_private(iorp, mp);
	rc = (sam_sosd_vec.submit_req)(reqp, sam_object_req_done, iorp);
	if (rc == OSD_SUCCESS) {
		sam_osd_obj_req_wait(iorp);
		if ((error = iorp->error) != 0) {
			dcmn_err((CE_WARN, "SAM-QFS: %s: 1. OSD SET attr %x"
			    " rc=%d errno=%d, ino=%d.%d, setattr = 0x%llx",
			    mp->mt.fi_name, attr_num, iorp->err_code, error,
			    dp->id.ino, dp->id.gen, (long long)attribute));
		}
	} else {
		error = sam_osd_map_errno(rc);
	}
	sam_osd_remove_private(iorp);
fini:
	if (reqp) {
		(sam_sosd_vec.free_req)(reqp);
	}
	return (error);
}


/*
 * ----- sam_truncate_object_file - Truncate an osd file.
 */
int
sam_truncate_object_file(
	sam_node_t *ip,		/* Pointer to the inode */
	sam_truncate_t tflag,	/* Truncate file or Release file */
	offset_t size,		/* Current size of the file */
	offset_t length)	/* New file size */
{
	sam_mount_t		*mp = ip->mp;
	sam_di_osd_t		*oip;
	struct sam_inode_ext	*eip;
	struct buf		*bp = NULL;
	int			offline = ip->di.status.b.offline;
	uchar_t			unit = ip->di.unit;
	sam_id_t		ext_id;
	sam_disk_inode_t	di;
	int			num_group;
	int			error;


	bcopy((char *)&ip->di, (char *)&di, sizeof (sam_disk_inode_t));
	oip = (sam_di_osd_t *)(void *)&di.extent[0];
	ext_id = oip->ext_id;
	num_group = ip->di.rm.info.obj.num_group;

	/*
	 * Sync the inode to make sure the size is updated on disk.
	 * If error syncing inode for all except PURGE, restore previous state.
	 */
	ASSERT(tflag != SAM_REDUCE);
	if ((error = sam_sync_inode(ip, length, tflag))) {
		if (tflag == SAM_PURGE) {
			return (EIO);
		}

		bcopy((char *)oip, (char *)&di.extent[0],
		    sizeof (sam_di_osd_t));
		ip->di.rm.info.obj.num_group = num_group;
		ip->di.status.b.offline = offline;
		ip->di.rm.size = size;
		ip->di.unit = unit;
		return (EIO);
	}

	/*
	 * For PURGE (remove), delete all objects on the OSDs.
	 * Note, disk inode was zeroed in sam_sync_inode.
	 */
	if ((length == 0) && (tflag == SAM_PURGE)) {
		int i, n;

		i = 0;
		for (n = 0; n < num_group; n++) {
			if (n < SAM_OSD_DIRECT) {
				(void) sam_remove_object_id(mp,
				    oip->obj_id[n], oip->ord[n]);
			} else {
				if ((error = sam_get_object_id_ext(mp, &di, n,
				    ext_id, &bp, &eip))) {
					break;
				}
				(void) sam_remove_object_id(mp,
				    eip->ext.obj.obj_id[i],
				    eip->ext.obj.ord[i]);
				i++;
				if (i >= SAM_OSD_EXTENT) {
					ext_id = eip->hdr.next_id;
					bdwrite(bp);
					bp = NULL;
					i = 0;
				}
			}
		}
		if (bp) {
			brelse(bp);
		}

		/*
		 * For PURGE (remove), remove any extention inodes.
		 */
		if (oip->ext_id.ino) {
			sam_free_inode_ext_dp(mp, &di, S_IFOBJ,
			    SAM_ALL_COPIES, &oip->ext_id);
		}
		return (0);
	}

	/*
	 * Truncate up/down, reset end of object for all appropriate stripes.
	 */
	ASSERT(ip->olp);
	error = sam_set_end_of_obj(ip, length, 1);

	/*
	 * For RELEASE, update allocated block count.  Need to query
	 * the storage nodes for partial release.
	 */
	if (tflag == SAM_RELEASE) {
		if (length == 0) {
			ip->di.blocks = 0;
		} else {
			sam_osd_update_blocks(ip, 1);
		}
	}
	return (error);
}


/*
 * ----- sam_set_end_of_obj - Set the end of object for all OSDs.
 */
int
sam_set_end_of_obj(
	sam_node_t *ip,		/* Pointer to the inode */
	offset_t length,	/* File size */
	int	update)		/* Set if update OSD LUN */
{
	sam_obj_layout_t	*olp;
	int			num_group;
	int			i;
	sam_ioblk_t		ioblk;
	int			error = 0;

	if ((error = sam_map_osd(ip, length, 0, SAM_ALLOC_BLOCK, &ioblk))) {
		return (error);
	}
	num_group = ip->di.rm.info.obj.num_group;
	olp = ip->olp;
	ASSERT(olp);
	for (i = 0; i < num_group; i++) {
		offset_t	eoo;

		if (i < ioblk.obji) {
			eoo = ioblk.blk_off + ioblk.bsize;
		} else if (i == ioblk.obji) {
			eoo = ioblk.blk_off + ioblk.pboff;
		} else {
			eoo = ioblk.blk_off;
		}
		if (update && (olp->ol[i].eoo != eoo)) {
			error = sam_set_user_object_attr(ip->mp,
			    &ip->di, olp->ol[i].ord, olp->ol[i].obj_id,
			    OSD_USER_OBJECT_INFO_LOGICAL_LENGTH,
			    BE_64((int64_t)eoo));
		}
		olp->ol[i].eoo = eoo;
	}
	return (error);
}


/*
 * ----- sam_map_osd - Map request to osds.
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
				/* SAM_ALLOC_BLOCK don't update size */
				/* SAM_ALLOC_BITMAP alloc & bld bit map(srvr) */
				/* SAM_ALLOC_ZERO allocate and zero. */
	sam_ioblk_t *iop)	/* Ioblk array. */
{
	sam_obj_layout_t	*olp;
	offset_t		n;


	TRACE(T_SAM_OKMAP, SAM_ITOP(ip), (sam_tr_t)offset,
	    (sam_tr_t)count, (sam_tr_t)flag);
	ASSERT(ip->olp);
	olp = ip->olp;
	if (iop) {
		bzero((char *)iop, sizeof (sam_ioblk_t));
		iop->imap.flags = (M_OBJECT | M_VALID);
		iop->contig = ip->mp->mi.m_maxphys;
		if (flag <= SAM_RD_DIRECT_IO) {		/* If reading */
			offset_t size;

			size = (ip->size + PAGESIZE - 1) & PAGEMASK;
			iop->contig = size - offset;
			if (iop->contig > ip->mp->mi.m_maxphys) {
				iop->contig = ip->mp->mi.m_maxphys;
			}
		}
		iop->imap.ord0 = ip->di.unit;
		if (ip->di.rm.info.obj.num_group > 1) {
			int stripe_shift;

			iop->num_group = ip->di.rm.info.obj.num_group;
			stripe_shift = ip->di.rm.info.obj.stripe_shift;
			if (stripe_shift == 0) {
				stripe_shift = ip->mp->mt.fi_obj_depth_shift;
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
			iop->ord = olp->ol[n].ord;
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
		if (iop == NULL) {
			return (0);
		}
		if (iop->blk_off >= olp->ol[iop->obji].eoo) {
			iop->imap.flags |= M_OBJ_EOF;
		}
		return (0);
	}

	/*
	 * If writing, increment filesize if no error & flag < SAM_WRITE_SPARSE.
	 * If paged write past size, clear beginning of page.
	 */
	if (((offset + count) < ip->size) || (flag >= SAM_WRITE_SPARSE)) {
		return (0);
	}
	ASSERT(RW_WRITE_HELD(&ip->inode_rwl) || (flag == SAM_WR_DIRECT_IO));
	if (flag == SAM_WRITE && offset > ip->size) {
		struct fbuf *fbp;
		offset_t off = offset & PAGEMASK;
		offset_t len;

		if (off < ip->size) {
			off = ip->size;
		}
		len = (offset & PAGEOFFSET) - (off & PAGEOFFSET);
		if (len > 0) {
			SAM_SET_LEASEFLG(ip->mp);
			fbzero(SAM_ITOV(ip), off, len, &fbp);
			SAM_CLEAR_LEASEFLG(ip->mp);
			fbrelse(fbp, S_WRITE);
		}
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


/*
 * ----- sam_update_blocks - update inode allocated block count.
 */
void
sam_osd_update_blocks(
	sam_node_t *ip,		/* Pointer to the inode. */
	int pflag)		/* Partial release block count flag. */
{
	sam_obj_layout_t *olp;
	sam_osd_handle_t oh;
	uint64_t object_id;
	int64_t attr;
	int32_t blocks;
	int bn_shift = ip->mp->mi.m_bn_shift;
	int error;
	int i;
	int num_group;
	int partial = ip->di.psize.partial;

	attr = 0;
	blocks = 0;
	num_group = ip->di.rm.info.obj.num_group;
	ASSERT(num_group);
	ASSERT(ip->olp);
	olp = ip->olp;
	for (i = 0; i < num_group; i++) {
		object_id = olp->ol[i].obj_id;
		if (object_id == 0) {
			continue;
		}
		oh = ip->mp->mi.m_fs[olp->ol[i].ord].oh;
		error = sam_get_user_object_attr(ip->mp,
		    oh, object_id, &attr);
		if (error == 0) {
			blocks += (int32_t)attr;
		} else {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: Error=%d, Eq=%d, ino=%d.%d, "
			    "Cannot get allocated block count.",
			    ip->mp->mt.fi_name, error,
			    ip->mp->mi.m_fs[i].part.pt_eq,
			    ip->di.id.ino, ip->di.id.gen);
		}
		/*
		 * For partial release optimization, only count the number
		 * of blocks for the partial release size.
		 */
		if (pflag && ((blocks << bn_shift) >= partial)) {
			break;
		}
	}
	ip->di.blocks = blocks;
}


/*
 * ----- sam_osd_create_obj_layout - build the object layout.
 *  For the given inode, allocate the object layout array which contains the
 *  object id, ordinal, and end of object (eoo). sam_osd_create_obj_layout
 *  is called when we call SAM_HASH_INO.
 *
 */
int
sam_osd_create_obj_layout(
	sam_node_t *ip)		/* Pointer to the inode. */
{
	sam_di_osd_t		*oip;
	sam_obj_layout_t	*olp;
	struct sam_inode_ext	*eip;
	sam_id_t		ext_id;
	buf_t			*bp = NULL;
	int			n, i;
	int			num_group;
	int			error;

	num_group = ip->di.rm.info.obj.num_group;
	TRACE(T_SAM_OBJ_LAY_CRE, SAM_ITOP(ip), ip->di.id.ino,
	    ip->di.id.gen, num_group);
	ASSERT(num_group > 0);
	ASSERT((RW_OWNER_OS(&ip->inode_rwl) == curthread) ||
	    MUTEX_HELD(&ip->fl_mutex));

	/*
	 * Get object layout array. Use existing one if the length is the same
	 */
	if (ip->olp) {
		if (num_group != ip->olp->num_group) {
			int ow = ip->olp->num_group;

			kmem_free(ip->olp, sizeof (sam_obj_layout_t) +
			    (sizeof (sam_obj_ent_t) * (ow - 1)));
			ip->olp = NULL;
		} else {
			bzero((char *)ip->olp, sizeof (sam_obj_layout_t) +
			    (sizeof (sam_obj_ent_t) * (num_group - 1)));
		}
	}
	if (ip->olp == NULL) {
		ip->olp = kmem_zalloc(sizeof (sam_obj_layout_t) +
		    (sizeof (sam_obj_ent_t) * (num_group - 1)), KM_SLEEP);
	}
	olp = ip->olp;
	oip = (sam_di_osd_t *)(void *)&ip->di.extent[0];
	i = 0;
	ext_id = oip->ext_id;
	for (n = 0; n < num_group; n++) {
		if (n < SAM_OSD_DIRECT) {
			olp->ol[n].obj_id = oip->obj_id[n];
			olp->ol[n].eoo = 0;
			olp->ol[n].ord = oip->ord[n];
		} else {
			if ((error = sam_get_object_id_ext(ip->mp, &ip->di,
			    n, ext_id, &bp, &eip))) {
				if (bp) {
					brelse(bp);
				}
				return (error);
			}
			olp->ol[n].obj_id = eip->ext.obj.obj_id[i];
			olp->ol[n].eoo = 0;
			olp->ol[n].ord = eip->ext.obj.ord[i];
			i++;
			if (i >= SAM_OSD_EXTENT) {
				ext_id = eip->hdr.next_id;
				brelse(bp);
				bp = NULL;
				i = 0;
			}
		}
	}
	olp->num_group = num_group;
	olp->id = ip->di.id;		/* Set for debug only */
	if (bp) {
		brelse(bp);
	}
	error = sam_set_end_of_obj(ip, ip->di.rm.size, 1);
	return (error);
}


/*
 * ----- sam_osd_destroy_obj_layout - free the object layout array.
 *  sam_osd_destroy_obj_layout is called when we call SAM_UNHASH_INO.
 *  It is also called when we call sam_destroy_ino because the inode
 *  may have been put on the freelist with pages. Note, we need the
 *  object layout if dirty pages are flushed while the inode is on
 *  the freelist.
 *
 *  Generally this function should be called via the macro,
 *  SAM_DESTROY_OBJ_LAYOUT which does not destroy the object layout
 *  if any pages exist, and asserts proper locking is in place.
 *  You may call this function directly if you are sure no pages exist.
 */
void
sam_osd_destroy_obj_layout(
	sam_node_t *ip,
	int lock_held)
{
	int num_group;

	TRACE(T_SAM_OBJ_LAY_DES, SAM_ITOP(ip), ip->di.id.ino,
	    ip->di.id.gen, (sam_tr_t)ip->olp);
	num_group = ip->olp->num_group;
	ASSERT(num_group > 0);
	if (lock_held) {
		ASSERT((RW_OWNER_OS(&ip->inode_rwl) == curthread) ||
		    MUTEX_HELD(&ip->fl_mutex));
	}
	kmem_free(ip->olp, sizeof (sam_obj_layout_t) +
	    (sizeof (sam_obj_ent_t) * (num_group - 1)));
	ip->olp = NULL;
}


/*
 * ----- sam_init_object_cache - create object memory cache.
 */
void
sam_init_object_cache()
{
	samgt.object_cache = kmem_cache_create("sam_object_cache",
	    sizeof (sam_osd_req_priv_t), 0, NULL, NULL, NULL, NULL, NULL, 0);
	ASSERT(samgt.object_cache);
}


/*
 * ----- sam_delete_object_cache - destroy object memory cache.
 */
void
sam_delete_object_cache()
{
	if (samgt.object_cache) {
		kmem_cache_destroy(samgt.object_cache);
	}
}
