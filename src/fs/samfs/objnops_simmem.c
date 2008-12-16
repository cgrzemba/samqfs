/*
 * ----- objnops_simmem.c - Memory Based Object Node Operations Sumulation
 * routines.
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
 * sam_obj_remove_simmem
 * Remove the Object of the given handle.
 *
 * This set of code for now assumes it is a User Object
 *
 */
/* ARGSUSED */
int
sam_obj_remove_simmem(struct objnode *objnodep, void *cap, void *sec,
    void *curcmdpg, cred_t *credp)
{

	int error;

	if (objnodep->obj_membuf) {
		kmem_free(objnodep->obj_membuf, OBJECTOPS_MEM_SIM_SIZE);
	}
	error = sam_obj_remove(objnodep, curcmdpg, cap, sec, credp);
	return (error);

}

/*
 * sam_obj_read_simmem - Read data from OSD Object.
 */
/* ARGSUSED */
int
sam_obj_read_simmem(struct objnode *objnodep, uint64_t offset, uint64_t len,
    void *bufp, int segflg, int ioflag, uint64_t *size_read,
    void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	char *membuf;
	uint64_t size_left;
	int flags;
	uint64_t cn;
	int error = 0;

	if (!objnodep->obj_membuf) {
		objnodep->obj_membuf = kmem_alloc(OBJECTOPS_MEM_SIM_SIZE,
		    KM_SLEEP);
	}
	membuf = objnodep->obj_membuf;

	if (segflg == UIO_USERSPACE) {
		flags = 0;
	} else {
		flags = FKIOCTL;
	}

	size_left = len;
	while (size_left) {
		if (size_left >= OBJECTOPS_MEM_SIM_SIZE) {
			cn = OBJECTOPS_MEM_SIM_SIZE;
			size_left = size_left - OBJECTOPS_MEM_SIM_SIZE;
		} else {
			cn = size_left;
			size_left = 0;
		}
		error = ddi_copyout((void *)membuf, bufp, (size_t)cn, flags);
	}

	if (!error) {
		*size_read = len;
	} else {
		cmn_err(CE_WARN, "sam_obj_read_simmem: Read error %d\n", error);
		*size_read = 0;
	}

	return (error);

}

/*
 * sam_obj_write_simmem - Write data to OSD Object
 */
/* ARGSUSED */
int
sam_obj_write_simmem(struct objnode *objnodep, uint64_t offset, uint64_t len,
    void *bufp, int segflg, int ioflag, uint64_t *size_written,
    void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	char *membuf;
	uint64_t size_left;
	int flags;
	uint64_t cn;
	int error = 0;

	if (!objnodep->obj_membuf) {
		objnodep->obj_membuf = kmem_alloc(OBJECTOPS_MEM_SIM_SIZE,
		    KM_SLEEP);
	}
	membuf = objnodep->obj_membuf;

	if (segflg == UIO_USERSPACE) {
		flags = 0;
	} else {
		flags = FKIOCTL;
	}

	size_left = len;
	while (size_left) {
		if (size_left >= OBJECTOPS_MEM_SIM_SIZE) {
			cn = OBJECTOPS_MEM_SIM_SIZE;
			size_left = size_left - OBJECTOPS_MEM_SIM_SIZE;
		} else {
			cn = size_left;
			size_left = 0;
		}
		error = ddi_copyin(bufp, (void *)membuf, (size_t)cn, flags);
	}

	if (!error) {
		*size_written = len;
	} else {
		cmn_err(CE_WARN,
		    "sam_obj_write_simmem: Write error %d\n", error);
		*size_written = 0;
	}

	return (error);
}

/* ARGSUSED */
int
sam_obj_flush_simmem(struct objnode *objnodep, uint64_t offset, uint64_t len,
    uint8_t flush_scope, void *cap, void *sec, void *curcmdpg, cred_t *credp)
{

	return (0);
}

/*
 * sam_obj_truncate_simmem - Truncate the OSD User Object
 *
 */
/* ARGSUSED */
int
sam_obj_truncate_simmem(struct objnode *objnodep, uint64_t offset,
    void *cap, void *sec, void *curcmdpg, cred_t *credp)
{

	return (0);
}

/*
 * sam_obj_append_simmem - Append data to OSD Object
 */
/* ARGSUSED */
int
sam_obj_append_simmem(struct objnode *objnodep, uint64_t len, void *bufp,
    int segflg, int ioflag, uint64_t *size_written,
    uint64_t *start_append_addr, void *cap, void *sec, void *curcmdpg,
    cred_t *credp)
{
	sam_node_t *ip;
	char *membuf;
	uint64_t size_left;
	int flags;
	uint64_t cn;
	int error = 0;

	if (!objnodep->obj_membuf) {
		objnodep->obj_membuf = kmem_alloc(OBJECTOPS_MEM_SIM_SIZE,
		    KM_SLEEP);
	}
	membuf = objnodep->obj_membuf;

	if (segflg == UIO_USERSPACE) {
		flags = 0;
	} else {
		flags = FKIOCTL;
	}

	size_left = len;
	while (size_left) {
		if (size_left >= OBJECTOPS_MEM_SIM_SIZE) {
			cn = OBJECTOPS_MEM_SIM_SIZE;
			size_left = size_left - OBJECTOPS_MEM_SIM_SIZE;
		} else {
			cn = size_left;
			size_left = 0;
		}
		error = ddi_copyin(bufp, (void *)membuf, (size_t)cn, flags);
	}

	ip = (struct sam_node *)(objnodep->obj_data);
	*start_append_addr = ip->di.rm.size;
	if (!error) {
		*size_written = len;
		ip->di.rm.size = ip->di.rm.size + len;
	} else {
		cmn_err(CE_WARN,
		    "sam_obj_append_simmem: Write error %d\n", error);
		*size_written = 0;
	}

	return (error);
}

/* ARGSUSED */
int
sam_obj_punch_simmem(struct objnode *objnodep, uint64_t starting_addr,
    uint64_t len, void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	return (0);
}

struct objnodeops samfs_objnodeops_simmem = {
	sam_obj_busy,
	sam_obj_unbusy,
	sam_obj_rele,
	sam_obj_rw_enter,
	sam_obj_rw_tryenter,
	sam_obj_rw_exit,
	sam_obj_par_create,
	sam_obj_col_create,
	sam_obj_create,
	sam_obj_read_simmem,
	sam_obj_write_simmem,
	sam_obj_flush_simmem,
	sam_obj_remove_simmem,
	sam_obj_truncate_simmem,
	sam_obj_append_simmem,
	sam_obj_setattr,
	sam_obj_getattr,
	sam_obj_getattrpage,
	sam_obj_punch_simmem,
	sam_obj_dummy
};

struct objnodeops *samfs_objnodeops_simmemp = &samfs_objnodeops_simmem;
