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
/*
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */
#ifndef	_LICENSE_H_
#define	_LICENSE_H_

#pragma ident	"@(#)license.h	1.5	03/05/27 SMI"


#include <sys/types.h>
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/types.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* License types */
#define	NON_EXPIRING	(0)    /* License never expires */
#define	EXPIRING		(1)    /* License expires on exp_date */
#define	DEMO			(2)    /* DEMO license expires after 14 days */
#define	QFS_TRIAL		(3)    /* License expires after 60 days */
#define	QFS_SPECIAL		(4)    /* License never expires */

/* SAM-FS types */
#define	QFS				(1)    /* stand alone QFS */
#define	SAMFS			(2)    /* SAM-FS */
#define	SAMQFS			(3)    /* SAM-QFS */

/* Features suported */

#define	REMOTE_SAM_SVR_SUPPORT	0x00000001 /* remote sam server supported */
#define	REMOTE_SAM_CLNT_SUPPORT	0x00000002 /* remote sam client supported */
#define	MIG_TOOLKIT_SUPPORT	0x00000004 /* migration toolkit supported */
#define	FAST_FS_SUPPORT		0x00000008 /* fast file system supported */
#define	DATABASE_SUPPORT	0x00000010 /* database feature supported */
#define	FOREIGN_TAPE_SUPPORT	0x00000020 /* foreign tape supported */
#define	SHARED_FS_SUPPORT	0x00000040 /* shared file system supported */
#define	SEGMENT_SUPPORT		0x00000080 /* segment supported */
#define	SANAPI_SUPPORT		0x00000100 /* SAN API supported */
#define	STANDALONE_QFS_SUPPORT	0x00000200 /* standalone qfs supported */

/*
 * License information
 */
typedef struct license_info {

	int		type;		/* license type */
	int		expire;		/* expiration date */
	int		fsType;		/* licensed for SAM-FS SAM-QFS or QFS */
	uint_t		hostid;		/* hostid */
	uint32_t	feature_flags;	/* bit-mask for supported features */

	/*
	 * media license info:
	 * media type, robot type and num of licensed slots
	 *
	 */
	sqm_lst_t 	*media_list;	/* list of md_license_t */

} license_info_t;


/*
 * DESCRIPTION:
 *   get samfs license type
 * PARAMS:
 *   ctx_t *      IN   - context object
 * RETURNS:
 *   success -  NON_EXPIRING, EXPIRING, DEMO, QFS_TRIAL, or QFS_SPECIAL
 *   error   -  -1
 */
int get_license_type(ctx_t *);


/*
 * DESCRIPTION:
 *   get license expiration date if license type is EXPIRING
 * PARAMS:
 *   ctx_t *      IN   - context object
 * RETURNS:
 *   success -  time in seconds since the Epoch (00:00:00 UTC, January 1, 1970)
 *   error   -  -1
 */
int get_expiration_date(ctx_t *);


/*
 * DESCRIPTION:
 *   get SAMFS type
 * PARAMS:
 *   ctx_t *      IN   - context object
 * RETURNS:
 *   success -  QFS, SAM-FS, or SAM-QFS
 *   error   -  -1
 */
int get_samfs_type(ctx_t *);

/*
 * DESCRIPTION:
 *   get total number of licensed slots for the given
 *   robot_type and media_type pair
 * PARAMS:
 *   ctx_t *      IN   - context object
 *   const char * IN   - robot name
 *   const char * IN   - media name
 * RETURNS:
 *   success -  number of licensed slots
 *   error   -  -1
 */
int get_licensed_media_slots(ctx_t *,
		const char *robot_name, const char *media_name);


/*
 * DESCRIPTION:
 *   get a list of licensed media types (strings)
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   sqm_lst_t **	OUT  - a list of licensed media types (strings)
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int get_licensed_media_types(ctx_t *, sqm_lst_t **media_list);


/*
 * DESCRIPTION:
 *   get license detail information : hostid, type,
 *   features supported and media information (no. of
 *   slots licensed)
 *   This api is available in 4.3 and above
 *
 * PARAMS:
 *   ctx_t *		IN   - context object
 *   license_info_t *	OUT  - license info structure
 * RETURNS:
 *   success -  0
 *   error   -  -1
 */
int get_license_info(ctx_t *, license_info_t **);


/*
 * Free method
 */
void free_license_info(license_info_t *license);


#ifdef  __cplusplus
}
#endif	/* __cplusplus */

#endif	/* _LICENSE_H_ */
