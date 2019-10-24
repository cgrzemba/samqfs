static char SccsId[] = "@(#)csi_svcinit.c	5.5 11/11/93 (c) 1992 StorageTek";
/*
 * Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
 *
 *
 * Name:
 *
 *      csi_svcinit()
 *
 * Description:
 *
 *      Routine which controls the initialization of the CSI as a network 
 *      service.  
 *
 *      csi_svcinit() intializes the service designated by the two environment
 *      variables CSI_UDP_RPCSERVICE and/or CSI_TCP_RPCSERVICE.  The CSI can 
 *      operate as both a UDP server and a TCP server, concurrently.
 *
 *      Calls getenv with CSI_UDP_RPCSERVICE as a parameter to determine if it
 *      should initialize the CSI as a UDP server.  
 *
 *      If getenv returned "TRUE", performs the following:
 *      
 *              Sets udp_rpcsvc = TRUE;
 *
 *      Calls getenv with CSI_TCP_RPCSERVICE as a parameter to determine if it
 *      should initialize the CSI as a TCP server.  
 *
 *      If getenv returned "TRUE", performs the following:
 *      
 *              Sets tcp_rpcsvc = TRUE;
 *      
 *      If neither call to getenv() returned TRUE,
 *
 *              Calls csi_logevent() with a status of 
 *              STATUS_PROCESS_FAILURE, message MSG_INVALID_COMM_SERVICE.  
 *              Returns STATUS_INVALID_COMM_SERVICE to the caller.
 *
 *      Else if udp_rpcsvc == TRUE:
 *
 *              Calls csi_rpcuinit() to initialize UDP RPC services.  
 *              If STATUS_SUCCESS != csi_rpcuinit() STATUS_PROCESS_FAILURE is
 *              returned to the caller.
 *
 *      If tcp_rpcsvc == TRUE:
 *
 *              Calls csi_rpctinit() to initialize RPCTCP service.  
 *              If status != STATUS_SUCCESS: STATUS_PROCESS_FAILURE is returned
 *              to the caller.
 *
 *      STATUS_SUCCESS is returned to the caller.
 *      
 *
 * Return Values:
 *
 *      SUCCESS                         - Everything went OK.
 *      STATUS_PROCESS_FAILURE          - Failed to initialize one or more 
 *                                        designated RPC service types(TCP/UDP).
 *                                        A lower level routine has logged 
 *                                        error information in the event log.
 *
 * Implicit Inputs:
 *
 *      NONE           
 *
 * Implicit Outputs:
 *
 *      csi_retry_timeout - (int global) # of seconds per network send try 
 *
 *      csi_retry_tries   - (int global) # of time to try sending on network
 *
 * Considerations:
 *
 *      Please note the initialization of global variables, above.
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner       03-Jan-1989.    Created.
 *      E. A. Alongi        01-Sep-1992     As a result of R3.0.1 changes
 *                                          eliminated code segment now
 *                                          performed in csi_init().
 *	Emanuel A. Alongi   08-Aug-1993.    Dynamic variables initialization of
 *      				    tcp and udp rpc services.
 *      Mike Williams       01-Jun-2010     Included unistd.h to remedy warning.
 *      
 *     
 */


/*
 *      Header Files:
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "csi.h"
#include "cl_pub.h"
#include "ml_pub.h"
#include "dv_pub.h"



/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char     *st_src = __FILE__;
static char     *st_module = "csi_svcinit()";

/*
 *      Procedure Type Declarations:
 */

STATUS 
csi_svcinit (void)
{

    BOOLEAN      udp_rpcsvc;            	/* true if rpc running udp */
    BOOLEAN      tcp_rpcsvc;            	/* true if rpc running tcp */

#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module,                     /* routine name */
                 0,                             /* parameter count */
                 (unsigned long) 0);            /* argument list */
#endif /* DEBUG */

    /* set global hostname, host address of this host */
    if (gethostname(csi_hostname, CSI_HOSTNAMESIZE) < 0) {
	MLOGCSI((STATUS_PROCESS_FAILURE, st_module,"gethostname()",
	  MNOMSG));
	return(STATUS_PROCESS_FAILURE);
    }
    if (csi_hostaddr(csi_hostname,csi_netaddr,CSI_NETADDR_SIZE)!=STATUS_SUCCESS)
        return(STATUS_PROCESS_FAILURE);
    
    /* determine if should initialize as a udp rpc service */
    if (dv_get_boolean(DV_TAG_CSI_UDP_RPCSERVICE, &udp_rpcsvc) !=
							       STATUS_SUCCESS) {
	udp_rpcsvc = FALSE;
    }

    /* determine if should initialize as a tcp rpc service */
    if (dv_get_boolean(DV_TAG_CSI_TCP_RPCSERVICE, &tcp_rpcsvc) !=
							       STATUS_SUCCESS) {
	tcp_rpcsvc = FALSE;
    }

    /* if neither service initiated, error */
    if (FALSE == tcp_rpcsvc && FALSE == udp_rpcsvc) {
       MLOGCSI((STATUS_PROCESS_FAILURE, st_module,"getenv(CSI_???_RPCSERVICE)",
	 MMSG(945,"Invalid communications service")));
        return(STATUS_PROCESS_FAILURE);
    }

    /* initialize udp service if requested */
    if (TRUE == udp_rpcsvc) { 
        if (STATUS_SUCCESS != csi_rpcuinit())
            return(STATUS_PROCESS_FAILURE);
    }

    /* initialize tcp service if requested */
    if (TRUE == tcp_rpcsvc) { 
        if (STATUS_SUCCESS != csi_rpctinit())
            return(STATUS_PROCESS_FAILURE);
    }
    
    return(STATUS_SUCCESS);

}
