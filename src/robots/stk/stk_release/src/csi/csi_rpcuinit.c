static char SccsId[] = "@(#)csi_rpcuinit.c	5.5 1/5/94 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *
 *      csi_rpcuinit()
 *
 * Description:
 *
 *      This function contains the source code for implementing the CSI as an
 *      RPC/UDP service.
 *
 *      Sets the predefined RPC version number and protocol numbers to reflect
 *      the UDP version.
 *
 *      IF CONDITIONALLY COMPILED AS AN SSI:
 *
 *      Calls csi_rpctransient() to have a socket and transient program
 *      number assigned.  Then resets the protocol to 0 since this establishes
 *      a port mapping (obscure RPC rule, see Sun manual on callbacks).
 *
 *      Calls csi_getiaddr() to get the return internet address of this ssi.
 *
 *      Sets the global return address variables, a CSI_HEADER structure, which
 *      the SSI uses to initialize csi headers for sending to the CSI.  Used
 *      as a stamp-template.
 *
 *      ELSE IF CONDITIONALLY COMPILED AS A CSI
 *
 *      Calls pmap_unset() to un-register the service if currently registered.
 *
 *              If TRUE == pmap_unset(), csi_logevent is called with a status
 *              of STATUS_SUCCESS and the message MSG_UNMAPPED_RPCSERVICE to
 *              log the unmapping of a previously registered RPC service.
 *
 *      Calls svcudp_create() to be assigned a port and socket and transport
 *      handle.  Uses csi_rpc_udpsock (global) equal to RPC_ANYSOCK to have rpc
 *      mechanism create and default socket mechanisms.
 *
 *              If NULL == svcudp_create(), returns STATUS_RPC_FAILURE after
 *              calling csi_logevent() with status STATUS_RPC_FAILURE and the 
 *              message MSG_RPCUDP_SVCCREATE_FAILED.
 *
 *              Else if NULL != svcudp_create(), assigns the service transport 
 *              handle returned to csi_udpxprt (global).
 *
 *      Calls svc_register() to register the RPC service with the port mapper.
 *
 *              If 0 == svc_register(), returns STATUS_RPC_FAILURE after
 *              calling csi_logevent() with status STATUS_RPC_FAILURE and the 
 *              message MSG_RPCUDP_SVCREGISTER_FAILED.
 *
 *      Returns STATUS_SUCCESS.
 *
 *
 * Return Values:
 *
 *      STATUS_SUCCESS          - Service created.
 *      STATUS_RPC_FAILURE      - A Failure occurred calling RPC library
 *                                functions.  An error was logged to the event
 *                                log.
 *
 * Implicit Inputs:
 *
 *      svc_fds         - RPC manipulates these global descriptors
 *
 * Implicit Outputs:
 *
 *      svc_fds         - RPC initializes these global descriptors.
 *
 * Considerations:
 *
 *      There are no routines for determining an RPC service creation error.
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner   04-Jan-1989.    Created.
 *      J. A. Wishner   13-Jun-1989.    Installed changes so that only a
 *                                      single program number is used, and
 *                                      RPC transport types differentiated
 *                                      by version number.
 *      J. A. Wishner   11-25-91        Added for ssi testing::
 *                                      CSI_SSI_CALLBACK_VERSION_NUMBER.
 *                                      This gets an alternate version # for
 *                                      callbacks to be directed at by the CSI.
 *	D. B. Farmer	16-Aug-1993	changed addr.sin_addr.S_un.S_addr to
 *					addr.sin_addr.s_addr for bull
 *	E. A. Alongi	05-Jan-1994.    eliminated addr.sin_addr.S_un.S_addr
 *					for Sun since s_addr is defined as
 *					S_un.S_addr in netinet/in.h on Sun.
 *					Eliminated ifdef ADI since this module
 *					is no longer part of the ADI build.
 *					Cleanup resulting from flint run.
 *      
 *      Ken Stickney    23-Dec-1994     Changes for Solaris port.
 *      Van Lepthien    30-Aug-2001     Removed definition of svc_register
 *                                      which caused AIX compile failures.
 *      Anton Vatcky    01-Mar-2002     Relocated SSI includes
 *                                      ahead of other includes to avoid the
 *                                      redefine of the SA_RESTART macro.
 *

 *     
 */


/*
 *      Header Files:
 */
#ifdef SSI
#include <stdlib.h>
#include <ctype.h>
#endif

#include <rpc/rpc.h>
#include "csi.h"
#include "cl_pub.h"
#include "ml_pub.h"


/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char     *st_module = "csi_rpcuinit()";

/*
 *      Prototype declarations:
 */
#ifndef SSI
bool_t pmap_unset(u_long prognum, u_long versnum);
#endif
#ifndef SOLARIS
SVCXPRT *svcudp_create(int sock);
#endif


/*
 *      Procedure Type Declarations:
 */

STATUS 
csi_rpcuinit (void)
{

    unsigned long progno;                       /* RPC program number */
    unsigned long proto;                        /* protocol# for svc_register */
    unsigned long vers;                         /* version number */
    int           sock = RPC_ANYSOCK;           /* socket descriptor */

#ifndef SSI /* csi only declarations */
    int            i;                  	/* loop counter */ 
#else /* ssi only declarations */
    struct sockaddr_in addr;            /* inet address of this ssi */
    char           *sp;                 /* general string pointer */
    char           *envp;               /* environment string pointer */
#endif /* ~ SSI */


#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module,                     /* routine name */
                 0,                             /* parameter count */
                 (unsigned long) 0);            /* argument list */
#endif /* DEBUG */

    /* set version and protocol */
    vers   = CSI_UDP_VERSION;
    proto  = IPPROTO_UDP;

#ifdef SSI /* the ssi code */
 
    /* get ssi version number - if not there default it */
    envp = (char*) getenv(CSI_SSI_CALLBACK_VERSION_NUMBER);
    if ((char*) NULL != envp) {

	for (sp = envp; NULL != sp && '\0' != *sp; sp++) {
	    if (!isdigit(*sp)) {
		MLOGCSI((STATUS_PROCESS_FAILURE,  st_module, 
		  "getenv(SSI_VERSION_NUMBER)",
		  MMSG(962, "Invalid RPC version number")));
		return((STATUS) STATUS_RPC_FAILURE);
	    }
	}    

	/* override the default with the version specified in the environment */
	vers = atoi(envp);
    }

    /* get transient program number and version to build an ssi */
    progno = csi_rpctransient(proto, vers, &sock, &addr);
    if (0 == progno) {
        MLOGCSI(((STATUS) STATUS_RPC_FAILURE, st_module, "csi_rpctransient()", 
	  MMSG(963, "Invalid RPC program number")));
        return ((STATUS) STATUS_RPC_FAILURE);
    }

    /* proto reset to zero - just mapped (see Sun manual on callbacks) */
    proto  = 0;

    /* get the internet addres of this host, the return address */
    if (csi_getiaddr((caddr_t)&addr.sin_addr) != STATUS_SUCCESS)
        return((STATUS) STATUS_RPC_FAILURE);

    /* record the return address of this ssi in the csi header */
    csi_ssi_rpc_addr.ssi_identifier     = CSI_NO_SSI_IDENTIFIER;
    csi_ssi_rpc_addr.csi_syntax         = CSI_SYNTAX_XDR;
    csi_ssi_rpc_addr.csi_proto          = CSI_PROTOCOL_UDP;
    csi_ssi_rpc_addr.csi_ctype          = CSI_CONNECT_RPCSOCK;
    csi_ssi_rpc_addr.csi_handle.program = progno;
    csi_ssi_rpc_addr.csi_handle.version = vers;

    /* packet inet address, family, and port  from SSI */
    csi_ssi_rpc_addr.csi_handle.raddr.sin_addr.s_addr   =
        					htonl(addr.sin_addr.s_addr);
    csi_ssi_rpc_addr.csi_handle.raddr.sin_family = addr.sin_family;
    csi_ssi_rpc_addr.csi_handle.raddr.sin_port = htons(addr.sin_port);

#else /* the csi code */

    /* set predefined program number and version to build a csi */
    /* CSI_PROGRAM1 is the old program number (not officially sanctioned) */
    for(i=0;i<2;i++) {
        progno = (i==0) ? CSI_PROGRAM1 : CSI_PROGRAM;

        /* unmap this service if already mapped */
        if (pmap_unset(progno, vers) == TRUE) {
            MLOGCSI((STATUS_SUCCESS, st_module,  CSI_NO_CALLEE, 
	      MMSG(964, "Unmapped previously registered RPC service.")));
        }
#endif /* SSI */


        /* assign a port, socket, and transport handle */
        csi_udpxprt = svcudp_create(sock);

        /* log an error if service creation failed */
        if ((SVCXPRT *) NULL == csi_udpxprt) {
            MLOGCSI(((STATUS) STATUS_RPC_FAILURE, st_module, "svcudp_create()",
	      MMSG(967, "Create of RPC UDP service failed")));
            return((STATUS) STATUS_RPC_FAILURE);
        }

        /* register the udp rpc service with the port mapper */
        if (svc_register(csi_udpxprt, progno, vers, csi_rpcdisp, proto) == 0) {
                MLOGCSI(((STATUS) STATUS_RPC_FAILURE, st_module,
		  "svc_register()", 
		  MMSG(968, "Can't register RPC UDP service")));
            return((STATUS) STATUS_RPC_FAILURE);
        
        }

#ifndef SSI
    }
#endif
    return(STATUS_SUCCESS);
}
