/* api.c - general API interface library */

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

#include <stdio.h>
#include <synch.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/syslog.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "sam/devinfo.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "aml/robots.h"
#include "sam/nl_samfs.h"
#include "sam/defaults.h"
#include "sam/lib.h"
#include "generic.h"
#include "api_errors.h"
#include "aml/dev_log.h"


#pragma ident "$Revision: 1.22 $"

/* Using __FILE__ makes duplicate strings */
static char    *_SrcFile = __FILE__;

#define	ASSIGN_VALUE(_ret, _type, _ecode, _member) \
			switch (_type) { \
				case DT_GRAUACI: \
					if ((_ecode >= 0) && \
						(_ecode < NO_ECODES)) \
					_ret = grau_messages[_ecode]._member; \
					break; \
 \
				default: \
					break; \
			}

/*
 * api_valid_error() - Returns non-0 if this is a valid errorcode
 */
int
api_valid_error(dtype_t type, int errorcode, dev_ent_t *un)
{

#if !defined(SAM_OPEN_SOURCE)
	register api_messages_t *tmp;

	if ((errorcode < 0) && (errorcode >= NO_ECODES)) {
		if (un != (dev_ent_t *)0)
			DevLog(DL_ERR(6002), errorcode, NO_ECODES);
		else {
			sam_syslog(LOG_ERR,
			    catgets(catfd, SET, 9197,
			    "d_errno(%d): Range error 0-%d."),
			    errorcode, NO_ECODES);
		}
		return (0);
	}
	return (-1);
#endif
}

/*
 * api_return_degree(dtype_t type, int errorcode)
 */

api_errs_t
api_return_degree(dtype_t type, int errorcode)
{
	api_errs_t	ret = API_ERR_TR;

#if !defined(SAM_OPEN_SOURCE)
	ASSIGN_VALUE(ret, type, errorcode, degree);
#endif
	return (ret);
}

/*
 * api_return_retry(dtype_t type, int errorcode)
 */

uint_t
api_return_retry(dtype_t type, int errorcode)
{
	uint_t	   ret = 0;

#if !defined(SAM_OPEN_SOURCE)
	ASSIGN_VALUE(ret, type, errorcode, retry);
#endif
	return (ret);
}

/*
 * api_return_message(dtype_t type, int errorcode)
 */

char	   *
api_return_message(dtype_t type, int errorcode)
{
	char	   *ret = (char *)0;

#if !defined(SAM_OPEN_SOURCE)
	ASSIGN_VALUE(ret, type, errorcode, mess);
#endif
	return (ret);
}

/*
 * api_return_sleep(dtype_t type, int errorcode)
 */

uint_t
api_return_sleep(dtype_t type, int errorcode)
{
	uint_t	   ret = 0;

#if !defined(SAM_OPEN_SOURCE)
	ASSIGN_VALUE(ret, type, errorcode, sleep);
#endif
	return (ret);
}
