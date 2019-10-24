# ifndef lint
static char SccsId[] = "@(#)csi_xregister_st.c	6.1 10/16/01 ";
# endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xregister_status()
 *
 * Description:
 *
 *      Routine serializes/deserializes a REGISTER_STATUS struct.
 *
 * Return Values:
 *
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *
 *      NONE
 *
 * Implicit Outputs:
 *
 *      NONE
 *
 * Considerations:
 *
 *      NONE
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History
 *      C. J. Higgins       16-Oct-2001     Created.
 */

/*
 *      Header Files:
 */
# include "csi.h"
# include "ml_pub.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char *st_src = __FILE__;
static char *st_module = "csi_xregister_status()";
/*
 *      Procedure Type Declarations:
 */
bool_t
csi_xregister_status(
		     XDR *xdrsp,		/* xdr handle structure */
		     REGISTER_STATUS *reg_stat	/* register status */
)
{	
# ifdef DEBUG
	if TRACE(CSI_XDR_TRACE_LEVEL)
		cl_trace(st_module,		/* routine name */
			 2,			/* parameter count */
			 (unsigned long) xdrsp, /* argument list */
			 (unsigned long) reg_stat);
						/* argument list */
# endif /* DEBUG */

	/* translate the event_class member of register status */
	if (!xdr_enum(xdrsp, (enum_t *)& reg_stat->event_class))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "xdr_enum()", 
			 MMSG(928, "XDR message translation failure")));
		return (0);
	}
	/* translate the register_return member of register status */
	if (!xdr_enum(xdrsp, (enum_t *)& reg_stat->register_return))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "xdr_enum()", 
			 MMSG(928, "XDR message translation failure")));
		return (0);
	}
	return (1);
}

