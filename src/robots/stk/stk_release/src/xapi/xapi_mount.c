/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_mount.c                                     */
/** Description:    XAPI MOUNT processor.                            */
/**                                                                  */
/**                 Mount the specified volume on the specified      */
/**                 tape drive.                                      */
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
#define MOUNT_SEND_TIMEOUT         5   /* TIMEOUT values in seconds  */       
#define MOUNT_RECV_TIMEOUT_1ST     120
#define MOUNT_RECV_TIMEOUT_NON1ST  900

/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void initMountResponse(struct XAPIREQE *pXapireqe,
                              char            *pMountResponse);

static int convertMountRequest(struct XAPICVT   *pXapicvt,
                               struct XAPIREQE  *pXapireqe,
                               char            **ptrXapiBuffer,
                               int              *pXapiBufferSize);

static int extractMountResponse(struct XAPICVT  *pXapicvt,
                                struct XAPIREQE *pXapireqe,
                                char            *pMountResponse,
                                struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_mount                                        */
/** Description:   The XAPI MOUNT processor.                         */
/**                                                                  */
/** Mount the specified volume on the specified tape drive.          */
/**                                                                  */
/** If the MESSAGE_HEADER.lock_id is NOT 0 (NO_LOCK_ID), then        */
/** validate that the specified drive is locked by the specified     */
/** LOCKID.  Also validate that the specified volume is either       */
/** locked by the specified LOCKID or that the specified volume      */
/** is unlocked altogether.                                          */
/**                                                                  */
/** The ACSAPI format MOUNT request is translated into an            */
/** XAPI XML format <mount> request; the XAPI XML request is then    */
/** transmitted to the server via TCP/IP;  The received XAPI XML     */
/** response is then translated into the ACSAPI MOUNT response.      */
/**                                                                  */
/** The MOUNT command is NOT allowed to proceed when the             */
/** XAPI client is in the IDLE state.                                */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_mount"

extern int xapi_mount(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    int                 mountRC             = STATUS_SUCCESS;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    MOUNT_REQUEST      *pMount_Request      = (MOUNT_REQUEST*) pXapireqe->pAcsapiBuffer;
    MOUNT_RESPONSE      wkMount_Response;
    MOUNT_RESPONSE     *pMount_Response     = &wkMount_Response;

    TRMSGI(TRCI_XAPI,
           "Entered; MOUNT request=%08X, size=%d, "
           "MOUNT response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pMount_Response,
           sizeof(MOUNT_RESPONSE));

    initMountResponse(pXapireqe,
                      (char*) pMount_Response);

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pMount_Response,
                            (sizeof(MOUNT_RESPONSE)));

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    lastRC = convertMountRequest(pXapicvt,
                                 pXapireqe,
                                 &pXapiBuffer,
                                 &xapiBufferSize);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from convertMountRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pMount_Response,
                          sizeof(MOUNT_RESPONSE),
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
                      MOUNT_SEND_TIMEOUT,       
                      MOUNT_RECV_TIMEOUT_1ST,   
                      MOUNT_RECV_TIMEOUT_NON1ST,
                      (void**) &pXmlparse);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from xapi_tcp(pBuffer=%08X, size=%d, pXmlparse=%08X)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize,
           pXmlparse);

    if (lastRC != STATUS_SUCCESS)
    {
        mountRC = lastRC;
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
    /* Now generate the MOUNT ACSAPI response.                       */
    /*****************************************************************/
    if (mountRC == STATUS_SUCCESS)
    {
        lastRC = extractMountResponse(pXapicvt,
                                      pXapireqe,
                                      (char*) pMount_Response,
                                      pXmlparse);

        TRMSGI(TRCI_XAPI,"lastRC=%d from extractMountResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            mountRC = lastRC;
        }
    }

    if (mountRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pMount_Response,
                          sizeof(MOUNT_RESPONSE),
                          mountRC);
    }
    else
    {
        xapi_fin_response(pXapireqe,
                          (char*) pMount_Response,
                          sizeof(MOUNT_RESPONSE));
    }

    /*****************************************************************/
    /* When done, free the XMLPARSE and associated structures.       */
    /*****************************************************************/
    if (pXmlparse != NULL)
    {
        FN_FREE_HIERARCHY_STORAGE(pXmlparse);
    }

    return mountRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: initMountResponse                                 */
/** Description:   Initialize the ACSAPI MOUNT response.             */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI MOUNT response consists of:                           */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   MOUNT_RESPONSE data consisting of                           */
/**      i.   vol_id.external_label                                  */
/**      ii.  drive_id consisting of                                 */
/**           a.  panel_id.lsm_id.acs                                */
/**           b.  panel_id.lsm_id.lsm                                */
/**           c.  panel_id.panel                                     */
/**           d.  driveid                                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "initMountResponse"

static void initMountResponse(struct XAPIREQE *pXapireqe,
                              char            *pMountResponse)
{
    MOUNT_REQUEST      *pMount_Request      = (MOUNT_REQUEST*) pXapireqe->pAcsapiBuffer;
    MOUNT_RESPONSE     *pMount_Response     = (MOUNT_RESPONSE*) pMountResponse;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Initialize MOUNT response.                                    */
    /*****************************************************************/
    memset((char*) pMount_Response, 0, sizeof(MOUNT_RESPONSE));

    memcpy((char*) &(pMount_Response->request_header),
           (char*) &(pMount_Request->request_header),
           sizeof(REQUEST_HEADER));

    pMount_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pMount_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pMount_Response->message_status.status = STATUS_SUCCESS;
    pMount_Response->message_status.type = TYPE_NONE;

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: convertMountRequest                               */
/** Description:   Build an XAPI <mount> request.                    */
/**                                                                  */
/** Convert the ACSAPI format MOUNT request into an XAPI             */
/** XML format <mount> request.                                      */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI MOUNT request consists of:                            */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   MOUNT_REQUEST data consisting of                            */
/**      i.   vol_id.external_label                                  */
/**      ii.  count                                                  */
/**      ii.  drive_id[count] entries consisting of                  */
/**           a.   panel_id.lsm_id.acs                               */
/**           b.   panel_id.lsm_id.lsm                               */
/**           c.   panel_id.panel                                    */
/**           d.   driveid                                           */
/**                                                                  */
/** NOTE: In addition, the MESSAGE_HEADER.message_options READONLY   */
/** bit indicates whether the volume is mounted for read only.       */
/** The MESSAGE_HEADER.message_options BYPASS allows mounts to occur */
/** occur if the volser is virtual (meaning undefined) or unreadable */
/** and the media type is 3480 or compatible.                        */
/**                                                                  */
/** NOTE: Event though CDK call may specify a count > 1, the CDK     */
/** will only honor requests where the count specified is 1.         */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <mount> (for specific) request consists of:         */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <mount>                                                      */
/**       <user_name>hhhhhhhh</user_name>                            */
/**       <host_name>hhhhhhhh</host_name>                            */
/**       <volser>vvvvvv</volser>                                    */
/**       <drive_name>ccua</drive_name>                (N/A CDK) OR  */
/**       <drive_location_id>aa:ll:pp:dd:zz</drive_location_id>  OR  */
/**       <read_only>"Yes"|"No"</read_only>                          */
/**       <management_class>mmmmmmmm</management_class>              */
/**       <label_type>llll</label_type>                              */
/**                                                                  */
/**       The following parameters are for MOUNT scratch requests    */
/**       <scratch>"Yes"|"No"</scratch>                              */
/**       <subpool_name>sssssssssssss</subpool_name>             OR  */
/**       <subpool_index>nnn</subpool_index>                         */
/**                                                                  */
/**     </mount>                                                     */
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
/** (3) The CDK MOUNT command does not allow specification of        */
/**     the management class.                                        */
/** (4) <subpool_name> or <subpool_index>, <media>, <rectech>,       */
/**     and <label_type> are only specified in the XAPI MOUNT        */
/**     request for CDK MOUNT SCRATCH request.                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "convertMountRequest"

static int convertMountRequest(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char           **ptrXapiBuffer,
                               int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;
    int                 mountCount;
    int                 lockId;

    struct LIBDRVID     libdrvid;
    struct LIBDRVID    *pLibdrvid           = &libdrvid;
    struct XAPICFG     *pXapicfg            = NULL;
    struct RAWQLOCK     rawqlock;
    struct RAWQLOCK    *pRawqlock           = &rawqlock;

    char                driveLocIdString[24];

    char               *pXapiRequest        = NULL;
    MOUNT_REQUEST      *pMount_Request      = (MOUNT_REQUEST*) pXapireqe->pAcsapiBuffer;
    MESSAGE_HEADER     *pMessage_Header     = &(pMount_Request->request_header.message_header);
    struct XMLPARSE    *pXmlparse           = NULL;
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    mountCount = pMount_Request->count;
    lockId = (int) pMessage_Header->lock_id;

    TRMSGI(TRCI_XAPI,
           "Entered; mountCount=%d, lockId=%d\n",
           mountCount,
           lockId);

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

    if (mountCount < 1)
    {
        return STATUS_COUNT_TOO_SMALL;
    }

    if (mountCount > 1)
    {
        return STATUS_COUNT_TOO_LARGE;
    }

    memset((char*) pLibdrvid, 0 , sizeof(struct LIBDRVID));
    memset(driveLocIdString, 0, sizeof(driveLocIdString)); 

    pLibdrvid->acs = (unsigned char) pMount_Request->drive_id[0].panel_id.lsm_id.acs;
    pLibdrvid->lsm = (unsigned char) pMount_Request->drive_id[0].panel_id.lsm_id.lsm;
    pLibdrvid->panel = (unsigned char) pMount_Request->drive_id[0].panel_id.panel;
    pLibdrvid->driveNumber = (unsigned char) pMount_Request->drive_id[0].drive;

    pXapicfg = xapi_config_search_libdrvid(pXapicvt,
                                           pXapireqe,
                                           pLibdrvid);

    if (pXapicfg == NULL)
    {
        return STATUS_DRIVE_NOT_IN_LIBRARY;   
    }

    /*****************************************************************/
    /* For a specific mount:                                         */
    /* Only check drive and volume locks if a non-zero lock_id       */
    /* is specified.                                                 */
    /*                                                               */
    /* If no NO_LOCK_ID specified, then we do not really care if     */
    /* the drive and volume are locked!  This is probably not the    */
    /* way that lock_id processing should work, but it is the way    */
    /* that LibStation handled a mount lock_id specification.        */
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
               "MOUNT DRIVE lock status=%d; RAWQLOCK:\n",
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
                                    pMount_Request->vol_id.external_label,
                                    lockId);

        TRMEMI(TRCI_XAPI, pRawqlock, sizeof(struct RAWQLOCK),
               "MOUNT VOLUME lock status=%d; RAWQLOCK:\n",
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
                                      XNAME_mount,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_volser,
                                      pMount_Request->vol_id.external_label,
                                      6);

    if (pMessage_Header->message_options & READONLY)
    {
        TRMSGI(TRCI_XAPI,
               "READONLY option specified\n");

        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_read_only,
                                          XCONTENT_YES,
                                          0);
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
/** Function Name: extractMountResponse                              */
/** Description:   Extract the <mount_request> response.             */
/**                                                                  */
/** Parse the response of the XAPI XML <mount>                       */
/** request and update the appropriate fields of the                 */
/** ACSAPI MOUNT response.                                           */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML MOUNT response consists of:                         */
/**==================================================================*/
/** <libreply>                                                       */
/**   <mount_request>                                                */
/**     <header>                                                     */
/**       header element children                                    */
/**     </header>                                                    */
/**     <mount_data>                                                 */
/**       <volser>vvvvvv</volser>                                    */
/**       <library_address>                                          */
/**         <acs>aa</acs>                                            */
/**         <lsm>ll</lsm>                                            */
/**         <panel>pp</panel>                                        */
/**         <row>rr</row>                                            */
/**         <column>cc</column>                                      */
/**       </library_address>                                         */
/**       <device_address>ccua</device_address>                      */
/**       <drive_name>ccua</drive_name>                              */
/**       <drive_location_id>R:aa:ll:pp:dd</drive_location_id>       */
/**       <drive_library_address>                                    */
/**         <acs>aa</acs>                                            */
/**         <lsm>ll</lsm>                                            */
/**         <panel>pp</panel>                                        */
/**         <drive_number>dd</drive_number>                          */
/**       </drive_library_address>                                   */
/**       <drive_location_zone>zzzz</drive_location_zone>            */
/**       <media>mmmmmmmm</media>                                    */
/**       <media_type>n</media_type>                                 */
/**       <vtss_name>nnnnnnnn</vtss_name>                            */
/**       <resident_vtss>nnnnnnnn</resident_vtss>                    */
/**       <density>rrrrrrrr</density>                                */
/**       <encrypted>"Yes"|"No"</encrypted>                          */
/**       <scratch>"Yes"|"No"</scratch>                              */
/**       <pool_type>"SCRATCH"|"MVC"|"EXTERNAL"</pool_type>          */
/**       <pool_name>sssssssssssss</pool_name>                       */
/**     </mount_data>                                                */
/**     <exceptions>                                                 */
/**       <reason>ccc...ccc</reason>                                 */
/**       ...repeated <reason> entries                               */
/**     </exceptions>                                                */
/**     <uui_return_code>nnnn</uui_return_code>                      */
/**     <uui_reason_code>nnnn</uui_reason_code>                      */
/**   </mount_request>                                               */
/** </libreply>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractMountResponse"

static int extractMountResponse(struct XAPICVT  *pXapicvt,
                                struct XAPIREQE *pXapireqe,
                                char            *pMountResponse,
                                struct XMLPARSE *pXmlparse)
{
    int                 lastRC;
    int                 locInt;

    struct XMLELEM     *pMountDataXmlelem;
    struct XMLELEM     *pLibAddrXmlelem;
    struct XMLELEM     *pDriveAddrXmlelem;
    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;
    struct RAWCOMMON   *pRawcommon          = &(pXapicommon->rawcommon);
    struct RAWMOUNT     rawmount;           
    struct RAWMOUNT    *pRawmount           = &rawmount;
    struct RAWVOLUME   *pRawvolume          = &(pRawmount->rawvolume);

    MOUNT_RESPONSE     *pMount_Response     = (MOUNT_RESPONSE*) pMountResponse;

    struct XMLSTRUCT    mountDataXmlstruct[]     =
    {
        XNAME_mount_data,                   XNAME_volser,
        sizeof(pRawvolume->volser),         pRawvolume->volser,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_media,
        sizeof(pRawvolume->media),          pRawvolume->media,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_media_type,
        sizeof(pRawvolume->mediaType),      pRawvolume->mediaType,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_media_domain,
        sizeof(pRawvolume->mediaDomain),    pRawvolume->mediaDomain,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_home_cell,
        sizeof(pRawvolume->homeCell),       pRawvolume->homeCell,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_density,
        sizeof(pRawvolume->denRectName),    pRawvolume->denRectName,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_scratch,
        sizeof(pRawvolume->scratch),        pRawvolume->scratch,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_encrypted,
        sizeof(pRawvolume->encrypted),      pRawvolume->encrypted,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_pool_type,
        sizeof(pRawvolume->subpoolType),    pRawvolume->subpoolType,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_subpool_name,
        sizeof(pRawvolume->subpoolName),    pRawvolume->subpoolName,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_device_address,
        sizeof(pRawvolume->charDevAddr),    pRawvolume->charDevAddr,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_drive_name,
        sizeof(pRawvolume->driveName),      pRawvolume->driveName,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_vtss_name,
        sizeof(pRawvolume->vtssName),       pRawvolume->vtssName,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_drive_location_id,
        sizeof(pRawvolume->driveLocId),     pRawvolume->driveLocId,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_drive_location_zone,
        sizeof(pRawvolume->driveLocZone),   pRawvolume->driveLocZone,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 mountDataElementCount = sizeof(mountDataXmlstruct) / 
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

    pMountDataXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                pXmlparse->pHocXmlelem,
                                                XNAME_mount_data);

    if (pMountDataXmlelem != NULL)
    {
        FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                          pMountDataXmlelem,
                                          &mountDataXmlstruct[0],
                                          mountDataElementCount);

        pLibAddrXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                  pMountDataXmlelem,
                                                  XNAME_library_address);

        if (pLibAddrXmlelem != NULL)
        {
            FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                              pLibAddrXmlelem,
                                              &libAddrXmlstruct[0],
                                              libAddrElementCount);
        }

        pDriveAddrXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                    pMountDataXmlelem,
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
        memcpy(pMount_Response->vol_id.external_label,
               pRawvolume->volser,
               sizeof(pRawvolume->volser));
    }
    else
    {
        memcpy(pMount_Response->vol_id.external_label,
               "??????",
               sizeof(pMount_Response->vol_id.external_label));
    }

    if (pRawvolume->driveAcs[0] > ' ')
    {
        FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->driveAcs,
                                      sizeof(pRawvolume->driveAcs),
                                      &locInt);

        pMount_Response->drive_id.panel_id.lsm_id.acs = (ACS) locInt;

        if (pRawvolume->driveLsm[0] > ' ')
        {
            FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->driveLsm,
                                          sizeof(pRawvolume->driveLsm),
                                          &locInt);

            pMount_Response->drive_id.panel_id.lsm_id.lsm = (LSM) locInt;

            if (pRawvolume->drivePanel[0] > ' ')
            {
                FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->drivePanel,
                                              sizeof(pRawvolume->drivePanel),
                                              &locInt);

                pMount_Response->drive_id.panel_id.panel = (PANEL) locInt;

                if (pRawvolume->driveNumber[0] > ' ')
                {
                    FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->driveNumber,
                                                  sizeof(pRawvolume->driveNumber),
                                                  &locInt);

                    pMount_Response->drive_id.drive = (DRIVE) locInt;
                }
            }
        }
    }

    return STATUS_SUCCESS;
}



