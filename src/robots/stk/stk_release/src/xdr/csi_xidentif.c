#ifndef lint
static char SccsId[] = "@(#)csi_xidentif.c	5.6 2/13/02 ";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      csi_xidentifier()
 *
 * Description:
 *      Routine serializes/deserializes an identifier structure.
 *      Note:  The identifier structure is a true union, and the
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
 *      size of the individual identifier.  Use "alignment_bytes" variable
 *      of identifier as the designator for the total number of bytes needing
 *      translation.
 *
 *      The global variable csi_active_xdr_version_branch, which equals one of
 *      VERSION0, VERSION1, VERSION2, VERSION3 or VERSION4 is used to discern
 *	between the various types of cap identifiers.  This is necessary because
 *      there is no type discriminator between cap-ids of different versions,
 *      they are all just TYPE_CAP.  Invention of a new type code would not
 *      solve the problem since it would be of a value not defined for client
 *      systems which are using older versions of xdr code...and the reason
 *      that this hack global was installed was to enable backward compatibility
 *      where the server could be at a higher release level than a client.
 *
 * Module Test Plan:
 *
 * Revision History:
 *      J. A. Wishner       01-Feb-1989.    Created.
 *      J. A. Wishner       28-Jul-1989.    Portability.  Use of xdr_opaque
 *                                          removed & replaced with xdr_bytes().
 *                                          Uses alignment_size in identifier
 *                                          to calculate number of bytes to
 *                                          encode/decode.
 *      J. A. Wishner       11-Aug-1989.    Bugfix.  Added port_id translation.
 *      J. A. Wishner       12-Aug-1989.    Portability.  Volume id must be
 *                                          translated as external label size.
 *      J. W. Montgomery    23-Aug-1990.    Added TYPE_POOL and TYPE_LH.
 *      J. A. Wishner       09-Oct-1991.    Note kludge release 3 (version 2).
 *                                          Allowing all three versions of the
 *                                          cap identifier to get translated:
 *                                          .....V0_CAP_ID, V1_CAP_ID, CAP_ID
 *                                          as if they were all the larger
 *                                          version 2 cap identifier, CAP_ID.
 *                                          This means that for version 0 and
 *                                          version 1 an extra byte will be
 *                                          translated:  the one that
 *                                          corresponds to a version 2 cap
 *                                          number.  This is done to simplify
 *                                          the code.  It works because we
 *                                          always put the full size of the
 *                                          identifier union on the wire.
 *                                          See the note below in the code
 *                                          about how we always translate the
 *                                          trailing bytes of an identifier.
 *      J. A. Wishner       01-Feb-1989.    Created.
 *      E. A. Alongi        29-Jul-1992     added a case label for Version 3
 *                                          cap translation.
 *      E. A. Alongi        26-Oct-1992     Changes to decode trailing bytes
 *                                          so as to move the xdrsp pointer
 *                                          appropriately.
 *      E. A. Alongi        03-Jun-1993     added case label for Version 4 cap
 *					    identifier translation.
 *	E. A. Alongi	    03-Mar-1994     Added code to handle a media type
 *					    identifier (TYPE_MEDIA_TYPE).  The
 *					    (invalid) media type identifier is
 *					    returned in the RESPONSE_STATUS as
 *					    a result of specifing an invalid
 *					    media type in a query mount scratch
 *					    request.  Added default for switch
 *					    on csi_active_xdr_version_branch
 *					    (version number) under TYPE_CAP.
 *      C. J. Higgins       18-Oct-2001     Added code to handle a new identifr
 *                                          of type HANDID.
 *      S. L. Siao       28-Nov-2001     Added code to handle TYPE_SERVER
 *      S. L. Siao       05-Dec-2001     Added code to handle TYPE_PTP
 *      S. L. Siao       13-Feb-2002     Added code to handle TYPE_VTDID, 
 *						TYPE_SUBPOOL_NAME, TYPE_DRIVE_GROUP.
 *      Wipro (Subhash)  03-Jun-2004     Modified to handle 
 *                                       TYPE_MOUNT and TYPE_DISMOUNT
 *      Mitch Black      29-Nov-2004     Changed to use csi_xptp_id.
 *      Mitch Black      30-Nov-2004     Changed message to say "csi_xptp_id".
 */


/*      Header Files: */
#include <string.h>
#include "csi.h"
#include "ml_pub.h"
#include "cl_pub.h"
#include "csi_xdr_pri.h"
 
/*      Defines, Typedefs and Structure Definitions: */

/*      Global and Static Variable Declarations: */
static char     *st_module = "csi_xidentifier()";
 
/*      Procedure Type Declarations: */

bool_t 
csi_xidentifier (
    XDR *xdrsp,    /* xdr handle structure */
    IDENTIFIER *identifp, /* identifier structure */
    TYPE type     /* type of identifier (discriminant of xdr union) */
)
{
    unsigned int          cursize = 0;     /* current size translated */
    char        *cp;                       /* string ptr for xdr_bytes () */
    char        *sp;                       /* ptr to ptr for xdr_string */

#ifdef DEBUG
    if TRACE(CSI_XDR_TRACE_LEVEL)
        cl_trace(st_module, 2, (unsigned long) xdrsp, (unsigned long) identifp,
                 (unsigned long) type);
#endif /* DEBUG */

    /* handle each part of the union as a distinct structure */
    switch(type) {
        case TYPE_ACS:
            cursize = sizeof(identifp->acs_id);
            if (!csi_xacs(xdrsp, &identifp->acs_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "csi_xacs()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_CAP:

            /*
             *  translate different cap versions - see considerations 
             */

            switch (csi_active_xdr_version_branch) {
                
                case VERSION0:

                    cursize = sizeof(identifp->v0_cap_id);
                    if (!csi_xv0_cap_id(xdrsp, &identifp->v0_cap_id)) {
                        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
			  "csi_xv0_cap_id()", 
			  MMSG(928, "XDR message translation failure")));
                        return(0);
                    }
                    break;

                case VERSION1:

                    cursize = sizeof(identifp->v1_cap_id);
                    if (!csi_xv1_cap_id(xdrsp, &identifp->v1_cap_id)) {
                        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
			  "csi_xv1_cap_id()", 
			  MMSG(928, "XDR message translation failure")));
                        return(0);
                    }
                    break;

                case VERSION2:
                case VERSION3:
                case VERSION4:

                    cursize = sizeof(identifp->cap_id);
                    if (!csi_xcap_id(xdrsp, &identifp->cap_id)) {
                        MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
			  "csi_xcap_id()", 
			  MMSG(928, "XDR message translation failure")));
                        return(0);
                    }
                    break;

		default:
		    /* log invalid version number as translation failure. This
		     * shouldn't happen since the assignment of version number
		     * to csi_active_xdr_version_branch takes place in 
		     * csi_xlm_response() and invalid version numbers are
		     * detected there. 
		     */
		    MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, 
			  CSI_NO_CALLEE, MMSG(924, "Invalid version number %d"),
			  (int) csi_active_xdr_version_branch ));
												    return (0);

            } /* end of switch on xdr code branch */
            break;

        case TYPE_CELL:
            cursize = sizeof(identifp->cell_id);
            if (!csi_xcell_id(xdrsp, &identifp->cell_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xcell_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_DRIVE:
            cursize = sizeof(identifp->drive_id);
            if (!csi_xdrive_id(xdrsp, &identifp->drive_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xdrive_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_POOL:
            cursize = sizeof(identifp->pool_id);
            if (!csi_xpool_id(xdrsp, &identifp->pool_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xpool_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_PORT:
            cursize = sizeof(identifp->port_id);
            if (!csi_xport_id(xdrsp, &identifp->port_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xport_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_LH:
            cursize = sizeof(identifp->lh_error);
            if (!xdr_short(xdrsp, &identifp->lh_error)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module, "xdr_short()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_LSM:
            cursize = sizeof(identifp->lsm_id);
            if (!csi_xlsm_id(xdrsp, &identifp->lsm_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,"csi_xlsm_id()",
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_LOCK:
            cursize = sizeof(identifp->lock_id);
            if (!csi_xlockid(xdrsp, &identifp->lock_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,"csi_xlockid()",
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_PANEL:
            cursize = sizeof(identifp->panel_id);
            if (!csi_xpnl_id(xdrsp, &identifp->panel_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,"csi_xpnl_id()",
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_PTP:
            cursize = sizeof(identifp->ptp_id);
	    /* must call csi_xptp_id because ptp_id is no longer the */
            /* same as a panel_id */
            if (!csi_xptp_id(xdrsp, &identifp->ptp_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,"csi_xptp_id()",
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_SUBPANEL:
            cursize = sizeof(identifp->subpanel_id);
            if (!csi_xspnl_id(xdrsp, &identifp->subpanel_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xspnl_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_VOLUME:
            cursize = sizeof(identifp->vol_id.external_label);
            if (!csi_xvol_id(xdrsp, &identifp->vol_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,"csi_xvol_id()",
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_SERVER:     
        case TYPE_MOUNT:     
        case TYPE_DISMOUNT:     
            /* decode for max size, disregards contents */
            cp = identifp->alignment_size;
            cursize = sizeof(identifp->alignment_size);
            if (!xdr_bytes(xdrsp, &cp, &cursize, cursize)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_bytes()",
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_NONE:     
            /* decode for max size, disregards contents */
            cp = identifp->alignment_size;
            cursize = sizeof(identifp->alignment_size);
            if (!xdr_bytes(xdrsp, &cp, &cursize, cursize)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_bytes()",
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_IPC:
            cp = identifp->socket_name;
            cursize = strlen(identifp->socket_name);
            if (!xdr_bytes(xdrsp, &cp,&cursize,sizeof(identifp->socket_name))) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_bytes()",
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_REQUEST:
            cursize = sizeof(identifp->request);
            if (!xdr_long(xdrsp, &identifp->request)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_long()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_MEDIA_TYPE:

	    /* if the media type supplied in a query mount scratch request was
	     * invalid, the invalid media type is returned in the response
	     * status.
	     */
            cursize = sizeof(identifp->media_type);
            if (!csi_xmedia_type(xdrsp, &identifp->media_type)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
			"csi_xmedia_type()", MMSG(928,
					  "XDR message translation failure")));
                return(0);
            }
            break;

        case TYPE_HAND:
            cursize = sizeof(identifp->hand_id);
            if (!csi_xhand_id(xdrsp, &identifp->hand_id)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		  "csi_xhand_id()", 
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
            break;
	case TYPE_VTDID:
	    cursize = sizeof(identifp->vtd_id);
	    if (!csi_xdrive_id(xdrsp, &identifp->drive_id)) {
		MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		    "csi_xdrive_id()",
		    MMSG(928, "XDR message translation failure")));
		return(0);
	    }
	    break;
	case TYPE_SUBPOOL_NAME:
	    cursize = sizeof(identifp->subpool_name);
	    sp = (char *)&identifp->subpool_name;
	    if (!xdr_string(xdrsp, (char **)&sp,SUBPOOL_NAME_SIZE+1)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		    "xdr_string()",
		    MMSG(928, "XDR message translation failure")));
		return(0);
	    }
	    break;
	case TYPE_DRIVE_GROUP:
	    cursize = sizeof(identifp->groupid);
	    sp = (char *)&identifp->groupid;
	    if (!xdr_string(xdrsp, (char **)sp, GROUPID_SIZE + 1)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,
		    "xdr_string()",
		    MMSG(928, "XDR message translation failure")));
		return(0);
	    }
	    break;


        case TYPE_FIRST:
        case TYPE_LAST:
        default:
            /* log two messages to make the problem clear */
            /* invalid type, cannot decipher */
            MLOGCSI((STATUS_INVALID_TYPE, st_module,  CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            /* translation failed */
            MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  CSI_NO_CALLEE, 
	      MMSG(928, "XDR message translation failure")));
            return(0);
    }

    /*
     *  Must translate trailing bytes if identifier is not the full size of the
     *  union since must send whole union on wire to maintain packet structure.
     */

    /* point to end of currently translated bytes */
    cp = (char*)identifp + cursize;

    /* determine if any trailing bytes are left to be translated */
    cursize = sizeof(identifp->alignment_size) - cursize;

    /* translate trailing bytes */
    if (cursize > 0) {

        /* Use a temporary buffer when decoding trailing bytes and decode
         * trailing bytes with maxsize ALIGNMENT_PAD_SIZE. This approach
         * has the side effect of correctly updating the xdrsp pointer.
         */
        if (XDR_DECODE == xdrsp->x_op) {
                char temp[ALIGNMENT_PAD_SIZE]; /* temp buffer for xdr_bytes */
                char *tmp = temp;              /* xdr_bytes expects as arg */

            if (!xdr_bytes(xdrsp, &tmp, &cursize, ALIGNMENT_PAD_SIZE)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_bytes()",
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
        }
        else {
            if (!xdr_bytes(xdrsp, &cp, &cursize, cursize)) {
                MLOGCSI((STATUS_TRANSLATION_FAILURE, st_module,  "xdr_bytes()",
		  MMSG(928, "XDR message translation failure")));
                return(0);
            }
        }
    }
    return(1);
}
