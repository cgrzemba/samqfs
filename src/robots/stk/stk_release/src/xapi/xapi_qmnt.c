/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qmnt.c                                      */
/** Description:    XAPI QUERY MOUNT processor.                      */
/**                                                                  */
/**                 Return a drive list for the specified            */
/**                 VOLUME(s).                                       */
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
#define QMNTERR_RESPONSE_SIZE (offsetof(QUERY_RESPONSE, status_response)) + \
                              (offsetof(QU_MNT_RESPONSE, mount_status[0])) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qmnt                                         */
/** Description:   The XAPI QUERY MOUNT processor.                   */
/**                                                                  */
/** Return a drive list of eligible drives for the specified         */
/** VOLUME(s).                                                       */
/**                                                                  */
/** The QUERY MOUNT request is processed by calling the XAPI         */
/** query volume service (xapi_qvol_one) requesting a                */
/** drive list.                                                      */
/**                                                                  */
/** If multiple volser(s) are specified, then the request is         */
/** NOT treated as a multi-volume request, but the request is        */
/** treated as multiple single volser requests.  In such cases,      */
/** the XAPI client query volume service is called multiple times,   */
/** and multiple drive lists are returned, each drive list in        */
/** its own intermediate response.                                   */
/**                                                                  */
/** The QUERY MOUNT command is allowed to proceed even when          */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY MOUNT request consists of:                      */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY_REQUEST data consisting of:                           */
/**      i.   type                                                   */
/** 3.   QU_VOL_CRITERIA data consisting of:                         */
/**      i.   count (of VOLID(s) requested; must be > 0)             */
/**      ii.  VOLID[count] entries consisting of:                    */
/**           a.   external_label (6 character volser)               */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY MOUNT response consists of:                     */
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
/**      ii.  QU_MNT_RESPONSE data consisting of:                    */
/**           a.   count                                             */
/**           b.   QU_MNT_STATUS[count] data entries consisting of:  */
/**                1.   VOLID consisting of:                         */
/**                     i.    external_label (6 character volser)    */
/**                2.   status                                       */
/**                3.   drive_count                                  */
/**                4.   QU_DRV_STATUS[drive_count] data entries      */
/**                     consisting of:                               */
/**                     i.   DRIVEID consisting of:                  */
/**                          a.   drive_id.panel_id.lsm_id.acs       */
/**                          b.   drive_id.panel_id.lsm_id.lsm       */
/**                          c.   drive_id.panel_id.panel            */
/**                          d.   drive_id.drive                     */
/**                     ii.  VOLID consisting of:                    */
/**                          a.   external_label (6 char volser)     */
/**                     iii. drive_type (integer)                    */
/**                     iv.  state                                   */
/**                     v.   status                                  */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qmnt"

extern int xapi_qmnt(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 qmntResponseSize;
    int                 i;
    int                 j;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_VOL_CRITERIA    *pQu_Vol_Criteria    = &(pQuery_Request->select_criteria.vol_criteria);

    int                 volCount            = pQu_Vol_Criteria->volume_count;
    int                 matchingDriveCount;

    struct RAWVOLUME    rawvolume;
    struct RAWVOLUME   *pRawvolume          = &rawvolume;
    struct RAWDRLST     rawdrlst;
    struct RAWDRLST    *pRawdrlst           = &rawdrlst;
    struct RAWDRONE    *pRawdrone;
    struct XAPICFG     *pXapicfg;
    unsigned short      hexDevAddr;
    char                volser[6];

    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;
    QU_MNT_RESPONSE    *pQu_Mnt_Response    = &(pQuery_Response->status_response.mount_response);
    QU_MNT_STATUS      *pQu_Mnt_Status      = &(pQu_Mnt_Response->mount_status[0]);
    QU_DRV_STATUS      *pQu_Drv_Status      = &(pQu_Mnt_Status->drive_status[0]);

    /*****************************************************************/
    /* NOTE: The ACSAPI supposedly allows MAX_ID number of           */
    /* QU_MNT_STATUS data elements for each intermediate or final    */
    /* response.  However that would cause the QUERY MOUNT           */
    /* response size to be larger than the maximum IPC buffer size   */
    /* (MAX_MESSAGE_SIZE) causing unpredicatble results.             */
    /*                                                               */
    /* We therefore create an intermediate response for each         */
    /* volume in the request (except for the last volume which       */
    /* creates a final response).                                    */
    /*****************************************************************/
    qmntResponseSize = (char*) pQu_Mnt_Status -
                       (char*) pQuery_Response +
                       ((sizeof(QU_MNT_STATUS)) * 1);

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         qmntResponseSize);

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY MOUNT request=%08X, size=%d, "
           "count=%d, MAX_ID=%d, MAX_MESSAGE_SIZE=%d, "
           "QUERY MOUNT response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           volCount,
           MAX_ID,
           MAX_MESSAGE_SIZE,
           pQuery_Response,
           qmntResponseSize);

    /*****************************************************************/
    /* NOTE: The ACSAPI QUERY MOUNT request allows multiple volumes  */
    /* to be specified.   This is not interpreted at a multi-volume  */
    /* dataset, but a multiple requests for a specific volser (this  */
    /* is the way that LibStation would process the request).        */
    /*****************************************************************/
    if (volCount < 1)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QMNTERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_SMALL); 

        return STATUS_COUNT_TOO_SMALL;
    }

    if (volCount > MAX_ID)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QMNTERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_LARGE); 

        return STATUS_COUNT_TOO_LARGE;
    }

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

    /*****************************************************************/
    /* Call the xapi_qvol_one service for each volume in the request.*/
    /*****************************************************************/
    for (i = 0;
        i < volCount;
        i++)
    {
        xapi_query_init_resp(pXapireqe,
                             (char*) pQuery_Response,
                             qmntResponseSize);

        pQu_Mnt_Response->mount_status_count = 1;

        memset((char*) pRawvolume, 0, sizeof(struct RAWVOLUME));
        memset((char*) pRawdrlst, 0, sizeof(struct RAWDRLST));

        memcpy(volser,
               &(pQu_Vol_Criteria->volume_id[i]),
               sizeof(volser));

        lastRC = xapi_qvol_one(pXapicvt,
                               pXapireqe,
                               pRawvolume,
                               pRawdrlst,
                               volser);

        TRMSGI(TRCI_XAPI,
               "lastRC=%i from xapi_qvol_one(%.6s)\n",
               lastRC,
               volser);

        if (lastRC != STATUS_SUCCESS)
        {
            pQu_Mnt_Status->status = STATUS_VOLUME_NOT_IN_LIBRARY;
        }
        else
        {
            if (pRawvolume->volser[0] > ' ')
            {
                memcpy(pQu_Mnt_Status->vol_id.external_label,
                       pRawvolume->volser,
                       sizeof(pRawvolume->volser));
            }
            else
            {
                memcpy(pQu_Mnt_Status->vol_id.external_label,
                       "??????",
                       sizeof(pQu_Mnt_Status->vol_id.external_label));
            }

            if (toupper(pRawvolume->result[0]) == 'F')
            {
                pQu_Mnt_Status->status = STATUS_VOLUME_NOT_IN_LIBRARY;
            }
            else
            {
                pQu_Mnt_Status->status = STATUS_SUCCESS;
            }
        }

        pRawdrone = &(pRawdrlst->rawdrone[0]);
        pQu_Drv_Status = (QU_DRV_STATUS*) &(pQu_Mnt_Status->drive_status[0]);
        matchingDriveCount = 0;

        for (j = 0;
            j < pRawdrlst->driveCount;
            j++, pRawdrone++)
        {
            if (pRawdrone->driveLocId[0] > ' ')
            {
                pXapicfg = xapi_config_search_drivelocid(pXapicvt,
                                                         pXapireqe,
                                                         pRawdrone->driveLocId);
            }
            else if (pRawdrone->driveName[0] > ' ')
            {
                FN_CONVERT_CHARDEVADDR_TO_HEX(pRawdrone->driveName,
                                              &hexDevAddr);

                pXapicfg = xapi_config_search_hexdevaddr(pXapicvt,
                                                         pXapireqe,
                                                         hexDevAddr);
            }
            else
            {
                pXapicfg = NULL;
            }

            if (pXapicfg != NULL)
            {
                pQu_Drv_Status->drive_id.panel_id.lsm_id.acs = pXapicfg->libdrvid.acs;
                pQu_Drv_Status->drive_id.panel_id.lsm_id.lsm = pXapicfg->libdrvid.lsm;
                pQu_Drv_Status->drive_id.panel_id.panel = pXapicfg->libdrvid.panel;
                pQu_Drv_Status->drive_id.drive = pXapicfg->libdrvid.driveNumber;
                pQu_Drv_Status->drive_type = (DRIVE_TYPE) pXapicfg->acsapiDriveType;
                pQu_Drv_Status->state = STATE_ONLINE;
                pQu_Drv_Status->status = STATUS_DRIVE_AVAILABLE;

                if (memcmp(pRawdrone->state,
                           "OFFLINE",
                           3) == 0)
                {
                    pQu_Drv_Status->state = STATE_OFFLINE;
                }

                matchingDriveCount++;
                pQu_Drv_Status++;       
            }
        }

        pQu_Mnt_Status->drive_count = matchingDriveCount;

        if ((i + 1) < volCount)
        {
            xapi_int_response(pXapireqe,
                              (char*) pQuery_Response,
                              qmntResponseSize);
        }
        else
        {
            xapi_fin_response(pXapireqe,
                              (char*) pQuery_Response,
                              qmntResponseSize);
        }
    }

    return queryRC;
}



