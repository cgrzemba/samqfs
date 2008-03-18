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
#ifndef _DIRECTIVES_H
#define	_DIRECTIVES_H

#pragma ident	"$Revision: 1.9 $"

/*
 * directives.h - Common directive definition.
 */

#include "sam/types.h"

/*
 * structure for the buffer directives
 */
typedef struct buffer_directive {

	mtype_t	media_type;
	fsize_t size;
	boolean_t lock;
	uint32_t change_flag;

} buffer_directive_t;

/* Buffer directive set flags */
#define	BD_size		0x00000002
#define	BD_lock		0x00000004


/*
 * structure for the drive directives
 */
typedef struct drive_directive {

	uname_t auto_lib;
	int	count;
	uint32_t change_flag;

} drive_directive_t;

/*
 * drive_directive set_flags masks
 */
#define	DD_count	0x00000001

#endif /* _DIRECTIVES_H */
