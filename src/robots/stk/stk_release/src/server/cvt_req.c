#ifndef lint
static char SccsId[] = "@(#) %full_name:	1/csrc/cvt_req.c/2 %";
#endif

/*
 * Copyright (C) 1993,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *
 *      lm_cvt_req.c
 *
 * Description:
 *
 *      This routine is the front end of the request conversion logic 
 *      within the ACSLM.  It is called to convert any VERSION request packets
 *      to the most current VERSION of the request due to the fact that
 *      the ACSLM only deals internally with the most current VERSION 
 *      request packets.
 *
 *      o Validate that the VERSION of the request is within the scope of
 *        valid request VERSIONs.
 *      o Convert the packet (if needed) to the most current version
 *        request.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *      STATUS_INVALID_COMMAND       Message contains invalid command
 *      STATUS_INVALID_VERSION       Message has invalid Version number
 *      STATUS_PROCESS_FAILURE       General failure
 *
 * Implicit Inputs:
 *
 *      None
 *
 * Implicit Outputs:
 *
 *      None
 *
 * Considerations:
 *
 *      None.
 *
 * Module Test Plan:
 *
 *      None
 *
 * Revision History:
 *
 *      H. I. Grapek    02-Oct-1993     Original
 * {1}  Alec Sharp      27-Apr-1993     Added V2 to V3 conversion. Also
 *                      fixed bug in code checking the extended bit and the
 *                      version number. Determine which packet versions
 *                      are legal by comparing with a global variable.
 *      Alec Sharp      27-Jul-1993     Determine the minimum version
 *                      supported every time this function is called.
 *
 *	J. Borzuchowski 24-Aug-1993	R5.0 Mixed Media-- Added V3 to V4
 *			conversion by calling new routine lm_cvt_v3_v4().
 *      Mike Williams   Included stdlib.h to remedy warnings.
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <stdio.h>
#include <stdlib.h>

#include "cl_pub.h"
#include "ml_pub.h"
#include "acslm.h"
#include "dv_pub.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char             module_string[] = "lm_cvt_req";

/*
 * Procedure declarations
 */
STATUS 
lm_cvt_req (
    REQUEST_TYPE *reqp,          /* pointer to incoming request packet */
    int *byte_count    /* byte count size of request packet */
)
{
    MESSAGE_HEADER  *msg_hdr_ptr;   /* pointer to generic message header  */
    STATUS          status;         /* return value from convert routines */
    long            min_version;    /* Minimum version number accepted    */
    
#ifdef DEBUG
    if TRACE (0)
        cl_trace(module_string, 2, (ulong)reqp, (ulong)byte_count);
#endif

    /* Check the sanity of incomming request */
    CL_ASSERT(module_string, (reqp));

    /* set up short hand pointer */
    msg_hdr_ptr = &(reqp->generic_request.message_header);

    MLOGDEBUG(0,(MMSG(1012, "%s: here... cmd(%s), vers(%d), options (%x), bytes(%d)"), 
	module_string, cl_command(msg_hdr_ptr->command), msg_hdr_ptr->version,
        msg_hdr_ptr->message_options, *byte_count));

    /* 
     * Validate packet version:  By definition, the version field is present
     * only in packets with EXTENDED present in message_options.  If the 
     * version is invalid, log a message and discard the packet.
     *
     * Note: VERSION0 packets with EXTENDED present in message_options can't 
     * be distinguished from VERSION1 or greater packets and will be treated
     * as though they had an extended message header.  This could produce
     * unpredictable results in both conversion and validation.
     */

    /* {1} Reject packets unless they are allowed */

    /* Determine the minimum version number accepted */
    if(((char *)getenv("ACSAPI_MIN_VERSION") == NULL) || 
       ((min_version = atoi(getenv("ACSAPI_MIN_VERSION"))) < (long)VERSION0) ||
       (min_version  > (long)VERSION_LAST))
	 min_version = (long) VERSION_MINIMUM_SUPPORTED;


    if (msg_hdr_ptr->message_options & EXTENDED) {
	/* These are packets with a version number greater than VERSION0 */
	if ((long)msg_hdr_ptr->version < min_version ||
	    (long)msg_hdr_ptr->version >= (long)VERSION_LAST) {
	    MLOG((MMSG(152,"%s: Unsupported version %d packet detected: discarded"), module_string, 
				    (int)msg_hdr_ptr->version));
	    return (STATUS_INVALID_VERSION);
	}
    }
    else {
	
	/* These are Version 0 packets */
	if (min_version > (long)VERSION0) {
	    MLOG((MMSG(152,"%s: Unsupported version %d packet detected: discarded"), module_string, 
				    (int)VERSION0));
	    return (STATUS_INVALID_VERSION);
	}
    }
	

    /* 
     * Convert VERSION0 requests: by definition, VERSION0 packets don't have
     * EXTENDED present in message_options.  These packets are converted to 
     * the VERSION1 format with VERSION0 set in version and EXTENDED missing
     * in message_options.  VERSION0 now becomes a valid packet version for `
     * subsequent version tests.
     * 
     * Note: VERSION1 or greater packets without EXTENDED in message_options
     * can't be distinguished from VERSION0 packets and will be converted as 
     * though they were VERSION0 packets.  This could produce unpredictable 
     * results in both conversion and validation.
     */
    
    if (!(msg_hdr_ptr->message_options & EXTENDED) &&
        ((status = lm_cvt_v0_v1((char *)reqp, byte_count)) != STATUS_SUCCESS)) {

        /* unsuccessful conversion */
	MLOGU((module_string, "lm_cvt_v0_v1", status,
	     MNOMSG));
        return (status);
    }

    /* Note that the code between the lint comments goes through lint 
     * without checking error 568-- unsigned is never less than zero.
     */

    /*lint -e568 */

    /* 
     * Convert VERSION0 through VERSION1 requests to VERSION2 format.
     */

    if (((int)msg_hdr_ptr->version >= (int)VERSION0) &&
        ((int)msg_hdr_ptr->version <= (int)VERSION1) &&
        ((status = lm_cvt_v1_v2((char *)reqp, byte_count)) != STATUS_SUCCESS)) {

        /* unsuccessful conversion */
	MLOGU((module_string, "lm_cvt_v1_v2", status,
	     MNOMSG));
        return (status);
    }

    /* 
     * Convert VERSION0 through VERSION2 requests to VERSION3 format. {1}
     */
    if (((int)msg_hdr_ptr->version >= (int)VERSION0) &&
        ((int)msg_hdr_ptr->version <= (int)VERSION2) &&
        ((status = lm_cvt_v2_v3((V2_REQUEST_TYPE *)reqp, byte_count)) != STATUS_SUCCESS)) {

        /* unsuccessful conversion */
	MLOGU((module_string, "lm_cvt_v2_v3", status,
	     MNOMSG));
        return (status);
    }

    /* 
     * Convert VERSION0 through VERSION3 requests to VERSION4 format.
     */
    if (((int)msg_hdr_ptr->version >= (int)VERSION0) &&
        ((int)msg_hdr_ptr->version <= (int)VERSION3) &&
        ((status = lm_cvt_v3_v4((V3_REQUEST_TYPE*)reqp, byte_count)) != 
	  STATUS_SUCCESS)) {

        /* unsuccessful conversion */
	MLOGU((module_string, "lm_cvt_v3_v4", status,
	     MNOMSG));
        return (status);
    }
    /*lint +e568 */
    
    return (STATUS_SUCCESS);
}
