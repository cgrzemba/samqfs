/*
 *	config.h - configuration related definitions
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#if !defined(SHFS_CONFIG_H)
#define	SHFS_CONFIG_H

#pragma ident "$Revision: 1.20 $"

/*
 * These bits + structure keeps track of all the items that
 * reflect the filesystem configuration.  This includes the FS
 * partition list, either the label block or the FS superblock
 * (depending on whether this host is a client-only host or not),
 * and the modification time of the hosts.<fs>.local file.
 *
 * Periodically, we may be called and asked if anything has
 * changed lately.  We need to be able to answer authoritatively.
 */

#define	R_MNTD		0x10000	/* FS is mounted */
#define	R_LBLK_VERS	0x20000	/* bad label block version # */
#define	R_FOREIGN	0x40000	/* Native FS byte order is foreign */

#define	R_FSCFG_BITS	0x0c000
#define	R_FSCFG		0x08000	/* tried to get fs config from OS */
#define	R_FSCFG_ERR	0x04000	/* failed */

#define	R_SBLK_BITS	0x00f00
#define	R_SBLK		0x00800	/* tried to get root info */
#define	R_SBLK_ERR	0x00400	/* misc error, but not busy */
#define	R_SBLK_BUSY	0x00200	/* can't open dev (EBUSY) */
#define	R_SBLK_NOFS	0x00100	/* no or bad superblock */

#define	R_LBLK_BITS	0x000f0
#define	R_LBLK		0x00080	/* tried to get label */
#define	R_LBLK_ERR	0x00040	/* misc error, but not busy */
#define	R_LBLK_BUSY	0x00020	/* can't open dev (EBUSY) */
#define	R_LBLK_NONE	0x00010	/* no or bad label */

#define	R_LHOSTS_BITS	0x0000d
#define	R_LHOSTS	0x00008	/* tried to get local hosts file */
#define	R_LHOSTS_ERR	0x00004	/* misc error */
#define	R_LHOSTS_NONE	0x00001	/* no local hosts file */


#define	R_ERRBITS	(R_FSCFG_ERR | R_SBLK_ERR | R_LBLK_ERR | R_LHOSTS_ERR)
#define	R_BUSYBITS	(R_SBLK_BUSY | R_LBLK_BUSY)

struct shfs_config {
	int init;				/* this struct initialized */
	int curTab;				/* current copy */
	int freeTab;				/* non-current copy */
	sam_time_t fsId;			/* fs 'init' timestamp */
	struct sam_mount_info mnt;		/* kernel FS config */
	struct cfdata {
		int flags;			/* R_* flags, above */
		sam_time_t fsInit;		/* fs 'init' timestamp */
		offset_t hostsOffset;		/* disk offset of .hosts */
		upath_t serverName;
		upath_t serverAddr;
		struct sam_label_blk lb;	/* label block from fs disk */
		timestruc_t lHostModTime;	/* mod time, hosts.fs.local */
		struct sam_host_table_blk *ht;	/* host table from root slice */
		int htsize;
	} cp[2];
};

/*
 * Change flags returned by CmpConfig() and the
 * structure comparison functions.
 *
 * Split by categories:
 *  _CF - in core configuration changes
 *  _SB - superblock changes
 *  _HT - hosts table changes
 *  _LB - label block changes
 *  _LC - local hosts file changes
 */
#define	CH_CF_FLAGS	0x00000001		/* config item [dis]appeared */
#define	CH_CF_MOUNT	0x00000002		/* mount/unmount occurred */
#define	CH_CF_PART	0x00000004		/* partition cmt/item changed */

#define	CH_SB_SBLK	0x00000010		/* superblock data changed */
#define	CH_SB_NOFS	0x00000020		/* No FS superblock */
#define	CH_SB_NOTSH	0x00000040		/* FS no longer shared */
#define	CH_SB_NEWFS	0x00000080		/* New fs ID (usu. mkfs) */
#define	CH_SB_FSNAME	0x00000100		/* FS name changed (!config) */

#define	CH_HT_HOSTS	0x00001000		/* hosts file changed */
#define	CH_HT_BAD	0x00002000		/* bad hosts file */
#define	CH_HT_VERS	0x00004000		/* hosts file vers changed */
#define	CH_HT_RESET	0x00008000		/* hosts file gen # reset */
#define	CH_HT_INVOL	0x00010000		/* involuntary failover */
#define	CH_HT_PEND	0x00020000		/* pending voluntary failover */

#define	CH_LB_LABEL	0x00100000		/* label changed */
#define	CH_LB_BAD	0x00200000		/* bad label block */
#define	CH_LB_RESET	0x00400000		/* label block gen # reset */
#define	CH_LB_NAME	0x00800000		/* label block FS name */
#define	CH_LB_FSID	0x01000000		/* label block FS ID */
#define	CH_LB_SRVR	0x02000000		/* label block FS server */

#define	CH_LC_HOSTS	0x10000000		/* local host file change */


/*
 * Flags for device open() calls.  We need to disable caching
 * whenever we're doing I/Os to the superblock, shared hosts
 * file, and label block.  These vary slightly by OS.
 */

#ifdef sun
#define		OPEN_READ_RAWFLAGS	(O_RDONLY | O_LARGEFILE | O_RSYNC)
#define		OPEN_RDWR_RAWFLAGS	(O_RDWR | O_LARGEFILE | O_RSYNC)
#endif	/* sun */

#ifdef linux
#define		OPEN_READ_RAWFLAGS	(O_RDONLY | O_RSYNC | O_DIRECT)
#define		OPEN_RDWR_RAWFLAGS	(O_RDWR | O_RSYNC | O_DIRECT)
#endif	/* linux */

int GetConfig(char *fs, struct sam_sblk *);
int CmpConfig(char *fs, struct sam_sblk *);

#endif	/* SHFS_CONFIG_H */
