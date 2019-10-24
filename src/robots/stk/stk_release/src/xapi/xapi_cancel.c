/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_cancel.c                                    */
/** Description:    XAPI CANCEL processor.                           */
/**                                                                  */
/**                 Cancel an executing XAPI thread by matching      */
/**                 seqNumber.                                       */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     05/11/11                          */
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
static void initCancelResponse(struct XAPIREQE *pXapireqe,
                               char            *pCancelResponse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_cancel                                       */
/** Description:   The XAPI CANCEL processor.                        */
/**                                                                  */
/** Cancel a currently executing XAPI thread with the matching       */
/** seqNumber.                                                       */
/**                                                                  */
/** Currently, there is no XAPI <cancel> transaction that is         */
/** sent to the XAPI server.  The CANCEL command is processed        */
/** locally in the XAPI client without server interaction.           */
/**                                                                  */
/** The CANCEL command operates by marking the appropriate           */
/** XAPIREQE table entry to cancel itself by setting the             */
/** XAPIREQE.requestFlag XAPIREQE_CANCEL bit.  The XAPIREQE          */
/** table entry that is marked to cancel itself is NOT this          */
/** thread's XAPIREQE, but the XAPIREQE of some parallel (still      */
/** executing) thread.                                               */
/**                                                                  */
/** The XAPIREQE.requestFlag is checked periodically by the parallel */
/** executing thread (in xapi_tcp); If the flag is set, then the     */
/** executing thread will exit prematurely with the appropriate      */
/** error final response (STATUS_CANCELLED).                         */
/**                                                                  */
/** The CANCEL command treats MESSAGE_HEADER.packet_id               */
/** (i.e. seqNumber) in non-standard way.  The                       */
/** MESSAGE_HEADER.packet_id is treated as the seqNumber of          */
/** the request to cancel.                                           */
/**                                                                  */
/** The CANCEL command's MESSAGE_HEADER.packet_id  (i.e. this        */
/** command's seqNumber) is then set to 0 to make this CANCEL        */
/** command effectively non-cancellable).                            */
/**                                                                  */
/** The CANCEL command is allowed to proceed even when the           */
/** XAPI client is in the IDLE state (so we do NOT call              */
/** xapi_idle_test).                                                 */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI CANCEL request consists of:                           */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   CANCEL_REQUEST data consisting of                           */
/**      i.   message_id                                             */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_cancel"

extern int xapi_cancel(struct XAPICVT  *pXapicvt,
                       struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    int                 acsapiRC;
    int                 cancelSeqNumber;
    int                 i;
    struct XAPIREQE    *pCurrXapireqe;

    CANCEL_REQUEST     *pCancel_Request     = (CANCEL_REQUEST*) pXapireqe->pAcsapiBuffer;
    MESSAGE_HEADER     *pMessage_Header     = &(pCancel_Request->request_header.message_header);

    CANCEL_RESPONSE     wkCancel_Response;
    CANCEL_RESPONSE    *pCancel_Response    = &wkCancel_Response;

    /*****************************************************************/
    /* NOTE: CANCEL treats packet_id in non-standard way.  The       */
    /* MESSAGE_HEADER.packet_id is the request to cancel.  The       */
    /* CANCEL command's packet_id is then set to 0 (to make the      */
    /* CANCEL command effectively non-cancellable).                  */
    /*****************************************************************/
    cancelSeqNumber = pMessage_Header->packet_id;
    pXapireqe->seqNumber = 0;

    TRMSGI(TRCI_XAPI,
           "Entered; CANCEL request=%08X, size=%d, "
           "CANCEL response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pCancel_Response,
           sizeof(CANCEL_RESPONSE));

    initCancelResponse(pXapireqe,
                       (char*) pCancel_Response);

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

    TRMSGI(TRCI_XAPI,
           "cancelSeqNumber=%d\n",
           cancelSeqNumber);

    pCancel_Response->request = pCancel_Request->request;

    /*****************************************************************/
    /* Loop through the XAPIREQE queue to find the matching          */
    /* seqNumber.                                                    */
    /*****************************************************************/
    pCancel_Response->message_status.status = STATUS_MESSAGE_NOT_FOUND;

    for (i = 0;
        i < MAX_XAPIREQE;
        i++)
    {
        pCurrXapireqe = (struct XAPIREQE*) &(pXapicvt->xapireqe[i]);

        if (pCurrXapireqe->seqNumber == cancelSeqNumber)
        {
            TRMSGI(TRCI_XAPI,
                   "Found matching seqNumber=%d; XAPIREQE=%08X, "
                   "index=%d, requestFlag=%02X\n",
                   pCurrXapireqe->seqNumber,
                   pCurrXapireqe,
                   i,
                   pCurrXapireqe->requestFlag);

            if (pCurrXapireqe->requestFlag & XAPIREQE_START)
            {
                if (pCurrXapireqe->requestFlag & XAPIREQE_END)
                {
                    TRMSGI(TRCI_XAPI,
                           "Request already completed\n");
                }
                /*****************************************************/
                /* If the request is eligible to be cancelled, mark  */
                /* its XAPIREQE entry as cancelled; it is then up    */
                /* to the TCP/IP process in xapi_tcp to recognize    */
                /* that the request has been cancelled and to        */
                /* exit.                                             */
                /*****************************************************/
                else
                {
                    LOGMSG(STATUS_SUCCESS,
                           "XAPI CANCEL; Marking XAPIREQE entry[%d] "
                           "seq=%d cancelled",
                           (i + 1),
                           cancelSeqNumber);

                    pCurrXapireqe->requestFlag |= XAPIREQE_CANCEL;

                    pCancel_Response->message_status.status = STATUS_SUCCESS;
                }
            }

            break;
        }
    }

    if (pCancel_Response->message_status.status != STATUS_SUCCESS)
    {
        LOGMSG(pCancel_Response->message_status.status,
               "XAPI CANCEL; seq=%d not found or not eligible for cancel",
               cancelSeqNumber);
    }

    xapi_fin_response(pXapireqe,
                      (char*) pCancel_Response,
                      sizeof(CANCEL_RESPONSE));

    return(int) pCancel_Response->message_status.status;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: initCancelResponse                                */
/** Description:   Initialize the ACSAPI CANCEL response.            */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI CANCEL response consists of:                          */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   CANCEL_RESPONSE data consisting of:                         */
/**      i.   RESPONSE_STATUS; consisting of:                        */
/**           a.   status                                            */
/**           b.   type                                              */
/**           c.   identifier                                        */
/**      ii.  message_id                                             */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "initCancelResponse"

static void initCancelResponse(struct XAPIREQE *pXapireqe,
                               char            *pCancelResponse)
{
    CANCEL_REQUEST     *pCancel_Request     = (CANCEL_REQUEST*) pXapireqe->pAcsapiBuffer;
    CANCEL_RESPONSE    *pCancel_Response    = (CANCEL_RESPONSE*) pCancelResponse;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Initialize CANCEL response.                                   */
    /*****************************************************************/
    memset((char*) pCancel_Response, 0, sizeof(CANCEL_RESPONSE));

    memcpy((char*) &(pCancel_Response->request_header),
           (char*) &(pCancel_Request->request_header),
           sizeof(REQUEST_HEADER));

    pCancel_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pCancel_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pCancel_Response->message_status.status = STATUS_SUCCESS;
    pCancel_Response->message_status.type = TYPE_NONE;

    return;
}


