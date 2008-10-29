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

#ifndef _SYS_FS_QFS_TRANS_H
#define	_SYS_FS_QFS_TRANS_H

#ifdef sun
#pragma ident	"$Revision: 1.6 $"
#endif /* sun */

/*
 * #ifdef	__cplusplus
 * extern "C" {
 * #endif
 */

#include	<sys/types.h>
#include	<sys/cred.h>
#ifdef LUFS
#include	<sys/fs/qfs_fs.h>
#endif /* LUFS */

/*
 * Types of deltas
 */
typedef enum delta_type {
	DT_NONE,	/*  0 no assigned type */
	DT_SB,		/*  1 superblock */
	DT_CG,		/*  2 cylinder group */
	DT_SI,		/*  3 summary info */
	DT_AB,		/*  4 allocation block */
	DT_ABZERO,	/*  5 a zero'ed allocation block */
	DT_DIR,		/*  6 directory */
	DT_INODE,	/*  7 inode */
	DT_FBI,		/*  8 fbiwrite */
	DT_QR,		/*  9 quota record */
	DT_COMMIT,	/* 10 commit record */
	DT_CANCEL,	/* 11 cancel record */
	DT_BOT,		/* 12 begin transaction */
	DT_EOT,		/* 13 end   transaction */
	DT_UD,		/* 14 userdata */
	DT_SUD,		/* 15 userdata found during log scan */
	DT_SHAD,	/* 16 data for a shadow inode */
	DT_MAX		/* 17 maximum delta type */
} delta_t;

/*
 * transaction operation types
 */
typedef enum top_type {
	TOP_READ_SYNC,		/* 0 */
	TOP_WRITE,		/* 1 */
	TOP_WRITE_SYNC,		/* 2 */
	TOP_SETATTR,		/* 3 */
	TOP_CREATE,		/* 4 */
	TOP_REMOVE,		/* 5 */
	TOP_LINK,		/* 6 */
	TOP_RENAME,		/* 7 */
	TOP_MKDIR,		/* 8 */
	TOP_RMDIR,		/* 9 */
	TOP_SYMLINK,		/* 10 */
	TOP_FSYNC,		/* 11 */
	TOP_GETPAGE,		/* 12 */
	TOP_PUTPAGE,		/* 13 */
	TOP_SBUPDATE_FLUSH,	/* 14 */
	TOP_SBUPDATE_UPDATE,	/* 15 */
	TOP_SBUPDATE_UNMOUNT,	/* 16 */
	TOP_SYNCIP_CLOSEDQ,	/* 17 */
	TOP_SYNCIP_FLUSHI,	/* 18 */
	TOP_SYNCIP_HLOCK,	/* 19 */
	TOP_SYNCIP_SYNC,	/* 20 */
	TOP_SYNCIP_FREE,	/* 21 */
	TOP_SBWRITE_RECLAIM,	/* 22 */
	TOP_SBWRITE_STABLE,	/* 23 */
	TOP_IFREE,		/* 24 */
	TOP_IUPDAT,		/* 25 */
	TOP_MOUNT,		/* 26 */
	TOP_COMMIT_ASYNC,	/* 27 */
	TOP_COMMIT_FLUSH,	/* 28 */
	TOP_COMMIT_UPDATE,	/* 29 */
	TOP_COMMIT_UNMOUNT,	/* 30 */
	TOP_SETSECATTR,		/* 31 */
	TOP_QUOTA,		/* 32 */
	TOP_ITRUNC,		/* 33 */
	TOP_CHECKPOINT,		/* 34 */
	TOP_MAX			/* 35 */
} top_t;

#ifdef LUFS
struct inode;
struct qfsvfs;
#else
struct sam_node;
struct sam_mount;
#endif /* LUFS */

/*
 * incore log address == NULL means not logging
 */
#define	TRANS_ISTRANS(qfsvfsp)	(LQFS_GET_LOGP(qfsvfsp))

/*
 * begin a synchronous transaction
 */
#define	TRANS_BEGIN_SYNC(qfsvfsp, vid, vsize, error)		\
{								\
	if (TRANS_ISTRANS(qfsvfsp)) {				\
		error = 0;					\
		top_begin_sync(qfsvfsp, vid, vsize, &error);	\
	}							\
}

/*
 * begin a asynchronous transaction
 */
#define	TRANS_BEGIN_ASYNC(qfsvfsp, vid, vsize)			\
{								\
	if (TRANS_ISTRANS(qfsvfsp)) {				\
		(void) top_begin_async(qfsvfsp, vid, vsize, 0);	\
	}							\
}

/*
 * try to begin a asynchronous transaction
 */
#define	TRANS_TRY_BEGIN_ASYNC(qfsvfsp, vid, vsize, err)		\
{								\
	if (TRANS_ISTRANS(qfsvfsp)) {				\
		err = top_begin_async(qfsvfsp, vid, vsize, 1);	\
	} else {						\
		err = 0;					\
	}							\
}

/*
 * Begin a synchronous or asynchronous transaction.
 * QFS does not support the filesystem mount option 'syncdir'.
 * The lint case is needed because vsize can be a constant.
 */
#ifdef LUFS
#ifndef __lint

#define	TRANS_BEGIN_CSYNC(qfsvfsp, issync, vid, vsize)			\
{									\
	if (TRANS_ISTRANS(qfsvfsp)) {					\
		if (qfsvfsp->vfs_syncdir) {				\
			int error = 0;					\
			ASSERT(vsize);					\
			top_begin_sync(qfsvfsp, vid, vsize, &error);	\
			ASSERT(error == 0);				\
			issync = 1;					\
		} else {						\
			(void) top_begin_async(qfsvfsp, vid, vsize, 0);	\
			issync = 0;					\
		}							\
	}								\
}

#else /* __lint */

#define	TRANS_BEGIN_CSYNC(qfsvfsp, issync, vid, vsize)			\
{									\
	if (TRANS_ISTRANS(qfsvfsp)) {					\
		if (qfsvfsp->vfs_syncdir) {				\
			int error = 0;					\
			top_begin_sync(qfsvfsp, vid, vsize, &error);	\
			issync = 1;					\
		} else {						\
			(void) top_begin_async(qfsvfsp, vid, vsize, 0);	\
			issync = 0;					\
		}							\
	}								\
}
#endif /* __lint */
#else
#define	VFS_SYNCDIR	0
#ifndef __lint

#define	TRANS_BEGIN_CSYNC(qfsvfsp, issync, vid, vsize)			\
{									\
	if (TRANS_ISTRANS(qfsvfsp)) {					\
		if (VFS_SYNCDIR) {					\
			int error = 0;					\
			ASSERT(vsize);					\
			top_begin_sync(qfsvfsp, vid, vsize, &error);	\
			ASSERT(error == 0);				\
			issync = 1;					\
		} else {						\
			(void) top_begin_async(qfsvfsp, vid, vsize, 0);	\
			issync = 0;					\
		}							\
	}								\
}

#else /* __lint */

#define	TRANS_BEGIN_CSYNC(qfsvfsp, issync, vid, vsize)			\
{									\
	if (TRANS_ISTRANS(qfsvfsp)) {					\
		if (VFS_SYNCDIR) {					\
			int error = 0;					\
			top_begin_sync(qfsvfsp, vid, vsize, &error);	\
			issync = 1;					\
		} else {						\
			(void) top_begin_async(qfsvfsp, vid, vsize, 0); \
			issync = 0;					\
		}							\
	}								\
}
#endif /* __lint */
#endif /* LUFS */

/*
 * try to begin a synchronous or asynchronous transaction
 */

#ifdef LUFS
#define	TRANS_TRY_BEGIN_CSYNC(qfsvfsp, issync, vid, vsize, error)	\
{									\
	if (TRANS_ISTRANS(qfsvfsp)) {					\
		if (qfsvfsp->vfs_syncdir) {				\
			ASSERT(vsize);					\
			top_begin_sync(qfsvfsp, vid, vsize, &error);	\
			ASSERT(error == 0);				\
			issync = 1;					\
		} else {						\
			error = top_begin_async(qfsvfsp, vid, vsize, 1); \
			issync = 0;					\
		}							\
	}								\
}
#else
#define	TRANS_TRY_BEGIN_CSYNC(qfsvfsp, issync, vid, vsize, error)	\
{									\
	if (TRANS_ISTRANS(qfsvfsp)) {					\
		if (VFS_SYNCDIR) {					\
			ASSERT(vsize);					\
			top_begin_sync(qfsvfsp, vid, vsize, &error);	\
			ASSERT(error == 0);				\
			issync = 1;					\
		} else {						\
			error = top_begin_async(qfsvfsp, vid, vsize, 1); \
			issync = 0;					\
		}							\
	}								\
}
#endif /* LUFS */


/*
 * end a asynchronous transaction
 */
#define	TRANS_END_ASYNC(qfsvfsp, vid, vsize)		\
{							\
	if (TRANS_ISTRANS(qfsvfsp)) {			\
		top_end_async(qfsvfsp, vid, vsize);	\
	}						\
}

/*
 * end a synchronous transaction
 */
#define	TRANS_END_SYNC(qfsvfsp, error, vid, vsize)		\
{								\
	if (TRANS_ISTRANS(qfsvfsp)) {				\
		top_end_sync(qfsvfsp, &error, vid, vsize);	\
	}							\
}

/*
 * end a synchronous or asynchronous transaction
 */
#define	TRANS_END_CSYNC(qfsvfsp, error, issync, vid, vsize)		\
{									\
	if (TRANS_ISTRANS(qfsvfsp)) {					\
		if (issync) {						\
			top_end_sync(qfsvfsp, &error, vid, vsize);	\
		} else {						\
			top_end_async(qfsvfsp, vid, vsize);		\
		}							\
	}								\
}
/*
 * record a delta
 */
#define	TRANS_DELTA(qfsvfsp, mof, ord, nb, dtyp, func, arg)	\
	if (TRANS_ISTRANS(qfsvfsp)) {				\
		top_delta(qfsvfsp, (offset_t)(mof), ord, nb, dtyp, func, arg); \
	}

/*
 * cancel a delta
 */
#define	TRANS_CANCEL(qfsvfsp, mof, ord, nb, flags)	\
	if (TRANS_ISTRANS(qfsvfsp)) {			\
		top_cancel(qfsvfsp, (offset_t)(mof), ord, nb, flags); \
	}
/*
 * log a delta
 */
#define	TRANS_LOG(qfsvfsp, va, mof, ord, nb, buf, bufsz)	\
	if (TRANS_ISTRANS(qfsvfsp)) {				\
		top_log(qfsvfsp, va, (offset_t)(mof), ord, nb, buf, bufsz); \
	}
/*
 * check if a range is being canceled (converting from metadata into userdata)
 */
#define	TRANS_ISCANCEL(qfsvfsp, mof, ord, nb)	\
	((TRANS_ISTRANS(qfsvfsp)) ?		\
		top_iscancel(qfsvfsp, (offset_t)(mof), ord, nb) : 0)
/*
 * put the log into error state
 */
#define	TRANS_SETERROR(qfsvfsp)		\
	if (TRANS_ISTRANS(qfsvfsp)) {	\
		top_seterror(qfsvfsp);	\
	}
/*
 * check if device has had an error
 */
#define	TRANS_ISERROR(qfsvfsp)					\
	((TRANS_ISTRANS(qfsvfsp)) ?				\
		(qfsvfsp)->vfs_log->un_flags & LDL_ERROR : 0)

/*
 * The following macros provide a more readable interface to TRANS_DELTA
 */
#define	TRANS_BUF(qfsvfsp, vof, ord, nb, bp, type)			\
	TRANS_DELTA(qfsvfsp,						\
		ldbtob(bp->b_blkno) + (offset_t)(vof), ord, nb, type,	\
		qfs_trans_push_buf, bp->b_blkno)

#define	TRANS_BUF_ITEM_128(qfsvfsp, item, base, ord, bp, type)		\
	TRANS_BUF(qfsvfsp,						\
		(((uintptr_t)&(item)) & ~(128 - 1)) - (uintptr_t)(base), ord, \
		128, bp, type)

/*
 * #define	TRANS_INODE(qfsvfsp, ip) \
 *	TRANS_DELTA(qfsvfsp, ip->i_doff, sizeof (struct dinode), \
 *			DT_INODE, qfs_trans_push_inode, ip->i_number)
 */

#define	TRANS_INODE(qfsvfsp, ip)				\
	TRANS_DELTA(qfsvfsp, ip->doff, ip->dord,		\
			sizeof (struct sam_perm_inode),		\
			DT_INODE, qfs_trans_push_inode,		\
			((((uint64_t)(ip->di.id.ino)) << 32) |	\
				ip->di.id.gen))

/*
 * If ever parts of an inode except the timestamps are logged using
 * this macro (or any other technique), bootloader logging support must
 * be made aware of these changes.
 */

/*
 * #define	TRANS_INODE_DELTA(qfsvfsp, vof, nb, ip) \
 *	TRANS_DELTA(qfsvfsp, (ip->i_doff + (offset_t)(vof)), \
 *		nb, DT_INODE, qfs_trans_push_inode, ip->i_number)
 */

#define	TRANS_INODE_DELTA(qfsvfsp, vof, ord, nb, ip)			\
	TRANS_DELTA(qfsvfsp, (ip->doff + (offset_t)(vof)),		\
		ip->dord, nb, DT_INODE, qfs_trans_push_inode,		\
		((((uint64_t)(ip->di.id.ino)) << 32) | ip->di.id.gen))

#define	TRANS_INODE_TIMES(qfsvfsp, ip)					\
	TRANS_INODE_DELTA(qfsvfsp, (caddr_t)&ip->i_atime - (caddr_t)&ip->i_ic, \
		sizeof (struct timeval32) * 3, ip)

#define	TRANS_DISK_INODE(qfsvfsp, iid, doff, dord)			\
	TRANS_DELTA(qfsvfsp, doff, dord, sizeof (struct sam_perm_inode), \
			DT_INODE, qfs_trans_push_ext_inode,		\
			((((uint64_t)(iid.ino)) << 32) | iid.gen))

#define	TRANS_WRITE_DISK_INODE(qfsvfsp, bp, iip, iid)			\
	{								\
		offset_t doff;						\
		uchar_t dord;						\
									\
		dord = lqfs_find_ord(qfsvfsp, bp);			\
		doff = ldbtob(bp->b_blkno) + (SAM_ISIZE *		\
		    ((iid.ino - 1) & (INO_BLK_FACTOR - 1)));		\
		TRANS_DISK_INODE(qfsvfsp, iid, doff, dord);		\
		TRANS_LOG(qfsvfsp, (caddr_t)iip, doff,			\
		    dord, sizeof (struct sam_perm_inode),		\
		    (caddr_t)P2ALIGN((uintptr_t)iip, DEV_BSIZE),	\
		    DEV_BSIZE);						\
		brelse(bp);						\
	}


/*
 * Check if we need to log cylinder group summary info.
 * NOTE: RJS - ord 0 for compile.
 */
#define	TRANS_SI(qfsvfsp, fs, cg) \
	if (TRANS_ISTRANS(qfsvfsp)) {					\
		if (LQFS_GET_NOLOG_SI(qfsvfsp)) {			\
			fs->fs_si = FS_SI_BAD;				\
		} else {						\
			TRANS_DELTA(qfsvfsp,				\
				ldbtob(fsbtodb(fs, fs->fs_csaddr)) +	\
				((caddr_t)&fs->fs_cs(fs, cg) -		\
				(caddr_t)fs->fs_u.fs_csp),		\
				0,					\
				sizeof (struct csum), DT_SI,		\
				qfs_trans_push_si, cg)			\
		}							\
	}

#ifdef LUFS
#define	TRANS_DIR(ip, offset)						\
	(TRANS_ISTRANS(ip->i_qfsvfs) ? qfs_trans_dir(ip, offset) : 0)
#else
#define	TRANS_DIR(ip, offset)						\
	if (TRANS_ISTRANS(ip->mp)) {					\
		qfs_trans_dir((ip), (offset));				\
	} else {							\
		while (0) { }						\
	}
#endif /* LUFS */

#define	TRANS_QUOTA(dqp)			\
	if (TRANS_ISTRANS(dqp->dq_qfsvfsp)) {	\
		qfs_trans_quota(dqp);		\
	}

#define	TRANS_DQRELE(qfsvfsp, dqp)				\
	if (TRANS_ISTRANS(qfsvfsp) &&				\
	    ((curthread->t_flag & T_DONTBLOCK) == 0)) {		\
		qfs_trans_dqrele(dqp);				\
	} else {						\
		rw_enter(&qfsvfsp->vfs_dqrwlock, RW_READER);	\
		dqrele(dqp);					\
		rw_exit(&qfsvfsp->vfs_dqrwlock);		\
	}

#define	TRANS_ITRUNC(ip, length, flags, cr)	\
	qfs_trans_itrunc(ip, length, flags, cr)

#ifdef LUFS
#define	TRANS_WRITE_RESV(ip, uiop, ulp, resvp, residp)	\
	if ((TRANS_ISTRANS(ip->i_qfsvfs) != NULL) && (ulp != NULL)) {	\
		qfs_trans_write_resv(ip, uiop, resvp, residp);		\
	}

#define	TRANS_WRITE(ip, uiop, ioflag, err, ulp, cr, resv, resid)	\
	if ((TRANS_ISTRANS(ip->i_qfsvfs) != NULL) && (ulp != NULL)) {	\
		err = qfs_trans_write(ip, uiop, ioflag, cr, resv, resid); \
	} else {							\
		err = wrip(ip, uiop, ioflag, cr);			\
	}
#else
#define	TRANS_WRITE_RESV(ip, uiop, ulp, resvp, residp)		\
	if (TRANS_ISTRANS(ip->mp) != NULL) {			\
		qfs_trans_write_resv(ip, uiop, resvp, residp);	\
	}

#define	TRANS_WRITE(ip, uiop, ioflag, err, ulp, cr, resv, resid)	\
	if (TRANS_ISTRANS(ip->mp) != NULL) {				\
		err = qfs_trans_write(ip, uiop, ioflag, cr, resv, resid); \
	} else {							\
		err = wrip(ip, uiop, ioflag, cr);			\
	}
#endif /* LUFS */

/*
 * These functions "wrap" functions that are not VOP or VFS
 * entry points but must still use the TRANS_BEGIN/TRANS_END
 * protocol
 */
#define	TRANS_SBUPDATE(qfsvfsp, vfsp, topid) \
		qfs_trans_sbupdate((qfsvfsp), (vfsp), (topid))
#define	TRANS_SYNCIP(ip, bflags, iflag, topid) \
		qfs_syncip((ip), (bflags), (iflag), (topid))
#define	TRANS_SBWRITE(qfsvfsp, topid) \
		qfs_trans_sbwrite((qfsvfsp), (topid))
#define	TRANS_IUPDAT(ip, waitfor) \
		qfs_trans_iupdat((ip), (waitfor))

#ifdef	DEBUG
/*
 * Test/Debug ops
 *	The following ops maintain the metadata map.
 *	The metadata map is a debug/test feature.
 *	These ops are *not* used in the production product.
 */

/*
 * Set a flag if meta data checking.
 */
#define	TRANS_DOMATAMAP(qfsvfsp)					\
		if (TRANS_ISTRANS((qfsvfsp))) {				\
			(qfsvfsp)->vfs_domatamap =			\
				(TRANS_ISTRANS((qfsvfsp)) &&		\
				((qfsvfsp)->vfs_log->un_debug & MT_MATAMAP)); \
		}

#define	TRANS_MATA_IGET(qfsvfsp, ip)					\
	if (TRANS_ISTRANS((qfsvfsp))) {					\
		if ((qfsvfsp)->vfs_domatamap) {				\
			qfs_trans_mata_iget((ip));			\
		}							\
	}

#define	TRANS_MATA_FREE(qfsvfsp, mof, ord, nb)				\
	if (TRANS_ISTRANS((qfsvfsp))) {					\
		if ((qfsvfsp)->vfs_domatamap) {				\
			qfs_trans_mata_free((qfsvfsp), (offset_t)(mof),	\
				(ord), (nb));				\
		}							\
	}

#define	TRANS_MATA_ALLOC(qfsvfsp, ip, bno, size, zero)			\
	if (TRANS_ISTRANS((qfsvfsp))) {					\
		if ((qfsvfsp)->vfs_domatamap) {				\
		    qfs_trans_mata_alloc((qfsvfsp), (ip), (bno),	\
					(size), (zero));		\
		}							\
	}

#define	TRANS_MATA_MOUNT(qfsvfsp)					\
	if (TRANS_ISTRANS((qfsvfsp))) {					\
		if ((qfsvfsp)->vfs_domatamap) {				\
			qfs_trans_mata_mount((qfsvfsp));		\
		}							\
	}

#define	TRANS_MATA_UMOUNT(qfsvfsp)					\
	if (TRANS_ISTRANS((qfsvfsp))) {					\
		if ((qfsvfsp)->vfs_domatamap) {				\
			qfs_trans_mata_umount((qfsvfsp));		\
		}							\
	}

#define	TRANS_MATA_SI(qfsvfsp, fs)					\
	if (TRANS_ISTRANS((qfsvfsp))) {					\
		if ((qfsvfsp)->vfs_domatamap) {				\
			qfs_trans_mata_si((qfsvfsp), (fs));		\
		}							\
	}

#define	TRANS_MATAADD(qfsvfsp, mof, ord, nb)				\
	if (TRANS_ISTRANS((qfsvfsp))) {					\
		top_mataadd((qfsvfsp), (offset_t)(mof), (ord), (nb));	\
	}

#else

#define	TRANS_DOMATAMAP(qfsvfsp)
#define	TRANS_MATA_IGET(qfsvfsp, ip)
#define	TRANS_MATA_FREE(qfsvfsp, mof, ord, nb)
#define	TRANS_MATA_ALLOC(qfsvfsp, ip, bno, size, zero)
#define	TRANS_MATA_MOUNT(qfsvfsp)
#define	TRANS_MATA_UMOUNT(qfsvfsp)
#define	TRANS_MATA_SI(qfsvfsp, fs)
#define	TRANS_MATAADD(qfsvfsp, mof, ord, nb)

#endif  /* DEBUG */

#ifdef LUFS
#include	<sys/fs/qfs_quota.h>
#include	<sys/fs/qfs_lockfs.h>
#endif /* LUFS */
/*
 * identifies the type of operation passed into TRANS_BEGIN/END
 */
#define	TOP_SYNC		(0x00000001)
#define	TOP_ASYNC		(0x00000002)
#define	TOP_SYNC_FORCED		(0x00000004)	/* forced sync transaction */
/*
 *  estimated values
 */
#define	HEADERSIZE		(128)
#define	ALLOCSIZE		(160)
#ifdef LUFS
#define	INODESIZE		(sizeof (struct dinode) + HEADERSIZE)
#define	SIZESB			((sizeof (struct fs)) + HEADERSIZE)
#else
#define	INODESIZE		(sizeof (struct sam_perm_inode) + HEADERSIZE)
#define	SIZESB			((sizeof (struct sam_sblk)) + HEADERSIZE)
#define	DIRBLKSIZ		(DIR_BLK)
#define	SIZEDIR			(DIRBLKSIZ + HEADERSIZE)
#endif /* LUFS */
/*
 * calculated values
 */
#ifdef LUFS
#define	SIZECG(IP)		((IP)->i_fs->fs_cgsize + HEADERSIZE)
#define	FRAGSIZE(IP)		((IP)->i_fs->fs_fsize + HEADERSIZE)
#define	MAXACLSIZE		((MAX_ACL_ENTRIES << 1) * sizeof (aclent_t))
#define	ACLSIZE(IP)		(((IP)->i_qfsvfs->vfs_maxacl + HEADERSIZE) + \
					INODESIZE)
#define	DIRSIZE(IP)		(INODESIZE + (4 * ALLOCSIZE) + \
				    (IP)->i_fs->fs_fsize + HEADERSIZE)
#else
/* QFS DOESN'T SUPPORT CYLINDER GROUPS */
#define	SIZECG(IP)		(0)

#define	FRAGSIZE(IP)		(FS_FSIZE((IP)->mp) + HEADERSIZE)
/*
 * QFS WORK TO DO - ACLs
 * #define	MAXACLSIZE	((MAX_ACL_ENTRIES << 1) * \
 *				    sizeof (sam_acl_t))
 * #define	ACLSIZE(IP)	(((((MAX_ACL_ENTRIES / 38) + 1) * \
 *				512) + HEADERSIZE) + INODESIZE)
 */
#define		MAXACLSIZE	(0)
#define		ACLSIZE(IP)	(0)

/*
 * QFS WORK TO DO - EXTENDED ATTRIBUTES
 */
#define	DIRSIZE(IP)		(INODESIZE + (4 * ALLOCSIZE) + \
				    (DIR_BLK) + HEADERSIZE)
#endif /* LUFS */

#ifdef LUFS
#define	QUOTASIZE		sizeof (struct dquot) + HEADERSIZE
#else
#define	QUOTASIZE		(0)
#endif /* LUFS */
/*
 * size calculations
 */
#define	TOP_CREATE_SIZE(IP)	\
	(ACLSIZE(IP) + SIZECG(IP) + DIRSIZE(IP) + INODESIZE)
#define	TOP_REMOVE_SIZE(IP)	\
	DIRSIZE(IP)  + SIZECG(IP) + INODESIZE + SIZESB
#define	TOP_LINK_SIZE(IP)	\
	DIRSIZE(IP) + INODESIZE
#define	TOP_RENAME_SIZE(IP)	\
	DIRSIZE(IP) + DIRSIZE(IP) + SIZECG(IP)
#define	TOP_MKDIR_SIZE(IP)	\
	DIRSIZE(IP) + INODESIZE + DIRSIZE(IP) + INODESIZE + FRAGSIZE(IP) + \
	    SIZECG(IP) + ACLSIZE(IP)
#define	TOP_SYMLINK_SIZE(IP)	\
	DIRSIZE((IP)) + INODESIZE + INODESIZE + SIZECG(IP)
#define	TOP_GETPAGE_SIZE(IP)	\
	ALLOCSIZE + ALLOCSIZE + ALLOCSIZE + INODESIZE + SIZECG(IP)
#define	TOP_SYNCIP_SIZE		INODESIZE
#define	TOP_READ_SIZE		INODESIZE
#define	TOP_RMDIR_SIZE		(SIZESB + (INODESIZE * 2) + SIZEDIR)
#define	TOP_SETQUOTA_SIZE(FS)	((FS)->fs_bsize << 2)
#define	TOP_QUOTA_SIZE		(QUOTASIZE)
#define	TOP_SETSECATTR_SIZE(IP)	(MAXACLSIZE)
#define	TOP_IUPDAT_SIZE(IP)	INODESIZE + SIZECG(IP)
#define	TOP_SBUPDATE_SIZE	(SIZESB)
#define	TOP_SBWRITE_SIZE	(SIZESB)
#define	TOP_PUTPAGE_SIZE(IP)	(INODESIZE + SIZECG(IP))
#define	TOP_SETATTR_SIZE(IP)	(SIZECG(IP) + INODESIZE + QUOTASIZE + \
		ACLSIZE(IP))
#define	TOP_IFREE_SIZE(IP)	(SIZECG(IP) + INODESIZE + QUOTASIZE)
#define	TOP_MOUNT_SIZE		(SIZESB)
#define	TOP_COMMIT_SIZE		(0)
#define	TOP_CHECKPOINT_SIZE(blksz) \
				((blksz) + (INODESIZE * 3) + (SIZESB))

/*
 * The minimum log size is 1M.  So we will allow 1 fs operation to
 * reserve at most 512K of log space.
 */
#define	TOP_MAX_RESV	(512 * 1024)


/*
 * qfs trans function prototypes
 */
#if defined(_KERNEL) && defined(__STDC__)

#ifdef LUFS
extern int	qfs_trans_hlock();
extern void	qfs_trans_onerror();
extern int	qfs_trans_push_inode(struct qfsvfs *, delta_t, uint64_t);
extern int	qfs_trans_push_buf(struct qfsvfs *, delta_t, daddr_t);
extern int	qfs_trans_push_si(struct qfsvfs *, delta_t, int);
extern void	qfs_trans_sbupdate(struct qfsvfs *, struct vfs *, top_t);
extern void	qfs_trans_sbwrite(struct qfsvfs *, top_t);
extern void	qfs_trans_iupdat(struct inode *, int);
extern void	qfs_trans_mata_mount(struct qfsvfs *);
extern void	qfs_trans_mata_umount(struct qfsvfs *);
extern void	qfs_trans_mata_si(struct qfsvfs *, struct fs *);
extern void	qfs_trans_mata_iget(struct inode *);
extern void	qfs_trans_mata_free(struct qfsvfs *, offset_t, uchar_t, off_t);
extern void	qfs_trans_mata_alloc(struct qfsvfs *, struct inode *,
			daddr_t, ulong_t, int);
extern int	qfs_trans_dir(struct inode *, off_t);
extern void	qfs_trans_quota(struct dquot *);
extern void	qfs_trans_dqrele(struct dquot *);
extern int	qfs_trans_itrunc(struct inode *, u_offset_t, int, cred_t *);
extern int	qfs_trans_write(struct inode *, struct uio *, int,
			cred_t *, int, long);
extern void	qfs_trans_write_resv(struct inode *, struct uio *,
			int *, int *);
extern int	qfs_trans_check(dev_t);
extern void	qfs_trans_redev(dev_t odev, dev_t ndev);

/*
 * transaction prototypes
 */
void	lqfs_unsnarf(struct qfsvfs *qfsvfsp);
int	lqfs_snarf(struct qfsvfs *qfsvfsp, struct fs *fs, int ronly);
void	top_delta(struct qfsvfs *qfsvfsp, offset_t mof, uchar_t ord, off_t nb,
		delta_t dtyp, int (*func)(), uint64_t arg);
void	top_cancel(struct qfsvfs *qfsvfsp, offset_t mof, uchar_t ord, off_t nb,
		int flags);
int	top_iscancel(struct qfsvfs *qfsvfsp, offset_t mof, uchar_t ord,
		off_t nb);
void	top_seterror(struct qfsvfs *qfsvfsp);
int	top_iserror(struct qfsvfs *qfsvfsp);
void	top_begin_sync(struct qfsvfs *qfsvfsp, top_t topid, ulong_t size,
	    int *error);
int	top_begin_async(struct qfsvfs *qfsvfsp, top_t topid, ulong_t size,
	    int tryasync);
void	top_end_sync(struct qfsvfs *qfsvfsp, int *ep, top_t topid,
	    ulong_t size);
void	top_end_async(struct qfsvfs *qfsvfsp, top_t topid, ulong_t size);
void	top_log(struct qfsvfs *qfsvfsp, char *va, offset_t vamof, uchar_t ord,
		off_t nb, caddr_t buf, uint32_t bufsz);
void	top_mataadd(struct qfsvfs *qfsvfsp, offset_t mof, uchar_t ord,
		off_t nb);
void	top_matadel(struct qfsvfs *qfsvfsp, offset_t mof, uchar_t ord,
		off_t nb);
void	top_mataclr(struct qfsvfs *qfsvfsp);
#else
extern int	qfs_trans_hlock();
extern void	qfs_trans_onerror();
extern int	qfs_trans_push_inode(struct sam_mount *, delta_t, uint64_t);
extern int	qfs_trans_push_ext_inode(struct sam_mount *, delta_t, uint64_t);
extern int	qfs_trans_push_buf(struct sam_mount *, delta_t, daddr_t);
extern int	qfs_trans_push_si(struct sam_mount *, delta_t, int);
extern void	qfs_trans_sbupdate(struct sam_mount *, struct vfs *, top_t);
extern void	qfs_trans_sbwrite(struct sam_mount *, top_t);
extern void	qfs_trans_iupdat(struct sam_node *, int);
extern void	qfs_trans_mata_mount(struct sam_mount *);
extern void	qfs_trans_mata_umount(struct sam_mount *);
extern void	qfs_trans_mata_si(struct sam_mount *, struct sam_mount *);
extern void	qfs_trans_mata_iget(struct sam_node *);
extern void	qfs_trans_mata_free(struct sam_mount *, offset_t,
			uchar_t, off_t);
extern void	qfs_trans_mata_alloc(struct sam_mount *, struct sam_node *,
			daddr_t, ulong_t, int);
extern int	qfs_trans_dir(struct sam_node *, off_t);
#ifdef LQFS_TODO_QUOTAS
extern void	qfs_trans_quota(struct dquot *);
extern void	qfs_trans_dqrele(struct dquot *);
#endif /* LQFS_TODO_QUOTAS */
extern int	qfs_trans_itrunc(struct sam_node *, u_offset_t, int, cred_t *);
extern int	qfs_trans_write(struct sam_node *, struct uio *, int,
			cred_t *, int, long);
extern void	qfs_trans_write_resv(struct sam_node *, struct uio *,
			int *, int *);
extern int	qfs_trans_check(dev_t);
extern void	qfs_trans_redev(dev_t odev, dev_t ndev);

/*
 * transaction prototypes
 */
void	lqfs_unsnarf(struct sam_mount *qfsvfsp);
int	lqfs_snarf(struct sam_mount *qfsvfsp, struct sam_mount *fs, int ronly);
void	top_delta(struct sam_mount *qfsvfsp, offset_t mof, uchar_t ord,
		off_t nb, delta_t dtyp, int (*func)(), uint64_t arg);
void	top_cancel(struct sam_mount *qfsvfsp, offset_t mof, uchar_t ord,
		off_t nb, int flags);
int	top_iscancel(struct sam_mount *qfsvfsp, offset_t mof, uchar_t ord,
		off_t nb);
void	top_seterror(struct sam_mount *qfsvfsp);
int	top_iserror(struct sam_mount *qfsvfsp);
void	top_begin_sync(struct sam_mount *qfsvfsp, top_t topid, ulong_t size,
	    int *error);
int	top_begin_async(struct sam_mount *qfsvfsp, top_t topid, ulong_t size,
	    int tryasync);
void	top_end_sync(struct sam_mount *qfsvfsp, int *ep, top_t topid,
	    ulong_t size);
void	top_end_async(struct sam_mount *qfsvfsp, top_t topid, ulong_t size);
void	top_log(struct sam_mount *qfsvfsp, char *va, offset_t vamof,
		uchar_t ord, off_t nb, caddr_t buf, uint32_t bufsz);
void	top_mataadd(struct sam_mount *qfsvfsp, offset_t mof, uchar_t ord,
		off_t nb);
void	top_matadel(struct sam_mount *qfsvfsp, offset_t mof, uchar_t ord,
		off_t nb);
void	top_mataclr(struct sam_mount *qfsvfsp);
int	qfs_syncip(sam_node_t *ip, int flags, int waitfor, top_t topid);
int	qfs_scan_inodes(int rwtry, int (*func)(sam_node_t *, void *),
		void *arg, sam_mount_t *mp);
int	qfs_flush_inode(sam_node_t *ip, void *arg);
#endif /* LUFS */


#endif	/* defined(_KERNEL) && defined(__STDC__) */

/*
 * #ifdef	__cplusplus
 * }
 * #endif
 */

#endif	/* _SYS_FS_QFS_TRANS_H */
