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
#pragma ident   "$Revision: 1.27 $"

static char *_SrcFile = __FILE__;


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mgmt/config/archiver.h"
#include "pub/mgmt/archive_sets.h"
#include "pub/mgmt/sqm_list.h"
#include "mgmt/hash.h"

#include "pub/mgmt/types.h"
#include "pub/mgmt/error.h"
#include "sam/sam_trace.h"
#include "mgmt/util.h"

static int populate_hashtables(archiver_cfg_t *cfg, hashtable_t **criteria,
	hashtable_t **maps, hashtable_t **params, boolean_t disconnect);

static int build_arch_set(char *name, sqm_lst_t *crit, set_type_t set_type,
	hashtable_t *param_tbl, hashtable_t *vsn_map_tbl, arch_set_t **set);

static int build_default_set_criteria(ar_fs_directive_t *fs, sqm_lst_t **l);

static int set_fs_copies(ar_fs_directive_t *fs, arch_set_t *set);

static int reset_maps_and_params(hashtable_t *params,
	hashtable_t *maps, set_name_t name);

static int process_criteria(archiver_cfg_t *cfg, hashtable_t *criteria,
	arch_set_t *set);

static int process_params_and_maps(archiver_cfg_t *cfg, hashtable_t *params,
	hashtable_t *maps, arch_set_t *set);

static int set_params(archiver_cfg_t *cfg, hashtable_t *params,
	set_name_t cpy_nm, ar_set_copy_params_t *p);

static int set_maps(archiver_cfg_t *cfg, hashtable_t *maps,
	set_name_t cpy_nm, vsn_map_t *m);

static int remove_criteria(hashtable_t *crit_tbl, arch_set_t *set);

void free_hashed_lists(hashtable_t *t);

int dup_arch_set(const arch_set_t *in, arch_set_t **out);

static int add_criteria(sqm_lst_t *l, ar_set_criteria_t *c);

/* hashtable free functions */
static void free_ar_set_criteria_list_void(void *v);
static void free_ar_set_copy_params_void(void *v);
static void free_vsn_map_void(void *v);
static void lst_free_void(void *v);

/*
 * Get a list of arch_set_t structures describing all archive sets.
 * This includes:
 *	All defined archive sets (policies and legacy policies),
 *	A single no_archive set that may contain many criteria,
 *	Default sets for each archiving filesystem.
 *
 * There is no order to the returned list.
 */
int
get_all_arch_sets(
ctx_t *ctx,
sqm_lst_t **l)	/* returned list of arch_set_t */
{
	archiver_cfg_t	*cfg = NULL;
	hashtable_t	*criteria = NULL;
	hashtable_t	*params = NULL;
	hashtable_t	*maps = NULL;
	ht_iterator_t	*it = NULL;
	ar_fs_directive_t *tmp_fs = NULL;
	node_t *n;
	arch_set_t *set = NULL;
	set_type_t type;


	Trace(TR_MISC, "getting all archive sets");

	if (ISNULL(l)) {
		Trace(TR_ERR, "getting all archive sets failed: %s",
		    samerrmsg);
		return (-1);
	}


	*l = NULL;

	/* just read it copy and free the cfg and return */
	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "getting all archive sets failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (populate_hashtables(cfg, &criteria, &maps,
	    &params, B_TRUE) != 0) {

		goto err;
	}


	*l = lst_create();
	if (*l == NULL) {
		goto err;
	}

	/*
	 * build the allsets set
	 */
	if (build_arch_set(ALL_SETS, NULL, ALLSETS_PSEUDO_SET,
	    params, maps, &set) != 0) {
		goto err;
	}

	if (lst_append(*l, set) != 0) {
		goto err;
	}

	/* build the default archive sets */
	if (cfg->ar_fs_p != NULL) {
		Trace(TR_DEBUG, "getting all archsets ar_fs_p not null");
		for (n = cfg->ar_fs_p->head; n != NULL; n = n->next) {
			sqm_lst_t *tmp_lst = NULL;

			tmp_fs = (ar_fs_directive_t *)n->data;
			Trace(TR_MISC, "getting all archsets tmp fs = %s",
			    tmp_fs->fs_name);
			if (tmp_fs->change_flag & AR_FS_no_archive) {
				continue;
			}

			/*
			 * Check to see if there is an explicit default
			 * archive set. To be true archive_meta must be
			 * off and the last criteria in the list must
			 * select all files.
			 *
			 * If so do not build the implicit set.
			 */
			if (tmp_fs->options &  FS_HAS_EXPLICIT_DEFAULT) {
				continue;
			}

			if (build_default_set_criteria(tmp_fs,
			    &tmp_lst) != 0) {
				goto err;
			}

			/* build the set for the fs */
			if (build_arch_set(tmp_fs->fs_name, tmp_lst,
			    DEFAULT_SET, params, maps, &set) != 0) {
				free_ar_set_criteria_list(tmp_lst);
				goto err;
			}
			if (lst_append(*l, set) != 0) {
				goto err;
			}
		}
	}

	/* build the no archive and general sets */
	if (ht_get_iterator(criteria, &it) != 0) {
		goto err;
	}

	while (ht_has_next(it)) {
		char *key;
		void *value;

		if (ht_get_next(it, &key, &value) != 0) {
			goto err;
		}

		if (strcmp(NO_ARCHIVE, key) == 0) {
			type = NO_ARCHIVE_SET;
		} else {
			type = GENERAL_SET;
		}
		if (build_arch_set(key, (sqm_lst_t *)value, type, params,
		    maps, &set) != 0) {
			goto err;
		}

		if (lst_append(*l, set) != 0) {
			goto err;
		}
	}


	free_hashtable(&criteria);
	free_hashtable(&params);
	free_hashtable(&maps);
	free_archiver_cfg(cfg);
	free(it);

	Trace(TR_MISC, "got %d archive sets", (*l)->length);
	return (0);

err:
	free_hashtable(&criteria);
	free_hashtable(&params);
	free_hashtable(&maps);
	free_archiver_cfg(cfg);
	free(it);
	free_arch_set(set);
	free_arch_set_list(*l);
	*l = NULL;

	Trace(TR_ERR, "getting all archive sets failed: %s", samerrmsg);

	return (-1);
}

static int
build_arch_set(
char *name,
sqm_lst_t *crits,
set_type_t set_type,
hashtable_t *param_tbl,
hashtable_t *vsn_map_tbl,
arch_set_t **set)
{
	arch_set_t *tmp;
	set_name_t cpy_nm;
	void *vp;
	int i;
	node_t *n;
	if (ISNULL(name)) {
		return (-1);
	}

	tmp = (arch_set_t *)mallocer(sizeof (arch_set_t));
	if (tmp == NULL) {
		return (-1);
	}

	memset(tmp, 0, sizeof (arch_set_t));
	strlcpy(tmp->name, name, sizeof (tmp->name));
	tmp->type = set_type;
	tmp->criteria = crits;


	if (set_type == GENERAL_SET && crits != NULL) {

		/* Check to see if it is actually an explicit default */
		for (n = crits->head; n != NULL; n = n->next) {
			ar_set_criteria_t *c = n->data;
			if (c->change_flag & AR_ST_default_criteria) {
				tmp->type = EXPLICIT_DEFAULT;
				break;
			}
		}

	}

	for (i = 0; i < MAX_COPY; i++) {
		snprintf(cpy_nm, sizeof (set_name_t), "%s.%d", name, i+1);
		if (ht_remove(param_tbl, cpy_nm, &vp) != 0) {
			return (-1);
		}
		tmp->copy_params[i] = (ar_set_copy_params_t *)vp;

		if (ht_remove(vsn_map_tbl, cpy_nm, &vp) != 0) {
			return (-1);
		}
		tmp->vsn_maps[i] = (vsn_map_t *)vp;

	}

	for (i = 0; i < MAX_COPY; i++) {
		vp = NULL;

		snprintf(cpy_nm, sizeof (set_name_t), "%s.%dR", name, i+1);
		if (ht_remove(param_tbl, cpy_nm, &vp) != 0) {
			return (-1);
		}
		tmp->rearch_copy_params[i] = (ar_set_copy_params_t *)vp;

		if (ht_remove(vsn_map_tbl, cpy_nm, &vp) != 0) {
			return (-1);
		}
		tmp->rearch_vsn_maps[i] = (vsn_map_t *)vp;

	}

	/* if this is an allsets populate the allset copy params and maps */
	if (strcmp(ALL_SETS, name) == 0) {
		vp = NULL;

		if (ht_remove(param_tbl, ALL_SETS, &vp) != 0) {
			return (-1);
		}
		tmp->copy_params[MAX_COPY] = (ar_set_copy_params_t *)vp;


		vp = NULL;
		if (ht_remove(vsn_map_tbl, ALL_SETS, &vp) != 0) {
			return (-1);
		}
		tmp->vsn_maps[MAX_COPY] = (vsn_map_t *)vp;



		/*
		 * allsetsR does not currently work but we can safely
		 * do the following steps anyway since the hashtable
		 * returns NULL
		 */
		vp = NULL;
		snprintf(cpy_nm, sizeof (set_name_t), "%sR", name, i);
		if (ht_remove(param_tbl, cpy_nm, &vp) != 0) {
			return (-1);
		}
		tmp->rearch_copy_params[MAX_COPY] = (ar_set_copy_params_t *)vp;

		vp = NULL;
		if (ht_remove(vsn_map_tbl, cpy_nm, &vp) != 0) {
			return (-1);
		}
		tmp->rearch_vsn_maps[MAX_COPY] = (vsn_map_t *)vp;
	}

	*set = tmp;
	return (0);
}


/*
 * currently does not remove the criteria from the list but maybe should
 */
static int
hash_criteria(
hashtable_t *t,
sqm_lst_t *l)
{

	ar_set_criteria_t *c;
	node_t *n;

	if (l == NULL) {
		return (0);
	}

	for (n = l->head; n != NULL; n = n->next) {
		c = (ar_set_criteria_t *)n->data;

		if (list_hash_put(t, c->set_name, c) != 0) {
			return (-1);
		}
	}
	return (0);

}


/*
 * Populates hashtables of all of the ar_set_criteria, vsn_maps and
 * ar_set_copy_params contained in the config based on their names.
 *
 * If disconnect is true, then the connections(pointers) from the cfg
 * to the structures in the hashtables will be broken so that the cfg
 * can be freed separately.
 *
 * If disconnect is false, the cfg will be left in tact. This is
 * useful as it allows rapid searching with the hashtables but
 * preserves the config so that a write can subsequently be performed.
 */
static int
populate_hashtables(
archiver_cfg_t *cfg,
hashtable_t **criteria,
hashtable_t **maps,
hashtable_t **params,
boolean_t disconnect)
{

	ar_fs_directive_t *tmp_fs = NULL;
	ar_set_copy_params_t *tmp_param = NULL;
	vsn_map_t *tmp_map = NULL;
	node_t *n;


	Trace(TR_DEBUG, "populating hashtables");

	/* hash all of the global and per fs criteria */
	*criteria = ht_create();
	if (*criteria == NULL) {
		goto err;
	}

	if (hash_criteria(*criteria, cfg->global_dirs.ar_set_lst) != 0) {
		goto err;
	}


	if (disconnect) {
		/* free the list but not the elements */
		lst_free(cfg->global_dirs.ar_set_lst);
		cfg->global_dirs.ar_set_lst = NULL;
	}

	if (cfg->ar_fs_p != NULL) {
		for (n = cfg->ar_fs_p->head; n != NULL; n = n->next) {

			tmp_fs = (ar_fs_directive_t *)n->data;
			if (hash_criteria(*criteria,
			    tmp_fs->ar_set_criteria) != 0) {

				goto err;
			}

			if (disconnect) {
				/* free the list but not the elements */
				lst_free(tmp_fs->ar_set_criteria);
				tmp_fs->ar_set_criteria = NULL;
			}
		}
	}

	/* hash all of the ar_set_copy_params */
	*params = ht_create();
	if (*params == NULL) {
		goto err;
	}

	if (cfg->archcopy_params != NULL) {
		for (n = cfg->archcopy_params->head; n != NULL; n = n->next) {
			tmp_param = (ar_set_copy_params_t *)n->data;

			if (ht_put(*params, tmp_param->ar_set_copy_name,
			    tmp_param) != 0) {

				goto err;
			}
		}

		if (disconnect) {
			/* free the list but not the elements */
			lst_free(cfg->archcopy_params);
			cfg->archcopy_params = NULL;
		}
	}

	/* hash all of the vsn maps */
	*maps = ht_create();
	if (*maps == NULL) {
		goto err;
	}

	if (cfg->vsn_maps != NULL) {
		for (n = cfg->vsn_maps->head; n != NULL; n = n->next) {
			tmp_map = (vsn_map_t *)n->data;

			if (ht_put(*maps, tmp_map->ar_set_copy_name,
			    tmp_map) != 0) {

				goto err;
			}
		}

		if (disconnect) {
			/* free the list but not the elements */
			lst_free(cfg->vsn_maps);
			cfg->vsn_maps = NULL;
		}
	}


	Trace(TR_DEBUG, "populated hashtables");

	return (0);
err:
	free_hashtable(criteria);
	free_hashtable(params);
	free_hashtable(maps);
	Trace(TR_ERR, "populating hashtables failed: %s", samerrmsg);
	return (-1);


}



/*
 * From the fs directive build the criteria for the file system's default
 * copy.
 */
int
build_default_set_criteria(
ar_fs_directive_t *fs,
sqm_lst_t **l)
{

	sqm_lst_t *tmp_lst;
	ar_set_criteria_t *tmp_crit;
	int i;

	/* create a criteria to contain the fs copies */
	tmp_lst = lst_create();
	if (tmp_lst == NULL) {
		return (-1);
	}
	tmp_crit = mallocer(sizeof (ar_set_criteria_t));
	if (tmp_crit == NULL) {
		lst_free(tmp_lst);
		return (-1);
	}
	memset(tmp_crit, 0, sizeof (ar_set_criteria_t));
	strlcpy(tmp_crit->fs_name, fs->fs_name, sizeof (tmp_crit->fs_name));

	strlcpy(tmp_crit->set_name, fs->fs_name,
	    sizeof (tmp_crit->fs_name));

	for (i = 0; i < MAX_COPY; i++) {
		tmp_crit->arch_copy[i] = fs->fs_copy[i];
		fs->fs_copy[i] = NULL;
	}

	if (lst_append(tmp_lst, tmp_crit) != 0) {
		lst_free(tmp_lst);
		free_ar_set_criteria(tmp_crit);
		return (-1);
	}

	*l = tmp_lst;
	return (0);
}


/*
 * get the named arch_set structure
 */
int
get_arch_set(
ctx_t *ctx,		/* can be used to read from alternate location */
set_name_t name,	/* name of the set to return */
arch_set_t **set)	/* Malloced return value */
{

	archiver_cfg_t	*cfg = NULL;
	hashtable_t	*criteria = NULL;
	hashtable_t	*params = NULL;
	hashtable_t	*maps = NULL;
	ar_fs_directive_t *tmp_fs = NULL;
	node_t *n;
	set_type_t type = -1;
	void *vp = NULL;
	boolean_t found = B_FALSE;


	Trace(TR_MISC, "getting archive set %s", Str(name));

	if (ISNULL(name, set)) {
		Trace(TR_ERR, "getting archive set %s failed: %s",
		    Str(name), samerrmsg);
		return (-1);
	}

	/* set the return param to NULL so free will work */
	*set = NULL;

	/* just read it copy and free the cfg and return */
	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "getting archive set %s failed: %s",
		    Str(name), samerrmsg);
		return (-1);
	}

	if (populate_hashtables(cfg, &criteria, &maps,
	    &params, B_TRUE) != 0) {

		goto err;
	}

	/* See if it is a no_archive or general set */
	/* build the no archive and general sets */

	if (strcmp(ALL_SETS, name) == 0) {
		type = ALLSETS_PSEUDO_SET;
		vp = lst_create();

	} else if (ht_remove(criteria, name, &vp) != 0) {
		goto err;

	} else if (vp != NULL) {
		if (strcmp(NO_ARCHIVE, name) == 0) {
			type = NO_ARCHIVE_SET;
		} else {
			type = GENERAL_SET;
		}
	} else {
		/* potentially a default set or possibly does not exist */
		type = DEFAULT_SET;
	}


	/* based on type build the set */
	if (type != DEFAULT_SET) {

		/* this branch builds allsets, general and no_archive sets */
		if (build_arch_set(name, (sqm_lst_t *)vp, type, params,
		    maps, set) != 0) {
			goto err;
		}

		found = B_TRUE;

	} else if (cfg->ar_fs_p != NULL) {
		/* must be default set or not found */

		/* find the fs directive for the named set and build the set */
		for (n = cfg->ar_fs_p->head; n != NULL; n = n->next) {
			tmp_fs = (ar_fs_directive_t *)n->data;

			if (strcmp(tmp_fs->fs_name, name) == 0) {
				sqm_lst_t *tmp_lst = NULL;

				if (build_default_set_criteria(tmp_fs,
				    &tmp_lst) != 0) {
					goto err;
				}

				if (build_arch_set(tmp_fs->fs_name, tmp_lst,
				    DEFAULT_SET, params, maps, set) != 0) {
					free_ar_set_criteria_list(tmp_lst);
					goto err;
				}
				found = B_TRUE;
				break;
			}
		}
	}

	if (!found) {
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno), name);
		goto err;
	}



	/*
	 * Since the structs have been disconnected from the cfg they
	 * must be freed
	 */
	ht_free_deep(&criteria, &free_ar_set_criteria_list_void);
	ht_free_deep(&params, &free_ar_set_copy_params_void);
	ht_free_deep(&maps, &free_vsn_map_void);
	free_archiver_cfg(cfg);

	Trace(TR_MISC, "got archive set %s", Str(name));

	return (0);

err:
	ht_free_deep(&criteria, &free_ar_set_criteria_list_void);
	ht_free_deep(&params, &free_ar_set_copy_params_void);
	ht_free_deep(&maps, &free_vsn_map_void);
	free_archiver_cfg(cfg);
	free_arch_set(*set);
	*set = NULL;

	Trace(TR_ERR, "get archive set %s failed: %s", Str(name), samerrmsg);
	return (-1);
}


/*
 * Function to create a general or no_archive archive set. This
 * function can not be used to create the allsets set or the default
 * archive sets because those all exist by default? The criteria will
 * be added to the end of any file system for which they are
 * specified. Can not be used to create a default arch set. This
 * function will fail if the set already exists.
 */
int
create_arch_set(
ctx_t *ctx,    		/* contains optional dump path */
arch_set_t *set)	/* archive set to create */
{

	archiver_cfg_t *cfg = NULL;
	arch_set_t *tmp = NULL;
	int i;
	node_t *n;
	node_t *fsn;

	if (ISNULL(set)) {
		Trace(TR_ERR, "create archive set failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "creating archive set %s", Str(set->name));

	/* just read it copy and free the cfg and return */
	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "create archive set failed: %s", samerrmsg);
		return (-1);
	}


	/* verify that the set does not exist yet. */
	if (set_exists(cfg, set->name)) {
		samerrno = SE_ALREADY_EXISTS;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ALREADY_EXISTS), set->name);

		free_archiver_cfg(cfg);
		Trace(TR_ERR, "create archive set failed: %s", samerrmsg);
		return (-1);
	}


	/*
	 * We must duplicate the input or keep track of
	 * all of its pieces as they get spread out into the cfg. This
	 * allows us to free the cfg without breaking the input.
	 */
	if (dup_arch_set(set, &tmp) != 0) {
		free_archiver_cfg(cfg);
		Trace(TR_ERR, "create archive set failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * lst_append each of the criteria to the appropriate
	 * fs/global lists
	 */
	for (n = tmp->criteria->head; n != NULL; n = n->next) {

		ar_set_criteria_t *c = (ar_set_criteria_t *)n->data;

		if (strcmp(c->fs_name, GLOBAL) == 0) {

			if (add_criteria(cfg->global_dirs.ar_set_lst,
			    c) != 0) {

				free_archiver_cfg(cfg);
				free_arch_set(tmp);

				Trace(TR_ERR, "create archive set failed: %s",
				    samerrmsg);
				return (-1);
			}

			/*
			 * set the entry in list to NULL so free will work.
			 */
			n->data = NULL;
			continue;
		}


		/* note that if it was a global crit you don't get this far */

		for (fsn = cfg->ar_fs_p->head; fsn != NULL; fsn = fsn->next) {
			ar_fs_directive_t *fs = (ar_fs_directive_t *)fsn->data;

			if (strcmp(fs->fs_name, c->fs_name) == 0) {

				Trace(TR_OPRMSG, "creating archive set %s",
				    c->set_name);

				if (add_criteria(fs->ar_set_criteria,
				    c) != 0) {
					free_archiver_cfg(cfg);
					free_arch_set(tmp);

					Trace(TR_ERR, "create archive %s: %s",
					    "set failed", samerrmsg);
					return (-1);
				}

				/*
				 * set the entry in list to NULL so
				 * free will work.
				 */
				n->data = NULL;
				break;
			}
		}
	}

	/* lst_append copy params and vsn maps */
	for (i = 0; i < MAX_COPY; i++) {
		if (tmp->copy_params[i] != NULL) {
			if (lst_append(cfg->archcopy_params,
			    tmp->copy_params[i]) != 0) {

				free_arch_set(tmp);
				free_archiver_cfg(cfg);
				Trace(TR_ERR, "create archive set failed: %s",
				    samerrmsg);
				return (-1);
			}

			/* set entry to NULL so free will work */
			tmp->copy_params[i] = NULL;
		}

		if (tmp->rearch_copy_params[i] != NULL) {
			if (lst_append(cfg->archcopy_params,
			    tmp->rearch_copy_params[i]) != 0) {

				free_arch_set(tmp);
				free_archiver_cfg(cfg);
				Trace(TR_ERR, "create archive set failed: %s",
				    samerrmsg);
				return (-1);
			}

			/* set entry to NULL so free will work */
			tmp->rearch_copy_params[i] = NULL;
		}

		if (tmp->vsn_maps[i] != NULL) {
			if (lst_append(cfg->vsn_maps, tmp->vsn_maps[i]) != 0) {

				free_arch_set(tmp);
				free_archiver_cfg(cfg);
				Trace(TR_ERR, "create archive set failed: %s",
				    samerrmsg);
				return (-1);
			}

			/* set entry to NULL so free will work */
			tmp->vsn_maps[i] = NULL;
		}

		if (tmp->rearch_vsn_maps[i] != NULL) {
			if (lst_append(cfg->vsn_maps,
			    tmp->rearch_vsn_maps[i]) != 0) {

				free_arch_set(tmp);
				free_archiver_cfg(cfg);
				Trace(TR_ERR, "create archive set failed: %s",
				    samerrmsg);
				return (-1);
			}

			/* set entry to NULL so free will work */
			tmp->rearch_vsn_maps[i] = NULL;
		}
	}


	/* write it */
	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {
		free_archiver_cfg(cfg);
		free_arch_set(tmp);
		Trace(TR_ERR, "create archive set failed: %s", samerrmsg);
		return (-1);
	}

	free_archiver_cfg(cfg);
	free_arch_set(tmp);
	return (0);
}


/*
 * create a malloced copy of the input arch_set_t
 */
int
dup_arch_set(
const arch_set_t *in,
arch_set_t **out)
{

	int i;


	if (ISNULL(in, out)) {
		return (-1);
	}

	*out = (arch_set_t *)mallocer(sizeof (arch_set_t));
	if (*out == NULL) {
		return (-1);
	}

	memset((*out), 0, sizeof (arch_set_t));
	strlcpy((*out)->name, in->name, sizeof ((*out)->name));
	(*out)->type = in->type;
	if (dup_ar_set_criteria_list(in->criteria, &((*out)->criteria)) != 0) {
		free_arch_set(*out);
		return (-1);
	}


	/* include the zero index for the allsets set */
	for (i = 0; i <= MAX_COPY; i++) {
		if (in->copy_params[i] != NULL) {
			if (dup_ar_set_copy_params(in->copy_params[i],
			    &((*out)->copy_params[i])) != 0) {

				free_arch_set(*out);
				return (-1);
			}
		}

		if (in->rearch_copy_params[i] != NULL) {
			if (dup_ar_set_copy_params(in->rearch_copy_params[i],
			    &((*out)->rearch_copy_params[i])) != 0) {

				free_arch_set(*out);
				return (-1);
			}
		}

		if (in->vsn_maps[i] != NULL) {
			if (dup_vsn_map(in->vsn_maps[i],
			    &((*out)->vsn_maps[i])) != 0) {

				free_arch_set(*out);
				return (-1);
			}
		}

		if (in->rearch_vsn_maps[i] != NULL) {
			if (dup_vsn_map(in->rearch_vsn_maps[i],
			    &((*out)->rearch_vsn_maps[i])) != 0) {

				free_arch_set(*out);
				return (-1);
			}

		}

	}

	return (0);
}



/*
 * Delete will delete the entire set including copies, criteria,
 * params and vsn associations.
 *
 * If the named set is the allsets pseudo set this function will
 * reset defaults because the allsets set can not really be deleted.
 *
 * If the named set is a default set this function will return the
 * error: SE_CANNOT_DELETE_DEFAULT_SET. It is not possible to delete a
 * default set because an archiving file system must have archive
 * media assigned to it. Users wishing to remove all archiving
 * related traces of an archiving file system from the archiver.cmd
 * should instead use reset_ar_fs_directive.
 */
int
delete_arch_set(
ctx_t *ctx,		/* contains optional dump path */
set_name_t name)	/* the name of the set to delete */
{

	archiver_cfg_t *cfg;
	hashtable_t *crits = NULL;
	hashtable_t *params = NULL;
	hashtable_t *maps = NULL;
	node_t *n = NULL;
	ar_set_criteria_t *c;
	set_name_t cpy_nm;
	int i;
	sqm_lst_t *l;
	void *vp;
	boolean_t found = B_FALSE;


	Trace(TR_MISC, "delete archive set %s", Str(name));

	if (ISNULL(name)) {
		Trace(TR_ERR, "delete archive set %s failed: %s",
		    Str(name), samerrmsg);
		return (-1);
	}

	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "delete archive set %s failed: %s",
		    name, samerrmsg);
		return (-1);
	}

	/*
	 * populate the hashtables, then zero the change flags for all
	 * items and rewrite the config.
	 */
	if (populate_hashtables(cfg, &crits, &maps, &params, B_FALSE) != 0) {
		goto err;
	}

	for (i = 0; i < MAX_COPY; i++) {

		snprintf(cpy_nm, sizeof (set_name_t), "%s.%d", name, i+1);



		if (reset_maps_and_params(params, maps, cpy_nm) != 0) {
			goto err;
		}

		snprintf(cpy_nm, sizeof (set_name_t), "%s.%dR", name, i+1);
		if (reset_maps_and_params(params, maps, cpy_nm) != 0) {
			goto err;
		}

	}

	/*
	 * if it is an allsets, handle the allsets copy params and vsn maps
	 * and skip the criteria which do not apply to allsets
	 */
	if (strcmp(ALL_SETS, name) == 0) {

		if (reset_maps_and_params(params, maps, ALL_SETS) != 0) {
			goto err;
		}

		/*
		 * allsetsR does not currently work but we can safely
		 * do the following steps anyway since the hashtable
		 * returns NULL. So do it incase the allsetsR gets fixed.
		 */

		snprintf(cpy_nm, sizeof (set_name_t), "%sR", name);
		if (reset_maps_and_params(params, maps, cpy_nm) != 0) {
			goto err;
		}
		found = B_TRUE;

	} else if (ht_get(crits, name, &vp) != 0) {
		goto err;
	} else if (vp != NULL) {
		l = (sqm_lst_t *)vp;

		/* Got the criteria now remove them */
		for (n = l->head; n != NULL; n = n->next) {
			c = (ar_set_criteria_t *)n->data;

			/*
			 * Check to see if the criteria is for an
			 * explicit default set. Trace a message so
			 * we know this code has been hit. It should
			 * only occur with a file system deletion.
			 */
			if (c->change_flag & AR_ST_default_criteria) {
				Trace(TR_MISC, "delete called for default"
				    " archive set %s", c->set_name);
			}

			/* Indicate in cfg that the crit should be deleted */
			c->change_flag = 0;
		}
		found = B_TRUE;

	} else {
		/* must be a file system default set */
		for (n = cfg->ar_fs_p->head; n != NULL; n = n->next) {
			ar_fs_directive_t *fs = (ar_fs_directive_t *)n->data;
			if (strcmp(fs->fs_name, name) == 0) {
				samerrno = SE_CANNOT_DELETE_DEFAULT_SET;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_CANNOT_DELETE_DEFAULT_SET),
				    name);
				goto err;
			}
		}

	}

	if (!found) {
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    name);
		goto err;
	}


	/* now write the modified config */
	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {
		goto err;
	}


	free_hashed_lists(crits);
	free_hashtable(&crits);
	free_hashtable(&params);
	free_hashtable(&maps);
	free_archiver_cfg(cfg);
	Trace(TR_MISC, "deleted archive set %s", name);
	return (0);

err:
	free_hashed_lists(crits);
	free_hashtable(&crits);
	free_hashtable(&params);
	free_hashtable(&maps);
	free_archiver_cfg(cfg);
	Trace(TR_ERR, "delete archive set failed: %s", samerrmsg);
	return (-1);
}


void
free_hashed_lists(hashtable_t *t) {
	int i;
	hnode_t *hn;

	for (i = 0; i < t->size; i++) {
		if (t->items[i] != NULL) {
			hn = t->items[i];
			do {
				lst_free((sqm_lst_t *)hn->data);
				hn = hn->next;
			} while (hn != NULL);
		}
	}
}


static int
reset_maps_and_params(hashtable_t *params, hashtable_t *maps,
    set_name_t name) {

	ar_set_copy_params_t *p;
	vsn_map_t *v;
	void *vp;



	/* delete copy params, by setting change flags to zero */
	vp = NULL;
	if (ht_get(params, name, &vp) != 0) {
		return (-1);
	}

	if (vp != NULL) {
		p = (ar_set_copy_params_t *)vp;
		p->change_flag = 0;
		p->recycle.change_flag = 0;
	}


	/* delete vsn maps by setting the fields to null */
	vp = NULL;
	if (ht_get(maps, name, &vp) != 0) {
		return (-1);
	}
	if (vp != NULL) {
		v = (vsn_map_t *)vp;
		lst_free_deep(v->vsn_names);
		lst_free_deep(v->vsn_pool_names);
		v->vsn_names = NULL;
		v->vsn_pool_names = NULL;
	}

	return (0);
}


int
modify_arch_set(
ctx_t *ctx,
arch_set_t *set)
{

	archiver_cfg_t *cfg = NULL;
	hashtable_t *params = NULL;
	hashtable_t *maps = NULL;
	hashtable_t *crits = NULL;


	if (ISNULL(set)) {
		Trace(TR_ERR, "modifying arch set failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "modifying arch set %s", Str(set->name));


	/* just read it copy and free the cfg and return */
	if (read_arch_cfg(ctx, &cfg) != 0) {
		/* leave samerrno as set */

		Trace(TR_ERR, "modifying arch set failed: %s", samerrmsg);
		return (-1);
	}


	if (populate_hashtables(cfg, &crits, &maps, &params, B_FALSE) != 0) {
		goto err;
	}


	/* Need to make sure the set already exists, see if it is an allsets */

	/*
	 * process the criteria entries except for the allsets which does not
	 * have any
	 */
	if (set->type != ALLSETS_PSEUDO_SET) {
		if (process_criteria(cfg, crits, set) != 0) {
			goto err;
		}
	}



	/* modify the copy params and vsn maps make sure they are required */
	if (set->type != NO_ARCHIVE_SET) {
		/* skip params and resource processing */
		if (process_params_and_maps(cfg, params, maps, set) != 0) {

			goto err;
		}
	}

	/* write it  and verify it */
	if (write_arch_cfg(ctx, cfg, B_TRUE) != 0) {
		goto err;
	}

	free_hashtable(&params);
	free_hashtable(&maps);
	ht_free_deep(&crits, &lst_free_void);
	free_archiver_cfg(cfg);

	return (0);


err:
	free_hashtable(&params);
	free_hashtable(&maps);
	ht_free_deep(&crits, &lst_free_void);
	free_archiver_cfg(cfg);
	Trace(TR_ERR, "modify archive set failed: %s", samerrmsg);
	return (-1);

}





/*
 * Make this serve the purpose of reset_maps_and_params
 */
int
process_params_and_maps(
archiver_cfg_t *cfg,
hashtable_t *params,
hashtable_t *maps,
arch_set_t *set)
{

	int i;
	set_name_t cpy_nm;

	for (i = 0; i < MAX_COPY; i++) {

		snprintf(cpy_nm, sizeof (set_name_t), "%s.%d", set->name, i+1);
		if (set_params(cfg, params, cpy_nm,
		    set->copy_params[i]) != 0) {

			goto err;
		}

		if (set_maps(cfg, maps, cpy_nm, set->vsn_maps[i]) != 0) {
			goto err;
		}

		snprintf(cpy_nm, sizeof (set_name_t), "%s.%dR",
		    set->name, i+1);

		if (set_params(cfg, params, cpy_nm,
		    set->rearch_copy_params[i]) != 0) {

			goto err;
		}

		if (set_maps(cfg, maps, cpy_nm,
		    set->rearch_vsn_maps[i]) != 0) {

			goto err;
		}

	}


	/*
	 * if it is an allsets handle the allsets copy params and vsn maps
	 * and skip the criteria which do not apply to allsets
	 */
	if (strcmp(ALL_SETS, set->name) == 0) {
		if (set_params(cfg, params, ALL_SETS,
		    set->copy_params[i]) != 0) {

			goto err;
		}

		if (set_maps(cfg, maps, ALL_SETS, set->vsn_maps[i]) != 0) {
			goto err;
		}

		/*
		 * allsetsR does not currently work but we can safely
		 * do the following steps anyway since the hashtable
		 * returns NULL. So do it incase the allsetsR gets fixed.
		 */

		snprintf(cpy_nm, sizeof (set_name_t), "%sR", set->name);
		if (set_params(cfg, params, cpy_nm,
		    set->rearch_copy_params[i]) != 0) {

			goto err;
		}

		if (set_maps(cfg, maps, cpy_nm,
		    set->rearch_vsn_maps[i]) != 0) {
			goto err;
		}
	}

	Trace(TR_OPRMSG, "processing maps and params succeeded");

	return (0);

err:
	Trace(TR_ERR, "processing maps and params failed: %d %s", samerrno,
	    samerrmsg);
	return (-1);

}


static int
set_fs_copies(
ar_fs_directive_t *fs,
arch_set_t *set)
{
	ar_set_criteria_t *c;
	int i;


	c = (ar_set_criteria_t *)set->criteria->head->data;
	for (i = 0; i < MAX_COPY; i++) {
		if (fs->fs_copy[i] == NULL && c->arch_copy[i] != NULL) {

			if (dup_ar_set_copy_cfg(c->arch_copy[i],
			    &(fs->fs_copy[i])) != 0) {

				return (-1);
			}

		} else if (fs->fs_copy[i] != NULL && c->arch_copy[i] == NULL) {

			free(fs->fs_copy[i]);
			fs->fs_copy[i] = NULL;

		} else if (fs->fs_copy[i] != NULL && c->arch_copy[i] != NULL) {

			memcpy(fs->fs_copy[i], c->arch_copy[i],
			    sizeof (ar_set_copy_cfg_t));
		}

	}
	return (0);
}


/*
 * set the copy params in the hashtable to match the input argument p.
 * If p == NULL this is equivalent to a reset copy params.
 */
int
set_params(
archiver_cfg_t *cfg,
hashtable_t *params,
set_name_t cpy_nm,
ar_set_copy_params_t *p)
{

	void *vp = NULL;
	ar_set_copy_params_t *cp;


	if (ht_get(params, cpy_nm, &vp) != 0) {
		return (-1);
	} else if (vp == NULL && p != NULL) {
		/*
		 * params must be duplicated so that the cfg can be
		 * freed without impacting the users input argument
		 */
		if (dup_ar_set_copy_params(p, &cp) != 0) {
			return (-1);
		}

		if (cfg_insert_copy_params(cfg->archcopy_params, cp) != 0) {
			free_ar_set_copy_params(cp);
			return (-1);
		}
	} else if (vp != NULL && p == NULL) {
		/* reset vp */
		lst_free_deep(((ar_set_copy_params_t *)vp)->priority_lst);
		((ar_set_copy_params_t *)vp)->priority_lst = NULL;
		((ar_set_copy_params_t *)vp)->change_flag = 0;
		((ar_set_copy_params_t *)vp)->recycle.change_flag = 0;

	} else if (vp != NULL && p != NULL) {

		lst_free_deep(((ar_set_copy_params_t *)vp)->priority_lst);
		((ar_set_copy_params_t *)vp)->priority_lst = NULL;

		if (cp_ar_set_copy_params(p,
		    (ar_set_copy_params_t *)vp) != 0) {

			return (-1);
		}
	}

	return (0);
}



/*
 * set the vsn maps in the hashtable to match the input argument m.
 * If m == NULL this is equivalent to reseting the vsn map.
 */
static int
set_maps(
archiver_cfg_t *cfg,
hashtable_t *maps,
set_name_t cpy_nm,
vsn_map_t *m)
{

	void *vp = NULL;
	vsn_map_t *v;


	/*
	 * Things to be aware of:
	 * 1. vp is the map in the cfg.
	 * 2. m is a user input that should be added/copied to the cfg
	 * 3. v is a copy of m so that it can be added and allow
	 * freeing of memory
	 */
	if (ht_get(maps, cpy_nm, &vp) != 0) {
		return (-1);
	} else if (vp == NULL && m != NULL) {

		if (dup_vsn_map(m, &v) != 0) {
			return (-1);
		}
		if (cfg_insert_copy_map(cfg->vsn_maps, v) != 0) {
			return (-1);
		}
	} else if (vp != NULL && m == NULL) {

		/* reset map */
		v = (vsn_map_t *)vp;
		lst_free_deep(v->vsn_names);
		lst_free_deep(v->vsn_pool_names);
		v->vsn_names = NULL;
		v->vsn_pool_names = NULL;

	} else if (vp != NULL && m != NULL) {

		v = (vsn_map_t *)vp;
		lst_free_deep(v->vsn_names);
		lst_free_deep(v->vsn_pool_names);
		if (cp_vsn_map(m, v) != 0) {
			return (-1);
		}
	}
	/* else if (vp == NULL && m == NULL) return (0); */

	return (0);
}

static int
process_criteria(archiver_cfg_t *cfg, hashtable_t *criteria, arch_set_t *set) {

	node_t *n;
	node_t *crit_node;
	hashtable_t *fs_tbl = NULL;
	ar_set_criteria_t *crit = NULL;
	void *vp = NULL;
	ar_fs_directive_t *fs_dir;


	/* hash the fs directives to avoid multiple list traversals */
	fs_tbl = ht_create();
	if (fs_tbl == NULL) {
		return (-1);
	}

	for (n = cfg->ar_fs_p->head; n != NULL; n = n->next) {
		if (ht_put(fs_tbl, ((ar_fs_directive_t *)n->data)->fs_name,
			n->data) != 0) {

			free_hashtable(&fs_tbl);
			return (-1);
		}
	}


	if (set->type == DEFAULT_SET) {
		if (ht_get(fs_tbl, set->name, &vp) != 0) {
			free_hashtable(&fs_tbl);
			return (-1);
		}
		fs_dir = (ar_fs_directive_t *)vp;

		if (fs_dir == NULL) {
			samerrno = SE_NOT_FOUND;
			snprintf(samerrmsg, MAX_MSG_LEN,
				GetCustMsg(SE_NOT_FOUND), set->name);
			free_hashtable(&fs_tbl);
			return (-1);
		}

		if (set_fs_copies(fs_dir, set) != 0) {
			free_hashtable(&fs_tbl);
			return (-1);
		}

		free_hashtable(&fs_tbl);
		return (0);

	} else if (ht_get(criteria, set->name, &vp) != 0) {
		free_hashtable(&fs_tbl);
		return (-1);
	} else if (vp == NULL) {
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), set->name);
		free_hashtable(&fs_tbl);
		return (-1);
	}



	/*
	 * loop through the criteria in the cfg, search for the keys in set
	 * if they don't exist delete from cfg.
	 */
	if (remove_criteria(criteria, set) != 0) {
		free_hashtable(&fs_tbl);
		return (-1);
	}



	/* for each criteria in set, find it in cfg and update it to match */
	for (n = set->criteria->head; n != NULL; n = n->next) {

		/* make a copy so we can replace in/add to the cfg */
		if (dup_ar_set_criteria((ar_set_criteria_t *)n->data,
			&crit) != 0) {
			goto err;
		}

		if (find_ar_set_criteria_node(cfg, crit, &crit_node) != 0) {

			if (samerrno != SE_NOT_FOUND) {
				goto err;
			}

			/*
			 * for each criteria in set but not in cfg,
			 * add it to cfg. This means either a new criteria was
			 * added or one was applied to this set that was
			 * previously applied elsewhere.
			 */
			if (strcmp(crit->fs_name, GLOBAL) != 0) {
				if (ht_get(fs_tbl, crit->fs_name, &vp) != 0) {
					goto err;
				} else if (vp == NULL) {
					samerrno = SE_NOT_FOUND;
					snprintf(samerrmsg, MAX_MSG_LEN,
						GetCustMsg(SE_NOT_FOUND),
						crit->fs_name);
					goto err;
				} else {
					fs_dir = (ar_fs_directive_t *)vp;
					if (add_criteria(
						fs_dir->ar_set_criteria,
						crit) != 0) {

						goto err;
					}
				}
			} else {
				/* the criteria is for the global section */
				if (add_criteria(cfg->global_dirs.ar_set_lst,
					crit) != 0) {

					goto err;
				}
			}

		} else {
			/* the criteria was found in the cfg */
			free_ar_set_criteria(
				(ar_set_criteria_t *)crit_node->data);

			crit_node->data = crit;
		}
	}

	free_hashtable(&fs_tbl);
	return (0);

err:
	free_hashtable(&fs_tbl);
	return (-1);
}


/*
 * Add a criteria to the list. This function makes sure that no
 * ordering rules are violated. This means that the explicit default
 * policy will be kept in the last position.
 */
static int
add_criteria(sqm_lst_t *l, ar_set_criteria_t *c) {
	ar_set_criteria_t *tmp = NULL;


	if (ISNULL(l)) {
		return (-1);
	}
	if (l->tail != NULL) {
		tmp = (ar_set_criteria_t *)l->tail->data;
	}

	if (tmp != NULL && tmp->change_flag & AR_ST_default_criteria) {

		if (lst_ins_before(l, l->tail, c) != 0) {
			return (-1);
		}
	} else if (lst_append(l, c) != 0) {
		return (-1);
	}

	return (0);
}

static int
remove_criteria(
hashtable_t *crit_tbl,
arch_set_t *set)
{

	sqm_lst_t *l;
	void *vp;
	ar_set_criteria_t *c1;
	ar_set_criteria_t *c2;
	node_t *cfg_nd;
	node_t *set_nd;
	boolean_t found;

	/* get the list of criteria from the cfg for this set */
	if (ht_get(crit_tbl, set->name, &vp) != 0) {
		return (-1);
	}


	l = (sqm_lst_t *)vp;
	if (l == NULL) {
		return (0);
	}


	/*
	 * The goal of these nested loops is to find criteria that should
	 * no longer be in the configuration when this call completes.
	 *
	 * In the outer loop:
	 * Loop on the criteria that were part of the set in the cfg
	 * prior to this call
	 */
	for (cfg_nd = l->head; cfg_nd != NULL; cfg_nd = cfg_nd->next) {
		c1 = (ar_set_criteria_t *)cfg_nd->data;
		found = B_FALSE;

		/*
		 * In this inner loop:
		 * Loop through all of the criteria that will
		 * be part of the set after this call. If the
		 * criteria from cfg is not found in set delete it
		 * from cfg.
		 */
		for (set_nd = set->criteria->head; set_nd != NULL;
		    set_nd = set_nd->next) {
			c2 = (ar_set_criteria_t *)set_nd->data;

			if (strcmp(c1->fs_name, c2->fs_name) == 0) {
				if (keys_match(c1->key, c2->key)) {
					found = B_TRUE;
				}
			}
		}

		if (!found) {
			Trace(TR_MISC, "process criteria is %s %s",
			    "deleting criteria for", c1->set_name);
			c1->change_flag = 0;
		}

	} /* end outer loop */

	return (0);
}




static void
free_ar_set_criteria_list_void(void *v) {
	free_ar_set_criteria_list((sqm_lst_t *)v);
}


static void
free_ar_set_copy_params_void(void *v) {
	free_ar_set_copy_params((ar_set_copy_params_t *)v);
}


static void
free_vsn_map_void(void *v) {
	free_vsn_map((vsn_map_t *)v);
}


static void
lst_free_void(void *v) {
	lst_free((sqm_lst_t *)v);
}
