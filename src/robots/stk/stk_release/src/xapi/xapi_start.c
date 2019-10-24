/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_start.c                                     */
/** Description:    XAPI START processor.                            */
/**                                                                  */
/**                 Place the XAPI client into the START (fully      */
/**                 active) state.                                   */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     08/01/11                          */
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
/* Prototypes:                                                       */
/*********************************************************************/
static void initStartResponse(struct XAPIREQE *pXapireqe,
                              char            *pStartResponse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_start                                        */
/** Description:   The XAPI START processor.                         */
/**                                                                  */
/** Place the XAPI client into the START (fully active) state.       */
/**                                                                  */
/** Currently, there is no XAPI <start> transaction that is          */
/** sent to the XAPI server.  The START command is processed         */
/** locally in the XAPI client without server interaction.           */
/**                                                                  */
/** The START command operates by marking the XAPICVT.status as      */
/** XAPI_ACTIVE.  The XAPI client has 3 possible status; XAPI_ACTIVE,*/
/** XAPI_IDLE, and XAPI_IDLE_PENDING.  Only CANCEL, IDLE, QUERY,     */
/** and START commands are allowed when the status is XAPI_IDLE,     */
/** or IDLE_PENDING (VARY is also documented as being allowed, but   */
/** VARY commands are unsupported by the XAPI).  Otherwise all       */
/** command are allowed (though they may not be supported by the     */
/** XAPI client).                                                    */
/**                                                                  */
/** The START command is allowed to proceed when the XAPI client     */
/** is in the IDLE state.                                            */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI START request consists of:                            */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/**                                                                  */
/** NOTE: There is no START request data.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_start"

extern int xapi_start(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe)
{
    int                 lastRC;

    START_RESPONSE      wkStart_Response;
    START_RESPONSE     *pStart_Response     = &wkStart_Response;

    TRMSGI(TRCI_XAPI,
           "Entered; START request=%08X, size=%d, "
           "START response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pStart_Response,
           sizeof(START_RESPONSE));

    initStartResponse(pXapireqe,
                      (char*) pStart_Response);

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

    pXapicvt->status = XAPI_ACTIVE;

    xapi_fin_response(pXapireqe,
                      (char*) pStart_Response,     
                      sizeof(START_RESPONSE));  

    return STATUS_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: initStartResponse                                 */
/** Description:   Initialize the ACSAPI START response.             */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI START response consists of:                           */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/**                                                                  */
/** NOTE: There is no START response data.                           */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "initStartResponse "

static void initStartResponse(struct XAPIREQE *pXapireqe,
                              char            *pStartResponse)
{
    START_REQUEST      *pStart_Request      = (START_REQUEST*) pXapireqe->pAcsapiBuffer;
    START_RESPONSE     *pStart_Response     = (START_RESPONSE*) pStartResponse;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Initialize START response.                                     */
    /*****************************************************************/
    memset((char*) pStart_Response, 0, sizeof(START_RESPONSE));

    memcpy((char*) &(pStart_Response->request_header),
           (char*) &(pStart_Request->request_header),
           sizeof(REQUEST_HEADER));

    pStart_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pStart_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pStart_Response->message_status.status = STATUS_SUCCESS;
    pStart_Response->message_status.type = TYPE_NONE;

    return;
}




