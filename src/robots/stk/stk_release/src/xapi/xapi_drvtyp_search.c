/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_drvtyp_search.c                             */
/** Description:    XAPI client drive type table (XAPIDRVTYP)        */
/**                 search service.                                  */
/**                                                                  */
/**                 The XAPIDRVTYP is a table for converting         */
/**                 an XAPI tape drive model name into an ACSAPI     */
/**                 tape drive type code (and vice versa).           */
/**                 Each XAPIDRVTYP table entry also contains a      */
/**                 table of compatible ACSAPI media type codes.     */
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
/** Function Name: xapi_drvtyp_search_name                           */
/** Description:   Search the XAPIDRVTYP table by name.              */
/**                                                                  */
/** Search the XAPIDRVTYP table and return the matching              */
/** entry for the input XAPI model name.  Format: NULL terminated    */
/** string; max strlen() = 8.                                        */
/**                                                                  */
/** NULL indicates that no matching XAPIDRVTYP entry was found.      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_drvtyp_search_name"

extern struct XAPIDRVTYP *xapi_drvtyp_search_name(struct XAPICVT  *pXapicvt,
                                                  struct XAPIREQE *pXapireqe,
                                                  char            *modelNameString)

{
    struct XAPIDRVTYP  *pNextXapidrvtyp      = &(pXapicvt->xapidrvtyp[0]);
    struct XAPIDRVTYP  *pMatchingXapidrvtyp  = NULL;
    int                 i;

    for (i = 0;
        i < pXapicvt->xapidrvtypCount;
        i++, pNextXapidrvtyp++)
    {
        if (strcmp(pNextXapidrvtyp->modelNameString, modelNameString) == 0)
        {
            pMatchingXapidrvtyp = pNextXapidrvtyp;

            break;
        }
    }

    TRMSGI(TRCI_XAPI,
           "modelNameString=%s, pMatchingXapidrvtyp=%08X\n",
           modelNameString,
           pMatchingXapidrvtyp);

    return pMatchingXapidrvtyp;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_drvtyp_search_type                           */
/** Description:   Search the XAPIDRVTYP table by drive type code.   */
/**                                                                  */
/** Search the XAPIDRVTYP table and return the matching              */
/** entry for the input ACSAPI drive type code: Format: character.   */
/**                                                                  */
/** NULL indicates that no matching XAPIDRVTYP entry was found.      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_drvtyp_search_type"

extern struct XAPIDRVTYP *xapi_drvtyp_search_type(struct XAPICVT  *pXapicvt,
                                                  struct XAPIREQE *pXapireqe,
                                                  char             acsapiDriveType)

{
    struct XAPIDRVTYP  *pNextXapidrvtyp      = &(pXapicvt->xapidrvtyp[0]);
    struct XAPIDRVTYP  *pMatchingXapidrvtyp  = NULL;
    int                 i;

    for (i = 0;
        i < pXapicvt->xapidrvtypCount;
        i++, pNextXapidrvtyp++)
    {
        if (pNextXapidrvtyp->acsapiDriveType == acsapiDriveType)
        {
            pMatchingXapidrvtyp = pNextXapidrvtyp;

            break;
        }
    }

    TRMSGI(TRCI_XAPI,
           "acsapiDriveType=%d, MatchingXapidrvtyp=%08X\n",
           acsapiDriveType,
           pMatchingXapidrvtyp);

    return pMatchingXapidrvtyp;
}



