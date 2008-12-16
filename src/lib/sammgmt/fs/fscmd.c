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
#pragma ident   "$Revision: 1.38 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * fscmd.c
 * functions to make, mount, umount and grow filesystems.
 */


#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/mnttab.h>
#include <string.h>
#include "pub/mgmt/error.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/filesystem.h"
#include "mgmt/control/fscmd.h"
#include "mgmt/util.h"
#include "mgmt/config/common.h"
#include "mgmt/config/vfstab.h"
#include "sam/sam_trace.h"
#include "sam/mount.h"

#define	MOUNT_CMD "/usr/sbin/mount -F "
#define	DEFAULT43_FS_TYPE SAMFS_TYPE
#define	UMOUNT_CMD "/usr/sbin/umount"
#define	NEWFS_CMD "/usr/sbin/newfs"

static int mount_cmd(const char *cmd);


// ------------------------ API functions ------------------------------


/*
 * mount a SAM-FS/QFS file system. deprecated. use the one next one instead
 */
int
mount_fs(
ctx_t		*ctx,
upath_t		fsname)
{
	return (mount_generic_fs(ctx, fsname, DEFAULT43_FS_TYPE));
}


// since 4.4
int
mount_generic_fs(
ctx_t *ctx /* ARGSUSED */,
char *fsname,
char *type)
{

	int res = 0;
	char cmd[500];

	snprintf(cmd, sizeof (cmd), "%s %s %s", MOUNT_CMD, type, fsname);
	res = mount_cmd(cmd);
	return (res);
}

/*
 * umount a file system
 */
int
umount_fs(
ctx_t		*ctx, /* ARGSUSED */
upath_t		name_or_mntpt,
boolean_t	force)
{
	pid_t pid;
	int status;
	FILE *err_stream = NULL;
	char line[200]; /* first line of stderr */
	char cmd[200];
	char *mntpt;
	FILE *mnttab;
	struct mnttab mnt, ckmnt;
	sqm_lst_t *nfslst = NULL;

	Trace(TR_MISC, "umounting fs %s%s", Str(name_or_mntpt),
	    force ? " -force" : "");

	/*
	 * Ensure any/all NFS shared directories are unshared.
	 * At least with QFS, NFS can leave locks open on the
	 * file system precluding the umount.  If the umount
	 * fails, re-establish the shares, effectively leaving
	 * things as we found them.
	 *
	 * If this function wasn't given a mount point, translate
	 * it here for the NFS stuff.
	 *
	 * Note that UFS file systems are umounted with the
	 * device spec, so always check mnttab.
	 *
	 */
	mntpt = name_or_mntpt;

	mnttab = fopen64(MNTTAB, "r");
	if (mnttab) {
		rewind(mnttab);		/* ensure set to top of file */

		memset(&ckmnt, 0, sizeof (struct mnttab));
		ckmnt.mnt_special = name_or_mntpt;

		status = getmntany(mnttab, &mnt, &ckmnt);

		fclose(mnttab);

		if (status == 0) {
			mntpt = mnt.mnt_mountp;
		}
	}

	(void) nfs_unshare_all(NULL, mntpt, &nfslst);

	snprintf(cmd, sizeof (cmd),
	    "%s %s%s", UMOUNT_CMD, name_or_mntpt, force ? " -f" : "");

	pid = exec_get_output(cmd, NULL, &err_stream);

	if (pid < 0) {
		if (nfslst != NULL) {
			(void) nfs_modify_active(nfslst, TRUE);
			lst_free_deep(nfslst);
		}
		return (-1);
	}

	line[0] = '\0';
	fgets(line, 200, err_stream);

	if ((pid = waitpid(pid, &status, 0)) < 0) {
		Trace(TR_ERR, "waitpid failed");
	} else {
		Trace(TR_DEBUG, "umount(pid = %ld) status: %d\n", pid, status);
	}
	fclose(err_stream);

	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status)) { /* non zero exit code */
			samerrno = SE_UMOUNT_FAILED;
			strlcpy(samerrmsg, line, MAX_MSG_LEN);
			Trace(TR_ERR, "umount exit code: %d\n",
			    WEXITSTATUS(status));
		} else {
			Trace(TR_MISC, "umounted %s. reinitializing config",
			    name_or_mntpt);
			// make the persisted mount options active
			init_config(ctx);
			if (nfslst != NULL) {
				lst_free_deep(nfslst);
			}
			return (0);
		}
	} else {
		samerrno = SE_UMOUNT_FAILED;
		strlcpy(samerrmsg, "umount abnormally terminated/stopped",
		    MAX_MSG_LEN);
	}
	Trace(TR_ERR, "umounting fs %s failed %s", name_or_mntpt, samerrmsg);
	if (nfslst != NULL) {
		(void) nfs_modify_active(nfslst, TRUE);
		lst_free_deep(nfslst);
	}
	return (-1);
}

// ------------------------ NON-API functions ------------------------------


/*
 * mount a SAM-FS/QFS file system with the specified options.
 */
static int
mount_cmd(
const char *cmd)
{

	int status;
	pid_t pid;
	FILE *err_stream = NULL;
	char line[200]; /* first line of stderr */

	Trace(TR_MISC, "mounting fs %s", cmd);

	pid = exec_get_output(cmd, NULL, &err_stream);

	if (pid < 0) {
		return (-1);
	}

	line[0] = '\0';
	fgets(line, 200, err_stream);

	if ((pid = waitpid(pid, &status, 0)) < 0) {
		Trace(TR_DEBUG, "waitpid failed");
	} else {
		Trace(TR_DEBUG, "mount (pid = %ld) status: %d\n", pid, status);
	}
	fclose(err_stream);

	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status)) { /* non zero exit code */
			samerrno = SE_MOUNT_FAILED;
			strlcpy(samerrmsg, line, MAX_MSG_LEN);
			Trace(TR_ERR, "mount exit code: %d\n",
			    WEXITSTATUS(status));
		} else {
			Trace(TR_MISC, "mounted fs %s", cmd);
			return (0);
		}
	} else {
		samerrno = SE_MOUNT_FAILED;
		strlcpy(samerrmsg, "mount abnormally terminated/stopped",
		    MAX_MSG_LEN);
	}
	Trace(TR_ERR, "mounting fs %s, failed %s", cmd, samerrmsg);
	return (-1);
}


/*
 * create a SAM/SAMQFS/QFS/UFS file system.
 * use 0 if you want to use default values for arguments
 */
int
fs_mk(
const char *name,
const int dau,		/* disk allocation unit size (blocks) */
const int inodes,	/* NOTE: if set now, cannot be increased later */
const boolean_t prevsblk,	/* previous superblock version */
const boolean_t shared)
{

	int status, i = 1;
	pid_t pid;
	FILE *err_stream = NULL;
	size_t cmdlen = MAXPATHLEN;
	char cmd[cmdlen];
	char line[200]; /* first line of stderr */


	Trace(TR_MISC, "making fs %s, %d, %d, %d",
	    name, dau, inodes, shared);

	if (name[0] == '/') {		// ufs
		snprintf(cmd, cmdlen, "%s ", NEWFS_CMD);

	} else {
		snprintf(cmd, cmdlen, "%s/%s ", SBIN_DIR, "sammkfs");

		if (dau) {
			snprintf(line, sizeof (line),  "-a%d ", dau);
			strlcat(cmd, line, cmdlen);
		}
		if (inodes) {
			snprintf(line, sizeof (line), "-i%d ", inodes);
			strlcat(cmd, line, cmdlen);
		}
		if (prevsblk)
			strlcat(cmd, "-P ", cmdlen);
		if (shared)
			strlcat(cmd, "-S ", cmdlen);
	}

	strlcat(cmd, name, cmdlen);

	/* reset line so we don't get weird info back in any trace msgs */
	line[0] = '\0';

	pid = exec_get_output(cmd, NULL, &err_stream);
	if (pid < 0)
		return (-1);

	/*
	 * consume output. needed when a lot of data is sent to stderr
	 * (e.g. mkfs on large slices)
	 * otherwise the forked process would block indefinitely
	 */
	while (NULL != fgets(line, 200, err_stream)) {
		if (i++ % 200 == 0)
			Trace(TR_MISC, "fs_mk progress: %s", Str(line));
	}

	if ((pid = waitpid(pid, &status, 0)) < 0) {
		Trace(TR_DEBUG, "waitpid failed");
	} else {
		Trace(TR_DEBUG,
		    "%s (pid = %ld) status: %d\n", cmd, pid, status);
	}

	fclose(err_stream);
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status)) { /* non zero exit code */
			samerrno = SE_MKFS_FAILED;
			strlcpy(samerrmsg, line, MAX_MSG_LEN);
			Trace(TR_ERR, "%s exit code: %d", cmd,
			    WEXITSTATUS(status));
		} else {
			/* everything is ok */
			Trace(TR_MISC, "created fs %s with command: %s",
			    name, cmd);
			return (0);
		}
	} else {
		samerrno = SE_MKFS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    "%s abnormally terminated/stopped", cmd);
	}

	Trace(TR_ERR, "making fs %s failed %s", name, samerrmsg);
	return (-1);
}


/*
 * grow an unmounted SAM/SAMQFS/QFS file system.
 */
int
fs_grow(
const char *name)	/* name of fs to grow */
{

	int status;
	pid_t pid;
	FILE *err_stream = NULL;
	char line[200]; /* first line of stderr */
	size_t cmdlen = MAXPATHLEN;
	char cmd[cmdlen];


	Trace(TR_MISC, "growing fs %s", name);

	if (ISNULL(name)) {
		return (-1);
	}

	snprintf(cmd, cmdlen, "%s/%s %s", SBIN_DIR, "samgrowfs", name);

	pid = exec_get_output(cmd, NULL, &err_stream);

	if (pid < 0)
		return (-1);

	line[0] = '\0';
	fgets(line, sizeof (line), err_stream);

	if ((pid = waitpid(pid, &status, 0)) < 0) {
		Trace(TR_OPRMSG, "waitpid failed");
	} else {
		Trace(TR_OPRMSG, "samgrowfs(pid = %ld) status: %d\n", pid,
		    status);
	}
	fclose(err_stream);

	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status)) { /* non zero exit code */
			samerrno = SE_GROW_FAILED;
			strlcpy(samerrmsg, line, MAX_MSG_LEN);
			Trace(TR_ERR, "samgrowfs exit code: %d\n",
			    WEXITSTATUS(status));
		} else {
			Trace(TR_DEBUG, "grew fs %s", name);
			return (0);
		}
	} else {
		samerrno = SE_GROW_FAILED;
		strlcpy(samerrmsg, "samgrowfs abnormally terminated/stopped",
		    MAX_MSG_LEN);
	}
	Trace(TR_ERR, "grow fs %s failed %s", name, samerrmsg);
	return (-1);
}



int
is_any_fs_mounted(
ctx_t		*ctx   	/* ARGSUSED */,
boolean_t	*mounted) {

	sqm_lst_t *l;
	node_t *n;

	if (ISNULL(mounted)) {
		return (-1);
	}

	*mounted = B_FALSE;

	if (get_fs_names(NULL, &l) == -1) {
		return (-1);
	}

	for (n = l->head; n != NULL; n = n->next) {
		if (is_fs_mounted(NULL, (char *)n->data, mounted) == -1) {
			return (-1);
		} else if (B_TRUE == *mounted) {
			lst_free_deep(l);
			return (0);
		}
	}

	lst_free_deep(l);

	/* only get to here if no fs was mounted */
	return (0);
}


/*
 * Returns 0 if able to determine that fs is mounted or not.
 * mounted is set to B_TRUE if the fs is mounted.
 * mounted is set to B_FALSE if the fs is not mounted.
 * Returns -1 if unable to make the determination.
 */
int
is_fs_mounted(
ctx_t		*ctx	/* ARGSUSED */,
char		*fs_name,
boolean_t	*mounted)
{

	fs_t	*fs = NULL;
	int	ret;

	if (ISNULL(fs_name, mounted)) {
		return (-1);
	}

	/* check if filesystem is mounted */
	ret = get_fs(NULL, fs_name, &fs);
	if (ret == 0) {
		if (fs->fi_status & FS_MOUNTED) {
			free_fs(fs);
			*mounted = B_TRUE;
		} else {
			free_fs(fs);
			*mounted = B_FALSE;
		}
	} else if (ret != -2) {
		/*
		 * only return an error if it is an error.
		 * if the fs is not found assume sam knows nothing about it.
		 * if the GetFsStatus failed assume sam is not running or has
		 * no filesystems.
		 * if the GET_FS_INFO_FAILED it is not mounted.
		 * If reading the super block failed consider the fs not
		 *
		 */
		if (samerrno != SE_NOT_FOUND &&
		    samerrno != SE_GET_FS_INFO_FAILED &&
		    samerrno != SE_GET_FS_STATUS_FAILED &&
		    samerrno != SE_OPENDISK_FAILED &&
		    samerrno != SE_LLSEEK_FAILED &&
		    samerrno != SE_READ_FAILED &&
		    samerrno != SE_READ_INCOMPLETE) {

			Trace(TR_ERR, "check if %s is mounted failed: %s",
			    fs_name, samerrmsg);

			return (-1);
		} else {
			*mounted = B_FALSE;
		}
	} else {
		/*
		 * we got the information from the file. That means
		 * that no filesystems are mounted.
		 */
		free_fs(fs);
		*mounted = B_FALSE;
	}


	Trace(TR_OPRMSG, "is %s mounted returning %s",
	    fs_name, *mounted ? "true" : "false");

	return (0);
}


#ifdef TEST
int
testgrow(char *name) {
	if (!fs_grow(name))
		printf("%s grown.\n", name);
	else
		printf("There was an error while trying to grow %s:\n\t%s\n",
			name, samerrmsg);
	printf("Mounting %s...\n", name);

	if (!fs_mount(name, lst_create()))
		printf("%s mounted.\n", name);
	else
		printf("There was an error while trying to mount %s:\n\t%s\n",
			name, samerrmsg);
	return (0);
}
main(int argc, char ** argv) {
	testgrow(argv[1]);
	if (argc != 4) {
		printf("Syntax: %s <fs_name> <dau> <ino>\n",
			basename(argv[0]));
		exit(1);
	}

	if (!fs_mk(argv[1], atoi(argv[2]), atoi(argv[3]), 0, 0))
		printf("%s created.\n", argv[1]);
	else
		printf("There was an error while trying to create %s:\n\t%s\n",
			argv[1], samerrmsg);

	// now try to mount it
	printf("Mounting %s...\n", argv[1]);

	if (!fs_mount(argv[1], lst_create()))
		printf("%s mounted.\n", argv[1]);
	else
		printf("There was an error while trying to mount %s:\n\t%s\n",
			argv[1], samerrmsg);
}
#endif
