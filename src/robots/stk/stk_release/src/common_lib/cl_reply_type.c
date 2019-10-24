static char P4_Id[]="$Id: //depot/acsls6.0/lib/common/cl_reply_type.c#1 $";

/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_reply_type
 *
 * Description:
 *
 *      Common module that converts a resource identifier, according to its specified
 *      type, to an equivalent character string counterpart.  If an invalid
 *      type is specified, the string "no format for TYPE_type" is returned.
 *
 *      cl_reply_type ensures character string resources won't overrun the
 *      formatting buffer by limiting string sizes to EVENT_SIZE bytes.
 *
 * Return Values:
 *
 *      (char *)pointer to character string.
 *
 * Implicit Inputs:
 *
 *      NONE
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
 *      NONE
 *
 * Revision History:
 *
 *      S. L. Siao              19-Oct-2001     Original.
 *      S. L. Siao              19-Apr-2002     From code review, made def of tbuf static,
 *                                              changed strcat and sprintf to strcpy.
 *      Wipro (Hemendra)        18-Jun-2004     Support for mount/ dismount events (for CIM)
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <string.h>
#include <stdio.h>                      /* ANSI-C compatible */

#include "cl_pub.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */


/*
 *      Procedure Type Declarations:
 */


char *cl_reply_type( EVENT_REPLY_TYPE event_reply_type)
{
    static char tbuf[80];

#ifdef DEBUG
    if TRACE(0)
        cl_trace("cl_reply_type", 1,    /* routine name and parameter count */
                 (EVENT_REPLY_TYPE) event_reply_type);
#endif /* DEBUG */

switch (event_reply_type) {

      case EVENT_REPLY_REGISTER:
	  strcpy(tbuf, "EVENT_REPLY_REGISTER");
	  break;
      case EVENT_REPLY_UNREGISTER:
	  strcpy(tbuf, "EVENT_REPLY_UNREGISTER");
	  break;
      case EVENT_REPLY_SUPERCEDED:
          strcpy(tbuf, "EVENT_REPLY_SUPERCEDED");
	  break;
      case EVENT_REPLY_SHUTDOWN:
          strcpy(tbuf, "EVENT_REPLY_SHUTDOWN");
	  break;
      case EVENT_REPLY_CLIENT_CHECK:
          strcpy(tbuf, "EVENT_REPLY_CLIENT_CHECK");
	  break;
      case EVENT_REPLY_RESOURCE:
          strcpy(tbuf, "REVENT_REPLY_RESOURCE");
	  break;
      case EVENT_REPLY_VOLUME:
          strcpy(tbuf, "EVENT_REPLY_VOLUME");
	  break;
      case EVENT_REPLY_DRIVE_ACTIVITY:
          strcpy(tbuf, "EVENT_REPLY_DRIVE_ACTVITY");
	  break;
      default:
        strcpy(tbuf, "EVENT_UNKNOWN");
        break;
    }

    return tbuf;
}
