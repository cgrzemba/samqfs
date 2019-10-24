/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qsubpool.c                                  */
/** Description:    XAPI QUERY SUBPOOL NAME processor.               */
/**                                                                  */
/**                 Return scratch subpool name information.         */
/**                                                                  */
/**                 NOTE: The XAPI QUERY SUBPOOL NAME request is     */
/**                 processed locally by returning information       */
/**                 maintained in the XAPISCRPOOL table.             */
/**                                                                  */
/**                 NOTE: QUERY SUBPOOL is essentially the same      */
/**                 as QUERY POOL except that a different subset     */
/**                 of XAPISCRPOOL data is returned.                 */
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
#define INDEX_NOT_FOUND        -1
#define QSPNERR_RESPONSE_SIZE (offsetof(QUERY_RESPONSE, status_response)) + \
                              (offsetof(QU_SPN_RESPONSE, subpl_name_status[0]))

/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qsubpool                                     */
/** Description:   XAPI QUERY SUBPOOL NAME processor.                */
/**                                                                  */
/** Return scratch subpool names and status from the                 */
/** XAPI client XAPISCRPOOL table for the specified scratch          */
/** subpools (or for all defined scratch subpools when the           */
/** request subpool count is 0).                                     */
/**                                                                  */
/** The XAPISCRPOOL table is a conversion table to convert XAPI      */
/** scratch subpool names into ACSAPI scratch subpool number         */
/** (and vice versa).  In addition the XAPISCRPOOL table             */
/** also contains gross scratch subpool volume counts.               */
/**                                                                  */
/** The XAPISCRPOOL table is populated by xapi_main when the         */
/** XAPI client is initialized.  The gross scratch subpool           */
/** volume counts are also updated periodically by xapi_main.        */
/**                                                                  */
/** The QUERY SUBPOOL NAME command is allowed to proceed even        */
/** when the XAPI client is in the IDLE state.                       */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY SUBPOOL NAME request consists of:               */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY_REQUEST data consisting of                            */
/**      i.   type                                                   */
/** 3.   QU_SPN_CRITERIA data consisting of:                         */
/**      i.   count (of subpools(s) requested or 0 for "all")        */
/**      ii.  SUBPOOL_NAME[count] subpool data entries               */
/**           consisting of:                                         */
/**           a.   subpool_name (string)                             */
/**                                                                  */
/** NOTE: MAX_SPN is 20.  So a max subset of 20 subpool(s) can be    */
/** requested at once.  If 0 subpool(s) are specified, then all      */
/** subpool(s) are returned in possible multiple intermediate        */
/** responses.                                                       */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY SUBPOOL NAME response consists of:              */
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
/**      ii.  QU_SPN_RESPONSE data consisting of:                    */
/**           a.   count                                             */
/**           b.   QU_SUBPOOL_NAME_STATUS[count] data                */
/**                entries consisting of:                            */
/**                1.   SUBPOOL_NAME consisting of:                  */
/**                     i.   subpool_name (string)                   */
/**                2.   POOLID                                       */
/**                3.   STATUS                                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qsubpool"

extern int xapi_qsubpool(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 qspnResponseSize;
    int                 scrpoolIndex;
    int                 i;
    int                 j;
    int                 spnRemainingCount;
    int                 spnResponseCount;
    time_t              currTime;
    char                subpoolNameString[XAPI_SUBPOOL_NAME_SIZE + 1];

    struct XAPISCRPOOL  wkXapiscrpool[MAX_XAPISCRPOOL];
    struct XAPISCRPOOL *pWkXapiscrpool;
    struct XAPISCRPOOL *pXapiscrpool;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_SPN_CRITERIA    *pQu_Spn_Criteria    = 
    &(pQuery_Request->select_criteria.subpl_name_criteria);

    int                 spnCount            = pQu_Spn_Criteria->spn_count;

    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;

    QU_SPN_RESPONSE    *pQu_Spn_Response    = 
    &(pQuery_Response->status_response.subpl_name_response);

    QU_SUBPOOL_NAME_STATUS *pQu_Subpool_Name_Status = 
    &(pQu_Spn_Response->subpl_name_status[0]);

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         sizeof(QU_SPN_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY SUBPOOL NAME request=%08X, size=%d, "
           "count=%d, MAX_SPN=%d, MAX_ID=%d, "
           "QUERY SUBPOOL NAME response=%08X ,size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           spnCount,
           MAX_SPN,
           MAX_ID,
           pQuery_Response,
           qspnResponseSize);

    if (pXapicvt->scrpoolTime == 0)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QSPNERR_RESPONSE_SIZE,
                          STATUS_NI_FAILURE);

        return STATUS_NI_FAILURE;
    }

    if (spnCount < 0)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QSPNERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_SMALL);

        return STATUS_COUNT_TOO_SMALL;
    }

    if (spnCount > MAX_SPN)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QSPNERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_LARGE);

        return STATUS_COUNT_TOO_LARGE;
    }

    qspnResponseSize = (char*) pQu_Spn_Response -
                       (char*) pQuery_Response +
                       (sizeof(QU_SPN_RESPONSE));

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

    /*****************************************************************/
    /* Build a work list of the subpool(s) that will be returned.    */
    /*                                                               */
    /* If input count > 0, then return all subpool(s) specified      */
    /* in the request.                                               */
    /*                                                               */
    /* If input count is 0, then return all named subpool(s) in      */
    /* the XAPISCRPOOL table excluding the DEFAULTPOOL (which is     */
    /* POOL or subpool index 0).                                     */
    /*****************************************************************/
    time(&currTime);

    TRMSGI(TRCI_XAPI,
           "XAPISCRPOOL age=%d\n",
           (currTime - pXapicvt->scrpoolTime));

    memset((char*) &wkXapiscrpool, 0, sizeof(wkXapiscrpool));

    if (spnCount > 0)
    {
        spnRemainingCount = spnCount;

        for (i = 0;
            i < spnCount;
            i++)
        {
            pWkXapiscrpool = &(wkXapiscrpool[i]);

            strcpy(subpoolNameString, 
                   pQu_Spn_Criteria->subpl_name[i].subpool_name);

            TRMSGI(TRCI_XAPI,
                   "Next input subpool=%s\n",
                   subpoolNameString);

            pXapiscrpool = xapi_scrpool_search_name(pXapicvt,
                                                    pXapireqe,
                                                    subpoolNameString);

            if (pXapiscrpool == NULL)
            {
                pWkXapiscrpool->subpoolIndex = INDEX_NOT_FOUND;
                strcpy(pWkXapiscrpool->subpoolNameString, subpoolNameString);
            }
            else
            {
                memcpy((char*) pWkXapiscrpool,
                       (char*) pXapiscrpool,
                       sizeof(struct XAPISCRPOOL));
            }
        }
    }
    else
    {
        for (i = 1, spnRemainingCount = 0;
            i < MAX_XAPISCRPOOL; 
            i++)
        {
            pXapiscrpool = &(pXapicvt->xapiscrpool[i]);

            if (pXapiscrpool->subpoolNameString[0] > 0)
            {
                pWkXapiscrpool = &(wkXapiscrpool[spnRemainingCount]);

                memcpy((char*) pWkXapiscrpool,
                       (char*) pXapiscrpool,
                       sizeof(struct XAPISCRPOOL));

                spnRemainingCount++;
            }
        }
    }

    TRMSGI(TRCI_XAPI,
           "spnCount=%d, spnRemainingCount=%d\n",
           spnCount,
           spnRemainingCount);

    /*****************************************************************/
    /* Build the ACSAPI response(s).                                 */
    /*****************************************************************/
    scrpoolIndex = 0;

    while (1)
    {
        if (spnRemainingCount > MAX_ID)
        {
            spnResponseCount = MAX_ID;
        }
        else
        {
            spnResponseCount = spnRemainingCount;
        }

        qspnResponseSize = (char*) pQu_Subpool_Name_Status -
                           (char*) pQuery_Response +
                           ((sizeof(QU_SUBPOOL_NAME_STATUS)) * spnResponseCount);

        xapi_query_init_resp(pXapireqe,
                             (char*) pQuery_Response,
                             qspnResponseSize);

        pQu_Subpool_Name_Status = 
        (QU_SUBPOOL_NAME_STATUS*) &(pQu_Spn_Response->subpl_name_status[0]);

        pQu_Spn_Response->spn_status_count = spnResponseCount;

        TRMSGI(TRCI_XAPI,
               "At top of while; spnRemaining=%d, spnResponse=%d, "
               "scrpoolIndex=%d, MAX_ID=%d\n",
               spnRemainingCount,
               spnResponseCount,
               scrpoolIndex,
               MAX_ID);

        for (i = scrpoolIndex, j = 0;
            i < (spnRemainingCount - scrpoolIndex);
            i++)
        {
            pWkXapiscrpool = &(wkXapiscrpool[i]);

            strcpy(pQu_Subpool_Name_Status->subpool_name.subpool_name,
                   pWkXapiscrpool->subpoolNameString);

            if (pWkXapiscrpool->subpoolIndex == INDEX_NOT_FOUND)
            {
                pQu_Subpool_Name_Status->status = STATUS_POOL_NOT_FOUND;
            }
            else
            {
                pQu_Subpool_Name_Status->status = STATUS_SUCCESS;
                pQu_Subpool_Name_Status->pool_id.pool = 
                (POOL) pWkXapiscrpool->subpoolIndex; 
            }

            pQu_Subpool_Name_Status++;
            j++;

            if (j >= spnResponseCount)
            {
                spnRemainingCount -= spnResponseCount;

                break; 
            }
        }

        if (spnRemainingCount > 0)
        {
            xapi_int_response(pXapireqe,
                              (char*) pQuery_Response,
                              qspnResponseSize);
        }
        else
        {
            xapi_fin_response(pXapireqe,
                              (char*) pQuery_Response,
                              qspnResponseSize);

            break;
        }
    }

    return queryRC;
}



