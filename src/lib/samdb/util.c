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

#pragma ident "$Revision: 1.2 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include <sam/sam_malloc.h>
#include <sam/sam_trace.h>
#include <sam/sam_db.h>

static MYSQL MM;

/*
 * sam_db_connect - Initialize mySQL connection
 *
 *	Description:
 *	    Creates the initial connection to the database.
 *
 *	On Return:
 *	    MM = mySQL context structure initialized.
 *	    Database connected.
 */
int
sam_db_connect(
	sam_db_connect_t *con)	/* SAM DB connect control */
{
	SAMDB_conn = &MM;
	if (mysql_init(SAMDB_conn) == NULL) {
		Trace(TR_ERR, "unable to initialize mysql", 0);
	}

	if (mysql_real_connect(SAMDB_conn, con->SAM_host, con->SAM_user,
	    con->SAM_pass, con->SAM_name, con->SAM_port, NULL,
	    con->SAM_client_flag) == NULL) {
		Trace(TR_ERR, "mysql connect fail: %s",
		    mysql_error(SAMDB_conn));
		return (-1);
	}

	SamMalloc(SAMDB_qbuf, SAMDB_QBUF_LEN);

	return (0);
}

/*
 * sam_db_disconnect - Disconnect the mySQL connection.
 *
 *	Description:
 *	    Terminates connection to the database.
 *
 *	On Return:
 *	    Database disconnected.
 */
int
sam_db_disconnect(void)
{
	if (SAMDB_conn != NULL) {
		mysql_close(SAMDB_conn);
		SAMDB_conn = NULL;
	}
	return (0);
}
