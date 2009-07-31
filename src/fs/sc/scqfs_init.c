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
 * scqfs_init.c - BOOT/INIT method for SUNW.qfs
 *
 * Called when the Metadata Server resource (of type SUNW.qfs) is
 * created for the very first time.  Mounts the specified mountpoint.
 * This happens on ALL nodes of the Cluster where the QFS resource
 * can fail over (i.e. the Nodelist of the Resource Group which
 * contains the Metadata server resource).
 *
 * This is also the BOOT method implementation (scqfs_boot is a
 * symlink to scqfs_init).
 */

#pragma ident "$Revision: 1.20 $"

#include <strings.h>
#include <unistd.h>

#include <libgen.h>
#include <rgm/libdsdev.h>

#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/shareops.h"
#include "sam/custmsg.h"

#include "scqfs.h"

/*
 * Interval (in seconds) between checks for device availability.
 */
#define	DEVICE_WAIT_INTERVAL	15

/* Prototypes */
extern void ChkFs(void);		/* src/fs/lib/mount.c */

static int ProcFS(int ac, char **av, struct FsInfo *fp);
static int GetBootTimeout(struct RgInfo *rgp, int *boot_timeout);


int
main(
	int argc,
	char *argv[])
{
	struct RgInfo	rg;
	struct FsInfo	*fi;
	int				rc;

	/*
	 * Process arguments passed by RGM and initialize syslog.
	 */
	program_name = basename(argv[0]);
	CustmsgInit(0, NULL);

	if (GetRgInfo(argc, argv, &rg) < 0) {
		return (1);
	}

	/*
	 * For the get_qfsdev() calls below, sam-fsd must be up.
	 * Make sure it's running.
	 */
	ChkFs();

	/*
	 * Allocate space for the FsInfo array and clear it.
	 */
	if ((fi = GetFileSystemInfo(&rg)) == NULL) {
		return (1);
	}

	/*
	 * Close the open ds/cluster/rg handles we have open
	 * prior to forking off children to handle the FSes.
	 * These have RPC handles and other state in them
	 * that shouldn't be shared about across a fork().
	 */
	RelRgInfo(&rg);

	/*
	 * We've got the FS names, their mount points, and
	 * the list of nodes that can serve as MDSes.  Now
	 * we need to fork/thread off procs for each FS.
	 * Each thread needs to read up its FS's hosts file,
	 * determine if the FS's server is available (if this
	 * proc is the leader), wait for fencing (if this...),
	 * and then mount the FS (if it's not mounted).  Then
	 * we're done.
	 */
	rc = DepForkProcs(argc, argv, fi, ProcFS);

	FreeFileSystemInfo(fi);

	return (rc);
}


/*
 * ProcFS -- verify MDS & mount status of an FS
 *
 * Check out the FS indicated by 'fp'.  If 'leader' is set,
 * this node is responsible for ensuring that the FS's
 * metadata server is UP.  (Or if a failover is pending,
 * that both the MDSes involved are UP.)
 *
 * If they're not, then an involuntary failover must be
 * forced.
 *
 * And, whether 'leader' is set or not, the FS must be
 * mounted before we finish.  (It's OK if it's already
 * mounted; if it's not, we must mount it.)
 */
static int
ProcFS(
	int argc,
	char **argv,
	struct FsInfo *fp)
{
	struct RgInfo rg;
	int rc, leader;
	int fenced = FALSE;
	int waittime;
	int boot_timeout;

	scds_syslog_debug(DBG_LVL_HIGH, "ProcFS - Begin");

	if (!fp->fi_fs || !fp->fi_mntpt) {
		scds_syslog_debug(DBG_LVL_MED,
		    "Null file set or mount point name");
		scds_syslog_debug(DBG_LVL_HIGH,
		    "ProcFS - End/Err");
		return (1);
	}

	if (GetRgInfo(argc, argv, &rg) < 0) {
		scds_syslog_debug(DBG_LVL_HIGH,
		    "ProcFS - End/Err");
		return (1);		/* errors logged by GetRgInfo */
	}

	if (GetBootTimeout(&rg, &boot_timeout) < 0) {
		scds_syslog_debug(DBG_LVL_HIGH,
		    "ProcFS - End/Err");
		return (1);		/* errors logged by GetBootTimeout */
	}

	/*
	 *  Loop checking devices but don't loop forever.  Return an
	 *  error prior to hitting the boot timeout to avoid the RGM
	 *  sending us a kill (and dumping core).  The kill is ugly
	 *  and the devices may have been administratively disabled.
	 */
	waittime = boot_timeout - (DEVICE_WAIT_INTERVAL * 2);
	scds_syslog_debug(DBG_LVL_HIGH,
	    "ProcFS - Device check boot_timeout=%d, waittime=%d.",
	    boot_timeout, waittime);
	while (CheckFsDevs(fp->fi_fs) < 0) {
		if (waittime <= 0) {
			scds_syslog(LOG_ERR,
			    "FS %s: devices not available; exiting.",
			    fp->fi_fs);
			scds_syslog_debug(DBG_LVL_HIGH,
			    "ProcFS - Device unavailable; End/Err.");
			return (1);
		}
		scds_syslog(LOG_NOTICE,
		    "FS %s: devices not available; waiting.",
		    fp->fi_fs);
		scds_syslog_debug(DBG_LVL_MED,
		    "ProcFS - Device unavailable/sleeping for %d seconds",
		    DEVICE_WAIT_INTERVAL);
		(void) sleep(DEVICE_WAIT_INTERVAL);
		waittime -= DEVICE_WAIT_INTERVAL;
	}

	/*
	 * Find out whether or not the FS's current server
	 * (and pending server if that's different) is up.
	 * If the server (or pending server) is down, record
	 * the fact.
	 */

restart:
	for (;;) {
		if (GetMdsInfo(&rg, fp) < 0) {
			scds_syslog_debug(DBG_LVL_HIGH,
			    "ProcFS - End/Err");
			return (1);		/* error logged by GetMdsInfo */
		}

		if (fp->fi_deadmds && !fenced) {
			rc = wait_for_fence(&rg, fp->fi_server);
			if (rc < 0) {
				scds_syslog(LOG_ERR,
				    "%s: Error waiting for fencing.",
				    fp->fi_fs);
				scds_syslog_debug(DBG_LVL_HIGH,
				    "ProcFS - End/Err");
				return (1);
			}
			if (rc == 0) {
				fenced = TRUE;
			}
			continue;
		}

		leader = IsLeader(&rg);

		/*
		 * If we're the leader and the MDS is dead, then force
		 * an involuntary failover to this node.
		 */
		if (!leader || !fp->fi_deadmds ||
		    (rc = set_server(fp->fi_fs, rg.rg_nodename, RAW)) == 0) {
			break;
		}

		/* 22664 */
		scds_syslog(LOG_ERR,
		    "%s: Error failing over to new server (%s): %s",
		    fp->fi_fs, rg.rg_nodename, strerror(rc));
		scds_syslog_debug(DBG_LVL_HIGH,
		    "ProcFS - End/Err");
		return (1);
	}

	/*
	 * Set appropriate lease recovery mount options.  These are
	 * set in the defaults area so they persist when unmounted.
	 */
	(void) set_cluster_lease_mount_opts(&rg, fp->fi_fs);

	/*
	 * Issue mount request if the FS isn't mounted
	 */
	rc = 0;
	if (!validate_mounted(fp->fi_mntpt, B_FALSE)) {
		if (!leader && fp->fi_deadmds) {
			int count = 1;
			/*
			 * Wait for new server to take over.
			 */
			while (fp->fi_deadmds) {
				if (GetMdsInfo(&rg, fp) < 0) {
					scds_syslog_debug(DBG_LVL_HIGH,
					    "ProcFS - End/Err");
					return (1);
				}
				if (fp->fi_deadmds) {
					if (++count > 15) {
						/* leader left? */
						scds_syslog_debug(DBG_LVL_MED,
						    "Server wait timeout");
						goto restart;
					}
					(void) sleep(2);
				}
			}
			/*
			 * MDS changed or the old MDS rejoined the cluster.
			 * Notify sam-sharefsd that the MDS may have changed,
			 * wait 2 seconds for it to notice, and then start
			 * the mount.
			 */
			if (sam_shareops(fp->fi_fs,
			    SHARE_OP_WAKE_SHAREDAEMON, 0) < 0) {
				/* 13749 */
				scds_syslog(LOG_ERR, "%s:  "
			    "sam_shareops(%s, SHARE_OP_WAKE_SHAREDAEMON, 0)"
				    " failed",
				    fp->fi_fs, fp->fi_fs);
			}
			(void) sleep(2);
		}
		if ((rc = mount_qfs(fp->fi_fs)) != 0) {
			/* 22685 */
			scds_syslog(LOG_ERR,
			    "%s: Error while attempting to mount %s.",
			    fp->fi_fs, fp->fi_mntpt);
		}
	}
	scds_syslog_debug(DBG_LVL_HIGH, "ProcFS - End");
	return (rc == 0 ? 0 : 1);
}


/*
 * GetBootTimeout -- Get the SC configured boot method timeout
 *	for this resource.
 */
static int
GetBootTimeout(struct RgInfo *rgp, int *boot_timeout)
{
	char *resource_name;
	scha_resource_t handle;
	scha_err_t e;

	/*  Open the resource */
	resource_name = (char *)scds_get_resource_name(rgp->rg_scdsh);
	if (resource_name == NULL) {
		scds_syslog(LOG_ERR, "ProcFS: Failed to get resource name.");
		scds_syslog_debug(DBG_LVL_HIGH,
		    "ProcFS Failed to get resource name - End/Err");
		return (-1);
	}
	e = scha_resource_open(resource_name, rgp->rg_name, &handle);
	if (e != SCHA_ERR_NOERR) {
		scds_syslog(LOG_ERR, "ProcFS: Failed to open resource %s: %s.",
		    resource_name, scds_error_string(e));
		scds_syslog_debug(DBG_LVL_HIGH,
		    "ProcFS Failed to open resource %s - End/Err",
		    resource_name);
		return (-1);
	}
	e = scha_resource_get(handle, SCHA_BOOT_TIMEOUT, boot_timeout);
	if (e != SCHA_ERR_NOERR) {
		scds_syslog(LOG_ERR,
		    "ProcFS: Failed to retrieve the property %s: %s.",
		    SCHA_BOOT_TIMEOUT, scds_error_string(e));
		scds_syslog_debug(DBG_LVL_HIGH,
		    "ProcFS Failed to get property %s - End/Err",
		    SCHA_BOOT_TIMEOUT);
		return (-1);
	}
	e = scha_resource_close(handle);
	if (e != SCHA_ERR_NOERR) {
		scds_syslog(LOG_ERR, "ProcFS: Failed to close resource %s: %s.",
		    resource_name, scds_error_string(e));
		scds_syslog_debug(DBG_LVL_HIGH,
		    "ProcFS Failed resource close %s - End/Err",
		    resource_name);
		return (-1);
	}
	return (0);
}
