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
#include <sys/param.h>
#include <errno.h>
#include <libgen.h>
#include <sys/mnttab.h>
#include <stdio.h>
#include <dirent.h>

#include "db.h"
#include "mgmt/fsmdb.h"
#include "mgmt/fsmdb_int.h"
#include "mgmt/file_metrics.h"
#include "mgmt/util.h"

/* function prototypes */

static int dbdir_from_fsname(char *fsname, char *buf, size_t buflen);
static void fsmdb_remove_all(char *topdir);

/* bt_* functions are used for insert sorts on BTREE databases */
static int bt_compare_uint32(DB *dbp, const DBT *in_a, const DBT *in_b);
static int bt_compare_fmRpt(DB *dbp, const DBT *in_a, const DBT *in_b);
static int bt_compare_fuid(DB *dbp, const DBT *in_a, const DBT *in_b);
static int bt_compare_sfid(DB *dbp, const DBT *in_a, const DBT *in_b);

/* helper functions for sorts */
static int compare_uint32(const uint32_t in_a, const uint32_t in_b);
static int compare_uint64(const uint64_t in_a, const uint64_t in_b);
static int compare_path_t(DB *dbp, const DBT *in_a, const DBT *in_b);
static int compare_pathname(DB *dbp, const DBT *in_a, const DBT *in_b);

/* functions to set secondary database keys */
static int index_path(DB *dbp, const DBT *key, const DBT *data, DBT *skey);
static int index_vsnname(DB *dbp, const DBT *key, const DBT *data, DBT *skey);
static int index_snapname(DB *dbp, const DBT *key, const DBT *data, DBT *skey);

/* functions for RECNO databases to set the index number persistently */
static int set_snapid(DB *dbp, DBT *data, db_recno_t recno);
static int set_vsnid(DB *dbp, DBT *data, db_recno_t recno);

#ifdef	TODO
/* when support for the vsn database is improved, will need these locks */
/* protect vsnDBs */
pthread_mutex_t	vsnLock = PTHREAD_MUTEX_INITIALIZER;

/* Macros for managing mutexes */
#define	LOCKVSN	pthread_mutex_lock(&vsnLock);
#define	UNLOCKVSN pthread_mutex_unlock(&vsnLock);
#endif	/* TODO */

DB_ENV		*dbEnv = NULL;		/* global Database environment */
DB		*vsnDB = NULL;		/* VSN database - shared across FSs */
DB		*vsnnameDB = NULL;	/* secondary VSN DB */

/*
 *  This should be done before the program is open for business.
 *  As such, it does not need to be reentrant.
 */
int
open_db_env(char *topdir)
{
	DB_ENV		*tmpenv = NULL;
	int		st;
	struct stat64	sbuf;
	char		logdir[MAXPATHLEN+1];
	char		tmpdir[MAXPATHLEN+1];
	char		*dirarr[3];
	int		i;

	if (topdir == NULL) {
		return (-1);
	}

	snprintf(logdir, sizeof (tmpdir), "%s/.logs", topdir);
	snprintf(tmpdir, sizeof (tmpdir), "%s/.tmp", topdir);

	dirarr[0] = topdir;
	dirarr[1] = logdir;
	dirarr[2] = tmpdir;

	/* first, set up the environment */
	st = db_env_create(&tmpenv, 0);

	if (st != 0) {
		return (st);
	}

	/* make sure the directories exist */
	for (i = 0; i < 3; i++) {
		st = stat64(dirarr[i], &sbuf);
		if ((st != 0) && (errno == ENOENT)) {
			st = mkdirp(dirarr[i], 0744);
			if (st == 0) {
				st = stat64(dirarr[i], &sbuf);
			}
		}
		if ((st == 0) && (!S_ISDIR(sbuf.st_mode))) {
			st = -1;
			break;
		}
	}

	if (st != 0) {
		return (st);
	}

	st = tmpenv->set_data_dir(tmpenv, topdir);
	if (st != 0) {
		return (st);
	}
	st = tmpenv->set_lg_dir(tmpenv, logdir);
	if (st != 0) {
		return (st);
	}

	st = tmpenv->set_tmp_dir(tmpenv, tmpdir);
	if (st != 0) {
		return (st);
	}

	st = tmpenv->set_flags(tmpenv, env_fl, 1);
	if (st != 0) {
		return (st);
	}

	/* overall database cache size */
	st = tmpenv->set_cachesize(tmpenv, 0, (60 * MEGA), 1);

	st = tmpenv->set_shm_key(tmpenv, FSM_SHM_MASTER_KEY);
	if (st != 0) {
		return (st);
	}

	/* log buffer in memory */
	st = tmpenv->set_lg_bsize(tmpenv, (30 * MEGA));
	if (st != 0) {
		return (st);
	}

	/* set up additional error logging */
	tmpenv->set_errcall(tmpenv, fsmdb_log_err);

	/* Increase the number of locks available */
	tmpenv->set_lk_max_locks(tmpenv, 10000);
	tmpenv->set_lk_max_lockers(tmpenv, 10000);
	tmpenv->set_lk_max_objects(tmpenv, 10000);

	/* Increase the number of concurrent transactions available */
	/* Note:  Default in 4.4-20 is '20'.  In later versions it's 100 */
	tmpenv->set_tx_max(tmpenv, 100);

	st = tmpenv->open(tmpenv, topdir, env_ofl, 0644);
	if (st != 0) {
		/* check for a major failure */
		if (st == DB_RUNRECOVERY) {
			st = tmpenv->open(tmpenv, topdir, env_ffl, 0644);
		}
		/* log catastrophic failure and remove all db files. */
		if (st == DB_RUNRECOVERY) {
			fsmdb_log_err(dbEnv, NULL,
			    "Database files corrupt, cannot recover.  "
			    "Files will be removed.  Please re-index any "
			    "recovery points to regenerate the database.");
			fsmdb_remove_all(topdir);
		}
	}

	if (st != 0) {
		return (st);
	}

	/* clear out unneeded log files */
	tmpenv->log_archive(tmpenv, NULL, DB_ARCH_REMOVE);

	/* all set, ready to use */
	dbEnv = tmpenv;

	return (st);
}

/*
 *  Open the VSN databases.  Like the DB environment, this should
 *  be called from main() as it persists across all file systems and
 *  is open for the duration of the process.  This function is NOT
 *  reentrant.
 */
int
open_vsn_db(void)
{
	int		st;
	mode_t		md = 0644;

	/* open the VSN databases */
	if (vsnDB != NULL) {
		/* already done, shouldn't happen */
		return (0);
	}

	if (dbEnv == NULL) {
		/* we should never get called with a closed environment */
		return (-1);
	}

	st = db_create(&vsnDB, dbEnv, 0);

	if (st == 0) {
		st = vsnDB->set_append_recno(vsnDB, set_vsnid);
	}

	if (st == 0) {
		st = vsnDB->open(vsnDB, NULL, "vsn.db", NULL, DB_RECNO,
		    db_fl, md);
	}

	if (st == 0) {
		st = db_create(&vsnnameDB, dbEnv, 0);
	}

	if (st == 0) {
		st = vsnnameDB->open(vsnnameDB, NULL, "vsn_name.db",
		    NULL, DB_BTREE, db_fl, md);
	}

	if (st == 0) {
		st = vsnDB->associate(vsnDB, NULL, vsnnameDB,
		    index_vsnname, DB_CREATE|DB_IMMUTABLE_KEY);
	}

	if (st != 0) {
		close_vsn_db();
	}

	return (st);
}

int
open_fs_db(
	char		*dirnam,
	char		*fsname,
	DB_ENV		*p_env,
	boolean_t	create,		/* add new databases if not existing */
	fs_db_t		*fsdb)
{
	DB		*pdb;
	int		st;
	char		buf[MAXPATHLEN+1];
	mode_t		md = 0644;
	char		namebuf[MAXPATHLEN+1] = {0};
	struct stat64	sbuf;

	memset(fsdb, 0, sizeof (fs_db_t));

	/* make sure fs-specific directory exists */
	st = dbdir_from_fsname(fsname, namebuf, sizeof (namebuf));
	if (st != 0) {
		goto err;
	}

	snprintf(buf, sizeof (buf), "%s/%s", dirnam, namebuf);

	if (!create) {
		if (stat64(buf, &sbuf) != 0) {
			return (ENOENT);
		}
	}
	(void) mkdirp(buf, 0744);

	if (fsdb->snapDB == NULL) {
		/* Create & open the snapshot database - RECNO */
		snprintf(buf, sizeof (buf), "%s/snaps.db", namebuf);

		if ((st = db_create(&fsdb->snapDB, p_env, 0)) != 0) {
			goto err;
		}

		pdb = fsdb->snapDB;

		if ((st = pdb->set_append_recno(pdb, set_snapid)) != 0) {
			goto err;
		}

		if ((st = pdb->open(pdb, NULL, buf, NULL, DB_RECNO,
		    db_fl, md)) != 0) {
			goto err;
		}

		/*  create the secondary for the snapdb */
		snprintf(buf, sizeof (buf), "%s/snapid.db", namebuf);

		if ((st = db_create(&fsdb->snapidDB, p_env, 0)) != 0) {
			goto err;
		}

		pdb = fsdb->snapidDB;

		pdb->set_bt_compare(pdb, compare_pathname);

		if ((st = pdb->open(pdb, NULL, buf, NULL, DB_BTREE, db_fl,
		    md)) != 0) {
			goto err;
		}

		if ((st = (fsdb->snapDB)->associate(fsdb->snapDB, NULL, pdb,
		    index_snapname, DB_CREATE)) != 0) {
			goto err;
		}
	}

	if (fsdb->fidDB == NULL) {
		/* Path components by FID - Primary, BTREE */
		snprintf(buf, sizeof (buf), "%s/path_fid.db", namebuf);

		if ((st = db_create(&fsdb->fidDB, p_env, 0)) != 0) {
			goto err;
		}

		pdb = fsdb->fidDB;

		pdb->set_bt_compare(pdb, bt_compare_uint64);

		if ((st = pdb->open(pdb, NULL, buf, NULL, DB_BTREE, db_fl,
		    md)) != 0) {
			goto err;
		}

		/* pathname DB - BTREE */
		snprintf(buf, sizeof (buf), "%s/path.db", namebuf);

		if ((st = db_create(&fsdb->pathDB, p_env, 0)) != 0) {
			goto err;
		}

		pdb = fsdb->pathDB;

		pdb->set_bt_compare(pdb, compare_path_t);

		if ((st = pdb->open(pdb, NULL, buf, NULL, DB_BTREE, db_fl,
		    md)) != 0) {
			goto err;
		}

		if ((st = (fsdb->fidDB)->associate(fsdb->fidDB, NULL, pdb,
		    index_path, DB_CREATE|DB_IMMUTABLE_KEY)) != 0) {
			goto err;
		}

		/*
		 * dirName DB - BTREE - full path for quick lookups
		 * Note:  This was a secondary database to fidDB, but
		 * it was impossible to delete items from this DB because
		 * we were breaking the reproducibility rules when creating
		 * the secondary index.  This db exists for performance
		 * reasons when adding files/looking up full paths.  The
		 * "most right" answer is probably a directory name cache
		 * and should be considered for a future enhancement.
		 */
		snprintf(buf, sizeof (buf), "%s/dir.db", namebuf);

		if ((st = db_create(&fsdb->dirDB, p_env, 0)) != 0) {
			goto err;
		}

		pdb = fsdb->dirDB;

		pdb->set_bt_compare(pdb, compare_pathname);

		if ((st = pdb->open(pdb, NULL, buf, NULL, DB_BTREE, db_fl,
		    md)) != 0) {
			goto err;
		}
	}

	if (fsdb->snapfileDB == NULL) {
		snprintf(buf, sizeof (buf), "%s/snapfiles.db", namebuf);

		/* snapfile DB */
		if ((st = db_create(&fsdb->snapfileDB, p_env, 0)) != 0) {
			goto err;
		}

		pdb = fsdb->snapfileDB;

		pdb->set_bt_compare(pdb, bt_compare_sfid);

		if ((st = pdb->open(pdb, NULL, buf, NULL, DB_BTREE, db_fl,
		    md)) != 0) {
			goto err;
		}
	}

	if (fsdb->filesDB == NULL) {
		snprintf(buf, sizeof (buf), "%s/files.db", namebuf);

		/* next, the db for modified files */
		if ((st = db_create(&fsdb->filesDB, p_env, 0)) != 0) {
			goto err;
		}

		pdb = fsdb->filesDB;

		pdb->set_bt_compare(pdb, bt_compare_fuid);

		if ((st = pdb->open(pdb, NULL, buf, NULL, DB_BTREE, db_fl,
		    md)) != 0) {
			goto err;
		}
	}

	/* VSNs by snapshot */
	if (fsdb->snapvsnDB == NULL) {
		snprintf(buf, sizeof (buf), "%s/snapvsn.db", namebuf);

		if ((st = db_create(&fsdb->snapvsnDB, p_env, 0)) != 0) {
			goto err;
		}

		pdb = fsdb->snapvsnDB;

		pdb->set_bt_compare(pdb, bt_compare_uint32);

		pdb->set_flags(pdb, DB_DUPSORT);

		pdb->set_dup_compare(pdb, bt_compare_uint32);

		if ((st = pdb->open(pdb, NULL, buf, NULL, DB_BTREE,
		    db_fl, md)) != 0) {
			goto err;
		}
	}

	if (fsdb->metricsDB == NULL) {
		snprintf(buf, sizeof (buf), "%s/metrics.db", namebuf);

		/* next, the db for file metrics results.  No duplicates.  */
		if ((st = db_create(&fsdb->metricsDB, p_env, 0)) != 0) {
			goto err;
		}

		pdb = fsdb->metricsDB;

		pdb->set_bt_compare(pdb, bt_compare_fmRpt);

		if ((st = pdb->open(pdb, NULL, buf, NULL, DB_BTREE, db_fl,
		    md)) != 0) {
			goto err;
		}
	}

	return (0);

err:
	close_fsdb(fsname, fsdb, FALSE);
	return (st);

}

/*
 * function used to facilitate insert sorts for BTREE databases with
 * a datastructure of 2 uint32_t as keys.
 */
static int
bt_compare_fmRpt(DB *dbp, const DBT *in_a, const DBT *in_b)	/* ARGSUSED */
{
	int		st;
	uint32_t	a = 0;
	uint32_t	b = 0;
	uint32_t	c = 0;
	uint32_t	d = 0;
	fmrpt_key_t	*fm;

	if (in_a->size) {
		fm = (fmrpt_key_t *)in_a->data;
		memcpy(&a, &(fm->rptType), 4);
		memcpy(&b, &(fm->snapid), 4);
	}

	if (in_b->size) {
		fm = (fmrpt_key_t *)in_b->data;
		memcpy(&c, &(fm->rptType), 4);
		memcpy(&d, &(fm->snapid), 4);
	}

	st = compare_uint32(a, c);
	if (st == 0) {
		st = compare_uint32(b, d);
	}

	return (st);
}

/*
 * function used to facilitate insert sorts for BTREE databases with
 * uint32_t keys.
 */
static int
bt_compare_uint32(DB *dbp, const DBT *in_a, const DBT *in_b) /* ARGSUSED */
{
	uint32_t	a = 0;
	uint32_t	b = 0;

	if (in_a->size) {
		memcpy(&a, in_a->data, 4);
	}

	if (in_b->size) {
		memcpy(&b, in_b->data, 4);
	}

	return (compare_uint32(a, b));
}

/*
 * function used to facilitate insert sorts for BTREE databases with
 * uint64_t keys.  The default insert sort algorithm is VERY inefficient
 * on little endian systems (x64) as it tries to sort lexically.
 */
int
bt_compare_uint64(DB *dbp, const DBT *in_a, const DBT *in_b) /* ARGSUSED */
{
	uint64_t	a = 0;
	uint64_t	b = 0;

	if (in_a->size) {
		memcpy(&a, in_a->data, 8);
	}

	if (in_b->size) {
		memcpy(&b, in_b->data, 8);
	}

	return (compare_uint64(a, b));
}


/*
 * function used to facilitate insert sorts for BTREE databases with
 * fuid_t and sfid_t keys.  As both of these are comprised of 1 uint64_t
 * value and 1 uint32_t value, we can use the same compare function.
 */
static int
bt_compare_fuid(DB *dbp, const DBT *in_a, const DBT *in_b) /* ARGSUSED */
{
	uint64_t 	a = 0;
	uint64_t	b = 0;
	uint32_t	c = 0;
	uint32_t	d = 0;
	fuid_t		*af;
	fuid_t		*bf;
	int		st;

	/* compare the first uint64_t */
	if (in_a->size) {
		af = (fuid_t *)(in_a->data);
		memcpy(&a, &(af->fid), 8);
		memcpy(&c, &(af->mtime), 4);
	}

	if (in_b->size) {
		bf = (fuid_t *)(in_b->data);
		memcpy(&b, &(bf->fid), 8);
		memcpy(&d, &(bf->mtime), 4);
	}

	st = compare_uint64(a, b);

	/* if they're equal, subsort on second value */
	if (st == 0) {
		/* reverse sort - newer first */
		st = compare_uint32(d, c);
	}

	return (st);
}

static int
bt_compare_sfid(DB *dbp, const DBT *in_a, const DBT *in_b) /* ARGSUSED */
{
	uint64_t 	a = 0;
	uint64_t	b = 0;
	uint32_t	c = 0;
	uint32_t	d = 0;
	sfid_t		*af;
	sfid_t		*bf;
	int		st;

	if (in_a->size) {
		af = (sfid_t *)(in_a->data);

		memcpy(&a, &(af->fid), 8);
		memcpy(&c, &(af->snapid), 4);
	}

	if (in_b->size) {
		bf = (sfid_t *)(in_b->data);

		memcpy(&b, &(bf->fid), 8);
		memcpy(&d, &(bf->snapid), 4);
	}

	/* compare the snapshot id, oldest first */
	st = compare_uint32(c, d);

	/* if they're equal, secondary sort on file id */
	if (st == 0) {
		st = compare_uint64(a, b);
	}

	return (st);
}

static int
compare_uint32(const uint32_t in_a, const uint32_t in_b)
{
	if (in_a > in_b) {
		return (1);
	} else if (in_a < in_b) {
		return (-1);
	} else {
		return (0);
	}
}

static int
compare_uint64(const uint64_t in_a, const uint64_t in_b)
{
	if (in_a > in_b) {
		return (1);
	} else if (in_a < in_b) {
		return (-1);
	} else {
		return (0);
	}
}

static int
compare_path_t(DB *dbp, const DBT *in_a, const DBT *in_b)
{
	DBT		a;
	DBT		b;
	uint64_t	ai;
	uint64_t	bi;
	int		st;

	memcpy(&ai, in_a->data, 8);
	memcpy(&bi, in_b->data, 8);

	st = compare_uint64(ai, bi);

	if (st == 0) {
		a.data = ((path_t *)(in_a->data))->pathname;
		a.size = in_a->size - 8;

		b.data = ((path_t *)(in_b->data))->pathname;
		b.size = in_b->size - 8;

		st = compare_pathname(dbp, &a, &b);
	}
	return (st);
}

/*
 *  BTREE Insertion comparison function.  Compares 2 keys with strcmp()
 *
 *  NOTE:  This needs to properly sort in relative order even comparing
 *  with empty strings.  If one string or the other is NULL, compare with
 *  the empty string to get the correct relative position.
 */
static int
compare_pathname(DB *dbp, const DBT *in_a, const DBT *in_b) /* ARGSUSED */
{
	char	*empty = "";
	char	*a = empty;
	char	*b = empty;
	int	st;

	/* this function needs to be able to evaluate empty strings. */
	if (in_a->size != 0) {
		a = (char *)in_a->data;
	}
	if (in_b->size != 0) {
		b = (char *)in_b->data;
	}

	/*
	 * our keys are always nul terminated.  if this changes,
	 * copy unterminated strings and terminate them.  strncmp()
	 * does not give us sufficient relative position information.
	 */

	st = strcmp(a, b);

	return (st);
}

static int
index_path(DB *dbp, const DBT *key, const DBT *data, DBT *skey) /* ARGSUSED */
{
	memset(skey, 0, sizeof (DBT));

	skey->data = data->data;
	skey->size = data->size;

	return (0);
}

static int
index_snapname(
	DB *dbp, const DBT *key, const DBT *data, DBT *skey) /* ARGSUSED */
{
	fsmsnap_t	*ptr;
	char		*str;

	ptr = (fsmsnap_t *)(data->data);

	/* don't index if name not set - snaps must have unique filenames */
	if ((ptr == NULL) || (ptr->snapname == NULL)) {
		return (DB_DONOTINDEX);
	}

	str = &(ptr->snapname[0]);
	memset(skey, 0, sizeof (DBT));

	skey->data = str;
	skey->size = strlen(str) + 1;

	return (0);
}

static int
index_vsnname(
	DB *dbp, const DBT *key, const DBT *data, DBT *skey) /* ARGSUSED */
{
	audvsn_t	*ptr;
	char		*str;

	ptr = (audvsn_t *)(data->data);

	str = ptr->vsn;
	memset(skey, 0, sizeof (DBT));

	skey->data = str;
	skey->size = sizeof (ptr->vsn);

	return (0);
}

static int
set_snapid(DB *dbp, DBT *data, db_recno_t recno)	/* ARGSUSED */
{
	fsmsnap_t	*ptr = (fsmsnap_t *)(data->data);

	ptr->snapid = recno;

	return (0);
}

static int
set_vsnid(DB *dbp, DBT *data, db_recno_t recno)		/* ARGSUSED */
{
	audvsn_t	*ptr = (audvsn_t *)(data->data);

	ptr->id = recno;

	return (0);
}

/*
 *  Closes databases for a given filesystem.  If delete_databases is TRUE,
 *  removes the databases from the system.
 */
void
close_fsdb(char *fsname, fs_db_t *fsdb, boolean_t delete_databases)
{
	size_t		i;
	DB		*dbp;
	char		fname[MAXPATHLEN + 1];
	DB_TXN		*txn;
	int		st;

	if (fsdb == NULL) {
		return;
	}

	i = sizeof (fs_db_t) / sizeof (DB *);

	/* close secondaries first, so start from the last in the struct */
	while (i > 0) {
		i--;
		dbp = ((DB **)fsdb)[i];

		if (dbp != NULL) {
			if (dbp->fname != NULL) {
				strlcpy(fname, dbp->fname, sizeof (fname));
			} else {
				fname[0] = '\0';
			}

			/* databases must be closed before they're removed. */
			dbp->close(dbp, 0);
			((DB **)fsdb)[i] = NULL;

			if ((delete_databases) && (fname[0] != '\0')) {
				dbEnv->txn_begin(dbEnv, NULL, &txn, 0);
				dbEnv->dbremove(dbEnv, txn, fname, NULL, 0);
				txn->commit(txn, 0);
			}
		}
	}

	if (delete_databases) {
		char	namebuf[MAXPATHLEN];

		/* delete fs-specific directory */
		st = dbdir_from_fsname(fsname, namebuf, sizeof (namebuf));
		if (st == 0) {
			snprintf(fname, sizeof (fname), "%s/%s", fsmdbdir,
			    namebuf);

			(void) rmdir(fname);
		}
	}
}

/*  Removes all database files associated with a given filesystem */
void
db_remove_filesys(fs_entry_t *fsent)
{
	if (fsent == NULL) {
		/* nothing to do */
		return;
	}

	/*
	 * drain off any other active threads, then remove the DBs
	 * Leave the list entry for now to preclude any dangling requests
	 * from trying to recreate the databases while we're trying to
	 * remove the files.
	 */
	destroy_fsent(fsent, TRUE);

	(void) pthread_mutex_lock(&fsent->statlock);
	fsent->active--;	/* take ourselves off the active list */
	(void) pthread_mutex_unlock(&fsent->statlock);

	/* blow away the entire entry */
	destroy_fs_list(fsent->fsname);
}

void
close_vsn_db(void)
{
	if (vsnnameDB != NULL) {
		vsnnameDB->close(vsnnameDB, 0);
		vsnnameDB = NULL;
	}

	if (vsnDB != NULL) {
		vsnDB->close(vsnDB, 0);
		vsnDB = NULL;
	}
}

void
close_env(void)
{

	if (dbEnv != NULL) {
		dbEnv->close(dbEnv, 0);
		dbEnv = NULL;
	}
}

/*
 * Passed in file system name is as listed in mnttab in the
 * mnt_special field.  Replace any slashes with "__".
 */
static int
dbdir_from_fsname(char *fsname, char *buf, size_t buflen)
{
	char	tmpbuf[MAXPATHLEN+1];
	char	*ptr;
	char	*fsp;
	size_t	i;


	if ((fsname == NULL) || (buf == NULL)) {
		return (-1);
	}

	strlcpy(tmpbuf, fsname, sizeof (tmpbuf));
	fsp = tmpbuf;

	while ((ptr = strchr(fsp, '/')) != NULL) {
		strlcat(buf, "__", buflen);
		i = PTRDIFF(ptr, fsp);
		*ptr = '\0';
		if (i > 0) {
			strlcat(buf, fsp, buflen);
		}
		fsp += (i + 1);
	}

	if (fsp != NULL) {
		strlcat(buf, fsp, buflen);
	}

	return (0);
}

#if 0
/*
 * function to trickle-write the cache.  Seemed to slow things down
 * a LOT so not used for now.  Left for reference.
 */
static void *
trickle_cache(void *arg)
{
	int	wrote;
	int	st = 0;

	(void) pthread_detach(pthread_self());

	while (st == 0) {
		st = dbEnv->memp_trickle(dbEnv, 50, &wrote);
	}

	fprintf(stdout, "trickle_cache exiting %d\n", st);
	fflush(stdout);

	return (NULL);
}
#endif

/*  Function called after catastrophic failure to remove bad db files */
static void
fsmdb_remove_all(char *topdir)
{
	DIR			*curdir;
	DIR			*subdir;
	struct dirent64		*entry;
	struct dirent64		*entryp;
	struct stat64		sbuf;
	char			buf[MAXPATHLEN + 1];
	char			*bufp;
	char			*fnam;
	char			*subp;

	if ((topdir == NULL) || (strcmp(topdir, "/") == 0) ||
	    (strcmp(topdir, "/usr") == 0) || (strcmp(topdir, "/opt") == 0) ||
	    (strcmp(topdir, "/var") == 0)) {
		return;
	}

	curdir = opendir(topdir);
	if (curdir == NULL) {
		return;
	}

	entry = malloc(sizeof (struct dirent64) + MAXPATHLEN + 1);
	if (entry == NULL) {
		closedir(curdir);
		return;
	}

	strlcpy(buf, topdir, sizeof (buf));
	strlcat(buf, "/", sizeof (buf));
	bufp = buf + strlen(buf);

	while ((readdir64_r(curdir, entry, &entryp)) == 0) {
		if (entryp == NULL) {
			break;
		}

		fnam = &(entry->d_name[0]);

		if ((strcmp(fnam, ".") == 0) || (strcmp(fnam, "..") == 0) ||
		    (strcmp(fnam, ".tmp") == 0)) {
			continue;
		}

		*bufp = '\0';
		strlcat(buf, fnam, sizeof (buf));

		if (lstat64(buf, &sbuf) != 0) {
			continue;
		}

		if (!S_ISDIR(sbuf.st_mode)) {
			if ((S_ISREG(sbuf.st_mode)) &&
			    (strncmp(fnam, "vsn", 3) == 0)) {
				/* one of our DB files */
				unlink(buf);
			}
			continue;
		}

		subdir = opendir(buf);
		if (subdir == NULL) {
			continue;
		}

		strlcat(buf, "/", sizeof (buf));
		subp = buf + strlen(buf);

		while ((readdir64_r(subdir, entry, &entryp)) == 0) {
			if (entryp == NULL) {
				break;
			}

			fnam = &(entry->d_name[0]);

			if (*fnam == '.') {
				continue;
			}

			*subp = '\0';
			strlcat(buf, fnam, sizeof (buf));

			unlink(buf);
		}

		closedir(subdir);
	}

	closedir(curdir);
	free(entry);
}
