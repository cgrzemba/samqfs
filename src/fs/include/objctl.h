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

#ifndef _OBJCTL
#define _OBJCTL

#include "mount.h" 

#define OBJCTL_NAME "keine Ahnung"

#if !defined(_NoOSD_)
void sam_objctl_create(sam_mount_t * mp); 
void sam_objctl_destroy(sam_mount_t * mp);
void sam_objctl_init();
void sam_objctl_fini();
#else
#define sam_objctl_create(mp)
#define sam_objctl_destroy(mp)
#define sam_objctl_init()
#define sam_objctl_fini()
#endif

#endif
