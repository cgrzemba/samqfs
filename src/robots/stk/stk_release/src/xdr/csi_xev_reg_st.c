# ifndef lint
static char SccsId[] = "@(#)csi_xev_reg_st.c	6.1 10/16/01 ";
# endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_xevent_reg_status()
 *
 * Description:
 *
 *      Routine serializes/deserializes an EVENT_REGISTER_STATUS structure 
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
 *     A pointer to response is passed so that we can recalc size in case of
 *     compiler padding after last element of the array
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
static char *st_module = "csi_xevent_reg_status()";
/*      Static Prototype Declarations: */

/*      Procedure Type Declarations: */

bool_t
csi_xevent_reg_status(
		      XDR *xdrsp,		/* xdr handle structure */
		      EVENT_REGISTER_STATUS *ev_reg_stat_ptr, /* event_reg_status struct ptr*/
		      int *total
)
{	
	register int	i;			/* loop counter */
	register int	count;			/* holds count value */
	register int	part_size = 0;		/* size this part of packet */
	BOOLEAN         badsize;		/* TRUE if packet size bogus*/

# ifdef DEBUG
	if TRACE(CSI_XDR_TRACE_LEVEL)
		cl_trace(st_module, 3, (unsigned long) xdrsp, 
			 (unsigned long) ev_reg_stat_ptr, (unsigned long)
			 total);

# endif /* DEBUG */

	/* translate registration id*/
	part_size = (char *)& ev_reg_stat_ptr->count
		    - (char *)& ev_reg_stat_ptr->registration_id;
	badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	if (badsize || !csi_xregistration_id(xdrsp, &ev_reg_stat_ptr->
					     registration_id))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "csi_xregistration_id()", 
			 MMSG(928, "XDR message translation failure")));
		return 0;
	}
	csi_xcur_size += part_size;
	*total = part_size;
	/* translate count */
	part_size = (char *)& ev_reg_stat_ptr->register_status[0]
		    - (char *)& ev_reg_stat_ptr->count;
	badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	if (badsize || !xdr_u_short(xdrsp, &ev_reg_stat_ptr->count))
	{
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "xdr_u_short()", 
			 MMSG(928, "XDR message translation failure")));
		return (0);
	}
	csi_xcur_size += part_size;
	*total += part_size;
	/* check boundary condition before loop */
	if (ev_reg_stat_ptr->count > MAX_ID)
	{
		MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, 
			 CSI_NO_CALLEE, 
			 MMSG(928, "XDR message translation failure")));
		return 0;
	}
	/* loop on register_status translating */
	for (i = 0, count = ev_reg_stat_ptr->count; i < count; i++)
	{
		/* translate starting at register_status */
		part_size = (char *)& ev_reg_stat_ptr->register_status[i + 1]
			    - (char *)& ev_reg_stat_ptr->register_status[i];
		badsize = CHECKSIZE(csi_xcur_size, part_size, 
				    csi_xexp_size);
		if (badsize || 
		    !csi_xregister_status(xdrsp, &ev_reg_stat_ptr->
					  register_status[i]))
		{
			MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
				 "csi_xregister_status()", MMSG(928, 
				 "XDR message translation failure")));
			return (0);
		}
		csi_xcur_size += part_size;
		*total += part_size;
	}
	return (1);
}

