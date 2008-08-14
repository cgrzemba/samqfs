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
#pragma ident "$Revision: 1.1 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <stdlib.h>
#include <string.h>

#include <sam/sam_malloc.h>

#include "util.h"

char *
xstrdup(
	char *string) /* String to duplicate */
{
	char *s;

	SamMalloc(s, strlen(string)+1);
	if (s == 0) {
		return (s);
	}

	return (strcpy(s, string));
}

char *
xstrdup2(
	char *string, /* String to duplicate	*/
	int len)
{
	char *s;

	SamMalloc(s, len);
	if (s == 0) {
		return (s);
	}
	return (strcpy(s, string));
}
