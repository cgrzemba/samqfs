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

#ifdef sun
#pragma ident	"$Revision: 1.17 $"
#endif

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/errno.h>
#define	_LQFS_INFRASTRUCTURE
#include <lqfs_common.h>
#include <extern.h>
#undef _LQFS_INFRASTRUCTURE
#ifdef LUFS
#include <sys/fssnap_if.h>
#include <sys/fs/qfs_inode.h>
#include <sys/fs/qfs_filio.h>
#endif /* LUFS */
#include <sys/sysmacros.h>
#include <sys/modctl.h>
#ifdef LUFS
#include <sys/fs/qfs_log.h>
#include <sys/fs/qfs_bio.h>
#include <sys/fs/qfs_fsdir.h>
#endif /* LUFS */
#include <sys/debug.h>
#include <sys/kmem.h>
#include <sys/inttypes.h>
#include <sys/vfs.h>
#include <sys/mntent.h>
#include <sys/conf.h>
#include <sys/param.h>
#include <sys/kstat.h>
#include <sys/cmn_err.h>

extern	kmutex_t	qfs_scan_lock;

extern void (*bio_lqfs_strategy)(void *, buf_t *);

static kmutex_t	log_mutex;	/* general purpose log layer lock */
kmutex_t	ml_scan;	/* Scan thread syncronization */
kcondvar_t	ml_scan_cv;	/* Scan thread syncronization */

kstat_t		*lqfs_logstats = NULL;
kstat_t		*lqfs_deltastats = NULL;
struct kmem_cache	*lqfs_sv = NULL;
struct kmem_cache	*lqfs_bp = NULL;
extern struct kmem_cache	*mapentry_cache;

/* Tunables */
uint_t		ldl_maxlogsize	= LDL_MAXLOGSIZE;
uint_t		ldl_minlogsize	= LDL_MINLOGSIZE;
uint32_t	ldl_divisor	= LDL_DIVISOR;
uint32_t	ldl_mintransfer	= LDL_MINTRANSFER;
uint32_t	ldl_maxtransfer	= LDL_MAXTRANSFER;
uint32_t	ldl_minbufsize	= LDL_MINBUFSIZE;

uint32_t	last_loghead_ident = 0;

/*
 * Logging delta and roll statistics
 */
struct delta_kstats {
	kstat_named_t ds_superblock_deltas;
	kstat_named_t ds_bitmap_deltas;
	kstat_named_t ds_suminfo_deltas;
	kstat_named_t ds_allocblk_deltas;
	kstat_named_t ds_ab0_deltas;
	kstat_named_t ds_dir_deltas;
	kstat_named_t ds_inode_deltas;
	kstat_named_t ds_fbiwrite_deltas;
	kstat_named_t ds_quota_deltas;
	kstat_named_t ds_shadow_deltas;

	kstat_named_t ds_superblock_rolled;
	kstat_named_t ds_bitmap_rolled;
	kstat_named_t ds_suminfo_rolled;
	kstat_named_t ds_allocblk_rolled;
	kstat_named_t ds_ab0_rolled;
	kstat_named_t ds_dir_rolled;
	kstat_named_t ds_inode_rolled;
	kstat_named_t ds_fbiwrite_rolled;
	kstat_named_t ds_quota_rolled;
	kstat_named_t ds_shadow_rolled;
} dkstats = {
	{ "superblock_deltas",	KSTAT_DATA_UINT64 },
	{ "bitmap_deltas",	KSTAT_DATA_UINT64 },
	{ "suminfo_deltas",	KSTAT_DATA_UINT64 },
	{ "allocblk_deltas",	KSTAT_DATA_UINT64 },
	{ "ab0_deltas",		KSTAT_DATA_UINT64 },
	{ "dir_deltas",		KSTAT_DATA_UINT64 },
	{ "inode_deltas",	KSTAT_DATA_UINT64 },
	{ "fbiwrite_deltas",	KSTAT_DATA_UINT64 },
	{ "quota_deltas",	KSTAT_DATA_UINT64 },
	{ "shadow_deltas",	KSTAT_DATA_UINT64 },

	{ "superblock_rolled",	KSTAT_DATA_UINT64 },
	{ "bitmap_rolled",	KSTAT_DATA_UINT64 },
	{ "suminfo_rolled",	KSTAT_DATA_UINT64 },
	{ "allocblk_rolled",	KSTAT_DATA_UINT64 },
	{ "ab0_rolled",		KSTAT_DATA_UINT64 },
	{ "dir_rolled",		KSTAT_DATA_UINT64 },
	{ "inode_rolled",	KSTAT_DATA_UINT64 },
	{ "fbiwrite_rolled",	KSTAT_DATA_UINT64 },
	{ "quota_rolled",	KSTAT_DATA_UINT64 },
	{ "shadow_rolled",	KSTAT_DATA_UINT64 }
};

uint64_t delta_stats[DT_MAX];
uint64_t roll_stats[DT_MAX];

/*
 * General logging kstats
 */
struct logstats logstats = {
	{ "master_reads",		KSTAT_DATA_UINT64 },
	{ "master_writes",		KSTAT_DATA_UINT64 },
	{ "log_reads_inmem",		KSTAT_DATA_UINT64 },
	{ "log_reads",			KSTAT_DATA_UINT64 },
	{ "log_writes",			KSTAT_DATA_UINT64 },
	{ "log_master_reads",		KSTAT_DATA_UINT64 },
	{ "log_roll_reads",		KSTAT_DATA_UINT64 },
	{ "log_roll_writes",		KSTAT_DATA_UINT64 }
};

int
trans_not_done(struct buf *cb)
{
	sema_v(&cb->b_io);
	return (0);
}

static void
trans_wait_panic(struct buf *cb)
{
	while ((cb->b_flags & B_DONE) == 0) {
		drv_usecwait(10);
	}
}

int
trans_not_wait(struct buf *cb)
{
	/*
	 * In case of panic, busy wait for completion
	 */
	if (panicstr) {
		trans_wait_panic(cb);
	} else {
		sema_p(&cb->b_io);
	}

	return (geterror(cb));
}

int
trans_wait(struct buf *cb)
{
	/*
	 * In case of panic, busy wait for completion and run md daemon queues
	 */
	if (panicstr) {
		trans_wait_panic(cb);
	}

	return (biowait(cb));
}

static void
setsum(int32_t *sp, int32_t *lp, int nb)
{
	int32_t csum = 0;

	*sp = 0;
	nb /= sizeof (int32_t);
	while (nb--) {
		csum += *lp++;
	}
	*sp = csum;
}

static int
checksum(int32_t *sp, int32_t *lp, int nb)
{
	int32_t ssum = *sp;

	setsum(sp, lp, nb);
	if (ssum != *sp) {
		*sp = ssum;
		return (0);
	}
	return (1);
}

void
cirbuf_validate(cirbuf_t *cb)
{
	if (cb != NULL) {
		LQFS_MSG(CE_WARN, "cb->cb_bp = 0x%x\n", (int)cb->cb_bp);
		LQFS_MSG(CE_WARN, "cb->cb_dirty = 0x%x\n", (int)cb->cb_dirty);
		LQFS_MSG(CE_WARN, "cb->cb_free = 0x%x\n", (int)cb->cb_free);
		LQFS_MSG(CE_WARN, "cb->cb_va = 0x%lx\n", (int)cb->cb_va);
		LQFS_MSG(CE_WARN, "cb->cb_nb = %d\n", cb->cb_nb);
		LQFS_MSG(CE_WARN, "cb->cb_rwlock = 0x%x\n",
		    (int)&cb->cb_rwlock);
	}
}

void
ml_odunit_validate(ml_odunit_t *ud)
{
	if (ud != NULL) {
		LQFS_MSG(CE_WARN, "ud->od_version = %d\n", ud->od_version);
		LQFS_MSG(CE_WARN, "ud->od_maxtransfer = %d\n",
		    ud->od_maxtransfer);
		LQFS_MSG(CE_WARN, "ud->od_devbsize = %d\n", ud->od_devbsize);
		LQFS_MSG(CE_WARN, "ud->od_requestsize = %d\n",
		    ud->od_requestsize);
		LQFS_MSG(CE_WARN, "ud->od_statesize = %d\n", ud->od_statesize);
		LQFS_MSG(CE_WARN, "ud->od_logsize = %d\n", ud->od_logsize);
		LQFS_MSG(CE_WARN, "ud->od_statebno = 0x%x\n", ud->od_statebno);
		LQFS_MSG(CE_WARN, "ud->od_head_ident = 0x%x\n",
		    ud->od_head_ident);
		LQFS_MSG(CE_WARN, "ud->od_tail_ident = 0x%x\n",
		    ud->od_tail_ident);
		LQFS_MSG(CE_WARN, "ud->od_chksum = 0x%x\n", ud->od_chksum);
		LQFS_MSG(CE_WARN, "ud->od_bol_lof = %ld\n", ud->od_bol_lof);
		LQFS_MSG(CE_WARN, "ud->od_eol_lof = %ld\n", ud->od_eol_lof);
		LQFS_MSG(CE_WARN, "ud->od_head_lof = %ld\n", ud->od_head_lof);
		LQFS_MSG(CE_WARN, "ud->od_tail_lof = %ld\n", ud->od_tail_lof);
		LQFS_MSG(CE_WARN, "ud->od_debug = 0x%x\n", ud->od_debug);
	}
}

void
ml_unit_validate(ml_unit_t *ul)
{
	if (ul != NULL) {
		LQFS_MSG(CE_WARN, "ul->un_next = 0x%x\n", (int)ul->un_next);
		LQFS_MSG(CE_WARN, "ul->un_flags = 0x%x\n", ul->un_flags);
		LQFS_MSG(CE_WARN, "ul->un_bp = 0x%x\n", (int)ul->un_bp);
		LQFS_MSG(CE_WARN, "ul->un_qfsvfs = 0x%x\n", (int)ul->un_qfsvfs);
		LQFS_MSG(CE_WARN, "ul->un_dev = 0x%lx\n", ul->un_dev);
		LQFS_MSG(CE_WARN, "ul->un_ebp = 0x%x\n", (int)ul->un_ebp);
		LQFS_MSG(CE_WARN, "ul->un_nbeb = %d\n", ul->un_nbeb);
		LQFS_MSG(CE_WARN, "ul->un_deltamap = 0x%x\n",
		    (int)ul->un_deltamap);
		LQFS_MSG(CE_WARN, "ul->un_logmap = 0x%x\n", (int)ul->un_logmap);
		LQFS_MSG(CE_WARN, "ul->un_matamap = 0x%x\n",
		    (int)ul->un_matamap);
		LQFS_MSG(CE_WARN, "ul->un_maxresv = %ld\n", ul->un_maxresv);
		LQFS_MSG(CE_WARN, "ul->un_resv = %ld\n", ul->un_resv);
		LQFS_MSG(CE_WARN, "ul->un_resv_wantin = %ld\n",
		    ul->un_resv_wantin);
		LQFS_MSG(CE_WARN, "ul->un_tid = 0x%lx\n", ul->un_tid);
		LQFS_MSG(CE_WARN, "ul->un_rdbuf:\n");
		cirbuf_validate(&(ul->un_rdbuf));
		LQFS_MSG(CE_WARN, "ul->un_wrbuf:\n");
		cirbuf_validate(&(ul->un_wrbuf));
		LQFS_MSG(CE_WARN, "ul->un_ondisk:\n");
		ml_odunit_validate(&(ul->un_ondisk));
		LQFS_MSG(CE_WARN, "ul->un_log_mutex = 0x%x\n",
		    (int)&ul->un_log_mutex);
		LQFS_MSG(CE_WARN, "ul->un_state_mutex = 0x%x\n",
		    (int)&ul->un_state_mutex);
	}
}


void
lqfs_unsnarf(qfsvfs_t *qfsvfsp)
{
	ml_unit_t *ul;
	mt_map_t *mtm;

	ul = LQFS_GET_LOGP(qfsvfsp);
	if (ul == NULL) {
		return;
	}

	mtm = ul->un_logmap;

	/*
	 * Wait for a pending top_issue_sync which is
	 * dispatched (via taskq_dispatch()) but hasnt completed yet.
	 */

	mutex_enter(&mtm->mtm_lock);

	ASSERT(mtm->mtm_taskq_sync_count >= 0);

	while (mtm->mtm_taskq_sync_count > 0) {
		cv_wait(&mtm->mtm_cv, &mtm->mtm_lock);
	}

	mutex_exit(&mtm->mtm_lock);

	/* Roll committed transactions */
	logmap_roll_dev(ul);

	/* Kill the roll thread */
	logmap_kill_roll(ul);

	/* release saved alloction info */
	if (ul->un_ebp) {
		kmem_free(ul->un_ebp, ul->un_nbeb);
	}

	/* release circular bufs */
	free_cirbuf(&ul->un_rdbuf);
	free_cirbuf(&ul->un_wrbuf);

	/* release maps */
	if (ul->un_logmap) {
		ul->un_logmap = map_put(ul->un_logmap);
	}
	if (ul->un_deltamap) {
		ul->un_deltamap = map_put(ul->un_deltamap);
	}
	if (ul->un_matamap) {
		ul->un_matamap = map_put(ul->un_matamap);
	}

	mutex_destroy(&ul->un_log_mutex);
	mutex_destroy(&ul->un_state_mutex);

	/* release state buffer MUST BE LAST!! (contains our ondisk data) */
	if (ul->un_bp) {
		brelse(ul->un_bp);
	}
	kmem_free(ul, sizeof (*ul));

	LQFS_SET_LOGP(qfsvfsp, NULL);
}

int
lqfs_snarf(qfsvfs_t *qfsvfsp, fs_lqfs_common_t *fs, int ronly)
{
	buf_t		*bp, *tbp;
	ml_unit_t	*ul;
	extent_block_t	*ebp;
	ic_extent_block_t  *nebp;
	size_t		nb;
	daddr_t		bno;	/* in disk blocks */
	int		ord;
	int		i;

	/* LINTED: warning: logical expression always true: op "||" */
	ASSERT(sizeof (ml_odunit_t) < DEV_BSIZE);

	/*
	 * Get the allocation table
	 *	During a remount the superblock pointed to by the qfsvfsp
	 *	is out of date.  Hence the need for the ``new'' superblock
	 *	pointer, fs, passed in as a parameter.
	 */
	sam_bread_db(qfsvfsp, qfsvfsp->mi.m_fs[LQFS_GET_LOGORD(fs)].dev,
	    logbtodb(fs, LQFS_GET_LOGBNO(fs)), FS_BSIZE(fs), &bp);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return (EIO);
	}
	ebp = (void *)bp->b_un.b_addr;
	if (!checksum(&ebp->chksum, (int32_t *)(void *)bp->b_un.b_addr,
	    FS_BSIZE(fs))) {
		brelse(bp);
		return (ENODEV);
	}

	/*
	 * It is possible to get log blocks with all zeros.
	 * We should also check for nextents to be zero in such case.
	 */
	if (ebp->type != LQFS_EXTENTS || ebp->nextents == 0) {
		brelse(bp);
		return (EDOM);
	}
	/*
	 * Put allocation into memory.  This requires conversion between
	 * on the ondisk format of the extent (type extent_t) and the
	 * in-core format of the extent (type ic_extent_t).  The
	 * difference is the in-core form of the extent block stores
	 * the physical offset of the extent in disk blocks, which
	 * can require more than a 32-bit field.
	 */
	nb = (size_t)(sizeof (ic_extent_block_t) +
	    ((ebp->nextents - 1) * sizeof (ic_extent_t)));
	nebp = kmem_alloc(nb, KM_SLEEP);
	nebp->ic_nextents = ebp->nextents;
	nebp->ic_nbytes = ebp->nbytes;
	nebp->ic_nextbno = ebp->nextbno;
	nebp->ic_nextord = ebp->nextord;
	for (i = 0; i < ebp->nextents; i++) {
		nebp->ic_extents[i].ic_lbno = ebp->extents[i].lbno;
		nebp->ic_extents[i].ic_nbno = ebp->extents[i].nbno;
		nebp->ic_extents[i].ic_pbno =
		    logbtodb(fs, ebp->extents[i].pbno);
		nebp->ic_extents[i].ic_ord = ebp->extents[i].ord;
	}
	brelse(bp);

	/*
	 * Get the log state
	 */
	bno = nebp->ic_extents[0].ic_pbno;
	ord = nebp->ic_extents[0].ic_ord;
	sam_bread_db(qfsvfsp, qfsvfsp->mi.m_fs[ord].dev, bno, DEV_BSIZE, &bp);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		sam_bread_db(qfsvfsp, qfsvfsp->mi.m_fs[ord].dev, bno + 1,
		    DEV_BSIZE, &bp);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			kmem_free(nebp, nb);
			return (EIO);
		}
	}

	/*
	 * Put ondisk struct into an anonymous buffer
	 *	This buffer will contain the memory for the ml_odunit struct
	 */
	tbp = ngeteblk(dbtob(LS_SECTORS));
	tbp->b_edev = bp->b_edev;
	tbp->b_dev = bp->b_dev;
	tbp->b_blkno = bno;
	bcopy(bp->b_un.b_addr, tbp->b_un.b_addr, DEV_BSIZE);
	bcopy(bp->b_un.b_addr, tbp->b_un.b_addr + DEV_BSIZE, DEV_BSIZE);
	bp->b_flags |= (B_STALE | B_AGE);
	brelse(bp);
	bp = tbp;

	/*
	 * Verify the log state
	 *
	 * read/only mounts w/bad logs are allowed.  umount will
	 * eventually roll the bad log until the first IO error.
	 * fsck will then repair the file system.
	 *
	 * read/write mounts with bad logs are not allowed.
	 *
	 */
	ul = (ml_unit_t *)kmem_zalloc(sizeof (*ul), KM_SLEEP);
	bcopy(bp->b_un.b_addr, &ul->un_ondisk, sizeof (ml_odunit_t));
	if ((ul->un_chksum != ul->un_head_ident + ul->un_tail_ident) ||
	    (ul->un_version != LQFS_VERSION_LATEST) ||
	    (!ronly && ul->un_badlog)) {
		kmem_free(ul, sizeof (*ul));
		brelse(bp);
		kmem_free(nebp, nb);
		return (EIO);
	}
	/*
	 * Initialize the incore-only fields
	 */
	if (ronly) {
		ul->un_flags |= LDL_NOROLL;
	}
	ul->un_bp = bp;
	ul->un_qfsvfs = qfsvfsp;
	ul->un_dev = qfsvfsp->mi.m_fs[ord].dev;
	ul->un_ebp = nebp;
	ul->un_nbeb = nb;
	ul->un_maxresv = btodb(ul->un_logsize) * LDL_USABLE_BSIZE;
	ul->un_deltamap = map_get(ul, deltamaptype, DELTAMAP_NHASH);
	ul->un_logmap = map_get(ul, logmaptype, LOGMAP_NHASH);
	if (ul->un_debug & MT_MATAMAP) {
		ul->un_matamap = map_get(ul, matamaptype, DELTAMAP_NHASH);
	}
	sam_mutex_init(&ul->un_log_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&ul->un_state_mutex, NULL, MUTEX_DEFAULT, NULL);

	/*
	 * Aquire the qfs_scan_lock before linking the mtm data
	 * structure so that we keep qfs_sync() and qfs_update() away
	 * when they execute the qfs_scan_inodes() run while we're in
	 * progress of enabling/disabling logging.
	 */

	mutex_enter(&qfs_scan_lock);
	LQFS_SET_LOGP(qfsvfsp, ul);
	ml_unit_validate(ul);

	/* remember the state of the log before the log scan */
	logmap_logscan(ul);
	mutex_exit(&qfs_scan_lock);

	/*
	 * Error during scan
	 *
	 * If this is a read/only mount; ignore the error.
	 * At a later time umount/fsck will repair the fs.
	 *
	 */
	if (ul->un_flags & LDL_ERROR) {
		if (!ronly) {
			/*
			 * Aquire the qfs_scan_lock before de-linking
			 * the mtm data structure so that we keep qfs_sync()
			 * and qfs_update() away when they execute the
			 * qfs_scan_inodes() run while we're in progress of
			 * enabling/disabling logging.
			 */
			mutex_enter(&qfs_scan_lock);
			lqfs_unsnarf(qfsvfsp);
			mutex_exit(&qfs_scan_lock);
			return (EIO);
		}
		ul->un_flags &= ~LDL_ERROR;
	}
	if (!ronly) {
		logmap_start_roll(ul);
	}

	return (0);
}

static int
lqfs_initialize(qfsvfs_t *qfsvfsp, daddr_t bno, int ord, size_t nb,
    struct fiolog *flp)
{
	ml_odunit_t	*ud, *ud2;
	buf_t		*bp;
	timeval_lqfs_common_t tv;
	int error = 0;

	/* LINTED: warning: logical expression always true: op "||" */
	ASSERT(sizeof (ml_odunit_t) < DEV_BSIZE);
	ASSERT(nb >= ldl_minlogsize);

	bp = QFS_GETBLK(qfsvfsp, qfsvfsp->mi.m_fs[ord].dev, bno,
	    dbtob(LS_SECTORS));
	bzero(bp->b_un.b_addr, bp->b_bcount);

	ud = (void *)bp->b_un.b_addr;
	ud->od_version = LQFS_VERSION_LATEST;
	ud->od_maxtransfer = MIN(VFS_IOTRANSZ(qfsvfsp), ldl_maxtransfer);
	if (ud->od_maxtransfer < ldl_mintransfer) {
		ud->od_maxtransfer = ldl_mintransfer;
	}
	ud->od_devbsize = DEV_BSIZE;

	ud->od_requestsize = flp->nbytes_actual;
	ud->od_statesize = dbtob(LS_SECTORS);
	ud->od_logsize = nb - ud->od_statesize;

	ud->od_statebno = INT32_C(0);

	uniqtime(&tv);
	if (tv.tv_usec == last_loghead_ident) {
		tv.tv_usec++;
	}
	last_loghead_ident = tv.tv_usec;
	ud->od_head_ident = tv.tv_usec;
	ud->od_tail_ident = ud->od_head_ident;
	ud->od_chksum = ud->od_head_ident + ud->od_tail_ident;

	ud->od_bol_lof = dbtob(ud->od_statebno) + ud->od_statesize;
	ud->od_eol_lof = ud->od_bol_lof + ud->od_logsize;
	ud->od_head_lof = ud->od_bol_lof;
	ud->od_tail_lof = ud->od_bol_lof;

	ASSERT(lqfs_initialize_debug(ud));

	ml_odunit_validate(ud);

	ud2 = (void *)(bp->b_un.b_addr + DEV_BSIZE);
	bcopy(ud, ud2, sizeof (*ud));

	if ((error = SAM_BWRITE2(qfsvfsp, bp)) != 0) {
		brelse(bp);
		return (error);
	}
	brelse(bp);

	return (0);
}

/*
 * Free log space
 *	Assumes the file system is write locked and is not logging
 */
int
lqfs_free(qfsvfs_t *qfsvfsp)
{
	int		error = 0, i, j;
	buf_t		*bp = NULL;
	extent_t	*ep;
	extent_block_t	*ebp;
	fs_lqfs_common_t	*fs = VFS_FS_PTR(qfsvfsp);
	daddr_t		fno;
	int32_t		logbno;
	ushort_t	logord;
	long		nfno;
	inode_t		*ip = NULL;
	char		clean;

	/*
	 * Nothing to free
	 */
	if (LQFS_GET_LOGBNO(fs) == 0) {
		return (0);
	}

	/*
	 * Mark the file system as FSACTIVE and no log but honor the
	 * current value of fs_reclaim.  The reclaim thread could have
	 * been active when lqfs_disable() was called and if fs_reclaim
	 * is reset to zero here it could lead to lost inodes.
	 */
	UL_SBOWNER_SET(qfsvfsp, curthread);
	VFS_LOCK_MUTEX_ENTER(qfsvfsp);
	clean = LQFS_GET_FS_CLEAN(fs);
	logbno = LQFS_GET_LOGBNO(fs);
	logord = LQFS_GET_LOGORD(fs);
	LQFS_SET_FS_CLEAN(fs, FSACTIVE);
	LQFS_SET_LOGBNO(fs, INT32_C(0));
	LQFS_SET_LOGORD(fs, 0);
#ifdef LUFS
	qfs_sbwrite(qfsvfsp);
	error = (qfsvfsp->vfs_bufp->b_flags & B_ERROR);
#else
	error = sam_update_sblk(qfsvfsp, 0, 0, TRUE);
	error = sam_update_sblk(qfsvfsp, 0, 1, TRUE);
#endif /* LUFS */
	VFS_LOCK_MUTEX_EXIT(qfsvfsp);
	UL_SBOWNER_SET(qfsvfsp, -1);
	if (error) {
		error = EIO;
		LQFS_SET_FS_CLEAN(fs, clean);
		LQFS_SET_LOGBNO(fs, logbno);
		LQFS_SET_LOGORD(fs, logord);
		goto errout;
	}

	/*
	 * fetch the allocation block
	 *	superblock -> one block of extents -> log data
	 */
	sam_bread_db(qfsvfsp, qfsvfsp->mi.m_fs[logord].dev,
	    logbtodb(fs, logbno), FS_BSIZE(fs), &bp);
	if (bp->b_flags & B_ERROR) {
		error = EIO;
		goto errout;
	}

#ifdef LUFS
	/*
	 * Free up the allocated space (dummy inode needed for free())
	 */
	ip = qfs_alloc_inode(qfsvfsp, QFSROOTINO);
#else
	/*
	 * QFS doesn't need an inode to free blocks.
	 */
#endif /* LUFS */
	ebp = (void *)bp->b_un.b_addr;
	for (i = 0, ep = &ebp->extents[0]; i < ebp->nextents; ++i, ++ep) {
		fno = logbtofrag(fs, ep->pbno);
		nfno = dbtofsb(fs, ep->nbno);
		for (j = 0; j < nfno; j += FS_FRAG(fs), fno += FS_FRAG(fs)) {
#ifdef LUFS
			free(ip, fno, FS_BSIZE(fs), 0);
#else
			sam_free_block(qfsvfsp, SM, logbtofsblk(fs, fno),
			    ep->ord);
#endif /* LUFS */
		}
	}
#ifdef LUFS
	free(ip, logbtofrag(fs, logbno), FS_BSIZE(fs), 0);
#else
	sam_free_block(qfsvfsp, SM, logbtofsblk(fs, logbno), logord);
#endif /* LUFS */
	brelse(bp);
	bp = NULL;

	/*
	 * Push the metadata dirtied during the allocations
	 */
	UL_SBOWNER_SET(qfsvfsp, curthread);
#ifdef LUFS
	sbupdate(qfsvfsp->vfs_vfs);
#else
	sam_update_sblk(qfsvfsp, 0, 0, TRUE);
#endif /* LUFS */
	UL_SBOWNER_SET(qfsvfsp, -1);
	bflush(qfsvfsp->mi.m_fs[logord].dev);
	error = bfinval(qfsvfsp->mi.m_fs[logord].dev, 0);
	if (error) {
		goto errout;
	}

	/*
	 * Free the dummy inode
	 */
#ifdef LUFS
	qfs_free_inode(ip);
#else
	/* QFS uses a reserved inode */
	VN_RELE(SAM_ITOV(ip));
#endif /* LUFS */

	return (0);

errout:
	/*
	 * Free up all resources
	 */
	if (bp) {
		brelse(bp);
	}
	if (ip) {
#ifdef LUFS
		qfs_free_inode(ip);
#else
		/* QFS uses a reserved inode */
		VN_RELE(SAM_ITOV(ip));
#endif /* LUFS */
	}
	return (error);
}

/*
 * Allocate log space
 *	Assumes the file system is write locked and is not logging
 */
/* ARGSUSED2 */
static int
lqfs_alloc(qfsvfs_t *qfsvfsp, struct fiolog *flp, cred_t *cr)
{
	int		error = 0;
	buf_t		*bp = NULL;
	extent_t	*ep, *nep;
	extent_block_t	*ebp;
	fs_lqfs_common_t	*fs = VFS_FS_PTR(qfsvfsp);
	daddr_t		fno;	/* in frags */
#ifdef LUFS
	daddr_t		bno;	/* in disk blocks */
#endif /* LUFS */
	int32_t		logbno = INT32_C(0);	/* will be fs_logbno */
	ushort_t	logord = 0;		/* will be fs_logord */
	inode_t		*ip = NULL;
#ifndef LUFS
	sam_bn_t	bno;
#endif /* LUFS */
	size_t		nb = flp->nbytes_actual;
	size_t		tb = 0;
	int		ord = 0;
	uchar_t		save_unit;

	/*
	 * Mark the file system as FSACTIVE
	 */
	UL_SBOWNER_SET(qfsvfsp, curthread);
	VFS_LOCK_MUTEX_ENTER(qfsvfsp);
	LQFS_SET_FS_CLEAN(fs, FSACTIVE);
#ifdef LUFS
	qfs_sbwrite(qfsvfsp);
#else
	sam_update_sblk(qfsvfsp, 0, 0, TRUE);
	sam_update_sblk(qfsvfsp, 0, 1, TRUE);
#endif /* LUFS */
	VFS_LOCK_MUTEX_EXIT(qfsvfsp);
	UL_SBOWNER_SET(qfsvfsp, -1);

	/*
	 * Allocate the allocation block (need dummy shadow inode;
	 * we use a shadow inode so the quota sub-system ignores
	 * the block allocations.)
	 *	superblock -> one block of extents -> log data
	 */
#ifdef LUFS
	ip = qfs_alloc_inode(qfsvfsp, QFSROOTINO);
	ip->i_mode = IFSHAD;		/* make the dummy a shadow inode */
#else
	/*
	 * Although QFS has "extension" inodes, it doesn't really implement
	 * the concept of "shadow" inodes.  We can't bypass having the quota
	 * system track the allocation of log blocks.  For now, use the
	 * .inodes inode (SAM_INO_INO) to which we we will allocate space
	 * for a UFS-like log of the requested size.  Allocate space at mount
	 * time BEFORE the quota system is initialized.
	 */
	ip = qfsvfsp->mi.m_inodir;
#endif /* LUFS */

	rw_enter(&ip->i_contents, RW_WRITER);

#ifdef LUFS
	/* Allocate first log block (extent info). */
	fno = contigpref(qfsvfsp, nb + FS_BSIZE(fs));
	error = alloc(ip, fno, FS_BSIZE(fs), &fno, cr);
#else
	save_unit = ip->di.unit;
	ip->di.unit = 0;
	if ((error = sam_alloc_block(ip, SM, &bno, &ord)) == 0) {
		/* Convert 4K block offset to 1K frag offset. */
		fno = fsblktologb(fs, bno);
	}
#endif /* LUFS */
	if (error) {
		goto errout;
	}
	/* Convert 1K frag offset to 512B disk block offset. */
	bno = fsbtodb(fs, fno);
	sam_bread_db(qfsvfsp, qfsvfsp->mi.m_fs[ord].dev, bno,
	    FS_BSIZE(fs), &bp);
	if (bp->b_flags & B_ERROR) {
		error = EIO;
		goto errout;
	}

	ebp = (void *)bp->b_un.b_addr;
	ebp->type = LQFS_EXTENTS;
	ebp->nextbno = UINT32_C(0);
	ebp->nextord = 0;
	ebp->nextents = UINT32_C(0);
	ebp->chksum = INT32_C(0);
#ifdef LUFS
	if (fs->fs_magic == SAM_MAGIC) {
		logbno = bno;
	} else {
#endif /* LUFS */
		logbno = dbtofsb(fs, bno);
#ifdef LUFS
	}
#endif /* LUFS */
	logord = (ushort_t)ord;

	/*
	 * Initialize the first extent
	 */
	ep = &ebp->extents[0];
#ifdef LUFS
	error = alloc(ip, fno + FS_FRAG(fs), FS_BSIZE(fs), &fno, cr);
#else
	ip->di.unit = 0;
	if ((error = sam_alloc_block(ip, SM, &bno, &ord)) == 0) {
		/* Convert 4K block offset to 1K block (frag) offset. */
		fno = fsblktologb(fs, bno);
	}
#endif /* LUFS */
	if (error) {
		goto errout;
	}

	/* Convert frag offset to physical disk block (512B block) offset. */
	bno = fsbtodb(fs, fno);

	ep->lbno = UINT32_C(0);
#ifdef LUFS
	if (fs->fs_magic == SAM_MAGIC) {
		ep->pbno = (uint32_t)bno;
	} else {
#endif /* LUFS */
		ep->pbno = (uint32_t)fno;
#ifdef LUFS
	}
#endif /* LUFS */

	ep->nbno = (uint32_t)fsbtodb(fs, FS_FRAG(fs));	/* Has 8 disk blocks */
	ep->ord = ord;
	ebp->nextents = UINT32_C(1);
	tb = FS_BSIZE(fs);
	nb -= FS_BSIZE(fs);

	while (nb) {
#ifdef LUFS
		error = alloc(ip, fno + FS_FRAG(fs), FS_BSIZE(fs), &fno, cr);
#else
		ip->di.unit = 0;
		error = sam_alloc_block(ip, SM, &bno, &ord);
		if (!error) {
			fno = fsblktologb(fs, bno);
		}
#endif /* LUFS */
		if (error) {
			if (tb < ldl_minlogsize) {
				goto errout;
			}
			error = 0;
			break;
		}
		bno = fsbtodb(fs, fno);
		if ((ep->ord == ord) &&
		    ((daddr_t)((logbtodb(fs, ep->pbno) + ep->nbno) == bno))) {
			ep->nbno += (uint32_t)(fsbtodb(fs, FS_FRAG(fs)));
		} else {
			nep = ep + 1;
			if ((caddr_t)(nep + 1) >
			    (bp->b_un.b_addr + FS_BSIZE(fs))) {
#ifdef LUFS
				free(ip, fno, FS_BSIZE(fs), 0);
#else
				sam_free_block(qfsvfsp, SM,
				    logbtofsblk(fs, fno), ord);
#endif /* LUFS */
				break;
			}
			nep->lbno = ep->lbno + ep->nbno;
#ifdef LUFS
			if (fs->fs_magic == SAM_MAGIC) {
				nep->pbno = (uint32_t)bno;
			} else {
#endif /* LUFS */
				nep->pbno = (uint32_t)fno;
#ifdef LUFS
			}
#endif /* LUFS */
			nep->nbno = (uint32_t)(fsbtodb(fs, FS_FRAG(fs)));
			nep->ord = ord;
			ebp->nextents++;
			ep = nep;
		}
		tb += FS_BSIZE(fs);
		nb -= FS_BSIZE(fs);
	}
	ebp->nbytes = (uint32_t)tb;
	setsum(&ebp->chksum, (int32_t *)(void *)bp->b_un.b_addr, FS_BSIZE(fs));
	if ((error = SAM_BWRITE2(qfsvfsp, bp)) != 0) {
		goto errout;
	}

	/*
	 * Initialize the first two sectors of the log
	 */
	error = lqfs_initialize(qfsvfsp, logbtodb(fs, ebp->extents[0].pbno),
	    ebp->extents[0].ord, tb, flp);
	if (error) {
		goto errout;
	}

	/*
	 * We are done initializing the allocation block and the log
	 */
	brelse(bp);
	bp = NULL;

	/*
	 * Update the superblock and push the dirty metadata
	 */
	UL_SBOWNER_SET(qfsvfsp, curthread);
#ifdef LUFS
	sbupdate(qfsvfsp->vfs_vfs);
#else
	sam_update_sblk(qfsvfsp, 0, 0, TRUE);
#endif /* LUFS */
	UL_SBOWNER_SET(qfsvfsp, -1);

	bflush(qfsvfsp->mi.m_fs[logord].dev);

	error = bfinval(qfsvfsp->mi.m_fs[logord].dev, 1);
	if (error) {
		goto errout;
	}
#ifdef LUFS
	if (qfsvfsp->vfs_bufp->b_flags & B_ERROR) {
		error = EIO;
		goto errout;
	}
#endif /* LUFS */

	/*
	 * Everything is safely on disk; update log space pointer in sb
	 */
	UL_SBOWNER_SET(qfsvfsp, curthread);
	VFS_LOCK_MUTEX_ENTER(qfsvfsp);
	LQFS_SET_LOGBNO(fs, (uint32_t)logbno);
	LQFS_SET_LOGORD(fs, logord);
#ifdef LUFS
	qfs_sbwrite(qfsvfsp);
#else
	sam_update_sblk(qfsvfsp, 0, 0, TRUE);
	sam_update_sblk(qfsvfsp, 0, 1, TRUE);
#endif /* LUFS */
	VFS_LOCK_MUTEX_EXIT(qfsvfsp);
	UL_SBOWNER_SET(qfsvfsp, -1);
	ip->di.unit = save_unit;

	/*
	 * Free the dummy inode
	 */
	rw_exit(&ip->i_contents);
#ifdef LUFS
	qfs_free_inode(ip);
#endif /* LUFS */

	/* inform user of real log size */
	flp->nbytes_actual = tb;
	return (0);

errout:
	/*
	 * Free all resources
	 */
	if (bp) {
		brelse(bp);
	}
	if (logbno) {
		LQFS_SET_LOGBNO(fs, logbno);
		LQFS_SET_LOGORD(fs, logord);
		(void) lqfs_free(qfsvfsp);
	}
	if (ip) {
		ip->di.unit = save_unit;
		rw_exit(&ip->i_contents);
#ifdef LUFS
		qfs_free_inode(ip);
#endif /* LUFS */
	}
	return (error);
}

/* ARGSUSED */
static int
lqfs_log_validate(qfsvfs_t *qfsvfsp, struct fiolog *flp, cred_t *cr)
{
	int		error = 0;
	buf_t		*bp = NULL;
	extent_t	*ep;
	extent_block_t	*ebp;
	fs_lqfs_common_t	*fs = VFS_FS_PTR(qfsvfsp);
	daddr_t		fno;	/* in frags */
#ifdef LUFS
	extent_t	*nep;
	daddr_t		bno;	/* in disk blocks */
#endif /* LUFS */
	int32_t		logbno;
	int		logord;
	int		i;
	int		j;
	long		nfno;

	logbno = LQFS_GET_LOGBNO(fs);
	logord = LQFS_GET_LOGORD(fs);

	if (logbno == 0) {
		error = EINVAL;
		goto errout;
	}

	LQFS_MSG(CE_WARN,
	    "lqfs_log_validate: Extent alloc block offset 0x%x ord %d.\n",
	    (dbtob(logbtodb(fs, logbno))), logord);
	sam_bread_db(qfsvfsp, qfsvfsp->mi.m_fs[logord].dev,
	    logbtodb(fs, logbno), FS_BSIZE(fs), &bp);
	if (bp->b_flags & B_ERROR) {
		LQFS_MSG(CE_WARN,
		    "lqfs_log_validate: Can't read extent alloc block\n");
		error = EIO;
		goto errout;
	}
	ebp = (void *)bp->b_un.b_addr;
	LQFS_MSG(CE_WARN,
	    "lqfs_log_validate: Ext alloc block type 0x%x chksum 0x%x\n",
	    ebp->type, ebp->chksum);
	LQFS_MSG(CE_WARN,
	    "lqfs_log_validate: Ext alloc block nextents 0x%x nb 0x%x.\n",
	    ebp->nextents, ebp->nbytes);
	LQFS_MSG(CE_WARN,
	    "lqfs_log_validate: Ext alloc block nxtbno 0x%x nxtord 0x%x.\n",
	    ebp->nextbno, ebp->nextord);
	for (i = 0, ep = &ebp->extents[0]; i < ebp->nextents; ++i, ++ep) {
		fno = logbtofrag(fs, ep->pbno);
		nfno = dbtofsb(fs, ep->nbno);
		LQFS_MSG(CE_WARN,
		    "   Extent # %d - 0x%x 1K blocks:\n", i, nfno);
		for (j = 0; j < nfno; j += FS_FRAG(fs), fno += FS_FRAG(fs)) {
			int lastord;

			if (j == 0) {
				LQFS_MSG(CE_WARN,
				    "      First 4K block at offset 0x%x "
				    "ord %d (incl. last 1K frag 0x%x, last "
				    "sector 0x%x)\n",
				    logbtofsblk(fs, fno), ep->ord,
				    fno+FS_FRAG(fs)-1,
				    logbtodb(fs, fno+1)-1);
				lastord = ep->ord;
			} else if (j >= (nfno - (FS_FRAG(fs)))) {
				LQFS_MSG(CE_WARN,
				    "      Last 4K block at offset 0x%x "
				    "ord %d (incl. last 1K frag 0x%x, last "
				    "sector 0x%x)\n",
				    logbtofsblk(fs, fno), ep->ord,
				    fno+FS_FRAG(fs)-1,
				    logbtodb(fs, fno+1)-1);
			} else if (ep->ord != lastord) {
				LQFS_MSG(CE_WARN,
				    "      Includes 4K block at offset "
				    "0x%x ord %d (incl. last 1K frag 0x%x, "
				    "last sector 0x%x)\n",
				    logbtofsblk(fs, fno), ep->ord,
				    fno+FS_FRAG(fs)-1,
				    logbtodb(fs, fno+1)-1);
				lastord = ep->ord;
			}
		}
	}
errout:
	if (bp) {
		brelse(bp);
	}

	if (!error) {
		LQFS_MSG(CE_WARN, "lqfs_log_validate(): Returning OK.\n");
	} else {
		LQFS_MSG(CE_WARN,
		    "lqfs_log_validate(): Returning error code %d\n", error);
	}
	return (error);
}


/*
 * Disable logging
 */
int
lqfs_disable(vnode_t *vp, struct fiolog *flp)
{
	int		error = 0;
	inode_t		*ip = VTOI(vp);
	qfsvfs_t	*qfsvfsp = ip->i_qfsvfs;
	fs_lqfs_common_t	*fs = VFS_FS_PTR(qfsvfsp);
#ifdef LUFS
	struct lockfs	lf;
	struct ulockfs	*ulp;
#else
	/* QFS doesn't really support LOCKFS. */
#endif /* LUFS */

	flp->error = FIOLOG_ENONE;

	/*
	 * Logging is already disabled; done
	 */
	if (LQFS_GET_LOGBNO(fs) == 0 || LQFS_GET_LOGP(qfsvfsp) == NULL ||
	    !LQFS_CAPABLE(qfsvfsp)) {
		vfs_setmntopt(qfsvfsp->vfs_vfs, MNTOPT_NOLOGGING, NULL, 0);
		error = 0;
		goto out;
	}

#ifdef LUFS
	/*
	 * File system must be write locked to disable logging
	 */
	error = qfs_fiolfss(vp, &lf);
	if (error) {
		goto out;
	}
	if (!LOCKFS_IS_ULOCK(&lf)) {
		flp->error = FIOLOG_EULOCK;
		error = 0;
		goto out;
	}
	lf.lf_lock = LOCKFS_WLOCK;
	lf.lf_flags = 0;
	lf.lf_comment = NULL;
	error = qfs_fiolfs(vp, &lf, 1);
	if (error) {
		flp->error = FIOLOG_EWLOCK;
		error = 0;
		goto out;
	}
#else
	/* QFS doesn't really support LOCKFS. */
#endif /* LUFS */

	if (LQFS_GET_LOGP(qfsvfsp) == NULL || LQFS_GET_LOGBNO(fs) == 0) {
		goto errout;
	}

	/*
	 * WE ARE COMMITTED TO DISABLING LOGGING PAST THIS POINT
	 */

	/*
	 * Disable logging:
	 * Suspend the reclaim thread and force the delete thread to exit.
	 *	When a nologging mount has completed there may still be
	 *	work for reclaim to do so just suspend this thread until
	 *	it's [deadlock-] safe for it to continue.  The delete
	 *	thread won't be needed as qfs_iinactive() calls
	 *	qfs_delete() when logging is disabled.
	 * Freeze and drain reader ops.
	 *	Commit any outstanding reader transactions (lqfs_flush).
	 *	Set the ``unmounted'' bit in the qfstrans struct.
	 *	If debug, remove metadata from matamap.
	 *	Disable matamap processing.
	 *	NULL the trans ops table.
	 *	Free all of the incore structs related to logging.
	 * Allow reader ops.
	 */
#ifdef LUFS
	qfs_thread_suspend(&qfsvfsp->vfs_reclaim);
	qfs_thread_exit(&qfsvfsp->vfs_delete);
#else
	/* QFS doesn't have file reclaim nor i-node delete threads. */
#endif /* LUFS */

	vfs_lock_wait(qfsvfsp->vfs_vfs);
#ifdef LQFS_TODO_LOCKFS
	ulp = &qfsvfsp->vfs_ulockfs;
	mutex_enter(&ulp->ul_lock);
	(void) qfs_quiesce(ulp);
#else
	/* QFS doesn't really support LOCKFS. */
#endif /* LQFS_TODO_LOCKFS */

#ifdef LQFS_TODO
	(void) qfs_flush(qfsvfsp->vfs_vfs);
#else
	(void) lqfs_flush(qfsvfsp);
	if (LQFS_GET_LOGP(qfsvfsp)) {
		logmap_start_roll(LQFS_GET_LOGP(qfsvfsp));
	}
#endif /* LQFS_TODO */

	TRANS_MATA_UMOUNT(qfsvfsp);
	LQFS_SET_DOMATAMAP(qfsvfsp, 0);

	/*
	 * Free all of the incore structs
	 * Aquire the ufs_scan_lock before de-linking the mtm data
	 * structure so that we keep ufs_sync() and ufs_update() away
	 * when they execute the ufs_scan_inodes() run while we're in
	 * progress of enabling/disabling logging.
	 */
	mutex_enter(&qfs_scan_lock);
	(void) lqfs_unsnarf(qfsvfsp);
	mutex_exit(&qfs_scan_lock);

#ifdef LQFS_TODO_LOCKFS
	atomic_add_long(&ufs_quiesce_pend, -1);
	mutex_exit(&ulp->ul_lock);
#else
	/* QFS doesn't do this yet. */
#endif /* LQFS_TODO_LOCKFS */
	vfs_setmntopt(qfsvfsp->vfs_vfs, MNTOPT_NOLOGGING, NULL, 0);
	vfs_unlock(qfsvfsp->vfs_vfs);

	LQFS_SET_FS_ROLLED(fs, FS_ALL_ROLLED);
	LQFS_SET_NOLOG_SI(qfsvfsp, 0);

	/*
	 * Free the log space and mark the superblock as FSACTIVE
	 */
	(void) lqfs_free(qfsvfsp);

#ifdef LUFS
	/*
	 * Allow the reclaim thread to continue.
	 */
	qfs_thread_continue(&qfsvfsp->vfs_reclaim);
#else
	/* QFS doesn't have a file reclaim thread. */
#endif /* LUFS */

#ifdef LQFS_TODO_LOCKFS
	/*
	 * Unlock the file system
	 */
	lf.lf_lock = LOCKFS_ULOCK;
	lf.lf_flags = 0;
	error = qfs_fiolfs(vp, &lf, 1);
	if (error) {
		flp->error = FIOLOG_ENOULOCK;
	}
#else
	/* QFS doesn't really support LOCKFS. */
#endif /* LQFS_LOCKFS */

	error = 0;
	goto out;

errout:
#ifdef LQFS_LOCKFS
	lf.lf_lock = LOCKFS_ULOCK;
	lf.lf_flags = 0;
	(void) qfs_fiolfs(vp, &lf, 1);
#else
	/* QFS doesn't really support LOCKFS. */
#endif /* LQFS_LOCKFS */

out:
	mutex_enter(&ip->mp->ms.m_waitwr_mutex);
	ip->mp->mt.fi_status |= FS_LOGSTATE_KNOWN;
	mutex_exit(&ip->mp->ms.m_waitwr_mutex);
	return (error);
}

/*
 * Enable logging
 */
int
lqfs_enable(struct vnode *vp, struct fiolog *flp, cred_t *cr)
{
	int		error;
	inode_t		*ip = VTOI(vp);
	qfsvfs_t	*qfsvfsp = ip->i_qfsvfs;
	fs_lqfs_common_t	*fs = VFS_FS_PTR(qfsvfsp);
	ml_unit_t	*ul;
#ifdef LQFS_TODO_LOCKFS
	int		reclaim = 0;
	struct lockfs	lf;
	struct ulockfs	*ulp;
#else
	/* QFS doesn't really support LOCKFS. */
#endif /* LQFS_TODO_LOCKFS */
	vfs_t		*vfsp = qfsvfsp->vfs_vfs;
	uint64_t	tmp_nbytes_actual;
	char fsclean;
	sam_sblk_t	*sblk = qfsvfsp->mi.m_sbp;

	/*
	 * File system is not capable of logging.
	 */
	if (!LQFS_CAPABLE(qfsvfsp)) {
		flp->error = FIOLOG_ENOTSUP;
		error = 0;
		goto out;
	}
	if (!SAM_MAGIC_V2A_OR_HIGHER(&sblk->info.sb)) {
		cmn_err(CE_WARN, "SAM-QFS: %s: Not enabling logging, "
		    " file system is not version 2A.", qfsvfsp->mt.fi_name);
		cmn_err(CE_WARN, "\tUpgrade file system with samfsck -u "
		    "first.");
		flp->error = FIOLOG_ENOTSUP;
		error = 0;
		goto out;
	}

	if (LQFS_GET_LOGBNO(fs)) {
		error = lqfs_log_validate(qfsvfsp, flp, cr);
	}

	/*
	 * Check if logging is already enabled
	 */
	if (LQFS_GET_LOGP(qfsvfsp)) {
		flp->error = FIOLOG_ETRANS;
		/* for root ensure logging option is set */
		vfs_setmntopt(vfsp, MNTOPT_LOGGING, NULL, 0);
		error = 0;
		goto out;
	}

	/*
	 * Come back here to recheck if we had to disable the log.
	 */
recheck:
	error = 0;
	flp->error = FIOLOG_ENONE;

	/*
	 * Adjust requested log size
	 */
	flp->nbytes_actual = flp->nbytes_requested;
	if (flp->nbytes_actual == 0) {
		tmp_nbytes_actual =
		    (((uint64_t)FS_SIZE(fs)) / ldl_divisor) << FS_FSHIFT(fs);
		flp->nbytes_actual = (uint_t)MIN(tmp_nbytes_actual, INT_MAX);
	}
	flp->nbytes_actual = MAX(flp->nbytes_actual, ldl_minlogsize);
	flp->nbytes_actual = MIN(flp->nbytes_actual, ldl_maxlogsize);
	flp->nbytes_actual = blkroundup(fs, flp->nbytes_actual);

	/*
	 * logging is enabled and the log is the right size; done
	 */
	ul = LQFS_GET_LOGP(qfsvfsp);

	if (ul && LQFS_GET_LOGBNO(fs) &&
	    (flp->nbytes_actual == ul->un_requestsize)) {
		vfs_setmntopt(vfsp, MNTOPT_LOGGING, NULL, 0);
		error = 0;
		goto out;
	}

	/*
	 * Readonly file system
	 */
	if (FS_RDONLY(fs)) {
		flp->error = FIOLOG_EROFS;
		error = 0;
		goto out;
	}

#ifdef LQFS_TODO_LOCKFS
	/*
	 * File system must be write locked to enable logging
	 */
	error = qfs_fiolfss(vp, &lf);
	if (error) {
		goto out;
	}
	if (!LOCKFS_IS_ULOCK(&lf)) {
		flp->error = FIOLOG_EULOCK;
		error = 0;
		goto out;
	}
	lf.lf_lock = LOCKFS_WLOCK;
	lf.lf_flags = 0;
	lf.lf_comment = NULL;
	error = qfs_fiolfs(vp, &lf, 1);
	if (error) {
		flp->error = FIOLOG_EWLOCK;
		error = 0;
		goto out;
	}
#else
	/* QFS doesn't really support lockfs. */
#endif /* LQFS_TODO_LOCKFS */

	/*
	 * Grab appropriate locks to synchronize with the rest
	 * of the system
	 */
	vfs_lock_wait(vfsp);
#ifdef LQFS_TODO_LOCKFS
	ulp = &ufsvfsp->vfs_ulockfs;
	mutex_enter(&ulp->ul_lock);
#else
	/* QFS doesn't really support lockfs. */
#endif /* LQFS_TODO_LOCKFS */

	/*
	 * File system must be fairly consistent to enable logging
	 */
	fsclean = LQFS_GET_FS_CLEAN(fs);
	if (fsclean != FSLOG &&
	    fsclean != FSACTIVE &&
	    fsclean != FSSTABLE &&
	    fsclean != FSCLEAN) {
		flp->error = FIOLOG_ECLEAN;
		goto unlockout;
	}

#ifdef LUFS
	/*
	 * A write-locked file system is only active if there are
	 * open deleted files; so remember to set FS_RECLAIM later.
	 */
	if (LQFS_GET_FS_CLEAN(fs) == FSACTIVE) {
		reclaim = FS_RECLAIM;
	}
#else
	/* QFS doesn't have a reclaim file thread. */
#endif /* LUFS */

	/*
	 * Logging is already enabled; must be changing the log's size
	 */
	if (LQFS_GET_LOGBNO(fs) && LQFS_GET_LOGP(qfsvfsp)) {
#ifdef LQFS_TODO_LOCKFS
		/*
		 * Before we can disable logging, we must give up our
		 * lock.  As a consequence of unlocking and disabling the
		 * log, the fs structure may change.  Because of this, when
		 * disabling is complete, we will go back to recheck to
		 * repeat all of the checks that we performed to get to
		 * this point.  Disabling sets fs->fs_logbno to 0, so this
		 * will not put us into an infinite loop.
		 */
		mutex_exit(&ulp->ul_lock);
#else
		/* QFS doesn't really support lockfs. */
#endif /* LQFS_TODO_LOCKFS */
		vfs_unlock(vfsp);

#ifdef LQFS_TODO_LOCKFS
		lf.lf_lock = LOCKFS_ULOCK;
		lf.lf_flags = 0;
		error = qfs_fiolfs(vp, &lf, 1);
		if (error) {
			flp->error = FIOLOG_ENOULOCK;
			error = 0;
			goto out;
		}
#else
		/* QFS doesn't really support lockfs. */
#endif /* LQFS_TODO_LOCKFS */
		error = lqfs_disable(vp, flp);
		if (error || (flp->error != FIOLOG_ENONE)) {
			error = 0;
			goto out;
		}
		goto recheck;
	}

	error = lqfs_alloc(qfsvfsp, flp, cr);
	if (error) {
		goto errout;
	}
#ifdef LUFS
#else
	if ((error = lqfs_log_validate(qfsvfsp, flp, cr)) != 0) {
		goto errout;
	}
#endif /* LUFS */

	/*
	 * Create all of the incore structs
	 */
	error = lqfs_snarf(qfsvfsp, fs, 0);
	if (error) {
		goto errout;
	}

	/*
	 * DON'T ``GOTO ERROUT'' PAST THIS POINT
	 */

	/*
	 * Pretend we were just mounted with logging enabled
	 *	freeze and drain the file system of readers
	 *		Get the ops vector
	 *		If debug, record metadata locations with log subsystem
	 *		Start the delete thread
	 *		Start the reclaim thread, if necessary
	 *	Thaw readers
	 */
	vfs_setmntopt(vfsp, MNTOPT_LOGGING, NULL, 0);

	TRANS_DOMATAMAP(qfsvfsp);
	TRANS_MATA_MOUNT(qfsvfsp);
	TRANS_MATA_SI(qfsvfsp, fs);

#ifdef LUFS
	qfs_thread_start(&qfsvfsp->vfs_delete, qfs_thread_delete, vfsp);
	if (fs->fs_reclaim & (FS_RECLAIM|FS_RECLAIMING)) {
		fs->fs_reclaim &= ~FS_RECLAIM;
		fs->fs_reclaim |=  FS_RECLAIMING;
		qfs_thread_start(&qfsvfsp->vfs_reclaim,
		    qfs_thread_reclaim, vfsp);
	} else {
		fs->fs_reclaim |= reclaim;
	}
#else
	/* QFS doesn't have file reclaim nor i-node delete threads. */
#endif /* LUFS */

#ifdef LUFS
	mutex_exit(&ulp->ul_lock);
#else
	/* QFS doesn't really support LOCKFS. */
#endif /* LUFS */

	vfs_unlock(vfsp);

#ifdef LQFS_TODO_LOCKFS
	/*
	 * Unlock the file system
	 */
	lf.lf_lock = LOCKFS_ULOCK;
	lf.lf_flags = 0;
	error = qfs_fiolfs(vp, &lf, 1);
	if (error) {
		flp->error = FIOLOG_ENOULOCK;
		error = 0;
		goto out;
	}
#else
	/* QFS doesn't really support LOCKFS. */
#endif /* LQFS_TODO_LOCKFS */

	/*
	 * There's nothing in the log yet (we've just allocated it)
	 * so directly write out the super block.
	 * Note, we have to force this sb out to disk
	 * (not just to the log) so that if we crash we know we are logging
	 */
	VFS_LOCK_MUTEX_ENTER(qfsvfsp);
	LQFS_SET_FS_CLEAN(fs, FSLOG);
	LQFS_SET_FS_ROLLED(fs, FS_NEED_ROLL);	/* Mark the fs as unrolled */
#ifdef LUFS
	QFS_BWRITE2(NULL, qfsvfsp->vfs_bufp);
#else
	sam_update_sblk(qfsvfsp, 0, 0, TRUE);
#endif /* LUFS */
	VFS_LOCK_MUTEX_EXIT(qfsvfsp);

	error = 0;
	goto out;

errout:
	/*
	 * Aquire the qfs_scan_lock before de-linking the mtm data
	 * structure so that we keep qfs_sync() and qfs_update() away
	 * when they execute the ufs_scan_inodes() run while we're in
	 * progress of enabling/disabling logging.
	 */
	mutex_enter(&qfs_scan_lock);
	(void) lqfs_unsnarf(qfsvfsp);
	mutex_exit(&qfs_scan_lock);

	(void) lqfs_free(qfsvfsp);
unlockout:
#ifdef LQFS_TODO_LOCKFS
	mutex_exit(&ulp->ul_lock);
#else
	/* QFS doesn't really support LOCKFS. */
#endif /* LQFS_TODO_LOCKFS */

	vfs_unlock(vfsp);

#ifdef LQFS_TODO_LOCKFS
	lf.lf_lock = LOCKFS_ULOCK;
	lf.lf_flags = 0;
	(void) qfs_fiolfs(vp, &lf, 1);
#else
	/* QFS doesn't really support LOCKFS. */
#endif /* LQFS_TODO_LOCKFS */

out:
	mutex_enter(&ip->mp->ms.m_waitwr_mutex);
	ip->mp->mt.fi_status |= FS_LOGSTATE_KNOWN;
	mutex_exit(&ip->mp->ms.m_waitwr_mutex);
	return (error);
}

void
lqfs_read_strategy(ml_unit_t *ul, buf_t *bp)
{
	mt_map_t	*logmap	= ul->un_logmap;
	offset_t	mof	= ldbtob(bp->b_blkno);
	off_t		nb	= bp->b_bcount;
	mapentry_t	*age;
	char		*va;
	int		(*saviodone)();
	int		entire_range;
	uchar_t		ord;
	qfsvfs_t	*qfsvfsp = ul->un_qfsvfs;

	/*
	 * get a linked list of overlapping deltas
	 * returns with &mtm->mtm_rwlock held
	 */
	ord = lqfs_find_ord(qfsvfsp, bp);
	entire_range = logmap_list_get(logmap, mof, ord, nb, &age);

	/*
	 * no overlapping deltas were found; read master
	 */
	if (age == NULL) {
		rw_exit(&logmap->mtm_rwlock);
		if (ul->un_flags & LDL_ERROR) {
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			biodone(bp);
		} else {
			LQFS_SET_IOTSTAMP(ul->un_qfsvfs, ddi_get_lbolt());
			logstats.ls_lreads.value.ui64++;
			if ((bp->b_flags & B_READ) == 0) {
				LQFS_MSG(CE_WARN, "lqfs_read_strategy(): "
				    "bdev_strategy writing mof 0x%x "
				    "edev %ld nb %d\n", bp->b_blkno * 512,
				    bp->b_edev, bp->b_bcount);
			} else {
				LQFS_MSG(CE_WARN, "lqfs_read_strategy(): "
				    "bdev_strategy reading mof 0x%x "
				    "edev %ld nb %d\n", bp->b_blkno * 512,
				    bp->b_edev, bp->b_bcount);
			}
			(void) bdev_strategy(bp);
#ifdef LQFS_TODO_STATS
			lwp_stat_update(LWP_STAT_INBLK, 1);
#endif /* LQFS_TODO_STATS */
		}
		return;
	}

	va = bp_mapin_common(bp, VM_SLEEP);
	/*
	 * if necessary, sync read the data from master
	 *	errors are returned in bp
	 */
	if (!entire_range) {
		saviodone = bp->b_iodone;
		bp->b_iodone = trans_not_done;
		logstats.ls_mreads.value.ui64++;
		if ((bp->b_flags & B_READ) == 0) {
			LQFS_MSG(CE_WARN, "lqfs_read_strategy(): "
			    "bdev_strategy writing mof 0x%x edev %ld "
			    "nb %d\n", bp->b_blkno * 512, bp->b_edev,
			    bp->b_bcount);
		} else {
			LQFS_MSG(CE_WARN, "lqfs_read_strategy(): "
			    "bdev_strategy reading mof 0x%x edev %ld "
			    "nb %d\n", bp->b_blkno * 512, bp->b_edev,
			    bp->b_bcount);
		}
		(void) bdev_strategy(bp);
#ifdef LQFS_TODO_STATS
		lwp_stat_update(LWP_STAT_INBLK, 1);
#endif /* LQFS_TODO_STATS */
		if (trans_not_wait(bp)) {
			ldl_seterror(ul, "Error reading master");
		}
		bp->b_iodone = saviodone;
	}

	/*
	 * sync read the data from the log
	 *	errors are returned inline
	 */
	if (ldl_read(ul, va, mof, ord, nb, age)) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
	}

	/*
	 * unlist the deltas
	 */
	logmap_list_put(logmap, age);

	/*
	 * all done
	 */
	if (ul->un_flags & LDL_ERROR) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
	}
	biodone(bp);
}

void
lqfs_write_strategy(ml_unit_t *ul, buf_t *bp)
{
	offset_t	mof	= ldbtob(bp->b_blkno);
	off_t		nb	= bp->b_bcount;
	char		*va;
	mapentry_t	*me;
	uchar_t		ord;
	qfsvfs_t	*qfsvfsp = ul->un_qfsvfs;
#ifdef LUFS
#else
	caddr_t		buf;

	va = bp_mapin_common(bp, VM_SLEEP);
	buf = bp->b_un.b_addr;
#endif /* LUFS */

	ASSERT((nb & DEV_BMASK) == 0);
	ul->un_logmap->mtm_ref = 1;

	/*
	 * if there are deltas, move into log
	 */
	ord = lqfs_find_ord(qfsvfsp, bp);
#ifdef LUFS
	me = deltamap_remove(ul->un_deltamap, mof, ord, nb);
	if (me) {
		va = bp_mapin_common(bp, VM_SLEEP);

		ASSERT(((ul->un_debug & MT_WRITE_CHECK) == 0) ||
		    (ul->un_matamap == NULL)||
		    matamap_within(ul->un_matamap, mof, ord, nb));

		/*
		 * move to logmap
		 */
		if (qfs_crb_enable) {
			logmap_add_buf(ul, va, mof, ord, me,
			    bp->b_un.b_addr, nb);
		} else {
			logmap_add(ul, va, mof, ord, me);
		}

		if (ul->un_flags & LDL_ERROR) {
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
		}
		biodone(bp);
		return;
	}
#else
	if (buf && qfs_crb_enable) {
		uint32_t	bufsz;
		offset_t	vamof;
		offset_t	hmof;
		uchar_t		vaord;
		uchar_t		hord;
		uint32_t	hnb, nb1;

		bufsz = bp->b_bcount;
		ASSERT((bufsz & DEV_BMASK) == 0);

		vamof = mof;
		vaord = ord;

		/*
		 * Move any deltas to the logmap. Split requests that
		 * straddle MAPBLOCKSIZE hash boundaries (i.e. summary info).
		 */
		for (hmof = vamof - (va - buf), nb1 = nb; bufsz;
		    bufsz -= hnb, hmof += hnb, buf += hnb, nb1 -= hnb) {
			hnb = MAPBLOCKSIZE - (hmof & MAPBLOCKOFF);
			if (hnb > bufsz) {
				hnb = bufsz;
			}
			LQFS_MSG(CE_WARN, "lqfs_write_strategy(): Removing "
			    "deltamap deltas within mof 0x%llx ord %d nb %d\n",
			    MAX(hmof, vamof), vaord, MIN(hnb, nb1));
			me = deltamap_remove(ul->un_deltamap,
			    MAX(hmof, vamof), vaord, MIN(hnb, nb1));
			hord = vaord;
			if (me) {
				logmap_add_buf(ul, va, hmof, hord,
				    me, buf, hnb);

				if (ul->un_flags & LDL_ERROR) {
					bp->b_flags |= B_ERROR;
					bp->b_error = EIO;
				}
				biodone(bp);
				return;
			}
		}
	} else {
		/*
		 * if there are deltas
		 */
		LQFS_MSG(CE_WARN, "lqfs_write_strategy(): Removing "
		    "deltamap deltas within mof 0x%x ord %d nb %d\n",
		    mof, ord, nb);
		me = deltamap_remove(ul->un_deltamap, mof, ord, nb);
		if (me) {
			ASSERT(((ul->un_debug & MT_WRITE_CHECK) == 0) ||
			    (ul->un_matamap == NULL)||
			    matamap_within(ul->un_matamap, mof, ord, nb));

			/*
			 * move to logmap
			 */
			logmap_add(ul, va, mof, ord, me);

			if (ul->un_flags & LDL_ERROR) {
				bp->b_flags |= B_ERROR;
				bp->b_error = EIO;
			}
			biodone(bp);
			return;
		}
	}
#endif /* LUFS */
	if (ul->un_flags & LDL_ERROR) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		biodone(bp);
		return;
	}

	/*
	 * Check that we are not updating metadata, or if so then via B_PHYS.
	 */
	ASSERT((ul->un_matamap == NULL) ||
	    !(matamap_overlap(ul->un_matamap, mof, ord, nb) &&
	    ((bp->b_flags & B_PHYS) == 0)));

	LQFS_SET_IOTSTAMP(ul->un_qfsvfs, ddi_get_lbolt());
	logstats.ls_lwrites.value.ui64++;

#ifdef LQFS_TODO_SNAPSHOT
	/* If snapshots are enabled, write through the snapshot driver */
	if (ul->un_qfsvfs->vfs_snapshot) {
		fssnap_strategy(&ul->un_qfsvfs->vfs_snapshot, bp);
	} else {
#endif /* LQFS_TODO_SNAPSHOT */
		if ((bp->b_flags & B_READ) == 0) {
			LQFS_MSG(CE_WARN, "lqfs_write_strategy(): "
			    "bdev_strategy writing mof 0x%x edev %ld "
			    "nb %d\n", bp->b_blkno * 512, bp->b_edev,
			    bp->b_bcount);
		} else {
			LQFS_MSG(CE_WARN, "lqfs_write_strategy(): "
			    "bdev_strategy reading mof 0x%x edev %ld "
			    "nb %d\n", bp->b_blkno * 512, bp->b_edev,
			    bp->b_bcount);
		}
		(void) bdev_strategy(bp);
#ifdef LQFS_TODO_SNAPSHOT
	}
#endif /* LQFS_TODO_SNAPSHOT */

#ifdef LQFS_TODO_STATS
	lwp_stat_update(LWP_STAT_OUBLK, 1);
#endif /* LQFS_TODO_STATS */
}

void
lqfs_strategy(ml_unit_t *ul, buf_t *bp)
{
	if (bp->b_flags & B_READ) {
		lqfs_read_strategy(ul, bp);
	} else {
		lqfs_write_strategy(ul, bp);
	}
}

/* ARGSUSED */
static int
delta_stats_update(kstat_t *ksp, int rw)
{
	if (rw == KSTAT_WRITE) {
		delta_stats[DT_SB] = dkstats.ds_superblock_deltas.value.ui64;
		delta_stats[DT_CG] = dkstats.ds_bitmap_deltas.value.ui64;
		delta_stats[DT_SI] = dkstats.ds_suminfo_deltas.value.ui64;
		delta_stats[DT_AB] = dkstats.ds_allocblk_deltas.value.ui64;
		delta_stats[DT_ABZERO] = dkstats.ds_ab0_deltas.value.ui64;
		delta_stats[DT_DIR] = dkstats.ds_dir_deltas.value.ui64;
		delta_stats[DT_INODE] = dkstats.ds_inode_deltas.value.ui64;
		delta_stats[DT_FBI] = dkstats.ds_fbiwrite_deltas.value.ui64;
		delta_stats[DT_QR] = dkstats.ds_quota_deltas.value.ui64;
		delta_stats[DT_SHAD] = dkstats.ds_shadow_deltas.value.ui64;

		roll_stats[DT_SB] = dkstats.ds_superblock_rolled.value.ui64;
		roll_stats[DT_CG] = dkstats.ds_bitmap_rolled.value.ui64;
		roll_stats[DT_SI] = dkstats.ds_suminfo_rolled.value.ui64;
		roll_stats[DT_AB] = dkstats.ds_allocblk_rolled.value.ui64;
		roll_stats[DT_ABZERO] = dkstats.ds_ab0_rolled.value.ui64;
		roll_stats[DT_DIR] = dkstats.ds_dir_rolled.value.ui64;
		roll_stats[DT_INODE] = dkstats.ds_inode_rolled.value.ui64;
		roll_stats[DT_FBI] = dkstats.ds_fbiwrite_rolled.value.ui64;
		roll_stats[DT_QR] = dkstats.ds_quota_rolled.value.ui64;
		roll_stats[DT_SHAD] = dkstats.ds_shadow_rolled.value.ui64;
	} else {
		dkstats.ds_superblock_deltas.value.ui64 = delta_stats[DT_SB];
		dkstats.ds_bitmap_deltas.value.ui64 = delta_stats[DT_CG];
		dkstats.ds_suminfo_deltas.value.ui64 = delta_stats[DT_SI];
		dkstats.ds_allocblk_deltas.value.ui64 = delta_stats[DT_AB];
		dkstats.ds_ab0_deltas.value.ui64 = delta_stats[DT_ABZERO];
		dkstats.ds_dir_deltas.value.ui64 = delta_stats[DT_DIR];
		dkstats.ds_inode_deltas.value.ui64 = delta_stats[DT_INODE];
		dkstats.ds_fbiwrite_deltas.value.ui64 = delta_stats[DT_FBI];
		dkstats.ds_quota_deltas.value.ui64 = delta_stats[DT_QR];
		dkstats.ds_shadow_deltas.value.ui64 = delta_stats[DT_SHAD];

		dkstats.ds_superblock_rolled.value.ui64 = roll_stats[DT_SB];
		dkstats.ds_bitmap_rolled.value.ui64 = roll_stats[DT_CG];
		dkstats.ds_suminfo_rolled.value.ui64 = roll_stats[DT_SI];
		dkstats.ds_allocblk_rolled.value.ui64 = roll_stats[DT_AB];
		dkstats.ds_ab0_rolled.value.ui64 = roll_stats[DT_ABZERO];
		dkstats.ds_dir_rolled.value.ui64 = roll_stats[DT_DIR];
		dkstats.ds_inode_rolled.value.ui64 = roll_stats[DT_INODE];
		dkstats.ds_fbiwrite_rolled.value.ui64 = roll_stats[DT_FBI];
		dkstats.ds_quota_rolled.value.ui64 = roll_stats[DT_QR];
		dkstats.ds_shadow_rolled.value.ui64 = roll_stats[DT_SHAD];
	}
	return (0);
}

extern size_t qfs_crb_limit;
extern int qfs_max_crb_divisor;
extern uint_t topkey;

void
lqfs_init(void)
{
	/* Create kmem caches */
	lqfs_sv = kmem_cache_create("lqfs_save", sizeof (lqfs_save_t), 0,
	    NULL, NULL, NULL, NULL, NULL, 0);
	lqfs_bp = kmem_cache_create("lqfs_bufs", sizeof (lqfs_buf_t), 0,
	    NULL, NULL, NULL, NULL, NULL, 0);

	sam_mutex_init(&log_mutex, NULL, MUTEX_DEFAULT, NULL);

	_init_top();

	if (&bio_lqfs_strategy != NULL) {
		bio_lqfs_strategy = (void (*) (void *, buf_t *)) lqfs_strategy;
	}

	/*
	 * Initialise general logging and delta kstats
	 */
	lqfs_logstats = kstat_create("sam-qfs", 0, "logstats", "fs",
	    KSTAT_TYPE_NAMED, sizeof (logstats) / sizeof (kstat_named_t),
	    KSTAT_FLAG_VIRTUAL);
	if (lqfs_logstats) {
		lqfs_logstats->ks_data = (void *) &logstats;
		kstat_install(lqfs_logstats);
	}

	lqfs_deltastats = kstat_create("sam-qfs", 0, "deltastats", "fs",
	    KSTAT_TYPE_NAMED, sizeof (dkstats) / sizeof (kstat_named_t),
	    KSTAT_FLAG_VIRTUAL);
	if (lqfs_deltastats) {
		lqfs_deltastats->ks_data = (void *) &dkstats;
		lqfs_deltastats->ks_update = delta_stats_update;
		kstat_install(lqfs_deltastats);
	}

	/*
	 * Set up the maximum amount of kmem that the crbs (system wide)
	 * can use.
	 */
	qfs_crb_limit = kmem_maxavail() / qfs_max_crb_divisor;
}

int
lqfs_fini(void)
{
	if (lqfs_sv != NULL) {
		kmem_cache_destroy(lqfs_sv);
	}
	if (lqfs_bp != NULL) {
		kmem_cache_destroy(lqfs_bp);
	}
	if (mapentry_cache != NULL) {
		kmem_cache_destroy(mapentry_cache);
	}
	if (lqfs_logstats != NULL) {
		kstat_delete(lqfs_logstats);
		lqfs_logstats = NULL;
	}
	if (lqfs_deltastats != NULL) {
		kstat_delete(lqfs_deltastats);
		lqfs_deltastats = NULL;
	}

	ASSERT(topkey);
	tsd_destroy(&topkey);
	topkey = 0;

	return (0);
}
