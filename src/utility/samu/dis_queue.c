/*
 * dis_queue.c - Display staging queues.
 *
 * Displays staging queues.
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

#pragma ident "$Revision: 1.25 $"


/* ANSI headers. */
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

/* SAM-FS headers. */
#include "aml/fifo.h"
#include "aml/id_to_path.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "aml/stager_defs.h"

/* Local headers. */
#include "samu.h"

/* Private data. */
static media_t media;
static int FileFirst = 0;
static int qshow_path;
static upath_t fullpath;

/* Public functions. */

/* Private functions. */

/* External functions. */
extern FileInfo_t *MapInStageReqs(char *file_name, size_t *len);
extern StreamInfo_t *MapInStageStream(char *file_name);
extern void UnMapStageStream(StreamInfo_t *stream);
extern void UnMapStageReqs(FileInfo_t *stageReqs, size_t len);
extern int GetStageStreamFiles(StreamInfo_t *stream, FileInfo_t *stageReqs,
						FileInfo_t **arrFiles);

void
DisQueue(void)
{
	char *display_media;

	display_media = (media == 0) ? "all" : sam_mediatoa(media);
	Mvprintw(0, 0, catgets(catfd, SET, 7377,
	    "Staging queue by media type: %s"), display_media);

	DisplayStageStreams(B_TRUE, FileFirst, qshow_path, media);
}

#define	MAX(X, Y) ((X) > (Y)? (X) : (Y))
#define	TLINES MAX(((LINES - 6)), 1)
#define	HLINES MAX(((LINES - 6)/2), 1)
#define	QLINES MAX(((LINES - 6)/4), 1)

/*
 * Keyboard processing.
 */
boolean
KeyQueue(char key)
{
	int tlines, hlines;

	if (qshow_path) {
		tlines = HLINES;
		hlines = QLINES;
	} else {
		tlines = TLINES;
		hlines = HLINES;
	}

	switch (key) {

	case KEY_adv_fmt:
	case KEY_details:
		qshow_path ^= 1;
		break;

	case KEY_full_fwd:
		FileFirst += tlines;
		break;

	case KEY_full_bwd:
		if ((FileFirst -= tlines) < 0) FileFirst = 0;
		break;

	case KEY_half_bwd:
		if ((FileFirst -= hlines) < 0) FileFirst = 0;
		break;

	case KEY_half_fwd:
		FileFirst += hlines;
		break;

	default:
		return (FALSE);
	}
	return (TRUE);
}

/*
 * Initialize display.
 */
boolean
InitQueue(void)
{
	media_t m = (media_t)0;
	int i = 1;

	qshow_path = FALSE;

	while (Argc > i) {
		if ((m = sam_atomedia(Argv[i])) != 0) {
			media = m;
		} else if (strcmp(Argv[i], "all") == 0) {
			media = 0;
		} else if (strcmp(Argv[i], "path") == 0) {
			qshow_path = TRUE;
		} else {
			Error(catgets(catfd, SET, 2768,
			    "Unknown media type (%s)"),
			    Argv[i]);
		}
		i++;
	}
	return (TRUE);
}

void
DisplayStageStreams(boolean_t display_active, int file_first,
	int display_filename, media_t media)
{
	int i, j;

	char file_size[15];
	char *cp;
	int copy;
	equ_t fseq;
	int inode;
	char *ty;
	u_longlong_t position;
	ulong_t offset;
	vsn_t vsn;

	StagerStateInfo_t *state = NULL;
	FileInfo_t *stageReqs = NULL;
	StreamInfo_t *stream;
	FileInfo_t *file;
	FileInfo_t *filearr;
	int num;
	size_t reqlen;
	char *filename;
	char *verify;

	int file_current = 0;
	int total_files = 0;
	int total_volumes = 0;

	state = MapInStagerState();
	if (state == NULL) {
		Mvprintw(2, 0, catgets(catfd, SET, 2400,
		    "Staging data not available"));
		return;
	}

	Mvprintw(ln++, 0, catgets(catfd, SET, 7375,
	    "ty     length  fseq        ino   position     offset vsn"));
	Mvprintw(ln++, 0, " ");

	/*
	 * Map in stage requests file.  This struct contains all stage
	 * requests in progress.
	 */
	stageReqs = MapInStageReqs(state->stageReqsFile, &reqlen);
	if (stageReqs == NULL) {
		UnMapStagerState(state);
		return;
	}

	for (i = 0; i < STAGER_DISPLAY_STREAMS; i++) {
		if (state->streams[i].vsn[0] == '\0')
			break;

		if (media != 0 && media != state->streams[i].media)
			continue;

		if (((display_active == B_FALSE) &&
		    (state->streams[i].active == B_TRUE)) ||
		    ((display_active == B_TRUE) &&
		    (state->streams[i].active == B_FALSE))) {
			continue;
		}

		(void) sprintf(fullpath, "%s/%s.%d", state->streamsDir,
		    state->streams[i].vsn, state->streams[i].seqnum);

		/*
		 * Map in stage stream.  A stream is a group of files to
		 * be staged together.
		 */
		stream = MapInStageStream(fullpath);

		if (stream == NULL) {
			continue;
		}

		(void) pthread_mutex_lock(&(stream->mutex));
		total_files += stream->count;
		(void) pthread_mutex_unlock(&(stream->mutex));

		total_volumes++;
		UnMapStageStream(stream);
	}

	for (i = 0; i < STAGER_DISPLAY_STREAMS; i++) {
		if (state->streams[i].vsn[0] == '\0') break;

		if (media != 0 && media != state->streams[i].media) continue;

		if (((display_active == B_FALSE) &&
		    (state->streams[i].active == B_TRUE)) ||
		    ((display_active == B_TRUE) &&
		    (state->streams[i].active == B_FALSE))) {
			continue;
		}

		(void) sprintf(fullpath, "%s/%s.%d", state->streamsDir,
		    state->streams[i].vsn, state->streams[i].seqnum);

		/*
		 * Map in stage stream.  A stream is a group of files to
		 * be staged together.
		 */
		stream = MapInStageStream(fullpath);

		if (stream == NULL) {
			continue;
		}

		num = GetStageStreamFiles(stream, stageReqs, &filearr);

		if (ln > LINES - 3) {
			if (filearr != NULL) {
				(void) free(filearr);
			}
			UnMapStageStream(stream);
			break;
		}

		for (j = 0; j < num; j++) {

			file_current++;
			if (file_current < file_first) continue;

			file = &filearr[j];

			cp = FsizeToB(file->len);
			(void) sprintf(file_size, "%9s", cp);

			fseq = file->fseq;
			inode = file->id.ino;
			copy = file->copy;
			ty = device_to_nm(file->ar[copy].media);
			position = file->ar[copy].section.position;
			offset = file->ar[copy].section.offset;
			(void) strcpy(vsn, file->ar[copy].section.vsn);
			if (display_filename) {
				filename = id_to_path(GetFsMountName(fseq),
				    file->id);
			}
			if (file->ar[copy].flags & STAGE_COPY_VERIFY) {
				verify = "V";
			} else {
				verify = " ";
			}

			if (ln > LINES - 3) {
				Mvprintw(LINES-2, 0,
				    catgets(catfd, SET, 2, "     more"));
				break;
			}

			Mvprintw(ln++, 0, "%s %10s %5d %10d %10llx %10lx %s %s",
			    ty, file_size, fseq, inode,
			    position, offset, vsn, verify);

			if (ln > LINES - 3) {
				Mvprintw(LINES-2, 0,
				    catgets(catfd, SET, 2, "     more"));
				break;
			}

			if (display_filename) {
				Mvprintw(ln++, 3, "%s", filename);
			}

		}
		if (filearr != NULL) {
			(void) free(filearr);
		}
		UnMapStageStream(stream);
	}
	UnMapStageReqs(stageReqs, reqlen);

	Mvprintw(1, 50, catgets(catfd, SET, 7376, "volumes %d files %d"),
	    total_volumes, total_files);

	UnMapStagerState(state);
}
