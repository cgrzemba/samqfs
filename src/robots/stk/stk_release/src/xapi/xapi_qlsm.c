/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qlsm.c                                      */
/** Description:    XAPI QUERY LSM processor.                        */
/**                                                                  */
/**                 Return LSM information and status.               */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     06/21/11                          */
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
#define QUERY_SEND_TIMEOUT         5   /* TIMEOUT values in seconds  */       
#define QUERY_RECV_TIMEOUT_1ST     300
#define QUERY_RECV_TIMEOUT_NON1ST  600
#define QLSMERR_RESPONSE_SIZE (offsetof(QUERY_RESPONSE, status_response)) + \
                              (offsetof(QU_LSM_RESPONSE, lsm_status[0])) 


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int convertQlsmRequest(struct XAPICVT   *pXapicvt,
                              struct XAPIREQE  *pXapireqe,
                              char            **ptrXapiBuffer,
                              int              *pXapiBufferSize);

static int extractQlsmResponse(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char            *pQueryResponse,
                               int              qlsmResponseSize,
                               int             *pFinalLsmCount,
                               struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qlsm                                         */
/** Description:   The XAPI QUERY LSM processor.                     */
/**                                                                  */
/** Return LSM information and status from the TapePlex server.      */
/**                                                                  */
/** The ACSAPI format QUERY LSM request is translated into an        */
/** XAPI XML format <query_lsm> request; the XAPI XML request is     */
/** then transmitted to the server via TCP/IP;  The received         */
/** XAPI XML response is then translated into one or more            */
/** ACSAPI QUERY LSM responses.                                      */
/**                                                                  */
/** There will only be one individual XAPI <query_lsm> request       */
/** issued for a single ACSAPI QUERY LSM request regardless of       */
/** the requested lsm count (as the XAPI <query_lsm> request allows  */
/** an <lsm_list> to be specified).                                  */
/**                                                                  */
/** The QUERY LSM command is allowed to proceed even when            */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qlsm"

extern int xapi_qlsm(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;
    int                 qlsmResponseSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_LSM_CRITERIA    *pQu_Lsm_Criteria    = &(pQuery_Request->select_criteria.lsm_criteria);

    int                 lsmCount            = pQu_Lsm_Criteria->lsm_count;
    int                 finalLsmCount       = 0;

    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;
    QU_LSM_RESPONSE    *pQu_Lsm_Response    = &(pQuery_Response->status_response.lsm_response);
    QU_LSM_STATUS      *pQu_Lsm_Status      = &(pQu_Lsm_Response->lsm_status[0]);

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         sizeof(QU_LSM_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY LSM request=%08X, size=%d, count=%d, "
           "MAX_ID=%d, QUERY LSM response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           lsmCount,
           MAX_ID,
           pQuery_Response,
           sizeof(QU_LSM_RESPONSE));

    lastRC = convertQlsmRequest(pXapicvt,
                                pXapireqe,
                                &pXapiBuffer,
                                &xapiBufferSize);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from convertQlsmRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QLSMERR_RESPONSE_SIZE,
                          lastRC);

        return lastRC;
    }

    if (lsmCount == 0)
    {
        qlsmResponseSize = (char*) pQu_Lsm_Status -
                           (char*) pQuery_Response +
                           ((sizeof(QU_LSM_STATUS)) * MAX_ID);
    }
    else
    {
        qlsmResponseSize = (char*) pQu_Lsm_Status -
                           (char*) pQuery_Response +
                           ((sizeof(QU_LSM_STATUS)) * lsmCount);
    }

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

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
    /* Now generate the QUERY LSM ACSAPI response.                   */
    /*****************************************************************/
    if (queryRC == STATUS_SUCCESS)
    {
        lastRC = extractQlsmResponse(pXapicvt,
                                     pXapireqe,
                                     (char*) pQuery_Response,
                                     qlsmResponseSize,
                                     &finalLsmCount,
                                     pXmlparse);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractQlsmResponse; finalLsmCount=%d\n",
               lastRC,
               finalLsmCount);

        if (finalLsmCount != lsmCount)
        {
            qlsmResponseSize = (char*) pQu_Lsm_Status -
                               (char*) pQuery_Response +
                               ((sizeof(QU_LSM_STATUS)) * finalLsmCount);
        }

        if (lastRC != STATUS_SUCCESS)
        {
            queryRC = lastRC;
        }
    }

    if (queryRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QLSMERR_RESPONSE_SIZE,
                          queryRC);
    }
    else
    {
        xapi_fin_response(pXapireqe,
                          (char*) pQuery_Response,
                          qlsmResponseSize);
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
/** Function Name: convertQlsmRequest                                */
/** Description:   Build an XAPI <query_lsm> request.                */
/**                                                                  */
/** Convert the ACSAPI format QUERY LSM request into an              */
/** XAPI XML format <query_lsm> request.                             */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY LSM request consists of:                        */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY_REQUEST data consisting of:                           */
/**      i.   type                                                   */
/** 3.   QU_LSM_CRITERIA data consisting of:                         */
/**      i.   count (of lsm(s) requested or 0 for "all")             */
/**      ii.  LSMID[count] lsm data entries consisting of:           */
/**           a.   lsm_id.lsm_id.acs                                 */
/**           b.   lsm_id.lsm_id.lsm                                 */
/**           c    lsm_id.lsm                                        */
/**                                                                  */
/** NOTE: MAX_ID is 42.  So a max subset of 42 lsm(s) can be         */
/** requested at once.  If 0 lsm(s) is specified, then all           */
/** lsm(s) are returned in possible multiple intermediate responses. */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_lsm> request consists of:                    */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**                                                                  */
/**   If count = 0 (all lsms are displayed):                         */
/**                                                                  */
/**     <query_lsm>                                                  */
/**     </query_lsm>                                                 */
/**                                                                  */
/**   If count = 1:                                                  */
/**                                                                  */
/**     <query_lsm>                                                  */
/**       <lsm_location_data>                                        */
/**         <acs>aa</acs>                                            */
/**         <lsm>ll</lsm>                                            */
/**         <lsm>nn</lsm>                                            */
/**       </lsm_location_data>                                       */
/**     </query_lsm>                                                 */
/**                                                                  */
/**   If count > 1:                                                  */
/**                                                                  */
/**     <query_lsm>                                                  */
/**       <detail_request>NO</detail_request>                        */
/**       <lsm_list>                                                 */
/**         <lsm_location_data>                                      */
/**           <acs>aa</acs>                                          */
/**           <lsm>ll</lsm>                                          */
/**         </lsm_location_data>                                     */
/**         ...repeated <lsm_location_data> entries                  */
/**       </lsm_list>                                                */
/**     </query_lsm>                                                 */
/**                                                                  */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** NOTES:                                                           */
/** (1) The CDK allows specification of a LSM list.  However this    */
/**     is currently not supported by the XAPI.                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "convertQlsmRequest"

static int convertQlsmRequest(struct XAPICVT  *pXapicvt,
                              struct XAPIREQE *pXapireqe,
                              char           **ptrXapiBuffer,
                              int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;
    int                 i;
    int                 lsmCount;
    char                lsmString[3];

    char               *pXapiRequest        = NULL;
    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_LSM_CRITERIA    *pQu_Lsm_Criteria    = &(pQuery_Request->select_criteria.lsm_criteria);

    struct XMLPARSE    *pXmlparse; 
    struct XMLELEM     *pParentXmlelem;
    struct XMLELEM     *pLsmListXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    lsmCount = pQu_Lsm_Criteria->lsm_count;

    TRMSGI(TRCI_XAPI,
           "Entered; lsmCount=%d\n",
           lsmCount);

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

    if (lsmCount < 0)
    {
        return STATUS_COUNT_TOO_SMALL;
    }

    if (lsmCount > MAX_ID)
    {
        return STATUS_COUNT_TOO_LARGE;
    }

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
                        TRUE,
                        FALSE,
                        XML_CASE_UPPER,
                        XML_DATE_STCK);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_command,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_query_lsm,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_detail_request,
                                      XCONTENT_N,
                                      0);

    if (lsmCount > 0)
    {
        if (lsmCount > 1)
        {
            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pParentXmlelem,
                                              XNAME_lsm_list,
                                              NULL,
                                              0);

            pLsmListXmlelem = pXmlparse->pCurrXmlelem;
        }
        else
        {
            pLsmListXmlelem = pParentXmlelem;
        }

        for (i = 0;
            i < lsmCount;
            i++)
        {
            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pLsmListXmlelem,
                                              XNAME_lsm_location_data,
                                              NULL,
                                              0);

            pParentXmlelem = pXmlparse->pCurrXmlelem;

            sprintf(lsmString,
                    "%.2d",
                    pQu_Lsm_Criteria->lsm_id[i].acs);

            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pParentXmlelem,
                                              XNAME_acs,
                                              lsmString,
                                              0);

            sprintf(lsmString,
                    "%.2d",
                    pQu_Lsm_Criteria->lsm_id[i].lsm);

            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pParentXmlelem,
                                              XNAME_lsm,
                                              lsmString,
                                              0);
        }
    }

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

    return STATUS_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: extractQlsmResponse                               */
/** Description:   Extract the <lsm_data> response.                  */
/**                                                                  */
/** Parse the response of the XAPI XML <query_lsm> request and       */
/** update the appropriate fields of the ACSAPI QUERY LSM response.  */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_lsm> (<detail>NO) responses consists of:     */
/**==================================================================*/
/** <libreply>                                                       */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   <lsm_data>                                                     */
/**     <lsm_location_data>                                          */
/**       <acs>aa</acs>                                              */
/**       <lsm>ll</lsm>                                              */
/**     </lsm_location_data>                                         */
/**     <model>MMMMMMMM</model>                                      */
/**     <status>ssssssssssssssss</status>                            */
/**     <state>sssssssss</state>                                     */
/**     <cell_count>nnnn</cell_count>                                */
/**     <free_cell_count>nnnnnnnn</free_cell_count>                  */
/**     <scratch_count>nnnnnnn</scratch_count>                       */
/**     <cleaner_count>nnnn</cleaner_count>                          */
/**     <cap_count>nnn</cap_count>                                   */
/**     <panel_count>nnn</panel_count>                               */
/**     <adjacent_count>nnnn</adjacent_count>                        */
/**     <adjacent_lsm>                                               */
/**       <lsm>ll</lsm>                                              */
/**       ...repeated <lsm> entries                                  */
/**     </adjacent_lsm>                                              */
/**     <result>SUCCESS"|"FAILURE"</result>                          */
/**     <error>xxxxxxxx</error>                                      */
/**     <reason>ccc...ccc</reason>                                   */
/**   </lsm_data>                                                    */
/**   ...repeated <lsm_data> entries                                 */
/**   <exceptions>                                                   */
/**     <reason>ccc...ccc</reason>                                   */
/**     ...repeated <reason> entries                                 */
/**   </exceptions>                                                  */
/**   <uui_return_code>nnnn</uui_return_code>                        */
/**   <uui_reason_code>nnnn</uui_reason_code>                        */
/** </libreply>                                                      */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY LSM response consists of:                       */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   QUERY_RESPONSE data consisting of                           */
/**      i.   type                                                   */
/**      ii.  QU_LSM_RESPONSE data consisting of:                    */
/**           a.   count                                             */
/**           b.   QU_LSM_STATUS[count] data entries consisting of:  */
/**                1.   LSMID consisting of:                         */
/**                     i.   lsm_id.acs                              */
/**                     ii.  lsm_id.lsm                              */
/**                2.   state                                        */
/**                3.   freecells                                    */
/**                4.   REQ_SUMMARY                                  */
/**                     which is an array of:                        */
/**                     requests[MAX_COMMANDS] [MAX_DISPOSITIONS]    */
/**                5.   status                                       */
/**                                                                  */
/** MAX_COMMANDS is 5 (for AUDIT, MOUNT, DISMOUNT, ENTER, and EJECT).*/
/** MAX_DISPOSITIONS is 2 (for CURRENT and PENDING).                 */
/**                                                                  */
/** NOTE: Currently, the XAPI QUERY LSM response does not            */
/** return any request summary information, so that portion of       */
/** the ACSAPI QUERY LSM FINAL_RESPONSE will be 0.                   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractQlsmResponse"

static int extractQlsmResponse(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char            *pQueryResponse,
                               int              qlsmResponseSize,
                               int             *pFinalLsmCount,
                               struct XMLPARSE *pXmlparse)
{
    int                 lastRC;
    int                 lsmResponseCount;
    int                 lsmRemainingCount;
    int                 lsmPacketCount;
    int                 i;
    int                 wkInt;

    struct XMLELEM     *pFirstLsmDataXmlelem;
    struct XMLELEM     *pNextLsmDataXmlelem;
    struct XMLELEM     *pLsmLocationDataXmlelem;
    struct XMLELEM     *pParentXmlelem;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_LSM_CRITERIA    *pQu_Lsm_Criteria    = (QU_LSM_CRITERIA*) &(pQuery_Request->select_criteria);
    int                 lsmRequestCount     = pQu_Lsm_Criteria->lsm_count;

    QUERY_RESPONSE     *pQuery_Response     = (QUERY_RESPONSE*) pQueryResponse;
    QU_LSM_RESPONSE    *pQu_Lsm_Response    = (QU_LSM_RESPONSE*) &(pQuery_Response->status_response);
    QU_LSM_STATUS      *pQu_Lsm_Status      = (QU_LSM_STATUS*) &(pQu_Lsm_Response->lsm_status);

    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;

    struct RAWLSM       rawlsm;
    struct RAWLSM      *pRawlsm             = &rawlsm;

    struct XMLSTRUCT    lsmDataXmlstruct[]  =
    {
        XNAME_lsm_data,                     XNAME_model,
        sizeof(pRawlsm->model),             pRawlsm->model,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_lsm_data,                     XNAME_status,
        sizeof(pRawlsm->status),            pRawlsm->status,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_lsm_data,                     XNAME_state,
        sizeof(pRawlsm->state),             pRawlsm->state,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_lsm_data,                     XNAME_mode,
        sizeof(pRawlsm->mode),              pRawlsm->mode,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_lsm_data,                     XNAME_cell_count,
        sizeof(pRawlsm->cellCount),         pRawlsm->cellCount,
        ZEROFILL, NOBITVALUE, 0, 0,
        XNAME_lsm_data,                     XNAME_free_cell_count,
        sizeof(pRawlsm->freeCellCount),     pRawlsm->freeCellCount,
        ZEROFILL, NOBITVALUE, 0, 0,
        XNAME_lsm_data,                     XNAME_scratch_count,
        sizeof(pRawlsm->scratchCount),      pRawlsm->scratchCount,
        ZEROFILL, NOBITVALUE, 0, 0,
        XNAME_lsm_data,                     XNAME_cleaner_count,
        sizeof(pRawlsm->cleanerCount),      pRawlsm->cleanerCount,
        ZEROFILL, NOBITVALUE, 0, 0,
        XNAME_lsm_data,                     XNAME_adjacent_count,
        sizeof(pRawlsm->adjacentCount),     pRawlsm->adjacentCount,
        ZEROFILL, NOBITVALUE, 0, 0,
        XNAME_lsm_data,                     XNAME_cap_count,
        sizeof(pRawlsm->capCount),          pRawlsm->capCount,
        ZEROFILL, NOBITVALUE, 0, 0,
        XNAME_lsm_data,                     XNAME_panel_count,
        sizeof(pRawlsm->panelCount),        pRawlsm->panelCount,
        ZEROFILL, NOBITVALUE, 0, 0,
        XNAME_lsm_data,                     XNAME_result,
        sizeof(pRawlsm->result),            pRawlsm->result,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_lsm_data,                     XNAME_error,
        sizeof(pRawlsm->error),             pRawlsm->error,
        NOBLANKFILL, NOBITVALUE, 0, 0,      
        XNAME_lsm_data,                     XNAME_reason,
        sizeof(pRawlsm->reason),            pRawlsm->reason,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 lsmDataElementCount = sizeof(lsmDataXmlstruct) / 
                                              sizeof(struct XMLSTRUCT);

    struct XMLSTRUCT    lsmLocDataXmlstruct[]  =
    {
        XNAME_lsm_location_data,            XNAME_acs,
        sizeof(pRawlsm->acs),               pRawlsm->acs,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_lsm_location_data,            XNAME_lsm,
        sizeof(pRawlsm->lsm),               pRawlsm->lsm,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 lsmLocDataElementCount = sizeof(lsmLocDataXmlstruct) / 
                                                 sizeof(struct XMLSTRUCT);

    lastRC = xapi_parse_header_trailer(pXapicvt,
                                       pXapireqe,
                                       pXmlparse,
                                       pXapicommon);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from xapi_parse_header_trailer; "
           "uuiRC=%d, uuiReason=%d\n",
           lastRC,
           pXapicommon->uuiRC,
           pXapicommon->uuiReason);

    /*****************************************************************/
    /* Unfortunately, HSC produces any error <reason> for the XAPI   */
    /* QUERY LSM command under the <lsm_data> tag, and not under     */
    /* the <libreply><exceptions>.  Therefore, we continue trying    */
    /* to extract <lsm_data> even when xapi_parse_header_trailer     */
    /* returns a bad RC.                                             */
    /*****************************************************************/

    /*****************************************************************/
    /* Count the number of <lsm_data> entries.                       */
    /*****************************************************************/
    pFirstLsmDataXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                   pXmlparse->pHocXmlelem,
                                                   XNAME_lsm_data);

    lsmResponseCount = 0;

    if (pFirstLsmDataXmlelem != NULL)
    {
        pParentXmlelem = pFirstLsmDataXmlelem->pParentXmlelem;

        TRMSGI(TRCI_XAPI,
               "pFirstLsmDataXmlelem=%08X; pParentXmlelem=%08X\n",
               pFirstLsmDataXmlelem,
               pParentXmlelem);

        pNextLsmDataXmlelem = pFirstLsmDataXmlelem;

        while (pNextLsmDataXmlelem != NULL)
        {
            lsmResponseCount++;

            pNextLsmDataXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                               pParentXmlelem,
                                                               pNextLsmDataXmlelem,
                                                               XNAME_lsm_data);

#ifdef DEBUG

            if (pNextLsmDataXmlelem != NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextLsmDataXmlelem=%08X\n",
                       pNextLsmDataXmlelem);
            }

#endif

        }
    }

    TRMSGI(TRCI_XAPI,
           "lsmResponseCount=%d, MAX_ID=%d\n",
           lsmResponseCount,
           MAX_ID);

    /*****************************************************************/
    /* If no <lsm_data> elements found, then return an error.        */
    /*****************************************************************/
    if (lsmResponseCount == 0)
    {
        if (lastRC > 0)
        {
            return lastRC;
        }

        return STATUS_PROCESS_FAILURE;
    }

    /*****************************************************************/
    /* If <lsm_data> elements found, then extract the data.          */
    /*****************************************************************/
    pNextLsmDataXmlelem = pFirstLsmDataXmlelem;
    lsmRemainingCount = lsmResponseCount;

    while (1)
    {
        xapi_query_init_resp(pXapireqe,
                             (char*) pQueryResponse,   
                             qlsmResponseSize);

        pQuery_Response = (QUERY_RESPONSE*) pQueryResponse;
        pQu_Lsm_Response = (QU_LSM_RESPONSE*) &(pQuery_Response->status_response);
        pQu_Lsm_Status = (QU_LSM_STATUS*) &(pQu_Lsm_Response->lsm_status);

        if (lsmRemainingCount > MAX_ID)
        {
            lsmPacketCount = MAX_ID;
        }
        else
        {
            lsmPacketCount = lsmRemainingCount;
        }

        TRMSGI(TRCI_XAPI,
               "At top of while; lsmResponse=%d, lsmRemaining=%d, "
               "lsmPacket=%d, MAX_ID=%d\n",
               lsmResponseCount,
               lsmRemainingCount,
               lsmPacketCount,
               MAX_ID);

        pQu_Lsm_Response->lsm_count = lsmPacketCount;

        for (i = 0;
            i < lsmPacketCount;
            i++, pQu_Lsm_Status++)
        {
            memset(pRawlsm, 0, sizeof(struct RAWLSM));

            if (pNextLsmDataXmlelem == NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextLsmDataXmlelem=NULL at lsmCount=%i\n",
                       (i+1));

                break;
            }

            FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                              pNextLsmDataXmlelem,
                                              &lsmDataXmlstruct[0],
                                              lsmDataElementCount);

            pLsmLocationDataXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                              pNextLsmDataXmlelem,
                                                              XNAME_lsm_location_data);

            if (pLsmLocationDataXmlelem != NULL)
            {
                FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                                  pLsmLocationDataXmlelem,
                                                  &lsmLocDataXmlstruct[0],
                                                  lsmLocDataElementCount);
            }

            TRMEMI(TRCI_XAPI,
                   pRawlsm, sizeof(struct RAWLSM),
                   "RAWLSM:\n");

            if (pRawlsm->acs[0] > ' ')
            {
                FN_CONVERT_DIGITS_TO_FULLWORD(pRawlsm->acs,
                                              sizeof(pRawlsm->acs),
                                              &wkInt);

                pQu_Lsm_Status->lsm_id.acs = (ACS) wkInt;

                if (pRawlsm->lsm[0] > ' ')
                {
                    FN_CONVERT_DIGITS_TO_FULLWORD(pRawlsm->lsm,
                                                  sizeof(pRawlsm->lsm),
                                                  &wkInt);

                    pQu_Lsm_Status->lsm_id.lsm = (LSM) wkInt;
                }
            }
            /*********************************************************/
            /* If we cannot find the ACS and LSM in the response,    */
            /* then get them from the request.                       */
            /*********************************************************/
            else if (lsmRequestCount > 0)
            {
                pQu_Lsm_Status->lsm_id.acs = 
                pQu_Lsm_Criteria->lsm_id[i].acs;

                pQu_Lsm_Status->lsm_id.lsm = 
                pQu_Lsm_Criteria->lsm_id[i].lsm;
            }

            if (pRawlsm->result[0] == 'F')
            {
                pQu_Lsm_Status->status = STATUS_LSM_NOT_IN_LIBRARY;    
            }
            else
            {
                pQu_Lsm_Status->status = STATUS_SUCCESS;    

                if (memcmp(pRawlsm->state,
                           "ONLINE",
                           3) == 0)
                {
                    pQu_Lsm_Status->state = STATE_ONLINE;    
                }
                else
                {
                    pQu_Lsm_Status->state = STATE_OFFLINE;    
                }

                if (pRawlsm->freeCellCount[0] > ' ')
                {
                    FN_CONVERT_DIGITS_TO_FULLWORD(pRawlsm->freeCellCount,
                                                  sizeof(pRawlsm->freeCellCount),
                                                  &wkInt);

                    pQu_Lsm_Status->freecells = wkInt;    
                }
            }

            pNextLsmDataXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                               pParentXmlelem,
                                                               pNextLsmDataXmlelem,
                                                               XNAME_lsm_data);
        } /* for(i) */

        lsmRemainingCount = lsmRemainingCount - lsmPacketCount;

        if (lsmRemainingCount <= 0)
        {
            *pFinalLsmCount = lsmPacketCount;

            break;
        }

        xapi_int_response(pXapireqe,
                          pQueryResponse,     
                          qlsmResponseSize);  
    }

    return STATUS_SUCCESS;
}


