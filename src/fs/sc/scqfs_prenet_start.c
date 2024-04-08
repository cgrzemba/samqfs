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
 * scqfs_prenet_start.c - Start method for SUNW.qfs.
 *
 * Issues the samsharefs command to take over Metadata
 * server from a remote cluster node.
 */

#pragma ident "$Revision: 1.27 $"

#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <libgen.h>
#include <stdlib.h>
#include <rgm/libdsdev.h>
#include <sys/statvfs.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>

#include "sam/nl_samfs.h"
#include "sam/custmsg.h"
#include "sam/exit.h"

#include "scqfs.h"

/* Prototypes */
extern void ChkFs(void);			/* src/fs/lib/mount.c */

static int switchOver(int, char **, struct FsInfo *);
static int failInvoluntary(struct RgInfo *, char *, char *, char *);
static int failVoluntary(char *, char *);

int
main(int argc, char *argv[])
{
	struct RgInfo  rg;
	struct FsInfo *fi;
	int unmounted = 0;
	int i, rc;

	program_name = basename(argv[0]);
	CustmsgInit(0, NULL);

	if (GetRgInfo(argc, argv, &rg) < 0) {
		return (1);
	}

	/*
	 * Make sure sam-fsd is running.
	 */
	ChkFs();

	if ((fi = GetFileSystemInfo(&rg)) == NULL) {
		return (1);
	}

	for (i = 0; fi[i].fi_mntpt != NULL; i++) {
		struct sam_fs_info fsi;

		if (GetFsInfo(fi[i].fi_fs, &fsi) < 0) {
			scds_syslog(LOG_ERR,
			    "Couldn't family set info for %s: %s",
			    fi[i].fi_fs, strerror(errno));
			unmounted++;
		}
	}

	if (unmounted) {
		return (1);
	}

	RelRgInfo(&rg);

	rc = ForkProcs(argc, argv, fi, switchOver);

	FreeFileSystemInfo(fi);

	return (rc);
}


/*
 * ----- static int checkInodesFile(struct FsInfo *fp)
 *
 * Use the ls command to check the existence of .inodes in the target
 * filesystem.
 *
 * Returns:
 *
 *
 *	-1		: Out of memory error in creating command string
 *	0		: Success
 *  non-zero: Failure of the ls command
 */
static int
checkInodesFile(struct FsInfo *fp)
{
	int		l, rc;
	char	cmd[MAXPATHLEN+64];

	scds_syslog_debug(DBG_LVL_HIGH, "checkInodesFile - Begin");

	l = snprintf(cmd, sizeof (cmd), "/bin/ls -l %s/.inodes", fp->fi_mntpt);
	if (l >= sizeof (cmd)) {
		scds_syslog(LOG_ERR, "%s: mount pathname too long", fp->fi_fs);
		scds_syslog_debug(DBG_LVL_HIGH,
		    "checkInodesFile - End/Err");
		return (1);
	}
	/* Suppress ls command output */
	closefds();

	rc = run_system(LOG_ERR, cmd);

	scds_syslog_debug(DBG_LVL_HIGH, "checkInodesFile - End");
	return (rc);
}


/*
 * Failover actions.
 */
#define		FX_NOSWITCH		0	/* Do not initiate failover */
#define		FX_VOLUNTARY	1	/* Initiate voluntary failover */
#define		FX_INVOLUNTARY	2	/* Initiate involuntary failover */

/*
 * switchOver()
 *
 * Ensure that the current host is the given FS's MDS.  If it
 * isn't, use appropriate means to fail it over.
 */
static int
switchOver(int argc, char **argv, struct FsInfo *fp)
{
	int				rc;
	boolean_t		status;
	int				mode;
	struct RgInfo	rg;
	char			*curmaster = NULL;
	struct sam_fs_info fsi;

	scds_syslog_debug(DBG_LVL_HIGH, "switchOver - Begin");

	if (!fp->fi_fs || !fp->fi_mntpt) {
		scds_syslog_debug(DBG_LVL_MED,
		    "Null file set or mount point name");
		scds_syslog_debug(DBG_LVL_HIGH,
		    "switchOver - End/Err");
		return (1);
	}

	if (GetRgInfo(argc, argv, &rg) < 0) {
		scds_syslog_debug(DBG_LVL_HIGH,
		    "switchOver - End/Err");
		return (1);		/* error logged by GetRgInfo */
	}

	if (GetFsInfo(fp->fi_fs, &fsi) < 0) {
		scds_syslog(LOG_ERR,
		    "%s: Couldn't get file system info",
		    fp->fi_fs, strerror(errno));
		scds_syslog_debug(DBG_LVL_HIGH,
		    "switchOver - End/Err");
		return (1);
	}

	/*
	 * Set FS failover lease recovery modes in the kernel,
	 * so that sam-sharefsd will find them and perform the
	 * appropriate kind.
	 */
	(void) set_cluster_lease_mount_opts(&rg, fp->fi_fs);

	for (;;) {
		if (curmaster) {
			free(curmaster);
			curmaster = NULL;
		}

		/* Get master */
		if ((rc =
		    get_serverinfo(fp->fi_fs, NULL, &curmaster, NULL)) < 0) {
			break; /* Errors logged by get_serverinfo() */
		}

		mode = FX_INVOLUNTARY;
		if (curmaster) {
			/*
			 * Old master exists.  If it's up, and the FS is in
			 * reasonable shape, try VOLUNTARY failover.
			 */
			if (IsNodeUp(&rg, curmaster, &status)) {
				/* 22655 */
				scds_syslog(LOG_ERR,
				    "%s: Error determining if %s is up.",
				    fp->fi_fs, curmaster);
				rc = 1;
				break;
			}

			/* If the server is down */
			if (status == B_FALSE) {
				/* MASTER DOWN - INVOLUNTARY */
				/* 22656 */
				scds_syslog(LOG_WARNING,
				    "%s: Metadata server %s is down.",
				    fp->fi_fs, curmaster);
				/* 22657 */
				scds_syslog(LOG_WARNING,
				    "%s: Switching to involuntary failover.",
				    fp->fi_fs);
			} else {
				/*
				 * The file system should already be mounted,
				 * The BOOT or INIT Method should have performed
				 * the mount, however, there has been some cases
				 * where the MDS was in failover so adding the
				 * check and mount here is to cover this
				 * situation.
				 */
				if (!(fsi.fi_status & FS_MOUNTED)) {
					scds_syslog(LOG_NOTICE,
					    "%s: Mounting file system ",
					    fp->fi_fs);
					rc = mount_qfs(fp->fi_fs, FALSE);
					if (rc == EXIT_NOMDS) {
						/* Switch to involuntary */
						scds_syslog(LOG_NOTICE,
						    "%s: MDS not mounted",
						    fp->fi_fs);
					} else if (rc != 0) {
						scds_syslog(LOG_ERR,
						    "%s: Error mounting %s.",
						    fp->fi_fs, fp->fi_mntpt);
						rc = 1;
						break;
					}
					if (GetFsInfo(fp->fi_fs, &fsi) < 0) {
						scds_syslog(LOG_ERR,
						    "%s: Couldn't get FS info",
						    fp->fi_fs, strerror(errno));
						rc = 1;
						break;
					}
				}

				if (fsi.fi_status & FS_MOUNTED) {
					/*
					 * If this node is the metadata server,
					 * verify we have access to the
					 * filesystem and return.
					 */
					if (strcasecmp(rg.rg_nodename,
					    curmaster) == 0) {
						/* 22651 */
						scds_syslog(LOG_NOTICE, "%s: "
						    "This node is the metadata"
						    " server.", fp->fi_fs);

						if (checkInodesFile(fp) == 0) {
							/* 22652 */
							scds_syslog(LOG_NOTICE,
							    "%s: The filesystem"
							    " is online.",
							    fp->fi_fs);
							rc = 0;
							break;
						}
					}
					mode = FX_VOLUNTARY;
				}
			}
		}

		if (mode == FX_INVOLUNTARY) {
			/* 22649 */
			scds_syslog(LOG_WARNING,
			    "%s: Metadata server not found.", fp->fi_fs);
			/* 22650 */
			scds_syslog(LOG_WARNING,
			    "%s: Using involuntary failover.", fp->fi_fs);

			rc = failInvoluntary(&rg, fp->fi_fs, rg.rg_nodename,
			    curmaster);
			if (rc != 0) {
				break;
			}
			/*
			 * If the file system isn't mounted, it's ok, we
			 * mount it here.
			 */
			if ((fsi.fi_status & FS_MOUNTED) == 0) {
				scds_syslog(LOG_NOTICE,
				    "%s: Mounting file system ",
				    fp->fi_fs);
				if ((rc = mount_qfs(fp->fi_fs, TRUE)) != 0) {
					scds_syslog(LOG_ERR,
					    "%s: Error mounting %s.",
					    fp->fi_fs, fp->fi_mntpt);
					break;
				}
				if (GetFsInfo(fp->fi_fs, &fsi) < 0) {
					scds_syslog(LOG_ERR,
					    "%s: Couldn't get FS info",
					    fp->fi_fs, strerror(errno));
					rc = 1;
					break;
				}
			}
		} else {
			/* 22653 */
			scds_syslog(LOG_NOTICE,
			    "%s: Metadata server %s found.",
			    fp->fi_fs, curmaster);
			/* 22654 */
			scds_syslog(LOG_NOTICE,
			    "%s: Attempting voluntary failover.", fp->fi_fs);

			rc = failVoluntary(fp->fi_fs, rg.rg_nodename);
			if (rc != 0) {
				/* 22677 */
				scds_syslog(LOG_ERR,
				    "%s: Metadata server is not responding.",
				    fp->fi_fs);
				mode = FX_INVOLUNTARY;
			}
		}
		scds_syslog_debug(DBG_LVL_MED,
		    "Sleep 2 seconds then check qfs state");
		(void) sleep(2);

		/* Wait for stable state */
		rc = wait_for_stable_qfsstate(&rg, fp->fi_fs, curmaster);

		if (rc < 0) {
			/* 22665 */
			scds_syslog(LOG_ERR,
			    "%s: Error getting QFS state %d.", fp->fi_fs, rc);
			break;
		}
	}

	if (curmaster) {
		free(curmaster);
	}

	scds_syslog_debug(DBG_LVL_HIGH, "switchOver - End rc = %d", rc);
	return (rc);
}


int
failVoluntary(char *qfsdev, char *newmaster)
{
	int rc;

	scds_syslog_debug(DBG_LVL_HIGH, "failVoluntary - Begin");
	/* Set new master */
	if ((rc = set_server(qfsdev, newmaster, FSD)) != 0) {
		/* 22664 */
		scds_syslog(LOG_ERR,
		    "%s: Error failing over to new server (%s): %s.",
		    qfsdev, newmaster, strerror(rc));
		scds_syslog_debug(DBG_LVL_HIGH,
		    "failVoluntary - End/Err");
		return (rc);
	}

	scds_syslog_debug(DBG_LVL_HIGH, "failVoluntary - End");
	return (0);
}


int
failInvoluntary(struct RgInfo *rgp,
				char *qfsdev,
				char *newmaster,
				char *oldmaster)
{
	int		rc;
	int		used_prevsrv = 0;

	scds_syslog_debug(DBG_LVL_HIGH, "failInvoluntary - Begin");

	/*
	 * If we don't have an oldmaster and this is an involuntary
	 * failure, the RGM is in control. Data fencing has already
	 * completed and the old metadata server is long ago forgotten
	 */
	if (oldmaster == NULL) {
		rc = get_serverinfo(qfsdev, &oldmaster, NULL, NULL);
		if (rc < 0 && oldmaster) {
			/* 22680 */
			scds_syslog(LOG_NOTICE,
			    "%s: Involuntary failover from no server.",
			    qfsdev);
			/* 22681 */
			scds_syslog(LOG_NOTICE, "%s: Using previous "
			    "metadata server %s for fencing check.",
			    qfsdev, oldmaster);

			/*
			 * Save the fact that oldmaster was derived
			 * from prevsrv
			 */
			used_prevsrv++;
		}
	}

	if (oldmaster) {
		if (!used_prevsrv) {
			/* If we used prevsrv, no server is already set */
			/* Set metadata server to NO_SERVER */
			/* 22658 */
			scds_syslog(LOG_NOTICE,
			    "%s: Clearing the metadata server.",
			    qfsdev);
			if ((rc = set_server(qfsdev, NO_SERVER, RAW)) != 0) {
				/* 22659 */
				scds_syslog(LOG_ERR, "%s: Error clearing ",
				    "the metadata server, err=%s.",
				    qfsdev, strerror(rc));
				scds_syslog_debug(DBG_LVL_HIGH,
				    "failInvoluntary - End/Err rc = %d", rc);
				return (rc);
			}
		}

		/* Wait for cluster to tell us fencing is complete */
		/* 22660 */
		scds_syslog(LOG_NOTICE,
		    "%s: Checking cluster for fencing of node %s.",
		    qfsdev, oldmaster);

		if ((rc = wait_for_fence(rgp, oldmaster)) != 0) {
			/* 22661 */
			scds_syslog(LOG_ERR, "%s: Error waiting for fencing.",
			    qfsdev);
			scds_syslog_debug(DBG_LVL_HIGH,
			    "failInvoluntary - End/Err rc = %d", rc);
			return (rc);
		}
	}

	/* Set new master */
	if ((rc = set_server(qfsdev, newmaster, RAW)) != 0) {
		/* 22664 */
		scds_syslog(LOG_ERR,
		    "%s: Error failing over to new server (%s): %s",
		    qfsdev, newmaster, strerror(rc));
		scds_syslog_debug(DBG_LVL_HIGH,
		    "failInvoluntary - End/Err rc = %d", rc);
		return (rc);
	}

	scds_syslog_debug(DBG_LVL_HIGH, "failInvoluntary - End");
	return (0);
}
