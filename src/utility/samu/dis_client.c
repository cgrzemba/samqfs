/*
 * dis_client.c - Display file system shared client status.
 *
 * Display the status of the clients of a specified file system.
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

#pragma ident "$Revision: 1.7 $"

/* ANSI headers. */
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <fcntl.h>
#include <unistd.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/param.h"
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

/* External data and functions - see dis_mount.c */
extern sam_flagtext_t confTable[];
extern sam_flagtext_t conf1Table[];
extern sam_flagtext_t statusTable[];
extern int confTableSize;
extern int conf1TableSize;
extern int statusTableSize;

extern void
PrintFlags(uint32_t field, sam_flagtext_t *table, int ntable, char *label);

/* Private data. */
static int numfs;		/* Number of filesystems */
static int details = 0;		/* Detailed display: 1 = show flag names */
static int fsfirst = 0;		/* first filesystem on screen */
static int clifirst = 0;	/* first client on screen */
static int ncli = 0;		/* number of  clients for current fs */
static struct sam_fs_info   *finfo = NULL;
static struct sam_client_info 		*fclient = NULL;

/*
 *  bit definitions - see src/fs/include/client.h
 */
#define	SAM_CLIENT_DEAD 0x01    /* Client assumed dead during failover */
#define	SAM_CLIENT_INOP 0x02    /* Don't hold up failover (client known dead) */

static sam_flagtext_t cflagsTable[] = {
    SAM_CLIENT_DEAD,  "DEAD",   /* Client assumed dead during failover */
    SAM_CLIENT_INOP,  "INOP",	/* Don't hold up failover (client known dead) */
};
static int cflagsTableSize = sizeof (cflagsTable)/sizeof (sam_flagtext_t);

void
DisClients()
{
	struct sam_fs_status *fsarray;
	int		i, j;

	if ((numfs = GetFsStatus(&fsarray)) == -1) {
		Error("GetFsStatus failed");
		/*NOTREACHED*/
	}

	if (finfo != NULL) free(finfo);
	finfo = (struct sam_fs_info *)
	    malloc(numfs * sizeof (struct sam_fs_info));


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

	fclient = malloc(SAM_MAX_SHARED_HOSTS * sizeof (sam_client_info_t));
	ln = 2;

	for (i = fsfirst; i < numfs; i++) {
		struct sam_fs_info *fi;

		if (ln > LINES - 3) {
			break;
		}
		fi = finfo + i;

		if (fi->fi_config1 & MC_SHARED_FS) {
			struct sam_get_fsclistat_arg arg;
			sam_client_info_t *fc;

			strncpy(arg.fs_name, fi->fi_name, sizeof (arg.fs_name));
			arg.maxcli = SAM_MAX_SHARED_HOSTS;
			arg.numcli = 0;
			arg.fc.ptr = fclient;

			if (sam_syscall(SC_getfsclistat, &arg,
			    sizeof (arg)) < 0) {
				Mvprintw(ln++, 0, "SC_getfsclistat failed");
				return;
			}
			ncli = arg.maxcli;
			if (*fi->fi_server == '\0') {
				continue;
			}
			Mvprintw(ln++, 0, catgets(catfd, SET, 7420,
			    "%s is shared, server is %s, %d clients %d max"),
			    fi->fi_name, fi->fi_server, arg.numcli, arg.maxcli);
			Mvprintw(ln++, 0, catgets(catfd, SET, 7421,
"ord hostname              seqno nomsgs status   config  conf1  flags"));

			for (j = clifirst; j < ncli; j++) {
				char extra[17];

				if (ln > LINES - 3) {
					break;
				}
				fc = fclient + j;

				if (fc->cl_status == 0) {
					continue;
				}

				if (fc->cl_status & FS_MOUNTED) {
					strcpy(extra, "MNT ");
				} else {
					strcpy(extra, "    ");
				}
				if (fc->cl_status & FS_SERVER) {
					strcpy(extra+4, "SVR ");
				} else {
					strcpy(extra+4, "CLI ");
				}
				if (fc->cl_flags & SAM_CLIENT_DEAD) {
					strcpy(extra+8, "DEAD");
				} else {
					strcpy(extra+8, "    ");
				}
				if (fc->cl_flags & SAM_CLIENT_INOP) {
					strcpy(extra+12, "GONE");
				} else {
					strcpy(extra+12, "    ");
				}
				Mvprintw(ln++, 0,
				    "%3d %-20s %6d %6d %6x %8x %6x %6x %s",
				    j+1, fc->hname,
				    fc->cl_min_seqno, fc->cl_nomsg,
				    fc->cl_status,
				    fc->cl_config, fc->cl_config1,
				    fc->cl_flags, extra);

				if (details == 1 || ScreenMode == FALSE) {
					if (ln > LINES - 8) {
						break;
					}
					PrintFlags(fc->cl_config, confTable,
					    confTableSize, "config");
					PrintFlags(fc->cl_config1, conf1Table,
					    conf1TableSize, "config1");
					PrintFlags(fc->cl_flags, cflagsTable,
					    cflagsTableSize, "flags");
					PrintFlags(fc->cl_status, statusTable,
					    statusTableSize, "status");
					Mvprintw(ln++, 0, "");
				}
			}
			Mvprintw(ln++, 0, "");
		}
	}

	free(fclient);

#ifdef DeBug
	Mvprintw(LINES-2, 0, " fs %d part %d nfs %d np %d ln %d ",
	    fsfirst, clclist, numfs, 0, ln);
#endif

	if (i >= numfs) {  /* the end */
		Mvprintw(LINES-1, 0, "         ");
	} else {
		Mvprintw(LINES-1, 0, catgets(catfd, SET, 2, "     more"));
	}
}


/*
 * Initialization for client display.
 */
boolean
InitClients(
void)
{
	return (TRUE);
}

/*
 * Keyboard processing.
 */

#define	MAX(X, Y) ((X) > (Y)? (X) : (Y))
#define	TLINES MAX(((LINES - 6)), 1)
#define	HLINES MAX(((LINES - 6)/8), 1)

boolean
KeyClients(
char key)
{
	int n;
	int count;
	int tlines;

	if (finfo == NULL) {
		return (FALSE);
	}
	if (details > 0) {
		tlines = HLINES;
	} else {
		tlines = TLINES;
	}

	switch (key) {

	case KEY_full_fwd:
		for (n = TLINES; n > 0; ) {
			if (fsfirst >= numfs) {
				return (FALSE);
			}
			/* lines to next fs */
			count = (finfo+fsfirst)->fs_count;
			if (fsfirst == numfs - 1) {
				/* the last fs */
				return (FALSE);
			}
			/* scroll to next fs */
			n -= count + 1;
			fsfirst++;
			clifirst = 0;
		}
		break;

	case KEY_full_bwd:
		for (n = TLINES; n > 0; ) {
			if (fsfirst <= 0) {
				return (FALSE);
			}
			/* lines to previous fs */
			count = (finfo+fsfirst)->fs_count + 1;
			if (count == 1) break;
			/* scroll to previous fs */
			n -= count;
			fsfirst--;
			clifirst = 0;
		}
		break;

	case KEY_details:
		if (++details > 1) details = 0;
		break;

	case KEY_half_bwd:
		if ((clifirst -= tlines) < 0) clifirst = 0;
		break;
	case KEY_half_fwd:
		if ((clifirst += tlines) > ncli) clifirst = 0;
		break;
	default:
		return (FALSE);
	}
	return (TRUE);
}
