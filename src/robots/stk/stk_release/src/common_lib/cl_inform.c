static char SccsId[] = "@(#)cl_inform.c	5.4 11/5/93 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_inform
 *
 * Description:
 *
 *      Common routine to construct an unsolicited message based on input
 *      parameters and send it to the ACSSA.  This routine constructs an
 *      unsolicited_message from the input parameters:
 *
 *       o  unsolicited_message packet is zeroed.
 *       o  message_header.packet_id is set to request_id.
 *       o  message_header.command is set to COMMAND_UNSOLICITED_EVENT
 *       o  unsolicited_message.message_status is set to message_status.
 *       o  unsolicited_message.type is set to type.
 *       o  unsolicited_message.identifier is set to identifier.
 *       o  unsolicited_message.error is set to error.
 *       o  unsolicited_message.byte_count is computed and set.
 *
 *      If cl_ipc_send() fails, cl_log_unexpected() is called to attempt to
 *      log the failure to the event_logger.
 *
 * Return Values:
 *
 *      NONE
 *
 * Implicit Inputs:
 *
 *      request_id                      global, set by cl_rp_init().
 *
 * Implicit Outputs:
 *
 *      NONE
 *
 * Considerations:
 *
 *      NONE
 *
 * Module Test Plan:
 *
 *      Verified in inform_test.c.
 *
 * Revision History:
 *
 *      D. F. Reed              17-Oct-1988     Original.
 *
 *      D. F. Reed              08-Aug-1989     Modify to set identifier field
 *          based on status and type.  Allows passing in a pointer to a specific
 *          identifier instead of a union.
 *
 *      J. W. Montgomery        27-Apr-1990     Added TYPE_POOL.
 *
 *      D. A. Beidle            05-Aug-1991     Changed to void function.
 *
 *      J. A. Wishner           19-Oct-1991.    R3.  Add STATUS_DISK_FULL.
 *
 *      D. A. Beidle            25-Nov-1991.    IBR#151 - Log a more informative
 *          on why cl_ipc_send failed.
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "cl_pub.h"
#include "ml_pub.h"
#include "cl_ipc_pub.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */

static UNSOLICITED_MESSAGE  msg_buf;
static char                *self = "cl_inform";

/*
 *      Procedure Type Declarations:
 */


void 
cl_inform (
    STATUS message_status,     /* unsolicited message status */
    TYPE type,                 /* denotes type of identifier */
    IDENTIFIER *identifier,    /* identifier of component involved */
    int error                  /* internal error code */
)
{
    STATUS      ret;

#ifdef DEBUG
    if TRACE(0)
        cl_trace(self, 4,               /* routine name and parameter count */
                 (unsigned long)message_status,
                 (unsigned long)type,
                 (unsigned long)identifier,
                 (unsigned long)error);
#endif /* DEBUG */


    /* clear packet buffer */
    memset((char *)&msg_buf, 0, sizeof(msg_buf));

    /* fill in message fields */
    msg_buf.message_header.packet_id = request_id;
    msg_buf.message_header.command = COMMAND_UNSOLICITED_EVENT;
    msg_buf.message_status.status = message_status;
    msg_buf.message_status.type = type;

    if (identifier != (IDENTIFIER *)0) {

        /* check status */
        switch (message_status) {

         case STATUS_DEGRADED_MODE:
         case STATUS_LIBRARY_FAILURE:

            /* identifier is actually of LH_ADDR.address type */
	    /* NOTE: sizeof construct is a preprocessor directive and the
		     following indirection off of NULL is a legal construct */
            memcpy((char *)&msg_buf.message_status.identifier,
	           (char *)identifier, sizeof(((LH_ADDR *)NULL)->address));
            break;

         case STATUS_DISK_FULL:
 
            strncpy(msg_buf.message_status.identifier.alignment_size,
                identifier->alignment_size, sizeof(identifier->alignment_size));
            break;


         default:

            /* check type for the rest */
            switch (type) {
             case TYPE_ACS:
                msg_buf.message_status.identifier.acs_id = identifier->acs_id;
                break;

             case TYPE_CAP:
                msg_buf.message_status.identifier.cap_id = identifier->cap_id;
                break;

             case TYPE_CELL:
                msg_buf.message_status.identifier.cell_id = identifier->cell_id;
                break;

             case TYPE_DRIVE:
                msg_buf.message_status.identifier.drive_id =
                    identifier->drive_id;
                break;

             case TYPE_LSM:
                msg_buf.message_status.identifier.lsm_id = identifier->lsm_id;
                break;

             case TYPE_PANEL:
                msg_buf.message_status.identifier.panel_id =
                    identifier->panel_id;
                break;

             case TYPE_POOL:
                msg_buf.message_status.identifier.pool_id = identifier->pool_id;
                break;

             case TYPE_PORT:
                msg_buf.message_status.identifier.port_id = identifier->port_id;
                break;

             case TYPE_SUBPANEL:
                msg_buf.message_status.identifier.subpanel_id =
                    identifier->subpanel_id;
                break;

             case TYPE_VOLUME:
                msg_buf.message_status.identifier.vol_id = identifier->vol_id;
                break;

             case TYPE_IPC:
                memcpy(msg_buf.message_status.identifier.socket_name,
                       identifier->socket_name, SOCKET_NAME_SIZE);
                break;

             case TYPE_REQUEST:
                msg_buf.message_status.identifier.request = identifier->request;
                break;

             default:

                /* any other type is unexpected here */
                MLOG((MMSG(147,"%s: Unexpected type %s detected"), self,  cl_type(type)));
                return;
            }
        }
    }

    msg_buf.error = error;


    /* ok, report it! */
    ret = cl_ipc_send(ACSSA, (char *)&msg_buf, sizeof(msg_buf), MAX_RETRY / 2);

    /* log an event on error */
    if (ret != STATUS_SUCCESS) {
        MLOG((MMSG(123,"%s: Sending message to socket %s failed on \"%s\""), self,  ACSSA, strerror(errno)));
    }

    return;
}
