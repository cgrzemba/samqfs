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
#ifndef	_ARCHIVE_SETS_H
#define	_ARCHIVE_SETS_H

#pragma ident "	$Revision: 1.18 $	"


/*
 * archive_sets.h - Extended APIs for the support of archiving
 * operations on archive sets. These APIs allow many multi-step
 * operations using the archive.h APIs to be reduced to a single
 * remote call. This allows for the cleaner failure of these
 * operations as opposed to the previous multiple remote call
 * versions.
 */

#include <sys/types.h>

#include "sam/types.h"
#include "aml/archset.h"

#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/archive.h"


typedef enum set_type {
	DEFAULT_SET,
	GENERAL_SET,
	NO_ARCHIVE_SET,
	ALLSETS_PSEUDO_SET,
	EXPLICIT_DEFAULT
} set_type_t;


/*
 * arch_set_t
 * There are four types of archive set.
 * 1. general archive set (includes policy)
 * 2. no_archive set
 * 3. Default archive set
 * 4. Allsets pseudo archive sets
 *
 * There are different rules based on what type of arch_set is being
 * handled.
 *
 * General archive sets:
 *	The set name may not match that of a filesystem, nor can they
 *	be named no_archive.
 *
 * no_archive sets:
 *	vsn_maps and copy_params specified for these will be
 *	ignored and will not be written to the archiver.cmd file as they
 *	have no meaning.
 *
 * default archive sets
 *	There will be one default archive set for each archiving
 *	file system.  Default sets will have a single criteria entry
 *	and only its arch_copy array may be set. Any other criteria
 *	information will be ignored. The vsn_maps and copy_params can
 *	be set.
 *
 * allsets pseudo archive sets:
 *	The allsets and allsets.<copy_num> archive sets can be used to
 *	configure the copy parameters and vsn associations for all of
 *	the other archive sets. Settings for an specific set always
 *	override those made on an allset. Any criteria contained in
 *	an allset archive set will be ignored.
 *
 * Both the General Archive Set and the no_archive set can contain global
 * archiving criteria that will apply to all filesystems.
 *
 * The copy params and vsn_maps are indexed by copy number - 1. The
 * highest entry in each array is used only for params or maps in the
 * allsets pseudo set. So, for example in the archive set largefiles,
 * copy_params[0] contains a pointer to the copy params for
 * largefiles.1 This is done to be consistent with the existing copy
 * arrays in the ar_set_criteria and ar_fs_directive structs.
 */
typedef struct arch_set {

	set_name_t name;

	/* what type of set Default Set, No Archive, set */
	set_type_t type;

	/* list of all ar_set_criteria_t for this set. */
	sqm_lst_t *criteria;

	/* copy params */
	ar_set_copy_params_t *copy_params[5];

	/* vsn_maps for the copies */
	vsn_map_t *vsn_maps[5];

	/* rearchive copy params. If NULL then the copy_params are used */
	ar_set_copy_params_t *rearch_copy_params[5];

	/* rearchive copy vsn_maps. if NULL then the vsn_maps are used */
	vsn_map_t *rearch_vsn_maps[5];

	char *description;
} arch_set_t;


/*
 * Get a list of all arch_set_t structures, describing all archive sets.
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
sqm_lst_t **l);	/* returned list of arch_set_t */



/*
 * get the named arch_set structure
 */
int
get_arch_set(
ctx_t *ctx,		/* can be used to read from alternate location */
set_name_t name,	/* name of the set to return */
arch_set_t **set);	/* Malloced return value */


/*
 * Function to create an archive set. The criteria will be added to the
 * end of any file system for which they are specified. Can not be used to
 * create a default arch set. This function will fail if the set already
 * exists.
 */
int
create_arch_set(
ctx_t *ctx,		/* contains optional dump path */
arch_set_t *set);	/* archive set to create */


/*
 * This function would modify each of the criteria in the archive set, and the
 * copy params and vsn_map for this function. It is also important to note that
 * Global criteria may be contained in the set, in which case this function
 * can affect all file systems. If this function is being called with the
 * intellistore separate lifecycle feature enabled, it cannot be used to delete
 * criteria nor to reassign criteria to a different policy.
 */
int
modify_arch_set(
ctx_t *ctx,	/* contains optional dump path */
arch_set_t *);	/* archive set to modify */


/*
 * Delete will delete the entire set including copies, criteria,
 * params and vsn associations.
 *
 * If the named set is the allsets pseudo set this function will
 * reset defaults because the allsets set can not really be deleted.
 *
 * If the named set is a default set this function will return the
 * error: SE_CANNOT_DELETE_DEFAULT_SET. It is not possible to delete
 * the default set because it will exist as long as the file system of
 * the same name exists. Users wishing to remove all archiving related
 * traces of an archiving file system from the archiver.cmd should
 * instead use reset_ar_fs_directive.
 */
int
delete_arch_set(
ctx_t *ctx,		/* contains optional dump path */
set_name_t name);	/* the name of the set to delete */


void free_arch_set(arch_set_t *);
void free_arch_set_list(sqm_lst_t *l);

#endif	/* _ARCHIVE_SETS_H */
