static char SccsId[] = "@(#)csi_globals.c   5.7 12/2/93 ";
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Source/System:
 *
 *      csi_globals.c   - source file of the CSI module of the ACSSS
 *
 * Name:
 *
 *      csi_globals
 *
 * Description:
 *
 *      This function is used only for storage of global variables.
 *      All CSI specific global variables are kept in this file, with the
 *      exception of those defined and used by the common library (CL,cl)
 *      functions.
 *
 * Return Values:
 *
 *      NONE
 *
 * Implicit Inputs:
 *
 *      Storage for global variables is here.
 *
 * Implicit Outputs:
 *
 *      Storage for global variables is here.
 *
 * Considerations:
 *
 *      Consideration as always per usage of global variables.
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      J. A. Wishner       03-Jan-1989     Created.
 *      J. A. Wishner       30-May-1989     csi_netbuf becomes pointer, and
 *                                          csi_netbuf_data array goes away.
 *      E. A. Alongi        28-Jul-1992     added csi_minimum_version and
 *                                          csi_allow_acspd.
 *                                          added csi_trace_flag (R3.0.1)
 *      Emanuel A. Alongi   27-Jul-1993     Deleted global env vars. Access
 *                                          these vars locally and dynamically.
 *      David Farmer        17-Aug-1993     added csi_broke_pipe
 *      Emanuel A. Alongi   26-Oct-1993.    Eliminated csi_ssi_alt_procno_pd,
 *                                          the alternate procedure number for
 *                                          the ACSPD.
 *      Mitch Black         01-Dec-2004     Moved csi_ssi_alt_procno_lm outside of
 *                                          "ifdef SSI" so that ifdef SSI could be
 *                                          removed from csi.h, which caused 
 *                                          rebuild problems.
 *      Joseph Nofi         15-May-2011     XAPI support;
 *                                          Added csi_xapi_conversion_flag and
 *                                          csi_xapi_conversion_table global variables.
 *                                           
 */                                          


/*
 *      Header Files:
 */
#include "csi.h"


/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */

int         csi_rpc_tcpsock = RPC_ANYSOCK;      /* rpc tcp service socket */
int         csi_rpc_udpsock = RPC_ANYSOCK;      /* rpc udp service socket */
SVCXPRT    *csi_udpxprt = NULL;                 /* CSI UDP transport handle */
SVCXPRT    *csi_tcpxprt = NULL;                 /* CSI TCP transport handle */
QM_QID      csi_lm_qid = 0;                     /* ID of CSI connection queue */
QM_QID      csi_ni_out_qid = 0;                 /* ID of CSI output queue */
CSI_MSGBUF *csi_netbufp;                        /* network packet buffer */
char        csi_hostname[CSI_HOSTNAMESIZE];     /* name of this host */
int         csi_pid;                            /* process id of this process */
int         csi_xexp_size = 0;                  /* expected translation bytes */
int         csi_xcur_size = 0;                  /* current translation bytes */
int         csi_co_process_pid;                 /* ADI co-process pid        */
int         csi_adi_ref;                        /* ADI connection reference  */
VERSION     csi_active_xdr_version_branch;      /* version being translated */
int         csi_trace_flag = FALSE;             /* flag for packet tracing */
int         csi_broke_pipe;         /* broke pipe flag */

#ifndef ADI /* rpc declaration */
unsigned char csi_netaddr[CSI_NETADDR_SIZE];    /* address of this host */

#else /* adi declaration */
unsigned char csi_client_name[CSI_ADI_NAME_SIZE];/* OSLAN name of this host */

#endif /* ~ADI */


#ifdef SSI
CSI_HEADER   csi_ssi_rpc_addr;                  /* rpc address of this ssi */
CSI_HEADER   csi_ssi_adi_addr;                  /* adi address of this ssi */
#endif /* SSI */
 
long csi_ssi_alt_procno_lm = CSI_HAVENT_GOTTEN_ENVIRONMENT_YET;
                                        /* alt. ssi callback ACSLM proc# */

#if defined(XAPI) && defined(SSI)
/*********************************************************************/
/* Variables to control ACSAPI-to-XAPI conversion.                   */
/*********************************************************************/
int           csi_xapi_conversion_flag = FALSE;/* Conversion flag    */
void         *csi_xapi_control_table   = NULL; /* XAPI control table */
#endif /* XAPI and SSI */


