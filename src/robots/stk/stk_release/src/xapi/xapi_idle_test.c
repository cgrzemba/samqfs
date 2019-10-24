/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_idle_test.c                                 */
/** Description:    XAPI client IDLE state test service.             */
/**                                                                  */
/**                 Test if the STATUS_IDLE error response           */
/**                 should be generated.                             */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     08/01/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/***END PROLOGUE******************************************************/

/*********************************************************************/
/* Includes:                                                         */
/*********************************************************************/
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "api/defs_api.h"
#include "csi.h"
#include "smccxmlx.h"
#include "srvcommon.h"
#include "xapi.h"


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_idle_test                                    */
/** Description:   Test if the XAPI client is in IDLE state.         */
/**                                                                  */
/** The XAPI client has 3 possible status; XAPI_ACTIVE,              */
/** XAPI_IDLE, and XAPI_IDLE_PENDING.  Only CANCEL, IDLE, QUERY,     */
/** and START commands are allowed when the status is XAPI_IDLE,     */
/** or IDLE_PENDING (VARY is also documented as being allowed, but   */
/** VARY commands are unsupported by the XAPI).                      */
/**                                                                  */
/** This routine tests if the STATUS_IDLE error response should      */
/** be generated when the XAPI client is in the IDLE or              */
/** IDLE_PENDING state.  If the XAPICVT.status field is              */
/** XAPI_IDLE or XAPI_IDLE_PENDING, then return the STATUS_IDLE      */
/** return code.  Otherwise, return STATUS_SUCCESS.                  */
/**                                                                  */
/** NOTE: This routine should not be called for those ASCAPI         */
/** requests that are allowed to proceed when the system is IDLE.    */
/** ACSAPI requests that are allowed to proceed when IDLE are        */
/** CANCEL, IDLE, QUERY, QUERY_LOCK, START, and VARY.                */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_idle_test"

extern int xapi_idle_test(struct XAPICVT  *pXapicvt,
                          struct XAPIREQE *pXapireqe,
                          char            *pResponseBuffer,
                          int              responseBufferSize)
{
    REQUEST_TYPE       *pRequest_Type;
    REQUEST_HEADER     *pRequest_Header;
    MESSAGE_HEADER     *pMessage_Header;

    if (pXapicvt->status == XAPI_ACTIVE)
    {
        return STATUS_SUCCESS;
    }

    if (pXapireqe != NULL)
    {
        pRequest_Type = (REQUEST_TYPE*) pXapireqe->pAcsapiBuffer;
        pRequest_Header = &(pRequest_Type->generic_request);
        pMessage_Header = &(pRequest_Header->message_header);

        if ((pMessage_Header->command == COMMAND_CANCEL) ||
            (pMessage_Header->command == COMMAND_IDLE) ||
            (pMessage_Header->command == COMMAND_QUERY) ||
            (pMessage_Header->command == COMMAND_QUERY_LOCK) ||
            (pMessage_Header->command == COMMAND_START) ||
            (pMessage_Header->command == COMMAND_VARY))
        {
            return STATUS_SUCCESS;
        }
    }

    xapi_err_response(pXapireqe,
                      pResponseBuffer,
                      responseBufferSize,
                      STATUS_IDLE);

    return STATUS_IDLE;
}



