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
#ifndef _RELEASE_H
#define	_RELEASE_H

#pragma	ident	"$Revision: 1.15 $"
/*
 *	release.h - SAM-FS APIs for releasing operations.
 */


#include "sam/types.h"

#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"

/*
 *	structs
 */


typedef enum { NOT_SET, SIMPLE_AGE_PRIO, DETAILED_AGE_PRIO } age_prio_type;


/*
 * structure for release age priority.
 */
typedef struct rl_age_priority_cfg {
	float	access_weight;
	float	modify_weight;
	float	residence_weight;
} rl_age_priority_t;


/*
 * structure for file system release directive. see the man pages for
 * releaser.cmd for more details. If the fs is set to the constant GLOBAL
 * from types.h, then the directives in this structure are global.
 */
typedef struct rl_fs_directive {
	uname_t		fs;	/* GLOBAL from types.h for global directives */
	upath_t		releaser_log;
	float		size_priority;
	age_prio_type	type;
	union {
		float			simple;
		rl_age_priority_t	detailed;
	} age_priority;
	boolean_t	no_release;
	boolean_t	rearch_no_release;
	boolean_t	display_all_candidates;
	long		min_residence_age; /* min residence time seconds  */
	boolean_t	debug_partial;
	int		list_size;	/* number of files to consider    */
	uint32_t	change_flag;	/* Flag to indicate what fields   */
					/* have been changed by the user. */
} rl_fs_directive_t;

typedef struct release_fs {
	uname_t		fi_name;	/* file system name */
	ushort_t	fi_low;		/* Low water % threshold for releaser */
	int		used_pct;	/* Disk used space percentage */
} release_fs_t;


/*
 * get all the file system directives for releaser.
 *	sqm_lst_t **rl_fs_directives -	returned list of rl_fs_directive_t data,
 *						it must be freed by caller.
 */
int get_all_rl_fs_directives(ctx_t *ctx, sqm_lst_t **rl_fs_directive_list);


/*
 * get the release directives for a file system; use the constant GLOBAL
 * from types.h as fs_name to get the global directives.
 *
 *	const uname_t fs_name -			file sytem name.
 *	rl_fs_directive_t **rl_fs_directives -	returned rl_fs_directive_t data,
 *						it must be freed by caller.
 */
int get_rl_fs_directive(ctx_t *ctx, const uname_t fs_name,
	rl_fs_directive_t **rl_fs_directive);


/*
 * get the default release directives.
 * rl_fs_directive_t **rl_fs_directive - returned data, must be freed by caller.
 */
int get_default_rl_fs_directive(ctx_t *ctx,
	rl_fs_directive_t **rl_fs_directive);


/*
 * functions to set release directives for a file system.
 *
 * rl_fs_directive_t *rl_fs_directives -
 *	rl_fs_directive_t data needed to be set,
 *	it is not freed in this funuction.
 */
int set_rl_fs_directive(ctx_t *ctx, rl_fs_directive_t *rl_fs_directive);


/*
 * functions to reset release directives for a file system.
 * reset means that reset fs_directive to default and it will be
 * removed from configuration file.
 *
 * rl_fs_directive_t *rl_fs_directives -
 *	rl_fs_directive_t data needed to be reset,
 *	it is not freed in this function.
 */
int reset_rl_fs_directive(ctx_t *ctx, rl_fs_directive_t *rl_fs_directive);


/*
 * functions to get a fs list which is doing release job.
 * the list is a list of structure release_fs_t.
 *
 *	sqm_lst_t **releasing_fs_list -	a reyurned list of release_fs_t,
 *						it must be freed by caller.
 */
int
get_releasing_fs_list(ctx_t *ctx, sqm_lst_t **releasing_fs_list);


/*
 * Free methods.
 */

void free_list_of_rl_fs_directive(sqm_lst_t *directives);



/*
 * Options for the release files command
 */
#define	RL_OPT_RECURSIVE 0x00000001	/* recursively perform release */
#define	RL_OPT_NEVER	 0x00000002	/* set the never release flag */
#define	RL_OPT_WHEN_1	 0x00000004	/* set the always release flag */
#define	RL_OPT_PARTIAL   0x00000008	/* set the partial release flag */
#define	RL_OPT_DEFAULTS	 0x00000010	/* reset release flags to default */


int
release_files(ctx_t *c, sqm_lst_t *files, int32_t option, int32_t partial_sz,
    char **job_id);


/* change flag values for rl_fs_directive_t */
#define	RL_fs				0x00000001
#define	RL_releaser_log			0x00000002
#define	RL_size_priority		0x00000004
#define	RL_type				0x00000008
#define	RL_access_weight		0x00000010
#define	RL_modify_weight		0x00000020
#define	RL_residence_weight		0x00000040
#define	RL_simple			0x00000080
#define	RL_no_release			0x00000100
#define	RL_rearch_no_release		0x00000200
#define	RL_display_all_candidates	0x00000400
#define	RL_list_size			0x00000800
#define	RL_min_residence_age		0x00001000
#define	RL_debug_partial		0x00002000

#endif	/* _RELEASE_H */
