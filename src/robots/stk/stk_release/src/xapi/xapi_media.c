/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_media.c                                     */
/** Description:    XAPI client media type table (XAPIMEDIA)         */
/**                 build service.                                   */
/**                                                                  */
/**                 The XAPIMEDIA is a table for converting an       */
/**                 XAPI media name into an ACSAPI media type        */
/**                 code (and vice versa).  Each XAPIMEDIA table     */
/**                 entry also contains a table of compatible        */
/**                 ACSAPI tape drive type codes.                    */
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
/** Function Name: xapi_media                                        */
/** Description:   Build the XAPI-ACSAPI media type conversion table.*/
/**                                                                  */
/** The XAPIMEDIA table is a conversion table to convert XAPI        */
/** media names into ACSAPI media type codes (and vice versa).       */
/** In addition the XAPIMEDIA table entry for a single ACSAPI        */
/** media type code contains a table of compatible ACSAPI            */
/** drive type codes (see the XAPIDRVTYP table).  Together, the      */
/** XAPIMEDIA and XAPIDRVTYP tables are the XAPI equivalent of       */
/** the MVS SRMM tables.                                             */
/**                                                                  */
/** The XAPIMEDIA information is obtained by creating an             */
/** XAPI XML format <query_media> request; the XAPI XML request      */
/** is transmitted to the server via TCP/IP; the returned XAPI XML   */
/** response is parsed and the fixed length XAPIMEDIA table          */
/** data populated.                                                  */
/**                                                                  */
/** The fixed length XAPIMEDIA and XAPIDRVTYP tables are embedded    */
/** within the XAPICVT.                                              */
/**                                                                  */
/** Building the XAPIMEDIA table is allowed to proceed when          */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_media"

extern int xapi_media(struct XAPICVT  *pXapicvt,
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
/** Description:   Build an XAPI <query_media> request.              */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_media> request consists of:                  */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**                                                                  */
/**   <command>                                                      */
/**     <query_media>                                                */
/**     </query_media>                                               */
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
                                      XNAME_query_media,
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
/** Description:   Extract the XAPI <media_info> response.           */
/**                                                                  */
/** Parse the output of the XAPI <query_media> response and          */
/** build the XAPIMEDIA table.                                       */
/**                                                                  */
/** The XAPIMEDIA table is a finite size because it is derived       */
/** from the MVS SRMM RMCODE which is 8 bytes (or a maximim of       */
/** 64 bits) in length.  This means there are a maximum of 64        */
/** single bit media type codes (or media names).  The XAPIMEDIA     */
/** table is constructed as a fixed length table with 64 entries.    */
/** Because it is fixed length, it is simply embedded within the     */
/** XAPICVT.  When it is updated, the embedded table entries are     */
/** simply overlayed with the response data.                         */
/**                                                                  */
/** We should only update the XAPIMEDIA table once when xapi_main    */
/** is first entered.                                                */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <media_info> responses consists of:                 */
/**==================================================================*/
/** <libreply>                                                       */
/**   <media_info>                                                   */
/**     <media>MMMMMMMM</media>                                      */
/**     <acsapi_value>NNN</acsapi_value>                             */
/**     <model_list>                                                 */
/**       <model>MMMMMMMM</model>                                    */
/**       <acsapi_value>NNN</acsapi_value>                           */
/**       ...repeated <model> and <acsapi_value> entries             */
/**     </model_list>                                                */
/**   </media_info>                                                  */
/**   ...repeated <media_info> entries                               */
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
    int                 driveCount          = 0;
    int                 mediaCount          = 0;
    int                 wkInt;

    struct XMLELEM     *pMediaInfoXmlelem;
    struct XMLELEM     *pMediaInfoParentXmlelem;
    struct XMLELEM     *pMediaXmlelem;
    struct XMLELEM     *pMediaAcsapiXmlelem;
    struct XMLELEM     *pCleanerXmlelem;
    struct XMLELEM     *pModelListXmlelem;
    struct XMLELEM     *pModelAcsapiXmlelem;

    char                mediaNameString[XAPI_MEDIA_NAME_SIZE + 1];
    char                acsapiCodeString[4];
    char                cleanerString[14];

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    /*****************************************************************/
    /* Run through the <media_info> entries.                         */
    /*****************************************************************/
    pMediaInfoXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                pXmlparse->pHocXmlelem,
                                                XNAME_media_info);

    mediaCount = 0;

    if (pMediaInfoXmlelem != NULL)
    {
        pMediaInfoParentXmlelem = pMediaInfoXmlelem->pParentXmlelem;

        while (pMediaInfoXmlelem != NULL)
        {
            /*********************************************************/
            /* Extract the top-level <media>, <acsapi_value>,        */
            /* and <cleaner_media_indicator> content.                */
            /*********************************************************/
            pMediaXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                    pMediaInfoXmlelem,
                                                    XNAME_media);

            pMediaAcsapiXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                          pMediaInfoXmlelem,
                                                          XNAME_acsapi_value);

            if ((pMediaXmlelem != NULL) &&
                (pMediaAcsapiXmlelem != NULL))
            {
                memset(mediaNameString, 0, sizeof(mediaNameString));

                memcpy(mediaNameString,
                       pMediaXmlelem->pContent,
                       pMediaXmlelem->contentLen);

                memset(acsapiCodeString, 0, sizeof(acsapiCodeString));

                memcpy(acsapiCodeString,
                       pMediaAcsapiXmlelem->pContent,
                       pMediaAcsapiXmlelem->contentLen);

                wkInt = atoi(acsapiCodeString);

                strcpy(pXapicvt->xapimedia[mediaCount].mediaNameString, mediaNameString);
                pXapicvt->xapimedia[mediaCount].acsapiMediaType = wkInt;

                pCleanerXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                          pMediaInfoXmlelem,
                                                          XNAME_cleaner_media_indicator);

                if (pCleanerXmlelem != NULL)
                {
                    memset(cleanerString, 0, sizeof(cleanerString));

                    memcpy(cleanerString,
                           pCleanerXmlelem->pContent,
                           pCleanerXmlelem->contentLen);

                    if (cleanerString[0] == 'Y')
                    {
                        pXapicvt->xapimedia[mediaCount].cleanerMediaFlag = 
                        CLN_CART_ALWAYS;
                    }
                    else if (cleanerString[0] == 'N')
                    {
                        pXapicvt->xapimedia[mediaCount].cleanerMediaFlag = 
                        CLN_CART_NEVER;
                    }
                    else
                    {
                        pXapicvt->xapimedia[mediaCount].cleanerMediaFlag = 
                        CLN_CART_INDETERMINATE;
                    }
                }

                /*********************************************************/
                /* Extract the multiple <model> entries.                 */
                /*********************************************************/
                pModelListXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                            pMediaInfoXmlelem,
                                                            XNAME_model_list);

                if (pModelListXmlelem != NULL)
                {
                    driveCount = 0;

                    pModelAcsapiXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                                  pModelListXmlelem,
                                                                  XNAME_acsapi_value);

                    while (pModelAcsapiXmlelem != NULL)
                    {
                        memset(acsapiCodeString, 0, sizeof(acsapiCodeString));

                        memcpy(acsapiCodeString,
                               pModelAcsapiXmlelem->pContent,
                               pModelAcsapiXmlelem->contentLen);

                        wkInt = atoi(acsapiCodeString);
                        pXapicvt->xapimedia[mediaCount].compatDriveType[driveCount] = wkInt;
                        driveCount++;

                        pModelAcsapiXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                                           pModelListXmlelem,
                                                                           pModelAcsapiXmlelem,
                                                                           XNAME_acsapi_value);

                        if ((driveCount >= MAX_COMPATDRIVE) &&
                            (pModelAcsapiXmlelem != NULL))
                        {
                            TRMSGI(TRCI_XAPI,
                                   "MAX_COMPATDRIVE=%d exceeded; media info incomplete\n",
                                   MAX_COMPATDRIVE);

                            pModelAcsapiXmlelem = NULL;
                        }
                    } /* while next <model_list> <acsapi_value> found */

                    pXapicvt->xapimedia[mediaCount].numCompatDrives = driveCount;

                    TRMSGI(TRCI_XAPI,
                           "For media[%d]; media=%s (%d), numCompatDrives=%d\n",
                           mediaCount,
                           mediaNameString,
                           pXapicvt->xapimedia[mediaCount].acsapiMediaType,
                           pXapicvt->xapimedia[mediaCount].numCompatDrives);
                } /* if <media_info> <model_list> found */
            } /* if <media_info> <media> and <acsapi_info> found */

            mediaCount++;

            pMediaInfoXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                             pMediaInfoParentXmlelem,
                                                             pMediaInfoXmlelem,
                                                             XNAME_media_info);

            if ((mediaCount >= MAX_XAPIMEDIA) &&
                (pMediaInfoXmlelem != NULL))
            {
                TRMSGI(TRCI_XAPI,
                       "MAX_XAPIMEDIA=%d exceeded; media info incomplete\n",
                       MAX_XAPIMEDIA);

                pMediaInfoXmlelem = NULL;
            }
        } /* while next <media_info> found */
    }
    else
    {
        TRMSGI(TRCI_XAPI,
               "<media_info> element not found\n");

        return STATUS_NI_FAILURE;
    }

    pXapicvt->xapimediaCount = mediaCount;

    TRMEMI(TRCI_XAPI, 
           &(pXapicvt->xapimedia[0]), sizeof(pXapicvt->xapimedia),
           "xapimediaCount=%d; xapimedia:\n",
           pXapicvt->xapimediaCount);

    return STATUS_SUCCESS;
}



