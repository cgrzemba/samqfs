static char SccsId[] = "@(#)cl_str_to_buf.c	5.1 11/3/93 ";
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Description:
 *
 *   This file contains a function to convert a string with quoted
 *   characters into a string that could be used by printf.
 *
 * Revision History:
 *
 *   Alec Sharp       29-Oct-1993  Original.
 *
 */

/* ----------- Header Files -------------------------------------------- */

#include "flags.h"
#include "system.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>

/* ----------- Defines, Typedefs and Structure Definitions ------------- */

/* ----------- Global and Static Variable Declarations ----------------- */

/* ----------  Procedure Declarations ---------------------------------- */


    

/*--------------------------------------------------------------------------
 *
 * Name: cl_str_to_buf
 *
 * Description:
 *
 *   This file contains a function to convert a string with quoted
 *   characters into a string that could be used by printf.
 *
 *   For example, it will take a string such as:
 *
 *      \"now \t is the \n time for \063 good men\" 
 *
 *   into:
 *
 *      "now	is the
 *       time for 3 good men"
 *
 *   It handles the following escape characters:
 *	\b	backspace
 * 	\f	form feed
 *	\n	newline
 *	\r	return
 *	\t	tab
 *	\v	vertical tab
 *	\\	backslash
 *	\'	single quote
 *	\"	double quote
 *	\?	trigraph stuff
 *	\###   I.e. an octal number such as \063
 *	
 *   It doesn't handle
 *	\a (alert)
 *      \0
 *      \x### (hex values)
 *
 * Return Values:	A pointer to the \0 terminating the string.
 *
 * Parameters:		cp_in	Ptr to input string
 *			cp_out  Ptr to converted output buffer
 *
 * Implicit Inputs:	NONE
 * Implicit Outputs:	NONE
 */

char *
cl_str_to_buf (const char *cp_in, char *cp_out)
{
    char ca_octal[5];
    char *cp;
    long l_num;
    
    while (*cp_in != '\0') {
	
	if (*cp_in != '\\') {
	    /* Not a backslash, so just copy the character */ 
	    *cp_out++ = *cp_in++;
	}
	else {
	    cp_in++;
	    switch (*cp_in) {
		/* Characters */
	      case 'b':		/* Backspace */
		*cp_out = '\b';
		break;
	      case 'f':		/* Form feed */
		*cp_out = '\f';
		break;
	      case 'n':		/* Newline */
		*cp_out = '\n';
		break;
	      case 'r':		/* Carriage return */
		*cp_out = '\r';
		break;
	      case 't':		/* Tab */
		*cp_out = '\t';
		break;
	      case 'v':		/* Vertical tab */
		*cp_out = '\v';
		break;
	      case '\'':	/* Single quote */
		*cp_out = '\'';
		break;
	      case '"':		/* Double quote */
		*cp_out = '\"';
		break;
	      case '\\':	/* Backslash */
		*cp_out = '\\';
		break;
	      case '\?':	/* Question mark */
		*cp_out = '\?';
		break;
	      default:
		/* See if it's an octal number */
		strncpy (ca_octal, cp_in, 3);
		ca_octal[3] = '\0';
		errno = 0;
		l_num = strtol (ca_octal, &cp, 8);
		if (cp != ca_octal && errno == 0) {
		    /* Good octal number. Copy the number and increment
		       cp_in by 2. The end of the loop increments over
		       the last character */
		    *cp_out = l_num;
		    cp_in += 2;
		}
		else {
		    /* Copy the \ and the current character */
		    *cp_out++ = '\\';
		    *cp_out = *cp_in;
		}
		break;
	    }
	    cp_in++;
	    cp_out++;
	}
    }

    *cp_out = '\0';
    return (cp_out);
}
