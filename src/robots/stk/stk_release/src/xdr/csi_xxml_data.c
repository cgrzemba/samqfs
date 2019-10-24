# ifndef lint					     
static char SccsId[] = "@(#)csi_xxml_data.c	2.2 11/12/01 ";
# endif
/*
 * Copyright (2002, 2011) Oracle and/or its affiliates.  All rights reserved.
 * Name:
 *      csi_xxml_data()
 *
 * Description:
 *      Routine serializes/deserializes a xml_data structure.
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
 *     Chris Higgins      30-Oct-2001   Original
 *     Scott Siao         26-Apr-2002   Added Revision history and commented
 *                                      out some printfs in debug code because
 *                                      it prints too much data for standard
 *                                      debug mode.
 */

/*      Header Files: */
# include "csi.h"
# include "ml_pub.h"

/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static char *st_src = __FILE__;
static char *st_module = "csi_xxml_data()";
/*      Procedure Type Declarations: */

bool_t
csi_xxml_data(
	      XDR *xdrsp,			/* xdr handle structure */
	      DISPLAY_XML_DATA *xp			/* xml_data pointer */
)
{
	char *xmlString;	   /* used to point to the start of the data*/
	unsigned int size = 0;       /* size of the data */
	char * temp;
		
int i;
# ifdef DEBUG
	if TRACE(CSI_XDR_TRACE_LEVEL)
		cl_trace(st_module, 2, (unsigned long) xdrsp, 
			 (unsigned long) xp);

     /*   temp = (char *) xp;
        for(i=0;i<90;i++,temp++){
        printf("temp%d is %02X",i,*temp);} */
# endif /* DEBUG */
	if (!xdr_u_short(xdrsp, &xp->length))
	{
	  MLOGCSI((STATUS_TRANSLATION_FAILURE,st_module,"xdr_enum()", 
	  MMSG(928, "XDR message translation failure")));
	  return (0);
	}

	xmlString = xp->xml_data;
	size = xp->length;
	if (!xdr_bytes(xdrsp, &xmlString, &size, size))
	{
	  MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
	  "xdr_bytes()", 
	  MMSG(928, "XDR message translation failure")));
	  return (0);
	}
	return (1);
}

