/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_qvolume_info.c                              */
/** Description:    t_http server executable XAPI query_volume_info  */
/**                 response generator.                              */
/**                                                                  */
/**                 NOTE: query_volume_info also handles ACSAPI      */
/**                 QUERY MOUNT type requests.                       */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     08/15/11                          */
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
#include "srvcommon.h"
#include "xapi/http.h"
#include "xapi/smccxmlx.h"
#include "xapi/xapi.h"


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: http_qvolume_info                                 */
/** Description:   t_http server executable XAPI query_volume_info   */
/**                response generator.                               */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_qvolume_info"

int http_qvolume_info(struct HTTPCVT *pHttpcvt,
                      int             socketId,
                      int             seqNum,
                      void           *pRequestXmlparse)
{
    struct XMLPARSE    *pXmlparse           = (struct XMLPARSE*) pRequestXmlparse;
    int                 lastRC;
    struct XMLELEM     *pCsvBreakXmlelem    = NULL;
    struct XMLELEM     *pFirstVolserXmlelem = NULL;
    struct XMLELEM     *pMaxDriveCountXmlelem = NULL;
    char                firstVolserString[7];
    char                queryVolumeFilename[64];

    pCsvBreakXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                               pXmlparse->pHocXmlelem,
                                               XNAME_csv_break_tag);

    pFirstVolserXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                  pXmlparse->pHocXmlelem,
                                                  XNAME_volser);

    pMaxDriveCountXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                    pXmlparse->pHocXmlelem,
                                                    XNAME_max_drive_count);

    TRMSGI(TRCI_SERVER,
           "Entered, socketId=%d, seqNum=%d, "
           "pCsvBreakXmlelem=%08X, pFirstVolserXmlelem=%08X, "
           "pMaxDriveCountXmlelem=%08X\n",
           socketId,
           seqNum,
           pCsvBreakXmlelem,
           pFirstVolserXmlelem,
           pMaxDriveCountXmlelem);

    /*****************************************************************/
    /* A query_volume_info request received by the HTTP server       */
    /* will be for the complete volrpt (i.e. CSV specified), or      */
    /* for a specific <volser>.                                      */
    /*                                                               */
    /* Additionally, if the request specifies <max_drive_count>      */
    /* then it is a QUERY MOUNT type of query_volume_info request.   */
    /*****************************************************************/
    if (pCsvBreakXmlelem == NULL)
    {
        if (pFirstVolserXmlelem == NULL)
        {
            lastRC = http_gen_resp(pHttpcvt,
                                   socketId,
                                   seqNum,
                                   "invalid_command.resp");
        }
        else
        {
            memcpy(firstVolserString,
                   pFirstVolserXmlelem->pContent,
                   6);

            firstVolserString[6] = 0;

            if (pMaxDriveCountXmlelem == NULL)
            {
                strcpy(queryVolumeFilename, "query_volume_info_");
                strcat(queryVolumeFilename, firstVolserString);
                strcat(queryVolumeFilename, ".resp");
            }
            else
            {
                strcpy(queryVolumeFilename, "query_mount_info_");
                strcat(queryVolumeFilename, firstVolserString);
                strcat(queryVolumeFilename, ".resp");
            }

            lastRC = http_gen_resp(pHttpcvt,
                                   socketId,
                                   seqNum,
                                   queryVolumeFilename);
        }
    }
    else
    {
        lastRC = http_gen_resp(pHttpcvt,
                               socketId,
                               seqNum,
                               "csv_query_volume_info.resp");
    }

    return lastRC;
}



