#ifndef lint
static char SccsId[] = "@(#)csi_xv1qures.c	5.5 11/9/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_xv1quresponse()
 *
 * Description:
 *      Note:  This routine must only be considered within the context of
 *      csi_xdrresponse().  Please see documentation there before proceeding.
 *
 *      Routine serializes/deserializes a CSI_V1_QUERY_RESPONSE structure 
 *      from the point beginning with the type (type TYPE).  The
 *      data above that point in the CSI_V1_QUERY_RESPONSE structure
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
 *                          to csi_xlsm() to csi_xlsm_id().
 *
 *      J. A. Wishner       27-Jul-1989.    Bugfix.  Invalid location type
 *                          is no longer translated.  Packet is an error and
 *                          immediately truncated.
 *
 *      D. F. Reed          26-Oct-1989.    Modify TYPE_MOUNT interpretation
 *                          to accommodate drive_status instead of drive_id.
 *      J. W. Montgomery    27-Jul-1990.    Version 2.
 *
 *      J. A. Wishner       08-Oct-1991.    Complete mods release 3 (version 2).
 *                          Changed calls to csi_xcap_id() to csi_xv1_cap_id().
 *                          Deleted count, i, drives; unused.
 *      E. A. Alongi        30-Aug-1992     Deleted useless test for EXTENDED
 *                                          bit and corresponding else clause.
 *	E. A. Alongi	    17-Sep-1993.    Added V1 prefix to MAX_ACS_DRIVES in
 *					    st_xqu_mount() and 
 *					    st_xqu_mount_scratch.
 *	E. A. Alongi	    20-Oct-1993.    Eliminated function st_xqu_cap() -
 *			    this routine was no longer referenced.  Changed
 *			    status_response.ext_cap_status to
 *			    status_response.cap_status.  A change to the
 *			    corresponding union structure was made in 
 *			    csi_v1_structs.h
 *	S. L. Siao	    20-Mar-2002     Added type cast to arguments of xdr 
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
static char     *st_src = __FILE__;
static char     *st_module = "csi_xv1quresponse()";

/*      Static Prototype Declarations: */
static bool_t st_xqu_server (XDR *xdrsp, QU_SRV_STATUS *statp);
static bool_t st_xqu_acs (XDR *xdrsp, QU_ACS_STATUS *statp);
static bool_t st_xqu_lsm (XDR *xdrsp, QU_LSM_STATUS *statp);
static bool_t st_xqu_drive (XDR *xdrsp, V1_QU_DRV_STATUS *statp);
static bool_t st_xqu_mount (XDR *xdrsp, V1_QU_MNT_STATUS *statp);
static bool_t st_xqu_volume (XDR *xdrsp, V1_QU_VOL_STATUS *statp);
static bool_t st_xqu_port (XDR *xdrsp, QU_PRT_STATUS *statp);
static bool_t st_xqu_reqstat (XDR *xdrsp, QU_REQ_STATUS *statp);
static bool_t st_xqu_xcap (XDR *xdrsp, V1_QU_CAP_STATUS *statp);
static bool_t st_xqu_scratch (XDR *xdrsp, V1_QU_SCR_STATUS *statp);
static bool_t st_xqu_pool (XDR *xdrsp, QU_POL_STATUS *statp);
static bool_t st_xqu_mount_scratch (XDR *xdrsp, V1_QU_MSC_STATUS *statp);
static bool_t st_xqu_clean_volume (XDR *xdrsp, V1_QU_CLN_STATUS *statp);

/*      Procedure Type Declarations: */

bool_t 
csi_xv1quresponse (
    XDR *xdrsp,              /* xdr handle structure */
    CSI_V1_QUERY_RESPONSE *resp            /* vary response structure */
)
{
    register int        i;                      /* loop counter */
    register int        count;                  /* holds count value */
    register int        part_size = 0;          /* size this part of packet */
    register int        total_size = 0;         /* size of packet */
    BOOLEAN             badsize;                /* TRUE if packet size bogus */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module, 2, (unsigned long) xdrsp, (unsigned long) resp);
#endif /* DEBUG */

    /* translate type */
    part_size = (char*) &resp->count - (char*) &resp->type;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xtype(xdrsp, &resp->type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xtype()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate count */
    part_size = (char*) &resp->status_response - (char*) &resp->count;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &resp->count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* check boundary condition before loop */
    if (resp->count > MAX_ID) {
        MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    total_size = csi_xcur_size;
    /* translate by status type */
    for (i = 0, count = resp->count; i < count; i++ ) {
        switch (resp->type) {
         case TYPE_SERVER:
            /* translate rest of packet as server query status_response */
            if (!st_xqu_server(xdrsp, &resp->status_response.server_status[i])){
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "xqu_server()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->status_response.
                server_status[resp->count] - (char *)resp;
            break;
         case TYPE_ACS:
            /* translate rest of the packet as acs query status_response */
            if (!st_xqu_acs(xdrsp, &resp->status_response.acs_status[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xqu_acs()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->status_response.
                acs_status[resp->count] - (char *)resp;
            break;
         case TYPE_LSM:
            /* translate rest of the packet as lsm query status_response */
            if (!st_xqu_lsm(xdrsp, &resp->status_response.lsm_status[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "st_xqu_lsm()",
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->status_response.
                lsm_status[resp->count] - (char *)resp;
            break;
         case TYPE_CAP:
            /* translate rest of the packet as xcapid status_response */
            if (!st_xqu_xcap(xdrsp, &resp->status_response.cap_status[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xqu_xcap()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->status_response.
                    cap_status[resp->count] - (char *)resp;
            break;
         case TYPE_DRIVE:
            /* translate rest of the packet as drive query status_response*/
            if (!st_xqu_drive(xdrsp, &resp->status_response.drive_status[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xqu_drive()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->status_response.
                drive_status[resp->count] - (char *)resp;
            break;
         case TYPE_VOLUME:
            /* translate rest of packet as volume query status_response */
            if (!st_xqu_volume(xdrsp, &resp->status_response.volume_status[i])){
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xqu_volume()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->status_response.
                volume_status[resp->count] - (char *)resp;
            break;
         case TYPE_MOUNT:
            /* translate rest of the packet as mount query status_response*/
            if (!st_xqu_mount(xdrsp, &resp->status_response.mount_status[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xqu_mount()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->status_response.
                mount_status[resp->count] - (char *)resp;
            break;
          case TYPE_PORT:
            /* translate rest of packet as port query status_response */
            if (!st_xqu_port(xdrsp, &resp->status_response.port_status[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xqu_port()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->status_response.
                port_status[resp->count] - (char *)resp;
            break;
         case TYPE_REQUEST:
            /* translate rest of packet as request query status_response */
            if (!st_xqu_reqstat(xdrsp,
                &resp->status_response.request_status[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xqu_reqstat()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->status_response.
                request_status[resp->count] - (char *)resp;
            break;
         case TYPE_SCRATCH:
            /* translate rest of packet as scratch query status_response */
            if (!st_xqu_scratch(xdrsp,
                &resp->status_response.scratch_status[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xqu_scratch()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->status_response.
                scratch_status[resp->count] - (char *)resp;
            break;
         case TYPE_POOL:
            /* translate rest of packet as pool query status_response */
            if (!st_xqu_pool(xdrsp,
                &resp->status_response.pool_status[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xqu_pool()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->status_response.
                pool_status[resp->count] - (char *)resp;
            break;
         case TYPE_MOUNT_SCRATCH:
            /* translate rest of packet as mount_scratch query status_response */
            if (!st_xqu_mount_scratch(xdrsp,
                &resp->status_response.mount_scratch_status[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xqu_mount_scratch()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->status_response.
                mount_scratch_status[resp->count] - (char *)resp;
            break;
         case TYPE_CLEAN:
            /* translate rest of packet as clean query status_response */
            if (!st_xqu_clean_volume(xdrsp,
                &resp->status_response.clean_volume_status[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xqu_clean_volume()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->status_response.
                clean_volume_status[resp->count] - (char *)resp;
            break;
         default:
            /* If got here have an illegal type */
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
    }
    csi_xcur_size = total_size;
    return(1);
}


/*
 * Name:
 *      st_xqu_server
 *
 * Description:
 *      Serializes/Deserializes the server_status portion of a 
 *      CSI_V1_QUERY_RESPONSE packet "status_response" from the point after 
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
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_server()", "csi_xstate()",
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate freecells */
    part_size = (char*) &statp->requests - (char*) &statp->freecells;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xfreecells(xdrsp, &statp->freecells)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_server()",
	  "csi_xfreecells()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate requests (type REQ_SUMMARY) */
    part_size = sizeof(statp->requests);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xreqsummary(xdrsp, &statp->requests)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_server()",
	  "csi_xreqsummary()", 
	  MMSG(928, "XDR message translation failure")));
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
 *      CSI_V1_QUERY_RESPONSE packet "status_response" from the point after 
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
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_acs()", "csi_xacs()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate state */
    part_size = (char*) &statp->freecells - (char*) &statp->state;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstate(xdrsp, &statp->state)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_acs()", "csi_xstate()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate freecells */
    part_size = (char*) &statp->requests - (char*) &statp->freecells;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xfreecells(xdrsp, &statp->freecells)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_acs()",
	  "csi_xfreecells()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate requests (type REQ_SUMMARY) */
    part_size = (char*) &statp->status - (char*) &statp->requests;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xreqsummary(xdrsp, &statp->requests)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_acs()",
	  "csi_xreqsummary()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_acs()", "csi_xstatus()", 
	  MMSG(928, "XDR message translation failure")));
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
 *      "CSI_V1_QUERY_RESPONSE" packet "status_response" after the 
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
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_lsm()", "csi_xlsm()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate state */
    part_size = (char*) &statp->freecells - (char*) &statp->state;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstate(xdrsp, &statp->state)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_lsm()", "csi_xstate()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate freecells */
    part_size = (char*) &statp->requests - (char*) &statp->freecells;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xfreecells(xdrsp, &statp->freecells)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_lsm()",
	  "csi_xfreecells()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate requests (type REQ_SUMMARY) */
    part_size = (char*) &statp->status - (char*) &statp->requests;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xreqsummary(xdrsp, &statp->requests)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_lsm()",
	  "csi_xreqsummary()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(statp->status);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_lsm", "csi_xstatus()", 
	  MMSG(928, "XDR message translation failure")));
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
 *      "CSI_V1_QUERY_RESPONSE" packet "status_response" after the 
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
    V1_QU_DRV_STATUS *statp                 /* drive status */
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
    part_size = (char*) &statp->state - (char*) &statp->drive_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xdrive_id(xdrsp, &statp->drive_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_drive()",
	  "csi_xdrive_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate state */
    part_size = (char*) &statp->vol_id - (char*) &statp->state;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstate(xdrsp, &statp->state)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_drive()", "csi_xstate()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate volid */
    part_size = (char*) &statp->status - (char*) &statp->vol_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xvol_id(xdrsp, &statp->vol_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_drive()",
	  "csi_xvol_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(statp->status);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_drive()",
	  "csi_xstatus()", 
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
 *      "CSI_V1_QUERY_RESPONSE" packet "status_response" point after the 
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
    V1_QU_MNT_STATUS *statp                 /* mount status */
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
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mount()",
	  "csi_xvol_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = (char*) &statp->drive_count - (char*) &statp->status;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mount()",
	  "csi_xstatus()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate drive_count */
    part_size = (char*) &statp->drive_status[0] - (char*) &statp->drive_count;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &statp->drive_count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mount()",
	  "xdr_u_short()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* check boundary condition before loop */
    if (statp->drive_count > V1_MAX_ACS_DRIVES) {
        MLOGCSI((STATUS_COUNT_TOO_LARGE, "st_xqu_mount()",
	  CSI_NO_CALLEE, 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the drive_status's */
    for (drives = 0; drives < V1_MAX_ACS_DRIVES; drives++) {
        /* translate a drive_status */
        if (!st_xqu_drive(xdrsp, &statp->drive_status[drives])) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mount()",
	      "st_xqu_drive()", 
	      MMSG(928, "XDR message translation failure")));
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
 *      "CSI_V1_QUERY_RESPONSE" packet "status_response" after the 
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
    V1_QU_VOL_STATUS *statp                 /* volume status */
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
    part_size = (char*) &statp->location_type - (char*) &statp->vol_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xvol_id(xdrsp, &statp->vol_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_volume()",
	  "csi_xvol_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate location_type */
    part_size = (char*) &statp->location - (char*) &statp->location_type;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xlocation(xdrsp, &statp->location_type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_volume()",
	  "csi_xlocation()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate one of two location types */
    if (LOCATION_DRIVE == statp->location_type) {
        /* translate the drive_id */
        part_size = (char*) &statp->status - (char*) &statp->location.drive_id;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !csi_xdrive_id(xdrsp, &statp->location.drive_id)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_volume()",
	      "csi_xdrive_id()", 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += part_size;
    }
    else if (LOCATION_CELL == statp->location_type) {
        /* translate the cell_id */
        part_size = (char*) &statp->status - (char*) &statp->location.cell_id;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !csi_xcell_id(xdrsp, &statp->location.cell_id)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_volume()",
	      "csi_xcell_id()", 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += part_size;
    }
    else {
        /* invalid location type */
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_volume()",
	  CSI_NO_CALLEE, 
	  MMSG(976, "Invalid location type")));
        return(0);
    }

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_volume()",
	  "csi_xstatus()", 
	  MMSG(928, "XDR message translation failure")));
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
 *      "CSI_V1_QUERY_RESPONSE" packet "status_response" after the 
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
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_port()",
	  "csi_xport_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate state */
    part_size = (char*) &statp->status - (char*) &statp->state;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstate(xdrsp, &statp->state)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_port()", "csi_xstate()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_port()", "csi_xstatus()", 
	  MMSG(928, "XDR message translation failure")));
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
 *      "CSI_V1_QUERY_RESPONSE" packet "status_response" after the 
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
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_reqstat()",
	  "csi_xmsg_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate command */
    part_size = (char*) &statp->status - (char*) &statp->command;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xcommand(xdrsp, &statp->command)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_reqstat()",
	  "csi_xcommand()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_reqstat()",
	  "csi_xstatus()", 
	  MMSG(928, "XDR message translation failure")));
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
 *      "CSI_V1_QUERY_RESPONSE" packet "status_response" after the 
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
    V1_QU_CAP_STATUS *statp               /* cap status */
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
    if (badsize || !csi_xv1_cap_id(xdrsp, &statp->cap_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_xcap()",
	  "csi_xv1_cap_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_xcap()", "csi_xstatus()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate priority */
    part_size = sizeof(CAP_PRIORITY);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_char(xdrsp, (char *) &statp->cap_priority)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_xcap()", "xdr_char()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate cap_size */
    part_size = sizeof(unsigned short);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &statp->cap_size)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_xcap()", "xdr_u_short()", 
	  MMSG(928, "XDR message translation failure")));
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
 *      "CSI_V1_QUERY_RESPONSE" packet "status_response" after the 
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
    V1_QU_SCR_STATUS *statp                 /* scratch status */
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
    part_size = (char*) &statp->home_location - (char*) &statp->vol_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xvol_id(xdrsp, &statp->vol_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_scratch()",
	  "csi_xvol_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate home_location */
    part_size = (char*) &statp->pool_id - (char*) &statp->home_location;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xcell_id(xdrsp, &statp->home_location)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_scratch()",
	  "csi_xcell_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate pool_id */
    part_size = (char*) &statp->status - (char*) &statp->pool_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xpool_id(xdrsp, &statp->pool_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_scratch()",
	  "csi_xpool_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_scratch()",
	  "csi_xstatus()", 
	  MMSG(928, "XDR message translation failure")));
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
 *      "CSI_V1_QUERY_RESPONSE" packet "status_response" after the 
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
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_pool()",
	  "csi_xpool_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate volume_count */
    part_size = (char*) &statp->low_water_mark - (char*) &statp->volume_count;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_long(xdrsp, &statp->volume_count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_pool()",
	  "xdr_u_long()", 
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
 *      "CSI_V1_QUERY_RESPONSE" packet "status_response" after the 
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
    V1_QU_MSC_STATUS *statp                 /* mount_scratch status */
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
	  "csi_xpool_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = (char*) &statp->drive_count - (char*) &statp->status;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mount_scratch()",
	  "csi_xstatus()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate drive_count */
    part_size = (char*) &statp->drive_list[0] - (char*) &statp->drive_count;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &statp->drive_count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mount_scratch()",
	  "xdr_u_short()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* check boundary condition before loop */
    if (statp->drive_count > V1_MAX_ACS_DRIVES) {
        MLOGCSI((STATUS_COUNT_TOO_LARGE, "st_xqu_mount_scratch()",
	  CSI_NO_CALLEE, 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    /* translate the drive_list's */
    for (drives = 0; drives < V1_MAX_ACS_DRIVES; drives++) {
        /* translate a drive_list */
        if (!st_xqu_drive(xdrsp, &statp->drive_list[drives])) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_mount_scratch()",
	      "st_xqu_drive()", 
	      MMSG(928, "XDR message translation failure")));
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
 *      "CSI_V1_QUERY_RESPONSE" packet "status_response" after the 
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
    V1_QU_CLN_STATUS *statp                 /* clean_volume status */
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
    part_size = (char*) &statp->home_location - (char*) &statp->vol_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xvol_id(xdrsp, &statp->vol_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_clean_volume()",
	  "csi_xvol_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate home_location */
    part_size = (char*) &statp->max_use - (char*) &statp->home_location;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xcell_id(xdrsp, &statp->home_location)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_clean_volume()",
	  "csi_xcell_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate max_use */
    part_size = (char*)&statp->current_use - (char*)&statp->max_use;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &statp->max_use)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_clean_volume()",
	  "xdr_u_short()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate current_use */
    part_size = (char*)&statp->status - (char*)&statp->current_use;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &statp->current_use)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_clean_volume()",
	  "xdr_u_short()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate status */
    part_size = sizeof(STATUS);
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstatus(xdrsp, &statp->status)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_clean_volume()",
	  "csi_xstatus()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;
    return(1);
}
