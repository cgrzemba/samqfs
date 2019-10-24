# ifndef lint
static char SccsId[] = "@(#)csi_xev_rsrc_st.c	6.1 10/16/01 ";
# endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_xevent_rsrc_status()
 *
 * Description:
 *
 *      Routine serializes/deserializes an EVENT_RESOURCE_STATUS structure 
 * 
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE
 *
 * Implicit Outputs:
 *      NONE
 *
 * Considerations:
 *      NONE
 *     
 * Module Test Plan:
 *      NONE
 *
 * Revision History:
 *	C. J. Higgins	    17-Oct-2001	    Created
 */

/*      Header Files: */
# include "csi.h"
# include "ml_pub.h"
# include "csi_xdr_xlate.h"

/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static char *st_src = __FILE__;
static char *st_module = "csi_xevent_rsrc_status()";
/*      Static Prototype Declarations: */

/*      Procedure Type Declarations: */

bool_t
csi_xevent_rsrc_status(
		       XDR *xdrsp,		/* xdr handle structure */
		       EVENT_RESOURCE_STATUS *ev_rsrc_stat/* event_res_stat struct ptr */
)
{	
	register int	part_size = 0;		/* size this part of packet */
	register int	total_size = 0;         /* sum of parts in packet */
	unsigned int	pad_size = 0;		/* sum of parts in packet */
	BOOLEAN         badsize;		/* TRUE if packet size bogus*/

# ifdef DEBUG
	if TRACE(CSI_XDR_TRACE_LEVEL)
		cl_trace(st_module, 2, (unsigned long) xdrsp, 
			 (unsigned long) ev_rsrc_stat);

# endif /* DEBUG */

	/* translate resource_type */
	part_size = (char *)& ev_rsrc_stat->resource_identifier
		    - (char *)& ev_rsrc_stat->resource_type;
	badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	if (badsize || !xdr_enum(xdrsp, (enum_t *)& ev_rsrc_stat->
				 resource_type))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "xdr_enum()", 
			 MMSG(928, "XDR message translation failure")));
		return 0;
	}
	csi_xcur_size += part_size;
	total_size += part_size;
	/* translate identifier */
	part_size = (char *)& ev_rsrc_stat->resource_event
		    - (char *)& ev_rsrc_stat->resource_identifier;
	badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	if (badsize || !csi_xidentifier(xdrsp, &ev_rsrc_stat->
					resource_identifier, 
					ev_rsrc_stat->resource_type))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE,st_module, 
		"csi_xidentifier()",MMSG(928,"XDR message translation failure")));
		return (0);
	}
	csi_xcur_size += part_size;
	total_size += part_size;
	/* translate resource_event */
	part_size = (char *)& ev_rsrc_stat->resource_data_type
		    - (char *)& ev_rsrc_stat->resource_event;
	badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	if (badsize || !xdr_enum(xdrsp, (enum_t *)& ev_rsrc_stat->
				 resource_event))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "xdr_enum()", 
			 MMSG(928, "XDR message translation failure")));
		return 0;
	}
	csi_xcur_size += part_size;
	total_size += part_size;
	/* translate resource_data_type */
	part_size = (char *)& ev_rsrc_stat->resource_data
		    - (char *)& ev_rsrc_stat->resource_data_type;
	badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	if (badsize || !xdr_enum(xdrsp, (enum_t *)& ev_rsrc_stat->
				 resource_data_type))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "xdr_enum()", 
			 MMSG(928, "XDR message translation failure")));
		return 0;
	}
	csi_xcur_size += part_size;
	total_size += part_size;
	/* translate resource_data */
	part_size = RESOURCE_ALIGN_PAD_SIZE;
	badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	if (badsize || !csi_xresource_data(xdrsp, &ev_rsrc_stat->
					   resource_data, ev_rsrc_stat->
					   resource_data_type))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
		"csi_xresource_data()",MMSG(928,"XDR message translation failure")));
		return (0);
	}
	/* csi_xcur_size was updated inside csi_xresource_data */
	/* we should be at 108 bytes right now align bytes to 128*/
	total_size += part_size;
	pad_size = EVENT_ALIGN_PAD_SIZE - total_size;
	/* translate trailing bytes */
	if (pad_size > 0)
	{
		/* Use a temporary buffer when decoding padding bytes and decode
                                                * padding bytes with maxsize EVENT_ALIGN_PAD_SIZE. This approach
                                                * has the side effect of correctly updating the xdrsp pointer.*/
		if (XDR_DECODE == xdrsp->x_op)
		{
			char		temp[EVENT_ALIGN_PAD_SIZE];/* temp buffer for xdr_bytes */
			char		*tmp = temp;

			if (!xdr_bytes(xdrsp, &tmp, &pad_size, 
				       EVENT_ALIGN_PAD_SIZE))
			{
				MLOGCSI((STATUS_TRANSLATION_FAILURE, 
				st_module, 
				"xdr_bytes()",MMSG(928,"XDR message translation failure")));
				return (0);
			}
		}
		else/* XDR_ENCODE*/
			{
				char	*cp = (char *)(ev_rsrc_stat + total_size);

				if (!xdr_bytes(xdrsp, &cp, &pad_size, 
					       pad_size))
				{
					MLOGCSI((
						 STATUS_TRANSLATION_FAILURE, 
						 st_module, 
						 "xdr_bytes()", MMSG(
						 928, 
						 "XDR message translation failure")));
					return (0);
				}
			}
	}
	/* update csi_xcursize*/
	csi_xcur_size += pad_size;
	return (1);
}

