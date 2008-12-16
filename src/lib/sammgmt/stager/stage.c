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
#pragma ident   "$Revision: 1.36 $"

static char *_SrcFile = __FILE__;  /* Using __FILE__ makes duplicate strings */

/*
 * stage.c contains control side implementation of stage.h.
 */

#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#include <pwd.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include "aml/id_to_path.h"
#include "aml/catalog.h"
#include "aml/stager.h"
#include "aml/stager_defs.h"
#include "sam/lib.h"
#include "sam/sam_trace.h"
#include "sam/syscall.h"
#include "mgmt/util.h"
#include "mgmt/config/common.h"

#include "pub/lib.h"
#include "pub/stat.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/stage.h"
#include "pub/mgmt/process_job.h"

/* private helper functions */
static void *map_file(char *, int, size_t *);
static void unmap_file(void *, size_t);
static int get_stream_files(StreamInfo_t *, FileInfo_t *, FileInfo_t **);
static int get_current_file(sqm_lst_t *, stager_stream_t *);
static int map_active_file(stager_info_t *);
static int dodir(char *);
static int adddir(char *);
static char optsbuf[SAM_MAX_OPS_LEN];
static int st_compare_filename_ascending(const void *, const void *);
static int st_compare_filename_descending(const void *, const void *);
static int st_compare_uid_ascending(const void *, const void *);
static int st_compare_uid_descending(const void *, const void *);
static int map_stager_state_file(StagerStateInfo_t **state, size_t *state_len);

static int expand_stage_options(int32_t options, char *buf, int len);
static int stager_ready();

/*
 * This function will be used from outside of this file but
 * only through a function pointer.
 */
int display_stage_activity(samrthread_t *ptr, char **result);


/* private data */
static struct sam_stat sb;
static char fullpath[MAXPATHLEN + 4]; /* current full path name */
static char *base_name = fullpath; /* position in fullpath of base name */
static char *dir_names = NULL; /* unprocessed directory base names */
static int dn_size = 0; /* current size of dir_names */
static int which_dofile = 0; /* 0/1 for dofile/dofile_no_options */
static int (*dofile)(const char *name, const char *opns);
static int (*dofile_no_options)(const char *name);
static int dofile_error;
static int (*chk_file) (void);

/*
 * staging status information
 */
int
get_stager_info(
ctx_t *ctx,				/* ARGSUSED */
stager_info_t **info)	/* OUT - stager info */
{
	StagerStateInfo_t *state = NULL;
	StreamInfo_t *stream = NULL;
	size_t state_len;
	size_t stream_len;
	upath_t fullpath;
	stager_info_t *st_info = NULL;
	active_stager_info_t *st_active = NULL;
	stager_stream_t *st_stream = NULL;
	time_t now;
	int i;



	if (ISNULL(info)) {
		Trace(TR_OPRMSG, "null argument found");
		goto err;
	}

	st_info = (stager_info_t *)mallocer(sizeof (stager_info_t));
	if (st_info == NULL) {
		Trace(TR_OPRMSG, "out of memory");
		goto err;
	}

	memset(st_info, 0, sizeof (stager_info_t));

	Trace(TR_OPRMSG, "mapping in stager state file");

	if (map_stager_state_file(&state, &state_len) != 0) {
		/* NOTE: JNI/GUI client requests API to return an empty */
		/*  structure when stager daemon is not running! */

		*info = st_info;
		Trace(TR_MISC, "returned empty stager info");
		return (-2);
	}

	Trace(TR_OPRMSG, "filling in stager info structure");

	st_info->active_stager_info = lst_create();
	st_info->stager_streams = lst_create();
	if (st_info->active_stager_info == NULL ||
	    st_info->stager_streams == NULL) {
		Trace(TR_OPRMSG, "lst create failed");
		goto err;
	}

	snprintf(st_info->log_file, sizeof (st_info->log_file), state->logFile);
	snprintf(st_info->msg, sizeof (st_info->msg), state->errmsg);
	snprintf(st_info->streams_dir, sizeof (st_info->streams_dir),
	    state->streamsDir);
	snprintf(st_info->stage_req, sizeof (st_info->stage_req),
	    state->stageReqsFile);

	Trace(TR_OPRMSG, "getting active stager info");

	for (i = 0; i < STAGER_DISPLAY_ACTIVE; i++) {
		if (state->active[i].flags == 0) {
			break;
		}

		st_active = (active_stager_info_t *)
		    mallocer(sizeof (active_stager_info_t));
		if (st_active == NULL) {
			Trace(TR_OPRMSG, "out of memory");
			goto err;
		}

		memcpy(st_active, &state->active[i],
		    sizeof (active_stager_info_t));

		if (lst_append(st_info->active_stager_info, st_active) != 0) {
			Trace(TR_OPRMSG, "lst append failed");
			goto err;
		}
	}

	st_active = NULL;

	Trace(TR_OPRMSG, "getting stager streams info");

	for (i = 0; i < STAGER_DISPLAY_STREAMS; i++) {
		sprintf(fullpath, "%s/%s.%d", state->streamsDir,
		    state->streams[i].vsn, state->streams[i].seqnum);

		stream = (StreamInfo_t *)
		    map_file(fullpath, O_RDWR, &stream_len);
		if (stream == NULL) {
			continue;
		}

		st_stream = (stager_stream_t *)
		    mallocer(sizeof (stager_stream_t));
		if (st_stream == NULL) {
			unmap_file(stream, stream_len);
			Trace(TR_OPRMSG, "out of memory");
			goto err;
		}
		memset(st_stream, 0, sizeof (stager_stream_t));

		st_stream->active = state->streams[i].active;
		snprintf(st_stream->media, sizeof (st_stream->media),
		    sam_mediatoa(state->streams[i].media));
		st_stream->seqnum = state->streams[i].seqnum;
		snprintf(st_stream->vsn, sizeof (st_stream->vsn),
		    state->streams[i].vsn);
		st_stream->count = stream->count;
		st_stream->create = stream->create;
		now = time(NULL);
		st_stream->age = now - st_stream->create;

		snprintf(st_stream->msg, sizeof (st_stream->msg),
		    state->streams[i].oprmsg);

		unmap_file(stream, stream_len);

		if (lst_append(st_info->stager_streams, st_stream) != 0) {
			Trace(TR_OPRMSG, "lst append failed");
			goto err;
		}
	}

	st_stream = NULL;

	unmap_file(state, state_len);
	state = NULL;

	/*
	 * The staging information is contained in two lists
	 * active streams list and the (all) streams list
	 *
	 * check if stream is active and map the file details from the
	 * active stream into the stager stream.
	 */
	if (map_active_file(st_info) != 0) {
		Trace(TR_OPRMSG, "map active files failed");
		goto err;
	}

	*info = st_info;

	Trace(TR_MISC, "got stager info");
	return (0);

err:
	if (state) {
		unmap_file(state, state_len);
	}
	if (st_info) {
		free_stager_info(st_info);
	}
	if (st_active) {
		free(st_active);
	}
	if (st_stream) {
		free(st_stream);
	}

	Trace(TR_ERR, "get stager info failed: %s", samerrmsg);
	return (-1);
}


static int
map_stager_state_file(
StagerStateInfo_t **state,
size_t *state_len)
{

	upath_t fullpath;


	Trace(TR_OPRMSG, "mapping in stager state file");

	sprintf(fullpath, "%s/%s/%s", VAR_DIR, STAGER_DIRNAME,
	    STAGER_STATE_FILENAME);

	*state = (StagerStateInfo_t *)map_file(fullpath, O_RDONLY, state_len);

	if (*state == NULL) {
		samerrno = SE_STAGERD_NOT_RUN;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_OPRMSG, "unable to map in stager state file");

		return (-1);
	}

	Trace(TR_OPRMSG, "mapped stager state file");
	return (0);
}
/*
 * get total number of staging files
 *
 * RETURNS:
 *    0 -- success
 *   -1 -- error
 *   -2 -- stagerd not running
 */
int
get_total_staging_files(
ctx_t *ctx,		/* ARGSUSED */
size_t *total)	/* OUT - total number of staging files */
{
	StagerStateInfo_t *state = NULL;
	StreamInfo_t *stream = NULL;
	size_t state_len;
	size_t stream_len;
	upath_t fullpath;
	int i;

	Trace(TR_MISC, "getting total number of staging files");

	if (ISNULL(total)) {
		Trace(TR_OPRMSG, "null argument found");
		goto err;
	}

	*total = 0;


	Trace(TR_OPRMSG, "mapping in stager state file");

	sprintf(fullpath, "%s/%s/%s", VAR_DIR, STAGER_DIRNAME,
	    STAGER_STATE_FILENAME);
	state = (StagerStateInfo_t *)map_file(fullpath, O_RDONLY, &state_len);

	if (state == NULL) {
		samerrno = SE_STAGERD_NOT_RUN;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_OPRMSG, "unable to map in stager state file");

		Trace(TR_MISC, "returned 0 for total staging files");
		return (-2);
	}


	Trace(TR_OPRMSG, "calculating total number of staging files");

	for (i = 0; i < STAGER_DISPLAY_STREAMS; i++) {
		sprintf(fullpath, "%s/%s.%d", state->streamsDir,
		    state->streams[i].vsn, state->streams[i].seqnum);

		stream = (StreamInfo_t *)
		    map_file(fullpath, O_RDWR, &stream_len);
		if (stream == NULL) {
			continue;
		}

		*total += stream->count;

		unmap_file(stream, stream_len);
	}


	unmap_file(state, state_len);
	state = NULL;


	Trace(TR_MISC, "got total number of staging files: %u", *total);
	return (0);

err:
	if (state) {
		unmap_file(state, state_len);
	}

	Trace(TR_ERR, "get total staging files failed: %s", samerrmsg);
	return (-1);
}


/*
 * get number of staging files in the specified stream
 *
 * RETURNS:
 *    0 -- success
 *   -1 -- error
 */
int
get_numof_staging_files_in_stream(
ctx_t *ctx,					/* ARGSUSED */
stager_stream_t *in_stream,	/* IN - stager stream */
size_t *num)				/* OUT - num of staging files */
{
	StagerStateInfo_t *state = NULL;
	StreamInfo_t *stream = NULL;
	size_t state_len;
	size_t stream_len;
	upath_t fullpath;
	int i;

	Trace(TR_MISC, "getting num of staging files in stream");

	if (ISNULL(in_stream, num)) {
		Trace(TR_OPRMSG, "null argument found");
		goto err;
	}

	*num = 0;


	Trace(TR_OPRMSG, "mapping in stager state file");

	sprintf(fullpath, "%s/%s/%s", VAR_DIR, STAGER_DIRNAME,
	    STAGER_STATE_FILENAME);
	state = (StagerStateInfo_t *)map_file(fullpath, O_RDONLY, &state_len);

	if (state == NULL) {
		samerrno = SE_STAGERD_NOT_RUN;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_OPRMSG, "unable to map in stager state file");
		goto err;
	}


	Trace(TR_OPRMSG, "searching for the given stream");

	for (i = 0; i < STAGER_DISPLAY_STREAMS; i++) {
		if (state->streams[i].vsn[0] == '\0') {
			break;
		}

		if (in_stream->seqnum != state->streams[i].seqnum ||
		    strcmp(in_stream->vsn, state->streams[i].vsn) != 0) {
			continue;
		}

		sprintf(fullpath, "%s/%s.%d", state->streamsDir,
		    state->streams[i].vsn, state->streams[i].seqnum);

		stream = (StreamInfo_t *)
		    map_file(fullpath, O_RDWR, &stream_len);
		if (stream == NULL) {
			break;
		}

		*num = stream->count;

		unmap_file(stream, stream_len);
		break;
	}


	unmap_file(state, state_len);


	Trace(TR_OPRMSG, "returned num of staging files in stream: %u", *num);
	Trace(TR_MISC, "got num of staging files in stream");
	return (0);

err:
	if (state) {
		unmap_file(state, state_len);
	}

	Trace(TR_ERR, "get num of staging files in stream failed: %s",
	    samerrmsg);
	return (-1);
}


/*
 * staging queue information
 */
int
get_all_staging_files(
ctx_t *ctx,						/* IN - not used */
sqm_lst_t **staging_file_infos)	/* OUT - staging_file_info_t list */
{
	return (get_staging_files(ctx, 0, -1, staging_file_infos));
}


/* user can specify the starting point and number of entries */
int
get_staging_files(
ctx_t *ctx,	/* ARGSUSED */
int start,	/* IN - starting index in the list */
int size,	/* IN - num of entries to return, -1: all remaining */
sqm_lst_t **staging_file_infos)	/* OUT - staging_file_info_t list */
{
	StagerStateInfo_t *state = NULL;
	FileInfo_t *request = NULL;
	StreamInfo_t *stream = NULL;
	size_t state_len;
	size_t req_len;
	size_t stream_len;
	upath_t fullpath;
	FileInfo_t *filearr = NULL;
	FileInfo_t *file;
	sqm_lst_t *files = NULL;
	staging_file_info_t *f = NULL;
	struct passwd *pw = NULL;
	int num, total = 0;
	int i, j;

	Trace(TR_MISC, "getting staging files");

	if (ISNULL(staging_file_infos)) {
		Trace(TR_OPRMSG, "null argument found");
		goto err;
	}

	*staging_file_infos = NULL;

	Trace(TR_OPRMSG, "start: %d, size: %d", start, size);

	files = lst_create();
	if (files == NULL) {
		Trace(TR_OPRMSG, "lst create failed");
		goto err;
	}

	if (size == 0) {
		*staging_file_infos = files;
		Trace(TR_OPRMSG, "returned empty list as requested");
		Trace(TR_MISC, "got 0 staging files");
		return (0);
	}

	Trace(TR_OPRMSG, "mapping in stager state file");

	sprintf(fullpath, "%s/%s/%s", VAR_DIR, STAGER_DIRNAME,
	    STAGER_STATE_FILENAME);
	state = (StagerStateInfo_t *)map_file(fullpath, O_RDONLY, &state_len);

	if (state == NULL) {
		samerrno = SE_STAGERD_NOT_RUN;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno));
		Trace(TR_OPRMSG, "unable to map in stager state file");
		Trace(TR_OPRMSG, "stagerd is probably not running");
		*staging_file_infos = files;
		Trace(TR_MISC, "got 0 staging files");
		return (-2);
	}

	Trace(TR_OPRMSG, "mapping in stage requests");

	request = (FileInfo_t *)
	    map_file(state->stageReqsFile, O_RDWR, &req_len);
	if (request == NULL) {
		unmap_file(state, state_len);
		*staging_file_infos = files;
		Trace(TR_OPRMSG, "no stage requests");
		Trace(TR_MISC, "got 0 staging files");
		return (0);
	}

	Trace(TR_OPRMSG, "creating staging files list");

	for (i = 0; i < STAGER_DISPLAY_STREAMS; i++) {
		if (state->streams[i].vsn[0] == '\0') {
			break;
		}

		sprintf(fullpath, "%s/%s.%d", state->streamsDir,
		    state->streams[i].vsn, state->streams[i].seqnum);

		stream = (StreamInfo_t *)
		    map_file(fullpath, O_RDWR, &stream_len);
		if (stream == NULL) {
			continue;
		}

		num = get_stream_files(stream, request, &filearr);
		if (num == -1) {
			unmap_file(stream, stream_len);
			Trace(TR_OPRMSG, "get stream files failed");
			goto err;
		}

		if (start >= (total + num)) {
			total += num;
			goto ext;
		}

		for (j = 0; j < num; j++) {

			total++;
			if (start >= total) {
				continue;
			}

			file = &filearr[j];

			f = (staging_file_info_t *)
			    mallocer(sizeof (staging_file_info_t));

			if (f == NULL) {
				free(filearr);
				unmap_file(stream, stream_len);
				Trace(TR_OPRMSG, "out of memory");
				goto err;
			}

			f->id = file->id;
			f->fseq = file->fseq;
			f->copy = file->copy;
			f->len = file->len;
			f->position = file->ar[f->copy].section.position;
			f->offset = file->ar[f->copy].section.offset;
			snprintf(f->media, sizeof (f->media),
			    sam_mediatoa(file->ar[f->copy].media));
			snprintf(f->vsn, sizeof (f->vsn),
			    file->ar[f->copy].section.vsn);
			snprintf(f->filename, sizeof (f->filename),
			    id_to_path(GetFsMountName(f->fseq), f->id));
			f->pid = file->pid;

			pw = getpwuid(getuid());

			if (pw != NULL) {
				snprintf(f->user, sizeof (f->user),
				    pw->pw_name);
			} else {
				snprintf(f->user, sizeof (f->user),
				    "Unknown user");
			}

			if (lst_append(files, f) != 0) {
				free(filearr);
				unmap_file(stream, stream_len);
				Trace(TR_OPRMSG, "lst append failed");
				goto err;
			}

			if (size > 0 && files->length >= size) {
				break;
			}
		}

ext:
		if (filearr) {
			free(filearr);
			filearr = NULL;
		}

		unmap_file(stream, stream_len);

		if (size > 0 && files->length >= size) {
			break;
		}
	}


	unmap_file(state, state_len);

	*staging_file_infos = files;

	Trace(TR_OPRMSG, "returned staging file list of size %d",
	    files->length);
	Trace(TR_MISC, "got staging files");
	return (0);

err:
	if (state) {
		unmap_file(state, state_len);
	}
	if (request) {
		unmap_file(request, req_len);
	}
	if (files) {
		lst_free_deep(files);
	}
	if (f) {
		free(f);
	}

	Trace(TR_ERR, "get staging files failed: %s", samerrmsg);
	return (-1);
}


int
get_all_staging_files_in_stream(
ctx_t *ctx,						/* IN - not used */
stager_stream_t *in_stream,		/* IN - stager stream */
st_sort_key_t sort_key,			/* IN - sort key */
boolean_t ascending,			/* IN - ascending order */
sqm_lst_t **staging_file_infos)	/* OUT - staging_file_info_t list */
{
	return (get_staging_files_in_stream(ctx, in_stream, 0, -1,
	    sort_key, ascending, staging_file_infos));
}


int
get_staging_files_in_stream(
ctx_t *ctx,					/* ARGSUSED */
stager_stream_t *in_stream,	/* IN - stager stream */
int start,	/* IN - starting index in the list */
int size,	/* IN - num of entries to return, -1: all remaining */
st_sort_key_t sort_key,			/* IN - sort key */
boolean_t ascending,			/* IN - ascending order */
sqm_lst_t **staging_file_infos)	/* OUT - staging_file_info_t list */
{
	StagerStateInfo_t *state = NULL;
	FileInfo_t *request = NULL;
	StreamInfo_t *stream = NULL;
	size_t state_len;
	size_t req_len;
	size_t stream_len;
	upath_t fullpath;
	FileInfo_t *filearr = NULL;
	FileInfo_t *file;
	FileInfo_t **file_list = NULL;
	int (*comp) (const void *, const void *);
	sqm_lst_t *files = NULL;
	staging_file_info_t *f = NULL;
	struct passwd *pw = NULL;
	int num, total = 0;
	int i, j;

	Trace(TR_MISC, "getting staging files in stream");

	if (ISNULL(in_stream, staging_file_infos)) {
		Trace(TR_OPRMSG, "null argument found");
		goto err;
	}

	*staging_file_infos = NULL;

	Trace(TR_OPRMSG, "start: %d, size: %d", start, size);

	files = lst_create();
	if (files == NULL) {
		Trace(TR_OPRMSG, "lst create failed");
		goto err;
	}

	if (size == 0) {
		*staging_file_infos = files;
		Trace(TR_OPRMSG, "returned empty list as requested");
		Trace(TR_MISC, "got 0 staging files");
		return (0);
	}

	Trace(TR_OPRMSG, "mapping in stager state file");

	sprintf(fullpath, "%s/%s/%s", VAR_DIR, STAGER_DIRNAME,
	    STAGER_STATE_FILENAME);
	state = (StagerStateInfo_t *)map_file(fullpath, O_RDONLY, &state_len);

	if (state == NULL) {
		samerrno = SE_STAGERD_NOT_RUN;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno));
		Trace(TR_OPRMSG, "unable to map in stager state file");
		goto err;
	}

	Trace(TR_OPRMSG, "mapping in stage requests");

	request = (FileInfo_t *)
	    map_file(state->stageReqsFile, O_RDWR, &req_len);
	if (request == NULL) {
		unmap_file(state, state_len);
		*staging_file_infos = files;
		Trace(TR_OPRMSG, "no stage requests");
		Trace(TR_MISC, "got 0 staging files");
		return (0);
	}

	Trace(TR_OPRMSG, "creating staging files list");

	for (i = 0; i < STAGER_DISPLAY_STREAMS; i++) {
		if (state->streams[i].vsn[0] == '\0') {
			break;
		}

		if (in_stream->seqnum != state->streams[i].seqnum ||
		    strcmp(in_stream->vsn, state->streams[i].vsn) != 0) {
			continue;
		}

		sprintf(fullpath, "%s/%s.%d", state->streamsDir,
		    state->streams[i].vsn, state->streams[i].seqnum);

		stream = (StreamInfo_t *)
		    map_file(fullpath, O_RDWR, &stream_len);
		if (stream == NULL) {
			break;
		}

		num = get_stream_files(stream, request, &filearr);
		if (num == -1) {
			unmap_file(stream, stream_len);
			Trace(TR_OPRMSG, "get stream files failed");
			goto err;
		}

		if (start >= num) {
			goto ext;
		}

		// prepare to sort FileInfo_t list
		switch (sort_key) {
			case ST_SORT_BY_FILENAME:
				if (ascending) {
					comp = st_compare_filename_ascending;
				} else {
					comp = st_compare_filename_descending;
				}

				break;

			case ST_SORT_BY_UID:
				if (ascending) {
					comp = st_compare_uid_ascending;
				} else {
					comp = st_compare_uid_descending;
				}

				break;

			default:
				comp = NULL;
				break;
		}

		if (comp != NULL) {

			file_list = (FileInfo_t **)
			    mallocer(num * sizeof (FileInfo_t *));

			if (file_list == NULL) {
				unmap_file(stream, stream_len);
				Trace(TR_OPRMSG, "out of memory");
				goto err;
			}

			for (j = 0; j < num; j++) {
				file_list[j] = &filearr[j];
			}


			qsort(file_list, num, sizeof (FileInfo_t *), comp);
		}

		for (j = 0; j < num; j++) {

			total++;
			if (start >= total) {
				continue;
			}

			if (comp != NULL) {
				file = file_list[j];
			} else {
				file = &filearr[j];
			}

			f = (staging_file_info_t *)
			    mallocer(sizeof (staging_file_info_t));

			if (f == NULL) {
				unmap_file(stream, stream_len);
				Trace(TR_OPRMSG, "out of memory");
				goto err;
			}

			f->id = file->id;
			f->fseq = file->fseq;
			f->copy = file->copy;
			f->len = file->len;
			f->position = file->ar[f->copy].section.position;
			f->offset = file->ar[f->copy].section.offset;
			snprintf(f->media, sizeof (f->media),
			    sam_mediatoa(file->ar[f->copy].media));
			strcpy(f->vsn, file->ar[f->copy].section.vsn);
			snprintf(f->filename, sizeof (f->filename),
			    id_to_path(GetFsMountName(f->fseq), f->id));
			f->pid = file->pid;

			pw = getpwuid(getuid());

			if (pw != NULL) {
				snprintf(f->user, sizeof (f->user),
				    pw->pw_name);
			} else {
				snprintf(f->user, sizeof (f->user),
				    "Unknown user");
			}

			if (lst_append(files, f) != 0) {
				unmap_file(stream, stream_len);
				Trace(TR_OPRMSG, "lst append failed");
				goto err;
			}

			if (size > 0 && files->length >= size) {
				break;
			}
		}

ext:
		if (filearr) {
			free(filearr);
		}

		if (file_list) {
			free(file_list);
		}

		unmap_file(stream, stream_len);
		break;
	}


	unmap_file(state, state_len);

	*staging_file_infos = files;

	Trace(TR_OPRMSG, "returned staging file list of size %d",
	    files->length);
	Trace(TR_MISC, "got staging files in stream");
	return (0);

err:
	if (state) {
		unmap_file(state, state_len);
	}
	if (request) {
		unmap_file(request, req_len);
	}
	if (files) {
		lst_free_deep(files);
	}
	if (f) {
		free(f);
	}
	if (filearr) {
		free(filearr);
	}

	if (file_list) {
		free(file_list);
	}

	Trace(TR_ERR, "get staging files in stream failed: %s", samerrmsg);
	return (-1);
}


/*
 * find information about the given file in staging queue
 */
int
find_staging_file(
ctx_t *ctx,		/* ARGSUSED */
upath_t fname,	/* IN - file name (absolute path) */
vsn_t vsn,		/* IN - vsn to search for; NULL: search all */
staging_file_info_t **finfo)	/* OUT - staging_file_info_t list */
{
	StagerStateInfo_t *state = NULL;
	FileInfo_t *request = NULL;
	StreamInfo_t *stream = NULL;
	size_t state_len;
	size_t req_len;
	size_t stream_len;
	upath_t fullpath;
	upath_t tmp_fname;
	FileInfo_t *filearr = NULL;
	FileInfo_t *file;
	staging_file_info_t *f = NULL;
	struct passwd *pw = NULL;
	int num;
	int i, j;

	Trace(TR_MISC, "looking for staging file");

	if (ISNULL(fname, finfo)) {
		Trace(TR_OPRMSG, "null argument found");
		goto err;
	}

	*finfo = NULL;

	Trace(TR_OPRMSG, "fname: %s, vsn: %s", fname,
	    vsn == NULL ? "ALL" : vsn);

	if (fname[0] != '/') { /* expecting absolute path */
		samerrno = SE_INVALID_STAGING_FILE_NAME;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), fname);
		Trace(TR_OPRMSG, "invalid staging file name");
		goto err;
	}

	Trace(TR_OPRMSG, "mapping in stager state file");

	sprintf(fullpath, "%s/%s/%s", VAR_DIR, STAGER_DIRNAME,
	    STAGER_STATE_FILENAME);
	state = (StagerStateInfo_t *)map_file(fullpath, O_RDONLY, &state_len);

	if (state == NULL) {
		samerrno = SE_STAGERD_NOT_RUN;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno));
		Trace(TR_OPRMSG, "unable to map in stager state file");
		goto err;
	}

	Trace(TR_OPRMSG, "mapping in stage requests");

	request = (FileInfo_t *)
	    map_file(state->stageReqsFile, O_RDWR, &req_len);
	if (request == NULL) {
		unmap_file(state, state_len);
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), fname);
		Trace(TR_OPRMSG, "no stage requests");
		goto err;
	}

	Trace(TR_OPRMSG, "searching staging files...");

	for (i = 0; i < STAGER_DISPLAY_STREAMS; i++) {
		if (state->streams[i].vsn[0] == '\0') {
			break;
		}

		if (vsn != NULL && *vsn != '\0' &&
		    strcmp(vsn, state->streams[i].vsn) != 0) {
			continue;
		}

		sprintf(fullpath, "%s/%s.%d", state->streamsDir,
		    state->streams[i].vsn, state->streams[i].seqnum);

		stream = (StreamInfo_t *)
		    map_file(fullpath, O_RDWR, &stream_len);
		if (stream == NULL) {
			continue;
		}

		num = get_stream_files(stream, request, &filearr);
		if (num == -1) {
			unmap_file(stream, stream_len);
			Trace(TR_OPRMSG, "get stream files failed");
			goto err;
		}

		for (j = 0; j < num; j++) {

			file = &filearr[j];

			snprintf(tmp_fname, sizeof (tmp_fname),
			    id_to_path(GetFsMountName(file->fseq),
			    file->id));

			if (strcmp(tmp_fname, fname) != 0) {
				continue;
			}

			f = (staging_file_info_t *)
			    mallocer(sizeof (staging_file_info_t));

			if (f == NULL) {
				free(filearr);
				unmap_file(stream, stream_len);
				Trace(TR_OPRMSG, "out of memory");
				goto err;
			}

			f->id = file->id;
			f->fseq = file->fseq;
			f->copy = file->copy;
			f->len = file->len;
			f->position = file->ar[f->copy].section.position;
			f->offset = file->ar[f->copy].section.offset;
			snprintf(f->media, sizeof (f->media),
			    sam_mediatoa(file->ar[f->copy].media));
			strcpy(f->vsn, file->ar[f->copy].section.vsn);
			snprintf(f->filename, sizeof (f->filename), tmp_fname);
			f->pid = file->pid;

			pw = getpwuid(getuid());

			if (pw != NULL) {
				snprintf(f->user, sizeof (f->user),
				    pw->pw_name);
			} else {
				snprintf(f->user, sizeof (f->user),
				    "Unknown user");
			}

			break;
		}

		if (filearr) {
			free(filearr);
			filearr = NULL;
		}

		unmap_file(stream, stream_len);

		if (f) {
			break;
		}
	}

	if (f == NULL) {
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), fname);
		Trace(TR_OPRMSG, "staging file %s not found", fname);
		goto err;
	}

	unmap_file(state, state_len);

	*finfo = f;

	Trace(TR_MISC, "found staging file");
	return (0);

err:
	if (state) {
		unmap_file(state, state_len);
	}
	if (request) {
		unmap_file(request, req_len);
	}

	Trace(TR_ERR, "find staging file failed: %s", samerrmsg);
	return (-1);
}


/*
 * cancel pending requests for the named files.
 * recursive only applies to directories.
 */
int
cancel_stage(
ctx_t *ctx,					/* ARGSUSED */
const sqm_lst_t *file_or_dirs,	/* IN - list of files or dirs */
const boolean_t recursive)	/* IN - recursive for dirs */
{
	char cwd[MAXPATHLEN + 4]; /* current full path name */
	char *file;
	node_t *node;

	Trace(TR_MISC, "canceling stage");

	if (ISNULL(file_or_dirs)) {
		Trace(TR_OPRMSG, "null argument found");
		goto err;
	}

	if (recursive && getcwd(cwd, sizeof (cwd)) == NULL) {
		samerrno = SE_GETCWD_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		Trace(TR_OPRMSG, "get cwd failed");
		goto err;
	}

	node = file_or_dirs->head;

	while (node) {
		file = (char *)node->data;

		if (sam_lstat(file, &sb, sizeof (sb)) < 0) {
			samerrno = SE_STAGE_STAT_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), file, "");
			strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
			Trace(TR_OPRMSG, "sam lstat failed");
			goto err;
		}

		if (!SS_ISSAMFS(sb.attr)) {
			samerrno = SE_STAGE_NOT_SAM_FILE;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), file);
			Trace(TR_OPRMSG, "not samfs file");
			goto err;
		}

		if (sam_cancelstage(file) < 0) {
			samerrno = SE_STAGE_CANCEL_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), file);
			Trace(TR_OPRMSG, "sam cancel stage failed");
			goto err;
		}

		if (S_ISDIR(sb.st_mode) && recursive) {
			base_name = fullpath;
			dofile_no_options = sam_cancelstage;
			which_dofile = 1;
			dofile_error = SE_STAGE_CANCEL_FAILED;
			chk_file = NULL;

			if (dodir(file) < 0) {
				Trace(TR_OPRMSG, "do dir failed");
				goto err;
			}
			/* set back the cwd to where we entered */
			if (chdir(cwd) < 0) {
				samerrno = SE_CHDIR_FAILED;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno), cwd, "");
				strlcat(samerrmsg, strerror(errno),
				    MAX_MSG_LEN);
				Trace(TR_OPRMSG, "ch dir failed");
				goto err;
			}
		}

		node = node->next;
	}

	if (dir_names) {
		free(dir_names);
		dir_names = NULL;
	}

	Trace(TR_MISC, "stage canceled");
	return (0);

err:
	Trace(TR_ERR, "cancel stage failed: %s", samerrmsg);
	return (-1);
}


/*
 * cancel stage request.
 */
int
clear_stage_request(
ctx_t *ctx,		/* ARGSUSED */
mtype_t media,	/* IN - media type */
vsn_t vsn)		/* IN - vsn */
{
	char target[40];
	char msg[80];
	struct VolId vid;
	msg[0] = '\0';

	Trace(TR_MISC, "clearing stage requests");

	if (ISNULL(media, vsn)) {
		Trace(TR_OPRMSG, "null argument found");
		goto err;
	}

	sprintf(target, "%s.%s", media, vsn);

	if (StrToVolId(target, &vid) != 0) {
		samerrno = SE_STAGE_INVALID_VOLUME;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), target);
		Trace(TR_OPRMSG, "invalid volume specified");
		goto err;
	}

	(void) StagerControl("stclear", target, msg, sizeof (msg));

	if (*msg != '\0') {
		set_samdaemon_err(errno, SE_STAGER_CLEAR_FAILED, msg);
		goto err;
	}

	Trace(TR_MISC, "clear stage request done");
	return (0);

err:
	Trace(TR_ERR, "clear stage request failed: %s", samerrmsg);
	return (-1);
}


/*
 * Idle staging
 */
int
st_idle(
ctx_t *ctx /* ARGSUSED */)
{
	char msg[80];
	msg[0] = '\0';

	Trace(TR_MISC, "idling staging");

	(void) StagerControl("exec", "idle", msg, sizeof (msg));

	if (*msg != '\0') {
		set_samdaemon_err(errno, SE_STAGER_IDLE_FAILED, msg);
		Trace(TR_OPRMSG, "stager control cmd idle failed");
		goto err;
	}

	Trace(TR_MISC, "staging idled");
	return (0);

err:
	Trace(TR_ERR, "idle staging failed: %s", samerrmsg);
	return (-1);
}


/*
 * Start staging
 */
int
st_run(
ctx_t *ctx /* ARGSUSED */)
{
	char msg[80];
	msg[0] = '\0';

	Trace(TR_MISC, "starting staging");

	(void) StagerControl("exec", "run", msg, sizeof (msg));

	if (*msg != '\0') {
		set_samdaemon_err(errno, SE_STAGER_RUN_FAILED, msg);
		goto err;
	}

	Trace(TR_MISC, "staging started");
	return (0);

err:
	Trace(TR_ERR, "start staging failed: %s", samerrmsg);
	return (-1);
}


/*
 * **************************
 *  private helper functions
 * **************************
 */

/*
 *	Establish mapping between address space and file.
 */
static void *
map_file(
char *file_name,	/* IN  - file name */
int mode,			/* IN  - mode */
size_t *len)		/* OUT - length */
{
	int prot;
	int fd;
	int err;
	struct stat st;
	void *mp = NULL;

	prot = (O_RDONLY == mode) ? PROT_READ : PROT_READ | PROT_WRITE;
	fd = open(file_name, mode);
	if (fd == -1) {
		if (len != NULL) {
			*len = 0;
		}
		samerrno = SE_STAGE_OPEN_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), file_name, "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		Trace(TR_OPRMSG, "open file %s failed", file_name);
		return (NULL);
	}

	err = fstat(fd, &st);
	if (err != 0) {
		samerrno = SE_STAGE_FSTAT_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), file_name, "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		Trace(TR_OPRMSG, "fstat file %s failed", file_name);
		return (NULL);
	}

	mp = mmap(NULL, st.st_size, prot, MAP_SHARED, fd, 0);
	if (mp == MAP_FAILED) {
		samerrno = SE_STAGE_MMAP_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), file_name, "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		Trace(TR_OPRMSG, "mmap file %s failed", file_name);
		return (NULL);
	}

	(void) close(fd);

	if (len != NULL) {
		*len = st.st_size;
	}

	return (mp);
}


/*
 * Unmap memory mapped file.
 */
static void
unmap_file(
void *mp,		/* IN - pointer to mapped file */
size_t len)		/* IN - length */
{
	(void) munmap(mp, len);
}


/*
 * Get all files in a stage stream.
 */
static int
get_stream_files(
StreamInfo_t *stream,	/* IN  - stream info */
FileInfo_t *stageReqs,	/* IN  - request info */
FileInfo_t **arrFiles	/* OUT - array of files */
)
{
	int i = 0;
	int id;
	FileInfo_t *entry;
	int numFiles = 0;
	FileInfo_t *files = NULL;

	if (stream == NULL || stageReqs == NULL) {
		return (0);
	}

	(void) pthread_mutex_lock(&(stream->mutex));
	numFiles = stream->count;
	(void) pthread_mutex_unlock(&(stream->mutex));

	if (numFiles > 0) {
		files = (FileInfo_t *)mallocer(numFiles * sizeof (FileInfo_t));
		if (files == NULL) {
			Trace(TR_OPRMSG, "out of memory");
			return (-1);
		}

		(void) memset(files, 0, numFiles * sizeof (FileInfo_t));

		id = stream->first;
		while (id >= 0 && i < numFiles) {
			entry = &stageReqs[id];

			(void) pthread_mutex_lock(&(entry->mutex));
			(void) memcpy(&files[i], entry, sizeof (FileInfo_t));
			id = entry->next;
			(void) pthread_mutex_unlock(&(entry->mutex));
			i++;
		}
	}

	*arrFiles = files;

	return (numFiles);
}


/*
 * For each active stream, map file currently being staged
 */
static int
map_active_file(
stager_info_t *st_info	/* IN/OUT - stager info */
)
{
	node_t *node;
	stager_stream_t *st_stream;
	sqm_lst_t *tmp_lst;

	if ((tmp_lst = lst_create()) == NULL) {
		Trace(TR_OPRMSG, "lst create failed");
		goto err;
	}

	node = st_info->active_stager_info->head;
	while (node != NULL) {
		if (lst_append(tmp_lst, node->data) != 0) {
			Trace(TR_OPRMSG, "lst append failed");
			goto err;
		}
		node = node->next;
	}

	node = st_info->stager_streams->head;
	while (node != NULL) {

		st_stream = (stager_stream_t *)node->data;

		/* If stream is active, then get current file details */
		if (st_stream->active &&
		    (get_current_file(tmp_lst, st_stream) != 0)) {
			Trace(TR_OPRMSG, "get current file failed");
			goto err;
		}

		node = node->next;
	}

	lst_free(tmp_lst);
	Trace(TR_OPRMSG, "map active file succeeded");
	return (0);

err:

	lst_free(tmp_lst);
	Trace(TR_OPRMSG, "map active file failed: %s", samerrmsg);
	return (-1);
}


/*
 * Get the info about the file currently being staged
 * and the state of the stage request (loading VSN, copying, waiting etc.)
 * The stream being passed is active currently
 */
static int
get_current_file(
sqm_lst_t *active_stagers,	/* IN  - active stager info */
stager_stream_t *stream	/* IN/OUT - active stream */
)
{
	active_stager_info_t *st_active;
	node_t *node = active_stagers->head;
	stream->current_file = NULL;
	stream->state_flags = 0;

	while (node != NULL) {

		st_active = (active_stager_info_t *)node->data;

		/*
		 * the stream is matched with the active stream
		 * using the media type and the vsn
		 */
		if (strcmp(st_active->media, stream->media) != 0 ||
		    strcmp(st_active->vsn, stream->vsn) != 0) {

			node = node->next;
			continue;
		}
		stream->state_flags = st_active->flags;

		if (st_active->flags == STAGER_STATE_COPY ||
		    st_active->flags == STAGER_STATE_POSITIONING) {

			// found the match active_stager_info
			stream->current_file = (StagerStateDetail_t	*)
			    mallocer(sizeof (StagerStateDetail_t));
			if (stream->current_file == NULL) {
				Trace(TR_OPRMSG, "out of memory");
				return (-1);
			}

			memcpy(stream->current_file, &st_active->detail,
			    sizeof (StagerStateDetail_t));

		}

		lst_remove(active_stagers, node);
		break;
	}

	Trace(TR_OPRMSG, "get current file succeeded");
	return (0);
}


/*
 *	Descend through the directories, starting at "name".
 *	Call dofile() for each directory entry.
 *	Save each directory name, and process at the end.
 */
static int
dodir(
char *name)		/* IN - dir name */
{
	struct dirent *dirp;
	DIR *dp;
	size_t dn_mark, dn_next;
	char *prev_base;
	strcpy(base_name, name);

	/*
	 * Change to the new directory.
	 * Extend the full path.
	 */
	if ((dp = opendir(name)) == NULL) {
		samerrno = SE_OPENDIR_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), fullpath, "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		Trace(TR_OPRMSG, "open dir %s failed", name);
		return (-1);
	}

	if (chdir(name)  < 0 && chdir(fullpath) < 0) {
		samerrno = SE_CHDIR_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), fullpath, "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		(void) closedir(dp);
		Trace(TR_OPRMSG, "ch dir %s failed", name);
		return (-1);
	}

	prev_base = base_name;
	base_name += strlen(name);
	if (base_name[-1] != '/') {
		*base_name++ = '/';
	}
	*base_name = '\0';

	/*
	 * Mark the directory name stack.x
	 */
	dn_mark = dn_next = dn_size;

	while ((dirp = readdir(dp)) != NULL) {
		/* ignore dot and dot-dot */
		if (strcmp(dirp->d_name, ".") == 0 ||
		    strcmp(dirp->d_name, "..") == 0) {
			continue;
		}

		strcpy(base_name, dirp->d_name);
		if (sam_lstat(dirp->d_name, &sb, sizeof (sb)) < 0) {
			samerrno = SE_STAGE_STAT_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), fullpath);

			Trace(TR_OPRMSG, "sam lstat %s failed", dirp->d_name);
			continue;
		}

		if (chk_file == NULL || !chk_file()) {
			if (((which_dofile == 0) ?
			    dofile(dirp->d_name, &optsbuf[0]) :
			    dofile_no_options(dirp->d_name)) < 0) {

				samerrno = dofile_error;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno), fullpath);

				Trace(TR_OPRMSG, "stage of %s failed %d",
				    dirp->d_name, dofile_error);
				continue;
			}
		}
		if (S_ISDIR(sb.st_mode)) {
			/*
			 * we cannot recover from an adddir failure
			 * so return an error.
			 */
			if (adddir(dirp->d_name) < 0) {
				(void) closedir(dp);
				Trace(TR_OPRMSG, "add dir %s failed",
				    dirp->d_name);
				return (-1);
			}
		}
	}

	if (closedir(dp) < 0) {
		samerrno = SE_CLOSEDIR_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), fullpath, "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		Trace(TR_OPRMSG, "close dir %s failed", name);
		return (-1);
	}

	/*
	 * Process all the directories found.
	 */
	while (dn_next < dn_size) {
		char *dname;

		dname = &dir_names[dn_next];
		dn_next += strlen(dname) + 1;
		if (dodir(dname) < 0) {
			Trace(TR_OPRMSG, "do dir %s failed", dname);
			return (-1);
		}
	}

	dn_size = dn_mark;

	base_name = prev_base;
	*base_name = '\0';

	if (chdir("..") < 0 && chdir(fullpath) < 0) {
		samerrno = SE_CHDIR_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(samerrno), fullpath, "");
		strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
		Trace(TR_OPRMSG, "ch dir failed");
		return (-1);
	}

	Trace(TR_OPRMSG, "do dir [%s] succeeded", name);
	return (0);
}


/*
 *	Add a directory name to table.
 */
static int
adddir(
char *name)		/* IN - dir name to be added */
{
	static int dn_limit = 0;
	size_t l;

	l = strlen(name) + 1;
	if (dn_size + l >= dn_limit) {
		/*
		 * No room in table for entry.
		 * realloc() space for enlarged table.
		 */
		dn_limit += 5000;

		dir_names = (char *)realloc(dir_names, dn_limit);
		if (dir_names == NULL) {
			samerrno = SE_REALLOC_FAILED;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(samerrno), "");
			strlcat(samerrmsg, strerror(errno), MAX_MSG_LEN);
			Trace(TR_OPRMSG, "realloc failed");
			return (-1);
		}
	}

	memcpy(&dir_names[dn_size], name, l);
	dn_size += l;

	Trace(TR_OPRMSG, "add dir [%s] succeeded", name);
	return (0);
}


/*
 *	Compare two FileInfo_t structures by file name in ascending order
 */
static int
st_compare_filename_ascending(const void *p1, const void *p2)
{
	FileInfo_t *f1 = *(FileInfo_t **)p1;
	FileInfo_t *f2 = *(FileInfo_t **)p2;

	return (strcmp(id_to_path(GetFsMountName(f1->fseq), f1->id),
	    id_to_path(GetFsMountName(f2->fseq), f2->id)));
}


/*
 *	Compare two FileInfo_t structures by file name in descending order
 */
static int
st_compare_filename_descending(const void *p1, const void *p2)
{
	FileInfo_t *f1 = *(FileInfo_t **)p1;
	FileInfo_t *f2 = *(FileInfo_t **)p2;

	return (strcmp(id_to_path(GetFsMountName(f2->fseq), f2->id),
	    id_to_path(GetFsMountName(f1->fseq), f1->id)));
}


/*
 *	Compare two FileInfo_t structures by uid in ascending order
 */
static int
st_compare_uid_ascending(const void *p1, const void *p2)
{
	FileInfo_t *f1 = *(FileInfo_t **)p1;
	FileInfo_t *f2 = *(FileInfo_t **)p2;

	return (f1->user - f2->user);
}

/*
 *	Compare two FileInfo_t structures by uid in descending order
 */
static int
st_compare_uid_descending(const void *p1, const void *p2)
{
	FileInfo_t *f1 = *(FileInfo_t **)p1;
	FileInfo_t *f2 = *(FileInfo_t **)p2;

	return (f2->user - f1->user);
}

#define	STAGE_FILES_CMD BIN_DIR"/stage"

/*
 * API for staging files and setting stager attributes on files and directories
 * in 4.6 and beyond.
 *
 * Flags for setting options
 * ST_OPT_STAGE_NEVER
 * ST_OPT_ASSOCIATIVE_STAGE
 * ST_OPT_STAGE_DEFAULTS
 * ST_OPT_PARTIAL
 * ST_OPT_RECURSIVE
 *
 * Flags to be used in the options mask to indicate copies
 * ST_OPT_COPY_1
 * ST_OPT_COPY_2
 * ST_OPT_COPY_3
 * ST_OPT_COPY_4
 *
 * In order to signal that a file specified as stage -a be set to stage -n
 * the ST_OPT_STAGE_DEFAULTS flag must be set also.
 *
 * Sets job_id if the stage command does not return within a short
 * period of time
 * returns 0 if stage succeeded or is still pending
 * returns -1 if /usr/bin/stage did not succeed
 */
int
stage_files(
ctx_t *c,
sqm_lst_t *files,
int32_t options,
char **job_id) /* ARGSUSED */ {


	char		buf[32];
	char		**command;
	node_t		*n;
	argbuf_t 	*arg;
	size_t		len = MAXPATHLEN * 2;
	boolean_t	found_one = B_FALSE;
	pid_t		pid;
	int		ret;
	int		status;
	FILE		*out;
	FILE		*err;
	exec_cleanup_info_t *cl;
	int arg_cnt;
	int cur_arg = 0;

	if (ISNULL(files, job_id)) {
		Trace(TR_ERR, "stage files failed:%s", samerrmsg);
		return (-1);
	}

	if (stager_ready() == -1) {
		Trace(TR_ERR, "stage files failed:%s", samerrmsg);
		return (-1);
	}

	if (options & ST_OPT_ASSOCIATIVE &&
	    options & ST_OPT_NEVER) {
		setsamerr(SE_MUTUALLY_EXCLUSIVE_STG_OPTS);
		return (-1);
	}


	/*
	 * Determine how many args to the command and create the command
	 * array. Note that command is malloced because the number of files
	 * is not known prior to execution. The arguments themselves need
	 * not be malloced because the child process will get a copy.
	 * Include space in the command array for:
	 * - the command
	 * - all possible options
	 * - an entry for each file in the list.
	 * - an entry for the NULL
	 */
	arg_cnt = 1 + 5 + files->length + 1;

	command = (char **)calloc(arg_cnt, sizeof (char *));
	if (command == NULL) {
		setsamerr(SE_NO_MEM);
		Trace(TR_ERR, "stage files failed:%s", samerrmsg);
		return (-1);
	}
	command[cur_arg++] = STAGE_FILES_CMD;
	if (options & ST_OPT_COPY_1) {
		command[cur_arg++] = "-c1";
	} else if (options & ST_OPT_COPY_2) {
		command[cur_arg++] = "-c2";
	} else if (options & ST_OPT_COPY_3) {
		command[cur_arg++] = "-c3";
	} else if (options & ST_OPT_COPY_4) {
		command[cur_arg++] = "-c4";
	}
	if (options & ST_OPT_RESET_DEFAULTS) {
		command[cur_arg++] = "-d";
	}
	if (options & ST_OPT_NEVER) {
		command[cur_arg++] = "-n";
	} else if (options & ST_OPT_ASSOCIATIVE) {
		command[cur_arg++] = "-a";
	}
	if (options & ST_OPT_PARTIAL) {
		command[cur_arg++] = "-p";
	}
	if (options & ST_OPT_RECURSIVE) {
		command[cur_arg++] = "-r";
	}

	/* make the argument buffer for the activity */
	arg = (argbuf_t *)mallocer(sizeof (stagebuf_t));
	if (arg == NULL) {
		free(command);
		Trace(TR_ERR, "stage files failed:%s", samerrmsg);
		return (-1);
	}

	arg->st.filepaths = lst_create();
	if (arg->st.filepaths == NULL) {
		free(command);
		Trace(TR_ERR, "stage files failed:%s", samerrmsg);
		return (-1);
	}
	arg->st.options = options;

	/* add the file names to the cmd string and the argument buffer */
	for (n = files->head; n != NULL; n = n->next) {
		if (n->data != NULL) {
			char *cur_file;
			command[cur_arg++] = (char *)n->data;

			found_one = B_TRUE;
			cur_file = copystr(n->data);
			if (cur_file == NULL) {
				free(command);
				free_argbuf(SAMA_STAGEFILES, arg);
				Trace(TR_ERR, "stage files failed:%s",
				    samerrmsg);
				return (-1);
			}
			if (lst_append(arg->st.filepaths, cur_file) != 0) {
				free(command);
				free_argbuf(SAMA_STAGEFILES, arg);
				Trace(TR_ERR, "stage files failed:%s",
				    samerrmsg);
				return (-1);
			}
		}
	}

	/*
	 * Check that at least one file was found.
	 */
	if (!found_one) {
		free(command);
		free_argbuf(SAMA_STAGEFILES, arg);
		Trace(TR_ERR, "stage files failed:%s", samerrmsg);
		return (-1);
	}

	/* create the activity */
	ret = start_activity(display_stage_activity, kill_fork,
	    SAMA_STAGEFILES, arg, job_id);
	if (ret != 0) {
		free(command);
		free_argbuf(SAMA_STAGEFILES, arg);
		Trace(TR_ERR, "stage files failed:%s", samerrmsg);
		return (-1);
	}

	/*
	 * create the cleanup struct prior to the exec because it is
	 * easier to cleanup here  if the malloc fails.
	 */
	cl = (exec_cleanup_info_t *)mallocer(sizeof (exec_cleanup_info_t));
	if (cl == NULL) {
		free(command);
		lst_free_deep(arg->st.filepaths);
		end_this_activity(*job_id);
		Trace(TR_ERR, "stage files failed, error:%d %s",
			samerrno, samerrmsg);
		return (-1);
	}

	/* exec the process */
	pid = exec_mgmt_cmd(&out, &err, command);
	if (pid < 0) {
		free(command);
		lst_free_deep(arg->st.filepaths);
		end_this_activity(*job_id);
		Trace(TR_ERR, "stage files failed:%s", samerrmsg);
		return (-1);
	}
	free(command);
	set_pid_or_tid(*job_id, pid, 0);


	/* setup struct for call to cleanup */
	strlcpy(cl->func, STAGE_FILES_CMD, sizeof (cl->func));
	cl->pid = pid;
	strlcpy(cl->job_id, *job_id, MAXNAMELEN);
	cl->streams[0] = out;
	cl->streams[1] = err;


	/* possibly return the results or async notification */
	ret = bounded_activity_wait(&status, 10, *job_id, pid, cl,
	    cleanup_after_exec_get_output);

	if (ret == -1) {
		if (options & ST_OPT_RECURSIVE) {
			setsamerr(SE_STAGE_PARTIAL_FAILURE);
		} else {
			setsamerr(SE_STAGE_FAILED);
		}
		free(*job_id);
		*job_id = NULL;
		Trace(TR_ERR, "stage files failed:%s", samerrmsg);
		return (-1);
	} else if (ret == 0) {
		/*
		 * job_id was set by start_activity. Clear it now
		 * so that the caller knows the request has been submitted.
		 */
		free(*job_id);
		*job_id = NULL;
		Trace(TR_MISC, "stage files completed");
	}

	Trace(TR_MISC, "leaving stage files");
	return (0);


}

/*
 * Stage_files or set staging attributes.
 */
int
stage_files_pre46(
ctx_t *c,
sqm_lst_t *copies,
sqm_lst_t *filepaths,
sqm_lst_t *opt_list)
{

	int32_t options = 0;
	char *job_id;
	int ret_val;

	Trace(TR_MISC, "staging files pre46 called");
	if (copies != NULL || copies->length != 0) {
		int stage_from = *(int *)copies->head->data;
		if (stage_from == 1) {
			options |= ST_OPT_COPY_1;
		} else if (stage_from == 2) {
			options |= ST_OPT_COPY_2;
		} else if (stage_from == 3) {
			options |= ST_OPT_COPY_3;
		} else if (stage_from == 4) {
			options |= ST_OPT_COPY_4;
		}
	}

	if (opt_list != NULL || opt_list->length != 0) {
		options |= *(int *)opt_list->head->data;
		Trace(TR_MISC, "stage files options = %d", options);
	}

	ret_val = stage_files(c, filepaths, options, &job_id);
	if (ret_val != -1) {
		free(job_id);
	}
	return (ret_val);
}


/*
 * This function returns 0 if the stager is ready to accept stage requests.
 * If it is not ready a -1 is returned and samerrno and samerrmsg are set
 * to a message that explains why the stage would not succeed.
 */
static int
stager_ready() {

	StagerStateInfo_t *state;
	size_t state_len;

	/* Check if the stager is running. If not the command will hang */
	if (map_stager_state_file(&state, &state_len) != 0) {
		samerrno = SE_STAGE_DAEMON_NOT_RUNNING;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_STAGE_DAEMON_NOT_RUNNING));
		Trace(TR_ERR, "staging failed:%s", samerrmsg);
		return (-1);
	}

	/*
	 * Check if the stager is waiting for strun if so the command will
	 * hang. So return an error.
	 */
	if (strstr(state->errmsg, "strun") != NULL) {
		samerrno = SE_STAGE_DAEMON_WAITING;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_STAGE_DAEMON_WAITING));
		Trace(TR_ERR, "staging failed:%s", samerrmsg);
		return (-1);
	}

	unmap_file(state, state_len);
	return (0);
}


static int
expand_stage_options(int32_t options, char *buf, int len) {
	*buf = '\0';

	/* Add the copy */
	if (options & ST_OPT_COPY_1) {
		strlcat(buf, "-c ", len);
		strlcat(buf, "1 ", len);
	} else if (options & ST_OPT_COPY_2) {
		strlcat(buf, "-c ", len);
		strlcat(buf, "2 ", len);
	} else if (options & ST_OPT_COPY_3) {
		strlcat(buf, "-c ", len);
		strlcat(buf, "3 ", len);
	} else if (options & ST_OPT_COPY_4) {
		strlcat(buf, "-c ", len);
		strlcat(buf, "4 ", len);
	}


	if (options & ST_OPT_RESET_DEFAULTS) {
		strlcat(buf, "-d ", len);
	}

	if (options & ST_OPT_NEVER) {
		strlcat(buf, "-n ", len);
	}

	if (options & ST_OPT_ASSOCIATIVE) {
		strlcat(buf, "-a ", len);
	}

	if (options & ST_OPT_PARTIAL) {
		strlcat(buf, "-p ", len);
	}

	if (options & ST_OPT_RECURSIVE) {
		strlcat(buf, "-r ", len);
	}

	return (0);
}


int
display_stage_activity(samrthread_t *ptr, char **result) {
	char buf[2 * MAXPATHLEN];
	char details[MAXPATHLEN];

	snprintf(details, MAXPATHLEN, "stage ");
	expand_stage_options(ptr->args->st.options,
		details + 6, sizeof (details));

	if (ptr->args->st.filepaths != NULL &&
	    ptr->args->st.filepaths->length != 0) {
		strlcat(details, (char *)ptr->args->st.filepaths->head->data,
		    MAXPATHLEN);
	}
	if (*details == '\0') {
		snprintf(buf, sizeof (buf), "activityid=%s,starttime=%ld"
		    ",details=stage file,type=%s,pid=%d",
		    ptr->jobid, ptr->start, activitytypes[ptr->type],
		    ptr->pid);
	} else {
		snprintf(buf, sizeof (buf), "activityid=%s,starttime=%ld"
		    ",details=\"%s\",type=%s,pid=%d, description=%s",
		    ptr->jobid, ptr->start, details,
		    activitytypes[ptr->type], ptr->pid, details);
	}
	*result = copystr(buf);

	return (0);
}
