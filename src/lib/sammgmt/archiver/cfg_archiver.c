/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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
#pragma ident   "$Revision: 1.62 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


/*
 * cfg_archiver.c
 *
 * functions that interact with the parsers to read and write the archiver.cmd
 * file and helper functions to search the archiver_cfg_t structure.
 *
 * In this file if a function has the name get_X it will return a duplicate
 * of the structure that is in the config. This duplicate will have been
 * dynamically allocated so that it can be freed by an end user.
 *
 * If a function has the name find_X it will return a pointer to the actual
 * structure in the archiver_cfg_t structure. These functions are useful
 * for searching for and changing structures in the config. There is only one
 * find function currently but the code could be pared down by creating some
 * more.
 */


#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "pub/mgmt/types.h"
#include "sam/sam_trace.h"
#include "sam/setfield.h"
#include "mgmt/util.h"


#define	NEED_ARCHSET_NAMES
#define	NEED_EXAM_METHOD_NAMES
#define	NEED_FILEPROPS_NAMES

#undef  DCL
#undef  IVAL
#if defined(DEC_INIT)
#define	DCL
#define	IVAL(v) = v
#else /* defined(DEC_INIT) */
#define	DCL extern
#define	IVAL(v) /* v */
#endif /* defined(DEC_INIT) */

#define	IF_NO_MEDIA	"lt"

#include "src/archiver/include/archset.h"

#include "pub/mgmt/device.h"

#include "pub/mgmt/archive.h"
#include "mgmt/config/archiver.h"
#include "pub/devstat.h"

#include "pub/mgmt/sqm_list.h"
#include "mgmt/util.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/error.h"

#include "mgmt/config/cfg_diskvols.h"
#include "parser_utils.h"
#include "pub/mgmt/filesystem.h"
#include "mgmt/config/data_class.h"
#include "mgmt/config/cfg_fs.h"

/* full path to the archiver.cmd file */
static char *archiver_file = ARCHIVER_CFG;


/* externs used here defined elsewhere */
extern char *StrFromInterval(int interval, char *buf, int buf_size);



/* Private helper functions. */
static int write_archiver(const char *location, archiver_cfg_t *cfg);
static int write_ar_set_copy_params(FILE *arch_p, archiver_cfg_t *cfg);
static int write_vsn_pools(FILE *arch_p, archiver_cfg_t *cfg);
static int write_ar_fs_directive_t(FILE *f, ar_fs_directive_t *fs);
static int write_all_fs(FILE *f, archiver_cfg_t *cfg);
static int write_ar_set_criteria(FILE *f, ar_set_criteria_t *crit);
static boolean_t cond_print(FILE *f, char *str, boolean_t already_done);
static int write_global_dirs(FILE *f, ar_global_directive_t *g);

static int write_buffer_directive(FILE *f, char *name,
	buffer_directive_t *buf);

static int write_vsn_maps(FILE *arch_p, archiver_cfg_t *cfg);

static int parse_arch_cfg(ctx_t *ctx, char *location, archiver_cfg_t **cfg);

static int setup_archiver_fs_copy_maps(archiver_cfg_t *cfg);
static boolean_t fs_needs_default_map(archiver_cfg_t *cfg, fs_t *fs);
static int select_default_media_type(mtype_t mt, boolean_t *);
static int cfg_add_default_fs_vsn_map(archiver_cfg_t *cfg,
	uname_t fs_name, mtype_t mt);

static int set_fs_directive_indicators(archiver_cfg_t *cfg, sqm_lst_t *fs_list);
static int _get_ar_timeouts(sqm_lst_t **artimeout_lst);

void get_diskvols_ctx(ctx_t *ctx, ctx_t *dv_ctx);

/*
 * return true if the named set exists in the cfg.
 */
boolean_t
set_exists(
archiver_cfg_t *cfg,
set_name_t name)
{
	node_t *n, *o;
	ar_fs_directive_t *fs;
	ar_set_criteria_t *crit;


	if (strcmp(name, ALL_SETS) == 0) {
		return (B_TRUE);
	}

	/* look through file systems and fs_criteria */
	for (o = cfg->ar_fs_p->head; o != NULL; o = o->next) {
		fs = (ar_fs_directive_t *)o->data;
		if (strcmp(fs->fs_name, name) == 0) {
			return (B_TRUE);
		}

		for (n = fs->ar_set_criteria->head; n != NULL; n = n->next) {
			crit = (ar_set_criteria_t *)n->data;
			if (strcmp(crit->set_name, name) == 0) {
				return (B_TRUE);
			}
		}
	}

	/* look through global criteria */
	for (o = cfg->global_dirs.ar_set_lst->head; o != NULL;
	    o = o->next) {

		crit = (ar_set_criteria_t *)o->data;
		if (strcmp(crit->set_name, name) == 0) {
			return (B_TRUE);
		}
	}

	return (B_FALSE);
}


/*
 * get a list of all criteria with the indicated set_name
 */
int
cfg_get_criteria_by_name(
const archiver_cfg_t	*cfg,		/* cfg to search */
const char		*set_name,	/* name of set to get criteria for */
sqm_lst_t		**l)		/* malloced list of ar_set_criteria_t */
{

	node_t *out, *in;
	ar_set_criteria_t *crit;


	Trace(TR_DEBUG, "getting criteria by name for %s", Str(set_name));

	if (ISNULL(cfg, set_name, l)) {
		Trace(TR_OPRMSG, "getting criteria by name failed: %s",
		    samerrmsg);
		return (-1);
	}


	*l = lst_create();
	if (*l == NULL) {
		Trace(TR_OPRMSG, "getting criteria by name failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* search through criteria appending them to l */
	for (out = cfg->ar_fs_p->head; out != NULL; out = out->next) {
		ar_fs_directive_t *fs = (ar_fs_directive_t *)out->data;

		for (in = fs->ar_set_criteria->head; in != NULL;
		    in = in->next) {

			Trace(TR_OPRMSG, "compare set %s to crit %s", set_name,
			    ((ar_set_criteria_t *)in->data)->set_name);

			if (strcmp(((ar_set_criteria_t *)in->data)->set_name,
			    set_name) == 0) {

				if (dup_ar_set_criteria(in->data,
				    &crit) != 0) {

					free_ar_set_criteria_list(*l);
					Trace(TR_OPRMSG, "%s failed: %s",
					    "getting criteria by name",
					    samerrmsg);

					return (-1);
				}

				if (0 != lst_append(*l, crit)) {
					free_ar_set_criteria_list(*l);
					Trace(TR_OPRMSG, "%s failed: %s",
					    "getting criteria by name",
					    samerrmsg);
					return (-1);
				}
			}
		}
	}


	/* search through global criteria appending them to l */
	for (out = cfg->global_dirs.ar_set_lst->head;
	    out != NULL; out = out->next) {

		if (strcmp(((ar_set_criteria_t *)out->data)->set_name,
		    set_name) == 0) {

			if (dup_ar_set_criteria(out->data, &crit) != 0) {
				free_ar_set_criteria_list(*l);
				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting criteria by name", samerrmsg);
				return (-1);
			}

			if (0 != lst_append(*l, crit)) {
				free_ar_set_criteria_list(*l);
				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting criteria by name", samerrmsg);
				return (-1);
			}


		}
	}

	if ((*l)->length == 0) {

		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), set_name);

		lst_free(*l);

		Trace(TR_OPRMSG, "getting criteria by name failed: %s",
		    samerrmsg);

		return (-1);

	}

	Trace(TR_DEBUG, "got criteria by name found %d", (*l)->length);
	return (0);
}


/*
 * get all criteria for the fs.
 */
int
cfg_get_criteria_by_fs(
const archiver_cfg_t *cfg,	/* cfg to search */
const char *fs_name,		/* name of fs for which to get criteria */
sqm_lst_t **l)			/* malloced list of ar_set_criteria_t */
{

	ar_fs_directive_t *fs = NULL;


	Trace(TR_DEBUG, "getting criteria by fs for %s", Str(fs_name));

	if (ISNULL(cfg, fs_name, l)) {
		Trace(TR_OPRMSG, "getting criteria by fs failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* find and duplicate them for return */
	if (cfg_get_ar_fs(cfg, fs_name, B_TRUE, &fs) != 0) {
		/* leave samerrno as set */
		Trace(TR_OPRMSG, "getting criteria by fs failed: %s",
		    samerrmsg);

		return (-1);
	}
	*l = fs->ar_set_criteria;

	/* set criteria list to null so fs dir can be freed */
	fs->ar_set_criteria = NULL;
	free_ar_fs_directive(fs);
	Trace(TR_DEBUG, "got criteria by fs");
	return (0);

}


/*
 * get the fs directive for the named fs.
 */
int
cfg_get_ar_fs(
const archiver_cfg_t *cfg,	/* cfg to search */
const char *fs_name,		/* name of fs to get */
boolean_t malloc_return,	/* Either point into cfg or malloc */
ar_fs_directive_t **arch_fs)
{

	node_t *node = NULL;
	ar_fs_directive_t *tmp;


	Trace(TR_DEBUG, "getting ar_fs_directive for %s", Str(fs_name));

	if (ISNULL(cfg, fs_name, arch_fs)) {
		Trace(TR_OPRMSG, "getting ar_fs_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* find it and either duplicate it or return it directly */
	for (node = cfg->ar_fs_p->head; node != NULL; node = node->next) {
		ar_fs_directive_t *fs = (ar_fs_directive_t *)node->data;

		if (strcmp(fs->fs_name, fs_name) == 0) {
			if (malloc_return) {
				if (dup_ar_fs_directive(fs, &tmp) != 0) {

					Trace(TR_ERR, "getting ar_fs_directive"
					    "failed: %s", samerrmsg);
					return (-1);
				}

				*arch_fs = tmp;
			} else {
				*arch_fs = fs;
			}
			Trace(TR_DEBUG, "got ar_fs_directive from %s",
			    archiver_file);
			return (0);
		}
	}

	*arch_fs = NULL;
	samerrno = SE_NOT_FOUND;

	/* %s not found */
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(SE_NOT_FOUND),
	    fs_name);

	Trace(TR_OPRMSG, "getting ar_fs_directive failed: %s", samerrmsg);

	return (-1);
}


/*
 * get the copy params for the named copy.
 */
int
cfg_get_ar_set_copy_params(
const archiver_cfg_t *cfg,	/* cfg to search */
const char *copy_name,		/* name of copy for which to get params */
ar_set_copy_params_t **params)	/* malloced return */
{

	node_t *node;


	Trace(TR_DEBUG, "getting ar_set_copy_params for %s", Str(copy_name));

	if (ISNULL(cfg, copy_name, params)) {
		Trace(TR_OPRMSG, "getting ar_set_copy_params failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (cfg->archcopy_params == NULL) {
		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), copy_name);

		Trace(TR_OPRMSG, "getting ar_set_copy_params failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* find it, duplicate it for return */
	for (node = cfg->archcopy_params->head; node != NULL;
	    node = node->next) {
		ar_set_copy_params_t *p = (ar_set_copy_params_t *)node->data;
		if (strcmp(p->ar_set_copy_name, copy_name) == 0) {

			if (dup_ar_set_copy_params(p, params) != 0) {

				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting ar_set_copy_params", samerrmsg);

				return (-1);
			}

			Trace(TR_DEBUG, "got ar_set_copy_params");

			return (0);
		}
	}

	*params = NULL;
	samerrno = SE_NOT_FOUND;
	/* %s not found */
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(SE_NOT_FOUND),
	    copy_name);

	Trace(TR_OPRMSG, "getting ar_set_copy_params for %s failed: %s",
	    Str(copy_name), samerrmsg);

	return (-1);
}


/*
 * get the named vsn pool.
 */
int
cfg_get_vsn_pool(
const archiver_cfg_t *cfg,	/* cfg to search */
const char *pool_name,		/* name of pool to get */
vsn_pool_t **vsn_pool)		/* malloced return */
{

	node_t *node;


	Trace(TR_DEBUG, "getting vsn_pool %s", Str(pool_name));

	if (ISNULL(cfg, pool_name, vsn_pool)) {
		Trace(TR_OPRMSG, "getting vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	if (cfg->vsn_pools == NULL) {
		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), pool_name);

		Trace(TR_OPRMSG, "getting vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	/* find and duplicate the pool */
	for (node = cfg->vsn_pools->head; node != NULL; node = node->next) {
		vsn_pool_t *p = (vsn_pool_t *)node->data;
		if (strcmp(p->pool_name, pool_name) == 0) {

			if (dup_vsn_pool(p, vsn_pool) != 0) {
				Trace(TR_OPRMSG, "getting vsn_pool()%s%s",
				    " failed: ", samerrmsg);

				return (-1);
			}
			Trace(TR_DEBUG, "got vsn_pool");
			return (0);
		}
	}

	*vsn_pool = NULL;
	samerrno = SE_NOT_FOUND;
	/* %s not found */
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(SE_NOT_FOUND),
	    pool_name);

	Trace(TR_OPRMSG, "getting vsn_pool failed: %s", samerrmsg);
	return (-1);

}


/*
 * search all maps for the pool. Return true if the pool is in use.
 * Arguments must be non-null. This condition is not checked here.
 */
boolean_t
cfg_is_pool_in_use(
archiver_cfg_t *cfg,		/* cfg to search */
const uname_t pool_name,	/* name of pool to check */
uname_t used_by)		/* returns name of the copy using the pool */
{

	node_t *out;
	node_t *in;
	vsn_map_t *map;


	Trace(TR_DEBUG, "checking if pool is in use for %s", Str(pool_name));

	/* search all maps for any that includes the pool */
	for (out = cfg->vsn_maps->head; out != NULL; out = out->next) {
		map = (vsn_map_t *)out->data;

		if (map->vsn_pool_names != NULL) {
		for (in = map->vsn_pool_names->head; in != NULL;
		    in = in->next) {

			if (strcmp(pool_name, (char *)in->data) == 0) {
				strcpy(used_by,
				    ((vsn_map_t *)out->data)->ar_set_copy_name);

				Trace(TR_OPRMSG, "pool %s is in use",
				    pool_name);

				return (B_TRUE);
			}
		}
		}
	}

	Trace(TR_OPRMSG, "pool %s is not in use", pool_name);

	return (B_FALSE);
}


/*
 * get the vsn_map for the named copy.
 */
int
cfg_get_vsn_map(
const archiver_cfg_t *cfg,	/* cfg to search */
const char *copy_name,		/* copy for which to get the vsn_map_t */
vsn_map_t **vsn_map)		/* malloced return */
{

	node_t *n;


	Trace(TR_DEBUG, "getting vsn_map");
	if (ISNULL(cfg, copy_name, vsn_map)) {
		Trace(TR_OPRMSG, "getting vsn_map failed: %s", samerrmsg);
		return (-1);
	}

	/* find the map return a duplicate of it so cfg can safely be freed */
	for (n = cfg->vsn_maps->head; n != NULL; n = n->next) {
		if (strcmp(copy_name,
		    ((vsn_map_t *)n->data)->ar_set_copy_name) == 0) {

			if (dup_vsn_map((vsn_map_t *)n->data, vsn_map) != 0) {

				Trace(TR_OPRMSG, "getting vsn_map()%s%s",
				    " failed: ", samerrmsg);
				return (-1);
			}

			Trace(TR_DEBUG, "got vsn_map");
			return (0);
		}
	}

	*vsn_map = NULL;
	samerrno = SE_NOT_FOUND;

	/* %s not found */
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(SE_NOT_FOUND),
	    copy_name);

	Trace(TR_OPRMSG, "getting vsn_map failed: %s", samerrmsg);
	return (-1);
}


/*
 * get a list of all vsn_maps in the config.
 */
int
cfg_get_all_vsn_maps(
const archiver_cfg_t *cfg,	/* cfg to search */
sqm_lst_t **vsn_map_list)		/* malloced list of all vsn_map_t */
{

	node_t *node;


	Trace(TR_DEBUG, "getting all vsn_maps");
	if (ISNULL(cfg, vsn_map_list)) {
		Trace(TR_OPRMSG, "getting all vsn_maps failed: %s",
		    samerrmsg);
		return (-1);
	}

	*vsn_map_list = lst_create();
	if (*vsn_map_list == NULL) {
		Trace(TR_OPRMSG, "getting all vsn_maps failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (cfg->vsn_maps == NULL || cfg->vsn_maps->length == 0) {
		Trace(TR_DEBUG, "getting all vsn_maps");
		return (0);
	}

	/* duplicate the map list. */
	for (node = cfg->vsn_maps->head; node != NULL; node = node->next) {

		if (node->data != NULL) {
			vsn_map_t *mp = (vsn_map_t *)node->data;
			vsn_map_t *new_mp;

			if (dup_vsn_map(mp, &new_mp) != 0) {

				free_vsn_map_list(*vsn_map_list);
				*vsn_map_list = NULL;

				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting all vsn_maps", samerrmsg);

				return (-1);
			}
			if (lst_append(*vsn_map_list, new_mp) != 0) {
				free_vsn_map_list(*vsn_map_list);
				*vsn_map_list = NULL;
				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting all vsn_maps", samerrmsg);

				return (-1);
			}
		}
	}

	Trace(TR_DEBUG, "getting all vsn_maps");
	return (0);
}

/*
 * inserts a copy params in the appropriate place in the list.
 * ALL_SETS and ALL_SETS copies must come first and second relatively.
 * It would be nice if other copies were put in numerical order but there is
 * no guarantee that they will be.
 */
int
cfg_insert_copy_params(
sqm_lst_t *l,
ar_set_copy_params_t *cp)
{

	if (strncmp(cp->ar_set_copy_name, ALL_SETS, strlen(ALL_SETS)) == 0 &&
	    l->length != 0) {

		if (strlen(cp->ar_set_copy_name) == strlen(ALL_SETS)) {
			Trace(TR_DEBUG, "setting, inserting after head");

			if (lst_ins_before(l, l->head, cp) != 0) {
				goto err;
			}
		} else if (strlen(cp->ar_set_copy_name) ==
		    strlen(ALL_SETS".x") ||
		    strlen(cp->ar_set_copy_name) ==
		    strlen(ALL_SETS".xR")) {

			ar_set_copy_params_t *first_cp =
			    (ar_set_copy_params_t *)l->head->data;

			if (strcmp(first_cp->ar_set_copy_name,
			    ALL_SETS) == 0) {
				Trace(TR_DEBUG, "setting, insert after head");
				if (lst_ins_after(l, l->head, cp) != 0) {
					goto err;
				}
			} else {
				Trace(TR_DEBUG, "setting, insert before head");
				if (lst_ins_before(l, l->head, cp) != 0) {
					goto err;
				}
			}
		}
	} else {
		Trace(TR_DEBUG, "setting, appending");
		if (lst_append(l, cp) != 0) {
			goto err;
		}
	}

	Trace(TR_DEBUG, "inserted copy params into archiver cfg");

	return (0);
err:

	Trace(TR_ERR, "inserting copy params into archiver cfg failed: %s",
	    samerrmsg);
	return (-1);

}

/*
 * inserts a copy map in the appropriate place in the list.
 * ALL_SETS and ALL_SETS copies must come first and second relatively.
 * It would be nice if other copies were put in numerical order but there is
 * no guarantee that they will be.
 */
int
cfg_insert_copy_map(
sqm_lst_t *l,
vsn_map_t *m)
{

	Trace(TR_OPRMSG, "inserting copy map into archiver cfg");

	if (strncmp(m->ar_set_copy_name, ALL_SETS, strlen(ALL_SETS)) == 0 &&
	    l->length != 0) {

		if (strlen(m->ar_set_copy_name) == strlen(ALL_SETS)) {
			if (lst_ins_before(l, l->head, m) != 0) {
				goto err;
			}
		} else if (strlen(m->ar_set_copy_name) ==
		    strlen(ALL_SETS".x") ||
		    strlen(m->ar_set_copy_name) ==
		    strlen(ALL_SETS".xR")) {

			vsn_map_t *first_map = (vsn_map_t *)l->head->data;

			if (strcmp(first_map->ar_set_copy_name,
			    ALL_SETS) == 0) {

				if (lst_ins_after(l, l->head, m) != 0) {
					goto err;
				}
			} else {
				if (lst_ins_before(l, l->head, m) != 0) {
					goto err;
				}
			}
		}
	} else {
		if (lst_append(l, m) != 0) {
			goto err;
		}
	}

	Trace(TR_OPRMSG, "inserted copy map into archiver cfg");

	return (0);
err:

	Trace(TR_OPRMSG, "inserting copy map into archiver cfg failed: %s",
	    samerrmsg);
	return (-1);
}



/*
 * remove and free the copy map for the set.copy specified in name from
 * the cfg.
 */
int
cfg_remove_vsn_copy_map(
archiver_cfg_t *cfg,
const uname_t name)
{

	boolean_t found = B_FALSE;
	node_t *n;


	Trace(TR_OPRMSG, "removing vsn_copy_map for %s", Str(name));

	/* find it */
	for (n = cfg->vsn_maps->head; n != NULL; n = n->next) {
		if (strcmp(((vsn_map_t *)n->data)->ar_set_copy_name,
		    name) == 0) {

			found = B_TRUE;
			break;
		}
	}

	if (!found) {
		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), name);
		Trace(TR_OPRMSG, "removing vsn_copy_map failed: %s",
		    samerrmsg);
		return (-1);
	}

	/*
	 * free the map struct then remove the node
	 */
	free_vsn_map((vsn_map_t *)n->data);
	if (lst_remove(cfg->vsn_maps, n) != 0) {
		Trace(TR_OPRMSG, "removing vsn_copy_map failed: %s",
		    samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "removing vsn_copy_map");
	return (0);
}

/*
 * Read archive configuration from the default location and build the
 * archiver_cfg_t structure.
 */
int
read_arch_cfg(
ctx_t		*ctx,	/* contains the optional read_location to read from */
archiver_cfg_t	**cfg)	/* malloced return contains current config */
{
	upath_t alt_file;

	Trace(TR_DEBUG, "read archiver cfg");
	if (ISNULL(cfg)) {
		Trace(TR_ERR, "reading archiver cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (ctx != NULL && *ctx->read_location != '\0') {
		assemble_full_path(ctx->read_location, ARCH_DUMP_FILE,
		    B_FALSE, alt_file);

		if (parse_arch_cfg(ctx, alt_file, cfg) != 0) {
			Trace(TR_ERR, "reading archiver cfg failed: %s",
			    samerrmsg);
			return (-1);
		}

	} else if (parse_arch_cfg(NULL, archiver_file, cfg) != 0) {
		Trace(TR_ERR, "reading archiver cfg failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "read archiver cfg");
	return (0);
}


/*
 * private function to oversee parsing.
 */
static int
parse_arch_cfg(
ctx_t *ctx,
char *location,		/* path to archiver.cmd file to be parsed */
archiver_cfg_t **cfg)	/* malloced return containing current archiver cfg */
{

	diskvols_cfg_t	*dv_cfg = NULL;
	ctx_t		dv_ctx;
	archiver_cfg_t	*tmp_cfg;
	sqm_lst_t		*lib_list = NULL;
	sqm_lst_t		*fs_list = NULL;

	Trace(TR_FILES, "preparing to parse %s", Str(location));

	if (ISNULL(location, cfg)) {
		Trace(TR_OPRMSG, "preparing to parse failed: %s", samerrmsg);
		return (-1);
	}

	tmp_cfg = (archiver_cfg_t *)mallocer(sizeof (archiver_cfg_t));
	if (tmp_cfg == NULL) {
		Trace(TR_OPRMSG, "preparing to parse failed: %s", samerrmsg);

		return (-1);
	}
	memset(tmp_cfg, 0, sizeof (archiver_cfg_t));

	/* for now return if we can't successfully parse the mcf */
	if (cfg_get_all_fs(&fs_list) == -1) {
		Trace(TR_OPRMSG, "preparing to parse failed: %s", samerrmsg);
		free_archiver_cfg(tmp_cfg);
		return (-1);
	}

	if (get_all_libraries(NULL, &lib_list) == -1) {
		Trace(TR_OPRMSG, "preparing to parse failed: %s", samerrmsg);
		free_list_of_fs(fs_list);
		free_archiver_cfg(tmp_cfg);
		return (-1);
	}



	/*
	 * The following block of code is to support the GUI which couples
	 * the creation of a diskvol with the creation of an archive set.
	 * The end result is that we must be able to read from the diskvols
	 * that was written to an alternate location if one exists.
	 */
	get_diskvols_ctx(ctx, &dv_ctx);

	if (read_diskvols_cfg(&dv_ctx, &dv_cfg) != 0) {
		/* Don't return diskvols might not be needed. */
		Trace(TR_OPRMSG, "prep to parse, read diskvols failed:%s%s",
		    samerrmsg, " continuing ");
	}

	if (parse_archiver(location, fs_list, lib_list,
	    dv_cfg, tmp_cfg) != 0) {

		free_list_of_fs(fs_list);
		free_list_of_libraries(lib_list);
		free_diskvols_cfg(dv_cfg);
		free_archiver_cfg(tmp_cfg);
		Trace(TR_OPRMSG, "preparing to parse failed: %s", samerrmsg);
		return (-1);
	}


	if (set_fs_directive_indicators(tmp_cfg, fs_list) != 0) {

		free_list_of_fs(fs_list);
		free_list_of_libraries(lib_list);
		free_diskvols_cfg(dv_cfg);
		free_archiver_cfg(tmp_cfg);
		Trace(TR_OPRMSG, "preparing to parse failed: %s",
		    samerrmsg);
		return (-1);
	}

	free_list_of_fs(fs_list);
	free_list_of_libraries(lib_list);
	free_diskvols_cfg(dv_cfg);
	*cfg = tmp_cfg;
	Trace(TR_OPRMSG, "completed parsing archiver cfg");
	return (0);

}


/*
 * This function removes archiving information that pertains to shared
 * file systems. This is done to prevent the use of the GUI in configuring
 * shared sam-qfs which is not supported in 4.3.
 * This function will remove ar fs directives for the shared file systems.
 */
int
set_fs_directive_indicators(archiver_cfg_t *cfg, sqm_lst_t *fs_list) {
	node_t *n;
	node_t *dirnode;


	/* check each fs to see if it should be included */
	for (n = fs_list->head; n != NULL; n = n->next) {
		boolean_t is_set = B_TRUE;
		fs_t *fs = (fs_t *)n->data;
		ar_fs_directive_t *fsdir = NULL;


		/* find the fs directive for the fs */

		for (dirnode = cfg->ar_fs_p->head; dirnode != NULL;
		    dirnode = dirnode->next) {

			fsdir = (ar_fs_directive_t *)dirnode->data;
			if (strcmp(fs->fi_name, fsdir->fs_name) == 0) {
				break;
			}
			fsdir = NULL;
		}

		if (fsdir == NULL) {
			continue;
		}

		/*
		 * if this is not an archiving file system don't include it
		 * unless it has archiving configuration information set up
		 */
		if (fs->fi_archiving == B_FALSE) {
			int i;

			if (fsdir->change_flag == 0 &&
			    fsdir->ar_set_criteria->length == 0) {
				is_set = B_FALSE;
			}

			/*
			 * now check that there are no copies. If there
			 * are any we need to include the fs directive
			 */
			for (i = 0; i < MAX_COPY; i++) {
				if (fsdir->fs_copy[i] != NULL &&
				    fsdir->fs_copy[i]->change_flag != 0) {

					is_set = B_TRUE;
				}
			}

			if (!is_set) {
				fsdir->change_flag |= AR_FS_no_archive;
			}

		}

		/*
		 * if this is a shared file system flag it
		 */
		if (fs->fi_shared_fs) {
			fsdir->change_flag |= AR_FS_shared_fs;
		}

		fsdir = NULL;
	}

	return (0);
}

/*
 * This code is required to support the SAM/QFS Manager which combines
 * the creation of a diskvol with the creation of an archive set.
 * The end result is that if a dump is being performed we must be
 * able to conditionally read from the diskvols that was written
 * to an alternate location as part of the dump. This is complicated
 * by the fact that a dump may be being performed of something
 * that does not include any diskvols.
 */
void
get_diskvols_ctx(
ctx_t *ctx,
ctx_t *dv_ctx)
{

	struct stat	st;
	char		*dmp_loc;
	time_t		t;


	*dv_ctx->read_location = '\0';
	*dv_ctx->dump_path = '\0';

	/*
	 * Setup the read_location if this is a dump and there exists
	 * a dump of the diskvols.conf at the same location that is recent
	 */
	if (ctx != NULL && *ctx->dump_path != '\0') {
		dmp_loc = assemble_full_path(ctx->dump_path,
		    DISKVOLS_DUMP_FILE, B_FALSE, NULL);

		if (stat(dmp_loc, &st) == 0) {

			/*
			 * see if a diskvols.conf.dump file has been
			 * recently modified
			 */
			t = time(0);
			if ((t - 180) < st.st_mtime) {
#ifdef PRINTFDEBUG
				printf("size of dump_path is %ld",
				    sizeof (ctx->dump_path));
#endif

				strcpy(dv_ctx->read_location, ctx->dump_path);
			}
		}
	}

}


/*
 * verify that the archiver.cmd that would result from writing cfg is correct.
 */
int
verify_arch_cfg(
archiver_cfg_t *cfg)	/* archiver_cfg to be verified */
{
	archiver_cfg_t	*ver_cfg;
	sqm_lst_t		*l;
	char		ver_path[MAXPATHLEN+1];


	Trace(TR_DEBUG, "verifying archiver cfg");
	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "verifying archiver cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (mk_wc_path(ARCHIVER_CFG, ver_path, sizeof (ver_path)) != 0) {
		return (-1);
	}

	if (dump_arch_cfg(cfg, ver_path) != 0) {
		unlink(ver_path);
		Trace(TR_ERR, "dump failed in verify archiver cfg");
		return (-1);
	}
	if (parse_arch_cfg(NULL, ver_path, &ver_cfg) == 0) {
		free_archiver_cfg(ver_cfg);
		unlink(ver_path);

		Trace(TR_DEBUG, "verified archiver cfg");
		return (0);
	}

	if (samerrno == SE_CONFIG_CONTAINED_ERRORS) {
		samerrno = SE_VERIFY_FAILED;
		get_archiver_parsing_errors(&l);

		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_VERIFY_FAILED),
		    ((parsing_error_t *)l->head->data)->msg);

		lst_free_deep(l);
		unlink(ver_path);
		Trace(TR_ERR, "verifying archiver cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	unlink(ver_path);
	Trace(TR_ERR, "verifying archiver cfg failed: %s", samerrmsg);
	return (-1);
}


/*
 * write the archiver config to an alternate location.
 */
int
dump_arch_cfg(
archiver_cfg_t *cfg,	/* archiver_cfg to dump */
const char *location)	/* filename of file to dump to */
{

	Trace(TR_FILES, "dumping archiver cfg for %s", Str(location));

	if (ISNULL(cfg, location)) {
		Trace(TR_OPRMSG, "dumping archiver cfg failed: %s", samerrmsg);
		return (-1);
	}

	if (strcmp(location, ARCHIVER_CFG) == 0) {
		samerrno = SE_INVALID_DUMP_LOCATION;

		/* cannot dump the configuration to %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_DUMP_LOCATION), location);

		Trace(TR_OPRMSG, "dumping archiver cfg failed: %s", samerrmsg);
		return (-1);
	}



	if (write_archiver(location, cfg) != 0) {
		/* Leave the errors as set by write_archiver */

		Trace(TR_OPRMSG, "dumping archiver cfg failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "dumped archiver cfg");
	return (0);

}


/*
 * function to write the archiver_cfg to the default location.
 * If ctx is non-null and contains non-empty dump path the cfg will
 * be written to that path instead of the default location.
 */
int
write_arch_cfg(
ctx_t *ctx,		/* contains the optional dump path */
archiver_cfg_t *cfg,	/* the config to write */
const boolean_t force)	/* if true write the cfg even if backup fails */
{

	struct stat	st;
	boolean_t	dump_read = B_FALSE;

	Trace(TR_DEBUG, "writing archiver cfg");

	if (ISNULL(cfg)) {
		Trace(TR_OPRMSG, "writing archiver cfg failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * If this was done based on a dump read_location don't setup the
	 * initial archiver file. The stat will always fail so skip it.
	 */
	dump_read = ctx != NULL && *ctx->read_location != '\0';

	if (!dump_read && stat(archiver_file, &st) != 0) {
		Trace(TR_MISC, "writing archiver cfg stat failed: %s",
		    strerror(errno));

		/*
		 * if this write would setup the archiver.cmd for the first
		 * time. Setup the fs.1 vsn copy maps.
		 */
		if (ENOENT == errno) {
			if (setup_archiver_fs_copy_maps(cfg) != 0) {
				Trace(TR_OPRMSG,
				    "writing archiver cfg failed: %s",
				    samerrmsg);
				return (-1);
			}
		}
	}


	if (verify_arch_cfg(cfg) != 0) {
		/*
		 * Leave samerrno as set
		 */

		Trace(TR_OPRMSG, "writing archiver cfg failed: %s", samerrmsg);
		return (-1);
	}


	/* determine if this is a dump. */
	if (ctx != NULL && *ctx->dump_path != '\0') {
		char *dmp_loc;
		dmp_loc = assemble_full_path(ctx->dump_path, ARCH_DUMP_FILE,
		    B_FALSE, NULL);

		if (dump_arch_cfg(cfg, dmp_loc) != 0) {
			Trace(TR_OPRMSG, "writing archiver cfg failed: %s %s",
			    "dump_failed", samerrmsg);
			return (-1);
		} else {
			Trace(TR_FILES, "wrote archiver cfg to dump file %s",
			    dmp_loc);
			return (0);
		}
	}



	/* possibly backup the archiver.cmd (see backup_cfg for details) */
	if (backup_cfg(archiver_file) != 0 && !force) {
		/* leave samerrno as set */
		Trace(TR_OPRMSG, "writing archiver cfg failed: %s", samerrmsg);
		return (-1);
	}


	if (write_archiver(archiver_file, cfg) != 0) {
		Trace(TR_OPRMSG, "writing archiver cfg failed: %s", samerrmsg);
		return (-1);
	}

	/* always backup new archiver.cmd file Don't fail if it fails though */
	backup_cfg(archiver_file);

	Trace(TR_OPRMSG, "wrote archiver cfg");
	return (0);
}

/*
 * Do the actual business of writing the archiver file.
 */
static int
write_archiver(
const char *location,	/* location to write the archiver.cmd */
archiver_cfg_t *cfg)	/* the config to write */
{

	FILE		*f = NULL;
	time_t		the_time;
	char		err_buf[80];
	int		fd;

	Trace(TR_DEBUG, "writing archiver");

	if (ISNULL(location, cfg)) {
		Trace(TR_OPRMSG, "writing archiver failed: %s",
		    samerrmsg);
		return (-1);
	}

	if ((fd = open(location, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		f = fdopen(fd, "w");
	}
	if (f == NULL) {
		samerrno = errno;
		/* Open failed for %s: %s */
		StrFromErrno(samerrno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), location, err_buf);

		Trace(TR_OPRMSG, "writing archiver failed: %s",
		    samerrmsg);

		return (-1);
	}
	fprintf(f, "#\n#\tarchiver.cmd\n#");
	the_time = time(0);
	fprintf(f, "\n#\tGenerated by config api %s#", ctime(&the_time));



	write_global_dirs(f, &cfg->global_dirs);

	write_all_fs(f, cfg);
	write_ar_set_copy_params(f, cfg);
	write_vsn_pools(f, cfg);
	write_vsn_maps(f, cfg);
	fprintf(f, "\n");
	fclose(f);

	Trace(TR_FILES, "wrote archiver %s", location);
	return (0);
}


/*
 * write the global directives.
 */
static int
write_global_dirs(
FILE *f,
ar_global_directive_t *g)	/* globals to write */
{

	node_t *node;
	buffer_directive_t *buf;
	drive_directive_t *drv;

	char *global_dir_head = "\n#\n#\tGlobal Directives\n#\n#";
	boolean_t already_done = B_FALSE;

	if (g->change_flag & AR_GL_wait && g->wait != flag_reset) {
		already_done = cond_print(f, global_dir_head, already_done);
		fprintf(f, "\nwait");
	}

	if (g->change_flag & AR_GL_ar_interval &&
	    g->ar_interval != uint_reset) {

		already_done = cond_print(f, global_dir_head, already_done);
		fprintf(f, "\ninterval = %s", StrFromInterval(g->ar_interval,
		    NULL, 0));
	}

	if (g->change_flag & AR_GL_notify_script &&
	    *(g->notify_script) != char_array_reset) {

		already_done = cond_print(f, global_dir_head, already_done);
		fprintf(f, "\nnotify = %s", g->notify_script);
	}


	if (g->change_flag & AR_GL_log_path &&
	    *(g->log_path) != char_array_reset) {

		already_done = cond_print(f, global_dir_head, already_done);
		fprintf(f, "\nlogfile = %s", g->log_path);
	}

	if (g->change_flag & AR_GL_scan_method &&
	    g->scan_method != EM_NOT_SET && g->scan_method != enum_reset) {

		already_done = cond_print(f, global_dir_head, already_done);
		fprintf(f, "\nexamine = %s", ExamMethodNames[g->scan_method]);
	}

	if (g->change_flag & AR_GL_archivemeta) {
		already_done = cond_print(f, global_dir_head, already_done);
		fprintf(f, "\narchivemeta = %s",
		    g->archivemeta ? "on" : "off");
	}

	if (g->change_flag & AR_GL_scan_squash) {
		already_done = cond_print(f, global_dir_head, already_done);
		fprintf(f, "\nscanlist_squash = %s",
		    (g->options & SCAN_SQUASH_ON) ? "on" : "off");
	}

	if (g->change_flag & AR_GL_setarchdone) {
		already_done = cond_print(f, global_dir_head, already_done);
		fprintf(f, "\nsetarchdone = %s",
		    (g->options & SETARCHDONE_ON) ? "on" : "off");
	}

	/* if any timeouts print them */
	if (g->timeouts != NULL) {
		for (node = g->timeouts->head; node != NULL;
		    node = node->next) {

			char *tmout = (char *)node->data;

			if (tmout == NULL || *tmout == '\0') {
				continue;
			}
			already_done = cond_print(f, global_dir_head,
			    already_done);

			fprintf(f, "\n%s", tmout);
		}
	}

	/* if any ar_bufs print them */
	if (g->ar_bufs != NULL) {
		for (node = g->ar_bufs->head;
		    node != NULL; node = node->next) {

			buf = (buffer_directive_t *)node->data;

			if (buf == NULL) {
				return (-1);
			}

			if (buf->change_flag == 0) {
				continue;
			}

			already_done = cond_print(f, global_dir_head,
			    already_done);

			if (buf->change_flag & BD_lock && buf->lock) {
				if (buf->size != fsize_reset &&
				    buf->change_flag & BD_size) {
					/*
					 * if lock is set and size is set
					 * print them.
					 */
					fprintf(f, "\nbufsize = %s %llu lock",
					    buf->media_type, buf->size);

				} else {
					/*
					 * if lock is set and size is not
					 * print the default size with lock
					 */
					fprintf(f, "\nbufsize = %s %d lock",
					    buf->media_type,
					    DEFAULT_AR_BUFSIZE);
				}

			} else if (buf->size != fsize_reset &&
			    buf->change_flag & BD_size) {


				/*
				 * if lock is not set and size is set
				 * print the size
				 */
				fprintf(f, "\nbufsize = %s %llu",
				    buf->media_type, buf->size);

			}
			/*
			 * if lock is not set and size
			 * is not set print nothing
			 */

		}
	}

	/* if any archmax print them */
	if (g->ar_max != NULL) {
		for (node = g->ar_max->head;
		    node != NULL; node = node->next) {

			buf = (buffer_directive_t *)node->data;
			if (buf == NULL)
				return (-1);

			if (buf->change_flag == 0) {
				continue;
			}

			already_done = cond_print(f,
			    global_dir_head, already_done);

			write_buffer_directive(f, "archmax", buf);
		}
	}

	/* if any ar_drives print them */
	if (g->ar_drives != NULL) {
		for (node = g->ar_drives->head;
		    node != NULL; node = node->next) {

			drv = (drive_directive_t *)node->data;
			if (drv == NULL) {
				return (-1);
			}

			if (drv->change_flag == 0) {
				continue;
			}

			already_done = cond_print(f,
			    global_dir_head, already_done);

			fprintf(f, "\ndrives = %s %d", drv->auto_lib,
			    drv->count);
		}
	}

	if (g->ar_overflow_lst != NULL) {
		for (node = g->ar_overflow_lst->head;
		    node != NULL; node = node->next) {

			buf = (buffer_directive_t *)node->data;
			if (buf == NULL)
				return (-1);

			if (buf->change_flag == 0) {
				continue;
			}

			already_done = cond_print(f, global_dir_head,
			    already_done);

			write_buffer_directive(f, "ovflmin", buf);
		}
	}

	if (g->ar_set_lst != NULL) {
		for (node = g->ar_set_lst->head;
		    node != NULL; node = node->next) {

			write_ar_set_criteria(f,
			    (ar_set_criteria_t *)node->data);
		}
	}
	return (0);
}


/*
 * write the buffer directive. This function does not support writing
 * the lock member.
 */
static int
write_buffer_directive(
FILE *f,
char *name,			/* "archmax" | "bufsize" | "ovflmin" */
buffer_directive_t *buf)	/* buffer_directive to write */
{

	char fsizestr[FSIZE_STR_LEN];

	if (buf->size == fsize_reset && !(buf->change_flag & BD_size)) {
		return (0);
	}

	fprintf(f, "\n%s = %s %s", name, buf->media_type,
	    fsize_to_str(buf->size, fsizestr, FSIZE_STR_LEN));


	return (0);
}


/*
 * write all ar_fs_directives to the file
 */
static int
write_all_fs(
FILE *f,
archiver_cfg_t *cfg)
{

	node_t *node;

	Trace(TR_DEBUG, "writing all fs");
	for (node = cfg->ar_fs_p->head; node != NULL; node = node->next) {
		write_ar_fs_directive_t(f, (ar_fs_directive_t *)node->data);
	}

	return (0);
}


/*
 * write a single ar_fs_directive
 */
static int
write_ar_fs_directive_t(
FILE *f,
ar_fs_directive_t *fs)
{

	node_t *node;
	boolean_t printed = B_FALSE;
	char name_line[sizeof (uname_t) + 37] =  "";
	char *copy_str = NULL;


	Trace(TR_DEBUG, "writing ar_fs_directive");

	if (*(fs->fs_name) != '\0' && strcmp(fs->fs_name, GLOBAL) != 0) {
		snprintf(name_line, sizeof (name_line),
		    "\n#\n#\n#\tFile System Directives\n#\nfs = %s",
		    fs->fs_name);
	}

	copy_str = get_copy_cfg_str(fs->fs_copy);
	if (strlen(copy_str) != 0) {
		printed = cond_print(f, name_line, printed);
		fprintf(f, "%s\n", copy_str);
	}


	if (fs->change_flag & AR_FS_archivemeta) {
		printed = cond_print(f, name_line, printed);
		fprintf(f, "\narchivemeta = %s",
		    fs->archivemeta ? "on" : "off");
	}

	if (fs->change_flag & AR_FS_wait && fs->wait != flag_reset) {
		printed = cond_print(f, name_line, printed);
		fprintf(f, "\nwait");
	}

	if (fs->change_flag & AR_FS_scan_method &&
	    fs->scan_method != EM_NOT_SET &&
	    fs->scan_method != enum_reset) {

		printed = cond_print(f, name_line, printed);
		fprintf(f, "\nexamine = %s", ExamMethodNames[fs->scan_method]);
	}

	if (fs->change_flag & AR_FS_log_path &&
	    *(fs->log_path) != char_array_reset) {
		printed = cond_print(f, name_line, printed);
		fprintf(f, "\nlogfile = %s", fs->log_path);
	}

	if (fs->change_flag & AR_FS_fs_interval &&
	    fs->fs_interval != uint_reset) {

		printed = cond_print(f, name_line, printed);
		fprintf(f, "\ninterval = %s", StrFromInterval(fs->fs_interval,
		    NULL, 0));
	}

	if (fs->change_flag & AR_FS_scan_squash) {
		printed = cond_print(f, name_line, printed);
		fprintf(f, "\nscanlist_squash = %s",
		    (fs->options & SCAN_SQUASH_ON) ? "on" : "off");
	}
	if (fs->change_flag & AR_FS_setarchdone) {
		printed = cond_print(f, name_line, printed);
		fprintf(f, "\nsetarchdone = %s",
		    (fs->options & SETARCHDONE_ON) ? "on" : "off");
	}

	if (fs->ar_set_criteria != NULL && fs->ar_set_criteria->length != 0) {
		printed = cond_print(f, name_line, printed);
		for (node = fs->ar_set_criteria->head; node != NULL;
		    node = node->next) {

			write_ar_set_criteria(f,
			    (ar_set_criteria_t *)node->data);
		}
	}


	cond_print(f, "\n#\n#", !printed);

	return (0);
}


/*
 * write the ar_set_criteria to the file
 */
static int
write_ar_set_criteria(
FILE *f,
ar_set_criteria_t *crit)
{

	char *str;

	Trace(TR_DEBUG, "writing ar_set_criteria");
	str = criteria_to_str(crit);
	if (*str == '\0') {
		return (-1);
	}
	fprintf(f, "\n%s\n", str);

	/* put the new key into the criteria */
	get_key(str, strlen(str), &(crit->key));
	Trace(TR_OPRMSG, "wrote ar_set_criteria classname = %s",
	    Str(crit->class_name));


	return (0);
}


/*
 * write all vsn pools
 */
static int
write_vsn_pools(
FILE *arch_p,
archiver_cfg_t *cfg)
{

	node_t *out;
	node_t *in;
	vsn_pool_t *p;
	char *pool_head = "\n#\n#\n#\t VSN Pool Directives\n#\nvsnpools";

	Trace(TR_DEBUG, "writing vsn_pools");
	if (cfg->vsn_pools == NULL || cfg->vsn_pools->length == 0) {
		return (0);
	}

	fprintf(arch_p, pool_head);

	for (out = cfg->vsn_pools->head; out != NULL; out = out->next) {
		p = (vsn_pool_t *)out->data;

		fprintf(arch_p, "\n%s %s", p->pool_name, p->media_type);
		for (in = p->vsn_names->head;
		    in != NULL; in = in->next) {

			fprintf(arch_p, " %s", (char *)in->data);
		}

	}
	fprintf(arch_p, "\nendvsnpools");

	Trace(TR_DEBUG, "wrote vsn_pools");
	return (0);
}

/*
 * write all vsn maps.
 */
static int
write_vsn_maps(
FILE *arch_p,
archiver_cfg_t *cfg)
{

	node_t *out;
	node_t *in;
	vsn_map_t *map;
	boolean_t alreadyDone = B_FALSE;
	char *nm;
	char *vsn_head = "\n#\n#\n#\tVSN Directives\n#\nvsns";


	if (cfg->vsn_maps == NULL || cfg->vsn_maps->length == 0) {
		return (0);
	}



	Trace(TR_DEBUG, "writing vsn_maps");
	for (out = cfg->vsn_maps->head; out != NULL; out = out->next) {
		map = (vsn_map_t *)out->data;
		if (map == NULL) {
			continue;
		}

		alreadyDone = cond_print(arch_p, vsn_head, alreadyDone);
		nm = map->ar_set_copy_name;

		if ((map->vsn_names != NULL && map->vsn_names->length != 0) ||
		    (map->vsn_pool_names != NULL &&
		    map->vsn_pool_names->length != 0)) {

			fprintf(arch_p, "\n%s %s", nm, map->media_type);
		}

		if (map->vsn_names != NULL) {
			for (in = map->vsn_names->head;
			    in != NULL; in = in->next) {

				fprintf(arch_p, " %s", (char *)in->data);
			}
		}

		if (map->vsn_pool_names != NULL) {
			for (in = map->vsn_pool_names->head;
			    in != NULL; in = in->next) {

				fprintf(arch_p, " -pool %s", (char *)in->data);
			}
		}
	}
	cond_print(arch_p, "\nendvsns", !alreadyDone);

	Trace(TR_DEBUG, "wrote vsn_maps");
	return (0);
}


/*
 * conditionally prints the str if already_done = false.
 * always returns true.
 */
static boolean_t
cond_print(
FILE *f,
char *str,
boolean_t already_done)
{

	if (already_done) {
		return (already_done);
	} else {
		fprintf(f, str);
		return (B_TRUE);
	}
}


/*
 * write an ar_set_copy_params
 */
static int
write_ar_set_copy_params(
FILE *arch_p,
archiver_cfg_t *cfg)
{

	node_t *node;
	node_t *node2;
	ar_set_copy_params_t *a = NULL;
	priority_t *pri;
	int i = 0;
	int j;
	char *name;
	boolean_t done = B_FALSE;
	char *param_head = "\n#\t Copy Parameters Directives\n#\nparams";
	char fsizestr[FSIZE_STR_LEN];

	if (cfg->archcopy_params == NULL) {
		Trace(TR_DEBUG, "writing ar_set_copy_params exit null list");
		return (0);
	}

	if (cfg->archcopy_params->length == 0) {
		Trace(TR_DEBUG, "writing ar_set_copy_params exit empty list");
		return (0);
	}

	for (node = cfg->archcopy_params->head;
	    node != NULL;
	    node = node->next) {

		a = (ar_set_copy_params_t *)node->data;
		name = a->ar_set_copy_name;

		if (a->change_flag &  AR_PARAM_archmax &&
		    a->archmax != fsize_reset) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -archmax %s", name,
			    fsize_to_str(a->archmax,
			    fsizestr, FSIZE_STR_LEN));
		}
		if (a->change_flag &  AR_PARAM_bufsize &&
		    a->bufsize != int_reset) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -bufsize %d", name, a->bufsize);
		}
		if (a->change_flag &  AR_PARAM_buflock && a->buflock) {
			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -lock", name);
		}
		if (a->change_flag &  AR_PARAM_drives &&
		    a->drives != int_reset) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -drives %d", name, a->drives);
		}
		if (a->change_flag &  AR_PARAM_drivemin &&
		    a->drivemin != fsize_reset) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -drivemin %s", name,
			    fsize_to_str(a->drivemin,
			    fsizestr, FSIZE_STR_LEN));
		}

		if (a->change_flag &  AR_PARAM_drivemax &&
		    a->drivemax != fsize_reset) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -drivemax %s", name,
			    fsize_to_str(a->drivemax,
			    fsizestr, FSIZE_STR_LEN));
		}
		if (a->change_flag &  AR_PARAM_ovflmin &&
		    a->ovflmin != fsize_reset) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -ovflmin %s", name,
			    fsize_to_str(a->ovflmin,
			    fsizestr, FSIZE_STR_LEN));
		}

		if (a->change_flag &  AR_PARAM_fillvsns && a->fillvsns) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -fillvsns", name);
		}

		if (a->change_flag &  AR_PARAM_tapenonstop && a->tapenonstop) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -tapenonstop", name);
		}

		if (a->change_flag & AR_PARAM_rearch_stage_copy &&
		    a->rearch_stage_copy != 0) {
			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -rearch_stage_copy %d", name,
			    a->rearch_stage_copy);
		}

		if (a->change_flag &  AR_PARAM_reserve && a->reserve) {
			for (j = 0; Reserves[j].RsName[0] != '\0'; j++) {

				if (a->reserve & Reserves[j].RsValue) {

					done = cond_print(arch_p,
					    param_head, done);

					fprintf(arch_p, "\n%s -reserve %s",
					    name, Reserves[j].RsName);
				}
			}
		}
		if (a->change_flag & AR_PARAM_unarchage) {
			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -unarchage %s", name,
			    a->unarchage ? "modify" : "access");
		}

		if (a->change_flag & AR_PARAM_directio) {
			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -directio %s", name,
			    a->directio ? "on" : "off");
		}

		if (a->recycle.change_flag &  RC_hwm && a->recycle.hwm > 0) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -recycle_hwm %d",
			    name, a->recycle.hwm);
		}
		if (a->recycle.change_flag & RC_data_quantity &&
		    a->recycle.data_quantity != fsize_reset) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -recycle_dataquantity %s", name,
			    fsize_to_str(a->recycle.data_quantity,
			    fsizestr, FSIZE_STR_LEN));
		}
		if (a->recycle.change_flag & RC_ignore &&
		    a->recycle.ignore) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -recycle_ignore", name);
		}


		if (a->recycle.change_flag & RC_email_addr &&
		    a->recycle.email_addr[0] != '\0') {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -recycle_mailaddr %s", name,
			    a->recycle.email_addr);
		}
		if (a->recycle.change_flag & RC_mingain &&
		    a->recycle.mingain > 0) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -recycle_mingain %d", name,
			    a->recycle.mingain);
		}

		if (a->recycle.change_flag & RC_vsncount &&
		    a->recycle.vsncount > 0) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -recycle_vsncount %d", name,
			    a->recycle.vsncount);
		}

		if (a->recycle.change_flag & RC_minobs &&
		    a->recycle.minobs > 0) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -recycle_minobs %d", name,
			    a->recycle.minobs);
		}


		/* Priorities */
		if (a->priority_lst != NULL) {
			for (node2 = a->priority_lst->head;
			    node2 != NULL;
			    node2 = node2->next) {

				pri = (priority_t *)node2->data;
				done = cond_print(arch_p, param_head, done);

				fprintf(arch_p, "\n%s -priority %s %.2f",
				    name, pri->priority_name, pri->value);
			}
		}
		if (a->change_flag & AR_PARAM_join && a->join != enum_reset) {

			for (i = 0; Joins[i].EeName != NULL; i++) {
				if (a->join == Joins[i].EeValue) {

					done = cond_print(arch_p, param_head,
					    done);

					fprintf(arch_p, "\n%s -join %s", name,
					    Joins[i].EeName);
					break;
				}
			}
		}

		if (a->change_flag & AR_PARAM_sort && a->sort != enum_reset) {

			for (i = 0; Sorts[i].EeName != NULL; i++) {
				if (a->sort == Sorts[i].EeValue) {

					done = cond_print(arch_p, param_head,
					    done);

					fprintf(arch_p, "\n%s -sort %s", name,
					    Sorts[i].EeName);
					break;
				}
			}
		}

		if (a->change_flag & AR_PARAM_rsort &&
		    a->rsort != enum_reset) {

			for (i = 0; Sorts[i].EeName != NULL; i++) {
				if (a->rsort == Sorts[i].EeValue) {
					done = cond_print(arch_p, param_head,
					    done);

					fprintf(arch_p, "\n%s -rsort %s", name,
					    Sorts[i].EeName);

					break;
				}
			}
		}


		if (a->change_flag & AR_PARAM_offline_copy &&
		    a->offline_copy != enum_reset) {

			for (i = 0; OfflineCopies[i].EeName != NULL; i++) {

				if (a->offline_copy ==
				    OfflineCopies[i].EeValue) {

					done = cond_print(arch_p, param_head,
					    done);

					fprintf(arch_p,
					    "\n%s -offline_copy %s", name,
					    OfflineCopies[i].EeName);

					break;
				}
			}
		}

		if (a->change_flag &  AR_PARAM_startage &&
		    a->startage != uint_reset) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -startage %s", name,
			    StrFromInterval(a->startage, NULL, 0));
		}

		if (a->change_flag &  AR_PARAM_startcount &&
		    a->startcount != int_reset) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -startcount %d", name,
			    a->startcount);
		}

		if (a->change_flag &  AR_PARAM_startsize &&
		    a->startsize != fsize_reset) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -startsize %s", name,
			    fsize_to_str(a->startsize,
			    fsizestr, FSIZE_STR_LEN));
		}
		if (a->change_flag & AR_PARAM_queue_time_limit &&
		    a->queue_time_limit != 0) {
			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -queue_time_limit %s", name,
			    StrFromInterval(a->queue_time_limit, fsizestr,
			    FSIZE_STR_LEN));
		}

		if (a->change_flag &  AR_PARAM_tstovfl && a->tstovfl) {
			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -tstovfl", name);
		}


		if (a->change_flag &  AR_PARAM_simdelay &&
		    a->simdelay != uint_reset) {

			done = cond_print(arch_p, param_head, done);
			fprintf(arch_p, "\n%s -simdelay %s", name,
			    StrFromInterval(a->simdelay, NULL, 0));
		}


	}



	cond_print(arch_p, "\nendparams", !done);

	return (0);
}

/*
 * Functions to duplicate structures
 */


/*
 * duplicate the incoming struct. The resulting out structure
 * is dynamically allocated so it can be freed.
 */
int
dup_ar_fs_directive(
const ar_fs_directive_t *in,
ar_fs_directive_t **out)	/* malloced copy of in */
{

	sqm_lst_t *tmp;
	int i;


	Trace(TR_DEBUG, "duplicating ar_fs_directive");
	if (ISNULL(in, out)) {
		Trace(TR_DEBUG, "duplicating ar_fs_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	*out = (ar_fs_directive_t *)mallocer(sizeof (ar_fs_directive_t));
	if (*out == NULL) {
		Trace(TR_DEBUG, "duplicating ar_fs_directive failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* copy field data over. */
	memcpy(*out, in, sizeof (ar_fs_directive_t));

	/*
	 * set pointers to null so free wont free anything from in if
	 * any errors are encountered.
	 */
	(*out)->ar_set_criteria = NULL;
	for (i = 0; i < MAX_COPY; i++) {
		(*out)->fs_copy[i] = NULL;
	}

	/* now dup the criteria list */
	if (dup_ar_set_criteria_list(in->ar_set_criteria, &tmp) != 0) {
		Trace(TR_DEBUG, "duplicating ar_fs_directive failed: %s",
		    samerrmsg);

		free_ar_fs_directive(*out);
		return (-1);
	}

	(*out)->ar_set_criteria = tmp;


	/* dup the copies. */
	for (i = 0; i < MAX_COPY; i++) {
		if (in->fs_copy[i] != NULL) {
			ar_set_copy_cfg_t *tmp_cpy;

			if (dup_ar_set_copy_cfg(in->fs_copy[i],
			    &tmp_cpy) != 0) {

				Trace(TR_DEBUG, "duplicating %s%s",
				    " ar_fs_directive failed: ", samerrmsg);

				free_ar_fs_directive(*out);
				return (-1);
			}
			(*out)->fs_copy[i] = tmp_cpy;
		}
	}




	Trace(TR_DEBUG, "duplicated ar_fs_directive");
	return (0);
}


/*
 * duplicate the incoming struct. The resulting out structure
 * is dynamically allocated so it can be freed.
 */
int
dup_ar_set_copy_cfg(
const ar_set_copy_cfg_t *in,
ar_set_copy_cfg_t **out)	/* malloced copy of in */
{

	Trace(TR_DEBUG, "duplicating ar_set_copy_cfg");

	if (ISNULL(in, out)) {
		Trace(TR_DEBUG, "duplicating ar_set_copy_cfg failed: %s",
		    samerrmsg);
		return (-1);
	}


	*out = (ar_set_copy_cfg_t *)mallocer(sizeof (ar_set_copy_cfg_t));
	if (*out == NULL) {
		Trace(TR_DEBUG, "duplicating ar_set_copy_cfg failed: %s",
		    samerrmsg);
		return (-1);
	}

	memcpy(*out, in, sizeof (ar_set_copy_cfg_t));

	Trace(TR_DEBUG, "duplicated ar_set_copy_cfg");
	return (0);
}


/*
 * duplicate the incoming struct. The resulting out structure
 * is dynamically allocated so it can be freed.
 */
int
dup_ar_set_criteria_list(
const sqm_lst_t *in,
sqm_lst_t **out)		/* malloced copy of in */
{

	node_t *n;
	ar_set_criteria_t *tmp;


	Trace(TR_DEBUG, "duplicating ar_set_criteria list");

	if (ISNULL(in, out)) {
		Trace(TR_DEBUG, "duplicating ar_set_criteria list failed: %s",
		    samerrmsg);
		return (-1);
	}

	*out = lst_create();
	if (*out == NULL) {

		Trace(TR_DEBUG, "duplicating ar_set_criteria list failed: %s",
		    samerrmsg);

		return (-1);
	}

	for (n = in->head; n != NULL; n = n->next) {
		if (n->data != NULL) {

			if (dup_ar_set_criteria(n->data, &tmp) != 0) {
				free_ar_set_criteria_list(*out);
				Trace(TR_DEBUG, "%s failed: %s",
				    "duplicating ar_set_criteria list",
				    samerrmsg);

				return (-1);
			}
			if (lst_append(*out, tmp) != 0) {
				free_ar_set_criteria_list(*out);
				Trace(TR_DEBUG, "%s failed: %s",
				    "duplicating ar_set_criteria list",
				    samerrmsg);

				return (-1);
			}
		}
	}
	Trace(TR_DEBUG, "duplicated ar_set_criteria list");
	return (0);
}

/*
 * duplicate the incoming struct. The resulting out structure
 * is dynamically allocated so it can be freed.
 */
int
dup_ar_set_criteria(
const ar_set_criteria_t *in,
ar_set_criteria_t **out)	/* malloced copy of in */
{
	int i;

	Trace(TR_DEBUG, "duplicating ar_set_criteria");

	if (ISNULL(in, out)) {
		Trace(TR_DEBUG, "duplicating ar_set_criteria failed: %s",
		    samerrmsg);
		return (-1);
	}

	*out = (ar_set_criteria_t *)mallocer(sizeof (ar_set_criteria_t));
	if (*out == NULL) {

		Trace(TR_DEBUG, "duplicating ar_set_criteria failed: %s",
		    samerrmsg);

		return (-1);
	}

	/* copy data over. */
	memcpy(*out, in, sizeof (ar_set_criteria_t));

	/* set pointers to null so free can succeed w/o impact to in */
	for (i = 0; i < MAX_COPY; i++) {
		(* out)->arch_copy[i] = NULL;
	}

	/* set copies. */
	for (i = 0; i < MAX_COPY; i++) {
		if (in->arch_copy[i] != NULL) {
			if (dup_ar_set_copy_cfg(in->arch_copy[i],
			    &((*out)->arch_copy[i])) != 0) {

				free_ar_set_criteria(*out);
				Trace(TR_DEBUG, "%s failed: %s",
				    "duplicating ar_set_criteria",
				    samerrmsg);

				return (-1);
			}
		}
	}

	/* If the source description is non-null duplicate it */
	if (in->description != NULL) {
		(*out)->description = strdup(in->description);
	}

	Trace(TR_DEBUG, "duplicated ar_set_criteria");
	return (0);
}


/*
 * duplicate the incoming struct. The resulting out structure
 * is dynamically allocated so it can be freed.
 */
int
dup_ar_set_copy_params(
const ar_set_copy_params_t *in,
ar_set_copy_params_t **out)	/* malloced copy of in */
{


	Trace(TR_DEBUG, "duplicating ar_set_copy_params");

	if (ISNULL(in, out)) {
		Trace(TR_DEBUG, "duplicating ar_set_copy_params failed: %s",
		    samerrmsg);
		return (-1);
	}

	*out = (ar_set_copy_params_t *)mallocer(sizeof (ar_set_copy_params_t));
	if (*out == NULL) {

		Trace(TR_DEBUG, "duplicating ar_set_copy_params failed: %s",
		    samerrmsg);

		return (-1);
	}

	memset(*out, 0, sizeof (ar_set_copy_params_t));
	if (cp_ar_set_copy_params(in, *out) != 0) {
		Trace(TR_DEBUG, "duplicating ar_set_copy_params failed: %s",
		    samerrmsg);

		return (-1);
	}

	return (0);
}


int
cp_ar_set_copy_params(
const ar_set_copy_params_t *in,
ar_set_copy_params_t *out)	/* malloced copy of in */
{

	node_t *n;

	/* copy data over */
	memcpy(out, in, sizeof (ar_set_copy_params_t));

	/* set ptrs to null so out can be freed if errors occur */
	(out)->priority_lst = NULL;


	(out)->priority_lst = lst_create();
	if ((out)->priority_lst == NULL) {
		return (-1);
	}


	if (in->priority_lst != NULL) {
	for (n = in->priority_lst->head; n != NULL; n = n->next) {

		if (n->data != NULL) {
			priority_t *p;
			p = (priority_t *)mallocer(sizeof (priority_t));
			if (p == NULL) {
				free_ar_set_copy_params(out);
				return (-1);
			}


			strcpy(p->priority_name,
			    ((priority_t *)n->data)->priority_name);

			p->value = ((priority_t *)n->data)->value;
			if (lst_append((out)->priority_lst, p) != 0) {
				return (0);
			}
		}
	}
	}


	return (0);
}


/*
 * duplicate the incoming struct. The resulting out structure
 * is dynamically allocated so it can be freed.
 */
int
dup_vsn_pool(
const vsn_pool_t *in,
vsn_pool_t **out)	/* malloced copy of in */
{

	Trace(TR_DEBUG, "duplicating vsn_pool");

	if (ISNULL(in, out)) {
		Trace(TR_DEBUG, "duplicating vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	*out = (vsn_pool_t *)mallocer(sizeof (vsn_pool_t));
	if (*out == NULL) {

		Trace(TR_DEBUG, "duplicating vsn_pool failed: %s", samerrmsg);
		return (-1);
	}

	/* copy the data */
	memcpy(*out, in, sizeof (vsn_pool_t));

	/* set ptr to null so free can work if errors encountered. */
	(*out)->vsn_names = NULL;

	if (dup_string_list(in->vsn_names, &((*out)->vsn_names)) != 0) {

		free_vsn_pool(*out);

		Trace(TR_DEBUG, "duplicating vsn_pool failed: %s", samerrmsg);
		return (-1);
	}
	Trace(TR_DEBUG, "duplicated vsn_pool");
	return (0);
}

/*
 * duplicate the incoming struct. The resulting out structure
 * is dynamically allocated so it can be freed.
 */
int
dup_vsn_map(
const vsn_map_t *in,
vsn_map_t **out)	/* malloced copy of in */
{

	Trace(TR_DEBUG, "duplicating vsn_map");

	if (ISNULL(in, out)) {
		Trace(TR_DEBUG, "duplicating vsn_map failed: %s", samerrmsg);
		return (-1);
	}

	*out = (vsn_map_t *)mallocer(sizeof (vsn_map_t));
	if (*out == NULL) {
		Trace(TR_DEBUG, "duplicating vsn_map failed: %s", samerrmsg);
		return (-1);
	}


	memset(*out, 0, sizeof (vsn_map_t));
	if (cp_vsn_map(in, *out) != 0) {
		free_vsn_map(*out);
		Trace(TR_DEBUG, "duplicating vsn_map failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "duplicated vsn_map");
	return (0);
}


int
cp_vsn_map(
const vsn_map_t *in,
vsn_map_t *out)	/* copy of in */
{
	/* copy basic data */
	memcpy(out, in, sizeof (vsn_map_t));

	/* set pointers to null so free can work if an error is encountered. */
	out->vsn_names = NULL;
	out->vsn_pool_names = NULL;

	/* now copy the lists */
	if (dup_string_list(in->vsn_names, &(out->vsn_names)) != 0) {
		Trace(TR_DEBUG, "duplicating vsn_map failed: %s", samerrmsg);
		return (-1);
	}

	if (dup_string_list(in->vsn_pool_names,
	    &(out->vsn_pool_names)) != 0) {
		Trace(TR_DEBUG, "duplicating vsn_map failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "duplicated vsn_map");
	return (0);
}

/*
 * duplicate the incoming struct. The resulting out structure
 * is dynamically allocated so it can be freed.
 */
int
dup_ar_global_directive(
const ar_global_directive_t *in,
ar_global_directive_t **out)	/* malloced copy of in */
{

	Trace(TR_DEBUG, "duplicating ar_global_directive");

	if (ISNULL(in, out)) {
		Trace(TR_DEBUG, "duplicating ar_global_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	*out = (ar_global_directive_t *)mallocer(
	    sizeof (ar_global_directive_t));

	if (*out == NULL) {
		Trace(TR_DEBUG, "duplicating ar_global_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* copy the data in the struct */
	memcpy(*out, in, sizeof (ar_global_directive_t));


	/* set ptrs to NULL so if errors are encountered out can be freed */
	(*out)->ar_bufs = NULL;
	(*out)->ar_max = NULL;
	(*out)->ar_drives = NULL;
	(*out)->ar_set_lst = NULL;
	(*out)->ar_overflow_lst = NULL;


	if (dup_buffer_directive_list(in->ar_bufs, &((*out)->ar_bufs)) != 0) {

		free_ar_global_directive(*out);
		Trace(TR_DEBUG, "duplicating ar_global_directive failed: %s",
		    samerrmsg);

		return (-1);
	}
	if (dup_buffer_directive_list(in->ar_max, &((*out)->ar_max)) != 0) {
		free_ar_global_directive(*out);

		Trace(TR_DEBUG, "duplicating ar_global_directive failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (dup_drive_directive_list(in->ar_drives,
	    &((*out)->ar_drives)) != 0) {

		free_ar_global_directive(*out);
		Trace(TR_DEBUG, "duplicating ar_global_directive failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (dup_ar_set_criteria_list(in->ar_set_lst,
	    &((*out)->ar_set_lst)) != 0) {

		free_ar_global_directive(*out);
		Trace(TR_DEBUG, "duplicating ar_global_directive failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (dup_buffer_directive_list(in->ar_overflow_lst,
	    &((*out)->ar_overflow_lst)) != 0) {

		free_ar_global_directive(*out);
		Trace(TR_DEBUG, "duplicating ar_global_directive failed: %s",
		    samerrmsg);

		return (-1);
	}

	Trace(TR_DEBUG, "duplicated ar_global_directive");
	return (0);
}


/*
 * duplicate the incoming struct. The resulting out structure
 * is dynamically allocated so it can be freed.
 */
int
dup_buffer_directive_list(
const sqm_lst_t *in,
sqm_lst_t **out)		/* malloced copy of in */
{

	node_t *n;

	Trace(TR_DEBUG, "duplicating buffer_directive list");

	if (ISNULL(in, out)) {
		Trace(TR_DEBUG, "duplicating buffer_directive list failed: %s",
		    samerrmsg);
		return (-1);
	}

	*out = lst_create();
	if (*out == NULL) {
		Trace(TR_DEBUG, "duplicating buffer_directive list failed: %s",
		    samerrmsg);
		return (-1);
	}


	for (n = in->head; n != NULL; n = n->next) {
		buffer_directive_t *bd;
		if (dup_buffer_directive((buffer_directive_t *)n->data,
		    &bd) != 0) {

			free_buffer_directive_list(*out);

			Trace(TR_DEBUG, "%s failed: %s",
			    "duplicating buffer_directive list", samerrmsg);

			return (-1);
		}
		if (lst_append(*out, bd) != 0) {
			free_buffer_directive_list(*out);
			Trace(TR_DEBUG, "%s failed: %s",
			    "duplicating buffer_directive list", samerrmsg);

			return (-1);
		}
	}

	Trace(TR_DEBUG, "duplicated buffer_directive list");
	return (0);
}


/*
 * duplicate the incoming struct. The resulting out structure
 * is dynamically allocated so it can be freed.
 */
int
dup_drive_directive_list(
const sqm_lst_t *in,
sqm_lst_t **out)		/* malloced copy of in */
{

	node_t *n;

	Trace(TR_DEBUG, "duplicating drive_directive list");

	if (ISNULL(in, out)) {
		Trace(TR_DEBUG, "duplicating drive_directive list failed: %s",
		    samerrmsg);
		return (-1);
	}


	*out = lst_create();
	if (*out == NULL) {
		Trace(TR_DEBUG, "duplicating drive_directive list failed: %s",
		    samerrmsg);
		return (-1);
	}



	for (n = in->head; n != NULL; n = n->next) {

		drive_directive_t *dd;
		if (dup_drive_directive((drive_directive_t *)n->data,
		    &dd) != 0) {

			free_drive_directive_list(*out);
			Trace(TR_DEBUG, "%s failed: %s",
			    "duplicating drive_directive list", samerrmsg);

			return (-1);
		}
		if (lst_append(*out, dd) != 0) {
			free_drive_directive_list(*out);
			Trace(TR_DEBUG, "%s failed: %s",
			    "duplicating drive_directive list", samerrmsg);

			return (-1);
		}
	}
	Trace(TR_DEBUG, "duplicated drive_directive list");
	return (0);
}


/*
 * duplicate the incoming struct. The resulting out structure
 * is dynamically allocated so it can be freed.
 */
int
dup_buffer_directive(
const buffer_directive_t *in,
buffer_directive_t **out)	/* malloced copy of in */
{

	Trace(TR_DEBUG, "duplicating buffer_directive");

	if (ISNULL(in, out)) {
		Trace(TR_DEBUG, "duplicating buffer_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	*out = (buffer_directive_t *)mallocer(sizeof (buffer_directive_t));
	if (*out == NULL) {
		Trace(TR_DEBUG, "duplicating buffer_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	memcpy(*out, in, sizeof (buffer_directive_t));
	Trace(TR_DEBUG, "duplicated buffer_directive");

	return (0);
}


/*
 * duplicate the incoming struct. The resulting out structure
 * is dynamically allocated so it can be freed.
 */
int
dup_drive_directive(
const drive_directive_t *in,
drive_directive_t **out)	/* malloced copy of in */
{

	Trace(TR_DEBUG, "duplicating drive_directive");

	if (ISNULL(in, out)) {
		Trace(TR_DEBUG, "duplicating drive_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	*out = (drive_directive_t *)mallocer(sizeof (drive_directive_t));
	if (*out == NULL) {
		Trace(TR_DEBUG, "duplicating drive_directive failed: %s",
		    samerrmsg);
		return (-1);
	}

	memcpy(*out, in, sizeof (drive_directive_t));

	Trace(TR_DEBUG, "duplicated drive_directive");
	return (0);
}


/*
 * find the ar_set_criteria_node for the list node containing the
 * entry matching the input criteria. This function searchs the global
 * and per filesystem lists as needed.
 *
 * NOTE: This uses setname and the criterias key for matching and returns
 * a pointer to the actual node in cfg. It does NOT make a copy. So be
 * aware when freeing.
 */
int
find_ar_set_criteria_node(
const archiver_cfg_t *cfg,	/* cfg to search */
ar_set_criteria_t *crit,	/* criteria to find the node for */
node_t **ret)			/* actual node criteria is in */
{

	ar_set_criteria_t *tmp;
	ar_fs_directive_t *fs = NULL;
	node_t *n;


	Trace(TR_DEBUG, "finding criteria node");

	if (ISNULL(cfg, crit, ret)) {
		Trace(TR_OPRMSG, "%s failed: %s",
		    "finding criteria node", samerrmsg);
		return (-1);
	}


	*ret = NULL;

	if (strcmp(crit->fs_name, GLOBAL) == 0) {

		for (n = cfg->global_dirs.ar_set_lst->head;
		    n != NULL; n = n->next) {

			/* check for set name and key match */
			tmp = (ar_set_criteria_t *)n->data;
			if (strcmp(crit->set_name, tmp->set_name) == 0 &&
			    keys_match(tmp->key, crit->key)) {

				*ret = n;
			}
		}

	} else {

		/* find the fs */
		for (n = cfg->ar_fs_p->head; n != NULL; n = n->next) {
			if (strcmp(((ar_fs_directive_t *)n->data)->fs_name,
			    crit->fs_name) == 0) {

				fs = (ar_fs_directive_t *)n->data;
				break;
			}
		}

		if (fs == NULL) {
			samerrno = SE_NOT_FOUND;

			/* %s not found */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_NOT_FOUND), crit->fs_name);

			Trace(TR_OPRMSG, "%s failed: %s",
			    "finding criteria node", samerrmsg);

			return (-1);
		}

		for (n = fs->ar_set_criteria->head;
		    n != NULL; n = n->next) {

			/* check for set name and key match */
			tmp = (ar_set_criteria_t *)n->data;
			if (strcmp(crit->set_name, tmp->set_name) == 0 &&
			    keys_match(tmp->key, crit->key)) {

				*ret = n;
			}
		}
	}

	if (*ret == NULL) {
		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), crit->set_name);

		Trace(TR_OPRMSG, "finding criteria node failed: %s",
		    samerrmsg);

		return (-1);
	}

	Trace(TR_DEBUG, "found criteria node");
	return (0);
}




/*
 * returns a string representation as it would appear in the archiver.cmd file.
 * This string will contain newline characters and tabs as appropriate. The
 * string is held in an internal static buffer.
 *
 * returns the empty string if an error is encountered.
 */
char *
criteria_to_str(
ar_set_criteria_t *crit)	/* criteria to translate to a string */
{

	static char tmp[5 * MAX_LINE];
	int cur_length = 0;
	int added = 0;
	static char err = '\0';
	char fsizestr[FSIZE_STR_LEN];
	*tmp = '\0';


	/* If there are no criteria set, consider this criteria deleted. */
	if (crit->change_flag == 0) {
		return (&err);
	}

	if (*(crit->set_name) != char_array_reset) {
		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    "%s", crit->set_name);

		if (added > sizeof (tmp) - cur_length) {
			/* could not copy entire str */
			return (&err);
		}
		cur_length += added;

	}
	if (crit->change_flag & AR_ST_path &&
	    *(crit->path) != char_array_reset) {

		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " %s", crit->path);

		if (added > sizeof (tmp) - cur_length) {
			/* could not copy entire str */
			return (&err);
		}
		cur_length += added;
	}


	if (crit->change_flag & AR_ST_minsize &&
	    crit->minsize != fsize_reset) {

		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " -minsize %s", fsize_to_str(crit->minsize,
		    fsizestr, FSIZE_STR_LEN));

		if (added > sizeof (tmp) - cur_length) {
			/* could not copy entire str */
			return (&err);
		}
		cur_length += added;
	}
	if (crit->change_flag & AR_ST_maxsize &&
	    crit->maxsize != fsize_reset) {

		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " -maxsize %s", fsize_to_str(crit->maxsize,
		    fsizestr, FSIZE_STR_LEN));

		if (added > sizeof (tmp) - cur_length) {
			/* could not copy entire str */
			return (&err);
		}
		cur_length += added;
	}

	if (crit->change_flag & AR_ST_access && crit->access != int_reset) {

		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " -access %s", StrFromInterval(crit->access, NULL, 0));

		if (added > sizeof (tmp) - cur_length) {
			/* could not copy entire str */
			return (&err);
		}
		cur_length += added;
	}

	if (crit->change_flag & AR_ST_name &&
	    *(crit->name) != char_array_reset) {


		strlcat(tmp, " -name ", sizeof (tmp));
		cur_length += 7;

		compose_regex(crit->name, crit->regexp_type, tmp + cur_length,
		    sizeof (tmp) - cur_length);

		cur_length = strlen(tmp);
	}
	if (crit->change_flag & AR_ST_user &&
	    *(crit->user) != char_array_reset) {

		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " -user %s", crit->user);

		if (added > sizeof (tmp) - cur_length) {
			/* could not copy entire str */
			return (&err);
		}
		cur_length += added;
	}
	if (crit->change_flag & AR_ST_group &&
	    *(crit->group) != char_array_reset) {

		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " -group %s", crit->group);

		if (added > sizeof (tmp) - cur_length) {
			/* could not copy entire str */
			return (&err);
		}
		cur_length += added;
	}

	if (crit->change_flag & AR_ST_nftv &&
	    crit->nftv != B_FALSE) {

		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " -nftv");

		if (added > sizeof (tmp) - cur_length) {
			/* could not copy entire str */
			return (&err);
		}
		cur_length += added;
	}

	if (crit->change_flag & AR_ST_after &&
	    *(crit->after) != char_array_reset) {

		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " -after %s", crit->after);

		if (added > sizeof (tmp) - cur_length) {
			/* could not copy entire str */
			return (&err);
		}
		cur_length += added;
	}


	if (crit->change_flag & AR_ST_release &&
	    crit->release != char_array_reset) {

		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " -release %c", crit->release);

		if (added > sizeof (tmp) - cur_length) {
			/* could not copy entire str */
			return (&err);
		}
		cur_length += added;
	}
	if (crit->change_flag & AR_ST_stage &&
	    crit->stage != char_array_reset) {

		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " -stage %c", crit->stage);

		if (added > sizeof (tmp) - cur_length) {
			/* could not copy entire str */
			return (&err);
		}
		cur_length += added;

	}

	added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length, "%s",
	    get_copy_cfg_str(crit->arch_copy));

	if (added > sizeof (tmp) - cur_length) {
		return (&err);
	}

	return (tmp);
}


/*
 * given an array of MAX_COPY ar_set_copy_cfgs create the string that the
 * archiver would put in a cmd file for them.
 */
char *
get_copy_cfg_str(
ar_set_copy_cfg_t **arch_copy)	/* array of MAX_COPY ar_set_copy_cfg_t  */
{

	static char tmp2[MAX_COPY * MAX_LINE];
	char *cp;
	int cur_length = 0;
	int added = 0;
	int i;
	static char err = '\0';
	boolean_t copies_found = B_FALSE;

	*tmp2 = '\0';

	for (i = 1; i < MAX_COPY; i++) {
		if (arch_copy[i] != NULL) {
			copies_found = B_TRUE;
		}
	}

	for (i = 0; i < MAX_COPY; i++) {
		if (arch_copy[i] != NULL) {
			if (i == 0 && !copies_found &&
			    arch_copy[i]->change_flag == 0) {
				continue;
			}

			cp = ar_set_copy_cfg_to_str(arch_copy[i]);
			if (cp == '\0') {
				return (&err);
			}

			added = snprintf(tmp2 + cur_length,
			    sizeof (tmp2) - cur_length, "\n\t%s", cp);

			if (added > sizeof (tmp2) - cur_length) {
				/* could not copy entire str */
				return (&err);
			}
			cur_length += added;

		}
	}

	return (tmp2);
}


/*
 * translate the copy to a string in the form that is used in the cmd file.
 * returns the empty string if an error is encountered.
 */
char *
ar_set_copy_cfg_to_str(
ar_set_copy_cfg_t *cp)	/* cp to get string for */
{

	static char tmp[MAX_LINE];
	int cur_length = 0;
	int added = 0;
	static char err = '\0';

	Trace(TR_DEBUG, "archset_copy_to_str");

	*tmp = '\0';

	if (cp == NULL)
		return (&err);

	cur_length = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
	    "%d",  cp->ar_copy.copy_seq);

	if (cp->change_flag & AR_CP_release && cp->release) {

		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " -release");

		if (added > sizeof (tmp) - cur_length) {
			/* could not copy entire str */
			return (&err);
		}
		cur_length += added;
	}

	if (cp->change_flag & AR_CP_norelease && cp->norelease) {

		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " -norelease");

		if (added > sizeof (tmp) - cur_length) {
			/* could not copy entire str */
			return (&err);
		}
		cur_length += added;
	}

	if (cp->ar_copy.ar_age != uint_reset &&
	    (cp->change_flag & AR_CP_ar_age)) {

		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " %s", StrFromInterval(cp->ar_copy.ar_age, NULL, 0));
	} else  {

		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " %s", StrFromInterval(ARCH_AGE, NULL, 0));

	}

	if (added > sizeof (tmp) - cur_length) {
		/* could not copy entire str */
		return (&err);
	}
	cur_length += added;


	if (cp->change_flag & AR_CP_un_ar_age && cp->un_ar_age != uint_reset) {
		added = snprintf(tmp + cur_length, sizeof (tmp) - cur_length,
		    " %s", StrFromInterval(cp->un_ar_age, NULL, 0));

		if (added > sizeof (tmp) - cur_length) {
			/* could not copy entire str */
			return (&err);
		}
		cur_length += added;

	}

	Trace(TR_DEBUG, "archset_copy_to_str");
	return (tmp);
}


int
cfg_reset_ar_set_copy_params(
archiver_cfg_t *cfg,
const uname_t name)
{

	ar_set_copy_params_t *cfg_params;
	node_t *n;
	boolean_t found = B_FALSE;


	Trace(TR_DEBUG, "resetting ar_set_copy_params");

	/* find the current entry */
	for (n = cfg->archcopy_params->head; n != NULL; n = n->next) {
		cfg_params = (ar_set_copy_params_t *)n->data;
		if (strcmp(cfg_params->ar_set_copy_name, name) == 0) {
			found = B_TRUE;
			free_ar_set_copy_params(cfg_params);

			if (lst_remove(cfg->archcopy_params, n) != 0) {
				Trace(TR_OPRMSG, "%s failed: %s",
				    "resetting ar_set_copy_params", samerrmsg);

				return (-1);
			}
			break;
		}
	}


	if (!found) {
		samerrno = SE_NOT_FOUND;

		/* %s not found */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), name);

		Trace(TR_OPRMSG, "resetting ar_set_copy_params failed: %s",
		    samerrmsg);

		return (-1);
	}

	Trace(TR_DEBUG, "reset ar_set_copy_params");

	return (0);
}


/*
 * This function looks at the differences for copies that exist in old that
 * do not exist in new. For any such copies it finds it removes the params and
 * maps.
 */
int
remove_unneeded_maps_and_params(
archiver_cfg_t *cfg,
sqm_lst_t *old,	/* list of ar_set_criteria_t */
sqm_lst_t *new)	/* list of ar_set_criteria_t */
{

	node_t *out;
	node_t *in;
	boolean_t missing[MAX_COPY];
	ar_set_criteria_t *old_crit;
	ar_set_criteria_t *new_crit;
	int i;

	for (out = old->head; out != NULL; out = out->next) {
		old_crit = (ar_set_criteria_t *)out->data;

		if (strcmp(old_crit->set_name, NO_ARCHIVE) == 0) {
			/* has no copies to remove so don't loop */
			continue;
		}
		/*
		 * set up the array that determines if we need to find
		 * a match
		 */
		for (i = 0; i < MAX_COPY; i++) {
			if (old_crit->arch_copy[i] != NULL) {
				missing[i] = B_TRUE;
			} else {
				missing[i] = B_FALSE;
			}
		}

		/*
		 * loop over the new copies and figure out which ones
		 * match the things in the old
		 */
		for (in = new->head; in != NULL; in = in->next) {
			new_crit = (ar_set_criteria_t *)in->data;


			/* if setname matches check for copies. */
			if (strcmp(old_crit->set_name,
			    new_crit->set_name) == 0) {
				for (i = 0; i < MAX_COPY; i++) {
					if (new_crit->arch_copy[i] != NULL) {
						missing[i] = B_FALSE;
					}
				}
			}

		}

		/* remove any copies we have not found */
		for (i = 0; i < MAX_COPY; i++) {
			if (missing[i] == B_TRUE) {
				if (remove_maps_and_params(cfg,
				    old_crit->set_name, i+1) != 0) {
					return (-1);
				}
			}
		}
	}

	return (0);
}


/*
 * This function is used to remove the maps and parameters
 * associated with an archive set copy. The caller must have already
 * verified that it is appropriate for the copy to be removed.
 * It is not an error for no parameters or maps to be found.
 */
int
remove_maps_and_params(
archiver_cfg_t *cfg,
char *set_name,
int copy_num)
{

	uname_t copy_name;
	uname_t rearch_copy_name;

	Trace(TR_OPRMSG, "removing maps and params(%s %d) entry",
	    Str(set_name), copy_num);

	/*
	 * first make sure no other criteria by this name have
	 * this copy number
	 */
	snprintf(copy_name, sizeof (uname_t), "%s.%d",
	    set_name, copy_num);

	if (cfg_reset_ar_set_copy_params(cfg, copy_name) != 0) {
		if (samerrno != SE_NOT_FOUND) {
			Trace(TR_OPRMSG, "%s failed: %s",
			    "removing maps and params", samerrmsg);
			return (-1);
		}
	}
	snprintf(rearch_copy_name, sizeof (uname_t), "%s.%dR",
	    set_name, copy_num);

	if (cfg_reset_ar_set_copy_params(cfg, rearch_copy_name) != 0) {
		if (samerrno != SE_NOT_FOUND) {
			Trace(TR_OPRMSG, "%s failed: %s",
			    "removing maps and params", samerrmsg);
			return (-1);
		}
	}

	if (cfg_remove_vsn_copy_map(cfg, copy_name) != 0) {
		if (samerrno != SE_NOT_FOUND) {
			Trace(TR_OPRMSG, "%s failed: %s",
			    "removing maps and params", samerrmsg);
			return (-1);
		}
	}

	if (cfg_remove_vsn_copy_map(cfg, rearch_copy_name) != 0) {
		if (samerrno != SE_NOT_FOUND) {
			Trace(TR_OPRMSG, "%s failed: %s",
			    "removing maps and params", samerrmsg);
			return (-1);
		}
	}

	/*
	 * even if no copies/maps existed to remove if no error then return
	 * success
	 */
	Trace(TR_OPRMSG, "removed maps and params");

	return (0);
}



/*
 * this function returns a list of all criteria in a cfg.
 * the members of this list are not duplicates. They are the actual
 * items still in the cfg.
 */
int
get_list_of_all_criteria(archiver_cfg_t *cfg, sqm_lst_t **l) {
	node_t *out;
	node_t *n;

	Trace(TR_DEBUG, "getting list of all criteria");

	*l = lst_create();
	if (cfg->global_dirs.ar_set_lst != NULL) {

		for (n = cfg->global_dirs.ar_set_lst->head;
		    n != NULL; n = n->next) {

			if (lst_append(*l, n->data) != 0) {
				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting list of all criteria", samerrmsg);
				return (-1);
			}
		}

	}
	if (cfg->ar_fs_p == NULL) {
		return (0);
	}

	for (out = cfg->ar_fs_p->head; out != NULL; out = out->next) {

		ar_fs_directive_t *fs = (ar_fs_directive_t *)out->data;
		if (fs == NULL) {
			continue;
		}

		for (n = fs->ar_set_criteria->head; n != NULL; n = n->next) {
			if (lst_append(*l, n->data) != 0) {
				Trace(TR_OPRMSG, "%s failed: %s",
				    "getting list of all criteria", samerrmsg);
				return (-1);
			}
		}
	}

	Trace(TR_DEBUG, "got list of all criteria");
	return (0);
}



/*
 * This function should only be called if the archiver.cmd file does
 * not exist when write_arch_cfg is called. This function will add
 * a default vsn map for each archiving file system in the input cfg
 * that does not have one configured yet.
 */
static int
setup_archiver_fs_copy_maps(
archiver_cfg_t *cfg)
{

	sqm_lst_t *fs_list;
	node_t *n;
	boolean_t from_live;
	mtype_t mt;
	int ret;
	boolean_t need_hup;

	Trace(TR_MISC, "setting up default fs copy maps");

	if (select_default_media_type(mt, &need_hup) == -1) {
		return (-1);
	}


	/*
	 * where did the filesystems come from? If they come from the live
	 * system we can check the fi_archiving field otherwise we cannot.
	 */
	if ((ret = get_all_fs(NULL, &fs_list)) == -1) {
		Trace(TR_ERR, "setting up default fs copy maps failed %s",
		    samerrmsg);
		return (-1);
	} else if (ret == -2) {
		from_live = B_FALSE;
	} else {
		from_live = B_TRUE;
	}


	/*
	 * Check each archiving file system to determine if it needs
	 * a default vsn map. If so add one using the selected
	 * default media type.
	 */
	for (n = fs_list->head; n != NULL; n = n->next) {
		fs_t *fs = (fs_t *)n->data;

		/* if fs is not archiving do not set up anything. */
		if (from_live && !fs->fi_archiving) {
			continue;
		} else if (!(fs->mount_options->sam_opts.archive) ||
		    fs->mount_options->multireader_opts.reader) {
			continue;
		}

		if (!fs_needs_default_map(cfg, fs)) {
			continue;
		}

		if (cfg_add_default_fs_vsn_map(cfg, fs->fi_name, mt) != 0) {
			free_list_of_fs(fs_list);
			Trace(TR_ERR, "setting up default copy maps failed %s",
			    samerrmsg);
			return (-1);
		}
	}
	free_list_of_fs(fs_list);
	Trace(TR_MISC, "set up default fs copy maps");

	return (0);
}


static boolean_t
fs_needs_default_map(
archiver_cfg_t *cfg,
fs_t *fs) {

	node_t *out;
	node_t *in;
	uname_t fs_set_name;

	/* First check for an explicit default policy */
	for (out = cfg->ar_fs_p->head; out != NULL; out = out->next) {
		ar_fs_directive_t *fsdir = (ar_fs_directive_t *)out->data;

		if (fsdir == NULL || strcmp(fsdir->fs_name, fs->fi_name) != 0) {
			continue;
		}

		/*
		 * We've found the file system. If archmeta is set to true
		 * it means that a fs.1 copy map must exist- so break now and
		 * let the lower loop determine if one exists.
		 */
		if (fsdir->archivemeta) {
			break;
		}

		/*
		 * archive meta is set to false. A fs.1 copy map is still
		 * required unless there is a catchall criteria specified.
		 * Check for a catch all criteria.
		 */
		for (in = fsdir->ar_set_criteria->head; in != NULL;
			in = in->next) {
			ar_set_criteria_t *crit = (ar_set_criteria_t *)in->data;

			if (crit->change_flag & AR_ST_default_criteria) {
				return (B_FALSE);
			}
		}
	}

	/*
	 * check through the input cfg to see if a vsn map for this
	 * file system exists.
	 */
	snprintf(fs_set_name, sizeof (uname_t), "%s.1", fs->fi_name);
	for (in = cfg->vsn_maps->head; in != NULL; in = in->next) {
		vsn_map_t *map = in->data;

		if (map != NULL && strcmp(map->ar_set_copy_name,
		    fs_set_name) == 0) {
			return (B_FALSE);
		}
	}

	return (B_TRUE);
}

/*
 * This function modifies the cfg argument. It does not write the file.
 */
int
cfg_add_default_fs_vsn_map(
archiver_cfg_t *cfg,
uname_t fs_name,
mtype_t mt)
{

	vsn_map_t *vsn_map;
	char *vsn_name;


	Trace(TR_MISC, "adding default vsn map for %s\n", Str(fs_name));

	vsn_map = (vsn_map_t *)mallocer(sizeof (vsn_map_t));
	if (vsn_map == NULL) {
		Trace(TR_ERR, "adding default vsn map failed: %s", samerrmsg);
		return (-1);
	}


	memset(vsn_map, 0, sizeof (vsn_map_t));
	vsn_map->vsn_names = lst_create();
	vsn_map->vsn_pool_names = lst_create();
	if (ISNULL(vsn_map->vsn_names, vsn_map->vsn_pool_names)) {
		free_vsn_map(vsn_map);
		lst_free(vsn_map->vsn_names);
		lst_free(vsn_map->vsn_pool_names);
		Trace(TR_ERR, "adding default vsn map failed: %s", samerrmsg);
		return (-1);
	}

	snprintf(vsn_map->ar_set_copy_name, sizeof (uname_t), "%s.1", fs_name);
	strcpy(vsn_map->media_type, mt);

	vsn_name = strdup(".");
	if (NULL == vsn_name) {
		free_vsn_map(vsn_map);
		setsamerr(SE_NO_MEM);
		Trace(TR_ERR, "adding default vsn map failed: %s", samerrmsg);
		return (-1);
	}

	if (lst_append(vsn_map->vsn_names, vsn_name) != 0) {
		Trace(TR_ERR, "adding default vsn map failed: %s", samerrmsg);
		free_vsn_map(vsn_map);
		free(vsn_name);
		return (-1);
	}

	if (cfg_insert_copy_map(cfg->vsn_maps, vsn_map) == -1) {
		Trace(TR_ERR, "adding default vsn map failed: %s", samerrmsg);
		free_vsn_map(vsn_map);
		return (-1);
	}

	Trace(TR_MISC, "added default vsn map");
	return (0);
}


/*
 * Returns the media type to which the default copy should be set. Also if
 * disk media is selected and this function detects that the diskvols db
 * needs to be initialized dv_db_needs_sig will be set to true
 */
static int
select_default_media_type(
mtype_t mt,
boolean_t *dv_db_needs_sig)
{

	sqm_lst_t *l = NULL;
	boolean_t found = B_FALSE;

	/*
	 * If disk media is available select that as 1st copies on
	 * disk make the most sense and the default will get 1 copy.
	 */
	if (get_all_disk_vols(NULL, &l) == 0) {
		if (l != NULL && l->length != 0) {
			disk_vol_t *dv = l->head->data;

			found = B_TRUE;

			strlcpy(mt, DISK_MEDIA, sizeof (mtype_t));
			if (dv->status_flags & DV_DB_NOT_INIT) {
				*dv_db_needs_sig = B_TRUE;
			}
		}

		lst_free_deep(l);
		l = NULL;
	}

	if (!found) {
		if (get_all_available_media_type(NULL, &l) == -1) {
			return (-1);
		}

		/*
		 * select the first media type in the list.
		 */
		if (l->length > 0) {
			strlcpy(mt, (char *)l->head->data, sizeof (mtype_t));
			lst_free_deep(l);
		} else {
			/* none available, none licensed- use default */
			strlcpy(mt, IF_NO_MEDIA, sizeof (mtype_t));
		}

		lst_free_deep(l);
	}
	return (0);
}

/*
 * get the q timeout for archreq, these are archive requests in the
 * archive q that are writing to archive media
 */
int
get_archreq_qtimeout(int *timeout)
{
	sqm_lst_t *timeout_lst;
	node_t *node;

	if (ISNULL(timeout)) {
		Trace(TR_ERR, "unable to get archreq timeout: %s", samerrmsg);
		return (-1);
	}
	if (_get_ar_timeouts(&timeout_lst) != 0) {
		Trace(TR_ERR, "Unable to get archreq timeout: %s", samerrmsg);
		return (-1);
	}

	for (node = timeout_lst->head; node != NULL; node = node->next) {

		artimeout_t *t = (artimeout_t *)node->data;
		if (t == NULL) {
			continue; /* ignore partial failure */
		}
		if (t->type == TI_OPERATION && t->u.op == TO_write) {
			*timeout = t->timeout;

			lst_free_deep(timeout_lst);
			return (0);
		}
	}
	/* write timeout not configured, using default */
	lst_free_deep(timeout_lst);
	*timeout = WRITE_TIMEOUT;

	Trace(TR_OPRMSG, "archreq timeout = %d", *timeout);
	return (0);
}

/*
 * Timeouts are provided optionally in the archiver.cmd as a global directive
 * for the operations that may be stopped.
 *
 * This function gets the list of timeouts from the archiver global directive
 * as a list of kv pairs 'timeout = [ operation | media ] time', and parses the
 * kv pair to generate an artimeout_t structure.
 *
 * Returns 0 if success, -1 if unable to get the information
 * If no Timeouts are configured in archiver.cmd, an empty list is returned
 *
 * defaults:
 * READ_TIMEOUT               = 1 minute
 * REQUEST_TIMEOUT    = 15 minutes
 * STAGE_TIMEOUT      = 15 minutes
 * WRITE_TIMEOUT      = 15 minutes
 */
static int
_get_ar_timeouts(sqm_lst_t **artimeout_lst)
{
	size_t			len;
	node_t			*n;
	int			i;
	char			*ptr1, *ptr2, *ptr3, *last;
	ar_global_directive_t	*ar_global;

	if (ISNULL(artimeout_lst)) {

		Trace(TR_ERR, "unable to get archiver timeout: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "get archiver timeout");
	if (get_ar_global_directive(NULL, &ar_global) != 0) {

		Trace(TR_ERR, "unable to get archiver timeout: %s", samerrmsg);
		return (-1);
	}

	*artimeout_lst = lst_create();
	if (*artimeout_lst == NULL) {

		free_ar_global_directive(ar_global);

		Trace(TR_ERR, "unable to get archiver timeout: %s", samerrmsg);
		return (-1);
	}

	if (ar_global->timeouts == NULL || ar_global->timeouts->length == 0) {

		free_ar_global_directive(ar_global);

		Trace(TR_MISC, "archiver timeout is not configured");

		/* return empty list */
		return (0);
	}

	for (n = ar_global->timeouts->head; n != NULL; n = n->next) {

		/* timeout = [ operation | media ] time */
		char *kv = n->data;
		if (kv == NULL) {
			continue;
		}

		ptr1 = strchr(kv, '=');
		if (ptr1 != NULL)  {
			*ptr1 = '\0';
			ptr1++;
		}

		/* ptr2 = operation | media, ptr3 = timeout */
		ptr2 = strtok_r(ptr1, WHITESPACE, &last);
		ptr3 = strtok_r(NULL, WHITESPACE, &last);

		if (ptr2 != NULL && ptr3 != NULL) {

			artimeout_t *t = (artimeout_t *)
			    mallocer(sizeof (artimeout_t));

			t->timeout = strtol(ptr3, (char **)NULL, 10);

			for (i = 0; Timeouts[i].EeName != NULL; i++) {

				len = strlen(ptr2);
				if (strncmp(
				    ptr2, Timeouts[i].EeName, len) == 0) {

					t->u.op = Timeouts[i].EeValue;
					t->type = TI_OPERATION;
					break;
				}
			}

			if (Timeouts[i].EeName == NULL) {
				strlcpy(t->u.mtype, ptr2, sizeof (mtype_t));
				t->type = TI_MEDIA;
			}

			if (lst_append(*artimeout_lst, t) != 0) {

				free(t);
				lst_free_deep(*artimeout_lst);
				free_ar_global_directive(ar_global);

				Trace(TR_ERR, "unable to get archiver timeout:"
				    "%s", samerrmsg);
				return (-1);
			}
		}
	}

	free_ar_global_directive(ar_global);

	Trace(TR_MISC, "archiver timeout obtained");
	return (0);
}
