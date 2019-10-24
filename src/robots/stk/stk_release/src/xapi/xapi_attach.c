/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_attach.c                                    */
/** Description:    XAPI attach service.                             */
/**                                                                  */
/**                 XAPI thread services for attaching XAPICVT       */
/**                 and XAPICFG shared memory segments.              */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     07/15/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/***END PROLOGUE******************************************************/

/*********************************************************************/
/* Includes:                                                         */
/*********************************************************************/
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "api/defs_api.h"
#include "csi.h"
#include "smccodes.h"
#include "srvcommon.h"
#include "xapi.h"


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_attach_xapicvt                               */
/** Description:   Get access to the shared memory XAPICVT.          */
/**                                                                  */
/** A returned NULL address indicates that the XAPICVT shared        */
/** memory segment could not be located or attached.                 */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_attach_xapicvt"

extern struct XAPICVT *xapi_attach_xapicvt(void)
{
    struct XAPICVT     *pXapicvt            = NULL;
    int                 shMemSegId;
    int                 shmPermissions;
    key_t               shmKey;
    size_t              shmSize;

    /*****************************************************************/
    /* Locate the shared memory segment for the XAPICVT.             */
    /*****************************************************************/
    shmKey = XAPI_CVT_SHM_KEY;
    shmSize = sizeof(struct XAPICVT);
    shmPermissions = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    shMemSegId = shmget(shmKey, shmSize, shmPermissions);

    TRMSGI(TRCI_STORAGE,
           "shMemSegId=%d (%08X) after shmget(key=XAPICVT=%d, "
           "size=%d, permissions=%d (%08X))\n",
           shMemSegId,
           shMemSegId,
           shmKey,
           shmSize,
           shmPermissions,
           shmPermissions);

    if (shMemSegId < 0)
    {
        return NULL;
    }

    /*****************************************************************/
    /* Attach the shared memory segment for the XAPICVT to this      */
    /* thread.                                                       */
    /*****************************************************************/
    pXapicvt = (struct XAPICVT*) shmat(shMemSegId, NULL, 0);

    TRMSGI(TRCI_STORAGE,
           "pXapicvt=%08X after shmat(id=XAPICVT=%d (%08X), NULL, 0)\n",
           pXapicvt,
           shMemSegId,
           shMemSegId);

    return pXapicvt;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_attach_xapicfg                               */
/** Description:   Get access to the shared memory XAPICFG.          */
/**                                                                  */
/** A returned NULL address indicates that the XAPICFG shared        */
/** memory segment could not be located or attached.                 */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_attach_xapicfg"

extern struct XAPICFG *xapi_attach_xapicfg(struct XAPICVT *pXapicvt)
{
    struct XAPICFG     *pXapicfg            = NULL;
    int                 shMemSegId;
    int                 shmPermissions;
    key_t               shmKey;
    size_t              shmSize;

    /*****************************************************************/
    /* Locate the shared memory segment for the XAPICFG.             */
    /*****************************************************************/
    shmKey = XAPI_CFG_SHM_KEY;
    shmSize = pXapicvt->xapicfgSize;
    shmPermissions = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    shMemSegId = shmget(shmKey, shmSize, shmPermissions);

    TRMSGI(TRCI_STORAGE,
           "shMemSegId=%d (%08X) after shmget(key=XAPICFG=%d, "
           "size=%d, permission=%d (%08X))\n",
           shMemSegId,
           shMemSegId,
           shmKey,
           shmSize,
           shmPermissions,
           shmPermissions);

    if (shMemSegId < 0)
    {
        return NULL;
    }

    /*****************************************************************/
    /* Attach the shared memory segment for the XAPICFG to this      */
    /* thread.                                                       */
    /*****************************************************************/
    pXapicfg = (struct XAPICFG*) shmat(shMemSegId, NULL, 0);

    TRMSGI(TRCI_STORAGE,
           "pXapicfg=%08X after shmat(id=XAPICFG=%d (%08X), NULL, 0)\n",
           pXapicfg,
           shMemSegId,
           shMemSegId);

    return pXapicfg;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_attach_work_xapireqe                         */
/** Description:   Initialize a work XAPIREQE.                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_attach_work_xapireqe"

extern void xapi_attach_work_xapireqe(struct XAPICVT  *pXapicvt,
                                      struct XAPIREQE *pWorkXapireqe,
                                      char            *pAcsapiBuffer,
                                      int              acsapiBufferSize)
{
    IPC_HEADER         *pIpc_Header;
    REQUEST_HEADER     *pRequest_Header;
    MESSAGE_HEADER     *pMessage_Header;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    memset((char*) pWorkXapireqe, 0, sizeof(struct XAPIREQE));
    pWorkXapireqe->requestFlag = XAPIREQE_START;
    time(&(pWorkXapireqe->startTime));
    pWorkXapireqe->pAcsapiBuffer = pAcsapiBuffer;
    pWorkXapireqe->acsapiBufferSize = acsapiBufferSize;

    if (pAcsapiBuffer != NULL)
    {
        pIpc_Header = (IPC_HEADER*) pAcsapiBuffer;

        memcpy(pWorkXapireqe->return_socket, 
               pIpc_Header->return_socket,
               sizeof(pIpc_Header->return_socket));

        pRequest_Header = (REQUEST_HEADER*) pAcsapiBuffer;
        pMessage_Header = &(pRequest_Header->message_header);
        pWorkXapireqe->seqNumber = pMessage_Header->packet_id;
    }
    else if (pXapicvt != NULL)
    {
        pXapicvt->requestCount++;
        pWorkXapireqe->seqNumber = pXapicvt->requestCount;
    }

    return;
}



