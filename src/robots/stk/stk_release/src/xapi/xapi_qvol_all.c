/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qvol_all.c                                  */
/** Description:    XAPI client VOLRPT (query volume all) service.   */
/**                                                                  */
/**                 Return all tape volume information from the      */
/**                 TapePlex server.                                 */
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
#define QVOLERR_RESPONSE_SIZE (offsetof(QUERY_RESPONSE, status_response)) + \
                              (offsetof(QU_VOL_RESPONSE, volume_status[0])) 


/*********************************************************************/
/* Structures:                                                       */
/*********************************************************************/

/*********************************************************************/
/* QVOLCSV: The following maps the fixed UUITEXT lines returned in   */
/* the CSV response (VVVVV,MMMMMMMM,AA:LL:PP:RR:CC,AA:LL:PP:DD:ZZZZ).*/
/*********************************************************************/
struct QVOLCSV                
{
    char                volser[6];
    char                _f0;           /* comma                      */
    char                mediaName[8];
    char                _f1;           /* comma                      */
    char                cellAcs[2];
    char                _f2;           /* colon                      */
    char                cellLsm[2];
    char                _f3;           /* colon                      */
    char                cellPanel[2];
    char                _f4;           /* colon                      */
    char                cellRow[2];
    char                _f5;           /* colon                      */
    char                cellColumn[2];
    char                _f6;           /* comma                      */
    char                driveAcs[2];
    char                _f7;           /* colon                      */
    char                driveLsm[2];
    char                _f8;           /* colon                      */
    char                drivePanel[2];
    char                _f9;           /* colon                      */
    char                driveNumber[2];
    char                _f10;          /* colon                      */
    char                driveZone[4];
};


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void createQvolAllRequest(struct XAPICVT   *pXapicvt,
                                 struct XAPIREQE  *pXapireqe,
                                 char            **ptrXapiBuffer,
                                 int              *pXapiBufferSize);

static int extractQvolAllResponse(struct XAPICVT  *pXapicvt,
                                  struct XAPIREQE *pXapireqe,
                                  char            *pQueryResponse,
                                  int              qvolumeResponseSize,
                                  int             *pFinalVolCount,
                                  struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qvol_all                                     */
/** Description:   XAPI client VOLRPT (query volume all) service.    */
/**                                                                  */
/** Return ALL tape volume information from the TapePlex server.     */
/**                                                                  */
/** Build an XAPI XML format <query_volume_info>                     */
/** <query_all_volume_request>YES request to return all TapePlex     */
/** volumes.  Then setup the XAPI <query_volume_info> request        */
/** to also CSV fixed format output; The XAPI XML request is then    */
/** transmitted to the server via TCP/IP;  The received XAPI CSV     */
/** response is then translated into one or more ACSAPI QUERY VOLUME */
/** responses.                                                       */
/**                                                                  */
/** The XAPI client VOLRPT (query volume all) service is allowed     */
/** to proceed even when the XAPI client is in the IDLE state.       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qvol_all"

extern int xapi_qvol_all(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;
    int                 qvolumeResponseSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_VOL_CRITERIA    *pQu_Vol_Criteria    = &(pQuery_Request->select_criteria.vol_criteria);

    int                 volCount            = MAX_ID;
    int                 finalVolCount       = 0;

    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;
    QU_VOL_RESPONSE    *pQu_Vol_Response    = &(pQuery_Response->status_response.volume_response);
    QU_VOL_STATUS      *pQu_Vol_Status      = &(pQu_Vol_Response->volume_status[0]);

    qvolumeResponseSize = (char*) pQu_Vol_Status -
                          (char*) pQuery_Response +
                          ((sizeof(QU_VOL_STATUS)) * MAX_ID);

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         sizeof(QUERY_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY VOLUME ALL request=%08X, size=%d, "
           "count=%d, MAX_ID=%d, "
           "QUERY VOLUME ALL response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           volCount,
           MAX_ID,
           pQuery_Response,
           qvolumeResponseSize);

    createQvolAllRequest(pXapicvt,
                         pXapireqe,
                         &pXapiBuffer,
                         &xapiBufferSize);

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
    /* Now generate the QUERY VOLUME ACSAPI response.                */
    /*****************************************************************/
    if (queryRC == STATUS_SUCCESS)
    {
        lastRC = extractQvolAllResponse(pXapicvt,
                                        pXapireqe,
                                        (char*) pQuery_Response,
                                        qvolumeResponseSize,
                                        &finalVolCount,
                                        pXmlparse);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractQvolAllResponse; finalVolCount=%d\n",
               lastRC,
               finalVolCount);

        if (finalVolCount != volCount)
        {
            qvolumeResponseSize = (char*) pQu_Vol_Status -
                                  (char*) pQuery_Response +
                                  ((sizeof(QU_VOL_STATUS)) * finalVolCount);
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
                          QVOLERR_RESPONSE_SIZE,
                          queryRC);
    }
    else
    {
        xapi_fin_response(pXapireqe,
                          (char*) pQuery_Response,
                          qvolumeResponseSize);
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
/** Function Name: createQvolAllRequest                              */
/** Description:   Build an XAPI <query_volume_info> request.        */
/**                                                                  */
/** Build the XAPI XML format <query_volume_info> with               */
/** <query_all_volume_request>YES specified to return all volumes.   */
/** Additionally, request fixed format CSV output to limit the       */
/** size and parse complexity of the <query_volume_info> response.   */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY VOLUME request consists of:                     */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY_REQUEST data consisting of:                           */
/**      i.   type                                                   */
/** 3.   QU_VOL_CRITERIA data consisting of:                         */
/**      i.   count (of volumes requested or 0 for "all")            */
/**      ii.  VOLID[MAX_ID] volume entries consisting of:            */
/**           a.   external_label (6 character volser)               */
/**                                                                  */
/** NOTE: MAX_ID is 42, but has been specified as 0: If 0 volumes    */
/** are requested, then a <query_all_volume> request is generated,   */
/** and all volumes are returned in multiple imtermediate responses. */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML QUERY_VOLUME_INFO ALL request consists of:          */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**                                                                  */
/**   <csv_break>volume_data<csv_break>                              */
/**   <csv_fields>volser,media,home_cell,drive_location_id,          */
/**     </csv_fields>                                                */
/**   <csv_fixed_flag>Y</csv_fixed_flag>                             */
/**   <csv_notitle_flag>Y</csv_notitle_flag>                         */
/**                                                                  */
/**   <command>                                                      */
/**     <query_volume_info>                                          */
/**       <query_all_volume_request>YES                              */
/**         <query_all_volume_request>                               */
/**     </query_volume_info>                                         */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** NOTES:                                                           */
/** (1) csv_field "result" is not specified because the XAPI         */
/**     query_all_volume_request will only return volumes in the     */
/**     CDS.                                                         */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "createQvolAllRequest"

static void createQvolAllRequest(struct XAPICVT  *pXapicvt,
                                 struct XAPIREQE *pXapireqe,
                                 char           **ptrXapiBuffer,
                                 int             *pXapiBufferSize)
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
                                      "volser,media,home_cell,drive_location_id",
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_command,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_query_volume_info,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_query_all_volume_request,
                                      XCONTENT_YES,
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
/** Function Name: extractQvolAllResponse                            */
/** Description:   Extract the <query_volume_info> CSV response.     */
/**                                                                  */
/** Parse the CSV response from the <query_volume_info> XAPI         */
/** request and update the appropriate fields of the ACSAPI          */
/** QUERY VOLUME response.  Each <uui_text> tag represents a         */
/** single volser.                                                   */
/**                                                                  */
/**==================================================================*/
/** The XAPI CSV <query_volume_info> responses consists of:          */
/**==================================================================*/
/** <libreply>                                                       */
/**   <uui_line_type>V</uui_line_type>                               */
/**   <uui_text>VVVVV,MM..MM,AA:LL:PP:RR:CC,AA:LL:PP:DD:ZZZZ (FIXED) */
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
/**==================================================================*/
/** The ACSAPI QUERY VOLUME response consists of:                    */
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
/**      ii.  QU_VOL_RESPONSE data consisting of:                    */
/**           a.   count                                             */
/**           b.   QU_VOL_STATUS[count] data entries consisting of:  */
/**                1.   VOLID consisting of:                         */
/**                     i.    external_label (6 character volser)    */
/**                2.   media_type                                   */
/**                3.   location_type                                */
/**                4.   CELLID consisting of:                        */
/**                     ii.   panel_id.lsmid.acs                     */
/**                     ii.   panel_id.lsmid.lsm                     */
/**                     iii.  panel_id.panel.row                     */
/**                     iv.   panel_id.panel.col                     */
/**                     v.    column                                 */
/**                5.   status                                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractQvolAllResponse"

static int extractQvolAllResponse(struct XAPICVT  *pXapicvt,
                                  struct XAPIREQE *pXapireqe,
                                  char            *pQueryResponse,
                                  int              qvolumeResponseSize,
                                  int             *pFinalVolCount,
                                  struct XMLPARSE *pXmlparse)
{
    int                 lastRC;
    int                 volResponseCount;
    int                 volRemainingCount;
    int                 volPacketCount;
    int                 locInt;
    int                 i;

    char                mediaNameString[XAPI_MEDIA_NAME_SIZE + 1];

    struct XAPIMEDIA   *pXapimedia;
    struct XMLELEM     *pFirstUuiTextXmlelem;
    struct XMLELEM     *pNextUuiTextXmlelem;
    struct XMLELEM     *pParentXmlelem;
    struct QVOLCSV     *pQvolcsv;

    QUERY_RESPONSE     *pQuery_Response     = (QUERY_RESPONSE*) pQueryResponse;
    QU_VOL_RESPONSE    *pQu_Vol_Response    = (QU_VOL_RESPONSE*) &(pQuery_Response->status_response);
    QU_VOL_STATUS      *pQu_Vol_Status      = (QU_VOL_STATUS*) &(pQu_Vol_Response->volume_status);

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
           "volResponseCount=%d, MAX_ID=%d\n",
           volResponseCount,
           MAX_ID);

    /*****************************************************************/
    /* If no <uui_text> elements found, then return an error.        */
    /*****************************************************************/
    if (volResponseCount == 0)
    {
        return STATUS_NI_FAILURE;
    }

    /*****************************************************************/
    /* If <uui_text> elements found, then extract the data.          */
    /*****************************************************************/
    pNextUuiTextXmlelem = pFirstUuiTextXmlelem;
    volRemainingCount = volResponseCount;

    while (1)
    {
        xapi_query_init_resp(pXapireqe,
                             (char*) pQueryResponse,   
                             qvolumeResponseSize);

        pQuery_Response = (QUERY_RESPONSE*) pQueryResponse;
        pQu_Vol_Response = (QU_VOL_RESPONSE*) &(pQuery_Response->status_response);
        pQu_Vol_Status = (QU_VOL_STATUS*) &(pQu_Vol_Response->volume_status);

        if (volRemainingCount > MAX_ID)
        {
            volPacketCount = MAX_ID;
        }
        else
        {
            volPacketCount = volRemainingCount;
        }

        TRMSGI(TRCI_XAPI,
               "At top of while; volResponse=%d, volRemaining=%d, "
               "volPacket=%d, MAX_ID=%d\n",
               volResponseCount,
               volRemainingCount,
               volPacketCount,
               MAX_ID);

        pQu_Vol_Response->volume_count = volPacketCount;

        for (i = 0;
            i < volPacketCount;
            i++, pQu_Vol_Status++)
        {
            if (pNextUuiTextXmlelem == NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextUuiTextXmlelem=NULL at volCount=%i\n",
                       (i+1));

                break;
            }

            pQvolcsv = (struct QVOLCSV*) pNextUuiTextXmlelem->pContent;

#ifdef DEBUG

            TRMEM(pQvolcsv, sizeof(struct QVOLCSV), 
                  "Next pQu_Vol_Status=%08X, driveAcs=%08X; QVOLCSV:\n",
                  &pQu_Vol_Status,
                  pQvolcsv->driveAcs);

#endif

            pQu_Vol_Status->status = STATUS_SUCCESS;

            memcpy(pQu_Vol_Status->vol_id.external_label,
                   pQvolcsv->volser,
                   sizeof(pQvolcsv->volser));

            if (pQvolcsv->cellAcs[0] > ' ')
            {
                pQu_Vol_Status->location_type = LOCATION_CELL;

                FN_CONVERT_DIGITS_TO_FULLWORD(pQvolcsv->cellAcs,
                                              sizeof(pQvolcsv->cellAcs),
                                              &locInt);

                pQu_Vol_Status->location.cell_id.panel_id.lsm_id.acs = (ACS) locInt;

                if (pQvolcsv->cellLsm[0] > ' ')
                {
                    FN_CONVERT_DIGITS_TO_FULLWORD(pQvolcsv->cellLsm,
                                                  sizeof(pQvolcsv->cellLsm),
                                                  &locInt);

                    pQu_Vol_Status->location.cell_id.panel_id.lsm_id.lsm = (ACS) locInt;

                    if (pQvolcsv->cellPanel[0] > ' ')
                    {
                        FN_CONVERT_DIGITS_TO_FULLWORD(pQvolcsv->cellPanel,
                                                      sizeof(pQvolcsv->cellPanel),
                                                      &locInt);

                        pQu_Vol_Status->location.cell_id.panel_id.panel = (PANEL) locInt;

                        if (pQvolcsv->cellRow[0] > ' ')
                        {
                            FN_CONVERT_DIGITS_TO_FULLWORD(pQvolcsv->cellRow,
                                                          sizeof(pQvolcsv->cellRow),
                                                          &locInt);

                            pQu_Vol_Status->location.cell_id.row = (ROW) locInt;

                            if (pQvolcsv->cellColumn[0] > ' ')
                            {
                                FN_CONVERT_DIGITS_TO_FULLWORD(pQvolcsv->cellColumn,
                                                              sizeof(pQvolcsv->cellColumn),
                                                              &locInt);

                                pQu_Vol_Status->location.cell_id.col = (COL) locInt;
                            }
                        }
                    }
                }
            }
            else if (pQvolcsv->driveAcs[0] > ' ')
            {
                pQu_Vol_Status->location_type = LOCATION_DRIVE;

                FN_CONVERT_DIGITS_TO_FULLWORD(pQvolcsv->driveAcs,
                                              sizeof(pQvolcsv->driveAcs),
                                              &locInt);

                pQu_Vol_Status->location.drive_id.panel_id.lsm_id.acs = (ACS) locInt;

                if (pQvolcsv->driveLsm[0] > ' ')
                {
                    FN_CONVERT_DIGITS_TO_FULLWORD(pQvolcsv->driveLsm,
                                                  sizeof(pQvolcsv->driveLsm),
                                                  &locInt);

                    pQu_Vol_Status->location.drive_id.panel_id.lsm_id.lsm = (ACS) locInt;

                    if (pQvolcsv->drivePanel[0] > ' ')
                    {
                        FN_CONVERT_DIGITS_TO_FULLWORD(pQvolcsv->drivePanel,
                                                      sizeof(pQvolcsv->drivePanel),
                                                      &locInt);

                        pQu_Vol_Status->location.drive_id.panel_id.panel = (PANEL) locInt;

                        if (pQvolcsv->driveNumber[0] > ' ')
                        {
                            FN_CONVERT_DIGITS_TO_FULLWORD(pQvolcsv->driveNumber,
                                                          sizeof(pQvolcsv->driveNumber),
                                                          &locInt);

                            pQu_Vol_Status->location.drive_id.drive = (DRIVE) locInt;
                        }
                    }
                }
            }

            if (pQvolcsv->mediaName[0] >= ' ')
            {
                STRIP_TRAILING_BLANKS(pQvolcsv->mediaName,
                                      mediaNameString,
                                      sizeof(pQvolcsv->mediaName));

                pXapimedia = xapi_media_search_name(pXapicvt,
                                                    pXapireqe,
                                                    mediaNameString);

                if (pXapimedia != NULL)
                {
                    pQu_Vol_Status->media_type = 
                    (MEDIA_TYPE) pXapimedia->acsapiMediaType;
                }
                else
                {
                    pQu_Vol_Status->media_type = ANY_MEDIA_TYPE;
                }
            }

            pNextUuiTextXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                               pParentXmlelem,
                                                               pNextUuiTextXmlelem,
                                                               XNAME_uui_text);
        } /* for (i) */

        volRemainingCount = volRemainingCount - volPacketCount;

        if (volRemainingCount <= 0)
        {
            *pFinalVolCount = volPacketCount;

            break;
        }

        xapi_int_response(pXapireqe,
                          pQueryResponse,     
                          qvolumeResponseSize);  
    }

    return STATUS_SUCCESS;
}



