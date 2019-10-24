/* SccsId @(#)acssys_pvt.h	2.2 10/21/01 (c) 1992-2001 STK */
#ifndef _ACSSYS_PVT_
#define _ACSSYS_PVT_ 1

/*******************************************************************************
** Copyright (C) 1993,2010, Oracle and/or its affiliates. All rights reserved.
**
**   Name:     acssys.pvt
**
**   Purpose: System dependent defines and function prototypes.
**
**
**---------------------------------------------------------------------
**   Maintenance History:
**
**   07/09/93: KJS    Created from HSC land (Tom Rethard)
**   08/29/93: KJS    Portablized for BOS/X and MVS with TDR.
**   01/20/94: KJS    Removed defines for acs_type() & 
**                    acs_get_packet_version().
**   05/06/94: KJS    Removed alphanumerics aliases from all but AS400
**                    platforms.
**   05/31/94: KJS    Fixed acs_error_msg macro to work with ANSI 
**                    standard args (stdarg.h).
**   10/15/01: SLS    Changed copyright dates and added acs_register_int_resp.
**
**   05/17/2010 MPW   Changed extern acs_caller to extern int acs_caller to
**                    remove a compiler warning message.  Changed copyright to
**                    an Oracle copyright statement.
**
***********************************************************************
*/

/* Defines */
#ifdef ACS400
#define acs_verify_ssi_running    ACS100
#define acs_build_header          ACS101
#define acs_error                 ACS102
#define acs_ipc_read              ACS103
#define acs_ipc_write             ACS104
#define acs_build_ipc_header      ACS105
#define acs_get_sockname          ACS106

#define acs_vary_response         ACS200
#define acs_query_response        ACS201
#define acs_audit_fin_response    ACS202
#define acs_audit_int_response    ACS203
#define acs_select_input          ACS205
#define acs_register_int_response ACS206
#endif

#ifndef ANY_PORT
#define ANY_PORT "0"
#endif

#define acs_error_msg(args)    do {     \
          acs_caller = ACSMOD; \
          acs_error args; } while (0)

#ifdef DEBUG
#define acs_trace_entry() \
          printf("\n\nentering %s", SELF);
#else
#define acs_trace_entry() 
#endif

#ifdef DEBUG
#define acs_trace_exit(a) \
          printf("\nexiting %s returncode = %d", SELF, a);
#else
#define acs_trace_exit(a) 
#endif

#ifdef DEBUG
#define acs_trace_point(a) \
          printf("\ntracept in %s: %s", #a );
#else
#define acs_trace_point(a) 
#endif

#define COPYRIGHT\
   static const char* CR = SELF\
          " " __FILE__\
          " " __DATE__\
          "/" __TIME__\
          " copyright (c) Storage Technology Corp. 1992, 1993-2001"

/* Global and Static Declarations */

#ifndef ACS_ERROR_C
extern int acs_caller;
#endif

/* Function prototypes */
void acs_error(ACSMESSAGES * msgNo, ...);


/* 
 * Below is the definition of the acs_error function for AS400, 
 * commented out.
 */
/*
void acs_error( int caller, ACSMESSAGES msgNo, ... );
*/   

#endif
