/*
 * ----- utility/samfm.h - StorADE API.
 *
 * Shared memory put into XML message for StorADE use.
 * The XML DTD starts in /opt/SUNWsamfs/doc/message.dtd.
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

#ifndef _SAMFM_H
#define	_SAMFM_H


/*
 * Errors returned by samfm.
 */
#define	SAMFM_ERROR_ARGS    1	/* input args */
#define	SAMFM_ERROR_CONNECT 2	/* SAM-FS not responding */
#define	SAMFM_ERROR_WRITE   3	/* message write */
#define	SAMFM_ERROR_POLL    4	/* wait for response message timed-out */
#define	SAMFM_ERROR_READ    5	/* response message read */
#define	SAMFM_ERROR_MEMORY  6	/* out of memory for response message */


/*
 * Function prototypes.
 */
int samfm(char *host, int timeout, char *send, char **recv);


#endif /* _SAMFM_H */
