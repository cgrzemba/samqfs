/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi_response.c                                  */
/** Description:    The XAPI client ACSAPI output response service.  */
/**                                                                  */
/**                 The general format of an ACSAPI response is:     */
/**                 1. RESPONSE_HEADER consisting of:                */
/**                   i.  REQUEST_HEADER (which in the XAPI is       */
/**                       the IPC_HEADER), and the                   */
/**                   ii. RESPONSE_STATUS                            */
/**                                                                  */
/**                 The RESPONSE_HEADER is then followed by the:     */
/**                 2. Response specific response section(s).        */
/**                                                                  */
/**                 As an example, the ACKNOWLEDGEMENT_RESPOSE       */
/**                 consists of:                                     */
/**                 1. RESPONSE_HEADER consisting of:                */
/**                   i.  REQUEST_HEADER and                         */
/**                   ii. RESPONSE_STATUS                            */
/**                 2. MESSAGE_ID                                    */
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
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "acslm.h"
#include "api/defs_api.h"
#include "csi.h"
#include "srvcommon.h"
#include "xapi.h"


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
static void initGenericResponse(char *pRequest,
                                char *pGenericResponse,
                                int   genericResponseSize);


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_ack_response                                 */
/**                                                                  */
/** Description:   Output an ACSAPI ACKNOWLEDGE response.            */
/**                                                                  */
/** NOTE: The input pResponseBuffer MAY be specified as NULL.        */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_ack_response"

extern int xapi_ack_response(struct XAPIREQE *pXapireqe,
                             char            *pResponseBuffer,
                             int              responseBufferSize)
{
    int                 byteCount           = responseBufferSize;
    STATUS              status;
    IPC_HEADER         *pIpc_Header;
    ACKNOWLEDGE_RESPONSE *pAcknowledge_Response;
    ACKNOWLEDGE_RESPONSE  wkAcknowledge_Response;

    TRMSGI(TRCI_XAPI,
           "Entered; pXapireqe=%08X, pResponseBuffer=%08X, "
           "responseBufferSize=%d\n",
           pXapireqe,
           pResponseBuffer,
           responseBufferSize);

    /*****************************************************************/
    /* If not passed an response buffer, then create a generic       */
    /* response.                                                     */
    /*****************************************************************/
    if ((pResponseBuffer == NULL) ||
        (responseBufferSize == 0))
    {
        initGenericResponse(pXapireqe->pAcsapiBuffer,
                            (char*) &wkAcknowledge_Response,
                            sizeof(ACKNOWLEDGE_RESPONSE));

        pAcknowledge_Response = &wkAcknowledge_Response;
        pIpc_Header = (IPC_HEADER*) &(pAcknowledge_Response->request_header.ipc_header);
        byteCount = sizeof(ACKNOWLEDGE_RESPONSE);
    }
    else
    {
        pAcknowledge_Response = (ACKNOWLEDGE_RESPONSE*) pResponseBuffer;
        pIpc_Header = (IPC_HEADER*) &(pAcknowledge_Response->request_header.ipc_header);
    }

    TRMSGI(TRCI_XAPI,
           "pAcknowledge_Response=%08X, "
           "pIpc_Header=%08X, byteCount=%d\n",
           pAcknowledge_Response,
           pIpc_Header,
           byteCount);

    if (byteCount > MAX_MESSAGE_SIZE)
    {
        LOGMSG(STATUS_MESSAGE_TOO_LARGE, 
               "Acknowledge response size=%d; truncated to %d\n",
               byteCount,
               MAX_MESSAGE_SIZE);

        byteCount = MAX_MESSAGE_SIZE;
    }

    pIpc_Header->byte_count = byteCount;
    time(&(pXapireqe->eventTime));
    pXapireqe->packetCount++;
    pAcknowledge_Response->message_id = 1;
    pAcknowledge_Response->request_header.message_header.message_options |= ACKNOWLEDGE;

    LOG_ACSAPI_SEND(STATUS_SUCCESS, (char*) pAcknowledge_Response, byteCount,
                    "ACSAPI ACKNOWLEDGE response; status=%s (%d), command=%s (%d), seq=%d\n",
                    acs_status(STATUS_SUCCESS),
                    STATUS_SUCCESS,
                    acs_command((COMMAND) pXapireqe->command),
                    pXapireqe->command,
                    pXapireqe->seqNumber);

    status = cl_ipc_write(pXapireqe->return_socket,
                          (void*) pAcknowledge_Response,
                          byteCount);

    return(int) status;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_int_response                                 */
/**                                                                  */
/** Description:   Output an ACSAPI intermediate response.           */
/**                                                                  */
/** NOTE: The input pResponseBuffer MAY NOT be specified as NULL.    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_int_response"

extern int xapi_int_response(struct XAPIREQE *pXapireqe,
                             char            *pResponseBuffer,
                             int              responseBufferSize)
{
    int                 byteCount           = responseBufferSize;
    int                 lastRC;
    STATUS              status;
    STATUS              intermediateStatus;
    IPC_HEADER         *pIpc_Header;
    RESPONSE_TYPE      *pResponse_Type;

    TRMSGI(TRCI_XAPI,
           "Entered; pXapireqe=%08X, pResponseBuffer=%08X, "
           "responseBufferSize=%d\n",
           pXapireqe,
           pResponseBuffer,
           responseBufferSize);

    /*****************************************************************/
    /* If not passed an response buffer, then output a generic       */
    /* error response.                                               */
    /*****************************************************************/
    if ((pResponseBuffer == NULL) ||
        (responseBufferSize == 0))
    {
        lastRC = xapi_err_response(pXapireqe,
                                   NULL,
                                   0,
                                   STATUS_PROCESS_FAILURE);

        return STATUS_PROCESS_FAILURE;
    }

    pResponse_Type = (RESPONSE_TYPE*) pResponseBuffer;
    pIpc_Header = (IPC_HEADER*) &(pResponse_Type->generic_response.request_header.ipc_header);

    if (byteCount > MAX_MESSAGE_SIZE)
    {
        LOGMSG(STATUS_MESSAGE_TOO_LARGE, 
               "Intermediate response size=%d; truncated to %d\n",
               byteCount,
               MAX_MESSAGE_SIZE);

        byteCount = MAX_MESSAGE_SIZE;
    }

    pIpc_Header->byte_count = byteCount;
    time(&(pXapireqe->eventTime));
    pXapireqe->packetCount++;
    pResponse_Type->generic_response.request_header.message_header.message_options |= INTERMEDIATE;
    intermediateStatus = pResponse_Type->generic_response.response_status.status;

    LOG_ACSAPI_SEND((int) intermediateStatus, pResponseBuffer, byteCount,
                    "ACSAPI INTERMEDIATE response; status=%s (%d), command=%s (%d), seq=%d\n",
                    acs_status((STATUS) intermediateStatus),
                    intermediateStatus,
                    acs_command((COMMAND) pXapireqe->command),
                    pXapireqe->command,
                    pXapireqe->seqNumber);

    status = cl_ipc_write(pXapireqe->return_socket,
                          (void*) pResponseBuffer,
                          byteCount);

    return(int) status;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_err_response                                 */
/**                                                                  */
/** Description:   Output an ACSAPI final response with error status.*/
/**                                                                  */
/** NOTE: The input pResponseBuffer MAY be specified as NULL.        */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_err_response"

extern int xapi_err_response(struct XAPIREQE *pXapireqe,
                             char            *pResponseBuffer,
                             int              responseBufferSize,
                             int              errorStatus)
{
    int                 byteCount           = responseBufferSize;
    STATUS              status;
    IPC_HEADER         *pIpc_Header;
    RESPONSE_TYPE      *pResponse_Type;
    ACKNOWLEDGE_RESPONSE  wkAcknowledge_Response;

    TRMSGI(TRCI_XAPI,
           "Entered; pXapireqe=%08X, errorStatus=%d, "
           "pResponseBuffer=%08X, responseBufferSize=%d\n",
           pXapireqe,
           errorStatus,
           pResponseBuffer,
           responseBufferSize);

    /*****************************************************************/
    /* If not passed an response buffer, then create a generic       */
    /* response.                                                     */
    /*****************************************************************/
    if ((pResponseBuffer == NULL) ||
        (responseBufferSize == 0))
    {
        initGenericResponse(pXapireqe->pAcsapiBuffer,
                            (char*) &wkAcknowledge_Response,
                            sizeof(ACKNOWLEDGE_RESPONSE));

        pResponse_Type = (RESPONSE_TYPE*) &wkAcknowledge_Response;
        pIpc_Header = (IPC_HEADER*) &(pResponse_Type->generic_response.request_header.ipc_header);
        byteCount = sizeof(ACKNOWLEDGE_RESPONSE);
    }
    else
    {
        pResponse_Type = (RESPONSE_TYPE*) pResponseBuffer;
        pIpc_Header = (IPC_HEADER*) &(pResponse_Type->generic_response.request_header.ipc_header);
    }

    TRMSGI(TRCI_XAPI,
           "pResponse_Type=%08X, "
           "pIpc_Header=%08X, byteCount=%d\n",
           pResponse_Type,
           pIpc_Header,
           byteCount);

    if (byteCount > MAX_MESSAGE_SIZE)
    {
        LOGMSG(STATUS_MESSAGE_TOO_LARGE, 
               "Error response size=%d; truncated to %d\n",
               byteCount,
               MAX_MESSAGE_SIZE);

        byteCount = MAX_MESSAGE_SIZE;
    }

    pIpc_Header->byte_count = byteCount;
    time(&(pXapireqe->eventTime));
    pXapireqe->packetCount++;
    pResponse_Type->generic_response.response_status.status = (STATUS) errorStatus;

    LOG_ACSAPI_SEND(errorStatus, (char*) pResponse_Type, byteCount,
                    "ACSAPI FINAL response; error status=%s (%d), command=%s (%d), seq=%d\n",
                    acs_status((STATUS) errorStatus),
                    errorStatus,
                    acs_command((COMMAND) pXapireqe->command),
                    pXapireqe->command,
                    pXapireqe->seqNumber);

    status = cl_ipc_write(pXapireqe->return_socket,
                          (void*) pResponse_Type,
                          byteCount);

    return(int) status;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: xapi_fin_response                                 */
/**                                                                  */
/** Description:   Output an ACSAPI final response.                  */
/**                                                                  */
/** NOTE: The input pResponseBuffer MAY NOT be specified as NULL.    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "xapi_fin_response"

extern int xapi_fin_response(struct XAPIREQE *pXapireqe,
                             char            *pResponseBuffer,
                             int              responseBufferSize)
{
    int                 byteCount           = responseBufferSize;
    int                 lastRC;
    STATUS              status;
    STATUS              finalStatus;
    IPC_HEADER         *pIpc_Header;
    RESPONSE_TYPE      *pResponse_Type;

    TRMSGI(TRCI_XAPI,
           "Entered; pXapireqe=%08X, pResponseBuffer=%08X, "
           "responseBufferSize=%d\n",
           pXapireqe,
           pResponseBuffer,
           responseBufferSize);

    /*****************************************************************/
    /* If not passed an response buffer, then output a generic       */
    /* error response.                                               */
    /*****************************************************************/
    if ((pResponseBuffer == NULL) ||
        (responseBufferSize == 0))
    {
        lastRC = xapi_err_response(pXapireqe,
                                   NULL,
                                   0,
                                   STATUS_PROCESS_FAILURE);

        return STATUS_PROCESS_FAILURE;
    }

    pResponse_Type = (RESPONSE_TYPE*) pResponseBuffer;
    pIpc_Header = (IPC_HEADER*) &(pResponse_Type->generic_response.request_header.ipc_header);

    if (byteCount > MAX_MESSAGE_SIZE)
    {
        LOGMSG(STATUS_MESSAGE_TOO_LARGE, 
               "Final response size=%d; truncated to %d\n",
               byteCount,
               MAX_MESSAGE_SIZE);

        byteCount = MAX_MESSAGE_SIZE;
    }

    pIpc_Header->byte_count = byteCount;
    time(&(pXapireqe->eventTime));
    pXapireqe->packetCount++;
    finalStatus = pResponse_Type->generic_response.response_status.status;

    LOG_ACSAPI_SEND((int) finalStatus, pResponseBuffer, byteCount,
                    "ACSAPI FINAL response; status=%s (%d), command=%s (%d), seq=%d\n",
                    acs_status((STATUS) finalStatus),
                    finalStatus,
                    acs_command((COMMAND) pXapireqe->command),
                    pXapireqe->command,
                    pXapireqe->seqNumber);

    status = cl_ipc_write(pXapireqe->return_socket,
                          (void*) pResponseBuffer,
                          byteCount);

    return(int) status;
}


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: initGenericResponse                               */
/** Description:   Initialize the ACSAPI ACKNOWLEDGE response.       */
/**                                                                  */
/** This will be the generic default response if we were not         */
/** passed a command specific response.                              */
/**                                                                  */
/** The base ACKNOWLEDGE_RESPOSE consists of                         */
/** 1. RESPONSE_HEADER consisting of:                                */
/**    i.   REQUEST_HEADER and                                       */
/**    ii.  RESPONSE_STATUS                                          */
/** 2. MESSAGE_ID                                                    */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "initGenericResponse"

static void initGenericResponse(char *pRequest,
                                char *pGenericResponse,
                                int   genericResponseSize)
{
    REQUEST_HEADER     *pRequest_Header     = (REQUEST_HEADER*) pRequest;

    ACKNOWLEDGE_RESPONSE *pAcknowledge_Response = 
    (ACKNOWLEDGE_RESPONSE*) pGenericResponse;
    RESPONSE_STATUS    *pResponse_Status    = 
    (RESPONSE_STATUS*) &(pAcknowledge_Response->message_status); 

    TRMSGI(TRCI_XAPI,
           "Entered; pRequest=%08X, pGenericResponse=%08X, "
           "genericResponseSize=%d\n",
           pRequest,
           pGenericResponse,
           genericResponseSize);

    /*****************************************************************/
    /* Initialize generic ACKNOWLEDGE_RESPONSE.                      */
    /*****************************************************************/
    memset((char*) pAcknowledge_Response, 0, sizeof(ACKNOWLEDGE_RESPONSE));

    memcpy((char*) &(pAcknowledge_Response->request_header),
           (char*) pRequest_Header,
           sizeof(REQUEST_HEADER));

    pAcknowledge_Response->request_header.ipc_header.module_type = TYPE_LM;

    strcpy(pAcknowledge_Response->request_header.ipc_header.return_socket, 
           my_sock_name);

    pResponse_Status->status = STATUS_SUCCESS;
    pResponse_Status->type = TYPE_NONE;

    return;
}




