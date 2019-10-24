/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */ 
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_lock_vol.c                                  */
/** Description:    XAPI LOCK VOLUME processor.                      */
/**                                                                  */
/**                 Lock the specified volume(s) using the           */
/**                 specified LOCKID.                                */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     09/15/11                          */
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
/** Function Name: xapi_lock_vol                                     */
/** Description:   XAPI LOCK VOLUME processor.                       */
/**                                                                  */
/** Lock the specified volume(s) using the specified (i.e.           */
/** already existing) LOCKID, or if the input LOCKID is specified    */
/** as 0 (or NO_LOCK_ID), then lock the specified volume(s)          */
/** under a non-existent (i.e. unsed) LOCKID and return              */
/** the newly created LOCKID.  A LOCKID is simply a number           */
/** between 1 and 32767.                                             */
/**                                                                  */
/** VOLUME LOCK(s) are managed on the server.  The ACSAPI format     */
/** LOCK VOLUME request is translated into an XAPI XML format        */
/** <lock_volume> request; the XAPI XML request is then              */
/** transmitted to the server via TCP/IP;  The received XAPI XML     */
/** response is then translated into the ACSAPI LOCK VOLUME          */
/** response.                                                        */
/**                                                                  */
/** The LOCK VOLUME command is NOT allowed to proceed when           */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_lock_vol"

extern int xapi_lock_vol(struct XAPICVT  *pXapicvt,
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
           "Entered; LOCK VOLUME request=%08X, size=%d, count=%d, "
           "MAX_ID=%d, LOCK VOLUME response=%08X, size=%d\n",
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
    /* Now generate the LOCK VOLUME ACSAPI response.                 */
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
/** Description:   Build an XAPI <lock_volume> request.              */
/**                                                                  */
/** Convert the ACSAPI format LOCK VOLUME request into an            */
/** XAPI XML format <lock_volume> request.                           */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI LOCK VOLUME request consists of:                      */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   LOCK VOLUME data consisting of:                             */
/**      i.   type                                                   */
/**      ii.  count                                                  */
/**      iii. VOLID[count] data entries consisting of:               */
/**           a.   external_label (6 character volser)               */
/**                                                                  */
/** NOTE: The input LOCKID is in the MESSAGE_HEADER.lock_id field.   */
/**                                                                  */
/** NOTE: The MESSAGE_HEADER.extended_options.WAIT bit determines    */
/**       the LOCK wait option.                                      */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <lock_volume> request consists of:                  */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <lock_volume>                                                */
/**       <user_label>CCC...CCC</user_label>                         */
/**       <lock_id>0|nnnnn</lock_id>                                 */
/**       <wait_flag>"Yes"|"No"</wait_flag>                          */
/**       <volume_list>                                              */
/**         <volser>vvvvvv</volser>                                  */
/**         ...repeated <volser> entries                             */
/**       </volume_list>                                             */
/**     </lock_volume>                                               */
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
    VOLID              *pVolid              = &(pLock_Request->identifier.vol_id[0]);

    int                 lockCount           = pLock_Request->count;
    int                 lockId              = (int) pMessage_Header->lock_id;
    struct RAWVOLUME    rawvolume;
    struct RAWVOLUME   *pRawvolume          = &rawvolume;

    char                useridString[EXTERNAL_USERID_SIZE + 1];
    char                lockIdString[6];
    char                waitString[4];
    char                volserString[7];

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
           "Entered; lockCount=%d, lockId=%d, waitString=%s\n",
           lockCount,
           lockId,
           waitString);

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
        sprintf(lockIdString, "%d", lockId);
    }

    /*****************************************************************/
    /* If any of the specified VOLSER(s) is not found, then          */
    /* the entire request will fail.                                 */
    /*****************************************************************/
    for (i = 0, pVolid = &(pLock_Request->identifier.vol_id[0]);
        i < lockCount;
        i++, pVolid++)
    {
        memset((char*) pRawvolume, 0 , sizeof(struct RAWVOLUME));

        lastRC = xapi_qvol_one(pXapicvt,
                               pXapireqe,
                               pRawvolume,
                               NULL,
                               pVolid->external_label);

        if (lastRC != STATUS_SUCCESS)
        {
            TRMSGI(TRCI_XAPI,
                   "VOLID[%d] volser=%.6s entry not found\n",
                   i,
                   pVolid->external_label);

            return STATUS_VOLUME_NOT_IN_LIBRARY;   
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
                                      XNAME_lock_volume,
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
                                      XNAME_volume_list,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    for (i = 0, pVolid = &(pLock_Request->identifier.vol_id[0]);
        i < lockCount;
        i++, pVolid++)
    {
        memset((char*) volserString, 0, sizeof(volserString));

        memcpy(volserString,
               pVolid->external_label,
               XAPI_VOLSER_SIZE);

        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_volser,
                                          volserString,
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
/** Description:   Extract the <lock_volume_request> response.       */
/**                                                                  */
/** Parse the response of the XAPI XML <lock_volume>                 */
/** request and update the appropriate fields of the                 */
/** ACSAPI LOCK VOLUME response.                                     */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <lock_volume_request> response consists of:         */
/**==================================================================*/
/** <libreply>                                                       */
/**   <lock_volume_request>                                          */
/**     <header>                                                     */
/**       header element children                                    */
/**     </header>                                                    */
/**     <status>SSS...SSS</status>                                   */
/**     <lock_id>nnnnn</lock_id>                                     */
/**     <cdk_volume_status>                                          */
/**       <status>SSS...SSS</status>                                 */
/**       <volser>vvvvvv</volser>                                    */
/**     </cdk_volume_status>                                         */
/**     ...repeated <cdk_volume_status> entries                      */
/**     <exceptions>                                                 */
/**       <reason>ccc...ccc</reason>                                 */
/**       ...repeated <reason> entries                               */
/**     </exceptions>                                                */
/**     <uui_return_code>nnnn</uui_return_code>                      */
/**     <uui_reason_code>nnnn</uui_reason_code>                      */
/**   </lock_volume_request>                                         */
/** </libreply>                                                      */
/**                                                                  */
/** NOTE: Possible <lock_volume_request><status> values are:         */
/**       "success", "parser error", "process failure",              */
/**       "lockid not found", "lockid mismatch",                     */
/**       "database error", "no subfile", and                        */
/**       "lock failed".                                             */
/**                                                                  */
/** NOTE: Possible <cdk_volume_status><status> values are:           */
/**       "success", "cancelled", "deadlock", "locked",              */
/**       "lockid not found", "lockid mismatch",                     */
/**       "not in library", and "unknown".                           */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI LOCK VOLUME response consists of:                     */
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
/**      iii. LO_VOL_STATUS(count) data entries consisting of:       */
/**           a.   VOLID consisting of:                              */
/**                1.  external_label (6 character volser)           */
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
    char                volser[6]; 
    char                cdkStatusStatus[16];

    struct XMLELEM     *pParentXmlelem;
    struct XMLELEM     *pLockVolumeXmlelem;
    struct XMLELEM     *pStatusXmlelem;
    struct XMLELEM     *pLockidXmlelem;
    struct XMLELEM     *pFirstCdkStatusXmlelem;
    struct XMLELEM     *pNextCdkStatusXmlelem;
    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;
    struct RAWCOMMON   *pRawcommon          = &(pXapicommon->rawcommon);

    LOCK_RESPONSE      *pLock_Response      = (LOCK_RESPONSE*) pLockResponse;
    LO_VOL_STATUS      *pLo_Vol_Status      = &(pLock_Response->identifier_status.volume_status[0]);
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
        "LOCK FAILED     ",  STATUS_LOCK_FAILED,
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
        XNAME_cdk_volume_status,            XNAME_status,
        sizeof(cdkStatusStatus),            cdkStatusStatus,
        BLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cdk_volume_status,            XNAME_volser,
        sizeof(volser),                     volser,
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
    /* Get the <lock_volume_request><status> element.                */
    /*****************************************************************/
    lockRC = STATUS_NI_FAILURE;

    pLockVolumeXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                 pXmlparse->pHocXmlelem,
                                                 XNAME_lock_volume_request);

    /*****************************************************************/
    /* If we find the <lock_volume_request> element,                 */
    /* then extract the high level <status> and <lock_id>            */
    /*****************************************************************/
    if (pLockVolumeXmlelem != NULL)
    {
        pStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                 pLockVolumeXmlelem,
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
                                                 pLockVolumeXmlelem,
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
    /* Count the number of <cdk_volume_status> entries.               */
    /*****************************************************************/
    pFirstCdkStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                     pXmlparse->pHocXmlelem,
                                                     XNAME_cdk_volume_status);

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
                                                                 XNAME_cdk_volume_status);

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
    /* If no <cdk_volume_status> elements found, then return an      */
    /* error.                                                        */
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
    /* If <cdk_volume_status> elements found, then extract the data. */
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

        lockResponseSize = (char*) pLo_Vol_Status -
                           (char*) pLock_Response +
                           ((sizeof(LO_VOL_STATUS)) * lockPacketCount);

        xapi_lock_init_resp(pXapireqe,
                            (char*) pLockResponse,
                            lockResponseSize);

        pLock_Response = (LOCK_RESPONSE*) pLockResponse;
        pLo_Vol_Status = &(pLock_Response->identifier_status.volume_status[0]);

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
            i++, pLo_Vol_Status++)
        {
            if (pNextCdkStatusXmlelem == NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextCdkStatusXmlelem=NULL at lockCount=%i\n",
                       (i+1));

                break;
            }

            memset(volser, ' ', sizeof(volser));
            memset(cdkStatusStatus, ' ', sizeof(cdkStatusStatus));
            pLo_Vol_Status->status.status = STATUS_SUCCESS;

            FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                              pNextCdkStatusXmlelem,
                                              &cdkStatusXmlstruct[0],
                                              cdkStatusElementCount);

            TRMSGI(TRCI_XAPI,
                   "[i]=%d, volser=%.6s, status=%.16s\n",
                   i,
                   volser,
                   cdkStatusStatus);

            if (volser[0] > ' ')
            {
                memcpy(pLo_Vol_Status->vol_id.external_label,
                       volser,
                       XAPI_VOLSER_SIZE);

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
                        pLo_Vol_Status->status.status = (STATUS) cdkstatus[j].statusCode;

                        break;
                    }
                }

                TRMSGI(TRCI_XAPI,
                       "[j]=%d, status=%d, cdkStatusStatus=%.16s\n",
                       j,
                       (int) pLo_Vol_Status->status.status,
                       cdkStatusStatus);
            }
            else
            {
                pLo_Vol_Status->status.status = STATUS_VOLUME_NOT_IN_LIBRARY;
            }

            pNextCdkStatusXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                                 pParentXmlelem,
                                                                 pNextCdkStatusXmlelem,
                                                                 XNAME_cdk_volume_status);
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



