/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */ 
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_userid.c                                    */
/** Description:    XAPI client return userid service.               */
/**                                                                  */
/**                 Return the "best" userid.                        */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     09/01/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/***END PROLOGUE******************************************************/

/*********************************************************************/
/* Includes:                                                         */
/*********************************************************************/
#include <pwd.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "api/defs_api.h"                              
#include "csi.h"
#include "smccxmlx.h"
#include "srvcommon.h"
#include "xapi.h"


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_userid                                       */
/** Description:   XAPI client return userid service.                */
/**                                                                  */
/** Return the "best" userid.                                        */
/** The hierarchy of what is the "best" userid is:                   */
/** (1) The first choice is the MESSAGE_HEADER.access_id.user_id.    */
/**     This is set using the ACSAPI SET ACCESS function.            */
/** (2) If MESSAGE_HEADER.access_id.user_id is not specified;        */
/**     The 2nd choice is the XAPICVT.xapiUser.                      */
/**     This is set using the XAPI_USER environment variable.        */
/** (3) If XAPICVT.xapiUser is not specified;                        */
/**     The 3rd choice is the returned geteuid().                    */
/** (4) If the geteuid() cannot be resolved;                         */
/**     The the userid will default to "XAPIUSER".                   */
/**                                                                  */
/** NOTE: The input XAPICVT and pMessageHeader may be NULL.          */
/** Also, the input pUseridString must be large enough for the       */
/** ACSAPI USERID string (or at least 65 characters).                */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_userid"

extern void xapi_userid(struct XAPICVT *pXapicvt,
                        char           *pMessageHeader,
                        char           *pUseridString)
{
    MESSAGE_HEADER     *pMessage_Header     = (MESSAGE_HEADER*) pMessageHeader;
    uid_t               clientUid;
    struct passwd      *pClientPasswd;

    pUseridString[0] = 0;

    if (pMessage_Header != NULL)
    {
        STRIP_TRAILING_BLANKS(pMessage_Header->access_id.user_id.user_label,
                              pUseridString,
                              sizeof(pMessage_Header->access_id.user_id.user_label));
    }

    if ((pUseridString[0] <= ' ') &&
        (pXapicvt != NULL))
    {
        strcpy(pUseridString, pXapicvt->xapiUser);
    }

    if (pUseridString[0] <= ' ')
    {
        clientUid = geteuid();
        pClientPasswd = getpwuid(clientUid);

        if (pClientPasswd != NULL)
        {
            strcpy(pUseridString, pClientPasswd->pw_name);
        }
    }

    if (pUseridString[0] <= ' ')
    {
        strcpy(pUseridString, XAPI_DEFAULTUSER);
    }

    TRMSGI(TRCI_XAPI,
           "Userid=%s\n",
           pUseridString);

    return;
}



