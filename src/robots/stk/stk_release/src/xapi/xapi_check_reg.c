/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_check_reg.c                                 */
/** Description:    XAPI CHECK REGISTRATION processor.               */
/**                                                                  */
/**                 NOTE: CHECK REGISTRATION (event notification)    */
/**                 commands not are supported by the XAPI.          */
/**                 However the STATUS_UNSUPPORTED_COMMAND           */
/**                 logic of xapi_main was unsuited for              */
/**                 CHECK REGISTRATION error responses so            */
/**                 this stub was created to return the              */
/**                 approriate CHECK REGISTRATION error response.    */
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


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_check_reg                                    */
/** Description:   The XAPI CHECK REGISTRATION processor.            */
/**                                                                  */
/** Build and output an ACSAPI CHECK REGISTRATION error response.    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_check_reg"

extern int xapi_check_reg(struct XAPICVT  *pXapicvt,
                          struct XAPIREQE *pXapireqe)
{
    int                 lastRC;

    CHECK_REGISTRATION_REQUEST *pCheck_Registration_Request = 
    (CHECK_REGISTRATION_REQUEST*) pXapireqe->pAcsapiBuffer;
    CHECK_REGISTRATION_RESPONSE wkCheck_Registration_Response;
    CHECK_REGISTRATION_RESPONSE *pCheck_Registration_Response =
    &(wkCheck_Registration_Response);

    TRMSGI(TRCI_XAPI,
           "Entered; CHECK REGISTRATION request=%08X, size=%d, "
           "CHECK REGISTRATION response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pCheck_Registration_Response,
           sizeof(CHECK_REGISTRATION_RESPONSE));

    memset((char*) pCheck_Registration_Response, 
           0, 
           (sizeof(CHECK_REGISTRATION_RESPONSE)));

    memcpy((char*) &(pCheck_Registration_Response->request_header),
           (char*) &(pCheck_Registration_Request->request_header),
           sizeof(REQUEST_HEADER));

    pCheck_Registration_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pCheck_Registration_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pCheck_Registration_Response->message_status.status = STATUS_PROCESS_FAILURE;
    pCheck_Registration_Response->message_status.type = TYPE_NONE;

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pCheck_Registration_Response,
                            (sizeof(CHECK_REGISTRATION_RESPONSE)));

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    xapi_err_response(pXapireqe,
                      (char*) pCheck_Registration_Response,
                      (sizeof(CHECK_REGISTRATION_RESPONSE)),
                      STATUS_UNSUPPORTED_COMMAND); 

    return STATUS_UNSUPPORTED_COMMAND;
}


