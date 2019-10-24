/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_term.c                                      */
/** Description:    t_http server executable termination.            */
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
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>


#ifndef IPC_SHARED

    #include <netinet/in.h>

    #ifdef SOLARIS
        #include <sys/file.h>
    #endif

    #ifdef AIX
        #include <fcntl.h>
    #else
        #include <sys/fcntl.h>
    #endif

    #include <netdb.h>

#endif /* IPC_SHARED */


#include "api/defs_api.h"
#include "csi.h"
#include "srvcommon.h"
#include "xapi/http.h"
#include "xapi/smccxmlx.h"
#include "xapi/xapi.h"


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: http_term                                         */
/** Description:   t_http server executable termination.             */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_term"

void http_term(struct HTTPCVT *pHttpcvt)
{
    int                 lastRC;
    int                 shMemSegId;

    LOGMSG(STATUS_SUCCESS, 
           "HTTP Termination Started");

    /*****************************************************************/
    /* Clean up the HTTP shared memory and semaphore resources.      */
    /*****************************************************************/
    if (pHttpcvt != NULL)
    {
        /*************************************************************/
        /* Destroy the semaphore created to serialize access to the  */
        /* XAPICFG table.                                            */
        /*************************************************************/
        lastRC = sem_destroy(&pHttpcvt->httpCdsLock);

        TRMSGI(TRCI_STORAGE,
               "lastRC=%d from sem_destroy(addr=%08X)\n",
               lastRC,
               &pHttpcvt->httpCdsLock);

        /*************************************************************/
        /* Detach and remove the XAPICVT shared memory segment.      */
        /*************************************************************/
        shMemSegId = pHttpcvt->cvtShMemSegId;

        lastRC = shmdt((void*) pHttpcvt);

        TRMSGI(TRCI_STORAGE,
               "lastRC=%d from shmdt(addr=HTTPCVT=%08X)\n",
               lastRC,
               pHttpcvt);

        lastRC = shmctl(shMemSegId, IPC_RMID, NULL);

        TRMSGI(TRCI_STORAGE,
               "lastRC=%d from shctl(id=HTTPCVT=%d (%08X))\n",
               lastRC,
               shMemSegId,
               shMemSegId);
    }

    LOGMSG(STATUS_SUCCESS, 
           "HTTP Termination Completed");

    return;
}

