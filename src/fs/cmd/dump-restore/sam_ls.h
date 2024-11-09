/*
 *	sam_ls.h - Ids_ls interface definitions.
 *
 *	Interface definitions for ids_ls().
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
 * or https://illumos.org/license/CDDL.
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

#ifndef _DUMP_RESTORE_SAM_LS_H
#define	_DUMP_RESTORE_SAM_LS_H

#pragma ident "$Revision: 1.9 $"


#define	LS_FILE		0000		/* Print file name only	*/
#define	LS_LINE1	0001		/* Print 1 line long format */
#define	LS_LINE2	0002		/* Print 2 line long format */
#define	LS_INODES	0004		/* Print inode numbers */
#define	LS_CYCLE	0010		/* Print cycle information */
#define	LS_EXPTM	0020		/* Print expiration time */
#define	LS_ACL		0040		/* Print ACL */

extern	int	ls_options;		/* Options */
extern	time_t	ls_base_time;		/* Current time	*/

#endif /* _DUMP_RESTORE_SAM_LS_H */
