#ifndef lint
static char SccsId[] = "@(#)csi_xv4_res.c	5.3 11/11/01 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      csi_xv4_res()
 *
 * Description:
 *
 *      Encode/Decode Version 1 responses.
 *
 * Return Values:
 *
 *      bool_t          - 1 successful xdr conversion of at least request header
 *      bool_t          - 0 xdr conversion failed
 *
 * Implicit Inputs:
 *
 *      NONE
 *
 * Implicit Outputs:
 *
 *      bufferp->data            - data is placed here during deserialization.
 *      bufferp->size            - size of data put here during deserialization.
 *      bufferp->packet_status   - describes various translation errors
 *      bufferp->translated_size - bytes of data the could be translated
 *
 * Considerations:
 *
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
 *      The value of packet size returned is undefined when 0 return code
 *      (error) is returned.
 *
 * Module Test Plan:
 *
 *      See CSI Unit Test Plan
 *
 * Revision History
 *	E. A. Alongi	    10-Jun-1993	    Created. A copy of csi_xv2_res.c
 *					    modified to translate VERSION4
 *					    response packets.
 *      C. J. Higgins       26-Oct-2001     Added Event Notification commands.
 *
 *      C. J. Higgins       13-Nov-2001     Added Display command.
 *      S. L. Siao          15-Feb-2002     Added Virtual_mount command.
 *      S. L. Siao          20-Mar-2002     Added type cast to arguments of xdr 
 *                                          functions.
 *      S. L. Siao          26-Apr-2002     Commented out debug statements with
 *                                          printf.  They are too low level for
 *                                          a standard debug, uncomment if you
 *                                          want to dump data.
 *      Wipro (Subhash)     28-May-2004     Modified to support Event
 *                                          Notification for mount/dismount.
 *      Mitch Black     07-Dec-2004     Prevent Linux warnings by putting in 
 *                                      correct declarations for static functions
 *                                      before they are called.
 */


/*
 * Header Files: 
 */

#include "csi.h"
#include <csi_xdr_xlate.h>
#include "ml_pub.h"
 
/*
 * Defines, Typedefs and Structure Definitions: 
 */

/*
 * Global and Static Variable Declarations: 
 */

static char     *st_src = __FILE__;
static char     *st_module = "csi_xv4_res()";
 
/*
 * Procedure Type Declarations: 
 */
static bool_t st_xau_response (XDR *, CSI_AUDIT_RESPONSE *);
static bool_t st_xva_response (XDR *, CSI_VARY_RESPONSE *);
static bool_t st_xeject_enter (XDR *, CSI_EJECT_ENTER *);


bool_t 
csi_xv4_res (
    XDR *xdrsp,                         /* xdr handle */
    CSI_RESPONSE *resp                          /* request pointer */
)
{
    register int    i;                          /* counts identifiers in loop */
    register int    count;                      /* counts identifiers in loop */
    register int    part_size = 0;              /* size of one structure */
    register int    total_size = 0;             /* total size of request */
    unsigned int    pad_size = 0;               /* pad size of request */
    BOOLEAN         badsize;                    /* TRUE if size error found */
    unsigned int    t_size;            		/* special for user id */
    char            *cp;                        /* special for user id */
    int             total = 0;

    switch (resp->csi_req_header.message_header.command) {

    case COMMAND_AUDIT:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_audit_res.cap_id
                      - (char*) &resp->csi_audit_res;
        /* determine if response is eject_enter intermediate or regular */
        if ((unsigned char)INTERMEDIATE & resp->csi_audit_res.
                        csi_request_header.message_header.message_options) {
            /* translate enter_eject intermediate response */
            if (!st_xeject_enter(xdrsp,
                &resp->csi_eject_enter_res)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xcap_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }  
        } else {
            /* translate the rest of the audit response packet */
            if (!st_xau_response(xdrsp, &resp->csi_audit_res)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "st_xau_response()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
        }
        break;

    case COMMAND_CANCEL:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_cancel_res.request
                      - (char*) &resp->csi_cancel_res;
        /* translate starting at "request" (type MESSAGE_ID) */
        part_size = sizeof(MESSAGE_ID);
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !csi_xmsg_id(xdrsp, &resp->csi_cancel_res.request)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xmsg_id()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        /* re-size in case there is compiler tail padding on struture */
        csi_xcur_size = sizeof(CSI_CANCEL_RESPONSE);
        break;

    case COMMAND_DISMOUNT:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_dismount_res.vol_id
                      - (char*) &resp->csi_dismount_res;
        /* translate starting at volid */
        part_size = (char*) &resp->csi_dismount_res.drive_id
            - (char*) &resp->csi_dismount_res.vol_id;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !csi_xvol_id(xdrsp,&resp->csi_dismount_res.vol_id)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xvol_id()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += part_size;
        /* translate starting at driveid */
        part_size = sizeof(DRIVEID);
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || 
            !csi_xdrive_id(xdrsp, &resp->csi_dismount_res.drive_id)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xdrive_id()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        /* re-size in case there is compiler tail padding on struture */
        csi_xcur_size = sizeof(CSI_DISMOUNT_RESPONSE);
        break;

    case COMMAND_EJECT:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_eject_res.cap_id
                      - (char*) &resp->csi_eject_res;
        /* translate eject response (same as EJECT_ENTER) */
        if (!st_xeject_enter(xdrsp, &resp->csi_eject_enter_res)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xcap_id()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }  
        break;

    case COMMAND_ENTER:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_enter_res.cap_id
                      - (char*) &resp->csi_enter_res;
        /* translate enter response (same as EJECT_ENTER) */
        if (!st_xeject_enter(xdrsp, &resp->csi_eject_enter_res)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xcap_id()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }  
        break;

    case COMMAND_IDLE:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = sizeof(CSI_IDLE_RESPONSE);
        /* no more to translate */
        break;

    case COMMAND_MOUNT:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_mount_res.vol_id
                      - (char*) &resp->csi_mount_res;
        /* translate starting at volid */
        part_size = (char*) &resp->csi_mount_res.drive_id
            - (char*) &resp->csi_mount_res.vol_id;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !csi_xvol_id(xdrsp, &resp->csi_mount_res.vol_id)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xvol_id()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += part_size;
        /* translate starting at driveid */
        part_size = sizeof(DRIVEID);
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !csi_xdrive_id(xdrsp, &resp->csi_mount_res.drive_id)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xdrive_id()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        /* re-size in case there is compiler tail padding on struture */
        csi_xcur_size = sizeof(CSI_MOUNT_RESPONSE);
        break;

    case COMMAND_MOUNT_SCRATCH:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_mount_scratch_res.pool_id
                      - (char*) &resp->csi_mount_scratch_res;
        /* translate poolid */
        part_size = (char*) &resp->csi_mount_scratch_res.drive_id
            - (char*) &resp->csi_mount_scratch_res.pool_id;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || 
            !csi_xpool_id(xdrsp, &resp->csi_mount_scratch_res.pool_id)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xpool_id()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += part_size;
        /* translate driveid */
        part_size = sizeof(DRIVEID);
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || 
            !csi_xdrive_id(xdrsp, &resp->csi_mount_scratch_res.drive_id)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xdrive_id()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += (char*) &resp->csi_mount_scratch_res.vol_id
                       - (char*) &resp->csi_mount_scratch_res.drive_id;
        /* translate volid */
        part_size = sizeof(VOLID);
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || 
            !csi_xvol_id(xdrsp, &resp->csi_mount_scratch_res.vol_id)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xvol_id()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        /* re-size in case there is compiler tail padding on struture */
        csi_xcur_size = sizeof(CSI_MOUNT_SCRATCH_RESPONSE);
        break;

    case COMMAND_QUERY:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_query_res.type
                      - (char*) &resp->csi_query_res;
        /* translate the rest of the query packet */
        if (!csi_xqu_response(xdrsp, &resp->csi_query_res)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xqu_response()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        break;

    case COMMAND_START:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = sizeof(CSI_START_RESPONSE);
        /* nothing more to do */
        break;

    case COMMAND_VARY:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_vary_res.state
                      - (char*) &resp->csi_vary_res;
        /* translate the rest of the vary packet */
        if (!st_xva_response(xdrsp, &resp->csi_vary_res)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "st_xva_response()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        break;

    case COMMAND_LOCK:
    case COMMAND_UNLOCK:
    case COMMAND_CLEAR_LOCK:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_lock_res.type
                      - (char*) &resp->csi_lock_res;
        /* translate type */
        part_size = (char*) &resp->csi_lock_res.count
                  - (char*) &resp->csi_lock_res.type;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || 
            !csi_xtype(xdrsp, &resp->csi_lock_res.type)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xtype()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += part_size;
        /* translate count */
        part_size = (char*) &resp->csi_lock_res.identifier_status
                  - (char*) &resp->csi_lock_res.count;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !xdr_u_short(xdrsp, &resp->csi_lock_res.count)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()", 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += part_size;
        /* check boundary condition before loop */
        if (resp->csi_lock_res.count > MAX_ID) {
            MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        count = resp->csi_lock_res.count;
        total_size = csi_xcur_size;
        /* Translate identifier_status */
        for (i = 0; i < count; i++, csi_xcur_size += part_size) {
            switch (resp->csi_lock_res.type) {
             case TYPE_VOLUME:
                part_size = (char *)&resp->csi_lock_res.identifier_status.
                    volume_status[1] - 
                    (char *)&resp->csi_lock_res.identifier_status.
                    volume_status[0];
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      CSI_NO_CALLEE, 
		      MMSG(928, "XDR message translation failure")));
                    return(0);
                }
                /* translate the vol_id */
                if (!csi_xvol_id(xdrsp, &resp->csi_lock_res.identifier_status.
                    volume_status[i].vol_id)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xvol_id()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                /* translate the sub-status (type RESPONSE_STATUS) */
                if (!csi_xres_status(xdrsp, &resp->csi_lock_res.
                    identifier_status.volume_status[i].status)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xres_status()", 
		      MMSG(928, "XDR message translation failure")));
                    return(0);
                }
                total_size = (char *)&resp->csi_lock_res.identifier_status.
                    volume_status[resp->csi_lock_res.count] - (char *)resp;
                break;
             case TYPE_DRIVE:
                part_size = (char *)&resp->csi_lock_res.identifier_status.
                    drive_status[1] - 
                    (char *)&resp->csi_lock_res.identifier_status.
                    drive_status[0];
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      CSI_NO_CALLEE, 
		      MMSG(928, "XDR message translation failure")));
                    return(0);
                }
                /* translate the drive_id */
                if (!csi_xdrive_id(xdrsp, &resp->csi_lock_res.identifier_status.
                    drive_status[i].drive_id)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xdrive_id()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                /* translate the sub-status (type RESPONSE_STATUS) */
                if (!csi_xres_status(xdrsp, &resp->csi_lock_res.
                    identifier_status.drive_status[i].status)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xres_status()", 
		      MMSG(928, "XDR message translation failure")));
                    return(0);
                }
                total_size = (char *)&resp->csi_lock_res.identifier_status.
                    drive_status[resp->csi_lock_res.count] - (char *)resp;
                break;
             default:
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
		  MMSG(977, "Invalid type")));
                return(0);
            }
        }
        /* re-calc size in case of compiler padding on last element */
        csi_xcur_size = total_size;
        break;

    case COMMAND_QUERY_LOCK:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_query_lock_res.type
                      - (char*) &resp->csi_query_lock_res;
        /* translate type */
        part_size = (char*) &resp->csi_query_lock_res.count
            - (char*) &resp->csi_query_lock_res.type;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || 
            !csi_xtype(xdrsp, &resp->csi_query_lock_res.type)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xtype()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += part_size;
        /* translate count */
        part_size = (char*) &resp->csi_query_lock_res.identifier_status - 
                    (char*) &resp->csi_query_lock_res.count;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !xdr_u_short(xdrsp, &resp->csi_query_lock_res.count)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()", 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += part_size;
        /* check boundary condition before loop */
        if (resp->csi_query_lock_res.count > MAX_ID) {
            MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        count = resp->csi_query_lock_res.count;
        total_size = csi_xcur_size;
        /* Translate identifier_status */
        for (i = 0; i < count; i++, csi_xcur_size += part_size) {
            switch (resp->csi_query_lock_res.type) {
             case TYPE_VOLUME:
                part_size = (char *)&resp->csi_query_lock_res.identifier_status.
                    volume_status[1] - 
                    (char *)&resp->csi_query_lock_res.identifier_status.
                    volume_status[0];
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      CSI_NO_CALLEE, 
		      MMSG(928, "XDR message translation failure")));
                    return(0);
                }
                /* translate the vol_id */
                if (!csi_xvol_id(xdrsp, 
                    &resp->csi_query_lock_res.identifier_status.
                    volume_status[i].vol_id)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xvol_id()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                /* translate the lock_id */
                if (!csi_xlockid(xdrsp, 
                    &resp->csi_query_lock_res.identifier_status.
                    volume_status[i].lock_id)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xlockid()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                /* translate the lock_duration */
                if (!xdr_u_long(xdrsp, 
                    &resp->csi_query_lock_res.identifier_status.
                    volume_status[i].lock_duration)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "xdr_u_long()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                /* translate the locks_pending */
                if (!xdr_u_int(xdrsp, 
                    &resp->csi_query_lock_res.identifier_status.
                    volume_status[i].locks_pending)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "xdr_u_int()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                /* translate the user_id */
                cp = resp->csi_query_lock_res.identifier_status.
                    volume_status[i].user_id.user_label;
                t_size = EXTERNAL_USERID_SIZE;
                if (!xdr_bytes(xdrsp, &cp, &t_size, 
                    sizeof(resp->csi_query_lock_res.identifier_status.
                    volume_status[i].user_id.user_label))) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "xdr_bytes()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                /* translate the status */
                if (!csi_xstatus(xdrsp, 
                    &resp->csi_query_lock_res.identifier_status.
                    volume_status[i].status)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xstatus()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                total_size=(char *)&resp->csi_query_lock_res.identifier_status.
                    volume_status[resp->csi_query_lock_res.count] - 
                    (char *)resp;
                break;
             case TYPE_DRIVE:
                part_size = (char *)&resp->csi_query_lock_res.identifier_status.
                    drive_status[1] - 
                    (char *)&resp->csi_query_lock_res.identifier_status.
                    drive_status[0];
                badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
                if (badsize) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      CSI_NO_CALLEE, 
		      MMSG(928, "XDR message translation failure")));
                    return(0);
                }
                /* translate the drive_id */
                if (!csi_xdrive_id(xdrsp, 
                    &resp->csi_query_lock_res.identifier_status.
                    drive_status[i].drive_id)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xdrive_id()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                /* translate the lock_id */
                if (!csi_xlockid(xdrsp, 
                    &resp->csi_query_lock_res.identifier_status.
                    drive_status[i].lock_id)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xlockid()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                /* translate the lock_duration */
                if (!xdr_u_long(xdrsp, 
                    &resp->csi_query_lock_res.identifier_status.
                    drive_status[i].lock_duration)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "xdr_u_long()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                /* translate the locks_pending */
                if (!xdr_u_int(xdrsp, 
                    &resp->csi_query_lock_res.identifier_status.
                    drive_status[i].locks_pending)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "xdr_u_int()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                /* translate the user_id */
                cp = resp->csi_query_lock_res.identifier_status.
                    drive_status[i].user_id.user_label;
                t_size = EXTERNAL_USERID_SIZE;
                if (!xdr_bytes(xdrsp, &cp, &t_size, 
                    sizeof(resp->csi_query_lock_res.identifier_status.
                    drive_status[i].user_id.user_label))) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "xdr_bytes()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                /* translate the status */
                if (!csi_xstatus(xdrsp, 
                    &resp->csi_query_lock_res.identifier_status.
                    drive_status[i].status)) {
                    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		      "csi_xstatus()", 
		      MMSG(928, "XDR message translation failure")));
                    return 0;
                }
                total_size=(char *)&resp->csi_query_lock_res.identifier_status.
                    drive_status[resp->csi_query_lock_res.count] - 
                    (char *)resp;
                break;
             default:
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
		  MMSG(977, "Invalid type")));
                return(0);
            }
        }
        /* re-calc size in case of compiler padding on last element */
        csi_xcur_size = total_size;
        break;

    case COMMAND_DEFINE_POOL:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_define_pool_res.low_water_mark
                      - (char*) &resp->csi_define_pool_res;
        /* translate low_water_mark */
        part_size = (char*) &resp->csi_define_pool_res.high_water_mark
            - (char*) &resp->csi_define_pool_res.low_water_mark;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || 
            !xdr_u_long(xdrsp, &resp->csi_define_pool_res.low_water_mark)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_long()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += part_size;
        /* translate high_water_mark */
        part_size = (char*) &resp->csi_define_pool_res.pool_attributes
            - (char*) &resp->csi_define_pool_res.high_water_mark;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || 
            !xdr_u_long(xdrsp, &resp->csi_define_pool_res.high_water_mark)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_long()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += part_size;
        /* translate pool_attributes */
        part_size = (char*) &resp->csi_define_pool_res.count
            - (char*) &resp->csi_define_pool_res.pool_attributes;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || 
            !xdr_u_long(xdrsp, &resp->csi_define_pool_res.pool_attributes)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_long()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += part_size;
        /* translate count */
        part_size = (char*) &resp->csi_define_pool_res.pool_status[0] - 
                    (char*) &resp->csi_define_pool_res.count;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !xdr_u_short(xdrsp, &resp->csi_define_pool_res.count)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()", 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += part_size;
        /* check boundary condition before loop */
        if (resp->csi_define_pool_res.count > MAX_ID) {
            MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        count = resp->csi_define_pool_res.count;
        /* Translate identifier_status */
        for (i = 0; i < count; i++, csi_xcur_size += part_size) {
            /* Translate pool_id */
            part_size =(char *)&resp->csi_define_pool_res.pool_status[1].pool_id
                 - (char *)&resp->csi_define_pool_res.pool_status[0].pool_id;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            if (!csi_xpool_id(xdrsp, &resp->csi_define_pool_res.pool_status[i].
                pool_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xpool_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            /* translate the sub-status (type RESPONSE_STATUS) */
            if (!csi_xres_status(xdrsp, &resp->csi_define_pool_res.
                pool_status[i].status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
        }
        /* re-calc size in case of compiler padding on last element */
        csi_xcur_size =(char *)&resp->csi_define_pool_res.
            pool_status[resp->csi_define_pool_res.count] - (char *)resp;
        break;

    case COMMAND_DELETE_POOL:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_delete_pool_res.count
                      - (char*) &resp->csi_delete_pool_res;
        /* translate count */
        part_size = (char*) &resp->csi_delete_pool_res.pool_status[0] - 
                    (char*) &resp->csi_delete_pool_res.count;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !xdr_u_short(xdrsp, &resp->csi_delete_pool_res.count)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()", 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += part_size;
        /* check boundary condition before loop */
        if (resp->csi_delete_pool_res.count > MAX_ID) {
            MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        count = resp->csi_delete_pool_res.count;
        /* Translate identifier_status */
        for (i = 0; i < count; i++, csi_xcur_size += part_size) {
            /* Translate pool_id */
            part_size =(char *)&resp->csi_delete_pool_res.pool_status[1].pool_id
                 - (char *)&resp->csi_delete_pool_res.pool_status[0].pool_id;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            if (!csi_xpool_id(xdrsp, &resp->csi_delete_pool_res.pool_status[i].
                pool_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xpool_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            /* translate the sub-status (type RESPONSE_STATUS) */
            if (!csi_xres_status(xdrsp, &resp->csi_delete_pool_res.
                pool_status[i].status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
        }
        /* re-calc size in case of compiler padding on last element */
        csi_xcur_size =(char *)&resp->csi_delete_pool_res.
            pool_status[resp->csi_delete_pool_res.count] - (char *)resp;
        break;

    case COMMAND_SET_CAP:

        /* portability: size end of response header for compiler padding*/

        csi_xcur_size = (char*) &resp->csi_set_cap_res.cap_priority
                      - (char*) &resp->csi_set_cap_res;

        /* translate cap_priority */
        part_size = (char*) &resp->csi_set_cap_res.cap_mode
            - (char*) &resp->csi_set_cap_res.cap_priority;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !xdr_char(xdrsp, (char *) &resp->csi_set_cap_res.cap_priority)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_char()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += part_size;

        /* translate cap_mode */
        part_size = (char*) &resp->csi_set_cap_res.count
            - (char*) &resp->csi_set_cap_res.cap_mode;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !csi_xcap_mode(xdrsp, &resp->csi_set_cap_res.cap_mode)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xcap_mode()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += part_size;

        /* translate count */
        part_size = (char*) &resp->csi_set_cap_res.set_cap_status[0] - 
                    (char*) &resp->csi_set_cap_res.count;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !xdr_u_short(xdrsp, &resp->csi_set_cap_res.count)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()", 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += part_size;

        /* check boundary condition before loop */
        if (resp->csi_set_cap_res.count > MAX_ID) {
            MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }

        count = resp->csi_set_cap_res.count;

        /* Translate identifier_status */
        for (i = 0; i < count; i++, csi_xcur_size += part_size) {
            /* Translate cap_id */
            part_size =(char *)&resp->csi_set_cap_res.set_cap_status[1].cap_id
                 - (char *)&resp->csi_set_cap_res.set_cap_status[0].cap_id;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }

            if (!csi_xcap_id(xdrsp, &resp->csi_set_cap_res.set_cap_status[i].
                cap_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xcap_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }

            /* translate the sub-status (type RESPONSE_STATUS) */
            if (!csi_xres_status(xdrsp, &resp->csi_set_cap_res.
                set_cap_status[i].status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
        }

        /* re-calc size in case of compiler padding on last element */
        csi_xcur_size = (char *)&resp->csi_set_cap_res.
            set_cap_status[resp->csi_set_cap_res.count] - (char *)resp;
        break;

    case COMMAND_SET_CLEAN:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_set_clean_res.max_use
                      - (char*) &resp->csi_set_clean_res;
        /* translate max_use */
        part_size = (char*) &resp->csi_set_clean_res.count
            - (char*) &resp->csi_set_clean_res.max_use;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !xdr_u_short(xdrsp, &resp->csi_set_clean_res.max_use)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += part_size;
        /* translate count */
        part_size = (char*) &resp->csi_set_clean_res.volume_status[0] - 
                    (char*) &resp->csi_set_clean_res.count;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !xdr_u_short(xdrsp, &resp->csi_set_clean_res.count)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()", 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += part_size;
        /* check boundary condition before loop */
        if (resp->csi_set_clean_res.count > MAX_ID) {
            MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        count = resp->csi_set_clean_res.count;
        /* Translate identifier_status */
        for (i = 0; i < count; i++, csi_xcur_size += part_size) {
            /* Translate vol_id */
            part_size =(char *)&resp->csi_set_clean_res.volume_status[1].vol_id
                 - (char *)&resp->csi_set_clean_res.volume_status[0].vol_id;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            if (!csi_xvol_id(xdrsp, &resp->csi_set_clean_res.volume_status[i].
                vol_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xvol_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            /* translate the sub-status (type RESPONSE_STATUS) */
            if (!csi_xres_status(xdrsp, &resp->csi_set_clean_res.
                volume_status[i].status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
        }
        /* re-calc size in case of compiler padding on last element */
        csi_xcur_size = (char *)&resp->csi_set_clean_res.
            volume_status[resp->csi_set_clean_res.count] - (char *)resp;
        break;

    case COMMAND_SET_SCRATCH:

        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_set_scratch_res.pool_id
                      - (char*) &resp->csi_set_scratch_res;
        /* translate pool_id */
        part_size = (char*) &resp->csi_set_scratch_res.count
            - (char*) &resp->csi_set_scratch_res.pool_id;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || 
            !csi_xpool_id(xdrsp, &resp->csi_set_scratch_res.pool_id)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xpool_id()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += part_size;
        /* translate count */
        part_size = (char*) &resp->csi_set_scratch_res.scratch_status[0] - 
                    (char*) &resp->csi_set_scratch_res.count;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !xdr_u_short(xdrsp, &resp->csi_set_scratch_res.count)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_short()", 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += part_size;
        /* check boundary condition before loop */
        if (resp->csi_set_scratch_res.count > MAX_ID) {
            MLOGCSI((STATUS_COUNT_TOO_LARGE, st_module, CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        count = resp->csi_set_scratch_res.count;
        /* Translate identifier_status */
        for (i = 0; i < count; i++, csi_xcur_size += part_size) {
            /* Translate vol_id */
            part_size = (char *)&resp->csi_set_scratch_res.scratch_status[1].
                vol_id - (char *)&resp->csi_set_scratch_res.scratch_status[0].
                vol_id;
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            if (!csi_xvol_id(xdrsp, &resp->csi_set_scratch_res.
                scratch_status[i].vol_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xvol_id()", 
		  MMSG(928, "XDR message translation failure")));
                return 0;
            }
            /* translate the sub-status (type RESPONSE_STATUS) */
            if (!csi_xres_status(xdrsp, &resp->csi_set_scratch_res.
                scratch_status[i].status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
        }
        /* re-calc size in case of compiler padding on last element */
        csi_xcur_size = (char *)&resp->csi_set_scratch_res.
            scratch_status[resp->csi_set_scratch_res.count] - (char *)resp;
        break;

	case COMMAND_REGISTER:
	/* portability: size end of response header for compiler padding*/
	csi_xcur_size = (char *)& resp->csi_register_res.event_reply_type - (char *)& resp->csi_register_res;
	/* translate starting at event_reply_type */
	part_size = (char *)& resp->csi_register_res.event_sequence - (char *)& resp->csi_register_res.event_reply_type;
	badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	if (badsize || !xdr_enum(xdrsp, (enum_t *) &resp->csi_register_res.event_reply_type))
	{
	    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_enum()", MMSG(928, "XDR message translation failure")));
	    return 0;
	}
	csi_xcur_size += part_size;
	/* translate event_sequence */
	part_size = (char *)& resp->csi_register_res.event - (char *)& resp->csi_register_res.event_sequence;
	badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	if (badsize || !xdr_u_long(xdrsp, &resp->csi_register_res.event_sequence))
	{
	    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_u_long()",MMSG(928, "XDR message translation failure")));
	    return 0;
	}
	csi_xcur_size += part_size;
	/* check reply type */
	switch (resp->csi_register_res.event_reply_type)
	{	
	  case EVENT_REPLY_VOLUME:
	    if (!csi_xevent_vol_status(xdrsp, &resp->csi_register_res.event.event_volume_status))
	    {
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,"csi_xevent_vol_status()",MMSG(928, "XDR message translation failure")));
		return 0;
	    }
	    break;
	  case EVENT_REPLY_RESOURCE:
	    if (!csi_xevent_rsrc_status(xdrsp, &resp->csi_register_res.event.event_resource_status))
	    {
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,"csi_xevent_rsrc_status()",MMSG(928, "XDR message translation failure")));
		return 0;
	    }
	    break;
      case EVENT_REPLY_DRIVE_ACTIVITY:
        if (!csi_xevent_drive_status(xdrsp, &resp->csi_register_res.event.event_drive_status))
        {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xevent_drive_status",
                         MMSG(928, "XDR message translation failure")));
            return 0;
        }
        break;
	  case EVENT_REPLY_REGISTER:
	  case EVENT_REPLY_UNREGISTER:
	  case EVENT_REPLY_SUPERCEDED:
	  case EVENT_REPLY_CLIENT_CHECK:
	  case EVENT_REPLY_SHUTDOWN:
	    if (!csi_xevent_reg_status(xdrsp, &resp->csi_register_res.event.event_register_status, &total))
	    {
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,"csi_xevent_reg_status()",MMSG(928, "XDR message translation failure")));
		return 0;
	    }
	    /* align bytes from 42 or 50, depending on count, to 128 */
	    pad_size = EVENT_ALIGN_PAD_SIZE - total;
	    if (pad_size > 0)
	    {
		/* Use a temporary buffer when decoding padding bytes and decode
	                * padding bytes with maxsize RESOURCE_ALIGN_PAD_SIZE. This approach
	                * has the side effect of correctly updating the xdrsp pointer.
	                */
		if (XDR_DECODE == xdrsp->x_op)
		{
		    char	    temp[EVENT_ALIGN_PAD_SIZE];/* temp buffer for xdr_bytes */
		    char	    *tmp = temp;
		    if (!xdr_bytes(xdrsp, &tmp, &pad_size, EVENT_ALIGN_PAD_SIZE))
		    {
			MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,"xdr_bytes()",MMSG(928, "XDR message translation failure")));
			return (0);
		    }
		}
		else/* XDR_ENCODE*/
		{
		    char *cp = (char *)(&resp->csi_register_res.event.event_register_status + total);
	 	    if (!xdr_bytes(xdrsp, &cp, &pad_size, pad_size))
		    {
			MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,"xdr_bytes()",MMSG(928,"XDR message translation failure")));
			return (0);
		    }
		}
	    }
	    /* update csi_xcursize*/
	    csi_xcur_size += pad_size;
	    break;
	  default:
	    /* If this code segment is entered, the event_reply_type is illegal. */
	    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE,MMSG(928, "XDR message translation failure")));
	    return (0);
	}/*end switch on event_reply*/
	break;

	case COMMAND_UNREGISTER:
	/* portability: size end of response header for compiler padding*/
	csi_xcur_size = (char *)& resp->csi_unregister_res.event_register_status
			- (char *)& resp->csi_unregister_res;
	/* translate the rest of the unregister packet */
	if (!csi_xevent_reg_status(xdrsp, &resp->csi_unregister_res.
				   event_register_status, &total))
	{
	    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
		     "csi_xevent_reg_status()", 
		     MMSG(928, "XDR message translation failure")));
	    return 0;
	}
	/* re-calc size in case of compiler padding on last element */
	csi_xcur_size = (char *)& resp->csi_unregister_res.event_register_status.register_status
			[resp->csi_unregister_res.event_register_status.count] - (
			char *) resp;
	break;

	case COMMAND_CHECK_REGISTRATION:
	/* portability: size end of response header for compiler padding*/
	csi_xcur_size = (char *)& resp->csi_check_registration_res.event_register_status
			- (char *)& resp->csi_check_registration_res;
	/* translate the rest of the check_registration_res packet */
	if (!csi_xevent_reg_status(xdrsp, &resp->csi_check_registration_res.
				   event_register_status, &total))
	{
	    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
		     "csi_xevent_reg_status()", 
		     MMSG(928, "XDR message translation failure")));
	    return 0;
	}
	/* re-calc size in case of compiler padding on last element */
	csi_xcur_size = (char *)& resp->csi_check_registration_res.event_register_status.register_status
			[resp->csi_check_registration_res.event_register_status.count] - (
			char *) resp;
	break;

        case COMMAND_DISPLAY:
	/* portability: size end of response header for compiler padding*/
	csi_xcur_size = (char *)&resp->csi_display_res.display_type
			- (char *)&resp->csi_display_res;
        part_size = (char*) &resp->csi_display_res.display_xml_data
            - (char*) &resp->csi_display_res.display_type;
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || 
            !csi_xtype(xdrsp, &resp->csi_display_res.display_type)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xtype()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += part_size;
        part_size = sizeof(unsigned short) + resp->csi_display_res.display_xml_data.length;
	badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	/* translate the rest of the display packet */
	if (badsize ||
	    !csi_xxml_data(xdrsp, &resp->csi_display_res.display_xml_data))
	    {
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			 "csi_xxml_data()", 
			 MMSG(928, "XDR message translation failure")));
		return 0;
	    }
/*#ifdef DEBUG
       printf(" csi_xdr_v4_response, csi_xcur_size is: %d\n", csi_xcur_size);
       printf(" csi_xdr_v4_resp, length is: %d\n", 
	       resp->csi_display_res.display_xml_data.length);
#endif*/
	/* re-calc size in case of compiler padding on last element */
	csi_xcur_size = (char *) &resp->csi_display_res.display_xml_data +
	    (sizeof(unsigned short) + resp->csi_display_res.display_xml_data.length) - (char *) resp;		
#ifdef DEBUG
        printf(" csi_xdr_v4_response, csi_xcur_size is: %d\n", csi_xcur_size);
#endif 
	break;

	case COMMAND_MOUNT_PINFO:

	    /* portability: size end of response header for compiler padding*/
	    csi_xcur_size = (char*) &resp->csi_mount_pinfo_res.pool_id
			  - (char*) &resp->csi_mount_pinfo_res;

	    /* translate poolid */
	    part_size = (char*) &resp->csi_mount_pinfo_res.drive_id
			- (char*) &resp->csi_mount_pinfo_res.pool_id;
	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	    if (badsize || !csi_xpool_id(xdrsp, 
					&resp->csi_mount_pinfo_res.pool_id)) {
		 MLOGCSI((STATUS_TRANSLATION_FAILURE,
		        st_module, "csi_xpool_id()",
			MMSG(928, "XDR message translation failure")));
		 return 0;
	    } 
	    csi_xcur_size += part_size; 

	    /* translate driveid */
	    part_size = (char*) &resp->csi_mount_pinfo_res.vol_id
		      - (char*) &resp->csi_mount_pinfo_res.drive_id;
	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	    if (badsize || !csi_xdrive_id(xdrsp, 
				       &resp->csi_mount_pinfo_res.drive_id)) {
		MLOGCSI((STATUS_TRANSLATION_FAILURE,
			st_module, "csi_xdrive_id()", 
			MMSG(928, "XDR message translation failure")));
		return 0;
	    }
	    csi_xcur_size += part_size; 

	    /* translate volid */
	    part_size = sizeof(VOLID);
	    badsize = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
	    if (badsize || !csi_xvol_id(xdrsp, 
					&resp->csi_mount_pinfo_res.vol_id)) {
		MLOGCSI((STATUS_TRANSLATION_FAILURE,
			st_module, "csi_xvol_id()", 
			MMSG(928, "XDR message translation failure")));
		return 0;
	    }

	    /* re-size in case there is compiler tail padding on struture */
	    csi_xcur_size = sizeof(CSI_MOUNT_PINFO_RESPONSE);
	    break;

	default:
	MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, CSI_NO_CALLEE, 
		 MMSG(975, "Invalid command")));
	return 0;
	}/* end of switch on command */
    return 1;
}

/*
 *
 * Name:
 *      st_xau_response()
 *
 * Description:
 *      Routine serializes/deserializes a partly translated
 * 	CSI_AUDIT_RESPONSE structure from the positiion of the CAPID, onward,
 *      downstream.
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
 *      (int) "csi_xcur_size" - increased by translated size of this part of packet
 *
 * Considerations:
 *      Translation must begin at CAPID structure since this packet is 
 *      received in this routine already partly translated.
 *
 */

static bool_t 
st_xau_response (
    XDR *xdrsp,      /* xdr handle structure */
    CSI_AUDIT_RESPONSE *resp  /* start of partly translated audit response */
)
{
    register int        i;                      /* loop counter */
    register int        count;                  /* holds count value */
    register int        part_size = 0;          /* size this part of a packet */
    register int        total_size = 0;         /* size of packet */
    BOOLEAN             badsize;                /* TRUE if packet size bogus */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xau_response()", 2, (unsigned long) xdrsp,
                 (unsigned long) resp);
#endif /* DEBUG */

    /* translate cap_id */
    part_size = (char*) &resp->type - (char*) &resp->cap_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xcap_id(xdrsp, &resp->cap_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
	  "csi_xcap_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }  
    csi_xcur_size += part_size;

    /* translate type */
    part_size = (char*) &resp->count - (char*) &resp->type;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xtype(xdrsp, &resp->type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
	  "csi_xtype()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate count */
    part_size = (char*) &resp->identifier_status - (char*) &resp->count;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &resp->count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
	  "xdr_u_short()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* check boundary condition before loop */
    if (resp->count > MAX_ID) {
        MLOGCSI((STATUS_COUNT_TOO_LARGE, "st_xau_response()", CSI_NO_CALLEE, 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    total_size = csi_xcur_size;
    for (i = 0, count = resp->count; i < count; i++, csi_xcur_size += part_size) {
        switch (resp->type) {
         case TYPE_ACS:
            part_size = (char *)&resp->identifier_status.acs_status[1] -
                (char *)&resp->identifier_status.acs_status[0];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
		  CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate the acs */
            if (!csi_xacs(xdrsp,&resp->identifier_status.acs_status[i].acs_id)){
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
		  "csi_xacs()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate the sub-status (type RESPONSE_STATUS) */
            if (!csi_xres_status(xdrsp, 
                &resp->identifier_status.acs_status[i].status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->identifier_status.
                acs_status[resp->count] - (char *)resp;
            break;
         case TYPE_LSM:
            part_size = (char *)&resp->identifier_status.lsm_status[1] -
                (char *)&resp->identifier_status.lsm_status[0];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
		  CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate lsm */
            if (!csi_xlsm_id(xdrsp, 
                &resp->identifier_status.lsm_status[i].lsm_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
		  "csi_xlsm_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate the status */
            if (!csi_xres_status(xdrsp, 
                &resp->identifier_status.lsm_status[i].status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->identifier_status.
                lsm_status[resp->count] - (char *)resp;
            break;
         case TYPE_PANEL:
            part_size = (char *)&resp->identifier_status.panel_status[1] -
                (char *)&resp->identifier_status.panel_status[0];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
		  CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate panel status */
            if (!csi_xpnl_id(xdrsp, 
                &resp->identifier_status.panel_status[i].panel_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
		  "csi_panel_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate the status */
            if (!csi_xres_status(xdrsp, 
                &resp->identifier_status.panel_status[i].status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->identifier_status.
                panel_status[resp->count] - (char *)resp;
            break;
         case TYPE_SUBPANEL:
            part_size = (char*)&resp->identifier_status.subpanel_status[1] -
                (char*)&resp->identifier_status.subpanel_status[0];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
		  CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate sub-panel status */
            if (!csi_xspnl_id(xdrsp, 
                &resp->identifier_status.subpanel_status[i].subpanel_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
		  "csi_xspnl_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate the status */
            if (!csi_xres_status(xdrsp, 
                &resp->identifier_status.subpanel_status[i].status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->identifier_status.
                subpanel_status[resp->count] - (char *)resp;
            break;
         default:
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
	      CSI_NO_CALLEE, 
	      MMSG(977, "Invalid type")));
            return(0);
        }
    }
    /* re-calc size in case of compiler padding on last element */
    csi_xcur_size = total_size;
    return(1);
}

/*
 * Name:
 *      st_xva_response()
 *
 * Description:
 *      Routine serializes/deserializes a CSI_VARY_RESPONSE structure 
 *      from the point beginning with the state (type STATE).  The
 *      data above that point in the CSI_VARY_RESPONSE structure
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
 *      (int) "csi_xcur_size" - increased by translated size of this part of packet
 *
 * Considerations:
 *      Translation must start at type (type TYPE) since information
 *      above this in the structure has already been translated.
 *
 */

static bool_t 
st_xva_response (
    XDR *xdrsp,        /* xdr handle structure */
    CSI_VARY_RESPONSE *resp        /* vary response structure */
)
{

    register int        i;                      /* loop counter */
    register int        count;                  /* holds count value */
    register int        part_size = 0;          /* size this part of packet */
    register int        total_size = 0;         /* size of packet */
    BOOLEAN             badsize;                /* TRUE if packet size bogus */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xva_response()", 2, (unsigned long) xdrsp,
                 (unsigned long) resp);
#endif /* DEBUG */

    /* translate starting at state */
    part_size = (char*) &resp->type - (char*) &resp->state;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xstate(xdrsp, &resp->state)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
	  "csi_xstate()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate starting at type */
    part_size = (char*) &resp->count - (char*) &resp->type;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xtype(xdrsp, &resp->type)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
	  "csi_xtype()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate count */
    part_size = (char*) &resp->device_status - (char*) &resp->count;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &resp->count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
	  "xdr_u_short()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* check boundary condition before loop */
    if (resp->count > MAX_ID) {
        MLOGCSI((STATUS_COUNT_TOO_LARGE, "st_xva_response()", CSI_NO_CALLEE, 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    total_size = csi_xcur_size;
    /* translate vary response status structures */
    for (i = 0, count = resp->count; i < count; i++, csi_xcur_size += part_size ) {
        switch (resp->type) {
         case TYPE_ACS:
            part_size = (char *)&resp->device_status.acs_status[1] -
                (char *)&resp->device_status.acs_status[0];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate starting at acs_id */
            if (!csi_xacs(xdrsp, &resp->device_status.acs_status[i].acs_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  "csi_xacs()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate component status (type RESPONSE_STATUS) */
            if (!csi_xres_status(xdrsp, 
                &resp->device_status.acs_status[i].status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->device_status.acs_status[resp->count]
                - (char *)resp;
            break;

         case TYPE_PORT:
            part_size = (char *)&resp->device_status.port_status[1] -
                (char *)&resp->device_status.port_status[0];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate starting at port_id */
            if (!csi_xport_id(xdrsp, 
                &resp->device_status.port_status[i].port_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  "csi_xport_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate component status (type RESPONSE_STATUS) */
            if (!csi_xres_status(xdrsp, 
                &resp->device_status.port_status[i].status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->device_status.port_status[resp->count]
                - (char *)resp;
            break;

         case TYPE_CAP:
            part_size = (char *)&resp->device_status.cap_status[1] -
                (char *)&resp->device_status.cap_status[0];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate starting at cap_id */
            if (!csi_xcap_id(xdrsp,&resp->device_status.cap_status[i].cap_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  "csi_xcap_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate component status (type RESPONSE_STATUS) */
            if (!csi_xres_status(xdrsp, 
                &resp->device_status.cap_status[i].status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->device_status.cap_status[resp->count]
                - (char *)resp;
            break;

         case TYPE_LSM:
            part_size = (char *)&resp->device_status.lsm_status[1] -
                (char *)&resp->device_status.lsm_status[0];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate starting at lsm_id */
            if (!csi_xlsm_id(xdrsp,&resp->device_status.lsm_status[i].lsm_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  "csi_xlsm_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate component status (type RESPONSE_STATUS) */
            if (!csi_xres_status(xdrsp, 
                &resp->device_status.lsm_status[i].status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->device_status.lsm_status[resp->count]
                - (char *)resp;
            break;

         case TYPE_DRIVE:
            part_size = (char *)&resp->device_status.drive_status[1] -
                (char *)&resp->device_status.drive_status[0];
            badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
            if (badsize) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  CSI_NO_CALLEE, 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate starting at drive_id */
            if (!csi_xdrive_id(xdrsp,
                &resp->device_status.drive_status[i].drive_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  "csi_xdrive_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            /* translate component status (type RESPONSE_STATUS) */
            if (!csi_xres_status(xdrsp, 
                &resp->device_status.drive_status[i].status)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
		  "csi_xres_status()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            total_size = (char *)&resp->device_status.drive_status[resp->count]
                - (char *)resp;
            break;

         default:
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
	      CSI_NO_CALLEE, 
	      MMSG(977, "Invalid type")));
            return(0);
        }
    }
    /* re-calc size in case of compiler padding on last element */
    csi_xcur_size = total_size;
    return(1);
}

/*
 * Name:
 *      st_xeject_enter()
 *
 * Description:
 *      Routine serializes/deserializes an CSI_EJECT_ENTER structure/packet
 *      from the RESPONSE_STATUS sub-structure, downward.  The routine expects
 *      that the request_header above this point has already been translated
 *      into/from the XDR handle passed.
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
 *      (int) "csi_xcur_size" - increased by translated size of this part of packet
 *
 * Considerations:
 *      (int) "csi_xcur_size" - increased by translated size of this part of packet
 *
 * Module Test Plan:
 *
 */

static bool_t 
st_xeject_enter (
    XDR *xdrsp,         /* xdr handle structure */
    CSI_EJECT_ENTER *resp          /* eject enter structure */
)
{
    register int i;             /* loop counter */
    register int count;         /* holds count value */
    register int part_size;     /* size of a portion of the packet */
    register int total_size;    /* size of the packet */
    BOOLEAN      badsize;       /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xeject_enter", 2, (unsigned long) xdrsp,
                 (unsigned long) resp);
#endif /* DEBUG */

    /* translate the cap_id */

    part_size = (char*) &resp->count - (char*) &resp->cap_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xcap_id(xdrsp, &resp->cap_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xeject_enter()",
	  "csi_xcap_id()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* translate the count */

    part_size = (char*) &resp->volume_status[0] - (char*) &resp->count;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !xdr_u_short(xdrsp, &resp->count)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xeject_enter()",
	  "xdr_u_short()", 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }
    csi_xcur_size += part_size;

    /* check boundary condition before loop */

    if (resp->count > MAX_ID) {
        MLOGCSI((STATUS_COUNT_TOO_LARGE, "st_xeject_enter()", CSI_NO_CALLEE, 
	  MMSG(928, "XDR message translation failure")));
        return(0);
    }

    total_size = csi_xcur_size;
    for (i=0, count = resp->count; i < count; i++) {
        /* translate the volume status */
        part_size = (char*) &resp->volume_status[1]
            - (char*) &resp->volume_status[0];
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || !csi_xvol_status(xdrsp, &resp->volume_status[i])) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xeject_enter()",
	      "csi_xvol_status()", 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += part_size;
        total_size = (char*) &resp->volume_status[resp->count] - (char*)resp;
    }
    /* re-calc size in case of compiler padding on last element */
    csi_xcur_size = total_size;
    return(1);
}
