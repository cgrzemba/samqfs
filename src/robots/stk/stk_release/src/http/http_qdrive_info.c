/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_qdrive_info.c                               */
/** Description:    t_http server executable XAPI query_drive_info   */
/**                 response generator.                              */
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
/** Function Name: http_qdrive_info                                  */
/** Description:   t_http server executable XAPI query_drive_info    */
/**                response generator.                               */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_qdrive_info"

int http_qdrive_info(struct HTTPCVT *pHttpcvt,
                     int             socketId,
                     int             seqNum,
                     void           *pRequestXmlparse)
{
    struct XMLPARSE    *pXmlparse           = (struct XMLPARSE*) pRequestXmlparse;
    int                 lastRC;
    struct XMLELEM     *pCsvBreakXmlelem    = NULL;
    struct XMLELEM     *pDriveNameXmlelem    = NULL;
    char                driveNameString[5];
    char                queryDriveFileName[64];

    pCsvBreakXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                               pXmlparse->pHocXmlelem,
                                               XNAME_csv_break_tag);

    pDriveNameXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                pXmlparse->pHocXmlelem,
                                                XNAME_drive_name);

    TRMSGI(TRCI_SERVER,
           "Entered, socketId=%d, seqNum=%d, "
           "pCsvBreakXmlelem=%08X, pDriveNameXmlelem=%08X\n",
           socketId,
           seqNum,
           pCsvBreakXmlelem,
           pDriveNameXmlelem);

    /*****************************************************************/
    /* Any query_drive_info request received by the HTTP server      */
    /* that does not request CSV will request individual drives.     */
    /*****************************************************************/
    if (pCsvBreakXmlelem == NULL)
    {
        if (pDriveNameXmlelem == NULL)
        {
            lastRC = http_gen_resp(pHttpcvt,
                                   socketId,
                                   seqNum,
                                   "invalid_command.resp");

            lastRC = STATUS_INVALID_COMMAND;
        }
        else
        {
            memset(driveNameString, 0, sizeof(driveNameString));

            memcpy(driveNameString, 
                   pDriveNameXmlelem->pContent, 
                   pDriveNameXmlelem->contentLen);

            strcpy(queryDriveFileName, "query_drive_");
            strcat(queryDriveFileName, driveNameString);
            strcat(queryDriveFileName, ".resp");

            lastRC = http_gen_resp(pHttpcvt,
                                   socketId,
                                   seqNum,
                                   queryDriveFileName);
        }
    }
    /*****************************************************************/
    /* Otherwise, return the CSV response that returns the complete  */
    /* configuration.                                                */
    /*****************************************************************/
    else
    {
        lastRC = http_gen_resp(pHttpcvt,
                               socketId,
                               seqNum,
                               "csv_query_drive_info.resp");
    }

    return lastRC;
}
