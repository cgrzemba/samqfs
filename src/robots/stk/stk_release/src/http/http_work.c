/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_work.c                                      */
/** Description:    t_http server executable worker thread.          */
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


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#define RECV_INCREMENT         16384
#define RECV_TIMEOUT_1ST       60  
#define RECV_TIMEOUT_NON1ST    5 
#define EINTR_RETRY_COUNT      5   
#define TIMEOUT_RETRY_COUNT    2


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static struct HTTPCVT *attachHttpcvt(void);

static int recvAll(struct HTTPCVT *pHttpcvt,
                   int             socketId,
                   int            *pRecvLen,
                   char          **ptrRecvBuffer);


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: http_work                                         */
/** Description:   t_http server executable worker thread.           */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_work"

int http_work (struct HTTPCVT *pParentHttpcvt,
               int             socketId)
{
    struct HTTPCVT     *pHttpcvt            = NULL;
    int                 recvRC;
    int                 lastRC;
    int                 seqNum;
    int                 recvLen;
    struct XMLPARSE    *pXmlparse           = NULL;
    struct XMLELEM     *pTaskTokenXmlelem   = NULL;
    struct XMLELEM     *pCommandXmlelem     = NULL;
    struct XMLELEM     *pCmdChildXmlelem    = NULL;
    char               *pRecvBuffer         = NULL;
    char                seqNumString[9];
    char                commandString[33];

    TRMSGI(TRCI_SERVER,
           "Entered; socketId=%d\n",
           socketId);

    pHttpcvt = attachHttpcvt();

    if (pHttpcvt == NULL)
    {
        LOGMSG(STATUS_SHARED_MEMORY_ERROR, 
               "HTTP thread could not attach HTTPCVT shared memory segment");

        shutdown(socketId, 
                 SHUT_RDWR);

        close(socketId);

        return STATUS_SHARED_MEMORY_ERROR;
    }

    /*****************************************************************/
    /* Make the accept() socket a non-blocking socket so we have     */
    /* more control over TCP/IP timeout values using the select      */
    /* wait mechanism.                                               */
    /*                                                               */
    /* NOTE: We ignore any fcntl() errors and continue.              */
    /*****************************************************************/
    lastRC = fcntl(socketId, 
                   F_SETFL, 
                   O_NONBLOCK);

    TRMSGI(TRCI_HTCPIP,
           "lastRC=%d after fcntl(socket=%d, cmd=F_SETFL=%d, "
           "flags=O_NONBLOCK=%08X)\n",
           lastRC,
           socketId,
           F_SETFL,
           O_NONBLOCK);

    /*****************************************************************/
    /* recv() all of the request data.                               */
    /*****************************************************************/
    recvRC = recvAll(pHttpcvt,
                     socketId,
                     &recvLen, 
                     &pRecvBuffer);

    /*****************************************************************/
    /* If recv() success, test for presence of data.                 */
    /* If no data, then set a bad recvRC.                            */
    /*****************************************************************/
    TRMSGI(TRCI_HTCPIP,
           "recvAll() RC=%d, recvLen=%d, pRecvBuffer=%d\n",
           recvRC,
           recvLen,
           pRecvBuffer);

    if ((pRecvBuffer == 0) ||
        (recvLen == 0))
    {
        LOGMSG(STATUS_NI_FAILURE, 
               "HTTP recv error, len=0\n");

        recvRC = STATUS_NI_FAILURE;
    }
    else
    {
        TRMEMI(TRCI_SERVER, pRecvBuffer, recvLen,
               "recv() data:\n");

        pXmlparse = FN_PARSE_XML(pRecvBuffer,
                                 (recvLen - 1));

        /*************************************************************/
        /* If no XMLPARSE structure, then set a bad recvRC.          */
        /* If we had an XMLPARSE error returned, then try to         */
        /* continue.                                                 */
        /*************************************************************/
        if (pXmlparse == NULL)
        {
            LOGMSG(STATUS_NI_FAILURE, 
                   "HTTP XML parse error (no XMLPARSE)\n");

            recvRC = STATUS_NI_FAILURE;
        }
        else if (pXmlparse->errorCode != RC_SUCCESS)
        {
            TRMEMI(TRCI_HTCPIP, pXmlparse, sizeof(struct XMLPARSE),
                   "XML parse error RC=%d, reason=%d; XMLPARSE:\n",
                   pXmlparse->errorCode,
                   pXmlparse->reasonCode);
        }
    }

    /*****************************************************************/
    /* If we still indicate recv() success, then find the            */
    /* <task_token> in the XMLPARSE tree and extract the original    */
    /* ACSAPI seqNumber (or packet ID), and then log the XML         */
    /* buffer.                                                       */
    /*****************************************************************/
    if (recvRC == STATUS_SUCCESS)
    {
        pTaskTokenXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                    pXmlparse->pHocXmlelem,
                                                    XNAME_task_token);

        seqNum = -1;

        if (pTaskTokenXmlelem != NULL)
        {
            memset(seqNumString, 0, sizeof(seqNumString));

            memcpy(seqNumString,
                   &(pTaskTokenXmlelem->pContent[8]),
                   8);

            seqNum = atoi(seqNumString); 
        }

        LOG_HTTP_RECV(recvRC, 
                      seqNum, 
                      pHttpcvt->xapiHostname,
                      pHttpcvt->xapiPort,
                      pRecvBuffer, 
                      recvLen,
                      "HTTP recv()\n");
    }

    /*****************************************************************/
    /* Free the final recv() response buffer.                        */
    /*****************************************************************/
    TRMSGI(TRCI_STORAGE,
           "free final pRecvBuffer=%08X, len=%i\n",
           pRecvBuffer,
           recvLen);

    free(pRecvBuffer);

    /*****************************************************************/
    /* Now begin processing the request by extracting the command    */
    /* name.                                                         */
    /* If the <command> tag has no content, then the command name    */
    /* must be the 1st child xml element name under the <command>    */
    /* tag.                                                          */
    /*****************************************************************/
    memset(commandString, 0 , sizeof(commandString));

    pCommandXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                              pXmlparse->pHocXmlelem,
                                              XNAME_command);

    if (pCommandXmlelem != NULL)
    {
        if (pCommandXmlelem->contentLen > 0)
        {
            memcpy(commandString, 
                   pCommandXmlelem->pContent,
                   pCommandXmlelem->contentLen);
        }
        else
        {
            pCmdChildXmlelem = pCommandXmlelem->pChildXmlelem;

            if (pCmdChildXmlelem != NULL)
            {
                memcpy(commandString,
                       pCmdChildXmlelem->pStartElemName,
                       pCmdChildXmlelem->startElemLen);
            }
        }
    }

    /*****************************************************************/
    /* Route to processing routine based upon command string.        */
    /*****************************************************************/
    if (commandString[0] > 0)
    {
        if (strcmp(commandString, "dismount") == 0)
        {
            lastRC = http_dismount(pHttpcvt,
                                   socketId,
                                   seqNum,
                                   (void*) pXmlparse);
        }
        else if (strcmp(commandString, "eject_vol") == 0)
        {
            lastRC = http_eject(pHttpcvt,
                                socketId,
                                seqNum,
                                (void*) pXmlparse);
        }
        else if (strcmp(commandString, "enter") == 0)
        {
            lastRC = http_enter(pHttpcvt,
                                socketId,
                                seqNum,
                                (void*) pXmlparse);
        }
        else if (strcmp(commandString, "mount") == 0)
        {
            lastRC = http_dismount(pHttpcvt,
                                   socketId,
                                   seqNum,
                                   (void*) pXmlparse);
        }
        else if (strcmp(commandString, "query_acs") == 0)
        {
            lastRC = http_qacs(pHttpcvt,
                               socketId,
                               seqNum,
                               (void*) pXmlparse);
        }
        else if (strcmp(commandString, "query_cap") == 0)
        {
            lastRC = http_qcap(pHttpcvt,
                               socketId,
                               seqNum,
                               (void*) pXmlparse);
        }
        else if (strcmp(commandString, "query_drive_info") == 0)
        {
            lastRC = http_qdrive_info(pHttpcvt,
                                      socketId,
                                      seqNum,
                                      (void*) pXmlparse);
        }
        else if (strcmp(commandString, "query_drvtypes") == 0)
        {
            lastRC = http_qdrvtypes(pHttpcvt,
                                    socketId,
                                    seqNum,
                                    (void*) pXmlparse);
        }
        else if (strcmp(commandString, "query_lsm") == 0)
        {
            lastRC = http_qlsm(pHttpcvt,
                               socketId,
                               seqNum,
                               (void*) pXmlparse);
        }
        else if (strcmp(commandString, "query_media") == 0)
        {
            lastRC = http_qmedia(pHttpcvt,
                                 socketId,
                                 seqNum,
                                 (void*) pXmlparse);
        }
        else if (strcmp(commandString, "query_scratch") == 0)
        {
            lastRC = http_qscratch(pHttpcvt,
                                   socketId,
                                   seqNum,
                                   (void*) pXmlparse);
        }
        else if (strcmp(commandString, "query_scr_mnt_info") == 0)
        {
            lastRC = http_qscr_mnt_info(pHttpcvt,
                                        socketId,
                                        seqNum,
                                        (void*) pXmlparse);
        }
        else if (strcmp(commandString, "query_scrpool_info") == 0)
        {
            lastRC = http_qscrpool_info(pHttpcvt,
                                        socketId,
                                        seqNum,
                                        (void*) pXmlparse);
        }
        else if (strcmp(commandString, "query_server") == 0)
        {
            lastRC = http_qserver(pHttpcvt,
                                  socketId,
                                  seqNum,
                                  (void*) pXmlparse);
        }
        else if (strcmp(commandString, "query_volume_info") == 0)
        {
            lastRC = http_qvolume_info(pHttpcvt,
                                       socketId,
                                       seqNum,
                                       (void*) pXmlparse);
        }
        else if (strcmp(commandString, "scratch_vol") == 0)
        {
            lastRC = http_set_scr(pHttpcvt,
                                  socketId,
                                  seqNum,
                                  (void*) pXmlparse);
        }
        else if (strcmp(commandString, "unscratch") == 0)
        {
            lastRC = http_set_unscr(pHttpcvt,
                                    socketId,
                                    seqNum,
                                    (void*) pXmlparse);
        }
        else if (strcmp(commandString, "VOLRPT SUMMARY(SUBPOOL)") == 0)
        {
            lastRC = http_volrpt(pHttpcvt,
                                 socketId,
                                 seqNum,
                                 (void*) pXmlparse);
        }
        else
        {
            TRMSGI(TRCI_SERVER,
                   "Unknown XML command=%s\n",
                   commandString);

            lastRC = http_gen_resp(pHttpcvt,
                                   socketId,
                                   seqNum,
                                   "invalid_command.resp");

            lastRC = STATUS_INVALID_COMMAND;
        }
    }
    else
    {
        TRMSGI(TRCI_SERVER,
               "Cannot extract XML command\n");

        lastRC = http_gen_resp(pHttpcvt,
                               socketId,
                               seqNum,
                               "invalid_command.resp");

        lastRC = STATUS_INVALID_COMMAND;
    }

    /*****************************************************************/
    /* When done, shutdown and close the socket, and                 */
    /* free the XMLPARSE and associated structures.                  */
    /*****************************************************************/
    shutdown(socketId, 
             SHUT_RDWR);

    close(socketId);

    if (pXmlparse != NULL)
    {
        FN_FREE_HIERARCHY_STORAGE(pXmlparse);
    }

    return lastRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: attachHttpcvt                                     */
/** Description:   Get access to the shared memory HTTPCVT.          */
/**                                                                  */
/** A returned NULL address indicates that the HTTPCVT shared        */
/** memory segment could not be located or attached.                 */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "attachHttpcvt"

static struct HTTPCVT *attachHttpcvt(void)
{
    struct HTTPCVT     *pHttpcvt            = NULL;
    int                 shMemSegId;
    int                 shmPermissions;
    key_t               shmKey;
    size_t              shmSize;

    /*****************************************************************/
    /* Locate the shared memory segment for the HTTPCVT.             */
    /*****************************************************************/
    shmKey = HTTP_CVT_SHM_KEY;
    shmSize = sizeof(struct HTTPCVT);
    shmPermissions = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    shMemSegId = shmget(shmKey, shmSize, shmPermissions);

    TRMSGI(TRCI_STORAGE,
           "shMemSegId=%d (%08X) after shmget(key=HTTPCVT=%d, "
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
    /* Attach the shared memory segment for the HTTPCVT to this      */
    /* thread.                                                       */
    /*****************************************************************/
    pHttpcvt = (struct HTTPCVT*) shmat(shMemSegId, NULL, 0);

    TRMSGI(TRCI_STORAGE,
           "pHttpcvt=%08X after shmat(id=HTTPCVT=%d (%08X), NULL, 0)\n",
           pHttpcvt,
           shMemSegId,
           shMemSegId);

    return pHttpcvt;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: recvAll                                           */
/** Description:   recv() all the data available from the socket.    */
/**                                                                  */
/** Operation:                                                       */
/** (1) A dynamic currBuffer of RECV_INCREMENT size is allocated     */
/**     to recv() data into.                                         */
/** (2) A non-consumptive recv() loop for the currBuffer size is     */
/**     performed. We perform this non-consumptive recv() loop       */
/**     until we get an error, or until 2 consecutive recv()         */
/**     operations result in the same length.  However, if           */
/**     currBuffer proves too small (i.e. when the non-consumptive   */
/**     recv() loop completely fills it) it will be reallocated      */
/**     and the non-consumptive recv() loop is continued.  Each      */
/**     reallocation adds RECV_INCREMENT bytes to the size of        */
/**     currBuffer.                                                  */
/** (3) A recvBuffer of the exact length required is allocated.      */
/** (4) A consumptive recv() loop is then entered to recv() all      */
/**     of the data into the recvBuffer.                             */
/** (5) The address and length of the recvBuffer are returned,       */
/**     along with the return code.                                  */
/**                                                                  */
/** NOTE: RC_SUCCESS requires a non-0 return length.                 */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "recvAll"

static int recvAll(struct HTTPCVT *pHttpcvt,
                   int             socketId,
                   int            *pRecvLen,
                   char          **ptrRecvBuffer)
{
    int                 lastRC;
    int                 recvRC              = STATUS_SUCCESS;
    int                 recvTimeout         = -1;
    int                 lastLen             = -1;
    int                 recvLen;
    int                 finalRecvLen;
    int                 totalRecvLen        = 0;
    int                 eintrRetryCount     = 0;
    int                 recvTimeoutCount    = 0;
    int                 currBufferSize      = RECV_INCREMENT;
    int                 finalBufferSize;
    char               *pCurrBuffer         = NULL;
    char               *pFinalBuffer        = NULL;
    char               *pLibtransStart      = NULL;
    char               *pLibtransEnd        = NULL;

    *pRecvLen = 0;
    *ptrRecvBuffer = NULL;
    currBufferSize = RECV_INCREMENT;

    pCurrBuffer = (char*) malloc(currBufferSize);

    memset(pCurrBuffer, 0, currBufferSize);

    TRMSGI(TRCI_STORAGE,
           "malloc non-consumptive response buffer=%08X, len=%i\n",
           pCurrBuffer,
           currBufferSize);

    /*****************************************************************/
    /* We recv() currBufferSize in a non-consumptive recv() loop     */
    /* until a recv() error is returned or until 2 recv() operations */
    /* return the same length.  If we recv() exactly currBufferSize  */
    /* bytes, then we re-allocate a larger buffer incrementing its   */
    /* length each time by RECV_INCREMENT bytes.                     */
    /*                                                               */
    /* NOTE: Unlike the client side, we will not be returned a 0     */
    /* length at end-of-data (a TCP/IP mystery), but instead         */
    /* are returned a totalRecvLen of -1 and an errno = EAGAIN (35   */
    /* or X'23') indicating "resource termporarily unavailable".     */
    /*****************************************************************/
    while (1)
    {
        if (recvTimeout < 0)
        {
            recvTimeout = RECV_TIMEOUT_1ST;
        }
        else
        {
            recvTimeout = RECV_TIMEOUT_NON1ST;
        }

        TRMSGI(TRCI_HTCPIP,
               "At top of recv() for currBuffer; "
               "lastLen=%i, totalRecvLen=%d, recvTimeout=%d\n",
               lastLen,
               totalRecvLen,
               recvTimeout);

        /*************************************************************/
        /* Perform a select() before each recv().                    */
        /*                                                           */
        /* NOTE: A bad return code here means we either timed out,   */
        /* there was a socket error, or the request was cancelled.   */
        /*************************************************************/
        recvRC = http_select_wait(pHttpcvt,
                                  socketId,
                                  FALSE,
                                  recvTimeout);

        TRMSGI(TRCI_HTCPIP,
               "recvRC=%d after http_select_wait(socket=%d, "
               "writeFdsFlag=%d, timeout=%d)\n",
               recvRC,
               socketId,
               FALSE,
               recvTimeout);

        if (recvRC != STATUS_SUCCESS)
        {
            if (recvRC == STATUS_NI_TIMEOUT)
            {
                recvTimeoutCount++;

                TRMSGI(TRCI_HTCPIP,
                       "retry #%i detected for STATUS_NI_TIMEOUT, "
                       "totalRecvLen=%i\n",
                       recvTimeoutCount,
                       totalRecvLen);

                if (recvTimeoutCount <= TIMEOUT_RETRY_COUNT)
                {
                    recvRC = STATUS_SUCCESS;

                    continue;
                }

                /*****************************************************/
                /* If we have a TIMEOUT, but have already received   */
                /* some data, then treat the TIMEOUT as end-of-data. */
                /*****************************************************/
                if (totalRecvLen > 0)
                {
                    TRMSGI(TRCI_HTCPIP,
                           "STATUS_NI_TIMEOUT after totalRecvLen=%i\n",
                           totalRecvLen);

                    recvRC = STATUS_SUCCESS;

                    break;
                }
            }

            LOGMSG(recvRC, 
                   "HTTP select_wait(timeout=%d) error=%s (%d)\n",
                   recvTimeout,
                   acs_status((STATUS) recvRC),
                   recvRC);

            break;
        }

        /*************************************************************/
        /* Do the non-consumptive recv() (with MSG_PEEK option) for  */
        /* a maximum size of currBufferSize.  If we need a larger    */
        /* buffer we'll allocate one below.                          */
        /*************************************************************/
        recvLen = recv(socketId, 
                       pCurrBuffer, 
                       currBufferSize, 
                       MSG_PEEK);

        TRMSGI(TRCI_HTCPIP,
               "recv() RC=%d\n",
               recvLen);

        /*************************************************************/
        /* If recvLen > 0, then assume we are re-reading the         */
        /* entire buffer in a non-consumptive read.                  */
        /*************************************************************/
        if (recvLen > 0)
        {
            totalRecvLen = recvLen;
        }
        /*************************************************************/
        /* If EINTR (interrupted) error, AND                         */
        /*    If < SMCCVT.httpEintrRetryCount retries,               */
        /*    then retry the same recv() after a short wait.         */
        /* If any other recv error, and we've read 0 total bytes,    */
        /*    then return the UUI_IP_RECV_ERR return code.           */
        /* Otherwise, treat any error after we've read any data      */
        /*    as end-of-data.                                        */
        /*************************************************************/
        else if (recvLen < 0)
        {
            if (errno == EINTR)
            {
                eintrRetryCount++;

                TRMSGI(TRCI_HTCPIP,
                       "retry #%i detected for currBuffer;"
                       "errno=%i (%08X), totalRecvLen=%i\n",
                       eintrRetryCount,
                       errno,
                       errno,
                       totalRecvLen);

                if (eintrRetryCount <= EINTR_RETRY_COUNT)
                {
                    sleep(1);

                    continue;
                }
            }

            /*********************************************************/
            /* If we have a recv() error, but have already received  */
            /* some data, then treat the error as end-of-data.       */
            /*********************************************************/
            if (totalRecvLen > 0)
            {
                TRMSGI(TRCI_HTCPIP,
                       "errno=%d (%08X) after %i bytes read "
                       "for currBuffer; breaking\n",
                       errno,
                       errno,
                       totalRecvLen);

                recvRC = STATUS_SUCCESS;
            }
            else
            {
                recvRC = STATUS_NI_FAILURE;
            }

            break;
        }

        eintrRetryCount = 0;
        recvTimeoutCount = 0;

        /*************************************************************/
        /* If we have a recv() of currBufferSize bytes then          */
        /* allocate a new buffer incrementing the size by            */
        /* RECV_INCREMENT bytes.                                     */
        /*************************************************************/
        if (recvLen >= currBufferSize)
        {
            TRMSGI(TRCI_STORAGE,
                   "free non-consumptive response buffer=%08X, len=%i\n",
                   pCurrBuffer,
                   currBufferSize);

            free(pCurrBuffer);

            currBufferSize += RECV_INCREMENT;

            pCurrBuffer = (char*) malloc(currBufferSize);

            memset(pCurrBuffer, 0, currBufferSize);

            TRMSGI(TRCI_STORAGE,
                   "malloc non-consumptive response buffer=%08X, len=%i\n",
                   pCurrBuffer,
                   currBufferSize);
        }
        else if (recvLen == lastLen)
        {
            TRMSGI(TRCI_HTCPIP,
                   "totalRecvLen=%i, lastLen=%i; assumed end-of-data\n",
                   totalRecvLen,
                   lastLen);

            break;
        }

        lastLen = recvLen;
    }

    TRMSGI(TRCI_HTCPIP,
           "totalRecvLen=%i, recvRC=%d\n",
           totalRecvLen,
           recvRC);

    if (totalRecvLen == 0)
    {
        if (recvRC == RC_SUCCESS)
        {
            recvRC = STATUS_NI_FAILURE;
        }
    }

    /*****************************************************************/
    /* Free our non-consumptive buffer.                              */
    /*****************************************************************/
    TRMSGI(TRCI_STORAGE,
           "free non-consumptive response buffer=%08X, len=%i\n",
           pCurrBuffer,
           currBufferSize);

    free(pCurrBuffer);

    /*****************************************************************/
    /* Now recv() all of the data in a consumptive recv() loop.      */
    /* Allocate a final buffer that is just large enough for all     */
    /* the data.                                                     */
    /*****************************************************************/
    if (recvRC == RC_SUCCESS)
    {
        currBufferSize = totalRecvLen;
        finalRecvLen = totalRecvLen;

        pCurrBuffer = (char*) malloc(currBufferSize);

        memset(pCurrBuffer, 0, currBufferSize);

        TRMSGI(TRCI_STORAGE,
               "malloc consumptive response buffer=%08X, len=%i\n",
               pCurrBuffer,
               currBufferSize);

        recvTimeout = -1;
        recvTimeoutCount = 0;
        eintrRetryCount = 0;
        totalRecvLen = 0;
        lastLen = -1;
        recvRC = STATUS_SUCCESS;

        while (1)
        {
            if (recvTimeout < 0)
            {
                recvTimeout = RECV_TIMEOUT_1ST;
            }
            else
            {
                recvTimeout = RECV_TIMEOUT_NON1ST;
            }

            TRMSGI(TRCI_HTCPIP,
                   "At top of recv() for currBuffer; "
                   "lastLen=%i, totalRecvLen=%d, "
                   "finalRecvLen=%d, recvTimeout=%d\n",
                   lastLen,
                   totalRecvLen,
                   finalRecvLen,
                   recvTimeout);

            /*********************************************************/
            /* Perform a select() before each recv().                */
            /*                                                       */
            /* NOTE: A bad return code here means we either timed    */
            /* out, there was a socket error, or the request was     */
            /* cancelled.                                            */
            /*********************************************************/
            recvRC = http_select_wait(pHttpcvt,
                                      socketId,
                                      FALSE,
                                      recvTimeout);

            TRMSGI(TRCI_HTCPIP,
                   "recvRC=%d after http_select_wait(socket=%d, "
                   "writeFdsFlag=%d, timeout=%d)\n",
                   recvRC,
                   socketId,
                   FALSE,
                   recvTimeout);

            if (recvRC != STATUS_SUCCESS)
            {
                if (recvRC == STATUS_NI_TIMEOUT)
                {
                    recvTimeoutCount++;

                    TRMSGI(TRCI_HTCPIP,
                           "retry #%i detected for STATUS_NI_TIMEOUT, "
                           "totalRecvLen=%i\n",
                           recvTimeoutCount,
                           totalRecvLen);

                    if (recvTimeoutCount <= TIMEOUT_RETRY_COUNT)
                    {
                        recvRC = STATUS_SUCCESS;

                        continue;
                    }

                    /*************************************************/
                    /* If we have a TIMEOUT, but have already        */
                    /* received some data, then treat the TIMEOUT    */
                    /* as end-of-data.                               */
                    /*************************************************/
                    if (totalRecvLen > 0)
                    {
                        TRMSGI(TRCI_HTCPIP,
                               "STATUS_NI_TIMEOUT after totalRecvLen=%i\n",
                               totalRecvLen);

                        recvRC = STATUS_SUCCESS;

                        break;
                    }
                }

                LOGMSG(recvRC, 
                       "HTTP select_wait(timeout=%d) error=%s (%d)\n",
                       recvTimeout,
                       acs_status((STATUS) recvRC),
                       recvRC);

                break;
            }

            /*************************************************************/
            /* Do the consumptive recv() (without MSG_PEEK option) for   */
            /* a maximum size of (currBufferSize - 1).                   */
            /*************************************************************/
            recvLen = recv(socketId, 
                           pCurrBuffer, 
                           currBufferSize, 
                           0);

            TRMSGI(TRCI_HTCPIP,
                   "recv() RC=%d\n",
                   recvLen);

            /*********************************************************/
            /* If EINTR (interrupted) error, AND                     */
            /*    If < EINTR_RETRY_COUNT retries,                    */
            /*    then retry the same recv() after a short wait.     */
            /* If any other recv error, and we've read 0 total bytes,*/
            /*    then return the STATUS_NI_ERROR return code.       */
            /* Otherwise, treat any error after we've read any data  */
            /*    as end-of-data.                                    */
            /*********************************************************/
            if (recvLen < 0)
            {
                if (errno == EINTR)
                {
                    eintrRetryCount++;

                    TRMSGI(TRCI_HTCPIP,
                           "retry #%i detected for currBuffer;"
                           "errno=%i (%08X), totalRecvLen=%i\n",
                           eintrRetryCount,
                           errno,
                           errno,
                           totalRecvLen);

                    if (eintrRetryCount <= EINTR_RETRY_COUNT)
                    {
                        sleep(1);

                        continue;
                    }
                }
                /*********************************************************/
                /* If we have a recv() error, but have already received  */
                /* some data, then treat the error as end-of-data.       */
                /*********************************************************/
                if (totalRecvLen > 0)
                {
                    TRMSGI(TRCI_HTCPIP,
                           "errno=%d (%08X) after %i bytes read "
                           "for currBuffer; breaking\n",
                           errno,
                           errno,
                           totalRecvLen);

                    recvRC = STATUS_SUCCESS;
                }
                else
                {
                    recvRC = STATUS_NI_FAILURE;
                }

                break;
            }
            /*********************************************************/
            /* If recv RC = 0, then it should be end-of-data.        */
            /* To handle possible multiple blocks, look for two      */
            /* consecutive recv of 0 bytes.                          */
            /*                                                       */
            /* NOTE: This condition does not appear to occur on the  */
            /* server side (while it occurs on the client side).     */
            /* This is a TCP/IP mystery, but we perform this test    */
            /* just in case.                                         */
            /*********************************************************/
            else if (recvLen == 0)
            {
                if (lastLen == 0)
                {
                    TRMSGI(TRCI_HTCPIP,
                           "2 consecutive end-of-data\n");

                    break;
                }
                else
                {
                    lastLen = 0;

                    TRMSGI(TRCI_HTCPIP,
                           "1 end-of-data; continuing\n");

                    continue;
                }
            }

            /*********************************************************/
            /* Otherwise if we recv() all the data that we expect,   */
            /* then assume end-of-data.                              */
            /*********************************************************/
            eintrRetryCount = 0;
            recvTimeoutCount = 0;
            lastLen = recvLen;
            totalRecvLen += recvLen;

            if (totalRecvLen >= finalRecvLen)
            {
                TRMSGI(TRCI_HTCPIP,
                       "totalRecvLen=%i, finalRecvLen=%i; "
                       "presumed end-of-data\n",
                       totalRecvLen,
                       finalRecvLen);

                break;
            }
        }
    }

    /*****************************************************************/
    /* Allocate a final buffer that is the length of the <libtrans>  */
    /* to </libtrans> with a single trailing null.                   */
    /*****************************************************************/
    pLibtransStart = strstr(pCurrBuffer, XSTARTTAG_libtrans);
    pLibtransEnd = strstr(pCurrBuffer, XENDTAG_libtrans);

    if ((pLibtransStart == NULL) ||
        (pLibtransEnd == NULL))
    {
        LOGMSG(STATUS_NI_FAILURE, 
               "HTTP <libtrans> not found error\n");

        recvRC = STATUS_NI_FAILURE;
    }
    else
    {
        finalBufferSize = (int) (pLibtransEnd - pLibtransStart) + strlen(XENDTAG_libtrans) + 1;

        pFinalBuffer = (char*) malloc(finalBufferSize);

        memset(pFinalBuffer, 0, finalBufferSize);

        TRMSGI(TRCI_STORAGE,
               "malloc final response buffer=%08X, len=%i\n",
               pFinalBuffer,
               finalBufferSize);

        memcpy(pFinalBuffer, 
               pLibtransStart,
               finalBufferSize);

        TRMSGI(TRCI_STORAGE,
               "free consumptive response buffer=%08X, len=%i\n",
               pCurrBuffer,
               currBufferSize);

        free(pCurrBuffer);

        *pRecvLen = finalBufferSize;
        *ptrRecvBuffer = pFinalBuffer;
    }

    return recvRC;
}



