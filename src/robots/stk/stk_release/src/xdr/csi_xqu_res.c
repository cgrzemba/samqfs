#ifndef lint
static char SccsId[] = "@(#)csi_xqu_res.c	5.10 10/21/94 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_xqu_response()
 *
 * Description:
 *      Note:  This routine must only be considered within the context of
 *      csi_xdrresponse().  Please see documentation there before proceeding.
 *
 *      Routine serializes/deserializes a CSI_QUERY_RESPONSE structure 
 *      from the point beginning with the type (type TYPE).  The
 *      data above that point in the CSI_QUERY_RESPONSE structure
 *      should have already been translated.
 *
 *      This is a support routine normally only called by csi_xdrresponse().
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
 *      Translation must start at the "type" since the REQUEST_HEADER structure 
 *      and RESPONSE_STATUS was already translated.  
 *
 *      Variable csi_xexp_size (global int) must be set upon entry for this
 *      function and its static brother functions below to work properly.
 *      
 * Module Test Plan:
 *      NONE
 *
 * Revision History:
 *      J. A. Wishner       31-Jan-1989.    Created.
 *      J. A. Wishner       01-May-1989.    Bug: st_xqu_lsm() changed from call
 *                                          to csi_xlsm() to csi_xlsm_id().
 *      J. A. Wishner       27-Jul-1989.    Bugfix.  Invalid location type
 *                                          is no longer translated.  Packet is
 *                                          an error and immediately truncated.
 *      D. F. Reed          26-Oct-1989.    Modify TYPE_MOUNT interpretation
 *                                          to accommodate drive_status instead
 *                                          of drive_id.
 *      J. W. Montgomery    27-Jul-1990.    Version 2.
 *      J. A. Wishner       20-Oct-1991.    Delete drives, count, i; unused.
 *      Emanuel A. Alongi   22-Jun-1993.    Modifications to support R5.0 query
 *					    response structure changes.
 *      Emanuel A. Alongi   24-Aug-1993.    Change drive_count to mount_status_
 *					    count for TYPE_MOUNT and from 
 *					    drive_count to  msc_status_count
 *					    for TYPE_MOUNT_SCRATCH.
 *      Emanuel A. Alongi   01-Sep-1993.    Added missing maxlength parameter to
 *					    xdr_string().
 *	E. A. Alongi	    15-Sep-1993	    Added static function prototypes and
 *					    inserted logical not where missing.
 *					    Added MAX_ID boundary checks.
 *	Emanuel A. Alongi   01-Oct-1993	    Corrected flint errors.
 *	Emanuel A. Alongi   16-Feb-1994	    R5.0 BR#43 and R5.0 BR#44
 *					    Eliminated st_xqu_cap() function
 *					    which was no longer referenced.
 *					    Modified code to address bug #43 and
 *					    #44 - for type server.
 *	E. A. Alongi	    21-Oct-1994	    R5.0 BR#329 - corrected translation
 *					    order of QU_DRV_STATUS structure
 *					    members in st_xqu_drive() to
 *					    reflect the true order which appears
 *					    in h/api/structs_api.h.
 *	S. L. Siao	    13-Feb-2002	    Added query subpool name, 
 *                                          query drive group.
 *	S. L. Siao	    28-Feb-2002	    Added query_mount_scratch_pinfo 
 *	S. L. Siao	    20-Mar-2002	    Added type cast to arguments of xdr
 *                                          functions.
 *      
 */


/*      Header Files: */
#include "csi.h"
#include "cl_pub.h"
#include "ml_pub.h"
#include "csi_xdr_pri.h"

/*      Defines, Typedefs and Structure Definitions: */
#define CHECKSIZE(cursize, objsize, totsize) \
    (cursize + objsize > totsize) ?  TRUE : FALSE

/*      Global and Static Variable Declarations: */
static char     *st_module = "csi_xqu_response()";

/*      Static Prototype Declarations: */
static bool_t st_xqu_server (XDR *xdrsp, QU_SRV_STATUS *statp);
static bool_t st_xqu_acs (XDR *xdrsp, QU_ACS_STATUS *statp);
static bool_t st_xqu_lsm (XDR *xdrsp, QU_LSM_STATUS *statp);
static bool_t st_xqu_xcap (XDR *xdrsp, QU_CAP_STATUS *statp);
static bool_t st_xqu_drive (XDR *xdrsp, QU_DRV_STATUS *statp);
static bool_t st_xqu_volume (XDR *xdrsp, QU_VOL_STATUS *statp);
static bool_t st_xqu_mount (XDR *xdrsp, QU_MNT_STATUS *statp);
static bool_t st_xqu_port (XDR *xdrsp, QU_PRT_STATUS *statp);
static bool_t st_xqu_reqstat (XDR *xdrsp, QU_REQ_STATUS *statp);
static bool_t st_xqu_scratch (XDR *xdrsp, QU_SCR_STATUS *statp);
static bool_t st_xqu_pool (XDR *xdrsp, QU_POL_STATUS *statp);
static bool_t st_xqu_mount_scratch (XDR *xdrsp, QU_MSC_STATUS *statp);
static bool_t st_xqu_clean_volume (XDR *xdrsp, QU_CLN_STATUS *statp);
static bool_t st_xqu_mm_info_mt (XDR *xdrsp, QU_MEDIA_TYPE_STATUS *statp);
static bool_t st_xqu_mm_info_dt (XDR *xdrsp, QU_DRIVE_TYPE_STATUS *statp);
static bool_t st_xqu_drv_grp (XDR *xdrsp, QU_VIRT_DRV_MAP *statp);
static bool_t st_xqu_spn (XDR *xdrsp, QU_SUBPOOL_NAME_STATUS *statp);

/*      Procedure Type Declarations: */

bool_t 
csi_xqu_response (
    XDR *xdrsp,              /* xdr handle structure */
    CSI_QUERY_RESPONSE *resp               /* vary response structure */
)
{
    register int        i;                      /* loop counter */
    register int        count;                  /* holds count value */
    register int        part_size = 0;          /* size this part of packet */
    BOOLEAN             badsize;                /* TRUE if packet size bogus */
    char		*strp;			/*xdr_string() requires a */
						/* pointer to a pointer */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module, 2, (unsigned long) xdrsp, (unsigned long) resp);
#endif /* DEBUG */

    /* translate type */
    part_size = (char*) &resp->status_response - (char*) &resp->type;
    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xtype(xdrsp, &resp->type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xtype()",
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    switch (resp->type) {
        case TYPE_SERVER:

	    /* In previous releases, the count field was located in the fixed
	     * portion of the query response structure.  In R5.0 the count
	     * field was moved into the variable portion as part of each
	     * individual command response structure.  The one exception being
	     * the QU_SRV_RESPONSE structure which no longer includes a count
	     * (it made no sense anyway - there is only one server).  The ACSLM
	     * used to return 0 in the count field when an error was detected.
	     * This prevented the CSI/SSI from translating the variable portion
	     * of the response.  This is still true in R5.0 for all other 
	     * responses except query server.  The solution was to check the
	     * message status field and if an error was detected, terminate the
	     * translation and return the (translated) fixed portion of the
	     * packet.  This change addresses R5.0 bug #43 and #44.
	     */
	    if (resp->message_status.status != STATUS_SUCCESS) {

		/* the assumption is that csi_xcur_size, set just before the
		 * switch on resp->type, is correct if we return at this point.
		 */
		return (1);
	    }

            /* translate rest of packet as server query status_response */
            if (!st_xqu_server(xdrsp, &resp->status_response.server_response.
								server_status)){
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xqu_server()",
		  	 MMSG(928, "XDR message translation failure")));
                return(0);
            }

            csi_xcur_size = ((char *)&resp->status_response.server_response.
		        server_status + sizeof(QU_SRV_RESPONSE)) - (char *)resp;
            break;

        case TYPE_ACS:

    	    /* translate the acs count */
    	    part_size = (char*) resp->status_response.acs_response.acs_status -
			  (char*) &resp->status_response.acs_response.acs_count;
    	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    	    if (badsize || !xdr_u_short(xdrsp,
			       &resp->status_response.acs_response.acs_count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()",
				MMSG(928, "XDR message translation failure")));
        	return(0);
    	    }
            csi_xcur_size += part_size;

	    /* check boundary condition before loop */
	    if (resp->status_response.acs_response.acs_count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE,
			MMSG(928, "XDR message translation failure")));
                return(0);
            }

    	    for (i = 0, count = resp->status_response.acs_response.acs_count;
		 i < count; i++ ) {

                /* translate rest of the packet as acs query status_response */
                if (!st_xqu_acs(xdrsp,
			  &resp->status_response.acs_response.acs_status[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
			    "st_xqu_acs()", 
			    MMSG(928, "XDR message translation failure")));
                    return(0);
                }
	    }
            csi_xcur_size = (char *)&resp->status_response.acs_response.
                      acs_status[resp->status_response.acs_response.acs_count] -
		      (char *)resp;
            break;

        case TYPE_LSM:

    	    /* translate the lsm count */
    	    part_size = (char*) resp->status_response.lsm_response.lsm_status -
	    		  (char*) &resp->status_response.lsm_response.lsm_count;
    	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    	    if (badsize || !xdr_u_short(xdrsp,
			      &resp->status_response.lsm_response.lsm_count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE,  st_module, 			     "xdr_u_short()", MMSG(928,                	     "XDR message translation failure")));
        	return(0);
    	    }
            csi_xcur_size += part_size;

	    /* check boundary condition before loop */
	    if (resp->status_response.lsm_response.lsm_count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE,  st_module, 				CSI_NO_CALLEE, MMSG(928, 				"XDR message translation failure")));
                return(0);
            }

    	    for (i = 0, count = resp->status_response.lsm_response.lsm_count;
		 i < count; i++ ) {

                /* translate rest of the packet as lsm query status_response */
                if (!st_xqu_lsm(xdrsp,
			   &resp->status_response.lsm_response.lsm_status[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE,  st_module, 				"st_xqu_lsm()", MMSG(928,                        	"XDR message translation failure")));
                    return(0);
                }
	    }
            csi_xcur_size = (char *)&resp->status_response.lsm_response.
                      lsm_status[resp->status_response.lsm_response.lsm_count] -
		      (char *)resp;
            break;

        case TYPE_CAP:

    	    /* translate the cap count */
    	    part_size = (char*) resp->status_response.cap_response.cap_status -
			  (char*) &resp->status_response.cap_response.cap_count;
    	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    	    if (badsize || !xdr_u_short(xdrsp,
			       &resp->status_response.cap_response.cap_count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE,  st_module, 			     "xdr_u_short()", MMSG(928,                	     "XDR message translation failure")));
        	return(0);
    	    }
            csi_xcur_size += part_size;

	    /* make sure this is a VERSION2 or greater packet */
            if (!(resp->csi_request_header.message_header.message_options & 
                                                            EXTENDED)) {
                /* error, extended must be set for version 2 and beyond */
                MLOGCSI((STATUS_TRANSLATION_FAILURE,  st_module,  				"missing extended option", MMSG(928,                        	"XDR message translation failure")));
                return(0);
            }

	    /* check boundary condition before loop */
    	    if (resp->status_response.cap_response.cap_count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE,  st_module, 				CSI_NO_CALLEE, MMSG(928, 				"XDR message translation failure")));
                return(0);
            }

    	    for (i = 0, count = resp->status_response.cap_response.cap_count;
		 i < count; i++ ) {

                /* translate rest of the packet as xcapid status_response */
                if (!st_xqu_xcap(xdrsp,
			   &resp->status_response.cap_response.cap_status[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE,  st_module, 				"st_xqu_xcap()", MMSG(928,                        	"XDR message translation failure")));
                    return(0);
                }
	    }
            csi_xcur_size = (char *)&resp->status_response.cap_response.
                      cap_status[resp->status_response.cap_response.cap_count] -
		      (char *)resp;
            break;

        case TYPE_DRIVE:

    	    /* translate the drive count */
    	    part_size = (char*)
			     resp->status_response.drive_response.drive_status -
		      (char*) &resp->status_response.drive_response.drive_count;
    	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    	    if (badsize || !xdr_u_short(xdrsp,
			   &resp->status_response.drive_response.drive_count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE,  st_module, 			     "xdr_u_short()", MMSG(928,                	     "XDR message translation failure")));
        	return(0);
    	    }
            csi_xcur_size += part_size;

	    /* check boundary condition before loop */
	    if (resp->status_response.drive_response.drive_count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE,  st_module, 				CSI_NO_CALLEE, MMSG(928, 				"XDR message translation failure")));
                return(0);
            }

    	    for (i = 0,
		       count = resp->status_response.drive_response.drive_count;
		       i < count; i++ ) {

                /* translate rest of the packet as drive query status_response*/
                if (!st_xqu_drive(xdrsp,
		       &resp->status_response.drive_response.drive_status[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE,  st_module, 				"st_xqu_drive()", MMSG(928,                        	"XDR message translation failure")));
                    return(0);
                }
	    }
            csi_xcur_size = (char *)&resp->status_response.drive_response.
                drive_status[resp->status_response.drive_response.drive_count] -
		(char *)resp;
            break;

        case TYPE_VOLUME:

    	    /* translate the volume count */
    	    part_size = (char*) &resp->status_response.volume_response.
			 volume_status[0] - (char*) &resp->status_response.
						   volume_response.volume_count;
    	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    	    if (badsize || !xdr_u_short(xdrsp,
			 &resp->status_response.volume_response.volume_count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE,  st_module, 			     "xdr_u_short()", MMSG(928,                	     "XDR message translation failure")));
        	return(0);
    	    }
            csi_xcur_size += part_size;

	    /* check boundary condition before loop */
	    if (resp->status_response.volume_response.volume_count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE,  st_module, 				CSI_NO_CALLEE, MMSG(928, 				"XDR message translation failure")));
                return(0);
            }

    	    for (i = 0,
		     count = resp->status_response.volume_response.volume_count;
		     i < count; i++ ) {

               /* translate rest of packet as volume query status_response */
               if (!st_xqu_volume(xdrsp,
	              &resp->status_response.volume_response.volume_status[i])){
                   MLOGCSI((STATUS_TRANSLATION_FAILURE,  st_module, 				"st_xqu_volume()", MMSG(928,                        	"XDR message translation failure")));
                   return(0);
               }
	    }
            csi_xcur_size = (char *)&resp->status_response.volume_response.
               volume_status[resp->status_response.volume_response.volume_count]
	       - (char *)resp;
            break;

        case TYPE_MOUNT:

    	    /* translate the mount status count */
    	    part_size = (char*)
			    resp->status_response.mount_response.mount_status -
		        (char*) &resp->status_response.mount_response.
							     mount_status_count;
    	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    	    if (badsize || !xdr_u_short(xdrsp,
		    &resp->status_response.mount_response.mount_status_count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()",
			 MMSG(928, "XDR message translation failure")));
        	return(0);
    	    }
            csi_xcur_size += part_size;

	    /* check boundary condition before loop */
	    if (resp->status_response.mount_response.mount_status_count >
								       MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE,
			 MMSG(928, "XDR message translation failure")));
                return(0);
            }

    	    for (i = 0,
		     count = resp->status_response.mount_response.
							     mount_status_count;
		     i < count; i++ ) {

                /* translate rest of the packet as mount query status_response*/
                if (!st_xqu_mount(xdrsp,
		       &resp->status_response.mount_response.mount_status[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			     "st_xqu_mount()", MMSG(928, 
					   "XDR message translation failure")));
                    return(0);
                }
	    }
            csi_xcur_size = (char *)&resp->status_response.mount_response.
                mount_status[resp->status_response.mount_response.
			     mount_status_count] - (char *)resp;
            break;

        case TYPE_PORT:

    	    /* translate the port count */
    	    part_size = (char*)resp->status_response.port_response.port_status -
		        (char*) &resp->status_response.port_response.port_count;
    	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    	    if (badsize || !xdr_u_short(xdrsp,
			 &resp->status_response.port_response.port_count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()",
			 MMSG(928, "XDR message translation failure")));
        	return(0);
    	    }
            csi_xcur_size += part_size;

	    /* check boundary condition before loop */
	    if (resp->status_response.port_response.port_count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE,
			 MMSG(928, "XDR message translation failure")));
                return(0);
            }

    	    for (i = 0,
		     count = resp->status_response.port_response.port_count;
		     i < count; i++ ) {

                /* translate rest of packet as port query status_response */
                if (!st_xqu_port(xdrsp,
			 &resp->status_response.port_response.port_status[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			     "st_xqu_port()", MMSG(928, 
					   "XDR message translation failure")));
                    return(0);
                }
	    }
            csi_xcur_size = (char *)&resp->status_response.port_response.
                  port_status[resp->status_response.port_response.port_count] -
		  (char *)resp;
            break;

        case TYPE_REQUEST:

    	    /* translate the request count */
    	    part_size = (char*)
			resp->status_response.request_response.request_status -
		  (char*) &resp->status_response.request_response.request_count;
    	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    	    if (badsize || !xdr_u_short(xdrsp,
		       &resp->status_response.request_response.request_count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()",
			 MMSG(928, "XDR message translation failure")));
        	return(0);
    	    }
            csi_xcur_size += part_size;

	    /* check boundary condition before loop */
	    if (resp->status_response.request_response.request_count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE,
			 MMSG(928, "XDR message translation failure")));
                return(0);
            }

    	    for (i = 0,
		   count = resp->status_response.request_response.request_count;
		   i < count; i++ ) {

                /* translate rest of packet as request query status_response */
                if (!st_xqu_reqstat(xdrsp,
                   &resp->status_response.request_response.request_status[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			     "st_xqu_reqstat()", MMSG(928, 
					   "XDR message translation failure")));
                    return(0);
                }
	    }
            csi_xcur_size = (char *)&resp->status_response.request_response.
                	  request_status[resp->status_response.request_response.
			  		 request_count] - (char *)resp;
            break;

        case TYPE_SCRATCH:

    	    /* translate the scratch count */
    	    part_size = (char*)
			resp->status_response.scratch_response.scratch_status -
		  (char*) &resp->status_response.scratch_response.volume_count;
    	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    	    if (badsize || !xdr_u_short(xdrsp,
		       &resp->status_response.scratch_response.volume_count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()",
			 MMSG(928, "XDR message translation failure")));
        	return(0);
    	    }
            csi_xcur_size += part_size;

	    /* check boundary condition before loop */
	    if (resp->status_response.scratch_response.volume_count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE,
			 MMSG(928, "XDR message translation failure")));
                return(0);
            }

    	    for (i = 0,
		   count = resp->status_response.scratch_response.volume_count;
		   i < count; i++ ) {

                /* translate rest of packet as scratch query status_response */
                if (!st_xqu_scratch(xdrsp,
                   &resp->status_response.scratch_response.scratch_status[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			     "st_xqu_scratch()", MMSG(928, 
			     "XDR message translation failure")));
                    return(0);
                }
	    }
            csi_xcur_size = (char *)&resp->status_response.scratch_response.
                	scratch_status[resp->status_response.scratch_response.
			               volume_count] - (char *)resp;
            break;

        case TYPE_POOL:

    	    /* translate the pool count */
    	    part_size = (char*)
			resp->status_response.pool_response.pool_status -
		        (char*) &resp->status_response.pool_response.pool_count;
    	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    	    if (badsize || !xdr_u_short(xdrsp,
		            &resp->status_response.pool_response.pool_count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()",
			 MMSG(928, "XDR message translation failure")));
        	return(0);
    	    }
            csi_xcur_size += part_size;

	    /* check boundary condition before loop */
	    if (resp->status_response.pool_response.pool_count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE,
			 MMSG(928, "XDR message translation failure")));
                return(0);
            }

    	    for (i = 0,
		   count = resp->status_response.pool_response.pool_count;
		   i < count; i++ ) {

                /* translate rest of packet as pool query status_response */
                if (!st_xqu_pool(xdrsp,
                	 &resp->status_response.pool_response.pool_status[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			     "st_xqu_pool()", MMSG(928, 
					   "XDR message translation failure")));
                    return(0);
                }
	    }
            csi_xcur_size = (char *)&resp->status_response.pool_response.
                  pool_status[resp->status_response.pool_response.pool_count] 
		  - (char *)resp;
            break;

        case TYPE_MOUNT_SCRATCH_PINFO:
        case TYPE_MOUNT_SCRATCH:

    	    /* translate the mount_scratch count */
    	    part_size = (char*) resp->status_response.mount_scratch_response.
			mount_scratch_status - (char*) &resp->status_response.
				     mount_scratch_response.msc_status_count;
    	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    	    if (badsize || !xdr_u_short(xdrsp, &resp->status_response.
				  mount_scratch_response.msc_status_count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()",
			 MMSG(928, "XDR message translation failure")));
        	return(0);
    	    }
            csi_xcur_size += part_size;

	    /* check boundary condition before loop */
	    if (resp->status_response.mount_scratch_response.msc_status_count >
								       MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE,
			 MMSG(928, "XDR message translation failure")));
                return(0);
            }

    	    for (i = 0,
	    		count = resp->status_response.mount_scratch_response.
							       msc_status_count;
			i < count; i++ ) {

                /* translate rest of packet as mount_scratch query
		 * status_response */
                if (!st_xqu_mount_scratch(xdrsp, &resp->status_response.
			     mount_scratch_response.mount_scratch_status[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			     "st_xqu_mount_scratch()", MMSG(928, 
					  "XDR message translation failure")));
                    return(0);
                }
	    }
            csi_xcur_size = (char *)&resp->status_response.
			    mount_scratch_response.mount_scratch_status[
			          resp->status_response.mount_scratch_response.
				  msc_status_count] - (char *)resp;
            break;

        case TYPE_CLEAN:

    	    /* translate the clean_volume count */
    	    part_size = (char*) resp->status_response.clean_volume_response.
			clean_volume_status - (char*) &resp->status_response.
				             clean_volume_response.volume_count;
    	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    	    if (badsize || !xdr_u_short(xdrsp, &resp->status_response.
				          clean_volume_response.volume_count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()",
			 MMSG(928, "XDR message translation failure")));
        	return(0);
    	    }
            csi_xcur_size += part_size;

	    /* check boundary condition before loop */
    	    if (resp->status_response.clean_volume_response.volume_count > 
								       MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE,
			 MMSG(928, "XDR message translation failure")));
                return(0);
            }

    	    for (i = 0, count = resp->status_response.clean_volume_response.  
						volume_count; i < count; i++ ) {

                /* translate rest of packet as clean query status_response */
                if (!st_xqu_clean_volume(xdrsp, &resp->status_response.
			       clean_volume_response.clean_volume_status[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			     "st_xqu_clean_volume()", MMSG(928, 
					  "XDR message translation failure")));
                    return(0);
                }
	    }
            csi_xcur_size = (char*)&resp->status_response.clean_volume_response.
			    clean_volume_status[resp->status_response.
			    clean_volume_response.volume_count] - (char *)resp;
            break;

	case TYPE_MIXED_MEDIA_INFO:

    	    /* translate the media_type count */
    	    part_size = (char*) &resp->status_response.mm_info_response.
			media_type_status[0] - (char*) &resp->status_response.
				             mm_info_response.media_type_count;
    	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    	    if (badsize || !xdr_u_short(xdrsp, &resp->status_response.
				          mm_info_response.media_type_count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()",
			 MMSG(928, "XDR message translation failure")));
        	return(0);
    	    }
            csi_xcur_size += part_size;

	    /* check boundary condition before loop */
	    if (resp->status_response.mm_info_response.media_type_count >
								      MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE,
			 MMSG(928, "XDR message translation failure")));
                return(0);
            }

    	    for (i = 0, count =
			resp->status_response.mm_info_response.media_type_count;
	    		i < count; i++ ) {

                /* translate media_type_status part of packet */
                if (!st_xqu_mm_info_mt(xdrsp, &resp->status_response.
				       mm_info_response.media_type_status[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			     "st_xqu_clean_volume()", MMSG(928, 
					   "XDR message translation failure")));
                    return(0);
                }
	    }
            csi_xcur_size = (char*)&resp->status_response.mm_info_response.
			    media_type_status[resp->status_response.
			    mm_info_response.media_type_count] - (char *)resp;

    	    /* translate the drive_type count */
    	    part_size = (char*) &resp->status_response.mm_info_response.
			drive_type_status[0] - (char*) &resp->status_response.
				             mm_info_response.drive_type_count;
    	    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    	    if (badsize || !xdr_u_short(xdrsp, &resp->status_response.
				          mm_info_response.drive_type_count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()",			 MMSG(928, "XDR message translation failure")));
        	return(0);
    	    }
            csi_xcur_size += part_size;

	    /* check boundary condition before loop */
	    if (resp->status_response.mm_info_response.drive_type_count >
								      MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE,
			 MMSG(928, "XDR message translation failure")));
                return(0);
            }

    	    for (i = 0, count =
			resp->status_response.mm_info_response.drive_type_count;
	    		i < count; i++ ) {

                /* translate drive_type_status part of packet */
                if (!st_xqu_mm_info_dt(xdrsp, &resp->status_response.
				       mm_info_response.drive_type_status[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			     "st_xqu_clean_volume()", MMSG(928, 
					   "XDR message translation failure")));
                    return(0);
                }
	    }
            csi_xcur_size = (char*)&resp->status_response.mm_info_response.
			    drive_type_status[resp->status_response.
			    mm_info_response.drive_type_count] - (char *)resp;
            break;

	case TYPE_DRIVE_GROUP:

	    /* translate the group id */
	    part_size = (char*) &resp->status_response.
				       drive_group_response.group_type -
			(char*) &resp->status_response.drive_group_response.
				       group_id;
	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

	    strp = (char *)&resp->status_response.drive_group_response.group_id;

	    if (badsize || !xdr_string(xdrsp, &strp, GROUPID_SIZE+1)) {
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_string()",
			MMSG(928, "XDR message translation failure")));
		return(0);
	    }
	    csi_xcur_size += part_size;

	    /* translate the group type */
	    part_size = (char*) &resp->status_response.drive_group_response.
				       vir_drv_map_count -
			(char*) &resp->status_response.drive_group_response.
				       group_type;
	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

	    if (badsize || !csi_xgrp_type(xdrsp,
	      &resp->status_response.drive_group_response.group_type)) {
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			"csi_xgrp_type()",
			MMSG(928, "XDR message translation failure")));
		return(0);
	    }

	    csi_xcur_size += part_size;

	    /* translate the virtual drive map count */
	    part_size = (char*) resp->status_response.drive_group_response.
				      virt_drv_map - 
			(char*) &resp->status_response.drive_group_response.
				      vir_drv_map_count;
	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

	    if (badsize || !xdr_u_short(xdrsp,
	    &resp->status_response.drive_group_response.vir_drv_map_count)) {
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()",
			MMSG(928, "XDR message translation failure")));
		return(0);
	    }

	    csi_xcur_size += part_size;

	    /* check boundary condition before loop */

	    if (resp->status_response.drive_group_response.vir_drv_map_count >
	      MAX_VTD_MAP) {
		MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE,
			MMSG(928, "XDR message translation failure")));
		return(0);
	    }

	    count = resp->status_response.drive_group_response.
			  vir_drv_map_count;

	    for (i = 0; i < count; i++ ) {

		/* translate rest of the packet as drive query status_response*/
		if (!st_xqu_drv_grp(xdrsp,
		  &resp->status_response.drive_group_response.virt_drv_map[i])){
		    MLOGCSI((STATUS_TRANSLATION_FAILURE, 
			    st_module, "st_xqu_drv_grp()",
			    MMSG(928, "XDR message translation failure")));
		    return(0); 
		}
	    }
	    csi_xcur_size = (char *)&resp->status_response.drive_group_response.
	    virt_drv_map[count] - (char *)resp;
	    break;
	case TYPE_SUBPOOL_NAME:

	     /* translate the subpool status count */
	     part_size = (char*) resp->status_response.subpl_name_response.
			   subpl_name_status -
			 (char*) &resp->status_response.subpl_name_response.
			   spn_status_count;
	     badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	     if (badsize || !xdr_u_short(xdrsp, &resp->status_response.
						       subpl_name_response.
						       spn_status_count)) {
		 MLOGCSI((STATUS_TRANSLATION_FAILURE,
		         st_module, "xdr_u_short()",
			 MMSG(928, "XDR message translation failure")));
		 return(0);
	     }
	     csi_xcur_size += part_size;

	     /* check boundary condition before loop */
	     if (resp->status_response.subpl_name_response.
		   spn_status_count > MAX_ID) {
		 MLOGCSI((STATUS_COUNT_TOO_LARGE,
		         st_module, CSI_NO_CALLEE,
			 MMSG(928, "XDR message translation failure")));
		 return(0);
	     }

	     for (i = 0,
	     count = resp->status_response.subpl_name_response.spn_status_count;
	     i < count; i++ ) {
		 /* translate rest of packet as pool query status_response */
		 if (!st_xqu_spn(xdrsp, &resp->status_response.
					       subpl_name_response.
					       subpl_name_status[i])) {
		      MLOGCSI((STATUS_TRANSLATION_FAILURE,
		              st_module, "st_xqu_spn()", 
			      MMSG(928, "XDR message translation failure")));
		      return(0);
		 }

	      }
	      csi_xcur_size = (char *)&resp->status_response.
					     subpl_name_response.
					     subpl_name_status[resp->
					     status_response.
					     subpl_name_response.
					     spn_status_count] - 
			      (char *)resp;
	      break;

        default:

            /* If this code segment is entered, the type is illegal. */
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE,
		     MMSG(928, "XDR message translation failure")));
            return(0);
    }
    return(1);
}


/*
 * Name:
 *      st_xqu_server
 *
 * Description:
 *      Serializes/Deserializes the server_status portion of a 
 *      CSI_QUERY_RESPONSE packet "status_response" from the point after 
 *      the identifier count.  The upper contents of the packet/structure 
 *      above the variable "count" should have already been translated.
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *      NONE            (list of any outputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 *
 */

static bool_t 
st_xqu_server (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_SRV_STATUS *statp                 /* server status */
)
{
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size bogus */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xqu_server()", 2, (unsigned long) xdrsp,
             (unsigned long) statp);
#endif /* DEBUG */

    /* translate state */
    part_size = (char*) &statp->freecells - (char*) &statp->state;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstate(xdrsp, &statp->state)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,                 "st_xqu_server()",  "csi_xstate()", MMSG(928,                "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate freecells */
    part_size = (char*) &statp->requests - (char*) &statp->freecells;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xfreecells(xdrsp, &statp->freecells)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,                 "st_xqu_server()",  "csi_xfreecells()", MMSG(928,                "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate requests (type REQ_SUMMARY) */
    part_size = sizeof(statp->requests);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xreqsummary(xdrsp, &statp->requests)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,                 "st_xqu_server()",  "csi_xreqsummary()", MMSG(928,                "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    return(1);
}

/*
 * Name:
 *      st_xqu_acs()
 *
 * Description:
 *      Serializes/Deserializes the acs_status portion of a 
 *      CSI_QUERY_RESPONSE packet "status_response" from the point after 
 *      the identifier "count".  The upper contents of the packet/structure 
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
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
 */

static bool_t 
st_xqu_acs (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_ACS_STATUS *statp                 /* acs status */
)
{
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xqu_acs()", 2, (unsigned long)xdrsp, (unsigned long)statp);
#endif /* DEBUG */

    /* translate acs_id */
    part_size = (char *)&statp->state - (char *)&statp->acs_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xacs(xdrsp, &statp->acs_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_acs()",  "csi_xacs()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate state */
    part_size = (char*) &statp->freecells - (char*) &statp->state;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstate(xdrsp, &statp->state)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_acs()",  "csi_xstate()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate freecells */
    part_size = (char*) &statp->requests - (char*) &statp->freecells;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xfreecells(xdrsp, &statp->freecells)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_acs()",  "csi_xfreecells()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate requests (type REQ_SUMMARY) */
    part_size = (char*) &statp->status - (char*) &statp->requests;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xreqsummary(xdrsp, &statp->requests)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_acs()",  "csi_xreqsummary()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_acs()",  "csi_xstatus()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;
    return(1);
}


/*
 * Name:
 *      st_xqu_lsm()
 *
 * Description:
 *      Serializes/Deserializes the "lsm_status" portion of a 
 *      "CSI_QUERY_RESPONSE" packet "status_response" from the point after the 
 *      identifier "count".  The upper contents of the packet/structure above 
 *      the variable "count" should have already been translated. 
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
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
 */

static bool_t 
st_xqu_lsm (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_LSM_STATUS *statp                 /* lsm status */
)
{
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xqu_lsm()", 2, (unsigned long)xdrsp, (unsigned long)statp);
#endif /* DEBUG */

    /* translate lsm_id */
    part_size = (char*) &statp->state - (char*) &statp->lsm_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xlsm_id(xdrsp, &statp->lsm_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_lsm()",  "csi_xlsm()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate state */
    part_size = (char*) &statp->freecells - (char*) &statp->state;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstate(xdrsp, &statp->state)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_lsm()",  "csi_xstate()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate freecells */
    part_size = (char*) &statp->requests - (char*) &statp->freecells;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xfreecells(xdrsp, &statp->freecells)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_lsm()",  "csi_xfreecells()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate requests (type REQ_SUMMARY) */
    part_size = (char*) &statp->status - (char*) &statp->requests;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xreqsummary(xdrsp, &statp->requests)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_lsm()",  "csi_xreqsummary()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(statp->status);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_lsm",  "csi_xstatus()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;
    return(1);
}

/*
 * Name:
 *      st_xqu_drive()
 *
 * Description:
 *      Serializes/Deserializes the "drive_status" portion of a 
 *      "CSI_QUERY_RESPONSE" packet "status_response" from the point after the 
 *      identifier "count".  The upper contents of the packet/structure above 
 *      the variable "count" should have already been translated. 
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *      NONE            (list of any outputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 */

static bool_t 
st_xqu_drive (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_DRV_STATUS *statp                 /* drive status */
)
{
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */


#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xqu_drive()", 2, (unsigned long)xdrsp, 
            (unsigned long)statp);
#endif /* DEBUG */

    /* translate driveid */
    part_size = (char*) &statp->vol_id - (char*) &statp->drive_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xdrive_id(xdrsp, &statp->drive_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_drive()", 
		 "csi_xdrive_id()", MMSG(928,
					 "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate volid */
    part_size = (char*) &statp->drive_type - (char*) &statp->vol_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xvol_id(xdrsp, &statp->vol_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_drive()", "csi_xvol_id()",
				MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate drive type */
    part_size = (char*) &statp->state - (char*) &statp->drive_type;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    if (badsize || !csi_xdrive_type(xdrsp, &statp->drive_type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,  "st_xqu_drive()",
		 "csi_xdrive_type()", 
				MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate state */
    part_size = (char*) &statp->status - (char*) &statp->state;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstate(xdrsp, &statp->state)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_drive()", "csi_xstate()",
				MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_drive()", "csi_xstatus()",
				MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    return(1);
}

/*
 * Name:
 *      st_xqu_mount()
 *
 * Description:
 *      Serializes/Deserializes the "mount_status" portion of a 
 *      "CSI_QUERY_RESPONSE" packet "status_response" from the point after the 
 *      identifier "count".  The upper contents of the packet/structure above 
 *      the variable "count" should have already been translated. 
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *      NONE            (list of any outputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 */

static bool_t 
st_xqu_mount (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_MNT_STATUS *statp                 /* mount status */
)
{
    register int        drives;         /* counter for number of drives */
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xqu_mount()", 2, (unsigned long)xdrsp,
             (unsigned long)statp);
#endif /* DEBUG */

    /* translate volid */
    part_size = (char*) &statp->status - (char*) &statp->vol_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xvol_id(xdrsp, &statp->vol_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_mount()",  "csi_xvol_id()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = (char*) &statp->drive_count - (char*) &statp->status;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_mount()",  "csi_xstatus()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate drive_count */
    part_size = (char*) &statp->drive_status[0] - (char*) &statp->drive_count;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &statp->drive_count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_mount()",  "xdr_u_short()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* check boundary condition before loop */
    if (statp->drive_count > QU_MAX_DRV_STATUS) {
        MLOGCSI((STATUS_COUNT_TOO_LARGE,             "st_xqu_mount()",  CSI_NO_CALLEE, MMSG(928,             "XDR message translation failure")));
        return(0);
    }

    /* translate the drive_status's */
    for (drives = 0; drives < (int) statp->drive_count; drives++) {
        /* translate a drive_status */
        if (!st_xqu_drive(xdrsp, &statp->drive_status[drives])) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE,                 "st_xqu_mount()",  "st_xqu_drive()", MMSG(928,                "XDR message translation failure")));
            return(0);
        }
    }
    return(1);
}

/*
 * Name:
 *      st_xqu_volume()
 *
 * Description:
 *      Serializes/Deserializes the "volume_status" portion of a 
 *      "CSI_QUERY_RESPONSE" packet "status_response" from the point after the 
 *      identifier "count".  The upper contents of the packet/structure above 
 *      the variable "count" should have already been translated. 
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *      NONE            (list of any outputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 */

static bool_t 
st_xqu_volume (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_VOL_STATUS *statp                 /* volume status */
)
{
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xqu_volume()", 2, (unsigned long) xdrsp,
             (unsigned long) statp);
#endif /* DEBUG */

    /* translate volid */
    part_size = (char*) &statp->media_type - (char*) &statp->vol_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xvol_id(xdrsp, &statp->vol_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_volume()",  "csi_xvol_id()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate media type */
    part_size = (char*) &statp->location_type - (char*) &statp->media_type;

    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xmedia_type(xdrsp, &statp->media_type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,  "st_xqu_volume()", 		     "csi_xmedia_type()", MMSG(928,            	     "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate location_type */
    part_size = (char*) &statp->location - (char*) &statp->location_type;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xlocation(xdrsp, &statp->location_type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_volume()",  "csi_xlocation()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate one of two location types */
    if (LOCATION_DRIVE == statp->location_type) {
        /* translate the drive_id */
        part_size = (char*) &statp->status - (char*) &statp->location.drive_id;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !csi_xdrive_id(xdrsp, &statp->location.drive_id)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE,                 "st_xqu_volume()",  "csi_xdrive_id()", MMSG(928,                "XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += part_size;
    }
    else if (LOCATION_CELL == statp->location_type) {
        /* translate the cell_id */
        part_size = (char*) &statp->status - (char*) &statp->location.cell_id;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !csi_xcell_id(xdrsp, &statp->location.cell_id)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE,                 "st_xqu_volume()",  "csi_xcell_id()", MMSG(928,                "XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += part_size;
    }
    else {
        /* invalid location type */
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_volume()",  CSI_NO_CALLEE, MMSG(976,            "Invalid location type")));
        return(0);
    }

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_volume()",  "csi_xstatus()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    return(1);
}

/*
 * Name:
 *      st_xqu_port()
 *
 * Description:
 *      Serializes/Deserializes the "port_status" portion of a 
 *      "CSI_QUERY_RESPONSE" packet "status_response" from the point after the 
 *      identifier "count".  The upper contents of the packet/structure above 
 *      the variable "count" should have already been translated. 
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *      NONE            (list of any outputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 */

static bool_t 
st_xqu_port (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_PRT_STATUS *statp                 /* port status */
)
{
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xqu_port()", 2, (unsigned long) xdrsp,
             (unsigned long) statp);
#endif /* DEBUG */

    /* translate port_id */
    part_size = (char*) &statp->state - (char*) &statp->port_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xport_id(xdrsp, &statp->port_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_port()",  "csi_xport_id()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate state */
    part_size = (char*) &statp->status - (char*) &statp->state;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstate(xdrsp, &statp->state)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_port()",  "csi_xstate()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_port()",  "csi_xstatus()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;
    return(1);
}

/*
 * Name:
 *      st_xqu_reqstat()
 *
 * Description:
 *      Serializes/Deserializes the "request_status" portion of a 
 *      "CSI_QUERY_RESPONSE" packet "status_response" from the point after the 
 *      identifier "count".  The upper contents of the packet/structure above 
 *      the variable "count" should have already been translated. 
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *      NONE            (list of any outputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 */

static bool_t 
st_xqu_reqstat (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_REQ_STATUS *statp                 /* ptr to request status structure */
)
{
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xqu_reqstat()", 2, (unsigned long) xdrsp,
             (unsigned long) statp);
#endif /* DEBUG */

    /* translate message_id */
    part_size = (char*) &statp->command - (char*) &statp->request;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xmsg_id(xdrsp, &statp->request)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_reqstat()",  "csi_xmsg_id()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate command */
    part_size = (char*) &statp->status - (char*) &statp->command;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xcommand(xdrsp, &statp->command)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_reqstat()",  "csi_xcommand()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_reqstat()",  "csi_xstatus()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;
    return(1);
}

/*
 * Name:
 *      st_xqu_xcap()
 *
 * Description:
 *      Serializes/Deserializes the "xcap_status" portion of a 
 *      "CSI_QUERY_RESPONSE" packet "status_response" from the point after the 
 *      identifier "count".  The upper contents of the packet/structure above 
 *      the variable "count" should have already been translated. 
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *      NONE            (list of any outputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 */

static bool_t 
st_xqu_xcap (
    XDR *xdrsp,               /* xdr handle structure */
    QU_CAP_STATUS *statp               /* cap status */
)
{
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xqu_xcap()", 2, (unsigned long)xdrsp,(unsigned long)statp);
#endif /* DEBUG */

    /* translate capid */
    part_size = (char*) &statp->status - (char*) &statp->cap_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xcap_id(xdrsp, &statp->cap_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_xcap()",  "csi_xcap_id()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_xcap()",  "csi_xstatus()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate priority */
    part_size = sizeof(CAP_PRIORITY);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_char(xdrsp, (char *) &statp->cap_priority)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_xcap()",  "xdr_char()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate cap_size */
    part_size = sizeof(unsigned short);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &statp->cap_size)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_xcap()",  "xdr_u_short()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate cap_state */
    part_size = sizeof(statp->cap_state);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstate(xdrsp, &statp->cap_state)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_xcap()",  "csi_xstate()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }

    /* translate cap_mode */
    part_size = sizeof(statp->cap_mode);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xcap_mode(xdrsp, &statp->cap_mode)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE,             "st_xqu_xcap()",  "csi_xcap_mode()", MMSG(928,            "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;
    return(1);
}

/*
 * Name:
 *      st_xqu_scratch()
 *
 * Description:
 *      Serializes/Deserializes the "scratch_status" portion of a 
 *      "CSI_QUERY_RESPONSE" packet "status_response" from the point after the 
 *      identifier "count".  The upper contents of the packet/structure above 
 *      the variable "count" should have already been translated. 
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *      NONE            (list of any outputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 */

static bool_t 
st_xqu_scratch (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_SCR_STATUS *statp                 /* scratch status */
)
{
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xqu_scratch()", 2, (unsigned long) xdrsp,
             (unsigned long) statp);
#endif /* DEBUG */

    /* translate vol_id */
    part_size = (char*) &statp->media_type - (char*) &statp->vol_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xvol_id(xdrsp, &statp->vol_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_scratch()",
		"csi_xvol_id()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate media type */
    part_size = (char*) &statp->home_location - (char*) &statp->media_type;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    if (badsize || !csi_xmedia_type(xdrsp, &statp->media_type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_scratch()",
		"csi_xmedia_type()", MMSG(928,
					"XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate home_location */
    part_size = (char*) &statp->pool_id - (char*) &statp->home_location;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xcell_id(xdrsp, &statp->home_location)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_scratch()",
	       "csi_xcell_id()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate pool_id */
    part_size = (char*) &statp->status - (char*) &statp->pool_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xpool_id(xdrsp, &statp->pool_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_scratch()",
	       "csi_xpool_id()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_scratch()", 
		"csi_xstatus()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    return(1);
}

/*
 * Name:
 *      st_xqu_pool()
 *
 * Description:
 *      Serializes/Deserializes the "pool_status" portion of a 
 *      "CSI_QUERY_RESPONSE" packet "status_response" from the point after the 
 *      identifier "count".  The upper contents of the packet/structure above 
 *      the variable "count" should have already been translated. 
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *      NONE            (list of any outputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 */

static bool_t 
st_xqu_pool (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_POL_STATUS *statp                 /* pool status */
)
{
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xqu_pool()", 2, (unsigned long) xdrsp,
             (unsigned long) statp);
#endif /* DEBUG */

    /* translate pool_id */
    part_size = (char*) &statp->volume_count - (char*) &statp->pool_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xpool_id(xdrsp, &statp->pool_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_pool()", "csi_xpool_id()",
		MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate volume_count */
    part_size = (char*) &statp->low_water_mark - (char*) &statp->volume_count;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_long(xdrsp, &statp->volume_count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_pool()", "xdr_u_long()",
		MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate low_water_mark */
    part_size = (char*)&statp->high_water_mark - (char*)&statp->low_water_mark;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_long(xdrsp, &statp->low_water_mark)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_pool()", "xdr_u_long()", 
		MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate high_water_mark */
    part_size = (char*)&statp->pool_attributes - (char*)&statp->high_water_mark;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_long(xdrsp, &statp->high_water_mark)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_pool()", "xdr_u_long()", 
		MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate pool_attributes */
    part_size = (char*)&statp->status - (char*)&statp->pool_attributes;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_long(xdrsp, &statp->pool_attributes)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_pool()", "xdr_u_long()", 
		MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_pool()", "csi_xstatus()", 
		MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;
    return(1);
}

/*
 * Name:
 *      st_xqu_mount_scratch()
 *
 * Description:
 *      Serializes/Deserializes the "mount_scratch_status" portion of a 
 *      "CSI_QUERY_RESPONSE" packet "status_response" from the point after the 
 *      identifier "count".  The upper contents of the packet/structure above 
 *      the variable "count" should have already been translated. 
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *      NONE            (list of any outputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 */

static bool_t 
st_xqu_mount_scratch (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_MSC_STATUS *statp                 /* mount_scratch status */
)
{
    register int        drives;         /* counter for number of drives */
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xqu_mount_scratch()", 2, (unsigned long)xdrsp,
             (unsigned long)statp);
#endif /* DEBUG */

    /* translate pool_id */
    part_size = (char*) &statp->status - (char*) &statp->pool_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xpool_id(xdrsp, &statp->pool_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mount_scratch()",
	       "csi_xpool_id()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = (char*) &statp->drive_count - (char*) &statp->status;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mount_scratch()",
		"csi_xstatus()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate drive_count */
    part_size = (char*) &statp->drive_list[0] - (char*) &statp->drive_count;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &statp->drive_count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mount_scratch()",
		"xdr_u_short()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* check boundary condition before loop */
    if (statp->drive_count > QU_MAX_DRV_STATUS) {
        MLOGCSI((STATUS_COUNT_TOO_LARGE, "st_xqu_mount_scratch()",
		CSI_NO_CALLEE, MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the drive_list's */
    for (drives = 0; drives < (int) statp->drive_count; drives++) {
        /* translate a drive_list */
        if (!st_xqu_drive(xdrsp, &statp->drive_list[drives])) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mount_scratch()",
		    "st_xqu_drive()", MMSG(928, 
					"XDR message translation failure")));
            return(0);
        }
    }
    return(1);
}

/*
 * Name:
 *      st_xqu_clean_volume()
 *
 * Description:
 *      Serializes/Deserializes the "clean_volume_status" portion of a 
 *      "CSI_QUERY_RESPONSE" packet "status_response" from the point after the 
 *      identifier "count".  The upper contents of the packet/structure above 
 *      the variable "count" should have already been translated. 
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *      NONE            (list of any outputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 */

static bool_t 
st_xqu_clean_volume (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_CLN_STATUS *statp                 /* clean_volume status */
)
{
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xqu_clean_volume()", 2, (unsigned long) xdrsp,
             (unsigned long) statp);
#endif /* DEBUG */

    /* translate vol_id */
    part_size = (char*) &statp->media_type - (char*) &statp->vol_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xvol_id(xdrsp, &statp->vol_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_clean_volume()",
		"csi_xvol_id()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate media type */
    part_size = (char*) &statp->home_location - (char*) &statp->media_type;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    if (badsize || !csi_xmedia_type(xdrsp, &statp->media_type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_clean_volume()",
		"csi_xmedia_type()", MMSG(928,
					"XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate home_location */
    part_size = (char*) &statp->max_use - (char*) &statp->home_location;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xcell_id(xdrsp, &statp->home_location)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_clean_volume()",
	       "csi_xcell_id()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate max_use */
    part_size = (char*)&statp->current_use - (char*)&statp->max_use;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &statp->max_use)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_clean_volume()",
		"xdr_u_short()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate current_use */
    part_size = (char*)&statp->status - (char*)&statp->current_use;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &statp->current_use)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_clean_volume()",
		"xdr_u_short()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_clean_volume()",
		"csi_xstatus()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    return(1);
}

/*
 * Name:
 *      st_xqu_mm_info_mt()
 *
 * Description:
 *      Serializes/Deserializes the "media_type_status" portion of a 
 *      "CSI_QUERY_RESPONSE" packet "status_response.mm_info_response" from the
 *	point after the identifier "media_type_count".  The upper contents of
 *	the packet/structure above the variable "media_type_count" should have
 *	already been translated. 
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *      NONE            (list of any outputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 */

static bool_t 
st_xqu_mm_info_mt (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_MEDIA_TYPE_STATUS *statp	/* media_type status */
)
{
	register int	part_size = 0;	/* size of part of the packet */
    	register int    i;              /* loop counter */
    	register int    count;          /* holds count value */
	BOOLEAN		badsize;	/* TRUE if packet size invalid */
	char		*sp;		/* xdr_string() requires a pointer
					   to a pointer */

    /* translate the media type */
    part_size = (char *) statp->media_type_name - (char *) &statp->media_type;
    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    if (badsize || !csi_xmedia_type(xdrsp, &statp->media_type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mm_info_mt()", 
		"csi_xmedia_type()", MMSG(928,
					"XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate the media type name */
    part_size = (char *) &statp->cleaning_cartridge - 
						(char*) statp->media_type_name;
    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    sp = statp->media_type_name;
    if (badsize || !xdr_string(xdrsp, &sp, MEDIA_TYPE_NAME_LEN + 1)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mm_info_mt()", 
		"xdr_string()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate the cleaning capabilities of the cartridge */
    part_size = (char *) &statp->max_cleaning_usage - 
					(char *) &statp->cleaning_cartridge;
    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    if (badsize || !xdr_enum(xdrsp, (enum_t *) &statp->cleaning_cartridge)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mm_info_mt()", 
		"xdr_enum()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate the maximum cleaning usage */
    part_size = (char *) &statp->compat_count - 
					(char *) &statp->max_cleaning_usage;
    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    if (badsize || !xdr_int(xdrsp, &statp->max_cleaning_usage)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mm_info_mt()", "xdr_int()",
		MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate the compatibility count */
    part_size = (char *) statp->compat_drive_types - 
						  (char *) &statp->compat_count;
    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    if (badsize || !xdr_u_short(xdrsp, &statp->compat_count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mm_info_mt()",
		"xdr_u_short()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate the compatible drive types */
    for (i = 0, count = statp->compat_count; i < count; i++ ) {

    	/* translate compat_media types part of packet */
        if (!csi_xdrive_type(xdrsp, &statp->compat_drive_types[i])) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mm_info_mt()",
		    "csi_xdrive_type()", MMSG(928,
					"XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += (char*)&statp->compat_drive_types[i+1] - 
					(char *) &statp->compat_drive_types[i];
    }

    return (1);
}


/*
 * Name:
 *      st_xqu_mm_info_dt()
 *
 * Description:
 *      Serializes/Deserializes the "drive_type_status" portion of a 
 *      "CSI_QUERY_RESPONSE" packet "status_response.mm_info_response" from the
 *	point after the identifier "drive_type_count".  The upper contents of
 *	the packet/structure above the variable "drive_type_count" should have
 *	already been translated. 
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *      NONE            (list of any outputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 */

static bool_t 
st_xqu_mm_info_dt (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_DRIVE_TYPE_STATUS *statp	/* drive_type status */
)
{
	register int	part_size = 0;	/* size of part of the packet */
    	register int    i;              /* loop counter */
    	register int    count;          /* holds count value */
	BOOLEAN		badsize;	/* TRUE if packet size invalid */
	char		*sp;		/* xdr_string() requires a pointer
					   to a pointer */

    /* translate the drive type */
    part_size = (char *) statp->drive_type_name - (char *) &statp->drive_type;
    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    if (badsize || !csi_xdrive_type(xdrsp, &statp->drive_type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mm_info_dt()",
		"csi_xdrive_type()", MMSG(928,
					"XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate the drive type name */
    part_size = (char *) &statp->compat_count - (char*) statp->drive_type_name;
    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    sp = statp->drive_type_name;
    if (badsize || !xdr_string(xdrsp, &sp, DRIVE_TYPE_NAME_LEN + 1)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mm_info_dt()",
		"xdr_string()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate the compatibility count */
    part_size = (char *) statp->compat_media_types - 
						  (char *) &statp->compat_count;
    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    if (badsize || !xdr_u_short(xdrsp, &statp->compat_count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mm_info_dt()", 
		"xdr_u_short()", MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate the compatible media types */

    for (i = 0, count = statp->compat_count; i < count; i++ ) {

    	/* translate compat_media types part of packet */
        if (!csi_xmedia_type(xdrsp, &statp->compat_media_types[i])) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mm_info_dt()",
		     "csi_xmedia_type()", MMSG(928,
					"XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += (char*)&statp->compat_media_types[i+1] - 
					(char *) &statp->compat_media_types[i];
    }

    return (1);
}
/*
 * Name:
 *      st_xqu_drv_grp()
 *
 * Description:
 *      Serializes/Deserializes the "virt_drv_map" portion of a
 *      "CSI_QUERY_RESPONSE" packet "status_response" from the point after the
 *      identifier "vir_drv_map_count".  The upper contents of the packet/structure above
 *      the variable "vir_drv_map_count" should have already been translated.
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 *
 * Note: This is a new internal routine created in NCS 4.0.
 */

static bool_t
st_xqu_drv_grp (
    XDR *xdrsp,                 /* xdr handle structure */
    QU_VIRT_DRV_MAP *statp     /* virtual drive map pointer */
)
{
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
	cl_trace("st_xqu_drv_grp()", 2, (unsigned long)xdrsp,
		(unsigned long)statp);
#endif /* DEBUG */

    /* translate drive_id */
    part_size = (char*) &statp->drive_addr - (char*) &statp->drive_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xdrive_id(xdrsp, &statp->drive_id)) {
	MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_drv_grp()",
		"csi_xdrive_id()", 
		MMSG(928, "XDR message translation failure")));
	return(0);
    }

    csi_xcur_size += part_size;

    /* translate drive unit address */

    part_size = sizeof(statp->drive_addr);
    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &statp->drive_addr)) {
	MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_drv_grp()", 
		"xdu_u_short()", 
		MMSG(928, "XDR message translation failure")));
	return(0);
    }
    csi_xcur_size += part_size;
    return(1);
}

/*
 * Name:
 *      st_xqu_spn()
 *
 * Description:
 *      Serializes/Deserializes the "subpl_name_status" portion of a
 *      "CSI_QUERY_RESPONSE" packet "status_response" from the point after the
 *      identifier "count".  The upper contents of the packet/structure above
 *      the variable "count" should have already been translated.
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the
 *      valid portion of this part of this packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE            (list of any inputs not specified in argument list)
 *
 * Implicit Outputs:
 *      NONE            (list of any outputs not specified in argument list)
 *
 * Considerations:
 *      NONE
 *
 * Note:  This is a new internal routine created in NCS 4.0.
 */

static bool_t
st_xqu_spn (
    XDR *xdrsp,                       /* xdr handle structure */
    QU_SUBPOOL_NAME_STATUS *statp      /* subpool name status */
)
{
    register int        part_size = 0;  /* size of part of a packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */
    char                *sp;            /* ptr to ptr for xdr_string() */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
	cl_trace("st_xqu_spn()", 2, (unsigned long) xdrsp,
		 (unsigned long) statp);
#endif /* DEBUG */

    /* translate subpool_name */
    part_size = (char*) &statp->pool_id - (char*) &statp->subpool_name;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    sp = (char *)&statp->subpool_name;
    if (badsize || !xdr_string(xdrsp,(char **)&sp,SUBPOOL_NAME_SIZE+1)) {
	MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_spn()", "xdr_string()",
	        MMSG(928, "XDR message translation failure")));
	return(0);
    }
    csi_xcur_size += part_size;

    /* translate pool_id */
    part_size = (char*) &statp->status - (char*) &statp->pool_id;
    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);

    if (badsize || !csi_xpool_id(xdrsp, &statp->pool_id)) {
	MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_spn()", "csi_xpool_id()", 
	        MMSG(928, "XDR message translation failure")));
	return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
	MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_spn()", "csi_xstatus()",
	        MMSG(928, "XDR message translation failure")));
	return(0);
    }
    csi_xcur_size += part_size;
    return(1);
}

