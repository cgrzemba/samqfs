/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_scrpool.c                                   */
/** Description:    XAPI scratch subpool table name update service.  */
/**                                                                  */
/**                 Update the XAPI client scratch subpool table     */
/**                 (XAPISCRPOOL) name and index fields.             */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     07/15/11                          */
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
#define QUERY_RECV_TIMEOUT_NON1ST  900


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void buildQScrpoolRequest(struct XAPICVT   *pXapicvt,
                                 struct XAPIREQE  *pXapireqe,
                                 char            **ptrXapiBuffer,
                                 int              *pXapiBufferSize);

static int extractQScrpoolResponse(struct XAPICVT     *pXapicvt,
                                   struct XAPIREQE    *pXapireqe,
                                   struct XMLPARSE    *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_scrpool                                      */
/** Description:   XAPI scratch subpool table name update service.   */
/**                                                                  */
/** Update the XAPI client scratch subpool table (XAPISCRPOOL) name  */
/** and index fields.                                                */
/**                                                                  */
/** The XAPISCRPOOL is a fixed length table embedded within the      */
/** XAPICVT.  It is used to translate XAPI scratch subpool names     */
/** into ACSAPI subpool identifiers (and vice-versa).                */
/** ACSAPI subpool identifiers are the same value as an XAPI         */
/** subpool index.                                                   */
/**                                                                  */
/** The XAPISCRPOOL has 256 entries; Entry (or subpool index) = 0    */
/** is for the default subpool; Entries 1-255 are for the named      */
/** scratch subpools.                                                */
/**                                                                  */
/** There are 3 parts to the XAPISCRPOOL update.                     */
/** (1) xapi_scrpool issues the XAPI <query_scrpool_info> command    */
/**     and updates the XAPISCRPOOL.subpoolNameString and            */
/**     subpoolIndex.                                                */
/** (2) xapi_scrpool_counts issues the XAPI <query_threshold>        */
/**     command and updates the XAPISCRPOOL.threshold and            */
/**     scratchCount.  At this point the volumeCount is also         */
/**     updated with the same value as the scratchCount.             */
/**     This is only a termporary cludge as we do not know how       */
/**     many actual volumes are in the subpool.                      */
/** (3) xapi.scrpool_counts issues the XAPI VOLRPT SUMMARY(SUBPOOL)  */
/**     command to update the actual volume count for each subpool.  */
/**     The VOLRPT SUMMARY(SUBPOOL) request may be requested         */
/**     as a synchronous or asynchronous command (executed in its    */
/**     own thread).  The VOLRPT SUMMARY(SUBPOOL) is always          */
/**     requested as an asynchronous process at startup.             */
/**                                                                  */
/** The XAPI scratch subpool table name update service is allowed    */
/** to proceed even when the XAPI client is in the IDLE state.       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_scrpool"

extern int xapi_scrpool(struct XAPICVT  *pXapicvt,
                        struct XAPIREQE *pXapireqe,
                        char             processFlag)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;

    struct XMLPARSE    *pXmlparse           = NULL;

    TRMSGI(TRCI_XAPI,
           "Entered; processFlag=%d\n",
           processFlag);

    buildQScrpoolRequest(pXapicvt,
                         pXapireqe,
                         &pXapiBuffer,
                         &xapiBufferSize);

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

    if (queryRC == STATUS_SUCCESS)
    {
        lastRC = extractQScrpoolResponse(pXapicvt,
                                         pXapireqe,
                                         pXmlparse);

        TRMSGI(TRCI_XAPI,"lastRC=%d from extractQScrpoolResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            queryRC = STATUS_PROCESS_FAILURE;
        }
        else
        {
            queryRC = xapi_scrpool_counts(pXapicvt,
                                          pXapireqe,
                                          processFlag);

            TRMSGI(TRCI_XAPI,"queryRC=%d from xapi_scrpool_counts\n",
                   queryRC);

            if (queryRC == STATUS_SUCCESS)
            {
                pXapicvt->updateScrpool = FALSE;
                time(&(pXapicvt->scrpoolTime));
            }
        }
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
/** Function Name: buildQScrpoolRequest                              */
/** Description:   Build the XAPI <query_scrpool_info> request.      */
/**                                                                  */
/** Build the XAPI <query_scrpool_info> request to return all        */
/** scratch subpools accessibled from this host.                     */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_scrpool_info> request consists of:           */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**                                                                  */
/**   <command>                                                      */
/**     <query_scrpool_info>                                         */
/**     </query_scrpool_info>                                        */
/**   </command>                                                     */
/** </libtrans>                                                      */
/*                                                                   */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "buildQScrpoolRequest"

static void buildQScrpoolRequest(struct XAPICVT  *pXapicvt,
                                 struct XAPIREQE *pXapireqe,
                                 char           **ptrXapiBuffer,
                                 int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;

    char               *pXapiRequest        = NULL;
    struct XMLPARSE    *pXmlparse; 
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

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
                                      XNAME_query_scrpool_info,
                                      NULL,
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

    return;                                                     
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: extractQScrpoolResponse                           */
/** Description:   Extract the <scrpool_request_info> response.      */
/**                                                                  */
/** Parse the response of the XAPI XML <query_scrpool_info> request  */
/** and update the subpool name and index fields of the              */
/** XAPI client scratch subpool table (XAPISCRPOOL).                 */
/**                                                                  */
/** NOTE: At this point we are updating a working copy of the        */
/** XAPISCRPOOL table.  When we have completed the working copy      */
/** update, we will update the "real" version of the XAPISCRPOOL     */
/** table all at once.                                               */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <scrpool_request_info> responses consists of:       */
/**==================================================================*/
/** <libreply>                                                       */
/**   <scrpool_request_info>                                         */
/**     <scratch_pool_info>                                          */
/**       <subpool_name>SSSSSSSSSSSSS</subpool_name>                 */
/**       <subpool_index>NNN</subpool_index>                         */
/**     </scratch_pool_info>                                         */
/**     ...repeated <scratch_pool_info> entries                      */
/**   </scrpool_request_info>                                        */
/** </libreply>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractQScrpoolResponse"

static int extractQScrpoolResponse(struct XAPICVT     *pXapicvt,
                                   struct XAPIREQE    *pXapireqe,
                                   struct XMLPARSE    *pXmlparse)
{
    int                 lastRC;
    int                 i;
    int                 poolCount           = 0;
    int                 wkInt;

    struct XAPISCRPOOL  wkXapiscrpool[MAX_XAPISCRPOOL];
    struct XAPISCRPOOL *pWkXapiscrpool;
    struct XAPISCRPOOL *pXapiscrpool;
    struct XMLELEM     *pPoolInfoXmlelem;
    struct XMLELEM     *pPoolInfoParentXmlelem;
    struct XMLELEM     *pSubpoolNameXmlelem;
    struct XMLELEM     *pSubpoolIndexXmlelem;

    char                subpoolNameString[XAPI_SUBPOOL_NAME_SIZE + 1];
    char                subpoolIndexString[4];

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Create a working copy of the XAPISCRPOOL table.               */
    /*****************************************************************/
    memset(wkXapiscrpool, 0, sizeof(wkXapiscrpool));

    /*****************************************************************/
    /* Run through the <scratch_pool_info> entries.                  */
    /*****************************************************************/
    pPoolInfoXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                               pXmlparse->pHocXmlelem,
                                               XNAME_scratch_pool_info);

    poolCount = 0;

    if (pPoolInfoXmlelem != NULL)
    {
        pPoolInfoParentXmlelem = pPoolInfoXmlelem->pParentXmlelem;

        while (pPoolInfoXmlelem != NULL)
        {
            /*********************************************************/
            /* Extract the <subpool_name> and <subpool_index>        */
            /*********************************************************/
            pSubpoolNameXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                          pPoolInfoXmlelem,
                                                          XNAME_subpool_name);

            pSubpoolIndexXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                           pPoolInfoXmlelem,
                                                           XNAME_subpool_index);

            if ((pSubpoolNameXmlelem != NULL) &&
                (pSubpoolIndexXmlelem != NULL))
            {
                memset(subpoolNameString, 0, sizeof(subpoolNameString));

                memcpy(subpoolNameString,
                       pSubpoolNameXmlelem->pContent,
                       pSubpoolNameXmlelem->contentLen);

                memset(subpoolIndexString, 0, sizeof(subpoolIndexString));

                memcpy(subpoolIndexString,
                       pSubpoolIndexXmlelem->pContent,
                       pSubpoolIndexXmlelem->contentLen);

                wkInt = 0;
                wkInt = atoi(subpoolIndexString);

                if ((wkInt > 0) && (wkInt < 256))
                {
                    poolCount++;
                    pWkXapiscrpool = &(wkXapiscrpool[wkInt]);
                    pWkXapiscrpool->subpoolIndex = wkInt;
                    strcpy(pWkXapiscrpool->subpoolNameString, subpoolNameString);
                }
                else
                {
                    TRMSGI(TRCI_XAPI,
                           "Invalid subpool_index value=%d for "
                           "subpool_name=%s; entry bypassed\n",
                           wkInt,
                           subpoolNameString);
                }
            }
            else
            {
                TRMSGI(TRCI_XAPI,
                       "For pPoolInfoXmlelem=%08X; "
                       "pSubpoolNameXmlelem=%08X, "
                       "pSubpoolIndexXmlelem=%08X; entry bypassed\n",
                       pPoolInfoXmlelem,
                       pSubpoolNameXmlelem,
                       pSubpoolIndexXmlelem);
            }

            pPoolInfoXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                            pPoolInfoParentXmlelem,
                                                            pPoolInfoXmlelem,
                                                            XNAME_scratch_pool_info);
        } /* while next <scratch_pool_info> found */
    }
    else
    {
        TRMSGI(TRCI_XAPI,
               "<scratch_pool_info> element not found\n");

        return STATUS_NI_FAILURE;
    }

    /*****************************************************************/
    /* Update the "real" XAPISCRPOOL table.                          */
    /*****************************************************************/
    for (i = 0;
        i < MAX_XAPISCRPOOL;
        i++)
    {
        pWkXapiscrpool = &(wkXapiscrpool[i]);
        pXapiscrpool = &(pXapicvt->xapiscrpool[i]);

        if (i == 0)
        {
            strcpy(pXapiscrpool->subpoolNameString, XAPI_DEFAULTPOOL);
        }
        else
        {
            memcpy((char*) pXapiscrpool,
                   (char*) pWkXapiscrpool,
                   sizeof(struct XAPISCRPOOL));
        }
    }

#ifdef DEBUG

    TRMEMI(TRCI_XAPI,
           &(pXapicvt->xapiscrpool[0]),
           sizeof(pXapicvt->xapiscrpool),
           "XAPISCRPOOL after <query_scrpool>:\n");

#endif

    return STATUS_SUCCESS;
}



