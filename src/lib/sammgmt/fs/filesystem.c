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
#pragma ident   "$Revision: 1.85 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */



/*
 * filesystem.c
 * contains the implementations of methods declared in
 * filesystem.h.  This includes methods to create, grow and remove filesystems
 * and to change their mount options.
 *
 * It additionally contains implementations for functions in
 * mgmt/config/cfg_fs.h
 */

#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <time.h>

#include "mgmt/control/fscmd.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/sqm_list.h"
#include "sam/sam_trace.h"
#include "sam/mount.h"
#include "sam/syscall.h"
#include "sam/lib.h"

#include "pub/devstat.h"
#include "pub/mgmt/device.h"
#include "mgmt/config/master_config.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/filesystem.h"
#include "mgmt/config/vfstab.h"
#include "mgmt/config/cfg_fs.h"
#include "mgmt/config/mount_cfg.h"
#include "mgmt/util.h"
#include "pub/mgmt/license.h"
#include "pub/mgmt/archive.h"
#include "mgmt/config/archiver.h"
#include "pub/mgmt/archive_sets.h"
#include "pub/mgmt/hosts.h"
#include "mgmt/config/cfg_hosts.h"
#include "pub/mgmt/process_job.h"
#include "pub/mgmt/file_util.h"
#include "pub/mgmt/fsmdb_api.h"
#include "pub/mgmt/task_schedule.h"

boolean_t is_valid_eq(equ_t eq);
boolean_t is_valid_disk_name(upath_t name);

/* private functions. */
static void remove_eq(sqm_lst_t *l, equ_t eq);

static int
get_needed_eqs(mcf_cfg_t *mcf, equ_t fi_eq, const sqm_lst_t *data_disk_list,
    const sqm_lst_t *meta_data_disk_list, const sqm_lst_t *striped_group_list,
    sqm_lst_t **eqs);

static int setup_vfstab(fs_t *fs, boolean_t mount_at_boot);

static int grow_samfs_struct(mcf_cfg_t *mcf, fs_t *fs,
    const sqm_lst_t *new_data, sqm_lst_t **res_devs);

static int grow_qfs_struct(mcf_cfg_t *mcf, fs_t *fs, boolean_t mounted,
	const sqm_lst_t *new_metadata, const sqm_lst_t *new_data,
	const sqm_lst_t *striped_groups, sqm_lst_t **l);

static int find_striped_group(sqm_lst_t *l, uname_t name, striped_group_t **sg);

static int build_metadata_dev(sqm_lst_t *l, fs_t *fs, disk_t *disk,
	sqm_lst_t *eqs);

static int build_striped_group(sqm_lst_t *res_devs, fs_t *fs,
    striped_group_t *sg, sqm_lst_t *group_ids, sqm_lst_t *eqs);

static int build_qfs(mcf_cfg_t *mcf, fs_t *fs);

static int build_dev(sqm_lst_t *l, fs_t *fs, disk_t *disk, sqm_lst_t *eqs);

static int build_samfs(mcf_cfg_t *mcf, fs_t *fs);

static int make_vfstab_entry_for_fs(fs_t *fs, vfstab_entry_t **out);

static int verify_fs_unmounted(uname_t fs_name);

static int determine_datadevice_type(fs_t *fs, const sqm_lst_t *new_data,
	devtype_t dt);

static int precheck_shared_fs(fs_t *fs);

static int wait_for_sharefsd(char *fs_name);

static int create_ufs(fs_t *fs, boolean_t mount_at_boot); // since 4.4

static int create_fs_arch_set(char *fs_name, fs_arch_cfg_t *arc);

static int check_fs_arch_set(char *fs_name, fs_arch_cfg_t *arc);

static int internal_create_fs(ctx_t *ctx, fs_t *fs, boolean_t mount_at_boot,
    fs_arch_cfg_t *arc_info);

static int cmd_online_grow(char *fs_name, int eq);

static disk_t *find_disk_by_eq(fs_t *f, int eq);

static disk_t *find_disk_by_path(fs_t *f, char *path);

static int _set_device_state(fs_t *fs, sqm_lst_t *eqs, dstate_t new_state);

static int shrink(char *fs_name, int eq_to_exclude, int replacement_eq,
    char *kv_options, boolean_t release);

static int shrink_replace(ctx_t *c, char *fs_name, int eq_to_replace,
    disk_t *dk_replacement, striped_group_t *sg_replacement, char *kv_options);



static int _grow_fs(ctx_t *ctx, fs_t *fs, sqm_lst_t *new_meta,
    sqm_lst_t *new_data, sqm_lst_t *new_stripe_groups);

static int find_local_devices(sqm_lst_t *mms, sqm_lst_t *data, sqm_lst_t *sgs);





#define		DEFAULT_DATA_DISK_TYPE "md"
#define		META_DATA_DEVICE "mm"

#define	FSD_DISKVOLS	" -d "
#define	FSD_MCF		" -m "
#define	FSD_DEFAULTS	" -c "
#define	FSD_SAMFS	" -f "
#define	FSD_CMD		SBIN_DIR"/"SAM_FSD

static char *mcf_file = MCF_CFG;
static char *samfs_cmd_file = SAMFS_CFG;
static char *diskvols_file = DISKVOL_CFG;


/*
 * get a list of fs_t structures for all sam and qfs filesystems on the system.
 * This function gets all of its information from config files.
 */
int
cfg_get_all_fs(
sqm_lst_t **fs_list)	/* malloced list of all filesystems. */
{

	mount_cfg_t	*mount_cfg;
	int		ret = 0;


	Trace(TR_MISC, "get all filesystems from cfg");

	if (ISNULL(fs_list)) {
		Trace(TR_ERR, "get all filesystems from cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	if ((ret = read_mount_cfg(&mount_cfg)) == -1) {
		Trace(TR_ERR, "get all filesystems failed: %s", samerrmsg);
		return (-1);
	}

	/* set the return */
	*fs_list = mount_cfg->fs_list;

	/* set fs_list to NULL so it wont be freed. free rest of mount cfg */
	mount_cfg->fs_list = NULL;
	free_mount_cfg(mount_cfg);

	/*
	 * remove and free the global properties entry from the mount_cfg.
	 */
	if (strcmp(((fs_t *)(*fs_list)->head->data)->fi_name, GLOBAL) == 0) {

		free_fs((fs_t *)(*fs_list)->head->data);
		lst_remove(*fs_list, (*fs_list)->head);
	}

	Trace(TR_MISC, "got all filesystems");
	return (ret);
}


/*
 * get a list of all sam and qfs filesystem names.
 */
int
cfg_get_fs_names(
sqm_lst_t **fsys)		/* malloced list of uname_t */
{

	mcf_cfg_t	*mcf;
	sqm_lst_t		*l;
	Trace(TR_MISC, "get fs names");

	if (ISNULL(fsys)) {
		Trace(TR_ERR, "get fs names failed: %s", samerrmsg);
		return (-1);
	}
	if (read_mcf_cfg(&mcf) != 0) {
		/* Leave the samerrno that read set */
		Trace(TR_ERR, "get fs names failed: %s", samerrmsg);
		return (-1);
	}
	if (get_fs_family_set_names(mcf, &l) != 0) {
		/* Leave samerrno set */
		goto err;
	}

	*fsys = l;
	free_mcf_cfg(mcf);
	Trace(TR_MISC, "got fs names");
	return (0);

err:
	free_mcf_cfg(mcf);
	Trace(TR_ERR, "get fs names failed: %s", samerrmsg);
	return (-1);
}


/*
 * Get the filesystem named fsname, if it exists.
 */
int
cfg_get_fs(
uname_t fsname,	/* name of fs to get */
fs_t **fs)	/* malloced return */
{

	sqm_lst_t	*fs_list;
	node_t	*n;
	int err = 0;

	Trace(TR_MISC, "get fs %s", Str(fsname));
	if (ISNULL(fsname, fs)) {
		Trace(TR_ERR, "get fs failed: %s", samerrmsg);
		return (-1);
	}

	if ((err = cfg_get_all_fs(&fs_list)) == -1) {
		Trace(TR_ERR, "get fs failed: %s", samerrmsg);
		return (-1);
	}

	for (n = fs_list->head; n != NULL; n = n->next) {
		if (strcmp(((fs_t *)n->data)->fi_name, fsname) == 0) {
			*fs = (fs_t *)n->data;
			if (lst_remove(fs_list, n) != 0) {
				*fs = NULL;
				free_list_of_fs(fs_list);
				Trace(TR_ERR, "get fs failed: %s", samerrmsg);
				return (-1);
			}
			free_list_of_fs(fs_list);
			Trace(TR_MISC, "got fs returning %d", err);
			return (err);
		}
	}

	samerrno = SE_NOT_FOUND;
	/* %s not found */
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(SE_NOT_FOUND),
	    fsname);

	free_list_of_fs(fs_list);
	Trace(TR_ERR, "get fs failed: %s", samerrmsg);
	return (-1);
}


/*
 * Duplicate the fs structure mallocing space for the new structure and
 * copying all values.
 */
int
duplicate_fs(
fs_t *fs,		/* fs to copy */
fs_t **fs_copy)		/* malloced copy of fs */
{

	Trace(TR_DEBUG, "duplicate fs");

	if (ISNULL(fs, fs_copy)) {
		Trace(TR_ERR, "duplicate fs failed: %s", samerrmsg);
		return (-1);
	}

	(*fs_copy) = (fs_t *)mallocer(sizeof (fs_t));
	if (*fs_copy == NULL) {
		Trace(TR_ERR, "duplicate fs failed: %s", samerrmsg);
		return (-1);
	}
	memset(*fs_copy, 0, sizeof (fs_t));

	(*fs_copy)->mount_options =
	    (mount_options_t *)mallocer(sizeof (mount_options_t));

	if ((*fs_copy)->mount_options == NULL) {
		free_fs(*fs_copy);
		Trace(TR_ERR, "duplicate fs failed: %s", samerrmsg);
		return (-1);
	}



	memcpy(*fs_copy, fs, sizeof (fs_t));
	Trace(TR_DEBUG, "duplicated fs");
	return (0);
}


/*
 * change the mount options of a file system. This function
 * changes the mount options in the samfs.cmd and vfstab files.
 *
 * The caller of this function should set the change flag for any field
 * which they want written into the config files. Any field that does not
 * have its change_flag bit set will keep any existing setting in the
 * config files. This behavior is to support a live config that is different
 * from the written configuration files.
 *
 * This function sets all options(with change_flag bits) in the samfs.cmd
 * file. It only sets and unsets things in the vfstab file if a vfstab entry
 * exists for the filesystem and includes a mount option for which the input
 * options have their change flag set.
 *
 * If the ctx argument is non-null and contains a non-empty dump_path,
 * this function will not modify the existing samfs.cmd and vfstab files.
 * Instead it will write the modified samfs.cmd file to the path included in
 * the ctx.
 */
int
change_mount_options(
ctx_t *ctx,			/* contains the optional dump path */
uname_t fsname,			/* name of fs for which to change options */
mount_options_t *options)	/* options to set */
{

	fs_t *fs;

	Trace(TR_MISC, "change mount options for %s", fsname);

	if (ISNULL(fsname, options)) {
		Trace(TR_ERR, "change mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (cfg_get_fs(fsname, &fs) == -1) {
		/* leave samerrno as set. */
		Trace(TR_ERR, "change mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* free the old options and replace with the new. */
	free(fs->mount_options);
	fs->mount_options = options;

	if (cfg_change_mount_options(ctx, fs, B_FALSE) == -1) {
		/* don't free the options that were passed in */
		fs->mount_options = NULL;
		free_fs(fs);
		Trace(TR_ERR, "change mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (ctx != NULL && *ctx->dump_path != '\0') {
		/* don't free the options that were passed in */
		fs->mount_options = NULL;
		free_fs(fs);
		Trace(TR_MISC, "changed mount options in %s",
		    ctx->dump_path);
		return (0);
	}
	/* sighup the daemon */
	if (init_config(ctx) != 0) {
		/* don't free the options that were passed in */
		fs->mount_options = NULL;
		free_fs(fs);
		Trace(TR_ERR, "change mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	fs->mount_options = NULL;
	free_fs(fs);
	Trace(TR_MISC, "changed mount options for %s", fsname);
	return (0);
}


/*
 * private function to create an fs struct given a list of devices and a name.
 */
int
create_fs_struct(
char *fs_name,	/* filesystem name */
sqm_lst_t *devs,	/* list of base_dev_t to include in fs */
fs_t **fs)	/* malloced return */
{

	base_dev_t	*fs_dev, *tmp;
	disk_t		*disk;
	striped_group_t *sg;
	node_t		*n;

	Trace(TR_DEBUG, "create fs struct for %s", fs_name);
	if (ISNULL(fs_name, fs)) {
		Trace(TR_ERR, "create fs struct failed: %s", samerrmsg);
		return (-1);
	}

	/* create the struct */
	*fs = (fs_t *)mallocer(sizeof (fs_t));
	if (*fs == NULL) {
		Trace(TR_ERR, "create fs struct failed: %s", samerrmsg);
		return (-1);
	}


	memset(*fs, 0, sizeof (fs_t));
	(*fs)->meta_data_disk_list = lst_create();
	(*fs)->data_disk_list = lst_create();
	(*fs)->striped_group_list = lst_create();
	(*fs)->mount_options = (mount_options_t *)mallocer(
	    sizeof (mount_options_t));

	if ((*fs)->meta_data_disk_list == NULL ||
	    (*fs)->data_disk_list == NULL ||
	    (*fs)->striped_group_list == NULL ||
	    (*fs)->mount_options == NULL) {

		goto err;
	}


	memset((*fs)->mount_options, 0, sizeof (mount_options_t));

	/* no work to do */
	if (devs == NULL) {
		strcpy((*fs)->fi_name, fs_name);
		return (0);
	}

	/* populate it */
	for (n = devs->head; n != NULL; n = n->next) {
		tmp = (base_dev_t *)n->data;

		if (strcmp(tmp->name, fs_name) == 0) {
			fs_dev = tmp;
		} else {
			int num_type;

			disk = (disk_t *)mallocer(sizeof (disk_t));
			if (disk == NULL) {
				goto err;
			}

			memset(disk, 0, sizeof (disk_t));
			dev_cpy(&(disk->base_info), tmp);
			num_type = nm_to_dtclass(tmp->equ_type);

			if (num_type == DT_META) {
				/*
				 * if it is a nodev device setup fi_status
				 */
				if (strcmp(tmp->name, NODEV_STR) == 0) {
					(*fs)->fi_status |= FS_NODEVS;
					(*fs)->fi_status |= FS_CLIENT;
				}

				if (lst_append((*fs)->meta_data_disk_list,
				    disk) != 0) {

					free(disk);
					goto err;
				}

			} else if (is_stripe_group(num_type) ||
			    is_osd_group(num_type)) {
				if (find_striped_group(
				    (*fs)->striped_group_list,
				    tmp->equ_type, &sg) != 0) {

					free(disk);
					goto err;
				}

				if (lst_append(sg->disk_list, disk) != 0) {
					free(disk);
					free_striped_group(sg);
					goto err;
				}
			} else if (num_type == DT_DATA ||
			    num_type == DT_RAID) {

				if (lst_append((*fs)->data_disk_list,
				    disk) != 0) {
					free(disk);
					goto err;
				}
			}
		}
	}

	/* set up the fs info from fs_dev and list lengths */
	strcpy((*fs)->fi_name, fs_dev->name);
	(*fs)->fs_count = devs->length - 1;
	(*fs)->mm_count = (*fs)->meta_data_disk_list->length;
	(*fs)->fi_eq = fs_dev->eq;
	(*fs)->fi_state = fs_dev->state;
	strcpy((*fs)->equ_type, fs_dev->equ_type);
	if (strcmp(fs_dev->additional_params, "shared") == 0) {
		(*fs)->fi_shared_fs = B_TRUE;

		/*
		 * if nodevs is set we know it is a client. Otherwise
		 * set the shared status flags
		 */
		if (!((*fs)->fi_status & FS_NODEVS)) {
			if (set_shared_fs_status_flags(*fs) != 0) {
				return (-2);
			}
		}
	}

	Trace(TR_DEBUG, "created fs struct");
	return (0);

err:
	free_fs(*fs);
	Trace(TR_ERR, "create fs struct failed: %s", samerrmsg);
	return (-1);

}


int
set_shared_fs_status_flags(fs_t *f) {
	node_t *n;
	upath_t host_name;
	boolean_t found = B_FALSE;
	sqm_lst_t *hosts;

	Trace(TR_MISC,
	    "setting fs status bits for unmounted shared file system %s",
	    f->fi_name);

	if (!f->fi_shared_fs) {
		samerrno = SE_ONLY_SHARED_HAVE_HOSTS;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ONLY_SHARED_HAVE_HOSTS), f->fi_name);
		Trace(TR_ERR, "setting shared status flags failed: %s",
		    samerrmsg);
		return (-1);
	}


	if (cfg_get_hosts_config(NULL, f, &hosts, 0) != 0) {
		Trace(TR_ERR, "getting shared status flags failed: %s",
		    samerrmsg);
		return (-1);
	}

	gethostname(host_name, sizeof (host_name));

	Trace(TR_MISC, "host name is %s", Str(host_name));
	if (hosts != NULL) {
		for (n = hosts->head; n != NULL && found != B_TRUE;
			n = n->next) {

			host_info_t *hi = (host_info_t *)n->data;

			Trace(TR_MISC, "considering host %s", hi->host_name);

			if (strcasecmp(hi->host_name, host_name) == 0) {
				found = B_TRUE;
				Trace(TR_MISC, "found the host %s", host_name);
				if (hi->current_server) {
					f->fi_status |= FS_SERVER;
				} else {
					f->fi_status |= FS_CLIENT;
					Trace(TR_MISC, "set FS_CLIENT for %s",
					    f->fi_name);

					if (hi->server_priority == 0) {
						/*
						 * it cannot be the
						 * metadata server
						 */
						f->fi_status |= FS_NODEVS;
					}
				}
			}
		}
	}
	Trace(TR_MISC, "done considering hosts");

	free_list_of_host_info(hosts);

	if (!found) {
		samerrno = SE_UNABLE_TO_DETERMINE_HOST_STATUS;
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_UNABLE_TO_DETERMINE_HOST_STATUS),
			host_name, f->fi_name);
		Trace(TR_ERR, "setting shared fs status failed: %d %s",
			samerrno, samerrmsg);
		return (-1);
	}
	return (0);
}

/*
 * function to remove a SAM-FS/QFS filesystem from the configuration.
 * This function removes the filesystem from the mcf, samfs.cmd and vfstab.
 * Issues:
 * 1. Does not remove other configuration related information for this fs.
 *    for instance archiving policy.
 *
 * NOTE: this API is deprecated. use remove_generic_fs() instead.
 */
int
remove_fs(
ctx_t *ctx		/* ARGSUSED */,
uname_t fs_name)	/* name of fs to remove */
{

	mount_cfg_t *mnt_cfg;
	node_t *n;
	boolean_t found = B_FALSE;
	int ret = 0;
	int dbres;
	char buf[MAXPATHLEN + 1];


	Trace(TR_MISC, "remove fs (%s) entry", Str(fs_name));

	if (ISNULL(fs_name)) {
		Trace(TR_ERR, "remove fs failed: %s", samerrmsg);
		return (-1);
	}

	/* check to see if the fs is mounted */
	if (verify_fs_unmounted(fs_name) != 0) {
		Trace(TR_ERR, "remove fs failed: %s", samerrmsg);
		return (-1);
	}


	/* remove any metrics schedules */
	snprintf(buf, sizeof (buf), "task=RP,id=%s", fs_name);
	dbres = remove_task_schedule(NULL, buf);
	if (dbres != 0) {
		/* doesn't affect anything else */
		Trace(TR_ERR, "deleting metrics schedule failed: %d", dbres);
	}

	/* delete any database entries for this filesystem */
	dbres = delete_fs_from_db(fs_name);
	if (dbres != 0) {
		/* for now, doesn't affect anything else */
		Trace(TR_ERR, "deleting fsmdb entries failed: %d", dbres);
	}

	if (read_mount_cfg(&mnt_cfg) == -1) {
		Trace(TR_OPRMSG, "resetting default mount options failed: %s",
		    samerrmsg);
		return (-1);
	}


	/*
	 * find the file system and set flags to verify its presence and
	 * determine wether to update the host's file.
	 */
	for (n = mnt_cfg->fs_list->head; n != NULL; n = n->next) {
		fs_t *fs = (fs_t *)n->data;
		if (strcmp(fs->fi_name, fs_name) == 0) {
			found = B_TRUE;

		}
	}
	if (found != B_TRUE) {
		samerrno = SE_NOT_FOUND;
		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), fs_name);

		free_mount_cfg(mnt_cfg);
		Trace(TR_ERR, "remove fs failed: %s", samerrmsg);
		return (-1);
	}

	/* Remove the samfs.cmd entry if any */
	if (reset_default_mount_options(ctx, mnt_cfg, fs_name) != 0) {
		free_mount_cfg(mnt_cfg);
		Trace(TR_ERR, "remove fs failed: %s", samerrmsg);
		return (-1);
	}
	free_mount_cfg(mnt_cfg);


	/* rewrite the mcf without the file system */
	if (remove_family_set(ctx, fs_name) != 0) {
		return (-1);
	}

	/*
	 * if there is a ctx and its dump path is not empty,
	 * return success without sighup or deleting the vfstab
	 * file entry(we dont support vfstab dumping now).
	 */
	if (ctx != NULL && *ctx->dump_path != '\0') {
		Trace(TR_MISC, "removed fs %s in dump path %s", fs_name,
		    ctx->dump_path);
		return (0);
	}


	/*
	 * Remove the vfstab entry last because it can contain an entry
	 * without causing errors in the other config files.
	 */
	samerrno = 0;
	if (delete_vfstab_entry(fs_name) != 0) {

		/* if entry not found it is not an error. */
		if (samerrno != SE_NOT_FOUND) {
			/* leave samerrno as set */
			Trace(TR_ERR, "remove fs failed: %d %s", samerrno,
			    samerrmsg);
			return (-1);
		}
		Trace(TR_ERR, "delete_vfstab_entry %s failed: SE_NOT_FOUND %s",
		    fs_name, "returning success");
	}

	if (init_library_config(ctx) != 0) {
		Trace(TR_ERR, "remove fs failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "removed fs %s", fs_name);
	return (ret);

}


/*
 * verify_fs_unmounted will return an error if the named filesystem is mounted.
 */
static int
verify_fs_unmounted(uname_t fs_name)
{

	boolean_t mounted = B_FALSE;

	Trace(TR_MISC, "checking if %s is mounted()", fs_name);

	if (ISNULL(fs_name)) {
		return (-1);
	}

	if (is_fs_mounted(NULL, fs_name, &mounted) == -1) {
		return (-1);
	}

	if (mounted) {
		samerrno = SE_FS_MOUNTED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FS_MOUNTED), fs_name);

		Trace(TR_ERR, "check if %s is mounted failed: %s",
		    fs_name, samerrmsg);

		return (-1);
	}

	return (0);
}


/*
 * Internal grow file system function. The only difference between this
 * and grow_fs is that this function trusts that the fs argument is
 * up to date.
 */
static int
_grow_fs(
ctx_t *ctx,
fs_t *fs,				/* fs to grow */
sqm_lst_t *new_meta,		/* list of disk_t */
sqm_lst_t *new_data,		/* list of disk_t */
sqm_lst_t *new_stripe_groups)	/* list of striped_group_t */
{

	mcf_cfg_t *mcf;
	base_dev_t *dev;
	int num_type;
	sqm_lst_t *res_devs = NULL;
	boolean_t mounted = B_FALSE;
	int ret_val = 0;

	if (ISNULL(fs)) {
		Trace(TR_ERR, "growing fs failed: %s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "growing fs %s", Str(fs->fi_name));

	if (fs->fi_status & FS_MOUNTED) {
		mounted = B_TRUE;
	}


	/* If the fs is mounted check that it will support the online grow */
	if (mounted && fs->fi_version == 1) {
		setsamerr(SE_ONLINE_GROW_FAILED_SBLK_V1);
		Trace(TR_ERR, "growing fs failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * Check if this is a shared file system- if so and this is
	 * not the metadata server, locate the devices on this host and
	 * rewrite the device paths to match the local ones.
	 */
	if (fs->fi_shared_fs && !(fs->fi_status & FS_SERVER)) {
		if (find_local_devices(new_meta, new_data,
		    new_stripe_groups) != 0) {
			Trace(TR_ERR, "Matching devices were not found: %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
	}

	if (read_mcf_cfg(&mcf) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "growing fs failed: %s", samerrmsg);
		return (-1);
	}

	/* make sure the fs already exists. */
	if (get_mcf_dev(mcf, fs->fi_name, &dev) != 0) {
		free_mcf_cfg(mcf);

		Trace(TR_ERR, "growing fs failed: %s", samerrmsg);
		return (-1);
	}
	free(dev);

	num_type = nm_to_dtclass(fs->equ_type);
	if (num_type == DT_DISK_SET) {
		if ((new_meta != NULL && new_meta->length != 0) ||
		    (new_stripe_groups != NULL &&
		    new_stripe_groups->length != 0)) {

			samerrno = SE_INVALID_EQ_TYPE;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_INVALID_EQ_TYPE),
				    "mm or gXX");
				goto err;
		}
		if (grow_samfs_struct(mcf, fs, new_data, &res_devs) != 0) {
			goto err;
		}
	} else if (num_type == DT_META_SET || num_type == DT_META_OBJECT_SET ||
	    num_type == DT_META_OBJ_TGT_SET) {
		if (grow_qfs_struct(mcf, fs, mounted, new_meta, new_data,
		    new_stripe_groups, &res_devs) != 0) {
			goto err;
		}
	} else {
		samerrno = SE_INVALID_FS_TYPE;
		/* %s is not a valid filesystem type */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_FS_TYPE), fs->equ_type);
		goto err;
	}

	if (append_to_family_set(ctx, fs->fi_name, res_devs) != 0) {
		goto err;
	}


	/*
	 * if there is a ctx and its dump path is not empty,
	 * return success without sighup or calling grow.
	 */
	if (ctx != NULL && *ctx->dump_path != '\0') {
		free_mcf_cfg(mcf);
		lst_free_deep(res_devs);
		Trace(TR_MISC, "grow fs %s in %s", Str(fs->fi_name),
		    ctx->dump_path);
		return (0);
	}

	/* sighup the daemon */
	if (init_library_config(ctx) != 0) {
		goto err;
	}


	/*
	 * If the file system is shared and this host is not the
	 * metadata server there is no more work to do so clean up
	 * and return.
	 */
	if (fs->fi_shared_fs && !(fs->fi_status & FS_SERVER)) {
		lst_free_deep(res_devs);
		free_mcf_cfg(mcf);
		Trace(TR_MISC, "grew fs %s", Str(fs->fi_name));
		return (0);
	}


	if (mounted) {
		node_t *n;
		char *last_group = NULL;

		/* Loop over the new devices calling online grow for each */
		for (n = res_devs->head; n != NULL; n = n->next) {
			base_dev_t *dev = (base_dev_t *)n->data;

			/*
			 * It is only necessary to call grow for the
			 * first dev in a striped group. Make sure we
			 * don't double call as the command will
			 * return an error on the second call.
			 */
			if ((*(dev->equ_type) == 'g') ||
			    (*(dev->equ_type) == 'o')) {

				if (last_group != NULL &&
				    strcmp(dev->equ_type, last_group) == 0) {
					/*
					 * Skip devs from the group
					 * beyond the first
					 */
					continue;
				} else {
					/*
					 * set last_group for this first
					 * device of a group and go on to
					 * call grow.
					 */
					last_group = dev->equ_type;
				}

			}
			if (cmd_online_grow(fs->fi_name, dev->eq) != 0) {
				goto err;
			}
		}
	} else {
		/* call grow */
		if (fs_grow(fs->fi_name) != 0) {
			goto err;
		}
	}

	lst_free_deep(res_devs);
	free_mcf_cfg(mcf);


	/*
	 * If this is the mds of a shared file system setup the call to grow
	 * on the clients.
	 */
	if (fs->fi_shared_fs && (fs->fi_status & FS_SERVER)) {

		ret_val = grow_shared_fs_on_clients(ctx, fs, new_meta, new_data,
		    new_stripe_groups);

		Trace(TR_MISC, "growing shared fs returns jobid %d", ret_val);
	}

	Trace(TR_MISC, "grew fs %s", Str(fs->fi_name));
	return (ret_val);

err:
	lst_free_deep(res_devs);
	free_mcf_cfg(mcf);
	Trace(TR_ERR, "growing fs failed: %s", samerrmsg);
	return (-1);
}


int
grow_fs(
ctx_t *ctx,
fs_t *fs,				/* fs to grow */
sqm_lst_t *new_meta,		/* list of disk_t */
sqm_lst_t *new_data,		/* list of disk_t */
sqm_lst_t *new_stripe_groups)	/* list of striped_group_t */
{

	fs_t *live_fs;
	int ret_val;

	if (ISNULL(fs)) {
		Trace(TR_ERR, "growing fs failed: %s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "growing fs %s", Str(fs->fi_name));


	/*
	 * Don't simply rely on the argument, get the fs from the system
	 * so that its status is fresh. The argument from the user could
	 * be out of date.
	 */
	if (get_fs(NULL, fs->fi_name, &live_fs) == -1) {
		Trace(TR_ERR, "growing fs failed: %s", samerrmsg);
		return (-1);
	}


	ret_val = _grow_fs(ctx, live_fs, new_meta, new_data,
	    new_stripe_groups);

	free_fs(live_fs);
	Trace(TR_MISC, "grow fs %s returning %d", Str(fs->fi_name),
	    ret_val);
	return (ret_val);
}


/*
 * fills in the data_disk structures and inserts them into the mcf at
 * the appropriate place(after all other members of the fs).
 * inserts the disks into the fs structure as well.
 */
static int
grow_samfs_struct(
mcf_cfg_t *mcf,		/* data source for decision making */
fs_t *fs,		/* fs to grow */
const sqm_lst_t *new_data,	/* list of disk_t */
sqm_lst_t **res_devs)	/* list of base_dev_t */
{

	node_t *n;
	sqm_lst_t *eqs = NULL;

	Trace(TR_DEBUG, "grow samfs struct entry");
	if (new_data == NULL || new_data->length == 0) {

		samerrno = SE_FS_CONTAINS_NO_DEVICES;

		/* Filesystem %s contains no devices */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FS_CONTAINS_NO_DEVICES), fs->fi_name);
		Trace(TR_ERR, "grow samfs struct failed: %s", samerrmsg);
		return (-1);
	}

	*res_devs = lst_create();
	if (res_devs == NULL) {
		Trace(TR_ERR, "grow samfs struct failed: %s", samerrmsg);
		return (-1);
	}


	/* get needed eqs. */
	if (get_needed_eqs(mcf, fs->fi_eq, new_data, NULL, NULL, &eqs) != 0) {
		Trace(TR_ERR, "grow samfs struct failed: %s", samerrmsg);
		return (-1);
	}

	/* build each data device. */
	for (n = new_data->head; n != NULL; n = n->next) {
		/* insert into mcf and fs */
		if (build_dev(*res_devs, fs, (disk_t *)n->data,
		    eqs) != 0) {

			lst_free_deep(eqs);
			Trace(TR_ERR, "grow samfs struct failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	lst_free_deep(eqs);
	Trace(TR_DEBUG, "grew samfs struct");
	return (0);
}


/*
 * This function does not check for overall validity of the fs.
 * If there is an invalid device mix before it is called the call
 * to this function will not detect it and will return a device type based
 * on the first type that it encounters in the existing fs.
 */
int
determine_datadevice_type(
fs_t *fs,
const sqm_lst_t *new_data,
devtype_t dt)
{

	node_t *n;
	disk_t *disk;
	int tmp_dev_type;

	/* finding one md device means type is md. */
	/* finding one mr or gxx device means mr. */
	if (fs->data_disk_list == NULL || fs->data_disk_list->length == 0) {
		if (fs->striped_group_list != NULL &&
		    fs->striped_group_list->length != 0) {
			strcpy(dt, "mr");
			return (0);
		}
	} else {
		for (n = fs->data_disk_list->head; n != NULL; n = n->next) {
			disk = (disk_t *)n->data;
			tmp_dev_type = nm_to_dtclass(disk->base_info.equ_type);
			if (tmp_dev_type == DT_DATA) {
				strcpy(dt, "md");
				return (0);
			} else if (tmp_dev_type == DT_RAID) {
				strcpy(dt, "mr");
				return (0);
			}
		}
	}

	/*
	 * default to md if nothing else has been selected and the inputs do
	 * not have any specification
	 */
	for (n = new_data->head; n != NULL; n = n->next) {
		disk = (disk_t *)n->data;
		tmp_dev_type = nm_to_dtclass(disk->base_info.equ_type);
		if (tmp_dev_type == DT_RAID) {
			strcpy(dt, "mr");
			return (0);
		} else if (tmp_dev_type == DT_DATA) {
			strcpy(dt, "md");
			return (0);
		}
	}
	strcpy(dt, "md");
	return (0);
}

/*
 * fills in the data_disk structures and inserts them into the mcf at
 * the appropriate place(after all other members of the fs).
 * inserts the disks into the fs structure as well.
 */
static int
grow_qfs_struct(
mcf_cfg_t *mcf,			/* input for decision making */
fs_t *fs,			/* fs struct to modify */
boolean_t mounted,
const sqm_lst_t *new_metadata,	/* list of disk_t */
const sqm_lst_t *new_data,		/* list of disk_t */
const sqm_lst_t *striped_groups,	/* list of striped_group_t */
sqm_lst_t **res_devs)		/* list of striped_group_t */
{

	node_t		*n;
	sqm_lst_t		*eqs = NULL;
	sqm_lst_t		*group_ids = NULL;

	Trace(TR_DEBUG, "grow qfs struct %s", fs->fi_name);
	if (!mounted && (new_metadata == NULL || new_metadata->length == 0)) {
		/* no devices */
		samerrno = SE_FS_METADATA_DEV_REQUIRED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FS_METADATA_DEV_REQUIRED));

		Trace(TR_ERR, "grow qfs struct failed: %s", samerrmsg);
		return (-1);
	}


	*res_devs = lst_create();
	if (*res_devs == NULL) {
		Trace(TR_ERR, "grow qfs struct failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * Determine the data type for the data disks.
	 */
	if (new_data != NULL) {
		devtype_t dt;

		if (determine_datadevice_type(fs, new_data, dt) != 0) {
			lst_free_deep(*res_devs);
			*res_devs = NULL;
			Trace(TR_ERR, "grow qfs struct failed: %s", samerrmsg);
			return (-1);
		}

		/* Set the equ type for the disks in the new_data list */
		for (n = new_data->head; n != NULL; n = n->next) {
			if (ISNULL(n->data)) {
				lst_free_deep(*res_devs);
				*res_devs = NULL;
				Trace(TR_ERR, "grow qfs struct failed: %s",
				    samerrmsg);
				return (-1);
			}
			strlcpy(((disk_t *)n->data)->base_info.equ_type, dt,
			    sizeof (devtype_t));
		}
	}


	/* get needed eqs for the new devices */
	if (get_needed_eqs(mcf, fs->fi_eq, new_data,
	    new_metadata, striped_groups, &eqs) != 0) {
		lst_free_deep(*res_devs);
		*res_devs = NULL;
		Trace(TR_ERR, "grow qfs struct failed: %s", samerrmsg);
		return (-1);
	}

	/* Build the metadata devs. */
	if (new_metadata != NULL && new_metadata->length != 0) {

		Trace(TR_DEBUG, "about to build meta data disks");
		for (n = new_metadata->head; n != NULL; n = n->next) {
			if (build_metadata_dev(*res_devs, fs,
			    (disk_t *)n->data, eqs) != 0) {

				lst_free_deep(eqs);
				lst_free_deep(*res_devs);
				Trace(TR_ERR, "grow qfs struct failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	/* Build the data devs. */
	if (new_data != NULL && new_data->length != 0) {

		Trace(TR_DEBUG, "grow qfs struct build data disks");
		for (n = new_data->head; n != NULL; n = n->next) {
			if (build_dev(*res_devs, fs, (disk_t *)n->data,
			    eqs) != 0) {

				lst_free_deep(*res_devs);
				lst_free_deep(eqs);
				Trace(TR_ERR, "grow qfs struct failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}


	/*
	 * Now build the striped group devs.
	 * Note that these get inserted in sortof reverse order because
	 * ins_after is not updated after each striped group is added.
	 */
	if (striped_groups != NULL && striped_groups->length != 0) {

		/*
		 * Get striped group ids if needed. This block must be outside
		 * build_striped_group to support multiple new striped groups.
		 */

		int num_type = nm_to_dtclass(fs->equ_type);
		boolean_t object_groups = B_FALSE;

		Trace(TR_DEBUG, "about to build stiped groups");
		if (num_type == DT_META_OBJECT_SET) {
			object_groups = B_TRUE;
		}
		if (get_available_striped_group_id(mcf, fs->fi_name,
		    &group_ids, striped_groups->length, object_groups) != 0) {

			lst_free_deep(eqs);
			Trace(TR_ERR, "grow qfs struct failed: %s",
			    samerrmsg);
			return (-1);
		}

		for (n = striped_groups->head; n != NULL; n = n->next) {

			if (build_striped_group(*res_devs, fs,
			    (striped_group_t *)n->data, group_ids,
			    eqs) != 0) {

				lst_free_deep(eqs);
				lst_free_deep(group_ids);
				Trace(TR_ERR, "grow qfs struct failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	lst_free_deep(eqs);
	lst_free_deep(group_ids);
	Trace(TR_DEBUG, "grew qfs struct");
	return (0);
}


/*
 * create_fs() is for creating a file system. This single method can
 * be used to create all flavors of SAM-FS/QFS file systems.
 *
 * The type of the file system to be created can be derived from the
 * list of devices provided in fs data structure. Same thing for
 * mount options.
 *
 * The caller of this function can initialize the structures in
 * one of two ways.
 *
 * Each disk_t can contain only the au_t info. In this case
 * all eq_ordinals and device types will be selected by the api. Ordinals
 * will be the selected on a lowest available basis. Data disks will all
 * be md devices. Striped groups will take the lowest available striped
 * group id.
 *
 * Alternatively the base_info member of each disk can be set up to indicate
 * the equipment type and equipment ordinal of the device.
 *
 * If the ctx argument is non-null and contains a non-empty string dump_path
 * this function will not modify the existing mcf, samfs.cmd and vfstab files.
 * Instead it will write the mcf and samfs.cmd files to the path included in
 * the ctx.
 */
int
create_fs(
ctx_t *ctx,			/* contains the optional dump path */
fs_t *fs,			/* the fs_t to create */
boolean_t mount_at_boot)	/* if true mount at boot goes in vfstab */
{
	return (internal_create_fs(ctx, fs, mount_at_boot, NULL));
}


/*
 * Function to handle the actual creation of the file system,
 * configuration of mount options and setup of archiver.cmd
 */
static int
internal_create_fs(
ctx_t		*ctx,		/* contains the optional dump path */
fs_t		*fs,		/* the fs_t to create */
boolean_t	mount_at_boot,	/* if true mount at boot goes in vfstab */
fs_arch_cfg_t	*arc_info)
{


	mcf_cfg_t	*mcf;
	boolean_t	modify_vfstab = B_TRUE;
	base_dev_t	*dev = NULL;
	sqm_lst_t		*devs = NULL;
	int		num_type;
	boolean_t	dump = B_FALSE;
	vfstab_entry_t	*vfs_ent = NULL;
	int		error = 0;


	Trace(TR_MISC, "creating fs %s", fs ? fs->fi_name : "NULL");

	if (ISNULL(fs)) {
		Trace(TR_ERR, "create fs failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "fs shared flag is %s",
	    fs->fi_shared_fs ? "shared":"not shared");
	Trace(TR_MISC, "fs eq = %d, flags = %d", fs->fi_eq, fs->fi_status);

	if (read_mcf_cfg(&mcf) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "create fs failed: %s", samerrmsg);
		return (-1);
	}

	if (fs->fi_status & FS_SERVER ||
	    fs->fi_status & FS_CLIENT ||
	    fs->fi_status & FS_NODEVS) {

		fs->fi_shared_fs = B_TRUE;
	}

	if (fs->fi_shared_fs) {
		if (precheck_shared_fs(fs) != 0) {
			free_mcf_cfg(mcf);
			Trace(TR_ERR, "create fs failed: %s", samerrmsg);
			return (-1);
		}

		if (!(fs->fi_status & FS_SERVER)) {
			if (find_local_devices(fs->meta_data_disk_list,
			    fs->data_disk_list, fs->striped_group_list) != 0) {
				return (-1);
			}
		}
	}

	/* check to see if a family set with same name exists in the mcf */
	if (get_family_set_devs(mcf, fs->fi_name, &devs) == 0) {
		free_mcf_cfg(mcf);
		lst_free_deep(devs);
		samerrno = SE_FAMILY_SET_ALREADY_EXISTS;

		/* Family set %s already exists */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FAMILY_SET_ALREADY_EXISTS), fs->fi_name);

		Trace(TR_ERR, "create fs failed: %s", samerrmsg);
		return (-1);
	}

	/* check to see if there is a vfstab entry */
	if (modify_vfstab && get_vfstab_entry(fs->fi_name, &vfs_ent) == 0) {

		free_mcf_cfg(mcf);
		free(vfs_ent);

		/* A vfstab entry already exists for file system %s */
		samerrno = SE_VFSTAB_ENTRY_ALREADY_EXISTS;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_VFSTAB_ENTRY_ALREADY_EXISTS),
		    fs->fi_name);

		Trace(TR_ERR, "create fs failed: %s", samerrmsg);
		return (-1);
	}

	if (get_mcf_dev(mcf, fs->fi_name, &dev) == 0) {
		free_mcf_cfg(mcf);
		free(dev);
		samerrno = SE_DEVICE_ALREADY_EXISTS;

		/* Device %s already exists */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_DEVICE_ALREADY_EXISTS), fs->fi_name);

		Trace(TR_ERR, "create fs failed: %s", samerrmsg);
		return (-1);
	}

	/* determine type */
	num_type = nm_to_dtclass(fs->equ_type);

	/* create and write by type */
	if (num_type == DT_DISK_SET) {
		Trace(TR_DEBUG, "build sam");

		/* its sam only */
		if (build_samfs(mcf, fs) != 0) {
			goto err;
		}
	} else if (num_type == DT_META_SET ||
	    num_type == DT_META_OBJECT_SET ||
	    num_type == DT_META_OBJ_TGT_SET) {
		Trace(TR_DEBUG, "build qfs");
		/* its qfs or sam-qfs */
		if (build_qfs(mcf, fs) != 0) {
			goto err;
		}
	} else {
		/* %s is not a valid filesystem type */
		samerrno = SE_INVALID_FS_TYPE;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_FS_TYPE), fs->equ_type);
		goto err;
	}


	/* now check the devices in the fs against the super blocks on them */
	if (fs->fi_shared_fs && !(fs->fi_status & FS_SERVER) &&
	    num_type != DT_META_OBJECT_SET) {
		if (shared_fs_check(ctx, fs->fi_name, mcf) != 0) {
			goto err;
		}
	}

	devs = NULL;
	if (get_family_set_devs(mcf, fs->fi_name, &devs) != 0) {
		goto err;
	}

	if (add_family_set(ctx, devs) != 0) {
		lst_free_deep(devs);
		goto err;
	}

	lst_free_deep(devs);

	/* setup the vfstab entry if this is not a dump */
	dump = ctx != NULL && *ctx->dump_path != '\0';
	if (!dump && modify_vfstab) {
		if (setup_vfstab(fs, mount_at_boot) != 0) {
			goto err;
		}
	}

	/* update samfs.cmd */
	if ((cfg_change_mount_options(ctx, fs, B_TRUE)) == -1) {
		goto err;
	}

	free_mcf_cfg(mcf);
	mcf = NULL;

	/*
	 * if this is a dump create a hosts file before the init library
	 * config and return
	 */
	if (dump) {
		if (fs->fi_shared_fs) {
			if (create_hosts_config(ctx, fs) != 0) {
				goto err;
			}
		}

		Trace(TR_MISC, "created fs %s in %s", fs->fi_name,
		    ctx->dump_path);

		return (0);
	}

	/*
	 * Don't do anything with the archiver.cmd file if this is a dump,
	 * or nosam is set or if no archiving configuration was passed in.
	 * Note that external checks for pmds/client and qfs only system
	 * should have been done and arc_info set to NULL for these cases.
	 */
	if (arc_info && !dump && fs->mount_options->sam_opts.archive) {

		if (create_fs_arch_set(fs->fi_name, arc_info) != 0) {
			Trace(TR_ERR, "create and mount failed at add: %s",
			    samerrmsg);

			/*
			 * Set the error indicator but continue on to
			 * configure the file system. This matches the
			 * behavior when this was done outside of
			 * this function.
			 */
			error = 3;
		}
	}


	/* sighup the daemon */
	if (init_library_config(ctx) != 0) {
		goto err;
	}

	/* If it the metadata server create the hosts file */
	if (fs->fi_shared_fs && fs->fi_status & FS_SERVER) {
		if (create_hosts_config(ctx, fs) != 0) {
			goto err;
		}
	}

	/*
	 * don't call mkfs if this is a shared file system (unless it is the
	 * metadata server) or if ctime is non-zero (used for ha-qfs)
	 */
	if ((!fs->fi_shared_fs && (fs->ctime == 0)) ||
	    fs->fi_status & FS_SERVER) {
		if (-1 == fs_mk(fs->fi_name, fs->dau, 0,
		    0, fs->fi_shared_fs)) {

			goto err;
		}
	}


	/*
	 * If the file system is a shared file system remove the hosts file
	 */
	if (fs->fi_shared_fs && fs->fi_status & FS_SERVER) {
		upath_t hosts_file;

		snprintf(hosts_file, sizeof (hosts_file), "%s/hosts.%s",
		    CFG_DIR, fs->fi_name);

		/* removal is not critical- so ignore errors */
		unlink(hosts_file);
	}


	Trace(TR_MISC, "created fs %s", fs->fi_name);
	return (error);


err:
	free_mcf_cfg(mcf);
	Trace(TR_ERR, "create fs failed: %s", samerrmsg);
	return (-1);
}


/*
 * Function to check the shared fs and setup some flags prior to normal
 * fs processing.
 */
static int
precheck_shared_fs(fs_t *fs) {

	if (fs->fi_shared_fs) {
		/*
		 * set the shared mount option. The gui is not exposing this
		 * as different than creating a shared file system.
		 */
		fs->mount_options->sharedfs_opts.shared = B_TRUE;
		fs->mount_options->sharedfs_opts.change_flag |= MNT_SHARED;


		/* do some initial checks */
		if (!(fs->fi_status & FS_CLIENT) &&
		    !(fs->fi_status & FS_NODEVS) &&
		    !(fs->fi_status & FS_SERVER)) {

			samerrno = SE_HOST_TYPE_NOT_SET;
			snprintf(samerrmsg, MAX_MSG_LEN,
				GetCustMsg(SE_HOST_TYPE_NOT_SET));

			return (-1);
		}

		/*
		 * Make sure hosts information is present if this is the
		 * metadata server.
		 */
		if (fs->fi_status & FS_SERVER) {
			if (fs->hosts_config == NULL) {
				samerrno = SE_FS_HAS_NO_HOSTS_INFO;
				snprintf(samerrmsg, MAX_MSG_LEN,
					GetCustMsg(SE_FS_HAS_NO_HOSTS_INFO));
				return (-1);
			}
		}
		/*
		 * If this is for a client ensure that nodev has been
		 * set for the metadata devices.
		 */
		if (fs->fi_status & FS_NODEVS) {
			node_t *n;

			for (n = fs->meta_data_disk_list->head;
				n != NULL; n = n->next) {

				disk_t *d = (disk_t *)n->data;

				if (d == NULL) {
					continue;
				}

				strlcpy(d->base_info.name, NODEV_STR,
				    sizeof (upath_t));
				d->base_info.additional_params[0] = '\0';
			}
		}
	}

	return (0);
}


/*
 * private function to add an entry to the vfstab for a filesystem.
 */
static int
setup_vfstab(
fs_t *fs,
boolean_t mount_at_boot)
{

	vfstab_entry_t *vfs_ent;

	Trace(TR_DEBUG, "setup vfstab entry");
	/* if it already exists it is an error. */
	if (get_vfstab_entry(fs->fi_name, &vfs_ent) != 0) {
		if (samerrno != SE_NOT_FOUND) {
			Trace(TR_ERR, "setup vfstab failed %s", samerrmsg);
			return (-1);
		}
	} else {
		samerrno = SE_VFSTAB_ENTRY_ALREADY_EXISTS;
		/* A vfstab entry already exists for file system %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_VFSTAB_ENTRY_ALREADY_EXISTS),
		    fs->fi_name);

		free_vfstab_entry(vfs_ent);
		Trace(TR_ERR, "setup vfstab failed %s", samerrmsg);
		return (-1);
	}

	if (make_vfstab_entry_for_fs(fs, &vfs_ent) != 0) {
		Trace(TR_ERR, "setup vfstab failed %s", samerrmsg);
		return (-1);
	}


	/* if mount_at_boot is true, set mount at boot */
	if (mount_at_boot) {
		vfs_ent->mount_at_boot = B_TRUE;
	}


	if (add_vfstab_entry(vfs_ent) != 0) {
		free_vfstab_entry(vfs_ent);
		Trace(TR_ERR, "setup vfstab failed %s", samerrmsg);
		return (-1);
	}

	free_vfstab_entry(vfs_ent);
	Trace(TR_DEBUG, "setup vfstab exit");
	return (0);
}



/*
 * private function to create a vfstab_entry_t structure from information
 * in the fs_t structure.
 */
static int
make_vfstab_entry_for_fs(
fs_t *fs,		/* fs for which to make a vfstab entry */
vfstab_entry_t **out)	/* malloced return */
{
	char type[20];
	char *fsname = NULL;

	Trace(TR_DEBUG, "make vfstab entry for %s", fs->fi_name);

	*out = (vfstab_entry_t *)mallocer(sizeof (vfstab_entry_t));
	if (*out == NULL) {
		Trace(TR_ERR, "make vfstab entry failed: %s",
		    samerrmsg);
		return (-1);
	}
	memset(*out, 0, sizeof (vfstab_entry_t));
	(*out)->fsck_pass = -1;

	fsname = fs->fi_name;
	/* if ufs, then use data device as fsname */
	if (strcmp(fs->equ_type, UFS_TYPE) == 0) {
		strcpy(type, UFS_TYPE);
		if (ISNULL(fs->data_disk_list))
			goto err;
		fsname = (char *)(fs->data_disk_list->head->data);
		if (ISNULL(fsname))
			goto err;
		/* set device to fsck */
		dsk2rdsk(fsname, &((*out)->device_to_fsck));
	} else {
		strcpy(type, SAMFS_TYPE);
	}

	/* set its name */
	(*out)->fs_name = (char *)mallocer(strlen(fsname) + 1);
	if ((*out)->fs_name == NULL) {
		goto err;
	}
	strcpy((*out)->fs_name, fsname);

	/* set its mount point if one is present */
	if (*fs->fi_mnt_point != char_array_reset) {

		(*out)->mount_point =
		    (char *)mallocer(strlen(fs->fi_mnt_point) + 1);

		if ((*out)->mount_point == NULL) {
			goto err;
		}
		strcpy((*out)->mount_point, fs->fi_mnt_point);
	}

	/* set its type */
	(*out)->fs_type = (char *)mallocer(strlen(type) + 1);
	if ((*out)->fs_type == NULL) {
		goto err;
	}

	strcpy((*out)->fs_type, type);

	/* mount at boot stays set to no */


	/* mount options only set here if it is shared. */
	if (fs->fi_shared_fs) {
		(*out)->mount_options = (char *)mallocer(strlen(MO_SHARED) + 1);
		if ((*out)->mount_options == NULL) {
			goto err;
		}
		strcpy((*out)->mount_options, MO_SHARED);
	}

	/* UFS mount options set here. */
	if (strcmp(type, UFS_TYPE) == 0 && fs->mount_options != NULL) {
		(*out)->mount_options =
		    (char *)mallocer(strlen(MO_RO) + strlen(MO_NOSUID) + 2);
		(*out)->mount_options[0] = '\0';
		if ((*out)->mount_options == NULL) {
			goto err;
		}
		if (fs->mount_options->readonly)
			strcpy((*out)->mount_options, MO_RO);
		if (fs->mount_options->no_suid) {
			if (fs->mount_options->readonly)
				strcat((*out)->mount_options, MO_SEP);
			strcat((*out)->mount_options, MO_NOSUID);
		}
		if ((*out)->mount_options[0] == '\0') {
			free((*out)->mount_options);
			(*out)->mount_options = NULL;
		}
	}

	Trace(TR_DEBUG, "made vfstab entry for %s", Str(fsname));
	return (0);

err:
	free_vfstab_entry(*out);
	Trace(TR_ERR, "make vfstab entry failed: %s", samerrmsg);
	return (-1);
}


/*
 * must have checked that the fs does not duplicate an existing family_set
 * or device name before calling.
 */
static int
build_samfs(
mcf_cfg_t *mcf,	/* input for decision making */
fs_t *fs)	/* fs struct to populate */
{

	base_dev_t	*fs_dev;
	sqm_lst_t		*eqs = NULL;
	node_t		*n;


	Trace(TR_DEBUG, "building samfs for %s", fs->fi_name);

	/* must have some devices or return error */
	if (fs->data_disk_list == NULL ||
	    fs->data_disk_list->length == 0) {

		samerrno = SE_FS_CONTAINS_NO_DEVICES;

		/* Filesystem %s contains no devices */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FS_CONTAINS_NO_DEVICES), fs->fi_name);

		Trace(TR_ERR, "building samfs failed: %s", samerrmsg);
		return (-1);
	}

	/* figure out how many devices need a free eq to be selected */
	if (get_needed_eqs(mcf, fs->fi_eq, fs->data_disk_list,
	    fs->meta_data_disk_list, fs->striped_group_list, &eqs) != 0) {

		Trace(TR_ERR, "building samfs failed: %s", samerrmsg);
		return (-1);
	}

	/* build family set device */
	fs_dev = (base_dev_t *)mallocer(sizeof (base_dev_t));
	if (fs_dev == NULL) {
		lst_free_deep(eqs);
		Trace(TR_ERR, "building samfs failed: %s", samerrmsg);
		return (-1);
	}
	memset(fs_dev, 0, sizeof (base_dev_t));
	strcpy(fs_dev->name, fs->fi_name);
	if (is_valid_eq(fs->fi_eq)) {
		fs_dev->eq = fs->fi_eq;
	} else {
		fs_dev->eq = *(equ_t *)eqs->head->data;
		free(eqs->head->data);
		eqs->head->data = NULL;
		if (lst_remove(eqs, eqs->head) != 0) {
			free(fs_dev);
			lst_free_deep(eqs);
			Trace(TR_ERR, "building samfs failed: %s", samerrmsg);
			return (-1);
		}
	}
	strlcpy(fs_dev->equ_type, fs->equ_type, sizeof (fs_dev->equ_type));
	strlcpy(fs_dev->set, fs->fi_name, sizeof (fs_dev->set));
	if (fs->fi_shared_fs) {
		strcpy(fs_dev->additional_params, "shared");
	}

	Trace(TR_DEBUG, "appending dev %s", fs_dev->name);
	if (lst_append(mcf->mcf_devs, fs_dev) != 0) {
		free(fs_dev);
		lst_free_deep(eqs);
		Trace(TR_ERR, "building samfs failed: %s", samerrmsg);
		return (-1);
	}

	/* build each individual device */
	for (n = fs->data_disk_list->head; n != NULL; n = n->next) {

		if (build_dev(mcf->mcf_devs, fs, (disk_t *)n->data, eqs) != 0) {

			lst_free_deep(eqs);
			Trace(TR_ERR, "building samfs failed: %s", samerrmsg);
			return (-1);
		}
	}

	lst_free_deep(eqs);
	Trace(TR_DEBUG, "built samfs");
	return (0);
}


/*
 * get a list of available equipment ordinals for each device in the
 * filesystem which does not have an eq already specified.
 */
static int
get_needed_eqs(
mcf_cfg_t *mcf,	/* current mcf cfg, input for decision making */
equ_t fi_eq,
const sqm_lst_t *data_disk_list,
const sqm_lst_t *meta_data_disk_list,
const sqm_lst_t *striped_group_list,
sqm_lst_t **eqs)	/* malloced list of equ_t */
{

	int eqs_needed = 0;
	node_t *n, *inner;

	Trace(TR_DEBUG, "get eq numbers for fs entry");
	/* figure out how many devices need a free eq to be selected */
	if (!is_valid_eq(fi_eq)) {
		eqs_needed++;
	}

	if (data_disk_list != NULL) {
		for (n = data_disk_list->head; n != NULL; n = n->next) {
			disk_t *d = (disk_t *)n->data;
			if (!is_valid_eq(d->base_info.eq)) {
				eqs_needed++;
			}
		}
	}

	if (meta_data_disk_list != NULL) {
		for (n = meta_data_disk_list->head;
		    n != NULL; n = n->next) {

			disk_t *d = (disk_t *)n->data;
			if (!is_valid_eq(d->base_info.eq)) {
				eqs_needed++;
			}
		}
	}

	if (striped_group_list != NULL) {
		for (n = striped_group_list->head;
		    n != NULL; n = n->next) {

			striped_group_t *sg = (striped_group_t *)n->data;

			for (inner = sg->disk_list->head; inner != NULL;
			    inner = inner->next) {

				disk_t *d = (disk_t *)inner->data;
				if (!is_valid_eq(d->base_info.eq)) {
					eqs_needed++;
				}
			}
		}
	}

	if (eqs_needed != 0) {
		if (get_available_eq_ord(mcf, eqs, eqs_needed) != 0) {
			Trace(TR_ERR, "get eq numbers for fs failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	Trace(TR_DEBUG, "got eq numbers for fs");
	return (0);
}


/*
 * eqs list must contain enough eqs for each dev. The resulting
 * dev is inserted into the mcf structure after the insert_after node.
 */
static int
build_dev(
sqm_lst_t *res_devs,	/* cfg into which dev should be inserted */
fs_t *fs,		/* fs to which dev is being added */
disk_t *disk,		/* disk to build dev for */
sqm_lst_t *eqs)		/* list of available eqs */
{

	base_dev_t *dev;

	Trace(TR_DEBUG, "building device");


	dev = (base_dev_t *)mallocer(sizeof (base_dev_t));
	if (dev == NULL) {
		Trace(TR_ERR, "build device failed: %s", samerrmsg);
		return (-1);
	}
	memset(dev, 0, sizeof (base_dev_t));

	/* name -- select the disk name from au or base_info */
	if (is_valid_disk_name(disk->base_info.name)) {

		strlcpy(dev->name, disk->base_info.name,
		    sizeof (dev->name));

	} else if (is_valid_disk_name(disk->au_info.path)) {
		strlcpy(dev->name, disk->au_info.path,
		    sizeof (dev->name));
	} else {

		/* one of the paths has to be valid */
		samerrno = SE_INVALID_DEVICE_PATH;

		/* Invalid device path %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_DEVICE_PATH),
		    disk->base_info.name);

		Trace(TR_ERR, "build device failed: %s", samerrmsg);
		free(dev);
		return (-1);
	}

	Trace(TR_DEBUG, "build device building %s", dev->name);

	/* use input eq or select a free one. */
	if (is_valid_eq(disk->base_info.eq)) {
		Trace(TR_DEBUG, "using input eq for %s", dev->name);
		dev->eq = disk->base_info.eq;
		remove_eq(eqs, dev->eq);
	} else {
		Trace(TR_DEBUG, "using new free eq for dev %d for %s",
		    *(equ_t *)eqs->head->data, dev->name);

		dev->eq = *(equ_t *)eqs->head->data;
		disk->base_info.eq = *(equ_t *)eqs->head->data;
		free(eqs->head->data);
		if (lst_remove(eqs, eqs->head) != 0) {
			Trace(TR_ERR, "build device failed: %s", samerrmsg);
			free(dev);
			return (-1);
		}
	}


	/* equ_type */
	if (nm_to_dtclass(disk->base_info.equ_type) != 0) {
		strlcpy(dev->equ_type, disk->base_info.equ_type,
		    sizeof (dev->equ_type));
	} else {
		strlcpy(dev->equ_type, DEFAULT_DATA_DISK_TYPE,
		    sizeof (dev->equ_type));
	}


	/* set name */
	strlcpy(dev->set, fs->fi_name, sizeof (dev->set));

	/* family set eq */
	dev->fseq = fs->fi_eq;
	*dev->additional_params = '\0';

	if (lst_append(res_devs, dev) != 0) {
		free(dev);
		Trace(TR_ERR, "build device failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "adding dev %s %d to %s", dev->name, dev->eq, dev->set);
	Trace(TR_DEBUG, "built device");

	return (0);
}


/*
 * remove an eq from the list as it is used.
 */
static void
remove_eq(
sqm_lst_t *l,	/* list from which to remove eq */
equ_t eq)	/* eq to remove */
{

	node_t *n;
	if (l == NULL) {
		return;
	}

	for (n = l->head; n != NULL; n = n->next) {
		if (*(int *)n->data == eq) {
			free(n->data);
			lst_remove(l, n);
		}
	}
}


/*
 * check a disk name.
 */
boolean_t
is_valid_disk_name(
upath_t name)
{

	struct stat	st;
	errno = 0;

	Trace(TR_DEBUG, "checking disk name %s", name);
	if (strcmp(NODEV_STR, name) == 0) {
		Trace(TR_DEBUG, "disk name was valid");
		return (B_TRUE);
	}

	if (stat(name, &st) != 0) {
		Trace(TR_DEBUG, "stat failed for disk %s", name);
		return (B_FALSE);
	}
	if (strncmp(name, "/dev/osd", 8) != 0 && !S_ISBLK(st.st_mode)) {
		Trace(TR_DEBUG, "disk name %s was not valid", name);
		return (B_FALSE);
	}
	Trace(TR_DEBUG, "disk name was valid");
	return (B_TRUE);
}


/*
 * check that an eq is valid.
 */
boolean_t
is_valid_eq(
equ_t eq)
{
	Trace(TR_DEBUG, "checking eq ordinal %d", (int)eq);
	if (eq < 1) {
		Trace(TR_DEBUG, "eq ordinal is not valid");
		return (B_FALSE);
	}

	Trace(TR_DEBUG, "eq ordinal is valid");
	return (B_TRUE);
}


/*
 * private function to build a qfs file system.
 */
static int
build_qfs(
mcf_cfg_t *mcf,
fs_t *fs)
{

	node_t *n;
	base_dev_t *fs_dev;
	sqm_lst_t *eqs = NULL;
	sqm_lst_t *group_ids = NULL;

	Trace(TR_DEBUG, "build qfs entry");
	/* must have some devices or return error */
	if (fs->meta_data_disk_list == NULL ||
	    fs->meta_data_disk_list->length == 0) {

		/* Filesystem %s contains no metadevices */
		samerrno = SE_FS_CONTAINS_NO_METADATA_DEVICES;

		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FS_CONTAINS_NO_METADATA_DEVICES),
		    fs->fi_name);

		Trace(TR_ERR, "build qfs failed: %s", samerrmsg);
		return (-1);
	}

	if ((fs->data_disk_list == NULL ||
	    fs->data_disk_list->length == 0) &&
	    (fs->striped_group_list == NULL ||
	    fs->striped_group_list->length == 0)) {

		samerrno = SE_FS_CONTAINS_NO_DATA_DEVICES;

		/* Filesystem %s contained no data devices */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FS_CONTAINS_NO_DATA_DEVICES),
		    fs->fi_name);

		Trace(TR_ERR, "build qfs failed: %s", samerrmsg);
		return (-1);
	}

	if (get_needed_eqs(mcf, fs->fi_eq, fs->data_disk_list,
	    fs->meta_data_disk_list, fs->striped_group_list, &eqs) != 0) {

		Trace(TR_ERR, "build qfs failed: %s", samerrmsg);
		return (-1);
	}

	/* build family set device */
	fs_dev = (base_dev_t *)mallocer(sizeof (base_dev_t));
	if (fs_dev == NULL) {
		lst_free_deep(eqs);
		Trace(TR_ERR, "build_samfs() failed: %s", samerrmsg);
		return (-1);
	}
	memset(fs_dev, 0, sizeof (base_dev_t));
	strcpy(fs_dev->name, fs->fi_name);

	if (is_valid_eq(fs->fi_eq)) {
		Trace(TR_DEBUG, "using input eq");
		fs_dev->eq = fs->fi_eq;
	} else {
		Trace(TR_DEBUG, "using a new free eq");

		fs_dev->eq = *(equ_t *)eqs->head->data;
		free(eqs->head->data);
		eqs->head->data = NULL;
		if (lst_remove(eqs, eqs->head) != 0) {
			lst_free_deep(eqs);
			Trace(TR_ERR, "build qfs failed: %s", samerrmsg);
			return (-1);
		}
	}
	strlcpy(fs_dev->equ_type, fs->equ_type, sizeof (fs_dev->equ_type));

	strlcpy(fs_dev->set, fs->fi_name, sizeof (fs_dev->set));
	if (fs->fi_shared_fs) {
		strcpy(fs_dev->additional_params, "shared");
	}
	Trace(TR_DEBUG, "appending dev %s", fs_dev->name);
	if (lst_append(mcf->mcf_devs, fs_dev) != 0) {
		lst_free_deep(eqs);
		Trace(TR_ERR, "build qfs failed: %s", samerrmsg);
		return (-1);
	}


	/* Now build the metadata devs. */
	for (n = fs->meta_data_disk_list->head; n != NULL; n = n->next) {

		if (build_metadata_dev(mcf->mcf_devs, fs, (disk_t *)n->data,
		    eqs) != 0) {

			lst_free_deep(eqs);
			Trace(TR_ERR, "build qfs failed: %s", samerrmsg);
			return (-1);
		}
	}

	/* Now build the data devs. */
	if (fs->data_disk_list != NULL && fs->data_disk_list->length != 0) {
		for (n = fs->data_disk_list->head; n != NULL; n = n->next) {
			if (build_dev(mcf->mcf_devs, fs, (disk_t *)n->data,
			    eqs) != 0) {

				lst_free_deep(eqs);
				Trace(TR_ERR, "build qfs failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	/*
	 * Get striped group ids if needed. This block must be outside
	 * build_striped_group to support multiple new striped groups.
	 */
	if (fs->striped_group_list != NULL &&
	    fs->striped_group_list->length != 0) {
		int num_type = nm_to_dtclass(fs->equ_type);
		boolean_t object_groups = B_FALSE;

		if (num_type == DT_META_OBJECT_SET) {
			object_groups = B_TRUE;
		}

		if (get_available_striped_group_id(mcf, fs->fi_name,
		    &group_ids, fs->striped_group_list->length,
		    object_groups) != 0) {

			lst_free_deep(eqs);
			Trace(TR_ERR, "build qfs failed: %s", samerrmsg);
			return (-1);
		}

		/* Now build the striped group devs. */
		for (n = fs->striped_group_list->head; n != NULL;
		    n = n->next) {

			if (build_striped_group(mcf->mcf_devs, fs,
			    (striped_group_t *)n->data, group_ids, eqs) != 0) {

				lst_free_deep(eqs);
				lst_free_deep(group_ids);
				Trace(TR_ERR, "build qfs failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	lst_free_deep(eqs);
	lst_free_deep(group_ids);
	Trace(TR_DEBUG, "built qfs");
	return (0);
}


/*
 * build a striped group inserting it into the fs structure.
 */
static int
build_striped_group(
sqm_lst_t *res_devs,		/* cfg into which dev should be inserted */
fs_t *fs,		/* fs to which striped group is being added. */
striped_group_t *sg,	/* striped group to insert */
sqm_lst_t *group_ids,	/* list of available group ids */
sqm_lst_t *eqs)		/* list of available eqs */
{

	node_t *n;
	int num_type;

	Trace(TR_DEBUG, "build striped group");

	if (sg->disk_list == NULL || sg->disk_list->length == 0) {
		samerrno = SE_STRIPE_GROUP_CONTAINED_NO_DISKS;

		/* Stripe group contained no disks */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_STRIPE_GROUP_CONTAINED_NO_DISKS));

		Trace(TR_ERR, "build striped group failed: %s", samerrmsg);
		return (-1);
	}

	num_type = nm_to_dtclass(sg->name);
	if (!is_stripe_group(num_type) && !is_osd_group(num_type)) {
		if (group_ids == NULL || group_ids->length == 0) {
			samerrno = SE_INSUFFICIENT_FREE_STRIPED_GROUP_IDS;
			/* Filesystem %s has no free striped group ids */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_INSUFFICIENT_FREE_STRIPED_GROUP_IDS),
			    fs->fi_name);

			Trace(TR_ERR, "build striped group failed: %s",
			    samerrmsg);
			return (-1);
		}

		/*
		 * striped group names contain upto 4 chars and a '\0',
		 * copy them over.
		 */
		strlcpy(sg->name, (char *)group_ids->head->data, 5);
		free(group_ids->head->data);
		if (lst_remove(group_ids, group_ids->head) != 0) {
			Trace(TR_ERR, "build striped group failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	for (n = sg->disk_list->head; n != NULL; n = n->next) {

		disk_t *tmp_disk = (disk_t *)n->data;
		strcpy(tmp_disk->base_info.equ_type, sg->name);
		if (build_dev(res_devs, fs, tmp_disk, eqs) != 0) {

			Trace(TR_ERR, "build striped group failed: %s",
			    samerrmsg);
			return (-1);
		}
	}
	Trace(TR_DEBUG, "built striped group");
	return (0);
}


/*
 * build a metadata device
 */
static int
build_metadata_dev(
sqm_lst_t *res_devs,	/* cfg into which dev should be inserted */
fs_t *fs,		/* fs to which dev is being added */
disk_t *disk,		/* disk to build dev for */
sqm_lst_t *eqs)		/* list of available eqs */
{

	Trace(TR_DEBUG, "build metadata dev");


	/*
	 * There are no options for metadata devices, make this an mm in
	 * case the caller did not set it.
	 */
	strcpy(disk->base_info.equ_type, META_DATA_DEVICE);

	/*
	 * if the device is a nodev set up fi_status
	 *	else
	 * if fi_status has FS_NODEVS set, copy NODEV_STR into the device
	 * name field
	 */
	if (strcmp(disk->base_info.name, NODEV_STR) == 0) {
		fs->fi_status |= FS_NODEVS;
		fs->fi_status |= FS_CLIENT;
	} else if (fs->fi_status & FS_NODEVS) {
		strcpy(disk->base_info.name, NODEV_STR);
		disk->base_info.additional_params[0] = '\0';
	}

	if (build_dev(res_devs, fs, disk, eqs) != 0) {
		Trace(TR_ERR, "build metadata dev failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "built metadata dev");
	return (0);
}


/*
 * find the named group if it exists otherwise create it.
 */
static int
find_striped_group(
sqm_lst_t *l,		/* list to search for striped group in */
uname_t name,		/* name of the striped_group to find */
striped_group_t **sg)	/* return value */
{

	node_t *n;

	Trace(TR_DEBUG, "find striped group %s", name);

	for (n = l->head; n != NULL; n = n->next) {
		*sg = (striped_group_t *)n->data;
		if (strcmp((char *)n->data, name) == 0) {
			/* found it! */
			Trace(TR_DEBUG, "found striped group");
			return (0);
		}
	}

	/* didn't find it so create it. */
	*sg = (striped_group_t *)mallocer(sizeof (striped_group_t));
	if (*sg == NULL) {
		Trace(TR_ERR, "find striped group failed: %s", samerrmsg);
		return (-1);
	}
	memset(*sg, 0, sizeof (striped_group_t));
	strcpy((*sg)->name, name);
	(*sg)->disk_list = lst_create();
	if ((*sg)->disk_list == NULL) {
		Trace(TR_ERR, "find striped group failed: %s", samerrmsg);
		return (-1);
	}

	if (lst_append(l, *sg) != 0) {
		Trace(TR_ERR, "find striped group failed: %s", samerrmsg);
		return (-1);
	}
	Trace(TR_DEBUG, "created new striped group");
	return (0);
}




/*
 * check_config_with_fsd()
 * can be used to verify the mcf, samfs.cmd, diskvols.conf
 * files. It returns a list of the errors printed to stderr by
 * the sam-fsd command.
 *
 * If this function executes error free and no errors are detected in the
 * file being checked 0 is returned. The list parsing_errs will be NULL.
 *
 *
 * If this function executes error free but encounters errors in the cfg file
 * being checked the status of sam-fsd is returned and the parsing_errs list is
 * populated with strings describing the errors in the cmd file. The caller
 * needs to call lst_free_deep for parsing_errs.
 *
 * If this function experiences internal errors -1 is returned and samerrno
 * and samerrmsg are set. The list parsing_errs will be NULL.
 *
 * There is an ordering adhered to by the underlying function.
 * The order files are checked is mcf defaults.conf diskvols.conf samfs.cmd
 *
 * If errors are encountered in the mcf no more files are processed.
 * If errors are encountered in the defaults.conf no more files are processed.
 * If defaults.conf and mcf are error free the remaining
 * files both get checked. This means that you can get errors that pertain
 * to both the diskvols.conf and samfs.cmd files at the same time. But if you
 * get defaults.conf errors or mcf errors that it is all you will get.
 *
 * Parsing errors may be for any of the files. Including ones with NULL input
 * arguments.
 */
int
check_config_with_fsd(
char *mcf_location,		/* uses actual if NULL */
char *diskvols_location,	/* uses actual if NULL */
char *samfs_location,		/* uses actual if NULL */
char *defaults_location,	/* uses actual if NULL */
sqm_lst_t **parsing_errs)	/* return list of strings describing errors */
{

	FILE *res_stream = NULL;
	FILE *err_stream = NULL;
	char cmd[MAX_LINE];
	char line[MAX_LINE];
	char err_msg[MAX_LINE];
	sqm_lst_t *tmp_lst;
	node_t *n;
	node_t *tmp_node;
	pid_t pid;
	int status = 0;

	Trace(TR_MISC, "check fs config files(%s, %s, %s, %s) entry",
	    Str(mcf_location), Str(diskvols_location), Str(samfs_location),
	    Str(defaults_location));

	if (ISNULL(parsing_errs)) {
		Trace(TR_ERR, "check fs config files failed: %s",
		    samerrmsg);
		return (-1);
	}


	*parsing_errs = lst_create();
	tmp_lst = lst_create();
	if (*parsing_errs == NULL || tmp_lst == NULL) {
		Trace(TR_ERR, "check fs config files failed: %s",
		    samerrmsg);
		return (-1);
	}

	/*
	 * tmp_errors is needed because of the way errors get generated
	 * in the underlying CLI see the notes in the function header comment
	 */



	*cmd = '\0';
	strcat(cmd, FSD_CMD);

	if (mcf_location != NULL) {
		strlcat(cmd, FSD_MCF, sizeof (cmd));
		strlcat(cmd, mcf_location, sizeof (cmd));
		Trace(TR_OPRMSG, "check fs config files:checking mcf %s\n",
		    mcf_location);
	} else {
		if (strcmp(mcf_file, MCF_CFG) != 0) {
			/* This section is required for testing */
			strlcat(cmd, FSD_MCF, sizeof (cmd));
			strlcat(cmd, mcf_file, sizeof (cmd));
		}
	}

	if (diskvols_location != NULL) {
		Trace(TR_OPRMSG,
		    "check fs config files:checking diskvols.conf: %s\n",
		    diskvols_location);
		strlcat(cmd, FSD_DISKVOLS, sizeof (cmd));
		strlcat(cmd, diskvols_location, sizeof (cmd));
	} else {
		if (strcmp(diskvols_file, DISKVOL_CFG) != 0) {
			/* This section is required for testing */
			strlcat(cmd, FSD_DISKVOLS, sizeof (cmd));
			strlcat(cmd, diskvols_file, sizeof (cmd));
		}
	}

	if (samfs_location != NULL) {
		Trace(TR_DEBUG,
		    "check fs config files:checking samfs.cmd: %s\n",
		    samfs_location);
		strlcat(cmd, FSD_SAMFS, sizeof (cmd));
		strlcat(cmd, samfs_location, sizeof (cmd));

	} else {
		if (strcmp(samfs_cmd_file, SAMFS_CFG) != 0) {
			/* This section is required for testing */
			strlcat(cmd, FSD_SAMFS, sizeof (cmd));
			strlcat(cmd, samfs_cmd_file, sizeof (cmd));
		}
	}
	if (defaults_location != NULL) {
		Trace(TR_DEBUG,
		    "check fs config files:checking defaults.conf: %s\n",
		    defaults_location);
		strlcat(cmd, FSD_DEFAULTS, sizeof (cmd));
		strlcat(cmd, defaults_location, sizeof (cmd));
	}


	pid = exec_get_output(cmd, &res_stream, &err_stream);
	if (pid < 0) {
		/* leave samerrno as set */
		return (-1);
	}

	if ((pid = waitpid(pid, &status, 0)) < 0) {
		Trace(TR_ERR, "waitpid failed");
	} else {
		Trace(TR_OPRMSG, "check fs config status: pid %ld %d\n",
		    pid, status);
	}

	/*
	 * check each line for parsing error. if one is encountered, build
	 * a meaningful message and insert it into the temp list
	 */
	while (NULL != fgets(line, sizeof (line), err_stream)) {
		if (NULL != strstr(line, " *** Error in line")) {
			char *beg;
			char *newline;
			beg = strstr(line, "line");
			newline = strchr(line, '\n');
			*newline = '\0';
			if (lst_append(tmp_lst, strdup(beg)) != 0) {
				goto err;
			}
		} else if (strstr(line, "Cannot open configuration")) {
			if (lst_append(*parsing_errs, strdup(line)) != 0) {
				goto err;
			}
		} else {

			/*
			 * this block must handle any other output to stderr
			 * unfortunately not all of it represents a unique
			 * error.
			 * examples of lines for mcf errors:
			 * 1 error in 'files/mcf.grau.bad'
			 * sam-fsd: Read mcf files/mcf.grau.bad failed.
			 */


			/*
			 * One place this block gets called is after
			 * handling of a file is done. So transfer from temp
			 * to permanent list.
			 */
			if (strstr(line, "1 error in '") == line ||
			    strstr(line, "errors in '")) {
				char *filename;
				char *end;

				filename = strchr(line, '\'');
				end = strrchr(line, '\'');
				*end = '\0';
				filename++;
				/*
				 * now we know the filename so make
				 * the error strings and copy to return list
				 */
				for (n = tmp_lst->head; n != NULL;
				    /* Incremented inside */) {

					*err_msg = '\0';
					strcat(err_msg, filename);
					strcat(err_msg, ": Error at ");
					strcat(err_msg, (char *)n->data);
					if (lst_append(*parsing_errs,
					    strdup(err_msg)) != 0) {

						goto err;
					}

					/* free the entry in the tmp_lst */
					free(n->data);
					tmp_node = n;
					n = n->next;

					/* remove the node */
					lst_remove(tmp_lst, tmp_node);
					Trace(TR_DEBUG, "%s error :%s",
					    "check fs config files",
					    err_msg);
				}

			} else {
				if (lst_append(*parsing_errs,
				    strdup(line)) != 0) {
					goto err;
				}
			}
		}
	}

	fclose(res_stream);
	fclose(err_stream);

	Trace(TR_MISC, "checked fs config files %d",
	    (*parsing_errs)->length);

	lst_free_deep(tmp_lst);
	if (status == 0) {
		lst_free_deep(*parsing_errs);
		*parsing_errs = NULL;
	}
	return (status);

err:

	lst_free_deep(tmp_lst);
	lst_free_deep(*parsing_errs);
	*parsing_errs = NULL;
	fclose(res_stream);
	fclose(err_stream);
	Trace(TR_ERR, "checking fs config files failed: %s", samerrmsg);
	return (-1);
}



/*
 * find the fs in the list. Does not remove the fs from the list or
 * duplicate it. find_fs returns a pointer to the actual list data
 * corresponding to the named filesystem.
 */
int
find_fs(
sqm_lst_t	*fs_list,
char	*fs_name,
fs_t	**fs)
{

	node_t *n;
	boolean_t found = B_FALSE;
	fs_t *tmp;

	if (fs_list == NULL) {
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), fs_name);
		Trace(TR_DEBUG, "fs %s not found", fs_name);
		return (-1);
	}

	for (n = fs_list->head; n != NULL; n = n->next) {
		tmp = (fs_t *)n->data;

		if (strcmp(tmp->fi_name, fs_name) == 0) {
			found = TRUE;
			*fs = tmp;
			break;
		}
	}

	if (found == B_FALSE) {
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), fs_name);
		Trace(TR_DEBUG, "fs %s not found", fs_name);
		return (-1);
	}

	return (0);
}


/*
 * This function was requested to perform all of the operations that the GUI
 * performs. It will create the filesystem fs. Conditionally setting
 * mount_at_boot in the /etc/vfstab. Conditionally it will create the mount
 * point and mount the filesystem after creation.
 *
 *
 * This function's return value indicates which step of this multistep
 * operation failed.
 *
 * The steps are as follows:
 * -1 initial checks failed.
 * 1. create the filesystem- this includes setting mount at boot.
 * 2. create the mount point directory.
 * 3. setup the archiver.cmd file.		- SAM only
 * 4. activate the archiver.cmd file.		- SAM only
 * 5. mount the filesystem.
 * 6. Filesystem created and mounted but the archiver.cmd file contains
 *	warnings.				- SAM only
 *
 * If warnings are detected in the archiver.cmd file this function continues on
 * and returns success if the subsequent steps are successful. If errors are
 * detected it will be returned.
 */
int
create_fs_and_mount(
ctx_t		*ctx		/* ARGSUSED */,
fs_t		*fs,
boolean_t	mount_at_boot,
boolean_t	create_mnt_point,
boolean_t	mount)
{

	return (create_arch_fs(ctx, fs, mount_at_boot, create_mnt_point,
	    mount, NULL));
}


int
create_arch_fs(
ctx_t		*ctx		/* ARGSUSED */,
fs_t		*fs,
boolean_t	mount_at_boot,
boolean_t	create_mnt_point,
boolean_t	mount,
fs_arch_cfg_t	*arc_info)
{

	boolean_t		sam = B_FALSE;
	int			arch_err = 0;
	boolean_t		dump = B_FALSE;
	int			save_errno = 0;
	char			save_errmsg[MAX_MSG_LEN];
	int			create_err = 0;


	Trace(TR_MISC, "creating and mount %s", fs ? fs->fi_name : "NULL");

	if (ISNULL(fs)) {
		Trace(TR_ERR, "create and mount failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * if ufs, then handle appropriately
	 */
	if (0 == strcmp(fs->equ_type, UFS_TYPE)) {

		/*
		 * create mount point prior to creating the file system
		 * That way if it fails we have not polluted the vfstab
		 */
		if (create_mnt_point) {
			if (create_dir(NULL, fs->fi_mnt_point) != 0) {

				/* FS created. Create mount point failed */
				Trace(TR_ERR,
				    "create and mount failed at dir: %s",
				    samerrmsg);
				return (2);
			}
		}

		/* add entry to vfstab then make filesystem */
		if (create_ufs(fs, mount_at_boot) != 0) {
			Trace(TR_ERR, "create and mount failed at create: %s",
			    samerrmsg);
			return (1);
		}

		/* mount if needed */
		if (mount) {
			if (mount_generic_fs(NULL, fs->fi_mnt_point,
			    UFS_TYPE) != 0) {
				Trace(TR_ERR,
				    "create and mount failed at mount: %s",
				    samerrmsg);
				return (5);
			}
		}
		return (0);
	}


	/* determine license type to determine if archiving is available */
	if (get_samfs_type(NULL) != QFS) {
		sam = B_TRUE;

		/*
		 * Archiving is only configured on the metadata
		 * server.  If this is a client or a potential
		 * metadata server.  Set the arc_info to NULL.
		 *
		 * Otherwise if the file system is an archiving file
		 * system precheck the archiving configuration.
		 */
		if (fs->fi_status & FS_CLIENT || fs->fi_status & FS_NODEVS) {
			arc_info = NULL;
		} else if (fs->mount_options->sam_opts.archive &&
		    (check_fs_arch_set(fs->fi_name, arc_info) != 0)) {
			Trace(TR_ERR, "create and mount failed at"
			    "arch check:%s", samerrmsg);
			return (-1);
		}

	} else {
		/*
		 * This is a qfs system so even if archiving info
		 * was passed in we should ignore it.
		 */
		arc_info = NULL;
	}


	/*
	 * If create fails due to archiver setup continue on to do all
	 * non-archiver steps but save the error to return it at the
	 * end. If it is any other failure return an error.
	 */
	create_err = internal_create_fs(ctx, fs, mount_at_boot, arc_info);
	if (create_err != 0) {
		if (create_err == 3) {
			Trace(TR_ERR, "archiver errors encountered: %s",
			    samerrmsg);

			save_errno = SE_ARCHIVER_CMD_CONTAINED_ERRORS;
			snprintf(save_errmsg, sizeof (save_errmsg),
			    GetCustMsg(save_errno));

			arch_err = 3;

		} else {
			Trace(TR_ERR, "create and mount failed at create: %s",
			    samerrmsg);
			return (1);
		}
	}

	dump = ctx != NULL && *ctx->dump_path != '\0';
	if (!dump && create_mnt_point &&
	    create_dir(NULL, fs->fi_mnt_point) != 0) {

		/* filesystem %s created. Creation of mount point failed */
		Trace(TR_ERR, "create and mount failed at dir: %s", samerrmsg);
		return (2);
	}


	/*
	 * During the very first file system creation, the file system must be
	 * mounted before the diskvols.conf will be read. If the diskvols
	 * have not been read activate_archiver_cfg will report errors. So
	 * mount the fs before configuring archiving.
	 *
	 * This condition may be relaxed with the fix of CR 6456500 In which
	 * case this code can be moved back to after the archiver.cmd set up.
	 */
	if (mount && !dump) {
		/*
		 * don't proceed to the mount until the sharedfsd daemon comes
		 * up. But wait only a bounded period of time.
		 */
		if (fs->fi_shared_fs) {
			if (wait_for_sharefsd(fs->fi_name) != 0) {
				return (5);
			}
		}

		if (mount_fs(NULL, fs->fi_name) != 0) {
			Trace(TR_ERR, "create and mount failed at mount: %s",
			    samerrmsg);
			return (5);
		}
	}



	/*
	 * Don't check the archiver configuration if we have already
	 * encountered an error, nosam is set or arc_info is
	 * null. Otherwise run chk_arch_cfg to check the archiver.cmd
	 * file. We do not need to activate the config at this point
	 * because that occurred inside create_fs when
	 * init_library_config was called.
	 */
	if (!arch_err && sam && !dump && fs->mount_options->sam_opts.archive &&
	    arc_info) {


		sqm_lst_t			*l = NULL;

		/* activate the archiver cfg */
		arch_err = chk_arch_cfg(NULL, &l);
		if (arch_err == -3) {
			save_errno = samerrno;
			strlcpy(save_errmsg, samerrmsg,
			    sizeof (save_errmsg));
			arch_err = 6;

		} else if (arch_err < 0) {
			Trace(TR_ERR,
			    "create and mount failed at activate: %s",
			    samerrmsg);

			save_errno = samerrno;
			strlcpy(save_errmsg, samerrmsg,
			    sizeof (save_errmsg));

			arch_err = 4;
		}
		lst_free_deep(l);
	}

	if (arch_err) {
		samerrno = save_errno;
		snprintf(samerrmsg, MAX_MSG_LEN, save_errmsg);
		return (arch_err);
	}

	Trace(TR_MISC, "created and mounted fs %s", fs ? fs->fi_name : "NULL");
	return (0);
}


/*
 * check the vsn maps and copies. If the archive set was an existing
 * explicit default set we should not have recieved vsn maps and
 * we should not overwrite the existing ones. If on the other hand
 * No maps were passed in and this is for either the fsname default
 * or an explicit default we MUST have vsn maps.
 */
static int
check_fs_arch_set(
char *fs_name,
fs_arch_cfg_t *arc) {

	boolean_t	new_set = B_FALSE;
	archiver_cfg_t	*cfg = NULL;
	sqm_lst_t		*l = NULL;


	if (ISNULL(fs_name, arc)) {
		Trace(TR_ERR, "check fs arch cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(NULL, &cfg) != 0) {
		Trace(TR_ERR, "check fs arch cfg failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * Fetch the criteria for the named set to check for existence
	 * and to use if the set name identifies an existing set.
	 */
	if (cfg_get_criteria_by_name(cfg, arc->set_name, &l) != 0) {
		if (samerrno == SE_NOT_FOUND) {
			new_set = B_TRUE;
		} else {
			free_archiver_cfg(cfg);
			return (-1);
		}
	}

	if (new_set) {
		node_t *n;

		/* the cfg and criteria are not needed in this branch */
		free_ar_set_criteria_list(l);
		free_archiver_cfg(cfg);


		if (arc->copies == NULL || arc->copies->length == 0) {
			setsamerr(SE_COPIES_REQUIRED);
			Trace(TR_ERR, "check fs arch cfg failed:%s", samerrmsg);
			return (-1);
		}



		if (arc->vsn_maps == NULL || arc->vsn_maps->length == 0) {
			setsamerr(SE_VSNS_REQUIRED);
			Trace(TR_ERR, "check fs arch cfg failed: %s",
			    samerrmsg);
			return (-1);
		} else if (arc->vsn_maps->length != arc->copies->length) {
			setsamerr(SE_VSNS_MUST_MATCH_COPIES);
			Trace(TR_ERR, "check fs arch cfg failed: %s",
			    samerrmsg);
			return (-1);
		}

		for (n = arc->vsn_maps->head; n != NULL; n = n->next) {
			vsn_map_t *m = (vsn_map_t *)n->data;
			/*
			 * There must be at least one vsn name or pool
			 * in each map
			 */
			if ((m == NULL || m->vsn_names == NULL ||
				m->vsn_names->length == 0) &&
				(m->vsn_pool_names == NULL ||
				m->vsn_pool_names->length == 0)) {

				setsamerr(SE_VSNS_REQUIRED);

				Trace(TR_ERR, "check fs arch cfg failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	} else {

		ar_set_criteria_t	*cr;
		boolean_t		found = B_FALSE;
		node_t			*n;
		int			i;


		/*
		 * For an existing set maps should not have been
		 * populuated on input because they would overwrite the
		 * values for the existing set.
		 */
		if (arc->vsn_maps != NULL && arc->vsn_maps->length != 0) {
			free_vsn_map_list(arc->vsn_maps);
			arc->vsn_maps = NULL;
		}


		/*
		 * For an existing set copies should not have been
		 * populuated on input because they will be set to
		 * match the existing set. However, a check for their
		 * existence in the cfg needs to be done here, and
		 * matching values need to be written to the archiver.cmd
		 * so they will be added to the arc_info at this point.
		 */
		if (arc->copies == NULL) {
			arc->copies = lst_create();
		} else if (arc->copies->length != 0) {
			/*
			 * whatever came in cannot be correct as it would
			 * overwrite the existing policy, free it
			 * and start fresh
			 */
			lst_free_deep(arc->copies);
			arc->copies = lst_create();
		}

		/*
		 * loop through the criteria for the set making sure
		 * it is for an explicit default set. If not we will
		 * fail so that we do not change a non-default
		 * set to an explicit default.
		 */
		for (n = l->head; n != NULL; n = n->next) {
			cr = (ar_set_criteria_t *)n->data;

			if (cr == NULL) {
				continue;
			}
			if (cr->change_flag & AR_ST_default_criteria) {
				found = B_TRUE;
				break;
			}
		}

		/*
		 * If we found a criteria copy its copy_cfg into the
		 * fs_arch_cfg struct.
		 */
		if (found) {

			found = B_FALSE; /* reset found to look for copies */

			for (i = 0; i < MAX_COPY; i++) {
				ar_set_copy_cfg_t *dup = NULL;

				if (cr->arch_copy[i] == NULL) {
					continue;
				}

				if (dup_ar_set_copy_cfg(cr->arch_copy[i],
				    &dup) != 0) {
					free_ar_set_criteria_list(l);
					free_archiver_cfg(cfg);
					Trace(TR_ERR, "check fs arch cfg"
					    "failed: %s", samerrmsg);
					return (-1);
				}

				if (dup == NULL ||
				    lst_append(arc->copies, dup) != 0) {

					free_ar_set_criteria_list(l);
					free_archiver_cfg(cfg);
					free(dup);

					Trace(TR_ERR, "check fs arch cfg"
					    "failed: %s", samerrmsg);
					return (-1);
				}
				found = B_TRUE;
			}
		}

		free_ar_set_criteria_list(l);
		free_archiver_cfg(cfg);

		/*
		 * Either no explicit default set was found or no
		 * copies were configured in it.
		 */
		if (!found) {
			samerrno = SE_NOT_A_DEFAULT_SET;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
			Trace(TR_ERR, "check fs arch cfg"
			    "failed: %s", samerrmsg);
			return (-1);
		}
	}

	return (0);
}


static int
create_fs_arch_set(
char *fs_name,
fs_arch_cfg_t *arc) {

	int			i;
	node_t			*n;
	archiver_cfg_t		*cfg = NULL;
	ar_fs_directive_t	*fsdir = NULL;
	ar_set_copy_cfg_t	**copies;

	/* check the inputs */
	if (ISNULL(arc, fs_name)) {
		Trace(TR_ERR, "Adding fs arch cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(NULL, &cfg) != 0) {
		Trace(TR_ERR, "Adding fs arch cfg failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * find the fs directive (it should already exist since one exists
	 * by default for every archiving file system)
	 */
	if (cfg_get_ar_fs(cfg, fs_name, B_FALSE, &fsdir) != 0) {
		Trace(TR_ERR, "fs directive not found. "
		    "Adding fs arch cfg failed: %s", samerrmsg);
		free_archiver_cfg(cfg);
		return (-1);
	}

	/* If there is an input log path set it */
	if (*(arc->log_path) != '\0') {
		strlcpy(fsdir->log_path, arc->log_path, sizeof (upath_t));
		fsdir->change_flag |= AR_FS_log_path;
	}


	/*
	 * if set_name == fs name set up the default file system copies.
	 * Else setup an explicit default archive set.
	 */
	if (strcmp(arc->set_name, fs_name) == 0) {
		Trace(TR_DEBUG, "Adding fs arch cfg setname == fsname");

		/* grab a pointer to the copies to modify them below */
		copies = fsdir->fs_copy;
	} else {
		ar_set_criteria_t *crit;

		Trace(TR_DEBUG, "Adding fs arch cfg setname != fsname");

		/*
		 * must set archmeta to false or archiver -lv will complain
		 * that there is no media for the set named for the filesystem.
		 */
		fsdir->archivemeta = B_FALSE;
		fsdir->change_flag |= AR_FS_archivemeta;

		crit = (ar_set_criteria_t *)mallocer(
			sizeof (ar_set_criteria_t));
		if (crit == NULL) {
			free_archiver_cfg(cfg);
			Trace(TR_ERR, "Adding fs arch cfg failed err:%d %s",
			    samerrno, samerrmsg);

			return (-1);
		}
		memset(crit, 0, sizeof (ar_set_criteria_t));
		strcpy(crit->set_name, arc->set_name);
		strcpy(crit->path, ".");
		crit->change_flag = AR_ST_path;
		crit->change_flag |= AR_ST_default_criteria;

		/* grab a pointer to the copies to modify them below */
		copies = crit->arch_copy;
		if (lst_append(fsdir->ar_set_criteria, crit) != 0) {
			free_ar_set_criteria(crit);
			free_archiver_cfg(cfg);
			Trace(TR_ERR, "Adding fs arch cfg failed:%d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
	}


	/* put the copy information into the cfg */
	for (i = 0, n = arc->copies->head;
		i < MAX_COPY && n != NULL; i++, n = n->next) {

		ar_set_copy_cfg_t *cp = (ar_set_copy_cfg_t *)n->data;

		if (copies[i] != NULL) {
			memcpy(copies[i], cp,
			    sizeof (ar_set_copy_cfg_t));
			Trace(TR_DEBUG, "Add fs arch cfg existing copy");
		} else {
			/* dup the copy so no separate free required */
			dup_ar_set_copy_cfg(cp, &(copies[i]));
			Trace(TR_DEBUG, "Add fs arch cfg dup'd copy");
			if (copies[i] == NULL) {
				free_archiver_cfg(cfg);
				Trace(TR_ERR, "Adding fs arch cfg"
					"failed: %s", samerrmsg);
				return (-1);
			}
		}
	}



	/*
	 * If the archive set was an existing explicit default set
	 * we should not have recieved vsn maps and we should not
	 * overwrite the existing ones.
	 */
	if (arc->vsn_maps != NULL) {
		for (n = arc->vsn_maps->head; n != NULL; n = n->next) {
			vsn_map_t *map = (vsn_map_t *)n->data;
			vsn_map_t *dup = NULL;
			boolean_t found = B_FALSE;
			node_t *in;

			/*
			 * Look through the existing vsn maps to see
			 * if one already exists for for this copy.
			 */
			for (in = cfg->vsn_maps->head; in != NULL;
				in = in->next) {

				dup = (vsn_map_t *)in->data;
				if (strcmp(dup->ar_set_copy_name,
				    map->ar_set_copy_name) == 0) {
					found = B_TRUE;
					break;
				}
			}

			/* prevent overwriting existing vsn maps */
			if (found) {
				Trace(TR_MISC, "vsn map found for set %s",
					map->ar_set_copy_name);
				continue;
			}
			if (dup_vsn_map(map, &dup) != 0) {
				free_archiver_cfg(cfg);
				Trace(TR_ERR, "Adding fs arch cfg failed: %s",
				    samerrmsg);
				return (-1);
			}
			if (lst_append(cfg->vsn_maps, dup) != 0) {
				free_archiver_cfg(cfg);
				free_vsn_map(dup);
				Trace(TR_ERR, "Adding fs arch cfg failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	/* write the archiver.cmd */
	if (write_arch_cfg(NULL, cfg, B_TRUE) != 0) {
		/* leave samerrno as set */

		free_archiver_cfg(cfg);
		Trace(TR_ERR, "Adding default fs map failed: %s", samerrmsg);
		return (-1);
	}

	free_archiver_cfg(cfg);


	return (0);

}

static int
wait_for_sharefsd(char *fs_name) {
	upath_t daemon_name;
	upath_t filter;

	snprintf(daemon_name, sizeof (daemon_name), "%s %s",
		SAM_SHAREFSD, fs_name);

	snprintf(filter, sizeof (filter), FILTER_TYPE_ARGS,
	    SAM_SHAREFSD, daemon_name);

	/* ignore errors */
	wait_for_proc_start(filter, 15, 1);

	return (0);
}


int
reset_equipment_ordinals(
ctx_t *ctx /* ARGSUSED */,
char *fs_name,
sqm_lst_t *ordinals)
{

	boolean_t	no_more_ords = B_FALSE;
	boolean_t	fs_found = B_FALSE;
	mcf_cfg_t	*mcf = NULL;
	node_t		*m = NULL;
	node_t		*ord_node = NULL;


	if (ISNULL(fs_name, ordinals)) {
		Trace(TR_ERR, "get fs names failed: %s", samerrmsg);
		return (-1);
	}

	if (read_mcf_cfg(&mcf) != 0) {
		/* Leave the samerrno that read set */
		Trace(TR_ERR, "get fs names failed: %s", samerrmsg);
		return (-1);
	}

	/* find the entries for the fs and set their ordinals to the inputs */
	ord_node = ordinals->head;
	if (ordinals->length == 0) {
		no_more_ords = B_TRUE;
	}

	for (m = mcf->mcf_devs->head; m != NULL; m = m->next) {

		base_dev_t *dev = (base_dev_t *)m->data;
		if (strcmp(dev->set, fs_name) == 0) {

			if (no_more_ords) {

				/*
				 * file system %s had %d devices but
				 * only %d ordinals that were supplied.
				 */
				free_mcf_cfg(mcf);
				return (-1);
			}
			fs_found = B_TRUE;
			dev->eq = *(equ_t *)ord_node->data;
			ord_node = ord_node->next;
			if (ord_node == NULL) {
				no_more_ords = B_TRUE;
			}

		}
	}


	if (!fs_found) {
		samerrno = SE_NOT_FOUND;
		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), fs_name);
		free_mcf_cfg(mcf);
		return (-1);
	}

	/* write the mcf */

	free_mcf_cfg(mcf);

	if (init_library_config(NULL) != 0) {
		return (-1);
	}

	return (0);
}

// ----------------------------------------------------------------------
//			4.4 functions

static int
create_ufs(
fs_t *fs,			/* the fs_t to create */
boolean_t mount_at_boot)	/* if true mount at boot goes in vfstab */
{

	if (fs_mk((char *)fs->data_disk_list->head->data,
	    fs->dau, 0, 0, fs->fi_shared_fs) == -1)
		goto err;
	if (setup_vfstab(fs, mount_at_boot) != 0)
		goto err;
	return (0);

err:
	Trace(TR_ERR, "create fs failed: %s", samerrmsg);
	return (-1);
}


int
remove_generic_fs(
ctx_t *c /*ARGSUSED*/,
char *fsname,
char *type)
{
	int res;
	int dbres;
	char buf[MAXPATHLEN + 1];

	Trace(TR_MISC, "remove generic fs (%s,%s) entry",
	    Str(fsname), Str(type));

	if (ISNULL(fsname, type)) {
		Trace(TR_ERR, "remove generic fs failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * database entries are removed in remove_fs().  Turns out the
	 * GUI doesn't call this function for QFS filesystems, despite
	 * the RPC call remove_fs() being declared deprecated.
	 */
	if (0 == strcmp(type, SAMFS_TYPE)) {	/* samfs/qfs */
		return (remove_fs(c, fsname));
	}

	/* non-SAM/QFS continues here */

	/* remove any metrics schedules */
	snprintf(buf, sizeof (buf), "task=RP,id=%s", fsname);
	dbres = remove_task_schedule(NULL, buf);
	if (dbres != 0) {
		/* doesn't affect anything else */
		Trace(TR_ERR, "deleting metrics schedule failed: %d", dbres);
	}

	/* delete any database entries for this filesystem */
	dbres = delete_fs_from_db(fsname);
	if (dbres != 0) {
		/* for now, doesn't affect anything else */
		Trace(TR_ERR, "deleting fsmdb entries failed: %d", dbres);
	}

	res = delete_vfstab_entry(fsname);
	Trace(TR_MISC, "remove generic fs %s done (%s)", fsname,
	    (res != -1) ? "success" : "failed");

	return (res);
}


/*
 * set_device_state currently supports alloc/noalloc
 */
int
set_device_state(
ctx_t *ctx		/* ARGSUSED */,
char *fs_name,
dstate_t new_state,
sqm_lst_t *eqs) {


	fs_t *fs;

	if (ISNULL(fs_name, eqs)) {
		Trace(TR_ERR, "setting device state failed:%s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "setting device state:%d", new_state);

	if (get_fs(NULL, fs_name, &fs) == -1) {
		Trace(TR_ERR, "setting device state failed:%s", samerrmsg);
		return (-1);
	}

	if (!(fs->fi_status & FS_MOUNTED)) {
		setsamerr(SE_SET_DISK_STATE_FS_NOT_MOUNTED);
		free_fs(fs);
		Trace(TR_ERR, "setting device state failed:%s", samerrmsg);
		return (-1);
	}

	if (_set_device_state(fs, eqs, new_state) != 0) {
		free_fs(fs);
		Trace(TR_ERR, "setting device state failed:%s",
		    samerrmsg);
		return (-1);
	}
	free_fs(fs);

	Trace(TR_MISC, "set device state:%d", new_state);
	return (0);

}


/*
 * Setting device state is idempotent- e.g. setting a device that is noalloc
 * to noalloc will not return an error.
 */
static int
_set_device_state(fs_t *fs, sqm_lst_t *eqs, dstate_t new_state) {
	node_t *n;
	int32_t command = 0;
	char *last_group = NULL;

	if (new_state == DEV_NOALLOC) {
		command = DK_CMD_noalloc;
	} else if (new_state == DEV_ON) {
		command = DK_CMD_alloc;
	} else {
		samerrno = SE_INVALID_DEVICE_STATE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    new_state);
		Trace(TR_ERR, "setting device state failed:%s", samerrmsg);
		return (-1);
	}

	for (n = eqs->head; n != NULL; n = n->next) {
		int eq = *(int *)n->data;
		char eqbuf[7];
		disk_t *dsk;

		dsk = find_disk_by_eq(fs, eq);
		if (dsk == NULL) {
			Trace(TR_ERR, "setting device state failed:%s",
			    samerrmsg);
			return (-1);
		}

		/*
		 * If the state being set is noalloc verify that
		 * the device is not a metadata device. Metadata devices can
		 * not currently be turned off by setting to noalloc.
		 */
		if (strcmp(dsk->base_info.equ_type, "mm") == 0 &&
		    new_state == DEV_NOALLOC) {
			samerrno = SE_CANNOT_DISABLE_MM_ALLOCATION;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			    fs->fi_name);
			Trace(TR_ERR, "setting device state failed:%s",
			    samerrmsg);
			return (-1);
		}

		/*
		 * If the state being set is the same as the current state that
		 * the device is reporting skip it. Otherwise the SetFsPartCmd
		 * command will return an error.
		 */
		if (dsk->base_info.state == new_state) {
			continue;
		}

		/*
		 * It is only necessary to set the state for the
		 * first dev in a striped group. Don't double call
		 * SetFsPartCommand as it will return an error for
		 * the second call.
		 */
		if ((*(dsk->base_info.equ_type) == 'g') ||
		    (*(dsk->base_info.equ_type) == 'o')) {
			if (last_group != NULL &&
			    strcmp(dsk->base_info.equ_type, last_group) == 0) {
				/*
				 * Skip devs from the group
				 * beyond the first
				 */
				continue;
			} else {
				/*
				 * set last_group for this first
				 * device of a group and go on to
				 * call the command.
				 */
				last_group = dsk->base_info.equ_type;
			}
		}

		snprintf(eqbuf, 7, "%d", eq);
		if (SetFsPartCmd(fs->fi_name, eqbuf, command) != 0) {
			samerrno = SE_SET_DISK_STATE_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), eq, new_state);
			Trace(TR_ERR, "setting device state failed:%s",
			    samerrmsg);
			return (-1);
		}
	}

	Trace(TR_MISC, "_set device state:%d", new_state);
	return (0);
}

static int
cmd_online_grow(
char *fs_name,
int eq) {

	char eqbuf[7];

	snprintf(eqbuf, 7, "%d", eq);
	if (SetFsPartCmd(fs_name, eqbuf, DK_CMD_add) != 0) {
		samerrno = SE_ONLINE_GROW_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), eq);
		return (-1);
	}

	return (0);
}

int
fs_dev_remove(
char *fs_name,
int eq) {

	char eqbuf[7];

	snprintf(eqbuf, 7, "%d", eq);
	if (SetFsPartCmd(fs_name, eqbuf, DK_CMD_remove) != 0) {
		samerrno = SE_FS_DEV_REMOVE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), eq);
		return (-1);
	}

	return (0);
}

int
fs_dev_release(
char *fs_name,
int eq) {

	char eqbuf[7];

	snprintf(eqbuf, 7, "%d", eq);
	if (SetFsPartCmd(fs_name, eqbuf, DK_CMD_release) != 0) {
		samerrno = SE_FS_DEV_RELEASE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), eq);
		return (-1);
	}

	return (0);
}


int
shrink_release(
ctx_t *c		/* ARGSUSED */,
char *fs_name,
int eq_to_release,
char *kv_options) {

	int ret_val;

	ret_val = shrink(fs_name, eq_to_release, -1,
	    kv_options, B_TRUE);

	Trace(TR_MISC, "shrink release returning %d", ret_val);
	return (ret_val);
}

int
shrink_remove(
ctx_t *c		/* ARGSUSED */,
char *fs_name,
int eq_to_release,
int replacement_eq,
char *kv_options) {

	int ret_val;

	ret_val = shrink(fs_name, eq_to_release, replacement_eq,
	    kv_options, B_FALSE);

	Trace(TR_MISC, "shrink release returning %d", ret_val);

	return (ret_val);
}


int
shrink_replace_group(
ctx_t *c,
char *fs_name,
int eq_to_replace,
striped_group_t *replacement,
char *kv_options) {

	int ret_val;


	Trace(TR_MISC, "shrink replace group %d entry", eq_to_replace);


	if (ISNULL(fs_name, replacement)) {
		Trace(TR_ERR, "shrink replace group failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	ret_val = shrink_replace(c, fs_name, eq_to_replace, NULL, replacement,
	    kv_options);

	Trace(TR_MISC, "shrink replace group returning %d", ret_val);

	return (ret_val);
}



int
shrink_replace_device(
ctx_t *c,
char *fs_name,
int eq_to_replace,
disk_t *replacement,
char *kv_options) {

	int ret_val;

	Trace(TR_MISC, "shrink replace group %d entry", eq_to_replace);

	if (ISNULL(fs_name, replacement)) {
		Trace(TR_ERR, "shrink replace device failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	ret_val = shrink_replace(c, fs_name, eq_to_replace, replacement, NULL,
	    kv_options);

	Trace(TR_MISC, "shrink relplace_device returning %d", ret_val);

	return (ret_val);
}


/*
 * This essentially boils down to an online grow of the
 * file system followed by a shrink_remove
 */
static int
shrink_replace(
ctx_t *c,
char *fs_name,
int eq_to_replace,
disk_t *dk_replacement,
striped_group_t *sg_replacement,
char *kv_options) {


	fs_t		*fs = NULL;
	fs_t		*post_grow_fs;
	sqm_lst_t	*new_meta = NULL;
	sqm_lst_t	*new_data = NULL;
	sqm_lst_t	*new_group = NULL;
	sqm_lst_t	*new_dev_lst;
	disk_t		*dsk_being_removed;
	disk_t		*replacement_post_grow;
	char		*replacement_path = NULL;
	void		*replacement = NULL;
	int		new_dev_type;
	int		ret;


	/*
	 * check dk_replacemnt or sg_replacement after we know which should
	 * be non-NULL
	 */
	if (ISNULL(fs_name)) {
		Trace(TR_MISC, "shrink replace failed: %d %s", samerrno,
		    samerrmsg);
		return (-1);
	}

	/*
	 * You cannot shrink the fs unless it is mounted. Check this before
	 * performing the grow.
	 */
	ret = get_fs(NULL, fs_name, &fs);
	if (ret == -1) {
		Trace(TR_ERR, "shrinking fs failed: %s", samerrmsg);
		return (-1);
	}
	if (ret == -2 || !(fs->fi_status & FS_MOUNTED)) {
		setsamerr(SE_SHRINK_FS_NOT_MOUNTED);
		free_fs(fs);
		Trace(TR_ERR, "shrink replace device failed:%s", samerrmsg);
		return (-1);
	}

	/*
	 * Find the device to remove to make sure it exists prior to
	 * getting too far.
	 */
	dsk_being_removed = find_disk_by_eq(fs, eq_to_replace);
	if (dsk_being_removed == NULL) {
		free_fs(fs);
		Trace(TR_ERR, "shrink replace device failed:%s", samerrmsg);
		return (-1);
	}

	/* Create and populate the appropriate argument list for grow */
	new_dev_lst = lst_create();
	if (new_dev_lst == NULL) {
		free_fs(fs);
		Trace(TR_ERR, "shrink replace device failed:%s", samerrmsg);
		return (-1);
	}


	/*
	 * Check to see if the disk being removed is metadata, data or a
	 * striped group. This will determine which grow argument should
	 * be non-NULL.
	 */
	new_dev_type = nm_to_dtclass(dsk_being_removed->base_info.equ_type);
	if (new_dev_type == DT_META) {
		if (dk_replacement != NULL) {
			new_meta = new_dev_lst;
			replacement = dk_replacement;
			replacement_path = dk_replacement->au_info.path;
		}
	} else if ((new_dev_type & DT_STRIPE_GROUP_MASK) == DT_STRIPE_GROUP) {
		if (sg_replacement != NULL &&
		    sg_replacement->disk_list != NULL &&
		    sg_replacement->disk_list->head != NULL &&
		    sg_replacement->disk_list->head->data != NULL) {
			disk_t *tmp_dk =
			    (disk_t *)sg_replacement->disk_list->head->data;

			new_group = new_dev_lst;
			replacement = sg_replacement;
			replacement_path = tmp_dk->au_info.path;
		}
	} else  {
		if (dk_replacement != NULL) {
			/*
			 * set the data disk list. Grow will sort out
			 * wether it is an mr or md
			 */
			new_data = new_dev_lst;
			replacement = dk_replacement;
			replacement_path = dk_replacement->au_info.path;
		}
	}
	if (ISNULL(replacement)) {
		free_fs(fs);
		lst_free(new_dev_lst);
		Trace(TR_ERR, "shrink replace device failed:%s", samerrmsg);
		return (-1);
	}


	if (lst_append(new_dev_lst, replacement) != 0) {
		free_fs(fs);
		lst_free(new_dev_lst);
		Trace(TR_ERR, "shrink replace device failed:%s", samerrmsg);
		return (-1);
	}


	if (_grow_fs(c, fs, new_meta, new_data, new_group) != 0) {
		free_fs(fs);
		lst_free(new_dev_lst);
		Trace(TR_ERR, "shrink replace device failed:%s", samerrmsg);
		return (-1);
	}

	free_fs(fs);

	/*
	 * Refetch the file system and find the disk we just added to
	 * to get the eq that was set inside the grow function. Note
	 * this works for striped group too as only the eq for the
	 * first disk in the group is needed.
	 */
	if (get_fs(NULL, fs_name, &post_grow_fs) == -1) {
		lst_free(new_dev_lst);
		Trace(TR_ERR, "shrink replace device failed:%s", samerrmsg);
		return (-1);
	}

	replacement_post_grow =
	    find_disk_by_path(post_grow_fs, replacement_path);

	if (replacement_post_grow == NULL) {
		free_fs(post_grow_fs);
		lst_free(new_dev_lst);
		Trace(TR_ERR, "shrink replace device failed:%s", samerrmsg);
		return (-1);
	}


	if (shrink(fs_name, eq_to_replace, replacement_post_grow->base_info.eq,
	    kv_options, B_FALSE) != 0) {
		free_fs(post_grow_fs);
		lst_free(new_dev_lst);
		Trace(TR_ERR, "shrink replace device failed:%s", samerrmsg);
		return (-1);
	}

	/* free the lst but not the replacement argument */
	lst_free(new_dev_lst);
	free_fs(post_grow_fs);

	Trace(TR_MISC, "shrink replace succeeded for:%s", fs_name);
	return (0);
}


static int
shrink(char *fs_name, int eq_to_exclude, int replacement_eq,
    char *kv_options, boolean_t release) {


	fs_t *fs;
	int ret;
	sqm_lst_t *eqs;
	timespec_t waitspec = {1, 0};


	/* ignore NULL kv_options at this point */
	if (ISNULL(fs_name)) {
		Trace(TR_ERR, "shrinking fs failed: %s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "shrinking fs %s exclude: %d, replacement: %d",
	    fs_name, eq_to_exclude, replacement_eq);

	/*
	 * get the file system to check the superblock version and
	 * see if it is mounted or not.
	 */
	ret = get_fs(NULL, fs_name, &fs);
	if (ret == -1) {
		Trace(TR_ERR, "shrinking fs failed: %s", samerrmsg);
		return (-1);
	}
	if (ret == -2 || !(fs->fi_status & FS_MOUNTED)) {
		setsamerr(SE_SHRINK_FS_NOT_MOUNTED);
		free_fs(fs);
		Trace(TR_ERR, "shrink failed:%s", samerrmsg);
		return (-1);
	}


	/* set the shrink options if non-NULL */
	if (kv_options && *kv_options != '\0') {
		if (set_shrink_options(fs_name, kv_options) != 0) {
			free_fs(fs);
			Trace(TR_ERR, "shrinking fs failed: %s", samerrmsg);
			return (-1);
		}
	}

	/*
	 * The device must be set noalloc for the shrink to succeed.
	 * Call _set_device_state it will check that the eq is part of
	 * the fs and verify that the state is noalloc and if not
	 * will set the state to noalloc.0
	 */
	eqs = lst_create();
	if (eqs == NULL) {
		free_fs(fs);
		Trace(TR_ERR, "shrinking fs failed: %s", samerrmsg);
		return (-1);
	}
	if (lst_append(eqs, &eq_to_exclude) != 0) {
		lst_free(eqs);
		free_fs(fs);
		Trace(TR_ERR, "shrinking fs failed: %s", samerrmsg);
		return (-1);
	}
	if (_set_device_state(fs, eqs, DEV_NOALLOC) != 0) {
		lst_free(eqs);
		free_fs(fs);
		Trace(TR_ERR, "shrinking fs failed: %s", samerrmsg);
		return (-1);
	}

	lst_free(eqs);

	/*
	 * Sleep for 1 second to give the device state a chance to
	 * reflect DEV_NOALLOC.
	 */
	nanosleep(&waitspec, NULL);


	if (release) {
		if (fs_dev_release(fs->fi_name, eq_to_exclude) != 0) {
			free_fs(fs);
			Trace(TR_ERR, "shrinking fs failed: %s", samerrmsg);
			return (-1);
		}
	} else {
		/*
		 * Core support for specifying the replacement device
		 * is not yet available. For now simply call fs_dev_remove.
		 */
		if (fs_dev_remove(fs->fi_name, eq_to_exclude) != 0) {
			free_fs(fs);
			Trace(TR_ERR, "shrinking fs failed: %s", samerrmsg);
			return (-1);
		}
	}

	Trace(TR_MISC, "releasing %d from fs %s succeeded", eq_to_exclude,
	    fs->fi_name);

	free_fs(fs);

	return (0);
}


static disk_t *
find_disk_by_eq(fs_t *f, int eq) {
	node_t *n;
	node_t *sgn;

	if (ISNULL(f)) {
		return (NULL);
	}

	for (n = f->meta_data_disk_list->head; n != NULL; n = n->next) {
		disk_t *dsk = (disk_t *)n->data;
		if (dsk != NULL && dsk->base_info.eq == eq) {
			return (dsk);
		}
	}
	for (n = f->data_disk_list->head; n != NULL; n = n->next) {
		disk_t *dsk = (disk_t *)n->data;
		if (dsk != NULL && dsk->base_info.eq == eq) {
			return (dsk);
		}
	}
	for (sgn = f->striped_group_list->head; sgn != NULL; sgn = sgn->next) {
		striped_group_t *sg = (striped_group_t *)sgn->data;
		if (sg == NULL || sg->disk_list == NULL) {
			continue;
		}
		for (n = sg->disk_list->head; n != NULL; n = n->next) {
			disk_t *dsk = (disk_t *)n->data;
			if (dsk != NULL && dsk->base_info.eq == eq) {
				return (dsk);
			}
		}
	}

	samerrno = SE_EQ_NOT_FOUND;
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), eq);

	return (NULL);
}

static disk_t *
find_disk_by_path(fs_t *f, char *path) {
	node_t *n;
	node_t *sgn;

	if (ISNULL(f, path)) {
		return (NULL);
	}

	for (n = f->meta_data_disk_list->head; n != NULL; n = n->next) {
		disk_t *dsk = (disk_t *)n->data;
		if (dsk != NULL && strcmp(dsk->base_info.name, path) == 0) {
			return (dsk);
		}
	}
	for (n = f->data_disk_list->head; n != NULL; n = n->next) {
		disk_t *dsk = (disk_t *)n->data;
		if (dsk != NULL && strcmp(dsk->base_info.name, path) == 0) {
			return (dsk);
		}
	}
	for (sgn = f->striped_group_list->head; sgn != NULL; sgn = sgn->next) {
		striped_group_t *sg = (striped_group_t *)sgn->data;
		if (sg == NULL || sg->disk_list == NULL) {
			continue;
		}
		for (n = sg->disk_list->head; n != NULL; n = n->next) {
			disk_t *dsk = (disk_t *)n->data;
			if (dsk != NULL &&
			    strcmp(dsk->base_info.name, path) == 0) {
				return (dsk);
			}
		}
	}

	samerrno = SE_PATH_NOT_FOUND;
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), path);

	return (NULL);
}


/*
 * Function to locate devices for a shared file system client or potential
 * metadata server.
 *
 * The input arguments are lists of disk_t structures required
 * to either create or grow a file system. If matching devices are found
 * the device paths of the input arguments will be overwritten with the
 * local host's paths. Otherwise an error will be returned.
 *
 * If this host is a client in the shared file system set the mms input
 * to NULL so that failure to discover the unneeded mm devices does not
 * lead to an error.
 */
static int
find_local_devices(sqm_lst_t *mms, sqm_lst_t *data, sqm_lst_t *sgs) {

	sqm_lst_t *aus;
	node_t *n;
	node_t *sg_n;
	boolean_t need_ha_devs = B_FALSE;

	/*
	 * Determine if the file system uses did devices. If so
	 * perform discover_ha_aus too.
	 */
	if (mms != NULL) {
		for (n = mms->head; n != NULL; n = n->next) {
			disk_t *dsk = (disk_t *)n->data;

			if (dsk == NULL) {
				continue;
			}

			if (strstr(dsk->base_info.name,
			    "/dev/did/") != NULL) {
				need_ha_devs = B_TRUE;
				break;
			}
		}
	}

	if (!need_ha_devs && data != NULL) {
		for (n = data->head; n != NULL; n = n->next) {
			disk_t *dsk = (disk_t *)n->data;

			if (dsk == NULL) {
				continue;
			}

			if (strstr(dsk->base_info.name,
			    "/dev/did/") != NULL) {
				need_ha_devs = B_TRUE;
				break;
			}
		}
	}


	if (!need_ha_devs && sgs != NULL) {
		for (sg_n = sgs->head; !need_ha_devs && sg_n != NULL;
			sg_n = sg_n->next) {
			striped_group_t *sg = (striped_group_t *)sg_n->data;

			if (sg == NULL) {
				continue;
			}

			for (n = sg->disk_list->head; n != NULL; n = n->next) {
				disk_t *dsk = (disk_t *)n->data;

				if (dsk == NULL) {
					continue;
				}

				if (strstr(dsk->base_info.name,
				    "/dev/did/") != NULL) {
					need_ha_devs = B_TRUE;
					break;
				}
			}
		}
	}

	if (need_ha_devs) {
		if (discover_ha_aus(NULL, NULL,  B_TRUE, &aus) != 0) {
			Trace(TR_ERR, "Unable to match devices: %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
	} else {
		if (discover_aus(NULL, &aus) != 0) {
			Trace(TR_ERR, "Unable to match devices: %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
	}

	if (mms != NULL && mms->length != 0) {
		if (find_local_device_paths(mms, aus)  != 0) {
			free_au_list(aus);
			Trace(TR_ERR, "Unable to match devices: %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
	}

	if (data != NULL && data->length != 0) {
		if (find_local_device_paths(data, aus)  != 0) {
			free_au_list(aus);
			Trace(TR_ERR, "Unable to match devices: %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}
	}
	if (sgs != NULL && sgs->length != 0) {
		for (sg_n = sgs->head; sg_n != NULL; sg_n = sg_n->next) {
			striped_group_t *sg = (striped_group_t *)sg_n->data;
			if (find_local_device_paths(sg->disk_list, aus) != 0) {
				free_au_list(aus);
				Trace(TR_ERR, "Unable to match devices: %d %s",
				    samerrno, samerrmsg);
				return (-1);
			}
		}
	}
	free_au_list(aus);
	Trace(TR_MISC, "Found the local devices");
	return (0);
}




mount_options_t *
dup_mount_options(mount_options_t *mo) {
	mount_options_t *copy;

	if (ISNULL(mo)) {
		Trace(TR_ERR, "dup mount options failed: %s",
		    samerrmsg);
		return (NULL);
	}

	copy = (mount_options_t *)mallocer(sizeof (mount_options_t));
	if (copy == NULL) {
		Trace(TR_ERR, "dup mount options failed: %s",
		    samerrmsg);
		return (NULL);
	}

	memcpy(copy, mo, sizeof (mount_options_t));

	return (copy);
}


fs_t *
dup_fs(fs_t *src) {
	fs_t *fs;

	if (ISNULL(src)) {
		Trace(TR_ERR, "dup fs failed: %s", samerrmsg);
		return (NULL);
	}

	fs = (fs_t *)mallocer(sizeof (fs_t));
	if (fs == NULL) {
		Trace(TR_ERR, "dup fs failed: %s", samerrmsg);
		return (NULL);
	}

	memcpy(fs, src, sizeof (fs_t));

	/* reset pointers so free_fs can be used if an error is encountered */
	fs->mount_options = NULL;
	fs->meta_data_disk_list = NULL;
	fs->data_disk_list = NULL;
	fs->striped_group_list = NULL;
	fs->hosts_config = NULL;

	if (lst_dup_typed(src->meta_data_disk_list,
	    &(fs->meta_data_disk_list),
	    DUPFUNCCAST(dup_disk), FREEFUNCCAST(free_disk)) != 0) {
		free_fs(fs);
		Trace(TR_MISC, "dup fs failed: %d %s", samerrno,
		    samerrmsg);
		return (NULL);
	}
	if (lst_dup_typed(src->data_disk_list, &(fs->data_disk_list),
	    DUPFUNCCAST(dup_disk), FREEFUNCCAST(free_disk)) != 0) {
		free_fs(fs);
		Trace(TR_MISC, "dup fs failed: %d %s", samerrno,
		    samerrmsg);
		return (NULL);
	}

	if (lst_dup_typed(src->striped_group_list,
	    &(fs->striped_group_list),
	    DUPFUNCCAST(dup_striped_group),
	    FREEFUNCCAST(free_striped_group)) != 0) {
		free_fs(fs);

		Trace(TR_MISC, "dup fs failed: %d %s", samerrno,
		    samerrmsg);
		return (NULL);
	}

	fs->mount_options = dup_mount_options(src->mount_options);
	if (fs->mount_options == NULL) {
		free_fs(fs);

		Trace(TR_MISC, "dup fs failed: %d %s", samerrno,
		    samerrmsg);
		return (NULL);
	}
	return (fs);
}


striped_group_t *
dup_striped_group(striped_group_t *sg) {
	striped_group_t *cpy;

	if (ISNULL(sg)) {
		return (NULL);
	}
	cpy = (striped_group_t *)mallocer(sizeof (striped_group_t));
	if (cpy == NULL) {
		return (NULL);
	}
	strlcpy(cpy->name, sg->name, sizeof (devtype_t));

	if (lst_dup_typed(sg->disk_list,
	    &(cpy->disk_list),
	    DUPFUNCCAST(dup_disk),
	    FREEFUNCCAST(free_disk)) != 0) {

		free(cpy);
		return (NULL);
	}
	return (cpy);
}
