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


#pragma ident "$Revision: 1.11 $"


/* read_license() return codes: */

#define	RL_OK			0
#define	RL_CANT_STAT	1
#define	RL_MOD_TIME		2
#define	RL_CANT_OPEN	3
#define	RL_CANT_READ	4
#define	RL_LIC_DATA		5

/* verify_hostid return codes: */

#define	VH_OK			0
#define	VH_NO_MATCH		1

/* verify_expiration return codes: */

#define	VE_EXPIRED		(-1)
#define	VE_NO_EXPIRE	(0)
/* else verify_expiration returns the number of seconds until expiration */

/* verify_platform return codes: */
#define	VP_OK			0
#define	VP_NO_MATCH		1
