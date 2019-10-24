/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qacs.c                                      */
/** Description:    XAPI QUERY ACS processor.                        */
/**                                                                  */
/**                 Return ACS information and status.               */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     06/01/11                          */
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
#define QACSERR_RESPONSE_SIZE (offsetof(QUERY_RESPONSE, status_response)) + \
                              (offsetof(QU_ACS_RESPONSE, acs_status[0])) 


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int convertQacsRequest(struct XAPICVT   *pXapicvt,
                              struct XAPIREQE  *pXapireqe,
                              char            **ptrXapiBuffer,
                              int              *pXapiBufferSize);

static int extractQacsResponse(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char            *pQueryResponse,
                               int              qacsResponseSize,
                               int             *pFinalAcsCount,
                               struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qacs                                         */
/** Description:   XAPI QUERY ACS processor.                         */
/**                                                                  */
/** Return ACS information and status from the TapePlex server.      */
/**                                                                  */
/** The ACSAPI format QUERY ACS request is translated into an        */
/** XAPI XML format <query_acs> request; The XAPI XML request is     */
/** then transmitted to the server via TCP/IP;  The received         */
/** XAPI XML response is then translated into one or more            */
/** ACSAPI QUERY ACS responses.                                      */
/**                                                                  */
/** There will only be one individual XAPI <query_acs> request       */
/** issued for a single ACSAPI QUERY ACS request regardless of       */
/** the requested acs count (as the XAPI <query_acs> request allows  */
/** an <acs_list> to be specified).                                  */
/**                                                                  */
/** The QUERY ACS command is allowed to proceed even when            */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qacs"

extern int xapi_qacs(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;
    int                 qacsResponseSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_ACS_CRITERIA    *pQu_Acs_Criteria    = &(pQuery_Request->select_criteria.acs_criteria);

    int                 acsCount            = pQu_Acs_Criteria->acs_count;
    int                 finalAcsCount       = 0;

    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;
    QU_ACS_RESPONSE    *pQu_Acs_Response    = &(pQuery_Response->status_response.acs_response);
    QU_ACS_STATUS      *pQu_Acs_Status      = &(pQu_Acs_Response->acs_status[0]);

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         sizeof(QU_ACS_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY ACS request=%08X, size=%d, count=%d, "
           "MAX_ID=%d, QUERY ACS response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           acsCount,
           MAX_ID,
           pQuery_Response,
           sizeof(QU_ACS_RESPONSE));

    lastRC = convertQacsRequest(pXapicvt,
                                pXapireqe,
                                &pXapiBuffer,
                                &xapiBufferSize);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from convertQacsRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QACSERR_RESPONSE_SIZE,
                          lastRC);

        return lastRC;
    }

    if (acsCount == 0) 
    {
        qacsResponseSize = (char*) pQu_Acs_Status -
                           (char*) pQuery_Response +
                           ((sizeof(QU_ACS_STATUS)) * MAX_ID);
    }
    else
    {
        qacsResponseSize = (char*) pQu_Acs_Status -
                           (char*) pQuery_Response +
                           ((sizeof(QU_ACS_STATUS)) * acsCount);
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
    /* Now generate the QUERY ACS ACSAPI response.                   */
    /*****************************************************************/
    if (queryRC == STATUS_SUCCESS)
    {
        lastRC = extractQacsResponse(pXapicvt,
                                     pXapireqe,
                                     (char*) pQuery_Response,
                                     qacsResponseSize,
                                     &finalAcsCount,
                                     pXmlparse);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractQacsResponse; finalAcsCount=%d\n",
               lastRC,
               finalAcsCount);

        if (finalAcsCount != acsCount)
        {
            qacsResponseSize = (char*) pQu_Acs_Status -
                               (char*) pQuery_Response +
                               ((sizeof(QU_ACS_STATUS)) * finalAcsCount);
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
                          qacsResponseSize,
                          queryRC);
    }
    else
    {
        xapi_fin_response(pXapireqe,
                          (char*) pQuery_Response,
                          qacsResponseSize);
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
/** Function Name: convertQacsRequest                                */
/** Description:   Build an XAPI <query_acs> request.                */
/**                                                                  */
/** Convert the ACSAPI format QUERY ACS request into an              */
/** XAPI XML format <query_acs> request.                             */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY ACS request consists of:                        */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY_REQUEST data consisting of                            */
/**      i.   type                                                   */
/** 3.   QU_ACS_CRITERIA data consisting of:                         */
/**      i.   count (of acs(s) requested or 0 for "all")             */
/**      ii.  acs_id[count] acs entries                              */
/**                                                                  */
/** NOTE: MAX_ID is 42.  So a max subset of 42 acs(s) can be         */
/** requested at once.  If 0 acs(s) is specified, then all           */
/** acs(s) are returned in possible multiple intermediate responses. */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_acs> request consists of:                    */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**                                                                  */
/**   If count = 0 (all acs(s) are displayed):                       */
/**                                                                  */
/**     <query_acs>                                                  */
/**     </query_acs>                                                 */
/**                                                                  */
/**   If count > 0 (specific acs(s) are displayed):                  */
/**                                                                  */
/**     <query_acs>                                                  */
/**       <acs_list>                                                 */
/**         <acs>aa</acs>                                            */
/**         ...repeated <acs> entries                                */
/**       <acs_list>                                                 */
/**     </query_acs>                                                 */
/**                                                                  */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** NOTES:                                                           */
/** (1) If <acs> is omitted, then all acs(s) are returned.           */
/** (2) While the CDK allows count > 1, and multiple ACS(s) be       */
/**     specified, the XAPI does not currenly allow specification    */
/**     of multiple <acs> tags in the query.                         */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "convertQacsRequest"

static int convertQacsRequest(struct XAPICVT  *pXapicvt,
                              struct XAPIREQE *pXapireqe,
                              char           **ptrXapiBuffer,
                              int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;
    int                 i;
    int                 acsCount;
    char                acsString[3];

    char               *pXapiRequest        = NULL;
    char               *pAcsapiBuffer       = pXapireqe->pAcsapiBuffer;
    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pAcsapiBuffer;
    QU_ACS_CRITERIA    *pQu_Acs_Criteria    = (QU_ACS_CRITERIA*) &(pQuery_Request->select_criteria);

    struct XMLPARSE    *pXmlparse; 
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    acsCount = pQu_Acs_Criteria->acs_count;

    TRMSGI(TRCI_XAPI,
           "Entered; acsCount=%d\n",
           acsCount);

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

    if (acsCount < 0)
    {
        return STATUS_COUNT_TOO_SMALL;
    }

    if (acsCount > MAX_ID)
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
                                      XNAME_query_acs,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    if (acsCount > 0)
    {
        if (acsCount > 1)
        {
            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pParentXmlelem,
                                              XNAME_acs_list,
                                              NULL,
                                              0);

            pParentXmlelem = pXmlparse->pCurrXmlelem;
        }

        for (i = 0;
            i < acsCount;
            i++)
        {
            sprintf(acsString,
                    "%.2d",
                    pQu_Acs_Criteria->acs_id[i]);

            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pParentXmlelem,
                                              XNAME_acs,
                                              acsString,
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
/** Function Name: extractQacsResponse                               */
/** Description:   Extract the <acs_data> response.                  */
/**                                                                  */
/** Parse the response of the XAPI XML <query_acs> request and       */
/** update the appropriate fields of the ACSAPI QUERY ACS response.  */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_acs> responses consists of:                  */
/**==================================================================*/
/** <libreply>                                                       */
/**   <query_acs_request>                                            */
/**     <header>                                                     */
/**       header element children                                    */
/**     </header>                                                    */
/**     <acs_data>                                                   */
/**       <acs>aa></acs>                                             */
/**       <acs_status>"CONNECTED"|"DISCONNECTED"</acs_status>        */
/**       <lsm_count>nn</lsm_count>                                  */
/**       <scratch_count>nnnnn</scratch_count>                       */
/**       <free_cell_count>nnnn</free_cell_count>                    */
/**       <cap_count>nnn</cap_count>                                 */
/**       <dual_lmu_config>"YES"|"NO"</dual_lmu_config>              */
/**       <state>"ONLINE"|"OFFLINE"</state>                          */
/**       <result>SUCCESS"|"FAILURE"</result>                        */
/**       <error>xxxxxxxx</error>                                    */
/**       <reason>ccc...ccc</reason>                                 */
/**     </acs_data>                                                  */
/**     ...repeated <acs_data> entries                               */
/**     <exceptions>                                                 */
/**       <reason>ccc...ccc</reason>                                 */
/**       ...repeated <reason> entries                               */
/**     </exceptions>                                                */
/**     <uui_return_code>nnnn</uui_return_code>                      */
/**     <uui_reason_code>nnnn</uui_reason_code>                      */
/**   </query_acs_request>                                           */
/** </libreply>                                                      */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY ACS response consists of:                       */
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
/**      ii.  QU_ACS_RESPONSE data consisting of:                    */
/**           a.   count                                             */
/**           b.   QU_ACS_STATUS[count] data entries consisting of:  */
/**                1.   acs_id                                       */
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
/** NOTE: Currently, the XAPI QUERY ACS response does not            */
/** return any request summary information, so that portion of       */
/** the ACSAPI QUERY SERVER FINAL_RESPONSE will be 0.                */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractQacsResponse"

static int extractQacsResponse(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char            *pQueryResponse,
                               int              qacsResponseSize,
                               int             *pFinalAcsCount,
                               struct XMLPARSE *pXmlparse)
{
    int                 lastRC;
    int                 acsResponseCount;
    int                 acsRemainingCount;
    int                 acsPacketCount;
    int                 i;
    int                 wkInt;

    struct XMLELEM     *pFirstAcsDataXmlelem;
    struct XMLELEM     *pNextAcsDataXmlelem;
    struct XMLELEM     *pParentXmlelem;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_ACS_CRITERIA    *pQu_Acs_Criteria    = (QU_ACS_CRITERIA*) &(pQuery_Request->select_criteria);
    int                 acsRequestCount     = pQu_Acs_Criteria->acs_count;

    QUERY_RESPONSE     *pQuery_Response     = (QUERY_RESPONSE*) pQueryResponse;
    QU_ACS_RESPONSE    *pQu_Acs_Response    = (QU_ACS_RESPONSE*) &(pQuery_Response->status_response);
    QU_ACS_STATUS      *pQu_Acs_Status      = (QU_ACS_STATUS*) &(pQu_Acs_Response->acs_status);

    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;

    struct RAWACS       rawacs;
    struct RAWACS      *pRawacs             = &rawacs;

    struct XMLSTRUCT    acsDataXmlstruct[]  =
    {
        XNAME_acs_data,                     XNAME_acs,
        sizeof(pRawacs->acs),               pRawacs->acs,
        NOBLANKFILL, NOBITVALUE, 0, 0,      
        XNAME_acs_data,                     XNAME_acs_status,
        sizeof(pRawacs->status),            pRawacs->status,
        NOBLANKFILL, NOBITVALUE, 0, 0,      
        XNAME_acs_data,                     XNAME_lsm_count,
        sizeof(pRawacs->lsmCount),          pRawacs->lsmCount,
        ZEROFILL, NOBITVALUE, 0, 0,
        XNAME_acs_data,                     XNAME_scratch_count,
        sizeof(pRawacs->scratchCount),      pRawacs->scratchCount,
        ZEROFILL, NOBITVALUE, 0, 0,      
        XNAME_acs_data,                     XNAME_free_cell_count,
        sizeof(pRawacs->freeCellCount),     pRawacs->freeCellCount,
        ZEROFILL, NOBITVALUE, 0, 0,      
        XNAME_acs_data,                     XNAME_dual_lmu_config,
        sizeof(pRawacs->dualLmu),           pRawacs->dualLmu,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_acs_data,                     XNAME_state,
        sizeof(pRawacs->state),             pRawacs->state,
        NOBLANKFILL, NOBITVALUE, 0, 0,      
        XNAME_acs_data,                     XNAME_cap_count,
        sizeof(pRawacs->capCount),          pRawacs->capCount,
        ZEROFILL, NOBITVALUE, 0, 0,      
        XNAME_acs_data,                     XNAME_result,
        sizeof(pRawacs->result),            pRawacs->result,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_acs_data,                     XNAME_error,
        sizeof(pRawacs->error),             pRawacs->error,
        NOBLANKFILL, NOBITVALUE, 0, 0,      
        XNAME_acs_data,                     XNAME_reason,
        sizeof(pRawacs->reason),            pRawacs->reason,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 acsDataElementCount = sizeof(acsDataXmlstruct) / 
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
    /* Unfortunately, HSC produces any error <reason> for the        */
    /* QUERY ACS command under the <acs_data> tag, and not under     */
    /* the <libreply><exceptions>.  Therefore, we continue trying    */
    /* to extract <acs_data> even when xapi_parse_header_trailer   */
    /* returns a bad RC.                                             */
    /*****************************************************************/

    /*****************************************************************/
    /* Count the number of <acs_data> entries.                       */
    /*****************************************************************/
    pFirstAcsDataXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                   pXmlparse->pHocXmlelem,
                                                   XNAME_acs_data);

    acsResponseCount = 0;

    if (pFirstAcsDataXmlelem != NULL)
    {
        pParentXmlelem = pFirstAcsDataXmlelem->pParentXmlelem;

        TRMSGI(TRCI_XAPI,
               "pFirstAcsDataXmlelem=%08X; pParentXmlelem=%08X\n",
               pFirstAcsDataXmlelem,
               pParentXmlelem);

        pNextAcsDataXmlelem = pFirstAcsDataXmlelem;

        while (pNextAcsDataXmlelem != NULL)
        {
            acsResponseCount++;

            pNextAcsDataXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                               pParentXmlelem,
                                                               pNextAcsDataXmlelem,
                                                               XNAME_acs_data);

#ifdef DEBUG

            if (pNextAcsDataXmlelem != NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextAcsDataXmlelem=%08X\n",
                       pNextAcsDataXmlelem);
            }

#endif

        }
    }

    TRMSGI(TRCI_XAPI,
           "acsResponseCount=%d, MAX_ID=%d\n",
           acsResponseCount,
           MAX_ID);

    /*****************************************************************/
    /* If no <acs_data> elements found, then return an error.        */
    /*****************************************************************/
    if (acsResponseCount == 0)
    {
        if (lastRC > 0)
        {
            return lastRC;
        }

        return STATUS_PROCESS_FAILURE;
    }

    /*****************************************************************/
    /* If <acs_data> elements found, then extract the data.          */
    /*****************************************************************/
    pNextAcsDataXmlelem = pFirstAcsDataXmlelem;
    acsRemainingCount = acsResponseCount;

    while (1)
    {
        xapi_query_init_resp(pXapireqe,
                             (char*) pQueryResponse,
                             qacsResponseSize);

        pQuery_Response = (QUERY_RESPONSE*) pQueryResponse;
        pQu_Acs_Response = (QU_ACS_RESPONSE*) &(pQuery_Response->status_response);
        pQu_Acs_Status = (QU_ACS_STATUS*) &(pQu_Acs_Response->acs_status);

        if (acsRemainingCount > MAX_ID)
        {
            acsPacketCount = MAX_ID;
        }
        else
        {
            acsPacketCount = acsRemainingCount;
        }

        TRMSGI(TRCI_XAPI,
               "At top of while; acsResponse=%d, acsRemaining=%d, "
               "acsPacket=%d, MAX_ID=%d\n",
               acsResponseCount,
               acsRemainingCount,
               acsPacketCount,
               MAX_ID);

        pQu_Acs_Response->acs_count = acsPacketCount;

        for (i = 0;
            i < acsPacketCount;
            i++, pQu_Acs_Status++)
        {
            memset(pRawacs, 0, sizeof(struct RAWACS));

            if (pNextAcsDataXmlelem == NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextAcsDataXmlelem=NULL at acsCount=%i\n",
                       (i+1));

                break;
            }

            FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                              pNextAcsDataXmlelem,
                                              &acsDataXmlstruct[0],
                                              acsDataElementCount);

            TRMEMI(TRCI_XAPI,
                   pRawacs, sizeof(struct RAWACS),
                   "RAWACS:\n");

            if (pRawacs->acs[0] > ' ')
            {
                FN_CONVERT_DIGITS_TO_FULLWORD(pRawacs->acs,
                                              sizeof(pRawacs->acs),
                                              &wkInt);

                pQu_Acs_Status->acs_id = (ACS) wkInt;
            }
            /*********************************************************/
            /* If we cannot find the ACS in the response,            */
            /* then get it from the request.                         */
            /*********************************************************/
            else if (acsRequestCount > 0)
            {
                pQu_Acs_Status->acs_id = pQu_Acs_Criteria->acs_id[i];
            }

            if (pRawacs->result[0] == 'F')
            {
                pQu_Acs_Status->status = STATUS_ACS_NOT_IN_LIBRARY;    
            }
            else
            {
                pQu_Acs_Status->status = STATUS_SUCCESS;    

                if (memcmp(pRawacs->status,
                           "CONNECTED",
                           3) == 0)
                {
                    if (memcmp(pRawacs->state,
                               "OFFLINE",
                               3) == 0)
                    {
                        pQu_Acs_Status->state = STATE_OFFLINE;    
                    }
                    else
                    {
                        pQu_Acs_Status->state = STATE_ONLINE;    
                    }
                }
                else
                {
                    pQu_Acs_Status->state = STATE_OFFLINE;    
                }

                if (pRawacs->freeCellCount[0] > ' ')
                {
                    FN_CONVERT_DIGITS_TO_FULLWORD(pRawacs->freeCellCount,
                                                  sizeof(pRawacs->freeCellCount),
                                                  &wkInt);

                    pQu_Acs_Status->freecells = wkInt;    
                }
            }

            pNextAcsDataXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                               pParentXmlelem,
                                                               pNextAcsDataXmlelem,
                                                               XNAME_acs_data);
        } /* for(i) */

        acsRemainingCount = acsRemainingCount - acsPacketCount;

        if (acsRemainingCount <= 0)
        {
            *pFinalAcsCount = acsPacketCount;

            break;
        }

        xapi_int_response(pXapireqe,
                          pQueryResponse,     
                          qacsResponseSize);  
    }

    return STATUS_SUCCESS;
}



