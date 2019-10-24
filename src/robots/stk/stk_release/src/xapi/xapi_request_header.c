/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_request_header.c                            */
/** Description:    Build XAPI transaction header service.           */
/**                                                                  */
/**                 Add common identification and control element    */
/**                 XML tags and content to the XAPI transaction.    */
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
#include "smccxmlx.h"
#include "srvcommon.h"
#include "xapi.h"


/*********************************************************************/
/* Structures:                                                       */
/*********************************************************************/


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_request_header                               */
/** Description:   Build XAPI transaction header service.            */
/**                                                                  */
/** Add common XML <header> identification elements and content      */
/** to the XAPI transaction, followed by the common XAPI XML         */
/** transaction control elements.                                    */
/**                                                                  */
/** NOTE: The input XMLHDRINFO structure (if specified) provides     */
/** "override" identification information for the XML <header> and   */
/** <job_info> elements.                                             */
/**                                                                  */
/** END PROLOGUE *****************************************************/

/*********************************************************************/
/* XAPI XML transaction:                                             */
/*===================================================================*/
/* XML Transaction Header.  The XAPI XML transaction header will     */
/* always be the 1st child XML element under the <libtrans>          */
/* element, and will have the following structure:                   */
/*===================================================================*/
/* <header>                                                          */
/*   <els_version>nnnn</els_version>                                 */
/*   <host_name>cccccccc</host_name>                                 */
/*   <client_type>CDK</server_type>                                  */
/*   <client_name>cccccccc</client_name>                             */
/*   <library_name>cccccccc</library_name>                           */
/*   <server_name>cccccccc</server_name>                   N/A (CDK) */
/*   <subsystem_name>cccc</subsystem_name>                           */
/*   <task_token>cccccccccccccccc</task_token>                       */
/*   <operator_command_flag>Y</operator_command_flag>                */
/*   <trace_flag>Y</trace_flag>                                      */
/* </header>                                                         */
/*                                                                   */
/*===================================================================*/
/* Common XAPI XML control elements are as follows:                  */
/*===================================================================*/
/* <racf_user_id>cccccccc</racf_user_id>         $XAPI_USER_ID (CDK) */
/* <racf_group_id>cccccccc</racf_group_id>      $XAPI_GROUP_ID (CDK) */
/* <xml_response_flag>Y</xml_response_flag>                          */
/* <text_response_flag>Y</text_response_flag>                        */
/* <xml_date_format>n</xml_date_format>                              */
/* <xml_case>c</xml_case>                                            */
/* <uui_read_timeout>nnnnn<uui_read_timeout>                         */
/* <uui_command_timeout>nnnnn<uui_command_timeout>                   */
/* <job_info>                                                        */
/*   <host_name>cccccccc</host_name>                                 */
/*   <user_name>cccccccc</user_name>                    passwd (CDK) */
/*   <jobname>cccccccc</jobname>                         "CDK" (CDK) */
/*   <stepname>cccccccc</stepname>                       "SSI" (CDK) */
/*   <program_name>cccccccc</program_name>                           */
/*                                                                   */
/*   NOTE: The following are not normally included as part of the    */
/*   XML <header> but may be included for the XAPI MOUNT_PINFO       */
/*   request to pass policy key information to the server.           */
/*                                                                   */
/*   <ddname>cccccccc</ddname>                                       */
/*   <dsname>ccc...ccc</dsname>                                      */
/*   <expiration_date>yyddd</expiration_date>                        */
/*   <retention_period>nnn<retention_period>                         */
/*   <volser>vvvvvv</volser>                                         */
/*   <voltype>"SPECIFIC"|"NONSPEC"</voltype>                         */
/*                                                                   */
/* </job_info>                                                       */
/*                                                                   */
/*********************************************************************/
#undef SELF
#define SELF "xapi_request_header"

extern void xapi_request_header(struct XAPICVT    *pXapicvt,
                                struct XAPIREQE   *pXapireqe,
                                struct XMLHDRINFO *pXmlhdrinfo,
                                void              *pRequestXmlparse,
                                char               xmlResponseFlag,
                                char               textResponseFlag,
                                char              *xmlCaseString,
                                char              *xmlDateString)
{
#define MAX_HOSTNAME_SIZE 255

    REQUEST_TYPE       *pRequest_Type       = (REQUEST_TYPE*) pXapireqe->pAcsapiBuffer;
    REQUEST_HEADER     *pRequest_Header     = NULL;
    MESSAGE_HEADER     *pMessage_Header     = NULL;

    struct XMLPARSE    *pXmlparse           = (struct XMLPARSE*) pRequestXmlparse;
    struct XMLELEM     *pHeaderXmlelem;
    struct XMLELEM     *pJobinfoXmlelem;
    int                 lastRC;
    int                 i;
    char                clientHostnameString[MAX_HOSTNAME_SIZE + 1] = "";
    char                seqNumberChar[9];
    char                acsapiUseridString[EXTERNAL_USERID_SIZE + 1];
    char                acsapiPasswordString[EXTERNAL_PASSWORD_SIZE + 1];
    char                wkTaskToken[(XAPI_TASK_TOKEN_SIZE + 1)];
    char                wkXmlhdrinfoString[(sizeof(pXmlhdrinfo->dsname) + 1)];

    TRMSGI(TRCI_XAPI,
           "Entered; pXapireqe=%08X, pRequest_Type=%08X, "
           "pXmlhdrinfo=%08X\n",
           pXapireqe,
           pRequest_Type,
           pXmlhdrinfo);

    if (pRequest_Type != NULL)
    {
        pRequest_Header = &(pRequest_Type->generic_request);
        pMessage_Header = &(pRequest_Header->message_header);

        STRIP_TRAILING_BLANKS(pMessage_Header->access_id.user_id.user_label,
                              acsapiUseridString,
                              sizeof(pMessage_Header->access_id.user_id.user_label));

        STRIP_TRAILING_BLANKS(pMessage_Header->access_id.password.password,
                              acsapiPasswordString,
                              sizeof(pMessage_Header->access_id.password.password));
    }
    else
    {
        strcpy(acsapiUseridString, " ");
        strcpy(acsapiPasswordString, " ");
    }

    TRMSGI(TRCI_XAPI,
           "seqNumber=%d, requestCount=%d, "
           "userid=%s, password=%s\n",
           pXapireqe->seqNumber,
           pXapicvt->requestCount,
           acsapiUseridString,
           acsapiPasswordString);

    /*****************************************************************/
    /* Add the <header> elements                                     */
    /*****************************************************************/
    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_header,
                                      NULL,
                                      0);

    pHeaderXmlelem = pXmlparse->pCurrXmlelem;

    if (pXapicvt->xapiVersion[0] > ' ')
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pHeaderXmlelem,
                                          XNAME_els_version,
                                          pXapicvt->xapiVersion,
                                          0);
    }

    if (pXapicvt->xapiHostname[0] > ' ')
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pHeaderXmlelem,
                                          XNAME_host_name,
                                          pXapicvt->xapiHostname,
                                          0);
    }

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pHeaderXmlelem,
                                      XNAME_client_type,
                                      "CDK",
                                      0);

    if ((pXmlhdrinfo != NULL) &&
        (pXmlhdrinfo->systemId[0] > ' '))
    {
        STRIP_TRAILING_BLANKS(pXmlhdrinfo->systemId,
                              wkXmlhdrinfoString,
                              sizeof(pXmlhdrinfo->systemId));

        strcpy(clientHostnameString, wkXmlhdrinfoString);
    }
    else
    {
        lastRC = gethostname(clientHostnameString, MAX_HOSTNAME_SIZE);
    }

    if (clientHostnameString[0] > ' ')
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pHeaderXmlelem,
                                          XNAME_client_name,
                                          clientHostnameString,
                                          0);
    }

    if (pXapicvt->xapiTapeplex[0] > ' ')
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pHeaderXmlelem,
                                          XNAME_library_name,
                                          pXapicvt->xapiTapeplex,
                                          0);
    }

    if (pXapicvt->xapiSubsystem[0] > ' ')
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pHeaderXmlelem,
                                          XNAME_subsystem_name,
                                          pXapicvt->xapiSubsystem,
                                          0);
    }

    /*****************************************************************/
    /* Construct a task token consisting of:                         */
    /* the client system (host) ID + the unique seqNumber.           */
    /* Replace any blanks with 0's to make a single token.           */
    /*****************************************************************/
    memset(wkTaskToken, 0, sizeof(wkTaskToken));

    if (clientHostnameString[0] > ' ')
    {
        if (strlen(clientHostnameString) > 8)
        {
            memcpy(&(wkTaskToken[0]), 
                   clientHostnameString, 
                   8);
        }
        else
        {
            strcpy(wkTaskToken, clientHostnameString);
        }
    }

    sprintf(seqNumberChar, 
            "%.8d", 
            pXapireqe->seqNumber);

    memcpy(&(wkTaskToken[8]), 
           seqNumberChar, 
           8);

    for (i = 0;
        i < XAPI_TASK_TOKEN_SIZE;
        i++)
    {
        if ((wkTaskToken[i] == ' ' ) ||
            (wkTaskToken[i] == 0))
        {
            wkTaskToken[i] = '0';
        }
    }

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pHeaderXmlelem,
                                      XNAME_task_token,
                                      wkTaskToken,
                                      16);

    /*****************************************************************/
    /* Always set the trace_flag to "Y".                             */
    /*****************************************************************/
    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pHeaderXmlelem,
                                      XNAME_trace_flag,
                                      XCONTENT_Y,
                                      0);

    /*****************************************************************/
    /* Add the XAPI control elements                                 */
    /*****************************************************************/
    if (pXapicvt->xapiUser[0] > ' ')
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pXmlparse->pHocXmlelem,
                                          XNAME_racf_user_id,
                                          pXapicvt->xapiUser,
                                          0);
    }

    if (pXapicvt->xapiGroup[0] > ' ')
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pXmlparse->pHocXmlelem,
                                          XNAME_racf_group_id,
                                          pXapicvt->xapiGroup,
                                          0);
    }

    if (xmlResponseFlag)
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pXmlparse->pHocXmlelem,
                                          XNAME_xml_response_flag,
                                          XCONTENT_Y,
                                          0);
    }

    if (textResponseFlag)
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pXmlparse->pHocXmlelem,
                                          XNAME_xml_response_flag,
                                          XCONTENT_Y,
                                          0);
    }

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_xml_date_format,
                                      xmlDateString,
                                      0);

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_xml_case,
                                      xmlCaseString,
                                      0);

    /*****************************************************************/
    /* Add the <job_info> elements                                   */
    /*****************************************************************/
    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pXmlparse->pHocXmlelem,
                                      XNAME_job_info,
                                      NULL,
                                      0);

    pJobinfoXmlelem = pXmlparse->pCurrXmlelem;

    if (clientHostnameString[0] > ' ')
    {
        lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                          pJobinfoXmlelem,
                                          XNAME_host_name,
                                          clientHostnameString,
                                          0);
    }

    /*****************************************************************/
    /* Assign the <user_name> value.                                 */
    /* MESSAGE_HEADER.access_id.user_id is used if specified.        */
    /* Otherwise, use the "real" open systems uid.                   */
    /*****************************************************************/
    if (acsapiUseridString[0] <= ' ')
    {
        xapi_userid(pXapicvt,
                    NULL,
                    acsapiUseridString);
    }

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pJobinfoXmlelem,
                                      XNAME_user_name,
                                      acsapiUseridString,
                                      0);

    /*****************************************************************/
    /* Use jobname and stepname from input XMLHDRINFO if specified.  */
    /* Otherwise, dummy up a jobname and stepname for the request.   */
    /*****************************************************************/
    if ((pXmlhdrinfo != NULL) &&
        (pXmlhdrinfo->jobname[0] > ' '))
    {
        STRIP_TRAILING_BLANKS(pXmlhdrinfo->jobname,
                              wkXmlhdrinfoString,
                              sizeof(pXmlhdrinfo->jobname));
    }
    else
    {
        strcpy(wkXmlhdrinfoString, "CDK");
    }

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pJobinfoXmlelem,
                                      XNAME_jobname,
                                      wkXmlhdrinfoString,
                                      0);

    if ((pXmlhdrinfo != NULL) &&
        (pXmlhdrinfo->stepname[0] > ' '))
    {
        STRIP_TRAILING_BLANKS(pXmlhdrinfo->stepname,
                              wkXmlhdrinfoString,
                              sizeof(pXmlhdrinfo->stepname));
    }
    else
    {
        strcpy(wkXmlhdrinfoString, "SSI");
    }

    lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                      pJobinfoXmlelem,
                                      XNAME_stepname,
                                      wkXmlhdrinfoString,
                                      0);

    /*****************************************************************/
    /* If XMLHDRINFO programName, ddname, dsname, retpd, expdt,      */
    /* volser, or volType specified,                                 */
    /* then add their XML elements to the hierarchy.                 */
    /*****************************************************************/
    if (pXmlhdrinfo != NULL)
    {
        if (pXmlhdrinfo->programName[0] > ' ')
        {
            STRIP_TRAILING_BLANKS(pXmlhdrinfo->programName,
                                  wkXmlhdrinfoString,
                                  sizeof(pXmlhdrinfo->programName));

            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pJobinfoXmlelem,
                                              XNAME_program_name,
                                              wkXmlhdrinfoString,
                                              0);
        }

        if (pXmlhdrinfo->ddname[0] > ' ')
        {
            STRIP_TRAILING_BLANKS(pXmlhdrinfo->ddname,
                                  wkXmlhdrinfoString,
                                  sizeof(pXmlhdrinfo->ddname));

            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pJobinfoXmlelem,
                                              XNAME_ddname,
                                              wkXmlhdrinfoString,
                                              0);
        }

        if (pXmlhdrinfo->dsname[0] > ' ')
        {
            STRIP_TRAILING_BLANKS(pXmlhdrinfo->dsname,
                                  wkXmlhdrinfoString,
                                  sizeof(pXmlhdrinfo->dsname));

            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pJobinfoXmlelem,
                                              XNAME_dsname,
                                              wkXmlhdrinfoString,
                                              0);
        }

        if (pXmlhdrinfo->expdt[0] > ' ')
        {
            STRIP_TRAILING_BLANKS(pXmlhdrinfo->expdt,
                                  wkXmlhdrinfoString,
                                  sizeof(pXmlhdrinfo->expdt));

            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pJobinfoXmlelem,
                                              XNAME_expiration_date,
                                              wkXmlhdrinfoString,
                                              0);
        }
        else if (pXmlhdrinfo->retpd[0] > ' ')
        {
            STRIP_TRAILING_BLANKS(pXmlhdrinfo->retpd,
                                  wkXmlhdrinfoString,
                                  sizeof(pXmlhdrinfo->retpd));

            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pJobinfoXmlelem,
                                              XNAME_retention_period,
                                              wkXmlhdrinfoString,
                                              0);
        }

        if (pXmlhdrinfo->volser[0] > ' ')
        {
            STRIP_TRAILING_BLANKS(pXmlhdrinfo->volser,
                                  wkXmlhdrinfoString,
                                  sizeof(pXmlhdrinfo->volser));

            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pJobinfoXmlelem,
                                              XNAME_volser,
                                              wkXmlhdrinfoString,
                                              0);
        }

        if (pXmlhdrinfo->volType[0] > ' ')
        {
            STRIP_TRAILING_BLANKS(pXmlhdrinfo->volType,
                                  wkXmlhdrinfoString,
                                  sizeof(pXmlhdrinfo->volType));

            lastRC = FN_ADD_ELEM_TO_HIERARCHY(pXmlparse,
                                              pJobinfoXmlelem,
                                              XNAME_voltype,
                                              wkXmlhdrinfoString,
                                              0);
        }
    }

    return;
}


