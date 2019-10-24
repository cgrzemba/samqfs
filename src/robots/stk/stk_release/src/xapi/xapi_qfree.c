/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_qfree.c                                     */
/** Description:    XAPI client QUERY SERVER free cell count service.*/
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     06/01/11                          */
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

#include "api/defs_api.h"
#include "csi.h"
#include "smccxmlx.h"
#include "srvcommon.h"
#include "xapi.h"


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#define QUERY_SEND_TIMEOUT         5   /* TIMEOUT values in seconds  */       
#define QUERY_RECV_TIMEOUT_1ST     300
#define QUERY_RECV_TIMEOUT_NON1ST  600


/*********************************************************************/
/* Structures:                                                       */
/*********************************************************************/

/*********************************************************************/
/* QVOLACS: The following maps the fixed UUITEXT lines returned in   */
/* the CSV response (AA,NNNN,NNN,NNNNNNNN,NNNNNNN).                  */
/*********************************************************************/
struct QACSCSV               
{
    char                acs[2];
    char                _f0;           /* comma                      */
    char                lsmCount[4];
    char                _f1;           /* comma                      */
    char                capCount[3];
    char                _f2;           /* colon                      */
    char                freeCellCount[8];
    char                _f3;           /* colon                      */
    char                scratchCount[7];
};


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int buildQacsAllRequest(struct XAPICVT   *pXapicvt,
                               struct XAPIREQE  *pXapireqe,
                               char            **ptrXapiBuffer,
                               int              *pXapiBufferSize);

static int extractQacsAllResponse(struct XAPICVT  *pXapicvt,
                                  struct XAPIREQE *pXapireqe,
                                  struct XMLPARSE *pXmlparse,
                                  int             *pFreeCellCount);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_free                                         */
/** Description:   XAPI client QUERY SERVER free cell count service. */
/**                                                                  */
/** Return free cell count for the TapePlex server (all ACS(s)).     */
/** The ACSAPI QUERY SERVER response returns the total free cell     */
/** count for the TapePlex.  However, the total free cell count      */
/** is not returned from the XAPI <query_server> request.  This      */
/** function can be called in addition to the XAPI <query_server>    */
/** request to derive that missing piece of information.             */
/**                                                                  */
/** The XAPI XML format <query_acs> (all) request is built that      */
/** requests CSV fixed format output;  The XAPI XML request is       */
/** then transmitted to the server via TCP/IP;  The received         */
/** XAPI CSV response <uui_text> lines count fields are then         */
/** translated and accumulated into a total TapePlex free cell       */
/** count that is returned to the caller.                            */
/**                                                                  */
/** The <query_acs> command to accumulate the free cell count        */
/** is allowed to proceed even when the XAPI client is in the        */
/** IDLE state.                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_qfree"

extern int xapi_qfree(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe,
                      int             *pFreeCellCount)
{
    int                 qfreeRC             = STATUS_SUCCESS;
    int                 lastRC;
    int                 xapiBufferSize;
    char               *pXapiBuffer         = NULL;
    struct XMLPARSE    *pXmlparse           = NULL;

    TRMSGI(TRCI_XAPI, "Entered\n");

    *pFreeCellCount = 0;

    lastRC = buildQacsAllRequest(pXapicvt,
                                 pXapireqe,
                                 &pXapiBuffer,
                                 &xapiBufferSize);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from buildQacsAllRequest(pBuffer=%08X, size=%d)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize);

    lastRC = xapi_tcp(pXapicvt,
                      pXapireqe,
                      pXapiBuffer,
                      xapiBufferSize,
                      QUERY_SEND_TIMEOUT,         
                      QUERY_RECV_TIMEOUT_1ST,     
                      QUERY_RECV_TIMEOUT_NON1ST,  
                      (void**) &pXmlparse);

    TRMSGI(TRCI_XAPI,
           "lastRC=%d from xapi_tcp(pBuffer=%08X, size=%d, pXmlparse=%08X)\n",
           lastRC,
           pXapiBuffer,
           xapiBufferSize,
           pXmlparse);

    if (lastRC != STATUS_SUCCESS)
    {
        qfreeRC = lastRC;
    }

    /*****************************************************************/
    /* At this point we can free the XAPI XML request string.        */
    /*****************************************************************/
    TRMSGI(TRCI_STORAGE,
           "free pXapiBuffer=%08X, len=%i\n",
           pXapiBuffer,
           xapiBufferSize);

    free(pXapiBuffer);

    /*****************************************************************/
    /* Now run through the returned CSV.                             */
    /*****************************************************************/
    if (qfreeRC == STATUS_SUCCESS)
    {
        lastRC = extractQacsAllResponse(pXapicvt,
                                        pXapireqe,
                                        pXmlparse,
                                        pFreeCellCount);

        TRMSGI(TRCI_XAPI,
               "lastRC=%d from extractQacsAllResponse\n",
               lastRC);
    }

    /*****************************************************************/
    /* When done, free the XMLPARSE and associated structures.       */
    /*****************************************************************/
    if (pXmlparse != NULL)
    {
        FN_FREE_HIERARCHY_STORAGE(pXmlparse);
    }

    return qfreeRC;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: buildQacsAllRequest                               */
/** Description:   Build an XAPI <query_acs> request.                */
/**                                                                  */
/** Build the XAPI XML format <query_acs> (all) request              */
/** specifying fixed format CSV output.                              */
/**                                                                  */
/**==================================================================*/
/** The XAPI XML <query_acs> (all) request for CSV consists of:      */
/**==================================================================*/
/** <libtrans>COMMAND                                                */
/**   <header>                                                       */
/**     header element children                                      */
/**   </header>                                                      */
/**   ...additional XAPI control tags                                */
/**   <command>                                                      */
/**     <query_acs>                                                  */
/**     </query_acs>                                                 */
/**   </command>                                                     */
/**                                                                  */
/**   <csv_break>acs_data<csv_break>                                 */
/**   <csv_fields>acs,lsm_count,cap_count,free_cell_count,           */
/**     scratch_count</csv_fields>                                   */
/**   <csv_fixed_flag>Y</csv_fixed_flag>                             */
/**   <csv_notitle_flag>Y</csv_notitle_flag>                         */
/**                                                                  */
/** </libtrans>                                                      */
/**                                                                  */
/** NOTES:                                                           */
/** (1) If <acs> is omitted, then all acs(s) are returned.           */
/** (2) While the CDK allows count > 1, and multiple ACS(s) be       */
/**     specified, the XAPI does not currenly allow specification    */
/**     of multiple <acs> tags in the query.                         */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "buildQacsAllRequest"

static int buildQacsAllRequest(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char           **ptrXapiBuffer,
                               int             *pXapiBufferSize)
{
    int                 lastRC;
    int                 xapiRequestSize     = 0;

    char               *pXapiRequest        = NULL;

    struct XMLPARSE    *pXmlparse; 
    struct XMLELEM     *pParentXmlelem;
    struct XMLRAWIN    *pXmlrawin;

    TRMSGI(TRCI_XAPI,
           "Entered\n");

    *ptrXapiBuffer = NULL;
    *pXapiBufferSize = 0;

    pXmlparse = FN_CREATE_XMLPARSE();

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      NULL,
                                      XNAME_libtrans,
                                      NULL,
                                      0);

    xapi_request_header(pXapicvt,
                        pXapireqe,
                        NULL,
                        (void*) pXmlparse,
                        FALSE,
                        FALSE,
                        XML_CASE_UPPER,
                        XML_DATE_STCK);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_break_tag,
                                      XNAME_acs_data,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_notitle_flag,
                                      XCONTENT_Y,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_fixed_flag,
                                      XCONTENT_Y,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_csv_fields,
                                      "acs,lsm_count,cap_count,free_cell_count,scratch_count",
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_command,
                                      NULL,
                                      0);

    pParentXmlelem = pXmlparse->pCurrXmlelem;

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pParentXmlelem,
                                      XNAME_query_acs,
                                      NULL,
                                      0);

    pXmlrawin = FN_GENERATE_XML_FROM_HIERARCHY(pXmlparse);

    xapiRequestSize = (pXmlrawin->dataLen) + 1;

    pXapiRequest = (char*) malloc(xapiRequestSize);

    TRMSGI(TRCI_STORAGE,
           "malloc XAPI buffer=%08X, len=%i\n",
           pXapiRequest,
           xapiRequestSize);

    memcpy(pXapiRequest, 
           pXmlrawin->pData, 
           pXmlrawin->dataLen);

    pXapiRequest[pXmlrawin->dataLen] = 0;

    FN_FREE_HIERARCHY_STORAGE(pXmlparse);

    *ptrXapiBuffer = pXapiRequest;
    *pXapiBufferSize = xapiRequestSize;

    return STATUS_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: extractQacsAllResponse                            */
/** Description:   Extract the <query_acs> CSV response.             */
/**                                                                  */
/** Extract the output of the XAPI CSV <query_acs> request and       */
/** count the total number of free cells in the returned             */
/** <uui_text> CSV lines (one <uui_text> line per ACS).              */
/**                                                                  */
/**==================================================================*/
/** The XAPI CSV <query_acs> (all) responses consists of:            */
/**==================================================================*/
/** <libreply>                                                       */
/**   <uui_line_type>V</uui_line_type>                               */
/**   <uui_text>AA, nnn, nn,   nnnnn,   nnnn                 (FIXED) */
/**       (NOTE: Numeric fields may have leading blanks)             */
/**     </uui_text>                                                  */
/**   ...repeated <uui_line> and <uui_text> entries                  */
/**   <exceptions>                                                   */
/**     <reason>ccc...ccc</reason>                                   */
/**     ...repeated <reason> entries                                 */
/**   </exceptions>                                                  */
/**   <uui_return_code>nnnn</uui_return_code>                        */
/**   <uui_reason_code>nnnn</uui_reason_code>                        */
/** </libreply>                                                      */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "extractQacsAllResponse"

static int extractQacsAllResponse(struct XAPICVT  *pXapicvt,
                                  struct XAPIREQE *pXapireqe,
                                  struct XMLPARSE *pXmlparse,
                                  int             *pFreeCellCount)
{
    int                 i;
    int                 wkInt;
    int                 totalFreeCellCount = 0;
    char               *pChar;

    struct XMLELEM     *pFirstUuiTextXmlelem;
    struct XMLELEM     *pNextUuiTextXmlelem;
    struct XMLELEM     *pParentXmlelem;
    struct QACSCSV     *pQacscsv;

    /*****************************************************************/
    /* Run through the return CSV lines.                             */
    /*****************************************************************/
    pFirstUuiTextXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                   pXmlparse->pHocXmlelem,
                                                   XNAME_uui_text);

    if (pFirstUuiTextXmlelem != NULL)
    {
        pParentXmlelem = pFirstUuiTextXmlelem->pParentXmlelem;

        TRMSGI(TRCI_XAPI,
               "pFirstUuiTextXmlelem=%08X; pParentXmlelem=%08X\n",
               pFirstUuiTextXmlelem,
               pParentXmlelem);

        pNextUuiTextXmlelem = pFirstUuiTextXmlelem;

        while (pNextUuiTextXmlelem != NULL)
        {
            pQacscsv = (struct QACSCSV*) pNextUuiTextXmlelem->pContent;

            /*********************************************************/
            /* Zero fill the numeric fields returned in the CSV.     */
            /*********************************************************/
            for (i = 0, pChar = (char*) &(pQacscsv->acs);
                i < (sizeof(struct QACSCSV) - offsetof(struct QACSCSV, acs));
                i++, pChar++)
            {
                if (*pChar == ' ')
                {
                    *pChar = '0';
                }
            }

            TRMEM(pQacscsv, sizeof(struct QACSCSV), 
                  "QACSCSV:\n");

            if (pQacscsv->freeCellCount[0] > ' ')
            {
                wkInt = 0;

                FN_CONVERT_DIGITS_TO_FULLWORD(pQacscsv->freeCellCount,
                                              sizeof(pQacscsv->freeCellCount),
                                              &wkInt);

                totalFreeCellCount += wkInt;

                TRMSGI(TRCI_XAPI,
                       "totalFreeCellCount=%d\n",
                       totalFreeCellCount);
            }

            pNextUuiTextXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                               pParentXmlelem,
                                                               pNextUuiTextXmlelem,
                                                               XNAME_uui_text);

#ifdef DEBUG

            if (pNextUuiTextXmlelem != NULL)
            {
                TRMSGI(TRCI_XAPI,
                       "pNextUuiTextXmlelem=%08X\n",
                       pNextUuiTextXmlelem);
            }

#endif

        }
    }

    *pFreeCellCount = totalFreeCellCount;

    return STATUS_SUCCESS;
}


