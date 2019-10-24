#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  1/csrc/acs_cvt_v4_v3.c/2.1.2 %";
#endif
/**PROLOGUE**********************************************************
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      acs_cvt_v4_v3
 *
 * Description:
 *
 *      This routine will convert a VERSION4 packet to a VERSION3 packet.
 *
 *      o The steps taken to change a VERSION4 packet to a VERSION3 packet are
 *        as follows:
 *
 *          * convert the packet
 *          * All data (IPC headers, message headers, response_status,
 *            variable portion, etc) from the VERSION4 packet gets
 *            copied into the VERSION3 packet.
 *          * If the request is a mount scratch request, remove
 *            the media_type field.
 *      * If the request is a query request, copy the request into the new
 *        query request structures.  If the type is TYPE_MOUNT_SCRATCH,
 *        remove the media type count.
 *
 *      o return the new packet and it's length to the input process.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *      STATUS_INVALID_COMMAND
 *  Others as returned by routines st_query and st_mount_scr.
 *
 * Parameters:
 *  *v3_req_ptr pointer to input buffer area
 *  *byte_count     Used as both input and output parameter.
 *          Input Parameter:	Number of bytes in V3 request.
 *          Output Paramater:	Number of bytes in V4 request.
 *
 * Implicit Inputs: NONE
 *
 * Implicit Outputs:    NONE
 *
 * Considerations: If the query command is incorrect, or the query type is
 *  incorrect, or the mount scratch request does not contain one drive
 *  identifier, the appropiate error status codes are returned.
 *
 * Module Test Plan:    NONE
 *
 * Revision History:
 *
 *      Bernie Wolf 01/18/93 - Original    @R222
 *      Ken Stickney 05/18/94 - Added check for media types not supported
 *                              in V3 servers or lower, for the query mount
 *                              scratch and mount scratch requests.
 *                              BR#27.
 *      Ken Stickney 08/24/94 - Added code to strip off BYPASS flag in
 *                              message options, since it is not
 *                              supported in ACSLS releases below 5.0.
 *      Ken Stickney 12/17/94 - Removed v2_structs.h, lh_defs.h,
 *                              and acslm.h. Replaced with
 *                              acscvt.h. For AS400 portablility.
 *                              acscvt.h supplies packet
 *                              defintions independent of other
 *                              system/platform data
 *
***ENDPROLOGUE******************************************************/

/*
 *      Header Files:
 */
#include <stdio.h>
#include <string.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"

#include "acscvt.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

#undef SELF
#define SELF "acs_cvt_v4_v3"
 

/*
 *      Global and Static Variable Declarations:
 */
static ALIGNED_BYTES result_bfr[MAX_MESSAGE_BLOCK];	/* results */

/*
 * Procedure declarations
 */
static STATUS st_query (REQUEST_TYPE * req_ptr,
            V3_REQUEST_TYPE * v3_req_ptr,
            size_t * byte_count);

static STATUS st_mount_scr (REQUEST_TYPE * req_ptr,
                V3_REQUEST_TYPE * v3_req_ptr,
                size_t * byte_count);


STATUS
acs_cvt_v4_v3 (
        ALIGNED_BYTES request_ptr,		/* acslm input buffer area */
        size_t * byte_count	/* number of bytes in request. */
)
{
  V3_REQUEST_TYPE *v3_req_ptr;	/* V3 pointer to request type msg's */
  char *result_ptr;     /* ptr to result buffer area        */
  STATUS status = STATUS_SUCCESS;
  REQUEST_TYPE * req_ptr;

  acs_trace_entry ();

  req_ptr = (REQUEST_TYPE *) request_ptr;

  /* zero the result buffer out */
  memset ((char *) result_bfr, '\0', sizeof (result_bfr));

  /* set up some generic pointers */
  result_ptr = (char *) result_bfr;	/* result's data area */
  v3_req_ptr = (V3_REQUEST_TYPE *) result_ptr;	/* result pointer */

  /* copy ipc header (same between versions) */
  v3_req_ptr->generic_request.ipc_header =
    req_ptr->generic_request.ipc_header;

  /* Change version number to version 3 */
  req_ptr->generic_request.message_header.version = VERSION3;

  /* Strip out the BYPASS option from the message_options, since it 
   * is only supported for the mount request in Release 5.0 and higher
   */
  req_ptr->generic_request.message_header.message_options &= ~BYPASS;

  /* copy message header (same between versions) */
  v3_req_ptr->generic_request.message_header =
    req_ptr->generic_request.message_header;

  /*
   * copy the remainder of the request packet
   * Calculate the number of bytes (for this command) from the beginning
   * to the fixed portion of the VERSION3 packet.
   */
  switch (req_ptr->generic_request.message_header.command) {

  case COMMAND_MOUNT_SCRATCH:
    status = st_mount_scr (req_ptr, v3_req_ptr, byte_count);
    break;

  case COMMAND_QUERY:
    status = st_query (req_ptr, v3_req_ptr, byte_count);
    break;

  case COMMAND_AUDIT:
  case COMMAND_CANCEL:
  case COMMAND_CLEAR_LOCK:
  case COMMAND_DEFINE_POOL:
  case COMMAND_DELETE_POOL:
  case COMMAND_DISMOUNT:
  case COMMAND_EJECT:
  case COMMAND_ENTER:
  case COMMAND_IDLE:
  case COMMAND_LOCK:
  case COMMAND_MOUNT:
  case COMMAND_QUERY_LOCK:
  case COMMAND_SET_CAP:
  case COMMAND_SET_CLEAN:
  case COMMAND_SET_OWNER:
  case COMMAND_SET_SCRATCH:
  case COMMAND_START:
  case COMMAND_UNLOCK:
  case COMMAND_VARY:
    /* commands did't change from VERSION3 to VERSION4 */
      acs_trace_exit (STATUS_SUCCESS);
      return (STATUS_SUCCESS);

  default:
    status = STATUS_INVALID_COMMAND;
    break;
  }

  if (status == STATUS_SUCCESS) {
    /* stuff calculated byte count into the resulting request pointer */
    v3_req_ptr->generic_request.ipc_header.byte_count = *byte_count;

    /* return the resulting VERSION3 pkt */
    memcpy ((char *) req_ptr, result_ptr, *byte_count);
  }
  acs_trace_exit (status);
  return (status);
}

/*
 * Name:
 *
 *      st_query
 *
 * Description:
 *
 *      This routine converts a query request from VERSION4 to VERSION3
 *
 * Return Values:     
 *  STATUS_SUCCESS
 *  STATUS_INVALID_TYPE
 *  STATUS_INVALID_MEDIA_TYPE
 *
 * Parameters:
 *  *req_ptr    V4 pointer to query request
 *  *v3_req_ptr V3 pointer to converted query request from
 *  *byte_count Used as input and output parameter.
 *          Input Parameter:    Number of bytes in V4 request.
 *                      This becomes number of bytes in
 *                      the V3 request if unchanged.
 *          Output Parameter:   Number of bytes in converted
 *                      V3 request.
 *
 * Implicit Inputs:     NONE
 *
 * Implicit Outputs:    result_bfr
 *
 * Considerations:  NONE
 */
static STATUS
st_query (
       REQUEST_TYPE * req_ptr,	/* V4 pointer to request type msg's */
       V3_REQUEST_TYPE * v3_req_ptr,	/* V3 pointer to request type msg's */
       size_t * byte_count	/* number of bytes in request. */
)
{
  QUERY_REQUEST *qp;    	/* VERSION4 query request */
  V3_QUERY_REQUEST *v3_qp;	/* VERSION3 query request */
  unsigned short i;            /* loop index */
  MEDIA_TYPE mtype;
  /* to v4.  MAX_ID is the max number.
   */

  qp = (QUERY_REQUEST *) req_ptr;
  v3_qp = (V3_QUERY_REQUEST *) v3_req_ptr;

  /* copy fixed portion */
  v3_qp->type = qp->type;

  /* copy variable portion */
  switch (qp->type) {
  case TYPE_ACS:
    /* Copy count to old ACS criteria structure */
    v3_qp->count = qp->select_criteria.acs_criteria.acs_count;

    /* Copy identifiers to new ACS criteria structure. */
    for (i = 0; i < v3_qp->count; i++) {
      v3_qp->identifier.acs_id[i] =
    qp->select_criteria.acs_criteria.acs_id[i];
    }

    /* Calculate byte count. */
    *byte_count =
      ((char *) &v3_qp->identifier.acs_id[v3_qp->count]
       - (char *) v3_qp);
    break;

  case TYPE_CAP:
    /* Copy count to new cap criteria structure. */
    v3_qp->count = qp->select_criteria.cap_criteria.cap_count;

    /* Copy identifiers to new cap criteria.structure. */
    for (i = 0; i < v3_qp->count; i++) {
      v3_qp->identifier.cap_id[i] =
    qp->select_criteria.cap_criteria.cap_id[i];
    }

    /* Calculate byte count. */
    *byte_count =
      ((char *) &v3_qp->identifier.cap_id[v3_qp->count]
       - (char *) v3_qp);
    break;

  case TYPE_CLEAN:
  case TYPE_MOUNT:
  case TYPE_VOLUME:
    /* These query types all use the new volume criteria structure. 
     * Copy count to new volume criteria structure. 
     */
    v3_qp->count = qp->select_criteria.vol_criteria.volume_count;

    /* Copy identifiers to new volume criteria structure. */
    for (i = 0; i < v3_qp->count; i++) {
      v3_qp->identifier.vol_id[i] =
    qp->select_criteria.vol_criteria.volume_id[i];
    }
    /* Calculate byte count. */
    *byte_count =
      ((char *) &v3_qp->identifier.vol_id[v3_qp->count]
       - (char *) v3_qp);
    break;

  case TYPE_DRIVE:
    /* Copy count to new drive criteria structure. */
    v3_qp->count = qp->select_criteria.drive_criteria.drive_count;

    /* Copy identifiers to new drive criteria structure. */
    for (i = 0; i < v3_qp->count; i++) {
      v3_qp->identifier.drive_id[i] =
    qp->select_criteria.drive_criteria.drive_id[i];
    }

    /* Calculate byte count. */
    *byte_count =
      ((char *) &v3_qp->identifier.drive_id[v3_qp->count]
       - (char *) v3_qp);
    break;

  case TYPE_LSM:
    /* Copy count to new LSM criteria structure. */
    v3_qp->count = qp->select_criteria.lsm_criteria.lsm_count;

    /* Copy identifiers to new LSM criteria structure. */
    for (i = 0; i < v3_qp->count; i++) {
      v3_qp->identifier.lsm_id[i] =
    qp->select_criteria.lsm_criteria.lsm_id[i];
    }

    /* Calculate byte count. */
    *byte_count =
      ((char *) &v3_qp->identifier.lsm_id[v3_qp->count]
       - (char *) v3_qp);
    break;

  case TYPE_MOUNT_SCRATCH:
    /* Copy count to new mount scratch criteria structure. 
     * Note that this down level count is the number of pool identifiers. 
     */
    v3_qp->count = qp->select_criteria.mount_scratch_criteria.pool_count;

    mtype = qp->select_criteria.mount_scratch_criteria.media_type;

    /* Check for media types not supported by V3 or earlier servers */
    if( mtype != ANY_MEDIA_TYPE && mtype != ALL_MEDIA_TYPE &&
	mtype != MEDIA_TYPE_3480) {
	return STATUS_INVALID_MEDIA_TYPE;
    }

    /* Copy pool identifiers to new mount scratch criteria structure. */
    for (i = 0; i < v3_qp->count; i++) {
      v3_qp->identifier.pool_id[i] =
    qp->select_criteria.mount_scratch_criteria.pool_id[i];
    }

    /* Calculate byte count. */
    *byte_count =
      ((char *) &v3_qp->identifier.pool_id[v3_qp->count] - (char *) v3_qp);
    break;

  case TYPE_POOL:
  case TYPE_SCRATCH:
    /* These query types use the new pool criteria structure. 
     * Copy count to the new pool criteria structure. 
     */
    v3_qp->count = qp->select_criteria.pool_criteria.pool_count;

    /* Copy identifiers to new pool criteria structure. */
    for (i = 0; i < v3_qp->count; i++) {
      v3_qp->identifier.pool_id[i] = qp->select_criteria.pool_criteria.pool_id[i];
    }

    /* Calculate byte count. */
    *byte_count =
      ((char *) &v3_qp->identifier.pool_id[v3_qp->count]
       - (char *) v3_qp);
    break;

  case TYPE_PORT:
    /* Copy count to new port criteria structure. */
    v3_qp->count = qp->select_criteria.port_criteria.port_count;

    /* Copy identifiers to new port criteria structure. */
    for (i = 0; i < v3_qp->count; i++) {
      v3_qp->identifier.port_id[i] =
    qp->select_criteria.port_criteria.port_id[i];
    }

    /* Calculate byte count. */
    *byte_count =
      ((char *) &v3_qp->identifier.port_id[v3_qp->count]
       - (char *) v3_qp);
    break;

  case TYPE_REQUEST:
    /* Copy count to new request criteria structure. */
    v3_qp->count = qp->select_criteria.request_criteria.request_count;

    /* Copy identifiers to new request criteria structure. */
    for (i = 0; i < v3_qp->count; i++) {
      v3_qp->identifier.request[i] =
    qp->select_criteria.request_criteria.request_id[i];
    }

    /* Calculate byte count. */
    *byte_count =
      ((char *) &v3_qp->identifier.request[v3_qp->count]
       - (char *) v3_qp);
    break;

  case TYPE_SERVER:
    /* Note that the query server request uses no criteria structures, only
     * sets type to TYPE_SERVER, therefore, nothing to convert.
     * Don't recalculate byte count, just break and return. 
     */
    break;

  case TYPE_MIXED_MEDIA_INFO:
    /* Note that this type is new for Version 4 packets, so there is 
     * nothing to translate this request into. Just send it on to the
     * server and let it reject the request with STATUS_INVALID TYPE.
     */
    break;

  default:
    /* Note that the type is also checked in routine lm_req_valid for
     * packets already in V4 format.
     */
    return (STATUS_INVALID_TYPE);

  }             /* End switch */

  return (STATUS_SUCCESS);
}

/*
 * Name:
 *
 *      st_mount_scr
 *
 * Description:
 *
 *      This routine converts a mount scratch request from VERSION3 to VERSION4
 *
 * Return Values:
 *  STATUS_SUCCESS
 *  STATUS_COUNT_TOO_LARGE
 *  STATUS_COUNT_TOO_SMALL
 *  STATUS_INVALID_MEDIA_TYPE
 *
 * Parameters:
 *  *req_ptr    V4 pointer to converted query request
 *  *v3_req_ptr V3 pointer to query request from input buffer.
 *  *byte_count Used as input and output parameter.
 *          Input Parameter:    Number of bytes in V3 request.
 *                      This becomes number of bytes in
 *                      the V4 request if unchanged.
 *          Output Parameter:   Number of bytes in converted
 *                      V4 request.
 *
 * Implicit Inputs:     NONE
 *
 * Implicit Outputs:    NONE
 *
 * Considerations:  NONE
 *
 */
static STATUS
st_mount_scr (
           REQUEST_TYPE * req_ptr,	/* V4 pointer to request type msg's */
           V3_REQUEST_TYPE * v3_req_ptr,	/* V3 pointer to request type msg's */
           size_t * byte_count	/* number of bytes in request. */
)
{

  MOUNT_SCRATCH_REQUEST *mscp;	/* VERSION4 mount scratch request */
  V3_MOUNT_SCRATCH_REQUEST *v3_mscp;	/* VERSION3 mount scratch request */

  MEDIA_TYPE mtype;

  mscp = (MOUNT_SCRATCH_REQUEST *) req_ptr;
  v3_mscp = (V3_MOUNT_SCRATCH_REQUEST *) v3_req_ptr;

  /* Copy fixed portion */
  v3_mscp->pool_id = mscp->pool_id;
  v3_mscp->count = mscp->count;

  mtype = mscp->media_type;

  /* Check for media types not supported by V3 or earlier servers */
  if( mtype != ANY_MEDIA_TYPE && mtype != ALL_MEDIA_TYPE &&
	mtype != MEDIA_TYPE_3480) {
	return STATUS_INVALID_MEDIA_TYPE;
  }

  /* Copy variable portion. 
   * Note that for V4, count may only be set to 1. 
   */

  if (v3_mscp->count < 1) {
    return (STATUS_COUNT_TOO_SMALL);
  }
  else if (v3_mscp->count > 1) {
    return (STATUS_COUNT_TOO_LARGE);
  }
  else {
    v3_mscp->drive_id[0] = mscp->drive_id[0];
  }

  /* Calculate byte count. */
  *byte_count = ((char *) &v3_mscp->drive_id[v3_mscp->count] - (char *) v3_mscp);

  return (STATUS_SUCCESS);
}
