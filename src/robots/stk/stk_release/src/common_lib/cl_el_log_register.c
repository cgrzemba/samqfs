static char SccsId[] = "@(#)cl_el_log_register.c	5.3 11/29/93 ";
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Description:
 *
 *   Contains software to register logging functions.
 *
 * Revision History:
 *
 *   Alec Sharp       24-Sep-1993  Original.
 *   Alec Sharp	      04-Nov-1993  Register with ml_output function.
 *   David Farmer     24-Nov-1993  removed call to cl_log_event_register
 */

/* ----------- Header Files -------------------------------------------- */

#include "flags.h"
#include "system.h"

#include "cl_pub.h"
#include "ml_pub.h"

/* ----------- Defines, Typedefs and Structure Definitions ------------- */

/* ----------- Global and Static Variable Declarations ----------------- */

/* ----------  Procedure Declarations ---------------------------------- */


    

/*--------------------------------------------------------------------------
 *
 * Name: cl_el_log_register
 *
 * Description:
 *
 *   This function registers specific logging functions for sending
 *   event log and trace log messages to the acsel process.
 *
 * Return Values:	NONE
 *
 * Parameters:		NONE
 *
 * Implicit Inputs:	NONE
 *
 * Implicit Outputs:	NONE
 *
 */


void cl_el_log_register (void)
{
    ml_output_register (cl_el_log_event);
    cl_log_trace_register (cl_el_log_trace);
    cl_trace_register (cl_el_trace);
}
