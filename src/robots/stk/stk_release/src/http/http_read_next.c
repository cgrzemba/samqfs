/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http_read_next.c                                 */
/** Description:    t_http server read next logical record service.  */
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
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "api/defs_api.h"
#include "csi.h"
#include "srvcommon.h"
#include "xapi/http.h"
#include "xapi/smccxmlx.h"
#include "xapi/xapi.h"


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#define INPUT_BUFFER_LENGTH    133
#define MAX_RESPONSE_COLUMN    120  


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int buildLogicalBuffer(char  pInputBuffer[],
                              char  pOutputBuffer[],
                              char *pCommentFlag,
                              char *pContinuationFlag);

static int stripTrailingWhiteSpace(char pInputBuffer[],
                                   int  bufferLen);


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: http_read_next                                    */
/** Description:   t_http server read next logical record service.   */
/**                                                                  */
/** Read the specified input file for the next logical input line,   */
/** and update both the supplied input buffer and input length.      */
/** The return code is as follows:                                   */
/**                                                                  */
/** RC_SUCCESS indicates that a logical line has been returned.      */
/** RC_WARNING indicates End-Of-File (no logical line was returned). */
/** RC_FAILURE indicates line continuation, or comment or length     */
/**            error was detected (no logical line was returned).    */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "http_read_next"

int http_read_next(void *pFile,
                   char *pInputBuffer,
                   int   maxInputBufferLen,
                   int  *pInputBufferLen,
                   char *fileHandle)
{
    FILE               *pInputFile          = (FILE*) pFile;
    int                 lastRC;
    int                 readRC              = RC_SUCCESS;
    int                 i;

    int                 bufferLen;
    int                 inputLen;
    int                 inputBufferLen      = 0;

    char               *pBuffer;
    char                inBuffer[INPUT_BUFFER_LENGTH]; 
    char                workBuffer[INPUT_BUFFER_LENGTH];

    char                commentFlag         = FALSE;
    char                continuationFlag    = FALSE;
    char                lenErrorFlag        = FALSE;

    *pInputBufferLen = 0;

    memset(workBuffer, 
           0, 
           sizeof(workBuffer));

    /*****************************************************************/
    /* Read the specified file.  When fgets() returns NULL, it can   */
    /* mean either end-of-file or I/O error.  ferror() is called to  */
    /* determine which.                                              */
    /*****************************************************************/
    while (1)
    {
        pBuffer = fgets(inBuffer, 
                        sizeof(inBuffer), 
                        pInputFile);

        if (pBuffer == NULL)
        {
            lastRC = ferror(pInputFile);

            if (lastRC != 0)
            {
                clearerr(pInputFile);

                printf("HTTP http_gen_resp fgets() error for file=%s\n", 
                       fileHandle);

                return RC_FAILURE;
            }

            readRC = RC_WARNING;

            break;
        }

        inputLen = strlen(inBuffer);

        TRMEMI(TRCI_SERVER, inBuffer, inputLen,
               "Next physical input line:\n");

        /*************************************************************/
        /* If this is an "old style" comment card (i.e. one that     */
        /* begins with an asterisk in column 1), then ignore the     */
        /* line.                                                     */
        /*                                                           */
        /* NOTE: "New style" C-type comments that begin with "/*"    */
        /* may span multiple lines.                                  */
        /*************************************************************/
        if ((inputLen > 0) &&
            (inBuffer[0] == '*') &&
            (!(commentFlag)))
        {
            inBuffer[inputLen - 1] = 0;

            TRMSGI(TRCI_SERVER,
                   "Line is old style comment\n");

            continue;
        }

        /*************************************************************/
        /* We only process columns 1-120 of the input, in case the   */
        /* user has placed sequence numbers in columns beyond 120.   */
        /*************************************************************/
        inBuffer[MAX_RESPONSE_COLUMN] = 0;

        /*************************************************************/
        /* Strip leading blanks from the 1st physical card image     */
        /* for the next logical command.                             */
        /*************************************************************/
        if (inputBufferLen == 0)
        {
            for (i = 0; 
                i < sizeof(inBuffer);
                i++)
            {
                if (inBuffer[i] != ' ')
                {
                    break;
                }
            }

            memcpy(workBuffer, 
                   &inBuffer[i], 
                   sizeof(inBuffer) - i);
        }
        else
        {
            memcpy(workBuffer, 
                   inBuffer, 
                   sizeof(inBuffer));
        }

        bufferLen = buildLogicalBuffer(workBuffer,
                                       &pInputBuffer[inputBufferLen],
                                       &commentFlag,
                                       &continuationFlag);

        /*************************************************************/
        /* If line length error, then quit now.                      */
        /*************************************************************/
        inputBufferLen = inputBufferLen + bufferLen;

        if (inputBufferLen > maxInputBufferLen)
        {
            TRMSGI(TRCI_SERVER,
                   "inputBufferLen=%i now exceeds "
                   "maxInputBufferLen=%i\n",
                   inputBufferLen, 
                   maxInputBufferLen);

            lenErrorFlag = TRUE;
            inputBufferLen = maxInputBufferLen - 1;
            readRC = RC_FAILURE;

            break;
        }

        /*************************************************************/
        /* If no continuation line and no line length error, then    */
        /* pass the input back to the caller.                        */
        /*************************************************************/
        if (!(continuationFlag))
        {
            if (inputBufferLen > 0)
            {
                inputBufferLen = strlen(pInputBuffer);
                *pInputBufferLen = inputBufferLen;

                TRMEMI(TRCI_SERVER, pInputBuffer, inputBufferLen,
                       "Next logical input line:\n");

                return RC_SUCCESS;
            }
            /*********************************************************/
            /* If NULL logical input, then try again.                */
            /*********************************************************/
            else
            {
                inputBufferLen = 0;
                pInputBuffer[0] = 0;
            }
        }
        /*************************************************************/
        /* If we have a continuation line indication, then go get    */
        /* the next physical input card.                             */
        /*************************************************************/
    }  

    if (!lenErrorFlag)
    {
        if (commentFlag)
        {
            printf("HTTP http_gen_resp unterminated comment in file=%s\n", 
                   fileHandle);

            readRC = RC_FAILURE;
        }

        if (continuationFlag)
        {
            printf("HTTP http_gen_resp unterminated continuation in file=%s\n", 
                   fileHandle);

            readRC = RC_FAILURE;
        }
    }

    TRMSGI(TRCI_SERVER, 
           "At exit; lenErrorFlag=%i, commentFlag=%i, "
           "continuationFlag=%i, inputBufferLen=%i, "
           "readRC=%i\n",
           lenErrorFlag, 
           commentFlag, 
           continuationFlag, 
           inputBufferLen, 
           readRC);

    fclose(pInputFile);

    return readRC;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: buildLogicalBuffer                                */
/** Description:   Process the current input buffer.                 */
/**                                                                  */
/** This function:                                                   */
/** (1) removes any comments from the line,                          */
/** (2) removes any trailing continuation character ("+" or "-")     */
/** (3) removes any trailing white space                             */
/** (4) updates the output buffer (what remains of the line),        */
/** (5) updates the comment in progress flag,                        */
/** (6) updates the continuation in progress flag                    */
/** (7) returns the length of the output buffer.                     */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "buildLogicalbuffer"

static int buildLogicalBuffer(char  pInputBuffer[],
                              char  pOutputBuffer[],
                              char *pCommentFlag,
                              char *pContinuationFlag)
{
    char               *pCommentPtr;
    char               *pFirstChar;
    int                 bufferLen             = 0;
    int                 i;

    TRMEMI(TRCI_SERVER, pInputBuffer, strlen(pInputBuffer),
           "Entered; commentFlag=%i, continuationFlag=%i, card image:\n",
           *pCommentFlag, 
           *pContinuationFlag);

    /*****************************************************************/
    /* Process comments.                                             */
    /* Single line comments are identified with "*" in column 1.     */
    /* Non single-line comments may begin with "slash asterisk"      */
    /* and end with "asterisk slash".  Comments may extend across    */
    /* multiple lines.  We handle the comments by replacing          */
    /* the "slask asterisk" through terminating "asterisk slash"     */
    /* with spaces.                                                  */
    /*                                                               */
    /* Note that C returns the characters newline and null after the */
    /* after the last blank, so we check for a null to determine     */
    /* the end of the input data.                                    */
    /*****************************************************************/
    pCommentPtr = pInputBuffer;

    while (*pCommentPtr != 0)
    {
        if (*pCommentFlag == FALSE)
        {
            /*********************************************************/
            /* If we are not in the middle of a comment, and this    */
            /* command does not contain any comments, then exit      */
            /* this loop.                                            */
            /*********************************************************/
            pCommentPtr = strstr(pInputBuffer, "/*");

            if (pCommentPtr == NULL)
            {
                break;
            }
            else
            {
                *pCommentFlag = TRUE;
            }
        }

        /*************************************************************/
        /* At this point we are processing a comment.  We continue   */
        /* until end-of-comment or end-of-buffer, replacing any      */
        /* characters within the comment with blanks.                */
        /*                                                           */
        /* We do not allow ending "asterisk slash" within quotes     */
        /* within a comment.  Any "asterisk slash" is considered the */
        /* end of a comment.                                         */
        /*************************************************************/
        while (*pCommentPtr != 0)
        {
            if (memcmp(pCommentPtr, "*/", 2) == 0)
            {
                memset(pCommentPtr, 
                       ' ', 
                       2);

                *pCommentFlag = FALSE;

                break;
            }
            else
            {
                *pCommentPtr = ' ';
                pCommentPtr++;
            }
        }
    }

    /*****************************************************************/
    /* All done stripping comments, now process any trailing "+" or  */
    /* "-" character which indicates a continuation line.            */
    /*****************************************************************/
    bufferLen = strlen(pInputBuffer);

    bufferLen = stripTrailingWhiteSpace(pInputBuffer, 
                                        bufferLen);

    if (pInputBuffer[(bufferLen - 1)] == '+')
    {
        pInputBuffer[(bufferLen - 1)] = 0;
        bufferLen = bufferLen - 1;
        *pContinuationFlag = TRUE;
    }
    else if (pInputBuffer[(bufferLen - 1)] == '-')
    {
        pInputBuffer[(bufferLen - 1)] = ' ';
        pInputBuffer[bufferLen] = 0; 
        *pContinuationFlag = TRUE;
    }
    else
    {
        *pContinuationFlag = FALSE;
    }

    /*****************************************************************/
    /* Now strip leading blanks from the card image.                 */
    /*****************************************************************/
    for (i = 0;
        i < bufferLen;
        i ++)
    {
        if (pInputBuffer[i] != ' ')
        {
            pFirstChar = &(pInputBuffer[i]);

            break;
        }
    }

    bufferLen = bufferLen - i;

    /*****************************************************************/
    /* Now update everything that needs updating, and return.        */
    /*****************************************************************/
    if (bufferLen > 0)
    {
        strcpy(pOutputBuffer, pFirstChar);
    }

    return bufferLen;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: stripTrailingWhiteSpace                           */
/** Description:   Strip trailing white space characters.            */
/**                                                                  */
/** Strip any white space characters from the end of the input line, */
/** by converting them to a X'00' line-end character, and return     */
/** the new line length.                                             */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "stripTrailingWhiteSpace"

static int stripTrailingWhiteSpace(char pInputBuffer[],
                                   int  bufferLen)
{
    int                 i;

    for (i = bufferLen; 
        i > 0; 
        i--)
    {
        if (isspace(pInputBuffer[(i-1)]))
        {
            pInputBuffer[(i-1)] = 0;
        }
        else
        {
            break;
        }
    }

    return strlen(pInputBuffer);
}



