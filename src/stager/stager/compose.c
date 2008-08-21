/*
 * compose.c - sort stage file requests.
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

#pragma ident "$Revision: 1.27 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/param.h>

/* Solaris headers. */

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/shm.h"
#include "pub/stat.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "aml/stager_defs.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_lib.h"
#include "stager_config.h"
#include "copy_defs.h"
#include "file_defs.h"
#include "rmedia.h"
#include "stream.h"

#include "stage_reqs.h"
#include "schedule.h"

static void initComposeList();
static FileInfo_t **makeSortList();
static void separateVsns(FileInfo_t **sortList);
static void sortComposeList(FileInfo_t **sortList);
static int compareVsns(const void *p1, const void *p2);

/*
 * The composition list contains stage file requests that
 * have not been assigned to a stage stream yet.  Stage
 * requests in this list are composed (sorted) and assigned
 * to a stream.  The composition list maintains indices to
 * the request files.
 */
static struct {
	size_t		entries;		/* number of entries in use */
	size_t		alloc;			/* size of allocated space */
	int		*data;			/* indices to request files */
} composeList = { 0, 0, NULL };

/*
 * Add file to composition list.  Stage requests in this list
 * are composed (sorted) and assigned to a stream.
 */
void
AddCompose(
	int id)
{
	int free;

	if (composeList.data == NULL ||
	    (composeList.entries >= composeList.alloc)) {
		initComposeList();
	}

	if (composeList.entries < composeList.alloc) {
		free = composeList.entries;
		composeList.data[free] = id;
		composeList.entries++;
	} else {
		ASSERT_NOT_REACHED();
	}
}

/*
 * Compose stage file events into a stage stream.  Streams
 * represent optimal i/o requests for media.  A stream is
 * added to work queue.
 */
void
Compose(void)
{
	FileInfo_t **sortList;
	StreamInfo_t *stream;
	FileInfo_t *file;
	char *currentVsn;
	int i;
	int copy;
	int retry;
	boolean_t added;

	if (composeList.entries == 0) {
		return;
	}

	sortList = makeSortList();
	separateVsns(sortList);

	currentVsn = NULL;
	stream = NULL;

	for (i = 0; i < composeList.entries; i++) {

		file = GetFile(composeList.data[i]);

		copy = file->copy;

		/*
		 * If first VSN (currentVsn == NULL) or different VSN or
		 * disk cache already open, ie. multivolume, create a new
		 * stream.  Add new stream to the work queue.
		 */
		if (currentVsn == NULL ||
		    (strcmp(file->ar[copy].section.vsn, currentVsn) != 0) ||
		    GET_FLAG(file->flags, FI_DCACHE)) {

			retry = 3;
			stream = NULL;

			while (stream == NULL && retry-- > 0) {
				stream = CreateStream(file);
				if (stream == NULL) {
					Trace(TR_ERR, "Create stream failed");
					sleep(5);
				}
			}
			if (stream == NULL) {
				FatalSyscallError(EXIT_FATAL, HERE,
				    "Compose create stream", currentVsn);
			}

			AddWork(stream);

			if (GET_FLAG(file->flags, FI_DCACHE_CLOSE) == 0) {
				currentVsn = file->ar[copy].section.vsn;
			}
		}

		/*
		 * Add stage request to the stream.
		 */
		added = AddStream(stream, composeList.data[i],
		    ADD_STREAM_NOSORT);
		if (added == B_FALSE) {
			SetErrno = 0;	/* set for trace */
			Trace(TR_ERR,
			    "Compose add stream '%s.%d' 0x%x failed",
			    stream->vsn, stream->seqnum, (int)stream);
			SET_FLAG(stream->flags, SR_full);
		}

		/* Stream is full, create a new one. */
		if (GET_FLAG(stream->flags, SR_full)) {
			currentVsn = NULL;
		}
	}

	/*
	 * We should have added all requests to the work queue
	 * so free the composition list.
	 */
	composeList.entries = 0;
}

int
GetNumComposeEntries(void)
{
	return (composeList.entries);
}

/*
 * Construct list of stage file pointers. This
 * list is used in performing sorts and separations.
 * The actual list is private to this module.  Its size
 * will be increased to hold the required list.
 */
static FileInfo_t **
makeSortList(void)
{
	static FileInfo_t **sortList = NULL;
	static size_t alloc = 0;

	int i;
	size_t size;
	FileInfo_t *file;

	size = composeList.entries * sizeof (FileInfo_t *);
	if (size > alloc) {
		/*
		 *  Increasing size of sort list, free any old stuff.
		 */
		if (alloc > 0 && sortList != NULL) {
			SamFree(sortList);
		}
		alloc = size;
		SamMalloc(sortList, alloc);
	}

	for (i = 0; i < composeList.entries; i++) {
		file = GetFile(composeList.data[i]);
		sortList[i] = file;
	}
	return (sortList);
}

/*
 * Create sorted list based on VSN.
 */
static void
separateVsns(
	FileInfo_t **sortList)
{
	qsort(sortList, composeList.entries, sizeof (FileInfo_t *),
	    compareVsns);

	sortComposeList(sortList);
}

/*
 * Sort the composition list based on acquired sorting
 * information. The composition list maintains indices to
 * stage requests.
 */
static void
sortComposeList(
	FileInfo_t **sortList)
{
	int i;
	FileInfo_t *file;

	for (i = 0; i < composeList.entries; i++) {
		file = sortList[i];
		composeList.data[i] = file->sort;
	}
}

/*
 * Initialize composition list.
 */
static void
initComposeList(void)
{
	int size;

	if (composeList.data == NULL) {
		composeList.alloc = COMPOSE_LIST_CHUNKSIZE;
		size = composeList.alloc * sizeof (int);
		SamMalloc(composeList.data, size);
		memset(composeList.data, 0, size);

	} else {
		composeList.alloc += COMPOSE_LIST_CHUNKSIZE;
		size = composeList.alloc * sizeof (int);
		SamRealloc(composeList.data, size);
	}
}

/*
 * Compare VSNs.  Include position and offset in compare.
 */
static int
compareVsns(
	const void *p1,
	const void *p2)
{
	FileInfo_t **f1 = (FileInfo_t **)p1;
	FileInfo_t **f2 = (FileInfo_t **)p2;
	int copy1 = (*f1)->copy;
	int copy2 = (*f2)->copy;
	ArcopyInfo_t *ar1 = &((*f1)->ar[copy1]);
	ArcopyInfo_t *ar2 = &((*f2)->ar[copy2]);
	int icmp;

	icmp = strcmp(ar1->section.vsn, ar2->section.vsn);
	if (icmp < 0)
		return (-1);
	if (icmp > 0)
		return (1);

	if (ar1->section.position > ar2->section.position)
		return (1);
	if (ar1->section.position < ar2->section.position)
		return (-1);

	if (ar1->section.offset > ar2->section.offset)
		return (1);
	if (ar1->section.offset < ar2->section.offset)
		return (-1);

	return (0);
}
