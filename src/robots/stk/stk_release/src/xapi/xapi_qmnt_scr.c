/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qmnt_scr.c                                  */
/** Description:    XAPI QUERY MOUNT SCRATCH processor.              */
/**                                                                  */
/**                 Return a drive list for the specified POOLID     */
/**                 (subpool index).                                 */
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
#define QMSCERR_RESPONSE_SIZE (offsetof(QUERY_RESPONSE, status_response)) + \
                              (offsetof(QU_MSC_RESPONSE, mount_scratch_status[0])) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qmnt_scr                                     */
/** Description:   The XAPI QUERY MOUNT SCRATCH processor.           */
/**                                                                  */
/** Return a drive list of eligible drives for the specified         */
/** POOLID (subpool index).                                          */
/**                                                                  */
/** The QUERY MOUNT SCRATCH request is processed by calling the XAPI */
/** client query mount scratch service (xapi_qmnt_one).              */
/**                                                                  */
/** If multiple POOLIDs (subpool indexes) are specified, then the    */
/** request is treated as multiple single POOLID requests.           */
/** In such cases, the XAPI client query mount scratch service is    */
/** called multiple times, and multiple drive lists are returned,    */
/** each drive list in its own intermediate response.                */
/**                                                                  */
/** The QUERY MOUNT SCRATCH command is allowed to proceed even when  */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY MOUNT SCRATCH request consists of:              */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY_REQUEST data consisting of:                           */
/**      i.   type                                                   */
/** 3.   QU_MSC_PINFO_CRITERIA data consisting of:                   */
/**      i.   media_type (may be specified as ANY_MEDIA_TYPE)        */
/**      ii.  count (of POOL(s) requested; must be > 0)              */
/**      iii. POOL[count] ids (subpool index)                        */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY MOUNT SCRATCH response consists of:             */
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
/**      ii.  QU_MSC_RESPONSE data consisting of:                    */
/**           a.   count                                             */
/**           b.   QU_MSC_STATUS[count] data entries consisting of:  */
/**                1.   POOLID consisting of:                        */
/**                     i.    POOL (subpool index)                   */
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
#define SELF "xapi_qmnt_scr"

extern int xapi_qmnt_scr(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 qmscResponseSize;
    int                 i;
    int                 j;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_MSC_CRITERIA    *pQu_Msc_Criteria    = 
    &(pQuery_Request->select_criteria.mount_scratch_criteria);

    int                 poolCount           = pQu_Msc_Criteria->pool_count;
    int                 matchingDriveCount;

    struct XAPICFG     *pXapicfg;
    struct XAPISCRPOOL *pXapiscrpool;
    struct XAPIMEDIA   *pXapimedia;
    unsigned short      hexDevAddr;
    short               subpoolIndex;
    char                acsapiMediaType;
    char               *pMediaNameString    = NULL;

    struct RAWSCRMNT    rawscrmnt;
    struct RAWSCRMNT   *pRawscrmnt          = &rawscrmnt;
    struct RAWDRLST     rawdrlst;
    struct RAWDRLST    *pRawdrlst           = &rawdrlst;
    struct RAWDRONE    *pRawdrone;

    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;
    QU_MSC_RESPONSE    *pQu_Msc_Response    = 
    &(pQuery_Response->status_response.mount_scratch_response);
    QU_MSC_STATUS      *pQu_Msc_Status      = &(pQu_Msc_Response->mount_scratch_status[0]);
    QU_DRV_STATUS      *pQu_Drv_Status      = &(pQu_Msc_Status->drive_list[0]);

    /*****************************************************************/
    /* NOTE: The ACSAPI supposedly allows MAX_ID number of           */
    /* QU_MSC_STATUS data elements for each intermediate or final    */
    /* response.  However that would cause the QUERY MOUNT SCRATCH   */
    /* response size to be larger than the maximum IPC buffer size   */
    /* (MAX_MESSAGE_SIZE) causing unpredicatble results.             */
    /*                                                               */
    /* We therefore create an intermediate response for each         */
    /* poolid in the request (except for the last poolid which       */
    /* creates a final response).                                    */
    /*****************************************************************/
    qmscResponseSize = (char*) pQu_Msc_Status -
                       (char*) pQuery_Response +
                       ((sizeof(QU_MSC_STATUS)) * 1);

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY MOUNT SCRATCH request=%08X, size=%d, "
           "count=%d, MAX_ID=%d, MAX_MESSAGE_SIZE=%d, "
           "QUERY MOUNT SCRATCH response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           poolCount,
           MAX_ID,
           MAX_MESSAGE_SIZE,
           pQuery_Response,
           qmscResponseSize);

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         qmscResponseSize);

    /*****************************************************************/
    /* NOTE: The ACSAPI QUERY MOUNT SCRATCH request allows multiple  */
    /* pools to be specified.   This is not interpreted at a single  */
    /* request specifying multiple pools, but multiple requests for  */
    /* a single pool (this is the way that LibStation would          */
    /* process the request).                                         */
    /*****************************************************************/
    if (poolCount < 1)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QMSCERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_SMALL); 

        return STATUS_COUNT_TOO_SMALL;
    }

    if (poolCount > MAX_ID)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QMSCERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_LARGE); 

        return STATUS_COUNT_TOO_LARGE;
    }

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

    /*****************************************************************/
    /* Call the xapi_scrpool_search_index service for each poolid    */
    /* (pool index) in the request.                                  */
    /*****************************************************************/
    for (i = 0;
        i < poolCount;
        i++)
    {
        xapi_query_init_resp(pXapireqe,
                             (char*) pQuery_Response,
                             qmscResponseSize);

        pQu_Msc_Response->msc_status_count = 1;
        pQu_Msc_Status->pool_id.pool = pQu_Msc_Criteria->pool_id[i].pool;
        subpoolIndex = (short) pQu_Msc_Criteria->pool_id[i].pool;

        pXapiscrpool = xapi_scrpool_search_index(pXapicvt,
                                                 pXapireqe,
                                                 subpoolIndex);

        if (pXapiscrpool == NULL)
        {
            pQu_Msc_Status->status = STATUS_POOL_NOT_FOUND;
        }
        else
        {
            acsapiMediaType = (char) pQu_Msc_Criteria->media_type;

            if (acsapiMediaType != ALL_MEDIA_TYPE)
            {
                pXapimedia = xapi_media_search_type(pXapicvt,
                                                    pXapireqe,
                                                    acsapiMediaType);

                if (pXapimedia != NULL)
                {
                    pMediaNameString = &(pXapimedia->mediaNameString[0]); 
                }
            }

            memset(pRawscrmnt, 0, sizeof(struct RAWSCRMNT));
            memset(pRawdrlst, 0, sizeof(struct RAWDRLST));

            lastRC = xapi_qmnt_one(pXapicvt,
                                   pXapireqe,
                                   pRawscrmnt,
                                   pRawdrlst,
                                   pXapiscrpool->subpoolNameString,
                                   pMediaNameString,
                                   NULL);

            TRMSGI(TRCI_XAPI,
                   "lastRC=%i from xapi_qmnt_one(%s)\n",
                   lastRC,
                   pXapiscrpool->subpoolNameString);

            if (lastRC != RC_SUCCESS)
            {
                pQu_Msc_Status->status = (STATUS) lastRC;
                pQu_Msc_Status->drive_count = 0;
            }
            else
            {
                pRawdrone = &(pRawdrlst->rawdrone[0]);
                pQu_Drv_Status = (QU_DRV_STATUS*) &(pQu_Msc_Status->drive_list[0]);
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

                pQu_Msc_Status->drive_count = matchingDriveCount;
            }
        }

        if ((i + 1) < poolCount)
        {
            xapi_int_response(pXapireqe,
                              (char*) pQuery_Response,
                              qmscResponseSize);
        }
        else
        {
            xapi_fin_response(pXapireqe,
                              (char*) pQuery_Response,
                              qmscResponseSize);
        }
    }

    return queryRC;
}



