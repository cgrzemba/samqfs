/*
 * sam/shareops.h - SAM-FS shared FS operations
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

#ifndef	_SAM_SHAREOPS_H
#define	_SAM_SHAREOPS_H

#define	SHARE_OP_WAKE_SHAREDAEMON	1	/* wake FS share daemon */
#define	SHARE_OP_INVOL_FAILOVER		2	/* failover is involuntary */
#define	SHARE_OP_AWAIT_WAKEUP		3	/* await FS sharedaemon wake */
#define	SHARE_OP_AWAIT_FAILDONE		4	/* block 'til failover */
						/* complete */
#define	SHARE_OP_HOST_INOP		5	/* not needed to finish */
						/* failover */
#define	SHARE_OP_CL_MOUNTED		6	/* # of clients mounted */

int sam_gethost(const char *, int, char *tab);
int sam_sethost(const char *, int newserver, int len, const char *tab);

int sam_shareops(const char *, int op, int host);

#endif	/* _SAM_SHAREOPS_H */
