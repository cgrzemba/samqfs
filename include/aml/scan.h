/*
 * scan.h - Constants and structs for scanner interface.
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


#ifndef _AML_SCAN_H
#define	_AML_SCAN_H

#pragma ident "$Revision: 1.12 $"

#include <thread.h>

/* Some constants */

#define	SCANNER_CYCLE	10	/* How often to run the scanner */
#define	TP_SCAN_TIME	30	/* How often to scan tape drives */

/* Scanner message commands */

enum scan_cmds {
	SCAN_QCMD_TODO		/* todo request */
};

#endif /* _AML_SCAN_H */
