/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qvol_one.c                                  */
/** Description:    XAPI client query volume service.                */
/**                                                                  */
/**                 Return tape volume information and optional      */
/**                 drive list for the single input volser.          */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     06/21/11                          */
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
static int buildQvolumeRequest(struct XAPICVT   *pXapicvt,
                               struct XAPIREQE  *pXapireqe,
                               struct RAWDRLST  *pRawdrlst,
                               char              volser[6],
                               char            **ptrXapiBuffer,
                               int              *pXapiBufferSize);

static int extractQvolumeResponse(struct XAPICVT   *pXapicvt,
                                  struct XAPIREQE  *pXapireqe,
                                  struct XMLPARSE  *pXmlparse,
                                  struct RAWVOLUME *pRawvolume,
                                  struct RAWDRLST  *pRawdrlst);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qvol_one                                     */
/** Description:   XAPI client query volume service.                 */
/**                                                                  */
/** Return tape volume information and optional drive list for       */
/** the single input volser.                                         */
/**                                                                  */
/** Build an XAPI XML format <query_volume_info> request for the     */
/** single input volser;  If called with an non-NULL RAWDRLST        */
/** parameter, then setup the XAPI <query_volume_info> request       */
/** to also return a drive list; The XAPI XML request is then        */
/** transmitted to the server via TCP/IP;  The received XAPI XML     */
/** response is then translated into RAWVOLUME and optional          */
/** RAWDRLST structures for the caller.                              */
/**                                                                  */
/** The XAPI client query volume service is allowed to proceed       */
/** even when the XAPI client is in the IDLE state.                  */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qvol_one"

extern int xapi_qvol_one(struct XAPICVT   *pXapicvt,
                         struct XAPIREQE  *pXapireqe,
                         struct RAWVOLUME *pRawvolume,
                         struct RAWDRLST  *pRawdrlst,
                         char              volser[6])
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    TRMSGI(TRCI_XAPI, 
           "Entered; volser=%.6s\n",
           volser);

    lastRC = buildQvolumeRequest(pXapicvt,
                                 pXapireqe,
                                 pRawdrlst,
                                 volser,
                                 &pXapiBuffer,
                                 &xapiBufferSize);

    TRMSGI(TRCI_XAPI, 
           "lastRC=%d from buildQvolumeRequest(pBuffer=%08X, size=%d)\n",
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
    /* Now extract the volume data.                                  */
    /*****************************************************************/
    if (queryRC == STATUS_SUCCESS)
    {
        queryRC = extractQvolumeResponse(pXapicvt,
                                         pXapireqe,
                                         pXmlparse,
                                         pRawvolume,
                                         pRawdrlst);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractQvolumeResponse\n",
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
/** Function Name: buildQvolumeRequest                               */
/** Description:   Build an XAPI <query_volume_info> request.        */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_volume_info> request consists of:            */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <query_volume_info>                                          */
/**       <user_name>hhhhhhhh</user_name>                  N/A (CDK) */
/**       <host_name>hhhhhhhh</host_name>                  N/A (CDK) */
/**                                                                  */
/**     For single volser:                                           */
/**                                                                  */
/**       <volume_list>                                              */
/**         <volser>vvvvvv</volser>                        OR        */
/**       </volume_list>                                             */
/**                                                                  */
/**     If *pRawdrlst != NULL, then request drive list information:  */
/**                                                                  */
/**       <drive_info_format>DRIVE</drive_info_format>               */
/**       <max_drive_count>165</max_drive_count>                     */
/**                                                                  */
/**     </query_volume_info>                                         */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** NOTES:                                                           */
/** (1) <max_drive_count> of 165 (XAPI_MAX_DRLST_COUNT) should be    */
/**     same value as ACSAPI constant QU_MAX_DRV_STATUS.             */
/** (2) The xapi_hostname and unix userid specifications             */
/**     are not included and will not be used by the HSC server to   */
/**     control access to specific volsers (which is allowed using   */
/**     the HSC VOLPARMS feature).                                   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "buildQvolumeRequest"

static int buildQvolumeRequest(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               struct RAWDRLST *pRawdrlst,
                               char             volser[6],
                               char           **ptrXapiBuffer,
                               int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;

    char               *pXapiRequest        = NULL;
    char                maxDriveCountString[4];

    struct XMLPARSE    *pXmlparse; 
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    TRMSGI(TRCI_XAPI,
           "Entered; volser=%.6s, pRawdrlst=%08X\n",
           volser,
           pRawdrlst);

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
                                      XNAME_query_volume_info,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    if (pRawdrlst != NULL)
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_drive_info_format,
                                          "DRIVE",
                                          0);

        sprintf(maxDriveCountString,
                "%d",
                XAPI_MAX_DRLST_COUNT);


        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pParentXmlelem,
                                          XNAME_max_drive_count,
                                          maxDriveCountString,
                                          0);
    }

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_volume_list,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_volser,
                                      volser,
                                      6);

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
/** Function Name: extractQvolumeResponse                            */
/** Description:   Extract the <query_volume_info_request> response. */
/**                                                                  */
/** Parse the response of the XAPI XML <query_volume_info> request   */
/** and update the appropriate fields of the RAWVOLUME and           */
/** RAWDRLST structures.                                             */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_volume_info_request> responses consists of:  */
/**==================================================================*/
/** <libreply>                                                       */
/**   <query_volume_info_request>                                    */
/**     <header>                                                     */
/**       header element children                                    */
/**     </header>                                                    */
/**     <volume_data>                                                */
/**       <volser>vvvvvv</volser>                                    */
/**       <media>mmmmmmmm</media>                                    */
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
/**       <home_cell>aa:ll:pp:rr:cc</home_cell>                      */
/**       <library_address>                                          */
/**         <acs>aa</acs>                                            */
/**         <lsm>ll</lsm>                                            */
/**         <panel>pp</panel>                                        */
/**         <row>rr</row>                                            */
/**         <column>cc</column>                                      */
/**       </library_address>                                         */
/**       <resident_vtss>vvvvvvvv</resident_vtss>                    */
/**       <density>rrrrrrrr</density>                                */
/**       <encrypted>"YES"|"NO"</encrypted>                          */
/**       <scratch>"YES"|"NO"</scratch>                              */
/**       <pool_type>"SCRATCH"|"MVC"|"EXTERNAL"</pool_type>          */
/**       <subpool_name>sssssssssssss</subpool_name>                 */
/**       <mount_data>                                               */
/**         <drive_name>ccua</drive_name>                            */
/**         <drive_location_id>aa:ll:pp:dd:zzzz</drive_location_id>  */
/**         <drive_library_address>                                  */
/**           <acs>aa</acs>                                          */
/**           <lsm>ll</lsm>                                          */
/**           <panel>pp</panel>                                      */
/**           <drive_number>dd</drive_number>                        */
/**         </drive_library_address>                                 */
/**         <drive_location_zone>zzzz</drive_location_zone>          */
/**       <mount_data>                                               */
/**     </volume_data>                                               */
/**     ...repeated <volume_data> entries                            */
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
/**     if <drive_info_format> (for drive list) specified:           */
/**                                                                  */
/**     <drive_info>                                                 */
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
/**       <drive_data>                                               */
/**         <drive_name>ccua</drive_name>                            */
/**         <drive_location_id>aa:ll:pp:dd</drive_location_id>       */
/**         <drive_group_location>ccc...ccc</drive_group_location>   */
/**         <rectech>rrrrrrrr<rectech>                               */
/**         <preference_order>nnnn</preference_order>                */
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
/**   </query_volume_info_request>                                   */
/** </libreply>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractQvolumeResponse"

static int extractQvolumeResponse(struct XAPICVT   *pXapicvt,
                                  struct XAPIREQE  *pXapireqe,
                                  struct XMLPARSE  *pXmlparse,
                                  struct RAWVOLUME *pRawvolume,
                                  struct RAWDRLST  *pRawdrlst)
{
    int                 lastRC;
    int                 driveCount          = 0;
    int                 traceSize;

    struct XMLELEM     *pVolumeDataXmlelem;
    struct XMLELEM     *pLibAddrXmlelem;
    struct XMLELEM     *pMountDataXmlelem;
    struct XMLELEM     *pDriveAddrXmlelem;
    struct XMLELEM     *pDriveInfoXmlelem;
    struct XMLELEM     *pDriveDataXmlelem;
    struct XMLELEM     *pParentXmlelem;

    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;
    struct RAWDRONE     rawdrone;
    struct RAWDRONE    *pRawdrone           = &rawdrone;

    struct XMLSTRUCT    volumeDataXmlstruct[] =
    {
        XNAME_volume_data,                  XNAME_volser,
        sizeof(pRawvolume->volser),         pRawvolume->volser,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_status,
        sizeof(pRawvolume->status),         pRawvolume->status,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_media,
        sizeof(pRawvolume->media),          pRawvolume->media,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_media_type,
        sizeof(pRawvolume->mediaType),      pRawvolume->mediaType,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_media_domain,
        sizeof(pRawvolume->mediaDomain),    pRawvolume->mediaDomain,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_home_cell,
        sizeof(pRawvolume->homeCell),       pRawvolume->homeCell,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_vtss_name,
        sizeof(pRawvolume->vtssName),       pRawvolume->vtssName,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_resident_vtss,
        sizeof(pRawvolume->residentVtss),   pRawvolume->residentVtss,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_density,
        sizeof(pRawvolume->denRectName),    pRawvolume->denRectName,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_scratch,
        sizeof(pRawvolume->scratch),        pRawvolume->scratch,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_encrypted,
        sizeof(pRawvolume->encrypted),      pRawvolume->encrypted,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_pool_type,
        sizeof(pRawvolume->subpoolType),    pRawvolume->subpoolType,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_subpool_name,
        sizeof(pRawvolume->subpoolName),    pRawvolume->subpoolName,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_device_address,
        sizeof(pRawvolume->charDevAddr),    pRawvolume->charDevAddr,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_result,
        sizeof(pRawvolume->result),         pRawvolume->result,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_error,
        sizeof(pRawvolume->error),          pRawvolume->error,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_volume_data,                  XNAME_reason,
        sizeof(pRawvolume->reason),         pRawvolume->reason,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 volumeDataElementCount = sizeof(volumeDataXmlstruct) / 
                                                 sizeof(struct XMLSTRUCT);

    struct XMLSTRUCT    libAddrXmlstruct[]  =
    {
        XNAME_library_address,              XNAME_acs,
        sizeof(pRawvolume->cellAcs),        pRawvolume->cellAcs,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_lsm,
        sizeof(pRawvolume->cellLsm),        pRawvolume->cellLsm,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_panel,
        sizeof(pRawvolume->cellPanel),      pRawvolume->cellPanel,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_row,
        sizeof(pRawvolume->cellRow),        pRawvolume->cellRow,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_library_address,              XNAME_column,
        sizeof(pRawvolume->cellColumn),     pRawvolume->cellColumn,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 libAddrElementCount = sizeof(libAddrXmlstruct) / 
                                              sizeof(struct XMLSTRUCT);

    struct XMLSTRUCT    mountDataXmlstruct[] =
    {
        XNAME_mount_data,                   XNAME_drive_name,
        sizeof(pRawvolume->driveName),      pRawvolume->driveName,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_mount_data,                   XNAME_drive_location_id,
        sizeof(pRawvolume->driveLocId),     pRawvolume->driveLocId,
        BLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 mountDataElementCount = sizeof(mountDataXmlstruct) / 
                                                sizeof(struct XMLSTRUCT);

    struct XMLSTRUCT    driveAddrXmlstruct[] =
    {
        XNAME_drive_library_address,        XNAME_acs,
        sizeof(pRawvolume->driveAcs),       pRawvolume->driveAcs,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_library_address,        XNAME_lsm,
        sizeof(pRawvolume->driveLsm),       pRawvolume->driveLsm,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_library_address,        XNAME_panel,
        sizeof(pRawvolume->drivePanel),     pRawvolume->drivePanel,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_library_address,        XNAME_drive_number,
        sizeof(pRawvolume->driveNumber),    pRawvolume->driveNumber,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 driveAddrElementCount = sizeof(driveAddrXmlstruct) / 
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

    memset(pRawvolume, 0, sizeof(struct RAWVOLUME));

    FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                      pVolumeDataXmlelem,
                                      &volumeDataXmlstruct[0],
                                      volumeDataElementCount);

    pLibAddrXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                              pVolumeDataXmlelem,
                                              XNAME_library_address);

    TRMSGI(TRCI_XAPI,
           "pLidAddrXmlelem=%08X\n",
           pLibAddrXmlelem);

    if (pLibAddrXmlelem != NULL)
    {
        FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                          pLibAddrXmlelem,
                                          &libAddrXmlstruct[0],
                                          libAddrElementCount);
    }


    pMountDataXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                pVolumeDataXmlelem,
                                                XNAME_mount_data);

    TRMSGI(TRCI_XAPI,
           "pMountDataXmlelem=%08X\n",
           pMountDataXmlelem);

    if (pMountDataXmlelem != NULL)
    {
        FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                          pMountDataXmlelem,
                                          &mountDataXmlstruct[0],
                                          mountDataElementCount);

        pDriveAddrXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                    pMountDataXmlelem,
                                                    XNAME_drive_library_address);

        TRMSGI(TRCI_XAPI,
               "pDriveAddrXmlelem=%08X\n",
               pDriveAddrXmlelem);

        if (pDriveAddrXmlelem != NULL)
        {
            FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                              pDriveAddrXmlelem,
                                              &driveAddrXmlstruct[0],
                                              driveAddrElementCount);
        }
    }

    xapi_parse_rectechs_xmlparse(pXmlparse,
                                 pVolumeDataXmlelem,
                                 pRawvolume);

    xapi_parse_loctype_rawvolume(pRawvolume);

    TRMEMI(TRCI_XAPI,
           pRawvolume, sizeof(struct RAWVOLUME),
           "locType=%02X (addr=%08X); RAWVOLUME:\n",
           pRawvolume->locType[0],
           &pRawvolume->locType[0]);

    if (pRawdrlst != NULL)
    {
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

        traceSize = (sizeof(struct RAWDRONE) * driveCount) + 
                    offsetof(struct RAWDRLST, rawdrone[0]);

        TRMEMI(TRCI_XAPI,
               pRawdrlst, traceSize,
               "driveCount=%d; RAWDRLST:\n",
               driveCount);
    }

    return STATUS_SUCCESS;
}



