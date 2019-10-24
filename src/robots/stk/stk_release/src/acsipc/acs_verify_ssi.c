#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsipc/csrc/acs_verify_ssi/2.01A %";
#endif
/*
 *
 *                             Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_verify_ssi_running()
 *
 * Description:
 *      Common routine for setting up the process IPC environment. 
 *      Accomplishes this by using the common_lib routine cl_ipc_create().
 *      
 *
 * Return Values:
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *       This is a machine dependent module. Its signature remains the 
 *       same over platform, but the code contained herein will vary.
 *
 * Module Test Plan:
 *
 * Revision History:
 *    Ken Stickney         04_Jun_1993    Ansi version from HSC (Tom Rethard).
 *    Ken Stickney         06-Nov-1993    Added parameter section, shortened
 *                                        code lines to 72 chars or less,
 *                                        fixed SCCSID for AS400 compiler,
 *                                        added defines ACSMOD and SELF for
 *                                        trace and error messages, fixed
 *                                        copyright. Changes for new error
 *                                        messaging.
 *    Ken Stickney         07-Feb-1993    Added call to cl_el_register to 
 *                                        register the standard message 
 *                                        logging function.
 */

/* File was formerly named acs100.c */
#include "acssys.h"

#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"

#include "cl_pub.h"
#include "cl_ipc_pub.h"

#undef SELF
#define SELF "acs_verify_ssi_running"
#undef ACSMOD
#define ACSMOD 100


STATUS acs_verify_ssi_running ( void )
{

   STATUS acsReturn = STATUS_SUCCESS;

   static BOOLEAN verified = FALSE;
   ACSMESSAGES  msg_num;

   acs_trace_entry();

   if(!verified){

      my_module_type = TYPE_NONE;

      /* register the standard product logging functions */
      cl_el_log_register(); 

      if ((acsReturn = cl_ipc_create(ANY_PORT)) != STATUS_SUCCESS) {
#ifdef DEBUG
          printf("%s:  cl_ipc_create() failed\n", SELF);
#endif
          msg_num = ACSMSG_IPC_CREATE_FAILED;
          acs_error_msg((&msg_num, NULL));
      }
      verified = TRUE;
   }  

   acs_trace_exit(acsReturn);

   return acsReturn;
}
