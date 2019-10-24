/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qrequest.c                                  */
/** Description:    XAPI QUERY REQUEST processor.                    */
/**                                                                  */
/**                 Return message IDs and command codes for         */
/**                 currently executing XAPI requests.               */
/**                                                                  */
/**                 NOTE: Currently, there is no XAPI QUERY_REQUEST  */
/**                 transaction that is sent to the XAPI server.     */
/**                                                                  */
/**                 QUERY REQUEST is processed locally by            */
/**                 searching the XAPIREQE table entry for           */
/**                 the specified request ID(s) (sequence number).   */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     07/05/11                          */
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
#define QREQERR_RESPONSE_SIZE (offsetof(QUERY_RESPONSE, status_response)) + \
                              (offsetof(QU_REQ_RESPONSE, request_status[0])) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qrequest                                     */
/** Description:   The XAPI QUERY_REQUEST processor.                 */
/**                                                                  */
/** Return information concerning currently executing XAPI           */
/** command threads contained in the XAPI client XAPIREQE            */
/** table for the requested message IDs (or return information       */
/** concerning all currently executing XAPI command threads          */
/** if the input request count is 0).                                */
/**                                                                  */
/** The XAPIREQE is a task table embedded withing the XAPICVT        */
/** that records status information for currently executing          */
/** XAPI client command (request) threads.  There is a fixed         */
/** number of XAPIREQE  table entries (so there is a maximum         */
/** number of XAPI client requests that can be simultaneously        */
/** active).                                                         */
/**                                                                  */
/** The XAPIREQE table entry is assigned by ssi/csi_ipcdisp.         */
/**                                                                  */
/** The QUERY REQUEST command is allowed to proceed even             */
/** when the XAPI client is in the IDLE state.                       */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY REQUEST request consists of:                    */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY_REQUEST data consisting of:                           */
/**      i.   type                                                   */
/** 3.   QU_REQ_CRITERIA data consisting of:                         */
/**      i.   request_count (of requests or 0 for "all")             */
/**      ii.  MESSAGE_ID[count] data entries                         */
/**                                                                  */
/** NOTE: MAX_ID is 42.  So a max subset of 42 request(s) can be     */
/** specified at once.  If 0 request(s) is specified, then all       */
/** request(s) are returned in possible multiple intermediate        */
/** responses.                                                       */
/**                                                                  */
/** NOTE: The ACSAPI refers to SEQ_NO, PACKET_ID,                    */
/** and REQ_ID.  They really all refer to the same number.           */
/**                                                                  */
/** o  The SEQ_NO is assigned by the user for each request.          */
/**    It is not necessarilly sequential.                            */
/** o  The PACKET_ID is the SEQ_NO within the ACSAPI/CSI/SSI         */
/** o  The REQ_ID is the SEQ_NO requested by the ACSAPI CANCEL       */
/**    or QUERY REQUEST commands (i.e. the request to cancel or      */
/**    report).                                                      */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY REQUEST response consists of:                   */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   QUERY_RESPONSE data consisting of:                          */
/**      i.   type                                                   */
/**      ii.  QU_REQ_RESPONSE data consisting of:                    */
/**           a.   count                                             */
/**           b.   QU_REQ_STATUS[count] data entries consisting of:  */
/**                1.   MESSAGE_ID                                   */
/**                2.   COMMAND                                      */
/**                3.   STATUS                                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qrequest"

extern int xapi_qrequest(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    int                 qrequestResponseSize;
    int                 i;
    int                 j;
    struct XAPIREQE    *pCurrXapireqe;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_REQ_CRITERIA    *pQu_Req_Criteria    = &(pQuery_Request->select_criteria.request_criteria);

    int                 reqCount            = pQu_Req_Criteria->request_count;
    char                foundFlag;

    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;
    QU_REQ_RESPONSE    *pQu_Req_Response    = &(pQuery_Response->status_response.request_response);
    QU_REQ_STATUS      *pQu_Req_Status      = &(pQu_Req_Response->request_status[0]);

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         sizeof(QU_REQ_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY REQUEST request=%08X, size=%d, "
           "count=%d, MAX_ID=%d, "
           "QUERY REQUEST response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           reqCount,
           MAX_ID,
           pQuery_Response,
           sizeof(QU_REQ_RESPONSE));

    if (reqCount < 0)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QREQERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_SMALL); 

        return STATUS_COUNT_TOO_SMALL;
    }

    if (reqCount > MAX_ID)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QREQERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_LARGE); 

        return STATUS_COUNT_TOO_LARGE;
    }

    if (reqCount == 0)
    {
        qrequestResponseSize = (char*) pQu_Req_Status -
                               (char*) pQuery_Response +
                               ((sizeof(QU_REQ_STATUS)) * MAX_ID);
    }
    else
    {
        qrequestResponseSize = (char*) pQu_Req_Status -
                               (char*) pQuery_Response +
                               ((sizeof(QU_REQ_STATUS)) * reqCount);
    }

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

    /*****************************************************************/
    /* If reqCount = 0 (return all ACTIVE/PENDING requests), then    */
    /* just run through the XAPIREQE and return all active requests. */
    /*****************************************************************/
    if (reqCount == 0)
    {
        for (i = 0, j = 0, 
             pQu_Req_Status = 
             (QU_REQ_STATUS*) &(pQu_Req_Response->request_status[0]);
            i < MAX_XAPIREQE;
            i++)
        {
            pCurrXapireqe = (struct XAPIREQE*) &(pXapicvt->xapireqe[i]);

            if ((pCurrXapireqe->requestFlag > 0) &&
                (!(pCurrXapireqe->requestFlag & XAPIREQE_END)))
            {
                pQu_Req_Status->request = (MESSAGE_ID) pCurrXapireqe->seqNumber;
                pQu_Req_Status->command = (COMMAND) pCurrXapireqe->command; 
                pQu_Req_Status->status = STATUS_CURRENT;

                j++; 
                pQu_Req_Status++;

                if (j >= MAX_ID)
                {
                    pQu_Req_Response->request_count = j;

                    if ((i + 1) >= MAX_XAPIREQE)
                    {
                        xapi_fin_response(pXapireqe,
                                          (char*) pQuery_Response,     
                                          qrequestResponseSize);  
                    }
                    else
                    {
                        xapi_int_response(pXapireqe,
                                          (char*) pQuery_Response,     
                                          qrequestResponseSize);  
                    }
                }

                j = 0;

                xapi_query_init_resp(pXapireqe,
                                     (char*) pQuery_Response,
                                     qrequestResponseSize);

                pQu_Req_Status = 
                (QU_REQ_STATUS*) &(pQu_Req_Response->request_status[0]);
            }
        }

        if (j > 0)
        {
            pQu_Req_Response->request_count = j;
        }

        qrequestResponseSize = (char*) pQu_Req_Status -
                               (char*) pQuery_Response +
                               ((sizeof(QU_REQ_STATUS)) * j);

        xapi_fin_response(pXapireqe,
                          (char*) pQuery_Response,     
                          qrequestResponseSize);  
    }
    /*****************************************************************/
    /* Otherwise, if reqCount > 0 then run through the input list    */
    /* of request_id(s) (MESSAGE_ID(s)) and then try to find the     */
    /* matching request_id (seqNumber) in the XAPIREQE.              */
    /*****************************************************************/
    else
    {
        pQu_Req_Response->request_count = reqCount;

        for (j = 0, 
             pQu_Req_Status = 
             (QU_REQ_STATUS*) &(pQu_Req_Response->request_status[0]);
            j < reqCount;
            j++, pQu_Req_Status++)
        {
            pQu_Req_Status->request = 
            (MESSAGE_ID) pQu_Req_Criteria->request_id[j]; 

            foundFlag = FALSE;

            for (i = 0;
                i < MAX_XAPIREQE;
                i++)
            {
                pCurrXapireqe = (struct XAPIREQE*) &(pXapicvt->xapireqe[i]);

                if ((pCurrXapireqe->seqNumber == 
                     (int) pQu_Req_Criteria->request_id[j]) &&
                    (pCurrXapireqe->requestFlag > 0) &&
                    (!(pCurrXapireqe->requestFlag & XAPIREQE_END)))
                {
                    foundFlag = TRUE;

                    pQu_Req_Status->command = (COMMAND) pCurrXapireqe->command;
                    pQu_Req_Status->status = STATUS_CURRENT;
                }
            }

            if (!(foundFlag))
            {
                pQu_Req_Status->status = STATUS_MESSAGE_NOT_FOUND;
            }
        }

        xapi_fin_response(pXapireqe,
                          (char*) pQuery_Response,     
                          qrequestResponseSize);  
    }

    return STATUS_SUCCESS;
}



