/*
 * samrd_def.h
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

/*
 * $Revision: 1.14 $
 */

#ifndef _SAMRD_DEF_H
#define	_SAMRD_DEF_H

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Ioctl commands
 */
#define	SAMRDIOC	('R' << 8)
#define	SAMRDIOC_EOF	(SAMRDIOC|0)  /* Send EOF to reader */
#define	SAMRDIOC_EOT	(SAMRDIOC|1)  /* Send EOT to writer */
#define	SAMRDIOC_EIO_W	(SAMRDIOC|2)  /* Send EIO to writer */
#define	SAMRDIOC_EIO_R	(SAMRDIOC|3)  /* Send EIO to reader */
#define	SAMRDIOC_PIPE	(SAMRDIOC|4)  /* Send EPIPE to all io requests */
#ifdef  __cplusplus
}
#endif

#define	SAMRD_TIMEOUT	120

#endif /* _SAMRD_DEF_H */
