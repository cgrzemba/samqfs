/*
 * tapealert_text.h - TapeAlert flags to strings
 *
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

#ifndef _TAPEALERT_TEXT_H
#define	_TAPEALERT_TEXT_H

#pragma ident "$Revision: 1.9 $"

#include "aml/tapealert_vals.h"

/*
 * TapeAlert client support
 */

typedef enum { /* tapealert severity levels */
	SEVERITY_RSVD,		/* severity reserved */
	SEVERITY_INFO,			/* severity informational */
	SEVERITY_WARN,		/* severity warning */
	SEVERITY_CRIT 		/* severity critical */
} tapealert_severity_t;

typedef struct {
	char *manual;			/* T10 SSC-n, SMC-n */
	int pcode;			/* tapealert parameter code */
	char *flag;			/* tapealert flag */
	char *severity;			/* Critical, Warning, Information */
	tapealert_severity_t scode; /* severity integer value */
	char *appmsg;			/* tapealert application message */
	char *cause;			/* tapealert probable cause */
} tapealert_msg_t;

typedef struct   /* robot and drive tapealert text */
{
	int count;
	tapealert_msg_t msg [128];
} tapealert_text_t;

/* tapealert text function protos */
void tapealert_text(int version, int type, int len, uint64_t flags,
	tapealert_text_t *text);
void tapealert_text_done(tapealert_text_t *text);


#endif
