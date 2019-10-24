/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_media_search.c                              */
/** Description:    XAPI client media type table (XAPIMEDIA)         */
/**                 search service.                                  */
/**                                                                  */
/**                 The XAPIMEDIA is a table for converting an       */
/**                 XAPI media name into an ACSAPI media type        */
/**                 code (and vice versa).  Each XAPIMEDIA table     */
/**                 entry also contains a table of compatible        */
/**                 ACSAPI tape drive type codes.                    */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     07/15/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/***END PROLOGUE******************************************************/

/*********************************************************************/
/* Includes:                                                         */
/*********************************************************************/
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
/** Function Name: xapi_media_search_name                            */
/** Description:   Search the XAPIMEDIA table by name.               */
/**                                                                  */
/** Search the XAPIMEDIA table and return the matching               */
/** entry for the input XAPI media name.  Format: NULL terminated    */
/** string; max strlen() = 8.                                        */
/**                                                                  */
/** NULL indicates that no matching XAPIMEDIA entry was found.       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_media_search_name"

extern struct XAPIMEDIA *xapi_media_search_name(struct XAPICVT  *pXapicvt,
                                                struct XAPIREQE *pXapireqe,
                                                char            *mediaNameString)
{
    struct XAPIMEDIA   *pNextXapimedia      = &(pXapicvt->xapimedia[0]);
    struct XAPIMEDIA   *pMatchingXapimedia  = NULL;
    int                 i;

    for (i = 0;
        i < pXapicvt->xapimediaCount;
        i++, pNextXapimedia++)
    {
        if (strcmp(pNextXapimedia->mediaNameString, mediaNameString) == 0)
        {
            pMatchingXapimedia = pNextXapimedia;

            break;
        }
    }

    TRMSGI(TRCI_XAPI,
           "mediaNameString=%s, MatchingXapimedia=%08X\n",
           mediaNameString,
           pMatchingXapimedia);

    return pMatchingXapimedia;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_media_search_type                            */
/** Description:   Search the XAPIMEDIA table by media type code.    */
/**                                                                  */
/** Search the XAPIMEDIA table and return the matching               */
/** entry for the input ACSAPI media type code: Format: character.   */
/**                                                                  */
/** NULL indicates that no matching XAPIDRVTYP entry was found.      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_media_search_type"

extern struct XAPIMEDIA *xapi_media_search_type(struct XAPICVT  *pXapicvt,
                                                struct XAPIREQE *pXapireqe,
                                                char             acsapiMediaType)
{
    struct XAPIMEDIA   *pNextXapimedia      = &(pXapicvt->xapimedia[0]);
    struct XAPIMEDIA   *pMatchingXapimedia  = NULL;
    int                 i;

    for (i = 0;
        i < pXapicvt->xapimediaCount;
        i++, pNextXapimedia++)
    {
        if (pNextXapimedia->acsapiMediaType == acsapiMediaType)
        {
            pMatchingXapimedia = pNextXapimedia;

            break;
        }
    }

    TRMSGI(TRCI_XAPI,
           "acsapiMediaType=%d, MatchingXapimedia=%08X\n",
           acsapiMediaType,
           pMatchingXapimedia);

    return pMatchingXapimedia;
}



