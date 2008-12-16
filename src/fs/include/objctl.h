/*
 * ----- objctl.h - Object name space definition.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#ifndef	_OBJCTL_H
#define	_OBJCTL_H

#pragma ident "$Revision: 1.3 $"

#define	OBJCTL_NAME		".objects"
#define	OBJCTL_INO_ROOT		0x1

#if defined(SOL_511_ABOVE)
extern void sam_objctl_fini(void);
extern void sam_objctl_init(void);
extern void sam_objctl_create(struct sam_mount *);
extern void sam_objctl_destroy(struct sam_mount *);
#else
#define	sam_objctl_fini(void)
#define	sam_objctl_init(void)
#define	sam_objctl_create(mp)
#define	sam_objctl_destroy(mp)
#endif
#endif	/* _OBJCTL_H */
