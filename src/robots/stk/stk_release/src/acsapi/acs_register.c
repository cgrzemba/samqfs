# ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_register/2.2 %";
# endif
/*
 *
 *                            (c) Copyright (C) 2002)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_register()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      register request to the ACSSS software.  A register request packet
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
 *  registration_id - Id of the registrant.
 *  EventClass      - Pointer to an array of event classes the calling program is
 *                    registering for.
 *  count           - The number of event classes.
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
 *    Scott Siao           08-Oct-2001    Original.
 *    Scott Siao           19-Apr-2002    Removed debugging code.
 *    Scott Siao           29-Apr-2002    Changed MAX_ID to MAX_REGISTER_STATUS
 *    Scott Siao           20-May-2002    Changed MAX_REGISTER_STATUS to
 *                                        MAX_EVENT_CLASS_TYPE.
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
# define SELF "acs_register"
# undef ACSMOD
# define ACSMOD 48

STATUS acs_register
(
 SEQ_NO seqNumber, 
 REGISTRATION_ID registration_id, 
 EVENT_CLASS_TYPE eventClass[MAX_EVENT_CLASS_TYPE], 
 unsigned short count
)
{	
    COPYRIGHT;
    STATUS	    acsReturn;
    ACSMESSAGES     msg_num;
    EVENT_CLASS_TYPE *eventClassPointer;
    REGISTER_REQUEST registerRequest;
    unsigned int    i;

    acs_trace_entry();
    if (count > MAX_EVENT_CLASS_TYPE) {
	msg_num = ACSMSG_BAD_COUNT;
	acs_error_msg((&msg_num, count, MAX_EVENT_CLASS_TYPE));
	acsReturn = STATUS_INVALID_VALUE;
	acs_trace_exit(acsReturn);
	return acsReturn;
    }
    acsReturn = acs_verify_ssi_running();
    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header((char *)& registerRequest, 
				     sizeof(REGISTER_REQUEST), 
				     seqNumber, 
				     COMMAND_REGISTER, 
				     EXTENDED, 
				     VERSION_LAST - (VERSION)1, 
				     NO_LOCK_ID);
	if (acsReturn == STATUS_SUCCESS) {
	    registerRequest.registration_id = registration_id;
	    registerRequest.count = count;
	    eventClassPointer = &eventClass[0];
	    for (i = 0; i < registerRequest.count; i++, 
		     eventClassPointer++) {
		registerRequest.eventClass[i] = *eventClassPointer;
	    }
	    acsReturn = acs_send_request(&registerRequest, sizeof(
			REGISTER_REQUEST));
	}
    }
    acs_trace_exit(acsReturn);
    return acsReturn;
}

