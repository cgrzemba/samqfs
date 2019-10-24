/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_set_unscr.c                                 */
/** Description:    t_http server executable XAPI unscratch          */
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
/** Function Name: http_set_unscr                                    */
/** Description:   t_http server executable XAPI unscratch           */
/**                response generator.                               */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_set_unscr"

int http_set_unscr(struct HTTPCVT *pHttpcvt,
                   int             socketId,
                   int             seqNum,
                   void           *pRequestXmlparse)
{
    struct XMLPARSE    *pXmlparse           = (struct XMLPARSE*) pRequestXmlparse;
    int                 lastRC;
    struct XMLELEM     *pFirstVolserXmlelem = NULL;
    char                firstVolserString[7];
    char                unscratchFilename[64];

    pFirstVolserXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                  pXmlparse->pHocXmlelem,
                                                  XNAME_volser);

    TRMSGI(TRCI_SERVER,
           "Entered, socketId=%d, seqNum=%d, "
           "pFirstVolserXmlelem=%08X\n",
           socketId,
           seqNum,
           pFirstVolserXmlelem);

    /*****************************************************************/
    /* An unscratch request received by the HTTP server              */
    /* will be for a maximum of 42 volumes with a specific           */
    /* first volser.                                                 */
    /*****************************************************************/
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

        strcpy(unscratchFilename, "unscratch_");
        strcat(unscratchFilename, firstVolserString);
        strcat(unscratchFilename, ".resp");

        lastRC = http_gen_resp(pHttpcvt,
                               socketId,
                               seqNum,
                               unscratchFilename);
    }

    return lastRC;
}



