/*
 * log.h - collect rft events and write to log file
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

#if !defined(RFTD_LOG_H)
#define	RFTD_LOG_H

#pragma ident "$Revision: 1.9 $"

typedef struct LogInfo {
    FILE	*file;
} LogInfo_t;

extern LogInfo_t LogFile;

#define	IS_LOG_ENABLED() (LogFile.file != NULL)
#define	IS_NULL(x) (x != NULL ? x : "(nil)")

/* Functions */
void OpenLogFile(char *name);
void LogIt(Crew_t *crew);

#endif /* RFTD_LOG_H */
