/*
 * ----- vfssubs.c - Process the client externals that are not supported.
 *
 * Process the SAM-QFS client externals that are not supported.
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
 * or https://illumos.org/license/CDDL.
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

#pragma ident "$Revision: 1.14 $"

#include "sam/osversion.h"

/*
 * ----- UNIX Includes
 */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/flock.h>
#include <sys/debug.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ksynch.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/pathname.h>
#include <sys/systm.h>
#include <sys/ddi.h>
#include <sys/cmn_err.h>
#include <sys/file.h>
#include <sys/disp.h>
#include <sys/mount.h>
#include <sys/lockfs.h>

#include <vm/seg_map.h>


/*
 * ----- SAMFS Includes
 */

#include "sam/types.h"
#include "sam/mount.h"

#include "samfs.h"
#include "inode.h"
#include "mount.h"
#include "global.h"
#include "dirent.h"
#include "debug.h"
#include "extern.h"
#include "arfind.h"
#include "trace.h"



/*
 * ----- sam_init_block - initialize the block thread.
 * No block thread in client.
 */

/* ARGSUSED */
int					/* ERRNO if error, 0 if successful. */
sam_init_block(sam_mount_t *mp)
{
	return (0);
}


/*
 * -----	sam_kill_block - kill the block thread.
 * Free block pools.
 */

/* ARGSUSED */
void
sam_kill_block(sam_mount_t *mp)
{
}


/*
 * ----- sam_init_inode - initialize the reclaim thread.
 * No reclaim thread in client.
 */

/* ARGSUSED */
int			/* ERRNO if error, 0 if successful. */
sam_init_inode(sam_mount_t *mp)
{
	return (0);
}


/*
 * ----- sam_arfind_cmd - Process the arfind daemon command.
 * arfind daemon request returns a buffer of file system actions.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_arfind_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	return (0);
}


/*
 * ----	sam_arfind_init - initialize the arfind buffer.
 */

/* ARGSUSED */
void
sam_arfind_init(sam_mount_t *mp)
{
}


/*
 * ----	sam_arfind_fini - remove the arfind buffer.
 */

/* ARGSUSED */
void
sam_arfind_fini(sam_mount_t *mp)
{
}


/*
 * -----	sam_send_to_arfind - send file change event to arfind.
 *	Record the notification entry in the circular buffer.
 */

/* ARGSUSED */
void
sam_send_to_arfind(
	sam_node_t *ip,			/* Pointer to inode */
	enum sam_arfind_event event,	/* The file event */
	int copy)			/* Archive copy */
{
}


/*
 * ----	sam_arfind_umount - tell arfind about umount.
 */

/* ARGSUSED */
void
sam_arfind_umount(sam_mount_t *mp)
{
}


/*
 * ----- sam_amld_init - initialization.
 * This routine initializes the cmd internal structures.
 * It allocates space for the cmd_table and the cmd buffer list.
 * It sets up the cmd buffer free list.
 * It initializes the mutex's and cv's
 */

void
sam_amld_init(void)
{
}


/*
 * ----- sam_amld_fini
 * This routine frees the cmd internal structures.
 * It frees space for the cmd_table and the cmd buffer list.
 */

void
sam_amld_fini(void)
{
}


/*
 * ----- sam_start_releaser - start up the releaser
 * Start up the releaser because the high threshold has been met.
 */

/* ARGSUSED */
void
sam_start_releaser(sam_mount_t *mp)
{
}


/*
 * ----- sam_start_archiver - start up the archiver
 * Start up the archiver because release space for unarchived file.
 */

/* ARGSUSED */
void
sam_start_archiver(sam_mount_t *mp)
{
}


/*
 * ----- sam_quota_init - initialize quotas
 */

/* ARGSUSED */
void
sam_quota_init(sam_mount_t *mp)
{
}

/*
 * ----- sam_quota_fini - finish quotas
 */

/* ARGSUSED */
void
sam_quota_fini(sam_mount_t *mp)
{
}


/*
 * Sweep through the inodes list, reclaim all the sam_quot structures.
 * Then sweep through the sam_quot structures in the hash table, and
 * ensure that all have zero ref counts.
 */

/* ARGSUSED */
int
sam_quota_halt(sam_mount_t *mp)
{
	return (0);
}


/*
 * ----- sam_report_fs_watermark - send fs_watermark_state.
 * Unconditionally report the watermark state to sam-amld
 * via the command interface. Since xmsg_state and xmsg_time
 * fields are manipulated in the mount table, release_lock
 * should be set on entry and exit.
 */

/* ARGSUSED */
void
sam_report_fs_watermark(sam_mount_t *mp, int state)
{
}


/*
 * -----	sam_proc_lockfs -
 * Lock this file system or unlock this file system.
 * Get lockfs status for this file system.
 */

/* ARGSUSED */
int				/* ERRNO if error, 0 if successful. */
sam_proc_lockfs(
	sam_node_t *ip,		/* Pointer to inode. */
	int	cmd,		/* Command number. */
	int	*arg,		/* Pointer to arguments. */
	int flag,		/* Datamodel */
	int	*rvp,		/* Returned value pointer */
	cred_t *credp)		/* Credentials pointer.	*/
{
	return (ENOTSUP);
}
