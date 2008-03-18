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
#ifndef PUB_DISKVOLS_H
#define	PUB_DISKVOLS_H

#pragma ident   "$Revision: 1.18 $"


#include "sam/types.h"

#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"

#define	DISKVOLS_DUMP_FILE "diskvols.conf.dump"

/*
 * diskvol.h
 * Used to get the disk archive volume configuration and make changes to
 * it. This header contains structures related to the diskvols.conf file.
 */


/* diskvols status flags */
#define	DV_LABELED	DV_labeled
#define	DV_REMOTE	DV_remote
#define	DV_UNAVAILABLE	DV_unavail
#define	DV_READ_ONLY	DV_read_only
#define	DV_BAD_MEDIA	DV_bad_media
#define	DV_NEEDS_AUDIT	DV_needs_audit;
#define	DV_ARCHIVEFULL	DV_archfull;
#define	DV_STK5800_VOL		0x20000000 /* Volume is an STK 5800 vol */
#define	DV_DB_NOT_INIT		0x40000000 /* dictionary not initialized */
#define	DV_UNKNOWN_VOLUME	0x80000000 /* dictionary has no information */



typedef struct disk_vol {
	vsn_t	vol_name;
	host_t	host; /* if remote else '\0' */
	upath_t	path;
	uint32_t set_flags;
	fsize_t capacity;
	fsize_t free_space;
	uint32_t status_flags;
} disk_vol_t;


/*
 * Return one disk_vol_t based on vsn name.
 */
int get_disk_vol(ctx_t *ctx, const char *vol_name, disk_vol_t **disk_vol);


/*
 * returns a list of all configured diskvols.
 */
int get_all_disk_vols(ctx_t *ctx, sqm_lst_t **vol_list);


/*
 * return a list of all trusted clients.
 */
int get_all_clients(ctx_t *ctx, sqm_lst_t **clients);


/*
 * add a disk_vol_t
 */
int add_disk_vol(ctx_t *ctx, disk_vol_t *);


/*
 * remove a disk_vol_t
 */
int remove_disk_vol(ctx_t *ctx, const vsn_t);


/*
 * add a client
 */
int add_client(ctx_t *ctx, host_t new_client);

/*
 * remove a client
 */
int remove_client(ctx_t *ctx, host_t new_client);

/*
 * set the flags for the named disk volume
 */
int set_disk_vol_flags(ctx_t *ctx, char *vol_name, uint32_t flag);

#endif /* PUB_DISKVOLS_H */
