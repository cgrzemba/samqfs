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
#pragma ident   "$Revision: 1.87 $"

static char *_SrcFile = __FILE__;  /* Using __FILE__ makes duplicate strings */

/*
 * fs.c contains control side implementation of filesystem.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfstab.h>
#include <sys/mnttab.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <pwd.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/vfs.h>
#include <procfs.h>
#define	PORTMAP
#include <rpc/rpc.h>
#include <rpc/rpcent.h>

#include "sam/types.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/fs/sblk.h"
#include "sam/fs/sblk_mgmt.h"
#include "sam/sam_trace.h"

#include "mgmt/util.h"


#include "pub/mgmt/error.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/filesystem.h"
#include "pub/mgmt/device.h"
#include "pub/devstat.h"
#include "pub/mgmt/hosts.h"
#include "pub/mgmt/license.h"
#include "mgmt/config/common.h"
#include "mgmt/config/cfg_fs.h"
#include "mgmt/config/cfg_diskvols.h"
#include "mgmt/config/vfstab.h"
#include "bswap.h"

/* extern for byteswapping superblock */
extern int byte_swap_sb(struct sam_sblk *sblk, size_t len);

const char *default_samfsck_log = VAR_DIR"/samfsck.log";
const char *samfsck_cmd = SBIN_DIR"/samfsck";

/* private helper functions declaration */
static int fill_fs(struct sam_fs_info *, sqm_lst_t *, fs_t **);
static int fill_mount_options(struct sam_fs_info *, fs_t *);
static int fill_fs_parts(struct sam_fs_info *, fs_t *);
static int fill_fs_part(struct	sam_fs_part *, fs_t *);
static striped_group_t *find_striped_group(fs_t *, const char *);
static int get_info_from_sblk(char *, fs_t *);
static boolean_t is_fatal_error(sam_errno_t);

/* NFS shared directory datastructure */
#define	FSM_NFSOPTS_LEN 8192
typedef struct nfsshare {
	char		dir_name[MAXPATHLEN];
	boolean_t	hasopts;
	boolean_t	active;
	boolean_t	config;
	boolean_t	is_mountpt;
	char		opts[FSM_NFSOPTS_LEN*3];
	char		otheropts[FSM_NFSOPTS_LEN];
	char		optsec[1024];
	char		desc[1024];
	ulong_t		fsid;
} nfsshare_t;

#define	DFSTAB "/etc/dfs/dfstab"
#define	SHARETAB "/etc/dfs/sharetab"

static int get_set_dfstab(nfsshare_t *, sqm_lst_t **);
static sqm_lst_t *parse_sharetab(void);
static int parse_nfssvropts(char *, nfsshare_t *);
static int nfssvr_mrgcomp(void*, void*);
static int is_fs_nfsshared(sqm_lst_t *, fs_t *);
static char *create_nfsshare_cmd(nfsshare_t *);
static int nfs_unshare_dir(char *dir_name, char *errbuf, int errlen);
static int nfs_share_dir(nfsshare_t *nfsent, char *errbuf, int errlen);
static int get_all_nfs(char *, sqm_lst_t **, boolean_t);
static int nfs_start_server(void);

/*
 * get_all_fs returns both the mounted and unmounted SAM-FS/QFS
 * file systems.
 */
int
get_all_fs(
ctx_t *ctx,			/* ARGSUSED */
sqm_lst_t **fs_list)	/* OUT - list of fs_t structures */
{
	int numfs;
	int i, ret1, ret = 0;
	struct sam_fs_status *fsarray = NULL;
	struct sam_fs_status *fs;
	struct sam_fs_info	*finfo = NULL;
	struct sam_fs_info	*fi;
	fs_t *f = NULL;
	sqm_lst_t *vfsentries = NULL;

	Trace(TR_MISC, "getting all fs");

	if (ISNULL(fs_list)) {
		Trace(TR_ERR, "get all fs failed: null argument found");
		goto err;
	}

	*fs_list = NULL;

	if ((numfs = GetFsStatus(&fsarray)) <= 0) {
		samerrno = SE_GET_FS_STATUS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "get all fs failed: get fs status failed");
		goto err;
	}

	finfo = (struct sam_fs_info *)
	    mallocer(numfs * sizeof (struct sam_fs_info));
	if (finfo == NULL) {
		Trace(TR_ERR, "get all fs failed: out of memory");
		goto err;
	}
	memset(finfo, 0, sizeof (struct sam_fs_info));

	for (i = 0; i < numfs; i++) {
		fs = fsarray + i;
		fi = finfo + i;
		if (GetFsInfo(fs->fs_name, fi) == -1) {
			samerrno = SE_GET_FS_INFO_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), fs->fs_name);
			Trace(TR_ERR, "get all fs failed: get fs info failed");
			goto err;
		}
	}

	free(fsarray);
	fsarray = NULL;


	*fs_list = lst_create();
	if (*fs_list == NULL) {
		Trace(TR_ERR, "get all fs failed: lst create failed");
		goto err;
	}

	/*
	 * get the vfstab entries incase they are needed but ignore errors
	 */
	get_all_vfstab_entries(&vfsentries);

	for (i = 0; i < numfs; i++) {
		fi = finfo + i;

		if ((ret1 = fill_fs(fi, vfsentries, &f)) == -1) {
			Trace(TR_ERR, "get all fs failed: fill fs failed");
			goto err;
		}

		/* for get_all_fs(), ignore error forindividual fs */
		/* and return -2 to indicate that there was an error */
		if (ret1 != 0) {
			ret = ret1;
		}

		if (lst_append(*fs_list, f) == -1) {
			Trace(TR_ERR, "get all fs failed: lst append failed");
			goto err;
		}
	}

	free(finfo);
	finfo = NULL;
	lst_free_deep_typed(vfsentries, FREEFUNCCAST(free_vfstab_entry));

	is_fs_nfsshared(*fs_list, NULL);

	Trace(TR_MISC, "get all fs returning [%d] with ret code [%d]",
	    (*fs_list)->length, ret);
	return (ret);

err:
	if (fsarray) {
		free(fsarray);
	}
	if (finfo) {
		free(finfo);
	}
	if (f) {
		free_fs(f);
	}
	if (*fs_list) {
		free_list_of_fs(*fs_list);
		*fs_list = NULL;
	}
	if (vfsentries) {
		lst_free_deep_typed(vfsentries,
		    FREEFUNCCAST(free_vfstab_entry));
	}

	Trace(TR_MISC, "get all fs from control failed:  %s", samerrmsg);
	if (!is_fatal_error(samerrno)) {
		/* try to get from mcf */
		Trace(TR_MISC, "get all fs from cfg");
		if (cfg_get_all_fs(fs_list) != -1) {
			Trace(TR_MISC, "get all fs from cfg, return code -2");
			return (-2);
		}
	}

	Trace(TR_ERR, "get all fs failed: %s", samerrmsg);
	return (-1);
}


/*
 * return the names of all configured filesystems
 */
int
get_fs_names(
ctx_t *ctx,			/* ARGSUSED */
sqm_lst_t **fs_names)	/* OUT - list of fs names (string) */
{
	int numfs;
	int i;
	char *fs_name = NULL;
	struct sam_fs_status *fsarray = NULL;
	struct sam_fs_status *fs;

	Trace(TR_MISC, "getting fs names");

	if (ISNULL(fs_names)) {
		Trace(TR_ERR, "null argument found");
		goto err;
	}

	*fs_names = NULL;

	if ((numfs = GetFsStatus(&fsarray)) == -1) {
		samerrno = SE_GET_FS_STATUS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "get fs status failed");
		goto err;
	}

	*fs_names = lst_create();
	if (*fs_names == NULL) {
		Trace(TR_ERR, "lst create failed");
		goto err;
	}

	for (i = 0; i < numfs; i++) {
		fs = fsarray + i;

		fs_name = strdup(fs->fs_name);
		if (fs_name == NULL) {
			samerrno = SE_NO_MEM;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno));
			Trace(TR_ERR, "out of memory");
			goto err;
		}

		if (lst_append(*fs_names, fs_name) == -1) {
			Trace(TR_ERR, "lst append failed");
			goto err;
		}
	}

	free(fsarray);

	Trace(TR_MISC, "got fs names list size [%d]",
	    (*fs_names)->length);
	return (0);

err:
	if (fsarray) {
		free(fsarray);
	}
	if (fs_name) {
		free(fs_name);
	}
	if (*fs_names) {
		lst_free_deep(*fs_names);
		*fs_names = NULL;
	}

	Trace(TR_MISC, "get fs names from control failed: %s", samerrmsg);

	if (!is_fatal_error(samerrno)) {
		/* try to get from mcf */
		if (cfg_get_fs_names(fs_names) != -1) {
			Trace(TR_MISC,
			    "got fs names from config, return code -2");
			return (-2);
		}
	}

	Trace(TR_ERR, "get fs names failed: %s", samerrmsg);
	return (-1);
}


/*
 * return the device ids in vfstab
 */
int
get_fs_names_all_types(
ctx_t *ctx,			/* ARGSUSED */
sqm_lst_t **fs_names)	/* OUT - list of fs names (string) */
{
	char *fs_name = NULL;
	FILE *fp;
	struct vfstab vp;
	char err_buf[80];

	Trace(TR_MISC, "getting fs names from vfstab");

	if (ISNULL(fs_names)) {
		Trace(TR_ERR, "null argument found");
		goto err;
	}

	*fs_names = NULL;

	Trace(TR_DEBUG, "populating fs name list");

	*fs_names = lst_create();
	if (*fs_names == NULL) {
		Trace(TR_ERR, "lst create failed");
		goto err;
	}

	if ((fp = fopen(VFSTAB, "r")) == NULL) {
		Trace(TR_ERR, "failed to open vfstab");
		samerrno = SE_CFG_OPEN_FAILED;

		/* Open failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), VFSTAB, err_buf);
		goto err;
	}

	while (getvfsent(fp, &vp) == 0) {

		fs_name = strdup(vp.vfs_special);
		if (fs_name == NULL) {
			samerrno = SE_NO_MEM;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno));
			Trace(TR_ERR, "out of memory");
			goto err;
		}

		if (lst_append(*fs_names, fs_name) == -1) {
			Trace(TR_ERR, "lst append failed");
			goto err;
		}
	}

	fclose(fp);

	Trace(TR_MISC, "got fs names from vfstab");
	return (0);

err:
	if (fp) {
		fclose(fp);
	}
	if (fs_name) {
		free(fs_name);
	}
	if (*fs_names) {
		lst_free_deep(*fs_names);
		*fs_names = NULL;
	}

	Trace(TR_ERR, "get fs names from vfstab failed: %s", samerrmsg);
	return (-1);
}


/*
 * get the information about a specific file system.
 */
int
get_fs(
ctx_t *ctx,		/* ARGSUSED */
uname_t fsname,	/* IN  - fs name */
fs_t **fs)		/* OUT - fs_t structure */
{
	int numfs;
	int i, ret;
	struct sam_fs_status *fsarray = NULL;
	struct sam_fs_status *f;
	struct sam_fs_info	 *finfo = NULL;
	boolean_t found = FALSE;
	sqm_lst_t *vfsentries = NULL;

	Trace(TR_MISC, "getting fs by name (%s)", Str(fsname));

	if (ISNULL(fsname, fs)) {
		Trace(TR_ERR, "null argument found");
		goto err;
	}

	*fs = NULL;

	Trace(TR_DEBUG, "getting fs status");

	if ((numfs = GetFsStatus(&fsarray)) == -1) {
		samerrno = SE_GET_FS_STATUS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "get fs status failed");
		goto err;
	}

	for (i = 0; i < numfs; i++) {
		f = fsarray + i;

		if (strcmp(f->fs_name, fsname) == 0) {
			found = TRUE;
			break;
		}
	}

	if (!found) {
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), fsname);
		Trace(TR_ERR, "fs %s not found", fsname);
		goto err;
	}

	finfo = (struct sam_fs_info *)
	    mallocer(sizeof (struct sam_fs_info));
	if (finfo == NULL) {
		Trace(TR_ERR, "out of memory");
		goto err;
	}

	Trace(TR_DEBUG, "getting fs info");

	if (GetFsInfo(f->fs_name, finfo) == -1) {
		samerrno = SE_GET_FS_INFO_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), f->fs_name);
		Trace(TR_ERR, "get fs info failed");
		goto err;
	}

	free(fsarray);
	fsarray = NULL;

	Trace(TR_DEBUG, "creating fs");

	/*
	 * get the vfstab entries incase they are needed but ignore errors
	 */
	get_all_vfstab_entries(&vfsentries);

	if ((ret = fill_fs(finfo, vfsentries, fs)) == -1) {
		Trace(TR_ERR, "fill fs failed (%d)", ret);
		goto err;
	}

	free(finfo);
	finfo = NULL;
	lst_free_deep_typed(vfsentries, FREEFUNCCAST(free_vfstab_entry));

	/* Check if any part of file system is NFS shared */
	is_fs_nfsshared(NULL, *fs);

	Trace(TR_MISC, "got fs(%s)", fsname);
	return (ret);

err:
	if (fsarray) {
		free(fsarray);
	}
	if (finfo) {
		free(finfo);
	}
	if (*fs) {
		free_fs(*fs);
		*fs = NULL;
	}
	if (vfsentries) {
		lst_free_deep_typed(vfsentries,
		    FREEFUNCCAST(free_vfstab_entry));
	}

	Trace(TR_MISC, "get fs from control failed: %s", samerrmsg);

	if (!is_fatal_error(samerrno)) {
		/* try to get from mcf */
		Trace(TR_MISC, "trying to get fs from config");

		if (cfg_get_fs(fsname, fs) != -1) {
			Trace(TR_MISC, "got fs from config, return code -2");
			return (-2);
		}
	}

	Trace(TR_ERR, "get fs failed: %s", samerrmsg);
	return (-1);
}


/*
 * Helper function to remove <defunc> processes
 * created by samfsck_fs()
 */
static void *
wait_child(void *arg)
{
	int status;

	pthread_detach(pthread_self());
	waitpid((pid_t)arg, &status, 0);

	return (NULL);
}


/*
 * Function to check consistency of a file system.
 */
int
samfsck_fs(
ctx_t *ctx,			/* ARGSUSED */
uname_t fsname,		/* IN  - fs name */
upath_t logfile,	/* IN  - path to log file */
boolean_t repair)	/* IN  - repair or not */
{
	int numfs;
	int i;
	int fd = -1; /* logfile descriptor */
	pid_t pid;
	struct sam_fs_status *fsarray = NULL;
	struct sam_fs_status *fs;
	boolean_t found = FALSE;
	upath_t local_logfile;

	Trace(TR_MISC, "starting samfsck fs");

	if (ISNULL(fsname)) {
		Trace(TR_ERR, "null argument found");
		goto err;
	}

	Trace(TR_DEBUG, "getting fs status");

	if ((numfs = GetFsStatus(&fsarray)) == -1) {
		samerrno = SE_GET_FS_STATUS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "get fs status failed");
		goto err;
	}

	for (i = 0; i < numfs; i++) {
		fs = fsarray + i;

		if (strcmp(fsname, fs->fs_name) == 0) {
			if (repair && fs->fs_mnt_point[0] != '\0') {
				samerrno = SE_FS_MOUNTED;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno), fsname);
				Trace(TR_ERR, "fs %s is mounted", fsname);
				goto err;
			}

			found = TRUE;
			break;
		}
	}

	free(fsarray);
	fsarray = NULL;

	if (!found) {
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), fsname);
		Trace(TR_ERR, "fs %s not found", fsname);
		goto err;
	}

	Trace(TR_DEBUG, "executing samfsck");

	if (logfile == NULL || logfile[0] == '\0') {
		snprintf(local_logfile, sizeof (upath_t), default_samfsck_log);
	} else {
		snprintf(local_logfile, sizeof (upath_t), logfile);
	}

	if ((fd = open(local_logfile, O_RDWR | O_CREAT, 0644)) < 0) {
		samerrno = SE_CFG_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), "logfile", local_logfile);
		Trace(TR_ERR, "open log file %s failed", local_logfile);
		goto err;
	}

	if ((pid = fork()) < 0) {
		samerrno = SE_FORK_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno));
		Trace(TR_ERR, "fork failed");
		goto err;
	}

	if (pid == 0) { /* child process */
		int j = 0;
		char *args[4];

		args[j++] = (char *)samfsck_cmd;

		(void) dup2(fd, 1);
		(void) dup2(fd, 2);

		if (repair) {
			args[j++] = "-F";
		}

		args[j++] = (char *)fsname;
		args[j] = NULL;

		if (execvp(*args, args) == -1) {
			Trace(TR_ERR, "child: samfsck %s failed", fsname);
		} else {
			Trace(TR_MISC, "child: samfsck %s succeeded", fsname);
		}
		close(fd);
		exit(1);
	} else { /* parent */

		close(fd);
	}
	pthread_create(NULL, NULL, wait_child, (void *) pid);

	Trace(TR_MISC, "samfsck fs done");
	return (0);

err:
	if (fsarray) {
		free(fsarray);
	}
	if (fd != -1) {
		close(fd);
	}

	Trace(TR_ERR, "samfsck fs failed: %s", samerrmsg);
	return (-1);
}


/*
 * Function to check all running samfsck command.
 */
int
get_all_samfsck_info(
ctx_t *ctx,			/* ARGSUSED */
sqm_lst_t **fsck_info)	/* OUT - list of fsck_info_t structs */
{
	struct passwd *pwd;
	char	*ptr;
	node_t	*node;
	psinfo_t	*info;
	sqm_lst_t	*procs;


	samfsck_info_t *samfsck = NULL;

	Trace(TR_MISC, "getting all samfsck info");

	if (ISNULL(fsck_info)) {
		Trace(TR_ERR, "null argument found");
		goto err;
	}

	*fsck_info = NULL;

	Trace(TR_DEBUG, "getting samfsck process info");

	procs = lst_create();
	if (procs == NULL) {
		Trace(TR_ERR, "lst create failed");
		goto err;
	}

	*fsck_info = lst_create();
	if (*fsck_info == NULL) {
		Trace(TR_ERR, "lst create failed");
		goto err;
	}

	(void) find_process("samfsck", &procs);

	node = procs->head;

	/* for each active process --- */
	while (node != NULL) {
		info = (psinfo_t *)node->data;

		samfsck = mallocer(sizeof (samfsck_info_t));
		if (samfsck == NULL) {
			Trace(TR_ERR, "out of memory");
			goto err;
		}

		samfsck->state = info->pr_lwp.pr_sname; /* S */

		if ((pwd = getpwuid(info->pr_euid)) != NULL) {
			snprintf(samfsck->user, sizeof (samfsck->user),
			    pwd->pw_name);
		} else {
			snprintf(samfsck->user, sizeof (samfsck->user),
			    "%d", (int)info->pr_euid);
		}

		samfsck->pid = info->pr_pid; /* PID */

		samfsck->ppid = info->pr_ppid; /* PPID */

		samfsck->pri = info->pr_lwp.pr_pri; /* PRI */

		samfsck->size = info->pr_size; /* SZ */

		samfsck->stime = info->pr_start.tv_sec; /* STIME */
		if (info->pr_start.tv_nsec > 500000000) {
			samfsck->stime++;
		}

		samfsck->time = info->pr_time.tv_sec; /* TIME */

		snprintf(samfsck->cmd, sizeof (samfsck->cmd),
		    info->pr_psargs);

		if (strstr(info->pr_psargs, " -F") == NULL) {
			samfsck->repair = FALSE;
		} else {
			samfsck->repair = TRUE;
		}

		ptr = strrchr(info->pr_psargs, ' '); /* samfs name */
		if (ptr == NULL) {
			sprintf(samfsck->fsname, "?");
		} else {
			snprintf(samfsck->fsname, sizeof (samfsck->fsname),
			    ptr + 1);
		}

		if (lst_append(*fsck_info, samfsck) == -1) {
			Trace(TR_ERR, "lst append failed");
			goto err;
		}

		node = node->next;
	}

	lst_free_deep(procs);

	Trace(TR_MISC, "get all samfsck info returning [%d] results",
	    (*fsck_info)->length);
	return (0);

err:

	if (samfsck) {
		free(samfsck);
	}
	if (*fsck_info) {
		free_list_of_samfsck_info(*fsck_info);
	}
	if (procs) {
		lst_free_deep(procs);
	}

	Trace(TR_ERR, "get all samfsck info failed: %s", samerrmsg);
	return (-1);
}


/*
 * each non-SAMQ filesystem is a described by a set of name=value pairs:
 *  name=<name>,
 *  mountpt=<mntPt>,
 *  type=<ufs|zfs|...>
 *  state=mounted|umounted,
 *  capacity=<capacity>,
 *  availspace=<space available>
 */
#define	FSNAME  "name"
#define	FSMNTPT "mountpt"
#define	FSTYPE  "type"
#define	FSSTATE "state"
#define	FSCAPAC "capacity"
#define	FSAVAIL "availspace"
#define	FSNFS	"nfs"
#define	FSSEP	','

static int
fscomp(void *fs1_str, void *fs2_str) {

	char *fs1 = (char *)fs1_str;
	char *fs2 = (char *)fs2_str;
	int idx;

	idx = strlen(FSNAME) + 1;
	while (fs1[idx] == fs2[idx]) {
		if (fs1[idx] == FSSEP && fs2[idx] == FSSEP)
			break;
		else
			idx++;
	}
	if (fs1[idx] == FSSEP)
		return (0);
	else
		return (idx);
}

int
get_generic_filesystems(
ctx_t *ctx, /* ARGSUSED */
char *filter, /* IN  - filesystems type names */
sqm_lst_t **fss) /* OUT - list of strings, each structured as described above */
{
	if (ISNULL(fss))
		return (-1);
	FILE *fp;
	struct mnttab fs;
	struct statvfs64 statfs;
	char buf[1080]; // size = VFS_LINE_MAX + length of keys
	sqm_lst_t *vfs;
	struct vfstab vfsent;
	int res;
	sqm_lst_t *nfslst = NULL;
	node_t *nfsent;

	// STEP 1/3: get list of mounted filesystems

	if (NULL == (fp = fopen(MNTTAB, "r"))) {
		return (-1);
	}
	*fss = lst_create();

	/* get list of currently nfs shared directories */
	nfslst = parse_sharetab();

	while (0 == getmntent(fp, &fs)) {
		if (NULL == strstr(filter, fs.mnt_fstype))
			continue;
		statvfs64(fs.mnt_mountp, &statfs);
#ifdef TESTING_DEBUG
		printf("%-35s %-30s %-5s %10lluk %10lluk\n",
		    fs.mnt_special, fs.mnt_mountp, fs.mnt_fstype,
		    (uint64_t)(statfs.f_blocks / 1024) * statfs.f_frsize,
		    (uint64_t)(statfs.f_bfree / 1024) * statfs.f_frsize);
#endif /* TESTING_DEBUG */
		sprintf(buf,
		    FSNAME"=%s,"FSMNTPT"=%s,"FSTYPE"=%s,"FSSTATE"=mounted,"
		    FSCAPAC"=%llu,"FSAVAIL"=%llu",
		    fs.mnt_special, fs.mnt_mountp, fs.mnt_fstype,
		    (uint64_t)(statfs.f_blocks / 1024) * statfs.f_frsize,
		    (uint64_t)(statfs.f_bfree / 1024) * statfs.f_frsize);

		if (nfslst != NULL) {
			nfsent = nfslst->head;
			while (nfsent != NULL) {
				if (((nfsshare_t *)(nfsent->data))->fsid ==
				    statfs.f_fsid) {
					strlcat(buf, ","FSNFS"=yes",
					    sizeof (buf));
					break;
				}
				nfsent = nfsent->next;
			}
		}
		lst_append(*fss, strdup(buf));
	}
	fclose(fp);

	// STEP 2/3: get list of filesystems from vfstab

	vfs = lst_create();

	fp = fopen(VFSTAB, "r");
	if (NULL == fp) {
		samerrno = SE_CFG_OPEN_FAILED;

		/* Open failed for %s: %s */
		StrFromErrno(errno, buf, sizeof (buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), VFSTAB, buf);
		goto err;
	}

	while (-1 != (res = getvfsent(fp, &vfsent))) {
		if (res > 0) {
			/*
			 * res > 0 indicates there is something
			 * wrong with this vfstab entry. Don't stop
			 * now though, continue to evaluate the
			 * remainder of the vfstab. An entry
			 * will be created for this file system
			 * if everything up through type is
			 * present in the file.
			 */
			set_samerrno_for_vfstab_error("n/a", res);
			Trace(TR_MISC, samerrmsg);
		}
		if (vfsent.vfs_fstype != NULL)
			if (strstr(filter, vfsent.vfs_fstype) != NULL) {
				sprintf(buf,
				    FSNAME"=%s,"FSMNTPT"=%s,"FSTYPE"=%s,"
				    FSSTATE"=unmounted",
				    Str(vfsent.vfs_special),
				    Str(vfsent.vfs_mountp),
				    Str(vfsent.vfs_fstype));
				lst_append(vfs, strdup(buf));
			}
	};

	fclose(fp);

	// STEP 3/3: Merge the info obtained in first 2 steps

	lst_merge(*fss, vfs, fscomp);
	lst_free_deep(vfs);
	lst_free_deep(nfslst);

	Trace(TR_MISC, "obtained generic fs info");

	return (0);

err:
	if (nfslst) {
		lst_free_deep(nfslst);
	}

	Trace(TR_ERR, "gettting generic fs failed: samerrno %d %s",
	    samerrno, samerrmsg);

	return (-1);
}

/*
 * NFS Server functions
 */
/*
 *  Set or remove the NFS configuration for a given directory.
 */
int
set_nfs_opts(ctx_t *c, char *mnt_point, char *opts)	/* ARGSUSED */
{
	nfsshare_t	nfsent;
	struct stat64	statbuf;
	int		ret;
	int		len;
	char		tmpbuf[PATH_MAX+1];
	char		errbuf[MAX_MSG_LEN] = {0};

	memset(&nfsent, 0, sizeof (nfsent));

	ret = parse_nfssvropts(opts, &nfsent);

	if (ret != 0) {
		samerrno = SE_ERROR_BAD_NFS_OPTS;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		return (-1);
	}

	/*
	 * Make sure that requested directory exists.
	 * Use realpath() to make sure the provided path isn't
	 * a symlink - share_nfs resolves symlinks when sharing
	 * which causes the entries in dfstab (symlink) and
	 * sharetab (resolved path) to differ.  The results are
	 * very weird looking in the GUI: symlink target shared,
	 * symlink itself in showing up as non-shared.
	 */
	strcpy(tmpbuf, nfsent.dir_name);

	samerrno = 0;

	if (realpath(tmpbuf, nfsent.dir_name) == NULL) {
		/*
		 * if we're removing the share completely, don't fail
		 * if the specified directory doesn't actually exist in
		 * the filesystem.
		 */
		if ((nfsent.active != B_FALSE) || (nfsent.config != B_FALSE)) {
			samerrno = SE_ERROR_NFS_NOPATH;
		}
	} else {
		ret = stat64(tmpbuf, &statbuf);
		if (ret != 0) {
			samerrno = SE_ERROR_NFS_NOPATH;
		}
	}

	if (samerrno == SE_ERROR_NFS_NOPATH) {
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), nfsent.dir_name);
		return (-1);
	}

	/* make sure requested directory is part of this mount point */
	len = strlen(mnt_point);

	if ((strcmp(mnt_point, "/") != 0) &&
	    ((strncmp(nfsent.dir_name, mnt_point, len) != 0) ||
	    ((strlen(nfsent.dir_name) > len) &&
	    (nfsent.dir_name[len] != '/')))) {
		samerrno = SE_ERROR_NFS_NOT_SUBDIR;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), nfsent.dir_name, mnt_point);
		return (-1);
	}

	ret = get_set_dfstab(&nfsent, NULL);

	if (ret != 0) {
		samerrno = SE_SET_NFS_OPTS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), nfsent.dir_name);
		return (-1);
	}

	/* the share/unshare functions set samerrno */
	if (nfsent.active) {
		ret = nfs_share_dir(&nfsent, errbuf, sizeof (errbuf));
	} else {
		ret = nfs_unshare_dir(nfsent.dir_name, errbuf, sizeof (errbuf));
	}

	if (ret != 0) {
		if ((samerrno == SE_ERROR_NFS_SHARE) ||
		    (samerrno == SE_ERROR_NFS_UNSHARE)) {
			if (errbuf[0] == '\0') {
				strlcpy(errbuf,
				    GetCustMsg(SE_ERROR_NFS_UNKNOWN),
				    sizeof (errbuf));
			}
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), nfsent.dir_name, errbuf);
		} else {
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), nfsent.dir_name);
		}
	}

	return (ret);
}


/*
 *  Given a mount point, return all directories actively
 *  shared within that filesystem, and all those configured
 *  to be shared.
 *
 *  A mnt_point of "" indicates the caller wants all NFS
 *  shares for the entire system.
 */
int
get_nfs_opts(ctx_t *c, char *mnt_point, sqm_lst_t **opts)	/* ARGSUSED */
{
	sqm_lst_t		*nfslst;
	sqm_lst_t		*retlist;
	int		ret;
	node_t		*node;
	nfsshare_t	*nfsent;
	char		buf[sizeof (nfsshare_t)];
	int		len = sizeof (buf);

	if (ISNULL(mnt_point, opts)) {
		return (-1);
	}

	*opts = NULL;

	retlist = lst_create();
	if (retlist == NULL) {
		samerrno = SE_NO_MEM;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		return (-1);
	}

	/* check for both active and configured NFS shares */
	ret = get_all_nfs(mnt_point, &nfslst, FALSE);

	if (ret != 0) {
		lst_free(retlist);
		samerrno = SE_FIND_NFS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		return (-1);
	}

	node = nfslst->head;

	while (node != NULL) {
		memset(buf, 0, sizeof (buf));

		nfsent = (nfsshare_t *)node->data;

		snprintf(buf, len, "dirname=%s", nfsent->dir_name);
		strlcat(buf, ",nfs=", len);
		if (nfsent->active) {
			strlcat(buf, "yes", len);
		} else {
			strlcat(buf, "config", len);
		}
		if (strlen(nfsent->opts) > 0) {
			strlcat(buf, ",", len);
			strlcat(buf, nfsent->opts, len);
		}
		if (strlen(nfsent->otheropts) > 0) {
			strlcat(buf, ",", len);
			strlcat(buf, nfsent->otheropts, len);
		}
		if (strlen(nfsent->desc) > 0) {
			strlcat(buf, ",desc=", len);
			strlcat(buf, nfsent->desc, len);
		}
		if (nfsent->is_mountpt) {
			strlcat(buf, ",warning=mountpt", sizeof (buf));
		}

		lst_append(retlist, strdup(buf));

		node = node->next;
	}

	lst_free_deep(nfslst);

	*opts = retlist;

	return (0);
}

/*
 *  Given a mount point, unshare all directories actively
 *  shared within that file system.
 *  outdirs is optional.  If not NULL, this function will
 *  return the list of directories that were NFS unshared.
 *  If the list is returned, it is the caller's responsibility
 *  to lst_free_deep() it.
 */
int
nfs_unshare_all(ctx_t *c, char *mnt_point, sqm_lst_t **outdirs)	/* ARGSUSED */
{
	int	ret;
	sqm_lst_t	*nfslst;

	/* get all the active shares */
	ret = get_all_nfs(mnt_point, &nfslst, TRUE);

	if (ret != 0) {
		samerrno = SE_FIND_NFS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		return (-1);
	}

	ret = nfs_modify_active(nfslst, FALSE);

	if (outdirs == NULL) {
		lst_free_deep(nfslst);
	} else {
		*outdirs = nfslst;
	}

	return (ret);
}

/*
 *  Given a mount point, unshare and unconfigure all directories
 *  actively shared within that filesystem.
 */
int
nfs_remove_all(ctx_t *c, char *mnt_point)	/* ARGSUSED */
{
	int		ret;
	sqm_lst_t		*nfslst;
	node_t		*node;
	nfsshare_t	*nfsent;
	int		errcnt = 0;

	/* get all the shares */
	ret = get_all_nfs(mnt_point, &nfslst, FALSE);

	if (ret != 0) {
		samerrno = SE_FIND_NFS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		return (-1);
	}

	node = nfslst->head;

	/* unshare the directories */
	(void) nfs_modify_active(nfslst, FALSE);

	/* now remove them from dfstab */
	while (node != NULL) {
		nfsent = (nfsshare_t *)node->data;

		if (nfsent->config) {
			nfsent->config = FALSE;

			ret = get_set_dfstab(nfsent, NULL);

			if (ret != 0) {
				errcnt++;
			}
		}

		node = node->next;
	}

	lst_free_deep(nfslst);

	if (errcnt > 0) {
		samerrno = SE_ERROR_NFS_REMOVE;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), mnt_point);
		return (-1);
	}

	return (0);
}

/*
 *  Given a list of NFS shared directories, modify the
 *  active state of all directories on that list.
 */
int
nfs_modify_active(sqm_lst_t *dirs, boolean_t share)
{
	node_t		*node;
	nfsshare_t	*nfsent;

	/* No directories is not an error */
	if (dirs == NULL) {
		return (0);
	}

	node = dirs->head;

	while (node != NULL) {
		nfsent = (nfsshare_t *)node->data;

		/*
		 * nothing we can do if share/unshare fails, so
		 * ignore it for now.
		 */
		if (share == TRUE) {
			(void) nfs_share_dir(nfsent, NULL, 0);
		} else {
			(void) nfs_unshare_dir(nfsent->dir_name, NULL, 0);
		}

		node = node->next;
	}

	return (0);
}

/*
 * **************************
 *  Private helper functions
 * **************************
 */

/*
 * fill in fs_t structure
 */
static int
fill_fs(
struct sam_fs_info *fi,	/* IN  - src info */
sqm_lst_t *vfslist,	/* IN  - used to get mnt point of unmounted fs */
fs_t **fs)		/* OUT - filled fs_t struct */
{

	int ret;
	fs_t *f = NULL;
	*fs = NULL;


	Trace(TR_MISC, "filling fs structure %s", fi->fi_name);

	f = (fs_t *)mallocer(sizeof (fs_t));
	if (f == NULL) {
		Trace(TR_ERR, "out of memory");
		goto err;
	}

	memset(f, 0, sizeof (fs_t));

	f->meta_data_disk_list = lst_create();
	if (f->meta_data_disk_list == NULL) {
		Trace(TR_ERR, "lst create failed");
		goto err;
	}
	f->data_disk_list = lst_create();
	if (f->data_disk_list == NULL) {
		Trace(TR_ERR, "lst create failed");
		goto err;
	}
	f->striped_group_list = lst_create();
	if (f->striped_group_list == NULL) {
		Trace(TR_ERR, "lst create failed");
		goto err;
	}

	Trace(TR_DEBUG, "filling fs info for %s", fi->fi_name);

	snprintf(f->fi_name, sizeof (uname_t), fi->fi_name);
	f->fs_count = fi->fs_count;
	f->mm_count = fi->mm_count;
	f->fi_eq = fi->fi_eq;
	snprintf(f->equ_type, sizeof (devtype_t), device_to_nm(fi->fi_type));

	f->fi_shared_fs = (fi->fi_config1 & MC_SHARED_FS) ? B_TRUE : B_FALSE;

	/* dau will be set later in function fill_fs_parts() */
	f->fi_state = fi->fi_state;

	/* parameters set only when mounted -- BEGIN */
	f->fi_version = fi->fi_version;
	f->fi_status = fi->fi_status;
	snprintf(f->fi_mnt_point, sizeof (upath_t), fi->fi_mnt_point);
	snprintf(f->fi_server, sizeof (upath_t), fi->fi_server);
	f->fi_capacity = fi->fi_capacity;
	f->fi_space = fi->fi_space;
	if (get_samfs_type(NULL) == QFS ||
	    !(fi->fi_config & MT_SAM_ENABLED) ||
	    (fi->fi_config & MT_SHARED_READER) ||
	    (fi->fi_status & FS_CLIENT)) {
		f->fi_archiving = FALSE;
	} else {
		f->fi_archiving = TRUE;
	}
	/* parameters set only when mounted -- END */

	/*
	 * If the mount point is not set, search the vfstab entries for
	 * this file system. If it is present, include its mount point
	 * otherwise print a trace message indicating it could not
	 * be found and continue.
	 */
	if (*f->fi_mnt_point == '\0' && vfslist != NULL) {
		node_t *tmpnd;

		tmpnd = lst_search(vfslist, f->fi_name, cmp_str_2_str_ptr);

		/*
		 * if the vfstab entry was found and contains a mount point
		 * add it to the fs_t struct.
		 */
		if (tmpnd != NULL && tmpnd->data != NULL &&
		    ((vfstab_entry_t *)(tmpnd->data))->mount_point != NULL) {

			snprintf(f->fi_mnt_point, sizeof (upath_t),
			    ((vfstab_entry_t *)tmpnd->data)->mount_point);
		} else {
			Trace(TR_MISC, "mount point not available for %s",
			    f->fi_name);
		}
	}

	Trace(TR_DEBUG, "filling mount options for %s", fi->fi_name);

	if (fill_mount_options(fi, f) == -1) {
		Trace(TR_ERR, "fill mount options failed");
		goto err;
	}

	Trace(TR_DEBUG, "filling parts info for %s", fi->fi_name);

	if ((ret = fill_fs_parts(fi, f)) == -1) {
		Trace(TR_ERR, "fill fs parts failed");
		goto err;
	}

	/*
	 * Set up the file system struct to identify this host's role.
	 */
	if (f->fi_shared_fs && !(f->fi_status & FS_NODEVS)) {
		if (!(f->fi_status & FS_MOUNTED) ||
		    strcmp(f->equ_type, "ms") == 0) {

			/*
			 * For unmounted set the shared fs status flags from
			 * reading the files.
			 * For mounted shared ms file systems we also must
			 * do this to properly configure the nodevs flag
			 * to indicate if this is a client.
			 */
			if (set_shared_fs_status_flags(f) != 0) {
				ret = -2;
			}
		}
	}

	*fs = f;

	Trace(TR_MISC, "fill fs done (%d)", ret);
	return (ret);

err:
	if (f) {
		free_fs(f);
	}

	Trace(TR_ERR, "fill fs failed: %s", samerrmsg);
	return (-1);
}


/*
 * fill in fs_t mount_options field
 */
static int
fill_mount_options(
struct sam_fs_info *fi,	/* IN  - src info */
fs_t *f)				/* IN/OUT - filled fs_t */
{
	mount_options_t	*mnt_options = NULL;

	Trace(TR_MISC, "filling mount options");

	mnt_options = (mount_options_t	*)
	    mallocer(sizeof (mount_options_t));
	if (mnt_options == NULL) {
		Trace(TR_ERR, "out of memory");
		goto err;
	}

	/* zero out the struct */
	memset(mnt_options, 0, sizeof (mount_options_t));

	/* general filesystem mount options */
	mnt_options->no_mnttab_entry =
	    (fi->fi_mflag & MS_NOMNTTAB) ? B_TRUE : B_FALSE;

	mnt_options->global =
	    (fi->fi_mflag & MS_GLOBAL) ? B_TRUE : B_FALSE;

	mnt_options->overlay = (fi->fi_mflag & MS_OVERLAY) ? B_TRUE : B_FALSE;
	mnt_options->readonly = (fi->fi_mflag & MS_RDONLY) ? B_TRUE : B_FALSE;

	/* SAM-FS, QFS and SAM-QFS mount options */
	mnt_options->sync_meta = fi->fi_sync_meta;
	mnt_options->no_suid = (fi->fi_mflag & MS_NOSUID) ? B_TRUE : B_FALSE;
	mnt_options->stripe = fi->fi_stripe[DD];
	mnt_options->trace = (fi->fi_config & MT_TRACE) ? B_TRUE : B_FALSE;
	mnt_options->quota = (fi->fi_config & MT_QUOTA) ? B_TRUE : B_FALSE;

	mnt_options->rd_ino_buf_size = fi->fi_rd_ino_buf_size;
	mnt_options->wr_ino_buf_size = fi->fi_wr_ino_buf_size;
	mnt_options->worm_capable = fi->fi_config & MT_WORM ? B_TRUE : B_FALSE;
	mnt_options->gfsid = (fi->fi_config & MT_GFSID) ? B_TRUE : B_FALSE;


	/* I/O mount options */
	mnt_options->io_opts.dio_rd_consec = fi->fi_dio_rd_consec;
	mnt_options->io_opts.dio_rd_form_min = fi->fi_dio_rd_form_min;
	mnt_options->io_opts.dio_rd_ill_min = fi->fi_dio_rd_ill_min;
	mnt_options->io_opts.dio_wr_consec = fi->fi_dio_wr_consec;
	mnt_options->io_opts.dio_wr_form_min = fi->fi_dio_wr_form_min;
	mnt_options->io_opts.dio_wr_ill_min = fi->fi_dio_wr_ill_min;

	mnt_options->io_opts.forcedirectio =
	    (fi->fi_config & MT_DIRECTIO) ? B_TRUE : B_FALSE;

	mnt_options->io_opts.sw_raid =
	    (fi->fi_config & MT_SOFTWARE_RAID) ? B_TRUE : B_FALSE;

	mnt_options->io_opts.flush_behind = fi->fi_flush_behind / 1024;
	mnt_options->io_opts.readahead = fi->fi_readahead / (long long) 1024;

	mnt_options->io_opts.writebehind =
	    fi->fi_writebehind / (long long) 1024;

	mnt_options->io_opts.wr_throttle =
	    fi->fi_wr_throttle / (long long) 1024;

	mnt_options->io_opts.forcenfsasync =
	    (fi->fi_config & MT_NFSASYNC) ? B_TRUE : B_FALSE;


	/* SAM-FS and SAM-QFS related mount options */
	mnt_options->sam_opts.high = fi->fi_high;
	mnt_options->sam_opts.low = fi->fi_low;
	mnt_options->sam_opts.partial = fi->fi_partial;
	mnt_options->sam_opts.maxpartial = fi->fi_maxpartial;
	/* adding unsigned type cast */
	mnt_options->sam_opts.partial_stage =
	    (uint32_t)fi->fi_partial_stage / 1024;
	mnt_options->sam_opts.stage_n_window =
	    (uint32_t)fi->fi_stage_n_window / 1024;
	mnt_options->sam_opts.stage_retries = fi->fi_stage_retries;

	mnt_options->sam_opts.stage_flush_behind =
	    fi->fi_stage_flush_behind / 1024;

	mnt_options->sam_opts.hwm_archive =
	    (fi->fi_config & MT_HWM_ARCHIVE) ? B_TRUE : B_FALSE;

	mnt_options->sam_opts.archive =
	    (fi->fi_config & MT_SAM_ENABLED) ? B_TRUE : B_FALSE;

	mnt_options->sam_opts.arscan =
	    (fi->fi_config & MT_ARCHIVE_SCAN) ? B_TRUE : B_FALSE;


	mnt_options->sam_opts.oldarchive =
	    (fi->fi_config & MT_OLD_ARCHIVE_FMT) ? B_TRUE : B_FALSE;


	/* shared QFS mount options */
	mnt_options->sharedfs_opts.shared =
	    (fi->fi_config & MT_SHARED_MO) ? B_TRUE : B_FALSE;

	mnt_options->sharedfs_opts.bg =
	    (fi->fi_config & MT_SHARED_BG) ? B_TRUE : B_FALSE;


	mnt_options->sharedfs_opts.retry = fi->fi_retry;
	mnt_options->sharedfs_opts.minallocsz =
	    fi->fi_minallocsz / (int64_t)1024;

	mnt_options->sharedfs_opts.maxallocsz =
	    fi->fi_maxallocsz / (int64_t)1024;

	mnt_options->sharedfs_opts.rdlease = fi->fi_lease[RD_LEASE];
	mnt_options->sharedfs_opts.wrlease = fi->fi_lease[WR_LEASE];
	mnt_options->sharedfs_opts.aplease = fi->fi_lease[AP_LEASE];

	mnt_options->sharedfs_opts.mh_write =
	    (fi->fi_config & MT_MH_WRITE) ? B_TRUE : B_FALSE;

	mnt_options->sharedfs_opts.nstreams = fi->fi_nstreams;
	mnt_options->sharedfs_opts.meta_timeo = fi->fi_meta_timeo;
	mnt_options->sharedfs_opts.lease_timeo = fi->fi_lease_timeo;

	mnt_options->sharedfs_opts.soft =
	    (fi->fi_config & MT_SHARED_SOFT) ? B_TRUE : B_FALSE;

	/* multireader filesystem mount options */
	mnt_options->multireader_opts.writer =
	    (fi->fi_config & MT_SHARED_WRITER) ? B_TRUE : B_FALSE;

	mnt_options->multireader_opts.reader =
	    (fi->fi_config & MT_SHARED_READER) ? B_TRUE : B_FALSE;

	mnt_options->multireader_opts.invalid = fi->fi_invalid;

	mnt_options->multireader_opts.refresh_at_eof =
	    (fi->fi_config & MT_REFRESH_EOF) ? B_TRUE : B_FALSE;

	/* QFS mount options */
	mnt_options->qfs_opts.qwrite =
	    (fi->fi_config & MT_QWRITE) ? B_TRUE : B_FALSE;

	mnt_options->qfs_opts.mm_stripe = fi->fi_stripe[MM];

	/* post 4.2 options */
	mnt_options->post_4_2_opts.def_retention = fi->fi_def_retention;
	mnt_options->post_4_2_opts.abr =
	    (fi->fi_config & MT_ABR_DATA) ? B_TRUE : B_FALSE;
	mnt_options->post_4_2_opts.dmr =
	    (fi->fi_config & MT_DMR_DATA) ? B_TRUE : B_FALSE;
	mnt_options->post_4_2_opts.dio_szero =
	    (fi->fi_config & MT_ZERO_DIO_SPARSE) ? B_TRUE : B_FALSE;
	mnt_options->post_4_2_opts.cattr =
	    (fi->fi_config & MT_CONSISTENT_ATTR) ? B_TRUE : B_FALSE;

	/* release 4.6 options */
	mnt_options->rel_4_6_opts.worm_emul =
	    (fi->fi_config & MT_WORM_EMUL) ? B_TRUE : B_FALSE;
	mnt_options->rel_4_6_opts.worm_lite =
	    (fi->fi_config & MT_WORM_LITE) ? B_TRUE : B_FALSE;
	mnt_options->rel_4_6_opts.emul_lite =
	    (fi->fi_config & MT_EMUL_LITE) ? B_TRUE : B_FALSE;
	mnt_options->rel_4_6_opts.cdevid =
	    (fi->fi_config & MT_CDEVID) ? B_TRUE : B_FALSE;
	mnt_options->rel_4_6_opts.clustermgmt =
	    (fi->fi_config1 & MC_CLUSTER_MGMT) ? B_TRUE : B_FALSE;
	mnt_options->rel_4_6_opts.clusterfastsw =
	    (fi->fi_config1 & MC_CLUSTER_FASTSW) ? B_TRUE : B_FALSE;
	mnt_options->rel_4_6_opts.noatime =
	    (fi->fi_config & MT_NOATIME) ? B_TRUE : B_FALSE;
	mnt_options->rel_4_6_opts.atime = fi->fi_atime;
	mnt_options->rel_4_6_opts.min_pool = fi->fi_min_pool;

	/* release 5.0 options */
	mnt_options->rel_5_0_opts.obj_width = fi->fi_obj_width;
	mnt_options->rel_5_0_opts.obj_depth =
	    fi->fi_obj_depth / (long long)1024;
	mnt_options->rel_5_0_opts.obj_pool = fi->fi_obj_pool;
	mnt_options->rel_5_0_opts.obj_sync_data = fi->fi_obj_sync_data;

	mnt_options->rel_5_0_opts.logging =
	    (fi->fi_config1 & MC_LOGGING) ? B_TRUE : B_FALSE;
	mnt_options->rel_5_0_opts.sam_db =
	    (fi->fi_config1 & MC_SAM_DB) ? B_TRUE : B_FALSE;
	mnt_options->rel_5_0_opts.xattr =
	    (fi->fi_config1 & MC_NOXATTR) ? B_FALSE : B_TRUE;

	f->mount_options = mnt_options;

	Trace(TR_MISC, "fill mount options done");
	return (0);

err:
	if (mnt_options) {
		free(mnt_options);
	}

	Trace(TR_ERR, "fill mount options failed: %s", samerrmsg);
	return (-1);
}


/*
 * fill in fs_t disk list fields
 */
static int
fill_fs_parts(
struct sam_fs_info *fi,	/* IN - src info */
fs_t *f)				/* IN/OUT - filled fs_t */
{
	int i, ret = 0;
	struct	sam_fs_part *ptarray = NULL;
	struct	sam_fs_part *pt;
	char	*devrname = NULL;
	boolean_t need_client = B_FALSE;

	Trace(TR_MISC, "filling fs parts for %s", f->fi_name);

	ptarray = (struct sam_fs_part *)
	    mallocer(fi->fs_count * sizeof (struct sam_fs_part));
	if (ptarray == NULL) {
		Trace(TR_ERR, "out of memory");
		goto err;
	}

	Trace(TR_DEBUG, "getting fs parts");

	if (GetFsParts(fi->fi_name, fi->fs_count, ptarray) == -1) {
		samerrno = SE_GET_FS_PARTS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), fi->fi_name);
		Trace(TR_ERR, "get fs parts failed");
		goto err;
	}

	for (i = 0; i < fi->fs_count; i++) {

		pt = ptarray + i;

		if (fill_fs_part(pt, f) == -1) {
			Trace(TR_ERR, "fill fs part failed");
			// set dau to 2 to indicate bad devices
			f->dau = 2;
			ret = -2;
		}

		/*
		 * get dau from the sblk info on the first partition unless
		 * the first partition is a nodev
		 */
		if (i == 0 || (need_client)) {
			if (strcmp(pt->pt_name, "nodev") == 0 ||
			    (strcmp(f->equ_type, "mb") == 0 &&
			    strstr(pt->pt_name, "/dev/osd") != NULL)) {
				f->fi_status |= FS_CLIENT;
				f->fi_status |= FS_NODEVS;

				need_client = B_TRUE;

				/* shared client, DAU info not available */
				continue;
			} else {
				dsk2rdsk(pt->pt_name, &devrname);
				if (get_info_from_sblk(devrname, f) == -1) {
					Trace(TR_ERR,
					    "get sblk info failed");
					ret = -2;
				}
				need_client = B_FALSE;
				if (devrname != NULL) {
					free(devrname);
				}
			}
		}
	}

	free(ptarray);

	Trace(TR_MISC, "fill fs parts done (%d)", ret);
	return (ret);

err:
	if (ptarray) {
		free(ptarray);
	}

	Trace(TR_ERR, "fill fs parts failed: %s", samerrmsg);
	return (-1);
}


/*
 * insert one disk into fs_t disk list or striped group list
 */
static int
fill_fs_part(
struct sam_fs_part *pt,	/* IN - src info */
fs_t *f)				/* IN/OUT - filled fs_t */
{
	striped_group_t *group = NULL;
	disk_t *disk = NULL;
	sqm_lst_t *lst;
	char   *pname;

	Trace(TR_MISC, "filling fs part");

	/*
	 * Note that is_disk returns true for striped groups but not
	 * osd groups.
	 */
	if (is_disk(pt->pt_type) || is_osd_group(pt->pt_type)) {
		pname = device_to_nm(pt->pt_type);
		if (is_stripe_group(pt->pt_type) || is_osd_group(pt->pt_type)) {
			group = find_striped_group(f, pname);
			if (group == NULL) {
				Trace(TR_ERR, "find striped group failed");
				goto err;
			}
			lst = group->disk_list;
		} else if (pt->pt_type == DT_META) {
			lst = f->meta_data_disk_list;
		} else {
			lst = f->data_disk_list;
		}
	} else {
		samerrno = SE_INVALID_FS_DEVICE_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), pt->pt_type);
			Trace(TR_ERR, "invalid fs device");
		goto err;
	}

	Trace(TR_DEBUG, "creating disk");

	disk = (disk_t *)mallocer(sizeof (disk_t));
	if (disk == NULL) {
		Trace(TR_ERR, "out of memory");
		goto err;
	}
	memset(disk, 0, sizeof (disk_t));

	/* base_dev info */
	snprintf(disk->base_info.name, sizeof (upath_t), pt->pt_name);

	disk->base_info.eq = pt->pt_eq;

	snprintf(disk->base_info.equ_type, sizeof (devtype_t),
	    device_to_nm(pt->pt_type));

	snprintf(disk->base_info.set, sizeof (uname_t), f->fi_name);

	disk->base_info.fseq = f->fi_eq;

	disk->base_info.state = pt->pt_state;

	disk->base_info.additional_params[0] = '\0';

	/* au info */
	snprintf(disk->au_info.path, sizeof (upath_t), pt->pt_name);

	if (strncmp(pt->pt_name, "/dev/md/", 8) == 0) {
		disk->au_info.type = AU_SVM;
	} else if (strncmp(pt->pt_name, "/dev/vx/", 8) == 0) {
		disk->au_info.type = AU_VXVM;
	} else if (strncmp(pt->pt_name, "/dev/osd/", 9) == 0) {
		disk->au_info.type = AU_OSD;
	} else {
		disk->au_info.type = AU_SLICE;
	}

	disk->au_info.capacity = pt->pt_capacity;
	disk->au_info.fsinfo[0] = '\0';
	disk->au_info.raid = NULL;
	add_scsi_info(&disk->au_info);

	disk->freespace = pt->pt_space;

	if (lst_append(lst, disk) == -1) {
		Trace(TR_ERR, "lst append failed");
		goto err;
	}

	Trace(TR_MISC, "fill fs part done");
	return (0);

err:
	if (disk) {
		free(disk);
	}

	Trace(TR_ERR, "fill fs part failed: %s", samerrmsg);
	return (-1);
}


/*
 * find the named striped group in the given fs_t,
 * or create one if not found
 */
static striped_group_t *
find_striped_group(
fs_t *f,			/* IN - fs_t */
const char *gname)	/* IN - group name */
{
	striped_group_t *group = NULL;
	sqm_lst_t *lst = f->striped_group_list;
	boolean_t found = FALSE;

	Trace(TR_MISC, "finding striped group %s", gname);

	if (lst->length > 0) {
		node_t *node;
		Trace(TR_DEBUG, "searching group list");
		for (node = lst->head; node != NULL; node = node->next) {
			group = (striped_group_t *)node->data;
			if (strncmp(gname, group->name, sizeof (devtype_t))
			    == 0) {
				found = TRUE;
				break;
			}
		}
	}

	if (!found) {
		Trace(TR_DEBUG, "group not found, creating a new one");

		group = (striped_group_t *)
		    mallocer(sizeof (striped_group_t));

		if (group == NULL) {
			Trace(TR_ERR, "out of memory");
			goto err;
		}

		group->disk_list = lst_create();

		if (group->disk_list == NULL) {
			Trace(TR_ERR, "lst create failed");
			goto err;
		}

		snprintf(group->name, sizeof (devtype_t), gname);

		if (lst_append(lst, group) == -1) {
			Trace(TR_ERR, "lst append failed");
			goto err;
		}
	}

	Trace(TR_MISC, "found striped group");
	return (group);

err:
	if (group) {
		free_striped_group(group);
	}

	Trace(TR_ERR, "find striped group failed: %s", samerrmsg);
	return (NULL);
}


/*
 * get large dau size and fs creation time from reading sblk info from disk.
 */

#define	BUF_SIZE	1024	/* buffer size used to read sector info */

static int
get_info_from_sblk(
char *rdsk,		/* IN - path to rdsk */
fs_t *f)		/* OUT - fs_t with large dau and ctime filled out */
{
	int fd = -1;
	struct sam_sbinfo *sp;
	struct sam_sblk sblk;

	Trace(TR_MISC, "getting info from sblk");
	if (ISNULL(rdsk, f)) {
		goto err;
	}

	/* get the super block from the device */
	if (sam_dev_sb_get(rdsk, &sblk, &fd) != 0) {
		samerrno = SE_DEV_HAS_NO_SUPERBLOCK;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), rdsk, f->fi_name);
		goto err;
	}

	close(fd);
	sp = &sblk.info.sb;

	/*
	 * If the file system is shared it is possible that it was created
	 * on a system with reversed byte order. Check for this and fix the
	 * order before populating the creation time and dau.
	 * Ignore this if the byte swapping fails- it will only result in bad
	 * information in the dau and ctime fields.
	 */
	if (sp->magic == SAM_MAGIC_V1_RE || sp->magic == SAM_MAGIC_V2_RE ||
	    sp->magic == SAM_MAGIC_V2A_RE) {
		if ((byte_swap_sb(&sblk, sizeof (sblk))) != 0) {
			samerrno = SE_CANT_FIX_BYTE_ORDER;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CANT_FIX_BYTE_ORDER));
			Trace(TR_ERR, "Couldn't byteswap superblock");
			goto err;
		}
	}

	/* check that the super block name matches the file system name */
	if (strcmp(f->fi_name, sp->fs_name) != 0) {
		samerrno = SE_WRONG_FS_NAME;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    rdsk, f->fi_name);
		goto err;
	}

	f->ctime = (time_t)sp->init;
	f->dau = sp->dau_blks[LG];

	Trace(TR_MISC, "got dau size %d, and ctime %lx for %s", f->dau,
	    f->ctime, f->fi_name);
	return (0);

err:

	/* set dau to 1 as a flag to indicate reading super block failed. */
	f->dau = 1;

	Trace(TR_ERR, "get info from sblk failed: %s", samerrmsg);
	return (-1);
}


/*
 * check if the given sam_errno_t is a fatal error.
 */
static boolean_t
is_fatal_error(
sam_errno_t sam_errno) /* IN - error number to check */
{
	if (sam_errno == SE_NO_MEM ||
	    sam_errno == SE_NULL_PARAMETER ||
	    sam_errno == SE_OPENDISK_FAILED ||
	    sam_errno == SE_LLSEEK_FAILED ||
	    sam_errno == SE_READ_FAILED ||
	    sam_errno == SE_READ_INCOMPLETE) {
		return (TRUE);
	} else {
		return (FALSE);
	}
}

/* NFS Server helper funcs */

static int
get_set_dfstab(nfsshare_t *innfs, sqm_lst_t **list)
{
	sqm_lst_t		*dfslst = NULL;
	FILE		*dfstab;
	FILE		*tmpdfstab = NULL;
	int		fd;
	char		tmptabnam[MAXPATHLEN];
	char		buf[sizeof (nfsshare_t)];
	char		optbuf[sizeof (nfsshare_t)];
	char		*bufp;
	nfsshare_t	*nfsent;
	int		len;
	int		restlen;
	boolean_t	toolong = FALSE;
	off64_t		offset;
	int		ret = 0;
	char		*ptr;
	char		*rest;


	if ((list == NULL) && (innfs == NULL)) {
		return (1);
	}

	memset(buf, 0, sizeof (buf));

	dfstab = fopen64(DFSTAB, "r");
	if (dfstab == NULL) {
		return (1);
	}

	if (list) {
		dfslst = lst_create();
		if (dfslst == NULL) {
			fclose(dfstab);
			return (1);
		}
	}

	if (innfs) {
		ret = mk_wc_path(DFSTAB, tmptabnam, sizeof (tmptabnam));

		if (ret != 0) {
			fclose(dfstab);
			if (dfslst) {
				lst_free(dfslst);
			}
			return (1);
		}
		if ((fd = open(tmptabnam, O_RDWR|O_CREAT|O_TRUNC, 0644))
		    != -1) {
			tmpdfstab = fdopen(fd, "w+");
		}
		if (tmpdfstab == NULL) {
			fclose(dfstab);
			if (dfslst) {
				lst_free(dfslst);
			}
			unlink(tmptabnam);
			return (1);
		}
		offset = ftello64(tmpdfstab);
	}

	while (fgets(buf, sizeof (buf), dfstab) != NULL) {
		if (tmpdfstab) {
			fputs(buf, tmpdfstab);
		}
		if (buf[0] == '#') {	/* comment */
			if (tmpdfstab) {
				offset = ftello64(tmpdfstab);
			}
			continue;
		}
		/* try to get whole entry into the buf */
		restlen = sizeof (buf);
		bufp = buf;

		while (bufp[strlen(bufp) - 2] == '\\') {
			len = strlen(bufp) - 2;
			restlen -= len;
			if (restlen == 0) {
				toolong = TRUE;
				break;
			}

			bufp += len;
			*bufp = '\0';

			if (fgets(bufp, restlen, dfstab) == NULL) {
				toolong = TRUE;
				break;
			} else {
				if (tmpdfstab) {
					fputs(buf, tmpdfstab);
				}
			}
		}

		if (toolong) {		/* malformed entry, skip it */
			if (tmpdfstab) {
				offset = ftello64(tmpdfstab);
			}
			toolong = FALSE;	/* reset */
			continue;
		}

		bufp = buf;		/* reset bufp */

		ptr = strtok_r(buf, WHITESPACE, &rest);

		/* make sure this is a share command */
		if ((ptr == NULL) || (strcmp(ptr, "share") != 0)) {
			if (tmpdfstab) {
				offset = ftello64(tmpdfstab);
			}
			continue;
		}

		nfsent = mallocer(sizeof (nfsshare_t));
		if (nfsent == NULL) {
			if (dfslst) {
				lst_free_deep(dfslst);
			}
			if (tmpdfstab) {
				unlink(tmptabnam);
				fclose(tmpdfstab);
			}
			fclose(dfstab);
			return (NULL);
		}

		memset(nfsent, 0, sizeof (nfsshare_t));
		memset(optbuf, 0, sizeof (optbuf));

		while (ptr != NULL) {
			ptr = strtok_r(NULL, WHITESPACE, &rest);
			/* skip over filesystem type */
			if (strncmp(ptr, "-F", 2) == 0) {
				ptr += 2;
				if (*ptr == '\0') {
					ptr = strtok_r(NULL, WHITESPACE, &rest);
				}
				if ((ptr == NULL) ||
				    (strcmp(ptr, "nfs") != 0)) {
					/* not an NFS share */
					break;
				}
				continue;
			}

			/* description */
			if (strncmp(ptr, "-d", 2) == 0) {
				ptr += 2;
				if (*ptr == '\0') {
					ptr = strtok_r(NULL, WHITESPACE, &rest);
				}
				if (ptr == NULL) {
					continue;
				}

				/*
				 * if it's a quoted string, make sure we
				 * get all the words.
				 */
				if ((*ptr == '"') || (*ptr == '\'')) {
					int  desclen = 0;
					char sq = *ptr;

					/* save room for quotes & a space */
					int tmplen = sizeof (nfsent->desc) - 4;

					nfsent->desc[desclen] = *ptr;
					desclen++;
					ptr++;

					while ((desclen < tmplen) &&
					    (*ptr != sq)) {
						if (*ptr == '\0') {
							nfsent->desc[desclen] =
							    ' ';
							desclen++;
							ptr = strtok_r(NULL,
							    WHITESPACE, &rest);
							if (ptr == NULL) {
								break;
							}
						}

						nfsent->desc[desclen] =
						    *ptr;
						desclen++;
						ptr++;
					}

					/*
					 * end the string with the same
					 * quote character as we started with
					 */
					nfsent->desc[desclen] = sq;
				} else {
					strlcpy(nfsent->desc, ptr,
					    sizeof (nfsent->desc));
				}

				continue;
			}

			/* nfs options */
			if (strncmp(ptr, "-o", 2) == 0) {
				ptr += 2;

				if (*ptr == '\0') {
					ptr = strtok_r(NULL, WHITESPACE, &rest);
				}

				while (ptr != NULL) {
					strlcat(optbuf, ptr, sizeof (optbuf));
					if (ptr[strlen(ptr)-1] == ',') {
						ptr = strtok_r(NULL,
						    WHITESPACE, &rest);
					} else {
						/* that's all the opts */
						break;
					}
				}

				if (strlen(optbuf) > 0) {
					parse_nfssvropts(optbuf, nfsent);
				}

				continue;
			}

			/* NFS shared directory */
			if (*ptr == '/') {
				strlcpy(nfsent->dir_name, ptr,
				    sizeof (nfsent->dir_name));
				/* last part of the entry */
				break;
			}
		}

		/* make sure we found something */
		if (nfsent->dir_name[0] == '\0') {
			if (tmpdfstab) {
				offset = ftello64(tmpdfstab);
			}
			free(nfsent);
			continue;
		}

		nfsent->config = TRUE;

		if (innfs) {
			if (strcmp(innfs->dir_name, nfsent->dir_name) == 0) {
				/* delete this entry from the tmp file */
				if (tmpdfstab) {
					fseeko(tmpdfstab, offset, SEEK_SET);
					ftruncate(fileno(tmpdfstab), offset);
				}
				/*
				 * preserve any options not set in the
				 * new entry
				 */
				if ((nfsent->otheropts[0] != '\0') &&
				    (innfs->otheropts[0] == '\0')) {
						innfs->hasopts = TRUE;
						strcpy(innfs->otheropts,
						    nfsent->otheropts);
				}
				if ((nfsent->optsec[0] != '\0') &&
				    (innfs->optsec[0] == '\0')) {
						innfs->hasopts = TRUE;
						strcpy(innfs->optsec,
						    nfsent->optsec);
				}
				if ((nfsent->desc[0] != '\0') &&
				    (innfs->desc[0] == '\0')) {
					strcpy(innfs->desc, nfsent->desc);
				}
			}
		}

		if (tmpdfstab) {
			offset = ftello64(tmpdfstab);
		}

		if (dfslst) {
			lst_append(dfslst, nfsent);
		} else {
			free(nfsent);
		}
	}

	/* done with dfstab */
	fclose(dfstab);

	/* write out the new or updated entry */
	if (tmpdfstab) {
		if (innfs->config) {
			char *tmpstr;

			tmpstr = create_nfsshare_cmd(innfs);
			if (tmpstr) {
				fprintf(tmpdfstab, tmpstr);
				fprintf(tmpdfstab, "\n");
				free(tmpstr);
			} else {
				/* malformed entry */
				ret = 1;
			}
		}
		fclose(tmpdfstab);
		/* don't change if error */
		if (ret == 0) {
			backup_cfg(DFSTAB);
			cp_file(tmptabnam, DFSTAB);
			unlink(tmptabnam);
		}
		free(tmptabnam);
	}

	if (list) {
		*list = dfslst;
	}

	return (ret);
}

static sqm_lst_t *
parse_sharetab(void)
{
	sqm_lst_t		*sharelst = NULL;
	FILE		*sharetab;
	char		buf[sizeof (nfsshare_t)];
	char		optbuf[sizeof (nfsshare_t)];
	nfsshare_t	*nfsent;
	int		ret;
	int		num;
	char		junk[MAXPATHLEN];
	char		fstype[128] = {0};
	struct statvfs64  statbuf;

	memset(buf, 0, sizeof (buf));

	sharetab = fopen64(SHARETAB, "r");

	if (sharetab == NULL) {
		return (NULL);
	}

	sharelst = lst_create();
	if (sharelst == NULL) {
		fclose(sharetab);
		return (NULL);
	}

	while (fgets(buf, sizeof (buf), sharetab)) {
		nfsent = mallocer(sizeof (nfsshare_t));
		if (nfsent == NULL) {
			lst_free_deep(sharelst);
			fclose(sharetab);
			return (NULL);
		}

		memset(nfsent, 0, sizeof (nfsshare_t));

		num = sscanf(buf, "%s\t%s\t%s\t%s\t%s",
		    nfsent->dir_name, junk, fstype, optbuf, nfsent->desc);

		if (num >= 3) {
			if (strcasecmp(fstype, "nfs") != 0) {
				free(nfsent);
				continue;
			}

			nfsent->active = TRUE;
			ret = statvfs64(nfsent->dir_name, &statbuf);
			if (ret != 0) {
				free(nfsent);
				continue;
			}

			nfsent->fsid = statbuf.f_fsid;

			if (strlen(buf) > 0) {
				parse_nfssvropts(optbuf, nfsent);
			}

			lst_append(sharelst, nfsent);
		} else {
			free(nfsent);
		}
	}

	fclose(sharetab);
	return (sharelst);
}

static char *nfssvropts[] = {
#define	FSNFS_DIR	0
	"dirname",
#define	FSNFS_NFS	1
	"nfs",
#define	FSNFS_RO	2
	"ro",
#define	FSNFS_RW	3
	"rw",
#define	FSNFS_ROOT	4
	"root",
#define	FSNFS_OTHER	5
	"otheropts",
#define	FSNFS_DESC	6
	"desc",
#define	FSNFS_SEC	7
	"sec",
	NULL };

static int
parse_nfssvropts(char *optbuf, nfsshare_t *nfsent)
{
	char   *value;
	size_t len;

	if ((strlen(optbuf) == 0) || (nfsent == NULL)) {
		return (1);
	}


	while (*optbuf != '\0') {
		switch (getsubopt(&optbuf, nfssvropts, &value)) {
			case FSNFS_DIR:
				if ((value == NULL) || (strlen(value) == 0)) {
					return (1);
				}
				strlcpy(nfsent->dir_name, value,
				    sizeof (nfsent->dir_name));
				break;
			case FSNFS_NFS:
				if ((value) && (strlen(value) > 0)) {
					if (strcasecmp(value, "yes") == 0) {
						nfsent->config = TRUE;
						nfsent->active = TRUE;
					}
					if (strcasecmp(value, "config") == 0) {
						nfsent->config = TRUE;
					}
				}
				break;
			case FSNFS_RO:
				nfsent->hasopts = TRUE;
				if (strlen(nfsent->opts) > 0) {
					strlcat(nfsent->opts, ",",
					    sizeof (nfsent->opts));
				}
				strlcat(nfsent->opts, nfssvropts[FSNFS_RO],
				    sizeof (nfsent->opts));
				if ((value) && (strlen(value) > 0)) {
					strlcat(nfsent->opts, "=",
					    sizeof (nfsent->opts));
					strlcat(nfsent->opts, value,
					    sizeof (nfsent->opts));
				}
				break;
			case FSNFS_RW:
				nfsent->hasopts = TRUE;
				if (strlen(nfsent->opts) > 0) {
					strlcat(nfsent->opts, ",",
					    sizeof (nfsent->opts));
				}
				strlcat(nfsent->opts, nfssvropts[FSNFS_RW],
				    sizeof (nfsent->opts));
				if ((value) && (strlen(value) > 0)) {
					strlcat(nfsent->opts, "=",
					    sizeof (nfsent->opts));
					strlcat(nfsent->opts, value,
					    sizeof (nfsent->opts));
				}
				break;
			case FSNFS_ROOT:
				if ((value) && (strlen(value) > 0)) {
					nfsent->hasopts = TRUE;
					if (strlen(nfsent->opts) > 0) {
						strlcat(nfsent->opts, ",",
						    sizeof (nfsent->opts));
					}
					strlcat(nfsent->opts,
					    nfssvropts[FSNFS_ROOT],
					    sizeof (nfsent->opts));
					strlcat(nfsent->opts, "=",
					    sizeof (nfsent->opts));
					strlcat(nfsent->opts, value,
					    sizeof (nfsent->opts));
				}
				break;
			case FSNFS_SEC:
				if ((value) && (strlen(value) > 0)) {
					nfsent->hasopts = TRUE;
					nfsent->hasopts = TRUE;
					strlcat(nfsent->optsec,
					    nfssvropts[FSNFS_SEC],
					    sizeof (nfsent->optsec));
					strlcat(nfsent->optsec, "=",
					    sizeof (nfsent->optsec));
					strlcat(nfsent->optsec, value,
					    sizeof (nfsent->optsec));
				}
				break;
			case FSNFS_OTHER:
				/*
				 *  This should only be set if called
				 *  from set_nfs_opts()
				 */
				if ((value) && (strlen(value) > 0)) {
					nfsent->hasopts = TRUE;
					strlcpy(nfsent->otheropts, value,
					    sizeof (nfsent->otheropts));
				}
				break;
			case FSNFS_DESC:
				if ((value) && (strlen(value) > 0)) {
					strlcpy(nfsent->desc, value,
					    sizeof (nfsent->desc));
				}

				break;
			default:
				strlcat(nfsent->otheropts, value,
				    sizeof (nfsent->otheropts));
				strlcat(nfsent->otheropts, ",",
				    sizeof (nfsent->otheropts));
				break;
		}
	}

	len = strlen(nfsent->otheropts);

	if ((len > 0) && (nfsent->otheropts[len-1] == ',')) {
		nfsent->otheropts[len-1] = '\0';
	}

	return (0);
}

/*
 *  Utility function to merge lists of NFS shares from dfstab and
 *  sharetab.  Use realpath() to ensure we don't erroneously report
 *  the same share twice.
 */
static int
nfssvr_mrgcomp(void* node1, void* node2)
{
	nfsshare_t	*a = (nfsshare_t *)node1;
	nfsshare_t	*b = (nfsshare_t *)node2;
	char		real_a[MAXPATHLEN + 1];
	char		real_b[MAXPATHLEN + 1];
	char		*ptr_a = NULL;
	char		*ptr_b = NULL;

	ptr_a = realpath(a->dir_name, real_a);
	if (ptr_a == NULL) {
		ptr_a = a->dir_name;
	}
	ptr_b = realpath(b->dir_name, real_b);
	if (ptr_b == NULL) {
		ptr_b = b->dir_name;
	}

	if (strcmp(ptr_a, ptr_b) == 0) {
		if (b->config) {
			a->config = TRUE;
		}
		return (0);
	}

	return (1);
}

static int
is_fs_nfsshared(sqm_lst_t *fslst, fs_t *fs)
{
	sqm_lst_t	*nfslst = NULL;
	node_t	*nfsent;
	node_t	*fsent = NULL;
	node_t	tmpfsent = {0};
	fs_t	*f;
	struct statvfs64 statbuf;
	nfsshare_t *nfsdata;
	int	ret;

	nfslst = parse_sharetab();

	if (nfslst == NULL) {
		/* No shares, not an error */
		return (0);
	}

	if (fslst != NULL) {
		fsent = fslst->head;
	} else if (fs != NULL) {
		tmpfsent.next = NULL;
		tmpfsent.data = fs;
		fsent = &tmpfsent;
	}

	/* if fs is mounted, check if any part of it is NFS shared */
	while (fsent != NULL) {
		f = (fs_t *)fsent->data;

		if (f->fi_status & FS_MOUNTED) {

			ret = statvfs64(f->fi_mnt_point, &statbuf);
			if (ret == 0) {
				nfsent = nfslst->head;
				while (nfsent != NULL) {
					nfsdata = (nfsshare_t *)(nfsent->data);
					if (nfsdata->fsid == statbuf.f_fsid) {
						strcpy(f->nfs, "yes");
						break;
					}
					nfsent = nfsent->next;
				}
			}
		}
		fsent = fsent->next;
	}
	lst_free_deep(nfslst);

	return (0);
}

static char *
create_nfsshare_cmd(nfsshare_t *nfsent)
{
	char	buf[sizeof (nfsshare_t)];
	int	len = sizeof (nfsshare_t);
	int	opts = 0;

	if ((nfsent == NULL) || (strlen(nfsent->dir_name) == 0)) {
		return (NULL);
	}

	strlcpy(buf, "share -F nfs", len);

	if (nfsent->hasopts) {
		strlcat(buf, " -o ", len);
		if (strlen(nfsent->optsec) > 0) {
			opts = 1;
			strlcat(buf, nfsent->optsec, len);
		}
		if (strlen(nfsent->opts) > 0) {
			if (opts) {
				strlcat(buf, ",", len);
			} else {
				opts = 1;
			}
			strlcat(buf, nfsent->opts, len);
		}
		if (strlen(nfsent->otheropts) > 0) {
			if (opts) {
				strlcat(buf, ",", len);
			}
			strlcat(buf, nfsent->otheropts, len);
		}
	}

	if (strlen(nfsent->desc) > 0) {
		strlcat(buf, " -d ", len);
		strlcat(buf, nfsent->desc, len);
	}

	strlcat(buf, " ", len);
	strlcat(buf, nfsent->dir_name, len);

	return (strdup(buf));
}

/*
 * Function to NFS unshare a directory.
 * If errbuf is non-NULL, returns any error returned
 * from the exec()ed command.
 */
static int
nfs_unshare_dir(char *dir_name, char *errbuf, int errlen)
{
	int	status;
	pid_t	pid;
	FILE	*err_stream = NULL;
	char	line[200] = {0};		/* first line of stderr */
	char	cmd[MAXPATHLEN+32] = {0};	/* MAXPATHLEN + share command */
	int	ret = 0;

	Trace(TR_MISC, "NFS unsharing %s", dir_name);

	snprintf(cmd, sizeof (cmd), "/usr/sbin/unshare %s", dir_name);

	pid = exec_get_output(cmd, NULL, &err_stream);

	if (pid < 0) {
		samerrno = SE_ERROR_NFS_UNSHARE;
		return (-1);
	}

	fgets(line, 200, err_stream);

	if ((pid = waitpid(pid, &status, 0)) < 0) {
		Trace(TR_ERR, "waitpid failed");
	} else {
		Trace(TR_DEBUG, "unshare(pid = %ld) status: %d\n", pid, status);
	}
	fclose(err_stream);

	if (WIFEXITED(status)) {
		ret = WEXITSTATUS(status);
		if (ret == 32) {
			/* directory was already unshared */
			ret = 0;
		}
		if (ret == 0) {
			Trace(TR_MISC, "unshared %s", dir_name);
		} else {
			Trace(TR_ERR, "unshare %s exit code: %d\n",
			    dir_name, ret);
			samerrno = SE_ERROR_NFS_UNSHARE;
			if ((errbuf != NULL) && (strlen(line) > 0)) {
				size_t i = 0;
				/* strip off the command name */
				i = strspn(line, "nfs unshare:");
				strlcpy(errbuf, &line[i], errlen);
			}
			ret = -1;
		}
	} else {
		Trace(TR_ERR, "unshare abnormally terminated.");
		samerrno = SE_ERROR_NFS_UNSHARE;
		ret = -1;
	}

	return (ret);
}

/*
 * Function to NFS share a directory.
 * If errbuf is non-NULL, returns any error returned
 * from the exec()ed command.
 */
static int
nfs_share_dir(nfsshare_t *nfsent, char *errbuf, int errlen)
{
	int	status;
	pid_t	pid;
	FILE	*err_stream = NULL;
	char	line[200] = {0};		/* first line of stderr */
	char	*cmd;
	int	ret = 0;

	Trace(TR_MISC, "NFS sharing %s", nfsent->dir_name);

	cmd = create_nfsshare_cmd(nfsent);
	if (cmd == NULL) {
		samerrno = SE_ERROR_NFS_SHARE;
		if (errbuf != NULL) {
			strlcpy(errbuf, GetCustMsg(SE_NO_MEM), errlen);
		}
		return (-1);
	}

	/*
	 * Ensure the NFS daemon is running before issuing
	 * the share command.  Oddly, share doesn't fail if
	 * NFS isn't there to service remote mount requests.
	 */
	ret = nfs_start_server();
	if (ret != 0) {
		samerrno = SE_ERROR_NFSD_NOT_STARTED;
		free(cmd);
		return (-1);
	}

	pid = exec_get_output(cmd, NULL, &err_stream);

	if (pid < 0) {
		samerrno = SE_ERROR_NFS_SHARE;
		free(cmd);
		return (-1);
	}

	fgets(line, 200, err_stream);

	if ((pid = waitpid(pid, &status, 0)) < 0) {
		Trace(TR_ERR, "waitpid failed");
	} else {
		Trace(TR_DEBUG, "share(pid = %ld) status: %d\n", pid, status);
	}
	fclose(err_stream);

	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status)) { /* non zero exit code */
			Trace(TR_ERR, "share exit code: %d\n",
			    WEXITSTATUS(status));
			ret = -1;
		} else {
			Trace(TR_MISC, "shared %s\n", nfsent->dir_name);
		}
	} else {
		Trace(TR_ERR, "share abnormally terminated.");
		ret = -1;
	}

	if (ret == -1) {
		samerrno = SE_ERROR_NFS_SHARE;
		if ((errbuf != NULL) && (strlen(line) > 0)) {
			int i = 0;

			/* strip off the command name */
			i = strspn(line, "share_nfs:");
			strlcpy(errbuf, &line[i], errlen);
		}
	}

	free(cmd);
	return (ret);
}

/*
 * Function to start the NFS server processes if they
 * are not already running.
 */
static int
nfs_start_server(void)
{
	int	status;
	pid_t	pid;
	FILE	*err_stream = NULL;
	char	line[200];		/* first line of stderr */
	char	*cmd = "/etc/init.d/nfs.server start";
	int	ret = 0;
	struct	rpcent *rpcret;

	/* get the RPC number for NFS */
	rpcret = getrpcbyname("nfs");
	if (!rpcret) {
		return (-1);
	}

	/*
	 * Now see if nfsd is actually running on this system.
	 * Start with TCP, then if it fails, check UDP too.
	 */
	ret = getrpcport("localhost", rpcret->r_number, 0, IPPROTO_TCP);
	if (ret == 0) {		/* failed */
		ret = getrpcport("localhost", rpcret->r_number, 0, IPPROTO_UDP);
	}

	/* Already running, nothing for us to do here */
	if (ret != 0) {
		return (0);
	}

	pid = exec_get_output(cmd, NULL, &err_stream);

	if (pid < 0) {
		return (-1);
	}

	fgets(line, 200, err_stream);

	if ((pid = waitpid(pid, &status, 0)) < 0) {
		Trace(TR_ERR, "waitpid failed");
	} else {
		Trace(TR_DEBUG, "Start NFS(pid = %ld) status: %d\n",
		    pid, status);
	}
	fclose(err_stream);

	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status)) { /* non zero exit code */
			Trace(TR_ERR, "Start NFS exit code: %d\n",
			    WEXITSTATUS(status));
			ret = -1;
		} else {
			Trace(TR_MISC, "NFS started\n");
		}
	} else {
		Trace(TR_ERR, "Start NFS abnormally terminated.");
		ret = -1;
	}

	return (ret);
}

/*
 *  Function to retrieve all the NFS shares associated with a
 *  mountpoint.  If activeonly is TRUE, returns only those
 *  actively being shared (in /etc/dfs/sharetab).
 *
 *  Note that a mntpt of "" indicates the caller wants
 *  all NFS shares for all file systems.  This is a new behaviour
 *  introduced in 4.6/Intellistor2.0.
 *
 *  Used by get_nfs_opts(), nfs_unshare_all(), nfs_remove_all().
 *
 */
static int
get_all_nfs(char *mntpt, sqm_lst_t **shares, boolean_t activeonly)
{

	struct statvfs64	statbuf;
	struct statvfs64	statbuf2;
	nfsshare_t 	*nfsent;
	sqm_lst_t 		*retlist;
	sqm_lst_t 		*nfslst = NULL;
	sqm_lst_t 		*dfslst = NULL;
	FILE 		*vfsp;
	struct vfstab	vfsent;
	node_t 		*node;
	node_t 		*next;
	ulong_t		fsid;
	int			ret;

	if ((mntpt == NULL) || (shares == NULL)) {
		return (-1);
	}

	*shares = NULL;

	retlist = lst_create();
	if (retlist == NULL) {
		return (1);
	}

	if (!activeonly) {
		get_set_dfstab(NULL, &dfslst);
		if ((dfslst) && (dfslst->length == 0)) {
			lst_free(dfslst);
			dfslst = NULL;
		}
	}

	nfslst = parse_sharetab();
	if ((nfslst) && (nfslst->length == 0)) {
		lst_free(nfslst);
		nfslst = NULL;
	}

	if (nfslst && dfslst) {
		lst_merge(nfslst, dfslst, nfssvr_mrgcomp);
	} else if ((nfslst == NULL) && (dfslst != NULL)) {
		nfslst = dfslst;
	}

	/*
	 * If there are no shares _or_ the requested mountpoint
	 * is the empty string, there's nothing left to be done.
	 * Note that no shares is not an error.
	 */
	if (nfslst == NULL) {
		*shares = retlist;
		return (0);
	}

	if (strcmp(mntpt, "") == 0) {
		*shares = nfslst;
		return (0);
	}

	/*
	 * We're looking for shares belonging to a specific mountpoint.
	 */
	ret = statvfs64(mntpt, &statbuf);
	if (ret != 0) {		/* not mounted ? */
		lst_free_deep(nfslst);
		lst_free(retlist);
		return (1);
	}

	/* Used to check if the shared directory is also a mount point */
	vfsp = fopen(VFSTAB, "r");

	node = nfslst->head;

	while (node != NULL) {
		ret = 0;
		nfsent = (nfsshare_t *)node->data;
		next = node->next;
		fsid = nfsent->fsid;

		if (!fsid) {
			ret = statvfs64(nfsent->dir_name, &statbuf2);
			if (ret == 0) {
				fsid = statbuf2.f_fsid;
			}
		}

		if ((ret == 0) && (fsid == statbuf.f_fsid)) {
			/*
			 * check to make sure that we're flagging NFS shares
			 * that are also mountpoints, but aren't currently
			 * mounted.  The sysadmin might get confused and
			 * this is probably an unintentional error on their
			 * part.
			 */
			if (vfsp) {
				rewind(vfsp);	/* always start at top */
				ret = getvfsfile(vfsp, &vfsent,
				    nfsent->dir_name);
				if ((ret == 0) &&
				    (strcmp(nfsent->dir_name,
				    mntpt) == 0)) {
					nfsent->is_mountpt = TRUE;
				}
			}

			lst_append(retlist, nfsent);
			lst_remove(nfslst, node);
		}

		node = next;
	}

	if (vfsp) {
		fclose(vfsp);
	}
	lst_free_deep(nfslst);

	*shares = retlist;

	return (0);
}
