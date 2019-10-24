/** HEADER FILE PROLOGUE *********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      http.h                                           */
/** Description:    http_test executable common header file.         */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     08/15/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/** END PROLOGUE *****************************************************/

#ifndef HTTP_HEADER
#define HTTP_HEADER

#include <semaphore.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef IPC_SHARED

    #include <netinet/in.h>

    #ifdef SOLARIS
        #include <sys/file.h>
    #endif

    #ifdef AIX
        #include <fcntl.h>
    #else
        #include <sys/fcntl.h>
    #endif

    #include <netdb.h>

#endif /* IPC_SHARED */

#include "xapi.h"


/*********************************************************************/
/* http_test extern function name mapping:                           */
/*********************************************************************/
//#define http_dismount                        http_dismount1
//#define http_eject                           http_eject1
//#define http_enter                           http_enter1
//#define http_gen_resp                        http_gen_resp1
//#define http_listen                          http_listen1
//#define main                                 http_main1
//#define http_mount                           http_mount1
//#define http_qacs                            http_qacs1
//#define http_qcap                            http_qcap1
//#define http_qdrive_info                     http_qdrive_info1
//#define http_qdrvtypes                       http_qdrvtypes1
//#define http_qlsm                            http_qlsm1
//#define http_qmedia                          http_qmedia1
//#define http_qscratch                        http_qscratch1
//#define http_qscr_mnt_info                   http_qscr_mnt_info1
//#define http_qscrpool_info                   http_qscrpool_info1
//#define http_qserver                         http_qserver1
//#define http_qvolume_info                    http_qvolume_info1
//#define http_read_next                       http_read_next1
//#define http_select_wait                     http_select_wait1
//#define http_set_scr                         http_set_scr1
//#define http_set_unscr                       http_set_unscr1
//#define http_term                            http_term1
//#define http_volrpt                          http_volrpt1
//#define http_work                            http_work1



/*********************************************************************/
/* Global variable declaration.                                      */
/*********************************************************************/
extern struct HTTPCVT *global_httpcvt;


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/

/*********************************************************************/
/* Constants used to populate and access the HTTPCVT control table:  */
/*********************************************************************/
#define HTTP_CVT_SHM_KEY       1752462448  /* Shared mem key for     */
/*                                        ...HTTPCVT (ASCII "http")  */
#define HTTP_LOOPBACK_ADDRESS  2130706433  /* X'7F000001' = 127.0.0.1*/
#define HTTP_INIT_RETRY_COUNT  3       /* HTTP init retry count      */
#define HTTP_LISTEN_QUEUE_SIZE 15      /* Listen queue size          */
#define HTTP_RESPONSE_DIR      "HTTP_RESPONSE_DIR"
#define HTTP_SEND_BUFFER_SIZE  16383   /* HTTP send buffer size      */


/*********************************************************************/
/* HTTPCVT describes the http_test control table.                    */
/*********************************************************************/
struct HTTPCVT
{
    char               cbHdr[8];       /* Control block eyecatcher   */
#define HTTPCVT_ID     "HTTPCVT"       /* ..."HTTPCVT"               */
    int                cvtShMemSegId;  /* HTTPCVT segment ID         */
    int                socketId;       /* http_test listener socket  */
    struct sockaddr_in sockaddr_in;    /* listener socket sockaddr_in*/
    /*****************************************************************/
    /* Server IP address will be IPv4 address format.                */
    /*****************************************************************/
    unsigned int       ipAddress;      /* Server IPv4 address        */    
    char               ipAddressString[16];/* Server IP addr string  */
    pid_t              httpPid;        /* Pid of http_test thread    */
    sem_t              httpCdsLock;    /* Semaphore to serialize     */
    /*                                    ...access to psuedo CDS    */
    time_t             startTime;      /* Time http_test initialized */
    int                _f0[7];         /* Available                  */
    char               status;         /* HTTP status                */
#define HTTP_ACTIVE            0       /* ...HTTP active or start'ed */
#define HTTP_IDLE_PENDING      1       /* ...HTTP idle pending       */
#define HTTP_IDLE              2       /* ...HTTP idle               */
#define HTTP_CANCEL            3       /* ...HTTP cancelled          */
    char               _f1[3];         /* Available                  */
    /*****************************************************************/
    /* The following values are defined using environmental          */
    /* variables as follows:                                         */
    /* XAPI_PORT       : TCP/IP port number of http_test server on   */
    /*                   server host.  Default value is 8080.        */
    /* XAPI_HOSTNAME   : Hostname of server host where http_test     */
    /*                   server is executing.                        */
    /*                   Value must be specified if the environment  */
    /*                   variable CSI_XAPI_CONVERSION=1              */
    /* XAPI_TAPEPLEX   : TapePlex name of remote HSC server.         */
    /*                   Value must be specified if the environment  */
    /*                   variable CSI_XAPI_CONVERSION=1              */
    /* XAPI_SUBSYSTEM  : MVS subsystem name of remote HSC server.    */
    /*                   Value defaults to 1st 4 characters of       */
    /*                   XAPI_TAPEPLEX.                              */
    /* XAPI_VERSION    : Minimum version of XAPI that is supported.  */
    /*                   Default value is 710.                       */
    /* XAPI_USER       : RACF userid passed as part of XAPI          */
    /*                   transaction for XAPI command authorization. */
    /*                   This is optional.                           */
    /* XAPI_GROUP      : RACF groupid passed as part of XAPI         */
    /*                   transaction for XAPI command authorization. */
    /*                   This is optional.                           */
    /*****************************************************************/
    unsigned short     xapiPort;
    char               xapiHostname[XAPI_HOSTNAME_SIZE + 1];
    char               xapiTapeplex[XAPI_TAPEPLEX_SIZE + 1];
    char               xapiSubsystem[XAPI_SUBSYSTEM_SIZE + 1];
    char               xapiVersion[XAPI_VERSION_SIZE + 1];
    char               xapiUser[XAPI_USER_SIZE + 1];
    char               xapiGroup[XAPI_GROUP_SIZE + 1];
    char               xapiEjectText[XAPI_EJECT_TEXT_SIZE + 1];
};


/*********************************************************************/
/* Macros:                                                           */
/*********************************************************************/

/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
int http_dismount(struct HTTPCVT *pHttpcvt,
                  int             socketId,
                  int             seqNum,
                  void           *pRequestXmlparse);

int http_eject(struct HTTPCVT *pHttpcvt,
               int             socketId,
               int             seqNum,
               void           *pRequestXmlparse);

int http_enter(struct HTTPCVT *pHttpcvt,
               int             socketId,
               int             seqNum,
               void           *pRequestXmlparse);

int http_gen_resp(struct HTTPCVT *pHttpcvt,
                  int             socketId,
                  int             seqNum,
                  char           *respFileName);

int http_listen(struct HTTPCVT *pHttpcvt);

int http_main(int  argc, char **argv);           /* main() entry     */

int http_mount(struct HTTPCVT *pHttpcvt,
               int             socketId,
               int             seqNum,
               void           *pRequestXmlparse);

int http_qacs(struct HTTPCVT *pHttpcvt,
              int             socketId,
              int             seqNum,
              void           *pRequestXmlparse);

int http_qcap(struct HTTPCVT *pHttpcvt,
              int             socketId,
              int             seqNum,
              void           *pRequestXmlparse);

int http_qdrive_info(struct HTTPCVT *pHttpcvt,
                     int             socketId,
                     int             seqNum,
                     void           *pRequestXmlparse);

int http_qdrvtypes(struct HTTPCVT *pHttpcvt,
                   int             socketId,
                   int             seqNum,
                   void           *pRequestXmlparse);

int http_qlsm(struct HTTPCVT *pHttpcvt,
              int             socketId,
              int             seqNum,
              void           *pRequestXmlparse);

int http_qmedia(struct HTTPCVT *pHttpcvt,
                int             socketId,
                int             seqNum,
                void           *pRequestXmlparse);

int http_qscratch(struct HTTPCVT *pHttpcvt,
                  int             socketId,
                  int             seqNum,
                  void           *pRequestXmlparse);

int http_qscr_mnt_info(struct HTTPCVT *pHttpcvt,
                       int             socketId,
                       int             seqNum,
                       void           *pRequestXmlparse);

int http_qscrpool_info(struct HTTPCVT *pHttpcvt,
                       int             socketId,
                       int             seqNum,
                       void           *pRequestXmlparse);

int http_qserver(struct HTTPCVT *pHttpcvt,
                 int             socketId,
                 int             seqNum,
                 void           *pRequestXmlparse);

int http_qvolume_info(struct HTTPCVT *pHttpcvt,
                      int             socketId,
                      int             seqNum,
                      void           *pRequestXmlparse);

int http_read_next(void *pFile,
                   char *pInputBuffer,
                   int   maxInputBufferLen,
                   int  *pInputBufferLen,
                   char *fileHandle);

int http_select_wait(struct HTTPCVT *pHttpcvt,
                     int             socketId,
                     char            writeFdsFlag,
                     int             timeoutSeconds);

int http_set_scr(struct HTTPCVT *pHttpcvt,
                 int             socketId,
                 int             seqNum,
                 void           *pRequestXmlparse);

int http_set_unscr(struct HTTPCVT *pHttpcvt,
                   int             socketId,
                   int             seqNum,
                   void           *pRequestXmlparse);

void http_term(struct HTTPCVT *pHttpcvt);

int http_work(struct HTTPCVT *pParentHttpcvt,
              int             socketId);

int http_volrpt(struct HTTPCVT *pHttpcvt,
                int             socketId,
                int             seqNum,
                void           *pRequestXmlparse);


#endif                                           /* HTTP_HEADER      */




