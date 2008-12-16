/*
 * fsd.h - sam-fsd server definitions.
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

#ifndef _AML_FSD_H
#define	_AML_FSD_H

#pragma ident "$Revision: 1.12 $"

/* Solaris headers. */
#include <syslog.h>			/* Notify() uses syslog priorities */

/* Macros. */
#define	MAXMSGLEN 256			/* Maximum message length */

/* Functions */
/* The return is -1 if an error occurred. */
#define	FsdNotify(a, b, c, d) _FsdNotify(_SrcFile, __LINE__, (a), (b), (c), (d))
int _FsdNotify(const char *SrcFile, const int SrcLine, char *fname,
	int priority, int msg_num, char *msg);

#if defined(FSD_PRIVATE)

#define	SERVER_NAME "Fsd"
#define	SERVER_MAGIC 006230423

enum FsdSrvrReq {
	FSD_notify,			/* Execute notify file */
	FSD_MAX
};

/* Arguments for requests. */

struct FsdNotify {
	char	FnFname[MAXPATHLEN];	/* File to execute */
	int	FnPriority;		/* Syslog priority */
	int	FnMsgNnum;		/* Catalog message number. */
					/* If 0, use FnMsg */
	char	FnMsg[MAXMSGLEN];	/* Actual message */
};

/* General response. */
struct FsdGeneralRsp {
	int    GrStatus;
	int    GrErrno;
};

#endif /* defined(FSD_PRIVATE) */

#endif /* _AML_FSD_H */
