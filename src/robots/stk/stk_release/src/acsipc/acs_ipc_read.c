#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  1/csrc/acs_ipc_read.c/3 %";
#endif
/*
 *
 *                             Copyright (1993-2001)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_ipc_read()
 *
 * Description:
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; 
 *     If cl_ipc_read returns STATUS_PENDING, pass it up; 
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
 *    This module is machine dependent. The function signature will
 *    not vary across platforms, but the code contained herein will.

 * Module Test Plan:
 *
 * Revision History:
 *    Ken Stickney         04-Jun-1993    Ansi version from HSC (Tom Rethard).
 *    Ken Stickney         06-Nov-1993    Added parameter section, shortened
 *                                        code lines to 72 chars or less,
 *                                        fixed SCCSID for AS400 compiler,
 *                                        added defines ACSMOD and SELF for
 *                                        trace and error messages, fixed
 *                                        copyright.
 *    Ken Stickney         23-Dec-1994    Changes for Solaris port.
 *    Van Lepthien         22-Aug-2001    Pass STATUS_PENDING from cl_ipc_read
 *                                        back to the caller.
 */

/* File was formerly named acs103.c */

#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"
#include "cl_ipc_pub.h"

#undef SELF
#define SELF "acs_ipc_read"
#undef ACSMOD
#define ACSMOD 103


STATUS acs_ipc_read
  (
  ALIGNED_BYTES rbuf,
  size_t *            size
  )

{

  ACSMESSAGES msg_num;
  STATUS acsReturn;

  acs_trace_entry();

    /* Read one response from the SSI */
    acsReturn = cl_ipc_read(rbuf, (int *)size);
    switch (acsReturn)
    {
        case STATUS_SUCCESS:
        case STATUS_PENDING:
            break;
        default:
#ifdef DEBUG
            printf("%s:  cl_ipc_read() failed\n", SELF);
#endif
            msg_num = ACSMSG_IPC_READ_FAILED;
            acs_error_msg((&msg_num, NULL));
            return  STATUS_IPC_FAILURE;
    }


  acs_trace_exit(acsReturn);

  return acsReturn;
}

