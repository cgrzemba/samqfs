/*
 * ----- samfs/quota.c - Process the sam quota functions.
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

#pragma ident "$Revision: 1.78 $"

#include "sam/osversion.h"

/*
 * #define	MEM_DEBUG
 */
#define	MAX(x, y)	(((x) > (y)) ? (x) : (y))

#ifdef MEM_DEBUG
#define	KMA(x, y)	sam_quota_local_kma(x, y)
#define	KMF(x, y)	sam_quota_local_kmf(x, y)
#define	OPTRACE(w, x, y, z)	TRACE(T_SAM_QUOTA_OP, w, x, y, z)
#else
#define	KMA(x, y)	kmem_zalloc(x, y)
#define	KMF(x, y)	kmem_free(x, y)
#define	OPTRACE(w, x, y, z)
#endif

#define	NOW()	(SAM_SECOND())
#define	uwarn	uprintf

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/flock.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ksynch.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/policy.h>

/* ----- SAMFS Includes */

#include "sam/types.h"
#include "sam/param.h"
#include "inode.h"
#include "mount.h"
#include "rwio.h"
#include "extern.h"
#include "debug.h"
#include "macros.h"
#include "trace.h"
#include "quota.h"
#include "samfs.h"
#include "sam/syscall.h"
#include "qfs_log.h"


static void
b_log_AND(char *d, char *s, int n)
{
	for (; n--; d++, s++)
		*d = *d & *s;
}

static void
b_log_CAND(char *d, char *s, int n)
{
	for (; n--; d++, s++)
		*d = *d & ~*s;
}

static void
b_log_OR(char *d, char *s, int n)
{
	for (; n--; d++, s++)
		*d = *d | *s;
}


/*
 * ----- sam_quota_op -
 * Process all quota operations except stat.
 */

int
sam_quota_op(struct sam_quota_arg *argp)
{
	struct sam_quota_arg args;
	int err = 0;
	file_t *fp = NULL;
	sam_node_t *ip = NULL;
	struct sam_quot *uqp;
	struct sam_dquot dq;
	cred_t *credp = CRED();

	OPTRACE(NULL, __LINE__, (int)argp, 0);

	if (copyin(argp, &args, sizeof (args))) {
		TRACE(T_SAM_QUOTA_OPERR, NULL, __LINE__,
		    (sam_tr_t)argp, EFAULT);
		return (EFAULT);
	}

	OPTRACE(NULL, __LINE__, args.qcmd, args.qflags);
	if (args.qsize != sizeof (struct sam_quota_arg)) {
		TRACE(T_SAM_QUOTA_OPERR, NULL, __LINE__,
		    (int)args.qsize, EFAULT);
		return (EFAULT);
	}

	if (args.qcmd != SAM_QOP_PUT && args.qcmd != SAM_QOP_PUTALL) {
		TRACE(T_SAM_QUOTA_OPERR, NULL, __LINE__,
		    (int)args.qcmd, EINVAL);
		return (EINVAL);
	}

	if ((fp = getf(args.qfd)) == NULL) {
		TRACE(T_SAM_QUOTA_OPERR, NULL, __LINE__, (int)args.qfd, EBADF);
		return (EBADF);
	}

	if (SAM_VNODE_IS_NOTSAMFS(fp->f_vnode) ||
	    !(ip = SAM_VTOI(fp->f_vnode))) {
		TRACE(T_SAM_QUOTA_OPERR, NULL, __LINE__, 0, ENOTSUP);
		err = ENOTSUP;
		goto out;
	}

	if (secpolicy_fs_quota(credp, ip->mp->mi.m_vfsp)) {
		TRACE(T_SAM_QUOTA_OPERR, NULL, __LINE__, 0, EPERM);
		err = EPERM;
		goto out;
	}

	if (args.qtype >= SAM_QUOTA_DEFD) {
		TRACE(T_SAM_QUOTA_OPERR, fp->f_vnode, __LINE__,
		    args.qtype, EINVAL);
		err = EINVAL;
		goto out;
	}

	if ((args.qflags&SAM_QFL_INDEX) == 0) {
		args.qindex = sam_quota_get_index(ip, args.qtype);
	}

	uqp = (void *)args.qp.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		uqp = (void *)args.qp.p64;
	}

	if (copyin(uqp, &dq, sizeof (dq))) {
		err = EFAULT;
		goto out;
	}

	OPTRACE(fp->f_vnode, __LINE__, args.qcmd, args.qflags);
	if ((err = sam_quota_set_info(
	    ip, &dq, args.qcmd, args.qtype, args.qindex)) != 0) {
		goto out;
	}
	if (copyout(&args, argp, sizeof (args))) {
		err = EFAULT;
	}

out:
	OPTRACE(fp->f_vnode, __LINE__, args.qcmd, err);
	if (fp) {
		releasef(args.qfd);
	}
	return (err);
}


/*
 * mask of fields not to be updated.
 *
 * These fields are the "in use" fields, the live counts and timers
 * updated by the kernel during normal operation.  The masked fields
 * are only updated by special request (SAM_QOP_PUTALL vs. SAM_QOP_PUT).
 */
static struct sam_dquot sam_usemask;

/*
 * sam_quota_set_info - set quota entry's fields from user record
 *
 * Privileged call.  Handle both the client and server cases.
 */
int
sam_quota_set_info(sam_node_t *ip, struct sam_dquot *nqdp, int op, int type,
    int index)
{
	int err;
	int	qflags, before, qtest;
	struct sam_quot *qp;
	static struct sam_dquot *mqp = NULL;

	if (SAM_IS_SHARED_CLIENT(ip->mp)) {
		struct sam_inode_quota *siq;

		siq = (struct sam_inode_quota *)kmem_zalloc(sizeof (*siq),
		    KM_SLEEP);
		siq->operation = op;
		siq->type = type;
		siq->index = index;
		siq->len = sizeof (siq->quota);
		bcopy((char *)nqdp, (char *)&siq->quota, sizeof (siq->quota));
		err = sam_proc_inode(ip, INODE_putquota, siq, CRED());
		kmem_free(siq, sizeof (*siq));
		return (err);
	}

	/*
	 * On the server.  Get quota record directly, replace the
	 * appropriate entries, and flush the record out to disk.
	 */
	qp = sam_quot_get(ip->mp, NULL, type, index);
	if (qp == NULL) {
		return (ENOENT);
	}

	before = testquota(&qp->qt_dent, ip->mp->mt.fi_config & MT_SAM_ENABLED);

	/*
	 * If the operation is SAM_QOP_PUTALL, replace all fields in the
	 * quota record.  If SAM_QOP_PUT, then mask out the in-use fields
	 * and leave them alone.
	 */
	if (op == SAM_QOP_PUTALL) {
		bcopy((char *)nqdp, (char *)&qp->qt_dent, sizeof (*nqdp));
	} else {
		if (mqp == NULL) {
			sam_usemask.dq_folused = ~0LL;
			sam_usemask.dq_bolused = ~0LL;
			sam_usemask.dq_ftotused = ~0LL;
			sam_usemask.dq_btotused = ~0LL;
			sam_usemask.dq_ol_enforce = ~0;
			sam_usemask.dq_tot_enforce = ~0;
			sam_usemask.unused2 = ~0LL;
			sam_usemask.unused3 = ~0LL;
			mqp = &sam_usemask;
		}
		/*
		 * Clear all the fields in qp->qt_dent that we want to
		 * overwrite; leave those masked by sam_usemask alone.
		 * Then clear out the fields in nqdp that aren't masked
		 * in sam_usemask.  Finally, OR the structs together for
		 * the final result.
		 */
		b_log_AND((char *)&qp->qt_dent, (char *)mqp, sizeof (*mqp));
		b_log_CAND((char *)nqdp, (char *)mqp, sizeof (*mqp));
		b_log_OR((char *)&qp->qt_dent, (char *)nqdp, sizeof (*nqdp));
	}
	qp->qt_flags |= SAM_QF_DIRTY;
	qp->qt_flags &= ~(SAM_QF_OVEROB | SAM_QF_OVERTB
	    | SAM_QF_OVEROF | SAM_QF_OVERTF);

	/*
	 *	May have changed limits.  Make sure the enforce timers
	 *	are still running or not as appropriate.
	 */
	qflags = testquota(&qp->qt_dent, ip->mp->mt.fi_config & MT_SAM_ENABLED);

	qtest = SAM_QT_OL_BLOCK_SOFT | SAM_QT_OL_FILE_SOFT;
	if ((qflags & qtest) != 0) {
		if ((qp->qt_dent.dq_ol_enforce == 0) &&
		    (((before ^ qflags) & qtest) != 0)) {
			qp->qt_dent.dq_ol_enforce = NOW() +
			    qp->qt_dent.dq_ol_grace;
		}
	} else {
		qp->qt_dent.dq_ol_enforce = 0;
	}

	qtest = SAM_QT_TOT_BLOCK_SOFT | SAM_QT_TOT_FILE_SOFT;
	if ((qflags & qtest) != 0) {
		if ((qp->qt_dent.dq_tot_enforce == 0) &&
		    (((before ^ qflags) & qtest) != 0)) {
			qp->qt_dent.dq_tot_enforce = NOW() +
			    qp->qt_dent.dq_tot_grace;
		}
	} else {
		qp->qt_dent.dq_tot_enforce = 0;
	}

	err = sam_quota_flush(qp);
	sam_quot_rel(ip->mp, qp);
	return (err);
}


#ifdef MEM_DEBUG
void *
sam_quota_local_kma(size_t len, int flag)
{
	void *p;
	p = kmem_zalloc(len, flag);
	uprintf("quota:  kmem_zalloc(len = %d, flag = %x) = %llx\n",
	    (int)len, flag, (long long)p);
	return (p);
}

void
sam_quota_local_kmf(void *buf, size_t len)
{
	uprintf("quota:  kmem_free(buf = %llx, len = %d\n",
	    (long long)buf, (int)len);
	kmem_free(buf, len);
}
#endif

/*
 * Update block quota structure.  Locked on entry, locked on exit.
 *
 * n > 0 ==> allocation
 * n < 0 ==> release
 *
 */
static int
sam_quota_bcheck(struct sam_quot *qp, long long ol, long long tot,
    cred_t *credp)
{
	int check, checktotal, before, after, changes, bit;

	ASSERT(MUTEX_HELD(&qp->qt_lock));
	ASSERT(qp != NULL && qp != NODQUOT);

	/*
	 * No complicated cases.
	 */
	if (ol < 0 && tot > 0) {
		cmn_err(CE_WARN,
		    "SAM_QFS: sam_quota_bcheck:  ol < 0 && tot > 0\n");
		return (EDQUOT);
	}
	if (tot < 0 && ol > 0) {
		cmn_err(CE_WARN,
		    "SAM-QFS: sam_quota_bcheck:  ol > 0 && tot < 0\n");
		return (EDQUOT);
	}

	check = (tot+ol > 0) && (qp->qt_index != 0) && credp &&
	    (crgetuid(credp) != 0);

	checktotal = (SAM_VTOI(qp->qt_vp)->mp->mt.fi_config & MT_SAM_ENABLED);

	before = testquota(&qp->qt_dent, checktotal);
	if (check) {
		if (before &
		    (SAM_QT_ERROR|SAM_QT_OL_BLOCK_HARD|SAM_QT_TOT_BLOCK_HARD)) {
			goto errout;
		}
		if (ol > 0 && (before&SAM_QT_OL_BLOCK_SOFT) &&
		    qp->qt_dent.dq_ol_enforce != 0 &&
		    qp->qt_dent.dq_ol_enforce < NOW()) {
			goto errout;
		}
		if (tot > 0 && (before&SAM_QT_TOT_BLOCK_SOFT) &&
		    qp->qt_dent.dq_tot_enforce != 0 &&
		    qp->qt_dent.dq_tot_enforce < NOW()) {
			goto errout;
		}
		if ((before&(SAM_QT_OL_BLOCK_SOFT|SAM_QT_OL_FILE_SOFT)) == 0) {
			qp->qt_dent.dq_ol_enforce = 0;
		}
		if ((before &
		    (SAM_QT_TOT_BLOCK_SOFT|SAM_QT_TOT_FILE_SOFT)) == 0) {
			qp->qt_dent.dq_tot_enforce = 0;
		}
	}

	qp->qt_dent.dq_bolused += ol;
	qp->qt_dent.dq_btotused += tot;
	after = testquota(&qp->qt_dent, checktotal);
	if (check) {
		if (after &
		    (SAM_QT_ERROR|SAM_QT_TOT_BLOCK_HARD|SAM_QT_OL_BLOCK_HARD)) {
			qp->qt_dent.dq_bolused -= ol;
			qp->qt_dent.dq_btotused -= tot;
			goto errout;
		}
	}
	qp->qt_flags |= SAM_QF_DIRTY;

	/*
	 * Walk through the changed bits, one at a time.
	 */
	for (changes = before^after; changes; changes &= ~bit) {
		bit = changes^(changes&(changes-1));		/* get LSB */
		switch (bit) {
		case SAM_QT_OL_BLOCK_SOFT:
			if (after&SAM_QT_OL_BLOCK_SOFT) {
				if (qp->qt_index &&
				    (qp->qt_flags & SAM_QF_OVEROB) == 0) {
					uwarn("SAM-QFS:  Over %s/%d online "
					    "block soft limit\n",
					    quota_types[qp->qt_type],
					    qp->qt_index);
				}
				qp->qt_flags |= SAM_QF_OVEROB;
			} else {
				qp->qt_flags &= ~SAM_QF_OVEROB;
				if ((after &
				    (SAM_QT_OL_BLOCK_SOFT|
				    SAM_QT_OL_FILE_SOFT)) == 0) {
					qp->qt_dent.dq_ol_enforce = 0;
				}
			}
			break;

		case SAM_QT_TOT_BLOCK_SOFT:
			if (after&SAM_QT_TOT_BLOCK_SOFT) {
				if (qp->qt_index &&
				    (qp->qt_flags & SAM_QF_OVERTB) == 0) {
					uwarn("SAM-QFS:  Over %s/%d total "
					    "block soft limit\n",
					    quota_types[qp->qt_type],
					    qp->qt_index);
				}
				qp->qt_flags |= SAM_QF_OVERTB;
			} else {
				if ((after &
				    (SAM_QT_TOT_BLOCK_SOFT|
				    SAM_QT_TOT_FILE_SOFT)) == 0) {
					qp->qt_dent.dq_tot_enforce = 0;
				}
				qp->qt_flags &= ~SAM_QF_OVERTB;
			}
			break;

		/*
		 * Not necessarily wrong to flip the HARD quota bits; root
		 * can set quotas down or super-user can chown/chgrp files.
		 * When user gets below hard quota, these will kick off.
		 */
		case SAM_QT_OL_BLOCK_HARD:
			if (after&SAM_QT_OL_BLOCK_HARD) {
				if (qp->qt_index) {
					uwarn("SAM-QFS:  Over %s/%d online "
					    "block hard limit\n",
					    quota_types[qp->qt_type],
					    qp->qt_index);
				}
				qp->qt_flags |= SAM_QF_OVEROB;
			} else {
				if (qp->qt_index) {
					uwarn("SAM-QFS:  Now under %s/%d "
					    "online block hard limit\n",
					    quota_types[qp->qt_type],
					    qp->qt_index);
				}
			}
			break;

		case SAM_QT_TOT_BLOCK_HARD:
			if (after&SAM_QT_TOT_BLOCK_HARD) {
				if (qp->qt_index) {
					uwarn("SAM-QFS:  Over %s/%d total "
					    "block hard limit\n",
					    quota_types[qp->qt_type],
					    qp->qt_index);
				}
				qp->qt_flags |= SAM_QF_OVERTB;
			} else {
				if (qp->qt_index) {
					uwarn("SAM-QFS:  Now under %s/%d "
					    "total block hard limit\n",
					    quota_types[qp->qt_type],
					    qp->qt_index);
				}
			}
			break;

		default:
			cmn_err(CE_WARN,
			    "SAM-QFS:  Unexpected %s/%d block quota status"
			    " change:  was %x, is %x, ck = %d",
			    quota_types[qp->qt_type], qp->qt_index,
			    before, after, check);
			uwarn("SAM-QFS:  Unexpected %s/%d block quota "
			    "status change:  "
			    "was %x, is %x, ck = %d\n",
			    quota_types[qp->qt_type], qp->qt_index,
			    before, after, check);
		}
	}
	/*
	 * Since we could have been over quota and cleared a grace counter,
	 * we need to restart it now.
	 */
	if (after&SAM_QT_OL_BLOCK_SOFT && qp->qt_dent.dq_ol_enforce == 0) {
		if (qp->qt_index && (qp->qt_flags&SAM_QF_OVEROB) == 0) {
			uwarn("SAM-QFS:  Over %s/%d online block soft limit\n",
			    quota_types[qp->qt_type], qp->qt_index);
		}
		qp->qt_dent.dq_ol_enforce = NOW() + qp->qt_dent.dq_ol_grace;
		qp->qt_flags |= SAM_QF_OVEROB;
	}
	if (after&SAM_QT_TOT_BLOCK_SOFT && qp->qt_dent.dq_tot_enforce == 0) {
		if (qp->qt_index && (qp->qt_flags&SAM_QF_OVERTB) == 0) {
			uwarn("SAM-QFS:  Over %s/%d total block soft limit\n",
			    quota_types[qp->qt_type], qp->qt_index);
		}
		qp->qt_dent.dq_tot_enforce = NOW() + qp->qt_dent.dq_tot_grace;
		qp->qt_flags |= SAM_QF_OVERTB;
	}

	qp->qt_lastref = NOW();
	return (0);

errout:
	qp->qt_lastref = NOW();
	return (EDQUOT);
}


/*
 * Return OK (and increment all quota counts) if the
 * allocation (block) is OK.
 *
 * Return EDQUOT if not, and back down all the counts.
 */
int
sam_quota_balloc(sam_mount_t *mp, sam_node_t *ip, long long nblks,
    long long nblks2, cred_t *cr)
{
	int i, j, err = 0;
	struct sam_quot *qp;

	ASSERT(!SAM_IS_SHARED_CLIENT(mp));

	if (S_ISSEGI(&ip->di)) {
		return (0);	/* Don't count blocks for segment index */
	}

	if ((mp->mt.fi_config & MT_QUOTA) == 0) {
		return (0);
	}

	if (SAM_PRIVILEGE_INO(ip->di.version, ip->di.id.ino)) {
		return (0);
	}

	TRACE(T_SAM_QUOTA_ALLOCB, SAM_ITOV(ip), nblks, nblks2, 0);

	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		qp = sam_quot_get(mp, ip, i, sam_quota_get_index(ip, i));
		if (qp != NULL) {
			if (err = sam_quota_bcheck(qp, nblks, nblks2, cr)) {
				sam_quot_rel(mp, qp);
				break;
			}
			sam_quot_rel(mp, qp);
		}
	}
	if (!err) {
		return (0);
	}

	for (j = i-1; j >= 0; j--) {
		qp = sam_quot_get(mp, ip, j, sam_quota_get_index(ip, j));
		if (qp != NULL) {
			if (err = sam_quota_bcheck(qp, -nblks, -nblks2, NULL)) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Error returning "
				    "blocks in %s quota"
				    " %d/%lld/%lld",
				    mp->mt.fi_name, quota_types[j],
				    qp->qt_index,
				    nblks, nblks2);
			}
			sam_quot_rel(mp, qp);
		}
	}
	return (EDQUOT);
}


/*
 * Return OK (and increment all quota counts) if the
 * allocation (block) is OK.
 * A flavor of sam_quota_balloc() that takes a perm inode pointer.
 *
 * Return EDQUOT if not, and back down all the counts.
 */
int
sam_quota_balloc_perm(sam_mount_t *mp, struct sam_perm_inode *permip,
    long long nblks, long long nblks2, cred_t *cr)
{
	int i, j, err = 0;
	struct sam_quot *qp;

	ASSERT(!SAM_IS_SHARED_CLIENT(mp));

	if (S_ISSEGI(&permip->di)) {
		return (0);	/* Don't count blocks for segment index */
	}

	if ((mp->mt.fi_config & MT_QUOTA) == 0) {
		return (0);
	}

	if (SAM_PRIVILEGE_INO(permip->di.version, permip->di.id.ino)) {
		return (0);
	}

	TRACE(T_SAM_QUOTA_ALLOCB, NULL, nblks, nblks2, 0);

	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		qp = sam_quot_get(mp, NULL, i,
		    sam_quota_get_index_di(&permip->di, i));
		if (qp != NULL) {
			if (err = sam_quota_bcheck(qp, nblks, nblks2, cr)) {
				sam_quot_rel(mp, qp);
				break;
			}
			sam_quot_rel(mp, qp);
		}
	}
	if (!err) {
		return (0);
	}

	for (j = i-1; j >= 0; j--) {
		qp = sam_quot_get(mp, NULL, j,
		    sam_quota_get_index_di(&permip->di, j));
		if (qp != NULL) {
			if (err = sam_quota_bcheck(qp, -nblks, -nblks2, NULL)) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Error returning "
				    "blocks in %s quota"
				    " %d/%lld/%lld",
				    mp->mt.fi_name, quota_types[j],
				    qp->qt_index,
				    nblks, nblks2);
			}
			sam_quot_rel(mp, qp);
		}
	}
	return (EDQUOT);
}


/*
 * Return asked-for blocks, no questions asked.  Typically
 * called when an archive copy is staled out or a file is
 * truncated or deleted.
 */
int
sam_quota_bret(sam_mount_t *mp, sam_node_t *ip, long long nblks,
    long long nblks2)
{
	int i, err = 0;
	struct sam_quot *qp;

	ASSERT(!SAM_IS_SHARED_CLIENT(mp));

	if ((mp->mt.fi_config & MT_QUOTA) == 0) {
		return (0);
	}

	if (SAM_PRIVILEGE_INO(ip->di.version, ip->di.id.ino)) {
		return (0);
	}

	TRACE(T_SAM_QUOTA_RETBLK, SAM_ITOV(ip), __LINE__, nblks, nblks2);

	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		qp = sam_quot_get(mp, ip, i, sam_quota_get_index(ip, i));
		if (qp != NULL) {
			if (sam_quota_bcheck(qp, -nblks, -nblks2, NULL)) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_quota_bret:  "
				    "error returning blocks"
				    " in '%s' quota %i/%lld/%lld.",
				    mp->mt.fi_name, quota_types[i], i,
				    nblks, nblks2);
				err = 1;
			}
			sam_quot_rel(mp, qp);
		}
	}
	return (err ? EDQUOT : 0);
}

/*
 * Check for sam file quota (inode count).  Inode pointer is pointer to
 * directory that file will go in.  (Needed to get archive set ID or
 * other inherited property).
 */
int
sam_quota_falloc(sam_mount_t *mp, uid_t uid, gid_t gid, int aid, cred_t *cr)
{
	int i, j, err = 0;
	int idx[SAM_QUOTA_DEFD];

	ASSERT(!SAM_IS_SHARED_CLIENT(mp));

	if ((mp->mt.fi_config & MT_QUOTA) == 0) {
		return (0);
	}

	idx[SAM_QUOTA_ADMIN] = aid;
	idx[SAM_QUOTA_GROUP] = gid;
	idx[SAM_QUOTA_USER] = uid;

	TRACE(T_SAM_QUOTA_FILE, NULL, aid, gid, uid);

	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		if (mp->mi.m_quota_ip[i] && idx[i] >= 0) {
			err = sam_quota_fcheck(mp, i, idx[i], 1, 1, cr);
			if (err) {
				break;
			}
		}
	}
	if (!err) {
		return (0);
	}

	for (j = i-1; j >= 0; j--) {
		if (mp->mi.m_quota_ip[j] && idx[j] >= 0) {
			err = sam_quota_fcheck(mp, j, idx[j], -1, -1, NULL);
			if (err) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Error returning "
				    "file to '%s' quota %d",
				    mp->mt.fi_name, quota_types[j], idx[j]);
			}
		}
	}
	return (EDQUOT);
}

/*
 * Reassign the file and its blocks to the specified uid/gid.
 */
int
sam_quota_chown(sam_mount_t *mp, sam_node_t *ip, uid_t uid, gid_t gid,
    cred_t *cr)
{
	struct sam_quot *uquot = NULL, *gquot = NULL;
	struct sam_dquot *uq, *gq;
	offset_t ndkblks, ntpblks;
	int r = SAM_QT_OK;

	if ((mp->mt.fi_config & MT_QUOTA) == 0) {	/* no quotas? */
		return (0);
	}
	if (ip->di.uid == uid && ip->di.gid == gid)	{	/* no-op? */
		return (0);
	}
	if (!mp->mi.m_quota_ip[SAM_QUOTA_USER] &&
	    !mp->mi.m_quota_ip[SAM_QUOTA_GROUP]) {
		return (0);			/* no user or group quotas */
	}

	/*
	 * Can't allow chown/chgrp of an active quota file
	 * (deadlock would occur if any entry (to or from)
	 * isn't already in memory).
	 */
	if (ip->di.uid != uid && ip == mp->mi.m_quota_ip[SAM_QUOTA_USER]) {
		return (EBUSY);
	}

	if (ip->di.gid != gid && ip == mp->mi.m_quota_ip[SAM_QUOTA_GROUP]) {
		return (EBUSY);
	}

	sam_quota_inode_fini(ip);

	ndkblks = D2QBLKS(ip->di.blocks);
	ntpblks = TOTBLKS(ip);
	if (ip->di.uid != uid) {
		uquot = sam_quot_get(mp, NULL, SAM_QUOTA_USER, uid);
	}
	if (ip->di.gid != gid) {
		gquot = sam_quot_get(mp, NULL, SAM_QUOTA_GROUP, gid);
	}
	if (uquot) {
		uq = &uquot->qt_dent;
		uq->dq_folused++;
		uq->dq_ftotused++;
		uq->dq_bolused += ndkblks;
		uq->dq_btotused += ntpblks;
		r = (crgetuid(cr) == 0) ?
		    0 : testquota(uq, (mp->mt.fi_config & MT_SAM_ENABLED));
		if (r & (SAM_QT_OL_FILE_HARD|SAM_QT_TOT_FILE_HARD
		    |SAM_QT_OL_BLOCK_HARD|SAM_QT_TOT_BLOCK_HARD)) {
			uq->dq_folused--;
			uq->dq_ftotused--;
			uq->dq_bolused -= ndkblks;
			uq->dq_btotused -= ntpblks;
			if (gquot) {
				sam_quot_rel(mp, gquot);
			}
			sam_quot_rel(mp, uquot);
			return (EDQUOT);
		}
		uquot->qt_flags |= SAM_QF_DIRTY;
	}
	if (gquot) {
		gq = &gquot->qt_dent;
		gq->dq_folused++;
		gq->dq_ftotused++;
		gq->dq_bolused += ndkblks;
		gq->dq_btotused += ntpblks;
		r = (crgetuid(cr) == 0) ?
		    0 : testquota(gq, (mp->mt.fi_config & MT_SAM_ENABLED));
		if (r & (SAM_QT_OL_FILE_HARD|SAM_QT_TOT_FILE_HARD|
		    SAM_QT_OL_BLOCK_HARD|SAM_QT_TOT_BLOCK_HARD)) {
			gq->dq_folused--;
			gq->dq_ftotused--;
			gq->dq_bolused -= ndkblks;
			gq->dq_btotused -= ntpblks;
			sam_quot_rel(mp, gquot);

			if (uquot) {
				uq->dq_folused--;
				uq->dq_ftotused--;
				uq->dq_bolused -= ndkblks;
				uq->dq_btotused -= ntpblks;
				uquot->qt_flags |= SAM_QF_DIRTY;
				sam_quot_rel(mp, uquot);
			}
			return (EDQUOT);
		}
		gquot->qt_flags |= SAM_QF_DIRTY;
	}
	if (gquot) {
		sam_quot_rel(mp, gquot);
	}
	if (uquot) {
		sam_quot_rel(mp, uquot);
	}

	/*
	 * Cool.  We've set the new counts into the target, now we need
	 * to back out the old counts.
	 */
	if (ip->di.uid != uid) {
		uquot = sam_quot_get(mp, NULL, SAM_QUOTA_USER, ip->di.uid);
		if (uquot != NULL) {
			uq = &uquot->qt_dent;
			uq->dq_folused--;
			uq->dq_ftotused--;
			uq->dq_bolused -= ndkblks;
			uq->dq_btotused -= ntpblks;
			uquot->qt_flags |= SAM_QF_DIRTY;
			sam_quot_rel(mp, uquot);
		}
	}
	if (ip->di.gid != gid) {
		gquot = sam_quot_get(mp, NULL, SAM_QUOTA_GROUP, ip->di.gid);
		if (gquot != NULL) {
			gq = &gquot->qt_dent;
			gq->dq_folused--;
			gq->dq_ftotused--;
			gq->dq_bolused -= ndkblks;
			gq->dq_btotused -= ntpblks;
			gquot->qt_flags |= SAM_QF_DIRTY;
			sam_quot_rel(mp, gquot);
		}
	}
	return (0);
}

int
sam_quota_fret(sam_mount_t *mp, sam_node_t *ip)
{
	int i, err = 0;

	ASSERT(!SAM_IS_SHARED_CLIENT(mp));

	if ((mp->mt.fi_config & MT_QUOTA) == 0) {
		return (0);
	}

	TRACE(T_SAM_QUOTA_RETFILE, SAM_ITOV(ip), 0, 0, 0);

	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		if (ip->bquota[i] != NODQUOT && mp->mi.m_quota_ip[i]) {
			err = sam_quota_fcheck(mp, i,
			    sam_quota_get_index(ip, i),
			    ip->di.status.b.offline ? 0 : -1, -1, NULL);
			if (err) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_quota_fret:  "
				    "Error returning file"
				    " to '%s' quota %i",
				    mp->mt.fi_name, quota_types[i],
				    sam_quota_get_index(ip, i));
			}
		}
	}
	return (0);
}


/*
 * SAM interface functions.
 */

/*
 * Set archive set value in inode.  General function, but put here
 * because they're being added as part of quotas.
 */
int
sam_set_admid(void *arg, int size, cred_t *credp)
{
	int follow, error = 0;
	struct sam_chaid_arg args;
	char *path;
	vnode_t *vp, *rvp;
	sam_node_t *ip;
	sam_mount_t *mp;
	sam_inode_samaid_t sa;

	/*
	 * Validate and copy in the arguments.
	 */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	path = (void *)args.path.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		path = (void *)args.path.p64;
	}

	follow = args.follow ? FOLLOW : NO_FOLLOW;
	if ((error = lookupname((char *)path, UIO_USERSPACE, follow,
	    NULLVPP, &vp))) {
		return (error);
	}

	if (sam_set_realvp(vp, &rvp)) {
		error = ENOTTY;
		goto out;
	}

	ip = SAM_VTOI(rvp);

	if (secpolicy_fs_quota(credp, ip->mp->mi.m_vfsp)) {
		error = EPERM;
		goto out;
	}

	if (ip->di.admin_id == args.admin_id) {		/* no-op */
		goto out;
	}

	if (ip == ip->mp->mi.m_quota_ip[SAM_QUOTA_ADMIN]) { /* avoid deadlock */
		error = EBUSY;
		goto out;
	}

	if ((error = sam_open_operation(ip)) != 0) {
		goto out;
	}
	mp = ip->mp;

	bzero((char *)&sa, sizeof (sa));
	sa.operation = SAM_INODE_AID;
	sa.aid = args.admin_id;

	if (SAM_IS_SHARED_CLIENT(mp)) {
		error = sam_proc_inode(ip, INODE_samaid, &sa, credp);
	} else {
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		error = sam_set_aid(ip, &sa);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}
	SAM_CLOSE_OPERATION(mp, error);

out:
	VN_RELE(vp);
	return (error);
}


/*
 * Change the inode's admin ID.  We've got permissions,
 * we've got the inode, we're good to go.
 */
int
sam_set_aid(sam_node_t *ip, struct sam_inode_samaid *sa)
{
	offset_t ndkblks, ntpblks;
	struct sam_quot *aquot = NULL;
	struct sam_dquot *aq;
	uint32_t id = sa->aid;

	/*
	 * Adjust quota values if quotas are in force
	 */
	if ((sa->operation == SAM_INODE_AID) &&
	    (ip->mp->mt.fi_config & MT_QUOTA) &&
	    ip->mp->mi.m_quota_ip[SAM_QUOTA_ADMIN]) {
		sam_quota_inode_fini(ip);

		ndkblks = D2QBLKS(ip->di.blocks);
		ntpblks = TOTBLKS(ip);
		aquot = sam_quot_get(ip->mp, NULL, SAM_QUOTA_ADMIN, id);
		if (aquot != NULL) {
			aq = &aquot->qt_dent;
			aq->dq_folused++;
			aq->dq_ftotused++;
			aq->dq_bolused += ndkblks;
			aq->dq_btotused += ntpblks;
			aquot->qt_flags |= SAM_QF_DIRTY;
			sam_quot_rel(ip->mp, aquot);
		}
		/*
		 * Decrement blocks and file count from source archive set.
		 * Don't attach old quota entry to inode.
		 */
		aquot = sam_quot_get(ip->mp, NULL, SAM_QUOTA_ADMIN,
		    ip->di.admin_id);
		if (aquot != NULL) {
			aq = &aquot->qt_dent;
			aq->dq_folused--;
			aq->dq_ftotused--;
			aq->dq_bolused -= ndkblks;
			aq->dq_btotused -= ntpblks;
			aquot->qt_flags |= SAM_QF_DIRTY;
			sam_quot_rel(ip->mp, aquot);
		}
	}

	if (sa->operation == SAM_INODE_AID) {
		/*
		 * Set admin_id.
		 */
		ip->di.admin_id = id;
	} else if (sa->operation == SAM_INODE_PROJID) {
		/*
		 * Set project ID.
		 */
		ip->di2.projid = id;
	} else {
		return (EINVAL);
	}
	TRANS_INODE(ip->mp, ip);
	sam_mark_ino(ip, SAM_CHANGED);

	return (0);
}


void
sam_quota_init(sam_mount_t *mp)
{
	int i, j, error, nqf;

	TRACE(T_SAM_QUOTA_INI, mp, __LINE__, 0, 1);
	if ((mp->mt.fi_config & MT_QUOTA) == 0) {
		return;				/* mounted noquota */
	}

	if (mp->mi.m_quota_hash) {
		return;				/* already initialized */
	}

	nqf = 0;
	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		sam_node_t *dip = SAM_VTOI(mp->mi.m_vn_root);
		sam_node_t *ip;

		mp->mi.m_quota_ip[i] = NULL;
		if (SAM_IS_SHARED_CLIENT(mp)) {
			error = sam_client_lookup_name(dip, quota_files[i],
			    0, 0, &ip, CRED());
		} else {
			error = sam_lookup_name(dip, quota_files[i], &ip, NULL,
			    CRED());
		}
		if (error) {
			if (error != ENOENT) {
				cmn_err(CE_WARN,
				    "SAM-QFS %s: Quota file lookup failure"
				    " (file '%s', error %d)",
				    mp->mt.fi_name, quota_files[i], error);
			}
		} else {
			int err = 0;

			if (!S_ISREG(ip->di.mode)) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Quota file %s: "
				    "quota disabled (non-file)",
				    mp->mt.fi_name, quota_files[i]);
				err = 1;
			}
			if (ip->di.status.b.segment) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Quota file %s: "
				    "quota disabled (segment)",
				    mp->mt.fi_name, quota_files[i]);
				err = 1;
			}
			if (ip->di.status.b.offline) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Quota file %s: "
				    "quota disabled (offline)",
				    mp->mt.fi_name, quota_files[i]);
				err = 1;
			}
			if (ip->di.status.b.nodrop == 0) {
				cmn_err(CE_NOTE,
				    "SAM-QFS: %s: Quota file %s: "
				    "marking no-release",
				    mp->mt.fi_name, quota_files[i]);
				ip->di.status.b.nodrop = 1;
				TRANS_INODE(ip->mp, ip);
				sam_mark_ino(ip, SAM_CHANGED);
			}
			if (!err) {
				TRACE(T_SAM_QUOTA_INI, mp, __LINE__,
				    (sam_tr_t)ip, 2);
				nqf++;
				/*
				 * Avoid problems with recursive quota
				 * adjustment.
				 */
				for (j = 0; j < SAM_QUOTA_DEFD; j++)
					ip->bquota[j] = NODQUOT;
				mp->mi.m_quota_ip[i] = ip;
			} else {
				VN_RELE(SAM_ITOV(ip));
			}
		}
	}
	if (nqf != 0) {
		sam_mutex_init(&mp->mi.m_quota_hashlock, NULL,
		    MUTEX_DEFAULT, NULL);
		sam_mutex_init(&mp->mi.m_quota_availlock, NULL,
		    MUTEX_DEFAULT, NULL);
		TRACE(T_SAM_QUOTA_INI, mp, __LINE__, 0, 3);
		mp->mi.m_quota_hash = (struct sam_quot **)
		    KMA(SAM_QUOTA_HASH_SIZE*sizeof (struct sam_quot *),
		    KM_SLEEP);
		mp->mi.m_quota_avail = (struct sam_quot *)
		    KMA(sizeof (struct sam_quot), KM_SLEEP);
		mp->mi.m_quota_avail->qt_flags = SAM_QF_ANCHOR;
		mp->mi.m_quota_avail->qt_avail.next = mp->mi.m_quota_avail;
		mp->mi.m_quota_avail->qt_avail.prev = mp->mi.m_quota_avail;
		TRACE(T_SAM_QUOTA_INI, mp, __LINE__, 0, 4);
	} else {
		/*
		 * No quota files.  Turn the flag off.
		 */
		mp->mt.fi_config &= ~MT_QUOTA;
	}
	TRACE(T_SAM_QUOTA_INI, mp, __LINE__, 0, 5);
}


/*
 *	-----	sam_quota_common_done
 *
 * Finish up quota operations on this mp.  Called as part of
 * umount or failover.  Everything should be flushed at this
 * point; walk through the data structures and free up all
 * the quota records.  If 'close' is set, then finish up:
 * free up the hash tables, destroy the mutexes, and close
 * the quota files.
 */
static void
sam_quota_common_done(sam_mount_t *mp, int close)
{
	int i, j = 0;
	struct sam_quot *qp;

	TRACE(T_SAM_QUOTA_FIN, mp, __LINE__, close, 1);
	if (!mp->mi.m_quota_hash) {
		return;
	}

	/*
	 * Remove everything in the hash table.  This may miss some
	 * structures that haven't been assigned to anything yet.
	 */
	mutex_enter(&mp->mi.m_quota_hashlock);
	for (i = 0; i < SAM_QUOTA_HASH_SIZE; i++) {
		while ((qp = mp->mi.m_quota_hash[i]) != NULL) {
			TRACE(T_SAM_QUOTA_FIN, mp, __LINE__, (sam_tr_t)qp, 2);
			if (!mutex_tryenter(&qp->qt_lock)) {
				mutex_exit(&mp->mi.m_quota_hashlock);
				mutex_enter(&qp->qt_lock);
				mutex_enter(&mp->mi.m_quota_hashlock);
			}
			sam_quot_unhash(mp, qp);
			if (qp->qt_flags&SAM_QF_DIRTY) {
				cmn_err(CE_WARN,
				    "SAM-QFS %s: sam_quota_done(1): dirty "
				    "quota block (%d, %d)",
				    mp->mt.fi_name, qp->qt_type, qp->qt_index);
			}
			if (qp->qt_flags&SAM_QF_AVAIL) {
				mutex_enter(&mp->mi.m_quota_availlock);
				sam_quot_unfree(mp, qp);
				mutex_exit(&mp->mi.m_quota_availlock);
			}
			mutex_destroy(&qp->qt_lock);
			TRACE(T_SAM_QUOTA_FREE, mp,
			    (sam_tr_t)qp, qp->qt_type, qp->qt_index);
			KMF(qp, sizeof (struct sam_quot));
			j++;
		}
	}
	mutex_exit(&mp->mi.m_quota_hashlock);

	/*
	 * Remove everything on the available list.
	 * This should only include structures that
	 * haven't ever been assigned.
	 */
	mutex_enter(&mp->mi.m_quota_availlock);
	while (mp->mi.m_quota_avail != mp->mi.m_quota_avail->qt_avail.next) {
		qp = mp->mi.m_quota_avail->qt_avail.next;
		if (!mutex_tryenter(&qp->qt_lock)) {
			mutex_exit(&mp->mi.m_quota_availlock);
			mutex_enter(&qp->qt_lock);
			mutex_enter(&mp->mi.m_quota_availlock);
		}
		if (qp->qt_flags&SAM_QF_DIRTY) {
			cmn_err(CE_WARN,
			    "SAM-QFS %s: sam_quota_done(2): dirty quota "
			    "block (%d, %d)",
			    mp->mt.fi_name, qp->qt_type, qp->qt_index);
		}
		sam_quot_unfree(mp, qp);
		mutex_destroy(&qp->qt_lock);
		TRACE(T_SAM_QUOTA_FREE, mp, (sam_tr_t)qp, qp->qt_type,
		    qp->qt_index);
		KMF(qp, sizeof (struct sam_quot));
		j++;
	}
	if (!close) {
		mutex_exit(&mp->mi.m_quota_availlock);
		/*
		 * Flush all quota file pages.
		 */
		for (i = 0; i < SAM_QUOTA_DEFD; i++) {
			sam_node_t *ip;

			if ((ip = mp->mi.m_quota_ip[i]) != NULL) {
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				sam_flush_pages(ip, B_INVAL);
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			}
		}
		return;
	}

	/*
	 * The available list is a doubly-linked circular list with an
	 * "anchor" entry.  "Let go of your anchor, Luke."
	 */
	qp = mp->mi.m_quota_avail;
	ASSERT(qp != NULL && (qp->qt_flags&SAM_QF_ANCHOR));
	mutex_exit(&mp->mi.m_quota_availlock);
	mutex_enter(&qp->qt_lock);
	mutex_enter(&mp->mi.m_quota_availlock);
	mutex_destroy(&qp->qt_lock);
	TRACE(T_SAM_QUOTA_FREE, mp, (sam_tr_t)qp, qp->qt_type, qp->qt_index);
	KMF(qp, sizeof (struct sam_quot));
	mp->mi.m_quota_avail = 0;
	mutex_exit(&mp->mi.m_quota_availlock);
	j++;

	/*
	 * Close remaining quota files, and remove inodes/vnodes.
	 */
	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		vnode_t *vp;
		sam_node_t *ip;

		if ((ip = mp->mi.m_quota_ip[i]) != NULL) {
			vp = SAM_ITOV(ip);
			VN_RELE(vp);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			mp->mi.m_quota_ip[i] = NULL;
			sam_flush_pages(ip, B_INVAL);
			SAM_DESTROY_OBJ_LAYOUT(ip);
			if (vp->v_count || sam_delete_ino(vp)) {
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_quota_done:  i=%d,"
				    " v_count=%d || sam_delete_ino()",
				    mp->mt.fi_name, i, vp->v_count);
			} else {
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				sam_destroy_ino(ip, FALSE);
			}
		}
	}
	j++;
	/*
	 * Remove the base data structures
	 */
	mutex_enter(&mp->mi.m_quota_hashlock);
	KMF(mp->mi.m_quota_hash, SAM_QUOTA_HASH_SIZE *
	    sizeof (struct sam_quot *));
	mp->mi.m_quota_hash = 0;
	mutex_exit(&mp->mi.m_quota_hashlock);
	TRACE(T_SAM_QUOTA_FIN, mp, __LINE__, close, 3);
	mutex_destroy(&mp->mi.m_quota_availlock);
	mutex_destroy(&mp->mi.m_quota_hashlock);
	TRACE(T_SAM_QUOTA_FIN, mp, __LINE__, close, 4);
}


/*
 * Sweep through the inodes list, reclaim all the sam_quot structures.
 * Then sweep through the sam_quot structures in the hash table, and
 * ensure that all have zero ref counts.
 */
int
sam_quota_halt(sam_mount_t *mp)
{
	sam_ihead_t *hip;
	sam_node_t *ip;
	struct sam_quot *qp, *nqp;
	int ihash;
	kmutex_t *ihp;
	int i, j, err = 0;

	TRACE(T_SAM_QUOTA_HLT, mp, __LINE__, 0, 1);
	if (!mp->mi.m_quota_hash) {
		return (0);
	}

	/*
	 * Sweep through all the inodes; clean out all the quota pointers.
	 * If anything is trying to write, this is OK; the pointers are
	 * cleared to NULL, and will be re-obtained if any writing
	 * is done.
	 */
	hip = (sam_ihead_t *)&samgt.ihashhead[0];
	ASSERT(hip);
	for (ihash = 0; ihash < samgt.nhino; ihash++, hip++) {
		ihp = &samgt.ihashlock[ihash];
		mutex_enter(ihp);
		for (ip = (sam_node_t *)(void *)hip->forw;
		    ip != (sam_node_t *)(void *)hip;
		    ip = ip->chain.hash.forw) {
			if (ip->mp != mp) {
				continue;
			}
			if (rw_tryenter(&ip->inode_rwl, RW_WRITER) == 0) {
				mutex_exit(ihp);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				mutex_enter(ihp);
			}
			sam_quota_inode_fini(ip);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
		mutex_exit(ihp);
	}
	TRACE(T_SAM_QUOTA_HLT, mp, __LINE__, 0, 2);

	/*
	 * Now sweep through the sam_quot hash table.  If any ref counts
	 * are non-zero, return failure.
	 */
	mutex_enter(&mp->mi.m_quota_hashlock);
	for (i = 0; i < SAM_QUOTA_HASH_SIZE; i++) {
		qp = mp->mi.m_quota_hash[i];
		while (qp != NULL) {
			TRACE(T_SAM_QUOTA_HSW, mp, (sam_tr_t)qp,
			    qp->qt_type, qp->qt_index);
			if (!mutex_tryenter(&qp->qt_lock)) {
				TRACE(T_SAM_QUOTA_HSWE1, mp,
				    (sam_tr_t)qp, qp->qt_type, qp->qt_index);
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_quota_halt: "
				    "quota lock taken,"
				    " quota type %s, index %d",
				    mp->mt.fi_name, quota_types[qp->qt_type],
				    qp->qt_index);
				err++;
				qp = qp->qt_hash.next;
				continue;
			}
			nqp = qp->qt_hash.next;
			if (qp->qt_ref != 0) {
				TRACE(T_SAM_QUOTA_HSWE2, mp,
				    (sam_tr_t)qp, qp->qt_type, qp->qt_index);
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Quota ref count !0:  "
				    "ref=%d, flags=%x,"
				    " type=%d, index=%d\n",
				    mp->mt.fi_name, qp->qt_ref, qp->qt_flags,
				    qp->qt_type,
				    qp->qt_index);
				err++;
			}
			mutex_exit(&qp->qt_lock);
			qp = nqp;
		}
	}
	mutex_exit(&mp->mi.m_quota_hashlock);
	if (err) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: umount: returning error (%d);"
		    " quota_ip[x] not released\n",
		    mp->mt.fi_name, err);
		return (err);
	}
	/*
	 * If any of the quota files have been deleted, let them go first.
	 * But zero out the corresponding entries in the mount table first,
	 * so that we don't try to update the quota file being deleted in
	 * itself.  Deadlock ... baaad.
	 */
	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		if ((ip = mp->mi.m_quota_ip[i]) != NULL) {
			if (ip->di.nlink == 0) {
				mp->mi.m_quota_ip[i] = NULL;
				VN_RELE(SAM_ITOV(ip));
			}
		}
	}

	/*
	 * Flush any remaining quota entries now attached to the
	 * remaining quota files.
	 */
	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		if ((ip = mp->mi.m_quota_ip[i]) != NULL) {
			sam_quota_inode_fini(ip);
		}
	}
	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		if (mp->mi.m_quota_ip[i] == NULL) {
			continue;
		}
		for (j = 0; j < SAM_QUOTA_DEFD; j++) {
			if (mp->mi.m_quota_ip[i]->bquota[j] != NODQUOT) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: sam_quota_halt:"
				    " non-null quota found after flush,"
				    " index %d/%d",
				    mp->mt.fi_name, i, j);
			}
		}
	}

	TRACE(T_SAM_QUOTA_HLT, mp, __LINE__, 0, 3);

	sam_quota_common_done(mp, 0);

	return (0);
}


void
sam_quota_fini(sam_mount_t *mp)
{
	sam_quota_common_done(mp, 1);
}
