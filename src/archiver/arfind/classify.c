/*
 * classify.c - Classify a file for archival.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.7 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <stdio.h>
#include <stdlib.h>

/* POSIX headers. */
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <libgen.h>

/* Local headers. */
#include "arfind.h"
#include "dir_inode.h"
#if defined(lint)
#undef snprintf
#endif /* defined(lint) */


/*
 * Classify a file.
 * Determine which set of file properties that it matches.
 */
struct FilePropsEntry *
ClassifyFile(
	char *path,
	struct sam_perm_inode *pinode,	/* Permanent inode */
	sam_time_t *checkTime)
{
	struct sam_disk_inode *dinode = (struct sam_disk_inode *)pinode;
	struct FilePropsEntry *fp;
	boolean_t accessTimeFixed;
	int	fmode;
	int	i;

	/*
	 * Classify files by type.
	 */
	accessTimeFixed = FALSE;
	fmode = dinode->mode & S_IFMT;
	if (fmode == S_IFDIR || S_ISSEGI(dinode) ||
	    (dinode->version == SAM_INODE_VERS_1 &&
	    (fmode == S_IFREQ || fmode == S_IFLNK))) {

		/*
		 * Meta data - directories and segment index inodes.
		 * Plus pre 4.0 removable media files and symbolic links.
		 *   These were stored as meta data blocks.
		 *   In 4.0, the information is stored in inode extension(s).
		 */
		if (dinode->version != SAM_INODE_VERS_1 && S_ISSEGI(dinode)) {
			/*
			 * Segment index inode archiving depends on a kludge:
			 * Modification of the segment index inode is indicated
			 * by copy 1 stale bit, and creation time.
			 */
			if (dinode->ar_flags[0] & AR_stale) {
				dinode->modify_time.tv_sec =
				    pinode->ar.image[0].creation_time;
			} else {
				dinode->modify_time.tv_sec = TIME_NOW(dinode);
			}
		}
		fp = &FileProps->FpEntry[0];
		if (State->AfFlags & ASF_archivemeta) {
			/*
			 * Archiving meta data.
			 * Clear all the directory inherited flags.
			 */
			dinode->status.b.noarch  = 0;
			dinode->status.b.release = 0;
			{	/* Clear all archive immediate flags. */

		/* N.B. Bad indentation to meet cstyle requirements */
		uint_t *af =
		    (uint_t *)(void *)&dinode->ar_flags[0];
		*af &=
		    ~((AR_arch_i<<24)|(AR_arch_i<<16)|(AR_arch_i<<8)|AR_arch_i);

			}
			return (fp);
		} else if (EXAM_MODE(dinode) != EXAM_DIR || !S_ISSEGI(dinode)) {
			return (NULL);
		}
		/*
		 * A directory scan and a segmented file index.  Continue
		 * to check the file.  All data segments will be checked.
		 */
	}

	if (fmode != S_IFREG && fmode != S_IFLNK) {
		return (NULL);
	}

	/*
	 * Regular files and symbolic links.
	 * Search for matching file properties.
	 * Skip metadata file properties.
	 */
	for (i = 1; i < FileProps->FpCount; i++) {
		/*
		 * Compare the directory path to the file properties path.
		 * A empty file properties path will match all directories.
		 */
		fp = &FileProps->FpEntry[i];
		if (fp->FpPathSize != 0) {
			char	*pp, *dp;

			dp = path;
			if (dp[fp->FpPathSize] != '/') {
				continue;
			}
			pp = fp->FpPath;
			while (*pp != '\0' && *pp == *dp) {
				pp++;
				dp++;
			}
			if (*pp != '\0' || *dp != '/') {
				continue;
			}
		}
		if (!(fp->FpFlags & FP_props)) {
			/* No file properties to check. */
			break;
		}
		if ((fp->FpFlags & FP_name) &&
		    regex(fp->FpRegexp, path, NULL) == NULL) {
			continue;
		}
		Trace(TR_DEBUG, "Regexp match %s __loc1 %s", path, __loc1);
		if ((fp->FpFlags & FP_uid) && dinode->uid != fp->FpUid) {
			continue;
		}
		if ((fp->FpFlags & FP_gid) && dinode->gid != fp->FpGid) {
			continue;
		}
		if (fp->FpFlags & FP_minsize) {
			if (!S_ISSEGS(dinode) &&
			    dinode->rm.size < fp->FpMinsize) {
				continue;
			}
			if (S_ISSEGS(dinode) &&
			    *SEGFILE_SIZE(dinode) < fp->FpMinsize) {
				continue;
			}
		}
		if (fp->FpFlags & FP_maxsize) {
			if (!S_ISSEGS(dinode) &&
			    dinode->rm.size >= fp->FpMaxsize) {
				continue;
			}
			if (S_ISSEGS(dinode) &&
			    *SEGFILE_SIZE(dinode) >= fp->FpMaxsize) {
				continue;
			}
		}
		if (fp->FpFlags & FP_access) {
			time_t	accessRef;

			/*
			 * Check access time.
			 */
			if (!(fp->FpFlags & FP_nftv)) {
				/*
				 * Adjust file time for implausible times -
				 * too far in the past or future.
				 */
				if (dinode->access_time.tv_sec <
				    dinode->creation_time) {
					dinode->access_time.tv_sec =
					    dinode->creation_time;
				}
				if (dinode->access_time.tv_sec >
				    TIME_NOW(dinode)) {
					dinode->access_time.tv_sec =
					    dinode->change_time.tv_sec;
				}
				if (dinode->access_time.tv_sec < 0) {
					dinode->access_time.tv_sec = 0;
				}
				accessTimeFixed = TRUE;
			}
			accessRef = dinode->access_time.tv_sec + fp->FpAccess;
			if (accessRef > TIME_NOW(dinode)) {
				if (State->AfExamine >= EM_noscan &&
				    accessRef < *checkTime) {
					*checkTime = accessRef;
				}
				continue;
			}
		}
		if (fp->FpFlags & FP_after) {
			if (dinode->modify_time.tv_sec < fp->FpAfter &&
			    dinode->creation_time < fp->FpAfter) {
				continue;
			}
		}
		break;
	}
	if (i >= FileProps->FpCount) {
		/* No rules found */
		return (NULL);
	}

	if (fp->FpFlags & FP_noarch) {
		/* A "no_archive" Archive Set */
		Trace(TR_DEBUG, "Regexp no_archive match %s", path);
		return (fp);
	}

	if (fp->FpFlags & FP_default &&
	    !(FileProps->FpEntry[0].FpFlags & FP_noarch)) {
		/*
		 * Use the metadata entry.
		 * This will keep the data and meta data together.
		 */
		fp = &FileProps->FpEntry[0];
	}
	if (!(fp->FpFlags & FP_nftv)) {
		/*
		 * Adjust modification time for implausible times -
		 * too far in the past or future.
		 */
		if (dinode->modify_time.tv_sec < dinode->creation_time) {
			dinode->modify_time.tv_sec = dinode->creation_time;
		}
		if (dinode->modify_time.tv_sec > TIME_NOW(dinode)) {
			dinode->modify_time.tv_sec = dinode->change_time.tv_sec;
		}
		if (dinode->modify_time.tv_sec < 0) {
			dinode->modify_time.tv_sec = 0;
		}
		if (!accessTimeFixed) {
			/*
			 * Adjust access time for implausible times -
			 * too far in the past or future.
			 */
			if (dinode->access_time.tv_sec <
			    dinode->modify_time.tv_sec) {
				dinode->access_time.tv_sec =
				    dinode->modify_time.tv_sec;
			}
			if (dinode->access_time.tv_sec <
			    dinode->creation_time) {
				dinode->access_time.tv_sec =
				    dinode->creation_time;
			}
			if (dinode->access_time.tv_sec > TIME_NOW(dinode)) {
				dinode->access_time.tv_sec =
				    dinode->change_time.tv_sec;
			}
			if (dinode->access_time.tv_sec < 0) {
				dinode->access_time.tv_sec = 0;
			}
		}
	}
	return (fp);
}


/*
 * Initialize module.
 */
void
ClassifyInit(void)
{
	int	i;

	/*
	 * Compile regular expression for -name.
	 */
	for (i = 1; i < FileProps->FpCount; i++) {
		struct FilePropsEntry *fp;

		fp = &FileProps->FpEntry[i];
		if (fp->FpFlags & FP_name) {
			fp->FpRegexp = regcmp(fp->FpName, NULL);
			if (fp->FpRegexp == NULL) {
				Trace(TR_DEBUG, "Regexp error %s", fp->FpName);
				fp->FpFlags &= ~FP_name;
			}
		} else {
			fp->FpRegexp = NULL;
		}
	}
}
