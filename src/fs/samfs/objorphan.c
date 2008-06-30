/*
 * ---- objorphan.c - Object Orphan list management.
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
#include "objorphan.h"

/*
 * sam_obj_orphan_init - Initializes the Ophan list.
 */
/* ARGSUSED */
int
sam_obj_orphan_init(sam_mount_t *mp, void **handle)
{

	struct obj_orphan_list_hdr *oolhdr;
	uint64_t *orphanlist;
	uint64_t orphanlist_size;
	sam_node_t *ip = NULL;
	uint64_t id;
	uint64_t size_read;
	objnode_t *objnodep;
	uint64_t *tmplist;
	int error = 0;
	int i;

	/*
	 * Allocate the header structure for the Orphan List.
	 */
	oolhdr = kmem_zalloc(sizeof (struct obj_orphan_list_hdr), KM_SLEEP);
	if (!oolhdr) {
		cmn_err(CE_WARN,
		    "sam_obj_orphan_init:  Unable to allocate oolhdr\nn");
		return (ENOMEM);
	}

	/*
	 * The Orphan List file always exist.
	 */
	id = SAM_OBJ_ORPHANS_ID;
	error = sam_get_ino(mp->mi.m_vfsp, IG_EXISTS, (sam_id_t *)&id, &ip);
	if (!ip || error) {
		cmn_err(CE_WARN, "sam_obj_orphan_init: Id get 0x%llx "
		    "error %d\n",
		    id, error);
		return (error);
	}

	/*
	 * Read the header into memory.
	 */

	objnodep = ip->objnode;
	if (ip->di.rm.size) {
		error = OBJNODE_READ(objnodep,	0,
		    sizeof (struct obj_orphan_list_hdr), (char *)oolhdr,
		    UIO_SYSSPACE, 0, &size_read, NULL, NULL, NULL, CRED());
		if (error) {
			cmn_err(CE_WARN, "sam_obj_orphan_init: "
			    "Read hdr error %d\n",
			    error);
			VN_RELE(SAM_ITOV(ip));
			return (0);
		}
	}

	/*
	 * This covers the cases:
	 *	1.  The file is empty.
	 *	2.  The Header is not initialized.
	 *	3.  The Header has an empty list.
	 */
	cmn_err(CE_WARN, "sam_obj_orphan_init: Version 0x%llx count  0x%llx\n",
	    oolhdr->ool_version, oolhdr->ool_list_count);

	if ((oolhdr->ool_version == 0) || (oolhdr->ool_list_count == 0)) {
		/*
		 * Empty file or list .. Initialize incore header.
		 */
		oolhdr->ool_list_count = DEFAULT_OSDT_ORPHANS;
		orphanlist_size = ((sizeof (uint64_t)) * DEFAULT_OSDT_ORPHANS);
		orphanlist = (uint64_t *)kmem_zalloc(orphanlist_size, KM_SLEEP);
		if (!orphanlist) {
			cmn_err(CE_WARN, "sam_obj_orphan_init: Failed - "
			    "ENOMEM\n");
			VN_RELE(SAM_ITOV(ip));
			return (ENOMEM);
		}
	} else {
		if (oolhdr->ool_list_count < DEFAULT_OSDT_ORPHANS) {
			/*
			 * If list in File is smaller than default - we always
			 * allocate the default size in memory.
			 */
			orphanlist_size = DEFAULT_OSDT_ORPHANS
			    * sizeof (uint64_t);
		} else {
			/*
			 * The list has grown beyond the default size.
			 */
			orphanlist_size = oolhdr->ool_list_count
			    * sizeof (uint64_t);
		}

		orphanlist = (uint64_t *)kmem_zalloc(orphanlist_size,
		    KM_SLEEP);
		if (!orphanlist) {
			cmn_err(CE_WARN, "sam_obj_orphan_init: Failed - "
			    "ENOMEM\n");
			VN_RELE(SAM_ITOV(ip));
			return (ENOMEM);
		}

		/*
		 * Read actual len of the list.
		 */
		error = OBJNODE_READ(objnodep,
		    sizeof (struct obj_orphan_list_hdr) /* Offset from hdr */,
		    oolhdr->ool_list_count * sizeof (uint64_t) /* List size */,
		    (char *)orphanlist, UIO_SYSSPACE, 0, &size_read,
		    NULL, NULL, NULL, CRED());
		if (error) {
			cmn_err(CE_WARN,
			    "sam_obj_orphan_init: Read hdr error %d\n", error);
			VN_RELE(SAM_ITOV(ip));
			return (error);
		}
		if (oolhdr->ool_list_count < DEFAULT_OSDT_ORPHANS) {
			/* Adjust incore list count */
			oolhdr->ool_list_count = DEFAULT_OSDT_ORPHANS;
		}
	}

	/*
	 * Truncate the file - if we panic before we can update the file,
	 * samfsck will collect and build the Orphan list.
	 * The file gets updated duirng LUN detach time.
	 */
	RW_LOCK_OS(&ip->data_rwl, RW_WRITER);
	RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
	error = sam_truncate_ino(ip, 0, SAM_TRUNCATE, CRED());
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	RW_UNLOCK_OS(&ip->data_rwl, RW_WRITER);
	if (error) {
		cmn_err(CE_WARN,
		    "sam_obj_orphan_init: Truncate error %d\n", error);
		VN_RELE(SAM_ITOV(ip));
		return (error);
	}

	/*
	 * Sanity check that the list is good:
	 *	1.  Rebuild the free list - ool_freelist ignored.
	 *	2.  Verify Orphan is valid.  If not remove from list.
	 *
	 * Note that when file is closed, the list is compacted before
	 * writing out to file.  Process from end of table.
	 */
	tmplist = orphanlist + (oolhdr->ool_list_count - 1); /* Last Entry */
	oolhdr->ool_objp = objnodep;
	oolhdr->ool_freelist = oolhdr->ool_list_count; /* Invalid entry */
	oolhdr->ool_active_count = 0;
	oolhdr->ool_version = ORPHAN_LIST_VERSION;
	oolhdr->ool_listp = orphanlist;
	for (i = oolhdr->ool_list_count; i > 0; i--, tmplist--) {
		if (*tmplist > 0xffffffff) { /* Valid sam_id */
			oolhdr->ool_active_count++;
			continue;
		}
		*tmplist = oolhdr->ool_freelist;
		oolhdr->ool_freelist = i;
	}

	mutex_init(&(oolhdr->ool_kmutex), NULL, MUTEX_DEFAULT, NULL);
	*handle = (void *)oolhdr;
	return (0);
}

/*
 * sam_obj_orphan_close - Flush and close the Ophan list object.
 */
/* ARGSUSED */
int
sam_obj_orphan_close(void *handle)
{

	struct obj_orphan_list_hdr *oolhdr;
	objnode_t *obj;
	uint64_t *templist, *compact_list;
	uint64_t active_count;
	int error;
	uint64_t size_writtern;
	int i;

	oolhdr = (struct obj_orphan_list_hdr *)handle;
	obj = oolhdr->ool_objp;
	if (oolhdr->ool_active_count == 0) {
		/* No entries - Free list. */
		if (oolhdr->ool_listp)
			kmem_free(oolhdr->ool_listp,
			    oolhdr->ool_list_count * sizeof (uint64_t));
		mutex_destroy(&oolhdr->ool_kmutex);
		bzero(oolhdr, sizeof (struct obj_orphan_list_hdr));
		/* Free Header */
		kmem_free(oolhdr, sizeof (struct obj_orphan_list_hdr));
		error = OBJNODE_RELE(obj);
		return (0);
	}

	/*
	 * Compact the list.
	 */
	templist = oolhdr->ool_listp;
	compact_list = oolhdr->ool_listp;
	active_count = 0;
	for (i = 0; i < oolhdr->ool_list_count; templist++) {
		if (*templist < 0xffffffff) {
			/* A valid sam_id is always greater that 32 bits */
			continue;
		} else {
			/* Valid orphan uoid */
			*compact_list = *templist;
			compact_list++;
			active_count++;
		}
	}

	if (active_count != oolhdr->ool_active_count) {
		cmn_err(CE_WARN,
		    "sam_obj_orphan_close: active_count 0x%llx 0x%llx\n",
		    active_count, oolhdr->ool_active_count);
		/* There is a bug .. have to continue */
		oolhdr->ool_active_count = active_count;
		oolhdr->ool_list_count = active_count;
	}

	/*
	 * Flush the header and also the list to file.
	 * File has previously been truncated ..
	 */
	error = OBJNODE_WRITE(obj,  0, sizeof (struct obj_orphan_list_hdr),
	    oolhdr, UIO_SYSSPACE, 0, &size_writtern, NULL, NULL, NULL, CRED());
	if (error) {
		cmn_err(CE_WARN, "sam_obj_orphan_close write error %d\n",
		    error);
	}
	error = OBJNODE_WRITE(obj,  sizeof (struct obj_orphan_list_hdr),
	    oolhdr->ool_active_count * sizeof (uint64_t),
	    oolhdr->ool_listp, UIO_SYSSPACE, 0, &size_writtern,
	    NULL, NULL, NULL, CRED());

	kmem_free(oolhdr->ool_listp,
	    oolhdr->ool_list_count * sizeof (uint64_t));
	mutex_destroy(&(oolhdr->ool_kmutex));
	bzero(oolhdr, sizeof (struct obj_orphan_list_hdr));
	kmem_free(oolhdr, sizeof (struct obj_orphan_list_hdr));
	error = OBJNODE_RELE(obj);
	return (error);

}

/*
 * sam_obj_find_ophan_entry - Locate the object in the orphan list.
 */
/* ARGSUSED */
int
sam_obj_find_ophan_entry(struct obj_orphan_list_hdr *oolhdr, uint64_t uoid,
    uint64_t *index)
{

	uint64_t *orphanlist = NULL;
	uint64_t count;
	int i;

	orphanlist = oolhdr->ool_listp;
	count = oolhdr->ool_list_count;
	for (i = 0; i < count; orphanlist++) {
		if (*orphanlist == uoid) {
			*index = i;
			return (0);
		}
	}

	return (ENOENT);
}

/*
 * sam_obj_orphan_insert - Insert an object into the orphan list.
 */
/* ARGSUSED */
int
sam_obj_orphan_insert(void *handle, uint64_t created_uoid)
{

	struct obj_orphan_list_hdr *oolhdr;
	uint64_t *free;

	oolhdr = (struct obj_orphan_list_hdr *)handle;
	mutex_enter(&(oolhdr->ool_kmutex));
	if (oolhdr->ool_freelist == oolhdr->ool_list_count) {
		/*
		 * This should never happen .. no free space
		 * Drop the entry - need to sync with MDS
		 */
		cmn_err(CE_WARN, "sam_obj_orphan_insert: List is full?????\n");
		mutex_exit(&(oolhdr->ool_kmutex));
		return (-1);
	}

	free = oolhdr->ool_listp + oolhdr->ool_freelist;
	oolhdr->ool_freelist = *(oolhdr->ool_listp + *free);
	*free = created_uoid;
	mutex_exit(&(oolhdr->ool_kmutex));

	return (0);
}

/*
 * sam_obj_orphan_remove - Remove an object from the orphan list.
 */
/* ARGSUSED */
void
sam_obj_orphan_remove(void *handle, uint64_t uoid)
{

	struct obj_orphan_list_hdr *oolhdr;
	uint64_t entry;
	uint64_t *free;
	int error;

	oolhdr = (struct obj_orphan_list_hdr *)handle;
	mutex_enter(&(oolhdr->ool_kmutex));
	error = sam_obj_find_ophan_entry(oolhdr, uoid, &entry);
	if (error) {
		cmn_err(CE_WARN,
		    "sam_obj_orphan_remove:  Cannot find entry 0x%llx\n",
		    uoid);
		mutex_exit(&(oolhdr->ool_kmutex));
		return;
	}

	free = oolhdr->ool_listp + entry;
	*free = oolhdr->ool_freelist;
	oolhdr->ool_freelist = *free;
	mutex_exit(&(oolhdr->ool_kmutex));

}
