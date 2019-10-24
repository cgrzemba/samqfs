/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_drive.c                                     */
/** Description:    XAPI client drive validation, and drive-range    */
/**                 increment service.                               */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     08/15/11                          */
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
#include "smccxmlx.h"
#include "xapi.h"
#include "api/defs_api.h"
#include "csi.h"


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void convertDriveToMask(char drive[16],
                               char driveMask[16]);


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: xapi_validate_single_drive                        */
/** Description:   Validate the format of the input drive.           */
/**                                                                  */
/** Validate the format of the single input drive (character).       */
/** The input drive may have 1 of 3 formats:                         */
/** (1) R:AA:LL:PP:DD (real location id format)                      */
/** (2) V:VTSSNAME:NNN (virtual location id format)                  */
/** (3) CCUU (hexadecimal drive address format)                      */
/**                                                                  */
/** Return codes are:                                                */
/** STATUS_SUCCESS: The drive was valid.                             */
/** STATUS_INVALID_VALUE The drive was invalid,                      */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "xapi_validate_single_drive"

extern int xapi_validate_single_drive(char  drive[16],
                                      char *driveFormatFlag)
{
    int                 lastRC;
    int                 i;
    int                 j;
    int                 len;
    int                 wkInt;
    char                wkCuu[4];

    *driveFormatFlag = 0;

    /*****************************************************************/
    /* If real location id format, then there must be 3 additional   */
    /* colons (":") with intervening numeric characters exactly      */
    /* 2 digits in length with exactly 2 digits following last       */
    /* colon (i.e. "R:NN:NN:NN:NN").                                 */
    /*                                                               */
    /* When converted to an increment mask, only the last 2          */
    /* positions (the drive number) will be incremented, and the     */
    /* mask will be "R:NN:NN:NN:00".                                 */
    /*****************************************************************/
    if (memcmp(drive,
               "R:",
               2) == 0)
    {
        /*************************************************************/
        /* Validate ACS portion of real location id form.            */
        /*************************************************************/
        for (i = 2, len = 0;
            i < 15;
            i++)
        {
            if (drive[i] == ':')
            {
                break;
            }

            if (isdigit(drive[i]))
            {
                len++;

                if (len > 2)
                {
                    TRMSGI(TRCI_XAPI,
                           "ACS length too long\n");

                    return STATUS_INVALID_VALUE;
                }
            }
            else
            {
                TRMSGI(TRCI_XAPI,
                       "ACS not numeric at position %d\n",
                       (i - 2));

                return STATUS_INVALID_VALUE;
            }
        }

        if (len != 2)
        {
            TRMSGI(TRCI_XAPI,
                   "ACS length invalid\n");

            return STATUS_INVALID_VALUE;
        }

        /*************************************************************/
        /* Validate LSM portion of real location id form.            */
        /*************************************************************/
        for (i++, j = i, len = 0;
            i < 15;
            i++)
        {
            if (drive[i] == ':')
            {
                break;
            }

            if (isdigit(drive[i]))
            {
                len++;

                if (len > 2)
                {
                    TRMSGI(TRCI_XAPI,
                           "LSM length too long\n");

                    return STATUS_INVALID_VALUE;
                }
            }
            else
            {
                TRMSGI(TRCI_XAPI,
                       "LSM not numeric at position %d\n",
                       (i - j));

                return STATUS_INVALID_VALUE;
            }
        }

        if (len != 2)
        {
            TRMSGI(TRCI_XAPI,
                   "LSM length invalid, len=%d, i=%d, j=%d\n",
                   len,
                   i,
                   j);

            return STATUS_INVALID_VALUE;
        }

        /*************************************************************/
        /* Validate PANEL portion of real location id form.          */
        /*************************************************************/
        for (i++, j = i, len = 0;
            i < 15;
            i++)
        {
            if (drive[i] == ':')
            {
                break;
            }

            if (isdigit(drive[i]))
            {
                len++;

                if (len > 2)
                {
                    TRMSGI(TRCI_XAPI,
                           "PANEL length too long\n");

                    return STATUS_INVALID_VALUE;
                }
            }
            else
            {
                TRMSGI(TRCI_XAPI,
                       "PANEL not numeric at position %d\n",
                       (i - j));

                return STATUS_INVALID_VALUE;
            }
        }

        if (len != 2)
        {
            TRMSGI(TRCI_XAPI,
                   "PANEL length invalid\n");

            return STATUS_INVALID_VALUE;
        }

        /*************************************************************/
        /* Validate DRIVE portion of real location id form.          */
        /*************************************************************/
        for (i++, j = i, len = 0;
            i < 15;
            i++)
        {
            if (drive[i] == 0)
            {
                break;
            }

            if (isdigit(drive[i]))
            {
                len++;

                if (len > 2)
                {
                    TRMSGI(TRCI_XAPI,
                           "DRIVE length too long\n");

                    return STATUS_INVALID_VALUE;
                }
            }
            else
            {
                TRMSGI(TRCI_XAPI,
                       "DRIVE not numeric at position %d\n",
                       (i - j));

                return STATUS_INVALID_VALUE;
            }
        }

        if (len != 2)
        {
            TRMSGI(TRCI_XAPI,
                   "DRIVE length invalid\n");

            return STATUS_INVALID_VALUE;
        }

        *driveFormatFlag = DRIVE_RCOLON_FORMAT;
    }
    /*****************************************************************/
    /* If virtual location id format, then there must be 2 colons    */
    /* (":") with intervening national characters no longer than 8   */
    /* characters in length (VTSSNAME) with exactly 3 digits         */
    /* following the last colon.                                     */
    /*                                                               */
    /* When converted to an increment mask, only the last 3         */
    /* positions (the drive number) will be incremented, and the     */
    /* mask will be "V:AAAAAA:000".                                  */
    /*****************************************************************/
    else if (memcmp(drive,
                    "V:",
                    2) == 0)
    {
        /*************************************************************/
        /* Validate VTSSNAME portion of virtual location id form.    */
        /*************************************************************/
        for (i = 2, len = 0;
            i < 15;
            i++)
        {
            if (drive[i] == ':')
            {
                break;
            }

            len++;

            if (len > 8)
            {
                TRMSGI(TRCI_XAPI,
                       "VTSSNAME length too long\n");

                return STATUS_INVALID_VALUE;
            }
        }
        /*************************************************************/
        /* Validate DRIVE portion of real location id form.          */
        /*************************************************************/
        for (i++, j = i, len = 0;
            i < 15;
            i++)
        {
            if (drive[i] == 0)
            {
                break;
            }

            if (isdigit(drive[i]))
            {
                len++;

                if (len > 3)
                {
                    TRMSGI(TRCI_XAPI,
                           "DRIVE length too long\n");

                    return STATUS_INVALID_VALUE;
                }
            }
            else
            {
                TRMSGI(TRCI_XAPI,
                       "DRIVE not numeric at position %d\n",
                       (i - j));

                return STATUS_INVALID_VALUE;
            }
        }

        if (len != 3)
        {
            TRMSGI(TRCI_XAPI,
                   "DRIVE length invalid\n");

            return STATUS_INVALID_VALUE;
        }

        *driveFormatFlag = DRIVE_VCOLON_FORMAT;
    }
    /*****************************************************************/
    /* Otherwise, drive must be in CUU or CCUU format.               */
    /*                                                               */
    /* When converted to an increment mask, the 4 position           */
    /* CCUU will be incremented as hexadecimal and the mask will     */
    /* be "XXXX".                                                    */
    /*****************************************************************/
    else
    {
        if (drive[3] == 0)
        {
            memset(wkCuu, 0, sizeof(wkCuu));

            memcpy(wkCuu,
                   drive,
                   3);

            strcpy(drive, "0");
            strcat(drive, wkCuu);

            TRMSGI(TRCI_XAPI,
                   "Converted CUU to CCUU=%.4s\n",
                   drive);
        }

        lastRC = FN_CONVERT_CHARHEX_TO_FULLWORD(drive,
                                                4,
                                                &wkInt);

        if (lastRC != STATUS_SUCCESS)
        {
            TRMSGI(TRCI_XAPI,
                   "CUU or CCUU not hexadecimal, len=%d\n",
                   len);

            return STATUS_INVALID_VALUE;
        }

        *driveFormatFlag = DRIVE_CCUU_FORMAT;
    }

    return STATUS_SUCCESS;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: xapi_validate_drive_range                         */
/** Description:   Validate the input drive range.                   */
/**                                                                  */
/** Validate that the input drive(s) specify either 1 valid drive    */
/** or a valid drive range, and return the corresponding             */
/** XAPIDRANGE structure.                                            */
/**                                                                  */
/** A drive range may be a single drive range when the firstDrive    */
/** is equal to the lastDrive.                                       */
/**                                                                  */
/** Return codes are:                                                */
/** STATUS_SUCCESS: The drive or range was invalid.                  */
/** STATUS_INVALID_VALUE: A drive was invalid.                       */
/** STATUS_INVALID_RANGE: The range was invalid.                     */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "xapi_validate_drive_range"

extern int xapi_validate_drive_range(struct XAPIDRANGE *pXapidrange)
{
    int                 lastRC;
    int                 i;
    char                firstDriveMask[16];
    char                lastDriveMask[16];
    char                firstDriveFormat;
    char                lastDriveFormat;
    int                 currMaskRightPos;  
    char                currMask;
    char                differenceFound     = FALSE;

    /*****************************************************************/
    /* Validate the first drive of the range, or the only drive.     */
    /*****************************************************************/
    lastRC = xapi_validate_single_drive(pXapidrange->firstDrive,
                                        &firstDriveFormat);

    TRMSGI(TRCI_XAPI,
           "RC from xapi_validate_single_drive(%.16s)=%d, format=%c\n", 
           pXapidrange->firstDrive,
           lastRC,
           firstDriveFormat);

    if (lastRC != STATUS_SUCCESS)
    {
        return STATUS_INVALID_VALUE;
    }

    if ((pXapidrange->lastDrive[0] <= ' ') ||
        (memcmp(pXapidrange->firstDrive,
                pXapidrange->lastDrive,
                sizeof(pXapidrange->firstDrive)) == 0))
    {
        memcpy(pXapidrange->lastDrive,
               pXapidrange->firstDrive,
               sizeof(pXapidrange->firstDrive));

        TRMSGI(TRCI_XAPI,
               "lastDrive same as firstDrive or not specified\n");

        pXapidrange->numInRange = 1;
        pXapidrange->driveFormatFlag = firstDriveFormat;

        return STATUS_SUCCESS;
    }

    lastRC = xapi_validate_single_drive(pXapidrange->lastDrive,
                                        &lastDriveFormat);

    TRMSGI(TRCI_XAPI,
           "RC from xapi_validate_single_drive(%.16s)=%d, format=%c\n", 
           pXapidrange->lastDrive,
           lastRC,
           lastDriveFormat);

    if (lastRC != STATUS_SUCCESS)
    {
        return STATUS_INVALID_VALUE;
    }

    if (memcmp(pXapidrange->firstDrive,
               pXapidrange->lastDrive,
               sizeof(pXapidrange->firstDrive)) > 0)
    {
        return STATUS_INVALID_RANGE;
    }

    if (firstDriveFormat != lastDriveFormat)
    {
        return STATUS_INVALID_RANGE;
    }

    convertDriveToMask(pXapidrange->firstDrive,
                       firstDriveMask);

    convertDriveToMask(pXapidrange->lastDrive,
                       lastDriveMask);

    if (strcmp(firstDriveMask,
               lastDriveMask) != 0)
    {
        TRMSGI(TRCI_XAPI,
               "firstDriveMask=%.16s != lastDriveMask=%.16s\n", 
               firstDriveMask,
               lastDriveMask);

        return STATUS_INVALID_RANGE;
    }

    /*****************************************************************/
    /* Now find the starting position of the drive increment and     */
    /* its range.                                                    */
    /*****************************************************************/
    currMaskRightPos = 15;
    currMask = firstDriveMask[15];

    for (i = 15; 
        i >= 0;
        i--)
    {
        if (firstDriveMask[i] != currMask)
        {
            if (differenceFound)
            {
                break;
            }

            currMaskRightPos = i;
            currMask = firstDriveMask[i];
        }

        if (pXapidrange->firstDrive[i] != pXapidrange->lastDrive[i])
        {
            differenceFound = TRUE;
        }
    }

    pXapidrange->driveIncIx = i + 1;
    pXapidrange->driveIncLen = currMaskRightPos - i; 

    TRMSGI(TRCI_XAPI,
           "firstDriveMask=%.16s, currMaskRightPos=%d, "
           "i=%d, currMask=%c, incIx=%d, incLen=%d\n", 
           firstDriveMask,
           currMaskRightPos, 
           i, 
           currMask,
           pXapidrange->driveIncIx,
           pXapidrange->driveIncLen);

    memcpy(pXapidrange->mask,
           firstDriveMask,
           sizeof(pXapidrange->mask));

    /*****************************************************************/
    /* Now validate that we have only 1 logical increment in range.  */
    /*****************************************************************/
    for (; 
        i >= 0;
        i--)
    {
        if (pXapidrange->firstDrive[i] != pXapidrange->lastDrive[i])
        {
            TRMSGI(TRCI_XAPI,
                   "Another logical increment detected at position=%d\n",
                   i);

            return STATUS_INVALID_RANGE;
        }
    }

    /*****************************************************************/
    /* Now find out how many drives there are in the range.          */
    /*****************************************************************/
    memset(pXapidrange->currDrive, 0, sizeof(pXapidrange->currDrive));

    while (1)
    {
        xapi_drive_increment(pXapidrange);

        if (pXapidrange->rangeCompletedFlag)
        {
            break;
        }

        pXapidrange->numInRange++;
    }

    memset(pXapidrange->currDrive, 0, sizeof(pXapidrange->currDrive));
    pXapidrange->rangeCompletedFlag = FALSE;

    TRMSGI(TRCI_XAPI,
           "numInRange=%d\n",
           pXapidrange->numInRange);

    return STATUS_SUCCESS;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: convertDriveToMask                                */
/** Description:   Convert a drive into a drive range "mask".        */
/**                                                                  */
/** A drive range mask indicates the type of characters in the       */
/** drive identifier; either an increment, non-increment, or colon.  */
/** The mask is used to determine whether a valid range has been     */
/** specified, and how the range will be incremented.                */
/**                                                                  */
/** If a range of drives is to be processed, then the first drive    */
/** and last drive in the range must have an identical mask.         */
/**                                                                  */
/** As an example, if the first drive in the range is specified as   */
/** "V:aaaaaa:nnn" then its mask will be "V:AAAAAA:000" which        */
/** represents 9 characters non-incremental followed by 3 numerics.  */
/** The ending drive in this range must have the identical mask,     */
/** "V:AAAAAA:000".                                                  */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "convertDriveToMask"

static void convertDriveToMask(char drive[16],
                               char driveMask[16])
{
    int                 len;
    char               *posColon2;

    /*****************************************************************/
    /* If real location id format, then drive mask will be           */
    /* R:NN:NN:NN:00 with last 2 digits being numeric mask.          */
    /*****************************************************************/
    if (memcmp(drive,
               "R:",
               2) == 0)
    {
        strcpy(driveMask, "R:NN:NN:NN:00");
    }
    /*****************************************************************/
    /* If virtual location id format, then drive mask will be        */
    /* V:AAAAAA:000 with last 3 digits being numeric mask.           */
    /*****************************************************************/
    else if (memcmp(drive,
                    "V:",
                    2) == 0)
    {
        memset(driveMask, 0, sizeof(driveMask));
        strcpy(driveMask, "V:");
        posColon2 = strrchr(drive, ':');
        memset(&(driveMask[2]), 'A', (posColon2 - &(drive[2])));
        strcat(driveMask, ":000");
    }
    /*****************************************************************/
    /* Otherwise, it is CCUU format.                                 */
    /*****************************************************************/
    else
    {
        strcpy(driveMask, "XXXX");
    }

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_drive_increment                              */
/** Description:   Increment the drive in the input XAPIDRANGE.      */
/**                                                                  */
/** Increment the current drive in the input XAPIDRANGE.  If we      */
/** increment past the lastDrive in the range, then set the          */
/** rangeCompletedFlag in the updated XAPIDRANGE.                    */
/**                                                                  */ 
/** The increment will either be simple numeric "00" or "000" mask,  */
/** or hexadecimal "XXXX" mask.                                      */
/**                                                                  */ 
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_drive_increment"

extern void xapi_drive_increment(struct XAPIDRANGE *pXapidrange)
{
#define LAST_NUMERIC        "9999"
#define LAST_HEXADECIMAL    "FFFF"
#define NUMBER_RANGE        "0123456789"
#define HEXADECIMAL_RANGE   "0123456789ABCDEF"

    int                 i;
    int                 j;
    char                matchFound;

#ifdef DEBUG

    TRMEMI(TRCI_XAPI, pXapidrange, sizeof(struct XAPIDRANGE),
           "Entered; pXapidrange:\n");

#endif

    /*****************************************************************/
    /* If 1st time for this XAPIDRANGE, then set currDrive =         */
    /* firstDrive.                                                   */
    /*****************************************************************/
    if (pXapidrange->currDrive[0] < ' ')
    {
        memcpy(pXapidrange->currDrive,
               pXapidrange->firstDrive,
               sizeof(pXapidrange->currDrive));

        return;
    }

    /*****************************************************************/
    /* Check if end of specified range for single drive range.       */
    /*****************************************************************/
    if ((memcmp(pXapidrange->currDrive,
                pXapidrange->lastDrive,
                sizeof(pXapidrange->currDrive)) == 0) &&
        (memcmp(pXapidrange->currDrive,
                pXapidrange->firstDrive,
                sizeof(pXapidrange->currDrive)) == 0))
    {
        TRMSGI(TRCI_XAPI,
               "At end of specified single drive range\n");

        pXapidrange->rangeCompletedFlag = TRUE;
    }

    /*****************************************************************/
    /* Increment numeric increments.                                 */
    /*****************************************************************/
    if (pXapidrange->mask[(pXapidrange->driveIncIx)] == '0')
    {
        /*************************************************************/
        /* Check if at absolute end of numeric range.                */
        /*************************************************************/
        if (memcmp(&pXapidrange->currDrive[(pXapidrange->driveIncIx)],
                   LAST_NUMERIC, 
                   pXapidrange->driveIncLen) == 0)
        {
            TRMSGI(TRCI_XAPI,
                   "At absolute end of numeric range\n");

            pXapidrange->rangeCompletedFlag = TRUE;

            return;
        }

        for (i = ((pXapidrange->driveIncIx + pXapidrange->driveIncLen) - 1);
            i >= pXapidrange->driveIncIx;
            i --)
        {
            for (j = 0, matchFound = FALSE;
                j < strlen(NUMBER_RANGE);
                j++)
            {
                if (pXapidrange->currDrive[i] == NUMBER_RANGE[j])
                {
                    matchFound = TRUE;

                    break;
                }
            }

            j++;

            if (j < strlen(NUMBER_RANGE))
            {
                pXapidrange->currDrive[i] = NUMBER_RANGE[j];

                break;
            }

            pXapidrange->currDrive[i] = '0';
        }
    }
    /*****************************************************************/
    /* Increment hexadecimal increments.                             */
    /*****************************************************************/
    else
    {
        /*************************************************************/
        /* Check if at absolute end of hexadecimal range.            */
        /*************************************************************/
        if (memcmp(&pXapidrange->currDrive[(pXapidrange->driveIncIx)],
                   LAST_HEXADECIMAL, 
                   pXapidrange->driveIncLen) == 0)
        {
            TRMSGI(TRCI_XAPI,
                   "At absolute end of hexadecimal range\n");

            pXapidrange->rangeCompletedFlag = TRUE;

            return;
        }

        for (i = ((pXapidrange->driveIncIx + pXapidrange->driveIncLen) - 1);
            i >= pXapidrange->driveIncIx;
            i --)
        {
            for (j = 0, matchFound = FALSE;
                j < strlen(HEXADECIMAL_RANGE);
                j++)
            {
                if (pXapidrange->currDrive[i] == HEXADECIMAL_RANGE[j])
                {
                    matchFound = TRUE;

                    break;
                }
            }

            j++;

            if (j < strlen(HEXADECIMAL_RANGE))
            {
                pXapidrange->currDrive[i] = HEXADECIMAL_RANGE[j];

                break;
            }

            pXapidrange->currDrive[i] = '0';
        }
    }

    /*****************************************************************/
    /* Check if end of specified multi-drive range.                  */
    /*****************************************************************/
    if (memcmp(pXapidrange->currDrive,
               pXapidrange->lastDrive,
               sizeof(pXapidrange->currDrive)) > 0)
    {
        TRMSGI(TRCI_XAPI,
               "At end of specified multi-drive range\n");

        pXapidrange->rangeCompletedFlag = TRUE;
    }

    return;
}


