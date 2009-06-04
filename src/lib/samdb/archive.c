/*
 * --	archive.c - mySQL archive functions for SAM-db.
 *
 *	Descripton:
 *	    archive.c is a collecton of routines which manage
 *	    the archive table of samdb
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

#pragma ident "$Revision: 1.2 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <pub/stat.h>
#include <sam/sam_trace.h>
#include <sam/sam_db.h>
#include <sam/sam_malloc.h>
#include <sam/lib.h>
#include <sam/param.h>

#define	ARCH_REPLACE	4000
#define	ARCH_SELECT	4100
#define	ARCH_STALE	4200
#define	ARCH_DELETE	4300
#define	ARCH_DELETE_ALL	4301

static void bind_archive(MYSQL_BIND *bind, sam_db_archive_t *archive,
    boolean_t is_result);

/*
 * sam_db_archive_new - Creates a new list of archive entries for given copy
 * of a file.  Each copy may be spread across multiple vsns, one table entry
 * per vsn.
 *
 * Returns
 *    int - the number of archive entries returned
 *    archive - the list of archive entries for the specified archive copy
 */
int
sam_db_archive_new(
	sam_db_context_t *con,
	sam_id_t id,
	int copy,
	sam_db_archive_t **archive)
{
	struct sam_perm_inode perm;
	struct sam_vsn_section *mvap = NULL;
	struct sam_vsn_section *vsnp = NULL;
	sam_db_archive_t *ap = NULL;
	int nvsn;
	int i;

	if (archive == NULL) {
		Trace(TR_ERR, "Null sam_db_archive_t argument");
		return (-1);
	}

	if (sam_db_id_stat(con, id.ino, id.gen, &perm) < 0) {
		Trace(TR_ERR, "Error getting inode %d.%d stat",
		    id.ino, id.gen);
		return (-1);
	}

	/* Get multi-vsn data.  If none then copy information on inode. */
	nvsn = sam_db_id_mva(con, &perm, copy, &mvap);
	if (nvsn < 0) {
		Trace(TR_ERR, "Error getting inode multi-vsn info %d.%d",
		    id.ino, id.gen);
		if (mvap != NULL) {
			SamFree(mvap);
		}
		return (-1);
	}
	if (nvsn == 0) {
		nvsn = 1;
	}

	/* Allocate a list of archive entries for the number of vsns */
	SamMalloc(*archive, nvsn * sizeof (sam_db_archive_t));
	memset(*archive, 0, nvsn * sizeof (sam_db_archive_t));

	/*
	 * If only one vsn then the information is on the inode, otherwise
	 * use the multi-vsn data.
	 */
	if (nvsn == 1) {
		ap = *archive;
		ap->ino = id.ino;
		ap->gen = id.gen;
		ap->copy = copy;
		ap->seq = 0;
		strcpy(ap->media_type, sam_mediatoa(perm.di.media[copy]));
		strcpy(ap->vsn, perm.ar.image[copy].vsn);
		ap->position =
		    (((long long)perm.ar.image[copy].position_u << 32)) |
		    perm.ar.image[copy].position;
		ap->offset = perm.ar.image[copy].file_offset;
		ap->size = perm.di.rm.size;
		ap->create_time = perm.ar.image[copy].creation_time;
		ap->stale = perm.di.ar_flags[copy] & AR_stale ? 1 : 0;
	} else {
		for (i = 0; i < nvsn; i++) {
			ap = *archive + i;
			vsnp = mvap + i;
			ap->ino = id.ino;
			ap->gen = id.gen;
			ap->copy = copy;
			ap->seq = i;
			strcpy(ap->media_type,
			    sam_mediatoa(perm.di.media[copy]));
			strcpy(ap->vsn, vsnp->vsn);
			ap->position = vsnp->position;
			ap->offset = vsnp->offset;
			ap->size = vsnp->length;
			ap->create_time = perm.ar.image[copy].creation_time;
			ap->stale = 0;
		}
	}

	if (mvap != NULL) {
		SamFree(mvap);
	}

	return (nvsn);
}

/*
 * Frees the list of archive entries created with sam_db_archive_new
 */
void
sam_db_archive_free(sam_db_archive_t **archive)
{
	if (archive != NULL && *archive != NULL) {
		SamFree(*archive);
		*archive = NULL;
	}
}

/*
 * Inserts given archive information into database.
 *
 * Return: 0 on success, -1 on error
 */
int
sam_db_archive_replace(
	sam_db_context_t *con,
	sam_db_archive_t *archive)
{
	MYSQL_BIND bind[11];

	memset(bind, 0, sizeof (bind));
	bind_archive(bind, archive, FALSE);

	if (sam_db_execute_sql(con, bind, NULL, ARCH_REPLACE) == NULL) {
		Trace(TR_ERR, "Error inserting archive %d.%d",
		    archive->ino, archive->gen);
		return (-1);
	}

	return (0);
}

/*
 * Selects archive from database with given inode and gen number.
 *
 * Return: number entries on success, -1 on error
 * Return: newly allocated archive information copied into result
 */
int
sam_db_archive_select(
	sam_db_context_t *con,
	sam_id_t id,
	int copy,
	sam_db_archive_t **result)
{
	int err;
	int nvsn;
	int i;
	sam_db_archive_t *out_result;
	sam_db_archive_t buffer;
	MYSQL_STMT *stmt;
	MYSQL_BIND bind[3];
	MYSQL_BIND bind_result[11];

	memset(bind, 0, sizeof (bind));
	memset(bind_result, 0, sizeof (bind_result));
	out_result = NULL;

	SAMDB_BIND(bind[0], id.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], id.gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[2], copy, MYSQL_TYPE_TINY, TRUE);
	bind_archive(bind_result, &buffer, TRUE);

	if ((stmt = sam_db_execute_sql(con, bind,
	    bind_result, ARCH_SELECT)) == NULL) {
		Trace(TR_ERR, "Error selecting archive %d.%d", id.ino, id.gen);
		return (-1);
	}

	/* Use store result to know how many archive entries to allocate */
	if ((err = mysql_stmt_store_result(stmt)) != 0) {
		Trace(TR_ERR, "Error buffering results %d.%d: %s",
		    id.ino, id.gen, mysql_error(con->mysql));
		goto out;
	}

	/* Get the number of archive entries to allocate */
	nvsn = mysql_stmt_num_rows(stmt);
	if (nvsn == 0) {
		goto out;
	}

	SamMalloc(out_result, nvsn * sizeof (sam_db_archive_t));
	memset(out_result, 0, nvsn * sizeof (sam_db_archive_t));

	/* Populate the archive list */
	for (i = 0; i < nvsn; i++) {
		if ((err = mysql_stmt_fetch(stmt)) != 0) {
			Trace(TR_ERR, "Error fetching archive %d.%d: %s",
			    id.ino, id.gen, mysql_error(con->mysql));
			err = -1;
			goto out;
		}

		out_result[i] = buffer;
	}

out:
	if (mysql_stmt_free_result(stmt) != 0) {
		Trace(TR_ERR, "Error freeing result: %s",
		    mysql_error(con->mysql));
	}

	if (err == 0) {
		*result = out_result;
	} else if (out_result != NULL) {
		SamFree(out_result);
	}

	return (err == 0 ? nvsn : -1);
}

/*
 * Sets archive stale in database with given inode and gen number.
 *
 * Return number affected rows on success, -1 on error
 */
int
sam_db_archive_stale(sam_db_context_t *con, unsigned int ev_time, sam_id_t id)
{
	MYSQL_BIND bind[4];
	memset(bind, 0, sizeof (bind));
	unsigned char stale = 1;

	SAMDB_BIND(bind[0], ev_time, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], stale, MYSQL_TYPE_TINY, TRUE);
	SAMDB_BIND(bind[2], id.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[3], id.gen, MYSQL_TYPE_LONG, TRUE);

	if (sam_db_execute_sql(con, bind, NULL, ARCH_STALE) == NULL) {
		Trace(TR_ERR, "Error staling archive: %s",
		    mysql_error(con->mysql));
		return (-1);
	}

	return (mysql_affected_rows(con->mysql));
}

/*
 * Deletes archive from database with given inode, gen number
 * and copy.  If all copies should be deleted copy should be < 0.
 *
 * Return number affected rows on success, -1 on error
 */
int
sam_db_archive_delete(sam_db_context_t *con, sam_id_t id, int copy)
{
	MYSQL_BIND bind[3];
	memset(bind, 0, sizeof (bind));

	if (copy >= MAX_ARCHIVE) {
		Trace(TR_ERR, "Copy must be < MAX_ARCHIVE");
		return (-1);
	}

	SAMDB_BIND(bind[0], id.ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], id.gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[2], copy,  MYSQL_TYPE_LONG, FALSE);

	if (sam_db_execute_sql(con, bind, NULL,
	    copy < 0 ? ARCH_DELETE_ALL : ARCH_DELETE) == NULL) {
		Trace(TR_ERR, "Error deleting archive: %s",
		    mysql_error(con->mysql));
		return (-1);
	}

	return (mysql_affected_rows(con->mysql));
}

/*
 * bind_archive - Initializes the bind variable array with archive values
 */
static void
bind_archive(MYSQL_BIND *bind, sam_db_archive_t *ap, boolean_t is_result)
{
	SAMDB_BIND(bind[0], ap->ino, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[1], ap->gen, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[2], ap->copy, MYSQL_TYPE_TINY, TRUE);
	SAMDB_BIND(bind[3], ap->seq, MYSQL_TYPE_SHORT, TRUE);
	SAMDB_BIND_STR(bind[4], ap->media_type);
	SAMDB_BIND_STR(bind[5], ap->vsn);
	SAMDB_BIND(bind[6], ap->position, MYSQL_TYPE_LONGLONG, TRUE);
	SAMDB_BIND(bind[7], ap->offset, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[8], ap->size, MYSQL_TYPE_LONGLONG, TRUE);
	SAMDB_BIND(bind[9], ap->create_time, MYSQL_TYPE_LONG, TRUE);
	SAMDB_BIND(bind[10], ap->stale, MYSQL_TYPE_TINY, TRUE);

	if (is_result) {
		bind[4].buffer_length = sizeof (ap->media_type);
		bind[5].buffer_length = sizeof (ap->vsn);
	}
}
