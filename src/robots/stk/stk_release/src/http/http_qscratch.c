/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_qscratch.c                                  */
/** Description:    t_http server executable XAPI query_scratch      */
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
/** Function Name: http_qscratch                                     */
/** Description:   t_http server executable XAPI query_scratch       */
/**                response generator.                               */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_qscratch"

int http_qscratch(struct HTTPCVT *pHttpcvt,
                  int             socketId,
                  int             seqNum,
                  void           *pRequestXmlparse)
{
    struct XMLPARSE    *pXmlparse           = (struct XMLPARSE*) pRequestXmlparse;
    int                 lastRC;
    struct XMLELEM     *pCsvBreakXmlelem    = NULL;

    pCsvBreakXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                               pXmlparse->pHocXmlelem,
                                               XNAME_csv_break_tag);


    TRMSGI(TRCI_SERVER,
           "Entered, socketId=%d, seqNum=%d, pCsvBreakXmlelem=%08X\n",
           socketId,
           seqNum,
           pCsvBreakXmlelem);

    /*****************************************************************/
    /* Any query_scratch request received by the HTTP server should  */
    /* request CSV and will always return the complete query_scatch  */
    /* fixed format CSV response.                                    */
    /*****************************************************************/
    if (pCsvBreakXmlelem == NULL)
    {
        lastRC = http_gen_resp(pHttpcvt,
                               socketId,
                               seqNum,
                               "invalid_command.resp");

        lastRC = STATUS_INVALID_COMMAND;
    }
    else
    {
        lastRC = http_gen_resp(pHttpcvt,
                               socketId,
                               seqNum,
                               "csv_query_scratch.resp");
    }

    return lastRC;
}
