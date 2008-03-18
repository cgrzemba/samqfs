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
#ifndef _CFG_CPARAMS_DEFS_H
#define	_CFG_CPARAMS_DEFS_H

#pragma ident	"$Revision: 1.9 $"

#include <sys/types.h>

#include "sam/setfield.h"
#include "pub/mgmt/archive.h"

static struct EnumEntry join_tbl[] = {
	{ "not_set",	JOIN_NOT_SET },
	{ "none",	NO_JOIN },
	{ "path",	JOIN_PATH },
	{ NULL }};

static struct EnumEntry offline_cpy_tbl[] = {
	{ "not_set",	OC_NOT_SET },
	{ "none",	OC_NONE },
	{ "direct",	OC_DIRECT },
	{ "stageahead",	OC_STAGEAHEAD },
	{ "stageall",	OC_STAGEALL },
	{ NULL }};

static struct EnumEntry sort_method_tbl[] = {
	{ "not_set",	SM_NOT_SET },
	{ "none",	SM_NONE },
	{ "age",	SM_AGE },
	{ "path",	SM_PATH },
	{ "priority",	SM_PRIORITY },
	{ "size",	SM_SIZE },
	{ NULL }};

#endif	/* _CFG_CPARAMS_DEFS_H */
