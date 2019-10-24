#ifndef lint
static char SccsId[] = "@(#)td_csi_head.c       2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_csi_header
 *
 * Description:
 *      Decode and output a csi header, can be ADI/RPC.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_adi_packet.
 *
 * Implicit Outputs:
 *      NONE
 *
 * Considerations:
 *      The character string for csi_syntax, csi_protocol and csi_connect
 *      are hard coded,
 *      when their enum value is updated, they need to update with them.
 *
 * Revision History:
 *
 *      M. H. Shum          10-Sep-1993     Original.
 *      Joseph Nofi         15-Aug-2011     Fix "CSI HEADER" literal.
 *
 */

/* Header files */

#include  <stdio.h>
#include  <sys/types.h>
#include  <netinet/in.h>
#include  <arpa/inet.h>

#include  "csi.h"
#include  "csi_adi_header.h"
#include  "csi_rpc_header.h"
#include  "td.h"

/* Global Variables */

static char *csi_syntax[2] = {
    "CSI_SYNTAX_NONE",
    "CSI_SYNTAX_XDR"
};

static char *csi_protocol[4] = {
    "INVALID ENTRY",
    "CSI_PROTOCOL_TCP",
    "CSI_PROTOCOL_UDP",
    "CSI_PROTOCOL_ADI"
};

static char *csi_connect[3] = {
    "INVALID ENTRY",
    "CSI_CONNECT_RPCSOCK",
    "CSI_CONNECT_ADI"
};

/*
 * Decode RPC CSI header.
 */
void
td_csi_header_rpc(CSI_HEADER_RPC *csi_head)
{
    printf("\nCSI HEADER:\n");

    /* print CSI_XID */
    td_print("xid.addr", &csi_head->xid.addr, sizeof(csi_head->xid.addr),
         td_net_addr(csi_head->xid.addr));
    
    td_print("xid.pid", &csi_head->xid.pid, sizeof(csi_head->xid.pid),
         td_utoa(csi_head->xid.pid));
    td_print("xid.seq_num", &csi_head->xid.seq_num, sizeof(csi_head->xid.seq_num),
         td_ultoa(csi_head->xid.seq_num));

    /* print ssi_identifier */
    td_print("ssi_identifier", &csi_head->ssi_identifier, 
         sizeof(csi_head->ssi_identifier), td_ultoa(csi_head->ssi_identifier));

    /* print CSI_SYNTAX */
    if (csi_head->csi_syntax >= CSI_SYNTAX_NONE &&
    csi_head->csi_syntax <= CSI_SYNTAX_XDR)
    td_print("syntax", &csi_head->csi_syntax, sizeof(csi_head->csi_syntax),
         csi_syntax[csi_head->csi_syntax]);
    else
    td_print("syntax", &csi_head->csi_syntax, sizeof(csi_head->csi_syntax),
         "INVALID ENTRY");

    /* print CSI_PROTOCOL */
    if (csi_head->csi_proto >= CSI_PROTOCOL_TCP &&
    csi_head->csi_proto <= CSI_PROTOCOL_ADI)
    td_print("protocol", &csi_head->csi_proto, sizeof(csi_head->csi_proto),
         csi_protocol[csi_head->csi_proto]);
    else
    td_print("protocol", &csi_head->csi_proto, sizeof(csi_head->csi_proto),
         "INVALID ENTRY");
    
    /* print CSI_CONNECT */
    if (csi_head->csi_ctype >= CSI_CONNECT_RPCSOCK &&
    csi_head->csi_ctype <= CSI_CONNECT_ADI)
    td_print("connect_type", &csi_head->csi_ctype, sizeof(csi_head->csi_ctype),
         csi_connect[csi_head->csi_ctype]);
    else
    td_print("connect_type", &csi_head->csi_ctype, sizeof(csi_head->csi_ctype),
         "INVALID ENTRY");

    /* print CSI_HANDLE_RPC */
    putc('\n', stdout);
    td_print("handle.program", &csi_head->csi_handle.program, 
         sizeof(csi_head->csi_handle.program), 
         td_ultoa(csi_head->csi_handle.program));
    td_print("handle.version", &csi_head->csi_handle.version, 
         sizeof(csi_head->csi_handle.version), 
         td_ultoa(csi_head->csi_handle.version));
    td_print("handle.proc", &csi_head->csi_handle.proc, 
         sizeof(csi_head->csi_handle.proc), 
         td_ultoa(csi_head->csi_handle.proc));
    
    /* print CSI_HANDLE_RPC - return internet address */
    putc('\n', stdout);
    td_print("raddr.sin.family", &csi_head->csi_handle.raddr.sin_family,
         sizeof(csi_head->csi_handle.raddr.sin_family),
         td_stoa(csi_head->csi_handle.raddr.sin_family));
    td_print("raddr.sin.port", &csi_head->csi_handle.raddr.sin_port,
         sizeof(csi_head->csi_handle.raddr.sin_port),
         td_ustoa(csi_head->csi_handle.raddr.sin_port));
    td_print("raddr.sin_addr", &csi_head->csi_handle.raddr.sin_addr,
       sizeof(csi_head->csi_handle.raddr.sin_addr),
       inet_ntoa(csi_head->csi_handle.raddr.sin_addr));
}

/*
 * Decode ADI CSI header.
 */
void
td_csi_header_adi(CSI_HEADER_ADI *csi_head)
{
    printf("\nCSI HEADER:\n");

    /* print CSI_XID */
    td_print("xid.client_name", &csi_head->xid.client_name,
         sizeof(csi_head->xid.client_name),
         (char *) csi_head->xid.client_name);
    td_print("xid.procedure", &csi_head->xid.proc, sizeof(csi_head->xid.proc),
         td_ultoa(csi_head->xid.proc));
    
    td_print("xid.pid", &csi_head->xid.pid, sizeof(csi_head->xid.pid),
         td_utoa(csi_head->xid.pid));
    td_print("xid.seq_num", &csi_head->xid.seq_num, sizeof(csi_head->xid.seq_num),
         td_ultoa(csi_head->xid.seq_num));

    /* print ssi_identifier */
    td_print("ssi_identifier", &csi_head->ssi_identifier, 
         sizeof(csi_head->ssi_identifier), td_ultoa(csi_head->ssi_identifier));

    /* print CSI_SYNTAX */
    if (csi_head->csi_syntax >= CSI_SYNTAX_NONE &&
    csi_head->csi_syntax <= CSI_SYNTAX_XDR)
    td_print("syntax", &csi_head->csi_syntax, sizeof(csi_head->csi_syntax),
         csi_syntax[csi_head->csi_syntax]);
    else
    td_print("syntax", &csi_head->csi_syntax, sizeof(csi_head->csi_syntax),
         "INVALID ENTRY");

    /* print CSI_PROTOCOL */
    if (csi_head->csi_proto >= CSI_PROTOCOL_TCP &&
    csi_head->csi_proto <= CSI_PROTOCOL_ADI)
    td_print("protocol", &csi_head->csi_proto, sizeof(csi_head->csi_proto),
         csi_protocol[csi_head->csi_proto]);
    else
    td_print("protocol", &csi_head->csi_proto, sizeof(csi_head->csi_proto),
         "INVALID ENTRY");
    
    /* print CSI_CONNECT */
    if (csi_head->csi_ctype >= CSI_CONNECT_RPCSOCK &&
    csi_head->csi_ctype <= CSI_CONNECT_ADI)
    td_print("connect_type", &csi_head->csi_ctype, sizeof(csi_head->csi_ctype),
         csi_connect[csi_head->csi_ctype]);
    else
    td_print("connect_type", &csi_head->csi_ctype, sizeof(csi_head->csi_ctype),
         "INVALID ENTRY");

    /* print CSI_HANDLE_ADI */
    td_print("handle.client_name", &csi_head->csi_handle.client_name,
         sizeof(csi_head->csi_handle.client_name),
         (char *) csi_head->csi_handle.client_name);
    td_print("handle.procedure", &csi_head->csi_handle.proc,
         sizeof(csi_head->csi_handle.proc),
         td_ultoa(csi_head->csi_handle.proc));
}





