/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_display.c                                   */
/** Description:    XAPI DISPLAY processor.                          */
/**                                                                  */
/**                 NOTE: ACSAPI-type DISPLAY commands are           */
/**                 not supported by the XAPI.  However, the         */
/**                 STATUS_UNSUPPORTED_COMMAND logic of              */
/**                 xapi_main was unsuited for DISPLAY               */
/**                 error responses so this stub was created         */ 
/**                 to return the approriate DISPLAY                 */
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
#define DISPLAYERR_RESPONSE_SIZE (sizeof(DISPLAY_RESPONSE) - (MAX_XML_DATA_SIZE) + 1) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_display                                      */
/** Description:   The XAPI DISPLAY processor.                       */
/**                                                                  */
/** Build and output an ACSAPI DISPLAY error response.               */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_display"

extern int xapi_display(struct XAPICVT  *pXapicvt,
                        struct XAPIREQE *pXapireqe)
{
    int                 lastRC;

    DISPLAY_REQUEST    *pDisplay_Request    = (DISPLAY_REQUEST*) pXapireqe->pAcsapiBuffer;
    char                wkDisplayResponse[DISPLAYERR_RESPONSE_SIZE];
    DISPLAY_RESPONSE   *pDisplay_Response   = (DISPLAY_RESPONSE*) wkDisplayResponse;

    TRMSGI(TRCI_XAPI,
           "Entered; DISPLAY request=%08X, size=%d, "
           "DISPLAY response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pDisplay_Response,
           DISPLAYERR_RESPONSE_SIZE);

    memset((char*) pDisplay_Response, 0, DISPLAYERR_RESPONSE_SIZE);

    memcpy((char*) &(pDisplay_Response->request_header),
           (char*) &(pDisplay_Request->request_header),
           sizeof(REQUEST_HEADER));

    pDisplay_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pDisplay_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pDisplay_Response->message_status.status = STATUS_PROCESS_FAILURE;
    pDisplay_Response->message_status.type = TYPE_NONE;
    pDisplay_Response->display_type = pDisplay_Request->display_type;
    pDisplay_Response->display_xml_data.length = 0;

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pDisplay_Response,
                            DISPLAYERR_RESPONSE_SIZE);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    xapi_err_response(pXapireqe,
                      (char*) pDisplay_Response,
                      DISPLAYERR_RESPONSE_SIZE,
                      STATUS_UNSUPPORTED_COMMAND); 

    return STATUS_UNSUPPORTED_COMMAND;
}


