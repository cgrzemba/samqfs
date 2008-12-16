/*
 * ---- objnops_simops.c - No-op Object object node simulation routines.
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

/*
 * This is the set of qfs routines that supports the T10 OSD Commands.
 * Currently, these routines are called from the T10 OSD Server Driver
 * from User Land via samfs uioctl().  In the future, when we integrate
 * with Comstar, these routines will be called directly from the kernel.
 *
 * Note that we do not do any copyin or copyout in these routines.  It is all
 * done by the receiving uioctl() code.
 *
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
 * sam_obj_read_simiops - Read data from OSD Object.
 */
/* ARGSUSED */
int
sam_obj_read_simiops(struct objnode *objnodep, uint64_t offset, uint64_t len,
    void *bufp, int segflg, int ioflag, uint64_t *size_read,
    void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	*size_read = len;
	return (0);
}

/*
 * sam_obj_write_simiops - Write data to OSD Object
 */
/* ARGSUSED */
int
sam_obj_write_simiops(struct objnode *objnodep, uint64_t offset, uint64_t len,
    void *bufp, int segflg, int ioflag, uint64_t *size_written,
    void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	*size_written = len;
	return (0);
}

/* ARGSUSED */
int
sam_obj_flush_simiops(struct objnode *objnodep, uint64_t offset, uint64_t len,
    uint8_t flush_scope, void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	return (0);
}

/*
 * sam_obj_truncate_simiops - Truncate the OSD User Object
 *
 */
/* ARGSUSED */
int
sam_obj_truncate_simiops(struct objnode *objnodep, uint64_t offset,
    void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	return (0);
}

/*
 * sam_obj_append_simiops - Append data to OSD Object
 */
/* ARGSUSED */
int
sam_obj_append_simiops(struct objnode *objnodep, uint64_t len, void *bufp,
    int segflg, int ioflag, uint64_t *size_written,
    uint64_t *start_append_addr, void *cap, void *sec, void *curcmdpg,
    cred_t *credp)
{

	*size_written = len;

	return (0);
}

/* ARGSUSED */
int
sam_obj_punch_simiops(struct objnode *objnodep, uint64_t starting_addr,
    uint64_t len, void *cap, void *sec, void *curcmdpg, cred_t *credp)
{
	return (0);
}

struct objnodeops samfs_objnodeops_simiops = {
	sam_obj_busy,
	sam_obj_unbusy,
	sam_obj_rele,
	sam_obj_rw_enter,
	sam_obj_rw_tryenter,
	sam_obj_rw_exit,
	sam_obj_par_create,
	sam_obj_col_create,
	sam_obj_create,
	sam_obj_read_simiops,
	sam_obj_write_simiops,
	sam_obj_flush_simiops,
	sam_obj_remove,
	sam_obj_truncate_simiops,
	sam_obj_append_simiops,
	sam_obj_setattr,
	sam_obj_getattr,
	sam_obj_getattrpage,
	sam_obj_punch,
	sam_obj_dummy
};

struct objnodeops *samfs_objnodeops_simiopsp = &samfs_objnodeops_simiops;
