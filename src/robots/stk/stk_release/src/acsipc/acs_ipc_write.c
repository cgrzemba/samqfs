#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsipc/csrc/acs_ipc_write/2.01A %";
#endif
/*
 *
 *                             Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_ipc_write()
 *
 * Description:
 *      Writes a request packet to an SSI via IPC.
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
 *    This is a machine dependent module. The function signature will
 *    not vary across platforms, but the code contained herein will.
 *
 * Module Test Plan:
 *
 * Revision History:
 *    Ken Stickney         04-Jun-1993    Ansi version from HSC (Tom Rethard).
 *    Ken Stickney         06-Nov-1993    Added parameter section, shortened
 *                                        code lines to 72 chars or less,
 *                                        fixed SCCSID for AS400 compiler,
 *                                        added defines ACSMOD and SELF for
 *                                        trace and error messages, fixed
 *                                        copyright. Changes for new error
 *                                        messaging.
 */


#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"
#include "cl_ipc_pub.h"

#undef SELF
#define SELF "acs_ipc_write"
#undef ACSMOD
#define ACSMOD 104

STATUS acs_ipc_write
  (
  ALIGNED_BYTES rbuf,
  size_t        size
  )

{
  char msg[500];
  ACSMESSAGES msg_num;

  STATUS acsReturn = STATUS_SUCCESS;

  acs_trace_entry();

    /* Send the request to the SSI */
  if(cl_ipc_write(acs_get_sockname(),rbuf,size) != STATUS_SUCCESS){
#ifdef DEBUG
      sprintf(msg, "%s:  cl_ipc_write() failed\n", SELF);
#endif
      msg_num = ACSMSG_IPC_WRITE_FAILED;
      acs_error_msg((&msg_num, NULL));
      acsReturn =  STATUS_IPC_FAILURE;
  }

  acs_trace_exit(acsReturn);

  return acsReturn;

  }

/*
int main(void)
  {
  char testString1[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  char testString2[]= "01234567890";

  acs_ipc_write(testString1,sizeof(testString1));
  acs_ipc_write(testString2,sizeof(testString2));

  return 0;
  }
*/

