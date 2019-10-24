/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_main.c                                      */
/** Description:    The XAPI client thread mainline entry.           */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     05/11/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/***END PROLOGUE******************************************************/

/*********************************************************************/
/* Includes:                                                         */
/*********************************************************************/
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "api/defs_api.h"
#include "csi.h"
#include "smccodes.h"
#include "srvcommon.h"
#include "xapi.h"

/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void testIdleStatus(struct XAPICVT *pXapicvt);

static void setStackMode(struct XAPICVT *pXapicvt);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_main                                         */
/** Description:   The XAPI client thread mainline entry.            */
/**                                                                  */
/** This function is called from the child process from              */
/** ssi/csi_rpccall.c after fork().  Each ACSAPI command             */
/** request is processed in its own thread.                          */
/**                                                                  */
/** Initialize the ACSAPI-XAPI thread environment by attaching       */
/** the XAPICVT (XAPI component CVT), and XAPICFG (XAPI tape drive   */
/** configuration table) shared memory segments.                     */
/**                                                                  */
/** There can be a maximim of MAX_XAPIREQE (64) command threads      */
/** (or ACSAPI-XAPI command requests) executing simultaneously.      */
/** Each thread (or ACSAPI-XAPI command request) is assigned an      */
/** XAPIREQE index (the input xapireqIndex) for the duration of      */
/** its processing life.   The assigned XAPIREQE contains status     */
/** and processing information for the ACSAPI-XAPI command request.  */
/**                                                                  */
/** If the input xapireqeIndex is negative, it means that the        */
/** the maximum number of simultaneous ACSAPI-XAPI command requests  */
/** has been exceeded.  In that case we setup a dummy XAPIREQE entry */
/** and return a STATUS_MAX_REQUESTS_EXCEEDED error response.        */
/**                                                                  */
/** If this is the 1st time that the XAPI client is entered          */
/** then initialize the XAPICFG, XAPIDRVTYP, XAPIMEDIA, and          */
/** XAPISCRPOOL tables.                                              */
/**                                                                  */
/** If at least MAX_SCRPOOL_AGE_SECS (1 hour) have elapsed since     */
/** the last scratch subpool table (XAPISCRPOOL) update, then        */
/** update the XAPISCRPOOL table.                                    */ 
/**                                                                  */
/** Create a copy of the XAPICFG for the current command request,    */
/** and anchor the XAPICFG copy to the request's assigned XAPIREQE.  */
/**                                                                  */
/** The ACSAPI request is routed to the appropriate XAPI processor   */
/** according to the input ACSAPI command type code.                 */
/** Upon completion, the XAPIREQE is re-initialized as free for      */
/** subsequent command requests. If the XAPICVT.status indicates     */
/** IDLE_PENDING, and there are no XAPIREQE(s) assigned, then        */
/** the XAPICVT.status is set to IDLE.                               */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_main"

extern int xapi_main(char *pAcsapiBuffer,
                     int   acsapiBufferSize,
                     int   xapireqeIndex,
                     char  processFlag)
{
    int                 lastRC              = STATUS_SUCCESS;
    int                 scrpoolAge;
    time_t              currTime;

    struct XAPIREQE     workXapireqe;

    struct XAPICVT     *pXapicvt            = NULL;
    struct XAPICFG     *pXapicfg            = NULL;
    struct XAPIREQE    *pXapireqe           = NULL;

    REQUEST_TYPE       *pRequest_Type       = (REQUEST_TYPE*) pAcsapiBuffer;
    REQUEST_HEADER     *pRequest_Header     = &(pRequest_Type->generic_request);
    IPC_HEADER         *pIpc_Header         = &(pRequest_Header->ipc_header);
    MESSAGE_HEADER     *pMessage_Header     = &(pRequest_Header->message_header);
    QUERY_REQUEST      *pQuery_Request      = &(pRequest_Type->query_request);
    QU_DRV_CRITERIA    *pQu_Drv_Criteria    = &(pQuery_Request->select_criteria.drive_criteria);

    TRMSGI(TRCI_XAPI,
           "Entered; pAcsapiBuffer=%08X, acsapiBufferSize=%d, "
           "xapireqeIndex=%d, processFlag=%02X; "
           "csi_xapi_control_table=%08X\n",
           pAcsapiBuffer,
           acsapiBufferSize,
           xapireqeIndex,
           processFlag, 
           csi_xapi_control_table);

    /*****************************************************************/
    /* Initialize a work XAPIREQE to return certain                  */
    /* preliminary error conditions.                                 */
    /*****************************************************************/
    xapi_attach_work_xapireqe(NULL,
                              &workXapireqe,
                              pAcsapiBuffer,
                              acsapiBufferSize);

    pXapireqe = &workXapireqe;

    /*****************************************************************/
    /* Locate the shared memory segment for the XAPICVT.             */
    /*****************************************************************/
    pXapicvt = xapi_attach_xapicvt();

    if (pXapicvt == NULL)
    {
        xapi_err_response(pXapireqe,
                          NULL,
                          0,
                          STATUS_SHARED_MEMORY_ERROR);

        return STATUS_SHARED_MEMORY_ERROR;
    }

    if (pXapicvt->startTime == 0)
    {
        time(&(pXapicvt->startTime));

        LOGMSG(STATUS_SUCCESS, 
               "XAPI Start: XAPICVT=%08X, XAPIREQE=%08X, "
               "XAPIDRVTYP=%08X, XAPIMEDIA=%08X, "
               "XAPISCRPOOL=%08X\n",
               pXapicvt,
               &(pXapicvt->xapireqe[0]),
               &(pXapicvt->xapidrvtyp[0]),
               &(pXapicvt->xapimedia[0]),
               &(pXapicvt->xapiscrpool[0]));

        setStackMode(pXapicvt);
    }

    if (xapireqeIndex < 0)
    {
        xapi_err_response(pXapireqe,
                          NULL,
                          0,
                          STATUS_MAX_REQUESTS_EXCEEDED);

        return STATUS_MAX_REQUESTS_EXCEEDED;
    }

    pXapireqe = &(pXapicvt->xapireqe[xapireqeIndex]);

    if (processFlag == XAPI_FORK)
    {
        pXapireqe->xapiPid = getpid();
    }
    else
    {
        pXapireqe->xapiPid = pXapicvt->ssiPid;
    }

    /****************************************************************/
    /* If this is a QUERY DRIVE ALL request, then rebuild the       */
    /* XAPICFG drive configuration table before we begin            */
    /* processing the query:  We treat the QUERY DRIVE ALL ACSAPI   */
    /* command as the equivalent of a VMCLIENT/SMC RESYNCH command. */
    /****************************************************************/
    if ((pMessage_Header->command == COMMAND_QUERY) &&
        (pQuery_Request->type == TYPE_DRIVE) &&
        (pQu_Drv_Criteria->drive_count == 0))
    {
        pXapicvt->updateConfig = TRUE;

        TRMSGI(TRCI_XAPI, 
               "QUERY DRIVE ALL request; setting updateConfig\n");
    }

    /****************************************************************/
    /* If MAX_SCRPOOL_AGE_SECS since the XAPISCRPOOL was updated,   */
    /* then setup to refresh.                                       */
    /****************************************************************/
    time(&currTime);
    scrpoolAge = currTime - pXapicvt->scrpoolTime;

    if (scrpoolAge > MAX_SCRPOOL_AGE_SECS)
    {
        pXapicvt->updateScrpool = TRUE;

        TRMSGI(TRCI_XAPI, 
               "scrpoolAge=%d, setting updateScrpool\n");
    }

    /*****************************************************************/
    /* Get exclusive access to the XAPICFG drive configuration       */
    /* table and copy it (and create the original if it does not     */
    /* yet exist).                                                   */
    /*                                                               */
    /* From now on, this thread will only access its copy of         */
    /* the XAPICFG drive configuration table (excepting those        */
    /* threads that perform a drive configuration table update       */
    /* below under exclusive ownership of the semaphore).            */
    /*****************************************************************/
    sem_wait(&pXapicvt->xapicfgLock);

    /*****************************************************************/
    /* While holding the configuration table lock, test if           */
    /* the XAPIDRVTYP and XAPIMEDIA tables need to be initialized.   */
    /*                                                               */
    /* If either is rebuilt, then also force a rebuild of the        */
    /* XAPICFG drive configuration table (if the XAPICFG table       */
    /* was previously built without the XAPIDRVTYP table, then it    */
    /* will not contain the correct ACSAPI drive type codes).        */
    /*****************************************************************/
    if (pXapicvt->xapidrvtypCount == 0)
    {
        lastRC = xapi_drvtyp(pXapicvt,
                             pXapireqe);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d after xapi_drvtyp()\n",
               lastRC);

        if (lastRC == STATUS_SUCCESS)
        {
            pXapicvt->updateConfig = TRUE;
        }
    }

    if ((pXapicvt->xapimediaCount == 0) &&
        (lastRC != STATUS_NI_FAILURE))
    {
        lastRC = xapi_media(pXapicvt,
                            pXapireqe);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d after xapi_media()\n",
               lastRC);

        if (lastRC == STATUS_SUCCESS)
        {
            pXapicvt->updateConfig = TRUE;
        }
    }

    if (((pXapicvt->scrpoolTime == 0) ||
         (pXapicvt->updateScrpool)) &&
        (lastRC != STATUS_NI_FAILURE))
    {
        lastRC = xapi_scrpool(pXapicvt,
                              pXapireqe,
                              XAPI_FORK);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d after xapi_scrpool()\n",
               lastRC);
    }

    /*****************************************************************/
    /* If there is no XAPICFG drive configuration table, or a drive  */
    /* configuration rebuild is indicated, then invoke the XAPI      */
    /* configuration process.                                        */
    /*****************************************************************/
    if (((pXapicvt->pXapicfg == NULL) ||
         (pXapicvt->updateConfig)) &&
        (lastRC != STATUS_NI_FAILURE))
    {
        lastRC = xapi_config(pXapicvt,
                             pXapireqe);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d after xapi_config()\n",
               lastRC);
    }

    /*****************************************************************/
    /* Copy the XAPICFG for use by this thread.                      */
    /*****************************************************************/
    if (pXapicvt->pXapicfg != NULL)
    {
        /*************************************************************/
        /* Locate the shared memory segment for the XAPICFG drive    */
        /* configuration table.                                      */
        /*************************************************************/
        pXapicfg = xapi_attach_xapicfg(pXapicvt);

        if (pXapicfg == NULL)
        {
            xapi_err_response(pXapireqe,
                              NULL,
                              0,
                              STATUS_SHARED_MEMORY_ERROR);

            sem_post(&pXapicvt->xapicfgLock);

            return STATUS_SHARED_MEMORY_ERROR;
        }

        pXapireqe->pXapicfg = (struct XAPICFG*) malloc(pXapicvt->xapicfgSize);

        TRMSGI(TRCI_STORAGE,
               "malloc XAPIREQE.pXapicfg=%08X, len=%i\n",
               pXapireqe->pXapicfg,
               pXapicvt->xapicfgSize);

        pXapireqe->xapicfgSize = pXapicvt->xapicfgSize;
        pXapireqe->xapicfgCount = pXapicvt->xapicfgCount;

        memcpy((char*) pXapireqe->pXapicfg,
               (char*) pXapicfg,
               pXapireqe->xapicfgSize);
    }

    /*****************************************************************/
    /* Release access to the XAPICFG drive configuration table.      */
    /*****************************************************************/
    sem_post(&pXapicvt->xapicfgLock);

    TRMEMI(TRCI_XAPI,
           pXapireqe, sizeof(struct XAPIREQE),
           "XAPIREQE; index=%d, seqNumber=%d:\n",
           xapireqeIndex,
           pXapireqe->seqNumber);

    pIpc_Header = (IPC_HEADER*) pAcsapiBuffer;
    pRequest_Header = (REQUEST_HEADER*) pAcsapiBuffer;
    pMessage_Header = &(pRequest_Header->message_header);

    LOG_ACSAPI_RECV(STATUS_SUCCESS, 
                    pXapireqe->pAcsapiBuffer, 
                    pXapireqe->acsapiBufferSize,
                    "ACSAPI input\n");

    switch (pMessage_Header->command)
    {
    
    case COMMAND_AUDIT:
        /*************************************************************/
        /* AUDIT includes AUDIT ACS, AUDIT LSM, AUDIT PANEL,         */
        /* AUDIT SERVER, and AUDIT SUBPANEL.                         */
        /*                                                           */
        /* AUDIT commands not supported in the XAPI but              */
        /* require additional error response handling.               */
        /*************************************************************/

        lastRC = xapi_audit(pXapicvt,
                            pXapireqe);

        break;

    case COMMAND_CANCEL:
        /*************************************************************/
        /* CANCEL commands are processed locally by the XAPI         */
        /* process and do not result in communication with the       */
        /* HTTP server.                                              */
        /*************************************************************/

        lastRC = xapi_cancel(pXapicvt,
                             pXapireqe);

        break;

    case COMMAND_CHECK_REGISTRATION:
        /*************************************************************/
        /* CHECK_REGISTRATION commands are not supported in the XAPI */
        /* but require additional error response handling.           */
        /*************************************************************/

        lastRC = xapi_check_reg(pXapicvt,
                                pXapireqe);

        break;

    case COMMAND_CLEAR_LOCK:
        /*************************************************************/
        /* CLEAR_LOCK includes CLEAR_LOCK_DRIVE and                  */
        /* CLEAR_LOCK_VOLUME.                                        */
        /*************************************************************/

        lastRC = xapi_clr_lock(pXapicvt,
                               pXapireqe);

        break;

        /*************************************************************/
        /* COMMAND commands are internal ACSAPI commands and         */
        /* are never forwarded to the SSI/XAPI process.              */
        /*************************************************************/

    case COMMAND_DEFINE_POOL:
        /*************************************************************/
        /* DEFINE_POOL commands are not supported in the XAPI        */
        /* but return information as if it were a QUERY_POOL         */
        /* command.                                                  */
        /*************************************************************/

        lastRC = xapi_define_pool(pXapicvt,
                                  pXapireqe);

        break;

    case COMMAND_DELETE_POOL:
        /*************************************************************/
        /* DELETE_POOL commands are not supported in the XAPI        */
        /* but require additional error response handling.           */
        /*************************************************************/

        lastRC = xapi_delete_pool(pXapicvt,
                                  pXapireqe);

        break;

    case COMMAND_DISMOUNT:

        lastRC = xapi_dismount(pXapicvt,
                               pXapireqe);

        break;

    case COMMAND_DISPLAY:
        /*************************************************************/
        /* ACSAPI-type DISPLAY commands are not supported in the     */
        /* XAPI but require additional error response handling.      */
        /*************************************************************/

        lastRC = xapi_display(pXapicvt,
                              pXapireqe);

        break;

    case COMMAND_EJECT:
        /*************************************************************/
        /* COMMAND_EJECT includes both EJECT and XEJECT (even though */
        /* their parameter lists are different, they use the same    */
        /* COMMAND code!).  EJECT specifies volser(s), while         */
        /* XJECT specifies volrange(s).                              */
        /*                                                           */
        /* The MESSAGE_HEADER.extended_options RANGE flag determines */
        /* whether the request is EJECT or XEJECT.                   */
        /*************************************************************/

        if (pMessage_Header->extended_options & RANGE)
        {
            lastRC = xapi_xeject(pXapicvt,
                                 pXapireqe);

        }
        else
        {
            lastRC = xapi_eject(pXapicvt,
                                pXapireqe);
        }

        break;

    case COMMAND_ENTER:
        /*************************************************************/
        /* COMMAND_ENTER includes both ENTER and VENTER.             */
        /* VENTER is for entereing unlabelled (or "virtual"          */
        /* labelled) cartridges into the ACS.  VENTER is not         */
        /* supported by the XAPI (not can it be supported).          */
        /*                                                           */
        /* The MESSAGE_HEADER.extended_options VIRTUAL flag          */
        /* determines whether the request is EJECT or XEJECT.        */
        /*************************************************************/

        if (pMessage_Header->extended_options & VIRTUAL)
        {
            lastRC = xapi_venter(pXapicvt,
                                 pXapireqe);
        }
        else
        {
            lastRC = xapi_enter(pXapicvt,
                                pXapireqe);
        }

        break;

    case COMMAND_IDLE:
        /*************************************************************/
        /* IDLE commands are processed locally by the XAPI           */
        /* process and do not result in communication with the       */
        /* HTTP server.                                              */
        /*************************************************************/

        lastRC = xapi_idle(pXapicvt,
                           pXapireqe);

        break;

    case COMMAND_LOCK:
        /*************************************************************/
        /* LOCK includes LOCK DRIVE and LOCK VOLUME.                 */
        /*************************************************************/

        lastRC = xapi_lock(pXapicvt,
                           pXapireqe);

        break;

    case COMMAND_MOUNT:

        lastRC = xapi_mount(pXapicvt,
                            pXapireqe);

        break;

    case COMMAND_MOUNT_PINFO:

        lastRC = xapi_mount_pinfo(pXapicvt,
                                  pXapireqe);

        break;

    case COMMAND_MOUNT_SCRATCH:

        lastRC = xapi_mount_scr(pXapicvt,
                                pXapireqe);

        break;

    case COMMAND_QUERY:
        /*************************************************************/
        /* QUERY includes QUERY ACS, QUERY CAP, QUERY CLEAN,         */
        /* QUERY DRIVE, QUERY DRIVE GROUP, QUERY LSM,                */
        /* QUERY MIXED MEDIA INFO, QUERY_MOUNT, QUERY MOUNT SCRATCH, */
        /* QUERY MOUNT SCRATCH PINFO, QUERY POOL, QUERY_PORT,        */
        /* QUERY REQUEST, QUERY SCRATCH, QUERY SERVER,               */
        /* QUERY SUBPOOL NAME, and QUERY VOLUME.                     */
        /*                                                           */
        /* QUERY CLEAN commands are not supported in the XAPI (and   */
        /* result in STATUS_UNSUPPORTED_OPTION in xapi_query.c).     */
        /*                                                           */
        /* QUERY PORT commands are not supported in the XAPI (and    */
        /* result in STATUS_UNSUPPORTED_OPTION in xapi_query.c).     */
        /*************************************************************/

        lastRC = xapi_query(pXapicvt,
                            pXapireqe);

        break;

    case COMMAND_QUERY_LOCK:
        /*************************************************************/
        /* QUERY LOCK includes QUERY LOCK DRIVE and                  */
        /* QUERY LOCK VOLUME.                                        */
        /*************************************************************/

        lastRC = xapi_qlock(pXapicvt,
                            pXapireqe);

        break;

    case COMMAND_REGISTER:
        /*************************************************************/
        /* REGISTER commands are not suppored in the XAPI            */
        /* but require additional error response handling.           */
        /*************************************************************/

        lastRC = xapi_register(pXapicvt,
                               pXapireqe);

        break;

    case COMMAND_SET_CAP:
        /*************************************************************/
        /* SET_CAP commands are not supported in the XAPI            */
        /* but require additional error response handling.           */
        /*************************************************************/

        lastRC = xapi_set_cap(pXapicvt,
                              pXapireqe);

        break;

    case COMMAND_SET_CLEAN:
        /*************************************************************/
        /* SET_CLEAN commands are not supported in the XAPI          */
        /* but require additional error response handling.           */
        /*************************************************************/

        lastRC = xapi_set_clean(pXapicvt,
                                pXapireqe);

        break;

    case COMMAND_SET_SCRATCH:
        /*************************************************************/
        /* SET_SCRATCH also performs unscratch.                      */
        /*************************************************************/

        lastRC = xapi_set_scr(pXapicvt,
                              pXapireqe);

        break;

    case COMMAND_START:
        /*************************************************************/
        /* START commands are processed locally by the XAPI          */
        /* process and do not result in communication with the       */
        /* HTTP server.                                              */
        /*************************************************************/

        lastRC = xapi_start(pXapicvt,
                            pXapireqe);

        break;

        /*************************************************************/
        /* STATE commands are internal ACSAPI commands and           */
        /* are never forwarded to the SSI/XAPI process.              */
        /*                                                           */
        /* STATUS commands are internal ACSAPI commands and          */
        /* are never forwarded to the SSI/XAPI process.              */
        /*                                                           */
        /* TYPE commands are internal ACSAPI commands and            */
        /* are never forwarded to the SSI/XAPI process.              */
        /*************************************************************/

    case COMMAND_UNLOCK:
        /*************************************************************/
        /* UNLOCK includes UNLOCK DRIVE and UNLOCK VOLUME.           */
        /*************************************************************/

        lastRC = xapi_unlock(pXapicvt,
                             pXapireqe);

        break;

    case COMMAND_UNREGISTER:
        /*************************************************************/
        /* UNREGISTER commands are not supported in the XAPI         */
        /* but require additional error response handling.           */
        /*************************************************************/

        lastRC = xapi_unregister(pXapicvt,
                                 pXapireqe);

        break;

    case COMMAND_VARY:
        /*************************************************************/
        /* VARY includes VARY ACS, VARY CAP, VARY DRIVE,             */
        /* VARY_LSM, and VARY PORT.                                  */
        /*                                                           */
        /* VARY commands are not supported in the XAPI but           */
        /* require additional error response handling.               */
        /*************************************************************/

        lastRC = xapi_vary(pXapicvt,
                           pXapireqe);

        break;

    default:

        xapi_err_response(pXapireqe,
                          NULL,
                          0,
                          STATUS_UNSUPPORTED_COMMAND); 

        lastRC = STATUS_UNSUPPORTED_COMMAND;
    } 

    if (pXapireqe->pXapicfg != NULL)
    {
        TRMSGI(TRCI_STORAGE,
               "free XAPIREQE.pXapicfg=%08X, len=%i\n",
               pXapireqe->pXapicfg,
               pXapireqe->xapicfgSize);

        free(pXapireqe->pXapicfg);

        pXapireqe->pXapicfg = NULL;
        pXapireqe->xapicfgSize = 0;
        pXapireqe->xapicfgCount = 0;
    }

    pXapireqe->requestFlag |= XAPIREQE_END;
    testIdleStatus(pXapicvt);

    return lastRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: testIdleStatus                                    */
/** Description:   Test IDLE status.                                 */
/**                                                                  */
/** Determine if the XAPICVT.status field can be transitioned        */
/** from XAPI_IDLE_PENDING to XAPI_IDLE.                             */
/**                                                                  */
/** NOTE: There is no essential difference between XAPI_IDLE_PENDING */
/** and XAPI_IDLE state (all commands except CANCEL, IDLE, QUERY,    */
/** QUERY_LOCK, START, and VARY return STATUS_IDLE_PENDING): the     */
/** 2 states are for compatibility with ACSLS and LibStation.        */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "testIdleStatus"

static void testIdleStatus(struct XAPICVT *pXapicvt)
{
    int                 i;
    struct XAPIREQE    *pCurrXapireqe;
    char                activeRequestFlag   = FALSE;

    if (pXapicvt->status != XAPI_IDLE_PENDING)
    {
        return;
    }

    for (i = 0;
        i < MAX_XAPIREQE;
        i++)
    {
        pCurrXapireqe = (struct XAPIREQE*) &(pXapicvt->xapireqe[i]);

        if ((pCurrXapireqe->requestFlag & XAPIREQE_START) &&
            (!(pCurrXapireqe->requestFlag & XAPIREQE_END)))
        {
            activeRequestFlag == TRUE;

            break;
        }
    }

    if ((!(activeRequestFlag)) &&
        (pXapicvt->status == XAPI_IDLE_PENDING))
    {
        pXapicvt->status = XAPI_IDLE;
    }

    return;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: setStackMode                                      */
/** Description:   Set TCP/IP stack mode to IPv4 or IPv6.            */
/**                                                                  */
/** Use socket() call(s) to set the TCP/IP stack attributes as       */
/** either AF_INET (IPv4 only) of AF_INET6 (IPv6 capable stack).     */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "setStackMode"

static void setStackMode(struct XAPICVT *pXapicvt)
{
    auto int                 socketId;

    TRMSGI(TRCI_XTCPIP,
           "Entered; ipafFlag=%i\n",
           pXapicvt->ipafFlag);

    socketId = socket(AF_INET6,
                      SOCK_STREAM,
                      IPPROTO_TCP);

    if (socketId < 0)
    {
        socketId = socket(AF_INET,
                          SOCK_STREAM,
                          IPPROTO_TCP);

        if (socketId < 0)
        {
            pXapicvt->ipafFlag = AF_UNSPEC;
        }
        else
        {
            pXapicvt->ipafFlag = AF_INET;
            close(socketId);
        }
    }
    else
    {
        pXapicvt->ipafFlag = AF_INET6;
        close(socketId);
    }

    TRMSGI(TRCI_XTCPIP,
           "Exit; ipafFlag=%i, AF_INET=%i, AF_INET6=%i\n",
           pXapicvt->ipafFlag,
           AF_INET,
           AF_INET6);

    return;
}




