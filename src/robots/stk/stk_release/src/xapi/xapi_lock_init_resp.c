/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_lock_init_resp.c                            */
/** Description:    Initialize an ACSAPI "generic" LOCK,             */
/**                 UNLOCK, and CLEAR_LOCK response.                 */
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
/** Function Name: xapi_lock_init_resp                               */
/** Description:   Initialize an ACSAPI "generic" LOCK,              */
/**                UNLOCK, and CLEAR_LOCK response.                  */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI "generic" LOCK, UNLOCK, and CLEAR_LOCK response       */
/** consists of:                                                     */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   LOCK_RESPONSE, UNLOCK_RESPONSE, or CLEAR_LOCK_RESPONSE      */
/**      data consisting of:                                         */
/**      i.   type                                                   */
/**      ii.  count                                                  */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_lock_init_resp"

extern void xapi_lock_init_resp(struct XAPIREQE *pXapireqe,
                                char            *pLockResponse,
                                int              lockResponseSize)
{
    LOCK_REQUEST       *pLock_Request       = (LOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    LOCK_RESPONSE      *pLock_Response      = (LOCK_RESPONSE*) pLockResponse;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    memset((char*) pLock_Response, 0, lockResponseSize);

    memcpy((char*) &(pLock_Response->request_header),
           (char*) &(pLock_Request->request_header),
           sizeof(REQUEST_HEADER));

    pLock_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pLock_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pLock_Response->message_status.status = STATUS_SUCCESS;
    pLock_Response->message_status.type = TYPE_NONE;
    pLock_Response->type = pLock_Request->type;
    pLock_Response->count = 0;

    return;
}


