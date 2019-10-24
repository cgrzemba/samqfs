static char SccsId[] = "@(#)csi_hostaddr.c	5.9 10/12/94 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *
 *      csi_hostaddr()
 *
 * Description:
 *
 *      Returns the host address of the given host in the buffer "hostaddr".
 *      If the address cannot be obtained a message is logged to the event
 *      log, and STATUS_PROCESS_FAILURE is returned.
 *
 * Return Values:
 *
 *      (unsigned long)         - host address
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
 *      Dependent on the host tables /etc/hosts or any distributed system
 *      equivalent such as that in yellow pages on a Sun. 
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner       11-Apr-1989.    Created.
 *      E. A. Alongi        30-Oct-1992     replaced bzero and bcopy with
 *                                          memset and memcpy respectively.
 *	Emanuel A. Alongi   01-Oct-1993.    Cleaned up flint detected errors.
 *	Emanuel A. Alongi   12-Nov-1993.    Made flint error free.
 *	D. A. Myers	    12-Oct-1994	    Porting changes
 *      Anton Vatcky        01-Mar-2002     Removed prototype for
 *                                          gethostbyname for Solaris and AIX.
 *      Scott Siao          23-Apr-2002     Fixed Memset, memcpy, status functionality
 *                                          added else {} around those calls so
 *                                          that it wouldn't always return status_success.
 *	Hemendra       	    06-Jan-2003	    Removed prototype gethostbyname
 *				   	    for Linux.
 */


/*
 *      Header Files:
 */
#include <string.h>
#include <netdb.h>
#include "csi.h"
#include "ml_pub.h"
#include "system.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static  char *st_module = "csi_hostaddr()";

/*
 *	Prototypes:
 */
#ifdef SOLARIS
#elif AIX
#elif LINUX
#else
struct hostent *gethostbyname(char *);
#endif

/*
 *      Procedure Type Declarations:
 */

STATUS 
csi_hostaddr (
    char *hostname,              /* name of host to return address for */
    unsigned char *addrp,                 /* buffer to copy host address to */
    int maxlen                /*  size buffer to hold host address */
)
{

    STATUS          status;    			/* status */
    struct hostent *hostaddr;                   /* local host's address */

    if (NULL != hostname && '\0' != hostname[0])
	if ((hostaddr = gethostbyname(hostname)) == (struct hostent *) NULL) {
	    MLOGCSI((STATUS_PROCESS_FAILURE,st_module, "gethostbyname()", 
	      MMSG(929, "Undefined hostname")));
	    status = STATUS_PROCESS_FAILURE;
	} else {
	    if (maxlen < hostaddr->h_length)
		status = STATUS_PROCESS_FAILURE;
	    else {
		memset((char *) addrp, '\0', maxlen);
		memcpy((char *) addrp, hostaddr->h_addr, hostaddr->h_length);
		status = STATUS_SUCCESS;
	    }
	}
    else
	status = STATUS_PROCESS_FAILURE;

    if (STATUS_PROCESS_FAILURE == status)
        MLOGCSI((STATUS_PROCESS_FAILURE, st_module,  "", 
	  MMSG(929, "Undefined hostname")));

    return(status);
}
