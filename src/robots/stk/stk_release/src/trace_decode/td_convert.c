#ifndef lint
static char SccsId[] = "@(#)td_convert.c        2.2 11/10/01 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_convert
 *
 * Description:
 *      Functions to convert data to another format.
 *      ex. numeric data ( unsigned, unsigned long...) to ascii format
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
 *
 *      M. H. Shum          10-Sep-1993     Original.
 *      S. L. Siao          31-Oct-2001     Added td_htoa.
 *      Mike Williams       02-Jun-2010     Included ctype.h to remedy warning.
 *      Joseph Nofi         15-Aug-2011     XAPI support;
 *                                          Added td_ustodev function to 
 *                                          convert CCUU format device addresses.
 *
 */


#include <ctype.h>
#include "td.h"


static char id_buf[SIZEOFIDBUF];

/*
 * Decode an internet address.
 */
char* td_net_addr(unsigned char* addr)
{
    sprintf(id_buf, "%u.%u.%u.%u.%u.%u", 
            *addr, *(addr+1), *(addr+2), *(addr+3), *(addr+4), *(addr+5));

    return id_buf;
}


/*
 * Decode an unsigned integer
 */
char* td_utoa(unsigned int id)
{
    sprintf(id_buf, "%u", 
            id);

    return id_buf;   
}


/*
 * Decode a long integer
 */
char* td_ltoa(long id)
{
    sprintf(id_buf, "%ld", 
            id);

    return id_buf;   
}


/*
 * Decode an unsigned long integer
 */
char* td_ultoa(unsigned long id)
{
    sprintf(id_buf, "%lu", 
            id);

    return id_buf;   
}


/*
 * Decode a short integer
 */
char* td_stoa(short id)
{
    sprintf(id_buf, "%d", 
            id);

    return id_buf;   
}


/*
 * Decode an unsigned short integer
 */
char *td_ustoa(unsigned short id)
{
    sprintf(id_buf, "%u", id);

    return id_buf;   
}


/*
 * Decode an SIGNED char
 */
char* td_ctoa(SIGNED char id)
{
    sprintf(id_buf, "%d", id);

    return id_buf;   
}


/*
 * return hex value 
 */
char* td_htoa(SIGNED char id)
{
    sprintf(id_buf, "%X", id);

    return id_buf;   
}


/*
 * Remove all the spaces in a string.
 */
char* td_rm_space(char* str)
{
    char *cur;          /* pointer to the string */
    char *next;         /* pointer to next non-space character */

    cur = next = str;

    while (*next)
    {
        while (isspace(*next))
        {
            next++;
        }

        if (*next == '\0')
        {
            break;
        }

        if (isspace(*cur))
        {
            *cur = *next;
            *next = ' ';
        }

        cur++;
        next++;
    }
    *cur = '\0';

    return str;
}    


/*
 * Decode a device address
 */
char* td_ustodev(unsigned short id)
{
    sprintf(id_buf, "%.04X", 
           id);

    return id_buf;   
}





