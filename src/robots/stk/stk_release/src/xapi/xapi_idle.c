/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_idle.c                                      */
/** Description:    XAPI IDLE processor.                             */
/**                                                                  */
/**                 Place the XAPI client into the IDLE (quiesced)   */
/**                 state.                                           */
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
static void initIdleResponse(struct XAPIREQE *pXapireqe,
                             char            *pIdleResponse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_idle                                         */
/** Description:   The XAPI IDLE processor.                          */
/**                                                                  */
/** Place the XAPI client into the IDLE (quiesced) state.            */
/**                                                                  */
/** Currently, there is no XAPI <idle> transaction that is           */
/** sent to the XAPI server.  The IDLE command is processed          */
/** locally in the XAPI client without server interaction.           */
/**                                                                  */
/** The IDLE command operates by marking the XAPICVT.status as       */
/** XAPI_IDLE_PENDING; If the MESSAGE_HEADER.message_option FORCE    */
/** bit is set, then all currently executing XAPI threads are        */
/** set to cancel themselves by setting their                        */
/** XAPIREQE.requestFlag XAPIREQE_CANCEL bit (see xapi_cancel).      */
/**                                                                  */
/** If XAPICVT.status equals XAPI_IDLE_PENDING, then xapi_main,      */
/** at thread termination time, will change the XAPICVT.status       */
/** from XAPI_IDLE_PENDING to XAPI_IDLE if the terminating thread    */
/** is the last XAPI thread.                                         */
/**                                                                  */
/** The XAPI client has 3 possible status; XAPI_ACTIVE,              */
/** XAPI_IDLE, and XAPI_IDLE_PENDING.  Only CANCEL, IDLE, QUERY,     */
/** and START commands are allowed when the status is XAPI_IDLE,     */
/** or IDLE_PENDING (VARY is also documented as being allowed, but   */
/** VARY commands are unsupported by the XAPI).                      */
/**                                                                  */
/** Even though it is redundant, the IDLE command is allowed to      */
/** proceed even when the XAPI client is in the IDLE state.          */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI IDLE request consists of:                             */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/**                                                                  */
/** NOTE: There is no IDLE request data.                             */
/** However, the MESSAGE_HEADER.message_option FORCE bit indicates   */
/** to cancel all pending operations.                                */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_idle"

extern int xapi_idle(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    int                 i;

    struct XAPIREQE    *pCurrXapireqe;

    IDLE_REQUEST       *pIdle_Request       = (IDLE_REQUEST*) pXapireqe->pAcsapiBuffer;
    MESSAGE_HEADER     *pMessage_Header     = &(pIdle_Request->request_header.message_header);

    IDLE_RESPONSE       wkIdle_Response;
    IDLE_RESPONSE      *pIdle_Response      = &wkIdle_Response;

    char                forceFlag           = FALSE;

    if (pMessage_Header->message_options & FORCE)
    {
        forceFlag = TRUE;
    }

    TRMSGI(TRCI_XAPI,
           "Entered; IDLE request=%08X, size=%d, "
           "forceFlag=%d, IDLE response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           forceFlag,
           pIdle_Response,
           sizeof(IDLE_RESPONSE));

    initIdleResponse(pXapireqe,
                     (char*) pIdle_Response);

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

    pXapicvt->status = XAPI_IDLE_PENDING;

    /*****************************************************************/
    /* If FORCE specifed, then run through the XAPIREQE queue        */
    /* of active requests, and mark all requests to cancel           */
    /* themselves.                                                   */
    /*****************************************************************/
    if (forceFlag)
    {
        for (i = 0;
            i < MAX_XAPIREQE;
            i++)
        {
            pCurrXapireqe = (struct XAPIREQE*) &(pXapicvt->xapireqe[i]);

            if ((pCurrXapireqe->requestFlag & XAPIREQE_START) &&
                (!(pCurrXapireqe->requestFlag & XAPIREQE_END)))
            {
                pCurrXapireqe->requestFlag |= XAPIREQE_CANCEL;
            }
        }
    }

    xapi_fin_response(pXapireqe,
                      (char*) pIdle_Response,     
                      sizeof(IDLE_RESPONSE));  

    /*****************************************************************/
    /* When we exit via xapi_main, and the XAPICVT indicates         */
    /* XAPI_IDLE_PENDING status, then a test will be done to         */
    /* determine if the XAPI can be transitioned to XAPI_IDLE        */
    /* status.                                                       */
    /*****************************************************************/
    return STATUS_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: initIdleResponse                                  */
/** Description:   Initialize the ACSAPI IDLE response.              */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI IDLE response consists of:                            */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/**                                                                  */
/** NOTE: There is no IDLE response data.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "initIdleResponse "

static void initIdleResponse(struct XAPIREQE *pXapireqe,
                             char            *pIdleResponse)
{
    IDLE_REQUEST       *pIdle_Request       = (IDLE_REQUEST*) pXapireqe->pAcsapiBuffer;
    IDLE_RESPONSE      *pIdle_Response      = (IDLE_RESPONSE*) pIdleResponse;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Initialize IDLE response.                                     */
    /*****************************************************************/
    memset((char*) pIdle_Response, 0, sizeof(IDLE_RESPONSE));

    memcpy((char*) &(pIdle_Response->request_header),
           (char*) &(pIdle_Request->request_header),
           sizeof(REQUEST_HEADER));

    pIdle_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pIdle_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pIdle_Response->message_status.status = STATUS_SUCCESS;
    pIdle_Response->message_status.type = TYPE_NONE;

    return;
}




