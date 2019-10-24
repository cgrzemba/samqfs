/** HEADER FILE PROLOGUE *********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      srvcommon.h                                      */
/** Description:    Services common header file.                     */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     05/03/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/** END PROLOGUE *****************************************************/

#ifndef SRVCOMMON_HEADER
#define SRVCOMMON_HEADER


/*********************************************************************/
/* Global variable declaration.                                      */
/*                                                                   */
/* Defined (i.e. allocated) in srvvars.                              */
/* Set using the GLOBAL_SRVCOMMON_SET macro (which calls srvvars).   */
/*********************************************************************/
extern char  global_cdktrace_flag;
extern char  global_cdklog_flag;
extern char  global_srvtrcs_active_flag; 
extern char  global_srvlogs_active_flag;
extern char  global_module_type[8];


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#define MAX_MODULE_TYPE_SIZE       6   /* "CSI", "SSI', "(none)",    */
/*                                        ..."CLIENT", "HTTP", etc.  */


/*********************************************************************/
/* Environmental variable names used and/or set by the common        */
/* trace and log services.                                           */
/*********************************************************************/
#define CDKTYPE_VAR        "CDKTYPE"   /* Source module type variable*/
#define CDKTRACE_VAR       "CDKTRACE"  /* Trace control variable     */
#define CDKLOG_VAR         "CDKLOG"    /* Log control variable       */
#define STRCPIPE_VAR       "STRCPIPE"  /* Trace pipe variable        */
#define STRCFILE_VAR       "STRCFILE"  /* Trace file variable        */
#define SLOGPIPE_VAR       "SLOGPIPE"  /* Trace pipe variable        */
#define SLOGFILE_VAR       "SLOGFILE"  /* Trace file variable        */
#define SRVTRCSPID_VAR     "SRVTRCSPID"/* Trace service PID variable */
#define SRVLOGSPID_VAR     "SRVLOGSPID"/* Log service PID variable   */


/*********************************************************************/
/* The following are for defining names for CDKLOG index             */
/* positions to control log processing:                              */
/* CDKLOG=1             Log event messages to the log file.          */ 
/* CDKLOG=01            Log XAPI ACSAPI send packets to the log.file.*/
/* CDKLOG=001           Log XAPI ACSAPI recv packets to the log.file.*/
/* CDKLOG=0001          Log XAPI XML send packets to the log.file.   */
/* CDKLOG=00001         Log XAPI XML recv packets to the log.file.   */
/* CDKLOG=000001        Log CSI send packets to the log.file.        */
/* CDKLOG=0000001       Log CSI recv packets to the log.file.        */
/* CDKLOG=00000001      Log HTTP XML send packets to the log.file.   */
/* CDKLOG=000000001     Log HTTP XML recv packets to the log.file.   */
/* CDKLOG=0000000001    Log event error messages to stdout.          */
/*===================================================================*/
/* The CDKLOG variable can be up to MAX_CDKLOG_TYPE                  */
/* characters long.  Each character is evaluated as if it were       */
/* a bit (0 or 1).                                                   */
/*                                                                   */
/* If the specified srvlogc input logIndex in the CDKLOG             */
/* variable is on (set to "1"), the log record is written.           */
/* If the specified srvlogc input logIndex in the CDKLOG             */
/* variable is something other than "1", then the log record         */
/* is not written.                                                   */
/*                                                                   */
/* If the CDKLOG variable is not set at all, then srvlogc will       */
/* set it to "0" (effectively turning all logging OFF).              */
/*********************************************************************/
#define MAX_CDKLOG_TYPE            16
#define LOGI_EVENT_MESSAGE         0   /* Event message (no packet)  */
#define LOGI_ERROR_STDOUT          1   /* Error message to stdout    */
#define LOGI_EVENT_ACSAPI_SEND     2   /* ACSAPI packet              */
#define LOGI_EVENT_ACSAPI_RECV     3   /* ACSAPI packet              */
#define LOGI_EVENT_XAPI_SEND       4   /* XAPI XML packet            */
#define LOGI_EVENT_XAPI_RECV       5   /* XAPI XML packet            */
#define LOGI_EVENT_CSI_SEND        6   /* ACSAPI packet              */
#define LOGI_EVENT_CSI_RECV        7   /* ACSAPI packet              */
#define LOGI_EVENT_HTTP_SEND       8   /* XAPI XML packet            */
#define LOGI_EVENT_HTTP_RECV       9   /* XAPI XML packet            */
/*                                        ...server executables      */
/* CDKLOG indices 10-15 are currently undefined and available        */


/*********************************************************************/
/* The following are for defining names for CDKTRACE index           */
/* positions to control trace granuality.                            */
/* CDKTRACE=1           Trace errors.                                */ 
/* CDKTRACE=01          Trace ACSAPI.                                */
/* CDKTRACE=001         Trace SSI.                                   */
/* CDKTRACE=0001        Trace CSI.                                   */
/* CDKTRACE=00001       Trace common components.                     */
/* CDKTRACE=000001      Trace XAPI.                                  */
/* CDKTRACE=0000001     Trace XAPI (client) TCP/IP functions.        */
/* CDKTRACE=00000001    Trace t_acslm or t_http server executables.  */
/* CDKTRACE=000000001   Trace HTTP (server) TCP/IP functions.        */
/* CDKTRACE=0000000001  Trace malloc, free, and shared mem functions.*/
/* CDKTRACE=00000000001 Trace XML parser.                            */
/*===================================================================*/
/* The CDKTRACE variable can be up to MAX_CDKTRACE_TYPE              */
/* characters long.  Each character is evaluated as if it were       */
/* a bit (0 or 1).                                                   */
/*                                                                   */
/* If the specified srvtrcc input traceIndex in the CDKTRACE         */
/* variable is on (set to "1"), the trace record is written.         */
/* If the specified srvtrcc input traceIndex in the CDKTRACE         */
/* variable is something other than "1", then the trace record       */
/* is not written.                                                   */
/*                                                                   */
/* If the CDKTRACE variable is not set at all, then srvtrcc will     */
/* set it to "0" (effectively turning all tracing OFF).              */
/*********************************************************************/
#define MAX_CDKTRACE_TYPE          16
#define TRCI_ERROR                 0
#define TRCI_DEFAULT               0   
#define TRCI_ACSAPI                1   /* Trace ACSAPI               */
#define TRCI_SSI                   2   /* Trace SSI client           */
#define TRCI_CSI                   3   /* Trace CSI server           */
#define TRCI_COMMON                4   /* Trace common_lib           */
#define TRCI_XAPI                  5   /* Trace XAPI conversion      */
#define TRCI_XTCPIP                6   /* Trace XAPI client TCP/IP   */
#define TRCI_SERVER                7   /* Trace t_acslm or t_http    */
/*                                        ...server executables      */
#define TRCI_HTCPIP                8   /* Trace t_http server TCP/IP */
#define TRCI_STORAGE               9   /* Trace malloc() and free()  */
#define TRCI_XMLPARSER             10  /* Trace XML parser           */
/* CDKTRACE indices 11-15 are currently undefined and available      */

#define TRCI_CDKTYPE              -1   /* Determine CDKTRACE index   */
/*                                        ...based upon environment  */
/*                                        ...variable CDKTYPE        */


#define DEFAULT_STRCPIPE           "trace.pipe"
#define DEFAULT_STRCFILE           "trace.file"
#define STRC_MAX_LINE_SIZE         255

#define DEFAULT_SLOGPIPE           "log.pipe"
#define DEFAULT_SLOGFILE           "log.file"
#define SLOG_MAX_LINE_SIZE         255


/*********************************************************************/
/* The 5 LOG "header" lines will vary depending upon the type of     */
/* packet being traced:                                              */
/*                                                                   */
/* For LOGI_EVENT_ACSAPI_SEND and LOGI_EVENT_ACSAPI_RECV:            */
/* (1)  Date Time [type:process_id] status                           */
/* (2)  [object:line]: message                                       */
/* (3)  Packet source:      From/To                                  */
/* (4)  Packet ID:          number     command:  name                */
/* (5)  SSI return socket:  number     SSI return pid:  number       */
/*                                                                   */
/* For LOGI_EVENT_XAPI_SEND and LOGI_EVENT_XAPI_RECV:                */
/* (1)  Date Time [type:process_id] status                           */
/* (2)  [object:line]: message                                       */
/* (3)  Packet source:      From/To                                  */
/* (4)  Packet ID:          number     command:  name                */
/* (5)  HTTP server host:   name       HTTP server port:  number     */
/*                                                                   */
/* For LOGI_EVENT_CSI_SEND and LOGI_EVENT_CSI_RECV:                  */
/* (1)  Date Time [type:process_id] status                           */
/* (2)  [object:line]: message                                       */
/* (3)  Packet source:      From/To                                  */
/* (4)  SSI identifier:     number     command:  name                */
/* (5)  SSI client address: name       SSI client port:  number      */
/*                                                                   */
/* These header lines must be kept synchronized with td_get_packe.c  */
/* where the log.file is processed and individual packets decoded.   */
/*********************************************************************/
#define SLOG_PACKET_SOURCE         "Packet source:"
#define SLOG_PACKET_ID             "Packet ID:     "
#define SLOG_SSI_IDENTIFIER        "SSI identifier:"
#define SLOG_HTTP_SERVER_HOST      "HTTP server host:  "
#define SLOG_SSI_RETURN_SOCKET     "SSI return socket: "
#define SLOG_SSI_CLIENT_ADDRESS    "SSI client address:"

#define SLOG_TO_HTTP               "To HTTP    "
#define SLOG_FROM_HTTP             "From HTTP  "
#define SLOG_TO_ACSAPI             "To ACSAPI  "
#define SLOG_FROM_ACSAPI           "From ACSAPI"
#define SLOG_TO_CSI                "To CSI     "
#define SLOG_FROM_CSI              "From CSI   "

#ifndef COMPONENT_TYPE
    #define COMPONENT_TYPE         0
#endif


/*********************************************************************/
/* Macros:                                                           */
/*********************************************************************/

/*********************************************************************/
/* GLOBAL_SRVCOMMON_SET: Set global trace and log variables.         */
/*********************************************************************/
#define GLOBAL_SRVCOMMON_SET(pgmTypE) \
do \
{ \
    srvvars(pgmTypE); \
}while(0)


/*********************************************************************/
/* TR* and LOG* macros.                                              */
/*                                                                   */
/* The environmental variables to control the trace/log services are:*/
/*                                                                   */
/* STRCPIPE = The name of the trace pipe (default is "trace.pipe").  */
/* STRCFILE = The name of the trace file (default is "trace.file").  */
/* CDKTRACE = {1|0} to turn on tracing.  See settings above.         */
/*                                                                   */
/* SLOGPIPE = The name of the log pipe (default is "log.pipe").      */
/* SLOGFILE = The name of the log file (default is "log.file").      */
/* CDKLOG = {1|0} to turn on logging.  See settings above.           */
/* NOTE: That the LOG* macros also invoke TRMSG or TRMEM macros.     */
/*                                                                   */
/*********************************************************************/

/*********************************************************************/
/* TRMSG and TRMEM are for writing output to the trace.file.         */
/*                                                                   */
/* These macros call srvtrcc using the TRCI_CDKTYPE trace index.     */
/* The TRCI_CDKTYPE trace index is specified it tells the srvtrcc.c  */
/* trace function to set the trace index depending upon the value    */
/* of the CDKTYPE environment variable (which will be set to "CSI",  */
/* "SSI", or "CLIENT".                                               */
/*********************************************************************/
#define TRMSG(...) \
do \
{ \
    if (global_cdktrace_flag) \
    { \
        char trFileNamE[]  = __FILE__; \
        char trFuncNamE[]  = SELF; \
        int  trLineNuM     = __LINE__; \
        char trMsG[STRC_MAX_LINE_SIZE + 1]; \
        sprintf(trMsG, __VA_ARGS__); \
        srvtrcc(trFileNamE, \
                trFuncNamE, \
                trLineNuM, \
                TRCI_CDKTYPE, \
                NULL, \
                0, \
                trMsG); \
    } \
}while(0)


#define TRMEM(storageAddresS, storageLengtH, ...) \
do \
{ \
    if (global_cdktrace_flag) \
    { \
        char trFileNamE[]  = __FILE__; \
        char trFuncNamE[]  = SELF; \
        int  trLineNuM     = __LINE__; \
        char trMsG[STRC_MAX_LINE_SIZE + 1]; \
        sprintf(trMsG, __VA_ARGS__); \
        srvtrcc(trFileNamE, \
                trFuncNamE, \
                trLineNuM, \
                TRCI_CDKTYPE, \
                (char*) storageAddresS, \
                storageLengtH, \
                trMsG); \
    } \
}while(0)


/*********************************************************************/
/* TRMSGI and TRMEMI are for writing output to the trace.file.       */
/*                                                                   */
/* These macros call srvtrcc using the specified trace index value.  */
/* (see the comments above about the CDKTRACE evironment variable).  */
/*********************************************************************/
#define TRMSGI(trIndeX, ...) \
do \
{ \
    if (global_cdktrace_flag) \
    { \
        char trFileNamE[]  = __FILE__; \
        char trFuncNamE[]  = SELF; \
        int  trLineNuM     = __LINE__; \
        char trMsG[STRC_MAX_LINE_SIZE + 1]; \
        sprintf(trMsG, __VA_ARGS__); \
        srvtrcc(trFileNamE, \
                trFuncNamE, \
                trLineNuM, \
                trIndeX, \
                NULL, \
                0, \
                trMsG); \
    } \
}while(0)


#define TRMEMI(trIndeX, storageAddresS, storageLengtH, ...) \
do \
{ \
    if (global_cdktrace_flag) \
    { \
        char trFileNamE[]  = __FILE__; \
        char trFuncNamE[]  = SELF; \
        int  trLineNuM     = __LINE__; \
        char trMsG[STRC_MAX_LINE_SIZE + 1]; \
        sprintf(trMsG, __VA_ARGS__); \
        srvtrcc(trFileNamE, \
                trFuncNamE, \
                trLineNuM, \
                trIndeX, \
                (char*) storageAddresS, \
                storageLengtH, \
                trMsG); \
    } \
}while(0)


/*********************************************************************/
/* LOGMSG: Log a message to the log.file.                            */
/*********************************************************************/
#define LOGMSG(statuS, ...) \
do \
{ \
    if (global_cdklog_flag) \
    { \
        char logFileNamE[]  = __FILE__; \
        int  logLineNuM     = __LINE__; \
        char logMsG[SLOG_MAX_LINE_SIZE + 1]; \
        sprintf(logMsG, __VA_ARGS__); \
        srvlogc(logFileNamE, \
                logLineNuM, \
                statuS, \
                0, \
                NULL, \
                0, \
                LOGI_EVENT_MESSAGE, \
                NULL, \
                NULL, \
                logMsG); \
        TRMSG(__VA_ARGS__); \
    } \
}while(0)


/*********************************************************************/
/* LOG_XAPI_SEND and LOG_XAPI_RECV: Log XAPI (XML) transactions to   */
/* log.file.                                                         */
/*********************************************************************/
#define LOG_XAPI_SEND(statuS, packetID, hostnamE, porT, xmlBuffeR, xmlLengtH, ...) \
do \
{ \
    if (global_cdklog_flag) \
    { \
        char logFileNamE[]  = __FILE__; \
        int  logLineNuM     = __LINE__; \
        char portStrinG[6]; \
        char logMsG[STRC_MAX_LINE_SIZE + 1]; \
        sprintf(portStrinG, "%d", porT); \
        sprintf(logMsG, __VA_ARGS__); \
        srvlogc(logFileNamE, \
                logLineNuM, \
                statuS, \
                packetID, \
                xmlBuffeR, \
                xmlLengtH, \
                LOGI_EVENT_XAPI_SEND, \
                hostnamE, \
                portStrinG, \
                logMsG); \
        TRMEM(xmlBuffeR, xmlLengtH, __VA_ARGS__); \
    } \
}while(0)


#define LOG_XAPI_RECV(statuS, packetID, hostnamE, porT, xmlBuffeR, xmlLengtH, ...) \
do \
{ \
    if (global_cdklog_flag) \
    { \
        char logFileNamE[]  = __FILE__; \
        int  logLineNuM     = __LINE__; \
        char portStrinG[6]; \
        char logMsG[STRC_MAX_LINE_SIZE + 1]; \
        sprintf(portStrinG, "%d", porT); \
        sprintf(logMsG, __VA_ARGS__); \
        srvlogc(logFileNamE, \
                logLineNuM, \
                statuS, \
                packetID, \
                xmlBuffeR, \
                xmlLengtH, \
                LOGI_EVENT_XAPI_RECV, \
                hostnamE, \
                portStrinG, \
                logMsG); \
        TRMEM(xmlBuffeR, xmlLengtH, __VA_ARGS__); \
    } \
}while(0)


/*********************************************************************/
/* LOG_HTTP_SEND and LOG_HTTP_RECV: Log HTTP (XML) transactions to   */
/* log.file.                                                         */
/*********************************************************************/
#define LOG_HTTP_SEND(statuS, packetID, hostnamE, porT, xmlBuffeR, xmlLengtH, ...) \
do \
{ \
    if (global_cdklog_flag) \
    { \
        char logFileNamE[]  = __FILE__; \
        int  logLineNuM     = __LINE__; \
        char portStrinG[6]; \
        char logMsG[STRC_MAX_LINE_SIZE + 1]; \
        sprintf(portStrinG, "%d", porT); \
        sprintf(logMsG, __VA_ARGS__); \
        srvlogc(logFileNamE, \
                logLineNuM, \
                statuS, \
                packetID, \
                xmlBuffeR, \
                xmlLengtH, \
                LOGI_EVENT_HTTP_SEND, \
                hostnamE, \
                portStrinG, \
                logMsG); \
        TRMEM(xmlBuffeR, xmlLengtH, __VA_ARGS__); \
    } \
}while(0)


#define LOG_HTTP_RECV(statuS, packetID, hostnamE, porT, xmlBuffeR, xmlLengtH, ...) \
do \
{ \
    if (global_cdklog_flag) \
    { \
        char logFileNamE[]  = __FILE__; \
        int  logLineNuM     = __LINE__; \
        char portStrinG[6]; \
        char logMsG[STRC_MAX_LINE_SIZE + 1]; \
        sprintf(portStrinG, "%d", porT); \
        sprintf(logMsG, __VA_ARGS__); \
        srvlogc(logFileNamE, \
                logLineNuM, \
                statuS, \
                packetID, \
                xmlBuffeR, \
                xmlLengtH, \
                LOGI_EVENT_HTTP_RECV, \
                hostnamE, \
                portStrinG, \
                logMsG); \
        TRMEM(xmlBuffeR, xmlLengtH, __VA_ARGS__); \
    } \
}while(0)


/*********************************************************************/
/* LOG_ACSAPI_SEND and LOG_ACSAPI_RECV: Log ACSAPI transactions      */
/* beginning with the IPC_HEADER to the log.file.                    */
/*********************************************************************/
#define LOG_ACSAPI_SEND(statuS, acsapiBuffeR, acsapiLengtH, ...) \
do \
{ \
    if (global_cdklog_flag) \
    { \
        char logFileNamE[]  = __FILE__; \
        int  logLineNuM     = __LINE__; \
        IPC_HEADER *pIpc_HeadeR = (IPC_HEADER*) acsapiBuffeR; \
        REQUEST_HEADER *pRequest_HeadeR = (REQUEST_HEADER*) acsapiBuffeR; \
        MESSAGE_HEADER *pMessage_HeadeR = &(pRequest_HeadeR->message_header); \
        char returnPidStrinG[6]; \
        char logMsG[SLOG_MAX_LINE_SIZE + 1]; \
        sprintf(returnPidStrinG, "%d", pIpc_HeadeR->return_pid); \
        sprintf(logMsG, __VA_ARGS__); \
        srvlogc(logFileNamE, \
                logLineNuM, \
                statuS, \
                (int) pMessage_HeadeR->packet_id, \
                acsapiBuffeR, \
                acsapiLengtH, \
                LOGI_EVENT_ACSAPI_SEND, \
                pIpc_HeadeR->return_socket, \
                returnPidStrinG, \
                logMsG); \
        TRMEM(acsapiBuffeR, acsapiLengtH, __VA_ARGS__); \
    } \
}while(0)


#define LOG_ACSAPI_RECV(statuS, acsapiBuffeR, acsapiLengtH, ...) \
do \
{ \
    if (global_cdklog_flag) \
    { \
        char logFileNamE[]  = __FILE__; \
        int  logLineNuM     = __LINE__; \
        IPC_HEADER *pIpc_HeadeR = (IPC_HEADER*) acsapiBuffeR; \
        REQUEST_HEADER *pRequest_HeadeR = (REQUEST_HEADER*) acsapiBuffeR; \
        MESSAGE_HEADER *pMessage_HeadeR = &(pRequest_HeadeR->message_header); \
        char returnPidStrinG[6]; \
        char logMsG[SLOG_MAX_LINE_SIZE + 1]; \
        sprintf(returnPidStrinG, "%d", pIpc_HeadeR->return_pid); \
        sprintf(logMsG, __VA_ARGS__); \
        srvlogc(logFileNamE, \
                logLineNuM, \
                statuS, \
                (int) pMessage_HeadeR->packet_id, \
                acsapiBuffeR, \
                acsapiLengtH, \
                LOGI_EVENT_ACSAPI_RECV, \
                pIpc_HeadeR->return_socket, \
                returnPidStrinG, \
                logMsG); \
        TRMEM(acsapiBuffeR, acsapiLengtH, __VA_ARGS__); \
    } \
}while(0)


/*********************************************************************/
/* LOG_CSI_PACKET: Log CSI RPC/API packets to the log.file.          */
/*********************************************************************/
#define LOG_CSI_PACKET(statuS, csiBuffeR, ssiIdentifieR, \
                       ipStrinG, portStrinG, sourceStrinG, ...) \
do \
{ \
    if (global_cdklog_flag) \
    { \
        char  logFileNamE[]  = __FILE__; \
        int   logLineNuM     = __LINE__; \
        int   logIndeX = LOGI_EVENT_CSI_SEND; \
        int   logLengtH; \
        char *logBuffeR; \
        char  logMsG[SLOG_MAX_LINE_SIZE + 1]; \
        CSI_MSGBUF *pCsi_MsgbuF = (CSI_MSGBUF*) csiBuffeR; \
        logBuffeR = CSI_PAK_NETDATAP(pCsi_MsgbuF); \
        logLengtH = pCsi_MsgbuF->size; \
        if (memcmp(sourceStrinG, "From", 4) == 0) \
        { \
            logIndeX = LOGI_EVENT_CSI_RECV; \
        } \
        sprintf(logMsG, __VA_ARGS__); \
        srvlogc(logFileNamE, \
                logLineNuM, \
                statuS, \
                (int) ssiIdentifieR, \
                logBuffeR, \
                logLengtH, \
                logIndeX, \
                ipStrinG, \
                portStrinG, \
                logMsG); \
        TRMEM(logBuffeR, logLengtH, __VA_ARGS__); \
    } \
}while(0)


/*********************************************************************/
/* SETENV: Use either putenv() or setenv() depending upon compiler.  */
/*********************************************************************/
#if defined(SOLARIS) || defined(LINUX)
    #define SETENV(envNamE, envValuE) \
    do \
    { \
        auto char putEnvStrinG[256]; \
        sprintf(putEnvStrinG, "%s=%s", envNamE, envValuE); \
        putenv(putEnvStrinG); \
    }while(0)
#else
    #define SETENV(envNamE, envValuE) \
    do \
    { \
        setenv(envNamE, envValuE, 1); \
    }while(0)
#endif


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
extern int srvlogc(char *fileName,
                   int   lineNumber,
                   int   status,
                   int   packetId,
                   char *packetAddress,
                   int   packetLength,
                   int   logIndex,
                   char *packetAddressString1,
                   char *packetAddressString2,
                   char *logMsg);

extern int srvtrcc(char *fileName,
                   char *funcName,
                   int   lineNumber,
                   int   traceIndex,
                   char *storageAddress,
                   int   storageLength,
                   char *traceMsg);

extern void srvvars(char *moduleType);


/*********************************************************************/
/* The following prototypes conform to the "registered" trace and    */
/* log output functions: cl_el_log_evnt; cl_el_log_trace; and        */
/* cl_el_trace.                                                      */
/*********************************************************************/
extern void srvlogc_string_writer(const char *logMsg);

extern void srvtrcc_string_writer(const char *traceMsg);

#endif                                           /* SRVCOMMON_HEADER */



