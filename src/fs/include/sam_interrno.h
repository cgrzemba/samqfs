/*
 * sam_interrno.h - SAM-QFS internal error messages
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

#ifdef sun
#pragma ident "$Revision: 1.12 $"
#endif


#ifndef	_SAMINTERRNO_H
#define	_SAMINTERRNO_H

#ifdef  __cplusplus
extern "C" {
#endif

/*
 *	Define internal SAM-FS error code base.
 */

#define	SAM_INTERNAL_ERRNO 20000

/*
 *	Define internal error codes for SAM-FS
 */
enum sam_internal_errno { EISAM_INITIAL = SAM_INTERNAL_ERRNO,
	EISAM_MOUNT_OUTSYNC,	/* Client is out of sync after failover */
	EISAM_MAX_ERRNO		/* Upper limit of SAM internal errno */
};

#ifdef  __cplusplus
}
#endif

#endif /* _SAMINTERRNO_H */
