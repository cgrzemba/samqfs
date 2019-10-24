/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qvol.c                                      */
/** Description:    XAPI QUERY VOLUME processor.                     */
/**                                                                  */
/**                 Return volume information and status.            */
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
#define QVOLERR_RESPONSE_SIZE (offsetof(QUERY_RESPONSE, status_response)) + \
                              (offsetof(QU_VOL_RESPONSE, volume_status[0])) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qvol                                         */
/** Description:   The XAPI QUERY VOLUME processor.                  */
/**                                                                  */
/** Return volume information and status from the TapePlex server.   */
/**                                                                  */
/** If the ACSAPI QUERY VOLUME request count is 0 then call          */
/** the XAPI client VOLRPT service (xapi_qvol_all);  There will be   */
/** a single XAPI request generated for the VOLRPT equivalent        */
/** transaction.                                                     */
/**                                                                  */
/** Otherwise, call the XAPI client query volume service             */
/** (xapi_qvol_one) for each volume in the request;  There will      */
/** be multiple XAPI requests generated, one ror each volume         */
/** in the request;  For each call to xapi_qvol_one, translate       */
/** translate the returned RAWVOLUME structure into the              */
/** ACSAPI QUERY VOLUME response.                                    */
/**                                                                  */
/** The QUERY VOLUME command is allowed to proceed even when         */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY VOLUME request consists of:                     */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY_REQUEST data consisting of:                           */
/**      i.   type                                                   */
/** 3.   QU_VOL_CRITERIA data consisting of:                         */
/**      i.   count (of VOLID(s) requested; will be 0 for ALL)       */
/**      ii.  VOLID[count] entries consisting of:                    */
/**           a.   external_label (6 character volser)               */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY VOLUME response consists of:                    */
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
/**      ii.  QU_VOL_RESPONSE data consisting of:                    */
/**           a.   count                                             */
/**           b.   QU_VOL_STATUS[count] data entries consisting of:  */
/**                1.   VOLID consisting of:                         */
/**                     i.    external_label (6 character volser)    */
/**                2.   media_type                                   */
/**                3.   location_type                                */
/**                4.   CELLID consisting of:                        */
/**                     i.    panel_id.lsmid.acs                     */
/**                     ii.   panel_id.lsmid.lsm                     */
/**                     iii.  panel_id.panel.row                     */
/**                     iv.   panel_id.panel.col                     */
/**                     v.    column                                 */
/**                5.   status                                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qvol"

extern int xapi_qvol(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 qvolumeResponseSize;
    int                 i;
    int                 locInt;

    char                mediaNameString[XAPI_MEDIA_NAME_SIZE + 1];

    struct XAPIMEDIA   *pXapimedia;
    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_VOL_CRITERIA    *pQu_Vol_Criteria    = &(pQuery_Request->select_criteria.vol_criteria);

    int                 volCount            = pQu_Vol_Criteria->volume_count;
    char                volser[6];
    char                vtssNameString[XAPI_VTSSNAME_SIZE + 1];

    struct RAWVOLUME    rawvolume;
    struct RAWVOLUME   *pRawvolume          = &rawvolume;
    struct XAPICFG     *pXapicfg;

    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;
    QU_VOL_RESPONSE    *pQu_Vol_Response    = &(pQuery_Response->status_response.volume_response);
    QU_VOL_STATUS      *pQu_Vol_Status      = &(pQu_Vol_Response->volume_status[0]);

    /*****************************************************************/
    /* If this is a QUERY VOL ALL, then call xapi_qvol_all.c         */
    /* which requests CSV to limit the size of the response.         */
    /*****************************************************************/
    if (volCount == 0)
    {
        queryRC = xapi_qvol_all(pXapicvt,
                                pXapireqe);

        return queryRC;
    }

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         sizeof(QUERY_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY VOLUME request=%08X, size=%d, "
           "count=%d, MAX_ID=%d, "
           "QUERY VOLUME response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           volCount,
           MAX_ID,
           pQuery_Response,
           sizeof(QU_VOL_RESPONSE));

    if (volCount < 0)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QVOLERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_SMALL); 

        return STATUS_COUNT_TOO_SMALL;
    }

    if (volCount > MAX_ID)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QVOLERR_RESPONSE_SIZE,
                          STATUS_COUNT_TOO_LARGE); 

        return STATUS_COUNT_TOO_LARGE;
    }

    qvolumeResponseSize = (char*) pQu_Vol_Status -
                          (char*) pQuery_Response +
                          ((sizeof(QU_VOL_STATUS)) * MAX_ID);

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

    /*****************************************************************/
    /* Call the xapi_qvol_one service for each volume in the request.*/
    /*****************************************************************/
    pQu_Vol_Response->volume_count = volCount;

    for (i = 0;
        i < volCount;
        i++, pQu_Vol_Status++)
    {
        memset(pRawvolume, 0, sizeof(struct RAWVOLUME));

        memcpy(volser,
               &(pQu_Vol_Criteria->volume_id[i]),
               sizeof(volser));

        lastRC = xapi_qvol_one(pXapicvt,
                               pXapireqe,
                               pRawvolume,
                               NULL,
                               volser);

        TRMSGI(TRCI_XAPI,
               "lastRC=%i from xapi_qvol_one(%.6s)\n",
               lastRC,
               volser);

        if (lastRC != STATUS_SUCCESS)
        {
            queryRC = lastRC;

            break;
        }
        else
        {
            if (pRawvolume->volser[0] > ' ')
            {
                memcpy(pQu_Vol_Status->vol_id.external_label,
                       pRawvolume->volser,
                       sizeof(pRawvolume->volser));
            }
            else
            {
                memcpy(pQu_Vol_Status->vol_id.external_label,
                       "??????",
                       sizeof(pQu_Vol_Status->vol_id.external_label));
            }

            if (pRawvolume->media[0] >= ' ')
            {
                STRIP_TRAILING_BLANKS(pRawvolume->media,
                                      mediaNameString,
                                      sizeof(pRawvolume->media));

                pXapimedia = xapi_media_search_name(pXapicvt,
                                                    pXapireqe,
                                                    mediaNameString);

                if (pXapimedia != NULL)
                {
                    pQu_Vol_Status->media_type = 
                    (MEDIA_TYPE) pXapimedia->acsapiMediaType;
                }
                else
                {
                    pQu_Vol_Status->media_type = ANY_MEDIA_TYPE;
                }
            }

            if (toupper(pRawvolume->result[0]) == 'F')
            {
                pQu_Vol_Status->status = STATUS_VOLUME_NOT_IN_LIBRARY;
            }
            else
            {
                pQu_Vol_Status->status = STATUS_SUCCESS;
            }

            /*********************************************************/
            /* If VIRTUAL, then generate a "dummy" location.         */
            /*********************************************************/
            if (memcmp(pRawvolume->media,
                       "VIRTUAL",
                       7) == 0)
            {
                if (pRawvolume->driveLocId[0] > ' ')
                {
                    pXapicfg = xapi_config_search_drivelocid(pXapicvt,
                                                             pXapireqe,
                                                             pRawvolume->driveLocId);

                    if (pXapicfg != NULL)
                    {
                        pQu_Vol_Status->location_type = LOCATION_DRIVE;
                        pQu_Vol_Status->location.drive_id.panel_id.lsm_id.acs = (ACS) pXapicfg->libdrvid.acs;
                        pQu_Vol_Status->location.drive_id.panel_id.lsm_id.lsm = (LSM) pXapicfg->libdrvid.lsm;
                        pQu_Vol_Status->location.drive_id.panel_id.panel = (PANEL) pXapicfg->libdrvid.panel;
                        pQu_Vol_Status->location.drive_id.drive = (DRIVE) pXapicfg->libdrvid.driveNumber;
                    }
                }
                else if (pRawvolume->residentVtss[0] > ' ')
                {
                    STRIP_TRAILING_BLANKS(pRawvolume->residentVtss,
                                          vtssNameString,
                                          sizeof(pRawvolume->residentVtss));

                    pXapicfg = xapi_config_search_vtss(pXapicvt,
                                                       pXapireqe,
                                                       vtssNameString);

                    if (pXapicfg != NULL)
                    {
                        pQu_Vol_Status->location_type = LOCATION_CELL;
                        pQu_Vol_Status->location.cell_id.panel_id.lsm_id.acs = (ACS) pXapicfg->libdrvid.acs;
                        pQu_Vol_Status->location.cell_id.panel_id.lsm_id.lsm = (LSM) 0;
                        pQu_Vol_Status->location.cell_id.panel_id.panel = (PANEL) 0;
                        pQu_Vol_Status->location.cell_id.row = (ROW) 0;
                        pQu_Vol_Status->location.cell_id.col = (COL) 0;
                    }
                }
            }
            /*********************************************************/
            /* If "real" then extract the LIBRARY location.          */
            /*********************************************************/
            else
            {
                if (pRawvolume->cellAcs[0] > ' ')
                {
                    pQu_Vol_Status->location_type = LOCATION_CELL;

                    FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->cellAcs,
                                                  sizeof(pRawvolume->cellAcs),
                                                  &locInt);

                    pQu_Vol_Status->location.cell_id.panel_id.lsm_id.acs = (ACS) locInt;

                    if (pRawvolume->cellLsm[0] > ' ')
                    {
                        FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->cellLsm,
                                                      sizeof(pRawvolume->cellLsm),
                                                      &locInt);

                        pQu_Vol_Status->location.cell_id.panel_id.lsm_id.lsm = (ACS) locInt;

                        if (pRawvolume->cellPanel[0] > ' ')
                        {
                            FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->cellPanel,
                                                          sizeof(pRawvolume->cellPanel),
                                                          &locInt);

                            pQu_Vol_Status->location.cell_id.panel_id.panel = (PANEL) locInt;

                            if (pRawvolume->cellRow[0] > ' ')
                            {
                                FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->cellRow,
                                                              sizeof(pRawvolume->cellRow),
                                                              &locInt);

                                pQu_Vol_Status->location.cell_id.row = (ROW) locInt;

                                if (pRawvolume->cellColumn[0] > ' ')
                                {
                                    FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->cellColumn,
                                                                  sizeof(pRawvolume->cellColumn),
                                                                  &locInt);

                                    pQu_Vol_Status->location.cell_id.col = (COL) locInt;
                                }
                            }
                        }
                    }
                }
                else if (pRawvolume->driveAcs[0] > ' ')
                {
                    pQu_Vol_Status->location_type = LOCATION_DRIVE;

                    FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->driveAcs,
                                                  sizeof(pRawvolume->driveAcs),
                                                  &locInt);

                    pQu_Vol_Status->location.drive_id.panel_id.lsm_id.acs = (ACS) locInt;

                    if (pRawvolume->driveLsm[0] > ' ')
                    {
                        FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->driveLsm,
                                                      sizeof(pRawvolume->driveLsm),
                                                      &locInt);

                        pQu_Vol_Status->location.drive_id.panel_id.lsm_id.lsm = (ACS) locInt;

                        if (pRawvolume->drivePanel[0] > ' ')
                        {
                            FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->drivePanel,
                                                          sizeof(pRawvolume->drivePanel),
                                                          &locInt);

                            pQu_Vol_Status->location.drive_id.panel_id.panel = (PANEL) locInt;

                            if (pRawvolume->driveNumber[0] > ' ')
                            {
                                FN_CONVERT_DIGITS_TO_FULLWORD(pRawvolume->driveNumber,
                                                              sizeof(pRawvolume->driveNumber),
                                                              &locInt);

                                pQu_Vol_Status->location.drive_id.drive = (DRIVE) locInt;
                            }
                        }
                    }
                }
            } 
        }
    }

    if (queryRC != STATUS_SUCCESS)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          QVOLERR_RESPONSE_SIZE,
                          queryRC);
    }
    else
    {
        qvolumeResponseSize = (char*) pQu_Vol_Status -
                              (char*) pQuery_Response +
                              ((sizeof(QU_VOL_STATUS)) * volCount);

        xapi_fin_response(pXapireqe,
                          (char*) pQuery_Response,
                          qvolumeResponseSize);
    }

    return queryRC;
}




