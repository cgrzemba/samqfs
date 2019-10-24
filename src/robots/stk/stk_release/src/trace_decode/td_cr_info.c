#ifndef lint
static char SccsId[] = "@(#)td_cr_info.c        2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_critical_info
 *
 * Description:
 *      print the critical infomation of the packet, 
 *      i.e. type of command, type of packet and byte count
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_packet_type.
 *
 * Implicit Outputs:
 *      td_packet_type.
 *
 * Considerations:
 *      NONE
 *
 * Revision History:
 *
 *      M. H. Shum          10-Sep-1993     Original.
 *      Joseph Nofi         15-Aug-2011     XAPI support;
 *                                          Do not print "Bytes in packet" info
 *                                          if -c (compare) option specified. 
 *
 */


/*
 * header files
 */
#include <stdio.h>
#include "td.h"

/*
 * global variables
 */

static char *packet_type[3] = {
    "REQUEST",
    "RESPONSE",
    "ACKNOWLEDGE"              /* type of packet character string */
}; 


void
td_critical_info(MESSAGE_HEADER *msg_header_ptr, unsigned byte_count)
{
    /* print type of command */
    printf("Type of command:     %s\n",
       cl_command(msg_header_ptr->command));

    /* print type of packet */
    if (msg_header_ptr->message_options  & ACKNOWLEDGE)
        td_packet_type = PACKET_ACK;
    printf("Type of Packet:      %s\n", packet_type[td_packet_type]);

    /* print byte count */
    if (td_output_format != COMPARE)
    {
        printf("Bytes in Packet:     %u\n\n", byte_count);
    }

}




