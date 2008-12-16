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
#pragma ident   "$Revision: 1.49 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * archiver.c is the implementation of the interfaces in archive.h
 *
 * It provides functions to configure archiving and get information about
 * the current configuration.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include "pub/devstat.h"
#include "pub/mgmt/device.h"
#include "aml/archset.h"
#include "mgmt/config/archiver.h"
#include "pub/mgmt/error.h"
#include "mgmt/config/common.h"
#include "mgmt/util.h"
#include "sam/sam_trace.h"
#include "pub/mgmt/process_job.h"

static int remove_criteria_from_fs(ar_set_criteria_t *crit);

static int remove_criteria_from_global(ar_set_criteria_t *crit);

#define	ARCHIVER_LV	SBIN_DIR"/archiver -lv"
#define	ARCHIVER_DASH_C	SBIN_DIR"/archiver -c %s"



#define	ARCHIVE_FILES_CMD	BIN_DIR"/archive"

static int expand_archive_options(int32_t options, char *buf, int len);

int display_archive_activity(samrthread_t *ptr, char **result);

/*
 * get the global directive for archiver.
 */
int
get_ar_global_directive(
ctx_t *ctx				/* ARGSUSED */,
ar_global_directive_t **ar_global)	/* return- must be freed by caller */
{

	archiver_cfg_t *cfg = NULL;
	Trace(TR_MISC, "getting global directives");
	if (ISNULL(ar_global)) {
		Trace(TR_ERR, "getting global directives failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* just read it copy and free the cfg and return */
	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "getting global directives failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (dup_ar_global_directive(&cfg->global_dirs, ar_global) != 0) {
		free_archiver_cfg(cfg);

		Trace(TR_ERR, "getting global directives failed: %s",
		    samerrmsg);

		return (-1);
	}

	free_archiver_cfg(cfg);
	Trace(TR_MISC, "got global directives");
	return (0);
}


/*
 * set the archiver's global directives.
 */
int
set_ar_global_directive(
ctx_t *ctx,				/* contains optional dump path */
ar_global_directive_t *ar_global)	/* values to set */
{

	archiver_cfg_t *cfg = NULL;
	sqm_lst_t *old_crits;
	sqm_lst_t *all;

	Trace(TR_MISC, "setting global directives");

	if (ISNULL(ar_global)) {
		Trace(TR_ERR, "setting global directives failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* read in the config. */
	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "setting global directives failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* clean out the cfg's global struct. keeping the criteria list */
	lst_free_deep(cfg->global_dirs.ar_bufs);
	lst_free_deep(cfg->global_dirs.ar_max);
	lst_free_deep(cfg->global_dirs.ar_drives);
	lst_free_deep(cfg->global_dirs.ar_overflow_lst);

	old_crits = cfg->global_dirs.ar_set_lst;

	/*
	 * copy the incoming info into the cfgs global struct.
	 * NOTE: this is copying the pointers in too. They must not be freed.
	 */
	memcpy(&(cfg->global_dirs), ar_global, sizeof (ar_global_directive_t));


	/* create a list of all criteria in the new config */
	if (get_list_of_all_criteria(cfg, &all) != 0) {

		/* set pointers to null so user data is not freed. */
		cfg->global_dirs.ar_bufs = NULL;
		cfg->global_dirs.ar_max = NULL;
		cfg->global_dirs.ar_drives = NULL;
		cfg->global_dirs.ar_set_lst = NULL;
		cfg->global_dirs.ar_overflow_lst = NULL;

		free_archiver_cfg(cfg);
		free_ar_set_criteria_list(old_crits);
		Trace(TR_ERR, "setting global directives failed: %s",
		    samerrmsg);

		return (-1);
	}

	/*
	 * figure out what criteria are being removed and remove their
	 * copy params and maps
	 */
	if (remove_unneeded_maps_and_params(cfg,
	    old_crits, all) != 0) {
		/* set pointers to null so user data is not freed. */
		cfg->global_dirs.ar_bufs = NULL;
		cfg->global_dirs.ar_max = NULL;
		cfg->global_dirs.ar_drives = NULL;
		cfg->global_dirs.ar_set_lst = NULL;
		cfg->global_dirs.ar_overflow_lst = NULL;

		free_archiver_cfg(cfg);
		free_ar_set_criteria_list(old_crits);
		lst_free(all);
		Trace(TR_ERR, "setting global directives failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* write cfg */
	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {

		/* set pointers to null so user data is not freed. */
		cfg->global_dirs.ar_bufs = NULL;
		cfg->global_dirs.ar_max = NULL;
		cfg->global_dirs.ar_drives = NULL;
		cfg->global_dirs.ar_set_lst = NULL;
		cfg->global_dirs.ar_overflow_lst = NULL;
		free_archiver_cfg(cfg);
		free_ar_set_criteria_list(old_crits);
		lst_free(all);

		/* leave samerrno as set */
		Trace(TR_ERR, "setting global directives failed: %s",
		    samerrmsg);

		return (-1);
	}


	/* free cfg */
	/* set pointers to null so user data is not freed. */
	cfg->global_dirs.ar_bufs = NULL;
	cfg->global_dirs.ar_max = NULL;
	cfg->global_dirs.ar_drives = NULL;
	cfg->global_dirs.ar_set_lst = NULL;
	cfg->global_dirs.ar_overflow_lst = NULL;
	free_archiver_cfg(cfg);
	free_ar_set_criteria_list(old_crits);
	lst_free(all);

	Trace(TR_MISC, "set global directives");

	return (0);
}


/*
 * get a specific archive set.
 */
int
get_ar_set(
ctx_t *ctx			/* ARGSUSED */,
const uname_t set_name,		/* name of set to get */
sqm_lst_t **ar_set_criteria_list) /* list of ar_set_criteria_t for set */
{

	archiver_cfg_t *cfg = NULL;

	Trace(TR_ERR, "getting archive set %s", Str(set_name));
	if (ISNULL(ar_set_criteria_list)) {
		Trace(TR_ERR, "getting archive set failed: %s", samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(ctx, &cfg) != 0) {

		/* leave samerrno as set */
		Trace(TR_ERR, "getting archive set failed: %s", samerrmsg);
		return (-1);
	}

	if (cfg_get_criteria_by_name(cfg, set_name,
	    ar_set_criteria_list) != 0) {

		free_archiver_cfg(cfg);

		/* leave samerrno as set */
		Trace(TR_ERR, "getting archive set failed: %s", samerrmsg);
		return (-1);
	}


	free_archiver_cfg(cfg);
	Trace(TR_MISC, "got archive set");
	return (0);

}


/*
 * get all the archive set criteria for a specific file system.
 */
int
get_ar_set_criteria_list(
ctx_t *ctx			/* ARGSUSED */,
const uname_t fs_name,		/* name of fs to get criteria for */
sqm_lst_t **ar_set_criteria_list)	/* malloced list of ar_set_criteria_t */
{

	archiver_cfg_t *cfg = NULL;

	Trace(TR_ERR, "getting archive set criteria");

	if (ISNULL(fs_name, ar_set_criteria_list)) {
		Trace(TR_ERR, "getting archive set criteria failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "getting archive set criteria failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (strcmp(fs_name, GLOBAL) == 0) {
		*ar_set_criteria_list = cfg->global_dirs.ar_set_lst;
		cfg->global_dirs.ar_set_lst = NULL;

	} else if (cfg_get_criteria_by_fs(cfg, fs_name,
	    ar_set_criteria_list) != 0) {

		free_archiver_cfg(cfg);
		/* leave samerrno as set */
		Trace(TR_ERR, "getting archive set criteria failed: %s",
		    samerrmsg);
		return (-1);
	}


	free_archiver_cfg(cfg);
	Trace(TR_MISC, "got archive set criteria");
	return (0);

}


/*
 * get all the archive set criteria.
 */
int
get_all_ar_set_criteria(
ctx_t *ctx,
sqm_lst_t **ar_set_criteria_list)	/* malloced list of ar_set_criteria_t */
{

	archiver_cfg_t *cfg = NULL;
	node_t *node;

	Trace(TR_MISC, "getting all ar_set_criteria");

	if (ISNULL(ar_set_criteria_list)) {
		Trace(TR_ERR, "getting all ar_set_criteria failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "getting all ar_set_criteria failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (cfg->global_dirs.ar_set_lst != NULL) {
		*ar_set_criteria_list = cfg->global_dirs.ar_set_lst;

		/* set it to null so cfg can be freed */
		cfg->global_dirs.ar_set_lst = NULL;
	} else {

		/* create a list to contain the return values */
		*ar_set_criteria_list = lst_create();
		if (*ar_set_criteria_list == NULL) {
			free_archiver_cfg(cfg);

			Trace(TR_ERR, "getting all ar_set_criteria failed: %s",
			    samerrmsg);

			return (-1);
		}
	}
	if (cfg->ar_fs_p == NULL) {
		free_archiver_cfg(cfg);
		lst_free_deep(*ar_set_criteria_list);
		Trace(TR_ERR, "getting all ar_set_criteria failed: %s",
		    samerrmsg);

		return (-1);
	}

	for (node = cfg->ar_fs_p->head; node != NULL; node = node->next) {

		ar_fs_directive_t *fs = (ar_fs_directive_t *)node->data;
		if (fs->ar_set_criteria != NULL) {

			if (lst_concat(*ar_set_criteria_list,
			    fs->ar_set_criteria) != 0) {
				free_archiver_cfg(cfg);
				lst_free_deep(*ar_set_criteria_list);

				Trace(TR_ERR, "%s failed: %s",
				    "getting all ar_set_criteria",
				    samerrmsg);

				return (-1);
			}

			fs->ar_set_criteria = NULL;
		}
	}

	free_archiver_cfg(cfg);
	Trace(TR_MISC, "got all ar_set_criteria");
	return (0);

}


/*
 * remove the criteria from the configuration. The criteria's name, fs_name
 * and key are used to match this criteria to the one in the config.
 */
int
remove_ar_set_criteria(
ctx_t *ctx			/* ARGSUSED */,
ar_set_criteria_t *crit)	/* criteria to remove see above */
{

	Trace(TR_MISC, "removing ar_set_criteria");


	if (ISNULL(crit)) {
		Trace(TR_ERR, "removing ar_set_criteria failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (strcmp(crit->fs_name, GLOBAL) == 0) {
		if (remove_criteria_from_global(crit) != 0) {

			Trace(TR_ERR, "removing ar_set_criteria failed: %s",
			    samerrmsg);
			return (-1);
		}
	} else {
		if (remove_criteria_from_fs(crit) != 0) {

			Trace(TR_ERR, "removing ar_set_criteria failed: %s",
			    samerrmsg);

			return (-1);
		}
	}

	Trace(TR_MISC, "removed ar_set_criteria");
	return (0);
}


/*
 * remove the criteria from the configuration. Matching of this criteria to the
 * configuration is done using the key and the name.
 */
static int
remove_criteria_from_global(
ar_set_criteria_t *crit)	/* criteria to remove. */
{

	ar_global_directive_t *glob;
	ar_set_criteria_t *tmp;
	boolean_t found = B_FALSE;
	node_t *n;

	Trace(TR_OPRMSG, "removing global criteria");
	if (ISNULL(crit)) {
		Trace(TR_ERR, "removing global criteria failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (get_ar_global_directive(NULL, &glob) != 0) {
		Trace(TR_ERR, "removing global criteria failed: %s",
		    samerrmsg);
		return (-1);
	}


	for (n = glob->ar_set_lst->head; n != NULL; n = n->next) {

		/* check for set name and key match */
		tmp = (ar_set_criteria_t *)n->data;
		if (strcmp(crit->set_name, tmp->set_name) == 0 &&
		    keys_match(tmp->key, crit->key)) {


			if (lst_remove(glob->ar_set_lst, n) != 0) {
				free_ar_global_directive(glob);
				Trace(TR_ERR, "%s failed: %s",
				    "removing global criteria", samerrmsg);

				return (-1);
			}
			found = B_TRUE;
			break;
		}

	}

	if (!found) {
		free_ar_global_directive(glob);
		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), crit->set_name);

		Trace(TR_ERR, "removing global criteria failed: %s",
		    samerrmsg);
		return (-1);
	}

	free_ar_set_criteria(tmp);

	if (set_ar_global_directive(NULL, glob) != 0) {
		free_ar_global_directive(glob);
		Trace(TR_ERR, "removing global criteria failed: %s",
		    samerrmsg);
		return (-1);
	}
	free_ar_global_directive(glob);
	Trace(TR_OPRMSG, "removed global criteria");
	return (0);

}


/*
 * remove the criteria from an fs.
 * matching is done based on the fs name, set name and key.
 */
static int
remove_criteria_from_fs(
ar_set_criteria_t *crit)	/* criteria to remove */
{

	ar_fs_directive_t *fs;
	ar_set_criteria_t *tmp;
	boolean_t found = B_FALSE;
	node_t *n;

	Trace(TR_OPRMSG, "removing fs criteria");
	if (ISNULL(crit)) {
		Trace(TR_ERR, "removing fs criteria failed: %s",
		    samerrmsg);
		return (-1);
	}


	if (get_ar_fs_directive(NULL, crit->fs_name, &fs) != 0) {
		Trace(TR_ERR, "removing fs criteria failed: %s",
		    samerrmsg);
		return (-1);
	}
	for (n = fs->ar_set_criteria->head;
	    n != NULL; n = n->next) {

		/* check for set name and key match */
		tmp = (ar_set_criteria_t *)n->data;
		if (strcmp(crit->set_name,
		    tmp->set_name) == 0 &&
		    keys_match(tmp->key, crit->key)) {

			free_ar_set_criteria((ar_set_criteria_t *)n->data);
			if (lst_remove(fs->ar_set_criteria, n) != 0) {
				free_ar_fs_directive(fs);
				Trace(TR_ERR, "%s exit %s",
				    "removing fs criteria",
				    samerrmsg);

				return (-1);
			}
			found = B_TRUE;
			break;
		}
	}

	if (!found) {
		free_ar_fs_directive(fs);
		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), crit->name);

		Trace(TR_ERR, "removing fs criteria failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (set_ar_fs_directive(NULL, fs) != 0) {
		free_ar_fs_directive(fs);

		Trace(TR_ERR, "removing fs criteria failed: %s",
		    samerrmsg);

		return (-1);
	}

	free_ar_fs_directive(fs);
	Trace(TR_OPRMSG, "removed fs criteria");
	return (0);

}


/*
 * modify the configuration to reflect the fields of crit that have their
 * change_flag bit set. The fs_name, set_name and key in
 * the ar_set_criteria are used to identify which criteria to modify.
 */
int
modify_ar_set_criteria(
ctx_t *ctx,			/* contains optional dump path */
ar_set_criteria_t *crit)	/* criteria with modifications */
{

	archiver_cfg_t *cfg = NULL;
	node_t *target = NULL;

	Trace(TR_MISC, "modifying ar_set_criteria");

	if (ISNULL(crit)) {
		Trace(TR_ERR, "modifying ar_set_criteria failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* read in the config. */
	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "modifying ar_set_criteria failed: %s",
		    samerrmsg);

		return (-1);
	}

	/*
	 * find the node containing the matching criteria in the current config
	 * matching is done based on based on setname and key
	 */
	if (find_ar_set_criteria_node(cfg, crit, &target) != 0) {

		free_archiver_cfg(cfg);

		Trace(TR_ERR, "modifying ar_set_criteria failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* replace the cfg criteria with the input arg. */
	free_ar_set_criteria(target->data);
	target->data = crit;

	/* write it */
	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {
		/*
		 * set target's data to null so the user arg wont get freed
		 * with the cfg
		 */
		target->data = NULL;
		free_archiver_cfg(cfg);

		Trace(TR_ERR, "modifying ar_set_criteria failed: %s",
		    samerrmsg);

		/* leave samerrno as set */
		return (-1);
	}

	/*
	 * set target's data to null so the user arg won't get freed
	 * with the cfg
	 */
	target->data = NULL;
	free_archiver_cfg(cfg);

	Trace(TR_MISC, "modified ar_set_criteria %s %s", crit->fs_name,
	    crit->set_name);
	return (0);
}


/*
 * get all file systems directives for archiving.
 */
int
get_all_ar_fs_directives(
ctx_t *ctx			/* ARGSUSED */,
sqm_lst_t **ar_fs_directive_list) /* malloced list of all ar_fs_directives */
{

	archiver_cfg_t *cfg = NULL;
	node_t *n;


	Trace(TR_MISC, "getting all ar_fs_directives");

	if (ISNULL(ar_fs_directive_list)) {
		Trace(TR_ERR, "getting all ar_fs_directives failed: %s",
		    samerrmsg);
		return (-1);
	}


	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "getting all ar_fs_directives failed: %s",
		    samerrmsg);

		return (-1);
	}


	/*
	 * remove directives for file systems that have no archive set and
	 * and shared file systems
	 */
	for (n = cfg->ar_fs_p->head; n != NULL; /* update in loop */) {
		ar_fs_directive_t *fsdir = (ar_fs_directive_t *)n->data;

		if (fsdir->change_flag & AR_FS_no_archive) {

			node_t *next = n->next;
			free_ar_fs_directive(fsdir);

			if (lst_remove(cfg->ar_fs_p, n) != 0) {
				free_archiver_cfg(cfg);
				Trace(TR_ERR, "getting all %s: %s",
				    "ar_fs_directives failed",
				    samerrmsg);

				return (-1);
			}
			n = next;
		} else {
			n = n->next;
		}
	}

	*ar_fs_directive_list = cfg->ar_fs_p;
	cfg->ar_fs_p = NULL;

	free_archiver_cfg(cfg);
	Trace(TR_MISC, "got all ar_fs_directives");
	return (0);

}


/*
 * get a specific file systems directive for archiving.
 */
int
get_ar_fs_directive(
ctx_t *ctx				/* ARGSUSED */,
const uname_t fs_name,			/* name of fs to get directive for */
ar_fs_directive_t **ar_fs_directive)	/* malloced return value */
{

	archiver_cfg_t *cfg = NULL;


	Trace(TR_MISC, "getting fs_directive for %s", Str(fs_name));

	if (ISNULL(ar_fs_directive)) {
		Trace(TR_ERR, "getting fs_directive failed: %s", samerrmsg);
		return (-1);
	}

	/* read, duplicate list and return */
	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "getting fs_directive failed: %s", samerrmsg);
		return (-1);
	}

	if (cfg_get_ar_fs(cfg, fs_name, B_TRUE, ar_fs_directive) != 0) {
		/* leave samerrno as set */
		free_archiver_cfg(cfg);
		Trace(TR_ERR, "getting fs_directive failed: %s", samerrmsg);
		return (-1);
	}

	free_archiver_cfg(cfg);
	Trace(TR_MISC, "got fs_directive");
	return (0);
}


/*
 * modify/set the configuration to match any fields with change flags
 */
int
set_ar_fs_directive(
ctx_t *ctx,		/* contains optional dump path */
ar_fs_directive_t *fs)	/* values to set */
{

	archiver_cfg_t *cfg = NULL;
	node_t *n;
	sqm_lst_t *all;
	ar_fs_directive_t *old_fs = NULL;
	int i;

	Trace(TR_MISC, "setting ar_fs_directive for %s",
	    fs ? Str(fs->fs_name) : "NULL");

	if (ISNULL(fs)) {
		Trace(TR_ERR, "setting ar_fs_directive failed: %s",
		    samerrmsg);
		return (-1);
	}


	/* read in the config. */
	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "setting ar_fs_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* find the fs */
	for (n = cfg->ar_fs_p->head; n != NULL; n = n->next) {
		if (strcmp(((ar_fs_directive_t *)n->data)->fs_name,
		    fs->fs_name) == 0) {
			old_fs = (ar_fs_directive_t *)n->data;
			break;
		}
	}

	if (old_fs == NULL) {
		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), fs->fs_name);

		free_archiver_cfg(cfg);

		Trace(TR_ERR, "setting ar_fs_directive failed: %s",
		    samerrmsg);

		return (-1);
	}


	/* replace the fs in the config with the argument */
	n->data = fs;


	/* create a list of all criteria */
	if (get_list_of_all_criteria(cfg, &all) != 0) {

		free_archiver_cfg(cfg);
		free_ar_fs_directive(old_fs);
		Trace(TR_ERR, "setting ar_fs_directive failed: %s",
		    samerrmsg);

		return (-1);
	}

	/*
	 * figure out what criteria are being removed and remove their
	 * copy params and maps
	 */
	if (remove_unneeded_maps_and_params(cfg,
	    old_fs->ar_set_criteria, all) != 0) {

		free_archiver_cfg(cfg);
		free_ar_fs_directive(old_fs);
		lst_free(all);
		Trace(TR_ERR, "setting ar_fs_directive failed: %s",
		    samerrmsg);

		return (-1);
	}


	/* now we need to deal with the old_fs metadata copies. */
	for (i = 0; i < MAX_COPY; i++) {

		if (old_fs->fs_copy[i] != NULL && fs->fs_copy[i] == NULL) {
			if (remove_maps_and_params(cfg,
			    old_fs->fs_name, i+1) != 0) {

				free_archiver_cfg(cfg);
				free_ar_fs_directive(old_fs);
				lst_free(all);
				Trace(TR_ERR, "%s failed: %s",
				    "setting ar_fs_directive", samerrmsg);
				return (-1);
			}
		}

	}

	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {
		/* reset the cfgs pointer to null so user data is not freed */
		n->data = NULL;
		free_archiver_cfg(cfg);
		free_ar_fs_directive(old_fs);
		lst_free(all);
		Trace(TR_ERR, "setting ar_fs_directive failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* reset the cfgs pointer to null so user data is not freed */
	n->data = NULL;
	free_archiver_cfg(cfg);
	free_ar_fs_directive(old_fs);
	lst_free(all);


	Trace(TR_MISC, "set ar_fs_directive %s", fs->fs_name);
	return (0);
}


/*
 * This is reset defaults because if the file system still exists it
 * will still have its default behaviors which are that the filesystem will
 * have one copy of its metadata every 4minutes.
 *
 * This function will remove all traces of this file systems entries being
 * careful not to remove entries for the archive sets if they exist in other
 * file systems/ globals.
 */
int
reset_ar_fs_directive(
ctx_t *ctx,		/* contains optional dump path */
const uname_t fs_name)	/* name of fs to remove ar_fs_directive for */
{

	archiver_cfg_t	*cfg;
	node_t		*n;
	int		i;
	sqm_lst_t	*all;
	ar_fs_directive_t *fs_dir = NULL;


	Trace(TR_MISC, "resetting ar_fs_directive for %s", Str(fs_name));

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "resetting ar_fs_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	/*
	 * locate and remove the ar_fs_directive.
	 * don't free it. instead must traverse it cleaning up everything
	 * related to the filesystem including the set_criteria and copy
	 * params and vsn_maps.
	 *
	 * for any copy params for fs copies remove them.
	 * for any vsn association for fs copies remove them.
	 */
	for (n = cfg->ar_fs_p->head; n != NULL; n = n->next) {
		if (strcmp(((ar_fs_directive_t *)n->data)->fs_name,
		    fs_name) == 0) {

			fs_dir = (ar_fs_directive_t *)n->data;

			if (lst_remove(cfg->ar_fs_p, n) != 0) {
				free_archiver_cfg(cfg);
				Trace(TR_ERR, "%s failed: %s",
				    "resetting ar_fs_directive",
				    samerrmsg);
				return (-1);
			}

			break;
		}
	}

	if (fs_dir == NULL) {
		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), fs_name);

		free_archiver_cfg(cfg);

		Trace(TR_ERR, "resetting ar_fs_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* create a list of all criteria in the new config */
	if (get_list_of_all_criteria(cfg, &all) != 0) {

		free_archiver_cfg(cfg);
		free_ar_fs_directive(fs_dir);
		Trace(TR_ERR, "resetting ar_fs_directive failed: %s",
		    samerrmsg);

		return (-1);
	}

	/*
	 * figure out what criteria are being removed and remove their
	 * copy params and maps
	 */
	if (remove_unneeded_maps_and_params(cfg,
	    fs_dir->ar_set_criteria, all) != 0) {

		free_archiver_cfg(cfg);
		free_ar_fs_directive(fs_dir);
		lst_free(all);
		Trace(TR_ERR, "resetting ar_fs_directive failed: %s",
		    samerrmsg);

		return (-1);
	}

	lst_free(all);

	/* remove the copy params and vsn maps for the fs copies. */
	for (i = 0; i < MAX_COPY; i++) {
		if (fs_dir->fs_copy[i] == NULL) {
			continue;
		}
		if (remove_maps_and_params(cfg, fs_dir->fs_name, i+1) != 0) {
			free_ar_fs_directive(fs_dir);
			free_archiver_cfg(cfg);
			Trace(TR_ERR, "resetting ar_fs_directive failed: %s",
			    samerrmsg);
			return (-1);
		}
	}

	/* free the removed fs directive */
	free_ar_fs_directive(fs_dir);

	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {
		/* leave samerrno as set */
		free_archiver_cfg(cfg);

		Trace(TR_ERR, "resetting ar_fs_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	free_archiver_cfg(cfg);


	Trace(TR_MISC, "reset ar_fs_directive %s", fs_name);
	return (0);
}


/*
 * get all copy parameters.
 */
int
get_all_ar_set_copy_params(
ctx_t *ctx			/* ARGSUSED */,
sqm_lst_t **ar_set_copy_params_list) /* malloced list of ar_set_copy_params */
{

	archiver_cfg_t *cfg = NULL;

	Trace(TR_MISC, "getting all ar_set_copy_params");
	if (ISNULL(ar_set_copy_params_list)) {
		Trace(TR_ERR, "getting all ar_set_copy_params failed: %s",
		    samerrmsg);
		return (-1);
	}



	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "getting all ar_set_copy_params failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* set return value and free cfg */
	*ar_set_copy_params_list = cfg->archcopy_params;
	cfg->archcopy_params = NULL;
	free_archiver_cfg(cfg);

	Trace(TR_MISC, "got all ar_set_copy_params");
	return (0);

}


/*
 * get the copy parameters for all copies defined for this archive set
 */
int
get_ar_set_copy_params_for_ar_set(
ctx_t *ctx			/* ARGSUSED */,
const char *ar_set_name,	/* name of set to get params for */
sqm_lst_t **ar_set_copy_params_list) /* malloced list of all params for set */
{

	uname_t name;
	archiver_cfg_t *cfg = NULL;
	ar_set_copy_params_t *tmp;
	int i;

	Trace(TR_MISC, "getting ar_set_copy_params for set %s",
	    Str(ar_set_name));

	if (ISNULL(ar_set_name, ar_set_copy_params_list)) {
		Trace(TR_ERR, "getting ar_set_copy_params for set failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "getting ar_set_copy_params for set failed: %s",
		    samerrmsg);

		return (-1);
	}

	*ar_set_copy_params_list = lst_create();
	if (*ar_set_copy_params_list == NULL) {
		free_archiver_cfg(cfg);

		Trace(TR_ERR, "getting ar_set_copy_params for set failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* This call will only return results for allsets */
	if (strcmp(ALL_SETS, ar_set_name) == 0) {
		if (cfg_get_ar_set_copy_params(cfg, ar_set_name, &tmp) == 0) {

			if (lst_append(*ar_set_copy_params_list, tmp) != 0) {
				Trace(TR_ERR, "%s failed: %s",
				    "getting ar_set_copy_params for set",
				    samerrmsg);

				free_archiver_cfg(cfg);
				lst_free(*ar_set_copy_params_list);
				return (-1);
			}

		} else {
			Trace(TR_ERR, "%s failed: %s",
			    "getting ar_set_copy_params for set", samerrmsg);

			free_archiver_cfg(cfg);
			lst_free(*ar_set_copy_params_list);
			return (-1);
		}
	}

	/* get the numbered copies */
	for (i = 1; i <= MAX_COPY; i++) {
		snprintf(name, sizeof (name), "%s.%d", ar_set_name, i);
		if (cfg_get_ar_set_copy_params(cfg, name, &tmp) != 0) {

			if (samerrno == SE_NOT_FOUND) {
				/*
				 * it is valid to not have params for a
				 * particular copy. That copy may not exist
				 */
				continue;
			}
			Trace(TR_ERR, "%s failed: %s",
			    "getting ar_set_copy_params for set", samerrmsg);

			free_archiver_cfg(cfg);
			free_ar_set_copy_params_list(*ar_set_copy_params_list);

			return (-1);

		} else {
			if (lst_append(*ar_set_copy_params_list, tmp) != 0) {
				Trace(TR_ERR, "%s failed: %s",
				    "getting ar_set_copy_params for set",
				    samerrmsg);

				free_archiver_cfg(cfg);
				free_ar_set_copy_params_list(
				    *ar_set_copy_params_list);
				return (-1);
			}
		}
	}


	free_archiver_cfg(cfg);
	if ((*ar_set_copy_params_list)->length == 0) {
		lst_free(*ar_set_copy_params_list);
		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), ar_set_name);

		Trace(TR_ERR, "getting ar_set_copy_params for set failed: %s",
		    samerrmsg);

		return (-1);
	}

	Trace(TR_MISC, "got ar_set_copy_params for set");
	return (0);
}


/*
 * get all copy parameters for a specific archive set.copy
 */
int
get_ar_set_copy_params(
ctx_t *ctx					/* ARGSUSED */,
const uname_t ar_set_copy_name,			/* set.copy_num */
ar_set_copy_params_t **ar_set_copy_parameters)	/* malloced return */
{

	archiver_cfg_t *cfg = NULL;

	Trace(TR_MISC, "getting ar_set_copy_params for %s",
	    Str(ar_set_copy_name));


	if (ISNULL(ar_set_copy_parameters)) {
		Trace(TR_ERR, "getting ar_set_copy_params failed: %s",
		    samerrmsg);
		return (-1);
	}


	*ar_set_copy_parameters = NULL;



	/* read, duplicate list and return */
	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "getting ar_set_copy_params failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (cfg_get_ar_set_copy_params(cfg, ar_set_copy_name,
	    ar_set_copy_parameters) != 0) {

		/* leave samerrno as set */
		free_archiver_cfg(cfg);

		Trace(TR_ERR, "getting ar_set_copy_params failed: %s",
		    samerrmsg);

		return (-1);
	}

	free_archiver_cfg(cfg);
	Trace(TR_MISC, "got ar_set_copy_params");

	return (0);
}


/*
 * sets the copy params in the cfg replacing any existing settings.
 */
int
set_ar_set_copy_params(
ctx_t *ctx,			/* contains optional dump_path */
ar_set_copy_params_t *params)	/* params to set */
{

	archiver_cfg_t *cfg = NULL;
	ar_set_copy_params_t *cfg_params;
	node_t *n;
	boolean_t found = B_FALSE;


	Trace(TR_MISC, "setting ar_set_copy_params");

	if (ISNULL(params)) {
		Trace(TR_ERR, "setting ar_set_copy_params failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "setting ar_set_copy_params failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* find the current entry */
	for (n = cfg->archcopy_params->head; n != NULL; n = n->next) {
		cfg_params = (ar_set_copy_params_t *)n->data;
		if (strcmp(cfg_params->ar_set_copy_name,
		    params->ar_set_copy_name) == 0) {

			found = B_TRUE;
			break;
		}
	}


	if (!found) {
		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), params->ar_set_copy_name);

		free_archiver_cfg(cfg);

		Trace(TR_ERR, "setting ar_set_copy_params failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* free the cfgs current params and replace with arg */
	free_ar_set_copy_params(n->data);
	n->data = params;

	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {
		/* leave samerrno as set */

		/* Don't free the users data, set n->data to null */
		n->data = NULL;
		free_archiver_cfg(cfg);

		Trace(TR_ERR, "setting ar_set_copy_params failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* Don'T free the users data, set n->data to null */
	n->data = NULL;
	free_archiver_cfg(cfg);
	Trace(TR_MISC, "set ar_set_copy_params");
	return (0);
}


/*
 * remove all non-default parameter settings for the named ar_set_copy_params
 */
int
reset_ar_set_copy_params(
ctx_t *ctx,		/* contains optional dump path */
const uname_t name)	/* set.copy for which to reset copy params */
{

	archiver_cfg_t *cfg = NULL;

	Trace(TR_MISC, "resetting ar_set_copy_params %s", Str(name));

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "resetting ar_set_copy_params failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (cfg_reset_ar_set_copy_params(cfg, name) != 0) {
		Trace(TR_ERR, "resetting ar_set_copy_params failed: %s",
		    samerrmsg);

		free_archiver_cfg(cfg);
		return (-1);

	}

	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {
		/* leave samerrno as set */
		free_archiver_cfg(cfg);

		Trace(TR_ERR, "resetting ar_set_copy_params failed: %s",
		    samerrmsg);

		return (-1);
	}

	free_archiver_cfg(cfg);
	Trace(TR_MISC, "reset ar_set_copy_params");
	return (0);
}


/*
 * get all the vsn pools.
 */
int
get_all_vsn_pools(
ctx_t *ctx		/* ARGSUSED */,
sqm_lst_t **vsn_pool_list)	/* malloced list of vsn_pool_t */
{

	archiver_cfg_t *cfg = NULL;

	Trace(TR_MISC, "getting all vsn_pools");
	if (ISNULL(vsn_pool_list)) {
		Trace(TR_ERR, "getting all vsn_pools failed: %s", samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "getting all vsn_pools failed: %s", samerrmsg);
		return (-1);
	}

	*vsn_pool_list = cfg->vsn_pools;
	cfg->vsn_pools = NULL;

	free_archiver_cfg(cfg);
	Trace(TR_MISC, "got all vsn_pools");
	return (0);
}


/*
 * get a specific vsn pool.
 */
int
get_vsn_pool(
ctx_t *ctx			/* ARGSUSED */,
const uname_t pool_name,	/* name of vsn_pool to get */
vsn_pool_t **vsn_pool)		/* malloced return value */
{

	/* cfg_get_vsn_pool */
	archiver_cfg_t *cfg = NULL;

	Trace(TR_MISC, "getting vsn_pool %s", Str(pool_name));

	if (ISNULL(vsn_pool, pool_name)) {
		Trace(TR_ERR, "getting vsn_pool failed: %s", samerrmsg);
		return (-1);
	}
	if (read_arch_cfg(ctx, &cfg) != 0) {

		/* leave samerrno as set */
		Trace(TR_ERR, "getting vsn_pool(%s) exit %s", pool_name,
		    samerrmsg);

		return (-1);
	}

	if (cfg_get_vsn_pool(cfg, pool_name, vsn_pool) != 0) {
		/* leave samerrno as set */
		free_archiver_cfg(cfg);

		Trace(TR_ERR, "getting vsn_pool(%s) exit %s", pool_name,
		    samerrmsg);
		return (-1);
	}

	free_archiver_cfg(cfg);
	Trace(TR_MISC, "got vsn_pool %s", Str(pool_name));
	return (0);
}

/*
 * add the vsn pool to the configuration.
 */
int
add_vsn_pool(
ctx_t *ctx,			/* optional dump path */
const vsn_pool_t *vsn_pool)	/* pool to add to config */
{

	vsn_pool_t *tmp_pool;
	archiver_cfg_t *cfg = NULL;

	Trace(TR_MISC, "adding vsn_pool");
	if (ISNULL(vsn_pool)) {
		Trace(TR_ERR, "adding vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "adding vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	if (cfg_get_vsn_pool(cfg, vsn_pool->pool_name, &tmp_pool) == 0) {
		samerrno = SE_POOL_ALREADY_EXISTS;

		/* Vsn pool %s already exists */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_POOL_ALREADY_EXISTS),
		    vsn_pool->pool_name);

		free_archiver_cfg(cfg);
		free_vsn_pool(tmp_pool);

		Trace(TR_ERR, "adding vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	if (dup_vsn_pool(vsn_pool, &tmp_pool) != 0) {
		free_archiver_cfg(cfg);

		Trace(TR_ERR, "adding vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	if (lst_append(cfg->vsn_pools, tmp_pool) != 0) {
		free_archiver_cfg(cfg);
		free_vsn_pool(tmp_pool);
		Trace(TR_ERR, "adding vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {
		/* leave samerrno as set */
		free_archiver_cfg(cfg);

		Trace(TR_ERR, "adding vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	free_archiver_cfg(cfg);

	Trace(TR_MISC, "added vsn_pool %s", vsn_pool->pool_name);
	return (0);
}


/*
 * Modify the vsn pool to match the input.
 * This method is NOT implemented as a remove followed by an add in order
 * to save on reading and writing.
 */
int
modify_vsn_pool(
ctx_t *ctx,			/* contains optional dump_path */
const vsn_pool_t *vsn_pool)	/* pool to modify */
{

	vsn_pool_t *tmp_pool;
	archiver_cfg_t *cfg = NULL;
	boolean_t found = B_FALSE;
	node_t *n;

	Trace(TR_MISC, "modifying vsn_pool");
	if (ISNULL(vsn_pool)) {
		Trace(TR_ERR, "modifying vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "modifying vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	for (n = cfg->vsn_pools->head; n != NULL; n = n->next) {
		if (strcmp(((vsn_pool_t *)n->data)->pool_name,
		    vsn_pool->pool_name) == 0) {

			found = B_TRUE;
			break;
		}
	}

	if (!found) {
		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), vsn_pool->pool_name);

		free_archiver_cfg(cfg);

		Trace(TR_ERR, "modifying vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * replace the pool in the list with a copy of the new pool.
	 * A copy is used because the cfg is going to be freed
	 * and we should not free the struct the user passed in.
	 */
	free_vsn_pool((vsn_pool_t *)n->data);
	if (dup_vsn_pool(vsn_pool, &tmp_pool) != 0) {
		free_archiver_cfg(cfg);
		Trace(TR_ERR, "modifying vsn_pool failed: %s", samerrmsg);
		return (-1);
	}
	n->data = tmp_pool;

	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {
		/* leave samerrno as set */
		free_archiver_cfg(cfg);

		Trace(TR_ERR, "modifying vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	free_archiver_cfg(cfg);

	Trace(TR_MISC, "modified vsn_pool %s", vsn_pool->pool_name);
	return (0);

}


/*
 * remove the named vsn pool
 */
int
remove_vsn_pool(
ctx_t *ctx,			/* contains optional dump path */
const uname_t pool_name)	/* name of pool to remove */
{

	archiver_cfg_t *cfg = NULL;
	boolean_t found = B_FALSE;
	node_t *n;
	uname_t in_use_by;

	Trace(TR_MISC, "removing vsn_pool %s", Str(pool_name));

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "removing vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	/* find the pool */
	for (n = cfg->vsn_pools->head; n != NULL; n = n->next) {
		if (strcmp(((vsn_pool_t *)n->data)->pool_name,
		    pool_name) == 0) {

			found = B_TRUE;
			break;
		}
	}

	if (!found) {
		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), pool_name);

		free_archiver_cfg(cfg);

		Trace(TR_ERR, "removing vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	/* if the pool is in use can't remove it. */
	if (cfg_is_pool_in_use(cfg, pool_name, in_use_by)) {
		samerrno = SE_POOL_IN_USE;

		/* Pool %s is in use by %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_POOL_IN_USE), pool_name, in_use_by);

		free_archiver_cfg(cfg);

		Trace(TR_ERR, "removing vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	free_vsn_pool((vsn_pool_t *)n->data);
	if (lst_remove(cfg->vsn_pools, n) != 0) {
		free_archiver_cfg(cfg);
		Trace(TR_ERR, "removing vsn_pool failed: %s", samerrmsg);
		return (-1);
	}


	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {
		/* leave samerrno as set */
		free_archiver_cfg(cfg);
		Trace(TR_ERR, "removing vsn_pool failed: %s", samerrmsg);
		return (-1);
	}


	free_archiver_cfg(cfg);

	Trace(TR_MISC, "removed vsn_pool %s", Str(pool_name));
	return (0);

}


/*
 * get all vsn and archive set copy associations.
 */
int
get_all_vsn_copy_maps(
ctx_t *ctx		/* ARGSUSED */,
sqm_lst_t **vsn_map_list)	/* malloced list of all vsn_map_t structs */
{

	archiver_cfg_t *cfg = NULL;

	Trace(TR_MISC, "getting vsn_copy_maps");

	if (ISNULL(vsn_map_list)) {
		Trace(TR_ERR, "getting vsn_copy_maps failed: %s", samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "getting vsn_copy_maps failed: %s", samerrmsg);
		return (-1);
	}

	*vsn_map_list = cfg->vsn_maps;
	cfg->vsn_maps = NULL;

	free_archiver_cfg(cfg);
	Trace(TR_MISC, "got vsn_copy_maps");
	return (0);
}


/*
 * get a specific vsn and archive set copy association given the copy name.
 */
int
get_vsn_copy_map(
ctx_t *ctx			/* ARGSUSED */,
const uname_t ar_set_copy_name,	/* set.copy name of map to get */
vsn_map_t **vsn_map)		/* malloced return */
{

	archiver_cfg_t *cfg = NULL;


	Trace(TR_MISC, "getting vsn_copy_map %s", Str(ar_set_copy_name));

	if (ISNULL(vsn_map)) {
		Trace(TR_ERR, "getting vsn_copy_map failed: %s", samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "getting vsn_copy_map failed: %s", samerrmsg);
		return (-1);
	}

	if (cfg_get_vsn_map(cfg, ar_set_copy_name, vsn_map) != 0) {
		/* leave samerrno as set */

		free_archiver_cfg(cfg);
		Trace(TR_ERR, "getting vsn_copy_map failed: %s", samerrmsg);
		return (-1);
	}

	free_archiver_cfg(cfg);
	Trace(TR_MISC, "got vsn_copy_map");
	return (0);

}


/*
 * add the vsn_map to the config.
 */
int
add_vsn_copy_map(
ctx_t *ctx,			/* contains optional dump path */
const vsn_map_t *vsn_map)	/* map to add to config */
{

	vsn_map_t *tmp_map;
	archiver_cfg_t *cfg = NULL;

	Trace(TR_MISC, "adding vsn_copy_map");
	if (ISNULL(vsn_map)) {
		Trace(TR_ERR, "adding vsn_copy_map failed: %s", samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "adding vsn_copy_map failed: %s", samerrmsg);
		return (-1);
	}

	/* make sure there is not one already */
	if (cfg_get_vsn_map(cfg, vsn_map->ar_set_copy_name, &tmp_map) == 0) {
		samerrno = SE_MAP_ALREADY_EXISTS;

		/* A vsn copy map already exists for %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_MAP_ALREADY_EXISTS),
		    vsn_map->ar_set_copy_name);

		free_archiver_cfg(cfg);
		free_vsn_map(tmp_map);

		Trace(TR_ERR, "adding vsn_copy_map failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * make a copy because the cfg will be freed and we should not
	 * free the user input.
	 */
	if (dup_vsn_map(vsn_map, &tmp_map) != 0) {
		Trace(TR_ERR, "adding vsn_copy_map failed: %s", samerrmsg);
		free_archiver_cfg(cfg);
		return (-1);
	}

	if (cfg_insert_copy_map(cfg->vsn_maps, tmp_map) != 0) {
		free_archiver_cfg(cfg);
		/* have to free the map because it is not in the list yet. */
		free_vsn_map(tmp_map);

		Trace(TR_ERR, "adding vsn_copy_map failed: %s", samerrmsg);
		return (-1);

	}

	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {
		/* leave samerrno as set */

		free_archiver_cfg(cfg);
		Trace(TR_ERR, "adding vsn_copy_map failed: %s", samerrmsg);
		return (-1);
	}

	free_archiver_cfg(cfg);

	Trace(TR_MISC, "added vsn_copy_map %s", vsn_map->ar_set_copy_name);
	return (0);
}


/*
 * This method is not implemented as a remove followed by an add in order
 * to save on reading and writing.
 */
int
modify_vsn_copy_map(
ctx_t *ctx,			/* contains the optional dump path */
const vsn_map_t *vsn_map)	/* map to write to config */
{

	vsn_map_t *tmp_map;
	archiver_cfg_t *cfg = NULL;
	boolean_t found = B_FALSE;
	node_t *n;

	Trace(TR_MISC, "modifying vsn_copy_map");
	if (ISNULL(vsn_map)) {
		Trace(TR_ERR, "modifying vsn_copy_map %s", samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "modifying vsn_copy_map %s", samerrmsg);
		return (-1);
	}

	for (n = cfg->vsn_maps->head; n != NULL; n = n->next) {
		if (strcmp(((vsn_map_t *)n->data)->ar_set_copy_name,
		    vsn_map->ar_set_copy_name) == 0) {

			found = B_TRUE;
			break;
		}
	}



	if (!found) {
		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), vsn_map->ar_set_copy_name);

		free_archiver_cfg(cfg);
		Trace(TR_ERR, "modifying vsn_copy_map %s", samerrmsg);
		return (-1);
	}

	/*
	 * replace the map in the list with a copy of the new map.
	 * A copy is used because the cfg is going to be freed
	 * and we should not free the struct the user passed in.
	 */
	free_vsn_map((vsn_map_t *)n->data);

	if (dup_vsn_map(vsn_map, &tmp_map) != 0) {
		free_archiver_cfg(cfg);
		Trace(TR_ERR, "modifying vsn_copy_map %s", samerrmsg);
		return (-1);
	}
	n->data = tmp_map;

	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {
		/* leave samerrno as set */
		free_archiver_cfg(cfg);

		Trace(TR_ERR, "modifying vsn_copy_map %s", samerrmsg);
		return (-1);
	}

	free_archiver_cfg(cfg);

	Trace(TR_MISC, "modified vsn_copy_map %s", vsn_map->ar_set_copy_name);
	return (0);

}


/*
 * remove the copy map
 */
int
remove_vsn_copy_map(
ctx_t *ctx,			/* contains optional dump path */
const uname_t ar_set_copy_name)	/* the name of the copy map to remove */
{

	archiver_cfg_t *cfg = NULL;

	Trace(TR_MISC, "removing vsn_copy_map");

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "removing vsn_copy_map failed: %s", samerrmsg);
		return (-1);
	}

	if (cfg_remove_vsn_copy_map(cfg, ar_set_copy_name) != 0) {
		free_archiver_cfg(cfg);
		Trace(TR_ERR, "removing vsn_copy_map failed: %s", samerrmsg);
		return (-1);
	}


	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {
		/* leave samerrno as set */
		free_archiver_cfg(cfg);
		Trace(TR_ERR, "removing vsn_copy_map failed: %s", samerrmsg);
		return (-1);
	}

	free_archiver_cfg(cfg);

	Trace(TR_MISC, "removed vsn_copy_map %s", ar_set_copy_name);
	return (0);

}


/*
 * get a ar_set_copy_cfg with the default settings.
 */
int
get_default_ar_set_copy_cfg(
ctx_t *ctx			/* ARGSUSED */,
ar_set_copy_cfg_t **copy_cfg)	/* malloced return value */
{

	Trace(TR_MISC, "getting default ar_set_copy_cfg");

	if (ISNULL(copy_cfg)) {
		Trace(TR_ERR, "getting default ar_set_copy_cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (get_default_copy_cfg(copy_cfg) != 0) {
		Trace(TR_ERR, "getting default ar_set_copy_cfg failed: %s",
		    samerrmsg);

		return (-1);
	}

	Trace(TR_MISC, "got default ar_set_copy_cfg");
	return (0);
}


/*
 * get a default_ar_set_criteria with the default settings.
 */
int
get_default_ar_set_criteria(
ctx_t *ctx			/* ARGSUSED */,
ar_set_criteria_t **criteria)	/* malloced return value */
{

	Trace(TR_MISC, "getting default ar_set_criteria");
	if (ISNULL(criteria)) {
		Trace(TR_ERR, "getting default ar_set_criteria failed: %s",
		    samerrmsg);
		return (-1);
	}

	*criteria = (ar_set_criteria_t *)mallocer(sizeof (ar_set_criteria_t));
	if (*criteria == NULL) {
		Trace(TR_ERR, "getting default ar_set_criteria failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (init_criteria(*criteria, "no-name") != 0) {
		Trace(TR_ERR, "getting default ar_set_criteria failed: %s",
		    samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "got default ar_set_criteria");
	return (0);
}


/*
 * get an ar_global_directive with default settings.
 */
int
get_default_ar_global_directive(
ctx_t *ctx				/* ARGSUSED */,
ar_global_directive_t **ar_global)	/* malloced return value */
{

	sqm_lst_t *lib_list = NULL;

	Trace(TR_MISC, "getting default ar_global_directive");
	if (ISNULL(ar_global)) {
		Trace(TR_ERR, "getting default ar_global_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	*ar_global = (ar_global_directive_t *)mallocer(
	    sizeof (ar_global_directive_t));

	if (*ar_global == NULL) {
		Trace(TR_ERR, "getting default ar_global_directive failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (get_all_libraries(NULL, &lib_list) == -1) {
		free(*ar_global);
		Trace(TR_ERR, "getting default ar_global_directive failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (init_global_dirs(*ar_global, lib_list) != 0) {
		free_ar_global_directive(*ar_global);
		free_list_of_libraries(lib_list);
		*ar_global = NULL;
		Trace(TR_ERR, "getting default ar_global_directive failed: %s",
		    samerrmsg);

		return (-1);
	}
	free_list_of_libraries(lib_list);

	Trace(TR_MISC, "got default ar_global_directive");
	return (0);
}


/*
 * get an ar_fs_directive with default settings.
 */
int
get_default_ar_fs_directive(
ctx_t *ctx				/* ARGSUSED */,
ar_fs_directive_t **ar_fs_directive)	/* malloced return value */
{

	Trace(TR_MISC, "getting default ar_fs_directive");
	if (ISNULL(ar_fs_directive)) {
		Trace(TR_ERR, "getting default ar_fs_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	*ar_fs_directive = (ar_fs_directive_t *)mallocer(
	    sizeof (ar_fs_directive_t));

	if (ar_fs_directive == NULL) {
		Trace(TR_ERR, "getting default ar_fs_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (init_ar_fs_directive(*ar_fs_directive) != 0) {

		free_ar_fs_directive(*ar_fs_directive);
		*ar_fs_directive = NULL;

		Trace(TR_ERR, "getting default ar_fs_directive failed: %s",
		    samerrmsg);

		return (-1);
	}


	Trace(TR_MISC, "got default ar_fs_directive");
	return (0);
}



/*
 * Depending on media type TPARCHMAX or ODARCHMAX would be used to set
 * default_ar_set_copy_params arch max field.
 */
int
get_default_ar_set_copy_params(
ctx_t *ctx				/* ARGSUSED */,
ar_set_copy_params_t **parameters)	/* malloced return value */
{

	Trace(TR_MISC, "getting default ar_set_copy_params");

	if (ISNULL(parameters)) {
		Trace(TR_ERR, "getting default ar_set_copy_params failed: %s",
		    samerrmsg);
		return (-1);
	}

	*parameters = (ar_set_copy_params_t *)mallocer(
	    sizeof (ar_set_copy_params_t));

	if (*parameters == NULL) {
		Trace(TR_ERR, "getting default ar_set_copy_params failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (init_ar_copy_params(*parameters) != 0) {
		free_ar_set_copy_params(*parameters);
		*parameters = NULL;

		Trace(TR_ERR, "getting default ar_set_copy_params failed: %s",
		    samerrmsg);

		return (-1);
	}

	Trace(TR_MISC, "got default ar_set_copy_params");

	return (0);
}


/*
 * The default drive count is different depending on whether the drive
 * directive is in the global or not.
 */
int
get_default_ar_drive_directive(
ctx_t *ctx		/* ARGSUSED */,
uname_t lib_name,	/* name of library for which to get defaults */
boolean_t global,	/* if true get defaults for global value */
drive_directive_t **drive)	/* malloced return value */
{

	library_t *lib;
	Trace(TR_MISC, "getting default ar_drive_directive");

	if (ISNULL(drive)) {
		Trace(TR_ERR, "getting default ar_drive_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* make sure the library exists. */
	if (get_library_by_family_set(NULL, lib_name, &lib) != 0) {
		Trace(TR_ERR, "getting default ar_drive_directive failed: %s",
		    samerrmsg);

		return (-1);
	}

	*drive = (drive_directive_t *)mallocer(sizeof (drive_directive_t));
	if (*drive == NULL) {
		Trace(TR_ERR, "getting default ar_drive_directive failed: %s",
		    samerrmsg);

		free_library(lib);
		return (-1);
	}


	memset(*drive, 0, sizeof (drive_directive_t));
	strcpy((*drive)->auto_lib, lib_name);
	if (B_FALSE == global) {
		(*drive)->count = 1;
	} else {
		/* all drives is the default for global. */
		(*drive)->count = lib->no_of_drives;
	}

	free_library(lib);
	Trace(TR_MISC, "got default ar_drive_directive");
	return (0);
}


/*
 * returns a list of the names of all sets for which copy parameters can be
 * defined. The returned list includes the name of each file system and
 * the pseudo set ALLSETS. The performance of this method in terms of time
 * is identical to get_all_ar_set_copy_params.
 */
int
get_ar_set_copy_params_names(
ctx_t *ctx			/* ARGSUSED */,
sqm_lst_t **ar_set_copy_names)	/* malloced list of all copy params names */
{

	archiver_cfg_t *cfg = NULL;
	node_t *o, *in;
	sqm_lst_t *l;

	Trace(TR_MISC, "getting ar_set_copy_params names");

	if (ISNULL(ar_set_copy_names)) {
		Trace(TR_ERR, "getting ar_set_copy_params names failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "getting ar_set_copy_params names failed: %s",
		    samerrmsg);
		return (-1);
	}


	l = lst_create();
	if (l == NULL) {
		free_archiver_cfg(cfg);

		Trace(TR_ERR, "getting ar_set_copy_params names failed: %s",
		    samerrmsg);

		return (-1);
	}

	for (o = cfg->archcopy_params->head; o != NULL; o = o->next) {
		uname_t		name;
		int		dec = 0;
		boolean_t	found = B_FALSE;

		dec =  strcspn((char *)o->data, ".");
		strlcpy(name,
		    ((ar_set_copy_params_t *)o->data)->ar_set_copy_name, dec);

		name[dec] = '\0';

		/* if inner already contains it don't reinsert. */
		for (in = l->head; in != NULL; in = in->next) {
			if (strcmp(in->data, name) == 0) {
				found = B_TRUE;
				break;
			}
		}

		if (!found) {
			char *tmp;
			tmp = (char *)mallocer(sizeof (uname_t));
			if (tmp == NULL) {
				free_archiver_cfg(cfg);
				lst_free_deep(l);

				Trace(TR_ERR, "%s failed: %s",
				    "getting ar_set_copy_params names",
				    samerrmsg);
				return (-1);
			}

			strcpy(tmp, name);
			if (lst_append(l, tmp) != 0) {
				free_archiver_cfg(cfg);
				lst_free_deep(l);

				Trace(TR_ERR, "%s failed: %s",
				    "getting ar_set_copy_params names",
				    samerrmsg);
				return (-1);
			}
		}
	}

	*ar_set_copy_names = l;

	free_archiver_cfg(cfg);
	Trace(TR_MISC, "got ar_set_copy_params names");
	return (0);
}


/*
 * returns a list of all ar_set_criterias currently in the config.
 * This might include no archive criteria.
 */
int
get_ar_set_criteria_names(
ctx_t *ctx			/* ARGSUSED */,
sqm_lst_t **ar_set_crit_names)	/* malloced list of all ar_set_criteria */
{

	archiver_cfg_t *cfg = NULL;
	node_t *o, *in;
	node_t *f;
	sqm_lst_t *l;
	uname_t	name;
	boolean_t found = B_FALSE;
	ar_set_criteria_t *cur_crit;

	Trace(TR_MISC, "getting ar_set_criteria names");

	if (ISNULL(ar_set_crit_names)) {
		Trace(TR_ERR, "getting ar_set_criteria names failed: %s",
		    samerrmsg);
		return (-1);
	}


	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */
		Trace(TR_ERR, "getting ar_set_criteria names failed: %s",
		    samerrmsg);
		return (-1);
	}


	l = lst_create();
	if (l == NULL) {
		free_archiver_cfg(cfg);
		Trace(TR_ERR, "getting ar_set_criteria names failed: %s",
		    samerrmsg);

		return (-1);
	}


	for (o = cfg->global_dirs.ar_set_lst->head; o != NULL; o = o->next) {

		found = B_FALSE;
		strcpy(name, ((ar_set_criteria_t *)o->data)->set_name);


		/* if inner already contains it don't reinsert. */
		for (in = l->head; in != NULL; in = in->next) {
			if (strcmp(((ar_set_criteria_t *)in->data)->set_name,
			    name) == 0) {

				found = B_TRUE;
				break;
			}
		}

		if (!found) {
			char *tmp;
			tmp = (char *)mallocer(sizeof (uname_t));
			if (tmp == NULL) {
				free_archiver_cfg(cfg);
				lst_free_deep(l);

				Trace(TR_ERR, "%s failed: %s",
				    "getting ar_set_criteria names",
				    samerrmsg);
				return (-1);
			}

			strcpy(tmp, name);
			if (lst_append(l, tmp) != 0) {
				free_archiver_cfg(cfg);
				lst_free_deep(l);

				Trace(TR_ERR, "%s failed: %s",
				    "getting ar_set_criteria names",
				    samerrmsg);
				return (-1);
			}
		}
	}


	/*
	 * for each ar_fs_directive consider its sets if names are
	 * not duplicates insert them.
	 */
	for (f = cfg->ar_fs_p->head; f != NULL; f = f->next) {

		ar_fs_directive_t *fs = (ar_fs_directive_t *)f->data;
		for (o = fs->ar_set_criteria->head; o != NULL; o = o->next) {
			found = B_FALSE;
			strcpy(name, ((ar_set_criteria_t *)o->data)->set_name);


			/* if inner already contains it don't reinsert. */
			for (in = l->head; in != NULL; in = in->next) {
				cur_crit = (ar_set_criteria_t *)in->data;
				if (strcmp(cur_crit->set_name, name) == 0) {

					found = B_TRUE;
					break;
				}
			}

			if (!found) {
				char *tmp;
				tmp = (char *)mallocer(sizeof (uname_t));
				if (tmp == NULL) {
					free_archiver_cfg(cfg);
					lst_free_deep(l);

					Trace(TR_ERR, "%s failed: %s",
					    "get ar_set_criteria names",
					    samerrmsg);
					return (-1);
				}

				strcpy(tmp, name);
				if (lst_append(l, tmp) != 0) {
					free_archiver_cfg(cfg);
					lst_free_deep(l);

					Trace(TR_ERR, "%s failed: %s",
					    "getting ar_set_criteria names",
					    samerrmsg);
					return (-1);
				}
			}
		}
	}


	*ar_set_crit_names = l;

	free_archiver_cfg(cfg);
	Trace(TR_MISC, "got ar_set_criteria names");
	return (0);
}


/*
 * activate_archiver_config
 *
 * must be called after changes are made ato the archiver configuration. It
 * first checks the configuration to make sure it is valid. If the
 * configuration is valid it is made active by signaling sam-fsd.
 *
 * If any configuration errors are encountered, err_warn_list is populated
 * by strings that describe the errors, sam-fsd is not signaled
 * (the configuration will not become active), samerrno is set and a -2
 * is returned.
 *
 * In addition to encountering errors. It is possible that warnings will be
 * encountered. Warnings arise when the configuration will not prevent the
 * archiver from running but the configuration is suspect.
 * If warnings are encountered, err_warn_list will be populated
 * with strings describing the warnings, the daemon will be signaled, and -3
 * will be returned. A common case of a warning is that no volumes are
 * available for an archive set.
 *
 * If this function fails to execute properly a -1 is returned. This indicates
 * that there was an error during execution. In this case err_warn_list will
 * not need to be freed by the caller. The configuration has not been
 * activated.
 *
 * returns
 * 0  indicates successful execution, no errors or warnings encountered.
 * -1 indicates an internal error.
 * -2 indicates errors encountered in config.
 * -3 indicates warnings encountered in config.
 *
 */
int
activate_archiver_cfg(
ctx_t *ctx,
sqm_lst_t **err_warn_list)
{

	int status;

	status = chk_arch_cfg(ctx, err_warn_list);
	if (status == -1) {

		/*
		 * Indicates an error executing the check function.
		 * Something bad is going on so return an error.
		 */
		goto err;
	} else if (status != -2) {
		int err;
		int saveerrno = samerrno;
		char savemsg[MAX_MSG_LEN];

		/*
		 * Status != -2 indicates either complete success or
		 * warnings in the archiver config. In all cases but
		 * one SE_NO_FS_NO_ARCHIVING, this means the archiver
		 * configuration should be activated.
		 */

		Trace(TR_MISC, "chk_arch_cfg returned %d: error %d, %s",
		    status, samerrno, samerrmsg);

		if (status == -3 && samerrno == SE_NO_FS_NO_ARCHIVING) {
			return (0);
		}

		strlcpy(savemsg, samerrmsg, MAX_MSG_LEN);

		/* only init the config if no errors were found */
		err = ar_rerun_all(NULL);
		if (err != 0 && samerrno != SE_CONN_REFUSED) {
			/*
			 * SE_CONN_REFUSED means sam-archiverd is not
			 * running. In that case no need for an error. In all
			 * other cases return an error.
			 */
			samerrno = SE_ACTIVATE_ARCHIVER_CFG_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_ACTIVATE_ARCHIVER_CFG_FAILED));
			Trace(TR_ERR, "arrerun failed %s", samerrmsg);
			return (-1);
		} else {

			/*
			 * Connection was refused by the
			 * sam-archiverd. This will occur if it is not
			 * running. We don't want to consider this an
			 * error. So, reset the errno and msg to the message
			 * from chk_arch_cfg
			 */
			samerrno = saveerrno;
			strlcpy(samerrmsg, savemsg, MAX_MSG_LEN);
		}


	}

	return (status);

err:
	Trace(TR_ERR, "activating archiver cfg failed %s", samerrmsg);
	return (-1);
}


/*
 * returns true if the pool named is in use. If the pool is in use the function
 * also returns a string containing the name of an archive copy that uses
 * the pool. There could be many that use it but only one gets returned.
 */
int
is_pool_in_use(
ctx_t *ctx,
const uname_t pool_name,
boolean_t *in_use,
uname_t used_by) /* OUT parameter */
{


	archiver_cfg_t *cfg;

	if (ISNULL(pool_name, in_use, used_by)) {
		Trace(TR_ERR, "getting global directives failed: %s",
		    samerrmsg);
		return (-1);
	}

	*used_by = '\0';

	/* just read it copy and free the cfg and return */
	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "getting global directives failed: %s",
		    samerrmsg);

		return (-1);
	}

	*in_use = cfg_is_pool_in_use(cfg, pool_name, used_by);

	free_archiver_cfg(cfg);

	return (0);
}


/*
 * if the named group is a valid unix group, is_valid will be set to true.
 * if any errors are encountered -1 is returned.
 */
int
is_valid_group(
ctx_t *ctx	/* ARGSUSED */,
uname_t group,
boolean_t *is_valid)
{

	char err_buf[80];

	if (ISNULL(group, is_valid)) {
		/* leave samerrno as set */
		return (-1);
	}


	errno = 0;
	if (getgrnam(group) == NULL) {
		if (errno != 0) {
			samerrno = SE_CHECK_GROUP_FAILED;
			StrFromErrno(errno, err_buf, sizeof (err_buf));
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CHECK_GROUP_FAILED), err_buf);
			return (-1);
		}
		*is_valid = B_FALSE;
	} else {
		*is_valid = B_TRUE;
	}
	return (0);
}


int
is_valid_user(
ctx_t *ctx	/* ARGSUSED */,
char *user,
boolean_t *is_valid)
{


	if (ISNULL(user, is_valid)) {
		/* leave samerrno as set */
		return (-1);
	}

	if (getpwnam(user) == NULL) {
		*is_valid = B_FALSE;
		return (0);
	}

	*is_valid = B_TRUE;
	return (0);
}



/*
 * check_arch_cfg
 *
 * can be called after changes are made to the archiver configuration. It
 * checks the configuration to make sure it is valid.
 *
 * If any configuration errors are encountered, err_warn_list is populated
 * by strings that describe the errors, sam-fsd is not signaled
 * (the configuration will not become active), samerrno is set and a -2
 * is returned.
 *
 * In addition to encountering errors. It is possible that warnings will be
 * encountered. Warnings arise when the configuration will not prevent the
 * archiver from running but the configuration is suspect.
 * If warnings are encountered, err_warn_list will be populated
 * with strings describing the warnings and -3
 * will be returned. A common case of a warning is that no volumes are
 * available for an archive set.
 *
 * If this function fails to execute properly a -1 is returned. This indicates
 * that there was an error during execution. In this case err_warn_list will
 * not need to be freed by the caller. The configuration should not be
 * activated
 *
 * returns
 * 0  indicates successful execution, no errors or warnings encountered.
 * -1 indicates an internal error.
 * -2 indicates errors encountered in config.
 * -3 indicates warnings encountered in config.
 *
 */
int
chk_arch_cfg(
ctx_t *ctx,
sqm_lst_t **err_warn_list)
{

	FILE *res_stream = NULL;
	FILE *err_stream = NULL;
	upath_t cmd;
	char line[MAX_LINE];
	pid_t procpid, tmppid;
	int status;
	char *end;
	upath_t copy_name;
	char *read_path;
	boolean_t wait_done = B_FALSE;
	boolean_t syntax_errors = B_FALSE;
	boolean_t no_file_systems = B_FALSE;
	/* file descriptors so we can make them non-blocking. */
	int res_fd, err_fd;

	/*
	 * variables to handle possible partial line reads due to the
	 * non-blocking calls to fgets
	 */
	boolean_t output_partial = B_FALSE;
	boolean_t error_partial = B_FALSE;
	char part_line[MAX_LINE] = "";
	char part_err[MAX_LINE] = "";
	int chars = 0;
	boolean_t handling_pools = B_FALSE;
	boolean_t handling_sets = B_FALSE;
	timespec_t	waitspec = {1, 0};

	Trace(TR_MISC, "checking archiver cfg");
	if (ISNULL(err_warn_list)) {
		Trace(TR_ERR, "checking archiver cfg failed: %s", samerrmsg);
		return (-1);
	}

	*err_warn_list = lst_create();
	if (err_warn_list == NULL) {
		Trace(TR_ERR, "checking archiver cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (ctx != NULL && *ctx->read_location != '\0') {
		read_path = assemble_full_path(ctx->read_location,
		    ARCH_DUMP_FILE, B_FALSE, NULL);

		snprintf(cmd, sizeof (cmd), ARCHIVER_DASH_C, read_path);
	} else {
		snprintf(cmd, sizeof (cmd), ARCHIVER_LV);
	}

	/*
	 * Sleep for 1 second to give catserverd a chance to come up
	 * after samd start. Otherwise warnings can be incorrectly
	 * generated
	 */
	nanosleep(&waitspec, NULL);
	procpid = exec_get_output(cmd, &res_stream, &err_stream);

	if (procpid < 0) {
		/* leave samerrno as set */
		return (-1);
	}


	/*
	 * The output from the archiver command can be very long.
	 * Full buffers can then lead to it never finishing. To get
	 * around this get the file descriptors for the two streams
	 * and set them to non-blocking.
	 */

	res_fd = fileno(res_stream);
	err_fd = fileno(err_stream);
	fcntl(res_fd, F_SETFL, O_NONBLOCK);
	fcntl(err_fd, F_SETFL, O_NONBLOCK);

	if ((tmppid = waitpid(procpid, &status, WNOHANG)) < 0) {
		wait_done = B_TRUE;
		goto err;
	} else if (tmppid != 0) {
		wait_done = B_TRUE;
	}



	for (;;) {
		if (!wait_done) {
			if ((tmppid = waitpid(procpid, &status,
			    WNOHANG)) < 0) {

				goto err;
			} else if (tmppid != 0) {
				wait_done = B_TRUE;
			}

		}


		/*
		 * record parsing errors/warnings. Partial reads from
		 * the error stream can be treated completely separately from
		 * the partial reads of the res stream
		 */
		while (NULL != fgets(line, sizeof (line), err_stream)) {

			/* error/warning */
			end = strrchr(line, '\n');
			if (end != NULL) {
				*end = '\0';
			} else {

				error_partial = B_TRUE;
				chars = strlcat(part_err, line,
				    sizeof (line));

				if (chars > sizeof (line)) {
					line[sizeof (line) - 1] = '\0';
				}
				continue;
			}
			/*
			 * We had a partial but now recieved
			 * the rest of the line. Append
			 * this to the partial and copy the whole
			 * thing to the line buffer and reset the
			 * partial flags and char array.
			 */
			if (error_partial) {
				chars = strlcat(part_err, line,
				    sizeof (line));

				if (chars > sizeof (line)) {
					line[sizeof (line) - 1] = '\0';
				}
				strlcpy(line, part_err, sizeof (line));

				error_partial = B_FALSE;
				*part_err = '\0';
			}


			/*
			 * if any syntax errors have been detected, do not
			 * add any errors to the list.
			 */
			if (syntax_errors) {
				continue;
			}

			/*
			 * instead of excluding based on a bunch of things,
			 * determine what line to include. Only lines that say
			 * "setF.x has no volumes defined"
			 * should have their first word included.
			 */
			end = strchr(line, ' ');
			if (end != NULL) {
				if (strstr(end, GetCustMsg(31567)) != NULL) {
					*end = '\0';

					/*
					 * set names can not start with a digit
					 * and there is a line that will match
					 * the CustMsg 31567 that does start
					 * with a number
					 */
					if (isdigit(line[0])) {
						continue;
					}
				} else if (strstr(end, GetCustMsg(31570))
				    != NULL) {

					syntax_errors = B_TRUE;
					continue;

				} else if (strstr(line,
				    GetCustMsg(4006)) != NULL) {

					/*
					 * "No file systems found. No
					 * archiving will be performed"
					 */
					no_file_systems = B_TRUE;
					continue;

				} else {
					continue;
				}
			}
			Trace(TR_ERR, "Warning detected in archiver.cmd %s",
			    line);
			if (lst_append(*err_warn_list, strdup(line)) != 0) {
				goto err;
			}
		}


		/*
		 * Loop assembling partial lines.
		 * When a whole line is available, process it.
		 */
		while (NULL != fgets(line, sizeof (line), res_stream)) {
			if (strrchr(line, '\n') == NULL) {
				int chars = 0;
				/*
				 * didn't get the full line so need to keep
				 * the partial and continue to try to
				 * read the rest of this line.
				 */
				output_partial = B_TRUE;

				chars = strlcat(part_line, line,
				    sizeof (line));

				if (chars > sizeof (line)) {
					line[sizeof (line) - 1] = '\0';
				}
				continue;

			} else if (output_partial) {

				/*
				 * We had a partial but now recieved
				 * the rest of the line. Append this to
				 * the partial and copy the whole thing to
				 * the line buffer and reset the
				 * partial flags and char array.
				 */
				chars = strlcat(part_line, line,
				    sizeof (line));

				if (chars > sizeof (line)) {
					line[sizeof (line) - 1] = '\0';
				}
				strlcpy(line, part_line, sizeof (line));

				output_partial = B_FALSE;
				*part_line = '\0';

			}

			/* Whole line has been read so now process it. */
			if (handling_pools) {

				/*
				 * VSN Pools section ends at an
				 * empty line
				 */
				if (*line == '\n') {
					*copy_name = '\0';
					handling_pools = B_FALSE;
					continue;
				}

				/*
				 * if the line contains the string
				 * 'No volumes available' add the name
				 * (first word) to the list of warnings
				 */
				if (strstr(line, GetCustMsg(4333))) {
					/*
					 * First word on line is the
					 * pool name. Keep only that
					 */
					end = strchr(line, ' ');
					if (end != NULL) {
						*end = '\0';
					}

					if (lst_append(*err_warn_list,
					    strdup(line)) != 0) {
						goto err;
					}
				}

			} else if (handling_sets) {
				/* sets are separated by an empty line? */
				if (*line == '\0') {
					*copy_name = '\0';
					continue;
				}

				/* get the name of the copy */
				if (*line != ' ') {
					end = strrchr(line, '\n');
					if (end != NULL) {
						*end = '\0';
					}
					strlcpy(copy_name, line,
					    sizeof (copy_name));
					continue;
				}

				/*
				 * if the copy has no media available
				 * append its name to the list
				 */
				if (strstr(line, GetCustMsg(4333))) {
					if (lst_append(*err_warn_list,
					    strdup(copy_name)) != 0) {
						goto err;
					}
				}


			} else if (strstr(line, GetCustMsg(4629)) != NULL) {

				handling_pools = B_TRUE;

			} else if (strstr(line, GetCustMsg(4618)) != NULL) {

				/*
				 * Ready to get information about sets
				 * that have no available media
				 * because we have encountered the
				 * line that reads
				 * 'Archive Sets:'
				 * Format from here will be:
				 * setname.1
				 * media: type
				 * ** No volumes available. **
				 *
				 * actual sets will be separated by an
				 * empty line.
				 */
				handling_sets = B_TRUE;
			}

		}


		/*
		 * decide wether to break. If wait was finished at the
		 * loop entry then we have read everything from the streams.
		 */
		if (wait_done) {
			break;
		} else {
			sleep(1);
		}
	}


	fclose(res_stream);
	fclose(err_stream);


	/* check exit status to see if errors were encountered. */
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status)) { /* non zero exit code */

			if (no_file_systems) {
				samerrno = SE_NO_FS_NO_ARCHIVING;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_NO_FS_NO_ARCHIVING));
				return (-3);
			}
			samerrno = SE_ARCHIVER_CMD_CONTAINED_ERRORS;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_ARCHIVER_CMD_CONTAINED_ERRORS));

			Trace(TR_ERR, "checking archiver cfg failed %d %s",
			    -2, samerrmsg);

			return (-2);
		}

		/* normal exit with no error- so continue. */

	} else {
		samerrno = SE_ARCHIVER_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ARCHIVER_FAILED));
		goto err;
	}



	if ((*err_warn_list)->length == 0) {
		Trace(TR_MISC, "checked archiver cfg: OK");
		return (0);
	} else {
		/*
		 * This indicates the presence of warnings. We know we
		 * have warnings because archiver exited zero but printed
		 * out messages on stderr.
		 */
		samerrno = SE_ARCHIVER_CMD_CONTAINED_WARNINGS;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ARCHIVER_CMD_CONTAINED_WARNINGS));

		Trace(TR_ERR, "checked archiver cfg: warnings detected");
		return (-3);
	}



err:
	Trace(TR_ERR, "checking archiver cfg failed %s", samerrmsg);
	fclose(res_stream);
	fclose(err_stream);
	return (-1);
}


int
archive_files(
ctx_t	*c /* ARGSUSED */,
sqm_lst_t	*files,
int32_t options,
char	**job_id)
{

	char		buf[35];
	char		**command;
	node_t		*n;
	argbuf_t 	*arg;
	size_t		len = MAXPATHLEN * 2;
	boolean_t	found_one = B_FALSE;
	pid_t		pid;
	int		ret;
	int		status;
	FILE		*out;
	FILE		*err;
	exec_cleanup_info_t *cl;
	int arg_cnt;
	int cur_arg = 0;

	if (ISNULL(files, job_id)) {
		Trace(TR_ERR, "archive files failed:%s", samerrmsg);
		return (-1);
	}
	Trace(TR_MISC, "archive files: %d files options %d",
	    files->length, options);

	/*
	 * Determine how many args to the command and create the command
	 * array. Note that command is malloced because the number of files
	 * is not known prior to execution. The arguments themselves need
	 * not be malloced because the child process will get a copy.
	 * Include space in the command array for:
	 * - the command
	 * - all possible options
	 * - an entry for each file in the list
	 * - an entry for the NULL
	 */
	arg_cnt = 1 + 9 + files->length + 1;

	command = (char **)calloc(arg_cnt, sizeof (char *));
	if (command == NULL) {
		setsamerr(SE_NO_MEM);
		Trace(TR_ERR, "archive files failed:%s", samerrmsg);
		return (-1);
	}
	command[cur_arg++] = ARCHIVE_FILES_CMD;
	/* Multiple copies can be specified */
	if (options & AR_OPT_COPY_1) {
		command[cur_arg++] = "-c1";
	}
	if (options & AR_OPT_COPY_2) {
		command[cur_arg++] = "-c2";
	}
	if (options & AR_OPT_COPY_3) {
		command[cur_arg++] = "-c3";
	}
	if (options & AR_OPT_COPY_4) {
		command[cur_arg++] = "-c4";
	}
	if (options & AR_OPT_DEFAULTS) {
		command[cur_arg++] = "-d";
	}
	if (options & AR_OPT_NEVER) {
		command[cur_arg++] = "-n";
	}
	if (options & AR_OPT_CONCURRENT) {
		command[cur_arg++] = "-C";
	}
	if (options & AR_OPT_INCONSISTENT) {
		command[cur_arg++] = "-I";
	}
	/* Recursive must be specified last */
	if (options & AR_OPT_RECURSIVE) {
		command[cur_arg++] = "-r";
	}


	/* make the argument buffer for the activity */
	arg = (argbuf_t *)mallocer(sizeof (archivebuf_t));
	if (arg == NULL) {
		free(command);
		Trace(TR_ERR, "archive files failed:%s", samerrmsg);
		return (-1);
	}

	arg->a.filepaths = lst_create();
	if (arg->a.filepaths == NULL) {
		free(command);
		Trace(TR_ERR, "archive files failed:%s", samerrmsg);
		return (-1);
	}
	arg->a.options = options;

	/* add the file names to the command and the argument buffer */
	for (n = files->head; n != NULL; n = n->next) {
		if (n->data != NULL) {
			char *cur_file;
			command[cur_arg++] = (char *)n->data;

			found_one = B_TRUE;
			cur_file = copystr(n->data);
			if (cur_file == NULL) {
				free(command);
				free_argbuf(SAMA_ARCHIVEFILES, arg);
				Trace(TR_ERR, "archive files failed:%s",
				    samerrmsg);
				return (-1);
			}
			if (lst_append(arg->a.filepaths, n->data) != 0) {
				free(cur_file);
				free(command);
				free_argbuf(SAMA_ARCHIVEFILES, arg);
				Trace(TR_ERR, "archive files failed:%s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	/*
	 * Check that at least one file was found.
	 */
	if (!found_one) {
		free(command);
		free_argbuf(SAMA_ARCHIVEFILES, arg);
		Trace(TR_ERR, "archive files failed:%s", samerrmsg);
		return (-1);
	}

	/* create the activity */
	ret = start_activity(display_archive_activity, kill_fork,
	    SAMA_ARCHIVEFILES, arg, job_id);
	if (ret != 0) {
		free(command);
		free_argbuf(SAMA_ARCHIVEFILES, arg);
		Trace(TR_ERR, "archive files failed:%s", samerrmsg);
		return (-1);
	}

	cl = (exec_cleanup_info_t *)mallocer(sizeof (exec_cleanup_info_t));
	if (cl == NULL) {
		free(command);
		lst_free_deep(arg->a.filepaths);

		/* end_this_activity frees the main arg struct */
		end_this_activity(*job_id);
		Trace(TR_ERR, "archive files failed, error:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}


	/* exec the process */
	pid = exec_mgmt_cmd(&out, &err, command);
	if (pid < 0) {
		free(command);
		lst_free_deep(arg->a.filepaths);

		/* end_this_activity frees the main arg struct */
		end_this_activity(*job_id);
		Trace(TR_ERR, "archive files failed, error:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	set_pid_or_tid(*job_id, pid, 0);
	free(command);

	/* setup struct for call to cleanup */
	strlcpy(cl->func, ARCHIVE_FILES_CMD, sizeof (cl->func));
	cl->pid = pid;
	strlcpy(cl->job_id, *job_id, MAXNAMELEN);
	cl->streams[0] = out;
	cl->streams[1] = err;


	/*
	 * possibly return the results or async
	 * notification. bounded_activity_waitpid ends the activity
	 * and frees cl
	 */
	ret = bounded_activity_wait(&status, 10, *job_id, pid, cl,
	    cleanup_after_exec_get_output);

	if (ret == -1) {
		samerrno = SE_ARCHIVE_FILES_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ARCHIVE_FILES_FAILED));

		free(job_id);
		*job_id = NULL;
		Trace(TR_ERR, "archive files failed, error:%d %s",
		    samerrno, samerrmsg);

		return (-1);
	} else if (ret == 0) {
		/*
		 * A return of zero from bounded_activity_wait means
		 * that the job finished. So clear the job id now
		 * so that the caller knows the request has been submitted.
		 */
		free(job_id);
		*job_id = NULL;
		Trace(TR_MISC, "archive files completed");
	}
	Trace(TR_MISC, "leaving archive files");
	return (0);
}


static int
expand_archive_options(int32_t options, char *buf, int len) {

	*buf = '\0';
	if (options & AR_OPT_COPY_1) {
		strlcat(buf, "-c 1 ", len);
	}

	if (options & AR_OPT_COPY_2) {
		strlcat(buf, "-c 2 ", len);
	}

	if (options & AR_OPT_COPY_3) {
		strlcat(buf, "-c 3 ", len);
	}

	if (options & AR_OPT_COPY_4) {
		strlcat(buf, "-c 4 ", len);
	}
	if (options & AR_OPT_DEFAULTS) {
		strlcat(buf, "-d ", len);
	}
	if (options & AR_OPT_NEVER) {
		strlcat(buf, "-n ", len);
	}
	if (options & AR_OPT_CONCURRENT) {
		strlcat(buf, "-C ", len);
	}
	if (options & AR_OPT_INCONSISTENT) {
		strlcat(buf, "-I ", len);
	}
	/* Recursive must be specified last */
	if (options & AR_OPT_RECURSIVE) {
		strlcat(buf, "-r ", len);
	}

	return (0);
}

int
display_archive_activity(samrthread_t *ptr, char **result) {
	char buf[2 * MAXPATHLEN];
	char details[32];

	expand_archive_options(ptr->args->a.options,
		details, sizeof (details));

	snprintf(buf, sizeof (buf), "activityid=%s,starttime=%ld"
	    ",details=\"%s\",type=%s,pid=%d",
	    ptr->jobid, ptr->start, details, activitytypes[ptr->type],
	    ptr->pid);
	*result = copystr(buf);
	return (0);
}
