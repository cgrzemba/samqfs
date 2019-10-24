#ifndef lint
static char SccsId[] = "@(#) %filespec: csi_shutdown.c,3 %  (%full_filespec: csi_shutdown.c,3:csrc:1 %)";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Source/System:
 *      csi_shutdown.c  - source file of the CSI module of the ACSSS
 *
 * Name:
 *      csi_shutdown
 *
 * Description:
 *      This routine controls the subprocesses which shut down or discontinue
 *      CSI services as a network server.
 *
 *      Calls csi_logevent() with STATUS_SUCCESS and message 
 *      MSG_INITIATING_TERMINATION to log the beginning of the CSI shutdown.
 *
 *      If NULL != csi_udpxprt (global RPC UDP service transport handle),
 *
 *              Calls pmap_unset() to unmap old csi RPC program, version,
 *              port number rpc udp service port mapper mappings.
 *
 *              Calls pmap_unset() to unmap official RPC program, version,
 *              port number rpc udp service port mapper mappings.
 *
 *              Calls svc_destroy() to discontinue the CSI as an RPC/UDP server
 *              passing it the global UDP transport handle csi_udp_xprt.  
 *              Return status from this call is disregarded.
 *
 *
 *      If NULL != csi_tcpxprt (global RPC TCP service transport handle),
 *
 *              Calls pmap_unset() to unmap old csi RPC program, version,
 *              port number rpc tcp service port mapper mappings.
 *
 *              Calls pmap_unset() to unmap official RPC program, version,
 *              port number rpc tcp service port mapper mappings.
 *
 *              Calls svc_destroy() to discontinue the CSI as an RPC/TCP server
 *              passing it the global TCP transport handle csi_tcp_xprt.
 *              Return status from this call is disregarded.
 *
 *      Calls cl_ipc_destroy() to close IPC connection to the ACSLM module.
 *      Any input pending on this connection is lost.  Return status from this
 *      call is disregarded.
 *
 *      In order to log to the event log any pending storage server request 
 *      connections indicated by entries remaining in the connection queue, 
 *      calls csi_qclean().
 *
 *      Calls csi_logevent() with STATUS_SUCCESS and message
 *      MSG_TERMINATION_COMPLETE.
 *
 * Return Values:
 *
 * Implicit Inputs:
 *      csi_lm_qid      - ID of request queue storing return address information
 *                        for ACSLM storage server requests passed to
 *                        cl_qm_mlocate() and csi_free_qmemb().
 *
 *      csi_udpxprt     - Global RPC/UDP transport handle passed to 
 *                        csi_shutrpcsvc() to shut down UDP RPC service.
 *
 *      csi_tcpxprt     - Global RPC/TCP transport handle passed to 
 *                        csi_shutrpcsvc() to shut down TCP RPC service.
 *
 * Implicit Outputs:
 *
 * Considerations:
 *      Since this is a shutdown process, all errors are irrelevant to the 
 *      to the extent that the process will be terminated anyway; however,
 *      lower level processes will still attempt to log errors in an attempt
 *      to trap any unaccounted for software conditions (bugs?).
 *
 * Module Test Plan:
 *
 * Revision History:
 *      J. A. Wishner       05-Jan-1989     Created.
 *      J. A. Wishner       13-Jun-1989     Installed changes so that only a
 *                                          single program number is used, and
 *                                          RPC transport types differentiated
 *                                          by version number.
 *      J. W. Montgomery    29-Sep-1990     Modified for OSLAN.              
 *      E. A. Alongi        13-Oct-1992     Additions to unmap old RPC program
 *                                          numbers for UDP and TCP.
 *      E. A. Alongi        30-Oct-1992     Added comments to differentiate
 *                                          csi from ssi code.
 *      Anton Vatcky        01-Mar-2002     Relocate signal.h include to avoid
 *                                          SA_RESTART redefined.
 *      Mike Williams       01-Jun-2010     Included rpc/rpc.h and
 *                                          rpc/pmap_clnt.h to remedy warnings.
 *      Joseph Nofi         15-Jun-2011     XAPI support;
 *                                          Added XAPICVT shared memory segment 
 *                                          cleanup.
 *
 */


/*
 *      Header Files:
 */
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <signal.h>

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

#include "csi.h"
#include "cl_pub.h"
#include "ml_pub.h"
#include "cl_ipc_pub.h"
#include "srvcommon.h"

#if defined(XAPI) && defined(SSI)
    #include "xapi/xapi.h"
#endif /* XAPI and SSI */


/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static  char *st_module = "csi_shutdown()";
static  char *st_src = __FILE__;


/*
 *      Procedure Type Declarations:
 */
#undef SELF
#define SELF "csi_shutdown"

void 
csi_shutdown (void)
{

#if defined(XAPI) && defined(SSI)
    int                 lastRC;
    int                 shMemSegId;
    struct XAPICVT     *pXapicvt;
#endif /* XAPI and SSI */

#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module, 0);
#endif /* DEBUG */

    /* log the csi shutdown */
    MLOGCSI((STATUS_SUCCESS, st_module,  CSI_NO_CALLEE, 
      MMSG(969, "Termination Started")));

#ifndef SSI /* csi code */

    /* unmap first, then terminate udp rpc service */
    if ((SVCXPRT *) NULL != csi_udpxprt) {

        /* first unmap old csi RPC program number */
        pmap_unset(CSI_PROGRAM1, CSI_UDP_VERSION);

        /* then unmap official csi RPC program number */
        pmap_unset(CSI_PROGRAM, CSI_UDP_VERSION);
        svc_destroy(csi_udpxprt);
    }

    /* unmap first, then terminate tcp rpc service */
    if ((SVCXPRT *) NULL != csi_tcpxprt) {

        /* first unmap old csi RPC program number */
        pmap_unset(CSI_PROGRAM1, CSI_TCP_VERSION);

        /* then unmap official csi RPC program number */
        pmap_unset(CSI_PROGRAM, CSI_TCP_VERSION);
        svc_destroy(csi_tcpxprt);
    }

#else /* ssi code */

#ifndef ADI
    /* unmap first, then terminate udp rpc service */
    if ((SVCXPRT *) NULL != csi_udpxprt) {
        pmap_unset(csi_ssi_rpc_addr.csi_handle.program, 
            csi_ssi_rpc_addr.csi_handle.version);
        svc_destroy(csi_udpxprt);
    }

    /* unmap first, then terminate tcp rpc service */
    if ((SVCXPRT *) NULL != csi_tcpxprt) {
        pmap_unset(csi_ssi_rpc_addr.csi_handle.program, 
            csi_ssi_rpc_addr.csi_handle.version);
        svc_destroy(csi_tcpxprt);
    }
#endif /* ADI */

#endif /* SSI */


#ifdef ADI
    /* Send SIGHUP to the co-process */
    kill(csi_co_process_pid, SIGHUP);
#endif /* ADI */

    /* cl_select_input() dependency warning, can't do any selects beyond here */
    n_fds = 0;

    /* clean out the connection queue */
    (void) csi_qclean(csi_lm_qid, 0L, csi_fmtlmq_log);

    /* log the completion of termination */
    MLOGCSI((STATUS_SUCCESS,         st_module,  CSI_NO_CALLEE, 
      MMSG(970, "Termination Completed")));

    /* close connection to acslm */
    (void) cl_ipc_destroy();

#if defined(XAPI) && defined(SSI)
    /*****************************************************************/
    /* If SSI and XAPI, then clean up the XAPI shared memory and     */
    /* semaphore resources.                                          */
    /*****************************************************************/
    if (csi_xapi_control_table != NULL)
    {
        /*************************************************************/
        /* Destroy the semaphore created to serialize access to the  */
        /* XAPICFG table.                                            */
        /*************************************************************/
        pXapicvt = (struct XAPICVT*) csi_xapi_control_table;

        lastRC = sem_destroy(&pXapicvt->xapicfgLock);

        TRMSGI(TRCI_STORAGE,
               "lastRC=%d from sem_destroy(addr=%08X)\n",
               lastRC,
               &pXapicvt->xapicfgLock);

        /*************************************************************/
        /* Detach and remove the XAPICFG shared memory segment.      */
        /*************************************************************/
        if (pXapicvt->pXapicfg != NULL)
        {
            lastRC = shmdt((void*) pXapicvt->pXapicfg);

            TRMSGI(TRCI_STORAGE,
                   "lastRC=%d from shmdt(addr=XAPICFG=%08X)\n",
                   lastRC,
                   pXapicvt->pXapicfg);
        }

        lastRC = shmctl(pXapicvt->cfgShMemSegId, IPC_RMID, NULL);

        TRMSGI(TRCI_STORAGE,
               "lastRC=%d from shctl(id=XAPICFG=%d (%08X))\n",
               lastRC,
               pXapicvt->cfgShMemSegId,
               pXapicvt->cfgShMemSegId);

        /*************************************************************/
        /* Detach and remove the XAPICVT shared memory segment.      */
        /*************************************************************/
        shMemSegId = pXapicvt->cvtShMemSegId;

        lastRC = shmdt((void*) csi_xapi_control_table);

        TRMSGI(TRCI_STORAGE,
               "lastRC=%d from shmdt(addr=XAPICVT=%08X)\n",
               lastRC,
               csi_xapi_control_table);

        lastRC = shmctl(shMemSegId, IPC_RMID, NULL);

        TRMSGI(TRCI_STORAGE,
               "lastRC=%d from shctl(id=XAPICVT=%d (%08X))\n",
               lastRC,
               shMemSegId,
               shMemSegId);

        csi_xapi_control_table = NULL;
    }
#endif /* XAPI and SSI */

}

