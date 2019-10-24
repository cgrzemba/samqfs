/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */ 
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qdrv_group.c                                */
/** Description:    XAPI QUERY DRIVE GROUP processor.                */
/**                                                                  */
/**                 Return virtual tape drive ids.                   */
/**                                                                  */
/**                 NOTE: Currently, there is no XAPI                */
/**                 QUERY DRIVE GROUP transaction that is sent       */
/**                 to the XAPI server (nor is there a need for one).*/
/**                                                                  */
/**                 QUERY DRIVE GROUP is processed locally by        */
/**                 returning virtual tape drive information from    */
/**                 the XAPI client drive configuration table        */
/**                 (XAPICFG).                                       */
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
/* Structures:                                                       */
/*********************************************************************/
struct DRGCOUNT
{
    char                vtssName[XAPI_VTSSNAME_SIZE];   
    int                 driveCount;    
    char                vtssNameString[XAPI_VTSSNAME_SIZE + 1];
};


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qdrv_group                                   */
/** Description:   XAPI QUERY DRIVE GROUP processor.                 */
/**                                                                  */
/** Return virtual tape drive ids for the specified VTSS names       */
/** or for all VTSS names.                                           */
/**                                                                  */
/** The XAPICFG is the XAPI client drive configuration table.        */
/** There is one XAPICFG table entry for each TapePlex real or       */
/** virtual tape drive that is accessible from the XAPI client.      */
/** Therefore, the XAPICFG contains a subset of all TapePlex         */
/** real and virtual tape drives.                                    */
/**                                                                  */
/** The QUERY DRIVE GROUP request is processed by returning          */
/** the XAPICFG virtual tape drive information (if any) for          */
/** the specified VTSS names (or for all VTSS names if the           */
/** request VTSS name count is 0).                                   */
/**                                                                  */
/** The QUERY DRIVE GROUP command is allowed to proceed even when    */
/** the XAPI client is in the IDLE state.                            */
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY DRIVE GROUP request consists of:                */
/**==================================================================*/
/** 1.   REQUEST_HEADER data consisting of:                          */
/**      i.   IPC_HEADER                                             */
/**      ii.  MESSAGE_HEADER, and the                                */
/** 2.   QUERY DRIVE GROUP data consisting of:                       */
/**      i.   message_id                                             */
/**      ii.  groupType (which is always GROUP_TYPE_VTSS)            */
/**      iii. count                                                  */
/**      iv.  GROUPID[count] data entries consisting of:             */
/**           a.   group_id (8 character, blank filled VTSS name)    */
/**                                                                  */
/** NOTE: If count is 0, then all VTSS(s) or GROUPID(s) are returned.*/
/**                                                                  */
/**==================================================================*/
/** The ACSAPI QUERY DRIVE GROUP response consists of:               */
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
/**      ii.  QU_DRG_RESPONSE data consisting of:                    */
/**           a.   GROUPID consisting of:                            */
/**                i.   group_id (8 char, blank filled VTSS name)    */
/**           b.   GROUP_TYPE (always GROUP_TYPE_VTSS)               */
/**           c.   vir_drv_map_count (for this packet)               */
/**           d.   QU_VIRT_DRV_MAP[vir_drv_map_count] data entries   */
/**                consisting of:                                    */
/**                1.   DRIVEID consisting of:                       */
/**                     i.   drive_id.panel_id.lsm_id.acs            */
/**                     ii.  drive_id.panel_id.lsm_id.lsm            */
/**                     iii. drive_id.panel_id.panel                 */
/**                     iv.  drive_id.drive                          */
/**                2.   drive_addr (unsigned short CCUU address)     */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qdrv_group"

extern int xapi_qdrv_group(struct XAPICVT  *pXapicvt,
                           struct XAPIREQE *pXapireqe)
{
    int                 queryRC             = STATUS_SUCCESS;
    int                 qdrvgrpResponseSize;
    int                 i;
    int                 j; 
    int                 k; 
    int                 xapicfgStartIndex;

    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    QU_DRG_CRITERIA    *pQu_Drg_Criteria    = &(pQuery_Request->select_criteria.drive_group_criteria);

    int                 groupCount          = pQu_Drg_Criteria->drg_count;
    int                 maxDrvCount         = 0;
    int                 drvResponseCount;
    int                 drvRemainingCount;
    int                 drvPacketCount;
    int                 finalDrvCount;
    char                vtssInDrgcountFlag  = FALSE;

    struct XAPICFG     *pXapicfg            = pXapireqe->pXapicfg;

    QUERY_RESPONSE      wkQuery_Response;
    QUERY_RESPONSE     *pQuery_Response     = &wkQuery_Response;
    QU_DRG_RESPONSE    *pQu_Drg_Response    = &(pQuery_Response->status_response.drive_group_response);
    QU_VIRT_DRV_MAP    *pQu_Virt_Drv_Map    = &(pQu_Drg_Response->virt_drv_map[0]);

    struct DRGCOUNT     drgcount[MAX_ACS_NUMBER];

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY DRG request=%08X, size=%d, count=%d, "
           "sizeof QU_VIRT_DRV_MAP=%d (%08X)\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           groupCount,
           sizeof(QU_VIRT_DRV_MAP),
           sizeof(QU_VIRT_DRV_MAP));

    qdrvgrpResponseSize = (char*) pQu_Virt_Drv_Map -
                          (char*) pQuery_Response;

    xapi_query_init_resp(pXapireqe,
                         (char*) pQuery_Response,
                         qdrvgrpResponseSize);

    if (groupCount < 0)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          qdrvgrpResponseSize,
                          STATUS_COUNT_TOO_SMALL);

        return STATUS_COUNT_TOO_SMALL;
    }

    if (groupCount > MAX_DRG)
    {
        xapi_err_response(pXapireqe,
                          (char*) pQuery_Response,
                          qdrvgrpResponseSize,
                          STATUS_COUNT_TOO_LARGE);

        return STATUS_COUNT_TOO_LARGE;
    }

    xapi_ack_response(pXapireqe,
                      NULL,
                      0);

    /*****************************************************************/
    /* Build the DRGCOUNT table.                                     */
    /* If count=0; then populate the VTSS name(s) in the local       */
    /* DRGCOUNT table from the XAPICFG table.                        */
    /* Otherwise, populate the DRGCOUNT table from the input         */
    /* request.                                                      */
    /*****************************************************************/
    memset((char*) &(drgcount[0]), 0, sizeof(drgcount));

    if (groupCount == 0)
    {
        for (i = 0;
            i < pXapireqe->xapicfgCount;
            i++)
        {
            if (pXapicfg[i].vtssNameString[0] > ' ')
            {
                vtssInDrgcountFlag = FALSE;

                if (groupCount > 0)
                {
                    for (j = 0;
                        j < groupCount;
                        j++)
                    {
                        if (strcmp(pXapicfg[i].vtssNameString, 
                                   drgcount[j].vtssNameString) == 0)
                        {
                            vtssInDrgcountFlag = TRUE;

                            break;
                        }
                    }
                }

                if (!(vtssInDrgcountFlag))
                {
                    strcpy(drgcount[groupCount].vtssNameString,
                           pXapicfg[i].vtssNameString);

                    memset(drgcount[groupCount].vtssName, ' ', sizeof(drgcount[groupCount].vtssName));

                    memcpy(drgcount[groupCount].vtssName,
                           pXapicfg[i].vtssNameString,
                           strlen( pXapicfg[i].vtssNameString));

                    groupCount++;
                }
            }
        } 
    }
    else
    {
        for (i = 0;
            i < groupCount;
            i++)
        {
            memcpy(drgcount[i].vtssName,
                   pQu_Drg_Criteria->group_id[i].groupid,
                   sizeof(drgcount[i].vtssName));

            STRIP_TRAILING_BLANKS(drgcount[i].vtssName,
                                  drgcount[i].vtssNameString,
                                  sizeof(drgcount[i].vtssName));
        }
    }

    TRMSGI(TRCI_XAPI,
           "Output groupCount=%d\n",
           groupCount);

    /*****************************************************************/
    /* Once the DRGCOUNT table is populated with VTSS names, then    */
    /* run through the XAPICFG table to find the number of drives    */
    /* for each VTSS.                                                */
    /*****************************************************************/
    if (groupCount > 0)
    {
        for (i = 0;
            i < groupCount;
            i++)
        {
            for (j = 0;
                j < pXapireqe->xapicfgCount;
                j++)
            {
                if (strcmp(pXapicfg[j].vtssNameString, 
                           drgcount[i].vtssNameString) == 0)
                {
                    drgcount[i].driveCount++;
                }
            }
        }

        /*************************************************************/
        /* Find the maxDrvCount in the DRGCOUNT table.               */
        /*************************************************************/
        for (i = 0;
            i < groupCount;
            i++)
        {
            TRMSGI(TRCI_XAPI,
                   "GROUP=%s, drives=%d\n",
                   drgcount[i].vtssNameString,
                   drgcount[i].driveCount);

            if (drgcount[i].driveCount > maxDrvCount)
            {
                maxDrvCount = drgcount[i].driveCount;
            }
        }
    }

    TRMSGI(TRCI_XAPI,
           "maxDrvCount=%d, MAX_VTD_MAP=%d\n",
           maxDrvCount,
           MAX_VTD_MAP);

    if (maxDrvCount >= MAX_VTD_MAP)
    {
        qdrvgrpResponseSize = (char*) pQu_Virt_Drv_Map -
                              (char*) pQuery_Response +
                              ((sizeof(QU_VIRT_DRV_MAP)) * MAX_VTD_MAP);
    }
    else
    {
        qdrvgrpResponseSize = (char*) pQu_Virt_Drv_Map -
                              (char*) pQuery_Response +
                              ((sizeof(QU_VIRT_DRV_MAP)) * maxDrvCount);
    }

    /*****************************************************************/
    /* Build the ACSAPI responses.                                   */
    /*===============================================================*/
    /* Each response will contain information about 1 VTSS at a      */
    /* time.  If the request contains multiple VTSS(s), then         */
    /* the response will contain 1 or more intermediate responses,   */
    /* 1 for each VTSS.                                              */
    /*                                                               */
    /* If the response for a VTSS contains more than                 */
    /* MAX_VTD_MAP drives, then the response for that VTSS will      */
    /* contain 1 or more intermediate responses, before either       */
    /* the final intermediate response for that VTSS or the          */
    /* final final response for that VTSS (if the VTSS is the        */
    /* last VTSS requested).                                         */
    /*                                                               */
    /*****************************************************************/
    if (groupCount > 0)
    {
        for (i = 0;
            i < groupCount;
            i++)
        {
            drvResponseCount = drgcount[i].driveCount;
            drvRemainingCount = drvResponseCount;
            xapicfgStartIndex = 0; 

            while (1)
            {
                xapi_query_init_resp(pXapireqe,
                                     (char*) pQuery_Response,
                                     qdrvgrpResponseSize);

                pQuery_Response = (QUERY_RESPONSE*) pQuery_Response;
                pQu_Drg_Response = (QU_DRG_RESPONSE*) &(pQuery_Response->status_response);
                pQu_Virt_Drv_Map = (QU_VIRT_DRV_MAP*) &(pQu_Drg_Response->virt_drv_map[0]);

                if (drvRemainingCount > MAX_VTD_MAP)
                {
                    drvPacketCount = MAX_VTD_MAP;
                }
                else
                {
                    drvPacketCount = drvRemainingCount;
                }

                TRMSGI(TRCI_XAPI,
                       "At top of while for VTSS=%s; drvResponse=%d, drvRemaining=%d, "
                       "drvPacket=%d, MAX_VTD_MAP=%d\n",
                       drgcount[i].vtssNameString,
                       drvResponseCount,
                       drvRemainingCount,
                       drvPacketCount,
                       MAX_VTD_MAP);

                memcpy(pQu_Drg_Response->group_id.groupid,
                       drgcount[i].vtssName,
                       sizeof(drgcount[i].vtssName));

                pQu_Drg_Response->vir_drv_map_count = drvPacketCount;
                pQu_Drg_Response->group_type = GROUP_TYPE_VTSS;

                for (j = 0;
                    j < drvPacketCount;
                    j++)
                {
                    for (k = xapicfgStartIndex;
                        k < pXapireqe->xapicfgCount;
                        k++)
                    {
                        if (strcmp(drgcount[i].vtssNameString,
                                   pXapicfg[k].vtssNameString) == 0)
                        {
                            pQu_Virt_Drv_Map[j].drive_id.panel_id.lsm_id.acs = (ACS) pXapicfg[k].libdrvid.acs;
                            pQu_Virt_Drv_Map[j].drive_id.panel_id.lsm_id.lsm = (LSM) pXapicfg[k].libdrvid.lsm;
                            pQu_Virt_Drv_Map[j].drive_id.panel_id.panel = (PANEL) pXapicfg[k].libdrvid.panel; 
                            pQu_Virt_Drv_Map[j].drive_id.drive = (DRIVE) pXapicfg[k].libdrvid.driveNumber;   
                            pQu_Virt_Drv_Map[j].drive_addr = pXapicfg[k].driveName;
                            xapicfgStartIndex = k + 1;

                            break;
                        }
                    }
                }

                drvRemainingCount = drvRemainingCount - drvPacketCount;

                if (drvRemainingCount <= 0)
                {
                    finalDrvCount = drvPacketCount;

                    break;
                }

                xapi_int_response(pXapireqe,
                                  (char*) pQuery_Response,     
                                  qdrvgrpResponseSize);  
            }

            qdrvgrpResponseSize = (char*) pQu_Virt_Drv_Map -
                                  (char*) pQuery_Response +
                                  ((sizeof(QU_VIRT_DRV_MAP)) * finalDrvCount);

            if (i < (groupCount - 1))
            {
                xapi_int_response(pXapireqe,
                                  (char*) pQuery_Response,
                                  qdrvgrpResponseSize);
            }
            else
            {
                xapi_fin_response(pXapireqe,
                                  (char*) pQuery_Response,
                                  qdrvgrpResponseSize);
            }
        }
    }
    else
    {
        xapi_fin_response(pXapireqe,
                          (char*) pQuery_Response,
                          qdrvgrpResponseSize);
    }

    return queryRC;
}



