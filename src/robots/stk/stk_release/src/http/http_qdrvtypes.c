/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_qdrvtypes.c                                 */
/** Description:    t_http server executable XAPI query_drvtypes     */
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
/** Function Name: http_qdrvtypes                                    */
/** Description:   t_http server executable XAPI query_drvtypes      */
/**                response generator.                               */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_qdrvtypes"

int http_qdrvtypes(struct HTTPCVT *pHttpcvt,
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
    /* Any query_drvtypes request received by the HTTP server will   */
    /* always return the complete query_drvtypes response.           */
    /*****************************************************************/
    lastRC = http_gen_resp(pHttpcvt,
                           socketId,
                           seqNum,
                           "query_drvtypes.resp");

    return lastRC;
}
