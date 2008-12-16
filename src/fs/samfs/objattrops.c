/*
 * ----- objattrops.c - Object attributes routines.
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

#pragma ident "$Revision: 1.6 $"

#include "sam/osversion.h"

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <sys/cmn_err.h>
#include <sys/fs_subr.h>
#include <vm/seg_map.h>
#include <sys/policy.h>

#include "sys/cred.h"
#include "sam/types.h"
#include "sam/uioctl.h"
#include "sam/resource.h"
#include "pub/sam_errno.h"
#include "pub/rminfo.h"

#include "pub/objnops.h"

#include "samfs.h"
#include "inode.h"
#include "mount.h"
#include "ino_ext.h"
#include "ioblk.h"
#include "extern.h"
#include "arfind.h"
#include "trace.h"
#include "debug.h"
#include "kstats.h"
#include "qfs_log.h"
#include "scsi_osd.h"
#include "osd.h"
#include "object.h"

int sam_obj_get_userinfo_pg(objnode_t *objnodep, uint32_t pagenum,
    uint64_t len, void *buf, cred_t *credp);
int sam_obj_get_fsinfo_pg(objnode_t *objnodep, uint32_t pagenum, uint64_t len,
    void *buf, cred_t *credp);

/*
 * sam_objattr_set_file_samid - Set the owning file sam_id of this object.
 * This is the sam_id of the file on the MDS.
 *
 * OSD_USER_OBJECT_INFORMATION_PAGE and OSD_USER_OBJECT_INFORMATION_USERNAME
 */
/* ARGSUSED */
static int
sam_objattr_set_file_samid(objnode_t *objnodep, uint16_t len, void *buf,
	cred_t *credp)
{

	sam_node_t *ip = NULL;
	sam_mount_t *mp;
	sam_id_t *samid;
	int error = 0;

	samid = (sam_id_t *)buf;
	ip = (struct sam_node *)(objnodep->obj_data);
	RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	mp = ip->mp;

	ip->di.parent_id.ino = BE_32(samid->ino);
	ip->di.parent_id.gen = BE_32(samid->gen);
	TRANS_INODE(mp, ip);
	sam_mark_ino(ip, SAM_CHANGED);
	RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	return (error);
}

/*
 * sam_objattr_get_file_samid - Return the owning file's samid of this object.
 * This is the sam_id of the file on the MDS.
 *
 * OSD_USER_OBJECT_INFORMATION_PAGE and OSD_USER_OBJECT_INFORMATION_USERNAME
 */
/* ARGSUSED */
static int
sam_objattr_get_file_samid(objnode_t *objnodep, uint16_t len, void *buf,
	cred_t *credp)
{

	sam_node_t *ip = NULL;
	sam_id_t samid;

	ip = (struct sam_node *)(objnodep->obj_data);
	RW_LOCK_OS(&ip->data_rwl, RW_READER);
	RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	samid.ino = BE_32(ip->di.parent_id.ino);
	samid.gen = BE_32(ip->di.parent_id.gen);
	memcpy((char *)buf, (char *)&samid, sizeof (uint64_t));
	RW_UNLOCK_OS(&ip->data_rwl, RW_READER);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	return (0);
}

/*
 * objattr_set_eof - Sets the EOF of the Object.
 *
 * OSD_USER_OBJECT_INFORMATION_PAGE OSD_USER_OBJECT_INFORMATION_LOGICAL_LENGTH
 */
/* ARGSUSED */
static int
sam_objattr_set_eof(objnode_t *objnodep, offset_t offset, cred_t *credp)
{
	sam_node_t *ip = NULL;
	int error;

	ip = (struct sam_node *)(objnodep->obj_data);
	RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	error = sam_truncate_ino(ip, BE_64(offset), SAM_TRUNCATE, credp);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
	return (error);
}

/*
 * objattr_get_eof - Get the EOF of the object.
 */
/* ARGSUSED */
static int
sam_objattr_get_eof(objnode_t *objnodep, offset_t *offset, cred_t *credp)
{
	sam_node_t *ip = NULL;

	ip = (struct sam_node *)(objnodep->obj_data);
	RW_LOCK_OS(&ip->data_rwl, RW_READER);
	RW_LOCK_OS(&ip->inode_rwl, RW_READER);
	*offset = BE_64(ip->di.rm.size);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	RW_UNLOCK_OS(&ip->data_rwl, RW_READER);
	return (0);
}


/*
 * sam_obj_setattr - Sets 1 attribute in the Attribute Page.
 *
 */
/* ARGSUSED */
int
sam_obj_setattr(objnode_t *objnodep, uint32_t pagenum, uint32_t attrnum,
    uint64_t len, void *buf, void *cap, void *sec, cred_t *credp)
{

	offset_t *offset;

	if (pagenum != OSD_USER_OBJECT_INFORMATION_PAGE) {
		cmn_err(CE_WARN, "sam_obj_setattr: Pageno %d not Supported\n");
		return (ENOTSUP);
	}

	if (attrnum == OSD_USER_OBJECT_INFO_USERNAME) {
		return (sam_objattr_set_file_samid(objnodep, len, buf, credp));
	}
	if (attrnum == OSD_USER_OBJECT_INFO_LOGICAL_LENGTH) {
		offset = (offset_t *)buf;
		return (sam_objattr_set_eof(objnodep, *offset, credp));
	}

	return (ENOTSUP);
}

/*
 * sam_obj_getattr - Get 1 attribute from an Attribute Page.
 *
 */
/* ARGSUSED */
int
sam_obj_getattr(objnode_t *objnodep, uint32_t pagenum, uint32_t attrnum,
    uint64_t len, void *buf, void *cap, void *sec, cred_t *credp)
{

	offset_t offset;

	if (pagenum != OSD_USER_OBJECT_INFORMATION_PAGE) {
		cmn_err(CE_WARN, "sam_obj_getattr: Pageno %d not Supported\n");
		return (ENOTSUP);
	}

	if (attrnum == OSD_USER_OBJECT_INFO_USERNAME) {
		return (sam_objattr_get_file_samid(objnodep, len, buf, credp));
	}
	if (attrnum == OSD_USER_OBJECT_INFO_LOGICAL_LENGTH) {
		return (sam_objattr_get_eof(objnodep, &offset, credp));
	}

	return (ENOTSUP);

}

/*
 * sam_obj_getattrpage - Get an attribute page.
 */
/* ARGSUSED */
int
sam_obj_getattrpage(objnode_t *objnodep, uint32_t pagenum, uint64_t len,
    void *buf, void *cap, void *sec, cred_t *credp)
{
	int status = 0;

	if (pagenum == OSD_SAMQFS_VENDOR_USERINFO_ATTR_PAGE_NO) {
		status = sam_obj_get_userinfo_pg(objnodep, pagenum, len, buf,
		    credp);
		return (status);
	}

	if (pagenum == OSD_SAMQFS_VENDOR_FS_ATTR_PAGE_NO) {
		status = sam_obj_get_fsinfo_pg(objnodep, pagenum, len, buf,
		    credp);
		return (status);
	}

	cmn_err(CE_WARN, "sam_obj_getattrpage: Pageno 0x%x Not Supported\n",
	    pagenum);

	return (ENOTSUP);
}

/*
 * sam_obj_get_userinfo_pg - Returns the SAMQFS defined user information
 * page.
 */
/* ARGSUSED */
int
sam_obj_get_userinfo_pg(objnode_t *objnodep, uint32_t pagenum, uint64_t len,
    void *buf, cred_t *credp)
{

	sam_node_t *ip = NULL;
	struct sam_objinfo_page *sopp;
	uint64_t *temp;

	if (pagenum != OSD_SAMQFS_VENDOR_USERINFO_ATTR_PAGE_NO) {
		cmn_err(CE_WARN,
		    "sam_obj_get_userinfo_pg: Page 0x%llx not supported\n",
		    pagenum);
		return (ENOTSUP);
	}

	ip = (struct sam_node *)(objnodep->obj_data);
	RW_LOCK_OS(&ip->data_rwl, RW_READER);
	RW_LOCK_OS(&ip->inode_rwl, RW_READER);

	sopp = (struct sam_objinfo_page *)buf;
	sopp->sop_pgno = BE_32((uint32_t)
	    OSD_SAMQFS_VENDOR_USERINFO_ATTR_PAGE_NO);
	sopp->sop_pglen = BE_32((sizeof (struct sam_objinfo_page)));
	sopp->sop_partid = BE_64((uint64_t)SAM_OBJ_PAR_ID);
	temp = (void *)&ip->di.id;
	sopp->sop_oid = BE_64(*temp);
	sopp->sop_append_addr = BE_64((uint64_t)ip->di.rm.size);
	sopp->sop_bytes_alloc = BE_64((uint64_t)ip->di.blocks << SAM_SHIFT);
	sopp->sop_logical_len = BE_64((uint64_t)ip->di.rm.size);
	sopp->sop_trans_id = BE_64((uint64_t)0);
	sopp->sop_fino = BE_32(ip->di.parent_id.ino);
	sopp->sop_fgen = BE_32(ip->di.parent_id.gen);

	RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
	RW_UNLOCK_OS(&ip->data_rwl, RW_READER);

	return (0);

}

/*
 * sam_obj_get_fsinfo_pg - Returns SAMQFS defined File System Information.
 */
/* ARGSUSED */
int
sam_obj_get_fsinfo_pg(objnode_t *objnodep, uint32_t pagenum, uint64_t len,
    void *buf, cred_t *credp)
{

	sam_node_t *ip = NULL;
	sam_mount_t *mp;
	struct sam_sblk *sblk;
	struct sam_fsinfo_page *obj_fsinfop;

	if (pagenum != OSD_SAMQFS_VENDOR_FS_ATTR_PAGE_NO) {
		cmn_err(CE_WARN,
		    "sam_obj_get_fsinfo_pg: Page 0x%llx not supported\n",
		    pagenum);
		return (ENOTSUP);
	}

	ip = (struct sam_node *)(objnodep->obj_data);
	mp = ip->mp;
	sblk = mp->mi.m_sbp;
	obj_fsinfop = (struct sam_fsinfo_page *)buf;
	obj_fsinfop->sfp_pgno = BE_32(
	    (uint32_t)OSD_SAMQFS_VENDOR_FS_ATTR_PAGE_NO);
	obj_fsinfop->sfp_pglen = BE_32(sizeof (struct sam_fsinfo_page));
	obj_fsinfop->sfp_capacity = BE_64(sblk->info.sb.capacity);
	obj_fsinfop->sfp_space = BE_64(sblk->info.sb.space);
	obj_fsinfop->sfp_sm_dau = sblk->info.sb.dau_blks[SM] * SAM_DEV_BSIZE;
	obj_fsinfop->sfp_sm_dau = BE_64(obj_fsinfop->sfp_sm_dau);
	obj_fsinfop->sfp_lg_dau = sblk->info.sb.dau_blks[LG] * SAM_DEV_BSIZE;
	obj_fsinfop->sfp_lg_dau = BE_64(obj_fsinfop->sfp_lg_dau);
	return (0);

}

/*
 * sam_objattr_get_list - Retrieve the attribute values from a list of Page
 * Numbers and Attribute numbers.
 */
/* ARGSUSED */
int
sam_objattr_get_list(objnode_t *objnodep, uint32_t listlen, void *listbuf,
	uint32_t *outlen, void **outbuf, cred_t *credp)
{
	return (ENOTSUP); /* deferred implementation */
}

/*
 * sam_objattr_set_list - List node of setting attributes.
 *
 */
/* ARGSUSED */
int
sam_objattr_set_list(objnode_t *objnodep, uint32_t listlen, void *listbuf,
	uint32_t inlen, void *inbuf, cred_t *credp)
{
	return (ENOTSUP); /* deferred implementation */
}
