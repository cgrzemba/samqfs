/** HEADER FILE PROLOGUE *********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      smccodes.h                                       */
/** Description:    XAPI client external XAPI return codes           */
/**                 and message strings (as of ELS710).              */
/**                                                                  */
/**    ---------->>>>> NOTE NOTE NOTE <<<<<-------------             */
/**    ->> These XAPI codes should periodically be   <<-             */
/**    ->> synchronized with the most current        <<-             */
/**    ->> versions maintained in the most current   <<-             */
/**    ->> SMC/HSC/ELS project in smccodes.h.        <<-             */
/**    -------------------------------------------------             */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     05/11/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/** END PROLOGUE *****************************************************/

#ifndef SMCCODES_HEADER
#define SMCCODES_HEADER

#define UUI_MAX_ERR_MSG_LEN        48  /* Max len of reason string   */
#define UUI_MAX_ERR_DETAIL_LEN    512  /* Max len of reason detail   */

                                                               
/*********************************************************************/
/* XAPI Return Codes:                                                */
/*===================================================================*/
/* The following return codes are set by the XAPI server component   */
/* and returned as <uui_return_code> XML content.                    */
/*********************************************************************/
#define UUI_MORE_DATA              1   /* More data to return        */
#define UUI_EOF                    2   /* All data was returned (EOF)*/
#define UUI_READ_TIMEOUT           3   /* UUI read timeout; read     */
/*                                        ...timeout occurred before */
/*                                        ...any more data available */
#define UUI_CMD_WARNING            4   /* Command issued warning msg */
#define UUI_CMD_ERROR              8   /* Command issued error msg   */
#define UUI_CMD_FATAL              12  /* Command issued fatal error */
#define UUI_CMD_ABEND              16  /* ABEND detected.            */

/*********************************************************************/
/* XAPI Return Codes:                                                */
/*===================================================================*/
/* The following reason codes are set by the XAPI server component   */
/* and returned as <uui_reason_code> XML content.                    */
/*********************************************************************/

/*********************************************************************/
/* NRC (HSC XAPI Component) Reason Codes:                            */
/*===================================================================*/
/* NOTE: The reason codes prior to UUI_SMC_INACTIVE must be kept     */
/* synchronized with the reason codes in the HSC MACRO NRC.mac.      */
/*********************************************************************/
#define UUI_REQUEST_LENGTH_ERROR   4   /* Request length error       */
#define UUI_RC1                    4,  "Request length error"        ,\
        "An HSC/VTCS UUI transaction had a command verb with an "    \
        "invalid length, or the total command length exceeded the "  \
        "system maximum."
#define UUI_NO_EXITS_SPECIFIED     8   /* No exits specified         */
#define UUI_RC2                    8,  "No exits for server UUI"     ,\
        "No callback exits were found for an HSC or VTCS UUI request. "\
        "If this is a programmatic VTCS PGMI request, then correct " \
        "the input control block.  Otherwise, this is an internal "  \
        "error and should be reported to StorageTek Support."
#define UUI_PARSE_ERROR            12  /* Request parse error        */
#define UUI_RC3                    12, "Command parse error"         ,\
        "The HSC/VTCS UUI transaction contained an invalid format or "\
        "syntax.  An accompanying exception message explains the "    \
        "specific error condition."
#define UUI_NOT_FOUND              16  /* UUI request not found      */
#define UUI_RC10                   16, "UUI request not found"       ,\
        "An HSC/VTCS UUI transaction had a command that was not found "\
        "in the command request table."
#define UUI_REQUEST_ORIGIN_ERROR   20  /* Request origin             */
#define UUI_RC4                    20, "Command origin error"        ,\
        "An HSC/VTCS UUI transaction was received from an input "     \
        "source from which it is not permitted, such as a report "    \
        "request on a console command."
#define UUI_NOT_AUTHORIZED         24  /* LOADLIB not authorized     */
#define UUI_RC5                    24, "LOADLIB not authorized"      ,\
        "An HSC/VTCS UUI transaction that requires an authorized "    \
        "load library was processed from a load library that was not "\
        "APF-authorized."
#define UUI_INCOMPATIBLE_RELEASE   28  /* Incompatible LVT level     */
#define UUI_RC6                    28, "Server release incompatible" ,\
        "An HSC/VTCS UUI transaction was submitted using the SLUADMIN "\
        "utility program, but the release level of the utility and "  \
        "the release level of the subsystem did not match."
#define UUI_ADV_MANAGEMENT         32  /* Advanced Management Feature*/
#define UUI_RC7                    32, "Adv management feature required",\
        "A VTCS UUI transaction that requires the Advanced Management "\
        "feature was submitted in a system that did not have the "   \
        "feature."
#define UUI_REQUEST_CANCELLED      36  /* UUI request cancelled      */
#define UUI_RC8                    36, "Request cancelled"           ,\
        "An HSC UUI transaction was cancelled while in progress. "    \
        "This may have been caused by a user-generated UUI CANCEL "   \
        "request or by HSC subsystem termination."
#define UUI_MALFORMED_XML          40  /* Server XML format error    */
#define UUI_RC9                    40, "Malformed XML from server"   ,\
        "An HSC/VTCS UUI request created XML that was detected to "   \
        "be malformed, such as unmatched header and trailer tags. "   \
        "This is a system internal error and should be reported to "  \
        "StorageTek Support."    

/* Note: 44 used in HSC 6.2.                                         */

#define UUI_INVALID_PARAMETER      48  /* Invalid parameter value    */
#define UUI_RC11                   48, "Invalid parameter value"     ,\
        "An HSC/VTCS UUI request contained an input parameter that "  \
        "had an invalid value.  An accompanying exception message "   \
        "explains the specific error."
#define UUI_NO_VSM                 52  /* VSM inactive               */
#define UUI_RC12                   52, "VSM not active on server"    ,\
        "A VTCS UUI request was received for a system where VTCS was "\
        "not available."  
#define UUI_CSV_PARM_ERROR         56  /* CSV parameter error        */
#define UUI_RC13                   56, "Error in CSV parameters"     ,\
        "A UUI request specified CSV output, but the CSV parameters " \
        "were invalid or inconsistent."
#define UUI_CSV_FORMAT_ERROR       60  /* CSV parameter format error */
#define UUI_RC14                   60, "Error in CSV parameter format",\
        "An error occurred during parsing of the input CSV parameters. "\
        "An accompanying exception message explains the specific error."
#define UUI_REMOTE_FILE_REQ_ERROR  64  /* Remote file not supported  */
#define UUI_RC15                   64, "Remote file I/O not supported",\
        "An HSC/VTCS UUI request was received from a remote host. "   \
        "The request included an output flat file, which is not "     \
        "supported from a remote host."
#define UUI_SERVICE_LEVEL_ERROR    68  /* HSC not at required level  */
#define UUI_RC16                   68, "Not at required service level",\
        "An HSC/VTCS UUI request was received during HSC subsystem "  \
        "initialization or termination.  The request required HSC "   \
        "full service level, or VTCS initialized, and the subsystem " \
        "had not yet reached that point in its initialization process."
#define UUI_USER_NOT_AUTHORIZED    72  /* User is not authorized     */
#define UUI_RC17                   72, "User not authorized"         ,\
        "An HSC/VTCS UUI request was received which was not permitted "\
        "based on system security settings."
#define UUI_TOKEN_NOT_FOUND        76  /* Token does not exist       */
#define UUI_RC18                   76, "Specified UUI task not found",\
        "A UUI partial output or cancel request was received that "  \
        "specified a UUI token that was not found on the system."
#define UUI_RECON_ACTIVE           80  /* Dynamic recon active       */
#define UUI_RC19                   80, "Dynamic reconfig active"     ,\
        "An HSC/VTCS UUI request that was dependent on the current " \
        "library configuration was received while a dynamic "        \
        "reconfiguration was in progress; the request is not "       \
        "processed."
#define UUI_TIMEOUT                84  /* Timeout limit exceeded     */
#define UUI_RC20                   84, "Transaction timeout occurred",\
        "An HSC UUI request was received with a transaction timeout "\
        "value (NCOMTIMC) that has expired before the completion of "\
        "the transaction."
         
/* Note: 88 previously license key error                             */
 
#define UUI_SUBTASK_ABEND          92  /* Subtask abended            */
#define UUI_RC22                   92, "Subtask abended"             ,\
        "An HSC/VTCS UUI request terminated abnormally.  Check logs "\
        "for an associated error."
#define UUI_FUNCTION_PROCESS_ERROR 96  /* Function process error     */
#define UUI_RC23                   96, "Function process error"      ,\
        "An HSC/VTCS UUI request failed to complete the requested "   \
        "function.  An accompanying exception message explains the "  \
        "specific error."

/*********************************************************************/
/* SMC UUI Reason Codes:                                             */
/*===================================================================*/
/* The following reason codes are set by the SMC UUI Component       */
/* when processing the UUI request or the UUI response.              */
/*********************************************************************/
#define UUI_SMC_INACTIVE           300 /* SMC not active             */
#define UUI_RC25                   300, "SMC not active"             ,\
        "A UUI request was received that required SMC to be active, " \
        "but SMC was found to be inactive."
#define UUI_REQUEST_HDR_ERROR      301 /* UUI request header error   */
#define UUI_RC110                  301, "UUI request header error"   ,\
        "A UUI request sent to a remote SMC server had a format error "\
        "in its request header.  This is a system internal error and "\
        "should be reported to StorageTek Support."
#define UUI_TAPEPLEX_INACTIVE      302 /* Tapeplex inactive          */
#define UUI_RC111                  302, "TapePlex inactive due to error",\
        "A request directed to a specific TapePlex could not be "     \
        "processed due to an error.  A previous message explains "    \
        "the specific error."
#define UUI_NOT_SUPPORTED          304 /* UUI unsupported in release */
#define UUI_RC26                   304, "UUI unsupported by server release",\
        "A UUI request was received for a release that does not support "\
        "UUI transactions (pre 6.2)." 
#define UUI_XAPI_NOT_SUPPORTED     305 /* XAPI unsupported in release*/
#define UUI_RC105                  305, "XAPI unsupported by server release",\
        "An XAPI request was received for a release that does not support "\
        "XAPI transactions (pre 7.1)."
#define UUI_REQUEST_TYPE_ERROR     308 /* Request type invalid       */
#define UUI_RC27                   308, "Invalid request type specified",\
        "A programmatic UUI request was received with an invalid "   \
        "type (NCOMTYPE)."
#define UUI_TOKEN_ERROR            312 /* Error acquiring new token  */
#define UUI_RC28                   312, "Could not acquire task token",\
        "An SMC UUI request was unable to assign a request token. "  \
        "This is a system internal error and should be reported to " \
        "StorageTek Support."
#define UUI_LOAD_ERROR             316 /* Module load error          */
#define UUI_RC29                   316, "Error loading UUI module"   ,\
        "A module required for UUI processing could not be loaded. " \
        "If this error is returned from a user-written programmatic "\
        "interface, the module may not have access to the ELS load " \
        "library."
#define UUI_ATTACH_ERROR           320 /* Task attach error          */
#define UUI_RC30                   320, "Error attaching UUI subtask",\
        "Attach of a UUI subtask failed.  This is a system internal "\
        "error and should be reported to StorageTek Support."
#define UUI_NO_ELIGIBLE_TAPEPLEX   324 /* No active TAPEPLEX for UUI */
#define UUI_RC31                   324, "No eligible TAPEPLEX for request",\
        "A UUI request for a TapePlex list found no active TapePlexes."
#define UUI_NO_TAPEPLEX_SPECIFIED  328 /* No TAPEPLEX or LVT addr    */
/*                                        ...specified in NCSCOMM    */
#define UUI_RC32                   328, "No TAPEPLEX specified in request",\
        "A UUI request was made, but no TapePlex name was specified in "\
        "the NCSCOMM parameter list."
#define UUI_TAPEPLEX_NOT_FOUND     332 /* TAPEPLEX name not found    */
#define UUI_RC33                   332, "Specified TAPEPLEX not found",\
        "A UUI request was made for a TapePlex name that was not "   \
        "defined to the SMC."
#define UUI_TAPEPLEX_DISABLED      336 /* TAPEPLEX is disabled       */
#define UUI_RC34                   336, "Specified TAPEPLEX is disabled",\
        "A UUI request was made for a TapePlex name that was defined "\
        "to SMC but was disabled by the user."
#define UUI_TAPEPLEX_CSC           340 /* TAPEPLEX is MVS/CSC        */
#define UUI_RC35                   340, "Specified TAPEPLEX is MVS/CSC",\
        "A UUI request was made for a TapePlex name that was defined "\
        "as an MVS/CSC subsystem.  MVS/CSC does not support UUI." 
#define UUI_INVALID_TAPEPLEX_ADDR  344 /* Invalid TAPEPLEX addr      */
#define UUI_RC36                   344, "Invalid TAPELEX address specified",\
        "A UUI request was made with a specified TapePlex address, " \
        "but the address did not point to a valid TapePlex structure."
#define UUI_INVALID_PLIST          348 /* Invalid NCSCOMM PLIST      */
#define UUI_RC37                   348, "Error in NCSCOMM parameters",\
        "An NCSCOMM parameter list contained conflicting parameters. "\
        "This return code may indicate that an attempt was made to "  \
        "cancel a UUI request that was not currently in progress."
#define UUI_INVALID_OUTPUT_PLIST   352 /* Invalid PLIST for OUTPUT   */
#define UUI_RC38                   352, "Error in NCSCOMM output parameters",\
        "An NCSCOMM parameter list requested partial output but a new "\
        "NCSCOMM request was not built for the partial output."
#define UUI_INCONSISTENT_CSV       356 /* Inconsistent CSV parameters*/
#define UUI_RC107                  356, "Inconsistent CSV parameters",\
        "A UUI request for CSV output was received that requested CSV "\
        "output format without a CSV specification block, or had a CSV "\
        "specification block but did not request CSV output."
#define UUI_UNMATCHED_TAPEPLEX     360 /* Unmatched tapeplex name    */
#define UUI_RC109                  360, "Unmatched TapePlex name"    ,\
        "A UUI request was received for a TapePlex UUI command, but "\
        "the specified TapePlex name did not match that in the HSC CDS."
#define UUI_NO_LOCAL_HSC           364 /* No local HSC for LOCALHSC  */
#define UUI_RC39                   364, "No HSC found for LOCALHSC request",\
        "A UUI request was made with a TapePlex name of LOCALHSC, but "\
        "no TapePlex was found with a local subsystem."
#define UUI_NO_OUTPUT_TYPE         368 /* No valid UUI output type   */
#define UUI_RC108                  368, "No valid UUI output type"   ,\
        "A UUI request was made without a valid UUI output type "    \
        "(NCOMOFMT)."
#define UUI_XML_PARSE_ERROR        372 /* Parse error for remote XML */
#define UUI_RC40                   372, "XML response parse error"   ,\
        "An XML parse error occurred attempting to parse the UUI XML "\
        "sent to a server system or returned to a client system."
#define UUI_XML_NO_START_TAG       376 /* XML start tag not found    */ 
#define UUI_RC41                   376, "XML start tag not found"    ,\
        "An XML transaction received by the server did not begin " \
        "with <libtrans>, or an XML transaction returned to the client "\
        "did not begin with <libreply>."
#define UUI_XML_NO_END_TAG         380 /* XML end tag not found      */ 
#define UUI_RC42                   380, "XML end tag not found"      ,\
        "An XML transaction received by the server did not end with "\
        "</libtrans>, or an XML transaction returned to the client " \
        "did not end with </libreply>."
#define UUI_INVOKE_LOCAL_HSC       388 /* Use local HSC for utility  */
#define UUI_RC43                   388, "Local HSC is available for utility",\
        "This code is used internally only."
#define UUI_RESPONSE_LEN_ZERO      392 /* Response length is 0       */
#define UUI_RC44                   392, "No response received"       ,\
        "An XML transaction received a response containing no data."
#define UUI_RESPONSE_LEN_OVERFLOW  396 /* Response length too large  */
#define UUI_RC45                   396, "Response length overflow"   ,\
        "While processing buffers from a UUI transaction SMC detected "\
        "a condition where the corresponding ending XML tag was not " \
        "in the same buffer as the start tag nor in the next buffer."

/*********************************************************************/
/* SMC Command Processing Reason Codes:                              */
/*===================================================================*/
/* The following reason codes are set by the SMC Operator Command    */
/* component when processing an SMC UUI command.                     */
/*********************************************************************/
#define UUI_SMC_SERVICE_INACTIVE   400 /* Requested service inactive */
#define UUI_RC46                   400, "Requested service is inactive",\
        "An operation was requested that required an SMC service that "\
        "is not yet active at startup."
#define UUI_SMC_SERVICE_ERROR      404 /* SMC service error occurred */
#define UUI_RC47                   404, "SMC service error occurred" ,\
        "An internal SMC error occurred and should be reported to "  \
        "StorageTek Support."
#define UUI_CMD_FILE_NOT_FOUND     408 /* File not found             */
#define UUI_RC48                   408, "Command file not found"     ,\
        "An SMC READ command requested a file that was not found."
#define UUI_CMD_FILE_IO_ERROR      412 /* File I/O error             */
#define UUI_RC49                   412, "Command file I/O error"     ,\
        "An I/O error occurred reading an SMC command file (SMCPARMS "\
        "or SMCCMDS) or a file specified by an SMC READ command."
#define UUI_CMD_DEPTH_ERROR        416 /* Max READ depth exceeded    */
#define UUI_RC50                   416, "Command READ depth exceeded",\
        "While processing an SMC READ command, embedded READ "        \
        "commands were found that exceeded the allowed READ stack "   \
        "depth.  This is assumed to be a recursive READ invocation." 
#define UUI_INCOMPATIBLE           420 /* SMCPCE release is not      */
/*                                        ...equal to SMCCVT release */
#define UUI_RC51                   420, "Utility incompatible with SMC subsystem",\
        "An attempt was made to run the SMCUUUI utility or a user "  \
        "written programmatic interface in which the batch job used "\
        "a different release level of SMC software than the SMC "    \
        "subsystem."
#define UUI_NO_DATA                424 /* No data for command        */
#define UUI_RC52                   424, "No data returned for request",\
        "A UUI request was processed with a read timeout value.  The "\
        "timeout period expired before data was received.  This "     \
        "reason code is accompanied by return code UUI_READ_TIMEOUT."
#define UUI_CLIENT_EARLY_SHUTDOWN  428 /* UUI client early shutdown  */
#define UUI_RC104                  428, "UUI client early shutdown"  ,\
        "A UUI request requested a transaction cancel."

/*********************************************************************/
/* SMC ASCOMM Reason Codes:                                          */
/*===================================================================*/
/* The following reason codes are set by the SMC ASCOMM Component    */
/* for MVS inter and intra address space communication.              */
/*********************************************************************/
#define UUI_SMC_TERMINATED         500 /* SMC STOP command issued    */
/*                                        ...or task cancelled       */ 
/*                                        ...during communication    */
#define UUI_RC53                   500, "SMC subsystem termination in progress",\
        "A request was executing when SMC termination was requested. "\
        "The request is terminated."
#define UUI_QAS_VERSION_ERROR      504 /* Wrong version of SMCQASP   */
#define UUI_RC54                   504, "ASCOMM version not supported",\
        "A version of the internal SMCQASP control block did not match "\
        "between the subsystem and a batch job.  This is a system "  \
        "internal error and should be reported to StorageTek Support."
#define UUI_QAS_QUEUE_ERROR        508 /* At max SMCQUEE queue size  */
#define UUI_RC55                   508, "ASCOMM could not queue request",\
        "An error occurred attempting to place an cross-memory  "    \
        "request on the queue.  This is a system internal error and "\
        "should be reported to StorageTek Support."
#define UUI_QAS_STORAGE_ERROR      512 /* No GETMAIN storage         */
#define UUI_RC56                   512, "ASCOMM GETMAIN storage error",\
        "An error occurred when acquiring storage for cross-memory "   \
        "communication control blocks.  This error is most likely "  \
        "caused by a general subsystem storage shortage."
#define UUI_QAS_DATASPACE_ERROR    516 /* No dataspace storage       */
#define UUI_RC57                   516, "ASCOMM dataspace storage error",\
        "An error occurred processing the dataspace storage used for "\
        "cross-memory communications.  This is a system internal "    \
        "error and should be reported to StorageTek Support."
#define UUI_QAS_INDEX_ERROR        520 /* Invalid ASCOMM module index*/
#define UUI_RC58                   520, "Invalid ASCOMM module index",\
        "A cross-memory communication request specified a module "   \
        "index that did not exist in the subsystem.  This is a "     \
        "system internal error and should be reported to StorageTek "\
        "Support." 
#define UUI_QAS_TOKEN_ERROR        524 /* QASTOKN acquisition error  */
#define UUI_RC59                   524, "Error acquiring ASCOMM token",\
        "An error occurred attempting to acquire a token for a cross-"\
        "memory communication.  This is a system internal error and "\
        "should be reported to StorageTek Support."
#define UUI_QAS_TIMEOUT            528 /* Response timeout occurred  */
#define UUI_RC60                   528, "ASCOMM initial response timeout",\
        "A timeout occurred in a transaction outside the SMC address "\
        "space while waiting for an acknowledgment from the SMC "     \
        "subsystem that a cross-memory transaction request was "      \
        "received."
#define UUI_QAS_ACK_TIMEOUT        532 /* Final ACK timeout occurred */
#define UUI_RC61                   532, "ASCOMM final response timeout",\
        "A timeout occurred in the SMC address space while waiting "  \
        "for an acknowledgment that a transaction in another address "\
        "space has received its returned data.  This error may be "   \
        "caused by a transaction outside the SMC address space that " \
        "was abnormally terminated."
#define UUI_QAS_MODULE_NOT_FOUND   536 /* Indexed module not found   */
#define UUI_RC62                   536, "ASCOMM module index not found",\
        "A cross-memory communication request specified a module "    \
        "index that existed but contained a NULL entry.  This is a "  \
        "system internal error and should be reported to StorageTek " \
        "Support."
#define UUI_QAS_ASYNCH_IN_PROGRESS 540 /* Async service already in   */
/*                                        ...progress, req bypassed  */
#define UUI_RC63                   540, "Async ASCOMM request in progress",\
        "An internal request for a TapePlex list with no output "    \
        "data was received while another TapePlex list was already " \
        "in progress; the request is not performed."
#define UUI_TAPEPLEX_TIMEOUT       544 /* TAPEPLEX (non-SMC) ASCOMM  */
/*                                        ...timeout occurred        */
#define UUI_RC64                   544, "TAPEPLEX ASCOMM timeout occurred",\
        "A timeout occurred in a cross-memory request to a TapePlex."
#define UUI_TAPEPLEX_ASCOMM_ERROR  548 /* TAPEPLEX (non-SMC) ASCOMM  */
/*                                        ...error occurred          */
#define UUI_RC65                   548, "TAPEPLEX ASCOMM error occurred",\
        "An unexpected error occurred in a cross-memory request to " \
        "a TapePlex."
#define UUI_TAPEPLEX_ASCOMM_ABEND  552 /* TAPEPLEX (non-SMC) ASCOMM  */
                                       /* ...abend occurred          */
#define UUI_RC66                   552, "TAPEPLEX ASCOMM abend occurred",\
        "An abend occurred while processing a cross-memory request " \
        "to a TapePlex."
 
/* Note: 556 previously QAS close error; no references found.        */
 
#define UUI_QAS_ABEND              560 /* ASCOMM task abended        */
#define UUI_RC112                  560, "ASCOMM worker task has abended",\
        "An SMC cross-memory worker task abended prior to freeing "   \
        "dataspace storage associated with a task."   

/*********************************************************************/
/* SMC Communication Component Return Codes:                         */
/*===================================================================*/
/* The following reason codes are set by the SMC TAPEPLEX            */
/* Communication for local or remote communications.                 */
/*********************************************************************/
#define UUI_NO_ACTIVE_COMMPATH     600 /* No active COMMPATH         */
#define UUI_RC67                   600, "No active COMMPATH for TAPEPLEX",\
        "A request was received for a TapePlex that currently had no "\
        "active local or remote communication path."
#define UUI_TAPEPLEX_INVALIDATED   604 /* TAPEPLEX was invalidated   */
#define UUI_RC68                   604, "SSCVT TAPEPLEX was invalidated",\
        "A TapePlex created at SMC startup from the SSCVT was marked "\
        "as invalid when a TAPEPLEX command was processed."
#define UUI_SUBSYS_INACTIVE        608 /* HSC/CSC subsystem inactive */
#define UUI_RC69                   608, "TAPEPLEX subsystem is inactive",\
        "The subsystem associated with a TAPEPLEX communication path "\
        "is currently inactive or is being terminated."
#define UUI_SUBSYS_ERROR           612 /* HSC/CSC not valid          */
#define UUI_RC70                   612, "TAPEPLEX subsystem not valid",\
        "SMC was unable to access expected TapePlex subsystem control "\
        "blocks."
#define UUI_SUBSYS_NOT_FOUND       616 /* HSC/CSC not on SSCVT       */
#define UUI_RC71                   616, "TAPEPLEX subsystem not found",\
        "The requested TapePlex subsystem was not found on the SSCVT "\
        "subsystem list."
#define UUI_SUBSYS_REL_ERROR       620 /* HSC/CSC release error      */
#define UUI_RC72                   620, "TAPEPLEX subsystem release error",\
        "The release level of the TapePlex subsystem is incompatible "\
        "with the current SMC subsystem."
#define UUI_VLE_SUBSYS             624 /* TAPEPLEX is a VLE subsys   */
#define UUI_RC114                  624, "TAPEPLEX is a VLE subsystem",\
        "A request was received that is not a valid type for a "     \
        "VLE (Virtual Library Extension)."

/*********************************************************************/
/* The following are set by the SMC Communication and HTTP Sever     */
/* Component TCP/IP routines.                                        */  
/*********************************************************************/
#define UUI_REMOTE_REQUEST_ERROR   700 /* Invalid remote transaction */
#define UUI_RC73                   700, "Remote request specification error",\
        "The SMC client TCP/IP manager received a request with no "  \
        "specified data to send.  This is a system internal error "  \
        "and should be reported to StorageTek Support."
#define UUI_CGI_NAME_NOT_SPECIFIED 704 /* CGI module not specified   */
#define UUI_RC74                   704, "CGI POST name not specified",\
        "The SMC client TCP/IP manager received a request that did "  \
        "not specify a CGI routine to process the request.  This is " \
        "a system internal error and should be reported to StorageTek "\
        "Support."
#define UUI_IP_SETSOCKPARM_ERROR   708 /* TCPIP setsockparm() error  */
#define UUI_RC75                   708, "IP setsockparm() error"     ,\
        "A TCP/IP setsockparm() request failed. This is a system "   \
        "internal error and should be reported to "  \
        "StorageTek Support."
#define UUI_IP_SOCKET_ERROR        712 /* TCPIP socket() error       */
#define UUI_RC76                   712, "IP socket() error"          ,\
        "An error occurred when opening a TCP/IP socket."
#define UUI_IP_SETSOCKOPT_ERROR    716 /* TCPIP setsockopt() error   */
#define UUI_RC77                   716, "IP setsockopt() error"      ,\
        "A TCP/IP setsockopt() request failed. This is a system "     \
        "internal error and should be reported to StorageTek Support."
#define UUI_IP_NO_FREE_PORT_ERROR  720 /* No free port (SMCCTCPP)    */
#define UUI_RC78                   720, "IP no free ports in PORTRANGE",\
        "On an SMC client system with a fixed port range specified, "\
        "no free ports were available to send a transaction after "  \
        "waiting and retrying."
#define UUI_IP_BIND_ERROR          724 /* TCPIP bind() error         */
#define UUI_RC79                   724, "IP bind() error"            ,\
        "A TCP/IP bind() request failed."
#define UUI_IP_CONNECT_ERROR       728 /* TCPIP connect() error      */
#define UUI_RC80                   728, "IP connect() error"         ,\
        "A TCP/IP connect() request failed."
#define UUI_IP_SEND_ERROR          732 /* TCPIP send() error         */
#define UUI_RC81                   732, "IP send() error"            ,\
        "A TCP/IP send() request failed.  The most common reason for "\
        "this error is no receiver for the request, that is, the SMC "\
        "HTTP server on the receiving host is inactive."
#define UUI_IP_RECV_ERROR          736 /* TCPIP recv() error         */
#define UUI_RC82                   736, "IP recv() error"            ,\
        "A TCP/IP recv() request failed."
#define UUI_IP_TIMEOUT             740 /* TCPIP timeout occurred     */
#define UUI_RC83                   740, "IP timeout occurred"        ,\
        "A timeout occurred while sending or receiving data via TCP/IP. "\
        "Depending on the type of transaction, the timeout value "   \
        "is fixed for the system or set on the SERVER command."
#define UUI_IP_LISTEN_ERROR        744 /* TCPIP listen() error       */
#define UUI_RC84                   744, "IP listen() error"          ,\
        "A TCP/IP listen() request failed; this error is reported by "\
        "the SMC HTTP server." 
#define UUI_IP_GETCLIENTID_ERROR   748 /* TCPIP getclientid() error  */
#define UUI_RC85                   748, "IP getclientid() error"     ,\
        "A TCP/IP getclientid() request failed; this error is reported "\
        "by the SMC HTTP server." 
#define UUI_IP_ACCEPT_ERROR        752 /* TCPIP accept() error       */
#define UUI_RC86                   752, "IP accept() error"          ,\
        "A TCP/IP accept() request failed; this error is reported by "\
        "the SMC HTTP server." 
#define UUI_IP_GIVESOCKET_ERROR    756 /* TCPIP givesocket() error   */
#define UUI_RC87                   756, "IP givesocket() error"      ,\
        "A TCP/IP givesocket() request failed; this error is reported "\
        "by the SMC HTTP server." 
#define UUI_IP_TAKESOCKET_ERROR    760 /* TCPIP takesocket() error   */
#define UUI_RC88                   760, "IP takesocket() error"      ,\
        "A TCP/IP takesocket() request failed; this error is reported "\
        "by the SMC HTTP server." 
#define UUI_IP_INITAPI_ERROR       761 /* TCPIP initapi() error      */
#define UUI_RC106                  761, "IP initapi() error"         ,\
        "A TCP/IP initapi() request failed."
#define UUI_INVALID_HOST_NAME      762 /* Server invalid host name   */
#define UUI_RC95                   762, "Invalid host name for server",\
        "The HOSTname parameter on an SMC SERVER command could not "  \
        "be resolved by TCP/IP."
#define UUI_TCPIP_NOT_ACTIVE       763 /* TCP/IP is not active       */
#define UUI_RC115                  763, "TCP/IP is not active"       ,\
        "TCP/IP is not currently active on the SMC host.  If SMC is " \
        "resolving a SERVER HOSTname when TCP/IP is inactive, the "   \
        "resolution is delayed until TCP/IP becomes active."
#define UUI_IP_GETSOCKNAME_ERROR   764 /* IP getsockname() error     */
#define UUI_RC100                  764, "IP getsockname() error"     ,\
        "A TCP/IP getsockname() request failed; this error is reported "\
        "by the SMC HTTP server." 
#define UUI_IP_NTOP_PTON_ERROR     768 /* IP NTOP/PTON error         */
#define UUI_RC101                  768, "IP address translation error",\
        "A TCP/IP NTOP or PTON request failed."
#define UUI_IP_SELECTEX_ERROR      772 /* IP selectex() error        */
#define UUI_RC102                  772, "IP selectex() error"        ,\
        "A TCP/IP selectex() request failed."
#define UUI_SHUTDOWN_NO_SOCKET     776 /* No socket for shutdown     */
#define UUI_RC103                  776, "No socket for client shutdown",\
        "When attempting to shut down a TCP/IP socket, the socket "   \
        "was not active."
#define UUI_SEND_EXCEPTION_ERROR   780 /* SO_KEEPALIVE send error    */
#define UUI_RC21                   780, "Error detected during send probe",\
        "During a send wait, the FDS exception set was flagged indicating " \
        "that the end-to-end connection is no longer available."
#define UUI_RECV_EXCEPTION_ERROR   784 /* SO_KEEPALIVE recv error    */
#define UUI_RC24                   784, "Error detected during recv probe",\
        "During a recv wait, the FDS exception set was flagged indicating " \
        "that the end-to-end connection is no longer available."

/*********************************************************************/
/* The following are set by the SMC CGI routines or the HTTP         */
/* server routines.                                                  */
/*********************************************************************/
#define UUI_HTTP_TASK_LIMIT        800 /* HTTP task limit exceeded   */
#define UUI_RC89                   800, "No free HTTP tasks available",\
        "When attempting to start a new HTTP server task, the "       \
        "current task limit was exceeded.  The request is retried."
#define UUI_HTTP_CGI_MOD_NOT_FOUND 804 /* CGI module not found       */
#define UUI_RC90                   804, "CGI module not found"       ,\
        "The SMC HTTP server received a request for an unknown CGI "  \
        "module name."
#define UUI_HTTP_CGI_MOD_ABEND     808 /* CGI module abended         */
#define UUI_RC91                   808, "CGI module abended"         ,\
        "An SMC HTTP server CGI module abended."
#define UUI_HTTP_NOT_AUTHORIZED    812 /* HTTP server not authorized */
#define UUI_RC92                   812, "HTTP server not authorized" ,\
        "The SMC HTTP server could not perform a function due to "    \
        "operating system authorization restrictions."
#define UUI_HTTP_NOT_SUPPORTED     816 /* CGI module not supported   */
#define UUI_RC93                   816, "Incompatible CGI module"    ,\
        "An error occurred attempting to call a CGI module.  This "    \
        "error may be caused by incompatible releases of the SMC or " \
        "HTTP server software."
#define UUI_CGI_RETURNED_ERROR     820 /* CGI module returned error  */
#define UUI_RC94                   820, "Error detected by CGI module",\
        "An SMC CGI module returned an unexpected error."
#define UUI_CGI_INPUT_ERROR        832 /* CGI input error            */
#define UUI_RC96                   832, "CGI input function error"   ,\
        "No data was received for an SMC HTTP server transaction."
#define UUI_CGI_OUTPUT_ERROR       836 /* CGI output error           */ 
#define UUI_RC97                   836, "CGI output function error",  \
        "An error occurred attempting to output data from a CGI module"      
#define UUI_CGI_SERVICE_ERROR      840 /* CGI service error          */
#define UUI_RC98                   840, "CGI service function error" ,\
        "An internal error occurred processing an SMC HTTP server "   \
        "I/O task.  This error should be reported to StorageTek Support."
#define UUI_BROWSER_NOT_AUTHORIZED 844 /* Request from WEB browser   */
/*                                        ...not authorized          */
#define UUI_RC99                   844, "Web browser request not authorized",\
        "A transaction was received by the SMC HTTP server from an " \
        "unauthorized web browser."

/*********************************************************************/
/* The following tags are available for future use.                  */
/*********************************************************************/
#define UUI_RC113                  0,   "Available", ""
#define UUI_RC116                  0,   "Available", ""
#define UUI_RC117                  0,   "Available", ""
#define UUI_RC118                  0,   "Available", ""
#define UUI_RC119                  0,   "Available", ""
#define UUI_RC120                  0,   "Available", ""
#define UUI_RC121                  0,   "Available", ""
#define UUI_RC122                  0,   "Available", ""
#define UUI_RC123                  0,   "Available", ""
#define UUI_RC124                  0,   "Available", ""
#define UUI_RC125                  0,   "Available", ""
#define UUI_RC126                  0,   "Available", ""
#define UUI_RC127                  0,   "Available", ""
#define UUI_RC128                  0,   "Available", ""
#define UUI_RC129                  0,   "Available", ""
#define UUI_RC130                  0,   "Available", ""
#define UUI_RC131                  0,   "Available", ""
#define UUI_RC132                  0,   "Available", ""
#define UUI_RC133                  0,   "Available", ""
#define UUI_RC134                  0,   "Available", ""
#define UUI_RC135                  0,   "Available", ""
#define UUI_RC136                  0,   "Available", ""
#define UUI_RC137                  0,   "Available", ""
#define UUI_RC138                  0,   "Available", ""
#define UUI_RC139                  0,   "Available", ""
#define UUI_RC140                  0,   "Available", ""
#define UUI_RC141                  0,   "Available", ""
#define UUI_RC142                  0,   "Available", ""
#define UUI_RC143                  0,   "Available", ""
#define UUI_RC144                  0,   "Available", ""
#define UUI_RC145                  0,   "Available", ""
#define UUI_RC146                  0,   "Available", ""
#define UUI_RC147                  0,   "Available", ""
#define UUI_RC148                  0,   "Available", ""
#define UUI_RC149                  0,   "Available", ""
#define UUI_RC150                  0,   "Available", ""
#define UUI_RC151                  0,   "Available", ""
#define UUI_RC152                  0,   "Available", ""
#define UUI_RC153                  0,   "Available", ""
#define UUI_RC154                  0,   "Available", ""
#define UUI_RC155                  0,   "Available", ""
#define UUI_RC156                  0,   "Available", ""
#define UUI_RC157                  0,   "Available", ""
#define UUI_RC158                  0,   "Available", ""
#define UUI_RC159                  0,   "Available", ""
#define UUI_RC160                  0,   "Available", ""

/*********************************************************************/
/* Leave UUI_UNKNOWN_ERROR as the last UUI return code.              */
/*********************************************************************/
#define UUI_UNKNOWN_ERROR          996 /* Unknown logic error        */
#define UUI_RC999                  996, "Unknown UUI error"          ,\
        "An undetermined type of error occurred when processing "     \
        "a UUI request."
#define UUI_MAX_RC                 996
#ifndef MAX_UUI_LINE_SIZE
    #define MAX_UUI_LINE_SIZE      4096/* Max UUI line size          */
#endif


#endif                                           /* SMCCODES_HEADER  */


