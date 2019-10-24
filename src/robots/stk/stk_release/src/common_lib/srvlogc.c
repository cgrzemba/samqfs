/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      srvlogc.c                                        */
/** Description:    The log service client common component.         */
/**                                                                  */
/**                 The log client writes output to the log.pipe.    */
/**                                                                  */
/**                 There are 3 environmental variables to           */
/**                 control the log services implemented using       */
/**                 the LOG* macros in src/h/srvcommon.h.            */
/**                                                                  */
/**                 SLOGPIPE=pipe.name to specify the name of the    */
/**                 log pipe that log clients write to using         */
/**                 srvcommon.h LOG* macros (or the srvlogc          */
/**                 function).  SLOGPIPE is also used by the         */
/**                 log server (srvlogs) to specify the pipe that    */
/**                 will be read to create the final log file.       */
/**                                                                  */
/**                 SLOGFILE=file.name to specify the name of log    */
/**                 file that the log server (srvlogs) will write    */
/**                 the final log file.                              */
/**                                                                  */
/**                 CDKLOG={1|0} To turn on logging.                 */
/**                 The CDKLOG variable can be up to 16 characters   */
/**                 long.  Each character is evaluated as if it      */
/**                 were a bit (0 or 1) (i.e. CDKLOG=1100)           */
/**                 If the specified character "bit" in the          */
/**                 CDKLOG variable  is on (set to "1"),             */
/**                 the corresponding log process is enabled.        */
/**                 If the specified character "bit" in the          */
/**                 CDKLOG variable is not on (set to something      */
/**                 other than "1"), the corresponding log           */
/**                 log process is disabled.                         */
/**                                                                  */
/**                 NOTE that "1" and only "1" turns on the log      */
/**                 process.  These log processes are as follows:    */
/**                                                                  */
/**                 CDKLOG=1           Log event messages            */
/**                                    to the log file.              */
/**                 CDKLOG=01          Log event error messages      */
/**                                    to stdout     .               */
/**                 CDKLOG=001         Log XAPI ACSAPI send packets  */
/**                                    to the log.file.              */
/**                 CDKLOG=0001        Log XAPI ACSAPI recv packets  */
/**                                    to the log.file.              */
/**                 CDKLOG=00001       Log XAPI XML send packets     */
/**                                    to the log.file.              */
/**                 CDKLOG=000001      Log XAPI XML recv packets     */
/**                                    to the log.file.              */
/**                 CDKLOG=0000001     Log CSI (RPC or ADI) send     */
/**                                    packets to the log.file.      */
/**                 CDKLOG=00000001    Log CSI (RPC or ADI) recv     */
/**                                    packets to the log.file.      */
/**                 CDKLOG=000000001   Log HTTP XML send packets     */
/**                                    to the log.file.              */
/**                 CDKLOG=0000000001  Log HTTP XML recv packets     */
/**                                    to the log.file.              */
/**                                                                  */
/**                 There are 2 additional global variables used     */
/**                 to fast pass certain decisions within the        */
/**                 log services implemented by src/h/srvcommon.h.   */
/**                                                                  */
/**                 global_cdklog_flag is set by srvvars             */
/**                 depending upon the CDKLOG variable.              */
/**                                                                  */
/**                 global_srvlogs_active_flag is always set by      */
/**                 srvvars as it assumes the srvlogs daemon         */
/**                 is active.  However it will be reset by this     */
/**                 function if output to the log pipe results       */
/**                 in an error.                                     */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720         Joseph Nofi     05/03/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/***END PROLOGUE******************************************************/

/*********************************************************************/
/* Includes:                                                         */
/*********************************************************************/
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "api/defs_api.h"
#include "cl_pub.h"
#include "csi.h"
#include "srvcommon.h"
#include "xapi/xapi.h"


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#define MAX_STORAGE_SIZE           4096
#define MAX_STORAGE_PER_LINE       32
#define MAX_STORAGE_LINES          MAX_STORAGE_SIZE / MAX_STORAGE_PER_LINE
#define MAX_XML_PER_LINE           100
#define MAX_FILENAME_SIZE          16


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int srvlogc_packet(int   logFd,
                          FILE *pLogFile,
                          int   logIndex,
                          char *packetAddress,
                          int   packetLength);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: srvlogc                                           */
/** Description:   Log service client                                */
/**                                                                  */
/** This function:                                                   */
/** (1) Tests if the $CDKLOG environment variable is set.            */
/**     If not set to "0", then return;                              */
/** (2) Gets the $SLOGPIPE variable for the log pipe name and        */
/**     opens the log pipe.                                          */
/** (3) Creates and writes the log "header" lines;                   */
/**     If passed a log message, then the log message is written     */
/**     as part of the log "header".                                 */
/** (4) If passed a packet address, then calls srvlogc_packet        */
/**     to format and log the packet.                                */
/** (5) Closes the log pipe.                                         */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "srvlogc"

extern int srvlogc(char *fileName,
                   int   lineNumber,
                   int   status,
                   int   packetId,
                   char *packetAddress,
                   int   packetLength,
                   int   logIndex,
                   char *packetAddressString1,
                   char *packetAddressString2,
                   char *logMsg)
{
    int                 lastRC; 
    int                 logPid;
    int                 logFd               = 0;
    FILE               *pLogFile            = NULL;
    int                 bufferLength;
    int                 commandLen;

    time_t              logTime;

    REQUEST_HEADER     *pRequest_Header;
    MESSAGE_HEADER     *pMessage_Header;
    QUERY_REQUEST      *pQuery_Request;
    QUERY_RESPONSE     *pQuery_Response;

    char               *pXapiIdString;
    char               *pNextTag;
    char               *pOne;
    char                logBuffer[SLOG_MAX_LINE_SIZE + 1];
    char                logVarValue[256];
    char                logFlag[MAX_CDKLOG_TYPE + 1];
    char                timeString[80];
    char                cdkType[MAX_MODULE_TYPE_SIZE + 1] = "";
    char                typeString[9];
    char                commandString[80];
    char                sourceString[24];
    char                fixedFileName[MAX_FILENAME_SIZE + 1];
    char                fixedCdkType[MAX_MODULE_TYPE_SIZE + 1];

    /*****************************************************************/
    /* Test if CDKLOG variable is set.                               */
    /* If not set, then set it now to "0".                           */
    /*****************************************************************/
    memset(logFlag, 0, sizeof(logFlag));

    if (getenv(CDKLOG_VAR) != NULL)
    {
        strncpy(logFlag, getenv(CDKLOG_VAR), MAX_CDKLOG_TYPE);
    }
    else
    {
        SETENV(CDKLOG_VAR, "0");

        return(0);
    }

    /*****************************************************************/
    /* Get value of CDKTYPE environment variable.                    */
    /*****************************************************************/
    if (getenv(CDKTYPE_VAR) != NULL)
    {
        memset(cdkType, 0 , sizeof(cdkType));
        strncpy(cdkType, getenv(CDKTYPE_VAR), MAX_MODULE_TYPE_SIZE);
    }
    else
    {
        strcpy(cdkType, global_module_type);
    }

    memset(fixedCdkType, ' ', MAX_MODULE_TYPE_SIZE);
    fixedCdkType[MAX_MODULE_TYPE_SIZE] = 0; 

    if (strlen(cdkType) > MAX_MODULE_TYPE_SIZE)
    {
        memcpy(fixedCdkType, 
               cdkType, 
               MAX_MODULE_TYPE_SIZE);
    }
    else
    {
        memcpy(fixedCdkType, 
               cdkType, 
               strlen(cdkType));
    }

/*
    printf("cdkType=%s\n", 
           cdkType);
*/

    /*****************************************************************/
    /* If we are logging errors to stdout, then perform that first.  */
    /*****************************************************************/
    if (logFlag[LOGI_ERROR_STDOUT] == '1')
    {
        if (status != STATUS_SUCCESS)
        {
            if ((logMsg != NULL) &&
                (logMsg[0] > 0))
            {
                strcpy(logBuffer, logMsg);
                bufferLength = strlen(logBuffer);

                if (logBuffer[(bufferLength - 1)] != '\n')
                {
                    logBuffer[bufferLength] = '\n';
                    bufferLength++;
                    logBuffer[bufferLength] = 0;
                    bufferLength++;
                }

                printf("%s", logBuffer);
            }
        }
    }

    if ((logIndex == LOGI_EVENT_MESSAGE) &&
        (logFlag[LOGI_EVENT_MESSAGE] != '1'))
    {
        return(0);
    }

    /*****************************************************************/
    /* Packet log settings work in conjunction with the message log  */
    /* settings.  If message logging is enabled, but packet logging  */
    /* is disabled, then the packet log message is still written     */
    /* to the log file (but the buffer is not logged).               */
    /*****************************************************************/
    if ((logIndex != LOGI_EVENT_MESSAGE) &&
        (logFlag[LOGI_EVENT_MESSAGE] != '1'))
    {
        if (logFlag[logIndex] != '1')
        {
            return(0);
        }
    }

    /*****************************************************************/
    /* If the srvlogs log service daemon is active, then open        */
    /* the SLOGPIPE (log pipe) which is the preferred log target.    */
    /*****************************************************************/
    if (global_srvlogs_active_flag)
    {
        if (getenv(SLOGPIPE_VAR) != NULL)
        {
            strcpy(logVarValue, getenv(SLOGPIPE_VAR));
        }
        else
        {
            strcpy(logVarValue, DEFAULT_SLOGPIPE);
        }

        logFd = open(logVarValue, (O_WRONLY | O_NONBLOCK | O_SYNC));

        /*************************************************************/
        /* If error opening the log pipe, then reset the             */
        /* global_srvlogs_active_flag, and call ourself and          */
        /* attempt to write to the SLOGFILE directly.                */
        /*************************************************************/
        if (logFd == 0)
        {
            printf("%s[%i]: Error opening log pipe=%s; errno=%i; srvlogs not active\n",
                   __FILE__,
                   __LINE__,
                   logVarValue,
                   errno);

            global_srvlogs_active_flag = FALSE;

            lastRC = srvlogc(fileName,
                             lineNumber,
                             status,
                             packetId,
                             packetAddress,
                             packetLength,
                             logIndex,
                             packetAddressString1,
                             packetAddressString2,
                             logMsg);

            return lastRC;
        }
    }
    /*****************************************************************/
    /* Otherwise, if the srvlogs log service daemon is NOT active,   */
    /* then open the SLOGFILE (log file) which is a much slower      */
    /* log target.                                                   */
    /*****************************************************************/
    else
    {
        if (getenv(SLOGFILE_VAR) != NULL)
        {
            strcpy(logVarValue, getenv(SLOGFILE_VAR));
        }
        else
        {
            strcpy(logVarValue, DEFAULT_SLOGFILE);
        }

        pLogFile = fopen(logVarValue, "ab");

        /*************************************************************/
        /* If error opening the log file, then reset the             */
        /* global_cdklog_flag to turn tracing off altogether.        */
        /*************************************************************/
        if (pLogFile == NULL)
        {
            printf("%s[%i]: Error opening log file=%s; errno=%i; log now disabled\n",
                   __FILE__,
                   __LINE__,
                   logVarValue,
                   errno);

            SETENV(CDKLOG_VAR, "0");
            global_cdklog_flag = FALSE;

            return(-1);
        }
    }

    /*****************************************************************/
    /* Format and print the log message "header" lines.              */
    /*===============================================================*/
    /* The 5 LOG "header" lines will vary depending upon the type of */
    /* packet being logged:                                          */
    /*                                                               */
    /* For LOGI_EVENT_ACSAPI_SEND and LOGI_EVENT_ACSAPI_RECV:        */
    /* (1)  Date Time [type:process_id] status                       */
    /* (2)  [object:line] message                                    */
    /* (3)  Packet source:      From/To                              */
    /* (4)  Packet ID:          number     command:  name            */
    /* (5)  SSI return socket:  number     SSI return pid:   number  */
    /*                                                               */
    /* For LOGI_EVENT_XAPI_SEND and LOGI_EVENT_XAPI_RECV and         */
    /* for LOGI_EVENT_HTTP_SEND and LOGI_EVENT_HTTP_RECV:            */
    /* (1)  Date Time [type:process_id] status                       */
    /* (2)  [object:line] message                                    */
    /* (3)  Packet source:      From/To                              */
    /* (4)  Packet ID:          number     command:  name            */
    /* (5)  HTTP server host:   name       HTTP server port: number  */
    /*                                                               */
    /* For LOGI_EVENT_CSI_SEND and LOGI_EVENT_CSI_RECV:              */
    /* (1)  Date Time [type:process_id] status                       */
    /* (2)  [object:line] message                                    */
    /* (3)  Packet source:      From/To                              */
    /* (4)  SSI identifier:     number     command:  name            */
    /* (5)  SSI client address: name       SSI client port:  number  */
    /*                                                               */
    /* These header lines must be kept synchronized with             */
    /* td_get_packe.c where the log.file is processed and            */
    /* individual packets decoded.                                   */
    /*****************************************************************/

    /*****************************************************************/
    /* If we were passed a packet to log, then extract a printable   */
    /* command name.                                                 */
    /*****************************************************************/
    if (packetAddress != NULL)
    {
        if (logIndex == LOGI_EVENT_ACSAPI_RECV)
        {
            pRequest_Header = (REQUEST_HEADER*) packetAddress;
            pMessage_Header = &(pRequest_Header->message_header);
            strcpy(commandString, cl_command(pMessage_Header->command));

            if (pMessage_Header->command == COMMAND_QUERY)
            {
                pQuery_Request = (QUERY_REQUEST*) packetAddress;

                strcat(commandString, " ");
                strcat(commandString, cl_type(pQuery_Request->type));
            }
        }
        else if (logIndex == LOGI_EVENT_ACSAPI_SEND)
        {
            pRequest_Header = (REQUEST_HEADER*) packetAddress;
            pMessage_Header = &(pRequest_Header->message_header);
            strcpy(commandString, cl_command(pMessage_Header->command));

            if ((pMessage_Header->command == COMMAND_QUERY) &&
                (!(pMessage_Header->message_options & ACKNOWLEDGE)))
            {
                pQuery_Response = (QUERY_RESPONSE*) packetAddress;

                strcat(commandString, " ");
                strcat(commandString, cl_type(pQuery_Response->type));
            }
        }
        else if ((logIndex == LOGI_EVENT_XAPI_SEND) ||
                 (logIndex == LOGI_EVENT_HTTP_RECV))
        {
            pXapiIdString = strstr(packetAddress, XSTARTTAG_command);

            if (pXapiIdString != NULL)
            {
                pXapiIdString += strlen(XSTARTTAG_command);

                /*****************************************************/
                /* Test if the XAPI <command> is XML format or       */
                /* plain text.                                       */
                /*****************************************************/
                if (pXapiIdString[0] == '<')
                {
                    pXapiIdString++;
                    pNextTag = strchr(pXapiIdString, '>');
                }
                else
                {
                    pNextTag = strchr(pXapiIdString, '<');
                }

                if (pNextTag != NULL)
                {
                    commandLen = pNextTag - pXapiIdString;
                }
                else
                {
                    commandLen = packetLength - 
                                 (pXapiIdString - packetAddress);
                }

                if (commandLen > (sizeof(commandString) - 1))
                {
                    commandLen = (sizeof(commandString) - 1);
                }

                memset(commandString, 0, sizeof(commandString));
                memcpy(commandString, pXapiIdString, commandLen);
            }
        }
        else if ((logIndex == LOGI_EVENT_XAPI_RECV) ||
                 (logIndex == LOGI_EVENT_HTTP_SEND))
        {
            pXapiIdString = strstr(packetAddress, XSTARTTAG_libreply);

            if (pXapiIdString != NULL)
            {
                pXapiIdString += (strlen(XSTARTTAG_libreply) + 1);

                pNextTag = strchr(pXapiIdString, '>');

                if (pNextTag != NULL)
                {
                    commandLen = pNextTag - pXapiIdString;
                }
                else
                {
                    commandLen = packetLength - 
                                 (pXapiIdString - packetAddress);
                }

                if (commandLen > (sizeof(commandString) - 1))
                {
                    commandLen = (sizeof(commandString) - 1);
                }

                memset(commandString, 0, sizeof(commandString));
                memcpy(commandString, pXapiIdString, commandLen);
            }
        }
        else
        {
            commandString[0] = 0; 
        }
    }

    logPid = getpid();
    time(&logTime);

    strftime(timeString, 
             sizeof(timeString),
             "%m-%d-%y %H:%M:%S", 
             localtime(&logTime));

    if (getenv(CDKTYPE_VAR) != NULL)
    {
        strcpy(cdkType, getenv(CDKTYPE_VAR));
    }
    else
    {
        strcpy(cdkType, global_module_type);
    }

    memset(fixedCdkType, ' ', MAX_MODULE_TYPE_SIZE);
    fixedCdkType[MAX_MODULE_TYPE_SIZE] = 0; 

    if (strlen(cdkType) > MAX_MODULE_TYPE_SIZE)
    {
        memcpy(fixedCdkType, 
               cdkType, 
               MAX_MODULE_TYPE_SIZE);
    }
    else
    {
        memcpy(fixedCdkType, 
               cdkType, 
               strlen(cdkType));
    }

    memset(fixedFileName, ' ', MAX_FILENAME_SIZE);
    fixedFileName[MAX_FILENAME_SIZE] = 0; 

    if (strlen(fileName) > MAX_FILENAME_SIZE)
    {
        memcpy(fixedFileName, 
               fileName, 
               MAX_FILENAME_SIZE);
    }
    else
    {
        memcpy(fixedFileName, 
               fileName, 
               strlen(fileName));
    }

    if (status != STATUS_SUCCESS)
    {
        sprintf(logBuffer,
                "%s [%s:%.5d] %s\n",
                timeString,
                fixedCdkType,
                logPid,
                cl_status(((STATUS) status)));
    }
    else
    {
        sprintf(logBuffer,
                "%s [%s:%05d]\n",
                timeString,
                fixedCdkType,
                logPid);
    }

    bufferLength = strlen(logBuffer);

    /*****************************************************************/
    /* If the srvlogs log service daemon is active, then write       */
    /* the output to the SLOGPIPE (log pipe) which is the            */
    /* preferred log output method.                                  */
    /*                                                               */
    /* NOTE: We only test write RC for 1st write:  If 1st write      */
    /* succeeds, we assume remainder succeed for this log event.     */
    /*****************************************************************/
    if (global_srvlogs_active_flag)
    {
        lastRC = write(logFd, logBuffer, bufferLength);

        /*************************************************************/
        /* If error writing to the log pipe, then reset the          */
        /* global_srvlogs_active_flag, and call ourself and          */
        /* attempt to write to the SLOGFILE directly.                */
        /*************************************************************/
        if (lastRC < 0)
        {
            printf("%s[%i]: Error writing to log pipe=%s; errno=%i; srvlogs not active\n",
                   __FILE__,
                   __LINE__,
                   logVarValue,
                   errno);

            global_srvlogs_active_flag = FALSE;
            close(logFd);

            lastRC = srvlogc(fileName,
                             lineNumber,
                             status,
                             packetId,
                             packetAddress,
                             packetLength,
                             logIndex,
                             packetAddressString1,
                             packetAddressString2,
                             logMsg);

            return lastRC;
        }
    }
    /*****************************************************************/
    /* Otherwise, if the srvlogs log service daemon is NOT active,   */
    /* then write the output to the directly to the SLOGFILE (log    */
    /* file) which is much slower log output method.                 */
    /*****************************************************************/
    else
    {
        lastRC = fwrite(logBuffer, 1, bufferLength, pLogFile);

        /*************************************************************/
        /* If error writing to the log file, then reset the          */
        /* global_cdklog_flag to turn logging off altogether.        */
        /*************************************************************/
        if (lastRC < 0)
        {
            printf("%s[%i]: Error writing to log file=%s; errno=%i; log now disabled\n",
                   __FILE__,
                   __LINE__,
                   logVarValue,
                   errno);

            SETENV(CDKLOG_VAR, "0");
            global_cdklog_flag = FALSE;
            fclose(pLogFile);

            return(-1);
        }

        fflush(pLogFile);
    }

    sprintf(logBuffer,
            "[%s:%.4d] ",
            fixedFileName,
            lineNumber);

    /*****************************************************************/
    /* Append the log message to the 2nd log "header" line.          */
    /*****************************************************************/
    if ((logMsg != NULL) &&
        (logMsg[0] > 0))
    {
        strcat(logBuffer, logMsg);
    }

    bufferLength = strlen(logBuffer);

    if (logBuffer[(bufferLength - 1)] != '\n')
    {
        logBuffer[bufferLength] = '\n';
        bufferLength++;
        logBuffer[bufferLength] = 0;
        bufferLength++;
    }

    if (global_srvlogs_active_flag)
    {
        lastRC = write(logFd, logBuffer, bufferLength);
    }
    else
    {
        lastRC = fwrite(logBuffer, 1, bufferLength, pLogFile);
    }

    /*****************************************************************/
    /* If passed a packet to log, then log the packet info also.     */
    /*****************************************************************/
    if (((packetAddress != NULL) &&
         (packetLength > 0)) &&
        (logFlag[logIndex] == '1'))
    {
        if (logIndex == LOGI_EVENT_ACSAPI_SEND)
        {
            strcpy(sourceString, SLOG_TO_ACSAPI);
        }
        else if (logIndex == LOGI_EVENT_ACSAPI_RECV)
        {
            strcpy(sourceString, SLOG_FROM_ACSAPI);
        }
        else if ((logIndex == LOGI_EVENT_XAPI_SEND) ||
                 (logIndex == LOGI_EVENT_HTTP_RECV))
        {
            strcpy(sourceString, SLOG_TO_HTTP);
        }
        else if ((logIndex == LOGI_EVENT_XAPI_RECV) ||
                 (logIndex == LOGI_EVENT_HTTP_SEND))
        {
            strcpy(sourceString, SLOG_FROM_HTTP);
        }
        else if (logIndex == LOGI_EVENT_CSI_SEND)
        {
            strcpy(sourceString, SLOG_TO_CSI);
        }
        else if (logIndex == LOGI_EVENT_CSI_RECV)
        {
            strcpy(sourceString, SLOG_FROM_CSI);
        }
        else
        {
            sourceString[0] = 0;
        }

        if (sourceString[0] > 0)
        {
            sprintf(logBuffer,
                    "%s       %s\n",
                    SLOG_PACKET_SOURCE,
                    sourceString);

            bufferLength = strlen(logBuffer);

            if (global_srvlogs_active_flag)
            {
                lastRC = write(logFd, logBuffer, bufferLength);
            }
            else
            {
                lastRC = fwrite(logBuffer, 1, bufferLength, pLogFile);
            }
        }

        if (packetId != 0)
        {
            if ((logIndex == LOGI_EVENT_CSI_SEND) ||
                (logIndex == LOGI_EVENT_CSI_RECV))
            {
                sprintf(logBuffer,
                        "%s      %.5d             ",
                        SLOG_SSI_IDENTIFIER,
                        packetId);
            }
            else
            {
                sprintf(logBuffer,
                        "%s      %.5d             ",
                        SLOG_PACKET_ID,
                        packetId);
            }

            if (commandString[0] > 0)
            {
                if ((logIndex == LOGI_EVENT_XAPI_SEND) ||
                    (logIndex == LOGI_EVENT_HTTP_RECV))
                {
                    strcat(logBuffer, "request:  ");
                }
                else if ((logIndex == LOGI_EVENT_XAPI_RECV) ||
                         (logIndex == LOGI_EVENT_HTTP_SEND))
                {
                    strcat(logBuffer, "response: ");
                }
                else
                {
                    strcat(logBuffer, "command:  ");
                }

                strcat(logBuffer, commandString);
            }

            strcat(logBuffer, "\n");
            bufferLength = strlen(logBuffer);

            if (global_srvlogs_active_flag)
            {
                lastRC = write(logFd, logBuffer, bufferLength);
            }
            else
            {
                lastRC = fwrite(logBuffer, 1, bufferLength, pLogFile);
            }
        }

        /*************************************************************/
        /* Print out packet addressing infomation depending upon     */
        /* the input logIndex.                                       */
        /*************************************************************/
        if ((logIndex == LOGI_EVENT_XAPI_SEND) ||
            (logIndex == LOGI_EVENT_XAPI_RECV) ||
            (logIndex == LOGI_EVENT_HTTP_SEND) ||
            (logIndex == LOGI_EVENT_HTTP_RECV))
        {
            if ((packetAddressString1 != NULL) &&
                (packetAddressString1[0] > 0))
            {
                if ((packetAddressString2 != NULL) &&
                    (packetAddressString2[0] > 0))
                {
                    sprintf(logBuffer,
                            "%s  %-15.15s   HTTP server port:  %s\n",
                            SLOG_HTTP_SERVER_HOST,
                            packetAddressString1,
                            packetAddressString2);
                }
                else
                {
                    sprintf(logBuffer,
                            "%s  %-15.15s\n",
                            SLOG_HTTP_SERVER_HOST,
                            packetAddressString1);
                }

                bufferLength = strlen(logBuffer);

                if (global_srvlogs_active_flag)
                {
                    lastRC = write(logFd, logBuffer, bufferLength);
                }
                else
                {
                    lastRC = fwrite(logBuffer, 1, bufferLength, pLogFile);
                }
            }
        }
        else if ((logIndex == LOGI_EVENT_ACSAPI_SEND) ||
                 (logIndex == LOGI_EVENT_ACSAPI_RECV))
        {
            if ((packetAddressString1 != NULL) &&
                (packetAddressString1[0] > 0))
            {
                if ((packetAddressString2 != NULL) &&
                    (packetAddressString2[0] > 0))
                {
                    sprintf(logBuffer,
                            "%s  %-15.15s   SSI return pid:    %s\n",
                            SLOG_SSI_RETURN_SOCKET,
                            packetAddressString1,
                            packetAddressString2);
                }
                else
                {
                    sprintf(logBuffer,
                            "%s  %-15.15s\n",
                            SLOG_SSI_RETURN_SOCKET,
                            packetAddressString1);
                }

                bufferLength = strlen(logBuffer);

                if (global_srvlogs_active_flag)
                {
                    lastRC = write(logFd, logBuffer, bufferLength);
                }
                else
                {
                    lastRC = fwrite(logBuffer, 1, bufferLength, pLogFile);
                }
            }
        }
        else if ((logIndex == LOGI_EVENT_CSI_SEND) ||
                 (logIndex == LOGI_EVENT_CSI_RECV))
        {
            if ((packetAddressString1 != NULL) &&
                (packetAddressString1[0] > 0))
            {
                if ((packetAddressString2 != NULL) &&
                    (packetAddressString2[0] > 0))
                {
                    sprintf(logBuffer,
                            "%s  %-15.15s   SSI client port:   %s\n",
                            SLOG_SSI_CLIENT_ADDRESS,
                            packetAddressString1,
                            packetAddressString2);
                }
                else
                {
                    sprintf(logBuffer,
                            "%s  %-15.15s\n",
                            SLOG_SSI_CLIENT_ADDRESS,
                            packetAddressString1);
                }

                bufferLength = strlen(logBuffer);

                if (global_srvlogs_active_flag)
                {
                    lastRC = write(logFd, logBuffer, bufferLength);
                }
                else
                {
                    lastRC = fwrite(logBuffer, 1, bufferLength, pLogFile);
                }
            }
        }

        lastRC = srvlogc_packet(logFd,
                                pLogFile,
                                logIndex,
                                packetAddress,
                                packetLength);
    }

    /*****************************************************************/
    /* Add a blank line to separate log entries.                     */
    /*****************************************************************/
    strcpy(logBuffer, "\n");

    if (global_srvlogs_active_flag)
    {
        lastRC = write(logFd, logBuffer, strlen(logBuffer));
        close(logFd);
    }
    else
    {
        lastRC = fwrite(logBuffer, 1, strlen(logBuffer), pLogFile);
        fclose(pLogFile);
    }

    return(0);
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: srvlogc_packet                                    */
/** Description:   Log service client packet formatter.              */
/**                                                                  */
/** This function:                                                   */
/** (1) Outputs the buffer for the specified length in either plain  */
/**     text or translated character hexadecimal depending upon the  */
/**     value of the logIndex.                                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "srvlogc_packet"

static int srvlogc_packet(int   logFd,
                          FILE *pLogFile,
                          int   logIndex,
                          char *packetAddress,
                          int   packetLength)
{
    int                 lastRC; 
    int                 bytesRemaining;
    int                 bytesCurrent;
    char               *pCurrentPosition;
    int                 offset;
    int                 i;
    int                 target;
    int                 wordAdjust;
    unsigned int        hexnumber;

    char                offsetBuffer[10];
    char                hexBuffer[((MAX_STORAGE_PER_LINE * 2) + 
                                   (MAX_STORAGE_PER_LINE / 4) + 1)];
    char                charBuffer[MAX_XML_PER_LINE];
    char                conversionBuffer[4];
    char                outputBuffer[SLOG_MAX_LINE_SIZE + 1];
    char                logBuffer[SLOG_MAX_LINE_SIZE + 1];
    char                xmlScale[103]       ={
        "|....+...10....+...20....+...30....+...40....+...50"
        "....+...60....+...70....+...80....+...90....+..100|"
    };

    /*****************************************************************/
    /* If the packet is XML, then simply display the                 */
    /* packet, 100 characters to a line with minimal conversion.     */
    /*****************************************************************/
    if ((logIndex == LOGI_EVENT_XAPI_SEND) ||
        (logIndex == LOGI_EVENT_XAPI_RECV) ||
        (logIndex == LOGI_EVENT_HTTP_SEND) ||
        (logIndex == LOGI_EVENT_HTTP_RECV))
    {
        /*************************************************************/
        /* Format the log packet header message.                     */
        /* Write to either the log pipe or the log file              */
        /* depending upon whether we were passed a file              */
        /* descriptor (pipe) or file pointer (file).                 */
        /*************************************************************/
        sprintf(outputBuffer, 
                "XML     %s\n",
                xmlScale);

        if (pLogFile == NULL)
        {
            lastRC = write(logFd, outputBuffer, strlen(outputBuffer));
        }
        else
        {
            lastRC = fwrite(outputBuffer, 1, strlen(outputBuffer), pLogFile);
        }

        /*************************************************************/
        /* Now setup for character-hex conversion in 10 byte         */
        /* increments.                                               */
        /*************************************************************/
        bytesRemaining = packetLength;
        bytesCurrent = 0;
        pCurrentPosition = packetAddress;
        offset = 0 - MAX_XML_PER_LINE;

        while (bytesRemaining > 0)
        {
            memset(outputBuffer, 
                   ' ', 
                   sizeof(outputBuffer));

            memset(offsetBuffer, 
                   ' ', 
                   sizeof(offsetBuffer));

            memset(charBuffer, 
                   ' ', 
                   sizeof(charBuffer));

            if (bytesRemaining < MAX_XML_PER_LINE)
            {
                bytesCurrent = bytesRemaining;
                bytesRemaining = 0;

                memcpy(charBuffer, 
                       pCurrentPosition, 
                       bytesCurrent);

                pCurrentPosition = pCurrentPosition + bytesCurrent;
            }
            else
            {
                bytesCurrent = MAX_XML_PER_LINE;
                bytesRemaining -= MAX_XML_PER_LINE;

                memcpy(charBuffer, 
                       pCurrentPosition,
                       bytesCurrent);

                pCurrentPosition += MAX_XML_PER_LINE;
            }

            offset += MAX_XML_PER_LINE;

            sprintf(offsetBuffer, 
                    "%7.7d: ", 
                    offset);

            /*********************************************************/
            /* The only conversion is the convert any non-display    */
            /* characters into an "." character.                     */
            /*********************************************************/
            for (i = 0; 
                i < bytesCurrent; 
                i++)
            {
                if (!(isprint(charBuffer[i])))
                {
                    charBuffer[i] = '.';
                }
            }

            memcpy(&outputBuffer[0], 
                   offsetBuffer, 
                   8);

            memcpy(&outputBuffer[9], 
                   charBuffer,
                   100);

            outputBuffer[110] = 0;

            sprintf(logBuffer,
                    "%s\n",
                    outputBuffer);

            if (pLogFile == NULL)
            {
                lastRC = write(logFd, logBuffer, strlen(logBuffer));
            }
            else
            {
                lastRC = fwrite(logBuffer, 1, strlen(logBuffer), pLogFile);
            }
        }
    }
    /*****************************************************************/
    /* Otherwise if the packet is an ACSAPI or CSI packet,           */
    /* then do the character-hex thing                               */
    /*****************************************************************/
    else
    {
        /*************************************************************/
        /* Write a message describing what storage is being logged.  */
        /*************************************************************/
        sprintf(logBuffer, 
                "Packet contents (%d hex bytes):\n",
                packetLength);

        if (pLogFile == NULL)
        {
            lastRC = write(logFd, logBuffer, strlen(logBuffer));
        }
        else
        {
            lastRC = fwrite(logBuffer, 1, strlen(logBuffer), pLogFile);
        }

        /*************************************************************/
        /* Now setup for character-hex conversion in 16 byte         */
        /* increments.                                               */
        /*************************************************************/
        bytesRemaining = packetLength;
        bytesCurrent = 0;
        pCurrentPosition = packetAddress;
        offset = 0 - MAX_STORAGE_PER_LINE;

        while (bytesRemaining > 0)
        {
            memset(outputBuffer, 
                   ' ', 
                   sizeof(outputBuffer));

            memset(offsetBuffer, 
                   ' ', 
                   sizeof(offsetBuffer));

            memset(hexBuffer, 
                   ' ', 
                   sizeof(hexBuffer));

            memset(charBuffer, 
                   ' ', 
                   sizeof(charBuffer));

            if (bytesRemaining < MAX_STORAGE_PER_LINE)
            {
                bytesCurrent = bytesRemaining;
                bytesRemaining = 0;

                memcpy(charBuffer, 
                       pCurrentPosition, 
                       bytesCurrent);

                pCurrentPosition = pCurrentPosition + bytesCurrent;
            }
            else
            {
                bytesCurrent = MAX_STORAGE_PER_LINE;
                bytesRemaining -= MAX_STORAGE_PER_LINE;

                memcpy(charBuffer, 
                       pCurrentPosition,
                       bytesCurrent);

                pCurrentPosition += MAX_STORAGE_PER_LINE;
            }

            offset += MAX_STORAGE_PER_LINE;

            sprintf(offsetBuffer, 
                    "%4.4d: ", 
                    offset);

            for (i = 0; 
                i < bytesCurrent; 
                i++)
            {
                hexnumber = (unsigned char) charBuffer[i];

                sprintf(conversionBuffer, 
                        "%2.2X", 
                        (unsigned int) hexnumber);

                wordAdjust = (i / 4);
                target = (i * 2) + wordAdjust;

                hexBuffer[target] = conversionBuffer[0];
                hexBuffer[(target + 1)] = conversionBuffer[1];
            }

            for (i = 0; 
                i < bytesCurrent; 
                i++)
            {
                if (!(isprint(charBuffer[i])))
                {
                    charBuffer[i] = '.';
                }
            }

            memcpy(&outputBuffer[0], 
                   offsetBuffer, 
                   5);

            memcpy(&outputBuffer[7], 
                   hexBuffer, 
                   72);

            outputBuffer[79] = '|';

            memcpy(&outputBuffer[80], 
                   charBuffer, 
                   MAX_STORAGE_PER_LINE);

            outputBuffer[112] = '|';
            outputBuffer[114] = '\n';
            outputBuffer[115] = '\x00';

            if (pLogFile == NULL)
            {
                lastRC = write(logFd, outputBuffer, strlen(outputBuffer));
            }
            else
            {
                lastRC = fwrite(outputBuffer, 1, strlen(outputBuffer), pLogFile);
            }
        }
    }

    return(0);
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: srvlogc_string_writer                             */
/** Description:   Log service client string writer                  */
/**                                                                  */
/** This function:                                                   */
/** (1) Tests if the $CDKLOG environment variable is set.            */
/**     If not set to "1", then return;                              */
/** (2) Gets the $SLOGPIPE variable for the log pipe name;           */
/** (3) Opens the log pipe for output and writes the log buffer;     */
/** (4) Closes the log pipe.                                         */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "srvlogc_string_writer"

extern void srvlogc_string_writer(const char *logMsg)
{
    int                 lastRC;
    int                 logFd               = 0;
    FILE               *pLogFile            = NULL;
    int                 msgLength;

    char                logVarValue[256];
    char                logFlag[MAX_CDKLOG_TYPE + 1];
    char                msgBuffer[SLOG_MAX_LINE_SIZE + 1];

    /*****************************************************************/
    /* Test if CDKLOG variable is set.                               */
    /* If set, but not "1", then return now.                         */
    /* If not set, then set it now to "0".                           */
    /*****************************************************************/
    memset(logFlag, 0, sizeof(logFlag));

    if (getenv(CDKLOG_VAR) != NULL)
    {
        strncpy(logFlag, getenv(CDKLOG_VAR), MAX_CDKLOG_TYPE);

        if (logFlag[0] != '1')
        {
            return;
        }
    }
    else
    {
        SETENV(CDKLOG_VAR, "0");

        return;
    }

    /*****************************************************************/
    /* If the srvlogs log service daemon is active, then write       */
    /* the output to the SLOGPIPE (log pipe) which is the            */
    /* preferred log output method.                                  */
    /*****************************************************************/
    if (global_srvlogs_active_flag)
    {
        if (getenv(SLOGPIPE_VAR) != NULL)
        {
            strcpy(logVarValue, getenv(SLOGPIPE_VAR));
        }
        else
        {
            strcpy(logVarValue, DEFAULT_SLOGPIPE);
        }

        logFd = open(logVarValue, (O_WRONLY | O_NONBLOCK | O_SYNC));

        /*************************************************************/
        /* If error opening the log pipe, then reset the             */
        /* global_srvlogs_active_flag, and call ourself and          */
        /* attempt to write to the SLOGFILE directly.                */
        /*************************************************************/
        if (logFd == 0)
        {
            printf("%s[%i]: Error opening log pipe=%s; errno=%i; srvlogs not active\n",
                   __FILE__,
                   __LINE__,
                   logFlag,
                   errno);

            global_srvlogs_active_flag = FALSE;
            srvlogc_string_writer(logMsg);    

            return;
        }

        lastRC = write(logFd, logMsg, strlen(logMsg));

        /*************************************************************/
        /* If error writing to the log pipe, then reset the          */
        /* global_srvlogs_active_flag, and call ourself and          */
        /* attempt to write to the SLOGFILE directly.                */
        /*************************************************************/
        if (lastRC < 0)
        {
            printf("%s[%i]: Error writing to log pipe=%s; errno=%i; srvlogs not active\n",
                   __FILE__,
                   __LINE__,
                   logFlag,
                   errno);

            global_srvlogs_active_flag = FALSE;
            close(logFd);
            srvlogc_string_writer(logMsg);    

            return;
        }
    }
    /*****************************************************************/
    /* Otherwise, if the srvlogs log service daemon is NOT active, */
    /* then write the output to the directly to the SLOGFILE (log  */
    /* file) which is much slower log output method.               */
    /*****************************************************************/
    else
    {
        if (getenv(SLOGFILE_VAR) != NULL)
        {
            strcpy(logVarValue, getenv(SLOGFILE_VAR));
        }
        else
        {
            strcpy(logVarValue, DEFAULT_SLOGFILE);
        }

        pLogFile = fopen(logVarValue, "ab");

        /*************************************************************/
        /* If error opening the log file, then reset the             */
        /* global_cdklog_flag to turn logging off altogether.        */
        /*************************************************************/
        if (pLogFile == NULL)
        {
            printf("%s[%i]: Error opening log file=%s; errno=%i; log now disabled\n",
                   __FILE__,
                   __LINE__,
                   logFlag,
                   errno);

            SETENV(CDKLOG_VAR, "0");
            global_cdklog_flag = FALSE;

            return;
        }

        lastRC = fwrite(logMsg, 1, strlen(logMsg), pLogFile);

        /*************************************************************/
        /* If error writing to the log file, then reset the          */
        /* global_cdklog_flag to turn logging off altogether.        */
        /*************************************************************/
        if (lastRC < 0)
        {
            printf("%s[%i]: Error writing to log file=%s; errno=%i; log now disabled\n",
                   __FILE__,
                   __LINE__,
                   logFlag,
                   errno);

            SETENV(CDKLOG_VAR, "0");
            global_cdklog_flag = FALSE;
            fclose(pLogFile);

            return;
        }

        fflush(pLogFile);
    }

    if (global_srvlogs_active_flag)
    {
        close(logFd);
    }
    else
    {
        fclose(pLogFile);
    }

    return;
}





