/*
 * ----- objnode.c - Routines the initializes ithe Object Node specific area
 * of the inode..
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

#pragma ident "$Revision: 1.1 $"

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

#include "pub/objnops.h"

/*
 * sam_objnode_free - Frees the area allocated for Object Node.
 */
void
sam_objnode_free(sam_node_t *ip)
{
	struct objnode *objnode;

	objnode = ip->objnode;
	if (!objnode)
		return;
	mutex_destroy(&objnode->obj_mutex);
	rw_destroy(&objnode->obj_lock);
	if (objnode->obj_membuf) {
		kmem_free(objnode->obj_membuf, OBJECTOPS_MEM_SIM_SIZE);
	}
	kmem_free(objnode, sizeof (struct objnode));
	ip->objnode = NULL;
}

/*
 * sam_objnode_alloc - Creates the Object Node area.
 */
void
sam_objnode_alloc(sam_node_t *ip)
{

	struct objnode *objnode;

	/*
	 * Allocate an Object Node.
	 * We should use kmem cache allocator.
	 */
	objnode = (struct objnode *)
	    kmem_zalloc(sizeof (struct objnode), KM_SLEEP);
	ip->objnode = objnode;
	objnode->obj_data = (void *)ip;
}

/*
 * sam_objnode_init - Initializes the Object Node Area.
 */
void
sam_objnode_init(sam_node_t *ip)
{

	struct objnode *objnode;

	objnode = ip->objnode;
	if (!objnode) {
		cmn_err(CE_WARN,
		    "objnode_init: Objnode area not allocated "
		    "ino 0x%x gen 0x%x\n",
		    ip->di.id.ino, ip->di.id.gen);
		return;
	}
	sam_mutex_init(&objnode->obj_mutex, NULL, MUTEX_DEFAULT, NULL);
	objnode->obj_op = samfs_objnodeopsp;
	rw_init(&objnode->obj_lock, NULL, RW_DEFAULT, NULL);
	objnode->obj_data = (void *)ip;

}
