# ifndef lint
static char SccsId[] = "@(#)csi_xrgstrn_id.c	6.1 10/15/01 ";
# endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xregistration_id()
 *
 * Description:
 *
 *      Routine serializes/deserializes a REGISTRATION_ID structure.
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
 *	C. J. Higgins	    15-Oct-2001	    Created.
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
static char *st_module = "csi_xregistration_id()";
/*
 *      Procedure Type Declarations:
 */

bool_t
csi_xregistration_id(
		     XDR *xdrsp,		/* xdr handle structure */
		     REGISTRATION_ID *reg_id_ptr/* registration id pointer*/
)
{	
	char		*cp;			/* used on call to xdr_bytes since need a ptr-to-ptr */
	unsigned int	size;

						/* size of the identifier to be decoded */

# ifdef DEBUG
	if TRACE(CSI_XDR_TRACE_LEVEL)
		cl_trace(st_module,		/* routine name */
			 2,			/* parameter count */
			 (unsigned long) xdrsp, /* argument list */
			 (unsigned long) reg_id_ptr);
						/* argument list */
# endif /* DEBUG */

	/* translate reg_id */
	cp = reg_id_ptr->registration;
	size = sizeof(reg_id_ptr->registration);
	if (!xdr_bytes(xdrsp, &cp, &size, size))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "xdr_bytes()", 
			 MMSG(928, "XDR message translation failure")));
		return (0);
	}
	return (1);
}

