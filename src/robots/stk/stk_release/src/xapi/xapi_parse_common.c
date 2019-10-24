/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_parse_common.c                              */
/** Description:    XAPI client common response parsing services.    */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     05/11/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/***END PROLOGUE******************************************************/

/*********************************************************************/
/* Includes:                                                         */
/*********************************************************************/
#include <pwd.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "api/defs_api.h"
#include "api/db_defs_api.h"
#include "csi.h"
#include "smccodes.h"
#include "smccxmlx.h"
#include "srvcommon.h"
#include "xapi.h"


/*********************************************************************/
/* Structures:                                                       */
/*********************************************************************/

/*********************************************************************/
/* XAPIACSAPI: Table to convert an XAPI(UUI) reason code into an     */
/* ACSAPI status.                                                    */
/*********************************************************************/
struct XAPIACSAPI
{
    int                 xapiReason;
    int                 acsapiStatus;
};


/*********************************************************************/
/* MSGACSAPI: Table to convert a message number into an              */
/* ACSAPI status.                                                    */
/*********************************************************************/
struct MSGACSAPI
{
    int                 msgNum;
    int                 acsapiStatus;
};


/*********************************************************************/
/* UUI_REASON_MESSAGE: Table to convert an XAPI(UUI) reason code     */
/* into a text string.                                               */
/*********************************************************************/
struct UUI_REASON_MESSAGE
{
    int                 uuiReason;
    char                uuiReasonMsg[UUI_MAX_ERR_MSG_LEN];
    char                uuiDetailReason[UUI_MAX_ERR_DETAIL_LEN];
};


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static int xapi_translate_uui_reason_to_text(int   uuiReason,
                                             char *reasonText,
                                             char *detailReasonText);

static int xapi_translate_uui_reason_to_acs_rc(int  uuiReason,
                                               int *pAcsapiStatus);

static int xapi_translate_hscmsg_to_acs_rc(int   msgNum,
                                           char *msgText,
                                           int  *pAcsapiStatus);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_parse_header_trailer                         */
/** Description:   Parse the XAPI response <header> and trailer.     */
/**                                                                  */
/** Extract the <header> fields, <uui_return_code>,                  */
/** <uui_reason_code>, <exceptions> message, and messate number      */
/** from the XAPI response.                                          */
/**                                                                  */
/** The return code is the corresponding ACSAPI status code.         */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_parse_header_trailer"

extern int xapi_parse_header_trailer(struct XAPICVT    *pXapicvt,
                                     struct XAPIREQE   *pXapireqe,
                                     void              *pResponseXmlparse,
                                     struct XAPICOMMON *pXapicommon)
{
    struct XMLPARSE    *pXmlparse           = (struct XMLPARSE*) pResponseXmlparse;
    struct XMLELEM     *pExceptionsXmlelem  = NULL;
    struct XMLELEM     *p1stReasonXmlelem   = NULL;
    struct XMLELEM     *pNextReasonXmlelem  = NULL;

    int                 i;
    int                 j;
    int                 rx;
    int                 acsapiRC            = STATUS_SUCCESS;

#define HSC_UNRECOGNIZED_XML 106       /* SLS0106 UNRECOGNIZED XML   */

    struct RAWCOMMON   *pRawcommon          = &(pXapicommon->rawcommon);

    char                issueErrorMessage   = FALSE;
    char                normalizedReleaseString[4];
    char                msgNumString[8];
    unsigned int        intDword[2];

    char                errorText[SLOG_MAX_LINE_SIZE + 1]  = "";
    char                reasonText[SLOG_MAX_LINE_SIZE + 1] = "";

    struct XMLSTRUCT    libreplyXmlstruct[] =
    {
        XNAME_libreply,                     XNAME_uui_return_code,
        sizeof(pRawcommon->uuiRC),          pRawcommon->uuiRC,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_libreply,                     XNAME_uui_reason_code,
        sizeof(pRawcommon->uuiReason),      pRawcommon->uuiReason,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_header,                       XNAME_els_version,
        sizeof(pRawcommon->version),        pRawcommon->version,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_header,                       XNAME_library_name,
        sizeof(pRawcommon->tapeplexName),   pRawcommon->tapeplexName,
        BLANKFILL, NOBITVALUE,   0, 0,
        XNAME_header,                       XNAME_tapeplex_name,
        sizeof(pRawcommon->tapeplexName),   pRawcommon->tapeplexName,
        BLANKFILL, NOBITVALUE,   0, 0,
        XNAME_header,                       XNAME_host_name,
        sizeof(pRawcommon->hostName),       pRawcommon->hostName,
        BLANKFILL, NOBITVALUE,   0, 0,
        XNAME_header,                       XNAME_subsystem_name,
        sizeof(pRawcommon->subsystemName),  pRawcommon->subsystemName,
        BLANKFILL, NOBITVALUE,   0, 0,
        XNAME_header,                       XNAME_server_type,
        sizeof(pRawcommon->serverType),     pRawcommon->serverType,
        BLANKFILL, NOBITVALUE, 0, 0,
        XNAME_header,                       XNAME_configuration_token,
        sizeof(pRawcommon->configToken),    pRawcommon->configToken,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_header,                       XNAME_date,
        sizeof(pRawcommon->date),           pRawcommon->date,
        NOBLANKFILL, NOBITVALUE, 0, 0,
        XNAME_header,                       XNAME_time,
        sizeof(pRawcommon->time),           pRawcommon->time,
        NOBLANKFILL, NOBITVALUE, 0, 0,
    };

    int                 libreplyElementCount = sizeof(libreplyXmlstruct) / 
                                               sizeof(struct XMLSTRUCT);

    memset((char*) pXapicommon, 0, sizeof(struct XAPICOMMON));

    FN_MOVE_XML_ELEMS_TO_STRUCT(pXmlparse,
                                &libreplyXmlstruct[0],
                                libreplyElementCount);

    /*****************************************************************/
    /* Try to imply the <xml_date_format> from the <date> and        */
    /* <time> data:                                                  */
    /* If there is <date> data then we assume date format 0, 1, or 2,*/
    /* and differentiate upon the presence and position of the '-'.  */
    /* Otherwise, if there is a time only value and no date,         */
    /* then we assume date format 3.                                 */
    /* Otherwise, we set xmlDateFormat to '4' for indeterminate.     */
    /*****************************************************************/
    if (pRawcommon->date[0] > ' ')
    {
        if (pRawcommon->date[8] == '-')
        {
            pRawcommon->xmlDateFormat = '1';
        }
        else if (pRawcommon->date[7] == '-')
        {
            pRawcommon->xmlDateFormat = '2';
        }
        else
        {
            pRawcommon->xmlDateFormat = '0';
        }
    }
    else if (pRawcommon->time[0] > ' ')
    {
        memcpy(pRawcommon->stckTime,
               pRawcommon->time,
               sizeof(pRawcommon->stckTime));

        pRawcommon->xmlDateFormat = '3';
    }
    else
    {
        pRawcommon->xmlDateFormat = '4';
    }

    if (pRawcommon->version[0] > ' ')
    {
        memset(normalizedReleaseString, 0, sizeof(normalizedReleaseString));

        normalizedReleaseString[0] = pRawcommon->version[0];
        normalizedReleaseString[1] = pRawcommon->version[2];
        normalizedReleaseString[2] = pRawcommon->version[4];
        pXapicommon->normalizedRelease = atoi(normalizedReleaseString);
    }

    if (pRawcommon->stckTime[0] > ' ')
    {
        FN_CONVERT_CHARHEX_TO_FULLWORD(pRawcommon->stckTime,
                                       sizeof(pRawcommon->stckTime),
                                       (unsigned int*) &(pXapicommon->requestTime));
    }

    if (pRawcommon->configToken[0] > ' ')
    {
        FN_CONVERT_CHARHEX_TO_DOUBLEWORD(pRawcommon->configToken,
                                         sizeof(pRawcommon->configToken),
                                         &(pXapicommon->driveListTime));
    }

    if (pRawcommon->uuiRC[0] > ' ')
    {
        pXapicommon->uuiRC = atoi(pRawcommon->uuiRC);

        if (pXapicommon->uuiRC >= UUI_CMD_ERROR)
        {
            issueErrorMessage = TRUE;
        }
    }
    else
    {
        TRMSGI(TRCI_XAPI,
               "Defaulting uuiRC to UUI_CMD_FATAL\n");

        pXapicommon->uuiRC = UUI_CMD_FATAL;
        pXapicommon->uuiReason = UUI_UNKNOWN_ERROR;
    }

    if (pRawcommon->uuiReason[0] > ' ')
    {
        pXapicommon->uuiReason = atoi(pRawcommon->uuiReason);
    }

    if ((pXapicommon->uuiRC == UUI_CMD_ERROR) &&
        (pXapicommon->uuiReason == RC_SUCCESS))
    {
        TRMSGI(TRCI_XAPI,
               "Defaulting uuiReason to UUI_PARSE_ERROR\n");

        pXapicommon->uuiReason = UUI_PARSE_ERROR;
    }

    if ((pXapicommon->uuiRC > UUI_CMD_ERROR) &&
        (pXapicommon->uuiReason == RC_SUCCESS))
    {
        TRMSGI(TRCI_XAPI,
               "Defaulting uuiReason to UUI_UNKNOWN_ERROR\n");

        pXapicommon->uuiReason = UUI_UNKNOWN_ERROR;
    }


    /*****************************************************************/
    /* Extract up to 4 <reason> text messages from <exceptions>.     */
    /*****************************************************************/
    pExceptionsXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                 pXmlparse->pHocXmlelem,
                                                 XNAME_exceptions);

    if (pExceptionsXmlelem != NULL)
    {
        p1stReasonXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                    pExceptionsXmlelem,
                                                    XNAME_reason);

        pNextReasonXmlelem = p1stReasonXmlelem;

        rx = 0;

        while ((pNextReasonXmlelem != NULL) && 
               (rx < MAX_REASON_TEXTS))
        {
            memset(pRawcommon->reasonText[rx], ' ', sizeof(pRawcommon->reasonText[rx]));

            memcpy(pRawcommon->reasonText[rx],
                   pNextReasonXmlelem->pContent,
                   pNextReasonXmlelem->contentLen);

            pRawcommon->reasonText[rx] [124] = 0;

            pNextReasonXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                              pExceptionsXmlelem,
                                                              pNextReasonXmlelem,
                                                              XNAME_reason);

            rx++;
        }
    }

    /*****************************************************************/
    /* Find the first numeric substring in each reason text          */
    /* after the 1st reason text character and assume it is the      */
    /* message number (i.e. we allow for an HSC message              */
    /* prefix character that is numeric).                            */
    /*****************************************************************/
    for (rx = 0;
        rx < MAX_REASON_TEXTS;
        rx++)
    {
        pXapicommon->reasonNum[rx] = 0;

        if (pRawcommon->reasonText[rx] [0] > 0)
        {
            memset(msgNumString, 0, sizeof(msgNumString));

            for (i = 1, j = 0;
                i < sizeof(pRawcommon->reasonText[rx]);
                i++)
            {
                if (isdigit(pRawcommon->reasonText[rx] [i]))
                {
                    msgNumString[j] = pRawcommon->reasonText[rx] [i];
                    j++;
                }
                else
                {
                    if (j > 0)
                    {
                        TRMSGI(TRCI_XAPI,
                               "msgNumString=%.8s\n",
                               msgNumString);

                        break;
                    }
                }
            }

            if (j > 0)
            {
                pXapicommon->reasonNum[rx] = atoi(msgNumString);
            }
            else
            {
                pXapicommon->reasonNum[rx] = 9999;
            }
        }
    }

    /*****************************************************************/
    /* Find the most significant reason text and message number.     */
    /* NOTE: Any non-zero message number other than SLS0106          */
    /* UNRECOGNIZED XML is considered "more" significant than the    */
    /* SLS0106 message.                                              */
    /*                                                               */
    /* However, we output a separate warning for any SLS0106I        */
    /* UNRECOGNIZED XML <reason> regardless of the error code        */
    /* if MSGDEF LVL is 4 or above.                                  */
    /*****************************************************************/
    memcpy(pRawcommon->exceptionText,
           pRawcommon->reasonText[0],
           sizeof(pRawcommon->exceptionText));

    pXapicommon->exceptionNum = pXapicommon->reasonNum[0];

    for (rx = 0;
        rx < MAX_REASON_TEXTS;
        rx++)
    {
        if (pXapicommon->reasonNum[rx] == HSC_UNRECOGNIZED_XML)
        {
            TRUNCATE_BLANKS(pRawcommon->reasonText[rx],
                            reasonText,
                            sizeof(pRawcommon->reasonText[rx]));

            LOGMSG(STATUS_PROCESS_FAILURE, 
                   "XAPI warning=%s; command=%s (%d), seq=%d\n",
                   reasonText,
                   acs_command((COMMAND) pXapireqe->command),
                   pXapireqe->command,
                   pXapireqe->seqNumber);
        }
        else if ((pXapicommon->exceptionNum == HSC_UNRECOGNIZED_XML) &&
                 (pXapicommon->reasonNum[rx] > 0))
        {
            memcpy(pRawcommon->exceptionText,
                   pRawcommon->reasonText[rx],
                   sizeof(pRawcommon->exceptionText));

            pXapicommon->exceptionNum = pXapicommon->reasonNum[rx];
        }
    }

    TRMEMI(TRCI_XAPI,
           pXapicommon, sizeof(struct XAPICOMMON),
           "XAPICOMMON:\n");

    /*****************************************************************/
    /* Test if the XAPI configuration token needs to be saved, or    */
    /* indicates that a config update should be performed before the */
    /* next request.                                                 */
    /*****************************************************************/
    if (pRawcommon->configToken[0] > ' ')
    {
        if (pXapicvt->configToken[0] > ' ')
        {
            if (memcmp(pXapicvt->configToken,
                       pRawcommon->configToken,
                       sizeof(pXapicvt->configToken)) != 0)
            {
                memcpy(pXapicvt->configToken,
                       pRawcommon->configToken,
                       sizeof(pXapicvt->configToken));

                pXapicvt->updateConfig = TRUE;

                TRMSGI(TRCI_XAPI,
                       "New configToken=%.16s; old configToken=%.16s; "
                       "setting updateConfig\n",
                       pRawcommon->configToken,
                       pXapicvt->configToken);
            }
        }
        else
        {
            memcpy(pXapicvt->configToken,
                   pRawcommon->configToken,
                   sizeof(pXapicvt->configToken));

            TRMSGI(TRCI_XAPI,
                   "Initial configToken=%.16s\n",
                   pXapicvt->configToken);
        }
    }

    /*****************************************************************/
    /* If we have a real UUI error to report (i.e. we found the      */
    /* <uui_return_code> tag with a value >= UUI_CMD_ERROR),         */
    /* then log the error.                                           */
    /*****************************************************************/
    if (issueErrorMessage)
    {
        memset(errorText, 0, sizeof(reasonText));
        memset(reasonText, 0, sizeof(reasonText));

        switch (pXapicommon->uuiRC)
        {
        case UUI_CMD_ERROR:

            strcpy(errorText, "XAPI response error");

            break;

        case UUI_CMD_FATAL:

            strcpy(errorText, "XAPI fatal error");

            break;

        case UUI_CMD_ABEND:

            strcpy(errorText, "XAPI command abend");

            break;

        default:
            strcpy(errorText, "XAPI unknown error");
        }

        /*************************************************************/
        /* If we do not have a returned error message, then create   */
        /* a message from the <uui_reason_code>                      */
        /*************************************************************/
        if (pRawcommon->exceptionText[0] > 0)
        {
            TRUNCATE_BLANKS(pRawcommon->reasonText[0],
                            reasonText,
                            sizeof(pRawcommon->reasonText[0]));
        }
        else
        {
            xapi_translate_uui_reason_to_text(pXapicommon->uuiReason,
                                              reasonText,
                                              NULL);
        }

        /*************************************************************/
        /* Try to translate the returned uui reason code into a      */
        /* unique ACSAPI status (besides STATUS_PROCESS_FAILURE).    */
        /* If returned status is STATUS_PROCESS_FAILURE              */
        /* then try to translate the most significant error message  */
        /* number into a unique ACSAPI status.                       */
        /*************************************************************/
        xapi_translate_uui_reason_to_acs_rc(pXapicommon->uuiReason,
                                            &acsapiRC);

        if ((acsapiRC == STATUS_PROCESS_FAILURE) &&
            (pXapicommon->exceptionNum > 0))
        {
            xapi_translate_hscmsg_to_acs_rc(pXapicommon->exceptionNum,
                                            pRawcommon->exceptionText,
                                            &acsapiRC);
        }

        LOGMSG(acsapiRC, 
               "%s=%s; command=%s (%d), seq=%d\n",
               errorText,
               reasonText,
               acs_command((COMMAND) pXapireqe->command),
               pXapireqe->command,
               pXapireqe->seqNumber);
    }

    return acsapiRC;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: xapi_translate_uui_reason_to_text                 */
/** Description:   Translate a UUI reason code into a text string.   */
/**                                                                  */
/** Translate a UUI (XAPI) reason code (see smccodes.h) into a       */
/** text string.                                                     */
/**                                                                  */
/** NOTE: The parameter detailReasonText may be specified as NULL.   */
/** If a non-NULL value is passed, it must point to an area with     */
/** size of UUI_MAX_ERR_DETAIL_LEN + 1.                              */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "xapi_translate_uui_reason_to_text" 

static int xapi_translate_uui_reason_to_text(int   uuiReason,
                                             char *reasonText,
                                             char *detailReasonText)
{
    int                 reasonIndex;
    char                matchingReasonFound = FALSE;

    struct UUI_REASON_MESSAGE uuiReasonTable[] =
    {
        UUI_RC1,
        UUI_RC2,
        UUI_RC3,
        UUI_RC4,
        UUI_RC5,
        UUI_RC6,
        UUI_RC7,
        UUI_RC8,
        UUI_RC9,
        UUI_RC10,
        UUI_RC11,
        UUI_RC12,
        UUI_RC13,
        UUI_RC14,
        UUI_RC15,
        UUI_RC16,
        UUI_RC17,
        UUI_RC18,
        UUI_RC19,
        UUI_RC20,
        UUI_RC21,
        UUI_RC22,
        UUI_RC23,
        UUI_RC24,
        UUI_RC25,
        UUI_RC26,
        UUI_RC27,
        UUI_RC28,
        UUI_RC29,
        UUI_RC30,
        UUI_RC31,
        UUI_RC32,
        UUI_RC33,
        UUI_RC34,
        UUI_RC35,
        UUI_RC36,
        UUI_RC37,
        UUI_RC38,
        UUI_RC39,
        UUI_RC40,
        UUI_RC41,
        UUI_RC42,
        UUI_RC43,
        UUI_RC44,
        UUI_RC45,
        UUI_RC46,
        UUI_RC47,
        UUI_RC48,
        UUI_RC49,
        UUI_RC50,
        UUI_RC51,
        UUI_RC52,
        UUI_RC53,
        UUI_RC54,
        UUI_RC55,
        UUI_RC56,
        UUI_RC57,
        UUI_RC58,
        UUI_RC59,
        UUI_RC60,
        UUI_RC61,
        UUI_RC62,
        UUI_RC63,
        UUI_RC64,
        UUI_RC65,
        UUI_RC66,
        UUI_RC67,
        UUI_RC68,
        UUI_RC69,
        UUI_RC70,
        UUI_RC71,
        UUI_RC72,
        UUI_RC73,
        UUI_RC74,
        UUI_RC75,
        UUI_RC76,
        UUI_RC77,
        UUI_RC78,
        UUI_RC79,
        UUI_RC80,
        UUI_RC81,
        UUI_RC82,
        UUI_RC83,
        UUI_RC84,
        UUI_RC85,
        UUI_RC86,
        UUI_RC87,
        UUI_RC88,
        UUI_RC89,
        UUI_RC90,
        UUI_RC91,
        UUI_RC92,
        UUI_RC93,
        UUI_RC94,
        UUI_RC95,
        UUI_RC96,
        UUI_RC97,
        UUI_RC98,
        UUI_RC99,
        UUI_RC100,
        UUI_RC101,
        UUI_RC102,
        UUI_RC103,
        UUI_RC104,
        UUI_RC105,
        UUI_RC106,
        UUI_RC107,
        UUI_RC108,
        UUI_RC109,
        UUI_RC110,
        UUI_RC111,
        UUI_RC112,
        UUI_RC113,
        UUI_RC114,
        UUI_RC115,
        UUI_RC116,
        UUI_RC117,
        UUI_RC118,
        UUI_RC119,
        UUI_RC120,
        UUI_RC121,
        UUI_RC122,
        UUI_RC123,
        UUI_RC124,
        UUI_RC125,
        UUI_RC126,
        UUI_RC127,
        UUI_RC128,
        UUI_RC129,
        UUI_RC130,
        UUI_RC131,
        UUI_RC132,
        UUI_RC133,
        UUI_RC134,
        UUI_RC135,
        UUI_RC136,
        UUI_RC137,
        UUI_RC138,
        UUI_RC139,
        UUI_RC140,
        UUI_RC141,
        UUI_RC142,
        UUI_RC143,
        UUI_RC144,
        UUI_RC145,
        UUI_RC146,
        UUI_RC147,
        UUI_RC148,
        UUI_RC149,
        UUI_RC150,
        UUI_RC151,
        UUI_RC152,
        UUI_RC153,
        UUI_RC154,
        UUI_RC155,
        UUI_RC156,
        UUI_RC157,
        UUI_RC158,
        UUI_RC159,
        UUI_RC160,
        UUI_RC999
    };

#define UUI_REASON_ENTRIES sizeof(uuiReasonTable) / sizeof(struct UUI_REASON_MESSAGE)

    TRMSGI(TRCI_XAPI,
           "Entered; uuiReason=%i (%08X)\n",
           uuiReason,
           uuiReason);

    /*****************************************************************/
    /* Translate the input error code into a message index.          */
    /* If the reason text is "Available," then treat as unknown.     */
    /*****************************************************************/
    for (reasonIndex = 0;
        reasonIndex < UUI_REASON_ENTRIES;
        reasonIndex++)
    {
        if (uuiReason == uuiReasonTable[reasonIndex].uuiReason)
        {
            if (strcmp("Available", 
                       uuiReasonTable[reasonIndex].uuiReasonMsg) != 0)
            {
                matchingReasonFound = TRUE;
            }

            break;
        }
    }

    if (matchingReasonFound)
    {
        strcpy(reasonText, uuiReasonTable[reasonIndex].uuiReasonMsg);

        if (detailReasonText != NULL)
        {
            strcpy(detailReasonText,
                   uuiReasonTable[reasonIndex].uuiDetailReason);
        }

        return RC_SUCCESS;
    }

    strcpy(reasonText, "unknown error code");

    return RC_FAILURE;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: xapi_translate_uui_reason_to_acs_rc               */
/** Description:   Translate a UUI reason code into an ACSAPI status.*/
/**                                                                  */
/** Translate a UUI (XAPI) reason code (see smcodes.h) into an       */
/** equivalent ACSAPI status other than STATUS_PROCESS_FAILURE.      */
/**                                                                  */
/** RC_FAILURE indicates that the specified UUI (XAPI) reason code   */
/** was not found in the translation table.  RC_FAILURE will be a    */
/** normal result as there are many uui reason codes that do not     */
/** have a corresponding unique ACSAPI status.                       */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "xapi_translate_uui_reason_to_acs_rc" 

static int xapi_translate_uui_reason_to_acs_rc(int  uuiReason,
                                               int *pAcsapiStatus)
{
    int                 i;

    struct XAPIACSAPI   xapiAcsapiTable[]   = 
    {
        UUI_INCOMPATIBLE_RELEASE,  STATUS_INCOMPATIBLE_SERVER,
        UUI_NOT_SUPPORTED,         STATUS_INCOMPATIBLE_SERVER,
        UUI_XAPI_NOT_SUPPORTED,    STATUS_INCOMPATIBLE_SERVER,
    };

    int                 numXapiAcsapi = (sizeof(xapiAcsapiTable) / 
                                         sizeof(struct XAPIACSAPI));

    TRMSGI(TRCI_XAPI,
           "Entered; uuiReason=%i (%08X)\n",
           uuiReason,
           uuiReason);

    *pAcsapiStatus = STATUS_PROCESS_FAILURE;

    for (i = 0; 
        i < numXapiAcsapi;
        i++)
    {
        if (uuiReason == xapiAcsapiTable[i].xapiReason)
        {
            *pAcsapiStatus = xapiAcsapiTable[i].acsapiStatus;

            return RC_SUCCESS;
        }
    }

    return RC_FAILURE;
}


/***FUNCTION PROLOGUE*************************************************/
/**                                                                  */
/** Function Name: xapi_translate_hscmsg_to_acs_rc                   */
/** Description:   Translate a HSC msg number into an ACSAPI status. */
/**                                                                  */
/** Translate the input HSC server error message number into an      */
/** equivalent ACSAPI status other than STATUS_PROCESS_FAILURE.      */
/**                                                                  */
/** RC_FAILURE indicates that the specified HSC error message        */
/** number was not found in the translation table.  RC_FAILURE       */
/** will be a normal result as there are many HSC error message      */
/** numbers that do not have a corresponding unique ACSAPI status.   */
/**                                                                  */
/***END PROLOGUE******************************************************/
#undef SELF
#define SELF "xapi_translate_hscmsg_to_acs_rc" 

static int xapi_translate_hscmsg_to_acs_rc(int   msgNum,
                                           char *msgText,
                                           int  *pAcsapiStatus)
{
    int                 i;
    char               *pSubstr;

    struct MSGACSAPI    msgAcsapiTable[]    = 
    {
        10,     STATUS_INVALID_VALUE,
        50,     STATUS_INVALID_VALUE,
        163,    STATUS_VOLUME_NOT_IN_LIBRARY,
        332,    STATUS_NO_DRIVES_FOUND,
        603,    STATUS_VOLUME_NOT_IN_LIBRARY,
        5079,   STATUS_VOLUME_IN_DRIVE,
        5080,   STATUS_VOLUME_IN_DRIVE,
    };

    int                 numMsgAcsapi = (sizeof(msgAcsapiTable) / 
                                        sizeof(struct MSGACSAPI));

    TRMSGI(TRCI_XAPI,
           "Entered; msgNum=%d\n",
           msgNum);

    *pAcsapiStatus = STATUS_PROCESS_FAILURE;

    for (i = 0; 
        i < numMsgAcsapi;
        i++)
    {
        if (msgNum == msgAcsapiTable[i].msgNum)
        {
            *pAcsapiStatus = msgAcsapiTable[i].acsapiStatus;

            break;
        }
    }

    if (*pAcsapiStatus == STATUS_PROCESS_FAILURE)
    {
        return RC_FAILURE;
    }

    /*****************************************************************/
    /* If message translates into STATUS_INVALID_VALUE, then         */
    /* try to determine the type of invalid value (i.e. ACS, LSM,    */
    /* CAP, etc.).                                                   */
    /*****************************************************************/
    if (*pAcsapiStatus == STATUS_INVALID_VALUE)
    {
        pSubstr = strstr(msgText, "ACS");

        if (pSubstr != NULL)
        {
            *pAcsapiStatus == STATUS_ACS_NOT_IN_LIBRARY;

            return RC_SUCCESS;
        }

        pSubstr = strstr(msgText, "LSM");

        if (pSubstr != NULL)
        {
            *pAcsapiStatus == STATUS_LSM_NOT_IN_LIBRARY;

            return RC_SUCCESS;
        }

        pSubstr = strstr(msgText, "CAP");

        if (pSubstr != NULL)
        {
            *pAcsapiStatus == STATUS_CAP_NOT_IN_LIBRARY;

            return RC_SUCCESS;
        }

        pSubstr = strstr(msgText, "DEVICE");

        if (pSubstr != NULL)
        {
            *pAcsapiStatus == STATUS_DRIVE_NOT_IN_LIBRARY;

            return RC_SUCCESS;
        }

        pSubstr = strstr(msgText, "LOCK_ID");

        if (pSubstr != NULL)
        {
            *pAcsapiStatus == STATUS_LOCKID_NOT_FOUND;

            return RC_SUCCESS;
        }

        pSubstr = strstr(msgText, "UNIT");

        if (pSubstr != NULL)
        {
            *pAcsapiStatus == STATUS_DRIVE_NOT_IN_LIBRARY;

            return RC_SUCCESS;
        }
    }

    return RC_SUCCESS;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_parse_loctype_rawvolume                      */
/** Description:   Derive the locType flag in a RAWVOLUME structure. */
/**                                                                  */
/** Derive and update the locType field in the input RAWVOLUME       */
/** structure.                                                       */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_parse_loctype_rawvolume"

extern void xapi_parse_loctype_rawvolume(struct RAWVOLUME *pRawvolume)
{
    pRawvolume->locType[0] = LOCATION_FIRST;

    /*****************************************************************/
    /* Check for <result>failure</result>.                           */
    /*                                                               */
    /* NOTE: CDK does not recognize NONLIB.  If "FAILURE" result     */
    /* then just return LOCATION_FIRST (which is invalid).           */
    /*****************************************************************/
    if (toupper(pRawvolume->result[0]) == 'F')
    {
        pRawvolume->locType[0] = LOCATION_FIRST;

        return;
    }

    /*****************************************************************/
    /* Check for VTSS location (may be resident or non-resident).    */
    /*                                                               */
    /* NOTE: CDK does not recognize VIRTUAL.  If virtual, simulate   */
    /* a library location of ACS X'FF'.                              */
    /*****************************************************************/
    if (memcmp(pRawvolume->media,
               "VIRTUAL",
               7) == 0)
    {
        pRawvolume->locType[0] = LOCATION_CELL;

        return;
    }

    /*****************************************************************/
    /* Check for LIBRARY location if mounted.                        */
    /*****************************************************************/
    if ((toupper(pRawvolume->status[0]) == 'M') ||
        (pRawvolume->charDevAddr[0] > ' '))
    {
        pRawvolume->locType[0] = LOCATION_DRIVE;

        return;
    }

    /*****************************************************************/
    /* Check for LIBRARY location if in home cell.                   */
    /*****************************************************************/
    if ((toupper(pRawvolume->status[0]) == 'E') ||
        (pRawvolume->homeCell[0] > ' '))
    {
        pRawvolume->locType[0] = LOCATION_CELL;

        return;
    }

    return;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_parse_rectechs_xmlparse                      */
/** Description:   Derive MODEL names from a RAWVOLUME structure.    */
/**                                                                  */
/** Extract and update the multiple model names from the             */
/** <read_model_list>, <write_model_list>, and <append_model_list>   */
/** XML tags in the input RAWVOLUME structure.                       */
/**                                                                  */
/** NOTE: The composite "+" delimited model name string is           */
/** currently unused by the XAPI client.  The "+" delimited model    */
/** string is derived to be consistent with VMClient, and SMC MVS    */
/** clients.                                                         */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_parse_rectechs_xmlparse"

extern void xapi_parse_rectechs_xmlparse(void             *pResponseXmlparse,
                                         void             *pStartXmlelem,
                                         struct RAWVOLUME *pRawvolume)
{
    struct XMLPARSE    *pXmlparse           = (struct XMLPARSE*) pResponseXmlparse;
    struct XMLELEM     *pParentXmlelem      = (struct XMLELEM*) pStartXmlelem;
    struct XMLELEM     *pReadModelListXmlelem   = NULL;
    struct XMLELEM     *pWriteModelListXmlelem  = NULL;
    struct XMLELEM     *pAppendModelListXmlelem = NULL;
    struct XMLELEM     *p1stModelXmlelem    = NULL;
    struct XMLELEM     *pNextModelXmlelem   = NULL;
    int                 plusStringLen; 
    char               *pCurrModel;

    /*****************************************************************/
    /* Find the <read_model_list> header tag and then extract all    */
    /* of the <model> names under it.                                */
    /*****************************************************************/
    memset(pRawvolume->readModelList, 0, sizeof(pRawvolume->readModelList));

    pCurrModel = &(pRawvolume->readModelList[0]);

    pReadModelListXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                    pParentXmlelem,
                                                    XNAME_read_model_list);

    TRMSGI(TRCI_XAPI,
           "pReadModelListXmlelem=%08X\n",
           pReadModelListXmlelem);

    if (pReadModelListXmlelem != NULL)
    {
        p1stModelXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                   pReadModelListXmlelem,
                                                   XNAME_model);

        pNextModelXmlelem = p1stModelXmlelem;
        plusStringLen = 0;

        while (pNextModelXmlelem != NULL)
        {
            memcpy(pCurrModel,
                   pNextModelXmlelem->pContent,
                   pNextModelXmlelem->contentLen);

            pCurrModel += pNextModelXmlelem->contentLen;
            *pCurrModel = '+';
            pCurrModel++;
            plusStringLen = plusStringLen + (pNextModelXmlelem->contentLen + 1);

            if ((MAX_RMCODE_PLUS_STRING - plusStringLen) < 8)
            {
                TRMSGI(TRCI_XAPI,
                       "Breaking out of while pNextReadModelXmlelem\n");

                break;
            }

            pNextModelXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                             pReadModelListXmlelem,
                                                             pNextModelXmlelem,
                                                             XNAME_model);
        }

        if (pCurrModel != &(pRawvolume->readModelList[0]))
        {
            pCurrModel--;
            *pCurrModel = 0;
        }
    }

    /*****************************************************************/
    /* Find the <write_model_list> header tag and then extract all   */
    /* of the <model> names under it.                                */
    /*****************************************************************/
    memset(pRawvolume->writeModelList, 0, sizeof(pRawvolume->writeModelList));

    pCurrModel = &(pRawvolume->writeModelList[0]);

    pWriteModelListXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                     pParentXmlelem,
                                                     XNAME_write_model_list);

    TRMSGI(TRCI_XAPI,
           "pWriteModelListXmlelem=%08X\n",
           pReadModelListXmlelem);

    if (pWriteModelListXmlelem != NULL)
    {
        p1stModelXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                   pWriteModelListXmlelem,
                                                   XNAME_model);

        pNextModelXmlelem = p1stModelXmlelem;
        plusStringLen = 0;

        while (pNextModelXmlelem != NULL)
        {
            memcpy(pCurrModel,
                   pNextModelXmlelem->pContent,
                   pNextModelXmlelem->contentLen);

            pCurrModel += pNextModelXmlelem->contentLen;
            *pCurrModel = '+';
            pCurrModel++;
            plusStringLen = plusStringLen + (pNextModelXmlelem->contentLen + 1);

            if ((MAX_RMCODE_PLUS_STRING - plusStringLen) < 8)
            {
                TRMSGI(TRCI_XAPI,
                       "Breaking out of while pNextWriteModelXmlelem\n");

                break;
            }


            pNextModelXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                             pWriteModelListXmlelem,
                                                             pNextModelXmlelem,
                                                             XNAME_model);
        }

        if (pCurrModel != &(pRawvolume->writeModelList[0]))
        {
            pCurrModel--;
            *pCurrModel = 0;
        }
    }

    /*****************************************************************/
    /* Find the <append_model_list> header tag and then extract      */
    /* all of the <model> names under it.                            */
    /*****************************************************************/
    memset(pRawvolume->appendModelList, 0, sizeof(pRawvolume->appendModelList));

    pCurrModel = &(pRawvolume->appendModelList[0]);

    pAppendModelListXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                      pParentXmlelem,
                                                      XNAME_append_model_list);

    TRMSGI(TRCI_XAPI,
           "pAppendModelListXmlelem=%08X\n",
           pReadModelListXmlelem);

    if (pAppendModelListXmlelem != NULL)
    {
        p1stModelXmlelem = FN_FIND_ELEMENT_BY_NAME(pXmlparse,
                                                   pAppendModelListXmlelem,
                                                   XNAME_model);

        pNextModelXmlelem = p1stModelXmlelem;
        plusStringLen = 0;

        while (pNextModelXmlelem != NULL)
        {
            memcpy(pCurrModel,
                   pNextModelXmlelem->pContent,
                   pNextModelXmlelem->contentLen);

            pCurrModel += pNextModelXmlelem->contentLen;
            *pCurrModel = '+';
            pCurrModel++;
            plusStringLen = plusStringLen + (pNextModelXmlelem->contentLen + 1);

            if ((MAX_RMCODE_PLUS_STRING - plusStringLen) < 8)
            {
                TRMSGI(TRCI_XAPI,
                       "Breaking out of while pNextAppendModelXmlelem\n");

                break;
            }

            pNextModelXmlelem = FN_FIND_NEXT_SIBLING_BY_NAME(pXmlparse,
                                                             pAppendModelListXmlelem,
                                                             pNextModelXmlelem,
                                                             XNAME_model);
        }

        if (pCurrModel != &(pRawvolume->appendModelList[0]))
        {
            pCurrModel--;
            *pCurrModel = 0;
        }
    }

    return;
}


