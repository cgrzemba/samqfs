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
#pragma ident   "$Revision: 1.22 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


/*
 * arch_state.c contains implementations for functions from archive.h that
 * are related to fetching state/status information from archiver.
 */
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>

#include "pub/mgmt/error.h"
#include "sam/mount.h" /* for GetFsStatus */
#include "sam/lib.h"
#include "pub/mgmt/types.h"
#include "sam/sam_trace.h"
#include "pub/mgmt/archive.h"
#include "mgmt/util.h"
#include "aml/archiver.h"
#include "sam/mount.h"
#include "pub/mgmt/types.h"

static struct ArchiverdState *adState = NULL;

static int attachArchiverdStateFile(void);

static int get_queue_file(char *fs_name, char *ar_name,
	struct ArchReq **ret_val);

static int dup_arch_req(struct ArchReq *ar, struct ArchReq **copy);


/*
 * Function to get the status of the archiverd daemon.
 * see include/aml/archiver.h for ArchiverdState.
 */
int
get_archiverd_state(
ctx_t *ctx			/* ARGSUSED */,
struct ArchiverdState **ads)	/* ArchiverdState must be freed by caller */
{

	size_t size;

	Trace(TR_MISC, "getting archiverd state");
	if (ISNULL(ads)) {
		Trace(TR_ERR, "getting archiverd state failed: %s", samerrmsg);
		return (-1);
	}
	if (attachArchiverdStateFile() != 0) {
		Trace(TR_ERR, "getting archiverd state failed: %s", samerrmsg);
		return (-1);
	}

	/* allocate memory for archiverd state including the dynamic array */
	size = sizeof (struct ArchiverdState) +
	    ((adState->AdCount - 1) * sizeof (adState->AdArchReq));

	*ads = mallocer(size);
	if (*ads == NULL) {
		Trace(TR_ERR, "getting archiverd state failed: %s", samerrmsg);
		return (-1);
	}

	memcpy(*ads, adState, size);

	MapFileDetach(adState);
	adState = NULL;

	Trace(TR_MISC, "got archiverd state");
	return (0);
}


/*
 * Attach archiver daemon state file.
 */
static int
attachArchiverdStateFile(void)
{

	Trace(TR_DEBUG, "attaching archiverd state file");

	if (NULL != adState && adState->Ad.MfValid == 0) {
		MapFileDetach(adState);
		adState = NULL;
	}
	if (NULL != adState) {
		samerrno = SE_GET_ARCHIVERD_STATE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_GET_ARCHIVERD_STATE_FAILED), "NULL");

		Trace(TR_DEBUG, "attaching archiverd state file failed: %s",
		    samerrmsg);
		return (-1);
	}


	adState = MapFileAttach(ARCHIVER_DIR"/"ARCHIVER_STATE, AD_MAGIC,
	    O_RDONLY);
	if (adState == NULL) {

		samerrno = SE_GET_ARCHIVERD_STATE_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_GET_ARCHIVERD_STATE_FAILED), "");
		strlcat(samerrmsg, GetCustMsg(331), MAX_MSG_LEN);

		Trace(TR_ERR, "attaching archiverd state file failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (AD_VERSION != adState->AdVersion) {
		int		version;
		char		msgBuf[MAX_MSG_LEN];

		version = adState->AdVersion;
		MapFileDetach(adState);
		adState = NULL;
		samerrno = SE_GET_ARCHIVERD_STATE_FAILED;

		/*
		 * "Archiver shared memory version mismatch.
		 * Is: %d, should be: %d"
		 */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_GET_ARCHIVERD_STATE_FAILED), "");
		snprintf(msgBuf, MAX_MSG_LEN, GetCustMsg(440), version,
		    AD_VERSION);
		strlcat(samerrmsg, msgBuf, MAX_MSG_LEN);

		Trace(TR_ERR, "attaching archiverd state file failed: %s",
		    samerrmsg);

		return (-1);
	}

	Trace(TR_DEBUG, "attached archiverd state file");

	return (0);
}


/*
 * get the status of the arfind process for a given file system.
 * GUI note: This contains the status for the archive management page
 */
int
get_arfind_state(
ctx_t *ctx,			/* ARGSUSED */
uname_t fsname,			/* name of fs to get status for */
ar_find_state_t **state)	/* malloced return */
{

	struct ArfindState *af;
	Trace(TR_MISC, "getting arfind state");

	if (ISNULL(fsname, state)) {
		Trace(TR_ERR, "getting arfind state failed: %s",
		    samerrmsg);

		return (-1);
	}


	af = ArfindAttach(fsname, O_RDONLY);
	if (af == NULL) {
		/* " not archiving" */
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), fsname);

		Trace(TR_ERR, "getting arfind state failed: %s",
		    samerrmsg);

		return (-1);
	}

	*state = mallocer(sizeof (ar_find_state_t));
	if (*state == NULL) {
		Trace(TR_ERR, "getting arfind state failed: %s",
		    samerrmsg);
		return (-1);
	}
	memcpy(&((*state)->state), af, sizeof (struct ArfindState));
	strlcpy((*state)->fs_name, fsname, sizeof (uname_t));

	MapFileDetach(af);
	Trace(TR_MISC, "got arfind state(%s)", Str(fsname));
	return (0);
}


/*
 * get the ar_find_state_t structures for all archiving filesystems.
 */
int
get_all_arfind_state(
ctx_t *ctx		/* ARGSUSED */,
sqm_lst_t **find_states)	/* malloced list of ar_find_state_t */
{

	struct sam_fs_status	*fs_array;
	ar_find_state_t		*tmp_state;
	int fs_count;
	int i;

	Trace(TR_MISC, "getting all arfind state");
	if (ISNULL(find_states)) {
		Trace(TR_ERR, "getting all arfind state failed: %s",
		    samerrmsg);
		return (-1);
	}

	*find_states = lst_create();
	if (*find_states == NULL) {
		Trace(TR_ERR, "getting all arfind state failed: %s",
		    samerrmsg);

		return (-1);
	}

	if ((fs_count = GetFsStatus(&fs_array)) == -1) {

		/*
		 * sam is not running. return empty list and -2
		 * to indicate no arfind states should exist since
		 * sam is not running.
		 */
		samerrno = SE_GET_FS_STATUS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));

		Trace(TR_ERR, "getting all arfind state failed: %s",
		    samerrmsg);
		return (-2);
	}

	/* for each fs get an arfind state */
	for (i = 0; i < fs_count; i++) {

		if (get_arfind_state(NULL, fs_array[i].fs_name,
		    &tmp_state) != 0) {

			if (samerrno == SE_NOT_FOUND) {
				/* not found means non-archiving. */
				continue;
			}

			free(fs_array);
			lst_free_deep(*find_states);
			*find_states = NULL;
			Trace(TR_ERR, "getting all arfind state failed: %s",
			    samerrmsg);
			return (-1);

		}

		if (lst_append(*find_states, tmp_state) != 0) {
			free(fs_array);
			lst_free_deep(*find_states);
			*find_states = NULL;
			free(tmp_state);
			Trace(TR_ERR, "getting all arfind state failed: %s",
			    samerrmsg);

			return (-1);
		}
	}

	free(fs_array);
	Trace(TR_MISC, "got all arfind state");

	return (0);
}


/*
 * Returns a list of all arch reqs.
 * arch reqs contain information about each of the copy processes for an
 * archive set within a file system.
 */
int
get_all_archreqs(
ctx_t *ctx		/* ARGSUSED */,
sqm_lst_t **archreqs)	/* malloced ist of ArchReq structs */
{

	struct sam_fs_status	*fs_array;
	struct sam_fs_info	fi;
	int fs_count;
	int i;
	sqm_lst_t *tmp;

	Trace(TR_MISC, "getting all archreqs");

	*archreqs = lst_create();
	/* get all file systems. */

	if ((fs_count = GetFsStatus(&fs_array)) == -1) {

		samerrno = SE_GET_FS_STATUS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_ERR, "getting all archreqs failed: %s",
		    samerrmsg);

		return (-2);
	}

	if (*archreqs == NULL) {
		free(fs_array);
		Trace(TR_ERR, "getting all archreqs failed: %s", samerrmsg);
		return (-1);
	}


	/* for each fs append all archreqs to the list */
	for (i = 0; i < fs_count; i++) {
		if (GetFsInfo(fs_array[i].fs_name, &fi) == -1) {
			if (!(fi.fi_config & MT_SAM_ENABLED)) {
				/* no_archive */
				continue;
			}
		}

		if (get_archreqs(NULL, fs_array[i].fs_name, &tmp) != 0) {
			free(fs_array);
			return (-1);
		}

		if (lst_concat(*archreqs, tmp) != 0) {
			free(fs_array);
			Trace(TR_ERR, "getting all archreqs()%s exit",
			    samerrmsg);
			return (-1);
		}
	}

	free(fs_array);
	Trace(TR_MISC, "got all archreqs");
	return (0);
}


/*
 * get_archreqs gets all archreqs for the named file system.
 */
int
get_archreqs(
ctx_t *ctx			/* ARGSUSED */,
uname_t fsname,			/* name of fs to get archreqs for */
sqm_lst_t **archreqs)		/* return list of ArchReq structs */
{

	char path[MAXPATHLEN+1];
	DIR *dirp;
	dirent64_t *dirent;
	dirent64_t *direntp;
	struct ArchReq *ar;

	Trace(TR_MISC, "getting archreqs");

	dirent = mallocer((sizeof (dirent_t)) + MAXPATHLEN + 1);
	if (dirent == NULL) {
		Trace(TR_ERR, "getting archreqs failed: %s", samerrmsg);
		return (-1);
	}

	*archreqs = lst_create();
	if (*archreqs == NULL) {
		free(dirent);
		Trace(TR_ERR, "getting archreqs failed: %s", samerrmsg);
		return (-1);
	}

	snprintf(path, sizeof (path), ARCHIVER_DIR"/%s/"ARCHREQ_DIR,
	    fsname);

	if ((dirp = opendir(path)) == NULL) {
		if (errno == ENOENT) {
			Trace(TR_MISC, "no directory for %s archreqs",
			    Str(fsname));
			return (0);
		}
		samerrno = SE_GET_ARCHREQS_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_GET_ARCHREQS_FAILED), path);

		lst_free_deep(*archreqs);
		Trace(TR_ERR, "getting archreqs failed: %s",
		    samerrmsg);

		return (-1);
	}
	while ((readdir64_r(dirp, dirent, &direntp)) == 0) {
		if (direntp == NULL) {
			break;
		}
		/* skip the . entry but do all others. */
		if (*dirent->d_name == '.') {
			continue;
		}

		if (get_queue_file(fsname, dirent->d_name, &ar) != 0) {
			closedir(dirp);
			free(dirent);
			lst_free_deep(*archreqs);
			Trace(TR_ERR, "%s()%s%s",
			    "getting archreqs", " failed: ", samerrmsg);
			return (-1);

		} else {
			if (lst_append(*archreqs, ar) != 0) {
				lst_free_deep(*archreqs);
				free(dirent);
				closedir(dirp);
				Trace(TR_ERR, "%s()%s%s", "getting archreqs",
				    " failed: ", samerrmsg);
				return (-1);
			}
		}
	}

	closedir(dirp);

	free(dirent);
	Trace(TR_MISC, "got archreqs");
	return (0);
}

/*
 * internal function to get an archreq.
 */
static int
get_queue_file(
char *fs_name,			/* file system name */
char *ar_name,			/* archive set copy name */
struct ArchReq **ret_val)	/* return val must be freed by caller */
{

	struct ArchReq *ar;
	char err_buf[MAX_MSG_LEN];

	Trace(TR_DEBUG, "getting queue file");

	if (fs_name != NULL) {
		ar = ArchReqAttach(fs_name, ar_name, O_RDONLY);
	} else {
		ar = MapFileAttach(ar_name, ARCHREQ_MAGIC, O_RDONLY);
	}

	if (ar != NULL) {
		if (ar->ArVersion == ARCHREQ_VERSION) {
			dup_arch_req(ar, ret_val);
		} else {
			samerrno = SE_ARCHREQ_VERSION_MISMATCH;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_ARCHREQ_VERSION_MISMATCH),
			    ar_name, ar->ArVersion, ARCHREQ_VERSION);

			Trace(TR_ERR, "getting queue file failed: %s",
			    samerrmsg);
			return (-1);
		}
		(void) MapFileDetach(ar);
	} else {
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		samerrno = SE_CANNOT_GET_ARCHREQ;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CANNOT_GET_ARCHREQ), ar_name, err_buf);

		Trace(TR_ERR, "getting queue file failed: %s", samerrmsg);

		return (-1);
	}
	Trace(TR_DEBUG, "got queue file");
	return (0);
}


/*
 * This NON-RPC function will extract the name of the file currently being
 * copied. If a file is not currently being copied this will set file_name
 * to '\0'.
 */
int
get_current_file_name(
struct ArcopyInstance *copy,	/* input to get name from */
upath_t file_name)		/* file currently being copied */
{

	char *msg;
	size_t count;

	/*
	 * check the prefix against the catalog to check that this is the
	 * the correct message to parse.
	 */
	msg = GetCustMsg(4308);
	count = strcspn(msg, "%");


	if (strncmp(copy->CiOprmsg, msg, count) == 0) {
		sscanf(copy->CiOprmsg, msg, file_name);
	} else {
		file_name[0] = '\0';
	}

	return (0);
}


/*
 * This NON-RPC function will extract the stage_vsn and media type from the
 * ArcopyInstance's operator message. If a file is not currently being
 * staged the vsn[0], file_name[0] and media_type[0] will be set to '\0'.
 * If a vsn is returned it means that the archiver is waiting on a stage
 * from this vsn.
 */
int
get_stage_vsn(
struct ArcopyInstance *copy,	/* The arcopy from which to get stage vsn */
upath_t file_name,		/* the name of the file being staged */
vsn_t vsn_name,			/* vsn being staged from */
mtype_t media_type)		/* media type being staged from */
{

	int i;
	int j;
	char *fmt = "Staging - file %d (%d) from %s";
	char *fmt2 = "%s %s";
	char *dot;
	char *msg;
	size_t count;


	Trace(TR_MISC, "getting stage_vsn");

	if (ISNULL(file_name, vsn_name, media_type)) {
		Trace(TR_ERR, "getting stage_vsn failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * The format of the string we can accept is:
	 * Staging - file %d (%d) from %s.%s  %s/%s
	 *
	 * with inputs set as follows in waitForOnline of arcopy.c
	 * PostOprMsg(4348, fn, nextFile, sam_mediatoa(oi->OiMedia), oi->OiVsn,
	 *	MntPoint, File->f->FiName);
	 */
	/* check the prefix against the message. */

	/*
	 * check the prefix against the catalog to check that this is the
	 * the correct message to parse.
	 */
	msg = GetCustMsg(4348);
	count = strcspn(msg, "%");

	if (strncmp(copy->CiOprmsg, msg, count) != 0) {
		/* this message does not match the one we are looking for. */
		file_name[0] = '\0';
		vsn_name[0] = '\0';
		media_type[0] = '\0';
		Trace(TR_MISC, "getting stage_vsn no match");
		return (0);
	}

	/* find a landmark and extract what we want */
	dot = strchr(copy->CiOprmsg, '.');
	*dot = '\0';
	sscanf(copy->CiOprmsg, fmt, &i, &j, media_type);

	*dot = '.';
	sscanf(++dot, fmt2, vsn_name, file_name);

	Trace(TR_MISC, "got stage_vsn");
	return (0);
}

/*
 * make a copy of the archreq.
 */
static int
dup_arch_req(
struct ArchReq *ar,	/* input */
struct ArchReq **copy)	/* output */
{

	size_t size;

	Trace(TR_DEBUG, "duplicate archreq");

	size = sizeof (struct ArchReq) +
	    (ar->ArDrives * sizeof (struct ArcopyInstance));

	*copy = mallocer(size);
	if (*copy == NULL) {
		Trace(TR_DEBUG, "duplicate archreq failed: %s", samerrmsg);
		return (-1);
	}
	memcpy(*copy, ar, size);

	Trace(TR_DEBUG, "duplicated archreq");
	return (0);
}
