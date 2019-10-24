/* SccsId @(#)csi.h               2.2 10/29/01 */
#ifndef _CSI_
#define _CSI_
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:        
 *      csi.h
 *
 * Functional Description:      
 *
 *      CSI/SSI interface include file for the client system interface.
 *
 * Modified by:
 *      
 *      J. A. Wishner   12/02/88.       Created.
 *      J. A. Wishner   05/01/89.       TIME STAMP-POST CUSTOMER INITIAL RELEASE
 *      J. A. Wishner   05/15/89.       Added csi routine level tracing
 *                                      CSI_XDR_TRACE_LEVEL definition.
 *                                      Cleaned out redundant #includes.
 *      J. A. Wishner   05/30/89        Changed data portion allocation on
 *                                      CSI_MSGBUF.  Was "char *", now data[1].
 *                                      External for csi_netbuf goes to char *.
 *                                      External for csi_netbuf_data goes away.
 *      J. A. Wishner   06/16/89        Took limits off of queue sizes.
 *      J. A. Wishner   07/27/89        Added CSI_MIN_CONNECTQ_AGETIME
 *                                      minimum time connection aging;
 *                                      Added CSI_MAX_CONNECTQ_AGETIME
 *                                      maximum time connection aging.
 *      J. A. Wishner   02/08/89        Added ALIGNED_BYTES definition for data
 *                                      rather than using char* directly.
 *      J. W. Montgomery 15-Jun-1990    Version 2.
 *      J. A. Wishner   09/22/90        Modified packet offset macros, added
 *                                      CSI_PAK_IPCOFFSET and CSI_PAK_IPCDATAP.
 *                                      Removed csi_xdrrequest/csi_xdrresponse.
 *                                      Added:
 *                                      extern bool_t csi_xlm_request();
 *                                      extern bool_t csi_xlm_response();
 *                                      extern bool_t csi_xpd_request();
 *                                      extern bool_t csi_xpd_response();
 *                                      Added TYPE service_type to CSI_MSGUBF.
 *      H. I. Grapek    22-Aug-1991     Added csi_xcap_id, added CSI_V1_REQUEST
 *                                      CSI_V1_RESPONSE.
 *      J. A. Wishner   03-Oct-1991     Completed release 3 functionality.
 *      E. A. Alongi    30-Jul-1992     Support for minimum version allowed, 
 *                                      ACSPD allowed and add csi_trace_flag.
 *      E. A. Alongi    30-Sep-1992     Shortened environment variable name
 *                                      for minimum version to ACSLS_MIN_VERSION
 *                                      (The original name caused problems with
 *                                      echo and csh - name was too long.)
 *      E. A. Alongi    03-Dec-1992     For access control: added enum type
 *                                      csi_lookup and csi_get_accessid()
 *                                      function declaration. Flexelint mods.
 *      Emanuel Alongi  23-Jul-1993     Deleted csi_lookup enum and ref to
 *                                      csi_get_accessid() - now a common lib
 *                                      function.  Eliminated extern reference
 *                                      to global environment variables.  These
 *                                      variables are now accessed locally and
 *                                      dynamcially.  Changes to support VERSION
 *                                      4 packets and preserve version 2 packets
 *      David Farmer    17-Aug-1993     Added extern for csi_broke_pipe
 *      E. A. Alongi    27-Oct-1993     Eliminated ACSPD file prototypes.
 *      K. J. Stickney  09-Feb-1994     Changed csi_rpcdisp() from int to void.
 *      Jeff Hanson     08-Oct1998      R5.3 changed CSI_NAME_SIZE to 16.
 *      Scott Siao      15-Oct-2001     For event notification added register,
 *                                      unregister, check_registration.
 *      Scott Siao      12-Nov-2001     Added display command definitions
 *      Scott Siao      06-Feb-2002     Added virtual_mount command definitions
 *      Scott Siao      12-Mar-2002     Changed CSI_HOSTNAMESIZE from 32 to 128.
 *      Wipro (Subhash) 04-Jun-2004     Added functions to support
 *                                      mount/dismount events.
 *      Mitch Black     01-Dec-2004     Remove references to pdaemon structures
 *                                      (...ACSPD_REQUEST/RESPONSE) within CSI request
 *                                      and response structures.
 *      Mitch Black     01-Dec-2004     Removed references to csi_adicall and
 *                                      csi_netinput (never used in CDK).
 *                                      Removed #ifdef SSI and removed extern
 *                                      reference to csi_ssi_alt_procno_pd.
 *      Mike Williams   01-Jun-2010     Added prototype for csi_xgrp_type and
 *                                      for csi_ssi_api_resp
 *      Joseph Nofi     15-May-2011     XAPI support;
 *                                      Added csi_xapi_conversion_flag and
 *                                      csi_xapi_conversion_table global variables.
 * 
 */

#ifndef _CL_QM_DEFS_
#include "cl_qm_defs.h"
#endif

#ifndef _ACSLM_
#include "acslm.h"
#endif

#ifndef _CSI_MSG_
#include "csi_msg.h"
#endif

#ifndef _CSI_STRUCTS_
#include "csi_structs.h"
#endif

#ifndef _CSI_V1_STRUCTS_
#include "csi_v1_structs.h"
#endif

#ifndef _CSI_V2_STRUCTS_
#include "csi_v2_structs.h"
#endif

/* Log File Name  */
#ifdef ADI
#define  CSI_LOG_NAME    "OSLAN" 
#else
#define  CSI_LOG_NAME    "ONC RPC" 
#endif /* ADI */

/*      IPC related definitions. Including workaround (for common library) for 
        Sun IPC bug */
#ifndef INETSOCKETS
#define CSI_INPUT_SOCKET "./to_CSI"     /* csi input socket name */
#define CSI_ACSLM_SOCKET "./to_ACSLM"   /* acslm input socket name */
#else
#define CSI_INPUT_SOCKET ANY_PORT       /* csi input socket name */
#define CSI_ACSLM_SOCKET ACSLM          /* see defs.h acslm input socket name */
#endif

/* function as a variable on call */
typedef void (*CSI_VOIDFUNC)(CSI_HEADER *cs_hdrp, char *stringp, int maxsize);

/*      Miscellaneous CSI definitions */
#define CSI_XDR_TRACE_LEVEL      5          /* xdr routine level tracing */
#define CSI_DEF_CONNECTQ_AGETIME 172800     /* seconds time connection aging */
#define CSI_MIN_CONNECTQ_AGETIME 600        /* minimum seconds connect aging */
#define CSI_MAX_CONNECTQ_AGETIME 31536000   /* maximum seconds connect aging */
#define CSI_SELECT_TIMEOUT       2          /* seconds time timeout */
#define CSI_DEF_RETRY_TIMEOUT    3          /* seconds per network send try */
#define CSI_DEF_RETRY_TRIES      5          /* number of times network retry */
#define CSI_HOSTNAMESIZE         128        /* size of name of host csi is on */
#define CSI_NO_CALLEE   (char *) NULL       /* no fail function name passed to
                                             * to csi_logevent() */
#define CSI_NO_LOGFUNCTION (CSI_VOIDFUNC) NULL
#define CSI_NO_SSI_IDENTIFIER    0   /* csi_header-no value in ssi_identifier */
#define CSI_ISFINALRESPONSE(opt) (0 == (INTERMEDIATE & opt) && \
                                  0 == (ACKNOWLEDGE & opt) ? TRUE : FALSE)
#define CSI_MAX_MESSAGE_SIZE    MAX_MESSAGE_SIZE

/*      packet transfer direction used in packet tracing routine(s) */
#define CSI_TO_ACSLM    0              /* packet direction csi_ptrace() */
#define CSI_FROM_ACSLM  1              /* packet direction csi_ptrace() */

/* for PDAEMON routing */
#define CSI_TO_ACSPD    2              /* packet direction csi_ptrace() */
#define CSI_FROM_ACSPD  3              /* packet direction csi_ptrace() */
#define CSI_NAME_SIZE  16

/* ADI-specific defines */
#define CSI_DGRAM_SIZE  1495 /* Max size of OSLAN data piece. 
                              * Not including 2-byte preamble.
                              */
#define ADI_HANDSHAKE_TIMEOUT  30


/*      RPC variables specifically for a CSI. */
#define CSI_PROGRAM1            0x200000fe  /* OLD CSI RPC program number */
#define CSI_PROGRAM             0x000493ff  /* Official CSI RPC program number*/
#define CSI_UDP_VERSION         1               /* RPC UDP server version# */
#define CSI_TCP_VERSION         2               /* RPC TCP server version# */
#define CSI_ACSLM_PROC          1000            /* RPC server procedure# */
#define CSI_ACSPD_PROC          1001            /* acspd procedure number   */
#define CSI_DEF_TCPSENDBUF      0               /* size tcp rpc send buffer */
#define CSI_DEF_TCPRECVBUF      0               /* size tcp rpc receive buffer*/

/* network send options for routine csi_net_send() */
typedef enum {
    CSI_NORMAL_SEND,                            /* regular send of packet */
    CSI_FLUSH_OUTPUT_QUEUE                      /* flush network send queue */
} CSI_NET_SEND_OPTIONS;


/*      Environment Variables */
#define CSI_HOSTNAME        "CSI_HOSTNAME"         /* name of host csi is on */


/* alternate procedure numbers for ssi to get callbacks on */
#define CSI_SSI_ACSLM_CALLBACK_PROCEDURE "SSI_ACSLM_CALLBACK_PROCEDURE"
#define CSI_SSI_ACSPD_CALLBACK_PROCEDURE "SSI_ACSPD_CALLBACK_PROCEDURE"
#define CSI_SSI_CALLBACK_VERSION_NUMBER  "SSI_CALLBACK_VERSION_NUMBER"
#define CSI_HAVENT_GOTTEN_ENVIRONMENT_YET (long) -2    /* not accessed yet */
#define CSI_NOT_IN_ENVIRONMENT            (long) -1    /* not in environment */ 


/* Connection queue related defines for saving csi_header return addresses */
#define CSI_MAXMEMB_LM_QUEUE            0          /* unlimited size LM Q */
#define CSI_MAXMEMB_NI_OUT_QUEUE        0          /* unlimited size NI out Q */
#define CSI_MAXQUEUES           2                  /* max # of csi queues */
#define CSI_CONNECTQ_NAME "connection queue"       /* name of connection Q */
#define CSI_NI_OUTQ_NAME  "network output queue"   /* name of net output Q */
#define CSI_QCB_REMARKS   "master control block"   /* name of Q control block */

/* csi message buffer description passed to top level xdr translation routines*//* data offsets into packet buffer */
#define CSI_PAK_NETOFFSET       (sizeof(CSI_HEADER) > sizeof(IPC_HEADER))\
                                ? 0 : sizeof(IPC_HEADER) - sizeof(CSI_HEADER)
#define CSI_PAK_IPCOFFSET       (sizeof(CSI_HEADER) > sizeof(IPC_HEADER))\
                                ? sizeof(CSI_HEADER) - sizeof(IPC_HEADER) : 0
#define CSI_PAK_LMOFFSET        CSI_PAK_IPCOFFSET
#define CSI_PAK_NETDATAP(bufp)  ((char *)(bufp)->data) + \
                                                ((char *)CSI_PAK_NETOFFSET)
#define CSI_PAK_IPCDATAP(bufp)  ((char *)(bufp)->data) + \
                                                ((char *)CSI_PAK_IPCOFFSET)
#define CSI_PAK_LMDATAP(bufp)   CSI_PAK_IPCDATAP(bufp)

/* packet buffer status for buffer of type CSI_MSGBUF */
typedef enum {
    CSI_PAKSTAT_INITIAL = 0,             /* currently testing packet */
    CSI_PAKSTAT_XLATE_COMPLETED,         /* packet translation completed */
    CSI_PAKSTAT_XLATE_ERROR,             /* translate error incomplete packet */
    CSI_PAKSTAT_DUPLICATE_PACKET         /* packet in buffer is duplicate */
} CSI_PAKSTAT;

/* queue management */
typedef struct csi_q_mgmt {
    unsigned short      xmit_tries;     /* number of attempts at transmission */
} CSI_Q_MGMT;

/* packet buffer */
typedef struct {
    int           offset;               /* starting offset of packet data */
    int           size;                 /* size of the data in buffer */
    int           maxsize;              /* maximum size of the data in buffer */
    int           translated_size;      /* size valid data xdr translatable */
    CSI_PAKSTAT   packet_status;        /* success/failure of translation */
    CSI_Q_MGMT    q_mgmt;               /* for managment of queueing */
    TYPE          service_type;         /* destination/source module */
    ALIGNED_BYTES data[1];              /* starting address of data storage */
} CSI_MSGBUF;

#define CSI_MSGBUF_MAXSIZE      (sizeof(CSI_MSGBUF) + CSI_MAX_MESSAGE_SIZE)

/* format the internet address into a string */
/* byte1 internet addr*/
/* byte2 internet addr*/
/* byte3 internet addr*/
/* byte4 internet addr*/
#define CSI_INTERNET_ADDR_TO_STRING(strbuf, netaddr) \
    sprintf(strbuf, "%u.%u.%u.%u",           \
        (long)((netaddr & 0xff000000) >> 24), \
        (long)((netaddr & 0xff0000)   >> 16), \
        (long)((netaddr & 0xff00)     >> 8),  \
        (long) (netaddr & 0xff))

#ifdef DEBUG
#define CSI_DEBUG_LOG_NETQ_PACK( csi_request_headerp, action_str, status )    \
{                                                                             \
    CSI_REQUEST_HEADER *rp = csi_request_headerp;                             \
    char               *typ;                                                  \
    char               *actionp = action_str;                                 \
    STATUS              ecode = status;                                       \
    if (rp->message_header.message_options & ACKNOWLEDGE)                     \
    typ = "ACKNOWLEDGE";                                                  \
    else if (rp->message_header.message_options & INTERMEDIATE)               \
    typ = "INTERMEDIATE";                                                 \
    else                                                                      \
    typ = "FINAL, or REQUEST";                                            \
    MLOGCSI((ecode,st_module,CSI_NO_CALLEE,MMSG(1191,                 \
      "%s\ncommand:%s\ntype:%s\nsequence#:%d\nssi identifier:%d"),        \
      actionp, cl_command(rp->message_header.command),                        \
      typ, rp->csi_header.xid.seq_num, rp->csi_header.ssi_identifier));       \
}
#else
#define CSI_DEBUG_LOG_NETQ_PACK( csi_request_headerp, action_str, status )
#endif /*DEBUG*/

/* packet structure definitions for requests sent from SSI/NI to the csi */
/* Version 0 */
typedef union {
    CSI_V0_REQUEST_HEADER            csi_req_header;
    CSI_V0_AUDIT_REQUEST             csi_audit_req;
    CSI_V0_ENTER_REQUEST             csi_enter_req;
    CSI_V0_EJECT_REQUEST             csi_eject_req;
    CSI_V0_VARY_REQUEST              csi_vary_req;
    CSI_V0_MOUNT_REQUEST             csi_mount_req;
    CSI_V0_DISMOUNT_REQUEST          csi_dismount_req;
    CSI_V0_QUERY_REQUEST             csi_query_req;
    CSI_V0_CANCEL_REQUEST            csi_cancel_req;
    CSI_V0_START_REQUEST             csi_start_req;
    CSI_V0_IDLE_REQUEST              csi_idle_req;
} CSI_V0_REQUEST;

/* packet structure definitions for responses sent from the csi to SSI/NI */
/* Version 0 */
typedef union {
    CSI_V0_REQUEST_HEADER            csi_req_header;
    CSI_V0_ACKNOWLEDGE_RESPONSE      csi_ack_res;
    CSI_V0_AUDIT_RESPONSE            csi_audit_res;
    CSI_V0_ENTER_RESPONSE            csi_enter_res;
    CSI_V0_EJECT_RESPONSE            csi_eject_res;
    CSI_V0_VARY_RESPONSE             csi_vary_res; 
    CSI_V0_MOUNT_RESPONSE            csi_mount_res;
    CSI_V0_DISMOUNT_RESPONSE         csi_dismount_res;
    CSI_V0_QUERY_RESPONSE            csi_query_res;
    CSI_V0_CANCEL_RESPONSE           csi_cancel_res;
    CSI_V0_START_RESPONSE            csi_start_res;
    CSI_V0_IDLE_RESPONSE             csi_idle_res;
    CSI_V0_EJECT_ENTER               csi_eject_enter_res;
} CSI_V0_RESPONSE;


/* packet structure definitions for requests sent from SSI/NI to the csi */

/* Version V1 */
typedef union {
    CSI_V1_REQUEST_HEADER            csi_req_header;
    CSI_V1_AUDIT_REQUEST             csi_audit_req;
    CSI_V1_ENTER_REQUEST             csi_enter_req;
    CSI_V1_EJECT_REQUEST             csi_eject_req;
    CSI_V1_EXT_EJECT_REQUEST         csi_xeject_req;
    CSI_V1_VARY_REQUEST              csi_vary_req;
    CSI_V1_MOUNT_REQUEST             csi_mount_req;
    CSI_V1_DISMOUNT_REQUEST          csi_dismount_req;
    CSI_V1_QUERY_REQUEST             csi_query_req;
    CSI_V1_CANCEL_REQUEST            csi_cancel_req;
    CSI_V1_START_REQUEST             csi_start_req;
    CSI_V1_IDLE_REQUEST              csi_idle_req;
    CSI_V1_SET_CLEAN_REQUEST         csi_set_clean_req;
    CSI_V1_SET_CAP_REQUEST           csi_set_cap_req;
    CSI_V1_SET_SCRATCH_REQUEST       csi_set_scratch_req;
    CSI_V1_DEFINE_POOL_REQUEST       csi_define_pool_req;
    CSI_V1_DELETE_POOL_REQUEST       csi_delete_pool_req;
    CSI_V1_MOUNT_SCRATCH_REQUEST     csi_mount_scratch_req;
    CSI_V1_LOCK_REQUEST              csi_lock_req;
    CSI_V1_CLEAR_LOCK_REQUEST        csi_clear_lock_req;
    CSI_V1_QUERY_LOCK_REQUEST        csi_query_lock_req;
    CSI_V1_UNLOCK_REQUEST            csi_unlock_req;
    CSI_V1_VENTER_REQUEST            csi_venter_req;
} CSI_V1_REQUEST;

/* packet structure definitions for responses sent from the csi to SSI/NI */
/* Version V1 */
typedef union {
    CSI_V1_REQUEST_HEADER            csi_req_header;
    CSI_V1_ACKNOWLEDGE_RESPONSE      csi_ack_res;
    CSI_V1_AUDIT_RESPONSE            csi_audit_res;
    CSI_V1_ENTER_RESPONSE            csi_enter_res;
    CSI_V1_EJECT_RESPONSE            csi_eject_res;
    CSI_V1_VARY_RESPONSE             csi_vary_res; 
    CSI_V1_MOUNT_RESPONSE            csi_mount_res;
    CSI_V1_DISMOUNT_RESPONSE         csi_dismount_res;
    CSI_V1_QUERY_RESPONSE            csi_query_res;
    CSI_V1_CANCEL_RESPONSE           csi_cancel_res;
    CSI_V1_START_RESPONSE            csi_start_res;
    CSI_V1_IDLE_RESPONSE             csi_idle_res;
    CSI_V1_EJECT_ENTER               csi_eject_enter_res;
    CSI_V1_SET_CLEAN_RESPONSE        csi_set_clean_res;
    CSI_V1_SET_CAP_RESPONSE          csi_set_cap_res;
    CSI_V1_SET_SCRATCH_RESPONSE      csi_set_scratch_res;
    CSI_V1_DEFINE_POOL_RESPONSE      csi_define_pool_res;
    CSI_V1_DELETE_POOL_RESPONSE      csi_delete_pool_res;
    CSI_V1_MOUNT_SCRATCH_RESPONSE    csi_mount_scratch_res;
    CSI_V1_LOCK_RESPONSE             csi_lock_res;
    CSI_V1_CLEAR_LOCK_RESPONSE       csi_clear_lock_res;
    CSI_V1_QUERY_LOCK_RESPONSE       csi_query_lock_res;
    CSI_V1_UNLOCK_RESPONSE           csi_unlock_res;
} CSI_V1_RESPONSE;


/* packet structure definitions for requests sent from SSI/NI to the csi */
/* Version 2 */
typedef union {
    CSI_V2_REQUEST_HEADER         csi_req_header;
    CSI_V2_AUDIT_REQUEST          csi_audit_req;
    CSI_V2_ENTER_REQUEST          csi_enter_req;
    CSI_V2_EJECT_REQUEST          csi_eject_req;
    CSI_V2_EXT_EJECT_REQUEST      csi_xeject_req;
    CSI_V2_VARY_REQUEST           csi_vary_req;
    CSI_V2_MOUNT_REQUEST          csi_mount_req;
    CSI_V2_DISMOUNT_REQUEST       csi_dismount_req;
    CSI_V2_QUERY_REQUEST          csi_query_req;
    CSI_V2_CANCEL_REQUEST         csi_cancel_req;
    CSI_V2_START_REQUEST          csi_start_req;
    CSI_V2_IDLE_REQUEST           csi_idle_req;
    CSI_V2_SET_CLEAN_REQUEST      csi_set_clean_req;
    CSI_V2_SET_CAP_REQUEST        csi_set_cap_req;
    CSI_V2_SET_SCRATCH_REQUEST    csi_set_scratch_req;
    CSI_V2_DEFINE_POOL_REQUEST    csi_define_pool_req;
    CSI_V2_DELETE_POOL_REQUEST    csi_delete_pool_req;
    CSI_V2_MOUNT_SCRATCH_REQUEST  csi_mount_scratch_req;
    CSI_V2_LOCK_REQUEST           csi_lock_req;
    CSI_V2_CLEAR_LOCK_REQUEST     csi_clear_lock_req;
    CSI_V2_QUERY_LOCK_REQUEST     csi_query_lock_req;
    CSI_V2_UNLOCK_REQUEST         csi_unlock_req;
    CSI_V2_VENTER_REQUEST         csi_venter_req;
} CSI_V2_REQUEST;

/* packet structure definitions for responses sent from the csi to SSI/NI */
/* Version 2 */
typedef union {
    CSI_V2_REQUEST_HEADER         csi_req_header;
    CSI_V2_ACKNOWLEDGE_RESPONSE   csi_ack_res;
    CSI_V2_AUDIT_RESPONSE         csi_audit_res;
    CSI_V2_ENTER_RESPONSE         csi_enter_res;
    CSI_V2_EJECT_RESPONSE         csi_eject_res;
    CSI_V2_VARY_RESPONSE          csi_vary_res; 
    CSI_V2_MOUNT_RESPONSE         csi_mount_res;
    CSI_V2_DISMOUNT_RESPONSE      csi_dismount_res;
    CSI_V2_QUERY_RESPONSE         csi_query_res;
    CSI_V2_CANCEL_RESPONSE        csi_cancel_res;
    CSI_V2_START_RESPONSE         csi_start_res;
    CSI_V2_IDLE_RESPONSE          csi_idle_res;
    CSI_V2_EJECT_ENTER            csi_eject_enter_res;
    CSI_V2_SET_CLEAN_RESPONSE     csi_set_clean_res;
    CSI_V2_SET_CAP_RESPONSE       csi_set_cap_res;
    CSI_V2_SET_SCRATCH_RESPONSE   csi_set_scratch_res;
    CSI_V2_DEFINE_POOL_RESPONSE   csi_define_pool_res;
    CSI_V2_DELETE_POOL_RESPONSE   csi_delete_pool_res;
    CSI_V2_MOUNT_SCRATCH_RESPONSE csi_mount_scratch_res;
    CSI_V2_LOCK_RESPONSE          csi_lock_res;
    CSI_V2_CLEAR_LOCK_RESPONSE    csi_clear_lock_res;
    CSI_V2_QUERY_LOCK_RESPONSE    csi_query_lock_res;
    CSI_V2_UNLOCK_RESPONSE        csi_unlock_res;
} CSI_V2_RESPONSE;

/* packet structure definitions for requests sent from SSI/NI to the csi */
/* Version 4 */
typedef union {
    CSI_REQUEST_HEADER         csi_req_header;
    CSI_AUDIT_REQUEST          csi_audit_req;
    CSI_ENTER_REQUEST          csi_enter_req;
    CSI_EJECT_REQUEST          csi_eject_req;
    CSI_EXT_EJECT_REQUEST      csi_xeject_req;
    CSI_VARY_REQUEST           csi_vary_req;
    CSI_MOUNT_REQUEST          csi_mount_req;
    CSI_DISMOUNT_REQUEST       csi_dismount_req;
    CSI_QUERY_REQUEST          csi_query_req;
    CSI_CANCEL_REQUEST         csi_cancel_req;
    CSI_START_REQUEST          csi_start_req;
    CSI_IDLE_REQUEST           csi_idle_req;
    CSI_SET_CLEAN_REQUEST      csi_set_clean_req;
    CSI_SET_CAP_REQUEST        csi_set_cap_req;
    CSI_SET_SCRATCH_REQUEST    csi_set_scratch_req;
    CSI_DEFINE_POOL_REQUEST    csi_define_pool_req;
    CSI_DELETE_POOL_REQUEST    csi_delete_pool_req;
    CSI_MOUNT_SCRATCH_REQUEST  csi_mount_scratch_req;
    CSI_LOCK_REQUEST           csi_lock_req;
    CSI_CLEAR_LOCK_REQUEST     csi_clear_lock_req;
    CSI_QUERY_LOCK_REQUEST     csi_query_lock_req;
    CSI_UNLOCK_REQUEST         csi_unlock_req;
    CSI_VENTER_REQUEST         csi_venter_req;
    CSI_REGISTER_REQUEST       csi_register_req;
    CSI_UNREGISTER_REQUEST     csi_unregister_req;
    CSI_CHECK_REGISTRATION_REQUEST  csi_check_registration_req;
    CSI_DISPLAY_REQUEST        csi_display_req;
    CSI_MOUNT_PINFO_REQUEST    csi_mount_pinfo_req;
} CSI_REQUEST;

/* packet structure definitions for responses sent from the csi to SSI/NI */
/* Version 4 */
typedef union {
    CSI_REQUEST_HEADER         csi_req_header;
    CSI_ACKNOWLEDGE_RESPONSE   csi_ack_res;
    CSI_AUDIT_RESPONSE         csi_audit_res;
    CSI_ENTER_RESPONSE         csi_enter_res;
    CSI_EJECT_RESPONSE         csi_eject_res;
    CSI_VARY_RESPONSE          csi_vary_res;
    CSI_MOUNT_RESPONSE         csi_mount_res;
    CSI_DISMOUNT_RESPONSE      csi_dismount_res;
    CSI_QUERY_RESPONSE         csi_query_res;
    CSI_CANCEL_RESPONSE        csi_cancel_res;
    CSI_START_RESPONSE         csi_start_res;
    CSI_IDLE_RESPONSE          csi_idle_res;
    CSI_EJECT_ENTER            csi_eject_enter_res;
    CSI_SET_CLEAN_RESPONSE     csi_set_clean_res;
    CSI_SET_CAP_RESPONSE       csi_set_cap_res;
    CSI_SET_SCRATCH_RESPONSE   csi_set_scratch_res;
    CSI_DEFINE_POOL_RESPONSE   csi_define_pool_res;
    CSI_DELETE_POOL_RESPONSE   csi_delete_pool_res;
    CSI_MOUNT_SCRATCH_RESPONSE csi_mount_scratch_res;
    CSI_LOCK_RESPONSE          csi_lock_res;
    CSI_CLEAR_LOCK_RESPONSE    csi_clear_lock_res;
    CSI_QUERY_LOCK_RESPONSE    csi_query_lock_res;
    CSI_UNLOCK_RESPONSE        csi_unlock_res;
    CSI_REGISTER_RESPONSE      csi_register_res;
    CSI_UNREGISTER_RESPONSE    csi_unregister_res;
    CSI_CHECK_REGISTRATION_RESPONSE  csi_check_registration_res;
    CSI_DISPLAY_RESPONSE       csi_display_res;
    CSI_MOUNT_PINFO_RESPONSE   csi_mount_pinfo_res;
} CSI_RESPONSE;



/*
 *      external declarations for global variables
 */
extern QM_QID      csi_ni_out_qid;      /* network output queue */
extern long        csi_lmq_lastcleaned; /* time acslm connect queue cleaned */
extern int         csi_rpc_tcpsock;     /* rpc tcp service socket */
extern int         csi_rpc_udpsock;     /* rpc udp service socket */
extern BOOLEAN     csi_udp_rpcsvc;      /* TRUE if using RPC UDP server */
extern BOOLEAN     csi_tcp_rpcsvc;      /* TRUE if using RPC TCP server */
extern CSI_MSGBUF *csi_netbufp;         /* network packet buffer */
extern SVCXPRT    *csi_udpxprt;         /* CSI UDP transport handle */
extern SVCXPRT    *csi_tcpxprt;         /* CSI TCP transport handle */
extern QM_QID      csi_lm_qid;          /* ID of CSI connection queue */
extern IPC_HEADER  csi_ipc_header;      /* IPC header used to build packets */
extern int         csi_retry_tries;     /* number of times network retry */
extern char        csi_hostname[];      /* name of this host */
extern int         csi_pid;             /* process id this program */
extern int         csi_xexp_size;       /* track expected translation bytes */
extern int         csi_xcur_size;       /* track cur/actual translation bytes */
extern unsigned char csi_netaddr[];     /* address of this host */
extern int         csi_trace_flag;      /* flag to turn on packet tracing */
extern int         csi_broke_pipe;      /* broken pipe flag */

#ifdef XAPI
extern int         csi_xapi_conversion_flag; /* Convert ACSAPI-to-XAPI flag  */
extern void       *csi_xapi_control_table;   /* ACSAPI-to-XAPI control table */
#endif /* XAPI */


/*
 * Supports identifier translation for R3 cap-id being network compatible 
 * (backward compatible) with the R1,2 cap id's.  See csi_xidentif.c.
 */
extern VERSION     csi_active_xdr_version_branch;  

#ifdef ADI
extern int           csi_co_process_pid;  /* ADI co-process pid    */
extern unsigned char csi_client_name[]; /* OSLAN name of this host */
#endif /*ADI*/


extern CSI_HEADER  csi_ssi_rpc_addr;    /* CSI header to build ssi rpc packets*/
extern CSI_HEADER  csi_ssi_adi_addr;    /* CSI header to build ssi adi packets*/

extern int         csi_adi_ref;         /* ADI connection reference */

/* The following is only used by the SSI, but we removed the "#ifdef SSI" from */
/* here to prevent the need to recompile every file which includes this H file. */
extern long        csi_ssi_alt_procno_lm; /* alternate ssi program numbers */


/*
 *      external declarations for csi internal routines
 */

int     cl_chk_input(long tmo);
void    csi_fmtlmq_log(CSI_HEADER *cs_hdrp, char *stringp, int maxsize);
void    csi_fmtniq_log(CSI_MSGBUF *netbufp, char *stringp, int maxsize);
STATUS  csi_freeqmem(QM_QID queue_id, QM_MID member_id,
             CSI_VOIDFUNC log_fmt_func);
STATUS  csi_getiaddr(caddr_t addrp);
char    *csi_getmsg(CSI_MSGNO msgno);
STATUS  csi_hostaddr(char *hostname, unsigned char *addrp, int maxlen);
STATUS  csi_init(void);
STATUS  csi_ipcdisp(CSI_MSGBUF *netbufp);
void    csi_logevent(STATUS status, char *msg, char *caller,
             char *failed_func, char *source_file, int source_line);
STATUS  csi_net_send(CSI_MSGBUF *newpakp, CSI_NET_SEND_OPTIONS options);
STATUS  csi_netbufinit(CSI_MSGBUF **buffer);
void    csi_process(void);
void    csi_ptrace(register CSI_MSGBUF *msgbufp, unsigned long ssi_id,
           char *netaddr_strp, char *port_strp, char *dir);
STATUS  csi_qclean(QM_QID q_id, unsigned long agetime, CSI_VOIDFUNC log_func);
int     csi_qcmp(register QM_QID q_id, void *datap, unsigned int size);
STATUS  csi_qget(QM_QID q_id, QM_MID m_id, void **q_datap);
STATUS  csi_qinit(QM_QID *q_id, unsigned short max_members, char *name);
STATUS  csi_qput(QM_QID q_id, void *q_datap, int size, QM_MID *m_id);
STATUS  csi_rpccall(CSI_MSGBUF *netbufp);
void    csi_rpcdisp(struct svc_req *reqp, SVCXPRT *xprtp);
int     csi_rpcinput(SVCXPRT *xprtp, xdrproc_t inproc, CSI_MSGBUF *inbufp,
             xdrproc_t outproc, CSI_MSGBUF *outbufp,
             xdrproc_t free_rtn);
STATUS  csi_rpctinit(void);
unsigned long csi_rpctransient(unsigned long proto, unsigned long vers,
                   int *sockp, struct sockaddr_in *addrp);
STATUS  csi_rpcuinit(void);
void    csi_shutdown(void);
void    csi_sighdlr(int sig);
int     csi_ssicmp(CSI_XID *xid1, CSI_XID *xid2);
STATUS  csi_ssi_api_resp(CSI_MSGBUF *netbufp, STATUS response_status);
STATUS  csi_svcinit(void);

void sighdlr(int signum);


/*
 *      external declarations for XDR conversion routines
 */


bool_t csi_xaccess_id(XDR *xdrsp, ACCESSID *accessidp);
bool_t csi_xacs(XDR *xdrsp, ACS *acsp);
bool_t csi_xcap(XDR *xdrsp, CAP *capp);
bool_t csi_xcap_id(XDR *xdrsp, CAPID *capidp);
bool_t csi_xcap_mode(XDR *xdrsp, CAP_MODE *cap_mode);
bool_t csi_xcell_id(XDR *xdrsp, CELLID *cellidp);
bool_t csi_xcol(XDR *xdrsp, COL *colp);
bool_t csi_xcommand(XDR *xdrsp, COMMAND *comp);
bool_t csi_xcsi_hdr(XDR *xdrsp, CSI_HEADER *csi_hdr);
bool_t csi_xdrive(XDR *xdrsp, DRIVE *drivep);
bool_t csi_xdrive_id(XDR *xdrsp, DRIVEID *driveidp);
bool_t csi_xdrive_type(XDR *xdrsp, DRIVE_TYPE *drive_type);
bool_t csi_xfreecells(XDR *xdrsp, FREECELLS *freecells);
bool_t csi_xgrp_type(XDR *xdrsp, GROUP_TYPE *grptype);
int csi_xidcmp(CSI_XID *xid1, CSI_XID *xid2);
bool_t csi_xidentifier(XDR *xdrsp, IDENTIFIER *identifp, TYPE type);
bool_t csi_xipc_hdr(XDR *xdrsp, IPC_HEADER *ipchp);
bool_t csi_xlm_request(XDR *xdrsp, CSI_MSGBUF *bufferp);
bool_t csi_xlm_response(XDR *xdrsp, CSI_MSGBUF *bufferp);
bool_t csi_xlocation(XDR *xdrsp, LOCATION *locp);
bool_t csi_xlockid(XDR *xdrsp, LOCKID *lockid);
bool_t csi_xlsm(XDR *xdrsp, LSM *lsmp);
bool_t csi_xlsm_id(XDR *xdrsp, LSMID *lsmidp);
bool_t csi_xmedia_type(XDR *xdrsp, MEDIA_TYPE *media_type);
bool_t csi_xmsg_hdr(XDR *xdrsp, MESSAGE_HEADER *msghp);
bool_t csi_xmsg_id(XDR *xdrsp, MESSAGE_ID *msgid);
bool_t csi_xpnl(XDR *xdrsp, PANEL *pnlp);
bool_t csi_xpnl_id(XDR *xdrsp, PANELID *pnlidp);
bool_t csi_xpool(XDR *xdrsp, POOL *pool);
bool_t csi_xpool_id(XDR *xdrsp, POOLID *poolidp);
bool_t csi_xport(XDR *xdrsp, PORT *portp);
bool_t csi_xport_id(XDR *xdrsp, PORTID *portidp);
bool_t csi_xptp_id(XDR *xdrsp, PTPID *ptpidp);
bool_t csi_xqu_response(XDR *xdrsp, CSI_QUERY_RESPONSE *resp);
bool_t csi_xquv0_response(XDR *xdrsp, CSI_V0_QUERY_RESPONSE *resp);
bool_t csi_xreq_hdr(XDR *xdrsp, CSI_REQUEST_HEADER *req_hdr);
bool_t csi_xreqsummary(XDR *xdrsp, REQ_SUMMARY *sump);
bool_t csi_xres_status(XDR *xdrsp, RESPONSE_STATUS *rstatp);
bool_t csi_xrow(XDR *xdrsp, ROW *rowp);
bool_t csi_xsockname(XDR *xdrsp, char *socknamep);
bool_t csi_xspnl_id(XDR *xdrsp, SUBPANELID *spidp);
bool_t csi_xstate(XDR *xdrsp, STATE *state);
bool_t csi_xstatus(XDR *xdrsp, STATUS *status);
bool_t csi_xtype(XDR *xdrsp, TYPE *type);
bool_t csi_xv0_cap_id(XDR *xdrsp, V0_CAPID *capidp);
bool_t csi_xv0_req(XDR *xdrsp, CSI_V0_REQUEST *reqp);
bool_t csi_xv0_res(XDR *xdrsp, CSI_V0_RESPONSE *resp);
bool_t csi_xv0quresponse(XDR *xdrsp, CSI_V0_QUERY_RESPONSE *resp);
bool_t csi_xv1_cap_id(XDR *xdrsp, V1_CAPID *capidp);
bool_t csi_xv1_req(XDR *xdrsp, CSI_V1_REQUEST *reqp);
bool_t csi_xv1_res(XDR *xdrsp, CSI_V1_RESPONSE *resp);
bool_t csi_xv1quresponse(XDR *xdrsp, CSI_V1_QUERY_RESPONSE *resp);
bool_t csi_xv2_req(XDR *xdrsp, CSI_V2_REQUEST *reqp);
bool_t csi_xv2_res(XDR *xdrsp, CSI_V2_RESPONSE *resp);
bool_t csi_xv2quresponse(XDR *xdrsp, CSI_V2_QUERY_RESPONSE *resp);
bool_t csi_xv4_req(XDR *xdrsp, CSI_REQUEST *reqp);
bool_t csi_xv4_res(XDR *xdrsp, CSI_RESPONSE *resp);
bool_t csi_xversion(XDR *xdrsp, VERSION *version);
bool_t csi_xvol_id(XDR *xdrsp, VOLID *volidp);
bool_t csi_xvol_status(XDR *xdrsp, VOLUME_STATUS *vstatp);
bool_t csi_xvolrange(XDR *xdrsp, VOLRANGE *vp);

bool_t csi_xevent_reg_status(XDR *xdrsp,EVENT_REGISTER_STATUS *ev_reg_stat_ptr,int *total);
bool_t csi_xevent_rsrc_status(XDR *xdrsp,EVENT_RESOURCE_STATUS *ev_rsrc_stat);
bool_t csi_xevent_vol_status(XDR *xdrsp,EVENT_VOLUME_STATUS *ev_vol_stat_ptr);
bool_t csi_xhand_id(XDR *xdrsp,HANDID *hand_id_ptr);
bool_t csi_xregister_status(XDR *xdrsp,REGISTER_STATUS *reg_stat);
bool_t csi_xresource_data(XDR *xdrsp,RESOURCE_DATA *res_dta,RESOURCE_DATA_TYPE dta_type);
bool_t csi_xregistration_id(XDR *xdrsp,REGISTRATION_ID *reg_id_ptr);
bool_t csi_xsense_fsc(XDR *xdrsp,SENSE_FSC *sense_fsc_ptr);
bool_t csi_xsense_hli(XDR *xdrsp,SENSE_HLI *sense_hli);
bool_t csi_xsense_scsi(XDR *xdrsp,SENSE_SCSI *sense_scsi);
bool_t csi_xserial_num(XDR *xdrsp,SERIAL_NUM *serial_num_ptr);
bool_t csi_xxml_data(XDR *xdrsp,DISPLAY_XML_DATA *xml_data_ptr);
bool_t csi_xxgrp_type(XDR *xdrsp,GROUP_TYPE *grptype);
bool_t csi_xevent_drive_status(XDR *xdrsp, EVENT_DRIVE_STATUS *ev_drive_stat);
bool_t csi_xdrive_data(XDR *xdrsp, DRIVE_ACTIVITY_DATA *drive_data);

#endif /*_CSI_*/
