/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_register.c                                  */
/** Description:    XAPI REGISTER processor.                         */
/**                                                                  */
/**                 NOTE: REGISTER (event notification) commands     */
/**                 are not supported by the XAPI. However the       */
/**                 STATUS_UNSUPPORTED_COMMAND logic of              */
/**                 xapi_main was unsuited for REGISTER error        */
/**                 responses so this stub was created to return     */
/**                 the approriate REGISTER error response.          */
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
/** Function Name: xapi_register                                     */
/** Description:   The XAPI REGISTER processor.                      */
/**                                                                  */
/** Build and output an ACSAPI REGISTER error response.              */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_register"

extern int xapi_register(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe)
{
    int                 lastRC;

    REGISTER_REQUEST   *pRegister_Request   = (REGISTER_REQUEST*) pXapireqe->pAcsapiBuffer;
    REGISTER_RESPONSE   wkRegister_Response;
    REGISTER_RESPONSE  *pRegister_Response  = &(wkRegister_Response);

    TRMSGI(TRCI_XAPI,
           "Entered; REGISTER request=%08X, size=%d, "
           "REGISTER response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pRegister_Response,
           sizeof(REGISTER_RESPONSE));

    memset((char*) pRegister_Response, 
           0, 
           (sizeof(REGISTER_RESPONSE)));

    memcpy((char*) &(pRegister_Response->request_header),
           (char*) &(pRegister_Request->request_header),
           sizeof(REQUEST_HEADER));

    pRegister_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pRegister_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pRegister_Response->message_status.status = STATUS_PROCESS_FAILURE;
    pRegister_Response->message_status.type = TYPE_NONE;

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pRegister_Response,
                            (sizeof(REGISTER_RESPONSE)));

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    xapi_err_response(pXapireqe,
                      (char*) pRegister_Response,
                      (sizeof(REGISTER_RESPONSE)),
                      STATUS_UNSUPPORTED_COMMAND); 

    return STATUS_UNSUPPORTED_COMMAND;
}


