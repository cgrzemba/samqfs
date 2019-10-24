#ifndef lint
static char SccsId[] = "@(#) %full_name:	server/csrc/cvt_v1_v0/2.0A %";
#endif

/*
 * Copyright (1990, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *  lm_cvt_v1_v0
 *
 * Description:
 *
 *   General:
 *
 *      This module converts a VERSION1 response packet to a VERSION0 response
 *      packet, then returns the converted packet with the correct byte count.
 *
 *   Specific:
 *
 *        o Copy the data from the VERSION1 response packet's IPC_HEADER into
 *          the VERSION0 response packet's IPC_HEADER.
 *
 *        o Create a VERSION0 MESSAGE_HEADER in the resulting response pkt by:
 *            * copying all VERSION0 message header information from the
 *              VERSION1 response packet to the VERSION0 response packet.
 *
 *            * clear the extended bit. (just a sanity set, it should already
 *              be clear)
 *
 *        o Copy the data from the original VERSION1 response packet's 
 *          RESPONSE_STATUS into the resulting VERSION0 response packet's
 *          RESPONSE_STATUS.
 *
 *        o VERSION0 packets don't know about message_header.type of TYPE_LH.
 *          If this is encountered, for all commands, replace the type with
 *          TYPE_NONE in VERSION0 packet. 
 *
 *        o VERSION0 packets don't know about identifier status types of
 *          TYPE_LH.  Vary  is the only response which can have TYPE_LH set
 *          as the identifier's type.  This MUST be converted to TYPE_NONE
 *          for the VERSION0 csi to translate correcty.
 *
 *        o Copy the remainder of the response packet to the resulting 
 *          response packet.
 *
 *        o Update the byte count (input parameter) to be correct.
 *
 * Return Values:
 *
 *      STATUS_SUCCESS
 *      STATUS_PROCESS_FAILURE
 *
 * Implicit Inputs:
 *
 *      None
 *
 * Implicit Outputs:
 *
 *      None
 *
 * Considerations:
 *
 *      None
 *
 * Module Test Plan:
 *
 *      None
 *
 * Revision History:
 *
 *      H. I. Grapek        16-Jun-1993.    Original.
 *      H. I. Grapek        03-Oct-1993     Re-written for release 3
 *      H. I. Grapek        25-Oct-1993     Made start, idle and cancel work.
 *      D. A. Beidle        30-Apr-1993.    BR#51993 Add missing fields to
 *              converted VERSION0 packets: cancel, dismount, mount.
 *      H. I. Grapek        23-Jul-1992     R4.0 BR#519 Made start, idle work 
 *		through the CSI
 *	H. I. Grapek	    31-Mar-1993	    R4.0 BR#433 Added audit 
 *		intermediates.
 *	D. B. Farmer	    09-Aug-1993	    Changes from R4 Bull port
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <stdio.h>
#include <string.h>
#include "cl_pub.h"
#include "ml_pub.h"
#include "acslm.h"
#include "v0_structs.h"
#include "v1_structs.h"

/*
 *  Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static char             module_string[] = "lm_cvt_v1_v0";
static ALIGNED_BYTES    result_bfr[MAX_MESSAGE_BLOCK];    /* results */

STATUS 
lm_cvt_v1_v0 (
    char *response_ptr,                  /* acslm input buffer area */
    int *byte_count                    /* MODIFIED byte count */
)
{
    V0_RESPONSE_TYPE    *v0_res_ptr;    /* v0 pointer to response type msg's */
    V1_RESPONSE_TYPE    *v1_res_ptr;    /* V1 pointer to response type msg's */
    MESSAGE_HEADER      *v1_mhp;        /* ptr to MESSAGE_HEADER fields */
    V0_MESSAGE_HEADER   *v0_mhp;        /* ptr to v0 MESSAGE_HEADER fields */
    V0_AUDIT_RESPONSE   *v0_aup;        /* version 0 audit RESPONSE */
    V1_AUDIT_RESPONSE   *v1_aup;        /* version 1 audit RESPONSE */
    V0_EJECT_ENTER      *v0_irp;        /* version 0 intermed audit RESPONSE */
    V1_EJECT_ENTER      *v1_irp;        /* VERSION1 intermed. AUDIT RESPONSE */
    V0_CANCEL_RESPONSE  *v0_canp;       /* version 0 cancel RESPONSE */
    V1_CANCEL_RESPONSE  *v1_canp;       /* version 1 cancel RESPONSE */
    V0_DISMOUNT_RESPONSE *v0_dmp;       /* version 0 dismount RESPONSE */
    V1_DISMOUNT_RESPONSE *v1_dmp;       /* version 1 dismount RESPONSE */
    V0_EJECT_RESPONSE   *v0_ejp;        /* version 0 eject RESPONSE */
    V1_EJECT_RESPONSE   *v1_ejp;        /* version 1 eject RESPONSE */
    V0_ENTER_RESPONSE   *v0_enp;        /* version 0 enter RESPONSE */
    V1_ENTER_RESPONSE   *v1_enp;        /* version 1 enter RESPONSE */
    V0_MOUNT_RESPONSE   *v0_mtp;        /* version 0 mount RESPONSE */
    V1_MOUNT_RESPONSE   *v1_mtp;        /* version 1 mount RESPONSE */
    V0_QUERY_RESPONSE   *v0_qp;         /* version 0 query RESPONSE */
    V1_QUERY_RESPONSE   *v1_qp;         /* version 1 query RESPONSE */
    V0_VARY_RESPONSE    *v0_vp;         /* version 0 vary RESPONSE */
    V1_VARY_RESPONSE    *v1_vp;         /* version 1 vary RESPONSE */

    char                *result_ptr;    /* converted packet. */
    char                *from_ident;    /* misc pointer */
    char                *to_ident;      /* misc pointer */
    int                 copy_size = 0;  /* number of bytes to copy */
    int                 cnt;            /* count of identifier in response */


#ifdef DEBUG
    if TRACE (0)
        cl_trace(module_string, 2, (unsigned long) response_ptr,
                (unsigned long) byte_count);
#endif

    /* Check the sanity of incomming request */
    CL_ASSERT(module_string, ((response_ptr) && (byte_count))); 

    MLOGDEBUG(0,(MMSG(864, "%s: here... byte_count %d"), module_string, *byte_count));

    /* zero the result buffer out */
    memset((char *) result_bfr, 0, sizeof(result_bfr));

    /* set up some generic pointers */
    result_ptr = (char *) result_bfr;                   /* result's data area */
    v0_res_ptr = (V0_RESPONSE_TYPE *) result_ptr;       /* result pointer */
    v1_res_ptr = (V1_RESPONSE_TYPE *) response_ptr;     /* incomming packet */


    /* copy ipc header (same between versions) */
    v0_res_ptr->generic_response.request_header.ipc_header =
        v1_res_ptr->generic_response.request_header.ipc_header;

    /* copy message header */
    v1_mhp = &(v1_res_ptr->generic_response.request_header.message_header);
    v0_mhp = &(v0_res_ptr->generic_response.request_header.message_header); 

    v0_mhp->packet_id           = v1_mhp->packet_id;
    v0_mhp->command             = v1_mhp->command;
    v0_mhp->message_options     = v1_mhp->message_options;

    /* copy response status (same between versions) */
    v0_res_ptr->generic_response.response_status =
         v1_res_ptr->generic_response.response_status;

    /*
     * Version 0 packets don't know about identifier status's of TYPE_LH.
     * Today, commands AUDIT, MOUNT, DISMOUNT, ENTER, EJECT, and VARY all deal 
     * with the ACSLH.  VARY, however, is the only one which can have TYPE_LH 
     * set in the identifier's type.  This MUST be converted to TYPE_NONE for 
     * the VERSION0 csi to translate correctly.  Soooo, change message_status 
     * TYPE_LH to TYPE_NONE 
     */ 
    if (v0_res_ptr->generic_response.response_status.type == TYPE_LH) {
        v0_res_ptr->generic_response.response_status.type = TYPE_NONE;
    }

    /* 
     * If the response is an ACK, simply set up the size to be the size of
     * a V0 ACK packet, copy in the data, and return
     */
    if (v0_mhp->message_options & ACKNOWLEDGE) {
        *byte_count = sizeof(V0_ACKNOWLEDGE_RESPONSE); 
    	/* copy in the new V0 Packet. */
    	memcpy(response_ptr, result_ptr, *byte_count);

    	MLOGDEBUG(0,(MMSG(865, "%s: got ACK, leaving... byte_count %d"), 
		module_string, *byte_count));

        return (STATUS_SUCCESS);
    }

    /*
     * Command is either an intermediate or a final, deal with it on a 
     * command by command basis.
     */
    switch(v1_mhp->command) {
        case COMMAND_AUDIT:
	    if (v1_mhp->message_options & INTERMEDIATE) {
		/* INTERMEDIATE RESPONSE */
                v0_irp = &(v0_res_ptr->eject_response);
                v1_irp = &(v1_res_ptr->eject_response);

		MLOGDEBUG(0,(MMSG(866,
		  "%s: AUDIT INTER: CAPID: (%d,%d), COUNT: %d, Version: %d\n"),
		  module_string, v1_irp->cap_id.acs, v1_irp->cap_id.lsm,
		  v1_irp->count, v1_mhp->version));

                /* copy fixed portion */
                v0_irp->cap_id = v1_irp->cap_id;
                v0_irp->count  = v1_irp->count;
    
                /* copy the variable portion of the pachet */
                from_ident = (char *) v1_irp->volume_status;
                to_ident   = (char *) v0_irp->volume_status;
                copy_size  = (*byte_count - (from_ident - (char *)v1_irp));
		if (copy_size < 0)
		    copy_size = 0;
                memcpy(to_ident, from_ident, copy_size);
    
                /* calculate new byte count */
                *byte_count = (copy_size + (to_ident - (char *)v0_irp));
	    } else {
		/* FINAL RESPONSE */
                v0_aup = &(v0_res_ptr->audit_response);
                v1_aup = &(v1_res_ptr->audit_response);

		MLOGDEBUG(0,(MMSG(867,
		  "%s: AUDIT FINAL: CAPID: (%d,%d), TYPE: %s, COUNT: %d\n"),
		  module_string, v1_aup->cap_id.acs, v1_aup->cap_id.lsm, 
		  cl_type(v1_aup->type), v1_aup->count));

                /* copy fixed portion */
                v0_aup->cap_id = v1_aup->cap_id;
                v0_aup->type   = v1_aup->type;
                v0_aup->count  = v1_aup->count;
    
                /* copy the variable portion of the pachet */
                from_ident = (char *) &v1_aup->identifier_status;
                to_ident   = (char *) &v0_aup->identifier_status;
                copy_size  = (*byte_count - (from_ident - (char *)v1_aup));
		if (copy_size < 0)
		    copy_size = 0;
                memcpy(to_ident, from_ident, copy_size);
    
                /* calculate new byte count */
                *byte_count = (copy_size + (to_ident - (char *)v0_aup));
            } 
            break;

        case COMMAND_CANCEL:
            v0_canp = &(v0_res_ptr->cancel_response);
            v1_canp = &(v1_res_ptr->cancel_response);

            /* copy message identifier */
            v0_canp->request = v1_canp->request;

            /* calculate new byte count */
            *byte_count = sizeof(V0_CANCEL_RESPONSE);

            break;

        case COMMAND_DISMOUNT:
            v0_dmp = &(v0_res_ptr->dismount_response);
            v1_dmp = &(v1_res_ptr->dismount_response);

            /* copy volume and drive IDs */
            v0_dmp->vol_id = v1_dmp->vol_id;
            v0_dmp->drive_id = v1_dmp->drive_id;

            /* calculate new byte count */
            *byte_count = sizeof(V0_DISMOUNT_RESPONSE);

            break;

        case COMMAND_ENTER:
            v0_enp = &(v0_res_ptr->enter_response);
            v1_enp = &(v1_res_ptr->enter_response);

            /* copy the fixed portion */
            v0_enp->cap_id = v1_enp->cap_id;
            v0_enp->count  = v1_enp->count;

            /* copy the variable portion */
            for (cnt = 0; cnt < (int)v1_enp->count; ++cnt) {
                v0_enp->volume_status[cnt].vol_id =
                        v1_enp->volume_status[cnt].vol_id;
                if (v0_enp->volume_status[cnt].status.type == TYPE_CAP) {
                    v0_enp->volume_status[cnt].status.status =
                        v1_enp->volume_status[cnt].status.status;
                    v0_enp->volume_status[cnt].status.type =
                        v1_enp->volume_status[cnt].status.type;
                    v0_enp->volume_status[cnt].status.identifier.cap_id =
                        v1_enp->volume_status[cnt].status.identifier.cap_id;
                }
                else {
                    v0_enp->volume_status[cnt].status =
                        v1_enp->volume_status[cnt].status;
                }
            }    

            from_ident = (char *) &v1_enp->volume_status[v1_enp->count];
            to_ident   = (char *) &v0_enp->volume_status[v0_enp->count];
            copy_size  = (*byte_count - (from_ident - (char *)v1_enp));

            /* calculate new byte count */
            *byte_count = (copy_size + (to_ident - (char *)v0_enp));

            break;

        case COMMAND_EJECT:
            v0_ejp = &(v0_res_ptr->eject_response);
            v1_ejp = &(v1_res_ptr->eject_response);

            /* copy the fixed portion */
            v0_ejp->cap_id = v1_ejp->cap_id;
            v0_ejp->count  = v1_ejp->count;

            /* copy the variable portion */
            for (cnt = 0; cnt < (int)v1_ejp->count; ++cnt) {
                v0_ejp->volume_status[cnt].vol_id =
                        v1_ejp->volume_status[cnt].vol_id;
                if (v0_ejp->volume_status[cnt].status.type == TYPE_CAP) {
                    v0_ejp->volume_status[cnt].status.status =
                        v1_ejp->volume_status[cnt].status.status;
                    v0_ejp->volume_status[cnt].status.type =
                        v1_ejp->volume_status[cnt].status.type;
                    v0_ejp->volume_status[cnt].status.identifier.cap_id =
                        v1_ejp->volume_status[cnt].status.identifier.cap_id;
                }
                else {
                    v0_ejp->volume_status[cnt].status =
                        v1_ejp->volume_status[cnt].status;
                }
            }    

            from_ident = (char *) &v1_ejp->volume_status[v1_ejp->count];
            to_ident   = (char *) &v0_ejp->volume_status[v0_ejp->count];
            copy_size  = (*byte_count - (from_ident - (char *)v1_ejp));

            /* calculate new byte count */
            *byte_count = (copy_size + (to_ident - (char *)v0_ejp));

            break;

        case COMMAND_IDLE:
            *byte_count = sizeof(V0_IDLE_RESPONSE); 
            break;

        case COMMAND_START:
            *byte_count = sizeof(V0_START_RESPONSE); 
            break;

        case COMMAND_MOUNT:
            v0_mtp = &(v0_res_ptr->mount_response);
            v1_mtp = &(v1_res_ptr->mount_response);

            /* copy volume and drive IDs */
            v0_mtp->vol_id = v1_mtp->vol_id;
            v0_mtp->drive_id = v1_mtp->drive_id;

            /* calculate new byte count */
            *byte_count = sizeof(V0_MOUNT_RESPONSE);

            break;

        case COMMAND_QUERY:
            /* has cap_id in variable portion (status) of the packet. */
            v0_qp = &(v0_res_ptr->query_response);
            v1_qp = &(v1_res_ptr->query_response);

            /* copy the fixed portion */
            v0_qp->type  = v1_qp->type;
            v0_qp->count = v1_qp->count;

            /* copy the variable portion of the packet */
            from_ident = (char *) &v1_qp->status_response;
            to_ident   = (char *) &v0_qp->status_response;
            copy_size  = (*byte_count - (from_ident - (char *)v1_qp));

            if (v1_qp->type == TYPE_CAP) {
                for (cnt = 0; cnt < (int)v1_qp->count; ++cnt) {
                    v0_qp->status_response.cap_status[cnt].cap_id =
                        v1_qp->status_response.cap_status[cnt].cap_id;
                    v0_qp->status_response.cap_status[cnt].status =
                        v1_qp->status_response.cap_status[cnt].status;
                }
            }
            else {
                from_ident = (char *) &v1_qp->status_response;
                to_ident   = (char *) &v0_qp->status_response;
                copy_size  = (*byte_count - (from_ident - (char *)v1_qp));
		if (copy_size < 0)
		    copy_size = 0;
                memcpy(to_ident, from_ident, copy_size);

            }

            /* calculate new byte count */
            *byte_count = (copy_size + (to_ident - (char *)v0_qp));

            break;

        case COMMAND_VARY:
            v0_vp = &(v0_res_ptr->vary_response);
            v1_vp = &(v1_res_ptr->vary_response);

            /* copy the fixed portion */
            v0_vp->state = v1_vp->state;
            v0_vp->type  = v1_vp->type;
            v0_vp->count = v1_vp->count;
 
            /* copy the variable portion */
            from_ident = (char *) &v1_vp->device_status;
            to_ident   = (char *) &v0_vp->device_status;
            copy_size  = (*byte_count - (from_ident - (char *)v1_vp));
	    if (copy_size < 0)
		copy_size = 0;
            memcpy(to_ident, from_ident, copy_size);

            /* calculate new byte count */
            *byte_count = (copy_size + (to_ident - (char *)v0_vp));
 
            /* check the identifier's type */
            switch (v0_vp->type) {
                case TYPE_ACS:
                    for (cnt = 0; cnt < (int)v0_vp->count; ++cnt) 
                    if (v0_vp->device_status.acs_status[cnt].status.type == 
                        TYPE_LH)
                        v0_vp->device_status.acs_status[cnt].status.type = 
                            TYPE_NONE;
                    break; 

                case TYPE_DRIVE:
                    for (cnt = 0; cnt < (int)v0_vp->count; ++cnt) 
                        if (v0_vp->device_status.drive_status[cnt].status.type 
                            == TYPE_LH)
                            v0_vp->device_status.drive_status[cnt].status.type 
                            = TYPE_NONE;
                    break; 

                case TYPE_LSM:
                    for (cnt = 0; cnt < (int)v0_vp->count; ++cnt) 
                        if (v0_vp->device_status.lsm_status[cnt].status.type ==
                             TYPE_LH)
                            v0_vp->device_status.lsm_status[cnt].status.type = 
                                TYPE_NONE;
                    break; 

                case TYPE_PORT:
                    for (cnt = 0; cnt < (int)v0_vp->count; ++cnt) 
                        if (v0_vp->device_status.port_status[cnt].status.type 
                            == TYPE_LH)
                            v0_vp->device_status.port_status[cnt].status.type 
                                = TYPE_NONE;
                    break;
		    
		default:
		    break;
            }

            break;

        default:
            MLOG((MMSG(136,"%s: Unexpected command %s detected"), module_string,  
                                    cl_command(v1_mhp->command)));
            return (STATUS_PROCESS_FAILURE);
    }

    /* copy in the new V0 Packet. */
    memcpy (response_ptr, result_ptr, *byte_count);

    MLOGDEBUG(0,(MMSG(868, "%s: leaving... byte_count %d"), module_string, *byte_count));

    return (STATUS_SUCCESS);
}
