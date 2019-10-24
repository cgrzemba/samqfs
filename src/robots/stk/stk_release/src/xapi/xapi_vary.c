/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_vary.c                                      */
/** Description:    XAPI VARY processor.                             */
/**                                                                  */
/**                 NOTE: VARY commands are not supported            */
/**                 by the XAPI.  However, the                       */
/**                 STATUS_UNSUPPORTED_COMMAND logic of              */
/**                 xapi_main was unsuited for VARY                  */
/**                 error responses so this stub was created         */ 
/**                 to return the approriate VARY                    */
/**                 error response.                                  */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     08/15/11                          */
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

#include "srvcommon.h"
#include "xapi.h"
#include "api/defs_api.h"
#include "csi.h"


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#define VARYERR_RESPONSE_SIZE (offsetof(VARY_RESPONSE, device_status)) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_vary                                         */
/** Description:   The XAPI VARY processor.                          */
/**                                                                  */
/** Build and output an ACSAPI VARY error response.                  */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_vary"

extern int xapi_vary(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    VARY_REQUEST       *pVary_Request       = (VARY_REQUEST*) pXapireqe->pAcsapiBuffer;
    char                wkVaryResponse[VARYERR_RESPONSE_SIZE];
    VARY_RESPONSE      *pVary_Response      = (VARY_RESPONSE*) wkVaryResponse;     

    TRMSGI(TRCI_XAPI,
           "Entered; VARY request=%08X, size=%d, type=%d, "
           "VARY response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pVary_Request->type,
           pVary_Response,
           VARYERR_RESPONSE_SIZE);

    memset((char*) pVary_Response, 0, VARYERR_RESPONSE_SIZE);

    memcpy((char*) &(pVary_Response->request_header),
           (char*) &(pVary_Request->request_header),
           sizeof(REQUEST_HEADER));

    pVary_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pVary_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pVary_Response->message_status.status = STATUS_PROCESS_FAILURE;
    pVary_Response->message_status.type = TYPE_NONE;
    pVary_Response->state = pVary_Request->state;
    pVary_Response->type = pVary_Request->type;
    pVary_Response->count = 0;

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pVary_Response,
                            VARYERR_RESPONSE_SIZE);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    xapi_err_response(pXapireqe,
                      (char*) pVary_Response,
                      VARYERR_RESPONSE_SIZE,
                      STATUS_UNSUPPORTED_COMMAND); 

    return STATUS_UNSUPPORTED_COMMAND;
}


