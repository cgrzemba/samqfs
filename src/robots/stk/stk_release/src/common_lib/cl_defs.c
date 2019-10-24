static char SccsId[] = "@(#)cl_defs.c	5.4 12/13/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_defs
 *
 * Description:
 *
 *      This module contains global data declarations for IPC and trace
 *      facilities.
 *
 * Return Values:       NONE
 *
 * Implicit Input:      NONE
 *
 * Implicit Outputs:    NONE
 *
 * Considerations:      NONE
 *
 * Module Test Plan:    NONE
 *
 * Revision History:
 *
 *      D. F. Reed              29-Sep-1988     Original.
 *
 *      D. A. Beidle            23-Jul-1991     Removed acs_count, port_count.
 *          Added cl_trace_enter, cl_trace_volume.
 *
 *      D. A. Beidle            24-Nov-1991.    IBR#151 - Initialize sd_in
 *          to -1 to indicate the process' socket is not yet open.  This is 
 *          consistent with file descriptors (>= 0) and system functions.
 *      A. W. Steere            22-Jul-1993     R5.0 added mixed media globals.
 *          Removed cl_trace_enter, cl_trace_volume.
 *      D. A. Beidle            17-Dec-1993     Removed acsss_version[] since
 *          replaced by cl_get_version function.  acsss_version[] is no longer
 *          referenced (actually never was).
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"

#include <stdio.h>

#include "defs.h"
#include "cl_mm_pub.h"
#include "cl_mm_pri.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */

int             sd_in = -1;             /* module input socket descriptor   */
                                        /*   < 0 if socket closed           */
                                        /*  >= 0 if open socket descriptor  */

int             n_fds;                  /* number of input descriptors */
int             fd_list[FD_SETSIZE];    /* input descriptor list */
char            my_sock_name[SOCKET_NAME_SIZE];
                                        /* module input socket name */
TYPE            my_module_type;         /* executing module's type */
TYPE            requestor_type;         /* request originator's module type */
int             restart_count;          /* process failed/restart count */
MESSAGE_ID      request_id = 0;         /* associated request ID, or 0 if */
                                        /* not associated with a request */
STATE           process_state = STATE_RUN;
                                        /* executing process' state flag */
unsigned long   trace_module;           /* module trace define value */
unsigned long   trace_value;            /* trace flag value */


/*
 * ------------------------- Mixed Media -----------------------------------
 *
 *      Private Data to be used only by mixed media common library routines.
 */

/*
 * Define global pointers to two arrays.  The arrays contain all the
 * mixed_media information.
 */
MEDIA_TYPE_INFO *Mm_media_info_ptr = (MEDIA_TYPE_INFO *)NULL;
DRIVE_TYPE_INFO *Mm_drive_info_ptr = (DRIVE_TYPE_INFO *)NULL;

/*
 * Define the array boundaries for MEDIA_TYPE_INFO and DRIVE_TYPE_INFO.
 * Use variables, so at a later time a realloc scheme similar to cl_pnl.c
 * can be used.  All the for loops will not have to be changed.
 */
int Mm_max_media_types = MM_MAX_MEDIA_TYPES;
int Mm_max_drive_types = MM_MAX_DRIVE_TYPES;

/* Cleaning cartridge capability list and string equivilents.  It is here
 * to allow shared libraries.
 */ 
MM_CLN_CAPAB mm_cln_capab[] = {
        CLN_CART_FIRST,         "First",
        CLN_CART_NEVER,         "No",
        CLN_CART_INDETERMINATE, "Maybe",
        CLN_CART_ALWAYS,        "Yes",
        CLN_CART_LAST,          "Last %d",
};

 
/*------------------------ end of Mixed Media -------------------------------*/

/*
 *      Procedure Type Declarations:
 */

