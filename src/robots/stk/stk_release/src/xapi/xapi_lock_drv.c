/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */ 
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_lock_drv.c                                  */
/** Description:    XAPI LOCK DRIVE processor.                       */
/**                                                                  */
/**                 Lock the specified drive(s) using the            */
/**                 specified LOCKID.                                */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     09/01/11                          */
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

#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "api/defs_api.h"                              
#include "csi.h"
#include "smccxmlx.h"
#include "srvcommon.h"
#include "xapi.h"


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#define LOCKERR_RESPONSE_SIZE (offsetof(LOCK_RESPONSE, identifier_status)) 
#define LOCK_SEND_TIMEOUT          5   /* TIMEOUT values in seconds  */       
#define LOCK_RECV_TIMEOUT_1ST      120
#define LOCK_RECV_TIMEOUT_NON1ST   900


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int convertLockRequest(struct XAPICVT   *pXapicvt,
                              struct XAPIREQE  *pXapireqe,
                              char            **ptrXapiBuffer,
                              int              *pXapiBufferSize);

static int extractLockResponse(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char            *pLockResponse,
                               struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_lock_drv                                     */
/** Description:   XAPI LOCK DRIVE processor.                        */
/**                                                                  */
/** Lock the specified drive(s) using the specified (i.e.            */
/** already existing) LOCKID, or if the input LOCKID is specified    */
/** as 0 (or NO_LOCK_ID), then lock the specified drive(s)           */
/** under a non-existent (i.e. unsed) LOCKID and return              */
/** the newly created LOCKID.  A LOCKID is simply a number           */
/** between 1 and 32767.                                             */
/**                                                                  */
/** DRIVE LOCK(s) are managed on the server.  The ACSAPI format      */
/** LOCK DRIVE request is translated into an XAPI XML format         */
/** <lock_drive> request; the XAPI XML request is then               */
/** transmitted to the server via TCP/IP;  The received XAPI XML     */
/** response is then translated into one or more ACSAPI              */
/** LOCK DRIVE responses.                                            */
/**                                                                  */
/** The LOCK DRIVE command is NOT allowed to proceed when            */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_lock_drv"

extern int xapi_lock_drv(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe)
{
    int                 lockRC              = STATUS_SUCCESS;
    int                 lastRC;
    int                 i;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    LOCK_REQUEST       *pLock_Request       = (LOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    int                 lockCount           = pLock_Request->count;

    LOCK_RESPONSE       wkLock_Response;
    LOCK_RESPONSE      *pLock_Response      = &wkLock_Response;

    xapi_lock_init_resp(pXapireqe,
                        (char*) pLock_Response,
                        sizeof(LOCK_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; LOCK DRIVE request=%08X, size=%d, count=%d, "
           "MAX_ID=%d, LOCK DRIVE response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           lockCount,
           MAX_ID,
           pLock_Response,
           LOCKERR_RESPONSE_SIZE);

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pLock_Response,
                            LOCKERR_RESPONSE_SIZE);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    lastRC = convertLockRequest(pXapicvt,
                                pXapireqe,
                                &pXapiBuffer,
                                &xapiBufferSize);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from convertLockRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pLock_Response,
                          LOCKERR_RESPONSE_SIZE,
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
                      LOCK_SEND_TIMEOUT,       
                      LOCK_RECV_TIMEOUT_1ST,   
                      LOCK_RECV_TIMEOUT_NON1ST,
                      (void**) &pXmlparse);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from xapi_tcp(pBuffer=%08X, size=%d, pXmlparse=%08X)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize,
           pXmlparse);

    if (lastRC != STATUS_SUCCESS)
    {
        lockRC = lastRC;
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
    /* Now generate the LOCK DRIVE ACSAPI response.                  */
    /*****************************************************************/
    if (lockRC == STATUS_SUCCESS)
    {
        lastRC = extractLockResponse(pXapicvt,
                                     pXapireqe,
                                     (char*) pLock_Response,
                                     pXmlparse);

        TRMSGI(TRCI_XAPI,"lastRC=%d from extractLockResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            lockRC = lastRC;
        }
    }

    if (lockRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pLock_Response,
                          LOCKERR_RESPONSE_SIZE,
                          lockRC);
    }

    /*****************************************************************/
    /* When done, free the XMLPARSE and associated structures.       */
    /*****************************************************************/
    if (pXmlparse != NULL)
    {
        FN_FREE_HIERARCHY_STORAGE(pXmlparse);
    }

    return lockRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: convertLockRequest                                */
/** Description:   Build an XAPI <lock_drive> request.               */
/**                                                                  */
/** Convert the ACSAPI format LOCK DRIVE request into an             */
/** XAPI XML format <lock_drive> request.                            */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI LOCK DRIVE request consists of:                       */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   LOCK DRIVE data consisting of:                              */
/**      i.   type                                                   */
/**      ii.  count                                                  */
/**      iii. DRIVEID[count] data entries consisting of:             */
/**           a.   drive_id.panel_id.lsm_id.acs                      */
/**           b.   drive_id.panel_id.lsm_id.lsm                      */
/**           c.   drive_id.panel_id.panel                           */
/**           d.   drive_id.drive                                    */
/**                                                                  */
/** NOTE: The input LOCKID is in the MESSAGE_HEADER.lock_id field.   */
/**                                                                  */
/** NOTE: The MESSAGE_HEADER.extended_options.WAIT bit determines    */
/**       the LOCK wait option.                                      */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <lock_drive> request consists of:                   */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <lock_drive>                                                 */
/**       <user_label>CCC...CCC</user_label>                         */
/**       <lock_id>0|nnnnn</lock_id>                                 */
/**       <wait_flag>"Yes"|"No"</wait_flag>                          */
/**       <drive_list>                                               */
/**         <drive_location_id>aa:ll:pp:dd:zz</drive_location_id>    */
/**         ...repeated <drive_location_id> entries                  */
/**       </drive_list>                                              */
/**     </lock_drive>                                                */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** NOTES:                                                           */
/** (1) The XAPI <user_name> will default to the                     */
/**     MESSAGE_HEADER.access_id.user_id.                            */
/**     If MESSAGE_HEADER.access_id.user_id is not specified         */
/**     the XAPI <user_name> will default to the XAPICVT.xapiUser.   */
/**     If XAPICVT.xapiUser (RACF userid) is not specified           */
/**     the XAPI <user_name> will default to the geteuid().          */
/**     If the geteuid() cannot be resolved                          */
/**     the XAPI <user_name> will default to "XAPICVT".              */
/** (2) The XAPI <lock_id> is optional.                              */
/**     If "0" or not specified, then a LOCKID in the range 1-32767  */
/**     will be generated for the request.                           */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "convertLockRequest"

static int convertLockRequest(struct XAPICVT  *pXapicvt,
                              struct XAPIREQE *pXapireqe,
                              char           **ptrXapiBuffer,
                              int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 i;
    int                 xapiRequestSize     = 0;
    LOCK_REQUEST       *pLock_Request       = (LOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    MESSAGE_HEADER     *pMessage_Header     = &(pLock_Request->request_header.message_header);
    DRIVEID            *pDriveid            = &(pLock_Request->identifier.drive_id[0]);

    int                 lockCount           = pLock_Request->count;
    int                 lockId              = (int) pMessage_Header->lock_id;
    struct LIBDRVID     libdrvid;
    struct LIBDRVID    *pLibdrvid           = &libdrvid;
    struct XAPICFG     *pXapicfg[MAX_ID];

    char                useridString[EXTERNAL_USERID_SIZE + 1];
    char                lockIdString[6];
    char                waitString[4];
    char                driveLocIdString[24];

    char               *pXapiRequest        = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    if (pMessage_Header->extended_options & WAIT)
    {
        strcpy(waitString, XCONTENT_YES);
    }
    else
    {
        strcpy(waitString, XCONTENT_NO);
    }

    TRMSGI(TRCI_XAPI,
           "Entered; lockCount=%d, lockId=%d, waitString=%s, pDriveid[0]=%08X\n",
           lockCount,
           lockId,
           waitString,
           pDriveid);

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

    if (lockCount < 1)
    {
        return STATUS_COUNT_TOO_SMALL;
    }

    if (lockCount > MAX_ID)
    {
        return STATUS_COUNT_TOO_LARGE;
    }

    if ((lockId < 0) ||
        (lockId > 32767))
    {
        return STATUS_INVALID_LOCKID;
    }
    else
    {
        sprintf(lockIdString, "%05d", lockId);
    }

    /*****************************************************************/
    /* If any of the specified DRIVEID(s) is not found, then         */
    /* the entire request will fail.                                 */
    /*****************************************************************/
    for (i = 0, pDriveid = &(pLock_Request->identifier.drive_id[0]);
        i < lockCount;
        i++, pDriveid++)
    {
        pXapicfg[i] = NULL;
        memset((char*) pLibdrvid, 0 , sizeof(struct LIBDRVID));
        pLibdrvid->acs = (unsigned char) pDriveid->panel_id.lsm_id.acs;
        pLibdrvid->lsm = (unsigned char) pDriveid->panel_id.lsm_id.lsm;
        pLibdrvid->panel = (unsigned char) pDriveid->panel_id.panel;
        pLibdrvid->driveNumber = (unsigned char) pDriveid->drive;

        pXapicfg[i] = xapi_config_search_libdrvid(pXapicvt,
                                                  pXapireqe,
                                                  pLibdrvid);

        if (pXapicfg[i] == NULL)
        {
            TRMSGI(TRCI_XAPI,
                   "XAPICFG[%d] entry not found for drive=%02d:%02d:%02d:%02d\n",
                   i,
                   pLibdrvid->acs,
                   pLibdrvid->lsm,
                   pLibdrvid->panel,
                   pLibdrvid->driveNumber);

            return STATUS_DRIVE_NOT_IN_LIBRARY;   
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
                                      XNAME_lock_drive,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    xapi_userid(pXapicvt,
                (char*) pMessage_Header,
                useridString);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_user_label,
                                      useridString,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_lock_id,
                                      lockIdString,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_wait_flag,
                                      waitString,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_drive_list,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    for (i = 0;
        i < lockCount;
        i++)
    {
        memset(driveLocIdString, 0, sizeof(driveLocIdString));

        STRIP_TRAILING_BLANKS(pXapicfg[i]->driveLocId,
                              driveLocIdString,
                              sizeof(pXapicfg[i]->driveLocId));

        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_drive_location_id,
                                          driveLocIdString,
                                          0);
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
/** Function Name: extractLockResponse                               */
/** Description:   Extract the <lock_drive_request> response.        */
/**                                                                  */
/** Parse the response of the XAPI XML <lock_drive>                  */
/** request and update the appropriate fields of the                 */
/** ACSAPI LOCK DRIVE response.                                      */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <lock_drive_request> response consists of:          */
/**==================================================================*/
/** <libreply>                                                       */
/**   <lock_drive_request>                                           */
/**     <header>                                                     */
/**       header element children                                    */
/**     </header>                                                    */
/**     <status>SSS...SSS</status>                                   */
/**     <lock_id>nnnnn</lock_id>                                     */
/**     <cdk_drive_status>                                           */
/**       <status>SSS...SSS</status>                                 */
/**       <drive_location_id>aa:ll:pp:dd:zz</drive_location_id>      */
/**     </cdk_drive_status>                                          */
/**     ...repeated <cdk_drive_status> entries                       */
/**     <exceptions>                                                 */
/**       <reason>ccc...ccc</reason>                                 */
/**       ...repeated <reason> entries                               */
/**     </exceptions>                                                */
/**     <uui_return_code>nnnn</uui_return_code>                      */
/**     <uui_reason_code>nnnn</uui_reason_code>                      */
/**   </lock_drive_request>                                          */
/** </libreply>                                                      */
/**                                                                  */
/** NOTE: Possible <lock_drive_request><status> values are:          */
/**       "success", "parser error", "process failure",              */
/**       "lockid not found", "lockid mismatch",                     */
/**       "database error", "no subfile", and                        */
/**       "lock failed".                                             */
/**                                                                  */
/** NOTE: Possible <cdk_drive_status><status> values are:            */
/**       "success", "cancelled", "deadlock", "locked",              */
/**       "lockid not found", "lockid mismatch",                     */
/**       "not in library", and "unknown".                           */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI LOCK DRIVE response consists of:                      */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   LOCK_RESPONSE data consisting of:                           */
/**      i.   type                                                   */
/**      ii.  count                                                  */
/**      iii. LO_DRV_STATUS(count) data entries consisting of:       */
/**           a.   DRIVE_ID consisting of:                           */
/**                1.   drive_id.panel_id.lsm_id.acs                 */
/**                2.   drive_id.panel_id.lsm_id.lsm                 */
/**                3.   drive_id.panel_id.panel                      */
/**                4.   drive_id.drive                               */
/**           b.   RESPONSE_STATUS consisting of:                    */
/**                1.   status                                       */
/**                2.   type                                         */
/**                3.   identifier                                   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractLockResponse"

static int extractLockResponse(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char            *pLockResponse,
                               struct XMLPARSE *pXmlparse)
{
    int                 lockRC;
    int                 headerRC;
    int                 i;
    int                 j;
    int                 lockResponseCount;
    int                 lockRemainingCount;
    int                 lockPacketCount;
    int                 lockResponseSize;
    char                lockStatus[16];
    char                lockIdString[6];
    char                driveLocId[16]; 
    char                cdkStatusStatus[16];

    struct XMLELEM     *pParentXmlelem;
    struct XMLELEM     *pLockDriveXmlelem;
    struct XMLELEM     *pStatusXmlelem;
    struct XMLELEM     *pLockidXmlelem;
    struct XMLELEM     *pFirstCdkStatusXmlelem;
    struct XMLELEM     *pNextCdkStatusXmlelem;
    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;
    struct RAWCOMMON   *pRawcommon          = &(pXapicommon->rawcommon);
    struct XAPICFG     *pXapicfg;

    LOCK_RESPONSE      *pLock_Response      = (LOCK_RESPONSE*) pLockResponse;
    LO_DRV_STATUS      *pLo_Drv_Status      = &(pLock_Response->identifier_status.drive_status[0]);
    MESSAGE_HEADER     *pMessage_Header     = &(pLock_Response->request_header.message_header);

    int                 lockId              = (int) pMessage_Header->lock_id;
    int                 newLockId;

    struct CDKSTATUS    cdkstatus[]         =
    {
        "SUCCESS         ",  STATUS_SUCCESS,
        "CANCELLED       ",  STATUS_CANCELLED,
        "DATABASE ERROR  ",  STATUS_DATABASE_ERROR,
        "DEADLOCK        ",  STATUS_DEADLOCK,
        "LOCKED          ",  STATUS_VOLUME_IN_USE,
        "UNLOCK FAILED   ",  STATUS_LOCK_FAILED,
        "LOCKID MISMATCH ",  STATUS_INVALID_LOCKID,
        "LOCKID NOT FOUND",  STATUS_LOCKID_NOT_FOUND,
        "NO SUBFILE      ",  STATUS_DATABASE_ERROR,
        "NOT IN LIBRARY  ",  STATUS_VOLUME_NOT_IN_LIBRARY,
        "PARSER ERROR    ",  STATUS_PROCESS_FAILURE,
        "PROCESS FAILURE ",  STATUS_PROCESS_FAILURE,
        "UNKNOWN         ",  STATUS_PROCESS_FAILURE,
    };

    int                 cdkstatusCount      = sizeof(cdkstatus) / 
                                              sizeof(struct CDKSTATUS);

    struct XMLSTRUCT    cdkStatusXmlstruct[] =
    {
        XNAME_cdk_drive_status,             XNAME_status,
        sizeof(cdkStatusStatus),            cdkStatusStatus,
        BLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cdk_drive_status,             XNAME_drive_location_id,
        sizeof(driveLocId),                 driveLocId,
        BLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 cdkStatusElementCount = sizeof(cdkStatusXmlstruct) / 
                                                sizeof(struct XMLSTRUCT);

    headerRC = xapi_parse_header_trailer(pXapicvt,
                                         pXapireqe,
                                         pXmlparse,
                                         pXapicommon);

    TRMSGI(TRCI_XAPI,
           "headerRC=%d from xapi_parse_header_trailer; "
           "uuiRC=%d, uuiReason=%d\n",
           headerRC,
           pXapicommon->uuiRC,
           pXapicommon->uuiReason);

    /*****************************************************************/
    /* Get the <lock_drive_request><status> element.                 */
    /*****************************************************************/
    lockRC = STATUS_NI_FAILURE;

    pLockDriveXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                pXmlparse->pHocXmlelem,
                                                XNAME_lock_drive_request);

    /*****************************************************************/
    /* If we find the <lock_drive_request> element,                  */
    /* then extract the high level <status> and <lock_id>            */
    /*****************************************************************/
    if (pLockDriveXmlelem != NULL)
    {
        pStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                 pLockDriveXmlelem,
                                                 XNAME_status);

        if (pStatusXmlelem != NULL)
        {
            memset(lockStatus, ' ', sizeof(lockStatus));

            memcpy(lockStatus,
                   pStatusXmlelem->pContent,
                   pStatusXmlelem->contentLen);

            for (i = 0;
                i < sizeof(lockStatus);
                i++)
            {
                lockStatus[i] = toupper(lockStatus[i]);
            }

            for (i = 0;
                i < cdkstatusCount;              
                i++)
            {
                if (memcmp(cdkstatus[i].statusString, 
                           lockStatus,
                           sizeof(lockStatus)) == 0)
                {
                    lockRC = cdkstatus[i].statusCode;

                    break;
                }
            }

            TRMSGI(TRCI_XAPI,
                   "[i]=%d, lockRC=%d, lockStatus=%.16s\n",
                   i,
                   lockRC,
                   lockStatus);
        }

        pLockidXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                 pLockDriveXmlelem,
                                                 XNAME_lock_id);

        if (pLockidXmlelem != NULL)
        {
            memset(lockIdString, '0', sizeof(lockIdString));
            lockIdString[(sizeof(lockIdString) - 1)] = 0;

            memcpy(&(lockIdString[((sizeof(lockIdString) - 1) - (pLockidXmlelem->contentLen))]),
                   pLockidXmlelem->pContent,
                   pLockidXmlelem->contentLen);

            newLockId = atoi(lockIdString);

            TRMSGI(TRCI_XAPI,
                   "lockId=%d, lockIdString=%s, newLockId=%d\n",
                   lockId,
                   lockIdString,
                   newLockId);
        }
        else
        {
            newLockId = lockId;
        }
    }

    /*****************************************************************/
    /* Count the number of <cdk_drive_status> entries.               */
    /*****************************************************************/
    pFirstCdkStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                     pXmlparse->pHocXmlelem,
                                                     XNAME_cdk_drive_status);

    lockResponseCount = 0;

    if (pFirstCdkStatusXmlelem != NULL)
    {
        pParentXmlelem = pFirstCdkStatusXmlelem->pParentXmlelem;

        TRMSGI(TRCI_XAPI,
               "pFirstCdkStatusXmlelem=%08X; pParentXmlelem=%08X\n",
               pFirstCdkStatusXmlelem,
               pParentXmlelem);

        pNextCdkStatusXmlelem = pFirstCdkStatusXmlelem;

        while (pNextCdkStatusXmlelem != NULL)
        {
            lockResponseCount++;

            pNextCdkStatusXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                                 pParentXmlelem,
                                                                 pNextCdkStatusXmlelem,
                                                                 XNAME_cdk_drive_status);

#ifdef DEBUG

            if (pNextCdkStatusXmlelem != NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextCdkStatusXmlelem=%08X\n",
                       pNextCdkStatusXmlelem);
            }

#endif

        }
    }

    TRMSGI(TRCI_XAPI,
           "lockResponseCount=%d, MAX_ID=%d\n",
           lockResponseCount,
           MAX_ID);

    /*****************************************************************/
    /* If no <cdk_drive_status> elements found, then return an error.*/
    /*****************************************************************/
    if (lockResponseCount == 0)
    {
        if (headerRC > 0)
        {
            return headerRC;
        }
        else if (lockRC != STATUS_SUCCESS)
        {
            return lockRC;
        }
        else
        {
            return STATUS_PROCESS_FAILURE;
        }
    }

    /*****************************************************************/
    /* If <cdk_drive_status> elements found, then extract the data.  */
    /*****************************************************************/
    pNextCdkStatusXmlelem = pFirstCdkStatusXmlelem;
    lockRemainingCount = lockResponseCount;

    while (1)
    {
        if (lockRemainingCount > MAX_ID)
        {
            lockPacketCount = MAX_ID;
        }
        else
        {
            lockPacketCount = lockRemainingCount;
        }

        lockResponseSize = (char*) pLo_Drv_Status -
                           (char*) pLock_Response +
                           ((sizeof(LO_DRV_STATUS)) * lockPacketCount);

        xapi_lock_init_resp(pXapireqe,
                            (char*) pLockResponse,
                            lockResponseSize);

        pLock_Response = (LOCK_RESPONSE*) pLockResponse;
        pLo_Drv_Status = &(pLock_Response->identifier_status.drive_status[0]);

        if ((lockId == 0) &&
            (newLockId != lockId))
        {
            pMessage_Header->lock_id = (LOCKID) newLockId;

            TRMSGI(TRCI_XAPI,
                   "Updated lock_id=%d at %08X\n",
                   pMessage_Header->lock_id,
                   &(pMessage_Header->lock_id));
        }

        TRMSGI(TRCI_XAPI,
               "At top of while; lockResponse=%d, "
               "lockRemaining=%d, lockPacket=%d, MAX_ID=%d\n",
               lockResponseCount,
               lockRemainingCount,
               lockPacketCount,
               MAX_ID);

        pLock_Response->count = lockPacketCount;
        pLock_Response->message_status.status = (STATUS) lockRC;

        for (i = 0;
            i < lockPacketCount;
            i++, pLo_Drv_Status++)
        {
            if (pNextCdkStatusXmlelem == NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextCdkStatusXmlelem=NULL at lockCount=%i\n",
                       (i+1));

                break;
            }

            memset(driveLocId, ' ', sizeof(driveLocId));
            memset(cdkStatusStatus, ' ', sizeof(cdkStatusStatus));
            pLo_Drv_Status->status.status = STATUS_SUCCESS;

            FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                              pNextCdkStatusXmlelem,
                                              &cdkStatusXmlstruct[0],
                                              cdkStatusElementCount);

            if (driveLocId[0] > ' ')
            {
                pXapicfg = xapi_config_search_drivelocid(pXapicvt,
                                                         pXapireqe,
                                                         driveLocId);

                if (pXapicfg != NULL)
                {
                    pLo_Drv_Status->drive_id.panel_id.lsm_id.acs = pXapicfg->libdrvid.acs;
                    pLo_Drv_Status->drive_id.panel_id.lsm_id.lsm = pXapicfg->libdrvid.lsm;
                    pLo_Drv_Status->drive_id.panel_id.panel = pXapicfg->libdrvid.panel;
                    pLo_Drv_Status->drive_id.drive = pXapicfg->libdrvid.driveNumber;
                }
                else
                {
                    pLo_Drv_Status->status.status = STATUS_DRIVE_NOT_IN_LIBRARY;
                }
            }
            else
            {
                pLo_Drv_Status->status.status = STATUS_DRIVE_NOT_IN_LIBRARY;
            }

            if ((pLo_Drv_Status->status.status == STATUS_SUCCESS) &&
                (cdkStatusStatus[0] > ' '))
            {
                pLo_Drv_Status->status.status = STATUS_PROCESS_FAILURE;

                for (j = 0;
                    j < sizeof(cdkStatusStatus);
                    j++)
                {
                    cdkStatusStatus[i] = toupper(cdkStatusStatus[i]);
                }

                for (j = 0;
                    j < cdkstatusCount;              
                    j++)
                {
                    if (memcmp(cdkstatus[j].statusString, 
                               cdkStatusStatus,
                               sizeof(cdkStatusStatus)) == 0)
                    {
                        pLo_Drv_Status->status.status = (STATUS) cdkstatus[j].statusCode;

                        break;
                    }
                }

                TRMSGI(TRCI_XAPI,
                       "[j]=%d, status=%d, cdkStatusStatus=%.16s\n",
                       j,
                       (int) pLo_Drv_Status->status.status,
                       cdkStatusStatus);
            }

            pNextCdkStatusXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                                 pParentXmlelem,
                                                                 pNextCdkStatusXmlelem,
                                                                 XNAME_cdk_drive_status);
        } /* for(i) */

        lockRemainingCount = lockRemainingCount - lockPacketCount;

        if (lockRemainingCount <= 0)
        {
            xapi_fin_response(pXapireqe,
                              pLockResponse,     
                              lockResponseSize); 

            break; 
        }
        else
        {
            xapi_int_response(pXapireqe,
                              pLockResponse,     
                              lockResponseSize);  
        }
    }

    return STATUS_SUCCESS;
}



