/* SccsId @(#)lm_structs.h	1.2 1/11/94  */
#ifndef _LM_STRUCTS_
#define _LM_STRUCTS_
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      definitions of VERSION2 specific ACSLM data structures
 *
 * Considerations:
 *
 *      The structures defined here have corresponding definitions for the
 *      CSI/SSI in csi_structs.h.  any modifications to this file SHOULD be
 *      reflected in csi_structs.h as well.
 *
 * Modified by:
 *
 *      D. F. Reed          27-Jan-1989     Original.
 *      J. W. Montgomery    12-Mar-1990     Added define_pool, delete_pool, and
 *                      set_scratch requests. Modified query for scratch.
 *      J. A. Wishner       06-Apr-1990     Added set-clean requests/responses.
 *                      Added MOUNT_SCRATCH_REQUEST/RESPONSE
 *      J. A. Wishner       10-Apr-1990     Added LOCK, UNLOCK, CLEAR-LOCK,
 *                      QUERY-LOCK requests/responses, SET_CAP_REQUEST/RESPONSE,
 *                      QU_EXT_CAP_STATUS
 *      H. I. Grapek        11-May-1990     Modified struct SET_CLEAN_RESPONSE 
 *                      to conform to the PFS.
 *      J. S. Alexander     18-Jun-1991     Added support for multiple CAPs per
 *                      LSM structures.
 *      H. I. Grapek        24-Sep-1991     Removed QU_EXT_CAP_STATUS and
 *                      QU_EXT2_CAP_STATUS, cleanup to coding standards.
 *      D. A. Beidle        27-Sep-1991     Removed conditional includes of
 *                      identifier.h and db_structs.h since structs.h does the
 *                      job for us.
 *      Alec Sharp       15-Aug-1992        Added SET_OWNER_REQUEST and
 *                                          SET_OWNER_RESPONSE.
 *	J. Borzuchowski	 05-Aug-1993	    R5.0 Mixed Media-- Added new 
 *			select_criteria union and deleted count for 
 *			QUERY_REQUEST.  This union replaces the identifier 
 *			union in QUERY_REQUEST and the count from the fixed 
 *			portion is distributed to the new union member 
 *			structures along with the identifier.
 *			Added new members to the status_response union and 
 *			deleted count for QUERY_RESPONSE.  The new union members
 *			replace the status members.  The count from the fixed
 *			portion is distributed to the new union members along 
 *			with the status structures.  The new union member
 *			for mixed media info contains media type and drive
 *			type status structures for the new query mixed media 
 *			request.
 *			Added media type field to MOUNT_SCRATCH_REQUEST.
 *
 *      K. Stickney      28-Oct-1993       Stripped out all public structures.
 *                                         Added lm_structs_api.h include stmt.
 */

/*
 *      Header Files:
 */

#ifndef _STRUCTS_
#include "structs.h"
#endif

#ifndef _LM_STRUCTS_API_
#include "api/lm_structs_api.h"
#endif

#endif /* _LM_STRUCTS_ */

