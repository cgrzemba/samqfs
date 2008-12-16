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
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stddef.h>
#include <libgen.h>
#include <fcntl.h>

#include "db.h"
#include "mgmt/fsmdb.h"
#include "mgmt/restore_int.h"
#include "mgmt/file_metrics.h"
#include "pub/mgmt/restore.h"
#include "pub/mgmt/sqm_list.h"
#include "mgmt/cmn_csd.h"

static int add_file_path(fs_entry_t *fsent, DB_TXN *txn, char *path,
		boolean_t isdir, uint64_t *filno, uint64_t *parent);
static int get_file_parent(fs_db_t *fsdb, DB_TXN *txn, uint32_t snapid,
		uint64_t fid, filvar_t *fvar);
static int get_avail_file_id(DB *dbp, DB_TXN *txn, uint64_t *fid);
static int get_all_snapids(fs_entry_t *fsent, uint32_t exclude,
		uint32_t *nsnaps, uint32_t **snaps);
static int db_delete_path(fs_db_t *fsdb, DB_TXN *txn, uint64_t fid);
static int db_delete_dirs(fs_db_t *fsdb, DB_TXN *txn, uint64_t *dirIDs,
		int dirc);
static int db_curs_delete(fs_db_t *fsdb, uint32_t snapid, int nsnaps,
		uint32_t *snaps, uint64_t *dirIDs, int *dirc, uint64_t *count);
static int is_file_multiref(fs_db_t *fsdb, DB_TXN *ptxn, fuid_t fuid,
		uint32_t nsnaps, uint32_t *snaps);

static void close_snapshot(dumpspec_t *dsp);
static int set_snapshot(char *snapname, dumpspec_t *dsp);
static int fix_snap_name(char *snapname);

/*
 *  Imports filesystem information from a samfsdump snapshot file
 *  and generates metrics.
 */
int
db_process_samfsdump(fs_entry_t *fsent, char *snappath)
{
	int			st = 0;
	dumpspec_t		dsp;
	fsmsnap_t		*snapdata = NULL;
	uint32_t		snapid;
	size_t			len;
	filvar_t		filvar;
	filinfo_t		filinfo;
	filinfo_t		oldinfo;
	DB*			dbp;
	DBT			key;
	DBT			data;
	DB_TXN			*txn = NULL;
	DB_COMPACT		cstats;
	DBT			startkey;
	DBT			endkey;
	uint64_t		filno = 0;
	char			*fnam = NULL;
	filvar_t		pfvar;
	uint64_t		parentid;
	int			count = 0;
	time_t			now;
	time_t			ckp = 0;
	boolean_t		isdir = FALSE;
	sfid_t			start;
	sfid_t			end;
	fs_db_t			*fsdb;
	int			i;
	char			snapname[MAXPATHLEN + 1];
	int			mst = 0;
	void			*rptArg = NULL;
	void			*rptRes = NULL;
	boolean_t		locked = FALSE;
	int			med[4];
	boolean_t		updinfo = FALSE;
	char			errbuf[MAXPATHLEN + 1];
	char			*errpfx = "index";

	now = time(NULL);

	if ((fsent == NULL) || (snappath == NULL)) {
		LOGERR("No filesystem or recovery point specified");
		return (EINVAL);
	}

	(void) pthread_mutex_lock(&fsent->statlock);
	if (fsent->status == FSENT_DELETING) {
		st = EINTR;
		LOGERR("Request rejected for %s, deletion in progress",
		    snappath);
	} else {
		(fsent->addtasks)++;
		incr_active();
	}
	(void) pthread_mutex_unlock(&fsent->statlock);

	if (st != 0) {
		goto done;
	}

	fsdb = fsent->fsdb;

	st = set_snapshot(snappath, &dsp);
	if (st != 0) {
		LOGERR("Failed to read recovery point %s %d", snappath, st);
		goto done;
	}

	/* remove the file extension from the snappath */
	strlcpy(snapname, snappath, sizeof (snapname));
	(void) fix_snap_name(snapname);

	len = sizeof (fsmsnap_t) + strlen(snapname);
	snapdata = malloc(len);
	if (snapdata == NULL) {
		LOGERR("Out of memory");
		goto done;
	}
	memset(snapdata, 0, len);

	snapdata->snapdate = dsp.snaptime;
	(void) strcpy(snapdata->snapname, snapname);

	/*
	 * Prevent concurrent add/delete operations
	 */
	(void) pthread_mutex_lock(&fsent->lock);
	locked = TRUE;

	/*
	 * see if this snapshot already exists.  we may need to pick up
	 * and keep entering if this entry failed
	 */
	st = db_add_snapshot(fsent, snapdata, len, &snapid);

	/* don't reimport duplicate data, but asking to is not an error */
	if (st == EEXIST) {
		st = 0;
		goto done;
	} else if (st != 0) {
		LOGERR("Error adding recovery point %s %d", snapname, st);
		goto done;
	}

	/* init with root (".") entry */

	st = dbEnv->txn_begin(dbEnv, NULL, &txn, 0);

	st = add_file_path(fsent, txn, ".", TRUE, &filno, &parentid);
	if (st != 0) {
		LOGERR("addpath failed for '.' : %d", st);
		st = -1;
		goto done;
	}

	st = txn->commit(txn, 0);
	txn = NULL;
	if (st != 0) {
		LOGERR("txn commit failed for '.' : %d", st);
		goto done;
	}

	while ((read_snapfile_entry(&dsp, &fnam, &filvar, &filinfo, med))
	    == 0) {

		/* see if we've been told to cancel */
		(void) pthread_mutex_lock(&fsent->statlock);
		if (fsent->status == FSENT_DELETING) {
			st = EINTR;
			LOGERR("Index for %s interrupted for deletion",
			    snapname);
		}
		(void) pthread_mutex_unlock(&fsent->statlock);
		if (st != 0) {
			goto done;
		}

		/* fnam can be NULL if the inode was a SAM special file */
		if (fnam == NULL) {
			continue;
		}

		if (strcmp(fnam, ".") == 0) {
			free(fnam);
			fnam = NULL;
			continue;
		}

		st = dbEnv->txn_begin(dbEnv, NULL, &txn, 0);
		if (st != 0) {
			LOGERR("txn begin failed %d", st);
			goto done;
		}

		if (S_ISDIR(filinfo.perms)) {
			isdir = TRUE;
		} else {
			isdir = FALSE;
		}

		st = add_file_path(fsent, txn, fnam, isdir, &filno, &parentid);
		if (st != 0) {
			LOGERR("addpath failed for %s : %d", fnam, st);
			goto done;
		}

		/* special case for file system root = parent always == 0 */
		if (parentid != 0) {
			st = get_file_parent(fsdb, txn, snapid, parentid,
			    &pfvar);
			if (st != 0) {
				/* cleanup and get out */
				LOGERR("Could not find parent dir for %s",
				    fnam);
				goto done;
			}
			filinfo.parent_mtime = pfvar.mtime;
		}

		/* set snapid & fid */
		filvar.fuid.snapid = snapid;
		filvar.fuid.fid = filno;
		filinfo.fuid.fid = filno;

		memset(&key, 0, sizeof (DBT));
		memset(&data, 0, sizeof (DBT));

		key.data = &filvar.fuid;
		key.size = FILVAR_DATA_OFF;

		data.data = ((char *)&filvar) + FILVAR_DATA_OFF;
		data.size = FILVAR_DATA_SZ;

		dbp = fsdb->snapfileDB;

		CKPUT(dbp->put(dbp, txn, &key, &data, DB_NOOVERWRITE));
		if (st != 0) {
			LOGERR("Could not add filvar for %s %d", fnam, st);
			goto done;
		}

		/*
		 * If this same file is already in the database,
		 * check archives to see if this version has newer
		 * information before updating.  New archives do not change
		 * the modification date on the file, and we don't want
		 * to accidentally overwrite with outdated archive
		 * information.
		 */
		memset(&key, 0, sizeof (DBT));
		memset(&data, 0, sizeof (DBT));

		key.data = &filinfo.fuid;
		key.size = FILINFO_DATA_OFF;

		memset(&oldinfo, 0, sizeof (filinfo_t));

		data.data = ((char *)&oldinfo) + FILINFO_DATA_OFF;
		data.size = data.ulen = FILINFO_DATA_SZ;
		data.flags = DB_DBT_USERMEM;

		/* assumption is we'll update */
		updinfo = TRUE;

		dbp = fsdb->filesDB;

		st = dbp->get(dbp, txn, &key, &data, DB_RMW);
		if (st == 0) {
			/* chances are, they have identical arch copies... */
			updinfo = FALSE;

			/* ...but check for a newer one just in case... */
			for (i = 0; i < 4; i++) {
				if (filinfo.arch[i].archtime >
				    oldinfo.arch[i].archtime) {
					updinfo = TRUE;
				}
			}
		}

		if (updinfo == TRUE) {
			memset(&data, 0, sizeof (DBT));

			data.data = ((char *)&filinfo) + FILINFO_DATA_OFF;
			data.size = FILINFO_DATA_SZ;

			CKPUT(dbp->put(dbp, txn, &key, &data, 0));

			if (st != 0) {
				LOGERR("Could not add filinfo for %s %d",
				    fnam, st);
				goto done;
			}
		}

		/* index the vsns */
		dbp = fsdb->snapvsnDB;
		for (i = 0; i < 4; i++) {
			if (filinfo.arch[i].archtime == 0) {
				break;
			}
			memset(&key, 0, sizeof (DBT));
			memset(&data, 0, sizeof (DBT));

			key.data = &filinfo.arch[i].vsn;
			key.size = sizeof (uint32_t);

			data.data = &snapid;
			data.size = sizeof (uint32_t);

			CKPUT(dbp->put(dbp, txn, &key, &data, DB_NODUPDATA));
			if (st != 0) {
				LOGERR("Could not add VSN for %s %d", fnam, st);
				goto done;
			}
		}

		st = txn->commit(txn, 0);
		txn = NULL;
		if (st != 0) {
			LOGERR("txn commit failed for %s : %d", fnam, st);
			goto done;
		}

		/*
		 * Add this entry to the metrics.  It's possible that metrics
		 * could already exist from a prior import of this snapshot
		 * so do not regenerate if that's the case
		 */
		if ((mst == 0) && !(snapdata->flags & METRICS_AVAIL)) {
			mst = gather_snap_metrics(fsdb, snapdata, &filinfo,
			    &filvar, &rptArg, med, &rptRes);
		}

		free(fnam);
		fnam = NULL;

		count++;

		if ((count % 10000) == 0) {
			time_t  pnow = time(NULL);

			if (ckp == 0) {
				ckp = now;
			}

			(void) printf("processed %d\t%ld\t%ld\n",
			    count, (pnow - ckp), (pnow - now));
			ckp = time(NULL);
		}
	}

	/* update the snapshot entry with final values */
	snapdata->numfiles = count;
	snapdata->flags |= SNAPFLAG_READY;

	(void) db_update_snapshot(fsent, snapdata, len);

done:
	if (txn != NULL) {
		if (st != 0) {
			LOGERR("aborting final txn for %s", snapname);
			txn->abort(txn);
		} else {
			LOGERR("committing final txn for %s", snapname);
			txn->commit(txn, 0);
		}
	}
	txn = NULL;

	/*
	 * If we haven't been told to exit, keep the size small,
	 * compact this snapshot
	 */
	if ((st != EINTR) && (st != ENOSPC)) {
		int cst = 0;

		memset(&cstats, 0, sizeof (DB_COMPACT));
		memset(&start, 0, sizeof (DBT));
		memset(&end, 0, sizeof (DBT));

		cstats.compact_fillpercent = 90;
		cstats.compact_timeout = 15;

		start.fid = 0;
		start.snapid = end.snapid = snapid;
		end.fid = UINT64_MAX;

		startkey.data = &start;
		endkey.data = &end;
		startkey.size = endkey.size = FILVAR_DATA_OFF;

		LOGERR("beginning compact after indexing %s",
		    snapdata->snapname);

		cst = (fsdb->snapfileDB)->compact((fsdb->snapfileDB), txn,
		    &startkey, &endkey, &cstats, DB_FREE_SPACE, NULL);

		if (cst != 0) {
			LOGERR("failed to compact database %d", cst);
		}
#ifdef DO_DEBUG_PRINTF
		snprintf(errbuf, sizeof (errbuf), "compact deadlocks: %d",
		    cstats.compact_deadlock);
		fsmdb_log_err(dbEnv, NULL, errbuf);
		snprintf(errbuf, sizeof (errbuf), "compact levels: %d",
		    cstats.compact_levels);
		fsmdb_log_err(dbEnv, NULL, errbuf);
		snprintf(errbuf, sizeof (errbuf), "compact pages freed: %d",
		    cstats.compact_pages_free);
		fsmdb_log_err(dbEnv, NULL, errbuf);
		snprintf(errbuf, sizeof (errbuf), "compact pages trunc-ed: %d",
		    cstats.compact_pages_truncated);
		fsmdb_log_err(dbEnv, NULL, errbuf);
		snprintf(errbuf, sizeof (errbuf), "compact pages examined: %d",
		    cstats.compact_pages_examine);
		fsmdb_log_err(dbEnv, NULL, errbuf);
#endif
	}

	/* done with the parts of this operation that need to be serialized */
	if (locked) {
		/*
		 * Reset highid.  Don't want/need to maintain this between
		 * add and delete snapshot requests.
		 */
		fsent->highid = 0;
		(void) pthread_mutex_unlock(&fsent->lock);
	}

	close_snapshot(&dsp);

	if (fnam != NULL) {
		free(fnam);
	}

	/*
	 * Only save metrics if snapshot is complete
	 */
	if (snapdata != NULL) {
		if ((snapdata->flags & SNAPFLAG_READY) && (mst == 0) &&
		    !(snapdata->flags & METRICS_AVAIL)) {
			st = finish_snap_metrics(fsdb, snapdata, &rptArg,
			    &rptRes);

			/* note metrics available */
			if (st == 0) {
				snapdata->flags |= METRICS_AVAIL;

				(void) db_update_snapshot(fsent, snapdata, len);
				LOGERR("Metrics stored for %s", snapname);
			} else {
				LOGERR("Failed to store metrics for %s %d",
				    snapname, st);
			}
		} else {
			/* free any results we don't need */
			free_metrics_results(&rptArg, &rptRes);
		}

		free(snapdata);
	}

	/* mark that we're done here */
	(void) pthread_mutex_lock(&fsent->statlock);
	(fsent->addtasks)--;
	decr_active();
	(void) pthread_cond_broadcast(&fsent->statcond);
	(void) pthread_mutex_unlock(&fsent->statlock);

	return (st);
}

int
db_add_vsn(
	audvsn_t	*vsn,
	uint32_t	*vsnid)
{
	DBT		key;
	DBT		data;
	int		st;
	db_recno_t 	recno = 0;
	audvsn_t	oldvsn;
	DB_TXN		*txn = NULL;

	/*
	 * First, see if this snap is already in the database.
	 * Only applies to named snapshots.
	 */

	st = dbEnv->txn_begin(dbEnv, NULL, &txn, 0);
	if (st != 0) {
		return (st);
	}

	*vsnid = 0;

	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	key.data = vsn->vsn;
	key.size = sizeof (vsn->vsn);

	data.flags = DB_DBT_USERMEM;
	data.size = data.ulen = AUDVSN_SZ;
	data.data = &oldvsn;

	st = vsnnameDB->get(vsnnameDB, txn, &key, &data, DB_RMW);

	if (st == 0) {
		*vsnid = oldvsn.id;
		st = txn->commit(txn, 0);

		return (st);
	}

	/* id initialized to 0, will be overwritten with assigned recno */
	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	key.data = &recno;
	key.size = key.ulen = sizeof (recno);
	key.flags = DB_DBT_USERMEM;

	data.data = vsn;
	data.size = sizeof (audvsn_t);

	CKPUT(vsnDB->put(vsnDB, txn, &key, &data, DB_APPEND));

	if (st != 0) {
		vsnDB->err(vsnDB, st, "vsnDB->put");
		txn->abort(txn);
	} else {
		*vsnid = vsn->id;
		st = txn->commit(txn, 0);
	}

	return (st);
}

int
db_add_snapshot(
	fs_entry_t	*fsent,
	fsmsnap_t	*snapdata,
	size_t		len,
	uint32_t	*snapid)
{
	DBT		key;
	DBT		data;
	int		st;
	db_recno_t 	recno = 0;
	fs_db_t		*fsdb = fsent->fsdb;
	DB		*dbp = fsdb->snapDB;
	DB_TXN		*txn = NULL;

	/*
	 * First, see if this snap is already in the database.
	 * Only applies to named snapshots.
	 */

	st = dbEnv->txn_begin(dbEnv, NULL, &txn, 0);
	if (st != 0) {
		return (st);
	}

	if (snapdata->snapname != NULL) {
		memset(&key, 0, sizeof (DBT));
		memset(&data, 0, sizeof (DBT));

		key.data = snapdata->snapname;
		key.size = strlen(snapdata->snapname) + 1;

		data.flags = DB_DBT_MALLOC;

		st = (fsdb->snapidDB)->get(fsdb->snapidDB, txn,
		    &key, &data, 0);

		if (st == 0) {
			txn->commit(txn, 0);
			/*
			 * return back the existing snapshot entry contents,
			 * don't bother copying back the snapname as it's
			 * the same
			 */
			memcpy(snapdata, data.data, sizeof (fsmsnap_t));
			free(data.data);
			if (snapid != NULL) {
				*snapid = snapdata->snapid;
			}
			/* if snapshot is not complete, allow re-entry */
			if (snapdata->flags & SNAPFLAG_READY) {
				return (EEXIST);
			} else {
				return (0);
			}
		}
	}

	/* snapid initialized to 0, will be overwritten with assigned recno */
	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	key.data = &recno;
	key.size = key.ulen = sizeof (recno);
	key.flags = DB_DBT_USERMEM;

	data.data = snapdata;
	data.size = len;

	CKPUT(dbp->put(dbp, txn, &key, &data, DB_APPEND));

	if (st != 0) {
		txn->abort(txn);
	} else {
		*snapid = snapdata->snapid = recno;
		st = txn->commit(txn, 0);
	}

	return (st);
}

/*
 *  Updates the stored snapshot entry information
 */
int
db_update_snapshot(
	fs_entry_t	*fsent,
	fsmsnap_t	*snapdata,
	size_t		len)
{
	int		st;
	DBT		key;
	DBT		data;
	fs_db_t		*fsdb;
	DB		*dbp;
	DB_TXN		*txn = NULL;

	if ((fsent == NULL) || (snapdata == NULL) || (len == 0)) {
		return (EINVAL);
	}

	fsdb = fsent->fsdb;
	dbp = fsdb->snapDB;

	st = dbEnv->txn_begin(dbEnv, NULL, &txn, 0);
	if (st != 0) {
		return (st);
	}

	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	key.data = &snapdata->snapid;
	key.size = sizeof (uint32_t);

	data.data = snapdata;
	data.size = len;

	st = dbp->put(dbp, NULL, &key, &data, 0);

	if (st == 0) {
		st = txn->commit(txn, 0);
	} else {
		txn->abort(txn);
	}

	return (st);
}

/* fsent->lock must be locked from the caller to protect highid */
static int
add_file_path(
	fs_entry_t	*fsent,
	DB_TXN		*txn,
	char		*path,
	boolean_t	isdir,
	uint64_t	*filno,
	uint64_t	*parent)
{
	int		st = 0;
	fs_db_t		*fsdb = fsent->fsdb;
	DB		*dbp = fsdb->fidDB;
	DBT		key;
	DBT		data;
	path_t		*newfil = NULL;
	size_t		len;
	uint64_t	id;
	uint64_t	pd = 0;
	char		*bufp;

	if ((path == NULL) || (strlen(path) < 1)) {
		return (-1);
	}

	if (fsent->highid == 0) {
		st = get_avail_file_id(dbp, txn, &id);
		if (st == 0) {
			fsent->highid = id;
		}
	}

	if (st != 0) {
		return (st);
	}

	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	st = get_file_path_id(fsdb, txn, path, &id, &pd);

	if (st == 0) {			/* path already in database */
		*filno = id;
		*parent = pd;
		return (st);
	} else if (st != DB_NOTFOUND) {
		/* can't continue, something's very wrong */
		return (st);
	}

	/* add the new file */
	bufp = strrchr(path, '/');
	if (bufp != NULL) {
		bufp++;
	} else {
		bufp = path;
	}

	len = sizeof (uint64_t) + strlen(bufp) + 1;
	newfil = malloc(len);

	if (strcmp(path, ".") != 0) {
		id = fsent->highid;
	} else {
		/* special case root to always be fid 0 */
		id = 0;
	}

	*filno = id;
	*parent = pd;

	newfil->parent = pd;
	(void) strcpy(&newfil->pathname[0], bufp);

	key.data = &id;
	key.size = sizeof (uint64_t);

	data.size = len;
	data.data = newfil;

	st = dbp->put(dbp, txn, &key, &data, 0);

	if (st == 0) {
		if (isdir) {
			/* add to directory DB - fidDB key is dirDB data */
			memset(&data, 0, sizeof (DBT));
			data.data = path;
			data.size = strlen(path) + 1;

			st = (fsdb->dirDB)->put(fsdb->dirDB, txn, &data,
			    &key, 0);
		}
	}

	free(newfil);

	if (st == 0) {
		fsent->highid++;
	}

	return (st);
}

/* returns the next highest available file ID.  */
static int
get_avail_file_id(DB *dbp, DB_TXN *txn, uint64_t *fid)
{
	/*
	 * given that the fidDB is sorted in fileno order, we don't
	 * need to waste time storing the fileno every time.  Just get
	 * the last entry in this DB.
	 */
	int		st;
	DBT		key;
	DBT		data;
	DBC		*curs;
	DB_TXN		*ftxn = NULL;

	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	if ((st = dbEnv->txn_begin(dbEnv, txn, &ftxn, 0)) != 0) {
		return (st);
	}

	st = dbp->cursor(dbp, ftxn, &curs, 0);
	if (st != 0) {
		ftxn->abort(ftxn);
		return (st);
	}

	st = curs->c_get(curs, &key, &data, DB_LAST);
	if (st == 0) {
		*fid = *(uint64_t *)(key.data) + 1;
	} else if (st == DB_NOTFOUND) {
		/* first path */
		*fid = 0;
		st = 0;
	}

	curs->c_close(curs);
	ftxn->commit(ftxn, 0);

	return (st);
}

/* returns the file ID associated with the specified path */
int
get_file_path_id(
	fs_db_t		*fsdb,
	DB_TXN		*txn,
	char		*path,
	uint64_t	*fid,
	uint64_t	*parent)
{
	int		st;
	DBT		key;
	DBT		pkey;
	DBT		data;
	char		buf[2048];
	char		*bufp;
	uint64_t	id = 0;
	uint64_t	pd = 0;
	fake_path_t	exist;

	if ((path == NULL) || (strlen(path) < 1)) {
		return (-1);
	}

	memset(&data, 0, sizeof (DBT));
	memset(&key, 0, sizeof (DBT));
	memset(&pkey, 0, sizeof (DBT));

	/* first, find the parent directory.  Strip off "./" if it exists. */
	if ((path[0] != '\0') && (path[0] == '.') && (path[1] == '/')) {
		strlcpy(buf, path+2, sizeof (buf));
	} else {
		strlcpy(buf, path, sizeof (buf));
	}
	bufp = strrchr(buf, '/');
	if (bufp == NULL) {
		*parent = 0;
		bufp = buf;
	} else {
		*bufp = '\0';
		bufp++;

		key.data = buf;
		key.size = strlen(buf) + 1;

		data.size = data.ulen = sizeof (uint64_t);
		data.data = &pd;
		data.flags = DB_DBT_USERMEM;

		st = (fsdb->dirDB)->get(fsdb->dirDB, txn, &key, &data, 0);

		if (st != 0) {
			return (st);
		}

		*parent = pd;
	}

	/* now we can see if this one exists already */
	exist.parent = pd;
	strlcpy(exist.pathname, bufp, sizeof (exist.pathname));

	key.data = &exist;
	key.size = sizeof (uint64_t) + strlen(bufp) + 1;

	pkey.data = &id;
	pkey.size = pkey.ulen = sizeof (uint64_t);
	pkey.flags = DB_DBT_USERMEM;

	/* don't need data */
	memset(&data, 0, sizeof (DBT));
	data.flags = DB_DBT_USERMEM|DB_DBT_PARTIAL;

	st = (fsdb->pathDB)->pget(fsdb->pathDB, txn, &key, &pkey, &data, 0);

	if (st == 0) {
		/* already in database */
		*fid = id;
	}

	return (st);
}

static int
get_file_parent(fs_db_t *fsdb, DB_TXN *txn, uint32_t snapid, uint64_t fid,
	filvar_t *fvar)
{
	int		st;
	DBT		key;
	DBT		data;
	sfid_t		parent;
	DB		*dbp;
	DB_TXN		*ftxn = NULL;

	if ((fvar == NULL) || (fsdb == NULL)) {
		return (-1);
	}

	if ((st = dbEnv->txn_begin(dbEnv, txn, &ftxn, 0)) != 0) {
		return (-1);
	}

	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	parent.fid = fid;
	parent.snapid = snapid;

	key.data = &parent;
	key.size = sizeof (parent);

	data.data = fvar;
	data.ulen = sizeof (filvar_t);
	data.flags = DB_DBT_USERMEM;

	dbp = fsdb->snapfileDB;

	st = dbp->get(dbp, ftxn, &key, &data, 0);

	ftxn->commit(ftxn, 0);

	return (st);
}

/*
 *  db_get_snapshots()
 *
 *  Returns a list of snapshots in the database for a given filesystem
 *
 */
int
db_get_snapshots(fs_entry_t *fsent, uint32_t *nsnaps, char **snapnames)
{
	DBT		key;
	DBT		data;
	int		st;
	DBC		*curs;
	DB		*dbp;
	char		namebuf[MAXPATHLEN + 1];
	size_t		len = 8192;
	size_t		outlen = 0;
	char		*outbuf;
	char		*bufp;
	DB_TXN		*txn = NULL;

	if ((fsent == NULL) || (nsnaps == NULL) || (snapnames == NULL)) {
		return (-1);
	}

	dbp = fsent->fsdb->snapidDB;

	/* Acquire a cursor for the database. */
	if ((st = dbp->cursor(dbp, txn, &curs, 0)) != 0) {
		dbp->err(dbp, st, "DB->cursor");
		return (1);
	}

	*nsnaps = 0;
	*snapnames = NULL;

	outbuf = malloc(len);
	if (outbuf == NULL) {
		curs->c_close(curs);
		return (-1);
	}
	/* init the buffer for strlcat */
	*outbuf = '\0';

	/* Re-initialize the key/data pair. */
	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	key.data = namebuf;
	key.size = key.ulen = sizeof (namebuf);
	key.flags = DB_DBT_USERMEM;

	/* we only need the name (which is the key) */
	data.flags = DB_DBT_PARTIAL|DB_DBT_USERMEM;

	/* Walk through the database and print out the key/data pairs. */
	while ((st = curs->c_get(curs, &key, &data, DB_NEXT)) == 0) {
		if (namebuf[0] == '\0') {
			continue;
		}

		if ((len - outlen) < (strlen(namebuf) + 2)) {
			len += 2048;
			bufp = realloc(outbuf, len);
			if (bufp == NULL) {
				free(outbuf);
				st = -1;
				break;
			}
			outbuf = bufp;
		}
		strlcat(outbuf, namebuf, len);
		outlen = strlcat(outbuf, "\n", len);
		(*nsnaps)++;
	}

	if (st == DB_NOTFOUND) {
		st = 0;
	}

	/* Close the cursor. */
	curs->c_close(curs);

	if (st == 0) {
		*snapnames = outbuf;
	} else {
		*nsnaps = 0;
		free(outbuf);
	}

	return (st);
}

/*
 *  db_get_snapshot_status()
 *
 *  Returns an array of datastructures describing each snapshot registered
 *  in the database for a given filesystem.
 */
int
db_get_snapshot_status(
	fs_entry_t	*fsent,
	uint32_t	*nsnaps,
	char		**results,
	size_t		*len)
{
	DBT		key;
	DBT		data;
	int		st;
	DBC		*curs;
	DB		*dbp;
	fsmsnap_t	*snap;
	DB_BTREE_STAT	*statp;
	uint32_t	nkeys;
	snapspec_t	*details;
	uint32_t	i = 0;
	DB_TXN		*txn = NULL;

	if ((fsent == NULL) || (nsnaps == NULL) || (results == NULL) ||
	    (len == NULL)) {
		return (EINVAL);
	}

	*nsnaps = 0;
	*results = NULL;

	/* DB->stat must be done on a primary database */
	dbp = fsent->fsdb->snapDB;

	if ((st = dbp->stat(dbp, txn, &statp, DB_FAST_STAT)) != 0) {
		dbp->err(dbp, st, "DB->stat");
		return (1);
	}

	nkeys = statp->bt_nkeys;
	free(statp);

	if (nkeys == 0) {
		/* nothing to do */
		return (0);
	}

	/* allocate space for the results */
	*len = nkeys * sizeof (snapspec_t);

	details = malloc(*len);
	if (details == NULL) {
		return (ENOMEM);
	}

	/* allocate space for the snapshot struct - add extra for name */
	snap = malloc(sizeof (fsmsnap_t) + MAXPATHLEN);
	if (snap == NULL) {
		free(details);
		return (ENOMEM);
	}

	/* Acquire a cursor for the database. */
	dbp = fsent->fsdb->snapidDB;

	if ((st = dbp->cursor(dbp, txn, &curs, 0)) != 0) {
		dbp->err(dbp, st, "DB->cursor");
		free(snap);
		free(details);
		return (st);
	}

	/* Re-initialize the key/data pair. */
	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	key.data = snap->snapname;
	key.size = key.ulen = MAXPATHLEN;
	key.flags = DB_DBT_USERMEM;

	/* we need the name and the flags, so get the whole struct */
	data.data = snap;
	data.size = data.ulen = sizeof (fsmsnap_t) + MAXPATHLEN;
	data.flags = DB_DBT_USERMEM;

	/* Walk through the database and print out the key/data pairs. */
	while ((st = curs->c_get(curs, &key, &data, DB_NEXT)) == 0) {
		if (snap->snapname[0] == '\0') {
			/* hmm, should we allow unnamed snaps?? */
			continue;
		}

		/*
		 * in theory, more snaps could be added while we're looping
		 * so make sure we don't overrun our allocated space
		 */
		if (i >= nkeys) {
			snapspec_t	*ptr;

			*len = i * sizeof (snapspec_t);
			ptr = realloc(details, *len);
			if (ptr == NULL) {
				st = ENOMEM;
				break;
			}

			details = ptr;
			nkeys++;
		}

		/* set the snap specific details */
		details[i].fildmp = details[i].logfil = NULL;
		details[i].byteswapped = FALSE;	/* we don't know this here */
		details[i].csdversion = 0;	/* also unknown */
		details[i].snaptime = snap->snapdate;
		details[i].numEntries = snap->numfiles;
		if (snap->flags & SNAPFLAG_READY) {
			details[i].snapState = INDEXED;
		} else if (snap->flags & METRICS_AVAIL) {
			details[i].snapState = METRICS;
		} else {
			details[i].snapState = DAMAGED;
		}
		strlcpy(details[i].snapname, snap->snapname,
		    sizeof (details[i].snapname));

		i++;
	}

	if (st == DB_NOTFOUND) {
		st = 0;
	}

	/* Close the cursor. */
	curs->c_close(curs);

	free(snap);

	if (st == 0) {
		*results = (char *)details;
		*nsnaps = i;
	} else {
		free(details);
	}

	return (st);
}

/* used when deleting directory entries */
#define	MAX_DEL_DIRS 60000

/*
 *  db_delete_snapshot()
 *
 *  Removes a snapshot and all of its entries from the database.
 *  If a path and/or VSN is no longer referenced, it is also deleted.
 *
 *  Called in a separate thread to allow the caller to return while
 *  work proceeds.
 *
 *  TODO:  This function is as-yet very inefficient, particularly with
 *	   respect to lock handling.  While it's working for now, it
 *	   needs to be reworked to be less resource-intensive.
 */
void*
db_delete_snapshot(void* arg)
{
	int		st;
	delete_snap_t	*delete_arg = (delete_snap_t *)arg;
	fs_entry_t	*fsent = NULL;
	DBT		key;
	DBT		data;
	DB		*dbp;
	fsmsnap_t	*snapinfo = NULL;
	uint32_t	nsnaps = 0;
	uint32_t	*snaparray = NULL;
	char		snapname[MAXPATHLEN+1] = {0};
	int		snapsz;		/* size of snapinfo struct in db */
	boolean_t	locked = FALSE;
	boolean_t	ckarray = TRUE;
	uint64_t	count = 0;
	uint64_t	dirIDs[MAX_DEL_DIRS];
	int		dirc = 0;
	char		*errpfx = "unindex";

	if (delete_arg != NULL) {
		fsent = delete_arg->fsent;
		strlcpy(snapname, delete_arg->snapname, sizeof (snapname));
	}

	if ((fsent == NULL) || (snapname[0] == '\0')) {
		LOGERR("Filesystem or recovery point not provided");
		goto done;
	}

	incr_active();

	(void) fix_snap_name(snapname);

	/*
	 *  1.  Mark snapshot unready
	 *  2.  Lock this filesystem so no other adds/delete's confuse
	 * 	things.
	 *  3.  for each entry in snapfiles.db
	 *  		Remove filvar
	 *		if filinfo ! referenced in by another snap
	 *  			remove filinfo {fid, mtime}
	 *		if fid not referenced
	 *  			remove path entry
	 *			if dir, remove dir entry
	 *
	 *  4.  when complete,
	 *		go through snapvsnDB, remove refs to snapid
	 *		kick off thread to purge vsn DB
	 *		(independent task because it needs to lock all
	 *		filesys databases)
	 */


	dbp = fsent->fsdb->snapidDB;

	/*  Get the snapshot flags, and clear the READY flag */
	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	key.data = snapname;
	key.size = strlen(snapname) + 1;

	data.flags = DB_DBT_MALLOC;

	st = dbp->get(dbp, NULL, &key, &data, 0);

	if (st == DB_NOTFOUND) {
		st = 0;
		LOGERR("Recovery point %s not found", snapname);
		goto done;
	}

	if (st != 0) {
		goto done;
	}

	snapinfo = data.data;
	snapsz = data.size;

	/* update the snapflags */
	snapinfo->flags &= ~SNAPFLAG_READY;	/* clear ready */
	snapinfo->flags |= SNAPFLAG_DELETING;	/* set deleting */

	(void) db_update_snapshot(fsent, snapinfo, snapsz);

	for (;;) {
		/*
		 * If status is FSENT_DELETING break out.  If there are
		 * active or pending indexing tasks, wait for those to
		 * complete before doing the deletion.  The intention is
		 * to allow indexing jobs to preempt delete jobs.
		 */
		(void) pthread_mutex_lock(&fsent->statlock);
		while ((fsent->status == FSENT_OK) && (fsent->addtasks > 0)) {
			if (locked) {
				(void) pthread_mutex_unlock(&fsent->lock);
				locked = FALSE;
			}
			/*
			 * if the number of snapshots changes while we're
			 * processing, need to refetch the array.
			 */
			if (snaparray) {
				free(snaparray);
				snaparray = NULL;
				ckarray = TRUE;
			}
			(void) pthread_cond_wait(&fsent->statcond,
			    &fsent->statlock);
		}
		if (fsent->status != FSENT_OK) {
			st = EINTR;
			(void) pthread_mutex_unlock(&fsent->statlock);
			LOGERR("Request interrupted");
			break;
		}
		(void) pthread_mutex_unlock(&fsent->statlock);

		/* Ready to delete - lock this filesystem */
		if (!locked) {
			(void) pthread_mutex_lock(&fsent->lock);
			locked = TRUE;
		}

		/*
		 * this list is used to determine if there are any other
		 * references to a specific file version.
		 */
		if (ckarray) {
			st = get_all_snapids(fsent, snapinfo->snapid, &nsnaps,
			    &snaparray);
			if (st != 0) {
				LOGERR(
				    "Failed to get list of recovery points %d",
				    st);
				goto done;
			}
			ckarray = FALSE;
		}

		st = db_curs_delete(fsent->fsdb, snapinfo->snapid, nsnaps,
		    snaparray, dirIDs, &dirc, &count);

		if ((st != 0) && (st != DB_NOTFOUND)) {
			LOGERR("Removing entries failed %d", st);
			break;
		}

		/* do directory deletes in bulk */
		if (((st == 0) && (dirc > (MAX_DEL_DIRS - 20000))) ||
		    ((st == DB_NOTFOUND) && (dirc > 0))) {
			(void) db_delete_dirs(fsent->fsdb, NULL, dirIDs, dirc);
			dirc = 0;
		}

		if (st == DB_NOTFOUND) {
			break;
		}
	}

	/* clean up any directory paths if we broke out prematurely. */
	if (dirc != 0) {
		(void) db_delete_dirs(fsent->fsdb, NULL, dirIDs, dirc);
	}

	if (st == DB_NOTFOUND) {
		st = 0;
	}

	if (st != 0) {
		goto done;
	}

#if TODO
	/* only do this after all the files have been removed */
	db_delete_snapshot_vsns(fsent, NULL, snapinfo->snapid);
#endif

#ifdef TODO
	/* should we compress snapfiles here? */

	/* kick off task to sort out the global VSN DB too */
#endif

done:
	if ((st == 0) && (snapinfo != NULL)) {
		/* update the snapflags to indicate we're finished deleting */
		snapinfo->flags &= ~SNAPFLAG_DELETING;
		(void) db_update_snapshot(fsent, snapinfo, snapsz);

		free(snapinfo);
	}

	if (locked) {
		(void) pthread_mutex_unlock(&fsent->lock);
	}

	if (arg != NULL) {
		free(arg);
	}

	if (snaparray != NULL) {
		free(snaparray);
	}

	decr_active();

	done_with_db_task(fsent);

	return (NULL);
}

/*
 * helper function for db_delete_snapshot.  Iterates through the main
 * cursor.
 */
static int
db_curs_delete(
	fs_db_t		*fsdb,
	uint32_t	snapid,
	int		nsnaps,
	uint32_t	*snaps,
	uint64_t	*dirIDs,
	int		*dirc,
	uint64_t	*count
)
{
	int		st;
	int		st2;
	DB_TXN		*stxn;
	DB_TXN		*txn;
	DBC		*curs;
	DBT		key;
	DBT		data;
	DBT		fkey;
	DBT		fdata;
	filvar_t	filvar;
	filinfo_t	filinfo;
	fuid_t		fuid;
	uint32_t	fl;
	DB		*dbp;
	DB		*fdb;
	char		*errpfx = "rmidxnode";

	dbp = fsdb->snapfileDB;
	fdb = fsdb->filesDB;

	/* Transaction control for the snapfile cursor */
	dbEnv->txn_begin(dbEnv, NULL, &stxn, 0);

	st = dbp->cursor(dbp, stxn, &curs, 0);
	if (st != 0) {
		stxn->abort(stxn);
		LOGERR("Could not create cursor %d", st);
		return (st);
	}

	/* initialize key for walking the snapshot entries */
	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	filvar.fuid.snapid = snapid;
	filvar.fuid.fid = 0;

	key.data = &(filvar.fuid);
	key.size = key.ulen = FILVAR_DATA_OFF;
	key.flags = DB_DBT_USERMEM;

	data.data = ((char *)&filvar) + FILVAR_DATA_OFF;
	data.size = data.ulen = FILVAR_DATA_SZ;
	data.flags = DB_DBT_USERMEM;

	/* set the flags for the cursor */
	fl = DB_SET_RANGE|DB_RMW;

	/*
	 * Cursors accumulate locks over their lifetime, and there is
	 * no way to release them except by closing the cursor.  Since
	 * we're operating on a potentially huge dataset, close it
	 * periodically and restart.
	 */
	while ((st = curs->c_get(curs, &key, &data, fl)) == 0) {
		if (filvar.fuid.snapid != snapid) {
			/* done with snapshot */
			st = DB_NOTFOUND;
			break;
		}
		fl = DB_NEXT|DB_RMW;

		/* delete the reference from snapfiles */
		st = curs->c_del(curs, 0);
		if (st != 0) {
			/* don't remove any of the other bits */
			LOGERR("Could not delete node %d, status = %d",
			    filvar.fuid.fid, st);
			continue;
		}

		/* see if other snapshots reference this file */
		fuid.fid = filvar.fuid.fid;
		fuid.mtime = filvar.mtime;

		st = is_file_multiref(fsdb, stxn, fuid, nsnaps, snaps);

		/* 2 means another snapshot references this exact file */
		if (st < 2) {
			/* delete from filesDB */

			memset(&fkey, 0, sizeof (DBT));

			fkey.data = &fuid;
			fkey.size = FILINFO_DATA_OFF;

			/* start the file-specific sub transaction */
			dbEnv->txn_begin(dbEnv, NULL, &txn, 0);
			/*
			 * we need the file perms if we're going to
			 * delete the path
			 */
			if (st == 0) {
				memset(&fdata, 0, sizeof (DBT));
				fdata.data = (char *)&filinfo +
				    FILINFO_DATA_OFF;
				fdata.size = fdata.ulen = FILINFO_DATA_SZ;
				fdata.flags = DB_DBT_USERMEM;

				st2 = fdb->get(fdb, txn, &fkey, &fdata, DB_RMW);
			}

			st2 = fdb->del(fdb, txn, &fkey, 0);

			if (st2 == DB_NOTFOUND) {
				st2 = 0;
			}

			if (st2 == 0) {
				st2 = txn->commit(txn, 0);
			} else {
				txn->abort(txn);
			}
			if (st2 != 0) {
				st = st2;
				break;
			}
		}

		if ((st == 0) && (fuid.fid != 0)) {
			/* ok to delete the path too */
			st = db_delete_path(fsdb, NULL, fuid.fid);
			if ((st == 0) && (S_ISDIR(filinfo.perms))) {
				dirIDs[*dirc] = fuid.fid;
				(*dirc)++;
			}
		}

		/* reset st so we don't falsely return an error */
		st = 0;

		(*count)++;

		if ((*count % 20000) == 0) {
			break;
		}
	}

	/* close cursor and commit transaction */
	curs->c_close(curs);
	/*
	 * track commit failures, but maintain the "we're done" status
	 * indicated by DB_NOTFOUND.
	 */
	if ((st == 0) || (st == DB_NOTFOUND)) {
		st2 = stxn->commit(stxn, 0);
		if (st2 != 0) {
			LOGERR("Failed to commit txn %d", st2);
			st = st2;
		}
	} else {
		stxn->abort(stxn);
		LOGERR("Failed, status = %d", st);
	}

	return (st);
}

/*
 *  get_all_snapids()
 *
 *  Returns an allocated list of snapshot IDs
 *
 */
static int
get_all_snapids(
	fs_entry_t	*fsent,
	uint32_t	exclude,
	uint32_t	*nsnaps,
	uint32_t	**snaps)
{
	int		st;
	DBT		key;
	DBT		data;
	uint32_t	*snaparray;
	uint32_t	num;
	uint32_t	i;
	DBC		*curs;
	DB		*dbp;
	DB_BTREE_STAT	*stats;
	uint8_t		snapflags;

	if ((fsent == NULL) || (nsnaps == NULL) || (snaps == NULL)) {
		return (-1);
	}

	*nsnaps = 0;
	*snaps = NULL;

	/* first, get a count of snapshots */
	dbp = fsent->fsdb->snapDB;
	st = dbp->stat(dbp, NULL, &stats, DB_FAST_STAT);
	if (st == 0) {
		num = stats->bt_nkeys;
		free(stats);
	} else {
		/* guess - should never happen */
		num = 500;
	}

	/* cursor for traversing the snapshot database */
	dbp = fsent->fsdb->snapDB;
	st = dbp->cursor(dbp, NULL, &curs, 0);
	if (st != 0) {
		return (st);
	}

	/* allocate array */
	snaparray = malloc(num * sizeof (uint32_t));
	if (snaparray == NULL) {
		curs->c_close(curs);
		return (-1);
	}

	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	i = 0;
	key.data = &i;
	key.size = key.ulen = sizeof (uint32_t);
	key.flags = DB_DBT_USERMEM;

	/* only need the flags */
	data.data = &snapflags;
	data.size = data.ulen = data.dlen = sizeof (uint8_t);
	data.doff = offsetof(fsmsnap_t, flags);
	data.flags = DB_DBT_PARTIAL|DB_DBT_USERMEM;

	num = 0;

	/* we only want snapshots with file data */
	while ((st = curs->c_get(curs, &key, &data, DB_NEXT)) == 0) {
		if (!(snapflags & SNAPFLAG_READY)) {
			continue;
		}

		if (i == exclude) {
			continue;
		}

		snaparray[num] = i;
		num++;
	}
	curs->c_close(curs);

	*nsnaps = num;
	*snaps = snaparray;

	return (0);
}

/*
 *  db_delete_path()
 *
 *  Removes a path from the path.  It is the callers responsibility to
 *  ensure that this operation won't clobber a referenced file.
 *
 */
static int
db_delete_path(fs_db_t *fsdb, DB_TXN *txn, uint64_t fid)
{
	int		st;
	DBT		key;
	DB		*dbp;
	DB_TXN		*ftxn;
	char		*errpfx = "rmpath";


	if (fsdb == NULL) {
		return (-1);
	}

	dbp = fsdb->fidDB;

	memset(&key, 0, sizeof (DBT));

	key.data = &fid;
	key.size = sizeof (uint64_t);

	dbEnv->txn_begin(dbEnv, txn, &ftxn, 0);
	st = dbp->del(dbp, ftxn, &key, 0);
	if (st == DB_NOTFOUND) {
		st = 0;
	}
	if (st == 0) {
		st = ftxn->commit(ftxn, 0);
	} else {
		LOGERR("Failed, status = %d", st);
		st = ftxn->abort(ftxn);
	}

	return (st);
}

/*
 *  db_delete_dirs()
 *
 *  Deletes the full directory name.
 */
static int
db_delete_dirs(fs_db_t *fsdb, DB_TXN *txn, uint64_t *dirIDs, int dirc)
{
	int		st = 0;
	int		st2;
	DBT		data;
	DBT		key;
	DB		*dbp;
	DB_TXN		*ftxn;
	DBC		*curs;
	uint64_t	dirid = 0;
	int		i;
	int		count = 0;
	char		buf[2048];
	int		rmvd = 0;
	uint32_t	fl;

	if (fsdb == NULL) {
		return (-1);
	}

	if (dirc == 0) {
		return (0);
	}

	dbp = fsdb->dirDB;

	/*
	 * More issues with lock consumption here.  Make sure
	 * the cursor doesn't run too long before restarting it.
	 */

	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	/* don't need the name */
	key.flags = DB_DBT_USERMEM|DB_DBT_PARTIAL;

	data.data = &dirid;
	data.size = data.ulen = sizeof (uint64_t);
	data.flags = DB_DBT_USERMEM;

	fl = DB_FIRST|DB_RMW;

	while ((st == 0) && (rmvd < dirc)) {
		dbEnv->txn_begin(dbEnv, txn, &ftxn, 0);
		dbp->cursor(dbp, ftxn, &curs, 0);

		st = curs->c_get(curs, &key, &data, fl);

		/*
		 * don't get the full dir path every c_get.  Only
		 * needed when we're resetting the cursor.
		 */
		memset(&key, 0, sizeof (DBT));
		key.flags = DB_DBT_USERMEM|DB_DBT_PARTIAL;

		for (count = 0; count < 20000; count++) {
			if (st != 0) {
				break;
			}
			for (i = 0; i < dirc; i++) {
				if (dirid == dirIDs[i]) {
					/* found it */
					st2 = curs->c_del(curs, 0);
					rmvd++;
					break;
				}
			}
			if (rmvd == dirc) {
				/* found em all, stop now */
				break;
			}
			st = curs->c_get(curs, &key, &data, DB_NEXT|DB_RMW);
		}

		if (st == 0) {
			/*
			 * grab our current position so we can start where we
			 * left off
			 */
			memset(&key, 0, sizeof (DBT));
			key.data = buf;
			key.size = key.ulen = sizeof (buf);
			key.flags = DB_DBT_USERMEM;

			st = curs->c_get(curs, &key, &data, DB_CURRENT|DB_RMW);

			/* set up the flags for the next c_get */
			fl = DB_SET_RANGE|DB_RMW;
		}

		/* always need to close the cursor and commit */
		curs->c_close(curs);
		st2 = ftxn->commit(ftxn, 0);

		if (st2 != 0) {
			st = st2;
			break;
		}
	}

	if (st == DB_NOTFOUND) {
		st = 0;
	}

	return (st);
}

/*
 *  is_file_multiref()
 *
 *  Checks the snapfileDB to see if the file is referenced more than once
 *
 *  Returns:
 *	0	No references to this File ID found in any snapshot
 *	1	Reference found, but modified time is different
 *	2	Reference found, modified time matched
 *
 *	If 2, the caller may not delete.
 *	If 1, the caller may delete the filinfo_t from fileDB
 *	If 0, the caller may delete both filinfo_t and path_t from pathDB.
 */
static int
is_file_multiref(fs_db_t *fsdb, DB_TXN *ptxn, fuid_t fuid, uint32_t nsnaps,
	uint32_t *snaps)
{
	int		st;
	DBT		key;
	DBT		data;
	DB		*dbp;
	sfid_t		sfid;
	uint32_t	i;
	int		count = 0;
	DB_TXN		*txn;
	uint32_t	mtime = 0;
	int		exact = 0;

	if ((fsdb == NULL) || (snaps == NULL)) {
		return (-1);
	}

	dbp = fsdb->snapfileDB;

	sfid.fid = fuid.fid;

	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	key.data = &sfid;
	key.size = sizeof (sfid_t);

	/* get only the mtime */
	data.data = &mtime;
	data.size = data.ulen = data.dlen = sizeof (uint32_t);
	data.doff = offsetof(filvar_t, mtime) - FILVAR_DATA_OFF;
	data.flags = DB_DBT_PARTIAL|DB_DBT_USERMEM;

	dbEnv->txn_begin(dbEnv, ptxn, &txn, 0);
	for (i = 0; i < nsnaps; i++) {
		sfid.snapid = snaps[i];

		st = dbp->get(dbp, txn, &key, &data, 0);
		if (st == 0) {
			if (mtime == fuid.mtime) {
				exact++;
			} else {
				count++;
			}
		}

		if (exact > 0) {
			break;
		}
	}
	txn->commit(txn, 0);

	if (exact) {
		return (2);
	} else if (count) {
		return (1);
	} else {
		return (0);
	}
}

/*
 * Derived from set_dump() in samrdumps.c.  Added to remove dependency on
 * libfsmgmt.so.  If libsamut ever converts to using the same version of
 * Sleepycat as we do, this restriction, and these replacement functions,
 * may be removed and the link to libfsmgmt.so reestablished.
 */
static int
set_snapshot(char *snapname, dumpspec_t *dsp)
{
	int		st;
	int		fd = -1;
	csd_hdrx_t	hdr;
	boolean_t	datapossible;


	if ((snapname == NULL) || (dsp == NULL)) {
		return (-1);
	}

	memset(dsp, 0, sizeof (dumpspec_t));

	/* requires fully-qualified path to snap */
	fd = open64(snapname, O_RDONLY);
	if (fd == -1) {
		return (-1);
	}

	dsp->fildmp = gzdopen(fd, "rb");
	if (dsp->fildmp == NULL) {
		close(fd);
		return (-1);
	}

	st = common_get_csd_header(dsp->fildmp, &dsp->byteswapped,
	    &datapossible, &hdr);
	if (st != 0) {
		gzclose(dsp->fildmp);
		return (-1);
	}

	dsp->csdversion = hdr.csd_header.version;
	dsp->snaptime = hdr.csd_header.time;

	return (0);
}

/*
 *  function may not be necessary, depends on if other fields in dsp are
 *  used later on.
 */
static void
close_snapshot(dumpspec_t *dsp)
{
	if (dsp->fildmp != NULL) {
		gzclose(dsp->fildmp);
	}
}

/* helper function to manipulate snapshot names */
static int
fix_snap_name(char *snapname)
{
	char	*namep;

	if (snapname == NULL) {
		return (-1);
	}

	/* remove the file extension from the snappath */
	namep = strstr(snapname, ".dmp");
	if (namep != NULL) {
		if ((namep[4] == '\0') || (namep[4] == '.')) {
			*namep = '\0';
		}
		/* handle locked snaps too */
		namep = strstr(snapname, ".lk");
		if (namep != NULL) {
			*namep = '\0';
		}
	}

	return (0);
}

/* retrieves the fsmsnap_t structure for a snapshot if available */
int
get_snap_by_name(
	fs_db_t		*fsdb,
	char		*snapname,		/* IN */
	fsmsnap_t	**snapdata)		/* OUT */
{
	int		st;
	DBT		key;
	DBT		data;
	DB		*dbp;

	if ((fsdb == NULL) || (snapname == NULL) || (snapdata == NULL)) {
		return (EINVAL);
	}

	dbp = fsdb->snapidDB;

	memset(&key, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	key.data = snapname;
	key.size = strlen(snapname) + 1;

	data.flags = DB_DBT_MALLOC;

	st = dbp->get(dbp, NULL, &key, &data, 0);

	if (st == 0) {
		*snapdata = data.data;
	} else {
		*snapdata = NULL;
	}

	return (st);
}
