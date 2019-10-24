/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      smccxtk.c                                        */
/** Description:    XAPI client XML parser tokenizing service.       */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     05/15/11                          */
/**     Created for CDK to add XAPI support.                         */
/** I7054722       Joseph Nofi     06/28/11  CDK-XAPI 06/29/11       */
/**     Do not strip leading blanks for <uui_text> content.          */
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


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_GENERATE_HIERARCHY_FROM_XML                    */
/** Description:   Convert an XML string into an XMLPARSE hierarchy. */
/**                                                                  */
/** Parse the current XMLCNVIN data (an XML "blob" string) and       */
/** create the XMLPARSE data hierarchy tree structures and link      */
/** them to the input XMLPARSE.                                      */
/**                                                                  */
/** We parse the data in stream mode, from start to finish without   */
/** going back to an earlier position in the buffer.  This will      */
/** allow future releases to parse an XML "blob" in chunks,          */
/** instead of all at once as is done here.                          */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_GENERATE_HIERARCHY_FROM_XML"

extern int FN_GENERATE_HIERARCHY_FROM_XML(struct XMLPARSE *pXmlparse)
{
    int                 iteration           = 0;
    int                 leafRC;
    int                 xmlStringValue;

    /*****************************************************************/
    /* Within this main loop, we should only find types of start and */
    /* end tags.                                                     */
    /*****************************************************************/
    while (pXmlparse->endOfXml == FALSE)
    {
        iteration++;

        if (iteration > pXmlparse->charCount)
        {
            TRMSGI(TRCI_ERROR,
                   "charCount/iteration error; leaving parse loop\n");

            XML_BYPASS_AND_MARK_ERROR(ERR_CHAR_COUNT_LOGIC, 
                                      pXmlparse, 
                                      pXmlparse->lastParseLen);

            break;
        }

        xmlStringValue = FN_PARSE_NEXT_STRING(pXmlparse);

        TRMEMI(TRCI_XMLPARSER,
               pXmlparse->pBufferLast, 16,
               "Start and end tag parse loop iteration=%i; "
               "xmlStringValue=%i, errorCode=%i, "
               "charParsed=%i, charRemaining=%i, "
               "bufferLast=%08X, bufferPos=%08X, pBufferLast:\n",
               iteration, 
               xmlStringValue, 
               pXmlparse->errorCode, 
               pXmlparse->charParsed, 
               pXmlparse->charRemaining, 
               pXmlparse->pBufferLast, 
               pXmlparse->pBufferPos);

        if (xmlStringValue == XML_NOTE_STARTTAG)
        {
            TRMSGI(TRCI_XMLPARSER,
                   "Found notation starttag\n");

            /* Process notation starttag                             */
        }
        else if (xmlStringValue == XML_ENTITY_STARTTAG)
        {
            TRMSGI(TRCI_XMLPARSER,
                   "Found entity starttag\n");

            /* Process entity starttag                               */
        }
        else if (xmlStringValue == XML_CDATA_ENDTAG)
        {
            TRMSGI(TRCI_XMLPARSER,
                   "Found cdata endtag\n");

            /* Process cdata endtag                                  */
        }
        else if (xmlStringValue == XML_ELEMEND_STARTTAG)
        {
            TRMSGI(TRCI_XMLPARSER,
                   "Found elemend starttag\n");

            /* Process elemend starttag                              */
        }
        else if (xmlStringValue == XML_EMPTYELEM_ENDTAG)
        {
            TRMSGI(TRCI_XMLPARSER,
                   "Found emptyelem endtag\n");

            /* Process emptyelem endtag                              */
        }
        else if (xmlStringValue == XML_COMMENT_STARTTAG)
        {
            leafRC = FN_CREATE_XMLCOMM(pXmlparse);
        }
        else if (xmlStringValue == XML_DECL_STARTTAG)
        {
            leafRC = FN_CREATE_XMLDECL(pXmlparse);
        }
        else if (xmlStringValue == XML_DECL_ENDTAG)
        {
            TRMSGI(TRCI_XMLPARSER,
                   "Found decl endtag\n");

            /* Process decl endtag                                   */
        }
        else if (xmlStringValue == XML_STARTTAG)
        {
            leafRC = FN_CREATE_XMLELEM(pXmlparse);
        }
        else if (xmlStringValue == XML_ENDTAG)
        {
            TRMSGI(TRCI_XMLPARSER,
                   "Found endtag\n");

            /* Process endtag                                        */
        }
        else
        {
            pXmlparse->reasonCode = xmlStringValue;

            XML_BYPASS_AND_MARK_ERROR(ERR_NAKED_XML_STRING, 
                                      pXmlparse, 
                                      pXmlparse->lastParseLen);

            return(pXmlparse->errorCode);
        }
    }

    return pXmlparse->errorCode;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_PARSE_DECL                                     */
/** Description:   Parse the current declaration data.               */
/**                                                                  */
/** As far as this parser is concerned, a declaration is just        */
/** free-form text between a decl start tag ("<?") and any type      */
/** of end tag (">"). We will allow quotes, equal signs, etc.        */
/** However, we will not allow other start tags until an end tag     */
/** is reached (i.e. NO nested declarations, comments, etc.)         */
/**                                                                  */
/** Valid examples:                                                  */
/** <?XML VERSION=1.0?>                                              */
/** <?XML VERSION=1.0>                                               */
/** <?THIS IS CONSIDERED JUST "TEXT">                                */
/** Invalid examples:                                                */
/** <?XML VERSION=1.0 <!DO NOT ALLOW COMMENTS HERE>>                 */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_PARSE_DECL"

extern int FN_PARSE_DECL(struct XMLPARSE *pXmlparse)
{
    struct XMLDECL     *pXmldecl            = pXmlparse->pCurrXmldecl;
    int                 iteration           = 0;
    int                 xmlStringValue;
    int                 lastRC;
    char                foundDeclEnd        = FALSE;

    TRMSGI(TRCI_XMLPARSER,
           "Entered\n");

    /*****************************************************************/
    /* Find the real start of the declaration literal.               */
    /*****************************************************************/
    lastRC = FN_PARSE_NEXT_NONBLANK(pXmlparse);

    if (lastRC != RC_SUCCESS)
    {
        XML_BYPASS_AND_MARK_ERROR(ERR_ALL_BLANK, 
                                  pXmlparse, 
                                  0);

        return(pXmlparse->errorCode);
    }

    /*****************************************************************/
    /* Within this main loop, we should only find certain end tags.  */
    /* (i.e. we do not allow nested declarations or comments within  */
    /* declarations, etc.).                                          */
    /*****************************************************************/
    while (pXmlparse->endOfXml == FALSE)
    {
        iteration++;

        if (iteration > pXmlparse->charCount)
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_NO_DECL_END, 
                                      pXmlparse, 
                                      0);

            return(pXmlparse->errorCode);
        }

        xmlStringValue = FN_PARSE_NEXT_STRING(pXmlparse);

        TRMEMI(TRCI_XMLPARSER,
               pXmlparse->pBufferLast, 16,
               "Specific end tag parse loop iteration=%i; "
               "xmlStringValue=%i, errorCode=%i, "
               "charParsed=%i, charRemaining=%i, "
               "bufferLast=%08X, bufferPos=%08X, pBufferLast:\n",
               iteration, 
               xmlStringValue, 
               pXmlparse->errorCode, 
               pXmlparse->charParsed, 
               pXmlparse->charRemaining, 
               pXmlparse->pBufferLast, 
               pXmlparse->pBufferPos);

        if ((xmlStringValue == XML_NOTE_STARTTAG) ||
            (xmlStringValue == XML_ENTITY_STARTTAG) ||
            (xmlStringValue == XML_ELEMEND_STARTTAG) ||
            (xmlStringValue == XML_COMMENT_STARTTAG) ||
            (xmlStringValue == XML_DECL_STARTTAG) ||
            (xmlStringValue == XML_STARTTAG))
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_STARTTAG_INSIDE_DECL, 
                                      pXmlparse, 
                                      pXmlparse->lastParseLen);

            return(pXmlparse->errorCode);
        }

        /*************************************************************/
        /* Lets allow any type of end tag for a declaration.         */
        /*************************************************************/
        else if ((xmlStringValue == XML_CDATA_ENDTAG) ||
                 (xmlStringValue == XML_EMPTYELEM_ENDTAG) ||
                 (xmlStringValue == XML_DECL_ENDTAG) ||
                 (xmlStringValue == XML_ENDTAG))
        {
            foundDeclEnd = TRUE;

            break;
        }
        /*************************************************************/
        /* Anything else is considered declaration text.  Record the */
        /* start of the declaration.                                 */
        /*************************************************************/
        else
        {
            FN_CONVERT_ASCII_NON_DISPLAY(pXmlparse->pBufferLast,
                                         1);

            if (pXmldecl->pDeclaration == NULL)
            {
                pXmldecl->pDeclaration = pXmlparse->pBufferLast; 
            }
        }
    }

    /*****************************************************************/
    /* If we did not find the end of the declaration, flag the error.*/
    /*****************************************************************/
    if (!(foundDeclEnd))
    {
        if (pXmlparse->errorCode != RC_SUCCESS)
        {
            return(pXmlparse->errorCode);
        }

        XML_BYPASS_AND_MARK_ERROR(ERR_NO_DECL_END, 
                                  pXmlparse, 
                                  0);

        return(pXmlparse->errorCode); 
    }

    TRMSGI(TRCI_XMLPARSER,
           "Have a well formed XMLDECL; "
           "startTagCount=%i, endtagCount=%i\n",
           pXmlparse->starttagCount, 
           pXmlparse->endtagCount);

    pXmldecl->flag |= DECL_END;
    pXmldecl->declLen = pXmlparse->pBufferLast - pXmldecl->pDeclaration;

    return RC_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_PARSE_COMM                                     */
/** Description:   Parse the current comment data.                   */
/**                                                                  */
/** As far as this parser is concerned, a comment is just            */
/** free-form text between a comment start tag ("<!") and any type   */
/** of end tag (">"). We will allow quotes, equal signs, etc.        */
/** However, we will not allow other start tags until an end tag     */
/** is reached (i.e. NO nested declarations, comments, etc.)         */
/**                                                                  */
/** Valid examples:                                                  */
/** <!This is a comment>                                             */
/** <!'This is just plain "TEXT"'>                                   */
/** Invalid examples:                                                */
/** <!One comment<!Do NOT allow another>>                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_PARSE_COMM"

extern int FN_PARSE_COMM(struct XMLPARSE *pXmlparse)
{
    struct XMLCOMM     *pXmlcomm            = pXmlparse->pCurrXmlcomm;
    int                 iteration           = 0;
    int                 xmlStringValue;
    int                 lastRC;
    char                foundCommEnd        = FALSE;

    TRMSGI(TRCI_XMLPARSER,
           "Entered\n");

    /*****************************************************************/
    /* Find the real start of the comment literal.                   */
    /*****************************************************************/
    lastRC = FN_PARSE_NEXT_NONBLANK(pXmlparse);

    if (lastRC != RC_SUCCESS)
    {
        XML_BYPASS_AND_MARK_ERROR(ERR_ALL_BLANK, 
                                  pXmlparse, 
                                  0);

        return(pXmlparse->errorCode);
    }

    /*****************************************************************/
    /* Within this main loop, we should only find certain end tags.  */
    /* (i.e. we do not allow nested comments or declarations within  */
    /* comments, etc.).                                              */
    /*****************************************************************/
    while (pXmlparse->endOfXml == FALSE)
    {
        iteration++;

        if (iteration > pXmlparse->charCount)
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_NO_COMM_END, 
                                      pXmlparse, 
                                      0);

            return(pXmlparse->errorCode);
        }

        xmlStringValue = FN_PARSE_NEXT_STRING(pXmlparse);

        TRMEMI(TRCI_XMLPARSER,
               pXmlparse->pBufferLast, 16,
               "Specific end tag iteration=%i; xmlStringValue=%i, "
               "errorCode=%i, charParsed=%i, "
               "charRemaining=%i, bufferLast=%08X, "
               "bufferPos=%08X, pBufferLast:\n",
               iteration, 
               xmlStringValue, 
               pXmlparse->errorCode, 
               pXmlparse->charParsed, 
               pXmlparse->charRemaining, 
               pXmlparse->pBufferLast, 
               pXmlparse->pBufferPos);

        if ((xmlStringValue == XML_NOTE_STARTTAG) ||
            (xmlStringValue == XML_ENTITY_STARTTAG) ||
            (xmlStringValue == XML_ELEMEND_STARTTAG) ||
            (xmlStringValue == XML_COMMENT_STARTTAG) ||
            (xmlStringValue == XML_DECL_STARTTAG) ||
            (xmlStringValue == XML_STARTTAG))
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_STARTTAG_INSIDE_COMM, 
                                      pXmlparse, 
                                      pXmlparse->lastParseLen);

            return(pXmlparse->errorCode);
        }

        /*************************************************************/
        /* Lets allow any type of end tag for a comment.             */
        /*************************************************************/
        else if ((xmlStringValue == XML_CDATA_ENDTAG) ||
                 (xmlStringValue == XML_EMPTYELEM_ENDTAG) ||
                 (xmlStringValue == XML_DECL_ENDTAG) ||
                 (xmlStringValue == XML_ENDTAG))
        {
            foundCommEnd = TRUE;

            break;
        }
        /*************************************************************/
        /* Anything else is considered comment text.  Record the     */
        /* start of the comment.                                     */
        /*************************************************************/
        else
        {
            FN_CONVERT_ASCII_NON_DISPLAY(pXmlparse->pBufferLast,
                                         1);

            if (pXmlcomm->pComment == NULL)
            {
                pXmlcomm->pComment = pXmlparse->pBufferLast; 
            }
        }
    }

    /*****************************************************************/
    /* If we did not find the end of the comment, flag the error.    */
    /*****************************************************************/
    if (!(foundCommEnd))
    {
        if (pXmlparse->errorCode != RC_SUCCESS)
        {
            return(pXmlparse->errorCode);
        }

        XML_BYPASS_AND_MARK_ERROR(ERR_NO_COMM_END, 
                                  pXmlparse, 
                                  0);

        return(pXmlparse->errorCode); 
    }

    TRMSGI(TRCI_XMLPARSER,
           "Have a well formed XMLCOMM; "
           "startTagCount=%i, endtagCount=%i\n",
           pXmlparse->starttagCount, 
           pXmlparse->endtagCount);

    pXmlcomm->flag |= COMM_END;
    pXmlcomm->commLen = pXmlparse->pBufferLast - pXmlcomm->pComment; 
    return RC_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_PARSE_ELEM                                     */
/** Description:   Parse the current element data.                   */
/**                                                                  */
/** Elements have a start structure, content, and end structure.     */
/** Any text between the start structure and end structure is        */ 
/** the element content. The start structure and end structure       */
/** are similar, each has a start tag and end tag with the           */
/** text between them being the element name.  We do allow           */
/** nesting of elements as long as they are properly nested.         */
/**                                                                  */
/** Valid examples:                                                  */
/** <ELEMENT_NAME>element content</ELEMENT_NAME>                     */
/** <NAME1>content1<NAME2>content2</NAME2></NAME1>                   */
/** Invalid examples:                                                */
/** <NAME1>content1<NAME2>content2</NAME1></NAME2>                   */
/**                                                                  */
/** Element content may also contain comments delindated by the      */
/** XML start comment tag as in the following examples:              */
/**                                                                  */
/** Valid examples:                                                  */
/** <NAME1>content1<!training comment></NAME1>                       */
/** <NAME2><!leading comment>content2</NAME2>                        */
/**                                                                  */
/** Element start structures may contain attribute-value pair(s),    */
/** after the element name, delineated by blanks as in the           */
/** following examples:                                              */
/**                                                                  */
/** Valid examples:                                                  */
/** <NAME1 attr1=value1>content1<NAME1>                              */
/** <NAME2 attr2=value2 attr3=value3>content1<NAME2>                 */
/** Invalid examples:                                                */
/** <attr4=value4 NAME4>content4<NAME4>                              */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_PARSE_ELEM"

extern int FN_PARSE_ELEM(struct XMLPARSE *pXmlparse)
{
    struct XMLELEM     *pXmlelem            = pXmlparse->pCurrXmlelem;
    struct XMLELEM     *pParentXmlelem;
    int                 iteration           = 0;
    int                 xmlStringValue;
    int                 lastRC;
    int                 leafRC;
    char                foundStartNameEnd   = FALSE;
    char                foundEndNameEnd     = FALSE;
    char                foundContentEnd     = FALSE;
    char                foundElemendStarttag= FALSE;  

    TRMSGI(TRCI_XMLPARSER,
           "Entered\n");

    /*****************************************************************/
    /* Find the real start of the element.                           */
    /*****************************************************************/
    lastRC = FN_PARSE_NEXT_NONBLANK(pXmlparse);

    if (lastRC != RC_SUCCESS)
    {
        XML_BYPASS_AND_MARK_ERROR(ERR_ALL_BLANK, 
                                  pXmlparse, 
                                  0);

        return(pXmlparse->errorCode);
    }

    /*****************************************************************/
    /* Loop to extract start element name:                           */
    /* Within this first loop, we should only find plain text and    */
    /* end tags.                                                     */
    /* If we find a blank here, then we will create an attribute.    */
    /*****************************************************************/
    while (pXmlparse->endOfXml == FALSE)
    {
        iteration++;

        if (iteration > pXmlparse->charCount)
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_NO_ELEM_END_NAME_END, 
                                      pXmlparse, 
                                      0);

            return(pXmlparse->errorCode);
        }

        xmlStringValue = FN_PARSE_NEXT_STRING(pXmlparse);

        TRMEMI(TRCI_XMLPARSER,
               pXmlparse->pBufferLast, 16,
               "Element name parse loop iteration=%i; "
               "xmlStringValue=%i, errorCode=%i, "
               "charParsed=%i, charRemaining=%i, "
               "bufferLast=%08X, bufferPos=%08X, pBufferLast:\n",
               iteration, 
               xmlStringValue, 
               pXmlparse->errorCode, 
               pXmlparse->charParsed, 
               pXmlparse->charRemaining, 
               pXmlparse->pBufferLast, 
               pXmlparse->pBufferPos);

        if ((xmlStringValue == XML_NOTE_STARTTAG) ||
            (xmlStringValue == XML_ENTITY_STARTTAG) ||
            (xmlStringValue == XML_ELEMEND_STARTTAG) ||
            (xmlStringValue == XML_COMMENT_STARTTAG) ||
            (xmlStringValue == XML_DECL_STARTTAG) ||
            (xmlStringValue == XML_STARTTAG))
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_STARTTAG_INSIDE_START_NAME, 
                                      pXmlparse, 
                                      pXmlparse->lastParseLen);

            return(pXmlparse->errorCode);
        }

        /*************************************************************/
        /* Lets allow any type of end tag for the starting element   */
        /* name.                                                     */
        /*************************************************************/
        else if ((xmlStringValue == XML_CDATA_ENDTAG) ||
                 (xmlStringValue == XML_EMPTYELEM_ENDTAG) ||
                 (xmlStringValue == XML_DECL_ENDTAG) ||
                 (xmlStringValue == XML_ENDTAG))
        {
            foundStartNameEnd = TRUE;

            break;
        }
        /*************************************************************/
        /* Before creating a nested XMLATTR, see if we need to       */
        /* record the end of the current start name.                 */
        /*************************************************************/
        else if ((xmlStringValue == XML_BLANK) ||
                 (xmlStringValue == XML_COMMA))
        {
            if (pXmlelem->pStartElemName != NULL)
            {
                if (pXmlelem->startElemLen == 0)
                {
                    TRMSGI(TRCI_XMLPARSER,
                           "Finished start element name (1)\n");

                    pXmlelem->flag |= START_NAME_END;
                    pXmlelem->startElemLen = pXmlparse->pBufferLast 
                                             - pXmlelem->pStartElemName; 
                }
            }

            TRMSGI(TRCI_XMLPARSER,
                   "Found attribute within element\n");

            leafRC = FN_CREATE_XMLATTR(pXmlparse);
        }
        /*************************************************************/
        /* Anything else is considered start element name text.      */
        /* Record the beginning of the start element name.           */
        /*************************************************************/
        else
        {
            FN_CONVERT_ASCII_NON_DISPLAY(pXmlparse->pBufferLast,
                                         1);

            if (pXmlelem->pStartElemName == NULL)
            {
                pXmlelem->pStartElemName = pXmlparse->pBufferLast; 
            }
        }
    }

    /*****************************************************************/
    /* If we did not find the end of the start element name, flag    */
    /* the error.                                                    */
    /*****************************************************************/
    if (!(foundStartNameEnd))
    {
        if (pXmlparse->errorCode != RC_SUCCESS)
        {
            return(pXmlparse->errorCode);
        }

        XML_BYPASS_AND_MARK_ERROR(ERR_NO_ELEM_START_NAME_END, 
                                  pXmlparse, 
                                  0);

        return(pXmlparse->errorCode); 
    }
    else
    {
        if (pXmlelem->pStartElemName != NULL)
        {
            if (pXmlelem->startElemLen == 0)
            {
                TRMSGI(TRCI_XMLPARSER,
                       "Finished start element name (2)\n");

                pXmlelem->flag |= START_NAME_END;
                pXmlelem->startElemLen = pXmlparse->pBufferLast 
                                         - pXmlelem->pStartElemName; 
            }
        }
    }

    /*****************************************************************/
    /* Loop to extract element content:                              */
    /* Within this second loop, we may find start tags.  At this     */
    /* point, end tags are errors.                                   */
    /*                                                               */
    /* NOTE: If element name is not <uui_text> then logically strip  */
    /* leading blanks from the element content.                      */
    /*****************************************************************/
    if ((pXmlelem->startElemLen == SIZEOF_STR_UUI_TEXT) &&
        (memcmp(pXmlelem->pStartElemName,
                STR_UUI_TEXT,
                SIZEOF_STR_UUI_TEXT) == 0))
    {
        TRMSGI(TRCI_XMLPARSER,
               "Bypassing strip leading blanks for <uui_text>\n");
    }
    else
    {
        lastRC = FN_PARSE_NEXT_NONBLANK(pXmlparse);

        if (lastRC != RC_SUCCESS)
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_ALL_BLANK, 
                                      pXmlparse, 
                                      0);

            return(pXmlparse->errorCode);
        }
    }

    iteration = 0;

    while (pXmlparse->endOfXml == FALSE)
    {
        iteration++;

        if (iteration > pXmlparse->charCount)
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_NO_ELEM_CONTENT_END, 
                                      pXmlparse, 
                                      0);

            return(pXmlparse->errorCode);
        }

        xmlStringValue = FN_PARSE_NEXT_STRING(pXmlparse);

        TRMEMI(TRCI_XMLPARSER,
               pXmlparse->pBufferLast, 16,
               "Element content parse loop iteration=%i; "
               "xmlStringValue=%i, errorCode=%i, "
               "charParsed=%i, charRemaining=%i, "
               "bufferLast=%08X, bufferPos=%08X, pBufferLast:\n",
               iteration, 
               xmlStringValue, 
               pXmlparse->errorCode, 
               pXmlparse->charParsed, 
               pXmlparse->charRemaining, 
               pXmlparse->pBufferLast, 
               pXmlparse->pBufferPos);

        if ((xmlStringValue == XML_NOTE_STARTTAG) ||
            (xmlStringValue == XML_ENTITY_STARTTAG))
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_UNSUPPORTED_FUNC, 
                                      pXmlparse, 
                                      pXmlparse->lastParseLen);

            return(pXmlparse->errorCode);
        }
        /*************************************************************/
        /* Before creating a nested XMLCOMM, see if we need to       */
        /* record the end of the current content.                    */
        /*************************************************************/
        else if (xmlStringValue == XML_COMMENT_STARTTAG)
        {
            if (pXmlelem->flag & CONTENT_START)
            {
                if (pXmlelem->contentLen == 0)
                {
                    TRMSGI(TRCI_XMLPARSER,
                           "Finished element content (1)\n");

                    pXmlelem->flag |= CONTENT_END;
                    pXmlelem->contentLen = pXmlparse->pBufferLast 
                                           - pXmlelem->pContent; 

                    FN_XML_RESTORE_SPECIAL_CHARS(pXmlelem->pContent,
                                                 &pXmlelem->contentLen);
                }
            }

            TRMSGI(TRCI_XMLPARSER,
                   "Found comment within element\n");

            leafRC = FN_CREATE_XMLCOMM(pXmlparse);
        }
        else if (xmlStringValue == XML_DECL_STARTTAG)
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_UNEXPECTED_DECL, 
                                      pXmlparse, 
                                      pXmlparse->lastParseLen);

            return(pXmlparse->errorCode);
        }
        /*************************************************************/
        /* Before creating a nested XMLELEM, see if we need to       */
        /* record the end of the current content.                    */
        /*************************************************************/
        else if (xmlStringValue == XML_STARTTAG)
        {
            /*********************************************************/
            /* If we are processing <uui_text> or <reason> content,  */
            /* and we encounter a "<" string that is not a "</"      */
            /* string, then consider the string to be plain text.    */
            /*                                                       */
            /* NOTE: This is not kosher for a stateless XML parser,  */
            /* but we coverup possible problems with XML responses   */
            /* from HSC and VTCS that do not follow the rules.       */
            /*********************************************************/
            if (((pXmlelem->startElemLen == SIZEOF_STR_UUI_TEXT) &&
                 (memcmp(pXmlelem->pStartElemName,
                         STR_UUI_TEXT,
                         SIZEOF_STR_UUI_TEXT) == 0)) ||
                ((pXmlelem->startElemLen == SIZEOF_STR_REASON) &&
                 (memcmp(pXmlelem->pStartElemName,
                         STR_REASON,
                         SIZEOF_STR_REASON) == 0)))
            {
                TRMSGI(TRCI_XMLPARSER,
                       "Allowing \"<\" within <uui_text>\n");

                if (pXmlelem->pContent == NULL)
                {
                    pXmlelem->flag |= CONTENT_START;
                    pXmlelem->pContent = pXmlparse->pBufferLast; 
                }

                continue;
            }

            if (pXmlelem->flag & CONTENT_START)
            {
                if (pXmlelem->contentLen == 0)
                {
                    TRMSGI(TRCI_XMLPARSER,
                           "Finished element content (2)\n");

                    pXmlelem->flag |= CONTENT_END;
                    pXmlelem->contentLen = pXmlparse->pBufferLast 
                                           - pXmlelem->pContent; 

                    FN_XML_RESTORE_SPECIAL_CHARS(pXmlelem->pContent,
                                                 &pXmlelem->contentLen);
                }
            }

            TRMSGI(TRCI_XMLPARSER,
                   "Found element within element\n");

            leafRC = FN_CREATE_XMLELEM(pXmlparse);
        }
        else if (xmlStringValue == XML_ELEMEND_STARTTAG)
        {
            foundContentEnd = TRUE;
            foundElemendStarttag = TRUE;

            break;
        }
        else if (xmlStringValue == XML_ENDTAG)
        {
            /*********************************************************/
            /* If we are processing <uui_text> or <reason> content,  */
            /* and we encounter a ">" string, then consider the      */
            /* the string to be plain text.                          */
            /*                                                       */
            /* NOTE: This is not kosher for a stateless XML parser,  */
            /* but we coverup possible problems with XML responses   */
            /* from HSC and VTCS that do not follow the rules.       */
            /*********************************************************/
            if (((pXmlelem->startElemLen == SIZEOF_STR_UUI_TEXT) &&
                 (memcmp(pXmlelem->pStartElemName,
                         STR_UUI_TEXT,
                         SIZEOF_STR_UUI_TEXT) == 0)) ||
                ((pXmlelem->startElemLen == SIZEOF_STR_REASON) &&
                 (memcmp(pXmlelem->pStartElemName,
                         STR_REASON,
                         SIZEOF_STR_REASON) == 0)))
            {
                TRMSGI(TRCI_XMLPARSER,
                       "Allowing \">\" within <uui_text>\n");

                if (pXmlelem->pContent == NULL)
                {
                    pXmlelem->flag |= CONTENT_START;
                    pXmlelem->pContent = pXmlparse->pBufferLast; 
                }

                continue;
            }
            /*********************************************************/
            /* Otherwise the ">" within element content is an error. */
            /*********************************************************/
            else
            {
                XML_BYPASS_AND_MARK_ERROR(ERR_UNEXPECTED_ENDTAG, 
                                          pXmlparse, 
                                          pXmlparse->lastParseLen);

                return(pXmlparse->errorCode);
            }
        }
        else if ((xmlStringValue == XML_CDATA_ENDTAG) ||
                 (xmlStringValue == XML_EMPTYELEM_ENDTAG) ||
                 (xmlStringValue == XML_DECL_ENDTAG))
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_UNEXPECTED_ENDTAG, 
                                      pXmlparse, 
                                      pXmlparse->lastParseLen);

            return(pXmlparse->errorCode);
        }

        /*************************************************************/
        /* Anything else is considered element content text. Record  */
        /* start of the element content.                             */
        /*************************************************************/
        else
        {
            FN_CONVERT_ASCII_NON_DISPLAY(pXmlparse->pBufferLast,
                                         1);

            if (pXmlelem->pContent == NULL)
            {
                pXmlelem->flag |= CONTENT_START;
                pXmlelem->pContent = pXmlparse->pBufferLast; 
            }
        }
    }

    /*****************************************************************/
    /* If we did not find the end of the element content, see if we  */
    /* had a beginning of the content.                               */
    /*****************************************************************/
    if (pXmlelem->flag & CONTENT_START)
    {
        if (!(foundContentEnd))
        {
            if (pXmlparse->errorCode != RC_SUCCESS)
            {
                return(pXmlparse->errorCode);
            }

            XML_BYPASS_AND_MARK_ERROR(ERR_NO_ELEM_CONTENT_END, 
                                      pXmlparse, 
                                      0);

            return(pXmlparse->errorCode); 
        }
        else
        {
            if (pXmlelem->contentLen == 0)
            {
                TRMSGI(TRCI_XMLPARSER,
                       "Finished element content (3)\n");

                pXmlelem->flag |= CONTENT_END;
                pXmlelem->contentLen = pXmlparse->pBufferLast 
                                       - pXmlelem->pContent; 

                FN_XML_RESTORE_SPECIAL_CHARS(pXmlelem->pContent,
                                             &pXmlelem->contentLen);
            }
        }
    }

    /*****************************************************************/
    /* Loop to extract element end name:                             */
    /* Within this third loop, we should just find end tags and      */
    /* plain text.  The text should be the same as the start element */
    /* name.                                                         */
    /*****************************************************************/
    if (!(foundElemendStarttag))
    {
        XML_BYPASS_AND_MARK_ERROR(ERR_MISMATCHED_TAGS, 
                                  pXmlparse, 
                                  0);

        return(pXmlparse->errorCode); 
    }

    pXmlelem->flag |= END_NAME_START;
    iteration = 0;

    while (pXmlparse->endOfXml == FALSE)
    {
        iteration++;

        if (iteration > pXmlparse->charCount)
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_NO_ELEM_END_NAME_END, 
                                      pXmlparse, 
                                      0);

            return(pXmlparse->errorCode);
        }

        xmlStringValue = FN_PARSE_NEXT_STRING(pXmlparse);

        TRMEMI(TRCI_XMLPARSER,
               pXmlparse->pBufferLast, 16,
               "Element end name parse loop iteration=%i; "
               "xmlStringValue=%i, errorCode=%i, "
               "charParsed=%i, charRemaining=%i, "
               "bufferLast=%08X, bufferPos=%08X, pBufferLast:\n",
               iteration, 
               xmlStringValue, 
               pXmlparse->errorCode, 
               pXmlparse->charParsed, 
               pXmlparse->charRemaining, 
               pXmlparse->pBufferLast, 
               pXmlparse->pBufferPos);

        if ((xmlStringValue == XML_NOTE_STARTTAG) ||
            (xmlStringValue == XML_ENTITY_STARTTAG) ||
            (xmlStringValue == XML_ELEMEND_STARTTAG) ||
            (xmlStringValue == XML_COMMENT_STARTTAG) ||
            (xmlStringValue == XML_DECL_STARTTAG) ||
            (xmlStringValue == XML_STARTTAG))
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_STARTTAG_INSIDE_END_NAME, 
                                      pXmlparse, 
                                      pXmlparse->lastParseLen);

            return(pXmlparse->errorCode);
        }

        /*************************************************************/
        /* Lets allow any type of end tag for the ending element     */
        /* name.                                                     */
        /*************************************************************/
        else if ((xmlStringValue == XML_CDATA_ENDTAG) ||
                 (xmlStringValue == XML_EMPTYELEM_ENDTAG) ||
                 (xmlStringValue == XML_DECL_ENDTAG) ||
                 (xmlStringValue == XML_ENDTAG))
        {
            foundEndNameEnd = TRUE;

            break;
        }
        /*************************************************************/
        /* Anything else is considered element name text. Record the */
        /* start of the element name.                                */
        /*************************************************************/
        else
        {
            FN_CONVERT_ASCII_NON_DISPLAY(pXmlparse->pBufferLast,
                                         1);

            if (pXmlelem->pEndElemName == NULL)
            {
                pXmlelem->pEndElemName = pXmlparse->pBufferLast; 
            }
        }
    }

    /*****************************************************************/
    /* If we did not find the end tag of the end element name, flag  */
    /* the error.                                                    */
    /*****************************************************************/
    if (!(foundEndNameEnd))
    {
        if (pXmlparse->errorCode != RC_SUCCESS)
        {
            return(pXmlparse->errorCode);
        }

        XML_BYPASS_AND_MARK_ERROR(ERR_NO_ELEM_END_NAME_END, 
                                  pXmlparse, 
                                  0);

        return(pXmlparse->errorCode); 
    }
    else
    {
        if (pXmlelem->pEndElemName != NULL)
        {
            TRMSGI(TRCI_XMLPARSER,
                   "Finished end element name\n");

            pXmlelem->flag |= END_NAME_END;
            pXmlelem->endElemLen = pXmlparse->pBufferLast 
                                   - pXmlelem->pEndElemName; 
        }
    }

    /*****************************************************************/
    /* Now compare the start and end element names.                  */
    /* If they are the same, then we have a well formed XML element. */
    /* If we have a well formed XMLELEM, pop both the current level  */
    /* and the current XMLELEM pointer.                              */
    /*****************************************************************/
    if (memcmp(pXmlelem->pStartElemName,
               pXmlelem->pEndElemName,
               pXmlelem->startElemLen) != 0)
    {
        XML_BYPASS_AND_MARK_ERROR(ERR_NO_DECL_END, 
                                  pXmlparse, 
                                  0);

        return(pXmlparse->errorCode); 
    }

    TRMSGI(TRCI_XMLPARSER,
           "Have a well formed XMLELEM; "
           "startTagCount=%i, endtagCount=%i\n",
           pXmlparse->starttagCount, 
           pXmlparse->endtagCount);

    pXmlelem->flag |= ELEM_END;
    pParentXmlelem = pXmlelem->pParentXmlelem;

    if (pParentXmlelem != NULL)
    {
        if (pXmlelem != pXmlparse->pCurrXmlelem)
        {
            TRMSGI(TRCI_XMLPARSER,
                   "pXmlelem=%08X != pXmlparse->pCurrXmlelem=%08X\n",
                   pXmlelem,
                   pXmlparse->pCurrXmlelem);
        }

        TRMSGI(TRCI_XMLPARSER,
               "Resetting current XMLELEM "
               "seq=%i, level=%i at %08X "
               "from seq=%i level=%i at %08X\n",
               pParentXmlelem->sequence,
               pParentXmlelem->level,
               pParentXmlelem,
               pXmlelem->sequence,
               pXmlelem->level,
               pXmlelem);

        pXmlparse->pCurrXmlelem = pParentXmlelem;
        pXmlparse->currLevel = pParentXmlelem->level;
    }
    else
    {
        TRMEMI(TRCI_XMLPARSER,
               pXmlelem, sizeof(struct XMLELEM),
               "XMLELEM has no parent; "
               "level=%i currLevel=%i; XMLELEM:\n",
               pXmlelem->level,
               pXmlparse->currLevel);
    }

    return RC_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_PARSE_ATTR                                     */
/** Description:   Parse the current attribute name and value.       */
/**                                                                  */
/** Attributtes have a name="value" structure.                       */
/** Any text between the start of the name and the equal sign is     */
/** considered the attribute name. Any text between the double       */
/** or single quotes is considered the attribute value.              */
/**                                                                  */
/** Attributes will be found contained in the start name structure,  */
/** after the element name, delineated by a blank.                   */
/**                                                                  */
/** Valid examples:                                                  */
/** <NAME1 attr1=value1>content1<NAME1>                              */
/** <NAME2 attr2=value2 attr3=value3>content1<NAME2>                 */
/** Invalid examples:                                                */
/** <attr4=value4 NAME4>content4<NAME4>                              */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_PARSE_ATTR"

extern int FN_PARSE_ATTR(struct XMLPARSE *pXmlparse)
{
    struct XMLELEM     *pXmlelem            = pXmlparse->pCurrXmlelem;
    struct XMLATTR     *pXmlattr            = pXmlelem->pCurrXmlattr;
    int                 iteration           = 0;
    int                 xmlStringValue;
    int                 xmlStartStringValue;
    int                 lastRC;
    char                foundKeywordEnd        = FALSE;
    char                foundValueStart     = FALSE;
    char                foundValueEnd       = FALSE;

    TRMSGI(TRCI_XMLPARSER,
           "Entered\n");

    /*****************************************************************/
    /* Find the real start of the attribute.                         */
    /*****************************************************************/
    lastRC = FN_PARSE_NEXT_NONBLANK(pXmlparse);

    if (lastRC != RC_SUCCESS)
    {
        XML_BYPASS_AND_MARK_ERROR(ERR_ALL_BLANK, 
                                  pXmlparse, 
                                  0);

        return(pXmlparse->errorCode);
    }

    /*****************************************************************/
    /* Loop to extract attribute keyword (name):                     */
    /* Within this first loop, we should only find plain text and    */
    /* the equal sign.                                               */
    /*****************************************************************/
    while (pXmlparse->endOfXml == FALSE)
    {
        iteration++;

        if (iteration > pXmlparse->charCount)
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_NO_ATTR_NAME_END, 
                                      pXmlparse, 
                                      0);

            return(pXmlparse->errorCode);
        }

        xmlStringValue = FN_PARSE_NEXT_STRING(pXmlparse);

        TRMEMI(TRCI_XMLPARSER,
               pXmlparse->pBufferLast, 16,
               "Attribute keyword parse loop iteration=%i; "
               "xmlStringValue=%i, errorCode=%i, "
               "charParsed=%i, charRemaining=%i, "
               "bufferLast=%08X, bufferPos=%08X, pBufferLast:\n",
               iteration, 
               xmlStringValue, 
               pXmlparse->errorCode, 
               pXmlparse->charParsed, 
               pXmlparse->charRemaining, 
               pXmlparse->pBufferLast, 
               pXmlparse->pBufferPos);

        if ((xmlStringValue == XML_NOTE_STARTTAG) ||
            (xmlStringValue == XML_ENTITY_STARTTAG) ||
            (xmlStringValue == XML_ELEMEND_STARTTAG) ||
            (xmlStringValue == XML_COMMENT_STARTTAG) ||
            (xmlStringValue == XML_DECL_STARTTAG) ||
            (xmlStringValue == XML_STARTTAG) ||
            (xmlStringValue == XML_CDATA_ENDTAG) ||
            (xmlStringValue == XML_EMPTYELEM_ENDTAG) ||
            (xmlStringValue == XML_DECL_ENDTAG) ||
            (xmlStringValue == XML_ENDTAG))
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_XMLTAG_INSIDE_ATTR_NAME, 
                                      pXmlparse, 
                                      pXmlparse->lastParseLen);

            return(pXmlparse->errorCode);
        }
        /*************************************************************/
        /* An equal sign is the end of the attribute keyword.        */
        /*************************************************************/
        else if (xmlStringValue == XML_EQUALSIGN)
        {
            foundKeywordEnd = TRUE;

            break;
        }
        /*************************************************************/
        /* Anything else is considered keyword name text.            */
        /* Record the beginning of the attribute keyword name.       */
        /*************************************************************/
        else
        {
            FN_CONVERT_ASCII_NON_DISPLAY(pXmlparse->pBufferLast,
                                         1);

            if (pXmlattr->pKeyword == NULL)
            {
                pXmlattr->pKeyword = pXmlparse->pBufferLast; 
            }
        }
    }

    /*****************************************************************/
    /* If we did not find the end of the attribute name, flag        */
    /* the error.                                                    */
    /*****************************************************************/
    if (!(foundKeywordEnd))
    {
        if (pXmlparse->errorCode != RC_SUCCESS)
        {
            return(pXmlparse->errorCode);
        }

        XML_BYPASS_AND_MARK_ERROR(ERR_NO_ATTR_NAME_END, 
                                  pXmlparse, 
                                  0);

        return(pXmlparse->errorCode); 
    }
    else
    {
        if (pXmlattr->pKeyword != NULL)
        {
            if (pXmlattr->keywordLen == 0)
            {
                TRMSGI(TRCI_XMLPARSER,
                       "Finished attribute name\n");

                pXmlattr->flag |= ATTR_END;
                pXmlattr->keywordLen = pXmlparse->pBufferLast 
                                       - pXmlattr->pKeyword; 
            }
        }
    }

    /*****************************************************************/
    /* Loop to extract attribute value:                              */
    /* Within this second loop, we may find only an apostrophe or    */
    /* a quotation mark; the start of the attribute value.           */
    /*****************************************************************/
    lastRC = FN_PARSE_NEXT_NONBLANK(pXmlparse);

    if (lastRC != RC_SUCCESS)
    {
        XML_BYPASS_AND_MARK_ERROR(ERR_ALL_BLANK, 
                                  pXmlparse, 
                                  0);

        return(pXmlparse->errorCode);
    }

    iteration = 0;

    while (pXmlparse->endOfXml == FALSE)
    {
        iteration++;

        if (iteration > pXmlparse->charCount)
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_NO_ATTR_VALUE_END, 
                                      pXmlparse, 
                                      0);

            return(pXmlparse->errorCode);
        }

        xmlStringValue = FN_PARSE_NEXT_STRING(pXmlparse);

        TRMEMI(TRCI_XMLPARSER,
               pXmlparse->pBufferLast, 16,
               "Atrribute value start parse loop iteration=%i; "
               "xmlStringValue=%i, errorCode=%i, "
               "charParsed=%i, charRemaining=%i, "
               "bufferLast=%08X, bufferPos=%08X, pBufferLast:\n",
               iteration, 
               xmlStringValue, 
               pXmlparse->errorCode, 
               pXmlparse->charParsed, 
               pXmlparse->charRemaining, 
               pXmlparse->pBufferLast, 
               pXmlparse->pBufferPos);

        if ((xmlStringValue == XML_QUOTE) ||
            (xmlStringValue == XML_APOST))
        {
            foundValueStart = TRUE;
            xmlStartStringValue = xmlStringValue;

            break;
        }
        else
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_XMLTAG_INSIDE_ATTR_VALUE, 
                                      pXmlparse, 
                                      pXmlparse->lastParseLen);

            return(pXmlparse->errorCode);
        }
    }

    /*****************************************************************/
    /* If we did not find the start of the attribute value, then     */
    /* flag the error.                                               */
    /*****************************************************************/
    if (!(foundValueStart))
    {
        if (pXmlparse->errorCode != RC_SUCCESS)
        {
            return(pXmlparse->errorCode);
        }

        XML_BYPASS_AND_MARK_ERROR(ERR_NO_ATTR_VALUE_END, 
                                  pXmlparse, 
                                  0);

        return(pXmlparse->errorCode); 
    }
    else
    {
        pXmlattr->valueDelimiter = *pXmlparse->pBufferLast;
        pXmlattr->pValue = pXmlparse->pBufferPos; 
    }

    /*****************************************************************/
    /* Loop to extract attribute value:                              */
    /* Within this third loopm anything up to the matching delimiter */
    /* (apostrophe or quotation mark) is considered valid.           */
    /*****************************************************************/
    iteration = 0;

    while (pXmlparse->endOfXml == FALSE)
    {
        iteration++;

        if (iteration > pXmlparse->charCount)
        {
            XML_BYPASS_AND_MARK_ERROR(ERR_NO_ATTR_VALUE_END, 
                                      pXmlparse, 
                                      0);

            return(pXmlparse->errorCode);
        }

        xmlStringValue = FN_PARSE_NEXT_STRING(pXmlparse);

        TRMEMI(TRCI_XMLPARSER,
               pXmlparse->pBufferLast, 16,
               "Attribute value end parse loop iteration=%i; "
               "xmlStringValue=%i, errorCode=%i, "
               "charParsed=%i, charRemaining=%i, "
               "bufferLast=%08X, bufferPos=%08X, pBufferLast:\n",
               iteration, 
               xmlStringValue, 
               pXmlparse->errorCode, 
               pXmlparse->charParsed, 
               pXmlparse->charRemaining, 
               pXmlparse->pBufferLast, 
               pXmlparse->pBufferPos);

        if (xmlStringValue == xmlStartStringValue)
        {
            foundValueEnd = TRUE;

            break;
        }
        /*************************************************************/
        /* Anything else is considered attribute value text. Record  */
        /* the start of the attribure value.                         */
        /*************************************************************/
        else
        {
            FN_CONVERT_ASCII_NON_DISPLAY(pXmlparse->pBufferLast,
                                         1);

            if (pXmlattr->pValue == NULL)
            {
                pXmlattr->pValue = pXmlparse->pBufferLast; 
            }
        }
    }

    /*****************************************************************/
    /* If we did not find the ending delimiter, then flag the error. */
    /*****************************************************************/
    if (!(foundValueEnd))
    {
        if (pXmlparse->errorCode != RC_SUCCESS)
        {
            return(pXmlparse->errorCode);
        }

        XML_BYPASS_AND_MARK_ERROR(ERR_NO_ATTR_VALUE_END, 
                                  pXmlparse, 
                                  0);

        return(pXmlparse->errorCode); 
    }

    TRMSGI(TRCI_XMLPARSER,
           "Have a well formed XMLATTR; "
           "startTagCount=%i, endtagCount=%i\n",
           pXmlparse->starttagCount, 
           pXmlparse->endtagCount);

    if (pXmlattr->pValue != NULL)
    {
        if (pXmlattr->valueLen == 0)
        {
            TRMSGI(TRCI_XMLPARSER,
                   "Finished attribute value\n");

            pXmlattr->flag |= VALUE_END;
            pXmlattr->valueLen = pXmlparse->pBufferLast 
                                 - pXmlattr->pValue; 
        }
    }

    return RC_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_PARSE_NEXT_STRING                              */
/** Description:   Parse the current input data.                     */
/**                                                                  */
/** Beginning at the current position in the input, evaluate the     */
/** next character(s) to see if they match any of the XML string     */
/** definitions.  If not the next character defaults to plain text.  */
/**                                                                  */
/** NOTE: Under some conditions, the caller may consider that a      */
/** returned XML string is merely plain text; as, for instance,      */
/** a literal of "PUBLIC" within a string.                           */
/**                                                                  */
/** Exit conditions:                                                 */
/** RC = XML equate value for matched string                         */
/**                                                                  */
/** Updated fields withing XMLPARSE:                                 */
/** lastParseLen  = len of parsed subtring                           */
/** pBufferLast   = start of parsed substring                        */ 
/** pBufferPos    = new current buffer position                      */
/** charParsed    = entry charParsed + exit lastParseLen             */
/** charRemaining = entry charRemaining - exit lastParseLen          */
/** starttagCount = updated as required                              */
/** endtagCount   = updated as required                              */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_PARSE_NEXT_STRING"

extern int FN_PARSE_NEXT_STRING(struct XMLPARSE *pXmlparse)
{
    int                 lastRC;
    int                 xmlStringValue;

    /*************************************************************/
    /* Look for special XML tags in descending size order.       */
    /*************************************************************/
    if ((pXmlparse->charRemaining >= SIZEOF_STR_NOTE_STARTTAG) &&
        (memcmp(pXmlparse->pBufferPos,
                STR_NOTE_STARTTAG,
                SIZEOF_STR_NOTE_STARTTAG) == 0))
    {
        pXmlparse->starttagCount++;
        pXmlparse->lastParseLen = SIZEOF_STR_NOTE_STARTTAG;
        xmlStringValue = XML_NOTE_STARTTAG;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_ENTITY_STARTTAG) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_ENTITY_STARTTAG,
                     SIZEOF_STR_ENTITY_STARTTAG) == 0))
    {
        pXmlparse->starttagCount++;
        pXmlparse->lastParseLen = SIZEOF_STR_ENTITY_STARTTAG;
        xmlStringValue = XML_ENTITY_STARTTAG;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_PUBLIC) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_PUBLIC,
                     SIZEOF_STR_PUBLIC) == 0))
    {
        pXmlparse->lastParseLen = SIZEOF_STR_PUBLIC;
        xmlStringValue = XML_PUBLIC;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_SYSTEM) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_SYSTEM,
                     SIZEOF_STR_SYSTEM) == 0))
    {
        pXmlparse->lastParseLen = SIZEOF_STR_SYSTEM;
        xmlStringValue = XML_SYSTEM;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_START_CDATA) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_START_CDATA,
                     SIZEOF_STR_START_CDATA) == 0))
    {
        pXmlparse->lastParseLen = SIZEOF_STR_START_CDATA;
        xmlStringValue = XML_START_CDATA;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_EQU_APOST) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_EQU_APOST,
                     SIZEOF_STR_EQU_APOST) == 0))
    {
        pXmlparse->lastParseLen = SIZEOF_STR_EQU_APOST;
        xmlStringValue = XML_EQU_APOST;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_EQU_QUOTE) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_EQU_QUOTE,
                     SIZEOF_STR_EQU_QUOTE) == 0))
    {
        pXmlparse->lastParseLen = SIZEOF_STR_EQU_QUOTE;
        xmlStringValue = XML_EQU_QUOTE;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_EQU_AMPER) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_EQU_AMPER,
                     SIZEOF_STR_EQU_AMPER) == 0))
    {
        pXmlparse->lastParseLen = SIZEOF_STR_EQU_AMPER;
        xmlStringValue = XML_EQU_AMPER;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_EQU_GREATTHAN) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_EQU_GREATTHAN,
                     SIZEOF_STR_EQU_GREATTHAN) == 0))
    {
        pXmlparse->lastParseLen = SIZEOF_STR_EQU_GREATTHAN;
        xmlStringValue = XML_EQU_GREATTHAN;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_EQU_LESSTHAN) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_EQU_LESSTHAN,
                     SIZEOF_STR_EQU_LESSTHAN) == 0))
    {
        pXmlparse->lastParseLen = SIZEOF_STR_EQU_LESSTHAN;
        xmlStringValue = XML_EQU_LESSTHAN;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_CDATA_ENDTAG) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_CDATA_ENDTAG,
                     SIZEOF_STR_CDATA_ENDTAG) == 0))
    {
        pXmlparse->endtagCount++;
        pXmlparse->lastParseLen = SIZEOF_STR_CDATA_ENDTAG;
        xmlStringValue = XML_CDATA_ENDTAG;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_ELEMEND_STARTTAG) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_ELEMEND_STARTTAG,
                     SIZEOF_STR_ELEMEND_STARTTAG) == 0))
    {
        pXmlparse->starttagCount++;
        pXmlparse->lastParseLen = SIZEOF_STR_ELEMEND_STARTTAG;
        xmlStringValue = XML_ELEMEND_STARTTAG;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_EMPTYELEM_ENDTAG) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_EMPTYELEM_ENDTAG,
                     SIZEOF_STR_EMPTYELEM_ENDTAG) == 0))
    {
        pXmlparse->endtagCount++;
        pXmlparse->lastParseLen = SIZEOF_STR_EMPTYELEM_ENDTAG;
        xmlStringValue = XML_EMPTYELEM_ENDTAG;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_COMMENT_STARTTAG) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_COMMENT_STARTTAG,
                     SIZEOF_STR_COMMENT_STARTTAG) == 0))
    {
        pXmlparse->starttagCount++;
        pXmlparse->lastParseLen = SIZEOF_STR_COMMENT_STARTTAG;
        xmlStringValue = XML_COMMENT_STARTTAG;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_DECL_STARTTAG) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_DECL_STARTTAG,
                     SIZEOF_STR_DECL_STARTTAG) == 0))
    {
        pXmlparse->starttagCount++;
        pXmlparse->lastParseLen = SIZEOF_STR_DECL_STARTTAG;
        xmlStringValue = XML_DECL_STARTTAG;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_DECL_ENDTAG) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_DECL_ENDTAG,
                     SIZEOF_STR_DECL_ENDTAG) == 0))
    {
        pXmlparse->endtagCount++;
        pXmlparse->lastParseLen = SIZEOF_STR_DECL_ENDTAG;
        xmlStringValue = XML_DECL_ENDTAG;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_STARTTAG) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_STARTTAG,
                     SIZEOF_STR_STARTTAG) == 0))
    {
        pXmlparse->starttagCount++;
        pXmlparse->lastParseLen = SIZEOF_STR_STARTTAG;
        xmlStringValue = XML_STARTTAG;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_ENDTAG) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_ENDTAG,
                     SIZEOF_STR_ENDTAG) == 0))
    {
        pXmlparse->endtagCount++;
        pXmlparse->lastParseLen = SIZEOF_STR_ENDTAG;
        xmlStringValue = XML_ENDTAG;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_EQUALSIGN) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_EQUALSIGN,
                     SIZEOF_STR_EQUALSIGN) == 0))
    {
        pXmlparse->lastParseLen = SIZEOF_STR_EQUALSIGN;
        xmlStringValue = XML_EQUALSIGN;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_QUOTE) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_QUOTE,
                     SIZEOF_STR_QUOTE) == 0))
    {
        pXmlparse->lastParseLen = SIZEOF_STR_QUOTE;
        xmlStringValue = XML_QUOTE;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_APOST) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_APOST,
                     SIZEOF_STR_APOST) == 0))
    {
        pXmlparse->lastParseLen = SIZEOF_STR_APOST;
        xmlStringValue = XML_APOST;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_COMMA) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_COMMA,
                     SIZEOF_STR_COMMA) == 0))
    {
        pXmlparse->lastParseLen = SIZEOF_STR_COMMA;
        xmlStringValue = XML_COMMA;
    }
    else if ((pXmlparse->charRemaining >= SIZEOF_STR_BLANK) &&
             (memcmp(pXmlparse->pBufferPos,
                     STR_BLANK,
                     SIZEOF_STR_BLANK) == 0))
    {
        pXmlparse->lastParseLen = SIZEOF_STR_BLANK;
        xmlStringValue = XML_BLANK;
    }
    else
    {
        pXmlparse->lastParseLen = SIZEOF_STR_PLAIN_TEXT;
        xmlStringValue = XML_PLAIN_TEXT;
    }

    pXmlparse->pBufferLast = pXmlparse->pBufferPos;

    XML_BYPASS_INPUT(lastRC, 
                     pXmlparse, 
                     (pXmlparse->lastParseLen));   

    return xmlStringValue;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_PARSE_NEXT_NONBLANK                            */
/** Description:   Find the next non-blank character in the input.   */
/**                                                                  */
/** RC_FAILURE indicates that there were no more non-blank           */
/** characters in the input XML "blob" string.                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_PARSE_NEXT_NONBLANK"

extern int FN_PARSE_NEXT_NONBLANK(struct XMLPARSE *pXmlparse)
{
    int                 lastRC;
    int                 i;
    char                foundNonblank       = FALSE;

    pXmlparse->pBufferLast = pXmlparse->pBufferPos;

    for (i = 0;
        i < pXmlparse->charRemaining;
        i++)
    {
        if (pXmlparse->pBufferPos[i] != ' ')
        {
            foundNonblank = TRUE;

            break;
        }
    }

    XML_BYPASS_INPUT(lastRC, 
                     pXmlparse, 
                     i);

    if (!(foundNonblank))
    {
        return(RC_FAILURE); 
    }

    return RC_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_XML_RESTORE_SPECIAL_CHARS                      */
/** Description:   Restore "&" markets to their "real" equivalents.  */
/**                                                                  */
/** Re-translate XML "&" markers to their "real" text equivalents,   */
/** and compress the XML string in place, if appropriate.            */
/**                                                                  */
/** The input xmlStringLen will be updated if XML "&" markers were   */
/** found and the input XML "blob" string was compressed.            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_XML_RESTORE_SPECIAL_CHARS"

extern void FN_XML_RESTORE_SPECIAL_CHARS(char *pXmlString,
                                         int  *pXmlStringLen)
{
    int                 i;
    int                 j;
    int                 lenRemaining;   
    int                 xmlStringLen        = *pXmlStringLen;

    TRMEMI(TRCI_XMLPARSER,
           pXmlString, xmlStringLen,
           "Entered; input pXmlString:\n");

    for (i = 0, j = 0;
        i < xmlStringLen;
        j++)
    {
        lenRemaining = xmlStringLen - 1;

        if ((lenRemaining >= SIZEOF_STR_EQU_APOST) &&
            (memcmp(&(pXmlString[i]),
                    STR_EQU_APOST,
                    SIZEOF_STR_EQU_APOST) == 0))
        {
            memcpy(&(pXmlString[j]),
                   "'",
                   1);

            i += SIZEOF_STR_EQU_APOST;
        }
        else if ((lenRemaining >= SIZEOF_STR_EQU_QUOTE) &&
                 (memcmp(&(pXmlString[i]),
                         STR_EQU_QUOTE,
                         SIZEOF_STR_EQU_QUOTE) == 0))
        {
            pXmlString[j] = '"';
            i += SIZEOF_STR_EQU_QUOTE;
        }
        else if ((lenRemaining >= SIZEOF_STR_EQU_AMPER) &&
                 (memcmp(&(pXmlString[i]),
                         STR_EQU_AMPER,
                         SIZEOF_STR_EQU_AMPER) == 0))
        {
            pXmlString[j] = '&';
            i += SIZEOF_STR_EQU_AMPER;
        }
        else if ((lenRemaining >= SIZEOF_STR_EQU_GREATTHAN) &&
                 (memcmp(&(pXmlString[i]),
                         STR_EQU_GREATTHAN,
                         SIZEOF_STR_EQU_GREATTHAN) == 0))
        {
            pXmlString[j] = '>';
            i += SIZEOF_STR_EQU_GREATTHAN;
        }
        else if ((lenRemaining >= SIZEOF_STR_EQU_LESSTHAN) &&
                 (memcmp(&(pXmlString[i]),
                         STR_EQU_LESSTHAN,
                         SIZEOF_STR_EQU_LESSTHAN) == 0))
        {
            pXmlString[j] = '<';
            i += SIZEOF_STR_EQU_LESSTHAN;
        }
        else
        {
            pXmlString[j] = pXmlString[i];
            i++;
        }
    }

    if (j != i)
    {
        TRMEMI(TRCI_XMLPARSER,
               pXmlString, j,
               "In len=%i, out len=%i; updated pXmlString:\n",
               xmlStringLen,
               j);

        *pXmlStringLen = j;
    }

    return;
}



