/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_define_pool.c                               */
/** Description:    XAPI DEFINE POOL processor.                      */
/**                                                                  */
/**                 Define a scratch subpool by subpool number.      */
/**                                                                  */
/**                 NOTE: The XAPI does not support a                */
/**                 DEFINE POOL request.  However, the               */
/**                 ACSAPI DEFINE POOL request is processed          */
/**                 as if it were an ACSAPI QUERY POOL request.      */
/**                                                                  */
/**                 The ACSAPI QUERY POOL request is                 */
/**                 processed locally by returning information       */
/**                 maintained in the XAPISCRPOOL table.             */
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
#define DEFINEERR_RESPONSE_SIZE (offsetof(DEFINE_POOL_RESPONSE, pool_status[0])) 


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void initDefineResponse(struct XAPIREQE *pXapireqe,
                               char            *pDefineResponse,
                               int              defineResponseSize);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_define_pool                                  */
/** Description:   XAPI DEFINE POOL processor.                       */
/**                                                                  */
/** Define a scratch subpool by subpool number.                      */
/**                                                                  */
/** Scratch subpools are maintained by the server.  Therefore XAPI   */
/** cannot support the DEFINE POOL command.  However, the XAPI       */
/** client does maintain a table of scratch subpools and attributes  */
/** obtained from the server.  This table is the XAPISCRPOOL.        */
/**                                                                  */
/** The DEFINE POOL request is processed as if it were a QUERY POOL  */
/** request; If the subpool number (i.e. subpool index) specified    */
/** in the DEFINE POOL command already exists in the XAPISCRPOOL     */ 
/** table, then the DEFINE POOL command will return STATUS_SUCCESS;  */
/** However if the subpool number specified in the DEFINE POOL       */
/** command does not exists in the XAPISCRPOOL table, then the       */ 
/** DEFINE POOL command will return STATUS_INVALID_POOL.             */
/**                                                                  */
/** The DEFINE POOL command is NOT allowed to proceed when the       */
/** XAPI client is in the IDLE state.                                */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI DEFINE POOL request consists of:                      */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   DEFINE POOL data consisting of                              */
/**      i.   low_water_mark (ignored for the XAPI)                  */
/**      ii.  high_water_mark (ignored for the XAPI)                 */
/**      iii. pool_attributes (OVERFLOW is the only value allowed)   */
/**      iv.  count (of POOL(s) to define (in the XAPI, this is      */
/**           the count of POOLs to query)                           */
/**      v.   POOL[count] pool data entries                          */
/**                                                                  */
/** NOTE: MAX_ID is 42.  So a max subset of 42 pool(s) can be        */
/** requested at once.  If 0 pool(s) are specified, then all         */
/** pools(s) are returned in possible multiple intermediate          */
/** responses.                                                       */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI DEFINE POOL response consists of:                     */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   DEFINE POOL data consisting of:                             */
/**      i.   low_water_mark (from the request)                      */
/**      ii.  high_water_mark (from the request)                     */
/**      iii. pool_attributes (from the request)                     */
/**      iv.  count (of POOL(s) returned)                            */
/**      v.   pool_status[count] entries consisting of:              */
/**           a.   POOLID consisting of.                             */
/**                1.   POOL (subpool index)                         */
/**           b.   RESPONSE_STATUS consisting of:                    */
/**                1.   status                                       */
/**                2.   type                                         */
/**                3.   identifier                                   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_define_pool"

extern int xapi_define_pool(struct XAPICVT  *pXapicvt,
                            struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    int                 defineRC             = STATUS_SUCCESS;
    int                 defineResponseSize;
    int                 scrpoolIndex;
    int                 i;
    time_t              currTime;
    short               subpoolIndex;

    struct XAPISCRPOOL  wkXapiscrpool[MAX_XAPISCRPOOL];
    struct XAPISCRPOOL *pWkXapiscrpool;
    struct XAPISCRPOOL *pXapiscrpool;

    DEFINE_POOL_REQUEST  *pDefine_Pool_Request = 
    (DEFINE_POOL_REQUEST*) pXapireqe->pAcsapiBuffer;

    int                 poolCount            = pDefine_Pool_Request->count;

    DEFINE_POOL_RESPONSE  wkDefine_Pool_Response;
    DEFINE_POOL_RESPONSE *pDefine_Pool_Response = &wkDefine_Pool_Response;

    initDefineResponse(pXapireqe,
                       (char*) pDefine_Pool_Response,
                       sizeof(DEFINE_POOL_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; DEFINE POOL request=%08X, size=%d, "
           "poolCount=%d, MAX_ID=%d, "
           "DEFINE POOL response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           poolCount,
           MAX_ID,
           pDefine_Pool_Response,
           sizeof(DEFINE_POOL_RESPONSE));

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pDefine_Pool_Response,
                            DEFINEERR_RESPONSE_SIZE);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    if (pXapicvt->scrpoolTime == 0)
    {
        xapi_err_response(pXapireqe,
                          (char*) pDefine_Pool_Response,
                          DEFINEERR_RESPONSE_SIZE,
                          STATUS_NI_FAILURE);

        return STATUS_NI_FAILURE;
    }

    if (poolCount < 1)
    {
        xapi_err_response(pXapireqe,
                          (char*) pDefine_Pool_Response,
                          DEFINEERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_SMALL);

        return STATUS_COUNT_TOO_SMALL;
    }

    if (poolCount > MAX_ID)
    {
        xapi_err_response(pXapireqe,
                          (char*) pDefine_Pool_Response,
                          DEFINEERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_LARGE);

        return STATUS_COUNT_TOO_LARGE;
    }

    defineResponseSize = (offsetof(DEFINE_POOL_RESPONSE, count)) +
                         (sizeof(pDefine_Pool_Response->count)) +  
                         (sizeof(pDefine_Pool_Response->pool_status[0]) * poolCount);

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

    /*****************************************************************/
    /* Build a work list of the pools that will be returned.         */
    /* For a DEFINE POOL request return all pool(s) specified        */
    /* in the request.                                               */
    /*****************************************************************/
    time(&currTime);

    TRMSGI(TRCI_XAPI,
           "XAPISCRPOOL age=%d\n",
           (currTime - pXapicvt->scrpoolTime));

    memset((char*) &wkXapiscrpool, 0, sizeof(wkXapiscrpool));

    for (i = 0;
        i < poolCount;
        i++)
    {
        pWkXapiscrpool = &(wkXapiscrpool[i]);

        subpoolIndex = pDefine_Pool_Request->pool_id[i].pool;

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

    /*****************************************************************/
    /* Build the ACSAPI response(s).                                 */
    /*****************************************************************/
    pDefine_Pool_Response->low_water_mark = pDefine_Pool_Request->low_water_mark;
    pDefine_Pool_Response->high_water_mark = pDefine_Pool_Request->high_water_mark;
    pDefine_Pool_Response->pool_attributes = pDefine_Pool_Request->pool_attributes;
    pDefine_Pool_Response->count = pDefine_Pool_Request->count;

    for (i = 0;
        i < poolCount;
        i++)
    {
        pWkXapiscrpool = &(wkXapiscrpool[i]);

        pDefine_Pool_Response->pool_status[i].pool_id.pool = 
        (POOL) pWkXapiscrpool->subpoolIndex; 

        /*************************************************************/
        /* If the subpool is not defined, then return a              */
        /* STATUS_INVALID_POOL for the not-found subpool.            */
        /*************************************************************/
        if (pWkXapiscrpool->subpoolNameString[0] == 0)
        {
            pDefine_Pool_Response->pool_status[i].status.status = 
            STATUS_INVALID_POOL;
        }
        else
        {
            pDefine_Pool_Response->pool_status[i].status.status = STATUS_SUCCESS;
        }
    }

    xapi_fin_response(pXapireqe,
                      (char*) pDefine_Pool_Response,
                      defineResponseSize);


    return defineRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: initDefineResponse                                */
/** Description:   Initialize the ACSAPI DEFINE POOL response.       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "initDefineResponse"

static void initDefineResponse(struct XAPIREQE *pXapireqe,
                               char            *pDefineResponse,
                               int              defineResponseSize)
{
    DEFINE_POOL_REQUEST  *pDefine_Pool_Request = 
    (DEFINE_POOL_REQUEST*) pXapireqe->pAcsapiBuffer;
    DEFINE_POOL_RESPONSE *pDefine_Pool_Response = 
    (DEFINE_POOL_RESPONSE*) pDefineResponse;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Initialize DEFINE POOL response.                              */
    /*****************************************************************/
    memset((char*) pDefine_Pool_Response, 0, defineResponseSize);

    memcpy((char*) &(pDefine_Pool_Response->request_header),
           (char*) &(pDefine_Pool_Request->request_header),
           sizeof(REQUEST_HEADER));

    pDefine_Pool_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pDefine_Pool_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pDefine_Pool_Response->message_status.status = STATUS_SUCCESS;
    pDefine_Pool_Response->message_status.type = TYPE_NONE;

    return;
}




