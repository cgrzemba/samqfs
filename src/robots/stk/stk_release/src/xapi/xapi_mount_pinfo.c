/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_mount_pinfo.c                               */
/** Description:    XAPI MOUNT PINFO processor.                      */
/**                                                                  */
/**                 Mount either a scratch or specific volume        */
/**                 on the specified tape drive using the            */
/**                 specified scratch subpool index,                 */
/**                 management class, or media type code.            */
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
#define MOUNT_SEND_TIMEOUT         5   /* TIMEOUT values in seconds  */       
#define MOUNT_RECV_TIMEOUT_1ST     120
#define MOUNT_RECV_TIMEOUT_NON1ST  900


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void initMountPinfoResponse(struct XAPIREQE *pXapireqe,
                                   char            *pMountPinfoResponse);

static int convertMountPinfoRequest(struct XAPICVT   *pXapicvt,
                                    struct XAPIREQE  *pXapireqe,
                                    char            **ptrXapiBuffer,
                                    int              *pXapiBufferSize);

static int extractMountPinfoResponse(struct XAPICVT  *pXapicvt,
                                     struct XAPIREQE *pXapireqe,
                                     char            *pMountPinfoResponse,
                                     struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_mount_pinfo                                  */
/** Description:   The XAPI MOUNT PINFO processor.                   */
/**                                                                  */
/** Mount either a scratch or specific volume on the specified       */
/** drive using the specified scratch subpool index,                 */
/** management class, or media type code.                            */
/**                                                                  */
/** The MESSAGE_HEADER.message_options SCRATCH flag determines       */
/** whether the request is for a scratch or specific volume.  If     */
/** MESSAGE_HEADER.message_options SCRATCH flag is set, then the     */
/** input volser is ignored.                                         */
/**                                                                  */
/** The ACSAPI MOUNT PINFO command implies that the input jobname,   */
/** stepname, and dsnname are used to select TAPEREQ policy on the   */
/** LibStation server.   This is not true after LibStation 5.0.      */
/** Server tape policy can only be specified using the input         */
/** POOLID (scratch subpool index), management class, and media      */
/** type code specification.                                         */
/**                                                                  */
/** Also, the ACSAPI MOUNT PINFO command states that virtual tape    */
/** drives are specified using a VTSS name, and LSM, PANEL, and      */
/** drive number of 0.  This is also untrue: virtual tape drives     */
/** are selected as any other tape drive in the XAPI client.         */
/**                                                                  */
/** If the MESSAGE_HEADER.lock_id is NOT 0 (NO_LOCK_ID), then        */
/** validate that the specified drive is locked by the specified     */
/** LOCKID.  If this is a specific volume request, then also         */
/** validate that the specified volume is either locked by the       */
/** specified LOCKID or that the specified volume is unlocked        */
/** altogether.  If this is a scratch request, then we do not know   */
/** the actual volser until the MOUNT PINFO is complete, so we       */
/** cannot validate the volser LOCKID.                               */
/**                                                                  */
/** The ACSAPI format MOUNT PINFO request is translated into an      */
/** XAPI XML format <mount> request; the XAPI XML request            */
/** is then transmitted to the server via TCP/IP;  The received      */
/** XAPI XML response is then translated into the ACSAPI             */
/** MOUNT PINFO response.                                            */
/**                                                                  */
/** The MOUNT PINFO command is NOT allowed to proceed when the       */
/** XAPI client is in the IDLE state.                                */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_mount_pinfo"

extern int xapi_mount_pinfo(struct XAPICVT  *pXapicvt,
                            struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    int                 mountRC             = STATUS_SUCCESS;
    int                 xapiBufferSize      = 0;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    MOUNT_PINFO_REQUEST  *pMount_Pinfo_Request  = 
    (MOUNT_PINFO_REQUEST*) pXapireqe->pAcsapiBuffer;
    MOUNT_PINFO_RESPONSE  wkMount_Pinfo_Response;
    MOUNT_PINFO_RESPONSE *pMount_Pinfo_Response = &wkMount_Pinfo_Response;

    TRMSGI(TRCI_XAPI,
           "Entered; MOUNT PINFO request=%08X, size=%d, "
           "MOUNT PINFO response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pMount_Pinfo_Response,
           sizeof(MOUNT_PINFO_RESPONSE));

    initMountPinfoResponse(pXapireqe,
                           (char*) pMount_Pinfo_Response);

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pMount_Pinfo_Response,
                            (sizeof(MOUNT_PINFO_RESPONSE)));

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    lastRC = convertMountPinfoRequest(pXapicvt,
                                      pXapireqe,
                                      &pXapiBuffer,
                                      &xapiBufferSize);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from convertMountPinfoRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pMount_Pinfo_Response,
                          sizeof(MOUNT_PINFO_RESPONSE),
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
    /* Now generate the MOUNT PINFO ACSAPI response.                 */
    /*****************************************************************/
    if (mountRC == STATUS_SUCCESS)
    {
        lastRC = extractMountPinfoResponse(pXapicvt,
                                           pXapireqe,
                                           (char*) pMount_Pinfo_Response,
                                           pXmlparse);

        TRMSGI(TRCI_XAPI,"lastRC=%d from extractMountPinfoResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            mountRC = lastRC;
        }
    }

    if (mountRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pMount_Pinfo_Response,
                          sizeof(MOUNT_PINFO_RESPONSE),
                          mountRC);
    }
    else
    {
        xapi_fin_response(pXapireqe,
                          (char*) pMount_Pinfo_Response,
                          sizeof(MOUNT_PINFO_RESPONSE));
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
/** Function Name: initMountPinfoResponse                            */
/** Description:   Initialize the ACSAPI MOUNT PINFO response.       */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI MOUNT PINFO response consists of:                     */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   MOUNT_PINFO_RESPONSE data consisting of                     */
/**      i.   pool_id consisting of:                                 */
/**           a.  pool (subpool index)                               */
/**      ii.  drive_id consisting of                                 */
/**           a.  panel_id.lsm_id.acs                                */
/**           b.  panel_id.lsm_id.lsm                                */
/**           c.  panel_id.panel                                     */
/**           d.  driveid                                            */
/**      iii. vol_id.external_label                                  */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "initMountPinfoResponse"

static void initMountPinfoResponse(struct XAPIREQE *pXapireqe,
                                   char            *pMountPinfoResponse)
{
    MOUNT_PINFO_REQUEST  *pMount_Pinfo_Request  = 
    (MOUNT_PINFO_REQUEST*) pXapireqe->pAcsapiBuffer;
    MOUNT_PINFO_RESPONSE *pMount_Pinfo_Response = 
    (MOUNT_PINFO_RESPONSE*) pMountPinfoResponse;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Initialize MOUNT response.                                    */
    /*****************************************************************/
    memset((char*) pMount_Pinfo_Response, 0, sizeof(MOUNT_PINFO_RESPONSE));

    memcpy((char*) &(pMount_Pinfo_Response->request_header),
           (char*) &(pMount_Pinfo_Request->request_header),
           sizeof(REQUEST_HEADER));

    pMount_Pinfo_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pMount_Pinfo_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pMount_Pinfo_Response->message_status.status = STATUS_SUCCESS;
    pMount_Pinfo_Response->message_status.type = TYPE_NONE;

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: convertMountPinfoRequest                          */
/** Description:   Build an XAPI <mount> request.                    */
/**                                                                  */
/** Convert the ACSAPI format MOUNT PINFO request into an            */
/** XAPI XML format <mount>.                                         */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI MOUNT PINFO request consists of:                      */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   MOUNT_PINFO_REQUEST data consisting of                      */
/**      i.   vol_id consisting of:                                  */
/**           a.   external_label (6 character volser)               */
/**      ii.  pool_id consisting of:                                 */
/**           a.   pool (subpool index)                              */
/**      iii. mgmt_clas (management class name string)               */
/**      iv.  media_type code (may be ANY_MEDIA_TYPE)                */
/**      v.   job_name                                               */
/**      vi.  dataset_name                                           */
/**      vii. step_name                                              */
/**      viii.drive_id entries consisting of                         */
/**           a.   panel_id.lsm_id.acs                               */
/**           b.   panel_id.lsm_id.lsm                               */
/**           c.   panel_id.panel                                    */
/**           d.   driveid                                           */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <mount> request consists of:                        */
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
/**       <scratch>"Yes"|"No"</scratch>                              */
/**       <drive_name>ccua</drive_name>                (N/A CDK) OR  */
/**       <drive_location_id>aa:ll:pp:dd:zz</drive_location_id>  OR  */
/**       <management_class>mmmmmmmm</management_class>              */
/**       <subpool_name>sssssssssssss</subpool_name>             OR  */
/**       <subpool_index>nnn</subpool_index>                         */
/**       <label_type>llll</label_type>                              */
/**                                                                  */
/**       The following parameters are for MOUNT specific requests   */
/**       <volser>vvvvvv</volser>                                    */
/**       <read_only>"Yes"|"No"</read_only>                          */
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
/** (2) The ACSAPI MOUNT PINFO request allows specification of the   */
/**     jobname, stepname, and dsname to influence the mount.        */
/**     However, as tape policy lookup is no longer an HSC server    */
/**     function, those attributes cannot (and have not) influenced  */
/**     HSC mount since HSC release 5.0.   We will however pass      */
/**     those attributes to the xapi_request_header() function       */
/**     to include those attributes as part of the XML <header>.     */
/** (3) The XAPI request translated from the CDK ACSAPI will always  */
/**     specify the <drive_location> version of the drive address.   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "convertMountPinfoRequest"

static int convertMountPinfoRequest(struct XAPICVT  *pXapicvt,
                                    struct XAPIREQE *pXapireqe,
                                    char           **ptrXapiBuffer,
                                    int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;
    int                 lockId;
    short               subpoolIndex        = 0;
    char                mediaType           = ANY_MEDIA_TYPE;
    char                scratchFlag         = FALSE;

    struct XMLHDRINFO   xmlhdrinfo;
    struct XMLHDRINFO  *pXmlhdrinfo         = &xmlhdrinfo;
    struct LIBDRVID     libdrvid;
    struct LIBDRVID    *pLibdrvid           = &libdrvid;
    struct XAPICFG     *pXapicfg            = NULL;
    struct XAPISCRPOOL *pXapiscrpool        = NULL;
    struct XAPIMEDIA   *pXapimedia          = NULL;
    struct RAWQLOCK     rawqlock;
    struct RAWQLOCK    *pRawqlock           = &rawqlock;

    char                driveLocIdString[24];
    char                volserString[XAPI_VOLSER_SIZE + 1];

    char               *pXapiRequest        = NULL;

    MOUNT_PINFO_REQUEST *pMount_Pinfo_Request = 
    (MOUNT_PINFO_REQUEST*) pXapireqe->pAcsapiBuffer;

    MESSAGE_HEADER     *pMessage_Header     = 
    &(pMount_Pinfo_Request->request_header.message_header);

    struct XMLPARSE    *pXmlparse           = NULL;
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    if (pMessage_Header->message_options & SCRATCH)
    {
        scratchFlag = TRUE;
    }

    if (scratchFlag)
    {
        if (pMount_Pinfo_Request->pool_id.pool > 0)
        {
            subpoolIndex = pMount_Pinfo_Request->pool_id.pool;
        }

        if (pMount_Pinfo_Request->media_type > 0)
        {
            mediaType = pMount_Pinfo_Request->media_type;
        }
    }

    lockId = (int) pMessage_Header->lock_id;

    TRMSGI(TRCI_XAPI,
           "Entered; scratchFlag=%d, subpoolIndex=%d, mediaType=%d, lockId=%d\n",
           scratchFlag,
           subpoolIndex,
           mediaType,
           lockId);

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

    memset((char*) pLibdrvid, 0 , sizeof(struct LIBDRVID));
    memset(driveLocIdString, 0, sizeof(driveLocIdString)); 

    pLibdrvid->acs = (unsigned char) pMount_Pinfo_Request->drive_id.panel_id.lsm_id.acs;
    pLibdrvid->lsm = (unsigned char) pMount_Pinfo_Request->drive_id.panel_id.lsm_id.lsm;
    pLibdrvid->panel = (unsigned char) pMount_Pinfo_Request->drive_id.panel_id.panel;
    pLibdrvid->driveNumber = (unsigned char) pMount_Pinfo_Request->drive_id.drive;

    pXapicfg = xapi_config_search_libdrvid(pXapicvt,
                                           pXapireqe,
                                           pLibdrvid);

    if (pXapicfg == NULL)
    {
        return STATUS_DRIVE_NOT_IN_LIBRARY;   
    }

    if (subpoolIndex > 0)
    {
        pXapiscrpool = xapi_scrpool_search_index(pXapicvt,
                                                 pXapireqe,
                                                 subpoolIndex);

        if (pXapiscrpool == NULL)
        {
            return STATUS_POOL_NOT_FOUND;   
        }
    }

    if (mediaType != ANY_MEDIA_TYPE)
    {
        pXapimedia = xapi_media_search_type(pXapicvt,
                                            pXapireqe,
                                            mediaType);

        if (pXapimedia == NULL)
        {
            return STATUS_INVALID_MEDIA_TYPE;   
        }
    }

    if (!(scratchFlag))
    {
        if (pMount_Pinfo_Request->vol_id.external_label[0] <= ' ')
        {
            return STATUS_INVALID_VOLUME;   
        }
        else
        {
            memset(volserString, 0, sizeof(volserString));

            memcpy(volserString, 
                   pMount_Pinfo_Request->vol_id.external_label,
                   XAPI_VOLSER_SIZE);
        }
    }

    /*****************************************************************/
    /* For a specific mount:                                         */
    /* Only check drive and volume locks if a non-zero lock_id       */
    /* is specified.                                                 */
    /*                                                               */
    /* For a scratch mount (i.e. we do not know volser):             */
    /* Only check drive resource lock if a non-zero lock_id is       */
    /* specified.                                                    */
    /*                                                               */
    /* If no NO_LOCK_ID specified, then we do not really care if     */
    /* the drive and/or volume are locked!  This is probably not the */
    /* way that lock_id processing should work, but it is the way    */
    /* that LibStation handles a mount lock_id specification.        */
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
               "MOUNT PINFO DRIVE lock status=%d; RAWQLOCK:\n",
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

        if (!(scratchFlag))
        {
            memset((char*) pRawqlock,
                   0,
                   sizeof(struct RAWQLOCK));

            lastRC = xapi_qlock_vol_one(pXapicvt,
                                        pXapireqe,
                                        pRawqlock,
                                        pMount_Pinfo_Request->vol_id.external_label,
                                        lockId);

            TRMEMI(TRCI_XAPI, pRawqlock, sizeof(struct RAWQLOCK),
                   "MOUNT PINFO VOLUME lock status=%d; RAWQLOCK:\n",
                   pRawqlock->queryRC);

            if (pRawqlock->queryRC != STATUS_SUCCESS)
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

    /*****************************************************************/
    /* Extract the PINFO jobname, stepname, and dsname.              */
    /* While these attributes will not affect the mount, they will   */
    /* be passed in the XML <header> for the transaction.            */
    /*****************************************************************/
    memset((char*) pXmlhdrinfo, 0 , sizeof(struct XMLHDRINFO));

    if (pMount_Pinfo_Request->job_name.job_name[0] > ' ')
    {
        memcpy(pXmlhdrinfo->jobname,
               pMount_Pinfo_Request->job_name.job_name,
               sizeof(pXmlhdrinfo->jobname));
    }

    if (pMount_Pinfo_Request->step_name.step_name[0] > ' ')
    {
        memcpy(pXmlhdrinfo->stepname,
               pMount_Pinfo_Request->step_name.step_name,
               sizeof(pXmlhdrinfo->stepname));
    }

    if (pMount_Pinfo_Request->dataset_name.dataset_name[0] > ' ')
    {
        memcpy(pXmlhdrinfo->dsname,
               pMount_Pinfo_Request->dataset_name.dataset_name,
               sizeof(pXmlhdrinfo->dsname));
    }

    if (scratchFlag)
    {
        memcpy(pXmlhdrinfo->volType,
               "NONSPEC ",
               sizeof(pXmlhdrinfo->volType));
    }
    else
    {
        memcpy(pXmlhdrinfo->volType,
               "SPECIFIC",
               sizeof(pXmlhdrinfo->volType));

        if (pMount_Pinfo_Request->vol_id.external_label[0] > ' ')
        {
            memcpy(pXmlhdrinfo->volser,
                   pMount_Pinfo_Request->vol_id.external_label,
                   sizeof(pXmlhdrinfo->volser));
        }
    }

    xapi_request_header(pXapicvt,
                        pXapireqe,
                        pXmlhdrinfo,
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

    if (scratchFlag)
    {
        TRMSGI(TRCI_XAPI,
               "SCRATCH option specified\n");

        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_scratch,
                                          XCONTENT_YES,
                                          0);

        if (pXapiscrpool != NULL)
        {
            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pParentXmlelem,
                                              XNAME_subpool_name,
                                              pXapiscrpool->subpoolNameString,
                                              0);
        }

        if (pXapimedia != NULL)
        {
            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pParentXmlelem,
                                              XNAME_media,
                                              pXapimedia->mediaNameString,
                                              0);
        }
    }
    else
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_volser,
                                          volserString,
                                          0);

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
    }

    if (pMount_Pinfo_Request->mgmt_clas.mgmt_clas[0] > ' ')
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_management_class,
                                          pMount_Pinfo_Request->mgmt_clas.mgmt_clas,
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
/** Function Name: extractMountPinfoResponse                         */
/** Description:   Extract the <mount_request> response.             */
/**                                                                  */
/** Parse the response of the XAPI XML <mount>                       */
/** request and update the appropriate fields of the                 */
/** ACSAPI MOUNT PINFO response.                                     */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <mount> response consists of:                       */
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
#define SELF "extractMountPinfoResponse"

static int extractMountPinfoResponse(struct XAPICVT  *pXapicvt,
                                     struct XAPIREQE *pXapireqe,
                                     char            *pMountPinfoResponse,
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

    MOUNT_PINFO_REQUEST *pMount_Pinfo_Request = 
    (MOUNT_PINFO_REQUEST*) pXapireqe->pAcsapiBuffer;
    MOUNT_PINFO_RESPONSE     *pMount_Pinfo_Response     = (MOUNT_PINFO_RESPONSE*) pMountPinfoResponse;

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

    pMount_Pinfo_Response->pool_id.pool = 
    pMount_Pinfo_Request->pool_id.pool; 

    if (pRawvolume->volser[0] > ' ')
    {
        memcpy(pMount_Pinfo_Response->vol_id.external_label,
               pRawvolume->volser,
               sizeof(pRawvolume->volser));
    }
    else
    {
        memcpy(pMount_Pinfo_Response->vol_id.external_label,
               "??????",
               sizeof(pMount_Pinfo_Response->vol_id.external_label));
    }

    if (pRawvolume->driveAcs[0] > ' ')
    {
        FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->driveAcs,
                                      sizeof(pRawvolume->driveAcs),
                                      &locInt);

        pMount_Pinfo_Response->drive_id.panel_id.lsm_id.acs = (ACS) locInt;

        if (pRawvolume->driveLsm[0] > ' ')
        {
            FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->driveLsm,
                                          sizeof(pRawvolume->driveLsm),
                                          &locInt);

            pMount_Pinfo_Response->drive_id.panel_id.lsm_id.lsm = (LSM) locInt;

            if (pRawvolume->drivePanel[0] > ' ')
            {
                FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->drivePanel,
                                              sizeof(pRawvolume->drivePanel),
                                              &locInt);

                pMount_Pinfo_Response->drive_id.panel_id.panel = (PANEL) locInt;

                if (pRawvolume->driveNumber[0] > ' ')
                {
                    FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->driveNumber,
                                                  sizeof(pRawvolume->driveNumber),
                                                  &locInt);

                    pMount_Pinfo_Response->drive_id.drive = (DRIVE) locInt;
                }
            }
        }
    }

    return STATUS_SUCCESS;
}



