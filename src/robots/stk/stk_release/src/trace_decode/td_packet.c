#ifndef lint
static char SccsId[] = "@(#)td_packet.c         2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_decode_packet
 *
 * Description:
 *      Decode the message content of a packet.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_msg_buf.
 *
 * Implicit Outputs:
 *      td_msg_ptr.
 *
 * Considerations:
 *      NONE
 *
 * Revision History:
 *
 *      M. H. Shum          10-Sep-1993.    Original.
 *      Joseph Nofi         15-Aug-2011     XAPI support;
 *                                          Differentiate RPC, and ADI, from 
 *                                          IPC packets (IPC packets are logged 
 *                                          from XAPI client)
 *
 */



/*********************************************************************/
/*Includes:                                                          */
/*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "td.h"


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void st_ipc_header(IPC_HEADER *ipc_header);


void td_decode_packet(int byte_count, unsigned short packet_id)
{
    CSI_REQUEST_HEADER_RPC  *rpc_header_ptr;
    CSI_REQUEST_HEADER_ADI  *adi_header_ptr;
    IPC_REQUEST_HEADER_XAPI *xapi_header_ptr;
    MESSAGE_HEADER     *msg_header_ptr;
    VERSION             version;
    COMMAND             command;

    /* initializes the pointers */
    td_msg_ptr = td_msg_buf;

    /*****************************************************************/
    /* Test if this is really an IPC header (and not a CSI header).  */
    /* The test is if the IPC_HEADER.byte_count and packet_id        */
    /* are equal to the input byte_count and packet_id:              */
    /* If they are consistent, then we assume that the buffer is     */
    /* an IPC_HEADER type as logged by the XAPI component.           */
    /*****************************************************************/
    xapi_header_ptr = (IPC_REQUEST_HEADER_XAPI*) td_msg_buf;

#ifdef DEBUG

    printf("byte_count=%d, xapi_header_ptr->ipc_header.byte_count=%d\n",
           byte_count,
           xapi_header_ptr->ipc_header.byte_count);

    printf("packet_id=%d, xapi_header_ptr->message_header.packet_id=%d\n",
           packet_id,
           xapi_header_ptr->message_header.packet_id);

#endif


    if ((xapi_header_ptr->ipc_header.byte_count == byte_count) &&
        (xapi_header_ptr->message_header.packet_id == packet_id))
    {
        td_packet_header = IPC;
        msg_header_ptr = &(xapi_header_ptr->message_header);

        /*************************************************************/
        /* Calculate the difference between an RPC header and an     */
        /* IPC header and adjust the td_msg_ptr, so that an IPC      */
        /* header packet can be treated as an RPC header packet      */
        /*************************************************************/
        td_msg_ptr -= (offsetof(CSI_REQUEST_HEADER_RPC, message_content) - 
                       offsetof(IPC_REQUEST_HEADER_XAPI, message_content));
    }
    else if (td_adi_protocol)          /* ADI header                 */
    {
        td_packet_header = ADI;
        adi_header_ptr = (CSI_REQUEST_HEADER_ADI*) td_msg_buf;
        msg_header_ptr = &adi_header_ptr->message_header;
        /*************************************************************/
        /* Calculate the difference between an RPC header and an     */
        /* ADI header and adjust the td_msg_ptr, so that an ADI      */
        /* header packet can be treated as an RPC header packet      */
        /*************************************************************/
        td_msg_ptr -= (offsetof(CSI_REQUEST_HEADER_RPC, message_content) - 
                       offsetof(CSI_REQUEST_HEADER_ADI, message_content));
    }
    else                               /* RPC header                 */
    {
        td_packet_header = CSI;
        rpc_header_ptr = (CSI_REQUEST_HEADER_RPC*) td_msg_buf;
        msg_header_ptr = &rpc_header_ptr->message_header;
    }

#ifdef DEBUG

    printf("td_packet_header=%d\n",
           td_packet_header);

#endif

    /* print the critical infomation of the packet */
    td_critical_info(msg_header_ptr, byte_count);

    /* print the header (the description) */
    td_print_header();

    /* decode and print the IPC or CSI header */
    if (td_output_format != COMPARE)
    {
        if (td_packet_header == IPC)
        {
            st_ipc_header((IPC_HEADER*) &xapi_header_ptr->ipc_header);
        }
        else
        {
            if (td_packet_header == ADI)
            {
                td_csi_header_adi(&adi_header_ptr->csi_header);
            }
            else
            {
                td_csi_header_rpc(&rpc_header_ptr->csi_header);
            }
        }
    }

    /* decode the message header */
    version = td_msg_header(msg_header_ptr);

    /* decode the command */
    command = msg_header_ptr->command;

    switch (td_packet_type)
    {
    case PACKET_REQUEST:
        td_decode_req(version, command);
        break;

    case PACKET_RESPONSE:
        td_decode_resp(version, command);
        break;

    case PACKET_ACK:
        td_decode_ack(version);
        break;

    default:
        fputs("Invalid packet! Unable to decode\n", stdout);
        break;
    }

    /* dump the raw hex data */
    if (td_output_format != COMPARE)
    {
        td_hex_dump();
    }

    /* print separator */
    puts("\n==============================================================="
         "=================\n");
}


/*
 * Decode IPC header.
 */
static void st_ipc_header(IPC_HEADER *ipc_header)
{
    printf("\nIPC HEADER:\n");

    td_print("byte_count", &ipc_header->byte_count, sizeof(ipc_header->byte_count),
             td_ultoa(ipc_header->byte_count));

    td_print("module_type", &ipc_header->module_type, sizeof(ipc_header->module_type),
             cl_type(ipc_header->module_type));

    td_print("options", &ipc_header->options, sizeof(ipc_header->options),
             td_msg_opts(ipc_header->options));

    td_print("seq_num", &ipc_header->seq_num, sizeof(ipc_header->seq_num),
             td_ultoa(ipc_header->seq_num));

    td_print("return_socket", &ipc_header->return_socket, sizeof(ipc_header->return_socket),
             ipc_header->return_socket);

    td_print("return_pid", &ipc_header->return_pid, sizeof(ipc_header->return_pid),
             td_utoa(ipc_header->return_pid));

    td_print("ipc_identifier", &ipc_header->ipc_identifier, sizeof(ipc_header->ipc_identifier),
             td_utoa(ipc_header->ipc_identifier));

    td_print("requestor_type", &ipc_header->requestor_type, sizeof(ipc_header->requestor_type),
             cl_type(ipc_header->requestor_type));

    td_print("host_id", &ipc_header->host_id, sizeof(ipc_header->host_id),
             (char*) &ipc_header->host_id);

    return;
}


