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

/* OSD/FS Interface VERSION 2 */

#ifndef	_SYS_OSD_H
#define	_SYS_OSD_H


#pragma ident	"%Z%%M%	%I%	%E% SMI"


#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/buf.h>
#include <sys/cred.h>
#include <sys/uio.h>
#include "scsi_osd.h"

/*
 * Standard Globals
 */
#define OSD_API_REV	2
#define	OSD_CDB_SIZE	224


/*
 * OSD device access handle (opaque to the client)
 */
typedef	void * osd_dev_t;


/*
 * Defined values for err_code (in osd_result_t)
 */

#define	OSD_SUCCESS		0	/* The requested operation has been */
					/* performed successfully */
#define	OSD_FAILURE		1	/* General failure code */
#define	OSD_RECOVERED_ERROR	2	/* Command completed with sense data */
#define	OSD_CHECK_CONDITION	3	/* A SCSI check condition occurred */
					/* while performing the request. */
#define	OSD_INVALID		4	/* An invalid parameter was detected */
					/* in the request */
#define	OSD_TOOBIG		5	/* The requested data transfer is */
					/* too large for the system. */
#define	OSD_BADREQUEST		6	/* The requested operation cannot be */
					/* performed due to an unrecoverable */
					/* error in the request */
#define	OSD_LUN_ERROR		7	/* The requested operation cannot be */
					/* performed due to an unrecoverable */
					/* error in the LUN */
#define	OSD_LUN_FAILURE		8	/* The LUN has become permanently */
					/* unusable or inacessible */
#define	OSD_BUSY		9	/* The requested operation cannot be */
					/* performed at this time */
#define	OSD_RESERVATION_CONFLICT 10	/* A SCSI Reservation conflict was */
					/* encontered at the LUN */
#define	OSD_RESIDUAL		11	/* One or more data transfers for the */
					/* request have a nonzero residual */
#define	OSD_NORESOURCES		12	/* Insufficient resources available */
					/* to complete the request */


/*
 * Data struct at errp when err_type is OSD_ERRTYPE_RESID. These
 * fields correspond to the Data In and Data Out memory segments in the
 * the osd_request. When nonzero, each field contains a 'residual'
 * value that is the number of data bytes from the I/O operation that
 * were not transferred as requested. A residual value of zero indicates
 * that all the requested bytesi for the segment were transferred.
 * The residual value is set upon I/O completion, before the done()
 * function is called to return the osd_req to the client.
 */
typedef struct osd_resid {
	/* Data-In segments */
	uint64_t	ot_in_command_resid;	/* DI Command/Parameter Data */
	uint64_t	ot_in_ret_attr_resid;	/* DI Retrieved Attributes */
	uint64_t	ot_in_integrity_resid;	/* DI Integrity Check Value */

	/* Data-Out segments */
	uint64_t	ot_out_command_resid;	/* DO Command/Parameter Data */
	uint64_t	ot_out_set_attr_resid;	/* DO Set Attributes */
	uint64_t	ot_out_get_attr_resid;	/* DO Get Attributes */
	uint64_t	ot_out_integrity_resid;	/* DO Integrity Check Value */
} osd_resid_t;


/*
 * Result structure.
 */
typedef struct osd_result {
	uint8_t		err_code;
	uint16_t	err_field_offset;
	uint16_t	service_action;
	osd_resid_t	resid_data;
	uint32_t	errp_len;
	void		*errp;
	uint32_t	sense_data_len;
	void		*sense_data;
} osd_result_t;



/*
 * OSD Request structure
 */
typedef struct osd_req {
	osd_dev_t	oh;
	uint8_t		state;
	uint16_t	service_action;
	osd_result_t	result;
	void		(*done)(struct osd_req *, void *, struct osd_result *);
	void		*client_private;
	void		*sosd_req;
} osd_req_t;

/*
 * Defined values for state (above) - Set by the driver.
 */
#define	OSD_REQ_ALLOCATED	0x1
#define	OSD_REQ_SUBMITTED	0x2
#define	OSD_REQ_COMPLETED	0x3


/* Defined flags for osd_set_default_req()/osd_modify_req() calls */
#define	OSD_O_DPO		0x00000001	/* Disable page out */
#define	OSD_O_FUA		0x00000002	/* Force unit access */

#define	OSD_O_IM_NONE		0x00000100	/* NONE */
#define	OSD_O_IM_STRICT		0x00000200	/* STRICT */
#define	OSD_O_IM_RANGE		0x00000400	/* RANGE */
#define	OSD_O_IM_FUNCTIONAL	0x00000800	/* FUNCTIONAL */
#define	OSD_O_IM_VENDOR		0x00001000	/* VENDOR SPECIFIC */

#define	OSD_O_TC_DEFAULT	0x00002000	/* Default updates */
#define	OSD_O_TC_DISABLED	0x00004000	/* No updates */


/* Function calls */

/*
 * osd_setup_xxx: Function calls to allocate and setup the OSD Request
 */
osd_req_t *osd_setup_format_osd(osd_dev_t oh, uint64_t formatted_capacity);

osd_req_t *osd_setup_create_partition(osd_dev_t oh,
    uint64_t requested_partition_id);

osd_req_t *osd_setup_remove_partition(osd_dev_t oh, uint64_t partition_id);

osd_req_t *osd_setup_create_object(osd_dev_t oh,
    uint64_t partition_id, uint64_t requested_object_id, uint16_t num_objs);

osd_req_t *osd_setup_remove_object(osd_dev_t oh,
    uint64_t partition_id, uint64_t object_id);

osd_req_t *osd_setup_create_and_write(osd_dev_t oh,
    uint64_t partition_id, uint64_t object_id, uint64_t len,
    uint64_t starting_byte_addr, uint8_t num_iovecs, iovec_t *iov);

osd_req_t *osd_setup_write(osd_dev_t oh,
    uint64_t partition_id, uint64_t object_id, uint64_t len,
    uint64_t starting_byte_addr, uint8_t num_iovecs, iovec_t *iov);

osd_req_t *osd_setup_read(osd_dev_t oh,
    uint64_t partition_id, uint64_t object_id, uint64_t len,
    uint64_t starting_byte_addr, uint8_t num_iovecs, iovec_t *iov);

osd_req_t *osd_setup_append(osd_dev_t oh, uint64_t partition_id,
    uint64_t object_id, uint64_t len, uint8_t num_iovecs, iovec_t *iov);

osd_req_t *osd_setup_create_and_write_bp(osd_dev_t oh,
    uint64_t partition_id, uint64_t object_id, uint64_t len,
    uint64_t starting_byte_addr, struct buf *bp);

osd_req_t *osd_setup_write_bp(osd_dev_t oh,
    uint64_t partition_id, uint64_t object_id, uint64_t len,
    uint64_t starting_byte_addr, struct buf *bp);

osd_req_t *osd_setup_read_bp(osd_dev_t oh,
    uint64_t partition_id, uint64_t object_id, uint64_t len,
    uint64_t starting_byte_addr, struct buf *bp);

osd_req_t *osd_setup_append_bp(osd_dev_t oh, uint64_t partition_id,
    uint64_t object_id, uint64_t len, struct buf *bp);

osd_req_t *osd_setup_clear(osd_dev_t oh, uint64_t partition_id,
    uint64_t object_id, uint64_t len, uint64_t starting_byte_address);

osd_req_t *osd_setup_punch(osd_dev_t oh, uint64_t partition_id,
    uint64_t user_object_id, uint64_t length, uint64_t starting_byte_address);

osd_req_t *osd_setup_object_structure_check(osd_dev_t oh,
    uint64_t partition_id);

osd_req_t *osd_setup_flush(osd_dev_t oh, uint8_t flush_scope,
    uint64_t partition_id, uint64_t object_id, uint64_t flush_len,
    uint64_t flush_start_byte_addr);

osd_req_t *osd_setup_flush_osd(osd_dev_t oh, uint8_t flush_scope);

osd_req_t *osd_setup_set_attr(osd_dev_t oh,
    uint64_t partition_id, uint64_t object_id);

osd_req_t *osd_setup_get_attr(osd_dev_t oh,
    uint64_t partition_id, uint64_t object_id);

/*
 * osd_add_xxx: Function calls to add additional parameters to an OSD Request
 *              that was already setup (using osd_setup_xxx)
 */
void osd_add_set_page_1attr_cdb(osd_req_t *req,
    uint32_t attr_page, uint32_t attr_num, uint16_t attr_len, char *attr_val);

void osd_add_set_page_1attr_to_req(osd_req_t *req,
    uint32_t set_attr_page, uint32_t set_attr_num,
    uint32_t set_attr_len, void *set_attr_val);

void osd_add_get_page_attr_to_req(osd_req_t *req,
    uint32_t get_attr_page, uint32_t get_attr_alloc_len, void *ret_attr);

void osd_add_set_list_attr_to_req(osd_req_t *req,
    uint32_t set_attr_page, uint32_t set_attr_num,
    uint32_t set_attr_len, void *set_attr_val);

void osd_add_get_list_attr_to_req(osd_req_t *req,
    uint32_t get_attr_page, uint32_t get_attr_num,
    uint32_t get_attr_alloc_len, void *ret_attr_buf);

void osd_add_capability_security_to_req(osd_req_t *req,
    osd_capability_format_t *ocap, osd_security_parameters_t *osec);

void osd_add_flags_to_req(osd_req_t *req, uint32_t flags);

/*
 * osd_submit_req: Function to send the request to the OSD target
 */
int osd_submit_req(osd_req_t *req,
    void (* done)(osd_req_t  *, void *, osd_result_t *), void *ct_priv);

/*
 * osd_get_result: Function to get result of a completed (or failed) Request
 */
void osd_get_result(osd_req_t *req, osd_result_t **res);

/*
 * osd_free_req: Function to release resources associated with an OSD Request
 */
void osd_free_req(osd_req_t *req);

/*
 * osd_open_by_name: Function to open an OSD device and get a device handle
 */
int osd_open_by_name(char *path, int flags, cred_t *cr, osd_dev_t *oh);

/*
 * osd_close: Function to close an OSD device and release this device handle
 */
int osd_close(osd_dev_t oh, int flags, cred_t *cr);

/*
 * osd_get_xxx: Miscellaneous utility APIs
 */
int osd_get_max_dma_size(osd_dev_t oh);

uint32_t osd_get_api_revision();


#ifdef	__cplusplus
}
#endif

#endif	/* _SYS_OSD_H */
