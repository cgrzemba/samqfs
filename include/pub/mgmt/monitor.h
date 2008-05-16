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
#ifndef _MONITOR_H
#define	_MONITOR_H

#pragma	ident	"$Revision: 1.15 $"


#include <sys/param.h>

#include "mgmt/config/common.h"
#include "pub/mgmt/device.h"
#include "aml/catalog.h"	/* for VSN status bit field defns */

/*
 * get_status_processes
 * Returns a list of formatted strings that represent daemons and process
 * (related to SAM activity) that are running or are expected to run.
 *
 * format of string:-
 * 	activityid=pid (if absent, then it is not running)
 * 	details=daemon or process name
 * 	type=SAMDXXXX
 * 	description=long user friendly name
 * 	starttime=secs
 * 	parentid=ppid
 * 	modtime=secs
 * 	path=/var/opt/SUNWsamfs/devlog/21
 */
int get_component_status_summary(ctx_t *ctx, sqm_lst_t **status_lst);

/*
 * Summarize the state of the system, return list of formatted strings
 * with the component name and status (OK, WARNING, ERR, FAILURE)
 * In future, the error or warning message may also be returned
 *
 * Returned list of strings:-
 *      name=daemons,status=0
 *      name=fs,status=0
 *      name=copyUtil,status=0
 *      name=libraries,status=0
 *      name=drives,status=0
 *      name=loadQ,status=0
 *      name=unusableVsn,status=0
 *      name=arcopyQ,status=0
 *      name=stageQ,status=0
 */
int get_status_processes(ctx_t *ctx, sqm_lst_t **daemon_status_list);

#endif	/* _MONITOR_H */
