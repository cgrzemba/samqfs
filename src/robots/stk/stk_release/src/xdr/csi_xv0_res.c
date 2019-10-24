#ifndef lint
static char SccsId[] = "@(#)csi_xv0_res.c	5.4 11/11/93 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_xv0_res()
 *
 * Description:
 *      Encode/Decode Version 0 responses.
 *
 *
 * Return Values:
 *      bool_t          - 1 successful xdr conversion of at least request header
 *      bool_t          - 0 xdr conversion failed
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
 *      The value of packet size returned is undefined when 0 return code
 *      (error) is returned.
 *
 * Module Test Plan:
 *      See CSI Unit Test Plan
 *
 * Revision History
 *      J. A. Wishner       10-Jan-1989.    Created.
 *      J. A. Wishner       10-Aug-1989.    Portability.  Changed size
 *                                          calculations for data structures
 *                                          so portable to other machines, such
 *                                          as SPARC.
 *      J. A. Wishner       08-Sep-1989.    Portability.  Change size calulation
 *                                          for CSI_ACKNOWLEDGE_RESPONSE.
 *      J. W. Montgomery    15-Jun-1990.    Version 2.
 *      J. A. Wishner       20-Oct-1991.    Delete count, i, rtype_mask; unused.
 *      Mitch Black     07-Dec-2004     Prevent Linux warnings by putting in 
 *                                      correct declarations for static functions
 *                                      before they are called.
 */


/*      Header Files: */
#include "csi.h"
#include <csi_xdr_xlate.h>
#include "ml_pub.h"
 
/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static char     *st_src = __FILE__;
static char     *st_module = "csi_xv0_res()";
 
/*      Procedure Type Declarations: */
static bool_t st_xau_response (XDR *, CSI_V0_AUDIT_RESPONSE *);
static bool_t st_xva_response (XDR *, CSI_V0_VARY_RESPONSE *);
static bool_t st_xeject_enter (XDR *, CSI_V0_EJECT_ENTER *);

bool_t 
csi_xv0_res (
    XDR *xdrsp,
    CSI_V0_RESPONSE *resp
)
{
    register int    part_size = 0;
    BOOLEAN         badsize;

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
		  "csi_xv0_cap_id()", 
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
        csi_xcur_size = sizeof(CSI_V0_CANCEL_RESPONSE);
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
        csi_xcur_size = sizeof(CSI_V0_DISMOUNT_RESPONSE);
        break;
    case COMMAND_EJECT:
        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_eject_res.cap_id
                      - (char*) &resp->csi_eject_res;
        /* translate eject response (same as V0_EJECT_ENTER) */
        if (!st_xeject_enter(xdrsp, &resp->csi_eject_enter_res)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xv0_cap_id()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }  
        break;
    case COMMAND_ENTER:
        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_enter_res.cap_id
                      - (char*) &resp->csi_enter_res;
        /* translate enter response (same as V0_EJECT_ENTER) */
        if (!st_xeject_enter(xdrsp, &resp->csi_eject_enter_res)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xv0_cap_id()",
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }  
        break;
    case COMMAND_IDLE:
        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = sizeof(CSI_V0_IDLE_RESPONSE);
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
            MLOGCSI((STATUS_TRANSLATION_FAILURE,  st_module, "csi_xvol_id()",
		MMSG(928, "XDR message translation failure")));
            return 0;
        }
        csi_xcur_size += part_size;
        /* translate starting at driveid */
        part_size = sizeof(DRIVEID);
        badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
        if (badsize || 
            !csi_xdrive_id(xdrsp, &resp->csi_mount_res.drive_id)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "csi_xdrive_id()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        /* re-size in case there is compiler tail padding on struture */
        csi_xcur_size = sizeof(CSI_V0_MOUNT_RESPONSE);
        break;
    case COMMAND_QUERY:
        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_query_res.type
                      - (char*) &resp->csi_query_res;
        /* translate the rest of the query packet */
        if (!csi_xv0quresponse(xdrsp, &resp->csi_query_res)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
	      "csi_xv0quresponse()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        break;
    case COMMAND_START:
        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = sizeof(CSI_V0_START_RESPONSE);
        /* nothing more to do */
        break;
    case COMMAND_VARY:
        /* portability: size end of response header for compiler padding*/
        csi_xcur_size = (char*) &resp->csi_vary_res.state
                      - (char*) &resp->csi_vary_res;
        /* translate the rest of the vary packet */
        if (!st_xva_response(xdrsp, &resp->csi_vary_res)) {
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
	      "st_xva_response()", 
	      MMSG(928, "XDR message translation failure")));
            return 0;
        }
        break;
    default:
        /* portability: size end of response header for compiler padding*/
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
 *      st_xau_response()
 *
 * Description:
 *      Routine serializes/deserializes a partly translated CSI_AUDIT_RESPONSE 
 *      structure from the positiion of the CAPID, onward, downstream.
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
    XDR *xdrsp,   /* xdr handle structure */
    CSI_V0_AUDIT_RESPONSE *resp    /* start of partly translated audit response */
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
    if (badsize || !csi_xv0_cap_id(xdrsp, &resp->cap_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xau_response()",
	  "csi_xv0_cap_id()", 
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
    for (i=0, count = resp->count; i < count; i++, csi_xcur_size += part_size) {
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
    /* re-size in case there is compiler tail padding on struture */
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
 *      (int) "csi_xcur_size" - increased by translated size this part of packet
 *
 * Considerations:
 *      Translation must start at type (type TYPE) since information
 *      above this in the structure has already been translated.
 *
 */

static bool_t 
st_xva_response (
    XDR *xdrsp,        /* xdr handle structure */
    CSI_V0_VARY_RESPONSE *resp     /* vary response structure */
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
    for (i=0, count=resp->count; i < count; i++, csi_xcur_size += part_size ) {
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
            total_size = (char *)&resp->device_status.acs_status[resp->count] -
                (char *) resp;
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
                - (char *) resp;
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
                - (char *) resp;
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
                - (char *) resp;
            break;
         default:
            MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xva_response()",
	      CSI_NO_CALLEE, 
	      MMSG(977, "Invalid type")));
            return(0);
        }
    }
    /* re-size in case there is compiler tail padding on struture */
    csi_xcur_size = total_size;
    return(1);
}

/*
 * Name:
 *      st_xeject_enter()
 *
 * Description:
 *      Routine serializes/deserializes an V0_EJECT_ENTER structure/packet from
 *      the RESPONSE_STATUS sub-structure, downward.  The routine expects
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
 *      (int) "csi_xcur_size" - increased by translated size this part of packet
 *
 * Considerations:
 *      (int) "csi_xcur_size" - increased by translated size this part of packet
 *
 * Module Test Plan:
 *
 */

static bool_t 
st_xeject_enter (
    XDR *xdrsp,         /* xdr handle structure */
    CSI_V0_EJECT_ENTER *resp               /* eject enter structure */
)
{
    register int i;             /* loop counter */
    register int count;         /* holds count value */
    register int part_size;     /* size of a portion of the packet */
    register int total_size;    /* size the packet */
    BOOLEAN      badsize;       /* TRUE if packet size invalid */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace("st_xeject_enter", 2, (unsigned long) xdrsp,
                 (unsigned long) resp);
#endif /* DEBUG */

    /* translate the cap_id */
    part_size = (char*) &resp->count - (char*) &resp->cap_id;
    badsize   = CHECKSIZE(csi_xcur_size, part_size, csi_xexp_size);
    if (badsize || !csi_xv0_cap_id(xdrsp, &resp->cap_id)) {
        MLOGCSI((STATUS_TRANSLATION_FAILURE, "st_xeject_enter()",
	  "csi_xv0_cap_id()", 
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
            MLOGCSI((STATUS_TRANSLATION_FAILURE,  "st_xeject_enter()",
	      "csi_xvol_status()", 
	      MMSG(928, "XDR message translation failure")));
            return(0);
        }
        csi_xcur_size += part_size;
        total_size = (char*) &resp->volume_status[resp->count] - (char*) resp;
    }
    /* re-size in case there is compiler tail padding on struture */
    csi_xcur_size = total_size;
    return(1);
}
