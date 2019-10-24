#ifndef lint
static char SccsId[] = "@(#)csi_xptp_id.c 1.0 11/29/04 ";
#endif
/*
 * Copyright (2003, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_xptp_id()
 *
 * Description:
 *      Routine serializes/deserializes a PTPID structure.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:  NONE
 *
 * Implicit Outputs:  NONE
 *
 * Considerations: NONE
 *
 * Module Test Plan: NONE
 *
 * History:
 *     Wipro(Prasad)   15-Jun-2003   New file created for PTPID. 
 *     Mitch Black     30-Nov-2004   Replaced P4_Id with SccsId,
 *                                   and cleaned up preamble.
 */


/*
 *      Header Files:
 */
#include "csi.h"
#include "ml_pub.h"
 
 
 
/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char     *st_src = __FILE__;
static char     *st_module = "csi_xptp_id()";
 
 
/*
 *      Procedure Type Declarations:
 */



bool_t 
csi_xptp_id (
    XDR *xdrsp,                         /* xdr handle structure */
    PTPID *ptpidp                        /* ptpid */
)
{


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module,                     /* routine name */
                 2,                             /* parameter count */
                 (unsigned long) xdrsp,         /* argument list */
                 (unsigned long) ptpidp);       /* argument list */
#endif /* DEBUG */

      if(!csi_xacs(xdrsp, &ptpidp->acs)) 
      {
           MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xacs()",
        MMSG(928, "XDR message translation failure")));
           return(0);
      }

      if(!csi_xlsm(xdrsp, &ptpidp->master_lsm))
      {
          MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xlsm()",
        MMSG(928, "XDR message translation failure")));
           return(0);
      }

      if(!csi_xpnl(xdrsp, &ptpidp->master_panel))
      {
           MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xpnl()",
        MMSG(928, "XDR message translation failure")));
           return(0);
      }

      if(!csi_xlsm(xdrsp, &ptpidp->slave_lsm))
      {
           MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xlsm()",
        MMSG(928, "XDR message translation failure")));
           return(0);
      }

      if(!csi_xpnl(xdrsp,  &ptpidp->slave_panel))
      {
         MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xpnl()",
        MMSG(928, "XDR message translation failure")));
           return(0);
      }


    return(1);

}
