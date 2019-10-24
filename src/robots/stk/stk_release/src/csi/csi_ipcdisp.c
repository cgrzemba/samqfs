static char SccsId[] = "@(#) %full_name:  1/csrc/csi_ipcdisp.c/3 %";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *
 *      csi_ipcdisp()
 *
 * Description:
 *
 *      Inter Process Communications packet dispatcher.  Receives packets off
 *      of an IPC socket, determines the source of the packet, and based on
 *      that determination, routes the packet to the intended destination.
 *
 *      o on first pass only, initialize duplicate message testing structure
 *      o read a packet from the ipc socket, return status if error     
 *      o dispatch packets to destination appropriate to caller's type
 *
 *      o DEFAULT INPUT:  module type-default case:     
 *          # if compiled as a csi      
 *              - only allow types for acslm input or acspd input
 *              - log an error
 *          # else compiled as an ssi   
 *              - allow any non-acslm, non-acspd module type to act as an acslm
 *                (a test application and test group requirement)
 *              - log an error if the type is not a legal TYPE value
 *          # endif
 *      o ACSLM INPUT:  module type acslm case:                 
 *          o throw out packets if too small
 *          # if compiled as an CSI
 *              - set packet direction (for packet tracing) "From ACSLM"
 *          # else compiled as an SSI
 *              - set packet direction (for packet tracing) "From Client"
 *          # end
 *          o throw out duplicate packets
 *          # if compiled as a CSI
 *              - get the network return address (csi header) from the queue
 *          # else compiled as an SSI
 *              - puts the ipc return address (ipc_header) on the queue
 *              - extracts its own previously initialized global network address
 *              - places the queue identifier into the network address header
 *          # endif
 *          o initialize network packet description buffer
 *          o if packet tracing is enabled
 *              - format network address and port number
 *              - perform packet trace
 *          o send packet on the network
 *              - success STATUS_SUCCESS,STATUS_PENDING,STATUS_NI_TIMEDOUT
 *              - return all other statuses as errors 
 *          # if compiled as an SSI
 *              - if a final response, delete the packet from the queue
 *                      
 *    #if compiled with -DADI  (OSLAN CSI)
 *      o CSI co-process INPUT:  module type csi case:                  
 *          o throw out packets if too small
 *          # if compiled as an CSI
 *              - set packet direction (for packet tracing) "To ACSLM"
 *          # else compiled as an SSI
 *              - set packet direction (for packet tracing) "To Client"
 *          # endif
 *          o throw out duplicate packets
 *          # if compiled as a CSI
 *              - puts the ipc return address (ipc_header) on the queue
 *              - extracts its own previously initialized global network address
 *              - places the queue identifier into the network address header
 *          # else compiled as an SSI
 *              - get the network return address (csi header) from the queue
 *          # endif
 *          o initialize network packet description buffer
 *          o if packet tracing is enabled
 *              - format network address and port number
 *              - perform packet trace
 *          o Replace CSI_HEADER with IPC_HEADER.
 *          # if compiled as a CSI
 *              - send packet via IPC to the ACSLM.
 *          # else compiled as an SSI
 *              - send packet via IPC to the client application.
 *          # endif
 *          # if compiled as an SSI
 *              - if a final response, delete the packet from the queue
 *          # endif
 *    #endif    -DADI  (OSLAN CSI)
 *                      
 *
 *      Returns STATUS_SUCCESS.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS              - Successfully sent response to SSI.
 *      STATUS_IPC_FAILURE          - Could not read ACSLM response packet.
 *      STATUS_QUEUE_FAILURE        - Could not get return address from queue.
 *      STATUS_MESSAGE_TOO_SMALL    - Message too small to be intelligible
 *      STATUS_INVALID_TYPE         - Type code is not TYPE_LM
 *      STATUS_RPC_FAILURE          - Failure occurred in RPC mechanisms.
 *      STATUS_NI_TIMEDOUT          - Timeout trying to send response to SSI.
 *      STATUS_PENDING              - Input pending, packet queue'd
 *
 * Implicit Inputs:
 *
 *      NONE
 *
 * Implicit Outputs:
 *
 *      NONE.
 *
 * Considerations:
 *
 *      With reference to the conditional compilation of a CSI or an SSI:
 *
 *              The SSI & CSI work different in queueing return addresses.
 *              The SSI takes an acslm format storage server packet from its
 *              application, converts it to a csi format packet and places 
 *              it on the network, sending it to the csi.  It is saving 
 *              ipc_headers on its connection queue, not csi_header structures
 *              as is the case when compiling a csi.
 *
 *      If compiled as an SSI, allows other modules to send, if compiled as
 *      a csi, then only TYPE_PD and TYPE_LM allowed.
 *
 *      NONE
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner       12-Jan-1989     Created.
 *      J. A. Wishner       06-Jun-1989     Modified to handle new parameters
 *                                          for csi_net_send, and for new
 *                                          return status's, STATUS_PENDING,
 *                                          and STATUS_NI_TIMEDOUT.
 *      R. P. Cushman       09-Apr-1990     Added conditonally compiled code
 *                                          for routing Pdaemon (TYPE_PD) pkts.
 *      J. S. Alexander     29-May-1990     Added #ifdef  STATS.
 *      J. W. Montgomery    15-Aug-1990     Use V0_MESSAGE_HEADER for size
 *                                          comparisons.
 *      J. A. Wishner       22-Sep-1990     Created this from csi_lminput.c
 *                                          Is a dispatch of IPC input.
 *                                          Replaces csi_lminput.c.  New setting
 *                                          sets netbufp->service_type.
 *                                          Changed interface to csi_ptrace.
 *      H. I. Grapek        30-Aug-1991     fixed lint errors. 
 *      J. A. Wishner       15-Oct-1991     Complete mods release 3 (version 2).
 *      E. A. Alongi        30-Jul-1992     Modifications for minimum version
 *                                          supported and csi_trace_flag.
 *      E. A. Alongi        22-Sep-1992     Modifications to support access
 *                                          control in adi csi.
 *      E. A. Alongi        30-Oct-1992     Added ifdef to conditionally
 *                                          compile in code not used for CSC's.
 *                                          Also replaced bzero and bcopy with
 *                                          memset and memcpy respectively.
 *      E. A. Alongi        30-Nov-1992     Changes resulting from ac code
 *                                          review and from running flexelint.
 *      Emanuel A. Alongi   03-Jun-1993     Added case label VERSION4 under
 *                                          TYPE_LM and TYPE_CO_CSI module_type
 *                                          switch to determine version
 *                                          dependent size for packet too small
 *                                          test (no change to REQUEST_HEADER
 *                                          in R5). Eliminated unnecessary 
 *                                          #define ADI from adi access control
 *                                          code block.  Added code to prohibit
 *                                          access control if packet is VERSION0
 *                                          Changed name of access control
 *                                          function that returns ADI host name.
 *                                          Determine minimum version supported
 *                                          dynamically.
 *      Alec Sharp          07-Sep-1993     Set the host_id field in the
 *                                          ipc_header.
 *      Emanuel A. Alongi   01-Oct-1993     Cleaned up flint detected errors.
 *      Emanuel A. Alongi   04-Oct-1993     Added errno.h back in.
 *      Emanuel A. Alongi   25-Oct-1993     Eliminated all ACSPD code.
 *      Emanuel A. Alongi   09-Nov-1993     Corrected error introduced after an
 *                                          earlier flint session on this file.
 *      Emanuel A. Alongi   12-Nov-1993     Corrected flint detected errors.
 *      Van Lepthien        20-Apr-2001     Changed to return STATUS_PENDING
 *                                          from cl_ipc_read back to caller.
 *      Joseph Nofi         15-Jun-2011     XAPI support;
 *                                          If csi_xapi_conversion_flag set, then
 *                                          fork() to xapi_main instead of calling
 *                                          RPC clntcall.
 *      Joseph Nofi         01-Jan-2013     I5974428;
 *                                          Add waitpid(-1) after each fork() to 
 *                                          cleanup <defunct> zombie processes 
 *                                          before termination. 
 */


/*
 *      Header Files:
 */
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/wait.h>

#ifdef ADI
#include <stdlib.h>
#endif

#include "cl_pub.h"
#include "ml_pub.h"
#include "cl_ipc_pub.h"
#include "csi.h"
#include "dv_pub.h"
#include "srvcommon.h"

#if defined(XAPI) && defined(SSI)
    #include "xapi/xapi.h"
#endif /* XAPI and SSI */

#ifdef NOT_CSC /* product code - not intended for CSC developers. */
#include "cl_ac_pub.h"
#endif

/*
 *      Defines, Typedefs and Structure Definitions:
 */
/* convert network address to portable format 
 * set pointer to packet start
 * put on csi header
 * set rest of network buffer variables
 */                     

#ifndef ADI /* the rpc specific macro */
#define PREP_NETBUF(cs_hdr, netaddr, type, size)                              \
    do {                                                                      \
        memcpy((char *) &netaddr,                         \
            (const char*) &cs_hdr.csi_handle.raddr.sin_addr,          \
                                  sizeof(unsigned long)); \
        net_datap = CSI_PAK_NETDATAP(netbufp);                                \
        * (CSI_HEADER *)net_datap = cs_hdr;                                   \
        netbufp->size = size + sizeof(CSI_HEADER) - sizeof(IPC_HEADER);       \
        netbufp->packet_status = CSI_PAKSTAT_INITIAL;                         \
        netbufp->translated_size = 0;                                         \
        netbufp->offset = CSI_PAK_NETOFFSET;                                  \
        netbufp->service_type = type;                                         \
        netbufp->q_mgmt.xmit_tries = 0;                                       \
    } while (0)

#else /* the ADI specific macro */
#define PREP_ADI_NETBUF(cs_hdr, type, size)                                   \
    do {                                                                      \
        net_datap = CSI_PAK_NETDATAP(netbufp);                                \
        * (CSI_HEADER *)net_datap = cs_hdr;                                   \
        netbufp->size = size + sizeof(CSI_HEADER) - sizeof(IPC_HEADER);       \
        netbufp->packet_status = CSI_PAKSTAT_INITIAL;                         \
        netbufp->translated_size = 0;                                         \
        netbufp->offset = CSI_PAK_NETOFFSET;                                  \
        netbufp->service_type = type;                                         \
        netbufp->q_mgmt.xmit_tries = 0;                                       \
    } while (0)
#endif /* ADI */

/*
 *      Global and Static Variable Declarations:
 */
static  char *st_module = "csi_ipcdisp()";

/*
 * To test for duplicate packets, used by st_isaduppak()
 */
static  struct st_dup_test {
    IPC_HEADER ipc_header;
    unsigned char message_options;
} st_dup_test;


/*
 *      Procedure Type Declarations:
 */

static BOOLEAN st_isaduppak (IPC_HEADER *ipc_hdrp, unsigned char msg_opt);

#if defined(XAPI) && defined(SSI)
    static struct XAPIREQE *st_xapi_queue_req(char *pAcsapiBuffer, 
                                              int   acsapiBufferSize,
                                              int  *pXapireqeIndex);
#endif /* XAPI and SSI */

#undef SELF
#define SELF "csi_ipcdisp"

STATUS 
csi_ipcdisp (
    CSI_MSGBUF *netbufp               /* network/ipc communications buffer */
)
{

    STATUS          status;             /* status */
    IPC_HEADER      ipc_header;         /* saves ipc header */
    CSI_HEADER      cs_hdr;          /* used for csi header copy to packet */
    char           *ipc_datap;          /* start of lm data in ipc pak buffer */
    char           *net_datap;          /* start of net data in net pak buffer*/

#ifndef SSI
    CSI_HEADER     *cs_hdr_qp;       /* ptr to csi header on a queue */
#endif

#if defined(XAPI) && defined(SSI)
    pid_t           pid;
    pid_t           termPid;
    int             xapireqeIndex;
    int             xapiRC;
    int             lastRC;
    struct XAPICVT *pXapicvt;
    struct XAPIREQE *pXapireqe;
#endif /* XAPI and SSI */

    int             size;               /* 2 way variable for # bytes read */
    int             vers_dep_size;      /* for version dependent size test */
    unsigned char   msg_opt;            /* saves message options */
    char            port_str[CSI_NAME_SIZE+1];    /* string port number */
    char           *directp;            /* string describing packet direction */
    static BOOLEAN  init_done = FALSE;  /* FALSE first pass */
    long        minimum_version;    /* for minimum version supported */

#if defined(ADI) || defined(SSI)
    QM_MID          member_id;          /* queue member id */
#endif

#ifdef ADI
#ifdef SSI
    IPC_HEADER     *ipc_header_qp;
#endif
    CSI_HEADER     *cs_hdrp;
    char           *lmp;
    IPC_HEADER     *ip;
    CSI_REQUEST_HEADER *csi_req_hdr;    /* to get access to message header */
    char           appl_socket[SOCKET_NAME_SIZE+1]; /* output socket */

#ifdef NOT_CSC             /* product code - not intended for CSC developers. */
    char           *client_name;        /* if not null, pointer to adi client
                                         * name returned by cl_ac_get_accessid()
                     */
#endif /* NOT_CSC */

#else /* ~ADI: the rpc specific delclarations */
    unsigned long   netaddr;            /* buffer for net address */
    char            netaddr_str[CSI_NAME_SIZE+1]; /* string net address */
#endif /* ADI */



#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module, (unsigned long) 1, (unsigned long) netbufp);
#endif /* DEBUG */

    /*
     * setup
     */

    /* get minimum version supported */
    if (dv_get_number(DV_TAG_ACSLS_MIN_VERSION, &minimum_version) !=
              STATUS_SUCCESS || minimum_version < (long)VERSION0 ||
                        minimum_version > (long)VERSION_LAST ) {
    minimum_version = (long) VERSION_MINIMUM_SUPPORTED;
    }

    /* zero out duplicate message testing structure */
    if (FALSE == init_done) {
        memset((char *) &st_dup_test, '\0', sizeof(st_dup_test));
        init_done = TRUE;
    }

    /* read a packet from the ipc socket */
    ipc_datap = (char*) CSI_PAK_IPCDATAP(netbufp);
    size = netbufp->maxsize;
    status = cl_ipc_read((char*)ipc_datap, &size);
    switch (status)
    {
    case STATUS_SUCCESS:
        break;
    case STATUS_PENDING:
        return (status);
    default:
        MLOGCSI((status, st_module, "cl_ipc_read()", 
            MMSG(980, "Cannot read message from ACSLM :discarded")));
        return(STATUS_IPC_FAILURE);
    }

    ipc_header = * (IPC_HEADER *)ipc_datap;

#ifdef DEBUG
    MLOGDEBUG(0,
      (MMSG(495, "%s: %s: Packet type = %d"), CSI_LOG_NAME, st_module,
      (int)ipc_header.module_type));
#endif /* DEBUG */

#ifdef STATS
    pe_collect(0,"csi_ipcdisp: read_lm          ");
#endif /* STATS */


    switch (ipc_header.module_type) { /* ipc input dispatched by module type */

    default:

        /* handle packets incoming from undesignated ipc sources */
        /* check for invalid module type */

#ifndef SSI /* csi code */

        /* if a csi, only acslm or acspd allowed */
        MLOGCSI((STATUS_INVALID_TYPE, st_module, CSI_NO_CALLEE, 
      MMSG(954, "Unsupported module type %d detected:discarded"), ipc_header.module_type));
        return(STATUS_INVALID_TYPE);

#else /* ssi code */
        /* if ssi, allow all legal types, makes it a general use tool */
        if (ipc_header.module_type <= TYPE_FIRST
                                    || ipc_header.module_type >= TYPE_LAST) {
            MLOGCSI((STATUS_INVALID_TYPE, st_module, CSI_NO_CALLEE, 
          MMSG(954, "Unsupported module type %d detected:discarded"),
                                        ipc_header.module_type));
            return(STATUS_INVALID_TYPE);
        }
        /* correct ipc header to acslm module type and fall-thru */
        ipc_header.module_type = TYPE_LM;
#endif /* SSI */

    case TYPE_LM:
        /* handle packets incoming from the acslm module */

        /* determine version dependent size for packet too small test */
        switch (minimum_version) {
        case VERSION0:
            vers_dep_size = sizeof(V0_REQUEST_HEADER);
            break;

        case VERSION1:
            vers_dep_size = sizeof(V1_REQUEST_HEADER);
            break;

        case VERSION2:
        case VERSION3:       /* no change to the size of REQUEST_HEADER in R4 */
        case VERSION4:       /* no change to the size of REQUEST_HEADER in R5 */
            vers_dep_size = sizeof(REQUEST_HEADER);
            break;

        default:

            /* should never get here ... then again, never say never */
            MLOGCSI((STATUS_PROCESS_FAILURE, st_module, CSI_NO_CALLEE, 
          MMSG(924, "Invalid version number %d"), minimum_version));
            return(STATUS_PROCESS_FAILURE);
        }

        /* throw out packets that are too small */
        if (size < vers_dep_size) {
            MLOGCSI((STATUS_MESSAGE_TOO_SMALL, st_module, CSI_NO_CALLEE, 
          MMSG(941, "Undefined message detected:discarded")));
            return(STATUS_MESSAGE_TOO_SMALL);
        }   

        /* set up message options */
        msg_opt = ((REQUEST_HEADER*)ipc_datap)->message_header.message_options;

#ifndef SSI /* csi code */              /* set packet direction string */
        directp = "From ACSLM";
#else /* ssi code */
        directp = "From Client";
#endif /* SSI */

        /*  throw out duplicates */
        if (st_isaduppak((IPC_HEADER *) ipc_datap, msg_opt) == TRUE)
            return(STATUS_INVALID_MESSAGE);

#ifndef SSI /* csi code */
        /* get the client's return address using ipc_identifier as the key */
        if ((status = csi_qget(csi_lm_qid, ipc_header.ipc_identifier, 
                            (void **)&cs_hdr_qp)) != STATUS_SUCCESS) {
            MLOGCSI((status, st_module, "csi_qget()", 
          MMSG(958, "Message for unknown client discarded")));
            return(STATUS_QUEUE_FAILURE);
        }
        /* set working csi header equal to one on the queue */
        cs_hdr = *cs_hdr_qp;

#else /* ssi code */

        /* ssi sets its own return address */

#ifdef ADI /* adi code */
        cs_hdr = csi_ssi_adi_addr;

#else /* ADI - rpc code */
        cs_hdr = csi_ssi_rpc_addr;

#endif /* ADI */

#ifdef XAPI /* SSI and XAPI */
        /*************************************************************/
        /* If we are performing ACSAPI-to-XAPI conversion, then:     */
        /* o  Enter an XAPIREQE request entry into the XAPICVT       */
        /*    able (csi_xapi_control_table).                         */
        /* o  Pass the index of the new XAPIREQE request             */
        /*    to the forked xapi_main.c thread to process.           */
        /*************************************************************/
        if (csi_xapi_conversion_flag)
        {
            pXapicvt = (struct XAPICVT*) csi_xapi_control_table;

            if (pXapicvt != NULL)
            {
                pXapicvt->requestCount++;

                TRMSG("XAPICVT.requestCount=%d\n",
                      pXapicvt->requestCount);
            }

            pXapireqe = st_xapi_queue_req((char*) ipc_datap,
                                          size,
                                          &xapireqeIndex);

            /*********************************************************/
            /* If we could not queue the XAPI request, we call       */
            /* xapi_main with a negative XAPIREQE index.  This       */
            /* signals to xapi_main to immediately respond with      */
            /* a STATUS_MAX_REQUESTS_EXCEEDED error response.        */
            /*********************************************************/
            if (pXapireqe == NULL)
            {
                TRMSG("Could not queue the new XAPI request\n");

                xapireqeIndex = -1;
            }

            pid = fork();

            /*********************************************************/
            /* When fork() returns 0, we are the child process.      */
            /* Set a new environmental variable for common trace and */
            /* log services; announce ourselves as a new child       */
            /* process; call xapi_main(), then exit.                 */
            /*********************************************************/
            if (pid == 0)
            {
                SETENV("CDKTYPE", "XAPI");

                TRMSG(">>>> XAPI xapi_main() child process <<<<\n");

                xapiRC = xapi_main((char*) ipc_datap,
                                   size,
                                   xapireqeIndex,
                                   XAPI_FORK);

                TRMSG("xapiRC=%d from xapi_main(xapireqeIndex=%d)\n",
                      xapiRC,
                      xapireqeIndex);

                _exit(0);
            }
            /*********************************************************/
            /* When fork() returns a positive number, we are the     */
            /* parent process.  Wait for any XAPI child process      */
            /* to terminate and return STATUS_SUCCESS; any           */
            /* ACSAPI responses will be created by the XAPI          */
            /* child process.                                        */
            /*                                                       */
            /* The wait prevents zombies and <defunct> processes.    */
            /* However, the "last" process may be a zombie until     */
            /* the SSI is shutdown.                                  */
            /*********************************************************/
            else if (pid > 0)
            {
                TRMSG("SSI parent process after fork() of XAPI child process\n");

                while (1)
                {
                    termPid = waitpid(-1, NULL, WNOHANG);

                    if (termPid <= 0)
                    {
                        break;
                    }

                    TRMSGI(TRCI_SERVER,
                           ">>>> XAPI xapi_main() child process %i ended <<<<",
                           termPid);
                }

                return STATUS_SUCCESS;
            }
            /*********************************************************/
            /* When fork() returns a negative number, the fork()     */
            /* failed.  We have no choice but to call the XAPI       */
            /* component synchronously.                              */
            /*********************************************************/
            else
            {
                TRMSG("fork() failed; calling xapi_main synchronously\n");

                xapiRC = xapi_main((char*) ipc_datap,
                                   size,
                                   xapireqeIndex,
                                   XAPI_CALL);

                TRMSG("xapiRC=%d from xapi_main(xapireqeIndex=%d)\n",
                      xapiRC,
                      xapireqeIndex);

                if (xapiRC != STATUS_SUCCESS)
                {
                    return(STATUS) xapiRC;
                }

                return STATUS_SUCCESS;
            }
        }
#endif /* XAPI and SSI */

        if ((status = csi_qput(csi_lm_qid, (char *) &ipc_header,
                sizeof(IPC_HEADER), &member_id )) != STATUS_SUCCESS) {
            MLOGCSI((status, st_module, "csi_qput()", 
          MMSG(958, "Message for unknown client discarded")));
            return(STATUS_QUEUE_FAILURE);
        }
        /* ssi_identifier for mapping application request to response */
        cs_hdr.ssi_identifier = member_id;
#endif /* SSI */

#ifndef ADI /* rpc code */
        /* set up for network transmission */
        PREP_NETBUF(cs_hdr, netaddr, TYPE_LM, size);

        /* packet trace if asked to */
        if (csi_trace_flag) {
            CSI_INTERNET_ADDR_TO_STRING(netaddr_str, netaddr);
            sprintf(port_str, "%u", cs_hdr.csi_handle.raddr.sin_port);
            csi_ptrace(netbufp, cs_hdr.ssi_identifier, netaddr_str,
                                                        port_str, directp);
        }
#else /* ADI code */

        /* set up for network transmission */
        PREP_ADI_NETBUF(cs_hdr, TYPE_LM, size);

        /* packet trace if asked to */
        if (csi_trace_flag) {
            sprintf(port_str, "%s", cs_hdr.csi_handle.client_name);
            csi_ptrace(netbufp, cs_hdr.ssi_identifier, port_str,
                                    port_str, directp);
        }
#endif /* ADI */

        /* send the packet on network */
        status = csi_net_send(netbufp, CSI_NORMAL_SEND);
        switch (status) {
            case STATUS_SUCCESS:                        /* send the packet */
            case STATUS_PENDING:                        /* queued the packet */
            case STATUS_NI_TIMEDOUT:                    /* send timed out */
                break;
            default:
                return(status);                         /* other failure */
        } /* end of switch on status */


#ifndef SSI /* csi code */

        /* delete final response from connection queue */
        if (TRUE == CSI_ISFINALRESPONSE(msg_opt)) {
            if ((status = csi_freeqmem(csi_lm_qid, ipc_header.ipc_identifier, 
                                        CSI_NO_LOGFUNCTION)) != STATUS_SUCCESS)
                return(STATUS_QUEUE_FAILURE);
        }
        return(STATUS_SUCCESS);
#else  /* SSI */
        break;
#endif /* SSI */

#ifdef ADI /* adi code */
    case TYPE_CO_CSI:
        /* determine version dependent size for packet too small test */
        switch (minimum_version) {
        case VERSION0:
            vers_dep_size = sizeof(V0_REQUEST_HEADER);
            break;

        case VERSION1:
            vers_dep_size = sizeof(V1_REQUEST_HEADER);
            break;

        case VERSION2:
        case VERSION3:       /* no change to the size of REQUEST_HEADER in R4 */
        case VERSION4:       /* no change to the size of REQUEST_HEADER in R5 */
            vers_dep_size = sizeof(REQUEST_HEADER);
            break;

        default:

            /* should never get here ... then again, never say never */
            MLOGCSI((STATUS_PROCESS_FAILURE, st_module, CSI_NO_CALLEE, 
          MMSG(924, "Invalid version number %d"), minimum_version));
            return(STATUS_PROCESS_FAILURE);
        }

        /* Input from the CSI co-process */
        /* throw out packets that are too small */
        if (size < vers_dep_size) {
            MLOGCSI((STATUS_MESSAGE_TOO_SMALL, st_module, CSI_NO_CALLEE, 
          MMSG(941, "Undefined message detected:discarded")));
            return(STATUS_MESSAGE_TOO_SMALL);
        }   
        netbufp->size  = size + sizeof(CSI_HEADER) - sizeof(IPC_HEADER);

        /* Make cs_hdr point to data after the IPC_HEADER.  Modify size. */
        cs_hdrp =  (CSI_HEADER *)((char*)ipc_datap + sizeof(IPC_HEADER));
        cs_hdr  = *cs_hdrp;
        size -= sizeof(IPC_HEADER); /* IPC_HEADER has been stripped */

        csi_req_hdr = (CSI_REQUEST_HEADER*)((char*)ipc_datap +
                                                            sizeof(IPC_HEADER));
        msg_opt = csi_req_hdr->message_header.message_options;

        strncpy(appl_socket, ACSLM, SOCKET_NAME_SIZE);

#ifndef SSI /* adi csi code */

        /* CSI checks queue for duplicates */
        if (0 == csi_qcmp(csi_lm_qid, cs_hdrp, sizeof(CSI_XID))) {

#ifdef DEBUG /* adi csi debug code */
            MLOGDEBUG(0, 
          (MMSG(496, "%s: %s: status:%s;\n"
          "Duplicate packet from Network detected:discarded\n"
          "Adman name:%s, process-id:%d, sequence number:%lu"),
          CSI_LOG_NAME, st_module, cl_status(STATUS_INVALID_MESSAGE),
              cs_hdrp->xid.client_name, cs_hdrp->xid.pid,
              cs_hdrp->xid.seq_num));
#endif /* DEBUG */
            return(STATUS_INVALID_MESSAGE);
        }
#endif /* SSI */


      /* set packet direction string */
#ifndef SSI /* adi csi code */
        directp = "From Client";

#else   /* adi ssi code */
        directp = "From ACSLM";

#endif /* SSI */


#ifndef SSI /* adi csi code */

        /* csi: save the return address of the caller */
        if ((status = csi_qput(csi_lm_qid, &cs_hdr, sizeof(CSI_HEADER),
                &member_id)) != STATUS_SUCCESS) {
           MLOGCSI((status, st_module, "csi_qput()", 
         MMSG(958, "Message for unknown client discarded")));
           return(STATUS_INVALID_MESSAGE);
        }
        /* save acslm node on the queue so he can return it in response */
        ipc_header.ipc_identifier = member_id;


#ifdef NOT_CSC /* product code - not intended for CSC developers. */

    /* Fill in the host ID */
    cl_set_hostid (&ipc_header.host_id, (char *)cs_hdrp->xid.client_name);

    /* component of access control - ADI exclusively: the adi csi passes
         * the client name to cl_ac_get_accessid().  The client name was stored
         * in the message header portion of the request header by the ADI
         * client application.  If cl_ac_get_accessid() returns a non null
         * string corresponding to the client host name, stuff that name
         * into the user_label field of the IPC packet destined for the aclsm.
     * Don't do this for VERSION0 packets because they don't have an
     * access_id field.
         */
    if (csi_req_hdr->message_header.message_options & EXTENDED) { 
                         /* VERSION0 packets do not have the EXTENDED bit */

            if ((client_name = cl_ac_get_accessid(
                    (char *)cs_hdrp->xid.client_name,
                            AC_GET_ADI)) != NULL) {
                strncpy(
            csi_req_hdr->message_header.access_id.user_id.user_label,
                                             client_name, EXTERNAL_USERID_SIZE);
        }
        }
#endif /* NOT_CSC */


#else /* adi ssi code */

        /* ssi: get the return ipc address of the application */
        if ((status = csi_qget(csi_lm_qid, cs_hdr.ssi_identifier,
                (void **)&ipc_header_qp)) != STATUS_SUCCESS) {
           MLOGCSI((status, st_module, "csi_qget()", 
         MMSG(958, "Message for unknown client discarded")));
           return(STATUS_INVALID_MESSAGE);
        }
        ipc_header = *ipc_header_qp;

        /* set the name of the socket to write to */
        strncpy(appl_socket, ipc_header.return_socket, SOCKET_NAME_SIZE);

#endif /* SSI */


        /* packet trace if asked to */
        if (csi_trace_flag) {
            {
                /* Use a different netbuf to trace packets from co-process */
                CSI_MSGBUF *np;
                np = (CSI_MSGBUF *)malloc(sizeof(CSI_MSGBUF) + 
                    MAX_MESSAGE_SIZE);
                if (NULL == np) {
                    MLOGCSI((STATUS_PROCESS_FAILURE, st_module, "malloc()", 
              MMSG(981, "Operating system error %d"), errno));
                    return(STATUS_PROCESS_FAILURE);
                }   
                /* initialize np statistics */
                np->size = size;
                np->translated_size   = 0;
                np->packet_status     = CSI_PAKSTAT_INITIAL;
                np->maxsize           = MAX_MESSAGE_SIZE;
                np->q_mgmt.xmit_tries = 0;
                memcpy((char *) np->data, (const char *)cs_hdrp, size);
                sprintf(port_str, "%s", cs_hdr.csi_handle.client_name);

                csi_ptrace(np, cs_hdr.ssi_identifier, port_str,
                    port_str, directp);
                free(np);
            }
        }

#ifdef TEST_HOOK
{
        extern csi_recv_test();
        csi_recv_test(cs_hdrp);
}
#endif /* TEST_HOOK */

        /* Replace CSI_HEADER with IPC_HEADER.  Modify size.*/
        lmp = (char *)((char *)cs_hdrp + (sizeof(CSI_HEADER) -
                            sizeof(IPC_HEADER)));
        size += sizeof(IPC_HEADER) - sizeof(CSI_HEADER);
        ip = (IPC_HEADER *)lmp;
        *ip = ipc_header;  /* Overlay ipc_header built here onto LM packet */

        /* send packet via ipc */
        if (cl_ipc_write(appl_socket, lmp, size) != STATUS_SUCCESS) {
            MLOGCSI((STATUS_IPC_FAILURE, st_module, CSI_NO_CALLEE, 
          MMSG(959, "Cannot send message %s:discarded"), directp));
            return(STATUS_IPC_FAILURE);
        }


#ifdef SSI /* adi ssi code */

    /* if final response, delete return address from connection queue */
    if (TRUE == CSI_ISFINALRESPONSE(msg_opt)) {
        if (csi_freeqmem(csi_lm_qid, cs_hdr.ssi_identifier,
            CSI_NO_LOGFUNCTION) != STATUS_SUCCESS) {
            return(STATUS_QUEUE_FAILURE); 
        }
    }    
#endif /* SSI */

        break;
#endif /* ADI */
        

    } /* end switch on module_type */
}

/*
 * Name:
 *
 *      st_isaduppak()
 *
 * Description:
 *
 *      This function checks for the existence of duplicate packets, returning
 *      TRUE if the packet is a duplicate or FALSE if it is not.  Some of these
 *      checks are superfluous until the CSI (in a future version) talks to
 *      other processes in addition to the ACSLM.
 *
 * Return Values:
 *
 *      (BOOLEAN)       - TRUE, means this packet is a duplicate
 *      (BOOLEAN)       - FALSE, means this packet is NOT a duplicate
 *
 * Implicit Inputs:
 *
 *      st_dup_test     - static structure holding identifying information from
 *                      - the previous packet is used as a basis for determining
 *                        if a packet is a duplicate.
 *
 * Implicit Outputs:
 *
 *      NONE
 *
 * Considerations:
 *
 *      NONE
 *
 */

static BOOLEAN 
st_isaduppak (
    IPC_HEADER *ipc_hdrp,                      /* ipc header */
    unsigned char msg_opt                       /* message options */
)
{

    BOOLEAN dup = TRUE;         /* flag TRUE if it is a duplicate packet */

    /* check if the retry bit was set */
    if (0 == (RETRY & ipc_hdrp->options))
        dup = FALSE;

    /* check if the packet sequence number is a duplicate */
    if (ipc_hdrp->seq_num != st_dup_test.ipc_header.seq_num)
        dup = FALSE;

    /* check if the module type is the same as previous */
    else if (ipc_hdrp->module_type != st_dup_test.ipc_header.module_type)
        dup = FALSE;

    /* check if the return_socket is the same as previous */
    else if (strcmp(ipc_hdrp->return_socket,
                                st_dup_test.ipc_header.return_socket) != 0)
        dup = FALSE;

    /* check if the ipc_identifier is the same as previous */
    else if (ipc_hdrp->ipc_identifier != st_dup_test.ipc_header.ipc_identifier)
        dup = FALSE;

    /* check if the message_options are the same */
    else if (msg_opt != st_dup_test.message_options)
        dup = FALSE;

    /* reset the test case if have a brand new packet */
    if (FALSE == dup) {
        st_dup_test.ipc_header = *ipc_hdrp;
        st_dup_test.message_options = msg_opt;
    }
    else {
    
        /* log occurrence of the duplicate packet */
        MLOGCSI((STATUS_SUCCESS, st_module, "st_isaduppak()", 
      MMSG(982, "Duplicate packet from ACSLM detected:discarded")));
    }

    return(dup);

}


#if defined(XAPI) && defined(SSI)
/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: st_xapi_queue_req                                 */
/** Description:   Find a free entry in the XAPIREQE table.          */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "st_xapi_queue_req"

static struct XAPIREQE *st_xapi_queue_req(char *pAcsapiBuffer, 
                                          int   acsapiBufferSize,
                                          int  *pXapireqeIndex)
{
    IPC_HEADER         *pIpc_Header         = (IPC_HEADER*) pAcsapiBuffer;
    REQUEST_HEADER     *pRequest_Header     = (REQUEST_HEADER*) pAcsapiBuffer;
    MESSAGE_HEADER     *pMessage_Header     = &(pRequest_Header->message_header);
    struct XAPICVT     *pXapicvt;
    struct XAPIREQE    *pCurrXapireqe;
    struct XAPIREQE    *pNewXapireqe        = NULL;
    auto int            i;
    auto int            seqNumber           = pMessage_Header->packet_id;

#ifdef DEBUG
    TRMSG("Entered; csi_xapi_control_table=%08X\n",
          csi_xapi_control_table);
#endif

    *pXapireqeIndex = -1;

    if (csi_xapi_control_table == NULL)
    {
        return NULL;
    }

    /*****************************************************************/
    /* Test if any existing XAPIREQE elements can be deleted.        */
    /*                                                               */
    /* NOTE: XAPIREQE_END requests can be free'd immediately.        */
    /* XAPIREQE_CANCEL requests can be deleted only if a sufficient  */
    /* time has elapsed since the XAPIREQE.eventTime.                */
    /*****************************************************************/
    pXapicvt = (struct XAPICVT*) csi_xapi_control_table;

    for (i = 0;
        i < MAX_XAPIREQE;
        i++)
    {
        pCurrXapireqe = (struct XAPIREQE*) &(pXapicvt->xapireqe[i]);

#ifdef DEBUG
        TRMSG("XAPIREQE=%08X, index=%d, requestFlag=%02X, seqNumber=%d\n",
              pCurrXapireqe,
              i,
              pCurrXapireqe->requestFlag,
              pCurrXapireqe->seqNumber);
#endif

        if (pCurrXapireqe->requestFlag & XAPIREQE_END)
        {
            pCurrXapireqe->requestFlag = XAPIREQE_FREE;

#ifdef DEBUG
            TRMSG("Freeing XAPIREQE=%08X, index=%d, seqNumber=%d\n",
                  pCurrXapireqe,
                  i,
                  pCurrXapireqe->seqNumber);
#endif

        }
    }

    /*****************************************************************/
    /* Now find a slot for the new XAPIREQE entry.                   */
    /*****************************************************************/
    pXapicvt = (struct XAPICVT*) csi_xapi_control_table;

    for (i = 0;
        i < MAX_XAPIREQE;
        i++)
    {
        pCurrXapireqe = (struct XAPIREQE*) &(pXapicvt->xapireqe[i]);

        if (pCurrXapireqe->requestFlag == XAPIREQE_FREE)
        {
            pNewXapireqe = pCurrXapireqe;
            *pXapireqeIndex = i;
            memset((char*) pNewXapireqe, 0, sizeof(struct XAPIREQE));
            pNewXapireqe->requestFlag = XAPIREQE_START;
            pNewXapireqe->command = (char) pMessage_Header->command;
            pNewXapireqe->seqNumber = seqNumber;
            time(&(pNewXapireqe->startTime));
            pNewXapireqe->pAcsapiBuffer = pAcsapiBuffer;
            pNewXapireqe->acsapiBufferSize = acsapiBufferSize;

            memcpy(pNewXapireqe->return_socket, 
                   pIpc_Header->return_socket,
                   sizeof(pIpc_Header->return_socket));

            TRMSG("XAPIREQE=%08X, index=%d, requestFlag=%d, seqNumber=%d\n",
                  pNewXapireqe,
                  i,
                  pNewXapireqe->requestFlag,
                  pNewXapireqe->seqNumber);

            break;
        }
    }

    return pNewXapireqe;
}
#endif /* XAPI and SSI */


