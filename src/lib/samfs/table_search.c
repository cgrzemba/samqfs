/*
 *  table_search.c - Return error message found in a table.
 *
 *	Description:
 *	    Returns an error message string for the given error.
 *		program name:prefix message:error message
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

#pragma ident "$Revision: 1.15 $"

#include <sys/types.h>

#include "aml/table_search.h"

const char *
table_search(uint_t type, table_t *table)
{
	const char *name = (const char *)"Unknown";

	while (table->string != (char *)NULL) {
		if (type == table->type) {
			name = table->string;
			break;
		}
		table++;
	}
	return (name);
}
