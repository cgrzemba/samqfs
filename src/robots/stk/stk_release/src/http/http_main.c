/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_main.c                                      */
/** Description:    t_http server main entry and initialization.     */
/**                                                                  */
/**                 t_http is a test HTTP server that will return    */
/**                 certain canned XAPI responses to client XAPI     */
/**                 queries.                                         */
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


/*********************************************************************/
/* Global variables:                                                 */
/*********************************************************************/
struct HTTPCVT *global_httpcvt;


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void http_termination(int signum);


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: main                                              */
/** Description:   t_http server main entry and initialization.      */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_main"

int main (int  argc, char **argv)
{
    int                 lastRC;
    int                 i;
    int                 goodPortFlag;
    int                 wkPort;
    int                 shMemSegId;
    int                 shmFlags;
    int                 shmPermissions;
    key_t               shmKey;
    size_t              shmSize;
    char                wkGetEnvString[(CSI_HOSTNAMESIZE * 2) + 1];
    struct HTTPCVT     *pHttpcvt            = NULL;

    global_httpcvt = NULL;

    GLOBAL_SRVCOMMON_SET("HTTP");

    LOGMSG(STATUS_SUCCESS, 
           "HTTP Initiation Started");

    /*****************************************************************/
    /* Handle termination signals.                                   */
    /*****************************************************************/
    signal(SIGINT, http_termination);
    signal(SIGQUIT, http_termination);
    signal(SIGTERM, http_termination);
    signal(SIGTSTP, http_termination);

    /*****************************************************************/
    /* Create the shared memory segment that will contain the        */
    /* XAPICVT including the XAPIREQE table of active XAPI requests, */
    /* and the remainder of the ACSAPI-to-XAPI control variables.    */
    /*****************************************************************/
    shmKey = HTTP_CVT_SHM_KEY;
    shmSize = sizeof(struct HTTPCVT);
    shmPermissions = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    shmFlags = (IPC_CREAT | shmPermissions);

    shMemSegId = shmget(shmKey, shmSize, shmFlags);

    TRMSGI(TRCI_STORAGE,
           "shMemSegId=%d (%08X) after shmget(key=HTTPCVT=%d, "
           "size=%d, flags=%d (%08X))\n",
           shMemSegId,
           shMemSegId,
           shmKey,
           shmSize,
           shmFlags,
           shmFlags);

    if (shMemSegId < 0)
    {
        LOGMSG(STATUS_PROCESS_FAILURE, 
               "HTTP server could not allocate HTTPCVT shared memory segment; errno=%d (%s)",
               errno,
               strerror(errno));

        return STATUS_SHARED_MEMORY_ERROR;
    }

    /*****************************************************************/
    /* Attach the shared memory segment to our data space.           */
    /*****************************************************************/
    pHttpcvt = (struct HTTPCVT*) shmat(shMemSegId, NULL, 0);

    TRMSGI(TRCI_STORAGE,
           "pHttpcvt=%08X after shmat(id=%d, NULL, 0)\n",
           pHttpcvt,
           shMemSegId);

    if (pHttpcvt == NULL)
    {
        LOGMSG(STATUS_SHARED_MEMORY_ERROR, 
               "HTTP server could not attach HTTPCVT shared memory segment");

        return STATUS_SHARED_MEMORY_ERROR;
    }

    global_httpcvt = pHttpcvt;

    /*****************************************************************/
    /* Initialize the HTTPCVT table.                                 */
    /*****************************************************************/
    memset((char*) pHttpcvt, 0, sizeof(struct HTTPCVT));

    memcpy(pHttpcvt->cbHdr, 
           HTTPCVT_ID,
           sizeof(pHttpcvt->cbHdr));

    pHttpcvt->cvtShMemSegId = shMemSegId;
    pHttpcvt->httpPid = getpid();

    /*****************************************************************/
    /* Initialize the global variables for ACSAPI-to-XAPI conversion.*/
    /*****************************************************************/
    if (getenv(XAPI_HOSTNAME) != NULL)
    {
        strcpy(wkGetEnvString, getenv(XAPI_HOSTNAME));

        if (strlen(wkGetEnvString) > XAPI_HOSTNAME_SIZE)
        {
            TRMSGI(TRCI_SERVER,
                   "Truncating HOSTNAME from %i to %i characters\n",
                   strlen(wkGetEnvString) ,
                   XAPI_HOSTNAME_SIZE);

            memcpy(pHttpcvt->xapiHostname,
                   wkGetEnvString,
                   XAPI_HOSTNAME_SIZE);

            pHttpcvt->xapiHostname[XAPI_HOSTNAME_SIZE] = 0;
        }
        else
        {
            strcpy(pHttpcvt->xapiHostname, wkGetEnvString);
        }
    }
    else
    {
        strcpy(pHttpcvt->xapiHostname, " ");
    }

    goodPortFlag = TRUE;

    if (getenv(XAPI_PORT) != NULL)
    {
        strcpy(wkGetEnvString, getenv(XAPI_PORT));

        TRMSGI(TRCI_SERVER,
               "XAPI_PORT=%s",
               wkGetEnvString);

        for (i = 0; i < strlen(wkGetEnvString); i++)
        {
            if (!(isdigit(wkGetEnvString[i])))
            {
                goodPortFlag = FALSE;

                break;
            }
        }

        if (goodPortFlag)
        {
            wkPort = atoi(wkGetEnvString);

            TRMSGI(TRCI_SERVER,
                   "wkPort=%i",
                   wkPort);

            if (wkPort > 65535)
            {
                goodPortFlag = FALSE;
            }
            else
            {
                pHttpcvt->xapiPort = wkPort;
            }
        }
    }
    else
    {
        goodPortFlag = FALSE;
    }

    if (!(goodPortFlag))
    {
        TRMSGI(TRCI_SERVER,
               "Invalid or no XAPI_PORT=%s specified; using default",
               wkGetEnvString);

        pHttpcvt->xapiPort = XAPI_PORT_DEFAULT;
    }

    if (getenv(XAPI_TAPEPLEX) != NULL)
    {
        strcpy(wkGetEnvString, getenv(XAPI_TAPEPLEX));

        if (strlen(wkGetEnvString) > XAPI_TAPEPLEX_SIZE)
        {
            TRMSGI(TRCI_SERVER,
                   "Truncating TAPEPLEX from %i to %i characters\n",
                   strlen(wkGetEnvString) ,
                   XAPI_TAPEPLEX_SIZE);

            memcpy(pHttpcvt->xapiTapeplex,
                   wkGetEnvString,
                   XAPI_TAPEPLEX_SIZE);

            pHttpcvt->xapiTapeplex[XAPI_TAPEPLEX_SIZE] = 0;
        }
        else
        {
            strcpy(pHttpcvt->xapiTapeplex, wkGetEnvString);
        }
    }
    else
    {
        strcpy(pHttpcvt->xapiTapeplex, " ");         
    }

    if (getenv(XAPI_SUBSYSTEM) != NULL)
    {
        strcpy(wkGetEnvString, getenv(XAPI_SUBSYSTEM));

        if (strlen(wkGetEnvString) > XAPI_SUBSYSTEM_SIZE)
        {
            TRMSGI(TRCI_SERVER,
                   "Truncating SUBSYSTEM from %i to %i characters\n",
                   strlen(wkGetEnvString) ,
                   XAPI_SUBSYSTEM_SIZE);

            memcpy(pHttpcvt->xapiSubsystem,
                   wkGetEnvString,
                   XAPI_SUBSYSTEM_SIZE);

            pHttpcvt->xapiSubsystem[XAPI_SUBSYSTEM_SIZE] = 0;
        }
        else
        {
            strcpy(pHttpcvt->xapiSubsystem, wkGetEnvString);
        }
    }
    else
    {
        if (pHttpcvt->xapiTapeplex[0] > ' ')
        {
            memcpy(pHttpcvt->xapiSubsystem,
                   pHttpcvt->xapiTapeplex,
                   XAPI_SUBSYSTEM_SIZE);

            pHttpcvt->xapiSubsystem[XAPI_SUBSYSTEM_SIZE] = 0;

            TRMSGI(TRCI_SERVER,
                   "Defaulting SUBSYSTEM=%s to TAPEPLEX name\n",
                   pHttpcvt->xapiSubsystem);
        }
        else
        {
            strcpy(pHttpcvt->xapiSubsystem, " ");
        }
    }

    if (getenv(XAPI_VERSION) != NULL)
    {
        strcpy(wkGetEnvString, getenv(XAPI_VERSION));

        if (strlen(wkGetEnvString) > XAPI_VERSION_SIZE)
        {
            TRMSGI(TRCI_SERVER,
                   "Truncating VERSION from %i to %i characters\n",
                   strlen(wkGetEnvString) ,
                   XAPI_VERSION_SIZE);

            memcpy(pHttpcvt->xapiVersion,
                   wkGetEnvString,
                   XAPI_VERSION_SIZE);

            pHttpcvt->xapiVersion[XAPI_VERSION_SIZE] = 0;
        }
        else
        {
            strcpy(pHttpcvt->xapiVersion, wkGetEnvString);
        }
    }
    else
    {
        strcpy(pHttpcvt->xapiVersion, XAPI_VERS_DEFAULT);

        TRMSGI(TRCI_SERVER,
               "Defaulting VERSION to %s\n",
               pHttpcvt->xapiVersion);
    }

    if (getenv(XAPI_USER) != NULL)
    {
        strcpy(wkGetEnvString, getenv(XAPI_USER));

        if (strlen(wkGetEnvString) > XAPI_USER_SIZE)
        {
            TRMSGI(TRCI_SERVER,
                   "Truncating USER from %i to %i characters\n",
                   strlen(wkGetEnvString) ,
                   XAPI_USER_SIZE);

            memcpy(pHttpcvt->xapiUser,
                   wkGetEnvString,
                   XAPI_USER_SIZE);

            pHttpcvt->xapiUser[XAPI_USER_SIZE] = 0;
        }
        else
        {
            strcpy(pHttpcvt->xapiUser, wkGetEnvString);
        }
    }
    else
    {
        strcpy(pHttpcvt->xapiUser, " ");
    }

    if (getenv(XAPI_GROUP) != NULL)
    {
        strcpy(wkGetEnvString, getenv(XAPI_GROUP));

        if (strlen(wkGetEnvString) > XAPI_GROUP_SIZE)
        {
            TRMSGI(TRCI_SERVER,
                   "Truncating GROUP from %i to %i characters\n",
                   strlen(wkGetEnvString) ,
                   XAPI_GROUP_SIZE);

            memcpy(pHttpcvt->xapiGroup,
                   wkGetEnvString,
                   XAPI_GROUP_SIZE);

            pHttpcvt->xapiGroup[XAPI_GROUP_SIZE] = 0;
        }
        else
        {
            strcpy(pHttpcvt->xapiGroup, wkGetEnvString);
        }
    }
    else
    {
        strcpy(pHttpcvt->xapiGroup, " ");
    }

    TRMSGI(TRCI_SERVER,
           "pHttpcvt->xapiHostname=%s, pHttpcvt->xapiPort=%i\n",
           pHttpcvt->xapiHostname,
           pHttpcvt->xapiPort);

    TRMSGI(TRCI_SERVER,
           "pHttpcvt->xapiTapeplex=%s, pHttpcvt->xapiSubsystem=%s, pHttpcvt->xapiVersion=%s\n",
           pHttpcvt->xapiTapeplex,
           pHttpcvt->xapiSubsystem,
           pHttpcvt->xapiVersion);

    TRMSGI(TRCI_SERVER,
           "pHttpcvt->xapiUser=%s, pHttpcvt->xapiGroup=%s\n",
           pHttpcvt->xapiUser,
           pHttpcvt->xapiGroup);

    /*****************************************************************/
    /* Initialize the httpCdsLock semaphore.                         */
    /*****************************************************************/
    lastRC = sem_init(&pHttpcvt->httpCdsLock, 0, 1);

    TRMSGI(TRCI_SERVER,
           "lastRC=%d from sem_init(addr=%08X)\n",
           lastRC,
           &pHttpcvt->httpCdsLock);

    if (lastRC < 0)
    {
        LOGMSG(STATUS_PROCESS_FAILURE, 
               "HTTP server could not initialize httpCdsLock semaphore");

        return STATUS_PROCESS_FAILURE;
    }

    TRMEMI(TRCI_SERVER, pHttpcvt, sizeof(struct HTTPCVT),
           "HTTPCVT:\n");

    lastRC = http_listen(pHttpcvt);

    http_term(pHttpcvt);

    return lastRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: http_termination                                  */
/** Description:   Terminate the http simulated server.              */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "http_termination"

void http_termination(int signum)
{
    http_term(global_httpcvt);

    if (signum == 0)
    {
        return;
    }

    raise(signum);
}




