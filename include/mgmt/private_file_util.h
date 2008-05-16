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

#ifndef _FSM_PVT_FILE_UTIL_H
#define	_FSM_PVT_FILE_UTIL_H

#pragma ident	"$Revision: 1.9 $"

/*
 * private_file_util.h - Utilities for manipulating files and directories
 */


#include <sys/param.h>
#include <sys/stat.h>

/* Structure to hold restrictions entry */
typedef struct restrict_s {
	char filename[MAXPATHLEN];
	uid_t user;
	gid_t gid;
	off64_t biggerthan;
	off64_t smallerthan;
	time_t modbefore;
	time_t modafter;
	time_t creatbefore;
	time_t creatafter;
	int online;
	int damaged;
	int inconsistent;
	int flags;
} restrict_t;

/* Definitions of restriction-in-force flags */
#define	fl_user		0x1
#define	fl_gid		0x2
#define	fl_biggerthan	0x4
#define	fl_smallerthan	0x8
#define	fl_modbefore	0x10
#define	fl_modafter	0x20
#define	fl_creatbefore	0x40
#define	fl_creatafter	0x80
#define	fl_filename	0x100
#define	fl_online	0x200
#define	fl_offline	0x400
#define	fl_damaged	0x800
#define	fl_undamaged	0x1000
#define	fl_inconsistent	0x2000
#define	fl_consistent	0x4000

/* Group flags */
#define	flg_ownsiz	(fl_user|fl_gid|fl_biggerthan|fl_smallerthan)
#define	flg_dates	(fl_modbefore|fl_modafter|fl_creatbefore|fl_creatafter)
#define	flg_state	(fl_online|fl_offline|fl_damaged|fl_undamaged| \
				fl_inconsistent|fl_consistent)

/*
 *  Function Prototypes
 */

int set_restrict(char *restrictions, restrict_t *filterp);

int check_restrict_stat(char *name, struct stat64 *ptr, restrict_t *filterp);

/*
 * check that file can/does exist at path specified.
 * if dir_is_suff is set to true verify will pass if the directory exists
 */
boolean_t verify_file(const char *file, boolean_t dir_is_suff);

#endif /* _FSM_PVT_FILE_UTIL_H */
