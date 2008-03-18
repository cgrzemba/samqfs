/*
 *	mode_string.c
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

#pragma ident "$Revision: 1.5 $"


#include <sys/types.h>
#include <sys/stat.h>
#include "sam/types.h"
#include "sam/param.h"
#include "sam/fs/ino.h"

char *
mode_string(
	mode_t mode,			/* File mode */
	char str[])			/* Mode string */
{
	str[0] = '?';
	str[1] = (mode & S_IRUSR) ? 'r' : '-';		/* user */
	str[2] = (mode & S_IWUSR) ? 'w' : '-';
	str[3] = (mode & S_IXUSR) ? 'x' : '-';
	str[4] = (mode & S_IRGRP) ? 'r' : '-';		/* group */
	str[5] = (mode & S_IWGRP) ? 'w' : '-';
	str[6] = (mode & S_IXGRP) ? 'x' : '-';
	str[7] = (mode & S_IROTH) ? 'r' : '-';		/* world */
	str[8] = (mode & S_IWOTH) ? 'w' : '-';
	str[9] = (mode & S_IXOTH) ? 'x' : '-';
	str[10] = '\0';

	if (mode & S_ISUID) {
		str[3] = (mode & S_IXUSR) ? 's' : 'S';
	}
	if (mode & S_ISGID) {
		str[6] = (mode & S_IXGRP) ? 's' : 'S';
	}
	if (mode & S_ISVTX) {
		str[9] = (mode & S_IXOTH) ? 't' : 'T';
	}

	if (S_ISBLK(mode)) {
		str[0] = 'b';			/* block special file */
	} else if (S_ISCHR(mode)) {
		str[0] = 'c';			/* char special file */
	} else if (S_ISDIR(mode)) {
		str[0] = 'd';			/* directory */
	} else if (S_ISREG(mode)) {
		str[0] = '-';			/* regular file */
	} else if (S_ISLNK(mode)) {
		str[0] = 'l';			/* symbolic link */
	} else if (S_ISFIFO(mode)) {
		str[0] = 'p';			/* fifo */
	} else if (S_ISSOCK(mode)) {
		str[0] = 's';			/* socket */
	} else if (S_ISREQ(mode)) {
		str[0] = 'R';			/* removable media file */
	}

	return (str);
}


#if	FALSE

char *
ext_mode_string(
	mode_t mode,			/* Extension inode mode */
	char str[])			/* Mode string */
{
	str[0] = '?';
	str[1] = '-';
	str[2] = '-';
	str[3] = '-';
	str[4] = '-';
	str[5] = '-';
	str[6] = '-';
	str[7] = '-';
	str[8] = '-';
	str[9] = '-';
	str[10] = '\0';

	if (S_ISMVA(mode)) str[0] = 'M';	/* multivol inode extension */
	if (S_ISSLN(mode)) str[0] = 'S';	/* symlink inode extension */
	if (S_ISRFA(mode)) str[0] = 'R';	/* removable media ino ext */
	if (S_ISHLP(mode)) str[0] = 'H';	/* hardlink parent id ino ext */
	if (S_ISACL(mode)) str[0] = 'A';	/* access ctrl list ino ext */

	return (str);
}

#endif		/* FALSE */
