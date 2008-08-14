/*
 * --	new.c - Create new record functions for SAM-db.
 *
 *	Descripton:
 *	    new.c is a collecton of routines which create
 *	    a new record image give an inode.
 *
 *	Contents:
 *	    sam_db_new_path	- New SAM path table.
 *	    sam_db_new_inode	- New SAM inode table.
 *	    sam_db_new_link	- New SAM link table.
 *	    sam_db_new_archive	- New SAM archive table.
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

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <pub/stat.h>
#include <sam/types.h>
#include <sam/lib.h>
#include <sam/uioctl.h>
#include <sam/fs/ino.h>
#include <sam/fs/dirent.h>
#include <sam/sam_malloc.h>

#include <sam/sam_db.h>

sam_db_inode_t *sam_db_new_inode(
	struct sam_perm_inode *inode)
{
	sam_db_inode_t *t;

	SamMalloc(t, sizeof (sam_db_inode_t));
	memset((char *)t, 0, sizeof (sam_db_inode_t));

	t->ino = inode->di.id.ino;
	t->gen = inode->di.id.gen;
	t->type = sam_db_get_ftype(inode);

	t->size = inode->di.rm.size;
	t->modify_time = inode->di.modify_time.tv_sec;
	t->create_time = inode->di.creation_time;
	t->uid = inode->di.uid;
	t->gid = inode->di.gid;

	return (t);
}

sam_db_path_t *sam_db_new_path(
	struct sam_perm_inode *inode,
	char *path,
	char *obj)
{
	sam_db_path_t *t;

	SamMalloc(t, sizeof (sam_db_path_t));
	memset((char *)t, 0, sizeof (sam_db_path_t));

	t->ino = inode->di.id.ino;
	t->gen = inode->di.id.gen;
	t->type = sam_db_get_ftype(inode);
	SamStrdup(t->path, path);
	SamStrdup(t->obj, obj);
	SamStrdup(t->initial_path, path);
	SamStrdup(t->initial_obj, obj);

	return (t);
}

sam_db_link_t *sam_db_new_link(
	struct sam_perm_inode *inode,
	char *link)
{
	sam_db_link_t *t;

	SamMalloc(t, sizeof (sam_db_link_t));
	memset((char *)t, 0, sizeof (sam_db_link_t));

	t->ino = inode->di.id.ino;
	t->gen = inode->di.id.gen;
	SamStrdup(t->link, link);

	return (t);
}

/*
 * SAM_db_ftype - Return SAM DB File Type.
 *
 *	Description:
 *	    Converts inode file type to database file type.
 *
 *	On Entry:
 *	    inode	Inode image.
 *
 *	Returns:
 *	    Database file type (see SAM_db_ftype in sam_db.h for types).
 */
sam_db_ftype sam_db_get_ftype(
	struct sam_perm_inode *inode /* Perminant inode */
)
{
	sam_db_ftype ftype; /* File type */

	ftype = FTYPE_OTHER;
	if (S_ISREG(inode->di.mode)) {
		ftype = FTYPE_REG;
	}
	if (S_ISDIR(inode->di.mode)) {
		ftype = FTYPE_DIR;
	}
	if (S_ISSEGS(&inode->di)) {
		ftype = FTYPE_SEGI;
	}
	if (S_ISSEGI(&inode->di)) {
		ftype = FTYPE_SEG;
	}
	if (S_ISLNK(inode->di.mode)) {
		ftype = FTYPE_LINK;
	}

	return (ftype);
}
