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

/* Copyright (c) 1983, 1984, 1985, 1986, 1987, 1988, 1989 AT&T */
/* All Rights Reserved */

/*
 * Portions of this source code were derived from Berkeley 4.3 BSD
 * under license from the Regents of the University of California.
 */

#ifdef sun
#pragma ident	"$Revision: 1.8 $"
#endif

#include <sys/sysmacros.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/t_lock.h>
#include <sys/uio.h>
#include <sys/kmem.h>
#include <sys/thread.h>
#include <sys/vfs.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#define	_LQFS_INFRASTRUCTURE
#include <lqfs_common.h>
#include <extern.h>
#undef _LQFS_INFRASTRUCTURE
#ifdef LUFS
#include <sys/fs/qfs_trans.h>
#include <sys/fs/qfs_inode.h>
#include <sys/fs/qfs_fs.h>
#include <sys/fs/qfs_fsdir.h>
#include <sys/fs/qfs_quota.h>
#include <sys/fs/qfs_panic.h>
#include <sys/fs/qfs_bio.h>
#include <sys/fs/qfs_log.h>
#else
#include <sys/stat.h>
#endif /* LUFS */
#include <sys/cmn_err.h>
#include <sys/file.h>
#include <sys/debug.h>


extern kmutex_t qfsvfs_mutex;
extern qfsvfs_t *qfs_instances;

extern void (*bio_lqfs_strategy)(void *, buf_t *);

/*
 * hlock any file systems w/errored logs
 */
int
qfs_trans_hlock()
{
#ifdef LQFS_TODO_LOCKFS
	qfsvfs_t	*qfsvfsp;
	struct lockfs	lockfs;
	int		error;
	int		retry	= 0;

	/*
	 * find fs's that paniced or have errored logging devices
	 */
	mutex_enter(&qfsvfs_mutex);
	for (qfsvfsp = qfs_instances; qfsvfsp; qfsvfsp = qfsvfsp->vfs_next) {
		/*
		 * not mounted; continue
		 */
		if ((qfsvfsp->vfs_vfs == NULL) ||
		    (qfsvfsp->vfs_validfs == UT_UNMOUNTED)) {
			continue;
		}
		/*
		 * disallow unmounts (hlock occurs below)
		 */
		if (TRANS_ISERROR(qfsvfsp)) {
			qfsvfsp->vfs_validfs = UT_HLOCKING;
		}
	}
	mutex_exit(&qfsvfs_mutex);

	/*
	 * hlock the fs's that paniced or have errored logging devices
	 */
again:
	mutex_enter(&qfsvfs_mutex);
	for (qfsvfsp = qfs_instances; qfsvfsp; qfsvfsp = qfsvfsp->vfs_next) {
		if (qfsvfsp->vfs_validfs == UT_HLOCKING) {
			break;
		}
	}
	mutex_exit(&qfsvfs_mutex);
	if (qfsvfsp == NULL) {
		return (retry);
	}
	/*
	 * hlock the file system
	 */
	(void) qfs_fiolfss(qfsvfsp->vfs_root, &lockfs);
	if (!LOCKFS_IS_ELOCK(&lockfs)) {
		lockfs.lf_lock = LOCKFS_HLOCK;
		lockfs.lf_flags = 0;
		lockfs.lf_comlen = 0;
		lockfs.lf_comment = NULL;
		error = qfs_fiolfs(qfsvfsp->vfs_root, &lockfs, 0);
		/*
		 * retry after awhile; another app currently doing lockfs
		 */
		if (error == EBUSY || error == EINVAL) {
			retry = 1;
		}
	} else {
		if (qfsfx_get_failure_qlen() > 0) {
			if (mutex_tryenter(&qfs_fix.uq_mutex)) {
				qfs_fix.uq_lowat = qfs_fix.uq_ne;
				cv_broadcast(&qfs_fix.uq_cv);
				mutex_exit(&qfs_fix.uq_mutex);
			}
		}
		retry = 1;
	}

	/*
	 * allow unmounts
	 */
	qfsvfsp->vfs_validfs = UT_MOUNTED;
	goto again;
#else
	return (0);
#endif /* LQFS_TODO_LOCKFS */
}

/*ARGSUSED*/
void
qfs_trans_onerror()
{
#ifdef LQFS_TODO_LOCKFS
	mutex_enter(&qfs_hlock.uq_mutex);
	qfs_hlock.uq_ne = qfs_hlock.uq_lowat;
	cv_broadcast(&qfs_hlock.uq_cv);
	mutex_exit(&qfs_hlock.uq_mutex);
#endif /* LQFS_TODO_LOCKFS */
}

/* ARGSUSED1 */
void
qfs_trans_sbupdate(qfsvfs_t *qfsvfsp, struct vfs *vfsp, top_t topid)
{
	if (curthread->t_flag & T_DONTBLOCK) {
#ifdef LUFS
		sbupdate(vfsp);
#else
		sam_update_sblk(qfsvfsp, 0, 0, TRUE);
		sam_update_sblk(qfsvfsp, 0, 1, TRUE);
#endif /* LUFS */
		return;
	} else {

		if (panicstr && TRANS_ISTRANS(qfsvfsp))
			return;

		curthread->t_flag |= T_DONTBLOCK;
		TRANS_BEGIN_ASYNC(qfsvfsp, topid, TOP_SBUPDATE_SIZE);
#ifdef LUFS
		sbupdate(vfsp);
#else
		sam_update_sblk(qfsvfsp, 0, 0, TRUE);
		sam_update_sblk(qfsvfsp, 0, 1, TRUE);
#endif /* LUFS */
		TRANS_END_ASYNC(qfsvfsp, topid, TOP_SBUPDATE_SIZE);
		curthread->t_flag &= ~T_DONTBLOCK;
	}
}

/*ARGSUSED*/
void
qfs_trans_iupdat(inode_t *ip, int waitfor)
{
	qfsvfs_t	*qfsvfsp;

	if (curthread->t_flag & T_DONTBLOCK) {
		/*
		 * NOTE: UFS only requires an inode read lock.  We need
		 * a write lock to handle sam_acl_flush().  ???
		 */
		rw_enter(&ip->i_contents, RW_WRITER);
#ifdef LUFS
		qfs_iupdat(ip, waitfor);
#else
		sam_update_inode(ip, SAM_SYNC_ONE, FALSE);
#endif /* LUFS */
		rw_exit(&ip->i_contents);
		return;
	} else {
		qfsvfsp = ip->i_qfsvfs;

		if (panicstr && TRANS_ISTRANS(qfsvfsp)) {
			return;
		}

		curthread->t_flag |= T_DONTBLOCK;
		TRANS_BEGIN_ASYNC(qfsvfsp, TOP_IUPDAT, TOP_IUPDAT_SIZE(ip));
		/*
		 * NOTE: UFS ony requires inode read lock.  We need
		 * a write lock to handle sam_acl_flush().  ???
		 */
		rw_enter(&ip->i_contents, RW_WRITER);
#ifdef LUFS
		qfs_iupdat(ip, waitfor);
#else
		sam_update_inode(ip, SAM_SYNC_ONE, FALSE);
#endif /* LUFS */
		rw_exit(&ip->i_contents);
		TRANS_END_ASYNC(qfsvfsp, TOP_IUPDAT, TOP_IUPDAT_SIZE(ip));
		curthread->t_flag &= ~T_DONTBLOCK;
	}
}

void
qfs_trans_sbwrite(qfsvfs_t *qfsvfsp, top_t topid)
{
	if (curthread->t_flag & T_DONTBLOCK) {
#ifdef LUFS
		mutex_enter(&qfsvfsp->vfs_lock);
		qfs_sbwrite(qfsvfsp);
		mutex_exit(&qfsvfsp->vfs_lock);
#else
		sam_update_sblk(qfsvfsp, 0, 0, TRUE);
		sam_update_sblk(qfsvfsp, 0, 1, TRUE);
#endif /* LUFS */
		return;
	} else {

		if (panicstr && TRANS_ISTRANS(qfsvfsp)) {
			return;
		}

		curthread->t_flag |= T_DONTBLOCK;
		TRANS_BEGIN_ASYNC(qfsvfsp, topid, TOP_SBWRITE_SIZE);
#ifdef LUFS
		mutex_enter(&qfsvfsp->vfs_lock);
		qfs_sbwrite(qfsvfsp);
		mutex_exit(&qfsvfsp->vfs_lock);
#else
		sam_update_sblk(qfsvfsp, 0, 0, TRUE);
		sam_update_sblk(qfsvfsp, 0, 1, TRUE);
#endif /* LUFS */
		TRANS_END_ASYNC(qfsvfsp, topid, TOP_SBWRITE_SIZE);
		curthread->t_flag &= ~T_DONTBLOCK;
	}
}

/*ARGSUSED*/
int
qfs_trans_push_si(qfsvfs_t *qfsvfsp, delta_t dtyp, int ignore)
{
#ifdef LQFS_TODO_SYSTEM_INFO
	fs_lqfs_common_t *fs = VFS_FS_PTR(qfsvfsp);

	mutex_enter(&qfsvfsp->vfs_lock);
	TRANS_LOG(qfsvfsp, (char *)fs->fs_u.fs_csp,
	    ldbtob(fsbtodb(fs, fs->fs_csaddr)), fs->fs_cssize,
	    (caddr_t)fs->fs_u.fs_csp, fs->fs_cssize);
	mutex_exit(&qfsvfsp->vfs_lock);
#endif /* LQFS_TODO_SYSTEM_INFO */
	return (0);
}

/*ARGSUSED*/
int
qfs_trans_push_buf(qfsvfs_t *qfsvfsp, delta_t dtyp, daddr_t bno)
{
	struct buf	*bp;

	bp = (struct buf *)QFS_GETBLK(qfsvfsp, (VFS_PTR(qfsvfsp))->vfs_dev,
	    bno, 1);
	if (bp == NULL) {
		return (ENOENT);
	}

	if (bp->b_flags & B_DELWRI) {
		/*
		 * Do not use brwrite() here since the buffer is already
		 * marked for retry or not by the code that called
		 * TRANS_BUF().
		 */
		(void) sam_bwrite_noforcewait_dorelease(qfsvfsp, bp);
		return (0);
	}
	/*
	 * If we did not find the real buf for this block above then
	 * clear the dev so the buf won't be found by mistake
	 * for this block later.  We had to allocate at least a 1 byte
	 * buffer to keep brelse happy.
	 */
	if (bp->b_bufsize == 1) {
		bp->b_dev = (o_dev_t)NODEV;
		bp->b_edev = NODEV;
		bp->b_flags = 0;
	}
	brelse(bp);
	return (ENOENT);
}

/*ARGSUSED*/
int
qfs_trans_push_inode(qfsvfs_t *qfsvfsp, delta_t dtyp, uint64_t ino)
{
	int		error;
	inode_t	*ip;
	sam_id_t id;

	/*
	 * Grab the quota lock (if the file system has not been forcibly
	 * unmounted).
	 */
#ifdef LQFS_TODO_QUOTAS
	if (qfsvfsp) {
		rw_enter(&qfsvfsp->vfs_dqrwlock, RW_READER);
	}
#endif /* LQFS_TODO_QUOTAS */

#ifdef LUFS
	error = qfs_iget(qfsvfsp->vfs_vfs, ino, &ip, kcred);
#else
	id.ino = (ino >> 32);
	id.gen = (ino & UINT32_MAX);
	error = sam_get_ino(qfsvfsp->mi.m_vfsp, IG_EXISTS, &id, &ip);
#endif /* LUFS */

#ifdef LQFS_TODO_QUOTAS
	if (qfsvfsp) {
		rw_exit(&qfsvfsp->vfs_dqrwlock);
	}
#endif /* LQFS_TODO_QUOTAS */
	if (error) {
		return (ENOENT);
	}

#ifdef LUFS
	if (ip->i_flag & (IUPD|IACC|ICHG|IMOD|IMODACC|IATTCHG)) {
#else
	if (ip->flags.bits & SAM_UPDATE_FLAGS) {
#endif /* LUFS */
		/*
		 * NOTE: UFS ony requires inode read lock.  We need
		 * a write lock to handle sam_acl_flush().  ???
		 */
		rw_enter(&ip->i_contents, RW_WRITER);
#ifdef LUFS
		qfs_iupdat(ip, 1);
#else
		LQFS_MSG(CE_WARN, "qfs_trans_push_inode(): Updating inode %d\n",
		    ip->di.id.ino);
		sam_update_inode(ip, SAM_SYNC_ONE, FALSE);
#endif /* LUFS */
		rw_exit(&ip->i_contents);
		VN_RELE(SAM_ITOV(ip));
		return (0);
	}
	VN_RELE(SAM_ITOV(ip));
	return (ENOENT);
}

/*ARGSUSED*/
int
qfs_trans_push_ext_inode(qfsvfs_t *qfsvfsp, delta_t dtyp, uint64_t ino)
{
	int error;
	sam_id_t eid;
	buf_t *bp;
	struct sam_inode_ext *eip;

	eid.ino = (ino >> 32);
	eid.gen = (ino & UINT32_MAX);
	error = sam_read_ino(qfsvfsp, eid.ino, &bp,
	    (struct sam_perm_inode **)&eip);

	if (!error) {
		offset_t doff;
		uchar_t dord;

		dord = lqfs_find_ord(qfsvfsp, bp);
		doff = ldbtob(bp->b_blkno) + (SAM_ISIZE *
		    ((eid.ino - 1) & (INO_BLK_FACTOR - 1)));

		TRANS_LOG(qfsvfsp, (caddr_t)eip, doff, dord,
		    sizeof (struct sam_perm_inode),
		    (caddr_t)P2ALIGN((uintptr_t)eip, DEV_BSIZE), DEV_BSIZE);
		brelse(bp);
	}

	return (error);
}


#ifdef DEBUG
/*
 *	These routines maintain the metadata map (matamap)
 */

/*
 * update the metadata map at mount
 */
static int
qfs_trans_mata_mount_scan(inode_t *ip, void *arg)
{
	/*
	 * wrong file system; keep looking
	 */
	if (ip->i_qfsvfs != (qfsvfs_t *)arg) {
		return (0);
	}

	/*
	 * load the metadata map
	 */
	rw_enter(&ip->i_contents, RW_WRITER);
	qfs_trans_mata_iget(ip);
	rw_exit(&ip->i_contents);
	return (0);
}

void
qfs_trans_mata_mount(qfsvfs_t *qfsvfsp)
{
	fs_lqfs_common_t	*fs	= VFS_FS_PTR(qfsvfsp);
	uchar_t		ord = 0;

	/*
	 * put static metadata into matamap
	 *	superblock
	 *	cylinder groups
	 *	inode groups
	 *	existing inodes
	 */
	TRANS_MATAADD(qfsvfsp, ldbtob(SBLOCK_OFFSET(qfsvfsp)), ord,
	    fs->fs_sbsize);

#ifdef LQFS_TODO_CYLINDER_GROUPS
	{
		ino_t		ino;
		int		i;

		for (ino = i = 0; i < fs->fs_ncg; ++i, ino += fs->fs_ipg) {
			TRANS_MATAADD(qfsvfsp,
			    ldbtob(fsbtodb(fs, cgtod(fs, i))), ord,
			    fs->fs_cgsize);
			TRANS_MATAADD(qfsvfsp,
			    ldbtob(fsbtodb(fs, itod(fs, ino))), ord,
			    fs->fs_ipg * sizeof (struct dinode));
		}
	}
#else
	/* QFS doesn't define cylinder groups. */
#endif /* LQFS_TODO_CYLINDER_GROUPS */
	(void) qfs_scan_inodes(0, qfs_trans_mata_mount_scan, qfsvfsp, qfsvfsp);
}

/*
 * clear the metadata map at umount
 */
void
qfs_trans_mata_umount(qfsvfs_t *qfsvfsp)
{
	top_mataclr(qfsvfsp);
}

/*
 * summary info (may be extended during growfs test)
 */
/*ARGSUSED*/
void
qfs_trans_mata_si(qfsvfs_t *qfsvfsp, fs_lqfs_common_t *fs)
{
#ifdef LUFS
	int ord = 0;

	TRANS_MATAADD(qfsvfsp, ldbtob(fsbtodb(fs, fs->fs_csaddr)), ord,
	    fs->fs_cssize);
#else
	/* QFS doesn't support cylinder group summary. */
#endif /* LUFS */
}

/*
 * scan an allocation block (either inode or true block)
 */
static void
qfs_trans_mata_direct(
	inode_t *ip,
	daddr_t *fragsp,
	daddr32_t *blkp,
#ifdef LUFS
	unsigned int nblk)
#else
	uchar_t *ordp,		/* DAU ordinal number is passed-in for QFS. */
	unsigned int nblk,
	ushort_t fragsperdau)	/* Size of DAU (SM/LG) is passed-in for QFS. */
#endif /* LUFS */
{
	int		i;
	daddr_t		frag;
	ulong_t		nb;
	qfsvfs_t	*qfsvfsp	= ip->i_qfsvfs;
#ifdef LUFS
	fs_lqfs_common_t	*fs	= VFS_FS_PTR(qfsvfsp);
	unsigned short	fragsperdau	= 1;	/* Passed-in for QFS. */
#endif /* LUFS */

	for (i = 0; i < nblk && *fragsp; ++i, ++blkp, ++ordp)
		/*
		 * Write one DAU at-a-time, except excess frags.
		 * Skip NULL extent addresses.
		 */
		if ((frag = *blkp) != 0) {
#ifdef LUFS
			if (*fragsp > FS_FRAG(fs)) {
				nb = FS_BSIZE(fs);
				*fragsp -= FS_FRAG(fs);
#else
			/* Convert QFS 4K block address to frag */
			frag = fsblktologb(fs, frag);
			if (*fragsp > fragsperdau) {
				nb = (fragsperdau * FS_FSIZE(fs));
				*fragsp -= fragsperdau;
#endif /* LUFS */
			} else {
				nb = *fragsp * FS_FSIZE(fs);
				*fragsp = 0;
			}
			TRANS_MATAADD(qfsvfsp, ldbtob(fsbtodb(fs, frag)),
				*ordp, nb);
		}
}

/*
 * scan an indirect allocation block (either inode or true block)
 */
static void
qfs_trans_mata_indir(
	inode_t *ip,
	daddr_t *fragsp,
	daddr_t frag,
	uchar_t ord,
	int level)
{
	qfsvfs_t *qfsvfsp	= ip->i_qfsvfs;
#ifdef	LUFS
	fs_lqfs_common_t *fs = VFS_FS_PTR(qfsvfsp);
	int ne = FS_BSIZE(fs) / (int)sizeof (daddr32_t);
#else
	int ne = DEXT;	/* QFS has only 2048 (DEXT) ext/ord pairs per DAU */
	sam_indirect_extent_t	*iep;
	uchar_t *ordp;
#endif /* LUFS */
	int i;
	struct buf *bp;
	daddr32_t *blkp;
#ifdef LUFS
	o_mode_t ifmt = ip->i_mode & IFMT;
#else
	o_mode_t ifmt = ip->di.mode & S_IFMT;
#endif /* LUFS */

#ifdef LUFS
	bp = QFS_BREAD(qfsvfsp, ip->i_dev, fsbtodb(fs, frag), FS_BSIZE(fs));
#else
	/* QFS indirect extents are one large DAU in size. */
	sam_bread_db(qfsvfsp,
	    qfsvfsp->mi.m_fs[qfsvfsp->mi.m_sbp->eq[ord].fs.mm_ord].dev,
	    fsbtodb(ip->mp, frag),
	    ip->mp->mi.m_sbp->info.sb.mm_blks[LG] * FS_FSIZE(fs), &bp);
#endif /* LUFS */
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return;
	}

#ifdef LUFS
	blkp = bp->b_un.b_daddr;
	if (level || (ifmt == IFDIR) || (ifmt == IFSHAD) ||
	    (ifmt == IFATTRDIR) || (ip == ip->i_qfsvfs->vfs_qinod)) {
		qfs_trans_mata_direct(ip, fragsp, blkp, ne);
	}

	if (level) {
		for (i = 0; i < ne && *fragsp; ++i, ++blkp) {
			qfs_trans_mata_indir(ip, fragsp, *blkp, level-1);
		}
	}
#else
	iep = (sam_indirect_extent_t *)bp->b_un.b_daddr;
	blkp = (daddr32_t *)&iep->extent[0];
	ordp = &iep->extent_ord[0];
	if (level || (ifmt == S_IFDIR)) {
		/* QFS doesn't support shadow inodes. */
		/* LQFS_TODO_EXTENDED_ATTRS */
		/* LQFS_TODO_QUOTAS */
		/* QFS indirect extents identify large DAUs */
		qfs_trans_mata_direct(ip, fragsp, blkp, ordp, ne,
		    ip->mp->mi.m_sbp->info.sb.mm_blks[LG]);
	}

	if (level) {
		for (i = 0; i < ne && *fragsp; ++i, ++blkp, ++ordp) {
			qfs_trans_mata_indir(ip, fragsp, *blkp, *ordp, level-1);
		}
	}
#endif /* LUFS */
	brelse(bp);
}

/*
 * put appropriate metadata into matamap for this inode
 */
void
qfs_trans_mata_iget(inode_t *ip)
{
	int		i;
#ifdef LUFS
	daddr_t		frags	= dbtofsb(ip->i_fs, ip->i_blocks);
#else
	daddr_t		frags	= ip->i_blocks;
#endif /* LUFS */
	o_mode_t	ifmt 	= ip->i_mode & IFMT;

#ifdef LUFS
	if (frags && ((ifmt == IFDIR) || (ifmt == IFSHAD) ||
	    (ifmt == IFATTRDIR) || (ip == ip->i_qfsvfs->vfs_qinod))) {
		qfs_trans_mata_direct(ip, &frags, &ip->i_db[0], NDADDR);
	}

	if (frags) {
		qfs_trans_mata_direct(ip, &frags, &ip->i_ib[0], NIADDR);
	}

	for (i = 0; i < NIADDR && frags; ++i) {
		if (ip->i_ib[i]) {
			qfs_trans_mata_indir(ip, &frags, ip->i_ib[i], i);
		}
	}
#else
	if (frags && (ifmt == S_IFDIR)) {
		/* QFS doesn't support shadow inodes */
		/* LQFS_TODO_EXTENDED_ATTRS */
		/* LQFS_TODO_QUOTAS */
		/*
		 * Each of the first 8 QFS extent/ordinal pairs
		 * identifies a small (4K) DAU that contains file data.
		 */
		qfs_trans_mata_direct(ip, &frags,
		    (daddr32_t *)&ip->di.extent[0],
		    &ip->di.extent_ord[0], NSDEXT,
		    ip->mp->mi.m_sbp->info.sb.mm_blks[SM]);

		if (frags) {
			/*
			 * Each of the next 8 QFS extent/ordinal pairs
			 * identifies a large (default 64K, but configurable
			 * size) DAU that contains file data.
			 */
			qfs_trans_mata_direct(ip, &frags,
			    (daddr32_t *)&ip->di.extent[NSDEXT],
			    &ip->di.extent_ord[NSDEXT], NLDEXT,
			    ip->mp->mi.m_sbp->info.sb.mm_blks[LG]);
		}
	}

	if (frags) {
		/*
		 * The next 3 extent/ordinal pairs refer to level 1 thru 3
		 * indirect exts, respectively.  Each indirect extent is
		 * the size of a large DAU (64K default, but configurable
		 * size), and contains 2048 (DEXT) extent/ordinal pairs (yes,
		 * there is currently much wasted space).  Each of the 2048
		 * extent/ordinal pairs in the last indirect extent of
		 * each chain refers to a large DAU that contains file
		 * data.
		 */
		qfs_trans_mata_direct(ip, &frags,
		    (daddr32_t *)&ip->di.extent[NDEXT],
		    &ip->di.extent_ord[NDEXT], NIEXT,
		    ip->mp->mi.m_sbp->info.sb.mm_blks[LG]);
	}

	for (i = 0; i < NIEXT && frags; ++i) {
		if (ip->di.extent[NDEXT+i]) {
			qfs_trans_mata_indir(ip, &frags, ip->di.extent[NDEXT+i],
			    ip->di.extent_ord[NDEXT+i], i);
		}
	}
#endif /* LUFS */
}

/*
 * freeing possible metadata (block of user data)
 */
void
qfs_trans_mata_free(qfsvfs_t *qfsvfsp, offset_t mof, uchar_t ord, off_t nb)
{
	top_matadel(qfsvfsp, mof, ord, nb);
}

/*
 * allocating metadata
 */
void
qfs_trans_mata_alloc(
	qfsvfs_t *qfsvfsp,
	inode_t *ip,
	daddr_t frag,
	ulong_t nb,
	int indir)
{
#ifdef LUFS
	fs_lqfs_common_t	*fs	= VFS_FS_PTR(qfsvfsp);
	o_mode_t	ifmt 	= ip->i_mode & IFMT;
#else
	o_mode_t	ifmt	= ip->di.mode & S_IFMT;
#endif /* LUFS */

#ifdef LUFS
	if (indir || ((ifmt == IFDIR) || (ifmt == IFSHAD) ||
	    (ifmt == IFATTRDIR) || (ip == ip->i_qfsvfs->vfs_qinod))) {
		TRANS_MATAADD(qfsvfsp, ldbtob(fsbtodb(fs, frag)), nb);
	}
#else
	if (indir || (ifmt == S_IFDIR)) {
		TRANS_MATAADD(qfsvfsp, ldbtob(fsbtodb(fs, frag)), ip->dord, nb);
	}
#endif /* LUFS */
}

#endif /* DEBUG */

/*
 * qfs_trans_dir is used to declare a directory delta
 */
int
qfs_trans_dir(inode_t *ip, off_t offset)
{
#ifdef LUFS
	daddr_t	bn;
	int	contig = 0, error;
#else
	int error;
	sam_ioblk_t ioblk;
#endif /* LUFS */

	ASSERT(ip);
	ASSERT(RW_WRITE_HELD(&ip->i_contents));
#ifdef LUFS
	error = bmap_read(ip, (u_offset_t)offset, &bn, &contig);
	if (error || (bn == QFS_HOLE)) {
		cmn_err(CE_WARN, "qfs_trans_dir - could not get block"
		    " number error = %d bn = %d\n", error, (int)bn);
		if (error == 0)	{ /* treat UFS_HOLE as an I/O error */
			error = EIO;
		}
#else
	bzero((char *)&ioblk, sizeof (ioblk));
	error = sam_map_block(ip, offset, DIR_BLK, SAM_READ, &ioblk, CRED());
	if (error) {
		cmn_err(CE_WARN, "qfs_trans_dir - could not get inode %d"
		    " block at offset %d error = %d\n", ip->di.id.ino, offset,
		    error);
#endif /* LUFS */
		return (error);
	}
#ifdef LUFS
	TRANS_DELTA(ip->i_qfsvfs, ldbtob(bn), DIRBLKSIZ, DT_DIR, 0, 0);
#else
	LQFS_MSG(CE_WARN, "qfs_trans_dir(): TRANS_DELTA for dir inode %d "
	    "offset %ld - mof 0x%x ord %d size 4K\n", ip->di.id.ino,
	    offset, (ldbtob(fsbtodb(ip->mp, ioblk.blkno)) + ioblk.pboff) &
	    ~(DIR_BLK - 1), ioblk.ord);
	TRANS_DELTA(ip->mp,
	    (ldbtob(fsbtodb(ip->mp, ioblk.blkno)) + ioblk.pboff) &
	    ~(DIR_BLK - 1), ioblk.ord, DIR_BLK, DT_DIR, 0, 0);
#endif /* LUFS */
	return (error);
}

#ifdef LQFS_TODO_QUOTAS
/*ARGSUSED*/
int
qfs_trans_push_quota(qfsvfs_t *qfsvfsp, delta_t dtyp, struct dquot *dqp)
{
	/*
	 * Lock the quota subsystem (qfsvfsp can be NULL
	 * if the DQ_ERROR is set).
	 */
	if (qfsvfsp) {
		rw_enter(&qfsvfsp->vfs_dqrwlock, RW_READER);
	}
	mutex_enter(&dqp->dq_lock);

	/*
	 * If this transaction has been cancelled by closedq_scan_inode(),
	 * then bail out now.  We don't call dqput() in this case because
	 * it has already been done.
	 */
	if ((dqp->dq_flags & DQ_TRANS) == 0) {
		mutex_exit(&dqp->dq_lock);
		if (qfsvfsp) {
			rw_exit(&qfsvfsp->vfs_dqrwlock);
		}
		return (0);
	}

	if (dqp->dq_flags & DQ_ERROR) {
		/*
		 * Paranoia to make sure that there is at least one
		 * reference to the dquot struct.  We are done with
		 * the dquot (due to an error) so clear logging
		 * specific markers.
		 */
		ASSERT(dqp->dq_cnt >= 1);
		dqp->dq_flags &= ~DQ_TRANS;
		dqput(dqp);
		mutex_exit(&dqp->dq_lock);
		if (qfsvfsp) {
			rw_exit(&qfsvfsp->vfs_dqrwlock);
		}
		return (1);
	}

	if (dqp->dq_flags & (DQ_MOD | DQ_BLKS | DQ_FILES)) {
		ASSERT((dqp->dq_mof != QFS_HOLE) && (dqp->dq_mof != 0));
		TRANS_LOG(qfsvfsp, (caddr_t)&dqp->dq_dqb,
		    dqp->dq_mof, dqp->dq_ord, (int)sizeof (struct dqblk),
		    NULL, 0);
		/*
		 * Paranoia to make sure that there is at least one
		 * reference to the dquot struct.  Clear the
		 * modification flag because the operation is now in
		 * the log.  Also clear the logging specific markers
		 * that were set in qfs_trans_quota().
		 */
		ASSERT(dqp->dq_cnt >= 1);
		dqp->dq_flags &= ~(DQ_MOD | DQ_TRANS);
		dqput(dqp);
	}

	/*
	 * At this point, the logging specific flag should be clear,
	 * but add paranoia just in case something has gone wrong.
	 */
	ASSERT((dqp->dq_flags & DQ_TRANS) == 0);
	mutex_exit(&dqp->dq_lock);
	if (qfsvfsp) {
		rw_exit(&qfsvfsp->vfs_dqrwlock);
	}
	return (0);
}

/*
 * qfs_trans_quota take in a uid, allocates the disk space, placing the
 * quota record into the metamap, then declares the delta.
 */
/*ARGSUSED*/
void
qfs_trans_quota(struct dquot *dqp)
{

	inode_t	*qip = dqp->dq_qfsvfsp->vfs_qinod;

	ASSERT(qip);
	ASSERT(MUTEX_HELD(&dqp->dq_lock));
	ASSERT(dqp->dq_flags & DQ_MOD);
	ASSERT(dqp->dq_mof != 0);
	ASSERT(dqp->dq_mof != QFS_HOLE);
	ASSERT((dqp->dq_ord >= 0) &&
	    (dqp->dq_ord <= dqp->dq_qfsvfsp->mt.fs_count));

	/*
	 * Mark this dquot to indicate that we are starting a logging
	 * file system operation for this dquot.  Also increment the
	 * reference count so that the dquot does not get reused while
	 * it is on the mapentry_t list.  DQ_TRANS is cleared and the
	 * reference count is decremented by qfs_trans_push_quota.
	 *
	 * If the file system is force-unmounted while there is a
	 * pending quota transaction, then closedq_scan_inode() will
	 * clear the DQ_TRANS flag and decrement the reference count.
	 *
	 * Since deltamap_add() drops multiple transactions to the
	 * same dq_mof/dq_ord and qfs_trans_push_quota() won't get called,
	 * we use DQ_TRANS to prevent repeat transactions from
	 * incrementing the reference count (or calling TRANS_DELTA()).
	 */
	if ((dqp->dq_flags & DQ_TRANS) == 0) {
		dqp->dq_flags |= DQ_TRANS;
		dqp->dq_cnt++;
		TRANS_DELTA(qip->i_qfsvfs, dqp->dq_mof, dqp->dq_ord,
		    sizeof (struct dqblk), DT_QR, qfs_trans_push_quota,
		    (uint64_t)dqp);
	}
}

void
qfs_trans_dqrele(struct dquot *dqp)
{
	qfsvfs_t	*qfsvfsp = dqp->dq_qfsvfsp;

	curthread->t_flag |= T_DONTBLOCK;
	TRANS_BEGIN_ASYNC(qfsvfsp, TOP_QUOTA, TOP_QUOTA_SIZE);
	rw_enter(&qfsvfsp->vfs_dqrwlock, RW_READER);
	dqrele(dqp);
	rw_exit(&qfsvfsp->vfs_dqrwlock);
	TRANS_END_ASYNC(qfsvfsp, TOP_QUOTA, TOP_QUOTA_SIZE);
	curthread->t_flag &= ~T_DONTBLOCK;
}
#endif /* LQFS_TODO_QUOTAS */

int qfs_trans_max_resv = TOP_MAX_RESV;	/* will be adjusted for testing */
long qfs_trans_avgbfree = 0;		/* will be adjusted for testing */
#define	TRANS_MAX_WRITE	(1024 * 1024)
size_t qfs_trans_max_resid = TRANS_MAX_WRITE;

/*
 * Calculate the log reservation for the given write or truncate
 */
static ulong_t
qfs_log_amt(inode_t *ip, offset_t offset, ssize_t resid, int trunc)
{
	long		last2blk;
	long		niblk		= 0;
	u_offset_t	writeend, offblk;
	int		resv;
	daddr_t		nblk, maxfblk;
#ifdef LUFS
	qfsvfs_t	*qfsvfsp	= ip->i_qfsvfs;
	fs_lqfs_common_t	*fs	= VFS_FS_PTR(qfsvfsp);
	long		avgbfree;
	long		ncg;
	long		fni		= NINDIR(fs);
#else
	long		fni		= DEXT;
#endif /* LUFS */
	int		bsize = FS_BSIZE(fs);

	/*
	 * Assume that the request will fit in 1 or 2 cg's,
	 * resv is the amount of log space to reserve (in bytes).
	 */
	resv = SIZECG(ip) * 2 + INODESIZE + 1024;

	/*
	 * get max position of write in fs blocks
	 */
	writeend = offset + resid;
	maxfblk = lblkno(fs, writeend);
	offblk = lblkno(fs, offset);
	/*
	 * request size in fs blocks
	 */
	nblk = lblkno(fs, blkroundup(fs, resid));
	/*
	 * Adjust for sparse files
	 */
	if (trunc) {
#ifdef LUFS
		nblk = MIN(nblk, ip->i_blocks);
#else
		nblk = MIN(nblk, (ip->di.blocks * (SAM_BLK/DEV_BSIZE)));
#endif /* LUFS */
	}

#ifdef LQFS_CYLINDER_GROUPS
	/*
	 * Adjust avgbfree (for testing)
	 */
	avgbfree = (qfs_trans_avgbfree) ? 1 : qfsvfsp->vfs_avgbfree + 1;
#else
	/* QFS doesn't define cylinder groups. */
#endif /* LQFS_CYLINDER_GROUPS */

	/*
	 * Calculate maximum number of blocks of triple indirect
	 * pointers to write.
	 */
#ifdef LUFS
	last2blk = NDADDR + fni + fni * fni;
#else
	last2blk = NDEXT + fni + fni * fni;
#endif /* LUFS */
	if (maxfblk > last2blk) {
		long nl2ptr;
		long n3blk;

		if (offblk > last2blk) {
			n3blk = maxfblk - offblk;
		} else {
			n3blk = maxfblk - last2blk;
		}
		niblk += roundup(n3blk * sizeof (daddr_t), bsize) / bsize + 1;
		nl2ptr = roundup(niblk, fni) / fni + 1;
		niblk += roundup(nl2ptr * sizeof (daddr_t), bsize) / bsize + 2;
		maxfblk -= n3blk;
	}
	/*
	 * calculate maximum number of blocks of double indirect
	 * pointers to write.
	 */
#ifdef LUFS
	if (maxfblk > NDADDR + fni) {
		long n2blk;

		if (offblk > NDADDR + fni) {
			n2blk = maxfblk - offblk;
		} else {
			n2blk = maxfblk - NDADDR + fni;
		}
		niblk += roundup(n2blk * sizeof (daddr_t), bsize) / bsize + 2;
		maxfblk -= n2blk;
	}
	/*
	 * Add in indirect pointer block write
	 */
	if (maxfblk > NDADDR) {
		niblk += 1;
	}
#else
	if (maxfblk > NDEXT + fni) {
		long n2blk;

		if (offblk > NDEXT + fni) {
			n2blk = maxfblk - offblk;
		} else {
			n2blk = maxfblk - NDEXT + fni;
		}
		niblk += roundup(n2blk * sizeof (daddr_t), bsize) / bsize + 2;
		maxfblk -= n2blk;
	}
	/*
	 * Add in indirect pointer block write
	 */
	if (maxfblk > NDEXT) {
		niblk += 1;
	}
#endif /* LUFS */
	/*
	 * Calculate deltas for indirect pointer writes
	 */
	resv += niblk * (FS_BSIZE(fs) + sizeof (struct delta));

#ifdef LQFS_TODO_CYLINDER_GROUPS
	/*
	 * maximum number of cg's needed for request
	 */
	ncg = nblk / avgbfree;
	if (ncg > fs->fs_ncg) {
		ncg = fs->fs_ncg;
	}

	/*
	 * maximum amount of log space needed for request
	 */
	if (ncg > 2) {
		resv += (ncg - 2) * SIZECG(ip);
	}
#endif /* LQFS_TODO_CYLINDER_GROUPS */
	return (resv);
}

/*
 * Calculate the amount of log space that needs to be reserved for this
 * trunc request.  If the amount of log space is too large, then
 * calculate the size that the requests needs to be split into.
 */
static void
qfs_trans_trunc_resv(
	inode_t *ip,
	u_offset_t length,
	int *resvp,
	u_offset_t *residp)
{
	ulong_t		resv;
	u_offset_t	size, offset, resid;
	int		nchunks;

	/*
	 *    *resvp is the amount of log space to reserve (in bytes).
	 *    when nonzero, *residp is the number of bytes to truncate.
	 */
	*residp = 0;

#ifdef LUFS
	if (length < ip->i_size) {
		size = ip->i_size - length;
#else
	if (length < ip->size) {
		size = ip->size - length;
#endif /* LUFS */
	} else {
		resv = SIZECG(ip) * 2 + INODESIZE + 1024;
		/*
		 * truncate up, doesn't really use much space,
		 * the default above should be sufficient.
		 */
		goto done;
	}

	offset = length;
	resid = size;
	nchunks = 1;
	for (; (resv = qfs_log_amt(ip, offset, resid, 1)) > qfs_trans_max_resv;
	    offset = length + (nchunks - 1) * resid) {
		nchunks++;
		resid = size / nchunks;
	}
	/*
	 * If this request takes too much log space, it will be split
	 */
	if (nchunks > 1) {
		*residp = resid;
	}
done:
	*resvp = resv;
}

int
qfs_trans_itrunc(inode_t *ip, u_offset_t length, int flags, cred_t *cr)
{
	int 		err, issync, resv;
	u_offset_t	resid;
	int		do_block	= 0;
	qfsvfs_t	*qfsvfsp	= ip->i_qfsvfs;
#ifdef LUFS
	fs_lqfs_common_t	*fs	= VFS_FS_PTR(qfsvfsp);
#endif /* LUFS */

	/*
	 * Not logging; just do the trunc
	 */
	if (!TRANS_ISTRANS(qfsvfsp)) {
#ifdef LQFS_TODO_QUOTAS
		rw_enter(&qfsvfsp->vfs_dqrwlock, RW_READER);
#endif /* LQFS_TODO_QUOTAS */
		rw_enter(&ip->i_contents, RW_WRITER);
#ifdef LUFS
		err = qfs_itrunc(ip, length, flags, cr);
#else
		err = sam_clear_file(ip, length, flags, cr);
#endif /* LUFS */
#ifdef LQFS_TODO_QUOTAS
		rw_exit(&qfsvfsp->vfs_dqrwlock);
#endif /* LQFS_TODO_QUOTAS */
		rw_exit(&ip->i_contents);
		return (err);
	}

	/*
	 * within the lockfs protocol but *not* part of a transaction
	 */
	do_block = curthread->t_flag & T_DONTBLOCK;
	curthread->t_flag |= T_DONTBLOCK;

	/*
	 * Trunc the file (in pieces, if necessary)
	 */
again:
	qfs_trans_trunc_resv(ip, length, &resv, &resid);
	TRANS_BEGIN_CSYNC(qfsvfsp, issync, TOP_ITRUNC, resv);
#ifdef LQFS_TODO_QUOTAS
	rw_enter(&qfsvfsp->vfs_dqrwlock, RW_READER);
#endif /* LQFS_TODO_QUOTAS */
	rw_enter(&ip->i_contents, RW_WRITER);
	if (resid) {
		/*
		 * resid is only set if we have to truncate in chunks
		 */
#ifdef LUFS
		ASSERT(length + resid < ip->i_size);
#else
		ASSERT(length + resid < ip->size);
#endif /* LUFS */

		/*
		 * Partially trunc file down to desired size (length).
		 * Only retain I_FREE on the last partial trunc.
		 * Round up size to a block boundary, to ensure the truncate
		 * doesn't have to allocate blocks. This is done both for
		 * performance and to fix a bug where if the block can't be
		 * allocated then the inode delete fails, but the inode
		 * is still freed with attached blocks and non-zero size.
		 */
#ifdef LUFS
		err = qfs_itrunc(ip, blkroundup(fs, (ip->i_size - resid)),
		    flags & ~I_FREE, cr);
		ASSERT(ip->i_size != length);
#else
		err = sam_clear_file(ip, blkroundup(ip->mp, (ip->size - resid)),
		    flags, cr);
		ASSERT(ip->size != length);
#endif /* LUFS */
	} else {
#ifdef LUFS
		err = qfs_itrunc(ip, length, flags, cr);
#else
		err = sam_clear_file(ip, length, flags, cr);
#endif /* LUFS */
	}
	if (!do_block) {
		curthread->t_flag &= ~T_DONTBLOCK;
	}
	rw_exit(&ip->i_contents);
#ifdef LQFS_TODO_QUOTAS
	rw_exit(&qfsvfsp->vfs_dqrwlock);
#endif /* LQFS_TODO_QUOTAS */
	TRANS_END_CSYNC(qfsvfsp, err, issync, TOP_ITRUNC, resv);

#ifdef LQFS_TODO_CYLINDER_GROUPS
	if ((err == 0) && resid) {
		qfsvfsp->vfs_avgbfree = fs->fs_cstotal.cs_nbfree / fs->fs_ncg;
		goto again;
	}
#endif /* LQFS_TODO_CYLINDER_GROUPS */
	return (err);
}

/*
 * Fault in the pages of the first n bytes specified by the uio structure.
 * 1 byte in each page is touched and the uio struct is unmodified.
 * Any error will terminate the process as this is only a best
 * attempt to get the pages resident.
 */
static void
qfs_trans_touch(ssize_t n, struct uio *uio)
{
	struct iovec *iov;
	ulong_t cnt, incr;
	caddr_t p;
	uint8_t tmp;

	iov = uio->uio_iov;

	while (n) {
		cnt = MIN(iov->iov_len, n);
		if (cnt == 0) {
			/* empty iov entry */
			iov++;
			continue;
		}
		n -= cnt;
		/*
		 * touch each page in this segment.
		 */
		p = iov->iov_base;
		while (cnt) {
			switch (uio->uio_segflg) {
			case UIO_USERSPACE:
			case UIO_USERISPACE:
				if (fuword8(p, &tmp)) {
					return;
				}
				break;
			case UIO_SYSSPACE:
				if (kcopy(p, &tmp, 1)) {
					return;
				}
				break;
			}
			incr = MIN(cnt, PAGESIZE);
			p += incr;
			cnt -= incr;
		}
		/*
		 * touch the last byte in case it straddles a page.
		 */
		p--;
		switch (uio->uio_segflg) {
		case UIO_USERSPACE:
		case UIO_USERISPACE:
			if (fuword8(p, &tmp)) {
				return;
			}
			break;
		case UIO_SYSSPACE:
			if (kcopy(p, &tmp, 1)) {
				return;
			}
			break;
		}
		iov++;
	}
}

/*
 * Calculate the amount of log space that needs to be reserved for this
 * write request.  If the amount of log space is too large, then
 * calculate the size that the requests needs to be split into.
 * First try fixed chunks of size qfs_trans_max_resid. If that
 * is too big, iterate down to the largest size that will fit.
 * Pagein the pages in the first chunk here, so that the pagein is
 * avoided later when the transaction is open.
 */
void
qfs_trans_write_resv(
	inode_t *ip,
	struct uio *uio,
	int *resvp,
	int *residp)
{
	ulong_t		resv;
	offset_t	offset;
	ssize_t		resid;
	int		nchunks;

	*residp = 0;
	offset = uio->uio_offset;
	resid = MIN(uio->uio_resid, qfs_trans_max_resid);
	resv = qfs_log_amt(ip, offset, resid, 0);
	if (resv <= qfs_trans_max_resv) {
		qfs_trans_touch(resid, uio);
		if (resid != uio->uio_resid) {
			*residp = resid;
		}
		*resvp = resv;
		return;
	}

	resid = uio->uio_resid;
	nchunks = 1;
	for (; (resv = qfs_log_amt(ip, offset, resid, 0)) > qfs_trans_max_resv;
	    offset = uio->uio_offset + (nchunks - 1) * resid) {
		nchunks++;
		resid = uio->uio_resid / nchunks;
	}
	qfs_trans_touch(resid, uio);
	/*
	 * If this request takes too much log space, it will be split
	 */
	if (nchunks > 1) {
		*residp = resid;
	}
	*resvp = resv;
}

/*
 * Issue write request.
 *
 * Split a large request into smaller chunks.
 */
/*ARGSUSED3*/
int
qfs_trans_write(
	inode_t *ip,
	struct uio *uio,
	int ioflag,
	cred_t *cr,
	int resv,
	long resid)
{
	long		realresid;
	int		err = 0;
	qfsvfs_t	*qfsvfsp = ip->i_qfsvfs;

	/*
	 * since the write is too big and would "HOG THE LOG" it needs to
	 * be broken up and done in pieces.  NOTE, the caller will
	 * issue the EOT after the request has been completed
	 */
	realresid = uio->uio_resid;

again:
	/*
	 * Perform partial request (uiomove will update uio for us)
	 *	Request is split up into "resid" size chunks until
	 *	"realresid" bytes have been transferred.
	 */
	uio->uio_resid = MIN(resid, realresid);
	realresid -= uio->uio_resid;
#ifdef LQFS_TODO
	err = wrip(ip, uio, ioflag, cr);
#endif /* LQFS_TODO */

	/*
	 * Error or request is done; caller issues final EOT
	 */
	if (err || uio->uio_resid || (realresid == 0)) {
		uio->uio_resid += realresid;
		return (err);
	}

	/*
	 * Generate EOT for this part of the request
	 */
	rw_exit(&ip->i_contents);
#ifdef LQFS_TODO
	rw_exit(&qfsvfsp->vfs_dqrwlock);
#endif /* LQFS_TODO */
	if (ioflag & (FSYNC|FDSYNC)) {
		TRANS_END_SYNC(qfsvfsp, err, TOP_WRITE_SYNC, resv);
	} else {
		TRANS_END_ASYNC(qfsvfsp, TOP_WRITE, resv);
	}

	/*
	 * Make sure the input buffer is resident before starting
	 * the next transaction.
	 */
	qfs_trans_touch(MIN(resid, realresid), uio);

	/*
	 * Generate BOT for next part of the request
	 */
	if (ioflag & (FSYNC|FDSYNC)) {
		int error;
		TRANS_BEGIN_SYNC(qfsvfsp, TOP_WRITE_SYNC, resv, error);
		ASSERT(!error);
	} else {
		TRANS_BEGIN_ASYNC(qfsvfsp, TOP_WRITE, resv);
	}
#ifdef LQFS_TODO
	rw_enter(&qfsvfsp->vfs_dqrwlock, RW_READER);
#endif /* LQFS_TODO */
	rw_enter(&ip->i_contents, RW_WRITER);
	/*
	 * Error during EOT (probably device error while writing commit rec)
	 */
	if (err) {
		return (err);
	}
	goto again;
}
