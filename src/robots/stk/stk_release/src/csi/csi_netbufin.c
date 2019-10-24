static char SccsId[] = "@(#)csi_netbufin.c	5.6 11/12/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Name:
 *
 *      csi_netbufinit()
 *
 * Description:
 *
 *      Routine initializes storage for the network packet buffer.
 *
 *      Allocates and initializes a network packet buffer structure of type
 *      (CSI_MSGBUF) "buffer" and returns a pointer to it. Initialized as
 *      follows:
 *
 *      o  allocates buffer                 malloc space
 *      o  sets buffer->size              = 0
 *      o  sets buffer->translated_size   = 0
 *      o  sets buffer->packet_status     = CSI_PAKSTAT_INITIAL
 *      o  sets buffer->maxsize           = MAX_MESSAGE_SIZE
 *      o  sets buffer->q_mgmt.xmit_tries = 0;
 *
 *
 * Return Values:
 *
 *      STATUS_PROCESS_FAILURE  - malloc for buffer failed
 *      STATUS_SUCCESS          - network buffer initialized
 *
 *
 * Implicit Inputs:
 *
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *
 *      o  allocates buffer
 *      o  sets buffer->size
 *      o  sets buffer->translated_size
 *      o  sets buffer->packet_status
 *      o  sets buffer->maxsize
 *      o  sets buffer->q_mgmt.xmit_tries
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
 *      J. A. Wishner       09-Jan-1989.    Created.
 *      J. A. Wishner       30-May-1989.    Allocate (malloc()) network buffer.
 *      J. A. Wishner       01-Jun-1989.    Add queue management initialization.
 *      E. A. Alongi        30-Oct-1992     Replaced bzero with memset.
 *      E. A. Alongi        04-Oct-1993.    Corrected flint detected error.
 *      E. A. Alongi        12-Nov-1993.    Corrected all flint detected errors.
 */


/*
 *      Header Files:
 */
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "cl_pub.h"
#include "ml_pub.h"
#include "csi.h"



/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char     *st_module = "csi_netbufinit()";


/*
 *      Procedure Type Declarations:
 */


STATUS 
csi_netbufinit (
    CSI_MSGBUF **buffer                    /* ptr to buffer ptr to initialize */
)
{
#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module,                     /* routine name */
        0,                                      /* parameter count */
        (unsigned long) 0);                     /* argument list */
#endif /* DEBUG */

    /* allocate the network buffer */
    *buffer = (CSI_MSGBUF *) malloc(sizeof(CSI_MSGBUF) + MAX_MESSAGE_SIZE);
    if (NULL == *buffer) {
        MLOGCSI((STATUS_PROCESS_FAILURE,  st_module,  "malloc()", 
	  MMSG(981, "Operating system error %d"), errno));
        return(STATUS_PROCESS_FAILURE);
    }

    /* initialize buffer statistics */
    (*buffer)->size              = 0;
    (*buffer)->translated_size   = 0;
    (*buffer)->packet_status     = CSI_PAKSTAT_INITIAL;
    (*buffer)->maxsize           = MAX_MESSAGE_SIZE;
    (*buffer)->q_mgmt.xmit_tries = 0;

    /* clear out data buffer */
    memset((char *)(*buffer)->data, '\0', (int) MAX_MESSAGE_SIZE);

    return(STATUS_SUCCESS);
        
}
