/*
 * errorlog.h - definitions for error log destinations
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


#if !defined(_AML_ERRORLOG_H)
#define	_AML_ERRORLOG_H

#pragma ident "$Revision: 1.10 $"


typedef struct errlog {
	char	emsg_pfx[32];
	int	emsg_dest;
	FILE	*emsg_fp;
} errlog_t;

/* Values for emsg_dest  */

#define	TO_TTY	1
#define	TO_SYS	2
#define	TO_FILE	4
#define	TO_ALL	TO_TTY|TO_SYS|TO_FILE


#endif /* !defined(_AML_ERRORLOG_H) */
