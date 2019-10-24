/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      srvtrcc.c                                        */
/** Description:    The trace service client common component.       */
/**                                                                  */
/**                 The trace client normally writes output to the   */
/**                 trace.pipe.  However, it may also write          */
/**                 directly to the trace.file if the srvtrcs        */
/**                 service daemon is not active.                    */
/**                                                                  */
/**                 There are 3 environmental variables to           */
/**                 control the trace services implemented using     */
/**                 the TR* macros in src/h/srvcommon.h.             */
/**                                                                  */
/**                 STRCPIPE=pipe.name to specify the name of the    */
/**                 trace pipe that trace clients write to using     */
/**                 srvcommon.h TR* macros (or the srvtrcc           */
/**                 function).  STRCPIPE is also used by the         */
/**                 trace server (srvtrcs) to specify the pipe that  */
/**                 will be read to create the final trace file.     */
/**                                                                  */
/**                 STRCFILE=file.name to specify the name of trace  */
/**                 file that the trace server (srvtrcs) will write  */
/**                 the final trace file.                            */  
/**                                                                  */
/**                 CDKTRACE={1|0} To turn on tracing.               */
/**                 The CDKTRACE variable can be up to               */
/**                 MAX_CDKTRACE_TYPE characters long.               */
/**                 Each character is evaluated as if it were        */
/**                 a bit (0 or 1) (i.e. CDKTRACE=011011100).        */
/**                 If the specified character "bit" in the          */
/**                 CDKTRACE variable  is on (set to "1"),           */
/**                 the corresponding trace records are written.     */
/**                 If the specified character "bit" in the          */
/**                 CDKTRACE variable is not on (set to              */
/**                 something other than "1"), the corresponding     */
/**                 trace records are not written.                   */
/**                                                                  */
/**                 NOTE that "1" and only "1" turns on the          */
/**                 trace for a specific component or event.         */
/**                 See srvcommon.h for the list of components       */
/**                 and events in the CDKTRACE variable.             */
/**                                                                  */
/**                 There are 3 additional global variables used     */
/**                 to fast pass certain decisions within the        */
/**                 trace services implemented by src/h/srvcommon.h. */
/**                                                                  */
/**                 global_module_type is set by srvvars to set      */
/**                 the process type ("CSI", "SSI", "XAPI",          */
/**                 "HTTP", etc).  srvvars also sets the             */
/**                 environment variable CDKTYPE to this value.      */
/**                                                                  */
/**                 global_cdktrace_flag is set by srvvars           */
/**                 depending upon the CDKTRACE variable.            */
/**                                                                  */
/**                 global_srvtrcs_active_flag is always set by      */
/**                 srvvars as it assumes the srvtrcs daemon         */
/**                 is active.  However it will be reset by this     */
/**                 function if output to the trace pipe results     */
/**                 in an error.                                     */
/**                                                                  */
/** Special Considerations:                                          */
/**                 Do not attempt to use any TRMSG or TRMEM         */
/**                 facilities from this module as it will cause     */
/**                 a recursive loop.                                */
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
#include "srvcommon.h"


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#define MAX_STORAGE_SIZE           4096
#define MAX_STORAGE_PER_LINE       16
#define MAX_STORAGE_LINES          MAX_STORAGE_SIZE / MAX_STORAGE_PER_LINE
#define MAX_FILENAME_SIZE          16
#define MAX_WRITE_RETRY            5


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int srvtrcc_memory(int   traceFd,
                          FILE *pTraceFile,
                          char *traceVarValue,
                          char *tracePrefix,
                          char *storageAddress,
                          int   storageLength);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: srvtrcc                                           */
/** Description:   Trace service client                              */
/**                                                                  */
/** This function:                                                   */
/** (1) Tests if the $CDKTRACE environment variable is set.          */
/**     If not set to "1", then return;                              */
/** (2) Creates the trace message prefix;                            */
/** (3) Creates the trace buffer by appending the input trace        */
/**     message to the message prefix;                               */
/** (4) Gets the $STRCPIPE variable for the trace pipe name.         */
/** (5) Opens the trace pipe for output and writes the trace buffer; */
/** (6) If passed a storage address, then calls srvtrcc_memory       */
/**     to format and output the storage block;                      */
/** (7) Closes the trace pipe.                                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "srvtrcc"

extern int srvtrcc(char *fileName,
                   char *funcName,
                   int   lineNumber,
                   int   traceIndex,
                   char *storageAddress,
                   int   storageLength,
                   char *traceMsg)
{
    int                 cdktraceIndex       = traceIndex;
    int                 lastRC; 
    int                 retryCount;
    int                 tracePid;
    int                 traceFd             = 0;
    FILE               *pTraceFile          = NULL;
    int                 bufferLength;

    time_t              traceTime;
    char               *pDotc;

    char                traceBuffer[STRC_MAX_LINE_SIZE * 2];
    char                tracePrefix[STRC_MAX_LINE_SIZE + 1];
    char                traceVarValue[256];
    char                traceFlag[MAX_CDKTRACE_TYPE + 1];
    char                timeString[80];
    char                cdkType[MAX_MODULE_TYPE_SIZE + 1] = "";
    char                fixedFileName[MAX_FILENAME_SIZE + 1];
    char                fixedCdkType[MAX_MODULE_TYPE_SIZE + 1];

    /*****************************************************************/
    /* Get value of CDKTYPE environment variable, and derive         */
    /* the traceIndex value, if appropriate.                         */
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

    if (cdktraceIndex == TRCI_CDKTYPE)
    {
        if (strcmp(cdkType, "CSI") == 0)
        {
            cdktraceIndex = TRCI_CSI;
        }
        else if (strcmp(cdkType, "SSI") == 0)
        {
            cdktraceIndex = TRCI_SSI;
        }
        else if (strcmp(cdkType, "XAPI") == 0)
        {
            cdktraceIndex = TRCI_XAPI;
        }
        else if (strcmp(cdkType, "CLIENT") == 0)
        {
            cdktraceIndex = TRCI_ACSAPI;
        }
        else if ((strcmp(cdkType, "HTTP") == 0) ||
                 (strcmp(cdkType, "ACSLM") == 0))
        {
            cdktraceIndex = TRCI_SERVER;
        }
        else
        {
            cdktraceIndex = TRCI_DEFAULT;
        }
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
    printf("traceIndex=%d, cdktraceIndex=%d, cdkType=%s\n", 
           traceIndex, 
           cdktraceIndex, 
           cdkType);
*/

    /*****************************************************************/
    /* Test if the appropriate CDKTRACE variable is set.             */
    /*                                                               */
    /* The CDKTRACE variable can be up to MAX_CDKTRACE_TYPE          */
    /* characters long.  Each character is evaluated as if it were   */
    /* a bit (0 or 1).                                               */
    /*                                                               */
    /* If the specified input traceIndex in the CDKTRACE variable    */
    /* is on (set to "1"), the trace record is written.              */
    /* If the specified input traceIndex in the CDKTRACE variable    */
    /* is something other than "1", then the trace record is not     */
    /* written.                                                      */
    /*                                                               */
    /* If the CDKTRACE variable is not set at all, then we           */
    /* set it now to "0" (effectively turning all tracing OFF).      */
    /*****************************************************************/
    if (cdktraceIndex > (MAX_CDKTRACE_TYPE - 1))
    {
        return 0;
    }

    memset(traceFlag, 0, sizeof(traceFlag));

    if (getenv(CDKTRACE_VAR) != NULL)
    {
        strncpy(traceFlag, getenv(CDKTRACE_VAR), MAX_CDKTRACE_TYPE);

        if (traceFlag[cdktraceIndex] != '1')
        {
            return(0);
        }
    }
    else
    {
        SETENV(CDKTRACE_VAR, "0");

        return(0);
    }

    /*****************************************************************/
    /* Format the trace message "prefix".                            */
    /*****************************************************************/
    tracePid = getpid();
    time(&traceTime);

    strftime(timeString, 
             sizeof(timeString),
             "%m-%d-%y %H:%M:%S", 
             localtime(&traceTime));

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

    /*****************************************************************/
    /* The trace prefix is fixed length up to the file line number.  */
    /* If the funcName is the same as the fileName, then just        */
    /* display the fileName in the prefix; otherwise display         */
    /* both the fileName and the funcName in the prefix.             */
    /*****************************************************************/
    if (memcmp(fileName, 
               funcName,
               (strlen(fileName) - 2)) == 0)
    {
        sprintf(tracePrefix,
                "%.5d.%.2d %s %s [%s:%.4d]",
                tracePid,
                cdktraceIndex,
                fixedCdkType,
                timeString, 
                fixedFileName,
                lineNumber); 
    }
    else
    {
        sprintf(tracePrefix,
                "%.5d.%.2d %s %s [%s:%.4d].[%s]",
                tracePid,
                cdktraceIndex,
                fixedCdkType,
                timeString, 
                fixedFileName,
                lineNumber,
                funcName); 
    }

    sprintf(traceBuffer,
            "%s %s",
            tracePrefix,
            traceMsg);

    bufferLength = strlen(traceBuffer);

    if (traceBuffer[(bufferLength - 1)] != '\n')
    {
        traceBuffer[bufferLength] = '\n';
        bufferLength++;
    }

    /*****************************************************************/
    /* If the srvtrcs trace service daemon is active, then write     */
    /* the output to the STRCPIPE (trace pipe) which is the          */
    /* preferred trace output method.                                */
    /*****************************************************************/
    if (global_srvtrcs_active_flag)
    {
        if (getenv(STRCPIPE_VAR) != NULL)
        {
            strcpy(traceVarValue, getenv(STRCPIPE_VAR));
        }
        else
        {
            strcpy(traceVarValue, DEFAULT_STRCPIPE);
        }

        traceFd = open(traceVarValue, (O_WRONLY | O_NONBLOCK | O_SYNC));

        /*************************************************************/
        /* If error opening the trace pipe, then reset the           */
        /* global_srvtrcs_active_flag, and call ourself and          */
        /* attempt to write to the STRCFILE directly.                */
        /*************************************************************/
        if (traceFd == 0)
        {
            printf("%s[%i]: Error opening trace pipe=%s; errno=%i; srvtrcs not active\n",
                   __FILE__,
                   __LINE__,
                   traceVarValue,
                   errno);

            global_srvtrcs_active_flag = FALSE;

            lastRC = srvtrcc(fileName,
                             funcName,
                             lineNumber,
                             traceIndex,
                             storageAddress,
                             storageLength,
                             traceMsg);

            return lastRC;
        }

        /*************************************************************/
        /* If EAGAIN error writing to the trace pipe, then retry     */
        /* up to MAX_WRITE_RETRY times.                              */
        /*                                                           */
        /* Otherwise if any other write error  then reset the        */
        /* global_srvtrcs_active_flag, and call ourself to           */
        /* attempt to write to the STRCFILE directly.                */
        /*************************************************************/
        for (retryCount = 0;
            retryCount < MAX_WRITE_RETRY;
            retryCount++)
        {
            lastRC = write(traceFd, traceBuffer, bufferLength);

            if ((lastRC < 0) &&
                (errno == EAGAIN))
            {
                sleep(1);

                continue;
            }
            else
            {
                break;
            }
        }

        if (lastRC < 0)
        {
            printf("%s[%i]: Error writing to trace pipe=%s; errno=%i; srvtrcs not active\n",
                   __FILE__,
                   __LINE__,
                   traceVarValue,
                   errno);

            global_srvtrcs_active_flag = FALSE;
            close(traceFd);

            lastRC = srvtrcc(fileName,
                             funcName,
                             lineNumber,
                             traceIndex,
                             storageAddress,
                             storageLength,
                             traceMsg);

            return lastRC;
        }
    }
    /*****************************************************************/
    /* Otherwise, if the srvtrcs trace service daemon is NOT active, */
    /* then write the output to the directly to the STRCFILE (trace  */
    /* file) which is much slower trace output method.               */
    /*****************************************************************/
    else
    {
        if (getenv(STRCFILE_VAR) != NULL)
        {
            strcpy(traceVarValue, getenv(STRCFILE_VAR));
        }
        else
        {
            strcpy(traceVarValue, DEFAULT_STRCFILE);
        }

        pTraceFile = fopen(traceVarValue, "ab");

        /*************************************************************/
        /* If error opening the trace file, then reset the           */
        /* global_cdktrace_flag to turn tracing off altogether.      */
        /*************************************************************/
        if (pTraceFile == NULL)
        {
            printf("%s[%i]: Error opening trace file=%s; errno=%i; trace now disabled\n",
                   __FILE__,
                   __LINE__,
                   traceVarValue,
                   errno);

            SETENV(CDKTRACE_VAR, "0");
            global_cdktrace_flag = FALSE;

            return(-1);
        }

        lastRC = fwrite(traceBuffer, 1, bufferLength, pTraceFile);

        /*************************************************************/
        /* If error writing to the trace file, then reset the        */
        /* global_cdktrace_flag to turn tracing off altogether.      */
        /*************************************************************/
        if (lastRC < 0)
        {
            printf("%s[%i]: Error writing to trace file=%s; errno=%i; trace now disabled\n",
                   __FILE__,
                   __LINE__,
                   traceVarValue,
                   errno);

            SETENV(CDKTRACE_VAR, "0");
            global_cdktrace_flag = FALSE;
            fclose(pTraceFile);

            return(-1);
        }

        fflush(pTraceFile);
    }

    /*****************************************************************/
    /* If entered via a TRMEM, then trace the storage block also.    */
    /*****************************************************************/
    if ((storageAddress != NULL) &&
        (storageLength > 0))
    {
        lastRC = srvtrcc_memory(traceFd,
                                pTraceFile,
                                traceVarValue,
                                tracePrefix,
                                storageAddress,
                                storageLength);
    }

    if (global_srvtrcs_active_flag)
    {
        close(traceFd);
    }
    else
    {
        fclose(pTraceFile);
    }

    return(0);
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: srvtrcc_memory                                    */
/** Description:   Trace service client memory formatter.            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "srvtrcc_memory"

static int srvtrcc_memory(int   traceFd,
                          FILE *pTraceFile,
                          char *traceVarValue,
                          char *tracePrefix,
                          char *storageAddress,
                          int   storageLength)
{
    int                 lastRC; 
    int                 retryCount;
    int                 bytesRemaining;
    int                 bytesCurrent;
    char               *pCurrentPosition;
    int                 offset;
    int                 i;
    int                 wordAdjust;
    int                 target;
    unsigned int        hexnumber;

    char                addressBuffer[12];
    char                offsetBuffer[10];
    char                hexBuffer[((MAX_STORAGE_PER_LINE * 2) + 
                                   (MAX_STORAGE_PER_LINE / 4) + 1)];
    char                charBuffer[MAX_STORAGE_PER_LINE];
    char                conversionBuffer[4];
    char                outputBuffer[STRC_MAX_LINE_SIZE * 2];
    char                traceBuffer[STRC_MAX_LINE_SIZE * 2];

    /*****************************************************************/
    /* Write a message describing what storage is being dumped.      */
    /*****************************************************************/
    sprintf(traceBuffer,
            "%s Storage at address=%08X for %d bytes:\n",
            tracePrefix,
            storageAddress,
            storageLength);

    /*****************************************************************/
    /* Write to either the trace pipe or the trace file depending    */
    /* upon whether we were passed a file descriptor (pipe) or       */
    /* file pointer (file).                                          */
    /*                                                               */
    /* If EAGAIN error writing to the trace pipe, then retry         */
    /* up to MAX_WRITE_RETRY times.                                  */
    /*****************************************************************/
    if (pTraceFile == NULL)
    {
        for (retryCount = 0;
            retryCount < MAX_WRITE_RETRY;
            retryCount++)
        {
            lastRC = write(traceFd, traceBuffer, strlen(traceBuffer));

            if ((lastRC < 0) &&
                (errno == EAGAIN))
            {
                sleep(1);

                continue;
            }
            else
            {
                break;
            }
        }

        if (lastRC < 0)
        {
            printf("%s[%i]: Error writing to trace pipe=%s; errno=%i; trace line lost\n",
                   __FILE__,
                   __LINE__,
                   traceVarValue,
                   errno);
        }
    }
    else
    {
        lastRC = fwrite(traceBuffer, 1, strlen(traceBuffer), pTraceFile);
        fflush(pTraceFile);
    }

    /*****************************************************************/
    /* Now setup for character-hex conversion in 16 byte increments. */
    /*****************************************************************/
    bytesRemaining = storageLength;
    bytesCurrent = 0;
    pCurrentPosition = storageAddress;
    offset = 0 - MAX_STORAGE_PER_LINE;

    while (bytesRemaining > 0)
    {
        memset(outputBuffer, 
               ' ', 
               sizeof(outputBuffer));

        memset(addressBuffer, 
               ' ', 
               sizeof(addressBuffer));

        memset(offsetBuffer, 
               ' ', 
               sizeof(offsetBuffer));

        memset(hexBuffer, 
               ' ', 
               sizeof(hexBuffer));

        memset(charBuffer, 
               ' ', 
               sizeof(charBuffer));

        sprintf(addressBuffer, 
                "%8.8X|  ", 
                pCurrentPosition);

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
                "%08X ", 
                offset);

        for (i = 0; 
            i < bytesCurrent; 
            i++)
        {
            hexnumber = (unsigned char) charBuffer[i];

            sprintf(conversionBuffer, 
                    "%2.2X", 
                    (unsigned int)hexnumber);

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
               addressBuffer, 
               8);

        outputBuffer[9] = '+';

        memcpy(&outputBuffer[10], 
               &offsetBuffer[4], 
               4);

        outputBuffer[14] = '|';

        memcpy(&outputBuffer[16], 
               hexBuffer, 
               36);

        outputBuffer[52] = '|';

        memcpy(&outputBuffer[53], 
               charBuffer, 
               MAX_STORAGE_PER_LINE);

        outputBuffer[69] = '|';
        outputBuffer[78] = '\n';
        outputBuffer[79] = '\x00';

        sprintf(traceBuffer,
                "%s %s",
                tracePrefix,
                outputBuffer);

        /*************************************************************/
        /* Write to either the trace pipe or trace file.             */
        /*                                                           */
        /* If EAGAIN error writing to the trace pipe, then retry     */
        /* up to MAX_WRITE_RETRY times.                              */
        /*************************************************************/
        if (pTraceFile == NULL)
        {
            for (retryCount = 0;
                retryCount < MAX_WRITE_RETRY;
                retryCount++)
            {
                lastRC = write(traceFd, traceBuffer, strlen(traceBuffer));

                if ((lastRC < 0) &&
                    (errno == EAGAIN))
                {
                    sleep(1);

                    continue;
                }
                else
                {
                    break;
                }
            }

            if (lastRC < 0)
            {
                printf("%s[%i]: Error writing to trace pipe=%s; errno=%i; trace line lost\n",
                       __FILE__,
                       __LINE__,
                       traceVarValue,
                       errno);
            }
        }
        else
        {
            lastRC = fwrite(traceBuffer, 1, strlen(traceBuffer), pTraceFile);
            fflush(pTraceFile);
        }
    }

    return(0);
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: srvtrcc_string_writer                             */
/** Description:   Trace service client string writer.               */
/**                                                                  */
/** This function:                                                   */
/** (1) Tests if the $CDKTRACE environment variable is set.          */
/**     If not set to "0", then return;                              */
/** (2) Gets the $STRCPIPE variable for the trace pipe name;         */
/** (3) Opens the trace pipe for output and writes the trace buffer; */
/** (4) Closes the trace pipe.                                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "srvtrcc_string_writer"

extern void srvtrcc_string_writer(const char *traceMsg)
{
    int                 lastRC;
    int                 traceFd             = 0;
    FILE               *pTraceFile          = NULL;

    char                traceVarValue[256];
    char                traceFlag[MAX_CDKTRACE_TYPE + 1];

    /*****************************************************************/
    /* Test if CDKTRACE variable is set.                             */
    /* If set, but not "1", then return now.                         */
    /* If not set, then set it now to "0".                           */
    /*****************************************************************/
    memset(traceFlag, 0, sizeof(traceFlag));

    if (getenv(CDKTRACE_VAR) != NULL)
    {
        strncpy(traceFlag, getenv(CDKTRACE_VAR), MAX_CDKTRACE_TYPE);

        if (traceFlag[0] != '1')
        {
            return;
        }
    }
    else
    {
        SETENV(CDKTRACE_VAR, "0");

        return;
    }

    /*****************************************************************/
    /* If the srvtrcs trace service daemon is active, then write     */
    /* the output to the STRCPIPE (trace pipe) which is the          */
    /* preferred trace output method.                                */
    /*****************************************************************/
    if (global_srvtrcs_active_flag)
    {
        if (getenv(STRCPIPE_VAR) != NULL)
        {
            strcpy(traceVarValue, getenv(STRCPIPE_VAR));
        }
        else
        {
            strcpy(traceVarValue, DEFAULT_STRCPIPE);
        }

        traceFd = open(traceVarValue, (O_WRONLY | O_NONBLOCK | O_SYNC));

        /*************************************************************/
        /* If error opening the trace pipe, then reset the           */
        /* global_srvtrcs_active_flag, and call ourself and          */
        /* attempt to write to the STRCFILE directly.                */
        /*************************************************************/
        if (traceFd == 0)
        {
            printf("%s[%i]: Error opening trace pipe=%s; errno=%i; srvtrcs not active\n",
                   __FILE__,
                   __LINE__,
                   traceVarValue,
                   errno);

            global_srvtrcs_active_flag = FALSE;
            srvtrcc_string_writer(traceMsg);

            return;
        }

        lastRC = write(traceFd, traceMsg, strlen(traceMsg));

        /*************************************************************/
        /* If error writing to the trace pipe, then reset the        */
        /* global_srvtrcs_active_flag, and call ourself and          */
        /* attempt to write to the STRCFILE directly.                */
        /*************************************************************/
        if (lastRC < 0)
        {
            printf("%s[%i]: Error writing to trace pipe=%s; errno=%i; srvtrcs not active\n",
                   __FILE__,
                   __LINE__,
                   traceVarValue,
                   errno);

            global_srvtrcs_active_flag = FALSE;
            close(traceFd);
            srvtrcc_string_writer(traceMsg);

            return;
        }
    }
    /*****************************************************************/
    /* Otherwise, if the srvtrcs trace service daemon is NOT active, */
    /* then write the output to the directly to the STRCFILE (trace  */
    /* file) which is much slower trace output method.               */
    /*****************************************************************/
    else
    {
        if (getenv(STRCFILE_VAR) != NULL)
        {
            strcpy(traceVarValue, getenv(STRCFILE_VAR));
        }
        else
        {
            strcpy(traceVarValue, DEFAULT_STRCFILE);
        }

        pTraceFile = fopen(traceVarValue, "ab");

        /*************************************************************/
        /* If error opening the trace file, then reset the           */
        /* global_cdktrace_flag to turn tracing off altogether.      */
        /*************************************************************/
        if (pTraceFile == NULL)
        {
            printf("%s[%i]: Error opening trace file=%s; errno=%i; trace now disabled\n",
                   __FILE__,
                   __LINE__,
                   traceVarValue,
                   errno);

            SETENV(CDKTRACE_VAR, "0");
            global_cdktrace_flag = FALSE;

            return;
        }

        lastRC = fwrite(traceMsg, 1, strlen(traceMsg), pTraceFile);

        /*************************************************************/
        /* If error writing to the trace file, then reset the        */
        /* global_cdktrace_flag to turn tracing off altogether.      */
        /*************************************************************/
        if (lastRC < 0)
        {
            printf("%s[%i]: Error writing to trace file=%s; errno=%i; trace now disabled\n",
                   __FILE__,
                   __LINE__,
                   traceVarValue,
                   errno);

            SETENV(CDKTRACE_VAR, "0");
            global_cdktrace_flag = FALSE;
            fclose(pTraceFile);

            return;
        }
        else
        {
            fflush(pTraceFile);
        }
    }

    if (global_srvtrcs_active_flag)
    {
        close(traceFd);
    }
    else
    {
        fclose(pTraceFile);
    }

    return;
}


