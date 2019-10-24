#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsipc/csrc/acs_error/2.1 %";
#endif
/*
 *
 *                             Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_error()
 *
 * Description:
 *      Generic routine for acsapi error messages.
 *      Takes as input arguments the name of the calling function,
 *      the index of the message to be sent, and a variable argument
 *      list. All the messages that this function can send are defined
 *      locally, in this module. A message can contain printf formatting
 *      information, which maps to the arguments in the variable argu-
 *      ment list. This function uses the varargs macros to process
 *      the variable argument list.
 *
 * Return Values:
 *
 * Parameters:
 *   MsgNo - an ACSMESSAGES enum specifying an entry in the message
 *            table found below.
 *   ... - a variable arguement list, used to supply values to the
 *              message strings.
 *
 * Implicit Inputs:
 *
 *   The globals below are set by the macro acs_error_msg found in 
 *   acssys_pvt.h
 *   acs_caller - an integer that corresponds to an entry in the function
 *            name lookup table below. Specifies the calling function.
 *
 * Implicit Outputs:
 *
 *   NONE
 *
 * Considerations:
 *     This module is machine dependent. The function signature will
 *     remain constant over platforms, but the code contained herein
 *     will vary. This function must be updated when new interface
 *     functions are created.
 *
 * Module Test Plan:
 *
 * Revision History:
 *    Ken Stickney       04-Jun-1993    Ansi version from HSC (Tom Rethard).
 *    Ken Stickney       28-Jun-1993    Recode to use variable argument
 *                                      processing.
 *    Ken Stickney       09-Aug-1993    Cleaned up for code review.
 *    Ken Stickney       06-Nov-1993    Added parameter section, shortened
 *                                      code lines to 72 chars or less,
 *                                      fixed SCCSID for AS400 compiler,
 *                                      added defines ACSMOD and SELF for
 *                                      trace and error messages, fixed
 *                                      copyright. Routine completely 
 *                                      rewritten and renamed, for 
 *                                      portability
 *    Ken Stickney       25-May-1994    Added entry in table for error
 *                                      message comming out of acs_get_
 *                                      packet_version(). BR#26.
 *    Ken Stickney       31-Aug_1994    Changed from cl_log_event to MLOG
 *                                      Fix for BR#37.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <stdarg.h>

#ifndef ACS_ERROR_C
#define ACS_ERROR_C
#endif

#include "acsapi.h"
#include "ml_pub.h"
#include "acssys_pvt.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

#undef SELF
#define SELF "acs_error"

/*
 *      Global and Static Variable Declarations:
 */

int acs_caller; 

static char lbuf[512];

  static struct fnames {
      int fname_id;
      char *fname_string;
  }
fname_table[] = {

    0,
    "acs_audit_acs",

    1,
    "acs_audit_lsm",

    2,
    "acs_audit_panel",

    3,
    "acs_audit_subpanel",

    4,
    "acs_cancel",

    5,
    "acs_clear_lock_drive",

    6,
    "acs_clear_lock_volume",

    7,
    "acs_define_pool",

    8,
    "acs_delete_pool",

    9,
    "acs_dismount",

    10,
    "acs_eject",

    11,
    "acs_enter",

    12,
    "acs_idle",

    13,
    "acs_lock_drive",

    14,
    "acs_lock_volume",

    15,
    "acs_mount",

    16,
    "acs_mount_scratch",

    17,
    "acs_query_acs",

    18,
    "acs_query_lsm",

    19,
    "acs_query_clean",

    20,
    "acs_query_drive",

    21,
    "acs_query_lock_drive",

    22,
    "acs_query_lock_volume",

    23,
    "acs_query_cap",

    24,
    "acs_query_mount",

    25,
    "acs_query_mount_scratch",

    26,
    "acs_query_pool",

    27,
    "acs_query_port",

    28,
    "acs_query_port",

    29,
    "acs_query_scratch",

    30,
    "acs_query_server",

    31,
    "acs_query_volume",

    32,
    "acs_response",

    33,
    "acs_set_cap",

    33,
    "acs_set_clean",

    34,
    "acs_set_scratch",

    35,
    "acs_start",

    36,
    "acs_unlock_drive",

    37,
    "acs_unlock_volume",

    38,
    "acs_unlock_volume",

    39,
    "acs_vary_acs",

    40,
    "acs_vary_drive",

    41,
    "acs_vary_lsm",

    42,
    "acs_vary_port",

    43,
    "acs_venter",

    44,
    "acs_xeject",

    45,
    "acs_query_mm_info",

    46,
    "acs_vary_cap",

    47,
    "acs_audit_server",

    100,
    "acs_verify_ssi_running",

    101,
    "acs_build_header",

    102,
    "acs_error",

    103,
    "acs_ipc_read",

    104,
    "acs_ipc_write",

    105,
    "acs_build_ipc_header",

    107, 
    "acs_get_packet_version",

    108,
    "acs_type",

    200,
    "acs_vary_response",

    201,
    "acs_query_response",

    202,
    "acs_audit_fin_response",

    203,
    "acs_audit_int_response",

    205,
    "acs_select_input",

    999,
    "acs_cvt_v1_v2",

};

  static struct messages {
      ACSMESSAGES msg_code;
      char *msg_string;
  }
msg_table[] = {

    /* 0 */
    ACSMSG_BAD_INPUT_SELECT,
    "FATAL_ERROR! cl_select_input() failed.\n",

    ACSMSG_IPC_READ_FAILED,
    "FATAL_ERROR! cl_ipc_read() failed.\n",

    ACSMSG_IPC_WRITE_FAILED,
    "FATAL_ERROR! cl_ipc_write() failed.\n",

    ACSMSG_IPC_CREATE_FAILED,
    "FATAL_ERROR! cl_ipc_create() failed.\n",

    ACSMSG_NOT_EXTENDED,
    "FATAL_ERROR! extended options not set.\n",

    /* 5 */
    ACSMSG_BAD_VERSION,
    "FATAL_ERROR! Bad packet version, #%d, cannot recover.\n",

    ACSMSG_INVALID_VERSION,
    "WARNING. invalid packet version - %d - specified.\n Using default version %d.\n",

    ACSMSG_BAD_COUNT,
    "FATAL_ERROR! Number of objects requested, %d, greater than %d.\n",

    ACSMSG_BAD_SOCKET,
    "FATAL_ERROR! Socket name supplied, %s, has alphabetic characters.\n",

    ACSMSG_BAD_FINAL_RESPONSE,
    "FATAL_ERROR! bad final response for query request.\n",

    /* 10 */
    ACSMSG_BAD_INT_RESPONSE,
    "FATAL_ERROR! bad intermediate response for query request.\n",

    ACSMSG_BAD_QUERY_RESPONSE,
    "FATAL_ERROR! acs_query_response() failed.\n",

    ACSMSG_BAD_RESPONSE,
    "FATAL_ERROR! Bad command value for response packet.\n",

    ACSMSG_BAD_AUDIT_RESP_TYPE,
    "FATAL_ERROR! Library entity type returned, %d, is invalid for audits.\n",

    ACSMSG_BAD_QUERY_RESP_TYPE,
    "FATAL_ERROR! Library entity type returned, %d, is invalid for queries.\n",

    /* 15 */
    ACSMSG_BAD_VARY_RESP_TYPE,
    "FATAL_ERROR! Library entity type returned, %d is invalid for varies.\n",

    ACSMSG_VARY_RESP_FAILED,
    "FATAL_ERROR! acs_vary_response() failed.\n",

    ACSMSG_BAD_RESPONSE_TYPE,
    "FATAL_ERROR! Invalid ACS_RESPONSE_TYPE %d as input argument.\n",

    ACSMSG_INVALID_TYPE,
    "FATAL_ERROR! Invalid TYPE found in packet\n",

};

void acs_error (ACSMESSAGES *MsgNo , ... )
{

    va_list ap;			/* argument pointer */
    int i, j;			/* argument counter */

    STATUS acsReturn = STATUS_SUCCESS;

    acs_trace_entry ();

    /* zero out the message buffer */
    memset((char *)lbuf, 0, sizeof(lbuf));

    /* determine function name */
    for (j = 0; j < (sizeof (fname_table) / sizeof (struct fnames)); j++)
	if (acs_caller == fname_table[j].fname_id)
	    break;

    /* did we find it? */
    if (j >= (sizeof (fname_table) / sizeof (struct fnames))) {
	sprintf (lbuf + strlen (lbuf), "Format undefined for message %d",
	    acs_caller);
    }
    else {
	/* format caller name */
	sprintf (lbuf, "%s: ", fname_table[j].fname_string);
    }

    /* determine message format */
    for (i = 0; i < (sizeof (msg_table) / sizeof (struct messages)); i++)
	if (*MsgNo == msg_table[i].msg_code)
	    break;

    /* did we find it? */
    if (i >= (sizeof (msg_table) / sizeof (struct messages))) {
	sprintf (lbuf + strlen (lbuf), "Format undefined for message %d",
	    MsgNo);
    }
    else {
	va_start (ap, MsgNo); 
	vsprintf (lbuf + strlen (lbuf), msg_table[i].msg_string, ap);
	va_end (ap);
    }

    MLOG((MMSG(510, "%s\n"),lbuf));

    acs_trace_exit (acsReturn);

    return;
}

