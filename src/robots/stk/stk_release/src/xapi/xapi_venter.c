/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_venter.c                                    */
/** Description:    XAPI VENTER processor.                           */
/**                                                                  */
/**                 NOTE: VENTER commands are not supported          */
/**                 by the XAPI.  However, the                       */
/**                 STATUS_UNSUPPORTED_COMMAND logic of              */
/**                 xapi_main was unsuited for VENTER                */
/**                 error responses so this stub was created         */ 
/**                 to return the approriate VENTER                  */
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
#define ENTERERR_RESPONSE_SIZE (offsetof(ENTER_RESPONSE, volume_status)) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_venter                                       */
/** Description:   The XAPI VENTER processor.                        */
/**                                                                  */
/** Build and output an ACSAPI VENTER error response.                */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_venter"

extern int xapi_venter(struct XAPICVT  *pXapicvt,
                       struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    VENTER_REQUEST     *pVenter_Request     = (VENTER_REQUEST*) pXapireqe->pAcsapiBuffer;
    char                pWkEnterResponse[ENTERERR_RESPONSE_SIZE];
    ENTER_RESPONSE     *pEnter_Response     = (ENTER_RESPONSE*) pWkEnterResponse;     

    TRMSGI(TRCI_XAPI,
           "Entered; VENTER request=%08X, size=%d, "
           "VENTER response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pEnter_Response,
           ENTERERR_RESPONSE_SIZE);

    memset((char*) pEnter_Response, 0, ENTERERR_RESPONSE_SIZE);

    memcpy((char*) &(pEnter_Response->request_header),
           (char*) &(pVenter_Request->request_header),
           sizeof(REQUEST_HEADER));

    pEnter_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pEnter_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pEnter_Response->message_status.status = STATUS_PROCESS_FAILURE;
    pEnter_Response->message_status.type = TYPE_NONE;

    memcpy((char*) &(pEnter_Response->cap_id),
           (char*) &(pVenter_Request->cap_id),
           sizeof(CAPID));

    pEnter_Response->count = 0;

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pEnter_Response,
                            ENTERERR_RESPONSE_SIZE);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    xapi_err_response(pXapireqe,
                      (char*) pEnter_Response,
                      ENTERERR_RESPONSE_SIZE,
                      STATUS_UNSUPPORTED_OPTION); 

    return STATUS_UNSUPPORTED_OPTION;
}


