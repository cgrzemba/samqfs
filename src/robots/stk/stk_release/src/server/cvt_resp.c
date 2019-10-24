#ifndef lint
static char SccsId[] = "@(#) %full_name:	server/csrc/cvt_resp/2.0A %";
#endif
/*
 * Copyright (1991, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      lm_cvt_resp.c
 *
 * Description:
 *
 *      This routine is the front end of the response conversion logic 
 *      within the ACSLM.  It is called to convert the most current
 *      VERSION response packet to its originating VERSION. 
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *      STATUS_PROCESS_FAILURE     Error in one of the conversion routines
 *
 * Implicit Inputs:
 *
 *      None.
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
 *                      29-Oct-1993     Added debug output, 
 *                                      fixed error message string.
 * {2}  Alec Sharp      27-Apr-1993     Added conversion from V3 packets down
 *                                      to V2.
 * {3}  J. Borzuchowski 24-Aug-1993	R5.0 Mixed Media-- Added V4 to V3
 *			conversion by calling new routine lm_cvt_v4_v3().
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"

#include <stdio.h>
#include "cl_pub.h"
#include "ml_pub.h"
#include "acslm.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char             module_string[] = "lm_cvt_resp";

/*
 * Procedure declarations
 */
STATUS 
lm_cvt_resp (
    RESPONSE_TYPE *rssp,          /* pointer to response packet */
    int *byte_count    /* byte count size of response packet */
)
{
    MESSAGE_HEADER      *resp_msg_hdr;  /* pointer to generic message header */
    STATUS              status;         /* return value from convert routines */

#ifdef DEBUG
    if TRACE (0)
        cl_trace(module_string, 2, (ulong)rssp, (ulong)byte_count);
#endif

    /* Check the sanity of incomming request */
    CL_ASSERT(module_string, (rssp));

    /* set up generic response's message header pointer */
    resp_msg_hdr = &(rssp->generic_response.request_header.message_header);

    MLOGDEBUG(0,(MMSG(862, "%s: here... cmd(%s), vers(%d), bytes(%d)"), module_string, 
        cl_command(resp_msg_hdr->command), resp_msg_hdr->version, *byte_count));

    /* 
     * Convert current response version to original packet version in the
     * request. The response is converted in steps from the current 
     * version to the current version minus one, and so on, until the 
     * the version matches the version in the request packet.  The original
     * packet version was copied into the message response header.  This is
     * is the version compared to determine the number of conversion steps.
     */

    /* Note that the code between the lint comments goes through lint 
     * without checking error 568-- unsigned is never less than zero.
     */

    /*lint -e568 */

    /*  Convert down to VERSION3 if necessary {3} */
    if (((int)resp_msg_hdr->version >= (int)VERSION0) &&
        ((int)resp_msg_hdr->version <= (int)VERSION3) &&
        ((status = lm_cvt_v4_v3((RESPONSE_TYPE *)rssp, byte_count)) != 
	 STATUS_SUCCESS)) {

        /* unsuccessful conversion */
        MLOGU((module_string, "lm_cvt_v4_v3", status,
             MNOMSG));
        return (STATUS_PROCESS_FAILURE);
    }

    /*  Convert down to VERSION2 if necessary {2} */
    if (((int)resp_msg_hdr->version >= (int)VERSION0) &&
        ((int)resp_msg_hdr->version <= (int)VERSION2) &&
        ((status = lm_cvt_v3_v2((V2_RESPONSE_TYPE *)rssp, byte_count)) != STATUS_SUCCESS)) {

        /* unsuccessful conversion */
        MLOGU((module_string, "lm_cvt_v3_v2", status,
             MNOMSG));
        return (STATUS_PROCESS_FAILURE);
    }

    /*  Convert down to VERSION1 if necessary */
    if (((int)resp_msg_hdr->version >= (int)VERSION0) &&
        ((int)resp_msg_hdr->version <= (int)VERSION1) &&
        ((status = lm_cvt_v2_v1((char *)rssp, byte_count)) != STATUS_SUCCESS)) {

        /* unsuccessful conversion */
        MLOGU((module_string, "lm_cvt_v2_v1", status,
             MNOMSG));
        return (STATUS_PROCESS_FAILURE);
    }

    /*lint +e568 */

    /* Convert down to VERSION0 if necesssary */
    if (((int)resp_msg_hdr->version == (int)VERSION0) &&
        ((status = lm_cvt_v1_v0((char *)rssp, byte_count)) != STATUS_SUCCESS)) {

        /* unsuccessful conversion */
        MLOGU((module_string, "lm_cvt_v1_v0", status,
             MNOMSG));
        return (STATUS_PROCESS_FAILURE);
    }

    return (STATUS_SUCCESS);
}
