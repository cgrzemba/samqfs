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
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stddef.h>
#include <libgen.h>

#include "db.h"
#include "mgmt/fsmdb.h"

/*
 *  db_check_vsn_inuse()
 *
 *  Checks if a VSN is currently registered in the database
 *
 *  Return values:
 *	0	VSN is not in use
 *	1	VSN is in use
 *	-1	an error occurred while reading the database;
 */
int
db_check_vsn_inuse(char *vsn)
{
	int		st;
	DBT		key;
	DBT		data;
	char		buf[32];

	if ((vsn == NULL) || (vsnnameDB == NULL)) {
		return (-1);
	}

	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	strlcpy(buf, vsn, sizeof (buf));

	key.data = buf;
	key.size = sizeof (buf);

	/* don't want the data - successful get tells us all we need */
	data.flags = DB_DBT_USERMEM|DB_DBT_PARTIAL;

	st = vsnnameDB->get(vsnnameDB, NULL, &key, &data, 0);

	if (st == 0) {
		return (1);
	} else if (st == DB_NOTFOUND) {
		return (0);
	}

	return (st);
}

/*
 *  db_list_all_vsns)
 *
 *  Returns a list of all VSN names registered in the database
 *
 *  The returned list is an array of char[32] buffers.
 *
 */
int
db_list_all_vsns(int *count, char **vsns)
{
	int		st;
	DBT		key;
	DBT		data;
	DBC		*curs;
	DB_BTREE_STAT	*stats;
	char		*buf;
	char		*bufp;
	char		vsnbuf[32];
	uint32_t	i = 0;
	size_t		len = 0;
	size_t		outlen = 0;

	if ((vsnDB == NULL) || (vsnnameDB == NULL) || (count == NULL) ||
	    (vsns == NULL)) {
		return (EINVAL);
	}

	*count = 0;
	*vsns = NULL;

	/*
	 * oddly, DB->stat doesn't work on secondary databases, so get the
	 * number from the vsnDB directly
	 */
	st = vsnDB->stat(vsnDB, NULL, (void *)&stats, DB_FAST_STAT);
	if (st != 0) {
		return (EINVAL);
	}

	if (stats->bt_nkeys == 0) {
		/* no VSNs */
		free(stats);
		return (0);
	}

	/* max length of a VSN is 31 characters */
	len = stats->bt_nkeys * 32;
	free(stats);

	buf = malloc(len);
	if (buf == NULL) {
		return (ENOMEM);
	}
	*buf = '\0';		/* for strlcat later */

	st = vsnnameDB->cursor(vsnnameDB, NULL, &curs, 0);

	if (st != 0) {
		free(buf);
		return (-1);
	}

	/*
	 *  The stat value is just a guess - more VSNs could be added
	 *  or deleted as we're processing.  Make sure we don't overrun
	 *  the buffer.
	 */

	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	key.data = vsnbuf;
	key.size = key.ulen = sizeof (vsnbuf);
	key.flags = DB_DBT_USERMEM;

	/* we just want the name, so we don't need the key */
	data.flags = DB_DBT_PARTIAL|DB_DBT_USERMEM;

	while ((st = curs->c_get(curs, &key, &data, DB_NEXT)) == 0) {
		if (outlen > (len - 33)) {
			/* increment by 10.  Probably not that far off */
			len += (10 * 32);
			bufp = realloc(buf, len);
			if (bufp == NULL) {
				st = ENOMEM;
				break;
			}
			buf = bufp;
		}

		strlcat(buf, vsnbuf, len);
		outlen = strlcat(buf, "\n", len);
		i++;
	}

	curs->c_close(curs);

	if (st == DB_NOTFOUND) {
		st = 0;
	}

	if (st == 0) {
		*count = i;
		*vsns = buf;
	} else {
		free(buf);
	}

	return (st);
}

/*
 *  db_get_snapshot_vsns()
 *
 *  Returns a list of VSNs referenced by a specific snapshot
 *
 */
int
db_get_snapshot_vsns(fs_entry_t *fsent, char *snapname, uint32_t *nvsns,
	char **vsns)
{
	int		st;
	DBT		key;
	DBT		data;
	DBT		skey;
	DBT		sdata;
	DBC		*curs;
	fsmsnap_t	snapinfo;
	uint32_t	*vsnarr;
	fs_db_t		*fsdb;
	DB		*dbp;
	DB_BTREE_STAT	*stats;
	uint32_t	num;
	uint32_t	vsnid;
	uint32_t	snapid;
	uint32_t	fl;
	int		i = 0;
	char		*buf;
	char		vsnname[32];

	if ((fsent == NULL) || (snapname == NULL) || (nvsns == NULL) ||
	    (vsns == NULL)) {
		return (-1);
	}

	*nvsns = 0;
	*vsns = NULL;

	fsdb = fsent->fsdb;
	dbp = fsdb->snapvsnDB;

	st = dbp->stat(dbp, NULL, (void *)&stats, DB_FAST_STAT);

	if (st != 0) {
		/* should never happen */
		num = 50;
	} else {
		num = stats->bt_nkeys;
		free(stats);
	}

	if (num == 0) {
		/* no VSNs associated with this snapshot */
		return (0);
	}

	/* get the snapshot index  */
	memset(&skey, 0, sizeof (DBT));
	memset(&sdata, 0, sizeof (DBT));

	skey.data = snapname;
	skey.size = strlen(snapname) + 1;

	sdata.data = &snapinfo;
	sdata.size = sdata.ulen = sdata.dlen = sizeof (fsmsnap_t);
	sdata.flags = DB_DBT_PARTIAL|DB_DBT_USERMEM;

	st = dbp->get(dbp, NULL, &skey, &sdata, 0);

	/* don't return data if the snapshot is incomplete */
	if ((st == 0) && (!(snapinfo.flags & SNAPFLAG_READY))) {
		st = EINVAL;
	}

	if (st != 0) {
		if (st == DB_NOTFOUND) {
			/* unknown snapshot */
			st = EINVAL;
		}
		return (st);
	}

	st = dbp->cursor(dbp, NULL, &curs, 0);
	if (st != 0) {
		return (st);
	}

	vsnarr = malloc(num * sizeof (uint32_t *));
	if (vsnarr == NULL) {
		curs->c_close(curs);
		return (ENOMEM);
	}

	/* get the vsn IDs - database key is vsnid */
	key.data = &vsnid;
	key.size = sizeof (uint32_t);
	key.flags = DB_DBT_USERMEM;

	data.data = &snapid;
	data.size = sizeof (uint32_t);
	data.flags = DB_DBT_USERMEM;

	fl = DB_NEXT;

	while ((st = curs->c_get(curs, &key, &data, fl)) == 0) {
		if (snapid == snapinfo.snapid) {
			vsnarr[i] = vsnid;
			i++;
			if (i > num) {
				break;
			}
			/* skip to the next VSN */
			fl = DB_NEXT_NODUP;
		}
		fl = DB_NEXT;
	}
	curs->c_close(curs);

	if (i == 0) {
		/* No VSNs, we're done */
		free(vsnarr);
		return (0);
	}

	/* set up the array to return */
	buf = malloc(i * 33);	/* add space for the \n */

	if (buf == NULL) {
		return (ENOMEM);
	}

	dbp = vsnDB;

	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	key.size = sizeof (uint32_t);

	data.data = vsnname;
	data.size = data.ulen = data.dlen = 32;
	data.doff = offsetof(audvsn_t, vsn);
	data.flags = DB_DBT_PARTIAL|DB_DBT_USERMEM;

	for (num = 0; num < i; num++) {
		key.data = &vsnarr[num];

		st = dbp->get(dbp, NULL, &key, &data, 0);
		if (st == 0) {
			strlcat(buf, vsnname, sizeof (buf));
			strlcat(buf, "\n", sizeof (buf));
		}
	}

	*nvsns = num;
	*vsns = buf;

	return (0);
}

#ifdef	TODO
/* Function not yet called.  Enable this when VSN database support improved */
/*
 *  db_delete_snapshot_vsns()
 *
 *  Deletes VSNs associated with a snapshot
 *
 */
int
db_delete_snapshot_vsns(fs_entry_t *fsent, DB_TXN *txn, uint32_t snapid)
{
	int		st;
	DBT		key;
	DBT		data;
	DBC		*curs;
	DB		*dbp;
	DB_COMPACT	cstats;
	uint32_t	vsnid;
	uint32_t	id;
	uint32_t	fl;

	if (fsent == NULL) {
		return (-1);
	}

	dbp = fsent->fsdb->snapvsnDB;

	st = dbp->cursor(dbp, txn, &curs, 0);
	if (st != 0) {
		return (st);
	}

	/* get the vsn IDs - database key is vsnid */
	vsnid = 0;

	key.data = &vsnid;
	key.size = sizeof (uint32_t);
	key.flags = DB_DBT_USERMEM;

	data.data = &id;
	data.size = sizeof (uint32_t);
	data.flags = DB_DBT_USERMEM;

	fl = DB_NEXT;

	while ((st = curs->c_get(curs, &key, &data, fl)) == 0) {
		if (id == snapid) {
			/* delete this one */
			curs->c_del(curs, 0);
			/* skip to the next VSN */
			fl = DB_NEXT_NODUP;
		}
		fl = DB_NEXT;
	}
	curs->c_close(curs);

	/* compact the db */
	memset(&cstats, 0, sizeof (DB_COMPACT));

	dbp->compact(dbp, txn, NULL, NULL, &cstats, DB_FREE_SPACE, NULL);

	return (0);
}
#endif	/* TODO */
