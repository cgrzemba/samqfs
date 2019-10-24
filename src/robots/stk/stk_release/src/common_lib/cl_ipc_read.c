static char SccsId[] = "@(#) %full_name:  1/csrc/cl_ipc_read.c/5 %";
/*
 * Copyright (C) 1988,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *
 *	cl_ipc_read
 *
 * Description:
 *
 *      Common routine for reading from IPC input socket.
 *	Assumes external (global) definition for input
 *	socket descriptor (sd_in).  All reads are for MAX_MESSAGE_SIZE
 *	bytes, which the caller's buffer must accommodate.
 *      If the bytes read does not equal the amount expected, 
 *      then the socket is read again until all bytes are read or
 *      an error is detected.
 *	If possible, failures are reported to the event_logger and
 *	unsolicited_messages are sent to the acssa.
 *	Before returning, make a call to cl_ipc_send() to flush any
 *	outgoing ipc messages that may exist on the Write_Queue.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *	STATUS_CANCELLED	Cancel signal received while pending on read().
 *      STATUS_IPC_FAILURE	Read failed (returned -1).
 *
 * Implicit Inputs:
 *
 *      sd_in			Input socket descriptor.
 *	my_sock_name		Input socket name (for cl_inform).
 *	errno			System error number.
 *
 * Implicit Outputs:
 *
 *	byte_count		Actual byte count returned by read().
 *
 * Considerations:
 *
 *      NONE
 *
 * Module Test Plan:
 *
 *	Verified in ipc_test.c
 *
 * Revision History:
 *
 *	D. F. Reed		27-Sep-1988	Original.
 *	D. F. Reed		31-Mar-1989	Modified to assume a stream
 *			socket on input.  Reads the IPC_HEADER first, which
 *			contains message size and then reads the balance.
 *	Jim Montgomery		04-May-1989	Added Write_Queue flush.
 *	D. L. Trachy		12-Jun-1990	Added shared memory ipc
 *	D. L. Trachy		26-Jul-1990	Added check EINTR
 *      Ken Stickney            02-Feb-1994     Changed algorithm for 
 *                                              reading all of the data
 *                                              from the socket. Multiple
 *                                              reads now made, if necessary.
 *                                              Algorithm was customer-
 *                                              generated.
 *      Ken Stickney            04-Feb-1994     Changed cl_log_event calls
 *                                              to MLOG calls.
 *      Ken Stickney            15-Jun-1994     Got rid of diagnostic MLOG
 *                                              calls.
 *      Van Lepthien            27-Aug-2001     Correct processing if read
 *                                              gets no bytes.
 *	Mitch Black		28-Mar-2004	Corrected handling of EINTR to 
 *			retry rather than cancel.  Needed for AIX, as BSD 
 *			based OS's handle the retry at the system level.
 *      Mike Williams           28-May-2010     Added include of sys/socket.h to
 *                                              get the prototype for accept.
 *                                              Changed the accept parameter
 *                                              from pointer to struct
 *                                              sockaddr_in to pointer to struct
 *                                              sockaddr.
 *
 */
/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef IPC_SHARED
#include <netinet/in.h>
#endif /* IPC_SHARED */

#include "cl_pub.h"
#include "cl_ipc_pub.h"
#include "sblk_pub.h"
#include "ml_pub.h"

/*
 *	Defines, Typedefs and Structure Definitions:
 */
/*
 *      Global and Static Variable Declarations:
 */
/*
 *      Procedure Type Declarations:
 */

STATUS 
cl_ipc_read (
    char buffer[],			/* pointer to input buffer area */
    int *byte_count			/* pointer to actual byte_count */
					/* returned from read() */
)
{
#ifdef DEBUG
    int ii;
#endif

#ifndef IPC_SHARED
    register int nsd, cnt, bytesleft;
    struct sockaddr name;
    int name_len = sizeof(name);
    IPC_HEADER *ipc = (IPC_HEADER *)buffer;
    char                *bp, lbuf[128];
    BOOLEAN status, frag = FALSE;
#endif
    IDENTIFIER identifier;
    int shared_memory_block;

#ifdef DEBUG
    if TRACE(0)
        cl_trace("cl_ipc_read",		/* routine name */
                 2,			/* parameter count */
                 (unsigned long)buffer,
                 (unsigned long)byte_count);
#endif /* DEBUG */

#ifdef IPC_SHARED
    /* read packet buffer */
    while (read(sd_in, &shared_memory_block, sizeof(shared_memory_block)) !=
      sizeof (shared_memory_block)) 
    {

        /* cancelled? */
        if (errno == EINTR) {
		MLOG((MMSG(486, "cl_ipc_read: shared memory read() returned EINTR (errno=%d); Retrying...\n"), errno));
		continue;
		/* return(STATUS_CANCELLED); */
	}
        MLOG((MMSG(484,"cl_ipc_read: Failed datagram read, errno = %d.\n"),
         errno));

        strncpy(identifier.socket_name, my_sock_name, SOCKET_NAME_SIZE);
        cl_inform(STATUS_IPC_FAILURE, TYPE_IPC, &identifier, errno);
        return(STATUS_IPC_FAILURE);
    }

    /* read message out of shared memory */
    if (cl_sblk_read(buffer,shared_memory_block,byte_count) != TRUE) 
    {
        MLOG((MMSG(485,"cl_ipc_read: shared_block_read failed, errno = %d.\n"),
            errno));
        strncpy(identifier.socket_name, my_sock_name, SOCKET_NAME_SIZE);
        cl_inform(STATUS_IPC_FAILURE, TYPE_IPC, &identifier, errno);
        return(STATUS_IPC_FAILURE);
    }

#else
	/* accept connection */
	/* Changed "if" here to "while" in order to retry accept on EINTR */
    
	while ((nsd = accept(sd_in, &name, &name_len)) < 0) 
	{
		/* EINTR received during system accept call */
		if (errno == EINTR)
		{
			MLOG((MMSG(486, "cl_ipc_read: accept() returned EINTR (errno=%d); Retrying...\n"), errno));
			continue;
			/* return(STATUS_CANCELLED); */
		}
		
		/* log error */
		MLOG((MMSG(486, "cl_ipc_read: accept() failed, errno=%d\n"), errno));
		strncpy(identifier.socket_name, my_sock_name, SOCKET_NAME_SIZE);
		cl_inform(STATUS_IPC_FAILURE, TYPE_IPC, &identifier, errno);
		
		return(STATUS_IPC_FAILURE);
	}
	
	bp = buffer;
	bytesleft = sizeof(IPC_HEADER);
	
	while (bytesleft != 0) 
	{
		cnt = read(nsd, bp, bytesleft);
		
#ifdef DEBUG
                printf("cl_ipc_read_d: socket=%d read(%d) (header), cnt=%d \n%d: ",
                       nsd,bytesleft,cnt,nsd);
                for (ii = 0 ; ii < cnt ; ii++)
                {
                    if (bp[ii] < 16)
                        printf(" 0%X",bp[ii]);
                    else
                        printf(" %X",bp[ii]);
        	    if ((ii+1)%8 == 0)
                        printf("   ");
        	    if ((ii+1)%32 == 0)
                        printf("\n%d: ",nsd);
                } 
                printf("\n\n");
        	
#endif 
		
		if (cnt == 0) 
		{
			close(nsd);
			if (bytesleft == sizeof(IPC_HEADER))
			{
				return STATUS_PENDING;
			}
			else
			{   
				/* client has gone away */
				MLOG((MMSG(488, "cl_ipc_read: read() returned 0, read terminated") ));
				return(STATUS_TERMINATED);
			}
		}
		/* oops ... system error or got fragmented packet */
		else if (cnt < 0) 
		{
			if (errno == EAGAIN)
			{
				continue;
			}
			else if (errno == EINTR)
			{
				MLOG((MMSG(486, "cl_ipc_read: read() returned EINTR (errno=%d); Retrying...\n"), errno));
				continue;
				/* close(nsd); */
				/* return(STATUS_CANCELLED); */
			}
			else
			{
				close(nsd);
				MLOG((MMSG(487, "cl_ipc_read: read() failed, errno=%d\n"), 
					errno));
				strncpy(identifier.socket_name, my_sock_name, SOCKET_NAME_SIZE);
				cl_inform(STATUS_IPC_FAILURE, TYPE_IPC, &identifier, errno);
				return(STATUS_IPC_FAILURE);
			}
		} 
		else if (cnt < bytesleft) 
		{
			bp += cnt;
			bytesleft -= cnt;
			frag = TRUE;
		}
		else if (cnt == bytesleft) 
		{
			frag = FALSE;
			bytesleft -= cnt;
		}
		else 
		{
			MLOG((MMSG(490,
				"cl_ipc_read: read IPC hdr bad pkt: exp=%d got=%d"),
				bytesleft, cnt));
			close(nsd);
			return(STATUS_IPC_FAILURE);
		}
	}
	
	/* check header byte_count */
	if ((ipc->byte_count == 0) || (ipc->byte_count > MAX_MESSAGE_SIZE)) 
	{
		
		/* make sure new descriptor is closed before return */
		close(nsd);
		
		MLOG((MMSG(491,"cl_ipc_read: invalid byte_count detected\n")));
		return(STATUS_IPC_FAILURE);
	}
	
	/* calculate and get the rest */
	bytesleft = ipc->byte_count - sizeof(IPC_HEADER);
	bp = buffer + sizeof(IPC_HEADER);
	
	while (bytesleft != 0) 
	{
		cnt = read(nsd, bp, bytesleft);
#ifdef DEBUG
                printf("cl_ipc_read_d: socket=%d read(%d) (data), cnt=%d \n%d: ",
                       nsd,bytesleft,cnt,nsd);
                for (ii = 0 ; ii < cnt ; ii++)
                {
                    if (bp[ii] < 16)
                        printf(" 0%X",bp[ii]);
                    else
                        printf(" %X",bp[ii]);
        	    if ((ii+1)%8 == 0)
                        printf("   ");
        	    if ((ii+1)%32 == 0)
                        printf("\n%d: ",nsd);
         } 
         printf("\n\n");
        	
#endif 
		
		if (cnt == 0) 
		{
			/* client has gone away */
			close(nsd);
			MLOG((MMSG(488, "cl_ipc_read: read() returned 0, read terminated") ));
			return(STATUS_TERMINATED);
		} 
		else if (cnt < 0) 
		{
			if (errno = EAGAIN)
			{
				continue;
			}
			else if (errno == EINTR)
			{
				MLOG((MMSG(486, "cl_ipc_read: read() returned EINTR (errno=%d); Retrying...\n"), errno));
				continue;
				/* close(nsd); */
				/* return(STATUS_CANCELLED); */
			}
			else
			{
				close(nsd);
				MLOG((MMSG(487, "cl_ipc_read: read() failed, errno=%d\n"), 
					errno));
				strncpy(identifier.socket_name, my_sock_name, SOCKET_NAME_SIZE);
				cl_inform(STATUS_IPC_FAILURE, TYPE_IPC, &identifier, errno);
				return(STATUS_IPC_FAILURE);
			}
		} 
		else if (cnt < bytesleft) 
		{
			bp += cnt;
			bytesleft -= cnt;
			frag = TRUE;
		} 
		else if (cnt == bytesleft) 
		{
			frag = FALSE;
			bytesleft -= cnt;
		} 
		else 
		{
			MLOG((MMSG(490,
				"cl_ipc_read: read IPC hdr bad pkt: exp=%d got=%d"),
				bytesleft, cnt));
			close(nsd);
			return(STATUS_IPC_FAILURE);
		}
	}
	
	/* looks ok, set byte count and return */
	*byte_count = ipc->byte_count;
	
	/* make sure new descriptor is closed before return */
	close(nsd);

#endif /* IPC_SHARED */
    /* Outbound messages may have been interrupted to process this input.
     *    Flush the Write_Queue 
     */
    (void)cl_ipc_send((char *)NULL, (char *)NULL, 0, 0);

    return(STATUS_SUCCESS);
}
