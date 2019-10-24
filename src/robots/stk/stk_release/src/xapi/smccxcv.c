/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      smccxcv.c                                        */
/** Description:    XAPI client XML parser character conversion      */
/**                 service.                                         */
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
/** Function Name: FN_XML_ASCII_TO_ASCII                             */
/** Description:   Convert an ASCII XMLRAWIN to an ASCII XMLCNVIN.   */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF 
#define SELF "FN_XML_ASCII_TO_ASCII"

extern struct XMLCNVIN *FN_XML_ASCII_TO_ASCII(struct XMLPARSE *pXmlparse,
                                              char             checkResultsFlag)
{
    struct XMLRAWIN    *pXmlrawin           = pXmlparse->pCurrXmlrawin;
    struct XMLCNVIN    *pNewXmlcnvin;
    int                 i;
    int                 xmlcnvinSize        = (sizeof(struct XMLCNVIN) 
                                               + pXmlrawin->dataLen);

    pNewXmlcnvin = (struct XMLCNVIN*) malloc(xmlcnvinSize);

    TRMSGI(TRCI_STORAGE,
           "malloc XMLCNVIN=%08X, len=%i\n",
           pNewXmlcnvin,
           xmlcnvinSize);

    memset(pNewXmlcnvin, 0, xmlcnvinSize);
    pNewXmlcnvin->xmlcnvinSize = xmlcnvinSize;
    pNewXmlcnvin->dataLen = pXmlrawin->dataLen;

    memcpy(pNewXmlcnvin->pData,
           pXmlrawin->pData,
           pXmlrawin->dataLen);

    /*****************************************************************/
    /* If checking the results of the conversion:                    */
    /* Convert any bad characters to '?' and count them.             */
    /*****************************************************************/
    if (checkResultsFlag)
    {
        for (i = 0; 
            i < pNewXmlcnvin->dataLen; 
            i++)
        {
            if (((pNewXmlcnvin->pData[i] >= '\x20') &&
                 (pNewXmlcnvin->pData[i] <= '\x7E')) ||
                (pNewXmlcnvin->pData[i] == '\x0A') ||
                (pNewXmlcnvin->pData[i] == '\x0D'))
            {
                DONOTHING;
            }
            else
            {
                pNewXmlcnvin->nonTextCount++;
                pNewXmlcnvin->pData[i] = '?';
            }
        }

        if (pNewXmlcnvin->nonTextCount > 0)
        {
            TRMEMI(TRCI_XMLPARSER,
                   pNewXmlcnvin, pNewXmlcnvin->xmlcnvinSize,
                   "ASCII_TO_ASCII found %i "
                   "non-text characters; XMLCNVIN:\n",
                   pNewXmlcnvin->nonTextCount);
        }
    }

    return pNewXmlcnvin;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: FN_XML_EBCDIC_TO_ASCII                            */
/** Description:   Convert an EBCDIC XMLRAWIN to an ASCII XMLCNVIN.  */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF 
#define SELF "FN_XML_EBCDIC_TO_ASCII"

extern struct XMLCNVIN *FN_XML_EBCDIC_TO_ASCII(struct XMLPARSE *pXmlparse,
                                               char             checkResultsFlag)
{
    struct XMLRAWIN    *pXmlrawin           = pXmlparse->pCurrXmlrawin;
    struct XMLCNVIN    *pNewXmlcnvin;
    int                 i;
    int                 xmlcnvinSize        = (sizeof(struct XMLCNVIN) 
                                               + pXmlrawin->dataLen);
    pNewXmlcnvin = (struct XMLCNVIN*) malloc(xmlcnvinSize);

    TRMSGI(TRCI_STORAGE,
           "malloc XMLCNVIN=%08X, len=%i\n",
           pNewXmlcnvin,
           xmlcnvinSize);

    memset(pNewXmlcnvin, 0, xmlcnvinSize);
    pNewXmlcnvin->xmlcnvinSize = xmlcnvinSize;
    pNewXmlcnvin->dataLen = pXmlrawin->dataLen;

    memcpy(pNewXmlcnvin->pData,
           pXmlrawin->pData,
           pXmlrawin->dataLen);

    FN_CONVERT_EBCDIC_TO_ASCII(pNewXmlcnvin->pData, 
                               pXmlrawin->dataLen);

    /*****************************************************************/
    /* If checking the results of the conversion:                    */
    /* Convert any bad characters to '?' and count them.             */
    /*****************************************************************/
    if (checkResultsFlag)
    {
        for (i = 0; 
            i < pNewXmlcnvin->dataLen; 
            i++)
        {
            if ((pNewXmlcnvin->pData[i] >= ' ') ||
                (pNewXmlcnvin->pData[i] == '\x15'))
            {
                DONOTHING;
            }
            else
            {
                pNewXmlcnvin->nonTextCount++;
                pNewXmlcnvin->pData[i] = '?';
            }
        }

        if (pNewXmlcnvin->nonTextCount > 0)
        {
            if (pNewXmlcnvin->nonTextCount > 0)
            {
                TRMEMI(TRCI_XMLPARSER,
                       pNewXmlcnvin, pNewXmlcnvin->xmlcnvinSize,
                       "ASCII_TO_EBCDIC found %i "
                       "non-text characters; XMLCNVIN:\n",
                       pNewXmlcnvin->nonTextCount);
            }
        }
    }

    return pNewXmlcnvin;
}


