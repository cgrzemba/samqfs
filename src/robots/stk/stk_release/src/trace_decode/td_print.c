#ifndef lint
static char SccsId[] = "@(#)td_print.c          2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_print
 *
 * Description:
 *      This module format and output a line of data.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_msg_buf, td_ascii_buf, td_output_format
 *
 * Implicit Outputs:
 *      NONE
 *
 * Considerations:
 *      NONE
 *
 * Revision History:
 *
 *      M. H. Shum          10-Sep-1993     Original.
 *      Joseph Nofi         15-Aug-2011     Change MAX_FORM_DATA from 26 to
 *                                          30 to fix possible line truncation.
 *
 */


#include <stdio.h>
#include <string.h>
#include "td.h"

/*
 * external variable
 */
extern FORMAT td_output_format;
 
/*
 * Format and output the data
 * For short format, output the description and data
 * For long format, output the description, offset, size, hex data and ascii data
 */
void
td_print(char *desc, void *bptr, size_t byte_cnt, char *fdata)
{
#define MAX_ASCII_DATA  8       /* maximum number of ASCII data per line */
#define MAX_HEX_DATA    16  /* MAX_HEX_DATA = MAX_ASCII_DATA * 2 */
#define MAX_FORM_DATA   30  /* maximum number of formatted data per line */
    
    if (td_output_format == SHORT) {
    printf("    %-25s%s\n", desc, fdata);
    }
    else {
    unsigned offset,            /* data offset */
             hpsize, fpsize;    /* hex and formatted data print size */
    int h_count = 0,        /* number of hex data printed */
        f_count = 0,                /* number of formatted data printed */
        fsize, hsize;       /* size of formatted data and hex data */
    char hex_data[MAX_HEX_DATA+1];  /* hex data buffer */
    char form_data[MAX_FORM_DATA+1];/* formatted data buffer */
    char *hdata;                /* pointer to the hex data */

    fsize = strlen(fdata);
    offset = (unsigned) ((char *)bptr - (char *)td_msg_buf);
    fpsize = (fsize <= MAX_FORM_DATA) ? fsize : MAX_FORM_DATA;
    hpsize = (byte_cnt <= MAX_ASCII_DATA) ? byte_cnt * 2 : MAX_HEX_DATA;
    hdata = &td_ascii_buf[offset*2]; /* pointer to the hex data */
    
    /* print the first line */
    memset(form_data, 0, MAX_FORM_DATA+1);
    memset(hex_data, 0, MAX_HEX_DATA+1);
    strncpy(form_data, fdata, fpsize);
    strncpy(hex_data, hdata, hpsize);

    printf("    %-20s[%04u:%02d]  0x%-17s%s\n", desc, offset, byte_cnt, 
           hex_data, form_data);

    hsize = byte_cnt * 2;
    h_count += hpsize;
    f_count += fpsize;

    /* print the other data */
    while (h_count < hsize || f_count < fsize) {
        memset(hex_data, 0, MAX_HEX_DATA+1);
        if (h_count < hsize) {            /* print hex data */
        hdata += hpsize;              /* increment hex data pointer */
        hpsize = hsize - h_count;     /* calculate the print size */
        if (hpsize > MAX_HEX_DATA)
            hpsize = MAX_HEX_DATA;
        strncpy(hex_data, hdata, hpsize);
        h_count += hpsize;
        }
        memset(form_data, 0, MAX_FORM_DATA+1);
        if (f_count < fsize) {            /* print formatted data */
        fdata += fpsize;              /* increment formatted data ptr */
        fpsize = fsize - f_count;     /* calculate the print size */
        if (fpsize > MAX_FORM_DATA)
            fpsize = MAX_FORM_DATA;
        strncpy(form_data, fdata, fpsize);
        f_count += fpsize;
        }
        printf("                                     %-17s%s\n", hex_data,
           form_data);
    }
    }

#undef MAX_ASCII_DATA
#undef MAX_HEX_DATA
#undef MAX_FORM_DATA
}

/*
 * Print a header to descript every column
 */
void
td_print_header(void)
{
    if (td_output_format == SHORT)
    fputs("    Description              Formatted Data\n", stdout);
    else
    fputs("    Description        Offset:Byte Hex Data           Formatted Data\n"
          , stdout);
    fputs("------------------------------------------------------------------"
      "--------------\n",
      stdout);
}








