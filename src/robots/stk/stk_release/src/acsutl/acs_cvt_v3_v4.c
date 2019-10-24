#ifndef lint
static char SccsId[] = "@(#) %full_name:  1/csrc/acs_cvt_v3_v4.c/2.1.2 %";
#endif
/**PROLOGUE**********************************************************
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      acs_cvt_v3_v4
 *
 * Description:
 *
 *      This routine converts a VERSION3 packet to a VERSION4 packet.
 *
 *      o All information will be preserved in the packet during the conversion.
 *
 *  o For the query response-- 
 *      - Convert from the new response structures to the VERSION 3 
 *        response structures.  
 *      - Copy the count from the new structures to the fixed portion.
 *      - Set the media type to MEDIA_TYPE_3480 for the query clean, 
 *        query scratch, and query volume responses.
 *      - Set the drive type to DRIVE_TYPE_4480 for the query drive, 
 *        query mount, and query mount scratch responses.
 *
 *  o For the vary response--
 *      - Convert STATUS_INVALID_DRIVE_TYPE -> STATUS_INVALID_DRIVE
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *  STATUS_INVALID_TYPE
 *  Others as returned by routines st_audit, st_enter, st_mount, 
 *  st_mount_scr, st_query, and st_vary.
 *
 * Parameters:
 *  *response_ptr       ACSLM input buffer area for V3 response
 *  *byte_count             Used as an input and output parameter.
 *              Input pararmeter- 	V3 byte count.
 *              Output pararmeter- 	V4 byte count.
 *
 * Implicit Inputs:     result_bfr
 *
 * Implicit Outputs:    result_bfr
 *
 * Considerations:  NONE
 *
 * Module Test Plan:    NONE
 *
 * Revision History:
 *
 *      Bernie Wolf 07-FEB-1994     Original.       @R222
 *      K. J. Stickney 18-MAY-1994  Changed media type value in 
 *                                  query clean, query scratch, and 
 *                                  query volume responses from UNKNOWN_
 *                                  MEDIA_TYPE to MEDIA_TYPE_3840 (0).
 *      K. J. Stickney 18-MAY-1994  Changed drive type value in 
 *                                  query drive, query mount, and 
 *                                  query mount scratch from UNKNOWN_
 *                                  DRIVE_TYPE to DRIVE_TYPE_4480.
 *                                  BR#27.
 *      K. J. Stickney 16-JUN-1994  Changes for BR#36
 *      Ken Stickney   17-Dec-1994  Removed v2_structs.h, lh_defs.h,
 *                                  and acslm.h. Replaced with
 *                                  acscvt.h. For AS400 portablility.
 *                                  acscvt.h supplies packet
 *                                  defintions independent of other
 *                                  system/platform data
 *                                  
 *
***ENDPROLOGUE******************************************************/

/*
 *      Header Files:
 */
#include <stdio.h>
#include <string.h>

#include "acssys.h"
#include "acscvt.h"
#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

#undef SELF
#define SELF "acs_cvt_v3_v4"

/*
 *      Global and Static Variable Declarations:
 */
static ALIGNED_BYTES result_bfr[MAX_MESSAGE_BLOCK];	/* results */

/*
 * Procedure declarations
 */
static void st_audit (RESPONSE_TYPE * resp_ptr,
              V3_RESPONSE_TYPE * v3_resp_ptr,
              size_t * byte_count);

static void st_enter (RESPONSE_TYPE * resp_ptr,
              V3_RESPONSE_TYPE * v3_resp_ptr,
              size_t * byte_count);

static void st_mount (RESPONSE_TYPE * resp_ptr,
              V3_RESPONSE_TYPE * v3_resp_ptr);

static void st_mount_scr (RESPONSE_TYPE * resp_ptr,
              V3_RESPONSE_TYPE * v3_resp_ptr);

static STATUS st_query (RESPONSE_TYPE * resp_ptr,
            V3_RESPONSE_TYPE * v3_resp_ptr,
            size_t * byte_count);

static STATUS st_vary (RESPONSE_TYPE * resp_ptr,
               V3_RESPONSE_TYPE * v3_resp_ptr,
               size_t * byte_count);

STATUS
acs_cvt_v3_v4 (
        ALIGNED_BYTES response_ptr,	/* acslm input V3 buffer area */
        size_t * byte_count)
{               /* response byte count */
  RESPONSE_TYPE *resp_ptr;	/* v4 pointer to response */
  V3_RESPONSE_TYPE *v3_resp_ptr;	/* v3 pointer to response */
  STATUS status = STATUS_SUCCESS;	/* return value */
  char *result_ptr;     /* converted V3 packet. */
  MESSAGE_HEADER *p_msghdr;
  COMMAND cmd;

  acs_trace_entry ();

  /* zero the result buffer out */
  memset ((char *) result_bfr, '\0', sizeof (result_bfr));

  /* set up some generic pointers */
  result_ptr = (char *) result_bfr;	/* V4 result data area */
  resp_ptr = (RESPONSE_TYPE *) result_ptr;	/* V4 result pointer */
  v3_resp_ptr = (V3_RESPONSE_TYPE *) response_ptr;	/* incomming V3 packet */

  /* Copy ipc header (same between versions) */
  resp_ptr->generic_response.request_header.ipc_header =
    v3_resp_ptr->generic_response.request_header.ipc_header;

  /* Change the version number */
    v3_resp_ptr->generic_response.request_header.message_header.version = 
        VERSION4;

  /* Copy message header (same between versions) */
  resp_ptr->generic_response.request_header.message_header =
    v3_resp_ptr->generic_response.request_header.message_header;

  /* Copy message status */
  resp_ptr->generic_response.response_status =
    v3_resp_ptr->generic_response.response_status;


  /* if response is an ACKNOWLEDGE the only change required is the version
     number of the packet */

  p_msghdr = &v3_resp_ptr->generic_response.request_header.message_header;

  if (p_msghdr->message_options & ACKNOWLEDGE) {
      status = STATUS_SUCCESS;
      p_msghdr->version = VERSION4;
      acs_trace_exit (status);
      return(status);
  }

  /* Otherwise, make changes to the rest of the packet. */
  else {

      cmd = resp_ptr->generic_response.request_header.message_header.command;
      switch (cmd) {
          case COMMAND_AUDIT:
               st_audit (resp_ptr, v3_resp_ptr, byte_count);
               break;

          case COMMAND_ENTER:
               st_enter (resp_ptr, v3_resp_ptr, byte_count);
               break;

          case COMMAND_MOUNT:
               st_mount (resp_ptr, v3_resp_ptr);
               break;

          case COMMAND_MOUNT_SCRATCH:
               st_mount_scr (resp_ptr, v3_resp_ptr);
               break;

          case COMMAND_QUERY:
               status = st_query (resp_ptr, v3_resp_ptr, byte_count);
               break;

          case COMMAND_VARY:
               status = st_vary (resp_ptr, v3_resp_ptr, byte_count);
               break;

          default:

          /* for other commands the packet is unchanged */
               status = STATUS_SUCCESS;
               acs_trace_exit (status);
               return (status);
      }
  }

  /* return the resulting VERSION3 response */
  if (status == STATUS_SUCCESS) {
    (void) memcpy ((char *) response_ptr, result_ptr, *byte_count);
  }

  acs_trace_exit (status);
  return (status);
}

/*
 * Name:
 *
 *      st_audit
 *
 * Description:
 *
 *      This module converts an audit response from VERSION3 to VERSION4
 *
 * Return Values:   NONE
 *
 * Parameters:
 *  *resp_ptr       ACSLM input buffer area for V4 response.
 *  *v3_resp_ptr        V3 packet being converted.
 *  *byte_count             Used as an input and output parameter.
 *              Input pararmeter-       V3 byte count.
 *              Output pararmeter-      V4 byte count.
 */
static void
st_audit (RESPONSE_TYPE * resp_ptr,	/* v4 response pointer */
      V3_RESPONSE_TYPE * v3_resp_ptr,	/* v3 response pointer */
      size_t * byte_count)
{               /* byte count in packet */
  V3_AUDIT_RESPONSE *v3_audit_ptr;	/* Version 3 audit response pointer */
  AUDIT_RESPONSE *v4_audit_ptr;	/* Version 4 audit response pointer */
  V3_EJECT_RESPONSE *v3_eject_ptr;	/* Version 3 eject response pointer */
  EJECT_RESPONSE *v4_eject_ptr;	/* Version 4 eject response pointer */
  int i;            /* loop index */


  /* initialize */
  v3_audit_ptr = &v3_resp_ptr->audit_response;
  v4_audit_ptr = &resp_ptr->audit_response;
  v3_eject_ptr = &v3_resp_ptr->eject_response;
  v4_eject_ptr = &resp_ptr->eject_response;

  /* Determine if this is an intermediate eject response or a final 
   * audit response by checking message options INTERMEDIATE bit.
   * In intermediate responses, volume information about ejected volumes
   * may contain volume status that must me converted.  This information
   * is not returned in the final response.
   */

  if (v3_audit_ptr->request_header.message_header.message_options &
      INTERMEDIATE) {
    /* copy the fixed portion */
    v4_eject_ptr->count = v3_eject_ptr->count;
    v4_eject_ptr->cap_id = v3_eject_ptr->cap_id;

    /* copy the variable portion */
    for (i = 0; i < (int) v3_eject_ptr->count; ++i) {
      v4_eject_ptr->volume_status[i].status =
    v3_eject_ptr->volume_status[i].status;
    }
    *byte_count = (char *) &v4_eject_ptr->volume_status[v4_eject_ptr->count]
      - (char *) v4_eject_ptr;
  }             /* if INTERMEDIATE response */
  else {
    /* Copy audit final response packet as is, note that byte count does
     * not change.
     */
    (void) memcpy ((char *) v4_audit_ptr, (char *) v3_audit_ptr, *byte_count);
  }
}

/*
 * Name:
 *
 *      st_enter
 *
 * Description:
 *
 *      This module converts an enter response from VERSION3 to VERSION4
 *
 * Return Values:   NONE
 *
 * Parameters:
 *  *resp_ptr       ACSLM input buffer area for V4 response.
 *  *v3_resp_ptr        V3 packet being converted.
 *  *byte_count             Used as an input and output parameter.
 *              Input pararmeter-       V3 byte count.
 *              Output pararmeter-      V4 byte count.
 */
static void
st_enter (RESPONSE_TYPE * resp_ptr,	/* v4 response pointer */
      V3_RESPONSE_TYPE * v3_resp_ptr,	/* v3 response pointer */
      size_t * byte_count)
{               /* byte count in packet */
  V3_ENTER_RESPONSE *v3_enter_ptr;	/* Version 3 enter response pointer */
  ENTER_RESPONSE *v4_enter_ptr;	/* Version 4 enter response pointer */
  int i;            /* loop index */


  /* initialize */
  v3_enter_ptr = &v3_resp_ptr->enter_response;
  v4_enter_ptr = &resp_ptr->enter_response;

  /* copy the fixed portion */
  v4_enter_ptr->count = v3_enter_ptr->count;
  v4_enter_ptr->cap_id = v3_enter_ptr->cap_id;

  /* copy the variable portion */
  for (i = 0; i < (int) v4_enter_ptr->count; ++i) {
    v4_enter_ptr->volume_status[i] =
      v3_enter_ptr->volume_status[i];
  }

  *byte_count = (char *) &v4_enter_ptr->volume_status[v4_enter_ptr->count] -
    (char *) v4_enter_ptr;
}

/*
 * Name:
 *
 *      st_mount
 *
 * Description:
 *
 *      This module converts a mount response from VERSION3 to VERSION4
 *  The byte count stays the same, so it is not passed as a parameter.
 *
 * Return Values:   NONE
 *
 * Parameters:
 *  *resp_ptr       ACSLM input buffer area for V4 response.
 *  *v3_resp_ptr        V3 packet being converted.
 */
static void
st_mount (RESPONSE_TYPE * resp_ptr,	/* v4 response pointer */
      V3_RESPONSE_TYPE * v3_resp_ptr)
{               /* v3 response pointer */
  V3_MOUNT_RESPONSE *v3_mount_ptr;	/* Version 3 mount response pointer */
  MOUNT_RESPONSE *v4_mount_ptr;	/* Version 4 mount response pointer */

  /* initialize */
  v3_mount_ptr = &v3_resp_ptr->mount_response;
  v4_mount_ptr = &resp_ptr->mount_response;

  /* copy the fixed portion (note that there is no variable portion) */
  v4_mount_ptr->vol_id = v3_mount_ptr->vol_id;
  v4_mount_ptr->drive_id = v3_mount_ptr->drive_id;

  v4_mount_ptr->message_status = v4_mount_ptr->message_status;
}

/*
 * Name:
 *
 *      st_mount_scr
 *
 * Description:
 *
 *      This module converts a mount scratch response from VERSION3 to VERSION4.
 *  The byte count stays the same, so it is not passed as a parameter.
 *
 * Return Values:   NONE
 *
 * Parameters:
 *  *resp_ptr       ACSLM input buffer area for V4 response.
 *  *v3_resp_ptr        V3 packet being converted.
 */
static void
st_mount_scr (RESPONSE_TYPE * resp_ptr,		/* v4 response pointer */
          V3_RESPONSE_TYPE * v3_resp_ptr)
{               /* v3 response pointer */
  V3_MOUNT_SCRATCH_RESPONSE *v3_msc_ptr;	/* V3 mount scratch
                           response pointer */
  MOUNT_SCRATCH_RESPONSE *v4_msc_ptr;	/* V4 mount scratch 
                       response pointer */

  /* initialize */
  v3_msc_ptr = &v3_resp_ptr->mount_scratch_response;
  v4_msc_ptr = &resp_ptr->mount_scratch_response;

  /* copy the fixed portion (note that there is no variable portion) */
  v4_msc_ptr->pool_id = v3_msc_ptr->pool_id;
  v4_msc_ptr->drive_id = v3_msc_ptr->drive_id;
  v4_msc_ptr->vol_id = v3_msc_ptr->vol_id;
  v4_msc_ptr->message_status = v3_msc_ptr->message_status;

}

/*
 * Name:
 *
 *      st_query
 *
 * Description:
 *
 *      This module converts a query response from VERSION3 to VERSION4
 *
 * Return Values:   
 *      STATUS_SUCCESS
 *  STATUS_INVALID_TYPE
 *
 * Parameters:
 *  *resp_ptr       ACSLM input buffer area for V4 response.
 *  *v3_resp_ptr        V3 packet being converted.
 *  *byte_count             Used as an input and output parameter.
 *              Input pararmeter-       V4 byte count.
 *              Output pararmeter-      V3 byte count.
 */
static STATUS
st_query (RESPONSE_TYPE * resp_ptr,	/* v4 response pointer  */
      V3_RESPONSE_TYPE * v3_resp_ptr,	/* v3 response pointer  */
      size_t * byte_count)
{               /* byte count in packet */
  V3_QUERY_RESPONSE *v3_qp;	/* V3 query response pointer    */
  QUERY_RESPONSE *v4_qp;	/* V4 query response pointer    */
  int i, j;         /* loop indices                 */
  int drive_count;      /* # of V3 drive status structs */

  /* initialize */
  v3_qp = &v3_resp_ptr->query_response;
  v4_qp = &resp_ptr->query_response;

  /* copy the fixed portion */
  v4_qp->type = v3_qp->type;

  /* copy the count and variable portion for each type */
  switch (v4_qp->type) {
  case TYPE_ACS:
    /* Copy count from new ACS response structure */
    v4_qp->status_response.acs_response.acs_count = v3_qp->count;

    /* Copy status from new ACS response structure. */
    for (i = 0; i < (int) v3_qp->count; i++) {
      v4_qp->status_response.acs_response.acs_status[i] =
    v3_qp->status_response.acs_status[i];
    }

    /* Calculate byte count. */
    *byte_count = (char *) &v4_qp->status_response.acs_response.acs_status[v3_qp->count]
      - (char *) v4_qp;
    break;

  case TYPE_CAP:
    /* Copy count from new CAP response structure */
    v4_qp->status_response.cap_response.cap_count = v3_qp->count;

    /* Copy status from new CAP response structure. */
    for (i = 0; i < (int) v3_qp->count; i++) {
      v4_qp->status_response.cap_response.cap_status[i] =
    v3_qp->status_response.cap_status[i];
    }

    /* Calculate byte count. */
    *byte_count = (char *) &v4_qp->status_response.cap_response.cap_status[v3_qp->count]
      - (char *) v4_qp;
    break;

  case TYPE_CLEAN:
    /* Copy count from new clean response structure */
    v4_qp->status_response.clean_volume_response.volume_count =
      v3_qp->count;

    /* Copy status from new clean response structure, omitting 
     * media type.
     */
    for (i = 0; i < (int) v3_qp->count; i++) {
      v4_qp->status_response.clean_volume_response.
    clean_volume_status[i].vol_id =
    v3_qp->status_response.clean_volume_status[i].vol_id;

      v4_qp->status_response.clean_volume_response.
    clean_volume_status[i].media_type = MEDIA_TYPE_3480;

      v4_qp->status_response.clean_volume_response.
    clean_volume_status[i].home_location =
    v3_qp->status_response.clean_volume_status[i].home_location;

      v4_qp->status_response.clean_volume_response.
    clean_volume_status[i].max_use =
    v3_qp->status_response.clean_volume_status[i].max_use;

      v4_qp->status_response.clean_volume_response.
    clean_volume_status[i].current_use =
    v3_qp->status_response.clean_volume_status[i].current_use;

      v4_qp->status_response.clean_volume_response.
    clean_volume_status[i].status =
    v3_qp->status_response.clean_volume_status[i].status;
    }

    /* Calculate byte count. */
    *byte_count = (char *) &v4_qp->status_response.clean_volume_response.
      clean_volume_status[v3_qp->count] - (char *) v4_qp;

    break;

  case TYPE_DRIVE:
    /* Copy count from new drive response structure */
    v4_qp->status_response.drive_response.drive_count = v3_qp->count;

    /* Copy status from new drive response structure, omitting 
     * drive type.
     */
    for (i = 0; i < (int) v3_qp->count; i++) {
      v4_qp->status_response.drive_response.drive_status[i].drive_id =
    v3_qp->status_response.drive_status[i].drive_id;

      v4_qp->status_response.drive_response.drive_status[i].drive_type =
             DRIVE_TYPE_4480;

      v4_qp->status_response.drive_response.drive_status[i].state =
    v3_qp->status_response.drive_status[i].state;

      v4_qp->status_response.drive_response.drive_status[i].vol_id =
    v3_qp->status_response.drive_status[i].vol_id;

      v4_qp->status_response.drive_response.drive_status[i].status =
    v3_qp->status_response.drive_status[i].status;
    }

    /* Calculate byte count. */
    *byte_count = (char *) &v4_qp->status_response.drive_response.
      drive_status[v3_qp->count] - (char *) v4_qp;

    break;

  case TYPE_LSM:
    /* Copy count from new LSM response structure */
    v4_qp->status_response.lsm_response.lsm_count = v3_qp->count;

    /* Copy status from new LSM response structure. */
    for (i = 0; i < (int) v3_qp->count; i++) {
      v4_qp->status_response.lsm_response.lsm_status[i] =
    v3_qp->status_response.lsm_status[i];
    }

    /* Calculate byte count. */
    *byte_count = (char *) &v4_qp->status_response.lsm_response.
      lsm_status[v3_qp->count] - (char *) v4_qp;

    break;

  case TYPE_MOUNT:
    /* Copy count from new mount response structure */

    v4_qp->status_response.mount_response.mount_status_count =
      v3_qp->count;

    /* Copy status from new mount response structure. */
    for (i = 0; i < (int) v3_qp->count; i++) {
      v4_qp->status_response.mount_response.mount_status[i].vol_id =
         v3_qp->status_response.mount_status[i].vol_id;

      v4_qp->status_response.mount_response.mount_status[i].status =
         v3_qp->status_response.mount_status[i].status;

      v4_qp->status_response.mount_response.mount_status[i].drive_count =
         v3_qp->status_response.mount_status[i].drive_count;

      drive_count = (int) v3_qp->status_response.mount_status[i].drive_count;

      /* Copy status from new drive response structure, omitting 
       * drive type.
       */
      for (j = 0; j < drive_count; j++) {
        v4_qp->status_response.mount_response.mount_status[i].
          drive_status[j].drive_id =
          v3_qp->status_response.mount_status[i].drive_status[j].drive_id;

        v4_qp->status_response.mount_response.mount_status[i].
          drive_status[j].state =
          v3_qp->status_response.mount_status[i].drive_status[j].state;

        v4_qp->status_response.mount_response.mount_status[i].
          drive_status[j].vol_id =
         v3_qp->status_response.mount_status[i].drive_status[j].vol_id;

        v4_qp->status_response.mount_response.mount_status[i].
          drive_status[j].drive_type = DRIVE_TYPE_4480;

        v4_qp->status_response.mount_response.mount_status[i].
           drive_status[j].status =
           v3_qp->status_response.mount_status[i].drive_status[j].status;

      }             /* end for (copy drive status structs) */
    }               /* end for (copy mount status structs) */

    /* Calculate byte count. */
    *byte_count = (char *) &v4_qp->status_response.mount_response.mount_status[v3_qp->count]
      - (char *) v4_qp;
    break;

  case TYPE_MOUNT_SCRATCH:
    /* Copy count from new mount scratch response structure */
    v4_qp->status_response.mount_scratch_response.msc_status_count =
      v3_qp->count;

    /* Copy status from new mount_scratch response structure. */
    for (i = 0; i < (int) v3_qp->count; i++) {
      v4_qp->status_response.mount_scratch_response.
         mount_scratch_status[i].pool_id =
         v3_qp->status_response.mount_scratch_status[i].pool_id;

      v4_qp->status_response.mount_scratch_response.
         mount_scratch_status[i].status =
         v3_qp->status_response.mount_scratch_status[i].status;

      v4_qp->status_response.mount_scratch_response.
         mount_scratch_status[i].drive_count =
         v3_qp->status_response.mount_scratch_status[i].drive_count;

      drive_count = (int) v3_qp->status_response.mount_scratch_status[i].
         drive_count;


      /* Copy status from new drive response structure, omitting
         * drive type.
       */
      for (j = 0; j < drive_count; j++) {
        v4_qp->status_response.mount_scratch_response.
          mount_scratch_status[i].drive_list[j].drive_id =
          v3_qp->status_response.mount_scratch_status[i].drive_list[j].
          drive_id;

        v4_qp->status_response.mount_scratch_response.
          mount_scratch_status[i].drive_list[j].state =
          v3_qp->status_response.mount_scratch_status[i].drive_list[j].
          state;

        v4_qp->status_response.mount_scratch_response.
          mount_scratch_status[i].drive_list[j].vol_id =
          v3_qp->status_response.mount_scratch_status[i].drive_list[j].
          vol_id;

        v4_qp->status_response.mount_scratch_response.
          mount_scratch_status[i].drive_list[j].drive_type = DRIVE_TYPE_4480;

        v4_qp->status_response.mount_scratch_response.
          mount_scratch_status[i].drive_list[j].status =
          v3_qp->status_response.mount_scratch_status[i].drive_list[j].
          status;

      }             /* end for (copy drive status structs) */
    }               /* end for (copy mount scratch status structs) */

    /* Calculate byte count. */
    *byte_count = (char *)
      &v4_qp->status_response.mount_scratch_response.mount_scratch_status[v3_qp->count]
      - (char *) v4_qp;
    break;

  case TYPE_PORT:
    /* Copy count from new port response structure */
    v4_qp->status_response.port_response.port_count = v3_qp->count;

    /* Copy status from new port response structure. */
    for (i = 0; i < (int) v3_qp->count; i++) {
      v4_qp->status_response.port_response.port_status[i] =
    v3_qp->status_response.port_status[i];
    }

    /* Calculate byte count. */
    *byte_count = (char *) &v4_qp->status_response.port_response.port_status[v3_qp->count]
      - (char *) v4_qp;
    break;

  case TYPE_POOL:
    /* Copy count from new pool response structure */
    v4_qp->status_response.pool_response.pool_count = v3_qp->count;

    /* Copy status from new pool response structure. */
    for (i = 0; i < (int) v3_qp->count; i++) {
      v4_qp->status_response.pool_response.pool_status[i] =
    v3_qp->status_response.pool_status[i];
    }

    /* Calculate byte count. */
    *byte_count = (char *) &v4_qp->status_response.pool_response.
      pool_status[v3_qp->count] - (char *) v4_qp;
    break;

  case TYPE_REQUEST:
    /* Copy count from new request response structure */
    v4_qp->status_response.request_response.request_count = v3_qp->count;

    /* Copy status from new request response structure. */
    for (i = 0; i < (int) v3_qp->count; i++) {
      v4_qp->status_response.request_response.request_status[i] =
    v3_qp->status_response.request_status[i];
    }

    /* Calculate byte count. */
    *byte_count = (char *) &v4_qp->status_response.request_response.request_status[v3_qp->count]
      - (char *) v4_qp;
    break;

  case TYPE_SCRATCH:
    /* Copy count from new scratch response structure */
    v4_qp->status_response.scratch_response.volume_count = v3_qp->count;

    /* Copy status from new scratch response structure, omitting media type. */
    for (i = 0; i < (int) v3_qp->count; i++) {
      v4_qp->status_response.scratch_response.scratch_status[i].vol_id =
    v3_qp->status_response.scratch_status[i].vol_id;

      v4_qp->status_response.scratch_response.scratch_status[i].media_type =
      MEDIA_TYPE_3480;

      v4_qp->status_response.scratch_response.scratch_status[i].home_location =
    v3_qp->status_response.scratch_status[i].home_location;

      v4_qp->status_response.scratch_response.scratch_status[i].pool_id =
    v3_qp->status_response.scratch_status[i].pool_id;

      v4_qp->status_response.scratch_response.scratch_status[i].status =
    v3_qp->status_response.scratch_status[i].status;
    }

    /* Calculate byte count. */
    *byte_count = (char *) &v4_qp->status_response.scratch_response.scratch_status[v3_qp->count]
      - (char *) v4_qp;
    break;

  case TYPE_SERVER:

    v4_qp->status_response.server_response.server_status.state =
      v3_qp->status_response.server_status[0].state;

    v4_qp->status_response.server_response.server_status.freecells =
      v3_qp->status_response.server_status[0].freecells;

    v4_qp->status_response.server_response.server_status.requests =
      v3_qp->status_response.server_status[0].requests;

    /* Calculate byte count. */
    *byte_count =
      (size_t) ((char *) &v4_qp->status_response.server_response - (char *) v4_qp) +
      sizeof (QU_SRV_RESPONSE);

    break;

  case TYPE_VOLUME:
    /* Copy count from new volume response structure */
    v4_qp->status_response.volume_response.volume_count = v3_qp->count;

    /* Copy status from new volume response structure, omitting media 
     * type.
     */
    for (i = 0; i < (int) v3_qp->count; i++) {
      v4_qp->status_response.volume_response.volume_status[i].vol_id =
        v3_qp->status_response.volume_status[i].vol_id;

      v4_qp->status_response.volume_response.volume_status[i].media_type =
	MEDIA_TYPE_3480;

      v4_qp->status_response.volume_response.volume_status[i].
        location_type =
        v3_qp->status_response.volume_status[i].location_type;

      if (v3_qp->status_response.volume_status[i].location_type ==LOCATION_CELL) {
         v4_qp->status_response.volume_response.volume_status[i].
           location.cell_id =
           v3_qp->status_response.volume_status[i].location.cell_id;
      }
      else {
        v4_qp->status_response.volume_response.volume_status[i].
          location.drive_id =
          v3_qp->status_response.volume_status[i].location.drive_id;
      }

      v4_qp->status_response.volume_response.volume_status[i].status =
        v3_qp->status_response.volume_status[i].status;

    }
    /* Calculate byte count. */
    *byte_count = (char *) &v4_qp->status_response.volume_response.
      volume_status[v3_qp->count] - (char *) v4_qp;
    break;

  default:
    /* Return error */
    return (STATUS_INVALID_TYPE);

  }             /* end switch (v3_qp->type) */

  return (STATUS_SUCCESS);
}

/*
 * Name:
 *
 *      st_vary
 *
 * Description:
 *
 *      This module converts a vary response from VERSION3 to VERSION4
 *
 * Return Values:   
 *      STATUS_SUCCESS
 *  STATUS_INVALID_TYPE
 *
 * Parameters:
 *  *resp_ptr       ACSLM input buffer area for V4 response.
 *  *v3_resp_ptr        V3 packet being converted.
 *  *byte_count             Used as an input and output parameter.
 *              Input pararmeter-       V3 byte count.
 *              Output pararmeter-      V4 byte count.
 */
static STATUS
st_vary (RESPONSE_TYPE * resp_ptr,	/* v4 response pointer */
     V3_RESPONSE_TYPE * v3_resp_ptr,	/* v3 response pointer */
     size_t * byte_count)
{               /* byte count in packet */
  V3_VARY_RESPONSE *v3_vary_ptr;	/* Version 3 vary response pointer */
  VARY_RESPONSE *v4_vary_ptr;	/* Version 4 vary response pointer */
  int i;            /* loop index */


  /* initialize */
  v3_vary_ptr = &(v3_resp_ptr->vary_response);
  v4_vary_ptr = &(resp_ptr->vary_response);

  /* copy the fixed portion */
  v4_vary_ptr->state = v3_vary_ptr->state;
  v4_vary_ptr->type = v3_vary_ptr->type;
  v4_vary_ptr->count = v3_vary_ptr->count;

  /* copy the variable portion */
  switch (v4_vary_ptr->type) {
  case TYPE_ACS:
    /* No changes, copy over to v4 packet. */
    for (i = 0; i < (int) v3_vary_ptr->count; i++) {
      v4_vary_ptr->device_status.acs_status[i] =
    v3_vary_ptr->device_status.acs_status[i];
    }
    break;

  case TYPE_CAP:
    /* No changes, copy over to v4 packet. */
    for (i = 0; i < (int) v3_vary_ptr->count; i++) {
      v4_vary_ptr->device_status.cap_status[i] =
    v3_vary_ptr->device_status.cap_status[i];
    }
    break;

  case TYPE_LSM:
    /* No changes, copy over to v4 packet. */
    for (i = 0; i < (int) v3_vary_ptr->count; i++) {
      v4_vary_ptr->device_status.lsm_status[i] =
    v3_vary_ptr->device_status.lsm_status[i];
    }
    break;

  case TYPE_PORT:
    /* No changes, copy over to v4 packet. */
    for (i = 0; i < (int) v3_vary_ptr->count; i++) {
      v4_vary_ptr->device_status.port_status[i] =
    v3_vary_ptr->device_status.port_status[i];
    }
    break;

  case TYPE_DRIVE:
    /* Copy drive status. */
    for (i = 0; i < (int) v3_vary_ptr->count; i++) {
      v4_vary_ptr->device_status.drive_status[i] =
    v3_vary_ptr->device_status.drive_status[i];

    }               /* end for (copy drive status) */

    break;

  default:
    /* Return error */
    return (STATUS_INVALID_TYPE);

  }             /* end switch(v4_vary_ptr->type) */

  *byte_count = (char *) &v4_vary_ptr->device_status.
    drive_status[v3_vary_ptr->count] - (char *) v4_vary_ptr;

  return (STATUS_SUCCESS);
}
