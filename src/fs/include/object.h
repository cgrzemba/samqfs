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

#pragma ident "$Revision: 1.13 $"

#include "sam/osversion.h"

#ifndef	_SAM_FS_OBJECT_H
#define	_SAM_FS_OBJECT_H

#include	<sys/types.h>
#include	<sys/vnode.h>
#include	<sys/cred.h>
#include	<sys/mutex.h>
#include	<sys/condvar.h>
#include	<sys/conf.h>
#include	<sys/ddi.h>
#include	"osd.h"
#include	<sys/byteorder.h>

#include	<vm/page.h>
#include	<vm/as.h>
#include	<vm/seg_vn.h>
#include	<vm/seg_map.h>


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


#if SAM_ATTR_LIST

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

#endif /* SAM_ATTR_LIST */

#define	SAM_OBJ_PRIVATE_NAME	0x504f524a

typedef struct sam_osd_req_priv {
	int		name;		/* Sanity check */
	int		error;		/* Error -- set in interrupt routine */
	int		obji;		/* Object layout index */
	uint16_t	err_code;	/* OSD return code */
	uint16_t	service_action;	/* OSD service action */
	ksema_t		osd_sema;	/* I/O Synchronization for io_task */
	osd_req_t	*reqp;		/* Pointer to request */
	sam_mount_t	*mp;		/* Pointer to mount table */
	struct sam_node	*ip;		/* Pointer to inode */
	struct buf	*bp;		/* Pointer to optional buffer */
	offset_t	offset;		/* Logical offset in object I/O */
} sam_osd_req_priv_t;

#define	sam_osd_setup_private(iorp, mp) {  \
	bzero(iorp, sizeof (sam_osd_req_priv_t)); \
	sema_init(&((iorp)->osd_sema), 0, NULL, SEMA_DEFAULT, NULL); \
	iorp->name = SAM_OBJ_PRIVATE_NAME; \
	iorp->mp = mp; \
}

#define	sam_osd_remove_private(iorp)  \
	sema_destroy(&(iorp)->osd_sema)

#define	sam_osd_obj_req_wait(iorp)	sema_p(&(iorp)->osd_sema)

#define	sam_osd_obj_req_done(iorp)  \
	sema_v(&(iorp)->osd_sema)

/*
 * SAM-QFS Vendor Defined OSD Page
 * The format is in Big Endian
 */
#define	OSD_SAMQFS_VENDOR_USERINFO_ATTR_PAGE_NO    0x2fffffff

typedef struct sam_objinfo_page {
	uint32_t	sop_pgno;
	uint32_t	sop_pglen;
	uint64_t	sop_partid;
	uint64_t	sop_oid;
	uint64_t	sop_append_addr;
	uint64_t	sop_bytes_alloc;	/* Data bytes allocated */
	uint64_t	sop_logical_len;	/* Logical end of object */
	uint64_t	sop_trans_id;
	uint32_t	sop_fino;
	uint32_t	sop_fgen;
} sam_objinfo_page_t;

#define	SOP_PGNO(SOPP)		BE_32(SOPP->sop_pgno)
#define	SOP_PGLEN(SOPP)		BE_32(SOPP->sop_pglen)
#define	SOP_PARTID(SOPP)	BE_64(SOPP->sop_partid)
#define	SOP_OID(SOPP)		BE_64(SOPP->sop_oid)
#define	SOP_APPEND_ADDR(SOPP)	BE_64(SOPP->sop_append_addr)
#define	SOP_BYTES_ALLOC(SOPP)	BE_64(SOPP->sop_bytes_alloc)
#define	SOP_LOGICAL_LEN(SOPP)	BE_64(SOPP->sop_logical_len)
#define	SOP_TRANSID(SOPP)	BE_64(SOPP->sop_trans_id)
#define	SOP_FINO(SOPP)		BE_32(SOPP->sop_fino)
#define	SOP_FGEN(SOPP)		BE_32(SOPP->sop_fgen)

#define	OSD_SAMQFS_VENDOR_CREATED_OBJECTS_LIST_PAGE    0x2ffffffe

typedef struct sam_objlist_page {
	uint32_t	solp_pgno;
	uint32_t	solp_pglen;
	uint64_t	solp_object_id[1];
} sam_objlist_page_t;

/*
 * SAM-QFS Vendor Defined file system attribute page
 */
#define	OSD_SAMQFS_VENDOR_FS_ATTR_PAGE_NO    0x2ffffffd

typedef struct sam_fsinfo_page {
	uint32_t	sfp_pgno;
	uint32_t	sfp_pglen;
	offset_t	sfp_capacity;
	offset_t	sfp_space;
} sam_fsinfo_page_t;

#define	SFP_PGNO(SFPP)		BE_32(SFPP->sfp_pgno)
#define	SFP_PGLEN(SFPP)		BE_32(SFPP->sfp_pglen)
#define	SFP_CAPACITY(SFPP)	BE_64(SFPP->sfp_capacity)
#define	SFP_SPACE(SFPP)		BE_64(SFPP->sfp_space)

/*
 * There are 2 driver interfaces.
 *
 * drv/sosd - Contains the Open/Close routines.
 */
#define	SCSI_SOSD_MOD_NAME	"misc/scsi_osd"
#define	SOSD_MOD_NAME		"drv/sosd"

#define	OSD_SETUP_CREATE_OBJECT		"osd_setup_create_object"
#define	OSD_SETUP_REMOVE_OBJECT		"osd_setup_remove_object"
#define	OSD_OPEN_BY_NAME		"osd_open_by_name"
#define	OSD_CLOSE			"osd_close"
#define	OSD_SETUP_READ			"osd_setup_read"
#define	OSD_SETUP_WRITE			"osd_setup_write"
#define	OSD_SETUP_WRITE_BP		"osd_setup_write_bp"
#define	OSD_SETUP_READ_BP		"osd_setup_read_bp"
#define	OSD_SUBMIT_REQ			"osd_submit_req"
#define	OSD_FREE_REQ			"osd_free_req"
#define	OSD_SETUP_SET_ATTR		"osd_setup_set_attr"
#define	OSD_SETUP_GET_ATTR		"osd_setup_get_attr"
#define	OSD_ADD_SET_PAGE_1ATTR_CDB	"osd_add_set_page_1attr_cdb"
#define	OSD_ADD_SET_PAGE_1ATTR_TO_REQ	"osd_add_set_page_1attr_to_req"
#define	OSD_ADD_SET_PAGE_ATTR_TO_REQ	"osd_add_get_page_attr_to_req"

typedef struct sam_sosd_vec {

	ddi_modhandle_t scsi_osd_hdl;
	ddi_modhandle_t sosd_hdl;

	/* osd_setup_create_object */
	osd_req_t *(*setup_create_object)(osd_dev_t, uint64_t, uint64_t,
	    uint16_t);

	/* osd_setup_remove_object */
	osd_req_t *(*setup_remove_object)(osd_dev_t, uint64_t, uint64_t);

	/* osd_open_by_name */
	int (*open_by_name)(char *, int, cred_t *, osd_dev_t *);

	/* osd_close */
	int (*close)(osd_dev_t, int, cred_t *);

	/* osd_setup_read */
	osd_req_t *(*setup_read)(osd_dev_t, uint64_t, uint64_t, uint64_t,
	    uint64_t, uint8_t, iovec_t *);

	/* osd_setup_write */
	osd_req_t *(*setup_write)(osd_dev_t, uint64_t, uint64_t, uint64_t,
	    uint64_t, uint8_t, iovec_t *);

	/* osd_setup_write_bp */
	osd_req_t *(*setup_write_bp)(osd_dev_t, uint64_t, uint64_t, uint64_t,
	    uint64_t, struct buf *);

	/* osd_setup_read_bp */
	osd_req_t *(*setup_read_bp)(osd_dev_t, uint64_t, uint64_t, uint64_t,
	    uint64_t, struct buf *);

	/* osd_submit_req */
	int (*submit_req)(osd_req_t *, void (* done)(osd_req_t  *, void *,
	    osd_result_t *), void *ct_priv);

	/* osd_free_req */
	void (*free_req)(osd_req_t *);

	/* osd_setup_set_attr */
	osd_req_t *(*setup_set_attr)(osd_dev_t, uint64_t, uint64_t);

	/* osd_setup_get_attr */
	osd_req_t *(*setup_get_attr)(osd_dev_t, uint64_t, uint64_t);

	/* osd_add_set_page_1attr_cdb */
	void (*add_set_page_1attr_cdb)(osd_req_t *, uint32_t, uint32_t,
	    uint16_t, char *);

	/* osd_add_set_page_1attr_to_req */
	void (*add_set_page_1attr_to_req)(osd_req_t *, uint32_t, uint32_t,
	    uint32_t, void *);

	/* osd_add_get_page_attr_to_req */
	void (*add_get_page_attr_to_req)(osd_req_t *, uint32_t, uint32_t,
	    void *);

} sam_sosd_vec_t;

#endif	/* _SAM_FS_OBJECT_H */
