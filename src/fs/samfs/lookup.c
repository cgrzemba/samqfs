/*
 * -----sam/lookup.c - Process the directory lookup functions.
 *
 *	Processes the SAMFS inode directory functions.
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

#pragma ident "$Revision: 1.81 $"

#include "sam/osversion.h"

/*
 * ----- UNIX Includes
 */

#include <sys/types.h>
#include <sys/t_lock.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/cred.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/mode.h>
#include <sys/vfs.h>
#include <sys/dnlc.h>
#include <sys/fbuf.h>

/*
 * ----- SAMFS Includes
 */

#include "sam/param.h"
#include "sam/types.h"

#include "sblk.h"
#include "inode.h"
#include "mount.h"
#include "dirent.h"
#include "extern.h"
#include "debug.h"
#include "trace.h"
#include "kstats.h"
#include "qfs_log.h"
#include "arfind.h"

extern ushort_t sam_dir_gennamehash(int nl, char *np);

static int sam_find_component(sam_node_t *pip, char *cp, sam_node_t **ipp,
		struct sam_name *namep);
static void sam_rm_dir_entry(sam_node_t *pip, char *cp, sam_u_offset_t offset,
	int in, int bsize, sam_id_t id);
static void sam_shrink_zerodirblk(sam_node_t *ip, sam_u_offset_t offset);
static int sam_xattr_mkdir(sam_node_t *pip, sam_node_t **ipp, cred_t *credp);

boolean_t sam_use_negative_dnlc = TRUE;		/* non-zero use negative DNLC */
boolean_t sam_trust_enhanced_dnlc = TRUE;	/* non-zero trust dir cache */

/*
 * Stolen directly from usr/src/uts/common/fs/ufs/ufs_dir.c
 *
 * The average size of a typical on disk directory entry is about 24 bytes
 * and so defines AVG_DIRECT_ENT at 24,
 * This define is only used to approximate the number of entries
 * in a directory. This is needed for dnlc_dir_start() which will immediately
 * return an error if the value is not within its acceptable range of
 * number of files in a directory.
 */
#define	AVG_DIRECT_ENT (24)
#define	ENHANCED_DNLC_NEEDED (200)
/*
 * If the directory size (from di.rm.size) is greater than the
 * tunable then we request dnlc directory caching.
 * This has found to be profitable after 1024 file names.
 */
int sam_enhance_dnlc_size = ENHANCED_DNLC_NEEDED * AVG_DIRECT_ENT;

int ednlc_didnt_work;
/*
 * We record the "purged due to failure" time in the directory inode.
 * Do not attempt to cache this directory for at least
 * SAM_DDNLC_RETRY_MIN seconds.
 */
#define	SAM_DDNLC_RETRY_MIN (5)

/*
 * ----- sam_lookup_name - Lookup a directory entry.
 * Given a parent directory inode and component name, search the
 * parent directory for the component name. If found, return inode
 * pointer and inode "held" (v_count incremented). If namep != NULL,
 * return found entry else return slot where this component name could fit.
 * For namep == NULL, only lookup -- no lock set on entry.
 * For namep != NULL, remove, link, etc. -- parent data_rwl WRITERS
 * lock set on entry.
 */

int					/* ERRNO if error, 0 if successful. */
sam_lookup_name(
	sam_node_t *pip,		/* parent directory inode */
	char *cp,			/* component name to lookup. */
	sam_node_t **ipp,		/* inode (returned). */
	struct sam_name *namep,		/* entry info returned here if found */
	cred_t *credp)			/* credentials. */
{
	int error;

	if (S_ISDIR(pip->di.mode) == 0) {	/* Parent must be a directory */
		return (ENOTDIR);
	}
	/* Execute access required to search a directory */
	if (error = sam_access_ino(pip, S_IEXEC, FALSE, credp)) {
		return (error);
	}
	/*
	 * Short cut a null namestring and ".", as this directory.
	 */
	if (*cp == '\0' || (*(cp+1) == '\0' && *cp == '.')) {
		VN_HOLD(SAM_ITOV(pip));
		*ipp = pip;
		return (0);
	}

	if (namep && namep->operation == SAM_RENAME_LOOKUPNEW) {
		/*
		 * sam_rename_inode() wants either the location on disk of the
		 * target name so it can be removed if it exists, or an empty
		 * slot for the target name if it does not exist.
		 */
		namep->need_slot = TRUE;
		goto scan_for_slot;

	} else if (namep == NULL || namep->operation == SAM_CREATE ||
	    namep->operation == SAM_MKDIR || namep->operation == SAM_RESTORE ||
	    namep->operation == SAM_LINK) {
		/*
		 * Use the dnlc lookup cache for normal lookups plus SAM_CREATE,
		 * SAM_MKDIR, SAM_LINK, and SAM_RESTORE operations.
		 */
		vnode_t *vp;

		if (namep) {
			namep->need_slot = TRUE;
		}

		error = 0;
		if ((vp = dnlc_lookup(SAM_ITOV(pip), cp))) {
			sam_node_t *ip;

			if (vp == DNLC_NO_VNODE) {
				ASSERT(sam_use_negative_dnlc);
				SAM_COUNT64(dnlc, dnlc_neg_uses);
				if (namep == NULL) {
					error = ENOENT;
					TRACE(T_SAM_DNLC_LU_N, SAM_ITOV(pip),
					    (sam_tr_t)vp,
					    error, vp->v_count);
					VN_RELE(vp);
					return (error);
				}
				TRACE(T_SAM_DNLC_LU_N, SAM_ITOV(pip),
				    (sam_tr_t)vp,
				    error, vp->v_count);
				VN_RELE(vp);
				/*
				 * No name was found, check for a place to put a
				 * name.
				 */
				goto scan_for_slot;
			}

			*ipp = SAM_VTOI(vp);	/* dnlc_lookup does VN_HOLD */
			ip = *ipp;
			TRACE(T_SAM_DNLC_LU, SAM_ITOV(pip), (sam_tr_t)vp,
			    ip->di.id.ino,
			    vp->v_count);
			ASSERT(ip->flags.bits & SAM_HASH);
			/*
			 * Note: If we race with remove, nlink may be 0 here.
			 * The inode will not be freed until the hold we've
			 * placed on the vnode via dnlc_lookup is gone, so we're
			 * safe.
			 */

			/*
			 * If shared_reader, verify dnlc cache entry.
			 * If stale, remove from the dnlc cache.
			 */
			if (!(SAM_IS_SHARED_READER(pip->mp))) {
				return (0);
			}
			if (ip->di.id.ino != SAM_INO_INO) {
				RW_LOCK_OS(&ip->inode_rwl, RW_READER);
				error = sam_refresh_shared_reader_ino(ip,
				    FALSE, credp);
				RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			}
			if (error == ENOENT) {
				dnlc_remove(SAM_ITOV(pip), cp);
				VN_RELE(vp);
			} else {
				return (error);
			}
		}
	}
	/*
	 *	Initialize locking and such.
	 */
scan_for_slot:
	if (namep == NULL) {
		RW_LOCK_OS(&pip->data_rwl, RW_READER);
	} else {
		namep->type = SAM_NULL;
	}

	/*
	 * If directory is offline, stage it in now.
	 */
	if (pip->di.status.b.offline || pip->flags.b.stage_pages) {
		RW_LOCK_OS(&pip->inode_rwl, RW_WRITER);
		if (pip->di.status.b.offline || pip->flags.b.stage_pages) {
			error = sam_clear_ino(pip, pip->di.rm.size,
			    MAKE_ONLINE, credp);
			RW_UNLOCK_OS(&pip->inode_rwl, RW_WRITER);
			if (error) {
				if (namep == NULL) {
					RW_UNLOCK_OS(&pip->data_rwl, RW_READER);
				}
				return (error);
			}
		} else {
			RW_UNLOCK_OS(&pip->inode_rwl, RW_WRITER);
		}
	}
	error = sam_find_component(pip, cp, ipp, namep);

	/*
	 * If not found && shared_reader access, refresh directory pages and
	 * retry.
	 */
	if ((error == ENOENT) && SAM_IS_SHARED_READER(pip->mp)) {
		RW_LOCK_OS(&pip->inode_rwl, RW_READER);
		if ((error = sam_refresh_shared_reader_ino(pip, FALSE,
		    credp)) == 0) {
			RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
			error = sam_find_component(pip, cp, ipp, namep);
		} else {
			RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
		}
	}
	if (namep == NULL) {
		RW_UNLOCK_OS(&pip->data_rwl, RW_READER);
	}
	return (error);
}


/*
 * ----- sam_find_component - Lookup a directory entry for the component.
 * Given a parent directory inode and component name, search the
 * directory for the requested component name.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_find_component(
	sam_node_t *pip,		/* parent directory inode */
	char *cp,			/* component name to lookup. */
	sam_node_t **ipp,		/* inode (returned). */
	struct sam_name *namep)		/* entry info returned here if found */
{
	sam_id_t id;
	sam_ino_t ino;
	sam_node_t *ip;
	sam_u_offset_t start_offset, search_offset_limit;
	sam_u_offset_t offset, hndl_off;
	struct fbuf *fbp;
	vnode_t *pvp;
	int bsize;
	int	check_name;
	int entries_added;
	int error;
	int fbsize;
	int in;
	int namlen;
	ushort_t name_hash, slot_size;
	boolean_t lock_held = FALSE;
	boolean_t slot_found = FALSE;
	boolean_t found_ednlc_entry = FALSE;
	boolean_t found_ednlc_slot = FALSE;
	boolean_t creating_cache = FALSE;
	boolean_t tried_creating_cache = FALSE;
	boolean_t name_found_while_creating_cache = FALSE;
	boolean_t slot_found_while_creating_cache = FALSE;
	boolean_t want_neg_dnlc;
	boolean_t trust_enhanced_dnlc = sam_trust_enhanced_dnlc;
	uint64_t handle;
	dcanchor_t *dcap;
	dcret_t	reply;

	fbp = NULL;
	pvp = SAM_ITOV(pip);
	namlen = strlen(cp);
	name_hash = sam_dir_gennamehash(namlen, cp);
	offset = start_offset = pip->dir_offset;
	search_offset_limit = pip->di.rm.size;
	error = ENOENT;

	pip->dir_offset = 0;

	/*
	 * Use negative DNLC for normal lookups plus SAM_REMOVE and SAM_RMDIR
	 * operations.  Do not use it for shared_reader since it gets stale
	 * because it is not notified of creates.  Do not trust the directory
	 * cache for shared_reader because its directory copy may be
	 * out-of-date.
	 */
	want_neg_dnlc = TRUE;
	if (SAM_IS_SHARED_READER(pip->mp)) {
		want_neg_dnlc = FALSE;
		trust_enhanced_dnlc = FALSE;
	} else if (namep && namep->operation != SAM_REMOVE &&
	    namep->operation != SAM_RMDIR) {
		want_neg_dnlc = FALSE;
	}

	ASSERT(!SAM_IS_SHARED_CLIENT(pip->mp));

	/*
	 * Grab the reader lock on the directory before checking
	 * the directory cache, to avoid race conditions. The following
	 * code is pretty much identical to that in ufs_dir.c.
	 */
	RW_LOCK_OS(&pip->inode_rwl, RW_READER);
	lock_held = TRUE;
	dcap = &pip->i_danchor;
	reply = dnlc_dir_lookup(dcap, cp, &handle);

	switch (reply) {

	case DFOUND:
		/*
		 * A directory cache entry exists for this file.
		 */
		ino = (sam_ino_t)SAM_H_TO_INO(handle);
		if (pip->di.id.ino == ino) {	/* looking for "." */
			VN_HOLD(pvp);
			*ipp = pip;
			RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
			return (0);
		}
		found_ednlc_entry = TRUE;
		/*
		 * shared_reader needs to verify the generation number.
		 */
		if (!trust_enhanced_dnlc) {
			break;
		}
		RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
		lock_held = FALSE;

		/*
		 * The directory cache handle does not contain the
		 * generation number.  Use IG_DNLCDIR for this case.
		 * VN_HOLD if found in sam_get_ino().
		 */
		id.ino = ino;
		id.gen = 0;
		error = sam_get_ino(pvp->v_vfsp, IG_DNLCDIR, &id, &ip);
		if (error) {
			TRACE(T_SAM_EDNLC_RET, pvp, error, 1, 0);
			break;
		}

		hndl_off = SAM_H_TO_OFF(handle);
		offset = hndl_off & (sam_u_offset_t)~(DIR_BLK - 1);
		fbsize = SAM_FBSIZE(offset);
		bsize = fbsize;
		if (fbsize > DIR_BLK) {
			bsize = fbsize - DIR_BLK;
		}
		in = hndl_off % bsize;

		pip->dir_offset = offset;
		if (namep) {
			namep->type = SAM_ENTRY;
			namep->data.entry.offset = offset + in;
		}
		*ipp = ip;

		dnlc_update(SAM_ITOV(pip), cp, SAM_ITOV(ip));

		SAM_COUNT64(dnlc, ednlc_name_hit);
		return (0);

	case DNOENT:
		/*
		 * No entry exists for this name. If we don't need a slot,
		 * and we're using the negative DNLC, make an entry for this
		 * name. Otherwise, we need to go ahead and find an empty slot.
		 */
		error = ENOENT;
		if (trust_enhanced_dnlc && want_neg_dnlc) {
			if (sam_use_negative_dnlc) {
				/*
				 * add the name to negative cache.
				 */
				dnlc_update(pvp, cp, DNLC_NO_VNODE);
				SAM_COUNT64(dnlc, dnlc_neg_entry);
			}
			RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
			*ipp = NULL;
			TRACE(T_SAM_EDNLC_RET, pvp, error, 2, 0);
			return (error);
		}
		/*
		 * We need a directory slot to complete SAM_CREATE, SAM_MKDIR,
		 * SAM_RESTORE, SAM_LINK, and SAM_RENAME_LOOKUPNEW operations.
		 */
		if (namep && namep->need_slot) {
			reply = dnlc_dir_rem_space_by_len(dcap,
			    SAM_DIRLEN(cp), &handle);
			TRACE(T_SAM_EDNLC_REM_L, pvp,
			    (sam_tr_t)SAM_H_TO_OFF(handle),
			    (sam_tr_t)SAM_DIRLEN(cp), reply);
			if (reply == DFOUND) {
				found_ednlc_slot = TRUE;
			} else if (reply == DNOENT) {
				/*
				 * The directory cache says there is no space,
				 * so don't read the directory looking for it.
				 */
				RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
				*ipp = NULL;
				TRACE(T_SAM_EDNLC_RET, pvp, error, 5, 0);
				return (error);
			}
		}
		break;

	case DNOCACHE:
		/*
		 * If no directory cache, try to set one up.
		 * Don't start if this directory had been purged
		 * within SAM_DDNLC_RETRY_MIN seconds.
		 *
		 * Note that a directory cache is set up for the
		 * shared_reader hosts, even though it is not
		 * trusted for the DFOUND and DNOENT cases.  This
		 * is done for performance reasons.  The directory
		 * cache provides a good starting offset for searching
		 * the directory.
		 */
		if ((pip->di.rm.size > sam_enhance_dnlc_size) &&
		    ((pip->ednlc_ft + SAM_DDNLC_RETRY_MIN) < SAM_SECOND())) {
			reply = dnlc_dir_start(dcap,
			    pip->di.rm.size/AVG_DIRECT_ENT);
			if (reply == DOK) {
				entries_added = 0;
				pip->ednlc_ft = 0;
				offset = start_offset = 0;
				tried_creating_cache = creating_cache = TRUE;
				SAM_COUNT64(dnlc, ednlc_starts);
			} else if (reply == DNOMEM) {
				SAM_COUNT64(dnlc, ednlc_no_mem);
			} else if (reply == DTOOBIG) {
				SAM_COUNT64(dnlc, ednlc_too_big);
			}
			TRACE(T_SAM_EDNLC_STR, pvp, pip->di.id.ino,
			    pip->di.rm.size, reply);
		}
		break;

	default:
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: sam_find_component:"
		    " invalid dnlc_dir_lookup reply %d",
		    pip->mp->mt.fi_name, reply);
		break;
	}

	if (lock_held) {
		RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
	}

	if (found_ednlc_slot || found_ednlc_entry) {
		/*
		 * Set up the offset to read the entry (or slot) we're looking
		 * for.
		 */
		hndl_off = SAM_H_TO_OFF(handle);
		start_offset = offset = hndl_off &
		    (sam_u_offset_t)~(DIR_BLK - 1);
		if (found_ednlc_entry) {
			TRACE(T_SAM_EDNLC_LU_E, pvp, (sam_tr_t)hndl_off,
			    (sam_tr_t)SAM_H_TO_INO(handle), (sam_tr_t)offset);
		} else {
			TRACE(T_SAM_EDNLC_LU_S, pvp, (sam_tr_t)hndl_off,
			    (sam_tr_t)SAM_H_TO_LEN(handle), (sam_tr_t)offset);
		}
	}

restart:
	while (offset < search_offset_limit) {
		int ednlc_in;

		fbsize = SAM_FBSIZE(offset);
		bsize = fbsize;
		if (fbsize > DIR_BLK) {
			bsize = fbsize - DIR_BLK;
		}
		if ((error = fbread(SAM_ITOV(pip), offset,
		    bsize, S_OTHER, &fbp))) {
			creating_cache = FALSE;
			goto done;
		}
		/*
		 * Validate directory block.
		 * EDOM is returned if validation fails.
		 * ENOENT is returned if a full zero block at end of directory
		 *		 (bogus zero directory page).
		 */
		if ((error = sam_validate_dir_blk(pip, offset,
		    bsize, &fbp)) != 0) {
			creating_cache = FALSE;
			goto done;
		}
		/* start at the beginning */
		error = ENOENT;
		in = 0;
		ednlc_in = DIR_BLK;
		if (found_ednlc_slot || found_ednlc_entry) {
			/* get a pointer to the record in question */
			if (trust_enhanced_dnlc) {
				in = ednlc_in = hndl_off % bsize;
			} else {
				ednlc_in = hndl_off % bsize;
			}
		}
		/* stop at the directory validation record */
		while (in < (bsize - sizeof (struct sam_dirval))) {
			struct sam_dirent *dp;

			slot_found_while_creating_cache = FALSE;
			dp = (struct sam_dirent *)(void *)(fbp->fb_addr + in);
			if (dp->d_fmt > SAM_DIR_VERSION) { /* If full entry */
				id = dp->d_id;
				if ((id.ino == 0) ||
				    (dp->d_namlen == 0) ||
				    (dp->d_namlen > dp->d_reclen)) {
					/* Directory inode entry invalid */
					error = EBADSLT;
					creating_cache = FALSE;
					cmn_err(SAMFS_CE_DEBUG_PANIC,
					    "SAM-QFS: %s: sam_find_component:"
					    " invalid directory entry EBADSLT"
					    " ino=%d offset=%llx dp=%p"
					    " d_fmt=%x d_reclen=%x d_namlen=%x",
					    pip->mp->mt.fi_name, id.ino,
					    offset, (void *)dp,
					    dp->d_fmt, dp->d_reclen,
					    dp->d_namlen);
					goto done;
				}
				/*
				 * Lookup a name component in the directory
				 * block.  If the directory block entries
				 * namehash is zero, use the "first character"
				 * method, otherwise use the namehash as a
				 * filter.
				 */
				check_name = 0;
				if (dp->d_namlen == namlen) {
					if (dp->d_namehash) {
						if (dp->d_namehash ==
						    name_hash) {
							check_name++;
						}
					} else if (*cp == (char)*dp->d_name) {
						check_name++;
					}
				}
				if (check_name &&
				    (bcmp((cp), (dp->d_name),
				    (dp->d_namlen)+1) == 0)) {
					if (id.ino == pip->di.id.ino) {
						/* if '.' lookup */
						VN_HOLD(SAM_ITOV(pip));
						ip = pip;
						error = 0;
					} else {
						/*
						 * VN_HOLD if found in
						 * sam_get_ino
						 */
						if ((error =
						    sam_get_ino(pvp->v_vfsp,
						    IG_EXISTS,
						    &id, &ip)) == ENOENT) {
							/*
							 * If incompleted
							 * directory entry,
							 * remove entry.  This
							 * make the filesystem
							 * consistent for
							 * directory entries
							 * existing in the
							 * directory, but not in
							 * .inodes.
							 */
							creating_cache = FALSE;
							fbrelse(fbp, S_OTHER);
							fbp = NULL;
							sam_rm_dir_entry(pip,
							    cp, offset, in,
							    bsize, id);
						}
					}
					if (error == 0) {
						pip->dir_offset = offset;
						if (namep) {
							namep->type = SAM_ENTRY;
					namep->data.entry.offset = offset + in;
						}
						slot_found = TRUE;
					name_found_while_creating_cache = TRUE;
						*ipp = ip;
						/*
						 * dnlc_update() seems to have
						 * identical functionality to
						 * dnlc_enter(). The following
						 * dnlc_update() replaces a
						 * dnlc_purge_vp() and a
						 * dnlc_enter().
						 */
						dnlc_update(SAM_ITOV(pip), cp,
						    SAM_ITOV(ip));

						if (found_ednlc_entry) {
							if (ednlc_in == in) {
					SAM_COUNT64(dnlc, ednlc_name_found);
							} else {
							goto abandon_ednlc;
							}
						}

					}
					if (creating_cache == FALSE) {
						goto done;
					}
				}
				/* Find a big enough slot */
				if ((namep != NULL) && (!slot_found)) {
					if (SAM_DIRLEN(cp) <=
					    (dp->d_reclen - SAM_DIRSIZ(dp))) {
						slot_found = TRUE;
						if (creating_cache) {
							/*
							 * Do not add this space
							 * to the directory
							 * cache.  The caller
							 * will do this.
							 */
					slot_found_while_creating_cache = TRUE;
						}
						namep->type = SAM_BIG_SLOT;
						namep->data.slot.offset =
						    offset + in;
						namep->data.slot.reclen =
						    dp->d_reclen;
						namep->slot_handle = NULL;
						if (found_ednlc_entry &&
						    ednlc_in == in) {
							namep->slot_handle =
							    handle;
						}
					}
				}



	/* N.B. Bad indentation here to meet cstyle requirements */
	if (creating_cache) {
		/*
		 * Got a directory entry, now enter it
		 * into the cache
		 */
		entries_added++;
		handle = SAM_DIR_OFF_TO_H(id.ino,
		    offset+in);
		reply = dnlc_dir_add_entry(dcap,
		    (char *)dp->d_name, handle);
		if (reply != DOK) {
			TRACE(T_SAM_EDNLC_ERR, pvp,
			    id.ino,
			    strlen((char *)dp->d_name),
			    reply);
			if (reply == DTOOBIG) {
				SAM_COUNT64(dnlc,
				    ednlc_too_big);
			}
			creating_cache = FALSE;
		} else {
			TRACE(T_SAM_EDNLC_ADD_E, pvp,
			    (sam_tr_t)SAM_H_TO_OFF(
			    handle),
			    (sam_tr_t)SAM_H_TO_INO(
			    handle), reply);
			if (!slot_found_while_creating_cache &&
			    (dp->d_reclen >
			    SAM_DIRSIZ(dp))) {
				slot_size =
				    dp->d_reclen -
				    SAM_DIRSIZ(dp);
				if (slot_size >=
				    SAM_DIRLEN_MIN) {
					/*
					 * Add big slot
					 * space to the
					 * directory
					 * cache
					 */
					handle = SAM_LEN_TO_H(slot_size,
					    offset+in);
					reply = dnlc_dir_add_space(dcap,
					    slot_size, handle);
					if (reply != DOK) {
						creating_cache = FALSE;
						TRACE(T_SAM_EDNLC_ERR, pvp, 0,
						    strlen((char *)dp->d_name),
						    reply);
					} else {
						TRACE(T_SAM_EDNLC_ADD_S, pvp,
						    (sam_tr_t)SAM_H_TO_OFF(
						    handle),
						    (sam_tr_t)SAM_H_TO_LEN(
						    handle),
						    reply);
					}
				}
			}
		}
	}




			} else {	/* If empty entry */
				if ((namep != NULL) && (!slot_found)) {
					if (SAM_DIRLEN(cp) <= dp->d_reclen) {
						slot_found = TRUE;
						if (creating_cache) {
							/*
							 * Do not add this space
							 * to the directory
							 * cache.  The caller
							 * will do this.
							 */
					slot_found_while_creating_cache = TRUE;
						}
						namep->type = SAM_EMPTY_SLOT;
						namep->data.slot.offset =
						    offset + in;
						namep->data.slot.reclen =
						    dp->d_reclen;
					}
				}



	/* N.B. Bad indentation here to meet cstyle requirements */
	if (creating_cache) {
		/*
		 * Got an empty entry, now enter it into
		 * the cache
		 */
		if (!slot_found_while_creating_cache) {
			slot_size = dp->d_reclen;
			if (slot_size >=
			    SAM_DIRLEN_MIN) {
				handle =
				    SAM_LEN_TO_H(slot_size, offset+in);
				reply =
				    dnlc_dir_add_space(dcap,
				    slot_size, handle);
				if (reply != DOK) {
					creating_cache =
					    FALSE;
					TRACE(T_SAM_EDNLC_ERR, pvp, 0,
					    strlen((char *)dp->d_name), reply);
				} else {
					int spaces_added;

					spaces_added = dp->d_reclen /
					    AVG_DIRECT_ENT;
					entries_added += spaces_added;
					TRACE(T_SAM_EDNLC_ADD_S, pvp,
					    (sam_tr_t)SAM_H_TO_OFF(handle),
					    (sam_tr_t)SAM_H_TO_LEN(handle),
					    reply);
				}
			}
		}
	}



			}
			/* Following code duplicated in getdents.c */
			if (((int)dp->d_reclen <= 0) ||
			    ((int)dp->d_reclen > bsize) ||
			    ((int)dp->d_reclen & (NBPW-1))) {
				error = EINVAL;	/* Ill-formed directory entry */
				creating_cache = FALSE;
				cmn_err(SAMFS_CE_DEBUG_PANIC,
				    "SAM-QFS: %s: sam_find_component:"
				    " invalid directory entry EINVAL"
				    " ino=%d offset=%llx bsize=%x dp=%p"
				    " fmt=%x d_reclen=%x d_namlen=%x",
				    pip->mp->mt.fi_name, id.ino, offset,
				    bsize, (void *)dp,
				    dp->d_fmt, dp->d_reclen, dp->d_namlen);
				goto done;
			}
			if (found_ednlc_slot || found_ednlc_entry) {
				if (found_ednlc_slot && slot_found) {
					if (in == ednlc_in) {
						SAM_COUNT64(dnlc,
						    ednlc_space_found);
						goto done;
					}
				}
				if (found_ednlc_entry) {
					SAM_COUNT64(dnlc, ednlc_name_missed);
				} else {
					SAM_COUNT64(dnlc, ednlc_space_missed);
				}
abandon_ednlc:
				/*
				 * There are problems with the directory cache
				 * copy, fall back to the "normal" search.
				 */
				found_ednlc_slot = found_ednlc_entry = FALSE;
				if (fbp != NULL) {
					fbrelse(fbp, S_OTHER);
					fbp = NULL;
				}
				ednlc_didnt_work++;
				if (name_found_while_creating_cache) {
					goto done;
				}
				goto restart;
			}
			in += dp->d_reclen;
		}
		offset += bsize;
		/* Handle directory offset caching wrap-around */
		if (offset >= pip->di.rm.size && start_offset != 0) {
			search_offset_limit = start_offset;
			offset = 0;
		}
		fbrelse(fbp, S_OTHER);
		fbp = NULL;
	}
done:
	if (fbp != NULL) {
		fbrelse(fbp, S_OTHER);
		fbp = NULL;
	}
	if (name_found_while_creating_cache) {
		/*
		 * The name lookup succeeded.  We need to clear any errors that
		 * were encountered while setting up the directory cache so that
		 * these are not returned to the caller.  The directory cache
		 * will be purged for all unexpected errors.  Note that ENOENT
		 * in an expected by-product of setting up the directory cache.
		 * However, ENOENT will cause the directory cache to be purged
		 * when returned from sam_get_ino().
		 */
		if (error) {
			TRACE(T_SAM_EDNLC_RET, pvp, error, 3, 0);
		}
		error = 0;
	}
	if (error != 0 && found_ednlc_entry) {
		goto abandon_ednlc;
	} else if (tried_creating_cache) {
		if (error && (error != ENOENT)) {
			/*
			 * Purge the directory cache for unexpected errors.
			 */
			creating_cache = FALSE;
		}
		/*
		 * Purge the directory cache if it meets the minimum size
		 * requirement, but does not meet half of the minimum
		 * directory entry number requirement.  This can occur
		 * when you have lots of large file names.
		 */
		if (entries_added < (ENHANCED_DNLC_NEEDED/2)) {
			creating_cache = FALSE;
		}

		if (creating_cache == FALSE) {
			/*
			 * Delete the partial or complete directory cache.
			 */
			dnlc_dir_purge(dcap);
			pip->ednlc_ft = SAM_SECOND();
			SAM_COUNT64(dnlc, ednlc_purges);
			TRACE(T_SAM_EDNLC_PURGE, pvp, pip->di.id.ino, 1, 0);
		} else {
			/*
			 * The directory cache is complete.
			 */
			dnlc_dir_complete(dcap);
			TRACE(T_SAM_EDNLC_STR_RET, pvp, pip->di.id.ino, 0, 0);
		}
	}
	/*
	 * Handle the scenario where either we don't have this directory cached,
	 * or we're not trusting/using the directory cache.
	 */
	if (error == ENOENT && want_neg_dnlc) {
		if (sam_use_negative_dnlc) {
			/*
			 * add the name to negative cache.
			 */
			dnlc_update(pvp, cp, DNLC_NO_VNODE);
			SAM_COUNT64(dnlc, dnlc_neg_entry);
		}
		TRACE(T_SAM_EDNLC_RET, pvp, error, 4, 0);
	}
	return (error);
}


/*
 * ----- sam_rm_dir_entry - Remove directory entry that has no inode.
 * The directory entry and the inode entry are written asynronously.
 * It is possible to get the directory entry written without
 * the inode entry written (Uncompleted transaction). Remove these
 * incompleted entries.
 *
 * Return 0 fbuf acquired for write, 1 error getting fbp.
 */

static void
sam_rm_dir_entry(
	sam_node_t *pip,	/* parent directory inode */
	char *cp,		/* component name to lookup. */
	sam_u_offset_t offset,
	int in,
	int bsize,
	sam_id_t id)
{
	struct fbuf *fbp;
	struct sam_dirent *dp;
	int namlen;

	if ((pip->mp->mt.fi_mflag & MS_RDONLY) ||
	    SAM_IS_SHARED_READER(pip->mp)) {
		return;
	}
	/*
	 * Need to get the page in writeable mode.
	 */
	if (fbread(SAM_ITOV(pip), offset, bsize, S_WRITE, &fbp) != 0) {
		return;
	}

	if (!rw_write_held(&pip->data_rwl)) {
		if (!rw_tryupgrade(&pip->data_rwl)) {
			fbrelse(fbp, S_WRITE);
			return;
		}
	}
	dp = (struct sam_dirent *)(void *)(fbp->fb_addr + in);
	namlen = strlen(cp);
	if ((dp->d_namlen == namlen) && (*cp == (char)*dp->d_name) &&
	    (bcmp((cp + 1), (dp->d_name + 1), dp->d_namlen) == 0)) {
		ushort_t reclen;

		reclen = dp->d_reclen;
		bzero((caddr_t)dp, reclen);
		dp->d_fmt = 0;
		dp->d_reclen = reclen;
		RW_LOCK_OS(&pip->inode_rwl, RW_READER);
		mutex_enter(&pip->fl_mutex);
		TRANS_INODE(pip->mp, pip);
		sam_mark_ino(pip, (SAM_UPDATED | SAM_CHANGED));
		mutex_exit(&pip->fl_mutex);
		RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
		TRACE(T_SAM_RMDIRENT, SAM_ITOV(pip), id.ino,
		    id.gen, pip->di.id.ino);
	}
	fbrelse(fbp, S_WRITE);
}


/*
 * ----- sam_shrink_zerodirblk -
 * At 3.3.1-25 through 3.3.1-34 and 3.5.0-xx through 3.5.0-xy, a problem
 * in create code could have caused a zero block at the end of a directory,
 * without a validation record. sam_shrink_zerodirblk corrects this condition
 * by adjusting the size of the directory.
 */

static void
sam_shrink_zerodirblk(sam_node_t *pip, sam_u_offset_t offset)
{
	if (pip->mp->mt.fi_mflag & MS_RDONLY) {
		return;
	}
	cmn_err(CE_NOTE,
	    "SAM-QFS: %s: sam_lookup_name: corrected directory inode %d.%d",
	    pip->mp->mt.fi_name, pip->di.id.ino, pip->di.id.gen);
	RW_LOCK_OS(&pip->inode_rwl, RW_WRITER);
	ASSERT(pip->di.rm.size == pip->size);
	pip->di.rm.size = pip->size = offset;
	TRANS_INODE(pip->mp, pip);
	sam_mark_ino(pip, (SAM_UPDATED | SAM_CHANGED));
	RW_UNLOCK_OS(&pip->inode_rwl, RW_WRITER);
}

static	int sam_dnlc_purge_count;

/*
 * ----- sam_invalidate_dnlc -
 * Invalidate all dnlc entries associated with the
 * specified vnode. The vnode should be a directory.
 * dnlc_purge_vp() is called, and have been found to
 * be very expensive. So, this routine should be used
 * sparingly.
 */
void
sam_invalidate_dnlc(vnode_t *vp)
{
	ASSERT(vp->v_type == VDIR);
	sam_dnlc_purge_count++;
	dnlc_purge_vp(vp);
}

/*
 * ----- sam_validate_dir_blk -
 * Check that the contents of the block specified are either
 * a valid directory block, belonging to the parent directory
 * or the bogus zero block, which is then eliminated.
 * Returns
 *	0, if valid.
 *	EDOM, if not a valid directory block.
 *	ENOENT, if the bougus zero block was encountered.
 */
int
sam_validate_dir_blk(sam_node_t *pip, offset_t offset, int bsize,
    struct fbuf **fbpp)
{
	int error = 0;
	struct sam_dirval *dvp;
	struct fbuf *fbp = *fbpp;

	dvp = (struct sam_dirval *)(void *) (fbp->fb_addr +
	    (bsize - sizeof (struct sam_dirval)));
	if ((dvp->d_id.ino != pip->di.id.ino) ||
	    (dvp->d_id.gen != pip->di.id.gen) ||
	    (dvp->d_version != SAM_DIR_VERSION)) {
		if (sam_check_zerodirblk(pip, fbp->fb_addr, offset) == 0) {
			error = EDOM;	/* Not directory data */
			SAMFS_PANIC_INO(pip->mp->mt.fi_name,
			    "Bad directory validation", pip->di.id.ino);
		} else {
			fbrelse(fbp, S_OTHER);
			*fbpp = NULL;
			sam_shrink_zerodirblk(pip, offset);
			error = ENOENT;
		}
	}
	return (error);
}


/*
 * ----- sam_xattr_mkdir -
 * Create the unnamed directory that hangs off of pip and acts as the
 * root of the xattr directory tree for that file.
 */
static int
sam_xattr_mkdir(
	sam_node_t *pip,
	sam_node_t **ipp,
	cred_t *credp)
{
	int error;
	vattr_t va;
	sam_mount_t *mp;

	if (SAM_INODE_HAS_XATTR(pip)) {
		return (EEXIST);
	}
	if ((error = sam_access_ino(pip, S_IWRITE, TRUE, credp)) != 0) {
		return (error);
	}
	if (vn_is_readonly(SAM_ITOV(pip))) {
		return (EROFS);
	}
	va.va_type = VDIR;
	va.va_uid = pip->di.uid;
	va.va_gid = pip->di.gid;

	if ((pip->di.mode & S_IFMT) == S_IFDIR) {
		va.va_mode = S_IFATTRDIR;
		va.va_mode |= pip->di.mode & 0777;
	} else {
		va.va_mode = S_IFATTRDIR|0700;
		if (pip->di.mode & 0040) {
			va.va_mode |= 0750;
		}
		if (pip->di.mode & 0004) {
			va.va_mode |= 0705;
		}
	}
	va.va_mask = AT_TYPE|AT_MODE;
	mp = pip->mp;
	error = sam_make_ino(pip, &va, ipp, credp);
	if (!error) {
		sam_node_t *ip;

		ip = *ipp;

		error = sam_make_dir(pip, ip, credp);
		if (!error) {
			ASSERT(RW_OWNER_OS(&pip->inode_rwl) == curthread);
			pip->di2.xattr_id = ip->di.id;
			sam_mark_ino(pip, (SAM_UPDATED | SAM_CHANGED));
			TRANS_INODE(mp, pip);
			ip->di2.p2flags |= P2FLAGS_XATTR;
			SAM_ITOV(ip)->v_flag |= V_XATTRDIR;
			TRANS_INODE(mp, ip);
			sam_mark_ino(ip, (SAM_UPDATED | SAM_CHANGED));
			sam_send_to_arfind(ip, AE_create, 0);
			if (TRANS_ISTRANS(mp) || SAM_SYNC_META(mp)) {
				sam_sync_meta(pip, ip, credp);
			}

		} else {
			ip->di.nlink = 0;
			TRANS_INODE(mp, ip);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			sam_inactive_ino(ip, credp);
			return (error);
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}
	if (error == 0) {
		sam_sbinfo_t *sbp;

		sbp = &mp->mi.m_sbp->info.sb;
		if (SBLK_XATTR(sbp) == 0) {
			sbp->opt_mask |= SBLK_OPTV1_XATTR;
			sbp->magic = SAM_MAGIC_V2A;
			if ((sam_update_the_sblks(mp)) != 0) {
				cmn_err(CE_WARN, "SAM-QFS: %s: Error writing "
				    "superblock to update extended "
				    "attributes\n", mp->mt.fi_name);
			}
		}
	}
	return (error);
}


/*
 * ----- sam_xattr_getattrdir -
 * Process extended attributes flags: LOOKUP_ATTR & CREATE_XATTR_DIR.
 */
int
sam_xattr_getattrdir(
	vnode_t *pvp,
	sam_node_t **ipp,
	int flags,
	cred_t *credp)
{
	sam_node_t *pip, *sdp;
	int error;

	pip = SAM_VTOI(pvp);
	if (flags & LOOKUP_XATTR) {
		if (pip && SAM_INODE_HAS_XATTR(pip)) {
			error = sam_get_ino(pvp->v_vfsp, IG_EXISTS,
			    &pip->di2.xattr_id, ipp);
			if (error) {
				return (error);
			}
			sdp = *ipp;
			if (!S_ISATTRDIR(sdp->di.mode)) {
				cmn_err(CE_WARN,
				    "sam_getattrdir: inode %d.%d "
				    "points to non-attribute "
				    "directory %d.%d; run fsck",
				    pip->di.id.ino, pip->di.id.gen,
				    sdp->di.id.ino, sdp->di.id.gen);
				VN_RELE(SAM_ITOV(sdp));
				return (ENOENT);
			}
			SAM_ITOV(sdp)->v_type = VDIR;
			SAM_ITOV(sdp)->v_flag |= V_XATTRDIR;
			error = 0;
		} else if (flags & CREATE_XATTR_DIR) {
			error = sam_xattr_mkdir(pip, ipp, credp);
		} else {
			error = ENOENT;
		}
	} else if (flags & CREATE_XATTR_DIR) {
		error = sam_xattr_mkdir(pip, ipp, credp);
	} else {
		error = ENOENT;
	}
	return (error);
}


/*
 * ----- sam_lookup_xattr -
 * Process extended attributes flags: LOOKUP_ATTR & CREATE_XATTR_DIR.
 */
int
sam_lookup_xattr(
	vnode_t *pvp,		/* Pointer to parent directory vnode */
	vnode_t **vpp,		/* Pointer pointer to the vnode (returned). */
	int flags,		/* Flags. */
	cred_t *credp)		/* credentials pointer. */
{
	int error = 0;
	sam_node_t *pip, *ip;
	vnode_t *vp;
	int issync;
	int trans_size;
	int terr = 0;

	pip = SAM_VTOI(pvp);
	TRACE(T_SAM_LOOKUP_XATTR, pvp, pip->di.id.ino, SAM_INODE_IS_XATTR(pip),
	    flags);
	ASSERT(flags & LOOKUP_XATTR);
	if (pip->mp->mt.fi_config1 & MC_NOXATTR) {
		/*
		 * Extended attributes are disabled on this mount.
		 */
		return (EINVAL);
	}
	trans_size = (int)TOP_MKDIR_SIZE(pip);
	TRANS_BEGIN_CSYNC(pip->mp, issync, TOP_MKDIR, trans_size);

	RW_LOCK_OS(&pip->inode_rwl, RW_WRITER);

	if (SAM_INODE_IS_XATTR(pip)) {
		/*
		 * We don't allow recursive extended attributes.
		 */
		error = EINVAL;
		goto out;
	}

	if ((vp = dnlc_lookup(pvp, XATTR_DIR_NAME)) == NULL) {
		error = sam_xattr_getattrdir(pvp, &ip, flags, credp);
		if (error) {
			*vpp = NULL;
			goto out;
		}
		vp = SAM_ITOV(ip);
		dnlc_update(pvp, XATTR_DIR_NAME, vp);
	}

	if (vp == DNLC_NO_VNODE) {
		VN_RELE(vp);
		error = ENOENT;
		goto out;
	}

	/*
	 * Check accessibility of directory.
	 */
	error = sam_access_ino(SAM_VTOI(vp), S_IEXEC, FALSE, credp);
	if (error) {
		VN_RELE(vp);
		goto out;
	}
	*vpp = vp;

out:
	RW_UNLOCK_OS(&pip->inode_rwl, RW_WRITER);
	TRANS_END_CSYNC(pip->mp, terr, issync, TOP_MKDIR, trans_size);
	if (error == 0) {
		error = terr;
	}
	if (error) {
		TRACE(T_SAM_LOOKUP_XA_ERR, pvp, pip->di.id.ino, error, 0);
	} else {
		ip = SAM_VTOI(vp);
		TRACE(T_SAM_LOOKUP_XA_RET, pvp, (sam_tr_t)*vpp, ip->di.id.ino,
		    ip->di.nlink);
	}
	return (error);
}
