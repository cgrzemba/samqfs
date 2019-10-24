/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */ 
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qlock_drv.c                                 */
/** Description:    XAPI QUERY LOCK DRIVE processor.                 */
/**                                                                  */
/**                 Return lock information for the specified        */
/**                 drive(s).                                        */
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
#define QUERYERR_RESPONSE_SIZE (offsetof(QUERY_LOCK_RESPONSE, identifier_status)) 
#define QUERY_SEND_TIMEOUT         5   /* TIMEOUT values in seconds  */       
#define QUERY_RECV_TIMEOUT_1ST     120
#define QUERY_RECV_TIMEOUT_NON1ST  900


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int convertQueryRequest(struct XAPICVT   *pXapicvt,
                               struct XAPIREQE  *pXapireqe,
                               char            **ptrXapiBuffer,
                               int              *pXapiBufferSize);

static int extractQueryResponse(struct XAPICVT  *pXapicvt,
                                struct XAPIREQE *pXapireqe,
                                char            *pQueryResponse,
                                struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qlock_drv                                    */
/** Description:   XAPI QUERY LOCK DRIVE processor.                  */
/**                                                                  */
/** Return lock information for the specified drive(s) using         */
/** the specified (i.e. already existing) LOCKID, or if the input    */
/** LOCKID is specified as 0 (or NO_LOCK_ID), then return any        */
/** lock information for the specified drive(s).                     */
/**                                                                  */
/** DRIVE LOCK(s) are managed on the server.  The ACSAPI format      */
/** QUERY LOCK DRIVE request is translated into an XAPI XML format   */
/** <qry_drive_lock> request; the XAPI XML request is then           */
/** transmitted to the server via TCP/IP;  The received XAPI XML     */
/** response is then translated into one or more ACSAPI              */
/** QUERY LOCK DRIVE responses.                                      */
/**                                                                  */
/** There will only be one individual XAPI <qry_drive_lock> request  */
/** issued for a single ACSAPI QUERY LOCK DRIVE request regardless   */
/** of the requested drive count (as the XAPI <qry_drive_lock>       */
/** request allows a <drive_list> to be specified).                  */
/**                                                                  */
/** The QUERY LOCK DRIVE command is allowed to proceed even when     */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qlock_drv"

extern int xapi_qlock_drv(struct XAPICVT  *pXapicvt,
                          struct XAPIREQE *pXapireqe)
{
    int                 queryRC              = STATUS_SUCCESS;
    int                 lastRC;
    int                 i;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    QUERY_LOCK_REQUEST *pQuery_Lock_Request = (QUERY_LOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    int                 driveCount          = pQuery_Lock_Request->count;

    QUERY_LOCK_RESPONSE wkQuery_Lock_Response;
    QUERY_LOCK_RESPONSE *pQuery_Lock_Response = &wkQuery_Lock_Response;

    xapi_qlock_init_resp(pXapireqe,
                         (char*) pQuery_Lock_Response,
                         sizeof(QUERY_LOCK_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY LOCK DRIVE request=%08X, size=%d, count=%d, "
           "MAX_ID=%d, QUERY LOCK DRIVE response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           driveCount,
           MAX_ID,
           pQuery_Lock_Response,
           QUERYERR_RESPONSE_SIZE);

    lastRC = convertQueryRequest(pXapicvt,
                                 pXapireqe,
                                 &pXapiBuffer,
                                 &xapiBufferSize);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from convertQueryRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Lock_Response,
                          QUERYERR_RESPONSE_SIZE,
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
    /* Now generate the QUERY LOCK DRIVE ACSAPI response.            */
    /*****************************************************************/
    if (queryRC == STATUS_SUCCESS)
    {
        lastRC = extractQueryResponse(pXapicvt,
                                      pXapireqe,
                                      (char*) pQuery_Lock_Response,
                                      pXmlparse);

        TRMSGI(TRCI_XAPI,"lastRC=%d from extractQueryResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            queryRC = lastRC;
        }
    }

    if (queryRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Lock_Response,
                          QUERYERR_RESPONSE_SIZE,
                          queryRC);
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
/** Function Name: convertQueryRequest                               */
/** Description:   Build an XAPI <qry_drive_lock> request.           */
/**                                                                  */
/** Convert the ACSAPI format QUERY LOCK DRIVE request into an       */
/** XAPI XML format <qry_drive_lock> request.                        */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY LOCK DRIVE request consists of:                 */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY LOCK DRIVE data consisting of:                        */
/**      i.   type                                                   */
/**      ii.  count                                                  */
/**      iii. DRIVEID[count] data entries consisting of:             */
/**           a.   drive_id.panel_id.lsm_id.acs                      */
/**           b.   drive_id.panel_id.lsm_id.lsm                      */
/**           c.   drive_id.panel_id.panel                           */
/**           d.   drive_id.drive                                    */
/**                                                                  */
/** NOTE: The input LOCKID is in the MESSAGE_HEADER.lock_id field.   */
/**       If specified as 0 (or NO_LOCK_ID), then ALL matching       */
/**       drives that are currently locked are returned.             */
/**                                                                  */
/** NOTE: Specifying a count of 0 will return ALL drives with a      */
/**       matching LOCKID (or all locked drives if NO_LOCK_ID is     */
/**       also specified).                                           */
/**                                                                  */
/** To summarize:                                                    */
/**       (1) If count is non-zero, and LOCKID is non-zero,          */
/**           then the response will include all matching drives     */
/**           currently locked with the specified LOCKID.            */
/**                                                                  */
/**       (2) If count is 0, but LOCKID is non-zero, then the        */
/**           response includes all drives locked with the           */
/**           specified LOCKID.                                      */
/**                                                                  */
/**       (3) If count is non-zero, but LOCKID is 0, then the        */
/**           response includes all matching drives locked with      */
/**           any LOCKID.                                            */
/**                                                                  */
/**       (4) If both count and LOCKID are 0, then the               */
/**           response includes all locked drives with any LOCKID.   */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <qry_drive_lock> request consists of:               */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <qry_drive_lock>                                             */
/**       <lock_id>nnnnn</lock_id>                                   */
/**       <drive_list>                                               */
/**         <drive_location_id>aa:ll:pp:dd:zz</drive_location_id>    */
/**         ...repeated <drive_location_id> entries                  */
/**       </drive_list>                                              */
/**     </qry_drive_lock>                                            */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "convertQueryRequest"

static int convertQueryRequest(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char           **ptrXapiBuffer,
                               int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 i;
    int                 xapiRequestSize     = 0;
    QUERY_LOCK_REQUEST *pQuery_Lock_Request = (QUERY_LOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    MESSAGE_HEADER     *pMessage_Header     = &(pQuery_Lock_Request->request_header.message_header);
    DRIVEID            *pDriveid            = &(pQuery_Lock_Request->identifier.drive_id[0]);

    int                 driveCount          = pQuery_Lock_Request->count;
    int                 lockId              = pMessage_Header->lock_id;
    struct LIBDRVID     libdrvid;
    struct LIBDRVID    *pLibdrvid           = &libdrvid;
    struct XAPICFG     *pXapicfg[MAX_ID];

    char                lockIdString[6];
    char                driveLocIdString[24];

    char               *pXapiRequest        = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    sprintf(lockIdString,
            "%d",
            lockId);

    TRMSGI(TRCI_XAPI,
           "Entered; lockId=%s, driveCount=%d\n",
           lockIdString,
           driveCount);

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

    if (driveCount < 0)
    {
        return STATUS_COUNT_TOO_SMALL;
    }

    if (driveCount > MAX_ID)
    {
        return STATUS_COUNT_TOO_LARGE;
    }

    /*****************************************************************/
    /* If any of the specified drives is not found, then             */
    /* we still process the request for all specified drives.        */
    /* We pass a "dummy" drive_location_id for the not found drives  */
    /* so that an error response will be generated in the ACSAPI     */
    /* response for the not found drives.                            */
    /*****************************************************************/
    if (driveCount > 0)
    {
        for (i = 0, pDriveid = &(pQuery_Lock_Request->identifier.drive_id[0]);
            i < driveCount;
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
                                      XNAME_qry_drive_lock,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_lock_id,
                                      lockIdString,
                                      0);

    if (driveCount > 0)
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_drive_list,
                                          NULL,
                                          0);

        pParentXmlelem = pXmlparse->pCurrXmlelem;

        for (i = 0, pDriveid = &(pQuery_Lock_Request->identifier.drive_id[0]);
            i < driveCount;
            i++, pDriveid++)
        {
            memset(driveLocIdString, 0, sizeof(driveLocIdString));

            /*********************************************************/
            /* If the input DRIVEID was not found, then generate     */
            /* a dummy <drive_location_id> of "R:AA:LL:PP:NN".       */
            /*********************************************************/
            if (pXapicfg[i] == NULL)
            {
                sprintf(driveLocIdString,
                        "R:%02d:%02d:%02d:%02d",
                        (unsigned char) pDriveid->panel_id.lsm_id.acs,
                        (unsigned char) pDriveid->panel_id.lsm_id.lsm,
                        (unsigned char) pDriveid->panel_id.panel,
                        (unsigned char) pDriveid->drive);
            }
            else
            {
                STRIP_TRAILING_BLANKS(pXapicfg[i]->driveLocId,
                                      driveLocIdString,
                                      sizeof(pXapicfg[i]->driveLocId));
            }

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
/** Function Name: extractQueryResponse                              */
/** Description:   Extract the <qry_drive_lock_request> response.    */
/**                                                                  */
/** Parse the response of the XAPI XML <qry_drive_lock>              */
/** request and update the appropriate fields of the                 */
/** ACSAPI QUERY LOCK DRIVE response.                                */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <qry_drive_lock_request> response consists of:      */
/**==================================================================*/
/** <libreply>                                                       */
/**   <qry_drive_lock_request>                                       */
/**     <header>                                                     */
/**       header element children                                    */
/**     </header>                                                    */
/**     <status>SSS...SSS</status>                                   */
/**     <cdk_drive_status>                                           */
/**       <status>SSS...SSS</status>                                 */
/**       <drive_location_id>aa:ll:pp:dd:zz</drive_location_id>      */
/**       <lock_id>nnnnn</lock_id>                                   */
/**       <user_label>nnnnn</user_label>                             */
/**       <lock_duration>nnnnnnnnnn</lock_duration>                  */
/**       <locks_pending>nnnnnnnnnn</locks_pending>                  */
/**     </cdk_drive_status>                                          */
/**     ...repeated <cdk_drive_status> entries                       */
/**     <exceptions>                                                 */
/**       <reason>ccc...ccc</reason>                                 */
/**       ...repeated <reason> entries                               */
/**     </exceptions>                                                */
/**     <uui_return_code>nnnn</uui_return_code>                      */
/**     <uui_reason_code>nnnn</uui_reason_code>                      */
/**   </qry_drive_lock_request>                                      */
/** </libreply>                                                      */
/**                                                                  */
/** NOTE: Possible <qry_drive_lock><status> values are:              */
/**       "success", "parser error", "process failure",              */
/**       "lockid not found", "lockid mismatch",                     */
/**       "database error", and "no subfile"                         */
/**                                                                  */
/** NOTE: Possible <cdk_drive_status><status> values are:            */
/**       "success", "cancelled", "deadlock", "locked",              */
/**       "lockid mismatch", "not in library",                       */
/**       "not locked", and "unknown".                               */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY LOCK DRIVE response consists of:                */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   QUERY_LOCK_RESPONSE data consisting of:                     */
/**      i.   type                                                   */
/**      ii.  count                                                  */
/**      iii. QL_DRV_STATUS(count) data entries consisting of:       */
/**           a.   DRIVE_ID consisting of:                           */
/**                1.   drive_id.panel_id.lsm_id.acs                 */
/**                2.   drive_id.panel_id.lsm_id.lsm                 */
/**                3.   drive_id.panel_id.panel                      */
/**                4.   drive_id.drive                               */
/**           b.   lock_id (unsigned short or NO_LOCK_ID)            */
/**           c.   lock_duration (unsigned long)                     */
/**           d.   locks_pending (unsigned int)                      */
/**           e.   USERID consisting of:                             */
/**                1.   user_label (64 character userid)             */
/**           f.   status                                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractQueryResponse"

static int extractQueryResponse(struct XAPICVT  *pXapicvt,
                                struct XAPIREQE *pXapireqe,
                                char            *pQueryResponse,
                                struct XMLPARSE *pXmlparse)
{
    int                 queryRC;
    int                 headerRC;
    int                 i;
    int                 j;
    int                 drvResponseCount;
    int                 drvRemainingCount;
    int                 drvPacketCount;
    int                 drvResponseSize;
    unsigned int        wkInt;

    char                queryStatus[16];
    char                wkString[EXTERNAL_USERID_SIZE + 1];
    struct RAWQLOCK     rawqlock;
    struct RAWQLOCK    *pRawqlock           = &rawqlock;

    struct XMLELEM     *pParentXmlelem;
    struct XMLELEM     *pQryDriveXmlelem;
    struct XMLELEM     *pStatusXmlelem;
    struct XMLELEM     *pFirstCdkStatusXmlelem;
    struct XMLELEM     *pNextCdkStatusXmlelem;
    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;
    struct RAWCOMMON   *pRawcommon          = &(pXapicommon->rawcommon);
    struct XAPICFG     *pXapicfg;

    QUERY_LOCK_RESPONSE *pQuery_Lock_Response = 
    (QUERY_LOCK_RESPONSE*) pQueryResponse;
    QL_DRV_STATUS      *pQl_Drv_Status      = &(pQuery_Lock_Response->identifier_status.drive_status[0]);

    struct CDKSTATUS    cdkstatus[]         =
    {
        "SUCCESS         ",  STATUS_SUCCESS,
        "CANCELLED       ",  STATUS_CANCELLED,
        "DATABASE ERROR  ",  STATUS_DATABASE_ERROR,
        "DEADLOCK        ",  STATUS_DEADLOCK,
        "LOCKED          ",  STATUS_DRIVE_IN_USE,
        "LOCKID MISMATCH ",  STATUS_INVALID_LOCKID,
        "LOCKID NOT FOUND",  STATUS_LOCKID_NOT_FOUND,
        "NO SUBFILE      ",  STATUS_DATABASE_ERROR,
        "NOT IN LIBRARY  ",  STATUS_DRIVE_NOT_IN_LIBRARY,
        "NOT LOCKED      ",  STATUS_DRIVE_AVAILABLE,
        "PARSER ERROR    ",  STATUS_PROCESS_FAILURE,
        "PROCESS FAILURE ",  STATUS_PROCESS_FAILURE,
        "UNKNOWN         ",  STATUS_PROCESS_FAILURE,
    };

    int                 cdkstatusCount      = sizeof(cdkstatus) / 
                                              sizeof(struct CDKSTATUS);

    struct XMLSTRUCT    cdkStatusXmlstruct[] =
    {
        XNAME_cdk_drive_status,             XNAME_status,
        sizeof(pRawqlock->resStatus),       pRawqlock->resStatus,
        BLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cdk_drive_status,             XNAME_drive_location_id,
        sizeof(pRawqlock->driveLocId),      pRawqlock->driveLocId,
        BLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cdk_drive_status,             XNAME_lock_id,
        sizeof(pRawqlock->lockId),          pRawqlock->lockId,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cdk_drive_status,             XNAME_lock_duration,
        sizeof(pRawqlock->lockDuration),    pRawqlock->lockDuration,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cdk_drive_status,             XNAME_locks_pending,
        sizeof(pRawqlock->locksPending),    pRawqlock->locksPending,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cdk_drive_status,             XNAME_user_label,
        sizeof(pRawqlock->userLabel),       pRawqlock->userLabel,
        NOBLANKFILL, NOBITVALUE, 0, 0,
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
    /* Get the <qry_drive_lock_request><status> element.             */
    /*****************************************************************/
    queryRC = STATUS_NI_FAILURE;

    pQryDriveXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                               pXmlparse->pHocXmlelem,
                                               XNAME_qry_drive_lock_request);

    if (pQryDriveXmlelem != NULL)
    {
        pStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                 pQryDriveXmlelem,
                                                 XNAME_status);

        if (pStatusXmlelem != NULL)
        {
            memset(queryStatus, ' ', sizeof(queryStatus));

            memcpy(queryStatus,
                   pStatusXmlelem->pContent,
                   pStatusXmlelem->contentLen);

            for (i = 0;
                i < sizeof(queryStatus);
                i++)
            {
                queryStatus[i] = toupper(queryStatus[i]);
            }

            for (i = 0;
                i < cdkstatusCount;              
                i++)
            {
                if (memcmp(cdkstatus[i].statusString, 
                           queryStatus,
                           sizeof(queryStatus)) == 0)
                {
                    queryRC = cdkstatus[i].statusCode;

                    break;
                }
            }

            TRMSGI(TRCI_XAPI,
                   "[i]=%d, queryRC=%d, queryStatus=%.16s\n",
                   i,
                   queryRC,
                   queryStatus);
        }
    }

    /*****************************************************************/
    /* Count the number of <cdk_drive_status> entries.               */
    /*****************************************************************/
    pFirstCdkStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                     pXmlparse->pHocXmlelem,
                                                     XNAME_cdk_drive_status);

    drvResponseCount = 0;

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
            drvResponseCount++;

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
           "drvResponseCount=%d, MAX_ID=%d\n",
           drvResponseCount,
           MAX_ID);

    /*****************************************************************/
    /* If no <cdk_drive_status> elements found, then return an error.*/
    /*****************************************************************/
    if (drvResponseCount == 0)
    {
        if (headerRC > 0)
        {
            return headerRC;
        }
        else if (queryRC != STATUS_SUCCESS)
        {
            return queryRC;
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
    drvRemainingCount = drvResponseCount;

    while (1)
    {
        if (drvRemainingCount > MAX_ID)
        {
            drvPacketCount = MAX_ID;
        }
        else
        {
            drvPacketCount = drvRemainingCount;
        }

        drvResponseSize = (char*) pQl_Drv_Status -
                          (char*) pQuery_Lock_Response +
                          ((sizeof(QL_DRV_STATUS)) * drvPacketCount);

        xapi_lock_init_resp(pXapireqe,
                            (char*) pQueryResponse,
                            drvResponseSize);

        pQuery_Lock_Response = (QUERY_LOCK_RESPONSE*) pQueryResponse;
        pQl_Drv_Status = &(pQuery_Lock_Response->identifier_status.drive_status[0]);

        TRMSGI(TRCI_XAPI,
               "At top of while; drvResponse=%d, "
               "drvRemaining=%d, drvPacket=%d, MAX_ID=%d\n",
               drvResponseCount,
               drvRemainingCount,
               drvPacketCount,
               MAX_ID);

        pQuery_Lock_Response->count = drvPacketCount;
        pQuery_Lock_Response->message_status.status = (STATUS) queryRC;

        for (i = 0;
            i < drvPacketCount;
            i++, pQl_Drv_Status++)
        {
            if (pNextCdkStatusXmlelem == NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextCdkStatusXmlelem=NULL at driveCount=%i\n",
                       (i+1));

                break;
            }

            memset((char*) pRawqlock, 0, sizeof(struct RAWQLOCK));

            pQl_Drv_Status->status = STATUS_SUCCESS;

            FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                              pNextCdkStatusXmlelem,
                                              &cdkStatusXmlstruct[0],
                                              cdkStatusElementCount);

            TRMEMI(TRCI_XAPI, 
                   pRawqlock, 
                   sizeof(struct RAWQLOCK),
                   "[i]=%d, RAWQLOCK:\n",
                   i);

            if (pRawqlock->driveLocId[0] > ' ')
            {
                pXapicfg = xapi_config_search_drivelocid(pXapicvt,
                                                         pXapireqe,
                                                         pRawqlock->driveLocId);

                if (pXapicfg != NULL)
                {
                    pQl_Drv_Status->drive_id.panel_id.lsm_id.acs = pXapicfg->libdrvid.acs;
                    pQl_Drv_Status->drive_id.panel_id.lsm_id.lsm = pXapicfg->libdrvid.lsm;
                    pQl_Drv_Status->drive_id.panel_id.panel = pXapicfg->libdrvid.panel;
                    pQl_Drv_Status->drive_id.drive = pXapicfg->libdrvid.driveNumber;
                }
                else
                {
                    pQl_Drv_Status->status = STATUS_DRIVE_NOT_IN_LIBRARY;
                }
            }
            else
            {
                pQl_Drv_Status->status = STATUS_DRIVE_NOT_IN_LIBRARY;
            }

            if (pRawqlock->lockId[0] > 0)
            {
                wkInt = 0;
                memset(wkString, 0 , sizeof(wkString));

                memcpy(wkString, 
                       pRawqlock->lockId, 
                       sizeof(pRawqlock->lockId));

                wkInt = atoi(wkString);
                pQl_Drv_Status->lock_id = (LOCKID) wkInt;
            }

            if (pRawqlock->lockDuration[0] > 0)
            {
                wkInt = 0;
                memset(wkString, 0 , sizeof(wkString));

                memcpy(wkString, 
                       pRawqlock->lockDuration, 
                       sizeof(pRawqlock->lockDuration));

                wkInt = atoi(wkString);
                pQl_Drv_Status->lock_duration = (unsigned long) wkInt;
            }

            if (pRawqlock->locksPending[0] > 0)
            {
                wkInt = 0;
                memset(wkString, 0 , sizeof(wkString));

                memcpy(wkString, 
                       pRawqlock->locksPending, 
                       sizeof(pRawqlock->locksPending));

                wkInt = atoi(wkString);
                pQl_Drv_Status->locks_pending = wkInt;
            }

            if (pRawqlock->userLabel[0] > 0)
            {
                memcpy(pQl_Drv_Status->user_id.user_label, 
                       pRawqlock->userLabel, 
                       sizeof(pRawqlock->userLabel));
            }

            if ((pQl_Drv_Status->status == STATUS_SUCCESS) &&
                (pRawqlock->resStatus[0] > ' '))
            {
                pQl_Drv_Status->status = STATUS_PROCESS_FAILURE;

                for (j = 0;
                    j < sizeof(pRawqlock->resStatus);
                    j++)
                {
                    pRawqlock->resStatus[i] = toupper(pRawqlock->resStatus[i]);
                }

                for (j = 0;
                    j < cdkstatusCount;              
                    j++)
                {
                    if (memcmp(cdkstatus[j].statusString, 
                               pRawqlock->resStatus,
                               sizeof(pRawqlock->resStatus)) == 0)
                    {
                        pRawqlock->resRC = cdkstatus[j].statusCode;
                        pQl_Drv_Status->status = (STATUS) pRawqlock->resRC;

                        break;
                    }
                }

                TRMSGI(TRCI_XAPI,
                       "[j]=%d, status=%d, resStatus=%.16s\n",
                       j,
                       (int) pQl_Drv_Status->status,
                       pRawqlock->resStatus);
            }

            pNextCdkStatusXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                                 pParentXmlelem,
                                                                 pNextCdkStatusXmlelem,
                                                                 XNAME_cdk_drive_status);
        } /* for(i) */

        drvRemainingCount = drvRemainingCount - drvPacketCount;

        if (drvRemainingCount <= 0)
        {
            xapi_fin_response(pXapireqe,
                              pQueryResponse,     
                              drvResponseSize); 

            break; 
        }
        else
        {
            xapi_int_response(pXapireqe,
                              pQueryResponse,     
                              drvResponseSize);  
        }
    }

    return STATUS_SUCCESS;
}



