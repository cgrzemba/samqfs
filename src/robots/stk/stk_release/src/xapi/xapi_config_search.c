/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_config_search.c                             */
/** Description:    XAPI client drive configuration table (XAPICFG)  */
/**                 search service.                                  */
/**                                                                  */
/**                 The XAPICFG is the XAPI client drive             */
/**                 configuration table.  There is one XAPICFG       */
/**                 table entry for each TapePlex drive that is      */
/**                 accessible from the XAPI client.  The XAPICFG    */
/**                 contains a subset of all TapePlex real and       */
/**                 virtual tape drives.                             */
/**                                                                  */
/**                 NOTE: Every XAPI thread creates its own copy     */
/**                 of the XAPICFG table for its request.            */
/**                 XAPICFG search routines always access this       */
/**                 XAPICFG copy.  Therefore, ownership of the       */
/**                 XAPICVT.xapicfgLock semaphore is NOT             */
/**                 required for search.                             */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     05/11/11                          */
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
#include "api/db_defs_api.h"
#include "csi.h"
#include "srvcommon.h"
#include "xapi.h"


/*********************************************************************/
/* Structures:                                                       */
/*********************************************************************/


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_config_search_drivelocid                     */
/** Description:   Search the XAPICFG table by driveLocId.           */
/**                                                                  */
/** Return the XAPICFG table entry that matches the input            */
/** driveLocId (maximum 16 characters). Format: character;           */
/** "R:nn:nn:nn:nn" or "V:VTSSNAME:nnn".                             */
/**                                                                  */
/** NULL indicates that no matching XAPICFG entry was found.         */
/**                                                                  */
/** NOTE: The XAPICFG table accessed here is a copy of the XAPICFG   */
/** table created by xapi.main when this request entered the XAPI.   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_config_search_drivelocid"

extern struct XAPICFG *xapi_config_search_drivelocid(struct XAPICVT  *pXapicvt,
                                                     struct XAPIREQE *pXapireqe,
                                                     char             driveLocId[16])
{
    struct XAPICFG     *pNextXapicfg        = pXapireqe->pXapicfg;
    struct XAPICFG     *pMatchingXapicfg    = NULL;
    int                 i;

    for (i = 0;
        i < pXapireqe->xapicfgCount;
        i++, pNextXapicfg++)
    {
        if (memcmp(pNextXapicfg->driveLocId,
                   driveLocId,
                   sizeof(pNextXapicfg->driveLocId)) == 0)
        {
            pMatchingXapicfg = pNextXapicfg;

            break;
        }
    }

    TRMSGI(TRCI_XAPI,
           "driveLocId=%.16s, MatchingXapicfg=%08X\n",
           driveLocId,
           pMatchingXapicfg);

    return pMatchingXapicfg;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_config_search_hexdevaddr                     */
/** Description:   Search the XAPICFG table by hexDevAddr.           */
/**                                                                  */
/** Return the XAPICFG table entry that matches the input            */
/** hexadeciman device address.  Format: unsigned short.             */
/**                                                                  */
/** NULL indicates that no matching XAPICFG entry was found.         */
/**                                                                  */
/** NOTE: The XAPICFG table accessed here is a copy of the XAPICFG   */
/** table created by xapi.main when this request entered the XAPI.   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_config_search_hexdevaddr"

extern struct XAPICFG *xapi_config_search_hexdevaddr(struct XAPICVT  *pXapicvt,
                                                     struct XAPIREQE *pXapireqe,
                                                     unsigned short   hexDevAddr)
{
    struct XAPICFG     *pNextXapicfg        = pXapireqe->pXapicfg;
    struct XAPICFG     *pMatchingXapicfg    = NULL;
    int                 i;

    for (i = 0;
        i < pXapireqe->xapicfgCount;
        i++, pNextXapicfg++)
    {
        if (pNextXapicfg->driveName == hexDevAddr)
        {
            pMatchingXapicfg = pNextXapicfg;

            break;
        }
    }

    TRMSGI(TRCI_XAPI,
           "hexDevAddr=%04X, MatchingXapicfg=%08X\n",
           hexDevAddr,
           pMatchingXapicfg);

    return pMatchingXapicfg;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_config_search_libdrvid                       */
/** Description:   Search the XAPICFG table by LIBDRVID.             */
/**                                                                  */
/** Return the XAPICFG table entry that matches the input            */
/** LIBDRVID structure.  Format: 4 bytes;                            */
/** 1 byte acs + 1 byte lsm + 1 byte panel + 1 byte drive.           */
/**                                                                  */
/** NULL indicates that no matching XAPICFG entry was found.         */
/**                                                                  */
/** NOTE: The XAPICFG table accessed here is a copy of the XAPICFG   */
/** table created by xapi.main when this request entered the XAPI.   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_config_search_libdrvid"

extern struct XAPICFG *xapi_config_search_libdrvid(struct XAPICVT  *pXapicvt,
                                                   struct XAPIREQE *pXapireqe,
                                                   struct LIBDRVID *pLibdrvid)
{
    struct XAPICFG     *pNextXapicfg        = pXapireqe->pXapicfg;
    struct XAPICFG     *pMatchingXapicfg    = NULL;
    int                 i;

    for (i = 0;
        i < pXapireqe->xapicfgCount;
        i++, pNextXapicfg++)
    {
        if (memcmp(&(pNextXapicfg->libdrvid),
                   (char*) pLibdrvid,
                   sizeof(struct LIBDRVID)) == 0)
        {
            pMatchingXapicfg = pNextXapicfg;

            break;
        }
    }

    TRMSGI(TRCI_XAPI,
           "libdrvid=%d:%d:%d:%d, MatchingXapicfg=%08X\n",
           pLibdrvid->acs,
           pLibdrvid->lsm,
           pLibdrvid->panel,
           pLibdrvid->driveNumber,
           pMatchingXapicfg);

    return pMatchingXapicfg;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_config_search_vtss                           */
/** Description:   Search the XAPICFG table by VTSS name string.     */
/**                                                                  */
/** Return the XAPICFG table entry that matches the input            */
/** VTSS name.  Format: NULL terminated string; max strlen() = 8.    */
/**                                                                  */
/** NULL indicates that no matching XAPICFG entry was found.         */
/**                                                                  */
/** NOTE: The XAPICFG table accessed here is a copy of the XAPICFG   */
/** table created by xapi.main when this request entered the XAPI.   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_config_search_vtss"

extern struct XAPICFG *xapi_config_search_vtss(struct XAPICVT  *pXapicvt,
                                               struct XAPIREQE *pXapireqe,
                                               char            *vtssNameString)
{
    struct XAPICFG     *pNextXapicfg        = pXapireqe->pXapicfg;
    struct XAPICFG     *pMatchingXapicfg    = NULL;
    int                 i;

    for (i = 0;
        i < pXapireqe->xapicfgCount;
        i++, pNextXapicfg++)
    {
        if (strcmp(pNextXapicfg->vtssNameString, vtssNameString) == 0)
        {
            pMatchingXapicfg = pNextXapicfg;

            break;
        }
    }

    TRMSGI(TRCI_XAPI,
           "vtssNameString=%s, MatchingXapicfg=%08X\n",
           vtssNameString,
           pMatchingXapicfg);

    return pMatchingXapicfg;
}



