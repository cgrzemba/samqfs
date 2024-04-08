/* samst.c - scsi target driver for optical disk and robot control. */

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

/*
 * bst.c - Block SCSI Target driver; an example block SCSA target
 *	   driver for SunOS 5.3 (SVR4)
 *
 * This file is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this file without charge, but are not authorized to
 * license or distribute it to anyone else except as part of a product
 * or program developed by the user.
 *
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * This file is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS FILE
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even
 * if Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * ---------------------------------------------------------------------------
 *
 * This driver is intended as an example of programming a block SCSA
 * target driver for SVR4; it is not intended for any particular
 * direct access device, although it has been tested on a hard disk drive.
 *
 * This driver looks for a Sun-style disk label (struct dk_label, as defined
 * in <sys/dklabel.h>) on sector zero of the device. The label contains the
 * offsets and lengths of each data partition on the device. The driver
 * creates a minor device node for each partition, and uses the partition
 * info from the label to calculate the physical block number from the
 * logical block requested.
 *
 * This driver also supports the "DKIO" ioctls necessary to format and
 * partition the disk (defined in <sys/dkio.h>).
 *
 * The other major difference between this and the character-only driver
 * (sst) is that this driver supports I/O request queueing.
 *
 * Terminology in this driver:
 *	"instance" - this device, e.g. the disk drive
 *	"partition" - the part of the device ("slice") that's being accessed
 * The instance and partition are coded into the minor device number; the
 * partition is the low three bits (i.e. 8 partitions), and the instance is
 * the rest (i.e. bits 3 - 15).
 *
 * NOTE: Areas where you may need to change this code or add your own to
 *	deal with your specific device are marked "Note".
 * Other warnings are marked "WARNING"
 *
 * To compile:
 *	% cc -D_KERNEL -c samst.c
 *	% ld -r samst.o -o samst
 * WARNING: Only the -Xa and -Xt (the default) will work! The header files
 *		don't conform to strict ANSI C; -Xs and -Xc will fail.
 *
 * To install:
 * 1. Copy the module (samst) and config file (samst.conf) into /usr/kernel/drv
 * 2. Run add_drv(1M).
 *
 * Note: you only need to run add_drv the very first time you introduce
 *	 the driver to the system. Thereafter you can use modunload to
 *	 unload the driver and just reference it (e.g. open(2) it) to
 *	reload.
 *
 * Setting variables
 *	Variables can be explicitly set from the /etc/system file, by
 *	adding an entry of the form
 *		"set samst:<variable name>=<value>"
 *	Alternatively, you can use adb to set variables and debug as
 *	follows:
 *		# adb -kw /dev/ksyms /dev/mem
 *	The /etc/system file is read only once at boot time, if you change
 *	it you must reboot for the change to take effect.
 */

/*
 * Notes:
 * 1) Removed the label code.  Optical disks and robots as used by SAMFS
 *	 do not use sun labeles.
 *
 * 2) Removed multiple slices.	See 1.
 *
 * 3) Add check for FNDELAY|FNONBLOCK to open.	If either of these flags
 *	  are set, the test_unit_ready is skipped.	This allows an open
 *	  followed by a reset or spinup type command.  If neither of these
 *	  flags are present at open, then the unit must pass test_unit_ready.
 */

#pragma ident "$Revision: 1.28 $"


/*
 * Includes, Declarations and Local Data
 */
#include "sam/osversion.h"
#define	_SYSCALL32		1
#define	_SYSCALL32_IMPL	1
#include <sys/scsi/scsi.h>
#include <sys/dklabel.h>
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <sys/stat.h>
#include <sys/modctl.h>
#include <sys/file.h>

#include "sam/types.h"
#include "driver/samst_def.h"
#include "aml/scsi.h"

#ifndef DDI_MODEL_NONE
#define	DDI_MODEL_NONE 0
#endif

#ifdef ORACLE_SOLARIS
#define DDI_PM_RESUME DDI_PM_RESUME_OBSOLETE
#define DDI_PM_SUSPEND DDI_PM_SUSPEND_OBSOLETE
#endif

/*
 * Local Function Prototypes
 */
static int	samst_open(dev_t *dev_p, int flag, int otyp, cred_t *cred_p);
static int	samst_close(dev_t dev, int flag, int otyp, cred_t *cred_p);
static int	samst_strategy(struct buf *bp);
static int	samst_ioctl(dev_t, int, sam_intptr_t, int, cred_t *, int *);
static int	samst_read(dev_t dev, struct uio *uio, cred_t *cred_p);
static int	samst_write(dev_t dev, struct uio *uio, cred_t *cred_p);

static int	samst_rw(dev_t dev, struct uio *uio, int rw);
static int	samst_ioctl_cmd(dev_t, struct uscsi_cmd *, enum uio_seg,
				enum uio_seg, int mode);
static int	samst_ready(struct samst_soft *targ, dev_t dev);
static int	samst_simple(dev_t dev, int cmd, int flag);
static void	samst_start(struct samst_soft *targ);
static int	samst_runout(caddr_t arg);
static void	samst_done(struct samst_soft *targ, struct buf *bp);
static void	samst_make_cmd(struct samst_soft *targ, struct buf *bp,
				int (*f) ());
static void	samst_restart(void *arg);
static void	samst_callback(struct scsi_pkt *pkt);
static int	samst_handle_incomplete(struct samst_soft *targ,
					struct buf *bp);
static int	samst_handle_arq(struct scsi_pkt *pktp,
				struct samst_soft *targ,
				struct buf *bp);
static int	samst_handle_sense(struct samst_soft *targ, struct buf *bp);
static int	samst_check_error(struct samst_soft *targ, struct buf *bp);
static void	samst_log(struct scsi_device *devp, int level,
			const char *fmt, ...);
static void	hex_print(char *msg, char *cp, int len);

/*
 * Local Static Data
 */
static int	samst_io_time = SAMST_IO_TIME;
static int	samst_retry_count = SAMST_RETRY_COUNT;
static void    *samst_state;

/*
 * Array of commands supported by the device, suitable for scsi_errmsg(9f)
 * Note: Add or remove commands here as appropriate for your device.
 */
static struct scsi_key_strings samst_cmds[] = {
	0x00, "test unit ready",
	0x01, "rezero",
	0x03, "request sense",
	0x04, "format",
	0x05, "read block limits",
	0x07, "reassign",
	0x08, "read",
	0x0a, "write",
	0x0b, "seek",
	0x0f, "read reverse",
	0x12, "inquiry",
	0x13, "verify",
	0x14, "recover buffered data",
	0x15, "mode select",
	0x16, "reserve",
	0x17, "release",
	0x18, "copy",
	0x1a, "mode sense",
	0x1b, "start/stop/load",
	0x1e, "door lock",
	0x28, "read (10)",
	0x2a, "write (10)",
	0x2e, "writev (10)",
	0x37, "read defect data",
	0x4c, "log select",
	0xa8, "read (12)",
	0xaa, "write (12)",
	0xae, "writev (12)",
	-1, NULL,
};

/*
 * Errors at or above this level will be reported
 */
int		samst_error_reporting = SCSI_ERR_RETRYABLE;

/*
 * Debug message control
 * Debug Levels:
 *	0 = no messages
 *	1 = Errors
 *	2 = Subroutine calls & control flow
 *	3 = I/O Data (verbose!)
 *	4 = really verbose - mostly at attach time
 * Can be set with adb or in the /etc/system file with
 * "set samst:samst_debug=<value>"
 * Defining DEBUG on the compile line (-DDEBUG) will enable debugging
 * statements in this driver, and will also enable the ASSERT statements.
 */
#ifdef DEBUG
int		samst_debug = 2;
#else
int		samst_debug = 0;
#endif

/*
 * If the following is non-zero, this driver will drive devices that
 * report as removable type 0 (disk devices) in the inquiry data.  This is
 * to allow users who have other products that have used the device and
 * must let the sun disk driver (sd) drive the device.	This driver will
 * make no use of sun labels or partition maps for this device type.
 *
 * This value my be changed as described for samst_debug above.
 */

int		samst_direct = 0;
int		samst_FULLdir = 0;	/* allow all driect devices */

/*
 *	Module Loading/Unloading and Autoconfiguration Routines
 */

/*
 * Device driver ops vector - cb_ops(9s)
 * Device switch table fields (equivalent to the old 4.x cdevsw and bdevsw).
 * Unsupported entry points (e.g. for xxprint() and xxdump()) are set to
 * nodev, except for the poll routine, which is set to nochpoll(), a routine
 * that returns ENXIO.
 * Note This uses ddi_prop_op for the prop_op(9e) routine. If your device
 *	has its own properties, you should implement a samst_prop_op() routine
 *	to manage them.
 */
static struct cb_ops samst_cb_ops = {
	samst_open,				/* b/c open */
	samst_close,				/* b/c close */
	samst_strategy,				/* b strategy */
	nodev,					/* b print */
	nodev,					/* b dump */
	samst_read,				/* c read */
	samst_write,				/* c write */
	samst_ioctl,				/* c ioctl */
	nodev,					/* c devmap */
	nodev,					/* c mmap */
	nodev,					/* c segmap */
	nochpoll,				/* c poll */
	ddi_prop_op,				/* cb_prop_op */
	NULL,					/* streamtab  */
	D_64BIT | D_MP | D_NEW | D_HOTPLUG,	/* Driver compatibility flag */
	CB_REV,					/* cb_ops revision */
	nodev,					/* c aread */
	nodev					/* c awrite */

};


/*
 * dev_ops(9S) structure
 * Device Operations table, for autoconfiguration
 * Note If you replace the samst_detach entry here with "nulldev", it
 *	implies that the detach is always successful. We need a real detach to
 *	free the sense packet and the unit structure. If you don't want the
 *	driver to ever be unloaded, replace the samst_detach entry with "nodev"
 *	(which always fails).
 */
static int
samst_info(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg,
	void **result);
static int	samst_probe(dev_info_t *devi);
static int	samst_attach(dev_info_t *devi, ddi_attach_cmd_t cmd);
static int	samst_detach(dev_info_t *devi, ddi_detach_cmd_t cmd);
static int	samst_doattach(dev_info_t *devi);
static int	samst_dodetach(dev_info_t *devi);

static struct dev_ops samst_ops = {
	DEVO_REV,		/* devo_rev, */
	0,			/* refcnt  */
	samst_info,		/* info */
	nulldev,		/* no identify routine */
	samst_probe,		/* probe */
	samst_attach,		/* attach */
	samst_detach,		/* detach */
	nodev,			/* reset */
	&samst_cb_ops,		/* driver operations */
	NULL,			/* bus operations */
	nodev,			/* power */
};


static struct modldrv modldrv = {
	&mod_driverops,				/* Type of module (driver). */
	"SAM-QFS MO/library SCSI Driver",	/* Description */
	&samst_ops,				/* driver ops */
};

static struct modlinkage modlinkage = {
	MODREV_1, &modldrv, NULL
};

/*
 * Tell the system that we depend on the general scsi support routines,
 * i.e the scsi "misc" module must be loaded
 * Note: This line must be present! Unfortunately it's not documented,
 *	 but you must have it here or the driver won't load.
 */
#ifndef lint
char	    _depends_on[] = "misc/scsi";
#endif


/*
 *	Module Loading/Installation Routines
 */

/*
 * Module Installation
 * Install the driver and initialize private data structure allocation
 */
int
_init(void)
{
	int		e;

	if ((e = ddi_soft_state_init(&samst_state, sizeof (struct samst_soft),
	    0)) != 0) {
		SAMST_LOG(0, SAMST_CE_DEBUG2,
		    "_init ddi_soft_state_init failed: 0x%x\n", e);
		return (e);
	}
	if ((e = mod_install(&modlinkage)) != 0) {
		SAMST_LOG(0, SAMST_CE_DEBUG2,
		    "_init mod_install failed: 0x%x\n", e);
		ddi_soft_state_fini(&samst_state);
	}
	SAMST_LOG(0, SAMST_CE_DEBUG2, "_init succeeded\n");
	return (e);
}


/*
 * Module removal
 */
int
_fini(void)
{
	int		e;

	if ((e = mod_remove(&modlinkage)) != 0) {
		SAMST_LOG(0, SAMST_CE_DEBUG1,
		    "_fini mod_remove failed, 0x%x\n", e);
		return (e);
	}
	ddi_soft_state_fini(&samst_state);

	SAMST_LOG(0, SAMST_CE_DEBUG2, "_fini succeeded\n");
	return (e);
}

/*
 * Return module info
 */
int
_info(struct modinfo *modinfop)
{
	SAMST_LOG(0, SAMST_CE_DEBUG2, "_info\n");
	return (mod_info(&modlinkage, modinfop));
}


/*
 *	Autoconfiguration Routines
 */

/*
 * probe(9e) - Check that we're talking to the right device on the SCSI bus.
 * Calls scsi_probe() to see if there's a device at our target id. If
 * there is, scsi_probe will fill in the sd_inq struct (in the devp
 * scsi device struct) with the inquiry data. Validate the data here,
 * allocate a request sense packet, and start filling in the private data
 * structure.
 * Note: Probe should have no side-effects. Just check that we have the right
 *	 device, don't set up any data or initialize the device here. Also,
 *	 it's a Sun convention to probe quietly; send messages to the log
 *	 file only, not to the console.	 Finally, call scsi_unprobe to undo
 *	 the effects of scsi_probe.
 * Note: The host adapter driver sets up the scsi_device structure and puts
 *	 it into the dev_info structure with ddi_set_driver_private().
 * Note: No need to allow for probing/attaching in the open() routine because
 *	 of the loadability - the first reference to the device will auto-load
 *	 it, i.e. will call this routine.
 */
#define	VIDSZ		8	/* Vendor Id length in Inquiry Data */
#define	PIDSZ		16	/* Product Id length in Inquiry Data */

static int
samst_probe(dev_info_t *dip)
{
	struct scsi_device *devp;
	int		err, rval = DDI_PROBE_FAILURE;
	int		tgt, lun;
	char	    vpid[VIDSZ + PIDSZ + 1];

	tgt = ddi_getprop(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS, "target", -1);
	lun = ddi_getprop(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS, "lun", -1);

	devp = (struct scsi_device *)ddi_get_driver_private(dip);
	SAMST_LOG(devp, SAMST_CE_DEBUG2, "samst_probe: target %d, lun %d\n",
	    ddi_getprop(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS, "target", -1),
	    ddi_getprop(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS, "lun", -1));

	/*
	 * scsi_probe(9f) uses the SCSI Inquiry command to  test for the
	 * presence of the device. If it's successful, it will fill in the
	 * sd_inq field in the scsi_device structure.
	 */
	switch (err = scsi_probe(devp, NULL_FUNC)) {
	case SCSIPROBE_NORESP:
		SAMST_LOG(devp, SAMST_CE_DEBUG4,
		    "No response from target %d, lun %d\n",
		    tgt, lun);
		rval = DDI_PROBE_FAILURE;
		break;

	case SCSIPROBE_NONCCS:
	case SCSIPROBE_NOMEM:
	case SCSIPROBE_FAILURE:
	default:
		SAMST_LOG(devp, SAMST_CE_DEBUG2,
		    "samst_probe: scsi_probe failed, 0x%x\n", err);
		rval = DDI_PROBE_FAILURE;
		break;

	case SCSIPROBE_EXISTS:
		/*
		 * Inquiry succeeded, devp->sd_inq is now filled in Note:
		 * Check inq_dtype, inq_vid, inq_pid and any other fields to
		 * make sure the target/unit is what's expected (sd_inq is a
		 * struct scsi_inquiry, defined in scsi/generic/inquiry.h).
		 * Note: Put device-specific checking into the appropriate
		 * case statement, and delete the rest. Note: The DTYPE_*
		 * defines are from <scsi/generic/inquiry.h>, this is the
		 * full list as of "now", check it for new types.
		 */
		switch (devp->sd_inq->inq_dtype) {
		case DTYPE_DIRECT:

			if (!((samst_direct && devp->sd_inq->inq_rmb) ||
			    samst_FULLdir)) {
				rval = DDI_PROBE_FAILURE;
				break;
			}
			/* FALLTHROUGH */

		case DTYPE_OPTICAL:
		case DTYPE_WORM:
		case DTYPE_CHANGER:
			/*
			 * Print what was found on the console. Note	For
			 * your device, you should send the 'found' message
			 * to the system log file only, by inserting an
			 * exclamation point, "!", as the first character of
			 * the message - see cmn_err(9f).
			 */
			SAMST_LOG(devp, SAMST_CE_DEBUG1,
			    "Found %s device at tgt%d, lun%d\n",
			    scsi_dname((int)devp->sd_inq->inq_dtype), tgt,
			    lun);
			bcopy((caddr_t)devp->sd_inq->inq_vid, (caddr_t)vpid,
			    VIDSZ);
			bcopy((caddr_t)devp->sd_inq->inq_pid,
			    (caddr_t)vpid + VIDSZ, PIDSZ);
			vpid[VIDSZ + PIDSZ] = 0;
			if (bcmp((caddr_t)vpid, "AMPEX	DST310", 14) != 0) {
				samst_log(devp, CE_CONT,
				    "Vendor/Product ID = %s\n",
				    vpid);
				rval = DDI_PROBE_SUCCESS;
			} else
				rval = DDI_PROBE_FAILURE;
			break;

		case DTYPE_NOTPRESENT:
			SAMST_LOG(devp, SAMST_CE_DEBUG4,
			    "Target reports no device present\n");
			rval = DDI_PROBE_FAILURE;
			break;

		case DTYPE_RODIRECT:
		default:
			SAMST_LOG(devp, SAMST_CE_DEBUG4,
			    "Unsupported %s device at tgt%d lun%d\n",
			    scsi_dname((int)devp->sd_inq->inq_dtype), tgt,
			    lun);
			rval = DDI_PROBE_FAILURE;
			break;
		}
	}
	scsi_unprobe(devp);	/* it's OK to call this if */
	/* scsi_probe failed */
	return (rval);
}


/*
 * Attach
 *	- create minor device nodes
 *	- initialize per-unit mutex's & conditional variables
 *	- if it's a fixed-disk device, read & validate the label now; if
 *	  it's removable, wait until open().
 */
static int
samst_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	int		instance;
	struct samst_soft *tgt;

	instance = ddi_get_instance(dip);
	tgt = ddi_get_soft_state(samst_state, instance);

	switch (cmd) {
	case DDI_ATTACH:
		if (samst_doattach(dip) == DDI_FAILURE) {
			return (DDI_FAILURE);
		}
		return (DDI_SUCCESS);

	case DDI_PM_RESUME:

		/*
		 * There is no hardware state to restart and no timeouts to
		 * restart since we didn't PM_SUSPEND with active cmds or
		 * active timeouts We just need to unblock waiting threads
		 * and restart I/O
		 *
		 * the code for DDI_RESUME is almost identical except it uses
		 * the suspend flag rather than pm_suspend flag
		 */
		if (tgt == NULL)
			return (DDI_FAILURE);

		mutex_enter(SAMST_MUTEX(tgt));

		/* allow new operations and unblock any waiting threads */
		tgt->pm_suspended = 0;
		cv_broadcast(&tgt->suspend_cv);

		mutex_exit(SAMST_MUTEX(tgt));

		/* restart I/O, there should not be an active cmd */
		ASSERT(tgt->targ_iorq.b_forw == NULL);

		samst_start(tgt);

		return (DDI_SUCCESS);

	case DDI_RESUME:
		if (tgt == NULL)
			return (DDI_FAILURE);

		mutex_enter(SAMST_MUTEX(tgt));

		/* unblock waiting threads */
		tgt->suspended = 0;
		cv_broadcast(&tgt->suspend_cv);

		/* are we still pm_suspended? */
		if (tgt->pm_suspended) {
			mutex_exit(SAMST_MUTEX(tgt));
			return (DDI_SUCCESS);
		}
		mutex_exit(SAMST_MUTEX(tgt));

		/* restart I/O, there should not be an active cmd */
		ASSERT(tgt->targ_iorq.b_forw == NULL);

		samst_start(tgt);

		return (DDI_SUCCESS);

	default:
		SAMST_LOG(0, SAMST_CE_DEBUG1,
		    "samst_attach: unsupported cmd 0x%x\n", cmd);
		return (DDI_FAILURE);
	}
}

static int
samst_doattach(dev_info_t *dip)
{
	struct scsi_pkt *rqpkt;
	struct samst_soft *targ;
	struct scsi_device *devp;
	struct buf	*bp;
	char	   *node_type;
	int		instance, minor;
	int		retval;
	char	    name[8];

	/*
	 * Find which instance we are, and create a data structure for the
	 * per-instance data. Cross-link the scsi_device struct (devp) with
	 * the per-instance structure (targ).
	 */
	devp = (struct scsi_device *)ddi_get_driver_private(dip);
	instance = ddi_get_instance(dip);
	SAMST_LOG(devp, SAMST_CE_DEBUG2,
	    "samst_doattach: unit %d; dip 0x%llx; devp 0x%llx\n",
	    instance, dip, devp);

	/*
	 * Re-probe the device to get the Inquiry data; it's used elsewhere
	 * in the driver. The Inquiry data was validated in samst_probe so
	 * there's no need to look at it again here.
	 */
	if (scsi_probe(devp, NULL_FUNC) != SCSIPROBE_EXISTS) {
		SAMST_LOG(0, SAMST_CE_DEBUG1,
		    "samst_attach: re-probe failed\n");
		scsi_unprobe(devp);
		return (DDI_FAILURE);
	}
	if (ddi_soft_state_zalloc(samst_state, instance) != DDI_SUCCESS) {
		scsi_unprobe(devp);
		return (DDI_FAILURE);
	}
	targ = ddi_get_soft_state(samst_state, instance);
	devp->sd_private = (opaque_t)targ;
	targ->targ_devp = devp;

	/* Set default block size and shift count */
	targ->targ_dev_bsize = 1024;
	targ->targ_dev_bshift = 10;

	/* Initialize the low and high write blocks */
	targ->targ_low_write = targ->targ_high_write = SAMST_RANGE_NOWRITE;

	/*
	 * Allocate a buffer for 'special' (driver internal) commands
	 */
	targ->targ_sbufp = getrbuf(KM_NOSLEEP);
	if (targ->targ_sbufp == NULL)
		goto error;

	/*
	 * Set auto-rqsense, per-target; record whether it's allowed in
	 * targ_arq
	 */
	/*
	 * Since the sun default extended sense is only 20 bytes, we can't
	 * use auto-rqsense.  Force tarq_arq false and do it ourselves. Note:
	 * This does not appear to work, so we are stuck with only 20 bytes
	 * of sense data.
	 */
#if !defined(SAMFS)
	targ->targ_arq = scsi_ifsetcap(ROUTE(targ), "auto-rqsense", 1, 1) ==
	    1 ? 1 : 0;
	SAMST_LOG(devp, SAMST_CE_DEBUG2, "samst_attach: Auto Sensing %s\n",
	    targ->targ_arq ? "enabled" : "disabled");
#else
	(void) scsi_ifsetcap(ROUTE(targ), "auto-rqsense", 0, 1);
	targ->targ_arq = scsi_ifgetcap(ROUTE(targ), "auto-rqsense", 1) ==
	    1 ? 1 : 0;
	cmn_err(CE_NOTE, "^samst_attach: Auto Sensing %s\n",
	    targ->targ_arq ? "enabled" : "disabled");
#endif

	if (!targ->targ_arq) {
		/*
		 * No auto sensing, have to do it ourselves. Set up a packet
		 * for it now so we don't have to possibly wait for resources
		 * for it when a command fails.
		 */
#if defined(SAMFS)
		bp = scsi_alloc_consistent_buf(&devp->sd_address,
		    NULL, SAM_SENSE_LENGTH, B_READ, NULL_FUNC,
		    NULL);
#else
		bp = scsi_alloc_consistent_buf(&devp->sd_address,
		    NULL, SENSE_LENGTH, B_READ, NULL_FUNC,
		    NULL);
#endif
		if (!bp)
			goto error;

		rqpkt = scsi_init_pkt(&devp->sd_address, NULL, bp, CDB_GROUP0,
		    1, 0, PKT_CONSISTENT, NULL_FUNC, NULL);
		if (!rqpkt)
			goto error;

		/*
		 * Note: can't change this to sam_extended_sense, but
		 * pointers is pointers.
		 */
		devp->sd_sense = (struct scsi_extended_sense *)bp->b_un.b_addr;
#if defined(SAMFS)
		makecom_g0(rqpkt, devp, FLAG_NOPARITY, SCMD_REQUEST_SENSE, 0,
		    SAM_SENSE_LENGTH);
#else
		makecom_g0(rqpkt, devp, FLAG_NOPARITY, SCMD_REQUEST_SENSE, 0,
		    SENSE_LENGTH);
#endif
		rqpkt->pkt_comp = samst_callback;
		rqpkt->pkt_time = samst_io_time;
		rqpkt->pkt_flags |= FLAG_SENSING;
		targ->targ_rqbp = bp;
		targ->targ_rqs = rqpkt;
	}
	/*
	 * Create the minor node. See the man page for
	 * ddi_create_minor_node(9f). The 2nd parameter is the minor node
	 * name; drvconfig(1M) or devfsadm(1M) appends it to the /devices
	 * entry, after the colon. The 4th parameter ('instance') is the
	 * actual minor number, put into the /devices entry's inode and
	 * passed to the driver. The 5th parameter (DDI_NT_*) is the node
	 * type; it's used by disks(1M) or devfsadm(1M) to create the links
	 * from /dev to /devices.
	 */

	minor = SAMST_MINOR(instance, 0);

	SAMST_LOG(devp, SAMST_CE_DEBUG4,
	    "interconnect_type: %d; scsi_version: %d\n",
	    scsi_ifgetcap(ROUTE(targ), "interconnect-type", 0),
	    scsi_ifgetcap(ROUTE(targ), "scsi-version", 0));
	if (scsi_ifgetcap(ROUTE(targ), "interconnect-type", 0) ==
	    INTERCONNECT_FABRIC) {
		node_type = DDI_NT_SAMST_FABRIC;
	} else if (scsi_ifgetcap(ROUTE(targ), "scsi-version", 0) ==
	    SCSI_VERSION_3) {
		node_type = DDI_NT_SAMST_WWN;
	} else {
		node_type = DDI_NT_SAMST_CHAN;
	}

	SAMST_LOG(devp, SAMST_CE_DEBUG1, "node_type: %s\n", node_type);

	(void) sprintf(name, "%c", 'a');
	if ((retval = ddi_create_minor_node(dip, name, S_IFBLK,
	    minor, node_type, 0)) == DDI_FAILURE) {
		SAMST_LOG(devp, SAMST_CE_DEBUG1,
		"ddi_create_minor_node(a) failed, returned %d\n", retval);
		goto error;
	}
	SAMST_LOG(devp, SAMST_CE_DEBUG4,
	    "ddi_create_minor_node(a) returned %d\n", retval);

	(void) sprintf(name, "%c,raw", 'a');
	if ((retval = ddi_create_minor_node(dip, name, S_IFCHR,
	    minor, node_type, 0)) == DDI_FAILURE) {
		SAMST_LOG(devp, SAMST_CE_DEBUG1,
		    "ddi_create_minor_node(a,raw) failed, returned %d\n",
		    retval);
		goto error;
	}
	SAMST_LOG(devp, SAMST_CE_DEBUG4,
	    "ddi_create_minor_node(a,raw) returned %d\n", retval);

	/*
	 * Initialize the conditional variable. Note: We don't need to
	 * initialize the driver instance mutex because it's actually
	 * devp->sd_mutex (in the struct scsi_device) and is initialized by
	 * our parent, the host adapter driver.
	 */
	cv_init(&targ->targ_sbuf_cv, "targ_cv", CV_DRIVER, NULL);

	/*
	 * Initialize the open/close semaphore. This allows open() and
	 * close() calls to be serialized (i.e. a current open/close thread
	 * will complete before another is allowed).
	 */
	sema_init(&targ->targ_semaphore, 1, "samst_sem", SEMA_DRIVER, NULL);

	SAMST_LOG(devp, SAMST_CE_DEBUG2, "Attached samst driver\n");
	ddi_report_dev(dip);
	return (DDI_SUCCESS);

error:
	if (targ->targ_rqbp)
		scsi_free_consistent_buf(targ->targ_rqbp);

	if (targ->targ_rqs)
		scsi_destroy_pkt(targ->targ_rqs);

	if (targ->targ_sbufp)
		freerbuf(targ->targ_sbufp);

	ddi_remove_minor_node(dip, NULL);	/* remove all minor nodes */
	ddi_soft_state_free(samst_state, instance);
	devp->sd_private = (opaque_t)0;
	devp->sd_sense = (struct scsi_extended_sense *)0;
	scsi_unprobe(devp);
	return (DDI_FAILURE);
}


/*
 * Detach
 * Free resources allocated in samst_attach
 * Note: If you implement a timeout routine in this driver, cancel it
 *	 here. Note that scsi_init_pkt is called with SLEEP_FUNC, so it
 *	 will wait for resources if non are available. Changing it to
 *	 a callback constitutes a timeout; however this detach routine
 *	 will be called only if the driver is not open, and there should be
 *	 no outstanding scsi_init_pkt's if we're closed.
 */
static int
samst_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	int		instance;
	struct samst_soft *tgt;
	struct diskhd  *dp;

	instance = ddi_get_instance(dip);
	tgt = ddi_get_soft_state(samst_state, instance);
	dp = &tgt->targ_iorq;

	switch (cmd) {
	case DDI_DETACH:
		return (samst_dodetach(dip));

	case DDI_SUSPEND:
		/*
		 * to process DDI_SUSPEND, we must do the following:
		 *
		 * - wait till outstanding operations completed and block new
		 * operations - cancel pending timeouts
		 *
		 * In this driver there is no hardware state to save.
		 *
		 * If a callback is outstanding which cannot be cancelled then
		 * either wait for the callback to complete or fail the
		 * suspend
		 *
		 * Note that there are two flags, suspend and pm_suspend for
		 * suspend/resume and power management, respectively
		 */
		mutex_enter(SAMST_MUTEX(tgt));

		/* this shouldn't happen */
		if (tgt->suspended) {
			mutex_exit(SAMST_MUTEX(tgt));
			return (DDI_SUCCESS);
		}
		/* stop new operations */
		tgt->suspended = 1;

		/*
		 * is the device already suspended thru DDI_PM_SUSPEND?
		 *
		 * A successful DDI_PM_SUSPEND ensures that the device isn't
		 * active and no timeouts are outstanding
		 */
		if (tgt->pm_suspended) {
			mutex_exit(SAMST_MUTEX(tgt));
			return (DDI_SUCCESS);
		}
		/*
		 * wait till current operation completed. If we are in the
		 * resource wait state (with a callback outstanding) then we
		 * need to wait till the callback completes and starts the
		 * next cmd. Note that if the request were never to be
		 * satisfied, we would hang forever
		 */
		while (dp->b_forw || (tgt->targ_state == SAMST_STATE_RWAIT)) {
			cv_wait(&tgt->disk_busy_cv, SAMST_MUTEX(tgt));
		}

		/* cancel outstanding timeouts, there shouldn't be any */
		if (tgt->targ_timeout_ret) {
			sam_timeout_id_t tmp_id = tgt->targ_timeout_ret;

			tgt->targ_timeout_ret = 0;
			mutex_exit(SAMST_MUTEX(tgt));
			(void) untimeout(tmp_id);
		} else {
			mutex_exit(SAMST_MUTEX(tgt));
		}

		return (DDI_SUCCESS);

	case DDI_PM_SUSPEND:
		ASSERT(tgt->suspended == 0);

		mutex_enter(SAMST_MUTEX(tgt));

		/*
		 * if the device is busy then we fail the PM_SUSPEND.
		 *
		 * the only reason that timeouts might be outstanding is
		 * that an error recovery is in progress so the device is
		 * really still busy
		 */
		if (dp->b_forw || dp->av_forw ||
		    (tgt->targ_state == SAMST_STATE_RWAIT)) {
			mutex_exit(SAMST_MUTEX(tgt));
			return (DDI_FAILURE);
		}
		tgt->pm_suspended = 1;
		mutex_exit(SAMST_MUTEX(tgt));
		return (DDI_SUCCESS);

	default:
		SAMST_LOG(0, SAMST_CE_DEBUG1,
		    "samst_detach: unsupported cmd 0x%x\n", cmd);
		return (DDI_FAILURE);
	}
}

static int
samst_dodetach(dev_info_t *dip)
{
	int		instance;
	struct scsi_device *devp;
	struct samst_soft *targ;



	devp = (struct scsi_device *)ddi_get_driver_private(dip);
	instance = ddi_get_instance(dip);
	SAMST_LOG(devp, SAMST_CE_DEBUG2, "samst_detach: unit %d\n", instance);

	if ((targ = ddi_get_soft_state(samst_state, instance)) == NULL) {
		SAMST_LOG(devp, CE_WARN, "No Target Struct for samst%d\n",
		    instance);
		return (DDI_FAILURE);
	}
	if (targ->targ_timeout_ret)
		untimeout(targ->targ_timeout_ret);

	/*
	 * Remove other data structures allocated in samst_attach()
	 */
	cv_destroy(&targ->targ_sbuf_cv);
	if (targ->targ_rqbp)
		scsi_free_consistent_buf(targ->targ_rqbp);

	if (targ->targ_rqs)
		scsi_destroy_pkt(targ->targ_rqs);

	if (targ->targ_sbufp)
		freerbuf(targ->targ_sbufp);

	sema_destroy(&targ->targ_semaphore);
	ddi_soft_state_free(samst_state, instance);
	devp->sd_private = (opaque_t)0;
	devp->sd_sense = (struct scsi_extended_sense *)0;
	ddi_remove_minor_node(dip, NULL);
	scsi_unprobe(devp);
	return (DDI_SUCCESS);
}


/*
 *	Unix Entry Points
 */


/*
 * open(9e)
 * Called for each open(2) call on the device.
 * There are no mutexes in the open or close routines here. Synchronization
 * and locking are done with a semaphore. This serializes open and close
 * calls: if one is in progress, another will block until it completes. If
 * you add access to data that may also be accessed by other than open/close,
 * you may need to add a mutex.
 * If samst_open is called for a non-existent instance, we return ENXIO. This
 * causes the framework to retry the autoconfiguration (including calling
 * samst_probe and samst_attach). This is known as "deferred attach".
 * Note: Credp can be used to restrict access to root, by calling drv_priv(9f);
 *	 see also cred(9s).
 *	 Flag shows the access mode (read/write). Check it if the device is
 *	 read or write only, or has modes where this is relevant.
 *	 Otyp is an open type flag, see open.h.
 */
/*ARGSUSED*/
static int
samst_open(dev_t *dev_p, int flag, int otyp, cred_t *cred_p)
{
	dev_t	   dev = *dev_p;
	struct samst_soft *targ;
	int		firstopen;
	char	   *msg;

	if ((targ = ddi_get_soft_state(samst_state, SAMST_INST(dev))) ==
	    NULL) {
		SAMST_LOG(0, SAMST_CE_DEBUG1,
		    "open - unattached instance %d\n",
		    SAMST_INST(dev));
		return (ENXIO);	/* causes deferred attach */
	}
	if ((otyp != OTYP_CHR) && (otyp != OTYP_BLK)) {
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG1,
		    "Unsupported open type %d", otyp);
		return (EINVAL);
	}
	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "open - %s\n",
	    (otyp == OTYP_CHR) ? "raw" : "block");

	/*
	 * Decrement semaphore to lock out other open/close calls while this
	 * thread is active (see semaphore(9F)).	 This semaphore is
	 * also used in ioctl processing to retrive the open_count.
	 */
	sema_p(&targ->targ_semaphore);

	/*
	 * Find out if this is the first open. Don't need the mutex because
	 * this is read only access.
	 */
	firstopen = (targ->targ_open_cnt == 0);

	/*
	 * If this is the first open or the device hasn't been responding,
	 * test to make sure it's still powered on and ready by sending the
	 * SCSI Test Unit Ready command.	If FNDELAY|FNOBLOCK, don't do
	 * test_ready.
	 */
	if (firstopen || (targ->targ_state == SAMST_STATE_NORESP)) {
		if (!(flag & (FNDELAY | FNONBLOCK)) &&
		    !samst_ready(targ, dev)) {
			samst_log(targ->targ_devp, CE_NOTE,
			    "Device not ready(open)\n");
			sema_v(&targ->targ_semaphore);
			return (EIO);
		}
	}
	if (flag & (FNDELAY | FNONBLOCK))
		targ->targ_state = SAMST_STATE_NIL;	/* make it look good */

	/*
	 * Check for idle device.	 Return EBUSY if the device is idle,
	 * FNDELAY is not set and not root.
	 */
	if ((targ->targ_flags & SAMST_FLAGS_IDLE) &&
	    !(flag & (FNDELAY | FNONBLOCK)) &&
	    drv_priv(cred_p)) {
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG1, "device idle.\n");
		sema_v(&targ->targ_semaphore);
		return (EBUSY);
	}
	/*
	 * Check for exclusive open - exclusivity affects the whole disk.
	 */
	if (flag & FEXCL) {
		if (!firstopen) {
			SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG1,
			    "(excl) unit already open\n");
			sema_v(&targ->targ_semaphore);
			return (EBUSY);
		}
		targ->targ_excl = 1;
	} else if (targ->targ_excl) {
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG1,
		    "already opened exclusively\n");
		sema_v(&targ->targ_semaphore);
		return (EBUSY);
	}
	targ->targ_open_cnt++;	/* increment open count */
	sema_v(&targ->targ_semaphore);
	return (0);
}


/*
 * close(9e)
 * Called on final close only, i.e. the last close(2) call.
 */
/*ARGSUSED*/
static int
samst_close(dev_t dev, int flag, int otyp, cred_t *cred_p)
{
	struct samst_soft *targ;

	if ((targ = ddi_get_soft_state(samst_state, SAMST_INST(dev))) ==
	    NULL) {
		SAMST_LOG(0, SAMST_CE_DEBUG1, "samst_close bad instance %d\n",
		    SAMST_INST(dev));
		return (ENXIO);
	}
	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "close - %s\n",
	    (otyp == OTYP_CHR) ? "raw" : "block");

	sema_p(&targ->targ_semaphore);

	targ->targ_excl = 0;	/* turn off exclusivity */
	targ->targ_open_cnt = 0; /* clear open count */
	sema_v(&targ->targ_semaphore);
	return (0);
}


/*
 * Device Configuration Routine
 * link instance number (unit) with dev_info structure
 */
/*ARGSUSED*/
static int
samst_info(dev_info_t *dip, ddi_info_cmd_t infocmd, void *arg, void **result)
{
	struct samst_soft *targ;
	int		instance, error;

	SAMST_LOG(0, SAMST_CE_DEBUG2, "samst_info\n");

	switch (infocmd) {
	case DDI_INFO_DEVT2DEVINFO:
		instance = SAMST_INST((dev_t)arg);
		if ((targ = ddi_get_soft_state(samst_state, instance)) == NULL)
			return (DDI_FAILURE);

		*result = (void *) targ->targ_devp->sd_dev;
		error = DDI_SUCCESS;
		break;

	case DDI_INFO_DEVT2INSTANCE:
		*result = (void *) SAMST_INST((dev_t)arg);
		error = DDI_SUCCESS;
		break;

	default:
		error = DDI_FAILURE;
	}
	return (error);
}


/*
 * Character (raw) read and write routines, called via read(2) and
 * write(2). These routines perform "raw" (i.e. unbuffered) i/o.
 * Since they're so similar, there's actually one 'rw' routine for both,
 * these devops entry points just call the general routine with the
 * appropriate flag.
 */
/* ARGSUSED2 */
static int
samst_read(dev_t dev, struct uio *uio, cred_t *cred_p)
{
	return (samst_rw(dev, uio, B_READ));
}

/* ARGSUSED2 */
static int
samst_write(dev_t dev, struct uio *uio, cred_t *cred_p)
{
	return (samst_rw(dev, uio, B_WRITE));
}

/*
 * General raw read/write routine
 * Just verify the unit number and transfer offset & length, and call
 * strategy via physio. Physio(9f) will take care of address mapping
 * and locking, and will split the transfer if ncessary, based on minphys,
 * possibly calling the strategy routine multiple times.
 */
static int
samst_rw(dev_t dev, struct uio *uio, int flag)
{
	struct samst_soft *targ;

	if ((targ = ddi_get_soft_state(samst_state, SAMST_INST(dev))) == NULL)
		return (ENXIO);

	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "%s at offset %x for %x\n",
	    (flag == B_READ) ? "Read" : "Write",
	    uio->uio_offset, uio->uio_iov->iov_len);

	/*
	 * Verify the offset/length of the request - disk drives will accept
	 * only multiples of sectors.
	 */
	if (uio->uio_offset & (targ->targ_dev_bsize - 1)) {
		samst_log(targ->targ_devp, CE_CONT,
		    "Offset %d is not an integral number of sectors\n",
		    uio->uio_offset);
		return (EINVAL);
	} else if (uio->uio_iov->iov_len & (targ->targ_dev_bsize - 1)) {
		samst_log(targ->targ_devp, CE_CONT,
		"Transfer length %d is not an integral number of sectors\n",
		    uio->uio_iov->iov_len);
		return (EINVAL);
	}
	return (physio(samst_strategy, NULL, dev, flag, minphys, uio));
}


/*
 * strategy
 * Main routine for commands to the device.	 Called from samst_rw(),
 * samst_ioctl_cmd, and directly from the file system.
 * The mutex prevents this routine from being called simultaneously by
 * two threads.
 */
static int
samst_strategy(struct buf *bp)
{
	struct samst_soft *targ;
	struct diskhd  *dp;

	if ((targ = ddi_get_soft_state(samst_state, SAMST_INST(bp->b_edev)))
	    == NULL) {
		bp->b_resid = bp->b_bcount;
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return (0);
	}
	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "samst_strategy - %s\n",
	    (bp == targ->targ_sbufp) ? "special" : "regular");

	if (bp != targ->targ_sbufp) {	/* Regular read/write */
		daddr_t	 bn = bp->b_blkno;
		/*
		 * First check that the device is still there
		 */
		if (targ->targ_state == SAMST_STATE_NORESP) {
			if (!samst_ready(targ, bp->b_edev)) {
				bp->b_resid = bp->b_bcount;
				bp->b_error = ENXIO;
				bp->b_flags |= B_ERROR;
				biodone(bp);
				return (0);
			}
		}
		/*
		 * If write request, check for block in range
		 */
		if (bp->b_flags & B_WRITE) {
			daddr_t	 blkno, end_blk;

			/*
			 * requests always come is as 512 blocks, shift
			 * appropriately
			 */
			blkno = bp->b_blkno >> (targ->targ_dev_bshift - 9);
			end_blk = blkno +
			    (bp->b_bcount >> targ->targ_dev_bshift);
			SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG4,
			    "WC1 %x-%x:%x(%x):%x-%x.\n",
			    targ->targ_low_write, targ->targ_high_write, bn,
			    bp->b_flags, blkno, end_blk);
			if (blkno < targ->targ_low_write ||
			    end_blk > targ->targ_high_write ||
			    targ->targ_low_write == SAMST_RANGE_NOWRITE) {
				samst_log(targ->targ_devp, CE_CONT,
				    "Permitted write range %x to %x.",
				    targ->targ_low_write,
				    targ->targ_high_write);
				samst_log(targ->targ_devp, CE_CONT,
				    "Attempted write range %x to %x.",
				    blkno, end_blk);
				bp->b_resid = bp->b_bcount;
				bp->b_error = ENOSPC;
				bp->b_flags |= B_ERROR;
				biodone(bp);
				return (0);
			}
		}
		bp->b_resid = bn;
	} else {
		/*
		 * Special command, put it at the front of the queue
		 */
		bp->b_resid = 0;
	}

	/*
	 * Add the new request into the i/o queue, pointed to by
	 * targ->targ_iorq. The queue is chained through av_forw and
	 * dp->b_forw holds the currently active request. Requests are added
	 * to the queue by disksort(9F), which uses the value in b_resid as
	 * the sort key. I'm using the physical block number here, although
	 * the cylinder number can also be used.
	 */
	dp = &targ->targ_iorq;	/* i/o request queue for this device */

	bp->av_forw = NULL;
	bp->av_back = NULL;	/* av_back is BP_PKT */

	/*
	 * LOCK the targ struct so we can fiddle with the queue pointers. We
	 * need to hold it for a bit because we don't want anyone to change
	 * dp->b_forw or dp->av_forw while we're accessing them.
	 */
	mutex_enter(SAMST_MUTEX(targ));

	disksort(dp, bp);	/* add request into queue */

	if (dp->b_forw == NULL) {	/* this device inactive? */
		mutex_exit(SAMST_MUTEX(targ));
		samst_start(targ);
	} else if (BP_PKT(dp->av_forw) == 0) {
		/*
		 * There's an active request, so samst_done will call
		 * samst_start when it's completed. Try to pre-allocate
		 * resources for the next request in line. Don't care if it
		 * fails.
		 */
		samst_make_cmd(targ, dp->av_forw, NULL_FUNC);
		mutex_exit(SAMST_MUTEX(targ));
	} else
		mutex_exit(SAMST_MUTEX(targ));

	return (0);
}


/*
 * ioctl calls.
 * Ioctls are device specific. This driver supports the SAM-FS ioctl's
 * and the USCSI (CDB pass through) command.
 */
/* ARGSUSED3 */
static int
samst_ioctl(dev_t dev, int cmd, intptr_t arg, int mode, cred_t *cred_p,
	    int *rval_p)
{
	int		err = 0, len;
	int		tmpi, priv;
	uint_t	  attn[2];
	struct uscsi_cmd uscsi;
	struct scsi_device *devp;
	struct samst_soft *targ;
	struct dk_cinfo *ctlr;

	if ((targ = ddi_get_soft_state(samst_state, SAMST_INST(dev))) == NULL)
		return (ENXIO);	/* invalid minor number */

	devp = targ->targ_devp;
	bzero((caddr_t)& uscsi, sizeof (struct uscsi_cmd));
	SAMST_LOG(devp, SAMST_CE_DEBUG2, "samst_ioctl: cmd = 0x%x\n", cmd);

	priv = drv_priv(cred_p); /* get privliged status */
	switch (cmd) {

	case SAMSTIOC_READY:
		SAMST_LOG(devp, SAMST_CE_DEBUG2,
		    "samst_ioctl: cmd = SAMSTIOC_READY\n");
		tmpi = samst_ready(targ, dev);
		if (ddi_copyout((caddr_t)& tmpi, (caddr_t)arg,
		    sizeof (int), mode))
			err = EFAULT;
		break;

	case SAMSTIOC_ERRLEV:
		SAMST_LOG(devp, SAMST_CE_DEBUG2,
		    "samst_ioctl: cmd = SAMSTIOC_ERRLEV\n");
		/*
		 * Check root permissions
		 */
		if (priv)
			return (EPERM);

		if (ddi_copyin((caddr_t)arg, (caddr_t)& tmpi,
		    sizeof (int), mode))
			return (EFAULT);

		samst_debug = tmpi;	/* set new debug level */
		break;

	case SAMSTIOC_SETBLK:
		SAMST_LOG(devp, SAMST_CE_DEBUG2,
		    "samst_ioctl: cmd = SAMSTIOC_SETBLK\n");
		/*
		 * Check root permissions
		 */
		if (priv)
			return (EPERM);

		if (ddi_copyin((caddr_t)arg, (caddr_t)& tmpi,
		    sizeof (int), mode))
			return (EFAULT);

		/* block size has to be between 512 and 16K */
		if (tmpi >= 512 && tmpi <= 16384) {
			uint_t	  b_shift;

			/*
			 * work up the powers of 2 to find the bit shift to
			 * match block size
			 */
			for (b_shift = 1;
			    (tmpi ^ (1 << b_shift)) && b_shift < 30;
			    b_shift++) {
			}
			/* if it was a valid block size */
			if (b_shift != 30) {
				SAMST_LOG(devp, SAMST_CE_DEBUG1,
				    "Setblocksize %x\n", tmpi);
				targ->targ_dev_bsize = tmpi;
				targ->targ_dev_bshift = b_shift;
			} else {
				samst_log(devp, CE_CONT,
				    "SETBLK block size %d is not valid\n",
				    tmpi);
				err = EINVAL;
			}
		} else {
			samst_log(devp, CE_CONT,
			    "SETBLK block size %d is not valid\n", tmpi);
			err = EINVAL;
		}
		break;

#ifdef SAMSTIOC_GETBLK
	case SAMSTIOC_GETBLK:
		SAMST_LOG(devp, SAMST_CE_DEBUG2,
		    "samst_ioctl: cmd = SAMSTIOC_GETBLK\n");

		sema_p(&targ->targ_semaphore);
		tmpi = targ->targ_dev_bsize;
		sema_v(&targ->targ_semaphore);
		if (ddi_copyout((caddr_t)& tmpi, (caddr_t)arg,
		    sizeof (int), mode))
			err = EFAULT;

		break;

#endif

	case USCSICMD:
		SAMST_LOG(devp, SAMST_CE_DEBUG2,
		    "samst_ioctl: cmd = USCSICMD\n");
		/*
		 * Check root permissions
		 */
		if (priv)
			return (EPERM);

		SAMST_LOG(devp, SAMST_CE_DEBUG3, "USCSICMD\n");
		/*
		 * Send an arbitrary SCSI command to the device. The user has
		 * specified the CDB, and and address/length for data (if
		 * needed). We do no validation of the passed uscsi_cmd
		 * structure (defined in <sys/scsi/impl/uscsi.h>).
		 *
		 */
		switch (ddi_model_convert_from(mode & FMODELS)) {
		case DDI_MODEL_ILP32: {
			struct uscsi_cmd32 uscsi32;
			struct uscsi_cmd32 *u32 = &uscsi32;
			struct uscsi_cmd *ucmd = &uscsi;

			if (ddi_copyin((caddr_t)arg, (caddr_t)& uscsi32,
			    sizeof (struct uscsi_cmd32), mode))
				return (EFAULT);
			uscsi_cmd32touscsi_cmd(u32, ucmd);
			}
			break;

		case DDI_MODEL_NONE:
			if (ddi_copyin((caddr_t)arg, (caddr_t)& uscsi,
			    sizeof (struct uscsi_cmd), mode))
				return (EFAULT);

			break;
		}
		err = samst_ioctl_cmd(dev, &uscsi, UIO_USERSPACE,
		    UIO_USERSPACE, mode);

		switch (ddi_model_convert_from(mode & FMODELS)) {
		case DDI_MODEL_ILP32: {
			struct uscsi_cmd32 uscsi32;
			struct uscsi_cmd32 *u32 = &uscsi32;
			struct uscsi_cmd *ucmd = &uscsi;

			uscsi_cmdtouscsi_cmd32(ucmd, u32);
			if (ddi_copyout((caddr_t)& uscsi32, (caddr_t)arg,
			    sizeof (struct uscsi_cmd32), mode))
				return (EFAULT);
			}
			break;

		case DDI_MODEL_NONE:
			if (ddi_copyout((caddr_t)& uscsi, (caddr_t)arg,
			    sizeof (struct uscsi_cmd), mode))
				return (EFAULT);

			break;
		}
		break;

		/*
		 * Idle status is used to prevent more open calls.  Only
		 * opens with NDELAY set will be allowd.  Understand that
		 * some other part of the sam system could unload a drive
		 * from under you if you open with FNDELAY.	This flag is
		 * cleared in ioctl processing if the command issued is a
		 * start/stop with the eject bit set and the start bit
		 * cleared.  Returns open count throught the parameter.
		 */
	case SAMSTIOC_IDLE:	/* set/clear idle status */
		SAMST_LOG(devp, SAMST_CE_DEBUG2,
		    "samst_ioctl: cmd = SAMSTIOC_IDLE\n");
		/*
		 * check permissions
		 */
		if (priv)
			return (EPERM);

		if (ddi_copyin((caddr_t)arg, (caddr_t)& tmpi,
		    sizeof (int), mode))
			return (EFAULT);

		sema_p(&targ->targ_semaphore);	/* lock out open/close */
		mutex_enter(SAMST_MUTEX(targ));	/* lock targ struct */

		/* set/clear idle flag */
		targ->targ_flags = tmpi ?
		    (targ->targ_flags | SAMST_FLAGS_IDLE) :
		    (targ->targ_flags & ~SAMST_FLAGS_IDLE);

		tmpi = targ->targ_open_cnt;
		mutex_exit(SAMST_MUTEX(targ));	/* free targ struct */
		sema_v(&targ->targ_semaphore);	/* free open/close */
		if (ddi_copyout((caddr_t)& tmpi, (caddr_t)arg,
		    sizeof (int), mode))
			err = EFAULT;
		break;


		/*
		 * Since open/close processing manipulate the open_cnt use
		 * their semaphore to insure a good value.
		 */
	case SAMSTIOC_GETOPEN:	/* return open count */
		SAMST_LOG(devp, SAMST_CE_DEBUG2,
		    "samst_ioctl: cmd = SAMSTIOC_GETOPEN\n");
		sema_p(&targ->targ_semaphore);
		tmpi = targ->targ_open_cnt;
		sema_v(&targ->targ_semaphore);
		if (ddi_copyout((caddr_t)& tmpi, (caddr_t)arg,
		    sizeof (int), mode))
			err = EFAULT;

		break;

	case SAMSTIOC_ATTN:
		SAMST_LOG(devp, SAMST_CE_DEBUG2,
		    "samst_ioctl: cmd = SAMSTIOC_ATTN\n");
		/*
		 * Since open calls samst_ready, we must lock open processing
		 * while we get and clear the flag.
		 */
		sema_p(&targ->targ_semaphore);	/* lock out open/close */
		mutex_enter(SAMST_MUTEX(targ));	/* lock targ struct */

		/* get current value */
		attn[0] = targ->targ_flags & SAMST_FLAGS_ATTN;
		attn[1] = ((targ->targ_add_sense << 8) |
		    (targ->targ_sense_qual));
		targ->targ_flags &= ~SAMST_FLAGS_ATTN;	/* clear the  flag */

		mutex_exit(SAMST_MUTEX(targ));	/* free targ struct */
		sema_v(&targ->targ_semaphore);	/* free open/close */

		if (ddi_copyout((caddr_t)& attn, (caddr_t)arg,
		    sizeof (attn), mode))
			err = EFAULT;

		break;

	case SAMSTIOC_RANGE:	/* set write range */
		SAMST_LOG(devp, SAMST_CE_DEBUG2,
		    "samst_ioctl: cmd = SAMSTIOC_RANGE\n");
		{
			samst_range_t   ranges;
			/* check permissions */
			if (priv)
				return (EPERM);

			if (ddi_copyin((caddr_t)arg, (caddr_t)& ranges,
			    sizeof (ranges), mode))
				return (EFAULT);

			if (ranges.low_bn > ranges.high_bn)
				return (EINVAL);

			mutex_enter(SAMST_MUTEX(targ));	/* lock targ struct */
			targ->targ_low_write = ranges.low_bn;
			targ->targ_high_write = ranges.high_bn;
			SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG1,
			    "setting range(%x:%x).\n",
			    targ->targ_low_write, targ->targ_high_write);
			mutex_exit(SAMST_MUTEX(targ));	/* free targ struct */
		}
		break;

	default:
		err = ENOTTY;
		break;
	}
	return (err);
}


/*
 * Run a command for user (from samst_ioctl) or from someone else in the
 * driver.
 *
 * cdbspace is for address space of cdb; dataspace is for address space
 * of the buffer - user or kernel.
 */
static int
samst_ioctl_cmd(dev_t dev, struct uscsi_cmd *scmd,
		enum uio_seg cdbspace, enum uio_seg dataspace, int mode)
{
	int		err, rw;
	int		flag;
	caddr_t	 cdb, user_cdbp;
	struct buf	*bp;
	struct scsi_pkt *pkt;
	struct samst_soft *targ;

	if ((targ = ddi_get_soft_state(samst_state, SAMST_INST(dev))) ==
	    NULL) {
		SAMST_LOG(0, SAMST_CE_DEBUG1,
		"samst_ioctl_cmd: invalid instance %d\n", SAMST_INST(dev));
		return (ENXIO);
	}
	ASSERT(mutex_owned(SAMST_MUTEX(targ)) == 0);

	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "samst_ioctl_cmd\n");

	/*
	 * Is this a request to reset the bus? If so, we need go no further.
	 */
	if (scmd->uscsi_flags & (USCSI_RESET | USCSI_RESET_ALL)) {
		flag = ((scmd->uscsi_flags & USCSI_RESET_ALL)) ?
		    RESET_ALL : RESET_TARGET;

		err = (scsi_reset(ROUTE(targ), flag)) ? 0 : EIO;

		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "reset %s %s\n",
		    (flag == RESET_ALL) ? "all" : "target",
		    (err == 0) ? "ok" : "failed");
		return (err);
	}
	/*
	 * The uscsi structure itself is already in kernel space (copied in
	 * by samst_ioctl, or declared there by our caller); but we need to
	 * copy in the cdb here.
	 */
	cdb = kmem_zalloc((size_t)scmd->uscsi_cdblen, KM_SLEEP);
	if (cdbspace == UIO_SYSSPACE)
		bcopy(scmd->uscsi_cdb, cdb, scmd->uscsi_cdblen);

	else {
		if (ddi_copyin(scmd->uscsi_cdb, cdb,
		    (uint_t)scmd->uscsi_cdblen, mode)) {
			kmem_free(cdb, (size_t)scmd->uscsi_cdblen);
			SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG1,
			    "samst_ioctl_cmd: copyin failed\n");
			return (EFAULT);
		}
	}

	/*
	 * The cdb pointer in the structure passed by the user is pointing to
	 * user space. We've just copied the cdb into a local buffer, so
	 * point uscsi_cdb to it now. We'll restore the user's pointer at the
	 * end.
	 */
	user_cdbp = scmd->uscsi_cdb;	/* save the user's pointer */
	scmd->uscsi_cdb = cdb;	/* point to the local cdb buffer */
	rw = (scmd->uscsi_flags & USCSI_READ) ? B_READ : B_WRITE;

	/*
	 * Get the 'special command' buffer. First lock the targ struct; if
	 * the buffer's busy, wait for it to become free. Once we get the
	 * buffer, mark it busy to lock out other requests for it. Note
	 * cv_wait will release the mutex, allowing other parts of the driver
	 * to acquire it. Note	Once we have the special buffer, we can
	 * safely release the mutex; the buffer is now busy and another
	 * thread will block in the cv_wait until we release it. All the code
	 * from here until we unset the busy flag is non-reentrant, including
	 * the physio/strategy/start/callback/done thread.
	 */
	mutex_enter(SAMST_MUTEX(targ));
	while (targ->targ_sbuf_busy)
		cv_wait(&targ->targ_sbuf_cv, SAMST_MUTEX(targ));
	targ->targ_sbuf_busy = 1;

	bp = targ->targ_sbufp;
	mutex_exit(SAMST_MUTEX(targ));

	if (scmd->uscsi_buflen) {
		/*
		 * We're sending/receiving data; create a uio structure and
		 * call physio to do the right things.
		 */
		struct iovec    aiov;
		struct uio	auio;
		struct uio	*uio = &auio;

		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
		    "Data transfer (%d bytes)\n", scmd->uscsi_buflen);

		bzero((caddr_t)& auio, sizeof (struct uio));
		bzero((caddr_t)& aiov, sizeof (struct iovec));
		aiov.iov_base = scmd->uscsi_bufaddr;
		aiov.iov_len = scmd->uscsi_buflen;
		uio->uio_iov = &aiov;

		uio->uio_iovcnt = 1;
		uio->uio_resid = scmd->uscsi_buflen;
		uio->uio_segflg = dataspace;
		uio->uio_offset = 0;
		uio->uio_fmode = 0;

		/*
		 * Let physio do the rest...
		 */
		bp->av_back = NULL;	/* av_back is BP_PKT */
		bp->b_forw = (struct buf *)scmd;
		err = physio(samst_strategy, bp, dev, rw, minphys, uio);
		/*
		 * Copy resid to the user scsi request
		 */
		scmd->uscsi_resid = bp->b_resid;
	} else {
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "No data\n");
		/*
		 * No data transfer, we can call samst_strategy directly
		 */
		bp->av_back = NULL;	/* av_back is BP_PKT */
		bp->b_forw = (struct buf *)scmd;
		bp->b_flags = B_BUSY | rw;
		bp->b_edev = dev;
		bp->b_bcount = bp->b_blkno = 0;
		(void) samst_strategy(bp);
		err = biowait(bp);
	}

	/*
	 * get the status block, if any, and release any resources that we
	 * had.
	 */

	scmd->uscsi_status = 0;
	if ((pkt = BP_PKT(bp)) != NULL) {
		scmd->uscsi_status = SCBP_C(pkt);
	}
	/*
	 * If err and rqsense, copy back sense data to the user
	 */

	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "uscsi rqlen = %d",
	    scmd->uscsi_rqlen);
	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "err - %d, status - %d",
	    err, scmd->uscsi_status);

	if (err && scmd->uscsi_status == STATUS_CHECK && scmd->uscsi_rqlen) {
		int		len = (scmd->uscsi_rqlen > SENSE_LENGTH) ?
		    SENSE_LENGTH : scmd->uscsi_rqlen;

		if (copyout((caddr_t)targ->targ_devp->sd_sense,
		    (caddr_t)scmd->uscsi_rqbuf, len)) {
			SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
			    "Can't copy sense back to the user");
		} else {
			char	    sbuf[3 * SENSE_LENGTH + 1] = "";
			char	   *cp = sbuf;
			int		i;

			for (i = 0; i < len; i++) {
				cp = sbuf + strlen(sbuf);
				sprintf(cp, "%x ",
				    ((char *)(targ->targ_devp->sd_sense))[i]
				    & 0xff);
			}
			SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
			    "sense copied to user: %s", sbuf);
		}
	}
	if (pkt != NULL) {
		scsi_destroy_pkt(pkt);
		bp->av_back = NULL;	/* av_back is BP_PKT */
	}
	/*
	 * Tell anybody who cares that the special buffer is now available.
	 * Again, lock the targ struct first, then mark the buffer not busy
	 * and signal anyone who may be waiting for it. At this point, check
	 * the command just issued.	If its a start/stop with LoEJ set and
	 * start clear, clear the idle flag in the targ struct.
	 */
	mutex_enter(SAMST_MUTEX(targ));	/* LOCK the targ struct */
	targ->targ_sbuf_busy = 0;
	cv_signal(&targ->targ_sbuf_cv);
	if ((GETCMD((union scsi_cdb *)scmd->uscsi_cdb) == SCMD_START_STOP) &&
	((((union scsi_cdb *)(scmd->uscsi_cdb))->g0_count0 & 0x3) == 0x2)) {
		targ->targ_flags &= ~SAMST_FLAGS_IDLE;
		targ->targ_open_cnt = 1;	/* force the open count */
	}
	mutex_exit(SAMST_MUTEX(targ));	/* UNLOCK the targ struct */

	kmem_free(scmd->uscsi_cdb, (size_t)scmd->uscsi_cdblen);
	scmd->uscsi_cdb = user_cdbp;	/* restore the user's pointer */
	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
	    "samst_ioctl_cmd returning 0x%x\n", err);
	return (err);
}


/*
 * Unit start and Completion
 * Get the next command from the queue, allocate resources if they
 * haven't already been allocated, and send off the command.
 */
static void
samst_start(struct samst_soft *targ)
{
	struct buf	*bp;
	struct diskhd  *dp;

	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "samst_start\n");

	dp = &targ->targ_iorq;

	/*
	 * LOCK the targ struct, we're going to fiddle with the queue
	 * pointers
	 */
	mutex_enter(SAMST_MUTEX(targ));

	/*
	 * If b_forw is non-NULL there's an active request already, if
	 * av_forw is NULL there are no requests queued. In either case do
	 * nothing.
	 */
	if (dp->b_forw || (bp = dp->av_forw) == NULL) {
		mutex_exit(SAMST_MUTEX(targ));
		return;
	}
	/*
	 * If resources have not been allocated yet, allocate them now.
	 * Unlike the 'pre-allocation' calls, specify samst_runout here; if
	 * the allocation fails samst_runout will be called to retry later.
	 */
	if (!BP_PKT(bp)) {
		bp->b_error = 0;
		samst_make_cmd(targ, bp, samst_runout);
		if (!BP_PKT(bp)) {
			if (bp->b_error) {
				/*
				 * scsi_init_pkt failed for other than out of
				 * resources. Mark error and pull the request
				 * off the pending queue for samst_done.
				 */
				bp->b_flags |= B_ERROR;
				bp->b_resid = bp->b_bcount;
				dp->b_forw = bp;
				dp->av_forw = bp->av_forw;
				mutex_exit(SAMST_MUTEX(targ));
				samst_done(targ, bp);
				return;
			}
			targ->targ_state = SAMST_STATE_RWAIT;
			mutex_exit(SAMST_MUTEX(targ));
			return;
		} else if (targ->targ_state == SAMST_STATE_RWAIT)
			targ->targ_state = SAMST_STATE_NIL;

	}
	dp->b_forw = bp;	/* new active request */
	dp->av_forw = bp->av_forw;	/* take it off the pending queue */

	mutex_exit(SAMST_MUTEX(targ));	/* UNLOCK the targ structure */

	bp->av_forw = 0;

	/*
	 * Clear the resid now. Must do it after samst_make_cmd, else if that
	 * fails and we get held in RWAIT, the next call to disksort (from
	 * another thread) will use our (zero) b_resid as the sort key, which
	 * would be wrong.
	 */
	bp->b_resid = 0;

	/*
	 * Send the SCSI command to the host adapter
	 */
	if (scsi_transport(BP_PKT(bp)) != TRAN_ACCEPT) {
		samst_log(targ->targ_devp, CE_WARN,
		    "Command transport failed\n");
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		samst_done(targ, bp);
	} else {
		/*
		 * Command sent OK, try to pre-allocate resources for the
		 * next request, don't care if it fails. Need to reacquire
		 * the mutex to lock the targ struct
		 */
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG3,
		    "Command Transported\n");
		mutex_enter(SAMST_MUTEX(targ));
		if (dp->av_forw && !BP_PKT(dp->av_forw))
			samst_make_cmd(targ, dp->av_forw, NULL_FUNC);

		mutex_exit(SAMST_MUTEX(targ));
	}
}


/*
 * 'Out of resources' callback routine. Passed to scsi_init_pkt(); if
 * the allocation fails, this routine is called when resources may be
 * available.
 * Just call samst_start to retry the resource allocation.
 */
/*ARGSUSED*/
static int
samst_runout(caddr_t arg)
{
	int		rval = 1;
	struct samst_soft *targ = (struct samst_soft *)arg;

	SAMST_LOG(0, SAMST_CE_DEBUG2, "samst_runout\n");

	if (targ->targ_state == SAMST_STATE_RWAIT) {
		samst_start(targ);
		if (targ->targ_state == SAMST_STATE_RWAIT)
			rval = 0;
	}
	return (rval);
}


/*
 * Done with a command.
 * Start the next command and then call biodone() to tell physio or
 * samst_ioctl_cmd that this i/o has completed.
 *
 */
static void
samst_done(struct samst_soft *targ, struct buf *bp)
{
	struct diskhd  *dp;

	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "samst_done\n");

	dp = &targ->targ_iorq;	/* get the queue pointer */
	mutex_enter(SAMST_MUTEX(targ));	/* LOCK the targ struct */

	if (bp == dp->b_forw)
		dp->b_forw = NULL;	/* request is no longer active */

	mutex_exit(SAMST_MUTEX(targ));	/* UNLOCK the targ struct */

	/*
	 * Start the next one before releasing resources on this one
	 */
	samst_start(targ);

	/*
	 * Free resources unless this was a special command (in which case
	 * samst_ioctl_cmd will do the free).
	 */
	if ((bp != targ->targ_sbufp) && BP_PKT(bp))
		scsi_destroy_pkt(BP_PKT(bp));

	/* tell interested parties that the i/o is done */
	biodone(bp);
}


/*
 * Allocate resources for a SCSI command - call scsi_init_pkt to get
 * a packet and allocate DVMA resources, and create the SCSI CDB.
 * If scsi_init_pkt fails, give up silently. If samst_runout was specified
 * as func, the allocation will be retried, if NULL_FUNC was called
 * nothing will happen - the caller will not be notified of the failure.
 */
static void
samst_make_cmd(struct samst_soft *targ, struct buf *bp, int (*func) ())
{
	int		com;
	struct scsi_pkt *pkt;
	struct uscsi_cmd *scmd = (struct uscsi_cmd *)bp->b_forw;
	int		flags, tval;

	/*
	 * Normally, mutexes are used to serialize data structure access.
	 * Here we use it to serialize access to this routine, which is
	 * called from several places in the driver. We could acquire and
	 * drop it in this routine, but since the caller may need the lock
	 * anyway, it's easier just to hold it than to keep dropping and
	 * reacquiring it
	 */
	ASSERT(mutex_owned(SAMST_MUTEX(targ)));

	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
	"samst_make_cmd: block %d, count=%d\n", bp->b_blkno, bp->b_bcount);

	flags = 0;
	if ((scsi_options & SCSI_OPTIONS_DR) == 0)
		flags = FLAG_NODISCON;

	if (bp != targ->targ_sbufp) {
		daddr_t	 blkno;

		/*
		 * convert blkno from units of 512 to whatever the target is
		 * currently running.
		 */

		blkno = bp->b_blkno >> (targ->targ_dev_bshift - 9);
		pkt = scsi_init_pkt(ROUTE(targ), NULL, bp, CDB_GROUP1,
		    targ->targ_arq ? sizeof (struct scsi_arq_status) : 1,
		    0, 0, func, (caddr_t)targ);
		if (pkt == (struct scsi_pkt *)0)
			return;

		/*
		 * Note: I'm using the group 1 Read/Write commands here. If
		 * your device needs different commands for normal data
		 * transfer (e.g. extended read/write), change it here.
		 */
		com = (bp->b_flags & B_READ) ? SCMD_READ_G1 : SCMD_WRITE_G1;
		makecom_g1(pkt, targ->targ_devp, flags, com, blkno,
		    bp->b_bcount >> targ->targ_dev_bshift);
		pkt->pkt_flags = flags;
		tval = samst_io_time;
	} else {
		/*
		 * All special command come through samst_ioctl_cmd, which
		 * uses the uscsi interface. Just need to get the CDB from
		 * scmd and plug it in. Still call makecom_g0 because it
		 * fills in some of the pkt field for us. Its cdb
		 * manipulations will be overwritten by the bcopy.
		 */
		if (scmd->uscsi_flags & USCSI_SILENT)
			flags |= FLAG_SILENT;
		if (scmd->uscsi_flags & USCSI_DIAGNOSE)
			flags |= FLAG_DIAGNOSE;
		if (scmd->uscsi_flags & USCSI_ISOLATE)
			flags |= FLAG_ISOLATE;
		if (scmd->uscsi_flags & USCSI_NODISCON)
			flags |= FLAG_NODISCON;
		pkt = scsi_init_pkt(ROUTE(targ), (struct scsi_pkt *)NULL,
		    (bp->b_bcount) ? bp : NULL, scmd->uscsi_cdblen,
		    targ->targ_arq ? sizeof (struct scsi_arq_status) : 1,
		    0, 0, func, (caddr_t)targ);
		if (pkt == (struct scsi_pkt *)0) {
			bp->av_back = NULL;	/* flag no packet */
			SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG1,
			    "No scsi packets.\n");
			return;
		}
		makecom_g0(pkt, targ->targ_devp, flags, scmd->uscsi_cdb[0],
		    0, 0);
		bcopy(scmd->uscsi_cdb,
		    (caddr_t)pkt->pkt_cdbp, scmd->uscsi_cdblen);
		tval = scmd->uscsi_timeout;
	}

	pkt->pkt_comp = samst_callback;
	pkt->pkt_time = tval;
	pkt->pkt_private = (opaque_t)bp;
	bp->av_back = (struct buf *)pkt;	/* av_back is BP_PKT */

	if (samst_debug > 2)
		hex_print("samst_make_cmd, CDB", (caddr_t)pkt->pkt_cdbp, 6);
}


/*
 * Interrupt Service Routines
 */

/*
 * Restart a command - the device was either busy or not ready
 */
static void
samst_restart(void *arg)
{
	struct samst_soft *targ = (struct samst_soft *)arg;
	struct buf	*bp;

	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "samst_restart\n");

	targ->targ_timeout_ret = 0;	/* clear timeout holder */

	bp = targ->targ_iorq.b_forw;	/* read only, no need to lock */
	if (bp) {
		struct scsi_pkt *pkt = BP_PKT(bp);
		if (!targ->targ_arq && (pkt->pkt_flags & FLAG_SENSING))
			pkt = targ->targ_rqs;

		if (scsi_transport(pkt) != TRAN_ACCEPT) {
			bp->b_resid = bp->b_bcount;
			bp->b_flags |= B_ERROR;
			samst_done(targ, bp);
		}
	}
}


/*
 * Command completion processing, called by the host adapter driver
 * when it's done with the command.
 */
static void
samst_callback(struct scsi_pkt *pkt)
{
	struct samst_soft *targ;
	struct buf	*bp;
	int		action;

	bp = (struct buf *)pkt->pkt_private;
	targ = ddi_get_soft_state(samst_state, SAMST_INST(bp->b_edev));

	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
	    "samst_callback: pkt_reason = 0x%x, pkt_flags = 0x%x\n",
	    pkt->pkt_reason, pkt->pkt_flags);

	/*
	 * This mutex locks the targ struct (as usual!). We're interested in
	 * targ_retry_ct - if we retry, we don't want the count touched by
	 * another thread. Also, if we do a request sense we'll update the
	 * sense data in sd_sense.
	 */
	mutex_enter(SAMST_MUTEX(targ));

	/* if the bp packet is null, reset it to calling packet */
	if (BP_PKT(bp) == NULL)
		bp->av_back = (struct buf *)pkt;

	if (pkt->pkt_reason != CMD_CMPLT) {
		/*
		 * The command did not complete. Retry if possible.
		 */
		action = samst_handle_incomplete(targ, bp);
	} else if (targ->targ_arq && pkt->pkt_state & STATE_ARQ_DONE) {
		/*
		 * The auto-rqsense happened, and the packet has a filled-in
		 * scsi_arq_status structure, pointed to by pkt_scbp.
		 */
		action = samst_handle_arq(pkt, targ, bp);
	} else if (pkt->pkt_flags & FLAG_SENSING) {
		/*
		 * We were running a REQUEST SENSE. Decode the sense data and
		 * decide what to do next.
		 */
		pkt = BP_PKT(bp);	/* get the pkt for the orig command */
		pkt->pkt_flags &= ~FLAG_SENSING;
		action = samst_handle_sense(targ, bp);
	} else {
		/*
		 * Command completed and we're not getting sense. Check for
		 * errors and decide what to do next.
		 */
		action = samst_check_error(targ, bp);
	}
	mutex_exit(SAMST_MUTEX(targ));

	switch (action) {
	case QUE_SENSE:
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "Getting Sense\n");
		/*
		 * LOCK targ struct, there's only one sense packet & data for
		 * the unit - don't want to zero sd_sense while someone's
		 * looking at it!
		 */
		mutex_enter(SAMST_MUTEX(targ));
		pkt->pkt_flags |= FLAG_SENSING;
		targ->targ_rqs->pkt_private = (opaque_t)bp;
#if defined(SAMFS)
		bzero((caddr_t)targ->targ_devp->sd_sense, SAM_SENSE_LENGTH);
#else
		bzero((caddr_t)targ->targ_devp->sd_sense, SENSE_LENGTH);
#endif
		mutex_exit(SAMST_MUTEX(targ));
		if (scsi_transport(targ->targ_rqs) == TRAN_ACCEPT)
			break;

		SAMST_LOG(targ->targ_devp, CE_NOTE,
		    "Request Sense transport failed\n");
		/* FALLTHROUGH */

	case COMMAND_DONE_ERROR:
		bp->b_resid = bp->b_bcount;
		bp->b_flags |= B_ERROR;
		/* FALLTHROUGH */

	case COMMAND_DONE:
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "Command Done\n");
		samst_done(targ, bp);
		break;

	case QUE_COMMAND:
		if ((int)targ->targ_retry_ct++ >= samst_retry_count) {
			SAMST_LOG(targ->targ_devp, CE_WARN,
			    "Out of retries, failing command\n");
			bp->b_resid = bp->b_bcount;
			bp->b_flags |= B_ERROR;
			samst_done(targ, bp);
		} else {
			SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
			    "Retrying Command\n");
			if (scsi_transport(BP_PKT(bp)) != TRAN_ACCEPT) {
				SAMST_LOG(targ->targ_devp, CE_NOTE,
				    "Retry transport failed\n");
				bp->b_resid = bp->b_bcount;
				bp->b_flags |= B_ERROR;
				samst_done(targ, bp);
				return;
			}
		}
		break;

	case JUST_RETURN:
		break;
	}

}


/*
 * Incomplete command handling. Figure out what to do based on
 * how far the command did get.
 */
static int
samst_handle_incomplete(struct samst_soft *targ, struct buf *bp)
{
	int		rval = COMMAND_DONE_ERROR;
	struct scsi_pkt *pkt = BP_PKT(bp);

	/*
	 * Need the lock, we may be doing retries (targ_retry_ct) or looking
	 * at the sense packet (targ_rqs)
	 */
	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
	    "samst_handle_incomplete.\n");

	ASSERT(mutex_owned(SAMST_MUTEX(targ)));

	if (!targ->targ_arq && (pkt->pkt_flags & FLAG_SENSING))
		pkt = targ->targ_rqs;

	/*
	 * The target may still be running the  command, so try and reset it,
	 * to get it into a known state. Note: This is forcible, there may be
	 * a more polite method for your device. WARNING: If the targets
	 * reset fails, we give up, we do not reset the entire bus! Resetting
	 * the bus can have grave effects if the system disk is on it. Just
	 * give up, print a message and let the user decide what to do. If
	 * your device is critical to the system's operation, you can reset
	 * the bus by #define-ing SCSI_BUS_RESET. However this is not
	 * recommended. Note that the bus can also be reset in
	 * samst_check_error().
	 */
	if ((pkt->pkt_statistics &
	    (STAT_BUS_RESET | STAT_DEV_RESET | STAT_ABORTED)) == 0) {
		/*
		 * We can temporarily give up the lock here
		 */
		mutex_exit(SAMST_MUTEX(targ));
		SAMST_LOG(targ->targ_devp, CE_NOTE, "Aborting Command\n");
		if (!(scsi_abort(ROUTE(targ), (struct scsi_pkt *)0))) {
			SAMST_LOG(targ->targ_devp, CE_NOTE,
			    "Resetting Target\n");

			if (!(scsi_reset(ROUTE(targ), RESET_TARGET))) {
#ifdef SCSI_BUS_RESET
				SAMST_LOG(targ->targ_devp, CE_NOTE,
				    "Resetting SCSI Bus\n");
				if (!scsi_reset(ROUTE(targ), RESET_ALL)) {
					samst_log(targ->targ_devp, CE_WARN,
					    "SCSI bus reset failed\n");
				}
#else				/* SCSI_BUS_RESET */
				samst_log(targ->targ_devp, CE_WARN,
				    "Reset Target failed\n");
#endif				/* SCSI_BUS_RESET */
				mutex_enter(SAMST_MUTEX(targ));
				return (COMMAND_DONE_ERROR);
			}
		}
		mutex_enter(SAMST_MUTEX(targ));	/* reacquire the lock */
	}
	/*
	 * If we were running a request sense, try it again if possible. Some
	 * devices can handle retries, others will not.
	 */
	if (pkt->pkt_flags & FLAG_SENSING) {
		if ((int)targ->targ_retry_ct++ < samst_retry_count)
			rval = QUE_SENSE;
	} else if (bp == targ->targ_sbufp && (pkt->pkt_flags & FLAG_ISOLATE))
		rval = COMMAND_DONE_ERROR;
	else
		rval = QUE_COMMAND;	/* retry the command */

	if (pkt->pkt_state == STATE_GOT_BUS && rval == COMMAND_DONE_ERROR) {
		/*
		 * The device is not responding
		 */
		samst_log(targ->targ_devp, CE_WARN, "Device not responding");
		targ->targ_state = SAMST_STATE_NORESP;
	}
	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG1, "Cmd incomplete, %s\n",
	    (rval == COMMAND_DONE_ERROR) ? "giving up" : "retrying");

	return (rval);
}


/*
 * Decode sense data
 */
static int
samst_handle_sense(struct samst_soft *targ, struct buf *bp)
{
	struct diskhd  *dp = &targ->targ_iorq;
	struct scsi_pkt *pkt = BP_PKT(bp), *rqpkt = targ->targ_rqs;
	int		rval = COMMAND_DONE_ERROR;
	int		level, amt;

	/*
	 * Need the lock, we may be doing retries (targ_retry_ct) or looking
	 * at the sense packet (targ_rqs) or data (sd_sense)
	 */
	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "samst_handle_sense\n");

	ASSERT(mutex_owned(SAMST_MUTEX(targ)));

	if (SCBP(rqpkt)->sts_busy) {
		/*
		 * Retry the command unless we're out of retries or the 'fail
		 * on error' flag is set
		 */
		if (pkt && !(pkt->pkt_flags & FLAG_DIAGNOSE) &&
		    (int)targ->targ_retry_ct++ < samst_retry_count) {
			if (dp->b_forw == NULL)
				dp->b_forw = bp;
			targ->targ_timeout_ret = timeout(samst_restart,
			    (caddr_t)targ, SAMST_BSY_TIMEOUT);
			rval = JUST_RETURN;
		}
		SAMST_LOG(targ->targ_devp, CE_NOTE, "Target Busy, %s\n",
		    (rval == JUST_RETURN) ? "restarting" : "giving up");
		return (rval);
	}
	if (SCBP(rqpkt)->sts_chk) {
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
		    "Check Condition on Request Sense!\n");
		return (rval);
	}
#if defined(SAMFS)
	amt = SAM_SENSE_LENGTH - rqpkt->pkt_resid;
#else
	amt = SENSE_LENGTH - rqpkt->pkt_resid;
#endif
	if ((rqpkt->pkt_state & STATE_XFERRED_DATA) == 0 || amt == 0) {
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "no sense data\n");
		return (rval);
	}
	/*
	 * Now, check to see whether we got enough sense data to make any
	 * sense out if it (heh-heh).
	 */
	if (amt < SUN_MIN_SENSE_LENGTH) {
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
		    "not enough sense data\n");
		return (rval);
	}
	if (samst_debug > 2)
#if defined(SAMFS)
		hex_print("samst Sense Data",
		    (caddr_t)targ->targ_devp->sd_sense,
		    SENSE_LENGTH);
#else
		hex_print("samst Sense Data",
		    (caddr_t)targ->targ_devp->sd_sense,
		    SENSE_LENGTH);
#endif

	/*
	 * Decode the sense data Note: I'm only looking at the sense key
	 * here. Most devices have unique additional sense codes &
	 * qualifiers, so it's often more useful to look at them instead.
	 *
	 */
	switch (targ->targ_devp->sd_sense->es_key) {
	case KEY_NOT_READY:
		/*
		 * If we get a not-ready indication, wait a bit and try it
		 * again, unless this is a special command with the 'fail on
		 * error' (FLAG_DIAGNOSE) option set.
		 */
		if ((bp == targ->targ_sbufp) &&
		    (pkt->pkt_flags & FLAG_DIAGNOSE)) {
			rval = COMMAND_DONE_ERROR;
			level = SCSI_ERR_FATAL;
		} else if (pkt &&
		    (int)targ->targ_retry_ct++ < samst_retry_count) {
			if (dp->b_forw == NULL)
				dp->b_forw = bp;
			targ->targ_timeout_ret = timeout(samst_restart,
			    (caddr_t)targ, SAMST_BSY_TIMEOUT);
			rval = JUST_RETURN;
			level = SCSI_ERR_RETRYABLE;
		} else {
			rval = COMMAND_DONE_ERROR;
			level = SCSI_ERR_FATAL;
		}
		break;

	case KEY_ABORTED_COMMAND:
		/* FALLTHRU */
	case KEY_UNIT_ATTENTION:
		rval = QUE_COMMAND;
		level = SCSI_ERR_INFO;
		break;

	case KEY_RECOVERABLE_ERROR:
		/* FALLTHRU */
	case KEY_NO_SENSE:
		rval = COMMAND_DONE;
		level = SCSI_ERR_RECOVERED;
		break;

	case KEY_BLANK_CHECK:
		rval = COMMAND_DONE_ERROR;
		level = SCSI_ERR_INFO;
		break;

	case KEY_HARDWARE_ERROR:
		/* FALLTHRU */
	case KEY_MEDIUM_ERROR:
		/* FALLTHRU */
	case KEY_MISCOMPARE:
		/* FALLTHRU */
	case KEY_VOLUME_OVERFLOW:
		/* FALLTHRU */
	case KEY_WRITE_PROTECT:
		/* FALLTHRU */
	case KEY_ILLEGAL_REQUEST:
		/* FALLTHRU */
	default:
		rval = COMMAND_DONE_ERROR;
		level = SCSI_ERR_FATAL;
		break;
	}

	/*
	 * If this was for a special command, check the options
	 */
	if (bp == targ->targ_sbufp) {
		if ((rval == QUE_COMMAND) && (pkt->pkt_flags & FLAG_DIAGNOSE))
			rval = COMMAND_DONE_ERROR;

		if (((pkt->pkt_flags & FLAG_SILENT) == 0) || samst_debug) {
			/*
			 * Report the error. Note: the zero is the error
			 * block number. You may want to fill it in from the
			 * sense data for appropriate errors.
			 */
			scsi_errmsg(targ->targ_devp, pkt, "samst", level,
			    bp->b_blkno, 0, samst_cmds,
			    targ->targ_devp->sd_sense);
		}
	} else if ((level >= samst_error_reporting) || samst_debug) {
		scsi_errmsg(targ->targ_devp, pkt, "samst", level,
		    bp->b_blkno, 0, samst_cmds,
		    targ->targ_devp->sd_sense);
	}
	return (rval);
}


/*
 * Decode auto-rqsense data
 */
static int
samst_handle_arq(struct scsi_pkt *pktp, struct samst_soft *targ,
		struct buf *bp)
{
	int		rval = COMMAND_DONE_ERROR;
	struct diskhd  *dp = &targ->targ_iorq;
	int		level, amt;

	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
	    "Auto Request Sense done\n");

	if (ARQP(pktp)->sts_rqpkt_status.sts_chk) {
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG1,
		    "Check Condition on Auto Request Sense!\n");
		return (rval);
	} else if (ARQP(pktp)->sts_rqpkt_reason != CMD_CMPLT) {
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG1,
		    "Auto Request Sense command incomplete\n");
		return (QUE_COMMAND);	/* retry original command */
	}
	amt = SENSE_LENGTH - ARQP(pktp)->sts_rqpkt_resid;
	if ((ARQP(pktp)->sts_rqpkt_state & STATE_XFERRED_DATA) == 0 ||
	    amt == 0) {
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
		    "no auto sense data\n");
		return (rval);
	}
	/*
	 * Stuff the sense data pointer into sd_sense
	 */
	targ->targ_devp->sd_sense = &(ARQP(pktp)->sts_sensedata);

	/*
	 * Now, check to see whether we got enough sense data to make any
	 * sense out if it (heh-heh).
	 */
	if (amt < SUN_MIN_SENSE_LENGTH) {
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
		    "not enough auto sense data\n");
		return (rval);
	}
	if (samst_debug > 2)
		hex_print("samst Auto Sense Data",
		    (caddr_t)& (ARQP(pktp)->sts_sensedata), SENSE_LENGTH);

	/*
	 * Decode the sense data Note: I'm only looking at the sense key
	 * here. Most devices have unique additional sense codes &
	 * qualifiers, so it's often more useful to look at them instead.
	 *
	 */
	switch (ARQP(pktp)->sts_sensedata.es_key) {
	case KEY_NOT_READY:
		/*
		 * If we get a not-ready indication, wait a bit and try it
		 * again, unless this is a special command with the 'fail on
		 * error' (FLAG_DIAGNOSE) option set.
		 */
		if ((bp == targ->targ_sbufp) &&
		    (pktp->pkt_flags & FLAG_DIAGNOSE)) {
			rval = COMMAND_DONE_ERROR;
			level = SCSI_ERR_FATAL;
		} else if (targ->targ_retry_ct++ < samst_retry_count) {
			if (dp->b_forw == NULL)
				dp->b_forw = bp;
			targ->targ_timeout_ret = timeout(samst_restart,
			    (caddr_t)targ, SAMST_BSY_TIMEOUT);
			rval = JUST_RETURN;
			level = SCSI_ERR_RETRYABLE;
		} else {
			rval = COMMAND_DONE_ERROR;
			level = SCSI_ERR_FATAL;
		}
		break;

	case KEY_UNIT_ATTENTION:
		targ->targ_flags |= SAMST_FLAGS_ATTN;
		/* FALLTHRU */
	case KEY_ABORTED_COMMAND:
		rval = QUE_COMMAND;
		level = SCSI_ERR_INFO;
		break;

	case KEY_RECOVERABLE_ERROR:
		/* FALLTHRU */
	case KEY_NO_SENSE:
		rval = COMMAND_DONE;
		level = SCSI_ERR_RECOVERED;
		break;

	case KEY_BLANK_CHECK:
		rval = COMMAND_DONE_ERROR;
		level = SCSI_ERR_INFO;
		break;

	case KEY_HARDWARE_ERROR:
		/* FALLTHRU */
	case KEY_MEDIUM_ERROR:
		/* FALLTHRU */
	case KEY_MISCOMPARE:
		/* FALLTHRU */
	case KEY_VOLUME_OVERFLOW:
		/* FALLTHRU */
	case KEY_WRITE_PROTECT:
		/* FALLTHRU */
	case KEY_ILLEGAL_REQUEST:
		/* FALLTHRU */
	default:
		rval = COMMAND_DONE_ERROR;
		level = SCSI_ERR_FATAL;
		break;
	}

	/*
	 * If this was for a special command, check the options
	 */
	if (bp == targ->targ_sbufp) {
		if ((rval == QUE_COMMAND) && (pktp->pkt_flags & FLAG_DIAGNOSE))
			rval = COMMAND_DONE_ERROR;

		if (((pktp->pkt_flags & FLAG_SILENT) == 0) || samst_debug) {
			/*
			 * Report the error. Note: the zero is the error
			 * block number. You may want to fill it in from the
			 * sense data for appropriate errors.
			 */
			scsi_errmsg(targ->targ_devp, pktp, "samst", level,
			    bp->b_blkno, 0, samst_cmds,
			    &(ARQP(pktp)->sts_sensedata));
		}
	} else if ((level >= samst_error_reporting) || samst_debug) {
		scsi_errmsg(targ->targ_devp, pktp, "samst", level,
		    bp->b_blkno, 0, samst_cmds,
		    &(ARQP(pktp)->sts_sensedata));
	}
	return (rval);
}

/*
 * Command completion routine. Check the returned status of the
 * command
 */
static int
samst_check_error(struct samst_soft *targ, struct buf *bp)
{
	struct diskhd  *dp = &targ->targ_iorq;
	struct scsi_pkt *pkt = BP_PKT(bp);
	int		action;

	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "samst_check_error\n");

	/*
	 * Need the lock, we may be doing retries (targ_retry_ct) or looking
	 * at the sense packet (targ_rqs)
	 */
	ASSERT(mutex_owned(SAMST_MUTEX(targ)));

	if (SCBP(pkt)->sts_busy) {
		/*
		 * Target was busy. If we're not out of retries, call timeout
		 * to restart in a bit; otherwise give up and reset the
		 * target. If the fail on error flag is set, give up
		 * immediately.
		 */
		int		tval = (pkt->pkt_flags & FLAG_DIAGNOSE) ? 0
		    : SAMST_BSY_TIMEOUT;

		if (SCBP(pkt)->sts_is) {
			/*
			 * Implicit assumption here is that a device will
			 * only be reserved long enough to permit a single
			 * i/o operation to complete.
			 */
			tval = samst_io_time * drv_usectohz(1000000);
		}
		if ((int)targ->targ_retry_ct++ < samst_retry_count) {
			if (tval) {
				if (dp->b_forw == NULL)
					dp->b_forw = bp;
				targ->targ_timeout_ret =
				    timeout(samst_restart, (caddr_t)targ, tval);
				action = JUST_RETURN;
				SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
				    "Target busy, retrying\n");
			} else {
				action = COMMAND_DONE_ERROR;
				SAMST_LOG(targ->targ_devp, CE_NOTE,
				    "Target busy, no retries\n");
			}
		} else {
			/*
			 * We can temporarily give up the lock here. If we're
			 * pre-empted it's safe for another thread to get the
			 * lock
			 */
			mutex_exit(SAMST_MUTEX(targ));
			/*
			 * WARNING: See the warning in
			 * samst_handle_incomplete
			 */
			SAMST_LOG(targ->targ_devp, CE_NOTE,
			    "Resetting Target\n");
			if (!scsi_reset(ROUTE(targ), RESET_TARGET)) {
#ifdef SCSI_BUS_RESET
				SAMST_LOG(targ->targ_devp, CE_NOTE,
				    "Resetting SCSI Bus\n");
				if (!scsi_reset(ROUTE(targ), RESET_ALL)) {
					samst_log(targ->targ_devp, CE_WARN,
					    "SCSI bus reset failed\n");
				}
#else				/* SCSI_BUS_RESET */
				samst_log(targ->targ_devp, CE_WARN,
				    "Reset Target failed\n");
#endif				/* SCSI_BUS_RESET */
			}
			action = COMMAND_DONE_ERROR;
			mutex_enter(SAMST_MUTEX(targ));	/* reacquire lock */
		}
	} else if (SCBP(pkt)->sts_chk) {
		if (targ->targ_arq) {
			samst_log(targ->targ_devp, CE_WARN,
			    "Check condition with Auto Sensing\n");
			action = COMMAND_DONE_ERROR;
		} else {
			SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
			    "Check Condition\n");
			action = QUE_SENSE; /* check condition: get sense */
		}
	} else {
		int		cmd = GETCMD((union scsi_cdb *)pkt->pkt_cdbp);

		action = COMMAND_DONE;	/* assume command completed OK */

		/*
		 * pkt_resid will reflect, at this point, a residual of how
		 * many bytes were not transferred; a non-zero pkt_resid on a
		 * read or a write is an error. Note I'm doing this because
		 * format asks for too much data, so there's always a resid
		 * on the USCSICMDs it sends; if your device uses different
		 * commands for normal data transfer, test for them here.
		 */
		if (pkt->pkt_resid &&
		    (cmd == SCMD_READ || cmd == SCMD_WRITE)) {
			action = COMMAND_DONE_ERROR;
			bp->b_resid += pkt->pkt_resid;
			SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG1,
			    "Residual data (%d bytes)\n", bp->b_resid);
		}
		targ->targ_retry_ct = 0;
		SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2,
		    "Command Complete\n");
	}
	return (action);
}


/*
 *	General Utility Routines
 */

/*
 * See if the device is ready. Send a couple of Test Unit Ready commands
 * to clear Unit Attention conditions. If it's still not ready, send the
 * Start command and try another Test Unit Ready
 * Return 1 is the unit's ready, 0 if not.
 */
static int
samst_ready(struct samst_soft *targ, dev_t dev)
{
	int		retries = 3;

	SAMST_LOG(targ->targ_devp, SAMST_CE_DEBUG2, "samst_ready\n");

#if defined(TRYSTART)
	while (--retries) {
		if (samst_simple(dev, SCMD_TEST_UNIT_READY, 0)) {
			break;
		}
	}

	if (retries > 0) {
		SAMST_LOG(0, SAMST_CE_DEBUG2, "samst%d is ready\n",
		    SAMST_INST(dev));
		targ->targ_state = SAMST_STATE_NIL;
		return (1);
	}
	/*
	 * Not Ready, try a Start command
	 */
	if (!(targ->targ_flags & SAMST_FLAGS_IDLE)) {
		SAMST_LOG(0, SAMST_CE_DEBUG3, "starting samst%d\n",
		    SAMST_INST(dev));
		if (samst_simple(dev, SCMD_START_STOP, SAMST_START) == 0) {
			SAMST_LOG(0, SAMST_CE_DEBUG1,
			    "unable to start samst%d\n",
			    SAMST_INST(dev));
			return (0);
		}
		/*
		 * Started OK, try another Test Unit Ready
		 */
		if (samst_simple(dev, SCMD_TEST_UNIT_READY, 0) == 0) {
			SAMST_LOG(0, SAMST_CE_DEBUG1, "samst%d not ready\n",
			    SAMST_INST(dev));
			return (0);
		}
		SAMST_LOG(0, SAMST_CE_DEBUG2, "samst%d is ready\n",
		    SAMST_INST(dev));
		targ->targ_state = SAMST_STATE_NIL;
		return (1);	/* Unit is ready */
	}
	return (0);		/* not ready and idle */
#else
	/* Attempt just two tests */
	if (!samst_simple(dev, SCMD_TEST_UNIT_READY, 0) &&
	    !samst_simple(dev, SCMD_TEST_UNIT_READY, 0)) {
		targ->targ_state = SAMST_STATE_NIL;
		return (1);
	}
	targ->targ_state = SAMST_STATE_NIL;
	return (1);
#endif
}


/*
 * Send a simple (no DMA) command to the device
 * return 1 if the command succeeds, 0 if it fails.
 */
static int
samst_simple(dev_t dev, int cmd, int flag)
{
	struct uscsi_cmd scmd, *com = &scmd;
	char	    cmdblk[CDB_GROUP0];
	int		err;

	SAMST_LOG(0, SAMST_CE_DEBUG2, "samst_simple\n");

	com->uscsi_bufaddr = 0;
	com->uscsi_buflen = 0;
	bzero(cmdblk, CDB_GROUP0);
	cmdblk[0] = (char)cmd;
	cmdblk[4] = flag;
	com->uscsi_flags = USCSI_DIAGNOSE | USCSI_SILENT | USCSI_WRITE;
	com->uscsi_cdb = cmdblk;
	com->uscsi_cdblen = CDB_GROUP0;

	if (err = samst_ioctl_cmd(dev, com, UIO_SYSSPACE, UIO_SYSSPACE,
	    DDI_MODEL_NONE)) {
		SAMST_LOG(0, SAMST_CE_DEBUG1, "samst_simple failed: 0x%x\n",
		    err);
		err = 0;
	} else
		err = 1;

	return (err);
}

/*
 *	Error Message Data and Routines
 */

/*
 * Log a message to the console and/or syslog with cmn_err
 */
/*ARGSUSED*/
static void
samst_log(struct scsi_device *devp, int level, const char *fmt, ...)
{
	char	    name[16];
	char	    buf[256];
	va_list	 ap;

	if (devp)
		(void) sprintf(name, "%s%d", ddi_get_name(devp->sd_dev),
		    ddi_get_instance(devp->sd_dev));
	else
		(void) sprintf(name, "samst");

	va_start(ap, fmt);
	(void) vsprintf(buf, fmt, ap);
	va_end(ap);

	switch (level) {
	case CE_CONT:
	case CE_NOTE:
	case CE_WARN:
	case CE_PANIC:
		cmn_err(level, "%s:\t%s", name, buf);
		break;

	case SAMST_CE_DEBUG4:
		if (samst_debug < 4)
			break;
		/* FALLTHRU */
	case SAMST_CE_DEBUG3:
		if (samst_debug < 3)
			break;
		/* FALLTHRU */
	case SAMST_CE_DEBUG2:
		if (samst_debug < 2)
			break;
		/* FALLTHRU */
	case SAMST_CE_DEBUG1:
		/* FALLTHRU */
	default:
		cmn_err(CE_CONT, "%s:\t%s", name, buf);
		break;
	}
}


/*
 * Print a buffer readably
 */
static void
hex_print(char *msg, char *cp, int len)
{
	int		i = 0, j;
	char	    buf[BUFLEN];
	uchar_t	*ucp = (uchar_t *)cp;

	bzero(buf, BUFLEN);
	for (i = 0; i < len; i++) {
		if ((j = strlen(buf)) >= BUFLEN) {
			buf[BUFLEN - 2] = '>';
			buf[BUFLEN - 1] = 0;
			break;	/* cp too long, give up */
		}
		(void) sprintf(&buf[j], "%x ", ucp[i]);
	}

	cmn_err(CE_NOTE, "%s: %s\n", msg, buf);
}
