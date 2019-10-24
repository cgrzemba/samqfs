/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_delete_pool.c                               */
/** Description:    XAPI DELETE POOL processor.                      */
/**                                                                  */
/**                 NOTE: DELETE POOL commands are not               */
/**                 supported by the XAPI.  However, the             */
/**                 STATUS_UNSUPPORTED_COMMAND logic of              */
/**                 xapi_main was unsuited for DELETE POOL           */
/**                 error responses so this stub was created         */ 
/**                 to return the approriate DELETE POOL             */
/**                 error response.                                  */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     07/15/11                          */
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


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#define DELETEERR_RESPONSE_SIZE (offsetof(DELETE_POOL_RESPONSE, pool_status[0])) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_delete_pool                                  */
/** Description:   The XAPI DELETE POOL processor.                   */
/**                                                                  */
/** Build and output an ACSAPI DELETE POOL error response.           */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_delete_pool"

extern int xapi_delete_pool(struct XAPICVT  *pXapicvt,
                            struct XAPIREQE *pXapireqe)
{
    int                 lastRC;

    DELETE_POOL_REQUEST *pDelete_Pool_Request = 
    (DELETE_POOL_REQUEST*) pXapireqe->pAcsapiBuffer;
    char                wkDeletePoolResponse[DELETEERR_RESPONSE_SIZE];
    DELETE_POOL_RESPONSE *pDelete_Pool_Response = 
    (DELETE_POOL_RESPONSE*) wkDeletePoolResponse;

    TRMSGI(TRCI_XAPI,
           "Entered; DELETE POOL request=%08X, size=%d, "
           "DELETE POOL response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pDelete_Pool_Response,
           DELETEERR_RESPONSE_SIZE);

    memset((char*) pDelete_Pool_Response, 0, DELETEERR_RESPONSE_SIZE);

    memcpy((char*) &(pDelete_Pool_Response->request_header),
           (char*) &(pDelete_Pool_Request->request_header),
           sizeof(REQUEST_HEADER));

    pDelete_Pool_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pDelete_Pool_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pDelete_Pool_Response->message_status.status = STATUS_PROCESS_FAILURE;
    pDelete_Pool_Response->message_status.type = TYPE_NONE;
    pDelete_Pool_Response->count = 0;

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pDelete_Pool_Response,
                            DELETEERR_RESPONSE_SIZE);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    xapi_err_response(pXapireqe,
                      (char*) pDelete_Pool_Response,
                      DELETEERR_RESPONSE_SIZE,
                      STATUS_UNSUPPORTED_COMMAND); 

    return STATUS_UNSUPPORTED_COMMAND;
}


