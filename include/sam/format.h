/*
 * format.h - Definitions to support formatting info for display.
 */

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

#ifndef _SAM_FORMAT_H
#define	_SAM_FORMAT_H

#ifdef sun
#pragma ident "$Revision: 1.10 $"
#endif

#define	SAM_FORMAT_TYPE_EOF		0
#define	SAM_FORMAT_TYPE_COMMENT		1
#define	SAM_FORMAT_TYPE_ELEMENT		2
#define	SAM_FORMAT_TYPE_PREFIX		3
#define	SAM_FORMAT_TYPE_NAME		4
#define	SAM_FORMAT_TYPE_MIDFIX		5
#define	SAM_FORMAT_TYPE_VALUE		6
#define	SAM_FORMAT_TYPE_SUFFIX		7

#define	SAM_FORMAT_PREFIX_DEF		""
#define	SAM_FORMAT_MIDFIX_DEF		" = "
#define	SAM_FORMAT_SUFFIX_DEF		"\n"

#define	SAM_FORMAT_WIDTH_NAME_DEF	0
#define	SAM_FORMAT_WIDTH_VALUE_DEF	0
#define	SAM_FORMAT_WIDTH_MAX		80

typedef struct {
	int	type;		/* Line type */
	int	width;		/* Width of this field */
	int	len;		/* Length of string */
	char	str[1];		/* Displayed string */
} sam_format_buf_t;

int
sam_format_prefix_default(char *prefix);

int
sam_format_midfix_default(char *midfix);

int
sam_format_suffix_default(char *suffix);

int
sam_format_width_name_default(int width);

int
sam_format_width_value_default(int width);

int
sam_format_element_append(sam_format_buf_t **bufp, char *name, char *value);

int
sam_format_element_prefix(sam_format_buf_t *bufp, char *name, char *prefix);

int
sam_format_element_midfix(sam_format_buf_t *bufp, char *name, char *midfix);

int
sam_format_element_suffix(sam_format_buf_t *bufp, char *name, char *suffix);

int
sam_format_element_width_name(sam_format_buf_t *bufp, char *name, int width);

int
sam_format_element_width_value(sam_format_buf_t *bufp, char *name, int width);

int
sam_format_print(sam_format_buf_t *bp, char *name);
#endif /* _SAM_FORMAT_H */
