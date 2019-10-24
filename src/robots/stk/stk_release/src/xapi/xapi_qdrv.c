/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qdrv.c                                      */
/** Description:    XAPI QUERY DRIVE processor.                      */
/**                                                                  */
/**                 Return tape drive information and status.        */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     06/01/11                          */
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
#define QUERY_SEND_TIMEOUT         5   /* TIMEOUT values in seconds  */       
#define QUERY_RECV_TIMEOUT_1ST     300
#define QUERY_RECV_TIMEOUT_NON1ST  600
#define DUMMY_DRIVE_LOCATION_ID    "DRIVELOCATIONID "
#define QDRVERR_RESPONSE_SIZE (offsetof(QUERY_RESPONSE, status_response)) + \
                              (offsetof(QU_DRV_RESPONSE, drive_status[0])) 


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int buildQdriveRequest(struct XAPICVT   *pXapicvt,
                              struct XAPIREQE  *pXapireqe,
                              char            **ptrXapiBuffer,
                              int              *pXapiBufferSize);

static int issueQdriveRequest(struct XAPICVT  *pXapicvt,
                              struct XAPIREQE *pXapireqe,
                              char            *pXapiBuffer,
                              int              xapiBufferSize,
                              char            *pDriveLocationId,
                              struct LIBDRVID *pLibdrvid,
                              struct XAPICFG  *pMatchingXapicfg,
                              QU_DRV_STATUS   *pQu_Drv_Status);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qdrv                                         */
/** Description:   XAPI QUERY DRIVE processor.                       */
/**                                                                  */
/** Return real and virtual tape drive information and status for    */
/** the specified drives or for all drives in the XAPI client drive  */
/** configuration table (XAPICFG).                                   */
/**                                                                  */
/** The XAPICFG is the XAPI client drive configuration table.        */
/** There is one XAPICFG table entry for each TapePlex real or       */
/** virtual tape drive that is accessible from the XAPI client.      */
/** Therefore, the XAPICFG contains a subset of all TapePlex         */
/** real and virtual tape drives.                                    */
/**                                                                  */
/** The QUERY DRIVE request is processed by issuing an               */
/** XAPI XML format <query_drive> request for each drive specified   */
/** in the ACSAPI format QUERY DRIVE request (or for all drives if   */
/** the request drive count is 0).  There may be multiple            */
/** individual XAPI <query_drive> requests issued for a single       */
/** ACSAPI QUERY DRIVE request.  Indeed, there may be                */
/** XAPICVT.xapicfgCount individual XAPI <query_drive>               */
/** requests issued.                                                 */
/**                                                                  */
/** The QUERY DRIVE command is allowed to proceed even when          */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qdrv"

extern int xapi_qdrv(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;
    int                 qdriveResponseSize;
    int                 i;

    char               *pXapiBuffer         = NULL;
    char               *pDriveLocationId    = NULL;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_DRV_CRITERIA    *pQu_Drv_Criteria    = &(pQuery_Request->select_criteria.drive_criteria);

    int                 drvResponseCount    = pQu_Drv_Criteria->drive_count;
    int                 drvRemainingCount;
    int                 drvPacketCount;
    int                 finalDrvCount;
    char                queryDriveAllFlag   = FALSE;

    struct XAPICFG     *pCurrXapicfg;
    struct XAPICFG     *pMatchingXapicfg;
    DRIVEID            *pDriveid;
    struct LIBDRVID     libdrvid;
    struct LIBDRVID    *pLibdrvid;

    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;
    QU_DRV_RESPONSE    *pQu_Drv_Response    = &(pQuery_Response->status_response.drive_response);
    QU_DRV_STATUS      *pQu_Drv_Status      = &(pQu_Drv_Response->drive_status[0]);

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         sizeof(QU_DRV_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY DRV request=%08X, size=%d, count=%d, "
           "MAX_ID=%d, QUERY DRV response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           drvResponseCount,
           MAX_ID,
           pQuery_Response,
           sizeof(QU_DRV_RESPONSE));

    if (drvResponseCount < 0)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QDRVERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_SMALL);

        return STATUS_COUNT_TOO_SMALL;
    }

    if (drvResponseCount > MAX_ID)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QDRVERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_LARGE);

        return STATUS_COUNT_TOO_LARGE;
    }

    if (pXapireqe->pXapicfg == NULL)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QDRVERR_RESPONSE_SIZE,
                          STATUS_NI_FAILURE);

        return STATUS_NI_FAILURE;
    }

    if (drvResponseCount == 0)
    {
        qdriveResponseSize = (char*) pQu_Drv_Status -
                             (char*) pQuery_Response +
                             ((sizeof(QU_DRV_STATUS)) * MAX_ID);
    }
    else
    {
        qdriveResponseSize = (char*) pQu_Drv_Status -
                             (char*) pQuery_Response +
                             ((sizeof(QU_DRV_STATUS)) * drvResponseCount);
    }

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

    buildQdriveRequest(pXapicvt,
                       pXapireqe,
                       &pXapiBuffer,
                       &xapiBufferSize);

    /*****************************************************************/
    /* Find the substring that is continually updated with the       */
    /* next drive.                                                   */
    /*****************************************************************/
    pDriveLocationId = strstr(pXapiBuffer, DUMMY_DRIVE_LOCATION_ID);

#ifdef DEBUG

    TRMEM(pXapiBuffer, xapiBufferSize,
          "pDriveLocationId=%08X, pXapiBuffer:\n",
          pDriveLocationId);

#endif

    /*****************************************************************/
    /* If this is a QUERY DRIVE ALL request, then setup              */
    /* to step through each XAPICFG table entry for the next drive.  */
    /*****************************************************************/
    if (drvResponseCount == 0)
    {
        queryDriveAllFlag = TRUE;
        drvResponseCount = pXapireqe->xapicfgCount;
        pCurrXapicfg = pXapireqe->pXapicfg;     
    }

    drvRemainingCount = drvResponseCount;
    pDriveid = (DRIVEID*) &(pQu_Drv_Criteria->drive_id[0]);

    /*****************************************************************/
    /* Run through the input drive list (or XAPICFG table), and      */
    /* issue the QUERY DRIVE request for each individual drive.      */
    /*****************************************************************/
    while (1)
    {
        xapi_query_init_resp(pXapireqe,
                             (char*) pQuery_Response,
                             qdriveResponseSize);

        pQuery_Response = (QUERY_RESPONSE*) pQuery_Response;
        pQu_Drv_Response = (QU_DRV_RESPONSE*) &(pQuery_Response->status_response);
        pQu_Drv_Status = (QU_DRV_STATUS*) &(pQu_Drv_Response->drive_status);

        if (drvRemainingCount > MAX_ID)
        {
            drvPacketCount = MAX_ID;
        }
        else
        {
            drvPacketCount = drvRemainingCount;
        }

        TRMSGI(TRCI_XAPI,
               "At top of while; drvResponse=%d, drvRemaining=%d, "
               "drvPacket=%d, MAX_ID=%d\n",
               drvResponseCount,
               drvRemainingCount,
               drvPacketCount,
               MAX_ID);

        pQu_Drv_Response->drive_count = drvPacketCount;

        for (i = 0;
            i < drvPacketCount;
            i++, pQu_Drv_Status++)
        {
            if (queryDriveAllFlag)
            {
                pLibdrvid = &(pCurrXapicfg->libdrvid);
                pMatchingXapicfg = pCurrXapicfg;
                pCurrXapicfg++;
            }
            else
            {
                pLibdrvid = &libdrvid;
                pLibdrvid->acs = pDriveid->panel_id.lsm_id.acs;
                pLibdrvid->lsm = pDriveid->panel_id.lsm_id.lsm;
                pLibdrvid->panel = pDriveid->panel_id.panel;
                pLibdrvid->driveNumber = pDriveid->drive;

                pMatchingXapicfg = xapi_config_search_libdrvid(pXapicvt,
                                                               pXapireqe,
                                                               pLibdrvid);

                pDriveid++;
            }

            lastRC = issueQdriveRequest(pXapicvt,
                                        pXapireqe,
                                        pXapiBuffer,
                                        xapiBufferSize,
                                        pDriveLocationId,
                                        pLibdrvid,
                                        pMatchingXapicfg,
                                        pQu_Drv_Status);

            if (lastRC == STATUS_PROCESS_FAILURE)
            {
                queryRC = lastRC;
            }
        } /* for(i) */

        drvRemainingCount = drvRemainingCount - drvPacketCount;

        if (drvRemainingCount <= 0)
        {
            finalDrvCount = drvPacketCount;

            break;
        }

        xapi_int_response(pXapireqe,
                          (char*) pQuery_Response,     
                          qdriveResponseSize);  
    }

    qdriveResponseSize = (char*) pQu_Drv_Status -
                         (char*) pQuery_Response +
                         ((sizeof(QU_DRV_STATUS)) * finalDrvCount);

    xapi_fin_response(pXapireqe,
                      (char*) pQuery_Response,
                      qdriveResponseSize);

    /*****************************************************************/
    /* At this point we can free the XAPI XML request string.        */
    /*****************************************************************/
    TRMSGI(TRCI_STORAGE,
           "free pXapiBuffer=%08X, len=%i\n",
           pXapiBuffer,
           xapiBufferSize);

    free(pXapiBuffer);

    return queryRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: buildQdriveRequest                                */
/** Description:   Build a "model" XAPI <query_drive> request.       */
/**                                                                  */
/** Build a "model" XAPI <query_drive> request for a single          */
/** drive.                                                           */
/**                                                                  */
/** NOTE: Because of the restrictions of the XAPI <query_drive>      */
/** request, we must build (or modify) a new XAPI <query_drive>      */
/** request for each individual drive specified in the input         */
/** ACSAPI QUERY DRIVE request.                                      */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY DRIVE request consists of:                      */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY_REQUEST data consisting of:                           */
/**      i.   type                                                   */
/** 3.   QU_DRV_CRITERIA data consisting of:                         */
/**      i.   count (of drive(s) requested or 0 for "all")           */
/**      ii.  DRIVEID[count] data entries consisting of:             */
/**           a.   drive_id.panel_id.lsm_id.acs                      */
/**           b.   drive_id.panel_id.lsm_id.lsm                      */
/**           c.   drive_id.panel_id.panel                           */
/**           d.   drive_id.drive                                    */
/**                                                                  */
/** NOTE: MAX_ID is 42.  So a max subset of 42 drives(s) can be      */
/** requested at once.  If 0 drives(s) is specified, then all        */
/** drives(s) are returned in possible multiple intermediate         */
/** responses.                                                       */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_drive> request consists of:                  */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <query_drive>                                                */
/**       <drive_location_id>DRIVELOCATIONID </drive_location_id>    */
/**     </query_drive>                                               */
/**   </command>                                                     */
/** </libtrans>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "buildQdriveRequest"

static int buildQdriveRequest(struct XAPICVT  *pXapicvt,
                              struct XAPIREQE *pXapireqe,
                              char           **ptrXapiBuffer,
                              int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;

    char               *pXapiRequest        = NULL;
    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_DRV_CRITERIA    *pQu_Drv_Criteria    = &(pQuery_Request->select_criteria.drive_criteria);

    struct XMLPARSE    *pXmlparse; 
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    TRMSGI(TRCI_XAPI, "Entered\n");

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
                                      XNAME_query_drive_info,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    /*****************************************************************/
    /* Insert a placeholder for the <drive_location_id> which we     */
    /* will later overlay with the correct XAPICFG value.            */
    /*****************************************************************/
    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_drive_location_id,
                                      DUMMY_DRIVE_LOCATION_ID,
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

    return STATUS_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: issueQdriveRequest                                */
/** Description:   Issue the XAPI <query_drive> request.             */
/**                                                                  */
/** Modify the "model" XAPI <query_drive> request for the next       */
/** drive; transmit the updated XAPI <query_drive> request to the    */
/** server via TCP/IP;  The received XAPI XML response is then       */
/** translated into the an ACSAPI QUERY DRIVE, QU_DRV_STATUS         */
/** response.                                                        */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_drive_info_request> response consists of:    */
/**==================================================================*/
/** <libreply>                                                       */
/**   <query_drive_info_request>                                     */
/**     <drive_info>                                                 */
/**       <drive_name>ccuu</drive_name>                              */
/**       <drive_location_id>ccc...ccc</drive_location_id>           */
/**       <model>mmmmmmmm</model>                                    */
/**       <rectech>rrrrrrrr</rectech>                                */
/**       <drive_library_address>                                    */
/**         <acs>aa</acs>                                            */
/**         <lsm>ll</lsm>                                            */
/**         <panel>pp|ppp</panel>                                    */
/**         <drive_number>dd|ddd</drive_number>                      */
/**       </drive_library_address>                                   */
/**       <drive_location_zone>zzzzz</drive_location_zone>           */
/**       <vtss_name>vvvvvvvv</vtss_name>                            */
/**       <drive_group_location>ccc...ccc</drive_group_location>     */
/**       <volser>vvvvvv</volser>                                    */
/**       <status>MOUNTING|ON DRIVE|DISMOUNT</status>                */
/**       <state>ssssssss</state>                                    */
/**       <world_wide_name>ccc...ccc</world_wide_name>               */
/**       <serial_number>ccc...ccc</serial_number>                   */
/**     </drive_info>                                                */
/**   </query_drive_info_request>                                    */
/** </libreply>                                                      */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY DRIVE response consists of:                     */
/**==================================================================*/
/** 1.   RESPONSE_HEADER data consisting of:                         */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER                                         */
/** 2.   RESPONSE_STATUS consisting of:                              */
/**      i.   status                                                 */
/**      ii.  type                                                   */
/**      iii. identifier                                             */
/** 3.   QUERY_RESPONSE data consisting of:                          */
/**      i.   type                                                   */
/**      ii.  QU_DRV_RESPONSE data consisting of:                    */
/**           a.   count                                             */
/**           b.   QU_DRV_STATUS[count] data entries consisting of:  */
/**                1.   DRIVEID consisting of:                       */
/**                     i.   drive_id.panel_id.lsm_id.acs            */
/**                     ii.  drive_id.panel_id.lsm_id.lsm            */
/**                     iii. drive_id.panel_id.panel                 */
/**                     iv.  drive_id.drive                          */
/**                2.   VOLID consisting of:                         */
/**                     i.   external_label (6 character volser)     */
/**                3.   drive_type (integer)                         */
/**                4.   state                                        */
/**                5.   status                                       */
/**                                                                  */
/** NOTE: QU_DRV_STATUS.status of STATUS_DRIVE_AVAILABLE only        */
/** takes into consideration if a volume is currently mounted        */
/** on the drive.  A status of STATUS_DRIVE_AVAILABLE can be         */
/** returned even if the drive is locked, or if it is not            */
/** "online" to the current host.                                    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "issueQdriveRequest"

static int issueQdriveRequest(struct XAPICVT  *pXapicvt,
                              struct XAPIREQE *pXapireqe,
                              char            *pXapiBuffer,
                              int              xapiBufferSize,
                              char            *pDriveLocationId,
                              struct LIBDRVID *pLibdrvid,
                              struct XAPICFG  *pMatchingXapicfg,
                              QU_DRV_STATUS   *pQu_Drv_Status)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    struct XMLPARSE    *pXmlparse           = NULL;

    struct XMLELEM     *pDriveInfoXmlelem;
    struct XMLELEM     *pDriveAddrXmlelem;
    struct XMLELEM     *pParentXmlelem;

    struct XAPICOMMON   xapicommon;
    struct XAPICOMMON  *pXapicommon         = &xapicommon;

    struct RAWDRIVE     rawdrive;
    struct RAWDRIVE    *pRawdrive           = &rawdrive;
    struct XAPICFG     *pXapicfg;
    unsigned short      hexDevAddr;
    char                charDevAddr[5];

    struct XMLSTRUCT    driveInfoXmlstruct[] =
    {
        XNAME_drive_info,                   XNAME_volser,
        sizeof(pRawdrive->volser),          pRawdrive->volser,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_info,                   XNAME_drive_name,
        sizeof(pRawdrive->driveName),       pRawdrive->driveName,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_info,                   XNAME_drive_location_id,
        sizeof(pRawdrive->driveLocId),      pRawdrive->driveLocId,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_info,                   XNAME_model,
        sizeof(pRawdrive->model),           pRawdrive->model,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_drive_info,                   XNAME_rectech,
        sizeof(pRawdrive->rectech),         pRawdrive->rectech,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_drive_info,                   XNAME_vtss_name,
        sizeof(pRawdrive->vtssName),        pRawdrive->vtssName,
        BLANKFILL,   NOBITVALUE, 0, 0,
        XNAME_drive_info,                   XNAME_drive_group_location,
        sizeof(pRawdrive->driveGroup),      pRawdrive->driveGroup,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_info,                   XNAME_serial_number,
        sizeof(pRawdrive->serialNumber),    pRawdrive->serialNumber,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_info,                   XNAME_status,
        sizeof(pRawdrive->status),          pRawdrive->status,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_info,                   XNAME_state,
        sizeof(pRawdrive->state),           pRawdrive->state,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_info,                   XNAME_world_wide_name,
        sizeof(pRawdrive->worldWideName),   pRawdrive->worldWideName,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_info,                   XNAME_drive_location_zone,
        sizeof(pRawdrive->driveLocZone),    pRawdrive->driveLocZone,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 driveInfoElementCount = sizeof(driveInfoXmlstruct) / 
                                                sizeof(struct XMLSTRUCT);

    struct XMLSTRUCT    driveAddrXmlstruct[] =
    {
        XNAME_drive_library_address,        XNAME_acs,
        sizeof(pRawdrive->acs),             pRawdrive->acs,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_library_address,        XNAME_lsm,
        sizeof(pRawdrive->lsm),             pRawdrive->lsm,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_library_address,        XNAME_panel,
        sizeof(pRawdrive->panel),           pRawdrive->panel,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_drive_library_address,        XNAME_drive_number,
        sizeof(pRawdrive->driveNumber),     pRawdrive->driveNumber,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 driveAddrElementCount = sizeof(driveAddrXmlstruct) / 
                                                sizeof(struct XMLSTRUCT);

    TRMSGI(TRCI_XAPI,
           "Entered; next drive=%d:%d:%d:%d\n",
           pLibdrvid->acs,
           pLibdrvid->lsm,
           pLibdrvid->panel,
           pLibdrvid->driveNumber);

    pQu_Drv_Status->drive_id.panel_id.lsm_id.acs = pLibdrvid->acs;
    pQu_Drv_Status->drive_id.panel_id.lsm_id.lsm = pLibdrvid->lsm;
    pQu_Drv_Status->drive_id.panel_id.panel = pLibdrvid->panel;
    pQu_Drv_Status->drive_id.drive = pLibdrvid->driveNumber;
    pQu_Drv_Status->state = STATE_ONLINE;
    pQu_Drv_Status->status = STATUS_DRIVE_AVAILABLE;

    if (pMatchingXapicfg == NULL)
    {
        pQu_Drv_Status->status = STATUS_DRIVE_NOT_IN_LIBRARY;

        return queryRC;
    }

    TRMEMI(TRCI_XAPI, pMatchingXapicfg, sizeof(struct XAPICFG),
           "Matching XAPICFG:\n");

    pQu_Drv_Status->drive_type = (DRIVE_TYPE) pMatchingXapicfg->acsapiDriveType;

    sprintf(charDevAddr, "%.4X", pMatchingXapicfg->driveName);
     
    TRMSGI(TRCI_XAPI, 
           "charDevAddr=%.4s\n", 
           charDevAddr);

    memcpy(pDriveLocationId,
           pMatchingXapicfg->driveLocId,
           sizeof(pMatchingXapicfg->driveLocId));

    memset((char*) pRawdrive, 0, sizeof(struct RAWDRIVE));

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
        pQu_Drv_Status->status = lastRC;
        queryRC = lastRC;
    }

    if (queryRC == STATUS_SUCCESS)
    {
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
            pQu_Drv_Status->status = STATUS_PROCESS_FAILURE;
            queryRC = STATUS_PROCESS_FAILURE;
        }

        if (queryRC == STATUS_SUCCESS)
        {
            pDriveInfoXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                        pXmlparse->pHocXmlelem,
                                                        XNAME_drive_info);

#ifdef DEBUG

            TRMSGI(TRCI_XAPI,
                   "pDriveInfoXmlelem=%08X\n",
                   pDriveInfoXmlelem);

#endif

            if (pDriveInfoXmlelem == NULL)
            {
                pQu_Drv_Status->status = STATUS_PROCESS_FAILURE;
                queryRC = STATUS_PROCESS_FAILURE;
            }
            else
            {
                FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                                  pDriveInfoXmlelem,
                                                  &driveInfoXmlstruct[0],
                                                  driveInfoElementCount);

                pDriveAddrXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                            pDriveInfoXmlelem,
                                                            XNAME_drive_library_address);

                if (pDriveAddrXmlelem != NULL)
                {
                    FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                                      pDriveAddrXmlelem,
                                                      &driveAddrXmlstruct[0],
                                                      driveAddrElementCount);
                }
            }

//#ifdef DEBUG

            TRMEMI(TRCI_XAPI,
                   pRawdrive, sizeof(struct RAWDRIVE),
                   "RAWDRIVE:\n");

//#endif

            if (pRawdrive->volser[0] > ' ')
            {
                memcpy(pQu_Drv_Status->vol_id.external_label,
                       pRawdrive->volser,
                       sizeof(pRawdrive->volser));

                pQu_Drv_Status->status = STATUS_DRIVE_IN_USE;
            }

            if (memcmp(pRawdrive->state,
                       "OFFLINE",
                       3) == 0)
            {
                pQu_Drv_Status->state = STATE_OFFLINE;
            }

            if ((memcmp(pRawdrive->status,
                        "MOUNTING",
                        3) == 0) ||
                (memcmp(pRawdrive->status,
                        "ON DRIVE",
                        3) == 0))
            {
                pQu_Drv_Status->status = STATUS_DRIVE_IN_USE;
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



