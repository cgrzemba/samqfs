/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_audit.c                                     */
/** Description:    XAPI AUDIT processor.                            */
/**                                                                  */
/**                 NOTE: AUDIT commands are not supported           */
/**                 by the XAPI.  However, the                       */
/**                 STATUS_UNSUPPORTED_COMMAND logic of              */
/**                 xapi_main was unsuited for AUDIT                 */
/**                 error responses so this stub was created         */ 
/**                 to return the approriate AUDIT                   */
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

#include "srvcommon.h"
#include "xapi.h"
#include "api/defs_api.h"
#include "csi.h"


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#define AUDITERR_RESPONSE_SIZE (offsetof(AUDIT_RESPONSE, identifier_status)) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_audit                                        */
/** Description:   The XAPI AUDIT processor.                         */
/**                                                                  */
/** Build and output an ACSAPI AUDIT error response.                 */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_audit"

extern int xapi_audit(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    AUDIT_REQUEST      *pAudit_Request      = (AUDIT_REQUEST*) pXapireqe->pAcsapiBuffer;
    char                wkAuditResponse[AUDITERR_RESPONSE_SIZE];
    AUDIT_RESPONSE     *pAudit_Response     = (AUDIT_RESPONSE*) wkAuditResponse;     

    TRMSGI(TRCI_XAPI,
           "Entered; AUDIT request=%08X, size=%d, type=%d, "
           "AUDIT response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pAudit_Request->type,
           pAudit_Response,
           AUDITERR_RESPONSE_SIZE);

    memset((char*) pAudit_Response, 0, AUDITERR_RESPONSE_SIZE);

    memcpy((char*) &(pAudit_Response->request_header),
           (char*) &(pAudit_Request->request_header),
           sizeof(REQUEST_HEADER));

    pAudit_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pAudit_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pAudit_Response->message_status.status = STATUS_PROCESS_FAILURE;
    pAudit_Response->message_status.type = TYPE_NONE;
    pAudit_Response->type = pAudit_Request->type;
    pAudit_Response->count = 0;

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pAudit_Response,
                            AUDITERR_RESPONSE_SIZE);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    xapi_err_response(pXapireqe,
                      (char*) pAudit_Response,
                      AUDITERR_RESPONSE_SIZE,
                      STATUS_UNSUPPORTED_COMMAND); 

    return STATUS_UNSUPPORTED_COMMAND;
}


