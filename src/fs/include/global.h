/*
 *	global.h - global table for all the SAM-FS file systems.
 *
 *	Defines the structure of the global table for all of the SAM-FS mounts.
 *
 *	Solaris 2.x Sun Storage & Archiving Management File System
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

#ifndef	_SAM_FS_GLOBAL_H
#define	_SAM_FS_GLOBAL_H

#ifdef sun
#pragma ident "$Revision: 1.56 $"
#endif


#ifdef	sun
#include	<sys/pathname.h>
#include	<sys/vfs.h>
#endif	/* sun */

#include	<sam/types.h>

#ifdef METADATA_SERVER
#include	<license/license.h>
#endif	/* METADATA_SERVER */

#include	"inode.h"
#include	"sam_thread.h"

/* Begin: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack(4)
#endif

/*
 * sam_global_tbl_t is the global table for all SAM-FS filsystems.
 * inocount & inofree -- protected by ifreelock.
 * global_cv and flags protected by global_mutex.
 *
 * Note that the 'pragma pack(4)' done for amd64 can result in
 * mutexes that are not 64bit aligned so add padding where needed
 * when changing this structure.
 */

typedef struct {
	sam_ichain_t ifreehead;		/* Head of inode free chain. */
	sam_ihead_t *ihashhead;		/* array of head hash pointers */
	sam_mount_t *mp_list;		/* Head of mount points */
	sam_mount_t *mp_stale;		/* Ex-mnt struct list (umount -f'd) */
#ifdef	sun
	vnode_t		*samaio_vp;	/* Samaio vnode pointer */
	buf_t		*buf_freelist;	/* Head of buffer freelist */
#endif	/* sun */
	kmutex_t	ifreelock;	/* Lock for inode free chain. */
	kmutex_t	*ihashlock;	/* array of hash chain locks */

	int	num_fs_configured;	/* number of file systems configured */
	int	num_fs_mounting;	/* number of file systems mounting */
	int	num_fs_mounted;		/* number of file systems mounted */
	int	num_fs_syncing;		/* number of file systems syncing */
#ifdef sun
	int	nhino;			/* Size of inode hash table */
	int	ninodes;		/* Max number of allocated inodes */
#endif
	int	inocount;		/* Count of allocated incore inodes */
	int	inofree;		/* Count of inodes in free chain */
	int	fstype;			/* SAM-FS module ordinal number */
	int	buf_wait;		/* Waiting for buffer */

	/* sam-amld daemon pid.  0 => never started; -1 => not running */
	int	amld_pid;
#ifdef	sun
	/*
	 * sam-stagerd daemon pid. 0 => not running;
	 *   -1 => shutdown for failover.
	 */
	int	stagerd_pid;
#endif
#ifdef linux
	int		meta_minor;	/* Highest meta minor number */
#endif /* linux */
	int		samioc_major;	/* samioc major */
	char		pad[4];
	kmutex_t	buf_mutex;	/* Mutex for buffer header */
	kmutex_t	global_mutex;	/* mutex for the global area */
	kmutex_t	time_mutex;	/* mutex for the unique time */
#ifdef	linux
#ifdef __KERNEL__
	kmem_cache_t	*inode_cache;
	kmem_cache_t	*item_cache;
	kmem_cache_t	*client_msg_cache;
#else
	/* kmem_cache_t defined in <linux/slab.h> */
	void		*inode_cache;
	void		*item_cache;
	void		*client_msg_cache;
#endif
#endif /* linux */
#ifdef sun
	struct kmem_cache *inode_cache; /* incore inode cache pointer */
	struct kmem_cache *object_cache; /* Object private cache pointer */
	struct kmem_cache *msg_array_cache; /* cache for srvr msg array */
	struct kmem_cache *item_cache; /* cache for incoming RPC msgs */
	struct kmem_cache *client_msg_cache; /* cache for client msgs */
#endif
	kmutex_t	schedule_mutex;		/* Mutex for scheduler */
	sam_schedule_queue_t schedule;		/* Scheduler queue head */
	uint32_t	schedule_count;		/* Count of scheduled tasks */
	uint32_t	schedule_flags;		/* Scheduler flags */
	kcondvar_t	schedule_cv;		/* CV for scheduler */
#ifdef METADATA_SERVER
	/* license value, specifying features */
	sam_license_t_33	license;
#endif	/* METADATA_SERVER */
#ifdef sun
	uint_t	tsd_fsflush_key;	/* TSD key for fsflush check */
#endif
} sam_global_tbl_t;

typedef struct {
	sam_mount_t *mp_list;			/* Head of mount points */
	sam_mount_t *mp_stale;			/* Ex-mnt list (umount -f'd) */
	int		num_fs_configured;	/* num FS's configured */
	int		num_fs_mounting;	/* num file systems mounting */
	int		num_fs_mounted;		/* num file systems mounted */
	int		num_fs_syncing;		/* num file systems syncing */
#ifdef sun
	int		nhino;			/* Size of inode hash table */
	int		ninodes;		/* Max num of alloc'd inodes */
#endif
	int		inocount;		/* Cnt alloc'd incore inodes */
	int		inofree;		/* Cnt inodes in free chain */
	int		fstype;			/* SAM-FS module ord number */
	int		buf_wait;		/* Waiting for buffer */
	int		amld_pid;		/* sam-amld, -1 not running */
#ifdef linux
	int		meta_minor;		/* Highest meta minor number */
#endif /* linux */
	int		samioc_major;		/* samioc major */
#ifdef	linux
#ifdef __KERNEL__
	kmem_cache_t	*inode_cache;
#else
	void		*inode_cache; /* kmem_cache_t in <linux/slab.h> */
#endif
#else
	struct kmem_cache *inode_cache; /* SAM-FS incore inode cache pointer */
#endif	/* linux */
	uint32_t	schedule_count;		/* Count of scheduled tasks */
	uint32_t	schedule_flags;		/* Scheduler flags */
} trace_global_tbl_t;

/* End: 32-bit align copyin() structs for amd64 only due to 32-bit x86 ABI */
#if _LONG_LONG_ALIGNMENT == 8 && _LONG_LONG_ALIGNMENT_32 == 4
#pragma pack()
#endif


#endif /* _SAM_FS_GLOBAL_H */
