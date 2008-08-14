/*
 *  ----	vsn_cache.c - VSN caching.
 *
 *	Description:
 *	    Caching routines for managing VSNs.
 *
 *	Contents:
 *	    Initialize_VSN_Cache - Initialize cache.
 *	    Find_VSN		 - Find VSN in cache.
 *	    Insert_VSN		 - Insert VSN into cache.
 *	    Load_VSNs_From_DB	 - Load VSNs from database.
 *
 *	Exports:
 *	    VSN_Cache	VSN cache.
 *	    n_vsns	Number of VSNs in cache.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.1 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <sam/sam_trace.h>
#include <sam/sam_malloc.h>
#include <sam/sam_db.h>

/* VSN Cache */
static int a_vsns; /* Number of allocated VSNs */
static int l_vsns[MAX_LEVELS+1]; /* Last VSN for each level */

void
sam_db_init_vsn_cache(void)
{
	int i;

	n_vsns = 0;
	a_vsns = L_VSN_CACHE;
	SamMalloc(VSN_Cache, a_vsns * sizeof (vsn_cache_t));
	for (i = 0; i < MAX_LEVELS; i++) {
		l_vsns[i] = 0;
	}
}

int
sam_db_find_vsn(
	int level,		/* Archive level */
	char *media,	/* Media type */
	char *vsn)		/* VSN */
{
	int i;

	if (n_vsns == 0) {
		return (0);
	}
	if (strcasecmp(VSN_Cache[l_vsns[level]].media, media) == 0 &&
	    strcasecmp(VSN_Cache[l_vsns[level]].vsn, vsn) == 0) {
		return (VSN_Cache[l_vsns[level]].vsn_id);
	}

	/* Search cache */
	for (i = 0; i < n_vsns; i++) {
		if (strcasecmp(VSN_Cache[i].media, media) == 0 &&
		    strcasecmp(VSN_Cache[i].vsn, vsn) == 0) {
			l_vsns[level] = i;
			return (VSN_Cache[i].vsn_id);
		}
	}

	return (0);
}

int
sam_db_cache_vsn(
	int vsn_id,		/* VSN record ordinal */
	char *media,	/* Media type */
	char *vsn)		/* VSN */
{
	if (n_vsns == a_vsns) {
		a_vsns += L_VSN_CACHE_INC;
		SamRealloc(VSN_Cache, a_vsns * sizeof (vsn_cache_t));
	}

	VSN_Cache[n_vsns].vsn_id = vsn_id;
	strcpy(VSN_Cache[n_vsns].media, media);
	strcpy(VSN_Cache[n_vsns].vsn, vsn);
	n_vsns++;

	if (SAMDB_Debug) {
		Trace(TR_DEBUG, "VSN cache insert %d %s:%s = %d\n", n_vsns,
		    media, vsn, vsn_id);
	}

	return (vsn_id);
}

int
sam_db_load_vsns(void)
{
	my_ulonglong rc;		/* Record count from query */
	unsigned long long rcc;	/* Record count processed */
	unsigned long fc;		/* Field count */

	MYSQL_RES *res;	/* mySQL fetch results */
	MYSQL_ROW row;	/* mySQL row result */
	char *q;
	int err; /* Error count	*/

	err = 0;

	q = strmov(SAMDB_qbuf, "SELECT id, media_type, vsn FROM ");
	q = strmov(q, T_SAM_VSNS);
	q = strmov(q, " ORDER BY media_type, vsn");
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Debug) {
		Trace(TR_DEBUG, "%s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "select fail [%s]: %s",
		    T_SAM_VSNS, mysql_error(SAMDB_conn));
		return (-1);
	}

	if ((res = mysql_store_result(SAMDB_conn)) == NULL) {
		Trace(TR_ERR, "results fail [%s]: %s", T_SAM_VSNS,
		    mysql_error(SAMDB_conn));
		return (-1);
	}

	rc = mysql_num_rows(res);
	fc = mysql_num_fields(res);
	rcc = 0;

	if (fc != 3) {
		Trace(TR_ERR, "results fail [%s]: field count "
		    "expected 3, got %d", T_SAM_VSNS, fc);
		return (-1);
	}

	if (rc == 0) {
		goto novsns;
	}

	for (rcc = 0; (row = mysql_fetch_row(res)) != NULL; rcc++) {
		if (row[0] == NULL) {
			Trace(TR_ERR, "Mising id [%s] row=%ld",
			    T_SAM_VSNS, rcc);
			err++;
			continue;
		}
		if (row[1] == NULL) {
			Trace(TR_ERR, "Mising media_type [%s] row=%ld",
			    T_SAM_VSNS, rcc);
			err++;
			continue;
		}
		if (row[2] == NULL) {
			Trace(TR_ERR, "Mising vsn [%s] row=%ld",
			    T_SAM_VSNS, rcc);
			err++;
			continue;
		}
		sam_db_cache_vsn(atoi(row[0]), row[1], row[2]);
	}

novsns:
	mysql_free_result(res);

	if (rcc != rc) {
		Trace(TR_ERR, "missing rows, expected: %ld, got: %ld", rc, rcc);
	}
	if (err != 0) {
		Trace(TR_ERR, "rows with errors: %d", err);
	}

	return (err != 0 ? -2 : n_vsns);
}
