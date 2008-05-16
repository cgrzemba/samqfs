/*
 * ----- ialloc.c - Process the alloc/free inode functions.
 *
 * Process the SAM-QFS allocate/free inode functions.
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

#pragma ident "$Revision: 1.56 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/t_lock.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/cred.h>
#include <sys/vnode.h>
#include <sys/buf.h>

/* ----- SAMFS Includes */

#include "sam/types.h"

#include "macros.h"
#include "inode.h"
#include "mount.h"
#include "ino_ext.h"
#include "extern.h"
#include "debug.h"
#include "trace.h"
#include "qfs_log.h"


/*
 * ----- sam_alloc_inode - Process get free inode.
 *
 * Get free inode. If no free_ino set, allocate at the end of the .inodes
 * file. Hold the .inode inode rwlock, cannot create an inode when
 * releasing an inode.
 */

int				/* ERRNO if error, 0 if successful. */
sam_alloc_inode(
	sam_mount_t *mp,	/* The mount table pointer. */
	mode_t mode,		/* The mode of the new file */
	sam_ino_t *ino)		/* The inode number (returned). */
{
	ino_t inumber;
	struct sam_perm_inode *permip;
	ino_t max_inum;
	struct sam_node *dip = mp->mi.m_inodir;
	buf_t *bp = NULL;
	boolean_t found_inode = B_TRUE;
	boolean_t need_fsck = B_FALSE;
	int error = 0;

	mutex_enter(&mp->ms.m_inode_mutex);

	inumber = dip->di.free_ino;
	max_inum = ((dip->di.rm.size + (dip->di.status.b.direct_map ?
	    0 : mp->mt.fi_rd_ino_buf_size)) >> SAM_ISHIFT);

	/* Get free list head i-node. */
	if ((inumber < mp->mi.m_min_usr_inum) ||
	    (inumber > max_inum) ||
	    (sam_read_ino(mp, inumber, &bp, &permip) != 0) ||
	    (permip->di.mode != 0)) {
		found_inode = B_FALSE;
	}

	for (;;) {
		if (found_inode) {
			*ino = inumber;
			permip->di.id.ino = inumber;
			permip->di.mode = mode;

			/*
			 * If this i-node points to another free one, then make
			 * that one the new head.  For pre-alloc'ed i-nodes,
			 * it's acceptible to terminate the (now empty) list
			 * with 0.  For non-pre-alloc'ed i-nodes, a 0 (empty
			 * free list) forces the free list head to refer to the
			 * next sequential i-node number which, when it exceeds
			 * the current end of .inodes, will force sam_read_ino()
			 * to auto-extend .inodes.
			 */
			if (permip->di.free_ino ||
			    dip->di.status.b.direct_map) {
				dip->di.free_ino = permip->di.free_ino;
				permip->di.free_ino = 0;
				/*
				 * The last i-node in legacy pre-allocated
				 * freelists can point one i-node beyond the
				 * end of the i-node space.  Force a proper
				 * freelist termination here.
				 */
				if (dip->di.status.b.direct_map) {
					if (dip->di.free_ino == (max_inum+1)) {
						dip->di.free_ino = 0;
					}
				}
			} else {
				dip->di.free_ino = ++inumber;
			}
			dip->flags.b.changed = 1;

			bdwrite(bp);

			if ((SAM_SYNC_META(mp)) || (TRANS_ISTRANS(mp))) {
				(void) sam_update_inode(dip, SAM_SYNC_ONE,
				    FALSE);
			}

			TRACE(T_SAM_INO_ALLOC, SAM_ITOV(dip), *ino,
			    dip->di.free_ino,
			    inumber);
			/*
			 * We found and are returning an available i-node
			 * number.  But, if we found a bad i-node during the
			 * search, then go mark the file system to require an
			 * fsck before returning success.  Return common success
			 * case here for better performance.
			 */
			if (!need_fsck) {
				mutex_exit(&mp->ms.m_inode_mutex);
				return (error);		/* Success */
			}

			break;
		} else {
			ino_t first_user_inum = mp->mi.m_min_usr_inum;
			boolean_t no_prealloc_inodes =
			    (dip->di.status.b.direct_map == 0);
			int inodes_per_buf = (mp->mt.fi_rd_ino_buf_size >>
			    SAM_ISHIFT);

			if (no_prealloc_inodes) {
				if (max_inum < first_user_inum) {
					/*
					 * .inodes is too small to accomodate
					 * user i-nodes (i.e.  this is the first
					 * i-node allocation on this new file
					 * system).  Grow .inodes one buffer
					 * at-a-time until it is large enough.
					 * This check is placed here to minimize
					 * overhead in the success code path.
					 */
					while (max_inum < first_user_inum) {
						if ((error = sam_read_ino(mp,
						    max_inum, &bp,
						    &permip)) != 0) {
							dcmn_err((CE_WARN,
							    "SAM-QFS: %s: "
							    "Can't extend "
							    "i-node free "
							    "list to inode "
							    "# %ld",
							    mp->mt.fi_name,
							    max_inum));
							break;
						}
						brelse(bp);
						max_inum += inodes_per_buf;
					}

					if (error) {
						need_fsck = B_TRUE;
						/*
						 * Couldn't extend .inodes -
						 * return error.
						 */
						break;
					}

					/*
					 * Check if the first user i-node is
					 * available.
					 */
					inumber = first_user_inum;
					if ((sam_read_ino(mp, inumber, &bp,
					    &permip) == 0) &&
					    (permip->di.mode == 0)) {
						/* Go use this i-node. */
						found_inode = B_TRUE;
						continue;
					}
				}
			} else {
				/* Prealloc'ed i-nodes */

				if ((inumber == 0) ||
				    (inumber == (max_inum+1))) {
					/*
					 * I-node number indicates that the free
					 * list is exhausted (which is okay).
					 * Log this condition.  Don't mark the
					 * file system for fsck, but return
					 * ENOSPC to indicate that no i-nodes
					 * are available.
					 */

					dcmn_err((CE_WARN,
					    "SAM-QFS: %s: Pre-allocated "
					    "i-node free list is exhausted",
					    mp->mt.fi_name));

					error = ENOSPC;
					break;
				}
			}

			/*
			 * Either the i-node number is out-of-range, the i-node
			 * couldn't be read or it was already allocated.  Mark
			 * the file system for fsck, and log the specific
			 * problem.
			 */
			if ((inumber >= first_user_inum) &&
			    (inumber <= max_inum)) {
				/* I-node number is in-range */

				if (bp != NULL) {
					/*
					 * We successfully read the i-node, so
					 * it must have been allocated.  Don't
					 * cause alarm, because we can hit this
					 * condition legitimately.  Just go
					 * pick-up at the end of .inodes.
					 */
					brelse(bp);
					bp = NULL;
				}
				/*
				 * If the i-node was un-readable, it was
				 * likely for one of many possible reasons
				 * that neither we nor the customer can
				 * do anything about.  There's no reason to
				 * cause alarm.  Just go pick-up at the end
				 * of .inodes.
				 */
			} else {
				need_fsck = B_TRUE;
				dcmn_err((CE_WARN,
				    "SAM-QFS: %s: I-node free list "
				    "references out-of-range i-node # %ld",
				    mp->mt.fi_name, inumber));
			}

			/*
			 * Try a sequential search, starting one buffer size
			 * before the current end of .inodes.  For pre-alloc'ed
			 * i-nodes, search only up to the current end of
			 * .inodes.  Otherwise, search up to one buffer size
			 * beyond the end of .inodes (where sam_read_ino() will
			 * perform an auto-extend).  This approach is preserved
			 * here for posterity.
			 */
			inumber = max_inum - (inodes_per_buf *
			    (no_prealloc_inodes ? 2 : 1));
			if (inumber < first_user_inum) {
				inumber = first_user_inum;
			}

			while (inumber <= max_inum) {
				if (sam_read_ino(mp, inumber, &bp,
				    &permip) == 0) {
					if (permip->di.mode != 0) {
						/* Skip allocated i-nodes. */
						brelse(bp);
					} else {
						found_inode = B_TRUE;
						break;
					}
				}
				inumber++;
			}

			if (found_inode) {
				continue;	/* Go use this i-node. */
			}

			dcmn_err((CE_WARN,
			    "SAM-QFS: %s: Can't find free i-node",
			    mp->mt.fi_name));

			error = ENOSPC;
			break;
		}
	}

	mutex_exit(&mp->ms.m_inode_mutex);

	if (need_fsck) {
		sam_req_fsck(mp, -1,
		    "SAM-QFS: Found bad i-node(s) in free list");
	}

	return (error);
}

/*
 * ---- sam_free_inode - Process free inode.
 *
 * Link the inode in the free list. Forward pointer is in the .inodes file.
 * NOTE: .inode inode rwlock set on entry and exit.
 */

void
sam_free_inode(
	sam_mount_t *mp,		/* The mount table pointer. */
	struct sam_disk_inode *dp)	/* The disk inode table pointer. */
{
	sam_ino_t ino = dp->id.ino;

	TRACE(T_SAM_INO_FREE, NULL, ino,
	    dp->id.gen, mp->mi.m_inodir->di.free_ino);
	ASSERT(MUTEX_HELD(&mp->ms.m_inode_mutex));

	if (ino >= mp->mi.m_min_usr_inum) {
		dp->free_ino =  mp->mi.m_inodir->di.free_ino;
		mp->mi.m_inodir->di.free_ino = ino;
		mp->mi.m_inodir->flags.b.changed = 1;
	}
}


/*
 * ----- sam_alloc_inode_ext - allocate a list of inode extensions.
 *
 *	Allocate and initialize a list of inode extensions for the base inode.
 *	Link the newly allocated list at head of any existing list.
 */

int					/* ERRNO if error, 0 if successful. */
sam_alloc_inode_ext(
	sam_node_t *bip,		/* Pointer to base inode. */
	mode_t mode,			/* Mode for inode extension. */
	int count,			/* Number of inode extensions to */
					/*   allocate/link. */
	sam_id_t *fid)			/* out: first inode extension id */
{
	int	error;
	struct buf *bp;
	sam_id_t first_id, eid;
	struct sam_inode_ext *eip;

	first_id.ino = first_id.gen = 0;

	while (count > 0) {

		/* Allocate and read inode extension */
		if ((error = sam_alloc_inode(bip->mp, mode, &eid.ino))) {
			if (first_id.ino) {
				sam_free_inode_ext(bip, mode,
				    SAM_ALL_COPIES, &first_id);
			}
			return (error);
		}
		if (error = sam_read_ino(bip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip)) {
			if (first_id.ino) {
				sam_free_inode_ext(bip, mode,
						SAM_ALL_COPIES, &first_id);
			}
			return (error);
		}

		/*
		 * Initialize the new inode extension.
		 */
		eid.gen = eip->hdr.id.gen;
		if (++eid.gen == 0) {
			eid.gen++;
		}
		bzero((char *)eip, sizeof (*eip));
		eip->hdr.mode = (mode | (count & S_IFORD));
		/* Use version of base inode */
		eip->hdr.version = bip->di.version;
		eip->hdr.id = eid;
		eip->hdr.file_id = bip->di.id;
		if (first_id.ino == 0) {
			eip->hdr.next_id = *fid;
		} else {
			eip->hdr.next_id = first_id;
		}

		first_id = eid;			/* new first extension id */
		TRACE(T_SAM_INO_EXT_ALLOC, SAM_ITOV(bip), (int)bip->di.id.ino,
		    (int)eip->hdr.id.ino, mode);
		bdwrite(bp);
		count--;
	}
	*fid = first_id;			/* return success */
	return (0);
}


/*
 * ----- sam_free_inode_ext - free inode extensions.
 *
 * Free inode extension list for base inode.  Any list members with
 * different mode values will remain linked to base inode at completion.
 */

void
sam_free_inode_ext(
	sam_node_t *bip,	/* Pointer to base inode. */
	mode_t mode,		/* Mode of inode extension(s) to free. */
	int copy,		/* Copy number of exts to free for */
				/*   mode == S_IFMVA. */
	sam_id_t *fid)		/* out: first inode extension id. */
{
	int remove;
	buf_t *bp;
	struct sam_inode_ext *eip;
	sam_id_t eid, first_id, last_id, next_id;

	first_id.ino = first_id.gen = 0;
	last_id.ino = last_id.gen = 0;

	eid = *fid;
	while (eid.ino) {
		mutex_enter(&bip->mp->ms.m_inode_mutex);
		if (sam_read_ino(bip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip)) {
			mutex_exit(&bip->mp->ms.m_inode_mutex);
			return;
		}
		if (EXT_HDR_ERR(eip, eid, bip)) {	/* Validate header */
			mutex_exit(&bip->mp->ms.m_inode_mutex);
			brelse(bp);
			return;
		}
		next_id = eip->hdr.next_id;

		/*
		 * Remove all inode extensions with mode from the list,
		 * except when a specific copy number for multivolume
		 * archive inode extensions has been requested.
		 */
		remove = 0;
		if ((eip->hdr.mode & S_IFEXT) == mode) {
			if ((mode == S_IFMVA) && (copy >= 0)) {
				/* Specific copy */
				if ((eip->hdr.version >= SAM_INODE_VERS_2) &&
				    (eip->ext.mva.copy == copy)) {
					remove++;
				} else if ((eip->hdr.version ==
				    SAM_INODE_VERS_1) &&
				    (eip->ext.mv1.copy == copy)) {
					remove++;
				}
			} else {
				remove++;
			}
		}
		if (remove) {			/* Take out of list */
			TRACE(T_SAM_INO_EXT_FREE, SAM_ITOV(bip),
			    (int)bip->di.id.ino,
			    (int)eip->hdr.id.ino, eip->hdr.mode);
			bzero((char *)eip, sizeof (union sam_ino_ext));
			eip->hdr.id = eid;
			sam_free_inode(bip->mp, (struct sam_disk_inode *)eip);
			mutex_exit(&bip->mp->ms.m_inode_mutex);
			if (TRANS_ISTRANS(bip->mp)) {
				sam_ioblk_t ioblk;
				int error;

				RW_LOCK_OS(&bip->mp->mi.m_inodir->inode_rwl,
				    RW_READER);
				error = sam_map_block(bip->mp->mi.m_inodir,
				    (offset_t)SAM_ITOD(eid.ino), SAM_ISIZE,
				    SAM_READ, &ioblk, CRED());
				RW_UNLOCK_OS(&bip->mp->mi.m_inodir->inode_rwl,
				    RW_READER);
				if (!error) {
					offset_t doff;

					doff = ldbtob(fsbtodb(bip->mp,
					    ioblk.blkno)) + ioblk.pboff;
					TRANS_EXT_INODE(bip->mp, eid,
					    doff, ioblk.ord);
				} else {
					error = 0;
				}
				brelse(bp);
			} else {
				bdwrite(bp);
			}

		} else {				/* Keep in list */
			mutex_exit(&bip->mp->ms.m_inode_mutex);
			brelse(bp);
			if (first_id.ino == 0) first_id = eid;
			if (last_id.ino != 0) {		/* Link to last */
				if (sam_read_ino(bip->mp, last_id.ino, &bp,
					(struct sam_perm_inode **)&eip)) {
					return;
				}
				eip->hdr.next_id = eid;
				if (TRANS_ISTRANS(bip->mp)) {
					sam_ioblk_t ioblk;
					int error;

					RW_LOCK_OS(
					    &bip->mp->mi.m_inodir->inode_rwl,
					    RW_READER);
					error = sam_map_block(
					    bip->mp->mi.m_inodir,
					    (offset_t)SAM_ITOD(eid.ino),
					    SAM_ISIZE, SAM_READ, &ioblk,
					    CRED());
					RW_UNLOCK_OS(
					    &bip->mp->mi.m_inodir->inode_rwl,
					    RW_READER);
					if (!error) {
						offset_t doff;

						doff = ldbtob(fsbtodb(bip->mp,
						    ioblk.blkno)) + ioblk.pboff;
						TRANS_EXT_INODE(bip->mp,
						    eid, doff, ioblk.ord);
					} else {
						error = 0;
					}
					brelse(bp);
				} else {
					bdwrite(bp);
				}
			}
			last_id = eid;				/* New last */
		}
		eid = next_id;
	}

	/* Make sure list is terminated correctly */
	if (last_id.ino != 0) {
		if (sam_read_ino(bip->mp, last_id.ino, &bp,
				(struct sam_perm_inode **)&eip)) {
			return;
		}
		eip->hdr.next_id.ino = eip->hdr.next_id.gen = 0;
		if (TRANS_ISTRANS(bip->mp)) {
			sam_ioblk_t ioblk;
			int error;

			RW_LOCK_OS(&bip->mp->mi.m_inodir->inode_rwl, RW_READER);
			error = sam_map_block(bip->mp->mi.m_inodir,
			    (offset_t)SAM_ITOD(eid.ino), SAM_ISIZE,
			    SAM_READ, &ioblk, CRED());
			RW_UNLOCK_OS(&bip->mp->mi.m_inodir->inode_rwl,
			    RW_READER);
			if (!error) {
				offset_t doff;

				doff = ldbtob(fsbtodb(bip->mp,
				    ioblk.blkno)) + ioblk.pboff;
				TRANS_EXT_INODE(bip->mp, last_id, doff,
				    ioblk.ord);
			} else {
				error = 0;
			}
			brelse(bp);
		} else {
			bdwrite(bp);
		}
	}

	/* Return head of updated list */
	*fid = first_id;
}


/*
 * ----- sam_fix_mva_inode_ext - delete a faulty multivolume inode.
 *
 *	In 4.0, a logic error would allow a multivolume inode to be created
 *  when only one vsn existed. This routine attempts to remove that
 *  multivolume inode. If this routine fails for any reason, the calling
 *  routine has already suppressed the "bad" mva from reaching the user,
 *  so ignoring the error until next time causes the situation to be no
 *  worse than it was on entry.
 */

void
sam_fix_mva_inode_ext(
	sam_node_t *bip,		/* Pointer to base inode. */
	int copy,			/* Copy number, 0..(MAX_ARCHIVE-1) */
	sam_id_t *ret_eidp)		/* Returned extension id */
{
	int		error;
	buf_t	*bp;
	sam_id_t eid, next_eid;
	sam_ino_t last_eid;
	struct sam_inode_ext *eip;

	eid = bip->di.ext_id;
	last_eid = 0;
	mutex_enter(&bip->mp->ms.m_inode_mutex);
	while (eid.ino) {
		bp = NULL;
		if (sam_read_ino(bip->mp, eid.ino, &bp,
				(struct sam_perm_inode **)&eip) != 0) {
			break;
		}

		/*
		 * Since this was just checked in the caller, this shouldn't
		 * happen. But, check it anyway.
		 */
		if (EXT_HDR_ERR(eip, eid, bip)) {
			error = ENOCSI;
			break;
		}
		if (S_ISMVA(eip->hdr.mode)) {		/* is a multi-volume */
			if (copy == eip->ext.mva.copy) { /* is offending copy */
				next_eid = eip->hdr.next_id;
				brelse(bp);
				/*
				 * Link past the faulty multivolume extension.
				 */
				if (last_eid == 0) { /* Change in base inode */
					bip->di.ext_id = next_eid;
					bip->flags.bits |= SAM_CHANGED;
					sam_update_inode(bip, SAM_SYNC_ONE,
					    FALSE);
				} else {	/* Change in last extension */
					if (sam_read_ino(bip->mp, last_eid, &bp,
					    (struct sam_perm_inode **)&eip) !=
					    0) {
						eip->hdr.next_id = next_eid;
						bdwrite(bp); /* write it back */
					}
				}
				*ret_eidp = next_eid;
				/*
				 * Now free the multivolume extension.
				 */
				if (sam_read_ino(bip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip) == 0) {
					if (EXT_HDR_ERR(eip, eid, bip) == 0) {
						TRACE(T_SAM_INO_EXT_FREE,
							SAM_ITOV(bip),
							(int)bip->di.id.ino,
							(int)eip->hdr.id.ino,
							eip->hdr.mode);
						bzero((char *)eip,
						    sizeof (union sam_ino_ext));
						eip->hdr.id = eid;
						sam_free_inode(bip->mp,
						    (struct sam_disk_inode *)
						    eip);
						bdwrite(bp);
					} else {
						brelse(bp);
					}
				}
				mutex_exit(&bip->mp->ms.m_inode_mutex);
				return;
			}
		}
		eid = eip->hdr.next_id;
		brelse(bp);
	}
	/*
	 * This should not fail, but if it did, it is like ignoring
	 * it when locked. Just move on.
	 */
	if (error) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: Inode <%d.%d>,"
		    " Unable to correct mva inode, error <%d>",
		    bip->mp->mt.fi_name, bip->di.id.ino, bip->di.id.gen, error);
	}
	mutex_exit(&bip->mp->ms.m_inode_mutex);
	ret_eidp->ino = ret_eidp->gen = 0;
}
