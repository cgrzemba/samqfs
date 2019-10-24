/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qlock_drv_one.c                             */
/** Description:    XAPI client query drive lock service.            */
/**                                                                  */
/**                 Return lock information for a single drive.      */
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

#include "api/defs_api.h"
#include "csi.h"
#include "smccxmlx.h"
#include "srvcommon.h"
#include "xapi.h"


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#define QUERY_SEND_TIMEOUT         5   /* TIMEOUT values in seconds  */       
#define QUERY_RECV_TIMEOUT_1ST     300
#define QUERY_RECV_TIMEOUT_NON1ST  600


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int buildQlockRequest(struct XAPICVT   *pXapicvt,
                             struct XAPIREQE  *pXapireqe,
                             char             *pDriveLocIdString,
                             char             *pLockIdString,
                             char            **ptrXapiBuffer,
                             int              *pXapiBufferSize);

static int extractQlockResponse(struct XAPICVT   *pXapicvt,
                                struct XAPIREQE  *pXapireqe,
                                struct XMLPARSE  *pXmlparse,
                                struct RAWQLOCK  *pRawqlock);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qlock_drv_one                                */
/** Description:   XAPI client query drive lock service.             */
/**                                                                  */
/** Return lock information for the single input drive using a       */
/** specific (i.e. already existing) LOCKID.                         */
/**                                                                  */
/** DRIVE LOCK(s) are managed on the server.  Create an              */
/** XAPI XML format <qry_drive_lock> request; the XAPI XML request   */
/** is then transmitted to the server via TCP/IP;  The received      */
/** XAPI XML response is then translated into a single RAWQLOCK      */
/** structure containing the lock information for the input drive.   */
/**                                                                  */
/** The XAPI client query drive lock service is allowed to proceed   */
/** even when the XAPI client is in the IDLE state.                  */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qlock_drv_one"

extern int xapi_qlock_drv_one(struct XAPICVT   *pXapicvt,
                              struct XAPIREQE  *pXapireqe,
                              struct RAWQLOCK  *pRawqlock,
                              char              driveLocId[16],
                              int               lockId)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;
    char                driveLocIdString[17];
    char                lockIdString[6];

    STRIP_TRAILING_BLANKS(driveLocId,
                          driveLocIdString,
                          16);

    sprintf(lockIdString, "%d", lockId);

    TRMSGI(TRCI_XAPI,
           "Entered; driveLocId=%s, lockId=%s\n",
           driveLocIdString,
           lockIdString);

    lastRC = buildQlockRequest(pXapicvt,
                               pXapireqe,
                               driveLocIdString,
                               lockIdString,
                               &pXapiBuffer,
                               &xapiBufferSize);

    TRMSGI(TRCI_XAPI, 
           "lastRC=%d from buildQlockRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

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
    /* Now extract the lock data.                                    */
    /*****************************************************************/
    if (queryRC == STATUS_SUCCESS)
    {
        lastRC = extractQlockResponse(pXapicvt,
                                      pXapireqe,
                                      pXmlparse,
                                      pRawqlock);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractQlockResponse\n",
               lastRC);
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
/** Function Name: buildQlockRequest                                 */
/** Description:   Build an XAPI <qry_drive_lock> request.           */
/**                                                                  */
/** Build the XAPI XML format <qry_drive_lock> request for the       */
/** specified drive and LOCKID.                                      */
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
/** NOTE: <lock_id> is required and must be between 1 and 32767.     */
/**                                                                  */
/** NOTE: This service always specifies a single <drive_location_id>.*/
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "buildQlockRequest"

static int buildQlockRequest(struct XAPICVT   *pXapicvt,
                             struct XAPIREQE  *pXapireqe,
                             char             *pDriveLocIdString,
                             char             *pLockIdString,
                             char            **ptrXapiBuffer,
                             int              *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;

    char               *pXapiRequest        = NULL;

    struct XMLPARSE    *pXmlparse; 
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    TRMSGI(TRCI_XAPI,
           "Entered; driveLocId=%s, lockId=%s\n",
           pDriveLocIdString,
           pLockIdString);

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

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
                                      pLockIdString,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_drive_list,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_drive_location_id,
                                      pDriveLocIdString,
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
/** Function Name: extractQlockResponse                              */
/** Description:   Extract the <qry_drive_lock_request> response.    */
/**                                                                  */
/** Parse the response of the XAPI XML <qry_drive_lock>              */
/** request and update the appropriate fields of the RAWQLOCK        */
/** structure.                                                       */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <qry_drive_lock> responses consists of:             */
/**==================================================================*/
/** <libreply>                                                       */
/**   <qry_drive_lock_request>                                       */
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
/**       "lockid mismatch", "not in library", and "unknown".        */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractQlockResponse"

static int extractQlockResponse(struct XAPICVT   *pXapicvt,
                                struct XAPIREQE  *pXapireqe,
                                struct XMLPARSE  *pXmlparse,
                                struct RAWQLOCK  *pRawqlock)
{
    int                 headerRC;
    int                 i;

    struct XMLELEM     *pQryLockXmlelem;
    struct XMLELEM     *pCdkStatusXmlelem;
    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;
    struct RAWCOMMON   *pRawcommon          = &(pXapicommon->rawcommon);

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
        "PARSER ERROR    ",  STATUS_PROCESS_FAILURE,
        "PROCESS FAILURE ",  STATUS_PROCESS_FAILURE,
        "UNKNOWN         ",  STATUS_PROCESS_FAILURE,
    };

    int                 cdkstatusCount      = sizeof(cdkstatus) / 
                                              sizeof(struct CDKSTATUS);

    struct XMLSTRUCT    qryLockXmlstruct[] =
    {
        XNAME_qry_drive_lock_request,       XNAME_status,
        sizeof(pRawqlock->queryStatus),     pRawqlock->queryStatus,
        BLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 qryLockElementCount = sizeof(qryLockXmlstruct) / 
                                              sizeof(struct XMLSTRUCT);

    struct XMLSTRUCT    cdkStatusXmlstruct[] =
    {
        XNAME_cdk_drive_status,             XNAME_status,
        sizeof(pRawqlock->resStatus),       pRawqlock->resStatus,
        BLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cdk_drive_status,             XNAME_drive_location_id,
        sizeof(pRawqlock->driveLocId),      pRawqlock->driveLocId,
        BLANKFILL, NOBITVALUE, 0, 0,
        XNAME_qry_drive_lock_request,       XNAME_lock_id,
        sizeof(pRawqlock->lockId),          pRawqlock->lockId,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_qry_drive_lock_request,       XNAME_lock_duration,
        sizeof(pRawqlock->lockDuration),    pRawqlock->lockDuration,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_cdk_drive_status,             XNAME_locks_pending,
        sizeof(pRawqlock->locksPending),    pRawqlock->locksPending,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_qry_drive_lock_request,       XNAME_user_label,
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

    pQryLockXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                              pXmlparse->pHocXmlelem,
                                              XNAME_qry_drive_lock_request);

    TRMSGI(TRCI_XAPI, 
           "pQryLockXmlelem=%08X\n",
           pQryLockXmlelem);

    if (pQryLockXmlelem == NULL)
    {
        if (headerRC != STATUS_SUCCESS)
        {
            return headerRC;
        }
        else
        {
            return STATUS_NI_FAILURE;
        }
    }

    memset(pRawqlock, 0, sizeof(struct RAWQLOCK));

    FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                      pQryLockXmlelem,
                                      &qryLockXmlstruct[0],
                                      qryLockElementCount);

    pCdkStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                pQryLockXmlelem,
                                                XNAME_cdk_drive_status);

    TRMSGI(TRCI_XAPI,
           "pCdkStatusXmlelem=%08X\n",
           pCdkStatusXmlelem);

    if (pCdkStatusXmlelem != NULL)
    {
        FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                          pCdkStatusXmlelem,
                                          &cdkStatusXmlstruct[0],
                                          cdkStatusElementCount);
    }

    for (i = 0;
        i < sizeof(pRawqlock->queryStatus);
        i++)
    {
        pRawqlock->queryStatus[i] = toupper(pRawqlock->queryStatus[i]);
    }

    for (i = 0;
        i < cdkstatusCount;              
        i++)
    {
        if (memcmp(cdkstatus[i].statusString, 
                   pRawqlock->queryStatus,
                   sizeof(pRawqlock->queryStatus)) == 0)
        {
            pRawqlock->queryRC = cdkstatus[i].statusCode;

            break;
        }
    }

    TRMSGI(TRCI_XAPI,
           "[i]=%d, queryRC=%d, queryStatus=%.16s\n",
           i,
           pRawqlock->queryRC,
           pRawqlock->queryStatus);

    for (i = 0;
        i < sizeof(pRawqlock->resStatus);
        i++)
    {
        pRawqlock->resStatus[i] = toupper(pRawqlock->resStatus[i]);
    }

    for (i = 0;
        i < cdkstatusCount;              
        i++)
    {
        if (memcmp(cdkstatus[i].statusString, 
                   pRawqlock->resStatus,
                   sizeof(pRawqlock->resStatus)) == 0)
        {
            pRawqlock->resRC = cdkstatus[i].statusCode;

            break;
        }
    }

    TRMSGI(TRCI_XAPI,
           "[i]=%d, resRC=%d, resStatus=%.16s\n",
           i,
           pRawqlock->resRC,
           pRawqlock->resStatus);

    return STATUS_SUCCESS;
}



