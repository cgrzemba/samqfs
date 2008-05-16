/*
 * historian.h - structs and defines for catalog historian.
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

#if !defined(_AML_HISTORIAN_H)
#define	_AML_HISTORIAN_H

#pragma ident "$Revision: 1.11 $"

#include "sam/types.h"

/* Flags used in request messages */

#define	 HIST_UPDATE_INFO 0x80000000	/* if already there, update info */
#define	 HIST_SEND_ACK 0x40000000	/* send response back */
#define	 HIST_SET_UNAVAIL 0x20000000	/* set unavail bit in historian */
#define	 HIST_REMOVE 0x10000000		/* remove it */

#define	 HIST_BARCODE 0x01000000	/* search is by barcode else vsn */

/* return codes in response messages (MESS_CMD_HIST_INFO) */

#define	HIST_ENTRY_FOUND   0
#define	HIST_ENTRY_NOTFND  1

#endif /* !defined(_AML_HISTORIAN_H) */
