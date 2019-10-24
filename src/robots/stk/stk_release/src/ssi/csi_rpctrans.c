static char SccsId[] = "@(#)csi_rpctrans.c	5.4 10/12/94 (c) 1994 StorageTek";
/*
 * Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *
 *      csi_rpctransient()
 *
 * Description:
 *
 *      Gets a transient program number from the RPC port mapper.
 *      Returns the next available RPC transient program number. 
 *      For UDP only, it will assign a socket if the contents of 
 *      sockp = RPC_ANYSOCK (for TCP, the socket is assigned in
 *      csi_rpctinit()).  It initializes "addrp" (type sockaddr_in) 
 *      internet socket address structure.
 *
 * Return Values:
 *
 *      (int)           - program number
 *      (0)             - Failed to assign program number
 *
 * Implicit Inputs:
 *
 *      NONE
 *
 * Implicit Outputs:
 *
 *      Initializes (sockaddr_in)"addrp" internet socket address structure.
 *
 * Considerations:
 *
 *      NONE
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner       24-Jan-1989.    Created.
 *	D. A. Myers	    12-Oct-1994	    Porting changes
 *      Anton Vatcky        01-Mar-2002     Set correct type cast paramters for 
 *                                          getsockname argument.
 *      Mitch Black         12-Aug-2003     Changes for firewall-secure CSI/SSI
 *	Mike Williams	    03-May_2010     Changes for 64-bit compile. Changed
 *					    len to be type socklen_t.  Included
 *                                          rpc/rpc.h and rpc/pmap_clnt.h
 */


/*
 *      Header Files:
 */

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include "csi.h"
#include "cl_pub.h"
#include "ml_pub.h"



/*
 *      Defines, Typedefs and Structure Definitions:
 */
#define START_TRANSIENT 0x40000000      /* start number for transient progs */
#define END_TRANSIENT   0x5ffffffe      /* last transient prog# available */

/*
 *      Global and Static Variable Declarations:
 */
static char     *st_src = __FILE__;
static char     *st_module = "csi_rpctransient()";

/*
 *      Procedure Type Declarations:
 */

unsigned long 
csi_rpctransient (
    unsigned long proto,                 /* socket protocol */
    unsigned long vers,                  /* version number */
    int *sockp,                 /* pointer to socket */
    struct sockaddr_in *addrp              /* pointer to inet return address */
)
{

static  unsigned long   prognum = START_TRANSIENT;
        int             s;              /* socket descriptor */
        socklen_t       len;            /* size of internet addres */
        int             socktype;       /* type of socket */

#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module,                     /* routine name */
                 4,                             /* parameter count */
                 (unsigned long) proto,         /* argument list */
                 (unsigned long) vers,          /* argument list */
                 (unsigned long) sockp,         /* argument list */
                 (unsigned long) addrp);        /* argument list */
#endif /* DEBUG */


    switch (proto) {

        case IPPROTO_UDP:
            socktype = SOCK_DGRAM;
            break;

        case IPPROTO_TCP:
            socktype = SOCK_STREAM;
            break;
            
        default:
            MLOGCSI((STATUS_RPC_FAILURE, st_module,  CSI_NO_CALLEE, 
	      MMSG(957, "Invalid network protocol")));
            return(0);

    } /* end of switch */

    /* Firewall secure changes begin here. */
    /* Only do this for UDP - For TCP it's done in csi_rpctinit() */
    if (socktype == SOCK_DGRAM) {
    if (RPC_ANYSOCK == *sockp) {
        if ((s = socket(AF_INET, socktype, 0)) < 0) {
            return(0);
        }
        *sockp = s;
    } else
        s = *sockp;                     /* use the socket passed in */

    addrp->sin_addr.s_addr = INADDR_ANY;
    addrp->sin_family = AF_INET;
    addrp->sin_port   = 0;
    len               = sizeof(struct sockaddr_in);
    bind(s, (struct sockaddr *)addrp, len);  /* error OK if already bound */

#ifdef AIX
    if (getsockname(s, (struct sockaddr *)addrp, &len) < 0)
#else
    if (getsockname(s, (struct sockaddr *)addrp, &len) < 0)
#endif
        return(0);
    }  /* End of UDP / SOCK_DGRAM stuff */
    /* End of firewall secure changes. */

    /*
     *  count up from the first program number until find one that is available
     */
    for (prognum = START_TRANSIENT; prognum < END_TRANSIENT; prognum++) {

        /*
         *      see if found a transient program number available
         */
        if (pmap_set(prognum, vers, proto, ntohs(addrp->sin_port)) > 0) {
            return(prognum);
        }
    }

    /*
     *  failed to get a transient program number if got here
     */
    return(0);

}
