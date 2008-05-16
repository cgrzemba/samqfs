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
#pragma ident	"$Revision: 1.6 $"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

/*
 * Replacement functions and/or stubs for those in libfsmgmt.so
 * Only required until we can link against libfsmgmt or we reorganize
 * things so it's not a problem.
 */

typedef void ctx_t;
typedef char upath_t[128];
char *junkStr = "";

/*
 * lint complains about the ctx structure definition.  Since we never
 * actually use create_dir(), just stop lint from complaining.
 */
#ifndef	__lint
int
create_dir(ctx_t *ctx, upath_t dir) /* ARGSUSED */
{
	return (-1);
}
#endif	/* __lint */

char *
GetCustMsg(int msgNum)	/* ARGSUSED */
{
	return (junkStr);
}


void
_Trace(
	const int	type,			/* ARGSUSED */
	const char	*SrcFile,		/* ARGSUSED */
	const int	SrcLine,		/* ARGSUSED */
	const char	*fmt, ...)		/* ARGSUSED */
{
}
