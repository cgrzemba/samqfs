/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *  td_get_packet
 *
 * Description:
 *      get a complete packet from stdin, find out the packet type
 *      (REQUEST/RESPONSE/ACK) from the packet header, save the packet
 *      in td_ascii_buf, then convert it from ascii representation
 *      to binary representation and save the result in td_msg_buf.
 *
 * Return Values:
 *      None.
 *
 * Implicit Inputs:
 *      td_ascii_buf, td_msg_buf, td_packet_type
 *
 * Implicit Outputs:
 *      td_ascii_buf, td_msg_buf
 *
 * Considerations:
 *      This module assumes the packets begins with "0000:".
 *
 * Revision History:
 *
 *      M. H. Shum          10-Sep-1993     Original.
 *      S. L. Siao          22-Mar-1999     Allow to recognize 2 or 4 digit year.
 *      Van Lepthien        30-Aug-2001     Add to base code.
 *      Mitch Black         28-Dec-2004     Clear buffers before re-using.
 *                                          Not doing this caused different 
 *                                          problems under different OSs, including
 *                                          re-use of spurious data from one packet 
 *                                          to the next, or even core dumps.
 *      Joseph Nofi         15-Aug-2011     XAPI support;
 *                                          Added processing of XAPI hex packet types.  
 *                                          Added additional tests to determine
 *                                          whether input is non-hex display line.
 *                                          Added conversion of IPC_REQUEST_HDR_XAPI
 *                                          type buffers when ACSAPI requests and
 *                                          responsese are logged by the XAPI client;
 *                                          Extracted packet id from log and saved 
 *                                          in td_packet_id global variable.
 *
 */


/*
 * Header Files
 */
#include <stdio.h>
#include <string.h>

#include "td.h"
#include "srvcommon.h"

/*
 * Defines
 */
#define MAX_BYTE_LINE   SLOG_MAX_LINE_SIZE + 1 /* max num of byte per line */

/*
 * Global variables
 */
static int  max_trace_byte; /* the maximum byte count for the current packet */
static char max_trace_str[10];  /* the value of max_trace_byte in ASCII format*/

/*
 * Static Function Prototypes
 */
static void st_process_header(char *, char *);
static int  st_save_buf(char *, char *, char *);
static char st_hex2bin(char *);
static char st_h2d(char);
static int  st_date_time(char *);

/* 
 * Get a packet from stdin, save the raw data in td_ascii_buf, convert it
 * into binary format, save the result in td_msg_buf and call td_decode_packet
 * to decode it.
 */
void td_get_packet()
{
    char *msg_buf_ptr;                     /* pointer to td_msg_buf */
    char *ascii_buf_ptr;                   /* pointer to td_ascii_buf */
    char *data_buf_ptr;                    /* pointer to data_buf */
    char  packet_src_buf[MAX_BYTE_LINE];   /* packet source's buffer */
    char  packet_src_save[MAX_BYTE_LINE];  /* packet source's buffer */
    char  data_buf[MAX_BYTE_LINE];         /* message content's buffer */
    char  date_time_buf[MAX_BYTE_LINE];    /* date and time stamp's buffer */
    char  date_time_save[MAX_BYTE_LINE];   /* date and time stamp's buffer */
    int   byte_count = 0;                  /* the size of the  packet */
    int   count;
    char  packetIdString[6];
    unsigned short curr_packet_id;

    /* initialize */
    msg_buf_ptr = td_msg_buf;
    ascii_buf_ptr = td_ascii_buf;
    packet_src_buf[0] = 0;
    date_time_buf[0] = 0;

    while (1)
    {
        memset(data_buf, 0, sizeof(data_buf));
        data_buf_ptr = fgets(data_buf, MAX_BYTE_LINE, stdin);

        if ((data_buf_ptr == NULL) &&  /* got the last packet */
            (msg_buf_ptr != td_msg_buf))
        {
            /* process the header and decode the packet */
            st_process_header(date_time_save, packet_src_save);
            td_decode_packet(byte_count, curr_packet_id);
            break;
        }

#ifdef DEBUG

        printf("next line=%.8s\n",
               data_buf);

#endif

        if (memcmp(&(data_buf[0]),
                   "0000: ",
                   6) == 0)
        {

#ifdef DEBUG

            printf("found start of ACSAPI packet, packet_id=%d, "
                   "td_msg_buf=%08X, msg_buf_ptr=%08X\n",
                   curr_packet_id,
                   td_msg_buf,
                   msg_buf_ptr);

#endif

            if (msg_buf_ptr != td_msg_buf)             /* got a packet */
            {
                /* process the header and decode the packet */
                st_process_header(date_time_save, packet_src_save);
                td_decode_packet(byte_count, curr_packet_id);
                /* re-initialize these variables for the next packet */
                msg_buf_ptr = td_msg_buf;
                ascii_buf_ptr = td_ascii_buf;
                byte_count = 0;
                /* Clear out buffers to prevent re-use of data for next packet */
                memset(td_msg_buf, 0, MAX_MESSAGE_SIZE);
                memset(td_ascii_buf, 0, MAX_MESSAGE_SIZE*2);
            }
            /* save the packet source and the date and time stamp */
            curr_packet_id = td_packet_id;
            memcpy(packet_src_save, packet_src_buf, MAX_BYTE_LINE); 
            memcpy(date_time_save, date_time_buf, MAX_BYTE_LINE);
            /* re-initialize them */
            packet_src_buf[0] = 0;
            date_time_buf[0] = 0;
        }

        if ((memcmp(&(data_buf[4]),        /* ACSAPI packet content */
                    ": ",
                    2) == 0) &&
            (isdigit(data_buf[0])) &&
            (isdigit(data_buf[1])) &&
            (isdigit(data_buf[2])) &&
            (isdigit(data_buf[3])))
        {
            /* convert and save the data */
            count = st_save_buf(ascii_buf_ptr, msg_buf_ptr, data_buf);
            msg_buf_ptr += count;
            ascii_buf_ptr += count * 2;
            byte_count += count;

#ifdef DEBUG

            printf("After data convert, "
                   "td_msg_buf=%08X, msg_buf_ptr=%08X, count=%d\n",
                   td_msg_buf,
                   msg_buf_ptr,
                   count);
#endif

        }
        else if (st_date_time(data_buf)) /* date and time stamp */
        {
            strcpy(date_time_buf, data_buf);
        }
        else if (strstr(data_buf, SLOG_PACKET_SOURCE)) /* packet source */
        {
            strcpy(packet_src_buf, data_buf);
        }
        else if ((strstr(data_buf, SLOG_PACKET_ID)) ||
                 (strstr(data_buf, SLOG_SSI_IDENTIFIER))) /* packet id */
        {
            memset(packetIdString, 0, sizeof(packetIdString));
            memcpy(packetIdString, &(data_buf[22]), 5);
            td_packet_id = (unsigned short) atoi(packetIdString);

#ifdef DEBUG

            printf("found packet_id=%d\n", td_packet_id);

#endif

        }
    }
}

/*
 * Process the date and time stamp and the packet source
 */
static void st_process_header(char *date_time, char *p_source)
{
    /* if date and time stamp is not blank, print it */
    if ((*date_time) &&
        (td_output_format != COMPARE))
    {
        fputs(date_time, stdout);
    }

    td_packet_type = PACKET_RESPONSE; /* default to response */

    /* if packet source is not blank, print it */
    if (*p_source)
    {
        fputs(p_source, stdout);

        if ((strstr(p_source, SLOG_TO_HTTP)) || 
            (strstr(p_source, SLOG_FROM_ACSAPI)) ||
            (strstr(p_source, SLOG_TO_CSI)))
        {
            td_packet_type = PACKET_REQUEST;
        }
    }
    else
    {
        fputs("Packet source:\tMISSING-default to response\n", stdout);
    }
}

/*
 * Check to see if start of the buffer contains a date and time stamp.
 */
static int st_date_time(char *buf)
{
    /* looks for this string : "xx-xx-xx xx:xx:xx " */
    if ((buf[2] == '-') && 
        (buf[5] == '-') && 
        (buf[8] == ' ') && 
        (buf[11] == ':') &&
        (buf[14] == ':'))
    {
        return 1;
    }

    return 0;
}

/*
 * Save the raw data from a temporary buffer to td_msg_buf,
 * convert character hex data to binary data.
 * The raw data looks is in 1 or 2 formats:
 *
 * (1) "0010:   00 0A 00 00 FF 00 00 00 10 00", or
 * (2) "0010:  02000010 00000000 00001900 00000000 00000000 000000A0 00000000 000000FF"
 *
 * Type (1) format is the trace.log file format (pre-XAPI)
 * Type (2) format is the log.file file format (XAPI)
 * 
*/
static int st_save_buf(char *ascii_buf_ptr, char *msg_buf_ptr, char *raw_data)
{
    char               *work_ptr            = msg_buf_ptr;  
    int                 count;
    int                 i;
    char                hexwork[72];
    char               *pHexwork;

    /* Determine if format (1) or format (2) type of hexadecimal data */
    /* blank in position 7, 10, and 13 mean its a format (1) type */
    if ((raw_data[7] == ' ') &&
        (raw_data[10] == ' ') &&
        (raw_data[13] == ' ')) 
    {
        raw_data += 8;     /* skip the byte count, i.e. 0010:    */       
        count = 29;        /* Max hex bytes is (2 * 10) data + 9 spaces */ 
    }
    else
    {
        raw_data += 7;     /* skip the byte count, i.e. 0010:    */
        count = 71;        /* Max hex bytes is (8 * 8) data + 7 spaces */
    }

    memset(hexwork, 0, sizeof(hexwork));
    memcpy(hexwork, raw_data, count);
    td_rm_space(hexwork);

    for (i = 0;
        i < strlen(hexwork);
        i++)
    {
        hexwork[i] = toupper(hexwork[i]);
    }

    strcpy(ascii_buf_ptr, hexwork);    /* save the hex data in td_ascii_buf */

    count = 0;
    pHexwork = &(hexwork[0]);
    while (*pHexwork != 0)
    {
        *work_ptr = st_hex2bin(pHexwork);
        work_ptr++;
        count++;
        pHexwork += 2;                 /* skip 2 hex bytes */
    }

#ifdef DEBUG

    printf("input  hex=%.64s\n",
           hexwork);
    printf("output hex=");
    for (i = 0;
        i < count;
        i++)
    {
        printf("%02X",
               msg_buf_ptr[i]);
    }
    printf("\n");

#endif

    return count;
} 

/*
 * Convert two hex characters to a number
 */
static char st_hex2bin(char *a_ptr)
{
    return((st_h2d(*a_ptr) << 4) + st_h2d(*(a_ptr + 1)));
}

/*
 * Convert a hex character to a decimal
 */
static char st_h2d(char h)
{
    return(h >= 'A' && h <= 'F') ? (10 + h - 'A') : (h - '0');
}
