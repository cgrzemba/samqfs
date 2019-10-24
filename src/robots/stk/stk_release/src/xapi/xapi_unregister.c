/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_unregister.c                                */
/** Description:    XAPI UNREGISTER processor.                       */
/**                                                                  */
/**                 NOTE: UNREGISTER (event notification) commands   */
/**                 are not supported by the XAPI. However the       */
/**                 STATUS_UNSUPPORTED_COMMAND logic of              */
/**                 xapi_main was unsuited for UNREGISTER error      */
/**                 responses so this stub was created to return     */
/**                 the approriate UNREGISTER error response.        */
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


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_unregister                                   */
/** Description:   The XAPI UNREGISTER processor.                    */
/**                                                                  */
/** Build and output an ACSAPI UNREGISTER error response.            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_unregister"

extern int xapi_unregister(struct XAPICVT  *pXapicvt,
                           struct XAPIREQE *pXapireqe)
{
    int                 lastRC;

    UNREGISTER_REQUEST  *pUnregister_Request  = 
    (UNREGISTER_REQUEST*) pXapireqe->pAcsapiBuffer;
    UNREGISTER_RESPONSE  wkUnregister_Response;
    UNREGISTER_RESPONSE *pUnregister_Response = &(wkUnregister_Response);

    TRMSGI(TRCI_XAPI,
           "Entered; UNREGISTER request=%08X, size=%d, "
           "UNREGISTER response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pUnregister_Response,
           sizeof(UNREGISTER_RESPONSE));

    memset((char*) pUnregister_Response, 
           0, 
           (sizeof(UNREGISTER_RESPONSE)));

    memcpy((char*) &(pUnregister_Response->request_header),
           (char*) &(pUnregister_Request->request_header),
           sizeof(REQUEST_HEADER));

    pUnregister_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pUnregister_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pUnregister_Response->message_status.status = STATUS_PROCESS_FAILURE;
    pUnregister_Response->message_status.type = TYPE_NONE;

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pUnregister_Response,
                            (sizeof(UNREGISTER_RESPONSE)));

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    xapi_err_response(pXapireqe,
                      (char*) pUnregister_Response,
                      (sizeof(UNREGISTER_RESPONSE)),
                      STATUS_UNSUPPORTED_COMMAND); 

    return STATUS_UNSUPPORTED_COMMAND;
}


