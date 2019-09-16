/*
 * ----- sam/cacl.c - Process the inode access control list functions.
 *
 *	Processes the SAMFS inode access control list functions.
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

#pragma ident "$Revision: 1.31 $"

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/t_lock.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/cred.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/acl.h>
#include <sys/policy.h>

/* ----- SAMFS Includes */

#include "inode.h"
#include "mount.h"
#include "extern.h"
#include "debug.h"
#include "trace.h"
#include "ino_ext.h"
#include "qfs_log.h"

static int sam_get_acl_ext(sam_node_t *bip, sam_ic_acl_t **aclpp);
static int sam_get_acl_ext_cnts(sam_node_t *bip, int *cntp, int *dfcntp);
static int sam_get_acl_ext_data(sam_node_t *bip, int cnt, sam_acl_t *entp,
			int dfcnt, sam_acl_t *dfentp);

extern void bcopy(const void *s1, void *s2, size_t n);

#if defined(ORACLE_SOLARIS)
/* it still exists but nt exported anymore */
int secpolicy_vnode_access(const cred_t *, vnode_t *, uid_t, mode_t);
#endif

/*
 * ----- sam_acl_access - Access control list access.
 *
 * Use inode access control list, if present, to see if mode of
 * access is allowed.
 */
int				/* ERRNO if error, 0 if successful */
sam_acl_access(
	sam_node_t *ip,		/* Pointer to inode */
	mode_t mode,		/* Requested mode permissions */
	cred_t *credp)		/* Credentials pointer */
{
	int error = 0, errline = 0;
	sam_ic_acl_t *aclp;
	int is_def, bits;
	int i;
	int ngroup = 0, group_perm = 0;

	ASSERT(ip->di.status.b.acl);

	/*
	 * If no incore ACLs are present for the inode, get them from the
	 * the inode extension(s) on disk or inherit them from from parent
	 * directory default ACLs.
	 */
	if ((aclp = ip->aclp) == NULL) {
		int got_inode_wlock = RW_WRITE_HELD(&ip->inode_rwl);

		ASSERT(RW_LOCK_HELD(&ip->inode_rwl));

		if (!got_inode_wlock) {
			if (!rw_tryupgrade(&ip->inode_rwl)) {
				RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			}
		}
		if ((aclp = ip->aclp) == NULL) {
			error = sam_get_acl(ip, &aclp);
			if (error != 0 || !aclp) {
				errline = __LINE__ - 1;
				if (!error) {
					error = ENOENT;
				}
				goto out;
			}
			if (ip->aclp != aclp) {
				sam_free_acl(ip);
			}
			ip->aclp = aclp;
		}
		if (!got_inode_wlock) {
			RW_DOWNGRADE_OS(&ip->inode_rwl);
		}
	}

	/*
	 * Capture mask, if defined.
	 */
	is_def = aclp->acl.mask.is_def;
	bits = is_def ? aclp->acl.mask.bits : ~0;

	/*
	 * If user owns the file, obey user mode bits.
	 */
	if (crgetuid(credp) == aclp->acl.owner->a_id) {
		if (MODE_CHECK(mode, aclp->acl.owner->a_perm << 6)) {
			errline = __LINE__ - 1;
			error = EACCES;
		}
		goto out;
	}

	/*
	 * Obey any matching USER ACL entry.
	 */
	if (aclp->acl.nusers) {
		for (i = 0; i < aclp->acl.nusers; i++) {
			if (crgetuid(credp) == aclp->acl.users[i].a_id) {
				if (MODE_CHECK(mode, (bits &
				    aclp->acl.users[i].a_perm) << 6)) {
					errline = __LINE__ - 1;
					error = EACCES;
				}
				goto out;
			}
		}
	}

	/*
	 * Check file's group against caller's group list.  If there's
	 * no mask set, then check against the unix group permission.
	 */
	if (groupmember(aclp->acl.group->a_id, credp)) {
		ngroup++;
		group_perm = aclp->acl.group->a_perm;
		if (!is_def) {
			if (MODE_CHECK(mode, group_perm << 6)) {
				errline = __LINE__ - 1;
				error = EACCES;
			}
			goto out;
		}
	}

	/*
	 * Accumulate the permissions in matching GROUP ACL entries.
	 */
	if (aclp->acl.ngroups) {
		for (i = 0; i < aclp->acl.ngroups; i++) {
			if (groupmember(aclp->acl.groups[i].a_id, credp)) {
				ngroup++;
				group_perm |= aclp->acl.groups[i].a_perm;
			}
		}
	}
	if (ngroup) {
		if (MODE_CHECK(mode, (group_perm & bits) << 6)) {
			errline = __LINE__ - 1;
			error = EACCES;
		}
		goto out;
	}

	/*
	 * Use the OTHER mode bits.
	 */
	if (MODE_CHECK(mode, aclp->acl.other->a_perm << 6)) {
		errline = __LINE__ - 1;
		error = EACCES;
	}

out:
	if (error) {
		if (secpolicy_vnode_access(credp,
		    SAM_ITOV(ip), ip->di.uid, mode) == 0) {
			return (0);
		}
		TRACE(T_SAM_CACL_ERR, SAM_ITOV(ip), (int)ip->di.id.ino,
		    errline, error);
	}
	return (error);
}


/*
 *	-----	sam_synth_acl
 *
 * Synthesize minimal acls from the base inode.
 */
void
sam_synth_acl(sam_node_t *ip, sam_acl_t *entp)
{
	entp->a_type = USER_OBJ;		/* Owner */
	entp->a_id   = ip->di.uid;		/* Really undefined */
	entp->a_perm = ((ushort_t)(ip->di.mode & 0700)) >> 6;
	entp++;

	entp->a_type = GROUP_OBJ;		/* Group */
	entp->a_id   = ip->di.gid;		/* Really undefined */
	entp->a_perm = ((ushort_t)(ip->di.mode & 0070)) >> 3;
	entp++;

	entp->a_type = CLASS_OBJ;		/* Class */
	entp->a_id   = 0;			/* Really undefined */
	entp->a_perm = ((ushort_t)(ip->di.mode & 0070)) >> 3;
	entp++;

	entp->a_type = OTHER_OBJ;		/* Other */
	entp->a_id   = 0;			/* Really undefined */
	entp->a_perm = (ushort_t)(ip->di.mode & 0007);
}


/*
 * ----- sam_acl_get_vsecattr - Get vnode secattrs structure.
 *
 * Return vnode secattrs from access control list struct in
 * incore inode.  Return a dummy regular access control list
 * if an access control list hasn't been specified for the inode.
 *
 * Caller must have the inode read lock held.  Lock is not released.
 */
int				/* ERRNO if error, 0 if successful */
sam_acl_get_vsecattr(
	sam_node_t *ip,		/* Pointer to inode */
	vsecattr_t *vsap)	/* Pointer to vnode secattr struct */
{
	int error = 0, errline = 0;
	sam_ic_acl_t *aclp;
	int aclcnt = 0, dfaclcnt = 0;
	aclent_t *aclentp = NULL;
	aclent_t *dfaclentp = NULL;
	aclent_t *entp = NULL;

	ASSERT(RW_READ_HELD(&ip->inode_rwl));

	if (!CHECK_ACL_ALLOWED(ip->di.mode & S_IFMT)) {
		errline = __LINE__ - 1;
		error = ENOSYS;
		goto out;
	}

	vsap->vsa_aclcnt = vsap->vsa_dfaclcnt = 0;
	vsap->vsa_aclentp = vsap->vsa_dfaclentp = NULL;

	if ((ip->di.status.b.acl || ip->di.status.b.dfacl) &&
	    (ip->di.ext_attrs & ext_acl) == 0) {
		if (ip->aclp) {
			sam_free_acl(ip);
		}
		mutex_enter(&ip->fl_mutex);
		ip->di.status.b.acl = 0;
		ip->di.status.b.dfacl = 0;
		mutex_exit(&ip->fl_mutex);
	}

	/*
	 * Synthesize a dummy set of ACLs if none are present for this inode.
	 */
	if (!ip->di.status.b.acl && !ip->di.status.b.dfacl) {
		if (vsap->vsa_mask & (VSA_ACLCNT | VSA_ACL)) {
			/* USER/GROUP/OTHER/CLASS */
			vsap->vsa_aclcnt = aclcnt = 4;
			if (vsap->vsa_mask & VSA_ACL) {
				aclentp = kmem_zalloc((aclcnt *
				    sizeof (aclent_t)), KM_SLEEP);
				vsap->vsa_aclentp = aclentp;

				sam_synth_acl(ip, (sam_acl_t *)aclentp);
			}
		}
		goto out;
	}

	/*
	 * If no incore ACLs are present for the inode, get them from the
	 * the inode extension(s) on disk or inherit them from parent
	 * directory default ACLs.
	 */
	if ((aclp = ip->aclp) == NULL) {
		int got_inode_wlock = RW_WRITE_HELD(&ip->inode_rwl);

		if (!got_inode_wlock) {
			if (!rw_tryupgrade(&ip->inode_rwl)) {
				RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			}
		}
		if ((aclp = ip->aclp) == NULL) {
			ASSERT(!SAM_IS_SHARED_FS(ip->mp) ||
			    SAM_IS_SHARED_SERVER(ip->mp));
			if (error = sam_get_acl(ip, &aclp)) {
				errline = __LINE__ - 1;
				goto out;
			}
			ASSERT(aclp != NULL);
			if (ip->aclp != aclp) {
				sam_free_acl(ip);
			}
			ip->aclp = aclp;
		}
		if (!got_inode_wlock) {
			RW_DOWNGRADE_OS(&ip->inode_rwl);
		}
	}

	/*
	 * Copy incore regular ACL to vnode secattr structure.
	 */
	if (vsap->vsa_mask & VSA_ACL) {
		ASSERT(aclp->acl.owner);
		ASSERT(aclp->acl.group);
		ASSERT(aclp->acl.other);
	}

	aclcnt = 3;		/* USER_OBJ, GROUP_OBJ, and OTHER_OBJ */
	aclcnt += aclp->acl.nusers;
	aclcnt += aclp->acl.ngroups;
	aclcnt += aclp->acl.mask.is_def ? 1 : 0;
	if (vsap->vsa_mask & (VSA_ACLCNT | VSA_ACL)) {
		vsap->vsa_aclcnt = aclcnt;
		if (vsap->vsa_mask & VSA_ACL) {
			aclentp = kmem_zalloc((aclcnt *
			    sizeof (aclent_t)), KM_SLEEP);
			vsap->vsa_aclentp = entp = aclentp;

			ACL_VSEC(USER_OBJ, 1, aclp->acl.owner, entp);
			ACL_VSEC(USER, aclp->acl.nusers, aclp->acl.users,
			    entp);
			ACL_VSEC(GROUP_OBJ, 1, aclp->acl.group, entp);
			ACL_VSEC(GROUP, aclp->acl.ngroups, aclp->acl.groups,
			    entp);
			if (aclp->acl.mask.is_def) {
				ACL_VSEC_CLASS(CLASS_OBJ, aclp->acl.mask,
				    entp);
			}
			ACL_VSEC(OTHER_OBJ, 1, aclp->acl.other, entp);

			if (error = sam_check_acl(vsap->vsa_aclcnt,
			    vsap->vsa_aclentp,
			    ACL_CHECK)) {
				errline = __LINE__ - 1;
				goto out;
			}
		}
	}

	/*
	 * Copy incore default ACL to vnode secattr structure.
	 */
	dfaclcnt = aclp->dfacl.owner ? 1 : 0;
	dfaclcnt += aclp->dfacl.nusers;
	dfaclcnt += aclp->dfacl.group ? 1 : 0;
	dfaclcnt += aclp->dfacl.ngroups;
	dfaclcnt += aclp->dfacl.mask.is_def ? 1 : 0;
	dfaclcnt += aclp->dfacl.other ? 1 : 0;

	if (dfaclcnt > 0) {
		ASSERT(aclp->dfacl.owner);
		ASSERT(aclp->dfacl.group);
		ASSERT(aclp->dfacl.other);
		if (vsap->vsa_mask & (VSA_DFACLCNT | VSA_DFACL)) {
			vsap->vsa_dfaclcnt = dfaclcnt;
			if (vsap->vsa_mask & VSA_DFACL) {
				dfaclentp = kmem_zalloc((dfaclcnt *
				    sizeof (aclent_t)), KM_SLEEP);
				vsap->vsa_dfaclentp = entp = dfaclentp;

				ACL_VSEC(DEF_USER_OBJ, 1, aclp->dfacl.owner,
				    entp);
				ACL_VSEC(DEF_USER, aclp->dfacl.nusers,
				    aclp->dfacl.users, entp);
				ACL_VSEC(DEF_GROUP_OBJ, 1, aclp->dfacl.group,
				    entp);
				ACL_VSEC(DEF_GROUP, aclp->dfacl.ngroups,
				    aclp->dfacl.groups, entp);
				if (aclp->dfacl.mask.is_def) {
					ACL_VSEC_CLASS(DEF_CLASS_OBJ,
					    aclp->dfacl.mask, entp);
				}
				ACL_VSEC(DEF_OTHER_OBJ, 1, aclp->dfacl.other,
				    entp);

				if (error = sam_check_acl(vsap->vsa_dfaclcnt,
				    vsap->vsa_dfaclentp,
				    DEF_ACL_CHECK)) {
					errline = __LINE__ - 3;
					goto out;
				}
			}
		}
	}

out:
	if (error) {
		TRACE(T_SAM_CACL_ERR, SAM_ITOV(ip), (int)ip->di.id.ino,
		    errline, error);
		if (aclentp) {
			kmem_free(aclentp, (aclcnt * sizeof (aclent_t)));
		}
		if (dfaclentp) {
			kmem_free(dfaclentp, (dfaclcnt * sizeof (aclent_t)));
		}
		vsap->vsa_aclcnt = vsap->vsa_dfaclcnt = 0;
		vsap->vsa_aclentp = vsap->vsa_dfaclentp = NULL;
	}
	return (error);
}


/*
 * ----- sam_get_acl - Get access control list.
 *
 * Create incore access control list structure from ACL entries in
 * inode extension(s) or inherit from parent directory default ACLs,
 * if present.
 *
 * Caller must have the inode read or write lock held (and must not
 * hold the inode flag mutex).
 *
 * If the caller has the parent inode, it should pass the parent inode
 * in, and ensure that at least a read lock is held on the parent.
 */
int					/* ERRNO if error, 0 if successful */
sam_get_acl(
	sam_node_t *bip,		/* Pointer to base inode */
	sam_ic_acl_t **aclpp)		/* out: incore ACL structure */
{
	int error = 0, errline = 0;
	int got_inode_wlock;
	sam_ic_acl_t *aclp = NULL;

	ASSERT(RW_LOCK_HELD(&bip->inode_rwl));
	got_inode_wlock = RW_WRITE_HELD(&bip->inode_rwl);

	if (!bip->di.status.b.acl && !bip->di.status.b.dfacl) {
		cmn_err(CE_WARN, "sam_get_acl: File has no ACL (%d.%d)\n",
		    bip->di.id.ino, bip->di.id.gen);
		return (ENOCSI);
	}

	while ((aclp = bip->aclp) == NULL) { /* Incore ACL struct present */
		if (SAM_IS_SHARED_FS(bip->mp) &&
		    !SAM_IS_SHARED_SERVER(bip->mp)) {
			RW_UNLOCK_OS(&bip->inode_rwl, got_inode_wlock ?
			    RW_WRITER : RW_READER);
			error = sam_client_get_acl(bip, CRED());
			RW_LOCK_OS(&bip->inode_rwl, got_inode_wlock ?
			    RW_WRITER : RW_READER);
			if ((aclp = bip->aclp) == NULL) {
				TRACE(T_SAM_ACL_CLRETRY, SAM_ITOP(bip),
				    error, 0, 0);
				if (!error) {
					break;
				}
			}
			if (error &&
			    error != ENOTCONN && error != ETIMEDOUT &&
			    error != EPIPE) {
				break;
			}
		} else {
			/*
			 * Fetch ACLs from inode extension(s).
			 */
			if (error = sam_get_acl_ext(bip, &aclp)) {
				errline = __LINE__ - 1;
			}
			break;
		}
	}
	if (aclp &&
	    (aclp->id.ino != bip->di.id.ino ||
	    aclp->id.gen != bip->di.id.gen)) {
		errline = __LINE__ - 2;
		error = EINVAL;			/* Invalid base inode id */
	}

	if (error || !aclp) {
		TRACE(T_SAM_CACL_ERR,
		    SAM_ITOV(bip), (int)bip->di.id.ino, errline, error);
		if (aclp) {
			(void) sam_free_icacl(aclp);
			aclp = NULL;
		}
		if (!error) {
			error = ENOCSI;
		}
	}
	*aclpp = aclp;
	return (error);
}


/*
 * ----- sam_get_acl_ext - Get access control list from extension(s).
 *
 * Read access control list from inode extension(s) into allocated
 * incore ACL structure.
 *
 * Caller must have both the inode lock and flag mutex held.
 */
static int				/* ERRNO if error, 0 if successful */
sam_get_acl_ext(
	sam_node_t *bip,		/* Pointer to inode */
	sam_ic_acl_t **aclpp)		/* out: incore ACL structure */
{
	int error = 0, errline = 0, changed = 0;
	sam_ic_acl_t *aclp = NULL;
	int acl_size = 0;
	int synth_acl = 0;
	int cnt = 0, dfcnt = 0;
	sam_acl_t *entp = NULL;
	sam_acl_t *dfentp = NULL;

	/*
	 * Retrieve ACL size(s) from inode ext(s).
	 */
	if (error = sam_get_acl_ext_cnts(bip, &cnt, &dfcnt)) {
		errline = __LINE__ - 1;
		goto out;
	}

	if (cnt == 0) {
		cnt = 4;
		synth_acl = 1;		/* synthesize ACLs from base inode */
	}

	/*
	 * Allocate in-core ACL structure.
	 */
	acl_size = sizeof (sam_ic_acl_t) +
	    ((cnt + dfcnt - 1) * sizeof (sam_acl_t));
	aclp = kmem_zalloc(acl_size, KM_SLEEP);
	aclp->id = bip->di.id;			/* Base inode id */
	aclp->size = acl_size;			/* Incore ACL structure size */

	if (synth_acl) {
		sam_synth_acl(bip, &aclp->ent[0]);
	}
	aclp->acl.cnt = cnt;
	aclp->acl.entp = entp = &aclp->ent[0];
	if (dfcnt > 0) {
		aclp->dfacl.cnt = dfcnt;
		aclp->dfacl.entp = dfentp = &aclp->ent[cnt];
	}
	if (error = sam_get_acl_ext_data(bip, synth_acl ? 0 : cnt,
	    entp, dfcnt, dfentp)) {
		errline = __LINE__ - 1;
		goto out;
	}

	/*
	 * Validate ACLs, if present.
	 */
	if (cnt > 0) {
		if (error = sam_check_acl(cnt, entp, ACL_CHECK)) {
			errline = __LINE__ - 1;
			goto out;
		}
	}
	if (dfcnt > 0) {
		if (error = sam_check_acl(dfcnt, dfentp, DEF_ACL_CHECK)) {
			errline = __LINE__ - 1;
			goto out;
		}
	}

	/*
	 * Build regular and default ACL sections of incore ACL.
	 */
	if (error = sam_build_acl(aclp)) {
		errline = __LINE__ - 1;
		goto out;
	}

	/*
	 * Change the inode mode bits and ids to follow the ACL list.
	 */
	if (aclp->acl.owner) {				/* Owner */
		if (bip->di.uid != aclp->acl.owner->a_id) {
			changed = 1;
		}
		bip->di.mode &= ~0700;			/* Clear owner */
		bip->di.mode |= (aclp->acl.owner->a_perm & 07) << 6;
		bip->di.uid = aclp->acl.owner->a_id;
	}

	if (aclp->acl.group) {				/* Group */
		if (bip->di.gid != aclp->acl.group->a_id) {
			changed = 1;
		}
		bip->di.mode &= ~0070;			/* Clear group */
		bip->di.mode |= MASK2MODE(aclp);	/* Apply mask */
		bip->di.gid = aclp->acl.group->a_id;
	}

	if (aclp->acl.other) {				/* Other */
		if ((bip->di.mode & 07) != (aclp->acl.other->a_perm & 07)) {
			changed = 1;
		}
		bip->di.mode &= ~0007;			/* Clear other */
		bip->di.mode |= (aclp->acl.other->a_perm);
	}
	if (changed) {
		TRACE(T_SAM_ACL_CHMODE,
		    SAM_ITOV(bip),
		    (int)bip->di.id.ino, bip->di.mode, __LINE__);
		TRANS_INODE(bip->mp, bip);
		sam_mark_ino(bip, SAM_CHANGED);
	}

out:
	if (error) {
		TRACE(T_SAM_CACL_ERR,
		    SAM_ITOV(bip), (int)bip->di.id.ino, errline, error);
		if (aclp) {
			(void) sam_free_icacl(aclp);
		}
		aclp = NULL;
	}
	*aclpp = aclp;
	return (error);
}


/*
 * ----- sam_get_acl_ext_cnts - Retrieve access control list counts.
 *
 * Retrieve regular and/or default access control list counts from
 * inode extension(s).
 *
 * Caller must have the inode read lock held.  Lock is not released.
 */
static int			/* ERRNO if error, 0 if successful */
sam_get_acl_ext_cnts(
	sam_node_t *bip,	/* Pointer to base inode */
	int	*cntp,		/* Returned count of regular ACL entries */
	int *dfcntp)		/* Returned count of default ACL entries */
{
	int error = 0, errline = 0;
	buf_t *bp;
	sam_id_t eid;
	struct sam_inode_ext *eip;

	if ((cntp == NULL) || (dfcntp == NULL)) {
		errline = __LINE__ - 1;
		error = EINVAL;
	} else if ((bip->di.ext_attrs & ext_acl) == 0) {
		errline = __LINE__ - 1;
		error = ENOCSI;
		sam_req_ifsck(bip->mp, -1,
		    "sam_get_acl_ext_cnts: ext_acl = 0", &bip->di.id);
	} else {
		/*
		 * Find first ACL inode extension and copy counts of regular
		 * and default ACL entries to caller.
		 */
		*cntp = *dfcntp = 0;
		eid = bip->di.ext_id;
		while (eid.ino) {
			if (error = sam_read_ino(bip->mp, eid.ino, &bp,
					(struct sam_perm_inode **)&eip)) {
				errline = __LINE__ - 2;
				break;
			}
			if (EXT_HDR_ERR(eip, eid, bip)) {
				errline = __LINE__ - 1;
				error = ENOCSI;
				brelse(bp);
				sam_req_ifsck(bip->mp, -1,
				    "sam_get_acl_ext_cnts: EXT_HDR",
				    &bip->di.id);
				break;
			}
			if (S_ISACL(eip->hdr.mode)) {
				/*
				 * Return regular and default list totals.
				 */
				*cntp = eip->ext.acl.t_acls;
				*dfcntp = eip->ext.acl.t_dfacls;

				brelse(bp);
				break;
			} else {
				eid = eip->hdr.next_id;
				brelse(bp);
			}
		}
	}

	if (error) {
		TRACE(T_SAM_CACL_ERR,
		    SAM_ITOV(bip), (int)bip->di.id.ino, errline, error);
	}
	TRACE(T_SAM_GET_ACL_EXT, SAM_ITOV(bip), (int)bip->di.id.ino,
	    (*cntp + *dfcntp), error);
	return (error);
}


/*
 * ----- sam_get_acl_ext_data - Retrieve access control list from ext(s).
 *
 * Retrieve regular and/or default access control list entries from
 * inode extension(s).
 *
 * Caller must have the inode read lock held.  Lock is not released.
 */
static int			/* ERRNO if error, 0 if successful */
sam_get_acl_ext_data(
	sam_node_t *bip,	/* Pointer to base inode */
	int	cnt,		/* Count of regular ACL entries to return */
	sam_acl_t *entp,	/* Returned list of regular ACL entries */
	int dfcnt,		/* Count of default ACL entries to return */
	sam_acl_t *dfentp)	/* Returned list of default ACL entries */
{
	int error = 0, errline = 0;
	buf_t *bp;
	int done;
	sam_id_t eid;
	struct sam_inode_ext *eip;
	int i;
	int i_cnt = 0, i_dfcnt = 0;

	if ((cnt == 0) && (dfcnt == 0)) {
		errline = __LINE__ - 1;
		error = EINVAL;
		goto out;
	} else if ((cnt > 0) && (entp == NULL)) {
		errline = __LINE__ - 1;
		error = EINVAL;
		goto out;
	} else if ((dfcnt > 0) && (dfentp == NULL)) {
		errline = __LINE__ - 1;
		error = EINVAL;
		goto out;
	} else if ((bip->di.ext_attrs & ext_acl) == 0) {
		errline = __LINE__ - 1;
		error = ENOCSI;
		sam_req_ifsck(bip->mp, -1,
		    "sam_get_acl_ext_data: ext_acl = 0", &bip->di.id);
		goto out;
	}

	/*
	 * Find ACL inode extension(s) and copy regular and/or
	 * default ACL entries to caller.
	 */
	done = 0;
	eid = bip->di.ext_id;
	while (eid.ino && !done && !error) {
		if (error = sam_read_ino(bip->mp, eid.ino, &bp,
				(struct sam_perm_inode **)&eip)) {
			errline = __LINE__ - 2;
			break;
		}
		if (EXT_HDR_ERR(eip, eid, bip)) {
			errline = __LINE__ - 1;
			error = ENOCSI;
			brelse(bp);
			sam_req_ifsck(bip->mp, -1,
			    "sam_get_acl_ext_data: EXT_HDR", &bip->di.id);
			break;
		}
		if (S_ISACL(eip->hdr.mode)) {
			/*
			 * Copy all regular ACL entries before copying
			 * default ACLs.
			 */
			for (i = 0; i < MAX_ACL_ENTS_IN_INO; i++) {
				if (i_cnt < cnt) {
					if (eip->ext.acl.ent[i].a_type &
					    ACL_DEFAULT) {
						errline = __LINE__ - 1;
						error = ENOCSI;
						break;
					}
					entp[i_cnt] = eip->ext.acl.ent[i];
					i_cnt++;
				} else if (i_dfcnt < dfcnt) {
					ASSERT(eip->ext.acl.ent[i].a_type &
					    ACL_DEFAULT);
					if ((eip->ext.acl.ent[i].a_type &
					    ACL_DEFAULT) == 0) {
						errline = __LINE__ - 1;
						error = ENOCSI;
						break;
					}
					dfentp[i_dfcnt] = eip->ext.acl.ent[i];
					i_dfcnt++;
				} else {
					break;
				}
			}
			if (i_cnt == cnt && i_dfcnt == dfcnt) {
				done = 1;
			}
		}
		eid = eip->hdr.next_id;
		brelse(bp);
	}
	if (!done) {
		if (!error) {
			errline = __LINE__ - 1;
			error = ENOCSI;
		}
		sam_req_ifsck(bip->mp, -1,
		    "sam_get_acl_ext_data: done = 0", &bip->di.id);
	}

out:
	if (error) {
		TRACE(T_SAM_CACL_ERR,
		    SAM_ITOV(bip), (int)bip->di.id.ino, errline, error);
	}
	TRACE(T_SAM_GET_ACL_EXT, SAM_ITOV(bip), (int)bip->di.id.ino,
	    (cnt + dfcnt), error);
	return (error);
}


/*
 * ----- sam_check_acl - Check access control list.
 *
 * Check access control list for the following problems:
 * 1) Any duplicate/conflicting entries (same type and id)
 * 2) More than one of USER_OBJ, GROUP_OBJ, OTHER_OBJ, or CLASS_OBJ
 * 3) More than one of DEF_USER_OBJ, DEF_GROUP_OBJ, DEF_OTHER_OBJ,
 *    or DEF_CLASS_OBJ.
 */
int				/* ERRNO if error, 0 if successful */
sam_check_acl(
	int	cnt,		/* Number of ACL entries */
	sam_acl_t *entp,	/* Pointer to list of ACL entries */
	int flag)		/* ACL_CHECK (reg) or DEF_ACL_CHECK (def) */
{
	int i;
	int user_objs = 0;
	int users = 0;
	int group_objs = 0;
	int groups = 0;
	int other_objs = 0;
	int class_objs = 0;
	int duser_objs = 0;
	int dusers = 0;
	int dgroup_objs = 0;
	int dgroups = 0;
	int dother_objs = 0;
	int dclass_objs = 0;
	int ndefs = 0;

	if (cnt <= 0 || entp == NULL) {
		TRACE(T_SAM_CACL_ERR, NULL, cnt, __LINE__, EINVAL);
		return (EINVAL);
	}

	if (flag != ACL_CHECK && flag != DEF_ACL_CHECK) {
		return (EINVAL);
	}

	/*
	 * Sort the ACLs passed in.
	 */
	ksort((caddr_t)entp, cnt, sizeof (sam_acl_t), cmp2acls);

	/*
	 * Check entries for duplicates and bad permission values.
	 */
	for (i = 1; i < cnt; i++) {
		if (entp[i-1].a_perm > 07) {
			TRACE(T_SAM_CACL_ERR, NULL, i, __LINE__, EINVAL);
			return (EINVAL);
		}
		if (entp[i-1].a_type == entp[i].a_type &&
		    entp[i-1].a_id == entp[i].a_id) {
			TRACE(T_SAM_CACL_ERR, NULL, i, __LINE__, EINVAL);
			return (EINVAL);
		}
	}
	if (entp[cnt-1].a_perm > 07) {
		TRACE(T_SAM_CACL_ERR, NULL, 0, __LINE__, EINVAL);
		return (EINVAL);
	}

	for (i = 0; i < cnt; i++) {		/* Count types */
		switch (entp[i].a_type) {
		case USER_OBJ:				/* Owner */
			user_objs++;
			break;
		case USER:				/* Users */
			users++;
			break;
		case GROUP_OBJ:				/* Group */
			group_objs++;
			break;
		case GROUP:				/* Groups */
			groups++;
			break;
		case OTHER_OBJ:				/* Other */
			other_objs++;
			break;
		case CLASS_OBJ:				/* Mask */
			class_objs++;
			break;
		case DEF_USER_OBJ:			/* Default owner */
			duser_objs++;
			break;
		case DEF_USER:				/* Default users */
			dusers++;
			break;
		case DEF_GROUP_OBJ:			/* Default group */
			dgroup_objs++;
			break;
		case DEF_GROUP:				/* Default groups */
			dgroups++;
			break;
		case DEF_OTHER_OBJ:			/* Default other */
			dother_objs++;
			break;
		case DEF_CLASS_OBJ:			/* Default mask */
			dclass_objs++;
			break;
		default:				/* Unknown type */
			TRACE(T_SAM_CACL_ERR, NULL, 0, __LINE__, EINVAL);
			return (EINVAL);
		}
	}

	/*
	 * For regular ACLs, it is required that there be exactly
	 * one USER_OBJ, GROUP_OBJ and OTHER_OBJ.
	 * Zero or one CLASS_OBJ is allowable.
	 */
	if (flag & ACL_CHECK) {
		if (user_objs != 1 || group_objs != 1 ||
		    other_objs != 1 || class_objs > 1) {
			return (EINVAL);
		}

		/*
		 * If there are any group ACLs, there must be a class
		 * object (mask).
		 */
		if (groups && !class_objs) {
			return (EINVAL);
		}
		if (user_objs + group_objs + other_objs + class_objs +
		    users + groups > MAX_ACL_ENTRIES) {
			return (EINVAL);
		}
	}

	/*
	 * For default ACLs, it is required that there be exactly one
	 * DEF_USER_OBJ, DEF_GROUP_OBJ and DEF_OTHER_OBJ, or there be
	 * none of them.
	 */
	if (flag & DEF_ACL_CHECK) {
		if (duser_objs != 1 || dgroup_objs != 1 ||
		    dother_objs != 1 || dclass_objs > 1) {
			return (EINVAL);
		}

		ndefs = duser_objs + dgroup_objs + dother_objs;
		if (ndefs != 0 && ndefs != 3) {
			return (EINVAL);
		}

		/*
		 * If there are ANY default group ACLs, there must be a
		 * default class object (mask).
		 */
		if (dgroups != 0 && !dclass_objs) {
			return (EINVAL);
		}
		if (ndefs != 3 && !dclass_objs &&
		    (dusers != 0 || dgroups != 0)) {
			return (EINVAL);
		}

		if (duser_objs + dgroup_objs + dother_objs + dclass_objs +
		    dusers + dgroups > MAX_ACL_ENTRIES) {
			return (EINVAL);
		}
	}
	return (0);
}


/*
 * ----- sam_acl_inactive - Inactivate access control list.
 *
 * Save incore access control list to inode extension(s) and
 * free incore structure.
 *
 * Caller must have the inode write lock held.  Lock is not released.
 */
int					/* ERRNO if error, 0 if successful */
sam_acl_inactive(sam_node_t *ip)
{
	int error = 0;

	ASSERT(RW_WRITE_HELD(&ip->inode_rwl));

	sam_acl_flush(ip);
	if (ip->aclp) {
		(void) sam_free_acl(ip);
	}

	return (error);
}


/*
 * ----- sam_build_acl - Build incore access control list.
 *
 * Construct incore inode struct for access control list entries
 * from supplied regular and default lists.
 */
int					/* ERRNO if error, 0 if successful */
sam_build_acl(sam_ic_acl_t *aclp)
{
	int i, error = 0;
	int cnt, dfcnt;
	int ccnt = 0, cdfcnt = 0;		/* check counts */
	sam_acl_t *entp;
	sam_acl_t *dfentp;

	if (aclp == NULL) {
		return (EINVAL);
	}

	/*
	 * Construct regular ACL section of incore ACL structure.
	 */
	cnt = aclp->acl.cnt;
	entp = aclp->acl.entp;

	for (i = 0; i < cnt; i++) {
		switch (entp[i].a_type) {
		case USER_OBJ:
			if (aclp->acl.owner != NULL) {
				error = ENOCSI;
				break;
			}
			aclp->acl.owner = &entp[i];
			ccnt++;
			break;
		case USER:
			if (aclp->acl.users == NULL) {
				aclp->acl.users = &entp[i];
			}
			aclp->acl.nusers++;
			ccnt++;
			break;
		case GROUP_OBJ:
			if (aclp->acl.group != NULL) {
				error = ENOCSI;
				break;
			}
			aclp->acl.group = &entp[i];
			ccnt++;
			break;
		case GROUP:
			if (aclp->acl.groups == NULL) {
				aclp->acl.groups = &entp[i];
			}
			aclp->acl.ngroups++;
			ccnt++;
			break;
		case CLASS_OBJ:
			aclp->acl.mask.is_def = 1;
			aclp->acl.mask.bits = entp[i].a_perm;
			ccnt++;
			break;
		case OTHER_OBJ:
			if (aclp->acl.other != NULL) {
				error = ENOCSI;
				break;
			}
			aclp->acl.other = &entp[i];
			ccnt++;
			break;
		default:
			error = ENOCSI;
		}
		if (error) {
			TRACE(T_SAM_CACL_ERR, NULL, i, __LINE__, EINVAL);
		}
	}
	if (cnt > 0) {
		if (aclp->acl.owner == NULL) {
			TRACE(T_SAM_CACL_ERR, NULL, 0, __LINE__, EINVAL);
			error = EINVAL;
		}
		if (aclp->acl.group == NULL) {
			TRACE(T_SAM_CACL_ERR, NULL, 1, __LINE__, EINVAL);
			error = EINVAL;
		}
		if (aclp->acl.other == NULL) {
			TRACE(T_SAM_CACL_ERR, NULL, 2, __LINE__, EINVAL);
			error = EINVAL;
		}
	}

	/*
	 * Construct default ACL section of incore ACL structure.
	 */
	dfcnt = aclp->dfacl.cnt;
	dfentp = aclp->dfacl.entp;

	for (i = 0; i < dfcnt; i++) {
		switch (dfentp[i].a_type) {
		case DEF_USER_OBJ:
			if (aclp->dfacl.owner != NULL) {
				error = ENOCSI;
				break;
			}
			aclp->dfacl.owner = &dfentp[i];
			cdfcnt++;
			break;
		case DEF_USER:
			if (aclp->dfacl.users == NULL) {
				aclp->dfacl.users = &dfentp[i];
			}
			aclp->dfacl.nusers++;
			cdfcnt++;
			break;
		case DEF_GROUP_OBJ:
			if (aclp->dfacl.group != NULL) {
				error = ENOCSI;
				break;
			}
			aclp->dfacl.group = &dfentp[i];
			cdfcnt++;
			break;
		case DEF_GROUP:
			if (aclp->dfacl.groups == NULL) {
				aclp->dfacl.groups = &dfentp[i];
			}
			aclp->dfacl.ngroups++;
			cdfcnt++;
			break;
		case DEF_CLASS_OBJ:
			aclp->dfacl.mask.is_def = 1;
			aclp->dfacl.mask.bits = dfentp[i].a_perm;
			cdfcnt++;
			break;
		case DEF_OTHER_OBJ:
			if (aclp->dfacl.other != NULL) {
				error = ENOCSI;
				break;
			}
			aclp->dfacl.other = &dfentp[i];
			cdfcnt++;
			break;
		default:
			error = ENOCSI;
		}
	}

	if (dfcnt > 0) {
		if (aclp->dfacl.owner == NULL) {
			TRACE(T_SAM_CACL_ERR, NULL, 10, __LINE__, EINVAL);
			error = EINVAL;
		}
		if (aclp->dfacl.group == NULL) {
			TRACE(T_SAM_CACL_ERR, NULL, 11, __LINE__, EINVAL);
			error = EINVAL;
		}
		if (aclp->dfacl.other == NULL) {
			TRACE(T_SAM_CACL_ERR, NULL, 12, __LINE__, EINVAL);
			error = EINVAL;
		}
	}

	if (cnt != ccnt) {
		TRACE(T_SAM_CACL_ERR, NULL, 20, cnt, ccnt);
		error = EINVAL;
	}
	if (dfcnt != cdfcnt) {
		TRACE(T_SAM_CACL_ERR, NULL, 21, dfcnt, cdfcnt);
		error = EINVAL;
	}

	return (error);
}


int
sam_reset_client_acl(sam_node_t *ip, int nacl, int ndfacl, void *aclents,
    int acllen)
{
	int error;
	sam_acl_t *acls = (sam_acl_t *)aclents;
	sam_ic_acl_t *aclp;
	int acl_size;

	if (nacl < 0 || nacl > MAX_ACL_ENTRIES) {
		cmn_err(CE_WARN, "reset_client_acl: nacl=%d, ndfacl=%d",
		    nacl, ndfacl);
		return (ENOCSI);
	}

	if (ndfacl < 0 || ndfacl > MAX_ACL_ENTRIES) {
		cmn_err(CE_WARN, "reset_client_acl: nacl=%d, ndfacl=%d",
		    nacl, ndfacl);
		return (ENOCSI);
	}

	if (acllen < (nacl + ndfacl) * sizeof (sam_acl_t)) {
		cmn_err(CE_WARN, "reset_client_acl: nacl=%d, ndfacl=%d, "
		    "len=%d, act=%d",
		    nacl, ndfacl, acllen,
		    (nacl + ndfacl) * sizeof (sam_acl_t));
		return (ENOCSI);
	}

	ASSERT(RW_WRITE_HELD(&ip->inode_rwl));
	ASSERT(nacl >= 0 && nacl <= MAX_ACL_ENTRIES);
	ASSERT(ndfacl >= 0 && ndfacl <= MAX_ACL_ENTRIES);

	acl_size = sizeof (sam_ic_acl_t) + (nacl + ndfacl - 1) *
	    sizeof (sam_acl_t);
	aclp = (sam_ic_acl_t *)kmem_zalloc(acl_size, KM_SLEEP);
	aclp->id = ip->di.id;
	aclp->size = acl_size;
	aclp->acl.cnt = nacl;
	aclp->dfacl.cnt = ndfacl;
	aclp->acl.entp = &aclp->ent[0];
	aclp->dfacl.entp = &aclp->ent[nacl];
	bcopy(&acls[0], aclp->acl.entp, nacl * sizeof (sam_acl_t));
	bcopy(&acls[nacl], aclp->dfacl.entp, ndfacl * sizeof (sam_acl_t));
	error = sam_build_acl(aclp);
	if (ip->aclp != aclp) {
		sam_free_acl(ip);
	}
	ip->aclp = aclp;

	return (error);
}


/*
 * Free an in-core ACL entry.
 */
void
sam_free_icacl(sam_ic_acl_t *aclp)
{
	int acl_size;

	acl_size = sizeof (sam_ic_acl_t) +
	    ((aclp->acl.cnt + aclp->dfacl.cnt - 1) * sizeof (sam_acl_t));
	ASSERT(acl_size == aclp->size);
	kmem_free(aclp, aclp->size);
}


/*
 * ----- sam_free_acl - Free access control list.
 *
 * Free incore access control list structure.
 *
 * Caller must have the inode lock held.  If write lock isn't held,
 * it's acquired, and then downgraded back to a reader lock.
 */
void
sam_free_acl(sam_node_t *ip)
{
	int got_inode_wlock;

	ASSERT(RW_LOCK_HELD(&ip->inode_rwl));
	got_inode_wlock = RW_WRITE_HELD(&ip->inode_rwl);

	if (!got_inode_wlock) {
		if (!rw_tryupgrade(&ip->inode_rwl)) {
			RW_UNLOCK_OS(&ip->inode_rwl, RW_READER);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		}
	}
	/*
	 * Release incore ACL structure.
	 */
	if (ip->aclp) {
		sam_free_icacl(ip->aclp);
		ip->aclp = NULL;
	}

	if (!got_inode_wlock) {
		RW_DOWNGRADE_OS(&ip->inode_rwl);
	}
}
