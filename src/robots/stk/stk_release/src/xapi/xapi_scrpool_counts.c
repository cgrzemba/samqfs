/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_scrpool_counts.c                            */
/** Description:    XAPI scratch subpool table count update service. */
/**                                                                  */
/**                 Update the XAPI client scratch subpool table     */
/**                 (XAPISCRPOOL) threshold count and volume count   */
/**                 fields.                                          */
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
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
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

#define VOLRPT_SEND_TIMEOUT        5   /* TIMEOUT values in seconds  */       
#define VOLRPT_RECV_TIMEOUT_1ST    600
#define VOLRPT_RECV_TIMEOUT_NON1ST 900


/*********************************************************************/
/* Structures:                                                       */
/*********************************************************************/

/*********************************************************************/
/* THRESHCSV: The following maps the fixed UUITEXT lines returned    */
/* in the <query_threshold> CSV response:                            */
/* (SSSSSSSSSSSSS,AA,NNN,NNNNNNN,NNNNNNN).                           */
/*********************************************************************/
struct THRESHCSV                
{
    char                subpoolName[13];
    char                _f0;           /* comma                      */
    char                acs[2];                                      
    char                _f1;           /* comma                      */
    char                subpoolIndex[3];
    char                _f2;           /* comma                      */
    char                scratchCount[7];
    char                _f3;           /* comma                      */
    char                threshold[7];

};                          


/*********************************************************************/
/* VOLRPTCSV: The following maps the fixed UUITEXT lines returned    */
/* in the VOLRPT SUMMARY(SUBPOOL) CSV response:                      */
/* (SSSSSSSSSSSSS,AA,NNN,NNNNNNN,NNNNNNN).                           */
/*********************************************************************/
struct VOLRPTCSV                
{
    char                subpoolName[13];
    char                _f0;           /* comma                      */
    char                acs[2];                                      
    char                _f1;           /* comma                      */
    char                subpoolIndex[3];
    char                _f2;           /* comma                      */
    char                scratchCount[7];
    char                _f3;           /* comma                      */
    char                nonScratchCount[7];
};


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void createThreshRequest(struct XAPICVT   *pXapicvt,
                                struct XAPIREQE  *pXapireqe,
                                char            **ptrXapiBuffer,
                                int              *pXapiBufferSize);

static int extractThreshResponse(struct XAPICVT  *pXapicvt,
                                 struct XAPIREQE *pXapireqe,
                                 struct XMLPARSE *pXmlparse);

static int scrpoolVolrpt(struct XAPICVT  *pParentXapicvt,
                         struct XAPIREQE *pParentXapireqe,
                         char             processFlag);

static void createVolrptRequest(struct XAPICVT   *pXapicvt,
                                struct XAPIREQE  *pXapireqe,
                                char            **ptrXapiBuffer,
                                int              *pXapiBufferSize);

static int extractVolrptResponse(struct XAPICVT  *pXapicvt,
                                 struct XAPIREQE *pXapireqe,
                                 struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_scrpool_counts                               */
/** Description:   XAPI scratch subpool table count update service.  */
/**                                                                  */
/** Update XAPI client scratch subpool table (XAPISCRPOOL)           */
/** threshold count and volume count fields.                         */
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
/** The XAPI scratch subpool table count update service is allowed   */
/** to proceed even when the XAPI client is in the IDLE state.       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_scrpool_counts"

extern int xapi_scrpool_counts(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char             processFlag)
{
    pid_t               pid;
    int                 lastRC;
    int                 queryRC             = STATUS_SUCCESS;
    int                 xapiBufferSize;

    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    TRMSGI(TRCI_XAPI,
           "Entered; pXapicvt=%08X, pXapireqe=%08X, "
           "processFlag=%d\n",
           pXapicvt,
           pXapireqe,
           processFlag);

    createThreshRequest(pXapicvt,
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

    /*****************************************************************/
    /* Now update the XAPISCRPOOL table with the response of the     */
    /* XAPI <query_threshold> request.                               */
    /*****************************************************************/
    if (queryRC == STATUS_SUCCESS)
    {
        lastRC = extractThreshResponse(pXapicvt,
                                       pXapireqe,
                                       pXmlparse);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractThreshResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            queryRC = lastRC;
        }
    }

    /*****************************************************************/
    /* When done, free the XMLPARSE and associated structures.       */
    /*****************************************************************/
    if (pXmlparse != NULL)
    {
        FN_FREE_HIERARCHY_STORAGE(pXmlparse);
    }

    /*****************************************************************/
    /* If problems with <query_threshold> then bypass the VOLRPT.    */
    /*****************************************************************/
    if (queryRC != STATUS_SUCCESS)
    {
        return queryRC;
    }

    /*****************************************************************/
    /* Either fork() or call the scrpoolVolrpt process to complete   */
    /* the update of the XAPISCRPOOL table.                          */
    /*****************************************************************/
    if (processFlag == XAPI_FORK)
    {
        pid = fork();

        /*************************************************************/
        /* When fork() returns 0, we are the child process.          */
        /* Announce ourselves as a new child process, then           */
        /* call scrpoolVolrpt(), then exit.                          */
        /*************************************************************/
        if (pid == 0)
        {
            TRMSGI(TRCI_XAPI,
                   ">>>> XAPI scrpoolVolrpt() asynchronous process <<<<\n");

            lastRC = scrpoolVolrpt(pXapicvt,
                                   pXapireqe,
                                   processFlag);

            TRMSGI(TRCI_XAPI,
                   "lastRC=%d from scrpoolVolrpt()\n",
                   lastRC);

            _exit(0);
        }
        /*************************************************************/
        /* When fork() returns a positive number, we are the         */
        /* parent process.  Just return STATUS_SUCCESS.              */
        /*************************************************************/
        else if (pid > 0)
        {
            TRMSGI(TRCI_XAPI,
                   "SSI parent process after fork() "
                   "of scrpoolVolrpt() asynchronous process\n");

            return STATUS_SUCCESS;
        }
        /*************************************************************/
        /* When fork() returns a negative number, the fork()         */
        /* failed.  Return STATUS_PROCESS_FAILURE, and let the       */
        /* caller decide whether to call synchronously.              */
        /*************************************************************/
        else
        {
            TRMSGI(TRCI_XAPI,
                   "fork() failed\n");

            return STATUS_PROCESS_FAILURE;
        }
    }
    else
    {
        TRMSGI(TRCI_XAPI,
               "Calling scrpoolVolrpt() synchronously\n");

        lastRC = scrpoolVolrpt(pXapicvt,
                               pXapireqe,
                               processFlag);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from scrpoolVolrpt()\n",
               lastRC);
    }

    return lastRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: createThreshRequest                               */
/** Description:   Build an XAPI <query_threshold> request.          */
/**                                                                  */
/** Build the XML format XAPI <query_threshold> with                 */
/** <all_request>YES specified to return all accessible subpools.    */
/** Additionally, request fixed format CSV output to limit the       */
/** size and parse complexity of the <query_threshold> response.     */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_threshold> request consists of:              */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**                                                                  */
/**   <csv_break>volume_data<csv_break>                              */
/**   <csv_fields>subpool_name,acs,subpool_index,                    */
/**     scratch_count,threshold_count</csv_fields>                   */
/**   <csv_fixed_flag>Y</csv_fixed_flag>                             */
/**   <csv_notitle_flag>Y</csv_notitle_flag>                         */
/**                                                                  */
/**   <command>                                                      */
/**     <query_threshold>                                            */
/**       <all_request>YES</all_request>                             */
/**     </query_threshold>                                           */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "createThreshRequest"

static void createThreshRequest(struct XAPICVT  *pXapicvt,
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
                        FALSE,
                        FALSE,
                        XML_CASE_UPPER,
                        XML_DATE_STCK);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_break_tag,
                                      XNAME_subpool_data,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_notitle_flag,
                                      XCONTENT_Y,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_fixed_flag,
                                      XCONTENT_Y,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_fields,
                                      "subpool_name,acs,subpool_index,scratch_count,threshold_count",
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_command,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
//                                    XNAME_query_threshold,
                                      XNAME_query_scratch,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_all_request,
                                      XCONTENT_YES,
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
/** Function Name: extractThreshResponse                             */
/** Description:   Extract the <query_threshold> CSV response.       */
/**                                                                  */
/** Parse the CSV response from the <query_threshold> XAPI           */
/** request and update the appropriate fields of the working copy    */
/** XAPISCRPOOL table.  Each <uui_text> tag represents a             */
/** single scratch subpool by ACS.                                   */
/**                                                                  */
/**==================================================================*/
/** The XAPI CSV <query_threshold> responses consists of:            */
/**==================================================================*/
/** <libreply>                                                       */
/**   <uui_line_type>V</uui_line_type>                               */
/**   <uui_text>SSSSSSSSSSSSS,AA,NNN,NNNNNNN,NNNNNNN         (Fixed) */
/**     </uui_text>                                                  */
/**   ...repeated <uui_line> and <uui_text> entries                  */
/**   <exceptions>                                                   */
/**     <reason>ccc...ccc</reason>                                   */
/**     ...repeated <reason> entries                                 */
/**   </exceptions>                                                  */
/**   <uui_return_code>nnnn</uui_return_code>                        */
/**   <uui_reason_code>nnnn</uui_reason_code>                        */
/** </libreply>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractThreshResponse"

static int extractThreshResponse(struct XAPICVT  *pXapicvt,
                                 struct XAPIREQE *pXapireqe,
                                 struct XMLPARSE *pXmlparse)
{
    int                 lastRC;
    int                 i;
    int                 subpoolIndex;
    int                 scratchCount;
    int                 threshold;
    char                subpoolNameString[XAPI_SUBPOOL_NAME_SIZE + 1];

    struct XAPISCRPOOL  wkXapiscrpool[MAX_XAPISCRPOOL];
    struct XAPISCRPOOL *pWkXapiscrpool;
    struct XAPISCRPOOL *pXapiscrpool;
    char               *pChar;
    struct XMLELEM     *pFirstUuiTextXmlelem;
    struct XMLELEM     *pNextUuiTextXmlelem;
    struct XMLELEM     *pParentXmlelem;
    struct THRESHCSV   *pThreshcsv;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Create a working copy of the XAPISCRPOOL table.               */
    /*****************************************************************/
    for (i = 0;
        i < MAX_XAPISCRPOOL;
        i++)
    {
        pWkXapiscrpool = &(wkXapiscrpool[i]);
        pXapiscrpool = &(pXapicvt->xapiscrpool[i]);

        memcpy((char*) pWkXapiscrpool,
               (char*) pXapiscrpool,
               sizeof(struct XAPISCRPOOL));

        pWkXapiscrpool->scratchCount = 0;
        pWkXapiscrpool->volumeCount = 0;
    }

    /*****************************************************************/
    /* Find the 1st <uui_text> entry.                                */
    /*****************************************************************/
    pFirstUuiTextXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                   pXmlparse->pHocXmlelem,
                                                   XNAME_uui_text);

    /*****************************************************************/
    /* If no <uui_text> elements found, then return an error.        */
    /*****************************************************************/
    if (pFirstUuiTextXmlelem == 0)
    {
        return STATUS_NI_FAILURE;
    }

    /*****************************************************************/
    /* If <uui_text> elements found, then extract the data.          */
    /*****************************************************************/
    pNextUuiTextXmlelem = pFirstUuiTextXmlelem;
    pParentXmlelem = pFirstUuiTextXmlelem->pParentXmlelem;

    while (pNextUuiTextXmlelem != NULL)
    {
        pThreshcsv = (struct THRESHCSV*) pNextUuiTextXmlelem->pContent;

        /*************************************************************/
        /* Zero fill the subpoolIndex, scratchCount and              */
        /* nonScratchCount fields of the CSV.                        */
        /*************************************************************/
        for (i = 0, pChar = (char*) &(pThreshcsv->subpoolIndex);
            i < (sizeof(struct THRESHCSV) - offsetof(struct THRESHCSV, subpoolIndex)); 
            i++, pChar++)
        {
            if (*pChar == ' ')
            {
                *pChar = '0';
            }
        }

//#ifdef DEBUG

        TRMEM(pThreshcsv, sizeof(struct THRESHCSV), 
              "THRESHCSV:\n");

//#endif

        subpoolIndex = 0;

        STRIP_TRAILING_BLANKS(pThreshcsv->subpoolName,
                              subpoolNameString,
                              sizeof(pThreshcsv->subpoolName));

        if (strcmp(subpoolNameString, XAPI_DEFAULTPOOL) != 0)
        {
            FN_CONVERT_DIGITS_TO_FULLWORD(pThreshcsv->subpoolIndex,
                                          sizeof(pThreshcsv->subpoolIndex),
                                          &subpoolIndex);
        }

        if ((subpoolIndex >= 0) &&
            (subpoolIndex < MAX_XAPISCRPOOL))
        {
            pWkXapiscrpool = &(wkXapiscrpool[subpoolIndex]);
            scratchCount = 0;

            FN_CONVERT_DIGITS_TO_FULLWORD(pThreshcsv->scratchCount,
                                          sizeof(pThreshcsv->scratchCount),
                                          &scratchCount);

            threshold = 0;

            FN_CONVERT_DIGITS_TO_FULLWORD(pThreshcsv->threshold,
                                          sizeof(pThreshcsv->threshold),
                                          &threshold);

            TRMSGI(TRCI_XAPI,
                   "For subpool[%d]=%s, scratchCount=%d, threshold=%d\n",
                   subpoolIndex,
                   pWkXapiscrpool->subpoolNameString,
                   scratchCount,
                   threshold);

            pWkXapiscrpool->scratchCount += scratchCount;
            pWkXapiscrpool->threshold += threshold;

            if ((memcmp(pThreshcsv->acs,
                        "VS",
                        2) == 0) &&
                (scratchCount > 0))
            {
                TRMSGI(TRCI_XAPI,
                       "Marking subpool[%d]=%s as virtual\n",
                       subpoolIndex,
                       pWkXapiscrpool->subpoolNameString);

                pWkXapiscrpool->virtualFlag = TRUE;
            }
        }

        pNextUuiTextXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                           pParentXmlelem,
                                                           pNextUuiTextXmlelem,
                                                           XNAME_uui_text);
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

        if (pWkXapiscrpool->subpoolNameString[0] > 0)
        {
            TRMSGI(TRCI_XAPI,
                   "For subpool[%d]=%s(%s), "
                   "old scratchCount=%d, threshold=%d; "
                   "new scratchCount=%d, threshold=%d\n",
                   i,
                   pXapiscrpool->subpoolNameString,
                   pWkXapiscrpool->subpoolNameString,
                   pXapiscrpool->scratchCount,
                   pXapiscrpool->threshold,
                   pWkXapiscrpool->scratchCount,
                   pWkXapiscrpool->threshold);

            pXapiscrpool->scratchCount = pWkXapiscrpool->scratchCount;
            pXapiscrpool->threshold = pWkXapiscrpool->threshold;
            pXapiscrpool->virtualFlag = pWkXapiscrpool->virtualFlag;

            /*********************************************************/
            /* Until we get the volume count from                    */
            /* VOLRPT SUMMARY(SBUPOOL), set a temporary volumeCount  */
            /* as the scratchCount.                                  */
            /*********************************************************/
            pXapiscrpool->volumeCount = pXapiscrpool->scratchCount;
        }
    }

#ifdef DEBUG

    TRMEMI(TRCI_XAPI,
           &(pXapicvt->xapiscrpool[0]),
           sizeof(pXapicvt->xapiscrpool),
           "XAPISCRPOOL after <query_threshold>:\n");

#endif 

    return STATUS_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: scrpoolVolrpt                                     */
/** Description:   Update XAPISCRPOOL table volume counts.           */
/**                                                                  */
/** Real volume counts are obtained by issuing a                     */
/** VOLRPT SUMMARY(SUBPOOL) command specifying CSV output.           */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "scrpoolVolrpt"

static int scrpoolVolrpt(struct XAPICVT  *pParentXapicvt,
                         struct XAPIREQE *pParentXapireqe,
                         char             processFlag)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;

    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;
    struct XAPICVT     *pXapicvt            = NULL;
    struct XAPIREQE    *pXapireqe           = NULL;
    struct XAPIREQE     wkXapireqe; 

    TRMSGI(TRCI_XAPI,
           "Entered; pParentXapicvt=%08X, pParentXapireqe=%08X, "
           "processFlag=%d\n",
           pParentXapicvt,
           pParentXapireqe,
           processFlag);

    /*****************************************************************/
    /* If invoked via fork(), then we must attach the XAPICVT shared */
    /* memory segment.   We must also use a work XAPIREQE as         */
    /* the "real" XAPIREQE might be updated by a now-parallel parent */
    /* process.                                                      */
    /*****************************************************************/
    if (processFlag == XAPI_FORK)
    {
        pXapicvt = xapi_attach_xapicvt();
        pXapireqe = NULL;

        if (pXapicvt == NULL)
        {
            return STATUS_SHARED_MEMORY_ERROR;
        }
    }
    else
    {
        pXapicvt = pParentXapicvt;

        if (pXapicvt == NULL)
        {
            return STATUS_PROCESS_FAILURE;
        }
    }

    if (pXapireqe == NULL)
    {
        pXapireqe = &wkXapireqe;

        xapi_attach_work_xapireqe(pXapicvt,
                                  pXapireqe,
                                  NULL,
                                  0);
    }

    createVolrptRequest(pXapicvt,
                        pXapireqe,
                        &pXapiBuffer,
                        &xapiBufferSize);

    lastRC = xapi_tcp(pXapicvt,
                      pXapireqe,
                      pXapiBuffer,
                      xapiBufferSize,
                      VOLRPT_SEND_TIMEOUT,       
                      VOLRPT_RECV_TIMEOUT_1ST,   
                      VOLRPT_RECV_TIMEOUT_NON1ST,
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
    /* Now update the XAPISCRPOOL with the response received from    */
    /* the VOLRPT SUMMARY(SUBPOOL) request.                          */
    /*****************************************************************/
    if (queryRC == STATUS_SUCCESS)
    {
        lastRC = extractVolrptResponse(pXapicvt,
                                       pXapireqe,
                                       pXmlparse);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractVolrptResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            queryRC = lastRC;
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
/** Function Name: createVolrptRequest                               */
/** Description:   Create the XAPI VOLRPT SUMMARY(SUBPOOL) request.  */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML VOLRPT SUMMARY(SUBPOOL) request consists of:        */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**                                                                  */
/**   <csv_break>subpool_data<csv_break>                             */
/**   <csv_fields>subpool_data.subpool_name,                         */
/**     subpool_data.acs,                                            */
/**     subpool_data.subpool_index,                                  */
/**     subpool_data.scratch_count,                                  */
/**     subpool_data.non_scratch_count</csv_fields>                  */
/**   <csv_fixed_flag>Y</csv_fixed_flag>                             */
/**   <csv_notitle_flag>Y</csv_notitle_flag>                         */
/**                                                                  */
/**   <command>                                                      */
/**     VOLRPT SUMMARY(SUBPOOL)                                      */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "createVolrptRequest"

static void createVolrptRequest(struct XAPICVT  *pXapicvt,
                                struct XAPIREQE *pXapireqe,
                                char           **ptrXapiBuffer,
                                int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;
    char               *pXapiRequest        = NULL;

    struct XMLPARSE    *pXmlparse; 
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
                        FALSE,
                        FALSE,
                        XML_CASE_UPPER,
                        XML_DATE_STCK);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_break_tag,
                                      XNAME_subpool_data,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_notitle_flag,
                                      XCONTENT_Y,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_fixed_flag,
                                      XCONTENT_Y,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_fields,
                                      "subpool_data.subpool_name,"
                                      "subpool_data.acs,"
                                      "subpool_data.subpool_index,"
                                      "subpool_data.scratch_count,"
                                      "subpool_data.non_scratch_count",
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_command,
                                      "VOLRPT SUMMARY(SUBPOOL)",
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
/** Function Name: extractVolrptResponse                             */
/** Description:   Extract the VOLRPT SUMMARY(SUBPOOL) CSV response. */
/**                                                                  */
/** Parse the CSV response from the VOLRPT SUMMARY(SUBPOOL) XAPI     */
/** request and update the appropriate fields of the working copy    */
/** XAPISCRPOOL table.  Each <uui_text> tag represents a             */
/** single scratch subpool by ACS.                                   */
/**                                                                  */
/**==================================================================*/
/** The XAPI CSV VOLRPT SUMMARY(SUBPOOL) responses consists of:      */
/**==================================================================*/
/** <libreply>                                                       */
/**   <uui_line_type>V</uui_line_type>                               */
/**   <uui_text>SSSSSSSSSSSSS,AA,NNN,NNNNNNN,NNNNNNN         (Fixed) */
/**     </uui_text>                                                  */
/**   ...repeated <uui_line> and <uui_text> entries                  */
/**   <exceptions>                                                   */
/**     <reason>ccc...ccc</reason>                                   */
/**     ...repeated <reason> entries                                 */
/**   </exceptions>                                                  */
/**   <uui_return_code>nnnn</uui_return_code>                        */
/**   <uui_reason_code>nnnn</uui_reason_code>                        */
/** </libreply>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractVolrptResponse"

static int extractVolrptResponse(struct XAPICVT  *pXapicvt,
                                 struct XAPIREQE *pXapireqe,
                                 struct XMLPARSE *pXmlparse)
{
    int                 lastRC;
    int                 i;
    int                 subpoolIndex;
    int                 scratchCount;
    int                 nonScratchCount;
    char                subpoolNameString[XAPI_SUBPOOL_NAME_SIZE + 1];

    char               *pChar;
    struct XAPISCRPOOL  wkXapiscrpool[MAX_XAPISCRPOOL];
    struct XAPISCRPOOL *pWkXapiscrpool;
    struct XAPISCRPOOL *pXapiscrpool;
    struct XMLELEM     *pFirstUuiTextXmlelem;
    struct XMLELEM     *pNextUuiTextXmlelem;
    struct XMLELEM     *pParentXmlelem;
    struct VOLRPTCSV   *pVolrptcsv;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Create a working copy of the XAPISCRPOOL table.               */
    /*****************************************************************/
    for (i = 0;
        i < MAX_XAPISCRPOOL;
        i++)
    {
        pWkXapiscrpool = &(wkXapiscrpool[i]);
        pXapiscrpool = &(pXapicvt->xapiscrpool[i]);

        memcpy((char*) pWkXapiscrpool,
               (char*) pXapiscrpool,
               sizeof(struct XAPISCRPOOL));

        pWkXapiscrpool->scratchCount = 0;
        pWkXapiscrpool->volumeCount = 0;
    }

    /*****************************************************************/
    /* Find the 1st <uui_text> entry.                                */
    /*****************************************************************/
    pFirstUuiTextXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                   pXmlparse->pHocXmlelem,
                                                   XNAME_uui_text);

    /*****************************************************************/
    /* If no <uui_text> elements found, then return an error.        */
    /*****************************************************************/
    if (pFirstUuiTextXmlelem == 0)
    {
        return STATUS_NI_FAILURE;
    }

    /*****************************************************************/
    /* If <uui_text> elements found, then extract the data.          */
    /*****************************************************************/
    pNextUuiTextXmlelem = pFirstUuiTextXmlelem;
    pParentXmlelem = pFirstUuiTextXmlelem->pParentXmlelem;

    while (pNextUuiTextXmlelem != NULL)
    {
        pVolrptcsv = (struct VOLRPTCSV*) pNextUuiTextXmlelem->pContent;

        /*************************************************************/
        /* Zero fill the subpoolIndex, scratchCount and              */
        /* nonScratchCount fields of the CSV.                        */
        /*************************************************************/
        for (i = 0, pChar = (char*) &(pVolrptcsv->subpoolIndex);
            i < (sizeof(struct VOLRPTCSV) - offsetof(struct VOLRPTCSV, subpoolIndex)); 
            i++, pChar++)
        {
            if (*pChar == ' ')
            {
                *pChar = '0';
            }
        }

//#ifdef DEBUG

        TRMEM(pVolrptcsv, sizeof(struct VOLRPTCSV), 
              "VOLRPTCSV:\n");

//#endif

        subpoolIndex = 0;

        STRIP_TRAILING_BLANKS(pVolrptcsv->subpoolName,
                              subpoolNameString,
                              sizeof(pVolrptcsv->subpoolName));

        if (strcmp(subpoolNameString, XAPI_DEFAULTPOOL) != 0)
        {
            FN_CONVERT_DIGITS_TO_FULLWORD(pVolrptcsv->subpoolIndex,
                                          sizeof(pVolrptcsv->subpoolIndex),
                                          &subpoolIndex);
        }

        if ((subpoolIndex >= 0) &&
            (subpoolIndex < MAX_XAPISCRPOOL))
        {
            pWkXapiscrpool = &(wkXapiscrpool[subpoolIndex]);
            scratchCount = 0;

            FN_CONVERT_DIGITS_TO_FULLWORD(pVolrptcsv->scratchCount,
                                          sizeof(pVolrptcsv->scratchCount),
                                          &scratchCount);

            nonScratchCount = 0;

            FN_CONVERT_DIGITS_TO_FULLWORD(pVolrptcsv->nonScratchCount,
                                          sizeof(pVolrptcsv->nonScratchCount),
                                          &nonScratchCount);

            TRMSGI(TRCI_XAPI,
                   "For subpool[%d]=%s, scratchCount=%d, nonScratchCount=%d\n",
                   subpoolIndex,
                   pWkXapiscrpool->subpoolNameString,
                   scratchCount,
                   nonScratchCount);

            pWkXapiscrpool->scratchCount += scratchCount;
            pWkXapiscrpool->volumeCount += (scratchCount + nonScratchCount);
        }

        pNextUuiTextXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                           pParentXmlelem,
                                                           pNextUuiTextXmlelem,
                                                           XNAME_uui_text);
    }

    /*****************************************************************/
    /* Update the "real" XAPISCRPOOL table if the subpool is         */
    /* not totally or partially virtual.                             */
    /*****************************************************************/
    for (i = 0;
        i < MAX_XAPISCRPOOL;
        i++)
    {
        pWkXapiscrpool = &(wkXapiscrpool[i]);
        pXapiscrpool = &(pXapicvt->xapiscrpool[i]);

        if (pWkXapiscrpool->subpoolNameString[0] > 0)
        {
            TRMSGI(TRCI_XAPI,
                   "For subpool[%d]=%s(%s), "
                   "old scratchCount=%d, volumeCount=%d, virtualFlag=%d; "
                   "new scratchCount=%d, volumeCount=%d\n",
                   i,
                   pXapiscrpool->subpoolNameString,
                   pWkXapiscrpool->subpoolNameString,
                   pXapiscrpool->scratchCount,
                   pXapiscrpool->volumeCount,
                   pXapiscrpool->virtualFlag,
                   pWkXapiscrpool->scratchCount,
                   pWkXapiscrpool->volumeCount);

            if (!(pXapiscrpool->virtualFlag))
            {
                pXapiscrpool->scratchCount = pWkXapiscrpool->scratchCount;
                pXapiscrpool->volumeCount = pWkXapiscrpool->volumeCount;
            }
        }
    }

//#ifdef DEBUG

    TRMEMI(TRCI_XAPI,
           &(pXapicvt->xapiscrpool[0]),
           sizeof(pXapicvt->xapiscrpool),
           "XAPISCRPOOL after VOLRPT:\n");

//endifs

    return STATUS_SUCCESS;
}



