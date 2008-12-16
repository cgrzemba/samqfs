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
#pragma ident	"$Revision: 1.6 $"
#endif

#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/errno.h>
#define	_LQFS_INFRASTRUCTURE
#include <lqfs_common.h>
#undef _LQFS_INFRASTRUCTURE
#ifdef LUFS
#include <sys/fs/qfs_inode.h>
#include <sys/fs/qfs_filio.h>
#include <sys/fs/qfs_log.h>
#endif /* LUFS */
#include <sys/sunddi.h>
#include <sys/file.h>
#include <sam/fioctl.h>

/* ARGSUSED */
int
qfs_fiologenable(vnode_t *vp, fiolog_t *ufl, cred_t *cr, int flags)
{
	int		error = 0;
	fiolog_t	fl;

	/*
	 * Enable logging
	 */
	if (ddi_copyin(*((caddr_t *)ufl), &fl, sizeof (fl), flags)) {
		return (EFAULT);
	}
	error = lqfs_enable(vp, &fl, cr);
	if (ddi_copyout(&fl, *((caddr_t *)ufl), sizeof (*ufl), flags)) {
		return (EFAULT);
	}

	return (error);
}

/* ARGSUSED */
int
qfs_fiologdisable(vnode_t *vp, fiolog_t *ufl, cred_t *cr, int flags)
{
	int		error = 0;
	struct fiolog	fl;

	/*
	 * Disable logging
	 */
	if (ddi_copyin(*((caddr_t *)ufl), &fl, sizeof (fl), flags)) {
		return (EFAULT);
	}
	error = lqfs_disable(vp, &fl);
	if (ddi_copyout(&fl, *((caddr_t *)ufl), sizeof (*ufl), flags)) {
		return (EFAULT);
	}

	return (error);
}

/*
 * qfs_fioislog
 *	Return FIOLOG_ENONE if log is present and active,
 *	FIOLOG_ENOTSUP if journaling is not supported,
 *	FIOLOG_EPEND if the state of journaling is not yet known (try again).
 */
/* ARGSUSED */
int
qfs_fioislog(vnode_t *vp, uint32_t *islog, cred_t *cr, int flags)
{
	qfsvfs_t	*qfsvfsp	= VTOI(vp)->i_qfsvfs;
	int		active;
	uint32_t	il;

	if (!qfsvfsp) {
		active = FIOLOG_EPEND;
	} else if (!LQFS_CAPABLE(qfsvfsp)) {
		active = FIOLOG_ENOTSUP;
	} else if ((qfsvfsp->mt.fi_status & FS_LOGSTATE_KNOWN) == 0) {
		active = FIOLOG_EPEND;
	} else {
		/*
		 * The journaling state has been set to something (on/off).
		 * Return code FIOLOG_ENONE indicates that journaling is
		 * enabled.
		 */
		active = (LQFS_GET_LOGP(qfsvfsp) ?
		    FIOLOG_ENONE : FIOLOG_ENOTSUP);
	}
	if (flags & FKIOCTL) {
		*islog = active;
	} else {
#ifdef LUFS
		if (suword32(islog, active)) {
#else
		if (ddi_copyin(*((caddr_t *)islog), &il, sizeof (il), flags)) {
			return (EFAULT);
		}
		il = active;
		if (ddi_copyout(&il, *((caddr_t *)islog), sizeof (*islog),
		    flags)) {
#endif /* LUFS */
			return (EFAULT);
		}
	}

	return (0);
}
