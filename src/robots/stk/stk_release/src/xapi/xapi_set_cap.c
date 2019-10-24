/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_set_cap.c                                   */
/** Description:    XAPI SET CAP processor.                          */
/**                                                                  */
/**                 NOTE: SET CAP (set cap attributes) commands      */
/**                 are not supported by the XAPI. However the       */
/**                 STATUS_UNSUPPORTED_COMMAND logic of              */
/**                 xapi_main was unsuited for SET CAP error         */
/**                 responses so this stub was created to            */
/**                 return the approriate SET CAP error response.    */
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
/** Function Name: xapi_set_cap                                      */
/** Description:   The XAPI SET CAP processor.                       */
/**                                                                  */
/** Build and output an ACSAPI SET CAP error response.               */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_set_cap"

extern int xapi_set_cap(struct XAPICVT  *pXapicvt,
                        struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    int                 setCapResponseSize = 
    offsetof(SET_CAP_RESPONSE, set_cap_status[0]); 

    SET_CAP_REQUEST    *pSet_Cap_Request    = (SET_CAP_REQUEST*) pXapireqe->pAcsapiBuffer;
    SET_CAP_RESPONSE    wkSet_Cap_Response;
    SET_CAP_RESPONSE   *pSet_Cap_Response   = &(wkSet_Cap_Response);

    TRMSGI(TRCI_XAPI,
           "Entered; SET CAP request=%08X, size=%d, "
           "SET_CAP response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pSet_Cap_Response,
           setCapResponseSize);

    memset((char*) pSet_Cap_Response, 
           0, 
           setCapResponseSize);

    memcpy((char*) &(pSet_Cap_Response->request_header),
           (char*) &(pSet_Cap_Request->request_header),
           sizeof(REQUEST_HEADER));

    pSet_Cap_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pSet_Cap_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pSet_Cap_Response->message_status.status = STATUS_PROCESS_FAILURE;
    pSet_Cap_Response->message_status.type = TYPE_NONE;
    pSet_Cap_Response->cap_priority = pSet_Cap_Request->cap_priority;
    pSet_Cap_Response->cap_mode = pSet_Cap_Request->cap_mode;
    pSet_Cap_Response->count = 0;

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pSet_Cap_Response,
                            setCapResponseSize);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    xapi_err_response(pXapireqe,
                      (char*) pSet_Cap_Response,
                      setCapResponseSize,
                      STATUS_UNSUPPORTED_COMMAND); 

    return STATUS_UNSUPPORTED_COMMAND;
}


