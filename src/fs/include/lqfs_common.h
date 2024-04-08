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

#ifndef _SYS_FS_LQFS_COMMON_H
#define	_SYS_FS_LQFS_COMMON_H

#ifdef sun
#pragma ident "$Revision: 1.7 $"
#endif /* sun */

#ifdef LUFS
typedef	struct timeval	timeval_lqfs_common_t;
typedef struct fs	fs_lqfs_common_t;

#define	VFS_FS_PTR(p)	((p)->vfs_fs)
#define	VFS_PTR(p)	((p))

/* Read-only FS */
#define	FS_RDONLY(fs)	((fs)->fs_rdonly)

/* Size of fundamental FS block */
#define	FS_BSIZE(fs)		((fs)->fs_bsize)

/* Shift to convert between bytes and 1K frags */
#define	FS_FSHIFT(fs)		((fs)->fs_fshift)

/* Shift to convert between FS fundamental blocks(4K) and 1K frags */
#define	FS_FRAGSHIFT(fs)	((fs)->fs_fragshift)

/* Shift to convert between 1K frags and disk sectors */
#define	FS_FSBTODB(fs)		((fs)->fs_fsbtodb)

/* # Frags in a block */
#define	FS_FRAG(fs)		((fs)->fs_frag)

/* Size of one frag */
#define	FS_FSIZE(fs)		((fs)->fs_fsize)

/* File system size in fundamental FS blocks */
#define	FS_SIZE(fs)		((fs)->fs_size)

/* Mask for rounding to fundamental FS block */
#define	FS_BMASK(fs)		((fs)->fs_bmask)

#define	VFS_LOCK_MUTEX_ENTER(qfsvfsp)	\
		mutex_enter(&((qfsvfsp)->vfs_lock))
#define	VFS_LOCK_MUTEX_EXIT(qfsvfsp)	\
		mutex_exit(&((qfsvfsp)->vfs_lock))

#define	UL_SBOWNER_SET(qfsvfsp, owner)	\
		((qfsvfsp)->vfs_ulockfs.ul_sbowner = (owner))

#define	VFS_IOTRANSZ(qfsvfsp)	((qfsvfsp)->vfs_iotransz)

#define	SBLOCK_OFFSET(qfsvfsp)	(SBLOCK)

#else

/* vfs Field Names */

#define	vfs_log		mi.m_log
/* #define	vfs_root	mi.m_vn_root */
#define	vfs_vfs		mi.m_vfsp
#define	vfs_fs		mi.m_sbp
#define	vfs_domatamap	mi.m_vfs_domatamap
#define	vfs_nolog_si	mi.m_vfs_nolog_si
#define	vfs_iotstamp	mi.m_vfs_iotstamp

/* fs Field Names */

#define	fs_magic	mi.m_sbp->info.sb.magic
#define	fs_logbno	mi.m_sbp->info.sb.logbno
#define	fs_logord	mi.m_sbp->info.sb.logord
#define	fs_clean	mi.m_sbp->info.sb.qfs_clean
#define	fs_rolled	mi.m_sbp->info.sb.qfs_rolled
#define	fs_sbsize	mi.m_sbp->info.sb.sblk_size
#define	fs_fsmnt	info.sb.fs_name

/*
 * The UFS logging infrastructure depends heavily on the type
 * inode_t for management of inode-related attributes.  QFS
 * maintains some of the corresponding info in other structures
 * relative to the type sam_node_t.  Use the following
 * definitions to keep new QFS code as common as possible with
 * UFS code that references inode_t fields.
 */
#include <inode.h>

typedef	sam_node_t	inode_t;

#define	i_qfsvfs	mp
#define	i_contents	inode_rwl
#define	i_blocks	di.blocks
#define	i_mode		di.mode
#define	IFMT		S_IFMT

#define	VTOI(vp)	(SAM_VTOI((vp)))
#define	ITOV(ip)	(SAM_ITOV((ip)))

/*
 * The UFS logging infrastructure depends heavily on the type
 * qfsvfs_t for management of attributes related to the virtual
 * filesystem.  QFS maintains some of the corresponding info in
 * other structures relative to the type sam_mount_t.  Use the
 * following definitions to keep new QFS code as common as
 * possible with UFS code that references qfsvfs_t fields.
 */

/* Forward definition of struct ml_unit for sam_mt_instance_t */
struct ml_unit;

#include <sys/types.h>
#include <sam/types.h>
#include <quota.h>
#include <share.h>
#include <mount.h>
#include <sys/mount.h>
#ifdef _KERNEL
#include <extern.h>
#endif /* _KERNEL */
#include <sblk.h>
#include <sys/time_impl.h>
#include <sys/lockfs.h>

typedef	sam_mount_t	qfsvfs_t;

/*
 * The UFS logging infrastructure depends heavily on type
 * 'struct fs' for management of superblock-related attributes.
 * QFS maintains some of the corresponding info in other structures
 * relative to the type sam_mount_t.  Use the following definitions
 * to keep new QFS code as common as possible with UFS code that
 * references struct fs fields.
 */

typedef	sam_mount_t	fs_lqfs_common_t;
#define	VFS_FS_PTR(p)	(p)
#define	VFS_PTR(p)	((p)->mi.m_vfsp)

#define	ITOF(IP)	((struct sam_sblk *)(IP)->mp->mi.m_sbp)

#define	SBLOCK_OFFSET(qfsvfsp)	\
			(fsbtodb(qfsvfsp, qfsvfsp->mi.m_sblk_offset[0]))

#define	QFS_GETBLK(qfsvfsp, dev, blkno, bsize)	\
		getblk((dev), (blkno), (bsize))

#define	QFS_BWRITE(qfsvfsp, bp)		bwrite2((bp))
#define	QFS_BWRITE2(qfsvfsp, bp)	bwrite2((bp))

#define	vfs_iotransz	((1 << ((sizeof (ushort_t) << 3) - 1)) * DEV_BSIZE)

typedef	timespec_t	timeval_lqfs_common_t;
#define	uniqtime	SAM_HRESTIME
#ifdef _LQFS_INFRASTRUCTURE
#define	tv_usec		tv_nsec
#endif /* _LQFS_INFRASTRUCTURE */

/* Read-only FS */
#define	FS_RDONLY(fs)	((fs)->mt.fi_mflag & MS_RDONLY)

/* Size in bytes of FS fundamental (4K) block */
#define	FS_BSIZE(fs)			(SAM_BLK)

/* Number of 1K frags in an FS fundamental block */
#define	FS_FRAG(fs)			(SAM_LOG_BLOCK)

/* Shift to convert between byte offset and 1K frag offset */
#define	FS_FSHIFT(fs)			(SAM_DEV_BSHIFT)

/* Size of FS in 1K frags */
#define	FS_SIZE(fs)			((fs)->mi.m_sbp->info.sb.capacity)

/* Size of one frag */
#define	FS_FSIZE(fs)			(1024)

/* 64-bit mask for rounding to FS fundamental block offset */
#define	FS_BMASK(fs)			(0xffffffffffffffff ^ (SAM_BLK-1))

/* Shift to convert between 1K frags and disk 512-byte sectors */
#define	FS_FSBTODB(fs)			(SAM2SUN_BSHIFT)

/* Log frag (1K) offset to disk block (512-byte sector) offset */
#define	logbtodb(fs, log_frag_off)	\
			(((daddr_t)(log_frag_off)) << FS_FSBTODB((fs)))

/* Log frag (1K) offset to frag (1K) offset */
#define	logbtofrag(fs, log_frag_off)	(log_frag_off)

/* Shift to convert between 1K frags offset and FS 4k block offset */
#define	FS_FRAGSHIFT(fs)		(DIF_SAM_SHIFT)

/* Log frag (1K) offset to FS 4K block offset */
#define	logbtofsblk(fs, log_frag_off)	\
					((log_frag_off) >> FS_FRAGSHIFT((fs)))

/* FS fundamental block offset to log frag (1K) offset */
#define	fsblktologb(fs, log_blk_off)	\
					((log_blk_off) << FS_FRAGSHIFT((fs)))

/* Frag (1K) offset to disk block (sector) offset */
#define	fsbtodb(fs, frag_off)	\
				(((daddr_t)(frag_off)) << FS_FSBTODB((fs)))

/* Disk block (sector) offset to frag (1K) offset */
#define	dbtofsb(fs, disk_off)	\
					((disk_off) >> FS_FSBTODB((fs)))

/* Round up to next 4K block size */
#define	blkroundup(fs, size)		(((size) + (FS_BSIZE((fs)) - 1)) &  \
						(offset_t)FS_BMASK((fs)))

/* Calculate byte loc in FS 4K blocks */
#define	blkoff(fs, loc)			((int)((loc & ~(FS_BMASK(fs)))))

/* Calculate byte loc in FS 1K frags */
#define	numfrags(fs, loc)		((int32_t)((loc) >> (FS_FSHIFT(fs))))

/* Shift value for fundamental 4K block */
#define	FS_BSHIFT(fs)			SAM_BIT_SHIFT

/*
 * The cast to int32_t does not result in any loss of information because
 * the number of logical blocks in the file system is limited to
 * what fits in an int32_t anyway.
 */
#define	lblkno(fs, loc)		/* calculates (loc / FS_BSIZE(fs)) */	\
	((int32_t)((loc) >> FS_BSHIFT(fs)))


#define	qfs_fiolfss(vp, lfp)	\
		sam_lockfs_status((SAM_VTOI((vp)))->mp, (lfp))

#define	qfs_fiolfs(vp, lfp, i)	\
		sam_lockfs((SAM_VTOI((vp)))->mp, (lfp), (i))

#define	VFS_LOCK_MUTEX_ENTER(qfsvfsp)	\
		mutex_enter(&samgt.global_mutex);	\
		mutex_enter(&((qfsvfsp)->ms.m_waitwr_mutex))

#define	VFS_LOCK_MUTEX_EXIT(qfsvfsp)	\
		mutex_exit(&((qfsvfsp)->ms.m_waitwr_mutex));	\
		mutex_exit(&samgt.global_mutex)

#define	UL_SBOWNER_SET(qfsvfsp, owner)	\
		((qfsvfsp)->mi.m_ul_sbowner = (kthread_id_t)(owner))

#define	VFS_IOTRANSZ(qfsvfsp)	\
		((1 << (8 * sizeof (ushort_t) - 1)) - 1)

#include <qfs_log.h>

#endif /* LUFS */

#endif	/* _SYS_FS_LQFS_COMMON_H */
