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

#pragma ident "$Revision: 1.2 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include <sam/sam_malloc.h>
#include <sam/sam_trace.h>
#include <sam/sam_db.h>

/*
 * sam_db_context_conf_new - Create a new context structure using
 *  information from config entry.
 *
 *  return: a newly allocated context populated with values from
 *          configuration.  Must be freed using sam_db_context_free.
 */
sam_db_context_t *
sam_db_context_conf_new(
	sam_db_conf_t *conf)
{
	return sam_db_context_new(
	    conf->db_host,
	    conf->db_user,
	    conf->db_pass,
	    conf->db_name,
	    *conf->db_port != '\0' ?
	    atoi(conf->db_port) : SAMDB_DEFAULT_PORT,
	    *conf->db_client != '\0' ?
	    atoi(conf->db_client) : SAMDB_CLIENT_FLAG,
	    conf->db_mount);
}

/*
 * sam_db_context_new - Create a new context structure using
 *  provided values.
 *
 *  return: a newly allocated context populated with values from
 *          configuration.  Must be freed using sam_db_context_free.
 */
sam_db_context_t *
sam_db_context_new(
	char *host,
	char *user,
	char *pass,
	char *dbname,
	unsigned int port,
	unsigned long client_flag,
	char *sam_mount)
{
	sam_db_context_t *con;
	SamMalloc(con, sizeof (sam_db_context_t));

	if (con == NULL) {
		return (NULL);
	}

	memset(con, 0, sizeof (sam_db_context_t));

	SamStrdup(con->host, host);
	SamStrdup(con->user, user);
	SamStrdup(con->pass, pass);
	SamStrdup(con->dbname, dbname);
	con->port = port;
	con->client_flag = client_flag;
	SamMalloc(con->qbuf, SAMDB_SQL_MAXLEN);
	*con->qbuf = '\0';
	SamStrdup(con->mount_point, sam_mount);
	con->sam_fd = -1;

	return (con);
}

/*
 * sam_db_context_free - Frees a context, disconnecting from
 *   database if necessary.
 */
void
sam_db_context_free(sam_db_context_t *con)
{
	if (con != NULL) {
		if (con->mysql != NULL) {
			sam_db_disconnect(con);
		}
		if (con->sam_fd >= 0) {
			close(con->sam_fd);
			con->sam_fd = -1;
		}
		SamFree(con->mount_point);
		SamFree(con->qbuf);
		SamFree(con->dbname);
		SamFree(con->pass);
		SamFree(con->user);
		SamFree(con->host);
		SamFree(con);
	}
}

/*
 * sam_db_connect - Initialize mySQL connection
 *
 *	Description:
 *	    Opens mount point file descriptor
 *	    Creates the initial connection to the database.
 *
 *	On Return:
 *	    mysql connection initialized and connected
 */
int
sam_db_connect(
	sam_db_context_t *con)	/* SAM DB connect control */
{
	if (con->sam_fd < 0) {
		if ((con->sam_fd = open(con->mount_point, O_RDONLY)) < 0) {
			Trace(TR_ERR, "Can't open mount point: %s\n",
			    con->mount_point);
			return (-1);
		}
	}

	SamMalloc(con->mysql, sizeof (MYSQL));

	if (mysql_init(con->mysql) == NULL) {
		Trace(TR_ERR, "Unable to initialize mysql");
		return (-1);
	}

	if (mysql_real_connect(con->mysql, con->host, con->user,
	    con->pass, con->dbname, con->port, NULL,
	    con->client_flag) == NULL) {
		Trace(TR_ERR, "mysql connect fail: %s",
		    mysql_error(con->mysql));
		return (-1);
	}

	return (0);
}

/*
 * sam_db_disconnect - Disconnect the mySQL connection.
 *
 *	Description:
 *	    Closes mount point file descriptor
 *	    Terminates connection to the database.
 *
 *	On Return:
 *	    Mount point closed and sam_fd set to -1
 *	    Database disconnected and mysql connection set to null.
 */
int
sam_db_disconnect(sam_db_context_t *con)
{
	if (con != NULL) {
		if (con->sam_fd >= 0) {
			close(con->sam_fd);
			con->sam_fd = -1;
		}

		if (con->mysql != NULL) {
			int i;
			for (i = 0; i < con->cache_size; i++) {
				mysql_stmt_close(con->stmt_cache[i].stmt);
			}
			con->cache_size = 0;
			mysql_close(con->mysql);
			SamFree(con->mysql);
			con->mysql = NULL;
		}
	}
	return (0);
}
