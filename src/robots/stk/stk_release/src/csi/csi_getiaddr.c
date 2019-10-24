static char SccsId[] = "@(#) %full_name:  2/csrc/csi_getiaddr.c/5 %";
/*
 * Copyright (C) 1989,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *
 *      csi_getiaddr()
 *
 * Description:
 *
 *      The internet host address of the local host is copied to the
 *      byte array passed in and returned.  Currently used only be the
 *	SSI.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS          - OK, internet address returned
 *      STATUS_PROCESS_FAULURE  - Error, could not assign an internet address
 *
 * Implicit Inputs:
 *
 *      NONE
 *
 * Implicit Outputs:
 *
 *      (caddr_t)       addrp - local internet host address is set
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
 *      J. A. Wishner       01-Jan-1989.    Created.
 *	E. A. Alongi	    12-Nov-1993.    Corrrected flint detected errors.
 *	S. L. Siao	    17-Nov-1995.    Added getenv for SSI_HOSTNAME.
 *      Van Lepthien        30-Aug-2001     Changed hostnm from char to 
 *                                          char * to agree with returned value
 *                                          from getenv(). 
 *	S. L. Siao	    20-Mar-2002.    Changed order of includes to avoid
 *                                          redefine of SA_RESTART.
 *      Mike Williams       01-Jun-2010     Included unistd.h
 *     
 */


/*
 *      Header Files:
 */
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include "csi.h"
#include "ml_pub.h"
#include "system.h"



/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char     *st_module = "csi_getiaddr()";


/*
 *      Procedure Type Declarations:
 */

STATUS 
csi_getiaddr (
    caddr_t addrp                          /* ptr to a network address */
)
{

    char            hostname[CSI_HOSTNAMESIZE+1];
    char *          hostnm;
                              
    /*  initialize the internet address for this host */
    /*  either get it from an environmental variable or use gethostname */
    if ((hostnm = getenv("SSI_HOSTNAME")) == (char*)0) {
	 if (gethostname(hostname, (int) CSI_HOSTNAMESIZE) != 0) {
	     MLOGCSI((STATUS_RPC_FAILURE, st_module, "gethostname()",
	       MMSG(929, "Undefined hostname")));
	     return(STATUS_PROCESS_FAILURE);
 	 }
    }
    else {
	  hostname[CSI_HOSTNAMESIZE] = '\0';
	  strncpy(hostname, hostnm, CSI_HOSTNAMESIZE);
	 }
    if (csi_hostaddr(hostname, (unsigned char *) addrp, sizeof(struct in_addr))
							      != STATUS_SUCCESS)
        return(STATUS_PROCESS_FAILURE);

    return(STATUS_SUCCESS);
}
