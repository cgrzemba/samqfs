/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_scrpool_search.c                            */
/** Description:    XAPI client scratch subpool table (XAPISCRPOOL)  */
/**                 search service.                                  */
/**                                                                  */
/**                 The XAPI client scratch subpool table            */
/**                 (XAPISCRPOOL) is a fixed length table embedded   */
/**                 within the XAPICVT.  It is used to translate     */
/**                 XAPI scratch subpool names into ACSAPI subpool   */
/**                 identifiers (and vice-versa).  ACSAPI subpool    */
/**                 identifiers are the same value as an XAPI        */
/**                 subpool index.                                   */
/**                                                                  */
/**                 The XAPISCRPOOL has 256 entries; Entry (or       */
/**                 subpool index) = 0) is for the default subpool;  */
/**                 Entries 1-255 are for the named scratch subpools.*/
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
/** Function Name: xapi_scrpool_search_name                          */
/** Description:   Search the XAPISCRPOOL table by subpool name.     */
/**                                                                  */
/** Return the XAPISCRPOOL table entry that matches the input        */
/** subpool name.  Format: NULL terminated string; max strlen() = 13.*/
/**                                                                  */
/** NULL indicates that no matching XAPISCRPOOL entry was found.     */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_scrpool_search_name"

extern struct XAPISCRPOOL *xapi_scrpool_search_name(struct XAPICVT  *pXapicvt,
                                                    struct XAPIREQE *pXapireqe,
                                                    char            *subpoolNameString)
{
    struct XAPISCRPOOL *pNextXapiscrpool    = &(pXapicvt->xapiscrpool[0]);
    struct XAPISCRPOOL *pMatchingXapiscrpool= NULL;
    int                 i;

    for (i = 0;
        i < MAX_XAPISCRPOOL;
        i++, pNextXapiscrpool++)
    {
        if (strcmp(pNextXapiscrpool->subpoolNameString, subpoolNameString) == 0)
        {
            pMatchingXapiscrpool = pNextXapiscrpool;

            break;
        }
    }

    TRMSGI(TRCI_XAPI,
           "subpoolNameString=%s, MatchingXapiscrpool=%08X\n",
           subpoolNameString,
           pMatchingXapiscrpool);

    return pMatchingXapiscrpool;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_scrpool_search_index                         */
/** Description:   Search the XAPISCRPOOL table by subpool index.    */
/**                                                                  */
/** Return the XAPISCRPOOL table entry that matches the input        */
/** subpool index.  Format: short; value 0-255.                      */
/**                                                                  */
/** NULL indicates that no matching XAPISCRPOOL entry was found, or  */
/** that the entry was not defined (i.e. the subpool name was NULL). */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_scrpool_search_index"

extern struct XAPISCRPOOL *xapi_scrpool_search_index(struct XAPICVT  *pXapicvt,
                                                     struct XAPIREQE *pXapireqe,
                                                     short            subpoolIndex)
{
    struct XAPISCRPOOL *pMatchingXapiscrpool= NULL;

    if ((subpoolIndex < 1) ||
        (subpoolIndex > 255))
    {
        pMatchingXapiscrpool = NULL;
    }
    else
    {
        pMatchingXapiscrpool = &(pXapicvt->xapiscrpool[subpoolIndex]);

        if (pMatchingXapiscrpool->subpoolNameString[0] == 0)
        {
            pMatchingXapiscrpool = NULL;
        }
    }

    TRMSGI(TRCI_XAPI,
           "subpoolIndex=%d, MatchingXapiscrpool=%08X\n",
           subpoolIndex,
           pMatchingXapiscrpool);

    return pMatchingXapiscrpool;
}



