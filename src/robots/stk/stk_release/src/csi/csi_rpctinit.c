static char SccsId[] = "@(#)csi_rpctinit.c	5.5 1/5/94 (c) 1989 StorageTek";
/*
 * Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *
 *      csi_rpctinit()
 *
 * Description:
 *
 *      This function implements the CSI as an RPC/TCP service.
 *
 *      Sets the predefined RPC version number and protocol numbers to reflect
 *      the TCP version.
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
 *      Sets the predefined program number to CSI_PROGRAM.
 *
 *      Calls pmap_unset() to un-register the service if currently registered.
 *
 *              If TRUE == pmap_unset(), csi_logevent is called with 
 *              STATUS_SUCCESS and message MSG_UNMAPPED_RPCSERVICE to log
 *              the removal of a previously registered RPC service.
 *
 *      END CONDITIONAL COMPILATION
 *
 *      Calls svctcp_create() to be assigned a port, socket, and transport
 *      handle.  Uses csi_rpc_tcpsock (global) equal to RPC_ANYSOCK to have rpc
 *      mechanism create and default socket mechanisms.
 *
 *              If NULL == svctcp_create(), returns STATUS_RPC_FAILURE after
 *              calling csi_logevent() with status STATUS_RPC_FAILURE and the 
 *              message MSG_RPCTCP_SVCCREATE_FAILED.
 *
 *              Else if NULL != svctcp_create(), the service handle returned 
 *              is assigned to csi_tcpxprt (global).
 *
 *      Calls svc_register() to register the RPC service.
 *
 *              If 0 == svc_register(), returns STATUS_RPC_FAILURE after
 *              calling csi_logevent() with status STATUS_RPC_FAILURE and the 
 *              message MSG_RPCTCP_SVCREGISTER_FAILED.
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
 *      J. A. Wishner   03-Jan-1989.    Created.
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
 *	E. A. Alongi	04-Jan-1994.    eliminated addr.sin_addr.S_un.S_addr
 *					for Sun since s_addr is defined as
 *					S_un.S_addr in netinet/in.h on Sun.
 *					Eliminated ifdef ADI since this module
 *					is no longer part of the ADI build.
 *					Cleanup resulting from flint run.
 *      Ken Stickney    23-Dec-1994.    Changes for Solaris port.
 *      Van Lepthien    30-Aug-2001     Removed definition of svc_register
 *                                      which caused AIX compile failures.
 *      Anton Vatcky    01-Mar-2002     Relocated SSI includes
 *                                      ahead of other includes to avoid the
 *                                      redefine of the SA_RESTART macro.
 *      Mitch Black     12-Aug-2003     Synch'ed Linux/Solaris changes from
 *                            CDK to the ACSLS base code and vice versa.
 *      Mitch Black     12-Aug-2003     Firewall secure CSI/SSI changes.
 *	Mitch Black	06-Dec-2004	For AIX, #include socket.h
 *	Mike Williams	03-May-2010	Removed duplicate prototype for
 *					svc_register when not LINUX.
 *					Removed propotype for pmap_unset
 *					when code is compiled as SSI. Included
 *                                      stdlib.h, ctype.h, and rpc/pmap_clnt.h.
 *     
 */


/*
 *      Header Files:
 */
#include <stdlib.h>
#include <ctype.h>

#ifdef AIX
/* The following is included within <rpc/rpc.h> on some systems, but not on AIX. */
/* So, we must include it here for all the socket structures and #defines. */
#include <sys/socket.h> 
#endif

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include "cl_pub.h"
#include "ml_pub.h"
#include "csi.h"



/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Prototypes:
 */
#ifndef SOLARIS
SVCXPRT * svctcp_create(int sock, u_int sendsz, u_int recvsz);
#endif

/* This local routine used for debugging only (defined at the end */
/* of this file).  The real version replaces the stub when debugging. */
void printsock(char *, int, struct sockaddr *);

/*
 *      Global and Static Variable Declarations:
 */
static char     *st_module = "csi_rpctinit()";

/*
 *      Procedure Type Declarations:
 */


STATUS 
csi_rpctinit (void)
{

    unsigned long progno;               /* RPC program number */
    unsigned long proto;                /* protocol# for svc_register */
    unsigned long vers;                 /* version number */
    int           sock  = RPC_ANYSOCK;  /* socket descriptor */
    struct sockaddr_in addr;            /* inet address,port of this csi/ssi */
    char           *envp;               /* environment string pointer */
    int           inetport;             /* port number to use */
    int           true = 1;             /* Used as a boolean */
    int            i;                   /* loop counter */
    char           *sp;                 /* general string pointer */
    socklen_t      len;                 /* size of internet addres */



#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module,                     /* routine name */
                 0,                             /* parameter count */
                 (unsigned long) 0);            /* argument list */
#endif /* DEBUG */

    /* version and protocol to use for both ssi and csi */
    vers   = CSI_TCP_VERSION;
    proto  = IPPROTO_TCP;

    /***** Changes for firewall-secure, which means fixed incoming port */
    /* If the env variable is set, use it as the port.  Otherwise,  */
    /* allow RPC to choose the port at random.                      */
#ifdef SSI  /* Get the SSI env variable */
    envp = (char*)getenv("SSI_INET_PORT");
#else /* Get the CSI env variable */
    envp = (char*)getenv("CSI_INET_PORT");
#endif  /* SSI */

    inetport = 0;       /* Default to 0, let RPC choose the port */
    if ((char*) NULL != envp) {  /* There was a value in the env */
        /* Validate that it is a well formatted number */
        for (sp = envp; NULL != sp && '\0' != *sp; sp++) {
            if (!isdigit(*sp)) {
                MLOGCSI((STATUS_PROCESS_FAILURE,  st_module,
                  "getenv(CSI_ or SSI_INET_PORT)",
                  MMSG(MUNK, "Illegal value %s: Must be numeric.\n"), envp));
                return((STATUS) STATUS_PROCESS_FAILURE);
            }
        }
        inetport = atoi(envp);  /* Set the specified port */
        if ((inetport != 0) && ((inetport<1024) || (inetport>65535))) {
            MLOGCSI((STATUS_PROCESS_FAILURE,  st_module, NULL,
              MMSG(MUNK, "Out-of-bounds value %d: CSI_ or SSI_INET_PORT.\n"),
              inetport));
            return((STATUS) STATUS_PROCESS_FAILURE);
        }
    }

    /* Get the socket that's going to be used */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        MLOGCSI((STATUS_PROCESS_FAILURE,  st_module,
           "socket(AF_INET...)",
           MMSG(MUNK, "Unable to allocate socket for RPC TCP service.\n")));
        return((STATUS) STATUS_RPC_FAILURE);
    }

    /* Make sure we give it the REUSE option, for coming up and */
    /* down (SSI/CSI) */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) < 0) {
        MLOGCSI((STATUS_PROCESS_FAILURE,  st_module,
           "setsockopt()",
           MMSG(MUNK, "Unable to set SO_REUSEADDR on socket.\n")));
        return((STATUS) STATUS_RPC_FAILURE);
    }

    /* Now set up the address structure with the specified port, */
    /* or without the port to allow RPC to choose it. */
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    /* If inetport 0, RPC chooses the port, else use the specified inetport */
    addr.sin_port   = htons(inetport); /* Either 0 or something specified */
    memset(&(addr.sin_zero), '\0', 8);
    len               = sizeof(struct sockaddr_in);

    (void) printsock("A0", sock, (struct sockaddr *)&addr);

    /* Then bind the socket to the port */
    if (bind(sock, (struct sockaddr *)&addr, len) < 0) {
        MLOGCSI((STATUS_PROCESS_FAILURE,  st_module, "bind()",
           MMSG(MUNK, "Unable to bind socket to port %d.\n"), inetport));
        return((STATUS) STATUS_RPC_FAILURE);
    }

#ifdef AIX
    if (getsockname(sock, (struct sockaddr *)&addr, (size_t *)&len) < 0) {
#else
    if (getsockname(sock, (struct sockaddr *)&addr, &len) < 0) {
#endif
        MLOGCSI((STATUS_PROCESS_FAILURE,  st_module,
           "getsockname()",
           MMSG(MUNK, "Failed on attempt to get socket name.\n")));
        return((STATUS) STATUS_RPC_FAILURE);
    }
    (void) printsock("A2", sock, (struct sockaddr *)&addr);

    if (csi_getiaddr((caddr_t)&addr.sin_addr) != STATUS_SUCCESS)
        return((STATUS) STATUS_RPC_FAILURE);

    (void) printsock("B1", sock, (struct sockaddr *)&addr);
    /***** End of firewall-secure changes. */

#ifdef SSI
 
    /* get ssi version number - if not there default it */
    envp = (char*)getenv(CSI_SSI_CALLBACK_VERSION_NUMBER);
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

    /* get transient program number and version, and port to build an ssi */
    progno = csi_rpctransient(proto, vers, &sock, &addr);
    if (0 == progno) {
        MLOGCSI(((STATUS) STATUS_RPC_FAILURE, st_module, "csi_rpctransient()", 
	  MMSG(963, "Invalid RPC program number")));
        return ((STATUS) STATUS_RPC_FAILURE);
    }

    /* proto reset to zero - since just mapped (see Sun manual on callbacks) */
    proto  = 0;

    /* record the return address of this ssi in the csi header */
    /* address this host */
    if (csi_getiaddr((caddr_t)&addr.sin_addr) != STATUS_SUCCESS)
        return((STATUS) STATUS_RPC_FAILURE);
    (void) printsock("B2", sock, (struct sockaddr *)&addr);

    /* record the return address of this ssi in csi header (global storage) */
    csi_ssi_rpc_addr.ssi_identifier     = CSI_NO_SSI_IDENTIFIER;
    csi_ssi_rpc_addr.csi_syntax         = CSI_SYNTAX_XDR;
    csi_ssi_rpc_addr.csi_proto          = CSI_PROTOCOL_TCP;
    csi_ssi_rpc_addr.csi_ctype          = CSI_CONNECT_RPCSOCK;

    csi_ssi_rpc_addr.csi_handle.program = progno;
    csi_ssi_rpc_addr.csi_handle.version = vers;
 
    /* packet inet address, family, and port  from ssi*/
    csi_ssi_rpc_addr.csi_handle.raddr.sin_addr.s_addr   =
        					htonl(addr.sin_addr.s_addr);
    csi_ssi_rpc_addr.csi_handle.raddr.sin_family = addr.sin_family;
    csi_ssi_rpc_addr.csi_handle.raddr.sin_port = htons(addr.sin_port);

#else /* SSI - the csi code */

    /* set predefined program number */
    /* CSI_PROGRAM1 is the old program number (not officially sanctioned) */
    for(i=0;i<2;i++) {
        progno = (i==0) ? CSI_PROGRAM1 : CSI_PROGRAM;

        /* unmap this service if already mapped */
        if (pmap_unset(progno, vers) == TRUE) {
            MLOGCSI((STATUS_SUCCESS, st_module,  CSI_NO_CALLEE, 
	      MMSG(964, "Unmapped previously registered RPC service.")));
        }

#endif /* SSI */

        /* assign a port, socket, and transport handle to the service */
        /* Note that the original sock value must be passed in here */
        (void) printsock("C1", sock, (struct sockaddr *)&addr);
        csi_tcpxprt= svctcp_create(sock,CSI_DEF_TCPSENDBUF,CSI_DEF_TCPRECVBUF);

        /* log an error if service creation failed */
        if ((SVCXPRT *) NULL == csi_tcpxprt) {
            MLOGCSI(((STATUS) STATUS_RPC_FAILURE, st_module, "svctcp_create()",
	      MMSG(965, "Create of RPC TCP service failed")));
            return((STATUS) STATUS_RPC_FAILURE);
        }

        /* register the tcp rpc service with the port mapper */
        if (svc_register(csi_tcpxprt, progno, vers, csi_rpcdisp, proto)
									== 0) {
            MLOGCSI(((STATUS) STATUS_RPC_FAILURE, st_module, "svc_register()", 
	      MMSG(966, "Can't register RPC TCP service")));
            return((STATUS) STATUS_RPC_FAILURE);
        
        }

#ifndef SSI
    }
#endif

    return(STATUS_SUCCESS);
}


/* Here's the empty NO-OP version */
void printsock(char *id, int socknum, struct sockaddr *sp)
{}

/* Uncomment this if you need the socket debug printouts...
************
void printsock(char *id, int socknum, struct sockaddr *sp)
{
    char str[100];
    struct sockaddr_in *sip;

    sip = (struct sockaddr_in *)sp;

    sprintf(str, "%s SOCKET %d: family=%2d port=%2u IPaddr=%16s\n", id, socknum,
 sip->sin_family, ntohs(sip->sin_port), inet_ntoa(sip->sin_addr));
    MLOGCSI((STATUS_SUCCESS,  st_module, CSI_NO_CALLEE,
             MMSG(MUNK, str)));
}
***********/

