#include "acssys.h"
#ifndef lint
static char SccsId[] = "@(#) %full_name:	acsapi/csrc/acs_status/2.0A %";
#endif
/*
 *
 *                            (c) Copyright (1993-1994)
 *                      Storage Technology Corporation
 *                            All Rights Reserved
 *
 * Name:
 *      acs_status()
 *
 * Description:
 *      This procedure is called by acs toolkit interface functions.
 *      It returns a the name of the acsls STATUS given an acsls
 *      STATUS enum value.
 *
 * Return Values:
 *      A string describing the STATUS.
 *
 * Parameters:
 *
 *  status - STATUS enum 
 *
 * Implicit Inputs:
 *
 * Implicit Outputs:
 *
 * Considerations:
 *
 * Module Test Plan:
 *
 * Revision History:
 *    Ken Stickney         29-Apr-1994    Original
 */

/* Header Files: */
#include <stddef.h>
#include <stdio.h>

#include "acsapi.h"
#include "acssys_pvt.h"
#include "cl_pub.h"

/* Defines, Typedefs and Structure Definitions: */

#undef SELF
#define SELF	"acs_status"
#undef ACSMOD
#define ACSMOD 110

/* Global and Static Variable Declarations: */

/* Procedure Type Declarations: */

char * acs_status(STATUS status)
{
 
    acs_trace_entry ();
 
    acs_trace_exit (0);
 
    return cl_status(status);
}
