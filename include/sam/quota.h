/*
 * include/sam/quota.h - SAM-FS filesystem quota definitions.
 *
 * Defines the on-disk quota structures.
 *
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#ifdef sun
#pragma ident "$Revision: 1.17 $"
#endif

#ifndef	_SAM_QUOTA_H
#define	_SAM_QUOTA_H

/*
 * General stuff:
 *
 * Quotas are kept on a per-fs basis.  Each mounted filesystem with
 * active quotas has a hash table of *sam_quot entries.  These are
 * hashed by index (UID or GID) to find the in-core entries quickly.
 * If not present in memory, we create an entry, enter it into the
 * hash table, and go to disk to get the entry; the location is the
 * /.quota_uid or /.quota_gid, at an offset of UID or GID *
 * sizeof(sam_dquot).
 *
 * For block quotas, things are pretty simple.  Inodes have links to
 * their associated sam_quot structures, and these are checked and/or
 * updated when blocks are allocated/freed.  (The reference count field
 * in the sam_quot structure shows how many in-core inodes point to this
 * particular entry.  Note that these only need to be allocated to files
 * that are being allocated to.) Note that we don't need to do the lookup,
 * even for files being written to, until a block is allocated.
 *
 * For file quotas, it's tougher.  We have to search the hash table to
 * see if one already exists.  If not, we create one.  When we're done
 * with it, we don't recycle it until it has been idle for a while, to
 * avoid having to go to disk constantly.
 */

/*
 * Quota type definitions.  If ACCTID quotas were added, they'd go here,
 * and SAM_QUOTA_MAX would be bumped up.
 */
#define	SAM_QUOTA_ADMIN		0
#define	SAM_QUOTA_GROUP		1
#define	SAM_QUOTA_USER		2


#define	SAM_QUOTA_DEFD		SAM_QUOTA_MAX		/* Defined quotas */

#define	QUOTA_ADMIN_FILE	".quota_a"
#define	QUOTA_GROUP_FILE	".quota_g"
#define	QUOTA_USER_FILE		".quota_u"

#ifdef	__QUOTA_DEFS
char *quota_files[SAM_QUOTA_MAX] = {
	QUOTA_ADMIN_FILE,
	QUOTA_GROUP_FILE,
	QUOTA_USER_FILE,
};

char *quota_types[SAM_QUOTA_MAX] = {
	"admin",
	"group",
	"user",
};
#endif

extern char *quota_files[];
extern char *quota_types[];

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif

typedef struct sam_quota_group {
	int64_t qg_in_use;
	int64_t qg_soft_limit;
	int64_t qg_hard_limit;
} sam_quota_group_t;

typedef struct sam_quota_group_pair {
	sam_quota_group_t qgp_files;
	sam_quota_group_t qgp_blocks;
} sam_quota_group_pair_t;

/*
 * On-disk quota information.
 * Needs to stay 2^n bytes, so it won't wrap over block boundaries.
 * The same structure is used for both user and group quotas.
 */
typedef struct sam_dquot {
	sam_quota_group_pair_t dq_online;
	sam_quota_group_pair_t dq_total;
	int		dq_ol_grace;	/* on-line grace period (seconds) */
	sam_time_t	dq_ol_enforce;	/* when to enforce soft limit as hard */
	int		dq_tot_grace;	/* totals grace period (seconds) */
	sam_time_t	dq_tot_enforce;	/* when to enforce soft limit as hard */
	int64_t		unused2;
	int64_t		unused3;
} sam_dquot_t;

/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif

#define	dq_folused	dq_online.qgp_files.qg_in_use
#define	dq_folsoft	dq_online.qgp_files.qg_soft_limit
#define	dq_folhard	dq_online.qgp_files.qg_hard_limit

#define	dq_bolused	dq_online.qgp_blocks.qg_in_use
#define	dq_bolsoft	dq_online.qgp_blocks.qg_soft_limit
#define	dq_bolhard	dq_online.qgp_blocks.qg_hard_limit

#define	dq_ftotused	dq_total.qgp_files.qg_in_use
#define	dq_ftotsoft	dq_total.qgp_files.qg_soft_limit
#define	dq_ftothard	dq_total.qgp_files.qg_hard_limit

#define	dq_btotused	dq_total.qgp_blocks.qg_in_use
#define	dq_btotsoft	dq_total.qgp_blocks.qg_soft_limit
#define	dq_btothard	dq_total.qgp_blocks.qg_hard_limit

/*
 * If total hard files limit and total hard blocks limit are 0, then
 * the quota is infinite.  For a zero quota, set the hard total limits
 * small (e.g., 1), and set the on-line limits to zero.  If everything
 * is zero, then the quota record will be initialized from index zero
 * of the same quota file.
 *
 * Index zero (i.e., UID 0, GID 0, or ADMIN ID 0) also always has an
 * infinite quota.  (But can't be tested for here.)
 */
#define	QUOTA_INF(qp) ((qp)->dq_ftothard == 0 && (qp)->dq_btothard == 0)

/*
 * Verify a correct partial ordering for block and file limits.
 *
 *    total-hard >= total-soft  >= online-soft >= 0
 * && total-hard >= online-hard >= online-soft
 */
#define	QUOTA_SANE(qp) \
	    (((qp)->dq_folsoft >= 0) && \
	((qp)->dq_ftotsoft >= (qp)->dq_folsoft) && \
	((qp)->dq_ftothard >= (qp)->dq_ftotsoft) &&  \
	((qp)->dq_folhard  >= (qp)->dq_folsoft) && \
	((qp)->dq_ftothard >= (qp)->dq_folhard) && \
	((qp)->dq_bolsoft >= 0) && \
	((qp)->dq_btotsoft >= (qp)->dq_bolsoft) && \
	((qp)->dq_btothard >= (qp)->dq_btotsoft) && \
	((qp)->dq_bolhard  >= (qp)->dq_bolsoft) && \
	((qp)->dq_btothard >= (qp)->dq_bolhard))

/*
 * Definitions for SAM quota system call
 */
#define	SAM_QOP_QSTAT	1	/* non-priv; return active quotas on FS */
#define	SAM_QOP_GET	2	/* non-priv; read quota record (active FS) */
#define	SAM_QOP_PUT	3	/* priv; write quota record (active FS only) */
#define	SAM_QOP_PUTALL	4	/* priv; write quota record (active FS only) */

#define	SAM_QFL_ADMIN	(1<<SAM_QUOTA_ADMIN)
#define	SAM_QFL_GROUP	(1<<SAM_QUOTA_GROUP)
#define	SAM_QFL_USER	(1<<SAM_QUOTA_USER)
#define	SAM_QFL_INDEX	0x100

int sam_get_quota_stat(int fd, int *mask);

int sam_get_quota_entry_by_fd(
	int fd,				/* in */
	int type,			/* in */
	int *index,			/* out */
	struct sam_dquot *dq);		/* out */

int sam_get_quota_entry_by_index(
	int fd,				/* in */
	int type,			/* in */
	int index,			/* in */
	struct sam_dquot *dq);		/* out */

/*
 * Enter a quota entry into system.   Does NOT overwrite
 * the entry's in-use values.
 */
int sam_put_quota_entry(
	int fd,				/* in */
	int type,			/* in */
	int index,			/* in */
	struct sam_dquot *dq);		/* in */

/*
 * Enter a quota entry into system.  Overwrites ALL fields
 * of the entry, including in-use (and possibly active) fields.
 */
int sam_putall_quota_entry(
	int fd,				/* in */
	int type,			/* in */
	int index,			/* in */
	struct sam_dquot *dq);		/* in */

/*
 * Set a file's/dir's/... Admin Id.
 * (And the 'don't follow the target if it's a symbolic link' flavor.)
 */
int sam_chaid(const char *path, int admin_id);
int sam_lchaid(const char *path, int admin_id);

#endif	/* _SAM_QUOTA_H */
