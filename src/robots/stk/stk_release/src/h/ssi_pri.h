/*  SccsId      @(#) %full_name: 1/incl/ssi_pri.h/1 % %release:  % %date_created: Tue Jan 24 18:00:10 1995 %   */
#ifndef _SSI_PRI_
#define _SSI_PRI_
/*
 * Copyright (1994, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      Container for SSI specific structures used in SSI specific 
 *      functions. The SSI specific functions are:
 *       
 *       o Network Failure Notification
 *
 * Modified by:
 *
 *      K. J. Stickney        24-Jan-1995.    Original.
 */

/*
 *      Header Files:
 */

#ifndef _CSI_STRUCTS_
#include "csi_structs.h"
#endif /*_CSI_STRUCTS_ */

#ifndef _CSI_V0_STRUCTS_
#include "csi_v0_structs.h"
#endif /*_CSI_V0_STRUCTS_ */


/*
 *      Defines, Typedefs and Structure Definitions:
 */

/* used for Network Failure notification. */

typedef struct {
	CSI_REQUEST_HEADER csi_request_header;
	RESPONSE_STATUS    message_status;
} CSI_RESPONSE_HEADER;

typedef struct {
	CSI_V0_REQUEST_HEADER csi_request_header;
	RESPONSE_STATUS    message_status;
} CSI_V0_RESPONSE_HEADER;

/*
 *      Global Variable Declarations:
 */

/*
 *      Procedure Type Declarations:
 */

#endif /* _SSI_PRI_ */
