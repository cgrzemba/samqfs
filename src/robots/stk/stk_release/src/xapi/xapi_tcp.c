/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_tcp.c                                       */
/** Description:    XAPI client TCP/IP communications service.       */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     05/11/11                          */
/**     Created for CDK to add XAPI support.                         */
/** I6087333       Joseph Nofi     01/08/13                          */
/**     Fix connect() errno=146 (connection reset) on X86 systems    */
/**     due to little-endian/big-endian issues.                      */
/**                                                                  */
/***END PROLOGUE******************************************************/

/*********************************************************************/
/* Includes:                                                         */
/*********************************************************************/

#include <errno.h>                      
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>                      
#include <string.h>                     
#include <sys/socket.h>
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
#include "smccxmlx.h"
#include "srvcommon.h"
#include "xapi.h"


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#define RECV_INCREMENT             16384


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int selectWait(struct XAPIREQE *pXapireqe,
                      int              clientSocket,
                      char             writeFdsFlag,
                      int              timeoutSeconds);

static int getHostId6(struct XAPICVT          *pXapicvt,
                      struct XAPIREQE         *pXapireqe,
                      char                    *pHostName,
                      unsigned int            *pAddressFamily,
                      socklen_t               *pSockaddrLen,
                      struct sockaddr_storage *pSockaddr);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_tcp                                          */
/** Description:   The XAPI client TCP/IP communications service.    */
/**                                                                  */
/** Input is the XAPI XML transaction string.                        */
/** Output is an XMLPARSE hierarchy tree of the parsed XML response. */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_tcp"

extern int xapi_tcp(struct XAPICVT  *pXapicvt,
                    struct XAPIREQE *pXapireqe,
                    char            *pXapiBuffer,
                    int              xapiBufferSize,
                    int              sendTimeout,
                    int              recvTimeout1st,
                    int              recvTimeoutNon1st,
                    void           **ptrResponseXmlparse)
{

    int                 lastRC;
    int                 clientSocket;
    int                 currBufferSize      = 0;
    int                 oldBufferSize;
    int                 bytesRecv;
    int                 totalRecv           = 0;
    int                 lastRecv            = -1;
    int                 connectRetryCount   = 0;
    int                 recvRetryCount      = 0;
    int                 recvRC              = STATUS_SUCCESS;
    int                 recvErrorFlag       = FALSE;
    unsigned int        ipAddress[4]        = {0,0,0,0};
    unsigned int        addressFamily       = AF_UNSPEC; 

    char               *pCurrBuffer         = NULL;
    char               *pOldBuffer;
    char               *pLibreplyStart;
    char               *pLibreplyEnd;
    struct XMLPARSE    *pXmlparse;
    struct XMLCNVIN    *pXmlcnvin;

    /*****************************************************************/
    /* sockaddr_storage for either IPv4 or IPv6 sockaddr_in format.  */
    /*****************************************************************/
    struct sockaddr_storage  wkSockaddr;
    struct sockaddr_storage *pSockaddr      = &wkSockaddr;
    struct sockaddr_in *pSockaddr_in        = (struct sockaddr_in*) &wkSockaddr;

    socklen_t           sockaddrLen;

    char                portString[6];
    char                pWorkBuffer[RECV_INCREMENT + 1];

    *ptrResponseXmlparse = NULL;

    if (pXapireqe->requestFlag & XAPIREQE_CANCEL)
    {
        TRMSGI(TRCI_XTCPIP,
               "Request cancelled before entry to xapi_tcp\n");

        return STATUS_CANCELLED;
    }

    TRMSGI(TRCI_XAPI,
           "Entered; pXapiBuffer=%08X, xapiBufferSize=%d, "
           "sendTimeout=%d, recvTimeout1st=%d, recvTimeoutNon1st=%d)\n",
           pXapiBuffer,
           xapiBufferSize,
           sendTimeout,
           recvTimeout1st,
           recvTimeoutNon1st);

    LOG_XAPI_SEND(STATUS_SUCCESS, 
                  pXapireqe->seqNumber, 
                  pXapicvt->xapiHostname,
                  pXapicvt->xapiPort,
                  pXapiBuffer, 
                  xapiBufferSize,
                  "XML send()\n");

    /*****************************************************************/
    /* Get IP address for $XAPI_HOSTNAME.                            */
    /*****************************************************************/
    sprintf(portString, 
            "%d",
            pXapicvt->xapiPort);

    lastRC = getHostId6(pXapicvt,
                        pXapireqe,
                        pXapicvt->xapiHostname,
                        &addressFamily,
                        &sockaddrLen,
                        pSockaddr);

    TRMSGI(TRCI_XTCPIP,  
           "getHostId6() RC=%i, addressFamily=%d, "
           "sockaddrLen=%d, pSockaddr=%08X\n",
           lastRC,
           addressFamily,
           sockaddrLen,
           pSockaddr);

    if (lastRC != STATUS_SUCCESS)
    {
        LOGMSG(lastRC, 
               "TCP/IP getHostId6() error=%d, command=%s (%d), seq=%d\n",
               lastRC,
               acs_command((COMMAND) pXapireqe->command),
               pXapireqe->command,
               pXapireqe->seqNumber);

        return STATUS_NI_FAILURE;
    }

    /*****************************************************************/
    /* If $CSI_API_HOSTNAME defined, then create a socket of the     */
    /* returned address family type (AF_INET or AF_INET6).           */
    /*****************************************************************/
    clientSocket = socket(addressFamily, 
                          SOCK_STREAM, 
                          IPPROTO_TCP);

    TRMSGI(TRCI_XTCPIP,
           "clientSocket=%d after socket(family=%d, "
           "type=SOCK_STREAM=%d, protocol=IPPROTO_TCP=%d)\n",
           clientSocket,
           addressFamily,
           SOCK_STREAM,
           IPPROTO_TCP);

    if (clientSocket < 0)
    {
        LOGMSG(STATUS_NI_FAILURE, 
               "TCP/IP socket error, command=%s (%d), seq=%d\n",
               acs_command((COMMAND) pXapireqe->command),
               pXapireqe->command,
               pXapireqe->seqNumber);

        return STATUS_NI_FAILURE;
    }

    /*****************************************************************/
    /* Update the IPv4 or IPv6 sockaddr with the XAPI_PORT number.   */
    /*****************************************************************/
    pSockaddr_in->sin_port = htons(pXapicvt->xapiPort);

    TRMEMI(TRCI_XTCPIP,
           (char*) pSockaddr, sockaddrLen, 
           "Updated sockaddr_in:\n");

    /*****************************************************************/
    /* Make the socket a non-blocking socket so we have more control */
    /* over TCP/IP timeout values using the select wait mechanism.   */
    /*                                                               */
    /* NOTE: We ignore any fcntl() errors and continue.              */
    /*****************************************************************/    
    lastRC = fcntl(clientSocket, 
                   F_SETFL, 
                   O_NONBLOCK);

    TRMSGI(TRCI_XTCPIP,
           "lastRC=%d after fcntl(socket=%d, cmd=F_SETFL=%d, "
           "flags=O_NONBLOCK=%08X)\n",
           lastRC,
           clientSocket,
           F_SETFL,
           O_NONBLOCK);

    /*****************************************************************/
    /* Test if the request has been cancelled.                       */
    /*****************************************************************/    
    if (pXapireqe->requestFlag & XAPIREQE_CANCEL)
    {
        LOGMSG(STATUS_CANCELLED, 
               "TCP/IP request cancelled, command=%s (%d), seq=%d\n",
               acs_command((COMMAND) pXapireqe->command),
               pXapireqe->command,
               pXapireqe->seqNumber);

        shutdown(clientSocket, 
                 SHUT_RDWR);

        close(clientSocket);

        return STATUS_CANCELLED;
    }

    /*****************************************************************/
    /* Perform the connect.                                          */
    /*****************************************************************/
    while (1)
    {
        lastRC = connect(clientSocket, 
                         (struct sockaddr*) pSockaddr, 
                         sockaddrLen);

        if (lastRC == RC_FAILURE)
        {
            lastRC = (int) errno;
        }

        TRMSGI(TRCI_XTCPIP,
               "connect RC=%d (socket=%d, a(sockaddr_in)=%08X)\n",
               lastRC,
               clientSocket,
               pSockaddr);

        /*************************************************************/
        /* If EAGAIN error, and there has been < 10 EAGAIN errors,   */
        /*    then retry the same connect() after a short wait.      */
        /* If any other connect() return code, then break.           */
        /*************************************************************/
        if (lastRC == EAGAIN)
        {
            connectRetryCount++;

            TRMSGI(TRCI_XTCPIP,
                   "EAGAIN #%i detected\n",
                   connectRetryCount);

            if (connectRetryCount > 10)
            {
                break;
            }

            /*****************************************************/
            /* Wait 1 a second after the EAGAIN error.           */
            /*****************************************************/
            sleep(1);

            continue;
        }
        else
        {
            break;
        }
    }

    /*****************************************************************/
    /* NOTE: For a non-blocking socket, the RC from connect()        */
    /* may be a -1 with an errno of EINPROGRESS.  We accept this     */
    /* error and let the subsequent select() determine if we         */
    /* should continue with the send().                              */
    /*****************************************************************/    
    if ((lastRC != STATUS_SUCCESS) &&
        (lastRC != EINPROGRESS))
    {
        LOGMSG(STATUS_NI_FAILURE, 
               "TCP/IP connect error=%d, command=%s (%d), seq=%d\n",
               lastRC,
               acs_command((COMMAND) pXapireqe->command),
               pXapireqe->command,
               pXapireqe->seqNumber);

        shutdown(clientSocket, 
                 SHUT_RDWR);

        close(clientSocket);

        return STATUS_NI_FAILURE;
    }

    /*****************************************************************/
    /* Perform a select() before the send().                         */
    /*                                                               */
    /* NOTE: A bad return code here means we either timed out,       */
    /* there was a socket error, or the request was cancelled.       */
    /*****************************************************************/
    lastRC = selectWait(pXapireqe,
                        clientSocket,
                        TRUE,
                        sendTimeout);

    TRMSGI(TRCI_XTCPIP,
           "lastRC=%d after selectWait(socket=%d, writeFdsFlag=%d, timeout=%d)\n",
           lastRC,
           clientSocket,
           TRUE,
           sendTimeout);

    if (lastRC != STATUS_SUCCESS)
    {
        LOGMSG(lastRC, 
               "TCP/IP selectWait send error=%s (%d), command=%s (%d), seq=%d\n",
               acs_status((STATUS) lastRC),
               lastRC,
               acs_command((COMMAND) pXapireqe->command),
               pXapireqe->command,
               pXapireqe->seqNumber);

        shutdown(clientSocket, 
                 SHUT_RDWR);

        close(clientSocket);

        return lastRC;
    }

    /*****************************************************************/
    /* Now send the XAPI transaction buffer.                         */
    /*                                                               */
    /* NOTE: Normally we would preface the XAPI transaction with     */
    /* the HTTP MIME header.  However the SMC HTTP server will       */
    /* accept a "headerless" XAPI transaction and implictly          */
    /* perform the POST of the correct HTTP server CGI program.      */
    /*****************************************************************/
    lastRC = send(clientSocket, 
                  pXapiBuffer, 
                  xapiBufferSize, 
                  0);

    TRMSGI(TRCI_XTCPIP,
           "lastRC=%d after send(socket=%d, buffer=%08X, "
           "size=%d, flags=0)\n",
           lastRC,
           clientSocket,
           pXapiBuffer,
           xapiBufferSize);

    if (lastRC == RC_FAILURE)
    {
        lastRC = (int) errno;

        LOGMSG(STATUS_NI_FAILURE, 
               "TCP/IP send error=%d, command=%s (%d), seq=%d\n",
               lastRC,
               acs_command((COMMAND) pXapireqe->command),
               pXapireqe->command,
               pXapireqe->seqNumber);

        shutdown(clientSocket, 
                 SHUT_RDWR);

        close(clientSocket);

        return STATUS_NI_FAILURE;
    }

    /*****************************************************************/
    /* Acquire storage for the initial response buffer.  If this     */
    /* buffer is too small, we will acquire a larger buffer when     */
    /* needed.                                                       */
    /*****************************************************************/
    currBufferSize = RECV_INCREMENT;

    pCurrBuffer = (char*) malloc(currBufferSize);

    memset(pCurrBuffer, 0, currBufferSize);

    TRMSGI(TRCI_STORAGE,
           "malloc response buffer=%08X, len=%i\n",
           pCurrBuffer,
           currBufferSize);

    /*****************************************************************/
    /* Perform a select() before the 1st recv().                     */
    /*                                                               */
    /* NOTE: A bad return code here means we either timed out,       */
    /* there was a socket error, or the request was cancelled.       */
    /*****************************************************************/
    lastRC = selectWait(pXapireqe,
                        clientSocket,
                        FALSE,
                        recvTimeout1st);

    TRMSGI(TRCI_XTCPIP,
           "lastRC=%d after selectWait(socket=%d, writeFdsFlag=%d, timeout=%d)\n",
           lastRC,
           clientSocket,
           FALSE,
           recvTimeout1st);

    if (lastRC != STATUS_SUCCESS)
    {
        LOGMSG(lastRC, 
               "TCP/IP selectWait 1st error=%s (%d), command=%s (%d), seq=%d\n",
               acs_status((STATUS) lastRC),
               lastRC,
               acs_command((COMMAND) pXapireqe->command),
               pXapireqe->command,
               pXapireqe->seqNumber);

        shutdown(clientSocket, 
                 SHUT_RDWR);

        close(clientSocket);

        return lastRC;
    }

    /*****************************************************************/
    /* Recv() the entire response.                                   */
    /*****************************************************************/
    while (1)
    {
        bytesRecv = recv(clientSocket, 
                         pWorkBuffer, 
                         RECV_INCREMENT, 
                         0);

        TRMSGI(TRCI_XTCPIP,
               "bytesRecv=%d after recv(socket=%d, buffer=%08X, "
               "size=%d, flags=0\n", 
               bytesRecv,
               clientSocket,
               pWorkBuffer,
               RECV_INCREMENT);

        /*************************************************************/
        /* If recv() RC < 0 then it is some type of error.           */
        /* If EINTR error, and there has been < 10 EINTR errors,     */
        /*    then retry the same recv() after a short wait.         */
        /* If any other recv() error and we've previously had        */
        /*    another recv() error on this socket, then return.      */
        /*************************************************************/
        if (bytesRecv < 0)
        {
            lastRC = (int) errno;

            if (lastRC == EINTR)
            {
                recvRetryCount++;

                TRMSGI(TRCI_XTCPIP,
                       "EINTR #%i detected\n",
                       recvRetryCount);

                if (recvRetryCount > 10)
                {
                    recvRC = STATUS_NI_FAILURE;

                    break;
                }

                /*****************************************************/
                /* Wait 1 a second after the EINTR.                  */
                /*****************************************************/
                sleep(1);

                lastRecv = -1;

                continue;
            }
            else
            {
                TRMSGI(TRCI_XTCPIP,
                       "recv errno=%d on socket=%d\n",
                       lastRC,
                       clientSocket);

                recvErrorFlag = TRUE;
                lastRecv = 0;

                continue;
            }
        }
        /*************************************************************/
        /* If recv() RC = 0, then it should be end-of-data.          */
        /* To handle possible multiple blocks, look for 2            */
        /* consecutive recv() of 0 bytes.                            */
        /*************************************************************/
        else if (bytesRecv == 0)
        {
            if (lastRecv == 0)
            {
                TRMSGI(TRCI_XTCPIP,
                       "2 consecutive end-of-data\n");

                break;
            }
            else
            {
                lastRecv = 0;

                TRMSGI(TRCI_XTCPIP,
                       "1 end-of-data\n");

                continue;
            }
        }

        /*************************************************************/
        /* Else append the recv() data to our response buffer.       */
        /*************************************************************/
        recvErrorFlag = FALSE;
        recvRC = STATUS_SUCCESS;
        lastRecv = bytesRecv;

        /*************************************************************/
        /* If by adding this response to our buffer we would exceed  */
        /* the maximum buffer size, then acquire another buffer      */
        /* that is MAXRECVSIZE bigger than the current buffer, move  */
        /* all current data into it, and free the old buffer.        */
        /*************************************************************/
        if ((totalRecv + bytesRecv) > currBufferSize)
        {
            pOldBuffer = pCurrBuffer;
            oldBufferSize = currBufferSize;
            currBufferSize = currBufferSize + RECV_INCREMENT;

            TRMSGI(TRCI_XTCPIP,
                   "Response buffer size exceeded, expanding to %i\n",
                   currBufferSize);

            pCurrBuffer = (char*) malloc(currBufferSize);

            memset(pCurrBuffer, 0, currBufferSize);

            TRMSGI(TRCI_STORAGE,
                   "malloc response buffer=%08X, len=%i\n",
                   pCurrBuffer,
                   currBufferSize);

            memcpy(pCurrBuffer,
                   pOldBuffer,
                   totalRecv);

            TRMSGI(TRCI_STORAGE,
                   "free old response buffer=%08X, len=%i\n",
                   pOldBuffer,
                   oldBufferSize);

            free(pOldBuffer);
        }

        memcpy(&(pCurrBuffer[totalRecv]),
               pWorkBuffer,
               bytesRecv);

        totalRecv += bytesRecv;

        /*************************************************************/
        /* Perform a select() before each subsequent recv().         */
        /*                                                           */
        /* NOTE: A bad return code here means we either timed out,   */
        /* there was a socket error, or the request was cancelled.   */
        /*************************************************************/
        lastRC = selectWait(pXapireqe,
                            clientSocket,
                            FALSE,
                            recvTimeoutNon1st);

        TRMSGI(TRCI_XTCPIP,
               "lastRC=%d after selectWait(socket=%d, writeFdsFlag=%d, timeout=%d)\n",
               lastRC,
               clientSocket,
               FALSE,
               recvTimeoutNon1st);

        if (lastRC != STATUS_SUCCESS)
        {
            LOGMSG(lastRC, 
                   "TCP/IP selectWait next error=%s (%d), command=%s (%d), seq=%d\n",
                   acs_status((STATUS) lastRC),
                   lastRC,
                   acs_command((COMMAND) pXapireqe->command),
                   pXapireqe->command,
                   pXapireqe->seqNumber);

            recvRC = lastRC;

            break;
        }
    }

    /*****************************************************************/
    /* When recv() complete, shutdown the socket.                    */
    /*****************************************************************/
    TRMSGI(TRCI_XTCPIP,
           "recvRC=%d. totalRecv=%d after recv loop\n",
           recvRC,
           totalRecv);

    shutdown(clientSocket, 
             SHUT_RDWR);

    close(clientSocket);

    if (totalRecv > 0)
    {
        LOG_XAPI_RECV(recvRC, 
                      pXapireqe->seqNumber, 
                      pXapicvt->xapiHostname,
                      pXapicvt->xapiPort,
                      pCurrBuffer, 
                      (totalRecv + 1),
                      "XML recv()\n");
    }
    else
    {
        LOGMSG(STATUS_NI_FAILURE, 
               "TCP/IP recv error, len=0, command=%s (%d), seq=%d\n",
               acs_command((COMMAND) pXapireqe->command),
               pXapireqe->command,
               pXapireqe->seqNumber);

        recvRC = STATUS_NI_FAILURE;
    }

    /*****************************************************************/
    /* Check for request cancelled after recv complete.              */
    /*****************************************************************/
    if (pXapireqe->requestFlag & XAPIREQE_CANCEL)
    {
        LOGMSG(STATUS_CANCELLED, 
               "TCP/IP request cancelled, command=%s (%d), seq=%d\n",
               acs_command((COMMAND) pXapireqe->command),
               pXapireqe->command,
               pXapireqe->seqNumber);

        recvRC = STATUS_CANCELLED;
    }

    /*****************************************************************/
    /* If recv success, test for a <libreply> tag in the response.   */
    /* If no <libreply> tag in response, then set a bad recvRC.      */
    /*****************************************************************/
    if (recvRC == STATUS_SUCCESS)
    {
        pLibreplyStart = strstr(pCurrBuffer, XSTARTTAG_libreply);
        pLibreplyEnd = strstr(pCurrBuffer, XENDTAG_libreply);

        if ((pLibreplyStart == NULL) ||
            (pLibreplyEnd == NULL))
        {
            LOGMSG(STATUS_NI_FAILURE, 
                   "TCP/IP <libreply> not found error, command=%s (%d), seq=%d\n",
                   acs_command((COMMAND) pXapireqe->command),
                   pXapireqe->command,
                   pXapireqe->seqNumber);

            recvRC = STATUS_NI_FAILURE;
        }
        /*************************************************************/
        /* Otherwise, parse the XAPI response into an XML parse tree.*/
        /*************************************************************/
        else
        {
            pXmlparse = FN_PARSE_XML(pCurrBuffer,
                                     totalRecv);

            /*********************************************************/
            /* If we could not parse the XAPI XML response, then     */
            /* set a a bad recvRC.                                   */
            /*********************************************************/
            if (pXmlparse == NULL)
            {
                LOGMSG(STATUS_NI_FAILURE, 
                       "TCP/IP XML parse error, command=%s (%d), seq=%d\n",
                       acs_command((COMMAND) pXapireqe->command),
                       pXapireqe->command,
                       pXapireqe->seqNumber);

                recvRC = STATUS_NI_FAILURE;
            }
            else if (pXmlparse->errorCode != RC_SUCCESS)
            {
                LOGMSG(STATUS_NI_FAILURE, 
                       "TCP/IP XML parse error RC=%d, reason=%d, command=%s (%d), seq=%d\n",
                       pXmlparse->errorCode,
                       pXmlparse->reasonCode,
                       acs_command((COMMAND) pXapireqe->command),
                       pXapireqe->command,
                       pXapireqe->seqNumber);

                recvRC = STATUS_NI_FAILURE;

                FN_FREE_HIERARCHY_STORAGE(pXmlparse);

                pXmlparse = NULL;
            }
        }
    }

    /*****************************************************************/
    /* Free the current dynamic response buffer;                     */
    /* Set the pointer to the response XMLPARSE; And return.         */
    /*****************************************************************/
    TRMSGI(TRCI_STORAGE,
           "free last pCurrBuffer=%08X, len=%i\n",
           pCurrBuffer,
           currBufferSize);

    free(pCurrBuffer);

    *ptrResponseXmlparse = (void*) pXmlparse;

    return recvRC;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: selectWait                                        */
/** Description:   Setup to send() or recv() on an opened socket.    */
/**                                                                  */
/** NOTE: We periodically check for the setting of the               */
/** XAPIREQE.requestFlag to test if this request has been            */
/** asynchronously cancelled.                                        */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "selectWait"

static int selectWait(struct XAPIREQE *pXapireqe,
                      int              clientSocket,
                      char             writeFdsFlag,
                      int              timeoutSeconds)
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

    TRMSGI(TRCI_XTCPIP,
           "Entered; socket=%i, writeFdsFlag=%d, timeoutSeconds=%d\n",
           clientSocket, 
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
    /* check the XAPIREQE to see if we have been cancelled during    */
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
            FD_SET(clientSocket, pWriteFds);
            pReadFds = NULL;
        }
        else
        {
            FD_ZERO(pReadFds);
            FD_SET(clientSocket, pReadFds);
            pWriteFds = NULL;
        }

        FD_ZERO(pExceptionFds);
        FD_SET(clientSocket, pExceptionFds);

        waitTimeval.tv_sec = 1;
        waitTimeval.tv_usec = 0;

        if (pXapireqe->requestFlag & XAPIREQE_CANCEL)
        {
            TRMSGI(TRCI_XTCPIP,
                   "Request cancelled during wait iteration=%d\n",
                   i);

            shutdown(clientSocket, 
                     SHUT_RDWR);

            close(clientSocket);

            return STATUS_CANCELLED;
        }

        /*************************************************************/
        /* Now wait for any of the events in the appropriate file    */
        /* descriptor sets.                                          */
        /*************************************************************/
        numReady = select((clientSocket + 1), 
                          pReadFds, 
                          pWriteFds, 
                          pExceptionFds, 
                          &waitTimeval);

        TRMSGI(TRCI_XTCPIP,"numReady=%d after select iteration=%d (nfds=%d, pReadFds=%08X, "
               "pWriteFds=%08X, pExceptionFds=%08X, waitInterval=%d.%d seconds)\n",
               numReady,
               i,
               (clientSocket + 1),
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

    TRMSGI(TRCI_XTCPIP,
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


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: getHostId6                                        */
/** Description:   gethostbyname() replacement for IPv6 support.     */
/**                                                                  */
/** Use getaddrinfo() facilities and return a copy of the 1st        */
/** sockaddr structure (along with length and address family)        */
/** from the returned linked list of addr_info structures.           */
/**                                                                  */
/** STATUS_SUCCESS indicates success (found).                        */
/** Any other RC indicates error (not found).                        */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "getHostId6"

static int getHostId6(struct XAPICVT          *pXapicvt,
                      struct XAPIREQE         *pXapireqe,
                      char                    *pHostName,
                      unsigned int            *pAddressFamily,
                      socklen_t               *pSockaddrLen,
                      struct sockaddr_storage *pSockaddr)
{
    struct addrinfo     hintsAddrinfo;
    struct addrinfo    *pHintsAddrinfo      = &hintsAddrinfo;
    struct addrinfo    *pFirstAddrinfo      = NULL;
    struct addrinfo    *pNextAddrinfo;
    struct sockaddr_in *pSockaddr_in;
    struct sockaddr_in6 *pSockaddr_in6;

    unsigned int        ipAddress[4]        = {0,0,0,0};
    unsigned int        ipPart;             
    int                 lastRC;
    char                hostFoundFlag       = FALSE;

    TRMSGI(TRCI_XTCPIP, 
           "Entered; family=%i, hostname=%s, ipafFlag=%d, "
           "AF_INET=%d, AF_INET6=%d\n",
           *pAddressFamily,    
           pHostName,
           pXapicvt->ipafFlag,
           AF_INET,
           AF_INET6);

    memset(pSockaddr, 0, sizeof(struct sockaddr_storage));
    *pSockaddrLen = 0;

    memset((char*) pHintsAddrinfo, 0, sizeof(struct addrinfo));
    pHintsAddrinfo->ai_family = AF_UNSPEC;
    pHintsAddrinfo->ai_socktype = SOCK_STREAM;

    lastRC = getaddrinfo(pHostName,
                         NULL,
                         pHintsAddrinfo,
                         &pFirstAddrinfo);

    if (lastRC != STATUS_SUCCESS)
    {
        TRMSGI(TRCI_XTCPIP,
               "getaddrinfo RC=%i, errno=%i (%08X)\n",
               lastRC,
               errno,
               errno);

        return errno;
    }
    else
    {
        TRMSGI(TRCI_XTCPIP,
               "getaddrinfo() pFirstAddrinfo=%08X\n",
               pFirstAddrinfo);

        pNextAddrinfo = pFirstAddrinfo;

        while (pNextAddrinfo != NULL)
        {
            TRMEMI(TRCI_XTCPIP, pNextAddrinfo, sizeof(struct addrinfo),
                   "Next addrinfo:\n");

            /*********************************************************/
            /* If we find an IPv6 address, then stop searching.      */
            /*********************************************************/
            if ((pNextAddrinfo->ai_family == AF_INET6) &&
                ((*pAddressFamily == AF_UNSPEC) ||
                 (*pAddressFamily == AF_INET6)))
            {
                memcpy(pSockaddr, 
                       pNextAddrinfo->ai_addr,
                       pNextAddrinfo->ai_addrlen);

                *pSockaddrLen = pNextAddrinfo->ai_addrlen;
                *pAddressFamily = AF_INET6;
                hostFoundFlag = TRUE;
                pSockaddr_in6 = (struct sockaddr_in6*) pNextAddrinfo->ai_addr;

                memcpy((char*) ipAddress,
                       (char*) &pSockaddr_in6->sin6_addr,
                       sizeof(ipAddress));

                TRMEMI(TRCI_XTCPIP, pNextAddrinfo->ai_addr, pNextAddrinfo->ai_addrlen,
                       "Matched IPv6 sockaddr_in:\n");

                break;
            }

            /*********************************************************/
            /* If we find an IPv4 address, prefer the 1st one found  */
            /* but continue searching.                               */
            /*********************************************************/
            if ((pNextAddrinfo->ai_family == AF_INET) &&
                ((*pAddressFamily == AF_UNSPEC) ||
                 (*pAddressFamily == AF_INET)))
            {
                if (!(hostFoundFlag))
                {
                    memcpy(pSockaddr, 
                           pNextAddrinfo->ai_addr,
                           pNextAddrinfo->ai_addrlen);

                    *pSockaddrLen = pNextAddrinfo->ai_addrlen;
                    *pAddressFamily = AF_INET;
                    hostFoundFlag = TRUE;
                    pSockaddr_in = (struct sockaddr_in*) pNextAddrinfo->ai_addr;

                    TRMEMI(TRCI_XTCPIP, pNextAddrinfo->ai_addr, pNextAddrinfo->ai_addrlen,
                           "Matched IPv4 sockaddr_in:\n");

                    ipPart = htonl(pSockaddr_in->sin_addr.s_addr);

                    TRMEMI(TRCI_XTCPIP, (char*) &ipPart, 4, "htonl ipPart:\n");

                    memcpy((char*) &(ipAddress[3]),
                           (char*) &ipPart,
                           4);
                }
                else
                {
                    TRMEMI(TRCI_XTCPIP, pNextAddrinfo->ai_addr, pNextAddrinfo->ai_addrlen,
                           "Subsequent/ignored IPv4 sockaddr_in:\n");
                }
            }

            pNextAddrinfo = pNextAddrinfo->ai_next;
        }

        freeaddrinfo(pFirstAddrinfo);
    }

    TRMSGI(TRCI_XTCPIP,
           "Exit; hostFoundFlag=%i, family=%i, "
           "ipAddress=%08X.%08X.%08X.%08X\n",
           hostFoundFlag,
           *pAddressFamily,
           ipAddress[0],
           ipAddress[1],
           ipAddress[2],
           ipAddress[3]);

    if (hostFoundFlag)
    {
        return STATUS_SUCCESS;
    }

    return STATUS_NI_FAILURE;
}




