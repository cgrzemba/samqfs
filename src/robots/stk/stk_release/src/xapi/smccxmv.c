/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011 Oracle and/or its affiliates.           */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      smccxmv.c                                        */
/** Description:    XAPI client XML parser element content           */
/**                 data mover service.                              */
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
static void findMatchingChildByXmlstruct(struct XMLPARSE  *pXmlparse,
                                         struct XMLELEM   *pParentXmlelem,
                                         struct XMLSTRUCT *pXmlstruct);

static void moveXmlFieldToStruct(int   fieldLen,
                                 char *pField,
                                 char  fillFlag, 
                                 char  bitValue,
                                 char *pXmlContent,
                                 int   xmlContentLen);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_MOVE_XML_ELEMS_TO_STRUCT                       */
/** Description:   Extract content from XMLPARSE data hierarchy.     */
/**                                                                  */
/** Based on an input table of XMLSTRUCT(s), move element content    */
/** from an XMLPARSE data hierarchy tree to the specified            */
/** output fields.                                                   */
/**                                                                  */
/** The input XMLSTRUCT(s) provide the "rules" for moving the data   */
/** specifying the XMLELEM tags names, output field names and        */
/** length, and simple formatting options.                           */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_MOVE_XML_ELEMS_TO_STRUCT"

extern void FN_MOVE_XML_ELEMS_TO_STRUCT(struct XMLPARSE  *pXmlparse,
                                        struct XMLSTRUCT *pStartXmlstruct,
                                        int               xmlstructEntries)
{
    struct XMLSTRUCT   *pXmlstruct;
    struct XMLSTRUCT   *pFirstParentXmlstruct;
    struct XMLELEM     *pParentXmlelem;
    int                 i;
    int                 xmlstructCount      = 0;
    char                parentName[33]      = "?";

    for (i = 0, pXmlstruct = pStartXmlstruct;
        i < xmlstructEntries;
        i++, pXmlstruct++)
    {
        /*************************************************************/
        /* On break in parent element name, if this is not the first */
        /* time through (xmlstructCount = 0) pass first XMLSTRUCT    */
        /* entry matching the parent name plus the count of entries  */
        /* with the same parent name to the subroutine that will     */
        /* format the structure(s).                                  */
        /*************************************************************/
        if (strcmp(pXmlstruct->parentName, parentName) != 0)
        {
            if (xmlstructCount > 0)
            {
                FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                                  pParentXmlelem,
                                                  pFirstParentXmlstruct,
                                                  xmlstructCount);
            }

            /*************************************************************/
            /* On any break in parent element name, find the matching    */
            /* parent XMLELEM.                                           */
            /*************************************************************/
            strcpy(parentName, pXmlstruct->parentName);

            pParentXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                     NULL,
                                                     parentName);

            pFirstParentXmlstruct = pXmlstruct;
            xmlstructCount = 1;
        }
        else
        {
            xmlstructCount++;
        }
    }

    FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(pXmlparse,
                                      pParentXmlelem,
                                      pFirstParentXmlstruct,
                                      xmlstructCount);

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_MOVE_XML_ATTRS_TO_STRUCT                       */
/** Description:   Extract attributes from XMLPARSE data hierarchy.  */
/**                                                                  */
/** Based on an input table of XMLSTRUCT(s), move attribute values   */
/** from an XMLPARSE data hierarchy tree to the specified            */
/** output fields.                                                   */
/**                                                                  */
/** The input XMLSTRUCT(s) provide the "rules" for moving the data   */
/** specifying the XMLELEM tags names, output field names and        */
/** length, and simple formatting options.                           */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_MOVE_XML_ATTRS_TO_STRUCT"

extern void FN_MOVE_XML_ATTRS_TO_STRUCT(struct XMLPARSE  *pXmlparse,
                                        struct XMLELEM   *pXmlelem,
                                        struct XMLSTRUCT *pStartXmlstruct,
                                        int               xmlstructEntries)
{
    struct XMLSTRUCT   *pXmlstruct;
    struct XMLATTR     *pXmlattr;
    int                 i;

    for (i = 0, pXmlstruct = pStartXmlstruct;
        i < xmlstructEntries;
        i++, pXmlstruct++)
    {
        pXmlattr = FN_FIND_ATTRIBUTE_BY_KEYWORD(pXmlparse,
                                                pXmlelem,
                                                pXmlstruct->elemName);

        if (pXmlattr != NULL)
        {
            moveXmlFieldToStruct(pXmlstruct->fieldLen,
                                 pXmlstruct->pField,
                                 pXmlstruct->fillFlag,
                                 pXmlstruct->bitValue,
                                 pXmlattr->pValue,
                                 pXmlattr->valueLen);
        }
    }

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT                 */
/** Description:   Extract content from XMLPARSE data hierarchy.     */
/**                                                                  */
/** Based on an input table of XMLSTRUCT(s), move element content    */
/** from an XMLPARSE data hierarchy tree to the specified            */
/** output fields.                                                   */
/**                                                                  */
/** The input XMLSTRUCT(s) provide the "rules" for moving the data   */
/** specifying the XMLELEM tags names, output field names and        */
/** length, and simple formatting options.                           */
/**                                                                  */
/** This is essentially the same as FN_MOVE_XML_ELEMS_TO_STRUCT      */
/** except that the input pParentXmlelem limits the scan of the      */
/** XMLPARSE data hierarchy tree to either its immediate children    */
/** or any of its child's descendants.                               */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT"

extern void FN_MOVE_CHILD_XML_ELEMS_TO_STRUCT(struct XMLPARSE  *pXmlparse,
                                              struct XMLELEM   *pParentXmlelem,
                                              struct XMLSTRUCT *pStartXmlstruct,
                                              int               xmlstructCount)
{
    int                 i;
    struct XMLSTRUCT   *pXmlstruct;
    struct XMLELEM     *pNewParentXmlelem   = NULL;
    char                parentName[33]      = ""; 

    if (pParentXmlelem == NULL)
    {
        TRMSGI(TRCI_XMLPARSER,
               "Input pParentXmlelem=NULL\n");

        return;
    }
    else
    {
        memset(parentName, 0, sizeof(parentName));

        memcpy(parentName,
               pParentXmlelem->pStartElemName,
               pParentXmlelem->startElemLen);

        TRMSGI(TRCI_XMLPARSER,
               "pParentXmlelem name=%s\n",
               parentName);
    }

    for (i = 0, pXmlstruct = pStartXmlstruct;
        i < xmlstructCount;
        i++, pXmlstruct++)
    {
        /*************************************************************/
        /* If the XMLSTRUCT.parentName is specified and is NOT the   */
        /* same name as the input pParentXmlelem name, then find     */
        /* the 1st instance of the XMLSTRUCT.parentName under the    */
        /* current pParentXmlelem name and begin the search there;   */
        /* i.e. search the XMLPARSE sub-tree beginning with          */
        /* pParentXmlelem for the 1st matching XMLSTRUCT.parentName  */
        /* to move its children.   This can effectively expand the   */
        /* scope of the search+move from only the immediate          */
        /* children of pParentXmlelem (when XMLSTRUCT.parentName     */
        /* is specified as "", or NULL, or the same name as          */
        /* pParentXmlelem) to any descendant of pParentXmlelem.      */
        /*************************************************************/
        if ((pXmlstruct->parentName != NULL) &&
            (pXmlstruct->parentName[0] > ' ') &&
            (strcmp(pXmlstruct->parentName, parentName) != 0))
        {
            TRMSGI(TRCI_XMLPARSER,
                   "Expanding scope; parentName=%s, "
                   "pNewParentXmlelem name=s\n",
                   parentName,
                   pXmlstruct->parentName);

            pNewParentXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                        pParentXmlelem,
                                                        pXmlstruct->parentName);

            if (pNewParentXmlelem == NULL)
            {
                TRMSGI(TRCI_XMLPARSER,
                       "Could not find pNewParentXmlelem name=%s\n",
                       pXmlstruct->parentName);

                continue;
            }
            else
            {
                findMatchingChildByXmlstruct(pXmlparse,
                                             pNewParentXmlelem,
                                             pXmlstruct);
            }
        }
        /*************************************************************/
        /* Otherwise, if the XMLSTRUCT.parentName is NOT specified   */
        /* or is the same name as the input pParentXmlelem name,     */
        /* then limit the scope of the search and move to the        */
        /* immediate children of the pParentXmlelem.                 */
        /*************************************************************/
        else
        {
            TRMSGI(TRCI_XMLPARSER,
                   "Limiting scope; parentName=%s\n",
                   parentName);

            findMatchingChildByXmlstruct(pXmlparse,
                                         pParentXmlelem,
                                         pXmlstruct);
        }
    }

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: findMatchingChildByXmlstruct                      */
/** Description:   Find a child XMLELEM by name and move its content.*/
/**                                                                  */
/** Find a child XMLELEM that matches the input XMLSTRUCT, and       */
/*  if found, move the XMLELEM data content as specified.            */                
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "findMatchingChildByXmlstruct"

static void findMatchingChildByXmlstruct(struct XMLPARSE  *pXmlparse,
                                         struct XMLELEM   *pParentXmlelem,
                                         struct XMLSTRUCT *pXmlstruct)
{
    struct XMLELEM     *pXmlelem            = NULL;

    do
    {
        pXmlelem = FN_FIND_NEXT_CHILD_ELEMENT(pXmlparse,
                                              pParentXmlelem,
                                              pXmlelem);

        if (pXmlelem != NULL)
        {
            if ((pXmlelem->startElemLen == strlen(pXmlstruct->elemName)) &&
                (memcmp(pXmlelem->pStartElemName,
                        pXmlstruct->elemName,
                        pXmlelem->startElemLen) == 0))
            {
                moveXmlFieldToStruct(pXmlstruct->fieldLen,
                                     pXmlstruct->pField,
                                     pXmlstruct->fillFlag,
                                     pXmlstruct->bitValue,
                                     pXmlelem->pContent,
                                     pXmlelem->contentLen);

                break;
            }
        }
    } while (pXmlelem != NULL);

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: moveXmlFieldToStruct                              */
/** Description:   Move data for an XML entity into a structure.     */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "moveXmlFieldToStruct"

static void moveXmlFieldToStruct(int   fieldLen,
                                 char *pField,
                                 char  fillFlag,
                                 char  bitValue,
                                 char *pXmlContent,
                                 int   xmlContentLen)
{
    int                 dataLen;
    int                 startPos            = 0;

    if ((fieldLen == 1) &&
        (bitValue != 0) &&
        (toupper(pXmlContent[0]) == 'Y'))
    {
        pField[0] |= bitValue;
    }
    else
    {
        if (xmlContentLen < fieldLen)
        {
            if (fillFlag & BLANKFILL)
            {
                memset(pField, ' ', fieldLen);
            }
            else if ((fillFlag & ZEROFILL) &&
                     (xmlContentLen > 0))
            {
                memset(pField, '0', fieldLen);
                startPos = fieldLen - xmlContentLen;
            }
            else if (fillFlag & ZEROFILLALL)
            {
                memset(pField, '0', fieldLen);
                startPos = fieldLen - xmlContentLen;
            }

            dataLen = xmlContentLen;
        }
        else if (xmlContentLen > fieldLen)
        {
            TRMEMI(TRCI_XMLPARSER, 
                   pXmlContent, xmlContentLen,
                   "specified fieldLen=%d too small for xmlContentLen=%d\n",
                   fieldLen,
                   xmlContentLen);

            dataLen = fieldLen;
        }
        else
        {
            dataLen = fieldLen;
        }

        if (xmlContentLen > 0)
        {
            memcpy(&(pField[startPos]),
                   pXmlContent,
                   dataLen);
        }
    }

    return;
}


