/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_drvtyp.c                                    */
/** Description:    XAPI client drive type table (XAPIDRVTYP)        */
/**                 build service.                                   */
/**                                                                  */
/**                 The XAPIDRVTYP is a table for converting         */
/**                 an XAPI tape drive model name into an ACSAPI     */
/**                 tape drive type code (and vice versa).           */
/**                 Each XAPIDRVTYP table entry also contains a      */
/**                 table of compatible ACSAPI media type codes.     */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     06/29/11                          */
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
static void buildQueryRequest(struct XAPICVT   *pXapicvt,
                              struct XAPIREQE  *pXapireqe,
                              char            **ptrXapiBuffer,
                              int              *pXapiBufferSize);

static int extractQueryResponse(struct XAPICVT  *pXapicvt,
                                struct XAPIREQE *pXapireqe,
                                struct XMLPARSE *pXmlparse);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_drvtyp                                       */
/** Description:   Build the XAPI-ACSAPI drive type conversion table.*/
/**                                                                  */
/** The XAPIDRVTYP table is a conversion table to convert XAPI       */
/** model names into ACSAPI drive type codes (and vice versa).       */
/** In addition the XAPIDRVTYP table entry for a single ACSAPI       */
/** drive type code contains a table of compatible ACSAPI            */
/** media type codes (see the XAPIMEDIA table).  Together, the       */
/** XAPIDRVTYPE and XAPIMEDIA tables are the XAPI equivalent of      */
/** the MVS SRMM tables.                                             */
/**                                                                  */
/** The XAPIDRVTYP information is obtained by creating an            */
/** XAPI XML format <query_drvtypes> request; the XAPI XML request   */
/** is transmitted to the server via TCP/IP; the returned XAPI XML   */
/** response is parsed and the fixed length XAPIDRVTYP table         */
/** data populated.                                                  */
/**                                                                  */
/** The fixed length XAPIDRVTYP and XAPIMEDIA tables are embedded    */
/** within the XAPICVT.                                              */
/**                                                                  */
/** Building the XAPIDRVTYP table is allowed to proceed when         */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_drvtyp"

extern int xapi_drvtyp(struct XAPICVT  *pXapicvt,
                       struct XAPIREQE *pXapireqe)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;

    struct XMLPARSE    *pXmlparse           = NULL;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    buildQueryRequest(pXapicvt,
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
        lastRC = extractQueryResponse(pXapicvt,
                                      pXapireqe,
                                      pXmlparse);

        TRMSGI(TRCI_XAPI,"lastRC=%d from extractQueryResponse\n",
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
/** Function Name: buildQueryRequest                                 */
/** Description:   Build an XAPI <query_drvtypes> request.           */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_drvtypes> request consists of:               */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**                                                                  */
/**   <command>                                                      */
/**     <query_drvtypes>                                             */
/**     </query_drvtypes>                                            */
/**   </command>                                                     */
/** </libtrans>                                                      */
/*                                                                   */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "buildQueryRequest"

static void buildQueryRequest(struct XAPICVT  *pXapicvt,
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
                                      XNAME_query_drvtypes,
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
/** Function Name: extractQueryResponse                              */
/** Description:   Extract the XAPI <drvtype_info> response.         */
/**                                                                  */
/** Parse the output of the XAPI <query_drvtypes> response and       */
/** build the XAPIDRVTYP table.                                      */
/**                                                                  */
/** The XAPIDRVTYP table is a finite size because it is derived      */
/** from the MVS SRMM RMCODE which is 8 bytes (or a maximim of       */
/** 64 bits) in length.  This means there are a maximum of 64        */
/** single bit drive type codes (or model names).  The XAPIDRVTYP    */
/** table is constructed as a fixed length table with 64 entries.    */
/** Because it is fixed length, it is simply embedded within the     */
/** XAPICVT.  When it is updated, the embedded table entries are     */
/** simply overlayed with the response data.                         */
/**                                                                  */
/** We should only update the XAPIDRVTYP table once when xapi_main   */
/** is first entered.                                                */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <drvtype_info> responses consists of:               */
/**==================================================================*/
/** <libreply>                                                       */
/**   <drvtype_info>                                                 */
/**     <model>MMMMMMMM</model>                                      */
/**     <acsapi_value>NNN</acsapi_value>                             */
/**     <media_list>                                                 */
/**       <media>MMMMMMMM</media>                                    */
/**       <acsapi_value>NNN</acsapi_value>                           */
/**       ...repeated <media> and <acsapi_value> entries             */
/**     </media_list>                                                */
/**   </drvtype_info>                                                */
/**   ...repeated <drvtype_info> entries                             */
/** </libreply>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractQueryResponse"

static int extractQueryResponse(struct XAPICVT  *pXapicvt,
                                struct XAPIREQE *pXapireqe,
                                struct XMLPARSE *pXmlparse)
{
    int                 lastRC;
    int                 drvtypCount         = 0;
    int                 mediaCount          = 0;
    int                 wkInt;

    struct XMLELEM     *pDrvtypeXmlelem;
    struct XMLELEM     *pDrvtypeParentXmlelem;
    struct XMLELEM     *pModelXmlelem;
    struct XMLELEM     *pModelAcsapiXmlelem;
    struct XMLELEM     *pMediaListXmlelem;
    struct XMLELEM     *pMediaAcsapiXmlelem;

    char                modelNameString[XAPI_MODEL_NAME_SIZE + 1];
    char                acsapiCodeString[4];

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Run through the <drvtype_info> entries.                       */
    /*****************************************************************/
    pDrvtypeXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                              pXmlparse->pHocXmlelem,
                                              XNAME_drvtype_info);

    drvtypCount = 0;

    if (pDrvtypeXmlelem != NULL)
    {
        pDrvtypeParentXmlelem = pDrvtypeXmlelem->pParentXmlelem;

        while (pDrvtypeXmlelem != NULL)
        {
            /*********************************************************/
            /* Extract the top-level <model> and <acsapi_value>.     */
            /*********************************************************/
            pModelXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                    pDrvtypeXmlelem,
                                                    XNAME_model);

            pModelAcsapiXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                          pDrvtypeXmlelem,
                                                          XNAME_acsapi_value);

            if ((pModelXmlelem != NULL) &&
                (pModelAcsapiXmlelem != NULL))
            {
                memset(modelNameString, 0, sizeof(modelNameString));

                memcpy(modelNameString,
                       pModelXmlelem->pContent,
                       pModelXmlelem->contentLen);

                memset(acsapiCodeString, 0, sizeof(acsapiCodeString));

                memcpy(acsapiCodeString,
                       pModelAcsapiXmlelem->pContent,
                       pModelAcsapiXmlelem->contentLen);

                wkInt = atoi(acsapiCodeString);

                strcpy(pXapicvt->xapidrvtyp[drvtypCount].modelNameString, modelNameString);
                pXapicvt->xapidrvtyp[drvtypCount].acsapiDriveType = wkInt;

                /*********************************************************/
                /* Extract the multiple <media> entries.                 */
                /*********************************************************/
                pMediaListXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                            pDrvtypeXmlelem,
                                                            XNAME_media_list);

                if (pMediaListXmlelem != NULL)
                {
                    mediaCount = 0;

                    pMediaAcsapiXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                                  pMediaListXmlelem,
                                                                  XNAME_acsapi_value);

                    while (pMediaAcsapiXmlelem != NULL)
                    {
                        memset(acsapiCodeString, 0, sizeof(acsapiCodeString));

                        memcpy(acsapiCodeString,
                               pMediaAcsapiXmlelem->pContent,
                               pMediaAcsapiXmlelem->contentLen);

                        wkInt = atoi(acsapiCodeString);
                        pXapicvt->xapidrvtyp[drvtypCount].compatMediaType[mediaCount] = wkInt;
                        mediaCount++;

                        pMediaAcsapiXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                                           pMediaListXmlelem,
                                                                           pMediaAcsapiXmlelem,
                                                                           XNAME_acsapi_value);

                        if ((mediaCount >= MAX_COMPATMEDIA) &&
                            (pMediaAcsapiXmlelem != NULL))
                        {
                            TRMSGI(TRCI_XAPI,
                                   "MAX_COMPATMEDIA=%d exceeded; XAPIDRVTYP info incomplete\n",
                                   MAX_COMPATMEDIA);

                            pMediaAcsapiXmlelem = NULL;
                        }
                    } /* while next <media_list> <acsapi_value> found */

                    pXapicvt->xapidrvtyp[drvtypCount].numCompatMedias = mediaCount;

                    TRMSGI(TRCI_XAPI,
                           "For XAPIDRVTYP[%d]; model=%s (%d), numCompatMedias=%d\n",
                           drvtypCount,
                           modelNameString,
                           pXapicvt->xapidrvtyp[drvtypCount].acsapiDriveType,
                           pXapicvt->xapidrvtyp[drvtypCount].numCompatMedias);
                } /* if <drvtype_info> <media_list> found */
            } /* if <drvtupe_info> <model> and <acsapi_info> found */

            drvtypCount++;

            pDrvtypeXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                           pDrvtypeParentXmlelem,
                                                           pDrvtypeXmlelem,
                                                           XNAME_drvtype_info);

            if ((drvtypCount >= MAX_XAPIDRVTYP) &&
                (pDrvtypeXmlelem != NULL))
            {
                TRMSGI(TRCI_XAPI,
                       "MAX_XAPIDRVTYP=%d exceeded; XAPIDRVTYP info incomplete\n",
                       MAX_XAPIDRVTYP);

                pDrvtypeXmlelem = NULL;
            }
        } /* while next <drvtype_info> found */
    }
    else
    {
        TRMSGI(TRCI_XAPI,
               "<drvtype_info> element not found\n");

        return STATUS_NI_FAILURE;
    }

    pXapicvt->xapidrvtypCount = drvtypCount;

    TRMEMI(TRCI_XAPI, 
           &(pXapicvt->xapidrvtyp[0]), sizeof(pXapicvt->xapidrvtyp),
           "xapidrvtypCount=%d; XAPIDRVTYP:\n",
           pXapicvt->xapidrvtypCount);

    return STATUS_SUCCESS;
}



