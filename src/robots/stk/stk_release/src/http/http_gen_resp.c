/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_gen_resp.c                                  */
/** Description:    t_http server generate response from             */
/**                 pre-canned response data file.                   */
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
#define SEND_TIMEOUT           20
#define SEND_RETRY_COUNT       3


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int convertBackslashStrings(char pInputBuffer[],
                                   int  inBufferLen,
                                   char pOutputBuffer[]);

static int sendBuffer(struct HTTPCVT *pHttpcvt,
                      int             socketId,
                      char           *pSendBuffer,
                      int             sendBufferLen);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: http_gen_resp                                     */
/** Description:                                                     */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "http_gen_resp"

int http_gen_resp(struct HTTPCVT *pHttpcvt,
                  int             socketId,
                  int             seqNum,
                  char           *respFileName)
{
    int                 lastRC;
    int                 readRC;
    int                 finalRC             = STATUS_SUCCESS;
    int                 respLen;
    int                 outputLen;

    FILE               *pRespFile           = NULL;

    char                openReadMode[]      = {"r"};
    char                respFileHandle[512];
    char                respBuffer[HTTP_SEND_BUFFER_SIZE + 1];
    char                outputBuffer[(2 * HTTP_SEND_BUFFER_SIZE) + 1];


    memset(respFileHandle, 0, sizeof(respFileHandle));

    if (getenv(HTTP_RESPONSE_DIR) != NULL)
    {
        strcpy(respFileHandle, getenv(HTTP_RESPONSE_DIR));
    }

    if (respFileHandle[(strlen(respFileHandle)) - 1] != '/')
    {
        strcat(respFileHandle, "/");
    }

    strcat(respFileHandle, respFileName);

    TRMSGI(TRCI_SERVER,
           "respFileHandle=%s",
           respFileHandle);

    memset(respBuffer, 
           0, 
           sizeof(respBuffer));

    pRespFile = fopen(respFileHandle, 
                      openReadMode);

    if (pRespFile == NULL)
    {
        printf("HTTP http_gen_resp fopen() failure for file=%s\n", 
               respFileHandle);

        finalRC = STATUS_PROCESS_FAILURE;
    }
    else
    {
        while (1)
        {
            readRC = http_read_next((void*) pRespFile,
                                    respBuffer,
                                    HTTP_SEND_BUFFER_SIZE,
                                    &respLen,
                                    respFileHandle);

            TRMSGI(TRCI_SERVER,
                   "http_read_next RC=%i\n",
                   readRC);

            if (readRC == RC_FAILURE)
            {
                finalRC = STATUS_QUEUE_FAILURE;

                break;
            }

            if (readRC == RC_WARNING)
            {
                finalRC = STATUS_SUCCESS;

                break;
            }

            memset(outputBuffer, 
                   0, 
                   sizeof(outputBuffer));

            outputLen = convertBackslashStrings(respBuffer,
                                                respLen,
                                                outputBuffer);

            TRMSGI(TRCI_SERVER,
                   "respLen=%i, outputLen=%i; outputBuffer:\n",
                   respLen,
                   outputLen);

            lastRC = sendBuffer(pHttpcvt,
                                socketId,
                                outputBuffer,
                                outputLen);

            LOG_HTTP_SEND(lastRC, 
                          seqNum, 
                          pHttpcvt->xapiHostname,
                          pHttpcvt->xapiPort,
                          outputBuffer, 
                          outputLen,
                          "HTTP send()\n");

            if (lastRC != STATUS_SUCCESS)
            {
                finalRC = lastRC;

                break;
            }
        }
    }

    if (pRespFile != NULL)
    {
        fclose(pRespFile);
    }

    TRMSGI(TRCI_SERVER,
           "http_gen_resp(%s) RC=%d\n",
           respFileHandle,
           finalRC);

    return finalRC;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: convertBackslashStrings                           */
/** Description:   Convert any "\Xnn" or "\xnn" strings in the       */
/**                input buffer into a single character in the       */
/**                output buffer.  The return code is the new        */
/**                output buffer length.                             */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "convertBackslackStrings"

static int convertBackslashStrings(char pInputBuffer[],
                                   int  inBufferLen,
                                   char pOutputBuffer[])
{
    int                 i;
    int                 j;
    int                 lastRC;
    unsigned int        charNum;
    char                charConvString[3];

    for (i = 0, j = 0; 
        i < inBufferLen; 
        i++)
    {
        if ((i < (inBufferLen - 4)) &&
            ((memcmp(&pInputBuffer[i],
                     "\\X",
                     2) == 0) ||
             (memcmp(&pInputBuffer[i],
                     "\\x",
                     2) == 0)))
        {
            TRMSGI(TRCI_SERVER, 
                   "Found suspect backslash string\n");

            if ((isxdigit(pInputBuffer[(i + 2)])) &&
                (isxdigit(pInputBuffer[(i + 3)])))
            {
                charConvString[0] = toupper(pInputBuffer[(i + 2)]);
                charConvString[1] = toupper(pInputBuffer[(i + 3)]);
                charConvString[2] = 0;

                lastRC = FN_CONVERT_CHARHEX_TO_FULLWORD(charConvString,
                                                        2,
                                                        &charNum);

                if (lastRC == RC_SUCCESS)
                {
                    TRMSGI(TRCI_SERVER,
                           "Substituting %02X for %.4s\n",
                           charNum,
                           &(pInputBuffer[i]));

                    pOutputBuffer[j] = charNum;
                    j++;
                    i += 3;
                }
                else
                {
                    TRMEMI(TRCI_SERVER, &(pInputBuffer[i]), 4,
                           "Non-hex backslash string (1):\n");

                    pOutputBuffer[j] = pInputBuffer[i];
                    j++;
                }
            }
            else
            {
                TRMEMI(TRCI_SERVER, &(pInputBuffer[i]), 4,
                       "Non-hex backslash string (2):\n");

                pOutputBuffer[j] = pInputBuffer[i];
                j++;
            }

            continue;
        }

        pOutputBuffer[j] = pInputBuffer[i];
        j++;
    }

    return j;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: sendBuffer                                        */
/** Description:   Perform an send() on the specified socket.        */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "sendBuffer"

static int sendBuffer(struct HTTPCVT *pHttpcvt,
                      int             socketId,
                      char           *pSendBuffer,
                      int             sendBufferLen)
{
#define MAX_SEND_TRACE_LEN 200

    auto int                 sendTraceLen;
    auto int                 lastRC;
    auto int                 sendRC              = STATUS_SUCCESS;
    auto int                 partialSendCount    = 0;
    auto int                 sendCount;
    auto int                 bytesWritten;
    auto int                 bytesRemaining;
    auto int                 currPos;

    TRMSGI(TRCI_HTCPIP, 
           "Entered; socketId=%i\n",
           socketId);

    bytesRemaining = sendBufferLen;
    sendCount = 0;
    partialSendCount = 0;
    currPos = 0;

    while (bytesRemaining > 0)
    {
        sendCount++;
        sendRC = STATUS_SUCCESS;

        TRMSGI(TRCI_HTCPIP,
               "send-top-of-while; sendCount=%d, "
               "partialSendCount=%d, bytesRemaining=%i, "
               "currPos=%i\n",
               sendCount,
               partialSendCount,
               bytesRemaining,
               currPos);

        sendRC = http_select_wait(pHttpcvt,
                                  socketId,
                                  TRUE,
                                  SEND_TIMEOUT);

        TRMSGI(TRCI_HTCPIP,
               "sendRC=%d after http_select_wait(socket=%d, "
               "writeFdsFlag=%d, timeout=%d)\n",
               sendRC,
               socketId,
               TRUE,
               SEND_TIMEOUT);

        if (sendRC == STATUS_NI_TIMEOUT)
        {
            if (bytesWritten > 0)
            {
                partialSendCount++;

                if (partialSendCount <= SEND_RETRY_COUNT)
                {
                    TRMSGI(TRCI_HTCPIP, 
                           "Timeout after bytesWritten=%i "
                           "and partialSendCount=%i<=max=%i; "
                           "continuing\n",
                           bytesWritten,
                           partialSendCount,
                           SEND_RETRY_COUNT);

                    continue;
                }
                else
                {
                    TRMSGI(TRCI_HTCPIP, 
                           "Timeout after bytesWritten=%i "
                           "and partialSendCount=%i>max=%i\n",
                           bytesWritten,
                           partialSendCount,
                           SEND_RETRY_COUNT);
                }
            }
            else
            {
                TRMSGI(TRCI_HTCPIP, 
                       "Timeout and bytesWritten=0\n");
            }
        }

        if (sendRC != STATUS_SUCCESS)
        {
            LOGMSG(sendRC, 
                   "HTTP select_wait(timeout=%d) error=%s (%d)\n",
                   SEND_TIMEOUT,
                   acs_status((STATUS) sendRC),
                   sendRC);

            break;
        }

        /*************************************************************/
        /* Now go send() the data.                                   */
        /*************************************************************/
        bytesWritten = send(socketId,
                            &(pSendBuffer[currPos]),
                            bytesRemaining,
                            0);

        if (bytesWritten < 0)
        {
            LOGMSG(STATUS_NI_FAILURE, 
                   "HTTP send() error on socket=%d RC=%d (%08X)\n",
                   socketId,
                   errno,
                   errno);

            sendRC = STATUS_NI_FAILURE;

            break;
        }
        else
        {
            bytesRemaining -= bytesWritten;
            currPos += bytesWritten;

            TRMSGI(TRCI_HTCPIP,
                   "send() bytesWritten=%i, "
                   "bytesRemaining=%i, currPos=%i\n",
                   bytesWritten,
                   bytesRemaining,
                   currPos);
        }
    }

    TRMSGI(TRCI_HTCPIP, 
           "send-end-of-while; sendRC=%i, "
           "sendCount=%i, partialSendCount=%i, "
           "bytesWritten=%i, bytesRemaining=%i\n",
           sendRC,
           sendCount,
           partialSendCount,
           bytesWritten,
           bytesRemaining);

    return sendRC;
}




