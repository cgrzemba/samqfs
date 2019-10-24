/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_qlsm.c                                      */
/** Description:    t_http server executable XAPI query_lsm          */
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
/** Function Name: http_qlsm                                         */
/** Description:   t_http server executable XAPI query_lsm           */
/**                response generator.                               */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_qlsm"

int http_qlsm(struct HTTPCVT *pHttpcvt,
              int             socketId,
              int             seqNum,
              void           *pRequestXmlparse)
{
    struct XMLPARSE    *pXmlparse           = (struct XMLPARSE*) pRequestXmlparse;
    int                 lastRC;

    TRMSGI(TRCI_SERVER,
           "Entered, socketId=%d, seqNum=%d\n",
           socketId,
           seqNum);

    /*****************************************************************/
    /* Assume the query_lsm request is a query request to return     */
    /* all lsm(s).                                                   */
    /*****************************************************************/
    lastRC = http_gen_resp(pHttpcvt,
                           socketId,
                           seqNum,
                           "query_lsm.resp");

    return lastRC;
}
