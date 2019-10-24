/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      smcstok.c                                        */
/** Description:    XAPI client command line character string        */
/**                 tokenizing service.                              */
/**                                                                  */
/**                 NOTE: Tokenizing rules for the command line      */
/**                 strings is the same as for SMC MVS.              */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     05/15/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/***END PROLOGUE******************************************************/


/*********************************************************************/
/* Includes:                                                         */
/*********************************************************************/
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "smccxmlx.h"
#include "srvcommon.h"
#include "xapi.h"


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: FN_TOKEN_TEST_DELIMITERS_IN_QUOTES                */
/** Description:   Pre-process quoted substrings.                    */
/**                                                                  */
/** Pre-process the input string, looking for potential delimiters   */
/** within quoted substrings.  These delimiters (which are part      */
/** of a parameter value and are not really delimiters) will be      */
/** converted into a special character that will later be            */
/** re-converted back into its original delimiter, after command     */ 
/** line tokenizing is complete.                                     */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "FN_TOKEN_CONVERT_DELIMITERS_IN_QUOTES"

extern int FN_TOKEN_CONVERT_DELIMITERS_IN_QUOTES(char *pUnparsedCommand,
                                                 char *pDelimiters)
{
    char               *pFirstQuote;
    char               *pFirstDblQuote;
    char               *pCurrChar;
    char                quoteChar;
    int                 i;

    pCurrChar = pUnparsedCommand;

    while (*pCurrChar != 0)
    {
        /*************************************************************/
        /* Begin the scan for delimiters to convert after the first  */
        /* double quote or single quote character found.  If both    */
        /* a double quote and a single quote are found, begin with   */
        /* the one found 1st (leftmost) within the line.             */
        /*************************************************************/
        pFirstDblQuote = strchr(pCurrChar, '"');
        pFirstQuote = strchr(pCurrChar, '\'');

        if ((pFirstDblQuote == NULL) &&
            (pFirstQuote == NULL))
        {
            return RC_SUCCESS;
        }

        if (pFirstDblQuote != NULL)
        {
            quoteChar = '"';
            pCurrChar = pFirstDblQuote;

            if (pFirstQuote != NULL)
            {
                if (pFirstQuote < pFirstDblQuote)
                {
                    quoteChar = '\'';
                    pCurrChar = pFirstQuote;
                }
            }
        }
        else
        {
            quoteChar = '\'';
            pCurrChar = pFirstQuote;
        }

        /*************************************************************/
        /* Convert the starting double quotes (or single quote) to   */
        /* our special beginning quote alias character.              */
        /*************************************************************/
        *pCurrChar = TOKEN_BEGQUOTE_ALIAS;
        pCurrChar++;

        /*************************************************************/
        /* If we have 2 double quotes or 2 single quotes in a row,   */
        /* then we must have 3 in a row, or else it's an error.      */
        /* We can have ''' or """ but not ''x.                       */
        /* In this case, the 2 outer quotes are converted to our     */
        /* special beginning and ending quote alias character,       */
        /* eventually leaving only the middle quote.                 */
        /*************************************************************/
        if (*pCurrChar == quoteChar)
        {
            pCurrChar++;

            if (*pCurrChar != quoteChar)
            {
                return RC_FAILURE;
            }
            else
            {
                *pCurrChar = TOKEN_ENDQUOTE_ALIAS;
            }
        }
        /*************************************************************/
        /* We found the start of a quoted string.  Convert any       */
        /* delimiters found within the string (between the 1st quote */
        /* found and the next matching quote).  We convert the       */
        /* delimiter character into its index within the delimiter   */
        /* string + 1.  Thus if the 1st delimiter character defined  */
        /* is an '=' sign, we will convert that '=' sign into a      */
        /* X'01' within the string.  Later, after tokenizing is      */
        /* complete, we will re-convert any X'01' found in the       */
        /* tokens back to an '='.                                    */
        /*************************************************************/
        else
        {
            while (1)
            {
                /*****************************************************/
                /* If we reach the end of the line before we find    */
                /* a matching quote, we have unmatched quotes, which */
                /* is an error.                                      */
                /*****************************************************/
                if (*pCurrChar == 0)
                {
                    return RC_FAILURE;
                }
                if (*pCurrChar == quoteChar)
                {
                    *pCurrChar = TOKEN_ENDQUOTE_ALIAS;
                    pCurrChar++;

                    break;
                }

                for (i = 0; 
                    i < strlen(pDelimiters); 
                    i++)
                {
                    if (*pCurrChar == pDelimiters[i])
                    {
                        *pCurrChar = i+1;
                    }
                }

                pCurrChar++;
            }
        }
    }

    return RC_SUCCESS;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: FN_TOKEN_TEST_MISMATCHED_PARENS                   */
/** Description:   Pre-process mismatched parenthesis.               */
/**                                                                  */
/** Pre-process the command line, looking for potential parenthesis  */
/** problems and mismatches.                                         */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "FN_TOKEN_TEST_MISMATCHED_PARENS"

extern int FN_TOKEN_TEST_MISMATCHED_PARENS(char  *pUnparsedCommand,
                                           short  maxParenDepth)
{
    char               *pCurrChar;
    short               parenDepth          = 0;

    pCurrChar = pUnparsedCommand;

    while (*pCurrChar != 0)
    {
        if (*pCurrChar == '(')
        {
            parenDepth++;

            if (parenDepth > maxParenDepth)
            {
                return RC_FAILURE;
            }
        }

        if (*pCurrChar == ')')
        {
            parenDepth--;

            if (parenDepth < 0)
            {
                return RC_FAILURE;
            }
        }

        *pCurrChar++;
    }

    if (parenDepth != 0)
    {
        return RC_FAILURE;
    }

    return RC_SUCCESS;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: FN_TOKEN_REMOVE_PAREN_STRINGS                     */
/** Description:   Pre-process matched parenthesis.                  */
/**                                                                  */
/** Pre-process the command line, looking for matched parenthesis.   */
/** Logically remove the parenthesis strings from the command line.  */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "FN_TOKEN_REMOVE_PAREN_STRINGS"

extern int FN_TOKEN_REMOVE_PAREN_STRINGS(char  *pCmdCpy,
                                         char  *pCmdStr,
                                         char **pParenPtr,
                                         int    maxParenPtrs, 
                                         int    maxParenDepth,
                                         int    compressBlankFlag)
{
    int                 cnt;
    int                 parenDepth          = 0;
    char               *pStringStart        = NULL;
    char               *pCharOut;

    for (cnt = 0; 
        *pCmdCpy != 0; 
        *pCmdCpy++)
    {
        /*************************************************************/
        /* Convert beginning an ending quotation marks to blanks.    */
        /*************************************************************/
        if (*pCmdCpy == TOKEN_BEGQUOTE_ALIAS)
        {
            *pCmdCpy = ' ';
        }
        else if (*pCmdCpy == TOKEN_ENDQUOTE_ALIAS)
        {
            *pCmdCpy = ' ';
        }

        if (*pCmdCpy == '(')
        {
            parenDepth++;

            if (parenDepth > maxParenDepth)
            {
                return RC_FAILURE;
            }
            if (parenDepth == 1)
            {
                if (cnt >= maxParenPtrs)
                {
                    return RC_FAILURE;
                }

                pStringStart = pCmdCpy + 1;
                pParenPtr[cnt] = pStringStart;      
                cnt++;
                *pCmdStr++ =  '(';
                *pCmdStr++ = TOKEN_PAREN_MARKER;
                *pCmdStr++ = ')';
            }
        }
        else if (*pCmdCpy == ')')
        {
            parenDepth--;

            if (parenDepth < 0)
            {
                return RC_FAILURE;
            }

            /*********************************************************/
            /* Null terminate the parenthesis string at depth zero.  */
            /* If necessary, compress out all of the blanks.         */
            /*********************************************************/
            if (parenDepth == 0)
            {
                *pCmdCpy = 0;

                if (compressBlankFlag)
                {
                    pCharOut = pStringStart;

                    while (*pStringStart != 0)
                    {
                        if (*pStringStart != ' ')
                        {
                            *pCharOut = *pStringStart;
                            pCharOut++;
                        }

                        pStringStart++;
                    }

                    *pCharOut = 0;
                }
            }
        }
        else if (parenDepth == 0)
        {
            *pCmdStr = *pCmdCpy;
            *pCmdStr++;
        }
    }

    if (parenDepth != 0)
    {
        return RC_FAILURE;
    }

    *pCmdStr = 0; 

    return RC_SUCCESS;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: FN_TOKEN_RETURN_NEXT                              */
/** Description:   Extract the next token from the command line.     */
/**                                                                  */
/** Parse the next token from the input command line string and      */
/** return the following:                                            */
/** (1) The next token in the input command line (the input          */
/**     parameter, pCurrCmdToken is updated).                        */
/** (2) The delimiter that began the token in the command line.      */
/** (3) The delimiter that ended the token in the command line.      */
/** (4) The number of characters to strip off the beginning of the   */
/**     command line (the input parameter pBypassChars, is updated). */
/** (5) The return code is as follows:                               */
/**     TOKEN_RETURNED = good token returned                         */
/**     TOKEN_IS_NULL = blank token; to be ignored when ')'          */
/**     delimiter is followed by a ','.                              */ 
/**     TOKEN_TRUNCATED = truncated token returned                   */
/**     TOKEN_END_OF_LINE = EOL detected.                            */
/**                                                                  */
/**  NOTE: When TOKEN_END_OF_LINE is returned, the updated (last)    */
/**  token should always be NULL!                                    */
/**                                                                  */
/**  There are 3 types of delimiters defined.                        */
/**  (1) Absolute ending delimiters.  These are ",=()" and they      */
/**      terminate the current token, even if the current token      */
/**      is a NULL string.                                           */
/**      For example: The string "FRED=,,,",                         */
/**      will return 4 tokens, "FRED" (delimited by the "="),        */
/**      then 3 NULL tokens (delimited by the ",").                  */
/**  (2) Possible ending delimiter.  The blank is a possible         */
/**      ending delimiter.  It may end a token, if no delimiter      */
/**      immediately follows.                                        */
/**      For example: the string "FRED=6, BARNEY=7"                  */
/**      is equivilent to the strings "FRED=6 BARNEY=7"              */
/**      and "FRED=6 , BARNEY=7".                                    */
/**  (3) Beginning delimiters.  These are the "(" and the blank.     */
/**      These will be stripped from the beginning of any token      */
/**      until we find a non-delimiter.                              */
/**                                                                  */
/**  This function replaces the much-too-simple strtok() function.   */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "FN_TOKEN_RETURN_NEXT"

extern int FN_TOKEN_RETURN_NEXT(char  *pCurrCmdTextPos,
                                int    tokenNum,
                                char  *pCurrCmdToken,
                                char  *pBegDelimiters,
                                char  *pEndDelimiters,
                                short *pBypassChars)
{
    int                 i                   = 0;
    int                 j                   = 0;
    int                 k                   = 0;
    int                 tokenRC             = TOKEN_RETURNED;
    char                delimiterFound      = FALSE;
    char                overflowDetected    = FALSE;
    char                blankDelimiterFound = FALSE;
    char                delimiterCharacter;
    char                markedParenthesis   = FALSE;

    char                righthandDelimiters[] = {",=()\n"};

    int                 lenCmdText          = strlen(pCurrCmdTextPos);
    int                 lenDelimiters       = strlen(righthandDelimiters);

    pBegDelimiters[tokenNum] = ' ';

    /*****************************************************************/
    /* Strip any beginning delimiters from the input command string. */
    /*****************************************************************/
    while (i < lenCmdText)
    {
        if (pCurrCmdTextPos[i] == ' ')
        {
            i++;
        }
        else if (pCurrCmdTextPos[i] == '(')
        {
            pBegDelimiters[tokenNum] = '(';
            i++;
        }
        else
        {
            break;
        }
    }

    /*****************************************************************/
    /* If nothing remains, then set end-of-line.                     */
    /*****************************************************************/
    if (i >= lenCmdText)
    {
        tokenRC = TOKEN_END_OF_LINE;
    }
    /*****************************************************************/
    /* Otherwise, start at 1st non-beginning delimiter to find,      */
    /* in order:                                                     */
    /* (1) End-of-line characters.                                   */
    /* (2) Blank delimiters.                                         */
    /* (3) Real delimiters.                                          */
    /*****************************************************************/
    else
    {
        for (; 
            i < lenCmdText; 
            i++)
        {
            if (pCurrCmdTextPos[i] == 0 ||
                pCurrCmdTextPos[i] == '\n')
            {
                delimiterCharacter = '\n';
                tokenRC = TOKEN_END_OF_LINE;

                break;
            }

            if (pCurrCmdTextPos[i] == ' ')
            {
                delimiterCharacter = ' ';
                blankDelimiterFound = TRUE;
            }
            else
            {
                for (j = 0; 
                    j < lenDelimiters; 
                    j++)
                {
                    if (pCurrCmdTextPos[i] == righthandDelimiters[j])
                    {
                        delimiterCharacter = righthandDelimiters[j];
                        delimiterFound = TRUE;

                        break;
                    }
                }

                if (delimiterFound)
                {
                    break;
                }

                /*****************************************************/
                /* If we found a non-delimiter character after a     */
                /* blank, then backup 1 character, so we don't       */
                /* lose the current "good" character the next        */
                /* go round.                                         */
                /*****************************************************/
                if (blankDelimiterFound)
                {
                    i--;

                    break;
                }

                /*****************************************************/
                /* If no overflow, move the current non-delimiter    */
                /* character in the input command string to the      */
                /* current character position in the input token.    */
                /*****************************************************/
                if (!(overflowDetected))
                {
                    pCurrCmdToken[k] = pCurrCmdTextPos[i];
                    k++;
                }

                /*****************************************************/
                /* Test if we've overflowed the current token.       */
                /*****************************************************/
                if (k >= (TOKEN_WIDTH - 1))
                {
                    if (!(overflowDetected))
                    {
                        tokenRC = TOKEN_TRUNCATED; 
                        overflowDetected = TRUE;
                    }
                }
            }
        }
    }

    /*****************************************************************/
    /* Delimit the token string.                                     */
    /*****************************************************************/
    pCurrCmdToken[k] = 0;
    pEndDelimiters[tokenNum] = delimiterCharacter;

    /*****************************************************************/
    /* Test for special blank tokens that should be ignored.         */
    /* Consider the following cases:                                 */
    /* (1) VERB KEYWORD1(VALUE1),KEYWORD2(VALUE2)                    */
    /* (2) VERB KEYWORD1=VALUE1,KEYWORD2=VALUE2                      */
    /* The 1st case would result in 6 tokens with the 4th being a    */
    /* blank token, while the 2nd case would result in 5 tokens.     */
    /* We want to detect the 1st case above and igore the 4th blank  */
    /* token.                                                        */
    /*                                                               */
    /* (1) VERB KEYWORD1(VALUE1(N))                                  */
    /* (2) VERB KEYWORD1=VALUE1(N)                                   */
    /* The 1st case would result in 5 tokens with the 5th being a    */
    /* blank token, while the 2nd case would result in 4 tokens.     */
    /* We want to detect the 1st case above and igore the 5th blank  */
    /* token.                                                        */
    /*****************************************************************/
    if (strlen(pCurrCmdToken) == 0)
    {
        TRMSGI(TRCI_ERROR,
               "Blank token (num=%i) detected, tokenRC=%i, "
               "begDelimiter(prev)=%c, endDelimiter(prev)=%c, "
               "begDelimiter(curr)=%c, endDelimiter(curr)=%c\n", 
               tokenNum, 
               tokenRC,
               pBegDelimiters[(tokenNum - 1)],
               pEndDelimiters[(tokenNum - 1)],
               pBegDelimiters[tokenNum],
               pEndDelimiters[tokenNum]);

        if (tokenRC != TOKEN_END_OF_LINE)
        {
            if (tokenNum > 1)
            {
                if ((pEndDelimiters[(tokenNum - 1)] == ')') &&
                    (pEndDelimiters[tokenNum] == ','))
                {
                    TRMSGI(TRCI_ERROR,
                           "Ignoring ')' followed by ',' blank token\n");

                    tokenRC = TOKEN_IS_NULL;
                }
                if ((pEndDelimiters[(tokenNum - 1)] == ')') &&
                    (pEndDelimiters[tokenNum] == ')'))
                {
                    TRMSGI(TRCI_ERROR,
                           "Ignoring ')' followed by ')' blank token\n");

                    tokenRC = TOKEN_IS_NULL;
                }
            }
        }
    }

    /*****************************************************************/
    /* Test for end-of-line after last token.                        */
    /* If so, then backup 1 character so we don't lose the           */
    /* end-of-line the next go round.                                */
    /*****************************************************************/
    if ((pCurrCmdTextPos[i] == 0) ||
        (pCurrCmdTextPos[i] == '\n'))
    {
        *pBypassChars = (i);
    }
    else
    {
        *pBypassChars = (i+1);
    }

    return tokenRC;
}



