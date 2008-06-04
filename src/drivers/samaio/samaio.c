/* samaio.c - pseudo device driver for async I/O. */

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

#pragma ident	"@(#)samaio.c	1.1	03/01/14 SMI"

/*
 * samaio pseudo driver - allows you to attach a QFS file to a character
 * device, which can then be accessed through that device. The simple model
 * is that you tell samaio to open a file, and then use the character device
 * you get as you would any character device. samaio translates access to the
 * character device into I/O on the underlying file. This is useful for
 * aio because raw device I/O is faster than fs aio.
 *
 * samaio is controlled through /dev/samaioctl - this is the only device
 * exported during attach, and is minor number 0. QFS communicates
 * with samaio through ioctls on this device. When a file is attached to
 * samaio, character devices are exported in /dev/rsamaio. These devices
 * are identified by their minor number. Minor devices are tracked with
 * state structures handled with ddi_soft_state(9F).
 *
 */

#include <sam/osversion.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/sysmacros.h>
#include <sys/cmn_err.h>
#include <sys/uio.h>
#include <sys/kmem.h>
#include <sys/cred.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/aio.h>
#include <sys/aio_req.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/modctl.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#include <sys/open.h>
#include <sys/disp.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/mutex.h>

#include "sam/types.h"
#include "sam/samaio.h"
#include "sam/fs/macros.h"
#include "sam/fs/ino.h"
#include "sam/fs/inode.h"
#include "sam/fs/debug.h"

#define	SIZE_PROP_NAME		"Size"
#define	NBLOCKS_PROP_NAME	"Nblocks"
#define	SAMAIO_MAX_MINOR	65536
static uint32_t samaio_max_minor = 0;

static dev_info_t	*samaio_dip = NULL;
static void		*samaio_statep;
static kmutex_t		samaio_mutex;			/* state mutex */

/*
 * -----	Check for any outstanding opens for a minor entry.
 */

#define	IS_OPEN(aiop)   (((aiop)->aio_chr_flags & SAM_AIO_OPEN) || \
			(aiop)->aio_chr_lyropen)


/*
 * -----	Increment opens for a minor entry.
 */

#define	MARK_OPEN(otyp, aiop)					\
	if ((otyp) == OTYP_LYR) {				\
		++(aiop)->aio_chr_lyropen;			\
	} else {						\
		(aiop)->aio_chr_flags |= SAM_AIO_OPEN;		\
	}


/*
 * -----	Decrement opens for a minor entry.
 */

#define	MARK_CLOSED(otyp, aiop)					\
	if ((otyp) == OTYP_LYR) {				\
		--(aiop)->aio_chr_lyropen;			\
	} else {						\
		(aiop)->aio_chr_flags &= ~SAM_AIO_OPEN;		\
	}


/*
 * -----	samaio_busy - Check for any outstanding opens
 */

static int
samaio_busy(void)
{
	struct samaio_state *aiop;
	minor_t	minor;

	mutex_enter(&samaio_mutex);

	for (minor = 1; minor <= samaio_max_minor; minor++) {
		if ((aiop = ddi_get_soft_state(samaio_statep, minor))
		    != NULL) {
			if (IS_OPEN(aiop)) {
				mutex_exit(&samaio_mutex);
				return (EBUSY);
			}
		}
	}
	mutex_exit(&samaio_mutex);
	return (0);
}


/*
 * -----	samaio_open - Device driver open
 */

static int
samaio_open(
	dev_t *devp,
	int flag,
	int otyp,
	struct cred *credp)
{
	minor_t	minor;
	struct samaio_state *aiop;
	int error = 0;

	minor = getminor(*devp);
	if (minor == 0) {
		/*
		 * master control device -- must be opened exclusively
		 */
		if (((flag & FEXCL) != FEXCL) || (otyp != OTYP_CHR)) {
			return (EINVAL);
		}
		aiop = ddi_get_soft_state(samaio_statep, 0);
		if (aiop == NULL) {
			return (ENXIO);
		}
		mutex_enter(&samaio_mutex);
		if (IS_OPEN(aiop)) {
			mutex_exit(&samaio_mutex);
			return (EBUSY);
		}
		MARK_OPEN(otyp, aiop);
		mutex_exit(&samaio_mutex);
		return (0);
	}

	/*
	 * otherwise, the mapping should already exist
	 */
	aiop = ddi_get_soft_state(samaio_statep, minor);
	if ((aiop == NULL) || (aiop->aio_vp == NULL)) {
		return (ENXIO);
	}

	mutex_enter(&samaio_mutex);
	if (IS_OPEN(aiop)) {
		MARK_OPEN(otyp, aiop);		/* increment layered count? */
	} else {
		if ((error = VOP_OPEN_OS(&aiop->aio_vp, flag, credp, NULL))
		    == 0) {
			aiop->aio_filemode = flag;
			MARK_OPEN(otyp, aiop);
		}
	}
	mutex_exit(&samaio_mutex);

	return (error);
}


/*
 * -----	samaio_close - Device driver close
 */

/*ARGSUSED2*/
static int
samaio_close(
	dev_t dev,
	int flag,
	int otyp,
	struct cred *credp)
{
	minor_t	minor;
	struct samaio_state *aiop;

	minor = getminor(dev);
	aiop = ddi_get_soft_state(samaio_statep, minor);
	if ((aiop == NULL) || (aiop->aio_vp == NULL)) {
		return (ENXIO);
	}

	mutex_enter(&samaio_mutex);
	if (!IS_OPEN(aiop)) {
		cmn_err(CE_WARN,
		    "samaio_close: device (minor %d) not open\n", minor);
	} else {
		MARK_CLOSED(otyp, aiop);
		if (!IS_OPEN(aiop)) {
			(void) VOP_CLOSE_OS(aiop->aio_vp, flag, 1, 0, credp,
			    NULL);
		}
	}
	mutex_exit(&samaio_mutex);

	return (0);
}


/*
 * -----	samaio_strategy - Device driver close
 * The samaio_strategy is required for kaio to issue raw I/O.
 */

static int
samaio_strategy(struct buf *bp)
{
	struct samaio_state *aiop;
	struct samaio_uio *aiouiop;
	int ioflag;

	if (((uint_t)bp->b_un.b_addr & 1) != 0) { /* Must be short aligned */
		bioerror(bp, EFAULT);
		return (0);
	}

	aiop = ddi_get_soft_state(samaio_statep, getminor(bp->b_edev));
	bp_mapin(bp);

	aiouiop = (struct samaio_uio *)kmem_zalloc(sizeof (struct samaio_uio),
	    KM_SLEEP);
	aiouiop->uio.uio_iov = &aiouiop->iovec[0];
	aiouiop->uio.uio_iovcnt = 1;
	aiouiop->uio.uio_loffset = bp->b_lblkno * DEV_BSIZE;
	aiouiop->uio.uio_segflg = UIO_SYSSPACE;
	aiouiop->uio.uio_llimit = SAM_MAXOFFSET_T;
	aiouiop->uio.uio_resid = bp->b_bcount;

	aiouiop->type = SAMAIO_CHR_AIO; /* Identifies a pseudo aio request */

	aiouiop->iovec[0].iov_base = bp->b_un.b_addr;
	aiouiop->iovec[0].iov_len = bp->b_bcount;

	aiouiop->bp = bp;
	ioflag = (aiop->aio_filemode | FASYNC);

	if (bp->b_flags & B_READ) {
		aiouiop->uio.uio_fmode = FREAD;
		(void) VOP_READ_OS(aiop->aio_vp, (struct uio *)aiouiop, ioflag,
		    CRED(), NULL);
	} else {
		aiouiop->uio.uio_fmode = FWRITE;
		(void) VOP_WRITE_OS(aiop->aio_vp, (struct uio *)aiouiop,
		    ioflag, CRED(), NULL);
	}
	return (0);
}


/*
 * -----	samaio_read - Device driver synchronous read
 */

static int
samaio_read(
	dev_t dev,
	struct uio *uiop,
	struct cred *credp)
{
	struct samaio_state *aiop;
	int minor;
	int error;

	minor = getminor(dev);
	if (minor == 0) {
		return (EINVAL);
	}
	aiop = ddi_get_soft_state(samaio_statep, minor);

	error = VOP_READ_OS(aiop->aio_vp, uiop, FREAD, credp, NULL);

	return (error);
}


/*
 * -----	samaio_read - Device driver synchronous read
 */

static int
samaio_write(
	dev_t dev,
	struct uio *uiop,
	struct cred *credp)
{
	struct samaio_state *aiop;
	int minor;
	int error;

	minor = getminor(dev);
	if (minor == 0) {
		return (EINVAL);
	}
	aiop = ddi_get_soft_state(samaio_statep, minor);

	error = VOP_WRITE_OS(aiop->aio_vp, uiop, FWRITE, credp, NULL);

	return (error);
}


/*
 * -----	samaio_aread - Device driver asynchronous read
 */

/*ARGSUSED*/
static int
samaio_aread(
	dev_t dev,
	struct aio_req *aio,
	struct cred *credp)
{
	if (getminor(dev) == 0) {
		return (EINVAL);
	}
	return (aphysio(samaio_strategy, anocancel, dev, B_READ, minphys,
	    aio));
}


/*
 * -----	samaio_awrite - Device driver asynchronous write
 */

/*ARGSUSED*/
static int
samaio_awrite(
	dev_t dev,
	struct aio_req *aio,
	struct cred *credp)
{
	if (getminor(dev) == 0) {
		return (EINVAL);
	}
	return (aphysio(samaio_strategy, anocancel, dev, B_WRITE, minphys,
	    aio));
}


/*
 * -----	samaio_getinfo - Return information about ctl device file.
 */

/*ARGSUSED*/
static int
samaio_getinfo(
	dev_info_t *dip,
	ddi_info_cmd_t infocmd,
	void *arg,
	void **result)
{
	switch (infocmd) {
	case DDI_INFO_DEVT2DEVINFO:
		*result = samaio_dip;
		return (DDI_SUCCESS);
	case DDI_INFO_DEVT2INSTANCE:
		*result = 0;
		return (DDI_SUCCESS);
	}
	return (DDI_FAILURE);
}


/*
 * -----	samaio_attach - create and attach samaio ctl device file,
 *		minor = 0.
 */

static int
samaio_attach(
	dev_info_t *dip,
	ddi_attach_cmd_t cmd)
{
	int	error;

	if (cmd != DDI_ATTACH) {
		return (DDI_FAILURE);
	}
	error = ddi_soft_state_zalloc(samaio_statep, 0);
	if (error == DDI_FAILURE) {
		cmn_err(CE_WARN, "%s: cannot allocate pseudo device",
		    ddi_get_name(dip));
		return (DDI_FAILURE);
	}
	error = ddi_create_minor_node(dip, SAMAIO_CTL_NODE, S_IFCHR, 0,
	    DDI_PSEUDO, NULL);
	if (error == DDI_FAILURE) {
		ddi_soft_state_free(samaio_statep, 0);
		cmn_err(CE_WARN, "%s: cannot create pseudo device minor node",
		    ddi_get_name(dip));
		return (DDI_FAILURE);
	}
	samaio_dip = dip;
	ddi_report_dev(dip);
	return (DDI_SUCCESS);
}


/*
 * -----	samaio_detach - detach and remove samaio ctl device file.
 */

static int
samaio_detach(
	dev_info_t *dip,
	ddi_detach_cmd_t cmd)
{
	if (cmd != DDI_DETACH) {
		return (DDI_FAILURE);
	}
	if (samaio_busy()) {
		return (DDI_FAILURE);
	}
	samaio_dip = NULL;
	ddi_remove_minor_node(dip, NULL);
	ddi_soft_state_free(samaio_statep, 0);
	return (DDI_SUCCESS);
}


/*
 * -----	samaio_prop_op - Process proc_op command for samaio
 *		pseudo device.
 */

/* ARGSUSED */
static int
samaio_prop_op(
	dev_t dev,
	dev_info_t *dip,
	ddi_prop_op_t prop_op,
	int mod_flags,
	char *name,
	caddr_t valuep,
	int *lengthp)
{
	struct samaio_state *aiop;
	int length;
	caddr_t buffer;
	vattr_t vattr;
	minor_t	minor;
	int error;

	minor = getminor(dev);
	if (minor == 0) {
		return (EINVAL);
	}
	if ((aiop = ddi_get_soft_state(samaio_statep, minor)) == NULL) {
		return (EINVAL);
	}
	if (aiop->aio_vp == NULL) {
		return (EINVAL);
	}

	vattr.va_mask = AT_SIZE;
	if ((error = VOP_GETATTR_OS(aiop->aio_vp, &vattr, ATTR_REAL,
	    CRED(), NULL)) == 0) {
		length = *lengthp;			/* Caller's length */
		*lengthp = sizeof (vattr.va_size);   /* Set callers length */

		switch (prop_op)  {

		case PROP_LEN:
			break;

		case PROP_LEN_AND_VAL_ALLOC:
			buffer = (caddr_t)kmem_alloc(length,
			    (mod_flags & DDI_PROP_CANSLEEP) ?
			    KM_SLEEP : KM_NOSLEEP);
			if (buffer == NULL) {
				return (DDI_PROP_NO_MEMORY);
			}
			*(caddr_t *)(void *)valuep = buffer;
			return (DDI_PROP_SUCCESS);

		case PROP_LEN_AND_VAL_BUF:
			if (strncmp(name, SIZE_PROP_NAME,
			    sizeof (SIZE_PROP_NAME)) == 0) {
				if (sizeof (vattr.va_size) > length) {
					return (DDI_PROP_BUF_TOO_SMALL);
				}
				*((u_offset_t *)(void *)valuep) =
				    vattr.va_size;
			} else if (strncmp(name, "size", sizeof ("size")) ==
			    0) {
				if (sizeof (int32_t) > length) {
					return (DDI_PROP_BUF_TOO_SMALL);
				}
				*((int32_t *)(void *)valuep) =
				    (int32_t)vattr.va_size;
			} else if (strncmp(name, NBLOCKS_PROP_NAME,
			    sizeof (NBLOCKS_PROP_NAME)) == 0) {
				if (sizeof (vattr.va_size) > length) {
					return (DDI_PROP_BUF_TOO_SMALL);
				}
				*((u_offset_t *)(void *)valuep) =
				    (vattr.va_size/DEV_BSIZE);
			} else {
				return (DDI_PROP_NOT_FOUND);
			}
			break;

		default:
			return (DDI_PROP_NOT_FOUND);
		}
		return (DDI_PROP_SUCCESS);
	}
	return (error);
}


/*
 * -----	samaio_ioctl - Process ioctl commands for samaio pseudo device.
 */

/* ARGSUSED */
static int
samaio_ioctl(
	dev_t dev,
	int cmd,
	intptr_t arg,
	int flag,
	cred_t *credp,
	int *rvalp)
{
	minor_t	minor;
	struct samaio_ioctl *lip = (struct samaio_ioctl *)arg;
	struct samaio_state *aiop;
	int	error;

	/*
	 * samaio should have already samaio_dip (samaio_attach).
	 */
	if (samaio_dip == NULL) {
		cmn_err(CE_WARN, "samaio: ioctl does not have samaio_dip");
		return (ENXIO);
	}

	error = 0;
	minor = getminor(dev);
	switch (cmd) {

	case SAMAIO_ATTACH_DEVICE: {
		char namebuf[50];
		vnode_t *vp;
		equ_t fs_eq;
		uint32_t ino;
		int32_t gen;

		if (minor != 0) {
			cmn_err(CE_WARN,
			    "samaio: samaio_ioctl called from %p with "
			    "minor=%d, cmd=%x",
			    (void *)caller(), minor, cmd);
			error = ENOTTY;
			break;
		}

		/*
		 * Get a minor number. First lookup existing devices for
		 * a match. If no match, create a new soft device.
		 */
		vp = lip->vp;
		fs_eq = lip->fs_eq;
		ino = lip->ino;
		gen = lip->gen;

		mutex_enter(&samaio_mutex);
		for (minor = 1; minor <= samaio_max_minor; minor++) {
			if ((aiop = ddi_get_soft_state(samaio_statep, minor))
			    != NULL) {
				if ((aiop->aio_fs_eq == fs_eq) &&
				    (aiop->aio_ino == ino) &&
				    (aiop->aio_gen == gen)) {
					goto done;
				}
				continue;
			} else {
				break;
			}
		}
		if (minor > SAMAIO_MAX_MINOR) {
			error = EBUSY;
			goto out;
		}
		samaio_max_minor = MAX(minor, samaio_max_minor);

		error = ddi_soft_state_zalloc(samaio_statep, minor);
		if (error == DDI_FAILURE) {
			error = ENOMEM;
			goto out;
		}
		(void) snprintf(namebuf, sizeof (namebuf), "%d,raw", minor);
		error = ddi_create_minor_node(samaio_dip, namebuf, S_IFCHR,
		    minor, DDI_PSEUDO, NULL);
		if (error != DDI_SUCCESS) {
			cmn_err(CE_WARN,
			    "samaio: cannot create minor node %s, minor=%d",
			    namebuf, minor);
			ddi_soft_state_free(samaio_statep, minor);
			error = ENXIO;
			goto out;
		}
		aiop = ddi_get_soft_state(samaio_statep, minor);
done:
		aiop->aio_vp = vp;
		aiop->aio_fs_eq = fs_eq;
		aiop->aio_ino = ino;
		aiop->aio_gen = gen;
		mutex_exit(&samaio_mutex);
		if (rvalp) {
			*rvalp = (int)minor;
		}
		error = 0;
		break;

out:
		mutex_exit(&samaio_mutex);
		}
		break;

	case SAMAIO_DETACH_DEVICE: {
		char namebuf[50];

		aiop = ddi_get_soft_state(samaio_statep, minor);
		mutex_enter(&samaio_mutex);
		if (IS_OPEN(aiop)) {
			mutex_exit(&samaio_mutex);
			return (EBUSY);
		}

		/*
		 * If no opens, remove minor node.
		 */
		aiop->aio_vp = NULL;
		aiop->aio_ino = 0;
		aiop->aio_gen = 0;
		(void) snprintf(namebuf, sizeof (namebuf), "%d,raw", minor);
		ddi_remove_minor_node(samaio_dip, namebuf);
		ddi_soft_state_free(samaio_statep, minor);
		mutex_exit(&samaio_mutex);
		error = 0;
		}
		break;

	default: {
		char type;

		type = (cmd >> 8) & 0xff;

		switch (type) {
		case ((DKIOC) >> 8):		/* 04 */
		case 'P':
		case 'f':
			aiop = ddi_get_soft_state(samaio_statep, minor);
			mutex_enter(&samaio_mutex);
			if (!IS_OPEN(aiop)) {
				mutex_exit(&samaio_mutex);
				return (EINVAL);
			}
			mutex_exit(&samaio_mutex);
			if (aiop != NULL) {
				error = VOP_IOCTL_OS(aiop->aio_vp, cmd, arg,
				    flag, credp, rvalp, NULL);
			} else {
				error = ENOTTY;
			}
			break;

		default:
			error = ENOTTY;
			cmn_err(CE_NOTE,
			    "SAM-AIO: ioctl called by %p; unknown cmd=%x, "
			    "minor=%d",
			    (void *)caller(), cmd, minor);
		}
		}
		break;
	}
	return (error);
}


/*
 * -----	Device driver module tables.
 */

static struct cb_ops samaio_cb_ops = {
	samaio_open,			/* open */
	samaio_close,			/* close */
	samaio_strategy,		/* strategy */
	nodev,				/* print */
	nodev,				/* dump */
	samaio_read,			/* read */
	samaio_write,			/* write */
	samaio_ioctl,			/* ioctl */
	nodev,				/* devmap */
	nodev,				/* mmap */
	nodev,				/* segmap */
	nochpoll,			/* poll */
	samaio_prop_op,			/* prop_op */
	0,				/* streamtab  */
	D_64BIT | D_NEW | D_MP,		/* Driver compatibility flag */
	DEVO_REV,
	samaio_aread,			/* async read */
	samaio_awrite			/* async write */
};


static struct dev_ops samaio_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	samaio_getinfo,		/* getinfo */
	nulldev,		/* no identify routine */
	nulldev,		/* probe */
	samaio_attach,		/* attach */
	samaio_detach,		/* detach */
	nodev,			/* reset */
	&samaio_cb_ops,		/* driver operations */
	NULL			/* no bus operations */
};


/*
 * -----	Module linkage information for the kernel.
 */

static struct modldrv samaio_modldrv = {
	&mod_driverops,
	"SAM-QFS pseudo AIO driver",
	&samaio_ops,
};

static struct modlinkage samaio_modlinkage = {
	MODREV_1,
	&samaio_modldrv,
	NULL
};


/*
 *	-----	_init - Module loading routine.
 *	Modload the SAMAIO device driver module.
 */

int
_init(void)
{
	int error;

	error = ddi_soft_state_init(&samaio_statep, sizeof (samaio_state_t), 0);
	if (error) {
		return (error);
	}

	sam_mutex_init(&samaio_mutex, NULL, MUTEX_DRIVER, NULL);
	error = mod_install(&samaio_modlinkage);
	if (error) {
		mutex_destroy(&samaio_mutex);
		ddi_soft_state_fini(&samaio_statep);
	}
	return (error);
}


/*
 * -----	_fini - Module unloading routine.
 *	Modunload the SAMAIO device driver module.
 */

int
_fini(void)
{
	int	error;

	if (samaio_busy()) {
		return (EBUSY);
	}

	error = mod_remove(&samaio_modlinkage);
	if (error) {
		return (error);
	}
	ddi_soft_state_fini(&samaio_statep);
	mutex_destroy(&samaio_mutex);
	return (error);
}


/*
 * -----	_info - Module information.
 *	Get module information for the SAMAIO device driver module.
 */

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&samaio_modlinkage, modinfop));
}
