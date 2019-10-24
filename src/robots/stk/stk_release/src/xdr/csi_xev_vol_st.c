# ifndef lint
static char SccsId[] = "@(#)csi_xev_vol_st.c	6.1 10/16/01 ";
# endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_xevent_vol_status()
 *
 * Description:
 *
 *      Routine serializes/deserializes an EVENT_VOLUME_STATUS structure 
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
 *	C. J. Higgins	    16-Oct-2001	    Created
 */

/*      Header Files: */
# include "csi.h"
# include "ml_pub.h"
# include "csi_xdr_xlate.h"

/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static char *st_src = __FILE__;
static char *st_module = "csi_xevent_vol_status()";
/*      Static Prototype Declarations: */

/*      Procedure Type Declarations: */

bool_t
csi_xevent_vol_status(
		      XDR *xdrsp,		/* xdr handle structure */
		      EVENT_VOLUME_STATUS *ev_vol_stat_ptr/* event_vol_status structure pointer */
)
{	
	register int	part_size = 0;		/* size this part of packet */
	register int	total_size = 0;         /* sum of parts in packet */
	unsigned int	pad_size = 0;		/* size of padding at end of union */
	BOOLEAN         badsize;		/* TRUE if packet size bogus */
	char		*cp;
	char		temp[EVENT_ALIGN_PAD_SIZE];/* temp buffer for xdr_bytes */
	char		*tmp;

# ifdef DEBUG
	if TRACE(CSI_XDR_TRACE_LEVEL)
		cl_trace(st_module, 2, (unsigned long) xdrsp, (unsigned
			 long) ev_vol_stat_ptr);

# endif /* DEBUG */

	/* translate event_type */
	part_size = (char *)& ev_vol_stat_ptr->vol_id
		    - (char *)& ev_vol_stat_ptr->event_type;
	badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	if (badsize || !xdr_enum(xdrsp, (enum_t *)& ev_vol_stat_ptr->
				 event_type))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "xdr_enum()", 
			 MMSG(928, "XDR message translation failure")));
		return 0;
	}
	csi_xcur_size += part_size;
	total_size += part_size;
	/* translate volid */
	part_size = sizeof(VOLID);
	badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	if (badsize || !csi_xvol_id(xdrsp, &ev_vol_stat_ptr->vol_id))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "csi_xvol_id()", 
			 MMSG(928, "XDR message translation failure")));
		return (0);
	}
	csi_xcur_size += part_size;
	total_size += part_size;
	pad_size = EVENT_ALIGN_PAD_SIZE - total_size;
	/* translate trailing bytes */
	if (pad_size > 0)
	{
		/* Use a temporary buffer when decoding padding bytes and decode
                        * padding bytes with maxsize RESOURCE_ALIGN_PAD_SIZE. This approach
                        * has the side effect of correctly updating the xdrsp pointer.*/
		if (XDR_DECODE == xdrsp->x_op)
		{
			tmp = temp;		/* xdr_bytes expects as arg */

			if (!xdr_bytes(xdrsp, &tmp, &pad_size, 
				       EVENT_ALIGN_PAD_SIZE))
			{
				MLOGCSI((STATUS_TRANSLATION_FAILURE, 
					 st_module, "xdr_bytes()", 
					 MMSG(928, 
					 "XDR message translation failure")));
				return (0);
			}
		}
		else/* XDR_ENCODE*/
		{
			cp = (char *)(ev_vol_stat_ptr + total_size);
			if (!xdr_bytes(xdrsp,&cp,&pad_size,pad_size))
			{
				MLOGCSI((STATUS_TRANSLATION_FAILURE, 
					st_module, 
					"xdr_bytes()", 
					MMSG(928, 
					"XDR message translation failure")));
					return (0);
			}
		}
	}
	/* update csi_xcursize*/
	csi_xcur_size += pad_size;
	return (1);
}

