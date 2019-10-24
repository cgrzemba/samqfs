#ifndef lint
static char *_csrc = "@(#) %filespec: csi_ipc_send.c,2 %  (%full_filespec: csi_ipc_send.c,2:csrc:1 %)";
static char SccsId[] = "@(#) %filespec: csi_ipc_send.c,2 %  (%full_filespec: csi_ipc_send.c,2:csrc:1 %)";
#endif
/*
 * Copyright (C) 1995,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Name:
 *      csi_ipc_send()
 *      
 * Description:
 *      Dispatcher of a network failure reponse back to client, if 
 *      there is a network failure in transmitting a request to the
 *      ACSLS server.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      NONE
 *
 * Implicit Outputs:
 *      NONE
 *
 * Considerations:
 *      NONE
 *
 * Revision History:
 *      Ken Stickney    5-Jan-1995      Original.
 *      Mike Williams   01-Jun-2010     Included string.h and cl_ipc_pub.h to
 *                                      remedy warnings.
 */


/* Header Files: */
#include <string.h>
#include "csi.h"
#include "ml_pub.h"
#include "cl_pub.h"
#include "cl_ipc_pub.h"

static char     *st_module = "csi_ipc_send()";

/* Procedure Type Declarations */

void csi_ipc_send(CSI_MSGBUF *netbufp)
{

    CSI_REQUEST_HEADER csi_req_hdr;
    IPC_HEADER *ipc_header_qp;
    IPC_HEADER ipc_header;
    char * net_datap;
    char * ipc_datap;
    char appl_socket[SOCKET_NAME_SIZE];
    char * directp;
    STATUS status;

    directp = "To Client";

    /* set up pointers */
    net_datap  = CSI_PAK_NETDATAP(netbufp);

    /* extract addressing information from the network data */
    csi_req_hdr = *(CSI_REQUEST_HEADER *)net_datap;

    /* get the return ipc address of the application */
    if ((status = csi_qget(csi_lm_qid, csi_req_hdr.csi_header.ssi_identifier,
                                (void **)&ipc_header_qp)) != STATUS_SUCCESS) {
       MLOGCSI((status,  st_module, "csi_qget()",
         MMSG(958, "Message for unknown client discarded")));
       return;
    }
    ipc_header = *ipc_header_qp;
 
    /* set the name of the socket to write to */
    strncpy(appl_socket, ipc_header.return_socket,SOCKET_NAME_SIZE);
 
 
    /*
     * ipc transmission - common to both csi and ssi.
     */
 
    /* set ipc return address and sender module type */
    strcpy(ipc_header.return_socket, my_sock_name);
    ipc_header.module_type = my_module_type;
 
    /* set up a pointer to the ipc data in the buffer */
    ipc_datap = CSI_PAK_IPCDATAP(netbufp);
 
    /* convert the net format packet to ipc format */
    * (IPC_HEADER *)ipc_datap = ipc_header;
 
    /* packet size changes by taking off csi_header & adding ipc_header */
    netbufp->size += sizeof(IPC_HEADER) - sizeof(CSI_HEADER);
 
    /* send packet via ipc */
    if (cl_ipc_write(appl_socket, ipc_datap, netbufp->size)
                                                    != STATUS_SUCCESS) {
        MLOGCSI((STATUS_IPC_FAILURE,  st_module, CSI_NO_CALLEE,
          MMSG(959, "Cannot send message %s:discarded"), directp));
        return;
    }

}
