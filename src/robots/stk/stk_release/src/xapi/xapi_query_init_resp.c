/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_query_init_resp.c                           */
/** Description:    Initialize an ACSAPI "generic" QUERY response.   */
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
/** Function Name: xapi_query_init_resp                              */
/** Description:   Initialize an ACSAPI "generic" QUERY response.    */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI "generic" QUERY response consists of:                 */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   QUERY_RESPONSE data consisting of                           */
/**      i.   type (i.e. "query" type)                               */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_query_init_resp"

extern void xapi_query_init_resp(struct XAPIREQE *pXapireqe,
                                 char            *pQueryResponse,
                                 int              queryResponseSize)
{
    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QUERY_RESPONSE     *pQuery_Response     = (QUERY_RESPONSE*) pQueryResponse;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    memset((char*) pQuery_Response, 0, queryResponseSize);

    memcpy((char*) &(pQuery_Response->request_header),
           (char*) &(pQuery_Request->request_header),
           sizeof(REQUEST_HEADER));

    pQuery_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pQuery_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pQuery_Response->message_status.status = STATUS_SUCCESS;
    pQuery_Response->message_status.type = TYPE_NONE;
    pQuery_Response->type = pQuery_Request->type;

    return;
}




