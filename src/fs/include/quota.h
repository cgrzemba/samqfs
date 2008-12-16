/*
 *	quota.h - SAM-QFS filesystem quota definitions.
 *
 *	Defines the in-core quota structures.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#ifndef	_SAM_FS_QUOTA_H
#define	_SAM_FS_QUOTA_H

#ifdef sun
#pragma ident "$Revision: 1.26 $"
#endif

#include "sam/quota.h"

/*
 * General stuff:
 *
 * Quotas are kept on a per-fs basis.  Each mounted filesystem with
 * active quotas has a hash table of *sam_quot entries.  These are
 * hashed by index (UID or GID) to find the in-core entries quickly.
 * If not present in memory, we create an entry, enter it into the
 * hash table, and go to disk to get the entry; the location is the
 * /.quota_uid or /.quota_gid, at an offset of UID or GID *
 * sizeof(sam_dquot).  We then relabel it (!invalid) and unlock it.
 * If we find that the quota entry from disk is not initialized
 * (zero limits), we copy the limits (but not in-use values)
 * from record zero.  (This allows administrators to set default
 * values.)
 *
 * For block quotas, things are pretty simple.  Inodes have links to
 * their associated sam_quot structures, and these are checked and/or
 * updated when blocks are allocated/freed.  (The reference count field
 * in the sam_quot structure shows how many in-core inodes point to this
 * particular entry.)  Note that we don't need to do the lookup, even
 * for files being written to, until a block is allocated.
 *
 * For file quotas, it's tougher.  We have to search the hash table to
 * see if one already exists.  If not, we create one.  When we're done
 * with it, we don't recycle it until it has been idle for a while
 * (SAM_QUOTA_EXPIRE seconds), to avoid having to go to disk constantly.
 */

/*
 * Macros
 */
#define	D2QBLKS(x)			(8*((long long)(x)))
#define	S2QBLKS(x)			(((long long)(x) + 512 - 1)/512)

#define		TOTBLKS(xip) \
	((xip)->di.status.b.offline ? \
		S2QBLKS((xip)->di.rm.size) : D2QBLKS((xip)->di.blocks))

#if defined(_KERNEL) || defined(SAM_QUOTA_KERNEL_STRUCTURES)
/*
 * One hash table per mounted filesystem
 */
#define	SAM_QUOTA_HASH_SIZE	128	/* size of mp's quota hash table */

#define	SAM_QUOTA_EXPIRE	120	/* keep sam_quots > this many secs */
#define	SAM_QUOTA_NALLOC	  8	/* sam_quots to alloc if none avail */

/*
 * Return values from quota test function.
 */
#define	SAM_QT_OK			0x0
#define	SAM_QT_OL_FILE_SOFT		0x01
#define	SAM_QT_OL_FILE_HARD		0x02
#define	SAM_QT_TOT_FILE_SOFT		0x04
#define	SAM_QT_TOT_FILE_HARD		0x08
#define	SAM_QT_OL_BLOCK_SOFT		0x10
#define	SAM_QT_OL_BLOCK_HARD		0x20
#define	SAM_QT_TOT_BLOCK_SOFT		0x40
#define	SAM_QT_TOT_BLOCK_HARD		0x80
#define	SAM_QT_ERROR			0x8000	/* quota botch (bad limits?) */

/*
 * Non-null, non-pointer used to place-hold for a quota pointer.
 * This avoids repeated lookups to discover quotas don't apply.
 */
#ifdef	linux
#undef	NODQUOT
#endif
#define	NODQUOT		((void *)-1)		/* non-null non-pointer */

/*
 * In-core disk quota structure
 *
 * These are kept reference counted, and are kept on hash chains.
 * When the reference count drops to zero, they are NOT removed
 * from the hash table.  They are added, however, to an available
 * list.  One can be reclaimed from the free list iff the qt_lastref
 * field shows that it hasn't been referenced for more than
 * SAM_QT_EXPIRE seconds.
 *
 * Conceivably, we might someday want to put an mp pointer in this
 * thing.  For right now, though, it can be got from SAM_VTOI(qt_vp)->mp.
 * vp will always be set, and points to the associated quota file
 * (user, group, or set) on this volume.
 */
struct sam_quot {
#ifdef	linux
	struct inode *qt_vp;
#else
	vnode_t *qt_vp;			/* quota file vnode pointer */
#endif	/* linux */
	int qt_flags;			/* See flags below */
	int qt_ref;			/* # of pointers to this entry */
	int qt_type;			/* user, group, acct? */
	int qt_index;			/* UID, GID, or Admin ID */
	time_t qt_lastref;		/* last touched */
	kmutex_t qt_lock;		/* lock on flags, qt_ref, quota */
					/*   entry. */
	struct {			/* hashed by type, index (always), */
		struct sam_quot *next;	/* though these can change when */
	} qt_hash;			/* the struct is reclaimed. */
	struct {
		struct sam_quot *next;	/* if SAM_QF_AVAIL, then this struct */
		struct sam_quot *prev;	/* is on a doubly-linked list using */
	} qt_avail;			/* these pointers. */
	struct sam_dquot qt_dent;	/* on-disk entry (authoritative) */
};

/*
 * Flags for the in-core quota structure
 */
#define	SAM_QF_BUSY	0x01		/* reading from or writing to disk */
#define	SAM_QF_DIRTY	0x02		/* Quota entry on disk stale */

#define	SAM_QF_OVEROB	0x10		/* over online (soft) block limit */
#define	SAM_QF_OVERTB	0x20		/* over total (soft) block limit */
#define	SAM_QF_OVEROF	0x40		/* over online (soft) file limit */
#define	SAM_QF_OVERTF	0x80		/* over total (soft) file limit */

#define	SAM_QF_AVAIL	0x100		/* On circ. available list */
#define	SAM_QF_HASHED	0x200		/* Initialized and on hash list */

#define	SAM_QF_ANCHOR	0x1000		/* circ. avail. list anchor */


/*
 * Syscalls
 *
 * int sam_quota_op(struct sam_quota_arg *);
 */
int sam_quota_chown(struct sam_mount *mp, struct sam_node *pip,
			uid_t uid, gid_t gid, cred_t *credp);

/*
 * Increment/decrement the used counts for on-line files and blocks.
 * +alloc, -free
 */
int sam_quota_falloc(struct sam_mount *mp,
			uid_t uid, gid_t gid, int, cred_t *credp);
int sam_quota_fret(struct sam_mount *mp, struct sam_node *pip);
int sam_quota_fonline(struct sam_mount *mp, struct sam_node *pip,
			cred_t *credp);
int sam_quota_foffline(struct sam_mount *mp, struct sam_node *pip,
			cred_t *credp);
int sam_quota_balloc(struct sam_mount *mp, struct sam_node *ip,
			long long nblks, long long nblks2, cred_t *credp);
int sam_quota_balloc_perm(struct sam_mount *mp, struct sam_perm_inode *permip,
			long long nblks, long long nblks2, cred_t *credp);
int sam_quota_bret(struct sam_mount *mp, struct sam_node *ip,
			long long nblks, long long nblks2);

#endif	/* _KERNEL || SAM_QUOTA_KERNEL_STRUCTURES */
#endif	/* _SAM_FS_QUOTA_H */
