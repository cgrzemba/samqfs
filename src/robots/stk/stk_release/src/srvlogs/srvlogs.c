/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      srvlogs.c                                        */
/** Description:    The log service server.                          */
/**                                                                  */
/**                 This module:                                     */
/**                 (1) Gets the environmental variable SLOGPIPE     */
/**                     containing the name of the log input pipe;   */
/**                 (2) Gets the environmental variable SLOGFILE     */
/**                     containing the name of the log output file;  */
/**                 (3) Creates the named pipe $SLOGPIPE and         */
/**                     opens the file $SLOGFILE.                    */
/**                 (4) Continuously reads from the named pipe and   */
/**                     writes to the named file.                    */
/**                 (5) The named pipe and named file are closed     */
/**                     when the process is terminated.              */ 
/**                                                                  */
/**                 In addition to the SLOGPIPE and SLOGFILE         */
/**                 environmental variables, the log client          */
/**                 function (srvlogc) uses the environmental        */
/**                 variable CDKLOG={1|0} To turn on logging.        */
/**                 See comments in srvlogc.c.                       */
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
/** ELS720/CDK240  Joseph Nofi     05/03/11                          */
/**     Created for CDK to add XAPI support.                         */
/** I6087283       Joseph Nofi     01/08/13                          */
/**     Fix high CPU utilization due to unblocked read processing.   */
/**                                                                  */
/***END PROLOGUE******************************************************/

/*********************************************************************/
/* Includes:                                                         */
/*********************************************************************/
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "api/defs_api.h"
#include "srvcommon.h"


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void srvlogs_termination(int signum);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: srvlogs (main)                                    */
/** Description:   Initialize the log service server.                */
/**                                                                  */
/** This function:                                                   */
/** (1) Gets the environmental variable SLOGPIPE                     */
/**     containing the name of the log input pipe;                   */
/** (2) Gets the environmental variable SLOGFILE                     */
/**     containing the name of the log output file;                  */
/** (3) Creates the named pipe $SLOGPIPE and opens                   */
/**     the file $SLOGFILE.                                          */
/** (4) Continuously reads from the named pipe and                   */
/**     writes to the named file.                                    */
/** (5) The named pipe and named file are closed                     */
/**     when the process is terminated.                              */ 
/**                                                                  */
/** Command Line Arguments:                                          */
/** DEBUG may be specified to produce debug messages to stdout.      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "srvlogs"

/*********************************************************************/
/* Global variables:                                                 */
/*********************************************************************/
int                logPid;
int                logDebugFlag;
int                logBytesRead;
int                logBytesWritten;
int                logPipeFd;
FILE              *pLogFile;
char               logPipeName[256];
char               logFileName[256];
char               logArg0[256];

int main(int   argc, 
         char *argv[])
{
    int                 lastRC;
    int                 i;
    int                 bytesRead;
    int                 bytesWritten;
    int                 tenKBytesRead;
    int                 lastTenKBytesRead   = 0;
    char               *pInput;
    char                fdFlags;
    char                logBuffer[SLOG_MAX_LINE_SIZE + 1];

    logPid = getpid();
    logDebugFlag = FALSE;
    logBytesRead = 0;
    logBytesWritten = 0;
    logPipeFd = 0;
    pLogFile = NULL;
    strcpy(logPipeName, DEFAULT_SLOGPIPE);
    strcpy(logPipeName, DEFAULT_SLOGFILE);

    /*****************************************************************/
    /* Handle termination signals.                                   */
    /*****************************************************************/
    signal(SIGINT, srvlogs_termination);
    signal(SIGQUIT, srvlogs_termination);
    signal(SIGTERM, srvlogs_termination);
    signal(SIGTSTP, srvlogs_termination);

    /*****************************************************************/
    /* Process the command line arguments.                           */
    /*****************************************************************/
    strcpy(logArg0, argv[0]);

    printf("\n%s: Log server started\n",
           logArg0);

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "DEBUG") == 0)
        {
            logDebugFlag = TRUE;
        }
        else
        {
            printf("%s: Unknown argument=%s ignored\n",
                   logArg0,
                   argv[i]);
        }
    }

    /*****************************************************************/
    /* Get the environmental variables for SLOGPIPE and SLOGFILE.    */
    /* If not defined, then use the default names.                   */
    /*****************************************************************/
    if (getenv(SLOGPIPE_VAR) != NULL)
    {
        strcpy(logPipeName, getenv(SLOGPIPE_VAR));
    }
    else
    {
        strcpy(logPipeName, DEFAULT_SLOGPIPE);
    }

    if (getenv(SLOGFILE_VAR) != NULL)
    {
        strcpy(logFileName, getenv(SLOGFILE_VAR));
    }
    else
    {
        strcpy(logFileName, DEFAULT_SLOGFILE);
    }

    /*****************************************************************/
    /* Open the log pipe and the log file.                           */
    /*****************************************************************/
    lastRC = mkfifo(logPipeName, 0666);

    if (lastRC == -1)
    {
        if (errno == EEXIST)
        {
            if (logDebugFlag)
            {
                printf("%s: Log input pipe=%s already exists\n",
                       logArg0,
                       logPipeName);
            }
        }
        else
        {
            printf("%s: Error creating log input pipe=%s; errno=%i\n",
                   logArg0,
                   logPipeName,
                   errno);

            exit(1);
        }
    }

    /*****************************************************************/
    /* Use open() and not fopen() for the pipe to avoid blocking     */
    /* until pipe is opened for output.                              */
    /*****************************************************************/
    logPipeFd = open(logPipeName, (O_RDONLY | O_NONBLOCK | O_SYNC));

    if (logPipeFd == 0)
    {
        printf("%s: Error opening log input pipe=%s; errno=%i\n",
               logArg0,
               logPipeName,
               errno);

        exit(1);
    }

    if (logDebugFlag)
    {
        printf("%s: Log input pipe=%s (FD=%i) opened\n",
               logArg0,
               logPipeName,
               logPipeFd);
    }

    pLogFile = fopen(logFileName, "ab");

    if (pLogFile == NULL)
    {
        printf("%s: Error opening log output file=%s; errno=%i\n",
               logArg0,
               logFileName,
               errno);

        exit(1);
    }

    if (logDebugFlag)
    {
        sprintf(logBuffer, "%s: Log ouput file=%s (FILE=%08X) opened\n",
                logArg0,
                logFileName,
                pLogFile);

        printf("%s", logBuffer);
        fputs(logBuffer, pLogFile);
        fflush(pLogFile);
    }

    /*****************************************************************/
    /* Read from the pipe, and write to the file until terminated.   */
    /* For each iteration we reset the O_NONBLOCK flag (i.e. we      */
    /* want the read to block until there is data).                  */
    /* FIFO read blocking significantly decreases CPU usage.         */
    /*                                                               */
    /* FIFO read blocking relies on the pipe being opened for        */
    /* write somewhere.  That somewhere is csi_init.c which          */
    /* opens (and keeps open until termination) the log pipe         */
    /* and trace pipe even though it will not write any data         */
    /* to the opened pipe(s).                                        */
    /*****************************************************************/
    while (1)
    {
        fdFlags = fcntl(logPipeFd, F_GETFL, 0);
        fdFlags &= ~(O_NONBLOCK);
        fcntl(logPipeFd, F_SETFL, fdFlags);

        memset(logBuffer, 0, sizeof(logBuffer));

        bytesRead = read(logPipeFd, logBuffer, SLOG_MAX_LINE_SIZE);

        if (bytesRead > 0)
        {
            logBytesRead += bytesRead;
            bytesWritten = fwrite(logBuffer, 1, bytesRead, pLogFile);
            fflush(pLogFile);

            if (bytesWritten != bytesRead)
            {
                printf("%s: Error writing to log output file=%s\n",
                       logArg0,
                       logFileName);
            }
            else
            {
                logBytesWritten += bytesWritten;
            }

            if (logDebugFlag)
            {
                tenKBytesRead = logBytesRead / 10000;

                if (tenKBytesRead != lastTenKBytesRead)
                {
                    lastTenKBytesRead = tenKBytesRead;

                    printf("%s: Processed %i bytes\n",
                           lastTenKBytesRead);
                }
            }
        }
    } 

    srvlogs_termination(0);

    exit(0);
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: srvlogs_termination                               */
/** Description:   Terminate the log service server.                 */
/**                                                                  */
/** This function closes the named pipe and file.                    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "srvlogs_termination"

void srvlogs_termination(int signum)
{
    if (logDebugFlag)
    {
        printf("\n%s: Log server termination signal=%i\n", 
               logArg0,
               signum);
    }

    sleep(1);

    if (logPipeFd != 0)
    {
        close(logPipeFd);
        logPipeFd = 0;
    }

    if (pLogFile != NULL)
    {
        fclose(pLogFile);
        pLogFile = NULL;
    }

    printf("\n%s: Log server terminated\n", 
           logArg0);

    if (signum == 0)
    {
        return;
    }

    raise(signum);
}


