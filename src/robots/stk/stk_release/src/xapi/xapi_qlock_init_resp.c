/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qlock_init_resp.c                           */
/** Description:    Initialize an ACSAPI "generic" QUERY LOCK        */
/**                 response.                                        */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     09/01/11                          */
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
/** Function Name: xapi_qlock_init_resp                              */
/** Description:   Initialize an ACSAPI "generic" QUERY LOCK         */
/**                response.                                         */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI "generic" QUERY LOCK response consists of:            */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   QUERY_LOCK_RESPONSE data consisting of:                     */
/**      i.   type                                                   */
/**      ii.  count                                                  */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qlock_init_resp"

extern void xapi_qlock_init_resp(struct XAPIREQE *pXapireqe,
                                 char            *pQueryResponse,
                                 int              queryResponseSize)
{
    QUERY_LOCK_REQUEST *pQuery_Lock_Request = 
    (QUERY_LOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    QUERY_LOCK_RESPONSE *pQuery_Lock_Response = 
    (QUERY_LOCK_RESPONSE*) pQueryResponse;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    memset((char*) pQuery_Lock_Response, 0, queryResponseSize);

    memcpy((char*) &(pQuery_Lock_Response->request_header),
           (char*) &(pQuery_Lock_Request->request_header),
           sizeof(REQUEST_HEADER));

    pQuery_Lock_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pQuery_Lock_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pQuery_Lock_Response->message_status.status = STATUS_SUCCESS;
    pQuery_Lock_Response->message_status.type = TYPE_NONE;
    pQuery_Lock_Response->type = pQuery_Lock_Request->type;
    pQuery_Lock_Response->count = 0;

    return;
}


