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
#pragma ident "$Revision: 1.48 $"

/*
 *	server_info.c -  generic system info functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/systeminfo.h>
#include <sys/mnttab.h>
#include <sys/statvfs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pkginfo.h>
#include <errno.h>

#include "pub/mgmt/types.h"
#include "pub/mgmt/mgmt.h"
#include "pub/mgmt/error.h"
#include "mgmt/config/media.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/sam_trace.h"
#include "mgmt/util.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/filesystem.h"

/* Headers for config status */
#include "mgmt/config/stager.h"
#include "mgmt/config/releaser.h"
#include "mgmt/config/recycler.h"
#include "mgmt/config/archiver.h"
#include "mgmt/config/cfg_fs.h"
#include "mgmt/config/init.h"
#include "pub/mgmt/license.h"
#include "pub/mgmt/process_job.h"

static char *_SrcFile = __FILE__;  /* Using __FILE__ makes duplicate strings */


#define	CFG_STATUS_FMT "config=%s,status=%s"
#define	CFG_STATUS_FMT_MSG "config=%s,status=%s,message=%s"
#define	CFG_STATUS_FMT_MSG_PLUS "config=%s,status=%s,message=%s %s"
#define	CFG_STATUS_FMT_W_VSNS "config=%s,status=%s,message=%s,vsns="
#define	DEF_MSG_SZ 192
#define	ARCH_MSG_SZ 352
#define	MAX_STATUS_LEN 32

#define	SC_RELEASE_FILE "/etc/cluster/release"
#define	SC_RELEASE_MAXLEN 200

#define	SC_CLUSTER_GET_CMD "/usr/cluster/bin/scha_cluster_get"
#define	SC_GET_CLUSTERNAME_CMD SC_CLUSTER_GET_CMD" -O CLUSTERNAME"
#define	SC_GET_NODES_NAMES_CMD SC_CLUSTER_GET_CMD" -O ALL_NODENAMES"
#define	SC_GET_NODES_IDS_CMD SC_CLUSTER_GET_CMD" -O ALL_NODEIDS"
#define	SC_GET_NODE_STATUS_CMD SC_CLUSTER_GET_CMD" -O NODESTATE_NODE "
#define	IS_WEBCON_RUNNING_CMD "/usr/sbin/smcwebserver status -p \
	| /usr/bin/grep running | /usr/bin/cut -d""="" -f2"
#define	SC_GET_SMREG_APP_CMD "/usr/sbin/smreg list -a \
	| /usr/bin/grep com.sun.cluster"


static int get_fs_capacities(int *fs_count, fsize_t *capacity,
	fsize_t *available);

static int get_ip_addrs(sqm_lst_t **lst_ip);
static int get_rel_cfg_status(sqm_lst_t *l);
static int get_arch_cfg_status(sqm_lst_t *l);
static int get_stg_cfg_status(sqm_lst_t *l);
static int get_rc_cfg_status(sqm_lst_t *l);
static int add_unknown_status(char *cfg_file, char *problem_file, sqm_lst_t *l);
static void get_patch_info(char *pkgname, char **patchrev);


/*
 * DESCRIPTION:
 * return file system and media capacity and usage as a comma
 * separated set of key value pairs. If this function is unable to
 * retrieve any of the information it will simply be omitted. e.g. If
 * the catalog daemon is not running there will be no information
 * available about media capacity.
 *
 * Capacity information is only reported for mounted file systems and is only
 * reported for shared file system if the function is called on the metadata
 * server.
 *
 * Library count will include the historian and pseudo libraries such
 * as network attached libraries
 *
 * Disk Archive information will include information for volumes that
 * are configured for use on this machine. This will include the
 * remote volumes that are configured to be used on this machine. If
 * two local disk volumes are created on the same local file
 * system the capacity will be reported only once. However, if remote
 * volumes used on this machine are from the same file system on
 * the server host, the capacity will be over-reported.
 *
 * All fields in the return are optional. A failure to collect the information
 * (e.g. due to daemons not running) could result in an empty string. A -1
 * return will only occur if there is a problem with the inputs or an out of
 * memory condition occurs.
 *
 * PARAMS:
 *   ctx_t *	IN   -	context object
 *   char **	OUT  -	malloced string of CSV key value pairs in the following
 *			format
 *
 *	MountedFS		= <number of mounted file systems>
 *	DiskCache		= <diskcache of mounted SAM-FS/QFS
 *					file systems in kilobytes>
 *	AvailableDiskCache	= <available disk cache in kilobytes>
 *	LibCount		= <number of libraries>
 *	MediaCapacity		= <capacity of library in kilobytes>
 *	AvailableMediaCapacity	= <available capacity in kilobytes>
 *	SlotCount		= <number of configured slots>
 *	DiskArchiveCount	= <number of disk volumes configured>
 *	DiskArchive		= <kilobytes of disk archive space>
 *	AvailableDiskArchive	= <kilobytes of available disk archive space>
 * RETURNS:
 *   success -  0
 *   error - -1
 */
int
get_server_capacities(
ctx_t *ctx, /* ARGSUSED */
char **res) /* malloced string result */
{

	int chars = 0;
	int count = 0;
	fsize_t cap = 0;
	fsize_t avail = 0;
	int slots = 0;
	boolean_t fs_info_added;
	char tmp_res[256];


	if (ISNULL(res)) {
		Trace(TR_OPRMSG, "null argument found");
		return (-1);
	}
	*res = '\0';

	/* ignore errors otherwise add results to the string */
	if (get_fs_capacities(&count, &cap, &avail) == 0) {
		chars = snprintf(tmp_res, sizeof (tmp_res),
		    "MountedFS=%d, DiskCache=%llu, AvailableDiskCache=%llu",
		    count, cap, avail);
		fs_info_added = B_TRUE;
	}

	if (chars < 0 || chars > sizeof (tmp_res)) {
		return (0);
	}

	/* ignore errors otherwise add results to the string */
	if (get_total_library_capacity(&count, &slots, &avail, &cap) == 0) {
		if (fs_info_added) {
			snprintf(tmp_res + chars, sizeof (tmp_res) - chars,
			    ", ");
			chars += 2;
		}

		chars += snprintf(tmp_res + chars, sizeof (tmp_res) - chars,
		    "LibCount=%d, SlotCount=%d, ", count, slots);

		/* get_total_library_capacity returns cap in KB */
		snprintf(tmp_res + chars, sizeof (tmp_res) - chars,
		    "MediaCapacity=%llu, AvailableMediaCapacity=%llu",
		    cap, avail);
	}


	if (chars < 0 || chars > sizeof (tmp_res)) {
		return (0);
	}



	*res = strdup(tmp_res);
	if (*res == NULL) {
		setsamerr(SE_NO_MEM);
		return (-1);
	}

	return (0);
}


static int
get_fs_capacities(
int	*fs_count,
fsize_t *capacity,
fsize_t *available)
{
	struct sam_fs_status *fsarray;
	struct sam_fs_status *fs;
	struct sam_fs_info fi;
	int i;
	int numfs;
	fsize_t cap = 0LL;
	fsize_t avail = 0LL;
	int mnt_cnt = 0;
	FILE *fp;
	struct mnttab mntfs;
	struct statvfs statfs;

	if (ISNULL(capacity, available, fs_count)) {

		return (-1);
	}

	/* STEP 1/2: get capacities for non-SAMQ filesystems */
	if (NULL == (fp = fopen(MNTTAB, "r"))) {
		samerrno = SE_GET_FS_STATUS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_OPRMSG, "get fs status failed: cannot open mnttab");
	} else {
		while (0 == getmntent(fp, &mntfs)) {

			if (NULL == strstr("ufs", mntfs.mnt_fstype))
				continue;
			statvfs(mntfs.mnt_mountp, &statfs);

			cap += (uint64_t)statfs.f_blocks * statfs.f_frsize;
			avail += (uint64_t)statfs.f_bfree * statfs.f_frsize;
			mnt_cnt++;
		}
		fclose(fp);
	}

	/* STEP 2/2: get capacities for (SAM)QFS filesystems */
	if ((numfs = GetFsStatus(&fsarray)) <= 0) {
		samerrno = SE_GET_FS_STATUS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_OPRMSG, "get fs status failed");
	} else {

		for (i = 0; i < numfs; i++) {
			fs = fsarray + i;
			if (GetFsInfo(fs->fs_name, &fi) == -1) {
				continue;
			}

			/*
			 * capacity information is not available
			 * unless mounted
			 */
			if (!(fi.fi_status & FS_MOUNTED)) {
				continue;
			}

			mnt_cnt++;

			/*
			 * only include shared fs capacity on the
			 * metadata server
			 */
			if (fi.fi_status & FS_CLIENT) {
				continue;
			}

			cap += fi.fi_capacity;
			avail += fi.fi_space;
		}
	}
	*fs_count = mnt_cnt;
	*capacity = cap / 1024;
	*available = avail / 1024;
	free(fsarray);
	return (0);
}


/*
 * Get all keys for the default list of packages. See get_package_information
 */
int
get_package_info(
ctx_t *ctx	/* ARGSUSED */,
char *pkgs,
sqm_lst_t **res)
{
	return (get_package_information(ctx, pkgs, PI_ALL, res));
}

#define	PKGBUFLEN	1024


/*
 * DESCRIPTION:
 *
 *	If packages is NULL or "\0" then this function will fetch
 *	information for each package in DEFAULT_PACKAGES.
 *
 *	This function will only return packages that are found. It
 *	will not return an error if the package is not found.
 *
 * PARAMS:
 *	ctx_t * IN - "ctx_t" context object char
 *	char * IN - "pkgs" a space separated list of packages to get info for
 *		If NULL or "\0" then this function will fetch information for
 *		the packages defined in DEFAULT_PACKAGES
 *	int32_t IN - Flags to indicate which of the keys to include. Use
 *			the PI_XXX Flags from above.
 *	sqm_lst_t ** OUT  - malloced list of strings containing CSV key value
 *			data.
 *
 * Example (new lines will not be present)
 *  PKGINST = SUNWsamfsu,
 *     NAME = Sun SAM and Sun SAM-QFS software Solaris 10 (usr),
 * CATEGORY = system,
 *     ARCH = sparc,
 *  VERSION = 4.6.5 REV=debug REV=5.10.2007.03.12,
 *   VENDOR = Sun Microsystems Inc.,
 *   STATUS = completely installed
 *
 * RETURNS:
 *   success -  0
 * error - -1
 */
int
get_package_information(
ctx_t *ctx	/* ARGSUSED */,
char *pkgs,
int32_t which_info,
sqm_lst_t **res)
{

	struct pkginfo pkg;
	char *pkgstr;
	char *workstr;
	char *strp;
	char *revstring;
	char buf[PKGBUFLEN] = "";
	char *last;

	if (ISNULL(res)) {
		Trace(TR_ERR, "Getting pkginfo failed:%d %s", samerrno,
		    samerrmsg);
		return (-1);
	}


	*res = lst_create();
	if (*res == NULL) {
		Trace(TR_ERR, "Getting pkginfo failed:%d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	if (pkgs == NULL || *pkgs == '\0') {
		workstr = strdup(DEFAULT_PACKAGES);
	} else {
		workstr = strdup(pkgs);
	}

	pkgstr = strtok_r(workstr, " ", &last);
	while (pkgstr != NULL) {
		if (pkginfo(&pkg, pkgstr, NULL, NULL) != -1) {
			*buf = '\0';
			if (which_info & PI_PKGINST && pkg.pkginst != NULL) {
				strlcat(buf, "PKGINST = ", PKGBUFLEN);
				strlcat(buf, pkg.pkginst, PKGBUFLEN);
				strlcat(buf, ", ", PKGBUFLEN);
			}
			if (which_info & PI_CATEGORY && pkg.catg != NULL) {
				strlcat(buf, "CATEGORY = ", PKGBUFLEN);
				strlcat(buf, pkg.catg, PKGBUFLEN);
				strlcat(buf, ", ", PKGBUFLEN);
			}
			if (which_info & PI_ARCH && pkg.arch != NULL) {
				strlcat(buf, "ARCH = ", PKGBUFLEN);
				strlcat(buf, pkg.arch, PKGBUFLEN);
				strlcat(buf, ", ", PKGBUFLEN);
			}
			if (which_info & PI_VERSION && pkg.version != NULL) {
				strlcat(buf, "VERSION = ", PKGBUFLEN);
				/*
				 * The VERSION string has an unwanted
				 * name-value pair embedded in it. e.g.:
				 * "VERSION=4.6.5,REV=5.10.2007.03.12"
				 * So we throw away everything after the ",".
				 */
				strp = (char *)strchr(pkg.version, ',');
				if (strp != NULL) {
					*strp = '\0';
				}

				strlcat(buf, pkg.version, PKGBUFLEN);
				/* Get the patch revision.  */
				get_patch_info(pkgstr, &revstring);
				if (revstring != NULL) {
					strlcat(buf, revstring, PKGBUFLEN);
					(void) free(revstring);
				}
				strlcat(buf, ", ", PKGBUFLEN);
			}
			if (which_info & PI_STATUS) {
				size_t len = strlcat(buf, "STATUS = ",
				    PKGBUFLEN);

				if (len < PKGBUFLEN) {
					snprintf(buf + len, PKGBUFLEN - len,
					    "%d, ", (int)pkg.status);
				}
			}
			if (which_info & PI_NAME && pkg.name != NULL) {
				strlcat(buf, "NAME = ", PKGBUFLEN);
				strlcat(buf, pkg.name, PKGBUFLEN);
				strlcat(buf, ", ", PKGBUFLEN);
			}
			if (which_info & PI_VENDOR && pkg.vendor != NULL) {
				strlcat(buf, "VENDOR = ", PKGBUFLEN);
				strlcat(buf, pkg.vendor, PKGBUFLEN);
				strlcat(buf, ", ", PKGBUFLEN);
			}

			/*
			 * Remove the final comma and insert a copy
			 * of the string into the list.
			 */
			if (*buf != '\0') {
				char *tmp;
				char *end = strrchr(buf, ',');

				if (end != NULL) {
					*end = '\0';
				}

				tmp = strdup(buf);
				if (tmp == NULL) {
					goto err;
				}
				if (lst_append(*res, tmp) != 0) {
					free(tmp);
					goto err;
				}
			}

		} else {
			/*
			 * can fail for : EINVAL ESRCH EACCES
			 * ESRCH indicates the package is not present.
			 */
			if (errno != ESRCH) {
				samerrno = SE_PKGINFO_FAILED;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_PKGINFO_FAILED));

				Trace(TR_ERR, "pkginfo error %d for %s",
				    errno, pkgstr);
				goto err;
			}
		}

		pkgstr = strtok_r(NULL, " ", &last);

	}
	Trace(TR_MISC, "got package info");
	free(workstr);
	return (0);


err:
	lst_free_deep(*res);
	free(workstr);
	Trace(TR_ERR, "getting package info failed %d %s",
	    samerrno, samerrmsg);
	return (-1);
}

/*
 * Get patch revision.
 *
 * There's no pkginfo() call for patch revision, so
 * we need to examine the the pkginfo file directly.
 */
#define	SEPARATOR	"="
#define	SPACE		" "

static void get_patch_info(
char *pkgname,
char **patchrev) {

	char pkginfoline[1024];
	char pkginfofile[1024];
	char patchidstr[1024];
	char *key;
	char *lasttoken;
	char *last1;
	char *last2;
	char *patch;
	char *value;
	FILE *filep;

	char *path = "/var/sadm/pkg";
	char *filename = "pkginfo";

	Trace(TR_MISC, "Checking patch revision");

	(void) snprintf(pkginfofile, sizeof (pkginfofile),
	    "%s/%s/%s", path, pkgname, filename);
	if ((filep = fopen(pkginfofile, "r")) == NULL) {
		patchrev = NULL;
		return;
	}

	while (fgets(pkginfoline, sizeof (pkginfoline), filep) != NULL) {
		if ('\n' == pkginfoline[strlen(pkginfoline) - 1]) {
			pkginfoline[strlen(pkginfoline) - 1] = '\0';
		}
		/* Make sure there's a "=" in pkginfoline */
		if (strchr(pkginfoline, *SEPARATOR) == NULL) {
			continue;
		}
		key = strtok_r(pkginfoline, SEPARATOR, &last1);
		if (key == NULL) {
		/* Something's not right, but let's persevere. */
			continue;
		}
		if (strncmp("SUNW_PATCHID", key, 12) == 0) {
			if ((value =
			    strtok_r(NULL, SEPARATOR, &last1)) != NULL) {
				/* Patch id found. */
				(void) fclose(filep);
				/* Dress up the patch id number. */
				(void) snprintf(patchidstr, sizeof (patchidstr),
				    " (%s)", value);
				*patchrev = strdup(patchidstr);
				return;
			}
		}
	}

	/*
	 * If we get this far, it's because the patch rev
	 * can't be determined via SUNW_PATCHID, let's
	 * check the PATCHLIST.
	 *
	 * If successive revs of the samq patch are loaded,
	 * and then current patch is backed out, SUNW_PATCHID
	 * is blank. In this case we check the PATCH_LIST for
	 * the current patch rev.
	 */

	rewind(filep);
	while (fgets(pkginfoline, sizeof (pkginfoline), filep) != NULL) {
		if ('\n' == pkginfoline[strlen(pkginfoline) - 1]) {
			pkginfoline[strlen(pkginfoline) - 1] = '\0';
		}
		/* Make sure there's a "=" in pkginfoline */
		if (strchr(pkginfoline, *SEPARATOR) == NULL) {
			continue;
		}
		key = strtok_r(pkginfoline, SEPARATOR, &last1);
		if (key == NULL) {
		/* Something's not right, but let's persevere. */
			continue;
		}
		if (strncmp("PATCHLIST", key, 9) == 0) {
			if ((value =
			    strtok_r(NULL, SEPARATOR, &last1)) != NULL) {
				/*
				 * Keep parsing to the end of the list; the
				 * last value in the list is the current patch.
				 */
				while ((patch =
				    strtok_r(value, SPACE, &last2)) != NULL) {
					lasttoken = patch;
					value = NULL;
				}
				/* Patch id found. */
				(void) fclose(filep);
				if (lasttoken != NULL) {
					/* Dress up the patch id number. */
					(void) snprintf(patchidstr,
					    sizeof (patchidstr),
					    " (%s)", lasttoken);
					*patchrev = strdup(patchidstr);
				} else {
					*patchrev = NULL;
				}
				return;
			}
		}
	}
	/* There's no patch. */
	(void) fclose(filep);
	*patchrev = NULL;
}


static int
get_rel_cfg_status(sqm_lst_t *l)
{

	releaser_cfg_t *rl_cfg = NULL;
	char status_buf[MAX_STATUS_LEN];
	char buf[DEF_MSG_SZ] = "";
	char *res;

	if (read_releaser_cfg(&rl_cfg) != 0) {
		if (samerrno == SE_CONFIG_ERROR) {
			snprintf(status_buf, MAX_STATUS_LEN,
			    GetCustMsg(SE_CFG_STAT_STR_ERR));

			snprintf(buf, sizeof (buf), CFG_STATUS_FMT_MSG,
			    RELEASE_CFG, status_buf,
			    GetCustMsg(SE_RELEASER_ERR_MSG));
		} else {
			snprintf(status_buf, MAX_STATUS_LEN,
			    GetCustMsg(SE_CFG_STAT_STR_UNKNOWN));

			snprintf(buf, sizeof (buf), CFG_STATUS_FMT_MSG_PLUS,
			    RELEASE_CFG, status_buf,
			    GetCustMsg(SE_CONFIG_REQUIRED), MCF_CFG);

			rl_cfg = NULL;

		}
	} else {

		snprintf(buf, sizeof (buf), CFG_STATUS_FMT,
		    RELEASE_CFG, GetCustMsg(SE_CFG_STAT_STR_OK));
		free_releaser_cfg(rl_cfg);
	}


	res =  strdup(buf);
	if (res == NULL) {
		setsamerr(SE_NO_MEM);
		Trace(TR_ERR, "getting releaser cfg status failed:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	if (lst_append(l, res) != 0) {
		free(res);
		Trace(TR_ERR, "getting releaser cfg status failed:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	return (0);
}


static int
get_rc_cfg_status(sqm_lst_t *l)
{

	recycler_cfg_t *rc_cfg = NULL;
	char status_buf[MAX_STATUS_LEN];
	char buf[DEF_MSG_SZ] = "";
	char *res;

	if (read_recycler_cfg(&rc_cfg) != 0) {
		if (samerrno == SE_CONFIG_CONTAINED_ERRORS) {
			snprintf(status_buf, MAX_STATUS_LEN,
			    GetCustMsg(SE_CFG_STAT_STR_ERR));

			snprintf(buf, sizeof (buf), CFG_STATUS_FMT_MSG,
			    RECYCLE_CFG, status_buf,
			    GetCustMsg(SE_RECYCLER_ERR_MSG));
		} else {
			snprintf(status_buf, MAX_STATUS_LEN,
			    GetCustMsg(SE_CFG_STAT_STR_UNKNOWN));

			snprintf(buf, sizeof (buf), CFG_STATUS_FMT_MSG_PLUS,
			    RECYCLE_CFG, status_buf,
			    GetCustMsg(SE_CONFIG_REQUIRED), MCF_CFG);

			rc_cfg = NULL;
		}
	} else {
		snprintf(buf, sizeof (buf), CFG_STATUS_FMT,
		    RECYCLE_CFG, GetCustMsg(SE_CFG_STAT_STR_OK));
	}

	free_recycler_cfg(rc_cfg);


	res =  strdup(buf);
	if (res == NULL) {
		setsamerr(SE_NO_MEM);
		Trace(TR_ERR, "getting recycler cfg status failed:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	if (lst_append(l, res) != 0) {
		free(res);
		Trace(TR_ERR, "getting recycler cfg status failed:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	return (0);
}


static int
get_stg_cfg_status(sqm_lst_t *l)
{

	stager_cfg_t *stg_cfg = NULL;
	char status_buf[MAX_STATUS_LEN];
	char buf[DEF_MSG_SZ] = "";
	char *res;

	if (read_stager_cfg(&stg_cfg) != 0) {
		if (samerrno == SE_CONFIG_CONTAINED_ERRORS) {
			snprintf(status_buf, MAX_STATUS_LEN,
			    GetCustMsg(SE_CFG_STAT_STR_ERR));

			snprintf(buf, sizeof (buf), CFG_STATUS_FMT_MSG,
			    STAGE_CFG, status_buf,
			    GetCustMsg(SE_STAGER_ERR_MSG));
		} else {
			snprintf(status_buf, MAX_STATUS_LEN,
			    GetCustMsg(SE_CFG_STAT_STR_UNKNOWN));

			snprintf(buf, sizeof (buf), CFG_STATUS_FMT_MSG_PLUS,
			    STAGE_CFG, status_buf,
			    GetCustMsg(SE_CONFIG_REQUIRED), MCF_CFG);

			stg_cfg = NULL;
		}
	} else {
		snprintf(buf, sizeof (buf), CFG_STATUS_FMT,
		    STAGE_CFG, GetCustMsg(SE_CFG_STAT_STR_OK));
	}

	free_stager_cfg(stg_cfg);


	res = strdup(buf);
	if (res == NULL) {
		setsamerr(SE_NO_MEM);
		Trace(TR_ERR, "getting stager cfg status failed:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	if (lst_append(l, res) != 0) {
		free(res);
		Trace(TR_ERR, "getting stager cfg status failed:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	return (0);
}


/*
 * Check the status of the archiver.cmd file.
 *
 * FSD Note: archiver -lv reports errors if no file systems are found.
 * This case will occur incorrectly when file systems are configured
 * but sam-fsd is not running(after a fresh install with
 * no mounted filesystems)
 *
 * To check this condition when chk_arch_cfg returns an error, check to see
 * if sam-fsd is running, if so return the error. Other wise return OK.
 */
static int
get_arch_cfg_status(sqm_lst_t *l) {
	char status_buf[MAX_STATUS_LEN];
	int status = 0;
	char *buf;
	sqm_lst_t *errs = NULL;
	node_t *n;
	char *fsd_restricts = "type=SAMDFSD";


	buf = (char *)mallocer(ARCH_MSG_SZ);
	if (buf == NULL) {
		return (-1);
	}

	status = chk_arch_cfg(NULL, &errs);
	if (status == -1) {
		free(buf);
		return (-1);
	} else if (status == -2) {
		sqm_lst_t *acts;


		if (errs->length == 0) {
			snprintf(status_buf, MAX_STATUS_LEN,
				GetCustMsg(SE_CFG_STAT_STR_ERR));

			snprintf(buf, ARCH_MSG_SZ, CFG_STATUS_FMT_MSG,
				ARCHIVER_CFG, status_buf,
				GetCustMsg(SE_ARCHIVER_ERR_MSG));
		} else {

			snprintf(status_buf, MAX_STATUS_LEN,
				GetCustMsg(SE_CFG_STAT_STR_ERR));

			snprintf(buf, ARCH_MSG_SZ, CFG_STATUS_FMT_W_VSNS,
				ARCHIVER_CFG, status_buf,
				GetCustMsg(SE_ARCHIVER_NO_VSNS_DEFINED));
		}
		/*
		 * See FSD Note in function comment
		 */
		if (list_activities(NULL, 1, fsd_restricts, &acts) == 0) {
			if (acts == NULL || acts->length == 0) {
				snprintf(buf, ARCH_MSG_SZ, CFG_STATUS_FMT,
					ARCHIVER_CFG,
					GetCustMsg(SE_CFG_STAT_STR_OK));
			}
		}

	} else if (status == -3) {

		/*
		 * if there are no file systems do not include the list element
		 * and use the samerrmsg message text
		 */
		if (samerrno == SE_NO_FS_NO_ARCHIVING) {
			snprintf(buf, ARCH_MSG_SZ, CFG_STATUS_FMT_MSG,
			    ARCHIVER_CFG, GetCustMsg(SE_CFG_STAT_STR_WARN),
			    samerrmsg);
		} else {

			snprintf(status_buf, MAX_STATUS_LEN,
				GetCustMsg(SE_CFG_STAT_STR_WARN));

			snprintf(buf, ARCH_MSG_SZ, CFG_STATUS_FMT_W_VSNS,
			    ARCHIVER_CFG, status_buf,
			    GetCustMsg(SE_ARCHIVER_NO_VSNS_AVAIL));
		}
	} else {
		/* status == 0 */
		snprintf(buf, ARCH_MSG_SZ, CFG_STATUS_FMT,
		    ARCHIVER_CFG, GetCustMsg(SE_CFG_STAT_STR_OK));
	}

	if (status == -2 || status == -3) {
		int buf_len = ARCH_MSG_SZ;
		size_t str_len = strlen(buf);

		/* need to append any vsns that exist */
		for (n = errs->head; n != NULL; n = n->next) {
			char *c = (char *)n->data;
			size_t len_c = strlen(c);

			if (len_c >= buf_len - str_len + 1) {
				buf = realloc(buf, buf_len + ARCH_MSG_SZ);
				buf_len += ARCH_MSG_SZ;
			}
			str_len = strlcat(buf, c, buf_len);
			str_len = strlcat(buf, " ", buf_len);
		}

	}

	if (lst_append(l, buf) != 0) {
		free(buf);
		Trace(TR_ERR, "getting archiver cfg status failed:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	lst_free_deep(errs);
	return (0);
}


static int
add_unknown_status(char *cfg_file, char *problem_file, sqm_lst_t *l) {
	char status_buf[MAX_STATUS_LEN];
	char buf[DEF_MSG_SZ] = "";
	char *res;

	if (ISNULL(cfg_file, problem_file, l)) {
		Trace(TR_ERR, "adding status unknown failed:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	snprintf(status_buf, MAX_STATUS_LEN,
		GetCustMsg(SE_CFG_STAT_STR_UNKNOWN));

	snprintf(buf, DEF_MSG_SZ, CFG_STATUS_FMT_MSG_PLUS,
		cfg_file, status_buf, GetCustMsg(SE_CONFIG_REQUIRED),
		problem_file);

	res = strdup(buf);
	if (res == NULL) {
		setsamerr(SE_NO_MEM);
		Trace(TR_ERR, "adding status unknown failed:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	if (lst_append(l, res) != 0) {
		free(res);
		Trace(TR_ERR, "adding status unknown failed:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	return (0);
}


/*
 * Fetch configuration file status. This function reflects the fact
 * that the configuration files have close relationships with one
 * another.  In order to report appropriate status for all files and
 * and consolidate the logic for omitting missing files fsd
 * checking is done inline in this function and calls are made out to
 * get status for non-fsd checked configuration files.
 *
 * returns -1 only if there are errors on the inputs or list creation.
 */
int
get_configuration_status(
ctx_t *ctx	/* ARGSUSED */,
sqm_lst_t **l) {

	boolean_t exists[4] = {B_FALSE, B_FALSE, B_FALSE, B_FALSE};
	char	*cfgnm[4] = {SAMFS_CFG, DISKVOL_CFG, DEFAULTS_CFG, MCF_CFG};
	char	*extra[4] = { NULL, NULL, NULL, NULL };
	int	usr_msg[4] = { 0, 0, 0, 0 };
	int	stat_msg[4] = { SE_CFG_STAT_STR_OK, SE_CFG_STAT_STR_OK,
				SE_CFG_STAT_STR_OK, SE_CFG_STAT_STR_OK};
	boolean_t	qfs_only = B_FALSE;
	boolean_t	other_configs = B_FALSE;
	boolean_t	mcf_exists = B_FALSE;
	sqm_lst_t		*errs = NULL;
	int		status;
	int		i;
	time_t		t;


	if (ISNULL(l)) {
		Trace(TR_ERR, "get config status failed:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	*l = lst_create();
	if (*l == NULL) {
		Trace(TR_ERR, "get config status failed:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	/* See which files exist */
	if (get_file_time(INIT_SAMFS_CMD, &t) == 0 && (t != 0)) {
		exists[0] = B_TRUE;
		other_configs = B_TRUE;
	}

	if (get_file_time(INIT_DISKVOLS_CONF, &t) == 0 && (t != 0)) {
		exists[1] = B_TRUE;
		other_configs = B_TRUE;
	}

	if (get_file_time(INIT_DEFAULTS_CONF, &t) == 0 && (t != 0)) {
		exists[2] = B_TRUE;
		other_configs = B_TRUE;
	}

	if ((get_file_time(INIT_MCF, &t) == 0) && (t != 0)) {
		exists[3] = B_TRUE;
		mcf_exists = B_TRUE;
	}


	/*
	 * If this is not a qfs only system get the status for
	 * configuration files that exist
	 */
	qfs_only = (get_samfs_type(NULL) == QFS);

	if (!qfs_only) {
		if ((get_file_time(INIT_ARCHIVER_CMD, &t) == 0) && (t != 0)) {
			if (mcf_exists) {
				get_arch_cfg_status(*l);
			} else {
				add_unknown_status(ARCHIVER_CFG, MCF_CFG, *l);
			}
			other_configs = B_TRUE;
		}

		if ((get_file_time(INIT_RELEASER_CMD, &t) == 0) && (t != 0)) {
			if (mcf_exists) {
				get_rel_cfg_status(*l);
			} else {
				add_unknown_status(RELEASE_CFG, MCF_CFG, *l);
			}
			other_configs = B_TRUE;
		}

		if ((get_file_time(INIT_RECYCLER_CMD, &t) == 0) && (t != 0)) {
			if (mcf_exists) {
				get_rc_cfg_status(*l);
			} else {
				add_unknown_status(RECYCLE_CFG, MCF_CFG, *l);
			}
			other_configs = B_TRUE;
		}

		if ((get_file_time(INIT_STAGER_CMD, &t) == 0) && (t != 0)) {
			if (mcf_exists) {
				get_stg_cfg_status(*l);
			} else {
				add_unknown_status(STAGE_CFG, MCF_CFG, *l);
			}
			other_configs = B_TRUE;
		}
	}


	/*
	 * If mcf does not exist, dont get its status
	 * unless dependent files do exist
	 */
	if (!mcf_exists) {
		if (!other_configs) {
			/*
			 * No mcf and no other configs just
			 * return success
			 */
			return (0);
		}
		/*
		 * mcf does not exist. If none of the others exist
		 * this is not a problem otherwise it may be
		 */
		if (exists[0]) {
			stat_msg[0] = SE_CFG_STAT_STR_UNKNOWN;
			usr_msg[0] = SE_CONFIG_REQUIRED;
			extra[0] = MCF_CFG;
		}
		if (exists[1]) {
			stat_msg[1] = SE_CFG_STAT_STR_UNKNOWN;
			usr_msg[1] = SE_CONFIG_REQUIRED;
			extra[1] = MCF_CFG;
		}
		if (exists[2]) {
			stat_msg[2] = SE_CFG_STAT_STR_UNKNOWN;
			usr_msg[2] = SE_CONFIG_REQUIRED;
			extra[2] = MCF_CFG;
		}

		stat_msg[3] = SE_CFG_STAT_STR_NOFILE;
		usr_msg[3] = SE_NO_MCF_ERR_MSG;

	} else {
		Trace(TR_MISC, "get_fsd_config_status execing fsd");
		status = check_config_with_fsd(NULL, NULL, NULL, NULL, &errs);
		if (status == -1) {
			Trace(TR_ERR, "checking sam-fsd status failed:%d %s",
				samerrno, samerrmsg);
			return (0);
		} else if (status > 0) {
			char *e = (char *)errs->head->data;

			/*
			 * figure out which file was bad, based on the
			 * order in which sam-fsd evaluates them. When
			 * we know which file had a problem we know which
			 * ones did not and which ones we don't know.
			 *
			 * sam-fsd evalutates mcf -> defaults.conf ->
			 * diskvols.conf -> samfs.cmd
			 *
			 * We check for errors in reverse order. Setting
			 * any files we don't know the results for to
			 * unknown.
			 */
			if (strstr(e, SAMFS_CFG) != NULL) {
					stat_msg[0] = SE_CFG_STAT_STR_ERR;
					usr_msg[0] = SE_SAMFS_ERR_MSG;

			} else if (strstr(e, DISKVOL_CFG) != NULL) {

				if (exists[0]) {
					stat_msg[0] = SE_CFG_STAT_STR_UNKNOWN;
					usr_msg[0] = SE_CONFIG_REQUIRED;
					extra[0] = DISKVOL_CFG;
				}

				stat_msg[1] = SE_CFG_STAT_STR_ERR;
				usr_msg[1] = SE_DISKVOLS_ERR_MSG;

			} else if (strstr(e, DEFAULTS_CFG) != NULL) {

				if (exists[0]) {
					stat_msg[0] = SE_CFG_STAT_STR_UNKNOWN;
					usr_msg[0] = SE_CONFIG_REQUIRED;
					extra[0] = DEFAULTS_CFG;
				}

				if (exists[1]) {
					stat_msg[1] = SE_CFG_STAT_STR_UNKNOWN;
					usr_msg[1] = SE_CONFIG_REQUIRED;
					extra[1] = DEFAULTS_CFG;
				}

				stat_msg[2] = SE_CFG_STAT_STR_ERR;
				usr_msg[2] = SE_DEFAULTS_ERR_MSG;


			} else if (strstr(e, MCF_CFG) != NULL) {
				if (exists[0]) {
					stat_msg[0] = SE_CFG_STAT_STR_UNKNOWN;
					usr_msg[0] = SE_CONFIG_REQUIRED;
					extra[0] = MCF_CFG;
				}

				if (exists[1]) {
					stat_msg[1] = SE_CFG_STAT_STR_UNKNOWN;
					usr_msg[1] = SE_CONFIG_REQUIRED;
					extra[1] = MCF_CFG;
				}

				if (exists[2]) {
					stat_msg[2] = SE_CFG_STAT_STR_UNKNOWN;
					usr_msg[2] = SE_CONFIG_REQUIRED;
					extra[2] = MCF_CFG;
				}

				stat_msg[3] = SE_CFG_STAT_STR_ERR;
				usr_msg[3] = SE_MCF_ERR_MSG;
			}
		}
	}
	/*
	 * Put them in to the list in the order that they are evaluated by
	 * sam-fsd. Insert into the beginning so that the mcf, the file on
	 * which most others depend, is shown first.
	 */
	for (i = 0; i <= 3; i++) {
		int sz = DEF_MSG_SZ;
		char status_buf[MAX_STATUS_LEN];
		char *buf;

		/*
		 * do not include diskvols.conf if we are dealing
		 * with a qfs only installation
		 */
		if (i == 1 && qfs_only) {
			continue;
		}

		if (i != 3 && !(exists[i])) {
			/*
			 * if the file is not the mcf and
			 * it does not exist don't make an entry
			 * for it.
			 */
			continue;
		} else if (usr_msg[i] == 0) {
			buf = (char *)mallocer(sz);
			sz = snprintf(buf, sz, CFG_STATUS_FMT, cfgnm[i],
			    GetCustMsg(stat_msg[i]));

			if (sz > DEF_MSG_SZ) {
				free(buf);
				buf = (char *)mallocer(sz + 1);
				snprintf(buf, sz + 1, CFG_STATUS_FMT,
					cfgnm[i], GetCustMsg(stat_msg[i]));

			}

		} else if (extra[i] != NULL) {
			buf = (char *)mallocer(sz);

			snprintf(status_buf, MAX_STATUS_LEN,
				GetCustMsg(stat_msg[i]));

			sz = snprintf(buf, sz, CFG_STATUS_FMT_MSG_PLUS,
				cfgnm[i], status_buf, GetCustMsg(usr_msg[i]),
				extra[i]);


			if (sz > DEF_MSG_SZ) {
				free(buf);
				buf = (char *)mallocer(sz + 1);
				sz = snprintf(buf, sz + 1,
					CFG_STATUS_FMT_MSG_PLUS,
					cfgnm[i], status_buf,
					GetCustMsg(usr_msg[i]), extra[i]);
			}
		} else {
			buf = (char *)mallocer(sz);

			snprintf(status_buf, MAX_STATUS_LEN,
				GetCustMsg(stat_msg[i]));

			sz = snprintf(buf, sz, CFG_STATUS_FMT_MSG,
				cfgnm[i], status_buf,
				GetCustMsg(usr_msg[i]));


			if (sz > DEF_MSG_SZ) {
				free(buf);
				buf = (char *)mallocer(sz + 1);
				sz = snprintf(buf, sz + 1, CFG_STATUS_FMT_MSG,
					cfgnm[i], status_buf,
					GetCustMsg(usr_msg[i]));
			}
		}
		lst_ins_before(*l, (*l)->head, buf);
	}

	lst_free_deep(errs);
	Trace(TR_MISC, "got configuration status");
	return (0);

}

#define	ADD_DELIM(chars, buffer) \
	if (chars != 0) { \
		snprintf(buffer + chars, sizeof (buffer) - chars, ", "); \
		chars += 2; \
	}

/*
 * get_system_info
 * returns a malloced string containing the system info
 *
 * system_info is a sequence of key-value pairs as follows:
 * Hostid=<hostid>,
 * Hostname=<hostname>,
 * OSname=<sysname>,
 * Release=<release>,
 * Version=<version>,
 * Machine=<machine>,
 * Cpus=<number>,
 * Memory=<memory in Mbytes>,
 * Architecture=<arc>,
 * IpAddress=<ipaddress>,
 *
 */
int
get_system_info(ctx_t *ctx /* ARGSUSED */, char **info) {

	int chars = 0;
	char buffer[4096];
	struct utsname name;
	sqm_lst_t *lst_ip = NULL;
	node_t *node = NULL;
	uint64_t memory = 0;

	char *arch = NULL;
	long pagesize = 0, npages = 0, cpus = 0;

	*buffer = '\0';
	*info = '\0';

	/* ignore errors otherwise add results to the string */

	chars += snprintf(buffer, sizeof (buffer), "Hostid=%x", gethostid());

	if (uname(&name) != -1) {
		ADD_DELIM(chars, buffer);

		chars += snprintf(buffer + chars, sizeof (buffer) - chars,
		"Hostname=%s, OSname=%s, Release=%s, Version=%s, Machine=%s",
			name.nodename, name.sysname,
			name.release, name.version, name.machine);
	}

	/*
	 * To calculate the memory size, get the System memory
	 * page size (_SC_PAGESIZE) and total number of pages
	 * of physical memory in system (_SC_PHYS_PAGES)
	 */
	cpus = sysconf(_SC_NPROCESSORS_CONF);
	pagesize = sysconf(_SC_PAGESIZE);
	npages = sysconf(_SC_PHYS_PAGES);

	if (cpus != -1) {
		ADD_DELIM(chars, buffer);
		chars += snprintf(buffer + chars, sizeof (buffer) - chars,
			"Cpus=%ld", cpus);
	}

	if (pagesize != -1 && npages != -1)  {
		memory = (uint64_t)pagesize * (uint64_t)npages;
		ADD_DELIM(chars, buffer);
		chars += snprintf(buffer + chars, sizeof (buffer) - chars,
			"Memory=%llu", ((memory+MEGA-1) / MEGA));
	}

	if ((get_architecture(NULL, &arch) != -1) && arch != NULL) {
		ADD_DELIM(chars, buffer);
		chars += snprintf(buffer + chars, sizeof (buffer) - chars,
			"Architecture=%s", arch);
		free(arch);
	}

	if (get_ip_addrs(&lst_ip) == 0) {
		if (lst_ip != NULL && lst_ip->length > 0) {

			ADD_DELIM(chars, buffer);
			chars += snprintf(buffer + chars,
				sizeof (buffer) - chars, "IPaddress=");


			node = lst_ip->head;
			while (node != NULL) {
				chars += snprintf(buffer + chars,
					sizeof (buffer) - chars, "%s",
					(char *)node->data);

				if ((node = node->next) != NULL) {
					snprintf(buffer + chars,
						sizeof (buffer) - chars, " ");
					chars += 1;

				}
			}
		}
	}


	*info = strdup(buffer);

	return (0);
}


int
get_config_summary(ctx_t *c, char **res) {
	sqm_lst_t	*lst = NULL;
	char		*tmp_res;
	char		detail_buf[32];
	size_t		sz = 1024;
	node_t		*n;

	tmp_res = (char *)mallocer(sz);
	if (tmp_res == NULL) {
		*res = NULL;
		return (-1);
	}
	*tmp_res = '\0';

	if ((get_all_libraries(c, &lst) != -1) && lst->length != 0) {
		int cat_entries = 0;
		boolean_t libs_printed = B_FALSE;

		snprintf(detail_buf, sizeof (detail_buf),
		    "lib_count=%d,", lst->length);
		STRLCATGROW(tmp_res, detail_buf, sz);

		for (n = lst->head; n != NULL; n = n->next) {
			library_t *lib = (library_t *)n->data;
			int lib_entries = 0;
			if (lib != NULL) {
				libs_printed = B_TRUE;

				if (n == lst->head) {
					STRLCATGROW(tmp_res, "lib_names=", sz);
				}
				STRLCATGROW(tmp_res, lib->base_info.set, sz);
				STRLCATGROW(tmp_res, " ", sz);

				if (get_no_of_catalog_entries(c,
				    lib->base_info.eq, &lib_entries) == 0) {
					cat_entries += lib_entries;
				}
			}
			if (libs_printed) {
				STRLCATGROW(tmp_res, ",", sz);
			}

		}
		if (cat_entries != 0) {
			snprintf(detail_buf, sizeof (detail_buf),
			    "tape_count=%d,", cat_entries);
			STRLCATGROW(tmp_res, detail_buf, sz);
		}
	}
	if (lst != NULL) {
		free_list_of_libraries(lst);
	}

	if (get_all_fs(c, &lst) != -1) {
		if (lst->length != 0) {
			snprintf(detail_buf, sizeof (detail_buf),
			    "qfs_count=%d,", lst->length);
			STRLCATGROW(tmp_res, detail_buf, sz);
		}

		free_list_of_fs(lst);
	}
	if (get_all_disk_vols(c, &lst) != -1) {
		if (lst->length != 0) {
			snprintf(detail_buf, sizeof (detail_buf),
			    "disk_vols_count=%d,", lst->length);
			STRLCATGROW(tmp_res, detail_buf, sz);
		}

		lst_free_deep(lst);
	}
	if (get_all_vsn_pools(c, &lst) != -1) {
		if (lst->length != 0) {
			snprintf(detail_buf, sizeof (detail_buf),
			    "volume_pools=%d,", lst->length);
			STRLCATGROW(tmp_res, detail_buf, sz);
		}

		free_vsn_pool_list(lst);
	}


	if (tmp_res != NULL) {
		if (*tmp_res != '\0') {
			char *end;
			end = strrchr(tmp_res, ',');
			if (end != NULL) {
				*end = '\0';
			}
		}

		*res = tmp_res;
		return (0);
	} else {
		*res = NULL;
		setsamerr(SE_NO_MEM);
		return (-1);
	}
}


/*
 * obtain a list of ip addresses under which this host is known
 */
int
get_ip_addrs(
	sqm_lst_t **lst_ip
)
{
#define	IFCONFIG_CMD "/usr/sbin/ifconfig -a"
#define	LOOPBACK_IP4 "127.0.0.1"
#define	LOOPBACK_IP6 "::1/128"

	FILE *res_stream, *err_stream;
	char cmd[] = IFCONFIG_CMD
	    /* extract ip address for each adapter */
	    " | /usr/bin/awk '{ if ($1 ~ /^inet.*/) print $1 \" \" $2}'";
	int status;
	pid_t pid;
	char type[10], ip[50];


	Trace(TR_MISC, "discovering ip addrs");
	if (ISNULL(lst_ip)) {
		Trace(TR_ERR, "null argument found (addrs)");
		return (-1);
	}
	*lst_ip = lst_create();
	if (*lst_ip == NULL) {
		Trace(TR_ERR, "lst create failed");
		return (-1);
	}

	if (-1 == (pid = exec_get_output(cmd, &res_stream, &err_stream))) {
		Trace(TR_ERR, "Could not collect network info using %s",
		    IFCONFIG_CMD);
		return (-1);
	}

	while (EOF != fscanf(res_stream, "%s %s", type, ip)) {
		Trace(TR_DEBUG, "addrtype=%s ip=%s", type, ip);
		if (0 == strcmp(type, "inet")) { // IPv4
			if (0 != strcmp(ip, LOOPBACK_IP4)) {
				/* add this IP to the list */
				if (-1 == lst_append(*lst_ip, strdup(ip))) {
					Trace(TR_ERR,
					    "cannot create list element (ip)");
					fclose(res_stream);
					fclose(err_stream);
					return (-1);
				}
			}
		} else if (0 == strcmp(type, "inet6")) { // IPv6
			if (0 != strcmp(ip, LOOPBACK_IP6)) {
				/* add this IP to the list */
				ip[strcspn(ip, "/")] = '\0';
				if (-1 == lst_append(*lst_ip, strdup(ip))) {
					Trace(TR_ERR,
					    "cannot create list element (ip)");
					fclose(res_stream);
					fclose(err_stream);
					return (-1);
				}
			}
		}
	}
	fclose(res_stream);
	fclose(err_stream);
	waitpid(pid, &status, 0);
	Trace(TR_MISC, "IP address info obtained");
	return (0);
}


/*
 * get_architecture
 *
 */
int
get_architecture(
	ctx_t *ctx /* ARGSUSED */,
	char **arch)
{

	long chars = 0;
	char buf[257] = {0};

	if ((chars = sysinfo(SI_ARCHITECTURE, buf, sizeof (buf))) == -1) {
		/* failed to get the architecture */
		samerrno = SE_ARCHITECTURE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

		Trace(TR_OPRMSG, "get architecture failed");
		return (-1);

	} else {
		if (chars > sizeof (buf)) {
			char tbuf[chars];
			chars = sysinfo(SI_ARCHITECTURE, tbuf, sizeof (tbuf));
			if (chars == -1) {
				samerrno = SE_ARCHITECTURE_FAILED;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno));
				Trace(TR_OPRMSG, "get architecture failed");
				return (-1);
			}
			*arch = strdup(tbuf);
		} else {
			*arch = strdup(buf);
		}
	}
	return (0);
}


int
get_sc_version(
ctx_t *ctx /*ARGSUSED*/,
char **release)
{
	char buf[SC_RELEASE_MAXLEN];
	int i = 0;
	FILE *f = fopen(SC_RELEASE_FILE, "r");
	if (f == NULL) { // not a SC node
		*release = (char *)malloc(1);
		(*release)[0] = '\0';
		return (0);
	}
	if (NULL == fgets(buf, SC_RELEASE_MAXLEN, f)) {
		samerrno = SE_SC_CANT_GET_VER;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), SC_RELEASE_FILE);
		fclose(f);
		return (-1);
	}
	// eliminate heading whitespaces in release string
	while ((buf[i] == ' ' || buf[i] == '\t') && i < SC_RELEASE_MAXLEN)
		i++;
	// eliminate end-of-line char
	buf[strlen(buf) - 1] = '\0';

	*release = (char *)strdup(&buf[i]);
	fclose(f);
	return (1);
}


int
get_sc_name(
ctx_t *ctx /* ARGSUSED */,
char **cname)
{
	FILE *res_stream;
	pid_t pid;
	int status;
	char buf[128], cmd[] = SC_GET_CLUSTERNAME_CMD;

	Trace(TR_MISC, "getting cluster name");
	if (ISNULL(cname)) {
		Trace(TR_ERR, "null argument found (cname)");
		return (-1);
	}

	if (-1 == (pid = exec_get_output(cmd, &res_stream, NULL))) {
		*cname = NULL;
		samerrno = SE_SC_CANT_GET_NAME;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "Could not get cluster name info using %s", cmd);
		return (-1);
	}

	if (NULL == fgets(buf, sizeof (buf), res_stream)) {
		*cname = NULL;
		samerrno = SE_SC_CANT_GET_NAME;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "Cannot read output of %s", cmd);
		return (-1);
	}
	*cname = strdup(buf);

	fclose(res_stream);
	waitpid(pid, &status, 0);
	Trace(TR_MISC, "cluster name obtained");
	return (0);
}

static char *
get_sc_node_info(char *nodename, char *subcmd) {
	static char node_info[16];
	FILE *res_stream = NULL;
	char cmd[128+MAXHOSTNAMELEN];
	int status;
	pid_t pid;

	Trace(TR_MISC, "get sc node info for %s", Str(nodename));

	strcpy(node_info, "unknown");

	if (ISNULL(nodename)) {
		Trace(TR_ERR, "null argument found (nodename)");
		return (node_info);
	}
	if (ISNULL(subcmd)) {
		Trace(TR_ERR, "null argument found (subcmd)");
		return (node_info);
	}

	snprintf(cmd, sizeof (cmd), "%s -O %s %s", SC_CLUSTER_GET_CMD,
		subcmd, nodename);
	if (-1 == (pid = exec_get_output(cmd, &res_stream, NULL))) {
		samerrno = SE_SC_CANT_GET_NODES_STATUS;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "Could not get node info using %s", cmd);
		return (node_info);
	}

	fgets(node_info, sizeof (node_info), res_stream);

	fclose(res_stream);
	waitpid(pid, &status, 0);
	Trace(TR_MISC, "node status info obtained");
	return (node_info);
}

int
get_sc_nodes(
ctx_t *ctx /* ARGSUSED */,
sqm_lst_t **nodes)
{
	FILE *res_stream;
	char cmd1[] = SC_GET_NODES_NAMES_CMD,
	    cmd2[] = SC_GET_NODES_IDS_CMD;

	int status;
	pid_t pid;

	node_t *node;
	char nodeinfo[128*3], *crt_info;
	char name[128], nodeid[3];

	Trace(TR_MISC, "get sc nodes information");
	if (ISNULL(nodes)) {
		Trace(TR_ERR, "null argument found (nodes)");
		return (-1);
	}
	*nodes = lst_create();
	if (*nodes == NULL) {
		Trace(TR_ERR, "lst create failed");
		return (-1);
	}

	// STEP 1/2: get node names, then states (UP/DOWN) and priv. IPs
	if (-1 == (pid = exec_get_output(cmd1, &res_stream, NULL))) {
		samerrno = SE_SC_CANT_GET_NODES_NAMES;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "Could not get nodes names using %s", cmd1);
		return (-1);
	}

	while (NULL != fgets(name, sizeof (name), res_stream)) {
		Trace(TR_DEBUG, "nodename=%s", Str(name));
		snprintf(nodeinfo, sizeof (nodeinfo),
		    "sc_nodename=%s,sc_nodestatus=%s", name,
		    get_sc_node_info(name, "NODESTATE_NODE"));
		snprintf(nodeinfo, sizeof (nodeinfo),
		    "%s,sc_nodeprivaddr=%s", nodeinfo,
		    get_sc_node_info(name, "PRIVATELINK_HOSTNAME_NODE"));
		/* add this node name to the list */
		if (-1 == lst_append(*nodes, strdup(nodeinfo))) {
			Trace(TR_ERR, "cannot create list element (nodeinfo)");
			fclose(res_stream);
			return (-1);
		}
	}

	fclose(res_stream);
	waitpid(pid, &status, 0);
	Trace(TR_MISC, "node name info obtained");


	// STEP 2/2: Get node ID-s
	if (-1 == (pid = exec_get_output(cmd2, &res_stream, NULL))) {
		samerrno = SE_SC_CANT_GET_NODES_IDS;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "Could not get node ids using %s", cmd2);
		return (-1);
	}
	node = (*nodes)->head;
	while (NULL != fgets(nodeid, sizeof (nodeid), res_stream)) {
		Trace(TR_DEBUG, "nodeid=%s", Str(nodeid));
		crt_info = (char *)node->data;
		if (crt_info != NULL) {
			snprintf(nodeinfo, sizeof (nodeinfo),
			    "%s,sc_nodeid=%s", crt_info, nodeid);
			/* update node info */
			free(node->data);
			node->data = strdup(nodeinfo);
		}
		node = node->next;
	}
	fclose(res_stream);
	waitpid(pid, &status, 0);
	Trace(TR_MISC, "node id info obtained");

	return (0);
}



/*
 * -1 == unexpected error. details in samerrormsg
 *
 * PLEXMGR_RUNNING == everything ok (websrv running and spm registered)
 *  SC UI should be reachable at https://hostname:6789/SunPlexManager/
 *
 * PLEXMGR_INSTALLED -> user needs to start smcwebserver
 *
 * PLEXMGR_INSTALLED_NOT_REG ->
 *  SC UI must register with the webserver on the node
 *
 * PLEXMGROLD_INSTALLED
 *  SC UI (non-lockhart) should be reachable at https://<hostname>:3000/
 *
 * PLEXMGR_NOTINSTALLED -> user needs to install SunPlex Mgr packages
 */
int
get_sc_ui_state(
ctx_t *ctx /* ARGSUSED */)
{
	FILE *res_stream;
	char running[4] = "n/a", buf[2];
	int status;
	pid_t pid;
	sqm_lst_t *pkglst;
	char *res;

	Trace(TR_MISC, "getting webconsole state");
	if (-1 == (pid = exec_get_output(IS_WEBCON_RUNNING_CMD, &res_stream,
	    NULL))) {
		samerrno = SE_CANT_GET_WEBCONS_STATE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "Could not get webconsole state using %s",
		    IS_WEBCON_RUNNING_CMD);
		return (-1);
	}
	fgets(running, 4, res_stream);
	fclose(res_stream);
	Trace(TR_MISC, "webconsole running=%s", Str(running));
	waitpid(pid, &status, 0);


	// check if PlexMgr is registered app
	if (-1 == (pid = exec_get_output(SC_GET_SMREG_APP_CMD, &res_stream,
	    NULL))) {
		samerrno = SE_CANT_RUN_SMREG;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "Could not execute smreg (%s)",
		    SC_GET_SMREG_APP_CMD);
		return (-1);
	}
	res = fgets(buf, 2, res_stream);
	fclose(res_stream);
	waitpid(pid, &status, 0);
	if (NULL != res) // registered app
		if (0 == strcmp(Str(running), "yes")) // webconsole running
			return (PLEXMGR_RUNNING);
		else
			return (PLEXMGR_INSTALLED_AND_REG);
	else {
		// app not registered. check if pkgs are installed

		if (-1 == get_package_info(NULL, PLEXMGR_PKGS, &pkglst))
			return (-1);
		if (pkglst->length == 3) {
			lst_free_deep(pkglst);
			return (PLEXMGR_INSTALLED_NOT_REG);
		}
		lst_free_deep(pkglst);

		if (-1 == get_package_info(NULL, PLEXMGR_OLD_PKGS, &pkglst))
			return (-1);
		if (pkglst->length == 2) {
			lst_free_deep(pkglst);
			return (PLEXMGROLD_INSTALLED);
		}
		lst_free_deep(pkglst);

		return (PLEXMGR_NOTINSTALLED);
	}

}

#define	INTELLISTORE_FLAG_FILE "/var/opt/SUNWcistk/.isCIS"

/*
 * -1 means an error.
 * 0 means Intelligent archive features not enabled
 * 1 means Intelligent archive features are enabled
 */
int
intellistore_archive_enabled(ctx_t *ctx /* ARGSUSED */) {
	struct stat	st;
	static int	status = -1;

	if (status == -1) {
		if (stat(INTELLISTORE_FLAG_FILE, &st) != 0) {
			Trace(TR_OPRMSG, "stat of %s had error %d",
			    INTELLISTORE_FLAG_FILE, errno);

			if (ENOENT == errno) {
				status = 0;
			} else {
				status = -1;
			}
		} else {
			status = 1;
			Trace(TR_OPRMSG, "stat of %s succeeded",
			    INTELLISTORE_FLAG_FILE);
		}
	}
	return (status);
}
