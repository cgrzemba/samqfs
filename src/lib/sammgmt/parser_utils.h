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
#ifndef	_PARSER_UTILS_H
#define	_PARSER_UTILS_H

#pragma ident   "$Revision: 1.18 $"

#include <sys/types.h>
#include <stdio.h>

#include "mgmt/config/common.h"
#include "pub/mgmt/sqm_list.h"
#include "sam/setfield.h"

typedef enum {
	ERROR,
	WARNING
} error_type_t;

typedef struct parsing_error {
	int		line_num;
	error_type_t	error_type;
	int		severity;
	char		input[MAX_LINE];
	char		msg[MAX_LINE];
} parsing_error_t;

/*
 * returns the index of mt in the dev_nm2mt array or -1 if not found.
 * this method replaces asmMedia's checking.
 */
int check_media_type(const char *mt);

/*
 * check the field for whitespace. Setup samerrno and samerrmsg if whitespace
 * is contained.
 */
int has_spaces(char *field, char *field_name);


/*
 * make and return a copy of the input list
 */
int dup_parsing_error_list(sqm_lst_t *in, sqm_lst_t **out);

int dup_string_list(sqm_lst_t *in, sqm_lst_t **out);

int check_field_value(const void *st, struct fieldVals *table, char *field,
	char *buf, int bufsize);

/*
 * get the cmd file string that, given the fieldVals entry would result in st
 * having its current value.
 */
char *get_cfg_str(const void *st, struct fieldVals *entry, boolean_t val_only,
	char *buf, int bufsize, boolean_t vfstab, char skip_entry[][20]);

/*
 * checks to see if defbits has been set to the CLR flag value for the field
 * described by entry.
 */
int
is_reset_value(int32_t defbits, struct fieldVals *entry, boolean_t *ret_val);


/*
 * returns true if the set_bits have the change flag that corresponds to
 * the fieldVals entry set.
 */
boolean_t is_explicit_set(uint32_t set_bits, struct fieldVals *entry);

/*
 * A utility function to print headers only if needed. The string
 * is only printed if *printed = false.
 * writes the string and sets printed to true if printed is false.
 */
int write_once(FILE *f, char *print_first, boolean_t *printed);

#endif	/* _PARSER_UTILS_H */
