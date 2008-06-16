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
#pragma ident "$Revision: 1.182 $"
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
#if defined(SAM_OSD_SUPPORT)
#include <sys/osd.h>
#endif

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
static int sam_license_info(int cmd, void *arg, int size, cred_t *credp);
static int sam_get_fsclistat(void *arg, int size);
static int sam_fseq_ord(void *arg, int size, cred_t *credp);
static int sam_move_ip_extents(sam_node_t *ip, sam_ib_arg_t *ib_args,
    int *doipupdate, cred_t *credp);
#endif
static int sam_get_fsstatus(void *arg, int size);
static int sam_get_fsinfo(void *arg, int size);
static int sam_get_fsinfo_def(void *arg, int size);
static int sam_get_fsinfo_common(void *arg, int size, int live);
static int sam_get_fspart(void *arg, int size);
static int sam_get_sblk(void *arg, int size);

#if defined(SAM_OSD_SUPPORT)
static int sam_osd_device(void *arg, int size, cred_t *credp);
static int sam_osd_command(void *arg, int size, cred_t *credp);
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

	TRACE(T_SAM_PSYSCALL, NULL, cmd, 0, 0);
	credp = CRED();
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
			error = sam_license_info(cmd, arg, size, credp);
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

#if defined(SAM_OSD_SUPPORT)
		case SC_osd_device:
			error = sam_osd_device(arg, size, credp);
			break;

		case SC_osd_command:
			error = sam_osd_command(arg, size, credp);
			break;
#endif

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


#if defined(SAM_OSD_SUPPORT)
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
 * ----- sam_osd_command - Process the osd command.
 *
 * User request that processes an osd command.
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
#ifdef linux
#ifdef	_LP64
	data = (char *)args.data.p64;
#else
	data = (char *)args.data.p32;
#endif /* _LP64 */
#endif /* linux */
#ifdef sun
	data = (char *)args.data.p32;
	if (curproc->p_model != DATAMODEL_ILP32) {
		data = (char *)args.data.p64;
	}
#endif

	if ((mp = sam_find_filesystem(args.fs_name)) == NULL) {
		return (ENOENT);
	}
	if (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
		error = EAGAIN;
		goto out;
	}
	dp = &mp->mi.m_fs[args.ord];
	if (!is_target_group(dp->part.pt_type)) {
		error = EINVAL;
		goto out;
	}

	switch (args.command) {

	case OSD_CMD_CREATE:
		error = sam_create_priv_object_id(args.oh,
		    args.obj_id);
		break;

	case OSD_CMD_WRITE:
		error = sam_issue_object_io(args.oh, FWRITE,
		    args.obj_id, UIO_USERSPACE, data, args.offset,
		    args.size);
		break;

	case OSD_CMD_READ:
		error = sam_issue_object_io(args.oh, FREAD,
		    args.obj_id, UIO_USERSPACE, data, args.offset,
		    args.size);
		break;

	default:
		error = EINVAL;
		break;
	}

out:
	SAM_SYSCALL_DEC(mp, 0);
	return (error);
}

#endif


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
static void
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
	mt = (void *)args.mt.p32;
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
			sam_clear_sock_fp(&mp->ms.m_cl_sh);
#ifdef METADATA_SERVER
			sam_free_incore_host_table(mp);
#endif
			mutex_exit(&mp->ms.m_cl_wrmutex);
		}
		*lastmp = mp->ms.m_mp_next;
		sam_mount_destroy(mp);
		samgt.num_fs_configured--;
	}

	/*
	 * If adding filesystem, allocate & zero the mount table if it does
	 * not already exist. If it exists, it is possible to add devices at
	 * the end of the array.
	 */
	if (mt != NULL) {
		int istart;
		short prev_mm_count;
		sam_mount_info_t *mntp;

		/*
		 * If file system does not exist, create it. Otherwise, devices
		 * are being added to file system.
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
			istart = 0;
		} else {		/* File system exists, add devices */
			if (mp->mt.fi_status &
			    (FS_MOUNTED | FS_MOUNTING |
			    FS_UMOUNT_IN_PROGRESS)) {
				struct sam_sblk *sblk;

				sblk = mp->mi.m_sbp;
				if (sblk->info.sb.fs_count >= args.fs_count) {
					error = EINVAL;
					goto done;
				}
				mp->mt.fs_count = sblk->info.sb.fs_count;
				mp->orig_mt.fs_count = sblk->info.sb.fs_count;
				istart = sblk->info.sb.fs_count;
				mp->mt.mm_count = sblk->info.sb.mm_count;
				mp->orig_mt.mm_count = sblk->info.sb.mm_count;
				prev_mm_count = sblk->info.sb.mm_count;
			} else {
				istart = 0;
			}
		}
		for (i = istart; i < args.fs_count; i++) {
			bzero(&mp->mi.m_fs[i], sizeof (struct samdent));
			if (copyin(&mntp->part[i], &mp->mi.m_fs[i],
					sizeof (struct sam_fs_part))) {
				kmem_free((void *)mp, mount_size);
				error = EFAULT;
				goto done;
			}
			if (istart) {
				mp->mi.m_fs[i].num_group = 1;
				mp->mi.m_fs[i].skip_ord = 1;
				mp->mi.m_fs[i].part.pt_state = DEV_OFF;
				mp->mt.fs_count++;
				mp->orig_mt.fs_count++;
				if (mp->mi.m_fs[i].part.pt_type == DT_META) {
					mp->mt.mm_count++;
					mp->orig_mt.mm_count++;
				}
			}
		}
#ifdef sun
		{
		int npart;

			error = sam_getdev(mp, istart, (FREAD | FWRITE),
			    &npart, credp);
			if (istart == 0) {
				sam_close_devices(mp, 0, (FREAD | FWRITE),
				    credp);
			} else {
				if (error) {
					sam_close_devices(mp, istart,
					    (FREAD | FWRITE), credp);
					mp->mt.fs_count = (short)istart;
					mp->orig_mt.fs_count = (short)istart;
					mp->mt.mm_count = prev_mm_count;
					mp->orig_mt.mm_count = prev_mm_count;
				} else if (SAM_IS_SHARED_CLIENT(mp) &&
				    (mp->mt.fi_status & FS_MOUNTED)) {
					error = sam_update_shared_filsys(mp,
					    SHARE_wait_one, 0);
				}

			}
		}
#endif
	}
done:
	mutex_exit(&samgt.global_mutex);
	return (error);
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
	fs = (struct sam_fs_status *)args.fs.p32;
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
	fi = (struct sam_fs_info *)args.fi.p32;
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
	pt = (struct sam_fs_part *)args.pt.p32;
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
		 * If shared client, update the partition table once every
		 * UPDATE_INTERVAL, from the super block on the server.
		 */
		if (SAM_IS_SHARED_CLIENT(mp)) {
			struct sam_sblk *sblk;
			int count = 0;

			sblk = mp->mi.m_sbp;
			if (sblk) {
				count = mp->mt.fs_count -
				    sblk->info.sb.fs_count;
			}
			if (count ||
			    ((mp->ms.m_sblk_time +
			    (hz * FSPART_INTERVAL)) < lbolt) ||
			    (mp->ms.m_sblk_time == 0)) {
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
			    !(mp->mt.fi_status &
			    (FS_MOUNTING | FS_UMOUNT_IN_PROGRESS)) &&
			    (sblk != NULL)) {
				if (i < sblk->info.sb.fs_count) {
					mp->mi.m_fs[i].part.pt_space =
					    (fsize_t)sblk->eq[
					    i].fs.space*1024;
					mp->mi.m_fs[ i].part.pt_capacity =
					    (fsize_t)sblk->eq[
					    i].fs.capacity*1024;
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
	cl = (struct sam_client_info *)args.fc.p32;
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
	if (mp != NULL) {
		args.numcli = mp->ms.m_no_clients;
		if (mp->ms.m_max_clients < args.maxcli) {
			args.maxcli = mp->ms.m_max_clients;
		}
		if (args.numcli <= args.maxcli) {
			struct client_entry *cli;
			struct sam_client_info clx;

			for (i = 1; i <= args.maxcli; i++, cl++) {
				cli = sam_get_client_entry(mp, i, 0);
				if (cli) {
					bcopy(cli->hname, clx.hname,
					    sizeof (upath_t));
					clx.cl_status = cli->cl_status;
					clx.cl_config = cli->cl_config;
					clx.cl_config1 = cli->cl_config1;
					clx.cl_flags = cli->cl_flags;
					clx.cl_nomsg = cli->cl_nomsg;
					clx.cl_min_seqno = cli->cl_min_seqno;
				} else {
					bzero((char *)&clx,
					    sizeof (struct sam_client_info));
				}
				if (copyout(&clx, cl,
					    sizeof (struct sam_client_info))) {
					error = EFAULT;
				}
			}
		}
	}
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

	sbinfo = (void *)args.sbinfo.p32;
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
	if (mp->mt.fi_status & (FS_FAILOVER|FS_RESYNCING)) {
		error = EAGAIN;
		goto out;
	}

	/*
	 * Search partitions for matching eq.
	 */
	mutex_enter(&mp->ms.m_waitwr_mutex);
	locked = 1;
	error = 0;
	ord = -1;
	for (i = 0; i < mp->mt.fs_count; i++) {
		dp = &mp->mi.m_fs[i];
		pt = &dp->part;
		if (pt->pt_eq != args.eq) {
			continue;
		}

		/*
		 * If the supplied equipment is an element of a stripe
		 * group, issue the change to the first equipment.
		 */
		ord = i;
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
		case DK_CMD_off:
			if (pt->pt_state == DEV_NOALLOC) {
				dp->command = (uchar_t)args.command;
				dp->skip_ord = 0;
			} else {
				error = EINVAL;
			}
			break;

		case DK_CMD_add:
			if (pt->pt_state == DEV_OFF) {
				dp->command = DK_CMD_add;
				dp->skip_ord = 0;
			} else {
				error = EINVAL;
			}
			break;

		case DK_CMD_remove:
			if (pt->pt_state == DEV_ON ||
			    pt->pt_state == DEV_NOALLOC) {
				dp->command = DK_CMD_remove;
				dp->skip_ord = 0;
			} else {
				error = EINVAL;
			}
			break;

		case DK_CMD_release:
			if ((pt->pt_type != DT_META) &&
			    pt->pt_state == DEV_ON ||
			    pt->pt_state == DEV_NOALLOC) {
				dp->command = DK_CMD_release;
				dp->skip_ord = 0;
			} else {
				error = EINVAL;
			}
			break;

		default:
			error = ENOTTY;
			break;

		}	/* end switch */
	}

	if (ord >= 0) {
		if (error == 0) {
			/*
			 * Signal block thread to process LUN state change
			 */
			SAM_KICK_BLOCK(mp);
		}
	} else {
		error = ENOENT;
	}

out:
	SAM_SYSCALL_DEC(mp, locked);
	return (error);
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
			    NULL, NULL, credp);

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
			ip->di.unit = ib_args.new_ord;
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
			error = ENOENT;
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
				ip->di.extent_ord[i] = nord;
				ip->di.extent[i] = nbn;
				*doipupdate = 1;

				sam_free_block(mp, bt, obn, oord);
			}
		}
	}

	return (error);
}
#endif
