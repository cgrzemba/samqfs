# ifndef lint
static char SccsId[] = "@(#) %full_name:  acsapi/csrc/acs_chk_reg/2.2 %";
# endif
/*
 *
 *                            (c) Copyright (C) 2001)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_check_registration()
 *
 * Description:
 *      This procedure is called by an Application Program to initiate a
 *      check registration request to the ACSSS software.  A check registration 
 *      request packet is constructed (using the parameters given) and sent
 *      to the SSI process.
 *
 * Return Values:
 *     If no errors, STATUS_SUCCESS; otherwise STATUS_IPC_FAILURE, or
 *                   STATUS_INVALID_VALUE.
 *
 * Parameters:
 *
 *  seqNumber       - A client defined number returned in the response.
 *  registration_id - Id of the registrant.
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
 *    Scott Siao           19-Apr-2002    Change acs_send_request size from REG_REQ to
 *                                        CHECK_REGISTRATION_REQUEST
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
# define SELF "acs_check_registration"
# undef ACSMOD
# define ACSMOD 50

STATUS acs_check_registration
(
 SEQ_NO seqNumber, 
 REGISTRATION_ID registration_id
)
{	
    COPYRIGHT;
    STATUS	    acsReturn;
    ACSMESSAGES     msg_num;
    CHECK_REGISTRATION_REQUEST check_registrationRequest;

    acs_trace_entry();
    acsReturn = acs_verify_ssi_running();
    if (acsReturn == STATUS_SUCCESS) {
	acsReturn = acs_build_header((char *)& 
				     check_registrationRequest, 
				     sizeof(CHECK_REGISTRATION_REQUEST), 
				     seqNumber, 
				     COMMAND_CHECK_REGISTRATION, 
				     EXTENDED, 
				     VERSION_LAST - (VERSION)1, 
				     NO_LOCK_ID);
	if (acsReturn == STATUS_SUCCESS) {
	    check_registrationRequest.registration_id = registration_id;
	    acsReturn = acs_send_request(&check_registrationRequest, 
					 sizeof(CHECK_REGISTRATION_REQUEST));
	}
    }
    acs_trace_exit(acsReturn);
    return acsReturn;
}

