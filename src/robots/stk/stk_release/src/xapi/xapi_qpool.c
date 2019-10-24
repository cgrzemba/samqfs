/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qpool.c                                     */
/** Description:    XAPI QUERY POOL processor.                       */
/**                                                                  */
/**                 Return scratch subpool attributes.               */
/**                                                                  */
/**                 NOTE: The XAPI QUERY POOL request is             */
/**                 processed locally by returning information       */
/**                 maintained in the XAPISCRPOOL table.             */
/**                                                                  */
/**                 NOTE: QUERY POOL is essentially the same         */
/**                 as QUERY SUBPOOL except that a different subset  */
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
#define QPOLERR_RESPONSE_SIZE (offsetof(QUERY_RESPONSE, status_response)) + \
                              (offsetof(QU_POL_RESPONSE, pool_status[0])) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_pool                                         */
/** Description:   XAPI QUERY POOL processor.                        */
/**                                                                  */
/** Return scratch subpool attributes and status from the            */
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
/** The QUERY POOL command is allowed to proceed even                */
/** when the XAPI client is in the IDLE state.                       */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY POOL request consists of:                       */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY_REQUEST data consisting of                            */
/**      i.   type                                                   */
/** 3.   QU_POL_CRITERIA data consisting of:                         */
/**      i.   count (of POOL(s) requested or 0 for "all")            */
/**      ii.  POOL[count] pool data entries                          */
/**                                                                  */
/** NOTE: MAX_ID is 42.  So a max subset of 42 pool(s) can be        */
/** requested at once.  If 0 pool(s) are specified, then all         */
/** pools(s) are returned in possible multiple intermediate          */
/** responses.                                                       */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY POOL response consists of:                      */
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
/**      ii.  QU_POL_RESPONSE data consisting of:                    */
/**           a.   count                                             */
/**           b.   QU_POL_STATUS[count] data entries consisting of:  */
/**                1.   POOLID                                       */
/**                2.   volume_count                                 */
/**                3.   low_water_mark                               */
/**                4.   high_water_mark                              */
/**                5.   pool_attributes (OVERFLOW only)              */
/**                6.   status                                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qpool"

extern int xapi_qpool(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 qpolResponseSize;
    int                 scrpoolIndex;
    int                 i;
    int                 j;
    int                 polRemainingCount;
    int                 polResponseCount;
    time_t              currTime;
    short               subpoolIndex;

    struct XAPISCRPOOL  wkXapiscrpool[MAX_XAPISCRPOOL];
    struct XAPISCRPOOL *pWkXapiscrpool;
    struct XAPISCRPOOL *pXapiscrpool;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_POL_CRITERIA    *pQu_Pol_Criteria    = &(pQuery_Request->select_criteria.pool_criteria);

    int                 polCount            = pQu_Pol_Criteria->pool_count;

    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;
    QU_POL_RESPONSE    *pQu_Pol_Response    = &(pQuery_Response->status_response.pool_response);
    QU_POL_STATUS      *pQu_Pol_Status      = &(pQu_Pol_Response->pool_status[0]);

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         sizeof(QU_POL_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY POOL request=%08X, size=%d, "
           "polCount=%d, MAX_ID=%d, "
           "QUERY POOL response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           polCount,
           MAX_ID,
           pQuery_Response,
           sizeof(QU_POL_RESPONSE));

    if (pXapicvt->scrpoolTime == 0)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QPOLERR_RESPONSE_SIZE,
                          STATUS_NI_FAILURE);

        return STATUS_NI_FAILURE;
    }

    if (polCount < 0)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QPOLERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_SMALL);

        return STATUS_COUNT_TOO_SMALL;
    }

    if (polCount > MAX_ID)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QPOLERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_LARGE);

        return STATUS_COUNT_TOO_LARGE;
    }

    qpolResponseSize = (char*) pQu_Pol_Response -
                       (char*) pQuery_Response + 
                       sizeof(QU_POL_RESPONSE);

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

    /*****************************************************************/
    /* Build a work list of the pools that will be returned.         */
    /*                                                               */
    /* If input count > 0, then return all pool(s) specified         */
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

    if (polCount > 0)
    {
        polRemainingCount = polCount;

        for (i = 0;
            i < polCount;
            i++)
        {
            pWkXapiscrpool = &(wkXapiscrpool[i]);

            subpoolIndex = pQu_Pol_Criteria->pool_id[i].pool;

            TRMSGI(TRCI_XAPI,
                   "Next input subpoolIndex=%d\n",
                   subpoolIndex);

            pXapiscrpool = xapi_scrpool_search_index(pXapicvt,
                                                     pXapireqe,
                                                     subpoolIndex);

            if (pXapiscrpool == NULL)
            {
                pWkXapiscrpool->subpoolIndex = subpoolIndex;
                pWkXapiscrpool->subpoolNameString[0] = 0;
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
        for (i = 1, polRemainingCount = 0;
            i < MAX_XAPISCRPOOL; 
            i++)
        {
            pXapiscrpool = &(pXapicvt->xapiscrpool[i]);

            if (pXapiscrpool->subpoolNameString[0] > 0)
            {
                pWkXapiscrpool = &(wkXapiscrpool[polRemainingCount]);

                memcpy((char*) pWkXapiscrpool,
                       (char*) pXapiscrpool,
                       sizeof(struct XAPISCRPOOL));

                polRemainingCount++;
            }
        }
    }

    TRMSGI(TRCI_XAPI,
           "polCount=%d, polRemainingCount=%d\n",
           polCount,
           polRemainingCount);

    /*****************************************************************/
    /* Build the ACSAPI response(s).                                 */
    /*****************************************************************/
    scrpoolIndex = 0;

    while (1)
    {
        if (polRemainingCount > MAX_ID)
        {
            polResponseCount = MAX_ID;
        }
        else
        {
            polResponseCount = polRemainingCount;
        }

        qpolResponseSize = (char*) pQu_Pol_Status -
                           (char*) pQuery_Response +
                           ((sizeof(QU_POL_STATUS)) * polResponseCount);

        xapi_query_init_resp(pXapireqe,
                             (char*) pQuery_Response,
                             qpolResponseSize);

        pQu_Pol_Status = 
        (QU_POL_STATUS*) &(pQu_Pol_Response->pool_status[0]);

        pQu_Pol_Response->pool_count = polResponseCount;

        TRMSGI(TRCI_XAPI,
               "At top of while; polRemaining=%d, polResponse=%d, "
               "scrpoolIndex=%d, MAX_ID=%d\n",
               polRemainingCount,
               polResponseCount,
               scrpoolIndex,
               MAX_ID);

        for (i = scrpoolIndex, j = 0;
            i < (polRemainingCount - scrpoolIndex);
            i++)
        {
            pWkXapiscrpool = &(wkXapiscrpool[i]);

            pQu_Pol_Status->pool_id.pool = 
            (POOL) pWkXapiscrpool->subpoolIndex; 

            if (pWkXapiscrpool->subpoolNameString[0] == 0)
            {
                pQu_Pol_Status->status = STATUS_POOL_NOT_FOUND;
            }
            else
            {
                pQu_Pol_Status->status = STATUS_SUCCESS;
                pQu_Pol_Status->volume_count = pWkXapiscrpool->volumeCount;
                pQu_Pol_Status->low_water_mark = pWkXapiscrpool->threshold;
                pQu_Pol_Status->pool_attributes = OVERFLOW;
            }

            pQu_Pol_Status++;             
            j++;

            if (j >= polResponseCount)
            {
                polRemainingCount -= polResponseCount;

                break; 
            }
        }

        if (polRemainingCount > 0)
        {
            xapi_int_response(pXapireqe,
                              (char*) pQuery_Response,
                              qpolResponseSize);
        }
        else
        {
            xapi_fin_response(pXapireqe,
                              (char*) pQuery_Response,
                              qpolResponseSize);

            break;
        }
    }

    return queryRC;
}


