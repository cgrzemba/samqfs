/*
 * exit.h - some common exit values, extend the 2 in stdlib.h
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

#ifndef EXIT_H
#define	EXIT_H

/* Marcros and defines */
#define	EXIT_USAGE	2	/* Problem with arguments */
#define	EXIT_NORESTART	3	/* Do not restart, will just fail again */
#define	EXIT_FATAL	4	/* Bad error, may fail again */
#define	EXIT_NOMEM	5	/* Out of memory, could fail again */
#define	EXIT_RETRY	6	/* Caller should retry command */
#define	EXIT_NOFS	7	/* Let caller to know no FS configured */
#define	EXIT_NOMDS	8	/* Let caller know MDS is not mounted */

#endif /* EXIT_H */
