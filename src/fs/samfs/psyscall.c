/*
 * ----- psyscall.c - Process the sam privileged system calls.
 *
 *	Processes the privileged system calls supported on the SAM File System.
 *	Called by the samioc system call module.
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

#ifdef sun
#pragma ident "$Revision: 1.215 $"
#endif

#include "sam/osversion.h"

#ifdef sun
/* ----- UNIX Includes */

#include <stddef.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ksynch.h>
#include <sys/file.h>
#include <sys/mode.h>
#include <sys/uio.h>
#include <sys/dirent.h>
#include <sys/proc.h>
#include <sys/disp.h>
#include <sys/policy.h>
#include <sys/stat.h>
#endif /* sun */

#ifdef  linux
/*
 * ----- Linux Includes
 */
#include <linux/module.h>
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

#include <asm/param.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#endif /* linux */


/* ----- SAMFS Includes */

#include "inode.h"
#include "mount.h"
#include "sam/param.h"
#include "sam/types.h"
#include "sam/checksum.h"
#include "sam/fioctl.h"
#include "sam/resource.h"
#include "pub/sam_errno.h"
#include "sam/mount.h"
#include "sam/syscall.h"
#include "pub/devstat.h"

#include "global.h"
#include "debug.h"
#include "fsdaemon.h"
#include "ino.h"
#include "inode.h"
#include "ino_ext.h"
#include "ioblk.h"
#ifdef linux
#include "clextern.h"
#endif /* linux */
#ifdef sun
#include "extern.h"
#include "syslogerr.h"
#include "copy_extents.h"
#endif /* sun */
#include "rwio.h"
#include "trace.h"
#include "sam_list.h"
#include "qfs_log.h"

#ifdef sun
extern struct vnodeops samfs_vnodeops;
extern char hw_serial[];
#endif /* sun */


/* Private functions. */
static int sam_fsd_call(void *arg, int size, cred_t *credp);
static int sam_set_fsparam(void *arg, int size, cred_t *credp);
static int sam_set_fsconfig(void *arg, int size, cred_t *credp);
static void sam_mount_destroy(sam_mount_t *mp);
static void sam_mount_init(sam_mount_t *mp);
static int sam_mount_info(void *arg, int size, cred_t *credp);
#ifdef METADATA_SERVER
static int sam_setfspartcmd(void *arg, cred_t *credp);

typedef struct sam_schedule_sync_close_arg {
	uint32_t			ord;
	uint32_t			command;
} sam_schedule_sync_close_arg_t;
static void sam_sync_close_inodes(sam_schedule_entry_t *entry);
static int sam_shrink_fs(sam_mount_t *mp, struct samdent *dp, int command);

static int sam_license_info(int cmd, void *arg, int size, cred_t *credp);
static int sam_get_fsclistat(void *arg, int size);
static int sam_fseq_ord(void *arg, int size, cred_t *credp);
static int sam_move_ip_extents(sam_node_t *ip, sam_ib_arg_t *ib_args,
    int *doipupdate, cred_t *credp);
static int sam_reset_unit(sam_mount_t *mp, struct sam_disk_inode *di);
#endif
static int sam_get_fsstatus(void *arg, int size);
static int sam_get_fsinfo(void *arg, int size);
static int sam_get_fsinfo_def(void *arg, int size);
static int sam_get_fsinfo_common(void *arg, int size, int live);
static int sam_get_fspart(void *arg, int size);
static int sam_get_sblk(void *arg, int size);
static int sam_check_stripe_group(sam_mount_t *mp, int istart);

#if defined(SOL_511_ABOVE) && !defined(_NoOSD_)
static int sam_osd_device(void *arg, int size, cred_t *credp);
static int sam_osd_command(void *arg, int size, cred_t *credp);
extern int sam_check_osd_daus(sam_mount_t *mp);
#endif


/*
 * ----- sam_priv_syscall - Process the sam privileged system calls.
 */

int				/* ERRNO if error, 0 if successful. */
sam_priv_syscall(
	int cmd,		/* Command number. */
	void *arg,		/* Pointer to arguments. */
	int size,
	int *rvp)
{
	cred_t *credp;
	int error = 0;

	credp = CRED();
	TRACE(T_SAM_PSYSCALL, NULL, cmd, (sam_tr_t)credp, 0);
	switch (cmd) {

		/*
		 *	File System call daemon.
		 */
		case SC_fsd:
			error = sam_fsd_call(arg, size, credp);
			break;

		/*
		 *	Set file system information in the mount table.
		 */
		case SC_setmount:
			error = sam_mount_info(arg, size, credp);
			break;

#ifdef METADATA_SERVER
		case SC_setlicense:
		case SC_getlicense:
#ifdef feature_not_needed_anymore
			error = sam_license_info(cmd, arg, size, credp);
#endif
			break;
#endif	/* METADATA_SERVER */

		case SC_setfsparam:
			error = sam_set_fsparam(arg, size, credp);
			break;

		case SC_setfsconfig:
			error = sam_set_fsconfig(arg, size, credp);
			break;

		/*
		 *	Filesystem mount & status calls (fs information).
		 */
		case SC_getfsstatus:
			error = sam_get_fsstatus(arg, size);
			break;

		case SC_getfsinfo:
			error = sam_get_fsinfo(arg, size);
			break;

		case SC_getfsinfo_defs:
			error = sam_get_fsinfo_def(arg, size);
			break;

		case SC_getfspart:
			error = sam_get_fspart(arg, size);
			break;

		case SC_fssbinfo:
			error = sam_get_sblk(arg, size);
			break;

#ifdef METADATA_SERVER
		case SC_getfsclistat:
			error = sam_get_fsclistat(arg, size);
			break;

		/*
		 *	File system miscellaneous calls.
		 */

		/*
		 * Quota operations.
		 */
		case SC_quota_priv:
			error = sam_quota_op((struct sam_quota_arg *)arg);
			break;

		case SC_chaid:
			error = sam_set_admid(arg, size, credp);
			break;
		/*
		 * Partition state operations.
		 */
		case SC_setfspartcmd:
			error = sam_setfspartcmd(
			    (struct sam_setfspartcmd_arg *)arg, credp);
			break;

		/*
		 *	Shared file system options.
		 */
		case SC_set_server:
			error = sam_set_server(arg, size, credp);
			break;

		case SC_failover:
			error = sam_voluntary_failover(arg, size, credp);
			break;
#endif

		case SC_set_client:
			error = sam_set_client(arg, size, credp);
			break;

		case SC_share_mount:
			error = sam_share_mount(arg, size, credp);
			break;

		case SC_client_rdsock:
			/*
			 * The samfs module can't be unloaded while this
			 * thread is present.
			 */
#ifdef linux
			rfs_try_module_get(THIS_MODULE);
#endif

			error = sam_client_rdsock(arg, size, credp);

#ifdef linux
			rfs_module_put(THIS_MODULE);
#endif
			break;

#ifdef METADATA_SERVER
		case SC_server_rdsock:
			error = sam_server_rdsock(arg, size, credp);
			break;

		case SC_sethosts:
			error = sam_sgethosts(arg, size, 1, credp);
			break;

		case SC_gethosts:
			error = sam_sgethosts(arg, size, 0, credp);
			break;

		case SC_fseq_ord:
			error = sam_fseq_ord(arg, size, credp);
			break;
#endif

		case SC_shareops:
			error = sam_sys_shareops(arg, size, credp, rvp);
			break;

#if defined(SOL_511_ABOVE) && !defined(_NoOSD_)
		case SC_osd_device:
			error = sam_osd_device(arg, size, credp);
			break;

		case SC_osd_command:
			error = sam_osd_command(arg, size, credp);
			break;
#endif /* defined NOOSD_SOL_511_ABOVE */

#ifdef METADATA_SERVER
		case SC_onoff_client:
			error = sam_onoff_client(arg, size, credp);
			break;
#endif	/* METADATA_SERVER */

		default:
			error = ENOTTY;
			break;

	}	/* end switch */

	SYSCALL_TRACE(T_SAM_PSYSCALL_RET, 0, cmd, error, *rvp);
	return (error);
}


#if defined(SOL_511_ABOVE) && !defined(_NoOSD_)
/*
 * ----- sam_osd_device - Process the user open & close osd system call.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_osd_device(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	sam_osd_dev_arg_t args;
	int error = 0;

	/*
	 * Validate and copyin the arguments.
	 */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	switch (args.param) {

	case OSD_DEV_OPEN: {
		struct samdent dev_ent;

		bzero((caddr_t)&dev_ent, sizeof (struct samdent));
		strncpy(dev_ent.part.pt_name, args.osd_name,
		    sizeof (dev_ent.part.pt_name));
		if ((error = sam_open_osd_device(&dev_ent, args.filemode,
		    credp)) == 0) {
			args.oh = (uint64_t)dev_ent.oh;
			if (copyout((caddr_t)&args, (caddr_t)arg,
			    sizeof (args))) {
				return (EFAULT);
			}
		}
		}
		break;

	case OSD_DEV_CLOSE:
		sam_close_osd_device(args.oh, args.filemode, credp);
		break;

	default:
		error = EINVAL;
		break;
	}
	return (error);
}

/*
 * ----- sam_osd_command - Process the user osd command system call.
 */

/* ARGSUSED2 */
static int			/* ERRNO if error, 0 if successful. */
sam_osd_command(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	sam_osd_cmd_arg_t args;
	sam_mount_t *mp = NULL;
	struct samdent *dp;
	char *data;
	int error = 0;

	/*
	 * Validate and copyin the arguments.
	 */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	data = (char *)args.data.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		data = (char *)args.data.p64;
	}

	if ((mp = sam_find_filesystem(args.fs_name)) == NULL) {
		return (ENOENT);
	}

	/*
	 * We need to issue OSD commands during failover because
	 * sam-sharefsd needs to write the label block.
	 */

	dp = &mp->mi.m_fs[args.ord];
	if (!is_osd_group(dp->part.pt_type)) {
		error = EINVAL;
		goto out;
	}

	switch (args.command) {

	case OSD_CMD_CREATE:
		error = sam_create_priv_object_id(mp, args.oh,
		    args.obj_id);
		break;

	case OSD_CMD_WRITE:
		error = sam_issue_object_io(mp, args.oh, FWRITE,
		    args.obj_id, UIO_USERSPACE, data, args.offset,
		    args.size);
		break;

	case OSD_CMD_READ:
		error = sam_issue_object_io(mp, args.oh, FREAD,
		    args.obj_id, UIO_USERSPACE, data, args.offset,
		    args.size);
		break;

	case OSD_CMD_ATTR:
		error = sam_get_osd_fs_attr(mp, args.oh, &dp->part);
		if (error == 0) {
			if (copyout(&dp->part, data,
			    sizeof (struct sam_fs_part))) {
				return (EFAULT);
			}
		}
		break;

	default:
		error = EINVAL;
		break;
	}

out:
	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}
#endif /* defined NOOSD_SOL_511_ABOVE */


/*
 * ----- sam_fsd_call - Process the sam-fsd system call.
 *
 * Demon request that returns when filesystem issues a command to sam-fsd.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_fsd_call(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	int error = ENOSYS;

	if (secpolicy_fs_config(credp, NULL)) {
		return (EACCES);
	}
	error = sam_wait_scd_cmd(SCD_fsd, UIO_USERSPACE, arg, size);
	return (error);
}


#ifdef sun
/*
 *	----	sam_tsd_destroy
 *
 * Destructor function for sam's thread-specific data.  Since these
 * data are allocated on the stack and destroyed on return, this is
 * a no-op.
 */
/* ARGSUSED */
void
sam_tsd_destroy(void *tsdp)
{
}
#endif	/* sun */


/*
 *	----	sam_mount_init
 *
 * Do initialization of the mount structure that needs to be
 * done before sam-sharefsd start/mount, other than device
 * configuration and setting default mount options.
 */
static void
sam_mount_init(sam_mount_t *mp)
{
#ifdef sun
	char *hostid_cp = &hw_serial[0];

	mp->ms.m_hostid = stoi(&hostid_cp);
	ASSERT(mp->ms.m_tsd_key == 0);
	tsd_create(&mp->ms.m_tsd_key, sam_tsd_destroy);
	tsd_create(&mp->ms.m_tsd_leasekey, sam_tsd_destroy);
#endif /* sun */

	sam_mutex_init(&mp->ms.m_cl_init, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&mp->ms.m_cl_mutex, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&mp->ms.m_cl_cv, NULL, CV_DEFAULT, NULL);
	sam_mutex_init(&mp->ms.m_cl_wrmutex, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&mp->ms.m_waitwr_cv, NULL, CV_DEFAULT, NULL);
	sam_mutex_init(&mp->ms.m_waitwr_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&mp->ms.m_seqno_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&mp->ms.m_sr_ino_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&mp->ms.m_synclock, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&mp->ms.m_xmsg_lock, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&mp->ms.m_inode_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&mp->ms.m_shared_lock, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&mp->ms.m_shared_cv, NULL, CV_DEFAULT, NULL);
	mp->ms.m_shared_event = 0;

	/*
	 * Used to make sure all lease, NFS and staging delayed inactive
	 * related VN_RELEs have completed during failover.
	 */
#ifdef sun
	sam_mutex_init(&mp->ms.m_fo_vnrele_mutex, NULL, MUTEX_DEFAULT, NULL);
	cv_init(&mp->ms.m_fo_vnrele_cv, NULL, CV_DEFAULT, NULL);
	mp->ms.m_fo_vnrele_count = 0;
#endif /* sun */

	list_create(&mp->ms.m_sharefs.queue.list,
	    sizeof (m_sharefs_item_t), offsetof(m_sharefs_item_t, node));
	SAM_SOCK_HANDLE_INIT(&mp->ms.m_cl_sh);

	sam_setup_threads(mp);
	sam_setup_thread(&mp->ms.m_sharefs);
}


/*
 *	----	sam_dup_mount_info
 *
 * Create a duplicate of the given mount point.  Put it on the
 * global mp_list.
 */
sam_mount_t *
sam_dup_mount_info(sam_mount_t *mp)
{
	sam_mount_t *nmp;
	size_t mount_size = sizeof (sam_mount_t) + (sizeof (struct samdent) *
	    (L_FSET -1));
	int ord;

	ASSERT(MUTEX_HELD(&samgt.global_mutex));

	if ((nmp = (sam_mount_t *)kmem_zalloc(mount_size, KM_SLEEP)) == NULL) {
		return (NULL);
	}
	/*
	 * Copy configured mount structure info from old to new, and from
	 * orig_mt to mt.
	 */
	nmp->orig_mt =  mp->orig_mt;
	nmp->mt		 = nmp->orig_mt;
	sam_mount_init(nmp);

	/*
	 * Copy equipment entries from old to new.
	 */
	for (ord = 0; ord < mp->mt.fs_count; ord++) {
		nmp->mi.m_fs[ord].part = mp->mi.m_fs[ord].part;
	}

	/*
	 * Put new structure on mount structures list.  Old one presumably
	 * has been removed and put on the mp_stale list.
	 */
	nmp->ms.m_mp_next = samgt.mp_list;
	samgt.mp_list = nmp;

	return (nmp);
}


/*
 *	----	sam_mount_destroy - destroy a mount structure and
 *		free its memory
 */
static void
sam_mount_destroy(sam_mount_t *mp)
{
	mutex_destroy(&mp->ms.m_cl_init);
	mutex_destroy(&mp->ms.m_cl_mutex);
	cv_destroy(&mp->ms.m_cl_cv);
	mutex_destroy(&mp->ms.m_cl_wrmutex);
	cv_destroy(&mp->ms.m_waitwr_cv);
	mutex_destroy(&mp->ms.m_waitwr_mutex);
	mutex_destroy(&mp->ms.m_seqno_mutex);
	mutex_destroy(&mp->ms.m_sr_ino_mutex);
	mutex_destroy(&mp->ms.m_synclock);
	mutex_destroy(&mp->ms.m_xmsg_lock);
	mutex_destroy(&mp->ms.m_inode_mutex);
	mutex_destroy(&mp->ms.m_shared_lock);
	cv_destroy(&mp->ms.m_shared_cv);

#ifdef sun
	ASSERT(mp->ms.m_tsd_key);
	tsd_destroy(&mp->ms.m_tsd_key);
	mp->ms.m_tsd_key = 0;
	ASSERT(mp->ms.m_tsd_leasekey);
	tsd_destroy(&mp->ms.m_tsd_leasekey);
	mp->ms.m_tsd_leasekey = 0;
#endif	/* sun */

	kmem_free(mp, sizeof (sam_mount_t) +
	    (sizeof (struct samdent) * (L_FSET -1)));
}


/*
 *	----	sam_mount_info - Process the daemon calls to
 *		get/set filesystem info.
 *
 * Set/Get mount - set or return mount point information.
 * Install if mt.ptr = mount info. Remove if mt.ptr = NULL.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_mount_info(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	struct sam_mount_arg args;
	sam_mount_t *mp = NULL, **lastmp = NULL;
	void *mt = NULL;
#ifdef sun
	struct vfs *vfsp = NULL;
#endif
	size_t mount_size;
	int i;
	int istart = 0;
	short prev_mm_count = 0;
	int error;

	/*
	 * Validate and copyin the arguments.
	 */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}
#ifdef linux
#ifdef	_LP64
	mt = (void *)args.mt.p64;
#else
	mt = (void *)args.mt.p32;
#endif /* _LP64 */
#endif /* linux */
#ifdef sun
	mt = (void *)(long)args.mt.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		mt = (void *)args.mt.p64;
	}
#endif

	/*
	 * Find requested mount point.
	 */
	error = ENOENT;
	mutex_enter(&samgt.global_mutex);
	lastmp = &samgt.mp_list;
	for (mp = samgt.mp_list; mp != NULL; mp = mp->ms.m_mp_next) {
		char *mnt;

		if (*args.fs_name == '/') {
			mnt = mp->mt.fi_mnt_point;
		} else {
			mnt = mp->mt.fi_name;
		}

		if (strncmp(mnt, args.fs_name, sizeof (args.fs_name)) == 0) {
#ifdef sun
			vfsp = mp->mi.m_vfsp;
#endif
			error = 0;
			break;
		}
		lastmp = &mp->ms.m_mp_next;
	}

	if (secpolicy_fs_config(credp, vfsp)) {
		error = EPERM;
		goto done;
	}

	/*
	 * If removing filesystem, it must not be mounted, mounting, or
	 * umounting, or have outstanding system calls.  (We're holding
	 * global_mutex, so mp->ms.m_syscall_cnt can't be incremented.)
	 */
	if (error == 0) {
		TRACE(T_SAM_MNT_INFO, mp, mp->mt.fi_status, mp->mt.fs_count,
		    mp->mi.m_fsgen_config);
	}
	if (!error && (mt == NULL)) {
		if (mp->mt.fi_status &
		    (FS_MOUNTED | FS_MOUNTING | FS_UMOUNT_IN_PROGRESS)) {
			error = EBUSY;
			goto done;
		}
		if (mp->ms.m_syscall_cnt != 0) {
			error = EBUSY;
			goto done;
		}
		/*
		 * If shared filesystem, terminate read socket threads.
		 */
		if (SAM_IS_SHARED_FS(mp)) {
			int count = 5;

			mutex_enter(&mp->ms.m_cl_wrmutex);
			while ((mp->ms.m_cl_thread || mp->ms.m_no_clients) &&
			    count--) {
				if (mp->ms.m_cl_thread) {
					THREAD_KILL_OS(mp->ms.m_cl_thread);
				}
				mutex_exit(&mp->ms.m_cl_wrmutex);
				delay(hz);		/* Delay for 1 second */
				mutex_enter(&mp->ms.m_cl_wrmutex);
			}
			if (mp->ms.m_cl_thread || mp->ms.m_no_clients) {
				mutex_exit(&mp->ms.m_cl_wrmutex);
				error = EBUSY;
				goto done;
			}
			mutex_exit(&mp->ms.m_cl_wrmutex);
			/*
			 * A sam_sys_shareops call could be using the daemon
			 * hold count (m_cl_nsocks) to hold the filesystem.
			 * Make sure this is clear before destroying the mount.
			 */
			mutex_enter(&mp->ms.m_shared_lock);
			if (mp->ms.m_cl_nsocks != 0) {
				mutex_exit(&mp->ms.m_shared_lock);
				error = EBUSY;
				goto done;
			}
			mutex_exit(&mp->ms.m_shared_lock);

			mutex_enter(&mp->ms.m_cl_wrmutex);
			sam_clear_sock_fp(&mp->ms.m_cl_sh);
#ifdef METADATA_SERVER
			sam_free_incore_host_table(mp);
#endif
			mutex_exit(&mp->ms.m_cl_wrmutex);
		}
		*lastmp = mp->ms.m_mp_next;
		sam_mount_destroy(mp);
		mp = NULL;
		samgt.num_fs_configured--;
	}

	/*
	 * If adding filesystem, allocate & zero the mount table if it does
	 * not already exist. If it exists, it is possible to change existing
	 * OFF devices and/or add new devices at the end of the array.
	 */
	if (mt != NULL) {
		short mm_count = 0;
		sam_mount_info_t *mntp;

		/*
		 * If file system does not exist, create it. Otherwise, devices
		 * are being added to an existing file system.
		 */
		mntp = (sam_mount_info_t *)mt;
		if (error == ENOENT) {		/* File system does not exist */
			mount_size = sizeof (sam_mount_t) +
			    (sizeof (struct samdent) *
			    (L_FSET -1));
			if ((mp = (sam_mount_t *)kmem_zalloc(mount_size,
			    KM_SLEEP)) == NULL) {
				error = ENOMEM;
				goto done;
			}
			sam_mount_init(mp);

			/*
			 * Copy in mount parameters, then copy in each device.
			 * The original parameters are saved and then restored
			 * at umount in order for mount to start with a fresh
			 * copy.
			 */
			if (copyin((char *)mntp, (char *)&mp->mt,
					sizeof (struct sam_fs_info))) {
				kmem_free((void *)mp, mount_size);
				error = EFAULT;
				goto done;
			}
			bcopy((char *)&mp->mt, (char *)&mp->orig_mt,
			    sizeof (mp->orig_mt));
			*lastmp = mp;
			samgt.num_fs_configured++;
			istart = 0; /* new filesystem, copy all entries */
			error = 0;  /* clear ENOENT now that init is done */
		} else {
			/*
			 * File system exists, add devices. istart zero means
			 * file system is not mounted. istart nonzero means the
			 * file system is mounted & istart is the size of the
			 * sblk. If mounted, the devices must be in
			 * ordinal order -- the equipments must match sblk.
			 */
			error = 0;
			mutex_enter(&mp->ms.m_waitwr_mutex);
			if (mp->mt.fi_status &
			    (FS_MOUNTING|FS_UMOUNT_IN_PROGRESS)) {
				mutex_exit(&mp->ms.m_waitwr_mutex);
				error = EBUSY;
				goto done;
			}
			if (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
				mutex_exit(&mp->ms.m_waitwr_mutex);
				error = EAGAIN;
				goto done;
			}
			if (mp->mt.fi_status & FS_MOUNTED) {
				struct sam_sblk *sblk;
				struct sam_fs_part part;

				if (mp->mt.fi_status & FS_RECONFIG) {
					mutex_exit(&mp->ms.m_waitwr_mutex);
					error = EBUSY;
					goto done;
				}
				mp->mt.fi_status |= FS_RECONFIG;
				mutex_exit(&mp->ms.m_waitwr_mutex);

				sblk = mp->mi.m_sbp;
				istart = sblk->info.sb.fs_count;
				prev_mm_count = sblk->info.sb.mm_count;
				if (SAM_IS_SHARED_CLIENT(mp)) {
					if ((error = sam_update_shared_sblk(mp,
					    SHARE_wait_one))) {
						goto fini;
					}
				}
				if (sblk->info.sb.fs_count > args.fs_count) {
					error = EINVAL;
					goto fini;
				}
				if (SAM_IS_SHARED_CLIENT(mp) &&
				    (sblk->info.sb.fs_count != args.fs_count)) {
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: Server %s has %d "
					    "partitions, client has %d "
					    "Execute samd buildmcf followed by "
					    "samd config to update mcf",
					    mp->mt.fi_name, mp->mt.fi_server,
					    sblk->info.sb.fs_count,
					    args.fs_count);
					error = EINVAL;
					goto fini;
				}
				istart = sblk->info.sb.fs_count;
				prev_mm_count = sblk->info.sb.mm_count;
				for (i = 0; i < istart; i++) {
					if (copyin(&mntp->part[i], &part,
					    sizeof (struct sam_fs_part))) {
						error = EFAULT;
						goto fini;
					}
					if (part.pt_eq != sblk->eq[i].fs.eq) {
						error = EINVAL;
						break;
					}
				}
				if (error) {
					cmn_err(CE_WARN, "SAM-QFS: %s: "
					    "mcf devices are not in ordinal "
					    "order: eq %d mismatch sblk eq %d "
					    "See samu f display for order",
					    mp->mt.fi_name, part.pt_eq,
					    sblk->eq[i].fs.eq);
					goto fini;
				}
			} else {
				istart = 0; /* Unmounted, overwrite entries */
				mutex_exit(&mp->ms.m_waitwr_mutex);
			}
		}

		/*
		 * If not mounted, update all mount entries from the mcf.
		 * If mounted, updated only OFF and DOWN devices from the
		 * sblk. Close those devices if they were previously
		 * opened.
		 */
		for (i = 0; i < args.fs_count; i++) {
			struct samdent *dp;

			dp = &mp->mi.m_fs[i];
			if (istart && (i < istart)) {
				struct sam_sblk *sblk;

				sblk = mp->mi.m_sbp;
				if (sblk->eq[i].fs.state == DEV_ON ||
				    sblk->eq[i].fs.state == DEV_NOALLOC ||
				    (SAM_IS_SHARED_SERVER(mp) &&
				    sblk->eq[i].fs.state == DEV_UNAVAIL)) {
					if (dp->part.pt_type == DT_META) {
						mm_count++;
					}
					continue;
				}
			}
#ifdef sun
			if (dp->opened) {
				sam_close_device(mp, dp, (FREAD | FWRITE),
				    credp);
			}
#endif
			bzero(dp, sizeof (struct samdent));
			if (copyin(&mntp->part[i], dp,
			    sizeof (struct sam_fs_part))) {
				error = EFAULT;
				goto fini;
			}
			sam_mutex_init(&dp->eq_mutex, NULL,
			    MUTEX_DEFAULT, NULL);
			dp->num_group = 1;
			dp->skip_ord = 1;
			if (dp->part.pt_type == DT_META) {
				mm_count++;
			}
			if (istart || i >= mp->mt.fs_count) {
				dp->part.pt_state = DEV_OFF;
			}
		}
		mp->mt.fs_count = (short)args.fs_count;
		mp->orig_mt.fs_count = (short)args.fs_count;
		mp->mt.mm_count = mm_count;
		mp->orig_mt.mm_count = mm_count;
		ASSERT(error == 0);
		if (istart) {
			error = sam_check_stripe_group(mp, istart);
		}
#ifdef sun
		if (error == 0) {
			if (istart) {
				int npart;

				error = sam_getdev(mp, istart, (FREAD | FWRITE),
				    &npart, credp);
#if defined(SOL_511_ABOVE) && !defined(_NoOSD_)
				if (error == 0) {
					error = sam_check_osd_daus(mp);
				}
#endif
				/*
				 * Verify all devices have superblock for
				 * this file system.
				 */
				if ((error == 0) && SAM_IS_SHARED_CLIENT(mp) &&
				    (mp->mt.fi_status & FS_MOUNTED)) {
					error = sam_update_shared_filsys(mp,
					    SHARE_wait_one, -1);
				}
				if (error) {
					sam_close_devices(mp, istart,
					    (FREAD | FWRITE), credp);
				}

			}
		}
#endif
	}

fini:
	if (istart) {
		if (error) {
			mp->mt.fs_count = (short)istart;
			mp->orig_mt.fs_count = (short)istart;
			mp->mt.mm_count = prev_mm_count;
			mp->orig_mt.mm_count = prev_mm_count;
		}
		mutex_enter(&mp->ms.m_waitwr_mutex);
		mp->mt.fi_status &= ~FS_RECONFIG;
		mutex_exit(&mp->ms.m_waitwr_mutex);
		if (error == 0 && SAM_IS_SHARED_CLIENT(mp)) {
			mp->ms.m_cl_vfs_time = 0;	/* Update sblk now */
			(void) sam_proc_block_vfsstat(mp, SHARE_wait_one);
		}
	}
done:
	mutex_exit(&samgt.global_mutex);
	if (mp) {
		TRACE(T_SAM_MNT_INFO_RET, mp, mp->mt.fi_status, mp->mt.fs_count,
		    mp->mi.m_fsgen_config);
	}
	return (error);
}


/*
 *	----	sam_check_stripe_group
 * Check for addition of a stripe group and verify size is the same.
 */
static int
sam_check_stripe_group(
	sam_mount_t *mp,
	int istart)
{
	struct samdent	*dp, *ddp;
	dtype_t		type;	/* Device type */
	int		ord;
	int		i;
	int		num_group;

	for (i = 0; i < mp->mt.fs_count; i++) {
		dp = &mp->mi.m_fs[i];
		if (istart && (i < istart) &&
		    (dp->part.pt_state != DEV_OFF)) {
			continue;
		}
		type = dp->part.pt_type;
		if (!is_stripe_group(type)) {
			continue;
		}
		if (dp->num_group == 0) {
			continue;
		}
		for (ord = i+1; ord < mp->mt.fs_count; ord++) {
			ddp = &mp->mi.m_fs[ord];
			if (is_stripe_group(ddp->part.pt_type) &&
			    (type == ddp->part.pt_type)) {
				dp->num_group++;
				ddp->num_group = 0;
			} else {
				break;
			}
		}
	}

	/*
	 * Check for validity of stripe group.
	 * All members must be the same size and must be adajcent.
	 */
	for (i = 0; i < mp->mt.fs_count; i++) {
		dp = &mp->mi.m_fs[i];
		if (istart && (i < istart) &&
		    (dp->part.pt_state != DEV_OFF)) {
			continue;
		}
		type = dp->part.pt_type;
		if (!is_stripe_group(type)) {
			continue;
		}
		if (dp->num_group == 0) {
			continue;
		}
		num_group = 1;
		for (ord = i+1; ord < mp->mt.fs_count; ord++) {
			ddp = &mp->mi.m_fs[ord];
			if (is_stripe_group(ddp->part.pt_type) &&
			    (type == ddp->part.pt_type)) {
				num_group++;
				if (dp->part.pt_size != ddp->part.pt_size) {
					ddp->error = EFBIG;
					dp->error = EFBIG;
					cmn_err(CE_WARN,
					    "SAM-QFS: %s: Error: "
					    "eq %d lun %s,"
					    " eq %d is not same size as "
					    "the stripe group",
					    mp->mt.fi_name, dp->part.pt_eq,
					    dp->part.pt_name,
					    ddp->part.pt_eq);
					return (EFBIG);
				}
			}
		}
		if (num_group != dp->num_group) {
			cmn_err(CE_WARN, "SAM-QFS: %s: Error: eq %d "
			    "is not adjacent to other members of this "
			    "stripe group", mp->mt.fi_name,
			    dp->part.pt_eq);
			return (EINVAL);
		}
	}
	return (0);
}


/*
 *	----	sam_remove_mplist
 *
 * Remove the given mp from the given list.  Return NULL on
 * failure; mp on success.  Appropriate locks are assumed held.
 */
static sam_mount_t *
sam_remove_mplist(sam_mount_t **mpp, sam_mount_t *mp)
{
	while (*mpp != NULL && *mpp != mp) {
		mpp = &(*mpp)->ms.m_mp_next;
	}
	if (*mpp != NULL) {
		*mpp = mp->ms.m_mp_next;
		mp->ms.m_mp_next = NULL;
		return (mp);
	}
	return (NULL);
}


/*
 *	----	sam_stale_mount_info
 *
 * Stale the given mount structure.  Put it on the mp_stale
 * list until the last reference goes away (at which time it
 * can be freed).
 */
void
sam_stale_mount_info(sam_mount_t *mp)
{
	sam_mount_t *mpr;

	ASSERT(MUTEX_HELD(&samgt.global_mutex));

	mpr = sam_remove_mplist(&samgt.mp_list, mp);

	if (mpr != NULL) {
		/*
		 * Put mp on stale list
		 */
		mpr->ms.m_mp_next = samgt.mp_stale;
		samgt.mp_stale = mpr;
	} else {
		cmn_err(CE_WARN,
		    "SAM-QFS %s: sam_stale_mount_info:  mp %p not found",
		    mp->mt.fi_name, (void *)mp);
	}
}


/*
 *	---- sam_destroy_stale_mount
 *
 * Find the given mount structure on the mp_stale (forcibly unmounted)
 * list, if it's there.  Destroy it.
 */
void
sam_destroy_stale_mount(sam_mount_t *mp)
{
	sam_mount_t *mpr;

	mutex_enter(&samgt.global_mutex);
	mpr = sam_remove_mplist(&samgt.mp_stale, mp);
	if (mpr != NULL) {
		sam_mount_destroy(mpr);
	} else {
		cmn_err(CE_WARN,
		    "SAM-QFS %s: sam_destroy_stale_mount:  mp %p not found",
		    mp->mt.fi_name, (void *)mp);
	}
	mutex_exit(&samgt.global_mutex);
}


#ifdef METADATA_SERVER
/*
 * ----- sam_license_info - Process the daemon license calls.
 *
 * Set/Get license - set or return license information.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_license_info(
	int cmd,
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	struct sam_license_arg args;

	if (secpolicy_fs_config(credp, NULL)) {
		return (EPERM);
	}

	/*
	 * Validate and copyin the arguments.
	 */
	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}
	if (cmd == SC_setlicense) {
		samgt.license = args.value;
	} else {
		args.value = samgt.license;
		if (copyout((caddr_t)&args, arg, sizeof (args))) {
			return (EFAULT);
		}
	}
	return (0);
}
#endif


/*
 * ----- sam_set_fsparam - Process the set filesystem parameter call.
 *
 * Set filesystem parameter values.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_set_fsparam(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	int error = ENOSYS;
#ifdef sun
	struct sam_setfsparam_arg args;
	sam_mount_t  *mp;

	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	if ((mp = sam_find_filesystem(args.sp_fsname)) == NULL) {
		return (ENOENT);
	}

	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EPERM;
		mutex_enter(&mp->ms.m_waitwr_mutex);
		goto out;
	}

	mutex_enter(&mp->ms.m_waitwr_mutex);
	if (!(mp->mt.fi_status & FS_MOUNTED)) {
		error = ENOENT;
		goto out;
	}

	error = 0;
	/*
	 *	Entries are in the same order as in the sam_fs_info struct.
	 *	Value is range checked in SetFsParam(), so don't bother here.
	 */
	switch (args.sp_offset) {
	case offsetof(struct sam_fs_info, fi_sync_meta):
		mp->mt.fi_sync_meta = (short)args.sp_value;
		if (mp->mt.fi_sync_meta) {
			mp->mt.fi_config |= MT_SYNC_META;
		} else {
			mp->mt.fi_config &= ~MT_SYNC_META;
		}
		break;

	case offsetof(struct sam_fs_info, fi_atime):
		mp->mt.fi_atime = (short)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_stripe[DD]):
		mp->mt.fi_stripe[DD] = (short)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_stripe[MM]):
		mp->mt.fi_stripe[MM] = (short)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_low):
		mp->mt.fi_low = (ushort_t)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_high):
		mp->mt.fi_high = (ushort_t)args.sp_value;
		/* Recompute blocks needed to satisfy high water mark */
		sam_mount_setwm_blocks(mp);

		mutex_exit(&mp->ms.m_waitwr_mutex);
		if (mp->mi.m_sbp->info.sb.space < mp->mi.m_high_blk_count) {
			(void) sam_start_releaser(mp);
		}
		mutex_enter(&mp->ms.m_waitwr_mutex);
		break;

	case offsetof(struct sam_fs_info, fi_wr_throttle):
		mp->mt.fi_wr_throttle = args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_readahead):
		mp->mt.fi_readahead = args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_writebehind):
		mp->mt.fi_writebehind = args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_invalid):
		mp->mt.fi_invalid = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_minallocsz):
		if (!SAM_IS_SHARED_FS(mp)) {
			error = EINVAL;
			break;
		}
		mp->mt.fi_minallocsz = args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_maxallocsz):
		if (!SAM_IS_SHARED_FS(mp)) {
			error = EINVAL;
			break;
		}
		mp->mt.fi_maxallocsz = args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_meta_timeo):
		if (!SAM_IS_SHARED_FS(mp)) {
			error = EINVAL;
			break;
		}
		mp->mt.fi_meta_timeo = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_lease_timeo):
		if (!SAM_IS_SHARED_FS(mp)) {
			error = EINVAL;
			break;
		}
		mp->mt.fi_lease_timeo = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_min_pool):
		if (!SAM_IS_SHARED_FS(mp)) {
			error = EINVAL;
			break;
		}
		mp->mt.fi_min_pool = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_lease[RD_LEASE]):
		if (!SAM_IS_SHARED_FS(mp)) {
			error = EINVAL;
			break;
		}
		mp->mt.fi_lease[RD_LEASE] = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_lease[WR_LEASE]):
		if (!SAM_IS_SHARED_FS(mp)) {
			error = EINVAL;
			break;
		}
		mp->mt.fi_lease[WR_LEASE] = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_lease[AP_LEASE]):
		if (!SAM_IS_SHARED_FS(mp)) {
			error = EINVAL;
			break;
		}
		mp->mt.fi_lease[AP_LEASE] = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_partial):
		mp->mt.fi_partial = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_maxpartial):
		mp->mt.fi_maxpartial = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_partial_stage):
		mp->mt.fi_partial_stage = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_flush_behind):
		mp->mt.fi_flush_behind = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_stage_flush_behind):
		mp->mt.fi_stage_flush_behind = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_stage_n_window):
		mp->mt.fi_stage_n_window = (uint_t)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_stage_retries):
		mp->mt.fi_stage_retries = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_dio_wr_consec):
		mp->mt.fi_dio_wr_consec = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_dio_wr_form_min):
		mp->mt.fi_dio_wr_form_min = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_dio_wr_ill_min):
		mp->mt.fi_dio_wr_ill_min = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_dio_rd_consec):
		mp->mt.fi_dio_rd_consec = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_dio_rd_form_min):
		mp->mt.fi_dio_rd_form_min = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_dio_rd_ill_min):
		mp->mt.fi_dio_rd_ill_min = (int)args.sp_value;
		break;

	case offsetof(struct sam_fs_info, fi_def_retention):
		mp->mt.fi_def_retention = (int)args.sp_value;
		break;

	default:
		error = EDOM;
		break;
	}

out:
	SAM_SYSCALL_DEC(mp, 1);
#endif /* sun */
	return (error);
}


/*
 * ----- sam_set_fsconfig - Process the set filesystem config parameter call.
 *
 * Set filesystem parameter values in fi_config, fi_config1 or fi_mflag.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_set_fsconfig(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	int error = ENOSYS;
#ifdef sun
	struct sam_setfsconfig_arg args;
	sam_mount_t  *mp;

	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	if ((mp = sam_find_filesystem(args.sp_fsname)) == NULL) {
		return (ENOENT);
	}

	if (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
		SAM_SYSCALL_DEC(mp, 0);
		return (EAGAIN);
	}

	error = secpolicy_fs_config(credp, mp->mi.m_vfsp);
	mutex_enter(&mp->ms.m_waitwr_mutex);
	if (!error) {
		if (mp->mt.fi_status & FS_MOUNTED) {
			switch (args.sp_offset) {
			case offsetof(struct sam_fs_info, fi_config):
				mp->mt.fi_config &= ~args.sp_mask;
				mp->mt.fi_config |= args.sp_value;
				if (args.sp_mask & MT_TRACE) {
					sam_tracing = mp->mt.fi_config &
					    MT_TRACE;
				}
				break;
			case offsetof(struct sam_fs_info, fi_config1):
				mp->mt.fi_config1 &= ~args.sp_mask;
				mp->mt.fi_config1 |= args.sp_value;
				break;
			case offsetof(struct sam_fs_info, fi_mflag):
				mp->mt.fi_mflag &= ~args.sp_mask;
				mp->mt.fi_mflag |= args.sp_value;
				break;
			default:
				error = EINVAL;
				break;
			}
		} else {
			error = ENOENT;
		}
	}

	SAM_SYSCALL_DEC(mp, 1);
#endif /* sun */
	return (error);
}


/*
 * ----- sam_get_fsstatus - Get filesystem configuration information.
 *
 * Returns an array of sam_fs_id structs to the user.
 * User provides the maximum number of entries for the array.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_get_fsstatus(
	void *arg,		/* Pointer to arguments. */
	int size)
{
	struct sam_get_fsstatus_arg args;
	struct sam_fs_status *fs;
	sam_mount_t *mp;
	int error = 0;

	/*
	 * Validate and copyin the arguments.
	 */
	if (size != sizeof (args) || copyin(arg, &args, sizeof (args))) {
		return (EFAULT);
	}
#ifdef linux
#ifdef	_LP64
	fs = (struct sam_fs_status *)args.fs.p64;
#else
	fs = (struct sam_fs_status *)args.fs.p32;
#endif /* _LP64 */
#endif /* linux */
#ifdef sun
	fs = (struct sam_fs_status *)(long)args.fs.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		fs = (struct sam_fs_status *)args.fs.p64;
	}
#endif

	/*
	 * Return configured filesystems.
	 */
	args.numfs = 0;
	error = 0;
	mutex_enter(&samgt.global_mutex);
	for (mp = samgt.mp_list; mp != NULL; mp = mp->ms.m_mp_next) {
		if (args.numfs < args.maxfs) {
			struct sam_fs_status fsb;

			bcopy(mp->mt.fi_name, fsb.fs_name,
			    sizeof (fsb.fs_name));
			bcopy(mp->mt.fi_mnt_point, fsb.fs_mnt_point,
			    sizeof (fsb.fs_mnt_point));
			fsb.fs_eq		= mp->mt.fi_eq;
			fsb.fs_status	= mp->mt.fi_status;
			if (copyout(&fsb, fs, sizeof (fsb))) {
				error = EFAULT;
			}
			fs++;
		}
		args.numfs++;
	}
	if (copyout(&args, arg, sizeof (args))) {
		error = EFAULT;
	}
	mutex_exit(&samgt.global_mutex);
	return (error);
}


/*
 * Return "live" filesystem options, flags, & info
 */
static int
sam_get_fsinfo(void *arg, int size)
{
	return (sam_get_fsinfo_common(arg, size, 1));
}


/*
 * Return configured filesystem options, flags, & info.
 */
static int
sam_get_fsinfo_def(void *arg, int size)
{
	return (sam_get_fsinfo_common(arg, size, 0));
}


/*
 * ----- sam_get_fsinfo - Process the daemon calls to get filesystem info.
 */
static int		/* ERRNO if error, 0 if successful. */
sam_get_fsinfo_common(
	void *arg,	/* Pointer to arguments. */
	int size,
	int live)	/* return "live" fs info (vs. configured defaults) */
{
	struct sam_get_fsinfo_arg args;
	struct sam_fs_info *fi;
	sam_mount_t *mp;
	int error;

	/*
	 * Validate and copyin the arguments.
	 */
	if (size != sizeof (args) || copyin(arg, &args, sizeof (args))) {
		return (EFAULT);
	}
#ifdef linux
#ifdef	_LP64
	fi = (struct sam_fs_info *)args.fi.p64;
#else
	fi = (struct sam_fs_info *)args.fi.p32;
#endif /* _LP64 */
#endif /* linux */
#ifdef sun
	fi = (struct sam_fs_info *)(long)args.fi.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		fi = (struct sam_fs_info *)args.fi.p64;
	}
#endif

	/*
	 * Find requested filesystem.
	 */
	mutex_enter(&samgt.global_mutex);
	for (mp = samgt.mp_list; mp != NULL; mp = mp->ms.m_mp_next) {
		if (*args.fs_name != '/' &&
		    strcmp(mp->mt.fi_name, args.fs_name) == 0) {
			/*
			 * File system name.
			 */
			break;
		} else if (strcmp(mp->mt.fi_mnt_point, args.fs_name) == 0) {
			/*
			 * Mount point.
			 */
			break;
		}
	}
	if (mp != NULL) {
		/*
		 * Copy to user.
		 */
		error = 0;
		mutex_enter(&mp->ms.m_waitwr_mutex);
		if ((mp->mt.fi_status & FS_MOUNTED) &&
		    !(mp->mt.fi_status &
		    (FS_MOUNTING | FS_UMOUNT_IN_PROGRESS)) &&
		    (mp->mi.m_sbp != NULL)) {
			mp->mt.fi_capacity =
			    (fsize_t)(mp->mi.m_sbp->info.sb.capacity)*1024;
			mp->mt.fi_space	=
			    (fsize_t)(mp->mi.m_sbp->info.sb.space)*1024;
		} else {
			mp->mt.fi_capacity = 0;
			mp->mt.fi_space	= 0;
		}
		mutex_exit(&mp->ms.m_waitwr_mutex);
		if (fi != NULL) {
			if (live) {
				/* copy out in-use mount parameters */
				if (copyout(&mp->mt, fi,
					    sizeof (struct sam_fs_info))) {
					error = EFAULT;
				}
			} else {
				/* copy out configured mount parameters */
				if (copyout(&mp->orig_mt, fi,
					    sizeof (struct sam_fs_info))) {
					error = EFAULT;
				}
			}
		}
	} else {
		error = ENOENT;
	}
	mutex_exit(&samgt.global_mutex);
	return (error);
}


/*
 * ----- sam_get_fspart - Return the filesystem partition data to the user.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_get_fspart(
	void *arg,		/* Pointer to arguments. */
	int size)
{
	struct sam_get_fspart_arg args;
	struct sam_fs_part *pt;
	sam_mount_t *mp;
	int error;

	/*
	 * Validate and copyin the arguments.
	 */
	if (size != sizeof (args) || copyin(arg, &args, sizeof (args))) {
		return (EFAULT);
	}
#ifdef linux
#ifdef	_LP64
	pt = (struct sam_fs_part *)args.pt.p64;
#else
	pt = (struct sam_fs_part *)args.pt.p32;
#endif /* _LP64 */
#endif /* linux */
#ifdef sun
	pt = (struct sam_fs_part *)(long)args.pt.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		pt = (struct sam_fs_part *)args.pt.p64;
	}
#endif

	/*
	 * Find requested mount point.
	 */
	mutex_enter(&samgt.global_mutex);
	for (mp = samgt.mp_list; mp != NULL; mp = mp->ms.m_mp_next) {
		if (strcmp(mp->mt.fi_name, args.fs_name) == 0) {
			break;
		}
	}
	if (mp != NULL) {
		int		i;

		/*
		 * Copy partitions to user.
		 */
		error = 0;
		/*
		 * If shared client and fs is not frozen/freezing/thawing
		 * or the file system isn't locked down, update the
		 * partition table once every FSPART_INTERVAL from the
		 * super block on the server.
		 */
		if (SAM_IS_SHARED_CLIENT(mp) &&
		    ((mp->mt.fi_status & (FS_FAILOVER|FS_LOCK_HARD)) == 0)) {
			struct sam_sblk *sblk;
			int count = 0;

			sblk = mp->mi.m_sbp;
			if (sblk) {
				count = mp->mt.fs_count -
				    sblk->info.sb.fs_count;
			}
			if (count ||
			    (mp->mt.fi_status & FS_MOUNTED &&
			    ((mp->ms.m_sblk_time +
			    (hz * FSPART_INTERVAL)) < ddi_get_lbolt()))) {
				error = sam_update_shared_sblk(mp,
				    SHARE_wait_one);
				error = 0;
			}
		}
		for (i = 0; i < mp->mt.fs_count; i++) {
			struct sam_sblk *sblk;

			sblk = mp->mi.m_sbp;
			if (i >= args.maxpts) {
				continue;
			}
			mutex_enter(&mp->ms.m_waitwr_mutex);
			if ((mp->mt.fi_status & FS_MOUNTED) &&
			   !(mp->mt.fi_status & (FS_MOUNTING | FS_UMOUNT_IN_PROGRESS)) &&
			    (sblk != NULL)) {
				if (i < sblk->info.sb.fs_count) {
					mp->mi.m_fs[i].part.pt_space =
					    (fsize_t)sblk->eq[i].fs.space*1024;
					mp->mi.m_fs[i].part.pt_capacity =
					    (fsize_t)sblk->eq[i].fs.capacity*1024;
				}
			}
			mutex_exit(&mp->ms.m_waitwr_mutex);
			if (copyout(&mp->mi.m_fs[i], pt,
			    sizeof (struct sam_fs_part))) {
				error = EFAULT;
			}
			pt++;
		}
		args.numpts = i;
	} else {
		error = ENOENT;
	}
	mutex_exit(&samgt.global_mutex);
	return (error);
}

#ifdef METADATA_SERVER
/*
 * ----- sam_get_fsclistat - Get client filesystem configuration information.
 *
 * Returns an array of sam_client_info structs to the user.
 * User provides the maximum number of entries for the array.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_get_fsclistat(
	void *arg,		/* Pointer to arguments. */
	int size)
{
	struct sam_get_fsclistat_arg args;
	sam_client_info_t *cl;
	sam_mount_t *mp;
	int error = 0;
	int i;

	/*
	 * Validate and copyin the arguments.
	 */
	if (size != sizeof (args) || copyin(arg, &args, sizeof (args))) {
		return (EFAULT);
	}
#ifdef linux
#ifdef	_LP64
	cl = (struct sam_client_info *)args.fc.p64;
#else
	cl = (struct sam_client_info *)args.fc.p32;
#endif /* _LP64 */
#endif /* linux */
#ifdef sun
	cl = (struct sam_client_info *)(long)args.fc.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		cl = (struct sam_client_info *)args.fc.p64;
	}
#endif

	/*
	 * Return configured clients of this filesystem.
	 */

	args.numcli = 0;
	error = 0;
	/*
	 * Find requested mount point.
	 */
	mutex_enter(&samgt.global_mutex);
	for (mp = samgt.mp_list; mp != NULL; mp = mp->ms.m_mp_next) {
		if (strcmp(mp->mt.fi_name, args.fs_name) == 0) {
			break;
		}
	}
	if (mp == NULL) {
		goto out;
	}
	args.numcli = mp->ms.m_no_clients;
	if (mp->ms.m_max_clients < args.maxcli) {
		args.maxcli = mp->ms.m_max_clients;
	}
	if (args.numcli <= args.maxcli) {
		struct client_entry *clp;
		struct sam_client_info clx;

		for (i = 1; i <= args.maxcli; i++, cl++) {
			clp = sam_get_client_entry(mp, i, 0);
			if (clp) {
				bcopy(clp->hname, clx.hname, sizeof (upath_t));
				clx.cl_status = clp->cl_status;
				clx.cl_config = clp->cl_config;
				clx.cl_config1 = clp->cl_config1;
				clx.cl_flags = clp->cl_flags;
				clx.cl_nomsg = clp->cl_nomsg;
				clx.cl_min_seqno = clp->cl_min_seqno;
				clx.cl_lastmsg_time = clp->cl_lastmsg;
			} else {
				bzero((char *)&clx, sizeof (sam_client_info_t));
			}
			if (copyout(&clx, cl, sizeof (sam_client_info_t))) {
				error = EFAULT;
			}
		}
	}
out:
	if (copyout(&args, arg, sizeof (args))) {
		error = EFAULT;
	}
	mutex_exit(&samgt.global_mutex);
	return (error);
}
#endif


/*
 * ----- sam_get_sblk - Process the get superblock call.
 *
 * Daemon request to return superblock information.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_get_sblk(
	void *arg,		/* Pointer to arguments. */
	int size)
{
	int error = ENOSYS;
#ifdef sun
	sam_fssbinfo_arg_t  args;
	sam_mount_t  *mp;
	void *sbinfo;

	error = 0;

	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	/*
	 * If the mount point is mounted, process cancel request.
	 */
	if ((mp = find_mount_point(args.fseq)) == NULL) {
		return (ECANCELED);
	}

	sbinfo = (void *)(long)args.sbinfo.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		sbinfo = (void *)args.sbinfo.p64;
	}
	if (copyout((char *)mp->mi.m_sbp, (char *)sbinfo,
			sizeof (struct sam_sbinfo))) {
		error = EFAULT;
	}

	SAM_SYSCALL_DEC(mp, 0);
#endif /* sun */
	return (error);
}


/*
 * ----- find_mount_point - find mount table entry.
 *
 * Given the fseq, return a pointer to the sam_mount structure.
 * If the return is non-null, mp->ms.m_syscall_cnt has been
 * incremented; caller is responsible for decrementing this
 * before returning to the user.
 */

sam_mount_t *
find_mount_point(int fseq)
{
	sam_mount_t  *mp;

	mutex_enter(&samgt.global_mutex);
	mp = samgt.mp_list;
	while (mp != NULL) {
		mutex_enter(&mp->ms.m_waitwr_mutex);
		if (mp->mt.fi_eq == fseq &&
		    !(mp->mt.fi_status & FS_UMOUNT_IN_PROGRESS) &&
		    (mp->mt.fi_status & FS_MOUNTED)) {
			if (mp->ms.m_syscall_cnt++ == 0) {
				ASSERT(!mp->ms.m_sysc_dfhold);
				mp->ms.m_sysc_dfhold = TRUE;
			}
			ASSERT(mp->ms.m_syscall_cnt > 0);
			mutex_exit(&mp->ms.m_waitwr_mutex);
			break;
		}
		mutex_exit(&mp->ms.m_waitwr_mutex);
		mp = mp->ms.m_mp_next;
	}
	mutex_exit(&samgt.global_mutex);
	return (mp);
}


/*
 * ----- sam_find_filesystem - find mount table entry.
 *
 * Given the family set name, return a pointer to the sam_mount structure.
 * If the return value is non-null, mp->ms.m_syscall_cnt has been
 * incremented; the caller is responsible for decrementing this before
 * returning to the user.
 */

sam_mount_t *
sam_find_filesystem(uname_t fs_name)
{
	sam_mount_t  *mp;

	mutex_enter(&samgt.global_mutex);
	mp = samgt.mp_list;
	while (mp != NULL) {
		mutex_enter(&mp->ms.m_waitwr_mutex);
		if (strncmp(mp->mt.fi_name, fs_name, sizeof (uname_t)) == 0) {
			if (mp->ms.m_syscall_cnt++ == 0) {
				ASSERT(!mp->ms.m_sysc_dfhold);
				mp->ms.m_sysc_dfhold = TRUE;
			}
			ASSERT(mp->ms.m_syscall_cnt > 0);
			mutex_exit(&mp->ms.m_waitwr_mutex);
			break;
		}
		mutex_exit(&mp->ms.m_waitwr_mutex);
		mp = mp->ms.m_mp_next;
	}
	mutex_exit(&samgt.global_mutex);
	return (mp);
}


#ifdef METADATA_SERVER
/*
 * ----- sam_setfspartcmd - Issue filesystem partition command.
 * Note: samu processes errors into user friendly messages.
 * If an error is added to this routine, samu probably needs
 * an update, also.
 */

static int		/* ERRNO if error, 0 if successful. */
sam_setfspartcmd(
	void *arg,	/* Pointer to arguments. */
	cred_t *credp)	/* Credentials */
{
	struct sam_setfspartcmd_arg args;
	struct sam_sblk *sblk;		/* Ptr to the superblock */
	struct samdent *dp;
	struct sam_fs_part *pt;
	sam_mount_t *mp;
	int	i, ord;
	int locked = 0;
	int error;

	/*
	 * Validate and copyin the arguments.
	 */
	if (copyin(arg, &args, sizeof (args))) {
		return (EFAULT);
	}

	/*
	 * Find requested mount point.
	 */
	if ((mp = sam_find_filesystem(args.fs_name)) == NULL) {
		return (ENOENT);
	}
	if ((error = secpolicy_fs_config(credp, mp->mi.m_vfsp)) != 0) {
		error = EACCES;
		goto out;
	}
	if (SAM_IS_CLIENT_OR_READER(mp)) {
		error = ENOTSUP;
		goto out;
	}

	/*
	 * Search partitions for matching eq.
	 */
	mutex_enter(&mp->ms.m_waitwr_mutex);
	locked = 1;
	error = 0;
	ord = -1;
	if (!(mp->mt.fi_status & FS_MOUNTED)) {
		error = EINVAL;
		goto out;
	}
	if (mp->mt.fi_status & (FS_MOUNTING|FS_UMOUNT_IN_PROGRESS)) {
		error = EBUSY;
		goto out;
	}
	if (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING|FS_RECONFIG)) {
		error = EAGAIN;
		goto out;
	}
	sblk = mp->mi.m_sbp;
	for (i = 0; i < mp->mt.fs_count; i++) {
		dp = &mp->mi.m_fs[i];
		pt = &dp->part;
		if (pt->pt_eq == args.eq) {
			ord = i;
			break;
		}
	}
	if (ord == -1) {
		error = ENOENT;
		goto out;
	}

	/*
	 * If the supplied equipment is an element of a stripe
	 * group, issue the change to the first equipment.
	 */
	if (is_stripe_group(pt->pt_type)) {
		int j;

		for (j = 0; j < i; j++) {
			if (mp->mi.m_fs[j].part.pt_type ==
			    pt->pt_type) {
				dp = &mp->mi.m_fs[j];
				pt = &dp->part;
				ord = j;
				break;
			}
		}
	}

	/*
	 * If busy processing previous command for this eq
	 */
	if (dp->command != DK_CMD_null) {
		SAM_KICK_BLOCK(mp);
		error = EBUSY;
		goto out;
	}

	switch (args.command) {

	case DK_CMD_noalloc:
		if ((pt->pt_type != DT_META) &&
		    (pt->pt_state == DEV_ON)) {
			dp->command = DK_CMD_noalloc;
			dp->skip_ord = 0;
		} else {
			error = EINVAL;
		}
		break;

	case DK_CMD_alloc:
		if (pt->pt_state == DEV_NOALLOC ||
		    pt->pt_state == DEV_UNAVAIL) {
			dp->command = DK_CMD_alloc;
			dp->skip_ord = 0;
		} else {
			error = EINVAL;
		}
		break;

	case DK_CMD_off:
		if (pt->pt_state == DEV_NOALLOC) {
			dp->command = DK_CMD_off;
			dp->skip_ord = 0;
		} else {
			error = EINVAL;
		}
		break;

	case DK_CMD_add:
	case DK_CMD_remove:
	case DK_CMD_release: {
		struct samdent *ddp;	/* Ptr to dev in mount table */
		int count = 0;
		int dt, ddt;
		int i;
		dtype_t	type;		/* Device type */

		/*
		 * Cannot add/remove/release devices in a non V2A file system.
		 * Must upgrade file system with samfsck -u.
		 */
		if (!SAM_MAGIC_V2A_OR_HIGHER(&sblk->info.sb)) {
			cmn_err(CE_WARN, "SAM-QFS: %s: Can't add/remove/release"
			    " eq %d lun %s, not file system vers 2A.",
			    mp->mt.fi_name, pt->pt_eq, pt->pt_name);
			if (SAM_MAGIC_V2_OR_HIGHER(&sblk->info.sb)) {
				cmn_err(CE_WARN, "\tUpgrade file system"
				    " with samfsck -u first.");
			}
			error = EINVAL;
			break;
		}
		if (args.command == DK_CMD_add) {
			if (pt->pt_state == DEV_OFF) {
				dp->command = DK_CMD_add;
				dp->skip_ord = 0;
			} else {
				error = EINVAL;
			}
			break;
		}

		if ((pt->pt_type == DT_META) ||
		    (mp->mt.mm_count == 0)) {
			cmn_err(CE_WARN, "SAM-QFS: %s: remove/release only "
			    "supported for data devices in a ma file system",
			    mp->mt.fi_name);
			error = EINVAL;
			break;
		}

		/*
		 * Cannot remove/release last meta or data ON device in the fs.
		 */
		type = dp->part.pt_type;
		dt = (type == DT_META) ? MM : DD;
		ddp = &mp->mi.m_fs[0];
		for (i = 0; i < sblk->info.sb.fs_count; i++, ddp++) {
			if (ddp->num_group == 0) {
				continue;
			}
			ddt = (ddp->part.pt_type == DT_META) ? MM : DD;
			if (dt != ddt) {
				continue;
			}
			if ((ddp->part.pt_state == DEV_ON) ||
			    (ddp->part.pt_state == DEV_NOALLOC)) {
				count++;
			}
		}
		if (count <= 1) {
			cmn_err(CE_WARN, "SAM-QFS: %s: cannot remove/release "
			    "last ON %s device in the file system",
			    mp->mt.fi_name, dt == MM ? "meta" : "data");
			error = EINVAL;
			break;
		}
		if (mp->mt.fi_status & FS_SHRINKING) {
			error = EBUSY;
			cmn_err(CE_WARN, "SAM-QFS: %s: remove/release "
			    "command in progress. Check shrink.log",
			    mp->mt.fi_name);
			break;
		}
		if (pt->pt_state == DEV_ON || pt->pt_state == DEV_NOALLOC) {
			sam_schedule_sync_close_arg_t *scarg;

			/*
			 * Start a task to sync all inodes to disk.
			 * This task will start running immediately.
			 */
			scarg = kmem_alloc(
			    sizeof (sam_schedule_sync_close_arg_t), KM_SLEEP);
			scarg->ord = ord;
			scarg->command = args.command;
			sam_taskq_add(sam_sync_close_inodes, mp,
			    (void *)scarg, 0);
		} else {
			error = EINVAL;
		}
		}
		break;

	default:
		error = ENOTTY;
		break;

	}	/* end switch */

	if (error == 0) {
		/*
		 * Signal block thread to process LUN state change
		 */
		SAM_KICK_BLOCK(mp);
	}

out:
	SAM_SYSCALL_DEC(mp, locked);
	return (error);
}


/*
 * ----- sam_sync_close_inodes - sync all inodes to disk
 * Call sam_update_filsys with SYNC_CLOSE to guarantee all inodes are
 * written to disk and no more allocation will occur on the equipment.
 */
static void
sam_sync_close_inodes(sam_schedule_entry_t *entry)
{
	sam_mount_t *mp;
	sam_schedule_sync_close_arg_t *ap;
	struct samdent *dp, *ddp;	/* Ptr to dev in mount table */
	int i;
	uchar_t command;
	int state;
	int ord;

	mp = entry->mp;
	mutex_enter(&mp->ms.m_waitwr_mutex);
	mp->mt.fi_status |= FS_SHRINKING;
	mutex_exit(&mp->ms.m_waitwr_mutex);

	mutex_enter(&samgt.schedule_mutex);
	mp->mi.m_schedule_flags |= SAM_SCHEDULE_TASK_SYNC_CLOSE_INODES;
	mutex_exit(&samgt.schedule_mutex);

	ap = (sam_schedule_sync_close_arg_t *)entry->arg;
	command = (uchar_t)ap->command;
	ord = ap->ord;
	dp = &mp->mi.m_fs[ord];
	kmem_free(ap, sizeof (sam_schedule_sync_close_arg_t));
	TRACE(T_SAM_SYNC_INO, mp, ord, command, dp->part.pt_state);

	/*
	 * Set state so there is no more allocation.
	 */
	state = dp->part.pt_state;
	for (i = 0, ddp = dp; i < dp->num_group; i++, ddp++) {
		ddp->part.pt_state = DEV_NOALLOC;
	}
	dp->skip_ord = 0;

	/*
	 * Sync all inodes. Call w/SYNC_CLOSE so all
	 * inodes are written to the .inodes file
	 */
	mutex_enter(&mp->ms.m_synclock);
	sam_update_filsys(mp, SYNC_CLOSE);
	mutex_exit(&mp->ms.m_synclock);

	/*
	 * Set command and request sam-fsd to start sam-shrink process.
	 * Signal block thread to process LUN state change.
	 */
	dp->command = command;
	if (sam_shrink_fs(mp, dp, dp->command) == 0) {
		SAM_KICK_BLOCK(mp);
	} else {
		mutex_enter(&mp->ms.m_waitwr_mutex);
		mp->mt.fi_status &= ~FS_SHRINKING;
		mutex_exit(&mp->ms.m_waitwr_mutex);
		dp->command = DK_CMD_null;
		for (i = 0, ddp = dp; i < dp->num_group; i++, ddp++) {
			ddp->part.pt_state = state;
		}
	}

	mutex_enter(&samgt.schedule_mutex);
	mp->mi.m_schedule_flags &= ~SAM_SCHEDULE_TASK_SYNC_CLOSE_INODES;
	sam_taskq_remove(entry);
	TRACE(T_SAM_SYNC_INO_RET, mp, ord, command, dp->part.pt_state);
}


/*
 * ----- sam_shrink_fs - Remove/Release this eq from the file system
 * Tell sam-fsd to start sam-shrink process.
 */
static int			/* 1 if error; 0 if successful */
sam_shrink_fs(
	sam_mount_t *mp,	/* Pointer to the mount table. */
	struct samdent *dp,	/* Pointer to device entry in mount table */
	int command)		/* Shrink command -- release or remove */
{
	struct sam_fsd_cmd cmd;

	bzero((char *)&cmd, sizeof (cmd));
	cmd.cmd = FSD_shrink;
	bcopy(mp->mt.fi_name, cmd.args.shrink.fs_name,
	    sizeof (cmd.args.shrink.fs_name));
	cmd.args.shrink.command = command;
	cmd.args.shrink.eq = dp->part.pt_eq;
	if (sam_send_scd_cmd(SCD_fsd, &cmd, sizeof (cmd))) {
		cmn_err(CE_WARN, "SAM-QFS: %s: Error removing eq %d lun %s,"
		    " cannot send command to sam-fsd",
		    mp->mt.fi_name, dp->part.pt_eq, dp->part.pt_name);
		return (1);
	}
	return (0);
}


/*
 * ----- sam_fseq_call - Check if inode is allocated on eq.
 */

static int			/* ERRNO if error, 0 if successful. */
sam_fseq_ord(
	void *arg,		/* Pointer to arguments. */
	int size,
	cred_t *credp)
{
	sam_fseq_arg_t args;
	sam_mount_t *mp;
	sam_node_t *ip = NULL;			/* pointer to rm inode */
	int i;
	int kptr[NIEXT + (NIEXT-1)];
	sam_ib_arg_t ib_args;
	int error = 0;
	int save_error = 0;
	int doipupdate = 0;
	int remove_lease = 0;
	krw_t rw_type;
	int trans_size;
	dtype_t opt = 0, npt = 0;

	if (size != sizeof (args) ||
	    copyin(arg, (caddr_t)&args, sizeof (args))) {
		return (EFAULT);
	}

	/*
	 * If the mount point is mounted, process on_ord request.
	 */
	if ((mp = find_mount_point(args.fseq)) == NULL) {
		return (ECANCELED);
	}
	if (mp->mi.m_fs[args.ord].part.pt_eq != args.eq) {
		error = EINVAL;
		goto out;
	}
	if (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
		error = EAGAIN;
		goto out;
	}
	if (secpolicy_fs_config(credp, mp->mi.m_vfsp)) {
		error = EINVAL;
		goto out;
	}

	if ((error = sam_find_ino(mp->mi.m_vfsp, IG_EXISTS, &args.id, &ip))) {
		goto out;
	}

	ib_args.cmd = args.cmd;
	ib_args.ord = args.ord;
	ib_args.first_ord = ip->di.extent_ord[0];
	ib_args.new_ord = args.new_ord;

	if (ib_args.cmd == SAM_MOVE_ORD) {
		int onum_group, nnum_group;

		if (SAM_IS_SHARED_CLIENT(mp)) {
			error = ENOSYS;
			goto outrele;
		}

		/*
		 * Old partition type.
		 * DT_META (258), DT_DATA (257)
		 * DT_RAID (259), or di->stripe_group|DT_STRIPE_GROUP
		 */
		opt = mp->mi.m_fs[ib_args.ord].part.pt_type;

		if (is_stripe_group(opt)) {
			/*
			 * Old ordinal is a stripe group so
			 * a new matching stripe group must be provided.
			 */
			if (ib_args.new_ord < 0) {
				error = EINVAL;
				goto outrele;
			}
			onum_group = mp->mi.m_fs[ib_args.ord].num_group;

			npt = mp->mi.m_fs[ib_args.new_ord].part.pt_type;
			nnum_group = mp->mi.m_fs[ib_args.new_ord].num_group;

			if (!is_stripe_group(npt) ||
			    nnum_group != onum_group) {
				error = EINVAL;
				goto outrele;
			}

		} else if (ib_args.new_ord >= 0) {
			/*
			 * New partition type.
			 */
			npt = mp->mi.m_fs[ib_args.new_ord].part.pt_type;
			if (opt != npt) {
				/*
				 * Partition types don't match.
				 */
				error = EINVAL;
				goto outrele;
			}
		}

		trans_size = (int)TOP_SETATTR_SIZE(ip);
		TRANS_BEGIN_ASYNC(ip->mp, TOP_SETATTR, trans_size);

		rw_type = RW_WRITER;
		RW_LOCK_OS(&ip->data_rwl, rw_type);

		/*
		 * Only need the lease for regular files since
		 * all directory modifications are done on the MDS.
		 */
		if (SAM_IS_SHARED_FS(mp) && !S_ISDIR(ip->di.mode)) {
			sam_lease_data_t data;

			bzero(&data, sizeof (data));
			data.ltype = LTYPE_exclusive;

			error = sam_proc_get_lease(ip, &data,
			    NULL, NULL, SHARE_wait, credp);

			if (error != 0) {
				goto transend;
			}
			remove_lease = 1;
		}
		RW_LOCK_OS(&ip->inode_rwl, rw_type);

		if (S_ISDIR(ip->di.mode)) {
			error = sam_sync_meta(ip, NULL, credp);
		} else {
			error = sam_flush_pages(ip, 0);
		}
		if (error != 0) {
			goto outunlock;
		}

		sam_clear_map_cache(ip);

		if (ib_args.new_ord >= 0) {
			if (is_stripe_group(opt)) {
				ip->di.stripe_group =
				    (npt & ~DT_STRIPE_GROUP_MASK);
			}
			ip->di.unit = (uchar_t)ib_args.new_ord;
		} else {
			error = sam_reset_unit(ip->mp, &(ip->di));
			if (error != 0) {
				goto outunlock;
			}
		}

		error = sam_move_ip_extents(ip, &ib_args, &doipupdate, credp);
		if (error != 0) {
			goto outunlock;
		}
		if (ip->di.status.b.direct_map) {
			/*
			 * If file is a direct map file then
			 * we are finished moving data.
			 */
			goto skip_indirects;
		}

	} else {

		rw_type = RW_READER;
		RW_LOCK_OS(&ip->inode_rwl, rw_type);
	}

	for (i = 0; i < (NIEXT+2); i++) {
		kptr[i] = -1;
	}

	/*
	 * Now do the indirects for SAM_FIND_ORD and SAM_MOVE_ORD.
	 */
	for (i = (NOEXT - 1); i >= NDEXT; i--) {
		int set = 0;
		int kptr_index;

		if (ip->di.extent[i] == 0) {
			continue;
		}

		if (ib_args.cmd == SAM_MOVE_ORD &&
		    ib_args.ord == ip->di.extent_ord[i]) {
			/*
			 * This top level indirect is on
			 * the ordinal getting moved so an inode
			 * update will be required when finished.
			 */
			doipupdate = 1;
		}

		if (i == NDEXT) {
			/*
			 * The current index in the top level indirect
			 * block matches the index where the new offset
			 * resides.  The values in the kptr array
			 * starting at 0 are those returned by
			 * sam_get_extent. Blocks with an index greater
			 * than the value in kptr at each level will be
			 * released. No indirect blocks down this tree
			 * will be release since the new offset is down
			 * this tree.
			 */
			kptr_index = 0;
			set = 1;	/* First indirect block */
		} else {
			/*
			 * Deleting all blocks down this indirect tree,
			 * this is indicated by values of -1 in the kptr
			 * array starting at kptr_index.
			 */
			kptr_index = NIEXT - 1;
		}
		error = sam_proc_indirect_block(ip, ib_args, kptr, kptr_index,
		    &ip->di.extent[i], &ip->di.extent_ord[i],
		    (i - NDEXT), &set);
		if (error == ECANCELED) {
			error = 0;
			args.on_ord = 1;
			break;
		} else if (args.cmd == SAM_FIND_ORD) {
			/*
			 * Either ordinal was not found
			 * or an error occured.
			 */
			args.on_ord = 0;
		}
	}

skip_indirects:
	if (doipupdate) {

		TRANS_INODE(ip->mp, ip);

		ip->flags.b.changed = 1;
		sam_update_inode(ip, SAM_SYNC_ONE, FALSE);
	}

	if (remove_lease) {
		error = sam_proc_rm_lease(ip, CL_EXCLUSIVE, RW_WRITER);
	}

	RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
	if (ib_args.cmd == SAM_MOVE_ORD) {
		RW_UNLOCK_OS(&ip->data_rwl, rw_type);
		TRANS_END_ASYNC(ip->mp, TOP_SETATTR, trans_size);
	}

outrele:
	if (S_ISSEGS(&ip->di) && ip->seg_held) {
		sam_rele_index(ip);
	}

	VN_RELE(SAM_ITOV(ip));

	if (error == 0) {
		if (size != sizeof (args) ||
		    copyout((caddr_t)&args, arg, sizeof (args))) {
			error = EFAULT;
		}
	}

out:
	SAM_SYSCALL_DEC(mp, 0);
	return (error);

outunlock:

	ASSERT(ib_args.cmd == SAM_MOVE_ORD);

	if (remove_lease) {
		save_error = error;
		error = sam_proc_rm_lease(ip, CL_EXCLUSIVE, RW_WRITER);
	}
	RW_UNLOCK_OS(&ip->inode_rwl, rw_type);
transend:
	RW_UNLOCK_OS(&ip->data_rwl, rw_type);

	TRANS_END_ASYNC(ip->mp, TOP_SETATTR, trans_size);

	if (S_ISSEGS(&ip->di) && ip->seg_held) {
		sam_rele_index(ip);
	}
	VN_RELE(SAM_ITOV(ip));

	if (error == 0 && save_error != 0) {
		error = save_error;
	}

	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}


/*
 * ----- sam_move_ip_extents -
 *
 * Move the data off of the direct extents in the inode
 * that are on the specified device ordinal.
 *
 */
static int			/* ERRNO if error, 0 if successful. */
sam_move_ip_extents(
	sam_node_t *ip,
	sam_ib_arg_t *ib_args,
	int *doipupdate,
	cred_t *credp)
{
	sam_mount_t *mp = ip->mp;
	int error = 0;
	int i;
	int bt;		/* file extent block type LG or SM */
	int dt;		/* file data device type META (1) or DATA (0) */

	if (ib_args->cmd != SAM_MOVE_ORD) {
		return (EINVAL);
	}

	if (ip->di.status.b.direct_map) {
		error = sam_allocate_and_copy_directmap(ip, ib_args, credp);
		if (error == 0) {
			*doipupdate = 1;
		}
		return (error);
	}

	/*
	 * File device type
	 * 1 = META, 0 = DATA
	 */
	dt = ip->di.status.b.meta;

	for (i = 0; i < NDEXT; i++) {
		if (ip->di.extent[i] != 0 &&
		    ip->di.extent_ord[i] == ib_args->ord) {
			sam_bn_t obn, nbn;
			int oord, nord;

			if (ip->di.status.b.on_large || (i >= NSDEXT)) {
				/*
				 * Large DAU.
				 */
				bt = LG;
			} else {
				/*
				 * Small DAU.
				 */
				bt = SM;
			}

			obn = ip->di.extent[i];
			oord =  ip->di.extent_ord[i];

			error = sam_allocate_and_copy_extent(ip, bt, dt,
			    obn, oord, NULL, &nbn, &nord, NULL);

			if (error == 0) {
				/*
				 * Save the new extent in the inode
				 * and free the old one.
				 */
				ip->di.extent_ord[i] = (uchar_t)nord;
				ip->di.extent[i] = nbn;
				*doipupdate = 1;

				sam_free_block(mp, bt, obn, oord);
			}
		}
	}

	return (error);
}


/*
 * ----- sam_reset_unit -
 *
 * Reset the unit for space allocation for the
 * specified inode. The new unit must be of the same
 * type as the existing unit and have available space.
 */
static int
sam_reset_unit(
	sam_mount_t *mp,
	struct sam_disk_inode *di)
{
	int ord, oldord, curord;
	dtype_t oldpt;
	offset_t curspace;
	int fs_count = mp->mi.m_sbp->info.sb.fs_count;

	oldord = di->unit;
	oldpt = mp->mi.m_fs[oldord].part.pt_type;

	if (is_stripe_group(oldpt)) {
		/*
		 * Don't do stripe groups here.
		 */
		return (EINVAL);
	}

	/*
	 * Find an available device that matches the
	 * old unit (partition type) and has space.
	 */
	curord = -1;
	curspace = 0;
	for (ord = 0; ord < fs_count; ord++) {
		if (mp->mi.m_fs[ord].part.pt_type == oldpt &&
		    mp->mi.m_fs[ord].part.pt_state == DEV_ON) {

			if (curord == -1 ||
			    mp->mi.m_sbp->eq[ord].fs.space > curspace) {

				curord = ord;
				curspace = mp->mi.m_sbp->eq[ord].fs.space;
			}
		}
	}
	if (curord == -1 || curspace == 0) {
		/*
		 * Didn't find a matching unit
		 * with available space.
		 */
		return (ENODEV);
	}
	di->unit = (uchar_t)curord;
	return (0);
}
#endif
