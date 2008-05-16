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
#pragma ident	"$Revision: 1.20 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include "pub/mgmt/types.h"
#include "pub/mgmt/file_util.h"
#include "mgmt/util.h"
#include "sam/sam_trace.h"
#include "pub/mgmt/error.h"

static char *read_previous_line(char *cur, char *beg);

/*
 * DESCRIPTION:
 *	Function to get lines from the tail of a file.
 *	Local users must free res and data when they are done with the
 *	strings. Local users should not free each of the strings pointed
 *	to by res.
 *
 *	RPC Users will recieve NULL for data but must free
 *	res and each of the strings it points to.
 *
 *	The returned array will contain one more element than the number
 *	of lines that was requested or one more than the number of
 *	lines in the file- if more lines were requested than exist in
 *	the file. The last element in the array is NULL.
 *
 * PARAMS:
 *	ctx_t *		IN	- context object
 *	char *		IN	- name of file to tail
 *	int *		IN-OUT	- IN: maximum number of lines to return,
 *				  OUT: number of lines being returned.
 *	char ***	OUT	- malloced array of ptrs to lines in file
 *	char **		OUT	- NOT PASSED ON REMOTE CALLS. malloced.
 *				  local users should not need to do anything
 *				  with data but free it when done with the res.
 */
int
tail(
ctx_t *ctx /* ARGSUSED */,
char *file,
uint32_t *items,
char ***res,
char **data)
{

	struct stat64	stbuf;
	off_t		sz = 0;
	char		*cmap;
	int32_t		curline = 0;
	char		*cur;
	char		**lines;
	int		fd;
	char		*beg, *end;
	int32_t		i;
	ptrdiff_t	delta;
	char		*newline;
	int32_t		lncnt = 0;
	int32_t		stop_at  = 0;
	int32_t		howmany = 0;
	char		**tmp;
	int		num = 0;
	char		err_buf[80];

	if (ISNULL(file, res, data, items)) {
		Trace(TR_ERR, "tail failed: %d %s", samerrno, samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "tail %d %s", *items, file);

	howmany = *items;
	*res = NULL;
	*data = NULL;
	if (howmany > 1) {
		curline = howmany - 1;
	}

	if ((fd = open64(file, O_RDONLY)) < 0) {
		samerrno = SE_TAIL_FAILED;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_TAIL_FAILED), err_buf);
		Trace(TR_ERR, "tail failed: %d %s", samerrno, samerrmsg);
		return (-1);
	}
	if (fstat64(fd, &stbuf) != 0) {
		samerrno = SE_TAIL_FAILED;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_TAIL_FAILED), err_buf);
		close(fd);
		Trace(TR_ERR, "tail failed: %d %s", samerrno, samerrmsg);
		return (-1);
	}

	/* check it is a regular file and get its size. */
	sz = stbuf.st_size;
	if (sz <= 0 || !S_ISREG(stbuf.st_mode)) {
		*items = 0;
		*res = NULL;
		*data = NULL;
		close(fd);

		Trace(TR_OPRMSG, "tail found empty file %s", file);
		return (0);
	}

	cmap = (char *)mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);

	if (cmap == MAP_FAILED) {
		samerrno = SE_TAIL_FAILED;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_TAIL_FAILED), err_buf);

		close(fd);
		Trace(TR_ERR, "tail failed: %d %s", samerrno, samerrmsg);
		return (-1);
	}

	beg = cmap;
	end = cmap + sz;


	/* check to see if file ends with a newline */
	if (*(end - 1) == '\n') {
		cur = end - 2;
	} else {
		cur = end;
	}

	lines = (char **)calloc(howmany, sizeof (char *));
	if (lines == NULL) {
		setsamerr(SE_NO_MEM);
		close(fd);
		munmap(cmap, sz);
		Trace(TR_ERR, "tail failed: %d %s", samerrno, samerrmsg);
		return (-1);
	}

	/*
	 * Read the file from tail to head saving pointers to the
	 * lines that were requested. Saved pointers point to the
	 * end of each line.
	 */
	while (cur = read_previous_line(cur, cmap)) {
		/* save a pointer to the line */
		lines[curline] = cur;
		lncnt++;

		/* all requested lines have been found or no more exist */
		if (curline == 0 || cur == beg) {
			break;
		}

		curline--;
	}


	/*
	 * Lines is an array of pointers to the lines that were
	 * requested.  Now allocate a data block to hold the
	 * output. This approach prevents the need for mallocs
	 * for each individual data line.
	 */
	(*data) = (char *)calloc(PTRDIFF(end, cur), sizeof (char));
	if (*data == NULL) {
		setsamerr(SE_NO_MEM);
		close(fd);
		free(lines);
		munmap(cmap, sz);
		Trace(TR_ERR, "tail failed: %d %s", samerrno, samerrmsg);
		return (-1);
	}
	memcpy((*data), cur, PTRDIFF(end, cur));

	/*
	 * allocate an array of the correct size to return the actual pointers
	 * in lines;
	 */
	tmp = (char **)calloc(lncnt + 1, sizeof (char *));
	if (*data == NULL) {
		setsamerr(SE_NO_MEM);
		close(fd);
		free(lines);
		free(*data);
		munmap(cmap, sz);
		Trace(TR_ERR, "tail failed: %d %s", samerrno, samerrmsg);
		return (-1);
	}


	/*
	 * Adjust the pointers so they reference the new data
	 * block and point to the beginnings of strings instead of the
	 * end.
	 */
	stop_at = howmany - lncnt;
	for (i = howmany -1, num = lncnt-1; i >= stop_at; i--, num--) {

		/*
		 * Where is the current line in relation to beginning of
		 * the range of memory that represents the lines we are
		 * going to return. This is the offset into the data
		 * block of the line.
		 */
		delta = PTRDIFF(lines[i], cur);

		/*
		 * The first branch of the if handles the beginning of the file
		 * coming before sufficent lines are collected.
		 */
		if (*(lines[i]) != '\n') {
			tmp[num] = *data + delta;
		} else {
			*(*data + delta) = '\0';
			tmp[num] = *data + delta + 1;
		}
	}
	tmp[lncnt] = NULL;

	/* make sure the very last char is a '\0' */
	newline = strstr(tmp[lncnt - 1], "\n");
	if (newline != NULL) {
		*newline = '\0';
	}

	*res = tmp;
	free(lines);
	munmap(cmap, sz);
	close(fd);
	*items = lncnt;

	Trace(TR_OPRMSG, "tail %d %s succeeded", howmany, file);
	return (0);
}


/*
 * Return a pointer to the newline that precedes the current position.
 * search backwards for '\n' without passing the beginning.
 */
static char *
read_previous_line(char *cur, char *beg) {

	if (cur <= beg) {
		return (beg);
	}

	while (*--cur != '\n') {
		if (cur == beg) {
			return (beg);
		}
	}
	return (cur);
}


/*
 * DESCRIPTION:
 *	Function to get a number of lines from a text file from a starting
 *	point.
 *
 *	The function has been written to avoid the need to malloc individual
 *	lines of text. Instead a data block is allocated to contain the
 *	actual text and an array of ptrs in to this block. Because of this
 *	instead of needing 2000 mallocs to hold 2000 strings, 3 will suffice.
 *	This design leads to the following usage patterns:
 *
 *	Local users must free only res and data when they are done with the
 *	strings. Local users should not free each of the strings pointed
 *	to by res.
 *
 *	RPC client users will recieve NULL for data but must free
 *	res and each of the strings it points to.
 *
 *	The returned array will contain one more element than the number
 *	of lines that was requested or one more than the number of
 *	lines in the file- if more lines were requested than exist in
 *	the file. The last element in the array is NULL.
 *
 * PARAMS:
 *	ctx_t *		IN	- context object
 *	char *		IN	- name of file to get lines from.
 *	int		IN	- the line at which to begin, first line = 1
 *	int *		IN-OUT	- IN: maximum number of lines to return,
 *				  OUT: number of lines being returned.
 *	char ***	OUT	- malloced array of ptrs to lines in file
 *	char **		OUT	- NOT PASSED ON REMOTE CALLS. malloced.
 *				  local users should not need to do anything
 *				  with data but free it when done with the res.
 */
int
get_txt_file(
ctx_t *c	/* ARGSUSED */,
char *file,
uint32_t start_at,
uint32_t *items,
char ***res,
char **data) {

	struct stat64	stbuf;
	char		**lines, **tmp;
	char		*cmap, *cur, *beg, *end;
	uint32_t	lncnt = 0, howmany, i;
	int		fd;
	off_t		sz;
	ptrdiff_t	delta;
	char		err_buf[80];

	if (ISNULL(file, items, res, data)) {
		Trace(TR_ERR, "get_txt_file failed: %d %s",
			samerrno, samerrmsg);
		return (-1);
	}
	howmany = *items;

	Trace(TR_OPRMSG, "get_text_file %s %d %d", file, start_at,
		howmany);

	/* open and stat the file */
	if ((fd = open64(file, O_RDONLY)) < 0) {
		samerrno = SE_TAIL_FAILED;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_TAIL_FAILED), err_buf);
		Trace(TR_ERR, "get_txt_file failed: %d %s",
			samerrno, samerrmsg);
		return (-1);
	}
	if (fstat64(fd, &stbuf) != 0) {
		samerrno = SE_TAIL_FAILED;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_TAIL_FAILED), err_buf);

		close(fd);
		Trace(TR_ERR, "get_txt_file failed: %d %s",
			samerrno, samerrmsg);
		return (-1);
	}

	/* check it is a regular file and get its size. */
	sz = stbuf.st_size;
	if (sz <= 0 || !S_ISREG(stbuf.st_mode)) {
		*items = 0;
		*res = NULL;
		*data = NULL;

		close(fd);

		Trace(TR_OPRMSG, "get_txt_file found empty file %s", file);
		return (0);
	}

	cmap = (char *)mmap64(NULL, sz, PROT_READ,
		MAP_PRIVATE, fd, 0);

	if (cmap == MAP_FAILED) {
		samerrno = SE_TAIL_FAILED;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_TAIL_FAILED), err_buf);

		close(fd);
		Trace(TR_ERR, "get_txt_file failed: %d %s",
			samerrno, samerrmsg);
		return (-1);
	}

	end = cmap + sz;
	cur = beg = cmap;

	/* walk the file looking for our starting point */
	do {
		/*
		 * increment lncnt because we find lines by finding
		 * the \n on the line before. So by here we have
		 * already passed the line contents.
		 */
		lncnt++;
		/* Reached the requested starting point */
		if (lncnt >= start_at) {
			break;
		}

		/* Increment current to get past the new line */
		cur++;

	} while ((cur = strpbrk(cur, "\n")) != NULL);

	if (lncnt < start_at) {
		/*
		 * There were fewer than start_at lines in
		 * the file. Return empty ptrs.
		 */
		*items = 0;
		*res = NULL;
		*data = NULL;
		close(fd);
		munmap(cmap, sz);
		Trace(TR_OPRMSG, "get_text_file %s",
			"end reached before start count");
		return (0);
	}

	/*
	 * cur will never be null here because that can only happen if
	 * insufficient lines were found
	 */
	if (*cur == '\n') {
		cur++;
	}

	/* create an array of char ptrs to hold the results */
	lines = (char **)calloc(howmany + 1, sizeof (char *));
	if (lines == NULL) {
		setsamerr(SE_NO_MEM);
		munmap(cmap, sz);
		close(fd);
		Trace(TR_ERR, "get_txt_file failed: %d %s",
			samerrno, samerrmsg);
		return (-1);
	}

	/* put the first element into the list */
	beg = cur;
	lncnt = 0;


	/*
	 * walk from start until sufficient lines have been read keeping
	 * pointers to each line.
	 */
	while (lncnt < howmany) {
		/*
		 * save a pointer to the cur newline then increment
		 * cur so that on the next call to strpbrk
		 * we will look past the cur \n
		 */
		lines[lncnt] = cur;
		if ((cur = strpbrk(cur, "\n")) == NULL) {
			/* EOF encountered before getting sufficent lines */


			/*
			 * if lncnt is zero (only 1 line was found) or
			 * if the file doesn't end with a new line
			 * increment lncnt to handle these further down.
			 */
			if (lncnt == 0 || *(end - 1) != '\n') {
				lncnt++;
			}
			cur = end;
			break;
		}
		lncnt++;
		cur++;
	}
	/* at this point lncnt should == the number of actual lines found */

	/*
	 * Lines is an array of pointers to the lines that were
	 * requested.  Now allocate and populate a data block to hold the
	 * output. This approach prevents the need for mallocs
	 * for each individual data line.
	 */
	(*data) = (char *)calloc(PTRDIFF(cur, (beg + 1)), sizeof (char));
	if (*data == NULL) {
		setsamerr(SE_NO_MEM);

		free(lines);
		munmap(cmap, sz);
		close(fd);

		Trace(TR_ERR, "get_txt_file failed: %d %s",
			samerrno, samerrmsg);

		return (-1);
	}
	memcpy((*data), beg, PTRDIFF(cur, (beg + 1)));


	/*
	 * Allocate an array of the correct size to return the actual pointers
	 * in lines.
	 */
	tmp = (char **)calloc(lncnt + 1, sizeof (char *));
	if (tmp == NULL) {
		setsamerr(SE_NO_MEM);
		free(lines);
		free(*data);

		munmap(cmap, sz);
		close(fd);

		Trace(TR_ERR, "get_txt_file failed: %d %s",
			samerrno, samerrmsg);
		return (-1);
	}

	/*
	 * Adjust the pointers so they reference the new data
	 * block
	 */
	for (i = 0; i < (lncnt); i++) {
		/*
		 * Where is the current line in relation to beginning of
		 * the range of memory that represents the lines we are
		 * going to return. This is the offset into the data
		 * block of the line.
		 */
		delta = PTRDIFF(lines[i], beg);
		if (i != 0) {
			if (*((lines[i]) - 1) == '\n') {
				*((*data + delta - 1)) = '\0';
			}
		}
		tmp[i] = *data + delta;

	}

	/* now add a null terminator for the last entry */
	if (*(*data + PTRDIFF(cur, beg) - 1) == '\n') {
		*(*data + PTRDIFF(cur, beg) - 1) = '\0';
	}
	tmp[i] = NULL;
	*res = tmp;
	free(lines);
	munmap(cmap, sz);
	close(fd);
	*items = lncnt;

	return (0);
}
