/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_query.c                                     */
/** Description:    XAPI QUERY processor.                            */
/**                                                                  */
/**                 Route QUERY request based upon QUERY TYPE.       */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     05/11/11                          */
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
#define QERR_RESPONSE_SIZE     (offsetof(QUERY_RESPONSE, status_response)) 


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_query                                        */
/** Description:   The XAPI QUERY processor.                         */
/**                                                                  */
/** Route the ACSAPI QUERY command request to the appropriate        */
/** processing functiob based upon QUERY TYPE.                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_query"

extern int xapi_query(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe)
{
    int                 lastRC;
    QUERY_REQUEST      *pQuery_Request      = (QUERY_REQUEST*) pXapireqe->pAcsapiBuffer;
    char                wkQerrResponse[QERR_RESPONSE_SIZE];

    TRMSGI(TRCI_XAPI,
           "Entered; QUERY request=%08X, size=%d, type=%d\n",
           pXapireqe->pAcsapiBuffer,
           pXapireqe->acsapiBufferSize,
           pQuery_Request->type);

#ifdef DEBUG

    TRMSGI(TRCI_XAPI,
           "sizeof: QUERY_REQUEST=%d, QUERY_RESPONSE=%d, MAX_SIZE=%d\n",
           sizeof(QUERY_REQUEST),
           sizeof(QUERY_RESPONSE),
           MAX_MESSAGE_SIZE);

    TRMSGI(TRCI_XAPI,
           "sizeof: QU_SRV=%d, QU_ACS=%d, QU_LSM=%d, QU_CAP=%d, "
           "QU_CLN=%d, QU_DRV=%d, QU_MNT=%d, QU_VOL=%d\n",
           sizeof(QU_SRV_RESPONSE),
           sizeof(QU_ACS_RESPONSE),
           sizeof(QU_LSM_RESPONSE),
           sizeof(QU_CAP_RESPONSE),
           sizeof(QU_CLN_RESPONSE),
           sizeof(QU_DRV_RESPONSE),
           sizeof(QU_MNT_RESPONSE),
           sizeof(QU_VOL_RESPONSE));

    TRMSGI(TRCI_XAPI,
           "sizeof: QU_PRT=%d, QU_REQ=%d, QU_SCR=%d, QU_POL=%d, "
           "QU_MSC=%d, QU_MMI=%d, QU_LMU=%d, QU_DRG=%d, QU_SPN=%d\n",
           sizeof(QU_PRT_RESPONSE),
           sizeof(QU_REQ_RESPONSE),
           sizeof(QU_SCR_RESPONSE),
           sizeof(QU_POL_RESPONSE),
           sizeof(QU_MSC_RESPONSE),
           sizeof(QU_MMI_RESPONSE),
           sizeof(QU_LMU_RESPONSE),
           sizeof(QU_DRG_RESPONSE),
           sizeof(QU_SPN_RESPONSE));

#endif

    switch (pQuery_Request->type)
    {
    case TYPE_ACS:

        lastRC = xapi_qacs(pXapicvt,
                           pXapireqe);

        break;

    case TYPE_CAP:

        lastRC = xapi_qcap(pXapicvt,
                           pXapireqe);

        break;

        /*************************************************************/
        /* TYPE_CLEAN is not supported by the XAPI.                  */
        /*************************************************************/

    case TYPE_DRIVE:

        lastRC = xapi_qdrv(pXapicvt,
                           pXapireqe);

        break;

    case TYPE_DRIVE_GROUP:

        lastRC = xapi_qdrv_group(pXapicvt,
                                 pXapireqe);

        break;

        /*************************************************************/
        /* NOTE: There is no TYPE_LOCK_DRIVE or TYPE_LOCK_VOLUME     */
        /* #defines for the QUERY command:                           */
        /* COMMAND_QUERY_LOCK is its own unique command type with    */
        /* subtypes LOCK and DRIVE.  Therefore, QUERY LOCK DRIVE     */
        /* and QUERY LOCK VOLUME are split out at the COMMAND        */
        /* level in xapi_main, and are not part of xapi_query.       */
        /*************************************************************/

    case TYPE_LSM:

        lastRC = xapi_qlsm(pXapicvt,
                           pXapireqe);

        break;

    case TYPE_MIXED_MEDIA_INFO:

        lastRC = xapi_qmedia(pXapicvt,
                             pXapireqe);

        break;

    case TYPE_MOUNT:

        lastRC = xapi_qmnt(pXapicvt,
                           pXapireqe);

        break;

    case TYPE_MOUNT_SCRATCH:

        lastRC = xapi_qmnt_scr(pXapicvt,
                               pXapireqe);

        break;

    case TYPE_MOUNT_SCRATCH_PINFO:

        lastRC = xapi_qmnt_pinfo(pXapicvt,
                                 pXapireqe);

        break;

    case TYPE_POOL:

        lastRC = xapi_qpool(pXapicvt,
                            pXapireqe);

        break;

        /*************************************************************/
        /* TYPE_PORT is not supported by the XAPI.                   */
        /*************************************************************/

    case TYPE_REQUEST:

        lastRC = xapi_qrequest(pXapicvt,
                               pXapireqe);

        break;

    case TYPE_SCRATCH:

        lastRC = xapi_qscr(pXapicvt,
                           pXapireqe);

        break;

    case TYPE_SERVER:

        lastRC = xapi_qserver(pXapicvt,
                              pXapireqe);

        break;

    case TYPE_SUBPOOL_NAME:

        lastRC = xapi_qsubpool(pXapicvt,
                               pXapireqe);

        break;

    case TYPE_VOLUME:

        lastRC = xapi_qvol(pXapicvt,
                           pXapireqe);

        break;

    default:

        xapi_query_init_resp(pXapireqe, 
                             wkQerrResponse,
                             QERR_RESPONSE_SIZE);

        xapi_err_response(pXapireqe,
                          wkQerrResponse,
                          QERR_RESPONSE_SIZE,
                          STATUS_UNSUPPORTED_TYPE); 

        lastRC = STATUS_UNSUPPORTED_TYPE;
    } 

    return lastRC;
}



