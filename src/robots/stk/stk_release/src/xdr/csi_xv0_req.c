#ifndef lint
static char SccsId[] = "@(#)csi_xv0_req.c	5.5 11/11/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_xv0_req()
 *
 * Description:
 *      Encode/Decode Version 0 requests.
 *
 * Return Values:
 *      (bool_t)        1 - successful xdr conversion.
 *      (bool_t)        0 - xdr conversion failed.
 *
 * Implicit Inputs:
 *      NONE
 *
 * Implicit Outputs:
 *      bufferp->data            - data is placed here during deserialization.
 *      bufferp->size            - size of data put here during deserialization.
 *      bufferp->packet_status   - describes various translation errors
 *      bufferp->translated_size - bytes of data the could be translated
 *
 * Considerations:
 *      Portability Issues--
 *      Note that sizing of the translated data must be done using address
 *      arithmetic rather than pure sizeof(data-structure) since sizeof
 *      is not a portable construct, especially for having this routine
 *      translate across different machine architectures and different
 *      versions of an operating system.  Different compilers will product
 *      different alignment of data structures and their contents.
 *      One can only do a sizeof() on the last element in the packet.
 *
 *      The CSI_PAK_NETDATAP() macro is used to extract the location of the
 *      data in the data buffer since the data may not start at the first byte,
 *      if the IPC_HEADER ever becomes larger than the CSI_V0_HEADER.  The start
 *      of the data is always at bufferp->offset.
 *
 *      This routine may return a partial packet.  Return code will be (1)
 *      if at least a request header can be serialized/deserialized.
 *
 *      Variable "csi_xexp_size" (global int) must always be set near the top of
 *      this function since it is also used by static routines in this file.
 *      
 *
 * Module Test Plan:
 *      See CSI Unit Test Plan
 *
 * Revision History:
 *      J. A. Wishner       10-Jan-1989.    Created.
 *      J. A. Wishner       10-Aug-1989.    Portabilit.  Changed size
 *                                          calculations for data structures
 *                                          so portable to other machines, such
 *                                          as SPARC.
 *      J. W. Montgomery    15-Jun-1990.    Release 2 (version 1).
 *      J. A. Wishner       03-Oct-1991.    Complete mods release 3 (version 2).
 *	Emanuel A. Alongi   08-Sep-1993.    Correction: under COMMAND_ENTER,
 *					    part size check now correctly uses 
 *					    V0_CAPID.  Was CAPID, which is 
 *					    used in V2 packets and later.
 *	Mitch Black	07-Dec-2004	Prevent Linux warnings by putting in 
 *					correct declarations for static functions
 *					before they are called.
 */

/*      Header Files: */
#include "csi.h"
#include <csi_xdr_xlate.h> 
#include "ml_pub.h"

/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static char     *st_src = __FILE__;
static char     *st_module = "csi_xv0_req()";

/*      Procedure Type Declarations: */
static bool_t st_xau_request (XDR *, CSI_V0_AUDIT_REQUEST *);
static bool_t st_xqu_request (XDR *, CSI_V0_QUERY_REQUEST *);
static bool_t st_xva_request (XDR *, CSI_V0_VARY_REQUEST *);

bool_t 
csi_xv0_req (
    XDR *xdrsp,                  /* XDR handle */
    CSI_V0_REQUEST *reqp
)
{
    register int    i;
    register int    count;
    register int    part_size = 0;
    BOOLEAN         badsize;

    switch (reqp->csi_req_header.message_header.command) {

        case COMMAND_AUDIT:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_audit_req.cap_id 
                - (char*) &reqp->csi_audit_req.csi_request_header;
            /* translate the rest of the audit packet */
            if (!st_xau_request(xdrsp, &reqp->csi_audit_req)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xau_request()", 
		   MMSG(928, "XDR message translation failure")));
                return 0;
            }
            break;
        case COMMAND_CANCEL:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_cancel_req.request 
                - (char*) &reqp->csi_cancel_req.csi_request_header;
            /* translate starting at "request" (type MESSAGE_ID) */
            part_size = sizeof(MESSAGE_ID);
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xmsg_id(xdrsp, &reqp->csi_cancel_req.request)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xmsg_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            /* re-calc size in case there is compiler padding on last element */
            csi_xcur_size = sizeof(CSI_V0_CANCEL_REQUEST);
            break;
        case COMMAND_DISMOUNT:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_dismount_req.vol_id 
                - (char*) &reqp->csi_dismount_req.csi_request_header;
            /* translate starting at volid */
            part_size = (char*) &reqp->csi_dismount_req.drive_id
                - (char*) &reqp->csi_dismount_req.vol_id;
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xvol_id(xdrsp, &reqp->csi_dismount_req.vol_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xvol_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;
            /* translate starting at driveid */
            part_size = sizeof(DRIVEID);
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xdrive_id(xdrsp, &reqp->csi_dismount_req.drive_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xdrive_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            /* re-calc size in case there is compiler padding on last element */
            csi_xcur_size = sizeof(CSI_V0_DISMOUNT_REQUEST);
            break;
        case COMMAND_EJECT:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_eject_req.cap_id 
                - (char*) &reqp->csi_eject_req.csi_request_header;
            /* translate starting at cap_id */
            part_size = (char*) &reqp->csi_eject_req.count
                - (char*) &reqp->csi_eject_req.cap_id;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xv0_cap_id(xdrsp,&reqp->csi_eject_req.cap_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xv0_cap_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }  
            csi_xcur_size += part_size;
            /* translate count */
            part_size = (char*) &reqp->csi_eject_req.vol_id[0]
                - (char *) &reqp->csi_eject_req.count;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !xdr_u_short(xdrsp, &reqp->csi_eject_req.count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "xdr_u_short()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;
            /* check boundary condition before loop */
            if (reqp->csi_eject_req.count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            /* translate array of volid's */
            for (i=0, count = reqp->csi_eject_req.count; i < count; i++) {
                /* translate starting at volid */
                part_size = (char*) &reqp->csi_eject_req.vol_id[i+1]
                    - (char *) &reqp->csi_eject_req.vol_id[i];
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || 
                    !csi_xvol_id(xdrsp, &reqp->csi_eject_req.vol_id[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xvol_id()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                csi_xcur_size += part_size;
            }
            /* re-calc size in case there is compiler padding on last element */
            csi_xcur_size = (char*) &reqp->csi_eject_req.
                vol_id[reqp->csi_eject_req.count] - (char *) reqp;
            break;
        case COMMAND_ENTER:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_enter_req.cap_id 
                - (char*) &reqp->csi_enter_req.csi_request_header;
            /* translate starting at capid */
            part_size = sizeof(V0_CAPID);
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xv0_cap_id(xdrsp,&reqp->csi_enter_req.cap_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xv0_cap_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            /* re-calc size in case there is compiler padding on last element */
            csi_xcur_size = sizeof(CSI_V0_ENTER_REQUEST);
            break;
        case COMMAND_IDLE:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = sizeof(CSI_V0_IDLE_REQUEST);
            /* Idle is only a request header packet, no further translation. */
            break;
        case COMMAND_MOUNT:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_mount_req.vol_id 
                - (char*) &reqp->csi_mount_req.csi_request_header;
            /* translate starting at volid */
            part_size = (char*) &reqp->csi_mount_req.count
                - (char*) &reqp->csi_mount_req.vol_id;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xvol_id(xdrsp, &reqp->csi_mount_req.vol_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xvol_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;
            /* translate count */
            part_size = (char*) &reqp->csi_mount_req.drive_id[0] 
                - (char*) &reqp->csi_mount_req.count;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !xdr_u_short(xdrsp, &reqp->csi_mount_req.count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "xdr_u_short()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;
            /* check boundary condition before loop */
            if (reqp->csi_mount_req.count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            /* loop on the drive-id translating */
            for (i = 0, count = reqp->csi_mount_req.count; i < count; i++ ) {
                /* translate starting at driveid */
                part_size = (char*) &reqp->csi_mount_req.drive_id[i+1]
                    - (char*) &reqp->csi_mount_req.drive_id[i];
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || 
                    !csi_xdrive_id(xdrsp, &reqp->csi_mount_req.drive_id[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xdrive_id()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                csi_xcur_size += part_size;
            }
            /* re-calc size in case there is compiler padding on last element */
            csi_xcur_size = (char*) &reqp->csi_mount_req.
                drive_id[reqp->csi_mount_req.count] - (char *) reqp;
            break;
        case COMMAND_QUERY:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_query_req.type 
                - (char*) &reqp->csi_query_req.csi_request_header;
            /* translate the rest of the query packet */
            if (!st_xqu_request(xdrsp, &reqp->csi_query_req)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xqu_request()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            break;
        case COMMAND_START:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = sizeof(CSI_V0_START_REQUEST);
            /* Start is only a request header packet, no further translation */
            break;
        case COMMAND_VARY:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_vary_req.state 
                - (char*) &reqp->csi_vary_req.csi_request_header;
            /* translate the rest of the vary packet */
            if (!st_xva_request(xdrsp, &reqp->csi_vary_req)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xva_request()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            break;
        default:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = sizeof(CSI_V0_REQUEST_HEADER);
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
	      MMSG(975, "Invalid command")));
            return 0;

    } /* end of switch on command */

    return 1;
}

/*
 *
 * Name:
 *
 *      st_xau_request()
 *
 * Description:
 *      serializes/deserializes a partly translated CSI_V0_AUDIT_REQUEST 
 *      structure from the position of the CAPID, onward, downstream.  
 *
 *      The current packet size "csi_xcur_size" is increased by the size of the 
 *      valid portion of this part of the packet.
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *      NONE
 *
 * Implicit Outputs:
 *      (int) "csi_xcur_size" - increased by translated size this part of packet
 *
 * Considerations:
 *      Translation must begin at CAPID structure since this packet is 
 *      received in this routine already partly translated.  
 */

static bool_t 
st_xau_request (
    XDR *xdrsp,    /* xdr handle structure */
    CSI_V0_AUDIT_REQUEST *reqp     /* partly translated audit request */
)
{
    register int        i;              /* loop counter */
    register int        count;          /* holds count value */
    register int        part_size;      /* size of a portion of the packet */
    register int        total_size;     /* size of the packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xau_request", 2, (unsigned long) xdrsp,
                 (unsigned long) reqp);
#endif /* DEBUG */

    /* translate cap_id */
    part_size = (char*) &reqp->type - (char*) &reqp->cap_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xv0_cap_id(xdrsp, &reqp->cap_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_request",
	  "csi_xv0_cap_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }  
    csi_xcur_size += part_size;

    /* translate type */
    part_size = (char*) &reqp->count - (char*) &reqp->type;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xtype(xdrsp, &reqp->type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_request", "csi_xtype()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate count */
    part_size = (char*) &reqp->identifier - (char*) &reqp->count;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &reqp->count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_request", "xdr_u_short()",
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* check boundary condition before loop */
    if (reqp->count > MAX_ID) {
        MLOGCSI((STATUS_COUNT_TOO_LARGE, "st_xau_request", CSI_NO_CALLEE, 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    total_size = csi_xcur_size;
    for (i = 0, count = reqp->count; i < count; i++ ) {
        switch (reqp->type) {
         case TYPE_ACS:
            part_size = (char*) &reqp->identifier.acs_id[i+1]
                - (char*) &reqp->identifier.acs_id[i];
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xacs(xdrsp, &reqp->identifier.acs_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_request",
		  "csi_xacs()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.acs_id[reqp->count]
                - (char*) reqp;
            break;
         case TYPE_LSM:
            part_size = (char*) &reqp->identifier.lsm_id[i+1]
                - (char*) &reqp->identifier.lsm_id[i];
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xlsm_id(xdrsp, &reqp->identifier.lsm_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_request", "csi_xlsm_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.lsm_id[reqp->count]
                - (char*) reqp;
            break;
         case TYPE_PANEL:
            part_size = (char*) &reqp->identifier.panel_id[i+1]
                - (char*) &reqp->identifier.panel_id[i];
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xpnl_id(xdrsp, &reqp->identifier.panel_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_request", "csi_xlsm_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.panel_id[reqp->count]
                - (char*) reqp;
            break;
         case TYPE_SUBPANEL:
            part_size = (char*) &reqp->identifier.subpanel_id[i+1]
                - (char*) &reqp->identifier.subpanel_id[i];
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xspnl_id(xdrsp, &reqp->identifier.subpanel_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_request", "csi_xspnl_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.subpanel_id[reqp->count]
                - (char*) reqp;
            break;
         default:
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_request", CSI_NO_CALLEE, 
	      MMSG(977, "Invalid type")));
            return(0);
        }
    }
    /* re-calc size in case there is compiler padding on last element */
    csi_xcur_size = total_size;
    return(1);
}

/*
 * Name:
 *      st_xqu_request()
 *
 * Description:
 *      Routine serializes/deserializes a CSI_V0_QUERY_REQUEST structure 
 *      from the point beginning with the type (type TYPE).  The
 *      data above that point in the CSI_V0_QUERY_REQUEST structure
 *      should have already been translated.  
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
 *      (int) "csi_xcur_size" - increased by translated size this part of packet
 *
 * Considerations:
 *      Translation must start at type (type TYPE) since information
 *      above this in the structure has already been translated.
 */

static bool_t 
st_xqu_request (
    XDR *xdrsp,       /* xdr handle structure */
    CSI_V0_QUERY_REQUEST *reqp     /* partly translated query request */
)
{
    register int        i;              /* loop counter */
    register int        count;          /* holds count value */
    register int        part_size;      /* size of a portion of the packet */
    register int        total_size;     /* size of the packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xqu_request", 2, (unsigned long) xdrsp,
             (unsigned long) reqp);
#endif /* DEBUG */

    /* translate starting at type */
    part_size = (char*) &reqp->count - (char*) &reqp->type;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xtype(xdrsp, &reqp->type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()", "xdr_enum()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate count */
    part_size = (char*) &reqp->identifier - (char*) &reqp->count;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &reqp->count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()", "xdr_u_short()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* check boundary condition before loop */
    if (reqp->count > MAX_ID) {
        MLOGCSI((STATUS_COUNT_TOO_LARGE, "st_xqu_request()", CSI_NO_CALLEE, 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    total_size = csi_xcur_size;
    /* translate array of volid's */
    for (i = 0, count = reqp->count; i < count; i++ ) {
        switch (reqp->type) {
         case TYPE_ACS:
            /* translate starting at volid */
            part_size = (char*) &reqp->identifier.acs_id[i+1]
                - (char*) &reqp->identifier.acs_id[i];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xacs(xdrsp, &reqp->identifier.acs_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()", "acs()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.acs_id[reqp->count]
                - (char*) reqp;
            break;
         case TYPE_LSM:
            /* translate starting at lsmid */
            part_size = (char*) &reqp->identifier.lsm_id[i+1]
                - (char*) &reqp->identifier.lsm_id[i];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xlsm_id(xdrsp, &reqp->identifier.lsm_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()", "csi_xlsm_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.lsm_id[reqp->count]
                - (char*) reqp;
            break;
         case TYPE_CAP:
            /* translate starting at capid */
            part_size = (char*) &reqp->identifier.cap_id[i+1]
                - (char*) &reqp->identifier.cap_id[i];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xv0_cap_id(xdrsp,&reqp->identifier.cap_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()", "csi_xv0_cap_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.cap_id[reqp->count]
                - (char*) reqp;
            break;
         case TYPE_DRIVE:
            /* translate starting at driveid */
            part_size = (char*) &reqp->identifier.drive_id[i+1]
                - (char*) &reqp->identifier.drive_id[i];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xdrive_id(xdrsp, &reqp->identifier.drive_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()", "csi_xdrive_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.drive_id[reqp->count]
                - (char*) reqp;
            break;
         case TYPE_VOLUME:
         case TYPE_MOUNT:
            /* translate starting at volid */
            part_size = (char*) &reqp->identifier.vol_id[i+1]
                - (char*) &reqp->identifier.vol_id[i];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xvol_id(xdrsp, &reqp->identifier.vol_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()", "csi_xvol_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.vol_id[reqp->count]
                - (char*) reqp;
            break;
         case TYPE_REQUEST:
            /* translate starting at request (type MESSAGE_ID) */
            part_size = (char*) &reqp->identifier.request[i+1]
                - (char*) &reqp->identifier.request[i];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xmsg_id(xdrsp, &reqp->identifier.request[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()", "csi_xmsg_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.request[reqp->count]
                - (char*) reqp;
            break;
         case TYPE_PORT:
            /* translate starting at port_id */
            part_size = (char*) &reqp->identifier.port_id[i+1]
                - (char*) &reqp->identifier.port_id[i];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xport_id(xdrsp, &reqp->identifier.port_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()", "csi_xport_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.port_id[reqp->count]
                - (char*) reqp;
            break;
         case TYPE_SERVER:
            /* nothing to do here, acslm does everything */
            break;
         default:
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()", CSI_NO_CALLEE, 
	      MMSG(977, "Invalid type")));
            return(0);
        }
    }
    /* re-calc size in case there is compiler padding on last element */
    csi_xcur_size = total_size;
    return(1);
}

/*
 * Name:
 *      st_xva_request()
 *
 * Description:
 *      Routine serializes/deserializes a CSI_V0_VARY_REQUEST structure 
 *      from the point beginning with the state (type STATE).  The
 *      data above that point in the CSI_V0_VARY_REQUEST structure
 *      should have already been translated.
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
 *      (int) "csi_xcur_size" - increased by translated size this part of packet
 *
 * Considerations:
 *      Translation must start at type (type TYPE) since information
 *      above this in the structure has already been translated.
 */

static bool_t 
st_xva_request (
    XDR *xdrsp,        /* xdr handle structure */
    CSI_V0_VARY_REQUEST *reqp              /* vary request structure */
)
{
    register int        i;              /* loop counter */
    register int        count;          /* holds count value */
    register int        part_size;      /* size of a portion of the packet */
    register int        total_size;     /* size of the packet */
    BOOLEAN             badsize;        /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xva_request", 2, (unsigned long) xdrsp,
             (unsigned long) reqp);
#endif /* DEBUG */

    /* translate starting at state */
    part_size = (char*) &reqp->type - (char*) &reqp->state;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstate(xdrsp, &reqp->state)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_request", "xdr_enum()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate starting at type */
    part_size = (char*) &reqp->count - (char*) &reqp->type;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xtype(xdrsp, &reqp->type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_request", "xdr_enum()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate count */
    part_size = (char*) &reqp->identifier - (char*) &reqp->count;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &reqp->count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_request", "xdr_u_short()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* check boundary condition before loop */
    if (reqp->count > MAX_ID) {
        MLOGCSI((STATUS_COUNT_TOO_LARGE, "st_xva_request", CSI_NO_CALLEE, 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    total_size = csi_xcur_size;
    /* translate array of volid's */
    for (i = 0, count = reqp->count; i < count; i++ ) {
        switch (reqp->type) {
         case TYPE_ACS:
            /* translate starting at acs */
            part_size = (char*) &reqp->identifier.acs_id[i+1]
                - (char *) &reqp->identifier.acs_id[i];
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xacs(xdrsp, &reqp->identifier.acs_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_request", "csi_xacs()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.acs_id[reqp->count]
                - (char *)reqp;
            break;
         case TYPE_PORT:
            /* translate starting at port_id */
            part_size = (char*) &reqp->identifier.port_id[i+1]
                - (char*) &reqp->identifier.port_id[i];
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xport_id(xdrsp, &reqp->identifier.port_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_request", "csi_xport_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.port_id[reqp->count]
                - (char *)reqp;
            break;
         case TYPE_LSM:
            /* translate starting at lsmid */
            part_size = (char*) &reqp->identifier.lsm_id[i+1]
                - (char*) &reqp->identifier.lsm_id[i];
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xlsm_id(xdrsp, &reqp->identifier.lsm_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_request", "csi_xlsm_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.lsm_id[reqp->count]
                - (char *)reqp;
            break;
         case TYPE_DRIVE:
            /* translate starting at driveid */
            part_size = (char*) &reqp->identifier.drive_id[i+1]
                - (char*) &reqp->identifier.drive_id[i];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xdrive_id(xdrsp, &reqp->identifier.drive_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_request", "csi_xdrive_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.drive_id[reqp->count]
                - (char *)reqp;
            break;
         default:
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_request", CSI_NO_CALLEE, 
	      MMSG(977, "Invalid type")));
            return(0);
        }
    }
    /* re-calc size in case there is compiler padding on last element */
    csi_xcur_size = total_size;
    return(1);
}
