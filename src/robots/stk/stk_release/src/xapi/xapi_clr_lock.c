/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_clr_lock.c                                  */
/** Description:    XAPI CLEAR LOCK processor.                       */
/**                                                                  */
/**                 Remove all current and pending LOCK(s) from      */
/**                 the specified drive(s) or volume(s).             */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     09/15/11                          */
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
#define LOCKERR_RESPONSE_SIZE (offsetof(CLEAR_LOCK_RESPONSE, identifier_status)) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_clr_lock                                     */
/** Description:   The XAPI CLEAR LOCK processor.                    */
/**                                                                  */
/** Remove all current and pending LOCK(s) from specified            */
/** drive(s) or volume(s).                                           */
/**                                                                  */
/** There is no COMMAND_CLEAR_LOCK_DRIVE or                          */
/** COMMAND_CLEAR_LOCK_VOLUME #define.  DRIVE and VOLUME are         */ 
/** subtypes processed under the COMMAND_CLEAR_LOCK #define.         */
/**                                                                  */
/** This program is a routing function that calls either             */
/** xapi_clr_lock_drv or xapi_clr_lock_vol.                          */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_clr_lock"

extern int xapi_clr_lock(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    CLEAR_LOCK_REQUEST *pClear_Lock_Request = (CLEAR_LOCK_REQUEST*) pXapireqe->pAcsapiBuffer;
    char                wkLockerrResponse[LOCKERR_RESPONSE_SIZE];

    TRMSGI(TRCI_XAPI,
           "Entered; CLEAR LOCK request=%08X, size=%d, type=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pClear_Lock_Request->type);

    switch (pClear_Lock_Request->type)
    {
    case TYPE_DRIVE:

        lastRC = xapi_clr_lock_drv(pXapicvt,
                                   pXapireqe);

        break;

    case TYPE_VOLUME:

        lastRC = xapi_clr_lock_vol(pXapicvt,
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


