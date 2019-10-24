/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      smccxcs.c                                        */
/** Description:    XAPI client XML parser common services.          */
/**                                                                  */
/**                 These services are used to add elements and      */
/**                 attributes to an XMLPARSE data hierarchy tree,   */
/**                 and to generate an XML text "blob" from an       */
/**                 XMLPARSE data hierarchy tree.                    */
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
static void getElemStorage(struct XMLPARSE  *pXmlparse,
                           struct XMLELEM  **pNewXmlelemPtr,
                           struct XMLDATA  **pNewXmldataPtr,
                           int               newXmldataLen);

static void getAttrStorage(struct XMLPARSE  *pXmlparse,
                           struct XMLATTR  **pNewXmlelemPtr,
                           struct XMLDATA  **pNewXmldataPtr,
                           int               newXmldataLen);

static void getDataStorage(struct XMLPARSE  *pXmlparse,
                           struct XMLDATA  **pNewXmldataPtr,
                           int               newXmldataLen);

static int calcDeclSize(struct XMLDECL *pXmldecl);

static int calcElemSize(struct XMLELEM *pXmlelem);

static void buildDeclText(struct XMLDECL  *pXmldecl,
                          char           **pBufferPosPtr);

static void buildElemText(struct XMLELEM  *pXmlelem,
                          char           **pBufferPosPtr);

static void freeXmlrawin(struct XMLRAWIN *pInputXmlrawin);

static void freeXmlcnvin(struct XMLCNVIN *pInputXmlcnvin);

static void freeXmldecl(struct XMLDECL *pInputXmldecl);

static void freeXmlcomm(struct XMLCOMM *pInputXmlcomm);

static void freeXmlelem(struct XMLELEM *pInputXmlelem);

static void freeXmldata(struct XMLDATA *pInputXmldata);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_ADD_ELEM_TO_HIERARCHY                          */
/** Description:   Add an element to the data hierarchy tree.        */
/**                                                                  */
/** Add a new XMLELEM data or header element to the XMLPARSE         */
/** data hierarchy tree.  The new XMLELEM will be placed at the      */
/** end of the sibling chain for the specified parent element as     */
/** the "youngest" sibling.                                          */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_ADD_ELEM_TO_HIERARCHY"

extern int FN_ADD_ELEM_TO_HIERARCHY(struct XMLPARSE *pXmlparse,
                                    struct XMLELEM  *pParentXmlelem,
                                    char            *pElemName,
                                    char            *pElemContent,
                                    int              inputElemContentLen)
{
    struct XMLELEM     *pCurrXmlelem        = pXmlparse->pCurrXmlelem;
    struct XMLELEM     *pHocXmlelem         = pXmlparse->pHocXmlelem;
    struct XMLELEM     *pNewXmlelem;
    struct XMLELEM     *pLastSiblingXmlelem;
    struct XMLDATA     *pNewXmldata         = NULL;
    int                 newXmldataLen       = 0;
    int                 lastRC;
    int                 elemNameLen;
    int                 elemContentLen      = inputElemContentLen;
    int                 i;

    elemNameLen = strlen(pElemName);

    /******************************************************************/
    /* If pElemContent is specified and inputElemContentLen = 0,      */
    /*    then accept pElemContent string as is (with possible        */
    /*    trailing blanks).                                           */
    /* Else if pElemContent is specified, and imputElemContentLen > 0 */
    /*    then strip trailing blanks from pElemContent.               */
    /******************************************************************/
    if (pElemContent != NULL)
    {
        if (elemContentLen == 0)
        {
            elemContentLen = strlen(pElemContent);
        }
        else
        {
            for (i = elemContentLen;
                i > 0;
                i--)
            {
                if ((pElemContent[(i - 1)] != ' ') &&
                    (pElemContent[(i - 1)] != 0))
                {
                    break;
                }
            }

            elemContentLen = i;
        }
    }

    /******************************************************************/
    /* Calculate XMLDATA length required.                             */
    /******************************************************************/
    newXmldataLen = (sizeof(struct XMLDATA)) +
                    elemNameLen +
                    elemContentLen;

    /******************************************************************/
    /* If no parent XMLELEM was specified, assume that we want to     */
    /* add the root XMLELEM.                                          */
    /******************************************************************/
    if (pParentXmlelem == NULL)
    {
        if ((pCurrXmlelem != NULL) ||
            (pHocXmlelem != NULL))
        {
            TRMSGI(TRCI_ERROR,
                   "Trying to add multiple root XMLELEMs; " 
                   "pCurrXmlelem=%08X, pHocXmlelem=%08X, " 
                   "pParentXmlelem=%08X\n",
                   pCurrXmlelem,
                   pHocXmlelem,
                   pParentXmlelem);

            return RC_FAILURE;
        }

        getElemStorage(pXmlparse,
                       &pNewXmlelem,
                       &pNewXmldata,
                       newXmldataLen);

        pXmlparse->pHocXmlelem = pNewXmlelem;
        pXmlparse->pCurrXmlelem = pNewXmlelem;
        pNewXmlelem->level = 1;
    }
    /******************************************************************/
    /* If a parent XMLELEM was specified, verify that it is           */
    /* present in the data hierarchy.                                 */
    /******************************************************************/
    else
    {
        XML_VERIFY_XMLELEM_IN_XMLPARSE(lastRC,
                                       pParentXmlelem,
                                       pXmlparse);

        if (lastRC != RC_SUCCESS)
        {
            TRMSGI(TRCI_ERROR,
                   "Parent XMLELEM=%08X does not exist in XMLPARSE\n",
                   pParentXmlelem);

            return RC_FAILURE;
        }

        XML_FIND_LAST_SIBLING_IN_XMLELEM(pParentXmlelem,
                                         pLastSiblingXmlelem);

        getElemStorage(pXmlparse,
                       &pNewXmlelem,
                       &pNewXmldata,
                       newXmldataLen);

        pNewXmlelem->level = pParentXmlelem->level + 1;
        pNewXmlelem->pParentXmlelem = pParentXmlelem;

        if (pLastSiblingXmlelem != NULL)
        {
            pNewXmlelem->pOlderXmlelem = pLastSiblingXmlelem;
            pLastSiblingXmlelem->pYoungerXmlelem = pNewXmlelem;
        }
        else
        {
            pParentXmlelem->pChildXmlelem = pNewXmlelem;
        }

        pNewXmlelem->pParentXmlelem = pParentXmlelem;
        pXmlparse->pCurrXmlelem = pNewXmlelem;
    }

    /*****************************************************************/
    /* Link the XMLDATA contents to the XMLELEM.                     */
    /*****************************************************************/
    pNewXmldata->data1Len = elemNameLen;
    pNewXmlelem->startElemLen = elemNameLen;
    pNewXmlelem->endElemLen = elemNameLen;

    memcpy(&(pNewXmldata->data[0]),
           pElemName,
           elemNameLen);

    pNewXmlelem->pStartElemName = &(pNewXmldata->data[0]);
    pNewXmlelem->pEndElemName = &(pNewXmldata->data[0]);

    if (elemContentLen > 0)
    {
        pNewXmldata->data2Len = elemContentLen;
        pNewXmlelem->contentLen = elemContentLen;

        memcpy(&(pNewXmldata->data[elemNameLen]),
               pElemContent,
               elemContentLen);

        pNewXmlelem->pContent = &(pNewXmldata->data[elemNameLen]);
    }

    return RC_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: getElemStorage                                    */
/** Description:   Get storage for new XMLELEM and optional XMLDATA. */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "getElemStorage"

static void getElemStorage(struct XMLPARSE  *pXmlparse,
                           struct XMLELEM  **pNewXmlelemPtr,
                           struct XMLDATA  **pNewXmldataPtr,
                           int               newXmldataLen)
{
    struct XMLELEM     *pNewXmlelem;
    struct XMLDATA     *pNewXmldata;

    pNewXmlelem = (struct XMLELEM*) malloc(sizeof(struct XMLELEM));

    TRMSGI(TRCI_STORAGE,
           "malloc XMLELEM=%08X, len=%i\n",
           pNewXmlelem,
           sizeof(struct XMLELEM));

    /*****************************************************************/
    /* Initialize the new XMLELEM, and XMLDATA                       */
    /*****************************************************************/
    pXmlparse->starttagCount += 2;
    pXmlparse->endtagCount += 2;

    memset(pNewXmlelem, 0, sizeof(struct XMLELEM));

    memcpy(pNewXmlelem->type,
           "ELEM",
           sizeof(pNewXmlelem->type));

    pNewXmlelem->sequence = pXmlparse->starttagCount;
    pXmlparse->pCurrLeaf = pNewXmlelem;
    *pNewXmlelemPtr = pNewXmlelem;

    getDataStorage(pXmlparse,
                   pNewXmldataPtr,
                   newXmldataLen);

    pNewXmldata = *pNewXmldataPtr;
    pNewXmldata->flag = ELEM_DATA;

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_ADD_ATTR_TO_HIERARCHY                          */
/** Description:   Add an attribute to the data hierarchy tree.      */
/**                                                                  */
/** Add a new XMLATTR attribure=value pair element to the XMLPARSE   */
/** data hierarchy tree.  The new XMLATTR will be placed at the      */
/** end of the attribure chain for the specified parent element.     */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_ADD_ATTR_TO_HIERARCHY"

extern int FN_ADD_ATTR_TO_HIERARCHY(struct XMLPARSE *pXmlparse,
                                    struct XMLELEM  *pParentXmlelem,
                                    char            *pKeyword,
                                    char            *pValue,
                                    int              inputValueLen)
{
    struct XMLATTR     *pNewXmlattr;
    struct XMLATTR     *pLastXmlattr;
    struct XMLDATA     *pNewXmldata         = NULL;
    int                 newXmldataLen       = 0;
    int                 lastRC;
    int                 attrKeywordLen;
    int                 attrValueLen        = inputValueLen;
    int                 tokenAttrValueLen   = 0;
    int                 i;

    /******************************************************************/
    /* Verify that specified XMLELEM is in the data hierarchy.        */
    /******************************************************************/
    if ((pKeyword == NULL) ||
        (pValue == NULL) ||
        (pParentXmlelem == NULL))
    {
        TRMSGI(TRCI_ERROR,
               "pKeyword=%08X, pValue=%08X, pParentXmlelem=%08X\n",
               pKeyword,
               pValue,
               pParentXmlelem);

        return RC_FAILURE;
    }

    XML_VERIFY_XMLELEM_IN_XMLPARSE(lastRC,
                                   pParentXmlelem,
                                   pXmlparse);

    if (lastRC != RC_SUCCESS)
    {
        TRMSGI(TRCI_ERROR,
               "Parent XMLELEM=%08X does not exist in XMLPARSE\n",
               pParentXmlelem);

        return RC_FAILURE;
    }

    attrKeywordLen = strlen(pKeyword);

    /******************************************************************/
    /* If inputValueLen = 0,                                          */
    /*    then accept pValue string as is (with possible trailing     */
    /*    blanks).                                                    */
    /* Else if inputValueLen > 0,                                     */
    /*    then strip trailing blanks from pValue.                     */
    /******************************************************************/
    if (attrValueLen == 0)
    {
        attrValueLen = strlen(pValue);
    }
    else
    {
        for (i = attrValueLen;
            i > 0;
            i--)
        {
            if ((pValue[(i - 1)] != ' ') &&
                (pValue[(i - 1)] != 0))
            {
                break;
            }
        }

        attrValueLen = i;
    }


    if (attrValueLen == 0)
    {
        TRMSGI(TRCI_ERROR,
               "pValue was all blanks\n");

        return RC_FAILURE;
    }

    /******************************************************************/
    /* Calculate XMLDATA length required.                             */
    /******************************************************************/
    newXmldataLen = (sizeof(struct XMLDATA)) +
                    attrKeywordLen +
                    attrValueLen;

    getAttrStorage(pXmlparse,
                   &pNewXmlattr,
                   &pNewXmldata,
                   newXmldataLen);

    /*****************************************************************/
    /* Link the new XMLATTR to the parent XMLELEM.                   */
    /*****************************************************************/
    pNewXmlattr->pParentXmlelem = pParentXmlelem;
    pLastXmlattr = pParentXmlelem->pCurrXmlattr;

    if (pLastXmlattr == NULL)
    {
        pParentXmlelem->pHocXmlattr = pNewXmlattr;
        pParentXmlelem->pCurrXmlattr = pNewXmlattr;
    }
    else
    {
        pLastXmlattr->pNextXmlattr = pNewXmlattr;
        pParentXmlelem->pCurrXmlattr = pNewXmlattr;
    }

    /*****************************************************************/
    /* Link the XMLDATA contents to the XMLATTR.                     */
    /*****************************************************************/
    pNewXmldata->data1Len = attrKeywordLen;
    pNewXmlattr->keywordLen = attrKeywordLen;

    memcpy(&(pNewXmldata->data[0]),
           pKeyword,
           attrKeywordLen);

    pNewXmlattr->pKeyword = &(pNewXmldata->data[0]);
    pNewXmldata->data2Len = attrValueLen;
    pNewXmlattr->valueLen = attrValueLen;

    memcpy(&(pNewXmldata->data[attrKeywordLen]),
           pValue,
           attrValueLen);

    pNewXmlattr->pValue = &(pNewXmldata->data[attrKeywordLen]);
    pNewXmlattr->valueDelimiter = STR_APOST[0];

    TRMEMI(TRCI_XMLPARSER,
           pNewXmlattr, sizeof(struct XMLATTR), 
           "New XMLATTR:\n");

    TRMEMI(TRCI_XMLPARSER,
           pNewXmldata, pNewXmldata->xmldataLen, 
           "New XMLDATA:\n");

    TRMEMI(TRCI_XMLPARSER,
           pParentXmlelem, sizeof(struct XMLELEM), 
           "Parent XMLELEM:\n");

    return RC_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: getAttrStorage                                    */
/** Description:   Get storage for new XMLATTR and optional XMLDATA. */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "getAttrStorage"

static void getAttrStorage(struct XMLPARSE  *pXmlparse,
                           struct XMLATTR  **pNewXmlattrPtr,
                           struct XMLDATA  **pNewXmldataPtr,
                           int               newXmldataLen)
{
    struct XMLATTR     *pNewXmlattr;
    struct XMLDATA     *pNewXmldata;

    pNewXmlattr = (struct XMLATTR*) malloc(sizeof(struct XMLATTR));

    TRMSGI(TRCI_STORAGE,
           "malloc XMLATTR=%08X, len=%i\n",
           pNewXmlattr,
           sizeof(struct XMLATTR));

    /*****************************************************************/
    /* Initialize the new XMLATTR and XMLDATA.                       */
    /*****************************************************************/
    pXmlparse->starttagCount += 2;
    pXmlparse->endtagCount += 2;

    memset(pNewXmlattr, 0, sizeof(struct XMLATTR));

    memcpy(pNewXmlattr->type,
           "ATTR",
           sizeof(pNewXmlattr->type));

    pNewXmlattr->sequence = pXmlparse->starttagCount;
    pXmlparse->pCurrLeaf = pNewXmlattr;
    *pNewXmlattrPtr = pNewXmlattr;

    getDataStorage(pXmlparse,
                   pNewXmldataPtr,
                   newXmldataLen);

    pNewXmldata = *pNewXmldataPtr;
    pNewXmldata->flag = ATTR_DATA;

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: getDataStorage                                    */
/** Description:   Get XMLDATA storage.                              */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "getDataStorage"

static void getDataStorage(struct XMLPARSE  *pXmlparse,
                           struct XMLDATA  **pNewXmldataPtr,
                           int               newXmldataLen)
{
    struct XMLDATA     *pNewXmldata;

    pNewXmldata = (struct XMLDATA*) malloc(newXmldataLen);

    TRMSGI(TRCI_STORAGE,
           "malloc XMLDATA=%08X, len=%i\n",
           pNewXmldata,
           newXmldataLen);

    /*****************************************************************/
    /* Initialize and link the XMLDATA                               */
    /*****************************************************************/
    memset(pNewXmldata, 0, newXmldataLen);

    memcpy(pNewXmldata->type,
           "DATA",
           sizeof(pNewXmldata->type));

    pNewXmldata->xmldataLen = newXmldataLen;
    pNewXmldata->pNextXmldata = pXmlparse->pHocXmldata;

    pXmlparse->pHocXmldata = pNewXmldata;
    pXmlparse->pCurrXmldata = pNewXmldata;

    *pNewXmldataPtr = pNewXmldata;

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_GENERATE_XML_FROM_HIERARCHY                    */
/** Description:   Generate an XML "blob" string from a XMLPARSE.    */
/**                                                                  */
/** This is the reverse of FN_GENERATE_HIERARCHY_FROM_XML.           */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_GENERATE_XML_FROM_HIERARCHY"

extern struct XMLRAWIN *FN_GENERATE_XML_FROM_HIERARCHY(struct XMLPARSE *pXmlparse)
{
    struct XMLDECL     *pXmldecl;
    struct XMLELEM     *pXmlelem;
    struct XMLRAWIN    *pNewXmlrawin;
    int                 xmlrawinSize;
    int                 xmlBlobSize;
    int                 declSize            = 0;
    int                 elemSize            = 0;
    char               *pBufferPos;

    /*****************************************************************/
    /* Calculate size needed for XML declaration text.               */
    /*****************************************************************/
    pXmldecl = pXmlparse->pHocXmldecl;

    if (pXmldecl != NULL)
    {
        declSize = calcDeclSize(pXmldecl);
    }

    /*****************************************************************/
    /* Calculate size needed for XML elements and attributes.        */
    /*****************************************************************/
    pXmlelem = pXmlparse->pHocXmlelem;

    if (pXmlelem != NULL)
    {
        elemSize = calcElemSize(pXmlelem);
    }

    xmlBlobSize = declSize + elemSize;

    /*****************************************************************/
    /* Get a buffer for the XMLRAWIN (includes the XML blob buffer). */
    /*****************************************************************/
    xmlrawinSize = sizeof(struct XMLRAWIN) + xmlBlobSize;

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
    pNewXmlrawin->dataLen = xmlBlobSize;
    pNewXmlrawin->pNextXmlrawin = pXmlparse->pHocXmlrawin;

    pXmlparse->pHocXmlrawin = pNewXmlrawin;
    pXmlparse->pCurrXmlrawin = pNewXmlrawin;

    pBufferPos = &(pNewXmlrawin->pData[0]);

    /*****************************************************************/
    /* Create XML blob text from the XMLDECL structures.             */
    /*****************************************************************/
    pXmldecl = pXmlparse->pHocXmldecl;

    if (pXmldecl != NULL)
    {
        buildDeclText(pXmldecl,
                      &pBufferPos);
    }

    /*****************************************************************/
    /* Create XML blob text from the XMLELEM structures.             */
    /*****************************************************************/
    pXmlelem = pXmlparse->pHocXmlelem;

    if (pXmlelem != NULL)
    {
        buildElemText(pXmlelem,
                      &pBufferPos);
    }

    return pNewXmlrawin;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: calcDeclSize                                      */
/** Description:   Calculate space needed for the XML declarations.  */
/**                                                                  */
/** The return code indicates the required size.                     */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "calcDeclSize"

static int calcDeclSize(struct XMLDECL *pXmldecl)
{
    int                 declSize            = 0;

    while (pXmldecl != NULL)
    {
        /*************************************************************/
        /* Size needed is (size of declaration) +                    */
        /*                (size of declaration start and end tags).  */
        /*************************************************************/
        declSize += (pXmldecl->declLen + SIZEOF_STR_DECL_STARTTAG + SIZEOF_STR_DECL_ENDTAG);
        pXmldecl = pXmldecl->pNextXmldecl;
    }

    return declSize;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: calcElemSize                                      */
/** Description:   Calculate space needed for the XML elements.      */
/**                                                                  */
/** The return code indicates the required size.                     */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "calcElemSize"

static int calcElemSize(struct XMLELEM *pXmlelem)
{
    struct XMLATTR     *pXmlattr;
    char               *pTranslatedContent;
    int                 elemSize            = 0;
    int                 attrSize;
    int                 i;
    int                 j;
    int                 childElemSize;
    int                 specialCharAddChars = 0;

    while (pXmlelem != NULL)
    {
        /*************************************************************/
        /* Size needed for element (without attributes) is:          */
        /* (2 * (size of element name))  +                           */
        /* (size of element name head start and end tags) +          */
        /* (size of element name tail start and end tags) +          */
        /* (size of element content).                                */
        /*************************************************************/
        elemSize += ((2 * (pXmlelem->startElemLen)) +
                     (SIZEOF_STR_STARTTAG + SIZEOF_STR_ENDTAG) +
                     (SIZEOF_STR_ELEMEND_STARTTAG + SIZEOF_STR_ENDTAG) +
                     (pXmlelem->contentLen));

        /*************************************************************/
        /* Check for special characters in the text requiring        */
        /* translation.  Count them in the total element length      */
        /* needed in the XML blob.                                   */
        /*************************************************************/
        for (i = 0;
            i < pXmlelem->contentLen;
            i++)
        {
            if (memcmp(&(pXmlelem->pContent[i]),
                       "'",
                       1) == 0)
            {
                specialCharAddChars += (SIZEOF_STR_EQU_APOST - 1);
            }
            else if (pXmlelem->pContent[i] == '"')
            {
                specialCharAddChars += (SIZEOF_STR_EQU_QUOTE - 1);
            }
            else if (pXmlelem->pContent[i] == '&')
            {
                specialCharAddChars += (SIZEOF_STR_EQU_AMPER - 1);
            }
            else if (pXmlelem->pContent[i] == '>')
            {
                specialCharAddChars += (SIZEOF_STR_EQU_GREATTHAN - 1);
            }
            else if (pXmlelem->pContent[i] == '<')
            {
                specialCharAddChars += (SIZEOF_STR_EQU_LESSTHAN - 1);
            }
        }

        /*************************************************************/
        /* If we found special characters, acquire a new buffer for  */
        /* the element content, and translate the characters to      */
        /* their XML & equivalents.                                  */
        /*************************************************************/
        if (specialCharAddChars > 0)
        {
            TRMSGI(TRCI_XMLPARSER,
                   "Increasing element length by %i\n",
                   specialCharAddChars);

            elemSize += specialCharAddChars;
            pXmlelem->translatedLen = pXmlelem->contentLen + specialCharAddChars;

            pTranslatedContent = malloc(pXmlelem->translatedLen);

            TRMSGI(TRCI_STORAGE,
                   "malloc pTranslatedContent=%08X, len=%i\n",
                   pTranslatedContent,
                   pXmlelem->translatedLen);

            memset(pTranslatedContent, 0, pXmlelem->translatedLen);

            pXmlelem->pTranslatedContent = pTranslatedContent;

            for (i = 0, j = 0;
                i < pXmlelem->contentLen;
                i++)
            {
                if (memcmp(&(pXmlelem->pContent[i]),
                           "'",
                           1) == 0)
                {
                    memcpy(&(pTranslatedContent[j]),
                           STR_EQU_APOST,
                           SIZEOF_STR_EQU_APOST);

                    j += SIZEOF_STR_EQU_APOST;
                }
                else if (pXmlelem->pContent[i] == '"')
                {
                    memcpy(&(pTranslatedContent[j]),
                           STR_EQU_QUOTE,
                           SIZEOF_STR_EQU_QUOTE);

                    j += SIZEOF_STR_EQU_QUOTE;
                }
                else if (pXmlelem->pContent[i] == '&')
                {
                    memcpy(&(pTranslatedContent[j]),
                           STR_EQU_AMPER,
                           SIZEOF_STR_EQU_AMPER);

                    j += SIZEOF_STR_EQU_AMPER;
                }
                else if (pXmlelem->pContent[i] == '>')
                {
                    memcpy(&(pTranslatedContent[j]),
                           STR_EQU_GREATTHAN,
                           SIZEOF_STR_EQU_GREATTHAN);

                    j += SIZEOF_STR_EQU_GREATTHAN;
                }
                else if (pXmlelem->pContent[i] == '<')
                {
                    memcpy(&(pTranslatedContent[j]),
                           STR_EQU_LESSTHAN,
                           SIZEOF_STR_EQU_LESSTHAN);

                    j += SIZEOF_STR_EQU_LESSTHAN;
                }
                else
                {
                    pTranslatedContent[j] = pXmlelem->pContent[i];
                    j++;
                }
            }

            TRMEMI(TRCI_XMLPARSER,
                   pTranslatedContent, pXmlelem->translatedLen,
                   "Translated XML after special chars converted:\n");

            specialCharAddChars = 0;
        }

        /*************************************************************/
        /* If there are attributes, then add their length also.      */
        /*************************************************************/
        pXmlattr = pXmlelem->pHocXmlattr;
        attrSize = 0;

        while (pXmlattr != NULL)
        {
            /*********************************************************/
            /* Size needed for attribute is:                         */
            /* (size of blank delimiter) +                           */
            /* (size of equalsign delimiter) +                       */
            /* (2 * size of quote or double quote delimiter) +       */
            /* (size of keyword + size of value)                     */
            /*********************************************************/
            attrSize += ((SIZEOF_STR_BLANK) +
                         (SIZEOF_STR_EQUALSIGN) +
                         (2 * SIZEOF_STR_QUOTE) +
                         (pXmlattr->keywordLen + pXmlattr->valueLen));
            pXmlattr = pXmlattr->pNextXmlattr;
        }

        elemSize += attrSize;

        if (pXmlelem->pChildXmlelem != NULL)
        {
            childElemSize = calcElemSize(pXmlelem->pChildXmlelem);

            elemSize += childElemSize;
        }

        pXmlelem = pXmlelem->pYoungerXmlelem;
    }

    return elemSize;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: buildDeclText                                     */
/** Description:   Build declarations from XMLDECL structures.       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "buildDeclText"

static void buildDeclText(struct XMLDECL  *pXmldecl,
                          char           **pBufferPosPtr)
{
    while (pXmldecl != NULL)
    {
        memcpy(*pBufferPosPtr,
               STR_DECL_STARTTAG,
               SIZEOF_STR_DECL_STARTTAG);

        *pBufferPosPtr += SIZEOF_STR_DECL_STARTTAG;

        memcpy(*pBufferPosPtr,
               pXmldecl->pDeclaration,
               pXmldecl->declLen);

        *pBufferPosPtr += pXmldecl->declLen;

        memcpy(*pBufferPosPtr,
               STR_DECL_ENDTAG,
               SIZEOF_STR_DECL_ENDTAG);

        *pBufferPosPtr += SIZEOF_STR_DECL_ENDTAG;
        pXmldecl = pXmldecl->pNextXmldecl;
    }

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: buildElemText                                     */
/** Description:   Build XML "blob" string element.                  */
/**                                                                  */
/** Build XML "blob" string elements and attributes from XMLPARSE    */
/** data hierarchy tree XMLELEM and XMLATTR structures.              */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "buildElemText"

static void buildElemText(struct XMLELEM  *pXmlelem,
                          char           **pBufferPosPtr)
{
    struct XMLATTR     *pXmlattr;
    int                 xmlattrCount;

    while (pXmlelem != NULL)
    {
        memcpy(*pBufferPosPtr,
               STR_STARTTAG,
               SIZEOF_STR_STARTTAG);

        *pBufferPosPtr += SIZEOF_STR_STARTTAG;

        memcpy(*pBufferPosPtr,
               pXmlelem->pStartElemName,
               pXmlelem->startElemLen);

        *pBufferPosPtr += pXmlelem->startElemLen;

        /*************************************************************/
        /* Attributes may be placed in either the element name head  */
        /* or element name tail structure.  We'll put them in the    */
        /* head.                                                     */
        /*************************************************************/
        pXmlattr = pXmlelem->pHocXmlattr;
        xmlattrCount = 0;

        while (pXmlattr != NULL)
        {
            if (xmlattrCount == 0)
            {
                memcpy(*pBufferPosPtr,
                       STR_BLANK,
                       SIZEOF_STR_BLANK);

                *pBufferPosPtr += SIZEOF_STR_BLANK;
            }
            else
            {
                memcpy(*pBufferPosPtr,
                       STR_COMMA,
                       SIZEOF_STR_COMMA);

                *pBufferPosPtr += SIZEOF_STR_COMMA;
            }

            xmlattrCount++;

            memcpy(*pBufferPosPtr,
                   pXmlattr->pKeyword,
                   pXmlattr->keywordLen);

            *pBufferPosPtr += pXmlattr->keywordLen;

            memcpy(*pBufferPosPtr,
                   STR_EQUALSIGN,
                   SIZEOF_STR_EQUALSIGN);

            *pBufferPosPtr += SIZEOF_STR_EQUALSIGN;

            memcpy(*pBufferPosPtr,
                   (char*) &(pXmlattr->valueDelimiter),
                   sizeof(pXmlattr->valueDelimiter));

            *pBufferPosPtr += (sizeof(pXmlattr->valueDelimiter));

            memcpy(*pBufferPosPtr,
                   pXmlattr->pValue,
                   pXmlattr->valueLen);

            *pBufferPosPtr += pXmlattr->valueLen;

            memcpy(*pBufferPosPtr,
                   (char*) &(pXmlattr->valueDelimiter),
                   sizeof(pXmlattr->valueDelimiter));

            *pBufferPosPtr += (sizeof(pXmlattr->valueDelimiter));

            pXmlattr = pXmlattr->pNextXmlattr;
        }

        memcpy(*pBufferPosPtr,
               STR_ENDTAG,
               SIZEOF_STR_ENDTAG);

        *pBufferPosPtr += SIZEOF_STR_ENDTAG;

        if (pXmlelem->translatedLen > 0)
        {
            memcpy(*pBufferPosPtr,
                   pXmlelem->pTranslatedContent,
                   pXmlelem->translatedLen);

            *pBufferPosPtr += pXmlelem->translatedLen;
        }
        else
        {
            memcpy(*pBufferPosPtr,
                   pXmlelem->pContent,
                   pXmlelem->contentLen);

            *pBufferPosPtr += pXmlelem->contentLen;
        }

        if (pXmlelem->pChildXmlelem != NULL)
        {
            buildElemText(pXmlelem->pChildXmlelem,
                          pBufferPosPtr);
        }

        memcpy(*pBufferPosPtr,
               STR_ELEMEND_STARTTAG,
               SIZEOF_STR_ELEMEND_STARTTAG);

        *pBufferPosPtr += SIZEOF_STR_ELEMEND_STARTTAG;

        memcpy(*pBufferPosPtr,
               pXmlelem->pStartElemName,
               pXmlelem->startElemLen);

        *pBufferPosPtr += pXmlelem->startElemLen;

        memcpy(*pBufferPosPtr,
               STR_ENDTAG,
               SIZEOF_STR_ENDTAG);

        *pBufferPosPtr += SIZEOF_STR_ENDTAG;

        pXmlelem = pXmlelem->pYoungerXmlelem;
    }

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_FREE_HIERARCHY_STORAGE                         */
/** Description:   Free XMLPARSE data hierarchy tree storage.        */
/**                                                                  */
/** Free all of the storage associated with an XMLPARSE data         */
/** hierarchy tree, and then free the input XMLPARSE structure       */
/** as well.                                                         */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_FREE_HIERARCHY_STORAGE"

extern void FN_FREE_HIERARCHY_STORAGE(struct XMLPARSE *pXmlparse)
{
    if (pXmlparse == NULL)
    {
        TRMSGI(TRCI_STORAGE,
               "No XMLPARSE to free\n");

        return;
    }

    if (pXmlparse->pHocXmlrawin != NULL)
    {
        freeXmlrawin(pXmlparse->pHocXmlrawin);
    }

    if (pXmlparse->pHocXmlcnvin != NULL)
    {
        freeXmlcnvin(pXmlparse->pHocXmlcnvin);
    }

    if (pXmlparse->pHocXmldecl != NULL)
    {
        freeXmldecl(pXmlparse->pHocXmldecl);
    }

    if (pXmlparse->pHocXmlcomm != NULL)
    {
        freeXmlcomm(pXmlparse->pHocXmlcomm);
    }

    if (pXmlparse->pHocXmlelem != NULL)
    {
        freeXmlelem(pXmlparse->pHocXmlelem);
    }

    if (pXmlparse->pHocXmldata != NULL)
    {
        freeXmldata(pXmlparse->pHocXmldata);
    }

    TRMSGI(TRCI_STORAGE,
           "free XMLPARSE=%08X, len=%i\n",
           pXmlparse,
           sizeof(struct XMLPARSE));

    free(pXmlparse);

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: freeXmlrawin                                      */
/** Description:   Free the the XMLRAWIN elements.                   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "freeXmlrawin"

static void freeXmlrawin(struct XMLRAWIN *pInputXmlrawin)
{
    struct XMLRAWIN    *pXmlrawin           = pInputXmlrawin;
    struct XMLRAWIN    *pDeleteXmlrawin;

    while (pXmlrawin != NULL)
    {
        pDeleteXmlrawin = pXmlrawin;
        pXmlrawin = pXmlrawin->pNextXmlrawin;

        TRMSGI(TRCI_STORAGE,
               "free XMLRAWIN=%08X, len=%i\n",
               pDeleteXmlrawin,
               pDeleteXmlrawin->xmlrawinSize);

        free(pDeleteXmlrawin);
    }

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: freeXmlcnvin                                      */
/** Description:   Free the the XMLCNVIN elements.                   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "freeXmlcnvin"

static void freeXmlcnvin(struct XMLCNVIN *pInputXmlcnvin)
{
    struct XMLCNVIN    *pXmlcnvin           = pInputXmlcnvin;
    struct XMLCNVIN    *pDeleteXmlcnvin;

    while (pXmlcnvin != NULL)
    {
        pDeleteXmlcnvin = pXmlcnvin;
        pXmlcnvin = pXmlcnvin->pNextXmlcnvin;

        TRMSGI(TRCI_STORAGE,
               "free XMLCNVIN=%08X, len=%i\n",
               pDeleteXmlcnvin,
               pDeleteXmlcnvin->xmlcnvinSize);

        free(pDeleteXmlcnvin);
    }

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: freeXmldecl                                       */
/** Description:   Free the the XMLDECL elements.                    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "freeXmldecl"

static void freeXmldecl(struct XMLDECL *pInputXmldecl)
{
    struct XMLDECL     *pXmldecl            = pInputXmldecl;
    struct XMLDECL     *pDeleteXmldecl;

    while (pXmldecl != NULL)
    {
        pDeleteXmldecl = pXmldecl;
        pXmldecl = pXmldecl->pNextXmldecl;

        TRMSGI(TRCI_STORAGE,
               "free XMLDECL=%08X, len=%i\n",
               pDeleteXmldecl,
               sizeof(struct XMLDECL));

        free(pDeleteXmldecl);
    }

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: freeXmlcomm                                       */
/** Description:   Free the the XMLCOMM elements.                    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "freeXmlcomm"

static void freeXmlcomm(struct XMLCOMM *pInputXmlcomm)
{
    struct XMLCOMM     *pXmlcomm            = pInputXmlcomm;
    struct XMLCOMM     *pDeleteXmlcomm;

    while (pXmlcomm != NULL)
    {
        pDeleteXmlcomm = pXmlcomm;
        pXmlcomm = pXmlcomm->pNextXmlcomm;

        TRMSGI(TRCI_STORAGE,
               "free XMLCOMM=%08X, len=%i\n",
               pDeleteXmlcomm,
               sizeof(struct XMLCOMM));

        free(pDeleteXmlcomm);
    }

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: freeXmlelem                                       */
/** Description:   Free the XMLELEM, XMLATTR, and content elements.  */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "freeXmlelem"

static void freeXmlelem(struct XMLELEM *pInputXmlelem)
{
    struct XMLELEM     *pXmlelem            = pInputXmlelem;
    struct XMLELEM     *pDeleteXmlelem;
    struct XMLELEM     *pChildXmlelem;
    struct XMLATTR     *pXmlattr;
    struct XMLATTR     *pDeleteXmlattr;

    while (pXmlelem != NULL)
    {
        pXmlattr = pXmlelem->pHocXmlattr;
        pChildXmlelem = pXmlelem->pChildXmlelem;
        pDeleteXmlelem = pXmlelem;

        while (pXmlattr != NULL)
        {
            pDeleteXmlattr = pXmlattr;
            pXmlattr = pXmlattr->pNextXmlattr;

            TRMSGI(TRCI_STORAGE,
                   "free XMLATTR=%08X, len=%i\n",
                   pDeleteXmlattr, 
                   sizeof(struct XMLATTR));

            free(pDeleteXmlattr);
        }

        if (pXmlelem->pTranslatedContent != NULL)
        {
            TRMSGI(TRCI_STORAGE,
                   "free pTranslatedContent=%08X, len=%i\n",
                   pXmlelem->pTranslatedContent,
                   pXmlelem->translatedLen);

            free(pXmlelem->pTranslatedContent);
        }

        if (pChildXmlelem != NULL)
        {
            freeXmlelem(pChildXmlelem);
        }

        pXmlelem = pXmlelem->pYoungerXmlelem;

        TRMSGI(TRCI_STORAGE,
               "free XMLELEM=%08X, len=%i\n",
               pDeleteXmlelem,
               sizeof(struct XMLELEM));

        free(pDeleteXmlelem);
    }

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: freeXmldata                                       */
/** Description:   Free the the XMLDATA elements.                    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "freeXmldata"

static void freeXmldata(struct XMLDATA *pInputXmldata)
{
    struct XMLDATA     *pXmldata            = pInputXmldata;
    struct XMLDATA     *pDeleteXmldata;

    while (pXmldata != NULL)
    {
        pDeleteXmldata = pXmldata;
        pXmldata = pXmldata->pNextXmldata;

        TRMSGI(TRCI_STORAGE,
               "free XMLDATA=%08X, len=%i\n",
               pDeleteXmldata,
               pDeleteXmldata->xmldataLen);

        free(pDeleteXmldata);
    }

    return;
}


