/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */ 
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qlock_vol.c                                 */
/** Description:    XAPI QUERY LOCK VOLUME processor.                */
/**                                                                  */
/**                 Return lock information for the specified        */
/**                 volume(s).                                       */
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
/** Function Name: xapi_qlock_vol                                    */
/** Description:   XAPI QUERY LOCK VOLUME processor.                 */
/**                                                                  */
/** Return lock information for the specified volumes(s) using       */
/** the specified (i.e. already existing) LOCKID, or if the input    */
/** LOCKID is specified as 0 (or NO_LOCK_ID), then return any        */
/** lock information for the specified volume(s).                    */
/**                                                                  */
/** VOLUME LOCK(s) are managed on the server.  The ACSAPI format     */
/** QUERY LOCK VOLUME request is translated into an XAPI XML format  */
/** <qry_volume_lock> request; the XAPI XML request is then          */
/** transmitted to the server via TCP/IP;  The received XAPI XML     */
/** response is then translated into one or more ACSAPI              */
/** QUERY LOCK VOLUME responses.                                     */
/**                                                                  */
/** There will only be one individual XAPI <qry_volume_lock> request */
/** issued for a single ACSAPI QUERY LOCK VOLUME request regardless  */
/** of the requested volume count (as the XAPI <qry_volume_lock>     */
/** request allows a <volume_list> to be specified).                 */
/**                                                                  */
/** The QUERY LOCK VOLUME command is allowed to proceed even when    */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qlock_vol"

extern int xapi_qlock_vol(struct XAPICVT  *pXapicvt,
                          struct XAPIREQE *pXapireqe)
{
    int                 queryRC              = STATUS_SUCCESS;
    int                 lastRC;
    int                 i;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    QUERY_LOCK_REQUEST *pQuery_Lock_Request = (QUERY_LOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    int                 volumeCount         = pQuery_Lock_Request->count;

    QUERY_LOCK_RESPONSE wkQuery_Lock_Response;
    QUERY_LOCK_RESPONSE *pQuery_Lock_Response = &wkQuery_Lock_Response;

    xapi_qlock_init_resp(pXapireqe,
                         (char*) pQuery_Lock_Response,
                         sizeof(QUERY_LOCK_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY LOCK VOLUME request=%08X, size=%d, count=%d, "
           "MAX_ID=%d, QUERY LOCK VOLUME response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           volumeCount,
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
    /* Now generate the QUERY LOCK VOLUME ACSAPI response.           */
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
/** Description:   Build an XAPI <qry_volume_lock> request.          */
/**                                                                  */
/** Convert the ACSAPI format QUERY LOCK VOLUME request into an      */
/** XAPI XML format <qry_volume_lock> request.                       */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY LOCK VOLUME request consists of:                */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY LOCK VOLUME data consisting of:                       */
/**      i.   type                                                   */
/**      ii.  count                                                  */
/**      iii. VOLID[count] data entries consisting of:               */
/**           a.   external_label (6 character volser)               */
/**                                                                  */
/** NOTE: The input LOCKID is in the MESSAGE_HEADER.lock_id field.   */
/**       If specified as 0 (or NO_LOCK_ID), then ALL matching       */
/**       volumes that are currently locked are returned.            */
/**                                                                  */
/** NOTE: Specifying a count of 0 will return ALL volumes with a     */
/**       matching LOCKID (or all locked volumes if NO_LOCK_ID is    */
/**       also specified).                                           */
/**                                                                  */
/** To summarize:                                                    */
/**       (1) If count is non-zero, and LOCKID is non-zero,          */
/**           then the response will include all matching volumes    */
/**           currently locked with the specified LOCKID.            */
/**                                                                  */
/**       (2) If count is 0, but LOCKID is non-zero, then the        */
/**           response includes all volumes locked with the          */
/**           specified LOCKID.                                      */
/**                                                                  */
/**       (3) If count is non-zero, but LOCKID is 0, then the        */
/**           response includes all matching volumes locked with     */
/**           any LOCKID.                                            */
/**                                                                  */
/**       (4) If both count and LOCKID are 0, then the               */
/**           response includes all locked volumes with any LOCKID.  */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <qry_volume_lock> request consists of:              */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <qry_volume_lock>                                            */
/**       <lock_id>nnnnn</lock_id>                                   */
/**       <volume_list>                                              */
/**         <volser>vvvvvv</volser>                                  */
/**         ...repeated <volser> entries                             */
/**       </volume_list>                                             */
/**     </qry_volume_lock>                                           */
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
    VOLID              *pVolid              = &(pQuery_Lock_Request->identifier.vol_id[0]);

    int                 volumeCount         = pQuery_Lock_Request->count;
    int                 lockId              = pMessage_Header->lock_id;

    char                lockIdString[6];
    char                volserString[7];

    char               *pXapiRequest        = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    sprintf(lockIdString,
            "%d",
            lockId);

    TRMSGI(TRCI_XAPI,
           "Entered; lockId=%s, volumeCount=%d\n",
           lockIdString,
           volumeCount);

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

    if (volumeCount < 0)
    {
        return STATUS_COUNT_TOO_SMALL;
    }

    if (volumeCount > MAX_ID)
    {
        return STATUS_COUNT_TOO_LARGE;
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
                                      XNAME_qry_volume_lock,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_lock_id,
                                      lockIdString,
                                      0);

    /*****************************************************************/
    /* If any of the specified volumes is not found, then            */
    /* we still process the request for all specified volumes.       */
    /* In such cases, the response will contain both found and       */
    /* not founnd volumes.                                           */
    /*****************************************************************/
    if (volumeCount > 0)
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_volume_list,
                                          NULL,
                                          0);

        pParentXmlelem = pXmlparse->pCurrXmlelem;

        for (i = 0, pVolid = &(pQuery_Lock_Request->identifier.vol_id[0]);
            i < volumeCount;
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
/** Description:   Extract the <qry_volume_lock_request> response.   */
/**                                                                  */
/** Parse the response of the XAPI XML <qry_volume_lock>             */
/** request and update the appropriate fields of the                 */
/** ACSAPI QUERY LOCK VOLUME response.                               */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <qry_volume_lock_request> response consists of:     */
/**==================================================================*/
/** <libreply>                                                       */
/**   <qry_volume_lock_request>                                      */
/**     <header>                                                     */
/**       header element children                                    */
/**     </header>                                                    */
/**     <status>SSS...SSS</status>                                   */
/**     <cdk_volume_status>                                          */
/**       <status>SSS...SSS</status>                                 */
/**       <volser>vvvvvv</volser>                                    */
/**       <lock_id>nnnnn</lock_id>                                   */
/**       <user_label>nnnnn</user_label>                             */
/**       <lock_duration>nnnnnnnnnn</lock_duration>                  */
/**       <locks_pending>nnnnnnnnnn</locks_pending>                  */
/**     </cdk_volume_status>                                         */
/**     ...repeated <cdk_volume_status> entries                      */
/**     <exceptions>                                                 */
/**       <reason>ccc...ccc</reason>                                 */
/**       ...repeated <reason> entries                               */
/**     </exceptions>                                                */
/**     <uui_return_code>nnnn</uui_return_code>                      */
/**     <uui_reason_code>nnnn</uui_reason_code>                      */
/**   </qry_volume_lock_request>                                     */
/** </libreply>                                                      */
/**                                                                  */
/** NOTE: Possible <qry_volume_lock><status> values are:             */
/**       "success", "parser error", "process failure",              */
/**       "lockid not found", "lockid mismatch",                     */
/**       "database error", and "no subfile"                         */
/**                                                                  */
/** NOTE: Possible <cdk_volume_status><status> values are:           */
/**       "success", "cancelled", "deadlock", "locked",              */
/**       "lockid mismatch", "not in library", and "unknown".        */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY LOCK VOLUME response consists of:               */
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
/**      iii. QL_VOL_STATUS(count) data entries consisting of:       */
/**           a.   VOLID consisting of:                              */
/**                1.   external_label (6 character volser)          */
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
    int                 volResponseCount;
    int                 volRemainingCount;
    int                 volPacketCount;
    int                 volResponseSize;
    unsigned int        wkInt;

    char                queryStatus[16];
    char                wkString[EXTERNAL_USERID_SIZE + 1];
    struct RAWQLOCK     rawqlock;
    struct RAWQLOCK    *pRawqlock           = &rawqlock;

    struct XMLELEM     *pParentXmlelem;
    struct XMLELEM     *pQryVolumeXmlelem;
    struct XMLELEM     *pStatusXmlelem;
    struct XMLELEM     *pFirstCdkStatusXmlelem;
    struct XMLELEM     *pNextCdkStatusXmlelem;
    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;
    struct RAWCOMMON   *pRawcommon          = &(pXapicommon->rawcommon);
    struct XAPICFG     *pXapicfg;

    QUERY_LOCK_RESPONSE *pQuery_Lock_Response = 
    (QUERY_LOCK_RESPONSE*) pQueryResponse;
    QL_VOL_STATUS      *pQl_Vol_Status      = &(pQuery_Lock_Response->identifier_status.volume_status[0]);

    struct CDKSTATUS    cdkstatus[]         =
    {
        "SUCCESS         ",  STATUS_SUCCESS,
        "CANCELLED       ",  STATUS_CANCELLED,
        "DATABASE ERROR  ",  STATUS_DATABASE_ERROR,
        "DEADLOCK        ",  STATUS_DEADLOCK,
        "LOCKED          ",  STATUS_VOLUME_IN_USE,
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
        sizeof(pRawqlock->resStatus),       pRawqlock->resStatus,
        BLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cdk_volume_status,            XNAME_volser,
        sizeof(pRawqlock->volser),          pRawqlock->volser,
        BLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cdk_volume_status,            XNAME_lock_id,
        sizeof(pRawqlock->lockId),          pRawqlock->lockId,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cdk_volume_status,            XNAME_lock_duration,
        sizeof(pRawqlock->lockDuration),    pRawqlock->lockDuration,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cdk_volume_status,            XNAME_locks_pending,
        sizeof(pRawqlock->locksPending),    pRawqlock->locksPending,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cdk_volume_status,            XNAME_user_label,
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
    /* Get the <qry_volume_lock_request><status> element.            */
    /*****************************************************************/
    queryRC = STATUS_NI_FAILURE;

    pQryVolumeXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                pXmlparse->pHocXmlelem,
                                                XNAME_qry_volume_lock_request);

    if (pQryVolumeXmlelem != NULL)
    {
        pStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                 pQryVolumeXmlelem,
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
    /* Count the number of <cdk_volume_status> entries.               */
    /*****************************************************************/
    pFirstCdkStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                     pXmlparse->pHocXmlelem,
                                                     XNAME_cdk_volume_status);

    volResponseCount = 0;

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
            volResponseCount++;

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
           "volResponseCount=%d, MAX_ID=%d\n",
           volResponseCount,
           MAX_ID);

    /*****************************************************************/
    /* If no <cdk_volume_status> elements found, then return an error.*/
    /*****************************************************************/
    if (volResponseCount == 0)
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
    /* If <cdk_volume_status> elements found, then extract the data. */
    /*****************************************************************/
    pNextCdkStatusXmlelem = pFirstCdkStatusXmlelem;
    volRemainingCount = volResponseCount;

    while (1)
    {
        if (volRemainingCount > MAX_ID)
        {
            volPacketCount = MAX_ID;
        }
        else
        {
            volPacketCount = volRemainingCount;
        }

        volResponseSize = (char*) pQl_Vol_Status -
                          (char*) pQuery_Lock_Response +
                          ((sizeof(QL_VOL_STATUS)) * volPacketCount);

        xapi_lock_init_resp(pXapireqe,
                            (char*) pQueryResponse,
                            volResponseSize);

        pQuery_Lock_Response = (QUERY_LOCK_RESPONSE*) pQueryResponse;
        pQl_Vol_Status = &(pQuery_Lock_Response->identifier_status.volume_status[0]);

        TRMSGI(TRCI_XAPI,
               "At top of while; volResponse=%d, "
               "volRemaining=%d, volPacket=%d, MAX_ID=%d\n",
               volResponseCount,
               volRemainingCount,
               volPacketCount,
               MAX_ID);

        pQuery_Lock_Response->count = volPacketCount;
        pQuery_Lock_Response->message_status.status = (STATUS) queryRC;

        for (i = 0;
            i < volPacketCount;
            i++, pQl_Vol_Status++)
        {
            if (pNextCdkStatusXmlelem == NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextCdkStatusXmlelem=NULL at volumeCount=%i\n",
                       (i+1));

                break;
            }

            memset((char*) pRawqlock, 0, sizeof(struct RAWQLOCK));

            pQl_Vol_Status->status = STATUS_SUCCESS;

            FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                              pNextCdkStatusXmlelem,
                                              &cdkStatusXmlstruct[0],
                                              cdkStatusElementCount);

            TRMEMI(TRCI_XAPI, 
                   pRawqlock, 
                   sizeof(struct RAWQLOCK),
                   "[i]=%d, RAWQLOCK:\n",
                   i);

            if (pRawqlock->volser[0] > ' ')
            {
                memcpy(pQl_Vol_Status->vol_id.external_label,
                       pRawqlock->volser,
                       XAPI_VOLSER_SIZE);
            }
            else
            {
                pQl_Vol_Status->status = STATUS_VOLUME_NOT_IN_LIBRARY;
            }

            if (pRawqlock->lockId[0] > 0)
            {
                wkInt = 0;
                memset(wkString, 0 , sizeof(wkString));

                memcpy(wkString, 
                       pRawqlock->lockId, 
                       sizeof(pRawqlock->lockId));

                wkInt = atoi(wkString);
                pQl_Vol_Status->lock_id = (LOCKID) wkInt;
            }

            if (pRawqlock->lockDuration[0] > 0)
            {
                wkInt = 0;
                memset(wkString, 0 , sizeof(wkString));

                memcpy(wkString, 
                       pRawqlock->lockDuration, 
                       sizeof(pRawqlock->lockDuration));

                wkInt = atoi(wkString);
                pQl_Vol_Status->lock_duration = (unsigned long) wkInt;
            }

            if (pRawqlock->locksPending[0] > 0)
            {
                wkInt = 0;
                memset(wkString, 0 , sizeof(wkString));

                memcpy(wkString, 
                       pRawqlock->locksPending, 
                       sizeof(pRawqlock->locksPending));

                wkInt = atoi(wkString);
                pQl_Vol_Status->locks_pending = wkInt;
            }

            if (pRawqlock->userLabel[0] > 0)
            {
                memcpy(pQl_Vol_Status->user_id.user_label, 
                       pRawqlock->userLabel, 
                       sizeof(pRawqlock->userLabel));
            }

            if ((pQl_Vol_Status->status == STATUS_SUCCESS) &&
                (pRawqlock->resStatus[0] > ' '))
            {
                pQl_Vol_Status->status = STATUS_PROCESS_FAILURE;

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
                        pQl_Vol_Status->status = (STATUS) pRawqlock->resRC;

                        break;
                    }
                }

                TRMSGI(TRCI_XAPI,
                       "[j]=%d, status=%d, resStatus=%.16s\n",
                       j,
                       (int) pQl_Vol_Status->status,
                       pRawqlock->resStatus);
            }

            pNextCdkStatusXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                                 pParentXmlelem,
                                                                 pNextCdkStatusXmlelem,
                                                                 XNAME_cdk_volume_status);
        } /* for(i) */

        volRemainingCount = volRemainingCount - volPacketCount;

        if (volRemainingCount <= 0)
        {
            xapi_fin_response(pXapireqe,
                              pQueryResponse,     
                              volResponseSize); 

            break; 
        }
        else
        {
            xapi_int_response(pXapireqe,
                              pQueryResponse,     
                              volResponseSize);  
        }
    }

    return STATUS_SUCCESS;
}


