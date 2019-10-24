/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_qscr_mnt_info.c                             */
/** Description:    t_http server executable XAPI query_scr_mnt_info */
/**                 response generator.                              */
/**                                                                  */
/**                 NOTE: query_scr_mnt_info also handles ACSAPI     */
/**                 QUERY MOUNT SCRATCH PINFO type requests.         */
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
/** Function Name: http_qscr_mnt_info                                */
/** Description:   t_http server executable XAPI query_scr_mnt_info  */
/**                response generator.                               */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_qscr_mnt_info"

int http_qscr_mnt_info(struct HTTPCVT *pHttpcvt,
                       int             socketId,
                       int             seqNum,
                       void           *pRequestXmlparse)
{
    struct XMLPARSE    *pXmlparse           = (struct XMLPARSE*) pRequestXmlparse;
    int                 lastRC;
    struct XMLELEM     *pSubpoolNameXmlelem = NULL;
    struct XMLELEM     *pMgmtClassXmlelem   = NULL;
    char                subpoolNameString[14];
    char                mgmtClassString[9];
    char                queryScrMntFilename[64];

    pSubpoolNameXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                  pXmlparse->pHocXmlelem,
                                                  XNAME_subpool_name);

    pMgmtClassXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                pXmlparse->pHocXmlelem,
                                                XNAME_management_class);

    TRMSGI(TRCI_SERVER,
           "Entered, socketId=%d, seqNum=%d, "
           "pSubpoolNameXmlelem=%08X, pMgmtClassXmlelem=%08X\n",
           socketId,
           seqNum,
           pSubpoolNameXmlelem,
           pMgmtClassXmlelem);

    /*****************************************************************/
    /* A query_scr_mnt_info request received by the HTTP server      */
    /* will be for a specific <subpool_name>.                        */
    /*                                                               */
    /* ACSAPI QUERY MOUNT SCRATCH PINFO requests also have a         */
    /* <management_class> name specified.                            */
    /*****************************************************************/
    if (pSubpoolNameXmlelem == NULL)
    {
        lastRC = http_gen_resp(pHttpcvt,
                               socketId,
                               seqNum,
                               "invalid_command.resp");
    }
    else
    {
        memset(subpoolNameString, 0, sizeof(subpoolNameString));

        memcpy(subpoolNameString,
               pSubpoolNameXmlelem->pContent,
               pSubpoolNameXmlelem->contentLen);

        if (pMgmtClassXmlelem == NULL)
        {
            strcpy(queryScrMntFilename, "query_scr_mnt_info_");
            strcat(queryScrMntFilename, subpoolNameString);
            strcat(queryScrMntFilename, ".resp");
        }
        else
        {
            memset(mgmtClassString, 0, sizeof(mgmtClassString));

            memcpy(mgmtClassString,
                   pMgmtClassXmlelem->pContent,
                   pMgmtClassXmlelem->contentLen);

            strcpy(queryScrMntFilename, "query_scr_mnt_pinfo_");
            strcat(queryScrMntFilename, subpoolNameString);
            strcat(queryScrMntFilename, "_");
            strcat(queryScrMntFilename, mgmtClassString);
            strcat(queryScrMntFilename, ".resp");
        }

        lastRC = http_gen_resp(pHttpcvt,
                               socketId,
                               seqNum,
                               queryScrMntFilename);
    }

    return lastRC;
}



