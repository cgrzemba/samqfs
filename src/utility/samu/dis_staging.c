/*
 * dis_staging.c - Display staging activity.
 *
 * Displays staging activity.
 *
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.28 $"


/* ANSI headers. */
#include <sys/types.h>
#include <sys/stat.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

/* Solaris headers. */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>

/* SAM-FS headers. */
#include "aml/fifo.h"
#include "aml/id_to_path.h"
#include "aml/tar.h"
#include "sam/nl_samfs.h"
#include "aml/stager.h"
#include "aml/stager_defs.h"

/* Local headers. */
#include "samu.h"

/* Private data. */
static int first_req;
static int first_stream;
static int nact;
static boolean show_path;
static StagerStateInfo_t *state = NULL;
static int details = 0;			/* detailed display */
static upath_t fullpath;

/* Private functions. */
static void disStager();
static void disGetDetails(int eq, char *media, char *vsn,
	StagerStateDetail_t *detail);
static void *mapInFile(char *file_name, int mode, size_t *len);
static void unMapFile(void *mp, size_t len);

StagerStateInfo_t *MapInStagerState();

/* External functions. */
extern StreamInfo_t *MapInStageStream(char *file_name);
extern void UnMapStageStream(StreamInfo_t *stream);


void
DisStaging(void)
{
	disStager();
}

#define	MAX(X, Y) ((X) > (Y)? (X) : (Y))
#define	TLINES MAX(((LINES - 6)), 1)
#define	HLINES MAX(((LINES - 6)/2), 1)
#define	QLINES MAX(((LINES - 6)/4), 1)

/*
 * Keyboard processing.
 */
boolean
KeyStaging(char key)
{
	int tlines;
	int blines;

	if (show_path) {
	tlines = MAX(HLINES/4, 1);
	} else {
	tlines = MAX(HLINES/3, 1);
	}
	blines = HLINES - 3;

	switch (key) {

	case KEY_adv_fmt:
		show_path ^= 1;
		return (TRUE);

	case KEY_full_fwd:
		if ((first_req += tlines) >= nact) first_req = 0;
		break;

	case KEY_full_bwd:
		if ((first_req -= tlines) < 0) first_req = 0;
		break;

	case KEY_half_bwd:
		if ((first_stream -= blines) < 0) first_stream = 0;
		break;

	case KEY_half_fwd:
		if ((first_stream += blines) >= STAGER_DISPLAY_STREAMS) {
			first_stream = 0;
		}
		break;

	case KEY_details:
		details ^= 1;
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
InitStaging(void)
{
	state = MapInStagerState();
	nact = 0;
	first_req = 0;
	first_stream = 0;
	if (Argc > 1) {
		int ac;
		for (ac = 1; ac < Argc; ac++) {
			switch (*Argv[ac]) {
				case 'I':
					details = 1;
					break;
			}
		}
	}
	return (TRUE);
}

/*
 * Map in stager daemon's state information.
 */
StagerStateInfo_t *
MapInStagerState()
{
	StagerStateInfo_t *state = NULL;

	(void) sprintf(fullpath, "%s/%s/%s",
	    SAM_VARIABLE_PATH, STAGER_DIRNAME, STAGER_STATE_FILENAME);
	state = (StagerStateInfo_t *)mapInFile(fullpath, O_RDONLY, NULL);

	return (state);
}

/*
 * Map in stage requests file.  This struct contains all stage
 * requests in progress.
 */
FileInfo_t *
MapInStageReqs(char *file_name,	size_t *len)
{
	FileInfo_t *stageReqs = NULL;

	stageReqs = (FileInfo_t *)mapInFile(file_name, O_RDWR, len);
	return (stageReqs);
}

/*
 * Map in stage stream file.  A stream is a group of files to
 * be staged together.
 */
StreamInfo_t *
MapInStageStream(char *file_name)
{
	StreamInfo_t *stream = NULL;

	stream = (StreamInfo_t *)mapInFile(file_name, O_RDWR, NULL);
	return (stream);
}

/*
 * Unmap stager daemon's state information.
 */
void
UnMapStagerState(StagerStateInfo_t *state)
{
	unMapFile((void *)state, sizeof (StagerStateInfo_t));
}

/*
 * Unmap stage requests file.
 */
void
UnMapStageReqs(FileInfo_t *stageReqs, size_t len)
{
	unMapFile((void *)stageReqs, len);
}

/*
 * Unmap stage stream file.
 */
void
UnMapStageStream(StreamInfo_t *stream)
{
	unMapFile((void *)stream, sizeof (StreamInfo_t));
}

/*
 * Get all files in a stage stream.
 */
int
GetStageStreamFiles(StreamInfo_t *stream, FileInfo_t *stageReqs,
	FileInfo_t **arrFiles)
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
		files = (FileInfo_t *)malloc(numFiles * sizeof (FileInfo_t));
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
 * Operator stager display.
 */
static void
disStager()
{
	int i;
	int copy;
	char *active;
	char *ty;
	time_t elapsed;
	time_t current;
	FileInfo_t *file;
	FileInfo_t *filearr;
	StreamInfo_t *stream;
	FileInfo_t *stageReqs;
	size_t reqlen;
	char *oprmsg;
	char *verify;
	boolean_t found = B_FALSE;

	ln = 2;

	if (state == NULL && (state = MapInStagerState()) == NULL) {
		Mvprintw(ln++, 0,
		    catgets(catfd, SET, 7370, "Stager daemon is not running."));
		return;
	}

	if (state->pid <= 0 || (kill(state->pid, 0) != 0)) {
		UnMapStagerState(state);
		state = NULL;
		Mvprintw(ln++, 0,
		    catgets(catfd, SET, 7370, "Stager daemon is not running."));
		return;
	}

	if (state->reqAlloc) {
		Mvprintw(1, 50, catgets(catfd, SET, 7404,
		    "maxactive %ld curr %ld"),
		    state->reqAlloc, state->reqEntries);
	}

	if (state->logFile) {
		Mvprintw(ln++, 0, catgets(catfd, SET, 7371,
		    "Log output to: %s"),
		    state->logFile);
	}

	Mvprintw(ln++, 0, "%s", " ");

	if (state->errmsg[0] != '\0') {
		Mvprintw(ln++, 0, state->errmsg);
		Mvprintw(ln++, 0, "%s", " ");
	}

	nact = 0;
	for (i = 0; i < STAGER_DISPLAY_ACTIVE; i++) {
		if (state->active[i].flags == 0) break;
		nact++;
	}

	if (nact) {

		for (i = first_req; i < nact; i++) {
			if (state->active[i].flags == 0) continue;

			Mvprintw(ln++, 0,
			    catgets(catfd, SET, 7398,
			    "Stage request %d: %s.%s"),
			    i+1, state->active[i].media, state->active[i].vsn);

			Mvprintw(ln++, 1, state->active[i].oprmsg);

			if (details) {
				if (state->active[i].flags ==
				    STAGER_STATE_COPY ||
				    state->active[i].flags ==
				    STAGER_STATE_POSITIONING) {
					disGetDetails(state->active[i].eq,
					    state->active[i].media,
					    state->active[i].vsn,
					    &state->active[i].detail);
				} else {
					Mvprintw(ln++, 0, " ");
				}
			} else {
				Mvprintw(ln++, 0, " ");
			}

			if (ln > HLINES + 3) {
				break;
			}
		}
		if (i < nact - 1) {
			Mvprintw(HLINES+3, 0,
			    catgets(catfd, SET, 2, "     more"));
		}
	}

	ln = HLINES + 4;

	/*
	 *	Map in stage requests file.  This struct contains all stage
	 *	requests in progress.
	 */
	stageReqs = MapInStageReqs(state->stageReqsFile, &reqlen);
	if (stageReqs == NULL) {
		Mvprintw(LINES-1, 0,
		    catgets(catfd, SET, 7374, "No active requests."));
		return;
	}

	/*
	 *	Display active and pending stage queues.
	 */

	for (i = 0; i < STAGER_DISPLAY_STREAMS; i++) {
		(void) sprintf(fullpath, "%s/%s.%d", state->streamsDir,
		    state->streams[i].vsn, state->streams[i].seqnum);

		stream = MapInStageStream(fullpath);

		if (stream == NULL) {
			continue;
		}

		if (ln > LINES - 3) {
			Mvprintw(LINES-2, 0,
			    catgets(catfd, SET, 2, "     more"));
			UnMapStageStream(stream);
			break;
		}

		ty = device_to_nm(stream->media);
		active = state->streams[i].active ?
		    (catgets(catfd, SET, 7379, "active")) :
		    (catgets(catfd, SET, 7380, "pending"));

		oprmsg = state->streams[i].oprmsg;

		(void) GetStageStreamFiles(stream, stageReqs, &filearr);
		file = &filearr[0];
		if (file == NULL) {
			if (filearr != NULL) {
				(void) free(filearr);
			}
			UnMapStageStream(stream);
			continue;
		}

		if (i < first_stream) {
				if (filearr != NULL) {
					(void) free(filearr);
				}
				UnMapStageStream(stream);
				found = B_FALSE;
				continue;
		} else  if (found == B_FALSE) {
				Mvprintw(ln++, 0, "");
				Mvprintw(ln++, 0, catgets(catfd, SET, 7399,
				    "Staging queues starting at %d"),
				    first_stream+1);
				Mvprintw(ln++, 0, catgets(catfd, SET, 7382,
				    "ty pid    user         status     "
				    "wait files vsn"));
				found = B_TRUE;
		}

		Mvprintw(ln++, 0, "%2s %-6d %-12s %-8s",
		    ty, file->pid, getuser(file->user), active);

		current = time(NULL);
		elapsed = (current - stream->create) / 60;   /* minutes */

		if (elapsed > 1440) {
			Printw("   %3dd", elapsed / 1440); /* days */
		} else {
			Printw("  %2d:%2.2d", elapsed / 60, elapsed % 60);
		}

		copy = file->copy;
		if (file->ar[copy].flags & STAGE_COPY_VERIFY) {
			verify = "V";
		} else {
			verify = " ";
		}

		Printw("%6d %s %s", stream->count, stream->vsn, verify);

		if (details) {
			Printw("    %s", oprmsg);
		}

		if (ln > LINES - 2) {
			Mvprintw(LINES-2, 0,
			    catgets(catfd, SET, 2, "     more"));
			if (filearr != NULL) {
				(void) free(filearr);
			}
			UnMapStageStream(stream);
			break;
		}

		if (filearr != NULL) {
			(void) free(filearr);
		}
		UnMapStageStream(stream);
	}
	UnMapStageReqs(stageReqs, reqlen);

	if (found == B_FALSE) {
		first_stream = 0;
	}
}

static void
disGetDetails(int eq, char *media, char *vsn, StagerStateDetail_t *detail)
{
	Mvprintw(ln++, 0, "%d %s.%s %llx.%lx %ld.%d %ld %lld %s %ld",
	    eq,
	    media, vsn,
	    detail->position, detail->offset,
	    detail->id.ino, detail->id.gen,
	    detail->fseq,
	    detail->len,
	    detail->name,
	    detail->pid);
}

/*
 *	Establish mapping between address space and file.
 */
static void *
mapInFile(char *file_name, int mode, size_t *len)
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
		return (NULL);
	}

	err = fstat(fd, &st);
	if (err != 0) {
		return (NULL);
	}

	mp = mmap(NULL, st.st_size, prot, MAP_SHARED, fd, 0);
	if (mp == MAP_FAILED) {
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
unMapFile(void *mp, size_t len)
{
	(void) munmap(mp, len);
}
