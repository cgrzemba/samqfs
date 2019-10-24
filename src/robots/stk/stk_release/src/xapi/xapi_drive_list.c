/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_drive_list.c                                */
/** Description:    Process the XAPI_DRIVE_LIST environment variable.*/
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


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: xapi_drive_list                                   */
/** Description:   Process the XAPI_DRIVE_LIST environment variable. */
/**                                                                  */
/** The XAPI_DRIVE_LIST environment variable is used to              */
/** control the building of the XAPI client tape drive               */
/** configuration table, XAPICFG.                                    */
/**                                                                  */
/** This routine reads the XAPI_DRIVE_LIST variable and builds a     */
/** linked list of XAPIDRANGE structures; the linked list of         */
/** XAPPIDRANGE structures is used to limit what server drives       */
/** are added to the XAPI client tape drive configuration table,     */
/** XAPICFG (also see xapi_drive).                                   */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "xapi_drive_list"

extern int xapi_drive_list(struct XAPICVT    *pXapicvt,
                           struct XAPIREQE   *pXapireqe,
                           struct XAPIDRLST **ptrXapidrlst,
                           int               *pNumXapidrlst)
{
#define ALL_DELIMITERS " ,=()\n"
#define MAX_DRIVE_LEN  16

    int                 lastRC;
    int                 tokenRC;
    int                 tokenNum            = 0;
    int                 currToken;
    int                 tokenIndex;
    int                 i;
    int                 j;
    int                 driveCount;
    int                 xapidrlstLen        = 0;

    short               currTokenLen        = 0;
    short               bypassChars         = 0;
    short               listDepth           = 0;
    short               firstDriveLen;
    short               lastDriveLen;

    char               *pNextPosOfCmdCopy;
    char               *pCurrToken          = NULL;
    char               *pCurrChar;
    char               *pDash;

    char                lenExceeded         = FALSE;
    char                withinQuotedString  = FALSE;

    char                cmdCopy[MAX_COMMAND_LEN + 1];
    char                begDelimiters[TOKEN_MAX_NUM];
    char                cmdTokens[TOKEN_MAX_NUM] [TOKEN_WIDTH];
    char                endDelimiters[TOKEN_MAX_NUM];

    struct XAPIDRANGE   wkXapidrange;
    struct XAPIDRANGE  *pHocXapidrange      = NULL;
    struct XAPIDRANGE  *pCurrXapidrange;
    struct XAPIDRANGE  *pEndXapidrange      = NULL;
    struct XAPIDRANGE  *pWkXapidrange       = &wkXapidrange;
    struct XAPIDRLST   *pXapidrlst          = NULL;
    struct XAPIDRLST   *pCurrXapidrlst;

    char                firstDrive[MAX_DRIVE_LEN + 1];
    char                lastDrive[MAX_DRIVE_LEN + 1];

    *pNumXapidrlst = 0;
    *ptrXapidrlst = NULL;

    if (getenv(XAPI_DRIVE_LIST) == NULL)
    {
        return STATUS_SUCCESS;
    }
    else
    {
        strcpy(cmdCopy, getenv(XAPI_DRIVE_LIST));
    }

    /*****************************************************************/
    /* Pre-analyze the XAPI_DRIVE_LIST variable before we do any     */
    /* tokenizing to                                                 */
    /* (1) Be able to handle our delimiter characters in quoted      */
    /*     strings (for instance a dataset name of the form          */
    /*     "DATA.SET.NAME(MEMBER)".                                  */
    /* (2) Be able to detect mismatched parentheses.                 */
    /* (3) Convert non-quoted substrings to upper case.              */
    /*****************************************************************/

    /*****************************************************************/
    /* (1) Convert any potential delimiter characters within         */
    /*     quotes or double quotes into a special character that     */
    /*     we can recognize after command tokenizing is complete.    */
    /*     We do this so that any delimiter characters ('=', '(',    */
    /*     etc.) that we really want to keep (for instance within    */
    /*     the READ DSN value, or within the MSGDEF PREFIX value)    */
    /*     will not be lost during the tokenizing operation below.   */
    /*****************************************************************/
    lastRC = FN_TOKEN_CONVERT_DELIMITERS_IN_QUOTES(cmdCopy, 
                                                   ALL_DELIMITERS);

    if (lastRC != RC_SUCCESS)
    {
        LOGMSG(STATUS_INVALID_COMMAND, 
               "XAPI_DRIVE_LIST FN_TOKEN_CONVERT_DELIMITERS_IN_QUOTES RC=%d",
               lastRC);

        return STATUS_INVALID_COMMAND;
    }

    /*****************************************************************/
    /* (2) Try to detect illogical parentheses or mismatched         */
    /*     parentheses.                                              */
    /*                                                               */
    /* NOTE: ELS syntax does not allow nested parentheses (unless    */
    /* they are in quoted strings).                                  */
    /*****************************************************************/
    lastRC = FN_TOKEN_TEST_MISMATCHED_PARENS(cmdCopy,
                                             1);

    if (lastRC != RC_SUCCESS)
    {
        LOGMSG(STATUS_INVALID_COMMAND, 
               "XAPI_DRIVE_LIST FN_TOKEN_TEST_MISMATCHED_PARENS RC=%d",
               lastRC);

        return STATUS_INVALID_COMMAND;
    }

    /*****************************************************************/
    /* (3) Convert any command text NOT enclosed in quotation marks  */
    /*     to upper case, and convert our special quote alias        */
    /*     characters to blanks.                                     */
    /*****************************************************************/
    TRMEMI(TRCI_XAPI, cmdCopy, strlen(cmdCopy),
           "Converted cmdCopy:\n");

    pCurrChar = cmdCopy;

    while (*pCurrChar != 0)
    {
        if (*pCurrChar == TOKEN_BEGQUOTE_ALIAS)
        {
            withinQuotedString = TRUE;
            *pCurrChar = ' ';

            TRMSGI(TRCI_XAPI, 
                   "Start of quoted substring=%08X\n",
                   pCurrChar);
        }
        else if (*pCurrChar == TOKEN_ENDQUOTE_ALIAS)
        {
            withinQuotedString = FALSE;
            *pCurrChar = ' ';

            TRMSGI(TRCI_XAPI,
                   "End of quoted substring=%08X\n",
                   pCurrChar);
        }
        else if (!(withinQuotedString))
        {
            *pCurrChar = toupper(*pCurrChar);
        }

        pCurrChar++;
    }

    /*****************************************************************/
    /* Initialize the token array, then tokenize the input command.  */
    /*****************************************************************/
    memset(&(cmdTokens[0] [0]), 
           ' ', 
           (TOKEN_MAX_NUM * TOKEN_WIDTH));

    pNextPosOfCmdCopy = cmdCopy;

    while (tokenNum < TOKEN_MAX_NUM)
    {
        tokenRC = FN_TOKEN_RETURN_NEXT(pNextPosOfCmdCopy,
                                       tokenNum,
                                       cmdTokens[tokenNum],
                                       begDelimiters,
                                       endDelimiters,
                                       &bypassChars);

        TRMSGI(TRCI_XAPI,
               "FN_TOKEN_RETURN_NEXT RC=%08X, tokenNum=%i, cmdToken=%s, "
               "begDelimiter=%c, endDelimiter=%c\n",
               tokenRC, 
               tokenNum, 
               cmdTokens[tokenNum],
               begDelimiters[tokenNum], 
               endDelimiters[tokenNum]);

        if (tokenRC == TOKEN_END_OF_LINE)
        {
            TRMSGI(TRCI_XAPI,
                   "Breaking at end-of-line\n");

            break;
        }
        else if (tokenRC == TOKEN_IS_NULL)
        {
            TRMSGI(TRCI_XAPI,
                   "Continuing after NULL token\n");

            pNextPosOfCmdCopy = &(pNextPosOfCmdCopy[bypassChars]);

            continue;
        }
        else if ((tokenRC == TOKEN_RETURNED) ||
                 (tokenRC == TOKEN_TRUNCATED))
        {
            if (tokenRC == TOKEN_TRUNCATED)
            {
                TRMSGI(TRCI_XAPI,
                       "Continuing with truncated token\n");

                lenExceeded = TRUE;
            }

            currToken = tokenNum;
            tokenNum++;

            if (currToken > 0)
            {
                TRMSGI(TRCI_XAPI,
                       "Non-NULL token; curr=%i, "
                       "token[prev]=%s, endDelimiter[prev]=%c, "
                       "token[curr]=%s, begDelimiter[curr]=%c "
                       "endDelimiter[curr]=%c\n",
                       currToken, 
                       cmdTokens[(currToken - 1)], 
                       endDelimiters[(currToken - 1)],
                       cmdTokens[currToken], 
                       begDelimiters[currToken], 
                       endDelimiters[currToken]);
            }

            /*********************************************************/
            /* If beginning of a value list, then insert the         */
            /* special beginning of list indicator, and              */
            /* bump the token count.                                 */
            /*********************************************************/
            if (((begDelimiters[currToken] == '(') ||
                 ((currToken > 0) &&
                  (endDelimiters[(currToken - 1)] == '('))) &&
                (endDelimiters[currToken] != ')'))
            {
                strcpy(cmdTokens[(currToken + 1)], cmdTokens[currToken]);
                begDelimiters[(currToken + 1)] = begDelimiters[currToken];
                endDelimiters[(currToken + 1)] = endDelimiters[currToken];

                strcpy(cmdTokens[currToken], TOKEN_BEGLIST);
                begDelimiters[currToken] = ' ';
                endDelimiters[currToken] = ' ';
                listDepth++;
                tokenNum++;

                TRMSGI(TRCI_XAPI,
                       "Starting possible keyword value list; depth=%i, "
                       "new tokenNum=%i, token[n-1]=%s\n",
                       listDepth,
                       tokenNum,
                       cmdTokens[(tokenNum - 1)]);
            }

            /*********************************************************/
            /* If ending of a value list, then append the            */
            /* special end of list indicator, and                    */
            /* bump the token count.                                 */
            /*********************************************************/
            if (((begDelimiters[currToken] != '(') ||
                 ((currToken > 0) &&
                  (endDelimiters[(currToken - 1)] != '('))) &&
                (endDelimiters[currToken] == ')') &&
                (listDepth > 0))
            {
                strcpy(cmdTokens[(currToken + 1)], TOKEN_ENDLIST);
                begDelimiters[(currToken + 1)] = ' ';
                endDelimiters[(currToken + 1)] = ')';

                TRMSGI(TRCI_XAPI,
                       "Ending possible keyword value list; depth=%i\n",
                       listDepth);

                listDepth--;
                tokenNum++;
            }

            pNextPosOfCmdCopy = &(pNextPosOfCmdCopy[bypassChars]);

        }
        else
        {
            LOGMSG(STATUS_INVALID_COMMAND, 
                   "XAPI_DRIVE_LIST FN_TOKEN_RETURN_NEXT RC=%d",
                   lastRC);

            return STATUS_INVALID_COMMAND;
        }
    }

    /*****************************************************************/
    /* After the command line is tokenized, we re-convert any        */
    /* previously converted delimiter characters (that were          */
    /* really part of a specified value and were not really          */
    /* delimiters) back to their original value.                     */
    /*****************************************************************/
    for (tokenIndex = 0; 
        tokenIndex < tokenNum; 
        tokenIndex++)
    {
        for (i = 0; 
            i < strlen(cmdTokens[tokenIndex]); 
            i++)
        {
            if (cmdTokens[tokenIndex] [i] < strlen(ALL_DELIMITERS))
            {
                j = (cmdTokens[tokenIndex] [i]) - 1;
                cmdTokens[tokenIndex] [i] = ALL_DELIMITERS[j];
            }
        }
    }

    /*****************************************************************/
    /* Display the tokenizing results:                               */
    /*****************************************************************/
    TRMSGI(TRCI_XAPI,
           "Results from tokenizing; tokenNum=%i\n", 
           tokenNum);

    for (i = 0; 
        i < tokenNum; 
        i++)
    {
        TRMSGI(TRCI_XAPI,
               "Token=%i length=%i, string=%s\n", 
               i, 
               strlen(cmdTokens[i]), 
               cmdTokens[i]);
    }

    /*************************************************************/
    /* Test for 0 tokens (i.e. a blank command line).            */
    /*************************************************************/
    if (tokenNum == 0)
    {
        LOGMSG(STATUS_INVALID_COMMAND, 
               "XAPI_DRIVE_LIST blank\n");

        return STATUS_INVALID_COMMAND;
    }
    /*************************************************************/
    /* Test for command line truncation (i.e too many tokens).   */
    /*************************************************************/
    else if (tokenNum >= TOKEN_MAX_NUM)
    {
        TRMSGI(TRCI_XAPI,
               "XAPI_DRIVE_LIST command truncated; continuing\n");
    }

    /*****************************************************************/
    /* If tokenizing was successful:                                 */
    /* Test for token truncation (i.e token too long).               */
    /*****************************************************************/
    if (lenExceeded)
    {
        TRMSGI(TRCI_XAPI,
               "XAPI_DRIVE_LIST token truncated; continuing\n");
    }

    /*****************************************************************/
    /* Process each of the tokens which should be a single drive or  */
    /* drive range, and build a linked list of XAPIDRANGE entries    */
    /* for each drive or range.                                      */
    /*****************************************************************/
    for (i = 0;
        i < tokenNum;
        i++)
    {
        memset((char*) pWkXapidrange, 0 , sizeof(struct XAPIDRANGE)); 
        memset(firstDrive, 0 , sizeof(firstDrive));
        memset(lastDrive, 0 , sizeof(lastDrive));
        currTokenLen = strlen(cmdTokens[i]);
        pDash = strchr(cmdTokens[i], '-');

        if (pDash != NULL)
        {
            firstDriveLen = (pDash - &(cmdTokens[i] [0]));    

            if (firstDriveLen > MAX_DRIVE_LEN)
            {
                firstDriveLen = MAX_DRIVE_LEN;
            }

            memcpy(firstDrive,
                   cmdTokens[i],
                   firstDriveLen);

            lastDriveLen = currTokenLen - firstDriveLen - 1;

            if (lastDriveLen > MAX_DRIVE_LEN)
            {
                lastDriveLen = MAX_DRIVE_LEN;
            }

            memcpy(lastDrive,
                   &(pDash[1]),
                   lastDriveLen);
        }
        else
        {
            firstDriveLen = currTokenLen;
            lastDriveLen = 0;

            if (firstDriveLen > MAX_DRIVE_LEN)
            {
                firstDriveLen = MAX_DRIVE_LEN;
            }

            memcpy(firstDrive,
                   cmdTokens[i],
                   firstDriveLen);
        }

        memcpy(pWkXapidrange->firstDrive, 
               firstDrive,
               firstDriveLen);

        if (lastDriveLen > 0)
        {
            memcpy(pWkXapidrange->lastDrive, 
                   lastDrive,
                   lastDriveLen);
        }

        lastRC = xapi_validate_drive_range(pWkXapidrange);

        if (lastRC == STATUS_SUCCESS)
        {
            pCurrXapidrange = (struct XAPIDRANGE*) malloc(sizeof(struct XAPIDRANGE));

            TRMSGI(TRCI_STORAGE,
                   "malloc XAPIDRANGE=%08X, len=%i\n",
                   pCurrXapidrange,
                   sizeof(struct XAPIDRANGE));

            memcpy((char*) pCurrXapidrange,
                   (char*) pWkXapidrange,
                   sizeof(struct XAPIDRANGE));

            if (pEndXapidrange == NULL)
            {
                pHocXapidrange = pCurrXapidrange;
                pEndXapidrange = pCurrXapidrange;
            }
            else
            {
                pEndXapidrange->pNextXapidrange = pCurrXapidrange;
                pEndXapidrange = pCurrXapidrange;
            }
        }
        /*************************************************************/
        /* If the drive or range token is invalid, just ignore it    */
        /* and try to process the remainder of the drive list.       */
        /*************************************************************/
        else
        {
            LOGMSG(STATUS_INVALID_COMMAND, 
                   "XAPI_DRIVE_LIST token %s invalid\n",
                   cmdTokens[i]);
        }
    }

    /*****************************************************************/
    /* Now run through the XAPIDRANGE linked list entries to count   */
    /* the actual number of drives.                                  */
    /*****************************************************************/
    pCurrXapidrange = pHocXapidrange;
    driveCount = 0;

    while (pCurrXapidrange != NULL)
    {
        driveCount += pCurrXapidrange->numInRange;

        pCurrXapidrange = pCurrXapidrange->pNextXapidrange;
    }

    TRMSGI(TRCI_XAPI,
           "driveCount=%d\n",
           driveCount);

    /*****************************************************************/
    /* Now allocate a single block of XAPIDRLST structures, and      */
    /* initialize with the appropriate drive identifier.             */
    /*****************************************************************/
    xapidrlstLen = driveCount * sizeof(struct XAPIDRLST);

    pXapidrlst = (struct XAPIDRLST*) malloc(xapidrlstLen);

    TRMSGI(TRCI_STORAGE,
           "malloc XAPIDRLST=%08X, len=%i\n",
           pXapidrlst,
           xapidrlstLen);

    memset((char*) pXapidrlst, 0, xapidrlstLen);

    pCurrXapidrange = pHocXapidrange;
    i = 0;

    while (pCurrXapidrange != NULL)
    {
        while (1)
        {
            xapi_drive_increment(pCurrXapidrange);

            if (pCurrXapidrange->rangeCompletedFlag)
            {
                break;
            }

            pCurrXapidrlst = &(pXapidrlst[i]);

            if (pCurrXapidrange->driveFormatFlag == DRIVE_CCUU_FORMAT)
            {
                memcpy(pCurrXapidrlst->driveName,
                       pCurrXapidrange->currDrive,
                       sizeof(pCurrXapidrlst->driveName));
            }
            else
            {
                memcpy(pCurrXapidrlst->driveLocId,
                       pCurrXapidrange->currDrive,
                       sizeof(pCurrXapidrlst->driveLocId));
            }

            i++;
        }

        pCurrXapidrange = pCurrXapidrange->pNextXapidrange;
    }

    TRMEMI(TRCI_XAPI, pXapidrlst, xapidrlstLen,
           "XAPIDRLST\n");

    *pNumXapidrlst = driveCount;
    *ptrXapidrlst = pXapidrlst;

    /*****************************************************************/
    /* At this point we can free the XAPIDRANGE linked list entries. */
    /*****************************************************************/
    pCurrXapidrange = pHocXapidrange;

    while (pCurrXapidrange != NULL)
    {
        pEndXapidrange = pCurrXapidrange;
        pCurrXapidrange = pCurrXapidrange->pNextXapidrange;

        TRMSGI(TRCI_STORAGE,
               "free XAPIDRANGE=%08X, len=%i\n",
               pEndXapidrange,
               sizeof(struct XAPIDRANGE));

        free(pEndXapidrange);
    }

    return STATUS_SUCCESS;
}






