/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_dismount.c                                  */
/** Description:    XAPI DISMOUNT processor.                         */
/**                                                                  */
/**                 Dismount the specified volume from the           */
/**                 specified drive.                                 */
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
#define DISMNT_SEND_TIMEOUT        5   /* TIMEOUT values in seconds  */       
#define DISMNT_RECV_TIMEOUT_1ST    120
#define DISMNT_RECV_TIMEOUT_NON1ST 600


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void initDismountResponse(struct XAPIREQE *pXapireqe,
                                 char            *pDismountResponse);

static int convertDismountRequest(struct XAPICVT   *pXapicvt,
                                  struct XAPIREQE  *pXapireqe,
                                  char            **ptrXapiBuffer,
                                  int              *pXapiBufferSize);

static int extractDismountResponse(struct XAPICVT  *pXapicvt,
                                   struct XAPIREQE *pXapireqe,
                                   char            *pDismountResponse,
                                   struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_dismount                                     */
/** Description:   The XAPI DISMOUNT processor.                      */
/**                                                                  */
/** Dismount the specified volume from the specified drive.          */
/**                                                                  */
/** If the MESSAGE_HEADER.lock_id is NOT 0 (NO_LOCK_ID), then        */
/** validate that both the specified volume and drive are locked     */
/** by the specified LOCKID.                                         */
/**                                                                  */
/** The ACSAPI format DISMOUNT request is translated into an         */
/** XAPI XML format <dismount> request; the XAPI XML request is      */
/** then transmitted to the server via TCP/IP;  The received         */
/** XAPI XML response is then translated into the                    */
/** ACSAPI DISMOUNT response.                                        */
/**                                                                  */
/** The DISMOUNT command is NOT allowed to proceed when the          */
/** XAPI client is in the IDLE state.                                */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_dismount"

extern int xapi_dismount(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    int                 dismountRC          = STATUS_SUCCESS;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    DISMOUNT_REQUEST   *pDismount_Request   = (DISMOUNT_REQUEST*) pXapireqe->pAcsapiBuffer;
    DISMOUNT_RESPONSE   wkDismount_Response;
    DISMOUNT_RESPONSE  *pDismount_Response  = &wkDismount_Response;

    TRMSGI(TRCI_XAPI,
           "Entered; DISMOUNT request=%08X, size=%d, "
           "DISMOUNT response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pDismount_Response,
           sizeof(DISMOUNT_RESPONSE));

    initDismountResponse(pXapireqe,
                         (char*) pDismount_Response);

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pDismount_Response,
                            (sizeof(DISMOUNT_RESPONSE)));

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    lastRC = convertDismountRequest(pXapicvt,
                                    pXapireqe,
                                    &pXapiBuffer,
                                    &xapiBufferSize);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from convertDismountRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pDismount_Response,
                          sizeof(DISMOUNT_RESPONSE),
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
                      DISMNT_SEND_TIMEOUT,       
                      DISMNT_RECV_TIMEOUT_1ST,   
                      DISMNT_RECV_TIMEOUT_NON1ST,
                      (void**) &pXmlparse);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from xapi_tcp(pBuffer=%08X, size=%d, pXmlparse=%08X)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize,
           pXmlparse);

    if (lastRC != STATUS_SUCCESS)
    {
        dismountRC = lastRC;
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
    /* Now generate the DISMOUNT ACSAPI response.                    */
    /*****************************************************************/
    if (dismountRC == STATUS_SUCCESS)
    {
        lastRC = extractDismountResponse(pXapicvt,
                                         pXapireqe,
                                         (char*) pDismount_Response,
                                         pXmlparse);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractDismountResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            dismountRC = lastRC;
        }
    }

    if (dismountRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pDismount_Response,
                          sizeof(DISMOUNT_RESPONSE),
                          dismountRC);
    }
    else
    {
        xapi_fin_response(pXapireqe,
                          (char*) pDismount_Response,
                          sizeof(DISMOUNT_RESPONSE));
    }

    /*****************************************************************/
    /* When done, free the XMLPARSE and associated structures.       */
    /*****************************************************************/
    if (pXmlparse != NULL)
    {
        FN_FREE_HIERARCHY_STORAGE(pXmlparse);
    }

    return dismountRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: initDismountResponse                              */
/** Description:   Initialize the ACSAPI DISMOUNT response.          */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI DISMOUNT response consists of:                        */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   DISMOUNT_REQUEST data consisting of                         */
/**      i.   vol_id.external_label                                  */
/**      ii.  drive_id consisting of                                 */
/**           a.   panel_id.lsm_id.acs                               */
/**           b.   panel_id.lsm_id.lsm                               */
/**           c.   panel_id.panel                                    */
/**           d.   driveid                                           */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "initDismountResponse"

static void initDismountResponse(struct XAPIREQE *pXapireqe,
                                 char            *pDismountResponse)
{
    DISMOUNT_REQUEST   *pDismount_Request   = (DISMOUNT_REQUEST*) pXapireqe->pAcsapiBuffer;
    DISMOUNT_RESPONSE  *pDismount_Response  = (DISMOUNT_RESPONSE*) pDismountResponse;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Initialize DISMOUNT response.                                 */
    /*****************************************************************/
    memset((char*) pDismount_Response, 0, sizeof(DISMOUNT_RESPONSE));

    memcpy((char*) &(pDismount_Response->request_header),
           (char*) &(pDismount_Request->request_header),
           sizeof(REQUEST_HEADER));

    pDismount_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pDismount_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pDismount_Response->message_status.status = STATUS_SUCCESS;
    pDismount_Response->message_status.type = TYPE_NONE;

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: convertDismountRequest                            */
/** Description:   Build an XAPI <dismount> request.                 */
/**                                                                  */
/** Convert the ACSAPI format DISMOUNT request into an               */
/** XAPI XML format <dismount> request.                              */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI DISMOUNT request consists of:                         */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   DISMOUNT_REQUEST data consisting of                         */
/**      i.   vol_id.external_label                                  */
/**      ii.  drive_id consisting of                                 */
/**           a.   panel_id.lsm_id.acs                               */
/**           b.   panel_id.lsm_id.lsm                               */
/**           c.   panel_id.panel                                    */
/**           d.   driveid                                           */
/**                                                                  */
/** NOTE: In addition, the MESSAGE_HEADER.message_options FORCE bit  */
/** indicates whether a force rewind/unload is requested.            */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <dismount> request consists of:                     */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <dismount>                                                   */
/**       <user_name>hhhhhhhh</user_name>                            */
/**       <host_name>hhhhhhhh</host_name>                            */
/**       <volser>vvvvvv</volser>                                    */
/**       <rewind_unload>"Yes"|"No"</rewind_unload>                  */
/**       <drive_name>ccua>/drive_name>                (N/A CDK) OR  */
/**       <drive_location_id>R:aa:ll:pp:dd:zz</drive_location_id>    */
/**     </dismount>                                                  */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** NOTES:                                                           */
/** (1) The xapi_hostname and unix userid specifications             */
/**     are not included and will not be used by the HSC server to   */
/**     control access to specific volsers (which is allowed using   */
/**     the HSC VOLPARMS feature).                                   */
/** (2) The XAPI request translated from the CDK ACSAPI will always  */
/**     specify the <drive_location> version of the drive address.   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "convertDismountRequest"

static int convertDismountRequest(struct XAPICVT  *pXapicvt,
                                  struct XAPIREQE *pXapireqe,
                                  char           **ptrXapiBuffer,
                                  int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;
    int                 lockId;

    struct LIBDRVID     libdrvid;
    struct LIBDRVID    *pLibdrvid           = &libdrvid;
    struct XAPICFG     *pXapicfg            = NULL;
    struct RAWQLOCK     rawqlock;
    struct RAWQLOCK    *pRawqlock           = &rawqlock;

    char                driveLocIdString[24];

    char               *pXapiRequest        = NULL;

    DISMOUNT_REQUEST   *pDismount_Request   = (DISMOUNT_REQUEST*) pXapireqe->pAcsapiBuffer;
    MESSAGE_HEADER     *pMessage_Header     = 
    &(pDismount_Request->request_header.message_header);

    struct XMLPARSE    *pXmlparse           = NULL; 
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    lockId = (int) pMessage_Header->lock_id;

    TRMSGI(TRCI_XAPI,
           "Entered; lockId=%d\n",
           lockId);

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

    memset((char*) pLibdrvid, 0 , sizeof(struct LIBDRVID));
    memset(driveLocIdString, 0, sizeof(driveLocIdString)); 

    pLibdrvid->acs = (unsigned char) pDismount_Request->drive_id.panel_id.lsm_id.acs;
    pLibdrvid->lsm = (unsigned char) pDismount_Request->drive_id.panel_id.lsm_id.lsm;
    pLibdrvid->panel = (unsigned char) pDismount_Request->drive_id.panel_id.panel;
    pLibdrvid->driveNumber = (unsigned char) pDismount_Request->drive_id.drive;

    pXapicfg = xapi_config_search_libdrvid(pXapicvt,
                                           pXapireqe,
                                           pLibdrvid);

    if (pXapicfg == NULL)
    {
        return STATUS_DRIVE_NOT_IN_LIBRARY;   
    }

    /*****************************************************************/
    /* For a dismount:                                               */
    /* Only check drive lock if a non-zero lock_id is specified.     */
    /*                                                               */
    /* Only the drive needs to be locked for the specified lock_id   */
    /* for dismount.  If the volume is either locked by the          */
    /* specified lock_id, or if the volume is not locked altogher    */
    /* then it is acceptable for dismount.                           */
    /* This allows the user to dismount volumes that                 */
    /* were mounted using the MOUNT SCRATCH and MOUNT PINFO          */
    /* commands (where a specific volser was not specified, or       */
    /* locked, at mount time).                                       */
    /*                                                               */
    /* If no NO_LOCK_ID specified, then we do not really care if     */
    /* the drive and volume are locked!  This is probably not the    */
    /* way that lock_id processing should work, but it is the way    */
    /* that LibStation handles a dismount lock_id specification.     */
    /*****************************************************************/
    if (lockId != NO_LOCK_ID)
    {
        memset((char*) pRawqlock,
               0,
               sizeof(struct RAWQLOCK));

        lastRC = xapi_qlock_drv_one(pXapicvt,
                                    pXapireqe,
                                    pRawqlock,
                                    pXapicfg->driveLocId,
                                    lockId);

        TRMEMI(TRCI_XAPI, pRawqlock, sizeof(struct RAWQLOCK),
               "DISMOUNT DRIVE lock status=%d; RAWQLOCK:\n",
               pRawqlock->queryRC);

        if (pRawqlock->queryRC != STATUS_SUCCESS)
        {
            if (pRawqlock->resRC != STATUS_SUCCESS)
            {
                return pRawqlock->resRC;
            }
            else
            {
                return STATUS_DRIVE_IN_USE;
            }
        }

        memset((char*) pRawqlock,
               0,
               sizeof(struct RAWQLOCK));

        lastRC = xapi_qlock_vol_one(pXapicvt,
                                    pXapireqe,
                                    pRawqlock,
                                    pDismount_Request->vol_id.external_label,
                                    lockId);

        TRMEMI(TRCI_XAPI, pRawqlock, sizeof(struct RAWQLOCK),
               "DISMOUNT VOLUME lock status=%d; RAWQLOCK:\n",
               pRawqlock->queryRC);

        if (pRawqlock->queryRC != STATUS_SUCCESS)
        {
            if (pRawqlock->resRC != STATUS_VOLUME_AVAILABLE)
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
                                      XNAME_dismount,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    if (pDismount_Request->vol_id.external_label[0] > ' ')
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_volser,
                                          pDismount_Request->vol_id.external_label,
                                          0);
    }

    if (pMessage_Header->message_options & FORCE)
    {
        TRMSGI(TRCI_XAPI,
               "FORCE option specified\n");

        if (memcmp(pXapicfg->model,
                   "VIRTUAL ",
                   sizeof(pXapicfg->model)) == 0)
        {
            TRMSGI(TRCI_XAPI,
                   "FORCE option ignored for VIRTUAL drive=%.16s (%02X)\n",
                   pXapicfg->driveLocId,
                   pXapicfg->driveName);
        }
        else
        {
            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pParentXmlelem,
                                              XNAME_rewind_unload,
                                              XCONTENT_YES,
                                              0);
        }
    }

    STRIP_TRAILING_BLANKS(pXapicfg->driveLocId,
                          driveLocIdString,
                          sizeof(pXapicfg->driveLocId));

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_drive_location_id,
                                      driveLocIdString,
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
/** Function Name: extractDismountResponse                           */
/** Description:   Extract the <dismount_data> response.             */
/**                                                                  */
/** Parse the response of the XAPI XML <dismount> request and        */
/** update the appropriate fields of the ACSAPI DISMOUNT response.   */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <dismount_data> response consists of:               */
/**==================================================================*/
/** <libreply>                                                       */
/**   <dismount_request>                                             */
/**     <header>                                                     */
/**       header element children                                    */
/**     </header>                                                    */
/**     <dismount_data>                                              */
/**       <volser>vvvvvv</volser>                                    */
/**       <device_address>ccua</device_address>                      */
/**       <drive_name>ccua</drive_name>                              */
/**       <drive_location_id>R:aa:ll:pp:dd:zzzz</drive_location_id>  */
/**       <drive_library_address>                                    */
/**         <acs>aa</acs>                                            */
/**         <lsm>ll</lsm>                                            */
/**         <panel>pp</panel>                                        */
/**         <drive_number>dd</drive_number>                          */
/**       </drive_library_address>                                   */
/**       <drive_location_zone>zzzz</drive_location_zone>            */
/**       <vtss_name>nnnnnnnn</vtss_name>                            */
/**     </dismount_data>                                             */
/**     <exceptions>                                                 */
/**       <reason>ccc...ccc</reason>                                 */
/**       ...repeated <reason> entries                               */
/**     </exceptions>                                                */
/**     <uui_return_code>nnnn</uui_return_code>                      */
/**     <uui_reason_code>nnnn</uui_reason_code>                      */
/**   </dismount_request>                                            */
/** </libreply>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractDismountResponse"

static int extractDismountResponse(struct XAPICVT  *pXapicvt,
                                   struct XAPIREQE *pXapireqe,
                                   char            *pDismountResponse,
                                   struct XMLPARSE *pXmlparse)
{
    int                 lastRC;
    int                 locInt;

    struct XMLELEM     *pDismountDataXmlelem;
    struct XMLELEM     *pLibAddrXmlelem;
    struct XMLELEM     *pDriveAddrXmlelem;
    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;
    struct RAWCOMMON   *pRawcommon          = &(pXapicommon->rawcommon);
    struct RAWMOUNT     rawmount;           
    struct RAWMOUNT    *pRawmount           = &rawmount;
    struct RAWVOLUME   *pRawvolume          = &(pRawmount->rawvolume);

    DISMOUNT_RESPONSE  *pDismount_Response  = (DISMOUNT_RESPONSE*) pDismountResponse;

    struct XMLSTRUCT    dismountDataXmlstruct[]     =
    {
        XNAME_dismount_data,                XNAME_volser,
        sizeof(pRawvolume->volser),         pRawvolume->volser,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_dismount_data,                XNAME_media,
        sizeof(pRawvolume->media),          pRawvolume->media,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_dismount_data,                XNAME_media_type,
        sizeof(pRawvolume->mediaType),      pRawvolume->mediaType,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_dismount_data,                XNAME_media_domain,
        sizeof(pRawvolume->mediaDomain),    pRawvolume->mediaDomain,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_dismount_data,                XNAME_home_cell,
        sizeof(pRawvolume->homeCell),       pRawvolume->homeCell,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_dismount_data,                XNAME_density,
        sizeof(pRawvolume->denRectName),    pRawvolume->denRectName,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_dismount_data,                XNAME_scratch,
        sizeof(pRawvolume->scratch),        pRawvolume->scratch,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_dismount_data,                XNAME_encrypted,
        sizeof(pRawvolume->encrypted),      pRawvolume->encrypted,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_dismount_data,                XNAME_pool_type,
        sizeof(pRawvolume->subpoolType),    pRawvolume->subpoolType,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_dismount_data,                XNAME_subpool_name,
        sizeof(pRawvolume->subpoolName),    pRawvolume->subpoolName,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_dismount_data,                XNAME_device_address,
        sizeof(pRawvolume->charDevAddr),    pRawvolume->charDevAddr,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_dismount_data,                XNAME_drive_name,
        sizeof(pRawvolume->driveName),      pRawvolume->driveName,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_dismount_data,                XNAME_vtss_name,
        sizeof(pRawvolume->vtssName),       pRawvolume->vtssName,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_dismount_data,                XNAME_drive_location_id,
        sizeof(pRawvolume->driveLocId),     pRawvolume->driveLocId,
        BLANKFILL, NOBITVALUE, 0, 0,
        XNAME_dismount_data,                XNAME_drive_location_zone,
        sizeof(pRawvolume->driveLocZone),   pRawvolume->driveLocZone,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 dismountDataElementCount = sizeof(dismountDataXmlstruct) / 
                                                   sizeof(struct XMLSTRUCT);

    struct XMLSTRUCT    libAddrXmlstruct[]  =
    {
        XNAME_library_address,              XNAME_acs,
        sizeof(pRawvolume->cellAcs),        pRawvolume->cellAcs,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_lsm,
        sizeof(pRawvolume->cellLsm),        pRawvolume->cellLsm,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_panel,
        sizeof(pRawvolume->cellPanel),      pRawvolume->cellPanel,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_row,
        sizeof(pRawvolume->cellRow),        pRawvolume->cellRow,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_column,
        sizeof(pRawvolume->cellColumn),     pRawvolume->cellColumn,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 libAddrElementCount = sizeof(libAddrXmlstruct) / 
                                              sizeof(struct XMLSTRUCT);

    struct XMLSTRUCT    driveAddrXmlstruct[] =
    {
        XNAME_drive_library_address,        XNAME_acs,
        sizeof(pRawvolume->driveAcs),       pRawvolume->driveAcs,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_library_address,        XNAME_lsm,
        sizeof(pRawvolume->driveLsm),       pRawvolume->driveLsm,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_library_address,        XNAME_panel,
        sizeof(pRawvolume->drivePanel),     pRawvolume->drivePanel,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_library_address,        XNAME_drive_number,
        sizeof(pRawvolume->driveNumber),    pRawvolume->driveNumber,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 driveAddrElementCount = sizeof(driveAddrXmlstruct) / 
                                                sizeof(struct XMLSTRUCT);

    memset(pRawmount, 0, sizeof(struct RAWMOUNT));

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

    pDismountDataXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                   pXmlparse->pHocXmlelem,
                                                   XNAME_dismount_data);

    if (pDismountDataXmlelem != NULL)
    {
        FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                          pDismountDataXmlelem,
                                          &dismountDataXmlstruct[0],
                                          dismountDataElementCount);

        pLibAddrXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                  pDismountDataXmlelem,
                                                  XNAME_library_address);

        if (pLibAddrXmlelem != NULL)
        {
            FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                              pLibAddrXmlelem,
                                              &libAddrXmlstruct[0],
                                              libAddrElementCount);
        }

        pDriveAddrXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                    pDismountDataXmlelem,
                                                    XNAME_drive_library_address);

        if (pDriveAddrXmlelem != NULL)
        {
            FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                              pDriveAddrXmlelem,
                                              &driveAddrXmlstruct[0],
                                              driveAddrElementCount);
        }
    }

    TRMEMI(TRCI_XAPI,
           pRawmount, sizeof(struct RAWMOUNT),
           "RAWMOUNT:\n");

    if (pRawvolume->volser[0] > ' ')
    {
        memcpy(pDismount_Response->vol_id.external_label,
               pRawvolume->volser,
               sizeof(pRawvolume->volser));
    }
    else
    {
        memcpy(pDismount_Response->vol_id.external_label,
               "??????",
               sizeof(pDismount_Response->vol_id.external_label));
    }

    if (pRawvolume->driveAcs[0] > ' ')
    {
        FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->driveAcs,
                                      sizeof(pRawvolume->driveAcs),
                                      &locInt);

        pDismount_Response->drive_id.panel_id.lsm_id.acs = (ACS) locInt;

        if (pRawvolume->driveLsm[0] > ' ')
        {
            FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->driveLsm,
                                          sizeof(pRawvolume->driveLsm),
                                          &locInt);

            pDismount_Response->drive_id.panel_id.lsm_id.lsm = (LSM) locInt;

            if (pRawvolume->drivePanel[0] > ' ')
            {
                FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->drivePanel,
                                              sizeof(pRawvolume->drivePanel),
                                              &locInt);

                pDismount_Response->drive_id.panel_id.panel = (PANEL) locInt;

                if (pRawvolume->driveNumber[0] > ' ')
                {
                    FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->driveNumber,
                                                  sizeof(pRawvolume->driveNumber),
                                                  &locInt);

                    pDismount_Response->drive_id.drive = (DRIVE) locInt;
                }
            }
        }
    }

    return STATUS_SUCCESS;
}



