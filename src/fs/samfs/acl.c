/*
 * ----- sam/acl.c - Process the inode access control list set functions.
 *
 *	Processes the SAMFS inode access control list set functions.
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

#pragma ident "$Revision: 1.63 $"

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
#include <sys/sysmacros.h>

/* ----- SAMFS Includes */

#include "inode.h"
#include "mount.h"
#include "extern.h"
#include "debug.h"
#include "trace.h"
#include "ino_ext.h"
#include "qfs_log.h"

static int sam_set_acl_ext(sam_node_t *bip, int	cnt, sam_acl_t *entp,
	int dfcnt, sam_acl_t *dfentp);

void sam_synth_acl(sam_node_t *ip, sam_acl_t *entp);


/*
 * ----- sam_acl_create - Create access control list.
 *
 * Create access control list from default access control list
 * of parent inode.  Calls sam_get_acl to do the work.
 *
 * Caller must have the inode write lock held.  Lock is not released.
 */
int				/* ERRNO if error, 0 if successful */
sam_acl_create(
	sam_node_t *ip,		/* Pointer to inode */
	sam_node_t *pip)	/* Pointer to parent inode */
{
	int error = 0;

	ASSERT(RW_WRITE_HELD(&ip->inode_rwl));

	if (pip->di.status.b.dfacl) {
		ip->di.status.b.acl = 1;
		if (S_ISDIR(pip->di.mode)) {
			ip->di.status.b.dfacl = 1;
		}
	}
	if (!ip->di.status.b.acl && !ip->di.status.b.dfacl) {
		return (0);
	}

	/*
	 * If no incore ACLs are present for the inode, get them from
	 * the inode extension(s) on disk or inherit them from parent
	 * directory default ACLs.
	 */
	if (ip->aclp == NULL) {
		if ((error = sam_acl_inherit(ip, pip, &ip->aclp)) == 0) {
			ASSERT(ip->aclp != 0);
			error = sam_set_acl(ip, &ip->aclp);
		}
	}

	return (error);
}


/*
 * ----- sam_acl_inherit - Inherit access control list.
 *
 * Copy default parent directory access control list to allocated
 * incore ACL structure.
 *
 * Caller must have the inode write lock held.  Lock is not released.
 *
 * If the caller has the parent inode, it should pass the parent inode
 * in, and ensure that at least a read lock is held on the parent.
 */
int				/* ERRNO if error, 0 if successful */
sam_acl_inherit(
	sam_node_t *bip,	/* Pointer to inode */
	sam_node_t *pip,	/* Pointer to parent inode, or NULL */
	sam_ic_acl_t **aclpp)	/* Returned pointer to incore ACL structure */
{
	int error = 0, errline = 0;
	sam_ic_acl_t *paclp;
	sam_ic_acl_t *aclp = NULL;
	int bits = 0;
	int i;
	int acl_size = 0;
	int cnt = 0, dfcnt = 0;
	sam_acl_t *entp = NULL;
	sam_acl_t *dfentp = NULL;

	/*
	 * Insure we're not already at the root directory.
	 */
	if (bip->di.id.ino == SAM_ROOT_INO) {
		errline = __LINE__ - 1;
		error = EIO;
		cmn_err(CE_WARN,
		    "SAM-FS: %s: Root directory default ACL missing",
		    bip->mp->mt.fi_name);
		goto out;
	}

	/*
	 * Make sure the parent directory inode has default ACLs to inherit.
	 */
	if (pip->di.status.b.dfacl == 0) {
		errline = __LINE__ - 1;
		error = EIO;
		cmn_err(CE_WARN,
		    "SAM-FS: %s: Directory default ACL missing, ino=%d.%d",
		    pip->mp->mt.fi_name, pip->di.id.ino, pip->di.id.gen);
		goto out;
	}

	/*
	 * If no incore ACLs are present for the parent inode, get them
	 * from the inode extension(s) on disk.
	 */
	if ((paclp = pip->aclp) == NULL) {
		int got_inode_wlock = RW_WRITE_HELD(&pip->inode_rwl);

		ASSERT(RW_LOCK_HELD(&pip->inode_rwl));

		if (!got_inode_wlock) {
			if (!rw_tryupgrade(&pip->inode_rwl)) {
				RW_UNLOCK_OS(&pip->inode_rwl, RW_READER);
				RW_LOCK_OS(&pip->inode_rwl, RW_WRITER);
			}
		}
		if ((paclp = pip->aclp) == NULL) {
			ASSERT(pip->di.ext_attrs & ext_acl);
			if (error = sam_get_acl(pip, &paclp)) {
				errline = __LINE__ - 1;
				goto out;
			}
			ASSERT(paclp != NULL);
			if (pip->aclp != paclp) {
				sam_free_acl(pip);
			}
			pip->aclp = paclp;
		}
		if (!got_inode_wlock) {
			RW_DOWNGRADE_OS(&pip->inode_rwl);
		}
	}

	/*
	 * Mask bits set must be user/group/other, or none.
	 */
	bits = 0;
	if (paclp->dfacl.owner)  bits++;
	if (paclp->dfacl.group)  bits++;
	if (paclp->dfacl.other)  bits++;

	if (bits == 0) {
		goto out;
	}
	if (bits != 3) {
		errline = __LINE__ - 1;
		error = EINVAL;
		goto out;
	}

	ASSERT(bip->aclp == NULL);

	/*
	 * Allocate incore ACL structure.
	 */
	acl_size = sizeof (sam_ic_acl_t) +
	    ((paclp->dfacl.cnt - 1) * sizeof (sam_acl_t));
	if (S_ISDIR(bip->di.mode)) {
		acl_size += (paclp->dfacl.cnt * sizeof (sam_acl_t));
	}
	aclp = kmem_zalloc(acl_size, KM_SLEEP);
	aclp->id = bip->di.id;			/* Base inode id */
	aclp->size = acl_size;			/* Incore ACL structure size */

	/*
	 * Copy parent default ACL to the inheriting inode as a
	 * regular ACL.
	 */
	if (paclp->dfacl.cnt > 0) {
		aclp->acl.cnt = cnt = paclp->dfacl.cnt;
		aclp->acl.entp = entp = &aclp->ent[0];
		bcopy((void *)paclp->dfacl.entp, (void *)entp,
		    (cnt * sizeof (sam_acl_t)));

		/* Change DEF_ default type values to regular type values. */
		for (i = 0; i < cnt; i++) {
			entp[i].a_type &= ~ACL_DEFAULT;
		}

		/* Validate regular ACL. */
		if (error = sam_check_acl(cnt, entp, ACL_CHECK)) {
			errline = __LINE__ - 1;
			goto out;
		}

		if (S_ISDIR(bip->di.mode)) {	/* Dirs inherit defaults too */
			/*
			 * Copy parent default ACL as defaults for
			 * inheriting dir inodes.
			 */
			aclp->dfacl.cnt = dfcnt = paclp->dfacl.cnt;
			aclp->dfacl.entp = dfentp = &aclp->ent[cnt];
			bcopy((void *)paclp->dfacl.entp, (void *)dfentp,
			    (dfcnt * sizeof (sam_acl_t)));

			/* Validate default ACL. */
			if (error = sam_check_acl(dfcnt, dfentp,
			    DEF_ACL_CHECK)) {
				errline = __LINE__ - 1;
				goto out;
			}
		}
	}

	/*
	 * Create regular and default sections of incore ACL structure.
	 */
	if (error = sam_build_acl(aclp)) {
		errline = __LINE__ - 1;
		goto out;
	}

	/*
	 * Set the owner/group/other values for the inheriting inode.
	 */
	aclp->acl.owner->a_id = bip->di.uid;
	aclp->acl.group->a_id = bip->di.gid;
	aclp->acl.other->a_id = 0;

	/* Do not change permissions on symlinks */
	if (!S_ISLNK(bip->di.mode)) {
		aclp->acl.owner->a_perm &= ((bip->di.mode >> 6) & 7);
		aclp->acl.group->a_perm &= ((bip->di.mode >> 3) & 7);
		aclp->acl.other->a_perm &= (bip->di.mode & 7);
		if (aclp->acl.mask.is_def) {
			aclp->acl.mask.bits &= (bip->di.mode >> 3);
		}
	} else {
		aclp->acl.owner->a_perm = ((bip->di.mode >> 6) & 7);
		aclp->acl.group->a_perm = ((bip->di.mode >> 3) & 7);
		aclp->acl.other->a_perm = (bip->di.mode & 7);
		aclp->acl.mask.bits = (bip->di.mode >> 3);
	}
	aclp->flags |= ACL_MODIFIED;

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
 * ----- sam_acl_setattr - Access control list set attributes.
 *
 * Set vnode attributes under influence of access control list info.
 *
 * Caller must have the inode write lock held.  Lock is not released.
 */
int				/* ERRNO if error, 0 if successful */
sam_acl_setattr(
	sam_node_t *ip,		/* Pointer to inode */
	struct vattr *vap)	/* Vnode attributes */
{
	int error = 0;
	int mask;
	sam_ic_acl_t *aclp;
	int val;

	ASSERT(RW_WRITE_HELD(&ip->inode_rwl));

	/*
	 * Return if no ACL has been specified for the inode.
	 */
	if (!ip->di.status.b.acl) {
		goto out;
	}

	/*
	 * Check mask for values affected by ACL entries.
	 */
	mask = vap->va_mask;
	if ((mask & (AT_MODE|AT_UID|AT_GID)) && !S_ISLNK(ip->di.mode)) {

		/*
		 * If no incore ACLs are present for the inode, get them from
		 * the inode extension(s) on disk or inherit them from parent
		 * directory default ACLs.
		 */
		aclp = ip->aclp;
		if (aclp == NULL) {
			mode_t save_mode;
			uid_t  save_uid;
			gid_t  save_gid;

			/*
			 * Save and restore fields that will get clobbered by
			 * an initial load of the ACL info.  These fields may
			 * have been given new values by the calling routine.
			 */
			if (mask & AT_MODE) save_mode = ip->di.mode;
			if (mask & AT_UID)  save_uid  = ip->di.uid;
			if (mask & AT_GID)  save_gid  = ip->di.gid;

			error = sam_get_acl(ip, &aclp);

			if (mask & AT_MODE) ip->di.mode = save_mode;
			if (mask & AT_UID)  ip->di.uid  = save_uid;
			if (mask & AT_GID)  ip->di.gid  = save_gid;

			if (error) {
				goto out;
			}
			ASSERT(aclp != NULL);
			if (ip->aclp != aclp) {
				sam_free_acl(ip);
			}
			ip->aclp = aclp;
		}

		/*
		 * Set the mask to the group permissions, if a mask entry
		 * exists.  This enables chmod to actually change group
		 * permissions.  It may indirectly change some other groups
		 * and users permissions because the mask is changing.
		 */
		if (mask & AT_MODE) {
			val = (ushort_t)((ip->di.mode & 0700) >> 6);
			if (val != aclp->acl.owner->a_perm) {
				aclp->acl.owner->a_perm = (ushort_t)val;
				aclp->flags |= ACL_MODIFIED;
			}
			val = (ushort_t)((ip->di.mode & 070) >> 3);
			if (val != aclp->acl.group->a_perm) {
				aclp->acl.group->a_perm = (ushort_t)val;
				aclp->flags |= ACL_MODIFIED;
			}
			if (aclp->acl.mask.is_def) {
				val = (ip->di.mode & 070) >> 3;
				if (val != aclp->acl.mask.bits) {
					aclp->acl.mask.bits = val;
					aclp->flags |= ACL_MODIFIED;
				}
			}
			val = (ushort_t)(ip->di.mode & 07);
			if (val != aclp->acl.other->a_perm) {
				aclp->acl.other->a_perm = (ushort_t)val;
				aclp->flags |= ACL_MODIFIED;
			}
		}
		if (mask & AT_UID) { /* Shouldn't allow chown if not suser */
			val = ip->di.uid;
			if (val != aclp->acl.owner->a_id) {
				aclp->acl.owner->a_id = val;
				aclp->flags |= ACL_MODIFIED;
			}
		}
		if (mask & AT_GID) {
			val = ip->di.gid;
			if (val != aclp->acl.group->a_id) {
				aclp->acl.group->a_id = val;
				aclp->flags |= ACL_MODIFIED;
			}
		}
	}

out:
	return (error);
}


/*
 * ----- sam_acl_flush - Flush modified access control list to disk.
 *
 * Sync modified incore access control list to inode extension(s).
 *
 * Caller must have the inode write lock held.  Lock is not released.
 */
int				/* ERRNO if error, 0 if successful */
sam_acl_flush(sam_node_t *ip)
{
	int error = 0;

	ASSERT(RW_WRITE_HELD(&ip->inode_rwl));

	if (ip->aclp && (ip->aclp->flags & ACL_MODIFIED)) {
		if (error = sam_set_acl(ip, &ip->aclp)) {
			TRACE(T_SAM_ACL_ERR, SAM_ITOV(ip),
			    ip->di.id.ino, __LINE__, error);
		}
	}

	return (error);
}


/*
 * ----- sam_acl_set_vsecattr - Save vnode secattrs structure.
 *
 * Save vnode secattrs as access control list in inode extension(s).
 *
 * Caller must have the inode write lock held.  Lock is not released.
 */
int				/* ERRNO if error, 0 if successful */
sam_acl_set_vsecattr(
	sam_node_t *ip,		/* Pointer to inode */
	vsecattr_t *vsap)	/* Pointer to vnode secattr struct */
{
	int error = 0, errline = 0;
	sam_ic_acl_t *aclp = NULL;
	int acl_size = 0;
	int cnt = 0, dfcnt = 0;
	sam_acl_t *entp = NULL;
	sam_acl_t *dfentp = NULL;

	ASSERT(RW_WRITE_HELD(&ip->inode_rwl));

	if (!CHECK_ACL_ALLOWED(ip->di.mode & S_IFMT)) {
		errline = __LINE__ - 1;
		error = ENOSYS;
		goto out;
	}

	/*
	 * Check if file system supports ACLs.
	 */
	if ((ip->di.version != SAM_INODE_VERSION) &&
	    (ip->di.version != SAM_INODE_VERS_2)) {
		errline = __LINE__ - 1;
		error = ENOSYS;
		goto out;
	}
	cnt = vsap->vsa_aclcnt;
	dfcnt = vsap->vsa_dfaclcnt;

	if (vsap->vsa_aclcnt == 0) {
		cnt = 4;
	}
	/*
	 * Allocate new incore ACL structure.
	 */
	acl_size = sizeof (sam_ic_acl_t) + (cnt + dfcnt - 1) *
	    sizeof (sam_acl_t);
	aclp = kmem_zalloc(acl_size, KM_SLEEP);
	aclp->id = ip->di.id;		/* Base inode id */
	aclp->flags |= ACL_MODIFIED;	/* New incore needs update */
	aclp->size = acl_size;		/* New incore ACL structure size */

	/*
	 * Create new incore ACL structure from vnode secattrs.
	 */
	if (vsap->vsa_aclcnt > 0) {
		aclp->acl.cnt = cnt;
		aclp->acl.entp = entp = &aclp->ent[0];

		/*
		 * XXX This here bcopy copies aclent_ts into sam_acl_ts.
		 * XXX The two are conformant, but have holes.  Some care
		 * XXX &/| conversion should be noted and/or provided for.
		 * XXX Ditto dfacls below.
		 */
		/* Copy and validate regular ACL. */
		bcopy((void *)vsap->vsa_aclentp, (void *)entp,
		    (cnt * sizeof (sam_acl_t)));
		if (error = sam_check_acl(cnt, entp, ACL_CHECK)) {
			errline = __LINE__ - 1;
			goto out;
		}
	} else {
		aclp->acl.entp = entp = &aclp->ent[0];
		sam_synth_acl(ip, entp);
		aclp->acl.cnt = cnt;
	}
	if (dfcnt > 0) {
		aclp->dfacl.cnt = dfcnt;
		aclp->dfacl.entp = dfentp = &aclp->ent[cnt];

		/* Copy and validate default ACL. */
		bcopy((void *)vsap->vsa_dfaclentp, (void *)dfentp,
		    (dfcnt * sizeof (sam_acl_t)));
		if (error = sam_check_acl(dfcnt, dfentp, DEF_ACL_CHECK)) {
			errline = __LINE__ - 1;
			goto out;
		}
	}

	/*
	 * Create regular and default sections of new incore ACL structure.
	 */
	if (error = sam_build_acl(aclp)) {
		errline = __LINE__ - 1;
		goto out;
	}

	/*
	 * Set user/group objs in the ACL to owning ids from the inode.
	 */
	if (aclp->acl.owner) {
		aclp->acl.owner->a_id = ip->di.uid;
	}
	if (aclp->acl.group) {
		aclp->acl.group->a_id = ip->di.gid;
	}

	/*
	 * Save new incore ACL to inode extension(s).
	 */
	if (error = sam_set_acl(ip, &aclp)) {
		errline = __LINE__ - 1;
		goto out;
	}

	/*
	 * Release any previous instance of incore ACL structure.
	 */
	if (ip->aclp) {
		(void) sam_free_acl(ip);
	}
	ip->aclp = aclp;

	TRANS_INODE(ip->mp, ip);
	sam_mark_ino(ip, SAM_CHANGED);

out:
	if (error) {
		TRACE(T_SAM_ACL_ERR, SAM_ITOV(ip),
		    ip->di.id.ino, errline, error);
		if (aclp) {
			(void) sam_free_icacl(aclp);
		}
	}
	return (error);
}


/*
 * ----- sam_set_acl - Save access control list.
 *
 * Save incore access control list structure as inode extension(s).
 *
 * Caller must have the inode write lock held, or the read lock and
 * flag mutex.  Lock is not released.
 */
int				/* ERRNO if error, 0 if successful */
sam_set_acl(
	sam_node_t *bip,	/* Pointer to base inode */
	sam_ic_acl_t **aclpp)	/* incore ACL structure (updated). */
{
	int error = 0, errline = 0;
	sam_ic_acl_t *aclp;

	ASSERT(RW_WRITE_HELD(&bip->inode_rwl));

	aclp = *aclpp;
	if ((aclp == NULL) ||	/* Missing or invalid incore ACL struct */
	    (aclp->id.ino != bip->di.id.ino) ||
	    (aclp->id.gen != bip->di.id.gen)) {
		errline = __LINE__ - 3;
		error = EINVAL;
		goto out;
	}

	if (aclp->flags & ACL_MODIFIED) {
		/*
		 * Don't bother saving ACLs to inode extension(s) if only
		 * owner/group/other entries are present.
		 */
		if ((aclp->acl.owner) &&
		    (aclp->acl.group) &&
		    (aclp->acl.other) &&
		    (aclp->acl.nusers == 0) &&
		    (aclp->acl.ngroups == 0) &&
		    (aclp->dfacl.owner == NULL) &&
		    (aclp->dfacl.group == NULL) &&
		    (aclp->dfacl.other == NULL) &&
		    (aclp->dfacl.mask.is_def == 0) &&
		    (aclp->dfacl.nusers == 0) &&
		    (aclp->dfacl.ngroups == 0)) {

			aclp->flags &= ~ACL_MODIFIED;	/* Incore updated */

			/* Apply ACL permission values to inode mode. */
			bip->di.mode = ((bip->di.mode & ~0777) |
			    ((aclp->acl.owner->a_perm & 07) << 6) |
			    MASK2MODE(aclp) |
			    (aclp->acl.other->a_perm & 07));

			/* Release any previous ACL inode ext(s). */
			if (bip->di.ext_attrs & ext_acl) {
				sam_free_inode_ext(bip, S_IFACL,
				    SAM_ALL_COPIES,
				    &bip->di.ext_id);
				bip->di.ext_attrs &= ~ext_acl;
			}

			sam_free_acl(bip);

			/* Clear inode flags that indicate ACLs set */
			bip->di.status.b.acl = 0;
			bip->di.status.b.dfacl = 0;

			TRANS_INODE(bip->mp, bip);
			sam_mark_ino(bip, SAM_CHANGED);

		} else {

			/* Save incore ACL structure to inode ext(s). */
			if (error = sam_set_acl_ext(bip, aclp->acl.cnt,
			    aclp->acl.entp,
			    aclp->dfacl.cnt,
			    aclp->dfacl.entp)) {
				errline = __LINE__ - 4;
				goto out;
			}

			bip->di.status.b.acl = 1;
			bip->di.status.b.dfacl = (aclp->dfacl.cnt != 0);

			aclp->flags &= ~ACL_MODIFIED;	/* Incore updated */

			/* Change the mode bits to follow the acl list */
			if (aclp->acl.owner) {		/* Owner */
				bip->di.mode &= ~0700;	/* Clear owner */
				bip->di.mode |=
				    (aclp->acl.owner->a_perm & 07) << 6;
				bip->di.uid = aclp->acl.owner->a_id;
			}

			if (aclp->acl.group) {		/* Group */
				bip->di.mode &= ~0070;	/* Clear group */
				bip->di.mode |= MASK2MODE(aclp);
				bip->di.gid = aclp->acl.group->a_id;
			}

			if (aclp->acl.other) {		/* Other */
				bip->di.mode &= ~0007;	/* Clear other */
				bip->di.mode |= (aclp->acl.other->a_perm & 07);
			}
			TRANS_INODE(bip->mp, bip);
			sam_mark_ino(bip, SAM_CHANGED);
		}
	}

out:
	if (error) {
		TRACE(T_SAM_ACL_ERR, SAM_ITOV(bip),
		    bip->di.id.ino, errline, error);
	}
	return (error);
}


/*
 * ----- sam_set_acl_ext - Save access control list to extension(s).
 *
 * Save regular and/or default access control list entries in inode
 * extension(s).
 *
 * Caller must have the inode write lock held.  Lock is not released.
 */
static int			/* ERRNO if error, 0 if successful */
sam_set_acl_ext(
	sam_node_t *bip,	/* Pointer to base inode */
	int	cnt,		/* Number of regular ACL entries */
	sam_acl_t *entp,	/* Pointer to list of regular ACL entries */
	int dfcnt,		/* Number of default ACL entries */
	sam_acl_t *dfentp)	/* Pointer to list of default ACL entries */
{
	int error = 0, errline = 0;
	int i_changed = 0;
	int n_exts;
	buf_t *bp;
	int done;
	sam_id_t eid;
	sam_ino_t ino;
	struct sam_inode_ext *eip;
	int i;
	int i_cnt = 0, i_dfcnt = 0;


	ASSERT(RW_WRITE_HELD(&bip->inode_rwl));

	/*
	 * Determine number of inode extension(s) needed for ACLs.
	 */
	n_exts = howmany(cnt + dfcnt, MAX_ACL_ENTS_IN_INO);

	if (n_exts != 1) {
		/*
		 * Release any previously allocated ACL inode extension(s).
		 */
		if (bip->di.ext_attrs & ext_acl) {
			sam_free_inode_ext(bip, S_IFACL, SAM_ALL_COPIES,
			    &bip->di.ext_id);
			bip->di.ext_attrs &= ~ext_acl;
			i_changed++;
		}
		if (n_exts == 0) {
			goto out;
		}
	}

	/*
	 * Reallocate if we need more than one since we don't know
	 * for sure how many we have.
	 *
	 * Allocate now if we don't have any inode extension(s).
	 */
	if (!(bip->di.ext_attrs & ext_acl)) {
		if ((error = sam_alloc_inode_ext(bip, S_IFACL, n_exts,
		    &bip->di.ext_id))) {
			errline = __LINE__ - 2;
		} else {
			bip->di.ext_attrs |= ext_acl;
			i_changed++;
		}
	}

	/*
	 * Find first ACL inode extension and copy regular and/or
	 * default ACL entries from caller.
	 */
	if (error == 0) {
		done = 0;
		eid = bip->di.ext_id;
		while ((ino = eid.ino) != 0 && !done) {
			if (error = sam_read_ino(bip->mp, ino, &bp,
					(struct sam_perm_inode **)&eip)) {
				errline = __LINE__ - 2;
				break;
			}
			if (EXT_HDR_ERR(eip, eid, bip)) {
				errline = __LINE__ - 1;
				error = ENOCSI;
				sam_req_ifsck(bip->mp, -1,
				    "sam_set_acl_ext: EXT_HDR", &bip->di.id);
				brelse(bp);
				break;
			}
			if (S_ISACL(eip->hdr.mode)) {
				/*
				 * Set up times, totals, and counts for
				 * this ext
				 */
				bzero((char *)&eip->ext.acl,
				    sizeof (eip->ext.acl));
				eip->ext.acl.creation_time = SAM_SECOND();
				eip->ext.acl.t_acls = cnt;
				eip->ext.acl.t_dfacls = dfcnt;
				eip->ext.acl.n_acls = 0;
				eip->ext.acl.n_dfacls = 0;

				/*
				 * Copy all regular ACL entries before copying
				 * default ACLs.
				 */
				for (i = 0; i < MAX_ACL_ENTS_IN_INO; i++) {
					if (i_cnt < cnt) {
						ASSERT((entp[i_cnt].a_type &
						    ACL_DEFAULT) == 0);
						eip->ext.acl.ent[i] =
						    entp[i_cnt];
						eip->ext.acl.n_acls++;
						i_cnt++;
					} else if (i_dfcnt < dfcnt) {
						ASSERT(
						    dfentp[i_dfcnt].a_type &
						    ACL_DEFAULT);
						eip->ext.acl.ent[i] =
						    dfentp[i_dfcnt];
						eip->ext.acl.n_dfacls++;
						i_dfcnt++;
					} else {
						break;
					}
				}
				if (i_cnt == cnt && i_dfcnt == dfcnt) {
					done = 1;
				}

				eid = eip->hdr.next_id;
				if (TRANS_ISTRANS(bip->mp)) {
					TRANS_WRITE_DISK_INODE(bip->mp, bp, eip,
					    eip->hdr.id);
				} else if (SAM_SYNC_META(bip->mp)) {
					if (error = sam_write_ino_sector(
					    bip->mp, bp, ino)) {
						errline = __LINE__ - 1;
						break;
					}
				} else {
					bdwrite(bp);
				}
			} else {
				eid = eip->hdr.next_id;
				brelse(bp);
			}
		}

		if (!done) {
			if (!error) {
				errline = __LINE__ - 2;
				error = ENOCSI;
			}
			sam_req_ifsck(bip->mp, -1,
			    "sam_set_acl_ext: done = 0", &bip->di.id);
		}
		if (error) {
			sam_free_inode_ext(bip, S_IFACL, SAM_ALL_COPIES,
			    &bip->di.ext_id);
			bip->di.ext_attrs &= ~ext_acl;
			i_changed++;
		}
	}

out:
	if (i_changed) {
		TRANS_INODE(bip->mp, bip);
		sam_mark_ino(bip, SAM_CHANGED);
	}
	TRACE(T_SAM_SET_ACL_EXT, SAM_ITOV(bip), (int)bip->di.id.ino,
	    (cnt + dfcnt), error);
	if (error) {
		TRACE(T_SAM_ACL_ERR, SAM_ITOV(bip),
		    bip->di.id.ino, errline, error);
	}
	return (error);
}
