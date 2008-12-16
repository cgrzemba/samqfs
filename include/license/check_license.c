/*
 * check_license.c - check license
 */

/*
 *	Solaris 2.x Sun Storage & Archiving Management File System
 */

/*
 *
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

#pragma ident "$Revision: 1.19 $"

/*
 *	NOTE NOTE NOTE NOTE NOTE NOTE  NOTE NOTE NOTE
 *
 * These files (check_license.c encrypt.c license.c) are intended to
 * be included with the pieces of samfs that need access to the license
 * data.  It is done this way for security reasons.  By including these
 * files, rather than compiling and linking, we can make all the routines
 * static, which makes them much more difficult for a hacker to find.
 *
 */

static int
fast_fs_enabled(sam_license_t_33 *sl) {
	if (sl->license.lic_u.b.fast_fs) {
		return (1);
	} else {
		return (0);
	}
}

static int
db_features_enabled(sam_license_t_33 *sl) {
	if (sl->license.lic_u.b.db_features) {
		return (1);
	} else {
		return (0);
	}
}

static int
sharedfs_enabled(sam_license_t_33 *sl) {
	if (sl->license.lic_u.b.shared_fs) {
		return (1);
	} else {
		return (0);
	}
}

static int
worm_fs_enabled(sam_license_t_33 *sl) {
	if (sl->license.lic_u.b.WORM_fs) {
		return (1);
	} else {
		return (0);
	}
}
