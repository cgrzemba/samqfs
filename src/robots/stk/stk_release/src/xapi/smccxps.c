/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      smccxps.c                                        */
/** Description:    XAPI client XML parser structure create and      */
/**                 link service.                                    */
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


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_CREATE_XMLPARSE                                */
/** Description:   Allocate an XMLPARSE and update its pointer.      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_CREATE_XMLPARSE"

extern struct XMLPARSE *FN_CREATE_XMLPARSE(void)
{
    struct XMLPARSE    *pXmlparse;
    int                 lastRC;

    pXmlparse = (struct XMLPARSE*) malloc(sizeof(struct XMLPARSE));

    TRMSGI(TRCI_STORAGE,
           "malloc XMLPASRSE=%08X, len=%i\n",
           pXmlparse,
           sizeof(struct XMLPARSE));

    memset(pXmlparse, 0, sizeof(struct XMLPARSE));

    memcpy(pXmlparse->leafEye,
           "LEAF",
           sizeof(pXmlparse->leafEye));

    memcpy(pXmlparse->bptrEye,
           "BPTR",
           sizeof(pXmlparse->bptrEye));

    memcpy(pXmlparse->xflgEye,
           "XFLG",
           sizeof(pXmlparse->xflgEye));

    memcpy(pXmlparse->xerrEye,
           "XERR",
           sizeof(pXmlparse->xerrEye));

    return pXmlparse;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_CREATE_XMLRAWIN                                */
/** Description:   Allocate an XMLRAWIN and update the XMLPARSE.     */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_CREATE_XMLRAWIN"

extern int FN_CREATE_XMLRAWIN(struct XMLPARSE *pXmlparse,
                              char            *pData,
                              int              dataLen)
{
    int                 xmlrawinSize;
    struct XMLRAWIN    *pNewXmlrawin;
    int                 lastRC;

    xmlrawinSize = sizeof(struct XMLRAWIN) + dataLen;

    pNewXmlrawin = (struct XMLRAWIN*) malloc(xmlrawinSize);

    TRMSGI(TRCI_STORAGE,
           "malloc XMLRAWIN=%08X, len=%i\n",
           pNewXmlrawin,
           xmlrawinSize);

    /*****************************************************************/
    /* Initialize and link the new XMLRAWIN into XMLPARSE            */
    /*****************************************************************/
    memset(pNewXmlrawin, 0, xmlrawinSize);

    pNewXmlrawin->xmlrawinSize = xmlrawinSize;
    pNewXmlrawin->dataLen = dataLen;

    memcpy((char*) &(pNewXmlrawin->pData[0]),
           pData,
           dataLen);

    pNewXmlrawin->pNextXmlrawin = pXmlparse->pHocXmlrawin;
    pXmlparse->pHocXmlrawin = pNewXmlrawin;
    pXmlparse->pCurrXmlrawin = pNewXmlrawin;

    return RC_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_CREATE_XMLCNVIN                                */
/** Description:   Allocate an XMLCNVIN and update the XMLPARSE.     */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_CREATE_XMLCNVIN"

extern int FN_CREATE_XMLCNVIN(struct XMLPARSE *pXmlparse)
{
    struct XMLRAWIN    *pXmlrawin           = pXmlparse->pCurrXmlrawin;
    struct XMLCNVIN    *pNewXmlcnvin        = NULL;
    struct XMLCNVIN    *pNewHocXmlcnvin     = NULL;
    struct XMLCNVIN    *pNewCurrXmlcnvin    = NULL;
    struct XMLCNVIN    *pDeleteXmlcnvin;
    struct XMLCNVIN    *pCurrXmlcnvin       = pXmlparse->pCurrXmlcnvin;

    int                 i;
    char                moreThanOneXmlcnvin = FALSE;

    struct XMLCNVIN  *(*pRoutine) (struct XMLPARSE *pXmlparse,
                                   char             checkResultsFlag);

#define NUM_CONV_ROUTINES  2

    void *pConvRoutine[NUM_CONV_ROUTINES] =
    {
        FN_XML_ASCII_TO_ASCII,
        FN_XML_EBCDIC_TO_ASCII,
    };

    char convType[NUM_CONV_ROUTINES] =
    {
        ASCII,
        EBCDIC,
    };

    /*****************************************************************/
    /* If the XMLPARSE structure has an address of a parse routine,  */
    /* then use it (i.e. we've already determined the translation).  */
    /*****************************************************************/
    pRoutine = (struct XMLCNVIN* (*) (struct XMLPARSE*, char))
               pXmlparse->pConvRoutine;

    if (pRoutine != NULL)
    {
        pNewXmlcnvin = pRoutine(pXmlparse,
                                FALSE);
    }
    /*****************************************************************/
    /* Else determine what type of translation to perform:           */
    /* Initially, we're assuming EBCDIC or ASCII raw input to ASCII  */
    /* converted output. We'll have to add more here to make this    */
    /* true XML.                                                     */
    /*****************************************************************/
    else
    {
        for (i = 0;
            i < NUM_CONV_ROUTINES;
            i++)
        {
            pRoutine = (struct XMLCNVIN* (*) (struct XMLPARSE*, char))
                       pConvRoutine[i];

            pNewXmlcnvin = pRoutine(pXmlparse,
                                    TRUE);

            /*********************************************************/
            /* Try to find the best conversion routine to use.       */
            /* If we found no conversion errors, then the current    */
            /* translation will be the best.  In that case, exit     */
            /* this loop and delete any non-best XMLCNVIN(s).        */
            /*********************************************************/
            if (pNewXmlcnvin != NULL)
            {
                pNewXmlcnvin->pRoutine = pRoutine;
                pNewXmlcnvin->inputEncoding = convType[i];

                if (pNewHocXmlcnvin == NULL)
                {
                    pNewHocXmlcnvin = pNewXmlcnvin;
                    pNewCurrXmlcnvin = pNewXmlcnvin;
                }
                else
                {
                    moreThanOneXmlcnvin = TRUE;
                    pNewCurrXmlcnvin->pNextTryXmlcnvin = pNewXmlcnvin;
                    pNewCurrXmlcnvin = pNewXmlcnvin;
                }

                if (pNewCurrXmlcnvin->nonTextCount == 0)
                {
                    break;
                }
            }
        }

        /*************************************************************/
        /* If we have more than 1 XMLCNVIN, choose the one that      */
        /* starts with a "<" and has the fewest non-text characters, */
        /* and delete the rest.                                      */
        /*************************************************************/
        if (moreThanOneXmlcnvin)
        {
            pNewXmlcnvin = pNewHocXmlcnvin;
            pNewCurrXmlcnvin = pNewHocXmlcnvin->pNextTryXmlcnvin;

            while (pNewCurrXmlcnvin != NULL)
            {
                if ((pNewXmlcnvin->pData[0] != '<') &&
                    (pNewCurrXmlcnvin->pData[0] == '<'))
                {
                    pDeleteXmlcnvin = pNewXmlcnvin;
                    pNewXmlcnvin = pNewCurrXmlcnvin;
                }
                else if (((pNewXmlcnvin->pData[0] != '<') &&
                          (pNewCurrXmlcnvin->pData[0] != '<')) &&
                         (pNewXmlcnvin->nonTextCount > pNewCurrXmlcnvin->nonTextCount))
                {
                    pDeleteXmlcnvin = pNewXmlcnvin;
                    pNewXmlcnvin = pNewCurrXmlcnvin;
                }
                else
                {
                    pDeleteXmlcnvin = pNewCurrXmlcnvin;
                }

                pNewCurrXmlcnvin = pNewCurrXmlcnvin->pNextTryXmlcnvin;

                TRMSGI(TRCI_STORAGE,
                       "free XMLCNVIN=%08X, len=%i\n",
                       pDeleteXmlcnvin,
                       pDeleteXmlcnvin->xmlcnvinSize);

                free(pDeleteXmlcnvin);
            }
        }
    }

    if (pNewXmlcnvin != NULL)
    {
        if (moreThanOneXmlcnvin)
        {
            TRMEMI(TRCI_XMLPARSER,
                   pNewXmlcnvin, pNewXmlcnvin->xmlcnvinSize,
                   "Selected XMLCNVIN:\n");
        }

        pNewXmlcnvin->pNextTryXmlcnvin = NULL;
        pXmlparse->pConvRoutine = pNewXmlcnvin->pRoutine;
        pXmlparse->inputEncoding = pNewXmlcnvin->inputEncoding;
    }
    else
    {
        return RC_FAILURE;
    }

    if (pXmlparse->pCurrXmlcnvin == NULL)
    {
        pXmlparse->pHocXmlcnvin = pNewXmlcnvin;
        pXmlparse->pCurrXmlcnvin = pNewXmlcnvin;
        pCurrXmlcnvin = pNewXmlcnvin;
    }
    else
    {
        pCurrXmlcnvin = pXmlparse->pCurrXmlcnvin;
        pCurrXmlcnvin->pNextXmlcnvin = pNewXmlcnvin;
        pXmlparse->pCurrXmlcnvin = pNewXmlcnvin;
        pCurrXmlcnvin = pNewXmlcnvin;
    }

    pXmlparse->pBufferBeg = &(pCurrXmlcnvin->pData[0]);
    pXmlparse->pBufferEnd = &(pCurrXmlcnvin->pData[(pCurrXmlcnvin->dataLen)-1]);
    pXmlparse->pBufferPos = &(pCurrXmlcnvin->pData[0]);
    pXmlparse->charCount = pCurrXmlcnvin->dataLen;
    pXmlparse->charParsed = 0;
    pXmlparse->charRemaining = pCurrXmlcnvin->dataLen;

    return RC_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_CREATE_DECL                                    */
/** Description:   Allocate an XMLDECL and update the XMLPARSE.      */
/**                                                                  */
/** An XML declaration is always considered a level 0 leaf.          */
/** If present, it should be the 1st thing in the XML "blob".        */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_CREATE_XMLDECL"

extern int FN_CREATE_XMLDECL(struct XMLPARSE *pXmlparse)
{
    struct XMLDECL     *pCurrXmldecl        = pXmlparse->pCurrXmldecl;
    struct XMLDECL     *pNewXmldecl;
    int                 lastRC;
    int                 i                   = 0;

    pNewXmldecl = (struct XMLDECL*) malloc(sizeof(struct XMLDECL));

    TRMSGI(TRCI_STORAGE,
           "malloc XMLDECL=%08X, len=%i\n",
           pNewXmldecl,
           sizeof(struct XMLDECL));

    /*****************************************************************/
    /* Initialize the new XMLDECL                                    */
    /*****************************************************************/
    memset(pNewXmldecl, 0, sizeof(struct XMLDECL));

    memcpy(pNewXmldecl->type,
           "DECL",
           sizeof(pNewXmldecl->type));

    pNewXmldecl->sequence = pXmlparse->starttagCount;
    pXmlparse->pCurrLeaf = pNewXmldecl;
    pNewXmldecl->flag = DECL_START;

    /*****************************************************************/
    /* Link the new XMLDECL                                          */
    /*****************************************************************/
    if (pCurrXmldecl == NULL)
    {
        pXmlparse->pHocXmldecl = pNewXmldecl;
        pXmlparse->pCurrXmldecl = pNewXmldecl;
    }
    else
    {
        pCurrXmldecl->pNextXmldecl = pNewXmldecl;
        pXmlparse->pCurrXmldecl = pNewXmldecl;
    }

    /*****************************************************************/
    /* Parse the remainder of the declaration.                       */
    /*****************************************************************/
    lastRC = FN_PARSE_DECL(pXmlparse);

    TRMSGI(TRCI_XMLPARSER,
           "lastRC=%d from FN_PARSE_DECL(XMLPARSE=%08X)\n",
           lastRC,
           pXmlparse);

    if (lastRC == RC_SUCCESS)
    {
        pCurrXmldecl = pXmlparse->pCurrXmldecl;

        TRMEMI(TRCI_XMLPARSER,
               pCurrXmldecl->pDeclaration, pCurrXmldecl->declLen,
               "XMLDECL declaration data:\n");
    }

    return lastRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_CREATE_COMM                                    */
/** Description:   Allocate an XMLCOMM and update the XMLPARSE.      */
/**                                                                  */
/** Even though XMLCOMM(s) may be found anywhere in the XML "blob",  */
/** we hang them off of the XMLPARSE because we will eventually      */
/** free them, as they are of no real value in the data hierarchy.   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_CREATE_XMLCOMM"

extern int FN_CREATE_XMLCOMM(struct XMLPARSE *pXmlparse)
{
    struct XMLCOMM     *pCurrXmlcomm        = pXmlparse->pCurrXmlcomm;
    struct XMLCOMM     *pNewXmlcomm;
    int                 lastRC;
    int                 i                   = 0;
    struct XMLLEAF     *pXmlleaf            = pXmlparse->pCurrLeaf;

    pNewXmlcomm = (struct XMLCOMM*) malloc(sizeof(struct XMLCOMM));

    TRMSGI(TRCI_STORAGE,
           "malloc XMLCOMM=%08X, len=%i\n",
           pNewXmlcomm,
           sizeof(struct XMLCOMM));

    /*****************************************************************/
    /* Initialize the new XMLCOMM                                    */
    /*****************************************************************/
    memset(pNewXmlcomm, 0, sizeof(struct XMLCOMM));

    memcpy(pNewXmlcomm->type,
           "COMM",
           sizeof(pNewXmlcomm->type));

    pNewXmlcomm->sequence = pXmlparse->starttagCount;

    if (pXmlleaf != NULL)
    {
        pNewXmlcomm->parentSequence = pXmlleaf->sequence;
    }

    pXmlparse->pCurrLeaf = pNewXmlcomm;
    pNewXmlcomm->flag = COMM_START;

    /*****************************************************************/
    /* Link the new XMLCOMM                                          */
    /*****************************************************************/
    if (pCurrXmlcomm == NULL)
    {
        pXmlparse->pHocXmlcomm = pNewXmlcomm;
        pXmlparse->pCurrXmlcomm = pNewXmlcomm;
    }
    else
    {
        pCurrXmlcomm->pNextXmlcomm = pNewXmlcomm;
        pXmlparse->pCurrXmlcomm = pNewXmlcomm;
    }

    /*****************************************************************/
    /* Parse the remainder of the comment.                           */
    /*****************************************************************/
    lastRC = FN_PARSE_COMM(pXmlparse);

    TRMSGI(TRCI_XMLPARSER,
           "lastRC=%d from FN_PARSE_COMM(XMLPARSE=%08X)\n",
           lastRC,
           pXmlparse);

    if (lastRC == RC_SUCCESS)
    {
        pCurrXmlcomm = pXmlparse->pCurrXmlcomm;

        TRMEMI(TRCI_XMLPARSER,
               pCurrXmlcomm->pComment, pCurrXmlcomm->commLen,
               "XMLCOMM comment data:\n");
    }

    return lastRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_CREATE_XMLELEM                                 */
/** Description:   Allocate an XMLELEM and update the XMLPARSE.      */
/**                                                                  */
/** XMLELEMs form the leaves of the XMLPARSE data hierarchy tree.    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_CREATE_XMLELEM"

extern int FN_CREATE_XMLELEM(struct XMLPARSE *pXmlparse)
{
    struct XMLELEM     *pCurrXmlelem        = pXmlparse->pCurrXmlelem;
    struct XMLELEM     *pHocXmlelem         = pXmlparse->pHocXmlelem;
    struct XMLELEM     *pNewXmlelem;
    struct XMLELEM     *pLastSiblingXmlelem;
    int                 level               = pXmlparse->currLevel;
    int                 lastRC;

    /*****************************************************************/
    /* If the current XMLELEM is not complete, then this XMLELEM     */
    /* must be a child. Otherwise, we must be the same level as      */
    /* as the current XMLELEM.                                       */
    /*****************************************************************/
    if (pCurrXmlelem != NULL)
    {
        if (!(pCurrXmlelem->flag & ELEM_END))
        {
            level++;
        }
    }
    else
    {
        level = 1;
    }

    pXmlparse->currLevel = level;

    if (pXmlparse->currLevel > pXmlparse->maxLevel)
    {
        pXmlparse->maxLevel = pXmlparse->currLevel;
    }

    pNewXmlelem = (struct XMLELEM*) malloc(sizeof(struct XMLELEM));

    TRMSGI(TRCI_STORAGE,
           "malloc XMLELEM=%08X, len=%i\n",
           pNewXmlelem,
           sizeof(struct XMLELEM));

    /*****************************************************************/
    /* Initialize the new XMLELEM.                                   */
    /*****************************************************************/
    memset(pNewXmlelem, 0, sizeof(struct XMLELEM));

    memcpy(pNewXmlelem->type,
           "ELEM",
           sizeof(pNewXmlelem->type));

    pNewXmlelem->level = level;
    pNewXmlelem->sequence = pXmlparse->starttagCount;
    pXmlparse->pCurrLeaf = pNewXmlelem;
    pNewXmlelem->flag = (ELEM_START + START_NAME_START);

    /*****************************************************************/
    /* Link the new XMLELEM based upon its relative level            */
    /*****************************************************************/
    if (pCurrXmlelem == NULL)
    {
        TRMSGI(TRCI_XMLPARSER,
               "XMLELEM seq=%i, level=%i at %08X is hoc XMLELEM\n",
               pNewXmlelem->sequence,
               pNewXmlelem->level,
               pNewXmlelem);

        pXmlparse->pHocXmlelem = pNewXmlelem;
        pXmlparse->pCurrXmlelem = pNewXmlelem;
    }
    else if (level == pCurrXmlelem->level)
    {
        TRMSGI(TRCI_XMLPARSER,
               "XMLELEM seq=%i level=%i at %08X is sibling of "
               "XMLELEM seq=%i level=%i at %08X\n",
               pNewXmlelem->sequence,
               pNewXmlelem->level,
               pNewXmlelem,
               pCurrXmlelem->sequence,
               pCurrXmlelem->level,
               pCurrXmlelem);

        pCurrXmlelem->pYoungerXmlelem = pNewXmlelem;
        pNewXmlelem->pOlderXmlelem = pCurrXmlelem;
        pNewXmlelem->pParentXmlelem = pCurrXmlelem->pParentXmlelem;
        pXmlparse->pCurrXmlelem = pNewXmlelem;
    }
    else
    {
        XML_FIND_LAST_SIBLING_IN_XMLELEM(pCurrXmlelem,
                                         pLastSiblingXmlelem);

        if (pLastSiblingXmlelem == NULL)
        {
            TRMSGI(TRCI_XMLPARSER,
                   "XMLELEM seq=%i level=%i at %08X is 1st child of "
                   "XMLELEM seq=%i level=%i at %08X\n",
                   pNewXmlelem->sequence,
                   pNewXmlelem->level,
                   pNewXmlelem,
                   pCurrXmlelem->sequence,
                   pCurrXmlelem->level,
                   pCurrXmlelem);

            pCurrXmlelem->pChildXmlelem = pNewXmlelem;
        }
        else
        {
            TRMSGI(TRCI_XMLPARSER,
                   "XMLELEM seq=%i level=%i at %08X is next child of "
                   "XMLELEM seq=%i level=%i at %08X\n",
                   pNewXmlelem->sequence,
                   pNewXmlelem->level,
                   pNewXmlelem,
                   pCurrXmlelem->sequence,
                   pCurrXmlelem->level,
                   pCurrXmlelem);

            pLastSiblingXmlelem->pYoungerXmlelem = pNewXmlelem;
            pNewXmlelem->pOlderXmlelem = pLastSiblingXmlelem;
        }

        pNewXmlelem->pParentXmlelem = pCurrXmlelem;
        pXmlparse->pCurrXmlelem = pNewXmlelem;
    }

    /*****************************************************************/
    /* Now parse the remainder of this element.                      */
    /*****************************************************************/
    lastRC = FN_PARSE_ELEM(pXmlparse);

    TRMEMI(TRCI_XMLPARSER,
           pNewXmlelem, sizeof(struct XMLELEM),
           "FN_PARSE_ELEM RC=%08X; XMLELEM:\n",
           lastRC);

    if (lastRC == RC_SUCCESS)
    {
        pCurrXmlelem = pXmlparse->pCurrXmlelem;

        TRMEMI(TRCI_XMLPARSER,
               pCurrXmlelem->pStartElemName, pCurrXmlelem->startElemLen,
               "XMLELEM element name:\n");

        if (pCurrXmlelem->contentLen > 0)
        {
            TRMEMI(TRCI_XMLPARSER,
                   pCurrXmlelem->pContent, pCurrXmlelem->contentLen,
                   "XMLELEM element content:\n");
        }
    }

    return lastRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_CREATE_XMLATTR                                 */
/** Description:   Allocate an XMLATTR and update the XMLELEM.       */
/**                                                                  */
/** XMLATTR structures are anchored from their corresponding XMLELEM.*/
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_CREATE_XMLATTR"

extern int FN_CREATE_XMLATTR(struct XMLPARSE *pXmlparse)
{
    struct XMLELEM     *pCurrXmlelem        = pXmlparse->pCurrXmlelem;
    struct XMLATTR     *pCurrXmlattr        = pCurrXmlelem->pCurrXmlattr;
    struct XMLATTR     *pNewXmlattr;
    int                 lastRC;
    char                keywordString[64];
    char                valueString[64];

    pNewXmlattr = (struct XMLATTR*) malloc(sizeof(struct XMLATTR));

    TRMSGI(TRCI_STORAGE,
           "malloc XMLATTR=%08X, len=%i\n",
           pNewXmlattr,
           sizeof(struct XMLATTR));

    /*****************************************************************/
    /* Initialize the new XMLATTR.                                   */
    /*****************************************************************/
    memset(pNewXmlattr, 0, sizeof(struct XMLATTR));

    memcpy(pNewXmlattr->type,
           "ATTR",
           sizeof(pNewXmlattr->type));

    pNewXmlattr->sequence = pXmlparse->starttagCount;
    pXmlparse->pCurrLeaf = pNewXmlattr;
    pNewXmlattr->flag = ATTR_START;
    pNewXmlattr->pParentXmlelem = pCurrXmlelem;

    /*****************************************************************/
    /* Link the new XMLATTR to its XMLELEM.                          */
    /*****************************************************************/
    if (pCurrXmlattr == NULL)
    {
        pCurrXmlelem->pHocXmlattr = pNewXmlattr;
        pCurrXmlelem->pCurrXmlattr = pNewXmlattr;
    }
    else
    {
        pCurrXmlattr->pNextXmlattr = pNewXmlattr;
        pCurrXmlelem->pCurrXmlattr = pNewXmlattr;
    }

    /*****************************************************************/
    /* Parse the remainder of the attribute.                         */
    /*****************************************************************/
    lastRC = FN_PARSE_ATTR(pXmlparse);

    TRMSGI(TRCI_XMLPARSER,
           "lastRC=%d from FN_PARSE_ATTR(XMLPARSE=%08X)\n",
           lastRC,
           pXmlparse);

    if (lastRC == RC_SUCCESS)
    {
        pCurrXmlattr = pCurrXmlelem->pCurrXmlattr;

        STRIP_TRAILING_BLANKS(pCurrXmlattr->pKeyword,
                              keywordString,
                              pCurrXmlattr->keywordLen);

        STRIP_TRAILING_BLANKS(pCurrXmlattr->pValue,
                              valueString,
                              pCurrXmlattr->valueLen);

        TRMSGI(TRCI_XMLPARSER,
               "XMLATTR KEYWORD=VALUE pair %s=%s\n",
               keywordString,
               valueString);
    }

    return lastRC;
}



