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
 * or https://illumos.org/license/CDDL.
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

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * file_util.c
 * contains some functions commonly used when looking at files
 */

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>
#include <fnmatch.h>
#include <stddef.h>

#include "sam/sam_trace.h"

#include "pub/mgmt/error.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/sqm_list.h"
#include "mgmt/private_file_util.h"
#include "mgmt/util.h"
#include "pub/mgmt/file_util.h"

/*
 *  Verify that a file exists or can be created.
 */
boolean_t
verify_file(
	const char *file,	/* file or directory name */
	boolean_t dir_is_suff)	/* if true only directory must exist */
{

	char *dupfile;
	char *path;
	struct stat64 buf;
	boolean_t rc = B_FALSE;

	/*
	 *	Must be a fully qualified path name.
	 */
	if (*file == '/') {
		/*
		 *	A file that already exists is good.
		 */
		if (stat64(file, &buf) == 0) {
			rc = B_TRUE;

		} else if (dir_is_suff) {
			/*
			 *	Check if valid directory.
			 */
			dupfile = strdup(file);
			path = dirname(dupfile);
			if (stat64(path, &buf) == 0) {
				rc = B_TRUE;
			}
			free(dupfile);
		}
	}

	return (rc);
}


int
file_exists(
	ctx_t	*ctx	/* ARGSUSED */,
	char	*file)
{

	struct stat64 buf;

	if (ISNULL(file)) {
		return (-1);
	}

	Trace(TR_MISC, "Check if %s exists", file);
	if (*file == '/') {
		if (stat64(file, &buf) == 0) {
			Trace(TR_MISC, "FOUND file %s", file);
			return (0);
		}
	}
	Trace(TR_MISC, "File %s not found", file);
	return (1);
}

/*
 * DESCRIPTION:
 *	Create a file if it does not already exist. This function will also
 *	create any missing directories. If the file already exists
 *	this function will return success.
 * PARAMS:
 *   ctx_t *	IN   - context object
 *   upath_t	IN   - fully qualified file name
 *   mode_t	IN   - permissions to set for file
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int
create_file(
	ctx_t 	*ctx,		/* ARGSUSED */
	char	*full_path)
{

	struct stat64 	buf;
	char		dupfile[MAXPATHLEN+1];
	char		*path;
	mode_t		mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	Trace(TR_MISC, "creating file %s", full_path);

	if (ISNULL(full_path)) {
		Trace(TR_ERR, "creating file failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	if (stat64(full_path, &buf) != 0) {

		if (ENOENT == errno) {
			/* file doesn't exist. */
			strlcpy(dupfile, full_path, sizeof (dupfile));
			path = dirname(dupfile);
			if (create_dir(NULL, path) != 0) {
				Trace(TR_ERR, "creating file %s failed: %s",
				    full_path, samerrmsg);
				return (-1);
			}

			/* create the file */
			if (mknod(full_path, mode, NULL) != 0) {
				samerrno = SE_CREATE_FILE_FAILED;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(SE_CREATE_FILE_FAILED),
				    full_path, "");
				strlcat(samerrmsg, strerror(errno),
				    MAX_MSG_LEN);
				Trace(TR_ERR, "creating file %s failed: %s",
				    full_path, samerrmsg);
				return (-1);
			}
		} else {
			samerrno = SE_CREATE_FILE_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CREATE_FILE_FAILED),
			    full_path, "");
			strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);

			Trace(TR_ERR, "creating file %s failed: %s",
			    full_path, samerrmsg);
			return (-1);
		}
	} else {
		/* check that it is not a directory */
		if (S_ISDIR(buf.st_mode)) {
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CREATE_FILE_FAILED),
			    full_path, "");
			strlcat(samerrmsg, strerror(EISDIR), MAX_MSG_LEN);
			Trace(TR_ERR, "creating file %s failed: %s",
			    full_path, samerrmsg);
			return (-1);
		}
	}

	return (0);
}

int
list_dir(
	ctx_t *c, int maxentries, char *filepath,
	char *restrictions, sqm_lst_t **direntries) /* ARGSUSED */
{
	int rval = 0;
	DIR *curdir;		/* Variable for directory system calls */
	struct dirent64 *entry;	/* Pointer to a directory entry */
	struct dirent64 *entryp;
	struct stat64 sout;
	restrict_t filter = {0};
	char *data;		/* Pointer to data item to add to list */
	char fullpath[MAXPATHLEN];

	/* Set up wildcard restrictions */
	rval = set_restrict(restrictions, &filter);
	if (rval) {
		return (rval);
	}

	curdir = opendir(filepath); /* Set up to ask for directory entries */
	if (curdir == NULL) {
		return (samrerr(SE_NOSUCHPATH, filepath));
	}

	*direntries = lst_create(); /* Return results in this list */
	if (*direntries == NULL) {
		closedir(curdir);
		return (-1);	/* If allocation failed, samerr is set */
	}

	entry = mallocer(sizeof (struct dirent64) + MAXPATHLEN + 1);
	if (entry == NULL) {
		closedir(curdir);
		lst_free(*direntries);
		*direntries = NULL;
		return (-1);
	}

	/* Walk through directory entries */
	while ((rval = readdir64_r(curdir, entry, &entryp)) == 0) {
		if (entryp == NULL) {
			break;
		}

		if ((strcmp(entry->d_name, ".") == 0) ||
		    (strcmp(entry->d_name, "..") == 0)) {
			continue;
		}
		/* Create full pathname and get stat info */
		snprintf(
		    fullpath, MAXPATHLEN, "%s/%s", filepath,
		    entry->d_name);

		if (stat64(fullpath, &sout) != 0) {
			continue; /* Ignore file which can't be stat'ed */
		}

		if (check_restrict_stat(entry->d_name, &sout, &filter))
			continue; /* Not this entry */

		data = copystr(entry->d_name); /* Copy data to allocated mem */
		if (data == NULL) {
			rval = -1;
			break;	/* samerr already set */
		}
		lst_append(*direntries, data);
		if ((*direntries)->length >= maxentries)
			break;	/* Keep list to designated limits */
	}
	free(entry);

	if (rval) {
		lst_free_deep(*direntries); /* On failure, don't return list */
		*direntries = NULL;
	} else {
		lst_qsort(*direntries, node_cmp);
	}
	closedir(curdir);
	return (rval);
}

int
get_file_status(
	ctx_t *c,		/* ARGSUSED */
	sqm_lst_t *filepaths,
	sqm_lst_t **filestatus)
{
	int rval = 0;
	int filestat;
	node_t *node;
	struct stat64 sout;	/* Buffer to receive file status */


	*filestatus = lst_create(); /* Return results in this list */
	if (*filestatus == NULL)
		return (-1);	/* samerr is already set */

	node = filepaths->head;
	while (node != NULL) {
		if (stat64(node->data, &sout)) { /* Ask about a file */
			filestat = SAMR_MISSING; /* File doesn't even exist */
		} else if ((sout.st_mode & S_IFMT) == S_IFDIR) {
			filestat = SAMR_DIRFILE; /* This is a directory */
		} else if ((sout.st_mode & S_IFMT) == S_IFREG) {
			if (sout.st_size && (sout.st_blocks == 0))
				filestat = SAMR_RELFILE; /* File not on disk */
			else
				filestat = SAMR_REGFILE; /* Is file, on disk */
		} else {
			filestat = SAMR_NOTFILE; /* Neither file nor dir. */
		}
		rval = lst_append(*filestatus, copyint(filestat));
		if (rval)
			break;
		node = node->next;
	}
	if (rval)
		lst_free_deep(*filestatus);
	return (rval);
}

/* Get details about a file. */
int
get_file_details(
	ctx_t *c,		/* ARGSUSED */
	char *fsname,
	sqm_lst_t *files,
	sqm_lst_t **status)
{
	int rval, mntfd;
	node_t *fil;
	char mountpt[MAXPATHLEN+1];
	char details[MAXPATHLEN+1];
	struct stat64 sout;
	char *filstat;

	if (strlen(fsname) != 0) {
		rval = getfsmountpt(fsname, mountpt, sizeof (mountpt));
		if (rval != 0) {
			return (rval);
		}
	} else {
		/* No FS specified, use root */
		strlcpy(mountpt, "/", sizeof (mountpt));
	}

	mntfd = open64(mountpt, O_RDONLY);
	if (mntfd < 0)
		return (samrerr(SE_NOT_A_DIR, mountpt));

	*status = lst_create();	/* Return results in this list */
	if (*status == NULL) {
		close(mntfd);
		return (-1);	/* If allocation failed, samerr is set */
	}

	fil = files->head;	/* Walk through list of files */
	while (fil != NULL) {

		memset(&sout, 0, sizeof (sout));
		rval = fstatat64(mntfd, fil->data, &sout, 0);
		if (rval) {
			filstat = copystr(""); /* File doesn't exist */
		} else {
			snprintf(
			    details, sizeof (details),
			    "size=%lld,created=%lu,modified=%lu",
			    sout.st_size, sout.st_ctim.tv_sec,
			    sout.st_mtim.tv_sec);
			filstat = copystr(details);
		}

		lst_append(*status, filstat);

		fil = fil->next; /* And check next file */
	}

	close(mntfd);
	return (0);
}


/*
 * create the directory dir. This function will not fail if the directory
 * already exists.
 */
int
create_dir(
	ctx_t	*ctx	/* ARGSUSED */,
	upath_t	dir)
{

	struct stat dir_stat;

	/* make the dir rwx by owner, and rx by other and group */
	static mode_t perms = 0 | S_IRWXU | S_IROTH | S_IXOTH |
	    S_IRGRP | S_IXGRP;

	if (ISNULL(dir)) {
		return (-1);
	}
	Trace(TR_MISC, "creating directory %s", dir);
	errno = 0;
	if (stat(dir, &dir_stat) == 0) {
		if (!S_ISDIR(dir_stat.st_mode)) {

			samerrno = SE_CREATE_DIR_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CREATE_DIR_FAILED), dir, "");
			strlcat(samerrmsg, strerror(ENOTDIR), MAX_MSG_LEN);
			Trace(TR_ERR, "creating directory failed: %s",
			    samerrmsg);

			return (-1);
		}
		Trace(TR_MISC, "directory %s already exists", dir);
		return (0);
	}

	if (errno == ENOENT) {
		errno = 0;
		if (mkdirp(dir, perms) == 0) {
			Trace(TR_MISC, "created directory %s", dir);
			return (0);
		}
	}

	samerrno = SE_CREATE_DIR_FAILED;
	snprintf(samerrmsg, MAX_MSG_LEN,
	    GetCustMsg(SE_CREATE_DIR_FAILED), dir, "");
	strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
	Trace(TR_ERR, "creating directory failed: %s", samerrmsg);
	return (-1);

}

/*
 * Check a stat_t structure against restrictions. Similar to samrdump's
 * check_restrict(), but uses a stat structure;
 */
int
check_restrict_stat(char *name, struct stat64 *ptr, restrict_t *filterp)
{

	if (filterp->flags == 0)
		return (0);	/* Most common case, no restrictions */

	if (filterp->flags & fl_filename) {
		if (fnmatch(filterp->filename, name, 0)) {
			return (1); /* Doesn't match filename */
		}
	}

	/* Next most common, by dates. Restricting any dates? */
	if (filterp->flags & flg_dates) {
		if ((filterp->flags & fl_modbefore) &&
		    (filterp->modbefore < ptr->st_mtim.tv_sec))
			return (1);
		if ((filterp->flags & fl_modafter) &&
		    (filterp->modafter > ptr->st_mtim.tv_sec))
			return (1);
		if ((filterp->flags & fl_creatbefore) &&
		    (filterp->creatbefore < ptr->st_ctim.tv_sec))
			return (1);
		if ((filterp->flags & fl_creatafter) &&
		    (filterp->creatafter > ptr->st_ctim.tv_sec))
			return (1);
	}

	/* Last case, owner/group and size restrictions */
	if (filterp->flags & flg_ownsiz) {
		if ((filterp->flags & fl_user) &&
		    (filterp->user != ptr->st_uid))
			return (1);
		if ((filterp->flags & fl_gid) &&
		    (filterp->gid != ptr->st_gid))
			return (1);
		if ((filterp->flags & fl_biggerthan) &&
		    (filterp->biggerthan > ptr->st_size))
			return (1);
		if ((filterp->flags & fl_smallerthan) &&
		    (filterp->smallerthan < ptr->st_size))
			return (1);
	}
	return (0);		/* Survived all checks, say yes. */
}

/* Macro to produce offset into structure */
#define	rest_off(name) (offsetof(restrict_t, name))

static parsekv_t resttokens[] = {
	{"filename",		rest_off(filename),	parsekv_string_1024},
	{"owner",		rest_off(user),		parse_userid},
	{"group",		rest_off(gid),		parse_gid},
	{"modifiedbefore",	rest_off(modbefore),	parsekv_time},
	{"modifiedafter",	rest_off(modafter),	parsekv_time},
	{"createdbefore",	rest_off(creatbefore),	parsekv_time},
	{"createdafter",	rest_off(creatafter),	parsekv_time},
	{"biggerthan",		rest_off(biggerthan),	parsekv_ll},
	{"smallerthan",		rest_off(smallerthan),	parsekv_ll},
	{"online",		rest_off(online),	parsekv_int},
	{"damaged",		rest_off(damaged),	parsekv_int},
	{"inconsistent",	rest_off(inconsistent),	parsekv_int},
	{"",			0,			NULL}
};

/* Set up restrictions for search or list_versions */

int
set_restrict(char *restrictions, restrict_t *filterp)
{
	int flags = 0;
	int rval;

	if (ISNULL(filterp)) {
		return (-1);
	}

	/* Start with clean filter */
	memset(filterp, 0, sizeof (restrict_t));

	/* Are there any restrictions to check? */
	if (ISNULL(restrictions)) {
		return (0);
	}

	Trace(TR_DEBUG, "Set restriction: %s", restrictions);

	filterp->user = -1;	/* 0 is valid (means root) */
	filterp->gid = -1;
	filterp->online = -1;
	filterp->damaged = -1;
	filterp->inconsistent = -1;

	/* Parse keywords we are passed */
	rval = parse_kv(restrictions, &resttokens[0], filterp);
	if (rval) {
		Trace(TR_MISC, "Parse restrict failed, %s", restrictions);
		return (rval);
	}

	/* Create flags for faster processing */

	/* First-nibble group */
	if (filterp->user != -1)
		flags |= fl_user;
	if (filterp->gid != -1)
		flags |= fl_gid;
	if (filterp->biggerthan != 0)
		flags |= fl_biggerthan;
	if (filterp->smallerthan != 0)
		flags |= fl_smallerthan;

	/* Second-nibble group */

	if (filterp->modbefore != 0)
		flags |= fl_modbefore;
	if (filterp->modafter != 0)
		flags |= fl_modafter;
	if (filterp->creatbefore != 0)
		flags |= fl_creatbefore;
	if (filterp->creatafter != 0)
		flags |= fl_creatafter;

	/* Filename checking goes by itself. */
	if (*(filterp->filename) != '\0')
		flags |= fl_filename;

	if (filterp->online == 1) {
		flags |= fl_online;
	} else if (filterp->online == 0) {
		flags |= fl_offline;
	}

	if (filterp->damaged == 1) {
		flags |= fl_damaged;
	} else if (filterp->damaged == 0) {
		flags |= fl_undamaged;
	}

	if (filterp->inconsistent == 1) {
		flags |= fl_inconsistent;
	} else if (filterp->inconsistent == 0) {
		flags |= fl_consistent;
	}

	filterp->flags = flags;

	return (rval);
}

/*
 *  filepath may be either a directory or a fully-qualified path.
 *  if it's fully-qualified, only directory entries that sort alphabetically
 *  after the specified file will be returned.
 *
 *  morefiles will be set if there are more entries left in the directory
 *  after maxentries have been returned.  This is intended to let the caller
 *  know they can continue reading.
 *
 *  Note that the directory may change while we're reading it.  If it does,
 *  files that have been added or removed since we started reading it may
 *  not be accurately reflected.
 */
int
list_directory(
	ctx_t		*c,			/* ARGSUSED */
	int		maxentries,
	char		*listDir,		/* directory to list */
	char		*startFile,		/* if continuing, start here */
	char		*restrictions,
	uint32_t	*morefiles,		/* OUT */
	sqm_lst_t		**direntries)		/* OUT */
{
	int		rval = 0;
	int		st = 0;
	DIR		*curdir; /* Variable for directory system calls */
	dirent64_t	*entry;	/* Pointer to a directory entry */
	dirent64_t	*entryp;
	struct stat64	sout;
	restrict_t	filter = {0};
	char		*data;	/* Pointer to data item to add to list */
	node_t		*node;
	sqm_lst_t		*lstp = NULL;
	char		buf[MAXPATHLEN + 1];
	char		*fname;

	if (ISNULL(listDir, direntries, morefiles)) {
		return (-1);
	}

	*morefiles = 0;

	/* Set up wildcard restrictions */
	rval = set_restrict(restrictions, &filter);
	if (rval) {
		return (rval);
	}

	curdir = opendir(listDir); /* Set up to ask for directory entries */
	if (curdir == NULL) {
		return (samrerr(SE_NOSUCHPATH, listDir));
	}

	*direntries = lst_create(); /* Return results in this list */
	if (*direntries == NULL) {
		closedir(curdir);
		return (-1);	/* If allocation failed, samerr is set */
	}
	lstp = *direntries;

	entry = mallocer(sizeof (struct dirent64) + MAXPATHLEN + 1);
	if (entry == NULL) {
		closedir(curdir);
		lst_free(*direntries);
		*direntries = NULL;
		return (-1);
	}

	/* Walk through directory entries */
	while ((rval = readdir64_r(curdir, entry, &entryp)) == 0) {
		if (entryp == NULL) {
			break;
		}

		fname = (char *)&(entry->d_name[0]);

		if ((strcmp(fname, ".") == 0) ||
		    (strcmp(fname, "..") == 0)) {
			continue;
		}

		/*
		 * If we were given a non-directory, start after
		 * that file alphabetically.
		 */
		if (startFile != NULL) {
			if ((strcmp(fname, startFile)) <= 0) {
				continue;
			}
		}

		/* Create full pathname and get stat info */
		snprintf(buf, sizeof (buf), "%s/%s", listDir, fname);
		if (lstat64(buf, &sout) != 0) {
			continue; /* Ignore file which can't be stat'ed */
		}

		/*
		 * think about ways to avoid a double-stat in when we're
		 * fetching file details
		 */
		if (check_restrict_stat(fname, &sout, &filter)) {
			continue; /* Not this entry */
		}

		/* copy to allocated struct */
		data = copystr(fname);
		if (data == NULL) {
			rval = -1;
			break;	/* samerr already set */
		}

		/*
		 * caller wants all entries for the directory
		 * should there be a top-end limit, to avoid the case where
		 * the directory has millions of entries?
		 */
		if (maxentries <= 0) {
			rval = lst_append(lstp, data);
			if (rval != 0) {
				free(data);
				break;
			}
			continue;
		}

		/*
		 * Directory may have more entries than requested, so pre-sort
		 * the list so we return the first <n> sorted alphabetically.
		 */
		for (node = lstp->head; node != NULL; node = node->next) {

			st = strcmp(data, (char *)(node->data));
			if (st > 0) {
				continue;
			}

			if (st < 0) {
				rval = lst_ins_before(lstp, node, data);
				data = NULL;
			}

			if ((rval != 0) || (st == 0)) {
				free(data);
				data = NULL;
			}
			break;
		}

		/* entry sorts higher than existing entries */
		if (data != NULL) {
			if (lstp->length < maxentries) {
				rval = lst_append(lstp, data);
				if (rval != 0) {
					free(data);
					break;
				}
			} else {
				/* no room for this entry */
				free(data);
				(*morefiles)++;
			}
		}

		/* Keep list to designated limits */
		if (lstp->length > maxentries) {
			/* pop off the last entry */
			lst_remove(lstp, lstp->tail);
			(*morefiles)++;
		}
	}

	closedir(curdir);
	free(entry);

	if (rval) {
		lst_free_deep(*direntries);
		*direntries = NULL;
	} else if (maxentries <= 0) {
		lst_qsort(*direntries, node_cmp);
	}

	return (rval);
}

/*
 * delete files
 *
 * unlinks the list of files, the fully qualified paths have to be
 * provided as input
 */
int
delete_files(
ctx_t *c /* ARGSUSED */,
sqm_lst_t *paths)
{

	char *path	= NULL;
	node_t *n	= NULL;

	if (ISNULL(paths)) {
		Trace(TR_ERR, "Failed to delete files:%d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	n = paths->head;
	while (n != NULL) {
		path = (char *)n->data;
		if (ISNULL(path)) {
			Trace(TR_ERR, "Failed to delete files:%d %s",
			    samerrno, samerrmsg);
			return (-1);
		}

		if (path[0] == '/') {
			unlink(path);
		} else {
			samerrno = SE_DELETE_FILE_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_DELETE_FILE_FAILED), path);
			Trace(TR_ERR, "Failed to delete files:%d %s",
			    samerrno, samerrmsg);

			return (-1);
		}
		n = n->next;
	}
	return (0);
}
