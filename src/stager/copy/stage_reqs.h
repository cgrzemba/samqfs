/*
 * stage_reqs.h - support for mapping in stage requests from daemon
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

#if !defined(STAGE_REQS_H)
#define	STAGE_REQS_H

#pragma ident "$Revision: 1.15 $"

/* SAM-FS headers.h */
#include "aml/stager_defs.h"

/* Local headers. */
#include "file_defs.h"

typedef int	reqid_t;	/* stage request identifier */

/* Structures. */

/* Functions */
FileInfo_t *GetFile(reqid_t id);
void SetStageDone(FileInfo_t *file);
void MapInRequestList();
void UnMapRequestList();

#endif /* STAGE_REQS_H */
