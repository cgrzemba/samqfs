/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_qscrpool_info.c                             */
/** Description:    t_http server executable XAPI query_scrpool_info */
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
/** Function Name: http_qscrpool_info                                */
/** Description:   t_http server executable XAPI query_scrpool_info  */
/**                response generator.                               */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_qscrpool_info"

int http_qscrpool_info(struct HTTPCVT *pHttpcvt,
                       int             socketId,
                       int             seqNum,
                       void           *pRequestXmlparse)
{
    struct XMLPARSE    *pXmlparse           = (struct XMLPARSE*) pRequestXmlparse;
    int                 lastRC;
    struct XMLELEM     *pCsvBreakXmlelem    = NULL;
    struct XMLELEM     *pSubpoolNameXmlelem = NULL;
    char                subpoolNameString[14];
    char                queryScrpoolFileName[64];

    pCsvBreakXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                               pXmlparse->pHocXmlelem,
                                               XNAME_csv_break_tag);

    pSubpoolNameXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                  pXmlparse->pHocXmlelem,
                                                  XNAME_subpool_name);

    TRMSGI(TRCI_SERVER,
           "Entered, socketId=%d, seqNum=%d, "
           "pCsvBreakXmlelem=%08X, pSubpoolNameXmlelem=%08X\n",
           socketId,
           seqNum,
           pCsvBreakXmlelem,
           pSubpoolNameXmlelem);

    /*****************************************************************/
    /* A query_scrpool_info request received by the HTTP server      */
    /* will either be for (1) the complete summary (i.e. no CSV and  */
    /* no subpool_name), or for (2) the CSV detail for a specific    */
    /* subpool (both CSV and subpool_name specified).                */
    /*                                                               */
    /* NOTE: ACSAPI requests that request a summary for a specific   */
    /* subpool or subpool(s) are processed locally in the SSI-XAPI   */
    /* client and never reach the HTTP server.                       */
    /*****************************************************************/
    if (pCsvBreakXmlelem != NULL)
    {
        if (pSubpoolNameXmlelem == NULL)
        {
            lastRC = http_gen_resp(pHttpcvt,
                                   socketId,
                                   seqNum,
                                   "invalid_command.resp");

            lastRC = STATUS_INVALID_COMMAND;
        }
        else
        {
            memset(subpoolNameString, 0, sizeof(subpoolNameString));

            memcpy(subpoolNameString, 
                   pSubpoolNameXmlelem->pContent, 
                   pSubpoolNameXmlelem->contentLen);

            strcpy(queryScrpoolFileName, "csv_query_scrpool_");
            strcat(queryScrpoolFileName, subpoolNameString);
            strcat(queryScrpoolFileName, ".resp");

            lastRC = http_gen_resp(pHttpcvt,
                                   socketId,
                                   seqNum,
                                   queryScrpoolFileName);
        }
    }
    else
    {
        lastRC = http_gen_resp(pHttpcvt,
                               socketId,
                               seqNum,
                               "query_scrpool_info.resp");
    }

    return lastRC;
}
