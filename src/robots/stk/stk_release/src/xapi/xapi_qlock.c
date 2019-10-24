/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qlock.c                                     */
/** Description:    XAPI QUERY LOCK processor.                       */
/**                                                                  */
/**                 Return the specified drive or volume lock        */
/**                 information.                                     */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     06/15/11                          */
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

#include "srvcommon.h"
#include "xapi.h"
#include "api/defs_api.h"
#include "csi.h"


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#define QUERYERR_RESPONSE_SIZE (offsetof(QUERY_LOCK_RESPONSE, identifier_status)) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_qlock                                        */
/** Description:   The XAPI QUERY LOCK processor.                    */
/**                                                                  */
/** Return the specified drive or volume lock information.           */
/**                                                                  */
/** There is no COMMAND_LOCK_DRIVE or COMMAND_LOCK_VOLUME            */
/** #define.  DRIVE and VOLUME are subtypes processed under          */
/** the COMMAND_LOCK #define.                                        */
/**                                                                  */
/** This program is a routing function that calls either             */
/** xapi_qlock_drv or xapi_qlock_vol.                                */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qlock"

extern int xapi_qlock(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    QUERY_LOCK_REQUEST *pQuery_Lock_Request = (QUERY_LOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    char                wkQueryResponse[QUERYERR_RESPONSE_SIZE];

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY LOCK request=%08X, size=%d, type=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pQuery_Lock_Request->type);

    switch (pQuery_Lock_Request->type)
    {
    case TYPE_DRIVE:

        lastRC = xapi_qlock_drv(pXapicvt,
                                pXapireqe);

        break;

    case TYPE_VOLUME:

        lastRC = xapi_qlock_vol(pXapicvt,
                                pXapireqe);

        break;

    default:

        xapi_qlock_init_resp(pXapireqe, 
                             wkQueryResponse,
                             QUERYERR_RESPONSE_SIZE);

        xapi_err_response(pXapireqe,
                          wkQueryResponse,
                          QUERYERR_RESPONSE_SIZE,
                          STATUS_UNSUPPORTED_TYPE); 

        lastRC = STATUS_INVALID_TYPE;
    } 

    return lastRC;
}



