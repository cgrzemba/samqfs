/*
 * ----- sam/mount.c - Process the mount/umount functions.
 *
 *	Processes the SAMFS mount and umount functions.
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

#ifdef sun
#pragma ident "$Revision: 1.230 $"
#endif

#include "sam/osversion.h"

#ifdef sun
/*
 * ----- UNIX Includes
 */
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/param.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/cred.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/pathname.h>
#include <sys/file.h>
#include <sys/ddi.h>
#include <sys/mutex.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/dkio.h>
#include <sys/errno.h>
#include <sys/vtoc.h>
#endif /* sun */

#ifdef  linux
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/version.h>

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/file.h>
#include <linux/uio.h>
#include <linux/dirent.h>
#include <linux/proc_fs.h>
#include <linux/limits.h>
#include <linux/skbuff.h>
#include <linux/ip.h>

#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#include <asm/statfs.h>
#endif

extern unsigned long *QFS_num_physpages;

#define	MS_FORCE	MNT_FORCE
#endif /* linux */

/* ----- SAMFS Includes */

#include "sam/param.h"
#include "sam/types.h"
#include "sam/mount.h"

#include "inode.h"
#include "mount.h"
#include "sblk.h"
#include "debug.h"
#ifdef sun
#include "extern.h"
#endif /* sun */
#ifdef linux
#include "clextern.h"
#endif /* linux */
#include "trace.h"
#ifdef sun
#include "qfs_log.h"
#endif /* sun */

extern void sam_set_dau(sam_dau_t *dau, int lg_kblocks, int sm_kblocks);
extern int byte_swap_sb(struct sam_sblk *sb, size_t len);

#ifdef linux
extern struct proc_dir_entry *proc_fs_samqfs;
extern struct file_system_type samqfs_fs_type;
#endif /* linux */

static int sam_build_geometry(sam_mount_t *mp);
static int sam_validate_sblk(sam_mount_t *mp, struct samdent *devp,
	int *sblk_meta_on, int *sblk_data_on);

#ifdef sun
static int sam_read_sblk(sam_mount_t *mp);
static int sam_destroy_vnode(vnode_t *vp, int fflag);
#endif /* sun */

#define	SYSCALL_CNT_RETRIES		5

#ifdef linux

#define	PROC_FS_SAMFS_PATH	"/proc/fs/samfs"

struct	block_device_operations	nodev_bops = {
	open: NULL,
	release: NULL,
	ioctl: NULL,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	media_changed: NULL,
	revalidate_disk: NULL,
#else
	check_media_change: NULL,
	revalidate: NULL,
#endif
	owner: NULL
};

int nodev_major = -1;

/*
 * ----- nodev_init - Register a QFS metadata block device.  Save nodev
 *	major number.  Returns 0 on success, <0 on failure.
 */
unsigned int
nodev_init(void)
{
	int ret;

	if (nodev_major > 0) {  /* Already initialized */
		return (EINVAL);
	}

	/*
	 *  Register nodev block device.
	 *  >0 nodev major number is assigned by Linux.
	 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	if ((ret = register_blkdev(0, "nodev")) < 0) {
		return (ret);
	}
#else
	if ((ret = register_blkdev(0, "nodev", &nodev_bops)) < 0) {
		return (ret);
	}
#endif

	nodev_major = ret;

	return (0);
}


/*
 * ----- nodev_cleanup - Unregister a QFS metadata block device.
 */
int
nodev_cleanup(void)
{
	int ret;

	if (nodev_major <= 0) { /* Never initialized */
		return (-EINVAL);
	}

	/*
	 *  Unregister nodev block device.
	 */
	if ((ret = unregister_blkdev(nodev_major, "nodev")) < 0) {
		return (ret);
	}

	return (ret);
}


/*
 * ----- sam_dev_cleanup - Close all the device opens for
 * the specified file system, Linux does not need to maintain
 * an open of these devices after the initial mount.
 */
int
sam_dev_cleanup(sam_mount_t *mp)
{
	int	ord;
	struct samdent *fs_dent;
	struct file *fp = NULL;

	mutex_enter(&samgt.global_mutex);
	if (mp != NULL) {
		for (ord = 0; ord < mp->mt.fs_count; ord++) {
			fs_dent = &mp->mi.m_fs[ord];
			fp = fs_dent->fp;
			if (fp != NULL && fs_dent->opened) {
				filp_close(fp, NULL);
				fs_dent->fp = NULL;
			}
		}
	}
	mutex_exit(&samgt.global_mutex);
	return (0);
}
#endif /* linux */



#ifdef DKIOCGETVOLCAP
/*
 * Return TRUE if the DiskSuite Volume Manager on this host
 * supports ABR writes.  Return FALSE if it does not.  If SVM
 * declares a property "ddi-abrwrite-supported", then it does;
 * otherwise it doesn't.
 *
 * ddi-abrwrite-supported is supported through the DKIOCGETVOLCAP
 * DKIOCSETVOLCAP, and DKIOCDMR ioctl()s.  These are defined in
 * /usr/include/sys/dkio.h.  SAM-QFS, through the SAM AIO interface,
 * emulates these ioctl()s for SAM AIO files.
 */
static int
sds_abr_capable()
{
	static int abr_capable;		/* 1 = YES, -1 = NO, 0 = Don't know */
	dev_info_t *dip;

	if (abr_capable == 0) {
		abr_capable = -1;
		if ((dip = ddi_find_devinfo("md", -1, 0)) != NULL) {
			if (ddi_prop_exists(DDI_DEV_T_ANY,
			    dip, DDI_PROP_NOTPROM,
			    "ddi-abrwrite-supported")) {
				abr_capable = 1;
			}
		}
	}

	if (abr_capable == 1) {
		return (TRUE);
	}
	return (FALSE);
}
#endif	/* DKIOCGETVOLCAP */


#ifdef sun
/*
 * ----- sam_getdev - get block special devices.
 *	Open vnodes for the block special devices for this filesystem.
 */

int				/* ERRNO if error, 0 if successful. */
sam_getdev(
	sam_mount_t *mp,	/* The pointer to the mount table */
	int istart,		/* Starting dev position in the mount table */
	int filemode,		/* Filemode for open */
	int *npartp,		/* Number of valid devices found */
	cred_t *credp)		/* Credentials pointer. */
{
	struct samdent *dp;
	vnode_t *svp;
	int nparts = 0;
	dev_t dev;
	int nometa = FALSE;
	int i;
	int error = 0;

	for (i = istart, dp = &mp->mi.m_fs[istart]; i < mp->mt.fs_count;
	    i++, dp++) {
		if (dp->opened) {
			nparts++;
			continue;
		}
		if (SAM_IS_SHARED_FS(mp) && dp->part.pt_type == DT_META &&
		    strcmp(dp->part.pt_name, "nodev") == 0) {
			if (SAM_IS_SHARED_SERVER(mp)) {
				dp->error = ENODEV;
				continue;
			}
			nometa = TRUE;
			continue;
		}
		TRACES(T_SAM_OPEN_DEV, mp, dp->part.pt_name);
		if (is_osd_group(dp->part.pt_type)) {
			if ((dp->error = sam_open_osd_device(dp, filemode,
			    credp)) != 0) {
				continue;
			}
			if ((dp->error = sam_get_osd_fs_attr(mp, dp->oh,
			    &dp->part)) != 0) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Eq %d Error %d: cannot get "
				    "OSD capacity and space",
				    mp->mt.fi_name, dp->part.pt_eq, error);
				continue;
			}

			/*
			 * OSD devices handle sparse correctly.
			 */
			mp->mt.fi_config &= ~(MT_ZERO_DIO_SPARSE);
			/*
			 * Void pages for this device.
			 */
			(void) VOP_PUTPAGE_OS(dp->svp, 0, 0, B_INVAL, credp,
			    NULL);
			nparts++;

		} else {
			if ((lookupname(dp->part.pt_name, UIO_SYSSPACE, FOLLOW,
			    NULL, &svp))) {
				dp->error = ENODEV;
				continue;
			}
			if (svp->v_type != VBLK) {
				VN_RELE(svp);
				dp->error = ENOTBLK;	/* Must be type blk */
				continue;
			}
			dev = svp->v_rdev;
			VN_RELE(svp);
			if (vfs_devismounted(dev)) {
				dp->error = EBUSY;	/* Already mounted */
				continue;
			}
			if ((getmajor(dev) >= devcnt) ||
			    (devopsp[getmajor(dev)]->devo_cb_ops->cb_flag &
			    D_TAPE)) {
				dp->error = ENXIO;	/* Illegal major no */
				continue;
			}

			/*
			 * Make a special vnode for device.
			 */
			dp->dev = dev;
			dp->svp = makespecvp(dev, VBLK);
			dp->dev_bsize = DEV_BSIZE;

			/*
			 * Open special block device.
			 */
			error = VOP_OPEN_OS(&dp->svp, filemode, credp, NULL);
			if (error) {
				dp->error = error;
				continue;
			}
			if (IS_SWAPVP(dp->svp)) {
				VN_RELE(svp);
				dp->error = EBUSY;	/* Dev used for swap */
				continue;
			}
			dp->opened = 1;

			/*
			 * Void pages & buffer cache for this device.
			 */
			(void) VOP_PUTPAGE_OS(dp->svp, 0, 0, B_INVAL, credp,
			    NULL);
			(void) bflush(dp->dev);	/* flush changed buffers */
			nparts++;
		}
	}
	*npartp = nparts;

	/*
	 * Set device state. If no devices, return error
	 */
	for (i = istart, dp = &mp->mi.m_fs[istart]; i < mp->mt.fs_count;
	    i++, dp++) {
		if (dp->error == 0) {
			if (istart == 0) {
				dp->part.pt_state = DEV_ON;
			} else {
				dp->part.pt_state = DEV_OFF;
			}
		} else if (dp->error == ENXIO) {
			dp->part.pt_state = DEV_OFF;
			dp->skip_ord = 1;
			error = ENXIO;
		} else {
			dp->part.pt_state = DEV_DOWN;
			dp->skip_ord = 1;
		}
		TRACE(T_SAM_DEV_OPEN, mp, dp->part.pt_eq, dp->part.pt_state,
		    dp->error);
	}
	if (nparts < 1) {
		return (ENODEV);
	}

	/*
	 * Check for nodev (shared), and make pseudo device
	 */
	if (SAM_IS_SHARED_FS(mp)) {
		if (nometa) {
			for (i = istart, dp = &mp->mi.m_fs[istart];
			    i < mp->mt.fs_count;
			    i++, dp++) {
				if (dp->part.pt_state != DEV_ON) {
					continue;
				}
				if (dp->part.pt_type == DT_META) {
					if (!dp->opened) {
						/*
						 * Make unique (pseudo) devs for
						 * the meta devices.
						 */
						dp->dev =
						    makedevice(
						    samgt.samioc_major,
						    dp->part.pt_eq);
						dp->svp = makespecvp(dp->dev,
						    VBLK);
					} else {
						cmn_err(CE_WARN,
						    "SAM-QFS: %s: Mixed "
						    "real/nodev metadevices in"
						    " shared FS",
						    mp->mt.fi_name);
					}
				}
			}
			mutex_enter(&mp->ms.m_waitwr_mutex);
			mp->mt.fi_status |= FS_NODEVS;
			mutex_exit(&mp->ms.m_waitwr_mutex);
		} else {
			mutex_enter(&mp->ms.m_waitwr_mutex);
			mp->mt.fi_status &= ~FS_NODEVS;
			mutex_exit(&mp->ms.m_waitwr_mutex);
		}
	}

#ifdef DKIOCGETVOLCAP
	if (!error) {
		if (sds_abr_capable()) {
			int abr_flags = mp->mt.fi_config &
			    (MT_ABR_DATA | MT_DMR_DATA);
			struct volcap *vc;

			/*
			 * Issue Get Volume Capabilities ioctl() to FS's
			 * devices.  Record whether or not all the FS's metadata
			 * and data devices are capable of supporting ABR
			 * (application based (mirror) recovery).  If so, set
			 * status flags in the mount structure saying so.
			 */
			vc = kmem_alloc(sizeof (*vc), KM_SLEEP);
			for (i = istart, dp = &mp->mi.m_fs[istart];
			    i < mp->mt.fs_count;
			    i++, dp++) {
				int xerror, xret;

				if (dp->part.pt_state != DEV_ON) {
					continue;
				}
				if (is_osd_group(dp->part.pt_type)) {
					continue;
				}
				if (dp->opened) {
					xret = 0;
					dp->abr_cap = 0;
					dp->dmr_cap = 0;
					xerror = cdev_ioctl(dp->dev,
					    DKIOCGETVOLCAP, (intptr_t)vc,
					    FKIOCTL | FNATIVE | FREAD, credp,
					    &xret);
					if (xerror == 0 && xret == 0) {
						if (vc->vc_info & DKV_ABR_CAP) {
							dp->abr_cap = 1;
						}
						if (vc->vc_info & DKV_DMR_CAP) {
							dp->dmr_cap = 1;
						}
					}
					switch (dp->part.pt_type) {
					case DT_META:
						break;

					case DT_DATA:
					case DT_RAID:
						if (!dp->abr_cap) {
							abr_flags &=
							    ~MT_ABR_DATA;
						}
						if (!dp->dmr_cap) {
							abr_flags &=
							    ~MT_DMR_DATA;
						}
						break;

					default:
						if (is_stripe_group(
						    dp->part.pt_type)) {
							if (!dp->abr_cap) {
						abr_flags &= ~MT_ABR_DATA;
							}
							if (!dp->dmr_cap) {
						abr_flags &= ~MT_DMR_DATA;
							}
						}
					}
				}
			}
			kmem_free(vc, sizeof (*vc));

			mutex_enter(&mp->ms.m_waitwr_mutex);
			mp->mt.fi_config &= ~(MT_ABR_DATA | MT_DMR_DATA);
			mp->mt.fi_config |= abr_flags;
			mutex_exit(&mp->ms.m_waitwr_mutex);
		} else {
			mutex_enter(&mp->ms.m_waitwr_mutex);
			mp->mt.fi_config &= ~(MT_ABR_DATA | MT_DMR_DATA);
			mutex_exit(&mp->ms.m_waitwr_mutex);
		}
	}
#endif	/* DKIOCGETVOLCAP */

	return (error);
}
#endif /* sun */



#ifdef linux
/*
 * ----- sam_getdev - get block special devices.
 *	Open vnodes for the block special devices for this filesystem.
 */

int					/* ERRNO if error, 0 if successful. */
sam_lgetdev(
	sam_mount_t *mp,		/* The pointer to the mount table */
	struct sam_fs_part *fsp)	/* Pointer to the device array */
{
	struct sam_fs_part *cur_fsp;
	struct samdent *fs_dent;
	dev_t dev;
	int nometa = FALSE;
	int i;
	int error = 0;
	struct file *fp;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
	char nodev_name[256];
#endif

	ASSERT(SAM_IS_SHARED_FS(mp));

	for (i = 0, cur_fsp = fsp, fs_dent = &mp->mi.m_fs[0];
	    i < mp->mt.fs_count;
	    i++, cur_fsp++, fs_dent++) {
		if (cur_fsp->pt_type == DT_META &&
		    strcmp(cur_fsp->pt_name, "nodev") == 0) {
			nometa = TRUE;
			continue;
		}
		fp = filp_open(cur_fsp->pt_name, O_RDWR, 0);

		error = PTR_ERR(fp);
		if (IS_ERR(fp)) {
			break;
		}
		error = 0;

		dev = fp->f_dentry->d_inode->i_rdev;

		if (!S_ISBLK(fp->f_dentry->d_inode->i_mode)) {
			error = ENOTBLK;
			break;
		}

		fs_dent->fp = fp;
		fs_dent->dev = dev;
		fs_dent->part = *cur_fsp;
		fs_dent->opened = 1;
	}

	if (error) {
		goto out_err;
	}

	if (!nometa) {
		mutex_enter(&mp->ms.m_waitwr_mutex);
		mp->mt.fi_status &= ~FS_NODEVS;
		mutex_exit(&mp->ms.m_waitwr_mutex);
		goto out_err;
	}

	if (samgt.meta_minor == 255) {
		samgt.meta_minor = 0;
	}
	for (i = 0, cur_fsp = fsp, fs_dent = &mp->mi.m_fs[0];
	    i < mp->mt.fs_count; i++, cur_fsp++, fs_dent++) {
		if (cur_fsp->pt_type == DT_META) {
			if (!fs_dent->opened) {
				/*
				 * Create /proc/fs/samfs/"fsname"/ if needed.
				 */
				if (mp->mi.proc_ent == NULL) {
					mp->mi.proc_ent = proc_mkdir(
					    mp->mt.fi_name,
					    proc_fs_samqfs);
				}

				/*
				 * Make unique proc entries for meta devices.
				 */
				if (mp->mi.proc_ent) {
					samgt.meta_minor++;
					fs_dent->fp = NULL;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0))
					sprintf(nodev_name, "nodev_%d",
					    samgt.meta_minor);
					proc_mknod(nodev_name, S_IFBLK | 0666,
					    mp->mi.proc_ent,
					    (kdev_t)MKDEV(nodev_major,
					    samgt.meta_minor));
					fs_dent->dev = ((nodev_major<<8) |
					    (samgt.meta_minor & 0xff));
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
					fs_dent->dev = MKDEV(nodev_major,
					    samgt.meta_minor);
#endif
					fs_dent->part = *cur_fsp;
				} else {
					cmn_err(CE_WARN, "SAM-QFS: %s: "
					    "Can't create nodev metadevices",
					    mp->mt.fi_name);
				}
			} else {
				cmn_err(CE_WARN, "SAM-QFS: %s: Mixed "
				    "real/nodev metadevices", mp->mt.fi_name);
			}
		}
	}
	mutex_enter(&mp->ms.m_waitwr_mutex);
	mp->mt.fi_status |= FS_NODEVS;
	mutex_exit(&mp->ms.m_waitwr_mutex);

out_err:
	return (error);
}
#endif /* linux */


/*
 * ----- sam_mount_fs - process UNIX mount.
 *	Set up the geometry for the SAM disk family set.
 */

int					/* ERRNO if error, 0 if successful. */
sam_mount_fs(sam_mount_t *mp)
{
	int	error = 0;

	/*
	 * Initialize the mount table.
	 */
	mp->mi.m_inodir = (sam_node_t *)NULL;
	mp->mi.m_vn_root = NULL;

	/*
	 * Set up the geometry for the SAM disk family set.
	 */
	if ((error = sam_build_geometry(mp)) != 0) {
		return (error);
	}
	error = sam_set_mount(mp);
	return (error);
}


/*
 * ----- sam_set_mount - process UNIX mount.
 *	Set up the release inode and release block tables.
 *	Read superblock and dau maps. Set up the mount table entry
 *	and SAM inode extension array.
 */

int					/* ERRNO if error, 0 if successful. */
sam_set_mount(sam_mount_t *mp)
{
	int i, dau, num_group;
	int err_line = 0;
	int error = 0;
	offset_t capacity = 0;
	offset_t mm_capacity = 0;
	struct sam_sblk *sblk;
	struct sam_sbinfo *sb;

	/*
	 * For shared client, get sblk OTW. For server or local mount,
	 * read sblks from the meta device.
	 */
	if (SAM_IS_SHARED_CLIENT(mp)) {
		error = sam_update_shared_filsys(mp, SHARE_wait, 0);
	} else {
#ifdef sun
		if (SAM_IS_SHARED_FS(mp) && !SAM_IS_SHARED_SERVER_ALT(mp)) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: shared file system with no meta "
			    "devices",
			    mp->mt.fi_name);
			err_line = __LINE__;
			error = EINVAL;
			TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, 0);
			return (error);
		}
		error = sam_read_sblk(mp);
#endif /* sun */
#ifdef linux
		cmn_err(CE_WARN,
		    "SAM-FS: mp %p is not a shared client file system", mp);
		err_line = __LINE__;
		error = ENOSYS;
		TRACE(T_SAM_MNT_ERRLN, mp, err_line, error, 0);
#endif /* linux */
	}
	if (error) {
		return (error);
	}

	sblk = mp->mi.m_sbp;
	sb = &sblk->info.sb;

	/*
	 * The superblock is modified to indicate WORM files
	 * are present in the filesystem.  If WORM files
	 * are resident and the appropriate mount option was
	 * not provided, fail the mount.  The following are
	 * the only allowable options/sblk config:
	 *	SBLK			Allowed Options
	 *	----			---------------
	 *	WORM			MT_WORM
	 *	WORM_LITE		MT_WORM_LITE, MT_WORM(upgrade)
	 *	EMUL			MT_WORM_EMUL
	 *	EMUL_LITE		MT_EMUL_LITE, MT_EMUL(upgrade)
	 *
	 * If no WORM option was provided and the sblk indicates
	 * WORM files are present for the particular mode, fail
	 * the mount with EROFS.
	 */

	if (sb->opt_mask_ver == SBLK_OPT_VER1) {

		if (SBLK_WORM(sb) || WORM_MT_OPT(mp)) {
			if (mp->mt.fi_config & (MT_WORM_LITE |
			    MT_ALLEMUL)) {
				if (SBLK_WORM(sb)) {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: Super block "
					    "WORM capable,"
					    " incorrect mount option "
					    "provided", mp->mt.fi_name);
				} else {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: Excess WORM "
					    "mount options",
					    mp->mt.fi_name);
				}
				return (EINVAL);
			}
			if (!WORM_MT_OPT(mp)) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Super block WORM capable,"
				    " incorrect mount option provided",
				    mp->mt.fi_name);
				return (EROFS);
			}
		}

		if (SBLK_WORM_EMUL(sb) || EMUL_MT_OPT(mp)) {
			if (mp->mt.fi_config & (MT_ALLWORM | MT_EMUL_LITE)) {
				if (SBLK_WORM_EMUL(sb)) {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: Super block Emulation"
					    " mode capable, incorrect mount "
					    "option provided",
					    mp->mt.fi_name);
				} else {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: Excess WORM "
					    "mount options",
					    mp->mt.fi_name);
				}
				return (EINVAL);
			}
			if (!EMUL_MT_OPT(mp)) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Super block Emulation"
				    " mode capable, incorrect mount option "
				    "provided",
				    mp->mt.fi_name);
				return (EROFS);
			}
		}

		if (SBLK_WORM_LITE(sb) || WORM_LITE_MT_OPT(mp)) {
			if (mp->mt.fi_config & MT_ALLEMUL) {
				if (SBLK_WORM_LITE(sb)) {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: Super block WORM_LITE"
					    " capable, incorrect mount "
					    "option provided",
					    mp->mt.fi_name);
				} else {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: Excess WORM "
					    "mount options",
					    mp->mt.fi_name);
				}
				return (EINVAL);
			}
			if ((mp->mt.fi_config & MT_ALLWORM) == 0) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Super block WORM_LITE"
				    " capable, incorrect mount option provided",
				    mp->mt.fi_name);
				return (EROFS);
			}
		}

		if (SBLK_EMUL_LITE(sb) || EMUL_LITE_MT_OPT(mp)) {
			if (mp->mt.fi_config & MT_ALLWORM) {
				if (SBLK_EMUL_LITE(sb)) {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: Super block Emulation"
					    " LITE capable, incorrect "
					    "mount option provided",
					    mp->mt.fi_name);
				} else {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: Excess WORM "
					    "mount options",
					    mp->mt.fi_name);
				}
				return (EINVAL);
			}

			if ((mp->mt.fi_config & MT_ALLEMUL) == 0) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Super block Emulation"
				    " LITE capable, incorrect mount "
				    "option provided",
				    mp->mt.fi_name);
				return (EROFS);
			}
		}
	}

	for (i = 0; i < sblk->info.sb.fs_count; i++) {
		sblk->eq[i].fs.eq = mp->mi.m_fs[i].part.pt_eq;

		/*
		 * Backward compatibility in superblock prior to 3.3
		 */
		if (mp->mt.mm_count == 0) {
			int len;

			sblk->eq[i].fs.mm_ord = (ushort_t)i;
			sblk->eq[i].fs.num_group = 1;
			sblk->eq[i].fs.type = mp->mi.m_fs[i].part.pt_type;
			sblk->eq[i].fs.state = DEV_ON;
			sblk->eq[i].fs.fill2 = 0;
			len = sblk->eq[i].fs.allocmap +
			    sblk->eq[i].fs.l_allocmap;
			dau = sblk->info.sb.dau_blks[LG];
			if (i == 0) {
				len += dau;
			}
			sblk->eq[i].fs.system = roundup(len, dau);
		}
		/*
		 * Fix up "system" in 3.3.0 and possibly other stripe groups.
		 * Each stripe subordinate must have the same system value as
		 * the first ordinal in the stripe group.
		 */
		if (is_stripe_group(sblk->eq[i].fs.type)) {
			if (i > 0 && sblk->eq[i - 1].fs.type ==
			    sblk->eq[i].fs.type) {
				/*
				 * Stripe subordinates (not first; num_group ==
				 * 0) have the same system value as the first
				 * stripe ordinal.
				 */
				if (sblk->eq[i].fs.system !=
				    sblk->eq[i - 1].fs.system) {
					sblk->eq[i].fs.system =
					    sblk->eq[i - 1].fs.system;
				}
			}
		}
	}
	if (mp->mt.mm_count == 0) {
		sblk->info.sb.mm_blks[SM] = sblk->info.sb.dau_blks[SM];
		sblk->info.sb.mm_blks[LG] = sblk->info.sb.dau_blks[LG];
		sblk->info.sb.da_count = sblk->info.sb.fs_count;
		sblk->info.sb.mm_count = 0;
	}

	/*
	 * Backward compatibility in superblock prior to 3.5. Set LG meta
	 * blocks.
	 */
	if (mp->mt.mm_count) {
		if (sblk->info.sb.mm_blks[LG] == 0) {
			sblk->info.sb.mm_blks[SM] = sblk->info.sb.meta_blks;
			sblk->info.sb.mm_blks[LG] = sblk->info.sb.meta_blks;
		}
	}

	/*
	 * Backwards compatibility for new (4.0) fsid field.
	 */
	if (sblk->info.sb.fs_id.val[0] == 0 &&
	    sblk->info.sb.fs_id.val[1] == 0) {
		cmn_err(CE_NOTE, "SAM-QFS: Initializing sblk.fs_id to %x/%x",
		    mp->ms.m_hostid, sblk->info.sb.init);
		sblk->info.sb.fs_id.val[0] = sblk->info.sb.init;
		sblk->info.sb.fs_id.val[1] = mp->ms.m_hostid;
	}

	/*
	 * Backwards compatibility for new (4.0) ext_bshift field.
	 */
	if (sblk->info.sb.ext_bshift == 0) {
		sblk->info.sb.ext_bshift = SAM_DEV_BSHIFT;
	}
	ASSERT((sblk->info.sb.ext_bshift == SAM_DEV_BSHIFT) ||
	    (sblk->info.sb.ext_bshift == SAM_SHIFT));
	mp->mi.m_bn_shift = sblk->info.sb.ext_bshift - SAM_DEV_BSHIFT;

	/*
	 * Backwards compatibility for new (5.0) fi_type field.
	 */
	if (sblk->info.sb.fi_type == 0) {
		sblk->info.sb.fi_type = mp->mt.fi_type;
	}
	if ((sblk->info.sb.fi_type != DT_DISK_SET) &&
	    (sblk->info.sb.fi_type != DT_META_SET) &&
	    (sblk->info.sb.fi_type != DT_META_OBJECT_SET) &&
	    (sblk->info.sb.fi_type != DT_META_OBJ_TGT_SET)) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: Unrecognized family set type 0x%x , "
		    "resetting to 0x%x", mp->mt.fi_name, sblk->info.sb.fi_type,
		    mp->mt.fi_type);
		sblk->info.sb.fi_type = mp->mt.fi_type;
	}

	/*
	 * Set lowest inumber to be assigned to a user inode.
	 */
	if (sblk->info.sb.magic == SAM_MAGIC_V1) {
		mp->mi.m_min_usr_inum = SAM_MIN_USER_INO_VERS_1;
	} else {
		mp->mi.m_min_usr_inum = sblk->info.sb.min_usr_inum;
	}

	/*
	 * Check file system versioning, if in use.
	 */
	if (sblk->info.sb.opt_mask_ver > 0) {	/* Versioning in use */
		error = 0;
		if (sblk->info.sb.opt_mask_ver == SBLK_OPT_VER1) {
			if (sblk->info.sb.opt_mask & ~SBLK_ALL_OPTV1) {
				/*
				 * This version of SAM-QFS only understands the
				 * following options:
				 *		WORM file system
				 *		WORM lite file system
				 *		WORM Emulation file system
				 *		WORM Emulation lite file system
				 *		Sparse file systems
				 * 		Large host table
				 * A mismatch has occurred, fail the mount.
				 */
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: mount failed, option "
				    "mask mismatch 0x%x/0x%x",
				    mp->mt.fi_name, sblk->info.sb.opt_mask,
				    SBLK_ALL_OPTV1);
				err_line = __LINE__;
				error = EINVAL;
			}
			/*
			 * If WORM files are present and have not been
			 * converted to version 2 (samfsck -F) then fail
			 * the mount.
			 */
			if (((sblk->info.sb.opt_mask &
			    SBLK_OPTV1_ALL_WORM) != 0) &&
			    ((sblk->info.sb.opt_mask &
			    SBLK_OPTV1_CONV_WORMV2) == 0)) {
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: mount failed, WORM V2"
				    " option mask mismatch 0x%x/0x%x",
				    mp->mt.fi_name, sblk->info.sb.opt_mask,
				    SBLK_OPTV1_CONV_WORMV2);
				cmn_err(CE_WARN,
				    "SAM-QFS: %s: Please run samfsck -F "
				    "to repair the filesystem",
				    mp->mt.fi_name);
				err_line = __LINE__;
				error = EINVAL;
			}
		} else {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: mount failed, option mask "
			    "version mismatch %d",
			    mp->mt.fi_name, sblk->info.sb.opt_mask_ver);
			err_line = __LINE__;
			error = EINVAL;
		}
		if (error != 0) {
			TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, 0);
			return (error);
		}
	}

	/*
	 * If shared filesystem, verify this is a shared mount.
	 */
	if (sblk->info.sb.hosts && !SAM_IS_SHARED_FS(mp)) {
		if (!SAM_IS_SHARED_READER(mp)) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: shared file system, add "
			    "shared to mcf",
			    mp->mt.fi_name);
			err_line = __LINE__;
			error = EINVAL;
			TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, 0);
			return (error);
		}
	} else if (!sblk->info.sb.hosts && SAM_IS_SHARED_FS(mp)) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: not a shared file system, remove "
		    "shared from mcf",
		    mp->mt.fi_name);
		err_line = __LINE__;
		error = EINVAL;
		TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, 0);
		return (error);
	}
	if ((mp->mt.fi_config & MT_SHARED_READER) &&
	    (mp->mt.fi_config & MT_SHARED_WRITER)) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: 'reader' and 'writer' mount options "
		    "incompatible",
		    mp->mt.fi_name);
		err_line = __LINE__;
		error = EINVAL;
		TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, 0);
		return (error);
	}
	if ((mp->mt.fi_config & MT_SHARED_READER) &&
	    ((mp->mt.fi_config1 & MC_SHARED_FS) ||
	    (mp->mt.fi_config & MT_SHARED_MO))) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: shared file system, 'reader' option "
		    "incompatible",
		    mp->mt.fi_name);
		err_line = __LINE__;
		error = EINVAL;
		TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, 0);
		return (error);
	}
	if ((mp->mt.fi_config & MT_SHARED_WRITER) &&
	    ((mp->mt.fi_config1 & MC_SHARED_FS) ||
	    (mp->mt.fi_config & MT_SHARED_MO))) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: shared file system, 'writer' option "
		    "incompatible",
		    mp->mt.fi_name);
		err_line = __LINE__;
		error = EINVAL;
		TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, 0);
		return (error);
	}

	num_group = -1;
	mp->mi.m_dk_max[MM] = 0;
	mp->mi.m_dk_max[DD] = 0;
	for (i = 0; i < sblk->info.sb.fs_count; i++) {
		if ((sblk->eq[i].fs.state == DEV_ON) ||
		    (sblk->eq[i].fs.state == DEV_NOALLOC)) {
			if (sblk->eq[i].fs.type == DT_META) {
				mm_capacity += sblk->eq[i].fs.capacity;
			} else {
				capacity += sblk->eq[i].fs.capacity;
			}
		}
		if ((error = sam_build_allocation_links(mp, sblk, i,
		    &num_group))) {
			err_line = __LINE__;
			TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, 0);
			return (error);
		}
	}
	if (mm_capacity != sblk->info.sb.mm_capacity) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: metadata eq capacity %llx "
		    "does not match fs capacity %llx\n",
		    mp->mt.fi_name, mm_capacity, sblk->info.sb.mm_capacity);
		err_line = __LINE__;
		error = EINVAL;
		TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, 0);
		return (error);
	}
	if (capacity != sblk->info.sb.capacity) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: data eq capacity %llx "
		    "does not match fs capacity %llx\n",
		    mp->mt.fi_name, capacity, sblk->info.sb.capacity);
		/*
		 * Do not fail the mount for object fs data capacity mismatch.
		 */
		if (sblk->info.sb.fi_type != DT_META_OBJECT_SET) {
			err_line = __LINE__;
			error = EINVAL;
			TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, 0);
			return (error);
		}
	}

	/* compute high and low block counts. */
	sam_mount_setwm_blocks(mp);
	mp->mi.m_fsfullmsg = 0;

	sam_set_dau(&mp->mi.m_dau[DD], sblk->info.sb.dau_blks[SM],
	    sblk->info.sb.dau_blks[LG]);
	sam_set_dau(&mp->mi.m_dau[MM], sblk->info.sb.mm_blks[SM],
	    sblk->info.sb.mm_blks[LG]);

	/*
	 * By default, set data device stripe size based on DAU size.
	 * DAU/STRIPE 16/8, 32/4, 64/2, 128/1, >128/1.
	 * Stripe defaults to round robin if striped groups exist.
	 */
	if (mp->mt.fi_stripe[DD] < 0) {
		if (SAM_IS_OBJECT_FS(mp)) {
			mp->mt.fi_stripe[DD] = 1;
		} else if (SAM_IS_SHARED_FS(mp)) {
			mp->mt.fi_stripe[DD] = 0;
		} else {
			dau = LG_DEV_BLOCK(mp, DD);
			mp->mt.fi_stripe[DD] = 1;
			if (mp->mt.fi_config1 & MC_STRIPE_GROUPS) {
				mp->mt.fi_stripe[DD] = 0;
			} else if (dau <= SAM_CONTIG_LUN) {
				mp->mt.fi_stripe[DD] = SAM_CONTIG_LUN / dau;
			}
		}
	}

	if (mp->mt.fi_minallocsz < 0) {
		mp->mt.fi_minallocsz = SAM_DEFAULT_MINALLOCSZ * LG_BLK(mp, DD);
	}
	if (mp->mt.fi_maxallocsz < 0) {
		mp->mt.fi_maxallocsz = SAM_DEFAULT_MAXALLOCSZ * LG_BLK(mp, DD);
	}
	if (mp->mt.fi_maxallocsz < mp->mt.fi_minallocsz) {
		mp->mt.fi_maxallocsz = mp->mt.fi_minallocsz;
	}
	mp->mt.fi_ext_bsize = 1 << sblk->info.sb.ext_bshift;

#ifdef sun
	/*
	 * Default maximum I/O request size to MAX(1MB, maxphys).
	 */
	mp->mi.m_maxphys = MAX(SAM_ONE_MEGABYTE, maxphys);

	if (SAM_IS_SHARED_FS(mp) &&
	    (mp->mt.fi_stage_n_window < mp->mt.fi_minallocsz)) {
		mp->mt.fi_stage_n_window = mp->mt.fi_minallocsz;
	}
	if (mp->mt.fi_stage_n_window > ((physmem * PAGESIZE) / 100)) {
		cmn_err(CE_NOTE,
		"SAM-QFS: %s: stage_n_window(%u) > 1%% physmem(%lld),"
		    " stage_n_window too large for swap space, may block"
		    " if file system full",
		    mp->mt.fi_name, mp->mt.fi_stage_n_window,
		    (long long)((physmem * PAGESIZE) / 100));
	}
#endif /* sun */
	return (0);
}


/*
 * ----- sam_build_allocation_links - build the allocation chain for MM & DD
 */

int				/* ERRNO if error, 0 if successful. */
sam_build_allocation_links(
	sam_mount_t *mp,	/* Pointer to the mount table */
	struct sam_sblk *sblk,	/* Pointer to the superblock */
	int i,			/* Ordinal */
	int *num_group_ptr)	/* Pointer to num_group */
{
	struct samdent *dp;
	int j, disk_type, num_group;
	int error = 0;

	num_group = *num_group_ptr;
	dp = &mp->mi.m_fs[i];
	dp->dt = (dp->part.pt_type == DT_META) ? MM : DD;
	dp->system = sblk->eq[i].fs.system;
	dp->part.pt_state = sblk->eq[i].fs.state;
	dp->num_group = sblk->eq[i].fs.num_group;
	if (((dp->part.pt_state != DEV_ON) &&
	    (dp->part.pt_state != DEV_NOALLOC)) ||
	    (dp->num_group == 0)) {
		dp->skip_ord = 1;		/* skip it */
		return (0);
	}
	if (dp->part.pt_type == DT_META) {
		disk_type = MM;
	} else if (is_osd_group(dp->part.pt_type)) {
		disk_type = DD;
		if (num_group < 0) {
			num_group = dp->num_group;
		}
	} else {
		disk_type = DD;
		/*
		 * If mismatched number of elements in the striped groups, must
		 * round robin.
		 */
		if (num_group < 0) {
			num_group = dp->num_group;
		} else {
			if (dp->num_group != num_group) {
				mp->mt.fi_config1 |= MC_MISMATCHED_GROUPS;
				if (mp->mt.fi_stripe[DD] > 0) {
					mp->mt.fi_stripe[DD] = 0;
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: mismatched "
					    "striped groups,"
					    " stripe set to round robin",
					    mp->mt.fi_name);
				}
			}
		}
	}

	/*
	 * Build forward link chain for MM or DD disks with state = ON/NOALLOC.
	 */
	if (mp->mi.m_dk_max[disk_type] == 0) {	/* First entry */
		mp->mi.m_dk_start[disk_type] = (short)i;
		mp->mi.m_unit[disk_type] = (short)i;
		mp->mi.m_dk_max[disk_type]++;
	} else {
		int ord;

		for (ord = 0; ord < sblk->info.sb.fs_count; ord++) {
			if (disk_type != mp->mi.m_fs[ord].dt) {
				continue;
			}
			if ((ord == i) ||
			    (mp->mi.m_fs[ord].num_group == 0) ||
			    ((sblk->eq[ord].fs.state != DEV_ON) &&
			    (sblk->eq[ord].fs.state != DEV_NOALLOC))) {
				continue;
			}
			if (mp->mi.m_fs[ord].next_ord == 0) {
				mp->mi.m_fs[ord].next_ord = i;
				mp->mi.m_dk_max[disk_type]++;
				break;
			}
		}
	}

	/*
	 * For OSD groups, return max. number of devices in the group.
	 */
	if (is_osd_group(dp->part.pt_type)) {
		*num_group_ptr = num_group;
		return (error);
	}

	/*
	 * Make sure that stripe group elements have adjacent
	 * ordinals.  Code in r/w paths increment the ordinal
	 * to move to the next device.
	 */
	for (j = 1; j < dp->num_group; j++) {
		if (i+j >= mp->mt.fs_count ||
		    mp->mi.m_fs[i+j].part.pt_type != dp->part.pt_type) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: non-contiguous stripe "
			    "element ordinals",
			    mp->mt.fi_name);
			error = EINVAL;
		}
	}
	*num_group_ptr = num_group;
	return (error);
}


#ifdef sun
/*
 * ----- sam_read_sblk - read the superblock from disk.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_read_sblk(sam_mount_t *mp)
{
	int error = ENOSYS;
	buf_t *bp[2] = {NULL, NULL};
	int i;
	int err_line = 0;
	int found_v1_sblk = 0;
	int found_v2_sblk = 0;
	int found_v2A_sblk = 0;
	int isblk;
	struct sam_sblk *sblk[2];
	buf_t *sbp;
	int32_t sblk_size;

	/*
	 * Read 2 superblocks from primary disk.
	 */
	for (i = 0; i < 2; i++) {
		error = SAM_BREAD(mp, mp->mi.m_fs[0].dev,
		    mp->mi.m_sblk_offset[i], DEV_BSIZE, &bp[i]);
		if (error) {
			err_line = __LINE__;
			goto out;
		}
		sblk[i] = (struct sam_sblk *)(void *)bp[i]->b_un.b_addr;

		switch (sblk[i]->info.sb.magic) {
		case SAM_MAGIC_V1:
			found_v1_sblk++;
			break;

		case SAM_MAGIC_V2:
			found_v2_sblk++;
			break;

		case SAM_MAGIC_V2A:
			found_v2A_sblk++;
			break;

		case SAM_MAGIC_V1_RE:
		case SAM_MAGIC_V2_RE:
		case SAM_MAGIC_V2A_RE:
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: non-native, non-shared "
			    "FS (reversed byte order)",
			    mp->mt.fi_name);
			err_line = __LINE__;
			error = EILSEQ;		/* "illegal byte sequence" */
			goto out;

		default:
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: superblock magic on "
			    "eq %d is invalid\n",
			    mp->mt.fi_name, mp->mi.m_fs[0].part.pt_eq);
			err_line = __LINE__;
			error = EINVAL;
			goto out;
		}
		if (i == 0)  continue;

		if ((found_v2_sblk && found_v1_sblk) ||
		    (found_v2A_sblk && found_v2_sblk)) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: superblock magic on "
			    "eq %d is invalid\n",
			    mp->mt.fi_name, mp->mi.m_fs[0].part.pt_eq);
			err_line = __LINE__;
			error = EINVAL;
			goto out;
		}
		/*
		 * If the file system is dirty, use 1st superblock.
		 * If the file system is clean, use 2nd superblock.
		 */
		if (sblk[0]->info.sb.time != sblk[i]->info.sb.time) {
			isblk = 0;
		} else {
			isblk = 1;
		}
		if (sblk[isblk]->info.sb.state & SB_FSCK_ALL) {
			cmn_err(CE_WARN, "SAM-QFS: %s: needs fsck",
			    sblk[i]->info.sb.fs_name);
		} else if ((sblk[0]->info.sb.state ^ sblk[1]->info.sb.state)
		    & SB_FSCK_ALL) {
			cmn_err(CE_WARN, "SAM-QFS: %s: superblocks disagree"
			    " on fsck request; setting fsck request",
			    sblk[isblk]->info.sb.fs_name);
			sblk[isblk]->info.sb.state |=
			    sblk[1-isblk]->info.sb.state;
		}
		sblk_size = roundup(sblk[isblk]->info.sb.sblk_size, DEV_BSIZE);
	}

	if ((sblk_size < (L_SBINFO + L_SBORD)) ||
	    (sblk_size > (L_SBINFO + (L_FSET *  L_SBORD)))) {
		cmn_err(CE_WARN, "SAM-QFS: superblock size out of range %d",
		    sblk_size);
		error = EINVAL;
		err_line = __LINE__;
		goto out;
	}

	if (found_v2A_sblk || found_v2_sblk) {
		mp->mi.m_sblk_version = mp->mt.fi_version = SAMFS_SBLKV2;
	} else {
		mp->mi.m_sblk_version = mp->mt.fi_version = SAMFS_SBLKV1;
	}
	bp[isblk]->b_flags |= B_STALE | B_AGE;
	brelse(bp[isblk]);
	bp[isblk] = NULL;
	error = SAM_BREAD(mp, mp->mi.m_fs[0].dev,
	    mp->mi.m_sblk_offset[isblk], sblk_size, &sbp);
	if (error) {
		err_line = __LINE__;
		goto out;
	}
	mp->mi.m_sblk_size = sblk_size;
	mp->mi.m_sbp = (struct sam_sblk *)kmem_alloc(sblk_size, KM_SLEEP);
	bcopy((void *)sbp->b_un.b_addr, mp->mi.m_sbp, sblk_size);
	mp->mi.m_sblk_fsid = mp->mi.m_sbp->info.sb.init;
	mp->mi.m_sblk_fsgen = mp->mi.m_sbp->info.sb.gen;
	brelse(sbp);

out:
	if (error) {
		TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, 0);
	}
	for (i = 0; i < 2; i++) {
		if (bp[i] != NULL) {
			bp[i]->b_flags |= B_STALE | B_AGE;
			brelse(bp[i]);
		}
	}
	return (error);
}

/*
 * ----- sam_mount_setwm_blocks - Compute hi and low blocks.
 *	Initialize or recompute the high and low block counts,
 *	given the capacity, the hi and the low percentages.
 */
void
sam_mount_setwm_blocks(sam_mount_t *mp)
{
	offset_t capacity = mp->mi.m_sbp->info.sb.capacity;

	/* Blocks needed to satisfy high and low water marks */

	mp->mi.m_high_blk_count =
	    capacity - ((capacity * (int)mp->mt.fi_high) / 100);
	mp->mi.m_low_blk_count = capacity - ((capacity * (int)mp->mt.fi_low) /
	    100);

	/*
	 * This test seems backward, but the counts are in number of "free"
	 * blocks.
	 */
	if (mp->mi.m_low_blk_count < mp->mi.m_high_blk_count) {
		mp->mi.m_low_blk_count = mp->mi.m_high_blk_count;
	}
}


/*
 * ----- sam_report_initial_watermark - Set and report watermark state.
 *	Unconditionally sets and reports the current block levels to sam-amld.
 *  If blocks are between high and low, report as FS_LHWM (between and
 *	rising). Unconditional reports are desired at mount time, at sam-amld
 *	resync, and after changing the watermark values.
 */
void
sam_report_initial_watermark(sam_mount_t *mp)
{
	offset_t	space;
	fs_wmstate_e state;

	state = FS_LHWM;			/* assume in-between */
	space = mp->mi.m_sbp->info.sb.space;

	if (space > mp->mi.m_low_blk_count)
		state = FS_LWM;
	else if (space < mp->mi.m_high_blk_count)
		state = FS_HWM;
	/* send the news to sam-amld */
	sam_report_fs_watermark(mp, (int)state);
}
#endif /* sun */


/*
 * ----- sam_validate_sblk - Read and validate all superblock labels.
 * Loop through all partitions and set the error and state.
 * Return number of partitions, number of ON partitions, and number of meta
 * ON partitions in the superblock.
 */

static int			/* Superblock fs_count */
sam_validate_sblk(
	sam_mount_t *mp,	/* Pointer to mount table */
	struct samdent *devp,	/* Pointer to array of device partitions */
	int *sblk_meta_on,	/* Return superblock ON meta devices */
	int *sblk_data_on)	/* Return superblock ON data devices */
{
	int fs_count = 0;
	int mm_count = 0;
	int meta_on = 0;
	int data_on = 0;
	struct sam_sblk *sblk;
	struct samdent *dp;
	int i;
	int ord;
	sam_time_t time = 0;
#ifdef sun
	buf_t *bp;
#endif /* sun */
	char *buf = NULL;
	int error;
	int err_line = 0;
	boolean_t ck_meta = TRUE;


	if (SAM_IS_SHARED_FS(mp) && !SAM_IS_SHARED_SERVER_ALT(mp)) {
		ck_meta = FALSE;
	}
	for (i = 0, dp = devp; i < mp->mt.fs_count; dp++, i++) {
		ord = i;
		error = 0;
		if (!ck_meta && (dp->part.pt_type == DT_META) &&
		    strcmp(dp->part.pt_name, "nodev") == 0) {
			continue;
		}
		if (fs_count != 0) {	/* Processed first device, ord 0 */
			if (mp->mi.m_fs[i].part.pt_state == DEV_OFF ||
			    mp->mi.m_fs[i].part.pt_state == DEV_DOWN) {
				err_line = __LINE__;
				error = ENOENT;
				goto setpart2;
			}
		}
		if (dp->opened == 0) {
			err_line = __LINE__;
			error = ENOENT;
			goto setpart2;
		}
#ifdef sun
		if (is_osd_group(dp->part.pt_type)) {
			buf = kmem_alloc(sizeof (struct sam_sblk), KM_SLEEP);
			if ((error = sam_issue_object_io(mp, dp->oh, FREAD,
			    SAM_OBJ_SBLK_ID, UIO_SYSSPACE, buf,
			    0, sizeof (struct sam_sblk))) != 0) {
				err_line = __LINE__;
				goto setpart1;
			}
			sblk = (struct sam_sblk *)(void *)buf;
		} else {
			error = SAM_BREAD(mp, dp->dev,
			    SUPERBLK, sizeof (struct sam_sblk), &bp);
			if (error) {
				err_line = __LINE__;
				goto setpart2;
			}
			sblk = (struct sam_sblk *)(void *)bp->b_un.b_daddr;
		}
#endif /* sun */
#ifdef linux
		buf = kmem_alloc(sizeof (struct sam_sblk), KM_SLEEP);
		error = sam_bread(dp, SUPERBLK, sizeof (struct sam_sblk), buf);
		if (error) {
			err_line = __LINE__;
			goto setpart1;
		}
		sblk = (struct sam_sblk *)(void *)buf;
#endif /* linux */

		if (mp->mt.fi_status & FS_SRVR_BYTEREV) {
			/*
			 * Need to byte swap the samfs super block
			 */
			if (byte_swap_sb(sblk, sizeof (struct sam_sblk))) {
				cmn_err(CE_WARN,
				    "SAM-QFS: sam_build_geometry sb "
				    "byte swap error");
				err_line = __LINE__;
				error = EIO;
				goto setpart1;
			}
		}
		if (fs_count == 0) {		/* First device */
			int j;

			/*
			 * Set fs_count, mm_count & device on from first device.
			 * Store state from sblk in mount table in ordinal
			 * order. This is temporary and is needed because the
			 * ordinals may be reordered.
			 */
			fs_count = sblk->info.sb.fs_count;
			mm_count = sblk->info.sb.mm_count;
			for (j = 0; j < fs_count; j++) {
				mp->mi.m_fs[j].part.pt_state =
				    sblk->eq[j].fs.state;
				if ((sblk->eq[j].fs.state == DEV_ON) ||
				    (sblk->eq[j].fs.state == DEV_NOALLOC)) {
					if (sblk->eq[j].fs.type == DT_META) {
						meta_on++;
					} else {
						data_on++;
					}
				}
			}
			mp->mi.m_sblk_offset[0] = sblk->info.sb.offset[0];
			mp->mi.m_sblk_offset[1] = sblk->info.sb.offset[1];
			time = sblk->info.sb.init;
		}

		/*
		 * Validate label is same for all members & not duplicated.
		 */
		ord = sblk->info.sb.ord;
		if (sblk->info.sb.magic == SAM_MAGIC_V1_RE ||
		    sblk->info.sb.magic == SAM_MAGIC_V2_RE ||
		    sblk->info.sb.magic == SAM_MAGIC_V2A_RE) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: Eq %d superblock "
			    "byte-reversed; host not client",
			    mp->mt.fi_name, dp->part.pt_eq);
			err_line = __LINE__;
			error = EILSEQ;		/* "illegal byte sequence" */
		} else if (bcmp(sblk->info.sb.name, "SBLK", 4) != 0) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: mcf eq %d: no superblock found",
			    mp->mt.fi_name, dp->part.pt_eq);
				err_line = __LINE__;
				error = EINVAL;
		} else if (time != sblk->info.sb.init) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: mcf eq %d:"
			    " superblock FsId does not match eq %d FsId",
			    mp->mt.fi_name, dp->part.pt_eq, devp->part.pt_eq);
			err_line = __LINE__;
			error = EINVAL;
		} else if (strcmp(mp->mt.fi_name, sblk->info.sb.fs_name) != 0) {
			cmn_err(CE_WARN,
			"SAM-QFS: %s: mcf eq %d: superblock "
			"FS name = '%s' != mcf FS name",
			    mp->mt.fi_name, dp->part.pt_eq,
			    sblk->info.sb.fs_name);
			if (!(mp->mt.fi_mflag & MS_RDONLY)) {
				err_line = __LINE__;
				error = EINVAL;
			}
		} else if (ord < 0 || ord >= fs_count) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: mcf eq %d:"
			    " superblock ordinal (%d) out of range [0,%d)",
			    mp->mt.fi_name, dp->part.pt_eq,
			    ord, fs_count);
			err_line = __LINE__;
			error = EINVAL;
		} else if (mp->mi.m_fs[ord].dev != 0) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: mcf eq %d: duplicate device in mcf",
			    mp->mt.fi_name, dp->part.pt_eq);
			err_line = __LINE__;
			error = EINVAL;
		} else if (dp->part.pt_type != sblk->eq[ord].fs.type) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: mcf eq %d: superblock "
			    "type (%x) != mcf type (%x)",
			    mp->mt.fi_name, dp->part.pt_eq,
			    sblk->eq[ord].fs.type, dp->part.pt_type);
			err_line = __LINE__;
			error = EINVAL;
		} else if (sblk->info.sb.fs_count != fs_count) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: mcf eq %d: eq count (%d) !="
			    " superblock eq count (%d)",
			    mp->mt.fi_name, dp->part.pt_eq,
			    sblk->info.sb.fs_count, fs_count);
			err_line = __LINE__;
			error = EINVAL;
		} else if (sblk->info.sb.mm_count != mm_count) {
			cmn_err(CE_WARN,
			    "SAM-QFS: %s: mcf eq %d: meta count (%d) !="
			    " superblock meta count (%d)",
			    mp->mt.fi_name, dp->part.pt_eq,
			    sblk->info.sb.mm_count, mm_count);
			err_line = __LINE__;
			error = EINVAL;
		} else if (dp->part.pt_eq != sblk->eq[ord].fs.eq) {
			cmn_err(CE_WARN,
			"SAM-QFS: %s: mcf eq %d: eq mismatch "
			"in superblock (eq = %d)",
			    mp->mt.fi_name, dp->part.pt_eq,
			    sblk->eq[ord].fs.eq);
			/* not a fatal error */
		}

		/*
		 * Set capacity & space for OSDs in sblk. Set in sam_getdev.
		 */
		if (is_osd_group(dp->part.pt_type)) {
			sblk->eq[i].fs.capacity = dp->part.pt_capacity;
			sblk->eq[i].fs.space = dp->part.pt_space;
		}

		dp->part.pt_capacity = (fsize_t)sblk->eq[i].fs.capacity;
		dp->part.pt_space = (fsize_t)sblk->eq[i].fs.space;
setpart1:

#ifdef sun
		if (is_osd_group(dp->part.pt_type)) {
			kmem_free(buf, sizeof (struct sam_sblk));
		} else {
			bp->b_flags |= B_STALE | B_AGE;
			brelse(bp);
		}
#endif /* sun */
#ifdef linux
		kfree(buf);
#endif /* linux */

setpart2:
		/*
		 * Set device state based on error. Set state for devices
		 * with no error from the superblock (on, noalloc).
		 */
		if (error) {
			dp->part.pt_state = DEV_OFF;
			dp->skip_ord = 1;
			dp->error = error;
			TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, ord);
		} else {
			int state;

			dp->error = 0;
			state = mp->mi.m_fs[ord].part.pt_state;
			bcopy((void *)dp, (void *)&mp->mi.m_fs[ord],
			    sizeof (*dp));
			mp->mi.m_fs[ord].part.pt_state = state;
		}
	}
	*sblk_meta_on = meta_on;
	*sblk_data_on = data_on;
	return (fs_count);
}


/*
 * ----- sam_build_geometry - Build disk family set geometry.
 * Read file system labels and build the geometry for the disk
 * family set in the mount entry.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_build_geometry(
	sam_mount_t *mp)
{
	struct samdent *devp;
	struct samdent *dp;
	int	i, j;
	int sblk_fs_count = 0;
	int sblk_meta_on = 0;
	int sblk_data_on = 0;
	int meta_on = 0;
	int data_on = 0;
	int error = 0;
	int err_line = 0;
	boolean_t ck_meta = TRUE;

	/*
	 * Save device entries; they may be re-ordered in the mount table.
	 */
	if ((devp = (struct samdent *)kmem_zalloc(
	    (sizeof (struct samdent) * mp->mt.fs_count),
	    KM_NOSLEEP)) == NULL) {
		return (ENOMEM);
	}
	bcopy((void *)&mp->mi.m_fs, (void *)devp,
	    (sizeof (struct samdent)*mp->mt.fs_count));
	bzero((void *)&mp->mi.m_fs,
	    (sizeof (struct samdent) * mp->mt.fs_count));

	/*
	 * Set state and error for all entries in the mcf. Read superblock and
	 * verify mcf matches superblock. Move ON devices into the device
	 * array in correct ordinal order.
	 */
	sblk_fs_count = sam_validate_sblk(mp, devp, &sblk_meta_on,
	    &sblk_data_on);

	if (SAM_IS_SHARED_FS(mp) && !SAM_IS_SHARED_SERVER_ALT(mp)) {
		ck_meta = FALSE;
	}

	/*
	 * Check for the correct number of data and metadata devices.
	 * Verify the dev is set.
	 */
	for (i = 0, dp = devp; i < sblk_fs_count; dp++, i++) {
		if ((dp->part.pt_state == DEV_ON) ||
		    (dp->part.pt_state == DEV_NOALLOC)) {
			if (dp->dev == 0) {
				continue;
			}
			if (dp->part.pt_type == DT_META) {
				meta_on++;
			} else {
				data_on++;
			}
		}
	}
	if (data_on != sblk_data_on) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: missing data "
		    "devices (%d/%d) in configuration",
		    mp->mt.fi_name, data_on, sblk_data_on);
		err_line = __LINE__;
		error = EINVAL;
	}
	if (meta_on != sblk_meta_on) {
		cmn_err(CE_WARN,
		    "SAM-QFS: %s: missing metadata "
		    "devices (%d/%d) in configuration",
		    mp->mt.fi_name, meta_on, sblk_meta_on);
		err_line = __LINE__;
		error = EINVAL;
	}

	/*
	 * If client only, move possible nodev entries into the device
	 * array.
	 */
	if (!ck_meta) {				/* Process nodev entries */
		j = 0;
		for (i = 0, dp = devp; i < sblk_fs_count; dp++, i++) {
			if ((dp->part.pt_type == DT_META) &&
			    strcmp(dp->part.pt_name, "nodev") == 0) {
				/*
				 * No real device for this entry.  Find
				 * the next pseudo-device and copy nodev
				 * entry into the device array.
				 */
				while (j < sblk_fs_count &&
				    (devp[j].part.pt_type != DT_META ||
				    strcmp(devp[j].part.pt_name, "nodev"))) {
					j++;	/* skip real entries */
				}
				if (j == sblk_fs_count) {
					cmn_err(CE_WARN,
					"SAM-QFS: %s: too many metadata "
					"devices (%d) in configuration",
					    mp->mt.fi_name, j);
					err_line = __LINE__;
					error = EINVAL;
					break;
				}
				bcopy((void *)&devp[j], (void *)&mp->mi.m_fs[i],
				    sizeof (*devp));
				j++;
			}
		}
	}

	/*
	 * Fill in the device array for OFF/DOWN ordinals in the superblock.
	 * Copy the remaining ordinals (new) which were not in the superblock.
	 */
	for (i = 0, dp = devp; i < mp->mt.fs_count; dp++, i++) {
		if ((dp->error == 0) && (i < sblk_fs_count)) {
			continue;	/* Already copied into device array */
		}
		for (j = 0; j < mp->mt.fs_count; j++) {
			if (mp->mi.m_fs[j].part.pt_type == 0) {
				bcopy((void *)dp, (void *)&mp->mi.m_fs[j],
				    sizeof (*dp));
				break;
			}
		}
	}

	/*
	 * If error, restore device array so devices can be closed.
	 */
	if (error) {
		bcopy((void *)devp, (void *)&mp->mi.m_fs,
		    (sizeof (struct samdent)*mp->mt.fs_count));
		TRACE(T_SAM_MNT_ERRLN, NULL, err_line, error, 0);
	}
	kmem_free((void *) devp, (sizeof (struct samdent) * mp->mt.fs_count));
	return (error);
}


/*
 * ----- sam_unmount_fs -  Unmount/Failover this file system.
 *	Wait for deferred nfs inodes and shared fs lease list to complete.
 *	Flush inodes. Kill threads based on unmount or failover.
 *	Update with SYNC_CLOSE.
 */

#ifdef sun

int					/* ERRNO if error, 0 if successful. */
sam_unmount_fs(
	sam_mount_t *mp,		/* Mount pointer */
	int fflag,			/* Force flag */
	sam_unmount_flag_t flag)	/* Unmount flag - umount or failover */
{
	vfs_t *vfsp;
	vnode_t *vp;
	int e;
	boolean_t dfhold;
	int retry_cnt;
	int error = 0;
	clock_t vn_wait_time;

	vfsp = mp->mi.m_vfsp;

	if (mp->mt.fi_type == DT_META_OBJ_TGT_SET) {
		if (mp->mi.m_osdfs_root) {
			vp = SAM_ITOV(mp->mi.m_osdfs_root);
			VN_RELE(vp);
			mp->mi.m_osdfs_root = NULL;
		}

		if (mp->mi.m_osdfs_part) {
			vp = SAM_ITOV(mp->mi.m_osdfs_part);
			VN_RELE(vp);
			mp->mi.m_osdfs_part = NULL;
		}

	}

	mutex_enter(&mp->mi.m_inode.mutex);
	mp->mi.m_inode.flag = SAM_THR_UMOUNTING;
	mutex_exit(&mp->mi.m_inode.mutex);

	/*
	 * Wait for the sam_expire_server_leases thread to remove all
	 * vnodes with leases from the server lease list and move them
	 * to one or more sam_release_vnodes threads.
	 */
#ifdef	METADATA_SERVER
	while ((mp->mi.m_sr_lease_chain.forw !=
	    (sam_lease_ino_t *)(void *)&mp->mi.m_sr_lease_chain)) {
		sam_sched_expire_server_leases(mp, 0, TRUE);
		delay(hz/4);
	}
#endif	/* METADATA_SERVER */

	/*
	 * Wait for all delayed inactive vnodes to be
	 * moved to one or more sam_release_vnodes threads.
	 */
	mutex_enter(&mp->mi.m_inode.mutex);
	while (mp->mi.m_invalp != NULL) {
		mutex_exit(&mp->mi.m_inode.mutex);
		sam_taskq_add(sam_inactivate_inodes, mp, NULL, 0);
		delay(hz/4);
		mutex_enter(&mp->mi.m_inode.mutex);
	}
	mutex_exit(&mp->mi.m_inode.mutex);

	/*
	 * Wait for all VN_RELEs to be completed on vnodes
	 * that are being processed by the sam_release_vnodes
	 * thread(s) that were created by the
	 * sam_expire_server_leases thread and the
	 * sam_inactivate_inodes thread.
	 */
	mutex_enter(&mp->ms.m_fo_vnrele_mutex);
	while (mp->ms.m_fo_vnrele_count > 0) {
		vn_wait_time = lbolt + (2*hz);
		cv_timedwait(&mp->ms.m_fo_vnrele_cv,
		    &mp->ms.m_fo_vnrele_mutex, vn_wait_time);
	}
	mutex_exit(&mp->ms.m_fo_vnrele_mutex);

	/*
	 * Wait for the release block list.
	 */
#ifdef	METADATA_SERVER
	sam_wait_release_blk_list(mp);
#endif	/* METADATA_SERVER */

	/*
	 * Stop any scheduled tasks.  (If failover, or if the unmount fails,
	 * we'll re-enable them before returning.)
	 */
	sam_taskq_stop_mp(mp);

	if ((error = sam_flush_ino(vfsp, flag, fflag)) != 0) {
		TRACE(T_SAM_UMNT_RET, vfsp, 3, (sam_tr_t)mp, error);
		if ((fflag & MS_FORCE) == 0) {
			goto busy;
		}
	}

	if (TRANS_ISTRANS(mp)) {
		(void) lqfs_flush(mp);
	}

	/*
	 * Wait a bit for daemon threads to finish decrementing the syscall
	 * count.
	 */
	dfhold = FALSE;
	retry_cnt = SYSCALL_CNT_RETRIES;
retry:
	mutex_enter(&mp->ms.m_waitwr_mutex);
	if (mp->ms.m_syscall_cnt) {
		mutex_exit(&mp->ms.m_waitwr_mutex);
		if (retry_cnt) {
			retry_cnt--;
			delay(hz);
			goto retry;
		}
		error = EBUSY;
		TRACE(T_SAM_UMNT_RET, vfsp, 4, (sam_tr_t)mp, error);
		if ((fflag & MS_FORCE) == 0) {
			goto busy;
		}
		dfhold = mp->ms.m_sysc_dfhold;
		mp->ms.m_sysc_dfhold = FALSE;
	} else {
		mutex_exit(&mp->ms.m_waitwr_mutex);
	}
	if (dfhold) {
		/*
		 * There are active threads in the FS.  Convert the deferred
		 * hold to a real hold to avoid premature release.
		 */
		SAM_VFS_HOLD(mp);
	}
	TRACE(T_SAM_UMNT_RET, vfsp, 5, (sam_tr_t)mp, mp->ms.m_syscall_cnt);

	/*
	 * Kill the inode allocator thread and the block allocator thread.
	 */
	sam_stop_threads(mp);

	mutex_enter(&mp->ms.m_synclock);
	e = sam_update_filsys(mp, SYNC_CLOSE);
	mutex_exit(&mp->ms.m_synclock);
	if (e) {
		error = e;
	}

	TRACE(T_SAM_UMNT_RET, vfsp, 6, (sam_tr_t)mp, error);
	if (flag == SAM_FAILOVER_OLD_SRVR) {
		sam_taskq_start_mp(mp);
	} else if (error && (fflag & MS_FORCE) == 0) {
		/* If umount failed, restart inode & block allocator threads */
		mutex_enter(&mp->mi.m_inode.mutex);
		mp->mi.m_inode.flag = SAM_THR_MOUNT;
		mutex_exit(&mp->mi.m_inode.mutex);
		sam_taskq_start_mp(mp);
	}
	return (error);

busy:
	mutex_enter(&mp->mi.m_inode.mutex);
	mp->mi.m_inode.flag = SAM_THR_MOUNT;
	mutex_exit(&mp->mi.m_inode.mutex);

	sam_taskq_start_mp(mp);

	return (error);
}
#endif /* sun */


#ifdef linux

int					/* ERRNO if error, 0 if successful. */
sam_unmount_fs(
	sam_mount_t *mp,		/* Mount pointer */
	int fflag,			/* Force flag */
	sam_unmount_flag_t flag)	/* Unmount flag - umount or failover */
{
	struct super_block *vfsp;
	boolean_t dfhold;
	int retry_cnt;
	int error = 0;

	vfsp = mp->mi.m_vfsp;

	mutex_enter(&mp->mi.m_inode.mutex);
	mp->mi.m_inode.flag = SAM_THR_UMOUNTING;
	mutex_exit(&mp->mi.m_inode.mutex);

	/*
	 * Stop any scheduled tasks.
	 */
	sam_taskq_stop_mp(mp);

	/*
	 * Wait for daemon threads to finish decrementing the syscall count.
	 */
	dfhold = FALSE;
	retry_cnt = SYSCALL_CNT_RETRIES;
retry:
	mutex_enter(&mp->ms.m_waitwr_mutex);
	if (mp->ms.m_syscall_cnt) {
		mutex_exit(&mp->ms.m_waitwr_mutex);
		if (retry_cnt) {
			retry_cnt--;
			delay(hz);
			goto retry;
		}
		error = EBUSY;
		TRACE(T_SAM_UMNT_RET, vfsp, 4, (sam_tr_t)mp, error);
		retry_cnt = SYSCALL_CNT_RETRIES;
		goto retry;
	}
	mutex_exit(&mp->ms.m_waitwr_mutex);

	TRACE(T_SAM_UMNT_RET, vfsp, 6, (sam_tr_t)mp, 0);

	return (0);
}
#endif /* linux */


/*
 * ----- sam_cleanup_mount -  Unmount or error on mount processing.
 *  Close devices and free memory for unmount or error on mount tables.
 */

#ifdef sun

void
sam_cleanup_mount(
	sam_mount_t *mp,	/* Pointer to the mount table. */
	pathname_t *pnp,	/* The pathname pointer. */
	cred_t *credp)		/* Credentials pointer. */
{
	int filemode;

	if (mp != NULL) {
		if (mp->mi.m_sbp) {			/* Superblock buffer */
			kmem_free(mp->mi.m_sbp, mp->mi.m_sblk_size);
			mp->mi.m_sbp = NULL;
		}
		filemode =
		    (mp->mi.m_vfsp->vfs_flag & VFS_RDONLY) ?
		    FREAD : FREAD | FWRITE;
		sam_close_devices(mp, 0, filemode, credp);
	}

	if (pnp != NULL) {
		pn_free(pnp);
	}
}


/*
 * ----- sam_close_devices -  Unmount or error on mount processing.
 *	If the device was opened, close block special files,
 *	Flush buffers and invalidate cache for all devices.
 *	Free memory for unmount or error on mount tables.
 */

void
sam_close_devices(
	sam_mount_t *mp,	/* Pointer to the mount table. */
	int istart,		/* Starting dev position in the mount table */
	int filemode,		/* Open mode */
	cred_t *credp)		/* Credentials pointer. */
{
	int i;
	dev_t dev;
	vnode_t	*svp;
	struct samdent *dp;

	for (i = istart, dp = &mp->mi.m_fs[istart]; i < mp->mt.fs_count;
	    i++, dp++) {
		if ((svp = dp->svp) == (vnode_t *)0) {
			continue;
		}
		(void) VOP_PUTPAGE_OS(svp, 0, 0, B_INVAL, credp, NULL);
		TRACE(T_SAM_DEV_CLOSE, mp, dp->part.pt_eq, dp->part.pt_state,
		    dp->error);
		if (is_osd_group(dp->part.pt_type)) {
			if (!dp->opened) {
				continue;
			}
			sam_close_osd_device(dp->oh, filemode, credp);
			if (!(mp->mt.fi_status & FS_FAILOVER)) {
				dp->opened = 0;
				continue;
			}
			dp->error = sam_open_osd_device(dp, filemode, credp);
		} else {
			if (dp->opened) {
				VOP_CLOSE_OS(svp, filemode, 1, (offset_t)0,
				    credp, NULL);
			} else {
				VN_RELE(svp);		/* "nodev" specvp */
			}
			dev = dp->dev;
			bflush(dev);
			bfinval(dev, 1);
			if (!dp->opened) {
				continue;
			}
			if (!(mp->mt.fi_status & FS_FAILOVER)) {
				VN_RELE(svp);
				dp->opened = 0;
				continue;
			}
			(void) VOP_OPEN_OS(&dp->svp, filemode, credp, NULL);
		}
		TRACE(T_SAM_DEV_OPEN, mp, dp->part.pt_eq,
		    dp->part.pt_state, dp->error);
	}
}
#endif /* sun */


#ifdef linux

void
sam_cleanup_mount(
	sam_mount_t *mp,	/* Pointer to the mount table. */
	void *pnp,		/* Unused by Linux */
	cred_t *credp)		/* Credentials pointer. */
{
	int	ord;
	dev_t dev;
	struct samdent *fs_dent;
	struct file *fp = NULL;
	struct super_block *vfsp = NULL;

	if (mp != NULL) {
		vfsp = mp->mi.m_vfsp;
		if (mp->mi.m_sbp) {			/* Superblock buffer */
			kmem_free(mp->mi.m_sbp, mp->mi.m_sblk_size);
			mp->mi.m_sbp = NULL;
		}
		for (ord = 0; ord < mp->mt.fs_count; ord++) {
			fs_dent = &mp->mi.m_fs[ord];
			TRACE(T_SAM_UMNT_RET, vfsp, 20, fs_dent->dev, ord);
			if ((fp = fs_dent->fp) != NULL) {
				filp_close(fp, NULL);
				fs_dent->fp = NULL;
				dev = fs_dent->dev;
				TRACE(T_SAM_UMNT_RET, mp->mi.m_vfsp,
				    22, dev, ord);
			}
		}
		TRACE(T_SAM_UMNT_RET, vfsp, 24, (sam_tr_t)mp, 0);
	}

	TRACE(T_SAM_UMNT_RET, vfsp, 25, (sam_tr_t)mp, 0);
}
#endif /* linux */


/*
 * ----- sam_flush_ino -  Flush cached inodes.
 *	Flush cached inodes on unmount. Clear filsys pointers.
 */

#ifdef sun
int					/* ERRNO if error, 0 if successful. */
sam_flush_ino(
	struct vfs *vfsp,		/* this instance of mount. */
	sam_unmount_flag_t flag,	/* Unmount flag - umount or failover */
	int fflag)			/* Force flag - unmount only */
{
	int	error = 0;
	sam_mount_t	*mp;		/* pointer to the mount table. */
	sam_ihead_t	*hip;		/* pointer to the hash pointer */
	sam_node_t *ip;
	int	ihash;

	mp = (sam_mount_t *)(void *)vfsp->vfs_data;

	/* Pre-cycle through hash chains, clearing unmount-scanned flag. */

	hip = (sam_ihead_t *)&samgt.ihashhead[0];
	ASSERT(hip != NULL);
	for (ihash = 0; ihash < samgt.nhino; ihash++, hip++) {
		kmutex_t *ihp;

		ihp = &samgt.ihashlock[ihash];
		mutex_enter(ihp);
		for (ip = hip->forw;
		    (ip != (sam_node_t *)(void *)hip);
		    ip = ip->chain.hash.forw) {
			if (ip->mp == mp) {
				ip->hash_flags &= ~SAM_HASH_FLAG_UNMOUNT;
			}
		}
		mutex_exit(ihp);
	}

	/*
	 * Solaris calls dnlc_purge_vfsp() on the FS prior to calling the
	 * unmount, but doesn't lock out entry by active procs.  Call it
	 * again now that we've set FS_UMOUNT_IN_PROGRESS and closed off
	 * further entry.
	 */
	dnlc_purge_vfsp(vfsp, 0);

	/*
	 * Cycle through hash chains.  If unmounting, verify only
	 * appropriate inodes active.  If failing over, push everything
	 * we can.  Restart from the beginning of the hash chain
	 * whenever we release the hash chain lock.
	 */
	hip = (sam_ihead_t *)&samgt.ihashhead[0];
	for (ihash = 0; ihash < samgt.nhino; ihash++, hip++) {
		kmutex_t *ihp;
		sam_node_t *nip;

		ihp = &samgt.ihashlock[ihash];
		mutex_enter(ihp);
		for (ip = hip->forw;
		    (ip != (sam_node_t *)(void *)hip); ip = nip) {
			vnode_t	*vp;

			nip = ip->chain.hash.forw;

			if (ip->mp != mp) { /* Not under this mount point? */
				continue;
			}
			if (ip->hash_flags & SAM_HASH_FLAG_UNMOUNT) {
				/* Already seen? */
				continue;
			}
			ip->hash_flags |= SAM_HASH_FLAG_UNMOUNT;

			vp = SAM_ITOV(ip);
			if ((ip->di.id.ino == SAM_INO_INO) ||
			    (ip->di.id.ino == SAM_ROOT_INO) ||
			    (ip->di.id.ino == SAM_BLK_INO) ||
			    sam_is_quota_inode(mp, ip)) {
				if (ip->di.id.ino == SAM_ROOT_INO) {
					dnlc_dir_purge(&ip->i_danchor);
					TRACE(T_SAM_EDNLC_PURGE, vp,
					    ip->di.id.ino, 5, 0);
				}
				if ((int)vp->v_count > 1) {
					TRACE(T_SAM_BUSY, vp, ip->di.id.ino,
					    vp->v_count,
					    ip->flags.bits);
					error = EBUSY;
				}
				if (flag != SAM_UNMOUNT_FS) {
					error = 0;
					ip->cl_ino_seqno = 0;
					ip->cl_srvr_ord = mp->ms.m_server_ord;
				}
				continue;
			}
			if (RW_TRYENTER_OS(&ip->inode_rwl, RW_WRITER) == 0) {
				/*
				 * Could not get writer lock, block until lock
				 * obtained.
				 */
				mutex_exit(ihp);
				RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
				mutex_enter(ihp);
				nip = hip->forw;
			}
			/*
			 * Issue note on held files.
			 */
			if (ip->flags.b.hold_blocks) {
				cmn_err(CE_NOTE,
				    "SAM-QFS: %s: Inode %d: held by SAN,"
				    " refcnt = %d.",
				    mp->mt.fi_name, ip->di.id.ino, vp->v_count);
			}
			sam_quota_inode_fini(ip);

			if (flag == SAM_FAILOVER_POST_PROCESSING) {
				ip->flags.bits &= ~SAM_NORECLAIM;
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				continue;

			} else if ((flag == SAM_FAILOVER_OLD_SRVR) ||
			    (flag == SAM_FAILOVER_NEW_SRVR)) {
				int held;

				ip->cl_ino_seqno = 0;
				ip->cl_srvr_ord = mp->ms.m_server_ord;
				ip->size_owner = 0;
				ip->write_owner = 0;
				held = FALSE;
				if (vp->v_count == 0) {
					/* Putpage needs active vnode */
					held = TRUE;
					VN_HOLD(vp);
				}
				if (flag == SAM_FAILOVER_NEW_SRVR) {
					/* New server */
					buf_t *bp;
					struct sam_perm_inode *permip;
					sam_id_t id;
					offset_t real_size;
					int flags;

					sam_clear_map_cache(ip);
					id = ip->di.id;
					real_size = ip->di.rm.size;
					if ((error = sam_read_ino(mp,
					    ip->di.id.ino, &bp,
					    &permip)) == 0) {
						int cmd;
						sam_ino_record_t irec;

						/*
						 * Force the reset of the inode
						 * information from the disk
						 * copy. This also restarts the
						 * sequence numbers.
						 */
						irec.in.seqno = 0;
						irec.in.srvr_ord =
						    mp->ms.m_server_ord;
						irec.in.ino_gen = 0;
						irec.sr_attr.actions = 0;
						irec.sr_attr.current_size =
						    ip->size;
						irec.sr_attr.stage_size =
						    ip->stage_size;
						irec.sr_attr.alloc_size =
						    ip->cl_allocsz;
						irec.di = permip->di;
						irec.di2 = permip->di2;
						ip->flags.b.changed = 1;
						if ((ip->cl_leases |
						    ip->cl_saved_leases) &
						    CL_APPEND) {
							cmd = SAM_CMD_LEASE;
				irec.sr_attr.size_owner = mp->ms.m_client_ord;
							ip->di.rm.size =
							    real_size;
						} else {
							cmd = SAM_CMD_INODE;
						irec.sr_attr.size_owner = 0;
						}
						if (ip->di.status.b.offline) {
							irec.sr_attr.stage_err =
							    ip->stage_err;
						}
						error = sam_reset_client_ino(
						    ip, cmd, &irec);
						ip->cl_allocsz = ip->di.rm.size;
						ip->cl_attr_seqno = 0;
						bp->b_flags |= B_STALE | B_AGE;
						brelse(bp);
					} else {
						cmn_err(CE_WARN,
						    "SAM-QFS: %s: Cannot "
						    "read ino %d.%d"
						    " during failover error=%d",
						    mp->mt.fi_name,
						    ip->di.id.ino,
						    ip->di.id.gen,
						    error);
					}
					if (ip->di.id.ino != id.ino ||
					    ip->di.id.gen != id.gen) {
						ip->flags.bits |= SAM_STALE;
						flags = (B_INVAL|B_TRUNC);
					} else if (S_ISDIR(ip->di.mode)) {
						flags = (B_INVAL|B_TRUNC);
					} else {
						flags = B_INVAL;
					}
					mutex_exit(ihp);
					sam_flush_pages(ip, flags);
					mutex_enter(ihp);
					nip = hip->forw;
				} else { /* flag == SAM_FAILOVER_OLD_SRVR */
					mutex_exit(ihp);
					sam_flush_pages(ip, B_INVAL);
					(void) sam_update_inode(ip,
					    SAM_SYNC_ONE, FALSE);
					sam_clear_map_cache(ip);
					ip->cl_allocsz = ip->di.rm.size;
					(void) sam_stale_indirect_blocks(ip, 0);
					if (S_ISDIR(ip->di.mode)) {
						dnlc_dir_purge(&ip->i_danchor);
						TRACE(T_SAM_EDNLC_PURGE, vp,
						    ip->di.id.ino, 6, 0);
					}
					mutex_enter(ihp);
					nip = hip->forw;
				}
				if (held) {
					mutex_enter(&vp->v_lock);
					vp->v_count--;
					mutex_exit(&vp->v_lock);
				}
				mutex_enter(&ip->write_mutex);
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				while (ip->cnt_writes) {
					ip->wr_fini++;
					cv_wait(&ip->write_cv,
					    &ip->write_mutex);
					if (--ip->wr_fini < 0) {
						ip->wr_fini = 0;
					}
				}
				mutex_exit(&ip->write_mutex);
				continue;
			} else {
				boolean_t hash_dropped;

				if (S_ISDIR(ip->di.mode)) {
					dnlc_dir_purge(&ip->i_danchor);
					TRACE(T_SAM_EDNLC_PURGE, vp,
					    ip->di.id.ino, 7, 0);
				}
				if (sam_flush_and_destroy_vnode(vp, ihp,
				    &hash_dropped) != 0) {
					RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
					error = EBUSY;
					if (fflag & MS_FORCE) {
						if (!hash_dropped) {
							ASSERT(
							    ip->flags.b.hash);
						}
						/*
						 * Remove from hash list.  All
						 * outstanding references must
						 * remain; any closes and/or
						 * chdirs will remove references
						 * and allow an eventual free.
						 * We don't want any future
						 * procs finding this inode and
						 * picking up the reference to
						 * its soon-to-be-staled mp.
						 */
						if (ip->flags.b.hash) {
							SAM_DESTROY_OBJ_LAYOUT(
							    ip);
							SAM_UNHASH_INO(ip);
						}
						sam_stale_inode(ip);
					}
				}

				/*
				 * sam_flush_and_destroy_vnode may drop & re-get
				 * hash lock.
				 */
				if (hash_dropped) {
					nip = hip->forw;
				}
			}
		}
		mutex_exit(ihp);
	}

	/*
	 * Push out all the quota records.
	 */
	sam_quota_halt(mp);

	/*
	 * There may still be inodes from this file system on the free list.
	 * Destroy them if we're unmounting.  Since they are unhashed, there
	 * is no way for another thread to get a reference to them.
	 *
	 * Q: If we're failing over, do we need to flush pages as above for
	 *    these inodes?
	 */

	if (flag == SAM_UNMOUNT_FS) {
		sam_node_t *list_to_destroy = NULL;
		sam_node_t *undestroyed_list = NULL;
		sam_node_t *nip;

		/*
		 * We cannot hold the free list lock while destroying a vnode.
		 * Instead, build a list of vnodes to be destroyed, then release
		 * the lock and destroy them all.
		 */

		mutex_enter(&samgt.ifreelock);

		for (ip = samgt.ifreehead.free.forw;
		    ip != (sam_node_t *)(void *)&samgt.ifreehead;
		    ip = nip) {
			nip = ip->chain.free.forw;
			if (ip->mp == mp) {
				sam_remove_freelist(ip);
				ip->chain.free.forw = list_to_destroy;
				list_to_destroy = ip;
			}
		}

		mutex_exit(&samgt.ifreelock);

		while (list_to_destroy != NULL) {
			boolean_t hash_dropped;

			ip = list_to_destroy;
			list_to_destroy = ip->chain.free.forw;
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			if (sam_flush_and_destroy_vnode(SAM_ITOV(ip), NULL,
			    &hash_dropped) != 0) {
				if (fflag & MS_FORCE) {
					sam_stale_inode(ip);
				}
				RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
				ip->chain.free.forw = undestroyed_list;
				undestroyed_list = ip;
				error = EBUSY;
			}
		}

		/*
		 * If we could not destroy all inodes (probably because a page
		 * could not be flushed), link remaining inodes back onto the
		 * free list.  If we're doing a forced unmount, do NOT put them
		 * back on the list.  We can't allow them to be looked up or
		 * re-used.
		 */

		if ((fflag & MS_FORCE) == 0) {
			while (undestroyed_list != NULL) {
				ip = undestroyed_list;
				undestroyed_list = ip->chain.free.forw;
				ip->chain.free.forw = ip;
				sam_put_freelist_tail(ip);
			}
		}
	}

	TRACE(T_SAM_FLUSH_RET, vfsp, error, 0, 0);
	return (error);
}
#endif /* sun */


/*
 * ----- sam_delete_ino -  unhash inode.
 * Remove from hash chain, sync, and invalidate pages.
 * inode_rwl WRITERS lock set on entry and exit.
 * NOTE: ihashlock must be acquired before vp->v_lock.
 */

#ifdef sun
int			/* ERRNO if error, 0 if successful. */
sam_delete_ino(vnode_t *vp)
{
	int error = 0;
	sam_node_t *ip = SAM_VTOI(vp);
	kmutex_t *ihp;
	int got_mutex = 0;

	ihp = &samgt.ihashlock[SAM_IHASH(ip->di.id.ino, ip->dev)];
	if (mutex_owner(ihp) != curthread) {
		mutex_enter(ihp);
		got_mutex = 1;
	}
	if (ip->flags.b.hash) {
		SAM_DESTROY_OBJ_LAYOUT(ip);
		SAM_UNHASH_INO(ip);		/* Remove from hash queue */
	}
	VN_HOLD(vp);
	ip->flags.b.accessed = 1;
	ip->flags.b.busy = 1;
	mutex_exit(ihp);

	/*
	 * If regular offline stage_n file, release stage_n blocks.
	 */
	ip->flags.b.staging = 0;
	if (ip->di.status.b.direct) {
		sam_free_stage_n_blocks(ip);
	}

	mutex_enter(ihp);
	mutex_enter(&vp->v_lock);
	vp->v_count--;
	if ((int)vp->v_count > 0) {
		TRACE(T_SAM_FLUSHERR2, vp, ip->di.id.ino,
		    vp->v_count, ip->flags.bits);
		error = EBUSY;
	}
	mutex_exit(&vp->v_lock);
	if (got_mutex) {
		mutex_exit(ihp);
	}
	return (error);
}
#endif /* sun */


#ifdef sun
/*
 * ----- sam_destroy_vnode -  Destroy a SAM-QFS vnode.
 * NOTE. hash lock set on entry and exit.  inode_rwl lock set on
 * entry and cleared and deleted on exit, if no error.
 */

static int				/* ERRNO if error, 0 if successful. */
sam_destroy_vnode(vnode_t *vp, int fflag)
{
	sam_node_t *ip = SAM_VTOI(vp);
	int error;

	ASSERT(ip != NULL);
	TRACE(T_SAM_FLUSH, vp, ip->di.id.ino, vp->v_count, ip->flags.bits);
	if ((fflag & MS_FORCE) == 0) {
		ASSERT(!vn_has_cached_data(vp));
	}
	if ((error = sam_delete_ino(vp)) != 0) {
		return (error);
	}
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	sam_destroy_ino(ip, FALSE);
	return (0);
}
#endif /* sun */


#ifdef linux
/*
 * ----- sam_destroy_inode -  Destroy a SAM-QFS inode.
 * NOTE. hash lock set on entry and exit.  inode_rwl lock set on
 * entry and cleared and deleted on exit, if no error.
 */
static int				/* ERRNO if error, 0 if successful. */
sam_destroy_inode(struct inode *li)
{
	sam_node_t *ip = SAM_LITOSI(li);

	TRACE(T_SAM_FLUSH, li, ip->di.id.ino,
	    atomic_read(&li->i_count), ip->flags.bits);
	RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);

	iput(li);

	return (0);
}
#endif /* linux */

#ifdef sun
/*
 * ----- sam_flush_and_destroy_vnode - Flush a vnode's pages, then destroy it.
 * If the vnode is busy, or still referenced, return EBUSY.
 * If the pages cannot be flushed, return the error from the flush attempt.
 *   (Except if 'force_destroy' flag is set.)
 * If the destroy fails, return the error from the destroy attempt.
 *
 * At entry:
 *   The inode_rwl lock must be held as WRITER.
 *   The hash chain mutex must be held if the inode is on the hash chain.
 *   The free lock mutex must not be held.
 *
 * At exit:
 *   The hash chain mutex will be held (but may have been dropped temporarily).
 *   If no error occurred, the inode_rwl lock will have been destroyed.
 *   If an error occurred, the inode_rwl lock will be held.
 */

int
sam_flush_and_destroy_vnode(vnode_t *vp, kmutex_t *ihp,
    boolean_t *hash_dropped)
{
	sam_node_t *ip = SAM_VTOI(vp);
	int error = 0;

	/*
	 * Write (and destroy) remaining pages for this vnode, if any.
	 * If we get an error, return the error.
	 */
	*hash_dropped = FALSE;
	if (vn_has_cached_data(vp) != 0) {
		/*
		 * Must drop hash-chain lock before flush pages to avoid
		 * deadlock with fsflush.  Re-acquire after getting
		 * inode_rwl for correct lock ordering.
		 */
		if (ihp != NULL) {
			*hash_dropped = TRUE;
			mutex_exit(ihp);
		}
		/*
		 * Must hold vp for flush_pages call.
		 * Must drop inode_rwl before VN_RELE.
		 */
		VN_HOLD(vp);
		error = sam_flush_pages(ip, B_INVAL);
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
		VN_RELE(vp);	/* XXX can call sam_inactive_ino */

		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		if (ihp != NULL) {
			mutex_enter(ihp);
		}
	}

	if (!error && vp->v_count == 0) {
		/*
		 * Pages are gone && ref count == 0, so we can destroy the
		 * vnode.  (Note that sam_destroy_vnode() drops the hash lock.)
		 */
		*hash_dropped = TRUE;
		error = sam_destroy_vnode(vp, 0);
	} else {
		error = EBUSY;
	}

out:
	if (error) {
		dcmn_err((CE_NOTE,
		    "FS %s: umount: inode %d.%d (vp=%p/ip=%p) "
		    "BUSY=%d, CACHED=%d\n",
		    ip->mp->mt.fi_name,
		    ip->di.id.ino, ip->di.id.gen,
		    (void *)vp, (void *)ip,
		    vp->v_count, (int)vn_has_cached_data(vp)));
		TRACE(T_SAM_BUSY, vp, ip->di.id.ino, vp->v_count,
		    ip->flags.bits);
	}
	return (error);
}
#endif	/* sun */


/*
 * ----- sam_umount_ino -  Deactive dir, root, and map inodes.
 *	Unhash inode directory, root and map inodes from the active chain.
 *	Release inodes (VN_RELE).
 */

#ifdef sun

int				/* ERRNO if error, 0 if successful. */
sam_umount_ino(
	struct vfs *vfsp,	/* vfs pointer for this instance of mount. */
	int fflag)		/* force flushes, even after error */
{
	sam_mount_t	*mp;
	vnode_t *vp[SAM_BLK_INO];
	int i, error = 0;

	/*
	 * If vfs pointer is NULL nothing to unmount, not sure what the
	 * trace # should be.
	 */
	if (vfsp == NULL) {
		TRACE(T_SAM_MNT_ERR, NULL, 10, 0, EINVAL);
		return (0);
	}

	mp = (sam_mount_t *)(void *)vfsp->vfs_data;
	vp[0] = mp->mi.m_vn_root;
	vp[1] = (mp->mi.m_inoblk != NULL) ? SAM_ITOV(mp->mi.m_inoblk) : NULL;
	vp[2] = (mp->mi.m_inodir != NULL) ? SAM_ITOV(mp->mi.m_inodir) : NULL;

	/*
	 * Stop umount if any busy (and the force flag isn't set).
	 */
	for (i = 0; i < SAM_BLK_INO; i++) {
		int flush_err;
		sam_node_t *ip;

		if (vp[i] == NULL) {
			continue;
		}
		ip = SAM_VTOI(vp[i]);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		flush_err = sam_flush_pages(ip, B_INVAL);
		if (flush_err != 0) {
			TRACE(T_SAM_UMNT_INO, vp[i], 1, flush_err, __LINE__);
			error = EBUSY;
		} else if (vp[i]->v_count > 1) {
			TRACE(T_SAM_UMNT_INO, vp[i], 2,
			    vp[i]->v_count, __LINE__);
			error = EBUSY;
		}
		RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
	}

	if (error && (fflag & MS_FORCE) == 0) {
		return (error);
	}

	/*
	 * All pages have been flushed and we're committed.
	 */
	for (i = 0; i < SAM_BLK_INO; i++) {
		int e;
		kmutex_t *ihp;
		sam_node_t *ip;

		if (vp[i] == NULL) {
			continue;
		}
		ip = SAM_VTOI(vp[i]);
		RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
		TRACE(T_SAM_FREE_LIST, vp[i], ip->di.id.ino,
		    (sam_tr_t)vp[i]->v_pages, ip->flags.bits);
		mutex_enter(&vp[i]->v_lock);
		vp[i]->v_count--;
		TRACE(T_SAM_UMNT_INO, vp[i], 3, vp[i]->v_count, __LINE__);
		mutex_exit(&vp[i]->v_lock);
		ihp = &samgt.ihashlock[SAM_IHASH(ip->di.id.ino, vfsp->vfs_dev)];
		mutex_enter(ihp);		/* Set hash lock */
		if ((e = sam_destroy_vnode(vp[i], fflag)) != 0) {
			TRACE(T_SAM_UMNT_INO, vp[i], 4, e, __LINE__);
			ASSERT(!ip->flags.b.hash);
			ASSERT(!ip->flags.b.free);
			error = e;
			sam_stale_inode(ip);
			RW_UNLOCK_OS(&ip->inode_rwl, RW_WRITER);
			dcmn_err((CE_NOTE,
			    "SAM-QFS: %s: umount error = %d: ino %d, "
			    "v_count=%d",
			    mp->mt.fi_name, error, ip->di.id.ino,
			    vp[i]->v_count));
		}
		mutex_exit(ihp);		/* Release hash lock */
	}

	mp->mi.m_vn_root = NULL;
	mp->mi.m_inoblk = NULL;
	mp->mi.m_inodir = NULL;
	return (error);
}
#endif /* sun */


#ifdef linux

int					/* ERRNO if error, 0 if successful. */
sam_umount_ino(
	struct super_block *vfsp,	/* super_block for this mount. */
	int fflag)			/* force flushes, even after error */
{
	sam_mount_t	*mp;
	struct inode *li[SAM_BLK_INO];
	int i, error = 0;

	/*
	 * if vfs pointer is NULL nothing to unmount, not sure
	 * what the trace # should be
	 */
	if (vfsp == NULL) {
		TRACE(T_SAM_MNT_ERR, NULL, 10, 0, EINVAL);
		return (0);
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
	mp = (sam_mount_t *)vfsp->s_fs_info;
#else
	mp = (sam_mount_t *)vfsp->u.generic_sbp;
#endif
	if (mp == NULL) {
		return (0);
	}
	li[0] = mp->mi.m_vn_root;
	li[1] = (mp->mi.m_inoblk != NULL) ? SAM_SITOLI(mp->mi.m_inoblk) : NULL;
	li[2] = (mp->mi.m_inodir != NULL) ? SAM_SITOLI(mp->mi.m_inodir) : NULL;

	/*
	 * Stop umount if any busy.
	 */
	for (i = 0; i < SAM_BLK_INO; i++) {
		if (li[i]) {
			if (atomic_read(&li[i]->i_count) > 1) {
				/*
				 * Stop umount if any busy.
				 */
				error = EBUSY;
			}
		}
	}

	if (error) {
		BUG();
	}

	/*
	 * All pages have been flushed and we're committed.
	 */
	for (i = 0; i < SAM_BLK_INO; i++) {
		if (li[i]) {
			int e;
			sam_node_t *ip;

			ip = SAM_LITOSI(li[i]);
			RW_LOCK_OS(&ip->inode_rwl, RW_WRITER);
			TRACE(T_SAM_FREE_LIST, li[i], ip->di.id.ino,
			    0, ip->flags.bits);
			if ((e = sam_destroy_inode(li[i])) != 0) {
				error = e;
				cmn_err(SAMFS_DEBUG_PANIC,
				    "SAM-QFS: %s: umount error = %d: "
				    "ino %d, i_count=%d",
				    mp->mt.fi_name, error,
				    ip->di.id.ino,
				    atomic_read(&li[i]->i_count));
			}
		}
	}

	mp->mi.m_vn_root = NULL;
	mp->mi.m_inoblk = NULL;
	mp->mi.m_inodir = NULL;
	return (0);
}
#endif /* linux */


#ifdef sun
#if	SAM_CHECK_CHAINS
/*
 * ----- sam_check_chains -  Check freelist & hashlist.
 *	Check freelist and hashlist for entries.
 */

void
sam_check_chains(sam_mount_t *mp)
{
	sam_node_t *ip;
	sam_ihashhead_t	*hip;
	int	ihash;

	/* Cycle through free chain. */

	mutex_enter(&samgt.ifreelock);
	ip = (sam_node_t *)samgt.ifreehead.free.forw;
	while ((ip != (sam_node_t *)&samgt.ifreehead)) {
		TRACE(T_SAM_CLEAN_FL, SAM_ITOV(ip), ip->di.id.ino,
		    samgt.inocount, ip->flags.bits);
		ip = ip->chain.free.forw;
	}
	mutex_exit(&samgt.ifreelock);

	/* Cycle through hash chains. */

	hip = (sam_ihashhead_t *)&samgt.ihashhead[0];
	ASSERT(hip != NULL);
	for (ihash = 0; ihash < samgt.nhino; ihash++, hip++) {

		mutex_enter(&samgt.ihashlock[ihash]);
		for (ip = (sam_node_t *)hip->forw; ip != (sam_node_t *)hip; ) {

			if (ip->mp == mp) {	/* If under this mount point? */
				TRACE(T_SAM_CLEAN_HL, SAM_ITOV(ip),
				    ip->di.id.ino,
				    (sam_tr_t)hip, ip->flags.bits);
				ip = ip->chain.hash.forw;
			}
		}
		mutex_exit(&samgt.ihashlock[ihash]);
	}
}
#endif
#endif /* sun */
