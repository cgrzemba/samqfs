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

#ifndef SAM_VERSION_H
#define	SAM_VERSION_H

#ifdef  __cplusplus
extern "C" {
#endif

#define	SAM_NAME "SAM-FS"
#define	SAM_MAJORV "%samqfs_version%"
#define	SAM_MINORV "0"
#define	SAM_FIXV "%level%"
#define	SAM_VERSION SAM_MAJORV "." SAM_FIXV
#define	SAM_BUILD_INFO SAM_VERSION ", %host% %datetime%"
#define	SAM_BUILD_UNAME "%uname%"
/* the sam_license_file is relative to the SAMPATH */
#define	 SAM_LICENSE_FILE    "LICENSE." SAM_MAJORV

#ifdef  __cplusplus
}
#endif

#endif /* SAM_VERSION_H */
