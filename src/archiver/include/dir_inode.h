/*
 * dir_inode.h - Archiver directory and inode reading function definitions.
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

#ifndef DIR_INODE_H
#define	DIR_INODE_H

#pragma ident "$Revision: 1.6 $"

/* Solaris headers. */
#include <sys/vfs.h>

/* SAM-FS headers. */
#include "sam/fioctl.h"
#include "sam/uioctl.h"
#include "sam/fs/dirent.h"
#include "sam/fs/ino.h"
#include "sam/fs/ino_ext.h"
#include "sam/fs/sblk.h"

/* Macros. */
#define	DIRBUF_SIZE 131072	/* Size of buffer for directory reads */

/* Where to put the current time from the idstat() call. */
#define	TIME_NOW(d) ((d)->residence_time)

/*
 * For opening and reading directories.
 */
struct DirRead {
	sam_ioctl_getdents_t DrGetdentsArgs;
	int	DrFd;
	int	DrCount;
	char	*DrDp;		/* Next directory entry. */
};

/* Functions. */
struct sam_dirent *GetDirent(struct DirRead *dr);
int GetDinode(sam_id_t id, struct sam_disk_inode *pinode);
int GetPinode(sam_id_t id, struct sam_perm_inode *pinode);
int GetSegmentInode(char *filePath, struct sam_perm_inode *pinode,
	struct sam_perm_inode *dsPinode, struct sam_ioctl_idseginfo *ss);
int OpenDir(sam_id_t id, struct sam_disk_inode *dinode, struct DirRead *dr);

#endif /* DIR_INODE_H */
