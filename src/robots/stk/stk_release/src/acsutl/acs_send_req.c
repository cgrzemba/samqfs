#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:	1/csrc/acs_send_req.c/2.1.2 %";
#endif
/*
 *
 *                             Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_send_request()
 *
 * Description:
 *      Translates a request into a down-level form if the server that
 *      the client is connected to is a down-level server.
 *      For example, if the Toolkit version supports version 4 packets,
 *      and the server supports at a maximum version 3 packets, then
 *      this function will translate the version 4 request into a 
 *      version 3 request and call acs_ipc_write, which will send the
 *      request via IPC to the SSI and hence to the server.
 *      
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE.
 *
 * Parameters:
 *
 *    rbuf - a pointer to a packet's worth of data.
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
 *    Ken Stickney         05-May-1994    Original.
 *      Ken Stickney        17-Dec-1994  Removed acslm.h. Replaced with
 *                                       acscvt.h. For AS400 portablility.
 *                                       acscvt.h supplies packet
 *                                       defintions independent of other
 *                                       system/platform data

 */


#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"
#include "acscvt.h"

#undef SELF
#define SELF "acs_send_request"
#undef ACSMOD
#define ACSMOD 104

STATUS (*cvtReqFuncs[]) ( ALIGNED_BYTES , size_t * ) =
{
    acs_cvt_v4_v3,
    acs_cvt_v3_v2
};

STATUS acs_send_request
  (
  ALIGNED_BYTES rbuf,
  size_t        size
  )

{
  char msg[500];

  REQUEST_TYPE      *ssi_req;                /* set up ssi req pointer */
  unsigned short i;
  unsigned short rel_index;
  VERSION highest_version;

  size_t byte_count;
  ACSMESSAGES msg_num;

  STATUS acsReturn = STATUS_SUCCESS;

  acs_trace_entry();

  /* we only support two versions back ( to VERSION_2 ) */
  if ( (int ) acs_get_packet_version() < 
       (int)  (VERSION_LAST - NUM_RECENT_VERSIONS) ) {
      
      return (STATUS_INVALID_VERSION);
  }

  byte_count = size;

  ssi_req  = (REQUEST_TYPE *)rbuf; /*loaded by the acsapi routine */

  /* convert the request down to the highest supported by the server */
  highest_version = VERSION_LAST - (VERSION)1;
  for(i = (unsigned short)highest_version; 
	   i > (unsigned short)acs_get_packet_version(); i--) {
      rel_index = (unsigned short)highest_version - i;
      acsReturn = cvtReqFuncs[rel_index](ssi_req, &byte_count);
      if (acsReturn != STATUS_SUCCESS) {
          acs_trace_exit(acsReturn);
          return (acsReturn);
      }
  }    

  /* send out the converted request packet */
  if(acs_ipc_write(rbuf,byte_count) != STATUS_SUCCESS){
#ifdef DEBUG
      sprintf(msg, "%s:  acs_ipc_write() failed\n", SELF);
#endif
      msg_num = ACSMSG_IPC_WRITE_FAILED;
      acs_error_msg((&msg_num, NULL));
      acsReturn =  STATUS_IPC_FAILURE;
  }

  acs_trace_exit(acsReturn);

  return acsReturn;

  }
