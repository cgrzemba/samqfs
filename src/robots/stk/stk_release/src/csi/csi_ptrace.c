static char SccsId[] = "@(#)csi_ptrace.c	5.3 11/9/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_ptrace()
 *
 * Description:
 *
 *      This module write the contents of an ACSLM format format packet to the 
 *      trace log. 
 *
 * Return Values:
 *
 *      NONE
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
 *      NONE
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner       15-Mar-1989.    Created.
 *      J. A. Wishner       23-Mar-1989.    Chg. cl_ipc_send() to cl_ipc_write()
 *      R. P. Cushman       05-Apr-1990.    Fix to stop overrun of tlog.
 *      R. P. Cushman       19-Apr-1990.    Added code for tracing pdaemon pkts.
 *      J. A. Wishner       22-Sep-1990.    Generalize, pass in direction string
 *                                          thus removing pdaemon dependency.
 *      J. A. Wishner       20-Oct-1991.    Delete st_src, unused.
 *      E. A. Alongi        30-Oct-1992.    Replace bzero with memset.
 *      
 *     
 */


/*
 *      Header Files:
 */
#include <string.h>
#include <stdio.h>
#include "cl_pub.h"
#include "cl_ipc_pub.h"
#include "csi.h"



/*
 *      Defines, Typedefs and Structure Definitions:
 */
#define  MAX_TRACE_BLOCK   380       /* output block of 380 bytes */

/*
 *      Global and Static Variable Declarations:
 */
static  char *st_module = "csi_ptrace()";
static  EVENT_LOG_MESSAGE       tlog;                   /* trace log buffer */


/*
 *      Procedure Type Declarations:
 */

void 
csi_ptrace (
    register CSI_MSGBUF *msgbufp,           /* csi network format message buffer */
    unsigned long ssi_id,            /* ipc identifier ala IPC_HEADER */
    char *netaddr_strp,      /* network address string */
    char *port_strp,         /* port number string */
    char *dir               /* string shows transmit direction */
)
{

    register char       *cp;            /* byte ptr trace formating buffer */
    register int         i;             /* packet position byte counter */
    register char       *bp = CSI_PAK_NETDATAP(msgbufp);
                                        /* pointer to packet to be traced */
    int                  len;           /* length data sent to event logger */
    int                  done = FALSE;
    int                  running_count = 0; /* total bytes traced */
 


#ifdef DEBUG
    if TRACE(0)
        cl_trace(st_module,                     /* routine name */
                 8L,                            /* parameter count */
                 (unsigned long) msgbufp,
                 (unsigned long) ssi_id,
                 (unsigned long) netaddr_strp,
                 (unsigned long) port_strp,
                 (unsigned long) dir);
#endif /* DEBUG */

    while (!done ) {

       /* clear out message buffer area */
       memset((char *)&tlog, '\0', sizeof(EVENT_LOG_MESSAGE));
   
       tlog.log_options = LOG_OPTION_TRACE;
   
       /* position trace message pointer at beginning */
       cp = tlog.event_message;

       /* format the packet trace log header information */
       sprintf(cp, 
          "Packet source:      %-14.14s \
          \nssi_identifier:     %-14ld  \
          \nssi client address: %-15.15s   ssi client port id: %s\
          \nMessage contents (hex bytes):",
                    dir,
                    ssi_id,
                    netaddr_strp,
                    port_strp);

        cp += strlen(cp);

        /* format packet data trace */
        for (i = 0; (i < MAX_TRACE_BLOCK) && (running_count < msgbufp->size);
             						i++, running_count++) {
 
 
           /* check if ten columns already printed */
           if ((i % 10) == 0) {
    
               /* start new row of columns of trace printout */
               *cp++ = '\n';
               sprintf(cp, "%04d:  ", running_count); /* print byte count */
               cp += strlen(cp);
           }
           sprintf(cp, " %02x", ((int)*bp++ & 0xff));
           cp += strlen(cp);
        }

        /* check if done */
        if (running_count < msgbufp->size )
           done = FALSE;
        else
           done = TRUE;
   
       /* limit length to max writeable */
       len = cp - (char *)&tlog;
       if (len > MAX_MESSAGE_SIZE)
           len = MAX_MESSAGE_SIZE;
   
       /* Send to event logger daemon.  */
       (void)cl_ipc_write(ACSEL, &tlog, len);
   
    } /* end while !done */
}
