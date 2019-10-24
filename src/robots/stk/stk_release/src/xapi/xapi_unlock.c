/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_unlock.c                                    */
/** Description:    XAPI UNLOCK processor.                           */
/**                                                                  */
/**                 Unlock the specified DRIVE(s) or VOLUME(s).      */
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
#define LOCKERR_RESPONSE_SIZE (offsetof(UNLOCK_RESPONSE, identifier_status)) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_unlock                                       */
/** Description:   The XAPI UNLOCK processor.                        */
/**                                                                  */
/** Unlock the specified DRIVE(s) or VOLUME(s).                      */
/**                                                                  */
/** There is no COMMAND_UNLOCK_DRIVE or COMMAND_UNLOCK_VOLUME        */
/** #define.  DRIVE and VOLUME are subtypes processed under          */
/** the COMMAND_UNLOCK #define.                                      */
/**                                                                  */
/** This program is a routing function that calls either             */
/** xapi_unlock_drv or xapi_unlock_vol.                              */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_unlock"

extern int xapi_unlock(struct XAPICVT  *pXapicvt,
                       struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    UNLOCK_REQUEST     *pUnlock_Request     = (UNLOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    char                wkLockerrResponse[LOCKERR_RESPONSE_SIZE];

    TRMSGI(TRCI_XAPI,
           "Entered; UNLOCK request=%08X, size=%d, type=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pUnlock_Request->type);

    switch (pUnlock_Request->type)
    {
    case TYPE_DRIVE:

        lastRC = xapi_unlock_drv(pXapicvt,
                                 pXapireqe);

        break;

    case TYPE_VOLUME:

        lastRC = xapi_unlock_vol(pXapicvt,
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



