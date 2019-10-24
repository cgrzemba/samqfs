# ifndef lint
static char SccsId[] = "@(#)csi_xresrc_dta.c	6.1 10/17/01 ";
# endif
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_xresource_data()
 *
 * Description:
 *      Routine serializes/deserializes a RESOURCE_DATA structure.
 *      Note:  The resource_data structure is a true union, and the
 *      application is expecting to receive the number of bytes for the
 *      size of the full union, regardless of the size of a particular
 *      substructure of the union.
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
 *
 *      Must always decode enough bytes for entire union, regardless of the
 *      size of the individual resource data.  Use the "resource_align_size" member
 *      of resource_data as the designator for the total number of bytes needing
 *      translation.
 *
  *
 * Module Test Plan:
 *
 * Revision History:
 *      C. J. Higgins       17-Oct-2001.    Created.
 *      S. L. Siao          20-Mar-2002.    Added type cast to arguments of xdr
 *                                          functions.
 *      Wipro (Subhash)     28-May-2004.    Modified to translate the
 *                                          mount/dismount event details.
 */

/*      Header Files: */
# include <string.h>
# include "csi.h"
# include "ml_pub.h"
# include "cl_pub.h"
# include "csi_xdr_pri.h"

/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static char *st_module = "csi_xresource_data()";
/*      Procedure Type Declarations: */

bool_t
csi_xresource_data(
		   XDR *xdrsp,			/* xdr handle */
		   RESOURCE_DATA *res_dta,	/* resource_data union handle*/
		   RESOURCE_DATA_TYPE dta_type	/* type of resource data*/
)
{	
	register int	part_size = 0;		/* size this part of packet */
	unsigned int	pad_size = 0;		/* size this part of packet */
	char		*cp;			/* string ptr for xdr_bytes*/

# ifdef DEBUG
	if TRACE(CSI_XDR_TRACE_LEVEL)
		cl_trace(st_module, 3, (unsigned long) xdrsp, 
			 (unsigned long) res_dta, (RESOURCE_DATA_TYPE)
			 dta_type);

# endif /* DEBUG */

	/* handle each part of the union as a distinct structure */
	switch (dta_type)
	{
	  case SENSE_TYPE_HLI:
		if (!csi_xsense_hli(xdrsp, &res_dta->sense_hli))
		{
			MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
				 "csi_xsense_hli()", 
				 MMSG(928, 
				      "XDR message translation failure")));
			return (0);
		}
		part_size = sizeof(res_dta->sense_hli);
		csi_xcur_size += part_size;
		break;
	  case SENSE_TYPE_SCSI:
		if (!csi_xsense_scsi(xdrsp, &res_dta->sense_scsi))
		{
			MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
				 "csi_xsense_scsi()", 
				 MMSG(928, 
				      "XDR message translation failure")));
			return (0);
		}
		part_size = sizeof(res_dta->sense_scsi);
		csi_xcur_size += part_size;
		break;
	  case SENSE_TYPE_FSC:
		if (!csi_xsense_fsc(xdrsp, &res_dta->sense_fsc))
		{
			MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
				 "csi_xsense_fsc()", 
				 MMSG(928, 
				      "XDR message translation failure")));
			return (0);
		}
		part_size = sizeof(res_dta->sense_fsc);
		csi_xcur_size += part_size;
		break;
	  case SENSE_TYPE_NONE:
		part_size = 0;
		break;
	  case RESOURCE_CHANGE_SERIAL_NUM:
		if (!csi_xserial_num(xdrsp, &res_dta->serial_num))
		{
			MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
				 "csi_xserial_num()", 
				 MMSG(928, 
				      "XDR message translation failure")));
			return (0);
		}
		part_size = sizeof(res_dta->serial_num);
		csi_xcur_size += part_size;
		break;
	  case RESOURCE_CHANGE_LSM_TYPE:
		if (!xdr_int(xdrsp, &res_dta->lsm_type))
		{
			MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
				 "xdr_int()", 
				 MMSG(928, 
				      "XDR message translation failure")));
			return (0);
		}
		part_size = sizeof(res_dta->lsm_type);
		csi_xcur_size += part_size;
		break;
	  case RESOURCE_CHANGE_DRIVE_TYPE:
		if (!xdr_char(xdrsp, (char *) &res_dta->drive_type))
		{
			MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
				 "xdr_char()", 
				 MMSG(928, 
				      "XDR message translation failure")));
			return (0);
		}
		part_size = sizeof(res_dta->drive_type);
		csi_xcur_size += part_size;
		break;
      case DRIVE_ACTIVITY_DATA_TYPE:
        if (!csi_xdrive_data(xdrsp, &res_dta->drive_activity_data))
        {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
                 "csi_xdrive_data()",
                 MMSG(928,
                      "XDR message translation failure")));
            return (0);
        }
        part_size = sizeof(res_dta->drive_activity_data);
        csi_xcur_size += part_size;
        break;
	  default:
		/* log two messages to make the problem clear */
		/* invalid type, cannot decipher */
		MLOGCSI((STATUS_INVALID_TYPE, st_module, CSI_NO_CALLEE, 
			 MMSG(928, "XDR message translation failure")));
		/* translation failed */
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 CSI_NO_CALLEE, 
			 MMSG(928, "XDR message translation failure")));
		return (0);
	  }
	/*
             *  xdr padding bytes if xdr'd member is not the full size of the
             *  union. We must send whole union on wire to maintain packet structure.*/

	/* point to end of currently translated bytes */
	cp = (char *)(res_dta + part_size);
	/* determine if any padding bytes are left to be translated */
	pad_size = sizeof(res_dta->resource_align_pad) - part_size;
	/* translate trailing bytes */
	if (pad_size > 0)
	{
		/* Use a temporary buffer when decoding padding bytes and decode
                        * padding bytes with maxsize RESOURCE_ALIGN_PAD_SIZE. This approach
                        * has the side effect of correctly updating the xdrsp pointer.*/
		if (XDR_DECODE == xdrsp->x_op)
		{
			char		temp[RESOURCE_ALIGN_PAD_SIZE];/* temp buffer for xdr_bytes */
			char		*tmp = temp;		/* xdr_bytes expects as arg */

			if (!xdr_bytes(xdrsp, &tmp, &pad_size, 
				       RESOURCE_ALIGN_PAD_SIZE))
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
			if (!xdr_bytes(xdrsp,&cp,&pad_size,pad_size))
			{
				MLOGCSI((
				STATUS_TRANSLATION_FAILURE,st_module, 
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

