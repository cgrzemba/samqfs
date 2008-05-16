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
#pragma ident   "$Revision: 1.38 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


/*
 * mount_cfg.c
 * contains function implementations for functions used to read, write,
 * and verify the samfs.cmd file.
 */


/* ANSI C headers. and Solaris headers. */
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* API Headers */
#include "sam/sam_trace.h"
#include "pub/devstat.h"
#include "pub/mgmt/device.h"
#include "mgmt/config/master_config.h"
#include "pub/mgmt/filesystem.h"
#include "mgmt/config/cfg_fs.h"
#include "mgmt/config/mount_cfg.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/types.h"
#include "mgmt/util.h"
#include "mgmt/config/vfstab.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/license.h"
#include "pub/mgmt/sqm_list.h"

/* full path to the samfs.cmd file */
static char *samfs_cmd_file = SAMFS_CFG;

static char *get_change_flag_str(fs_t *input);

/*
 * pass in a list of fs_t structures for all configured file systems.
 * the structures will be populated with mount options when returned.
 * If the first element in the cfg is not the GLOBAL fs that is used
 * for the global mount options one is inserted. If the caller does not
 * want to include those options they must remove and free that fs_t
 * structure.
 */
int
populate_mount_cfg(
mount_cfg_t *cfg)	/* struct to populate with mount options. */
{

	int err;
	node_t *n;
	fs_t *new_fs;
	sqm_lst_t *vfslist;

	Trace(TR_OPRMSG, "populating mount config");

	/*
	 * Put in global options at the head of the list.
	 * This is so that populating the mount options will succeed.
	 */
	if (create_fs_struct(GLOBAL, NULL, &new_fs) != 0) {
		Trace(TR_OPRMSG, "reading mount cfg failed: %s", samerrmsg);
		return (-1);

	}
	/* if the list is empty it means no filesystems exist. */
	if (cfg->fs_list->head == NULL) {
		if (lst_append(cfg->fs_list, new_fs) != 0) {
			Trace(TR_OPRMSG, "reading mount cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
	} else {
		if (lst_ins_before(cfg->fs_list, cfg->fs_list->head,
		    new_fs) != 0) {

			free_fs(new_fs);
			Trace(TR_OPRMSG, "reading mount cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	err = parse_samfs_cmd(samfs_cmd_file, cfg);
	if (err != 0) {
		Trace(TR_OPRMSG, "populating mount config failed: %s",
		    samerrmsg);
		return (-1);
	}

	/*
	 * Get the vfstab entries to populate mount points and
	 * configured mount options.
	 */
	if (get_all_vfstab_entries(&vfslist) != 0) {
		Trace(TR_OPRMSG, "populating mount config failed: %s",
		    samerrmsg);
		return (-1);
	}

	for (n = cfg->fs_list->head; n != NULL; n = n->next) {
		fs_t *fs = ((fs_t *)n->data);
		mount_options_t *mo = fs->mount_options;
		vfstab_entry_t *vfs_ent;
		node_t *tmpnd;

		if (strcmp(fs->fi_name, GLOBAL) == 0) {
			continue;
		}

		tmpnd = lst_search(vfslist, fs->fi_name, cmp_str_2_str_ptr);
		if (tmpnd == NULL) {
			/* No vfs entry found. This is legal so no error. */
			continue;
		}

		vfs_ent = (vfstab_entry_t *)tmpnd->data;

		if (add_mnt_opts_from_vfsent(vfs_ent, mo) != 0) {
			lst_free_deep_typed(vfslist,
			    FREEFUNCCAST(free_vfstab_entry));

			Trace(TR_OPRMSG, "getting vfstab options failed %s",
			    samerrmsg);
			return (-1);
		}

		if (vfs_ent->mount_point != NULL) {
			snprintf(fs->fi_mnt_point, sizeof (upath_t),
			    vfs_ent->mount_point);
		}
	}

	lst_free_deep_typed(vfslist, FREEFUNCCAST(free_vfstab_entry));

	Trace(TR_OPRMSG, "populated mount config");
	return (err);

}


/*
 * read the samfs.cmd file and populate a mount_cfg_t structure.
 */
int
read_mount_cfg(
mount_cfg_t **cfg)	/* malloced return value */
{

	mcf_cfg_t	*mcf;
	sqm_lst_t		*names, *devs, *fs_list;
	fs_t		*new_fs;
	node_t		*n;
	int		ret = 0;
	int		tmp_ret = 0;


	Trace(TR_OPRMSG, "read mount cfg");

	if (ISNULL(cfg)) {
		Trace(TR_ERR, "read mount cfg failed: %s", samerrmsg);
		return (-1);
	}
	if (read_mcf_cfg(&mcf) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "read mount cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (get_fs_family_set_names(mcf, &names) != 0) {
		free_mcf_cfg(mcf);
		Trace(TR_ERR, "read mount cfg failed: %s", samerrmsg);
		return (-1);
	}

	fs_list = lst_create();
	if (fs_list == NULL) {
		lst_free_deep(names);
		free_mcf_cfg(mcf);
		Trace(TR_ERR, "read mount cfg failed: %s", samerrmsg);
		return (-1);
	}

	for (n = names->head; n != NULL; n = n->next) {

		if (get_family_set_devs(mcf, (char *)n->data, &devs) != 0) {
			lst_free_deep(names);
			free_mcf_cfg(mcf);
			free_list_of_fs(fs_list);
			Trace(TR_ERR, "read mount cfg failed: %s",
			    samerrmsg);
			return (-1);
		}


		if ((tmp_ret = create_fs_struct((char *)n->data, devs,
		    &new_fs)) == -1) {

			free_mcf_cfg(mcf);
			lst_free_deep(names);
			lst_free_deep(devs);
			free_list_of_fs(fs_list);
			Trace(TR_ERR, "read mount cfg failed: %s",
			    samerrmsg);
			return (-1);
		} else if (tmp_ret == -2) {
			ret = -2;
		}

		/*
		 * free the devs list and its devices.
		 */
		lst_free_deep(devs);

		if (lst_append(fs_list, new_fs) != 0) {
			free_mcf_cfg(mcf);
			lst_free_deep(names);
			free_list_of_fs(fs_list);
			free_fs(new_fs);
			Trace(TR_ERR, "read mount cfg failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	lst_free_deep(names);
	free_mcf_cfg(mcf);

	/* now populate the mount options structures. */

	*cfg = (mount_cfg_t *)mallocer(sizeof (mount_cfg_t));
	if (*cfg == NULL) {
		free_list_of_fs(fs_list);
		Trace(TR_ERR, "read mount cfg failed: %s", samerrmsg);
		return (-1);
	}

	memset(*cfg, 0, sizeof (mount_cfg_t));
	(*cfg)->inodes = int_reset;
	(*cfg)->fs_list = fs_list;

	if (populate_mount_cfg(*cfg) != 0) {
		free_mount_cfg(*cfg);
		Trace(TR_ERR, "read mount cfg failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "read mount cfg. Returning %d", ret);
	return (ret);
}


/*
 * change the mount options for the fs.  This sets only those options that
 * have a setfield bit set all others remain unchanged.
 */
int
cfg_change_mount_options(
ctx_t *ctx,		/* contains the optional dump path */
fs_t *input,		/* the filesystem for which to change options */
boolean_t for_create)	/* if true and dump, fs need not exist in mcf */
{

	mount_cfg_t *mnt_cfg;
	node_t *n;
	fs_t *fs_to_file;
	boolean_t found = B_FALSE;
	boolean_t dump = B_FALSE;


	Trace(TR_OPRMSG, "changing mount options %s",
	    input->fi_name ? input->fi_name : "NULL");


	if (read_mount_cfg(&mnt_cfg) == -1) {
		/* leave samerrno as set */
		Trace(TR_OPRMSG, "changing mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	for (n = mnt_cfg->fs_list->head; n != NULL; n = n->next) {
		fs_to_file = (fs_t *)n->data;
		if (strcmp(input->fi_name, fs_to_file->fi_name) == 0) {
			found = B_TRUE;

			if (input->mount_options == NULL) {
				Trace(TR_OPRMSG, "changing mount options %s",
				    "Nothing to change");
				return (0);
			}

			Trace(TR_DEBUG, "before copysetfields %s",
			    get_change_flag_str(input));

			/*
			 * copying the set fields into a structure that
			 * contains the current options
			 * prevents fields that are not set from being
			 * automatically cleared.
			 */
			if (copy_fs_set_fields(fs_to_file, input) != 0) {
				Trace(TR_OPRMSG, "%s%s",
				    "changing mount options ",
				    samerrmsg);
				return (-1);
			}

			Trace(TR_DEBUG, "after copysetfields %s",
			    get_change_flag_str(fs_to_file));

			break;
		}
	}


	dump = ctx != NULL && *ctx->dump_path != '\0';
	if (!found) {
		/*
		 * if this is dump and a create, the fs will not be found
		 * and it is not an error. Append it.
		 */
		if (for_create && dump) {
			if (lst_append(mnt_cfg->fs_list, input) != 0) {
				free_mount_cfg(mnt_cfg);
				Trace(TR_OPRMSG, "%s() failed: %s",
				    "changing mount options", samerrmsg);
				return (-1);
			}
			/* set n so that later frees can work. */
			n = mnt_cfg->fs_list->tail;
		} else {
			samerrno = SE_NOT_FOUND;
			/* %s not found */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_NOT_FOUND), input->fi_name);
			free_mount_cfg(mnt_cfg);
			Trace(TR_OPRMSG, "changing mount options failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	/* write out the whole samfs.cmd file with the modified options */
	if (write_mount_cfg(ctx, mnt_cfg, B_FALSE, for_create) != 0) {
		free_mount_cfg(mnt_cfg);
		Trace(TR_OPRMSG, "changing mount options failed: %s",
		    samerrmsg);
		return (-1);
	}


	/* set only the vfstab mount options for the named file system. */
	if (!dump && set_vfstab_options(fs_to_file->fi_name,
	    fs_to_file->mount_options) != 0) {

		/* it is not an error for there to be no vfstab entry */
		if (samerrno != SE_NOT_FOUND) {
			free_mount_cfg(mnt_cfg);
			Trace(TR_OPRMSG, "changing mount options failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	/*
	 * If this function was called as part of
	 * a dump of a file system creation then the user supplied input
	 * has been appended to the mnt_cfg's list of file systems.
	 * It must not be freed.
	 */
	if (for_create && dump) {
		mnt_cfg->fs_list->tail->data = NULL;
	}

	free_mount_cfg(mnt_cfg);

	Trace(TR_OPRMSG, "changed mount options");

	return (0);
}


/*
 * This is equivalent of deleting the options for the named filesystem.
 */
int
reset_default_mount_options(
ctx_t *ctx,		/* contains the optional dump_path */
mount_cfg_t *mnt_cfg,
uname_t fs_name)	/* name of fs to reset mount options for */
{

	node_t *n;
	fs_t *fs_to_file;
	boolean_t found = B_FALSE;
	Trace(TR_OPRMSG, "resetting default mount options %s", fs_name);

	for (n = mnt_cfg->fs_list->head; n != NULL; n = n->next) {
		fs_to_file = (fs_t *)n->data;

		if (strcmp(fs_name, fs_to_file->fi_name) == 0) {
			found = B_TRUE;
			if (lst_remove(mnt_cfg->fs_list, n) != 0) {
				Trace(TR_OPRMSG, "%s() failed: %s",
				    "resetting default mount options",
				    samerrmsg);
				return (-1);
			}
			free_fs(fs_to_file);
			break;
		}
	}


	if (!found) {
		samerrno = SE_NOT_FOUND;
		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), fs_name);

		Trace(TR_OPRMSG, "resetting default mount options failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (write_mount_cfg(ctx, mnt_cfg, B_FALSE, B_FALSE) != 0) {
		Trace(TR_OPRMSG, "resetting default mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "reset default mount options");
	return (0);

}


int
get_default_mount_options(
ctx_t *ctx			/* ARGSUSED */,
devtype_t fs_type,
int dau_size,
boolean_t uses_striped_groups,
boolean_t shared,
boolean_t multi_reader,
mount_options_t **params)	/* malloced return value */
{

	Trace(TR_DEBUG, "getting default mount options");
	if (ISNULL(params)) {
		Trace(TR_DEBUG, "getting default mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (dau_size < 16) {

		samerrno = SE_INVALID_DAU;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_DAU), dau_size);

		Trace(TR_ERR, "getting default mount options failed: %s",
		    samerrmsg);
		return (-1);
	}
	if (get_default_mount_opts(params) != 0) {
		Trace(TR_ERR, "getting default mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* if the license says QFS only set archive (sam) to false */
	if (get_samfs_type(NULL) == QFS) {
		(*params)->sam_opts.archive = B_FALSE;
	}

	if (strcmp(fs_type, "ma") == 0) {

		/* if the fs is shared or contains striped groups stripe = 0 */
		if (shared || uses_striped_groups) {
			(*params)->stripe = 0;
			(*params)->sync_meta = 1;
		} else if (dau_size < 128 && dau_size != 0) {
			(*params)->stripe = (int)(128 / dau_size);
		} else {
			(*params)->stripe = 1;
		}

		(*params)->sharedfs_opts.maxallocsz = 128 * dau_size;
		(*params)->sharedfs_opts.minallocsz = 8 * dau_size;

	} else if (strcmp(fs_type, "ms") == 0) {
		if (dau_size < 128) {
			(*params)->stripe = (int)(128 / dau_size);
		} else {
			(*params)->stripe = 1;
		}

		/*
		 * the following are not really valid since this fs can't be
		 * shared but the core sets them this way.
		 */
		(*params)->sharedfs_opts.maxallocsz = 128 * dau_size;
		(*params)->sharedfs_opts.minallocsz = (long long)8 *
		    (long long)dau_size;
	} else {
		free(params);
		samerrno = SE_INVALID_FS_TYPE;

		/* %s is not a valid file system type */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_FS_TYPE), fs_type);

		Trace(TR_ERR, "getting default mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	(*params)->change_flag = (uint32_t)0;
	(*params)->io_opts.change_flag = (uint32_t)0;
	(*params)->sam_opts.change_flag = (uint32_t)0;
	(*params)->sharedfs_opts.change_flag = (uint32_t)0;
	(*params)->multireader_opts.change_flag = (uint32_t)0;
	(*params)->qfs_opts.change_flag = (uint32_t)0;

	Trace(TR_DEBUG, "got default mount options");
	return (0);
}


/*
 * get the mount options for the named filesystem.
 */
int
get_fs_mount_opts(
const mount_cfg_t *cfg,		/* cfg to search */
const char *fs_name,		/* fs for which to get mount options */
mount_options_t **params)	/* malloced return value */
{

	node_t *node;

	Trace(TR_DEBUG, "getting fs mount options for %s", Str(fs_name));

	if (ISNULL(cfg, fs_name)) {
		Trace(TR_DEBUG, "getting fs mount options failed: %s",
		    samerrmsg);
		return (-1);
	}


	for (node = cfg->fs_list->head;
	    node != NULL; node = node->next) {
		if (strcmp(((fs_t *)node->data)->fi_name,
		    fs_name) == 0) {
			*params = ((fs_t *)node->data)->mount_options;
			Trace(TR_DEBUG, "got fs mount options");
			return (0);
		}
	}
	*params = NULL;
	samerrno = SE_NOT_FOUND;

	/* %s not found */
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(SE_NOT_FOUND),
	    fs_name);

	Trace(TR_DEBUG, "getting fs mount options failed: %s", samerrmsg);
	return (-1);
}


/*
 * write the samfs.cmd file.
 */
int
write_mount_cfg(
ctx_t *ctx,		/* contains optional dump_path */
mount_cfg_t *cfg,	/* cfg to write */
const boolean_t force,	/* if true cfg will be written even if backup fails */
boolean_t for_create)	/* set to true if write is for create */
{

	Trace(TR_OPRMSG, "writing mount cfg");
	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "writing mount cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (verify_mount_cfg(ctx, cfg, for_create) != 0) {
		/* Leave samerrno as set */
		Trace(TR_OPRMSG, "writing mount cfg failed: %s", samerrmsg);
		return (-1);
	}


	/*
	 * if the context is non-NULL and contains a dump_path the file
	 * will be written to that location.
	 */
	if (ctx != NULL && *ctx->dump_path != '\0') {
		char *dmp_loc;

		dmp_loc = assemble_full_path(ctx->dump_path, SAMFS_DUMP_FILE,
		    B_FALSE, NULL);

		if (dump_mount_cfg(cfg, dmp_loc) != 0) {
			Trace(TR_OPRMSG, "writing mount cfg failed: %s",
			    samerrmsg);

			return (-1);
		}

		Trace(TR_MISC, "wrote mount cfg to %s", dmp_loc);
		return (0);
	}


	/* possibly backup the samfs.cmd (see backup_cfg for details) */
	if (backup_cfg(samfs_cmd_file) != 0 && !force) {
		/* Leave samerrno as set */
		Trace(TR_OPRMSG, "writing mount cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (write_samfs_cmd(samfs_cmd_file, cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_OPRMSG, "writing mount cfg failed: %s", samerrmsg);
		return (-1);
	} else {
		Trace(TR_OPRMSG, "wrote samfs.cmd file %s", samfs_cmd_file);
	}

	/* always backup the new file. */
	backup_cfg(samfs_cmd_file);

	Trace(TR_MISC, "wrote mount cfg to %s", samfs_cmd_file);
	return (0);

}

/*
 * verify that the mount configuration is valid.
 */
int
verify_mount_cfg(
ctx_t *ctx,	/* if dump and create use mcf at dump_path for checking */
mount_cfg_t *cfg,	/* mount_cfg to check */
boolean_t for_create)	/* true if verifying a newly created fs. */
{

	sqm_lst_t		*l;
	char		*mcf = NULL;
	char		ver_path[MAXPATHLEN+1];
	int		err;

	Trace(TR_DEBUG, "verifying mount cfg");

	if (ISNULL(cfg)) {
		Trace(TR_DEBUG, "verifying mount cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (mk_wc_path(SAMFS_CFG, ver_path, sizeof (ver_path)) != 0) {
		return (-1);
	}

	if (write_samfs_cmd(ver_path, cfg) != 0) {
		unlink(ver_path);
		Trace(TR_DEBUG, "verifying mount cfg failed: %s", samerrmsg);
		return (-1);
	}

	/* if this is for a dump of a create, set the alternate mcf file. */
	if (ctx != NULL && *ctx->dump_path != '\0' && for_create) {
		mcf = assemble_full_path(ctx->read_location, MCF_DUMP_FILE,
		    B_FALSE, NULL);
	}

	err = check_config_with_fsd(mcf, NULL, ver_path, NULL, &l);
	if (err == 0) {
		unlink(ver_path);
		Trace(TR_DEBUG, "verified mount cfg");
		return (0);
	}

	if (-1 != err) {
		samerrno = SE_VERIFY_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno),
		    l->head->data);
		lst_free_deep(l);
	}

	unlink(ver_path);
	Trace(TR_DEBUG, "verifying mount cfg %d %s", samerrno,
	    samerrmsg);

	return (-1);
}


/*
 * write the samfs.cmd file to an alternate location.
 */
int
dump_mount_cfg(
mount_cfg_t *cfg,	/* cfg to dump */
char *location)		/* location at which to dump the cfg */
{

	if (ISNULL(cfg, location)) {
		Trace(TR_DEBUG, "dumping mount cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (strcmp(location, SAMFS_CFG) == 0) {
		samerrno = SE_INVALID_DUMP_LOCATION;
		/* Cannot dump the file to %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_DUMP_LOCATION), location);
		Trace(TR_DEBUG, "dumping mount cfg failed: %s", samerrmsg);
		return (-1);
	}

	return (write_samfs_cmd(location, cfg));
}


/*
 * debug helper method. get a string of the change flags for each mount options
 * sub structure.
 */
static char *
get_change_flag_str(
fs_t *input)
{

	static upath_t output;

	sprintf(output,
	    "0x%08x \n0x%08x\n0x%08x \n0x%08x \n0x%08x \n0x%08x",
	    input->mount_options->change_flag,
	    input->mount_options->io_opts.change_flag,
	    input->mount_options->sam_opts.change_flag,
	    input->mount_options->sharedfs_opts.change_flag,
	    input->mount_options->multireader_opts.change_flag,
	    input->mount_options->qfs_opts.change_flag);

	return (output);
}
