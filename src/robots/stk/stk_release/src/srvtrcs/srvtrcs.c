/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      srvtrcs.c                                        */
/** Description:    The trace service server.                        */
/**                                                                  */
/**                 This module:                                     */
/**                 (1) Gets the environmental variable STRCPIPE     */
/**                     containing the name of the trace pipe;       */
/**                 (2) Gets the environmental variable STRCFILE     */
/**                     containing the name of the trace file;       */
/**                 (3) Creates the named pipe $STRCPIPE and         */
/**                     opens the file $STRCFILE.                    */
/**                 (4) Continuously reads from the named pipe and   */
/**                     writes to the named file.                    */
/**                 (5) The named pipe and named file are closed     */
/**                     when the process is terminated.              */
/**                                                                  */
/**                 In addition to the STRCPIPE and STRCFILE         */
/**                 environmental variables, the trace client        */
/**                 function (srvtrcc) uses the environmental        */
/**                 variable CDKTRACE={1|0} To turn on tracing.      */
/**                 See comments in srvtrcc.c.                       */
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
static void srvtrcs_termination(int signum);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: srvtrcs (main)                                    */
/** Description:   Initialize the trace service server.              */
/**                                                                  */
/** This function:                                                   */
/** (1) Gets the environmental variable STRCPIPE                     */
/**     containing the name of the trace pipe;                       */
/** (2) Gets the environmental variable STRCFILE                     */
/**     containing the name of the trace file;                       */
/** (3) Creates the named pipe $STRCPIPE and opens the               */
/**     file $STRCFILE.                                              */
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
#define SELF "srvtrcs"

/*********************************************************************/
/* Global variables:                                                 */
/*********************************************************************/
int                tracePid;
int                traceDebugFlag;
int                traceBytesRead;
int                traceBytesWritten;
int                tracePipeFd;
FILE              *pTraceFile;
char               tracePipeName[256];
char               traceFileName[256];
char               traceArg0[256];

int main(int   argc, 
         char *argv[])
{
    int                 lastRC;
    int                 i;
    int                 bytesRead;
    int                 bytesWritten;
    int                 tenKBytesRead;
    int                 lastTenKBytesRead   = 0;
    char                fdFlags;
    char                traceBuffer[STRC_MAX_LINE_SIZE + 1];

    tracePid = getpid();
    traceDebugFlag = FALSE;
    traceBytesRead = 0;
    traceBytesWritten = 0;
    tracePipeFd = 0;
    pTraceFile = NULL;
    strcpy(tracePipeName, DEFAULT_STRCPIPE);
    strcpy(tracePipeName, DEFAULT_STRCFILE);

    /*****************************************************************/
    /* Handle termination signals.                                   */
    /*****************************************************************/
    signal(SIGINT, srvtrcs_termination);
    signal(SIGQUIT, srvtrcs_termination);
    signal(SIGTERM, srvtrcs_termination);
    signal(SIGTSTP, srvtrcs_termination);

    /*****************************************************************/
    /* Process the command line arguments.                           */
    /*****************************************************************/
    strcpy(traceArg0, argv[0]);

    printf("\n%s: Trace server started\n",
           traceArg0);

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "DEBUG") == 0)
        {
            traceDebugFlag = TRUE;
        }
        else
        {
            printf("%s: Unknown argument=%s ignored\n",
                   traceArg0,
                   argv[i]);
        }
    }

    /*****************************************************************/
    /* Get the environmental variables for STRCPIPE and STRCFILE.    */
    /* If not defined, then use the default names.                   */
    /*****************************************************************/
    if (getenv(STRCPIPE_VAR) != NULL)
    {
        strcpy(tracePipeName, getenv(STRCPIPE_VAR));
    }
    else
    {
        strcpy(tracePipeName, DEFAULT_STRCPIPE);
    }

    if (getenv(STRCFILE_VAR) != NULL)
    {
        strcpy(traceFileName, getenv(STRCFILE_VAR));
    }
    else
    {
        strcpy(traceFileName, DEFAULT_STRCFILE);
    }

    /*****************************************************************/
    /* Open the trace pipe and the trace file.                       */
    /*****************************************************************/
    lastRC = mkfifo(tracePipeName, 0666);

    if (lastRC == -1)
    {
        if (errno == EEXIST)
        {
            if (traceDebugFlag)
            {
                printf("%s: Trace input pipe=%s already exists\n",
                       traceArg0,
                       tracePipeName);
            }
        }
        else
        {
            printf("%s: Error creating trace input pipe=%s; errno=%i\n",
                   traceArg0,
                   tracePipeName,
                   errno);

            exit(1);
        }
    }

    /*****************************************************************/
    /* Use open() and not fopen() for the pipe to avoid blocking     */
    /* until pipe is opened for output.                              */
    /*****************************************************************/
    tracePipeFd = open(tracePipeName, (O_RDONLY | O_NONBLOCK | O_SYNC));

    if (tracePipeFd == 0)
    {
        printf("%s: Error opening trace input pipe=%s; errno=%i\n",
               traceArg0,
               tracePipeName,
               errno);

        exit(1);
    }

    if (traceDebugFlag)
    {
        printf("%s: Trace input pipe=%s (FD=%i) opened\n",
               traceArg0,
               tracePipeName,
               tracePipeFd);
    }

    pTraceFile = fopen(traceFileName, "ab");

    if (pTraceFile == NULL)
    {
        printf("%s: Error opening trace output file=%s; errno=%i\n",
               traceArg0,
               traceFileName,
               errno);

        exit(1);
    }

    if (traceDebugFlag)
    {
        printf("%s: Trace ouput file=%s (FILE=%08X) opened\n",
               traceArg0,
               traceFileName,
               pTraceFile);
    }

    /*****************************************************************/
    /* Read from the pipe, and write to the file until terminated.   */
    /* For each iteration we reset the O_NONBLOCK flag (i.e. we      */
    /* want the read to block until there is data).                  */
    /* FIFO read blocking significantly decreases CPU usage.         */
    /*                                                               */
    /* FIFO read blocking relies on the pipe being opened for        */
    /* write somewhere.  That somewhere is csi_ipcdisp.c which       */
    /* opens (and keeps open until termination) the log pipe         */
    /* and trace pipe even though it will not write any data         */
    /* to the opened pipe(s).                                        */
    /*****************************************************************/
    while (1)
    {
        fdFlags = fcntl(tracePipeFd, F_GETFL, 0);
        fdFlags &= ~(O_NONBLOCK);
        fcntl(tracePipeFd, F_SETFL, fdFlags);

        memset(traceBuffer, 0, sizeof(traceBuffer));

        bytesRead = read(tracePipeFd, traceBuffer, STRC_MAX_LINE_SIZE);

        if (bytesRead > 0)
        {
            traceBytesRead += bytesRead;
            bytesWritten = fwrite(traceBuffer, 1, bytesRead, pTraceFile);
            fflush(pTraceFile);

            if (bytesWritten != bytesRead)
            {
                printf("%s: Error writing to trace output file=%s\n",
                       traceArg0,
                       traceFileName);
            }
            else
            {
                traceBytesWritten += bytesWritten;
            }

            if (traceDebugFlag)
            {
                tenKBytesRead = traceBytesRead / 10000;

                if (tenKBytesRead != lastTenKBytesRead)
                {
                    lastTenKBytesRead = tenKBytesRead;

                    printf("%s: Processed %i bytes\n",
                           lastTenKBytesRead);
                }
            }
        }
    } 

    srvtrcs_termination(0);

    exit(0);
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: srvtrcs_termination                               */
/** Description:   Terminate the trace service server.               */
/**                                                                  */
/** This function closes the named pipe and file.                    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "srvtrcs_termination"

void srvtrcs_termination(int signum)
{
    if (traceDebugFlag)
    {
        printf("\n%s: Trace server termination signal=%i\n", 
               traceArg0,
               signum);
    }

    sleep(1);

    if (tracePipeFd != 0)
    {
        close(tracePipeFd);
        tracePipeFd = 0;
    }

    if (pTraceFile != NULL)
    {
        fclose(pTraceFile);
        pTraceFile = NULL;
    }

    printf("\n%s: Trace server terminated\n", 
           traceArg0);

    if (signum == 0)
    {
        return;
    }

    raise(signum);
}


