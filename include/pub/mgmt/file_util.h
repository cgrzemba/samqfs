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
#ifndef	_FSM_FILE_UTIL_H
#define	_FSM_FILE_UTIL_H

#pragma ident   "$Revision: 1.17 $"

/*
 * file_util.h - Utilities for manipulating files and directories
 */


#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"


/*
 *	global definitions
 */

/* File status values for use with get_file_status */
#define	SAMR_MISSING	0	/* File does not exist */
#define	SAMR_REGFILE	1	/* File is regular file and on disk */
#define	SAMR_DIRFILE	2	/* File is directory and on disk */
#define	SAMR_RELFILE	3	/* File is not on disk */
#define	SAMR_NOTFILE	4	/* File is neither regular nor directory */


/* File types for use with the file_type key of get_extended_file_details */
#define	FTYPE_LNK  0
#define	FTYPE_REG  1
#define	FTYPE_DIR  2
#define	FTYPE_FIFO 3
#define	FTYPE_SPEC 4
#define	FTYPE_UNK  5

/*
 * Releaser attribute flags for use with release_atts key of
 * get_extended_file_details
 */
#define	RL_ATT_NEVER		0x0001
#define	RL_ATT_WHEN1COPY	0x0002
#define	RL_ATT_MASK		0x000F

/*
 * Stager attribute for use with the stage_atts key of
 * get_extended_file_details
 */
#define	ST_ATT_NEVER	0x0010
#define	ST_ATT_ASSOC	0x0020
#define	ST_ATT_MASK	0x00F0


/*
 * Archiver attribute for use with the archive_atts key of
 * get_extended_file_details
 */
#define	AR_ATT_NEVER		0x0100
#define	AR_ATT_CONCURRENT	0x0200
#define	AR_ATT_INCONSISTENT	0x0400
#define	AR_ATT_MASK		0x0F00


/*
 *	function prototypes
 */


/*
 * DESCRIPTION:
 *	Get a list of files from a specified directory up to "maxentries".
 *	If "restrictions" is not empty, applies the filter(s) specified.
 *
 * PARAMS:
 *   ctx_t*		IN   - context object
 *   int		IN   - maximum number of entries to return
 *   char*		IN   - filter to be applied when selecting files
 *   sqm_lst_t**	OUT  - a list of char* filenames relative to
 *				specified path
 * RETURNS:
 *    0 -- success
 *   -1 -- error
 */
int
list_dir(
	ctx_t *c,
	int maxentries,
	char *filepath,
	char *restrictions,
	sqm_lst_t **direntries);


/*
 * DESCRIPTION:
 *	Given a list of files on a SAM file system, sets whether the file is
 *	on disk or not.
 *
 * PARAMS:
 *   ctx_t*		IN   - context object
 *   sqm_lst_t**	IN   - a list of char* paths to be checked
 *   sqm_lst_t**	OUT  - a list of ints indicating status
 *
 * RETURNS:
 *    0 -- success
 *   -1 -- error
 */
int
get_file_status(
	ctx_t *c,
	sqm_lst_t *filepaths,
	sqm_lst_t **status);


/*
 * DESCRIPTION:
 *	Given a list of files, returns a list of string-value pairs
 *	describing the file.
 *
 *	Currently returns:
 *		size=%lld	file size in bytes
 *		created=%lu	creation date
 *		modified=%lu	modification date
 *
 * PARAMS:
 *   ctx_t*		IN   - context object
 *   char*		IN   - file system name
 *   sqm_lst_t**	IN   - a list of char* paths to be checked
 *   sqm_lst_t**	OUT  - a list of char* file details
 *
 * RETURNS:
 *    0 -- success
 *   -1 -- error
 */
int
get_file_details(
	ctx_t *c,
	char *fsname,
	sqm_lst_t *files,
	sqm_lst_t **filestatus);



#define	FD_FILE_TYPE	0x00000001
#define	FD_SIZE		0x00000002
#define	FD_CREATED	0x00000004
#define	FD_MODIFIED	0x00000008
#define	FD_ACCESSED	0x00000010
#define	FD_USER		0x00000020
#define	FD_GROUP	0x00000040
#define	FD_MODE		0x00000080
#define	FD_FNAME	0x00000100
#define	FD_WORM		0x00000200
#define	FD_SAM_STATE	0x00001000
#define	FD_RELEASE_ATTS	0x00002000
#define	FD_STAGE_ATTS	0x00004000
#define	FD_SEGMENT_ATTS 0x00008000
#define	FD_CHAR_MODE	0x00010000
#define	FD_COPY_SUMMARY	0x00020000
#define	FD_COPY_DETAIL	0x00040000
#define	FD_SEGMENT_ALL	0x00080000
#define	FD_ARCHIVE_ATTS	0x00100000

#define	FD_SUMMARY	FD_FILE_TYPE|FD_SIZE|FD_CREATED|FD_MODIFIED|\
			FD_ACCESSED|FD_USER|FD_GROUP|FD_MODE|FD_FNAME|\
			FD_SAM_STATE|FD_CHAR_MODE|FD_COPY_SUMMARY|FD_WORM

#define	FD_ALL		0xffffffff

/*
 * DESCRIPTION:
 *	Given a list of files, returns a list of string-value pairs
 *	describing the file. Which key value pairs are included is controlled
 *	by the which_details field.
 *
 * PARAMS:
 *   ctx_t*		IN   - context object
 *   sqm_lst_t**	IN   - a list of char* paths to be checked
 *   uint32_t		IN   - flags to determine which file details
 *				are included in output
 *
 *   sqm_lst_t**	OUT  - a list of char* file details
 *
 * RETURNS:
 *    0 -- success
 *   -1 -- error
 */
int
get_extended_file_details(
	ctx_t *c,
	sqm_lst_t *files,
	uint32_t which_details,
	sqm_lst_t ** file_details);


/*
 * DESCRIPTION:
 *	This function returns information about the status and location of
 *	the archive copies for the file specified in file_path.
 *
 *	e.g. non-segmented file on 1 vsn:
 *	copy=1,created=1130378055,media=dk,vsn=dk2
 *
 *	e.g output for a segmented file on 2 different vsns:
 *	copy=1,seg_count=3,created=1130289560,media=dk,vsn=dk1 dk2
 *
 *	each copy may also have a count of stale, damaged, inconsistent or
 *	unarchived. If any of these are returned the copy is not a valid
 *	candidate for staging.
 *
 *	damaged means the copy has been marked as damaged by the user with
 *	the damage command or by the system as a result of a fatal error
 *	when trying to stage. damaged copies ARE NOT???? available for staging
 *
 *	inconsistent means that the file was modified while the
 *	archive copy was being made. By default such a copy would not
 *	be allowed. However there is a flag to the archive command
 *	that allows the archival of inconsistent copies. inconsistent
 *	copies can only be staged after a samfsrestore. They are not
 *	candidates for staging in a general case. However, knowledge
 *	of their existence is certainly desireable to a user.
 *
 *
 * This function only returns aggregated information for segmented files
 * and thus will not return all fields for any segmented file.
 *
 * Likewise if any particular copy overflows into more than one vsn,
 * the information about the copy is aggregated.
 *
 * PARAMS:
 *   ctx_t *	IN   - context object
 *   char *	IN   - fully qualified file path
 *   uint32_t   IN   - flag to indicate what information to return (see below).
 *   list **	OUT  - Strings of key value pairs
 * RETURNS:
 *   success -  0
 *   error   -  -1
 *
 *
 * which_details is for future use it will allow collection of extened
 * information about all of the vsns on which a copy resides.
 * FLAG				Result
 * CD_AGGREGATE_VSN_INFO	a summary of information is included about
 *				all of the vsns used to archive each copy of
 *				the file. This means that position and length
 *				information which is on a per segment/section
 *				basis is not provided except for non-segmented
 *				files that do not overflow onto multiple vsns
 *				A single string will be returned for each
 *				copy.
 * CD_DETAILED_VSN_INFO		A string will be returned for each
 *				segment/section for each copy. If this option
 *				is used additional subfields could be
 *				returned.
 */
int
get_copy_details(
ctx_t *c,
char *file_path,
uint32_t which_details,
sqm_lst_t **res);


/*
 * DESCRIPTION:
 *	Create a file if it does not already exist. This function will also
 *	create any missing directories. If the file already exists
 *	this function will return success.
 * PARAMS:
 *   ctx_t *    IN   - context object
 *   upath_t    IN   - fully qualified file name
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int create_file(ctx_t *ctx, upath_t full_path);


/*
 * check if the file exist
 */
int file_exists(ctx_t *ctx, upath_t file_path);


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
 *	int *		IN-OUT	- IN: maximum number of lines to return
 *				  OUT: number of lines being returned
 *	char ***	OUT	- malloced array of ptrs to lines in file
 *	char **		OUT	- NOT PASSED ON REMOTE CALLS. malloced.
 *				  Users should not need to do anything with
 *				  data but free it when done with the res.
 */
int
tail(ctx_t *ctx, char *file, uint32_t *howmany, char ***res, char **data);


/*
 * create the directory dir if it does not already exist. If the directory
 * already exists this function returns success.
 */
int create_dir(ctx_t *ctx, upath_t dir);


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
get_txt_file(ctx_t *c, char *file, uint32_t start_at,
	uint32_t *items, char ***res, char **data);

/*
 *  list_directory()
 *  Replacement function for the older list_dir that allows the caller
 *  to continue where they left off if there are more directory entries
 *  not yet collected.
 *
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
	ctx_t		*c,
	int		maxentries,
	char		*listDir,	/* starting point. NULL = rootdir */
	char		*startFile,	/* if continuing, start from here */
	char		*restrictions,
	uint32_t	*morefiles,	/* OUT */
	sqm_lst_t	**direntries);	/* OUT */

/*
 * list_and_collect_file_details()
 *
 * Revised version of get_extended_file_details that doesn't require
 * the caller to provide the list of files.  This function may be used
 * for both live filesystems and getting file details from Restore Points.
 */
int
list_and_collect_file_details(
	ctx_t		*c,
	char		*fsname,	/* filesystem name */
	char		*snappath,	/* could be NULL if ! Restore */
	char		*startDir,	/* starting point. NULL = rootdir */
	char		*startFile,	/* if continuing, start from here */
	int32_t		howmany,	/* how many to return.  -1 for all */
	uint32_t	which_details,	/* file properties to return */
	char		*restrictions,	/* filtering options */
	uint32_t	*morefiles,	/* does the directory have more? */
	sqm_lst_t	**results	/* list of details strings */
);

/*
 * collect_file_details()
 *
 * Companion function to list_and_collect_file_details, but used when the
 * caller already knows the list of files.
 */
int
collect_file_details(
	ctx_t		*c,
	char		*fsname,	/* filesystem name */
	char		*snappath,	/* could be NULL if ! Restore */
	char		*usedir,	/* directory containing the files */
	sqm_lst_t	*files,		/* list of files to process */
	uint32_t	which_details,	/* file properties to return */
	sqm_lst_t	**status	/* list of details strings */
);


/*
 * delete files
 *
 * Delete the list of files. The fully qualified paths must be
 * provided as input
 */
int
delete_files(
	ctx_t *c,
	sqm_lst_t *paths	/* list of file names (absolute paths) */
);


#endif /* _FSM_FILE_UTIL_H */
