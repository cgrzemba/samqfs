static char SccsId[] = "@(#)csi_rpccall.c";
/*
 * Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
 *
 *
 * Name:
 *      csi_rpccall()
 *
 * Description:
 *      This function initiates either a TCP or UDP connection to a remote
 *      client SSI's and makes an an rpc call to transmit a storage server
 *      response to a remote client SSI.  In order to initiate the connection
 *      to the client, it extracts the necessary return address information
 *      from the csi_header structure in the network packet buffer.  This
 *      information includes the client's transient program number and
 *      a procedure number that the rpc call will be issued to.
 *
 *      Initializes a pointer into the csi_header portion of the network
 *      buffer passed.
 *
 *      Sets the rpc timeout variables to the timeout (global) per try.
 *
 *      First determines from the csi_proto of csi_header structure of the
 *      network response packet if the client is waiting for a response
 *      on a TCP or an UDP connection.
 *
 *      IF CONDITIONALLY COMPILED AS AN SSI
 *
 *      Calls st_ssi_setup_addr() to initialize rpc addressing of a CSI host.
 *
 *              If STATUS_SUCCESS != st_ssi_setup_addr(), then logs an error
 *              and returns STATUS_RPC_FAILURE to the caller.
 *
 *      Initializes the XDR translation function used to csi_xdrrequest() since
 *      SSI only sends request to the CSI.
 *
 *      ELSE IF CONDITIONALLY COMPILED AS A CSI
 *
 *      Sets up destination rpc network addressing based on contents of the csi
 *      header in the network packet.
 *
 *      Initializes the XDR translation function used to csi_xdrresponse() since
 *      CSI only sends responses to the SSI.
 *
 *      IF CSI_PROTOCOL_TCP == csi_proto, performs the following:
 *
 *              Calls clnttcp_create() to initiate a client TCP connection,
 *              passing address, transient program number, version number,
 *              socket fd = RPC_ANYSOCK, send buffer size = 0, and receive
 *              buffer size = 0.  Send and receive buffer sizes are
 *              defaulted by RPC when set == 0.  Setting the socket
 *              fd = RPC_ANYSOCK causes RPC to assign a socket.
 *
 *              If NULL == clntcp_create() (failure) calls clnt_spcreateerror()
 *              to retrieve a string description of why the call failed.  Then
 *              calls csi_logevent() with STATUS_RPC_FAILURE and message
 *              MSG_RPCTCP_CLNTCREATE, concatenated with the RPC error string.
 *              Returns STATUS_RPC_FAILURE to the caller.
 *
 *      IF CSI_PROTOCOL_UDP == csi_proto, performs the following:
 *
 *              Calls clnttcp_create() to initiate a client UDP connection,
 *              passing address, transient program_number, version_number,
 *              timout structure, and socket fd = RPC_ANYSOCK.  Setting the
 *              socket fd = RPC_ANYSOCK causes rpc to assign a socket.
 *
 *              If NULL == clntudp_create() (failure) calls clnt_spcreateerror()
 *              to retrieve a string description of why the call failed.  Then
 *              calls csi_logevent() with STATUS_RPC_FAILURE and message
 *              MSG_RPCUDP_CLNTCREATE, concatenated with the RPC error string.
 *              Returns STATUS_RPC_FAILURE to the caller.
 *
 *      Calls clnt_call() to send the storage server response, passing:
 *          - the client_handle (obtained by calling csi_rpcconnect())
 *          - procedure number,
 *          - XDR serialization conversion routine
 *          - buffer to be converted by XDR routines (response packet)
 *          - xdr_void and NULL specifying that no data is to be returned
 *          - timout structure specifying how long to wait for acknowledgement
 *
 *              If RPC_SUCCESS != clnt_call() (failure) and the rpc status
 *              returned is not an RPC timeout, calls clnt_sperrno()
 *              to retrieve a string description of why the call failed.  Then
 *              calls csi_logevent() with STATUS_RPC_FAILURE and message
 *              MSG_RPC_CLNTCALL_FAILURE, concatenated with the RPC error
 *              string.  Returns STATUS_RPC_FAILURE to the caller.
 *
 *      Calls clnt_destroy() to close the client handle created as a result of
 *      clnttcp_create()/clntudp_create() calls.
 *
 *      Calls close to close the socket used for clnt_call().
 *
 *      Returns STATUS_SUCCESS
 *
 * Return Values:
 *      STATUS_SUCCESS          - RPC connection is initiated
 *      STATUS_RPC_FAILURE      - RPC connection could not be initiated
 *
 * Implicit Inputs:
 *	NONE
 *
 * Implicit Outputs:
 *
 *      NONE
 *
 * Considerations:
 *      The client handle created as a result of
 *      clnttcp_create()/clntudp_create() calls is destroyed before leaving
 *      this routine.  In addition the socket used is closed.
 *
 * Module Test Plan:
 *
 * Revision History:
 *      J. A. Wishner       10-Jan-1989.    Created.
 *      J. A. Wishner       30-May-1989.    Delete multiple try timeouts.
 *                                          Retry handled at higher level.
 *                                          Use csi NI global timeout.
 *                                          Transaction id stamping moved
 *                                          up-level as a result.
 *      J. A. Wishner       13-Jun-1989.    Installed changes so that only a
 *                                          single program number is used, and
 *                                          RPC transport types differentiated
 *                                          by version number.
 *      J. W. Montgomery    29-Sep-1990.    Modified for OSLAN.
 *      J. A. Wishner       20-Oct-1991.    Delete hostaddr; unused.
 *      E. A. Alongi        29-Jul-1992     Modified for acspd allowed.
 *      E. A. Alongi        30-Oct-1992     Replaced bcopy with memcpy.
 *      E. A. Alongi        04-Nov-1992     When passing error message info,
 *                                          convert ulong Internet address to
 *                                          dotted-decimal equiv using inet_ntoa
 *	Emanuel A. Alongi   16-Aug-1993.    Modifications to support dynamic
 *					    variables.
 *	David Farmer	    23-Aug-1993	    changed S_un.S* to .s
 *	Emanuel A. Alongi   04-Oct-1993.    Corrected errors discovered by flint
 *					    after switching to new compiler.
 *	Emanuel A. Alongi   26-Oct-1993.    Eliminated all ACSPD support code.
 *	Emanuel A. Alongi   10-Nov-1993.    Corrected flint detected errors.
 *	Emanuel A. Alongi   15-Nov-1993.    Solved all but two flint warnings.
 *	Emanuel A. Alongi   20-Dec-1993.    If a call to clnttcp_create() fails,
 *					    close existing socket and try again.
 *	Emanuel A. Alongi   09-Feb-1994.    Corrected parameter to inet_ntoa()
 *					    and added its prototype.  Collapsed
 *					    code referencing struct in_addr
 *					    taddr for sun and other platforms.
 *	D. A. Myers	    12-Oct-1994	    Porting changes
 *	E. A. Alongi	    20-Oct-1994.    R5.0 BR#309 - Added code to check
 *					    for null pointers when logging
 *					    error messages.
 *	E. A. Alongi	    12-Jan-1995.    Modifications in the SSI part of the
 *					    code so that Library Station can
 *					    test in a mutli-server environment.
 *      Van Lepthien        06-Sep-2001     Add preprocessor code so the
 *                                          definition of clnt_sperrno that
 *                                          conflicts with ../rpc/clnt.h is
 *                                          not generated when compiling on
 *                                          SOLARIS.
 *      Anton Vatcky        01-Mar-2002.    Set correct type for
 *                                          clnt_sperrno_cp for Solaris.
 *	Mitch Black         02-Sep-2003     Added modification a to allow SSI to
 *                                          work without accessing the ACSLS portmap
 *                                          daemon.  Depends on defining the value
 *                                          of CSI_HOSTPORT in the environment.
 *	Mitch Black         28-Apr-2004     Removed usage of sys_nerr & sys_errlist.
 * 	                                    Use strerror instead (Solaris appcert).
 *	Mitch Black         30-Nov-2004     Ifdef'ed reference to csi_hostport so
 *                                          it's only compiled into SSI (not CSI).
 *	Mitch Black         07-Dec-2004     Type cast several variables to
 *                                          xdrproc_t to prevent Linux errors.
 *	Mike Williams       03-May-2010     For 64-bit compile in Solaris.
 *                                          Removed the duplicate prototypes for
 *                                          clnttcp_create and clntudp_create
 *					    Added include of <rpc/rpc.h>.
 *                                          Changed the sizeof the memcpy being
 *                                          done for raddr.sin_addr. Added
 *                                          includes for unistd.h, stdlib.h, and
 *                                          ctype.h to remedy warnings.
 */

/*      Header Files: */
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <rpc/rpc.h>
#include <unistd.h>

#include <stdlib.h>
#include <ctype.h>

#include "cl_pub.h"
#include "ml_pub.h"
#include "csi.h"
#include "dv_pub.h"
#include "system.h"

/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static  char   *st_module = "csi_rpccall()";

/*      Global and Static Prototype Declarations: */

#ifndef SOLARIS
char *clnt_sperrno(enum clnt_stat num);
#endif

char *inet_ntoa(struct in_addr in);
int st_get_csi_hostport();


/*      Procedure Type Declarations: */

STATUS
csi_rpccall (
    CSI_MSGBUF *netbufp                       /* net format msg buffer */
)
{
    int                 sock = RPC_ANYSOCK;     /* socket for connection */
    struct timeval      tout;                   /* retry/timout for rpc */
    unsigned long       progno;                 /* remote program# */
    unsigned long       procno;                 /* remote procedure# */
    unsigned long       vers;                   /* version program */
    enum clnt_stat      rpc_stat;               /* rpc status */
    CLIENT             *handle;                 /* client handle */
    CSI_HEADER         *cs_hdrp;                /* local ptr to csi header */
    STATUS              status = STATUS_SUCCESS;/* csi internal status */
    struct sockaddr_in  raddr;                  /* internet address structure */
    xdrproc_t           xlate_func;             /* xdr translation function */
    struct in_addr      r_netaddr;              /* for conversion of inet addr
                                                 * to dotted-decimal notation */
    long		retry_timeout;		/* dynamic variable - seconds
						 * per net send try. */
#ifdef SOLARIS
    const char 		*clnt_sperrno_cp;	/* points to RPC errno string */
#else
    char 		*clnt_sperrno_cp;	/* points to RPC errno string */
#endif
    char		*sys_errlist_cp;	/* points to error message in
						 * vector of message strings */
    char		*inet_ntoa_cp;		/* points to net addr string */

#ifdef SSI
    struct in_addr      taddr;
    static char        *hostname = NULL;        /* name storage server host */
    char               *sp;                     /* general string pointer */
    char               *envp;                   /* environment string pointer */

    char                *testprog;              /* LIBSTA: testers progno# */
    int			csi_hostport=0;		/* Port for SSI to call CSI */

#endif /* SSI */

#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module, 1, (unsigned long) netbufp);
#endif /* DEBUG */

    /* set a pointer to the addressing information */
    cs_hdrp = & ((CSI_REQUEST_HEADER *) netbufp->data)->csi_header;

    /* get retry_timeout */
    if (dv_get_number(DV_TAG_CSI_RETRY_TIMEOUT, &retry_timeout) !=
		      					      STATUS_SUCCESS ) {
	retry_timeout = (long) CSI_DEF_RETRY_TIMEOUT;
    }

    /* set the timeout for each connection/send try */
    tout.tv_sec  = retry_timeout;
    tout.tv_usec = 0;

    /* Set up remote addressing.  Note, ssi sends to predefined csi address,
     * while the csi sends to the address specified in the csi_header.
     */

#ifdef SSI /* the ssi code: */

    /* get the remote host name if haven't already done so */
    if (NULL == hostname) {
        if (NULL == (hostname = getenv(CSI_HOSTNAME))) {
            MLOGCSI((STATUS_RPC_FAILURE, st_module,  "getenv()",
	      MMSG(929, "Undefined hostname")));
            return((STATUS)STATUS_RPC_FAILURE);
        }
    }

    /* get the address of the remote host and set up socket address structure */
    if (csi_hostaddr(hostname, (unsigned char *)&taddr.s_addr,
      sizeof(taddr)) != STATUS_SUCCESS) {
        return((STATUS)STATUS_RPC_FAILURE);
    }

    /* call inet address from SSI */
    /* If CSI port predefined, eliminates need for SSI to query CSI portmap */
    csi_hostport = st_get_csi_hostport(); /* Zero, or predefined portnumber */
    raddr.sin_addr.s_addr = taddr.s_addr;
    raddr.sin_family = AF_INET;
    raddr.sin_port   = ntohs(csi_hostport);

    /*** start LibStation modification for multi-server testing ***/
    testprog = getenv("DIAG_PROGRAM");  /* are we directing to a test HSC/LS? */

    /* if no test HSC/LS program# to direct requests to ... */
    if (NULL == testprog) {
        progno = CSI_PROGRAM;     /* ... set call to csi program number */
    }

    else { /* new progno supplied to direct requests in order to test HSC/LS */

	/* convert test HSC/LS string*/
        progno = (unsigned long) atol(testprog);

	/* atol() failed */
        if (0 == progno) {
            progno = CSI_PROGRAM;     /* default to Sun csi program number */
        } /* couldn't convert it */

    } /* end conversion of test HSC/LS progno string */
    /*** end  LibStation modification for multi-server testing ***/

    switch (cs_hdrp->csi_proto) { /* set call-to-csi version number */
    case CSI_PROTOCOL_TCP:
        vers   = CSI_TCP_VERSION;
        break;

    case CSI_PROTOCOL_UDP:
        vers   = CSI_UDP_VERSION;
        break;

    default:
        MLOGCSI((STATUS_INVALID_COMM_SERVICE,  st_module, CSI_NO_CALLEE,
	  MMSG(945, "Invalid communications service")));
        return((STATUS)STATUS_RPC_FAILURE);
    }

    /* call-to-csi & ssi-callback procedure# depends on service & environment */
    switch (netbufp->service_type) {
    case TYPE_LM:
        procno = CSI_ACSLM_PROC; /* call-to csi procedure */

        /* get SSI acslm return procedure number if haven't already */
        if (CSI_HAVENT_GOTTEN_ENVIRONMENT_YET == csi_ssi_alt_procno_lm) {
            /* get callback/return proc number for acslm requests */
            envp = getenv(CSI_SSI_ACSLM_CALLBACK_PROCEDURE);
            for (sp = envp; NULL != sp && '\0' != *sp; sp++) {
                if (!isdigit(*sp)) {
                    MLOGCSI((STATUS_PROCESS_FAILURE,  st_module,
		      "getenv(CSI_SSI_ACSLM_CALLBACK_PROCEDURE)",
		      MMSG(953, "Invalid procedure number")));
                    return((STATUS)STATUS_RPC_FAILURE);
                }
            }

            /* assign procedure number if one has been set */
            if ((char*) NULL != envp) {
                csi_ssi_alt_procno_lm = atoi(envp);
            } else {
                csi_ssi_alt_procno_lm = CSI_NOT_IN_ENVIRONMENT;
            }
        }

        /* callback procedure */
        ((CSI_HEADER*)netbufp->data)->csi_handle.proc =
            (csi_ssi_alt_procno_lm < 0) ? procno : csi_ssi_alt_procno_lm;
        break;

    default:
	MLOGCSI((STATUS_INVALID_TYPE,  st_module,  CSI_NO_CALLEE,
	  MMSG(954, "Unsupported module type %d detected:discarded"),
	    netbufp->service_type));
	return(STATUS_INVALID_TYPE);
    }

    switch (netbufp->service_type) { /* determine translation routine */
    case TYPE_LM:
	xlate_func = (xdrproc_t)(*csi_xlm_request);
	break;
    default:
	MLOGCSI((STATUS_INVALID_TYPE,  st_module,  CSI_NO_CALLEE,
	  MMSG(954, "Unsupported module type %d detected:discarded"),
	    netbufp->service_type));
	return(STATUS_INVALID_TYPE);
    }

#else /* SSI */    /* the csi code: */

    /* set up CSI addressing to make an rpc callback to an SSI */
    progno = cs_hdrp->csi_handle.program;
    vers   = cs_hdrp->csi_handle.version;
    procno = cs_hdrp->csi_handle.proc;

    raddr.sin_family = cs_hdrp->csi_handle.raddr.sin_family;

    raddr.sin_addr.s_addr = ntohl(cs_hdrp->csi_handle.raddr.sin_addr.s_addr);

    raddr.sin_port = ntohs(cs_hdrp->csi_handle.raddr.sin_port);

    switch (netbufp->service_type) { /* determine translation routine */
    case TYPE_LM:
        xlate_func = (xdrproc_t)(*csi_xlm_response);
        break;
    default:
        MLOGCSI((STATUS_INVALID_TYPE,  st_module,  CSI_NO_CALLEE,
	  MMSG(954, "Unsupported module type %d detected:discarded"),
	    netbufp->service_type));
        return(STATUS_INVALID_TYPE);
    }

#endif /* SSI */

    /* in case of error, convert net address to portable format for logging */
    memcpy((char *) &r_netaddr.s_addr, (const char *) &raddr.sin_addr,
							sizeof(raddr.sin_addr));

    /* call setup different depending on protocol*/
    switch (cs_hdrp->csi_proto) {
    case CSI_PROTOCOL_TCP:

#ifdef TEST_HOOK
{
        extern csi_send_test();
        csi_send_test(netbufp);
}
#endif /* TEST_HOOK */

        /* initiate a client tcp connection */
        handle = clnttcp_create( &raddr, progno, vers, &sock,
            CSI_DEF_TCPSENDBUF, CSI_DEF_TCPRECVBUF);

#ifdef SSI
        if ((csi_hostport == 0) && (NULL == handle)) {
#else /* it's the CSI */
        if (NULL == handle) {
#endif  /* SSI */
            /* try again after going to portmapper */
            close(sock);
            raddr.sin_port = 0;
            raddr.sin_family = AF_INET;
            handle = clnttcp_create( &raddr, progno, vers, &sock,
                  			CSI_DEF_TCPSENDBUF, CSI_DEF_TCPRECVBUF);
        }
        if (NULL == handle) {

            /* use information stored in struct rpc_createerr to figure */
            /* out what went wrong.  Get all necessary string info. */
            clnt_sperrno_cp =  clnt_sperrno(rpc_createerr.cf_stat);

	    /* Get message if error number is within bounds, or NULL if not */
	    sys_errlist_cp = strerror(rpc_createerr.cf_error.re_errno);

            inet_ntoa_cp = inet_ntoa(r_netaddr);

            /* log the message being sure to check for null pointers */
            MLOGCSI((STATUS_RPC_FAILURE,  st_module, "clnttcp_create()",
	       	MMSG(955, "RPC TCP client connection failed, %s\n"
			  "Errno = %d(%s)\nRemote Internet address:%s,"
			  " Port: %d"),
               	clnt_sperrno_cp ? clnt_sperrno_cp : "NULL",
		rpc_createerr.cf_error.re_errno,
		sys_errlist_cp ? sys_errlist_cp : "NULL",
		inet_ntoa_cp ? inet_ntoa_cp : "NULL",
		raddr.sin_port));

            return((STATUS)STATUS_RPC_FAILURE);
        }
        break;

    case CSI_PROTOCOL_UDP:

#ifdef TEST_HOOK
{
        extern csi_send_test();
        csi_send_test(netbufp);
}
#endif /* TEST_HOOK */

        /* initiate a client udp port mapping */
        handle = clntudp_create( &raddr, progno, vers, tout, &sock);

        if (NULL == handle) {
	    /* get the necessary strings */
	    clnt_sperrno_cp = clnt_sperrno(rpc_createerr.cf_stat);
	    inet_ntoa_cp = inet_ntoa(r_netaddr);

            /* convert ulong inet addr to dotted-decimal notation for log msg */
	    /* check for null strings */
            MLOGCSI((STATUS_RPC_FAILURE,  st_module, "clntudp_create()",
	      MMSG(956, "RPC UDP client connection failed, %s\n"
	        "Remote Internet address:%s, Port: %d"),
                clnt_sperrno_cp ? clnt_sperrno_cp : "NULL",
		inet_ntoa_cp ? inet_ntoa_cp : "NULL", raddr.sin_port));

            return((STATUS)STATUS_RPC_FAILURE);
        }
        break;

    default:
        MLOGCSI((STATUS_RPC_FAILURE, st_module,  CSI_NO_CALLEE,
	  MMSG(957, "Invalid network protocol")));
        return((STATUS)STATUS_RPC_FAILURE);

    } /* end switch */

    /* initiate the call back to the client */
    rpc_stat = clnt_call(handle, procno, xlate_func, (caddr_t)netbufp,
      (xdrproc_t)xdr_void, NULL, tout);

    if (RPC_SUCCESS != rpc_stat) {
        if (RPC_TIMEDOUT == rpc_stat) {
            status = STATUS_NI_TIMEDOUT;
        } else {
            status = (STATUS) STATUS_RPC_FAILURE;
        }

        if ( (STATUS)STATUS_RPC_FAILURE == status) {

            /* convert ulong inet addr to dotted-decimal notation for log msg */
	    /* first get the necessary message strings */
	    clnt_sperrno_cp = clnt_sperrno(rpc_stat);

	    /* Get message if error number is within bounds, or NULL if not */
	    sys_errlist_cp = strerror(rpc_createerr.cf_error.re_errno);

	    inet_ntoa_cp = inet_ntoa(r_netaddr);

	    /* log message being sure to check strings for null pointers */
            MLOGCSI((status,  st_module,  "clnt_call()",
	      MMSG(1024, "Cannot send message to NI:discarded, %s\nErrno = %d"
	            	 " (%s)\nRemote Internet address: %s, Port: %d"),
	    	    clnt_sperrno_cp ? clnt_sperrno_cp : "NULL",
		    rpc_createerr.cf_error.re_errno,
		    sys_errlist_cp ? sys_errlist_cp : "NULL",
		    inet_ntoa_cp ? inet_ntoa_cp : "NULL",
		    raddr.sin_port));

        }
    }

    /* destroy/deallocate rpc resources used for the call */
    clnt_destroy(handle);
    close(sock);

    return(status);
}


int st_get_csi_hostport(){

    static int first_pass = TRUE;
    static int predefined_port = 0;
    char *envp;
    char *sp;		/* Simple string pointer */

    /***** Changes for firewall-security.  Avoid call to portmapper */
    /* on CSI platform (ACSLS server).  Use the predefined, fixed   */
    /* CSI port rather than its program number.                     */
    /* If the env variable is set, use it as the port.  Otherwise,  */
    /* continue to operate as normal, going to the portmapper.      */
    if (first_pass) {	/* Only do processing on first pass */
        first_pass = FALSE;
        envp = (char*)getenv("CSI_HOSTPORT");

        if ((char*) NULL != envp) {  /* There was a value in the env */
            /* Validate that it is a well formatted number */
            for (sp = envp; NULL != sp && '\0' != *sp; sp++) {
                if (!isdigit(*sp)) {
                    MLOGCSI((STATUS_PROCESS_FAILURE,  st_module,
                      "getenv(CSI_HOSTPORT)",
                      MMSG(MUNK, "Illegal value %s: Must be numeric.\n"), envp));
                    return(0);
                }
            }
            predefined_port = atoi(envp);  /* Set the specified port */
            if ((predefined_port != 0) && ((predefined_port<1024) || (predefined_port>65535))) {
                MLOGCSI((STATUS_PROCESS_FAILURE,  st_module, NULL,
                  MMSG(MUNK, "Out-of-bounds value %d: CSI_HOSTPORT.\n"),
                  predefined_port));
                return(0);
            }
        }
    }
    return(predefined_port);
}

