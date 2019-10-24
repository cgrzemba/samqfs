# ifndef lint
static char SccsId[] = "@(#)csi_xsense_scsi.c	6.1 10/16/01 ";
# endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xsense_scsiatus()
 *
 * Description:
 *
 *      Routine serializes/deserializes a SENSE_SCSI struct.
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
static char *st_module = "csi_xsense_scsi()";
/*
 *      Procedure Type Declarations:
 */
bool_t
csi_xsense_scsi(
		XDR *xdrsp,			/* xdr handle structure */
		SENSE_SCSI *sense_scsi		/* sense scsi ptr */
)
{	
# ifdef DEBUG
	if TRACE(CSI_XDR_TRACE_LEVEL)
		cl_trace(st_module,		/* routine name */
			 2,			/* parameter count */
			 (unsigned long) xdrsp, /* argument list */
			 (unsigned long) sense_scsi);
						/* argument list */
# endif /* DEBUG */

	/* translate the sense_key member of  sense scsi */
	if (!xdr_u_char(xdrsp, &sense_scsi->sense_key))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "xdr_u_char()", 
			 MMSG(928, "XDR message translation failure")));
		return (0);
	}
	/* translate the asc member of  sense scsi */
	if (!xdr_u_char(xdrsp, &sense_scsi->asc))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "xdr_u_char()", 
			 MMSG(928, "XDR message translation failure")));
		return (0);
	}
	/* translate the ascq member of sense scsi */
	if (!xdr_u_char(xdrsp, &sense_scsi->ascq))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "xdr_u_char()", 
			 MMSG(928, "XDR message translation failure")));
		return (0);
	}
	return (1);
}

