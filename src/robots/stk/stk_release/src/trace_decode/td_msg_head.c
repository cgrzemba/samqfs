#ifndef lint
static char SccsId[] = "@(#)td_msg_head.c       2.2 3/26/02 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_msg_header
 *
 * Description:
 *      Decode and print the message header.
 *
 * Return Values:
 *      version.
 *
 * Implicit Inputs:
 *      td_mopts, td_xopts, td_packet_type.
 *
 * Implicit Outputs:
 *      td_mopts, td_xopts, td_packet_type.
 *
 * Considerations:
 *      Message options and extended message options are hard coded, they need to 
 *      updated with defs.h.
 *
 * Revision History:
 *
 *      M. H. Shum          10-Sep-1993     Original.
 *      S. L. Siao          26-Jan-2002     Added Virtually Aware handling
 *      Joseph Nofi         15-Aug-2011     XAPI support;
 *                                          Converted st_msg_opts static function
 *                                          into td_msg_opts extern function.
 *                                         
 *
 */


/*
 * Header Files
 */
#include <string.h>
#include "td.h"

/*
 * Define
 */
#define MAXOPTIONS  50

/*
 * Global/Static variables
 */
static char str_buffer[MAXOPTIONS];

/*
 * local function prototype
 */
static char *st_version(VERSION);
static char *st_ext_opts(unsigned long);


/*
 * decode and print the message header.
 */
VERSION
td_msg_header(MESSAGE_HEADER  *msg_head)
{
    VERSION  ver;

    printf("\nMESSAGE HEADER:\n");

    td_print("packet_id", &msg_head->packet_id, sizeof(msg_head->packet_id),
         td_ustoa(msg_head->packet_id));
    td_print_command(&msg_head->command);
    
    td_print("message_options", &msg_head->message_options,
         sizeof(msg_head->message_options),
         td_msg_opts(msg_head->message_options));
    td_mopts = msg_head->message_options; /* save message_options for future use */
    
    /* only output the rest of the data if the extended bit is set,
       (version 0 has no extended bit set) */

    if (msg_head->message_options & EXTENDED) {
        ver = msg_head->version;
        td_print("version", &msg_head->version, sizeof(msg_head->version),
         st_version(msg_head->version));
        td_print("extended_options", &msg_head->extended_options,
         sizeof(msg_head->extended_options),
         st_ext_opts(msg_head->extended_options));
        td_print_lockid(&msg_head->lock_id);
    td_print_userid(&msg_head->access_id.user_id);
        td_print("password", &msg_head->access_id.password.password,
         sizeof(msg_head->access_id.password.password),
         msg_head->access_id.password.password);
    td_xopts = msg_head->extended_options;
    }
    else
        ver = 0;
    
    return ver; 
}

/*
 * decode message options in message header.
 */
char *td_msg_opts(unsigned char msgopt)
{
    if (msgopt == 0)
    strcpy(str_buffer, "NONE");
    else {
    str_buffer[0] = 0;
    
        if (msgopt & FORCE)
            strcat(str_buffer, "FORCE|");
        if (msgopt & INTERMEDIATE)
            strcat(str_buffer, "INTERMEDIATE|");
        if (msgopt & ACKNOWLEDGE) {
            strcat(str_buffer, "ACKNOWLEDGE|");
        } 
        if (msgopt & READONLY)
            strcat(str_buffer, "READONLY|");
        if (msgopt & BYPASS)
            strcat(str_buffer, "BYPASS|");
        if (msgopt & EXTENDED)
            strcat(str_buffer, "EXTENDED|");
        if (msgopt & VIRTAWARE)
            strcat(str_buffer, "VIRTAWARE|");

        /* Check for bad value (none of the above) */
        if (str_buffer[0] == 0)
        strcpy(str_buffer, "INVALID OPTION");
        else
            str_buffer[strlen(str_buffer)-1] = 0; /* don't need the last | */
    }
    return str_buffer; 
}

static char *
st_version(VERSION ver)
{
    sprintf(str_buffer, "VERSION%d", ver);
    return str_buffer;
}

static char *
st_ext_opts(unsigned long extopt)
{
    if (extopt == '\0') {
    strcpy(str_buffer, "NONE");
    }
    else {
    str_buffer[0] = 0;
    
        if (extopt & WAIT)  
            strcat(str_buffer, "WAIT|");
        if (extopt & RESET)
            strcat(str_buffer, "RESET|");
        if (extopt & VIRTUAL)
            strcat(str_buffer, "VIRTUAL|");
        if (extopt & CONTINUOUS)
            strcat(str_buffer, "CONTINUOUS|");
        if (extopt & RANGE)
            strcat(str_buffer, "RANGE|");
        if (extopt & CLEAN_DRIVE)
            strcat(str_buffer, "CLEAN_DRIVE|");

        /* Check for bad value (none of the above) */
        if (str_buffer[0] == 0)
        strcat(str_buffer, "INVALID OPTION");
        else
            str_buffer[strlen(str_buffer)-1] = 0;
    }
    return str_buffer;
}







