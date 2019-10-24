/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_enter.c                                     */
/** Description:    XAPI ENTER processor.                            */
/**                                                                  */
/**                 Place the specified CAP into enter mode.         */
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
#define ENTER_SEND_TIMEOUT         5   /* TIMEOUT values in seconds  */       
#define ENTER_RECV_TIMEOUT_1ST     600
#define ENTER_RECV_TIMEOUT_NON1ST  7200
#define ENTERERR_RESPONSE_SIZE (offsetof(ENTER_RESPONSE, volume_status[0]))


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void initEnterResponse(struct XAPIREQE *pXapireqe,
                              char            *pEnterResponse,
                              int              enterResponseSize);

static int convertEnterRequest(struct XAPICVT   *pXapicvt,
                               struct XAPIREQE  *pXapireqe,
                               char            **ptrXapiBuffer,
                               int              *pXapiBufferSize);

static int extractEnterResponse(struct XAPICVT  *pXapicvt,
                                struct XAPIREQE *pXapireqe,
                                char            *pEnterResponse,
                                struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_enter                                        */
/** Description:   The XAPI ENTER processor.                         */
/**                                                                  */
/** Place the specified CAP into enter mode.                         */
/**                                                                  */
/** The ACSAPI format ENTER request is translated into an            */
/** XAPI XML format <enter> request; the XAPI XML request is then    */
/** transmitted to the server via TCP/IP;  The received XAPI XML     */
/** response is then translated into one or more ACSAPI ENTER        */
/** responses.                                                       */
/**                                                                  */
/** The ENTER command is NOT allowed to proceed when the             */
/** XAPI client is in the IDLE state.                                */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_enter"

extern int xapi_enter(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    int                 enterRC             = STATUS_SUCCESS;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    ENTER_REQUEST      *pEnter_Request      = (ENTER_REQUEST*) pXapireqe->pAcsapiBuffer;

    ENTER_RESPONSE      wkEnter_Response;
    ENTER_RESPONSE     *pEnter_Response     = &wkEnter_Response;

    TRMSGI(TRCI_XAPI,
           "Entered; ENTER request=%08X, size=%d, "
           "MAX_ID=%d, ENTER response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           MAX_ID,
           pEnter_Response,
           sizeof(ENTER_RESPONSE));

    initEnterResponse(pXapireqe,
                      (char*) pEnter_Response,
                      sizeof(ENTER_RESPONSE));

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pEnter_Response,
                            ENTERERR_RESPONSE_SIZE);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    lastRC = convertEnterRequest(pXapicvt,
                                 pXapireqe,
                                 &pXapiBuffer,
                                 &xapiBufferSize);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from convertEnterRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pEnter_Response,
                          ENTERERR_RESPONSE_SIZE,
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
                      ENTER_SEND_TIMEOUT,        
                      ENTER_RECV_TIMEOUT_1ST,    
                      ENTER_RECV_TIMEOUT_NON1ST,
                      (void**) &pXmlparse);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from xapi_tcp(pBuffer=%08X, size=%d, pXmlparse=%08X)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize,
           pXmlparse);

    if (lastRC != STATUS_SUCCESS)
    {
        enterRC = lastRC;
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
    /* Now generate the ENTER ACSAPI response.                       */
    /*****************************************************************/
    if (enterRC == STATUS_SUCCESS)
    {
        lastRC = extractEnterResponse(pXapicvt,
                                      pXapireqe,
                                      (char*) pEnter_Response,
                                      pXmlparse);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractEnterResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            enterRC = lastRC;
        }
    }

    if (enterRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pEnter_Response,
                          ENTERERR_RESPONSE_SIZE,
                          enterRC);
    }

    /*****************************************************************/
    /* When done, free the XMLPARSE and associated structures.       */
    /*****************************************************************/
    if (pXmlparse != NULL)
    {
        FN_FREE_HIERARCHY_STORAGE(pXmlparse);
    }

    return enterRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: initEnterResponse                                 */
/** Description:   Initialize the ACSAPI ENTER response.             */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI ENTER response consists of:                           */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   ENTER data consisting of                                    */
/**      i.   CAPID consisting of:                                   */
/**           a.   cap_id.lsm_id.acs                                 */
/**           b.   cap_id.lsm_id.lsm                                 */
/**           c    cap_id.cap                                        */
/**      ii.  count of volumes entered                               */
/**      iii. VOLUME_STATUS[count] data entries consisting of:       */
/**           a.   VOLID consisting of:                              */
/**                1.    external_label (6 character volser)         */
/**           b.   RESPONSE_STATUS consisting of:                    */
/**                1.   status                                       */
/**                2.   type                                         */
/**                3.   identifier                                   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "initEnterResponse "

static void initEnterResponse(struct XAPIREQE *pXapireqe,
                              char            *pEnterResponse,
                              int              enterResponseSize)
{
    ENTER_REQUEST      *pEnter_Request      = (ENTER_REQUEST*) pXapireqe->pAcsapiBuffer;
    ENTER_RESPONSE     *pEnter_Response     = (ENTER_RESPONSE*) pEnterResponse;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Initialize ENTER response.                                    */
    /*****************************************************************/
    memset((char*) pEnter_Response, 0, enterResponseSize);

    memcpy((char*) &(pEnter_Response->request_header),
           (char*) &(pEnter_Request->request_header),
           sizeof(REQUEST_HEADER));

    pEnter_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pEnter_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pEnter_Response->message_status.status = STATUS_SUCCESS;
    pEnter_Response->message_status.type = TYPE_NONE;

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: convertEnterRequest                               */
/** Description:   Build an XAPI <enter> request.                    */
/**                                                                  */
/** Convert the ACSAPI format ENTER request into an                  */
/** XAPI XML format <enter> request.                                 */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI ENTER request consists of:                            */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   CAPID[count] cap data entries                               */
/**      i.   cap_id.lsm_id.acs                                      */
/**      ii.  cap_id.lsm_id.lsm                                      */
/**      iii. cap_id.cap                                             */
/**                                                                  */
/** NOTE: The MESSAGE_HEADER.extended_options CONTINUOUS bit         */
/** determines if CAP is to be placed in continuous mode for the     */
/** enter.  Currently this options is ignored by the XAPI: whatever  */
/** MODE the specified CAP is currently set, determines the MODE     */
/** of the ENTER.                                                    */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <enter> request consists of:                        */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <enter>                                                      */
/**       <cap_location_data>                                        */
/**         <acs>aa</acs>                                            */
/**         <lsm>ll</lsm>                                            */
/**         <cap>nn</cap>                                            */
/**       </cap_location_data>                                       */
/**       <scratch>NO|YES</scratch>                    (N/A for CDK) */
/**     </enter>                                                     */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "convertEnterRequest"

static int convertEnterRequest(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char           **ptrXapiBuffer,
                               int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;
    char                capString[3];

    char               *pXapiRequest        = NULL;
    ENTER_REQUEST      *pEnter_Request      = (ENTER_REQUEST*) pXapireqe->pAcsapiBuffer;

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
                                      XNAME_enter,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_cap_location_data,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    sprintf(capString,
            "%.2d",
            pEnter_Request->cap_id.lsm_id.acs);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_acs,
                                      capString,
                                      0);

    sprintf(capString,
            "%.2d",
            pEnter_Request->cap_id.lsm_id.lsm);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_lsm,
                                      capString,
                                      0);

    sprintf(capString,
            "%.2d",
            pEnter_Request->cap_id.cap);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_cap,
                                      capString,
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
/** Function Name: extractEnterResponse                              */
/** Description:   Extract the <enter_data> response.                */
/**                                                                  */
/** Parse the response of the XAPI XML <enter >                      */
/** request and update the appropriate fields of the                 */
/** ACSAPI ENTER response.                                           */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <enter_data> responses consists of:                 */
/**==================================================================*/
/** <libreply>                                                       */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   <enter_data>                                                   */
/**     <volser>vvvvvv</volser>                                      */
/**     <lsm_id>aa:ll</lsm_id>                                       */
/**     <library_address>                                            */
/**       <acs>aa</acs>                                              */
/**       <lsm>aa</lsm>                                              */
/**       <panel>aa</panel>                                          */
/**       <row>aa</row>                                              */
/**       <column>aa</column>                                        */
/**     </library_address>                                           */
/**     <media>mmmmmmmm</media>                                      */
/**     <media_type>t</media_type>                                   */
/**     <media_domain>d</media_domain>                               */
/**     <result>SUCCESS|FAILURE</result>                             */
/**     <error>xxxxxxxx</error>                                      */
/**     <reason>ccc...ccc</reason>                                   */
/**   </enter_data>                                                  */
/**   ...repeated <enter_data> entries                               */
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
#define SELF "extractEnterResponse"

static int extractEnterResponse(struct XAPICVT  *pXapicvt,
                                struct XAPIREQE *pXapireqe,
                                char            *pEnterResponse,
                                struct XMLPARSE *pXmlparse)
{
    int                 lastRC;
    int                 enterResponseSize;
    int                 volResponseCount;
    int                 volRemainingCount;
    int                 volPacketCount;
    int                 i;

    struct XMLELEM     *pFirstEnterDataXmlelem;
    struct XMLELEM     *pNextEnterDataXmlelem;
    struct XMLELEM     *pLibAddrXmlelem;
    struct XMLELEM     *pParentXmlelem;

    ENTER_REQUEST      *pEnter_Request      = (ENTER_REQUEST*) pXapireqe->pAcsapiBuffer;
    ENTER_RESPONSE     *pEnter_Response     = (ENTER_RESPONSE*) pEnterResponse;
    VOLUME_STATUS      *pVolume_Status      = &(pEnter_Response->volume_status[0]);

    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;

    struct RAWENTER     rawenter;
    struct RAWENTER    *pRawenter           = &rawenter;

    struct XMLSTRUCT    enterDataXmlstruct[] =
    {
        XNAME_enter_data,                   XNAME_volser,
        sizeof(pRawenter->volser),          pRawenter->volser,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_enter_data,                   XNAME_media,
        sizeof(pRawenter->media),           pRawenter->media,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_enter_data,                   XNAME_media_type,
        sizeof(pRawenter->mediaType),       pRawenter->mediaType,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_enter_data,                   XNAME_media_type,
        sizeof(pRawenter->mediaDomain),     pRawenter->mediaDomain,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_enter_data,                   XNAME_result,
        sizeof(pRawenter->result),          pRawenter->result,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_enter_data,                   XNAME_error,
        sizeof(pRawenter->error),           pRawenter->error,
        NOBLANKFILL, NOBITVALUE, 0, 0,      
        XNAME_enter_data,                   XNAME_reason,
        sizeof(pRawenter->reason),          pRawenter->reason,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 enterDataElementCount = sizeof(enterDataXmlstruct) / 
                                                sizeof(struct XMLSTRUCT);

    struct XMLSTRUCT    libAddrXmlstruct[]  =
    {
        XNAME_library_address,              XNAME_acs,
        sizeof(pRawenter->acs),             pRawenter->acs,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_lsm,
        sizeof(pRawenter->lsm),             pRawenter->lsm,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_panel,
        sizeof(pRawenter->panel),           pRawenter->panel,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_row,
        sizeof(pRawenter->row),             pRawenter->row,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_column,
        sizeof(pRawenter->column),          pRawenter->column,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 libAddrElementCount = sizeof(libAddrXmlstruct) / 
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
    /* ENTER command under the <enter_data> tag, and not under       */
    /* the <libreply><exceptions>.  Therefore, we continue trying    */
    /* to extract <enter_data> even when xapi_parse_header_trailer   */
    /* returns a bad RC.                                             */
    /*****************************************************************/

    /*****************************************************************/
    /* Count the number of <enter_data> entries.                     */
    /*****************************************************************/
    pFirstEnterDataXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                     pXmlparse->pHocXmlelem,
                                                     XNAME_enter_data);

    volResponseCount = 0;

    if (pFirstEnterDataXmlelem != NULL)
    {
        pParentXmlelem = pFirstEnterDataXmlelem->pParentXmlelem;

        TRMSGI(TRCI_XAPI,
               "pFirstEnterDataXmlelem=%08X; pParentXmlelem=%08X\n",
               pFirstEnterDataXmlelem,
               pParentXmlelem);

        pNextEnterDataXmlelem = pFirstEnterDataXmlelem;

        while (pNextEnterDataXmlelem != NULL)
        {
            volResponseCount++;

            pNextEnterDataXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                                 pParentXmlelem,
                                                                 pNextEnterDataXmlelem,
                                                                 XNAME_enter_data);

#ifdef DEBUG

            if (pNextEnterDataXmlelem != NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextEnterDataXmlelem=%08X\n",
                       pNextEnterDataXmlelem);
            }

#endif

        }
    }

    TRMSGI(TRCI_XAPI,
           "volResponseCount=%d, MAX_ID=%d\n",
           volResponseCount,
           MAX_ID);

    /*****************************************************************/
    /* If no <enter_data> elements found, then return an error.      */
    /*****************************************************************/
    if (volResponseCount == 0)
    {
        if (lastRC > 0)
        {
            return lastRC;
        }

        return STATUS_PROCESS_FAILURE;
    }

    /*****************************************************************/
    /* If <enter_data> elements found, then extract the data.        */
    /*****************************************************************/
    pNextEnterDataXmlelem = pFirstEnterDataXmlelem;
    volRemainingCount = volResponseCount;

    while (1)
    {
        pEnter_Response = (ENTER_RESPONSE*) pEnterResponse;
        pVolume_Status = &(pEnter_Response->volume_status[0]);

        if (volRemainingCount > MAX_ID)
        {
            volPacketCount = MAX_ID;
        }
        else
        {
            volPacketCount = volRemainingCount;
        }

        enterResponseSize = offsetof(ENTER_RESPONSE, volume_status[0]) +
                            (sizeof(VOLUME_STATUS) * volPacketCount);

        initEnterResponse(pXapireqe,
                          (char*) pEnterResponse,
                          sizeof(ENTER_RESPONSE));

        TRMSGI(TRCI_XAPI,
               "At top of while; volResponse=%d, volRemaining=%d, "
               "volPacket=%d, MAX_ID=%d, enterResponseSize=%d\n",
               volResponseCount,
               volRemainingCount,
               volPacketCount,
               MAX_ID,
               enterResponseSize);

        pEnter_Response->count = volPacketCount;

        for (i = 0;
            i < volPacketCount;
            i++, pVolume_Status++)
        {
            memset(pRawenter, 0, sizeof(struct RAWENTER));

            if (pNextEnterDataXmlelem == NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextEnterDataXmlelem=NULL at volCount=%i\n",
                       (i+1));

                break;
            }

            FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                              pNextEnterDataXmlelem,
                                              &enterDataXmlstruct[0],
                                              enterDataElementCount);

            pLibAddrXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                      pNextEnterDataXmlelem,
                                                      XNAME_library_address);

            if (pLibAddrXmlelem != NULL)
            {
                FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                                  pLibAddrXmlelem,
                                                  &libAddrXmlstruct[0],
                                                  libAddrElementCount);
            }

            TRMEMI(TRCI_XAPI,
                   pRawenter, sizeof(struct RAWENTER),
                   "RAWENTER:\n");

            if (pRawenter->volser[0] > 0)
            {
                memcpy(pVolume_Status->vol_id.external_label,
                       pRawenter->volser,
                       sizeof(pRawenter->volser));
            }

            if (pRawenter->result[0] == 'S')
            {
                pVolume_Status->status.status = STATUS_SUCCESS;
            }
            else
            {
                pVolume_Status->status.status = STATUS_DUPLICATE_LABEL;
            }

            pNextEnterDataXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                                 pParentXmlelem,
                                                                 pNextEnterDataXmlelem,
                                                                 XNAME_enter_data);
        } /* for(i) */

        volRemainingCount = volRemainingCount - volPacketCount;

        if (volRemainingCount <= 0)
        {
            xapi_fin_response(pXapireqe,
                              (char*) pEnter_Response,
                              enterResponseSize);

            break;
        }
        else
        {
            xapi_int_response(pXapireqe,
                              pEnterResponse,     
                              enterResponseSize);  
        }
    }

    return STATUS_SUCCESS;
}



