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
#pragma ident	"$Revision: 1.14 $"

#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <errno.h>
#include <fnmatch.h>

#include "db.h"
#include "mgmt/fsmdb.h"

#define	DEC_INIT
#include "pub/devstat.h"
#include "sam/devnm.h"
#undef DEC_INIT

#include "pub/mgmt/restore.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/file_util.h"
#include "mgmt/file_details.h"
#include "mgmt/private_file_util.h"

/*  Functions to return details about files referenced in the database */

static int add_file_info(
	fs_db_t		*fsdb,
	uint32_t	snapdate,
	char		*fname,
	sfid_t		*fid,
	restrict_t	*filter,
	int32_t		howmany,
	uint32_t	which_details,	/* file properties to return */
	sqm_lst_t		*lstp);

static int filedetails_from_db(
	uint32_t	snapdate,
	char		*fname,
	filinfo_t	*filinfo,
	filvar_t	*filvar,
	uint32_t	which_details,
	sqm_lst_t		*results);

static char *media_to_string(int mt);
void free_details(filedetails_t *details);

/*
 *  Extended listing - returns filedetails_t structures for the caller
 *  to interpret
 */
int
db_list_files(
	fs_entry_t	*fsent,		/* filesystem databases */
	char		*snappath,	/* Snapshot to browse.  */
	char		*startDir,	/* dir to start from.  NULL if root */
	char		*startFile,	/* start from here if continuing */
	int32_t		howmany,	/* how many to return. -1 for all */
	uint32_t	which_details,	/* file properties to return */
	restrict_t	restrictions,	/* filtering options */
	boolean_t	includeStart,	/* include startFile in return */
	uint32_t	*morefiles,	/* out - does dir have more files */
	sqm_lst_t	**results	/* out - list of filedetails_t */
)
{
	int		st;
	fs_db_t		*fsdb = fsent->fsdb;
	fsmsnap_t	*snapdata = NULL;
	DBT		key;
	DBT		data;
	DBT		pkey;
	uint64_t	dirid;
	uint64_t	fileid = 0;
	fake_path_t	fpath;
	fake_path_t	npath;
	DBC		*curs = NULL;
	sqm_lst_t		*lstp = NULL;
	sfid_t		fid;
	uint32_t	found = 0;
	int32_t		getnum;
	boolean_t	start_is_dir = TRUE;
	char		*namep;

	st = get_snap_by_name(fsdb, snappath, &snapdata);

	if (st != 0) {
		return (ENOENT);
	}

	if (!(snapdata->flags & SNAPFLAG_READY)) {
		/* what's a good error to indicate it exists, but is bad? */
		free(snapdata);
		return (EINVAL);
	}

	if (howmany == -1) {
		getnum = 0;
	} else {
		getnum = howmany;
	}

	/* Get the directory id, if it's not NULL or "/" or "." or "" */
	/* set up the path structure.  If startDir is empty or "/", use "." */
	fpath.pathname[0] = '\0';
	if ((startDir == NULL) || (*startDir == '\0') ||
	    (strcmp(startDir, "/") == 0)) {
		strlcat(fpath.pathname, ".", sizeof (fpath.pathname));
	} else {
		strlcat(fpath.pathname, startDir, sizeof (fpath.pathname));
	}

	if ((startFile != NULL) && (startFile[0] != '\0') &&
	    (strcmp(startFile, ".") != 0)) {
		strlcat(fpath.pathname, "/", sizeof (fpath.pathname));
		strlcat(fpath.pathname, startFile, sizeof (fpath.pathname));
		start_is_dir = FALSE;
	}

	/* set up the results list */
	lstp = lst_create();
	if (lstp == NULL) {
		st = ENOMEM;
		goto done;
	}
	*results = lstp;

	/* not changed through rest of function */
	fid.snapid = snapdata->snapid;

	/*
	 * Try to get the requested file.  If it's not there, fail.
	 * Unlike browsing with stat, we're dealing with static data so
	 * the requested file must still exist or we've got bigger
	 * problems.
	 */
	st = get_file_path_id(fsdb, NULL, fpath.pathname, &fileid, &dirid);
	if (st != 0) {
		st = ENOENT;
		goto done;
	}

	/*
	 * if we're only to get a single file, and we just found it,
	 * fill in the details then return
	 */
	if ((getnum == 1) && (includeStart)) {
		if (start_is_dir) {
			namep = fpath.pathname;
		} else {
			namep = basename(fpath.pathname);
		}
		fid.fid = fileid;
		st = add_file_info(fsdb, snapdata->snapdate, namep, &fid,
		    &restrictions, getnum, which_details, lstp);

		found++;

		goto done;
	}

	if (start_is_dir) {
		dirid = fileid;
		fileid = 0;
		fpath.pathname[0] = '\0';
	} else {
		/* for subsequent operations, we only want the file name */
		namep = strrchr(startFile, '/');
		if (namep == NULL) {
			namep = startFile;
		}
		strlcpy(fpath.pathname, namep, sizeof (fpath.pathname));
	}

	/* set up a cursor for the path database */
	st = (fsdb->pathDB)->cursor(fsdb->pathDB, NULL, &curs, 0);
	if (st != 0) {
		goto done;
	}

	fpath.parent = dirid;

	memset(&key, 0, sizeof (DBT));
	memset(&pkey, 0, sizeof (DBT));
	memset(&data, 0, sizeof (DBT));

	/* this key is only used for the initial cursor get call */
	key.data = &fpath;
	key.size = key.ulen = sizeof (fake_path_t);
	key.flags = DB_DBT_USERMEM;

	pkey.data = &fileid;
	pkey.size = pkey.ulen = sizeof (uint64_t);
	pkey.flags = DB_DBT_USERMEM;

	data.data = &npath;
	data.size = data.ulen = sizeof (fake_path_t);
	data.flags = DB_DBT_USERMEM;

	st = curs->c_pget(curs, &key, &pkey, &data, DB_SET_RANGE);
	/*
	 * We may have already included the first entry if includeStart is
	 * TRUE.  SET_RANGE will only give us a new entry to include if
	 * the starting point is the beginning of a directory, or the
	 * requested starting point was not found.
	 */
	if ((st == 0) && (includeStart) && (!start_is_dir) && (found == 1)) {
		st = curs->c_pget(curs, &key, &pkey, &data, DB_NEXT);
	}

	/* pathDB is already alphasorted, so we only need to count */
	while ((st == 0) && (npath.parent == dirid)) {
		/* this will always add the start entry.  fix later */
		/* skip the root entry - it has no interesting data */
		if (fileid != 0) {
			fid.fid = fileid;
			st = add_file_info(fsdb, snapdata->snapdate,
			    npath.pathname, &fid, &restrictions, getnum,
			    which_details, lstp);
			if (st == 0) {
				/* errors most likely mean file not in snap */
				found++;
			} else {
				st = 0;
			}
		}

		st = curs->c_pget(curs, &key, &pkey, &data, DB_NEXT);
	}

	curs->c_close(curs);
	curs = NULL;

	if (st == DB_NOTFOUND) {
		/* no more entries to read */
		st = 0;
	}

	*morefiles = found - lstp->length;
done:
	if (curs != NULL) {
		curs->c_close(curs);
	}
	if (snapdata) {
		free(snapdata);
	}
	if (st != 0) {
		if (lstp) {
			lst_free_deep(lstp);
		}
	}

	return (st);
}

static int
check_restrict_filinfo(restrict_t *filter, filinfo_t *filinfo)
{
	/* check for the common case, no filtering */
	if (filter->flags == 0) {
		return (0);
	}

	if (filter->flags & flg_dates) {
		if ((filter->flags & fl_modbefore) &&
		    (filter->modbefore < filinfo->fuid.mtime)) {
			return (1);
		}

		if ((filter->flags & fl_modafter) &&
		    (filter->modafter > filinfo->fuid.mtime)) {
			return (1);
		}

		if ((filter->flags & fl_creatbefore) &&
		    (filter->creatbefore < filinfo->ctime)) {
			return (1);
		}
		if ((filter->flags & fl_creatafter) &&
		    (filter->creatafter > filinfo->ctime)) {
			return (1);
		}
	}

	if (filter->flags & flg_ownsiz) {
		if ((filter->flags & fl_user) &&
		    (filter->user != filinfo->owner)) {
			return (1);
		}
		if ((filter->flags & fl_gid) &&
		    (filter->gid != filinfo->group)) {
			return (1);
		}
		if ((filter->flags & fl_biggerthan) &&
		    (filter->biggerthan > filinfo->size)) {
			return (1);
		}
		if ((filter->flags & fl_smallerthan) &&
		    (filter->smallerthan < filinfo->size)) {
			return (1);
		}
	}

	/*
	 * flg_state not currently supported.  Can we do this in the
	 * file browser ?
	 */

	return (0);
}

/* helper function for the list routine */
static int
add_file_info(
	fs_db_t		*fsdb,
	uint32_t	snapdate,
	char		*fname,
	sfid_t		*fid,
	restrict_t	*filter,
	int32_t		howmany,
	uint32_t	which_details,	/* file properties to return */
	sqm_lst_t		*lstp)		/* OUT */
{
	int		st = DB_LOCK_DEADLOCK;
	filinfo_t	filinfo;
	filvar_t	filvar;
	DBT		key;
	DBT		data;
	DB_TXN		*txn = NULL;
	int		retry = 0;

	/* check name filter first */
	if (filter->flags & fl_filename) {
		if (fnmatch(filter->filename, fname, 0)) {
			return (-1);
		}
	}

	/*
	 * Guard against deadlocks with input tasks and
	 * retry up to 5 times before failing.
	 */

	while ((st == DB_LOCK_DEADLOCK) && (retry < 5)) {

		retry++;

		st = dbEnv->txn_begin(dbEnv, NULL, &txn, DB_TXN_NOWAIT);
		if (st != 0) {
			continue;
		}

		/* get the variable file info */
		filvar.fuid.snapid = fid->snapid;
		filvar.fuid.fid = fid->fid;

		memset(&key, 0, sizeof (DBT));
		memset(&data, 0, sizeof (DBT));

		key.data = &(filvar.fuid);
		key.size = FILVAR_DATA_OFF;

		data.data = ((char *)&filvar) + FILVAR_DATA_OFF;
		data.size = data.ulen = FILVAR_DATA_SZ;
		data.flags = DB_DBT_USERMEM;

		st = (fsdb->snapfileDB)->get(fsdb->snapfileDB, txn, &key,
		    &data, 0);
		if (st != 0) {
			txn->abort(txn);
			continue;
		}

		/* get the rest of the file info */
		filinfo.fuid.fid = filvar.fuid.fid;
		filinfo.fuid.mtime = filvar.mtime;

		memset(&key, 0, sizeof (DBT));
		memset(&data, 0, sizeof (DBT));

		key.data = &(filinfo.fuid);
		key.size = FILINFO_DATA_OFF;

		data.data = ((char *)&filinfo) + FILINFO_DATA_OFF;
		data.size = data.ulen = FILINFO_DATA_SZ;
		data.flags = DB_DBT_USERMEM;

		st = (fsdb->filesDB)->get(fsdb->filesDB, txn, &key, &data, 0);
		if (st != 0) {
			txn->abort(txn);
			continue;
		}

		/* success! */
		txn->commit(txn, 0);
		break;
	}

	/* did we get the file info? No point in proceeding if not. */
	if (st != 0) {
		return (st);
	}

	/* Check the rest of the filters */
	st = check_restrict_filinfo(filter, &filinfo);
	if (st != 0) {
		return (-1);
	}

	/*
	 * If we've already added the requested number, merely count the
	 * rest.
	 */
	if (howmany && (lstp->length >= howmany)) {
		return (0);
	}

	st = filedetails_from_db(snapdate, fname, &filinfo, &filvar,
	    which_details, lstp);

	return (st);
}

static int
filedetails_from_db(
	uint32_t	snapdate,
	char		*fname,
	filinfo_t	*filinfo,
	filvar_t	*filvar,
	uint32_t	which_details,
	sqm_lst_t		*results)
{

	int		st;
	int		i;
	filedetails_t	*details;
	audvsn_t	vsninfo;
	DBT		key;
	DBT		data;
	char		*media;


	details = calloc(1, sizeof (filedetails_t));
	if (details == NULL) {
		return (ENOMEM);
	}

	details->file_name = strdup(fname);
	if (details->file_name == NULL) {
		free(details);
		return (ENOMEM);
	}

	details->user = filinfo->owner;
	details->group = filinfo->group;
	details->prot = filinfo->perms;
	if (S_ISDIR(details->prot)) {
		details->file_type = FTYPE_DIR;
	} else if (S_ISLNK(details->prot)) {
		details->file_type = FTYPE_LNK;
	} else if (S_ISREG(details->prot)) {
		details->file_type = FTYPE_REG;
	} else if (S_ISFIFO(details->prot)) {
		details->file_type = FTYPE_FIFO;
	} else if (S_ISBLK(details->prot) || S_ISCHR(details->prot)) {
		details->file_type = FTYPE_SPEC;
	} else {
		details->file_type = FTYPE_UNK;
	}

	/* Segments not yet fully supported.  Only overview available */
	details->summary.created = filinfo->ctime;
	details->summary.modified = filvar->mtime;
	details->summary.accessed = filvar->atime;
	details->summary.size = filinfo->size;
	details->snapOffset = filvar->offset;

	/* segflags */
	if (!(filinfo->flags & FILE_ONLINE)) {
		details->summary.flags |= FL_OFFLINE;
	}
	if (filinfo->flags & FILE_PARTIAL) {
		details->summary.flags |= FL_PARTIAL;
	}
	/* not currently supported set, but will follow this logic later */
	if (filinfo->flags & FILE_ISWORM) {
		details->summary.flags |= FL_WORM;
	}
	if (filinfo->flags & FILE_HASXATTRS) {
		details->summary.flags |= FL_EXATTRS;
	}
	if (filinfo->flags & FILE_HASDATA) {
		details->summary.flags |= FL_HASDATA;
	}
	/* need archdone & release attrs & segmented flag */


	/* copy details */
	for (i = 0; i < 4; i++) {
		/*
		 * if archtime is 0, copy does not exist.  Also screen
		 * out archive copies made after this snapshot was taken
		 * as they will not be available for restore.
		 */
		if ((filinfo->arch[i].archtime == 0) ||
		    (filinfo->arch[i].archtime > snapdate)) {
			continue;
		}
		/* get the vsn info */
		if (filinfo->arch[i].vsn == 0) {
			continue;
		}

		memset(&key, 0, sizeof (DBT));
		memset(&data, 0, sizeof (DBT));

		key.data = &(filinfo->arch[i].vsn);
		key.size = sizeof (uint32_t);

		data.data = &vsninfo;
		data.size = data.ulen = sizeof (audvsn_t);
		data.flags = DB_DBT_USERMEM;

		st = vsnDB->get(vsnDB, NULL, &key, &data, 0);
		if (st != 0) {
			st = 0;		/* skip this one */
			continue;	/* shouldn't happen */
		}

		/* convert mtype */
		media = media_to_string(vsninfo.mtype);
		strlcpy(details->summary.copy[i].mediaType, media,
		    sizeof (details->summary.copy[i].mediaType));

		details->summary.copy[i].created = filinfo->arch[i].archtime;

		if (filinfo->flags & ARCH_DAMAGED) {
			details->summary.copy[i].flags |= FL_DAMAGED;
		}
		if (filinfo->flags & ARCH_INCONSISTENT) {
			details->summary.copy[i].flags |= FL_INCONSISTENT;
		}

		if (which_details & FD_COPY_DETAIL) {
			details->summary.copy[i].vsns = strdup(vsninfo.vsn);
		}
	}

	st = lst_append(results, details);
	if (st != 0) {
		free_details(details);
	}

	return (st);
}

/*
 * YUCK.  Another side-effect of not being able to link with samut.  This
 * function is a duplicate of sam_mediatoa().
 *
 *
 *	Return media name from media type, empty string if not valid.
 *	Returned string is a constant and MUST NOT be freed,
 */
static char *
media_to_string(int mt)		/* Media type */
{
	char		*media_name;
	dev_nm_t	*tmp;

	for (tmp = &dev_nm2mt[0]; tmp->nm != NULL; tmp++) {
		if (mt == tmp->dt) {
			return (tmp->nm);
		}
	}
	/*
	 * Did not find matching media type.  Check for
	 * third party media types.  If not third party,
	 * an empty string is returned.
	 */
	media_name = "";
	if ((mt & DT_CLASS_MASK) == DT_THIRD_PARTY) {
		int n;

		n = mt & DT_MEDIA_MASK;
		if (n >= '0' && n <= '9') {
			media_name = dev_nmtr[n - '0'];
		} else if (n >= 'a' && n <= 'z') {
			media_name = dev_nmtr[(n - 'a') + 10];
		}
	}

	return (media_name);
}

/*
 *  And one more copied function since we can't link with libfsmgmt or
 *  even build in sam_file_details.c due to the samut issue.
 */
void
free_details(filedetails_t *details) {
	int		i;
	int		j;
	fildet_seg_t	*segp;

	if (details == NULL) {
		return;
	}

	free(details->file_name);

	/* free segments */
	if ((details->segCount > 0) && (details->segments)) {
		for (i = 0; i < details->segCount; i++) {
			segp = &(details->segments[i]);

			if (segp == NULL) {
				break;
			}

			for (j = 0; j < 4; j++) {
				if (segp->copy[j].vsns != NULL) {
					free(segp->copy[j].vsns);
				}
			}
		}
		free(details->segments);
	}

	/* free summary */
	segp = &(details->summary);
	for (j = 0; j < 4; j++) {
		if (segp->copy[j].vsns != NULL) {
			free(segp->copy[j].vsns);
		}
	}
}
