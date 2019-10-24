/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_set_clean.c                                 */
/** Description:    XAPI SET CLEAN processor.                        */
/**                                                                  */
/**                 NOTE: SET CLEAN (set cleaning cartridge          */
/**                 attribute) commands are not supported by         */
/**                 the XAPI (nor can modern cleaning cartrige       */
/**                 attributes be set at all).  However the          */
/**                 STATUS_UNSUPPORTED_COMMAND logic of              */
/**                 xapi_main was unsuited for SET CLEAN error       */
/**                 responses so this stub was created to return     */
/**                 the approriate SET CLEAN error response.         */
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

#include "srvcommon.h"
#include "xapi.h"
#include "api/defs_api.h"
#include "csi.h"


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_set_clean                                    */
/** Description:   The XAPI SET CLEAN processor.                     */
/**                                                                  */
/** Build and output an ACSAPI SET CLEAN error response.             */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_set_clean"

extern int xapi_set_clean(struct XAPICVT  *pXapicvt,
                          struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    int                 setCleanResponseSize = 
    offsetof(SET_CLEAN_RESPONSE, volume_status[0]); 

    SET_CLEAN_REQUEST  *pSet_Clean_Request   = (SET_CLEAN_REQUEST*) pXapireqe->pAcsapiBuffer;
    SET_CLEAN_RESPONSE  wkSet_Clean_Response;
    SET_CLEAN_RESPONSE *pSet_Clean_Response  = &(wkSet_Clean_Response);

    TRMSGI(TRCI_XAPI,
           "Entered; SET CLEAN request=%08X, size=%d, "
           "SET_CLEAN response=%08X, size=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pSet_Clean_Response,
           setCleanResponseSize);

    memset((char*) pSet_Clean_Response, 
           0, 
           setCleanResponseSize);

    memcpy((char*) &(pSet_Clean_Response->request_header),
           (char*) &(pSet_Clean_Request->request_header),
           sizeof(REQUEST_HEADER));

    pSet_Clean_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pSet_Clean_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pSet_Clean_Response->message_status.status = STATUS_PROCESS_FAILURE;
    pSet_Clean_Response->message_status.type = TYPE_NONE;
    pSet_Clean_Response->max_use = pSet_Clean_Request->max_use;
    pSet_Clean_Response->count = 0;

    lastRC = xapi_idle_test(pXapicvt,
                            pXapireqe,
                            (char*) pSet_Clean_Response,
                            setCleanResponseSize);

    if (lastRC != STATUS_SUCCESS)
    {
        return lastRC;
    }

    xapi_err_response(pXapireqe,
                      (char*) pSet_Clean_Response,
                      setCleanResponseSize,
                      STATUS_UNSUPPORTED_COMMAND); 

    return STATUS_UNSUPPORTED_COMMAND;
}


