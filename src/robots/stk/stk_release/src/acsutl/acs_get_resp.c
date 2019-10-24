#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:	1/csrc/acs_get_resp.c/3 %";
#endif
/*
 *
 *                             Copyright (1993-2001)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_get_response()
 *
 * Description:
 *      Converts down-level responses from a down-level server into
 *      the response version supported by the toolkit.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; 
 *     If acs_ipc_read returns STATUS_PENDING, STATUS_PENDING;
 *     otherwise STATUS_IPC_FAILURE.
 *
 * Parameters:
 *
 *    rbuf - space for stuffing a packet's worth of data.
 *    size - the size of the packet.
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *
 * Module Test Plan:
 *
 * Revision History:
 *    Ken Stickney         04-May-1994   Original. 
 *    Ken Stickney         24-Aug-1994   Added STATUS_INVALID_CAPID to 
 *                                       if statement when checking for
 *                                       return from acs_ipc_read(). 
 *    Ken Stickney         17-Dec-1994   Removed acslm.h. Replaced with
 *                                       acscvt.h. For AS400 portablility.
 *                                       acscvt.h supplies packet
 *                                       defintions independent of other
 *                                       system/platform data
 *    Van Lepthien         27-Aug-2001   Pass status of STATUS_PENDING from
 *                                       acs_ipc_read back to caller.
 */

/* File was formerly named acs103.c */

#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"
#include "acscvt.h"

#undef SELF
#define SELF "acs_get_response"
#undef ACSMOD
#define ACSMOD 103

STATUS (*cvtRespFuncs[]) ( ALIGNED_BYTES , size_t * ) =
{
    acs_cvt_v2_v3,
    acs_cvt_v3_v4
};


STATUS acs_get_response
  (
  ALIGNED_BYTES rbuf,
  size_t *            size
  )

{

  RESPONSE_TYPE     *ssi_res;        /* set up ssi response pointer */
  MESSAGE_HEADER    *msg_hdr_ptr;  /* pointer to generic msg header */
  unsigned int       i;
  VERSION server_version;
  VERSION highest_packet_version;
  VERSION lowest_packet_version;

  STATUS status;
  STATUS acsReturn = STATUS_SUCCESS;
  ACSMESSAGES msg_num;

  acs_trace_entry();

    highest_packet_version = (VERSION) ((int) VERSION_LAST - 1);
    lowest_packet_version = (VERSION) ((int) VERSION_LAST -
                                               NUM_RECENT_VERSIONS);
 
    /* Read one response from the SSI */
    status = acs_ipc_read(rbuf, size);
#ifdef DEBUG
    printf("\n%s:  acs_ipc_read() return: %s\n", 
           SELF, acs_status(status));
#endif
    switch (status)
    {
        case STATUS_SUCCESS:
            break;
        case STATUS_PENDING:
            return (status);
        default:
            msg_num = ACSMSG_IPC_READ_FAILED;
            acs_error_msg((&msg_num, NULL));
            return  STATUS_IPC_FAILURE;
    }

  /* get the version of the response */
  ssi_res  = (RESPONSE_TYPE *)rbuf; 
  msg_hdr_ptr = &ssi_res->generic_response.request_header.message_header;
  server_version = msg_hdr_ptr->version;

  /* Check validity of version number for this toolkit */
  if ( highest_packet_version < server_version || 
       lowest_packet_version >  server_version ) {
      return STATUS_INVALID_VERSION;
  }

  /* Do not convert if request/response transaction has a bad status */
  acsReturn = ssi_res->generic_response.response_status.status;
  if ((acsReturn != STATUS_SUCCESS) && 
      (acsReturn != STATUS_DONE) && 
      (acsReturn != STATUS_RECOVERY_COMPLETE) && 
      (acsReturn != STATUS_NORMAL) && 
      (acsReturn != STATUS_INVALID_CAP) && 
      (acsReturn != STATUS_MULTI_ACS_AUDIT) && 
      (acsReturn != STATUS_VALID )){
      return acsReturn;
  }

  for ( i =  server_version; i < (int)VERSION_LAST - 1; i++) {
      acsReturn = cvtRespFuncs[i - (int)lowest_packet_version]( rbuf,  size );
      if (acsReturn != STATUS_SUCCESS){
          acs_trace_exit(acsReturn);
          return (acsReturn);
      }
  }

  acs_trace_exit(acsReturn);

  return acsReturn;
}
