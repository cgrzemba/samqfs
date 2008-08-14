/*
 * sam_db_query_error - Process query error.
 *
 *	Description:
 *	   Process query error.  If client error terminate program
 *	   with failing query and error message.
 */

/*
 *	SAM-QFS_notice_begin
 *
 *	CDDL HEADER START
 *
 *	The contents of this file are subject to the terms of the
 *	Common Development and Distribution License (the "License")
 *	You may not use this file except in compliance with the License.
 *
 *	You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 *	or http://www.opensolaris.org/os/licensing.
 *	See the License for the specific language governing permissions
 *	and limitations under the License.
 *
 *	When distributing Covered Code, include this CDDL HEADER in each
 *	file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 *	If applicable, add the following below this CDDL HEADER, with the
 *	fields enclosed by brackets "[]" replaced with your own identifying
 *	information: Portions Copyright [yyyy] [name of copyright owner]
 *
 *	CDDL HEADER END
 */

/*
 *	Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 *	Use is subject to license terms.
 *
 *	SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.1 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <mysql.h>
#include <errmsg.h>

#include <sam/sam_trace.h>
#include <sam/sam_db.h>

void sam_db_query_error(void) {
	int ec; /* Error code */

	ec = mysql_errno(SAMDB_conn);

	if (ec < CR_MIN_ERROR || ec > CR_MAX_ERROR) {
		return;
	}

	Trace(TR_ERR, "Client error query: %s", SAMDB_qbuf);
	Trace(TR_ERR, "Client error: %s", mysql_error(SAMDB_conn));
}
