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
#ifndef	_FSM_FSMDB_H_
#define	_FSM_FSMDB_H_

#pragma ident   "$Revision: 1.20 $"

#include <sys/types.h>
#include "db.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/restore.h"
#include "mgmt/private_file_util.h"

/*
 *  fsmdb.h
 *
 *  Contains datastructures, defines, and public function definitions for
 *  manipulating a series of databases containing historical file information
 *  about each file in a given file system at a particular point in time.
 *
 *  There are several primary databases for each file system being
 *  audited.
 *
 *		fidDB		Allows lookups of path names by file ID.
 *				Contains and manages the unique file IDs
 *				assigned to each file.
 *
 *		snapDB		Contains datastructures representing each
 *				"point-in-time" for which file data has
 *				been imported.
 *
 *		filesDB		Contains semi-static file data for each
 *				version of a file as discovered at each
 *				point-in-time view (snapshot).  The data
 *				is only stored when a file's modification
 *				date has changed.  This reduces the number
 *				of overall entries if the change rate is low.
 *
 *		snapfileDB	Contains the more dynamic data for each file
 *				at each snapshot.  Since this will be the
 *				most populous database, it has the smallest
 *				datastructure.
 *
 *		snapvsnDB	Contains a list of VSNs and associated
 *				snapshots.
 *
 *		metricsDB	Contains the distilled metric report data
 *				for each snapshot.
 *
 *  In addition, there is the "global" (i.e., spans all SAM file systems)
 *  database that holds VSN information.
 *
 *		vsnDB
 *
 *  Secondary Indices are used to speed up references to a given databases.
 *  They take up quite a lot of space so should be used only sparingly.
 *
 *		vsnnameDB	Allows lookup of VSNs by name.
 *				Derived from vsnDB
 *
 *		snapidDB	Allows lookups of snapshots by name.
 *				Derived from snapDB.
 *
 *		pathDB		Contains the file path, relative to the
 *				file system mount point.  Paths are stored
 *				only here, along with a unique file ID.
 *				The unique file ID is used by the other
 *				databases when the path name is required.
 *				Derived from fidDB.
 *
 *		dirDB		Contains the full path of directories
 *				(raelative to the mountpoint).  This info
 *				can be derived from pathDB by tracking back
 *				parent IDs, however performance testing showed
 *				this method vastly preferable.  Possible
 *				future optimizations with respect to this and
 *				pathDB should be investigated.
 *				Derived from fidDB.
 *
 */


/*
 *  Implementation notes:
 *
 *  Datastructure packing:  Structures are packed to be 4-byte aligned.
 *  This is primarily to keep the size small on-disk, but also to keep
 *  the code consistent when dealing with both Opteron and Sparc platforms.
 */

/*
 *  Datastructure for VSN DB
 *
 *  The primary database key is the audvsn_t->id.
 *
 *  Secondary index is vsnnameDB, which is indexed by VSN name.
 *  (audvsn_t->vsn)
 *
 *  	Note that VSNs can be reused, so for the purposes of this database,
 *  	we'll keep an unofficial generation number.  (Not yet implemented
 *	and may be removed.)  Refcnt may also not be kept.
 *
 */
#if _LONG_LONG_ALIGNMENT == 8
#pragma	pack(4)
#endif
typedef struct {
	uint32_t	id;			/* VSN ID */
	char		vsn[32];		/* VSN name */
	uint16_t	mtype;			/* media type */
	uint16_t	state;			/* media state */
	uint32_t	gen;			/* generation number */
	uint64_t	refcnt;			/* not sure about this */
} audvsn_t;
#if _LONG_LONG_ALIGNMENT == 8
#pragma	pack()
#endif

#define	AUDVSN_SZ 52

/*
 *  Datastructure for Path DB
 *
 *  Database containing mapping between pathname and file ID.
 *  Done this way to keep sizes of stored structures small.
 *  Structure will be malloc()ed to sizeof (path_t) + strlen(pathname)
 *
 *  The path DB is sorted at insertion time by pathname.
 */

#if _LONG_LONG_ALIGNMENT == 8
#pragma	pack(4)
#endif
typedef struct {
	uint64_t	parent;
	char		pathname[1];
} path_t;
#if _LONG_LONG_ALIGNMENT == 8
#pragma	pack()
#endif

/* sham path_t datastructure to avoid mallocs... */
typedef struct {
	uint64_t	parent;
	char		pathname[2048];
} fake_path_t;

/*
 *  Datastructure for Snapshot DB
 *
 *  Contains information about the point-in-time snapshot itself.
 *  If "snapname" is non-NULL, that indicates the data was imported
 *  from a SAM/QFS metadata snapshot (samfsdump) file.
 *
 *  snapname extends beyond the end of the datastructure.
 *  Structure will be malloc()ed to sizeof (fsmsnap_t) + strlen(snapname)
 *
 *  Time to take snap is not always available.
 */

#if _LONG_LONG_ALIGNMENT == 8
#pragma	pack(4)
#endif
typedef struct {
	uint32_t	snapdate;		/* time snap was taken  */
	uint32_t	end;			/* time to take snap */
	uint64_t	numfiles;		/* number of entries in snap */
	uint32_t	snapid;			/* recno into database */
	uint32_t	parentid;		/* snapid of parent fs if */
						/* partial snapshot.  */
	uint16_t	version;		/* Version of dbstructs used */
						/* when snap was created. */
						/* NOT YET IMPLEMENTED */
	uint8_t		flags;			/* snap-specific flags */
	char		snapname[1];		/* may be NULL */
} fsmsnap_t;
#if _LONG_LONG_ALIGNMENT == 8
#pragma	pack()
#endif

#define	FSMSNAP_SZ 28

/*  Flags for fsmsnap_t->flags */
#define	SNAPFLAG_DELETING ((uint8_t)1 << 1)	/* snapshot is being deleted */
#define	SNAPFLAG_SAM	  ((uint8_t)1 << 2)	/* file system is SAM */
#define	SNAPFLAG_NOATIME  ((uint8_t)1 << 3)	/* atime is unreliable */
#define	SNAPFLAG_QUOTAS	  ((uint8_t)1 << 4)	/* quotas enabled */
#define	SNAPFLAG_PARTIAL  ((uint8_t)1 << 5)	/* snapshot represents only */
						/* part of a file system */
#define	METRICS_AVAIL	  ((uint8_t)1 << 6)	/* metrics generated */
#define	SNAPFLAG_READY	  ((uint8_t)1 << 7)	/* fully imported */

/*  Unique file id based on snapshot id */

#if _LONG_LONG_ALIGNMENT == 8
#pragma	pack(4)
#endif
typedef struct {
	uint32_t	snapid;			/* snapshot ID */
	uint64_t	fid;			/* file ID - tied to path */
} sfid_t;
#if _LONG_LONG_ALIGNMENT == 8
#pragma	pack()
#endif

#define	FSMDBID_SZ 12

/*
 *  Datastructure for snapfiles DB
 *
 *  Database containing cross-references to all files referenced by
 *  a given snapshot.
 *
 *  To retrieve the semi-static data for a file, create a key to the
 *  fileDB using fuid->id and mtime (fuid_t).
 *
 *  offset is only set when data is being gathered from a samfsdump file.
 *
 *  Parent directory can be fetched from the pathDB when it's required.
 */

#if _LONG_LONG_ALIGNMENT == 8
#pragma	pack(4)
#endif
typedef struct {
	sfid_t		fuid;			/* key */
	uint32_t	mtime;			/* modified time */
	uint32_t	atime;			/* access time */
	off64_t		offset;			/* offset into .dmp file */
} filvar_t;
#if _LONG_LONG_ALIGNMENT == 8
#pragma	pack()
#endif

#define	FILVAR_DATA_OFF 12
#define	FILVAR_DATA_SZ  16
#define	FILVAR_SZ FILVAR_DATA_OFF + FILVAR_DATA_SZ

/*  Unique File ID based on mtime */

#if _LONG_LONG_ALIGNMENT == 8
#pragma	pack(4)
#endif
typedef struct {
	uint64_t	fid;			/* file ID - tied to path */
	uint32_t	mtime;			/* modified time */
} fuid_t;

/*  archive copy information - only set for SAM file systems */

typedef struct {
	uint32_t	archtime;
	uint32_t	vsn;
} archcpy_t;
#if _LONG_LONG_ALIGNMENT == 8
#pragma	pack()
#endif

/*
 *  Datastructure for files DB
 *
 *  Contains detail information for all file versions referenced by
 *  all snapshots.
 *
 *  If parent ID is required, look up by name.  Save space in db by
 *  not storing.
 *  TODO:
 *	   There will likely be other 'extra' information to be stored
 *	   for specific files.  The extra will be unusual -
 *		more VSNs
 *		extended attributes
 *	    consider using typed extras where the first part saved is
 *	    an array of types & sizes of extras.  Read out when needed.
 */

#if _LONG_LONG_ALIGNMENT == 8
#pragma	pack(4)
#endif
typedef struct {
	fuid_t		fuid;		/* unique file id */
	uint32_t	parent_mtime;	/* parent directory mod time */
	uint32_t	perms;
	uint32_t	owner;
	uint32_t	group;
	uint32_t	ctime;
	uint32_t	flags;
	uint64_t	size;
	uint64_t	osize;		/* online size */
	archcpy_t	arch[4];
	/*
	 *  If the number of VSNs for any given archive copy
	 *  is > 1, extra vsns are stored at the end of this
	 *  structure.  As we mostly don't care about the VSNs,
	 *  except when used in conjunction with the recycler
	 *  and/or the Restore UI, they're not normally returned.
	 *  All VSNs are indexed when a filinfo_t is inserted into
	 *  the database.  To retrieve them, either check 'flags'
	 *  for FILE_MOREVSNS, or use the db->get call to see if
	 *  the structure is longer than sizeof (filinfo_t).  Then
	 *  use db->get to retrieve the extra data.
	 *
	 *  Extra VSN storage format is:
	 *	uint16_t	nvsns[4] inode->ar[copy].nvsns - 1
	 *				(first vsn always listed in arch above)
	 *				0 indicates no extra vsns for copy
	 * 	uint32_t	<vsnid>
	 *	...
	 */
} filinfo_t;
#if _LONG_LONG_ALIGNMENT == 8
#pragma	pack()
#endif

#define	FILINFO_DATA_OFF 12
#define	FILINFO_DATA_SZ  72

/*  Flags for filinfo_t->flags */
#define	FILE_MOREVSNS	(1 << 1)
#define	FILE_HASDATA	(1 << 2)
#define	FILE_ONLINE	(1 << 3)
#define	FILE_PARTIAL	(1 << 4)
#define	FILE_ISWORM	(1 << 5)
#define	FILE_HASXATTRS	(1 << 6)

/* for use when looping through archive copy arrays.  Max copies is 4 */
#define	ARCHDAMBIT	16
#define	ARCHINCBIT	20
#define	ARCH_DAMAGED	(1 << 16)
#define	ARCH_INCONSISTENT (1 << 17)
#define	ARCH1_DAMAGED	(1 << 16)
#define	ARCH2_DAMAGED	(1 << 17)
#define	ARCH3_DAMAGED	(1 << 18)
#define	ARCH4_DAMAGED	(1 << 19)
#define	ARCH1_INCONSISTENT (1 << 20)
#define	ARCH2_INCONSISTENT (1 << 21)
#define	ARCH3_INCONSISTENT (1 << 22)
#define	ARCH4_INCONSISTENT (1 << 23)


/*
 * key for metric reports in the database.  Primary sort on report type, as
 * the caller requires data aggregated this way.
 */
typedef struct {
	uint32_t	rptType;
	uint32_t	snapid;
} fmrpt_key_t;

/*  globals */

/* flags when creating the db environment */
#define	FSM_SHM_MASTER_KEY	60415

#define	env_fl	DB_TIME_NOTGRANTED|DB_LOG_AUTOREMOVE|DB_TXN_NOSYNC|\
		DB_AUTO_COMMIT

/* environment flags that must be set when opening */
#define	env_ofl	DB_INIT_LOCK|DB_INIT_LOG|DB_INIT_MPOOL|\
		DB_INIT_TXN|DB_RECOVER|DB_CREATE|DB_THREAD|\
		DB_PRIVATE

/*
 * environment flags for recovering from more serious errors - use
 * DB_RECOVER_FATAL
 */
#define	env_ffl	DB_INIT_LOCK|DB_INIT_LOG|DB_INIT_MPOOL|\
		DB_INIT_TXN|DB_RECOVER_FATAL|DB_CREATE|DB_THREAD|\
		DB_PRIVATE

/* flags when opening the db itself */
#define	db_fl		DB_CREATE|DB_THREAD|DB_AUTO_COMMIT

/* environment pointer - used to group all the databases together */
extern DB_ENV*	dbEnv;

/* pointer to the 'global' VSN database */
extern DB*	vsnDB;
extern DB*	vsnnameDB;		/* vsnDB->vsn */

/*
 * structure to hold pointers to all the databases for a file system
 */
typedef struct {
	/* primary databases */
	DB*	fidDB;
	DB*	dirDB;			/* full path to directory */
	DB*	snapDB;
	DB*	snapfileDB;
	DB*	filesDB;
	DB*	snapvsnDB;
	DB*	metricsDB;

	/* secondary databases */
	DB*	snapidDB;		/* snapDB->snapname */
	DB*	pathDB;			/* fidDB->(basename of path) */
} fs_db_t;

/* filesystem list entries */

/* status enum */
typedef enum {
	FSENT_OK,
	FSENT_DELETING
} fsent_status_t;

/*
 *  lock serializes write access to the databases, and protects highid.
 *  statlock guards status, active and addtasks.
 *  active indicates the number of threads acting on this particular
 *  set of databases.
 *  addtasks indicates the number of threads waiting to import new data.
 */
typedef struct fs_entry_s {
	char			*fsname;
	fs_db_t			*fsdb;
	pthread_mutex_t		statlock;
	pthread_cond_t		statcond;
	fsent_status_t		status;
	int			active;
	int			addtasks;
	pthread_mutex_t		lock;
	uint64_t		highid;
	struct fs_entry_s	*next;
} fs_entry_t;

/* filesystem list */
extern fs_entry_t *fs_entry_list;

/*
 *  Define to interpret return status from DB->put calls
 *  Rejection of duplicate keys on primary database entry is
 *  expected and not an error.
 */
#define	CKPUT(x)			\
	st = x;				\
	if (st == DB_KEYEXIST) {	\
		st = 0;			\
	}

/* structure to use for snapshot deletes */
typedef struct {
	fs_entry_t	*fsent;
	char 		snapname[MAXPATHLEN + 1];
} delete_snap_t;


/* Public Function Prototypes */

/*
 *  fsmdb_log_err()
 *
 *  Logging function also used by Sleepycat when it can provide
 *  additional information to clarify simple errno values (like EINVAL).
 */
void
fsmdb_log_err(const DB_ENV *dbenv, const char *errpfx, const char *msg);

/*
 *  open_db_env()
 *
 *  Opens the database "environment" under which all database operations
 *  are run.
 */
int open_db_env(char *topdir);

/*
 *  open_vsn_db()
 *
 *  Opens the VSN databases
 */
int open_vsn_db(void);

/*
 *  close_vsn_db()
 *
 *  Closes the global VSN database
 */
void close_vsn_db(void);

/*
 *  close_env()
 *
 *  Closes the global database environment
 */
void close_env(void);

/*
 *  open_fs_db()
 *
 *  Opens all the databases associated with a given file system.
 */
int open_fs_db(
	char		*dirnam,
	char		*fsname,
	DB_ENV		*p_env,
	boolean_t	create,		/* only create if adding info */
	fs_db_t		*fsdb);

/*
 *  close_fsdb()
 *
 *  Closes all the databases in the provided fs_db_t structure
 *  If delete_databases is true, removes the db files after closing.
 */
void close_fsdb(char *fsname, fs_db_t *fsdb, boolean_t delete_databases);

/*  Removes all database files associated with a given filesystem */
void db_remove_filesys(fs_entry_t *fsent);

/*
 *  Removes filesystem entries from the available list.  If
 *  fsname is NULL, destroys the whole list.  Otherwise, just removes
 *  the specified entry.
 */
void destroy_fs_list(char *fsname);

/*
 *  db_process_samfsdump()
 *
 *  Imports file data from a named samfsdump file
 */
int db_process_samfsdump(fs_entry_t *fsent, char *snapname);

/*
 *  import_from_snapdir()
 *
 *  Scans through a specified directory associated with a named
 *  SAM file system and reads each samfsdump file not already processed.
 *
 *  Optionally returns the name of any metadata snapshot file which failed
 *  to import cleanly.
 *
 */
int import_from_snapdir(
	fs_entry_t *fsent, char *fsname, char *dirname, sqm_lst_t **errlst);

/*
 *  db_add_vsn()
 *
 *  For each SAM inode read, ensures that referenced VSNs are added to
 *  the global VSN database.  The unique VSNid is all that is stored in
 *  the file datastructure.
 */
int db_add_vsn(audvsn_t *vsn, uint32_t *vsnid);

/*
 *  read_snapfile_entry()
 *
 *  Given a path to a samfsdump file, reads through it and adds the file
 *  information.  This file borrows heavily from the (hopefully soon to
 *  be obsolete) samindexdump function process_dump().  Should both the audit
 *  subsystem AND samindexdump persist, this function should be generalized
 *  to pick out the information relevant to both samindexdump and audits,
 *  with the command-specific code moved to another function.
 */
int
read_snapfile_entry(
	dumpspec_t	*dsp,		/* in */
	char		**filename,	/* out */
	filvar_t	*filvar,	/* out */
	filinfo_t	*filinfo,	/* out */
	int		*media		/* out */
);

/*
 *  Add a new filesystem to the list being monitored
 */
int
add_fsent(char *dirnam, char *fsname, boolean_t create);

/*
 *  Given a directory name in the database environment, derive the
 *  filesystem name.
 */
int fsname_from_dbdir(char *path, char *buf, size_t buflen);

/*
 *  Remove a filesystem from the list.
 *  If delete_databases is TRUE, removes the databases from the
 *  system.  Otherwise, the databases are just closed.
 */
void
destroy_fsent(fs_entry_t *fsent, boolean_t delete_databases);

/*
 *  Retrieve a filesystem from the list
 */
int
get_fs_entry(char *fsname, boolean_t create, fs_entry_t **fsent);

/* Helper function to ensure activity counter decremented */
void
done_with_db_task(fs_entry_t *fsent);

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
int db_check_vsn_inuse(char *vsn);

/*
 *  db_list_all_vsns)
 *
 *  Returns a list of all VSN names registered in the database
 *
 *  The returned list is an array of char[32] buffers.
 *
 */
int
db_list_all_vsns(int *count, char **vsns);

/*
 *  db_get_snapshots()
 *
 *  Returns a list of snapshots in the database for a given filesystem
 *
 */
int
db_get_snapshots(fs_entry_t *fsent, uint32_t *nsnaps, char **snapnames);

/*
 *  db_get_snapshot_vsns()
 *
 *  Returns a list of VSNs referenced by a specific snapshot
 *
 */
int
db_get_snapshot_vsns(fs_entry_t *fsent, char *snapname, uint32_t *nvsns,
	char **vsns);

/*
 *  db_delete_snapshot()
 *
 *  Removes a snapshot and all of its entries from the database.
 *  If a path and/or VSN is no longer referenced, it is also deleted.
 *
 *  Runs in a separate thread to allow the caller to return.
 */
void*
db_delete_snapshot(void *arg);

/*
 * start_delete_snap_task()
 *
 * Deletes a snapshot in a separate thread
 */
int
start_delete_snap_task(fs_entry_t *fsent, fsmsnap_t *snapinfo);

/*
 *  db_delete_snapshot_vsns()
 *
 *  Deletes VSNs associated with a snapshot
 *
 */
int
db_delete_snapshot_vsns(fs_entry_t *fsent, DB_TXN *txn, uint32_t snapid);

/*
 *  db_get_snapshots()
 *
 *  Returns an array of datastructures describing each snapshot registered
 *  in the database for a given filesystem.
 */
int
db_get_snapshot_status(fs_entry_t *fsent, uint32_t *nsnaps, char **results,
	size_t *len);

/* retrieves the fsmsnap_t structure for a snapshot if available */
int
get_snap_by_name(
	fs_db_t		*fsdb,
	char		*snapname,		/* IN */
	fsmsnap_t	**snapdata);		/* OUT */


/* comparison function for BTREE databases. */
int bt_compare_uint64(DB *dbp, const DBT *in_a, const DBT *in_b);

/*  Simple list - returns just the pathnames in alpha order */
int
db_list_files(
	fs_entry_t	*fsent,		/* filesystem databases */
	char		*snappath,	/* Snapshot to browse.  If NULL, */
					/* list files referenced by ANY */
					/* snapshot.  Used for version */
					/* browser. */
	char		*startDir,	/* dir to start from.  NULL if root */
	char		*startFile,	/* start from here if continuing */
	int32_t		howmany,	/* how many to return. -1 for all */
	uint32_t	which_details,	/* file properties to return */
	restrict_t	restrictions,	/* filtering options */
	boolean_t	includeStart,	/* include startFile in return */
	uint32_t	*morefiles,	/* out - does dir have more files */
	sqm_lst_t	**results);	/* out - list of strings */

/* returns the file ID and parent ID associated with the specified path */
int
get_file_path_id(fs_db_t *fsdb, DB_TXN *txn, char *path, uint64_t *fid,
	uint64_t *parent);


/* adds a new snapshot database entry */
int db_add_snapshot(
	fs_entry_t	*fsent,
	fsmsnap_t	*snapdata,
	size_t		len,
	uint32_t	*snapid);

/*
 *  Updates the stored snapshot entry information
 */
int
db_update_snapshot(
	fs_entry_t	*fsent,
	fsmsnap_t	*snapdata,
	size_t		len);

/*
 * walk_live_fs()
 *
 * Uses nftw() to walk through the live filesystem to gather metrics
 * information.
 */
int
walk_live_fs(fs_entry_t *fsent, char *mountpt);

#endif /* _FSM_FSMDB_H_ */
