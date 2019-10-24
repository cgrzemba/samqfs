#ifndef lint
static char SccsId[] = "@(#)td_hex_dump.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_hex_dump
 *
 * Description:
 *      Dump the raw hex data to stdout.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_acsii_buf.
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
 *
 */

#include <stdio.h>
#include "td.h"

#define BYTEPERCOL   8		/* number of bytes per column */
#define BYTEPERLINE  16		/* number of bytes per line */

void
td_hex_dump()
{
    char *buf_ptr;		/* pointer to td_ascii_buf */
    int  count = 0;			/* counter */

    buf_ptr = td_ascii_buf;

    fputs("-------------------------------------------------------------------"
	  "-------------\nMessage contents:\n", stdout);

    while (*buf_ptr) {
	if (count % BYTEPERLINE == 0) /* go to the next line, print the count */
	    printf("\n%04d:  ", count);
	else if (count % BYTEPERCOL == 0) /* seperate column by two spaces */
	    printf("  ");
	/* print two bytes */
	fputc(*buf_ptr++, stdout);
	fputc(*buf_ptr++, stdout);
	fputc(' ', stdout);
	count++;
    }
}




