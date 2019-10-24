/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_listen.c                                    */
/** Description:    t_http server executable socker listener.        */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     08/15/11                          */
/**     Created for CDK to add XAPI support.                         */
/** I6087283       Joseph Nofi     01/08/13                          */
/**     Add waitpid(-1) after each fork() to cleanup <defunct>       */
/**     zombie processes before termination.                         */
/**     Fix "listening on port" number display on X86 systems.       */
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
#include <sys/wait.h>
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
/** Function Name: http_listen                                       */
/** Description:   t_http server executable socker listener.         */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_listen"

int http_listen(struct HTTPCVT *pHttpcvt)
{
    int                 acceptRC;
    int                 lastRC;
    int                 newSocketId;
    int                 i;
    int                 toggleOn            = SETSOCKOPT_ON;
    int                 listenerRetryCount;
    pid_t               pid;
    pid_t               termPid;
    int                 lenNewSockaddr      = sizeof(struct sockaddr);

    struct sockaddr_in  newSockaddr_in;
    struct sockaddr_in *pNewSockaddr_in     = &newSockaddr_in;
    struct sockaddr_in *pLisSockaddr_in     = &(pHttpcvt->sockaddr_in);

    /*****************************************************************/
    /* Loop here for HTTP_INIT_RETRY_COUNT.                          */
    /* If we get an error during the accept() loop, we'll retry      */
    /* all of the precursor socket(), bind(), listen() steps to      */
    /* try to re-establish the TCP/IP listener environment.          */
    /*****************************************************************/
    for (listenerRetryCount = 1;
        listenerRetryCount <= HTTP_INIT_RETRY_COUNT;
        listenerRetryCount++)
    {
        /*************************************************************/
        /* Re-initialize the HTTPCVT for each iteration.             */
        /*************************************************************/
        acceptRC = STATUS_SUCCESS;
        pHttpcvt->socketId = RC_FAILURE;
        pHttpcvt->ipAddress = HTTP_LOOPBACK_ADDRESS;
        strcpy(pHttpcvt->ipAddressString, "UNKNOWN");

        /*************************************************************/
        /* Call socket() to get a stream socket for the HTTP server  */
        /* end of the connection.                                    */
        /*                                                           */
        /* NOTE: Since this is merely a test server, we will         */
        /* simply default to IPv4.                                   */
        /*************************************************************/
        pHttpcvt->socketId = socket(AF_INET, 
                                    SOCK_STREAM, 
                                    IPPROTO_TCP);

        TRMSGI(TRCI_HTCPIP,
               "pHttpcvt->socketId=%d after socket(domain=AF_INET=%d, "
               "type=SOCK_STREAM=%d, protocol=IPPROTO_TCP=%d)\n",
               pHttpcvt->socketId,
               AF_INET,
               SOCK_STREAM,
               IPPROTO_TCP);

        if (pHttpcvt->socketId == RC_FAILURE)
        {
            LOGMSG(STATUS_NI_FAILURE, 
                   "HTTP socket error\n");

            acceptRC = STATUS_RETRY;
        }

        /*****************************************************************/
        /* Set the SO_REUSEADDR socket option to allow retries using     */
        /* same socket id.                                               */
        /*                                                               */
        /* NOTE: We ignore any setsockopt errors and continue.           */
        /*****************************************************************/
        if (acceptRC == STATUS_SUCCESS)
        {
            lastRC = setsockopt(pHttpcvt->socketId, 
                                SOL_SOCKET,
                                SO_REUSEADDR, 
                                &toggleOn, 
                                sizeof(toggleOn));

            TRMSGI(TRCI_HTCPIP,
                   "lastRC=%d after setsockopt(socket=%d, SO_REUSEADDR, on)\n",
                   lastRC,
                   pHttpcvt->socketId);
        }

        /*************************************************************/
        /* Now bind the client socket to the specified HTTP Server   */
        /* port.                                                     */
        /*************************************************************/
        if (acceptRC == STATUS_SUCCESS)
        {
            TRMSGI(TRCI_HTCPIP,
                   "Using httpPort port=%d\n",
                   pHttpcvt->xapiPort);

            memset((char*) pLisSockaddr_in,
                   0,
                   sizeof(struct sockaddr_in));

            pLisSockaddr_in->sin_family = AF_INET;
            pLisSockaddr_in->sin_addr.s_addr = htonl(INADDR_ANY);
            pLisSockaddr_in->sin_port = htons(pHttpcvt->xapiPort);

            lastRC = bind(pHttpcvt->socketId, 
                          (struct sockaddr*) pLisSockaddr_in,
                          sizeof(struct sockaddr));

            TRMSGI(TRCI_HTCPIP,
                   "lastRC=%d from bind(socketId=%d, port=%d)\n", 
                   lastRC, 
                   pHttpcvt->socketId,
                   pHttpcvt->xapiPort);

            if (lastRC != STATUS_SUCCESS)
            {
                LOGMSG(STATUS_NI_FAILURE, 
                       "HTTP bind error to port=%d, RC=%d",
                       pHttpcvt->xapiPort,
                       errno);

                acceptRC = STATUS_RETRY;
            }
            else
            {
                printf("XAPI t_http server listening on port %d\n",
                       ntohs(pLisSockaddr_in->sin_port));
            }
        }

        /*************************************************************/
        /* Now do the listen().  This completes the binding of our   */
        /* port to our socket, and creates a connection request      */
        /* queue of length HTTP_LISTEN_QUEUE_SIZE to queue incoming  */
        /* connection requests.                                      */
        /*************************************************************/
        if (acceptRC == STATUS_SUCCESS)
        {
            lastRC = listen(pHttpcvt->socketId, 
                            HTTP_LISTEN_QUEUE_SIZE);

            TRMSGI(TRCI_HTCPIP,
                   "lastRC=%d from listen(socketId=%d, size=%d)\n", 
                   lastRC, 
                   pHttpcvt->socketId,
                   HTTP_LISTEN_QUEUE_SIZE);

            if (lastRC != STATUS_SUCCESS)
            {
                LOGMSG(STATUS_NI_FAILURE, 
                       "HTTP listen error on port=%d, socket=%d, RC=%d",
                       pHttpcvt->xapiPort,
                       pHttpcvt->socketId,
                       errno);

                acceptRC = STATUS_RETRY;
            }
        }

        if (acceptRC == STATUS_SUCCESS)
        {
            LOGMSG(STATUS_SUCCESS, 
                   "HTTP Initiation Completed");

            while (1)
            {
                newSocketId = accept(pHttpcvt->socketId,
                                     (struct sockaddr*) pNewSockaddr_in,
                                     &lenNewSockaddr);   

                TRMSGI(TRCI_HTCPIP,
                       "acceptSocket=%d from accept(socketId=%d, size=%d)\n", 
                       newSocketId, 
                       pHttpcvt->socketId,
                       HTTP_LISTEN_QUEUE_SIZE);

                if (newSocketId == RC_FAILURE)
                {
                    LOGMSG(STATUS_NI_FAILURE, 
                           "HTTP accept error on port=%d, socket=%d, RC=%d",
                           pHttpcvt->xapiPort,
                           pHttpcvt->socketId,
                           errno);

                    acceptRC = STATUS_NI_FAILURE;

                    break;
                }
                else
                {
                    /*************************************************/
                    /* If we have a successful accept(), then create */
                    /* child process to process the accept()ed       */
                    /* request.                                      */
                    /*************************************************/
                    pid = fork();
    
                    /*************************************************/
                    /* When fork() returns 0, we are the child       */
                    /* process:                                      */
                    /* Close the listener() socket, and process      */
                    /* the accept()ed socket.                        */
                    /*************************************************/
                    if (pid == 0)
                    {
                        TRMSGI(TRCI_SERVER,
                               ">>>> XAPI http_cgi() child process <<<<\n");
    
                        close(pHttpcvt->socketId);
    
                        lastRC = http_work(pHttpcvt,
                                           newSocketId);
    
                        TRMSGI(TRCI_SERVER,
                               "RC=%d from http_work(newSocketId=%d)\n",
                              lastRC,
                              newSocketId);
    
                        _exit(0);
                    }
                    /*****************************************************/
                    /* When fork() returns a positive number, we are     */
                    /* the parent process:                               */
                    /* Close the accept()ed socket and wait for any      */
                    /* child processes to terminate before continuing    */
                    /* the accept() loop.                                */
                    /*                                                   */
                    /* The wait prevents zombies and <defunct> processes.*/
                    /* However, the "last" accepted socket process may   */
                    /* be a zombie until the server is shutdown.         */
                    /*****************************************************/
                    else if (pid > 0)
                    {
                        close(newSocketId);

                        while (1)
                        {
                            termPid = waitpid(-1, NULL, WNOHANG);

                            if (termPid <= 0)
                            {
                                break;
                            }

                            TRMSGI(TRCI_SERVER,
                                   ">>>> XAPI http_cgi() child process %i ended <<<<",
                                   termPid);
                        }
                    }
                    /*****************************************************/
                    /* When fork() returns a negative number, the fork() */
                    /* failed:                                           */
                    /* Exit the accept() loop with a fatal error.        */
                    /*****************************************************/
                    else
                    {
                        LOGMSG(STATUS_NI_FAILURE, 
                               "HTTP fork() failure on port=%d, socket=%d, newSosketRC=%d, RC=%d\n",
                               pHttpcvt->xapiPort,
                               pHttpcvt->socketId,
                               newSocketId,
                               pid);
    
                        acceptRC = STATUS_PROCESS_FAILURE;

                        break;
                    }
                } /* accept() success   */
            } /* while(1) accept() loop */
        } /* acceptRC == STATUS_SUCCESS */

        /*************************************************************/
        /* If we reach the end of the listenerRetryCount loop        */
        /* with a fatal error, then exit the loop.  Otherwise,       */
        /* sleep for a second and retry.                             */
        /*************************************************************/
        if (acceptRC != STATUS_SUCCESS)
        {
            if (acceptRC != STATUS_RETRY)
            {
                sleep(1);
            }
            else
            {
                break;
            }
        }
    } /* For listenerRetryCount loop */

    return acceptRC;
}





