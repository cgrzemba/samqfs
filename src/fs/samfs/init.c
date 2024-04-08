/*
 * ----- init.c - Process the initialization functions.
 *
 *	Processes the modload, modunload, and modinfo.
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

#pragma ident "$Revision: 1.132 $"

#define	SAM_INIT

#include "sam/osversion.h"

/* ----- UNIX Includes */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/flock.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/modctl.h>
#include <sys/vfs.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/mntent.h>

#if defined(SOL_511_ABOVE)
#include <sys/vfs_opreg.h>
#endif

/* ----- SAMFS Includes */

#include "inode.h"
#include "samfs.h"
#include "extern.h"
#include "global.h"
#include "trace.h"
#include "rwio.h"
#include "kstats.h"
#include "pub/version.h"
#include "samhost.h"
#include "objctl.h"

extern int ncsize;

/*
 * Configurable variables for SAM.  These can be set in /etc/system:
 *
 * set samfs:ninodes = 65536;
 * sam samfs:nhino = 1024;
 */
int ninodes = 0;			/* number of inodes to keep */
int nhino = 0;				/* inode hash table size */
int sam_max_shared_hosts = SAM_MAX_SHARED_HOSTS; /* max shared fs hosts */
int sam_max_blkd_frlock = SAM_MAX_BLKD_FRLOCK; /* max blocked frlocks */

sam_global_tbl_t samgt;			/* SAM-QFS global parameters */
int sam_tracing = 1;			/* sam_tracing always enabled */
int sam_trace_size = 0;			/* <= 0 means compute a default */
char *sam_zero_block = NULL;		/* a block of zeros */
int sam_zero_block_size = 0;		/* size of a block of zeros */
int panic_on_fs_error = 1;		/* a debug variable */
int sam_vpm_enable = 1;			/* Use Solaris VPM if available */

/*
 * One means all signals except the critical ones are masked while waiting on
 * condition variable. Currently used only while staging.
 */
int sam_mask_signals = 0;

const char sam_version[] = SAM_BUILD_INFO;
const char sam_build_uname[] = SAM_BUILD_UNAME;

struct sam_gen_statistics sam_gen_stats;
struct sam_dio_statistics sam_dio_stats;
struct sam_page_statistics sam_page_stats;
struct sam_sam_statistics sam_sam_stats;
struct sam_shared_server_statistics sam_shared_server_stats;
struct sam_shared_client_statistics sam_shared_client_stats;
struct sam_thread_statistics sam_thread_stats;
struct sam_dnlc_statistics sam_dnlc_stats;
static kstat_t *sam_gen_kstats;
static kstat_t *sam_dio_kstats;
static kstat_t *sam_page_kstats;
static kstat_t *sam_sam_kstats;
static kstat_t *sam_shared_server_kstats;
static kstat_t *sam_shared_client_kstats;
static kstat_t *sam_thread_kstats;
static kstat_t *sam_dnlc_kstats;

#ifdef DEBUG
struct sam_debug_statistics sam_debug_stats;
static kstat_t *sam_debug_kstats;
#endif

static int samfs_init10(int fstype, char *name);
static int samfs_init(struct vfssw *vfsswp, int fstype);
static void sam_clean_ino(void);
static void sam_init_ino(void);
static void sam_buf_fini(void);
static void sam_init_kstats(void);
static void sam_fini_kstats(void);
static int sam_update_gen_kstats(struct kstat *, int);
static void sam_init_cache(void);
static void sam_clean_cache(void);

extern void sam_amld_init(void);
extern void sam_amld_fini(void);
extern void sam_sc_daemon_init(void);
extern void sam_sc_daemon_fini(void);

extern void lqfs_init(void);
extern int lqfs_fini();

void
sam_init_sharefs_rpc_item_cache(void)
{
	samgt.item_cache = kmem_cache_create("sam_rpcitem_cache",
	    sizeof (m_sharefs_item_t), 0,
	    NULL, NULL, NULL, NULL, NULL, 0);
	ASSERT(samgt.item_cache);
}

void
sam_delete_sharefs_rpc_item_cache(void)
{
	kmem_cache_destroy(samgt.item_cache);
	samgt.item_cache = NULL;
}

/* ----- samioc (syscall) data. */
static int samioc_id = 0;		/* Id returned from modunload() */

/* Set sam_syscall address and lock */
static void (*samioc_link)(void *) = NULL;

/* ----- vfsops contains the vfs entry points - defined in sam/vfsops.c. */
/* ----- vnodeops contains the vnode entry points - defined in sam/vnops.c. */
/* ----- and sam/clvnops.c. */

extern vfsops_t	*samfs_vfsopsp;

/*
 * Standalone SAM-QFS and QFS vnops table
 */
extern const fs_operation_def_t samfs_vnodeops_template[];
extern struct vnodeops *samfs_vnodeopsp;

/*
 * Shared SAM-QFS and QFS vnops table
 */
extern const fs_operation_def_t samfs_client_vnodeops_template[];
extern struct vnodeops *samfs_client_vnodeopsp;

/*
 * Standalone SAM-QFS and QFS vnops table for forcibly unmounted vnodes
 */
extern const fs_operation_def_t samfs_vnode_staleops_template[];
extern struct vnodeops *samfs_vnode_staleopsp;

/*
 * Shared SAM-QFS and QFS vnops table for forcibly unmounted, shared vnodes
 */
extern const fs_operation_def_t samfs_client_vnode_staleops_template[];
extern struct vnodeops *samfs_client_vnode_staleopsp;


/*
 * ---- Filesystem type switch table, vfssw, contains, in addition to the
 *      pointer to vfsops, the routines that support the loading and
 *      unloading of the sam filsystem.
 */

/*
 * Mount options table
 */
static char *rw_cancel[] = { MNTOPT_RO, NULL };
static char *ro_cancel[] = { MNTOPT_RW, NULL };
static char *suid_cancel[] = { MNTOPT_NOSUID, NULL };
static char *nosuid_cancel[] = { MNTOPT_SUID, NULL };
static char *quota_cancel[] = { MNTOPT_NOQUOTA, NULL };
static char *noquota_cancel[] = { MNTOPT_QUOTA, NULL };
static char *nologging_cancel[] = { MNTOPT_LOGGING, NULL };
static char *logging_cancel[] = { MNTOPT_NOLOGGING, NULL };
static char *xattr_cancel[] = { MNTOPT_NOXATTR, NULL };
static char *noxattr_cancel[] = { MNTOPT_XATTR, NULL };

#define	SAMFSMNT_ONERROR_PANIC_STR		"panic"

static mntopt_t mntopts[] = {
	/* Option name		Cancel Opt	Arg	Flags		Data */
	{ MNTOPT_RW,		rw_cancel,	NULL,	MO_DEFAULT,	NULL},
	{ MNTOPT_RO,		ro_cancel,	NULL,	0,		NULL},
	{ MNTOPT_SUID,		suid_cancel,	NULL,	MO_DEFAULT,	NULL},
	{ MNTOPT_NOSUID,	nosuid_cancel,	NULL,	0,		NULL},
	{ MNTOPT_INTR,		NULL,		NULL,	MO_DEFAULT,	NULL},
	{ MNTOPT_QUOTA,		quota_cancel,	NULL,	MO_IGNORE,	NULL},
	{ MNTOPT_NOQUOTA,	noquota_cancel,	NULL,	0,		NULL},
	{ MNTOPT_LARGEFILES,	NULL,		NULL,	MO_DEFAULT,	NULL},
	{ MNTOPT_ONERROR,	NULL,		SAMFSMNT_ONERROR_PANIC_STR,
							MO_DEFAULT|MO_HASVALUE,
									NULL},
	{ MNTOPT_NOLOGGING,	nologging_cancel, NULL,	MO_DEFAULT,	NULL},
	{ MNTOPT_LOGGING,	logging_cancel,	NULL,	0,		NULL},
	{ MNTOPT_XATTR,		xattr_cancel,	NULL,	MO_DEFAULT,	NULL},
	{ MNTOPT_NOXATTR,	noxattr_cancel,	NULL,	0,		NULL}
};


static mntopts_t samfs_mntopts = {
	sizeof (mntopts) / sizeof (mntopt_t),
	mntopts
};


static vfsdef_t samfs_vfsdef = {
	VFSDEF_VERSION,
	"samfs",			/* type name string */
	samfs_init10,		/* (*vsw_init)() */
	VSW_HASPROTO,
	&samfs_mntopts
};


/* ----- Module linkage information for the kernel.  */

extern	struct	mod_ops	mod_fsops;
static struct modlfs modlfs = {
	&mod_fsops,		/* type of module.  This is a file system */
	SAMFS_NAME_VERSION,	/* "SAM-QFS Storage Archiving Mgmt" */
	&samfs_vfsdef,		/* vfs def table */
};

static struct modlinkage modlinkage = {
	MODREV_1, &modlfs, NULL, NULL, NULL
};


static void
sam_init_client_msg_cache()
{
	samgt.client_msg_cache =
	    kmem_cache_create("sam_client_msg_cache",
	    sizeof (sam_client_msg_t), 0,
	    NULL, NULL, NULL, NULL, NULL, 0);
	ASSERT(samgt.client_msg_cache);
}

static void
sam_delete_client_msg_cache()
{
	if (samgt.client_msg_cache) {
		kmem_cache_destroy(samgt.client_msg_cache);
	}
}


/*
 * ----- _init - Module loading routine.
 *	Modload the SAM-QFS module.
 */

int
_init(void)
{
	return (mod_install(&modlinkage));		/* Install module */
}


/*
 * ----- _fini - Module unloading routine.
 *	Modunload the SAM-QFS module.
 */

int
_fini(void)
{
	sam_mount_t *mp, *next_mp;

	/*
	 * If we failed early initialization, just attempt to unload.
	 */
	if (samgt.fstype == 0) {
		return (mod_remove(&modlinkage));
	}

	/*
	 * If any filesystems mounting or mounted, return busy.
	 */
	mutex_enter(&samgt.global_mutex);
	if (samgt.num_fs_mounted ||
	    samgt.num_fs_mounting ||
	    samgt.num_fs_syncing ||
	    samgt.mp_stale != NULL) {
		mutex_exit(&samgt.global_mutex);
		return (EBUSY);
	}

	/*
	 * Cancel (or wait for) any scheduled tasks.
	 * Any subsequent returns must reinitialize the task queue.
	 */
	sam_taskq_destroy();

	/*
	 * Terminate all shared file system read socket threads.
	 */
	for (mp = samgt.mp_list; mp != NULL; ) {
		if (SAM_IS_SHARED_FS(mp)) {
			int count = 5;

			mutex_enter(&mp->ms.m_cl_wrmutex);
			while ((mp->ms.m_cl_thread ||
			    mp->ms.m_no_clients) && count--) {
				if (mp->ms.m_cl_thread) {
					psignal(ttoproc(mp->ms.m_cl_thread),
					    SIGTERM);
				}
				mutex_exit(&mp->ms.m_cl_wrmutex);
				delay(hz);		/* Delay for 1 second */
				mutex_enter(&mp->ms.m_cl_wrmutex);
			}
			if (mp->ms.m_cl_thread || mp->ms.m_no_clients) {
				mutex_exit(&mp->ms.m_cl_wrmutex);
				mutex_exit(&samgt.global_mutex);
				/*
				 * Must reinitialize the SAM task queue since we
				 * destroyed it.
				 */
				sam_taskq_init();
				return (EBUSY);
			}
			sam_clear_sock_fp(&mp->ms.m_cl_sh);
			mutex_exit(&mp->ms.m_cl_wrmutex);
		}
		mp = mp->ms.m_mp_next;
	}
	mutex_exit(&samgt.global_mutex);

	/*
	 * Terminate all syscall daemon processing.
	 */
	sam_sc_daemon_fini();
	delay(hz);		/* Delay for 1 second */

	/*
	 * Break the linkage and modunload the system call module, samioc.
	 * This prevents further calls from occurring and allows modunload
	 * of samioc.
	 */
	if (samioc_id > 0) {
		if (samioc_link != NULL) {
			/* Tell samioc that it's OK to modunload */
			samioc_link(NULL);
		}
		/*
		 * modunload() samioc module.
		 */
		if (modunload(samioc_id) != 0) {
			cmn_err(CE_WARN,
			    "SAM-QFS: Cannot modunload samioc, samioc_id = %d",
			    samioc_id);
			if (samioc_link != NULL) {
				samioc_link((void *)sam_syscall);
			}
			/*
			 * Must reinitialize the SAM task queue since we
			 * destroyed it.
			 */
			sam_taskq_init();
			return (EBUSY);
		}
	}

	/*
	 * Break the linkage with the AIO pseudo device driver, samaio.
	 */
	if (samgt.samaio_vp != NULL) {
		VOP_CLOSE_OS(samgt.samaio_vp, FWRITE, 1, (offset_t)0, kcred,
		    NULL);
		VN_RELE(samgt.samaio_vp);
		samgt.samaio_vp = NULL;
	}

	/*
	 * From this point forward, we are committed to unloading.
	 * Cancel all callbacks and de-allocate all memory.
	 */
	sam_buf_fini();
	(void) sam_clean_ino();
#if	SAM_TRACE
	(void) sam_trace_fini();
#endif
	sam_amld_fini();

	/*
	 * Free all mount tables.
	 */
	mutex_enter(&samgt.global_mutex);
	for (mp = samgt.mp_list; mp != NULL; ) {
		next_mp = mp->ms.m_mp_next;

		sam_free_incore_host_table(mp);

		kmem_free((void *)mp, sizeof (sam_mount_t) +
		    (sizeof (struct samdent) *
		    (L_FSET - 1)));
		mp = next_mp;
	}
	mutex_exit(&samgt.global_mutex);

	sam_clean_cache();
	sam_delete_sharefs_rpc_item_cache();
	sam_delete_client_msg_cache();
#ifndef _NoOSD_
	sam_delete_object_cache();
#endif
	kmem_free((void *)sam_zero_block, sam_zero_block_size);
	sam_zero_block = NULL;
	sam_zero_block_size = 0;
	tsd_destroy(&samgt.tsd_fsflush_key);

	sam_fini_kstats();

	lqfs_fini();

	sam_objctl_fini();
#ifndef _NoOSD_
	sam_sosd_unbind();
#endif

	return (mod_remove(&modlinkage));		/* Remove module */
}


/*
 * ----- _info - Module information.
 *	Get module information for the SAM-QFS module.
 */

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop)); /* Get module information */
}


static int
samfs_init10(int fstype, char *name)
{
	static const fs_operation_def_t samfs_vfsops_template[] = {
		VFSNAME_MOUNT, samfs_mount,
		VFSNAME_UNMOUNT, samfs_umount,
		VFSNAME_ROOT, samfs_root,
		VFSNAME_STATVFS, samfs_statvfs,
		VFSNAME_SYNC, (fs_generic_func_p) samfs_sync,
		VFSNAME_VGET, samfs_vget,
		VFSNAME_FREEVFS, (fs_generic_func_p) samfs_freevfs,
		NULL, NULL
	};
	int error;

	error = vfs_setfsops(fstype, samfs_vfsops_template, &samfs_vfsopsp);
	if (error != 0) {
		cmn_err(CE_WARN, "SAM-QFS: samfsinit: bad vfs ops template");
		return (error);
	}

	error = vn_make_ops(name, samfs_vnodeops_template, &samfs_vnodeopsp);
	if (error != 0) {
		(void) vfs_freevfsops_by_type(fstype);
		cmn_err(CE_WARN, "SAM-QFS: samfsinit: bad vnode ops template");
		return (error);
	}

	/* do it again for shared FS vnodes */
	error = vn_make_ops(name, samfs_client_vnodeops_template,
	    &samfs_client_vnodeopsp);
	if (error != 0) {
		(void) vfs_freevfsops_by_type(fstype);
		cmn_err(CE_WARN,
		    "SAM-QFS: samfsinit: bad vnode client ops template");
		return (error);
	}

	/* yet again for the forcibly-unmounted vnodes */
	error = vn_make_ops(name, samfs_vnode_staleops_template,
	    &samfs_vnode_staleopsp);
	if (error != 0) {
		(void) vfs_freevfsops_by_type(fstype);
		cmn_err(CE_WARN,
		    "SAM-QFS: samfsinit: bad vnode EIO ops template");
		return (error);
	}

	/* and yet again for the forcibly-unmounted shared vnodes */
	error = vn_make_ops(name, samfs_client_vnode_staleops_template,
	    &samfs_client_vnode_staleopsp);
	if (error != 0) {
		(void) vfs_freevfsops_by_type(fstype);
		cmn_err(CE_WARN,
		    "SAM-QFS: samfsinit: bad vnode EIO shared ops template");
		return (error);
	}
	return (samfs_init(NULL, fstype));
}

/* ARGSUSED */
static int
sam_msg_array_constructor(void *buf, void *not_used, int notused)
{
	bzero(buf, sizeof (sam_msg_array_t));
	return (0);
}

static void
sam_init_cache()
{
	samgt.msg_array_cache =
	    kmem_cache_create("sam_msg_array_cache",
	    sizeof (sam_msg_array_t), 0,
	    sam_msg_array_constructor,
	    NULL /* destructor */, NULL, NULL, NULL, 0);
	ASSERT(samgt.msg_array_cache != NULL);
}

static void
sam_clean_cache()
{
	if (samgt.msg_array_cache != NULL) {
		kmem_cache_destroy(samgt.msg_array_cache);
	}
}

/*
 * ----- samfs_init - Initialize the SAM filesystem.
 *	Initialize the SAM-QFS filesystem. Called after SAMFS module loaded.
 *	Allocate the global defaults and lock table.
 */

static int			/* ERRNO if error, 0 if successful. */
samfs_init(
/* LINTED argument unused in function */
	struct vfssw *vfsswp,	/* vfssw pointer for SAMFS. */
	int fstype)		/* SAMFS file index number. */
{
	int error;

	/*
	 * Solaris VPM default is on for x64, off for SPARC.
	 */
	if (sam_vpm_enable) {
		sam_vpm_enable = vpm_enable;
	}

	/*
	 * modload() the system call processor and set the actual
	 * system call processor linkage.
	 */
	if ((samioc_id = modload("drv", "samioc")) == -1) {
		cmn_err(CE_WARN, "SAM-QFS: modload(samioc) failed.");
		return (-1);
	}
	samioc_link = (void(*)(void *))modlookup("samioc", "samioc_link");
	if (samioc_link == NULL) {
		cmn_err(CE_WARN,
		    "SAM-QFS: modlookup(samioc, samioc_link) failed.");
		return (-1);
	}
	samgt.samioc_major = ddi_name_to_major("samioc");

	/*
	 * open() the AIO pseudo device driver.
	 */
	if ((error = vn_open("/dev/samaioctl", UIO_SYSSPACE, FEXCL|FWRITE, 0,
			&samgt.samaio_vp, (enum create)0, 0)) != 0) {
		cmn_err(CE_WARN,
			"SAM-QFS: open(\"/dev/samaioctl\") failed, err=%d.",
			error);
		samgt.samaio_vp = NULL;
	}

	/*
	 * Set the samfs filsys type = index for filsys switch table.
	 * Also flags that the system call was loaded successfully.
	 */
	samgt.fstype = fstype;

	sam_mutex_init(&samgt.global_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&samgt.buf_mutex, NULL, MUTEX_DEFAULT, NULL);
	sam_mutex_init(&samgt.time_mutex, NULL, MUTEX_DEFAULT, NULL);

	samgt.buf_freelist = NULL;
	tsd_create(&samgt.tsd_fsflush_key, sam_tsd_destroy);

	sam_zero_block_size = SAM_BLK * 2;
	sam_zero_block = (char *)kmem_zalloc(sam_zero_block_size, KM_SLEEP);

	sam_init_cache();	/* Initialize memory cache */
	sam_init_ino();		/* Initialize the inode hash & free chains. */
	sam_sc_daemon_init();	/* Initialize system call daemon processing. */
	sam_init_sharefs_rpc_item_cache();
	sam_init_client_msg_cache();
#ifndef _NoOSD_
	sam_init_object_cache();
#endif
#if	SAM_TRACE
	sam_trace_init();	/* Initialize the trace table. */
#endif
	sam_amld_init();

	sam_init_kstats();
	sam_taskq_init();

	/*
	 * Tell samioc where the system call processor is.
	 */
	samioc_link((void *)sam_syscall);

	lqfs_init();

#ifndef _NoOSD_
	sam_objctl_init();
	sam_sosd_bind();
#endif
#ifndef feature_not_needed_anymore
	/* license all features */
	samgt.license.license.lic_u.whole = (uint) -1;
#endif
	return (0);
}


#define	CLR_LSB(x)	((x) &= ((x)-1))	/* clear least-sig 1-bit in x */
#define	MULTI_BIT(x)	((x) & ((x)-1))		/* !0 if > 1 bit set in x */


/*
 * ----- sam_init_ino - Initialize the SAM inode chains.
 *	Initialize the SAM filesystem inode hash chains and locks.
 *	Initialize the SAM filesystem inode free chain pointers.
 *	Initialize the SAM filesystem trace lock.
 */

static void
sam_init_ino(void)
{
	sam_ihead_t *hip;
	int	i;

	/*
	 * Find suitable values for ninodes and nhino.
	 */
	if (!ninodes) {		/* not set in /etc/system.  default to ncsize */
		ninodes = ncsize;
	}
	if (ninodes < SAM_NINODES_MIN || ninodes > SAM_NINODES_MAX) {
		cmn_err(CE_WARN,
		    "SAM-QFS: ninodes out of range (%d) [%d-%d]; "
		    "resetting to %d.",
		    ninodes, SAM_NINODES_MIN, SAM_NINODES_MAX, SAM_NINODES_DEF);
		ninodes = SAM_NINODES_DEF;
	}

	if (!nhino) {
		nhino = ninodes / 8;
		while (MULTI_BIT(nhino)) {
			CLR_LSB(nhino);
		}
		nhino = nhino << 1;
	}
	if (nhino <= 0 || nhino > SAM_NHINO_MAX) {
		cmn_err(CE_WARN,
		    "SAM-QFS: nhino out of range (%d) [1 - %d]; using %d.",
		    nhino, SAM_NHINO_MAX, SAM_NHINO_DEF);
		nhino = SAM_NHINO_DEF;
	}
	if (MULTI_BIT(nhino)) {
		/*
		 * nhino must be a power of 2.  Pick the next larger power of 2.
		 */
		for (i = nhino; MULTI_BIT(i); CLR_LSB(i)) {
			;
		}
		i = i << 1;
		cmn_err(CE_WARN,
		    "SAM-QFS: nhino (%d) not a power of two; using %d",
		    nhino, i);
		nhino = i;
	}

	samgt.ninodes = ninodes;
	samgt.nhino = nhino;
	samgt.ihashlock = (kmutex_t *)kmem_zalloc(samgt.nhino *
	    sizeof (kmutex_t),
	    KM_SLEEP);
	samgt.ihashhead = (sam_ihead_t *)kmem_zalloc
	    (samgt.nhino * sizeof (sam_ihead_t), KM_SLEEP);
	hip = (sam_ihead_t *)&samgt.ihashhead[0];
	for (i = 0; i < samgt.nhino; i++, hip++) {
		hip->forw = (sam_node_t *)(void *)hip;
		hip->back = (sam_node_t *)(void *)hip;
		sam_mutex_init(&samgt.ihashlock[i], NULL, MUTEX_DEFAULT, NULL);
	}

	sam_mutex_init(&samgt.ifreelock, NULL, MUTEX_DEFAULT, NULL);
	samgt.ifreehead.hash.forw = NULL;
	samgt.ifreehead.hash.back = NULL;
	samgt.ifreehead.free.forw = (sam_node_t *)(void *)&samgt.ifreehead;
	samgt.ifreehead.free.back = (sam_node_t *)(void *)&samgt.ifreehead;
	sam_create_ino_cache();
}


/*
 * ----- sam_clean_ino - Free the SAM inode chains.
 *	Free the SAM filesystem inode hash chains and locks.
 */

static void
sam_clean_ino(void)
{
	sam_delete_ino_cache();
	kmem_free((void *)samgt.ihashlock, (samgt.nhino * sizeof (kmutex_t)));
	kmem_free((void *)samgt.ihashhead,
	    (samgt.nhino * sizeof (sam_ihead_t)));
}


/*
 * ----- sam_buf_fini - free memory acquired for async buf headers.
 */
static void
sam_buf_fini()
{
	sam_uintptr_t *bp, *nbp;

	mutex_enter(&samgt.buf_mutex);
	bp = (sam_uintptr_t *)samgt.buf_freelist;
	while (bp) {
		nbp = (sam_uintptr_t *)*bp;
		kmem_free((caddr_t *)bp, sizeof (sam_buf_t));
		bp = nbp;
	}
	mutex_exit(&samgt.buf_mutex);
}

#define	SAM_KSTAT32(cat, stat) \
	kstat_named_init(&sam_##cat##_stats.stat, #stat, KSTAT_DATA_ULONG)
#define	SAM_KSTAT64(cat, stat) \
	kstat_named_init(&sam_##cat##_stats.stat, #stat, KSTAT_DATA_ULONGLONG)
#define	SAM_KSTAT32N(cat, stat, name) \
	kstat_named_init(&sam_##cat##_stats.stat, name, KSTAT_DATA_ULONG)
#define	SAM_KSTAT64N(cat, stat, name) \
	kstat_named_init(&sam_##cat##_stats.stat, name, KSTAT_DATA_ULONGLONG)


/*
 * ----- sam_init_kstats - Initialize the SAM-QFS kernel statistics structures.
 */
static void
sam_init_kstats(void)
{
	sam_gen_kstats = kstat_create("sam-qfs", 0, "general", "fs",
	    KSTAT_TYPE_NAMED,
	    sizeof (struct sam_gen_statistics) / sizeof (kstat_named_t),
	    KSTAT_FLAG_VIRTUAL);
	if (sam_gen_kstats == NULL) {
		goto fail;
	}
	sam_gen_kstats->ks_data = &sam_gen_stats;
	sam_gen_kstats->ks_data_size += strlen(sam_version) + 1;
	sam_gen_kstats->ks_update = sam_update_gen_kstats;

	kstat_named_init(&sam_gen_stats.version, "version", KSTAT_DATA_STRING);
	kstat_named_setstr(&sam_gen_stats.version, sam_version);
	SAM_KSTAT32N(gen, n_fs_configured, "configured file systems");
	SAM_KSTAT32N(gen, n_fs_mounted, "mounted file systems");
	SAM_KSTAT32(gen, nhino);
	SAM_KSTAT32(gen, ninodes);
	SAM_KSTAT32(gen, inocount);
	SAM_KSTAT32(gen, inofree);

	sam_dio_kstats = kstat_create("sam-qfs", 0, "dio", "fs",
	    KSTAT_TYPE_NAMED,
	    sizeof (struct sam_dio_statistics) / sizeof (kstat_named_t),
	    KSTAT_FLAG_VIRTUAL);
	if (sam_dio_kstats == NULL) {
		goto fail;
	}
	sam_dio_kstats->ks_data = &sam_dio_stats;

	SAM_KSTAT64N(dio, mmap_2_dio, "mmap_to_dio");
	SAM_KSTAT64N(dio, dio_2_mmap, "dio_to_mmap");

	sam_page_kstats = kstat_create("sam-qfs", 0, "page", "fs",
	    KSTAT_TYPE_NAMED,
	    sizeof (struct sam_page_statistics) / sizeof (kstat_named_t),
	    KSTAT_FLAG_VIRTUAL);
	if (sam_page_kstats == NULL) {
		goto fail;
	}
	sam_page_kstats->ks_data = &sam_page_stats;

	SAM_KSTAT64(page, flush);
	SAM_KSTAT64(page, retry);

	sam_sam_kstats = kstat_create("sam-qfs", 0, "sam", "fs",
	    KSTAT_TYPE_NAMED,
	    sizeof (struct sam_sam_statistics) / sizeof (kstat_named_t),
	    KSTAT_FLAG_VIRTUAL);
	if (sam_sam_kstats == NULL) {
		goto fail;
	}
	sam_sam_kstats->ks_data = &sam_sam_stats;

	SAM_KSTAT64(sam, stage_start);
	SAM_KSTAT64(sam, stage_partial);
	SAM_KSTAT64(sam, stage_window);
	SAM_KSTAT64(sam, stage_errors);
	SAM_KSTAT64(sam, stage_reissue);
	SAM_KSTAT64(sam, stage_cancel);
	SAM_KSTAT64(sam, archived);
	SAM_KSTAT64(sam, archived_incon);
	SAM_KSTAT64(sam, archived_rele);
	SAM_KSTAT64(sam, noarch_incon);
	SAM_KSTAT64(sam, noarch_already);

#ifdef METADATA_SERVER
	sam_shared_server_kstats = kstat_create("sam-qfs", 0,
	    "shared_server", "fs",
	    KSTAT_TYPE_NAMED,
	    sizeof (struct sam_shared_server_statistics) /
	    sizeof (kstat_named_t),
	    KSTAT_FLAG_VIRTUAL);
	if (sam_shared_server_kstats == NULL) {
		goto fail;
	}
	sam_shared_server_kstats->ks_data = &sam_shared_server_stats;

	SAM_KSTAT64(shared_server, msg_in);
	SAM_KSTAT64(shared_server, msg_out);
	SAM_KSTAT64(shared_server, msg_dup);
	SAM_KSTAT64(shared_server, lease_new);
	SAM_KSTAT64(shared_server, lease_add);
	SAM_KSTAT64(shared_server, lease_wait);
	SAM_KSTAT64(shared_server, lease_grow);
	SAM_KSTAT64(shared_server, failovers);
	SAM_KSTAT64(shared_server, callout_action);
	SAM_KSTAT64(shared_server, callout_relinquish_lease);
	SAM_KSTAT64(shared_server, callout_directio);
	SAM_KSTAT64(shared_server, callout_stage);
	SAM_KSTAT64(shared_server, callout_abr);
	SAM_KSTAT64(shared_server, callout_acl);
	SAM_KSTAT64(shared_server, notify_lease);
	SAM_KSTAT64(shared_server, notify_dnlc);
	SAM_KSTAT64(shared_server, notify_expire);
	SAM_KSTAT64(shared_server, expire_task);
	SAM_KSTAT64(shared_server, expire_task_dup);
#endif /* METADATA_SERVER */

	sam_shared_client_kstats = kstat_create("sam-qfs", 0,
	    "shared_client", "fs",
	    KSTAT_TYPE_NAMED,
	    sizeof (struct sam_shared_client_statistics) /
	    sizeof (kstat_named_t),
	    KSTAT_FLAG_VIRTUAL);
	if (sam_shared_client_kstats == NULL) {
		goto fail;
	}
	sam_shared_client_kstats->ks_data = &sam_shared_client_stats;

	SAM_KSTAT64(shared_client, msg_in);
	SAM_KSTAT64(shared_client, msg_out);
	SAM_KSTAT64(shared_client, sync_pages);
	SAM_KSTAT64(shared_client, inval_pages);
	SAM_KSTAT64(shared_client, stale_indir);
	SAM_KSTAT64(shared_client, dio_switch);
	SAM_KSTAT64(shared_client, abr_switch);
	SAM_KSTAT64(shared_client, expire_task);
	SAM_KSTAT64(shared_client, expire_task_dup);
	SAM_KSTAT64(shared_client, retransmit_msg);
	SAM_KSTAT64(shared_client, notify_ino);
	SAM_KSTAT64(shared_client, notify_expire);
	SAM_KSTAT64(shared_client, expired_inuse);
	SAM_KSTAT64(shared_client, page_lease_retry);

	sam_thread_kstats = kstat_create("sam-qfs", 0, "thread", "fs",
	    KSTAT_TYPE_NAMED,
	    sizeof (struct sam_thread_statistics) / sizeof (kstat_named_t),
	    KSTAT_FLAG_VIRTUAL);
	if (sam_thread_kstats == NULL) {
		goto fail;
	}
	sam_thread_kstats->ks_data = &sam_thread_stats;

	SAM_KSTAT64N(thread, block_count, "block thread count");
	SAM_KSTAT64N(thread, reclaim_count, "reclaim thread count");
	SAM_KSTAT64N(thread, taskq_add, "tasks requested");
	SAM_KSTAT64N(thread, taskq_add_dup, "tasks already scheduled");
	SAM_KSTAT64N(thread, taskq_add_immed, "tasks dispatched immediately");
	SAM_KSTAT64N(thread, taskq_dispatch, "tasks executed immediately");
	SAM_KSTAT64N(thread, taskq_dispatch_fail, "tasks requeued");
	SAM_KSTAT64N(thread, inactive_count, "inactivate task count");
	SAM_KSTAT64N(thread, max_share_threads, "max sharefs threads");

	sam_dnlc_kstats = kstat_create("sam-qfs", 0, "ednlc", "fs",
	    KSTAT_TYPE_NAMED,
	    sizeof (struct sam_dnlc_statistics) / sizeof (kstat_named_t),
	    KSTAT_FLAG_VIRTUAL);
	if (sam_dnlc_kstats == NULL) {
		goto fail;
	}
	sam_dnlc_kstats->ks_data = &sam_dnlc_stats;

	SAM_KSTAT64N(dnlc, ednlc_starts, "dir dnlc starts");
	SAM_KSTAT64N(dnlc, ednlc_purges, "dir dnlc purges");
	SAM_KSTAT64N(dnlc, ednlc_name_hit, "dir dnlc name cache hit");
	SAM_KSTAT64N(dnlc, ednlc_name_found, "dir dnlc name found");
	SAM_KSTAT64N(dnlc, ednlc_name_missed, "dir dnlc name missed");
	SAM_KSTAT64N(dnlc, ednlc_space_found, "dir dnlc space found");
	SAM_KSTAT64N(dnlc, ednlc_space_missed, "dir dnlc space missed");
	SAM_KSTAT64N(dnlc, ednlc_too_big, "dir dnlc error too big");
	SAM_KSTAT64N(dnlc, ednlc_no_mem, "dir dnlc error no mem");
	SAM_KSTAT64N(dnlc, dnlc_neg_entry, "neg dnlc entries made");
	SAM_KSTAT64N(dnlc, dnlc_neg_uses, "neg dnlc entries used");

#ifdef DEBUG
	sam_debug_kstats = kstat_create("sam-qfs", 0, "debug", "fs",
	    KSTAT_TYPE_NAMED,
	    sizeof (struct sam_debug_statistics) / sizeof (kstat_named_t),
	    KSTAT_FLAG_VIRTUAL);
	if (sam_debug_kstats == NULL) {
		goto fail;
	}

	sam_debug_kstats->ks_data = &sam_debug_stats;

	SAM_KSTAT64N(debug, nreads, "reads");
	SAM_KSTAT64N(debug, nwrites, "writes");
	SAM_KSTAT64N(debug, breads, "rbytes");
	SAM_KSTAT64N(debug, bwrites, "wbytes");
	SAM_KSTAT64(debug, client_dir_putpage);
#endif

	kstat_install(sam_gen_kstats);
	kstat_install(sam_dio_kstats);
	kstat_install(sam_page_kstats);
	kstat_install(sam_sam_kstats);
#ifdef	METADATA_SERVER
	kstat_install(sam_shared_server_kstats);
#endif
	kstat_install(sam_shared_client_kstats);
	kstat_install(sam_thread_kstats);
	kstat_install(sam_dnlc_kstats);
#ifdef DEBUG
	kstat_install(sam_debug_kstats);
#endif

	return;

fail:
	cmn_err(CE_WARN, "SAM-QFS: failed to initialize statistics collection");
	sam_fini_kstats();
}


/*
 * ----- sam_fini_kstats - Delete the kernel statistics structure.
 */
static void
sam_fini_kstats(void)
{
	if (sam_gen_kstats != NULL) {
		kstat_delete(sam_gen_kstats);
		sam_gen_kstats = NULL;
	}
	if (sam_dio_kstats != NULL) {
		kstat_delete(sam_dio_kstats);
		sam_dio_kstats = NULL;
	}
	if (sam_page_kstats != NULL) {
		kstat_delete(sam_page_kstats);
		sam_page_kstats = NULL;
	}
	if (sam_sam_kstats != NULL) {
		kstat_delete(sam_sam_kstats);
		sam_sam_kstats = NULL;
	}
	if (sam_shared_server_kstats != NULL) {
		kstat_delete(sam_shared_server_kstats);
		sam_shared_server_kstats = NULL;
	}
	if (sam_shared_client_kstats != NULL) {
		kstat_delete(sam_shared_client_kstats);
		sam_shared_client_kstats = NULL;
	}
	if (sam_thread_kstats != NULL) {
		kstat_delete(sam_thread_kstats);
		sam_thread_kstats = NULL;
	}
	if (sam_dnlc_kstats != NULL) {
		kstat_delete(sam_dnlc_kstats);
		sam_dnlc_kstats = NULL;
	}
#ifdef DEBUG
	if (sam_debug_kstats != NULL) {
		kstat_delete(sam_debug_kstats);
		sam_debug_kstats = NULL;
	}
#endif
}


/*
 * ----- sam_update_gen_kstats - Update statistics from the samgt struct.
 */

/* ARGSUSED */
static int
sam_update_gen_kstats(struct kstat *ksp, int rw)
{
	if (rw == KSTAT_WRITE) {
		return (EACCES);
	} else {
		sam_gen_stats.n_fs_configured.value.ul =
		    samgt.num_fs_configured;
		sam_gen_stats.n_fs_mounted.value.ul = samgt.num_fs_mounted;
		sam_gen_stats.nhino.value.ul = samgt.nhino;
		sam_gen_stats.ninodes.value.ul = samgt.ninodes;
		sam_gen_stats.inocount.value.ul = samgt.inocount;
		sam_gen_stats.inofree.value.ul = samgt.inofree;
		return (0);
	}
}
