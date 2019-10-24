/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_config.c                                    */
/** Description:    XAPI client drive configuration table (XAPICFG)  */
/**                 build service.                                   */
/**                                                                  */
/**                 The XAPICFG is the XAPI client drive             */
/**                 configuration table.  There is one XAPICFG       */
/**                 table entry for each TapePlex drive that is      */
/**                 accessible from the XAPI client.  The XAPICFG    */
/**                 contains a subset of all TapePlex real and       */
/**                 virtual tape drives.                             */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     05/11/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/***END PROLOGUE******************************************************/

/*********************************************************************/
/* Includes:                                                         */
/*********************************************************************/
#include <errno.h>
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
#define CONFIG_SEND_TIMEOUT        5   /* TIMEOUT values in seconds  */       
#define CONFIG_RECV_TIMEOUT_1ST    300
#define CONFIG_RECV_TIMEOUT_NON1ST 900


/*********************************************************************/
/* Structures:                                                       */
/*********************************************************************/

/*********************************************************************/
/* QDRVCSV: The following maps the fixed UUITEXT lines returned in   */
/* the CSV response (R:AA:LL:PP:DD  ,CCUU,MMMMMMMM,RRRRRRRR).        */
/*********************************************************************/
struct QDRVCSV                
{
    char                driveLocId[16];
    char                _f0;           /* comma                      */
    char                driveName[4];
    char                _f1;           /* comma                      */
    char                model[8];
    char                _f2;           /* comma                      */
    char                rectech[8];
};


/*********************************************************************/
/* XAPIVACS keeps track of SVTSS-to-dummy ACS mapping.               */
/*********************************************************************/
struct XAPIVACS
{
    char                vtssNameString[XAPI_VTSSNAME_SIZE + 1];
    unsigned char       vacsId;
};


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void buildConfigRequest(struct XAPICVT   *pXapicvt,
                               struct XAPIREQE  *pXapireqe,
                               char            **ptrXapiBuffer,
                               int              *pXapiBufferSize);

static int extractConfigResponse(struct XAPICVT   *pXapicvt,
                                 struct XAPIREQE  *pXapireqe,
                                 struct XMLPARSE  *pXmlparse,
                                 struct XAPIDRLST *pXapidrlst,
                                 int               numXapidrlst);

static int matchXapidrlstEntry(struct XAPICVT   *pXapicvt,
                               struct XAPIREQE  *pXapireqe,
                               struct QDRVCSV   *pQdrvcsv,
                               struct XAPIDRLST *pXapidrlst,
                               int               numXapidrlst);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_config                                       */
/** Description:   XAPI drive configuration table build service.     */
/**                                                                  */
/** Build the XAPI client drive configuration table (XAPICFG).       */
/**                                                                  */
/** This process is serialized by the XAPICVT.xapicfgLock            */
/** semaphore (see xapi_main).                                       */
/**                                                                  */
/** Process the XAPI_DRIVE_LIST environment variable and build a     */
/** list of drives that will possibly be included in the XAPI        */
/** client drive configuration (the XAPI client configuration is     */
/** the intersection of the XAPI_DRIVE_LIST and the returned         */
/** TapePlex drive list configuration).                              */
/**                                                                  */
/** If there is no XAPI_DRIVE_LIST environment variable, then        */
/** the XAPI client drive configuration will include ALL tape        */
/** drives in the returned TapePlex server drive list configuration. */
/**                                                                  */
/** The TapePlex server drive list configuration is obtained by      */
/** creating an XAPI XML format <config_info_request> request;       */
/** To limit the size of the resultant response, the request         */
/** specifies CSV output; the request is transmitted to the          */
/** server via TCP/IP; the returned CSV text lines (each line        */
/** representing a single TapePlex tape drive) are then compared     */
/** to the drives specifed in the XAPI_DRIVE_LIST; If a match        */
/** is found (or if there was no XAPI_DRIVE_LIST), then the          */
/** drive is added to the XAPI client drive configuration.           */
/**                                                                  */
/** Each drive added to the XAPI client drive configuration is       */
/** represented by an XAPICFG structure; The XAPICFG structures      */
/** are consolidated into a single block of shared memory;           */
/** Any pre-existing XAPICFG shared memory block is detached and     */
/** removed;  The attributes of the new XAPICFG shared memory block  */
/** are recorded in the XAPICVT.                                     */
/**                                                                  */
/** The configuration build or rebuild is allowed to proceed even    */
/** when the XAPI client is in the IDLE state.                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_config"

extern int xapi_config(struct XAPICVT  *pXapicvt,
                       struct XAPIREQE *pXapireqe)
{
    int                 configRC            = STATUS_SUCCESS;
    int                 lastRC;
    int                 numXapidrlst;
    int                 xapiBufferSize;
    int                 i;

    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;
    struct XAPIDRLST   *pXapidrlst          = NULL;
    struct XAPIDRLST   *pCurrXapidrlst;
    char                driveString[17];

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Process the XAPI_DRIVE_LIST environment variable and return   */
    /* the XAPIDRLST structure and the number of drives in the list. */
    /*                                                               */
    /* We will use the XAPIDRLST when we process the XAPI            */
    /* <query_drive_info> response to potentially select only a      */
    /* subset of "library" devices for the XAPI.                     */
    /*****************************************************************/
    lastRC = xapi_drive_list(pXapicvt,
                             pXapireqe,
                             &pXapidrlst,
                             &numXapidrlst);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from xapi_drive_list; "
           "numXapidrlst=%d, pXapidrlst=%08X\n",
           lastRC,
           numXapidrlst,
           pXapidrlst);

    buildConfigRequest(pXapicvt,
                       pXapireqe,
                       &pXapiBuffer,
                       &xapiBufferSize);

    lastRC = xapi_tcp(pXapicvt,
                      pXapireqe,
                      pXapiBuffer,
                      xapiBufferSize,
                      CONFIG_SEND_TIMEOUT,       
                      CONFIG_RECV_TIMEOUT_1ST,   
                      CONFIG_RECV_TIMEOUT_NON1ST,
                      (void**) &pXmlparse);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from xapi_tcp(pBuffer=%08X, size=%d, pXmlparse=%08X)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize,
           pXmlparse);

    if (lastRC != STATUS_SUCCESS)
    {
        configRC = lastRC;
    }

    /*****************************************************************/
    /* At this point we can free the XAPI XML request string.        */
    /*****************************************************************/
    TRMSGI(TRCI_STORAGE,
           "free pXapiBuffer=%08X, len=%i\n",
           pXapiBuffer,
           xapiBufferSize);

    free(pXapiBuffer);

    if (configRC == STATUS_SUCCESS)
    {
        lastRC = extractConfigResponse(pXapicvt,
                                       pXapireqe,
                                       pXmlparse,
                                       pXapidrlst,
                                       numXapidrlst);

        TRMSGI(TRCI_XAPI,"lastRC=%d from extractConfigResponse\n",
               lastRC);

        if (lastRC != STATUS_SUCCESS)
        {
            configRC = STATUS_PROCESS_FAILURE;
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
    /* Test for unmatched XAPIDRLST entries (these would be drives   */
    /* that the user thought would be in the configuration, but      */
    /* which were not returned in the XAPI <query_drive_info>        */
    /* response).                                                    */
    /*****************************************************************/
    for (i = 0;
        i < numXapidrlst;
        i++)
    {
        pCurrXapidrlst = &(pXapidrlst[i]);

        if (!(pCurrXapidrlst->configDriveFlag))
        {
            if (pCurrXapidrlst->driveName[0] > ' ')
            {
                STRIP_TRAILING_BLANKS(pCurrXapidrlst->driveName,
                                      driveString,
                                      sizeof(pCurrXapidrlst->driveName));
            }
            else
            {
                STRIP_TRAILING_BLANKS(pCurrXapidrlst->driveLocId,
                                      driveString,
                                      sizeof(pCurrXapidrlst->driveLocId));
            }

            LOGMSG(STATUS_DRIVE_NOT_IN_LIBRARY, 
                   "XAPI_DRIVE_LIST drive=%s not in configuration",
                   driveString);

            configRC = STATUS_DRIVE_NOT_IN_LIBRARY;
        }
    }

    /*****************************************************************/
    /* When done, also free the XAPIDRLST (if allocated).            */
    /*****************************************************************/
    if (pXapidrlst != NULL)
    {
        TRMSGI(TRCI_STORAGE,
               "free pXapidrlst%08X, len=%i\n",
               pXapidrlst,
               (numXapidrlst * sizeof(struct XAPIDRLST)));

        free(pXapidrlst);
    }

    return configRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: buildConfigRequest                                */
/** Description:   Build an XAPI <query_drive_info> request.         */
/**                                                                  */
/** Build an XAPI <query_drive_info> request with                    */
/** <config_info_request>YES specified to return all TapePlex        */
/** devices.  Additionally, specify CSV fixed format output to       */
/** limit the size and parse complexity of the response.             */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_drive_info> request consists of:             */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**                                                                  */
/**   <csv_break>drive_info<csv_break>                               */
/**   <csv_fields>drive_location_id,drive_name,model,rectech         */
/**     </csv_fields>                                                */
/**   <csv_fixed_flag>Y</csv_fixed_flag>                             */
/**   <csv_notitle_flag>Y</csv_notitle_flag>                         */
/**                                                                  */
/**   <command>                                                      */
/**     <query_drive_info>                                           */
/**       <config_info_request>YES</config_info_request>             */
/**     </query_drive_info>                                          */
/**   </command>                                                     */
/** </libtrans>                                                      */
/*                                                                   */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "buildConfigRequest"

static void buildConfigRequest(struct XAPICVT  *pXapicvt,
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
                                      XNAME_drive_info,
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
                                      "drive_location_id,drive_name,model,rectech",
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_command,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_query_drive_info,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_config_info_request,
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
/** Function Name: extractConfigResponse                             */
/** Description:   Extract the <query_drive_info> response.          */
/**                                                                  */
/** Parse the fixed format CSV response from the <query_drive_info>  */
/** request and build a new XAPICFG (the XAPI client drive           */
/** configuration table).  The new XAPICFG table is created as a     */
/** shared memory segment whose attributes are recorded in the       */
/** XAPICVT.                                                         */
/**                                                                  */
/**==================================================================*/
/** The XAPI CSV <query_drive_info> response consists of:            */
/**==================================================================*/
/** <libreply>                                                       */
/**   <uui_line_type>V</uui_line_type>                               */
/**   <uui_text>R:AA:LL:PP:DD   ,CCUU,MMMMMMMMMM,RRRRRRRR    (FIXED) */
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
#define SELF "extractConfigResponse"

static int extractConfigResponse(struct XAPICVT   *pXapicvt,
                                 struct XAPIREQE  *pXapireqe,
                                 struct XMLPARSE  *pXmlparse,
                                 struct XAPIDRLST *pXapidrlst,
                                 int               numXapidrlst)
{
    int                 lastRC;
    int                 i;
    int                 j;
    int                 k;
    int                 l;
    int                 locInt;
    int                 driveCount          = 0;
    int                 matchingDriveCount  = 0;
    int                 xapivacsCount       = 0;
    int                 xapicfgSize         = 0;
    int                 shMemSegId;
    int                 shmFlags;
    int                 shmPermissions;
    key_t               shmKey;
    size_t              shmSize;

    struct XAPICFG     *pWorkXapicfg        = NULL;
    struct XAPICFG     *pNewXapicfg         = NULL;
    struct XAPICFG     *pNextXapicfg;
    struct XAPIVACS    *pXapivacs;
    struct XAPIDRVTYP  *pXapidrvtyp;

    struct XMLELEM     *pFirstUuiTextXmlelem;
    struct XMLELEM     *pNextUuiTextXmlelem;
    struct XMLELEM     *pParentXmlelem;
    struct QDRVCSV     *pQdrvcsv;

    char                acs[2];
    char                lsm[2];
    char                panel[2];
    char                driveNumber[2];
    char                driveNumberFlag;
    char                vtssNameString[XAPI_VTSSNAME_SIZE + 1];
    char                driveNumberString[XAPI_DRIVENUMBER_SIZE + 1];
    char                modelNameString[XAPI_MODEL_NAME_SIZE + 1];

    struct XAPIVACS     xapivacs[MAX_ACS_NUMBER];

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    memset((char*) &xapivacs, 0, sizeof(xapivacs));

    /*****************************************************************/
    /* Count the number of matching <uui_text> entries so we can     */
    /* compute the length of the XAPICFG drive configuration table.  */
    /*****************************************************************/
    pFirstUuiTextXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                   pXmlparse->pHocXmlelem,
                                                   XNAME_uui_text);

    if (pFirstUuiTextXmlelem != NULL)
    {
        pParentXmlelem = pFirstUuiTextXmlelem->pParentXmlelem;

        TRMSGI(TRCI_XAPI,
               "pFirstUuiTextXmlelem=%08X; pParentXmlelem=%08X\n",
               pFirstUuiTextXmlelem,
               pParentXmlelem);

        pNextUuiTextXmlelem = pFirstUuiTextXmlelem;

        while (pNextUuiTextXmlelem != NULL)
        {
            pQdrvcsv = (struct QDRVCSV*) pNextUuiTextXmlelem->pContent;
            driveCount++;

            lastRC = matchXapidrlstEntry(pXapicvt,
                                         pXapireqe,
                                         pQdrvcsv,
                                         pXapidrlst,
                                         numXapidrlst);


            if (lastRC == STATUS_SUCCESS)
            {
                matchingDriveCount++;
            }

            pNextUuiTextXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                               pParentXmlelem,
                                                               pNextUuiTextXmlelem,
                                                               XNAME_uui_text);

#ifdef DEBUG

            if (pNextUuiTextXmlelem != NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextUuiTextXmlelem=%08X\n",
                       pNextUuiTextXmlelem);
            }

#endif

        }
    }

    TRMSGI(TRCI_XAPI,
           "driveCount=%d, matchingDriveCount=%d\n",
           driveCount,
           matchingDriveCount);

    /*****************************************************************/
    /* If no matching <uui_text> elements found, then return an      */
    /* error.                                                        */
    /*****************************************************************/
    if (matchingDriveCount == 0)
    {
        return STATUS_NI_FAILURE;
    }

    /*****************************************************************/
    /* If <uui_text> elements found, then get a work XAPICFG         */
    /* large enough for all of the <uui_text> elements in the        */
    /* XAPI response.                                                */
    /*****************************************************************/
    xapicfgSize = matchingDriveCount * (sizeof(struct XAPICFG));

    pWorkXapicfg = (struct XAPICFG*) malloc(xapicfgSize);

    TRMSGI(TRCI_STORAGE,
           "malloc work XAPICFG=%08X, len=%i\n",
           pWorkXapicfg,
           xapicfgSize);

    memset((char*) pWorkXapicfg, 0, xapicfgSize);

    /*****************************************************************/
    /* If <uui_text> elements found, then extract the data.          */
    /*****************************************************************/
    pNextUuiTextXmlelem = pFirstUuiTextXmlelem;

    for (i = 0, pNextXapicfg = pWorkXapicfg;
        i < driveCount;
        i++)
    {
        if (pNextUuiTextXmlelem == NULL)
        {
            TRMSGI(TRCI_XAPI,
                   "pNextUuiTextXmlelem=NULL at driveCount=%i\n",
                   (i+1));

            break;
        }

        pQdrvcsv = (struct QDRVCSV*) pNextUuiTextXmlelem->pContent;

#ifdef DEBUG

        TRMEMI(TRCI_XAPI,
               pQdrvcsv, sizeof(struct QDRVCSV),
               "QDRVCSV:\n");

#endif

        lastRC = matchXapidrlstEntry(pXapicvt,
                                     pXapireqe,
                                     pQdrvcsv,
                                     pXapidrlst,
                                     numXapidrlst);


        /*************************************************************/
        /* If matching XAPIDRLST entry, then begin initialization    */
        /* of the current XAPICFG configuration drive entry.         */
        /*************************************************************/
        if (lastRC == STATUS_SUCCESS)
        {
            if (pQdrvcsv->driveLocId[0] <= ' ')
            {
                TRMSGI(TRCI_XAPI,
                       "QDRVCSV=%08X has no driveLocId\n",
                       pQdrvcsv);
            }
            else
            {
                memcpy(pNextXapicfg->driveLocId,
                       pQdrvcsv->driveLocId,
                       sizeof(pNextXapicfg->driveLocId));
            }

            if (pQdrvcsv->driveName[0] > ' ')
            {
                FN_CONVERT_CHARDEVADDR_TO_HEX(pQdrvcsv->driveName,
                                              &pNextXapicfg->driveName);
            }
            else
            {
                TRMSGI(TRCI_XAPI,
                       "Entry for driveLocId=%.16s has no <drive_name>\n",
                       pQdrvcsv->driveLocId);
            }

            if (pQdrvcsv->model[0] > ' ')
            {
                memcpy(pNextXapicfg->model,
                       pQdrvcsv->model,
                       sizeof(pNextXapicfg->model));

                STRIP_TRAILING_BLANKS(pNextXapicfg->model,
                                      modelNameString,
                                      sizeof(pNextXapicfg->model));

                pXapidrvtyp = xapi_drvtyp_search_name(pXapicvt,
                                                      pXapireqe,
                                                      modelNameString);

                if (pXapidrvtyp != NULL)
                {
                    pNextXapicfg->acsapiDriveType = pXapidrvtyp->acsapiDriveType;
                }
            }

            if (pQdrvcsv->rectech[0] > ' ')
            {
                memcpy(pNextXapicfg->rectech,
                       pQdrvcsv->rectech,
                       sizeof(pNextXapicfg->rectech));
            }

            /*********************************************************/
            /* If this is a VIRTUAL tape device then test if the     */
            /* VTSS is represented in the XAPIVACS table.            */
            /* If not, then add it to the table.                     */
            /*********************************************************/
            if (memcmp(pNextXapicfg->model, 
                       "VIRTUAL ",
                       sizeof(pNextXapicfg->model)) == 0)
            {
                pXapicvt->virtualFlag |= VIRTUAL_ENABLED;

                /*****************************************************/
                /* First extract the VTSS name and potential "dummy" */
                /* driveNumber from the <drive_location_id> which is */
                /* in the format: "V:vtssname:nnnn".                 */
                /*                                                   */
                /* NOTE: We start extracting with the 3rd character  */
                /*****************************************************/
                driveNumberFlag = FALSE;
                memset(vtssNameString, 0, sizeof(vtssNameString));
                memset(driveNumberString, 0, sizeof(driveNumberString));

                if (memcmp("V:",
                           pQdrvcsv->driveLocId,
                           2) == 0)
                {
                    for (j = 2, k = 0, l = 0;
                        j < sizeof(pQdrvcsv->driveLocId);
                        j++)
                    {
                        if (pQdrvcsv->driveLocId[j] == ':')
                        {
                            driveNumberFlag = TRUE;
                        }
                        else if (pQdrvcsv->driveLocId[j] <= ' ')
                        {
                            break;
                        }
                        else
                        {
                            if (driveNumberFlag)
                            {
                                driveNumberString[l] = pQdrvcsv->driveLocId[j];
                                l++;
                            }
                            else
                            {
                                vtssNameString[k] = pQdrvcsv->driveLocId[j];
                                k++;
                            }
                        }
                    }
                }

                TRMSGI(TRCI_XAPI,
                       "VTSS=%s, drive=%s\n",
                       vtssNameString,
                       driveNumberString);

                pXapivacs = NULL;

                for (j = 0;
                    j < xapivacsCount;
                    j++)
                {
                    if (strcmp(xapivacs[j].vtssNameString, vtssNameString) == 0)
                    {
                        pXapivacs = &(xapivacs[j]);

                        break;
                    }
                }

                if (pXapivacs == NULL)
                {
                    xapivacsCount++;
                    pXapivacs = &(xapivacs[(xapivacsCount - 1)]);
                    strcpy(pXapivacs->vtssNameString, vtssNameString);
                    pXapivacs->vacsId = (MAX_ACS_NUMBER - xapivacsCount);

                    TRMSGI(TRCI_XAPI,
                           "New XAPIVACS; VTSSNAME=%s, acs=%d\n",
                           pXapivacs->vtssNameString,
                           pXapivacs->vacsId);
                }

                /*****************************************************/
                /* The ACS:LSM:PANEL:DRIVE of a virtual device will  */
                /* be:                                               */
                /* ACS   = dummy acs id or vacsId (counting back     */
                /*         from MAX_ACS_NUMBER                       */
                /* LSM   = 0                                         */
                /* PANEL = 1st 2 digits drive_location_id            */
                /* DRIVE = last 2 digits drive_location_id           */
                /*****************************************************/
                pNextXapicfg->libdrvid.acs = pXapivacs->vacsId;
                pNextXapicfg->libdrvid.lsm = 0;

                FN_CONVERT_DIGITS_TO_FULLWORD(driveNumberString,
                                              2,
                                              &locInt);

                pNextXapicfg->libdrvid.panel = locInt;

                FN_CONVERT_DIGITS_TO_FULLWORD(&(driveNumberString[2]),
                                              2,
                                              &locInt);

                pNextXapicfg->libdrvid.driveNumber = locInt;

                strcpy(pNextXapicfg->vtssNameString, pXapivacs->vtssNameString);
            }
            /*********************************************************/
            /* Otherwise this is a "real" tape device.  Add the real */
            /* device to the XAPICGF drive configuration table.      */
            /*********************************************************/
            else
            {
                /*****************************************************/
                /* First extract the AA:LL:PP:DD elements from       */
                /* the <drive_location_id> which is in the           */
                /* format: "R:AA:LL:PP:DD".                          */
                /*                                                   */
                /* NOTE: We assume a fixed format                    */
                /* <drive_location_id>                               */
                /*****************************************************/
                if (memcmp("R:",
                           pQdrvcsv->driveLocId,
                           2) == 0)
                {
                    memcpy(acs,
                           &(pQdrvcsv->driveLocId[2]),
                           sizeof(acs));

                    if (acs[0] > ' ')
                    {
                        FN_CONVERT_DIGITS_TO_FULLWORD(acs,
                                                      sizeof(acs),
                                                      &locInt);

                        pNextXapicfg->libdrvid.acs = locInt;

                        memcpy(lsm,
                               &(pQdrvcsv->driveLocId[5]),
                               sizeof(lsm));

                        if (lsm[0] > ' ')
                        {
                            FN_CONVERT_DIGITS_TO_FULLWORD(lsm,
                                                          sizeof(lsm),
                                                          &locInt);

                            pNextXapicfg->libdrvid.lsm = locInt;

                            memcpy(panel,
                                   &(pQdrvcsv->driveLocId[8]),
                                   sizeof(panel));

                            if (panel[0] > ' ')
                            {
                                FN_CONVERT_DIGITS_TO_FULLWORD(panel,
                                                              sizeof(panel),
                                                              &locInt);

                                pNextXapicfg->libdrvid.panel = locInt;

                                memcpy(driveNumber,
                                       &(pQdrvcsv->driveLocId[11]),
                                       sizeof(driveNumber));

                                if (driveNumber[0] > ' ')
                                {
                                    FN_CONVERT_DIGITS_TO_FULLWORD(driveNumber,
                                                                  sizeof(driveNumber),
                                                                  &locInt);

                                    pNextXapicfg->libdrvid.driveNumber = locInt;
                                }
                            }
                        }
                    }
                }
            } 
            pNextXapicfg++;
        } /* If matching XAPIDRLST entry */
        pNextUuiTextXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                           pParentXmlelem,
                                                           pNextUuiTextXmlelem,
                                                           XNAME_uui_text);
    } /* for (i) */

    /*****************************************************************/
    /* If we have an existing XAPICFG table, then detach and remove  */
    /* its shared memory segment.                                    */
    /*****************************************************************/
    if (pXapicvt->pXapicfg != NULL)
    {
        lastRC = shmdt((void*) pXapicvt->pXapicfg);

        TRMSGI(TRCI_STORAGE,
               "lastRC=%d from shmdt(addr=XAPICFG=%08X)\n",
               lastRC,
               pXapicvt->pXapicfg);

        lastRC = shmctl(pXapicvt->cfgShMemSegId, IPC_RMID, NULL);

        TRMSGI(TRCI_STORAGE,
               "lastRC=%d from shctl(id-XAPICFG=%d (%08X))\n",
               lastRC,
               pXapicvt->cfgShMemSegId,
               pXapicvt->cfgShMemSegId);

        pXapicvt->cfgShMemSegId = 0;
        pXapicvt->pXapicfg = NULL;
        pXapicvt->xapicfgSize = 0;
        pXapicvt->xapicfgCount = 0;    
    }

    /*****************************************************************/
    /* Create the shared memory segment that will contain the new    */
    /* XAPICFG drive configuration table.                            */
    /*****************************************************************/
    shmKey = XAPI_CFG_SHM_KEY;
    shmSize = xapicfgSize;
    shmPermissions = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    shmFlags = (IPC_CREAT | shmPermissions);

    shMemSegId = shmget(shmKey, shmSize, shmFlags);

    TRMSGI(TRCI_STORAGE,
           "shMemSegId=%d (%08X) after shmget(key=XAPICFG=%d, "
           "size=%d, flags=%d (%08X))\n",
           shMemSegId,
           shMemSegId,
           shmKey,
           shmSize,
           shmFlags,
           shmFlags);

    if (shMemSegId < 0)
    {
        LOGMSG(STATUS_PROCESS_FAILURE, 
               "Could not allocate XAPICFG shared memory segment; errno=%d (%s)",
               errno,
               strerror(errno));

        return STATUS_PROCESS_FAILURE;
    }

    /*****************************************************************/
    /* Attach the shared memory segment to our data space.           */
    /*****************************************************************/
    pNewXapicfg = (struct XAPICFG*) shmat(shMemSegId, NULL, 0);

    TRMSGI(TRCI_STORAGE,
           "pNewXapicfg=%08X after shmat(id=XAPICFG=%d (%08X), NULL, 0)\n",
           pNewXapicfg,
           shMemSegId,
           shMemSegId);

    if (pNewXapicfg == NULL)
    {
        LOGMSG(STATUS_PROCESS_FAILURE, 
               "Could not attach XAPICFG shared memory segment");

        return STATUS_PROCESS_FAILURE;
    }

    memcpy((char*) pNewXapicfg,
           (char*) pWorkXapicfg,
           xapicfgSize);

    pXapicvt->cfgShMemSegId = shMemSegId;
    pXapicvt->pXapicfg = pNewXapicfg;
    pXapicvt->xapicfgSize = xapicfgSize;
    pXapicvt->xapicfgCount = driveCount;
    pXapicvt->updateConfig = FALSE;
    time(&(pXapicvt->configTime));

    TRMEMI(TRCI_XAPI,
           pXapicvt->pXapicfg,
           xapicfgSize,
           "XAPICFG:\n");

    /*****************************************************************/
    /* At this point we can free the work XAPICFG table.             */
    /*****************************************************************/
    TRMSGI(TRCI_STORAGE,
           "free pWorkXapicgf=%08X, len=%i\n",
           pWorkXapicfg,
           xapicfgSize);

    free(pWorkXapicfg);

    return STATUS_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: matchXapidrlstEntry                               */
/** Description:   Match the TapePlex drive to the XAPI_DRIVE_LIST.  */
/**                                                                  */
/** Attempt to match the CSV <query_drive_info> response line        */
/** to an entry in the XAPIDRLST table.  The XAPIDRLST table was     */
/** derived from the XAPI_DRIVE_LIST environment variable.           */
/**                                                                  */
/** STATUS_SUCCESS means that the TapePlex drive was found in        */
/** the XAPIDRLST table (or that there was no XAPI_DRIVE_LIST        */
/** environment variable defined, which means that all drives        */
/** returned in the TapePlex server drive list are available to      */
/** the XAPI client).                                                */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "matchXapidrlstEntry"

static int matchXapidrlstEntry(struct XAPICVT   *pXapicvt,
                               struct XAPIREQE  *pXapireqe,
                               struct QDRVCSV   *pQdrvcsv,
                               struct XAPIDRLST *pXapidrlst,
                               int               numXapidrlst)
{
    int                 i;
    struct XAPIDRLST   *pCurrXapidrlst;

    if ((pXapidrlst == NULL) ||
        (numXapidrlst == 0))
    {
        return STATUS_SUCCESS;
    }

    for (i = 0;
        i < numXapidrlst;
        i++)
    {
        pCurrXapidrlst = &(pXapidrlst[i]);

        if (pCurrXapidrlst->driveName[0] > ' ')
        {
            if (memcmp(pCurrXapidrlst->driveName,
                       pQdrvcsv->driveName,
                       sizeof(pCurrXapidrlst->driveName)) == 0)
            {
                pCurrXapidrlst->configDriveFlag = TRUE;

                return STATUS_SUCCESS;
            }
        }
        else if (pCurrXapidrlst->driveLocId[0] > ' ')
        {
            if (memcmp(pCurrXapidrlst->driveLocId,
                       pQdrvcsv->driveLocId,               
                       strlen(pCurrXapidrlst->driveLocId)) == 0)
            {
                pCurrXapidrlst->configDriveFlag = TRUE;

                return STATUS_SUCCESS;
            }
        }
    }

    TRMSGI(TRCI_XAPI,
           "Config drive=%.4s (%.16s) not in XAPI_DRIVE_LIST\n",
           pQdrvcsv->driveName,
           pQdrvcsv->driveLocId);

    return STATUS_DRIVE_NOT_IN_LIBRARY;
}


