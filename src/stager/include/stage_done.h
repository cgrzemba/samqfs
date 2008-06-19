/*
 * stage_done.h - support for stage done communication
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

#if !defined(STAGE_DONE_H)
#define	STAGE_DONE_H

#pragma ident "$Revision: 1.14 $"

/* Structures. */

typedef struct StageDoneInfo {
	pthread_mutex_t	sd_mutex;	/* protect access to data */
	pthread_cond_t	sd_cond;	/* cond set when data is available */
	int		sd_first;	/* start of completed file indices */
	int		sd_last;	/* last of completed file indices */
} StageDoneInfo_t;

#endif /* STAGE_DONE_H */
