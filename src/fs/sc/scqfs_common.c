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

/*
 * scqfs_common.c - Common routines for SUNW.qfs RT.
 */

#pragma ident "$Revision: 1.38 $"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/vfstab.h>
#include <sys/mnttab.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "sam/shareops.h"
#include "sam/exit.h"

#include "samhost.h"

#include "scqfs.h"

/* By default disable message output to STDERR */
int doprint = 0;

/*
 * man page to the contrary, this routine is not
 * declared in string.h.
 */
extern char *strtok_r(char *, const char *, char **);

static sam_host_table_blk_t *htb = NULL;
static int htbufsize = 0;

extern char *getfullrawname();

/*
 * Get all the ds/cluster/rg information we need to handle
 * the resource group, and save it in the given rgp structure.
 *
 * Return -1 on error.
 */
int
GetRgInfo(int ac, char **av, struct RgInfo *rgp)
{
	scha_extprop_value_t   *xprop = NULL;
	scha_extprop_value_t   *mprop;
	scha_err_t				e;

	bzero((char *)rgp, sizeof (*rgp));

	if (scds_initialize(&rgp->rg_scdsh, ac, av) != SCHA_ERR_NOERR) {
		return (-1);
	}

	/*
	 * Get Cluster handle.
	 */
	e = scha_cluster_open(&rgp->rg_cl);
	if (e != SCHA_ERR_NOERR) {
		/* 22644 */
		scds_syslog(LOG_ERR,
		    "Unable to get cluster handle: %s.", scds_error_string(e));
		return (-1);
	}

	/*
	 * Get nodename of this node.
	 */
	e = scha_cluster_get(rgp->rg_cl, SCHA_NODENAME_LOCAL,
	    &rgp->rg_nodename);
	if (e != SCHA_ERR_NOERR) {
		/* 22645 */
		scds_syslog(LOG_ERR, "Unable to get local node name: %s.",
		    scds_error_string(e));
		return (-1);
	}

	/*
	 * Get the name of the resource group we're handling.
	 */
	rgp->rg_name = scds_get_resource_group_name(rgp->rg_scdsh);
	if (!rgp->rg_name) {
		/* 22688 */
		scds_syslog(LOG_ERR, "Unable to get resource group name.");
		return (-1);
	}

retry:
	e = scha_resourcegroup_open(rgp->rg_name, &rgp->rg_rgh);
	if (e != SCHA_ERR_NOERR) {
		/* 22697 */
		scds_syslog(LOG_ERR, "Unable to open resource group '%s': %s.",
		    rgp->rg_name, scds_error_string(e));
		return (-1);
	}

	/*
	 * Get the list of nodes that can host the resource.
	 */
	e = scha_resourcegroup_get(rgp->rg_rgh, SCHA_NODELIST,
	    &rgp->rg_nodes);
	if (e != SCHA_ERR_NOERR || !rgp->rg_nodes ||
	    !rgp->rg_nodes->array_cnt) {
		if (e == SCHA_ERR_SEQID) {
			/* 22698 */
			scds_syslog(LOG_INFO,
			    "Retrying resource group get (%s): %s.",
			    rgp->rg_name, scds_error_string(e));
			(void) scha_resourcegroup_close(rgp->rg_rgh);
			rgp->rg_rgh = NULL;
			(void) sleep(1);
			goto retry;
		}
		/* 22689 */
		scds_syslog(LOG_ERR,
		    "Unable to get nodelist for resource group %s.",
		    rgp->rg_name);
		return (-1);
	}

	/*
	 * Get the list of filesystems in the resource group.
	 */
	e = scds_get_ext_property(rgp->rg_scdsh,
	    QFSFS, SCHA_PTYPE_STRINGARRAY, &xprop);
	if (e != SCHA_ERR_NOERR ||
	    !xprop ||
	    !xprop->val.val_strarray ||
	    !xprop->val.val_strarray->array_cnt) {
		/* 22600 */
		scds_syslog(LOG_ERR,
		    "Failed to retrieve the property %s: %s.",
		    QFSFS, scds_error_string(e));
		return (-1);
	}
	rgp->rg_mntpts = xprop->val.val_strarray;

	/*
	 * Get the wait-for-clients-to-unmount delay for the
	 * FS's server.
	 */
	rgp->rg_umnt_wait = 30;	/* Old, unprogrammable default */
	mprop = NULL;
	e = scds_get_ext_property(rgp->rg_scdsh,
	    QFS_UMNT_WAIT, SCHA_PTYPE_INT, &mprop);
	if (e != SCHA_ERR_NOERR || !mprop) {
		/* 22600 */
		scds_syslog(LOG_ERR,
		    "Failed to retrieve the property %s: %s.",
		    QFS_UMNT_WAIT, scds_error_string(e));
	} else {
		rgp->rg_umnt_wait = mprop->val.val_int;
		scds_free_ext_property(mprop);
	}

	/*
	 * Get the lease-recovery-policy, and save that in the
	 * RgInfo structure.  If missing, assume "Auto".  Other
	 * values are Timeout (always wait for all hosts to connect,
	 * up to default timeout), and Cluster (always accelerate
	 * failover if all hosts not part of the cluster reconfig
	 * are "down" according to SunCluster).
	 */
	rgp->rg_flags = RG_LEASEPOLICY_AUTO;
	mprop = NULL;
	e = scds_get_ext_property(rgp->rg_scdsh,
	    QFS_LEASE_POLICY, SCHA_PTYPE_ENUM, &mprop);
	if (e == SCHA_ERR_NOERR && mprop) {
		if (strcmp(mprop->val.val_enum, "Timeout") == 0) {
			rgp->rg_flags = RG_LEASEPOLICY_TIMEOUT;
		} else if (strcmp(mprop->val.val_enum, "Cluster") == 0) {
			rgp->rg_flags = RG_LEASEPOLICY_CLUSTER;
		} else if (strcmp(mprop->val.val_enum, "Auto") != 0) {
			scds_syslog(LOG_ERR,
			    "Unrecognized value '%s' for property %s: %s.",
			    mprop->val.val_enum, QFS_LEASE_POLICY);
		}
		scds_free_ext_property(mprop);
	}

	return (0);
}


/*
 * Close all the ds/cluster/rg handles and zero out
 * the associated info structure.
 */
void
RelRgInfo(struct RgInfo *rgp)
{
	scha_err_t e;

	if ((e = scha_resourcegroup_close(rgp->rg_rgh)) != SCHA_ERR_NOERR) {
		scds_syslog(LOG_ERR,
		    "RG %s: scha_resourcegroup_close() failed: %s.",
		    rgp->rg_name, scds_error_string(e));
	}
	if ((e = scha_cluster_close(rgp->rg_cl)) != SCHA_ERR_NOERR) {
		scds_syslog(LOG_ERR,
		    "RG %s: scha_cluster_close() failed: %s.",
		    rgp->rg_name, scds_error_string(e));
	}
	scds_close(&rgp->rg_scdsh);

	bzero((char *)rgp, sizeof (*rgp));
}


/*
 * IsLeader(rgp)
 *
 * Return TRUE if 'node' is the first node in the rgnodes list that's UP.
 */
int
IsLeader(struct RgInfo *rgp)
{
	int i, leader = FALSE;

	for (i = 0; i < rgp->rg_nodes->array_cnt; i++) {
		scha_node_state_t	nstate;
		scha_err_t			e;

		nstate = SCHA_NODE_DOWN;
		e = scha_cluster_get(rgp->rg_cl,
		    SCHA_NODESTATE_NODE, rgp->rg_nodes->str_array[i], &nstate);
		if (e == SCHA_ERR_NOERR) {
			if (nstate == SCHA_NODE_UP &&
			    strcasecmp(rgp->rg_nodes->str_array[i],
			    rgp->rg_nodename) == 0) {
				leader = TRUE;		/* Tag!  We're it. */
				break;
			}
		} else {
			/* 22600 */
			scds_syslog(LOG_ERR,
			    "Failed to retrieve the property %s: %s.",
			    QFSFS, scds_error_string(e));
		}
	}

	return (leader);
}


/*
 * ----- int IsNodeUp(struct RgInfo *rgp, char *hostname, boolean_t *is_up)
 *
 * Is the named node part of cluster membership?
 * Return value 0 if all went ok, -1 on error.
 * If 0 is returned, the passed pointer is set true/false.
 */
int
IsNodeUp(
	struct RgInfo *rgp,
	char *hostname,
	boolean_t *is_up)
{
	scha_err_t		e;
	scha_node_state_t	nstate;

	/* Find out the membership status of the node */
	e = scha_cluster_get(rgp->rg_cl, SCHA_NODESTATE_NODE, hostname,
	    &nstate);
	if (e != SCHA_ERR_NOERR) {
		/* 22628 */
		scds_syslog(LOG_ERR,
		    "Failed to get nodeid for node <%s>: %s.",
		    hostname, scds_error_string(e));
		return (-1);
	}

	if (nstate == SCHA_NODE_UP) {
		*is_up = B_TRUE;
	} else {
		*is_up = B_FALSE;
	}

	return (0);
}


/*
 * FreeFileSystemInfo(struct FsInfo *)
 *
 * Free up a struct FsInfo array allocated by GetFileSystemInfo
 */
void
FreeFileSystemInfo(struct FsInfo *fip)
{
	int i;

	for (i = 0; fip[i].fi_mntpt != NULL; i++) {
		free((void *)fip[i].fi_mntpt);
		if (fip[i].fi_fs != NULL) {
			free((void *)fip[i].fi_fs);
		}
	}
	free((void *)fip);
}


/*
 * ----- char *get_qfsdev(char *qfsfs)
 *
 * Returns the QFS device (family set) name (qfs1 etc..)
 * by looking in /etc/vfstab for the filesystem mount point
 * specified via the QFSFileSystem Extension property.
 *
 * Returned memory is allocated by the function and belongs to
 * the caller which should free() it.
 *
 * Returns NULL if errors are encountered.
 */
static char *
get_qfsdev(char *qfsfs)
{
	char *qfsdev = NULL;
	int found = FALSE;
	struct vfstab vp;
	FILE *fp;

	/* Check for this entry in /etc/vfstab */
	fp = fopen(VFSTAB, "r");
	if (fp == NULL) {
		/* 22603 */
		scds_syslog(LOG_ERR,
		    "Unable to open /etc/vfstab: %s",
		    strerror(errno));
		dprintf(stderr, catgets(catfd, SET, 22603,
		    "Unable to open /etc/vfstab: %s."),
		    strerror(errno));
		goto finished;
	}

	while (getvfsent(fp, &vp) == 0) {
		if (vp.vfs_mountp == NULL)
			continue;
		if (streq(qfsfs, vp.vfs_mountp)) {
			int option_err = FALSE;

			found = TRUE;

			/* Found the entry, make sure it looks good to us */

			/* Test 1: Should be of type samfs */
			if (!streq(vp.vfs_fstype, "samfs")) {
				/* 22604 */
				scds_syslog(LOG_ERR,
				    "Mount point %s is of fstype %s, should be "
				    "samfs.", qfsfs, vp.vfs_fstype);
				dprintf(stderr, catgets(catfd, SET, 22604,
				    "Mount point %s is of fstype %s, should be "
				    "samfs."), qfsfs, vp.vfs_fstype);
				goto finished;
			}

			/* Test 2: Should have mount-at-boot = no */
			if (!streq(vp.vfs_automnt, "no")) {
				/* 22605 */
				scds_syslog(LOG_ERR,
				    "Mount point %s should have mount-at-boot "
				    "set to 'no'.", qfsfs);
				dprintf(stderr, catgets(catfd, SET, 22605,
				    "Mount point %s should have mount-at-boot "
				    "set to 'no'."), qfsfs);
				goto finished;
			}

			/* Test 3: Should have shared and bg options */
			if (instring("shared", vp.vfs_mntopts) == 0) {
				option_err = TRUE;
				/* 22606 */
				scds_syslog(LOG_ERR,
				    "Mount point %s does not have the '%s' "
				    "option set.", qfsfs, "shared");
				dprintf(stderr, catgets(catfd, SET, 22606,
				    "Mount point %s does not have the '%s' "
				    "option set."), qfsfs, "shared");
			}

			if (instring("bg", vp.vfs_mntopts) == 1) {
				option_err = TRUE;
				/* 22693 */
				scds_syslog(LOG_ERR,
				    "Mount point %s should not have the '%s' "
				    "option set.", qfsfs, "bg");
				dprintf(stderr, catgets(catfd, SET, 22693,
				    "Mount point %s should not have the '%s' "
				    "option set."), qfsfs, "bg");
			}

			if (option_err) {
				goto finished;
			}

			/* vfstab entry looks good.  We're done. */
			break;
		}
	}

	if (!found) {
		/* 22607 */
		scds_syslog(LOG_ERR,
		    "Mount point %s is not present in /etc/vfstab.", qfsfs);
		dprintf(stderr, catgets(catfd, SET, 22607,
		    "Mount point %s is not present in /etc/vfstab."), qfsfs);
		goto finished;
	}

	/* All looks good */
	qfsdev = strdup(vp.vfs_special);
	if (qfsdev == NULL) {
		/* 22602 */
		scds_syslog(LOG_ERR, "Out of memory");
		dprintf(stderr, catgets(catfd, SET, 22602, "Out of memory"));
		goto finished;
	}

	scds_syslog_debug(1, "QFSDEV is <%s>.", qfsdev);

finished:

	return (qfsdev);
}


/*
 * ----- mountcompare(struct FsInfo *, struct FsInfo *)
 *
 * Compare the mount points of two FsInfo structures, and
 * return the alphabetical ordering.  This has the side-effect
 * of placing shorter mount points before longer ones, making
 * it the first part of determining mount-order dependencies.
 * Used by qsort.
 *
 * Describing the topological sort required for handling
 * mount and unmount dependencies takes more words than the
 * code.  Note that any filesystem in the resource group
 * can only have one other FS that must be mounted immediately
 * prior to its mount.  (Note that there may be a chain of
 * these, however.)
 *
 * For unmounts, however, it may be that several FSes in the
 * resource group may have to be unmounted before an FS may
 * be unmounted.
 *
 * So, mounting one FS may allow one to mount several other
 * FSes, but unmounting an FS will only allow one to unmount
 * at most one additional filesystem.
 *
 * To track all this, we keep fi_dep, which contains the
 * index of the FS with the longest-prefix-plus-slash name
 * that is a substring of the FS's mount point.
 */
static int
mountcompare(const void *p1, const void *p2)
{
	struct FsInfo *fp1 = (struct FsInfo *)p1;
	struct FsInfo *fp2 = (struct FsInfo *)p2;

	return (strcmp(fp1->fi_mntpt, fp2->fi_mntpt));
}


/*
 * depends_on(char *mp1, char *mp2)
 *
 * If mount path mp1 requires that mount path mp2 be mounted
 * first, then return true.  Else return false.
 *
 * I.e., return TRUE iff mp2 is a prefix of mp1, and the
 * first character in mp1 that is not in mp2 is a '/'.
 */
static int
depends_on(char *mp1, char *mp2)
{
	int len = strlen(mp2);

	if (strncmp(mp1, mp2, len) || mp1[len] != '/') {
		return (FALSE);
	}
	return (TRUE);
}


/*
 * GetFileSystemInfo(struct RgInfo *rgp)
 *
 * Allocate an array of FsInfo structures, one for each file system
 * in the resource group.  Set each file system's mount point and its
 * family set name into an FsInfo structure.  Leave the rest zero.
 * Add an extra entry to the end with a NULL fi_mntpnt pointer.
 *
 * NULL return on error.
 */
struct FsInfo *
GetFileSystemInfo(struct RgInfo *rgp)
{
	int i, j, fscount;
	struct FsInfo *fip;

	fscount = rgp->rg_mntpts->array_cnt;
	fip = (struct FsInfo *)malloc((fscount + 1) * sizeof (*fip));
	if (fip == NULL) {
		/* 22602 */
		scds_syslog(LOG_ERR, "Out of Memory");
		return (NULL);
	}
	bzero((char *)fip, (fscount + 1) * sizeof (*fip));

	/*
	 * Initialize the FsInfo array with the FS mount points
	 * and "specials" (AKA "devices" AKA "family set names").
	 */
	for (i = 0; i < fscount; i++) {
		fip[i].fi_mntpt = strdup(rgp->rg_mntpts->str_array[i]);
		if (fip[i].fi_mntpt == NULL) {
			/* 22602 */
			scds_syslog(LOG_ERR, "Out of Memory");
			goto err;
		}
		fip[i].fi_fs = get_qfsdev(fip[i].fi_mntpt);
		if (fip[i].fi_fs == NULL) {
			/* 22690 */
			scds_syslog(LOG_ERR,
			    "Can't find FS name for mount point %s.",
			    fip->fi_mntpt);
			goto err;
		}
	}
	/*
	 * Sort filesystems by mount point, and determine
	 * dependencies.  Find the nearest and longest previous
	 * FS whose mount point is a prefix of the FS, if any.
	 *
	 * After these steps are done, any FS with a dependency
	 * depends on an FS earlier in the array.  We mark both the
	 * FS with the dependency and the FS depended on.
	 */
	qsort((void *)fip, fscount, sizeof (*fip), mountcompare);

	fip[0].fi_dep = FS_NO_DEP;
/*
 *	dprintf(stderr, "%s: fip[%d] = %s : %d\n",
 *			rgp->rg_name, 0, fip[0].fi_mntpt, fip[0].fi_dep);
 */
	for (i = 1; i < fscount; i++) {
		fip[i].fi_dep = FS_NO_DEP;
		for (j = i-1; j >= 0; j--) {
			if (depends_on(fip[i].fi_mntpt, fip[j].fi_mntpt)) {
				fip[i].fi_dep = j;
				fip[i].fi_flags |= FS_FL_DEPDS;
				fip[j].fi_flags |= FS_FL_DEPDON;
				break;
			}
		}
/*
 *		dprintf(stderr, "%s: fip[%d] = %s : %d\n",
 *			rgp->rg_name, i, fip[i].fi_mntpt, fip[i].fi_dep);
 */
	}

	return (fip);

err:
	FreeFileSystemInfo(fip);
	return (NULL);
}


/*
 * ----- static int get_qfshosts(char *fsname, sam_host_table_blk_t *htp)
 *
 * return a copy of the shared hosts table to htp.
 * 0 on success, -1 on error.
 */
static int
get_qfshosts(char *fsname,
			sam_host_table_blk_t *htp,
			int htbufsize,
			int method)
{
	struct sam_fs_part pt;
	char *errstr;
	char *devrname;
	int rc, errnum;

	switch (method) {
	case FSD:
		/* Use the FS to retrieve the hosts table */
		rc = sam_gethost(fsname, htbufsize, (char *)htp);

		if (rc < 0) {
			if (errno == EXDEV) {
				/* 13754 */
				scds_syslog(LOG_ERR,
				    "%s:  Filesystem %s not mounted.",
				    program_name, fsname);
				dprintf(stderr, catgets(catfd, SET, 13754,
				    "%s:  Filesystem %s not mounted."),
				    program_name, fsname);
			}
			return (errno);
		}
		break;

	case RAW:
		/* Use the raw device to retrieve the hosts table */
		/* Get partition information */
		rc = GetFsParts(fsname, 1, &pt);
		if (rc < 0) {
			/* 22675 */
			scds_syslog(LOG_ERR,
			    "%s: Unable to get partition information: %s.",
			    fsname, strerror(errno));
			dprintf(stderr, catgets(catfd, SET, 22675,
			    "%s: Unable to get partition information: %s."),
			    fsname, strerror(errno));
			return (rc);
		}

		/* Get hosts table */
		if ((devrname = getfullrawname(pt.pt_name)) == NULL) {
			scds_syslog(LOG_ERR, "Out of Memory");
			return (ENOMEM);
		}
		rc = SamGetRawHosts(devrname, htp, htbufsize,
		    &errstr, &errnum);
		free(devrname);
		if (rc < 0) {
			/* 22671 */
			scds_syslog(LOG_ERR,
			    "%s: Unable to get QFS hosts table.  Error: %s",
			    fsname, errstr);
			dprintf(stderr, catgets(catfd, SET, 22671,
			    "%s: Unable to get QFS hosts table.  Error: %s"),
			    fsname, errstr);
			return (rc);
		}
	}

	/* Check version number */
	if (htp->info.ht.version != SAM_HOSTS_VERSION4) {
		/* 13728 */
		scds_syslog(LOG_ERR,
		    "%s: Unrecognized version in host file (%d).",
		    program_name, htp->info.ht.version);
		dprintf(stderr, catgets(catfd, SET, 13728,
		    "%s: Unrecognized version in host file (%d)."),
		    program_name, htp->info.ht.version);
		return (EINVAL);
	}

	/* Validate size of hosts table */
	if (htp->info.ht.length > SAM_HOSTS_TABLE_SIZE) {
		/* 13732 */
		scds_syslog(LOG_ERR,
		    "%s:  Host table length out of range (%d).",
		    program_name, htp->info.ht.length);
		dprintf(stderr, catgets(catfd, SET, 13732,
		    "%s:  Host table length out of range (%d)."),
		    program_name, htp->info.ht.length);
		return (EINVAL);
	}

	return (0);
}


/*
 * get_serverinfo(char *fs, char **prevsrv, char **server, char **pendsrv)
 *
 * Get the previous, current, and pending server names for FS 'fs'.
 * NULL parameters may be specified; their values will not be returned.
 * Non-NULL parameters will be set to point at freshly allocated strings
 * that should be free()d when no longer in use.
 *
 * Return 0 on success, -1 on failure.
 */
int
get_serverinfo(char *fs,
			char **prevsrv,
			char **server,
			char **pendsrv)
{
	upath_t host;
	int rc;

	if (htb == NULL) {
		htb =
		    (sam_host_table_blk_t *)malloc(SAM_LARGE_HOSTS_TABLE_SIZE);
		htbufsize = SAM_LARGE_HOSTS_TABLE_SIZE;
		bzero((char *)htb, htbufsize);
	}

	if ((rc = get_qfshosts(fs, htb, htbufsize, RAW)) < 0) {
		return (-1);		/* Errors logged by get_qfshosts */
	}

	if (prevsrv != NULL) {
		*prevsrv = NULL;
		if (htb->info.ht.prevsrv != HOSTS_NOSRV) {
			rc = SamGetSharedHostName(&htb->info.ht,
			    htb->info.ht.prevsrv, host);
			if (!rc) {
				/* 22694 */
				scds_syslog(LOG_ERR,
				    "FS %s: No previous master for QFS device.",
				    fs);
				return (-1);
			}
			*prevsrv = strdup(&host[0]);
		}
	}

	if (server != NULL) {
		*server = NULL;
		if (htb->info.ht.server != HOSTS_NOSRV) {
			rc = SamGetSharedHostName(&htb->info.ht,
			    htb->info.ht.server, host);
			if (!rc) {
				/* 22629 */
				scds_syslog(LOG_ERR,
				    "FS %s: No master for QFS device.", fs);
				return (-1);
			}
			*server = strdup(&host[0]);
		}
	}

	if (pendsrv != NULL) {
		*pendsrv = NULL;
		if (htb->info.ht.pendsrv != HOSTS_NOSRV) {
			rc = SamGetSharedHostName(&htb->info.ht,
			    htb->info.ht.pendsrv, host);
			if (!rc) {
				/* 22695 */
				scds_syslog(LOG_ERR,
				    "FS %s: No pending master for QFS device.",
				    fs);
				return (-1);
			}
			*pendsrv = strdup(&host[0]);
		}
	}

	return (0);
}


/*
 * GetMdsState - get the state of the FS's server(s) from
 * the cluster framework.
 *
 * Examine Filesystem 'fs'.  Determine its designated MDS;
 * and set fp->fi_deadmds according to whether the MDS is
 * up and healthy.  Tricky case: pending server.  If either
 * the old or the new server is down, deadmds is set.
 *
 * Probably should not set dead if only pending server
 * is dead.  Need to watch things until the failover
 * completes, however.
 */
static int
GetMdsState(scha_cluster_t cl,
			char *server,
			char *pendsrv,
			int *dead)
{
	scha_node_state_t nstate;
	scha_err_t e;

	*dead = FALSE;
	if (server == NULL || streq(server, NO_SERVER)) {
		*dead = TRUE;
		return (0);
	}
	if (pendsrv == NULL || streq(pendsrv, NO_SERVER)) {
		*dead = TRUE;
		return (0);
	}
	e = scha_cluster_get(cl, SCHA_NODESTATE_NODE, server, &nstate);
	if (e != SCHA_ERR_NOERR) {
		/* 22616 */
		scds_syslog(LOG_ERR,
		    "Failed to get state for node <%s>: %s.",
		    server, scds_error_string(e));
		return (-1);
	}
	if (nstate == SCHA_NODE_DOWN) {
		*dead = TRUE;
	} else if (!streq(server, pendsrv)) {
		/*
		 * A failover is pending on this FS.
		 *
		 * If the pending server is down, then the failover cannot
		 * complete and again we'll need to reassign the MDS.
		 */
		e = scha_cluster_get(cl, SCHA_NODESTATE_NODE, pendsrv, &nstate);
		if (e != SCHA_ERR_NOERR) {
			/* 22616 */
			scds_syslog(LOG_ERR,
			    "Failed to get state for node <%s>: %s.",
			    pendsrv, scds_error_string(e));
			return (-1);
		}
		if (nstate == SCHA_NODE_DOWN) {
			*dead = TRUE;
		}
	}

	return (0);
}


/*
 * GetMdsInfo(rgp, fp)
 *
 * Get the name of the current server from the on-disk shared
 * hosts file.  Call GetMdsState to determine if the MDS
 * is dead or not.  Updates fp->fi_server, fp->fi_pendsrv,
 * and fp->fi_deadmds.
 */
int
GetMdsInfo(struct RgInfo *rgp, struct FsInfo *fp)
{
	if (fp->fi_server) {
		free(fp->fi_server);
		fp->fi_server = NULL;
	}
	if (fp->fi_pendsrv) {
		free(fp->fi_pendsrv);
		fp->fi_pendsrv = NULL;
	}
	if (get_serverinfo(fp->fi_fs, NULL, &fp->fi_server, &fp->fi_pendsrv)) {
		/* 22691 */
		scds_syslog(LOG_ERR, "FS %s: Can't determine server.",
		    fp->fi_fs);
		return (-1);
	}

	if (GetMdsState(rgp->rg_cl,
	    fp->fi_server, fp->fi_pendsrv, &fp->fi_deadmds)) {
		/* 22692 */
		scds_syslog(LOG_ERR,
		    "FS %s: Can't get cluster state for server.", fp->fi_fs);
		return (-1);
	}
	return (0);
}


/*
 * CheckFsDevs - verify access to  all of fs's component devices
 */
int
CheckFsDevs(char *fs)		/* QFS family set name (not mount point) */
{
	int i, n, fd;
	struct sam_mount_info mp;
	char rdbuf[DEV_BSIZE];

	if (GetFsMount(fs, &mp) < 0) {
		return (-1);
	}

	for (i = 0; i < mp.params.fs_count; i++) {
		if ((fd = open(mp.part[i].pt_name, O_RDONLY|O_LARGEFILE)) < 0) {
			return (-1);
		}
		n = read(fd, rdbuf, DEV_BSIZE);
		if (n != DEV_BSIZE) {
			(void) close(fd);
			return (-1);
		}
		(void) close(fd);
	}
	return (0);
}


/*
 * Routines to handle fork()ing for agent component actions.
 *
 * Agent components may need to execute actions for each of
 * possibly several FSes.  These routines provide a convenient
 * mechanism.
 */

/*
 * fork_proc - call the given routine for each FS in fip
 * after forking.
 */
static int
fork_proc(int ac,
		char **av,
		struct FsInfo *fip,
		int (*fn)(int, char **, struct FsInfo *))
{
	pid_t pid;
	int rc;

	switch (pid = fork()) {
	case (pid_t)-1:			/* error */
		/* 22641 */
		scds_syslog(LOG_ERR,
		    "%s: Fork failed.  Error: %s",
		    fip->fi_mntpt, strerror(errno));
		return (-1);

	case 0:					/* child */
		rc = (*fn)(ac, av, fip);
		exit(rc);
		/*NOTREACHED*/

	default:				/* parent */
		fip->fi_pid = pid;
		fip->fi_flags |= FS_FL_STARTED;
	}
	return (0);
}


/*
 * ForkProcs
 *
 * For each filesystem in the resource group, we fork off a
 * child, and call the given procedure (in the child).  The
 * procedure should exit (not return), and this routine collects
 * all the exit statuses, and returns.  Any children that exit
 * with signals are noted.
 *
 * Return values:
 *	0 - all forked children exited w/ status 0.
 *  1 - one or more children exited w/ non-zero status.
 *	2 - one or more children exited w/ a signal-received status.
 *	3 - fork() failed
 */
int
ForkProcs(int ac,
		char **av,
		struct FsInfo *fip,
		int (*fn)(int, char **, struct FsInfo *))
{
	int nfs, i, rc;

	/*
	 * We've got the FS names, their mount points, and
	 * the list of nodes that can serve as MDSes.  Now
	 * we need to fork/thread off procs for each FS.
	 *
	 * Each forked copy calls (*fn)() with all our arguments.
	 * Once forked off, we wait around for all of them to
	 * return, noting discrepancies while we wait.
	 */
	for (nfs = 0; fip[nfs].fi_mntpt != NULL; ) {
		nfs++;
	}
	if (nfs == 1) {
		/*
		 * Don't bother forking if there's only one filesystem
		 */
		return (*fn)(ac, av, &fip[0]);
	}

	for (i = 0; i < nfs; i++) {
		if (fork_proc(ac, av, &fip[i], fn) < 0) {
			return (FORK_EFORK);	/* error logged by fork_proc */
		}
	}

	rc = 0;
	for (;;) {
		pid_t pid;
		int status, prc;

		pid = wait(&status);
		if (pid == (pid_t)-1) {
			if (errno == EAGAIN || errno == EINTR) {
				continue;
			}
			if (errno != ECHILD) {
				/* 22642 */
				scds_syslog(LOG_ERR,
				    "Wait for child returned unexpected "
				    "error: %s", strerror(errno));
			}
			break;
		}
		prc = 0;
		if (WIFSIGNALED(status)) {
			prc = FORK_ESIGNAL;
		} else if (WEXITSTATUS(status)) {
			prc = FORK_EEXIT;
		}
		rc = MIN(prc, rc);

		for (i = 0; i < nfs; i++) {
			if (pid == fip[i].fi_pid) {
				fip[i].fi_flags |= FS_FL_DONE;
				fip[i].fi_status = status;
				if (status != 0) {
					fip[i].fi_flags |= FS_FL_ERR;
				}

	/* N.B. Bad indentation here to meet cstyle requirements */

			if (prc == FORK_ESIGNAL) {
				/* 22685 */
				scds_syslog(LOG_ERR,
				    "%s: Error: exited w/ signal <%d>%s.",
				    fip[i].fi_fs, WTERMSIG(status),
				    WCOREDUMP(status) ? " -- core dumped" : "");
			}
				break;
			}
		}
		if (fip[i].fi_mntpt == NULL) {
			scds_syslog(LOG_ERR,
			    "Unknown child process returned "
			    "(pid %d, status=%#x)", pid, status);
		}
	}
	return (rc);
}


/*
 * ----- DepForkProcs(ac, av, fip, fn)
 *
 * Like ForkProcs, but we worry about any dependencies in the
 * list of FSes, delaying forks to set up FSes that require
 * other FSes' completion.
 *
 * Return values:
 *	0 - all forked children exited w/ status 0.
 *  1 - one or more children exited w/ non-zero status.
 *	2 - one or more children exited w/ a signal-received status.
 *	3 - fork() failed
 */
int
DepForkProcs(int ac,
			char **av,
			struct FsInfo *fip,
			int (*fn)(int, char **, struct FsInfo *))
{
	int nfs, i, rc;

	for (nfs = 0; fip[nfs].fi_mntpt != NULL; ) {
		nfs++;
	}
	if (nfs == 1) {
		/*
		 * Don't bother forking if there's only one filesystem
		 */
		return (*fn)(ac, av, &fip[0]);
	}

	/*
	 * We've got the FS names, their mount points, and
	 * the list of nodes that can serve as MDSes.  Now
	 * we need to fork off a proc for each FS.
	 *
	 * Each forked copy calls (*fn)() with all our arguments.
	 * Once forked off, we wait around for all of them to
	 * return, noting discrepancies while we wait.
	 */
	for (i = 0; i < nfs; i++) {
		if ((fip[i].fi_flags & FS_FL_DEPDS) == 0) {
			if (fork_proc(ac, av, &fip[i], fn) < 0) {
				/* error logged by fork_proc */
				return (FORK_EFORK);
			}
		}
	}

	rc = 0;
	for (;;) {
		pid_t pid;
		int status, prc;

		pid = wait(&status);
		if (pid == (pid_t)-1) {
			if (errno == EAGAIN || errno == EINTR) {
				continue;
			}
			if (errno != ECHILD) {
				/* 22642 */
				scds_syslog(LOG_ERR,
				    "Wait for child returned unexpected "
				    "error: %s", strerror(errno));
			}
			break;
		}
		prc = 0;
		if (WIFSIGNALED(status)) {
			prc = FORK_ESIGNAL;
		} else if (WEXITSTATUS(status)) {
			prc = FORK_EEXIT;
		}
		rc = MIN(prc, rc);

		for (i = 0; i < nfs; i++) {
			int j;

	/* N.B. Bad indentation here to meet cstyle requirements */

		if (pid == fip[i].fi_pid) {
			fip[i].fi_flags |= FS_FL_DONE;
			fip[i].fi_status = status;
			if (status != 0) {
				fip[i].fi_flags |= FS_FL_ERR;
			}

		if (prc == FORK_ESIGNAL) {
			/* 22685 */
			scds_syslog(LOG_ERR,
			    "%s: Error: exited w/ signal <%d>%s.",
			    fip[i].fi_fs, WTERMSIG(status),
			    WCOREDUMP(status) ? " -- core dumped" : "");
		}
		/*
		 * Start any processes that had to wait for this
		 * one to complete.
		 */
		if (fip[i].fi_flags & FS_FL_DEPDON) {
			for (j = i+1; j < nfs; j++) {
				if (fip[j].fi_dep == i) {
					if (status == 0) {
						if (fork_proc(ac,
						    av, &fip[j], fn) < 0) {
							return (FORK_EFORK);
						}
					}
					if (status) {
						scds_syslog(LOG_ERR,
						    "%s: Operation failed: "
						    "Not starting proc "
						    "for FS %s.",
						    fip[i].fi_fs,
						    fip[j].fi_fs);
					}
				}
			}
		}
		break;
		}
		}
		if (fip[i].fi_mntpt == NULL) {
			scds_syslog(LOG_ERR,
			    "Unknown child process returned "
			    "(pid %d, status=%#x)", pid, status);
		}
	}
	return (rc);
}


/*
 * ----- RevDepForkProcs(ac, av, fip, fn)
 *
 * Like ForkProcs, but we worry about any dependencies in the
 * list of FSes, delaying forks to set up FSes that require
 * other FSes' completion.  This works the dependencies backwards,
 * e.g., unmounting FSes.
 *
 * Return values:
 *	0 - all forked children exited w/ status 0.
 *  1 - one or more children exited w/ non-zero status.
 *	2 - one or more children exited w/ a signal-received status.
 *	3 - fork() failed
 */
int
RevDepForkProcs(int ac,
				char **av,
				struct FsInfo *fip,
				int (*fn)(int, char **, struct FsInfo *))
{
	int nfs, i, rc;

	for (nfs = 0; fip[nfs].fi_mntpt != NULL; ) {
		nfs++;
	}
	if (nfs == 1) {
		/*
		 * Don't bother forking if there's only one filesystem
		 */
		return (*fn)(ac, av, &fip[0]);
	}

	/*
	 * We've got the FS names, their mount points, and
	 * the list of nodes that can serve as MDSes.  Now
	 * we need to fork off a proc for each FS.
	 *
	 * Each forked copy calls (*fn)() with all our arguments.
	 * Once forked off, we wait around for all of them to
	 * return, noting discrepancies while we wait.
	 */
	for (i = 0; i < nfs; i++) {
		if ((fip[i].fi_flags & FS_FL_DEPDON) == 0) {
			if (fork_proc(ac, av, &fip[i], fn) < 0) {
				/* error logged by fork_proc */
				return (FORK_EFORK);
			}
		}
	}

	rc = 0;
	for (;;) {
		pid_t pid;
		int status, prc;

		pid = wait(&status);
		if (pid == (pid_t)-1) {
			if (errno == EAGAIN || errno == EINTR) {
				continue;
			}
			if (errno != ECHILD) {
				/* 22642 */
				scds_syslog(LOG_ERR,
				    "Wait for child returned unexpected "
				    "error: %s", strerror(errno));
			}
			break;
		}
		prc = 0;
		if (WIFSIGNALED(status)) {
			prc = FORK_ESIGNAL;
		} else if (WEXITSTATUS(status)) {
			prc = FORK_EEXIT;
		}
		rc = MIN(prc, rc);

		for (i = 0; i < nfs; i++) {
			int j;

			if (pid == fip[i].fi_pid) {
				fip[i].fi_flags |= FS_FL_DONE;
				fip[i].fi_status = status;
				if (status != 0) {
					fip[i].fi_flags |= FS_FL_ERR;
				}

	/* N.B. Bad indentation here to meet cstyle requirements */

			if (prc == FORK_ESIGNAL) {
				/* 22685 */
				scds_syslog(LOG_ERR,
				    "%s: Error: exited w/ signal <%d>%s.",
				    fip[i].fi_fs, WTERMSIG(status),
				    WCOREDUMP(status) ? " -- core dumped" : "");
			}
			/*
			 * Start the FS this FS depends on, if any, but
			 * only if all other FSes that depend on it have
			 * also completed.
			 */
			if (fip[i].fi_flags & FS_FL_DEPDS) {
				for (j = fip[i].fi_dep + 1;
				    j < nfs; j++) {
					if (fip[j].fi_dep == fip[i].fi_dep &&
					    (fip[j].fi_flags & (FS_FL_STARTED|
					    FS_FL_DONE|FS_FL_ERR)) !=
					    (FS_FL_STARTED| FS_FL_DONE)) {
						break;
					}
				}
				if (fip[j].fi_mntpt == NULL) {
					if (fork_proc(ac, av,
					    &fip[fip[i].fi_dep], fn) < 0) {
						return (FORK_EFORK);
					}
				}
			}
				break;
			}
		}
		if (fip[i].fi_mntpt == NULL) {
			scds_syslog(LOG_ERR,
			    "Unknown child process returned "
			    "(pid %d, status=%#x)", pid, status);
		}
	}
	return (rc);
}


/*
 * ----- int mount_qfs(char *qfsfs)
 *
 * Mounts the QFS filesystem given the family set name
 * specified via the QFSFileSystem Extension property.
 * Calls /usr/sbin/mount to do the actual mounting.
 */
int
mount_qfs(char *qfsfs)
{
	int	rc;
	int	count = 0;
	char cmd[2048];

	(void) snprintf(cmd, sizeof (cmd),
	    "/usr/sbin/mount -F samfs %s", qfsfs);
	while (TRUE) {
		rc = run_system(LOG_ERR, cmd);
		if (rc != EXIT_RETRY) {
			break;
		}
		count++;
		if (count == 2) {
			count = 0;
			scds_syslog(LOG_ERR,
			    "%s: Unable to mount qfs: %s. Retrying",
			    qfsfs, strerror(ENOTCONN));
		}
	}
	return (rc);
}


/*
 * ----- int umount_qfs(struct RgInfo *, struct FsInfo *)
 *
 * unmounts the QFS filesystem
 *
 * Calls /usr/sbin/umount to do the actual unmount.
 */
int
umount_qfs(struct RgInfo *rgp, struct FsInfo *fip)
{
	int	rc;
	char cmd[PATH_MAX+64];

	if (rgp->rg_umnt_wait == 0) {
		(void) snprintf(cmd, sizeof (cmd),
		    "/usr/sbin/umount -f %s", fip->fi_fs);
	} else {
		(void) snprintf(cmd, sizeof (cmd),
		    "/usr/sbin/umount -f -o await_clients=%d %s",
		    rgp->rg_umnt_wait, fip->fi_fs);
	}
	rc = run_system(LOG_ERR, cmd);
	return (rc);
}


/*
 * ----- boolean_t validate_qfsdevs(char *qfsdev)
 *
 * Given a qfs FS "dev" (family set name), verify:
 * 1) the FS exists;
 * 2) The FS's component devices are not /dev/global/ prefixed
 * 3) That potential servers have /dsk/ infixed (and no "nodev"
 *    metadata devices).
 *
 * Return 0 if OK, -1 for problems.
 */
static boolean_t
validate_qfsdevs(char *qfsdev)
{
	int i;
	struct sam_mount_info mi;

	if (GetFsInfo(qfsdev, &mi.params) < 0) {
		/* 22632 */
		scds_syslog(LOG_ERR,
		    "Couldn't get FS %s param info: %s.",
		    qfsdev, strerror(errno));
		dprintf(stderr, catgets(catfd, SET, 22632,
		    "Couldn't get FS %s param info: %s."),
		    qfsdev, strerror(errno));
		return (B_FALSE);
	}

	if (GetFsParts(qfsdev, mi.params.fs_count, &mi.part[0]) < 0) {
		/* 22675 */
		scds_syslog(LOG_ERR,
		    "%s: Unable to get partition information: %s.",
		    qfsdev, strerror(errno));
		dprintf(stderr, catgets(catfd, SET, 22675,
		    "%s: Unable to get partition information: %s."),
		    qfsdev, strerror(errno));
		return (B_FALSE);
	}

	for (i = 0; i < mi.params.fs_count; i++) {
		int err;
		/*
		 * This area should tolerate changes.  Someday,
		 * we may want to, say, allow only devices like
		 * /dev/did/... OR /dev/vx/... OR /dev/md/... OR &c
		 *
		 * For now, we only care that /dev/global/
		 * device names are not used.
		 *
		 *	if (strncmp(&mi.part[i].pt_name[0], "/dev/did/",
		 *		strlen("/dev/did/")) == 0) {
		 *		continue;	* /dev/did/ device OK *
		 *	}
		 */

		err = 0;
		/* /dev/global/ devs not allowed */
		if (strstr(&mi.part[i].pt_name[0], "/dev/global/") != 0) {
			dprintf(stderr,
			    "%s: /dev/global/* device (%s) in configuration.",
			    qfsdev, &mi.part[i].pt_name[0]);
			err = 1;
		}
		/* No "/dsk/" in path -- can't figure raw dev */
		if (strstr(&mi.part[i].pt_name[0], "/dsk/") == 0) {
			dprintf(stderr,
			    "%s: No /dsk/ string (%s) in device.",
			    qfsdev, &mi.part[i].pt_name[0]);
			err = 1;
		}

		if (err) {
			/* 22634 */
			scds_syslog(LOG_ERR,
			    "Inappropriate path in FS %s device component: %s",
			    qfsdev, &mi.part[i].pt_name[0]);
			dprintf(stderr, catgets(catfd, SET, 22634,
			    "Inappropriate path in FS %s device component: %s"),
			    qfsdev, &mi.part[i].pt_name[0]);
			return (B_FALSE);
		}
	}
	return (B_TRUE);
}


/*
 * validate_node(nodename, priority, special)
 *
 * Obtains the shared hosts file from the raw device, and
 * locates its own entry therein.  Verifies that the entry
 * is found, has a non-zero priority (i.e., can become the
 * MDS for the FS), and uses the local node's SC private
 * host interconnect for its IP connection.
 *
 * Returns B_TRUE if all these things are OK, B_FALSE otherwise.
 */
static boolean_t
validate_node(char *nodename, char *nodepriv, char *qfsdev)
{
	int		i;
	int		errc;

	char	*ip;
	char	*errmsg;
	char	*priority;
	char	*server;

	char	***ATab;

	if (htb == NULL) {
		htb =
		    (sam_host_table_blk_t *)malloc(SAM_LARGE_HOSTS_TABLE_SIZE);
		htbufsize = SAM_LARGE_HOSTS_TABLE_SIZE;
		bzero((char *)htb, htbufsize);
	}

	/* Get list of potential masters */
	if (get_qfshosts(qfsdev, htb, htbufsize, RAW)) {
		/* 22671 */
		scds_syslog(LOG_ERR, "%s: Unable to get the QFS hosts table.  "
		    "Error: %s", qfsdev, strerror(errno));
		dprintf(stderr, "%s: Unable to get the QFS hosts table.  "
		    "Error: %s", qfsdev, strerror(errno));
		return (B_FALSE); /* Errors logged by get_qfshosts() */
	}

	/* Convert hosts data to array */
	if ((ATab = SamHostsCvt(&htb->info.ht, &errmsg, &errc)) == NULL) {
		/* 13773 */
		scds_syslog(LOG_ERR, "%s:  Cannot convert hosts file (%s/%d).",
		    program_name, errmsg, errc);
		dprintf(stderr, "%s:  Cannot convert hosts file (%s/%d).",
		    program_name, errmsg, errc);
		return (B_FALSE);
	}

	/* Find entry associated with nodename */
	for (i = 0; ATab[i] != NULL; i++) {
		if (strcasecmp(nodename, ATab[i][HOSTS_NAME]) == 0) {
				break;
		}
	}

	/* No entry found */
	if (ATab[i] == NULL) {
		/* 22672 */
		scds_syslog(LOG_ERR,
		    "%s: Unable to find the host %s in the QFS hosts table.",
		    qfsdev, nodename);
		dprintf(stderr,
		    "%s: Unable to find the host %s in the QFS hosts table.",
		    qfsdev, nodename);
		return (B_FALSE);
	}

	ip = ATab[i][HOSTS_IP];
	server = ATab[i][HOSTS_NAME];
	priority = ATab[i][HOSTS_PRI];

	/* Validate privlink */
	if (incasestring(nodepriv, ip) == 0) {
		/* 22674 */
		scds_syslog(LOG_ERR, "%s: Unable to find private link %s "
		    "for host %s in the QFS hosts table.",
		    qfsdev, nodepriv, nodename);
		dprintf(stderr, "%s: Unable to find private link %s "
		    "for host %s in the QFS hosts table.",
		    qfsdev, nodepriv, nodename);
		return (B_FALSE);
	}

	/* Validate priority */
	if (priority == NULL || atoi(priority) == 0)  {
		/* 22673 */
		scds_syslog(LOG_ERR, "%s: Invalid priority (%s) for server %s",
		    qfsdev, priority, server);
		dprintf(stderr, "%s: Invalid priority (%s) for server %s",
		    qfsdev, priority, server);
		return (B_FALSE);
	}

	return (B_TRUE);
}


/*
 * validate_qfshosts(struct RgInfo *rgp, char *qfsdev)
 *
 * Read the FS's shared hosts file, and verify that the
 * SunCluster private interconnect link is named as the
 * host IP address for each cluster member.
 */
static boolean_t
validate_qfshosts(struct RgInfo *rgp, char *qfsdev)
{
	int		i;
	int		ecount = 0;
	int		errc;

	char	*errmsg;
	char	***ATab;

	scha_err_t		err;

	if (htb == NULL) {
		htb =
		    (sam_host_table_blk_t *)malloc(SAM_LARGE_HOSTS_TABLE_SIZE);
		htbufsize = SAM_LARGE_HOSTS_TABLE_SIZE;
		bzero((char *)htb, htbufsize);
	}

	/* Get list of potential masters */
	if (get_qfshosts(qfsdev, htb, htbufsize, RAW)) {
		/* 22671 */
		scds_syslog(LOG_ERR, "%s: Unable to get the QFS hosts table.  "
		    "Error: %s", qfsdev, strerror(errno));
		dprintf(stderr, "%s: Unable to get the QFS hosts table.  "
		    "Error: %s", qfsdev, strerror(errno));
		return (B_FALSE); /* Errors logged by get_qfshosts() */
	}

	/* Convert hosts data to array */
	if ((ATab = SamHostsCvt(&htb->info.ht, &errmsg, &errc)) == NULL) {
		/* 13773 */
		scds_syslog(LOG_ERR, "%s:  Cannot convert hosts file (%s/%d).",
		    program_name, errmsg, errc);
		dprintf(stderr, "%s:  Cannot convert hosts file (%s/%d).",
		    program_name, errmsg, errc);
		return (B_FALSE);
	}

	/* Find entry associated with nodename */
	for (i = 0; ATab[i] != NULL; i++) {
		char *nodepriv;
		char *server	= ATab[i][HOSTS_NAME];
		char *ip		= ATab[i][HOSTS_IP];
		char *priority	= ATab[i][HOSTS_PRI];

		/* Skip servers with a zero priority - these are clients */
		if (priority == NULL || atoi(priority) == 0)  {
			continue;
		}

		/* Get the node's private link name */
		err = scha_cluster_get(rgp->rg_cl,
		    SCHA_PRIVATELINK_HOSTNAME_NODE, server, &nodepriv);
		if (err != SCHA_ERR_NOERR) {
			/* 22683 */
			scds_syslog(LOG_ERR,
			    "%s: Unable to get private link name for %s:"
			    "%s.", qfsdev, server, scds_error_string(err));
			dprintf(stderr, catgets(catfd, SET, 22683,
			    "%s: Unable to get private link name for %s: %s"),
			    qfsdev, server, scds_error_string(err));
			ecount++;
			continue;
		}

		/*
		 * Validate the IP address field matches
		 * the private link name
		 */
		if (incasestring(nodepriv, ip) == 0) {
			/* 22684 */
			scds_syslog(LOG_ERR, "%s: Inconsistency found "
			    "in the QFS hosts table for host '%s'.  "
			    "Expected to find '%s', found '%s'.",
			    qfsdev, server, nodepriv, ip);
			dprintf(stderr, catgets(catfd, SET, 22684,
			    "%s: Inconsistency found in QFS hosts table for "
			    "host '%s'.  Expected to find '%s', found '%s'."),
			    qfsdev, server, nodepriv, ip);
			ecount++;
			continue;
		}
	}

	/* Free memory */
	SamHostsFree(ATab);

	return (ecount ? B_FALSE : B_TRUE);
}


/*
 * ----- int get_qfsstate(char *fsname)
 *
 * Get the state of the filesystem
 * 0 if it is OK,
 * 1 if reconfiguring,
 * 2 if it is configured to no server, and
 * -1 if error.
 */

#define		ST_DONE		0
#define		ST_PENDING	1
#define		ST_NOSRVR	2
#define		ST_ERROR	-1

static int
get_qfsstate(char *fsname)
{
	struct sam_fs_info fsi;
	int	rc;

	if (htb == NULL) {
		htb =
		    (sam_host_table_blk_t *)malloc(SAM_LARGE_HOSTS_TABLE_SIZE);
		htbufsize = SAM_LARGE_HOSTS_TABLE_SIZE;
		bzero((char *)htb, htbufsize);
	}

	if ((rc = GetFsInfo(fsname, &fsi)) < 0) {
		/* 22608 */
		scds_syslog(LOG_ERR,
		    "Failed to get state of QFS filesystem %s: %s.",
		    fsname, strerror(errno));
		dprintf(stderr, catgets(catfd, SET, 22608,
		    "Failed to get state of QFS filesystem %s: %s."),
		    fsname, strerror(errno));
		return (ST_ERROR);
	}
	if (get_qfshosts(fsname, htb, htbufsize, RAW)) {
		/* Errors logged by get_qfshosts(); */
		return (ST_ERROR);
	}

	rc = ST_DONE;
	if (htb->info.ht.server != htb->info.ht.pendsrv) {
		rc = ST_PENDING;
	} else if (fsi.fi_status & FS_MDS_CHANGING) {
		rc = ST_PENDING;
	} else if (htb->info.ht.server == HOSTS_NOSRV) {
		rc = ST_NOSRVR;
	}

	scds_syslog_debug(1, "QFS device %s is in state %d.", fsname, rc);

	return (rc);
}


/*
 * ----- int svc_validate(struct RgInfo *rgp, struct FsInfo *fip)
 *
 * Validate the configuration
 * 1. Make sure the QFSFileSystem property is set
 * 2. Validate the /etc/vfstab entry for QFSFileSystem
 *		2.1) It exists in /etc/vfstab
 *		2.2) vfstype is "samfs"
 *		2.3) Options contain "shared" and not "bg"?
 *		2.4) Mount-at-boot is false
 * 3. Validate the Shared QFS configuration
 *		3.1) The fs is configured
 *		3.2) All devices for the filesystem are /dev/did
 *		3.3) The shared hosts file exists (on the FS)
 *		3.4) The shared hosts file file contains Cluster nodenames
 *		3.5) The shared hosts file contains private interconnect IPs.
 *
 * Return 0 if OK, 1 for problems.
 */
int
svc_validate(struct RgInfo *rgp, struct FsInfo *fip)
{
	int i;
	char *nodepriv;
	scha_err_t e;
	int r, err = 0;

	/* Get this node's private link name */
	e = scha_cluster_get(rgp->rg_cl,
	    SCHA_PRIVATELINK_HOSTNAME_LOCAL, &nodepriv);
	if (e != SCHA_ERR_NOERR) {
		/* 22646 */
		scds_syslog(LOG_ERR,
		    "Unable to get local private link name: %s.",
		    scds_error_string(e));
		dprintf(stderr,
		    "Unable to get local private link name: %s.",
		    scds_error_string(e));
		return (1);
	}

	/*
	 * The QFS file system extension property is a list of one or more
	 * QFS file systems. Validate each one.
	 */
	for (i = 0; fip[i].fi_mntpt != 0; i++) {
		if (!validate_qfsdevs(fip[i].fi_fs)) {
			err = 1;
			dprintf(stderr,
			    "FS %s: validate_qfsdevs() failed.", fip[i].fi_fs);
			continue;	/* Errors logged by validate_qfsdevs */
		}

		if (!validate_node(rgp->rg_nodename, nodepriv, fip[i].fi_fs)) {
			err = 1;
			dprintf(stderr,
			    "FS %s: validate_node() failed.", fip[i].fi_fs);
			continue; /* Errors logged by validate_node */
		}

		/* Make sure qfsdev is a valid QFS device */
		if ((r = get_qfsstate(fip[i].fi_fs)) != ST_DONE) {
			err = 1;
			dprintf(stderr, "FS %s: get_qfsstate() failed (%d).",
			    fip[i].fi_fs, r);
			continue;  /* Errors logged by get_qfsstate */
		}

		/* Validate QFS hosts table */
		if (!validate_qfshosts(rgp, fip[i].fi_fs)) {
			err = 1;
			dprintf(stderr,
			    "FS %s: validate_qfshosts() failed.", fip[i].fi_fs);
			continue;
		}
	}

	return (err);
}


/*
 * ----- int wait_for_stable_qfsstate(char *qfsdev, char *oldmaster)
 *
 * The parameter "oldmaster" is NULL if this is an involuntary
 * switchover (old master is dead).  However, if it is not null,
 * it means this is a voluntary switchover from the oldmaster.
 *
 * We keep an eye on the oldmaster and if it happens to die while
 * we are waiting for the switchover to complete, we return early
 * with a special error code as the voluntary switchover is never
 * going to complete.
 *
 * Return values:
 *	FO_DONE		- failover complete
 *	FO_NOSRVR	- failover to "no server" complete
 *	FO_MDSDIED	- voluntary failover in progress; old server died
 *	FO_ERROR	- error while waiting for failover completion
 */

int
wait_for_stable_qfsstate(struct RgInfo *rgp,
						char *qfsdev,
						char *oldmaster)
{
	scha_err_t			e;
	scha_node_state_t	nstate;
	int					iterations = 0;
	boolean_t			stable = B_TRUE;

	/*
	 * Wait for error or success
	 */
	for (iterations = 0; ; iterations++) {
		switch (get_qfsstate(qfsdev)) {
		case ST_DONE:
			/* 22615 */
			if (stable == B_TRUE) {
				/* 22613 */
				scds_syslog(LOG_NOTICE,
				    "%s: Expected switchover not pending.",
				    qfsdev);
			}
			scds_syslog(LOG_NOTICE,
			    "%s: Switchover complete.", qfsdev);
			return (FO_DONE);

		case ST_NOSRVR:
			/* 22636 */
			scds_syslog(LOG_NOTICE,
			    "%s: Switchover completed to server (NONE).",
			    qfsdev);
			return (FO_NOSRVR);

		case ST_PENDING:
			stable = B_FALSE;
			if (iterations % 30 == 0) {
				/* 22614 */
				scds_syslog(LOG_NOTICE,
				    "%s: Waiting for switchover to complete.",
				    qfsdev);
			}
			break;

		case ST_ERROR:
		default:
			return (FO_ERROR);
		}

		/* Keep an eye on the old master */
		if (oldmaster) {
			if ((e = scha_cluster_get(rgp->rg_cl,
			    SCHA_NODESTATE_NODE,
			    oldmaster, &nstate)) != SCHA_ERR_NOERR) {
				/* 22616 */
				scds_syslog(LOG_ERR,
				    "Failed to get state for node <%s>: %s.",
				    oldmaster, scds_error_string(e));
				return (FO_ERROR);
			}

			if (nstate != SCHA_NODE_UP) {
				/*
				 * Oops.  During voluntary failover,
				 * old server died
				 */
				/* 22617 */
				scds_syslog(LOG_ERR,
				    "%s: Previous metadata server %s left "
				    "cluster during a voluntary switchover.",
				    qfsdev, oldmaster);
				return (FO_MDSDIED);
			}
		}

		/* Keep polling */
		(void) sleep(2);
	}
}


/*
 * ----- int mon_start(struct RgInfo *rgp)
 *
 * This function starts the fault monitor for a QFS resource.
 * This is done by starting the probe under PMF. The PMF tag
 * is derived as RG-name,RS-name.mon. The restart option of PMF
 * is used but not the "infinite restart". Instead
 * interval/retry_time is obtained from the RTR file.
 * It starts the daemon "scqfs_probe" in the RT base
 * directory.
 */
int
mon_start(struct RgInfo *rgp)
{
	if (scds_pmf_start(rgp->rg_scdsh, SCDS_PMF_TYPE_MON,
	    SCDS_PMF_SINGLE_INSTANCE, "scqfs_probe", 0) != SCHA_ERR_NOERR) {
		/* 22612 */
		scds_syslog(LOG_ERR, "Failed to start fault monitor.");
		return (1);
	}

	/* 22618 */
	scds_syslog(LOG_INFO, "Started the fault monitor.");
	return (0);
}


/*
 * ----- int mon_stop(struct RgInfo *rgp)
 *
 * This function stops the fault monitor for a QFS resource.
 * It uses the DSDL API scds_pmf_stop() to do that.
 * Failure here would land the resource in STOP_FAILED
 * state.
 * The monitor is killed with signal SIGKILL and the
 * last argument of "-1" denotes the timeout (infinite).
 */
int
mon_stop(struct RgInfo *rgp)
{
	if (scds_pmf_stop(rgp->rg_scdsh, SCDS_PMF_TYPE_MON,
	    SCDS_PMF_SINGLE_INSTANCE, SIGKILL, -1) != SCHA_ERR_NOERR) {
		/* 22619 */
		scds_syslog(LOG_ERR, "Failed to stop fault monitor.");
		return (1);
	}

	/* 22620 */
	scds_syslog(LOG_INFO, "Stopped the fault monitor.");
	return (0);
}


/*
 * ----- int run_cmd(int pri, char *cmd)
 *
 * A wrapper around system(): The difference is that it processes
 * the results of running the system() call and syslogs an appropriate
 * error message.  The return value is same as returned by system()
 * AFTER doing a WEXITSTATUS() on it.  If the command does not exit
 * cleanly, -1 is returned.
 *
 * All errors are syslogged at LOG_ERR, except in the case of the
 * command exiting with a non-zero exit-code, in which case the
 * passed in pri argument is used to syslog.  The idea is that if
 * that is an expected outcome, callers would call us with LOG_DEBUG.
 */
int
run_cmd(int pri, char *cmd)
{
	int		rc;
	char	msg[1024];
	int		p = LOG_ERR;

	rc = system(cmd);
	if (rc == -1) {
		/* 22621 */
		scds_syslog(LOG_ERR,
		    "Cannot execute %s: %s.", cmd, strerror(errno));
		return (rc);
	}

	if (WIFSIGNALED((uint_t)rc)) {
		rc = -1;
		/* 22623 */
		(void) snprintf(msg, sizeof (msg),
		    "%s terminated with signal %d", cmd, WTERMSIG((uint_t)rc));
	} else if (WIFSTOPPED((uint_t)rc)) {
		rc = -1;
		/* 22624 */
		(void) snprintf(msg, sizeof (msg),
		    "%s stopped with signal %d", cmd, WSTOPSIG((uint_t)rc));
	} else if (WIFEXITED((uint_t)rc)) {
		rc = WEXITSTATUS((uint_t)rc);
		/* 22625 */
		(void) snprintf(msg, sizeof (msg),
		    "%s exited with status %d", cmd, rc);
		if (rc != 0)
			p = pri;
		else
			p = LOG_DEBUG;
	} else {
		rc = -1;
		/* 22626 */
		(void) snprintf(msg, sizeof (msg),
		    "%s returned with status %d", cmd, rc);
	}

	/* 22622 */
	scds_syslog(p, "Command %s failed to run: %s.", cmd, msg);

	return (rc);
}


/*
 * ----- int run_system(int pri, char *cmd)
 *
 * A wrapper around run_cmd():  If the command being run exits
 * non-zero, the output from the command (stderr) is captured
 * and syslogged at the supplied priority.  A maximum of 1024 chars
 * are logged, the rest of the output is silently ignored.  Intended
 * to be used to run SAM-QFS admin commands.
 *
 * The return value is same as returned by system() AFTER doing
 * a WEXITSTATUS() on it.
 */
int
run_system(int pri, char *cmd)
{
	char	runcmd[2048];
	char	outfile[PATH_MAX];
	char	buf[1024];
	FILE	*fp = NULL;
	int		n, rc;

	(void) snprintf(outfile, sizeof (outfile),
	    "%s/scqfs_run.out.%d", SCQFS_TMP_DIR, (int)getpid());
	(void) snprintf(runcmd, sizeof (runcmd), "%s > %s 2>&1", cmd, outfile);

	rc = run_cmd(pri, runcmd);
	if (rc == -1) {
		(void) unlink(outfile);
		return (rc);
	}

	if (rc != 0) {
		fp = fopen(outfile, "r");
		if (fp != NULL) {
			n = (int)fread(buf, sizeof (char),
			    sizeof (buf) - 1, fp);
			/* The output file could be zero length */
			if (n > 0)
				buf[n - 1] = (char)0;
			else
				buf[0] = (char)0;
		} else {
			/* 22610 */
			scds_syslog(pri,
			    "Unable to open %s: %s", outfile, strerror(errno));
			buf[0] = (char)0;
		}
		/* 22627 */
		scds_syslog(pri, "%s failed: %s.", cmd, buf);
	}

	if (fp) {
		(void) fclose(fp);
	}
	(void) unlink(outfile);
	return (rc);
}


/*
 * ----- int set_server(char *qfsdev, char *master, int mode)
 *
 * Sets the current metadata server, using mode
 * to determine voluntary (mode=0) or involuntary
 * (mode!=0) methods
 *
 * Return code is 0 if success, non-zero
 * otherwise.
 */
int
set_server(char *qfsdev, char *master, int mode)
{
	int i, rc;
	int server_index = -1;

	char *errmsg;
	char ***ATab;

	int newhtsize;

	if (htb == NULL) {
		htb =
		    (sam_host_table_blk_t *)malloc(SAM_LARGE_HOSTS_TABLE_SIZE);
		htbufsize = SAM_LARGE_HOSTS_TABLE_SIZE;
		bzero((char *)htb, htbufsize);
	}

	scds_syslog(LOG_NOTICE,
	    "%s: Initiating %svoluntary MDS change to node %s",
	    qfsdev, (mode == RAW ? "in" : ""), master);

	/* Get list of potential masters */
	if (rc = get_qfshosts(qfsdev, htb, htbufsize, RAW)) {
		/* 22671 */
		scds_syslog(LOG_ERR, "%s: Unable to get the QFS hosts table.",
		    qfsdev);
		return (rc); /* Errors logged by get_qfshosts() */
	}

	if (!streq(master, NO_SERVER)) {
		/* Convert hosts data to array */
		if ((ATab = SamHostsCvt(&htb->info.ht, &errmsg, &rc)) == NULL) {
			/* 13773 */
			scds_syslog(LOG_ERR,
			    "%s:  Cannot convert hosts file (%s/%d).",
			    qfsdev, errmsg, rc);
			return (rc);
		}

		/* Find the server index */
		for (i = 0; ATab[i] != NULL; i++) {
			if (strcasecmp(master, ATab[i][HOSTS_NAME]) == 0) {
				if (!ATab[i][HOSTS_PRI] ||
				    atoi(ATab[i][HOSTS_PRI]) != 0) {
					server_index = i;
				}
				break;
			}
		}

		/* Search complete - free memory */
		SamHostsFree(ATab);

		/* Server not found. */
		if (server_index == -1) {
			/* 22643 */
			scds_syslog(LOG_ERR,
			    "%s: Server %s is not a valid metadata server.",
			    qfsdev, master);
			return (ENOENT);
		}
	}

	/*
	 * Increment generation number
	 *
	 * If this was a new hosts table, it would have been necessary to save
	 * the old generation number and apply it to the new table.
	 *
	 * This code only ever does an update to an existing hosts table, so we
	 * can increment the generation number in place.
	 *
	 */
	if (++htb->info.ht.gen == 0)
		++htb->info.ht.gen;

	/* Configure server */
	if (mode == RAW) {	/* Involuntary failover */
		struct sam_fs_part slice0;
		char *rdevname;

		/* Get device name */
		if (GetFsParts(qfsdev, 1, &slice0)) {
			/* 22675 */
			scds_syslog(LOG_ERR,
			    "%s: Unable to get partition information: %s.",
			    qfsdev, strerror(errno));
			return (errno);
		}

		/* Switch "dsk" for "rdsk" */
		rdevname = getfullrawname(slice0.pt_name);
		if ((rdevname = getfullrawname(slice0.pt_name)) == NULL) {
			scds_syslog(LOG_ERR, "Out of Memory");
			return (ENOMEM);
		}

		/*
		 * Save current server and populate table
		 * with new server index
		 */
		if (htb->info.ht.server != HOSTS_NOSRV) {
			/* mark involuntary */
			htb->info.ht.prevsrv = HOSTS_NOSRV;
		}
		htb->info.ht.pendsrv = (uint16_t)server_index;
		htb->info.ht.server = (uint16_t)server_index;

		/* Write table */
		if (SamPutRawHosts(rdevname, htb, htbufsize,
		    &errmsg, &rc) < 0) {
			/* 13751 */
			scds_syslog(LOG_ERR,
			    "%s  Cannot write hosts file -- %s",
			    qfsdev, errmsg);
			free(rdevname);
			return (rc);
		}
		free(rdevname);

	} else {	/* Voluntary failover */
		if (htb->info.ht.pendsrv != htb->info.ht.server) {
			/* 22679 */
			scds_syslog(LOG_ERR,
			    "%s:  Host failover already pending.", qfsdev);
			return (0);
		}

		/* Save current server and populate pending server */
		htb->info.ht.prevsrv = htb->info.ht.server;
		if (htb->info.ht.prevsrv == HOSTS_NOSRV) {
			htb->info.ht.prevsrv = 0;
		}
		htb->info.ht.pendsrv = (uint16_t)server_index;

		/* Write table */
		if (htb->info.ht.length <= SAM_HOSTS_TABLE_SIZE) {
			newhtsize = SAM_HOSTS_TABLE_SIZE;
		} else {
			newhtsize = SAM_LARGE_HOSTS_TABLE_SIZE;
		}
		rc = sam_sethost(qfsdev, server_index, newhtsize,
		    (char *)htb);
		if (rc) {
			/* 13751 */
			scds_syslog(LOG_ERR,
			    "%s:  Cannot write hosts file -- %s",
			    qfsdev, strerror(errno));
			return (rc);
		}
	}

	/* Quiet SAM  */

	/* Wake sam-sharefsd to initiate failover */
	if ((rc = sam_shareops(qfsdev, SHARE_OP_WAKE_SHAREDAEMON, 0)) < 0) {
		/* 13749 */
		scds_syslog(LOG_ERR,
		    "%s:  sam_shareops(%s, SHARE_OP_WAKE_SHAREDAEMON, 0)"
		    " failed",
		    qfsdev, qfsdev);

		return (rc);
	}

	return (0);
}


/*
 * ----- validate_mounted(char *qfsfs, boolean_t log)
 *
 * Called from the validate method to make sure the QFS filesystem
 * is mounted.
 *
 * The reason for this check is that the there is no good way to
 * validate that the QFS filesystem is configured correctly (what
 * with did devices, use of private interconnect etc.).
 * Seeing a filesystem mounted is the only good way.
 *
 * Also called from Monitor_check method and for that method,
 * the filesystem NEEDs to be mounted.
 *
 * Returns B_TRUE if the FS is mounted (found in the mount table),
 * and B_FALSE otherwise.
 */
boolean_t
validate_mounted(char *qfsfs, boolean_t log)
{
	FILE	*mfp;
	struct mnttab  mget;

	mfp = fopen(MNTTAB, "r");
	if (mfp == NULL) {
		/* 22610 */
		scds_syslog(LOG_ERR,
		    "Unable to open %s: %s", MNTTAB, strerror(errno));
		dprintf(stderr, catgets(catfd, SET, 22610,
		    "Unable to open %s: %s"), MNTTAB, strerror(errno));
		return (B_FALSE);
	}
	while (getmntent(mfp, &mget) != -1) {
		if (streq(mget.mnt_mountp, qfsfs)) {
			(void) fclose(mfp);
			return (B_TRUE);
		}
	}
	(void) fclose(mfp);

	if (log) {
		/* 22648 */
		scds_syslog(LOG_ERR, "%s: Filesystem is not mounted.", qfsfs);
		dprintf(stderr, catgets(catfd, SET, 22648,
		    "%s: Filesystem is not mounted."), qfsfs);
	}

	return (B_FALSE);
}


/*
 * int set_cluster_mnt_opts(struct RgInfo *)
 *
 * Set the cluster-related mount flags according to the
 * lease policy declaration from the RG property.
 */
int
set_cluster_lease_mount_opts(struct RgInfo *rgp, char *fs)
{
	int err = 0;
	char *flag, *msg;

	/*
	 * Set/clear cluster-managed flag on the FS.  This tells
	 * the sam-sharefsd daemon to query the cluster framework
	 * to determine if the other FS hosts are up or not.  If
	 * noclustermgmt is set this prevents accelerated failover,
	 * even if the FS is actually cluster-managed.
	 */
	flag = "clustermgmt";
	if (rgp->rg_flags & RG_LEASEPOLICY_TIMEOUT) {
		flag = "noclustermgmt";
	}
	msg = SetFsConfig(fs, flag);
	if (strcmp(msg, "") != 0) {
		err = 1;
		scds_syslog(LOG_NOTICE,
		    "FS %s: SetFsConfig(\"%s\", %s) failed: %s.",
		    fs, fs, flag, msg);
	}

	/*
	 * Set the [no]clusterfastsw flag on the FS.  If set, this
	 * flag forces sam-sharefsd to accelerate the failover rather
	 * than waiting for all hosts to connect, even if there are
	 * FS hosts outside the cluster.  Normally, sam-sharefsd would
	 * have the kernel wait for all hosts whenever there are FS
	 * hosts that are not part of the cluster, and accelerate it
	 * otherwise (provided the cluster framework claims the
	 * unconnected hosts are DOWN).
	 */
	flag = "noclusterfastsw";
	if (rgp->rg_flags & RG_LEASEPOLICY_CLUSTER) {
		flag = "clusterfastsw";
	}
	msg = SetFsConfig(fs, flag);
	if (strcmp(msg, "") != 0) {
		err = 1;
		scds_syslog(LOG_NOTICE,
		    "FS %s: SetFsConfig(\"%s\", %s) failed: %s.",
		    fs, fs, flag, msg);
	}

	return (err);
}


/*
 * ----- static int is_node_fenced(char *node)
 *
 * Checks to see if the specified node is fenced.
 * Calls /usr/cluster/lib/sc/reserve -l.
 *
 * NOTE: reserve -l currently WAITs for fencing to complete for any
 * node.  This should be changed as soon as appropriate interfaces
 * are available so that it only checks for a specific node to be fenced.
 *
 * Thus, currently, the "node" argument is not used.
 *
 * Return values:
 *	-1: Error occurred, don't know.
 *	0: Yes
 *	1: No, not yet.
 *
 */
static int
is_node_fenced(char *node)
{
	int		rc;

	/* 22637 */
	scds_syslog(LOG_NOTICE, "Waiting for fencing to complete on %s",
	    node ? node : "(NONE)");
	rc = run_system(LOG_ERR, "/usr/cluster/lib/sc/reserve -l");
	/* 22638 */
	scds_syslog(LOG_NOTICE, "Fencing is complete on %s",
	    node ? node : "(NONE)");
	return (rc);
}


/*
 * ----- int wait_for_fence(struct RgInfo *rgp, char *node)
 *
 * Wait till the specified node is fenced.
 * Return values:
 *	-1: Error, can't deal with it.
 *	0: Success, specified node is fenced.
 *	1: Node came back online while we were waiting.
 *
 * Note:  node is actually ignored by is_node_fenced (at least for now).
 * It can also be NULL, if there is no existing/historical server.
 */
int
wait_for_fence(struct RgInfo *rgp, char *node)
{
	scha_err_t			e;
	scha_node_state_t	nstate;
	int					rc;

	for (;;) {
		rc = is_node_fenced(node);
		if (rc == 0) {
			/* 22638 */
			scds_syslog(LOG_NOTICE, "Fencing complete on node %s.",
			    node ? node : "(NONE)");
			return (0);
		}
		if (node == NULL) {
			sleep(2);
			continue;
		}
		if ((e = scha_cluster_get(rgp->rg_cl,
		    SCHA_NODESTATE_NODE, node, &nstate)) != SCHA_ERR_NOERR) {
			/* 22616 */
			scds_syslog(LOG_ERR,
			    "Failed to get state for node <%s>: %s.",
			    node, scds_error_string(e));
			dprintf(stderr, catgets(catfd, SET, 22616,
			    "Failed to get state for node <%s>: %s."),
			    node, scds_error_string(e));
			return (-1);
		}
		if (nstate == SCHA_NODE_UP) {
			/* 22630 */
			scds_syslog(LOG_NOTICE,
			    "The old master node %s came back online.", node);
			return (1);
		}
		/* Keep looping */
		(void) sleep(2);
	}
}


/*
 * Common function for finding a match of some sort in a
 * comma-separated string.
 */
static int
instr_common(const char *item,
			const char *list,
			int (*cmpfun)(const char *, const char *))
{
	char *s, *lasts, *copy;
	int found = 0;

	if (!list) {
		return (0);
	}

	copy = strdup(list);
	if (!copy) {
		/* 22602 */
		scds_syslog(LOG_ERR, "Out of memory");
		dprintf(stderr, catgets(catfd, SET, 22602, "Out of memory"));
		return (-1);
	}

	s = strtok_r(copy, ",", &lasts);
	while (s) {
		if (cmpfun(item, s) == 0) {
			found = 1;
			break;
		}
		s = strtok_r(NULL, ",", &lasts);
	}
	free(copy);
	return (found);
}


/*
 * Search a comma-separated list for the given item.
 * Return 1 if found, 0 if not.  Case sensitive.
 */
int
instring(const char *item, const char *list)
{
	return (instr_common(item, list, strcmp));
}


/*
 * Search a comma-separated list for the given item.
 * Return 1 if found, 0 if not.  Not case sensitive.
 */
int
incasestring(const char *item, const char *list)
{
	return (instr_common(item, list, strcasecmp));
}


/*
 * ----- void closefds()
 *
 * Closes standard file descriptors
 */
void
closefds(void)
{
	int fd;

	fd = open("/dev/null", O_RDWR);
	(void) dup2(fd, 0);
	(void) dup2(fd, 1);
	(void) dup2(fd, 2);
	(void) close(fd);
}
