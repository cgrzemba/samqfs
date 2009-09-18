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

/*
 * event.h: Header file for SUNW.qfs RT implementation.
 */

#ifndef _SCQFS_H
#define	_SCQFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <rgm/libdsdev.h>
#include "sam/mount.h"
#include "sam/lib.h"

/*
 * Extension property name for Shared QFS filesystem
 * -- a stringarray property, containing the resource's QFS
 * mount points.
 */
#define	QFSFS				"QFSFileSystem"

/*
 * Extension property name for Shared QFS filesystem
 * -- how long the server should wait for clients to unmount
 * before itself unmounting.
 */
#define	QFS_UMNT_WAIT		"QFS_umount_await_clients"

/*
 * Extension property to override the default acceleration
 * policy under operation in SunCluster.
 */
#define	QFS_LEASE_POLICY	"QFSLeaseRecoveryPolicy"


#define	FSD			0
#define	RAW			1

#define	NO_SERVER	"(NONE)"

#define		FO_DONE		0
#define		FO_NOSRVR	1
#define		FO_MDSDIED	2
#define		FO_ERROR	-1

#define	SCQFS_TMP_DIR	"/var/run"
#define	SCQFS_MOUNT_NOMDS_SLEEP	5

/*
 * Flag bits from mp->mt.fi_status that relate to failover and
 * failover recovery (include/sam/mount.h).  FS_LOCK_HARD might
 * also be included, but if set during failover, FS_FREEZING or
 * FS_FROZEN should also always be set, and it can be set w/o
 * having a failover in progress.
 */
#define		FS_MDS_CHANGING	\
	(FS_FREEZING | FS_FROZEN | FS_THAWING | \
		FS_RESYNCING | FS_SRVR_DONE | FS_CLNT_DONE)


#define		RG_LEASEPOLICY_FLAGS		0x3
#define		RG_LEASEPOLICY_AUTO			0x0
#define		RG_LEASEPOLICY_TIMEOUT		0x1
#define		RG_LEASEPOLICY_CLUSTER		0x2

/*
 * Debug level values.
 *
 * LOW - Minimal debug messages. meant for low level debug messages
 *		with specific error capture data.
 * MED - Use for tracing execution path.
 * HIGH - Very high level messages (e.g.entrance to/exit from functions).
 */
#define		DBG_LVL_LOW	1
#define		DBG_LVL_MED	5
#define		DBG_LVL_HIGH	9

/*
 * ds/cluster/rg info describing everything about the
 * resource group we're responsible for.
 */
struct RgInfo {
	scds_handle_t			rg_scdsh;
	scha_cluster_t			rg_cl;
	scha_resourcegroup_t	rg_rgh;
	char				   *rg_nodename;
	const char			   *rg_name;
	scha_str_array_t	   *rg_nodes;
	scha_str_array_t	   *rg_mntpts;
	int						rg_umnt_wait;
	int						rg_flags;
};


/*
 * The working info about each filesystem in the resource group.
 * Contains or acquires all the information needed to determine
 * whether or not the FS needs to have its MDS reassigned, and
 * to mount the FS.
 */
struct FsInfo {
	char *fi_mntpt;		/* FS mount point */
	char *fi_fs;		/* FS family set name */
	char *fi_server;	/* FS metadata server (MDS) name */
	char *fi_pendsrv;	/* FS pending MDS (if !fi_server) */
	int   fi_deadmds;	/* FS MDS or pending MDS is dead */
	int   fi_pid;		/* pid of fork()ed child */
	int   fi_status;	/* exit status of child */
	int   fi_flags;		/* status of proc? */
	int   fi_dep;		/* any in-resource FS that must be mounted */
};

	/* fi_dep */
#define		FS_NO_DEP		-1

	/* Child for this FS was forked off */
#define		FS_FL_STARTED	0x1

	/* Child for this FS has exited */
#define		FS_FL_DONE		0x2

	/* Child for this FS has exited w/ error */
#define		FS_FL_ERR		0x4

	/* Another fs depends on this one */
#define		FS_FL_DEPDON	0x1000

	/* This fs depends on another */
#define		FS_FL_DEPDS		0x2000


/*
 * Error statuses (statii?) as returned by
 * ForkProcs/DepForkProcs/RevDepForkProcs
 */
#define		FORK_EEXIT		-1
#define		FORK_ESIGNAL	-2
#define		FORK_EFORK		-3

int instring(const char *, const char *);
int incasestring(const char *, const char *);

int get_serverinfo(char *, char **, char **, char **);
int set_server(char *, char *, int);

int svc_validate(struct RgInfo *, struct FsInfo *);
int mon_start(struct RgInfo *);
int mon_stop(struct RgInfo *);

void closefds(void);

int run_cmd(int pri, char *cmd);
int run_system(int pri, char *cmd);
int wait_for_stable_qfsstate(struct RgInfo *, char *fsname, char *oldmaster);
int wait_for_fence(struct RgInfo *, char *node);

boolean_t validate_mounted(char *, boolean_t);

int set_cluster_lease_mount_opts(struct RgInfo *, char *fs);

int mount_qfs(char *qfsfs, boolean_t retry_nomds);
int umount_qfs(struct RgInfo *, struct FsInfo *);

int CheckFsDevs(char *qfsfs);

int GetRgInfo(int argc, char **argv, struct RgInfo *rgp);
void RelRgInfo(struct RgInfo *rgp);

struct FsInfo *GetFileSystemInfo(struct RgInfo *rgp);
void FreeFileSystemInfo(struct FsInfo *fip);

int GetMdsInfo(struct RgInfo *rgp, struct FsInfo *fp);

int IsLeader(struct RgInfo *rgp);
int IsNodeUp(struct RgInfo *rgp, char *hostname, boolean_t *is_up);

int ForkProcs(int, char **, struct FsInfo *,
			int (*)(int, char **, struct FsInfo *));
int DepForkProcs(int, char **, struct FsInfo *,
			int (*)(int, char **, struct FsInfo *));
int RevDepForkProcs(int, char **, struct FsInfo *,
			int (*)(int, char **, struct FsInfo *));


/*
 * Global variable indicating whether or not to print errors on
 * stderr. Used by validate method.
 */
extern int doprint;

#define	dprintf	if (doprint) (void) fprintf /* LINTED */

#define	streq(x, y)		(strcmp((x), (y)) == 0)

#ifdef __cplusplus
}
#endif

#endif /* _SCQFS_H */
