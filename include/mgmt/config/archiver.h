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
#ifndef _CFG_ARCHIVER_H
#define	_CFG_ARCHIVER_H

#pragma ident   "$Revision: 1.27 $"

/*
 * archiver.h
 * Functions used to read and write and search the archiver configuration.
 * The archiver config is devided to 5 parts: global, file system,
 * file or set parameters, vsn pools, vsn and set mapping.
 *
 */

#include "pub/mgmt/archive.h"
#include "mgmt/config/cfg_diskvols.h"

#define	DEFAULT_NOTIFY SBIN_DIR"/"NOTIFY

#define	DEFAULT_AR_BUFSIZE 4

/* The entire archiver config in a struct. */
typedef struct archiver_cfg {
	ar_global_directive_t global_dirs;
	sqm_lst_t	*ar_fs_p;	/* list of ar_fs_directive_t */
	sqm_lst_t	*archcopy_params; /* list of ar_set_copy_params_t */
	sqm_lst_t	*vsn_pools;	/* list of vsn_pool_t */
	sqm_lst_t	*vsn_maps;
	time_t		read_time;
} archiver_cfg_t;

#define	TI_OPERATION	0
#define	TI_MEDIA	1

typedef struct artimeout {
	int	type;		/* TI_OPERATION | TI_MEDIA */
	union {
		mtype_t mtype;	/* 2 letter mnemonic */
		int	op;	/* TO_read | TO_write | TO_stage | TO_request */
	} u;
	int	timeout;
} artimeout_t;

/*
 * Function to get the timeout for archive requests waiting to write to
 * archive media
 */
int get_archreq_qtimeout(int *timeout);

/*
 * Begin file system and archive set api definition
 */


/* check the existence of an archive set by name */
boolean_t set_exists(archiver_cfg_t *cfg, set_name_t name);

/*
 * Given a archive set name, get a list of that sets criteria.
 */
int cfg_get_criteria_by_name(const archiver_cfg_t *cfg, const char *set_name,
	sqm_lst_t **criteria_list);


/*
 * Given a file system name, get all archive set criteria for that fs.
 * This method does not append the global archiveset criteria to the list.
 */
int cfg_get_criteria_by_fs(const archiver_cfg_t *cfg, const char *fs_name,
	sqm_lst_t **criteria_list);



/*
 * This function returns a list of all of the criteria in cfg.
 * The members of this list are not duplicates. They are the actual
 * items still in the cfg so when freeing this list don't free the members
 * if cfg will also be freed, instead simply call lst_free.
 */
int
get_list_of_all_criteria(archiver_cfg_t *cfg, sqm_lst_t **l);



/*
 * returns a pointer to node in the cfg that contains the matching criteria.
 * The criteria match is based on the fs_name, set_name and key.
 */
int find_ar_set_criteria_node(const archiver_cfg_t *cfg,
	ar_set_criteria_t *crit, node_t **node);


/*
 * Given a file system name, get file system's ar_fs_directive_t structure.
 * If malloc_return is set to B_TRUE the result is malloced. Otherwise a
 * a pointer to the fs directive in cfg is returned.
 *
 * returns -1 and sets samerrno if the fs does not exist.
 */
int cfg_get_ar_fs(const archiver_cfg_t *cfg, const char *fs_name,
	boolean_t malloc_return, ar_fs_directive_t **arch_fs);


/*
 * Given a archive set copy name, get archive set parameter structure.
 * returns -1 and sets samerrno if not found.
 */
int cfg_get_ar_set_copy_params(const archiver_cfg_t *cfg,
	const char *copy_name, ar_set_copy_params_t **params);


/*
 * Given an archset copy name including the .copy, remove it
 * from cfg and free it.
 */
int
cfg_reset_ar_set_copy_params(archiver_cfg_t *cfg, const uname_t name);


/*
 * inserts a copy params in the appropriate place in the list.
 * ALL_SETS and ALL_SETS copies must come first and second relatively.
 * It would be nice if other copies were put in numerical order but there is
 * no guarantee that they will be.
 */
int
cfg_insert_copy_params(sqm_lst_t *l, ar_set_copy_params_t *cp);


/*
 * Given a vsn pool name, get that vsn pool structure.
 * returns -1 and sets samerrno if not found.
 */
int cfg_get_vsn_pool(const archiver_cfg_t *cfg, const char *pool_name,
	vsn_pool_t **vsn_pool);

/*
 * search all maps for the pool. Return true if the pool is in use.
 * Arguments must be non-null. This condition is not checked here.
 */
boolean_t
cfg_is_pool_in_use(
	archiver_cfg_t *cfg,		/* cfg to search */
	const uname_t pool_name,	/* name of pool to check */
	uname_t used_by);

/*
 * Given a vsnset name, to get that vsn mapping structure.
 */
int cfg_get_vsn_map(const archiver_cfg_t *cfg, const char *copy_name,
	vsn_map_t **vsn_map);

/*
 * inserts a copy map in the appropriate place in the list.
 * ALL_SETS and ALL_SETS copies must come first and second relatively.
 * It would be nice if other copies were put in numerical order but there is
 * no guarantee that they will be.
 */
int cfg_insert_copy_map(sqm_lst_t *l, vsn_map_t *map);

/*
 * Get all vsn maps in whole configuration. Doesnt make sense the
 * archset name is not even in the map.
 */
int cfg_get_all_vsn_maps(const archiver_cfg_t *cfg, sqm_lst_t **vsn_map_list);

/*
 * remove and free the copy map for the set.copy specified in name from
 * the cfg.
 */
int cfg_remove_vsn_copy_map(archiver_cfg_t *cfg, const uname_t name);


/*
 * Function to remove the vsn maps and copy parameters
 * associated with an archive set copy. The caller must have already
 * verified that it is appropriate for the copy to be removed.
 * It is not an error for no parameters or maps to be found.
 */
int
remove_maps_and_params(archiver_cfg_t *cfg, char *set_name, int copy_num);

/*
 * This function looks at the differences between the ar_set_criteria in the
 * list old and the list new. For copies that exist in old that
 * do not exist in new it removes the copy params and vsn maps.
 */
int
remove_unneeded_maps_and_params(archiver_cfg_t *cfg, sqm_lst_t *old,
    sqm_lst_t *new);

/*
 * general functions
 */

/*
 * Read archive configuration from the default location and build the
 * archiver_cfg_t structure. Alternatively, if the ctx object has a
 * read_location specified, the file at that location will be read.
 */
int read_arch_cfg(ctx_t *ctx, archiver_cfg_t **cfg);

int get_archiver_parsing_errors(sqm_lst_t **l);

/*
 * Write the archiver configuration to the default location.
 *
 * If force is false and the archive configuration has been modified since
 * the archiver_cfg_t structure was returned by the read_arch_cfg function,
 * this function will set errno and return a -1.
 *
 * If force is true the archive configuration will be written without regard
 * to its modification time.
 */
int write_arch_cfg(ctx_t *ctx, archiver_cfg_t *cfg, const boolean_t force);

int verify_arch_cfg(archiver_cfg_t *cfg);


/*
 * dump the configuration to the specified location. If a file exists at
 * the specified location it will be overwritten.
 */
int dump_arch_cfg(archiver_cfg_t *cfg, const char *location);

/* default helpers */
int init_ar_fs_directive(ar_fs_directive_t *fs);

int get_default_copy_cfg(ar_set_copy_cfg_t **cp);

int init_criteria(ar_set_criteria_t *c, char *criteria_name);

char *get_fsize_str(fsize_t f);

int init_ar_copy_params(ar_set_copy_params_t *p);

int init_global_dirs(ar_global_directive_t *g, sqm_lst_t *libs);

buffer_directive_t *
create_buffer_directive(mtype_t mt, fsize_t size, boolean_t lock);


sqm_lst_t *
create_default_ar_drives(sqm_lst_t *libraries, boolean_t global);

/*
 * function to free nested data structures.
 */

void free_archiver_cfg(archiver_cfg_t *cfg);


/*
 * returns a string representation as it would appear in the archiver.cmd file.
 * This string will contain newline characters and tabs as appropriate. The
 * string is held in an internal static buffer.
 */
char *criteria_to_str(ar_set_criteria_t *crit);

/*
 * given an array of MAX_COPY ar_set_copy_cfgs create the string that the
 * archiver would put in a cmd file for them.
 */
char *
get_copy_cfg_str(ar_set_copy_cfg_t **arch_copy);

/*
 * returns a string representation as it would appear in the archiver.cmd file.
 * This string will contain newline characters and tabs as appropriate. The
 * string is held in an internal static buffer.
 */
char *ar_set_copy_cfg_to_str(ar_set_copy_cfg_t *cp);


/*
 * The actual internals of parsing the archiver.cmd file.
 */
int
parse_archiver(char *arch_file, sqm_lst_t *filesystems, sqm_lst_t *libraries,
	diskvols_cfg_t *disk_vols, archiver_cfg_t *cfg);


int
add_default_fs_vsn_map();




/*
 * chk_arch_cfg
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
chk_arch_cfg(ctx_t *ctx, sqm_lst_t **err_warn_list);



/* Functions to duplicate the structures of pub/mgmt/archive.h */
int dup_ar_set_copy_params(const ar_set_copy_params_t *in,
	ar_set_copy_params_t **out);

int cp_ar_set_copy_params(const ar_set_copy_params_t *in,
	ar_set_copy_params_t *out);

int dup_vsn_pool(const vsn_pool_t *in, vsn_pool_t **out);

int dup_vsn_map(const vsn_map_t *in, vsn_map_t **out);

int cp_vsn_map(const vsn_map_t *in, vsn_map_t *out);

int dup_ar_global_directive(const ar_global_directive_t *in,
	ar_global_directive_t **out);

int dup_buffer_directive_list(const sqm_lst_t *in, sqm_lst_t **out);

int dup_drive_directive_list(const sqm_lst_t *in, sqm_lst_t **out);

int dup_buffer_directive(const buffer_directive_t *in,
	buffer_directive_t **out);

int dup_drive_directive(const drive_directive_t *in, drive_directive_t **out);


int dup_ar_fs_directive(const ar_fs_directive_t *in, ar_fs_directive_t **out);

int dup_ar_set_copy_cfg(const ar_set_copy_cfg_t *in, ar_set_copy_cfg_t **out);

int dup_ar_set_criteria_list(const sqm_lst_t *in, sqm_lst_t **out);

int dup_ar_set_criteria(const ar_set_criteria_t *in, ar_set_criteria_t **out);


#endif /* _CFG_ARCHIVER_H */
