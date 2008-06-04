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
#pragma ident   "$Revision: 1.32 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <dirent.h>
#include "mgmt/private_file_util.h"
#include "pub/mgmt/error.h"
#include "sam/sam_trace.h"
#include "mgmt/util.h"
#include "pub/stat.h"
#include "pub/mgmt/file_util.h"
#include "mgmt/file_details.h"
#include "pub/mgmt/fsmdb_api.h"

/*
 * This function uses sam_stat which undefs some of the definitions in
 * sys/stat.h. It cannot therefore coexist with functions that require
 * those definitions.
 */

static int
collect_copy_status(
	struct sam_stat		*st,
	fildet_seg_t		*seg,
	fildet_seg_t		*summary,
	char			*file_path,
	uint32_t		which_details);

static int
collect_per_segment_status(struct sam_stat *st, fildet_seg_t *seg);

static int
collect_file_details_stat(
	char		*usedir,
	sqm_lst_t		*files,
	uint32_t	which_details,
	sqm_lst_t		**status);


static int
worm_details_to_str(
	filedetails_t *det,
	char *res,
	int len);


int
filedetails_from_stat(
	char		*fsname,
	char		*startDir,	/* starting point. NULL = rootdir */
	char		*startFile,	/* start from here if continuing */
	int32_t		howmany,	/* how many to return.  -1 for all */
	uint32_t	which_details,	/* file properties to return */
	char		*restrictions,	/* filtering options */
	uint32_t	*morefiles,
	sqm_lst_t	**results	/* list of filedetails_t structures */
);

int
filedetails_from_restore(
	char		*fsname,
	char		*snappath,	/* Snapshot to get the data from */
	char		*startDir,	/* starting point. NULL = rootdir */
	char		*startFile,	/* start from here if continuing */
	int32_t		howmany,	/* how many to return.  -1 for all */
	uint32_t	which_details,	/* file properties to return */
	char		*restrictions,	/* filtering options */
	uint32_t	*morefiles,
	sqm_lst_t	**results	/* list of filedetails_t structures */
);

void summarize_vsn_info(filedetails_t *details);

int
filedetails_to_string(
	uint32_t	which_details,
	sqm_lst_t		*details,
	sqm_lst_t		**results);

int
segmentdetails_to_string(
	filedetails_t	*details,
	uint32_t	which_details,
	char		*buf,
	int		buflen);

int
copydetails_to_string(
	fildet_seg_t	*segp,
	uint32_t	which_details,
	char		*buf,
	int		buflen);


extern void MinToStr(time_t chgtime, long num_mins,
    char *gtime, char *str);

#define	ADD_STR_TO_LIST(buf, lstp) { \
	int	len;						\
	char	*ptr;						\
								\
	if (buf[0] != '\0') {					\
		len = strlen(buf);				\
		if (buf[len - 1] == ',') {			\
			buf[len - 1] = '\0';			\
		}						\
		ptr = copystr(buf);				\
		if (ptr != NULL) {				\
			rval = lst_append(lstp, ptr);		\
			if (rval != 0) {			\
				free(ptr);			\
				ptr = NULL;			\
			}					\
		} else {					\
			rval = -1;				\
		}						\
	}							\
}

#define	ADD_DETAILS_TO_LIST(x, lstp) { \
	rval = lst_append(lstp, x);				\
	if (rval != 0) {					\
		free_file_details(x);				\
		break;						\
	}							\
}

/* OBSOLETE Get details about a file. */
int
get_extended_file_details(
ctx_t *c,		/* ARGSUSED */
sqm_lst_t *files,
uint32_t which_details,
sqm_lst_t **status)
{
	int	rval;

	if (ISNULL(files, status)) {
		Trace(TR_ERR, "get extended file details failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	rval = collect_file_details_stat("", files, which_details, status);

	return (rval);
}

/*
 * OBSOLETE
 *
 * This function returns information about the status and location of
 * the archive copies for the file specified in file_path.
 *
 * copy=%d
 *
 * damaged means the copy has been marked as damaged by the user with
 * the damage command or by the system as a result of a fatal error
 * when trying to stage. damaged copies ARE NOT available for staging
 *
 * inconsistent means that the file was modified while the archive copy was
 * being made. By default such a copy would not be allowed. However there is
 * a flag to the archive command that allows the archival of inconsistent
 * copies. inconsistent copies can only be staged after a samfsrestore. They
 * are not candidates for staging in a general case. However, knowledge
 * of their existence is certainly desireable to a user.
 *
 *
 * This function only returns aggregated information for segmented files
 * and thus will not return all fields for any segmented file.
 *
 * Likewise if any particular copy overflows into more than one vsn,
 * the information about the copy is aggregated.
 *
 * which_details use is reserved for the future.
 * flag				meaning
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
 *				is used the following subfields can be
 *				returned:
 */
int
get_copy_details(
ctx_t *c /* ARGSUSED */,
char *file_path,
uint32_t which_details /* ARGSUSED */,
sqm_lst_t **res) {

	int	rval;
	sqm_lst_t	*file_lst;
	sqm_lst_t	*det_lst;
	char	*ptr;
	char	*sep;
	char	*cpy;

	if (ISNULL(file_path, res)) {
		return (-1);
	}

	file_lst = lst_create();
	if (file_lst == NULL) {
		return (-1);
	}

	rval = lst_append(file_lst, file_path);
	if (rval != 0) {
		lst_free(file_lst);
		return (rval);
	}

	rval = collect_file_details_stat("", file_lst, which_details,
	    &det_lst);

	if (rval == 0) {
		/*
		 * collect returns a single string.  Callers of
		 * this function expect 1 string/copy
		 */
		ptr = (char *)(det_lst->head)->data;
		/* allocate results list */
		if (*res == NULL) {
			*res = lst_create();
		}
		/* if still NULL, create failed */
		if (*res == NULL) {
			rval = -1;
		}

		while ((ptr != NULL) && (rval == 0)) {
			sep = strstr(ptr, "###");
			if (sep != NULL) {
				*sep = '\0';
				cpy = copystr(ptr);
				ptr = sep + 3;
			} else {
				cpy = copystr(ptr);
				ptr = NULL;
			}

			if (cpy != NULL) {
				rval = lst_append(*res, cpy);
				if (rval != 0) {
					free(cpy);
				}
			} else {
				rval = -1;
			}
		}
		lst_free_deep(det_lst);
	}

	/* don't free the passed-in string, just the list struct */
	lst_free(file_lst);

	if ((rval != 0) && (*res != NULL)) {
		lst_free_deep(*res);
		*res = NULL;
	}
	return (rval);
}

/*
 *  New listing functions to support the file browser.
 */

int
list_and_collect_file_details(
	ctx_t		*c,		/* ARGSUSED */
	char		*fsname,	/* filesystem name */
	char		*snappath,	/* could be NULL if ! Restore */
	char		*startDir,	/* starting point. NULL = rootdir */
	char		*startFile,	/* start from here if continuing */
	int32_t		howmany,	/* how many to return.  -1 for all */
	uint32_t	which_details,	/* file properties to return */
	char		*restrictions,	/* filtering options */
	uint32_t	*morefiles,	/* does the directory have more? */
	sqm_lst_t		**results	/* list of details strings */
)
{
	int		st;
	sqm_lst_t		*rlist = NULL;

	if (ISNULL(fsname, results)) {
		return (-1);
	}

	*results = NULL;

	/*
	 * if snappath != NULL, restore mode
	 * else, live file data mode
	 */
	if ((!snappath) || (snappath[0] == '\0')) {
		st = filedetails_from_stat(fsname, startDir, startFile,
		    howmany, which_details, restrictions, morefiles,
		    &rlist);
	} else {
		st = filedetails_from_restore(fsname, snappath, startDir,
		    startFile, howmany, which_details, restrictions,
		    morefiles, &rlist);
	}

	if (st != 0) {
		if (samerrno != SE_NO_MEM) {
			samrerr(SE_NOT_A_DIR, startDir);
		}
		return (-1);
	}

	st = filedetails_to_string(which_details, rlist, results);

	/* change to typed */
	lst_free_deep_typed(rlist, FREEFUNCCAST(free_file_details));

	return (st);
}

int
filedetails_from_stat(
	char		*fsname,
	char		*startDir,	/* starting point. NULL = rootdir */
	char		*startFile,	/* start from here if continuing */
	int32_t		howmany,	/* how many to return.  -1 for all */
	uint32_t	which_details,	/* file properties to return */
	char		*restrictions,	/* filtering options */
	uint32_t	*morefiles,
	sqm_lst_t	**results	/* list of filedetails_t structures */
)
{
	int		st;
	sqm_lst_t		*flist = NULL;
	char		buf[MAXPATHLEN +1];
	char		*usedir = NULL;
	sqm_lst_t		*detlist = NULL;

	if (ISNULL(fsname, results)) {
		return (-1);
	}

	*results = NULL;

	buf[0] = '\0';

	/*
	 * fetch the mount point if the provided directory isn't fully
	 * qualified or not specified at all.
	 */
	if ((startDir == NULL) || (startDir[0] != '/')) {
		st = getfsmountpt(fsname, buf, sizeof (buf));
		if (st != 0) {
			/* try vfstab */
			st = getvfsmountpt(fsname, buf, sizeof (buf));
		}
		if (st != 0) {
			return (-1);	/* samerrno already set */
		}
		if (startDir[0] != '\0') {
			strlcat(buf, "/", sizeof (buf));
			strlcat(buf, startDir, sizeof (buf));
		}
		usedir = buf;
	} else {
		usedir = startDir;
	}

	st = list_directory(NULL, howmany, usedir, startFile, restrictions,
	    morefiles, &flist);

	if (st != 0) {
		return (-1);
	}

	st = collect_file_details_stat(usedir, flist, which_details,
	    &detlist);

	/* done with file list */
	lst_free_deep(flist);

	*results = detlist;

	return (st);
}

int
filedetails_from_restore(
	char		*fsname,
	char		*snappath,	/* Snapshot to get the data from */
	char		*startDir,	/* starting point. NULL = rootdir */
	char		*startFile,	/* start from here if continuing */
	int32_t		howmany,	/* how many to return.  -1 for all */
	uint32_t	which_details,	/* file properties to return */
	char		*restrictions,	/* filtering options */
	uint32_t	*morefiles,
	sqm_lst_t	**results	/* list of filedetails_t structures */
)
{
	int		st;
	restrict_t	filter;

	if (ISNULL(fsname, snappath, results)) {
		return (-1);
	}

	/*
	 * due to interlinking requirements, the api function to retrieve
	 * the snapshot files requires the results list to be pre-created.
	 */
	*results = lst_create();
	if (*results == NULL) {
		return (-1);
	}

	/* Set up wildcard restrictions */
	st = set_restrict(restrictions, &filter);
	if (st) {
		lst_free(*results);
		*results = NULL;
		return (st);
	}

	st = list_snapshot_files(fsname, snappath, startDir, startFile,
	    filter, which_details, howmany, FALSE, morefiles, *results);

	if (st != 0) {
		lst_free(*results);
		*results = NULL;
		samrerr(SE_NOT_A_DIR, startDir);
	}

	return (st);
}

/* Get details about a file. */
int
collect_file_details(
	ctx_t		*c,		/* ARGSUSED */
	char		*fsname,	/* filesystem name */
	char		*snappath,	/* could be NULL if ! Restore */
	char		*usedir,	/* directory containing the files */
	sqm_lst_t		*files,		/* list of files to examine */
	uint32_t	which_details,	/* file properties to return */
	sqm_lst_t		**status	/* list of details strings */
)
{
	int		st;
	sqm_lst_t		*details = NULL;

	if (ISNULL(fsname, files, status)) {
		return (-1);
	}

	if ((snappath == NULL) || (snappath[0] == '\0')) {
		char	buf[MAXPATHLEN + 1];
		char	*dirp = usedir;

		if ((dirp == NULL) || (dirp[0] == '\0')) {
			st = getfsmountpt(fsname, buf, sizeof (buf));
			if (st != 0) {
				return (-1);	/* samerrno already set */
			}
			dirp = buf;
		}

		st = collect_file_details_stat(
		    dirp, files, which_details, &details);
	} else {
		if (ISNULL(fsname, snappath)) {
			return (-1);
		}
		st = collect_file_details_restore(
		    fsname, snappath, usedir, files, which_details,
		    &details);
	}

	if (st == 0) {
		st = filedetails_to_string(which_details, details, status);
	}

	if (details != NULL) {
		lst_free_deep_typed(details, FREEFUNCCAST(free_file_details));
	}

	return (st);
}

int
collect_file_details_restore(
	char		*fsname,	/* filesystem name */
	char		*snappath,	/* could be NULL if ! Restore */
	char		*usedir,
	sqm_lst_t		*files,
	uint32_t	which_details,
	sqm_lst_t		**status)
{
	int		st;
	restrict_t	filter;
	char		*startFile;
	node_t		*node;

	if (ISNULL(fsname, snappath, files, status)) {
		return (-1);
	}

	/*
	 * due to interlinking requirements, the api function to retrieve
	 * the snapshot files requires the results list to be pre-created.
	 */
	*status = lst_create();
	if (*status == NULL) {
		return (-1);
	}

	/* we don't need to filter, but it keeps the db api simpler */
	memset(&filter, 0, sizeof (restrict_t));

	for (node = files->head; node != NULL; node = node->next) {
		startFile = (char *)node->data;

		st = list_snapshot_files(fsname, snappath, usedir, startFile,
		    filter, which_details, 1, TRUE, NULL, *status);

		if (st != 0) {
			break;
		}
	}
	if (st != 0) {
		lst_free(*status);
		*status = NULL;
		samrerr(SE_FILE_NOT_PRESENT, startFile);
	}

	return (st);
}

static int
collect_file_details_stat(
	char		*usedir,
	sqm_lst_t		*files,
	uint32_t	which_details,
	sqm_lst_t		**status)
{
	int		rval = 0;
	char		*fil;
	struct sam_stat	sbuf;
	struct sam_stat *sout = &sbuf;
	struct sam_stat *segstats;
	int		type_of_file = -1;
	filedetails_t	*details = NULL;
	node_t		*node;
	sqm_lst_t		*results;
	char		buf[MAXPATHLEN + 1];
	int		i;



	if (ISNULL(usedir, files, status)) {
		Trace(TR_ERR, "get extended file details failed: %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	*status = NULL;

	results = lst_create();
	if (results == NULL) {
		return (-1);
	}

	for (node = files->head; node != NULL; node = node->next) {
		fil = (char *)node->data;

		snprintf(buf, sizeof (buf), "%s/%s", usedir, fil);
		/* use sam_lstat() so broken links are handled correctly */
		rval = sam_lstat(buf, sout, sizeof (struct sam_stat));
		if (rval != 0) {
			rval = 0;
			continue;
		}

		details = mallocer(sizeof (filedetails_t));
		if (details == NULL) {
			lst_free_deep(results);
			return (-1);
		}
		memset(details, 0, sizeof (filedetails_t));

		/* save a malloc for the file name */
		details->file_name = fil;
		node->data = NULL;

		if (S_ISDIR(sout->st_mode)) {
			details->file_type = FTYPE_DIR;
		} else if (S_ISLNK(sout->st_mode)) {
			details->file_type = FTYPE_LNK;
		} else if (S_ISREG(sout->st_mode)) {
			details->file_type = FTYPE_REG;
		} else if (S_ISFIFO(sout->st_mode)) {
			details->file_type = FTYPE_FIFO;
		} else if (S_ISBLK(sout->st_mode) ||
		    S_ISCHR(sout->st_mode)) {
			details->file_type = FTYPE_SPEC;
		} else {
			details->file_type = FTYPE_UNK;
		}

		details->prot = sout->st_mode;
		details->user = sout->st_uid;
		details->group = sout->st_gid;

		/* get the flags for the file */
		collect_per_segment_status(sout, &(details->summary));

		/*
		 * If the file is not a sam file the rest of this
		 * is pointless so go to the next file.
		 */
		if (!SS_ISSAMFS(sout->attr)) {
			details->summary.flags |= FL_NOTSAM;
			ADD_DETAILS_TO_LIST(details, results);
			continue;
		}

		/* Add worm information */
		if (SS_ISREADONLY(sout->attr)) {
			details->worm_duration = sout->rperiod_duration;
			details->worm_start = sout->rperiod_start_time;
			details->summary.flags |= FL_WORM;
		}

		if (SS_ISRELEASE_A(sout->attr)) {
			details->samFileAtts |= RL_ATT_WHEN1COPY;
		} else if (SS_ISRELEASE_N(sout->attr)) {
			details->samFileAtts |= RL_ATT_NEVER;
		}
		if (SS_ISRELEASE_P(sout->attr)) {
			details->partialSzKB = sout->partial_size;
		}

		if (SS_ISSTAGE_A(sout->attr)) {
			details->samFileAtts |= ST_ATT_ASSOC;
		} else if (SS_ISSTAGE_N(sout->attr)) {
			details->samFileAtts |= ST_ATT_NEVER;
		}

		if (SS_ISARCHIVE_C(sout->attr)) {
			details->samFileAtts |= AR_ATT_CONCURRENT;
		}
		if (SS_ISARCHIVE_C(sout->attr)) {
			details->samFileAtts |= AR_ATT_INCONSISTENT;
		}
		if (SS_ISARCHIVE_N(sout->attr)) {
			details->samFileAtts |= AR_ATT_NEVER;
		}

		if (SS_ISSEGMENT_A(sout->attr)) {
			details->segStageAhead = sout->stage_ahead;
			details->segSzMB = sout->segment_size;
			/*
			 * This indicates wether the file will be segmented if
			 * it grows whereas a seg_cnt > 1 indicates only that
			 * the file has existing segments.
			 */
			details->summary.flags |= FL_SEGMENTED;
		}

		/*
		 * archdone is valid and interesting for directories
		 * but none of the other status is.
		 */
		if ((type_of_file == 0) && (SS_ISARCHDONE(sout->attr))) {
			details->summary.flags |= FL_ARCHDONE;
		}

		/*
		 * only collect status for files that can be archived
		 *  ? FIFO ?
		 */
		if (details->file_type == 0 || details->file_type == 4) {
			ADD_DETAILS_TO_LIST(details, results);
			continue;
		}

		/* determine if the file is segmented */
		details->segCount = NUM_SEGS(sout);
		if (SS_ISSEGMENT_F(sout->attr) && details->segCount > 1) {
			int segslen = (details->segCount) *
			    sizeof (struct sam_stat);

			segstats = (struct sam_stat *)mallocer(segslen);

			if (segstats == NULL) {
				Trace(TR_ERR,
				    "get extended file details failed: %d %s",
				    samerrno, samerrmsg);

				rval = -1;
				free_file_details(details);
				break;
			}
			memset((void *)segstats, 0, segslen);

			rval = sam_segment_stat(buf, segstats, segslen);

			if (rval) {
				/* fatal for this file, but not the others */
				Trace(TR_MISC,
				    "samsegstat failed %s :with %d",
				    buf, rval);
				free_file_details(details);
				rval = 0;
				continue;
			}

			details->segments = mallocer(details->segCount *
			    sizeof (fildet_seg_t));

			if (details->segments == NULL) {
				free_file_details(details);
				free(segstats);
				details = NULL;
				rval = -1;
				break;
			}
			memset(details->segments, 0,
			    details->segCount * sizeof (fildet_seg_t));

			for (i = 0; i < details->segCount; i++) {
				collect_per_segment_status(&(segstats[i]),
				    &(details->segments[i]));
			}
		}

		/* collect copy information for the file or its segments */
		if ((which_details & FD_COPY_SUMMARY) ||
		    (which_details & FD_COPY_DETAIL)) {

			/* for each segment */
			if (details->segCount >= 1) {
				for (i = 0; i < details->segCount; i++) {
					collect_copy_status(&(segstats[i]),
					    &(details->segments[i]),
					    &(details->summary),
					    buf, which_details);
				}
				if (which_details & FD_COPY_DETAIL) {
					summarize_vsn_info(details);
				}
			} else {
				/* for file */
				collect_copy_status(sout, &(details->summary),
				    NULL, buf, which_details);
			}
		}
		ADD_DETAILS_TO_LIST(details, results);
	}

	if (rval != 0) {
		lst_free_deep_typed(results, FREEFUNCCAST(free_file_details));
		results = NULL;
	}

	*status = results;

	return (rval);
}

/*
 * this function handles a non-segmented file too.
 */
static int
collect_per_segment_status(struct sam_stat *st, fildet_seg_t *seg)
{
	if (ISNULL(st, seg)) {
		return (-1);
	}

	/* each segment has distinct size and times */
	seg->size = st->st_size;
	seg->created = st->st_ctime;
	seg->modified = st->st_mtime;
	seg->accessed = st->st_atime;
	seg->segnum = st->segment_number;

	/*
	 * if partial is online and file is not offline
	 * the entire file is online
	 */
	if (SS_ISDAMAGED(st->attr)) {
		seg->flags |= FL_DAMAGED;
	}

	if (SS_ISOFFLINE(st->attr)) {
		seg->flags |= FL_OFFLINE;

		if (SS_ISPARTIAL(st->attr)) {
			seg->flags |= FL_PARTIAL;
		}
	}

	if (SS_ISARCHDONE(st->attr)) {
		seg->flags |= FL_ARCHDONE;
	}
	if (SS_ISSTAGING(st->flags)) {
		seg->flags |= FL_STAGEPEND;
	}

	return (0);
}

/*
 * This function returns information about the status and location of
 * the archive copies for the file specified in file_path.
 *
 * damaged means the copy has been marked as damaged by the user with
 * the damage command or by the system as a result of a fatal error
 * when trying to stage. damaged copies ARE NOT available for staging
 *
 * inconsistent means that the file was modified while the archive copy was
 * being made. By default such a copy would not be allowed. However there is
 * a flag to the archive command that allows the archival of inconsistent
 * copies. inconsistent copies can only be staged after a samfsrestore. They
 * are not candidates for staging in a general case. However, knowledge
 * of their existence is certainly desireable to a user.
 *
 */
static int
collect_copy_status(
	struct sam_stat		*st,
	fildet_seg_t		*seg,
	fildet_seg_t		*summary,
	char			*file_path,
	uint32_t		which_details)
{
	int		rval;
	int		copy;
	ushort_t	flags = (ushort_t)0;
	short		vsn_cnt;

	if (ISNULL(st, seg)) {
		Trace(TR_ERR, "getting copy details failed: %d %s",
		    samerrno, samerrmsg);

		return (-1);
	}

	/* For each copy get the info */
	for (copy = 0; copy < MAX_ARCHIVE; copy++) {

		flags = st->copy[copy].flags;

		if (!(flags & CF_ARCHIVED)) {
			continue;
		}
		if (flags & CF_STALE) {
			seg->copy[copy].flags |= FL_STALE;
			if (summary) {
				summary->copy[copy].flags |= FL_STALE;
			}
		}
		if (flags & CF_DAMAGED) {
			seg->copy[copy].flags |= FL_DAMAGED;
			if (summary) {
				summary->copy[copy].flags |= FL_DAMAGED;
			}
		}
		if (flags & CF_INCONSISTENT) {
			seg->copy[copy].flags |= FL_INCONSISTENT;
			if (summary) {
				summary->copy[copy].flags |= FL_INCONSISTENT;
			}
		}
		if (flags & CF_UNARCHIVED) {
			seg->copy[copy].flags |= FL_UNARCHIVED;
			if (summary) {
				summary->copy[copy].flags |= FL_UNARCHIVED;
			}
		}

		if (st->copy[copy].media[0] != '\0') {
			memcpy(seg->copy[copy].mediaType,
			    st->copy[copy].media, 4);
			if (summary &&
			    (summary->copy[copy].mediaType[0] == '\0')) {
				memcpy(summary->copy[copy].mediaType,
				    seg->copy[copy].mediaType, 4);
			}
		}

		seg->copy[copy].created = st->copy[copy].creation_time;
		if (summary &&
		    (summary->copy[copy].created <  seg->copy[copy].created)) {
			summary->copy[copy].created = seg->copy[copy].created;
		}

		/* get the VSN information only if requested */
		if (!(which_details & FD_COPY_DETAIL)) {
			continue;
		}

		/*
		 * If the copy overflows onto more than one vsn, use either
		 * sam_vsn_stat or sam_segment_vsn_stat
		 */
		vsn_cnt = st->copy[copy].n_vsns;

		if (vsn_cnt == 0) {
			/* should never happen */
			continue;
		} else {
			size_t	vsnsz = vsn_cnt * 32;
			char	vsnlist[vsnsz];

			/*
			 * the first VSN is always stored in the sam_stat
			 * struct
			 */
			strlcpy(vsnlist, st->copy[copy].vsn, vsnsz);

			if (vsn_cnt > 1) {
				int	j;
				struct sam_section *sects;
				size_t	sectsz = vsn_cnt *
				    sizeof (struct sam_section);

				sects = calloc(1, sectsz);
				if (sects == NULL) {
					setsamerr(SE_NO_MEM);
					Trace(TR_ERR,
					    "aggregate vsn info failed:%d %s",
					    samerrno, samerrmsg);
					return (-1);
				}

				if (SS_ISSEGMENT_S(st->attr)) {
					rval = sam_segment_vsn_stat(file_path,
					    copy, st->segment_number, sects,
					    sectsz);
				} else {
					rval = sam_vsn_stat(file_path, copy,
					    sects, sectsz);
				}

				if (rval != 0) {
					free(sects);
					setsamerr(SE_VSN_STAT_FAILED);
					Trace(TR_ERR,
					    "aggregate vsn info failed:%d %s",
					    samerrno, samerrmsg);
					return (-1);
				}

				for (j = 0; j < vsn_cnt; j++) {
					strlcat(vsnlist, " ", vsnsz);
					strlcat(vsnlist, sects[j].vsn, vsnsz);
				}

				free(sects);
			}

			if (vsnlist[strlen(vsnlist)] == ' ') {
				vsnlist[strlen(vsnlist)] = '\0';
			}

			seg->copy[copy].vsns = copystr(vsnlist);
		}

		if (seg->copy[copy].vsns == NULL) {
			return (-1);
		}
	}

	Trace(TR_MISC, "get copy info for %s", file_path);

	return (0);
}

int
filedetails_to_string(
	uint32_t	which_details,
	sqm_lst_t		*details,
	sqm_lst_t		**results)
{
	char		*funcnam = "file details to string";
	int		rval;
	int		total;
	node_t		*node;
	size_t		len = 2 * MAXPATHLEN;
	char		buf[len];
	char		*cur;
	char		tmpbuf[64];
	filedetails_t	*det;

	if (ISNULL(details, results)) {
		Trace(TR_ERR, "%s failed: %d %s",
		    funcnam, samerrno, samerrmsg);
		return (-1);
	}

	*results = lst_create();	/* Return results in this list */
	if (*results == NULL) {
		Trace(TR_ERR, "%s failed: %d %s",
		    funcnam, samerrno, samerrmsg);

		return (-1);	/* If allocation failed, samerr is set */
	}

	for (node = details->head; node != NULL; node = node->next) {
		buf[0] = '\0';
		cur = &(buf[0]);
		total = 0;

		det = (filedetails_t *)node->data;

		if (which_details & FD_FNAME) {
			if (strchr(det->file_name, ',') == NULL) {
				total += snprintf(cur, len,
				    "file_name=%s,", det->file_name);
			} else {
				/*
				 * commas in filenames break the kv parser if
				 * not enclosed in quotes
				 */
				total += snprintf(cur, len,
				    "file_name=\"%s\",", det->file_name);
			}
		}

		if (which_details & FD_FILE_TYPE) {
			total += snprintf(cur + total,
			    len - total,
			    "file_type=%d,", det->file_type);
		}

		if (which_details & FD_SIZE) {
			total += snprintf(cur + total,
			    len - total,
			    "size=%lld,", det->summary.size);
		}

		if (which_details & FD_CREATED) {
			total +=  snprintf(cur + total,
			    len - total, "created=%ld,",
			    det->summary.created);
		}

		if (which_details & FD_MODIFIED) {
			total +=  snprintf(cur + total,
			    len - total, "modified=%ld,",
			    det->summary.modified);
		}

		if (which_details & FD_ACCESSED) {
			total +=  snprintf(cur + total,
			    len - total, "accessed=%ld,",
			    det->summary.accessed);
		}

		/* mode is returned either as octal or as the "rwx" string */
		if (which_details & FD_MODE) {
			total +=  snprintf(cur + total,
			    len - total, "protection=%o,",
			    det->prot);
		} else if (which_details & FD_CHAR_MODE) {
			modeToString(det->prot, tmpbuf);

			total += snprintf(cur + total, len - total,
			    "cprotection=%s,", tmpbuf);
		}

		if (which_details & FD_USER) {
			get_user_name(det->user, tmpbuf, sizeof (tmpbuf));
			total +=  snprintf(cur + total, len - total,
			    "user=%s,", tmpbuf);
		}

		if (which_details & FD_GROUP) {
			get_group_name(det->group, tmpbuf, sizeof (tmpbuf));
			total +=  snprintf(cur + total, len - total,
			    "group=%s,", tmpbuf);
		}

		if (which_details & FD_WORM &&
		    det->summary.flags & FL_WORM) {

			total += worm_details_to_str(det, cur + total,
			    len - total);
		}

		/*
		 * If the file is not a sam file the rest of this
		 * is pointless so continue.
		 */
		if (det->summary.flags & FL_NOTSAM) {
			ADD_STR_TO_LIST(cur, *results);
			continue;
		}

		if (which_details & FD_RELEASE_ATTS) {
			if (det->samFileAtts & RL_ATT_MASK) {
				total +=  snprintf(cur + total,
				    len - total, "release_atts=%d,",
				    (det->samFileAtts & RL_ATT_MASK));
			}

			if (det->partialSzKB > 0) {
				total +=  snprintf(cur + total,
				    len - total,
				    "partial_release_sz=%ld,",
				    det->partialSzKB);
			}
		}

		if ((which_details & FD_STAGE_ATTS) &&
		    (det->samFileAtts & ST_ATT_MASK)) {
			total +=  snprintf(cur + total,
			    len - total,
			    "stage_atts=%d,", (det->samFileAtts & ST_ATT_MASK));
		}
		if ((which_details & FD_ARCHIVE_ATTS) &&
		    (det->samFileAtts & AR_ATT_MASK)) {
			total +=  snprintf(cur + total,
			    len - total, "archive_atts=%d,",
			    (det->samFileAtts & AR_ATT_MASK));
		}


		if ((which_details & FD_SEGMENT_ATTS) &&
		    (det->summary.flags & FL_SEGMENTED)) {
			total +=  snprintf(cur + total,
			    len - total,
			    "seg_size_mb=%ld,", det->segSzMB);

			total +=  snprintf(cur + total,
			    len - total,
			    "seg_stage_ahead=%ld,", det->segStageAhead);
		}


		/*
		 * archdone is valid and interesting for directories
		 * but none of the other status is.
		 */
		if ((det->file_type == 0) &&
		    (det->summary.flags & FL_ARCHDONE)) {
			total +=  snprintf(cur + total,
			    len - total,
			    "archdone=1,");
		}

		/*
		 * only collect status for files that can be archived
		 *  ? FIFO ?
		 */
		if (det->file_type == 0 || det->file_type == 4) {
			ADD_STR_TO_LIST(cur, *results);
			continue;
		}

		/* print status for the file or its segments */
		if (which_details & FD_SAM_STATE) {
			total += segmentdetails_to_string(
			    det, which_details, cur + total, len - total);
		}

		/* print copy summary */
		total += copydetails_to_string(&(det->summary),
		    which_details, cur + total, len - total);

		ADD_STR_TO_LIST(cur, *results);
	}

	return (0);
}

/*
 *  return value is number of bytes added to buf
 */
int
segmentdetails_to_string(
	filedetails_t	*details,
	uint32_t	which_details,
	char		*buf,
	int		buflen)
{
	int		damaged = 0;
	int		partial = 0;
	int		offline = 0;
	int		online = 0;
	int		archdone = 0;
	int		stage_pending = 0;
	int		added = 0;
	int		i;
	fildet_seg_t	*segp;
	int		segc;
	int		seglen = 1024;
	char		segbuf[seglen];
	int		a = 0;
	boolean_t	p = FALSE;

	if ((ISNULL(details, buf)) || (buflen <= 0)) {
		return (0);
	}

	/*
	 * if file is segmented, don't show the 'summary' flags/details.
	 * Those reflect the segment INDEX, and aren't particularly
	 * interesting.
	 */
	if (details->summary.flags & FL_SEGMENTED) {
		segp = details->segments;
		segc = details->segCount;
		added += snprintf(buf, buflen, "seg_count=%d,", segc);
		if (which_details & FD_SEGMENT_ALL) {
			p = TRUE;
		}
	} else {
		segp = &(details->summary);
		segc = 1;
	}

	/* Separate per-segment details with "|", segments with ":" */

	for (i = 0; i < segc; i++, segp++) {
		if ((p) && (i > 0)) {
			if (segbuf[a - 1] == ',') {
				segbuf[a - 1] = ':';
			}
		}
		if (p) {
			a += snprintf(segbuf + a, seglen -a,
			    "segment=%d", segp->segnum);
		}

		if (segp->flags & FL_DAMAGED) {
			damaged++;
			if (p) {
				a += snprintf(segbuf + a, seglen - a,
				    "|damaged=1");
			}
		}

		if (segp->flags & FL_PARTIAL) {
			partial++;
			if (p) {
				a += snprintf(segbuf + a, seglen - a,
				    "|partial_online=1");
			}
		} else if (segp->flags & FL_OFFLINE) {
			offline++;
			if (p) {
				a += snprintf(segbuf + a, seglen - a,
				    "|offline=1");
			}
		} else {
			online++;
			if (p) {
				a += snprintf(segbuf + a, seglen - a,
				"|online=1");
			}
		}

		if (segp->flags & FL_ARCHDONE) {
			archdone++;
			if (p) {
				a += snprintf(segbuf + a, seglen - a,
				    "|archdone=1");
			}
		}
		if (segp->flags & FL_STAGEPEND) {
			stage_pending++;
			a += snprintf(segbuf + a, seglen - a,
			    "|stage_pending=1");
		}

		/* get copy information if requested */
		if (p) {
			a += snprintf(segbuf + a, seglen - a, "|");
			a += copydetails_to_string(segp, which_details,
			    segbuf + a, seglen - a);
		}
	}

	if (a > 0) {
		added += snprintf(buf + added, buflen - added, "%s,",
		    segbuf);
	}

	/* add file-wide information to output string */
	if (damaged) {
		added += snprintf(buf + added, buflen - added, "damaged=%d,",
		    damaged);
	}
	if (partial) {
		added += snprintf(buf + added, buflen - added,
		    "partial_online=%d,", partial);
	}
	if (offline) {
		added += snprintf(buf + added, buflen - added, "offline=%d,",
		    offline);
	}
	if (online) {
		added += snprintf(buf + added, buflen - added, "online=%d,",
		    online);
	}
	if (archdone) {
		added += snprintf(buf + added, buflen - added, "archdone=%d,",
		    archdone);
	}
	if (stage_pending) {
		added += snprintf(buf + added, buflen - added,
		    "stage_pending=%d,", stage_pending);
	}

	return (added);
}

int
copydetails_to_string(
	fildet_seg_t	*segp,
	uint32_t	which_details,
	char		*buf,
	int		buflen)
{

	int		added = 0;
	int		i;
	int		clen = 1024;
	char		copybuf[clen];
	uint32_t	flags;

	if (!(which_details & FD_COPY_SUMMARY) &&
	    !(which_details & FD_COPY_DETAIL)) {
		/* no copy info requested */
		return (0);
	}

	for (i = 0; i < MAX_ARCHIVE; i++) {
		if (segp->copy[i].created == 0) {
			continue;
		}

		flags = segp->copy[i].flags;

		/* copy fields are separated with ###, copies with @@@ */
		if (added > 0) {
			added += snprintf(copybuf + added, clen - added, "@@@");
		}

		added += snprintf(copybuf + added, clen - added,
		    "copy=%d", i + 1);

		added += snprintf(copybuf + added, clen - added,
		    "###media=%s", segp->copy[i].mediaType);

		if (flags & FL_DAMAGED) {
			added += snprintf(copybuf + added, clen - added,
			    "###damaged=1");
		}
		if (flags & FL_STALE) {
			added += snprintf(copybuf + added, clen - added,
			    "###stale=1");
		}
		if (flags & FL_INCONSISTENT) {
			added += snprintf(copybuf + added, clen - added,
			    "###inconsistent=1");
		}
		if (flags & FL_UNARCHIVED) {
			added += snprintf(copybuf + added, clen - added,
			    "###unarchived=1");
		}

		if (which_details & FD_COPY_DETAIL) {
			added += snprintf(copybuf + added, clen - added,
			    "###created=%ld", segp->copy[i].created);

			if (segp->copy[i].vsns != NULL) {
				added += snprintf(copybuf + added,
				    clen - added, "###vsn=%s",
				    segp->copy[i].vsns);
			}
		}
	}

	/* add finished copy string to provided buffer */
	if (added > 0) {
		added = snprintf(buf, buflen, "%s", copybuf);
	}

	return (added);
}


void
free_file_details(filedetails_t *details) {
	int		i;
	int		j;
	fildet_seg_t	*segp;

	if (ISNULL(details)) {
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

			for (j = 0; j < MAX_ARCHIVE; j++) {
				if (segp->copy[j].vsns != NULL) {
					free(segp->copy[j].vsns);
				}
			}
		}
		free(details->segments);
	}

	/* free summary */
	segp = &(details->summary);
	for (j = 0; j < MAX_ARCHIVE; j++) {
		if (segp->copy[j].vsns != NULL) {
			free(segp->copy[j].vsns);
		}
	}

}

/*
 *  Pick out the unique VSNs from each segment and summarize them into
 *  the summary structure.
 */
void
summarize_vsn_info(filedetails_t *details)
{
	int		i;
	int		j;
	fildet_seg_t	*segp;
	int		vsnbuflen = 8192;
	char		vsnbuf[vsnbuflen];	/* holds 256 VSNs */
	char		buf[2048];
	char		*ptr;
	char		*rest;
	int		len;
	char		vsn[33];	/* max length of a single VSN + 1 */

	if (details->segments == NULL) {
		return;
	}

	vsnbuf[0] = '\0';

	for (i = 0; i < MAX_ARCHIVE; i++) {
		for (j = 0; j < details->segCount; j++) {
			segp = &(details->segments[j]);

			if (segp->copy[i].vsns == NULL) {
				continue;
			}

			strlcpy(buf, segp->copy[i].vsns, sizeof (buf));

			ptr = strtok_r(buf, " ", &rest);
			while (ptr != NULL) {
				snprintf(vsn, 33, "%s ", ptr);
				if (!strstr(vsnbuf, vsn)) {
					strlcat(vsnbuf, vsn, vsnbuflen);
				}
				ptr = strtok_r(NULL, " ", &rest);
			}
		}

		/* done, copy to summary */
		len = strlen(vsnbuf);
		if (len > 0) {
			if (vsnbuf[len - 1] == ' ') {
				vsnbuf[len - 1] = '\0';
			}
			details->summary.copy[i].vsns = copystr(vsnbuf);
		}
	}

}


static int
worm_details_to_str(
filedetails_t *det,
char *res,
int len) {

	int added = 0;

	if (ISNULL(det, res)) {
		Trace(TR_ERR, "Error in worm_details_to_str %s", samerrmsg);
		return (0);
	}

	if (det->file_type == FTYPE_DIR) {
		added += snprintf(res + added, len - added,
		    "worm=capable,");
		added += snprintf(res + added, len - added,
		    "worm_duration=%u,", det->worm_duration);
	} else {
		time_t current_time = time((time_t *)0);
		char ret_end[40];
		char ret_period[40];

		/*
		 * convert times to minutes to allow comparison.
		 */
		if (det->worm_duration == 0 ||
		    (det->worm_duration + det->worm_start/60) >
		    current_time/60) {
			added += snprintf(res + added, len - added,
			    "worm=active,");
		} else {
			added += snprintf(res + added, len - added,
			    "worm=expired,");
		}

		added += snprintf(res + added, len - added,
		    "worm_start=%u,", det->worm_start);

		if (det->worm_duration == 0) {
			added += snprintf(res + added, len - added,
			    "worm_duration=permanent,");
		} else {
			added += snprintf(res + added, len - added,
			    "worm_duration=%u,", det->worm_duration);

			MinToStr(det->worm_start, det->worm_duration,
			    ret_period, ret_end);

			added += snprintf(res + added, len - added,
			    "worm_end=%s,", ret_end);
		}
	}

	return (added);
}
