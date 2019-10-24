/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qserver.c                                   */
/** Description:    XAPI QUERY SERVER processor.                     */
/**                                                                  */
/**                 Return TapePlex server status.                   */
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
/* Constants:                                                        */
/*********************************************************************/
#define QUERY_SEND_TIMEOUT         5   /* TIMEOUT values in seconds  */       
#define QUERY_RECV_TIMEOUT_1ST     300
#define QUERY_RECV_TIMEOUT_NON1ST  600


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int convertQserverRequest(struct XAPICVT   *pXapicvt,
                                 struct XAPIREQE  *pXapireqe,
                                 char            **ptrXapiBuffer,
                                 int              *pXapiBufferSize);

static int extractQserverResponse(struct XAPICVT  *pXapicvt,
                                  struct XAPIREQE *pXapireqe,
                                  char            *pQserverResponse,
                                  int              qserverResponseSize,
                                  struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qserver                                      */
/** Description:   The XAPI QUERY SERVER processor.                  */
/**                                                                  */
/** Return TapePlex server information and status.                   */
/**                                                                  */
/** The ACSAPI format QUERY SERVER request is translated into an     */
/** XAPI XML format <query_server> request; the XAPI XML request     */
/** is then transmitted to the server via TCP/IP;  The received      */
/** XAPI XML response is then translated into the                    */
/** ACSAPI QUERY SERVER response.                                    */
/**                                                                  */
/** In addition to the XAPI XML format <query_server> request, the   */
/** XAPI client QUERY SERVER free cell count service (xapi_qfree)    */
/** is called which results in a XAPI CSV format <query_acs>         */
/** request being issued.                                            */
/**                                                                  */
/** The QUERY SERVER command is allowed to proceed even when         */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qserver"

extern int xapi_qserver(struct XAPICVT  *pXapicvt,
                        struct XAPIREQE *pXapireqe)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;
    int                 qserverResponseSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;
    QU_SRV_RESPONSE    *pQu_Srv_Response    = &(pQuery_Response->status_response.server_response);
    QU_SRV_STATUS      *pQu_Srv_Status      = &(pQu_Srv_Response->server_status);

    qserverResponseSize = (char*) pQu_Srv_Status -
                          (char*) pQuery_Response +
                          sizeof(QU_SRV_STATUS);

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY SERVER request=%08X, size=%d, "
           "QUERY SERVER response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pQuery_Response,
           qserverResponseSize);

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         qserverResponseSize);

    lastRC = convertQserverRequest(pXapicvt,
                                   pXapireqe,
                                   &pXapiBuffer,
                                   &xapiBufferSize);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from convertQserverRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          qserverResponseSize,
                          lastRC);

        return lastRC;
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
    /* Now generate the QUERY SERVER ACSAPI response.                */
    /*****************************************************************/
    if (queryRC == STATUS_SUCCESS)
    {
        lastRC = extractQserverResponse(pXapicvt,
                                        pXapireqe,
                                        (char*) pQuery_Response,
                                        qserverResponseSize,
                                        pXmlparse);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractQserverResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            queryRC = lastRC;
        }
    }

    if (queryRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          qserverResponseSize,
                          queryRC);
    }
    else
    {
        xapi_fin_response(pXapireqe,
                          (char*) pQuery_Response,
                          qserverResponseSize);
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
/** Function Name: convertQserverRequest                             */
/** Description:   Build an XAPI <query_server> request.             */
/**                                                                  */
/** Convert the ACSAPI format QUERY SERVER request into an           */
/** XAPI XML format <query_server> request.                          */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY SERVER request consists of:                     */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY_REQUEST data consisting of                            */
/**      i.   type                                                   */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_server> request consists of:                 */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <query_server>                                               */
/**     </query_server>                                              */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "convertQserverRequest"

static int convertQserverRequest(struct XAPICVT  *pXapicvt,
                                 struct XAPIREQE *pXapireqe,
                                 char           **ptrXapiBuffer,
                                 int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;
    char               *pXapiRequest        = NULL;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    struct XMLPARSE    *pXmlparse; 
    struct XMLELEM     *pCommandXmlelem;
    struct XMLRAWIN    *pXmlrawin;

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

    pCommandXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pCommandXmlelem,
                                      XNAME_query_server,
                                      NULL,
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

    return STATUS_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: extractQserverResponse                            */
/** Description:   Extract the <display_server_request> response.    */
/**                                                                  */
/** Parse the response of the XAPI XML <query_server> request and    */
/** update the appropriate fields of the ACSAPI QUERY SERVER         */
/** response.                                                        */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <display_server_request> response consists of:      */
/**==================================================================*/
/** <libreply>                                                       */
/**   <display_server_request>                                       */
/**     <header>                                                     */
/**       header element children                                    */
/**     </header>                                                    */
/**     <exceptions>                                                 */
/**       ... When non-zero return codes                             */
/**     </exceptions>                                                */
/**     <server_data>                                                */
/**       <server_type>HSC</server_type>                             */
/**       <service_level>"BASE"|"FULL"</service_level>               */
/**       <subsystem_name>nnnn</subsystem_name>                      */
/**       <termination_in_progress>                                  */
/**         "YES|"NO"</terimnation_in_progress>                      */
/**       <vtcs_available>"YES"|"NO"</vtcs_available>                */
/**       <advanced_management>"YES"|"NO"</advanced_management>      */
/**       <subsystem_start_date>yyyymondd</subsystem_start_data>     */
/**       <subsystem_start_time>hh:mm:ss</subsystem_start_time>      */
/**     </server_data>                                               */
/**     <dsn_data>                                                   */
/**       <dataset_type>                                             */
/**         "PRIMARY"|"SECONDARY"|"STANDBY"</dataset_type>           */
/**       <dsname>ccc...ccc</dsname>                                 */
/**       <active>"YES"|"NO"<//active>                               */
/**     </dsn_data>                                                  */
/**     <dsn_data>                                                   */
/**       ... repeated for Secondary and Standby                     */
/**     </dsn_data>                                                  */
/**   </display_server_request>                                      */
/**   <uui_return_code>nnnn</uui_return_code>                        */
/**   <uui_reason_code>nnnn</uui_reason_code>                        */
/** </libreply>                                                      */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY SERVER response consists of:                    */
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
/**      ii.  QU_SRV_STATUS data consisting of:                      */
/**           a.   state                                             */
/**           b.   freecells                                         */
/**           c.   REQ_SUMMARY                                       */
/**                which is an array of:                             */
/**                requests[MAX_COMMANDS] [MAX_DISPOSITIONS]         */
/**                                                                  */
/** MAX_COMMANDS is 5 (for AUDIT, MOUNT, DISMOUNT, ENTER, and EJECT).*/
/** MAX_DISPOSITIONS is 2 (for CURRENT and PENDING).                 */
/**                                                                  */
/** NOTE: Currently, the XAPI QUERY SERVER response does not         */
/** return any freecell, or request summary information,             */
/** so those portions of the ACSAPI QUERY SERVER FINAL RESPONSE      */
/** will be 0(s).                                                    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractQserverResponse"

static int extractQserverResponse(struct XAPICVT  *pXapicvt,
                                  struct XAPIREQE *pXapireqe,
                                  char            *pQserverResponse,
                                  int              qserverResponseSize,
                                  struct XMLPARSE *pXmlparse)
{
    int                 lastRC;
    int                 freeCellCount;
    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;
    struct RAWCOMMON   *pRawcommon          = &(pXapicommon->rawcommon);

    struct RAWSERVER    rawserver;
    struct RAWSERVER   *pRawserver          = &rawserver;

    QUERY_RESPONSE     *pQuery_Response     = (QUERY_RESPONSE*) pQserverResponse;
    QU_SRV_RESPONSE    *pQu_Srv_Response    = (QU_SRV_RESPONSE*) &(pQuery_Response->status_response);
    QU_SRV_STATUS      *pQu_Srv_Status      = (QU_SRV_STATUS*) &(pQu_Srv_Response->server_status);

    struct XMLSTRUCT    serverDataXmlstruct[] =
    {
        XNAME_server_data,                  XNAME_server_type,
        sizeof(pRawserver->serverType),     pRawserver->serverType,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_server_data,                  XNAME_service_level,
        sizeof(pRawserver->serviceLevel),   pRawserver->serviceLevel,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_server_data,                  XNAME_subsystem_name,
        sizeof(pRawserver->subsystemName),  pRawserver->subsystemName,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_server_data,                  XNAME_subsystem_start_date,
        sizeof(pRawserver->startDate),      pRawserver->startDate,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_server_data,                  XNAME_subsystem_start_time,
        sizeof(pRawserver->startTime),      pRawserver->startTime,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_server_data,                  XNAME_termination_in_progress,
        sizeof(pRawserver->termInProgress), pRawserver->termInProgress,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_server_data,                  XNAME_vtcs_available,
        sizeof(pRawserver->vtcsAvailable),  pRawserver->vtcsAvailable,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_server_data,                  XNAME_advanced_management,
        sizeof(pRawserver->advManagement),  pRawserver->advManagement,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 serverDataElementCount = sizeof(serverDataXmlstruct) / 
                                                 sizeof(struct XMLSTRUCT);

    memset((char*) pRawserver, 0, sizeof(struct RAWSERVER));

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

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    FN_MOVE_XML_ELEMS_TO_STRUCT(pXmlparse,
                                &serverDataXmlstruct[0],
                                serverDataElementCount);

    TRMEMI(TRCI_XAPI,
           pRawserver, sizeof(struct RAWSERVER), 
           "RAWSERVER:");

    pQu_Srv_Status->state = STATE_IDLE;

    if (memcmp(XCONTENT_FULL, 
               pRawserver->serviceLevel,
               strlen(XCONTENT_FULL)) == 0)
    {
        pQu_Srv_Status->state = STATE_RUN;
    }

    if (toupper(pRawserver->termInProgress[0]) == 'Y')
    {
        pQu_Srv_Status->state = STATE_IDLE_PENDING;
    }

    lastRC = xapi_qfree(pXapicvt,
                        pXapireqe,
                        &freeCellCount);

    pQu_Srv_Status->freecells = (FREECELLS) freeCellCount;

    return STATUS_SUCCESS;
}



