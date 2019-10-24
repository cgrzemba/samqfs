/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_set_scr.c                                   */
/** Description:    XAPI SET SCRATCH (and SET UNSCRACTH) processor.  */
/**                                                                  */
/**                 SCRATCH or UNSCRATCH the by volume range(s).     */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     07/05/11                          */
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
#define SET_SEND_TIMEOUT           5   /* TIMEOUT values in seconds  */       
#define SET_RECV_TIMEOUT_1ST       300
#define SET_RECV_TIMEOUT_NON1ST    600
#define SETSCRERR_RESPONSE_SIZE (offsetof(SET_SCRATCH_RESPONSE, scratch_status[0])) 


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void initSetscrResponse(struct XAPIREQE *pXapireqe,
                               char            *pSetscrResponse,
                               int              setscrResponseSize);

static int scratchVolsers(struct XAPICVT       *pXapicvt,
                          struct XAPIREQE      *pXapireqe,
                          SET_SCRATCH_RESPONSE *pSet_Scratch_Response,
                          char                  finalResponseFlag,
                          char                  unscratchFlag,
                          int                   volserCount,
                          char                  volserBatch[MAX_ID] [6]);

static int buildScratchRequest(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char             unscratchFlag,
                               int              volserCount,
                               char             volserBatch[MAX_ID] [6],
                               char           **ptrXapiBuffer,
                               int             *pXapiBufferSize);

static int extractScratchResponse(struct XAPICVT       *pXapicvt,
                                  struct XAPIREQE      *pXapireqe,
                                  struct XMLPARSE      *pXmlparse,
                                  SET_SCRATCH_RESPONSE *pSet_Scratch_Response,
                                  char                  finalResponseFlag,
                                  char                  unscratchFlag,
                                  int                   volserCount,
                                  char                  volserBatch[MAX_ID] [6]);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_set_scr                                      */
/** Description:   XAPI SET SCRATCH (and SET UNSCRACTH) processor.   */
/**                                                                  */
/** SCRATCH or UNSCRATCH the specified volume range(s).              */
/**                                                                  */
/** If the MESSAGE_HEADER.extended_options RESET flag is set, then   */
/** UNSCRATCH, otherwise, SCRATCH.                                   */
/**                                                                  */
/** If the MESSAGE_HEADER.lock_id is NOT 0 (NO_LOCK_ID), then        */
/** validate that all of the specified volume(s) in the specified    */
/** ranges are either locked by the specified LOCKID,                */
/** or that they are unlocked altogether.                            */
/**                                                                  */
/** The ACSAPI format SET SCRATCH request is translated into an      */
/** XAPI XML format <scratch_vol> or <unscratch> request;            */
/** the XAPI XML request is then transmitted to the server via       */
/** TCP/IP;  The received XAPI XML response is then translated       */
/** into the ACSAPI SET SCRATCH response.                            */
/**                                                                  */
/** The DISMOUNT command is NOT allowed to proceed when the          */
/** XAPI client is in the IDLE state.                                */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI SET SCRATCH request consists of:                      */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER which includes the                      */
/**           a.   LOCKID                                            */
/** 2.   SET_SCRATCH_REQUEST data consisting of:                     */
/**      i.   POOLID (may be specified as SAME_POOL)                 */
/**           Identifies the desired pool to assign or unassign      */
/**           the volser.  This parameter is ignored by the XAPI     */
/**           bacause pool volsers cannot be dynamically moved       */
/**           from one scratch pool to another.                      */
/**      ii.  count (of volser ranges)                               */
/**      iii. VOLRANGE[count] data entries consisting of             */
/**           a.    VOLID startvol                                   */
/**           b.    VOLID endvol                                     */
/**                                                                  */
/** NOTE: MAX_ID is 42.  Which is 42 ranges (a range may be a single */
/** volser or multiple volsers (without any specified limit).        */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_set_scr"

extern int xapi_set_scr(struct XAPICVT  *pXapicvt,
                        struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    int                 i;
    int                 j;

    SET_SCRATCH_REQUEST *pSet_Scratch_Request = 
    (SET_SCRATCH_REQUEST*) pXapireqe->pAcsapiBuffer;
    MESSAGE_HEADER     *pMessage_Header     = 
    &(pSet_Scratch_Request->request_header.message_header);

    int                 volrangeCount       = pSet_Scratch_Request->count;
    int                 lockId              = (int) pMessage_Header->lock_id;
    int                 lockRC;

    char                firstVol[6];
    char                lastVol[6];
    char                unscratchFlag       = FALSE;
    char                finalResponseFlag   = FALSE;
    char                volserBatch[MAX_ID] [6];

    struct XAPIVRANGE   xapivrange[MAX_ID];
    struct XAPIVRANGE  *pXapivrange         = &(xapivrange[0]);
    struct RAWQLOCK     rawqlock;
    struct RAWQLOCK    *pRawqlock           = &rawqlock;

    SET_SCRATCH_RESPONSE  wkSet_Scratch_Response;
    SET_SCRATCH_RESPONSE *pSet_Scratch_Response = &wkSet_Scratch_Response;

    /*****************************************************************/
    /* NOTE: The ACSAPI SET SCRATCH request both scratches and       */
    /* unscratches volumes.  The determination is in the             */
    /* MESSAGE_HEADER.extended_options RESET flag.                   */
    /*                                                               */
    /* If MESSAGE_HEADER.extended_options RESET flag is set, then    */
    /* ACSAPI SET SCRATCH will unscratch volumes;                    */
    /* Otherwise, ACSAPI SET SCRATCH will scratch volumes.           */
    /*****************************************************************/
    if (pMessage_Header->extended_options & RESET)
    {
        unscratchFlag = TRUE;
    }
    else
    {
        unscratchFlag = FALSE;
    }

    initSetscrResponse(pXapireqe,
                       (char*) pSet_Scratch_Response,
                       sizeof(SET_SCRATCH_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; SET SCRATCH request=%08X, size=%d, count=%d, "
           "unscratchFlag=%d, lockId=%d, SET SCRATCH response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           volrangeCount,
           unscratchFlag,
           lockId,
           pSet_Scratch_Response,
           sizeof(SET_SCRATCH_RESPONSE));

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pSet_Scratch_Response,
                            SETSCRERR_RESPONSE_SIZE);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    /*****************************************************************/
    /* NOTE: The ACSAPI SET SCRATCH request only allows volser       */
    /* ranges to be specified.   However, the volser range may be    */
    /* for a single volser (i.e. TAP001-TAP001).                     */
    /*****************************************************************/
    if (volrangeCount == 0)
    {
        xapi_err_response(pXapireqe,
                          (char*) pSet_Scratch_Response,
                          SETSCRERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_SMALL); 

        return STATUS_COUNT_TOO_SMALL;
    }

    if (volrangeCount > MAX_ID)
    {
        xapi_err_response(pXapireqe,
                          (char*) pSet_Scratch_Response,
                          SETSCRERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_LARGE); 

        return STATUS_COUNT_TOO_LARGE;
    }

    /*****************************************************************/
    /* Build the XAPIVRANGE block for each volser range in the       */
    /* request.  This also validates each volser range before we     */
    /* begin processing.                                             */
    /*****************************************************************/
    memset((char*) &xapivrange, 0, sizeof(xapivrange));

    for (i = 0, pXapivrange = &(xapivrange[0]);
        i < volrangeCount;
        i++, pXapivrange++)
    {
        memcpy(pXapivrange->firstVol,
               pSet_Scratch_Request->vol_range[i].startvol.external_label,
               sizeof(firstVol));

        memcpy(pXapivrange->lastVol,
               pSet_Scratch_Request->vol_range[i].endvol.external_label,
               sizeof(firstVol));

        lastRC = xapi_validate_volser_range(pXapivrange);

        if (lastRC != STATUS_SUCCESS)
        {
            xapi_err_response(pXapireqe,
                              (char*) pSet_Scratch_Response,
                              SETSCRERR_RESPONSE_SIZE,
                              lastRC); 

            return lastRC;
        }

        /*************************************************************/
        /* For scratch/unscratch:                                    */
        /* Only check volume lock_id(s) if specified.                */
        /*                                                           */
        /* Process each volser range:                                */
        /* If the volume is locked by the specified lock_id, or if   */
        /* the volume is unlocked altogether, or if the volume is    */
        /* not found, then it is acceptable for scratch/unscratch.   */
        /*                                                           */
        /* If no NO_LOCK_ID specified, then we do not really care    */
        /* if the volume(s) are locked!                              */
        /*************************************************************/
        if (lockId != NO_LOCK_ID)
        {
            while (1)
            {
                xapi_volser_increment(pXapivrange);    

                /*****************************************************/
                /* At the end of the range, reset the XAPIVRANGE     */
                /* for the next increment loop below.                */
                /*****************************************************/
                if (pXapivrange->rangeCompletedFlag)
                {
                    pXapivrange->rangeCompletedFlag = FALSE;
                    memset(pXapivrange->currVol, 0, sizeof(pXapivrange->currVol));

                    break;
                }

                memset((char*) pRawqlock,
                       0,
                       sizeof(struct RAWQLOCK));

                lastRC = xapi_qlock_vol_one(pXapicvt,
                                            pXapireqe,
                                            pRawqlock,
                                            pXapivrange->currVol,
                                            lockId);

                TRMEMI(TRCI_XAPI, pRawqlock, sizeof(struct RAWQLOCK),
                       "SET SCR VOLUME lock status=%d; RAWQLOCK:\n",
                       pRawqlock->queryRC);

                if (pRawqlock->queryRC != STATUS_SUCCESS)
                {
                    if ((pRawqlock->resRC != STATUS_VOLUME_AVAILABLE) &&
                        (pRawqlock->resRC != STATUS_VOLUME_NOT_IN_LIBRARY))
                    {
                        if (pRawqlock->resRC != STATUS_SUCCESS)
                        {
                            lockRC = pRawqlock->resRC;
                        }
                        else
                        {
                            lockRC = STATUS_VOLUME_IN_USE;
                        }

                        xapi_err_response(pXapireqe,
                                          (char*) pSet_Scratch_Response,
                                          SETSCRERR_RESPONSE_SIZE,
                                          lockRC); 

                        return lockRC;
                    }
                }
            }
        }
    }

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

    /*****************************************************************/
    /* Process each volser range (again):                            */
    /* We will build XAPI scratch or unscratch batches for           */
    /* MAX_ID volumes at a time.  Each XAPI request may be a subset  */
    /* of a single volser range, or comprise multiple partial or     */
    /* complete volser ranges.                                       */
    /*****************************************************************/
    memset(volserBatch, 0, sizeof(volserBatch));

    for (i = 0, j = 0, pXapivrange = &(xapivrange[0]);
        i < volrangeCount;
        i++, pXapivrange++)
    {
        TRMSGI(TRCI_XAPI, 
               "Next XAPVRANGE=%.6s-%.6s\n",
               pXapivrange->firstVol,
               pXapivrange->lastVol);

        while (1)
        {
            xapi_volser_increment(pXapivrange);    

            if (pXapivrange->rangeCompletedFlag)
            {
                break;
            }

            memcpy(volserBatch[j],
                   pXapivrange->currVol,
                   sizeof(pXapivrange->currVol));

            j++;

            if (j >= MAX_ID)
            {
                TRMEMI(TRCI_XAPI, volserBatch, (j * 6),
                       "j=%d; volserBatch:\n",
                       j);

                /*****************************************************/
                /* Determine if this full response is really the     */
                /* final response                                    */
                /*****************************************************/
                if (((i + 1) >= volrangeCount) &&
                    (memcmp(pXapivrange->currVol,
                            pXapivrange->lastVol,
                            sizeof(pXapivrange->currVol)) == 0))
                {
                    TRMSGI(TRCI_XAPI, 
                           "Setting finalResponseFlag for MAX_ID response\n");

                    finalResponseFlag = TRUE;
                }
                else
                {
                    finalResponseFlag = FALSE;
                }

                lastRC = scratchVolsers(pXapicvt,
                                        pXapireqe,
                                        pSet_Scratch_Response,
                                        finalResponseFlag,
                                        unscratchFlag,
                                        j,
                                        volserBatch);

                if (lastRC != STATUS_SUCCESS)
                {
                    return lastRC;
                }

                initSetscrResponse(pXapireqe,
                                   (char*) pSet_Scratch_Response,
                                   sizeof(SET_SCRATCH_RESPONSE));

                memset(volserBatch, 0, sizeof(volserBatch));

                j = 0;
            }
        }
    }

    /*****************************************************************/
    /* Process the last batch of volsers.                            */
    /*****************************************************************/
    if (j > 0)
    {
        TRMEMI(TRCI_XAPI, volserBatch, (j * 6),
               "j=%d; volserBatch:\n",
               j);

        finalResponseFlag = TRUE;

        lastRC = scratchVolsers(pXapicvt,
                                pXapireqe,
                                pSet_Scratch_Response,
                                finalResponseFlag,
                                unscratchFlag,
                                j,
                                volserBatch);

        if (lastRC != STATUS_SUCCESS)
        {
            return lastRC;
        }
    }

    return STATUS_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: initSetscrResponse                                */
/** Description:   Initialize the ACSAPI SET SCRATCH response.       */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI SET SCRATCH response consists of:                     */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   SET_SCRATCH_RESPONSE data consisting of:                    */
/**      i.   status                                                 */
/**      ii.  POOL (which will always be SAME_POOL for XAPI          */
/**           processed requests as the POOL cannot be changed)      */
/**      iii. count (of scratched/unscratched volumes)               */
/**      iv.  VOLID[count] data entries consisting of                */
/**           a.   external_label (6 character volser)               */
/**      v.   RESPONSE_STATUS[count] data entries consisting of:     */
/**           a.   status                                            */
/**           b.   type                                              */
/**           c.   identifier                                        */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "initSetscrResponse"

static void initSetscrResponse(struct XAPIREQE *pXapireqe,
                               char            *pSetscrResponse,
                               int              setscrResponseSize)
{
    SET_SCRATCH_REQUEST    *pSet_Scratch_Request  = 
    (SET_SCRATCH_REQUEST*) pXapireqe->pAcsapiBuffer;
    SET_SCRATCH_RESPONSE   *pSet_Scratch_Response = 
    (SET_SCRATCH_RESPONSE*) pSetscrResponse;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Initialize QUERY VOLUME response.                             */
    /*****************************************************************/
    memset((char*) pSet_Scratch_Response, 0, setscrResponseSize);

    memcpy((char*) &(pSet_Scratch_Response->request_header),
           (char*) &(pSet_Scratch_Request->request_header),
           sizeof(REQUEST_HEADER));

    pSet_Scratch_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pSet_Scratch_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pSet_Scratch_Response->message_status.status = STATUS_SUCCESS;
    pSet_Scratch_Response->message_status.type = TYPE_NONE;

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: scratchVolsers                                    */
/** Description:   SCRATCH or UNSCRATCH the batch of input volser(s).*/
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "scratchVolsers"

static int scratchVolsers(struct XAPICVT       *pXapicvt,
                          struct XAPIREQE      *pXapireqe,
                          SET_SCRATCH_RESPONSE *pSet_Scratch_Response,
                          char                  finalResponseFlag,
                          char                  unscratchFlag,
                          int                   volserCount,
                          char                  volserBatch[MAX_ID] [6])
{
    int                 setRC               = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    TRMSGI(TRCI_XAPI, 
           "Entered; unscratchFlag=%d, volserCount=%d, "
           "finalResponseFlag=%d\n",
           unscratchFlag,
           volserCount,
           finalResponseFlag);

    lastRC = buildScratchRequest(pXapicvt,
                                 pXapireqe,
                                 unscratchFlag,
                                 volserCount,
                                 volserBatch,
                                 &pXapiBuffer,
                                 &xapiBufferSize);

    TRMSGI(TRCI_XAPI, 
           "lastRC=%d from buildScratchRequest(pBuffer=%08X, size=%d)\n",
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
                      SET_SEND_TIMEOUT,       
                      SET_RECV_TIMEOUT_1ST,   
                      SET_RECV_TIMEOUT_NON1ST,
                      (void**) &pXmlparse);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from xapi_tcp(pBuffer=%08X, size=%d, pXmlparse=%08X)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize,
           pXmlparse);

    if (lastRC != STATUS_SUCCESS)
    {
        setRC = lastRC;
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
    /* Now extract the volume data.                                  */
    /*****************************************************************/
    if (setRC == STATUS_SUCCESS)
    {
        lastRC = extractScratchResponse(pXapicvt,
                                        pXapireqe,
                                        pXmlparse,
                                        pSet_Scratch_Response,
                                        finalResponseFlag,
                                        unscratchFlag,
                                        volserCount,
                                        volserBatch);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractScratchResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            setRC = lastRC;
        }
    }

    if (setRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pSet_Scratch_Response,
                          SETSCRERR_RESPONSE_SIZE,
                          setRC);
    }

    /*****************************************************************/
    /* When done, free the XMLPARSE and associated structures.       */
    /*****************************************************************/
    if (pXmlparse != NULL)
    {
        FN_FREE_HIERARCHY_STORAGE(pXmlparse);
    }

    return setRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: buildScratchRequest                               */
/** Description:   Build XAPI <scratch_vol> or <unscratch> request.  */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <scratch_vol> or <unscratch> request consists of:   */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <scratch_vol> or <unscratch>                                 */
/**       <volume_list>                                              */
/**         <volser>vvvvvv</volser>                                  */
/**         ...up to MAX_ID volser(s) per batch                      */
/**       </volume_list>                                             */
/**     </scratch_vol> or </unscratch>                               */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** NOTES:                                                           */
/** (1) The xapi_hostname and unix userid specifications             */
/**     are not included and will not be used by the HSC server to   */
/**     control access to specific volsers (which is allowed using   */
/**     the HSC VOLPARMS feature).                                   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "buildScratchRequest"

static int buildScratchRequest(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char             unscratchFlag,
                               int              volserCount,
                               char             volserBatch[MAX_ID] [6],
                               char           **ptrXapiBuffer,
                               int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 i;
    int                 xapiRequestSize     = 0;

    char               *pXapiRequest        = NULL;

    struct XMLPARSE    *pXmlparse; 
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    TRMSGI(TRCI_XAPI,
           "Entered; unscratchFlag=%d, volserCount=%d\n",
           unscratchFlag,
           volserCount);

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

    if (unscratchFlag)
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_unscratch,
                                          NULL,
                                          0);
    }
    else
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_scratch_vol,
                                          NULL,
                                          0);
    }

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_volume_list,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    for (i = 0;
        i < volserCount;
        i++)
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_volser,
                                          volserBatch[i],
                                          6);
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
/** Function Name: extractScratchResponse                            */
/** Description:   Extract the <scratch_vol> or <unscratch> response.*/
/**                                                                  */
/** Parse the response of the XAPI XML <scratch_vol> or              */
/** <unscratch> request and update the appropriate fields of         */
/** the ACSAPI SET SCRATCH response.                                 */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <scratch_vol> or <unscratch> responses consists of: */
/**==================================================================*/
/** <libreply>                                                       */
/**   <scratch_request> or <unscratch_request>                       */
/**     <header>                                                     */
/**       header element children                                    */
/**     </header>                                                    */
/**     <volume_data>                                                */
/**       <volser>vvvvvv</volser>                                    */
/**       <result>SUCCESS|FAILURE</result>                           */
/**       <error>xxxxxxxx</error>                                    */
/**       <reason>ccc...ccc</reason>                                 */
/**     </volume_data>                                               */
/**     ...repeated <volume_data> entries                            */
/**     </exceptions>                                                */
/**     <uui_return_code>nnnn</uui_return_code>                      */
/**     <uui_reason_code>nnnn</uui_reason_code>                      */
/**   </scratch_request> or </unscratch_request>                     */
/** </libreply>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractScratchResponse"

static int extractScratchResponse(struct XAPICVT       *pXapicvt,
                                  struct XAPIREQE      *pXapireqe,
                                  struct XMLPARSE      *pXmlparse,
                                  SET_SCRATCH_RESPONSE *pSet_Scratch_Response,
                                  char                  finalResponseFlag,
                                  char                  unscratchFlag,
                                  int                   volserCount,
                                  char                  volserBatch[MAX_ID] [6])
{
    int                 lastRC;
    int                 responseVolserCount = 0;
    int                 setScrResponseSize;
    int                 i;
    int                 j;

    struct XMLELEM     *pVolumeDataXmlelem;
    struct XMLELEM     *pParentXmlelem;

    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;
    struct RAWSCRATCH   rawscratch;
    struct RAWSCRATCH  *pRawscratch         = &rawscratch;

    struct XMLSTRUCT    volumeDataXmlstruct[] =
    {
        XNAME_volume_data,                  XNAME_volser,
        sizeof(pRawscratch->volser),        pRawscratch->volser,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_result,
        sizeof(pRawscratch->result),        pRawscratch->result,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_error,
        sizeof(pRawscratch->error),         pRawscratch->error,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_reason,
        sizeof(pRawscratch->reason),        pRawscratch->reason,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 volumeDataElementCount = sizeof(volumeDataXmlstruct) / 
                                                 sizeof(struct XMLSTRUCT);

    TRMSGI(TRCI_XAPI, 
           "Entered; volserCount=%d, finalResponseFlag=%d\n",
           volserCount,
           finalResponseFlag);

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

    pVolumeDataXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                 pXmlparse->pHocXmlelem,
                                                 XNAME_volume_data);

    TRMSGI(TRCI_XAPI, 
           "pVolumeDataXmlelem=%08X\n",
           pVolumeDataXmlelem);

    if (pVolumeDataXmlelem == NULL)
    {
        return STATUS_NI_FAILURE;
    }

    pParentXmlelem = pVolumeDataXmlelem->pParentXmlelem;
    pSet_Scratch_Response->pool_id.pool = (POOL) SAME_POOL;

    for (i = 0;
        i < volserCount;
        i++)
    {
        memset(pRawscratch, 0, sizeof(struct RAWSCRATCH));

        FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                          pVolumeDataXmlelem,
                                          &volumeDataXmlstruct[0],
                                          volumeDataElementCount);

        TRMSGI(TRCI_XAPI, 
               "RAWSCRATCH.volser[%i]=%.6s, result=%c, "
               "input volser[%i]=%.6s\n",
               i,
               pRawscratch->volser,
               pRawscratch->result[0], 
               i,
               volserBatch[i]);

        responseVolserCount++;

        memcpy(pSet_Scratch_Response->scratch_status[i].vol_id.external_label,
               pRawscratch->volser,
               sizeof(pRawscratch->volser));

        pSet_Scratch_Response->scratch_status[i].status.status = 
        (STATUS) STATUS_SUCCESS;

        pSet_Scratch_Response->scratch_status[i].status.type = 
        (STATUS) TYPE_NONE;

        /*************************************************************/
        /* If the FAILURE is really a warning (i.e. <error> is       */
        /* "00044008" indicating volume is already scratch, or       */
        /* "00044030" indicating volume is not a scratch, then       */
        /* treat as ACSAPI success.                                  */
        /*                                                           */
        /* If the FAILURE is "00044001" or "0004700C", then it is    */
        /* volume not found.                                         */
        /*                                                           */
        /* Otherwise, treat as volume access denied.                 */
        /*************************************************************/
        if (pRawscratch->result[0] == 'F')
        {
            if ((memcmp(pRawscratch->error,
                        "00044008",
                        8) == 0) ||
                (memcmp(pRawscratch->error,
                        "00044030",
                        8) == 0))
            {
                DONOTHING;
            }
            else if ((memcmp(pRawscratch->error,
                             "00044001",
                             8) == 0) ||
                     (memcmp(pRawscratch->error,
                             "0004700C",
                             8) == 0))
            {
                pSet_Scratch_Response->scratch_status[i].status.status = 
                (STATUS) STATUS_VOLUME_NOT_IN_LIBRARY;
            }
            else
            {
                pSet_Scratch_Response->scratch_status[i].status.status = 
                (STATUS) STATUS_VOLUME_ACCESS_DENIED;
            }
        }

        pVolumeDataXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                          pParentXmlelem,
                                                          pVolumeDataXmlelem,
                                                          XNAME_volume_data);

        if (pVolumeDataXmlelem == NULL)
        {
            TRMSGI(TRCI_XAPI, 
                   "No more volumes after RAWSCRATCH=%d; "
                   "volserCount=%d\n",
                   responseVolserCount,
                   volserCount);

            break;
        }
    }

    pSet_Scratch_Response->count = responseVolserCount;
    setScrResponseSize =   
    ((uintptr_t) (&pSet_Scratch_Response->scratch_status[responseVolserCount]) 
    - ((uintptr_t) pSet_Scratch_Response));

    if (finalResponseFlag)
    {
        xapi_fin_response(pXapireqe,
                          (char*) pSet_Scratch_Response,     
                          setScrResponseSize);  
    }
    else
    {
        xapi_int_response(pXapireqe,
                          (char*) pSet_Scratch_Response,     
                          setScrResponseSize);  
    }

    return STATUS_SUCCESS;
}



