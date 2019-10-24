static char SccsId[] = "@(#)csi_init.c      5.10 11/12/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Source/System:
 *
 *      csi_init.c      - source file of the CSI module of the ACSSS
 *
 * Name:
 *
 *      csi_init
 *
 * Description:
 *
 *      Routine csi_init() controls the initialization sequences of the CSI.
 *
 *      Gets process id of this process csi_pid (global) which is used for
 *      various identification schemes, like transaction id tagging.
 *
 *      Calls csi_logevent() with STATUS_SUCCESS and message
 *      MSG_INITIATION_STARTED to log a message indicating the start of 
 *      the CSI initializtion process.
 *
 *      Calls signal() to set the mechanism to catch the SIGTERM signal and 
 *      vector its response to the function csi_sighndlr().
 *
 *      If conditionally compiled for -DADI:
 *          Calls signal() to set the mechanism to catch the SIGCLD signal and
 *          vector its response to the function csi_sighndlr().
 *      End -DADI
 *
 *      If signal returns -1, calls csi_logevent with
 *      STATUS_PROCESS_FAILURE & message MSG_SYSTEM_ERROR concatenated
 *      with the current value of the global Unix variable "errno".
 *      Then returns STATUS_PROCESS_FAILURE to the caller.
 *
 *      Calls csi_netbufinit() to initialize the csi network packet buffer.
 *
 *              If STATUS_PROCESS_FAILURE == csi_netbufinit()
 *              returns STATUS_PROCESS_FAILURE to the caller.
 *
 *      Calls csi_qinit() to initialize the RPC SSI return address 
 *      storage queue (connect table) for tracking the addresses of CSI clients.
 *
 *              If STATUS_SUCCESS != csi_qinit(), csi_init() returns 
 *              status STATUS_PROCESS_FAILURE to the caller.
 *
 *      Calls csi_qinit() to initialize the network output queue.
 *
 *              If STATUS_SUCCESS != csi_qinit(), csi_init() returns 
 *              status STATUS_PROCESS_FAILURE to the caller.
 *
 *      If conditionally compiled with -DADI:
 *          Initialize the csi_ssi_adi_addr global.
 *      Else  (not compiled with -DADI) :
 *          Calls csi_svcinit() to initialize the CSI as an RPC service.
 *              If STATUS_SUCCESS != csi_svcinit(), csi_init() returns status 
 *              STATUS_PROCESS_FAILURE to the caller.
 *      End (-DADI)
 *
 *      Calls csi_logevent() with STATUS_SUCCESS and message
 *      MSG_INITIATION_COMPLETED to log the completion of the CSI 
 *      initialization process. 
 *
 * Return Values:
 *
 *      STATUS_SUCCESS          - Initialization successfully completed.
 *      STATUS_PROCESS_FAILURE  - Initialization failed.
 *
 * Implicit Inputs:
 *
 *      NONE
 *
 * Implicit Outputs:
 *
 *      NONE    
 *
 * Considerations:
 *
 *      Dependent on the handling of the global variables in cl_ipc_create().
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner       01-Jan-1989     Created.
 *      J. A. Wishner       05-Jun-1989     Added initialization of network
 *                                          output queue.
 *      H. I. Grapek        31-Aug-1991     Fixed lint errors 
 *      E. A. ALongi        29-Jul-1992     Support for minimum version and
 *                                          allowing acspd.  Merged in R3.0.1
 *                                          changes.
 *      E. A. Alongi        30-Sep-1992     Shortened environment variable name
 *                                          for minimum version to
 *                                          ACSLS_MIN_VERSION (The original
 *                                          name caused problems with echo and
 *                                          csh - the var name was too long.)
 *      E. A. Alongi         30-Oct-1992    Comment ifdefs and replace 0 with
 *                                          '\0' in call to memset().
 *      Emanuel A. Alongi    27-Jul-1993    Elminated global variable 
 *                                          initializations now set dynamically
 *      David Farmer         17-Aug-1993    changes from r4 bull port
 *                                          catch SIGPIPE
 *      E. A. Alongi         28-Sep-1993    Replace signal() with sigaction().
 *      E. A. Alongi         12-Nov-1993    Corrected all flint detected errors.
 *      Mike Williams        01-Jun-2010    Included unistd.h
 *      Joseph Nofi          15-Jun-2011    XAPI support;
 *                                          Added XAPICVT shared memory segment 
 *                                          acquisition and initialization.
 *      Joseph Nofi          01-Jan-2013    I6087283;
 *                                          Fix high CPU utilization in srvlogs 
 *                                          and srvtrcs daemons due to unblocked
 *                                          read processing. 
 */


/*
 *      Header Files:
 */
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#if defined(XAPI) && defined(SSI)
    #include <fcntl.h>
    #include <pthread.h>
    #include <semaphore.h>
    #include <sys/ipc.h>
    #include <sys/shm.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif /* XAPI and SSI */

#include "cl_pub.h"
#include "ml_pub.h"
#include "csi.h"
#include "dv_pub.h"
#include "system.h"
#include "srvcommon.h"

#if defined(XAPI) && defined(SSI)
    #include "xapi/xapi.h"
#endif /* XAPI and SSI */

#if defined(ADI) && defined(SSI)
#include <memory.h>
#include <string.h>
#endif

/*
 *      Defines, Typedefs and Structure Definitions:
 */


/*
 *      Global and Static Variable Declarations:
 */
static char     *st_module = "csi_init()";

/*
 *      Procedure Type Declarations:
 */
#undef SELF
#define SELF "csi_init"

STATUS 
csi_init (void)
{
    struct sigaction    action;     /* needed for calls to sigaction() */
    
#if defined(XAPI) && defined(SSI)
    int                 wkPort;
    char                wkGetEnvString[(CSI_HOSTNAMESIZE * 2) + 1];
    int                 i;
    int                 goodPortFlag;
    int                 lastRC;
    int                 shMemSegId;
    int                 shmFlags;
    int                 shmPermissions;
    key_t               shmKey;
    size_t              shmSize;
    struct XAPICVT     *pXapicvt;
    struct XAPIREQE    *pXapireqe;
    char                logVarValue[256];
    char                traceVarValue[256];
#endif /* XAPI  and SSI */

#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module, 0, (unsigned long) 0);
#endif /* DEBUG */

    csi_pid = getpid(); /* process id */

    /* initialize packet tracing */
    csi_trace_flag = (trace_value & TRACE_CSI_PACKETS) ? TRUE : FALSE;
    MLOGCSI((STATUS_SUCCESS,  st_module, CSI_NO_CALLEE, 
      MMSG(935, "Initiation Started")));

//  csi_trace_flag = -1; /* Turn on all possible trace bits*/

    /* setup for calls to sigaction() */
    action.sa_handler = (SIGFUNCP) csi_sighdlr;
    (void) sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    /* SA_RESTART is a sigaction() flag (sa_flags) defined on most platforms.   
     * This flag causes certain "slow" systems calls (usually those that can
     * block) to restart after being interrupted by a signal.  The SA_RESTART
     * flag is not defined under SunOS because the interrupted system call is
     * automatically restarted by default.  So, in order to make it possible
     * to write one version of the sigaction() code, SA_RESTART is defined in
     * defs.h for platforms running SunOS.
     */
    action.sa_flags |= SA_RESTART;

    /* register to trap signal */
    if (sigaction(SIGTERM, &action, NULL) < 0) {
        MLOGCSI((STATUS_PROCESS_FAILURE,  st_module,  "sigaction()", 
      MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    /* set up to catch signal used to toggle packet tracing */
    if (sigaction(SIGUSR1, &action, NULL) < 0) {
        MLOGCSI((STATUS_PROCESS_FAILURE,  st_module,  "sigaction()", 
      MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    /* set up to catch signal used to report closed socket */
    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        MLOGCSI((STATUS_PROCESS_FAILURE,  st_module,  "sigaction()", 
      MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }


#ifdef ADI
    /* register to trap SIGCLD (acceptable to 4.3BSD and System V) signal */
    if (sigaction(SIGCLD, &action, NULL) < 0) {
        MLOGCSI((STATUS_PROCESS_FAILURE,  st_module,  "sigaction()", 
      MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }
#endif /* ADI */

    /* initialize network packet buffering */
    if (csi_netbufinit(&csi_netbufp) != STATUS_SUCCESS)
        return(STATUS_PROCESS_FAILURE);

    /* initialize the csi connection queue */
    if (csi_qinit(&csi_lm_qid, CSI_MAXMEMB_LM_QUEUE, 
        CSI_CONNECTQ_NAME) != STATUS_SUCCESS) {
        MLOGCSI((STATUS_QUEUE_FAILURE, st_module,  "csi_qinit()", 
      MMSG(936, "Creation of connect queue failed")));
        return(STATUS_PROCESS_FAILURE);
    }

    /* initialize the network output queue */
    if (csi_qinit(&csi_ni_out_qid, CSI_MAXMEMB_NI_OUT_QUEUE,
        CSI_NI_OUTQ_NAME) != STATUS_SUCCESS) {
        MLOGCSI((STATUS_QUEUE_FAILURE, st_module,  "csi_qinit()",
      MMSG(937, "Creation of network output queue failed")));
        return(STATUS_PROCESS_FAILURE);
    }

#if defined(XAPI) && defined(SSI)
    /*****************************************************************/
    /* If XAPI and SSI, then Initialize the global variables for     */
    /* ACSAPI-to-XAPI conversion.                                    */
    /*****************************************************************/
    csi_xapi_conversion_flag = FALSE;
    csi_xapi_control_table = NULL;

    if (getenv(XAPI_CONVERSION) != NULL)
    {
        strcpy(wkGetEnvString, (char*) ((uintptr_t) getenv(XAPI_CONVERSION)));

        if ((strcmp("1", wkGetEnvString) == 0) ||
            (strcmp("TRUE", wkGetEnvString) == 0) ||
            (strcmp("YES", wkGetEnvString) == 0) ||
            (strcmp("ON", wkGetEnvString) == 0))
        {
            csi_xapi_conversion_flag = TRUE;
        }
    }

    /*****************************************************************/
    /* Create the shared memory segment that will contain the        */
    /* XAPICVT including the XAPIREQE table of active XAPI requests, */
    /* and the remainder of the ACSAPI-to-XAPI control variables.    */
    /*****************************************************************/
    shmKey = XAPI_CVT_SHM_KEY;
    shmSize = sizeof(struct XAPICVT);
    shmPermissions = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    shmFlags = (IPC_CREAT | shmPermissions);

    shMemSegId = shmget(shmKey, shmSize, shmFlags);

    TRMSGI(TRCI_STORAGE,
           "shMemSegId=%d (%08X) after shmget(key=XAPICVT=%d, "
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
               "Could not allocate XAPICVT shared memory segment; errno=%d (%s)",
               errno,
               strerror(errno));

        return STATUS_PROCESS_FAILURE;
    }

    /*****************************************************************/
    /* Attach the shared memory segment to our data space.           */
    /*****************************************************************/
    pXapicvt = (struct XAPICVT*) shmat(shMemSegId, NULL, 0);

    TRMSGI(TRCI_STORAGE,
           "pXapicvt=%08X after shmat(id=%d, NULL, 0)\n",
           pXapicvt,
           shMemSegId);

    if (pXapicvt == NULL)
    {
        LOGMSG(STATUS_PROCESS_FAILURE, 
               "Could not attach XAPICVT shared memory segment");

        return STATUS_PROCESS_FAILURE;
    }

    csi_xapi_control_table = pXapicvt;

    /*****************************************************************/
    /* Initialize the XAPICVT table.                                 */
    /*****************************************************************/
    memset((char*) pXapicvt, 0, sizeof(struct XAPICVT));

    memcpy(pXapicvt->cbHdr, 
           XAPICVT_ID,
           sizeof(pXapicvt->cbHdr));

    pXapicvt->cvtShMemSegId = shMemSegId;

    /*****************************************************************/
    /* The following is a bit of a kluge to enable FIFO read         */
    /* blocking within the log and trace daemons.  We open (and      */
    /* leave open) the log and trace pipes even though we            */
    /* have no intention of writing anything from here.              */
    /*                                                               */
    /* Without this open, the read logic in srvlogs.c and            */
    /* srvtrcs.c will always return immediately even though          */
    /* there is no data, causing an increase in CPU usage            */
    /* reading 0 bytes.  With this open, the read will               */
    /* block until there is actual data in the pipe.                 */
    /*****************************************************************/
    if (getenv(SLOGPIPE_VAR) != NULL)
    {
        strcpy(logVarValue, (char*) ((uintptr_t) getenv(SLOGPIPE_VAR)));
    }
    else
    {
        strcpy(logVarValue, DEFAULT_SLOGPIPE);
    }

    pXapicvt->logPipeFd = open(logVarValue, (O_WRONLY | O_NONBLOCK | O_SYNC));

    if (getenv(STRCPIPE_VAR) != NULL)
    {
        strcpy(traceVarValue, (char*) ((uintptr_t) getenv(STRCPIPE_VAR)));
    }
    else
    {
        strcpy(traceVarValue, DEFAULT_STRCPIPE);
    }

    pXapicvt->tracePipeFd = open(traceVarValue, (O_WRONLY | O_NONBLOCK | O_SYNC));

    /*****************************************************************/
    /* Initialize the global variables for ACSAPI-to-XAPI conversion.*/
    /*****************************************************************/
    if (getenv(XAPI_HOSTNAME) != NULL)
    {
        strcpy(wkGetEnvString, (char*) ((uintptr_t) getenv(XAPI_HOSTNAME)));

        if (strlen(wkGetEnvString) > XAPI_HOSTNAME_SIZE)
        {
            TRMSG("Truncating HOSTNAME from %i to %i characters\n",
                  strlen(wkGetEnvString) ,
                  XAPI_HOSTNAME_SIZE);

            memcpy(pXapicvt->xapiHostname,
                   wkGetEnvString,
                   XAPI_HOSTNAME_SIZE);

            pXapicvt->xapiHostname[XAPI_HOSTNAME_SIZE] = 0;
        }
        else
        {
            strcpy(pXapicvt->xapiHostname, wkGetEnvString);
        }
    }
    else
    {
        strcpy(pXapicvt->xapiHostname, " ");
    }

    goodPortFlag = TRUE;

    if (getenv(XAPI_PORT) != NULL)
    {
        strcpy(wkGetEnvString, (char*) ((uintptr_t) getenv(XAPI_PORT)));

        TRMSG("XAPI_PORT=%s",
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

            TRMSG("wkPort=%i",
                  wkPort);

            if (wkPort > 65535)
            {
                goodPortFlag = FALSE;
            }
            else
            {
                pXapicvt->xapiPort = wkPort;
            }
        }
    }
    else
    {
        goodPortFlag = FALSE;
    }

    if (!(goodPortFlag))
    {
        TRMSG("Invalid or no XAPI_PORT=%s specified; using default",
              wkGetEnvString);

        pXapicvt->xapiPort = XAPI_PORT_DEFAULT;
    }

    if (getenv(XAPI_TAPEPLEX) != NULL)
    {
        strcpy(wkGetEnvString, (char*) ((uintptr_t) getenv(XAPI_TAPEPLEX)));

        if (strlen(wkGetEnvString) > XAPI_TAPEPLEX_SIZE)
        {
            TRMSG("Truncating TAPEPLEX from %i to %i characters\n",
                  strlen(wkGetEnvString) ,
                  XAPI_TAPEPLEX_SIZE);

            memcpy(pXapicvt->xapiTapeplex,
                   wkGetEnvString,
                   XAPI_TAPEPLEX_SIZE);

            pXapicvt->xapiTapeplex[XAPI_TAPEPLEX_SIZE] = 0;
        }
        else
        {
            strcpy(pXapicvt->xapiTapeplex, wkGetEnvString);
        }
    }
    else
    {
        strcpy(pXapicvt->xapiTapeplex, " ");         
    }

    if (getenv(XAPI_SUBSYSTEM) != NULL)
    {
        strcpy(wkGetEnvString, (char*) ((uintptr_t) getenv(XAPI_SUBSYSTEM)));

        if (strlen(wkGetEnvString) > XAPI_SUBSYSTEM_SIZE)
        {
            TRMSG("Truncating SUBSYSTEM from %i to %i characters\n",
                  strlen(wkGetEnvString) ,
                  XAPI_SUBSYSTEM_SIZE);

            memcpy(pXapicvt->xapiSubsystem,
                   wkGetEnvString,
                   XAPI_SUBSYSTEM_SIZE);

            pXapicvt->xapiSubsystem[XAPI_SUBSYSTEM_SIZE] = 0;
        }
        else
        {
            strcpy(pXapicvt->xapiSubsystem, wkGetEnvString);
        }
    }
    else
    {
        if (pXapicvt->xapiTapeplex[0] > ' ')
        {
            memcpy(pXapicvt->xapiSubsystem,
                   pXapicvt->xapiTapeplex,
                   XAPI_SUBSYSTEM_SIZE);

            pXapicvt->xapiSubsystem[XAPI_SUBSYSTEM_SIZE] = 0;

            TRMSG("Defaulting SUBSYSTEM=%s to TAPEPLEX name\n",
                  pXapicvt->xapiSubsystem);
        }
        else
        {
            strcpy(pXapicvt->xapiSubsystem, " ");
        }
    }

    if (getenv(XAPI_VERSION) != NULL)
    {
        strcpy(wkGetEnvString, (char*) ((uintptr_t) getenv(XAPI_VERSION)));

        if (strlen(wkGetEnvString) > XAPI_VERSION_SIZE)
        {
            TRMSG("Truncating VERSION from %i to %i characters\n",
                  strlen(wkGetEnvString) ,
                  XAPI_VERSION_SIZE);

            memcpy(pXapicvt->xapiVersion,
                   wkGetEnvString,
                   XAPI_VERSION_SIZE);

            pXapicvt->xapiVersion[XAPI_VERSION_SIZE] = 0;
        }
        else
        {
            strcpy(pXapicvt->xapiVersion, wkGetEnvString);
        }
    }
    else
    {
        strcpy(pXapicvt->xapiVersion, XAPI_VERS_DEFAULT);

        TRMSG("Defaulting VERSION to %s\n",
              pXapicvt->xapiVersion);
    }

    if (getenv(XAPI_USER) != NULL)
    {
        strcpy(wkGetEnvString, (char*) ((uintptr_t) getenv(XAPI_USER)));

        if (strlen(wkGetEnvString) > XAPI_USER_SIZE)
        {
            TRMSG("Truncating USER from %i to %i characters\n",
                  strlen(wkGetEnvString) ,
                  XAPI_USER_SIZE);

            memcpy(pXapicvt->xapiUser,
                   wkGetEnvString,
                   XAPI_USER_SIZE);

            pXapicvt->xapiUser[XAPI_USER_SIZE] = 0;
        }
        else
        {
            strcpy(pXapicvt->xapiUser, wkGetEnvString);
        }
    }
    else
    {
        strcpy(pXapicvt->xapiUser, " ");
    }

    if (getenv(XAPI_GROUP) != NULL)
    {
        strcpy(wkGetEnvString, (char*) ((uintptr_t) getenv(XAPI_GROUP)));

        if (strlen(wkGetEnvString) > XAPI_GROUP_SIZE)
        {
            TRMSG("Truncating GROUP from %i to %i characters\n",
                  strlen(wkGetEnvString) ,
                  XAPI_GROUP_SIZE);

            memcpy(pXapicvt->xapiGroup,
                   wkGetEnvString,
                   XAPI_GROUP_SIZE);

            pXapicvt->xapiGroup[XAPI_GROUP_SIZE] = 0;
        }
        else
        {
            strcpy(pXapicvt->xapiGroup, wkGetEnvString);
        }
    }
    else
    {
        strcpy(pXapicvt->xapiGroup, " ");
    }

    if (getenv(XAPI_EJECT_TEXT) != NULL)
    {
        strcpy(wkGetEnvString, (char*) ((uintptr_t) getenv(XAPI_EJECT_TEXT)));

        if (strlen(wkGetEnvString) > XAPI_EJECT_TEXT_SIZE)
        {
            TRMSG("Truncating EJECT_TEXT from %i to %i characters\n",
                  strlen(wkGetEnvString) ,
                  XAPI_EJECT_TEXT_SIZE);

            memcpy(pXapicvt->xapiEjectText,
                   wkGetEnvString,
                   XAPI_EJECT_TEXT_SIZE);

            pXapicvt->xapiEjectText[XAPI_EJECT_TEXT_SIZE] = 0;
        }
        else
        {
            strcpy(pXapicvt->xapiEjectText, wkGetEnvString);
        }
    }
    else
    {
        strcpy(pXapicvt->xapiEjectText, " ");
    }

    TRMSG("csi_xapi_conversion_flag=%i, pXapicvt->xapiHostname=%s, pXapicvt->xapiPort=%i\n",
          csi_xapi_conversion_flag,
          pXapicvt->xapiHostname,
          pXapicvt->xapiPort);

    TRMSG("pXapicvt->xapiTapeplex=%s, pXapicvt->xapiSubsystem=%s, pXapicvt->xapiVersion=%s\n",
          pXapicvt->xapiTapeplex,
          pXapicvt->xapiSubsystem,
          pXapicvt->xapiVersion);

    TRMSG("pXapicvt->xapiUser=%s, pXapicvt->xapiGroup=%s, pXapicvt->xapiEjectText=%s\n",
          pXapicvt->xapiUser,
          pXapicvt->xapiGroup,
          pXapicvt->xapiEjectText);

    if ((csi_xapi_conversion_flag) &&
        ((pXapicvt->xapiHostname[0] <= ' ') ||
         (pXapicvt->xapiTapeplex[0] <= ' ')))
    {
        LOGMSG(STATUS_NI_FAILURE, 
               "XAPI conversion but no XAPI HOSTNAME or TAPEPLEX specified");

        return(STATUS_PROCESS_FAILURE);
    }

    /*****************************************************************/
    /* Initialize the xapicfgLock semaphore.                         */
    /*****************************************************************/
    lastRC = sem_init(&pXapicvt->xapicfgLock, 0, 1);

    TRMSG("lastRC=%d from sem_init(addr=%08X)\n",
          lastRC,
          &pXapicvt->xapicfgLock);

    if (lastRC < 0)
    {
        LOGMSG(STATUS_PROCESS_FAILURE, 
               "Could not initialize semaphore");

        return STATUS_PROCESS_FAILURE;
    }

    pXapicvt->ssiPid = csi_pid;

    TRMEM(pXapicvt, sizeof(struct XAPICVT),
          "XAPICVT:\n");
#endif /* XAPI and SSI */

#ifdef ADI

#ifdef SSI /* ssi code */
    /* record the return address of this ssi in csi header (global storage) */
    memset((char *) &csi_ssi_adi_addr, '\0', sizeof(csi_ssi_adi_addr));
    csi_ssi_adi_addr.ssi_identifier     = CSI_NO_SSI_IDENTIFIER;
    csi_ssi_adi_addr.csi_syntax         = CSI_SYNTAX_XDR;
    csi_ssi_adi_addr.csi_proto          = CSI_PROTOCOL_ADI;
    csi_ssi_adi_addr.csi_ctype          = CSI_CONNECT_ADI;
    /*  initialize the client_name for this host */
    if (gethostname((char *) csi_ssi_adi_addr.csi_handle.client_name, 
                             (int) CSI_ADI_NAME_SIZE) != 0) {
        MLOGCSI((STATUS_RPC_FAILURE, st_module,  "gethostname()", 
      MMSG(929, "Undefined hostname")));
        return(STATUS_PROCESS_FAILURE);
    }
    strcpy((char *)csi_client_name, 
        (char *)csi_ssi_adi_addr.csi_handle.client_name);
    csi_ssi_adi_addr.csi_handle.proc = CSI_ACSLM_PROC;;

#else /* csi code */

    /*  initialize the client_name for this CSI */
    if (gethostname((char *) csi_client_name, (int) CSI_ADI_NAME_SIZE) != 0) {
        MLOGCSI((STATUS_RPC_FAILURE, st_module,  "gethostname()", 
      MMSG(929, "Undefined hostname")));
        return(STATUS_PROCESS_FAILURE);
    }
#endif /* SSI */

#else /* if not ADI */

    /* initialize csi rpc services */
    if (csi_svcinit() != STATUS_SUCCESS)
        return(STATUS_PROCESS_FAILURE);
#endif /* ADI */

    /* log completion of initialization per pfs */
    MLOGCSI((STATUS_SUCCESS, st_module,  CSI_NO_CALLEE, 
      MMSG(938, "Initiation Completed")));

    return(STATUS_SUCCESS);
}
