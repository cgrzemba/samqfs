# ifndef lint
static char SccsId[] = "@(#)csi_xserial_num.c	6.1 10/15/01 ";
# endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xserial_num()
 *
 * Description:
 *
 *      Routine serializes/deserializes a SERIAL_NUM structure.
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
 *	C. J. Higgins	    18-Oct-2001	    Created.
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
static char *st_module = "csi_xserial_num()";
/*
 *      Procedure Type Declarations:
 */

bool_t
csi_xserial_num(
		XDR *xdrsp,			/* xdr handle structure */
		SERIAL_NUM *ser_num_ptr         /* serial_num pointer*/
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
			 (unsigned long) ser_num_ptr);
						/* argument list */
# endif /* DEBUG */

	/* translate ser_num */
	cp = ser_num_ptr->serial_nbr;
	size = sizeof(ser_num_ptr->serial_nbr);
	if (!xdr_bytes(xdrsp, &cp, &size, size))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "xdr_bytes()", 
			 MMSG(928, "XDR message translation failure")));
		return (0);
	}
	return (1);
}

