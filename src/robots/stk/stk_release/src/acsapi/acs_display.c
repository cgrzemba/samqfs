# ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_display/2.2 %";
# endif
/*
 *
 *                       (c) Copyright (C) 2001-2002)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_display()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      display request to the ACSSS software.  A display request packet
 *      is constructed (using the parameters given) and sent to the SSI
 *      process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE.
 *
 * Parameters:
 *
 *  seqNumber       - A client defined number returned in the response.
 *  display_xml_data        - The display_xml_data stream for the request.
 *  
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
 *    Scott Siao           12-Nov-2001    Original.
 *    Scott Siao           19-Apr-2002    Changed ACSMOD from 48 to 51, added comments
 *                                        for display_type
 *
 *
 */

# include <stddef.h>
# include <stdio.h>

# include "acssys.h"
# include "acsapi.h"
# include "acssys_pvt.h"
# include "acsapi_pvt.h"

# undef SELF
# define SELF "acs_display"
# undef ACSMOD
# define ACSMOD 51

STATUS acs_display
(
 SEQ_NO seqNumber, 
 TYPE   display_type,                /* these types are part of TYPE but only */
				     /* a few of them are valid. */
 DISPLAY_XML_DATA display_xml_data
)
{	
    COPYRIGHT;
    STATUS	    acsReturn;
    ACSMESSAGES     msg_num;
    DISPLAY_REQUEST displayRequest;
    int size;

    acs_trace_entry();
    acsReturn = acs_verify_ssi_running();
    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header((char *)& displayRequest, 
				     sizeof(DISPLAY_REQUEST), 
				     seqNumber, 
				     COMMAND_DISPLAY, 
				     EXTENDED, 
				     VERSION_LAST - (VERSION)1, 
				     NO_LOCK_ID);
	if (acsReturn == STATUS_SUCCESS) {
	    displayRequest.display_type = display_type;
	    displayRequest.display_xml_data = display_xml_data;
	    acsReturn = acs_send_request(&displayRequest, 
					 (sizeof(IPC_HEADER) +4
					 +sizeof(MESSAGE_HEADER)
					 +sizeof(TYPE)
					 +sizeof(unsigned short)
					 +display_xml_data.length));
	}
    }
    acs_trace_exit(acsReturn);
    return acsReturn;
}

