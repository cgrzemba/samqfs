/*
 * dbfile.c - SAM database file access functions.
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

#pragma ident "$Revision: 1.13 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/varargs.h>

/* Berkeley DB headers. */
#include <db.h>

/*	Local headers */
#include "sam/types.h"
#include "sam/sam_trace.h"
#include "sam/dbfile.h"

/* Private data. */
static char *tracePrefix = "DB";

/* Private functions. */
static int dbFileOpen(DBFile_t *dbfile, char *file, char *database,
	int flags);
static int dbFilePut(DBFile_t *dbfile, void *dbkey, unsigned int dbkey_size,
	void *dbdata, unsigned int dbdata_size, int flags);
static int dbFileGet(DBFile_t *dbfile, void *dbkey, unsigned int dbkey_size,
	void **dbdata);
static int dbFileDel(DBFile_t *dbfile, void *dbkey, unsigned int dbkey_size);
static int dbFileClose(DBFile_t *dbfile);

static int dbFileBeginIterator(DBFile_t *dbfile);
static int dbFileGetIterator(DBFile_t *dbfile, void **dbkey, void **dbdata);
static int dbFileEndIterator(DBFile_t *dbfile);

/* ErrMsg - return error message string */
static int dbFileNumof(DBFile_t *dbfile, int *numof);

static DB_ENV *setupEnv(char *homedir, char *datadir, char *progname);
static void dbError(const DB_ENV *dbenv, const char *errpfx, const char *msg);

/*
 * Initialize SAM database file access.
 */
int
DBFileInit(
	DBFile_t **dbfile,
	char *homedir,
	char *datadir,
	char *progname)
{
	int ret;
	DBFile_t *new;
	DB *db;
	DB_ENV *dbenv;

	Trace(TR_DBFILE, "%s init from %s homedir: '%s' datadir: '%s'",
	    tracePrefix, TrNullString(progname),
	    TrNullString(homedir), TrNullString(datadir));

	*dbfile = NULL;

	if (progname == NULL) {
		return (-1);
	}

	dbenv = setupEnv(homedir, datadir, progname);
	if (dbenv == NULL) {
		return (-1);
	}

	ret = db_create(&db, dbenv, 0);
	if (ret != 0) {
		return (-1);
	}

	new = (DBFile_t *)malloc(sizeof (DBFile_t));
	if (new == NULL) {
		return (-1);
	}
	memset(new, 0, sizeof (DBFile_t));

	new->db    = (void *)db;
	new->dbenv = (void *)dbenv;

	new->Open  = dbFileOpen;
	new->Put   = dbFilePut;
	new->Get   = dbFileGet;
	new->Del   = dbFileDel;
	new->Close = dbFileClose;

	new->BeginIterator = dbFileBeginIterator;
	new->GetIterator   = dbFileGetIterator;
	new->EndIterator   = dbFileEndIterator;

	new->Numof = dbFileNumof;

	*dbfile = new;
	Trace(TR_DBFILE, "%s [%x] init complete %d", tracePrefix, new, ret);

	return (ret);
}

/*
 * Destroy SAM database file handle.
 */
int
DBFileDestroy(
	DBFile_t *dbfile)
{
	DB_ENV *dbenv;

	Trace(TR_DBFILE, "%s [%x] destroy", tracePrefix, dbfile);

	if (DBFILE_IS_INIT(dbfile) == B_FALSE) {
		return (-1);
	}

	dbenv = (DB_ENV *)dbfile->dbenv;
	if (dbenv != NULL) {
		dbenv->close(dbenv, 0);
	}

	memset(dbfile, 0, sizeof (DBFile_t));
	free(dbfile);

	return (0);
}

/*
 * Trace SAM database file handle.
 */
void
DBFileTrace(
	DBFile_t *dbfile,
	char *srcFile,
	int srcLine)
{
	DB_ENV *dbenv;
	FILE *st;

	if (*TraceFlags & (1 << TR_dbfile)) {

		if (DBFILE_IS_INIT(dbfile) == B_FALSE) {
			return;
		}

		_Trace(TR_dbfile, srcFile, srcLine,
		    "%s [%x] trace", tracePrefix, dbfile);

		dbenv = (DB_ENV *)dbfile->dbenv;
		if (dbenv != NULL) {
			DB_LOCK_STAT *lock_stat;

			st = TraceOpen();
			dbenv->lock_stat(dbenv, &lock_stat, 0);
			dbenv->set_msgfile(dbenv, st);
			dbenv->lock_stat_print(dbenv, DB_STAT_ALL);
			dbenv->set_msgfile(dbenv, NULL);
			TraceClose(-1);
			free(lock_stat);
		}
	}
}


/*
 * Recover SAM database file.
 */
void
DBFileRecover(
	char *homedir,
	char *datadir,
	char *progname)
{
	int rval;
	DB_ENV *dbenv;
	int flags;
	upath_t pathname;
	uname_t filename;
	int digit;
	char *backing_prefix = "__db.00";

	Trace(TR_MISC, "%s recover dbenv from %s homedir: '%s' datadir: '%s'",
	    tracePrefix, TrNullString(progname), TrNullString(homedir),
	    TrNullString(datadir));

	rval = db_env_create(&dbenv, 0);
	if (rval == 0) {
		flags = DB_CREATE | DB_RECOVER_FATAL;
		rval = dbenv->open(dbenv, homedir, flags, 0);
		if (rval == 0) {
			dbenv->close(dbenv, 0);
		}
	}

	/*
	 * The dbenv->remove method destroys a Berkeley DB environment if it
	 * not currently in use.  The environment regions, including any
	 * backing files, are removed.  Any log or database files and the
	 * environment directory are not removed.
	 */
	rval = db_env_create(&dbenv, 0);
	if (rval == 0) {

		Trace(TR_MISC, "%s remove dbenv from %s homedir: '%s'",
		    tracePrefix, TrNullString(progname), TrNullString(homedir));

		(void) dbenv->remove(dbenv, homedir, 0);
	}

	/*
	 * In some cases, applications may choose to remove Berkeley DB files
	 * as part of their cleanup procedures, using system utilities instead
	 * of Berkeley DB interfaces (for example, using the UNIX rm utility
	 * instead of the DB_ENV->remove method). This is not a problem, as
	 * long as applications limit themselves to removing only files
	 * named "__db.###", where "###" are the digits 0 through 9.
	 * Applications * should never remove any files named with the prefix
	 * "__db" or "log", other than "__db.###" files.
	 */

	for (digit = 0; digit <= 9; digit++) {
		snprintf(filename, sizeof (filename), "%s%d",
		    backing_prefix, digit);
		snprintf(pathname, sizeof (pathname), "%s/%s",
		    datadir, filename);
		Trace(TR_MISC, "%s unlink dbenv from %s file: '%s'",
		    tracePrefix,
		    TrNullString(progname), TrNullString(pathname));
		(void) unlink(pathname);
	}
}

/*
 * Open SAM database file.
 */
static int
dbFileOpen(
	DBFile_t *dbfile,
	char *file,
	char *database,
	int flags)
{
	int ret;
	DB *db;
	int db_flags;

	Trace(TR_DBFILE, "%s [%x] open: '%s(%s)' flags: %d",
	    tracePrefix, dbfile, TrNullString(file),
	    TrNullString(database), flags);

	if (DBFILE_IS_INIT(dbfile) == B_FALSE) {
		return (-1);
	}

	if (file == NULL || file[0] == '\0') {
		return (-1);
	}

	db_flags = 0;
	if (flags & DBFILE_CREATE) {
		db_flags |= DB_CREATE;
	}
	if (flags & DBFILE_RDONLY) {
		db_flags |= DB_RDONLY;
	}

	db = dbfile->db;

	ret = db->open(db, NULL, file, database, DB_BTREE, db_flags, 0);
	if (ret != 0 && ret != ENOENT) {
		db->err(db, ret, "%s: open", file);
	}

	Trace(TR_DBFILE, "%s [%x] open complete %d", tracePrefix, dbfile, ret);
	return (ret);
}

/*
 * Store element in SAM database.
 */
static int
dbFilePut(
	DBFile_t *dbfile,
	void *dbkey,
	unsigned int dbkey_size,
	void *dbdata,
	unsigned int dbdata_size,
	int flags)
{
	DB *db;
	DBT key;
	DBT data;
	int ret;

	Trace(TR_DBFILE, "%s [%x] put key: %x/%d data: %x/%d flags: %d",
	    tracePrefix, dbfile, dbkey, dbkey_size,
	    dbdata, dbdata_size, flags);

	if (DBFILE_IS_INIT(dbfile) == B_FALSE) {
		return (-1);
	}

	db = dbfile->db;

	memset(&key,  0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	key.size = dbkey_size;
	key.data = dbkey;

	data.size = dbdata_size;
	data.data = dbdata;

	ret = db->put(db, NULL, &key, &data, flags);
	if (ret != 0) {
		if (ret != DB_KEYEXIST) {
			db->close(db, 0);
			db = dbfile->db = NULL;
		}
	}
	if (db != NULL) {
		db->sync(db, 0);
	}

	Trace(TR_DBFILE, "%s [%x] put complete %d",
	    tracePrefix, dbfile, ret);

	return (ret);
}

/*
 * Get element from SAM database.
 */
static int
dbFileGet(
	DBFile_t *dbfile,
	void *dbkey,
	unsigned int dbkey_size,
	void **dbdata)
{
	int ret;
	DB *db;
	DBT key;
	DBT data;

	Trace(TR_DBFILE, "%s [%x] get key: %x/%d",
	    tracePrefix, dbfile, dbkey, dbkey_size);

	if (DBFILE_IS_INIT(dbfile) == B_FALSE) {
		return (0);
	}

	db = dbfile->db;

	memset(&key,  0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	key.size = dbkey_size;
	key.data = dbkey;

	ret = db->get(db, NULL, &key, &data, 0);
	if (ret != 0) {
		/* FIXME Log error */
		*dbdata = NULL;
		return (ret);
	}
	*dbdata = data.data;

	Trace(TR_DBFILE, "%s [%x] get complete %d data: %x",
	    tracePrefix, dbfile, ret, data.data);

	return (ret);
}

/*
 * Delete element from SAM database.
 */
static int
dbFileDel(
	DBFile_t *dbfile,
	void *dbkey,
	unsigned int dbkey_size)
{
	int ret;
	DB *db;
	DBT key;

	Trace(TR_DBFILE, "%s [%x] del key: %x/%d",
	    tracePrefix, dbfile, dbkey, dbkey_size);

	if (DBFILE_IS_INIT(dbfile) == B_FALSE) {
		return (-1);
	}

	db = dbfile->db;

	memset(&key,  0, sizeof (DBT));

	key.size = dbkey_size;
	key.data = dbkey;

	ret = db->del(db, NULL, &key, 0);

	Trace(TR_DBFILE, "%s [%x] del complete %d",
	    tracePrefix, dbfile, ret);
	return (ret);
}

/*
 * Close SAM database.
 */
static int
dbFileClose(
	DBFile_t *dbfile)
{
	int ret;
	DB *db;

	Trace(TR_DBFILE, "%s [%x] close", tracePrefix, dbfile);

	db = dbfile->db;
	if (db == NULL) {
		return (-1);
	}

	ret = db->close(db, 0);

	Trace(TR_DBFILE, "%s close complete %d", tracePrefix, ret);
	return (ret);
}

/*
 * Begin SAM database iterator.
 */
static int
dbFileBeginIterator(
	DBFile_t *dbfile)
{
	int ret;
	DB *db;

	Trace(TR_DBFILE, "%s [%x] beginiterator",
	    tracePrefix, dbfile);

	if (DBFILE_IS_INIT(dbfile) == B_FALSE) {
		return (-1);
	}

	db = dbfile->db;

	ret = db->cursor(db, NULL, (DBC **)&dbfile->dbc, 0);

	Trace(TR_DBFILE, "%s [%x] beginiterator complete %d",
	    tracePrefix, dbfile, ret);

	return (ret);
}

/*
 * Get next element from SAM database iterator.
 */
static int
dbFileGetIterator(
	DBFile_t *dbfile,
	void **dbkey,
	void **dbdata)
{
	int ret;
	DBC *dbc;
	DBT key;
	DBT data;

	Trace(TR_DBFILE, "%s [%x] getiterator",
	    tracePrefix, dbfile);

	if (DBFILE_IS_INIT(dbfile) == B_FALSE ||
	    DBITERATOR_IS_INIT(dbfile) == B_FALSE) {
		return (-1);
	}

	dbc = (DBC *)dbfile->dbc;

	memset(&key,  0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	ret = dbc->get(dbc, &key, &data, DB_NEXT);
	if (ret != 0) {
		/* Log error */
		return (ret);
	}

	*dbkey  = key.data;
	*dbdata = data.data;

	Trace(TR_DBFILE, "%s [%x] getiterator complete %d key: %x data: %x",
	    tracePrefix, dbfile, ret, key.data, data.data);

	return (ret);
}

/*
 * End SAM database iterator.
 */
static int
dbFileEndIterator(
	DBFile_t *dbfile)
{
	int ret;
	DBC *dbc;

	Trace(TR_DBFILE, "%s [%x] enditerator",
	    tracePrefix, dbfile);

	if (DBFILE_IS_INIT(dbfile) == B_FALSE ||
	    DBITERATOR_IS_INIT(dbfile) == B_FALSE) {
		return (-1);
	}

	dbc = dbfile->dbc;

	ret = dbc->close(dbc);

	Trace(TR_DBFILE, "%s [%x] enditerator complete %d",
	    tracePrefix, dbfile, ret);

	return (ret);
}

/*
 * Number of elements in SAM database.
 */
static int
dbFileNumof(
	DBFile_t *dbfile,
	int *numof)
{
	int ret;
	DB *db;
	DB_BTREE_STAT *stat;

	Trace(TR_DBFILE, "%s [%x] numof",
	    tracePrefix, dbfile);

	if (DBFILE_IS_INIT(dbfile) == B_FALSE) {
		return (-1);
	}

	db = dbfile->db;

	ret = db->stat(db, NULL, &stat, DB_FAST_STAT);

	*numof = 0;
	if (ret == 0) {
		*numof = stat->bt_ndata;
	}

	Trace(TR_DBFILE, "%s [%x] numof complete %d numof: %d",
	    tracePrefix, dbfile, ret, *numof);

	return (ret);
}

/*
 * Setup SAM database environment.
 */
static DB_ENV *
setupEnv(
	char *homedir,
	/* LINTED argument unused in function */
	char *datadir,
	char *progname)
{
	int ret;
	DB_ENV *dbenv;
	int flags;

	ret = db_env_create(&dbenv, 0);
	if (ret != 0) {
		return (NULL);
	}

	/*
	 * Set error message file and prefix.
	 */
	dbenv->set_errcall(dbenv, dbError);
	dbenv->set_errpfx(dbenv, progname);

	/*
	 * Set environment data directory.
	 */
	/* FIXME (void) dbenv->set_data_dir(dbenv, datadir); */

	flags = DB_INIT_MPOOL | DB_INIT_CDB;
	ret = dbenv->open(dbenv, homedir, flags, 0);
	if (ret != 0) {
		dbenv->err(dbenv, ret, "environment open, will try to create: %s", homedir);
		flags |= DB_CREATE;
		ret = dbenv->open(dbenv, homedir, flags, 0);
		if (ret != 0) {
			dbenv->err(dbenv, ret, "environment open for create: %s", homedir);
			dbenv->close(dbenv, 0);
			return (NULL);
		}
	}
	return (dbenv);
}

/*
 * Called when a error occurs in the Berkeley DB library.
 */
static void
dbError(
	/* LINTED argument unused in function */
	const DB_ENV *dbenv,
	const char *errpfx,
	const char *msg)
{
	Trace(TR_ERR, "%s Error %s: %s",
	    tracePrefix, errpfx, msg);
}
