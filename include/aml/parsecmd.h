/*
 * parsecmd.h  - definition for command file parser
 */

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


#ifndef _AML_PARSECMD_H
#define	_AML_PARSECMD_H

#pragma ident "$Revision: 1.12 $"


#include <stdio.h>    /* for FILE definition */

typedef struct cmd_ent cmd_ent_t;

typedef enum parse_type {
	PARSE_NONE = 0,
	PARSE_LONG,
	PARSE_FLOAT,
	PARSE_BOOL,
	PARSE_STRING
} parse_t;

/* Keyword processor table entry */
struct cmd_ent {
	char	*key;		/* Keyword - NULL for table terminator */
	void	*var;		/* variable to hold the value from cmd file */
	parse_t	parse_type;	/* see above for possible values */
	void	*valid;		/* pointer to array of min/max range for */
				/* long/float or possible values for strings */
	int	(*post_func)(cmd_ent_t *);	/* Post processing function  */
};


void *parse_init(char *, char *, int, FILE *);
void parse_fini(void *);
int parse_cmd_file(void *, cmd_ent_t *);

#endif /* _AML_PARSECMD_H */
