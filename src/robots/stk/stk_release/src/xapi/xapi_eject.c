/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_eject.c                                     */
/** Description:    XAPI EJECT processor.                            */
/**                                                                  */
/**                 Eject up to MAX_ID (i.e. 42) volumes from        */
/**                 the ACS using the specified CAP.                 */
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
#define EJECT_SEND_TIMEOUT         5   /* TIMEOUT values in seconds  */       
#define EJECT_RECV_TIMEOUT_1ST     600
#define EJECT_RECV_TIMEOUT_NON1ST  7200
#define EJECTERR_RESPONSE_SIZE (offsetof(EJECT_RESPONSE, volume_status[0])) 


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void initEjectResponse(struct XAPIREQE *pXapireqe,
                              char            *pEjectResponse,
                              int              ejectResponseSize);

static int convertEjectRequest(struct XAPICVT   *pXapicvt,
                               struct XAPIREQE  *pXapireqe,
                               char            **ptrXapiBuffer,
                               int              *pXapiBufferSize);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_eject                                        */
/** Description:   The XAPI EJECT processor.                         */
/**                                                                  */
/** Eject up to MAX_ID (i.e. 42) volumes from the ACS using the      */
/** specified CAP.                                                   */
/**                                                                  */
/** If the MESSAGE_HEADER.lock_id is NOT 0 (NO_LOCK_ID), then        */
/** validate that all of the specified volume(s) are either locked   */
/** by the specified LOCKID, or that they are unlocked altogether.   */
/**                                                                  */
/** The ACSAPI format EJECT request is translated into an            */
/** XAPI XML format <eject_vol> request; the XAPI XML request is     */
/** then transmitted to the server via TCP/IP;  The received         */
/** XAPI XML response is then translated into one or more            */
/** ACSAPI EJECT responses.                                          */
/**                                                                  */
/** The EJECT command is NOT allowed to proceed when the             */
/** XAPI client is in the IDLE state.                                */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_eject"

extern int xapi_eject(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    int                 ejectRC             = STATUS_SUCCESS;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    EJECT_REQUEST      *pEject_Request      = (EJECT_REQUEST*) pXapireqe->pAcsapiBuffer;

    EJECT_RESPONSE      wkEject_Response;
    EJECT_RESPONSE     *pEject_Response     = &wkEject_Response;

    initEjectResponse(pXapireqe,
                      (char*) pEject_Response,
                      sizeof(EJECT_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Ejected; EJECT request=%08X, size=%d, "
           "MAX_ID=%d, EJECT response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           MAX_ID,
           pEject_Response,
           sizeof(EJECT_RESPONSE));

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pEject_Response,
                            EJECTERR_RESPONSE_SIZE);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    lastRC = convertEjectRequest(pXapicvt,
                                 pXapireqe,
                                 &pXapiBuffer,
                                 &xapiBufferSize);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from convertEjectRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pEject_Response,
                          EJECTERR_RESPONSE_SIZE,
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
                      EJECT_SEND_TIMEOUT,        
                      EJECT_RECV_TIMEOUT_1ST,    
                      EJECT_RECV_TIMEOUT_NON1ST, 
                      (void**) &pXmlparse);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from xapi_tcp(pBuffer=%08X, size=%d, pXmlparse=%08X)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize,
           pXmlparse);

    if (lastRC != STATUS_SUCCESS)
    {
        ejectRC = lastRC;
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
    /* Now generate the EJECT ACSAPI response.                       */
    /*****************************************************************/
    if (ejectRC == STATUS_SUCCESS)
    {
        lastRC = xapi_eject_response(pXapicvt,
                                     pXapireqe,
                                     (char*) pEject_Response,
                                     (void*) pXmlparse);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from xapi_eject_response\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            ejectRC = lastRC;
        }
    }

    if (ejectRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pEject_Response,
                          EJECTERR_RESPONSE_SIZE,
                          ejectRC);
    }

    /*****************************************************************/
    /* When done, free the XMLPARSE and associated structures.       */
    /*****************************************************************/
    if (pXmlparse != NULL)
    {
        FN_FREE_HIERARCHY_STORAGE(pXmlparse);
    }

    return ejectRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: initEjectResponse                                 */
/** Description:   Initialize the ACSAPI EJECT response.             */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI EJECT response consists of:                           */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   EJECT data consisting of                                    */
/**      i.   CAPID consisting of:                                   */
/**           a.   cap_id.lsm_id.acs                                 */
/**           b.   cap_id.lsm_id.lsm                                 */
/**           c    cap_id.cap                                        */
/**      ii.  count of volumes ejected                               */
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
#define SELF "initEjectResponse "

static void initEjectResponse(struct XAPIREQE *pXapireqe,
                              char            *pEjectResponse,
                              int              ejectResponseSize)
{
    EJECT_REQUEST      *pEject_Request      = (EJECT_REQUEST*) pXapireqe->pAcsapiBuffer;
    EJECT_RESPONSE     *pEject_Response     = (EJECT_RESPONSE*) pEjectResponse;

    TRMSGI(TRCI_XAPI,
           "Ejected\n");

    /*****************************************************************/
    /* Initialize EJECT response.                                    */
    /*****************************************************************/
    memset((char*) pEject_Response, 0, ejectResponseSize);

    memcpy((char*) &(pEject_Response->request_header),
           (char*) &(pEject_Request->request_header),
           sizeof(REQUEST_HEADER));

    pEject_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pEject_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pEject_Response->message_status.status = STATUS_SUCCESS;
    pEject_Response->message_status.type = TYPE_NONE;

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: convertEjectRequest                               */
/** Description:   Build an XAPI <eject_vol> request.                */
/**                                                                  */
/** Convert the ACSAPI format EJECT request into an XAPI             */
/** XML format <eject_vol> request.                                  */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI EJECT request consists of:                            */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER which includes the                      */
/**           a.   LOCKID                                            */
/** 2.   EJECT data consisting of:                                   */
/**      i.   CAPID data entry consisting of:                        */
/**           a.   cap_id.lsm_id.acs                                 */
/**           b.   cap_id.lsm_id.lsm                                 */
/**           c.   cap_id.cap                                        */
/**      ii.  count                                                  */
/**      iii. VOLID[count] data entries consisting of:               */
/**           a.   external_label (6 character volser)               */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <eject_vol> request consists of:                    */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <eject_vol>                                                  */
/**       <cap_list>                   (only 1 cap specified by CDK) */
/**         <cap_location_data>                                      */
/**           <acs>aa</acs>                                          */
/**           <lsm>ll</lsm>                                          */
/**           <cap>nn</cap>                                          */
/**         </cap_location_data>                                     */
/**       </cap_list>                                                */
/**       <volume_list>                                              */
/**         <volser>vvvvvv<volser>                                   */
/**         ...repeated <volser> entries                             */
/**       </volume_list>                                             */
/**       </vol_range>vvvvvv-vvvvvv</vol_range>        (N/A for CDK) */
/**       <scratch>NO|YES</scratch>                    (N/A for CDK) */
/**       <sequence_by_lsm>YES|NO</sequence_by_lsm>    (N/A for CDK) */
/**       <subpool_name>sssssssssssss</subpool_name>   (N/A for CDK) */
/**       <volume_count>nnnn</volume_count>            (N/A for CDK) */
/**       <media>mmmmmmmm</media>                      (N/A for CDK) */
/**       <media_type>t</media_type>                   (N/A for CDK) */
/**       <media_domain>d</media_domain>               (N/A for CDK) */
/**       <rectech>rrrrrrrr</rectech>                  (N/A for CDK) */
/**       <wait_cap>YES|NO</wait_cap>                  (N/A for CDK) */
/**       <eject_text>ttt...ttt</eject_text>                         */
/**     </eject_vol>                                                 */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "convertEjectRequest"

static int convertEjectRequest(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char           **ptrXapiBuffer,
                               int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;
    int                 i;
    char                capString[3];
    char                volserString[XAPI_VOLSER_SIZE + 1];

    char               *pXapiRequest        = NULL;
    EJECT_REQUEST      *pEject_Request      = (EJECT_REQUEST*) pXapireqe->pAcsapiBuffer;
    MESSAGE_HEADER     *pMessage_Header     = 
    &(pEject_Request->request_header.message_header);

    int                 volCount            = pEject_Request->count;
    int                 lockId              = (int) pMessage_Header->lock_id;

    struct XMLPARSE    *pXmlparse; 
    struct XMLELEM     *pEjectVolXmlelem;
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;
    struct RAWQLOCK     rawqlock;
    struct RAWQLOCK    *pRawqlock           = &rawqlock;

    TRMSGI(TRCI_XAPI,
           "Ejected; volCount=%d, lockId=%d\n",
           volCount,
           lockId);

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

    if (volCount < 1)
    {
        return STATUS_COUNT_TOO_SMALL;
    }

    if (volCount > MAX_ID)
    {
        return STATUS_COUNT_TOO_LARGE;
    }

    /*****************************************************************/
    /* For eject:                                                    */
    /* Only check volume lock_id(s) if specified.                    */
    /*                                                               */
    /* If the volume is locked by the specified lock_id, or if the   */
    /* volume is unlocked altogether, or if the volume is not        */
    /* found, then it is acceptable for eject.                       */
    /*                                                               */
    /* If no NO_LOCK_ID specified, then we do not really care if     */
    /* the volume(s) are locked!                                     */
    /*****************************************************************/
    if (lockId != NO_LOCK_ID)
    {
        for (i = 0;
            i < volCount;
            i++)
        {
            memset((char*) pRawqlock,
                   0,
                   sizeof(struct RAWQLOCK));

            lastRC = xapi_qlock_vol_one(pXapicvt,
                                        pXapireqe,
                                        pRawqlock,
                                        pEject_Request->vol_id[i].external_label,
                                        lockId);

            TRMEMI(TRCI_XAPI, pRawqlock, sizeof(struct RAWQLOCK),
                   "EJECT VOLUME lock status=%d; RAWQLOCK:\n",
                   pRawqlock->queryRC);

            if (pRawqlock->queryRC != STATUS_SUCCESS)
            {
                if ((pRawqlock->resRC != STATUS_VOLUME_AVAILABLE) &&
                    (pRawqlock->resRC != STATUS_VOLUME_NOT_IN_LIBRARY))
                {
                    if (pRawqlock->resRC != STATUS_SUCCESS)
                    {
                        return pRawqlock->resRC;
                    }
                    else
                    {
                        return STATUS_VOLUME_IN_USE;
                    }
                }
            }
        }
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
                                      XNAME_eject_vol,
                                      NULL,
                                      0);

    pEjectVolXmlelem = pXmlparse->pCurrXmlelem;
/*
    if (pXapicvt->xapiEjectText[0] > ' ')
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pEjectVolXmlelem,
                                          XNAME_eject_text,
                                          pXapicvt->xapiEjectText,
                                          0);
    }
*/
    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pEjectVolXmlelem,
                                      XNAME_cap_list,
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
            pEject_Request->cap_id.lsm_id.acs);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_acs,
                                      capString,
                                      0);

    sprintf(capString,
            "%.2d",
            pEject_Request->cap_id.lsm_id.lsm);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_lsm,
                                      capString,
                                      0);

    sprintf(capString,
            "%.2d",
            pEject_Request->cap_id.cap);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_cap,
                                      capString,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pEjectVolXmlelem,
                                      XNAME_volume_list,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;
    memset(volserString, 0, sizeof(volserString));

    for (i = 0;
        i < volCount;
        i++)
    {
        memcpy(volserString,
               pEject_Request->vol_id[i].external_label,
               XAPI_VOLSER_SIZE);

        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_volser,
                                          volserString,
                                          0);
    }

    pParentXmlelem = pXmlparse->pCurrXmlelem;

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
/** Function Name: xapi_eject_response                               */
/** Description:   Extract the <eject_data> response.                */
/**                                                                  */
/** Parse the response of the XAPI XML <eject_vol>                   */
/** request and update the appropriate fields of the                 */
/** ACSAPI EJECT response.                                           */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <eject_data> responses consists of:                 */
/**==================================================================*/
/** <libreply>                                                       */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   <eject_data>                                                   */
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
/**   </eject_data>                                                  */
/**   ...repeated <eject_data> entries                               */
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
#define SELF "xapi_eject_response"

extern int xapi_eject_response(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char            *pEjectResponse,
                               void            *pResponseXmlparse)
{
    int                 lastRC;
    int                 ejectResponseSize;
    int                 volResponseCount;
    int                 volRemainingCount;
    int                 volPacketCount;
    int                 i;
    char               *pSLS2973IVolume;

    struct XMLPARSE    *pXmlparse           = (struct XMLPARSE*) pResponseXmlparse;
    struct XMLELEM     *pFirstEjectDataXmlelem;
    struct XMLELEM     *pNextEjectDataXmlelem;
    struct XMLELEM     *pCapLocDataXmlelem;
    struct XMLELEM     *pParentXmlelem;

    EJECT_REQUEST      *pEject_Request      = (EJECT_REQUEST*) pXapireqe->pAcsapiBuffer;
    EJECT_RESPONSE     *pEject_Response     = (EJECT_RESPONSE*) pEjectResponse;
    VOLUME_STATUS      *pVolume_Status      = &(pEject_Response->volume_status[0]);

    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;

    struct RAWEJECT     raweject;
    struct RAWEJECT    *pRaweject           = &raweject;

    struct XMLSTRUCT    ejectDataXmlstruct[] =
    {
        XNAME_eject_data,                   XNAME_volser,
        sizeof(pRaweject->volser),          pRaweject->volser,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_eject_data,                   XNAME_result,
        sizeof(pRaweject->result),          pRaweject->result,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_eject_data,                   XNAME_error,
        sizeof(pRaweject->error),           pRaweject->error,
        NOBLANKFILL, NOBITVALUE, 0, 0,      
        XNAME_eject_data,                   XNAME_reason,
        sizeof(pRaweject->reason),          pRaweject->reason,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 ejectDataElementCount = sizeof(ejectDataXmlstruct) / 
                                                sizeof(struct XMLSTRUCT);

    struct XMLSTRUCT    capLocDataXmlstruct[] =
    {
        XNAME_library_address,              XNAME_acs,
        sizeof(pRaweject->acs),             pRaweject->acs,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_lsm,
        sizeof(pRaweject->lsm),             pRaweject->lsm,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_panel,
        sizeof(pRaweject->panel),           pRaweject->panel,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_row,
        sizeof(pRaweject->row),             pRaweject->row,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_column,
        sizeof(pRaweject->column),          pRaweject->column,
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
    /* EJECT command under the <eject_data> tag, and not under       */
    /* the <libreply><exceptions>.  Therefore, we continue trying    */
    /* to extract <eject_data> even when xapi_parse_header_trailer   */
    /* returns a bad RC.                                             */
    /*****************************************************************/

    /*****************************************************************/
    /* Count the number of <eject_data> entries.                     */
    /*****************************************************************/
    pFirstEjectDataXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                     pXmlparse->pHocXmlelem,
                                                     XNAME_eject_data);

    volResponseCount = 0;

    if (pFirstEjectDataXmlelem != NULL)
    {
        pParentXmlelem = pFirstEjectDataXmlelem->pParentXmlelem;

        TRMSGI(TRCI_XAPI,
               "pFirstEjectDataXmlelem=%08X; pParentXmlelem=%08X\n",
               pFirstEjectDataXmlelem,
               pParentXmlelem);

        pNextEjectDataXmlelem = pFirstEjectDataXmlelem;

        while (pNextEjectDataXmlelem != NULL)
        {
            volResponseCount++;

            pNextEjectDataXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                                 pParentXmlelem,
                                                                 pNextEjectDataXmlelem,
                                                                 XNAME_eject_data);

#ifdef DEBUG

            if (pNextEjectDataXmlelem != NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextEjectDataXmlelem=%08X\n",
                       pNextEjectDataXmlelem);
            }

#endif

        }
    }

    TRMSGI(TRCI_XAPI,
           "volResponseCount=%d, MAX_ID=%d\n",
           volResponseCount,
           MAX_ID);

    /*****************************************************************/
    /* If no <eject_data> elements found, then return an error.      */
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
    /* If <eject_data> elements found, then extract the data.        */
    /*****************************************************************/
    pNextEjectDataXmlelem = pFirstEjectDataXmlelem;
    volRemainingCount = volResponseCount;

    while (1)
    {
        pEject_Response = (EJECT_RESPONSE*) pEjectResponse;
        pVolume_Status = &(pEject_Response->volume_status[0]);

        if (volRemainingCount > MAX_ID)
        {
            volPacketCount = MAX_ID;
        }
        else
        {
            volPacketCount = volRemainingCount;
        }

        ejectResponseSize = offsetof(EJECT_RESPONSE, volume_status[0]) +
                            (sizeof(VOLUME_STATUS) * volPacketCount);

        initEjectResponse(pXapireqe,
                          (char*) pEjectResponse,
                          sizeof(EJECT_RESPONSE));

        TRMSGI(TRCI_XAPI,
               "At top of while; volResponse=%d, volRemaining=%d, "
               "volPacket=%d, MAX_ID=%d, ejectResponseSize=%d\n",
               volResponseCount,
               volRemainingCount,
               volPacketCount,
               MAX_ID,
               ejectResponseSize);

        pEject_Response->count = volPacketCount;

        for (i = 0;
            i < volPacketCount;
            i++, pVolume_Status++)
        {
            memset(pRaweject, 0, sizeof(struct RAWEJECT));

            if (pNextEjectDataXmlelem == NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextEjectDataXmlelem=NULL at volCount=%i\n",
                       (i+1));

                break;
            }

            FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                              pNextEjectDataXmlelem,
                                              &ejectDataXmlstruct[0],
                                              ejectDataElementCount);

            pCapLocDataXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                         pNextEjectDataXmlelem,
                                                         XNAME_cap_location_data);

            if (pCapLocDataXmlelem != NULL)
            {
                FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                                  pCapLocDataXmlelem,
                                                  &capLocDataXmlstruct[0],
                                                  capLocDataElementCount);
            }

            TRMEMI(TRCI_XAPI,
                   pRaweject, sizeof(struct RAWEJECT),
                   "RAWEJECT:\n");

            /*********************************************************/
            /* If <volser> tag not found, then assume that server    */
            /* <reason> will display volser after "VOLUME " literal. */
            /*********************************************************/
            if (pRaweject->volser[0] > 0)
            {
                memcpy(pVolume_Status->vol_id.external_label,
                       pRaweject->volser,
                       sizeof(pRaweject->volser));
            }
            else if (pRaweject->reason[0] > 0)
            {
                pSLS2973IVolume = strstr(pRaweject->reason, "VOLUME ");

                if (pSLS2973IVolume != NULL)
                {
                    memcpy(pVolume_Status->vol_id.external_label,
                           &(pSLS2973IVolume[7]),
                           XAPI_VOLSER_SIZE);
                }
            }

            if (pRaweject->result[0] == 'S')
            {
                pVolume_Status->status.status = STATUS_SUCCESS;
            }
            else
            {
                pVolume_Status->status.status = STATUS_VOLUME_NOT_IN_LIBRARY;
            }

            pNextEjectDataXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                                 pParentXmlelem,
                                                                 pNextEjectDataXmlelem,
                                                                 XNAME_eject_data);
        } /* for(i) */

        volRemainingCount = volRemainingCount - volPacketCount;

        if (volRemainingCount <= 0)
        {
            xapi_fin_response(pXapireqe,
                              (char*) pEject_Response,
                              ejectResponseSize);

            break;
        }
        else
        {
            xapi_int_response(pXapireqe,
                              pEjectResponse,     
                              ejectResponseSize);  
        }
    }

    return STATUS_SUCCESS;
}



