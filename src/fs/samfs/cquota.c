/*
 * ----- samfs/cquota.c - Process the sam common quota functions.
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

#pragma ident "$Revision: 1.37 $"

#include "sam/osversion.h"

/*
 * #define	MEM_DEBUG
 */
#define	MAX(x, y)	(((x) > (y)) ? (x) : (y))

#ifdef MEM_DEBUG
#define	KMA(x, y)	sam_quota_local_kma(x, y)
#define	OPTRACE(w, x, y, z)	TRACE(T_SAM_QUOTA_OP, w, x, y, z)
#else
#define	KMA(x, y)	kmem_zalloc(x, y)
#define	OPTRACE(w, x, y, z)
#endif


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
#include <sys/policy.h>

/* ----- SAMFS Includes */

#define	__QUOTA_DEFS
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
#undef	__QUOTA_DEFS


static struct sam_quot *sam_quot_getfree(sam_mount_t *mp);
static struct sam_quot *sam_quot_getfront(sam_mount_t *mp);
static void sam_quot_hold(struct sam_quot *qp);
static void sam_quot_unhold(struct sam_quot *qp);
static int sam_quota_read(struct sam_quot *qp);
static int sam_quota_read_entry(sam_node_t *, struct sam_dquot *, int, int);
static int sam_zerocmp(void *p, int n);
static void sam_quot_free2back(sam_mount_t *mp, struct sam_quot *dqp);
static void sam_quot_free2front(sam_mount_t *mp, struct sam_quot *dqp);
static int sam_quota_get_info(sam_mount_t *mp, int, int, struct sam_dquot *);

#define	NOW()	(SAM_SECOND())
#define	uwarn	uprintf


/*
 * ----- sam_quota_stat -
 *
 * Process the user stat quota operations.
 * Process all user (non-priv) quota operations.
 */

int
sam_quota_stat(struct sam_quota_arg *argp)
{
	struct sam_quota_arg args;
	int i, err = 0, ok;
	file_t *fp = NULL;
	sam_node_t *ip;
	sam_mount_t *mp;
	struct sam_dquot *uqp, q;

	OPTRACE(NULL, __LINE__, (int)argp, 0);
	if (copyin(argp, &args, sizeof (args))) {
		TRACE(T_SAM_QUOTA_OPERR, NULL, __LINE__, (int)argp, EFAULT);
		return (EFAULT);
	}

	OPTRACE(NULL, __LINE__, args.qcmd, args.qflags);
	if (args.qsize != sizeof (struct sam_quota_arg)) {
		TRACE(T_SAM_QUOTA_OPERR, NULL, __LINE__,
		    (int)args.qsize, EFAULT);
		return (EFAULT);
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

	mp = ip->mp;
	/*
	 * Process user quota functions.
	 */
	switch (args.qcmd) {

	case SAM_QOP_GET:
		if (args.qflags&SAM_QFL_INDEX) {
			/*
			 * user supplied index.
			 * Operation  permitted if:
			 *  (1) Superuser
			 *  (2) If USER request, and index == uid
			 *  (3) If GROUP request, and index in group set
			 *  (4) If user has write permissions on file,
			 *	and index matches file's corresponding index.
			 *
			 */
			err = ok = 0;
			switch (args.qtype) {
			case SAM_QUOTA_ADMIN:
				break;
			case SAM_QUOTA_GROUP:
				if (groupmember(args.qindex, CRED()))
					ok = 1;
				break;
			case SAM_QUOTA_USER:
				if (args.qindex == crgetuid(CRED()))
					ok = 1;
				break;
			}
			if (!ok &&
			    (err = sam_access_ino(ip, S_IWRITE, FALSE,
			    CRED())) == 0) {
				switch (args.qtype) {
				case SAM_QUOTA_ADMIN:
					if (args.qindex == ip->di.admin_id)
						ok = 1;
					break;
				case SAM_QUOTA_GROUP:
					if (args.qindex == ip->di.gid)
						ok = 1;
					break;
				case SAM_QUOTA_USER:
					if (args.qindex == ip->di.uid)
						ok = 1;
					break;
				}
			}
			if (!ok &&
			    (secpolicy_fs_quota(CRED(), mp->mi.m_vfsp) == 0)) {
				ok = 1;
			}
			if (!ok) {
				if (err == 0)
					err = EACCES;
				goto out;
			}
		} else {
			/*
			 * user supplied file descriptor.  Must be superuser or
			 * writable.
			 */
			if ((err = sam_access_ino(ip, S_IWRITE, FALSE,
			    CRED()))) {
				goto out;
			}
		}
		break;

	case SAM_QOP_QSTAT:
		args.qflags = 0;
		for (i = 0; i < SAM_QUOTA_DEFD; i++) {
			if (mp->mi.m_quota_ip[i] != NULL)
				args.qflags |= (1 << i);
		}
		goto copyout;

	case SAM_QOP_PUT:
	case SAM_QOP_PUTALL:
	default:
		err = EINVAL;
		goto out;
	}

	if (args.qtype >= SAM_QUOTA_DEFD) {
		TRACE(T_SAM_QUOTA_OP, fp->f_vnode, __LINE__,
		    args.qcmd, args.qflags);
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

	/*
	 * mp now tells us which FS, args.qtype which quota type, and
	 * args.qindex which entry.  Uqp points to the user buffer.  The
	 * command must be either SAM_QOP_PUT[ALL] or SAM_QOP_GET.
	 * Rock 'n Roll.
	 */
	OPTRACE(fp->f_vnode, __LINE__, args.qcmd, args.qflags);

	if ((err = sam_quota_get_info(mp, args.qtype, args.qindex, &q)) < 0) {
		TRACE(T_SAM_QUOTA_OPERR, fp->f_vnode, __LINE__, 0, ENOENT);
		goto out;
	}
	OPTRACE(fp->f_vnode, __LINE__, args.qcmd, args.qflags);
	if (copyout(&q, uqp, sizeof (q))) {
		err = EFAULT;
	}

copyout:
	OPTRACE(fp->f_vnode, __LINE__, args.qcmd, err);
	if (copyout(&args, argp, sizeof (args))) {
		err = EFAULT;
	}
out:
	if (fp) {
		releasef(args.qfd);
	}
	return (err);
}


/*
 * ----- sam_is_quota_inode -
 */

int
sam_is_quota_inode(sam_mount_t *mp, sam_node_t *ip)
{
	int i;

	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		if (ip == mp->mi.m_quota_ip[i])
			return (1);
	}
	return (0);
}


/*
 * ----- sam_quota_fonline - Check if quota file offline
 */

int
sam_quota_fonline(sam_mount_t *mp, sam_node_t *ip, cred_t *cr)
{
	int i, j, err = 0;

	if ((mp->mt.fi_config & MT_QUOTA) == 0) {
		return (0);
	}

	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		if (ip->bquota[i] != NODQUOT && mp->mi.m_quota_ip[i]) {
			err = sam_quota_fcheck(mp, i,
			    sam_quota_get_index(ip, i), 0, 0, cr);
			if (err) {
				break;
			}
		}
	}
	if (!err) {
		return (0);
	}
	for (j = i-1; j >= 0; j--) {
		if (ip->bquota[j] != NODQUOT && mp->mi.m_quota_ip[j]) {
			err = sam_quota_fcheck(mp, j,
			    sam_quota_get_index(ip, j), 0, 0, cr);
			if (err) {
				cmn_err(CE_WARN, "sam_quota_fonline:  "
				    "Error returning file to '%s' quota %d",
				    quota_types[j], sam_quota_get_index(ip, j));
			}
		}
	}
	return (EDQUOT);
}


/*
 * ----- sam_quota_foffline - Check if quota file offline
 */

int
sam_quota_foffline(sam_mount_t *mp, sam_node_t *ip, cred_t *cr)
{
	int i, err = 0;

	if ((mp->mt.fi_config & MT_QUOTA) == 0) {
		return (0);
	}

	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		if (ip->bquota[i] != NODQUOT && mp->mi.m_quota_ip[i]) {
			err = sam_quota_fcheck(mp, i,
			    sam_quota_get_index(ip, i), 0, 0, cr);
			if (err) {
				cmn_err(CE_WARN, "Error returning file "
				    "to '%s' quota %i",
				    quota_types[i], sam_quota_get_index(ip, i));
			}
		}
	}
	return (0);
}


/*
 * ----- sam_quota_inode_fini - remove quota file references from inode
 * Decrement references to quota records.
 */

void
sam_quota_inode_fini(struct sam_node *ip)
{
	struct sam_quot *qp;
	struct sam_mount *mp;
	int i;

	if (!ip->mp->mi.m_quota_hash) {
		return;
	}

	TRACE(T_SAM_QUOTA_INO_FIN, SAM_ITOV(ip), (int)ip->di.id.ino, 0, 0);

	for (i = 0; i < SAM_QUOTA_DEFD; i++) {
		qp = ip->bquota[i];
		if (qp != NULL && qp != NODQUOT) {
			mp = ip->mp;

			ASSERT(!SAM_IS_SHARED_CLIENT(mp));

			mutex_enter(&qp->qt_lock);
			/*
			 * Potential race condition here; check to ensure that
			 * the link to the quota record hasn't gone away
			 * already.
			 */
			if (qp == ip->bquota[i]) {
				sam_quot_unhold(qp);
				ip->bquota[i] = NULL;
				sam_quot_rel(mp, qp);
				TRACE(T_SAM_QUOTA_INO_FIN, SAM_ITOV(ip),
				    (int)ip->di.id.ino, i, 1);
			} else {
				mutex_exit(&qp->qt_lock);
			}
		}
	}
}

/*
 * ----- sam_quota_get_index_di - return quota index from inode according to
 * quota type.
 */

int
sam_quota_get_index_di(sam_disk_inode_t *di, int type)
{
	switch (type) {
	case SAM_QUOTA_ADMIN:
		return (MAX(-1, di->admin_id));

	case SAM_QUOTA_GROUP:
		return (MAX(-1, di->gid));

	case SAM_QUOTA_USER:
		return (MAX(-1, di->uid));

	default:
		cmn_err(CE_WARN, "bad quota type: %d", type);
		return (-1);
	}
}


/*
 * ----- sam_quota_get_index - return quota index from inode according to
 * quota type.
 */

int
sam_quota_get_index(sam_node_t *ip, int type)
{
	if (S_ISSEGS(&ip->di)) {
		ip = ip->segment_ip;
	}
	return (sam_quota_get_index_di(&ip->di, type));
}


/*
 * check the quota limits.  SAM_QT_OK (0) ==> quotas OK, not over limit.
 */

/* ARGSUSED1 */
int
testquota(struct sam_dquot *pr, int checktotal)
{
	int r = SAM_QT_OK;

	if (!QUOTA_SANE(pr))
		return (SAM_QT_ERROR);

	if (QUOTA_INF(pr))
		return (SAM_QT_OK);

	/*
	 * Now compare use with limits
	 */
	if (pr->dq_folused > pr->dq_folsoft)
		r |= SAM_QT_OL_FILE_SOFT;
	if (pr->dq_folused > pr->dq_folhard)
		r |= SAM_QT_OL_FILE_HARD;

#ifdef	SAM_QUOTA_TOTALS
	if (checktotal) {
		if (pr->dq_ftotused > pr->dq_ftotsoft)
			r |= SAM_QT_TOT_FILE_SOFT;
		if (pr->dq_ftotused > pr->dq_ftothard)
			r |= SAM_QT_TOT_FILE_HARD;
	}
#endif

	if (pr->dq_bolused > pr->dq_bolsoft)
		r |= SAM_QT_OL_BLOCK_SOFT;
	if (pr->dq_bolused > pr->dq_bolhard)
		r |= SAM_QT_OL_BLOCK_HARD;

	if (checktotal) {
		if (pr->dq_btotused > pr->dq_btotsoft)
			r |= SAM_QT_TOT_BLOCK_SOFT;
		if (pr->dq_btotused > pr->dq_btothard)
			r |= SAM_QT_TOT_BLOCK_HARD;
	}

	return (r);
}

/*
 * Update inode counts in quota struct.
 *
 * n > 0 ==> file creation
 * n < 0 ==> file deletion
 *
 */

int
sam_quota_fcheck(sam_mount_t *mp, int type, int index, int ol, int tot,
    cred_t *credp)
{
	struct sam_quot *qp;
	int check, checktotal, before, after, changes, bit;

	if (mp->mi.m_quota_ip[type] == NULL) {
		return (0);
	}

	if (index < 0) {
		cmn_err(CE_WARN, "sam_quota_fcheck:  index (%d) < 0\n", index);
		return (EDQUOT);
	}
	/*
	 * No complicated cases.
	 */
	if (ol < 0 && tot > 0) {
		cmn_err(CE_WARN, "sam_quota_fcheck:  ol < 0 && tot > 0\n");
		return (EDQUOT);
	}
	if (tot < 0 && ol > 0) {
		cmn_err(CE_WARN, "sam_quota_fcheck:  ol > 0 && tot < 0\n");
		return (EDQUOT);
	}

	if ((qp = sam_quot_get(mp, NULL, type, index)) == NULL) {
		return (0);
	}

	ASSERT(qp);

	check = (tot+ol > 0) && (index != 0) && credp &&
	    (crgetuid(credp) != 0);
	checktotal = (mp->mt.fi_config & MT_SAM_ENABLED);

	before = testquota(&qp->qt_dent, checktotal);
	if (check) {
		if (before &
		    (SAM_QT_ERROR|SAM_QT_OL_FILE_HARD|SAM_QT_TOT_FILE_HARD))
			goto errout;
		if (ol > 0 && (before&SAM_QT_OL_FILE_SOFT) &&
		    qp->qt_dent.dq_ol_enforce != 0 &&
		    qp->qt_dent.dq_ol_enforce < NOW())
			goto errout;
		if (tot > 0 && (before&SAM_QT_TOT_FILE_SOFT) &&
		    qp->qt_dent.dq_tot_enforce != 0 &&
		    qp->qt_dent.dq_tot_enforce < NOW())
			goto errout;
		if ((before&(SAM_QT_OL_BLOCK_SOFT|SAM_QT_OL_FILE_SOFT)) == 0)
			qp->qt_dent.dq_ol_enforce = 0;
		if ((before&(SAM_QT_TOT_BLOCK_SOFT|SAM_QT_TOT_FILE_SOFT)) == 0)
			qp->qt_dent.dq_tot_enforce = 0;
	}
	qp->qt_dent.dq_folused += ol;
	qp->qt_dent.dq_ftotused += tot;
	after = testquota(&qp->qt_dent, checktotal);

	if (check) {
		if (after &
		    (SAM_QT_ERROR|SAM_QT_OL_FILE_HARD|SAM_QT_TOT_FILE_HARD)) {
			qp->qt_dent.dq_folused -= ol;
			qp->qt_dent.dq_ftotused -= tot;
			goto errout;
		}
	}
	qp->qt_flags |= SAM_QF_DIRTY;

	/*
	 * Walk through the changed bits, one at a time.
	 */
	for (changes = before^after; changes; changes &= ~bit) {
		bit = changes^(changes&(changes-1));	/* extract LSB */
		switch (bit) {
		case SAM_QT_OL_FILE_SOFT:
			if (after&SAM_QT_OL_FILE_SOFT) {
				if (qp->qt_index &&
				    (qp->qt_flags&SAM_QF_OVEROF) == 0) {
					uwarn("SAM-QFS:  Over %s/%d "
					    "online file soft limit\n",
					    quota_types[type], index);
				}
				qp->qt_flags |= SAM_QF_OVEROF;
			} else {
				qp->qt_flags &= ~SAM_QF_OVEROF;
				if ((after &
				    (SAM_QT_OL_FILE_SOFT|
				    SAM_QT_OL_BLOCK_SOFT)) == 0)
					qp->qt_dent.dq_ol_enforce = 0;
			}
			break;

		case SAM_QT_TOT_FILE_SOFT:
			if (after&SAM_QT_TOT_FILE_SOFT) {
				if (qp->qt_index &&
				    (qp->qt_flags&SAM_QF_OVERTF) == 0) {
					uwarn("SAM-QFS:  Over %s/%d "
					    "total file soft limit\n",
					    quota_types[type], index);
				}
				qp->qt_flags |= SAM_QF_OVERTF;
			} else {
				qp->qt_flags &= ~SAM_QF_OVERTF;
				if ((after &
				    (SAM_QT_TOT_FILE_SOFT|
				    SAM_QT_TOT_BLOCK_SOFT)) == 0)
					qp->qt_dent.dq_tot_enforce = 0;
			}
			break;

		/*
		 * Not necessarily wrong to flip the HARD quota bits; root
		 * can set quotas down or super-user can chown/chgrp files.
		 * When user gets below hard quota, these will kick off.
		 */
		case SAM_QT_OL_FILE_HARD:
			if (after&SAM_QT_OL_FILE_HARD) {
				if (qp->qt_index) {
					uwarn("SAM-QFS:  Over %s/%d online "
					    "file hard limit\n",
					    quota_types[type], index);
				}
				qp->qt_flags |= SAM_QF_OVEROF;
			} else {
				if (qp->qt_index) {
					uwarn("SAM-QFS:  Now under %s/%d "
					    "online file hard limit\n",
					    quota_types[type], index);
				}
			}
			break;

		case SAM_QT_TOT_FILE_HARD:
			if (after&SAM_QT_TOT_FILE_HARD) {
				if (qp->qt_index) {
					uwarn("SAM-QFS: Over %s/%d "
					    "total file hard limit\n",
					    quota_types[type], index);
				}
				qp->qt_flags |= SAM_QF_OVERTF;
			} else {
				if (qp->qt_index) {
					uwarn("SAM-QFS:  Now under %s/%d "
					    "total file hard limit\n",
					    quota_types[type], index);
				}
			}
			break;

			/*
			 * Definitely unexpected.
			 */
		default:
			cmn_err(CE_WARN, "SAM-QFS:  Unexpected %s/%d "
			    "file quota status "
			    "change:  was %x, is %x, ck = %d",
			    quota_types[type], index, before, after, check);
			uwarn("SAM-QFS:  Unexpected %s/%d file quota "
			    "status change:  "
			    "was %x, is %x, ck = %d\n",
			    quota_types[type], index, before, after, check);
		}
	}
	/*
	 * Since we could have been over quota and cleared a grace counter,
	 * we need to restart it now.
	 */
	if (after&SAM_QT_OL_FILE_SOFT && qp->qt_dent.dq_ol_enforce == 0) {
		if (qp->qt_index && (qp->qt_flags&SAM_QF_OVEROF) == 0) {
			uwarn("SAM-QFS:  Over %s/%d online file soft limit\n",
			    quota_types[type], index);
		}
		qp->qt_dent.dq_ol_enforce = NOW() + qp->qt_dent.dq_ol_grace;
		qp->qt_flags |= SAM_QF_OVEROF;
	}
	if (after&SAM_QT_TOT_FILE_SOFT && qp->qt_dent.dq_tot_enforce == 0) {
		if (qp->qt_index && (qp->qt_flags&SAM_QF_OVERTF) == 0) {
			uwarn("SAM-QFS:  Over %s/%d total file soft limit\n",
			    quota_types[type], index);
		}
		qp->qt_dent.dq_tot_enforce = NOW() + qp->qt_dent.dq_tot_grace;
		qp->qt_flags |= SAM_QF_OVERTF;
	}

	qp->qt_lastref = NOW();
	sam_quot_rel(mp, qp);
	return (0);

errout:
	qp->qt_lastref = NOW();
	sam_quot_rel(mp, qp);
	return (EDQUOT);
}


/*
 * sam_quot_get -
 * Grab a sam_quot structure.  If there's one with the right info
 * in the hash table, get a lock on it and return it.  If not, then
 * if there's one on the free list that's sufficiently old, get it,
 * reassign it, and return it.  If that fails, allocate a new batch
 * and return one of them.
 *
 * This function should never return NODQUOT, tho' it may set that
 * value into the provided inode (if any).  It must also recognize
 * the NODQUOT value in the provided inode (if any), and return NULL
 * in that case.
 */

struct sam_quot *
sam_quot_get(sam_mount_t *mp, sam_node_t *ip, int type, int index)
{
	struct sam_quot *qp;
	int error = 0;

	TRACE(T_SAM_QUOT_GET, ip ? SAM_ITOV(ip) : NULL, type, index, 0);

	if (type < 0 || type >= SAM_QUOTA_DEFD) {
		TRACE(T_SAM_QUOT_NODQ, ip ? SAM_ITOV(ip) : NULL,
		    type, index, 0);
		return (NULL);
	}
	if (ip != NULL && S_ISSEGS(&ip->di)) {
		ip = ip->segment_ip;
	}
	if (ip != NULL && ip->bquota[type] == NODQUOT) {
		TRACE(T_SAM_QUOT_NODQ, SAM_ITOV(ip), type, index, 1);
		return (NULL);
	}
	if (index < 0 || mp->mi.m_quota_ip[type] == NULL) {
		TRACE(T_SAM_QUOT_NODQ, ip ? SAM_ITOV(ip) : NULL,
		    type, index, 2);
		if (ip != NULL) {
			ip->bquota[type] = NODQUOT;
		}
		return (NULL);
	}

	/*
	 * 0 <= type < SAM_QUOTA_DEFD.
	 * index >= 0.
	 * mp->mi.m_quota_ip[type] != NULL.
	 * !(ip == NULL && ip->bquota[type] == NODQUOT)
	 */
	if (ip != NULL && (qp = ip->bquota[type]) != NULL) {
		mutex_enter(&qp->qt_lock);
		if (type == qp->qt_type && index == qp->qt_index) {
			TRACE(T_SAM_QUOT_GOTATT, SAM_ITOV(ip),
			    qp->qt_type, qp->qt_index, (sam_tr_t)qp);
			return (qp);
		}
		/*
		 * This shouldn't happen.  Inode has pointers, and they
		 * point to a record that doesn't match the inode's info.
		 */
		cmn_err(SAMFS_DEBUG_PANIC,
		    "SAM-QFS: %s: sam_quot_get:  unexpected quota pointer,"
		    " (typ=%d/%d, index=%d/%d, qp=%lx, ip=%lx)\n",
		    mp->mt.fi_name, type, qp->qt_type, index,
		    qp->qt_index, (long)qp, (long)ip);
		sam_quot_unhold(qp);
		ip->bquota[type] = NULL;
		sam_quot_rel(mp, qp);		/* releases qp->qt_lock */
	}

	TRACE(T_SAM_QUOT_SRCH, ip ? SAM_ITOV(ip) : NULL, type, index, 0);
again:
	mutex_enter(&mp->mi.m_quota_hashlock);
	qp = mp->mi.m_quota_hash[index%SAM_QUOTA_HASH_SIZE];
	while (qp) {
		if (qp->qt_index == index && qp->qt_type == type)
			break;
		qp = qp->qt_hash.next;
	}

	OPTRACE(NULL, __LINE__, type, index);
	if (qp) {
		if (mutex_tryenter(&qp->qt_lock))
			goto out_w_hash;
		mutex_exit(&mp->mi.m_quota_hashlock);
		mutex_enter(&qp->qt_lock);
		if (qp->qt_index != index || qp->qt_type != type) {
			mutex_exit(&qp->qt_lock);
			goto again;
		}
		goto out_wo_hash;
	}
	OPTRACE(NULL, __LINE__, (int)qp, index);
	/*
	 * No entry.  Go get one, and hash it in.
	 *
	 * We may need to hold the hashlock here for a long time.  When
	 * we decide that we need to allocate a new entry for this quota,
	 * we can't allow another thread to enter and discover that
	 * there's no quota entry for it, and allocate a second one.
	 * So we hold the hashlock to prevent any other threads from
	 * getting in until we've got one allocated and hashed.
	 * Once it's on the hash list, we can drop the hashlock, and
	 * initiate the read-in from disk.  Until it's read in, we
	 * hold the lock on it so nobody tries to check quotas with it.
	 *
	 */
	qp = sam_quot_getfree(mp);

	OPTRACE(NULL, __LINE__, (int)qp, index);
	ASSERT(MUTEX_HELD(&qp->qt_lock));
	qp->qt_vp = SAM_ITOV(mp->mi.m_quota_ip[type]);
	qp->qt_index = index;
	qp->qt_type = type;
	qp->qt_ref = 0;
	qp->qt_flags = 0;
	qp->qt_lastref = NOW();
	qp->qt_hash.next = mp->mi.m_quota_hash[index%SAM_QUOTA_HASH_SIZE];
	mp->mi.m_quota_hash[index%SAM_QUOTA_HASH_SIZE] = qp;
	qp->qt_flags |= SAM_QF_HASHED;
	mutex_exit(&mp->mi.m_quota_hashlock);
	OPTRACE(NULL, __LINE__, (int)qp, index);
	error = sam_quota_read(qp);
	if (error) {
		cmn_err(CE_WARN, "Quota read failed (%d).  Zeroing entry.",
		    error);
		bzero((void *)&qp->qt_dent, sizeof (qp->qt_dent));
	}
	OPTRACE(NULL, __LINE__, (int)qp, error);
	if (ip != NULL && ip->bquota[type] == NULL) {
		sam_quot_hold(qp);
		ip->bquota[type] = qp;
	}
	TRACE(T_SAM_QUOT_GOTNEW, ip ? SAM_ITOV(ip) : NULL,
	    qp->qt_type, qp->qt_index, (sam_tr_t)qp);
	return (qp);

out_w_hash:		/* have locked entry and hash lock  */
	OPTRACE(NULL, __LINE__, (int)qp, index);
	mutex_exit(&mp->mi.m_quota_hashlock);

out_wo_hash:	/* have locked entry */
	OPTRACE(NULL, __LINE__, (int)qp, index);
	mutex_enter(&mp->mi.m_quota_availlock);
	if (qp->qt_flags&SAM_QF_AVAIL) {
		sam_quot_unfree(mp, qp);
	}
	mutex_exit(&mp->mi.m_quota_availlock);
	if (ip != NULL && ip->bquota[type] == NULL) {
		sam_quot_hold(qp);
		ip->bquota[type] = qp;
	}
	TRACE(T_SAM_QUOT_GOT, ip ? SAM_ITOV(ip) : NULL,
	    qp->qt_type, qp->qt_index, (sam_tr_t)qp);
	return (qp);
}


/*
 * sam_quot_unfree -
 * Remove quota structure from free list.  Assumes avail lock and
 * qt_lock held.
 *
 * Because we use an anchored circular list, we don't have to worry
 * about ever having to adjust the head or tail pointer of the list.
 * So, in fact, we don't need the mount point parameter, except maybe
 * to verify that we're holding the right freelock.
 */

void
sam_quot_unfree(sam_mount_t *mp, struct sam_quot *dqp)
{
	ASSERT(MUTEX_HELD(&mp->mi.m_quota_availlock));
	ASSERT(MUTEX_HELD(&dqp->qt_lock));
	ASSERT((dqp->qt_flags&SAM_QF_ANCHOR) == 0);
	ASSERT((dqp->qt_flags&SAM_QF_AVAIL) != 0);
	ASSERT(dqp->qt_ref == 0);

	dqp->qt_avail.next->qt_avail.prev = dqp->qt_avail.prev;
	dqp->qt_avail.prev->qt_avail.next = dqp->qt_avail.next;
	dqp->qt_avail.next = NULL;
	dqp->qt_avail.prev = NULL;
	dqp->qt_flags &= ~SAM_QF_AVAIL;
}


/*
 * - sam_quot_rel
 * Unlock a locked quota struct.  If the reference count is zero,
 * put it on the back of the freelist.
 */

void
sam_quot_rel(sam_mount_t *mp, struct sam_quot *qp)
{
	ASSERT(MUTEX_HELD(&qp->qt_lock));
	ASSERT(qp != NULL && qp != NODQUOT);

	TRACE(T_SAM_QUOT_REL, mp, (sam_tr_t)qp, qp->qt_ref, 0);
	sam_quota_flush(qp);
	if (qp->qt_ref == 0) {
		sam_quot_free2back(mp, qp);
	}
	mutex_exit(&qp->qt_lock);
}


/*
 * ---- sam_quota_flush
 * Write a locked quota structure to disk.
 */

int
sam_quota_flush(struct sam_quot *qp)
{
	sam_node_t *ip;
	vnode_t *vp;
	uio_t iop;
	iovec_t iov;
	int error;

	ASSERT(qp != NULL && qp != NODQUOT);
	ASSERT(MUTEX_HELD(&qp->qt_lock));

	TRACE(T_SAM_QUOTA_FLUSH, qp->qt_vp,
	    qp->qt_type, qp->qt_index, (sam_tr_t)qp);

	if ((qp->qt_flags&SAM_QF_DIRTY) == 0) {
		return (0);
	}

	if (qp->qt_flags&SAM_QF_BUSY) {
		return (0);
	}

	vp = qp->qt_vp;
	ip = SAM_VTOI(vp);
	if (ip->mp->mi.m_quota_ip[qp->qt_type] == NULL) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: cyclic quota adjustment during shutdown.\n",
		    ip->mp->mt.fi_name);
		return (ENXIO);
	}

	ASSERT(!SAM_IS_SHARED_CLIENT(ip->mp));

	TRACE(T_SAM_QUOTA_FLUSH2, qp->qt_vp,
	    qp->qt_type, qp->qt_index, (sam_tr_t)qp);
	qp->qt_flags |= SAM_QF_BUSY;
	qp->qt_flags &= ~SAM_QF_DIRTY;
	iov.iov_len = sizeof (struct sam_dquot);
	iov.iov_base = (void *)&qp->qt_dent;
	iop.uio_iov = &iov;
	iop.uio_iovcnt = 1;
	iop.uio_loffset =
	    ((u_offset_t)qp->qt_index) * sizeof (struct sam_dquot);
	iop.uio_segflg = UIO_SYSSPACE;
	iop.uio_fmode = FWRITE;
	iop.uio_resid = sizeof (struct sam_dquot);
	iop.uio_llimit = SAM_MAXOFFSET_T;

	sam_rwlock_vn(vp, 1, NULL);
	OPTRACE(vp, __LINE__, (sam_tr_t)qp, qp->qt_index);
	error = sam_write_vn(vp, &iop, 0, CRED(), NULL);
	sam_rwunlock_vn(vp, 1, NULL);

	if (error) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: quota write error %d, resid = %d\n",
		    ip->mp->mt.fi_name, error, (int)iop.uio_resid);
	}
	TRACE(T_SAM_QUOTA_FLUSHD, qp->qt_vp, (sam_tr_t)qp, 0, error);
	qp->qt_flags &= ~SAM_QF_BUSY;
	return (error);
}


/*
 * sam_quot_getfree - Hashlock must be held on entry.
 */

static struct sam_quot *
sam_quot_getfree(sam_mount_t *mp)
{
	int i, n = 0;
	struct sam_quot *qp;

	ASSERT(MUTEX_HELD(&mp->mi.m_quota_hashlock));

	for (;;) {
		OPTRACE(NULL, __LINE__, (int)mp, 0);
		if ((qp = sam_quot_getfront(mp)) != NULL) {
			sam_quot_unhash(mp, qp);
			return (qp);
		}
		/*
		 * No free structures, or the oldest available one has been too
		 * recently used.  Allocate a bunch, put them on the front of
		 * the freelist, and try again.
		 */
		for (i = 0; i < SAM_QUOTA_NALLOC; i++) {
			OPTRACE(NULL, __LINE__, (int)mp, i);
			qp = (struct sam_quot *)
			    KMA(sizeof (struct sam_quot), KM_SLEEP);
			sam_mutex_init(&qp->qt_lock, NULL, MUTEX_DRIVER, NULL);
			mutex_enter(&qp->qt_lock);
			sam_quot_free2front(mp, qp);
			mutex_exit(&qp->qt_lock);
		}
		if ((++n % 3) == 0) {
			cmn_err(CE_WARN, "sam_quot_getfree:  allocation loop");
		}
	}

#ifdef	lint
	/*NOTREACHED*/
	return (NULL);
#endif
}


/*
 * sam_quot_getfront -
 * Get oldest free quota structure, if more than SAM_QUOTA_EXPIRE
 * seconds old.  No locks on entry.  On non-NULL return, the returned
 * sam_quot structure is locked.
 */
static struct sam_quot *
sam_quot_getfront(sam_mount_t *mp)
{
	struct sam_quot *r;

	mutex_enter(&mp->mi.m_quota_availlock);
	OPTRACE(NULL, __LINE__, (int)mp, 0);

	OPTRACE(NULL, __LINE__, (int)mp, 0);
	r = mp->mi.m_quota_avail->qt_avail.next;
	if (r->qt_flags & SAM_QF_ANCHOR) {		/* freelist empty? */
		r = NULL;
		goto out;
	}
	if (!mutex_tryenter(&r->qt_lock)) {
		/*
		 * Quota struct was apparently obtained through the
		 * hashlist while we held the free lock.
		 */
		mutex_exit(&mp->mi.m_quota_availlock);
		mutex_enter(&r->qt_lock);
		mutex_enter(&mp->mi.m_quota_availlock);
		if ((r->qt_flags&SAM_QF_AVAIL) == 0) {
			mutex_exit(&r->qt_lock);
			r = NULL;
			goto out;
		}
	}
	if ((r->qt_flags & (SAM_QF_BUSY|SAM_QF_DIRTY)) ||
	    (r->qt_lastref + SAM_QUOTA_EXPIRE > NOW())) {
		mutex_exit(&r->qt_lock);
		r = NULL;
		goto out;
	}
	sam_quot_unfree(mp, r);

out:
	OPTRACE(NULL, __LINE__, (int)mp, 0);
	mutex_exit(&mp->mi.m_quota_availlock);
	return (r);
}


/*
 * sam_quot_hold -
 * Increment the hold count on a quota structure.  This is
 * normally done when a file open for writing is written to
 * and causes its first allocation.  The sam_quot structure
 * pointer is stuck in the inode, and we cannot free it while
 * any inodes have outstanding refs.
 */

static void
sam_quot_hold(struct sam_quot *qp)
{
	ASSERT(qp != NULL && qp != NODQUOT);
	ASSERT(MUTEX_HELD(&qp->qt_lock));
	ASSERT((qp->qt_flags&SAM_QF_AVAIL) == 0);
	ASSERT(qp->qt_ref >= 0);

	++qp->qt_ref;
}


/*
 * sam_quot_unhold -
 * Decrement the hold count on a quota structure.
 * Doesn't do much; but when next unlocked, the structure
 * will be pushed out to disk and put on the free list
 * where it can start aging.
 */
static void
sam_quot_unhold(struct sam_quot *qp)
{
	ASSERT(qp != NULL && qp != NODQUOT);
	ASSERT(MUTEX_HELD(&qp->qt_lock));
	ASSERT((qp->qt_flags&SAM_QF_AVAIL) == 0);
	ASSERT(qp->qt_ref >= 1);

	--qp->qt_ref;
}


/*
 * sam_quot_unhash -
 * Remove dqp from its hash chain.  Quota entry locked on entry and
 * exit, and must not be unlocked between, since our intention is
 * probably to reassign the locked entry to another quota record.
 */
void
sam_quot_unhash(sam_mount_t *mp, struct sam_quot *dqp)
{
	struct sam_quot **hqp;

	ASSERT(MUTEX_HELD(&mp->mi.m_quota_hashlock));
	ASSERT(MUTEX_HELD(&dqp->qt_lock));
	ASSERT(dqp->qt_ref == 0);

	if ((dqp->qt_flags&SAM_QF_HASHED) == 0) {
		return;
	}
	hqp = &mp->mi.m_quota_hash[dqp->qt_index%SAM_QUOTA_HASH_SIZE];
	while (*hqp != NULL && *hqp != dqp)
		hqp = &(*hqp)->qt_hash.next;
	if (!*hqp) {
		cmn_err(CE_WARN,
		    "sam_quot_unhash:  quota struct not found on hash list");
	}
	*hqp = dqp->qt_hash.next;
	dqp->qt_flags &= ~SAM_QF_HASHED;
}


/*
 * sam_quota_read_entry -
 * Disk stuff
 *
 * The quota entries are NOT offset by one; the zeroth entry in
 * the quota file is the 'default', and read in for any unset quota
 * entries.  Note that the quota file may be sparse, so we need
 * to allocate and zero the block before doing the read if a
 * block isn't already allocated.
 */

static int
sam_quota_read_entry(sam_node_t *ip, struct sam_dquot *dqp, int type,
    int index)
{
	int error;
	uio_t iop;
	iovec_t iov;
	vnode_t *vp;

	vp = SAM_ITOV(ip);

	if (SAM_IS_SHARED_CLIENT(ip->mp)) {
		TRACE(T_SAM_QUOTA_READ, SAM_ITOV(ip), 99, type, index);
		return (EREMOTE);		/* in failover? */
	}
	iov.iov_len = sizeof (*dqp);
	iov.iov_base = (void *)dqp;
	iop.uio_iov = &iov;
	iop.uio_iovcnt = 1;
	iop.uio_loffset = (u_offset_t)index * sizeof (*dqp);
	if (iop.uio_loffset >= ip->di.rm.size) {
		goto read_default;
	}
	iop.uio_segflg = UIO_SYSSPACE;
	iop.uio_fmode = FREAD;
	iop.uio_resid = sizeof (*dqp);
	iop.uio_llimit = SAM_MAXOFFSET_T;
	TRACE(T_SAM_QUOTA_READ, SAM_ITOV(ip), 1, type, index);

	sam_rwlock_vn(vp, 0, NULL);
	error = sam_read_vn(vp, &iop, 0, CRED(), NULL);
	sam_rwunlock_vn(vp, 0, NULL);

	if (error) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: %s quota record %d read error %d, "
		    "resid = %d\n",
		    ip->mp->mt.fi_name,
		    quota_types[type], index,
		    error, (int)iop.uio_resid);
		goto out;
	}
	if (sam_zerocmp((void *)dqp, sizeof (*dqp))) {
		goto read_default;
	}
out:
	return (error);

read_default:
	/*
	 * Initialize user's file + block limits to index zero's (uid 0,
	 * gid 0, or aid 0).  Initialize the "in use" values to zero.
	 * And if, somehow, root's enforcement times have gotten set,
	 * we don't want those copied either.  Just the limits and
	 * grace periods.
	 */
	iov.iov_len = sizeof (*dqp);
	iov.iov_base = (void *)dqp;
	iop.uio_iov = &iov;
	iop.uio_iovcnt = 1;
	iop.uio_loffset = 0;
	iop.uio_segflg = UIO_SYSSPACE;
	iop.uio_fmode = FREAD;
	iop.uio_resid = sizeof (*dqp);
	iop.uio_llimit = SAM_MAXOFFSET_T;
	TRACE(T_SAM_QUOTA_READDEF, SAM_ITOV(ip), 1, type, index);

	sam_rwlock_vn(vp, 0, NULL);
	error = sam_read_vn(vp, &iop, 0, CRED(), NULL);
	sam_rwunlock_vn(vp, 0, NULL);

	if (error) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: %s quota read_default (%d) error %d, "
		    "resid = %d\n",
		    ip->mp->mt.fi_name, quota_types[type], index,
		    error, (int)iop.uio_resid);
		goto out;
	}
	dqp->dq_bolused = 0;
	dqp->dq_btotused = 0;
	dqp->dq_folused = 0;
	dqp->dq_ftotused = 0;
	dqp->dq_ol_enforce = 0;
	dqp->dq_tot_enforce = 0;
	return (error);
}

static int
sam_quota_read(struct sam_quot *qp)
{
	sam_node_t *ip;
	int error = 0;

	ASSERT(qp != NULL && qp != NODQUOT);
	ASSERT(MUTEX_HELD(&qp->qt_lock));

	if (qp->qt_index < 0) {
		cmn_err(CE_WARN,
		    "SAM-QFS: sam_quota_read:  qp->qt_index (%d) < 0\n",
		    qp->qt_index);
		bzero((char *)&qp->qt_dent, sizeof (struct sam_dquot));
		return (EINVAL);
	}

	ip = SAM_VTOI(qp->qt_vp);
	qp->qt_flags |= SAM_QF_BUSY;
	error = sam_quota_read_entry(ip, &qp->qt_dent, qp->qt_type,
	    qp->qt_index);
	qp->qt_flags &= ~SAM_QF_BUSY;
	return (error);
}


/*
 * sam_quota_get_info
 *
 * Get the quota entry for the appropriate info, and copy it out to
 * *dqp.  If this is a client host, get it from the server.  If it's
 * the server or a standalone host, get it out of a quota record.
 */
static int
sam_quota_get_info(sam_mount_t *mp, int typ, int idx, struct sam_dquot *dqp)
{
	int error = 0;

	if (SAM_IS_SHARED_CLIENT(mp)) {
		struct sam_block_quota *qmsg;

		qmsg = (struct sam_block_quota *)kmem_zalloc(sizeof (*qmsg),
		    KM_SLEEP);
		qmsg->operation = SAM_QOP_GET;
		qmsg->type = typ;
		qmsg->index = idx;
		qmsg->len = sizeof (*dqp);
		qmsg->buf = (int64_t)(void *)dqp;
		error = sam_proc_block(mp, NULL, BLOCK_quota, SHARE_wait, qmsg);
		kmem_free(qmsg, sizeof (*qmsg));
	} else {
		struct sam_quot *qp;

		qp = sam_quot_get(mp, NULL, typ, idx);
		if (qp) {
			bcopy((char *)&qp->qt_dent, (char *)dqp, sizeof (*dqp));
			sam_quot_rel(mp, qp);
		} else {
			TRACE(T_SAM_QUOTA_OPERR, NULL, __LINE__, idx, ENOENT);
			error = ENOENT;
		}
	}
	return (error);
}


/*
 * sam_zerocmp -
 * test for uninitialized quota record
 */

static int
sam_zerocmp(void *p, int n)
{
	char *cp;

	for (cp = (char *)p; n > 0; n--) {
		if (*cp++ != 0) {
			return (0);
		}
	}
	return (1);
}


/*
 * - sam_quot_free2back
 * Add quota structure to free list.  Free list must be held on entry.
 *
 * "Front" holds the oldest structures, or newly allocated (never-assigned)
 * entries.  These are allocated first.  "Back" is where structures go when
 * their reference count goes to zero.  Here they sit, growing older.  If
 * referenced, they're moved to the back again.
 *   When we look for a free structure to plop a new quota record into, we
 * go to the front of the list.  If the record there isn't old enough, then
 * we allocate some more entries.
 *   Structures stay on their hash list until they are reassigned.  I.e.,
 * they can go on and off the freelist all day, and never move off the
 * hashlist.  Only when they expire off the end of the freelist and are
 * reclaimed for another quota record are they reassigned on the hash lists.
 */
static void
sam_quot_free2back(sam_mount_t *mp, struct sam_quot *dqp)
{
	ASSERT(MUTEX_HELD(&dqp->qt_lock));
	ASSERT((dqp->qt_flags&SAM_QF_ANCHOR) == 0);
	ASSERT((dqp->qt_flags&SAM_QF_AVAIL) == 0);
	ASSERT(dqp->qt_ref == 0);

	mutex_enter(&mp->mi.m_quota_availlock);
	dqp->qt_avail.next = mp->mi.m_quota_avail;
	dqp->qt_avail.prev = mp->mi.m_quota_avail->qt_avail.prev;
	dqp->qt_avail.next->qt_avail.prev = dqp;
	dqp->qt_avail.prev->qt_avail.next = dqp;

	dqp->qt_lastref = NOW();
	dqp->qt_flags |= SAM_QF_AVAIL;
	mutex_exit(&mp->mi.m_quota_availlock);
}


/*
 * sam_quot_free2front is normally called only done after allocating
 * new quota records.  New entries are put on the front, where they will
 * be allocated right away.
 */
static void
sam_quot_free2front(sam_mount_t *mp, struct sam_quot *dqp)
{
	ASSERT(MUTEX_HELD(&dqp->qt_lock));
	ASSERT((dqp->qt_flags&SAM_QF_ANCHOR) == 0);
	ASSERT((dqp->qt_flags&SAM_QF_AVAIL) == 0);
	ASSERT(dqp->qt_ref == 0);

	mutex_enter(&mp->mi.m_quota_availlock);
	dqp->qt_avail.next = mp->mi.m_quota_avail->qt_avail.next;
	dqp->qt_avail.prev = mp->mi.m_quota_avail;
	dqp->qt_avail.next->qt_avail.prev = dqp;
	dqp->qt_avail.prev->qt_avail.next = dqp;

	dqp->qt_lastref = NOW()-SAM_QUOTA_EXPIRE-1;
	dqp->qt_flags |= SAM_QF_AVAIL;
	mutex_exit(&mp->mi.m_quota_availlock);
}
