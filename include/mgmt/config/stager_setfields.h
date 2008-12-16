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

#ifndef	STAGER_SETFIELDS_H
#define	STAGER_SETFIELDS_H

#pragma ident	"$Revision: 1.10 $"


/*
 * stager_setfields.h
 * contains the setfield tables for the stager structures used for
 * processing stager.cmd for the api structs.
 */

#include <sys/types.h>
#include "sam/setfield.h"
#include "aml/stager.h"
#include "pub/mgmt/stage.h"

static int stage_cfg_table0;
static struct fieldInt stage_cfg_table1 = { STAGER_DEFAULT_MAX_ACTIVE, 1,
	INT_MAX, 0 };
static struct fieldInt stage_cfg_table2 = { STAGER_DEFAULT_MAX_RETRIES, 0,
	20, 0 };
static struct fieldString stage_cfg_table3 = { "", STAGER_MAX_PATHLEN };
static struct fieldVals stage_cfg_table[] = {
	{ "", DEFBITS, offsetof(struct stager_cfg, change_flag),
		&stage_cfg_table0, },
	{ "maxactive", INT, offsetof(struct stager_cfg, max_active),
		&stage_cfg_table1, ST_max_active},
	{ "maxretries", INT, offsetof(struct stager_cfg, max_retries),
		&stage_cfg_table2, ST_max_retries},
	{ "logfile", STRING, offsetof(struct stager_cfg, stage_log),
		&stage_cfg_table3, ST_stage_log},
	{ NULL }};

#endif	/* STAGER_SETFIELDS_H */
