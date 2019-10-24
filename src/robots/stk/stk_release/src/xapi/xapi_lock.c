/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_lock.c                                      */
/** Description:    XAPI LOCK processor.                             */
/**                                                                  */
/**                 Lock the specified drive(s) or volume(s).        */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     09/01/11                          */
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
#define LOCKERR_RESPONSE_SIZE (offsetof(LOCK_RESPONSE, identifier_status)) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_lock                                         */
/** Description:   The XAPI LOCK processor.                          */
/**                                                                  */
/** Lock the specified drive(s) or volume(s).                        */
/**                                                                  */
/** There is no COMMAND_LOCK_DRIVE or COMMAND_LOCK_VOLUME            */
/** #define.  DRIVE and VOLUME are subtypes processed under          */
/** the COMMAND_LOCK #define.                                        */
/**                                                                  */
/** This program is a routing function that calls either             */
/** xapi_lock_drv or xapi_lock_vol.                                  */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_lock"

extern int xapi_lock(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    LOCK_REQUEST       *pLock_Request       = (LOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    char                wkLockerrResponse[LOCKERR_RESPONSE_SIZE];

    TRMSGI(TRCI_XAPI,
           "Entered; LOCK request=%08X, size=%d, type=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pLock_Request->type);

    switch (pLock_Request->type)
    {
    case TYPE_DRIVE:

        lastRC = xapi_lock_drv(pXapicvt,
                               pXapireqe);

        break;

    case TYPE_VOLUME:

        lastRC = xapi_lock_vol(pXapicvt,
                               pXapireqe);

        break;

    default:

        xapi_lock_init_resp(pXapireqe, 
                            wkLockerrResponse,
                            LOCKERR_RESPONSE_SIZE);

        xapi_err_response(pXapireqe,
                          wkLockerrResponse,
                          LOCKERR_RESPONSE_SIZE,
                          STATUS_UNSUPPORTED_TYPE); 

        lastRC = STATUS_INVALID_TYPE;
    } 

    return lastRC;
}



