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
#pragma ident   "$Revision: 1.24 $"

static char *_SrcFile = __FILE__;  /* Using __FILE__ makes duplicate strings */

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "sam/lib.h"
#include "sam/types.h"
#include "sam/sam_trace.h"
#include "sam/syscall.h"
#include "aml/proto.h"

#include "mgmt/util.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/license.h"

/*
 * Get the license type:
 *	 NON_EXPIRING, EXPIRING, DEMO, QFS_TRIAL, or QFS_SPECIAL
 */
int
get_license_type(
ctx_t *ctx /* ARGSUSED */)
{
	return (NON_EXPIRING);
}


/*
 * Get the license expiration date if the license type is EXPIRING
 * (in seconds since 1970)
 */
int
get_expiration_date(
ctx_t *ctx /* ARGSUSED */)
{
	return (0);
}


/*
 * Get the SAMFS type: QFS, SAM-FS, or SAM-QFS
 * used by the filesystem api. Only do the stat one time.
 */
int
get_samfs_type(
ctx_t *ctx /* ARGSUSED */)
{
	static int type = 0;
	struct stat st;

	if (type != 0) {
		return (type);
	} else if (stat(SBIN_DIR"/"SAM_AMLD, &st) == 0) {
		type = SAMQFS;
	} else {
		type = QFS;
	}

	return (type);
}

/*
 * Get the total number of licensed slots for the given
 * robot_type and media_type pair
 */
int
get_licensed_slots(
ctx_t *ctx,			/* ARGSUSED */
ushort_t robot_type,	/* IN - robot type */
ushort_t media_type)	/* IN - media type */
{
	/* INT32_MAX = 0x7fffffff, MAX_SLOTS = 0x7fffffff */
	return (MAX_SLOTS - 1);

}


/*
 * Get the total number of licensed slots for the given
 * robot_type and media_type pair
 */
int
get_licensed_media_slots(
ctx_t *ctx,				/* ARGSUSED */
const char *robot_name,	/* IN - robot name */
const char *media_name)	/* IN - media name */
{
	return (MAX_SLOTS - 1);
}


/*
 * Get a list of licensed media_types (strings)
 */
int
get_licensed_media_types(
ctx_t *ctx,				/* ARGSUSED */
sqm_lst_t **media_list)	/* OUT - list of licensed media names */
{

	sqm_lst_t *lst = NULL;

	lst = lst_create();

	if (!lst) {
		Trace(TR_OPRMSG, "out of memory");
		return (-1);
	}

	*media_list = lst;

	return (lst->length);

}

/*
 * Get the license information:
 *	 license type, hostid, features supported, media licenses
 */
int
get_license_info(
ctx_t *ctx, 			/* ARGSUSED */
license_info_t **info		/* OUT - License info struct */
)
{

	struct stat st;

	Trace(TR_MISC, "getting license information");

	if (ISNULL(info)) {
		Trace(TR_OPRMSG, "null argument found");
		/* samerrno and samerrmsg are set */
		return (-1);
	}

	*info = NULL;

	/* fill in the license_info structure */

	*info = (license_info_t *)mallocer(sizeof (license_info_t));

	if (*info == NULL) {
		Trace(TR_ERR, "get license info failed: %s", samerrmsg);
		return (-1);
	}

	memset(*info, 0, sizeof (license_info_t));

	(*info)->type = NON_EXPIRING;
	(*info)->expire = 0;
	(*info)->hostid = 0;

	if (stat(SBIN_DIR"/"SAM_AMLD, &st) == 0) {
		(*info)->fsType = SAMQFS;
	} else {
		(*info)->fsType = QFS;
	}

	/* clear the bitmask */
	(*info)->feature_flags = (uint32_t)0;

	(*info)->feature_flags |= 0xffffffff;

	return (0);

}
