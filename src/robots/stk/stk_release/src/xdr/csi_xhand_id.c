# ifndef lint
static char SccsId[] = "@(#)csi_xhand_id.c	6.1 10/18/01 ";
# endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xhand_id()
 *
 * Description:
 *
 *      Routine serializes/deserializes a HANDID structure.
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
 *      C. J. Higgins       18-Oct-2001     Created.
 *	Mitch Black         30-Nov-2004     Replaced panel with panel_id
 *                            in hand_id_ptr.
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
static char *st_module = "csi_xhand_id()";
/*
 *      Procedure Type Declarations:
 */

bool_t
csi_xhand_id(
	     XDR *xdrsp,			/* xdr handle structure */
	     HANDID *hand_id_ptr		/* hand id structure */
)
{	
# ifdef DEBUG
	if TRACE(CSI_XDR_TRACE_LEVEL)
		cl_trace(st_module,		/* routine name */
			 2,			/* parameter count */
			 (unsigned long) xdrsp, /* argument list */
			 (unsigned long) hand_id_ptr);

	/* argument list */
# endif /* DEBUG */

	/* translate the panel id */
	if (!csi_xpnl_id(xdrsp, &hand_id_ptr->panel_id))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "csi_xpnl_id()", 
			 MMSG(928, "XDR message translation failure")));
		return (0);
	}
	/* translate the hand */
	if (!xdr_char(xdrsp, &hand_id_ptr->hand))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "xdr_char()", 
			 MMSG(928, "XDR message translation failure")));
		return (0);
	}
	return (1);
}

