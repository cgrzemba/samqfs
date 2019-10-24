/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qcap.c                                      */
/** Description:    XAPI QUERY CAP processor.                        */
/**                                                                  */
/**                 Return CAP information and status.               */
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
#define QCAPERR_RESPONSE_SIZE (offsetof(QUERY_RESPONSE, status_response)) + \
                              (offsetof(QU_CAP_RESPONSE, cap_status[0])) 


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int convertQcapRequest(struct XAPICVT   *pXapicvt,
                              struct XAPIREQE  *pXapireqe,
                              char            **ptrXapiBuffer,
                              int              *pXapiBufferSize);

static int extractQcapResponse(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char            *pQueryResponse,
                               int              qcapResponseSize,
                               int             *pFinalCapCount,
                               struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qcap                                         */
/** Description:   XAPI QUERY CAP processor.                         */
/**                                                                  */
/** Return CAP information and status from the TapePlex server.      */
/**                                                                  */
/** The ACSAPI format QUERY CAP request is translated into an        */
/** XAPI XML format <query_cap> request; the XAPI XML request is     */
/** then transmitted to the server via TCP/IP;  The received         */
/** XAPI XML response is then translated into one or more            */
/** ACSAPI QUERY CAP responses.                                      */
/**                                                                  */
/** There will only be one individual XAPI <query_cap> request       */
/** issued for a single ACSAPI QUERY CAP request regardless of       */
/** the requested cap count (as the XAPI <query_cap> request allows  */
/** a <cap_list> to be specified).                                   */
/**                                                                  */
/** The QUERY CAP command is allowed to proceed even when            */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qcap"

extern int xapi_qcap(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;
    int                 qcapResponseSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_CAP_CRITERIA    *pQu_Cap_Criteria    = &(pQuery_Request->select_criteria.cap_criteria);

    int                 capCount            = pQu_Cap_Criteria->cap_count;
    int                 finalCapCount       = 0;

    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;
    QU_CAP_RESPONSE    *pQu_Cap_Response    = &(pQuery_Response->status_response.cap_response);
    QU_CAP_STATUS      *pQu_Cap_Status      = &(pQu_Cap_Response->cap_status[0]);

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         sizeof(QU_CAP_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY CAP request=%08X, size=%d, count=%d, "
           "MAX_ID=%d, QUERY CAP response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           capCount,
           MAX_ID,
           pQuery_Response,
           sizeof(QU_CAP_RESPONSE));

    lastRC = convertQcapRequest(pXapicvt,
                                pXapireqe,
                                &pXapiBuffer,
                                &xapiBufferSize);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from convertQcapRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QCAPERR_RESPONSE_SIZE,
                          lastRC);

        return lastRC;
    }

    if (capCount == 0)
    {
        qcapResponseSize = (char*) pQu_Cap_Status -
                           (char*) pQuery_Response +
                           ((sizeof(QU_CAP_STATUS)) * MAX_ID);
    }
    else
    {
        qcapResponseSize = (char*) pQu_Cap_Status -
                           (char*) pQuery_Response +
                           ((sizeof(QU_CAP_STATUS)) * capCount);
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
    /* Now generate the QUERY CAP ACSAPI response.                   */
    /*****************************************************************/
    if (queryRC == STATUS_SUCCESS)
    {
        lastRC = extractQcapResponse(pXapicvt,
                                     pXapireqe,
                                     (char*) pQuery_Response,
                                     qcapResponseSize,
                                     &finalCapCount,
                                     pXmlparse);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractQcapResponse; finalCapCount=%d\n",
               lastRC,
               finalCapCount);

        if (finalCapCount != capCount)
        {
            qcapResponseSize = (char*) pQu_Cap_Status -
                               (char*) pQuery_Response +
                               ((sizeof(QU_CAP_STATUS)) * finalCapCount);
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
                          QCAPERR_RESPONSE_SIZE,
                          queryRC);
    }
    else
    {
        xapi_fin_response(pXapireqe,
                          (char*) pQuery_Response,
                          qcapResponseSize);
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
/** Function Name: convertQcapRequest                                */
/** Description:   Build an XAPI <query_cap> request.                */
/**                                                                  */
/** Convert the ACSAPI format QUERY CAP request into an              */
/** XAPI XML format <query_cap> request.                             */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY CAP request consists of:                        */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY_REQUEST data consisting of:                           */
/**      i.   type                                                   */
/** 3.   QU_CAP_CRITERIA data consisting of:                         */
/**      i.   count (of cap(s) requested or 0 for "all")             */
/**      ii.  CAPID[count] cap data entries                          */
/**           a.   cap_id.lsm_id.acs                                 */
/**           b.   cap_id.lsm_id.lsm                                 */
/**           c    cap_id.cap                                        */
/**                                                                  */
/** NOTE: MAX_ID is 42.  So a max subset of 42 cap(s) can be         */
/** requested at once.  If 0 cap(s) is specified, then all           */
/** cap(s) are returned in possible multiple intermediate responses. */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_cap> request consists of:                    */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**                                                                  */
/**   If count = 0 (all caps are displayed):                         */
/**                                                                  */
/**     <query_cap>                                                  */
/**     </query_cap>                                                 */
/**                                                                  */
/**   If count = 1:                                                  */
/**                                                                  */
/**     <query_cap>                                                  */
/**       <cap_location_data>                                        */
/**         <acs>aa</acs>                                            */
/**         <lsm>ll</lsm>                                            */
/**         <cap>nn</cap>                                            */
/**       </cap_location_data>                                       */
/**     </query_cap>                                                 */
/**                                                                  */
/**   If count > 1:                                                  */
/**                                                                  */
/**     <query_cap>                                                  */
/**       <cap_list>                                                 */
/**         <cap_location_data>                                      */
/**           <acs>aa</acs>                                          */
/**           <lsm>ll</lsm>                                          */
/**           <cap>nn</cap>                                          */
/**         </cap_location_data>                                     */
/**         ...repeated <cap_location_data> entries                  */
/**       </cap_list>                                                */
/**     </query_cap>                                                 */
/**                                                                  */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "convertQcapRequest"

static int convertQcapRequest(struct XAPICVT  *pXapicvt,
                              struct XAPIREQE *pXapireqe,
                              char           **ptrXapiBuffer,
                              int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;
    int                 i;
    int                 capCount;
    char                capString[3];

    char               *pXapiRequest        = NULL;
    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_CAP_CRITERIA    *pQu_Cap_Criteria    = (QU_CAP_CRITERIA*) &(pQuery_Request->select_criteria);

    struct XMLPARSE    *pXmlparse; 
    struct XMLELEM     *pParentXmlelem;
    struct XMLELEM     *pCapListXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    capCount = pQu_Cap_Criteria->cap_count;

    TRMSGI(TRCI_XAPI,
           "Entered; capCount=%d\n",
           capCount);

    if (capCount < 0)
    {
        return STATUS_COUNT_TOO_SMALL;
    }

    if (capCount > MAX_ID)
    {
        return STATUS_COUNT_TOO_LARGE;
    }

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
                                      XNAME_query_cap,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    if (capCount > 0)
    {
        if (capCount > 1)
        {
            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pParentXmlelem,
                                              XNAME_cap_list,
                                              NULL,
                                              0);

            pCapListXmlelem = pXmlparse->pCurrXmlelem;
        }
        else
        {
            pCapListXmlelem = pParentXmlelem;
        }

        for (i = 0;
            i < capCount;
            i++)
        {
            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pCapListXmlelem,
                                              XNAME_cap_location_data,
                                              NULL,
                                              0);

            pParentXmlelem = pXmlparse->pCurrXmlelem;

            sprintf(capString,
                    "%.2d",
                    pQu_Cap_Criteria->cap_id[i].lsm_id.acs);

            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pParentXmlelem,
                                              XNAME_acs,
                                              capString,
                                              0);

            sprintf(capString,
                    "%.2d",
                    pQu_Cap_Criteria->cap_id[i].lsm_id.lsm);

            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pParentXmlelem,
                                              XNAME_lsm,
                                              capString,
                                              0);

            sprintf(capString,
                    "%.2d",
                    pQu_Cap_Criteria->cap_id[i].cap);

            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pParentXmlelem,
                                              XNAME_cap,
                                              capString,
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
/** Function Name: extractQcapResponse                               */
/** Description:   Extract the <cap_data> response.                  */
/**                                                                  */
/** Parse the response of the XAPI XML <query_cap> request and       */
/** update the appropriate fields of the ACSAPI QUERY CAP response.  */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <cal_data> responses consists of:                   */
/**==================================================================*/
/** <libreply>                                                       */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   <cap_data>                                                     */
/**     <cap_location_data>                                          */
/**       <acs>aa</acs>                                              */
/**       <lsm>ll</lsm>                                              */
/**       <cap>cc</cap>                                              */
/**     </cap_location_data>                                         */
/**     <host_name>hhhhhhhh</host_name>                              */
/**     <cell_count>nnnn</cell_count>                                */
/**     <priority>nn</priority>                                      */
/**     <mode>mmmmmmmmmmmmmmmmmmmm</mode>                            */
/**     <status>ssssssssssssssss</status>                            */
/**     <state>sssssssss</state>                                     */
/**     <part_id>pppppppp</part_id>                                  */
/**     <row_count>nnnn</row_count>                                  */
/**     <column_count>nnnn</column_count>                            */
/**     <magazine_count>nn</magazine_count>                          */
/**     <cells_per_magazine>nn</cells_per_magazine>                  */
/**     <cap_type>cccccccccccc</cap_type>                            */
/**     <cap_jobname_owner>jjjjjjjj</cap_jobname_onwer>              */
/**     <result>SUCCESS"|"FAILURE"</result>                          */
/**     <error>xxxxxxxx</error>                                      */
/**     <reason>ccc...ccc</reason>                                   */
/**   </cap_data>                                                    */
/**   ...repeated <cap_data> entries                                 */
/**   <exceptions>                                                   */
/**     <reason>ccc...ccc</reason>                                   */
/**     ...repeated <reason> entries                                 */
/**   </exceptions>                                                  */
/**   <uui_return_code>nnnn</uui_return_code>                        */
/**   <uui_reason_code>nnnn</uui_reason_code>                        */
/** </libreply>                                                      */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY CAP response consists of:                       */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 2.   QUERY_RESPONSE data consisting of                           */
/**      i.   type                                                   */
/**      ii.  QU_CAP_RESPONSE data consisting of:                    */
/**           a.   count                                             */
/**           b.   QU_CAP_STATUS[count] data entries consisting of:  */
/**                1.   CAPID consisting of:                         */
/**                     i.   cap_id.lsm_id.acs                       */
/**                     ii.  cap_id.lsm_id.lsm                       */
/**                     iii. cap_id.cap                              */
/**                2.   status                                       */
/**                3.   cap_priority                                 */
/**                4.   cap_size                                     */
/**                5.   cap_state                                    */
/**                6.   cap_mode                                     */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractQcapResponse"

static int extractQcapResponse(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char            *pQueryResponse,
                               int              qcapResponseSize,
                               int             *pFinalCapCount,
                               struct XMLPARSE *pXmlparse)
{
    int                 lastRC;
    int                 capResponseCount;
    int                 capRemainingCount;
    int                 capPacketCount;
    int                 i;
    int                 wkInt;

    struct XMLELEM     *pFirstCapDataXmlelem;
    struct XMLELEM     *pNextCapDataXmlelem;
    struct XMLELEM     *pCapLocationDataXmlelem;
    struct XMLELEM     *pParentXmlelem;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_CAP_CRITERIA    *pQu_Cap_Criteria    = (QU_CAP_CRITERIA*) &(pQuery_Request->select_criteria);
    int                 capRequestCount     = pQu_Cap_Criteria->cap_count;

    QUERY_RESPONSE     *pQuery_Response     = (QUERY_RESPONSE*) pQueryResponse;
    QU_CAP_RESPONSE    *pQu_Cap_Response    = (QU_CAP_RESPONSE*) &(pQuery_Response->status_response);
    QU_CAP_STATUS      *pQu_Cap_Status      = (QU_CAP_STATUS*) &(pQu_Cap_Response->cap_status);

    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;

    struct RAWCAP       rawcap;
    struct RAWCAP      *pRawcap             = &rawcap;

    struct XMLSTRUCT    capDataXmlstruct[]  =
    {
        XNAME_cap_data,                     XNAME_host_name,
        sizeof(pRawcap->hostName),          pRawcap->hostName,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cap_data,                     XNAME_cell_count,
        sizeof(pRawcap->cellCount),         pRawcap->cellCount,
        ZEROFILL, NOBITVALUE, 0, 0,
        XNAME_cap_data,                     XNAME_mode,
        sizeof(pRawcap->mode),              pRawcap->mode,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cap_data,                     XNAME_status,
        sizeof(pRawcap->status),            pRawcap->status,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cap_data,                     XNAME_state,
        sizeof(pRawcap->state),             pRawcap->state,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cap_data,                     XNAME_priority,
        sizeof(pRawcap->priority),          pRawcap->priority,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cap_data,                     XNAME_part_id,
        sizeof(pRawcap->partId),            pRawcap->partId,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cap_data,                     XNAME_row_count,
        sizeof(pRawcap->rowCount),          pRawcap->rowCount,
        ZEROFILL, NOBITVALUE, 0, 0,
        XNAME_cap_data,                     XNAME_column_count,
        sizeof(pRawcap->columnCount),       pRawcap->columnCount,
        ZEROFILL, NOBITVALUE, 0, 0,
        XNAME_cap_data,                     XNAME_magazine_count,
        sizeof(pRawcap->magazineCount),     pRawcap->magazineCount,
        ZEROFILL, NOBITVALUE, 0, 0,
        XNAME_cap_data,                     XNAME_cells_per_magazine,
        sizeof(pRawcap->cellsPerMagazine),  pRawcap->cellsPerMagazine,
        ZEROFILL, NOBITVALUE, 0, 0,
        XNAME_cap_data,                     XNAME_cap_type,
        sizeof(pRawcap->capType),           pRawcap->capType,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cap_data,                     XNAME_cap_jobname_owner,
        sizeof(pRawcap->capJobnameOwner),   pRawcap->capJobnameOwner,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cap_data,                     XNAME_result,
        sizeof(pRawcap->result),            pRawcap->result,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cap_data,                     XNAME_error,
        sizeof(pRawcap->error),             pRawcap->error,
        NOBLANKFILL, NOBITVALUE, 0, 0,      
        XNAME_cap_data,                     XNAME_reason,
        sizeof(pRawcap->reason),            pRawcap->reason,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 capDataElementCount = sizeof(capDataXmlstruct) / 
                                              sizeof(struct XMLSTRUCT);

    struct XMLSTRUCT    capLocDataXmlstruct[]  =
    {
        XNAME_cap_location_data,            XNAME_acs,
        sizeof(pRawcap->acs),               pRawcap->acs,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cap_location_data,            XNAME_lsm,
        sizeof(pRawcap->lsm),               pRawcap->lsm,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cap_location_data,            XNAME_cap,
        sizeof(pRawcap->cap),               pRawcap->cap,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 capLocDataElementCount = sizeof(capLocDataXmlstruct) / 
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
    /* QUERY CAP command under the <cap_data> tag, and not under     */
    /* the <libreply><exceptions>.  Therefore, we continue trying    */
    /* to extract <cap_data> even when xapi_parse_header_trailer     */
    /* returns a bad RC.                                             */
    /*****************************************************************/

    /*****************************************************************/
    /* Count the number of <cap_data> entries.                       */
    /*****************************************************************/
    pFirstCapDataXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                   pXmlparse->pHocXmlelem,
                                                   XNAME_cap_data);

    capResponseCount = 0;

    if (pFirstCapDataXmlelem != NULL)
    {
        pParentXmlelem = pFirstCapDataXmlelem->pParentXmlelem;

        TRMSGI(TRCI_XAPI,
               "pFirstCapDataXmlelem=%08X; pParentXmlelem=%08X\n",
               pFirstCapDataXmlelem,
               pParentXmlelem);

        pNextCapDataXmlelem = pFirstCapDataXmlelem;

        while (pNextCapDataXmlelem != NULL)
        {
            capResponseCount++;

            pNextCapDataXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                               pParentXmlelem,
                                                               pNextCapDataXmlelem,
                                                               XNAME_cap_data);

#ifdef DEBUG

            if (pNextCapDataXmlelem != NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextCapDataXmlelem=%08X\n",
                       pNextCapDataXmlelem);
            }

#endif

        }
    }

    TRMSGI(TRCI_XAPI,
           "capResponseCount=%d, MAX_ID=%d\n",
           capResponseCount,
           MAX_ID);

    /*****************************************************************/
    /* If no <cap_data> elements found, then return an error.        */
    /*****************************************************************/
    if (capResponseCount == 0)
    {
        if (lastRC > 0)
        {
            return lastRC;
        }

        return STATUS_PROCESS_FAILURE;
    }

    /*****************************************************************/
    /* If <cap_data> elements found, then extract the data.          */
    /*****************************************************************/
    pNextCapDataXmlelem = pFirstCapDataXmlelem;
    capRemainingCount = capResponseCount;

    while (1)
    {
        xapi_query_init_resp(pXapireqe,
                             (char*) pQueryResponse,
                             qcapResponseSize);

        pQuery_Response = (QUERY_RESPONSE*) pQueryResponse;
        pQu_Cap_Response = (QU_CAP_RESPONSE*) &(pQuery_Response->status_response);
        pQu_Cap_Status = (QU_CAP_STATUS*) &(pQu_Cap_Response->cap_status);

        if (capRemainingCount > MAX_ID)
        {
            capPacketCount = MAX_ID;
        }
        else
        {
            capPacketCount = capRemainingCount;
        }

        TRMSGI(TRCI_XAPI,
               "At top of while; capResponse=%d, capRemaining=%d, "
               "capPacket=%d, MAX_ID=%d\n",
               capResponseCount,
               capRemainingCount,
               capPacketCount,
               MAX_ID);

        pQu_Cap_Response->cap_count = capPacketCount;

        for (i = 0;
            i < capPacketCount;
            i++, pQu_Cap_Status++)
        {
            memset(pRawcap, 0, sizeof(struct RAWCAP));

            if (pNextCapDataXmlelem == NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextCapDataXmlelem=NULL at capCount=%i\n",
                       (i+1));

                break;
            }

            FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                              pNextCapDataXmlelem,
                                              &capDataXmlstruct[0],
                                              capDataElementCount);

            pCapLocationDataXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                              pNextCapDataXmlelem,
                                                              XNAME_cap_location_data);

            if (pCapLocationDataXmlelem != NULL)
            {
                FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                                  pCapLocationDataXmlelem,
                                                  &capLocDataXmlstruct[0],
                                                  capLocDataElementCount);
            }

            TRMEMI(TRCI_XAPI,
                   pRawcap, sizeof(struct RAWCAP),
                   "RAWCAP:\n");

            if (pRawcap->acs[0] > ' ')
            {
                FN_CONVERT_DIGITS_TO_FULLWORD(pRawcap->acs,
                                              sizeof(pRawcap->acs),
                                              &wkInt);

                pQu_Cap_Status->cap_id.lsm_id.acs = (ACS) wkInt;

                if (pRawcap->lsm[0] > ' ')
                {
                    FN_CONVERT_DIGITS_TO_FULLWORD(pRawcap->lsm,
                                                  sizeof(pRawcap->lsm),
                                                  &wkInt);

                    pQu_Cap_Status->cap_id.lsm_id.lsm = (LSM) wkInt;

                    if (pRawcap->cap[0] > ' ')
                    {
                        FN_CONVERT_DIGITS_TO_FULLWORD(pRawcap->cap,
                                                      sizeof(pRawcap->cap),
                                                      &wkInt);

                        pQu_Cap_Status->cap_id.cap = (CAP) wkInt;
                    }
                }
            }
            /*********************************************************/
            /* If we cannot find the ACS, LSM and CAP in the         */
            /* response, then get them from the request.             */
            /*********************************************************/
            else if (capRequestCount > 0)
            {
                pQu_Cap_Status->cap_id.lsm_id.acs = 
                pQu_Cap_Criteria->cap_id[i].lsm_id.acs;

                pQu_Cap_Status->cap_id.lsm_id.lsm = 
                pQu_Cap_Criteria->cap_id[i].lsm_id.lsm;

                pQu_Cap_Status->cap_id.cap = 
                pQu_Cap_Criteria->cap_id[i].cap;
            }

            if (pRawcap->result[0] == 'F')
            {
                pQu_Cap_Status->status = STATUS_CAP_NOT_IN_LIBRARY;    
            }
            else
            {
                pQu_Cap_Status->status = STATUS_SUCCESS;    

                if (memcmp(pRawcap->state,
                           "ONLINE",
                           3) == 0)
                {
                    pQu_Cap_Status->cap_state = STATE_ONLINE;    
                }
                else
                {
                    pQu_Cap_Status->cap_state = STATE_OFFLINE;    
                }

                if (memcmp(pRawcap->mode,
                           "MANUAL",
                           3) == 0)
                {
                    pQu_Cap_Status->cap_mode = CAP_MODE_MANUAL;    
                }
                else
                {
                    pQu_Cap_Status->cap_mode = CAP_MODE_AUTOMATIC;    
                }

                if (pRawcap->cellCount[0] > ' ')
                {
                    FN_CONVERT_DIGITS_TO_FULLWORD(pRawcap->cellCount,
                                                  sizeof(pRawcap->cellCount),
                                                  &wkInt);

                    pQu_Cap_Status->cap_size = wkInt;    
                }

                if (pRawcap->priority[0] > ' ')
                {
                    FN_CONVERT_DIGITS_TO_FULLWORD(pRawcap->priority,
                                                  sizeof(pRawcap->priority),
                                                  &wkInt);

                    pQu_Cap_Status->cap_priority = wkInt;    
                }
            }

            pNextCapDataXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                               pParentXmlelem,
                                                               pNextCapDataXmlelem,
                                                               XNAME_cap_data);
        } /* for(i) */

        capRemainingCount = capRemainingCount - capPacketCount;

        if (capRemainingCount <= 0)
        {
            *pFinalCapCount = capPacketCount;

            break;
        }

        xapi_int_response(pXapireqe,
                          pQueryResponse,     
                          qcapResponseSize);  
    }

    return STATUS_SUCCESS;
}



