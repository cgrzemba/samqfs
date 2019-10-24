/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_volser.c                                    */
/** Description:    XAPI client volser validation, and volser-range  */
/**                 increment service.                               */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     07/05/11                          */
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

#include "srvcommon.h"
#include "xapi.h"
#include "api/defs_api.h"
#include "csi.h"


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void convertVolserToMask(char volser[6],
                                char volserMask[6]);


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: xapi_validate_single_volser                       */
/** Description:   Validate that the input volser is valid.          */
/**                                                                  */
/** Return codes are:                                                */
/** STATUS_SUCCESS: The volser was valid.                            */
/** STATUS_INVALID_VALUE The volser was invalid,                     */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "xapi_validate_single_volser"

extern int xapi_validate_single_volser(char volser[6])
{
    int                 i;

    for (i = 0;
        i < sizeof(volser);
        i++)
    {
        if ((isupper(volser[i])) ||
            (isdigit(volser[i])) ||
            (volser[i] == '$') ||
            (volser[i] == '@') ||
            (volser[i] == '#'))
        {
            continue;
        }
        else
        {
            return STATUS_INVALID_VALUE;
        }
    }

    return STATUS_SUCCESS;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: xapi_validate_volser_range                        */
/** Description:   Validate the input volser range.                  */
/**                                                                  */
/** Validate that the input volser(s) specify either 1 valid         */
/** volser or a valid volser range, and return the corresponding     */
/** XAPIVRANGE structure.                                            */
/**                                                                  */
/** NOTE: A volser range may be a single volser range when the       */
/** firstVol is equal to the lastVol.                                */
/**                                                                  */
/** Return codes are:                                                */
/** STATUS_SUCCESS: The volser or range was invalid.                 */
/** STATUS_INVALID_VALUE: A volser was invalid.                      */
/** STATUS_INVALID_RANGE: The range was invalid.                     */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "xapi_validate_volser_range"

extern int xapi_validate_volser_range(struct XAPIVRANGE *pXapivrange)
{
    int                 lastRC;
    int                 i;
    char                firstVolMask[6];
    char                lastVolMask[6];
    int                 currMaskRightPos;  
    char                currMask;
    char                differenceFound     = FALSE;

    /*****************************************************************/
    /* Validate the first volser of the range, or the only volser.   */
    /*****************************************************************/
    lastRC = xapi_validate_single_volser(pXapivrange->firstVol);

    TRMSGI(TRCI_XAPI,
           "RC from xapi_validate_single_volser(%.6s)=%d\n", 
           pXapivrange->firstVol,
           lastRC);

    if (lastRC != STATUS_SUCCESS)
    {
        return STATUS_INVALID_VALUE;
    }

    if ((pXapivrange->lastVol[0] <= ' ') ||
        (memcmp(pXapivrange->firstVol,
                pXapivrange->lastVol,
                sizeof(pXapivrange->firstVol)) == 0))
    {
        memcpy(pXapivrange->lastVol,
               pXapivrange->firstVol,
               sizeof(pXapivrange->firstVol));

        TRMSGI(TRCI_XAPI,
               "lastVol same as firstVol or not specified\n");

        pXapivrange->numInRange = 1;

        return STATUS_SUCCESS;
    }

    lastRC = xapi_validate_single_volser(pXapivrange->lastVol);

    TRMSGI(TRCI_XAPI,
           "RC from xapi_validate_single_volser(%.6s)=%d\n", 
           pXapivrange->lastVol,
           lastRC);

    if (lastRC != STATUS_SUCCESS)
    {
        return STATUS_INVALID_VALUE;
    }

    if (memcmp(pXapivrange->firstVol,
               pXapivrange->lastVol,
               sizeof(pXapivrange->firstVol)) > 0)
    {
        return STATUS_INVALID_RANGE;
    }

    convertVolserToMask(pXapivrange->firstVol,
                        firstVolMask);

    convertVolserToMask(pXapivrange->lastVol,
                        lastVolMask);

    if (memcmp(firstVolMask,
               lastVolMask,
               sizeof(firstVolMask)) != 0)
    {
        TRMSGI(TRCI_XAPI,
               "firstVolMask=%.6s != lastVolMask=%.6s\n", 
               firstVolMask,
               lastVolMask);

        return STATUS_INVALID_RANGE;
    }

    /*****************************************************************/
    /* Now find the starting position of the volser increment and    */
    /* its range.                                                    */
    /*****************************************************************/
    currMaskRightPos = 5;
    currMask = firstVolMask[5];

    for (i = 5; 
        i >= 0;
        i--)
    {
        if (firstVolMask[i] != currMask)
        {
            if (differenceFound)
            {
                break;
            }

            currMaskRightPos = i;
            currMask = firstVolMask[i];
        }

        if (pXapivrange->firstVol[i] != pXapivrange->lastVol[i])
        {
            differenceFound = TRUE;
        }
    }

    TRMSGI(TRCI_XAPI,
           "firstVolMask=%.6s, currMaskRightPos=%d, i=%d, currMask=%c\n", 
           firstVolMask,
           currMaskRightPos, 
           i, 
           currMask);

    pXapivrange->volIncIx = i + 1;
    pXapivrange->volIncLen = currMaskRightPos - i; 

    memcpy(pXapivrange->mask,
           firstVolMask,
           sizeof(pXapivrange->mask));

    /*****************************************************************/
    /* Now validate that we have only 1 logical increment in range.  */
    /*****************************************************************/
    for (; 
        i >= 0;
        i--)
    {
        if (pXapivrange->firstVol[i] != pXapivrange->lastVol[i])
        {
            TRMSGI(TRCI_XAPI,
                   "Another logical increment detected at position=%d\n",
                   i);

            return STATUS_INVALID_RANGE;
        }
    }

    /*****************************************************************/
    /* Now find out how many volsers there are in the range.         */
    /*****************************************************************/
    memset(pXapivrange->currVol, 0, sizeof(pXapivrange->currVol));

    while (1)
    {
        xapi_volser_increment(pXapivrange);

        if (pXapivrange->rangeCompletedFlag)
        {
            break;
        }

        pXapivrange->numInRange++;
    }

    memset(pXapivrange->currVol, 0, sizeof(pXapivrange->currVol));
    pXapivrange->rangeCompletedFlag = FALSE;

    TRMSGI(TRCI_XAPI,
           "numInRange=%d\n",
           pXapivrange->numInRange);

    return STATUS_SUCCESS;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: convertVolserToMask                               */
/** Description:   Convert a volser into a volser range "mask".      */
/**                                                                  */
/** A volser range mask indicates the type of characters in the      */
/** volser; either alphabetic, numeric, blank, or national           */
/** character(s).  The mask is used to determine whether a valid     */
/** volser range has been specified, and how the range will be       */
/** incremented.                                                     */
/**                                                                  */
/** If a range of volsers is to be processed, then the first volser  */
/** and last volser in the range must have an identical masks.       */
/**                                                                  */
/** As an example, if the first volser in the range is specified     */
/** as "TS@123" then its mask will be "AA@000" which represents      */
/** 2 alphabetic characters, followed by the national chaacter "@",  */
/** followed by 3 numerics.  The ending volser in this range         */
/** must have the identical mask, "AA@000".                          */
/**                                                                  */
/** In addition, we allow only 1 "logical" range be specified        */
/** in a volser range.  A logical range is defined when the          */
/** same mask type represents different actual values in the         */
/** first and last volser in the range.                              */
/**                                                                  */
/** As an example, if the first volser in the range is specified     */
/** as "TS@123" and the last volser is specified as "ZY@999" then    */
/** there are 2 logical ranges defined;  (1) The range TS-ZY and;    */
/** (2) The range 123-999. Such a volser range would be flagged as   */
/** invalid.  Valid volser ranges would be either TS@123-TS@999 or   */
/** or TS@123-XY@123, each representing a single logical range.      */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "convertVolserToMask"

static void convertVolserToMask(char volser[6],
                                char volserMask[6])
{
    int                 i;

    for (i = 0;
        i < 6;
        i++)
    {
        if (isupper(volser[i]))
        {
            volserMask[i] = 'A';

            continue;
        }

        if (isdigit(volser[i]))
        {
            volserMask[i] = '0';

            continue;
        }

        if ((volser[i] == '$') ||
            (volser[i] == '@') ||
            (volser[i] == '#'))
        {
            volserMask[i] = volser[i];

            continue;
        }
    }

    TRMSGI(TRCI_XAPI,
           "volser=%.6s, volserMask=%.6s\n", 
           volser,
           volserMask);

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_volser_increment                             */
/** Description:   Increment the volser in the input XAPIVRANGE.     */
/**                                                                  */
/** Increment the current volser in the input XAPIVRANGE.  If we     */
/** increment past the lastVol in the range, then set the            */
/** rangeCompletedFlag in the updated XAPIVRANGE.                    */
/**                                                                  */  
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_volser_increment"

extern void xapi_volser_increment(struct XAPIVRANGE *pXapivrange)
{
#define LAST_NUMERIC    "999999"
#define LAST_ALPHABETIC "ZZZZZZ"
#define NUMBER_RANGE    "0123456789"
#define ALPHA_RANGE     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

    int                 i;
    int                 j;
    char                matchFound;

#ifdef DEBUG

    TRMEMI(TRCI_XAPI, pXapivrange, sizeof(struct XAPIVRANGE),
           "Entered; pXapivrange:\n");

#endif

    /*****************************************************************/
    /* If 1st time for this XAPIVRANGE, then set currVol = firstVol. */
    /*****************************************************************/
    if (pXapivrange->currVol[0] < ' ')
    {
        memcpy(pXapivrange->currVol,
               pXapivrange->firstVol,
               sizeof(pXapivrange->currVol));

        return;
    }

    /*****************************************************************/
    /* Check if end of specified range for single volser range.      */
    /*****************************************************************/
    if ((memcmp(pXapivrange->currVol,
                pXapivrange->lastVol,
                sizeof(pXapivrange->currVol)) == 0) &&
        (memcmp(pXapivrange->currVol,
                pXapivrange->firstVol,
                sizeof(pXapivrange->currVol)) == 0))
    {
        TRMSGI(TRCI_XAPI,
               "At end of specified single volume range\n");

        pXapivrange->rangeCompletedFlag = TRUE;
    }

    /*****************************************************************/
    /* Increment numeric increments.                                 */
    /*****************************************************************/
    if (pXapivrange->mask[(pXapivrange->volIncIx)] == '0')
    {
        /*************************************************************/
        /* Check if at absolute end of numeric range.                */
        /*************************************************************/
        if (memcmp(&pXapivrange->currVol[(pXapivrange->volIncIx)],
                   LAST_NUMERIC, 
                   pXapivrange->volIncLen) == 0)
        {
            TRMSGI(TRCI_XAPI,
                   "At absolute end of numeric range\n");

            pXapivrange->rangeCompletedFlag = TRUE;

            return;
        }

        for (i = ((pXapivrange->volIncIx + pXapivrange->volIncLen) - 1);
            i >= pXapivrange->volIncIx;
            i --)
        {
            for (j = 0, matchFound = FALSE;
                j < strlen(NUMBER_RANGE);
                j++)
            {
                if (pXapivrange->currVol[i] == NUMBER_RANGE[j])
                {
                    matchFound = TRUE;

                    break;
                }
            }

            j++;

            if (j < strlen(NUMBER_RANGE))
            {
                pXapivrange->currVol[i] = NUMBER_RANGE[j];

                break;
            }

            pXapivrange->currVol[i] = '0';
        }
    }
    /*****************************************************************/
    /* Increment alphabetic increments.                              */
    /*****************************************************************/
    else
    {
        /*************************************************************/
        /* Check if at absolute end of alphabetic range.             */
        /*************************************************************/
        if (memcmp(&pXapivrange->currVol[(pXapivrange->volIncIx)],
                   LAST_ALPHABETIC, 
                   pXapivrange->volIncLen) == 0)
        {
            TRMSGI(TRCI_XAPI,
                   "At absolute end of alphabetic range\n");

            pXapivrange->rangeCompletedFlag = TRUE;

            return;
        }

        for (i = ((pXapivrange->volIncIx + pXapivrange->volIncLen) - 1);
            i >= pXapivrange->volIncIx;
            i --)
        {
            for (j = 0, matchFound = FALSE;
                j < strlen(ALPHA_RANGE);
                j++)
            {
                if (pXapivrange->currVol[i] == ALPHA_RANGE[j])
                {
                    matchFound = TRUE;

                    break;
                }
            }

            j++;

            if (j < strlen(ALPHA_RANGE))
            {
                pXapivrange->currVol[i] = ALPHA_RANGE[j];

                break;
            }

            pXapivrange->currVol[i] = 'A';
        }
    }

    /*****************************************************************/
    /* Check if end of specified multi-volser range.                 */
    /*****************************************************************/
    if (memcmp(pXapivrange->currVol,
               pXapivrange->lastVol,
               sizeof(pXapivrange->currVol)) > 0)
    {
        TRMSGI(TRCI_XAPI,
               "At end of specified multi-volume range\n");

        pXapivrange->rangeCompletedFlag = TRUE;
    }

    return;
}




