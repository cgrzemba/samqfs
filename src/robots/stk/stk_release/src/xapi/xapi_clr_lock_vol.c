/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */ 
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_clr_lock_vol.c                              */
/** Description:    XAPI CLEAR LOCK VOLUME processor.                */
/**                                                                  */
/**                 Remove all current and pending LOCK(s) from      */
/**                 specified volume(s).                             */
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
/** Function Name: xapi_clr_lock_vol                                 */
/** Description:   XAPI CLEAR LOCK VOLUME processor.                 */
/**                                                                  */
/** Remove all current and pending LOCK(s) from specified volume(s). */
/**                                                                  */
/** VOLUME LOCK(s) are managed on the server.                        */
/** The ACSAPI format CLEAR LOCK VOLUME request is translated        */
/** into an XAPI XML format <clr_volume_lock> request; the XAPI XML  */
/** request is then transmitted to the server via TCP/IP;  The       */
/** received XAPI XML response is then translated into one or more   */
/** ACSAPI CLEAR LOCK VOLUME responses.                              */
/**                                                                  */
/** The CLEAR LOCK VOLUME command is NOT allowed to proceed when     */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_clr_lock_vol"

extern int xapi_clr_lock_vol(struct XAPICVT  *pXapicvt,
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
           "Entered; CLEAR LOCK VOLUME request=%08X, size=%d, count=%d, "
           "MAX_ID=%d, CLEAR LOCK VOLUME response=%08X, size=%d\n",
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
    /* Now generate the CLEAR LOCK VOLUME ACSAPI response.           */
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
/** Description:   Build an XAPI <clr_volume_lock> request.          */
/**                                                                  */
/** Convert the ACSAPI format CLEAR LOCK VOLUME request into an      */
/** XAPI XML format <clr_volume_lock> request.                       */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI CLEAR LOCK VOLUME request consists of:                */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   CLEAR LOCK VOLUME data consisting of:                       */
/**      i.   type                                                   */
/**      ii.  count                                                  */
/**      iii. VOLID[count] data entries consisting of:               */
/**           a.   external_label (6 character volser)               */
/**                                                                  */
/** NOTE: The input LOCKID in the MESSAGE_HEADER.lock_id field       */
/**       is irrelevant; all lock IDs will be cleared for the        */
/**       specified volser(s).                                       */
/**                                                                  */
/** NOTE: Specifying a count of 0 will clear ALL volume locks.       */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <clr_volume_lock> request consists of:              */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <clr_volume_lock>                                            */
/**       <volume_list>                                              */
/**         <volser>vvvvvv</volser>                                  */
/**         ...repeated <volser> entries                             */
/**       </volume_list>                                             */
/**     </clr_volume_lock>                                           */
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
    VOLID              *pVolid              = &(pClear_Lock_Request->identifier.vol_id[0]);

    int                 clearCount          = pClear_Lock_Request->count;
    struct RAWVOLUME    rawvolume;
    struct RAWVOLUME   *pRawvolume;

    char                volserString[7];

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
                                      XNAME_clr_volume_lock,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    /*****************************************************************/
    /* If any of the specified volumes is not found, then            */
    /* we still process the request for all specified volumes.       */
    /* In such cases, the response will contain both found and       */
    /* not founnd volumes.                                           */
    /*****************************************************************/
    if (clearCount > 0)
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_volume_list,
                                          NULL,
                                          0);

        pParentXmlelem = pXmlparse->pCurrXmlelem;

        for (i = 0, pVolid = &(pClear_Lock_Request->identifier.vol_id[0]);
            i < clearCount;
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
/** Function Name: extractClearResponse                              */
/** Description:   Extract the <clr_volume_lock_request> response.   */
/**                                                                  */
/** Parse the response of the XAPI XML <clr_volume_lock>             */
/** request and update the appropriate fields of the                 */
/** ACSAPI CLEAR LOCK VOLUME response.                               */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <clr_volume_lock_request> response consists of:     */
/**==================================================================*/
/** <libreply>                                                       */
/**   <clr_volume_lock_request>                                      */
/**     <header>                                                     */
/**       header element children                                    */
/**     </header>                                                    */
/**     <status>SSS...SSS</status>                                   */
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
/**   </clr_volume_lock_request>                                     */
/** </libreply>                                                      */
/**                                                                  */
/** NOTE: Possible <clr_volume_lock_request><status> values are:     */
/**       "success", "cancelled", "parser error", "process failure", */
/**       "database error", and "no subfile".                        */
/**                                                                  */
/** NOTE: Possible <cdk_volume_status><status> values are:           */
/**       "success", "not in library", and "unknown".                */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI CLEAR LOCK VOLUME response consists of:               */
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
    char                volser[6]; 
    char                cdkStatusStatus[16];

    struct XMLELEM     *pParentXmlelem;
    struct XMLELEM     *pClrVolumeXmlelem;
    struct XMLELEM     *pStatusXmlelem;
    struct XMLELEM     *pFirstCdkStatusXmlelem;
    struct XMLELEM     *pNextCdkStatusXmlelem;
    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;
    struct RAWCOMMON   *pRawcommon          = &(pXapicommon->rawcommon);

    CLEAR_LOCK_RESPONSE *pClear_Lock_Response = 
    (CLEAR_LOCK_RESPONSE*) pClearResponse;
    LO_VOL_STATUS      *pLo_Vol_Status      = &(pClear_Lock_Response->identifier_status.volume_status[0]);

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
    /* Get the <clr_volume_lock_request><status> element.            */
    /*****************************************************************/
    clearRC = STATUS_NI_FAILURE;

    pClrVolumeXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                pXmlparse->pHocXmlelem,
                                                XNAME_clr_volume_lock_request);

    if (pClrVolumeXmlelem != NULL)
    {
        pStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                 pClrVolumeXmlelem,
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
    /* Count the number of <cdk_volume_status> entries.               */
    /*****************************************************************/
    pFirstCdkStatusXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                     pXmlparse->pHocXmlelem,
                                                     XNAME_cdk_volume_status);

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
           "clearResponseCount=%d, MAX_ID=%d\n",
           clearResponseCount,
           MAX_ID);

    /*****************************************************************/
    /* If no <cdk_volume_status> elements found, then return an      */
    /* error.                                                        */
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
    /* If <cdk_volume_status> elements found, then extract the data. */
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

        clearResponseSize = (char*) pLo_Vol_Status -
                            (char*) pClear_Lock_Response +
                            ((sizeof(LO_VOL_STATUS)) * clearPacketCount);

        xapi_lock_init_resp(pXapireqe,
                            (char*) pClearResponse,
                            clearResponseSize);

        pClear_Lock_Response = (CLEAR_LOCK_RESPONSE*) pClearResponse;
        pLo_Vol_Status = &(pClear_Lock_Response->identifier_status.volume_status[0]);

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
            i++, pLo_Vol_Status++)
        {
            if (pNextCdkStatusXmlelem == NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextCdkStatusXmlelem=NULL at clearCount=%i\n",
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


