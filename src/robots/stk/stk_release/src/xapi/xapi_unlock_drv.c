/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */ 
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_unlock_drv.c                                */
/** Description:    XAPI UNLOCK DRIVE processor.                     */
/**                                                                  */
/**                 Unlock the specified DRIVE(s) from the           */
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
#define LOCKERR_RESPONSE_SIZE (offsetof(UNLOCK_RESPONSE, identifier_status)) 
#define UNLOCK_SEND_TIMEOUT        5   /* TIMEOUT values in seconds  */       
#define UNLOCK_RECV_TIMEOUT_1ST    120
#define UNLOCK_RECV_TIMEOUT_NON1ST 900


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int convertUnlockRequest(struct XAPICVT   *pXapicvt,
                                struct XAPIREQE  *pXapireqe,
                                char            **ptrXapiBuffer,
                                int              *pXapiBufferSize);

static int extractUnlockResponse(struct XAPICVT  *pXapicvt,
                                 struct XAPIREQE *pXapireqe,
                                 char            *pUnlockResponse,
                                 struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_unlock_drv                                   */
/** Description:   XAPI UNLOCK DRIVE processor.                      */
/**                                                                  */
/** Unlock the specified DRIVE(s) from the specified (i.e.           */
/** already existing) LOCKID.  A LOCKID is simply a number           */
/** between 1 and 32767.                                             */
/**                                                                  */
/** DRIVE LOCK(s) are managed on the server.  The ACSAPI format      */
/** UNLOCK DRIVE request is translated into an XAPI XML format       */
/** <unlock_drive> request; the XAPI XML request is then             */
/** transmitted to the server via TCP/IP;  The received XAPI XML     */
/** response is then translated into one or more ACSAPI              */
/** UNLOCK DRIVE responses.                                          */
/**                                                                  */
/** The UNLOCK DRIVE command is NOT allowed to proceed when          */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_unlock_drv"

extern int xapi_unlock_drv(struct XAPICVT  *pXapicvt,
                           struct XAPIREQE *pXapireqe)
{
    int                 unlockRC            = STATUS_SUCCESS;
    int                 lastRC;
    int                 i;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    UNLOCK_REQUEST     *pUnlock_Request     = (UNLOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    int                 unlockCount         = pUnlock_Request->count;

    UNLOCK_RESPONSE     wkUnlock_Response;
    UNLOCK_RESPONSE    *pUnlock_Response    = &wkUnlock_Response;

    xapi_lock_init_resp(pXapireqe,
                        (char*) pUnlock_Response,
                        sizeof(UNLOCK_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; UNLOCK DRIVE request=%08X, size=%d, count=%d, "
           "MAX_ID=%d, UNLOCK DRIVE response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           unlockCount,
           MAX_ID,
           pUnlock_Response,
           LOCKERR_RESPONSE_SIZE);

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pUnlock_Response,
                            LOCKERR_RESPONSE_SIZE);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    lastRC = convertUnlockRequest(pXapicvt,
                                  pXapireqe,
                                  &pXapiBuffer,
                                  &xapiBufferSize);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from convertUnlockRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pUnlock_Response,
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
                      UNLOCK_SEND_TIMEOUT,       
                      UNLOCK_RECV_TIMEOUT_1ST,   
                      UNLOCK_RECV_TIMEOUT_NON1ST,
                      (void**) &pXmlparse);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from xapi_tcp(pBuffer=%08X, size=%d, pXmlparse=%08X)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize,
           pXmlparse);

    if (lastRC != STATUS_SUCCESS)
    {
        unlockRC = lastRC;
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
    /* Now generate the UNLOCK DRIVE ACSAPI response.                */
    /*****************************************************************/
    if (unlockRC == STATUS_SUCCESS)
    {
        lastRC = extractUnlockResponse(pXapicvt,
                                       pXapireqe,
                                       (char*) pUnlock_Response,
                                       pXmlparse);

        TRMSGI(TRCI_XAPI,"lastRC=%d from extractUnlockResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            unlockRC = lastRC;
        }
    }

    if (unlockRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pUnlock_Response,
                          LOCKERR_RESPONSE_SIZE,
                          unlockRC);
    }

    /*****************************************************************/
    /* When done, free the XMLPARSE and associated structures.       */
    /*****************************************************************/
    if (pXmlparse != NULL)
    {
        FN_FREE_HIERARCHY_STORAGE(pXmlparse);
    }

    return unlockRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: convertUnlockRequest                              */
/** Description:   Build an XAPI <unlock_drive> request.             */
/**                                                                  */
/** Convert the ACSAPI format UNLOCK DRIVE request into an           */
/** XAPI XML format <unlock_drive> request.                          */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI UNLOCK DRIVE request consists of:                     */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   UNLOCK DRIVE data consisting of:                            */
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
/**==================================================================*/
/** The XAPI XML <unlock_drive> request consists of:                 */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <unlock_drive>                                               */
/**       <lock_id>nnnnn</lock_id>                                   */
/**       <drive_list>                                               */
/**         <drive_location_id>aa:ll:pp:dd:zz</drive_location_id>    */
/**         ...repeated <drive_location_id> entries                  */
/**       </drive_list>                                              */
/**     </unlock_drive>                                              */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** NOTES:                                                           */
/** (1) The XAPI <lock_id> is required (unlike the XAPI LOCK         */
/**     command).                                                    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "convertUnlockRequest"

static int convertUnlockRequest(struct XAPICVT  *pXapicvt,
                                struct XAPIREQE *pXapireqe,
                                char           **ptrXapiBuffer,
                                int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 i;
    int                 xapiRequestSize     = 0;
    UNLOCK_REQUEST     *pUnlock_Request     = (UNLOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    MESSAGE_HEADER     *pMessage_Header     = &(pUnlock_Request->request_header.message_header);
    DRIVEID            *pDriveid            = &(pUnlock_Request->identifier.drive_id[0]);

    int                 unlockCount         = pUnlock_Request->count;
    int                 lockId              = (int) pMessage_Header->lock_id;
    struct LIBDRVID     libdrvid;
    struct LIBDRVID    *pLibdrvid           = &libdrvid;
    struct XAPICFG     *pXapicfg[MAX_ID];

    char                lockIdString[6];
    char                driveLocIdString[24];

    char               *pXapiRequest        = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    TRMSGI(TRCI_XAPI,
           "Entered; unlockCount=%d, lockId=%d, pDriveid[0]=%08X\n",
           unlockCount,
           lockId,
           pDriveid);

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

    /*****************************************************************/
    /* Allow unlock to specify a count of 0.                         */
    /*****************************************************************/
    if (unlockCount < 0)
    {
        return STATUS_COUNT_TOO_SMALL;
    }

    if (unlockCount > MAX_ID)
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
    if (unlockCount > 0)
    {
        for (i = 0, pDriveid = &(pUnlock_Request->identifier.drive_id[0]);
            i < unlockCount;
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
                                      XNAME_unlock_drive,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_lock_id,
                                      lockIdString,
                                      0);

    if (unlockCount > 0)
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_drive_list,
                                          NULL,
                                          0);

        pParentXmlelem = pXmlparse->pCurrXmlelem;

        for (i = 0;
            i < unlockCount;
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
/** Function Name: extractUnlockResponse                             */
/** Description:   Extract the <unlock_drive_request> response.      */
/**                                                                  */
/** Parse the response of the XAPI XML <unlock_drive> request and    */
/** update the appropriate fields of the ACSAPI UNLOCK DRIVE         */
/** response.                                                        */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <unlock_drive_request> response consists of:        */
/**==================================================================*/
/** <libreply>                                                       */
/**   <unlock_drive_request>                                         */
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
/**   </unlock_drive_request>                                        */
/** </libreply>                                                      */
/**                                                                  */
/** NOTE: Possible <unlock_drive_request><status> values are:        */
/**       "success", "parser error", "process failure",              */
/**       "lockid not found", "lockid mismatch",                     */
/**       "database error", "no subfile", and                        */
/**       "unlock failed".                                           */
/**                                                                  */
/** NOTE: Possible <cdk_drive_status><status> values are:            */
/**       "success", "cancelled", "deadlock", "locked",              */
/**       "lockid not found", "lockid mismatch",                     */
/**       "not in library", and "unknown".                           */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI UNLOCK DRIVE response consists of:                    */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   UNLOCK_RESPONSE data consisting of:                         */
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
#define SELF "extractUnlockResponse"

static int extractUnlockResponse(struct XAPICVT  *pXapicvt,
                                 struct XAPIREQE *pXapireqe,
                                 char            *pUnlockResponse,
                                 struct XMLPARSE *pXmlparse)
{
    int                 unlockRC;
    int                 headerRC;
    int                 i;
    int                 j;
    int                 unlockResponseCount;
    int                 unlockRemainingCount;
    int                 unlockPacketCount;
    int                 unlockResponseSize;
    char                unlockStatus[16];
    char                driveLocId[16]; 
    char                cdkStatusStatus[16];

    struct XMLELEM     *pParentXmlelem;
    struct XMLELEM     *pUnlockDriveXmlelem;
    struct XMLELEM     *pStatusXmlelem;
    struct XMLELEM     *pFirstCdkStatusXmlelem;
    struct XMLELEM     *pNextCdkStatusXmlelem;
    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;
    struct RAWCOMMON   *pRawcommon          = &(pXapicommon->rawcommon);
    struct XAPICFG     *pXapicfg;

    UNLOCK_RESPONSE    *pUnlock_Response    = (UNLOCK_RESPONSE*) pUnlockResponse;
    LO_DRV_STATUS      *pLo_Drv_Status      = &(pUnlock_Response->identifier_status.drive_status[0]);

    struct CDKSTATUS    cdkstatus[]         =
    {
        "SUCCESS         ",  STATUS_SUCCESS,
        "CANCELLED       ",  STATUS_CANCELLED,
        "DATABASE ERROR  ",  STATUS_DATABASE_ERROR,
        "DEADLOCK        ",  STATUS_DEADLOCK,
        "LOCKED          ",  STATUS_DRIVE_IN_USE,
        "UNLOCK FAILED   ",  STATUS_LOCK_FAILED,
        "LOCKID MISMATCH ",  STATUS_INVALID_LOCKID,
        "LOCKID NOT FOUND",  STATUS_LOCKID_NOT_FOUND,
        "NO SUBFILE      ",  STATUS_DATABASE_ERROR,
        "NOT IN LIBRARY  ",  STATUS_DRIVE_NOT_IN_LIBRARY,
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
    /* Get the <unlock_drive_request><status> element.               */
    /*****************************************************************/
    unlockRC = STATUS_NI_FAILURE;

    pUnlockDriveXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                  pXmlparse->pHocXmlelem,
                                                  XNAME_unlock_drive_request);

    if (pUnlockDriveXmlelem != NULL)
    {
        pStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                 pUnlockDriveXmlelem,
                                                 XNAME_status);

        if (pStatusXmlelem != NULL)
        {
            memset(unlockStatus, ' ', sizeof(unlockStatus));

            memcpy(unlockStatus,
                   pStatusXmlelem->pContent,
                   pStatusXmlelem->contentLen);

            for (i = 0;
                i < sizeof(unlockStatus);
                i++)
            {
                unlockStatus[i] = toupper(unlockStatus[i]);
            }

            for (i = 0;
                i < cdkstatusCount;              
                i++)
            {
                if (memcmp(cdkstatus[i].statusString, 
                           unlockStatus,
                           sizeof(unlockStatus)) == 0)
                {
                    unlockRC = cdkstatus[i].statusCode;

                    break;
                }
            }

            TRMSGI(TRCI_XAPI,
                   "[i]=%d, unlockRC=%d, unlockStatus=%.16s\n",
                   i,
                   unlockRC,
                   unlockStatus);
        }
    }

    /*****************************************************************/
    /* Count the number of <cdk_drive_status> entries.               */
    /*****************************************************************/
    pFirstCdkStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                     pXmlparse->pHocXmlelem,
                                                     XNAME_cdk_drive_status);

    unlockResponseCount = 0;

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
            unlockResponseCount++;

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
           "unlockResponseCount=%d, MAX_ID=%d\n",
           unlockResponseCount,
           MAX_ID);

    /*****************************************************************/
    /* If no <cdk_drive_status> elements found, then return an error.*/
    /*****************************************************************/
    if (unlockResponseCount == 0)
    {
        if (headerRC > 0)
        {
            return headerRC;
        }
        else if (unlockRC != STATUS_SUCCESS)
        {
            return unlockRC;
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
    unlockRemainingCount = unlockResponseCount;

    while (1)
    {
        if (unlockRemainingCount > MAX_ID)
        {
            unlockPacketCount = MAX_ID;
        }
        else
        {
            unlockPacketCount = unlockRemainingCount;
        }

        unlockResponseSize = (char*) pLo_Drv_Status -
                             (char*) pUnlock_Response +
                             ((sizeof(LO_DRV_STATUS)) * unlockPacketCount);

        xapi_lock_init_resp(pXapireqe,
                            (char*) pUnlockResponse,
                            unlockResponseSize);

        pUnlock_Response = (UNLOCK_RESPONSE*) pUnlockResponse;
        pLo_Drv_Status = &(pUnlock_Response->identifier_status.drive_status[0]);

        TRMSGI(TRCI_XAPI,
               "At top of while; unlockResponse=%d, "
               "unlockRemaining=%d, unlockPacket=%d, MAX_ID=%d\n",
               unlockResponseCount,
               unlockRemainingCount,
               unlockPacketCount,
               MAX_ID);

        pUnlock_Response->count = unlockPacketCount;
        pUnlock_Response->message_status.status = (STATUS) unlockRC;

        for (i = 0;
            i < unlockPacketCount;
            i++, pLo_Drv_Status++)
        {
            if (pNextCdkStatusXmlelem == NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextCdkStatusXmlelem=NULL at unlockCount=%i\n",
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

        unlockRemainingCount = unlockRemainingCount - unlockPacketCount;

        if (unlockRemainingCount <= 0)
        {
            xapi_fin_response(pXapireqe,
                              pUnlockResponse,     
                              unlockResponseSize); 

            break; 
        }
        else
        {
            xapi_int_response(pXapireqe,
                              pUnlockResponse,     
                              unlockResponseSize);  
        }
    }

    return STATUS_SUCCESS;
}











