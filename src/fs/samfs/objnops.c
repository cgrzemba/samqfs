/*
 * ----- objnops.c - Object Node Operations
 * These routines should never be called directly but rather through the
 * set of macros provided in objnops.h
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

#pragma ident "$Revision: 1.3 $"

#include "sam/osversion.h"

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <sys/cmn_err.h>
#include <sys/fs_subr.h>
#include <vm/seg_map.h>
#include <sys/nbmlock.h>
#include <sys/policy.h>

#include "sys/cred.h"
#include "sam/types.h"
#include "sam/uioctl.h"
#include "sam/resource.h"
#include "pub/sam_errno.h"
#include "pub/rminfo.h"
#include "scsi_osd.h"
#include "pub/objnops.h"
#include "pub/objattrops.h"

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
#include "objnode.h"

/*
 * sam_obj_busy
 * OSD commands has to be executed atomically to completion.  An OSD
 * command may require the invocation of more than 1 QFS Object function or
 * 1 QFS Object function multiple times .. potentially sleeping inbetween.
 *
 * This lock serialises OSD Object access at the OSD layer.
 *
 * Must not block trying to get the lock.
 * Calling thread will retry when appropriate.
 *
 * EAGAIN - Lock is busy.
 * EBUSY - Object is busy.
 * 0 - Caller has access to Object.
 *
 * Note: Caller is responsible for releasing the mutex.
 */
/* ARGSUSED */
int
sam_obj_busy(objnode_t *objnodep, cred_t *cr, caller_context_t *ct)
{

	if (mutex_tryenter(&objnodep->obj_mutex)) { /* Lock is busy */
		return (EAGAIN);
	} else { /* Got Lock. */
		if (objnodep->obj_busy) { /* Object is busy. */
			return (EBUSY);
		} else { /* Object is avaliable */
			objnodep->obj_busy++;
			return (0);
		}
	}
}

/*
 * sam_obj_unbusy - Unbusy the resource
 */
/* ARGSUSED */
int
sam_obj_unbusy(objnode_t *objnodep, cred_t *cr, caller_context_t *ct)
{

	/*
	 * Grab the lock and free the object.
	 * We should not be spinning too long here ..
	 */
	mutex_enter(&objnodep->obj_mutex);
	objnodep->obj_busy--;
	mutex_exit(&objnodep->obj_mutex);
	return (0);

}

/*
 * sam_obj_rele - Releases a hold on the given object.  It does nothing more
 * then a VN_RELE on the vnode of the object.
 */
/* ARGSUSED */
int
sam_obj_rele(objnode_t *objnodep)
{

	vnode_t *vp;

	vp = ((struct sam_node *)(objnodep->obj_data))->vnode;
	VN_RELE(vp);
	return (0);
}

/*
 * sam_obj_rw_enter - Grab Reader or Writer lock. This call is blocking.
 */
/* ARGSUSED */
void
sam_obj_rw_enter(objnode_t *objnodep, krw_t enter_type)
{
	rw_enter(&objnodep->obj_lock, enter_type);
}

/*
 * sam_obj_rw_tryenter - Try to grab Reader or Writer lock.  Do not block if
 *  not successful.
 */
/* ARGSUSED */
int
sam_obj_rw_tryenter(objnode_t *objnodep, krw_t enter_type)
{
	return (rw_tryenter(&objnodep->obj_lock, enter_type));
}

/*
 * sam_obj_rw_exit - Release the lock.
 */
/* ARGSUSED */
void
sam_obj_rw_exit(objnode_t *objnodep)
{
	rw_exit(&objnodep->obj_lock);
}

/*
 * sam_obj_par_create - Creates an OSD Partition Object.
 *  Parent object on entry and exit vnode held.
 *  Created object is returned vnode HELD.
 */
/* ARGSUSED */
int
sam_obj_par_create(objnode_t *pobjp, mode_t mode, uint64_t *oidp,
    objnode_t **objpp, void *cap, void *sec,
    void *curcmdpg, cred_t *credp)
{

	sam_mount_t *mp;
	int error = 0;

	mp = ((sam_node_t *)pobjp->obj_data)->mp;
	if (!mp) {
		error = EINVAL;
		cmn_err(CE_WARN, "samfs_obj_par_create: invalid mp\n");
		return (error);
	}

	/*
	 * For now we only support the creation of the OSD Default Partition
	 * This partition was created by sammkfs.  The Partition is identified
	 * by the Partition Id of SAM_OBJ_PAR_ID
	 */
	if (*oidp == SAM_OBJ_PAR_ID) {
		VN_HOLD(SAM_ITOV(mp->mi.m_osdfs_part));
		*objpp = (struct objnode *)mp->mi.m_osdfs_part->objnode;
	} else {
		/*
		 * We will support runtime creation of partitions nect release.
		 */
		*objpp = NULL;
		error = ENOTSUP;
		cmn_err(CE_WARN, "samfs_osdfs_par_create: Not supported.\n");
	}

	return (error);

}

/*
 * sam_obj_col_create - Creates an OSD Collection Object.
 *  Parent object on entry and exit vnode held.
 *  Created object is returned vnode HELD.
 *
 *  Add User Object to a Collection:
 *	Set User Attribute Page 4 Attribute Number 1 to ffffff00h
 *	to contain 8 bytes of Collection Id.
 *
 *	NOT DEFINED where in the Collection object holds the user Object Id!
 *
 * Remove User Object from a Collection:
 *
 */
/* ARGSUSED */
int
sam_obj_col_create(objnode_t *pobjp, mode_t mode, uint64_t *oidp,
    objnode_t **objpp, void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	vattr_t va;
	sam_node_t *pip;
	sam_node_t *ip;
	sam_mount_t *mp = NULL;
	struct objnode *objnodep;
	uint64_t *temp;
	int error;

	pip = (sam_node_t *)pobjp->obj_data;

	/*
	 * Create the inode that will represent the Collection Object.
	 */
	bzero((caddr_t)&va, sizeof (vattr_t));
	va.va_mode = mode;
	va.va_type = VREG;
	error = sam_make_ino(pip, &va, &ip, credp);
	if (error) {
		cmn_err(CE_WARN,
		    "sam_obj_col_create: Failed to create object %d\n", error);
		return (error);
	}

	/*
	 * If the OSD File System is in silmulation mode to
	 * test certain performance .. reset the objnops.
	 *
	 * For now we do allow IO operations on collection objects.
	 */
	objnodep = (struct objnode *)ip->objnode;
	mp = (sam_mount_t *)pip->mp;
	if (mp->mi.m_osdt_sim == OBJECTOPS_SIM_IOPS) {
		objnodep->obj_op = samfs_objnodeops_simiopsp;
	}

	if (mp->mi.m_osdt_sim == OBJECTOPS_SIM_MEM) {
		objnodep->obj_op = samfs_objnodeops_simmemp;
	}

	ip->di2.objtype = OBJCOL;
	TRANS_INODE(mp, ip);
	sam_mark_ino(ip, (SAM_UPDATED | SAM_CHANGED));

	temp = (void *)&ip->di.id;
	*oidp = *temp;
	*objpp = (struct objnode *)ip->objnode;
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

	return (0);
}

/*
 * sam_obj_create - Creates an OSD User Object.
 */
/* ARGSUSED */
int
sam_obj_create(objnode_t *pobjp, mode_t mode, uint64_t *created,
    uint64_t *buf, void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	vattr_t va;
	uint64_t num;
	uint64_t *objlist;
	sam_node_t *pip;
	sam_node_t *ip;
	struct objnode *objnodep;
	sam_mount_t *mp = NULL;
	uint64_t	*temp;
	int error;

	pip = (sam_node_t *)pobjp->obj_data;
	objlist = buf;
	num = *created;
	*created = 0;

	while (num) {
		/*
		 * Create the inode that will represent the Object.
		 */
		bzero((caddr_t)&va, sizeof (vattr_t));
		va.va_mode = mode;
		va.va_type = VREG;
		error = sam_make_ino(pip, &va, &ip, credp);
		if (error) {
			cmn_err(CE_WARN,
			    "sam_obj_create: Failed to create object %d\n",
			    error);
			break;
		}

		/*
		 * Change the object ops if we are in simulation mode.
		 */
		objnodep = (struct objnode *)ip->objnode;
		mp = (sam_mount_t *)ip->mp;
		if (mp->mi.m_osdt_sim == OBJECTOPS_SIM_IOPS) {
			objnodep->obj_op = samfs_objnodeops_simiopsp;
		} else {
			if (mp->mi.m_osdt_sim == OBJECTOPS_SIM_MEM) {
				objnodep->obj_op = samfs_objnodeops_simmemp;
			}
		}

		/*
		 * Set Object specific attributes in the inode.
		 * User Objects are initially modified to have the Ophan Object
		 * as it's parent.  When the MDS assigns this Object to a File,
		 * the parent_id of the Object is changed to the File's sam_id.
		 *
		 * If we crash after sam_make_ino() - ext_attrs is not set and
		 * it will be treated as an Ophan Inode when we samfsck.
		 *
		 * This is also a quick way to find orphan inodes if the
		 * orphan list is corrupted.
		 *
		 * Do it here instead of changing the bowels of QFS.
		 */
		ip->di.parent_id.ino = SAM_OBJ_ORPHANS_INO;
		ip->di.parent_id.gen = SAM_OBJ_ORPHANS_INO;
		ip->di2.objtype = OBJUSER;
		TRANS_INODE(mp, ip);
		sam_mark_ino(ip, (SAM_UPDATED | SAM_CHANGED));
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		temp = (void *)&ip->di.id;
		*objlist = *temp;
		num--; *created++; objlist++;
		VN_RELE(SAM_ITOV(ip));
	} /* while .. */

	if (!*created)
		return (error);

	return (0);
}

/*
 * sam_obj_remove
 * Remove the Object of the given handle.
 *
 * This set of code for now assumes it is a User Object
 *
 */
/* ARGSUSED */
int
sam_obj_remove(struct objnode *objnodep, void *cap, void *sec, void *curcmdpg,
    cred_t *credp)
{

	vnode_t *vp;
	struct sam_node *ip;
	int error = 0;

	vp = ((struct sam_node *)(objnodep->obj_data))->vnode;
	ip = SAM_VTOI(vp);

	/*
	 * Try to delete the inode ..
	 *
	 * We do not have hardlink to an Object .. if nlink != 1 .. an error
	 * has occurred somewhere ..
	 */
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	if (ip->di.nlink > 0) {
		if (ip->di.nlink != 1) {
			/*
			 * for now just log this weird value ..
			 */
			cmn_err(CE_WARN, "sam_obj_hdl_remove: Link count %d\n",
			    ip->di.nlink);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			return (EINVAL);
		}

		ip->di.nlink--;
		TRANS_INODE(ip->mp, ip);
		sam_mark_ino(ip, SAM_CHANGED);
	} else {
		/* Strange - nlink count is 0 .. */
		cmn_err(CE_WARN, "sam_obj_hdl_remove: Link count 0 %d\n",
		    ip->di.nlink);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		error = EINVAL;
		goto out;
	}

	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
out:
	/*
	 * We do not VN_RELE the vnode here because the caller still need
	 * to access the inode.
	 */
	return (error);
}

/*
 * sam_obj_read - Read data from OSD Object.
 */
/* ARGSUSED */
int
sam_obj_read(struct objnode *objnodep, uint64_t offset, uint64_t len,
    void *bufp, int segflg, int io_option, uint64_t *size_read,
    void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	vnode_t *vp;
	sam_node_t *ip;
	struct iovec iov;
	struct uio uio;
	int ioflag = 0;
	int error = 0;

	vp = ((struct sam_node *)(objnodep->obj_data))->vnode;
	ip = SAM_VTOI(vp);

	(void) VOP_RWLOCK(vp, V_WRITELOCK_FALSE, NULL);

	/*
	 * Should do a bunch of validation checks ..
	 */
	iov.iov_base = (caddr_t)bufp;
	iov.iov_len = (size_t)len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = (uio_seg_t)segflg;
	uio.uio_extflg = UIO_COPY_CACHED;
	uio.uio_loffset = offset;
	uio.uio_resid = (ssize_t)len;
	uio.uio_fmode = FREAD;
	uio.uio_llimit = SAM_MAXOFFSET_T;

	/*
	 * Directio or Page IO
	 */
	if (io_option == OBJECTIO_OPTION_DPO) {
		/*
		 * Set Directio Flag
		 */
		ip = SAM_VTOI(vp);
		ip->flags.bits |= SAM_DIRECTIO;
	}

	error = VOP_READ(vp, &uio, ioflag, CRED(), NULL);

	/*
	 * Turn off direct io.
	 */
	if (io_option == OBJECTIO_OPTION_DPO)
		ip->flags.bits &= ~SAM_DIRECTIO;

	*size_read = len - uio.uio_resid;

	VOP_RWUNLOCK(vp, V_WRITELOCK_FALSE, NULL);

out:
	return (error);
}

/*
 * sam_obj_write - Write data to OSD Object
 */
/* ARGSUSED */
int
sam_obj_write(struct objnode *objnodep, uint64_t offset, uint64_t len,
	void *bufp, int segflg, int io_option, uint64_t *size_written,
	void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	vnode_t *vp;
	struct iovec iov;
	struct uio uio;
	sam_node_t *ip;
	int ioflag = 0;
	int error = 0;

	vp = ((struct sam_node *)(objnodep->obj_data))->vnode;

	(void) VOP_RWLOCK(vp, V_WRITELOCK_TRUE, NULL);

	/*
	 * Should do a bunch of validation checks ..
	 */
	iov.iov_base = (caddr_t)bufp;
	iov.iov_len = (size_t)len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = (uio_seg_t)segflg;
	uio.uio_extflg = UIO_COPY_CACHED;
	uio.uio_loffset = offset;
	uio.uio_resid = (ssize_t)len;
	uio.uio_fmode = FWRITE;
	uio.uio_llimit = SAM_MAXOFFSET_T;

	/*
	 * Directio, Page IO or FSYNC
	 */
	if (io_option == OBJECTIO_OPTION_DPO) { /* By Pass Cache */
		ip = SAM_VTOI(vp);
		ip->flags.bits |= SAM_DIRECTIO;
	} else if (io_option == OBJECTIO_OPTION_FUA) /* Flush to disk */
		ioflag = FSYNC;

	error = VOP_WRITE(vp, &uio, ioflag, CRED(), NULL);
	/*
	 * Turn off direct io.
	 */
	if (io_option == OBJECTIO_OPTION_DPO)
		ip->flags.bits &= ~SAM_DIRECTIO;

	*size_written = len - uio.uio_resid;
	VOP_RWUNLOCK(vp, V_WRITELOCK_TRUE, NULL);

out:
	return (error);
}

/*
 * sam_obj_flush - Flush User Data and or attributes to stable storage.
 *  flush_scope:
 *    0 - Flush all data and attributes
 *    1 - Attributes only
 *    2 - Data range and attributes
 */
/* ARGSUSED */
int
sam_obj_flush(struct objnode *objnodep, uint64_t offset, uint64_t len,
	uint8_t flush_scope, void *cap, void *sec, void *curcmdpg,
	cred_t *credp)
{
	vnode_t *vp;
	int error = 0;

	/*
	 * Flush data.
	 */
	if ((flush_scope == 0) || (flush_scope == 2)) {
		vp = ((struct sam_node *)(objnodep->obj_data))->vnode;
		error = VOP_FSYNC(vp, FSYNC, credp, NULL);
		if (error) {
			cmn_err(CE_WARN, "sam_obj_flush errror %d\n", error);
			return (error);
		}
	}

	/*
	 * Always flush attributes.
	 */
	cmn_err(CE_WARN,
	    "sam_obj_flush: Attributes flushed NOT implemented.\n");

	return (error);
}

/*
 * sam_obj_colflush - Flush collection
 *  flush_scope:
 *    0 - Flush data and attributes of objects in collection
 *    1 - Flush Collection attributes only
 *    2 - Action 0 and 1 above.
 */
/* ARGSUSED */
int
sam_obj_colflush(struct objnode *objnodep, char flush_scope,
    void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	int error = 0;

	/*
	 * Always flush attributes.
	 */
	cmn_err(CE_WARN,
	    "sam_obj_colflush: Attributes flushed NOT implemented.\n");
	error = ENOTSUP;
	return (error);
}

/*
 * sam_obj_truncate - Truncate the OSD User Object
 *
 */
/* ARGSUSED */
int
sam_obj_truncate(struct objnode *objnodep, uint64_t offset,
    void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	sam_node_t *ip = NULL;
	int error = 0;

	ip = (struct sam_node *)(objnodep->obj_data);
	RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	error = sam_truncate_ino(ip, offset, SAM_TRUNCATE, CRED());
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
	if (error)
		cmn_err(CE_WARN, "sam_obj_truncate: Error %d offset %d\n",
		    error, offset);

	return (error);
}

/*
 * sam_obj_append - Append data to OSD Object
 */
/* ARGSUSED */
int
sam_obj_append(struct objnode *objnodep, uint64_t len, void *bufp, int segflg,
	int io_option, uint64_t *size_written, uint64_t *start_appended_addr,
	void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	vnode_t *vp;
	sam_node_t *ip;
	int ioflag = 0;
	struct iovec iov;
	struct uio uio;
	int error = 0;

	vp = ((struct sam_node *)(objnodep->obj_data))->vnode;
	ip = (struct sam_node *)(objnodep->obj_data);
	(void) VOP_RWLOCK(vp, V_WRITELOCK_TRUE, NULL);

	/*
	 * Should do a bunch of validation checks ..
	 */
	iov.iov_base = (caddr_t)bufp;
	iov.iov_len = (size_t)len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = (uio_seg_t)segflg;
	uio.uio_extflg = UIO_COPY_CACHED;
	uio.uio_loffset = 0;
	uio.uio_resid = (ssize_t)len;
	uio.uio_fmode = FWRITE;
	ioflag |= FAPPEND;

	/*
	 * Directio, Page IO or FSYNC
	 */
	if (io_option == OBJECTIO_OPTION_DPO) {
		/*
		 * Set Directio Flag
		 */
		ip = SAM_VTOI(vp);
		ip->flags.bits |= SAM_DIRECTIO;
	} else if (io_option == OBJECTIO_OPTION_FUA)
			ioflag |= FSYNC;

	error = VOP_WRITE(vp, &uio, ioflag, CRED(), NULL);

	/*
	 * Turn off direct io.
	 */
	if (io_option == OBJECTIO_OPTION_DPO)
		ip->flags.bits &= ~SAM_DIRECTIO;

	*start_appended_addr = uio.uio_loffset; /* Append offset set by QFS */
	*size_written = len - uio.uio_resid;
	VOP_RWUNLOCK(vp, V_WRITELOCK_TRUE, NULL);

out:
	if (error)
		cmn_err(CE_WARN, "sam_obj_append: Error %d\n", error);

	return (error);

}

/*
 * sam_obj_punch - Free space in OSD Object
 */
/* ARGSUSED */
int
sam_obj_punch(struct objnode *objnodep, uint64_t starting_addr, uint64_t len,
    void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	/*
	 * Set up to call VOP_SPACE()
	 *
	 * Currently not supported.
	 */

	return (ENOTSUP);

}


/*
 * sam_obj_dummy
 */
/* ARGSUSED */
int
sam_obj_dummy()
{
	return (0);
}

struct objnodeops samfs_objnodeops = {
	sam_obj_busy,
	sam_obj_unbusy,
	sam_obj_rele,
	sam_obj_rw_enter,
	sam_obj_rw_tryenter,
	sam_obj_rw_exit,
	sam_obj_par_create,
	sam_obj_col_create,
	sam_obj_create,
	sam_obj_read,
	sam_obj_write,
	sam_obj_flush,
	sam_obj_remove,
	sam_obj_truncate,
	sam_obj_append,
	sam_obj_setattr,
	sam_obj_getattr,
	sam_obj_getattrpage,
	sam_obj_punch,
	sam_obj_dummy
};

struct objnodeops *samfs_objnodeopsp = &samfs_objnodeops;
