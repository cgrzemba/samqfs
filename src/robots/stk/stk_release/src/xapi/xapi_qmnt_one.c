/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qmnt_one.c                                  */
/** Description:    XAPI client query mount scratch service.         */
/**                                                                  */
/**                 Generate a drive list for the specified POOLID   */
/**                 (subpool index), and optional media type code,   */
/**                 and management class name.                       */
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
#define QUERY_RECV_TIMEOUT_NON1ST  600


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int buildQscrmntRequest(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char            *subpoolNameString,
                               char            *mediaNameString,
                               char            *managementClassString,
                               char           **ptrXapiBuffer,
                               int             *pXapiBufferSize);

static int extractQscrmntResponse(struct XAPICVT   *pXapicvt,
                                  struct XAPIREQE  *pXapireqe,
                                  struct XMLPARSE  *pXmlparse,
                                  struct RAWSCRMNT *pRawscrmnt,
                                  struct RAWDRLST  *pRawdrlst);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qmnt_one                                     */
/** Description:   XAPI client query mount scratch service.          */
/**                                                                  */
/** Generate a drive list of eligible drives for the specified       */
/** POOLID (subpool index), and optional media, and management       */
/** class.                                                           */
/**                                                                  */
/** Search the XAPISCRPOOL table to translate the ACSAPI POOLID      */
/** into the XAPI subpool name; Search the XAPIMEDIA table to        */
/** translate the ACSAPI media type code into the XAPI media name;   */
/** Create an XAPI XML format <query_scr_mnt_info> request for the   */
/** input scratch subpool, and optional media name, and management   */
/** class name.  Transmit the XAPI XML request to the server via     */
/** TCP/IP;  The received XAPI XML response is then translated into  */
/** the RAWSCRMNT and RAWDRLST structures provided by the caller.    */
/**                                                                  */
/** The XAPI client query mount scratch service is allowed to        */
/** proceed even when the XAPI client is in the IDLE state.          */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qmnt_one"

extern int xapi_qmnt_one(struct XAPICVT   *pXapicvt,
                         struct XAPIREQE  *pXapireqe,
                         struct RAWSCRMNT *pRawscrmnt,
                         struct RAWDRLST  *pRawdrlst,
                         char             *subpoolNameString,
                         char             *mediaNameString,
                         char             *managementClassString)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    TRMSGI(TRCI_XAPI, 
           "Entered; subpoolName=%s, mediaNameString=%08X, "
           "managementClassString=%08X\n",
           subpoolNameString,
           mediaNameString,
           managementClassString);

    lastRC = buildQscrmntRequest(pXapicvt,
                                 pXapireqe,
                                 subpoolNameString,
                                 mediaNameString,
                                 managementClassString,
                                 &pXapiBuffer,
                                 &xapiBufferSize);

    TRMSGI(TRCI_XAPI, 
           "lastRC=%d from buildQscrmntRequest(pBuffer=%08X, size=%d)\n",
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
    /* Now extract the drive list data.                              */
    /*****************************************************************/
    if (queryRC == STATUS_SUCCESS)
    {
        lastRC = extractQscrmntResponse(pXapicvt,
                                        pXapireqe,
                                        pXmlparse,
                                        pRawscrmnt,
                                        pRawdrlst);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractQscrmntResponse\n",
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
/** Function Name: buildQscrmntRequest                               */
/** Description:   Build the XAPI <query_scr_mnt_info> request.      */
/**                                                                  */
/** Build an XAPI <query_scr_mnt_info> request for a single          */
/** input subpool name, optional media name, and optional management */
/** class.                                                           */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_scr_mnt_info> request consists of:           */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <query_scr_mnt_info>                                         */
/**       <subpool_name>sssssssssssss</subpool_name>                 */
/**                                                                  */
/**       <media_list>                                               */
/**         <media>mmmmmmmm</media>                        OR        */
/**       </media_list>                                              */
/**       <model_list>                                               */
/**         <model>mmmmmmmm</model>                        OR        */
/**       </model_list>                                              */
/**       <rectech_list>                                             */
/**         <rectech>rrrrrrrr</rectech>                    OR        */
/**       </rectech_list>                                            */
/**                                                                  */
/**       <label_type>SL|AL|NL</label_type>                (N/A CDK) */
/**       <management_class>mmmmmmmm</management_class>              */
/**                                                                  */
/**       <drive_info_format>DRIVE</drive_info_format>               */
/**       <max_drive_count>165</max_drive_count>                     */
/**                                                                  */
/**     </query_scr_mnt_info>                                        */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** NOTES:                                                           */
/** (1) <max_drive_count> of 165 (XAPI_MAX_DRLST_COUNT) should be    */
/**     same value as ACSAPI constant QU_MAX_DRV_STATUS.             */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "buildQscrmntRequest"

static int buildQscrmntRequest(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char            *subpoolNameString,
                               char            *mediaNameString,
                               char            *managementClassString,
                               char           **ptrXapiBuffer,
                               int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;

    char               *pXapiRequest        = NULL;
    char                maxDriveCountString[4];

    struct XMLPARSE    *pXmlparse; 
    struct XMLELEM     *pQueryMntScrXmlelem;
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    TRMSGI(TRCI_XAPI, 
           "Entered; subpoolName=%s, mediaNameString=%08X, "
           "managementClassString=%08X\n",
           subpoolNameString,
           mediaNameString,
           managementClassString);

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
                                      XNAME_query_scr_mnt_info,
                                      NULL,
                                      0);

    pQueryMntScrXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pQueryMntScrXmlelem,
                                      XNAME_drive_info_format,
                                      "DRIVE",
                                      0);

    sprintf(maxDriveCountString,
            "%d",
            XAPI_MAX_DRLST_COUNT);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pQueryMntScrXmlelem,
                                      XNAME_max_drive_count,
                                      maxDriveCountString,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pQueryMntScrXmlelem,
                                      XNAME_subpool_name,
                                      subpoolNameString,
                                      0);

    if (managementClassString != NULL)
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pQueryMntScrXmlelem,
                                          XNAME_management_class,
                                          managementClassString,
                                          0);
    }

    if (mediaNameString != NULL)
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pQueryMntScrXmlelem,
                                          XNAME_media_list,
                                          NULL,
                                          0);

        pParentXmlelem = pXmlparse->pCurrXmlelem;

        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_media,
                                          mediaNameString,
                                          0);
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
/** Function Name: extractQscrmntResponse                            */
/** Description:   Extract the <query_scr_mnt_info> response.        */
/**                                                                  */
/** Parse the response of the XAPI <query_scr_mnt_info>              */
/** request and update the appropriate fields of the                 */
/** RAWSCRMNT and RAWDRLST structure.                                */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_scratch_mount_request> responses consists of:*/
/**==================================================================*/
/** <libreply>                                                       */
/**   <query_scratch_mount_request>                                  */
/**     <header>                                                     */
/**       header element children                                    */
/**     </header>                                                    */
/**     <scratch_data>                                               */
/**       <invalid_management_class_flag>                            */
/**         YES|NO                                                   */
/**       </invalid_management_class_flag>                           */
/**       <no_library_scratch_flag>                                  */
/**         YES|NO                                                   */
/**       </no_library_scratch_flag>                                 */
/**       <invalid_subpool_name_flag>                                */
/**         YES|NO                                                   */
/**       </invalid_subpool_name_flag>                               */
/**       <no_scratch_for_lbltype_flag>                              */
/**         YES|NO                                                   */
/**       </no_scratch_for_lbltype_flag>                             */
/**       <media_list>                                               */
/**         <media>mmmmmmmm</media>                                  */
/**         ...repeated <media> entries                              */
/**       </media_list>                                              */
/**       <model_list>                                               */
/**         <model>mmmmmmmm</model>                                  */
/**         ...repeated <model> entries                              */
/**       </model_list>                                              */
/**     </scratch_data>                                              */
/**                                                                  */
/**     <drive_group_info>                                           */
/**       <read_rectech_list>                                        */
/**         <rectech>rrrrrrrr</rectech>                              */
/**         ...repeated <rectech> entries                            */
/**       </read_rectech_list>                                       */
/**       <write_rectech_list>                                       */
/**         <rectech>rrrrrrrr</rectech>                              */
/**         ...repeated <rectech> entries                            */
/**       </write_rectech_list>                                      */
/**       <append_rectech_list>                                      */
/**         <rectech>rrrrrrrr</rectech>                              */
/**         ...repeated <rectech> entries                            */
/**       </append_rectech_list>                                     */
/**     </drive_group_info>                                          */
/**     ...repeated <drive_group_info> entries                       */
/**                                                                  */
/**     if <drive_info_format> specified:                            */
/**                                                                  */
/**     <drive_info>                                                 */
/**       <drive_data>                                               */
/**         <drive_name>ccua</drive_name>                            */
/**         <drive_location_id>aa:ll:pp:dd</drive_location_id>       */
/**         <drive_group_location>ccc...ccc</drive_group_location>   */
/**         <state>sssssssss</state>                                 */
/**         <model>mmmmmmmm<model>                                   */
/**         <preference_order>nnnn</preference_order>                */
/**         <scratch_count>nnnn</scratch_count>                      */
/**       </drive_data>                                              */
/**       ...repeated <drive_data> entries                           */
/**     </drive_info>                                                */
/**                                                                  */
/**     <exceptions>                                                 */
/**       <reason>ccc...ccc</reason>                                 */
/**       ...repeated <reason> entries                               */
/**     </exceptions>                                                */
/**     <uui_return_code>nnnn</uui_return_code>                      */
/**     <uui_reason_code>nnnn</uui_reason_code>                      */
/**   </query_scratch_mount_request>                                 */
/** </libreply>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractQscrmntResponse"

static int extractQscrmntResponse(struct XAPICVT   *pXapicvt,
                                  struct XAPIREQE  *pXapireqe,
                                  struct XMLPARSE  *pXmlparse,
                                  struct RAWSCRMNT *pRawscrmnt,
                                  struct RAWDRLST  *pRawdrlst)
{
    int                 extractRC           = STATUS_SUCCESS;
    int                 lastRC;
    int                 driveCount          = 0;
    int                 traceSize;
    int                 elementCount;

    struct XMLELEM     *pScratchDataXmlelem;
    struct XMLELEM     *pListXmlelem;
    struct XMLELEM     *pListElementXmlelem;
    struct XMLELEM     *pDriveInfoXmlelem;
    struct XMLELEM     *pDriveDataXmlelem;
    struct XMLELEM     *pParentXmlelem;

    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;
    struct RAWDRONE     rawdrone;
    struct RAWDRONE    *pRawdrone           = &rawdrone;

    struct XMLSTRUCT    scratchDataXmlstruct[] =
    {
        XNAME_scratch_data,                 XNAME_invalid_management_class_flag,
        sizeof(pRawscrmnt->invalidMgmt),    pRawscrmnt->invalidMgmt,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_scratch_data,                 XNAME_no_library_scratch_flag,
        sizeof(pRawscrmnt->noLibScratch),   pRawscrmnt->noLibScratch,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_scratch_data,                 XNAME_invalid_subpool_name_flag,
        sizeof(pRawscrmnt->invalidSubpool), pRawscrmnt->invalidSubpool,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_scratch_data,                 XNAME_no_scratch_for_lbltype_flag,
        sizeof(pRawscrmnt->noLblScratch),   pRawscrmnt->noLblScratch,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 scratchDataElementCount = sizeof(scratchDataXmlstruct) / 
                                                  sizeof(struct XMLSTRUCT);

    struct XMLSTRUCT    driveDataXmlstruct[] =
    {
        XNAME_drive_data,                   XNAME_drive_name,
        sizeof(pRawdrone->driveName),       pRawdrone->driveName,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_data,                   XNAME_drive_location_id,
        sizeof(pRawdrone->driveLocId),      pRawdrone->driveLocId,
        BLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_data,                   XNAME_model,
        sizeof(pRawdrone->model),           pRawdrone->model,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_data,                   XNAME_state,
        sizeof(pRawdrone->state),           pRawdrone->state,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_data,                   XNAME_preference_order,
        sizeof(pRawdrone->prefOrder),       pRawdrone->prefOrder,
        ZEROFILL, NOBITVALUE, 0, 0,
        XNAME_drive_data,                   XNAME_scratch_count,
        sizeof(pRawdrone->scratchCount),    pRawdrone->scratchCount,
        ZEROFILL, NOBITVALUE, 0, 0,
    };

    int                 driveDataElementCount = sizeof(driveDataXmlstruct) / 
                                                sizeof(struct XMLSTRUCT);


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

    pScratchDataXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                  pXmlparse->pHocXmlelem,
                                                  XNAME_scratch_data);

    TRMSGI(TRCI_XAPI, 
           "pScratchDataXmlelem=%08X\n",
           pScratchDataXmlelem);

    if (pScratchDataXmlelem == NULL)
    {
        return STATUS_NI_FAILURE;
    }

    memset(pRawscrmnt, 0, sizeof(struct RAWSCRMNT));

    FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                      pScratchDataXmlelem,
                                      &scratchDataXmlstruct[0],
                                      scratchDataElementCount);

    if (pRawscrmnt->invalidMgmt[0] == 'Y')
    {
        extractRC == STATUS_MGMTCLAS_NOT_FOUND;
    }
    else if ((pRawscrmnt->noLibScratch[0] == 'Y') ||
             (pRawscrmnt->noLblScratch[0] == 'Y'))
    {
        extractRC == STATUS_SCRATCH_NOT_AVAILABLE;
    }
    else if (pRawscrmnt->invalidSubpool[0] == 'Y')
    {
        extractRC == STATUS_POOL_NOT_FOUND;
    }

    pListXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                           pScratchDataXmlelem,
                                           XNAME_media_list);

    TRMSGI(TRCI_XAPI,
           "pMediaListXmlelem=%08X\n",
           pListXmlelem);

    if (pListXmlelem != NULL)
    {
        pListElementXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                      pListXmlelem,
                                                      XNAME_media);

        elementCount = 0;

        while (pListElementXmlelem != NULL)
        {
            elementCount++;

            if (elementCount > 10)
            {
                TRMSGI(TRCI_XAPI,
                       "> 10 media names in list\n");

                break;
            }

            memcpy(pRawscrmnt->mediaString[(elementCount - 1)], 
                   pListElementXmlelem->pContent,
                   pListElementXmlelem->contentLen);

            pListElementXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                               pListXmlelem,
                                                               pListElementXmlelem,
                                                               XNAME_media);
        }
    }

    pListXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                           pScratchDataXmlelem,
                                           XNAME_model_list);

    TRMSGI(TRCI_XAPI,
           "pModelListXmlelem=%08X\n",
           pListXmlelem);

    if (pListXmlelem != NULL)
    {
        pListElementXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                      pListXmlelem,
                                                      XNAME_model);

        elementCount = 0;

        while (pListElementXmlelem != NULL)
        {
            elementCount++;

            if (elementCount > 10)
            {
                TRMSGI(TRCI_XAPI,
                       "> 10 model names in list\n");

                break;
            }

            memcpy(pRawscrmnt->modelString[(elementCount - 1)], 
                   pListElementXmlelem->pContent,
                   pListElementXmlelem->contentLen);

            pListElementXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                               pListXmlelem,
                                                               pListElementXmlelem,
                                                               XNAME_model);
        }
    }

    TRMEMI(TRCI_XAPI,
           pRawscrmnt, sizeof(struct RAWSCRMNT),
           "RAWSCRMNT:\n");

    memset(pRawdrlst, 0, sizeof(struct RAWDRLST));

    pDriveInfoXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                pXmlparse->pHocXmlelem,
                                                XNAME_drive_info);

    TRMSGI(TRCI_XAPI,
           "pDriveInfoXmlelem=%08X\n",
           pDriveInfoXmlelem);

    if (pDriveInfoXmlelem != NULL)
    {
        pDriveDataXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                    pDriveInfoXmlelem,
                                                    XNAME_drive_data);

        while (pDriveDataXmlelem != NULL)
        {
            memset(pRawdrone, 0, sizeof(struct RAWDRONE));

            FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                              pDriveDataXmlelem,
                                              &driveDataXmlstruct[0],
                                              driveDataElementCount);

            memcpy((char*) &(pRawdrlst->rawdrone[driveCount]),
                   (char*) pRawdrone,
                   sizeof(struct RAWDRONE));

            driveCount++;

            pDriveDataXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                             pDriveInfoXmlelem,
                                                             pDriveDataXmlelem,
                                                             XNAME_drive_data);
        }

        pRawdrlst->driveCount = driveCount;
    }
    else
    {
        if (extractRC == STATUS_SUCCESS)
        {
            extractRC = STATUS_NO_DRIVES_FOUND;
        }
    }

    traceSize = (sizeof(struct RAWDRONE) * driveCount) + 
                offsetof(struct RAWDRLST, rawdrone[0]);

    TRMEMI(TRCI_XAPI,
           pRawdrlst, traceSize,
           "extractRC=%d, driveCount=%d; RAWDRLST:\n",
           extractRC,
           driveCount);

    return extractRC;
}



