#pragma ident "$Revision: 1.15 $"

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

//
//	samioc.c - Driver which is the entry point for SAMFS system calls.
//
#include "sam/osversion.h"
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/buf.h>
#include <sys/modctl.h>
#include <sys/open.h>
#include <sys/kmem.h>
#include <sys/poll.h>
#include <sys/conf.h>
#include <sys/cmn_err.h>
#include <sys/stat.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

#include <sam/samioc.h>

#include "sam/fs/macros.h"

#define SAMIOC_NAME_VERSION "SAM-QFS system call interface v" MOD_VERSION
//
// Device information
//
typedef struct {
	dev_info_t	*dip;			// devinfo handle
} samioc_devstate_t;


//
// Private data.
//
static void	*samioc_state;
static kmutex_t	samioc_mutex;
static int	syscall_count = 0;	// Number of active system calls
static int	samioc_nofs(int, void *, int, rval_t *);
static int	(*fs_syscall)(int, void *, int, rval_t *) = samioc_nofs;


//
//	Pseudo-device open.
//
/*ARGSUSED*/
static int
samioc_open(dev_t *devp, int flag, int otyp, cred_t *cred)
{
	char				*ent_pnt = "samioc_open";
	samioc_devstate_t	*dev_state;

	dev_state = ddi_get_soft_state(samioc_state, getminor(*devp));

	if (dev_state == NULL) {
		return (ENXIO);			// causes deferred attach
	}

	if (otyp != OTYP_CHR) {
		return (EINVAL);
	}

	return (0);
}


//
//	Pseudo-device close..
//
/*ARGSUSED*/
static int
samioc_close(dev_t dev, int flag, int otyp, cred_t *cred)
{
	char				*ent_pnt = "samioc_close";
	samioc_devstate_t	*rsp;

	if ((rsp = ddi_get_soft_state(samioc_state, getminor(dev))) == NULL) {
		return (ENXIO);
	}

	return (0);
}


//
//	Dummy routine to return an error if the file system is not loaded.
//
static int
samioc_nofs(int a, void *b, int c, rval_t *d)
{
	cmn_err(CE_NOTE, "SAM-QFS: samfs kernel module not loaded\n");
	return (ENOPKG);
}


//
//	System call processor.
//
/*ARGSUSED*/
static int
samioc_ioctl(dev_t dev, int cmd, intptr_t arg, int mode,
    cred_t *credp, int *rvalp)
{
	char					*ent_pnt = "samioc_ioctl";
	samioc_devstate_t		*rsp;
	struct sam_syscall_args args;
	rval_t					rvp;
	int						rc;

	mutex_enter(&samioc_mutex);

	if ((rsp = ddi_get_soft_state(samioc_state, getminor(dev))) == NULL) {
		mutex_exit(&samioc_mutex);
		return (ENXIO);			// invalid minor number
	}

	if (ddi_model_convert_from((uint_t) mode) == DDI_MODEL_ILP32) {
		// Get 32-bit arguments and convert to 64 bit

		struct args32 {
			uint32_t	cmd;
			uint32_t	buf;
			uint32_t	size;
		} args32;

		if (ddi_copyin((void *)arg, &args32, sizeof (args32), mode)
		    != 0) {
			mutex_exit(&samioc_mutex);
			return (EFAULT);
		}

		args.cmd = (int)args32.cmd;
		args.buf = (void *)args32.buf;
		args.size = (int)args32.size;

	} else {
		// DDI_MODEL_NONE - no data conversion necessary
		if (ddi_copyin((void *)arg, &args, sizeof (args), mode) != 0) {
			mutex_exit(&samioc_mutex);
			return (EFAULT);
		}
	}

	syscall_count++;
	mutex_exit(&samioc_mutex);
	rvp.r_val1 = 0;
	rvp.r_val2 = 0;
	rc = fs_syscall(args.cmd, args.buf, args.size, &rvp);
	mutex_enter(&samioc_mutex);
	syscall_count--;
	mutex_exit(&samioc_mutex);
	*rvalp = rvp.r_val1;
	return (rc);
}


//
//	Establish or remove the linkage to the file system system call
//	handler.
//
//	The file system establishes the linkage after loading this driver.
//	The file system clears the linkage when it is being unloaded.
//
void
samioc_link(void *arg)
{
	mutex_enter(&samioc_mutex);

	if (arg) {
		//	Module loaded
		fs_syscall = (int(*)(int, void *, int, rval_t *))arg;
	} else {
		//	Module unloaded
		fs_syscall = samioc_nofs;
	}

	mutex_exit(&samioc_mutex);
}


/*ARGSUSED*/
static int
samioc_getinfo(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg,
    void **result)
{
	samioc_devstate_t	*rsp;
	int					rc = DDI_FAILURE;

	*result = NULL;

	switch (infocmd) {

	case DDI_INFO_DEVT2DEVINFO:
		rsp = ddi_get_soft_state(samioc_state, getminor((dev_t)arg));

		if (rsp != NULL) {
			*result = rsp->dip;
			rc = DDI_SUCCESS;
		}

		break;

	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *)getminor((dev_t)arg);
		rc = DDI_SUCCESS;
		break;
	}

	return (rc);
}


static int
samioc_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	if (cmd == DDI_DETACH) {
		int					instance;
		samioc_devstate_t	*rsp;

		ddi_prop_remove_all(dip);
		instance = ddi_get_instance(dip);
		rsp = ddi_get_soft_state(samioc_state, 0);

		if (rsp != NULL) {
			ddi_remove_minor_node(dip, NULL);
			ddi_soft_state_free(samioc_state, instance);
		}

		return (DDI_SUCCESS);
	}

	return (DDI_FAILURE);
}


static int
samioc_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	if (cmd == DDI_ATTACH) {
		int	instance = ddi_get_instance(dip);

		if (ddi_soft_state_zalloc(samioc_state, 0) == DDI_SUCCESS) {
			samioc_devstate_t	*rsp;

			rsp = ddi_get_soft_state(samioc_state, 0);
			rsp->dip = dip;

			// Create and initialize minor device
			if (ddi_create_minor_node(dip, "syscall", S_IFCHR,
			    0, NULL, 0) == DDI_SUCCESS) {
				ddi_report_dev(dip);
				return (DDI_SUCCESS);
			}
		}

		cmn_err(CE_CONT, "SAM-QFS: %s: can't allocate device\n",
		    ddi_get_name(dip));

		/*
		 * Use our own detach routine to toss
		 * away any stuff we allocated above.
		 */
		(void) samioc_detach(dip, DDI_DETACH);
	}

	return (DDI_FAILURE);
}


static struct cb_ops samioc_cb_ops = {
	&samioc_open,				// open(9E)
	&samioc_close,				// close(9E)
	nodev,					// strategy(9E)
	nodev,					// print(9E)
	nodev,					// dump(9E)
	nodev,					// read(9E)
	nodev,					// write(9E)
	&samioc_ioctl,				// ioctl(9E)
	nodev,					// devmap(9E)
	nodev,					// mmap(9E)
	nodev,					// segmap(9E)
	nochpoll,				// chpoll(9E)
	ddi_prop_op,				// prop_op(9E)
	NULL,					// streamtab(9E)
	D_NEW | D_MP,				// cb_flag
	CB_REV,					// cb_rev
	nodev,					// aread(9E)
	nodev					// awrite(9E)
};


static struct dev_ops samioc_dev_ops = {
	DEVO_REV,				// devo_rev
	0,					// devo_refcnt
	&samioc_getinfo,			// getinfo(9E)
	nulldev,				/* no identify routine */
	nulldev,				// probe(9E)
	&samioc_attach,				// attach(9E)
	&samioc_detach,				// detach(9E)
	nodev,					// devo_reset
	&samioc_cb_ops,				// devo_cb_ops
	NULL,					// devo_bus_ops
	NULL					// power(9E)
};


extern struct mod_ops mod_driverops;

static struct modldrv samioc_modldrv = {
	&mod_driverops,				// drv_modops
	SAMIOC_NAME_VERSION,			// drv_linkinfo
	&samioc_dev_ops				// drv_dev_ops
};


static struct modlinkage samioc_modlinkage = {
	MODREV_1,				// ml_rev
	&samioc_modldrv,			// ml_linkage[]
	0
};


//
//	Module initialization.
//
int
_init(void)
{
	int rc;

	rc = ddi_soft_state_init(&samioc_state, sizeof (samioc_devstate_t), 1);

	if (rc == 0) {
		rc = mod_install(&samioc_modlinkage);

		if (rc == 0) {
			sam_mutex_init(&samioc_mutex, NULL, MUTEX_DEFAULT,
			    NULL);
		} else {
			ddi_soft_state_fini(&samioc_state);
		}
	}

	return (rc);
}


//
//	Module termination.
//
//	There is a race condition where we can be loaded by the file system
//	but unloaded by a startup script.  In addition, if we're unloaded while
//	the file system is loaded, the system call won't be properly connected.
//	To solve this, we prevent unload if (a) this is the first attempt to
//	unload after we've been loaded and we've never been attached to the
//	file system, or (b) we're currently attached to the file system.
//
//  In addition, we can't let ourselves be unloaded while a system call is
//	active, since our address will be on its stack.
//
int
_fini(void)
{
	int rc;
	int	busy = 0;

	mutex_enter(&samioc_mutex);

	if ((fs_syscall != samioc_nofs) || (syscall_count > 0)) {
		busy = 1;
	}

	mutex_exit(&samioc_mutex);

	if (busy) {
		return (EBUSY);
	}

	rc = mod_remove(&samioc_modlinkage);

	if (rc == 0) {
		ddi_soft_state_fini(&samioc_state);
		mutex_destroy(&samioc_mutex);
	}

	return (rc);
}


//
//	Module information.
//
int
_info(struct modinfo *modinfop)
{
	return (mod_info(&samioc_modlinkage, modinfop));
}
