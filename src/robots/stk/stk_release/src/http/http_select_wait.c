/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_select_wait.c                               */
/** Description:    t_http server select() function.                 */
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
#include <time.h>
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
/** Function Name: http_select_wait                                  */
/** Description:   Setup to send() or recv() on the opened socket.   */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_select_wait"

extern int http_select_wait(struct HTTPCVT *pHttpcvt,
                            int             socketId,
                            char            writeFdsFlag,
                            int             timeoutSeconds)
{
    struct timeval      waitTimeval;

    fd_set              readFds;        
    fd_set              writeFds;       
    fd_set              exceptionFds;   
    fd_set             *pReadFds            = &readFds;
    fd_set             *pWriteFds           = &writeFds;
    fd_set             *pExceptionFds       = &exceptionFds;

    int                 numReady            = 0; 
    int                 lastRC;
    int                 i;

    TRMSGI(TRCI_HTCPIP,
           "Entered; socket=%i, writeFdsFlag=%d, timeoutSeconds=%d\n",
           socketId, 
           writeFdsFlag, 
           timeoutSeconds);

    /*****************************************************************/
    /* Issue the select() to see if we are ready to send() or        */
    /* recv() data.                                                  */
    /*                                                               */
    /* NOTE: numReady will be 0 if the timer popped before the       */
    /* socket is ready to send() or recv() data.                     */
    /*                                                               */
    /* NOTE: We set waitInterval to 1 second, but we interate        */
    /* over the select timeoutSeconds times.  This allows us to      */
    /* check the HTTPCVT to see if we have been cancelled during     */
    /* the wait.                                                     */
    /*****************************************************************/
    for (i = 0;
        i < timeoutSeconds;
        i++)
    {
        /*************************************************************/
        /* First add the socket to the appropriate file descriptor   */
        /* sets.                                                     */
        /*************************************************************/
        if (writeFdsFlag)
        {
            FD_ZERO(pWriteFds);
            FD_SET(socketId, pWriteFds);
            pReadFds = NULL;
        }
        else
        {
            FD_ZERO(pReadFds);
            FD_SET(socketId, pReadFds);
            pWriteFds = NULL;
        }

        FD_ZERO(pExceptionFds);
        FD_SET(socketId, pExceptionFds);

        waitTimeval.tv_sec = 1;
        waitTimeval.tv_usec = 0;

        if (pHttpcvt->status == HTTP_CANCEL)
        {
            TRMSGI(TRCI_HTCPIP,
                   "HTTP cancelled during wait iteration=%d\n",
                   i);

            return STATUS_CANCELLED;
        }

        /*************************************************************/
        /* Now wait for any of the events in the appropriate file    */
        /* descriptor sets.                                          */
        /*************************************************************/
        numReady = select((socketId + 1), 
                          pReadFds, 
                          pWriteFds, 
                          pExceptionFds, 
                          &waitTimeval);

        TRMSGI(TRCI_HTCPIP,"numReady=%d after select iteration=%d (nfds=%d, pReadFds=%08X, "
               "pWriteFds=%08X, pExceptionFds=%08X, waitInterval=%d.%d seconds)\n",
               numReady,
               i,
               (socketId + 1),
               pReadFds,
               pWriteFds,
               pExceptionFds,
               waitTimeval.tv_sec, 
               waitTimeval.tv_usec);

        if (numReady == 0)
        {
            continue;
        }
        else if (numReady == 1)
        {
            return STATUS_SUCCESS;
        }
        else
        {
            return STATUS_NI_FAILURE;
        }
    }

    TRMSGI(TRCI_HTCPIP,
           "numReady=%d after select end-of-loop\n",
           numReady);

    if (numReady == 0)
    {
        return STATUS_NI_TIMEOUT;
    }
    else if (numReady == 1)
    {
        return STATUS_SUCCESS;
    }
    else
    {
        return STATUS_NI_FAILURE;
    }
}


