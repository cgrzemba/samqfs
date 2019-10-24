#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:  acsutl/csrc/acs_build_hdr/2.1.1 %";
#endif
/*
*
*                            (c) Copyright (1993-1994)
*                      Storage Technology Corporation
*                            All Rights Reserved
*
* Name:
*
*      acs_build_header()
*
*
* Description:
*
*      This procedure is a utility called by every Toolkit procedure
*      that makes a request to the ACSSS software.  The message header
*      of an empty REQUEST_HEADER is filled in with the parameters input
*      by the calling Toolkit procedure.
*
*
* Return Values:
*     STATUS_SUCCESS.
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
*    Ken Stickney         04-Jun-1993    ANSI version from RMLS/400
*                                        (Tom Rethard).
*    Ken Stickney         06-Nov-1993    Added parameter section, shortened
*                                        code lines to 72 chars or less,
*                                        fixed SCCSID for AS400 compiler,
*                                        added defines ACSMOD and SELF for
*                                        trace and error messages, fixed
*                                        copyright. Changes for new error
*                                        messaging.
*
*    Mitch Black          01-Aug-1994    Added setting of packet access id
*                                        in support of access control.
*/

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "acsapi_pvt.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

#undef SELF
#define SELF "acs_build_header"
#undef ACSMOD
#define ACSMOD 101

/*
 *      Global and Static Variable Declarations:
 */

    /* Global access ID for setting from the API */
    /* Note that, because this is a global, it is initialized by the */
    /* compiler to all NULLs (necessary if this is never set by API call) */
    ACCESSID global_aid;

/*
 *      Procedure Type Declarations:
 */

STATUS acs_build_header
  (
    char *bp,
    size_t packetSize,
    SEQ_NO seqNumber,
    COMMAND requestCommand,
    unsigned char requestOptions,
    VERSION packetVersion,
    LOCKID requestLock
) {

    COPYRIGHT;

    STATUS acsResponse;

    REQUEST_HEADER *reqHeader;

    acs_trace_entry ();

    reqHeader = (REQUEST_HEADER *) bp;

    /* clear buffer to 0's */

    memset ((char *)reqHeader, '\0', packetSize - 1);

    reqHeader->message_header.packet_id = seqNumber;
    reqHeader->message_header.command = requestCommand;
    reqHeader->message_header.message_options = requestOptions;
    reqHeader->message_header.version = packetVersion;
    reqHeader->message_header.lock_id = requestLock;

    /* Set packet access ID from the global which the client sets */
    reqHeader->message_header.access_id = global_aid;

    acsResponse = STATUS_SUCCESS;

    acs_trace_exit (acsResponse);

    return acsResponse;
}

