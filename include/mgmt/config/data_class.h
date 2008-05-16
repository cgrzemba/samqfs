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
#ifndef _CFG_DATA_CLASS_H
#define	_CFG_DATA_CLASS_H

#pragma ident   "$Revision: 1.7 $"

#include "pub/mgmt/archive.h"
#include "sam/setfield.h"

#define	FILE_CONTAINS_SUFFIX	"[^/]*$"
#define	ENDS_WITH_SUFFIX	"$"


static struct EnumEntry RegExpTypes[] = {
	{ "regexp",		REGEXP },
	{ "name_contains",	FILE_NAME_CONTAINS },
	{ "path_contains",	PATH_CONTAINS },
	{ "file_starts",	FILE_NAME_STARTS_WITH },
	{ "ends",		ENDS_WITH },
	{ NULL }
};


/* Get a list of the data classes (ar_set_criteria_t structs for now) */
int get_data_classes(ctx_t *c, sqm_lst_t **l);


/*
 * remove the named data class from the data_class.cmd file
 */
int remove_data_class(ctx_t *c, char *name);


/*
 * copy the fields from the ar_set_criteria into the class configuration
 * for the class. This function will add a new data class if one with
 * the same name does not exist.
 */
int update_data_class(ctx_t *c, ar_set_criteria_t *cr);


/*
 * if arch_fmt is true this will return a string in the format accepted by
 * the archiver.cmd file. If it is false this will return a string in the
 * format expected by the data_class.cmd file.
 */
int get_class_str(ar_set_criteria_t *cr, boolean_t arch_fmt,
    char *buf, int buflen);


/*
 * Returns a pointer into the list of classes. Do not free the returned
 * criteria separately.
 */
int find_class_by_name(char *class_name, sqm_lst_t *classes,
	ar_set_criteria_t **data_class);

/*
 * Returns a pointer into the list of classes. Do not free the returned
 * criteria separately. This will return a data class that matches the input
 * according to the class_cmp function.
 */
int find_class_by_criteria(ar_set_criteria_t *cr, sqm_lst_t *classes,
    ar_set_criteria_t **data_class);


/*
 * detemines the match based only on the class criteria. It does not
 * include priority, fs_name, set_name or class_name, staging or
 * releasing characteristics.
 */
int class_cmp(ar_set_criteria_t *cr1, ar_set_criteria_t *cr2);

/*
 * This function takes a string and a regular expression type. It
 * applies the rules for the type to the input string to come up with
 * a regexp acceptable in the archiver.cmd. If the input string can't
 * be made into a regexp of the type indicated (e.g. the type is not
 * valid), the results buffer will simply contain the input string.
 */
int compose_regex(char *regex, regexp_type_t type, char *res, int res_sz);


/*
 * Set the class name, regexp type and priority for the criteria from
 * the classes
 */
int setup_class_data(ar_set_criteria_t *cr, sqm_lst_t *classes);


/*
 * returns true if the data class applies to the named file system.
 */
boolean_t class_applies_to_fs(ar_set_criteria_t *data_class, char *fs_name);

/*
 * parse the class file identified and return a list of ar_set_criteria_t
 */
int parse_class_file(char *class_file, sqm_lst_t **class_list);

#endif /* _CFG_DATA_CLASS_H */
