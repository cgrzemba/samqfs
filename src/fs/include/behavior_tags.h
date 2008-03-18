/*
 *	behavior_tags.h - message behavior tag names and bits
 *
 *	Defines the names of message behavior tags and their corresponding
 *	bitmask.
 */

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

#ifndef	_SAM_FS_TAGS_H
#define	_SAM_FS_TAGS_H

/*
 * Example code.  It's a good idea to keep the strings short so we don't
 * overrun QFS_TAG_LIST_SZ too soon.
 *
 * #define	QFS_TAG_BLK_QUOTA_V2_STR	"blkquota_v2"
 * #define	QFS_TAG_SPECIAL_STUFF_STR	"spec_stuff"
 */

#define	QFS_TAG_VFSSTAT_V2_STR		"vfsstat_v2"


/*
 * Example code:
 *
 * #define	QFS_TAG_BLK_QUOTA_V2	0x0001
 * #define	QFS_TAG_SPECIAL_STUFF	0x0002
 */

#define	QFS_TAG_VFSSTAT_V2	0x0001


#endif  /* _SAM_FS_TAGS_H */
