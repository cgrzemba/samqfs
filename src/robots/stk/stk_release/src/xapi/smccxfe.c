/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011 Oracle and/or its affiliates.           */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      smccxfe.c                                        */
/** Description:    XAPI client XML parser find element service.     */
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


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static struct XMLELEM *matchXmlelem(struct XMLELEM *pCurrXmlelem,
                                    char           *pElemName);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_FIND_ELEMENT_BY_NAME                           */
/** Description:   Find elememt (XMLELEM) by name.                   */
/**                                                                  */
/** Using the input element name, return a pointer to the first      */
/** occurrence of the element in the XMLPARSE data hierarchy tree.   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_FIND_ELEMENT_BY_NAME"

extern struct XMLELEM *FN_FIND_ELEMENT_BY_NAME(struct XMLPARSE *pXmlparse,
                                               struct XMLELEM  *pStartXmlelem,
                                               char            *pElemName)
{
    struct XMLELEM     *pCurrXmlelem        = pStartXmlelem;
    struct XMLELEM     *pMatchedXmlelem;

    if (pCurrXmlelem == NULL)
    {
        pCurrXmlelem = pXmlparse->pHocXmlelem;
    }

    pMatchedXmlelem = matchXmlelem(pCurrXmlelem,
                                   pElemName); 

    return pMatchedXmlelem;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_FIND_NEXT_CHILD_ELEMENT                        */
/** Description:   Find next child element for specified parent.     */
/**                                                                  */
/** Using the input parent element pointer, and child element        */
/** pointer, return a pointer to the next XMLELEM (i.e. the next     */
/** younger sibling) for the same parent.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_FIND_NEXT_CHILD_ELEMENT"

extern struct XMLELEM *FN_FIND_NEXT_CHILD_ELEMENT(struct XMLPARSE *pXmlparse,
                                                  struct XMLELEM  *pParentXmlelem,
                                                  struct XMLELEM  *pCurrChildXmlelem)
{
    if (pCurrChildXmlelem == NULL)
    {
        return pParentXmlelem->pChildXmlelem;
    }
    else
    {
        return pCurrChildXmlelem->pYoungerXmlelem;
    }
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_FIND_NEXT_SIBLING_BY_NAME                      */
/** Description:   Find next matching child for specified parent.    */
/**                                                                  */
/** Using the input parent element pointer, and child element        */
/** pointer, return a pointer to the next XMLELEM (i.e. the next     */
/** younger sibling) for the same parent that matches the input      */
/** element name.                                                    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_FIND_NEXT_SIBLING_BY_NAME"

extern struct XMLELEM *FN_FIND_NEXT_SIBLING_BY_NAME(struct XMLPARSE *pXmlparse,
                                                    struct XMLELEM  *pParentXmlelem,
                                                    struct XMLELEM  *pCurrChildXmlelem,
                                                    char            *pElemName)
{
    struct XMLELEM     *pStartingXmlelem;
    struct XMLELEM     *pNextYoungerXmlelem;
    struct XMLELEM     *pMatchingXmlelem    = NULL;
    int                 elemNameLen;

    if (pCurrChildXmlelem == NULL)
    {
        pStartingXmlelem = pParentXmlelem->pChildXmlelem;
    }
    else
    {
        pStartingXmlelem = pCurrChildXmlelem;
    }

    if (pStartingXmlelem == NULL)
    {
        return NULL;
    }

    pNextYoungerXmlelem = pStartingXmlelem->pYoungerXmlelem;
    elemNameLen = strlen(pElemName);

    while (pNextYoungerXmlelem != NULL)
    {
        if (memcmp(pElemName,
                   pNextYoungerXmlelem->pStartElemName,
                   elemNameLen) == 0)
        {
            pMatchingXmlelem = pNextYoungerXmlelem;

            break;
        }

        pNextYoungerXmlelem = pNextYoungerXmlelem->pYoungerXmlelem;
    }

    return pMatchingXmlelem;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_FIND_ATTRIBUTE_BY_KEYWORD                      */
/** Description:   Find matching attribute for specific element.     */
/**                                                                  */
/** Using the input XMLELEM and attribute keyword name, return a     */
/** pointer to the first occurrence of the named XMLATTR in the      */
/** chain of XMLATTR(s) anchored from the input XMLELEM.             */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_FIND_ATTRIBUTE_BY_KEYWORD"

extern struct XMLATTR *FN_FIND_ATTRIBUTE_BY_KEYWORD(struct XMLPARSE *pXmlparse,
                                                    struct XMLELEM  *pParentXmlelem,
                                                    char            *pKeyword)
{
    struct XMLATTR     *pCurrXmlattr;
    int                 keywordLen;

    if (pKeyword != NULL)
    {
        keywordLen = strlen(pKeyword);

        if (pParentXmlelem != NULL)
        {
            pCurrXmlattr = pParentXmlelem->pHocXmlattr;

            while (pCurrXmlattr != NULL)
            {
                if (memcmp(pCurrXmlattr->pKeyword,
                           pKeyword,
                           keywordLen) == 0)
                {
                    return pCurrXmlattr;
                }

                pCurrXmlattr = pCurrXmlattr->pNextXmlattr;
            }
        }
    }

    return NULL;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: matchXmlelem                                      */
/** Description:   Find elememt (XMLELEM) by name.                   */
/**                                                                  */
/** Using the input XMLELEM pointer and element name, return a       */
/** pointer to the first occurrence of the element in the XMLPARSE   */
/** data hierarchy subtree (beginning with the input XMLELEM)        */
/** that matches the input name.                                     */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "matchXmlelem"    

static struct XMLELEM *matchXmlelem(struct XMLELEM *pCurrXmlelem,
                                    char           *pElemName)
{
    struct XMLELEM     *pMatchedXmlelem;
    int                 elemNameLen;

    elemNameLen = strlen(pElemName);

    while (pCurrXmlelem != NULL)
    {
        if (elemNameLen == pCurrXmlelem->startElemLen)
        {
            if (memcmp(pElemName,
                       pCurrXmlelem->pStartElemName,
                       elemNameLen) == 0)
            {
                pMatchedXmlelem = pCurrXmlelem;

                return pMatchedXmlelem;
            }
        }

        if (pCurrXmlelem->pChildXmlelem != NULL)
        {
            pMatchedXmlelem = matchXmlelem(pCurrXmlelem->pChildXmlelem,
                                           pElemName);

            if (pMatchedXmlelem != NULL)
            {
                return pMatchedXmlelem;
            }
        }

        pCurrXmlelem = pCurrXmlelem->pYoungerXmlelem;
    }

    return NULL;
}


