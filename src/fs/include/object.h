/*
 * object.h - SAM-FS osd structs, defines, and function prototypes.
 *
 *  Defines the file system structs and prototypes for object-based storage.
 *
 */

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

#pragma ident "$Revision: 1.4 $"

#ifndef	_SAM_FS_OBJECT_H
#define	_SAM_FS_OBJECT_H

#include	<sys/types.h>
#include	<sys/vnode.h>
#include	<sys/cred.h>
#include	<sys/mutex.h>
#include	<sys/condvar.h>
#include	<sys/conf.h>
#include	<sys/ddi.h>
#if defined(SAM_OSD_SUPPORT)
#include	<sys/osd.h>
#endif /* SAM_OSD_SUPPORT */
#include	<sys/byteorder.h>

#include	<vm/page.h>
#include	<vm/as.h>
#include	<vm/seg_vn.h>
#include	<vm/seg_map.h>


/* Maxcontig for OSD I/O */

#define	SAM_OSD_MAX_WR_CONTIG		0x100000
#define	SAM_OSD_MAX_RD_CONTIG		0x20000

/*
 *  Macros to keep sane (NTOH - network to host; HTON - host to network).
 *  NTOH and HTON are the same.
 */

#define	NTOHS(__X)  BE_16(__X)
#define	NTOHL(__X)  BE_32(__X)
#define	NTOHLL(__X) BE_64(__X)
#define	HTONS(__X)  BE_16(__X)
#define	HTONL(__X)  BE_32(__X)
#define	HTONLL(__X) BE_64(__X)

/* ----- Object-based storage structs */

typedef union sam_id_attr {	/* Inode identification osd attribute */
	int64_t 	attr;
	sam_id_t	id;
} sam_id_attr_t;

/*
 *	Because the OSD structs are built using pack(1), any locals
 *	or statics need to be aligned. We cannot depend on the compiler
 *	to generate the appropriate code to access unaligned structs
 */
#pragma pack(1)
typedef struct sam_out_get_osd_attr_list {
	osd_attributes_list_t		attr_list;
	osd_list_type_retrieve_t	entry;
} sam_out_get_osd_attr_list_t;
#pragma pack()


#pragma pack(1)
typedef struct sam_in_get_osd_attr_list {
	osd_attributes_list_t	attr_list;
	osd_list_type_retset_t	entry;
	char					value[8];
} sam_in_get_osd_attr_list_t;
#pragma pack()

typedef union sam_osd_attr_page {
	offset_t				value;
} sam_osd_attr_page_t;

typedef struct sam_osd_iot_priv {
	ksema_t		osd_sema;	/* I/O Synchronization for io_task */
	struct sam_buf	*bp;		/* buf struct */
	struct sam_node	*ip;		/* Pointer to inode */
} sam_osd_iot_priv_t;

#define	iot_priv(__IOT)  ((struct sam_osd_iot_priv *)(__IOT)->ot_client_private)

#define	sam_osd_setup_PRIVATE(iotp)  \
	sema_init(&((iot_priv(iotp))->osd_sema), 0, NULL, SEMA_DEFAULT, NULL)

#define	sam_osd_remove_PRIVATE(iotp)  \
	sema_destroy(&(iot_priv(iotp))->osd_sema)

#define	sam_osd_io_task_WAIT(iotp)  \
	sema_p(&(iot_priv(iotp))->osd_sema)

#define	sam_osd_io_task_DONE(iotp)  \
	sema_v(&(iot_priv(iotp))->osd_sema)

int sam_osd_io_errno(struct osd_iotask *iotp, struct buf *bp);

#endif	/* _SAM_FS_OBJECT_H */
