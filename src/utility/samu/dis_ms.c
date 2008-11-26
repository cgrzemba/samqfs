/*
 * dis_ms.c - Display mass-storage status.
 *
 * Display the status of the mass-storage devices for the
 * specified mount point.
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

#pragma ident "$Revision: 1.29 $"

/* ANSI headers. */
#include <errno.h>
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

/* Local headers. */
#undef ERR
#include "samu.h"

/* Private functions. */
static int DisParts(struct sam_fs_info *fi, int first_part);

/* Private data. */
static int numfs;		/* Number of filesystems */
static int fsfirst = 0;		/* first filesystem on screen */
static int devfirst = 0;	/* first partition on screen */
static struct sam_fs_info   *finfo = NULL;

void
DisMs()
{
	struct sam_fs_status *fsarray;
	int i;
	int ismore;

	/* "ty      eq  status      use state ord" */
	Mvprintw(ln++, 0, GetCustMsg(7413));
	/* "   capacity       free    ra  part high low" */
	Printw(GetCustMsg(7334));

	if ((numfs = GetFsStatus(&fsarray)) == -1) {
		Error("GetFsStatus failed");
		/*NOTREACHED*/
	}

	if (finfo != NULL) free(finfo);
	finfo = (struct sam_fs_info *)malloc(numfs *
	    sizeof (struct sam_fs_info));


	for (i = 0; i < numfs; i++) {
		struct sam_fs_status *fs;
		struct sam_fs_info *fi;

		fs = fsarray + i;
		fi = finfo + i;
		if (GetFsInfo(fs->fs_name, fi) == -1) {
			Error("GetFsInfo(%s) failed", fs->fs_name);
			/*NOTREACHED*/
		}
		if (!(fi->fi_status & FS_MOUNTED)) {
			continue;
		}
	}
	free(fsarray);

	for (i = fsfirst; i < numfs; i++) {
		struct sam_fs_info *fi;
		char	cap[STR_FROM_FSIZE_BUF_SIZE];
		char	spc[STR_FROM_FSIZE_BUF_SIZE];
		char	rah[STR_FROM_FSIZE_BUF_SIZE];
		int	part;
		int	pct;	/* Disk free space percentage */

		if (ln > LINES - 3) {
			break;
		}
		fi = finfo + i;
		if (!(fi->fi_status & FS_MOUNTED)) {
			continue;
		}

		pct = llpercent_used(fi->fi_capacity, fi->fi_space);
		(void) StrFromFsize(fi->fi_capacity, 3, cap, sizeof (cap));
		(void) StrFromFsize(fi->fi_space, 3, spc, sizeof (spc));
		(void) StrFromFsize(fi->fi_readahead, 0, rah, sizeof (rah));
		Mvprintw(ln++, 0,
		    "%-4s %5d  %s%4d%% %-7s   %10s%10s%4s %5d ",
		    device_to_nm(fi->fi_type), fi->fi_eq,
		    StrFromFsStatus(fi), pct,
		    dev_state[fi->fi_state], cap, spc,
		    rah, fi->fi_partial);
		Printw("%3d%%% 3d%%", fi->fi_high, fi->fi_low);
		Clrtoeol();

		part = (i == fsfirst) ? devfirst : 0;
		ismore = DisParts(fi, part);
	}
#ifdef DeBug
		Mvprintw(LINES-2, 0, " fs %d dev %d nfs %d nd %d "
		    "ln %d thisd %d",
		    fsfirst, devfirst, nfsdevs, ndevs, ln, dev_thisfs);
#endif
	if (i >= numfs && ismore == 0) {  /* the end */
		Mvprintw(LINES-1, 0, "         ");
	} else {
		/* "     more" */
		Mvprintw(LINES-1, 0, GetCustMsg(2));
	}
}


/*
 * Display partition data.
 */
int
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
		char	spc[STR_FROM_FSIZE_BUF_SIZE];
		char	cap[STR_FROM_FSIZE_BUF_SIZE];
		int		pct;

		if (ln > LINES - 3) {
			break;
		}
		pt = ptarray + i;

		/*
		 * Compute and convert data.
		 */
		pct = llpercent_used(pt->pt_capacity, pt->pt_space);
		(void) StrFromFsize(pt->pt_capacity, 3, cap, sizeof (cap));
		(void) StrFromFsize(pt->pt_space, 3, spc, sizeof (spc));
		Mvprintw(ln, 0,
		    " %-4s %5d            %4d%% %-7s%3d%10s%10s",
		    device_to_nm(pt->pt_type), pt->pt_eq,
		    pct,
		    dev_state[pt->pt_state],
		    i, cap, spc);
		if (strcmp(device_to_nm(pt->pt_type), "mm") == 0) {
			Printw(" [%lld inodes]", pt->pt_space / SAM_ISIZE);
		}
		ln++;
		Clrtoeol();
	}
	free(ptarray);
	return (fi->fs_count - i);
}

/*
 * Keyboard processing.
 */

#define	MAX(X, Y) ((X) > (Y)? (X) : (Y))
#define	TLINES MAX(((LINES - 6)), 1)
#define	HLINES MAX(((LINES - 6)/2), 1)

boolean
KeyMs(char key)
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
			if (count - devfirst > n - 1) {
					/* more partitions than page */
				devfirst += n - 1;
					/* just scroll the partitions */
				return (TRUE);
			}
			if (fsfirst == numfs - 1) {	/* the last fs */
				return (FALSE);
			}
			/* scroll to next fs */
			n -= count + 1;
			fsfirst++;
			devfirst = 0;
		}
		break;

	case KEY_full_bwd:
		for (n = TLINES; n > 0; ) {
			if (fsfirst <= 0)
				return (FALSE);
			count = devfirst + 1;
			if (n  <= count) {
					/* partway in current fs */
				devfirst -= n - 1;
				return (TRUE);
			}
			count = (finfo+fsfirst-1)->fs_count + 1;
					/* lines to prev. fs */
			if (count == 1) break;
			if (n <= count) {
					/* partway in prev. fs */
				fsfirst--;
				devfirst = count - n;
					/* just scroll the partitions */
				return (TRUE);
			}
			/* scroll to previous fs */
			n -= count;
			fsfirst--;
			devfirst = 0;
		}
		break;

	case KEY_half_bwd:
	case KEY_half_fwd:
	default:
		return (FALSE);
		/* LINTED statement not reached */
		break;
	}
	return (TRUE);
}
