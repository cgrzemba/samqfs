/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qscr.c                                      */
/** Description:    XAPI QUERY SCRATCH processor.                    */
/**                                                                  */
/**                 Return a list of all volumes in scratch          */
/**                 status for the specified scratch subpool(s).     */
/**                                                                  */
/**                 NOTE: Unlike the HSC DISPLAY SCRATCH command     */
/**                 which lists scratch subpool totals, the          */
/**                 ACSAPI QUERY SCRATCH command returns a           */
/**                 list of all scratch volumes in the specified     */
/**                 POOL(s) (by subpool index), or a list of         */
/**                 all scratch volumes in all subpools.             */
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
#define INDEX_NOT_FOUND            -1
#define QUERY_SEND_TIMEOUT         5   /* TIMEOUT values in seconds  */       
#define QUERY_RECV_TIMEOUT_1ST     600
#define QUERY_RECV_TIMEOUT_NON1ST  900
#define QSCRERR_RESPONSE_SIZE (offsetof(QUERY_RESPONSE, status_response)) + \
                              (offsetof(QU_SCR_RESPONSE, scratch_status[0])) 


/*********************************************************************/
/* Structures:                                                       */
/*********************************************************************/

/*********************************************************************/
/* QSCRCSV: The following maps the fixed UUITEXT lines returned in   */
/* the CSV response (SS...SS,NNN,VVVVV,MMMMMMMM,AA:LL:PP:RR:CC).     */
/*********************************************************************/
struct QSCRCSV                
{
    char                subpoolName[13]; 
    char                _f0;           /* comma                      */
    char                subpoolIndex[3]; 
    char                _f1;           /* comma                      */
    char                volser[6];
    char                _f2;           /* comma                      */
    char                mediaName[8];
    char                _f3;           /* comma                      */
    char                cellAcs[2];
    char                _f4;           /* colon                      */
    char                cellLsm[2];
    char                _f5;           /* colon                      */
    char                cellPanel[2];
    char                _f6;           /* colon                      */
    char                cellRow[2];
    char                _f7;           /* colon                      */
    char                cellColumn[2];
};


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int queryScrpoolInfoOne(struct XAPICVT     *pXapicvt,
                               struct XAPIREQE    *pXapireqe,
                               struct XAPISCRPOOL *pXapiscrpool,
                               char               *pQscratchResponse,
                               char                finalSubpoolFlag);

static void createQscrpoolRequest(struct XAPICVT     *pXapicvt,
                                  struct XAPIREQE    *pXapireqe,
                                  struct XAPISCRPOOL *pXapiscrpool,
                                  char              **ptrXapiBuffer,
                                  int                *pXapiBufferSize);

static int extractQscrpoolResponse(struct XAPICVT     *pXapicvt,
                                   struct XAPIREQE    *pXapireqe,
                                   char               *pQscratchResponse,
                                   struct XAPISCRPOOL *pXapiscrpool,
                                   char                finalSubpoolFlag,
                                   struct XMLPARSE    *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qscr                                         */
/** Description:   XAPI QUERY SCRATCH processor.                     */
/**                                                                  */
/** Return a list of all volumes in scratch status for the           */
/** specified scratch subpool(s) (or return a list of all            */
/** volumes in scratch status for all defined scratch subpools       */
/** if the request subpool count is 0).                              */
/**                                                                  */
/** The QUERY SCRATCH request is processed by issuing an             */
/** XAPI XML format <query_scrpool_info> request that specifies      */
/** <detail_request>YES for each scratch subpool specified in        */
/** the request (or for all scratch subpools if the request          */
/** subpool count is 0).  There may be multiple individual           */
/** XAPI <query_scrpool_info> requests issued for a single           */
/** ACSAPI QUERY SCRATCH request.                                    */
/**                                                                  */
/** The QUERY SCRATCH command is allowed to proceed even when        */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY SCRATCH request maps identically to the         */
/** ACSAPI QUERY POOL request (there is no separate QU_POL_CRITERIA  */
/** structure, so we use the QU_POL_CRITERIA for mapping the input   */
/** request).                                                        */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY_REQUEST data consisting of                            */
/**      i.   type                                                   */
/** 3.   QU_POL_CRITERIA data consisting of:                         */
/**      i.   count (of pools(s) requested or 0 for "all")           */
/**      ii.  POOL[count] pool data entries                          */
/**                                                                  */
/** NOTE: MAX_ID is 42.  So a max subset of 42 pool(s) can be        */
/** requested at once.  If 0 pool(s) are specified, then all         */
/** pools(s) are returned in possible multiple intermediate          */
/** responses.                                                       */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY SCRATCH response consists of:                   */
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
/**      ii.  QU_SCR_RESPONSE data consisting of:                    */
/**           a.   count                                             */
/**           b.   QU_SCR_STATUS[count] data                         */
/**                entries consisting of:                            */
/**                1.   VOLID consisting of                          */
/**                     i.   external_label (6 character volser)     */
/**                2.   MEDIA_TYPE (ACSAPI media code)               */
/**                3.   POOLID                                       */
/**                4.   CELLID consisting of:                        */
/**                     ii.   panel_id.lsmid.acs                     */
/**                     ii.   panel_id.lsmid.lsm                     */
/**                     iii.  panel_id.panel.row                     */
/**                     iv.   panel_id.panel.col                     */
/**                     v.    column                                 */
/**                5.   STATUS                                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qscr"

extern int xapi_qscr(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe)
{
    int                 queryRC          = STATUS_SUCCESS;
    int                 lastRC;
    int                 qscrResponseSize;
    int                 i;
    int                 namedPoolCount;    
    time_t              currTime;
    short               subpoolIndex;
    char                finalSubpoolFlag    = FALSE;

    struct XAPISCRPOOL  wkXapiscrpool[MAX_XAPISCRPOOL];
    struct XAPISCRPOOL *pWkXapiscrpool;
    struct XAPISCRPOOL *pXapiscrpool;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_POL_CRITERIA    *pQu_Pol_Criteria    = &(pQuery_Request->select_criteria.pool_criteria);

    int                 reqPoolCount        = pQu_Pol_Criteria->pool_count;

    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;
    QU_SCR_RESPONSE    *pQu_Scr_Response    = &(pQuery_Response->status_response.scratch_response);
    QU_SCR_STATUS      *pQu_Scr_Status      = &(pQu_Scr_Response->scratch_status[0]);

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         sizeof(QU_SCR_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY SCRATCH request=%08X, size=%d, "
           "count=%d, MAX_ID=%d, "
           "QUERY POOL response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           reqPoolCount,
           MAX_ID,
           pQuery_Response,
           sizeof(QU_SCR_RESPONSE));

    if (pXapicvt->scrpoolTime == 0)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QSCRERR_RESPONSE_SIZE,
                          STATUS_NI_FAILURE);

        return STATUS_NI_FAILURE;
    }

    if (reqPoolCount < 0)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QSCRERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_SMALL);

        return STATUS_COUNT_TOO_SMALL;
    }

    if (reqPoolCount > MAX_ID)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QSCRERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_LARGE);

        return STATUS_COUNT_TOO_LARGE;
    }

    qscrResponseSize = (char*) pQu_Scr_Response -
                       (char*) pQuery_Response +
                       (MAX_ID * sizeof(QU_SCR_STATUS));

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

    if (reqPoolCount > 0)
    {
        for (i = 0, namedPoolCount = 0;
            i < reqPoolCount;
            i++)
        {
            pWkXapiscrpool = &(wkXapiscrpool[namedPoolCount]);

            subpoolIndex = (short) pQu_Pol_Criteria->pool_id[i].pool;

            TRMSGI(TRCI_XAPI,
                   "Next input subpoolIndex=%d\n",
                   subpoolIndex);

            pXapiscrpool = xapi_scrpool_search_index(pXapicvt,
                                                     pXapireqe,
                                                     subpoolIndex);

            if (pXapiscrpool != NULL)
            {
                memcpy((char*) pWkXapiscrpool,
                       (char*) pXapiscrpool,
                       sizeof(struct XAPISCRPOOL));

                namedPoolCount++;
            }
        }
    }
    else
    {
        for (i = 1, namedPoolCount = 0;
            i < MAX_XAPISCRPOOL; 
            i++)
        {
            pXapiscrpool = &(pXapicvt->xapiscrpool[i]);

            if (pXapiscrpool->subpoolNameString[0] > 0)
            {
                pWkXapiscrpool = &(wkXapiscrpool[namedPoolCount]);

                memcpy((char*) pWkXapiscrpool,
                       (char*) pXapiscrpool,
                       sizeof(struct XAPISCRPOOL));

                namedPoolCount++;
            }
        }
    }

    TRMSGI(TRCI_XAPI,
           "reqPoolCount=%d, namedPoolCount=%d\n",
           reqPoolCount,
           namedPoolCount);

    /*****************************************************************/
    /* Once the scratch subpool list is built, then execute an XAPI  */
    /* <query_scrpool_info><detail_request>YES request for           */
    /* each scratch subpool in the list.                             */
    /*****************************************************************/
    for (i = 0;
        i < namedPoolCount;
        i++)
    {
        pWkXapiscrpool = &(wkXapiscrpool[i]);

        if ((i + 1) >= namedPoolCount)
        {
            finalSubpoolFlag = TRUE;
        }

        lastRC = queryScrpoolInfoOne(pXapicvt,
                                     pXapireqe,
                                     pWkXapiscrpool,
                                     (char*) pQuery_Response,
                                     finalSubpoolFlag);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from queryScrpoolInfoOne\n",
               lastRC);
    }

    /*****************************************************************/
    /* When done, issue the final response.                          */
    /*****************************************************************/
    if (queryRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QSCRERR_RESPONSE_SIZE ,
                          queryRC);
    }
    else
    {
        qscrResponseSize = (char*) pQu_Scr_Status -
                           (char*) pQuery_Response +
                           (pQu_Scr_Response->volume_count * sizeof(QU_SCR_STATUS));

        xapi_fin_response(pXapireqe,
                          (char*) pQuery_Response,     
                          qscrResponseSize);
    }

    return queryRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: queryScrpoolInfoOne                               */
/** Description:   Execute an XAPI <query_scrpool_info> request.     */
/**                                                                  */
/** Execute an XAPI <query_scrpool_info><detail_request>YES request  */
/** for the specified subpool.                                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "queryScrpoolInfoOne"

static int queryScrpoolInfoOne(struct XAPICVT     *pXapicvt,
                               struct XAPIREQE    *pXapireqe,
                               struct XAPISCRPOOL *pXapiscrpool,
                               char               *pQscratchResponse,
                               char                finalSubpoolFlag)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    QUERY_RESPONSE     *pQuery_Response     = (QUERY_RESPONSE*) pQscratchResponse;
    QU_SCR_RESPONSE    *pQu_Scr_Response    = (QU_SCR_RESPONSE*) &(pQuery_Response->status_response);
    QU_SCR_STATUS      *pQu_Scr_Status      = (QU_SCR_STATUS*) &(pQu_Scr_Response->scratch_status[0]);

    int                 currCount           = pQu_Scr_Response->volume_count;

    TRMSGI(TRCI_XAPI,
           "Entered; currCount=%d, MAX_ID=%d, "
           "QUERY SCRATCH response=%08X, subpoolNameString=%s, "
           "finalSubpoolFlag=%d\n",
           currCount,
           MAX_ID,
           pQuery_Response,
           pXapiscrpool->subpoolNameString,
           finalSubpoolFlag);

    createQscrpoolRequest(pXapicvt,
                          pXapireqe,
                          pXapiscrpool,
                          &pXapiBuffer,
                          &xapiBufferSize);

    lastRC = xapi_tcp(pXapicvt,
                      pXapireqe,
                      pXapiBuffer,
                      xapiBufferSize,
                      QUERY_SEND_TIMEOUT,        
                      QUERY_RECV_TIMEOUT_1ST,    
                      QUERY_RECV_TIMEOUT_NON1ST, 
                      (void**) &pXmlparse);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from xapi_tcp(pBuffer=%08X, size=%d, pXmlparse=%08X)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize,
           pXmlparse);

    if (lastRC != STATUS_SUCCESS)
    {
        queryRC = lastRC;
    }

    /*****************************************************************/
    /* At this point we can free the XAPI XML request string.        */
    /*****************************************************************/
    TRMSGI(TRCI_STORAGE,
           "free pXapiBuffer=%08X, len=%i\n",
           pXapiBuffer,
           xapiBufferSize);

    free(pXapiBuffer);

    /*****************************************************************/
    /* Now generate the QUERY SCRATCH ACSAPI response.                */
    /*****************************************************************/
    if (queryRC == STATUS_SUCCESS)
    {
        lastRC = extractQscrpoolResponse(pXapicvt,
                                         pXapireqe,
                                         (char*) pQuery_Response,
                                         pXapiscrpool,
                                         finalSubpoolFlag,
                                         pXmlparse);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractQscrpoolResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            queryRC = lastRC;
        }
    }

    /*****************************************************************/
    /* When done, free the XMLPARSE and associated structures.       */
    /*****************************************************************/
    if (pXmlparse != NULL)
    {
        FN_FREE_HIERARCHY_STORAGE(pXmlparse);
    }

    return queryRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: createQscrpoolRequest                             */
/** Description:   Build an XAPI <query_scrpool_info> request.       */
/**                                                                  */
/** Build an XAPI <query_scrpool_info><detail_request>YES request    */
/** to return all volumes in scratch status for the specified        */
/** scratch subpool.  Additionally, specify CSV fixed format         */
/** output to limit the size and parse complexity of the response.   */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_scrpool_info><detail_request>YES request     */
/** consists of:                                                     */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**                                                                  */
/**   <csv_break>volume_data<csv_break>                              */
/**   <csv_fields>subpool_name,subpool_index,                        */
/**     volume_data.volser,volume_data.media,volume_data.home_cell   */
/**     </csv_fields>                                                */
/**   <csv_fixed_flag>Y</csv_fixed_flag>                             */
/**   <csv_notitle_flag>Y</csv_notitle_flag>                         */
/**                                                                  */
/**   <command>                                                      */
/**     <query_scrpool_info>                                         */
/**       <subpool_name>sssssssssssss</subpool_name>                 */
/**       <detail_request>YES</detail_request>                       */
/**     </query_scrpool_info>                                        */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "createQscrpoolRequest"

static void createQscrpoolRequest(struct XAPICVT     *pXapicvt,
                                  struct XAPIREQE    *pXapireqe,
                                  struct XAPISCRPOOL *pXapiscrpool,
                                  char              **ptrXapiBuffer,
                                  int                *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;

    char               *pXapiRequest        = NULL;
    char               *pAcsapiBuffer       = pXapireqe->pAcsapiBuffer;

    struct XMLPARSE    *pXmlparse; 
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

    pXmlparse = FN_CREATE_XMLPARSE();

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      NULL,
                                      XNAME_libtrans,
                                      NULL,
                                      0);

    xapi_request_header(pXapicvt,
                        pXapireqe,
                        NULL,
                        (void*) pXmlparse,
                        FALSE,
                        FALSE,
                        XML_CASE_UPPER,
                        XML_DATE_STCK);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_break_tag,
                                      XNAME_volume_data,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_notitle_flag,
                                      XCONTENT_Y,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_fixed_flag,
                                      XCONTENT_Y,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_fields,
                                      "subpool_name,subpool_index,"
                                      "volume_data.volser,"
                                      "volume_data.media,"
                                      "volume_data.home_cell",
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_command,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_query_scrpool_info,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_detail_request,
                                      XCONTENT_YES,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_subpool_name,
                                      pXapiscrpool->subpoolNameString,
                                      0);

    pXmlrawin = FN_GENERATE_XML_FROM_HIERARCHY(pXmlparse);

    xapiRequestSize = (pXmlrawin->dataLen) + 1;

    pXapiRequest = (char*) malloc(xapiRequestSize);

    TRMSGI(TRCI_STORAGE,
           "malloc XAPI buffer=%08X, len=%i\n",
           pXapiRequest,
           xapiRequestSize);

    memcpy(pXapiRequest, 
           pXmlrawin->pData, 
           pXmlrawin->dataLen);

    pXapiRequest[pXmlrawin->dataLen] = 0;

    FN_FREE_HIERARCHY_STORAGE(pXmlparse);

    *ptrXapiBuffer = pXapiRequest;
    *pXapiBufferSize = xapiRequestSize;

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: extractQscrpoolResponse                           */
/** Description:   Extract the <query_scrpool_info> response.        */
/**                                                                  */
/** Parse the response of the XAPI <qry_scrpool_info>                */
/** <detail_request>YES response, and update the appropriate         */
/** fields of the ACSAPI QUERY SCRATCH response.  Each <uui_text>    */
/** line of the returned CSV will represent a scratch volume.        */
/**                                                                  */
/**==================================================================*/
/** The XAPI CSV <query_scrpool_info><detail_request>YES responses   */
/** consists of:                                                     */
/**==================================================================*/
/** <libreply>                                                       */
/**   <uui_line_type>V</uui_line_type>                               */
/**   <uui_text>SS..SS,NNN,VVVVVV,MMMMMMMM,AA:LL:PP:RR:CC    (FIXED) */
/**     </uui_text>                                                  */
/**   ...repeated <uui_line> and <uui_text> entries                  */
/**   <exceptions>                                                   */
/**     <reason>ccc...ccc</reason>                                   */
/**     ...repeated <reason> entries                                 */
/**   </exceptions>                                                  */
/**   <uui_return_code>nnnn</uui_return_code>                        */
/**   <uui_reason_code>nnnn</uui_reason_code>                        */
/** </libreply>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractQscrpoolResponse"

static int extractQscrpoolResponse(struct XAPICVT     *pXapicvt,
                                   struct XAPIREQE    *pXapireqe,
                                   char               *pQscratchResponse,
                                   struct XAPISCRPOOL *pXapiscrpool,
                                   char                finalSubpoolFlag,
                                   struct XMLPARSE    *pXmlparse)
{
    int                 lastRC;
    int                 volResponseCount;
    int                 volRemainingCount;
    int                 volPacketCount;
    int                 qscrResponseSize;
    int                 locInt;
    int                 i;

    char                mediaNameString[XAPI_MEDIA_NAME_SIZE + 1];

    struct XAPIMEDIA   *pXapimedia;
    struct XMLELEM     *pFirstUuiTextXmlelem;
    struct XMLELEM     *pNextUuiTextXmlelem;
    struct XMLELEM     *pParentXmlelem;
    struct QSCRCSV     *pQscrcsv;

    QUERY_RESPONSE     *pQuery_Response     = (QUERY_RESPONSE*) pQscratchResponse;
    QU_SCR_RESPONSE    *pQu_Scr_Response    = (QU_SCR_RESPONSE*) &(pQuery_Response->status_response);
    QU_SCR_STATUS      *pQu_Scr_Status      = (QU_SCR_STATUS*) &(pQu_Scr_Response->scratch_status);

    int                 currCount           = pQu_Scr_Response->volume_count;

    /*****************************************************************/
    /* Count the number of <uui_text> entries.                       */
    /*****************************************************************/
    pFirstUuiTextXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                   pXmlparse->pHocXmlelem,
                                                   XNAME_uui_text);

    volResponseCount = 0;

    if (pFirstUuiTextXmlelem != NULL)
    {
        pParentXmlelem = pFirstUuiTextXmlelem->pParentXmlelem;

        TRMSGI(TRCI_XAPI,
               "pFirstUuiTextXmlelem=%08X; pParentXmlelem=%08X\n",
               pFirstUuiTextXmlelem,
               pParentXmlelem);

        pNextUuiTextXmlelem = pFirstUuiTextXmlelem;

        while (pNextUuiTextXmlelem != NULL)
        {
            volResponseCount++;

            pNextUuiTextXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                               pParentXmlelem,
                                                               pNextUuiTextXmlelem,
                                                               XNAME_uui_text);

#ifdef DEBUG

            if (pNextUuiTextXmlelem != NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextUuiTextXmlelem=%08X\n",
                       pNextUuiTextXmlelem);
            }

#endif

        }
    }

    TRMSGI(TRCI_XAPI,
           "volResponseCount=%d, MAX_ID=%d, currCount=%d\n",
           volResponseCount,
           MAX_ID,
           currCount);

    /*****************************************************************/
    /* If no <uui_text> elements found, then return.  If this        */
    /* happens to the be the final subpool, the caller will output   */
    /* any residual final response.                                  */
    /*****************************************************************/
    if (volResponseCount == 0)
    {
        return STATUS_SCRATCH_NOT_AVAILABLE;
    }

    /*****************************************************************/
    /* If <uui_text> elements found, then extract the data.          */
    /*****************************************************************/
    pNextUuiTextXmlelem = pFirstUuiTextXmlelem;
    volRemainingCount = volResponseCount;

    while (1)
    {
        pQuery_Response = (QUERY_RESPONSE*) pQscratchResponse;
        pQu_Scr_Response = (QU_SCR_RESPONSE*) &(pQuery_Response->status_response);
        pQu_Scr_Status = (QU_SCR_STATUS*) &(pQu_Scr_Response->scratch_status[currCount]);

        if (volRemainingCount + currCount > MAX_ID)
        {
            volPacketCount = MAX_ID;
        }
        else
        {
            volPacketCount = volRemainingCount;
        }

        pQu_Scr_Response->volume_count = volPacketCount;

        qscrResponseSize = (char*) pQu_Scr_Status -
                           (char*) pQuery_Response +
                           (volPacketCount * sizeof(QU_SCR_STATUS));

        TRMSGI(TRCI_XAPI,
               "At top of while; currCount=%d, volResponse=%d, "
               "volRemaining=%d, volPacket=%d, "
               "MAX_ID=%d, qscrResponseSize=%d\n",
               currCount,
               volResponseCount,
               volRemainingCount,
               volPacketCount,
               MAX_ID,
               qscrResponseSize);

        for (i = 0;
            i < volPacketCount;
            i++, pQu_Scr_Status++)
        {
            if (pNextUuiTextXmlelem == NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextUuiTextXmlelem=NULL at volCount=%i\n",
                       (i+1));

                break;
            }

            pQscrcsv = (struct QSCRCSV*) pNextUuiTextXmlelem->pContent;

#ifdef DEBUG

            TRMEM(pQscrcsv, sizeof(struct QSCRCSV), 
                  "Next pQu_Scr_Status=%08X, driveAcs=%08X; QSCRCSV:\n",
                  &pQu_Scr_Status,
                  pQscrcsv->driveAcs);

#endif

            pQu_Scr_Status->status = STATUS_SUCCESS;

            memcpy(pQu_Scr_Status->vol_id.external_label,
                   pQscrcsv->volser,
                   sizeof(pQscrcsv->volser));

            pQu_Scr_Status->pool_id.pool = (POOL) pXapiscrpool->subpoolIndex;

            if (pQscrcsv->cellAcs[0] > ' ')
            {
                FN_CONVERT_DIGITS_TO_FULLWORD(pQscrcsv->cellAcs,
                                              sizeof(pQscrcsv->cellAcs),
                                              &locInt);

                pQu_Scr_Status->home_location.panel_id.lsm_id.acs = (ACS) locInt;

                if (pQscrcsv->cellLsm[0] > ' ')
                {
                    FN_CONVERT_DIGITS_TO_FULLWORD(pQscrcsv->cellLsm,
                                                  sizeof(pQscrcsv->cellLsm),
                                                  &locInt);

                    pQu_Scr_Status->home_location.panel_id.lsm_id.lsm = (ACS) locInt;

                    if (pQscrcsv->cellPanel[0] > ' ')
                    {
                        FN_CONVERT_DIGITS_TO_FULLWORD(pQscrcsv->cellPanel,
                                                      sizeof(pQscrcsv->cellPanel),
                                                      &locInt);

                        pQu_Scr_Status->home_location.panel_id.panel = (PANEL) locInt;

                        if (pQscrcsv->cellRow[0] > ' ')
                        {
                            FN_CONVERT_DIGITS_TO_FULLWORD(pQscrcsv->cellRow,
                                                          sizeof(pQscrcsv->cellRow),
                                                          &locInt);

                            pQu_Scr_Status->home_location.row = (ROW) locInt;

                            if (pQscrcsv->cellColumn[0] > ' ')
                            {
                                FN_CONVERT_DIGITS_TO_FULLWORD(pQscrcsv->cellColumn,
                                                              sizeof(pQscrcsv->cellColumn),
                                                              &locInt);

                                pQu_Scr_Status->home_location.col = (COL) locInt;
                            }
                        }
                    }
                }
            }

            if (pQscrcsv->mediaName[0] >= ' ')
            {
                STRIP_TRAILING_BLANKS(pQscrcsv->mediaName,
                                      mediaNameString,
                                      sizeof(pQscrcsv->mediaName));

                pXapimedia = xapi_media_search_name(pXapicvt,
                                                    pXapireqe,
                                                    mediaNameString);

                if (pXapimedia != NULL)
                {
                    pQu_Scr_Status->media_type = 
                    (MEDIA_TYPE) pXapimedia->acsapiMediaType;
                }
                else
                {
                    pQu_Scr_Status->media_type = ANY_MEDIA_TYPE;
                }
            }

            pNextUuiTextXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                               pParentXmlelem,
                                                               pNextUuiTextXmlelem,
                                                               XNAME_uui_text);
        } /* for (i) */

        volRemainingCount = volRemainingCount - volPacketCount;
        currCount = 0;

        if (volRemainingCount <= 0)
        {
            break;
        }

        xapi_int_response(pXapireqe,
                          pQscratchResponse,     
                          qscrResponseSize);

        xapi_query_init_resp(pXapireqe,
                             (char*) pQuery_Response,
                             qscrResponseSize);
    }

    if (!(finalSubpoolFlag))
    {
        if (volPacketCount == MAX_ID)
        {
            xapi_int_response(pXapireqe,
                              pQscratchResponse,     
                              qscrResponseSize);

            xapi_query_init_resp(pXapireqe,
                                 (char*) pQuery_Response,
                                 qscrResponseSize);
        }
    }

    return STATUS_SUCCESS;
}



