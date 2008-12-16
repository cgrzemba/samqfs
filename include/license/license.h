/*
 * license.h - structs and defines for licenses.
 *
 * This file should NEVER be released outside of Sun proper.
 *
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

#ifndef _SAM_LICENSE_H
#define	_SAM_LICENSE_H

#pragma ident "$Revision: 1.23 $"

#include <sam/types.h>


typedef struct {
	union  {
	uint_t   whole;
	struct {
		uint_t
#if	defined(_BIT_FIELDS_HTOL)
		all_valid	: 1,	/* All features valid */
		license_type	: 8,	/* expiring, non-expiring, or demo */
		rmt_server	: 1,	/* remote sam server allowed */
		rmt_client	: 1,	/* remote sam client allowed */
		migration	: 1,	/* migration allowed */
		fast_fs		: 1,	/* Fast file system */
		qfs_stand_alone : 1,	/* QFS without SAM */
		unused1		: 4,	/* was platform */
		db_features	: 1,	/* database features */
		foreign_tape	: 1,	/* support Non SAM tape */
		shared_san	: 1,	/* shared SAN filesystem */
		segment		: 1,	/* segment */
		shared_fs	: 1,	/* shared filesystem */
		WORM_fs		: 1,	/* WORM filesystem */
		unused		: 8;	/* More features */
#else	/* defined(_BIT_FIELDS_HTOL) */
		unused		: 8,	/* More features */
		WORM_fs		: 1,	/* WORM filesystem */
		shared_fs	: 1,	/* shared filesystem */
		segment		: 1,	/* segment */
		shared_san	: 1,	/* shared SAN filesystem */
		foreign_tape	: 1,	/* support Non SAM tape */
		db_features	: 1,	/* database features */
		unused1		: 4,	/* was platform */
		qfs_stand_alone : 1,	/* QFS without SAM */
		fast_fs		: 1,	/* Fast file system */
		migration	: 1,	/* migration allowed */
		rmt_client	: 1,	/* remote sam client allowed */
		rmt_server	: 1,	/* remote sam server allowed */
		license_type	: 8,	/* expiring, non-expiring or demo */
		all_valid	: 1;	/* All features valid */
#endif  /* defined(_BIT_FIELDS_HTOL) */
		} b;
	} lic_u;
} sam_lic_value_33;

typedef struct {
	uint_t		hostid;		/* From sysinfo */
	sam_time_t	exp_date;	/* In seconds since 1970 */
	sam_lic_value_33   license;
	uint_t		check_sum;
} sam_license_t_33;

typedef struct {
	int		num_slots;
	uint_t		hostid;
#if defined(_BIG_ENDIAN)
	ushort_t	robot_type;
	ushort_t	media_type;
#endif /* defined(_BIG_ENDIAN) */
#if defined(_LITTLE_ENDIAN)
/*
 * This is a little strange, but since parts of the code, e.g., encrypt(), deal
 * with this first word as a long, the upper and lower parcels get swapped.
 */
	ushort_t	media_type;
	ushort_t	robot_type;
#endif /* defined(_LITTLE_ENDIAN) */
	uint_t	check_sum;
} sam_media_license_t_33;

/* License types */
#define	NON_EXPIRING	(0)
#define	EXPIRING	(1)
#define	DEMO		(2)
#define	QFS_TRIAL	(3)
#define	QFS_SPECIAL	(4)

/* time conversions */
#define	MINUTE	(60)
#define	HOUR	(MINUTE * 60)
#define	DAY	(HOUR *24)

#define	DAYS_TO_TRIAL_EXPIRATION (60)
#define	DAYS_TO_TRIAL_WARN	(45)
#define	DAYS_TO_DEMO_EXPIRATION	(14)
#define	DAYS_TO_DEMO_WARN	(7)

#define	 SAM_LICENSE_DATA_FILE	"LICENSE.dat"


#define	CHECK_SUM(a, len, sum) {\
	char *p = (char *)(a);\
	int i;\
	sum = 0;\
	for (i = 0; i < (len); i++)\
		sum ^= *p++;\
}

#endif /* _SAM_LICENSE_H */
