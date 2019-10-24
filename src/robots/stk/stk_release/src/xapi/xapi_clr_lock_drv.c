/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */ 
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_clr_lock_drv.c                              */
/** Description:    XAPI CLEAR LOCK DRIVE processor.                 */
/**                                                                  */
/**                 Remove all current and pending LOCK(s) from      */
/**                 specified drive(s).                              */
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
#define LOCKERR_RESPONSE_SIZE (offsetof(CLEAR_LOCK_RESPONSE, identifier_status)) 
#define CLEAR_SEND_TIMEOUT         5   /* TIMEOUT values in seconds  */       
#define CLEAR_RECV_TIMEOUT_1ST     120
#define CLEAR_RECV_TIMEOUT_NON1ST  900


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int convertClearRequest(struct XAPICVT   *pXapicvt,
                               struct XAPIREQE  *pXapireqe,
                               char            **ptrXapiBuffer,
                               int              *pXapiBufferSize);

static int extractClearResponse(struct XAPICVT  *pXapicvt,
                                struct XAPIREQE *pXapireqe,
                                char            *pClearResponse,
                                struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_clr_lock_drv                                 */
/** Description:   XAPI CLEAR LOCK DRIVE processor.                  */
/**                                                                  */
/** Remove all current and pending LOCK(s) from specified drive(s).  */
/**                                                                  */
/** DRIVE LOCK(s) are managed on the server.                         */
/** The ACSAPI format CLEAR LOCK DRIVE request is translated         */
/** into an XAPI XML format <clr_drive_lock> request; the XAPI XML   */
/** request is then transmitted to the server via TCP/IP;  The       */
/** received XAPI XML response is then translated into one or more   */
/** ACSAPI CLEAR LOCK DRIVE responses.                               */
/**                                                                  */
/** The CLEAR LOCK DRIVE command is NOT allowed to proceed when      */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_clr_lock_drv"

extern int xapi_clr_lock_drv(struct XAPICVT  *pXapicvt,
                             struct XAPIREQE *pXapireqe)
{
    int                 clearRC              = STATUS_SUCCESS;
    int                 lastRC;
    int                 i;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    CLEAR_LOCK_REQUEST *pClear_Lock_Request = (CLEAR_LOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    int                 clearCount          = pClear_Lock_Request->count;

    CLEAR_LOCK_RESPONSE wkClear_Lock_Response;
    CLEAR_LOCK_RESPONSE *pClear_Lock_Response = &wkClear_Lock_Response;

    xapi_lock_init_resp(pXapireqe,
                        (char*) pClear_Lock_Response,
                        sizeof(CLEAR_LOCK_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; CLEAR LOCK DRIVE request=%08X, size=%d, count=%d, "
           "MAX_ID=%d, CLEAR LOCK DRIVE response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           clearCount,
           MAX_ID,
           pClear_Lock_Response,
           LOCKERR_RESPONSE_SIZE);

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pClear_Lock_Response,
                            LOCKERR_RESPONSE_SIZE);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    lastRC = convertClearRequest(pXapicvt,
                                 pXapireqe,
                                 &pXapiBuffer,
                                 &xapiBufferSize);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from convertClearRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pClear_Lock_Response,
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
                      CLEAR_SEND_TIMEOUT,       
                      CLEAR_RECV_TIMEOUT_1ST,   
                      CLEAR_RECV_TIMEOUT_NON1ST,
                      (void**) &pXmlparse);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from xapi_tcp(pBuffer=%08X, size=%d, pXmlparse=%08X)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize,
           pXmlparse);

    if (lastRC != STATUS_SUCCESS)
    {
        clearRC = lastRC;
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
    /* Now generate the CLEAR LOCK DRIVE ACSAPI response.            */
    /*****************************************************************/
    if (clearRC == STATUS_SUCCESS)
    {
        lastRC = extractClearResponse(pXapicvt,
                                      pXapireqe,
                                      (char*) pClear_Lock_Response,
                                      pXmlparse);

        TRMSGI(TRCI_XAPI,"lastRC=%d from extractClearResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            clearRC = lastRC;
        }
    }

    if (clearRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pClear_Lock_Response,
                          LOCKERR_RESPONSE_SIZE,
                          clearRC);
    }

    /*****************************************************************/
    /* When done, free the XMLPARSE and associated structures.       */
    /*****************************************************************/
    if (pXmlparse != NULL)
    {
        FN_FREE_HIERARCHY_STORAGE(pXmlparse);
    }

    return clearRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: convertClearRequest                               */
/** Description:   Build an XAPI <clr_drive_lock> request.           */
/**                                                                  */
/** Convert the ACSAPI format CLEAR LOCK DRIVE request into an       */
/** XAPI XML format <clr_drive_lock> request.                        */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI CLEAR LOCK DRIVE request consists of:                 */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   CLEAR LOCK DRIVE data consisting of:                        */
/**      i.   type                                                   */
/**      ii.  count                                                  */
/**      iii. DRIVEID[count] data entries consisting of:             */
/**           a.   drive_id.panel_id.lsm_id.acs                      */
/**           b.   drive_id.panel_id.lsm_id.lsm                      */
/**           c.   drive_id.panel_id.panel                           */
/**           d.   drive_id.drive                                    */
/**                                                                  */
/** NOTE: The input LOCKID in the MESSAGE_HEADER.lock_id field       */
/**       is irrelevant; all lock IDs will be cleared for the        */
/**       specified DRIVEID(s).                                      */
/**                                                                  */
/** NOTE: Specifying a count of 0 will clear ALL drive locks.        */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <clr_drive_lock> request consists of:               */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <clr_drive_lock>                                             */
/**       <drive_list>                                               */
/**         <drive_location_id>aa:ll:pp:dd:zz</drive_location_id>    */
/**         ...repeated <drive_location_id> entries                  */
/**       </drive_list>                                              */
/**     </clr_drive_lock>                                            */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "convertClearRequest"

static int convertClearRequest(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char           **ptrXapiBuffer,
                               int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 i;
    int                 xapiRequestSize     = 0;
    CLEAR_LOCK_REQUEST *pClear_Lock_Request = (CLEAR_LOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    MESSAGE_HEADER     *pMessage_Header     = &(pClear_Lock_Request->request_header.message_header);
    DRIVEID            *pDriveid            = &(pClear_Lock_Request->identifier.drive_id[0]);

    int                 clearCount          = pClear_Lock_Request->count;
    struct LIBDRVID     libdrvid;
    struct LIBDRVID    *pLibdrvid           = &libdrvid;
    struct XAPICFG     *pXapicfg[MAX_ID];

    char                driveLocIdString[24];

    char               *pXapiRequest        = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    TRMSGI(TRCI_XAPI,
           "Entered; clearCount=%d\n",
           clearCount);

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

    if (clearCount < 0)
    {
        return STATUS_COUNT_TOO_SMALL;
    }

    if (clearCount > MAX_ID)
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
    if (clearCount > 0)
    {
        for (i = 0, pDriveid = &(pClear_Lock_Request->identifier.drive_id[0]);
            i < clearCount;
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
                                      XNAME_clr_drive_lock,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    if (clearCount > 0)
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_drive_list,
                                          NULL,
                                          0);

        pParentXmlelem = pXmlparse->pCurrXmlelem;

        for (i = 0, pDriveid = &(pClear_Lock_Request->identifier.drive_id[0]);
            i < clearCount;
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
/** Function Name: extractClearResponse                              */
/** Description:   Extract the <clr_drive_lock_request> response.    */
/**                                                                  */
/** Parse the response of the XAPI XML <clr_drive_lock>              */
/** request and update the appropriate fields of the                 */
/** ACSAPI CLEAR LOCK DRIVE response.                                */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <clr_drive_lock_request> response consists of:      */
/**==================================================================*/
/** <libreply>                                                       */
/**   <clr_drive_lock_request>                                       */
/**     <header>                                                     */
/**       header element children                                    */
/**     </header>                                                    */
/**     <status>SSS...SSS</status>                                   */
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
/**   </clr_drive_lock_request>                                      */
/** </libreply>                                                      */
/**                                                                  */
/** NOTE: Possible <clr_drive_lock_request><status> values are:      */
/**       "success", "cancelled", "parser error", "process failure", */
/**       "database error", and "no subfile".                        */
/**                                                                  */
/** NOTE: Possible <cdk_drive_status><status> values are:            */
/**       "success", "not in library", and "unknown".                */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI CLEAR LOCK DRIVE response consists of:                */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   CLEAR_LOCK_RESPONSE data consisting of:                     */
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
#define SELF "extractClearResponse"

static int extractClearResponse(struct XAPICVT  *pXapicvt,
                                struct XAPIREQE *pXapireqe,
                                char            *pClearResponse,
                                struct XMLPARSE *pXmlparse)
{
    int                 clearRC;
    int                 headerRC;
    int                 i;
    int                 j;
    int                 clearResponseCount;
    int                 clearRemainingCount;
    int                 clearPacketCount;
    int                 clearResponseSize;
    char                clearStatus[16];
    char                driveLocId[16]; 
    char                cdkStatusStatus[16];

    struct XMLELEM     *pParentXmlelem;
    struct XMLELEM     *pClrDriveXmlelem;
    struct XMLELEM     *pStatusXmlelem;
    struct XMLELEM     *pFirstCdkStatusXmlelem;
    struct XMLELEM     *pNextCdkStatusXmlelem;
    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;
    struct RAWCOMMON   *pRawcommon          = &(pXapicommon->rawcommon);
    struct XAPICFG     *pXapicfg;

    CLEAR_LOCK_RESPONSE *pClear_Lock_Response = 
    (CLEAR_LOCK_RESPONSE*) pClearResponse;
    LO_DRV_STATUS      *pLo_Drv_Status      = &(pClear_Lock_Response->identifier_status.drive_status[0]);

    struct CDKSTATUS    cdkstatus[]         =
    {
        "SUCCESS         ",  STATUS_SUCCESS,
        "CANCELLED       ",  STATUS_CANCELLED,
        "DATABASE ERROR  ",  STATUS_DATABASE_ERROR,
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
    /* Get the <clr_drive_lock_request><status> element.             */
    /*****************************************************************/
    clearRC = STATUS_NI_FAILURE;

    pClrDriveXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                               pXmlparse->pHocXmlelem,
                                               XNAME_clr_drive_lock_request);

    if (pClrDriveXmlelem != NULL)
    {
        pStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                 pClrDriveXmlelem,
                                                 XNAME_status);

        if (pStatusXmlelem != NULL)
        {
            memset(clearStatus, ' ', sizeof(clearStatus));

            memcpy(clearStatus,
                   pStatusXmlelem->pContent,
                   pStatusXmlelem->contentLen);

            for (i = 0;
                i < sizeof(clearStatus);
                i++)
            {
                clearStatus[i] = toupper(clearStatus[i]);
            }

            for (i = 0;
                i < cdkstatusCount;              
                i++)
            {
                if (memcmp(cdkstatus[i].statusString, 
                           clearStatus,
                           sizeof(clearStatus)) == 0)
                {
                    clearRC = cdkstatus[i].statusCode;

                    break;
                }
            }

            TRMSGI(TRCI_XAPI,
                   "[i]=%d, clearRC=%d, clearStatus=%.16s\n",
                   i,
                   clearRC,
                   clearStatus);
        }
    }

    /*****************************************************************/
    /* Count the number of <cdk_drive_status> entries.               */
    /*****************************************************************/
    pFirstCdkStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                     pXmlparse->pHocXmlelem,
                                                     XNAME_cdk_drive_status);

    clearResponseCount = 0;

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
            clearResponseCount++;

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
           "clearResponseCount=%d, MAX_ID=%d\n",
           clearResponseCount,
           MAX_ID);

    /*****************************************************************/
    /* If no <cdk_drive_status> elements found, then return an error.*/
    /*****************************************************************/
    if (clearResponseCount == 0)
    {
        if (headerRC > 0)
        {
            return headerRC;
        }
        else if (clearRC != STATUS_SUCCESS)
        {
            return clearRC;
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
    clearRemainingCount = clearResponseCount;

    while (1)
    {
        if (clearRemainingCount > MAX_ID)
        {
            clearPacketCount = MAX_ID;
        }
        else
        {
            clearPacketCount = clearRemainingCount;
        }

        clearResponseSize = (char*) pLo_Drv_Status -
                            (char*) pClear_Lock_Response +
                            ((sizeof(LO_DRV_STATUS)) * clearPacketCount);

        xapi_lock_init_resp(pXapireqe,
                            (char*) pClearResponse,
                            clearResponseSize);

        pClear_Lock_Response = (CLEAR_LOCK_RESPONSE*) pClearResponse;
        pLo_Drv_Status = &(pClear_Lock_Response->identifier_status.drive_status[0]);

        TRMSGI(TRCI_XAPI,
               "At top of while; clearResponse=%d, "
               "clearRemaining=%d, clearPacket=%d, MAX_ID=%d\n",
               clearResponseCount,
               clearRemainingCount,
               clearPacketCount,
               MAX_ID);

        pClear_Lock_Response->count = clearPacketCount;
        pClear_Lock_Response->message_status.status = (STATUS) clearRC;

        for (i = 0;
            i < clearPacketCount;
            i++, pLo_Drv_Status++)
        {
            if (pNextCdkStatusXmlelem == NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextCdkStatusXmlelem=NULL at clearCount=%i\n",
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

        clearRemainingCount = clearRemainingCount - clearPacketCount;

        if (clearRemainingCount <= 0)
        {
            xapi_fin_response(pXapireqe,
                              pClearResponse,     
                              clearResponseSize); 

            break; 
        }
        else
        {
            xapi_int_response(pXapireqe,
                              pClearResponse,     
                              clearResponseSize);  
        }
    }

    return STATUS_SUCCESS;
}


