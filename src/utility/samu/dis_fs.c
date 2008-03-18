/*
 * dis_fs.c - Display file system status.
 *
 * Display the status of the file system for the
 * specified mount point.
 *
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.23 $"

/* ANSI headers. */
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <fcntl.h>
#include <unistd.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/devnm.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/syscall.h"
#include "sam/uioctl.h"
#include "sam/fs/inode.h"
#include "sam/fs/sblk.h"
#include "sam/nl_samfs.h"

/* Local headers. */
#undef ERR
#include "samu.h"

/* Private data. */
static int DisParts(struct sam_fs_info *fi, int first_part);

/* Private data. */
static int numfs;			/* Number of filesystems */
static int fsfirst = 0;		/* first filesystem on screen */
static int partfirst = 0;	/* first partition on screen */
static struct sam_fs_info   *finfo = NULL;

void
DisFs()
{
	struct sam_fs_status *fsarray;
	int		i;
	int ismore;

	Mvprintw(ln++, 0, GetCustMsg(7412));

/* N.B. Bad indentation to meet cstyle requirements */
/*
 * "ty     eq state   device_name            status high low mountpoint server"
 */
	if ((numfs = GetFsStatus(&fsarray)) == -1) {
		Error("GetFsStatus failed");
		/*NOTREACHED*/
	}

	if (finfo != NULL) free(finfo);
	finfo =
	    (struct sam_fs_info *)malloc(numfs * sizeof (struct sam_fs_info));


	for (i = 0; i < numfs; i++) {
		struct sam_fs_status *fs;
		struct sam_fs_info *fi;

		fs = fsarray + i;
		fi = finfo + i;
		if (GetFsInfo(fs->fs_name, fi) == -1) {
			Error("GetFsInfo(%s) failed", fs->fs_name);
			/*NOTREACHED*/
		}
	}
	free(fsarray);

	for (i = fsfirst; i < numfs; i++) {
		struct sam_fs_info *fi;
		int		part;

		if (ln > LINES - 3) {
			break;
		}
		fi = finfo + i;

		Mvprintw(ln++, 0,
		    "%-4s %5d %7s %20s %12s %2d%% %2d%% %-10s %-6s",
		    device_to_nm(fi->fi_type), fi->fi_eq,
		    dev_state[fi->fi_state],
		    fi->fi_name,
		    StrFromFsStatus(fi),
		    fi->fi_high, fi->fi_low,
		    fi->fi_mnt_point, fi->fi_server);

		part = (i == fsfirst) ? partfirst : 0;
		ismore = DisParts(fi, part);
	}

#ifdef DeBug
	Mvprintw(LINES-2, 0, " fs %d part %d nfs %d np %d ln %d ",
	    fsfirst, partfirst, numfs, 0, ln);
#endif

	if (i >= numfs && ismore == 0) {  /* the end */
		Mvprintw(LINES-1, 0, "         ");
	} else {
		Mvprintw(LINES-1, 0, catgets(catfd, SET, 2, "     more"));
	}
}


/*
 * Display partition data.
 */
static int
DisParts(struct sam_fs_info *fi, int first_part)
{
	struct sam_fs_part *ptarray;
	int		i;

	ptarray = (struct sam_fs_part *)malloc(
	    fi->fs_count * sizeof (struct sam_fs_part));
	if (ptarray == NULL) {
		Error("no memory for partitions");
		/*NOTREACHED*/
	}
	if (GetFsParts(fi->fi_name, fi->fs_count, ptarray) == -1) {
		free(ptarray);
		Error("GetFsParts(%s) failed.", fi->fi_name);
		/*NOTREACHED*/
	}

	for (i = first_part; i < fi->fs_count; i++) {
		struct sam_fs_part *pt;

		if (ln > LINES - 3) {
			break;
		}
		pt = ptarray + i;
		Mvprintw(ln++, 0, " %-4s %5d %7s %20s ",
		    device_to_nm(pt->pt_type),
		    pt->pt_eq, dev_state[pt->pt_state],
		    pt->pt_name);
	}
	Clrtoeol();
	free(ptarray);
	return (fi->fs_count - i);
}

/*
 * Initialization for filesystem display.
 * f [filesystem] TBD.
 */
boolean
InitFs(void)
{
	return (TRUE);
}

/*
 * Keyboard processing.
 */

#define	MAX(X, Y) ((X) > (Y)? (X) : (Y))
#define	TLINES MAX(((LINES - 6)), 1)
#define	HLINES MAX(((LINES - 6)/2), 1)

boolean
KeyFs(char key)
{
	int n;
	int count;

	if (finfo == NULL)
		return (FALSE);

	switch (key) {

	case KEY_full_fwd:
		for (n = TLINES; n > 0; ) {
			if (fsfirst >= numfs)
				return (FALSE);
			count = (finfo+fsfirst)->fs_count;
					/* lines to next fs */
			if (count - partfirst > n - 1) {
					/* more partitions than page */
				partfirst += n - 1;
					/* just scroll the partitions */
				return (TRUE);
			}
			if (fsfirst == numfs - 1) {
					/* the last fs */
				return (FALSE);
			}
			/* scroll to next fs */
			n -= count + 1;
			fsfirst++;
			partfirst = 0;
		}
		break;

	case KEY_full_bwd:
		for (n = TLINES; n > 0; ) {
			if (fsfirst <= 0)
				return (FALSE);
			count = partfirst + 1;
			if (n <= count) {	/* partway in current fs */
				partfirst -= n - 1;
						/* just scroll the partitions */
				return (TRUE);
			}
			count = (finfo+fsfirst)->fs_count + 1;
						/* lines to previous fs */
			if (count == 1) break;
			if (n <= count) {	/* partway in previous fs */
				fsfirst--;
				partfirst =  count - n;
						/* just scroll the partitions */
				return (TRUE);
			}
			/* scroll to previous fs */
			n -= count;
			fsfirst--;
			partfirst = 0;
		}
		break;

	case KEY_half_bwd:
	case KEY_half_fwd:
	default:
		return (FALSE);
	}
	return (TRUE);
}
