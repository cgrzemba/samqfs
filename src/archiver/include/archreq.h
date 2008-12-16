/*
 * archreq.h - Archive requests private definitions.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#ifndef PRIVATE_ARCHREQ_H
#define	PRIVATE_ARCHREQ_H

#pragma ident "$Revision: 1.31 $"

#include "aml/archreq.h"

/* Macros. */
#define	ARCHREQ_INCR (1024*1024) /* Allocation increment for archive request */
#define	ARCHREQ_START 16384	/* Starting allocation for archive request */
#define	ARCHREQ_NAME_SIZE (sizeof (uname_t) * sizeof (set_name_t) + \
		13)	/* 10 digits and 2 '.' */
#define	ARCHREQ_FNAME_SIZE (sizeof (ARCHIVER_DIR) + sizeof (upath_t) + \
		sizeof (ARCHREQ_DIR) + ARCHREQ_NAME_SIZE)

/* Functions. */
boolean_t ArchReqCustMsgSent(struct ArchReq *ar, int msgNum);
void ArchReqFileName(struct ArchReq *ar, char *fname);
struct ArchReq *ArchReqGrow(struct ArchReq *ar);
void ArchReqMsg(const char *srcFile, const int srcLine, struct ArchReq *ar,
		int msgNum, ...);
char *ArchReqName(struct ArchReq *ar, char *arname);
void ArchReqPrint(FILE *st, struct ArchReq *ar, boolean_t showFiles);
void ArchReqTrace(struct ArchReq *ar, boolean_t files);
boolean_t ArchReqValid(struct ArchReq *ar);

#endif /* PRIVATE_ARCHREQ_H */
