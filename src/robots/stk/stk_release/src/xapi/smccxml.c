/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011 Oracle and/or its affiliates.           */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      smccxml.c                                        */
/** Description:    XAPI client XML parser wrapper.                  */
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
/** Function Name: FN_PARSE_XML                                      */
/** Description:   Convert an XML "blob" string into an XMLPARSE.    */
/**                                                                  */
/** Parse the raw input XML "blob" string and return the             */
/** corresponding XMLPARSE data hierarchy structure.                 */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "FN_PARSE_XML"

extern struct XMLPARSE *FN_PARSE_XML(char *xmlBlob,
                                     int   xmlBlobSize)
{
    int                 lastRC;
    struct XMLPARSE    *pXmlparse;

    /*****************************************************************/
    /* Create the XMLPARSE control structure and converted input.    */
    /*****************************************************************/
    pXmlparse = FN_CREATE_XMLPARSE();

    lastRC = FN_CREATE_XMLRAWIN(pXmlparse,
                                xmlBlob,
                                xmlBlobSize);

    lastRC = FN_CREATE_XMLCNVIN(pXmlparse);

    TRMSGI(TRCI_XMLPARSER,
           "After FN_CREATE_XMLCNVIN\n");

    /*****************************************************************/
    /* Now execute the parser.                                       */
    /*****************************************************************/
    lastRC = FN_GENERATE_HIERARCHY_FROM_XML(pXmlparse);

    if (pXmlparse->pHocXmlelem == NULL)
    {
        TRMSGI(TRCI_ERROR,
               "No XMLELEM found\n");

        pXmlparse->errorCode = ERR_NO_XMLELEM;
    }

    TRMEMI(TRCI_XMLPARSER,
           pXmlparse, sizeof(struct XMLPARSE),
           "After FN_GENERATE_HIERARCHY_FROM_XML RC=%i, XMLPARSE:\n",
           pXmlparse->errorCode);

    return pXmlparse;
}



