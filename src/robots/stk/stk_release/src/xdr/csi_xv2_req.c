#ifndef lint
static char SccsId[] = "@(#)csi_xv2_req.c	5.6 1/19/94 ";
#endif
/*
 * Copyright (1991, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_xv2_req()
 *
 * Description:
 *      Encode/Decode Version 2 or Version 3 requests.
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
 *      if the IPC_HEADER ever becomes larger than the CSI_HEADER.  The start
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
 *      None.
 *
 * Revision History:
 *      H. I. Grapek    29-Aug-1991     Created (actually, stolen and 
 *                                      modified from csi_xv1_req().
 *      J. A. Wishner   22-Nov-1991.    Check that server audit requests have
 *                                      a count of zero.
 *      J. A. Wishner   10-Dec-1991.    Fix bug where xdr_short was being
 *                                      used to translate cap_priority.  It
 *                                      is supposed to use xdr_char.
 *      E. A. Alongi    28-Aug-1992     Eliminated useless test for EXTENDED
 *                                      bit.
 *	E. A. Alongi	07-Jun-1993	Converted all instances of current 
 *					request structure CSI_REQUEST
 *					to CSI_V2_REQUEST for R5.0.
 *	E. A. Alongi	19-Jan-1994	Modifications due to flint run.
 *	S. L. Siao	20-Mar-2002	Added type cast to arguments of xdr functions.
 */

/*      Header Files: */
#include "csi.h"
#include <csi_xdr_xlate.h> 
#include "ml_pub.h"
#include "cl_pub.h"
#include "csi_xdr_pri.h"

/*
 * Defines, Typedefs and Structure Definitions 
 */

/*
 * Function prototypes:
 */

static bool_t st_xau_request (XDR *xdrsp, CSI_V2_AUDIT_REQUEST *reqp);
static bool_t st_xqu_request (XDR *xdrsp, CSI_V2_QUERY_REQUEST *reqp);
static bool_t st_xva_request (XDR *xdrsp, CSI_V2_VARY_REQUEST *reqp);

/*      Global and Static Variable Declarations: */
static char     *st_module = "csi_xv2_req()";

/*      Procedure Type Declarations: */

bool_t 
csi_xv2_req (
    XDR *xdrsp,                    /* XDR handle */
    CSI_V2_REQUEST *reqp
)
{
    register int    i;
    register int    count;
    register int    part_size = 0;
    register int    total_size = 0;
    BOOLEAN         badsize;

    switch (reqp->csi_req_header.message_header.command) {
        case COMMAND_AUDIT:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_audit_req.cap_id 
                          - (char*) &reqp->csi_audit_req;
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
                          - (char*) &reqp->csi_cancel_req;
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
            csi_xcur_size = sizeof(CSI_V2_CANCEL_REQUEST);
            break;
        case COMMAND_DISMOUNT:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_dismount_req.vol_id 
                          - (char*) &reqp->csi_dismount_req;
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
            csi_xcur_size = sizeof(CSI_V2_DISMOUNT_REQUEST);
            break;
        case COMMAND_EJECT:
            if (RANGE & reqp->csi_req_header.message_header.extended_options) {
                /* FORMAT B eject request - using volranges. */
                /* portability: size end of request header for  padding*/
                csi_xcur_size = (char*) &reqp->csi_xeject_req.cap_id 
                              - (char*) &reqp->csi_xeject_req;
                /* translate starting at cap_id */
                part_size = (char*) &reqp->csi_xeject_req.count
                          - (char*) &reqp->csi_xeject_req.cap_id;
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || !csi_xcap_id(xdrsp, 
                    &reqp->csi_xeject_req.cap_id)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xcap_id()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }  
                csi_xcur_size += part_size;
                /* translate count */
                part_size = (char*) &reqp->csi_xeject_req.vol_range[0]
                    - (char *) &reqp->csi_xeject_req.count;
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || !xdr_u_short(xdrsp, 
                    &reqp->csi_xeject_req.count)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "xdr_u_short()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                csi_xcur_size += part_size;
                /* check boundary condition before loop */
                if (reqp->csi_xeject_req.count > MAX_ID) {
                    MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                /* translate array of volrange's */
                for (i=0, count = reqp->csi_xeject_req.count; i < count; i++) {
                    /* translate starting at volid */
                    part_size = (char*) &reqp->csi_xeject_req.vol_range[i+1]
                        - (char *) &reqp->csi_xeject_req.vol_range[i];
                    badsize = CHECKSIZE(csi_xcur_size,part_size,csi_xexp_size);
                    if (badsize || !csi_xvolrange(xdrsp, 
                        &reqp->csi_xeject_req.vol_range[i])) {
                        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
			  "csi_xvol_range()", 
			  MMSG(928, "XDR message translation failure")));
                        return 0;
                    }
                    csi_xcur_size += part_size;
                }
                /* re-calc size in case there is padding on last element */
                csi_xcur_size = (char*) &reqp->csi_xeject_req.
                    vol_range[reqp->csi_eject_req.count] - (char*) reqp;
            } else {
                /* FORMAT A eject request - using volid's. */
                /* portability: size end of request header for  padding*/
                csi_xcur_size = (char*) &reqp->csi_eject_req.cap_id 
                              - (char*) &reqp->csi_eject_req;
                /* translate starting at cap_id */
                part_size = (char*) &reqp->csi_eject_req.count
                    - (char*) &reqp->csi_eject_req.cap_id;
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || !csi_xcap_id(xdrsp, 
                    &reqp->csi_eject_req.cap_id)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xcap_id()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }  
                csi_xcur_size += part_size;
                /* translate count */
                part_size = (char*) &reqp->csi_eject_req.vol_id[0]
                    - (char *) &reqp->csi_eject_req.count;
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || !xdr_u_short(xdrsp, 
                    &reqp->csi_eject_req.count)) {
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
                    badsize = CHECKSIZE(csi_xcur_size,part_size,csi_xexp_size);
                    if (badsize || 
                        !csi_xvol_id(xdrsp, &reqp->csi_eject_req.vol_id[i])) {
                        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
			  "csi_xvol_id()", 
			  MMSG(928, "XDR message translation failure")));
                        return 0;
                    }
                    csi_xcur_size += part_size;
                }
                /* re-calc size in case there is padding on last element */
                csi_xcur_size = (char*) &reqp->csi_eject_req.
                    vol_id[reqp->csi_eject_req.count] - (char*) reqp;
            }
            break;
        case COMMAND_ENTER:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_enter_req.cap_id 
                          - (char*) &reqp->csi_enter_req;
            /* translate starting at capid */
            part_size = sizeof(CAPID);
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xcap_id(xdrsp, &reqp->csi_enter_req.cap_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xcap_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            /* note:  cap_id csi_xcur_size depends on enter type */
            if (reqp->csi_venter_req.csi_request_header.message_header.
                extended_options & VIRTUAL) {
                /* VENTER request  */
                /* calculate csi_cur_size for the cap_id including pad bytes */
                csi_xcur_size += (char*) &reqp->csi_venter_req.count
                          - (char*) &reqp->csi_venter_req.cap_id;
                /* translate count */
                part_size = (char*) &reqp->csi_venter_req.vol_id[0]
                          - (char *) &reqp->csi_venter_req.count;
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || 
                    !xdr_u_short(xdrsp, &reqp->csi_venter_req.count)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "xdr_u_short()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                csi_xcur_size += part_size;
                /* check boundary condition before loop */
                if (reqp->csi_venter_req.count > MAX_ID) {
                    MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                /* translate array of volid's */
                for (i=0, count = reqp->csi_venter_req.count; i < count; i++) {
                    /* translate starting at volid */
                    part_size = (char*) &reqp->csi_venter_req.vol_id[i+1]
                        - (char *) &reqp->csi_venter_req.vol_id[i];
                    badsize = CHECKSIZE(csi_xcur_size,part_size,csi_xexp_size);
                    if (badsize || 
                        !csi_xvol_id(xdrsp, &reqp->csi_venter_req.vol_id[i])) {
                        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
			  "csi_xvol_id()", 
			  MMSG(928, "XDR message translation failure")));
                        return 0;
                    }
                    csi_xcur_size += part_size;
                }
                /* re-calc size in case of compiler padding on last element */
                csi_xcur_size = (char*) &reqp->csi_venter_req.
                    vol_id[reqp->csi_venter_req.count] - (char*) reqp;
            } else {
                /* re-calc size for compiler padding on last element */
                /* Only need to do this for normal (non-virtual) ENTER */
                csi_xcur_size = sizeof(CSI_V2_ENTER_REQUEST);
            }
            break;
        case COMMAND_IDLE:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = sizeof(CSI_V2_IDLE_REQUEST);
            /* Idle is only a request header packet, no further translation. */
            break;
        case COMMAND_MOUNT:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_mount_req.vol_id 
                          - (char*) &reqp->csi_mount_req;
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
            /* re-calc size in case of compiler padding on last element */
            csi_xcur_size = (char*) &reqp->csi_mount_req.
                drive_id[reqp->csi_mount_req.count] - (char*) reqp;
            break;
        case COMMAND_MOUNT_SCRATCH:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_mount_scratch_req.pool_id 
                          - (char*) &reqp->csi_mount_scratch_req;
            /* translate pool_id */
            part_size = (char*) &reqp->csi_mount_scratch_req.count
                - (char*) &reqp->csi_mount_scratch_req.pool_id;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xpool_id(xdrsp, &reqp->csi_mount_scratch_req.pool_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xpool_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;
            /* translate count */
            part_size = (char*) &reqp->csi_mount_scratch_req.drive_id[0] 
                - (char*) &reqp->csi_mount_scratch_req.count;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !xdr_u_short(xdrsp, &reqp->csi_mount_scratch_req.count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "xdr_u_short()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;
            /* check boundary condition before loop */
            if (reqp->csi_mount_scratch_req.count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            /* loop on the drive-id translating */
            for (i=0, count=reqp->csi_mount_scratch_req.count; i < count; i++) {
                /* translate starting at driveid */
                part_size = (char*) &reqp->csi_mount_scratch_req.drive_id[i+1]
                    - (char*) &reqp->csi_mount_scratch_req.drive_id[i];
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || !csi_xdrive_id(xdrsp, 
                    &reqp->csi_mount_scratch_req.drive_id[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xdrive_id()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                csi_xcur_size += part_size;
            }
            /* re-calc size in case of compiler padding on last element */
            csi_xcur_size = (char*) &reqp->csi_mount_scratch_req.
                drive_id[reqp->csi_mount_scratch_req.count] - (char*) reqp;
            break;
        case COMMAND_LOCK:
        case COMMAND_UNLOCK:
        case COMMAND_QUERY_LOCK:
        case COMMAND_CLEAR_LOCK:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_lock_req.type 
                          - (char*) &reqp->csi_lock_req;
            /* translate type */
            part_size = (char*) &reqp->csi_lock_req.count
                - (char*) &reqp->csi_lock_req.type;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xtype(xdrsp, &reqp->csi_lock_req.type)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_enum()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;
            /* translate count */
            part_size = (char*) &reqp->csi_lock_req.identifier
                - (char*) &reqp->csi_lock_req.count;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !xdr_u_short(xdrsp, &reqp->csi_lock_req.count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "xdr_u_short()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;
            /* check boundary condition before loop */
            if (reqp->csi_lock_req.count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            total_size = csi_xcur_size;
            /* loop on the drive-id translating */
            for (i=0, count=reqp->csi_lock_req.count; i < count; i++) {
                switch (reqp->csi_lock_req.type) {
                 case TYPE_VOLUME:
                    /* translate volid */
                    part_size = 
                        (char*) &reqp->csi_lock_req.identifier.vol_id[i+1]
                        - (char*) &reqp->csi_lock_req.identifier.vol_id[i];
                    badsize = CHECKSIZE(csi_xcur_size,part_size,csi_xexp_size);
                    if (badsize || !csi_xvol_id(xdrsp, 
                        &reqp->csi_lock_req.identifier.vol_id[i])) {
                        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
			  "csi_xvol_id()", 
			  MMSG(928, "XDR message translation failure")));
                        return(0);
                    }
                    csi_xcur_size += part_size;
                    total_size = (char*) &reqp->csi_lock_req.identifier.
                        vol_id[reqp->csi_lock_req.count] - (char*) reqp;
                    break;
                 case TYPE_DRIVE:
                    /* translate driveid */
                    part_size = 
                        (char*) &reqp->csi_lock_req.identifier.drive_id[i+1]
                        - (char*) &reqp->csi_lock_req.identifier.drive_id[i];
                    badsize = CHECKSIZE(csi_xcur_size,part_size,csi_xexp_size);
                    if (badsize || !csi_xdrive_id(xdrsp, 
                        &reqp->csi_lock_req.identifier.drive_id[i])) {
                        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
			  "csi_xdrive_id()", 
			  MMSG(928, "XDR message translation failure")));
                        return(0);
                    }
                    csi_xcur_size += part_size;
                    total_size = (char*) &reqp->csi_lock_req.identifier.
                        drive_id[reqp->csi_lock_req.count] - (char*) reqp;
                    break;
                 default:
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      CSI_NO_CALLEE, 
		      MMSG(977, "Invalid type")));
                    return(0);
                }
            }
            /* re-calc size in case of compiler padding on last element */
            csi_xcur_size = total_size;
            break;
        case COMMAND_QUERY:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_query_req.type 
                          - (char*) &reqp->csi_query_req;
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
            csi_xcur_size = sizeof(CSI_V2_START_REQUEST);
            /* Start is only a request header packet, no further translation */
            break;
        case COMMAND_VARY:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_vary_req.state 
                          - (char*) &reqp->csi_vary_req;
            /* translate the rest of the vary packet */
            if (!st_xva_request(xdrsp, &reqp->csi_vary_req)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xva_request()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            break;
        case COMMAND_DEFINE_POOL:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_define_pool_req.low_water_mark 
                          - (char*) &reqp->csi_define_pool_req;
            /* translate low water mark */
            part_size = (char*) &reqp->csi_define_pool_req.high_water_mark
                - (char*) &reqp->csi_define_pool_req.low_water_mark;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !xdr_u_long(xdrsp, &reqp->csi_define_pool_req.low_water_mark)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_long()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;
            /* translate high water mark */
            part_size = (char*) &reqp->csi_define_pool_req.pool_attributes
                - (char*) &reqp->csi_define_pool_req.high_water_mark;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !xdr_u_long(xdrsp, &reqp->csi_define_pool_req.high_water_mark)){
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_long()",
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;
            /* translate pool attributes */
            part_size = (char*) &reqp->csi_define_pool_req.count
                - (char*) &reqp->csi_define_pool_req.pool_attributes;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !xdr_u_long(xdrsp, &reqp->csi_define_pool_req.pool_attributes)){
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_long()",
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;
            /* translate count */
            part_size = (char*) &reqp->csi_define_pool_req.pool_id[0]
                - (char*) &reqp->csi_define_pool_req.count;
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !xdr_u_short(xdrsp, &reqp->csi_define_pool_req.count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "xdr_u_short()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;
            /* validate count */
            if (reqp->csi_define_pool_req.count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            /* translate the pool_id values */
            for (i=0, count=reqp->csi_define_pool_req.count; i < count; i++ ) {
                part_size = (char*) &reqp->csi_define_pool_req.pool_id[i+1]
                    - (char*) &reqp->csi_define_pool_req.pool_id[i];
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || !csi_xpool_id(xdrsp,&reqp->csi_define_pool_req.
                    pool_id[i])){
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xpool_id()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                csi_xcur_size += part_size;
            }
            /* re-calc size in case of compiler padding on last element */
            csi_xcur_size = (char*) &reqp->csi_define_pool_req.
                pool_id[reqp->csi_define_pool_req.count] - (char*) reqp;
            break;
        case COMMAND_DELETE_POOL:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_delete_pool_req.count 
                          - (char*) &reqp->csi_delete_pool_req;
            /* translate count */
            part_size = (char*) &reqp->csi_delete_pool_req.pool_id[0] 
                - (char*) &reqp->csi_delete_pool_req.count;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !xdr_u_short(xdrsp, &reqp->csi_delete_pool_req.count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "xdr_u_short()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;
            /* validate count */
            if (reqp->csi_delete_pool_req.count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            /* translate the pool_id values */
            for (i=0, count=reqp->csi_delete_pool_req.count; i < count; i++ ) {
                part_size = (char*) &reqp->csi_delete_pool_req.pool_id[i+1]
                    - (char*) &reqp->csi_delete_pool_req.pool_id[i];
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || !csi_xpool_id(xdrsp,&reqp->csi_delete_pool_req.
                    pool_id[i])){
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xpool_id()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                csi_xcur_size += part_size;
            }

            /* re-calc size in case of compiler padding on last element */
            csi_xcur_size = (char*) &reqp->csi_delete_pool_req.
                pool_id[reqp->csi_delete_pool_req.count] - (char*) reqp;
            break;

        case COMMAND_SET_CAP:

            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_set_cap_req.cap_priority 
                          - (char*) &reqp->csi_set_cap_req;

            /* translate cap_priority */
            part_size = (char*) &reqp->csi_set_cap_req.cap_mode
                - (char*) &reqp->csi_set_cap_req.cap_priority;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !xdr_char(xdrsp, (char *) &reqp->csi_set_cap_req.cap_priority)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "xdr_u_short()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;

            /* translate cap_mode */
            part_size = (char*) &reqp->csi_set_cap_req.count
                - (char*) &reqp->csi_set_cap_req.cap_mode;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xcap_mode(xdrsp, &reqp->csi_set_cap_req.cap_mode)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xcap_mode()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;

            /* translate count */
            part_size = (char*) &reqp->csi_set_cap_req.cap_id[0] 
                - (char*) &reqp->csi_set_cap_req.count;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !xdr_u_short(xdrsp, &reqp->csi_set_cap_req.count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "xdr_u_short()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;

            /* check boundary condition before loop */
            if (reqp->csi_set_cap_req.count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }

            /* translate the cap_id values */
            for (i=0, count=reqp->csi_set_cap_req.count; i < count; i++ ) {
                part_size = (char*) &reqp->csi_set_cap_req.cap_id[i+1]
                    - (char*) &reqp->csi_set_cap_req.cap_id[i];
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || !csi_xcap_id(xdrsp,&reqp->csi_set_cap_req.
                    cap_id[i])){
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xcap_id()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                csi_xcur_size += part_size;
            }

            /* re-calc size in case of compiler padding on last element */
            csi_xcur_size = (char*) &reqp->csi_set_cap_req.
                cap_id[reqp->csi_set_cap_req.count] - (char*) reqp;
            break;

        case COMMAND_SET_CLEAN:

            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_set_clean_req.max_use 
                          - (char*) &reqp->csi_set_clean_req;
            /* translate max_use */
            part_size = (char*) &reqp->csi_set_clean_req.count
                - (char*) &reqp->csi_set_clean_req.max_use;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !xdr_u_short(xdrsp, &reqp->csi_set_clean_req.max_use)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "xdr_u_short()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;

            /* translate count */
            part_size = (char*) &reqp->csi_set_clean_req.vol_range[0] 
                - (char*) &reqp->csi_set_clean_req.count;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !xdr_u_short(xdrsp, &reqp->csi_set_clean_req.count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "xdr_u_short()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;

            /* check boundary condition before loop */
            if (reqp->csi_set_clean_req.count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }

            /* translate the vol_range values */
            for (i=0, count=reqp->csi_set_clean_req.count; i < count; i++ ) {
                part_size = (char*) &reqp->csi_set_clean_req.vol_range[i+1]
                    - (char*) &reqp->csi_set_clean_req.vol_range[i];
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || !csi_xvolrange(xdrsp,&reqp->csi_set_clean_req.
                    vol_range[i])){
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xvol_range()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                csi_xcur_size += part_size;
            }

            /* re-calc size in case of compiler padding on last element */
            csi_xcur_size = (char*) &reqp->csi_set_clean_req.
                vol_range[reqp->csi_set_clean_req.count] - (char*) reqp;
            break;

        case COMMAND_SET_SCRATCH:

            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = (char*) &reqp->csi_set_scratch_req.pool_id 
                          - (char*) &reqp->csi_set_scratch_req;
            /* translate starting at poolid */
            part_size = (char*) &reqp->csi_set_scratch_req.count
                - (char*) &reqp->csi_set_scratch_req.pool_id;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xpool_id(xdrsp, &reqp->csi_set_scratch_req.pool_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xpool_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;

            /* translate count */
            part_size = (char*) &reqp->csi_set_scratch_req.vol_range[0] 
                - (char*) &reqp->csi_set_scratch_req.count;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !xdr_u_short(xdrsp, &reqp->csi_set_scratch_req.count)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "xdr_u_short()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            csi_xcur_size += part_size;

            /* check boundary condition before loop */
            if (reqp->csi_set_scratch_req.count > MAX_ID) {
                MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }

            /* translate the vol_range values */
            for (i=0, count=reqp->csi_set_scratch_req.count; i < count; i++ ) {
                part_size = (char*) &reqp->csi_set_scratch_req.vol_range[i+1]
                    - (char*) &reqp->csi_set_scratch_req.vol_range[i];
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize || !csi_xvolrange(xdrsp,&reqp->csi_set_scratch_req.
                    vol_range[i])){
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xvol_range()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                csi_xcur_size += part_size;
            }
            /* re-calc size in case of compiler padding on last element */
            csi_xcur_size = (char*) &reqp->csi_set_scratch_req.
                vol_range[reqp->csi_set_scratch_req.count] - (char*) reqp;
            break;
        default:
            /* portability: size end of request header for compiler padding*/
            csi_xcur_size = sizeof(CSI_V2_REQUEST_HEADER);
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
 *      Routine serializes/deserializes a partly translated CSI_V2_AUDIT_REQUEST
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
    XDR *xdrsp,       /* xdr handle structure */
    CSI_V2_AUDIT_REQUEST *reqp        /* partly translated audit request */
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
    if (badsize || !csi_xcap_id(xdrsp, &reqp->cap_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_request", "csi_xcap_id()",
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
            total_size = (char *)&reqp->identifier.acs_id[reqp->count] -
                (char *)reqp;
            break;
         case TYPE_LSM:
            part_size = (char*) &reqp->identifier.lsm_id[i+1]
                - (char*) &reqp->identifier.lsm_id[i];
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xlsm_id(xdrsp, &reqp->identifier.lsm_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_request",
		  "csi_xlsm_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.lsm_id[reqp->count] -
                (char *)reqp;
            break;
         case TYPE_PANEL:
            part_size = (char*) &reqp->identifier.panel_id[i+1]
                - (char*) &reqp->identifier.panel_id[i];
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xpnl_id(xdrsp, &reqp->identifier.panel_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_request",
		  "csi_xlsm_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.panel_id[reqp->count] -
                (char *)reqp;
            break;
         case TYPE_SUBPANEL:
            part_size = (char*) &reqp->identifier.subpanel_id[i+1]
                - (char*) &reqp->identifier.subpanel_id[i];
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xspnl_id(xdrsp, &reqp->identifier.subpanel_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_request",
		  "csi_xspnl_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.subpanel_id[reqp->count] -
                (char *)reqp;
            break;
         case TYPE_SERVER:
            /* should not get here, audit server has a count of zero */
            MLOGCSI((STATUS_COUNT_TOO_LARGE, "st_xau_request", CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            return(0);
         default:
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_request",
	      CSI_NO_CALLEE, 
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
 *      Routine serializes/deserializes a CSI_V2_QUERY_REQUEST structure 
 *      from the point beginning with the type (type TYPE).  The
 *      data above that point in the CSI_V2_QUERY_REQUEST structure
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
    CSI_V2_QUERY_REQUEST *reqp        /* partly translated query request */
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
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()",
	  "xdr_u_short()", 
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
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()",
		  "acs()", 
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
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()",
		  "csi_xlsm_id()", 
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
                if (badsize || !csi_xcap_id(xdrsp,
						&reqp->identifier.cap_id[i])) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()",
		      "csi_xcap_id()", 
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
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()",
		  "csi_xdrive_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.drive_id[reqp->count]
                - (char*) reqp;
            break;
         case TYPE_VOLUME:
         case TYPE_CLEAN:
         case TYPE_MOUNT:
            /* translate starting at volid */
            part_size = (char*) &reqp->identifier.vol_id[i+1]
                - (char*) &reqp->identifier.vol_id[i];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xvol_id(xdrsp, &reqp->identifier.vol_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()",
		  "csi_xvol_id()", 
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
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()",
		  "csi_xmsg_id()", 
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
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()",
		  "csi_xport_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.port_id[reqp->count]
                - (char*) reqp;
            break;
         case TYPE_POOL:
         case TYPE_SCRATCH:
         case TYPE_MOUNT_SCRATCH:
            /* translate starting at pool_id */
            part_size = (char*) &reqp->identifier.pool_id[i+1]
                - (char*) &reqp->identifier.pool_id[i];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xpool_id(xdrsp, &reqp->identifier.pool_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()",
		  "csi_xpool_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.pool_id[reqp->count]
                - (char*) reqp;
            break;
         case TYPE_SERVER:
            /* nothing to do here, acslm does everything */
            break;
         default:
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xqu_request()",
	      CSI_NO_CALLEE, 
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
 *      Routine serializes/deserializes a CSI_V2_VARY_REQUEST structure 
 *      from the point beginning with the state (type STATE).  The
 *      data above that point in the CSI_V2_VARY_REQUEST structure
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
    CSI_V2_VARY_REQUEST *reqp         /* vary request structure */
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
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_request",
		  "csi_xacs()", 
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
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_request",
		  "csi_xport_id()", 
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
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_request",
		  "csi_xlsm_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.lsm_id[reqp->count]
                - (char *)reqp;
            break;
         case TYPE_CAP:
            /* translate starting at capid */
            part_size = (char*) &reqp->identifier.cap_id[i+1]
                - (char*) &reqp->identifier.cap_id[i];
            badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || !csi_xcap_id(xdrsp, &reqp->identifier.cap_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_request",
		  "csi_xcap_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.cap_id[reqp->count]
                - (char *)reqp;
            break;
         case TYPE_DRIVE:
            /* translate starting at driveid */
            part_size = (char*) &reqp->identifier.drive_id[i+1]
                - (char*) &reqp->identifier.drive_id[i];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize || 
                !csi_xdrive_id(xdrsp, &reqp->identifier.drive_id[i])) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_request",
		  "csi_xdrive_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            csi_xcur_size += part_size;
            total_size = (char*) &reqp->identifier.drive_id[reqp->count]
                - (char *)reqp;
            break;
         default:
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_request",
	      CSI_NO_CALLEE, 
	      MMSG(977, "Invalid type")));
            return(0);
        }
    }
    /* re-calc size in case there is compiler padding on last element */
    csi_xcur_size = total_size;
    return(1);
}
