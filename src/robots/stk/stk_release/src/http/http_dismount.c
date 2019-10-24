/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_dismount.c                                  */
/** Description:    t_http server executable XAPI dismount           */
/**                 response generator.                              */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     10/15/11                          */
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
/** Function Name: http_dismount                                     */
/** Description:   t_http server executable XAPI dismount            */
/**                response generator.                               */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_dismount"

int http_dismount(struct HTTPCVT *pHttpcvt,
                  int             socketId,
                  int             seqNum,
                  void           *pRequestXmlparse)
{
    struct XMLPARSE    *pXmlparse           = (struct XMLPARSE*) pRequestXmlparse;
    int                 lastRC;
    struct XMLELEM     *pVolserXmlelem      = NULL;
    struct XMLELEM     *pDriveLocIdXmlelem  = NULL;
    char                volserString[7];
    char                driveLocIdString[17];
    char                dismountFilename[64];

    pVolserXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                             pXmlparse->pHocXmlelem,
                                             XNAME_volser);

    pDriveLocIdXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                 pXmlparse->pHocXmlelem,
                                                 XNAME_drive_location_id);

    TRMSGI(TRCI_SERVER,
           "Entered, socketId=%d, seqNum=%d, "
           "pVolserXmlelem=%08X, pDriveLocIdXmlelem=%08X\n",
           socketId,
           seqNum,
           pVolserXmlelem,
           pDriveLocIdXmlelem);

    /*****************************************************************/
    /* A dismount request for a non-scratch volume received by the   */
    /* HTTP server will specify both <drive_location_id> and         */
    /* volser>.                                                      */
    /*****************************************************************/
    if (pVolserXmlelem == NULL)
    {
        lastRC = http_gen_resp(pHttpcvt,
                               socketId,
                               seqNum,
                               "invalid_command.resp");
    }
    else
    {
        memset(volserString, 0, sizeof(volserString));

        memcpy(volserString,
               pVolserXmlelem->pContent,
               pVolserXmlelem->contentLen);

        if (pDriveLocIdXmlelem != NULL)
        {
            memset(driveLocIdString, 0, sizeof(driveLocIdString));

            memcpy(driveLocIdString,
                   pDriveLocIdXmlelem->pContent,
                   pDriveLocIdXmlelem->contentLen);

            strcpy(dismountFilename, "dismount_");
            strcat(dismountFilename, volserString);
            strcat(dismountFilename, "_");
            strcat(dismountFilename, driveLocIdString);
            strcat(dismountFilename, ".resp");

            lastRC = http_gen_resp(pHttpcvt,
                                   socketId,
                                   seqNum,
                                   dismountFilename);
        }
        else
        {
            lastRC = http_gen_resp(pHttpcvt,
                                   socketId,
                                   seqNum,
                                   "invalid_command.resp");
        }
    }

    return lastRC;
}



