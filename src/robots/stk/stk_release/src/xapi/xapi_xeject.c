/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_xeject.c                                    */
/** Description:    XAPI XEJECT (eXtended eject) processor.          */
/**                                                                  */
/**                 Eject the specified range(s) of volsers from     */
/**                 the specified CAP.                               */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     08/15/11                          */
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

static int convertEjectRequest(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               VOLID           *pFirstVolid,
                               int              volserCount,
                               char           **ptrXapiBuffer,
                               int             *pXapiBufferSize);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_xeject                                       */
/** Description:   XAPI XEJECT (eXtended eject) processor.           */
/**                                                                  */
/** Eject the specified range(s) of volsers from the specified CAP.  */
/**                                                                  */
/** If the MESSAGE_HEADER.lock_id is NOT 0 (NO_LOCK_ID), then        */
/** validate that all of the specified volume(s) in the specified    */
/** ranges are either locked by the specified LOCKID,                */
/** or that they are unlocked altogether.                            */
/**                                                                  */
/** The ACSAPI format XEJECT request is translated into an           */
/** XAPI XML format <eject_vol> request; the XAPI XML request        */
/** is then transmiited to the server via TCP/IP;  The received      */
/** XAPI XML response is then translated into one or more            */
/** ACSAPI EJECT responses.                                          */
/**                                                                  */
/** The XJECT command is NOT allowed to proceed when                 */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI XEJECT (EXT_EJECT) request consists of:               */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER which includes the                      */
/**           a.   LOCKID                                            */
/** 2.   XEJECT (EXT_EJECT) data consisting of:                      */
/**      i.   CAPID data entry consisting of:                        */
/**           a.   cap_id.lsm_id.acs                                 */
/**           b.   cap_id.lsm_id.lsm                                 */
/**           c.   cap_id.cap                                        */
/**      ii.  count                                                  */
/**      iii. VOLRANGE[count] data entries consisting of             */
/**           a.    VOLID startvol                                   */
/**           b.    VOLID endvol                                     */
/**                                                                  */
/** NOTE: MAX_ID is 42.  Which is 42 ranges (a range may be a single */
/** volser or multiple volsers (without any specified limit).        */
/**                                                                  */
/** NOTE: Since the ACSAPI only allows LOCK VOLUME and LOCK DRIVE    */
/** (i.e. there is no ACSAPI LOCK CAP), the MESSAGE_HEADER.LOCKID    */
/** is assumed to refer to VOLUME locks.  However, as the XAPI       */
/** makes no provision for a LOCKID on the EJECT command,            */
/** the ACSAPI LOCKID parameter is currently ignored by the XAPI.    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_xeject"

extern int xapi_xeject(struct XAPICVT  *pXapicvt,
                       struct XAPIREQE *pXapireqe)
{
    int                 ejectRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 i;
    int                 volserCount;
    int                 volidBlockLen;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;

    struct XMLPARSE    *pXmlparse           = NULL;
    VOLID              *pFirstVolid         = NULL;
    VOLID              *pCurrVolid          = NULL;

    EXT_EJECT_REQUEST  *pExt_Eject_Request  = (EXT_EJECT_REQUEST*) pXapireqe->pAcsapiBuffer;

    EJECT_RESPONSE      wkEject_Response;
    EJECT_RESPONSE     *pEject_Response     = &wkEject_Response;

    int                 volrangeCount       = pExt_Eject_Request->count;
    char                firstVol[6];
    char                lastVol[6];
    struct XAPIVRANGE   xapivrange[MAX_ID];
    struct XAPIVRANGE  *pXapivrange         = &(xapivrange[0]);

    initEjectResponse(pXapireqe,
                      (char*) pEject_Response,
                      sizeof(EJECT_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Ejected; XEJECT request=%08X, size=%d, "
           "volRangeCount=%d, MAX_ID=%d, "
           "XEJECT response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           volrangeCount,
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

    /*****************************************************************/
    /* NOTE: The ACSAPI XEJECT request only allows volser            */
    /* ranges to be specified.   However, the volser range may be    */
    /* for a single volser (i.e. TAP001-TAP001).                     */
    /*****************************************************************/
    if (volrangeCount < 1)
    {
        xapi_err_response(pXapireqe,
                          (char*) pEject_Response,
                          EJECTERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_SMALL); 

        return STATUS_COUNT_TOO_SMALL;
    }

    if (volrangeCount > MAX_ID)
    {
        xapi_err_response(pXapireqe,
                          (char*) pEject_Response,
                          EJECTERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_LARGE); 

        return STATUS_COUNT_TOO_LARGE;
    }

    /*****************************************************************/
    /* Build the XAPIVRANGE block for each volser range in the       */
    /* request.  This also validates each volser range before we     */
    /* begin processing.                                             */
    /*****************************************************************/
    memset((char*) &xapivrange, 0, sizeof(xapivrange));

    for (i = 0, pXapivrange = &(xapivrange[0]);
        i < volrangeCount;
        i++, pXapivrange++)
    {
        memcpy(pXapivrange->firstVol,
               pExt_Eject_Request->vol_range[i].startvol.external_label,
               sizeof(firstVol));

        memcpy(pXapivrange->lastVol,
               pExt_Eject_Request->vol_range[i].endvol.external_label,
               sizeof(firstVol));

        lastRC = xapi_validate_volser_range(pXapivrange);

        if (lastRC != STATUS_SUCCESS)
        {
            xapi_err_response(pXapireqe,
                              (char*) pEject_Response,
                              EJECTERR_RESPONSE_SIZE,
                              lastRC); 

            return lastRC;
        }
    }

    /*****************************************************************/
    /* Now count the total number of volsers represented in all      */
    /* the volser ranges.                                            */
    /*****************************************************************/
    for (i = 0, volserCount = 0, pXapivrange = &(xapivrange[0]);
        i < volrangeCount;
        i++, pXapivrange++)
    {
        volserCount += pXapivrange->numInRange;
    }

    TRMSGI(TRCI_XAPI,
           "volserCount=%d\n",
           volserCount);

    /*****************************************************************/
    /* Now allocate a single block of VOLIDs and initialize.         */
    /*****************************************************************/
    volidBlockLen = volserCount * sizeof(VOLID);

    pFirstVolid = (VOLID*) malloc(volidBlockLen);

    TRMSGI(TRCI_STORAGE,
           "malloc VOLID(s)=%08X, len=%i\n",
           pFirstVolid,
           volidBlockLen);

    memset((char*) pFirstVolid, 0, volidBlockLen);

    for (i = 0, pCurrVolid = pFirstVolid, pXapivrange = &(xapivrange[0]);
        i < volrangeCount;
        i++, pXapivrange++)
    {
        while (1)
        {
            xapi_volser_increment(pXapivrange);

            if (pXapivrange->rangeCompletedFlag)
            {
                break;
            }

            memcpy(pCurrVolid->external_label,
                   pXapivrange->currVol,
                   sizeof(pXapivrange->currVol));

            pCurrVolid++;
        }
    }

    TRMEMI(TRCI_XAPI, pFirstVolid, volidBlockLen,
           "VOLID(s):\n");

    lastRC = convertEjectRequest(pXapicvt,
                                 pXapireqe,
                                 pFirstVolid,
                                 volserCount,
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
    EXT_EJECT_REQUEST      *pExt_Eject_Request      = (EXT_EJECT_REQUEST*) pXapireqe->pAcsapiBuffer;
    EJECT_RESPONSE     *pEject_Response     = (EJECT_RESPONSE*) pEjectResponse;

    TRMSGI(TRCI_XAPI,
           "Ejected\n");

    /*****************************************************************/
    /* Initialize EJECT response.                                    */
    /*****************************************************************/
    memset((char*) pEject_Response, 0, ejectResponseSize);

    memcpy((char*) &(pEject_Response->request_header),
           (char*) &(pExt_Eject_Request->request_header),
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
/** Convert the input volume list (derived from the ACSAPI format    */
/** XEJECT request) into an XAPI XML format <eject_vol> request.     */
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
                               VOLID           *pFirstVolid,
                               int              volserCount,
                               char           **ptrXapiBuffer,
                               int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;
    int                 i;
    int                 lockId;
    char                capString[3];
    VOLID              *pCurrVolid;

    char               *pXapiRequest        = NULL;
    EXT_EJECT_REQUEST  *pExt_Eject_Request  = (EXT_EJECT_REQUEST*) pXapireqe->pAcsapiBuffer;
    MESSAGE_HEADER     *pMessage_Header     = 
    &(pExt_Eject_Request->request_header.message_header);

    struct XMLPARSE    *pXmlparse; 
    struct XMLELEM     *pEjectVolXmlelem;
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;
    struct RAWQLOCK     rawqlock;
    struct RAWQLOCK    *pRawqlock           = &rawqlock;

    lockId = (int) pMessage_Header->lock_id;

    TRMSGI(TRCI_XAPI,
           "Ejected; volserCount=%d. lockId=%d\n",
           volserCount,
           lockId);

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

    /*****************************************************************/
    /* For eXtended eject:                                           */
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
        for (i = 0, pCurrVolid = pFirstVolid;
            i < volserCount;
            i++, pCurrVolid++)
        {
            memset((char*) pRawqlock,
                   0,
                   sizeof(struct RAWQLOCK));

            lastRC = xapi_qlock_vol_one(pXapicvt,
                                        pXapireqe,
                                        pRawqlock,
                                        pCurrVolid->external_label,
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
            pExt_Eject_Request->cap_id.lsm_id.acs);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_acs,
                                      capString,
                                      0);

    sprintf(capString,
            "%.2d",
            pExt_Eject_Request->cap_id.lsm_id.lsm);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_lsm,
                                      capString,
                                      0);

    sprintf(capString,
            "%.2d",
            pExt_Eject_Request->cap_id.cap);

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

    for (i = 0, pCurrVolid = pFirstVolid;
        i < volserCount;
        i++, pCurrVolid++)
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_volser,
                                          pCurrVolid->external_label,
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



